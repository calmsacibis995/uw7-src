/*		copyright	"%c%" 	*/

#ident	"@(#)adt_cred.c	1.3"
#ident  "$Header$"

/*
 * This file contains routines to allocate and extract credential data blocks.
 * A list of credential data blocks (crlist structures) is maintained in
 * decreasing order of sequence number (i.e. head points to the credential
 * with the highest sequence number), since it is most likely that a higher
 * sequence number will be searched.
 */

/* LINTLIBRARY */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/vnode.h>
#include <sys/param.h>
#include <sys/privilege.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <mac.h>
#include <pfmt.h>
#include <sys/fcntl.h>
#include <sys/resource.h>
#include <audit.h>
#include <sys/auditrec.h>
#include "auditrptv4.h"

#define      SIZ_GID         sizeof(gid_t)
#define	CRED_HASH_SIZ	100	/* size of credentials hash table */

/*
 * type definition of the credential block entry kept for each credential
 * in the list
 */
struct	crlist {
	struct	crlist	*next;
	unsigned long crseqnum;
	struct cred_rec cred;
	gid_t	*groups;
};

static	struct	crlist	*cred_hash_tbl[CRED_HASH_SIZ];

static	struct	crlist	*listent(),
			*makecred(),
			*findcred();


#define	SIZ_GID		sizeof(gid_t)
#define	SIZ_CRLIST	sizeof(struct crlist)

/*
 * Return the head of the correct list to use, from the appropriate
 * slot in the hash table.
 */
#define GETHASH(C) &(cred_hash_tbl[C % CRED_HASH_SIZ])

/*
 * Create entry in the credential list, for the credential indicated by
 * cred->cr_crseqnum
 */
void
setcred(dumpp)
dumprec_t *dumpp;
{
	struct	crlist	*p;

	p = makecred(dumpp->cred.cr_crseqnum);
	(void)memcpy(&p->cred, &dumpp->cred, SIZ_CREDREC);
	if (dumpp->cred.cr_ngroups > 0){
		if ((p->groups = (gid_t *)malloc(SIZ_GID*dumpp->cred.cr_ngroups)) == NULL){
			(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
			adt_exit(ADT_MALLOC);
		}
		(void)memcpy(p->groups, ((char *)&dumpp->cred + SIZ_CREDREC), (dumpp->cred.cr_ngroups) * SIZ_GID);
	}
}

/*
 * Get the credential information for the event indicated by
 * dump_rec->cmn.c_crseqnum and fill dump_rec appropriately.
 *  Return -1 if the credential information is not found.
 */

int
getcred(dump_rec)
struct	rec	*dump_rec;
{
	struct	crlist	*p;

	p = findcred(dump_rec->cmn.c_crseqnum);
	if (!p){		/* if cred entry not found */
		return(-1);
	}
	(void)memcpy(&dump_rec->cred, &p->cred, SIZ_CREDREC);
	if (p->cred.cr_ngroups > 0)
		(void)memcpy(dump_rec->groups, p->groups, (p->cred.cr_ngroups) * SIZ_GID);
	return(0);
}

/*
 * Look up a particular credential block (based on cr_crseqnum) and return a
 * pointer to the data.  If the credential is not found, return a NULL
 * pointer.
 */
static
struct crlist *
findcred(crseqnum)
unsigned long	crseqnum;
{
	struct	crlist	*p, **xp;

	
	xp = GETHASH(crseqnum);
	for (p = *xp; (p != NULL) && (p->crseqnum > crseqnum) ; p = p->next){
			;
	}
	if ((p == NULL) || (p->crseqnum != crseqnum))
		return((struct crlist *)NULL);	/* entry not in list */
	else
		return(p);			/* found entry */
}

/*
 * Find the requested credential data block in the list of credential data
 * blocks.  If the block is not found, allocate a new one, insert it in
 * the list (in decreasing order of crseqnum) and initialize the data.
 */
static 
struct crlist *
makecred(crseqnum)
unsigned long   crseqnum;
{
	struct crlist	*lp, *plp, **xlp;

	/*
	 * Check the cred list for a cred entry with this crseqnum.
	 * If a match is found, ignore this cred record since the
	 * kernel may generate a duplicate cred record.
	 */
	lp = findcred(crseqnum);
	if (lp){
		return(lp);
	}
	xlp = GETHASH(crseqnum);	 	/* set to start of list */
	lp = plp = *xlp;
	/* if list is empty or this entry should be at head of list */
	if (!lp || (lp->crseqnum < crseqnum)){
		lp = listent(crseqnum, lp);
		*xlp = lp;
		return(lp);
	}
	/* determine where to insert entry */
	while (lp && lp->crseqnum > crseqnum){
		plp = lp;
		lp = plp->next;
	}
	plp->next = listent(crseqnum, lp);	/* create new entry */
	return(plp->next);
}


/*
 * Allocate and initialize a list entry and append the list indicated by
 * 'next' to it.
 */
static
struct crlist	*
listent(crseqnum, next)
unsigned long   crseqnum;
struct crlist	*next;
{
	struct	crlist	*p;

	if ((p = (struct crlist *)malloc(SIZ_CRLIST)) == NULL){
		(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
		adt_exit(ADT_MALLOC);
	}
	p->next = next;
	p->crseqnum = crseqnum;
	p->cred.cr_lid = 0;
	p->cred.cr_uid = -1;
	p->cred.cr_gid = -1;
	p->cred.cr_ruid = -1;
	p->cred.cr_rgid = -1;
	p->cred.cr_maxpriv = 0;
	p->cred.cr_ngroups = -1;
	p->groups = NULL;
	return(p);
}

/*
 * Find the entry with the specified crseqnum, remove it from the list,
 * and free it's memory.
 */
void
freecred(crseqnum)
unsigned long   crseqnum;
{
	struct crlist	*lp, *plp, **xlp;
	
	xlp = GETHASH(crseqnum);
	plp = lp = *xlp;
	/* find entry in list */
	while (lp && lp->crseqnum > crseqnum){
		plp = lp;
		lp = plp->next;
	}
	if ((lp == NULL) || (lp->crseqnum != crseqnum)){	/*entry not found */
		(void)pfmt(stdout, MM_WARNING, W_CRED_NO_FREE);
		return;

	}
	if (lp == *xlp){		/* entry is at head of list */
		*xlp = lp->next;	/* eliminate reference to cred entry */
	} else {
		plp->next = lp->next;	/* eliminate reference to cred entry */
	}
	free(lp->groups);		/* free space used to store groups */
	free(lp);
}
