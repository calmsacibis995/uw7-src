#ident	"@(#)crash:common/cmd/crash/map.c	1.1.1.1"

/*
 * This file contains code for the crash function map.
 */

#include "sys/param.h"
#include "stdio.h"
#include "sys/types.h"
#include "sys/map.h"
#include "crash.h"

/* get arguments for map function */
int
getmap()
{
	vaddr_t addr;
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		do {
			addr = strcon(args[optind],'h');
			fprintf(fp,"\n%s:\n",args[optind]);
			prmap(addr);
		}while(args[++optind]);
	}
	else longjmp(syn,0);
}

/* print map */
int
prmap(addr)
vaddr_t addr;
{
	struct map mbuf;
	unsigned units = 0, seg = 0;

	readmem(addr,1,-1,(char *)&mbuf,
		sizeof mbuf,"map table");
	fprintf(fp,"MAPSIZE: %u\tSLEEP VALUE: %u\n",
		mbuf.m_size,
		mbuf.m_addr);
	fprintf(fp,"\nSIZE    ADDRESS\n");
	for(;;) {
		addr += sizeof(mbuf);
		readmem(addr,1,-1,(char *)&mbuf,sizeof(mbuf),"map table");
		if (!mbuf.m_size) {
			fprintf(fp,"%u SEGMENTS, %u UNITS\n",
				seg,
				units);
			return;
		}
		fprintf(fp,"%4u   %8x\n",
			mbuf.m_size,
			mbuf.m_addr);
		units += mbuf.m_size;
		seg++;
	}
}
