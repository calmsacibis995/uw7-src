#ident	"@(#)kern-i386at:util/engine.c	1.21.10.1"
#ident	"$Header$"

/*
 * Routines to deal with "engines" (another name for processors).
 */

#include <util/types.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/engine.h>
#include <svc/errno.h>
#include <svc/psm.h>
#include <util/debug.h>
#include <util/ipl.h>
#include <util/plocal.h>
#include <util/processor.h>
#include <util/kdb/xdebug.h>
#include <mem/immu.h>
#include <mem/kma.h>
#include <mem/kmem.h>
#include <mem/vmparam.h>
#include <proc/disp.h>
#include <svc/systm.h>
#include <acc/priv/privilege.h>
#include <svc/limitctl.h>
#include <proc/cg.h>
#include <util/cmn_err.h>


extern void limit(int, int *);

int	nonline	= 1;	/* first proc does not do online_engine() */

lock_t eng_tbl_mutex;
LKINFO_DECL(eng_tbl_lkinfo, "eng_tbl_mutex", 0);
sleep_t	onoff_mutex;
LKINFO_DECL(onoff_lkinfo, "onoff_mutex", 0);

/*
 * Used to mutex the "e_count" field.  This solves a fairly sticky
 * problem involving bind inheritance.  The lock heirachy between
 * is eng_tbl_mutex first and l_mutex after.  Unfortunately, when
 * creating a new lwp, we need to hold the l_mutex of both the parent
 * and child to prevent the bindings of either from being changed before
 * the inheritance can take place.  Fortunately, the only thing which
 * happens when a binding is inherited is 1) the l_rq of the parent is
 * given to the child and 2) the e_count of the engine is incremented.
 * (1) happens as a matter of course and (2) must happen without
 * acquiring eng_tbl_mutex (lock order violation).  Hence, inheritance
 * of a binding increments the e_count of an engine while holding
 * "eng_count_mutex".
 *
 * One more thing to note:  e_count can be incremented without holding
 * eng_tbl_mutex, but will be decremented only while holding eng_tbl_mutex
 * (as well as eng_count_mutex), thus, you can test e_count greater then
 * a certain value while holding eng_tbl_mutex and not eng_count_mutex.
 */
fspin_t eng_count_mutex;

event_t	eng_wait;

/*
 * Variable in pstart.s (physical addresses after relocation)
 *
 *	reset_code is the entry point to the real mode
 *	"jump start" code.
 *
 *	reset_cr3 contains the address of the root of the page
 *	tables for the starting engine.
 */
extern void	(*reset_code)(void);
extern uint_t	*reset_cr3;

/*
 * online_engno is used by online_engine() to communicate to _start()
 * the logical engine number (engine[] index) for the newly onlined engine.
 */
ulong_t online_engno = 0;

/*
 * void
 * engine_init(void)
 *	Initialize the locks used in this file.
 *
 * Calling/Exit State:
 *	Assume we're called from an lwp context.
 * 
 */
void
engine_init(void)
{
	SLEEP_INIT(&onoff_mutex, 0, &onoff_lkinfo, KM_NOSLEEP);
	EVENT_INIT(&eng_wait);
	FSPIN_INIT(&eng_count_mutex);
}

struct online_args {
	int	processor;
	int	flag;
};

/*
 * int
 * online(struct online_args *uap, rval_t *rvp)
 *
 *	This is the p_online system call.
 *
 * Calling/Exit State:
 *
 *	This is a system call.
 *
 * Description:
 *
 *	System call to bring a processor online, offline, find the
 *	current status and the maximum number.
 */
int
online(struct online_args *uap, rval_t *rvp)
{
	engine_t *eng;
	int error;

	if (uap->processor == PROC_BAD) {
		rvp->r_val1 = Nengine;
		return(0);
	}

        switch (uap->flag) {
        case P_ONLINE:
        	if (pm_denied(u.u_lwpp->l_cred, P_SYSOPS))
                	return EPERM;

	        if (!(error = online_engine(uap->processor))) {
		     eng = PROCESSOR_MAP(uap->processor);
		     if (eng->e_flags & (E_OFFLINE|E_SHUTDOWN))
		          rvp->r_val1 = P_OFFLINE;
		     else
		          rvp->r_val1 = P_ONLINE;
		}
	        return error;

        case P_OFFLINE:
        	if (pm_denied(u.u_lwpp->l_cred, P_SYSOPS))
                	return EPERM;

		if (!(error = offline_engine(uap->processor))) {
		     eng = PROCESSOR_MAP(uap->processor);
		     if (eng->e_flags & (E_OFFLINE|E_SHUTDOWN))
		          rvp->r_val1 = P_OFFLINE;
		     else
		          rvp->r_val1 = P_ONLINE;
	        }
	        return error;

        case P_QUERY:
                eng = PROCESSOR_MAP(uap->processor);
                if (eng == NULL)
                        return(EINVAL);

                if (eng->e_flags & E_BAD)
                        rvp->r_val1 = P_BAD;
                else if (eng->e_flags & (E_OFFLINE|E_SHUTDOWN))
                        rvp->r_val1 = P_OFFLINE;
                else
                        rvp->r_val1 = P_ONLINE;

#ifdef CCNUMA
		if (!(cg_array[CPUtoCG(uap->processor)].cg_status & CG_FULLONLINE))
                        rvp->r_val1 = P_OFFLINE;
#endif 
                return(0);
        }

        return(EINVAL);

}

void
online_failure(int *engno)
{
        engine_t *eng;

        eng = &engine[*engno];
        eng->e_flags |= E_BAD;
        EVENT_SIGNAL(&eng_wait, 0);
}

#ifdef CCNUMA
/*
 * int
 * online_engine_sbsp(cgnum_t)
 *
 *	Online a engine which is a Secondary Boot Strap Procesor.
 *
 * Calling/Exit State:
 *
 *	Called well before online_engine() can be called, in simulation
 *	only. No error checking from psm_online_engine. We assume that
 *	SBSP's can be brought online.
 *
 *	This function does not wait for the sbsp to actually online.
 *	It is the responsibility of the caller to 
 */
online_engine_sbsp(cgnum_t cgnum)
{
	extern int gToLeader[];
	extern cgnum_t this_cg_num;

	/*
	 * Normal online_engne
	 */
	this_cg_num = cgnum;

	/*
	 * Set the reset address and enable the engine to 
	 * access the memory bus.
	 */
	ms_start_cpu(gToLeader[cgnum], (ms_paddr_t)reset_code);
}
#endif /* CCNUMA */

/*
 * int
 * online_engine(int)
 *
 *	Online a engine and do error checking.
 *
 * Calling/Exit State:
 *
 *	Coordinates concurrent online/offline by acquiring
 *	onoff_mutex.
 *
 *	Returns the errno on error, zero on sucess.
 *	No op for AT (uniproc).
 */
int
online_engine(int engno)
{
	engine_t *eng;
	int error = 0;
	int  maxneng;
	long *kl1pt;
	toid_t id;
        paddr_t *newcpu_engno;
	int pfn;
	cgnum_t cgnum;

	/*
	 * Validate engine number.
	 */

	if ((unsigned int)engno >= Nengine)
		return EINVAL;

	eng = &engine[engno];

	/*
	 * Coordinate with concurrent online or offline.
	 */
	SLEEP_LOCK(&onoff_mutex, PRIZERO);

	limit(L_GETPROCLIMIT, &maxneng);
	if (nonline >= maxneng && maxneng < MAXSKEYS) {
		SLEEP_UNLOCK(&onoff_mutex);
		return EINVAL;
	}

	/*
	 * Check if engine is already online, return zero.
	 */

	if (!(eng->e_flags & E_OFFLINE)) {
#ifdef CCNUMA
		cgnum = CPUtoCG(engno);	
		if (cg_array[cgnum].cg_status & CG_HALFONLINE) {
			ASSERT(eng->e_flags & E_CGLEADER);
			cg_array[cgnum].cg_status |= CG_FULLONLINE;
			cg_array[cgnum].cg_status &= ~CG_HALFONLINE;
			NcgonlineU++;
			nonline++;
		}
#endif
		SLEEP_UNLOCK(&onoff_mutex);
		return 0;
	}

#ifdef CCNUMA
	cgnum = CPUtoCG(engno);	
	if (!(cg_array[cgnum].cg_status & CG_FULLONLINE)) {
		SLEEP_UNLOCK(&onoff_mutex);
		return EINVAL;
	}
#endif 

	/*
	 * Check if engine is bad (failed diagnostics).
	 */

	if (eng->e_flags & E_BAD) {
		SLEEP_UNLOCK(&onoff_mutex);
		return ENODEV;
	}

	eng->e_flags &= ~E_SHUTDOWN;

	EVENT_CLEAR(&eng_wait);

	/*
	 * online_kl1pt is used to communicate to reset_code() the physical
	 * address of the newly onlined engine's level 1 page table.
	 *
	 * online_engno is used by online_engine() to communicate to
	 * reset_code() the logical engine number (engine[] index) for
	 * the newly onlined engine.
	 */

	online_engno = engno;

	kl1pt = (void *)physmap((paddr_t)reset_cr3,
				sizeof(paddr_t), KM_SLEEP);
#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		*kl1pt = kvtophys64((vaddr_t)
				    ENGINE_PLOCAL_PAE_PTR(engno)->pdpte);
		/*
		 * set the CPUFEAT_PAE flag before launching the 
		 * engine 
		 */ 
		ENGINE_PLOCAL_PAE_PTR(engno)->cpu_features[0] 
			|= CPUFEAT_PAE;
	} else
#endif /* PAE_MODE */
	{
		*kl1pt = 
			kvtophys((vaddr_t)&eng->e_local->pp_kl1pt[0][0]);
	}

	physmap_free((caddr_t) kl1pt, sizeof(paddr_t), 0);

	id = dtimeout(online_failure, &online_engno, 5*HZ, PLBASE, 
		      myengnum);
	ms_start_cpu(engno, (ms_paddr_t)reset_code);

	/* Wait for the target engine to signal online completion. */
        EVENT_WAIT(&eng_wait, PRIZERO);

	untimeout(id);

	if (eng->e_flags & E_BAD) {
		SLEEP_UNLOCK(&onoff_mutex);
		return ENODEV;
	}

	if ((error = kma_online(eng)) == 0) 
		++nonline;

        SLEEP_UNLOCK(&onoff_mutex);
	
        return error;
}


/*
 * int
 * offline_engine(int)
 *	Offline a engine and do error checking.
 *
 * Calling/Exit State:
 *	Coordinates concurrent online/offline by acquiring
 *	onoff_mutex.
 *
 *	Returns the errno on error, zero on sucess.
 *	No op for AT (uniproc).
 */
int
offline_engine(int engno)
{
	engine_t *eng;
	pl_t pl;

	/*
	 * Validate engine number.
	 */

	if ((unsigned int)engno >= Nengine)
		return EINVAL;

	eng = &engine[engno];

	/*
	 * Coordinate with concurrent online or offline.
	 */
	SLEEP_LOCK(&onoff_mutex, PRIZERO);

	/*
	 * If engine is already offline return zero.  If engine
         * is bad, return error.
	 */

	if (eng->e_flags & E_OFFLINE) {
		SLEEP_UNLOCK(&onoff_mutex);
		return 0;
	}

	if (eng->e_flags & E_BAD) {
		SLEEP_UNLOCK(&onoff_mutex);
		return ENODEV;
	}

	/*
	 * If this is the only processor online, then error.
	 */

	if (nonline == 1) {
		SLEEP_UNLOCK(&onoff_mutex);
		return EBUSY;
	}

#ifdef CCNUMA
	if (eng->e_flags & E_CGLEADER) {
		SLEEP_UNLOCK(&onoff_mutex);
		return EBUSY;
	}
#endif

	kma_offline(eng);

	pl = LOCK(&eng_tbl_mutex, PLHI);

	/*
	 * Call into the dispatcher and see if this engine
	 * cannot be taken offline due to bindings (exclusive or otherwise).
	 *
	 * If the engine's currently got bound drivers, refuse to offline
	 * things.
	 */
	if (!dispofflineok(eng) || eng->e_flags & E_DRIVERBOUND) {
		UNLOCK(&eng_tbl_mutex, pl);
		(void) kma_online(eng);
		SLEEP_UNLOCK(&onoff_mutex);
		return EBUSY;
	}

	/*
	 * Shutdown ok, set shutdown request bit while holding
	 * engine table mutex.  Setting of the shutdown request
	 * bit and nudge must be atomic to avoid races.
	 */

	EVENT_CLEAR(&eng_wait);

	eng->e_flags |= E_SHUTDOWN;
	UNLOCK(&eng_tbl_mutex, pl);

	/*
	 * Nudge engine and wait for engine to
	 * see shutdown request.
	 */
	RUNQUE_LOCK();
	kpnudge(PRINPRIS - 1, eng);
	RUNQUE_UNLOCK();

	EVENT_WAIT(&eng_wait, PRIZERO);
	nonline--;
	SLEEP_UNLOCK(&onoff_mutex);
	return 0;
}

/*
 * boolean_t
 * engine_disable_offline(int engno)
 *      Test and lock the cpu to turn off offline capability.
 *
 * Calling/Exit State:
 *	The global onoff_mutex sleep lock may or may not be held
 *	on entry.
 *
 * Description:
 *	If the cpu is offline, this function returns B_FALSE.
 *	Otherwise, this function sets the E_DRIVERBOUND flag and
 *	turn off the offline capability of the engine.
 */
boolean_t
engine_disable_offline(int engno)
{
	engine_t *eng;
	pl_t pl;

	/*
	 * Validate engine number.
	 */
	if ((unsigned int)engno >= Nengine)
		return B_FALSE;

	eng = &engine[engno];
	pl = LOCK(&eng_tbl_mutex, PLHI);

	if (eng->e_flags & E_NOWAY) {
		UNLOCK(&eng_tbl_mutex, pl);
		return B_FALSE;
	}

	eng->e_flags |= E_DRIVERBOUND;
	UNLOCK(&eng_tbl_mutex, pl);

	return B_TRUE;
}


/*
 * int
 * engine_state(int engnum, uint_t state, void *rval)
 *	Get the state of the engine.
 *
 * Calling/Exit State:
 *	<engnum> is a the engine number whose state is to be determined.
 *	<state> is the engine state that needs to be returned.
 *	<rval> is the return value data buffer.
 *
 *	Returns EINVAL if an incorrect argument is specified, otherwise
 *	return 0.
 *
 *	No locking assumptions.
 */
int
engine_state(int engnum, uint_t state, void *rval)
{
	engine_t *eng;

	if ((eng = PROCESSOR_MAP(engnum)) == NULL)
		return EINVAL;

	switch (state) {
	case ENGINE_ONLINE:
		*(boolean_t *)rval = !(PROCESSOR_NOWAY(eng));
		break;
	default:
		return EINVAL; 
	};

	return 0;
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_engine(engine_t *)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Thus function is intended for use from a kernel debugger.
 */
void
print_engine(engine_t *engp)
{
	int i;
#	define RQLISTSZ	sizeof(engp->e_rqlist)/sizeof(engp->e_rqlist[0])

	debug_printf("e_cpu_speed %d\te_flags 0x%x\te_local 0x%x\t"
		"e_cglocal 0x%x\n",
		engp->e_cpu_speed, engp->e_flags, engp->e_local,
		engp->e_cglocal);
	debug_printf("e_pri %d\te_npri %d\te_count %d\t"
		"e_nsets %d\n",
		engp->e_pri, engp->e_npri, engp->e_count,
		engp->e_nsets);
	debug_printf("e_nsets %d\te_setsize %d\te_local_todo 0x%x\t"
		"e_lastpick 0x%x\n",
		engp->e_nsets, engp->e_setsize, engp->e_local_todo,
		engp->e_lastpick);
	for (i = 0; i < RQLISTSZ; i++)
		debug_printf("rq_list[%d] 0x%x\t", i, engp->e_rqlist[i]);
	debug_printf("\ne_smodtime.tv_sec %d\te_smodtime.tv_nsec %d\n",
		engp->e_smodtime.tv_sec, engp->e_smodtime.tv_nsec);
}

#endif

