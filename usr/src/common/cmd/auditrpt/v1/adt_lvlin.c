/*		copyright	"%c%" 	*/

#ident	"@(#)adt_lvlin.c	1.2"
#ident  "$Header$"

/*
 * This file mirrors the code of the lvlin(3C) library routine 
 * in libc (libc-port:gen/lvlin.c).  It handles auditrpt's local 
 * LTDB the same way that lvlin(3C) handles the master LTDB in 
 * /etc/security/mac.  Any alterations done to the master LTDB 
 * that cause lvlin(3C) to change, should also be reflected in 
 * this code.
 */

/* LINTLIBRARY */
#include <sys/types.h>
#include <sys/param.h>
#include <mac.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* variables set to the location of the map files */
extern char	*lid_fp;	/* lid.internal */
extern char	*alias_fp;	/* ltf.alias */
extern char	*ctg_fp;	/* ltf.cat */
extern char	*class_fp;	/* ltf.class */

/*
 * The following routines are static to this file, and used by the
 * routine adt_lvlin.
 */
static int	alias_to_lid();
static int	lvl_lid_to_struct();
static int	level_to_lid();
static int	isa_name();
static ulong	name_to_index();
static int	turnon_cat();
static void	adjust_lvl();
static int	lvl_struct_to_lid();

#define	DELIMITERS	"\t\n :;,+-"

/*
 * lvlin	- translate a level from text format to internal format
 *
 * Notes:
 *	1. try alias name first, if possible; if not an alias, try fully
 *	   qualified level name.
 *	2. LID must be valid active for any success.
 *	3. use external errno cautiously; it may be set by other calls.
 *	4. set user's LID only when successful.
 *
 * Return:
 *	0	- success
 *	-1	- EINVAL: level name is invalid
 *		- EACCES: cannot open LTDB
 */
int
adt_lvlin(bufp, levelp)
	const char	*bufp;
	level_t		*levelp;
{
	level_t		lid;			/* temporary lid */
	int		err;			/* temporary err */

	/* not yet found */
	err = EINVAL;

	/*
	 * Try alias first, but only if the buffer is possibly an
	 * alias.
	 */
	if ((strlen(bufp)<=(size_t)LVL_MAXNAMELEN)
	&&  (strchr(bufp, ':')==(char *)NULL))
		err = alias_to_lid(bufp, &lid);
	
	if (err == EINVAL) /* it must be a fully qualified level name */
		err = level_to_lid(bufp, &lid);
	
	errno = err;		/* save to use errno */
	if (errno == 0) {
		*levelp = lid;
		return(0);
	}
	return(-1);
}

/*
 * alias_to_lid	- get the LID number for alias name
 *
 * Notes:
 *	1. open alias file, find lid with matching alias name, and make
 *	   sure LID is valid active.
 *
 * Return:
 *	0	- success
 *	EINVAL	- alias is not a valid name
 *		- LID for alias is not valid active
 *	EACCES	- cannot open LTDB
 *	
 */
static int
alias_to_lid(name, lidp)
	char	*name;
	level_t	*lidp;
{
	int	rfd;				/* read file descriptor */
	int	retval = 0;			/* return value */
	struct mac_level buf;		/* level structure buffer */
	register struct mac_level *lvlp = &buf;	/* level structure ptr */

	if (!isa_name(name))
		return(EINVAL);

	if ((rfd = open(alias_fp, O_RDONLY, 0)) == -1)
		return(EACCES);
	
	if ((*lidp = (level_t)name_to_index(rfd, name)) == (level_t)0) {
		(void)close(rfd);
		return(EINVAL);
	}

	(void)close(rfd);

	/* consistency check to make sure LID is valid active */
	if ((retval = lvl_lid_to_struct(*lidp, lvlp)) != 0)
		return(retval);
	if (lvlp->lvl_valid != LVL_ACTIVE)
		return(EINVAL);
	
	/* all is well */
	return(0);
}

/*
 * lvl_lid_to_struct	- convert LID to level structure
 *
 * Notes:
 *	1. open lid.internal, seek to entry for LID, and read entry.
 *	2. level must be valid; may be active or inactive.
 *
 * Return:
 *	0	- success
 *	EINVAL	- LID is 0, reserved
 *		- cannot seek through lid.internal
 *		- LID is not valid
 *	EACCES	- cannot open LTDB
 */
static int
lvl_lid_to_struct(lid, lvlp)
	level_t	lid;			/* lid to convert */
	struct mac_level *lvlp;		/* structure to use */
{
	int	rfd;			/* read file descriptor */

	/* 0 reserved */
	if (lid == (level_t)0)
		return(EINVAL);

	if ((rfd = open(lid_fp, O_RDONLY, 0)) == -1)
		return(EACCES);
	
	if (lseek(rfd, lid*sizeof(struct mac_level), 0) == -1) {
		(void)close(rfd);
		return(EINVAL);
	}

	/* level must be valid; may be active or inactive */
	if ((read(rfd, lvlp, sizeof(struct mac_level))
		!= sizeof(struct mac_level))
	||  (lvlp->lvl_valid == LVL_INVALID)) {
		(void)close(rfd);
		return(EINVAL);
	}

	(void)close(rfd);
	return(0);
}

/*
 * level_to_lid	- get LID for level name
 *
 * Notes:
 *	1. convert classification and categories to a level structure,
 *	   and search for its LID in lid.internal.
 *	2. if a category name is used more than once in the level name,
 *	   the level name is considered invalid.
 *	3. If LVL_ALLCATS is specified, all category bits are turned
 *	   on.
 *
 * Return:
 *	0	- success
 *	EINVAL	- a name component of level is not a valid name
 *		- level name is not valid active
 *	EACCES	- cannot open LTDB
 */
static int
level_to_lid(bufp, lidp)
	register char	*bufp;			/* level name */
	level_t		*lidp;			/* store at lid ptr */
{
	ushort		classnum;		/* class number */
	register ushort	catnum;			/* cat number */
	register int	rfd;			/* read file descriptor */
	register int	namelen;		/* length of name */
	char	nbuf[LVL_NAMESIZE];	/* name storage */
	register char	*name = nbuf;		/* name ptr */
	struct mac_level buf;		/* level structure buffer */
	register struct mac_level *lvlp = &buf;	/* level structure ptr */

	/* parse classification */
	if ((namelen = strcspn(bufp, ":")) > LVL_MAXNAMELEN)
		return(EINVAL);
	(void)strncpy(name, bufp, namelen);
	name[namelen] = '\0';
	bufp += namelen;
	
	if (!isa_name(name))
		return(EINVAL);

	if ((rfd = open(class_fp, O_RDONLY, 0)) == -1)
		return(EACCES);
	
	if ((classnum = (ushort)name_to_index(rfd, name))
	==  (ushort)0) {
		(void)close(rfd);
		return(EINVAL);
	}
	
	(void)close(rfd);

	/* zero static buffer before use */
	(void)memset((void *)lvlp, 0, (size_t)sizeof(struct mac_level));
	lvlp->lvl_class = classnum;

	if (*bufp) { /* parse categories */
		/*
		 * If LVL_ALLCATS is specified, all category bits are
		 * turned on.
		 */
		if (strcmp(bufp+1, LVL_ALLCATS) == 0) {
			for (catnum = 1; catnum <= MAXCATS; catnum++)
				(void)turnon_cat(catnum, lvlp);
		} else { /* must parse individual categories */
			if ((rfd = open(ctg_fp, O_RDONLY, 0)) == -1)
				return(EACCES);
		
			while (*bufp) {
				bufp++;
				if ((namelen = strcspn(bufp, ","))
				>  LVL_MAXNAMELEN) {
					(void)close(rfd);
					return(EINVAL);
				}
				(void)strncpy(name, bufp, namelen);
				name[namelen] = '\0';
				bufp += namelen;
			
				if (!isa_name(name)
				||  ((catnum = (ushort)name_to_index(rfd, name))
				    == (ushort)0)) {
					(void)close(rfd);
					return(EINVAL);
				}
			
				if (turnon_cat(catnum, lvlp)) {
					(void)close(rfd);
					return(EINVAL);
				}
			} /* end-while */
				
			(void)close(rfd);
		} /* end-else individual categories */

		adjust_lvl(lvlp);	/* adjust bits for performance */
	} /* end-if parse categories */

	/* find level in internal file */
	return(lvl_struct_to_lid(lvlp, lidp));
}

/*
 * isa_name	- check if input string is a valid name
 *
 * Notes:
 *	1. string must not be empty, may not start with a number, must be
 *	   within LVL_MAXNAMELEN in length, may not contain delimiter
 *	   characters.
 *
 * Return:
 *	1	- input string is a valid name
 *	0	- input string is not a valid name
 */
static int
isa_name(name)
	register char	*name;
{
	/* check for empty string */
	if (*name == '\0')
		return(0);

	/* name may not start with a number */
	if ((*name >= '0') && (*name <= '9'))
		return(0);
	
	/* name may not exceed LVL_MAXNAMELEN characters */
	if (strlen(name) > (size_t)LVL_MAXNAMELEN)
		return(0);
	
	/* no characters in name may contain delimiters */
	if (strpbrk(name, DELIMITERS))
		return(0);
	
	return(1);
}

/*
 * name_to_index	- get index for name in input file
 *
 * Notes:
 *	1. seek to the first legal entry, search for name, and return index.
 *
 * Return:
 *	index	- index in file containing name
 *	0	- name does not exist in file
 */
static ulong
name_to_index(rfd, name)
	register int	rfd;			/* file descriptor */
	register char	*name;			/* name */
{
	register ulong	index;			/* LID index */
	register int	cnt;			/* read count */
	char	buf[LVL_NAMESIZE];	/* name buffer */
	register char	*bufp = buf;		/* ptr to name buf */

	/* 0 is reserved */
	if (lseek(rfd, LVL_NAMESIZE, 0) == -1)
		return(0);

	for (index = 1, cnt = read(rfd, bufp, LVL_NAMESIZE);
	     cnt == LVL_NAMESIZE;
	     index++, cnt = read(rfd, bufp, LVL_NAMESIZE))
		if (strcmp(bufp, name) == 0)
			return(index);

	return(0);
}

/*
 * turnon_cat	- turn on bits in level structure for category number
 *
 * Return:
 *	-1	- category is already in use
 *	0	- success
 */
static int
turnon_cat(catnum, lvlp)
	ushort	catnum;			/* category number */
	struct mac_level *lvlp;		/* level structure to use */
{
	ulong	catmask;		/* category mask */
	ushort	catindex;		/* category index */

	/* categories start at 1; adjust for bit manipulation */
	catnum--;

	/* get index and mask for category */
	catindex = (ushort)(catnum>>LONG_SHIFT);
	catmask = (((unsigned long)1 << ((NB_LONG - 1)))>>(catnum&(NB_LONG - 1)));

	/* if already set, return failure */
	if (lvlp->lvl_cat[catindex] & catmask)
		return(-1);
	
	/* set category */
	lvlp->lvl_cat[catindex] |= catmask;
	lvlp->lvl_catsig[catindex] = 1;
	return(0);
}

/*
 * adjust_lvl	- adjust level structure for performance
 *
 * Notes:
 *	1. The catsig and cat arrays are packed so that the significant
 *	   data is kept together.
 */
static void
adjust_lvl(lvlp)
	register struct mac_level *lvlp;	/* level structure */
{
	register int	i;			/* loop counters */
	register int	j;

	/* compact categories */
	for (i = 0, j = 0; i < CAT_SIZE; i++) {
		if (lvlp->lvl_catsig[i]) {
			lvlp->lvl_catsig[j] = i + 1;	/* starts at 1 */
			if (i != j)
				lvlp->lvl_cat[j] = lvlp->lvl_cat[i];
			j++;
		}
	}

	/* zero out the rest */
	while (j < CAT_SIZE) {
		lvlp->lvl_catsig[j] = (ushort)0;
		lvlp->lvl_cat[j] = (ulong)0;
		j++;
	}
}

/*
 * lvl_struct_to_lid	- get lid for level structure
 *
 * Notes:
 *	1. Entry must be valid active.
 *
 * Return:
 *	0	- success
 *	EINVAL	- cannot seek through LTDB internal file
 *		- entry does not exist in LTDB internal file
 *		  (i.e., not valid-active)
 *	EACCES	- cannot open LTDB
 */
static int
lvl_struct_to_lid(lvlp, lidp)
	register struct mac_level *lvlp;	/* level structure */
	level_t		*lidp;			/* store at lid ptr */
{
	register int	rfd;			/* read file descriptor */
	register ulong	index;			/* index in file */
	register int	cnt;			/* read count */
	register int	i;			/* loop counter */
	struct mac_level buf;		/* static level buffer */
	register struct mac_level *bufp = &buf;	/* level structure ptr */

	if ((rfd = open(lid_fp, O_RDONLY, 0)) == -1)
		return(EACCES);
	
	/* 0 is reserved */
	if (lseek(rfd, sizeof(struct mac_level), 0) == -1) {
		(void)close(rfd);
		return(EINVAL);
	}

	for (index = 1, cnt = read(rfd, bufp, sizeof(struct mac_level));
	     cnt == sizeof(struct mac_level);
	     index++, cnt = read(rfd, bufp, sizeof(struct mac_level))) {
		if (bufp->lvl_valid != LVL_ACTIVE)
			continue;
		if (lvlp->lvl_class != bufp->lvl_class)
			continue;
		for (i = 0; i <= CAT_SIZE; i++) {
			if (lvlp->lvl_catsig[i] != bufp->lvl_catsig[i])
				break;
			if (bufp->lvl_catsig[i] == 0) {
				(void)close(rfd);
				*lidp = (level_t)index;
				return(0);
			}
			if (lvlp->lvl_cat[i] != bufp->lvl_cat[i])
				break;
		}
	}

	(void)close(rfd);
	return(EINVAL);
}
