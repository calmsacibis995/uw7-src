#ident	"@(#)crash:common/cmd/crash/callout.c	1.1.1.1"

/*
 * This file contains code for the crash function callout.
 */

#include "stdio.h"
#include "sys/types.h"
#include "sys/callo.h"
#include "crash.h"


static vaddr_t Callout;			/* from namelist */

/* get arguments for callout function */
int
getcallout()
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
	fprintf(fp,"FUNCTION        ARGUMENT   TIME  ID\n");
	if(args[optind]) 
		longjmp(syn,0);
	else prcallout();
}

/* print callout table */
int
prcallout()
{
	struct syment *sp;
	extern struct syment *findsym();
	struct callo callbuf;
	static char tname[SYMNMLEN+1];
	unsigned long addr;

	if(!Callout)
		Callout = symfindval("callout");
	addr = Callout;

	for(;;) {
		readmem(addr,1,-1,(char *)&callbuf,sizeof(callbuf),
				"callout table");
		if(!callbuf.c_func)	
			return;
		if(!(sp = findsym((unsigned long)callbuf.c_func)))
			error("%08x does not match in symbol table\n",
				callbuf.c_func);
		fprintf(fp,"%-15s",(char *) sp->n_offset);
		fprintf(fp," %08lx  %5u  %5u\n", 
			callbuf.c_arg,
			callbuf.c_time,
			callbuf.c_id);
		addr += sizeof(callbuf);
	}
}
