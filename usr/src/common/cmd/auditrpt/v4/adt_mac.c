/*		copyright	"%c%" 	*/

#ident	"@(#)adt_mac.c	1.2"
#ident  "$Header$"

/*
 * This file is auditrpt's version of MAC's lvldom(2) routine
 * (in uts-comm:mac/genmac.c).  It handles the types in auditrpt's
 * local LTDB the same way that lvlin(3C) handles the types in the
 * master LTDB in /etc/security/mac.  Any alterations done to the 
 * master LTDB that cause lvldom(3C) to change, should also be 
 * reflected in this code.
 */

/* LINTLIBRARY */
#include <sys/types.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <fcntl.h>
#include <sys/systm.h>
#include <mac.h>
#include <audit.h>

extern char	*lid_fp;	/* points to the map's lid.internal */

static	int	adt_getlevel();
static	int	adtmac_lvldom();

/**
 ** This routine returns a 1 if th level lid1 dominates lid2.  It returns a 0
 ** if lid1 does not dominate, and -1 if one of the lids is not valid.
 **/ 
int
adt_lvldom(lid1,lid2)
lid_t lid1;
lid_t lid2;
{
	struct mac_level *level1p, *level2p, ml1, ml2;
	level1p = &ml1;
	level2p = &ml2;

	/* validate level */
	if ((adt_getlevel(lid1,level1p))!= 1)
		return(-1);
	/* if they are equal, don't go further and return 1 */
	if (lid1==lid2)
		return(1);
	if ((adt_getlevel(lid2,level2p))!= 1)
		return(-1);
	if ( (adtmac_lvldom(level1p, level2p) == 0) )
		return(1);
	return(0);
}

/**
 ** This routine returns a 1 if lid is valid and fills the mac_level
 ** structure pointed by levelp for lid.  The validation is done against
 ** the auditmap lid.internal file.
 **/
int  
adt_getlevel(lid,levelp)
level_t lid;
struct  mac_level *levelp;
{
	int lidfd;

	if ((lidfd=open(lid_fp,O_RDONLY)) == -1)
		return(-1);
	if (lseek(lidfd,lid*LVL_STRUCT_SIZE,0)== -1)
		return(-1);
	if (read(lidfd,levelp,LVL_STRUCT_SIZE) == LVL_STRUCT_SIZE){
		(void)close(lidfd);
		return(1);
	} else {
		(void)close(lidfd);
		return(-1);
	}

}

/**
 **  This routine determines whether the level in the mac_level structure
 **  referenced by "level1p" dominates the level in the mac_level structure 
 **  referenced by "level2p". It returns 0 if it does, 1 otherwise.
 **/
int
adtmac_lvldom(level1p, level2p)
struct mac_level *level1p, *level2p;
{
	int error=0;
	ushort *catsig1p, *catsig2p;
	ulong *cat1p, *cat2p;
	int catsig1, catsig2;

	/*
	   If the second level's classification is greater than the first
	   we know the domination check fails.
	*/
	if (level2p->lvl_class > level1p->lvl_class) {
		error = 1;
		goto out;
	}

	/*
	   Set up the pointers to the category significance arrays, and the
	   category bit arrays for both levels.
	*/

	catsig1p = level1p->lvl_catsig;
	catsig2p = level2p->lvl_catsig;
	cat1p = level1p->lvl_cat;
	cat2p = level2p->lvl_cat;

	/*
	   The last entry of the category significance array  *always* contains
	   a null, so we have to make only one check (no check for exceeding
	   size of lvl_catsig[]).
	*/
	/*while there are more categories in each level*/
	while (*catsig1p != 0 && *catsig2p != 0) {
		/*lessen indirection*/
		catsig1 = *catsig1p;
		catsig2 = *catsig2p;
		/*if 1st level has a chunk of category bits the second
		  level doesn't, skip them*/
		if (catsig1 < catsig2) {
			catsig1p++;
			cat1p++;
		/*if the significance of the currently referenced category
		  bits is the same...*/
		} else if (catsig1 == catsig2) {
				/*see if level 1's bits are a superset of
				  level 2's bits*/
				if (!(*cat2p & ~*cat1p)) {
					/*they are. point to the next set of
					  bits and significance values.*/
					catsig1p++;
					catsig2p++;
					cat1p++;
					cat2p++;
				} else break;
		       } else break;
	}

	/*
	   When we get to this point, as long as we've exhausted the *second*
	   level's category bits, we have domination.
	*/

	if (*catsig2p == 0)
		error = 0;
	else	error = 1;
out:
	return error;
}
