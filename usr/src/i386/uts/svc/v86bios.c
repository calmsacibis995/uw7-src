#ident	"@(#)kern-i386:svc/v86bios.c	1.1.7.5"
#ident  "$Header$"

/*
 * 16-bit BIOS call in V8086 mode.
 *
 * A 16-bit BIOS call in V8086 mode runs in user-mode with paging enabled.
 * The well-known data/code required for BIOS calls are mapped in
 * mmu.c in advance or restablished by remap_pagetable(). 
 *
 * Limitation:
 * The current v86bios has basically two limitations, but in general they
 * don't matter for servicing BIOS calls needed for the kernel/X-Window.
 * One of the limitations is the size: data + stack <= 0x800. The
 * stack grows down from  V86BIOS_PSTACK_BASE, and the data grows up to
 * it. This is supposed to be okay, even for PCI BIOS calls accoring to the
 * spec.
 * Another limitation is protection/emulation. The OS needs to protect system
 * ports for, e.g. PIC, PIT, etc. to avoide conflicts with the OS code. In
 * this implementation, they are just skipped instead of being emulated. This
 * seems to be okay especially for video bios calls.
 */

#include <util/inline.h>
#include <util/plocal.h>
#include <util/engine.h>
#include <util/emask.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <util/param.h>
#include <util/debug.h>
#include <proc/regset.h>
#include <proc/tss.h>
#include <proc/seg.h>
#include <io/gvid/vdc.h>
#include <io/autoconf/ca/eisa/nvm.h>
#include <svc/v86bios.h>

#define	_PSM	2
#include <psm/toolkits/psm_i8254/psm_i8254.h>
#include <psm/toolkits/psm_i8259/psm_i8259.h>
#undef _PSM

static int _PSM_2 = 1;	/* for resolving the undefined symbol */

ullong_t remap_pagetable(vaddr_t, ullong_t);
void v86call(void);
void v86bios_setprot(void);

STATIC pl_t 		v86bios_lockpl;		/* pl for v86bios_lock */
STATIC lock_t		v86bios_lock;		/* mutex v86bios */

LKINFO_DECL(v86bios_lockinfo, "MP:v86bios:v86bios_lock", 0); /* lockinfo */
#define V86BIOS_LOCK() (v86bios_lockpl = LOCK(&v86bios_lock, PLHI))
#define V86BIOS_UNLOCK() (UNLOCK(&v86bios_lock,v86bios_lockpl))

extern struct tss386 *v86_tss;	/* defined in sysinit.c */
int v86bios_error;

STATIC uchar_t *v86mon_stack;
#ifndef UNIPROC
STATIC intregs_t *v86bios_regs;		/* for xcall */
#endif

/*
 * void
 * v86bios_init()
 *	Called by sysinit() to initialize data structures for v86bios.
 */
void
v86bios_init()
{
	LOCK_INIT(&v86bios_lock, 50, PLHI,
		  &v86bios_lockinfo, KM_NOSLEEP);
	
	v86mon_stack = os_alloc(V86MON_STACKSIZE);

#ifndef UNIPROC
	v86bios_regs = os_alloc(sizeof(intregs_t));
#endif	
	
}


STATIC void
v86bios0(intregs_t 	*regs)
{
	ushort_t 	*v86_stack, cs, off;	/* stack for the V86 program */
	ushort_t	leave_addr;
	pl_t		pl;
	flags_t 	*eflags;
	ullong_t	old_pte;
	extern ullong_t	save_kl1pte0;
	unsigned int	func_type;	/* how to call */
	v86_farptr_t 	*ent;
	v86_pl0stack_t	*pl0stack;
	uchar_t		gd_type_save; /* for saving the priv */

	func_type = regs->type & V86BIOS_FUNCTYPE; /* flag to determine BIOS
	or ROM call */

	v86bios_error = 0;

	/*
	 * To make int 0x10 cause GP#0, change the dpl to GATE_KACC.
	 */
	gd_type_save = cur_idtp[EXTERRFLT].gd_type_etc;
	cur_idtp[EXTERRFLT].gd_type_etc = GATE_386TRP | GATE_KACC;
	
	old_pte = remap_pagetable(0, save_kl1pte0);

	if (regs->type & V86BIOS_COPYOUT) {
		ASSERT(regs->buf_out->kaddr != NULL);
		bcopy(regs->buf_out->kaddr, regs->buf_out->v86addr,
		      regs->buf_out->size);
	}
	
	ent = (struct v86_farptr *) (uint_t) V86BIOS_PBASE;

	if (func_type == V86BIOS_CALL) {
		cs = regs->entry.farptr.v86_cs;
		off = regs->entry.farptr.v86_off;
	} else {
		cs = (ent + regs->entry.intval)->v86_cs;
		off = (ent + regs->entry.intval)->v86_off;
	}

	v86_stack = (ushort_t *) V86BIOS_PSTACK_BASE;
	*--v86_stack = *(ushort_t *)&v86bios_leave;
	leave_addr = (ushort_t) v86_stack;
	if (func_type == V86BIOS_INT16 || func_type == V86BIOS_INT32)
		*--v86_stack = (PS_IOPL | PS_VM); 	/* flags */
	*--v86_stack = 0; 				/* cs */
	*--v86_stack = leave_addr;			/* return address */

	v86_tss->t_esp = (ushort_t) v86_stack;
	v86_tss->t_eip = off;
	v86_tss->t_ss0 = KDSSEL;
	v86_tss->t_esp0 = (uint_t) (v86mon_stack + V86MON_STACKSIZE
				     - sizeof(uint_t));
	v86_tss->t_eflags = PS_IOPL | PS_VM;
	v86_tss->t_cr3 = _cr3();

	if (func_type == V86BIOS_INT32) {
		v86_tss->t_eax = regs->eax.eax;
		v86_tss->t_ecx = regs->ecx.ecx;
		v86_tss->t_edx = regs->edx.edx;
		v86_tss->t_ebx = regs->ebx.ebx;
	} else {
		v86_tss->t_eax = (uint_t) regs->eax.word.ax;
		v86_tss->t_ecx = (uint_t) regs->ecx.word.cx;
		v86_tss->t_edx = (uint_t) regs->edx.word.dx;
		v86_tss->t_ebx = (uint_t) regs->ebx.word.bx;
	}

	v86_tss->t_ebp = (uint_t) regs->bp;

	if (func_type == V86BIOS_INT32) {
		v86_tss->t_esi = regs->esi.esi;
		v86_tss->t_edi = regs->edi.edi;
	} else {
		v86_tss->t_esi = (uint_t) regs->esi.word.si;
		v86_tss->t_edi = (uint_t) regs->edi.word.di;
	}

	v86_tss->t_es = regs->es;
	v86_tss->t_cs = cs;
	v86_tss->t_ss = 0;
	v86_tss->t_ds = regs->ds;
	v86_tss->t_fs = 0;
	v86_tss->t_gs = 0;
	v86_tss->t_tbit = 0;
	v86_tss->t_bitmapbase = sizeof(struct tss386);	/* contiguous */

	v86bios_setprot();

	v86call();

	if (v86bios_error)
		goto v86bios_exit;

	if (regs->type & V86BIOS_COPYIN) {
		ASSERT(regs->buf_in->kaddr != NULL);
		bcopy(regs->buf_in->v86addr, regs->buf_in->kaddr,
		      regs->buf_in->size);	
	}

	/*
	 * Copyback to the kernel so that it can get the updates.
	 * Some BIOS calls (e.g. Get PCI Interrupt Routing Options),
	 * overwrite the input. 
	 */
	if (regs->type & V86BIOS_COPYOUT) {
		ASSERT(regs->buf_out->kaddr != NULL);
		bcopy(regs->buf_out->v86addr, regs->buf_out->kaddr,
		      regs->buf_out->size);	
	}

	if (func_type == V86BIOS_INT32) {
		regs->eax.eax = v86_tss->t_eax;
		regs->ecx.ecx = v86_tss->t_ecx;
		regs->edx.edx = v86_tss->t_edx;
		regs->ebx.ebx = v86_tss->t_ebx;
	} else {
		regs->eax.word.ax = (ushort_t) v86_tss->t_eax;
		regs->ecx.word.cx = (ushort_t) v86_tss->t_ecx;
		regs->edx.word.dx = (ushort_t) v86_tss->t_edx;
		regs->ebx.word.bx = (ushort_t) v86_tss->t_ebx;
	}

	regs->bp = (ushort_t) v86_tss->t_ebp;

	/* The segment registers %gs, %fs, %ds, and %es are cleared to
	 * 0 after intN, which is used to return back to the kernel
	 * mode. We need to pick them up in the stack frame.
	 */
	pl0stack = (v86_pl0stack_t *) v86_tss->t_esp;

	regs->es = pl0stack->v_es;
	regs->ds = pl0stack->v_ds;

	if (func_type == V86BIOS_INT32) {
		regs->esi.esi = (ushort_t) v86_tss->t_esi;
		regs->edi.edi = (ushort_t) v86_tss->t_edi;
	} else {
		regs->esi.word.si = (ushort_t) v86_tss->t_esi;
		regs->edi.word.di = (ushort_t) v86_tss->t_edi;
	}

	eflags = (flags_t *) &pl0stack->v_eflags;
	regs->retval = eflags->fl_cf;

v86bios_exit:
	remap_pagetable(0, old_pte);
	cur_idtp[EXTERRFLT].gd_type_etc = gd_type_save;
	regs->error = v86bios_error;
	regs->done = (volatile) B_TRUE;	
}

/*
 * void
 * v86bios()
 * 	Call BIOS/ROM call using V8086 mode. The ROM call must end with
 * 	'lret' (far return). 
 *
 * Calling/Exit State:
 *	The registers in regs are set by the caller, and are reset upon
 *	return by the BIOS/ROM routine. Since V86 program performs ljmp when
 *	returning, the context is saved in the TSS (v86_tss).
 *
 *	As the 32-bit operand prefix allows a real-mode program to use the
 *	32-bit general-purpose registers (EAX, EBX, etc.), the v86bios can
 *	pass 32-bit values for the registers. The type in the regs is used
 *	for that end (V86BIOS_INT32). PCI bios calls, for example, need this
 *	type set.
 *	
 *	The function is supposed to be available at any time inside/after
 *	initpsm(). 
 *
 */
void
v86bios(intregs_t *regs)
{
	
	V86BIOS_LOCK();

#ifndef UNIPROC
	if (myengnum != BOOTENG) {
		emask_t	target;

		bcopy(regs, v86bios_regs, sizeof(intregs_t));
		v86bios_regs->done = (volatile) B_FALSE;
		EMASK_INIT(&target, BOOTENG);
		/*
		 * xcall cannot look at the stack of another CPU,
		 * copy regs to the global space.
		 */
		xcall (&target, NULL, v86bios0, v86bios_regs);
		while (!v86bios_regs->done)
			;
		bcopy(v86bios_regs, regs, sizeof(intregs_t));
	} else
		v86bios0(regs);
#else
	v86bios0(regs);
#endif	/* UNIPROC */

	V86BIOS_UNLOCK();
}

/*
 * int
 * v86bios_pci_bios_call()
 * 	Call 16-bit BIOS call using V8086 mode.
 *
 *	It returns the value set in the %ah register, containing error
 *	code, if any.
 *
 *	Called from PCI routines.
 */
int
v86bios_pci_bios_call(regs *pci_reg)
{
	intregs_t	regs;

	bzero((caddr_t) &regs, sizeof(intregs_t));
	
	regs.type = V86BIOS_INT32; /* need 32-bit registers */
	regs.entry.intval = PCI_BIOS_INT;
	regs.eax.eax = pci_reg->eax.eax;
	regs.ecx.ecx = pci_reg->ecx.ecx;
	regs.edx.edx = pci_reg->edx.edx;
	regs.ebx.ebx = pci_reg->ebx.ebx;
	regs.bp = 0;
	regs.esi.esi = pci_reg->esi.esi;
	regs.edi.edi = pci_reg->edi.edi;
	regs.ds = 0xf000;	/* part of PCI spec */
	regs.es = 0;		/* intentionally 0 to pass RouteBuffer, e.g. */

	v86bios(&regs);

	pci_reg->eax.eax = regs.eax.eax;
	pci_reg->ecx.ecx = regs.ecx.ecx;
	pci_reg->edx.edx = regs.edx.edx;
	pci_reg->ebx.ebx = regs.ebx.ebx;
	pci_reg->esi.esi = regs.esi.esi;
	pci_reg->edi.edi = regs.edi.edi;

	return regs.eax.byte.ah;
}

/*
 * int
 * v86bios_pci_bios_call_usebuf(regs *pci_reg,
 *	v86bios_buf_t *buf_out, v86bios_buf_t *buf_in)
 *
 * 	Call 16-bit BIOS call using V8086 mode with buffer.
 *
 *	It returns the value set in the %ah register, containing error
 *	code, if any. addr and size are used to specify the buffer to
 *	copy the data from the BIOS call.
 *
 *	Called from PCI routines.
 */
int
v86bios_pci_bios_call_usebuf(regs *pci_reg,
			     v86bios_buf_t *buf_out, v86bios_buf_t *buf_in)
			     
{
	intregs_t	regs;

	bzero((caddr_t) &regs, sizeof(intregs_t));

	regs.type = V86BIOS_INT32 | V86BIOS_COPYIN | V86BIOS_COPYOUT;
	regs.entry.intval = PCI_BIOS_INT;
	regs.eax.eax = pci_reg->eax.eax;
	regs.ecx.ecx = pci_reg->ecx.ecx;
	regs.edx.edx = pci_reg->edx.edx;
	regs.ebx.ebx = pci_reg->ebx.ebx;
	regs.bp = 0;
	regs.esi.esi = pci_reg->esi.esi;
	regs.edi.edi = pci_reg->edi.edi;
	
	regs.ds = 0xf000;	/* part of PCI spec */
	regs.es = 0;		/* intentionally 0 to pass RouteBuffer, e.g. */

	if (buf_out != NULL) {
		regs.buf_out = buf_out;
	} 
	
	if (buf_in != NULL) {
		regs.buf_in = buf_in;
	}

	v86bios(&regs);

	pci_reg->eax.eax = regs.eax.eax;
	pci_reg->ecx.ecx = regs.ecx.ecx;
	pci_reg->edx.edx = regs.edx.edx;
	pci_reg->ebx.ebx = regs.ebx.ebx;
	pci_reg->esi.esi = regs.esi.esi;
	pci_reg->edi.edi = regs.edi.edi;

	return regs.eax.byte.ah;
}



/*
 * int
 * v86_eisa_read_slot(int slot, eisa_nvm_slotinfo_t* slotinfo)
 *
 * Calling/Exit State:
 *	Return error code returned in %ah. The info is already
 *	allocated by the caller.
 */
int
v86bios_eisa_read_slot(int slot, eisa_nvm_slotinfo_t *slotinfo)
{
	int		error;
	intregs_t	regs;
	ushort_t	*sdst;	/* for short */
	uchar_t		*cdst;	/* for char */

	struct reg_word {
	    uchar_t	l_reg;
	    uchar_t	h_reg;
	} 		*r_di, *r_si;

	bzero((caddr_t) &regs, sizeof(intregs_t));
	bzero((caddr_t) slotinfo, sizeof(eisa_nvm_slotinfo_t));
	
	regs.type = V86BIOS_INT16;
	regs.entry.intval = EISA_BIOS_INT;
	regs.eax.word.ax = 0xd800;
	regs.ecx.byte.cl = (uchar_t) slot;
	regs.edx.word.dx = 0;
	regs.ebx.word.bx = 0;
	regs.ds = 0;
	regs.bp = 0;
	regs.esi.word.si = 0;
	regs.es = 0;
	regs.edi.word.di = 0;

	v86bios(&regs);

	error = regs.eax.byte.ah;

	r_di = (struct reg_word *) &regs.edi.word.di;
	r_si = (struct reg_word *) &regs.esi.word.si;

	slotinfo->boardid[0] = r_di->l_reg;
	slotinfo->boardid[1] = r_di->h_reg;
	slotinfo->boardid[2] = r_si->l_reg;
	slotinfo->boardid[3] = r_si->h_reg;

	slotinfo->revision = regs.ebx.word.bx;	/* major and minor revision */
	slotinfo->functions = regs.edx.byte.dh;	/* no. of functions */

	cdst = (uchar_t *) &slotinfo->fib;
	*cdst = regs.edx.byte.dl;	/* function info byte */

	slotinfo->checksum = regs.ecx.word.cx;	/* checksum */
	
	sdst = (ushort_t *) (uint_t) &slotinfo->dupid;
	*sdst = regs.eax.byte.al;

	return error;		/* must be 0 */
}


/*
 * int
 * v86bios_eisa_read_func(int slot, int func, eisa_nvm_funcinfo_t *info)
 *	Read function data from EISA cmos memory/configuration space.
 *
 * Calling/Exit State:
 *	Return 1 if carry flag is set, otherwise return 0.
 */
int
v86bios_eisa_read_func(int slot, int func, eisa_nvm_funcinfo_t *info)
{
	intregs_t	regs;
	v86bios_buf_t	buf;
	
	bzero((caddr_t) &regs, sizeof(intregs_t));

	regs.type = V86BIOS_INT16 | V86BIOS_COPYIN;

	regs.buf_in = &buf;
	regs.buf_in->v86addr = (void *) V86BIOS_PDATA_BASE;
	regs.buf_in->kaddr = info;
	regs.buf_in->size = 0x140;	/* size of eisa_nvm_funcinfo_t */
	
	regs.entry.intval = EISA_BIOS_INT;
	regs.eax.word.ax = 0xd801;	/* 16-bit addressing */
	regs.ecx.byte.ch = (uchar_t) func;
	regs.ecx.byte.cl = (uchar_t) slot;
	regs.edx.word.dx = 0;
	regs.ebx.word.bx = 0;
	regs.ds = V86BIOS_DS_SEG;
	regs.esi.word.si = V86BIOS_PDATA_BASE;
	regs.bp = 0;
	regs.es = 0;
	regs.edi.word.di = 0;

	v86bios(&regs);

	/*
	 * If error, clean up junk data.
	 */
	if (regs.retval)
		bzero(regs.buf_in->kaddr, regs.buf_in->size);

	return regs.retval;
}


/*
 * caddrt_t
 * v86bios_vdc_info()
 *
 * Calling/Exit State:
 *	Called by vdc_info() in vdc.c to verify the monitor type, returning
 *	the address containing the info. This may be called mutiple times.
 *
 * Remarks:
 *	AH = 0x12 and BL = 0x10 return the information about the adapter's
 *	current setting -- color or monochrome mode, amount of memory and
 *	switch settings for EGA cards.
 */
struct vdc_info _vdc_info;

int vdc_info_init = 0;
caddr_t
v86bios_vdc_info(void)
{

	intregs_t	regs;

	bzero((caddr_t) &regs, sizeof(intregs_t));

	if (!vdc_info_init) {
		vdc_info_init = 1;

		regs.type = V86BIOS_INT16;
		regs.entry.intval = VIDEO_BIOS_INT;
		regs.eax.word.ax = 0x1200;
		regs.ecx.word.cx = 0;
		regs.edx.word.dx = 0;
		regs.ebx.word.bx = 0x0010;
		regs.bp = 0;
		regs.esi.word.si = 0;
		regs.edi.word.di = 0;
		regs.ds = 0;
		regs.es = 0;

		v86bios(&regs);

		if (regs.error)
			return NULL;

		_vdc_info.v_info.dsply =
			regs.ebx.byte.bh ? KD_STAND_M : KD_STAND_C;
		_vdc_info.v_info.rsrvd = regs.ebx.byte.bl; /* memory */
		_vdc_info.v_switch = regs.ecx.byte.cl;	  /* EGA switch setting */
	}

	return ((caddr_t)&_vdc_info);
}

extern uint_t egafontptr[];
extern ushort_t egafontsz[];

/*
 * void
 * v86bios_get_cdt(void)
 *
 */
void
v86bios_get_cdt(void)
{
	intregs_t	regs;
	int i;

	struct farptr {
	    ushort_t	offset;
	    ushort_t	es;
	} *cdtptr;

	ushort_t _bx[5] = {
	    0x0300,			/* 8x8 font */
	    0x0200,			/* 8x14 font */
	    0x0500,			/* 9x14 font */
	    0x0600,			/* 8x16 font */
	    0x0700,			/* 9x16 font */
	};

	bzero((caddr_t) &regs, sizeof(intregs_t));
	
	cdtptr = (struct farptr *) egafontptr;

	for (i = 0; i < 5; i++) {
		regs.type = V86BIOS_INT16;
		regs.entry.intval = VIDEO_BIOS_INT;
		regs.eax.word.ax = 0x1130; /* get charactor definition table */
		regs.ecx.word.cx = 0;
		regs.edx.word.dx = 0;
		regs.ebx.word.bx = _bx[i];
		regs.bp = 0;
		regs.esi.word.si = 0;
		regs.edi.word.di = 0;
		regs.ds = 0;
		regs.es = 0;

		v86bios(&regs);

		(cdtptr + i)->offset = regs.bp;
		(cdtptr + i)->es = regs.es;
		egafontsz[i] = regs.ecx.word.cx;
	}

}

/*
 * void
 * v86bios_setprot(void)
 *	Set protection against the system ports listed in
 *	v86bios_sysports[].
 *
 * Calling/Exit State:
 *	Called by v86bios().
 *
 * Note:
 *	Ports that require protection must be in the following list.
 */

int v86bios_sysports[] = {
	I8259_IC_1 + I8259_ICW1,
	I8259_IC_1 + I8259_ICW2,
	I8259_IC_2 + I8259_ICW1,
	I8259_IC_2 + I8259_ICW2,
	I8254_CTR0_PORT,
	I8254_CTR1_PORT,
	I8254_CTR2_PORT,
	-1,
};

STATIC void
v86bios_setprot(void)
{
	uchar_t	*bitmap, old_val;
	int i, index, offset, port;

	bitmap = (uchar_t *) ((int) (void *) v86_tss + sizeof (struct tss386));

	for (i = 0; (port = v86bios_sysports[i]) != -1; i++) {
		index = (port >> 3);	/* get the index to the array */
		offset = port - (index << 3); /* and which bit */
		old_val = *(bitmap + index);
		*(bitmap + index) = (1 << offset) | old_val; /* set the bit */
	}
}


/*
 * int
 * v86bios_chkopcode(uint_t eip, volatile int type)
 *
 * Calling/Exit State:
 *	Called by trap() when V86 program executes the IN, INS, OUT, and OUTS
 *	instructions, which cause a general protection error to protect
 *	system ports defined in the v86bios_sysports[]. The page0 is mapped
 *	at this moment, and the privilege level 0 stack is used.
 *
 *	The function basically does the following:
 *
 *	=> Check the opcode
 *	=> Add the length of any operand(s) to the opcode length.
 *	=> Change stack frame so that the processor can skip the instruction
 *	   or redirect the instruction (int N).
 *	=> Return 0 if successful, otherwise -1 (some other error)
 *
 * Note:
 *	The interface should be extended so that the caller can add/change
 *	ports to protect. See an Intel manual for the abbreviations below. 
 *
 *	The current implemenation does not check "type"
 */
struct  {
	uchar_t v_opcode;	/* opecode */
	int	v_length;	/* operand length */
} v86bios_io_opcode [] = {
	{0xe4, 1},		/* IN <AL,Ib> */
	{0xe5, 1},		/* IN <AX,Ib> */
	{0xe6, 1},		/* OUT <Ib,AL> */
	{0xe7, 1},		/* OUT <Ib,AX> */
	{0x6c, 0},		/* INSB <Yb,DX> */
	{0x6d, 0},		/* INSW <Yw,DX> */
	{0x6e, 0},		/* OUTSB <DX,Xb> */
	{0x6f, 0},		/* OUTSW <DX,Xw> */
	{0xec, 0},		/* IN <AL,DX> */
	{0xee, 0},		/* OUT <DX,AL> */
	{0},
};

#define PREFIX_ADDR	0x67	/* address-size prefix */
#define PREFIX_OPER	0x66	/* operand-size prefix */

int
v86bios_chkopcode(uint_t eip, volatile int type)
{
	uchar_t		*opcode;
	uint_t		cs;
	v86_pl0stack_t	*pl0stack;
	int		i, skip = 1,	/* size of opcode */
			found = 0;

	pl0stack = (v86_pl0stack_t *) (v86mon_stack + V86MON_STACKSIZE
				       - sizeof (uint_t)); 
	pl0stack--;		/* all registers pushed */

	cs = pl0stack->v_cs;
	eip = (cs << 4) + eip;	/* 8086 segmentation! */
	opcode = (uchar_t *) eip;

	if (*opcode == PREFIX_ADDR || *opcode == PREFIX_OPER) {
		opcode++;	/* skip prefix */
		skip++;
	}

	for (i = 0;  v86bios_io_opcode[i].v_opcode != 0; i++) {
		if (*opcode == v86bios_io_opcode[i].v_opcode) {
			found = 1;
			skip += v86bios_io_opcode[i].v_length;
			break;
		}
	}

	if (found) {
		/*
		 * Change the return link on the stack.
		 */
		pl0stack->v_eip = eip + skip;
		return 0;
	}

#define INTN_OPCODE 0xcd
	/*
	 * Check if the opcode is "int N"
	 *
	 * 1. Locate the handler, using "N"
	 * 2. Build PL3 stack frame so that "iret" can work
	 * 3. Set <cs, offset> in the PL0 stack to jump to the hanlder
	 */
	if (*opcode == (uchar_t) INTN_OPCODE) {
		uchar_t		*operand;
		v86_farptr_t 	*ent = 0; /* physical page 0 */
		ushort_t	cs, off, *pl3stack, pl3flags;

		skip += 1;	/* INT <Ib> */
		operand = opcode;
		operand++;
		cs = (ent + *operand)->v86_cs;
		off = (ent + *operand)->v86_off;

		pl3stack = (ushort_t *) pl0stack->v_esp;
		pl3flags = (ushort_t) (pl0stack->v_eflags & 0xffff);
		*(--pl3stack) = pl3flags;
		*(--pl3stack) = pl0stack->v_cs;
		*(--pl3stack) = (ushort_t) (pl0stack->v_eip + skip); /* next */

		pl0stack->v_cs = cs;
		pl0stack->v_eip = off;
		pl0stack->v_esp -= 6;
		return 0;
	} else {
		return -1;
	}
}



