#ident	"@(#)crash:i386at/cmd/crash/cg.c	1.2.1.2"

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <memory.h>
#include <sys/plocal.h>
#include <sys/vmparam.h>
#include <sys/disp_p.h>
#include <sys/procset.h>
#include <sys/processor.h>
#include <vm/page.h>
#include <sys/clock.h>
#include <sys/cglocal.h>
#include <sys/cg.h>
#include "crash.h"

int
getcg()
{
int	c, cgnum;

	optind = 1;
	while((c = getopt(argcnt,args,"bw:")) !=EOF) {
		switch(c) {
			case 'w' :      redirect();
				break;
			default  :      longjmp(syn,0);
		}
	}
	if(args[optind]) {
		cgnum = strcon(args[optind++],'d');
		if (args[optind])
			longjmp(syn,0);
		check_cgnum(cgnum);
	}
	
	pr_context();
	return 0;
}

int
getcglocal()
{
	int	cgnum;
	int c;
	int all = 0;
	
	optind = 1;
	while((c = getopt(argcnt,args,"ew:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'e' :	all = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		cgnum = strcon(args[optind++],'d');
		if (all || args[optind])
			longjmp(syn,0);
		prcglocal(cgnum);
	}
	else if (all) {
		for (cgnum = 0; cgnum < Ncg; cgnum++) {
			if (cgnum)
				fprintf(fp,"\n");
			prcglocal(cgnum);
		}
	}
	else
		prcglocal(Cur_cg);
}

int
prcglocal(int cgnum)
{
	struct cglocal cgl;
	char spin;

	readcglocal(cgnum, &cgl);
	fprintf(fp,"CGlocal structure for cg %d:\n\n", cgnum);
	fprintf(fp,"cg_nonline:\t0x%x\t",cgl.cg_nonline);
	fprintf(fp,"cg_nidle:\t0x%x\n",cgl.cg_nidle);
	fprintf(fp,"cg_avgqlen:\t0x%x\n",cgl.cg_avgqlen);
}
