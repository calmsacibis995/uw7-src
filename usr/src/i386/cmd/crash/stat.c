#ident	"@(#)crash:i386/cmd/crash/stat.c	1.1.1.1"

/*
 * This file contains code for the crash function stat.
 */

#include <sys/param.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/immu.h>
#include "crash.h"

static vaddr_t Sys, Time, Lbolt;

/* get arguments for stat function */
int
getstat()
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
	else prstat(); 
}


/* print system information */
int
prstat()
{
	int toc, lbolt;
	struct utsname utsbuf;

	/* 
	 * Locate, read, and print the system name, node name, release number,
	 * version number, and machine name.
	 */

	if(!Sys)
		Sys = symfindval("utsname");
	readmem(Sys,1,-1,&utsbuf,sizeof utsbuf,"utsname");

	fprintf(fp,"system name:\t%s\nrelease:\t%s\n",
		utsbuf.sysname,
		utsbuf.release);
	fprintf(fp,"node name:\t%s\nversion:\t%s\n",
		utsbuf.nodename,
		utsbuf.version);
	fprintf(fp,"machine name:\t%s\n", utsbuf.machine) ;
	/*
	 * Locate, read, and print the time of the crash.
	 */

	if(!Time)
		Time = symfindval("time");
	readmem(Time,1,-1,&toc,sizeof toc,"time");
	fprintf(fp,"time of crash:\t%s\n", date_time(toc));

	/*
	 * Locate, read, and print the age of the system since the last boot.
	 */

	if(!Lbolt)
		Lbolt = symfindval("lbolt");
	readmem(Lbolt,1,-1,&lbolt,sizeof lbolt,"lbolt");

	fprintf(fp,"age of system:\t");
	lbolt = lbolt/(60*HZ);
	if(lbolt / (long)(60 * 24))
		raw_fprintf(fp,"%d day, ", lbolt / (long)(60 * 24));
	lbolt %= (long)(60 * 24);
	if(lbolt / (long)60)
		raw_fprintf(fp,"%d hour, ", lbolt / (long)60);
	lbolt %= (long) 60;
	if(lbolt)
		raw_fprintf(fp,"%d min.", lbolt);
	fprintf(fp,"\n");
}
