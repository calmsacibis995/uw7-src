#ident	"@(#)kern-i386:svc/fpe.c	1.11.4.1"
#ident	"$Header$"

/*
 * Floating Point Emulator support.
 */

#include <fs/procfs/prsystm.h>
#include <fs/vnode.h>
#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/memresv.h>
#include <mem/page.h>
#include <mem/vmparam.h>
#include <proc/cg.h>
#include <proc/cred.h>
#include <proc/exec.h>
#include <proc/lwp.h>
#include <proc/mman.h>
#include <proc/seg.h>
#include <proc/user.h>
#include <svc/fp.h>
#include <svc/reg.h>
#include <svc/trap.h>
#include <util/cglocal.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

#define EMULPATH "/etc/emulator"

STATIC size_t fpesize;

vaddr_t EM80387;	/* emulator entry point; needed by VPIX trap code */

STATIC int fpeload(void);

/*
 * Opcode values; used by fpeclean.
 */
#define OPC_IRET	0xCF

/*
 * void
 * fpepostroot(void)
 *	Initialize floating point emulator support.
 *
 * Calling/Exit State:
 *	Called from main() after mounting root.
 *	No locking is needed since we are still single-threaded at this point.
 *
 * Description:
 *	Loads floating point emulator if necessary.
 *	If using the emulator on the boot (current) processor, modifies
 *	IDT, GDT and CR0 as needed.
 *
 * Remarks:
 *	This module's 'order' field must be set so this postroot routine
 *	gets called before any other postroot routines which might spawn
 *	LWPs or processes.  This is necessary to ensure that things are
 *	in fact still single-threaded, and that no user process begins to
 *	run before this routine has completed.
 */
void
fpepostroot(void)
{
	/*
	 * If we have floating-point hardware, don't bother to load
	 * the emulator.
	 */
	if (fp_kind & FP_HW)
		return;

	if (fpeload() != -1) {
		/*
		 * If the emulator was successfully loaded, set floating-point
		 * type to "software" and set up the emulator vectors for this
		 * (the boot) processor.
		 */
		fp_kind = FP_SW;
		fpesetvec();
	} else
		fp_kind = FP_NO;
}


/*
 * STATIC int
 * fpeload(void)
 *	Load floating point emulator.
 *
 * Calling/Exit State:
 *	No locking is needed since we are still single-threaded at this point.
 */
STATIC int
fpeload(void)
{
	vnode_t *vp;
	extext_t extext;
	page_t *plist;
	int error;
	int resid;

	/*
	 * Get the vnode for the emulator file.
	 */
	if (lookupname(EMULPATH, UIO_SYSSPACE, FOLLOW, NULLVPP, &vp) != 0)
		goto bad0;

	/*
	 * Open the file (make sure it's a regular file).
	 */
	if (vp->v_type != VREG || VOP_OPEN(&vp, FREAD, CRED()) != 0)
		goto bad1;

	/*
	 * The emulator is composed of a single text section, so all we have
	 * to do is find out the size and location of this one text section.
	 */
	if (exec_gettextinfo(vp, &extext) != 0) {
bad2:
		VOP_CLOSE(vp, FREAD, B_TRUE, 0, CRED());
bad1:
		VN_RELE(vp);
bad0:
		/*
		 *+ The floating point emulator could not be loaded,
		 *+ because the file did not exist, was not a valid emulator
		 *+ file, or was corrupted.  Check the /etc/emulator file
		 *+ and restore it from a backup if necessary.
		 */
		cmn_err(CE_WARN, "Cannot load floating point emulator");
		return -1;
	}
	fpesize = extext.extx_size;

	/*
	 * Allocate memory for the emulator.
	 */
	if (fpesize > FPEMUL_PAGES * MMU_PAGESIZE ||
	    !mem_resv(btopr(fpesize), M_BOTH))
		goto bad2;
	if ((plist = page_get(fpesize, P_NODMA|NOSLEEP, mycg)) == (page_t *)NULL) {
bad3:
		mem_unresv(btopr(fpesize), M_BOTH);
		goto bad2;
	}

	/*
	 * Map emulator memory into KVFPEMUL address.
	 */
	hat_statpt_memload(KVFPEMUL, btopr(fpesize), plist, PROT_ALL);

	/*
	 * Read in the emulator.
	 */
	error = vn_rdwr(UIO_READ, vp, (void *)KVFPEMUL, fpesize,
			extext.extx_offset, UIO_SYSSPACE, 0, (ulong_t)0,
			CRED(), &resid);

	if (error != 0 || resid != 0) {
		hat_statpt_unload(KVFPEMUL, btopr(fpesize));
		page_list_unlock(plist);
		goto bad3;
	}

	/*
	 * Close the file.
	 */
	(void) VOP_CLOSE(vp, FREAD, B_TRUE, 0, CRED());
	VN_RELE(vp);

	EM80387 = extext.extx_entloc;

	return 0;
}


/*
 * void
 * fpesetvec(void)
 *	Initialize vectors and descriptors for emulator support.
 *
 * Calling/Exit State:
 *	Called once for each engine when it comes online.  Thus
 *	serialized by the onoff_mutex.
 */
void
fpesetvec(void)
{
	struct segment_desc *sd;

	ASSERT(fp_kind == FP_SW);
	ASSERT(fpesize != 0);

	/*
	 * Set up GDT entry for FPESEL: the emulator text.
	 */
	sd = &u.u_dt_infop[DT_GDT]->di_table[seltoi(FPESEL)];
	BUILD_MEM_DESC(sd, KVFPEMUL, fpesize, UTEXT_ACC1, TEXT_ACC2_S);

	/*
	 * Set up the IDT NOEXTFLT vector to branch to the emulator in user
	 * mode rather than to the kernel.
	 */
	if (fpuon_noextflt.gd_selector == fpuoff_noextflt.gd_selector) {
		/*
		 * First time; init fpuon_noextflt segment descriptor
		 * for later copying in to the IDT NOEXTFLT vector.
		 */
		BUILD_GATE_DESC(&fpuon_noextflt, FPESEL, EM80387, GATE_386TRP,
				GATE_UACC, 0);
	}

	/*
	 * Set up LDT entry for USER_FPSTK: emulator 32-bit stack alias.
	 * Note that this is identical to USER_DS, except it doesn't have to
	 * include any address in the kernel range.
	 */
	sd = &u.u_dt_infop[DT_LDT]->di_table[seltoi(USER_FPSTK)];
	BUILD_MEM_DESC(sd, UVBASE, mmu_btop(uvend - UVBASE),
		       UDATA_ACC1, DATA_ACC2);

	/*
	 * Set up the segment descriptor for the USER_FP segment,
	 * used for emulator access to the floating-point save area
	 * in the uarea.
	 */
	sd = &u.u_dt_infop[DT_LDT]->di_table[seltoi(USER_FP)];
	BUILD_MEM_DESC(sd, &l.fpe_kstate, sizeof(l.fpe_kstate),
		       UDATA_ACC1, DATA_ACC2_S);
}


/*
 * boolean_t
 * fpeclean(void)
 *	Remove the fp emulator stack frame from the user stack in preparation
 *	for delivery of an fp-related signal.
 *
 * Calling/Exit State:
 *	This routine may block, so no spinlocks may be held on entry.
 *
 *	At the point when fpeclean is invoked, the user
 *	stack looks like this:
 *	
 *		old esp ->
 *				efl
 *				cs
 *				eip
 *				gs
 *				fs
 *				es
 *				ds
 *				<8 general registers>
 *				.
 *				.
 *		current esp ->	.
 *	
 *	"old" means roughly "when the fp instruction was invoked."
 *	"current" means "in u.u_ar0[]"
 */
boolean_t
fpeclean(void)
{
	struct emul_frame {
		ulong_t di;
		ulong_t si;
		ulong_t bp;
		ulong_t unused;
		ulong_t bx;
		ulong_t dx;
		ulong_t cx;
		ulong_t ax;
		ulong_t ds;
		ulong_t es;
		ulong_t fs;
		ulong_t gs;
		ulong_t eip;
		ulong_t cs;
		ulong_t efl;
	} emul_frame;
	gregset_t gregs;
	vaddr_t esp, eip, base;
	struct segment_desc *ssdp;
	uint_t n;

	ASSERT(fp_kind == FP_SW);
	ASSERT(KS_HOLD0LOCKS());

	/* get user stack pointer */
	esp = u.u_ar0[T_UESP];

	if (u.u_dt_infop[DT_LDT] != &myglobal_dt_info[DT_LDT]) {
		/*
		 * Private LDT active; segments may not be standard;
		 * convert %ss:%esp to a linear address.
		 */
		struct segment_desc *ssdp =
			&u.u_dt_infop[DT_LDT]->di_table[seltoi(u.u_ar0[T_SS])];
		if (!(SD_GET_ACC2(ssdp) & BIGSTACK)) {
			/*
			 * SS is a 16-bit stack segment; convert the base value
			 * to a 32 bit value.
			 */
			esp &= 0xFFFF;
		}
		esp += SD_GET_BASE(ssdp);
	}

	/*
	 * If the base stack pointer hasn't been saved yet (or it's been
	 * restored), we have to deduce its value.
	 */
	if (l.fpe_kstate.fpe_restart.fr_esp == 0) {
		ASSERT(l.fpe_kstate.fpe_restart.fr_eip == 0);

		/* get user instruction pointer (always 32-bit) */
		eip = u.u_ar0[T_EIP];

		/*
		 * If we're at the first or last instruction, nothing else
		 * has been pushed on the stack besides the iret frame;
		 * otherwise, the emulator has pushed one additional longword,
		 * which is the GS register.
		 */
		if (eip == EM80387 || *(uchar_t *)(KVFPEMUL + eip) == OPC_IRET)
			base = esp;
		else
			base = esp + sizeof(int);
	} else
		base = l.fpe_kstate.fpe_restart.fr_esp + sizeof(int);
	base += 3 * sizeof(int);	/* for EIP, CS, EFL */

	/* copy registers from user stack */

	n = base - esp;
	if (n > sizeof emul_frame) {
		n = sizeof emul_frame;
		esp = base - n;
	}
	if (ucopyin((void *)esp, (char *)(&emul_frame + 1) - n, n, 0) != 0)
		return B_FALSE;

	/* and fill this info into the frame for the current trap */

	prgetregs(&u, gregs);

	switch (n) {
	case 60:
		gregs[R_EDI] = emul_frame.di;
		/* FALLTHROUGH */
	case 56:
		gregs[R_ESI] = emul_frame.si;
		/* FALLTHROUGH */
	case 52:
		gregs[R_EBP] = emul_frame.bp;
		/* FALLTHROUGH */
	case 48:
		/* unused */
		/* FALLTHROUGH */
	case 44:
		gregs[R_EBX] = emul_frame.bx;
		/* FALLTHROUGH */
	case 40:
		gregs[R_EDX] = emul_frame.dx;
		/* FALLTHROUGH */
	case 36:
		gregs[R_ECX] = emul_frame.cx;
		/* FALLTHROUGH */
	case 32:
		gregs[R_EAX] = emul_frame.ax;
		/* FALLTHROUGH */
	case 28:
		gregs[R_DS] = emul_frame.ds;
		/* FALLTHROUGH */
	case 24:
		gregs[R_ES] = emul_frame.es;
		/* FALLTHROUGH */
	case 20:
		gregs[R_FS] = emul_frame.fs;
		/* FALLTHROUGH */
	case 16:
		gregs[R_GS] = emul_frame.gs;
		/* FALLTHROUGH */
	case 12:
		gregs[R_EIP] = emul_frame.eip;
		gregs[R_CS] = emul_frame.cs;
		gregs[R_EFL] = emul_frame.efl;
	}

	/*
	 * If the emulator has only partially adjusted the user's state
	 * (emulated FPU state, saved EIP), backout to the initial state
	 * snapshotted by the emulator in l.fpe_kstate.fpe_restart.
	 */
	if (l.fpe_kstate.fpe_restart.fr_eip != 0) {
		bcopy(&l.fpe_kstate.fpe_restart.fr_fpestate,
		      &l.fpe_kstate.fpe_state,
		      sizeof l.fpe_kstate.fpe_state);
		gregs[R_EIP] = l.fpe_kstate.fpe_restart.fr_eip;
		l.fpe_kstate.fpe_restart.fr_eip = 0;
	}

	/* remove the emulator frame from the user stack */
	gregs[R_ESP] = base;

	(void)prsetregs(&u, gregs);

	l.fpe_kstate.fpe_restart.fr_esp = 0;

	return B_TRUE;
}
