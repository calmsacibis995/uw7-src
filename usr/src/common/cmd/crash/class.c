#ident	"@(#)crash:common/cmd/crash/class.c	1.1.1.1"

/*
 * This file contains code for the crash functions:  class, claddr
 */

#include <sys/param.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/priocntl.h>
#include <sys/tspriocntl.h>
#include <sys/fppriocntl.h>
#include <sys/class.h>
#include "crash.h"


static vaddr_t Cls, Ncls;	/* from namelist */
static class_t *classaddr;

/* get arguments for class function */
int
getclass()
{
	int slot;
	int c;
	long arg1;
	long arg2;

	char *classhdg = "SLOT\tCLASS\tINIT FUNCTION\tCLASS FUNCTION\n";
	int nclass;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	/* Determine how many entries are in the class table */

	if(!Ncls)
		Ncls = symfindval("nclass");
	readmem(Ncls, 1, -1, &nclass, sizeof(int), "nclass");

	/* Read in the entire class table */

	if(!Cls)
		Cls = symfindval("class");
	classaddr = readmem((vaddr_t)Cls, 1, -1, NULL,
		nclass*sizeof(class_t), "class table");

	fprintf(fp, "%s", classhdg);

	if(args[optind]){
		do {
			if (!getargs(nclass,&arg1,&arg2))
				error("%d is out of range 0-%d\n",
					arg1, nclass - 1);
			for(slot = arg1; slot <= arg2; slot++)
				prclass(slot);
		}while(args[++optind]);
	}
	else for(slot = 0; slot < nclass; slot++)
		prclass(slot);
}

/* print class table  */
int
prclass(slot)
int slot;
{
	char name[PC_CLNMSZ];

	readmem((long)classaddr[slot].cl_name, 1, -1, name,
		sizeof name, "class name");

	fprintf(fp, "%d\t%s\t%x\t%x\n", slot, name, classaddr[slot].cl_init,
		classaddr[slot].cl_funcs);
}
