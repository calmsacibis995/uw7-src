#ident	"@(#)crash:common/cmd/crash/disp.c	1.2.2.1"

/*
 * This file contains code for the crash functions:  dispq
 */

#include <sys/param.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/priocntl.h>
#include <sys/proc.h>
#include <sys/disp.h>
#include <sys/engine.h>
#include <vm/page.h>
#include <sys/clock.h>
#include <sys/cglocal.h>
#include "crash.h"

static vaddr_t	Nglobpris;	/* address for "nglobpris" */
static int	nglobpris;

runque_t	*cg_rq = NULL;	
list_t		*dispqbuf;

#define localq_msk	0x01
#define globalq_msk	0x02

/* get arguments for dispq function */
int
getdispq()
{
	int slot;
	int c;
	long arg1;
	long arg2;
	runque_t **rqpp, *rq;
	engine_t engine;
	int quemask = 0;
	int savoptind;
	struct cglocal	cgl;

	char *dispqhdg = "SLOT     FLINK       RLINK\n";

	if(!Nglobpris)
		Nglobpris = symfindval("nglobpris");
	readmem(Nglobpris, 1, -1,
		&nglobpris, sizeof nglobpris, "nglobpris");
        if(!cg_rq) {
		readcglocal(Cur_cg,&cgl);
		cg_rq = cgl.cg_rq;
	}

	/* Read in our engine_t structure in order to get our run queues */
	readengine(Cur_eng, &engine);

	optind = 1;
	while((c = getopt(argcnt,args,"lgw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'l' :	quemask |= localq_msk;
					break;
			case 'g' :	quemask |= globalq_msk;
					break;
			default  :	longjmp(syn,0);
		}
	}

	if (quemask == 0)
		quemask = localq_msk|globalq_msk;
	savoptind = optind;

	/* Loop through all run queues */
	for (rqpp = engine.e_rqlist; (rq = *rqpp) != NULL; rqpp++) {

		if(rq == cg_rq) {
			/* If this is the CG Run Queue then say so */
			if(quemask & globalq_msk)
				fprintf(fp, "CG Run Queue:\n");
			else
				continue;
		}
		else {
			/* If this is a Local Run Queue then say so */
			if(quemask & localq_msk)
				fprintf(fp, "Local Run Queue:\n");
			else
				continue;
		}

		/* Read in rq_dispq from the runque_t structure */
		/* Avoid reading whole structure because UniProc shorter */

		readmem((vaddr_t)&rq->rq_dispq, 1, -1, &dispqbuf,
	    		sizeof(list_t *), "runque rq_dispq");

		/* Read in the entire table of dispq headers */

		dispqbuf = readmem((vaddr_t)dispqbuf, 1, -1, NULL,
	    		nglobpris * sizeof(list_t), "dispq header table");

		fprintf(fp, "%s", dispqhdg);

		/* Print out all (or some) of the dispatch queue headers */
		optind = savoptind;
		if(args[optind]){
			do {
				if (!getargs(nglobpris,&arg1,&arg2))
				    error("priority %d is out of range 0-%d\n",
					arg1, nglobpris - 1);
				for(slot = arg1; slot <= arg2; slot++)
					prdispq(slot);
			}while(args[++optind]);
		} else for(slot = 0; slot < nglobpris; slot++)
			prdispq(slot);

		fprintf(fp, "\n");
	}
}

/* print dispq header table  */
int
prdispq(slot)
int slot;
{
	fprintf(fp, "%4d     %8x    %8x",
		slot, dispqbuf[slot].flink, dispqbuf[slot].rlink);

	if(dispqbuf[slot].flink != dispqbuf[slot].rlink)
		fprintf(fp, "***\n");
	else
		fprintf(fp, "\n");
}
