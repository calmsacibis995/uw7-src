/*		copyright	"%c%" 	*/

#ident	"@(#)adt_lvlout.c	1.2"
#ident  "$Header$"

/*
 * This file mirrors the code of the lvlout(3C) library routine 
 * in libc (libc-port:gen/lvlout.c).  It handles auditrpt's local 
 * LTDB the same way that lvlout(3C) handles the master LTDB in 
 * /etc/security/mac.  Any alterations done to the master LTDB 
 * that cause lvlout(3C) to change, should also be reflected in 
 * this code.
 */

/* LINTLIBRARY */
#include <sys/types.h>
#include <sys/param.h>
#include <mac.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <unistd.h>

/* variables set to the location of the map files */
extern char	*lid_fp;	/* lid.internal */
extern char	*alias_fp;	/* ltf.alias */
extern char	*ctg_fp;	/* ltf.cat */
extern char	*class_fp;	/* ltf.class */

/*
 * The following routines are static to this file, and used by the 
 * routine adt_lvlout.
 */
static int	lvl_lid_to_struct();
static int	lid_to_alias();
static int	cnv_lvl_to_name();
static int	cnv_lid_to_string();
static char	*index_to_name();

#define	INITBUFSIZ	512
#define	ELIDSTR		-100

/*
 * lvlout	- translate a level from internal format to text format
 *
 * Notes:
 *	1. if LID is inactive, the decimal value of the LID number is used.
 *	2. if alias is not defined for LID, the decimal value of the LID
 *	   number is used.
 *	3. use external errno cautiously; it may be altered by other calls.
 *	4. error ELIDSTR is a special error case to denote that a
 *	   LID to (decimal) string conversion took place.
 *
 * Return:
 *	1	- conversion from LID to (decimal) string
 *	0	- success
 *	-1	- EINVAL: format is not valid; LID is not valid
 *		- EACCES: cannot open LTDB
 *		- ENOSPC: level name string is larger than bufsize
 */
int
adt_lvlout(levelp, bufp, bufsize, format)
	const level_t	*levelp;		/* lid ptr */
	char		*bufp;			/* user buffer */
	int		bufsize;		/* buffer size */
	int		format;			/* LVL_ALIAS or LVL_FULL */
{
	int		strsize;		/* actual string size */
	char		*name;			/* temp name string ptr */
	int		err;			/* temporary error */
	int		retval = 0;		/* return value */
	struct mac_level buf;		/* static level buffer */
	register struct mac_level *lvlp = &buf;	/* level structure ptr */

	/* okay to use errno here */
	if ((errno = lvl_lid_to_struct(*levelp, lvlp)) != 0)
		return(-1);

	switch(format) {
	case LVL_ALIAS:
		err = (lvlp->lvl_valid == LVL_ACTIVE)
			? lid_to_alias(*levelp, &name)
			: cnv_lid_to_string(*levelp, &name);
		break;

	case LVL_FULL:
		err = (lvlp->lvl_valid == LVL_ACTIVE)
			? cnv_lvl_to_name(lvlp, &name)
			: cnv_lid_to_string(*levelp, &name);
		break;

	default:
		err = EINVAL;
		break;
	}

	if (err == ELIDSTR) {
		err = 0;
		retval = 1;
	}

	if (err == 0) {
		strsize = strlen(name) + 1;
		if (bufsize == 0) {
			errno = 0;
			return(strsize);
		} else if (bufsize < strsize)
			errno = ENOSPC;
		else {
			(void)strcpy(bufp, name);
			errno = 0;
		}
	} else
		errno = err;

	return(errno ? -1 : retval);
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
	struct mac_level *lvlp;		/* level structure to use */
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
 * lid_to_alias	- get the alias name for LID number
 *
 * Notes:
 *	1. if alias is not defined, use the decimal value string for the LID.
 *
 * Return:
 *	0	- success
 *	EACCES	- cannot open LTDB
 */
static int
lid_to_alias(lid, namep)
	level_t	lid;
	char	**namep;
{
	int	rfd;		/* alias read file descriptor */
	int	retval = 0;	/* return value */

	if ((rfd = open(alias_fp, O_RDONLY, 0)) == -1)
		return(EACCES);
	
	if ((*namep = index_to_name(rfd, (ulong)lid))
	==  (char *)NULL)
		retval = cnv_lid_to_string(lid, namep);

	(void)close(rfd);
	return(retval);
}

/*
 * cnv_lvl_to_name	- get level name for level structure
 *
 * Notes:
 *	1. a statically allocated buffer is used to contain the string;
 *	   if not large enough, memory is allocated to store the string.
 *	2. If all category bits are turned on, LVL_ALLCATS is returned.
 *
 * Return:
 *	0	- success
 *	EACCES	- cannot open LTDB
 *	EINVAL	- component of level name no longer named; level name string
 *		  is too large
 */
static int
cnv_lvl_to_name(lvlp, namep)
	struct mac_level *lvlp;
	char	**namep;
{
	register ushort	*catsigp;		/* ptr to catsig */
	register ulong	*catp;			/* ptr to cat */
	register int	i;			/* bit counter */
	register int	rfd;			/* read file descriptor */
	int		allcats = 1;		/* all categories on */
	int		need_l;			/* needed length */
	int		firstcat = 1;		/* first category */
	char		*name;			/* ptr to a name */
	static char	*lvlname = NULL;	/* ptr to allocated string */
	static int	lvllength = INITBUFSIZ;	/* size of allocated string */

	/* get the classification string */
	if ((rfd = open(class_fp, O_RDONLY, 0)) == -1)
		return(EACCES);
	
	if ((name = index_to_name(rfd, (ulong)lvlp->lvl_class))
	==  (char *)NULL) {
		(void)close(rfd);
		return(EINVAL);
	}

	(void)close(rfd);
	if (lvlname == NULL) {
		if ((lvlname = malloc(lvllength)) ==  (char *)NULL) {
			(void)close(rfd);
			return(EINVAL);
		}
	}
	(void)strcpy(lvlname, name);

	/* get categories */

	/*
	 * If all category bits are on, return LVL_ALLCATS.
	 */
	for (i = 0, catp = &lvlp->lvl_cat[0]; i < CAT_SIZE; i++, catp++) {
		if (*catp != 0xffffffff) {
			allcats = 0;
			break;
		}
	}

	if (allcats) {	/* use generic name for all categories */
		/* plenty of space is available, don't need to malloc */
		(void)strcat(strcat(lvlname, ":"), LVL_ALLCATS);
	} else { /* must name each category */
		if ((rfd = open(ctg_fp, O_RDONLY, 0)) == -1)
			return(EACCES);

		for (catsigp = &lvlp->lvl_catsig[0], catp = &lvlp->lvl_cat[0];
		     *catsigp != 0; catsigp++, catp++) {
			for (i = 0; i < NB_LONG; i++) {
				if (*catp & (((unsigned long)1 << (NB_LONG - 1)) >>i)) {
					/* get a category string */
					if ((name=index_to_name(rfd,(ulong)(1+
					   ((ulong)(*catsigp-1)<<CAT_SHIFT)+i)))
					==  (char *)NULL) {
						(void)close(rfd);
						return(EINVAL);
					}
					need_l=strlen(lvlname)+strlen(name)+1;
					if (need_l > lvllength) {
						char	*tmp_s;
						int	tmp_l = lvllength;

						do {
							tmp_l *= 2;
						} while (need_l > tmp_l);
						if ((tmp_s = malloc(tmp_l))
						==  (char *)NULL) {
							(void)close(rfd);
							return(EINVAL);
						}
						(void)strcpy(tmp_s, lvlname);
						free(lvlname);
						lvlname = tmp_s;
						lvllength = tmp_l;
					}
					if (firstcat) {
						(void)strcat(lvlname, ":");
						firstcat = 0;
					} else
						(void)strcat(lvlname, ",");
					(void)strcat(lvlname, name);
				} /* end-if catp */
			} /* end-for till CATSIZE */
		} /* end-for catsig */

		(void)close(rfd);
	} /* end-else each category */

	*namep = lvlname;
	return(0);
}

/*
 * cnv_lid_to_string	- convert LID number to string
 *
 * Return:
 *	ELIDSTR	- unique notification of LID to string conversion
 */
static int
cnv_lid_to_string(lid, namep)
	level_t	lid;
	char	**namep;
{
	static char	buf[LVL_NUMSIZE];

	(void) sprintf(buf, "%ld", lid);
	*namep = buf;
	return(ELIDSTR);
}

/*
 * index_to_name	- get name to index of input file
 *
 * Notes:
 *	1. a static buffer is used to contain the name string; it is the
 *	   caller's responsibility to save the string, if necessary.
 *
 * Return:
 *	NULL	- cannot find name at index; cannot seek through file
 *	ptr	- pointer to string containing name at index
 */
static char *
index_to_name(rfd, index)
	int	rfd;
	ulong	index;
{
	static char name[LVL_NAMESIZE];	/* statically allocated name string */

	/* seek to name indexed by index */
	if (lseek(rfd, index<<LVL_NAMESHIFT, 0) == -1)
		return((char *)NULL);
	
	/* make sure definition exists */
	if ((read(rfd, name, LVL_NAMESIZE) == LVL_NAMESIZE)
	&&  (name[0] != '\0'))
		return(name);
	
	return((char *)NULL);
}
