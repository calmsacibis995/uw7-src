#ident	"@(#)crash:common/cmd/crash/rt.c	1.1.1.1"

/*
 * This file contains code for the crash functions:  rtproc, rtdptbl
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/rt.h>
#include "crash.h"


static vaddr_t Rtproc, Rtdptbl, Rtmaxpri;	/* from namelist */
rtdpent_t *rtdptbl;
struct rtproc rtbuf;
struct rtproc *rtp;

int
getrtproc()
{
	int c;
	char *rtprochdg = "PQUANT  TMLFT  PRI  FLAGS   PROCP   PSTATP    PPRIP   PFLAGP    NEXT     PREV\n";

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
	if(!Rtproc)
		Rtproc = symfindval("rt_plisthead");
	readmem(Rtproc, 1, -1, &rtbuf, sizeof rtbuf, "rt_plisthead");

	fprintf(fp, "%s", rtprochdg);
	rtp = rtbuf.rt_next;

	for(; rtp != (rtproc_t *)Rtproc; rtp = rtbuf.rt_next) {
		readmem((long)rtp, 1, -1, (char *)&rtbuf,
			sizeof rtbuf, "rtproc table");
		prrtproc();
	}
}




/* print the real time process table */
int
prrtproc()
{


	fprintf(fp, "%4d    %4d %4d %5x %10x %8x %8x %8x %8x %8x\n",  rtbuf.rt_pquantum,
		rtbuf.rt_timeleft, rtbuf.rt_pri, rtbuf.rt_flags,
		rtbuf.rt_procp, rtbuf.rt_pstatp, rtbuf.rt_pprip,
		rtbuf.rt_pflagp, rtbuf.rt_next, rtbuf.rt_prev);
}


/* get arguments for rtdptbl function */

int
getrtdptbl()
{
	int slot;
	long arg1;
	long arg2;
	int c;
	short rtmaxpri;

	char *rtdptblhdg = "SLOT     GLOBAL PRIORITY     TIME QUANTUM\n\n";

	optind=1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	if(!Rtmaxpri)
		Rtmaxpri = symfindval("rt_maxpri");
	readmem(Rtmaxpri,1,-1,&rtmaxpri,sizeof(short),"rt_maxpri");

	if(!Rtdptbl)
		Rtdptbl = symfindval("rt_dptbl");
	rtdptbl = readmem(Rtdptbl, 1, -1, NULL,
		(rtmaxpri + 1) * sizeof(rtdpent_t), "rt_dptbl");

	fprintf(fp,"%s",rtdptblhdg);

	if(args[optind]) {
		do {
			if (!getargs(rtmaxpri+1,&arg1,&arg2))
				error("priority %d is out of range 0-%d\n",
					arg1, rtmaxpri);
			for(slot = arg1; slot <= arg2; slot++)
				prrtdptbl(slot);
		}while(args[++optind]);
	}
	else 
		for(slot = 0; slot <= rtmaxpri; slot++)
			prrtdptbl(slot);
}

/* print the real time dispatcher parameter table */
int
prrtdptbl(slot)
int  slot;
{
	fprintf(fp, "%3d           %4d           %10ld\n",
	    slot, rtdptbl[slot].rt_globpri, rtdptbl[slot].rt_quantum);	
}
