#ident	"@(#)crash:common/cmd/crash/lidcache.c	1.2.1.1"

#include <sys/param.h>
#include <sys/sysmacros.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/var.h>
#include "crash.h"
#include <sys/time.h>
#include <mac.h>

static vaddr_t Lidvp, Lvls_hdr_blk, Mac_cachel;	/* from namelist */

char   *heading1 = "             LAST REFERENCE TIME FL\n";
char   *heading2 = "ROW       LID     (SEC)   (NSEC) AG CLASS CATEGORIES\n\n";

/*
 * Get and print the lidvp and lid cache
 */

int
getlidcache()
{
	int	c;
	int	row;			/* for looping through lid cache  */
	int	clength;		/* the lid cache length		  */
	struct	vnode	     *lidvp;   	/* lid.internal vnode pointer	  */
	struct	mac_cachent  *searchp;	/* initial search pointer	  */
	struct	mac_cachent  *cachentp;	/* subsequent searches		  */
	struct	mac_cachent  cachent;	/* subsequent searche structure   */
	struct	lvls_hdr_blk lvls_hdr_blk;	/* initial search entry   */
	struct	lvls_hdr_blk *lvls_hdr_blkp;	/* subsequent search hash */

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) != EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (args[optind])
		longjmp(syn,0);

	/* get the MAC LID translation file vnode pointer */
	if (!Lidvp)
		Lidvp = symfindval("mac_lidvp");
	readmem(Lidvp, 1, -1, &lidvp, sizeof(struct vnode *), "mac_lidvp");

	/* get the length of the lid cache */
	if (!Mac_cachel)
		Mac_cachel = symfindval("mac_cachel");
	readmem(Mac_cachel, 1, -1, &clength, sizeof clength, "mac_cachel");

	/* get the initial search point of lid cache */
	if (!Lvls_hdr_blk)
		Lvls_hdr_blk = symfindval("lvls_hdr_blk");
	readmem(Lvls_hdr_blk, 1, -1, &lvls_hdr_blk,
		sizeof(struct lvls_hdr_blk), "lvls_hdr_blk");
	lvls_hdr_blkp = readmem((vaddr_t)lvls_hdr_blk.lvls_start, 1, -1, NULL,
		sizeof(struct lvls_hdr_blk) * clength, "lvls_start");

	fprintf(fp, "LID.INTERNAL VNODE POINTER = 0x%x\n", lidvp);
	fprintf(fp, "LID CACHE LENGTH = %d\n", clength);
	fprintf(fp, heading1);
	fprintf(fp, heading2);

	for (row = 0; row < clength; row++) {
		if ((searchp = lvls_hdr_blkp[row].lvls_start) == NULL)
			 continue;
		readmem((vaddr_t)lvls_hdr_blkp[row].lvls_start, 1,-1, (char *)&cachent,
			sizeof(struct mac_cachent),"initial cache row entry");
		searchp = cachent.ca_next;
		cachentp = cachent.ca_next;
		do {
			readmem((vaddr_t)cachentp, 1, -1, (char *)&cachent,
				sizeof(struct mac_cachent), "cache entry");
			prt_cachent(row, &cachent);
			cachentp = cachent.ca_next;
		} while (cachentp != searchp);
	}
}

/*
 * Print out a single lid cache entry.
 */

prt_cachent(row, entryp)
int row;
struct mac_cachent *entryp;
{
	ushort	*catsigp;	/* ptr to category significance array entry */
	ulong	*catp, i;	/* ptr to category array entry */
	int	firstcat = 1;

	if (entryp->ca_lid == 0)
		return 0;
	fprintf(fp,"[%2d] %8d %9d %9d %c  %3d  ", row, entryp->ca_lid,
		entryp->ca_lastref.tv_sec, entryp->ca_lastref.tv_nsec,
		entryp->ca_level.lvl_valid, entryp->ca_level.lvl_class);

	/*
	 * Print out the category numbers in effect.
	 */
	for (catsigp = &entryp->ca_level.lvl_catsig[0],
		catp = &entryp->ca_level.lvl_cat[0];
	     *catsigp != 0; catsigp++, catp++) {
		for (i = 0; i < NB_LONG; i++) {
			if (*catp & (((ulong)1 << (NB_LONG - 1)) >> i)) {
				if (!firstcat)
					fprintf(fp,",");
				/* print the category # */
				fprintf(fp,"%d",
					((ulong) (*catsigp-1) << CAT_SHIFT)
					+ i + 1);
				firstcat = 0;
			}
		}
	}
	fprintf(fp,"\n");
}
