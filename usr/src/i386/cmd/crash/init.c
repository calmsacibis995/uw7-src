#ident	"@(#)crash:i386/cmd/crash/init.c	1.1.3.2"

/*
 * This file contains code for the crash initialization.
 */

#include <sys/var.h>
#include "crash.h"

int mem;			/* file descriptor for dumpfile */
vaddr_t Var;			/* kernel address of var structure */
struct var vbuf;		/* var structure buffer */
extern jmp_buf jmp;		/* jump on error or signal */
extern size_t sizeof_as, sizeof_engine, sizeof_lwp, sizeof_plocal;

/* initialize buffers, symbols, and global variables for crash session */
void
cr_open(int rwflag, char *dumpfile, char *namelist, char *modpath)
{
	jmp_buf sav_jmp;
	int engid;
	
	if (fp == NULL)
		fp = stdout;

	/* open dump file, if error print */
	if((mem = open(dumpfile, rwflag)) < 0)
		fatal("cannot open dumpfile %s\n",dumpfile);

	/* set a flag if the dumpfile is of an active system */
	active = (strcmp(dumpfile,"/dev/mem") == 0);

	rdsymtab(namelist);		/* open and read the symbol table */

	if ((UniProc = (symsrch("xcall") == NULL)))
		sizeof_engine -= 12;

	/*
	 * vtopinit and ptofinit are order sensitive and now
	 * require different orders on and dump to an active system !!!
	 * This is since vtop cannot be done on a dump without
	 * a ptof mapping, and ptof on an active system uses the topology
	 * stucture which requires vtop to read it
	 */

	if(!active)
		ptofinit();		/* initialize paddr -> faddr */
	vtopinit();			/* initialize vaddr -> paddr */
	if(active)
		ptofinit();		/* initialize paddr -> faddr */

	/*
	 * need to re-read the symbol table 
	 * since kvbase may have changed in vtopinit
	 */
	
	fixupsymtab();

	Var = symfindval("v");
	/* This is the first call to readmem(virtual) */
	readmem(Var,1,-1,&vbuf,sizeof vbuf,"var structure");

	engid = enginit();	/* set Nengine engine KPD[1..] panic_data */

	/* if caller set jmp, we must restore it before returning */
	memcpy(sav_jmp, jmp, sizeof(jmp));

	if (!setjmp(jmp))	/* come back here if error while... */
		rdmodsyms(modpath); /* load symbols from loaded modules */

	if (!setjmp(jmp)) {	/* come back here if error while... */
		if (active)	/* start with program itself as Cur_proc */
			Cur_proc = pid_to_slot(getpid());
		else	/* set Sav_eng Cur_eng Cur_proc Cur_lwp from dump */
			get_context(engid);
	}

	memcpy(jmp, sav_jmp, sizeof(jmp));
}
