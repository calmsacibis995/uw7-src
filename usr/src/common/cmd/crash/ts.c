#ident	"@(#)crash:common/cmd/crash/ts.c	1.2.1.1"

/*
 * This file contains code for the crash functions:  tslwp, tsdptbl
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ts.h>
#include "crash.h"

static vaddr_t Tshashp, Tsdptbl, Tsmaxumdpri;
tsdpent_t *tsdptbl;
struct tshash *tshp, *thashp, *tshphead;
struct tshash *tshashpbuf;
struct tslwp *tslwpp;
struct tslwp tslwpbuf;

/* get arguments for tslwp function */
int
gettslwp()
{
	int i,c;

	char *tshashhdg = " TMLFT CPUPRI UPRILIM UPRI UMDPRI NICE FLAGS DISPWAIT   LWPP    LSTATP		LPRIP		LFLAGP		SLPWAIT	NEXT     	PREV\n";

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	if(args[optind])
		longjmp(syn,0);

	if(!Tshashp)
		Tshashp = symfindval("tshashp");

	/* Get the address of the head of the hash table for TS class */
	readmem(Tshashp, 1, -1, &thashp, sizeof(tshash_t *), "tshashp");

	/* Read in the heads of all the hash buckets */
	tshashpbuf = readmem((vaddr_t)thashp, 1, -1, NULL,
		TSHASHSZ * sizeof(struct tshash), "tshashp table");

	fprintf(fp, "%s", tshashhdg);

	/* Loop through all the hash buckets */
	for (i=0; i < TSHASHSZ; i++) {
		/* Loop through all entries on the hash chain */
		tshp = &tshashpbuf[i];
		tshphead = &thashp[i]; /* Remember where we started. */
		tslwpp = (tslwp_t *)tshp->th_first;
		while (tslwpp != (tslwp_t *)tshphead) {
			readmem((long)tslwpp, 1, -1, (char *)&tslwpbuf,
				sizeof tslwpbuf, "tslwp_t struc");
			prtslwp();
			tslwpp = (tslwp_t *)tslwpbuf.ts_flink;
		}
	}
}

/* print the time sharing lwp table */
int
prtslwp()
{


	fprintf(fp, "  %4d    %2d      %2d    %2d    %2d    %2d   %#x    %2d   %.8x	%.8x 	%.8x 	%.8x 	%4d 	%.8x 	%.8x\n",
	tslwpbuf.ts_timeleft, tslwpbuf.ts_cpupri, tslwpbuf.ts_uprilim,
		tslwpbuf.ts_upri, tslwpbuf.ts_umdpri, tslwpbuf.ts_nice,
		tslwpbuf.ts_flags, tslwpbuf.ts_dispwait, tslwpbuf.ts_lwpp,
		tslwpbuf.ts_lstatp, tslwpbuf.ts_lprip, tslwpbuf.ts_lflagp,
		tslwpbuf.ts_sleepwait,
		tslwpbuf.ts_flink, tslwpbuf.ts_rlink);
}


/* get arguments for tsdptbl function */

int
gettsdptbl()
{
	int slot;
	long arg1;
	long arg2;
	int c;
	short tsmaxumdpri;

	char *tsdptblhdg = "SLOT     GLOBAL PRIO     TIME QUANTUM     TQEXP     SLPRET     MAXWAIT    LWAIT\n";

	optind=1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	if(!Tsmaxumdpri)
		Tsmaxumdpri = symfindval("ts_maxumdpri");
	readmem(Tsmaxumdpri,1,-1,&tsmaxumdpri,sizeof(short),"ts_maxumdpri");

	if(!Tsdptbl)
		Tsdptbl = symfindval("ts_dptbl");
	tsdptbl = readmem(Tsdptbl, 1, -1, NULL,
		(tsmaxumdpri + 1) * sizeof(tsdpent_t), "ts_dptbl");

	fprintf(fp,"%s",tsdptblhdg);

	if(args[optind]) {
		do {
			if (!getargs(tsmaxumdpri+1,&arg1,&arg2))
				error("priority %d is out of range 0-%d\n",
					arg1, tsmaxumdpri);
			for(slot = arg1; slot <= arg2; slot++)
				prtsdptbl(slot);
		}while(args[++optind]);
	}
	else 
		for(slot = 0; slot <= tsmaxumdpri; slot++)
			prtsdptbl(slot);
}

/* print the time sharing dispatcher parameter table */
int
prtsdptbl(slot)
int  slot;
{
	fprintf(fp,"%3d         %4d         %10ld        %3d       %3d        %5d       %3d\n",
	    slot, tsdptbl[slot].ts_globpri, tsdptbl[slot].ts_quantum,
	    tsdptbl[slot].ts_tqexp, tsdptbl[slot].ts_slpret,
	    tsdptbl[slot].ts_maxwait, tsdptbl[slot].ts_lwait);
}
