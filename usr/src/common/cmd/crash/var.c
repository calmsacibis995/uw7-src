#ident	"@(#)crash:common/cmd/crash/var.c	1.1.1.1"

/*
 * This file contains code for the crash function var.
 */

#include <sys/param.h>
#include <stdio.h>
#include <signal.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/vfs.h>
#include <sys/utsname.h>
#include "crash.h"

/* get arguments for var function */
int
getvar()
{
	int c;

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
	else prvar();
}

/* print var structure */
int
prvar()
{
	extern vaddr_t Var;
	int v_proc;

	v_proc = vbuf.v_proc;
	readmem(Var,1,-1,&vbuf,sizeof vbuf,"var structure");

	/*
	 * vbuf.v_proc should be fixed throughout the life of the system;
	 * but if it somehow got changed, it would confuse all of util.c's
	 * slottab handling, so check and remake if necessary.
	 */
	if (vbuf.v_proc != v_proc)
		makeslottab();

	fprintf(fp,"v_buf:       %10u    ", vbuf.v_buf);
	fprintf(fp,"v_call:      %10u    ", vbuf.v_call);
	fprintf(fp,"v_proc:      %10u\n", vbuf.v_proc);
	fprintf(fp,"v_maxsyspri: %10u    ", vbuf.v_maxsyspri);
	fprintf(fp,"v_maxup:     %10u    ", vbuf.v_maxup);
	fprintf(fp,"v_hbuf:      %10u\n", vbuf.v_hbuf);
	fprintf(fp,"v_hmask:     %10u    ", vbuf.v_hmask);
	fprintf(fp,"v_pbuf:      %10u    ", vbuf.v_pbuf);
	fprintf(fp,"v_autoup:    %10u    ", vbuf.v_autoup);
	fprintf(fp,"v_bufhwm:    %10u    ", vbuf.v_bufhwm);
	fprintf(fp,"v_scrn:      %10u\n", vbuf.v_scrn);
	fprintf(fp,"v_emap:      %10u    ", vbuf.v_emap);
	fprintf(fp,"v_sxt:       %10u    ", vbuf.v_sxt);
	fprintf(fp,"v_xsdsegs:   %10u\n", vbuf.v_xsdsegs);
	fprintf(fp,"v_xsdslots:  %10u    ", vbuf.v_xsdslots);
	fprintf(fp,"v_maxulwp:   %10u    ", vbuf.v_maxulwp);
	fprintf(fp,"v_nonexclusive: %7u\n", vbuf.v_nonexclusive);
	fprintf(fp,"v_max_proc_exbind: %4u    ", vbuf.v_max_proc_exbind);
	fprintf(fp,"v_static_sq: %10u\n", vbuf.v_static_sq);
}
