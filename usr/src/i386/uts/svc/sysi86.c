#ident	"@(#)kern-i386:svc/sysi86.c	1.26.6.2"
#ident	"$Header$"

/*
 * sysi86 System Calls
 */

#include <acc/priv/privilege.h>
#include <io/conssw.h>
#include <io/ioctl.h>
#include <mem/kmem.h>
#include <mem/vmparam.h>
#include <proc/cred.h>
#include <proc/exec.h>
#include <proc/seg.h>
#include <proc/tss.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/memory.h>
#include <svc/reg.h>
#include <svc/sysi86.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <svc/utsname.h>
/* Enhanced Application Compatibility Support */
#include <svc/scofeatures.h>
/* End Enhanced Application Compatibility Support */
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/kdb/xdebug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

extern size_t totalmem;			/* size of memory in pages */

extern int sysi86_p(struct sysi86a *, rval_t *);

#ifndef NO_ADDR0
STATIC void addr0_set(boolean_t);
#endif
boolean_t addr0_enabled(void);


STATIC int si86_call_demon(void);
STATIC int si86_setdscr(void *uargp);
STATIC int si86_iopl(uint_t);

/* Enhanced Application Compatibility Support */
/* OS Features Vector showing what OS specific features are supported */
static char features_vector[] = {
	FEATURE_SIGCHLD_VAL,
	FEATURE_MMAP_VAL,
	FEATURE_VNODE_VAL,
	FEATURE_WRITEV_VAL,
	FEATURE_SID_VAL,
	FEATURE_CANPUT_VAL,
	FEATURE_PROCSET_VAL,
	FEATURE_TCGETS_VAL,
	FEATURE_TERMIO_VAL,
	FEATURE_FSYNC_VAL,
	FEATURE_GTIMEOFDAY_VAL,
	FEATURE_INODE32_VAL
};
/* End Enhanced Application Compatibility Support */


/*
 * int
 * sysi86(struct sysi86a *, rval_t *)
 *	i386 family specific system call
 *
 * Calling/Exit State:
 *	None.
 */
int
sysi86(struct sysi86a *uap, rval_t *rvp)
{
	int	error = 0;

	switch (uap->cmd) {

	case SI86MEM:	/* return the size of memory */
		rvp->r_val1 = totalmem;
		break;

	/*
	 * Tell a user what kind of Floating Point support we have.
	 * fp_kind (defined in fp.h) is returned in the low-order byte.
	 * If Weitek support is included, weitek_kind (defined in weitek.h)
	 * is returned in the second byte.
	 */
	case SI86FPHW:
		if (suword(uap->arg.iparg, uvwin.uv_fp_hw) == -1)
			error = EFAULT;
		break;

	case SI86TODEMON:	/* Transfer to kernel debugger. */
		if (pm_denied(CRED(), P_SYSOPS))
			error = EPERM;
		else if (!si86_call_demon())
			error = EINVAL;
		break;

	/*
	 * Set a segment descriptor
	 */
	case SI86DSCR:
		error = si86_setdscr(uap->arg.sparg);
		break;

	/* XENIX Support */
	case SI86SHRGN:
	{

		extern	int	xsd86shrgn();
		/*
		 * Enable/disable XENIX small model shared data context    
		 * switching support.  The 'cparg' argument is a pointer   
		 * to an xsdbuf struct which contains the 386 start addr
		 * for the sd seg and the 286 start addr for the sd seg.
		 * When a proc that has requested shared data copying (via
		 * SI86BADVISE) is switched to, the kernel copies the sd
		 * segment from the 386 address to the 286 address.
		 * When the proc is switched from, the kernel copies the
		 * sd segment from the 286 address to the 386 address.
		 * Note that if the 286 addr is NULL, the shared data
		 * segment's context switching support is disabled.
		 */

		error = xsd86shrgn(uap->arg.cparg);
		break;
	}

	case SI86BADVISE:
		/*
	 	 * Specify XENIX variant behavior.
	 	 */
		switch (uap->arg.iarg & 0xFF00) {
		case SI86B_GET:
			/* Return badvise bits */
			if (BADVISE_PRE_SV)
				rvp->r_val1 |= SI86B_PRE_SV;
			if (BADVISE_XOUT)
				rvp->r_val1 |= SI86B_XOUT;
			if (BADVISE_XSDSWTCH)
				rvp->r_val1 |= SI86B_XSDSWTCH;
			break;
		case SI86B_SET:
			/* Set badvise bits.
			 * We assume that if pre-System V behavior
			 * is specified, then the x.out behavior is
			 * implied (i.e., the caller gets the same
			 * behavior by specifying SI86B_PRE_SV alone
			 * as they get by specifying both SI86B_PRE_SV
			 * and SI86B_XOUT).
			 */ 
			if (uap->arg.iarg & SI86B_PRE_SV) 
				RENV |= (UB_PRE_SV | UB_XOUT);
			else if (uap->arg.iarg & SI86B_XOUT) {
				RENV |= UB_XOUT;
				RENV &= ~UB_PRE_SV;
			} else
				RENV &= ~(UB_PRE_SV | UB_XOUT);	
			if (uap->arg.iarg & SI86B_XSDSWTCH) {
				/* copy the "real" sd to the 286 copy */
				xsdswtch(1);
				RENV |= UB_XSDSWTCH;
			} else {
				/* copy the 286 sd to the "real" copy */
				xsdswtch(0);
				RENV &= ~UB_XSDSWTCH;
			}
			break;
		default:
			error = EINVAL;
			break;
		}
		break;

	case SI86SHFIL:
		error = si86_mapfile(uap->arg.sparg, rvp);
		break;

	case SI86PCHRGN:
		error = si86_chmfile(uap->arg.sparg, rvp);
		break;

	/* End XENIX Support */

	/* Remove emulator special read access */
	case SI86EMULRDA:
		RENV &= ~RE_EMUL;
		break;

	case SI86GCON:		/* Get console device major/minor */
		if (copyout(&conschan.cnc_dev, uap->arg.lparg, sizeof(dev_t)))
			error = EFAULT;
		break;

	case SI86NULLPTR:	/* Enable/disable/query null-pointer setting */
		switch (uap->arg.iarg) {
		case 0: /* disable */
#ifndef NO_ADDR0
			/* FALLTHROUGH */
		case 1: /* enable */
			addr0_set(uap->arg.iarg);
#endif
			break;
		case 2: /* query */
			rvp->r_val1 = addr0_enabled();
			break;
		default:
			error = EINVAL;
		}
		break;

	case SI86IOPL:		/* Change I/O privilege level */
		error = si86_iopl((uint_t)uap->arg.iarg);
		break;

/*
 * For backward binary compatibility for applications such as AutoCAD
 * The uap->arg2 is the psw that contains the iopl, must extract iopl from it.
 */
#define	SI86V86		71
#define V86SC_IOPL	4

	case SI86V86:
		if (uap->arg.larg == V86SC_IOPL)
			error = si86_iopl(((flags_t *)&uap->arg2)->fl_iopl);
		else
			error = EINVAL;
		break;
/*
 * End of: backward binary compatibility for applications such as AutoCAD
 */


/* Enhanced Application Compatibility Support */

	/* 
	 * this OSR5 compat function tells the OSR5 libc that various
	 * features are in the kernel. For compat purposes we have some
	 * and don't have others. (Note: code lifted from OSR5)
	 */
	case SI86GETFEATURES :
	{
		register int    bytecnt;

		/* If we are here, then the binary is OpenServer, so */
		/* force the flag on, in user space.		     */
		RENV2 |= RE_ISSCO;

		/* arg2 is number of bytes to copy */
		if((int)uap->arg2 <= 0 ) {
			error = EINVAL;
			break;
		}

		/* must have min between argument size */
		/* and number of OS features supported */
		bytecnt = min((int)uap->arg2, sizeof(features_vector));

		/* arg.cparg is ptr to char array argument */
		if (copyout((caddr_t)features_vector, uap->arg.cparg, bytecnt) == -1) { 
			error = EFAULT;
			break;
		}

		/* return number OS features indicated */
		rvp->r_val1 = bytecnt;
		break;
	}

/* End Enhanced Application Compatibility Support */

	default:
		error = sysi86_p(uap, rvp);
		break;
	}
	return(error);
}

/*
 * int
 * si86_iopl(uint_t)
 *	i386 family specific system call - change I/O privilege level
 *
 * Calling/Exit State:
 *	None.
 */
int
si86_iopl(uint_t arg)
{
	/* Valid IOPLs are 0-3 */
	if (arg > 3)
		return EINVAL;
	/* Increasing I/O privilege requires P_SYSOPS privilege */
	if (arg > ((flags_t *)&u.u_ar0[T_EFL])->fl_iopl
	    && pm_denied(CRED(), P_SYSOPS))
		return EPERM;
	((flags_t *)&u.u_ar0[T_EFL])->fl_iopl = arg;

	return 0;
}

#ifndef NO_ADDR0

extern boolean_t nullptr_default;

STATIC struct addr0_user {
	uid_t		au_uid;
	boolean_t	au_enabled;
	struct addr0_user *au_next;
} *au_list;

STATIC fspin_t au_list_mutex;

/*
 * STATIC struct addr0_user *
 * addr0_lookup(uid_t uid)
 *	Lookup a uid in the addr0_user table.
 *
 * Calling/Exit State:
 *	Called with the au_list_mutex held, and returns the same.
 */
STATIC struct addr0_user *
addr0_lookup(uid_t uid)
{
	struct addr0_user *aup;

	for (aup = au_list; aup != NULL; aup = aup->au_next) {
		if (aup->au_uid == uid)
			break;
	}
	return aup;
}

/*
 * STATIC void
 * addr0_set(boolean_t enable)
 *	Enable/Disable null pointers for the current uid.
 *
 * Calling/Exit State:
 *	This routine may block, so no spin locks may be held on entry,
 *	and none will be held at exit.
 */
STATIC void
addr0_set(boolean_t enable)
{
	struct addr0_user *aup;
	struct addr0_user *naup = NULL;
	uid_t uid = CRED()->cr_uid;

	ASSERT(KS_HOLD0LOCKS());

	FSPIN_LOCK(&au_list_mutex);
	aup = addr0_lookup(uid);
	if (aup == NULL) {
		FSPIN_UNLOCK(&au_list_mutex);
		naup = kmem_alloc(sizeof(struct addr0_user), KM_SLEEP);
		FSPIN_LOCK(&au_list_mutex);
		aup = addr0_lookup(uid);
		if (aup == NULL) {
			aup = naup;
			aup->au_uid = uid;
			aup->au_next = au_list;
			au_list = aup;
			naup = NULL;
		}
	}
	aup->au_enabled = enable;
	FSPIN_UNLOCK(&au_list_mutex);
	if (naup != NULL)
		kmem_free(naup, sizeof(struct addr0_user));
}

#endif /* !NO_ADDR0 */

/*
 * boolean_t
 * addr0_enabled(void)
 *	Check if null pointer workaround enabled for current uid.
 *
 * Calling/Exit State:
 */
boolean_t
addr0_enabled(void)
{
#ifndef NO_ADDR0
	struct addr0_user *aup;
	uid_t uid = CRED()->cr_uid;
	boolean_t enabled;

	FSPIN_LOCK(&au_list_mutex);
	aup = addr0_lookup(uid);
	enabled = ((aup != NULL) ? aup->au_enabled : nullptr_default);
	FSPIN_UNLOCK(&au_list_mutex);
	return enabled;
#else
	return B_FALSE;
#endif
}


/*
 * STATIC int
 * si86_call_demon(void)
 *	Invoke a kernel debugger from a privileged system call.
 *
 * Calling/Exit State:
 */
STATIC int
si86_call_demon(void)
{
#ifndef	NODEBUGGER
	extern int demon_call_type;

	if (cdebugger != nullsys) {
		/*
		 * Set a flag and generate a trap into the debugger.
		 * This is done, rather than calling the debugger directly,
		 * to get a trap frame saved.
		 */
		demon_call_type = DR_SECURE_USER;
		asm(" int $3");	/* Force a debug trap */
		return 1;
	}
#endif
	return 0;
}



/* Do two selectors select the same descriptor? */
#define SAMEDSCR(sel1, sel2) ((((sel1)^(sel2))&~RPL_MASK)==0)

/*
 * STATIC int
 * si86_setdscr(void *uargp)
 *	Handle user request to change a segment descriptor.
 *
 * Calling/Exit State:
 *	uargp is a user-mode pointer to a struct ssd request structure.
 */
STATIC int
si86_setdscr(void *uargp)
{
	struct ssd ssd;		/* user's request structure */
	vaddr_t bottom, top;
	ulong_t limit;
	union { struct segment_desc seg; struct gate_desc gate; } dscr;
	greg_t *uregs;

	if (copyin(uargp, &ssd, sizeof ssd) < 0)
		return EFAULT;

	/*
	 * Don't allow changing the privilege level of a descriptor
	 * currently selected by a segment register.
	 */
	uregs = (greg_t *)u.u_ar0;
	if ((SAMEDSCR(ssd.sel, uregs[T_CS]) &&
	     ((ssd.acc1 & SEG_DPL) >> 5 != (uregs[T_CS] & RPL_MASK) ||
	      !(ssd.acc1 & SEG_CODE))) ||
	    (SAMEDSCR(ssd.sel, uregs[T_SS]) &&
	     ((ssd.acc1 & SEG_DPL) >> 5 != (uregs[T_SS] & RPL_MASK) ||
	      (ssd.acc1 & SEG_CODE))) ||
	    (SAMEDSCR(ssd.sel, uregs[T_DS]) &&
	     ((ssd.acc1 & SEG_DPL) >> 5 != (uregs[T_DS] & RPL_MASK) ||
	      (ssd.acc1 & SEG_CODE))) ||
	    (SAMEDSCR(ssd.sel, uregs[T_ES]) &&
	     ((ssd.acc1 & SEG_DPL) >> 5 != (uregs[T_ES] & RPL_MASK) ||
	      (ssd.acc1 & SEG_CODE))) ||
	    (SAMEDSCR(ssd.sel, _fs()) &&
	     ((ssd.acc1 & SEG_DPL) >> 5 != (_fs() & RPL_MASK) ||
	      (ssd.acc1 & SEG_CODE))) ||
	    (SAMEDSCR(ssd.sel, _gs()) &&
	     ((ssd.acc1 & SEG_DPL) >> 5 != (_gs() & RPL_MASK) ||
	      (ssd.acc1 & SEG_CODE))))
		return EINVAL;

	/*
	 * Validate requested descriptor:  must not have kernel
	 * privilege, must be a data or non-conforming code segment or
	 * gate descriptor, must cover only normal user addresses,
	 * must not replace any kernel private segments, must be
	 * present.
	 */
	if (ssd.acc1 == 0) {
		/* special case: clear descriptor */
		struct_zero(&dscr, sizeof dscr);
	} else {
		if (!(ssd.acc1 & SEG_PRESENT))
			return EINVAL;
		/* Ring 0 never permitted, 1 and 2 only with privilege. */
		if ((ssd.acc1 & SEG_DPL) != 0x60) { /* 0x60 == ring 3 */
			if ((ssd.acc1 & SEG_DPL) == 0 ||
			    pm_denied(CRED(), P_SYSOPS))
				return EPERM;
		}
		/* Don't mess with kernel's GDT segments */
		if (!(ssd.sel & SEL_LDT) &&
		    ssd.sel >= KERNSEG_MIN && ssd.sel < KERNSEG_MAX)
			return EINVAL;
		/* Clip limit and access bytes to valid ranges. */
		ssd.ls &= 0xFFFFF;
		ssd.acc1 &= 0xFF;
		/*
		 * Validate the segment type.
		 * Based on the segment type, compute the upper and lower
		 * linear addresses, inclusive, covered by the segment.
		 * Build the appropriate descriptor in `dscr'.
		 */
		switch (ssd.acc1 & 0x1C) {
		case 0x10:	/* expand-up data segment */
		case 0x18:	/* non-conforming code segment */
			ssd.acc2 &= 0x0F;
			limit = ((ssd.acc2 & GRANBIT) ?
				 ((ssd.ls << 12) | 0xFFF) :
				 ssd.ls);
			if (SAMEDSCR(ssd.sel, uregs[T_CS]) &&
			    uregs[T_EIP] > limit)
				return EINVAL;
			bottom = ssd.bo;
			top = ssd.bo + limit;
			if (bottom > top || top >= uvend)
				return EINVAL;
			BUILD_MEM_DESC(&dscr.seg, ssd.bo, ssd.ls + 1,
				       ssd.acc1, ssd.acc2);
			break;
		case 0x14:	/* expand-down data segment */
			ssd.acc2 &= 0x0F;
			if ((ssd.acc2 & GRANBIT) != (ssd.acc2 & BIGSTACK))
				return EINVAL;
			bottom = ssd.bo + ((ssd.acc2 & GRANBIT) ?
						((ssd.ls << 12) | 0xFFF) :
						ssd.ls) + 1;
			top = ssd.bo + ((ssd.acc2 & GRANBIT) ?
						0xFFFFFFFF : 0xFFFF);
			if (bottom > top || top >= uvend)
				return EINVAL;
			BUILD_MEM_DESC(&dscr.seg, ssd.bo, ssd.ls + 1,
				       ssd.acc1, ssd.acc2);
			break;
		case 0x04:	/* call gate */
		case 0x0C:	/* call gate */
			ssd.acc2 &= 0x1F;
			if ((ssd.acc1 & 3) != 0)
				return EINVAL;
			BUILD_GATE_DESC(&dscr.gate, ssd.ls, ssd.bo, 0,
					ssd.acc1, ssd.acc2);
			break;
		default:
			return EINVAL;
		}
	}

	return set_dt_entry(ssd.sel, &dscr.seg);
}
