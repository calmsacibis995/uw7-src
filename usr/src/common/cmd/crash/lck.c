#ident	"@(#)crash:common/cmd/crash/lck.c	1.4.3.1"

/*
 * This file contains code for the crash function lck.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/var.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#define _KERNEL			/* header file forgets _KMEMUSER */
#include <sys/flock.h>
#undef _KERNEL
#include <sys/metrics.h>
#include "crash.h"

static vaddr_t Sleeplcks, S_mets;		/* from namelist */
static struct syment *S_s5hinode;		/* pointers */
static struct syment *S_sfs_ihead;		/* pointers */
static struct syment *S_vxfs_icache;
static struct mets mets;


/* get arguments for lck function */
int
getlcks()
{
	int	totalflcks;
	int	inusedflcks;
	long addr;
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	if (!S_mets)
		S_mets = symfindval("m");
	readmem(S_mets,1,-1,&mets, sizeof mets,"mets table m");

	fprintf(fp,"\nAdministrative Info:\n");
	fprintf(fp,"Currently_in_use  Total_used\n");
	fprintf(fp,"     %5d             %5d\n\n",
		mets.mets_files.msf_flck[MET_INUSE],
		mets.mets_files.msf_flck[MET_TOTAL]);

	if(active)
		makeslottab();

	if(args[optind]) {
		fprintf(fp,"TYP WHENCE      START        LEN\n\t PROC     EPID    SYSID WAIT     PREV     NEXT\n\n");
		do {
			addr = strcon(args[optind], 'h');
			prlcks(addr);
		}while(args[++optind]);
	} else
		prilcks();
}

static char *
startlen(off64_t start, off64_t end)
{
	static char buf[50];
	char *format = hexmode? " %10llx": " %10lld";
	int n = sprintf(buf, format, start);
	if (++end < start)	/* don't print obscure decimal number! */
		sprintf(buf+n, " %10s", "-");
	else
		sprintf(buf+n, format, end-start);
	return buf;
}

/* print lock information relative to all inodes (default) */
int
prilcks()
{
	struct	filock	*slptr,fibuf;
	long iptr;
	int active = 0;
	int free = 0;
	int sleep = 0;
	int next,prev;
	long addr;
	int i;

	fprintf(fp,"Active Locks:\n");
	fprintf(fp,"INO         TYP WHENCE      START        LEN\n\t PROC     EPID    SYSID WAIT     PREV     NEXT\n");

	if (S_s5hinode = symsrch("hinode"))
		active += s5lck();
	if (S_sfs_ihead = symsrch("sfs_ihead"))
		active += sfslck();
	if (S_vxfs_icache = symsrch("vx_icache"))
		active += vxfs_lck();

	fprintf(fp,"\nSleep  Locks:\n");
	fprintf(fp,"            TYP WHENCE      START        LEN\n\tLPROC     EPID    SYSID BPROC     EPID    SYSID     PREV     NEXT\n");

	if(!Sleeplcks)
		Sleeplcks = symfindval("sleeplcks");
	readmem(Sleeplcks,1,-1,&slptr,
		sizeof slptr,"sleeplcks table");

	while (slptr) {
		readmem((vaddr_t)slptr,1,-1,&fibuf,sizeof fibuf,
			"sleeplcks table slot");
		++sleep;
		fprintf(fp, "            ");
		if(fibuf.set.l_type == F_RDLCK) 
			fprintf(fp," r  ");
		else if(fibuf.set.l_type == F_WRLCK) 
			fprintf(fp," w  ");
		else fprintf(fp," ?  ");
		fprintf(fp,"%6d%s\n\t%5d %8d %8d %5d %8d %8d %8x %8x\n\n",
			fibuf.set.l_whence,
			startlen(fibuf.set.l_start,fibuf.set.l_end),
			pid_to_slot(fibuf.set.l_pid),
			fibuf.set.l_pid,
			fibuf.set.l_sysid,
			pid_to_slot(fibuf.stat.blk.pid),
			fibuf.stat.blk.pid,
			fibuf.stat.blk.sysid,
			fibuf.prev,
			fibuf.next);
		if (fibuf.next == fibuf.prev)
			break;
		slptr = fibuf.next;
	}

	fprintf(fp,"\nSummary From Actual Lists:\n");
	fprintf(fp," TOTAL    ACTIVE  SLEEP\n");
	fprintf(fp," %4d    %4d    %4d\n",
		active+sleep,
		active,
		sleep);
}    

int
prlcks(addr)
long addr;
{
	struct filock fibuf;

	readmem(addr,1,-1,(char *)&fibuf,sizeof fibuf,"frlock");
	fprintf(fp," %c%c%c",
	(fibuf.set.l_type == F_RDLCK) ? 'r' : ' ',
	(fibuf.set.l_type == F_WRLCK) ? 'w' : ' ',
	(fibuf.set.l_type == F_UNLCK) ? 'u' : ' ');
	fprintf(fp,"%6d%s\n\t%5d %8d %8d %4x %8x %8x\n",
		fibuf.set.l_whence,
		startlen(fibuf.set.l_start,fibuf.set.l_end),
		pid_to_slot(fibuf.set.l_pid),
		fibuf.set.l_pid,
		fibuf.set.l_sysid,
		fibuf.stat.wakeflg,
		fibuf.prev,
		fibuf.next);
}

int
print_lock(actptr, addr, itype)
struct filock *actptr;
long addr;
char *itype;
{
	struct	filock	fibuf, *flp, *nflp;
	int active = 0;
	int n;

	for (flp = actptr; flp != NULL; flp = nflp) {
		readmem((long)flp,1,-1,(char *)&fibuf,
			sizeof fibuf,"filock information");
		nflp = fibuf.next;
		++active;
		/* vxfs_lck() provides slot number in place of addr */
		n = fprintf(fp,(addr<0)?"%x(%s)":"%d(%s)",addr,itype);
		while (++n < 13)
			putc(' ',fp);
		if(fibuf.set.l_type == F_RDLCK) 
			fprintf(fp," r  ");
		else if(fibuf.set.l_type == F_WRLCK) 
			fprintf(fp," w  ");
		else fprintf(fp," ?  ");
		fprintf(fp,"%6d%s\n\t%5d %8d %8d %4x %8x %8x\n\n",
			fibuf.set.l_whence,
			startlen(fibuf.set.l_start,fibuf.set.l_end),
			pid_to_slot(fibuf.set.l_pid),
			fibuf.set.l_pid,
			fibuf.set.l_sysid,
			fibuf.stat.wakeflg,
			fibuf.prev,
			fibuf.next);
		if (fibuf.next == fibuf.prev)
			break;
	}
	return(active);
}
