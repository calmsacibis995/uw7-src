#ident  "@(#)gid.c	1.3"
#ident  "$Header$"

#include	<sys/types.h>
#include	<stdio.h>
#include	<errno.h>
#include	<grp.h>
#include	<userdefs.h>
#include	"gid.h"

#include	<sys/param.h>
#ifndef	MAXUID
#include	<limits.h>
#ifdef UID_MAX
#define	MAXUID	UID_MAX
#else
#define	MAXUID	60000
#endif
#endif

static void gid_bcom();
static int add_gid();
static int add_gblk();
/*
 * Procedure:     findnextgid
 *
 * Restrictions:
 *               getgrent: none
 *               endgrent: none
 */

gid_t
findnextgid()
{


	register int	end_of_file = 0;

	struct group *grpbuf;

	extern	int	errno;

	struct	gid_blk	*gid_sp;

	errno = 0;
	/*
	 * create the head of the gid number list
	*/
	if ((gid_sp = (struct gid_blk *) malloc((size_t) sizeof(struct gid_blk))) == NULL) {
		exit(EX_FAILURE);
	}
	gid_sp->link = NULL;
	gid_sp->low = (DEFGID);
	gid_sp->high = (DEFGID);

	while (!end_of_file) {
		if ((grpbuf = getgrent()) != NULL) {
			if (add_gid(grpbuf->gr_gid, &gid_sp) == -1) {
				endgrent();
				return -1;
			}
		}
		else {
			if (errno == 0)
				end_of_file = 1;
			else {
				if (errno == EINVAL)
					errno = 0;

				else end_of_file = 1;
			}
		}
	}
	endgrent();
	return (gid_sp->high + 1);


}


/*
 * Procedure:	add_gid
 *
 * Notes:	adds a gid to the link list of used gids.
 *
 * 		A linked list of gid_blk is used to keep track of all the
 *		used gids.  Each gid_blk represents a range of used gid,
 *		where low represents the low inclusive end and high represents
 *		the high inclusive end.  In main(), a linked list of one gid_blk
 *		was initialized with low = high = (DEFGID).
 *
 *		When a used gid is read, it is added onto the linked list by
 *		either making a new gid_blk, decrementing the low of an existing 
 *		gid_blk, incrementing the high of an existing gid_blk, or combin- 
 *		ing two existing gid_blks.  After  the list is built, the first
 *		available gid above or equal to DEFGID is the high of the first
 *		gid_blk in the linked list + 1.
*/
static int
add_gid(gid, gid_sp)
	gid_t	gid;
	struct	gid_blk	*gid_sp;
{
	struct gid_blk *gid_p;

	/*
	 * Only keep track of the ones above DEFGID
	*/
	if (gid > DEFGID) {
		gid_p = gid_sp;
		while (gid_p != NULL) {
			if (gid_p->link != NULL) {
				if (gid >= gid_p->link->low)
					gid_p = gid_p->link;

				else if (gid >= gid_p->low && 
						gid <= gid_p->high) {
						gid_p = NULL;
				}
				else if (gid == (gid_p->high+1)) {
					if (++gid_p->high == (gid_p->link->low - 1)) {
						gid_bcom(gid_p);
					}
					gid_p = NULL;
				}
				else if (gid == (gid_p->link->low - 1)) {
					gid_p->link->low--;
					gid_p = NULL;
				}
				else if (gid < gid_p->link->low) {
					if (add_gblk(gid, gid_p) == -1)
						return -1;
					gid_p = NULL;
				}
			}	/* if gid_p->link */
			else {
				if (gid == (gid_p->high + 1)) {
					gid_p->high++;
					gid_p = NULL;
				}
				else if (gid >= gid_p->low && 
					gid <= gid_p->high) {
					gid_p = NULL;
				}
				else {
					if (add_gblk(gid, gid_p) == -1)
						return -1;
					gid_p = NULL;
				}
			}	/* else */
		}	/* while gid_p */

	}	/* if gid */
	return 0;
}


/*
 * Procedure:	gid_bcom
 *
 * Notes:	combine two gid_blk's
*/
static	void
gid_bcom(gid_p)
	struct gid_blk *gid_p;
{
	struct gid_blk *gid_tp;

	gid_tp = gid_p->link;
	gid_p->high = gid_tp->high;
	gid_p->link = gid_tp->link;

	free(gid_tp);
}


/*
 * Procedure:	add_gblk
 *
 * Notes:	add a new gid_blk
*/
static	int
add_gblk(num, gid_p)
	gid_t num;
	struct gid_blk *gid_p;
{
	struct gid_blk *gid_tp;

	if ((gid_tp = (struct gid_blk *) malloc((size_t) 
		       sizeof(struct gid_blk))) == NULL) {
		return -1;	
	}

	gid_tp->high = gid_tp->low = num;
	gid_tp->link = gid_p->link;
	gid_p->link = gid_tp;

	return 0;
}
