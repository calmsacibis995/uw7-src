#ident	"@(#)crash:common/cmd/crash/prnode.c	1.1.1.2"

/*
 * This file contains code for the crash function:  prnode.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/immu.h>
#include <sys/var.h>
#include <sys/proc.h>
#include <sys/procfs.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/vfs.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/cred.h>
#include <sys/uio.h>
#include <fs/procfs/prdata.h>
#include "crash.h"

static struct syment *Prrootnode;	/* namelist symbol pointers */

/* get arguments for prnode function */
int
getprnode()
{
	int slot;
	int full = 0;
	long arg1;
	long arg2;
	int c;
	char *heading = "SLOT   PROC     MODE   WRITER FLAGS\n";

	if(!Prrootnode)
		if(!(Prrootnode= symsrch("prrootnode")))
			error("prrootnode not found in symbol table\n");

	optind = 1;
	while((c = getopt(argcnt,args,"fw:")) !=EOF) {
		switch(c) {
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
					break;
		}
	}

	if(!full)
		fprintf(fp,"%s",heading);
	if(args[optind]) {
		do {
			if (getargs(PROCARGS,&arg1,&arg2))
			    for(slot = arg1; slot <= arg2; slot++)
				prprnode(full,slot,-1,heading);
			else
				prprnode(full,-1,arg1,heading);
		}while(args[++optind]);
	}
	else {
		if (active)
			makeslottab();
		for(slot = 0; slot < vbuf.v_proc; slot++)
			prprnode(full,slot,-1,heading);
	}
}

/* print prnode */
int
prprnode(full,slot,addr,heading)
int full,slot;
long addr;
char *heading;
{

	char ch;
	struct proc pbuf;
	struct vnode vnbuf;
	struct prnode prnbuf;
	long vp, pnp;
	proc_t *procaddr;
	proc_t *slot_to_proc();

	if(addr != -1)
	{
		readmem(addr,1,-1,(char *)&prnbuf,sizeof prnbuf,"prnode");
	}
	else
	{
		procaddr = slot_to_proc(slot);
		if (procaddr)
			readmem((long)procaddr,1,-1,(char *)&pbuf,
			  sizeof pbuf,"proc table");
		else
			return;
		if(pbuf.p_trace == 0)
			return;
		readmem((long)pbuf.p_trace,1,-1,(char *)&vnbuf,sizeof vnbuf,"vnode");
		readmem((long)vnbuf.v_data,1,-1,(char *)&prnbuf,sizeof prnbuf,"prnode");
	}

	if(full)
		fprintf(fp,"%s",heading);
	if(slot == -1)
		fprintf(fp,"  - ");
	else fprintf(fp,"%4d",slot);

	fprintf(fp,"  %8x    %s  %4d ",
		prnbuf.pr_proc,
		(prnbuf.pr_mode & 0600) ? "rw" : "  ",
		prnbuf.pr_writers);

	fprintf(fp,"%s%s\n",
		prnbuf.pr_flags & PREXCL ? " excl" : "",
		prnbuf.pr_flags & PRINVAL ? " inval" : "");
	if(!full)
		return;
	/* print vnode info */
	fprintf(fp,"\nVNODE :\n");
	fprintf(fp,vnheading);
	cprvnode(&prnbuf.pr_vnode);
	fprintf(fp,"\n");
}
