#ident	"@(#)kern-i386at:svc/mki.c	1.3"
#ident	"$Header$"

#define _MKI_C
#include <mem/kmem.h>
#include <mem/hat.h>
#include <mem/as.h>
#include <mem/page.h>
#include <mem/lock.h>
#include <mem/seg.h>
#include <mem/seg_vn.h>
#include <mem/ublock.h>
#include <mem/vmparam.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <proc/disp.h>
#include <svc/v86bios.h>
#include <svc/errno.h>
#include <svc/mki.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/inline.h>

/*
 * MKI.3 (Merge/Kernel Interface) for Merge 4.0.
 *
 * Merge requires two sets of interfaces, MKI and MHI. Most of the MKI
 * interfaces seem to be OS independent, but we have some UW specific
 * interfaces (see mki.h). Each MKI function must be implemented by the OS
 * implementor, thus, SCO. See the interface spec from Platinum for the
 * details. Some functions, such as mki_alloc_context(), are not implemented
 * yet. They are not required for functionality, but are desirable for
 * performance improvement.
 *
 * MHI (Merge Hook Interface) is used only in UW 7, especially to expose the
 * hooks needed for Merge. The calling method is exploiting the technique
 * used by the PSMv2. The MHI functions are implemented by the provider of the
 * Merge, thus, Platinum. The mki_install_hook() is used to install the
 * hooks. By default, the hooks are void, but some of them must return some
 * values.
 *
 * Intel Workaround for the "Invalid Operand with Locked Compare Exchange
 * 8Byte (CMPXCHG8B) Instruction" is complicating the implementation for
 * allocating new private IDT. 
 */

#ifdef MERGE386

#define MKI_DLP_CLEAR 	0x9f 	/* 10011111 -- P, DLP, type */

STATIC void *mki_scratch_ptr = NULL;

static int
mhi_void0(void)
{
	return 0;
}

static int
mhi_void1(void)
{
	return 1;
}

static int
mhi_void2(void)
{
	return -1;
}

int (*mhi_table[])() = {
	&mhi_void2,		/* SWITCH_AWAY */
	&mhi_void2,		/* SWITCH_TO */
	&mhi_void2,		/* THREAD_EXIT */
	&mhi_void2,		/* RET_USER */
	&mhi_void2,		/* SIGNAL */
	&mhi_void2,		/* QUERY */
	&mhi_void1,		/* UW_PORT_ALLOC */
	&mhi_void1,		/* UW_PORT_FREE */
	&mhi_void0,		/* UW_COM_PPI_STRIOCTL */
	&mhi_void0,		/* UW_KD_PPI_IOCTL */
};

/*
 * int
 * mki_install_hook(enum hook_id, int (*hook_fn)())
 *	Called when the Merge driver is loaded.
 *
 *	No locks are held because this is called only at _load time.
  */
int
mki_install_hook(enum hook_id id, int (*hook_fn)())
{

	switch(id) {	
	case SWITCH_TO:
		mhi_table[SWITCH_TO] = hook_fn;
		break;
	case SWITCH_AWAY:
		mhi_table[SWITCH_AWAY] = hook_fn;
		break;
	case THREAD_EXIT:
		mhi_table[THREAD_EXIT] = hook_fn;
		break;
	case RET_USER:
		mhi_table[RET_USER] = hook_fn;
		break;
	case SIGNAL:
		mhi_table[SIGNAL] = hook_fn;
		break;	
	case QUERY:
		mhi_table[QUERY] = hook_fn;
		break;
	case UW_PORT_ALLOC:
		mhi_table[UW_PORT_ALLOC] = hook_fn;
		break;
	case UW_PORT_FREE:
		mhi_table[UW_PORT_FREE] = hook_fn;
		break;
	case UW_COM_PPI_IOCTL:
		mhi_table[UW_COM_PPI_IOCTL] = hook_fn;
		break;
	case UW_KD_PPI_IOCTL:
		mhi_table[UW_KD_PPI_IOCTL] = hook_fn;
		break;
	default:
		goto mki_error;
	}

	return 0;

mki_error:
	return -1;
}

/*
 * void
 * mki_remove_hook(enum hooo_id id)
 *	Remove a hook sepcified by id.
 *	Called when the Merge driver is unloaded.
 */
void
mki_remove_hook(enum hook_id id)
{

	ASSERT(mhi_table != NULL);
	
	switch(id) {	
	case SWITCH_TO:
		mhi_table[SWITCH_TO] = NULL;
		break;
	case SWITCH_AWAY:
		mhi_table[SWITCH_AWAY] = NULL;
		break;
	case THREAD_EXIT:
		mhi_table[THREAD_EXIT] = NULL;
		break;
	case RET_USER:
		mhi_table[RET_USER] = NULL;
		break;
	case SIGNAL:
		mhi_table[SIGNAL] = NULL;
		break;	
	case QUERY:
		mhi_table[QUERY] = NULL;
		break;	
	case UW_PORT_ALLOC:
		mhi_table[UW_PORT_ALLOC] = NULL;
		break;
	case UW_PORT_FREE:
		mhi_table[UW_PORT_FREE] = NULL;
		break;
	case UW_COM_PPI_IOCTL:
		mhi_table[UW_COM_PPI_IOCTL] = NULL;
		break;
	case UW_KD_PPI_IOCTL:
		mhi_table[UW_KD_PPI_IOCTL] = NULL;
		break;
	default:
		break;
	}
}

/*
 * Allocate and initialize a private IDT (l.idtp).
 * It also allocates and set desctab (l.l_desctabp) so that the kernel can
 * load the idt across context switch easily.
 */
STATIC void
mki_build_private_idt(lwp_t *lwpp)
{
	struct gate_desc	*idtp;

	if (l.cpu_id != CPU_P5) {
		idtp = kmem_alloc(sizeof (struct gate_desc) * IDTSZ,
				  KM_SLEEP);
		bcopy((caddr_t) cur_idtp, idtp,
		      sizeof(struct gate_desc) * IDTSZ);
	} else {
		/*
		 * Intel Workaround for the "Invalid Operand
		 * with Locked Compare Exchange 8Byte (CMPXCHG8B) Instruction"
		 */

		page_t 	*pp;
		void	*tmp;
		extern void p5_pgflt();	/* in intr.s */
		char 	*save_pp;

		/*
		 * Note that it is assuming the size for a new private IDT
		 * is less than a page.
		 */
		ulong_t npages = 1;
		
		pp = (page_t *) allocpg();

		if (pp == NULL)
			do {
				tmp = kmem_alloc(2*PAGESIZE, KM_SLEEP);
				kmem_free(tmp, 2*PAGESIZE);
				pp = (page_t *) allocpg();
			} while (pp == NULL);

		idtp = (struct gate_desc *)
			kpg_ppid_mapin(npages,
				       pp->p_pfn, PROT_ALL & ~PROT_USER,
				       KM_SLEEP);
		bcopy((caddr_t) cur_idtp, idtp,
		      sizeof(struct gate_desc) * IDTSZ);
		/*
		 * Save pp for freepg() after IDT
		 */
		save_pp = (char *) (idtp + IDTSZ);
		*(page_t **) save_pp = pp;
		/*
		 * Reset pgflt handler to redirect the page fault to
		 * INVOPFLT gracefully.
		 */
		idtp[PGFLT].gd_offset_low = GD_OFFSET_LOW(&p5_pgflt);
		idtp[PGFLT].gd_offset_high = GD_OFFSET_HIGH(&p5_pgflt);
	}

	lwpp->l_desctabp = kmem_alloc(sizeof (struct desctab),
				      KM_SLEEP); 
	BUILD_TABLE_DESC(lwpp->l_desctabp, idtp, IDTSZ);
	cur_idtp = lwpp->l_idtp = idtp;
}

#define IDT_SET_WRPROT() \
	kvtol2ptep((vaddr_t)cur_idtp)->pg_pte &= ~(PG_US|PG_RW);\
	kvtol2ptep((vaddr_t)cur_idtp)->pg_pte |= PG_REF; \
	TLBSflush1((vaddr_t)cur_idtp)

#define IDT_RESET_WRPROT() \
	kvtol2ptep((vaddr_t)cur_idtp)->pg_pte |= PG_US|PG_RW; \
	TLBSflush1((vaddr_t)cur_idtp)
	
void
mki_set_idt_dpl (void)
{
	int			i;
	lwp_t 			*lwpp = u.u_lwpp;
	
	/*
	 * Check if it has a private IDT
	 */
	if (cur_idtp == l.std_idt) {
		mki_build_private_idt(lwpp);
		DISABLE_PRMPT();
		loadidt(lwpp->l_desctabp);
		lwpp->l_special |= SPECF_PRIVIDT;
	} else if (l.cpu_id == CPU_P5) {
		/*
		 * Make this page writable to update it
		 */
		DISABLE_PRMPT();
		IDT_RESET_WRPROT();
	}
	/*
	 * Clear DLP value. The rest of IDT entries are all 0.
	 */
	for (i = 0; i < IDTSZ; i++)
		cur_idtp[i].gd_type_etc &= MKI_DLP_CLEAR;

	/*
	 * Make unwriteable
	 */
	if (l.cpu_id == CPU_P5)
		IDT_SET_WRPROT();
	
	ENABLE_PRMPT();
}

void
mki_set_idt_entry (unsigned short vect_num, unsigned long *new_entry,
			unsigned long *prev_entry)
{
	lwp_t 			*lwpp = u.u_lwpp; 
	
	ASSERT( vect_num >= DIVERR && vect_num <= EXTERRFLT);

	/*
	 * Check if it has a private IDT
	 */
	if (cur_idtp == l.std_idt) {
		mki_build_private_idt(lwpp);
		DISABLE_PRMPT();
		loadidt(lwpp->l_desctabp);
		lwpp->l_special |= SPECF_PRIVIDT;
	} else if (l.cpu_id == CPU_P5) {
		/*
		 * Make this page writable to update it
		 */
		DISABLE_PRMPT();
		IDT_RESET_WRPROT();
	}

	*((struct gate_desc *) prev_entry) =
	  	*((struct gate_desc *) &cur_idtp[vect_num]);
	*((struct gate_desc *) &cur_idtp[vect_num]) =
		*(struct gate_desc *) new_entry;

	/*
	 * Make unwriteable
	 */
	if (l.cpu_id == CPU_P5)
		IDT_SET_WRPROT();

	ENABLE_PRMPT();
}

/*
 * Set a per-lwp value to point to a vm86 structure
 */
void
mki_set_vm86p(void *vm86p)
{
	u.u_vm86p = vm86p;
}

/*
 * Get a per-lwp pointer to a vm86 structure
 */
void *
mki_get_vm86p()
{
	return u.u_vm86p;
}


/*
 * void
 * mki_getparm(enum request, void *)
 *	Get values needed by the add-on MERGE386 module.
 *
 * Calling/Exit State:
 *	On return, (*parm) is set to the value of the parameter indicated
 *	by parm.
 *
 *	All of these parameters are either constant (from the base kernel's
 *	point of view) or are per-LWP, per-proc, or per-engine fields accessed
 *	in context, so no locking is needed.
 */
void
mki_getparm(enum request r, void *parm)
{

	*(int *) parm = -1;		/* for sanity */

	switch (r) {
	case PARM_POST_COOKIE:
		*(lwp_t **)parm = u.u_lwpp;
		break;
	case PARM_FRAME_BASE:
		*(int **)parm = u.u_ar0;
		break;
	case PARM_CPU_TYPE:
		*(uint_t *)parm = l.cpu_stepping | (l.cpu_id << 8);
		break;
	case PARM_PRIVATE:
		*(void **) parm = &mki_scratch_ptr;
		break;
	case PARM_IDTP:
		*(struct gate_desc **)parm = cur_idtp;
		break;
	case PARM_LDTP:
		*(struct segment_desc **)parm =
				u.u_dt_infop[DT_LDT]->di_table;
		break;
	case PARM_GDTP:
		*(struct segment_desc **)parm =
				u.u_dt_infop[DT_GDT]->di_table;
		break;
	case PARM_TSSP:
		/* (void) iobitmap_sync(); */
		if (u.u_lwpp->l_tssp == NULL)
			*(struct tss386 **)parm = NULL;
		else
			*(struct tss386 **)parm = &u.u_lwpp->l_tssp->st_tss;
		break;
	case PARM_RUID:
		*(uid_t *)parm = CRED()->cr_ruid;
		break;
	default:
#ifdef DEBUG
		cmn_err(CE_CONT,
			"mki_getparm: not support this request %d\n", r);
#endif
		break;
		
	}
}


/*
 * void
 * mki_post_event(void *cookie)
 *  Post a MERGE event to the given LWP pinted to by cookie.
 *
 * Calling/Exit State:
 *  No locks required on entry.
 *  This routine does not block.
 */
void
mki_post_event(void *cookie)
{
	lwp_t *lwpp = (lwp_t *) cookie;
	pl_t oldpl;

	oldpl = LOCK(&lwpp->l_mutex, PLHI);
	if (lwpp->l_trapevf & EVF_L_MERGE) {
		/* Already posted */
		UNLOCK(&lwpp->l_mutex, oldpl);
		return;
	}
	lwpp->l_trapevf |= EVF_L_MERGE;
	/*
	 * If LWP is running on another engine, we must nudge it.
	 */
	if (lwpp->l_stat == SONPROC && lwpp != u.u_lwpp) {
		ASSERT(lwpp->l_eng != l.eng);
		RUNQUE_LOCK();  /* must hold runque lock for nudge() */
		nudge(PRIMED, lwpp->l_eng);
		RUNQUE_UNLOCK();
	}
	UNLOCK(&lwpp->l_mutex, oldpl);
}

/*
 *
 * void
 * mki_mark_vm86(void)
 *
 *	Set the SPEC_VM86 flag indicating that the process is a merge 
 *	process.
 *
 * Calling/Exit State:
 *	This function is called by the merge driver. 
 *	It is always called in context of the lwp.
 *
 * Description:
 *	The kernel sets the flag in the lwp structure and
 * 	the plocal structure.
 */
void
mki_mark_vm86(void)
{
	u.u_lwpp->l_special |= SPECF_VM86;
	l.special_lwp |= SPECF_VM86;
}

/*
 *
 * boolean_t
 * mki_check_vm86(void)
 *
 *	Check if the SPECF_VM86 flag is set for the lwp.
 *
 * Calling/Exit State:
 *	This function is called by the merge driver. 
 *	It is always called in context of the lwp.
 */
boolean_t
mki_check_vm86(void)
{
	if (l.special_lwp & SPECF_VM86) {
		ASSERT(u.u_lwpp->l_special & SPECF_VM86); 
		return B_TRUE;
	}
	
	return B_FALSE;
}

/*
 *
 * void
 * mki_clear_vm86(void)
 *
 *	Clear the SPECF_VM86 flag indicating that the process is no longer
 *	a merge process.
 *
 * Calling/Exit State:
 *	This function is called by the merge driver. 
 *	It is always called in context of the lwp.
 *
 * Description:
 *	The kernel clears the flag in the lwp structure and
 * 	the plocal structure.
 */
void
mki_clear_vm86(void)
{
	lwp_t *lwpp = u.u_lwpp;
	
	lwpp->l_special &= ~SPECF_VM86;
	l.special_lwp &= ~SPECF_VM86;
	
	if (lwpp->l_special & SPECF_PRIVIDT) {
		restore_std_idt();
		lwpp->l_special &= ~SPECF_PRIVIDT; 
		if (l.cpu_id != CPU_P5) {
			kmem_free(lwpp->l_idtp, sizeof (struct gate_desc) *
			IDTSZ);
		} else {
			char *save_pp;
			page_t *pp;

			save_pp = (char *) (lwpp->l_idtp + IDTSZ);
			pp = *((page_t **) save_pp);
			kpg_mapout(lwpp->l_idtp, 1);
			freepg(pp);
		}
		kmem_free(lwpp->l_desctabp, sizeof (struct desctab));
	}
}


/*
 * void
 * mki_pgfault_get_state(int *pfault_ok, mki_fault_catch_t *fcstate)
 *	get fault catch state
 *
 * input:	pfault_ok - is our context ok to handle page faults
 * 	fcstate - fault catch state to return
 *
 * 	This function reads and resets are current fault catching state
 *
 * output:
 * 	pfault_ok set to indicate context
 * 	if pfault_ok true, previous fault catching state saved
 * 		in fcstate
 */
void
mki_pgfault_get_state(int *pfault_ok, mki_fault_catch_t *fcstate)
{
	boolean_t inter_servicing = B_FALSE;
	fault_catch_t *fc;

	/*
	 *  Context check.  Make sure that we are not in the middle of
	 *  servicing an interrupt.  If so, then the fault catch information
	 *  is not valid.
	 */
	if(servicing_interrupt())
		inter_servicing = B_TRUE;
		
	*pfault_ok = !inter_servicing;
	if (inter_servicing)
		return;
	
	/*
	 *  Save the old state and clear the current.
	 */
	fc = (fault_catch_t *) &u.u_fault_catch;

	fcstate->mkifc_catching_user_fault =
				fc->fc_flags & (CATCH_UFAULT|CATCH_ALL_FAULTS);
	fcstate->mkifc_os_dependent[FC_FUNC_INDEX] = (int) fc->fc_func;
	fcstate->mkifc_os_dependent[FC_FLAGS_INDEX] = fc->fc_flags;
	fc->fc_func = 0;
	fc->fc_flags = 0;
}

/*
 * void
 * mki_pgfault_restore_state(mki_fault_catch_t *fcstate)
 *
 * input:	fcstate - fault catch state to restore
 *
 * 	This function restores the current fault catching state
 *
 * output: None
 */
void
mki_pgfault_restore_state(mki_fault_catch_t *fcstate)
{
	fault_catch_t *fc;
	
	fc = (fault_catch_t *) &u.u_fault_catch;

	fc->fc_func = (void (*)()) fcstate->mkifc_os_dependent[FC_FUNC_INDEX];
	fc->fc_flags = fcstate->mkifc_os_dependent[FC_FLAGS_INDEX];
}

/*
 * struct tss386 *
 *	mki_alloc_priv_tss(void)
 *
 *	Allocate a maximally sized private TSS for use by the calling lwp.
 *	The private TSS is initialized to the values of the previous private
 *	TSS, and the previous private TSS if present will be freed. The I/O
 *	permission bitmap field of the TSS will be set to what was previously
 *	there, if any.
 *
 *	This function has conflicts with iobitmapctl(). The new flag
 *	SPECF_LWP_PRIVTSS is used for the kernel not to change the TSS.
 *
 *	The private TSS is freed when the process is freed.
 */
struct tss386 *
mki_alloc_priv_tss(void)
{
	lwp_t *lwpp = u.u_lwpp;
	proc_t *procp = u.u_procp;
	struct stss *tssp = u.u_procp->p_tssp;
	struct stss *ntssp, *ltssp = lwpp->l_tssp;
	uint_t allocsz;
	/* For maximally sized TSS, port 0 through 65535 */
	uint_t bitsz = IOB_MAXPORT_MAP + 1; /* bitmap + terminator */

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (!SINGLE_THREADED())
		SLEEP_LOCK(&procp->p_tsslock, PRIMED);

	allocsz = sizeof(struct stss) + bitsz;

	ntssp = kmem_alloc(allocsz, KM_SLEEP);

	if (tssp == NULL) {
		ntssp->st_tss = l.tss;
		BITMASKN_SETALL((uint_t *)(ntssp + 1),
				bitsz / sizeof(uint_t));
	} else {
		bcopy(tssp, ntssp, tssp->st_allocsz);
		BITMASKN_SETALL((uint_t *)(void *)
				((char *)ntssp +
				 tssp->st_allocsz),
				(allocsz - tssp->st_allocsz) /
				sizeof(uint_t));
		/*
		 * Remove the process reference from the old
		 * copy; free it if it's the last reference.
		 */
		ASSERT(tssp->st_refcnt != 0);
		if (--tssp->st_refcnt == 0)
			kmem_free(tssp, tssp->st_allocsz);
	}
	
	/*
	 * Free the current one, if any
	 */
	if (ltssp != NULL)
		kmem_free(ltssp, ltssp->st_allocsz);
		
	ntssp->st_allocsz = (ushort_t)allocsz;
	ntssp->st_tss.t_bitmapbase = sizeof(struct tss386);
	ntssp->st_refcnt = 1; 
	ntssp->st_gencnt = 0;

		/* Create a descriptor for this TSS */
	BUILD_SYS_DESC(&ntssp->st_tssdesc, &ntssp->st_tss,
		       sizeof(struct tss386) + bitsz - 1,
		       TSS3_KACC1, TSS_ACC2);

	/* Initialize kernel stack pointer in TSS */
	ntssp->st_tss.t_esp0 = (ulong_t)&u - KSTACK_RESERVE;
	ASSERT(ntssp->st_tss.t_ss0 == KDSSEL);

	/* Install the new TSS */
	lwpp->l_tssp = ntssp;

	DISABLE_PRMPT();
	/* Load descriptor into GDT */
	u.u_dt_infop[DT_GDT]->di_table[seltoi(KPRIVTSSSEL)] =
		lwpp->l_tssp->st_tssdesc;
	/* Set the Task Register */
	loadtr(KPRIVTSSSEL);
	u.u_lwpp->l_special |= (SPECF_PRIVTSS | SPECF_LWP_PRIVTSS);
	l.special_lwp = u.u_lwpp->l_special;
	ENABLE_PRMPT();
			
	if (!SINGLE_THREADED())
		SLEEP_UNLOCK(&procp->p_tsslock);
	
	return &ntssp->st_tss;
}

/*
 * int
 * mprotect_k(unsigned long base, unsigned int size, unsigned int prot)
 *	Back door for mprotect(2).
 *
 *  On success, zero is returned and the requested permissions have
 *  become effective on the specified range of addresses.
 *
 *  On failure, a non-zero errno is returned and the permissions of
 *  of the entire specified range may or may not be modified as per
 *  the request.
 *
 */
int
mprotect_k(unsigned long base, unsigned int size, unsigned int prot)
{
	struct as *as;
	int error;
	vaddr_t raddr;
	struct seg *seg;
	uint_t	nprot, oprot, rsize, ssize;

	raddr = (vaddr_t)((u_int)base & PAGEMASK);
	rsize = (((u_int)(base + size) + PAGEOFFSET) & PAGEMASK) -
		(u_int)raddr;

	if (VALID_USR_RANGE(base, size) == 0) {
		return(ENOMEM);
	}

	as = u.u_procp->p_as;
	seg = as_segat(as, base);

	if (seg == NULL)
		return (ENOMEM);

	do {
		if (raddr >= seg->s_base + seg->s_size) {
			seg = seg->s_next;  /* goto next seg */
			if (raddr != seg->s_base)
				return (ENOMEM);
		}
		if ((raddr + rsize) > (seg->s_base + seg->s_size))
			ssize = seg->s_base + seg->s_size - raddr;
		else
			ssize = rsize;

		/*
		 * If Asynchronous I/O in use, see if
		 * the ranges overlap.  If they do,
		 * wait for the IO to finish, and
		 * unlock the locked down memory.
		 */
		if (u.u_procp->p_aioprocp)
			aio_intersect(as, raddr, ssize);

		SOP_GETPROT(seg, raddr, &oprot);
		
		nprot = (oprot & PROT_USER)?
			prot | PROT_USER : prot & ~PROT_USER;
			
		error =  SOP_SETPROT(seg, raddr, ssize, nprot);
		if (error != 0)
			return (error);

		raddr += ssize;
		rsize -= ssize;
	} while (rsize != 0);

	return (0);
		
}

/*
 * The function not implemented.
 * These are needed for perfomance improvements (not needed for
 * functionality) 
 */
int mki_alloc_context() { return 0; }
void mki_free_context() {}
void mki_mmap() {}
void mki_unmap() {}
void mki_load_context() {}

/*
 * Compatibility. They were in the merge stub.
 */
int
com_ppi_strioctl(struct queue *a, struct msgb *b,
                struct mrg_com_data *c, int d)
{
	return (mhi_com_ppi_ioctl(a, b, c, d));
}

int
portalloc(unsigned long a, unsigned long b) {
	return (mhi_port_alloc(a, b));
}

void
portfree(int a, int b)
{
	mhi_port_free(a, b);
}

int
kdppi_ioctl(queue_t *a, mblk_t *b, struct iocblk *c,
				   ws_channel_t *d)
{
	return(mhi_kd_ppi_ioctl(a, b, c, d));
}

#endif



