#ident	"@(#)crash:common/cmd/crash/snode.c	1.2.2.1"

/*
 * This file contains code for the crash functions:  snode.
 */

#include <sys/param.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/conf.h>
#include <sys/fs/snode.h>
#include <sys/fs/devmac.h>
#include "crash.h"

static vaddr_t Snode, Mac_installed;
int mac_installed;		

/* get arguments for snode function */
int
getsnode()
{
	int slot;
	int full = 0;
	long arg1;
	long arg2;
	int c;
	char *baseheading = "SLOT  MAJ/MIN     REALVP    COMMONVP    NEXTR          SIZE       COUNT FLAGS \n";
	char *secheading  = "SLOT  MAJ/MIN     REALVP    COMMONVP    NEXTR          SIZE       COUNT FLAGS  S_DSECP  S_DSTATE S_DMODE S_SECFLAG D_LOLID D_HILID D_RELFLAG \n";
	char *heading;

	if(!Snode)
		Snode = symfindval("spectable");
	if (!Mac_installed)
		Mac_installed = symfindval("mac_installed");
	readmem(Mac_installed,1,-1,
		&mac_installed,sizeof mac_installed,"mac_installed");
	if (mac_installed)
		heading= secheading;
	else 	heading= baseheading;

	optind = 1;
	while((c = getopt(argcnt,args,"fw:")) !=EOF) {
		switch(c) {
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	fprintf(fp,"SNODE TABLE SIZE = %d\n", SPECTBSIZE);
	if(!full)
		fprintf(fp,"%s",heading);
	if(args[optind]) {
		do {
			if (getargs(SPECTBSIZE,&arg1,&arg2))
			    for(slot = arg1; slot <= arg2; slot++)
				prsnode(full,slot,-1,heading);
			else
				prsnode(full,-1,arg1,heading);
		}while(args[++optind]);
	}
	else for(slot = 0; slot < SPECTBSIZE; slot++)
		prsnode(full,slot,-1,heading);
}



/* print snode table */
int
prsnode(full,slot,addr,heading)
int full,slot;
long addr;
char *heading;
{
	struct snode *snp, snbuf;
	struct devmac secmac;
	vaddr_t Ksecp;

	if(addr == -1) {
		if(slot == -1)
			return;
		readmem(Snode+slot*(sizeof snp),1,-1,
			&snp,sizeof snp,"snode address");
		if(snp == NULL)
			return;
		readmem((vaddr_t)snp,1,-1,&snbuf,sizeof snbuf,"snode table");
	}
	else
		readmem(addr,1,-1,&snbuf,sizeof(snbuf),"snode table");

	while( 1 )
	{
		if(full)
			fprintf(fp,"%s",heading);

		if(slot == -1)
			fprintf(fp,"  - ");
		else fprintf(fp,"%4d",slot);
		fprintf(fp," %4u,%-5u %8x    %8x %8Lx ",
			getemajor(snbuf.s_dev),
			geteminor(snbuf.s_dev),
			snbuf.s_realvp,
			snbuf.s_commonvp,
			snbuf.s_nextr);

		/*
		 * UNKNOWN_SIZE == OFF_MAX == 0x7fffffffffffffff.
		 * OFF_MAX is not within _KMEMUSER defines so the hard code
		 * value is used instead.
		 */
		if (snbuf.s_devsize == 0x7fffffffffffffff)
			fprintf(fp,"%16s %8d", "UNKNOWN_SIZE", snbuf.s_count);
		else
			fprintf(fp,"%16Lx %8d", snbuf.s_devsize, snbuf.s_count);

		fprintf(fp,"%s%s%s%s",
			snbuf.s_flag & SUPD ? " up" : "",
			snbuf.s_flag & SACC ? " ac" : "",
			snbuf.s_flag & SWANT ? " wt" : "",
			snbuf.s_flag & SCHG ? " ch" : "");
	
		if (mac_installed) {
			fprintf(fp, "%8x %5d %5d",
			snbuf.s_dsecp,
			snbuf.s_dstate,
			snbuf.s_dmode);
			fprintf(fp, "%s%s%s\n",
			snbuf.s_secflag & D_INITPUB ? " ipub" : "",
			snbuf.s_secflag & D_RDWEQ ? " rweq" : "",
			snbuf.s_secflag & D_NOSPECMACDATA ? " nomac" : "");
			Ksecp = (vaddr_t)snbuf.s_dsecp;
			if (Ksecp){
				readmem(Ksecp, 1, -1, &secmac, sizeof(secmac),
					"kernel security structure");
				fprintf(fp," %ld %ld %d",
				secmac.d_lolid,
				secmac.d_hilid,
				secmac.d_relflag);
			}
		}
		fprintf(fp, "\n");
		if(full)
		{
			/* print vnode info */
			fprintf(fp,"VNODE :\n");
			fprintf(fp,vnheading);
			cprvnode(&snbuf.s_vnode);
			fprintf(fp,"\n");
		}

		if(snbuf.s_next == NULL)
			return;
		snp = snbuf.s_next;
		readmem((long)snp,1,-1,(char *)&snbuf,sizeof snbuf,
			"snode table");
	}
}
