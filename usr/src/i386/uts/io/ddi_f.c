#ident	"@(#)kern-i386:io/ddi_f.c	1.14.2.1"
#ident	"$Header$"

/*
 * platform-specific DDI routines
 */

#include <mem/faultcatch.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/msr.h>
#include <svc/systm.h>
#include <svc/uadmin.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

#define _DDI_C
#include <io/ddi.h>
#include <io/f_ddi.h>

static ulong_t mcg_status[2];
static ulong_t mci_status[2];

/*
 * int
 * etoimajor(major_t emajnum)     
 *	return internal major number corresponding to external
 *	major number argument or -1 if the external major number
 *	exceeds the bdevsw and cdevsw count *
 *	For the 386 internal major = external major
 *
 * Calling/Exit State:
 *	none
 */
int
etoimajor(major_t emajnum)
{
	if (emajnum > L_MAXMAJ)
		return (-1); /* invalid external major */

	return ((int) emajnum);
}

/* 
 * int
 * itoemajor(major_t imajnum, int lastemaj)
 *	return external major number corresponding to internal
 *	major number argument or -1 if no external major number
 *	can be found after lastemaj that maps to the internal
 *	major number. Pass a lastemaj val of -1 to start
 *	the search initially. (Typical use of this function is
 *	of the form:
 *	     lastemaj=-1;
 *	     while ( (lastemaj = itoemajor(imag,lastemaj)) != -1)
 *	        { process major number }
 *	For the 386 internal major = external major
 *
 * Calling/Exit State:
 *	none
 */
int
itoemajor(major_t imajnum, int lastemaj)
{
	extern int bdevcnt, cdevcnt;

	if (imajnum >= max(bdevcnt, cdevcnt))
		return (-1);

	/* if lastemaj == -1 then start from beginning of MAJOR table */
	if (lastemaj == -1)
		return ( (int) imajnum);

	return(-1);
}


#define	CALLBACK_HIER	0
#define	CALLBACK_SIZE	sizeof(struct callback)

STATIC struct callback {
	int	(*cb_func)();
	void	*cb_args;
	struct callback	*cb_next;
};

STATIC struct callback *nmihdlrs;
STATIC struct callback *wdhdlrs;
STATIC struct callback *pwrdwnhdlrs;	/* Power Down Handlers */
STATIC struct callback *mcahdlrs;	/* Machine Check Handlers */

STATIC lock_t callback_lock;
STATIC LKINFO_DECL(callback_lkinfo, "DDI:CB:callback_lock", 0);

/*
 * void 
 * drv_callback(int tag, int (*handler)(), void *args)
 *	DDI/DKI Version 7.1 drv_callback.  It introduces
 *	POWERDOWN_ATTACH/POWERDOWN_DETACH and WATCHDOG_ALIVE_ATTACH/
 *	WATCHDOG_ALIVE_DETACH.
 *
 * Calling/Exit State:
 *	May block, so should called be called with no locks held.
 */
void
drv_callback(int tag, int (*handler)(), void *args)
{
	pl_t	opl;
	struct callback *cbp;
	struct callback	**cbpp;		/* pointer to previous callback
					 * struct in the list
					 */
	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	switch(tag) {
	case MCA_ATTACH:
		cbpp = &mcahdlrs;
		goto attach;

	case WATCHDOG_ALIVE_ATTACH:
		cbpp = &wdhdlrs;
		goto attach;

	case POWERDOWN_ATTACH:
		cbpp = &pwrdwnhdlrs;
		goto attach;

	case NMI_ATTACH:
		cbpp = &nmihdlrs;
attach:
		cbp = kmem_alloc(CALLBACK_SIZE, KM_SLEEP);

		(void) LOCK_PLMIN(&callback_lock);

		cbp->cb_func = handler;
		cbp->cb_args = args;

		/*
		 * Prepend the handler to the list.
		 */
		cbp->cb_next = *cbpp;
		*cbpp = cbp;

		UNLOCK_PLMIN(&callback_lock, PLBASE);
		return;

	case MCA_DETACH:
		cbpp = &mcahdlrs;
		goto detach;

	case WATCHDOG_ALIVE_DETACH:
		cbpp = &wdhdlrs;
		goto detach;

	case POWERDOWN_DETACH:
		cbpp = &pwrdwnhdlrs;
		goto detach;

	case NMI_DETACH:
		cbpp = &nmihdlrs;
detach:
		(void) LOCK_PLMIN(&callback_lock);

		for (; cbp = *cbpp; cbpp = &cbp->cb_next) {

			if (cbp->cb_func == handler && cbp->cb_args == args) {
				*cbpp = cbp->cb_next;
				UNLOCK_PLMIN(&callback_lock, PLBASE);
				kmem_free(cbp, CALLBACK_SIZE);
				return;
			}
		}

		UNLOCK_PLMIN(&callback_lock, PLBASE);
		return;

	default:
		break;

	}; /* end switch */
}

/* TEMPORARY compatibility name */
void
drv_callback_8(int tag, int (*handler)(), void *args)
{
	drv_callback(tag, handler, args);
}

/*
 * void 
 * _Compat_drv_callback_5(int tag, int (*handler)(), void *args)
 *	Register/Unregister event handlers.
 *	Compatibility for ddi versions 5 through 7.
 *
 * Calling/Exit State:
 *	May block, so should called be called with no locks held.
 */
void
_Compat_drv_callback_5(int tag, int (*handler)(), void *args)
{
	switch (tag) {
	case NMI_ATTACH:
	case NMI_DETACH:
		drv_callback(tag, handler, args);
		break;
	};
}


STATIC boolean_t nmi_shutdown;

/*
 * void 
 * _nmi_hook(int *r0ptr)
 *
 * Calling/Exit State:
 *      The arguments are the saved registers which will be restored
 *      on return from this routine.
 *
 * Description:
 *      Currently, NMIs are presented to the processor in these situations:
 *
 *		- [Software NMI] 
 *		- [Access error from access to reserved processor
 *			LOCAL address]
 *		- Access error:
 *			- Bus Timeout
 *			- ECC Uncorrectable error
 *			- Parity error from System Memory
 *			- Assertion of IOCHK# (only expansion board assert this)
 *			- Fail-safe Timer Timeout
 *		- [Cache Parity error (these hold the processor & freeze
 *                                      the bus)]
 */
void
_nmi_hook(int *r0ptr)
{
	int	status;
	uint_t	fcflags;
	struct callback	*nmicb;
	boolean_t recognized = B_FALSE;

	/*
	 * Save the current fault-catch flags, and disable fault-catching
	 * during this routine.  Note that we may get here before it's safe
	 * to access "u.", but then it would be too early for printf's to work,
	 * so it doesn't matter how we panic, and--except for NMI_BENIGN,
	 * which is unlikely to happen during early initialization--we're
	 * going to panic anyway.
	 */
	fcflags = u.u_fault_catch.fc_flags;
	u.u_fault_catch.fc_flags = 0;

	/*
	 * We're playing fast-and-loose with locking in cmn_err calls below.
	 * Disable locktest checks temporarily.
	 */
	++l.disable_locktest;

	for (nmicb = nmihdlrs; nmicb != NULL; nmicb = nmicb->cb_next) {

		/*
		 * Call the registered NMI handler.
		 */
		status = (*nmicb->cb_func)(nmicb->cb_args);

		switch (status) {
		case NMI_BUS_TIMEOUT:
			if ((fcflags & (CATCH_BUS_TIMEOUT|CATCH_ALL_FAULTS))
			     && !was_servicing_interrupt()) {
				u.u_fault_catch.fc_flags = 0;
				u.u_fault_catch.fc_errno = EFAULT;
				r0ptr[T_EIP] = (int)u.u_fault_catch.fc_func;
				--l.disable_locktest;
				return;
			} else {
				/*
				 *+ Fatal Bus Timeout NMI error.
				 */
				cmn_err(CE_PANIC, "Bus Timeout NMI error");
			}
			/* NOTREACHED */

		case NMI_FATAL:
			cmn_err(CE_PANIC, "Fatal NMI Interrupt");
			/* NOTREACHED */

		case NMI_REBOOT:
			nmi_shutdown = B_TRUE;
			/* FALLTHROUGH */

		case NMI_BENIGN:
			recognized = B_TRUE;
			break;

		case NMI_UNKNOWN:
		default:
			break;

		}; /* end case */

	} /* end for */

	if (!recognized) {
		/*
		 *+ A Non-Maskable Interrupt was generated by an 
		 *+ unknown source. 
		 */
		cmn_err(CE_PANIC, "An unknown NMI interrupt occurred.");
		/* NOTREACHED */
	}

	/* Restore fault-catch flags. */
	u.u_fault_catch.fc_flags = fcflags;
	--l.disable_locktest;
}

/*
 * void
 * nmi_poll(void *arg)
 *	Periodic timer for NMI support.
 *
 * Calling/Exit State:
 *	Called as a timeout routine with no locks held.
 */
/* ARGSUSED */
void
nmi_poll(void *arg)
{
	if (nmi_shutdown) {
		drv_shutdown(SD_SOFT, AD_BOOT);
		nmi_shutdown = B_FALSE;
	}
}

/*
 * void
 * _watchdog_hook(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
_watchdog_hook(void)
{
	struct callback	*wdcb;

	for (wdcb = wdhdlrs; wdcb != NULL; wdcb = wdcb->cb_next)
		(void) (*wdcb->cb_func)(wdcb->cb_args);
}


/*
 * void
 * _powerdown_hook(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
_powerdown_hook(void)
{
	struct callback	*pdcb;

	for (pdcb = pwrdwnhdlrs; pdcb != NULL; pdcb = pdcb->cb_next)
		(void) (*pdcb->cb_func)(pdcb->cb_args);
}


/*
 * void
 * ddi_init_f(void)
 *
 * Calling/Exit State:
 *      none
 */
void
ddi_init_f(void)
{
	extern void ddi_init_p(void);

	LOCK_INIT(&callback_lock, CALLBACK_HIER, PLMIN, &callback_lkinfo, 0);
	if (itimeout(nmi_poll, NULL, HZ | TO_PERIODIC, PLTIMEOUT) == 0) {
		/*
		 *+ Insufficient resources to start a timer during startup.
		 */
		cmn_err(CE_PANIC, "ddi_init_f: can't start nmi_poll timer");
		/* NOTREACHED */
	}
	ddi_init_p();
}

asm void mca_sti(void)
{
	sti
}

asm void mca_serialize(void)
{
	pushl %ebx
	xorl %eax,%eax
	cpuid
	popl %ebx
}

STATIC void
mca_report(int putbuf, const char *msg, struct mca_info *mi)
{

	cmn_err(CE_WARN, "!%s: mcg_status=%lx%.8lx (MCIP=%d EIPV=%d RIPV=%d)\n"+putbuf,
		msg, mcg_status[1], mcg_status[0],
		!!(mcg_status[0] & MCG_STATUS_MCIP),
		!!(mcg_status[0] & MCG_STATUS_EIPV),
		!!(mcg_status[0] & MCG_STATUS_RIPV));

	if (mi) {
		cmn_err(CE_CONT, "!bank=%d mci_status=%lx%.8lx (DAM=%d AV=%d MV=%d EN=%d UC=%d O=%d V=%d)\n"+putbuf,
			mi->bank, mci_status[1], mci_status[0],
			!!(mci_status[1] & MCi_STATUS_DAM),
			!!(mci_status[1] & MCi_STATUS_ADDRV),
			!!(mci_status[1] & MCi_STATUS_MISCV),
			!!(mci_status[1] & MCi_STATUS_EN),
			!!(mci_status[1] & MCi_STATUS_UC),
			!!(mci_status[1] & MCi_STATUS_O),
			!!(mci_status[1] & MCi_STATUS_V));

		if (mci_status[0] & MCi_STATUS_ADDRV)
			cmn_err(CE_CONT, "!\tmci_addr=%lx\n"+putbuf,
					mi->mci_addr[0]);
		if (mci_status[0] & MCi_STATUS_MISCV)
			cmn_err(CE_CONT, "!\tmci_misc=%lx\n"+putbuf,
					mi->mci_misc[0]);
	}
}

/*
 * void 
 * _mca_hook(int *r0ptr)
 *
 * Calling/Exit State:
 *
 * Description:
 *
 *  Error reporting policy:
 *	Only exceptions which a handler has determined are MCA_BENIGN should
 *	not be logged.  Fatal/non-restartable exceptions should be logged to
 *	the console.  All other exceptions should be noted in putbuf.
 */

void
_mca_hook(int *r0ptr)
{
	boolean_t restartable = B_TRUE;
	boolean_t recognized = B_FALSE;
	boolean_t valid_bank_examined = B_FALSE;
	struct mca_info mi;
	int status;
	struct callback	*mcacb;
	uint_t	fcflags;

	extern int mca_banks;
	extern boolean_t mcg_ctl_present;

#ifndef P6_BUGS_FIXED
	int count;
#endif

	/*
	 * Save the current fault-catch flags, and disable fault-catching
	 * during this routine.
	 */
	fcflags = u.u_fault_catch.fc_flags;
	u.u_fault_catch.fc_flags = 0;

	/*
	 * We're playing fast-and-loose with locking in cmn_err calls below.
	 * Disable locktest checks temporarily.
	 */
	++l.disable_locktest;

#ifdef MCG_CTL
	if (mcg_ctl_present)  {
		ulong_t mcg_ctl[2] = {0, 0};
		_wrmsr(MCG_CTL, mcg_ctl);
	}
#endif

	_rdmsr(MCG_STATUS, mcg_status);

	if (!(mcg_status[0] & MCG_STATUS_RIPV))
		restartable = B_FALSE;

	for (mi.bank = 0; mi.bank < mca_banks; mi.bank++) {
		_rdmsr(MC0_STATUS + (mi.bank * MCA_REGS), mci_status);

/*
 *  Hardware may queue multiple machine checks through
 *  the reporting banks.  Pull them off in turn invoking any
 *  registered handlers for each.  Report unrecognized machine
 *  checks to putbuf.  Clear MCi_STATUS.O|V and repeat.
 */

#ifndef P6_BUGS_FIXED
		count = 0;
#endif

		while (mci_status[1] & MCi_STATUS_V) {
#ifndef P6_BUGS_FIXED
			if (mi.bank == 1 && mci_status[0] == 0x0115)
				break;
#endif
			valid_bank_examined = B_TRUE;
			mi.flags = 0;

			if ((mci_status[1] & MCi_STATUS_DAM))
				restartable =  B_FALSE;

			if (mci_status[1] & MCi_STATUS_ADDRV) {
				_rdmsr(MC0_ADDR + (mi.bank * MCA_REGS), mi.mci_addr);
				mi.flags |= MCA_INFO_ADDRV;
			}

			if (mci_status[1] & MCi_STATUS_MISCV) {
				_rdmsr(MC0_MISC + (mi.bank * MCA_REGS), mi.mci_misc);
				mi.flags |= MCA_INFO_MISCV;
			}

/*
 *  Handler routines should refer to the globals mcg_status, mci_status,
 *  mci_addr and mci_misc
 */

			for (mcacb = mcahdlrs; mcacb != NULL; mcacb = mcacb->cb_next) {
				status = (*mcacb->cb_func)(mcacb->cb_args, &mi);

				switch (status) {
				case MCA_BUS_TIMEOUT:
					if ((fcflags & (CATCH_BUS_TIMEOUT|CATCH_ALL_FAULTS))
					     && !was_servicing_interrupt()) {
						fcflags = 0;
						u.u_fault_catch.fc_errno = EFAULT;
						r0ptr[T_EIP] = (int)u.u_fault_catch.fc_func;
						recognized = B_TRUE;
					} else {
						mca_report(1, "Machine Check", &mi);
						cmn_err(CE_PANIC, "Fatal Machine Check: Bus Timeout");
					}
					break;

				case MCA_FATAL:
					mca_report(1, "Machine Check", &mi);
					cmn_err(CE_PANIC, "Fatal Machine Check Exception");
					/* NOTREACHED */

				case MCA_REBOOT:
					mca_report(1, "Machine Check Reboot", &mi);
					nmi_shutdown = B_TRUE;
					recognized = B_TRUE;
					break;

				case MCA_BENIGN:
					recognized = B_TRUE;
					break;

				case MCA_UNKNOWN:
				default:
					break;
				}
			}

			if (!recognized)
				mca_report(0, "Unrecognized Machine Check", &mi);

			/* clear overflow, valid bits */
			mci_status[0] = mci_status[1] = 0;
			_wrmsr(MC0_STATUS + (mi.bank * MCA_REGS), mci_status);
			mca_serialize();
			_rdmsr(MC0_STATUS + (mi.bank * MCA_REGS), mci_status);

#ifndef P6_BUGS_FIXED
			if (count++ > 50)    /* bump the loop if STATUS_V isn't being cleared */
				break;
#endif
		}
	}

	if (!restartable) {
		mca_report(1, "Machine Check", NULL);
		cmn_err(CE_PANIC, "Non-restartable Machine Check Exception\n");
	}

	if (!valid_bank_examined)
		mca_report(0, "Machine Check: no valid bank", NULL);

/*
 *  Clear the Machine Check In Progress bit
 */
	_rdmsr(MCG_STATUS, mcg_status);
	mcg_status[0] &= ~MCG_STATUS_MCIP;
	_wrmsr(MCG_STATUS, mcg_status);

#ifdef MCG_CTL
	if (mcg_ctl_present)  {
		ulong_t mcg_ctl[2] = {0xffffffff, 0xffffffff};
		_wrmsr(MCG_CTL, mcg_ctl);
	}
#endif


/*
 *  Restore fault-catch flags.
 *  Preemption and interrupts were disabled in t_mceflt;
 *  turn them back on.
 */

	u.u_fault_catch.fc_flags = fcflags;
mca_return:
	--l.disable_locktest;
	ENABLE_PRMPT();
	mca_sti();
}

