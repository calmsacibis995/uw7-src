#ident	"@(#)kern-i386:proc/cg_f.c	1.1.3.2"
#ident	"$Header$"

#include <proc/cg.h>
#include <proc/cguser.h>
#include <proc/disp.h>
#include <util/processor.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/ghier.h>

int	cg_kbindindex[MAXNUMCG]; /* which CPU in CG * to send cg_kbind() */
int	cg_dtimeindex[MAXNUMCG]; /* which CPU in CG * to send cg_dtimeout() */
cgnum_t cpu_to_cg[MAXNUMCPU];	 /* cpu to cg translation */
cg_t 	cg_array[MAXNUMCG];	/* CG array */

global_cginfo_t 	global_cginfo;


/* 
 * void cglocal_init(cgnum_t cgnum)
 * 
 * Calling/Exit State:
 *
 * Description:
 *
 * 	First time this CG is coming up.
 * 	CG level & cglocal initialization done here 
 */
void
cglocal_init(cgnum_t cgnum)
{
    	struct engine *eng, *next_eng;
	int i;
    	extern int strnsched;    

    	eng = PROCESSOR_MAP(cg_array[cgnum].cg_cpuid[0]);

        cg.strnsched = strnsched;
        cg.Nsched = 0;

	ASSERT(IsCGOnline(cgnum) == B_FALSE);
	ASSERT(cg.cg_num == cgnum);

    	cgnum = cg.cg_num;
	Ncgonline++;
	cg.cg_feng = eng; 
        cg.lastpick = cg.cg_feng;
	next_eng = NULL;
	i = cg_array[cgnum].cg_cpunum;
	/*
	 * Link up the engines on this CG.
	 */
	do {
		--i;
		eng = PROCESSOR_MAP(cg_array[cgnum].cg_cpuid[i]);
		eng->e_next = next_eng;
		next_eng = eng;
	} while (i != 0);
	if (cgnum == BOOTCG) {
		NcgonlineU++;
		cg_array[cgnum].cg_status |= CG_FULLONLINE;
	} else
		cg_array[cgnum].cg_status |= CG_HALFONLINE;
		
	/* initialize metric-related spin locks */
        met_init_cg();
}
