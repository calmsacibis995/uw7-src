#ident  "@(#)uidage.c	1.3"
#ident  "$Header$"

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<sys/mac.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<string.h>
#include 	<signal.h>
#include 	<errno.h>
#include 	<time.h>
#include	<shadow.h>
#include	<priv.h>
#include	<pfmt.h>
#include	<locale.h>
#include	"uidage.h"
#include	"messages.h"

extern	int	link(),
		chown(),
		access(),
		unlink(),
		ck_and_set_flevel();

extern	int	errno;

extern	long	strtol();

extern	void	exit();

static	struct	uidage	uidage,
			*getuaent(),
			*fgetuaent();

static	int	putuaent();

static	char	*uaskip();

static	void	uid_bcom(),
		uabad_news(),
		closeuaent();

static	FILE	*uaf;

/*
 * Procedure:	uid_age
 *
 * Restrictions:
 *		stat(2):	None
 *		fopen:		None
 *		fclose:		None
 *		unlink(2):	None
 *		chown(2):	None
 *		access(2):	None
 *		rename(2):	None
 *		link(2):	None
 *
 * Notes:	can be called in one of three ways:
 *
 *			ADD	- adds a uid to the aged uid file,
 *
 *			REM	- removes a uid from the aged uid file,
 *
 *			CHECK	- given a uid, determines if it exists
 *				  in the file.
 *
 *		NOTE:	REM and ADD both have the capability of removing
 *			entries that have aged long enough from the file.
*/
int
uid_age(cmd, uid, i_uidage)
	int	cmd;
	uid_t	uid;
	int	i_uidage;
{
	int	ret = 0,
		found = 0,
		i, error = 0,
		a_cmd = 0, c_cmd = 0,
		r_cmd = 0, end_of_file = 0;
	FILE	*fp_utemp;
	time_t	today,
		future;
	struct	uidage	tmp_uid;
	struct	stat	statbuf;
	struct	uidage	uage,
			*ua_ptr = &uage;

	errno = 0;
	uaf = (FILE *)NULL;

	switch(cmd) {
		case ADD:
			a_cmd = 1;
			break;
		case CHECK:
			c_cmd = 1;
			break;
		case REM:
			r_cmd = 1;
			break;
	}
	if (!c_cmd) {
		if (stat(UIDAGEF, &statbuf) < 0) {
			/*
			 * the "ageduid" file does not exist.  If the cmd
			 * is ADD, then set end_of_file to 1 so the while
			 * loop isn't entered and get the attributes of the
			 * SHADOW file to use for the "new" ageduid file.
			*/
			if (a_cmd) {
				end_of_file = 1;
				(void) stat(SHADOW, &statbuf);
			}
			else {
				return 1;
			}
		}
	
		(void) umask(~(statbuf.st_mode & S_IRUSR));
	
	 	if ((fp_utemp = fopen(UATEMP , "w")) == NULL) {
			errmsg (M_UID_AGING_UNCHANGED);
			return 1;
		}
	
		today = DAY_NOW;
		future = (i_uidage * 30);
	}

	/* The while loop for reading the UIDAGEF entries */
	while (!end_of_file) {		
		if ((ua_ptr = (struct uidage *)getuaent()) != NULL) {
			if (c_cmd) {
				if (ua_ptr->uid == uid) {
					found = 1;
					break;	/* exit the while loop */
				}
				continue;
			}
			if ((ua_ptr->age > 0) && (ua_ptr->age < today)) {
				if (r_cmd) {
					ret = 1;
				}
				continue;
			}
			if (putuaent(ua_ptr, fp_utemp)) {
				closeuaent();
				(void) fclose(fp_utemp);
				(void) unlink(UATEMP);
				errmsg (M_UID_AGING_UNCHANGED);
				return 1;
			}
		}
		else { 
			if (errno == 0)		/* end of file */
				end_of_file = 1;
			else if (errno == EINVAL) {
				error++;
				errno = 0;
			}
			else		/* unexpected error found */
				end_of_file = 1;		
		}
	}

	if (error >= 1) {
		errmsg (M_BADENTRY_IN_AGEDUID);
	}

	if (c_cmd) {
		closeuaent();
		return !found;
	}

	if (a_cmd) {
		if (i_uidage != 0) {
			tmp_uid.uid = uid;
			if (i_uidage > 0) {
				tmp_uid.age = today + future;
			}
			else if (i_uidage == -1) {
				tmp_uid.age = i_uidage;
			}

			if (putuaent(&tmp_uid, fp_utemp)) {
				closeuaent();
				(void) fclose(fp_utemp);
				(void) unlink(UATEMP);
				errmsg (M_UID_AGING_UNCHANGED);
				return 1;
			}
		}
		ret = 0;
	}

	closeuaent();
	(void) fclose(fp_utemp);

	if (ck_and_set_flevel(statbuf.st_level, UATEMP)) {
		(void) unlink(UATEMP);
		errmsg (M_UID_AGING_UNCHANGED);
		return 1;
	}

	if (chown(UATEMP, statbuf.st_uid, statbuf.st_gid) < 0) {
		(void) unlink(UATEMP);
		errmsg (M_UID_AGING_UNCHANGED);
		return 1;
	}

	if (access(UIDAGEF, 0) == 0) {
		if (access(OUIDAGEF, 0) == 0) {
			(void) unlink(OUIDAGEF);
		}
		if (rename(UIDAGEF, OUIDAGEF) == -1) {
			errmsg (M_UID_AGING_UNCHANGED);
			return 1;
		}
	}

	if (rename(UATEMP, UIDAGEF) == -1) {
		if (unlink(UIDAGEF) < 0) {
			if (link(OUIDAGEF, UIDAGEF) < 0) {
				uabad_news();
			}
		}
		errmsg (M_UID_AGING_UNCHANGED);
		return 1;
	}
	return ret;	
}


/*
 * Procedure:	getuaent
 *
 * Restrictions:
 *		fopen:		None
*/
static	struct uidage *
getuaent()
{
	struct uidage *fgetuaent();

	if (uaf == NULL) {
		if ((uaf = fopen(UIDAGEF, "r")) == NULL)
			return NULL;
	}
	return fgetuaent(uaf);
}


static	struct uidage *
fgetuaent(f)
	FILE *f;
{
	char	line[BUFSIZE],
		*linep = &line[0];
	register char *p;
	char *end;
	long x;

	p = fgets(linep, BUFSIZE, f);

	if (p == NULL)
		return NULL;
	x = strtol(p,&end,10);
	if (end!= memchr(p,':',strlen(p))){
		errno = EINVAL;
		return NULL;
	}
	uidage.uid = x;
	p = uaskip(p);
	x = strtol(p,&end,10);
	uidage.age = x;
	(void) uaskip(p);

	return &uidage;
}

static void
closeuaent()
{

	if ( uaf ) {
		(void) fclose(uaf);
		uaf = (FILE *)NULL;
	}

	return; 
}

static char *
uaskip(p)
register char *p;
{
	while(*p && *p != ':' && *p != '\n')
		++p;
	if (*p == '\n')
		*p = '\0';
	else if (*p)
		*p++ = '\0';
	return p;
}


static	int
putuaent(ua,f)
register const struct uidage *ua;
register FILE *f;
{
	(void) fprintf(f,"%ld:%ld\n", ua->uid, ua->age);
	(void) fflush(f);
	return ferror(f);
}


/*
 * Procedure:	uabad_news
 *
 * Restrictions:
 *		ulckpwdf:	None
*/
static	void
uabad_news()
{
	errmsg (M_UID_AGING_MISSING);

	(void) ulckpwdf();
	exit(7);
}


/*
 * Procedure:	add_uid
 *
 * Notes:	adds a uid to the link list of used uids.
 *
 * 		A linked list of uid_blk is used to keep track of all the
 *		used uids.  Each uid_blk represents a range of used uid,
 *		where low represents the low inclusive end and high represents
 *		the high inclusive end.  In main(), a linked list of one uid_blk
 *		was initialized with low = high = (UID_MIN - 1).
 *
 *		When a used uid is read, it is added onto the linked list by
 *		either making a new uid_blk, decrementing the low of an existing 
 *		uid_blk, incrementing the high of an existing uid_blk, or combin- 
 *		ing two existing uid_blks.  After  the list is built, the first
 *		available uid above or equal to UID_MIN is the high of the first
 *		uid_blk in the linked list + 1.
*/
int
add_uid(uid, uid_sp)
	uid_t	uid;
	struct	uid_blk	*uid_sp;
{
	struct uid_blk *uid_p;

	/*
	 * Only keep track of the ones above UID_MIN
	*/
	if (uid >= UID_MIN) {
		uid_p = uid_sp;
		while (uid_p != NULL) {
			if (uid_p->link != NULL) {
				if (uid >= uid_p->link->low)
					uid_p = uid_p->link;

				else if (uid >= uid_p->low && 
						uid <= uid_p->high) {
						uid_p = NULL;
				}
				else if (uid == (uid_p->high+1)) {
					if (++uid_p->high == (uid_p->link->low - 1)) {
						uid_bcom(uid_p);
					}
					uid_p = NULL;
				}
				else if (uid == (uid_p->link->low - 1)) {
					uid_p->link->low--;
					uid_p = NULL;
				}
				else if (uid < uid_p->link->low) {
					if (add_ublk(uid, uid_p) == -1)
						return -1;
					uid_p = NULL;
				}
			}	/* if uid_p->link */
			else {
				if (uid == (uid_p->high + 1)) {
					uid_p->high++;
					uid_p = NULL;
				}
				else if (uid >= uid_p->low && 
					uid <= uid_p->high) {
					uid_p = NULL;
				}
				else {
					if (add_ublk(uid, uid_p) == -1)
						return -1;
					uid_p = NULL;
				}
			}	/* else */
		}	/* while uid_p */

	}	/* if uid */
	return 0;
}


/*
 * Procedure:	uid_bcom
 *
 * Notes:	combine two uid_blk's
*/
static	void
uid_bcom(uid_p)
	struct uid_blk *uid_p;
{
	struct uid_blk *uid_tp;

	uid_tp = uid_p->link;
	uid_p->high = uid_tp->high;
	uid_p->link = uid_tp->link;

	free(uid_tp);
}


/*
 * Procedure:	add_ublk
 *
 * Notes:	add a new uid_blk
*/
static	int
add_ublk(num, uid_p)
	uid_t num;
	struct uid_blk *uid_p;
{
	struct uid_blk *uid_tp;

	if ((uid_tp = (struct uid_blk *) malloc((size_t) 
		       sizeof(struct uid_blk))) == NULL) {
		return -1;	
	}

	uid_tp->high = uid_tp->low = num;
	uid_tp->link = uid_p->link;
	uid_p->link = uid_tp;
	return 0;
}
