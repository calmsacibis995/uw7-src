#ident	"@(#)kern-i386at:util/mod/mod_intr.c	1.31.9.1"
#ident	"$Header$"

#include <mem/kmem.h>
#include <svc/bootinfo.h>
#include <svc/errno.h>
#include <svc/psm.h>
#include <svc/pic.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/mod/mod_hier.h>
#include <util/mod/mod_k.h>
#include <util/mod/mod_intr.h>
#include <util/mod/moddrv.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>


struct intr_vect_t *intr_vect;
extern	lock_t	mod_iv_lock;
extern	int	default_bindcpu;

extern	int	intnull();
extern int _Compat_intr_handler(void *);


int mod_add_intr(struct intr_info *, int (*)(), void *);
int mod_remove_intr(struct intr_info *, int (*)());

#define IV_LOCK()		LOCK(&mod_iv_lock, PLIV)
#define IV_UNLOCK()		UNLOCK(&mod_iv_lock, PLBASE)

/*
 * Macro to convert an interrupt spl priority to a value suitable for passing
 * to the PSM iv_idt.msi_order. This conversion is required because the base
 * kernel may use more priority levels than supported by the PSM. 
 * The current scheme is to map PLMAX (or greater) to the largest intr_order 
 * supported by the PSM. Values PLMAX-n are mapped to the larger 
 * of 0 or (intr_order_max - n). If the PSM supports enough priorities, great.
 * Otherwise, this gives unique priorities for the higher priority interrupts
 * and collapses the others to zero.
 */
#define MSI_ORDER(pri)	( ((int)(pri))>=PLMAX ? os_intr_order_max :		\
		(os_intr_order_max-PLMAX+((int)(pri)) < 0 ? 0 			\
		 : os_intr_order_max-PLMAX+((int)(pri))))

/*
 * void mod_shr_intn(int iv)
 * 	Shared interrupt routine called through intr_vect[].
 *	The argument idata is the pointer to the intr_vect[vecno].
 * 	Supports the dynamic addition and deletion of
 * 	shared interrupts, required by the dynamically
 * 	loadable module feature.
 *
 * Calling/Exit State:
 *	For the performance optimization, it begins with the one that
 *	asserts the last interrupt for DDI8 drivers (level triggered). 
 *	No lock should be held when enter this routine.
 *
 */
int
mod_shr_intn(void *idata)
{
	struct mod_shr_v	*svp, *next;
	int 			ret;
	boolean_t 		edge_type = B_TRUE;
	struct intr_vect_t 	*ivp  = (struct intr_vect_t *) idata;

	if (((ms_intr_dist_t *)ivp)->msi_flags & MSI_ITYPE_CONTINUOUS)
		edge_type = B_FALSE; /* level */
	
	for (svp = ivp->iv_mod_shr_p; ; svp = svp->msv_next) {
		ret = (svp->msv_handler)(svp->msv_idata);
		next = svp->msv_next;
		if (ret == ISTAT_ASSERTED) {
			if (edge_type) {
				ivp->iv_mod_shr_p = next;
				continue;
			}
			ivp->iv_mod_shr_p = svp;
			return ISTAT_ASSERTED;
		} 

		if (next == ivp->iv_mod_shr_p)
			return ISTAT_ASSUMED;
	
	}
}
	

/*
 * int mod_drvattach(struct mod_drvintr *aip)
 * 	Install and enable all interrupts required by a
 * 	given device driver.
 *
 * Calling/Exit State:
 *	Called from the _load routine of loadable modules.
 *	No locks held upon calling and exit.
 */
void
mod_drvattach(struct mod_drvintr *aip)
{
	struct	intr_info	*i_infop;
	struct	intr_info	ii;
	int	(*hndlr)();
	struct _Compat_intr_idata	*idata;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	moddebug(cmn_err(CE_CONT, "!MOD: mod_drvattach()\n"));

	if (!aip)
		return;

	if (aip->di_magic == MOD_INTR_MAGIC) {
		if (aip->di_version != MOD_INTR_VER)
			return;
		cm_intr_attach_all(aip->di_modname, (int (*)())aip->di_handler,
				   aip->di_devflagp, &aip->di_hook);
		return;
	}

	/*
	 * Convert old-style intr_info to something we can work with.
	 */
	if (!(i_infop = ((struct o_mod_drvintr *)aip)->drv_intrinfo))
		return;

	moddebug(cmn_err(CE_CONT, "!MOD:    Interrupts:\n"));

	hndlr = ((struct o_mod_drvintr *)aip)->ihndler;

	while (i_infop->ivect_no >= 0) {
		moddebug(cmn_err(CE_CONT, "!MOD:   %d\n", INTRNO(i_infop)));
		switch (INTRVER(i_infop)) {
		case 0:
			ii.ivect_no = ((struct intr_info0 *)i_infop)->ivect_no;
			ii.int_pri = ((struct intr_info0 *)i_infop)->int_pri;
			ii.itype = ((struct intr_info0 *)i_infop)->itype;
			ii.int_cpu = -1;
			ii.int_mp = 0;
			idata = kmem_alloc(sizeof(struct _Compat_intr_idata), KM_SLEEP );
                	idata->ivect = (void (*)()) hndlr;
                	idata->vec_no = ii.ivect_no;
			i_infop->int_idata = idata;
			i_infop = (struct intr_info *)
					((struct intr_info0 *)i_infop + 1);
			break;
		case MOD_INTRVER_42:
			ii = *i_infop++;
			ii.ivect_no &= ~MOD_INTRVER_MASK;
			break;
		}

		if (ii.itype == 0 || ii.ivect_no == 0)
			continue;

		mod_add_intr(&ii, &_Compat_intr_handler, idata);
	}

	return;
}

/*
 * int mod_drvdetach(struct mod_drvintr *aip)
 *	 Remove and disable all interrupts used by a given device driver.
 *
 * Calling/Exit State:
 *	Called from the _unload routine of loadable modules.
 *	No locks held upon calling and exit.
 */
void
mod_drvdetach(struct mod_drvintr *aip)
{
	struct	intr_info	*i_infop;
	struct	intr_info	ii;
	int	(*hndlr)();

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	moddebug(cmn_err(CE_CONT, "!MOD: mod_drvdetach()\n"));

	if (!aip)
		return;

	if (aip->di_magic == MOD_INTR_MAGIC) {
		if (aip->di_version != MOD_INTR_VER)
			return;
		cm_intr_detach_all(aip->di_hook);
		return;
	}

	/*
	 * Convert old-style intr_info to something we can work with.
	 */
	if (!(i_infop = ((struct o_mod_drvintr *)aip)->drv_intrinfo))
		return;

	moddebug(cmn_err(CE_CONT, "!MOD:    Interrupts:\n"));

	hndlr = ((struct o_mod_drvintr *)aip)->ihndler;

	while (i_infop->ivect_no >= 0) {
		moddebug(cmn_err(CE_CONT, "!MOD:   %d\n", INTRNO(i_infop)));
		switch (INTRVER(i_infop)) {
		case 0:
			ii.ivect_no = ((struct intr_info0 *)i_infop)->ivect_no;
			ii.int_pri = ((struct intr_info0 *)i_infop)->int_pri;
			ii.itype = ((struct intr_info0 *)i_infop)->itype;
			ii.int_cpu = -1;
			ii.int_mp = 0;
			ii.int_idata = i_infop->int_idata;
			i_infop = (struct intr_info *)
					((struct intr_info0 *)i_infop + 1);
			break;
		case MOD_INTRVER_42:
			ii = *i_infop++;
			ii.ivect_no &= ~MOD_INTRVER_MASK;
			break;
		}

		if (ii.itype == 0 || ii.ivect_no == 0)
			continue;

		mod_remove_intr(&ii, hndlr);
	}

	return;
}

/*
 * TEMPORARY WORKAROUND
 *
 * To avoid changing header files during the end-game, the iv_upcount
 * field is renamed to iv_bindcount here. Note also that iv_bindcpu is
 * no longer used.
 */
#define iv_bindcount iv_upcount

/*
 * int mod_add_intr(struct intr_info *iip, void (*ihp)(), void *)
 *	Add the interrupt handler ihp to the vector defined by iip.
 *
 * Calling/Exit State:
 *	None.
 */
int
mod_add_intr(struct intr_info *iip, int (*ihp)(), void *idata)
{
	uint_t			iv;
	int			err = 0, cpu;
	ms_cpu_t		cpu_dist, old_dist;
	boolean_t		intr_changed;
	struct intr_vect_t	*ivp;
	struct mod_shr_v	*current, *next;
	
	iv = iip->ivect_no;
	if (iv > os_islot_max)
		return(EINVAL);

	if (iip->itype == 0)
  		return(EINVAL);

	if (iv == 0)
		return(EBUSY);

	/*
	 * We need to allocate here to avoid lock hierarchy violation caused
	 * by IV_LOCK().
	 */
	current  = (struct mod_shr_v *)
		kmem_zalloc (sizeof (struct mod_shr_v), KM_SLEEP);

	if (current == NULL) {
			return(ENOMEM);
	}

	next  = (struct mod_shr_v *)
		kmem_zalloc (sizeof (struct mod_shr_v), KM_SLEEP);

	if (next == NULL) {
		kmem_free(current, sizeof (struct mod_shr_v));
		return(ENOMEM);
	}

	ivp = &intr_vect[iv];

	moddebug(cmn_err(CE_CONT, "!MOD: mod_add_intr(): %d\n", iv));

	cpu = iip->int_cpu;
	if (cpu == -2)
		cpu = default_bindcpu;
	if (cpu == -1 && !iip->int_mp)
		cpu = 0;
	cpu_dist = (cpu == -1 ? MS_CPU_ANY : cpu);

	(void)IV_LOCK();

	if (ivp->iv_ivect == intnull) {
		/*
		 * The interrupt vector is not currently used.
		 * We assume the interrupt is disabled.
		 */
		ivp->iv_ivect = ihp;
		ivp->iv_idt.msi_slot = iv;
		ivp->iv_idt.msi_order = MSI_ORDER(iip->int_pri);
		ivp->iv_idt.msi_flags = MSI_MASK_ON_INTR | (iip->itype == 4 ? MSI_ITYPE_CONTINUOUS : 0);
		ivp->iv_intpri = iip->int_pri;
		ivp->iv_itype = iip->itype;
		ivp->iv_osevent = 0;
		ivp->iv_mod_shr_p = NULL;
		ivp->iv_idata = idata; /* idata for the driver */

		ivp->iv_idt.msi_cpu_dist = cpu_dist;
		ivp->iv_bindcount = (cpu_dist == MS_CPU_ANY ? 0 : 1);

		DISABLE();
		if (!ms_intr_attach(&ivp->iv_idt)) {
attachfail:
			err = EINVAL;
			ivp->iv_ivect = intnull;
		}
		ENABLE();

		IV_UNLOCK();
		if (err) 
			cmn_err(CE_WARN, "Interrupt attach failed: slot:%d, pri:%d, type:%d\n",
				iv, iip->int_pri, iip->itype);
		
		kmem_free(current, sizeof(struct mod_shr_v));
		kmem_free(next, sizeof(struct mod_shr_v));
		return(err);
	}

	/*
	 * Check for sharing conflicts. Can't share interrupts if itypes don't
	 * match, or if different drivers need to be bound to different CPUs.
	 */
	if (iip->itype != ivp->iv_itype) {
conflict:
		IV_UNLOCK();
		kmem_free(current, sizeof(struct mod_shr_v));
		kmem_free(next, sizeof(struct mod_shr_v));
		return(EBUSY);
	}

	intr_changed = B_FALSE;
	old_dist = ivp->iv_idt.msi_cpu_dist;

	if (old_dist == cpu_dist) {
		;
	} else if (old_dist == MS_CPU_ANY) {
		ivp->iv_idt.msi_cpu_dist = cpu_dist;
		intr_changed = B_TRUE;
	} else if (cpu_dist != MS_CPU_ANY)
		goto conflict;

	/*
	 * If two devices sharing the same interrupt have different interrupt
	 * priorities, we will lower the priority of the shared interrupt to
	 * the minimum priority of all the devices.
	 */
	if (ivp->iv_intpri > (uchar_t)iip->int_pri) {
		ivp->iv_intpri = (uchar_t)iip->int_pri;
		intr_changed = B_TRUE;
	}

	if (intr_changed) {
		DISABLE();
		ms_intr_detach(&ivp->iv_idt);	/* XXX should be unnecessary */
		if (!ms_intr_attach(&ivp->iv_idt)) {
			goto attachfail;
		}
		ENABLE();
	}

	ivp->iv_bindcount += (cpu_dist == MS_CPU_ANY ? 0 : 1);

	if (ivp->iv_ivect != mod_shr_intn) {
		/*
		 * The interrupt is shared for the first time.
		 */

		/*
		 * Move the current one to the list, iv_vect, iv_idata are
		 * set to mod_shr_intn and ivp, respectively.
		 */
		current->msv_handler = ivp->iv_ivect;	/* current one */
		current->msv_idata = ivp->iv_idata; 	/* current one */
		/* new one */
		next->msv_handler = ihp; 		/* next handler */
		next->msv_idata = idata; 		/* idata for next
							   handler */
		/*
		 * Point idt entry to the shared interrupt routine.
		 */
		ivp->iv_ivect = mod_shr_intn;
		ivp->iv_idata = (void *) ivp; 		/* pointer to ivec */

		/*
		 * Make up the link
		 */
		ivp->iv_mod_shr_cnt = 2; 		/* current and next */
		ivp->iv_mod_shr_p = current;

		current->msv_next = current->msv_prev = next;
		next->msv_next = next->msv_prev = current;
		IV_UNLOCK();
		return(0);
	}

	/*
	 * If we get here, the interrupt vector is shared multiply already
	 * (more than two).
	 */
	{
		struct mod_shr_v  *head;
		
		head = ivp->iv_mod_shr_p;
		ASSERT(ivp->iv_mod_shr_cnt >= 2);
		ASSERT(head != NULL);

		/*
		 * Assign the new handler/idata to the slot and
		 * increment the packet's count.
		 */
		next->msv_handler = ihp;
		next->msv_idata = idata;
		ivp->iv_mod_shr_cnt++;
		/*
		 * Make up the link
		 */
		next->msv_next = head->msv_next;
		next->msv_prev = head;
		(head->msv_next)->msv_prev = next;
		head->msv_next = next;
		ivp->iv_mod_shr_p = next;
		IV_UNLOCK();
		/* did not use current */
		kmem_free(current, sizeof(struct mod_shr_v)); 
		return(0);
	}
}

/*
 * int mod_remove_intr(struct intr_info *iip, void (*ihp)())
 *	Remove the interrupt handler ihp from the vector defined by iip.
 *
 * Calling/Exit State:
 *	None.
 */
int
mod_remove_intr(struct intr_info *iip, int (*ihp)())
{
	uint_t			iv;
	int			cpu;
	ms_cpu_t		cpu_dist;
	struct intr_vect_t	*ivp;
	struct mod_shr_v	*svp;
	int			(*handler)();
	void			*idata;

	iv = iip->ivect_no;
	if (iv > os_islot_max)
		return(EINVAL);

	if (iip->itype == 0)
  		return(EINVAL);

	ivp = &intr_vect[iv];

	moddebug(cmn_err(CE_CONT, "!MOD: mod_remove_intr(): %d\n", iv));

	(void)IV_LOCK();

	/*
	 * If the parameters don't match, don't do anything.
	 * This prevents us from detaching an interrupt we didn't attach.
	 */
	if (iip->int_pri < ivp->iv_intpri || iip->itype != ivp->iv_itype) {
		IV_UNLOCK();
		return(ENOENT);
	}

	if (ivp->iv_ivect != mod_shr_intn) {
		/*
		 * The interrupt is not shared,
		 * reset intr_vect[].
		 */

		/*
		 * If we didn't find our handler, don't do anything.  This
		 * prevents us from detaching an interrupt we didn't attach.
		 */
		if (ivp->iv_ivect != ihp) {
			IV_UNLOCK();
			return(ENOENT);
		}

		/*
		 * Disable the interrupt vector.
		 */
		DISABLE();
		ms_intr_detach(&ivp->iv_idt);
		ivp->iv_ivect = intnull;
		ENABLE();
		IV_UNLOCK();
		return(0);
	}

	/*
	 * If we get here, the interrupt vector is shared.
	 */
	ASSERT(ivp->iv_mod_shr_cnt >= 2);
	
	for (svp = ivp->iv_mod_shr_p; ; svp = svp->msv_next) {
		struct mod_shr_v	*next;

		next = svp->msv_next;
		if (svp->msv_handler == ihp &&
		    svp->msv_idata == iip->int_idata) {
			break;
		}
		
		if (next == ivp->iv_mod_shr_p) {
			IV_UNLOCK();
			return(ENOENT);
		}
	
	}

	cpu = iip->int_cpu;
	if (cpu == -2)
		cpu = default_bindcpu;
	if (cpu == -1 && !iip->int_mp)
		cpu = 0;
	cpu_dist = (cpu == -1 ? MS_CPU_ANY : cpu);

	ASSERT(ivp->iv_bindcount == 0 && ivp->iv_idt.msi_cpu_dist == MS_CPU_ANY 
	    || ivp->iv_bindcount > 0 && ivp->iv_idt.msi_cpu_dist != MS_CPU_ANY);
	ASSERT(ivp->iv_bindcount > 0 || cpu_dist == MS_CPU_ANY);

	ivp->iv_bindcount -= (cpu_dist == MS_CPU_ANY ? 0 : 1);

	if (ivp->iv_bindcount == 0 && ivp->iv_idt.msi_cpu_dist != MS_CPU_ANY) {
		DISABLE();
		ms_intr_detach(&ivp->iv_idt);	/* XXX should be unnecessary */
		ivp->iv_idt.msi_cpu_dist = MS_CPU_ANY;
		if (!ms_intr_attach(&ivp->iv_idt)) {
			ENABLE();
			ivp->iv_ivect = intnull;
			IV_UNLOCK();
			cmn_err(CE_NOTE, "Interrupt re-attachment failed;"
					 " interrupts may be lost.");
			return(0);
		}
		ENABLE();
	}

	/*
	 * At this point we should have found the handler; svp holds the
	 * element. 
	 */
	if (svp == ivp->iv_mod_shr_p) {
		/* have to change iv_mod_shr_p because it's gone */
		ivp->iv_mod_shr_p = svp->msv_next;
	}
		
	(svp->msv_prev)->msv_next = svp->msv_next; /* disconnect links */
	(svp->msv_next)->msv_prev = svp->msv_prev;
	kmem_free(svp, sizeof(struct mod_shr_v));
	ivp->iv_mod_shr_cnt--;
	/*
	 * If there is only one interrupt handler left in the list,
	 * call it directly through intr_vect[].
	 */
	if (ivp->iv_mod_shr_cnt == 1) {
		ivp->iv_ivect = (ivp->iv_mod_shr_p)->msv_handler;
		ivp->iv_idata = (ivp->iv_mod_shr_p)->msv_idata;
		kmem_free(ivp->iv_mod_shr_p, sizeof(struct mod_shr_v));
		ivp->iv_mod_shr_p = NULL;
		ivp->iv_mod_shr_cnt = 0;
	}

	IV_UNLOCK();
	return(0);
}


#ifdef MERGE386

struct asyc_ipl {
	struct asyc_ipl *next;
	int iv;
	int refcnt;
};

static struct asyc_ipl *asyc_ipl_list = NULL;

void
downgrade_asyc_ipl(int iv, int (*ihp)())
{
	struct asyc_ipl		*new, *p, *pp;
	struct	mod_shr_v	*svp;
	struct intr_vect_t	*ivp;
	int			(**sihp)();
	int 			count = 0, foreign = 0;

	new = (struct asyc_ipl *)kmem_alloc(sizeof(struct asyc_ipl), KM_SLEEP);

	(void)IV_LOCK();

	ivp = &intr_vect[iv];

	if (ivp->iv_ivect == mod_shr_intn) {
		for (svp = ivp->iv_mod_shr_p; svp; svp = svp->msv_next)
			if (svp->msv_handler == ihp)
				count++;
			else
				foreign++;
	} else if (ivp->iv_ivect == ihp)
		count++;

	if (count == 0) {
		IV_UNLOCK();
		cmn_err(CE_WARN, "downgrade_async_ipl: no proper handler on IRQ %d, not downgrading\n", iv);
		kmem_free(new, sizeof(struct asyc_ipl));
		return;
	}

	if (foreign)
		cmn_err(CE_WARN, "downgrade_async_ipl: multiple handlers on IRQ %d, downgrading to PLHI anyway\n", iv);

	for (pp = NULL, p = asyc_ipl_list; p; pp = p, p = p->next)
		if (p->iv == iv)
			break;

	if (p) {
		ASSERT(p->refcnt >= 0);
		p->refcnt++;
	} else {
		new->iv = iv;
		new->refcnt = 1;
		new->next = NULL;

		if (pp)
			pp->next = new;
		else
			asyc_ipl_list = new;

		p = new;
		new = NULL;
	}

	if (p->refcnt == 1) {
		ASSERT(ivp->iv_intpri == PLMAX);
		DISABLE();
		ms_intr_detach(&ivp->iv_idt);
		ivp->iv_intpri = PLHI;
		ivp->iv_idt.msi_order = MSI_ORDER(ivp->iv_intpri);
		ms_intr_attach(&ivp->iv_idt);
		ENABLE();
	}

	IV_UNLOCK();

	if (new)
		kmem_free(new, sizeof(struct asyc_ipl));
}


void
restore_asyc_ipl(int iv, int (*ihp)())
{
	struct asyc_ipl		*p;
	struct	mod_shr_v	*svp;
	struct intr_vect_t	*ivp;
	int			(**sihp)();

	(void)IV_LOCK();

	ivp = &intr_vect[iv];

	if (ivp->iv_ivect == mod_shr_intn) {
		for (svp = ivp->iv_mod_shr_p; svp; svp = svp->msv_next)
			if (ihp != svp->msv_handler) {
				IV_UNLOCK();
				cmn_err(CE_WARN, "restore_asyc_ipl: can't restore PLMAX on IRQ %d\n", iv);
				return;		/* failure */
			}
	} else {
		ASSERT(ivp->iv_ivect == ihp);
	}

	for (p = asyc_ipl_list; p; p = p->next)
		if (p->iv == iv)
			break;

	ASSERT(p && p->refcnt > 0);
	p->refcnt--;

	if (p->refcnt == 0) {
		DISABLE();
		ms_intr_detach(&ivp->iv_idt);
		ivp->iv_intpri = PLMAX;
		ivp->iv_idt.msi_order = MSI_ORDER(ivp->iv_intpri);
		(void) ms_intr_attach(&ivp->iv_idt);
		ENABLE();
	}

	IV_UNLOCK();
}
#endif


#ifdef DEBUG
void
mod_shr_print(struct intr_vect_t *ivp)
{

	struct mod_shr_v	*svp, *next;
	int i;

	cmn_err(CE_CONT, "mod_shr_print: cnt = %d\n", ivp->iv_mod_shr_cnt);
	for (svp = ivp->iv_mod_shr_p, i = 0; ; svp = svp->msv_next) {
		cmn_err(CE_CONT, "element[%d]: handler = %x, idata = %x\n",
			i, svp->msv_handler, svp->msv_idata);
		next = svp->msv_next;
		i++;
		
		if (next == ivp->iv_mod_shr_p)
			break;
	}

}

int
mod_shr_hanlder1(void *idata)
{
	cmn_err(CE_CONT, "mod_shr_hanlder1: Called\n");
	return ISTAT_ASSUMED;
}

int mod_idata1;
int mod_idata2;

void
mod_shr_add_test1(int vec)
{

	struct intr_info iip;

	iip.ivect_no = vec;
	iip.itype = 4;
	iip.int_cpu = 0; 
	iip.int_pri = 4;
	iip.int_mp = 0;

	mod_add_intr(&iip, &mod_shr_hanlder1, (void *) &mod_idata1);
}

void
mod_shr_remove_test1(int vec)
{
	struct intr_info iip;

	iip.ivect_no = vec;
	iip.itype = 4;
	iip.int_cpu = 0; 
	iip.int_pri = 4;
	iip.int_mp = 0;
	iip.int_idata = &mod_idata1;

	mod_remove_intr(&iip, &mod_shr_hanlder1);
}

#endif




