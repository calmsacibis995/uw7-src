#ident	"@(#)kern-i386:svc/msintr.c	1.1.5.1"
#ident	"$Header$"

#include <proc/seg.h>
#include <util/plocal.h>
#include <util/inline.h>
#include <util/debug.h>
#include <io/prf/prf.h>
#include <svc/psm.h>
#include <util/ipl.h>
#include <util/types.h>
#include <mem/kmem.h>
#include <util/ksinline.h>
#include <util/cmn_err.h>
#include <util/mod/mod_intr.h>
#include <util/mod/moddrv.h>

extern ms_intr_dist_t *os_intr_dist_stray, *os_intr_dist_nop;
extern ms_intr_dist_t *(*uw_flhdlr_table[])(ms_ivec_t vec);
extern k_pl_t	ipl;
extern ulong_t	ulbolt;

void intr_service(struct intr_vect_t *);
void intr_defer(struct intr_vect_t *);
void intr_stray(struct intr_vect_t *, ms_ivec_t);
void intr_undefer();
STATIC int tick1clock();

extern void lclclock(), todclock();
extern int xcall_intr(), softint_hdlr();

/*
 * The following are pseudo-idt entries used for handling clock, soft, xint, etc
 * events and interrupts thru the interrupt machinery.
 */
struct intr_vect_t		idt_clock_user;
struct intr_vect_t		idt_clock_kernel;
struct intr_vect_t		idt_xcall;
struct intr_vect_t		idt_swint;

/*
 * Macro used to initialize the pseudo-iv entries
 */
#define IVINIT(iv,proc,pl,event,arg)	iv.iv_ivect = proc,		\
					iv.iv_intpri = pl,		\
					iv.iv_idata = (void *)arg,	\
					iv.iv_osevent = event



ulong_t ulbolt = 0;             /* free-flowing lbolt counter 
				   ZZZ revisit this when clock definition is complete */


/*
 * Array of mailboxes used for soft interrupt passing.  
 * No cache line padding is needed because a cpu
 * references its mailbox ONLY when it receives a xint.
 */
volatile uint_t	*softmbox=NULL;


/*
 * intr_handle
 *	The standard path for handling an interrupt.
 *	The assembler code in the idt transfers control here as quickly
 *	as possible, without enabling interrupts.  Because interrupts
 *	are not enabled, it is safe to touch all our per-engine data without
 *	worrying about locking.
 *
 *	First, we ask the PSM to map the vector to an interrupt slot.
 *	The ms_intr_dist_t is assumed to have the MASK_ON_INTR flag, so on
 *	return the interrupt is masked.  If the interrupt corresponds to
 *	a known I/O device, we service or defer it; in any case
 *	we handle any event flags that may have been set by os_post_events.
 *
 *	The final conditional call to intr_undefer() catches any interrupts that were 
 *	deferred while we servicing the current interrupt.
 *
 *	The mseventsdeferred flags are used to prevent deferring more than
 *	interrupt of a kind.  Since some interrupts cannot be
 *	masked and just keep on coming, we need to something to prevent
 *	deferred ticks from exhausting the deferqueue freelist.  The
 *	flag is set before we give the interrupt to intr_defer, and cleared
 *	when the interrupt is serviced. 
 *
 *	ZZZ TODO - interrupt statistics
 */
/* ARGSUSED */
void
intr_handle(ms_ivec_t vec, uint edx, uint ecx, uint eax,
	uint es, uint ds,
	uint eip, uint cs, uint efl)
{
	struct intr_vect_t	*ivp;

	/*
	 * Call PSM to locate the IDT for the interrupt source. Depending
	 * on the source & priority, we ignore it, service it, or defer it.
	 */
	ivp = IDTP2IVP ((*uw_flhdlr_table[vec])( vec ));
	if (ivp == IDTP2IVP (os_intr_dist_stray)) {
		intr_stray(ivp, vec);

	} else if (ivp != IDTP2IVP (os_intr_dist_nop)) {
		if (ivp->iv_intpri <= ipl)
			intr_defer(ivp);
		else
			intr_service(ivp);
	}


	/*
	 * If any OS events have been posted and not yet serviced, service
	 * them now. Note that the checks are ordered highest priority first.
	 */
	if (l.msevents)  {

#ifndef UNIPROC
		if (l.msevents & MS_EVENT_XCALL) {
			l.msevents &= ~MS_EVENT_XCALL;
			intr_service(&idt_xcall);
		}
#endif

		if (l.msevents & MS_EVENT_TICK_2) {
			l.msevents &= ~MS_EVENT_TICK_2;
			if (prfstat)
				prfintr(eip,  USERMODE(cs, efl));
		}
	
		if( l.msevents & MS_EVENT_TICK_1 ) {
			l.msevents &= ~MS_EVENT_TICK_1;
#ifdef _MPSTATS
			if (myengnum == BOOTENG) 	/* ZZZ revisit this later */
				ulbolt += (1000000/HZ);
#endif
			ivp = USERMODE(cs, efl) ? &idt_clock_user : &idt_clock_kernel;
			if (PLHI <= ipl)
				intr_defer(ivp);
			else
				intr_service(ivp);
		}

#ifndef UNIPROC
		if (l.msevents & MS_EVENT_SOFT) {
			l.msevents &= ~MS_EVENT_SOFT;
			l.eventflags |= atomic_fnc( &softmbox[myengnum] );
			if (PL1 <= ipl)
				intr_defer(&idt_swint);
			else
				intr_service(&idt_swint);
		}
#endif
	}

	/* 
	 * Now service any interrupts that got deferred while servicing the
	 * original interrupt.
	 */
	if( l.picipl > ipl )
		intr_undefer();

}


/*
 * intr_service
 *	Invoking a handler at the appropriate priority.
 *
 * 	Note that the spl routines cannot be used for doing the priority
 * 	manipulations here - this isn't a good time to go off and
 * 	check the deferred queue.
 */
STATIC void
intr_service(struct intr_vect_t *ivp)
{
	pl_t opl;
	int ret;

	ASSERT(!is_intr_enabled());

	plocal_intr_depth++;
	opl = ipl;
	ipl = ivp->iv_intpri;
	if( ipl <= PLHI )
		asm("sti");

	ret = (ivp->iv_ivect)(ivp->iv_idata);
	asm("cli");

	/* XXX
	 * We need to check the return value
	 */
	
	if( !ivp->iv_osevent)				/* indicates non-IO idt */
		ms_intr_complete(IVP2IDTP(ivp));

	ipl = opl;
	plocal_intr_depth--;
}


/*
 * intr_defer
 * 	Defer an interrupt because its priority is not higher than the
 * 	current priority.  The interrupt is linked into the deferqueue,
 * 	which is sorted by priority.  The picipl variable is updated to
 * 	reflect the highest priority deferred interrupt; this value is checked
 * 	by the spl() routines when lowering the priority.
 */
STATIC void
intr_defer(struct intr_vect_t *ivp )
{
	dfrq_t *new, **dqpp;
	pl_t	pl;

	ASSERT(!is_intr_enabled());
	if ( !(l.mseventsdeferred&ivp->iv_osevent)) {
		l.mseventsdeferred |= ivp->iv_osevent;
		pl = ivp->iv_intpri;
		if( l.picipl < pl )
			l.picipl = pl;

		dqpp = &l.deferqueue;
		while(*dqpp && pl <= (*dqpp)->dq_ivp->iv_intpri)
			dqpp = &((*dqpp)->dq_next);

		new = l.dqfree;
		l.dqfree = new->dq_next;

		new->dq_ivp = ivp;
		new->dq_next = *dqpp;
		*dqpp = new;
	}

}


/*
 * intr_stray
 * 	Handle a stray interrupt as reported by the PSM.
 */
void
intr_stray(struct intr_vect_t *ivp, ms_ivec_t vec)
{
#ifdef DEBUG
	asm("sti");
	cmn_err(CE_NOTE, "Stray interrupt received by cpu %d, vector 0x%x, slot 0x%x.\n", 
			myengnum, vec, ivp->iv_idt.msi_slot);
	asm("cli");
#endif
}


/*
 * intr_undefer
 *	Service the deferred interrupts.  This routine is called from the spl()
 *	routines when the priority is being lowered, which may make one or
 *	more deferred interrupts eligible for processing.  It is possible that
 *	there will be no deferred interrupts by the time we get here, as
 *	interrupts were enabled while the check against picipl was made
 *	when lowering the ipl, and the deferred interrupts may have been
 *	then.
 *
 *	Note that deferqueue must be read on every iteration of the loop,
 *	since intr_service has enabled interrupts and the list of deferred
 *	interrupts may have changed.
 */
void
intr_undefer()
{
	dfrq_t		*dqp;
	
	ASSERT(!is_intr_enabled());

	dqp = l.deferqueue;
	while (l.picipl > ipl) {
		ASSERT(!dqp->dq_ivp->iv_osevent || 
			l.mseventsdeferred&dqp->dq_ivp->iv_osevent);
		l.deferqueue = dqp->dq_next;
		l.picipl = (l.deferqueue== NULL) ? 0 : l.deferqueue->dq_ivp->iv_intpri;
		l.mseventsdeferred &= ~dqp->dq_ivp->iv_osevent;
		dqp->dq_next = l.dqfree;
		l.dqfree = dqp;
		intr_service(dqp->dq_ivp);
		dqp = l.deferqueue;
	}
}


/*
 * tick1clock
 *	This function is a wrapper around the global/local clock functions.
 *
 *	The argument to tick1clock is used to distinguish user-mode
 *	and kernel-mode clock ticks.
 */
STATIC int
tick1clock(uint_t arg)
{
#ifdef CCNUMA
		/* 
		 * One Cpu in each CG is designated to be the
		 * the one that handles the per CG level
		 * callout table
		 */

	engine_t *eng;

	eng = PROCESSOR_MAP(myengnum);

	ASSERT(eng != NULL);

	if (eng->e_flags & E_CGLEADER)
		cgclock();
#else /* CCNUMA */
	if (myengnum == 0)
		todclock();
#endif /* CCNUMA */
	lclclock(arg);
	return 0;
}


/*
 * void msintrinit(fullinit)
 *
 * Description:
 *
 * Initialize the interrupt handling and deferral machinery.
 * Allocate and initialize the queue of deferred interrupts and the
 * free list of deferred interrupt entries.  Initialize the array
 * of mailboxes used for software interrupts.
 *
 */
void
msintrinit()
{
	unsigned i;

	ipl = PLHI;

	if( l.dqfree == NULL ) {
		l.deferqueue = NULL;
		l.dqfree = (dfrq_t *) os_alloc( (1+os_islot_max) * sizeof(dfrq_t) );
		l.dqfree[os_islot_max].dq_next = NULL;
		for( i = 0; i < os_islot_max; i++ )
			l.dqfree[i].dq_next = &l.dqfree[i+1];
	} else {
		ASSERT (l.deferqueue == NULL);
	}
	

	if (softmbox == NULL) {
#ifndef UNIPROC
		IVINIT (idt_xcall, xcall_intr, PLHI, MS_EVENT_XCALL, 0);
#endif
		IVINIT (idt_clock_user, tick1clock, PLHI, MS_EVENT_TICK_1, 1 /* user mode */);
		IVINIT (idt_clock_kernel, tick1clock, PLHI, MS_EVENT_TICK_1, 0 /* !user mode */);
		IVINIT (idt_swint, softint_hdlr, PL1, MS_EVENT_SOFT, 0);
		softmbox = (uint_t *) os_alloc( Nengine * sizeof(*softmbox) );
	}
}


/*
 * Softint machinery.
 *
 * Soft interrupts are implemented by setting the desired flags
 * in the mailbox, then poking the processor to make it go look
 * in the mailbox.  A self-directed soft interrupt omits the
 * cross-interrupt and directly invokes the acceptance function.
 *
 * The acceptance function copies the flags into the processor-local
 * copy of the flags, and then calls intr_defer to defer
 * the soft interrupt.  We must defer the
 * interrupt, since the software interrupt runs at the lowest non-base
 * priority level & and this routine is never called at base pl with
 * interrupts enabled.
 *
 * All manipulations of the mailbox should be done with atomic
 * operations, as multiple processors may be touching them
 * simultaneously.  The local eventflags must be manipulated either
 * with interrupts off or with atomic operations.
 *
 * Always called with interrupts OFF.
 *
 */
void
mssendsoft( int engnum, uint_t arg )
{
	ASSERT(!is_intr_enabled());

#ifndef UNIPROC
	if( engnum == myengnum ) {
#endif
		l.eventflags |= arg;
		intr_defer(&idt_swint);
#ifndef UNIPROC
	} else {
		atomic_or(&softmbox[engnum], arg );
		ms_xpost( engnum, MS_EVENT_SOFT );
	}
#endif
}


/*
 * boolean_t
 * intrpend(pl_t pl)
 *	Check for pending interrupts above specified priority level.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Returns B_TRUE (B_FALSE) if there are (are not) any pending
 *	interrupts above the specified priority level.
 *
 * Remarks:
 *	picipl is set to the ipl of the highest priority
 *	deferred interrupt on this processor; if picipl == PLBASE,
 *	then there are no deferred interrupts on this processor.
 */
boolean_t
intrpend(pl_t pl)
{
	return (l.picipl > pl);
}

/*
 * Wrapper for pre-DDI interrupt drivers.
 */
int
_Compat_intr_handler(void *idata)
{
	struct _Compat_intr_idata	*_Compat_ivec;

	_Compat_ivec = (struct _Compat_intr_idata *) idata;
		
	(_Compat_ivec->ivect)(_Compat_ivec->vec_no);
	return ISTAT_ASSUMED;
}

void
psm_sendsoft( int engnum, uint_t arg )
{
	mssendsoft(engnum, arg);
}

