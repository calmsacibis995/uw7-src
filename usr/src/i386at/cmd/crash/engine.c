#ident	"@(#)crash:i386at/cmd/crash/engine.c	1.1.1.1"

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <memory.h>
#include <sys/plocal.h>
#include <sys/vmparam.h>
#include <sys/disp_p.h>
#include <sys/procset.h>
#include <sys/processor.h>
#include "crash.h"

int
getengine()
{
	int c;
	int engid;
	int bind = 0;
	static processorid_t bound = PBIND_NONE;

	optind = 1;
	while((c = getopt(argcnt,args,"bw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'b' :	bind = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(bind && !active)
		error("bind option -b is unsupported on a dumpfile\n");
	if(args[optind]) {
		engid = strcon(args[optind++],'d');
		if (args[optind])
			longjmp(syn,0);
		check_engid(engid);
		if(bind && Nengine > 1
		&& processor_bind(P_PID, getpid(), engid, &bound) != 0)
			error("cannot bind crash to engine %d\n", engid);
		get_context(engid);
	}
	else if(bind && bound != PBIND_NONE
	&& processor_bind(P_PID, getpid(), PBIND_NONE, &bound) != 0)
		error("cannot unbind crash from engine %d\n", bound);
	pr_context();
}

int
getplocal()
{
	int c, engid;
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
		engid = strcon(args[optind++],'d');
		if (all || args[optind])
			longjmp(syn,0);
		prplocal(engid);
	}
	else if (all) {
		for (engid = 0; engid < Nengine; engid++) {
			if (engid)
				fprintf(fp,"\n");
			prplocal(engid);
		}
	}
	else
		prplocal(Cur_eng);
}

int
prplocal(engid)
int engid;
{
	struct plocal l;
	char spin;

	readplocal(engid, &l);
	fprintf(fp,"Plocal structure for engine %d:\n\n", engid);
	fprintf(fp,"kl1ptp:\t 0x%08x\t",l.kl1ptp);
	fprintf(fp,"engine mask:\t0x%x\n",l.eng_mask);
	fprintf(fp,"fpuon:\t 0x%08x\t",l.fpuon);
	fprintf(fp,"fpuoff:\t 0x%08x\n",l.fpuoff);
	fprintf(fp,"usingfpu:\t0x%x\t",l.usingfpu);
	fprintf(fp,"cpuspeed:\t0x%x\n",l.cpu_speed);
	fprintf(fp,"eventflags:\t0x%x\t",l.eventflags & 0x03);
	fprintf(fp,"one_sec:\t0x%x\n",l.one_sec);
	fprintf(fp,"holdfastlock:\t%s\n", l.holdfastlock?"B_TRUE":"B_FALSE");
	if( l.fspin ) {
		readmem((vaddr_t)l.fspin,1,-1,&spin,sizeof(fspin_t),"fspin_t");
		fprintf(fp,"fspin:\t0x%x \t*(l.fspin) = 0x%x\n",l.fspin,spin);
	}
}
