#ident	"@(#)kern-i386:proc/iobitmap.c	1.14.4.1"
#ident	"$Header$"

#define _DDI_C

#include <io/conf.h>
#include <mem/kmem.h>
#include <proc/iobitmap.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/seg.h>
#include <proc/tss.h>
#include <svc/errno.h>
#include <util/bitmasks.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>
#include <io/ddi.h>

STATIC void iobitmap_sync_l(void);


/*
 * int
 * iobitmapctl(iob_request_t iob_rqt, ushort_t ports[])
 *	Enable/disable/check user access to I/O ports.
 *
 * Calling/Exit State:
 *	ports is a null-terminated array of I/O port numbers
 *
 *	iob_rqt is one of:
 *		IOB_ENABLE	Enable user access to the ports
 *				Return 0 if successful, errno otherwise.
 *		IOB_DISABLE	Disable user access to the ports
 *				Return 0 if successful, errno otherwise.
 *		IOB_CHECK	Check user access to the ports
 *				returns non-zero if all the ports have
 *				access enabled
 *
 *	Access control is on a per-process basis.  The iobitmapctl()
 *	works with the current process.
 *
 *	Must not be called from interrupt level.
 *	This function can block, so no locks may be held.
 */
int
iobitmapctl(iob_request_t iob_rqt, ushort_t ports[])
{
	proc_t *procp = u.u_procp;
	lwp_t *lwpp = u.u_lwpp;
	lwp_t *lwpp2;
	struct stss *tssp, *ntssp;
	ushort_t portno, *portp;
	uint_t bidx;
	uint_t allocsz;
	int retval = 0, error = 0;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (!SINGLE_THREADED())
		SLEEP_LOCK(&procp->p_tsslock, PRIMED);

	tssp = procp->p_tssp;

	switch (iob_rqt) {
	case IOB_ENABLE:
		/* Find highest port number */
		portno = 0;
		for (portp = ports; *portp != 0; portp++) {
			if (*portp > portno)
				portno = *portp;
		}
		/*
		 * Compute needed TSS size.
		 *
		 * To get the allocation size (allocsz),
		 * we round up (bidx + 1) to a multiple of 256.
		 * There is code below which depends on it
		 * being a multiple of sizeof(uint_t); 256
		 * keeps us from reallocating too often.
		 * The extra byte in (bidx + 1) is because
		 * the hardware requires a terminating byte
		 * (of all 1's).
		 */
		bidx = sizeof(struct stss) + (portno >> 3);
		allocsz = ((bidx + 256) & ~255);
		/*
		 * If no private TSS or it's not big enough,
		 * create a new one.
		 */
		if (tssp == NULL || allocsz > tssp->st_allocsz) {
			uint_t bitsz = allocsz - sizeof(struct stss);

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
			procp->p_tssp = tssp = ntssp;
		}
		while ((portno = *ports++) != 0) {
			bidx = sizeof(struct stss) + (portno >> 3);
			ASSERT(bidx < tssp->st_allocsz - 1);
		        if ((u.u_cfgp != NULL) &&
			    (CFG_CPU(u.u_cfgp) != -1) &&
			    (((uchar_t *)tssp)[bidx] & (1 << (portno & 7)))) {
				if (procp->p_iobcount == 0) {
					error = bindproc(
						   CFG_CPU(u.u_cfgp), MYSELF);
					if (error) 
						return error;
				}
				procp->p_iobcount++;
			}
			((uchar_t *)tssp)[bidx] &= ~(1 << (portno & 7));
		}
		break;
	case IOB_DISABLE:
		if (tssp == NULL)
			break;
		while ((portno = *ports++) != 0) {
			bidx = sizeof(struct stss) + (portno >> 3);
			if (bidx >= tssp->st_allocsz)
				continue;

		        if ((u.u_cfgp != NULL) &&
			    (CFG_CPU(u.u_cfgp) != -1) &&
			    !(((uchar_t *)tssp)[bidx] & (1 << (portno & 7)))) {
			        if (--procp->p_iobcount == 0)
				       unbindproc(MYSELF);
			}
			((uchar_t *)tssp)[bidx] |= (1 << (portno & 7));
		}
		break;
	case IOB_CHECK:
		if (tssp == NULL)
			break;
		retval = 1;	/* all ports enabled */
		while ((portno = *ports++) != 0) {
			bidx = sizeof(struct stss) + (portno >> 3);
			if (bidx >= tssp->st_allocsz ||
			    ((uchar_t *)tssp)[bidx] & (1 << (portno & 7))) {
				retval = 0;	/* port not enabled */
				break;
			}
		}
		break;
	}

	ASSERT(tssp || lwpp->l_tssp == NULL);

	if (tssp && iob_rqt != IOB_CHECK) {
		/*
		 * A change was made; advance the generation count.
		 */
		tssp->st_gencnt++;

		/*
		 * Invalidate old I/O bitmaps in any other
		 * LWPs, so accesses from the other LWPs fault
		 * and they sync up to the new TSS.
		 */
		if (!SINGLE_THREADED()) {
			(void) LOCK(&procp->p_mutex, PLHI);
			for (lwpp2 = procp->p_lwpp; lwpp2 != NULL;
			     lwpp2 = lwpp2->l_next) {
				if (lwpp2->l_tssp && lwpp2->l_tssp != tssp) {
					lwpp2->l_tssp->st_tss.t_bitmapbase =
						    TSS_NO_BITMAP;
				}
			}
			UNLOCK(&procp->p_mutex, PLBASE);
		}

		/*
		 * Bring our LWP up to date.
		 */
		if (lwpp->l_tssp != tssp)
			iobitmap_sync_l();
	}

	if (!SINGLE_THREADED())
		SLEEP_UNLOCK(&procp->p_tsslock);

	return retval;
}


/*
 * void
 * iobitmap_reset(void)
 *	Reset to the default I/O bitmap (none).
 *
 * Calling/Exit State:
 *	Caller guarantees there are no other LWPs in the process.
 *	No locks are acquired or required.
 */
void
iobitmap_reset(void)
{
	struct stss *tssp;

	if ((tssp = u.u_lwpp->l_tssp) != NULL) {
		DISABLE_PRMPT();
		/* clear KTSSSEL busy bit */
		setdscracc1(&u.u_dt_infop[DT_GDT]->di_table[seltoi(KTSSSEL)],
			    TSS3_KACC1);
		/* reload KTSSSEL (global TSS) */
		loadtr(KTSSSEL);
		/* free up private TSS */
		u.u_lwpp->l_tssp = NULL;
		/* we aren't special anymore */
		l.special_lwp = (u.u_lwpp->l_special &= ~SPECF_PRIVTSS);
		ENABLE_PRMPT();

		ASSERT(tssp->st_refcnt >= 1);
		if (--tssp->st_refcnt == 0) {
			ASSERT(u.u_procp->p_tssp != tssp);
			kmem_free(tssp, tssp->st_allocsz);
		}
#ifdef DEBUG
		else {
			ASSERT(tssp->st_refcnt == 1);
			ASSERT(tssp == u.u_procp->p_tssp);
		}
#endif
	}
	if ((tssp = u.u_procp->p_tssp) != NULL) {
		ASSERT(tssp->st_refcnt == 1);
		kmem_free(tssp, tssp->st_allocsz);
		u.u_procp->p_tssp = NULL;
	}
	if (u.u_procp->p_iobcount != 0) {
	        unbindproc(MYSELF);
		u.u_procp->p_iobcount = 0;
        }
}


/*
 * boolean_t
 * iobitmap_sync(void)
 *	Sync up this LWP's I/O bitmap to the current process version.
 *
 * Calling/Exit State:
 *	No locks required on entry.
 *
 *	Returns true if the bitmap was out of sync (and has been sync'ed).
 */
boolean_t
iobitmap_sync(void)
{
	ASSERT(u.u_procp != NULL);
	ASSERT(u.u_lwpp != NULL);

	SLEEP_LOCK(&u.u_procp->p_tsslock, PRIMED);
	if (u.u_procp->p_tssp != NULL &&
	    (u.u_lwpp->l_tssp == NULL ||
	     u.u_lwpp->l_tssp->st_gencnt != u.u_procp->p_tssp->st_gencnt)) {
		iobitmap_sync_l();
		SLEEP_UNLOCK(&u.u_procp->p_tsslock);
		return B_TRUE;
	}
	SLEEP_UNLOCK(&u.u_procp->p_tsslock);
	return B_FALSE;
}


/*
 * STATIC void
 * iobitmap_sync_l(void)
 *	Sync up this LWP's I/O bitmap to the current process version.
 *
 * Calling/Exit State:
 *	The p_tsslock is held on entry and remains held on exit.
 */
STATIC void
iobitmap_sync_l(void)
{
	lwp_t *lwpp = u.u_lwpp;
	struct stss *ptssp = u.u_procp->p_tssp;
	struct stss *ltssp;
	uint_t allocsz, bitsz;

	ASSERT(ptssp != NULL);
	ASSERT(lwpp->l_tssp == NULL ||
	       lwpp->l_tssp->st_gencnt != ptssp->st_gencnt);
	ASSERT(lwpp->l_tssp == NULL ||
	       lwpp->l_tssp->st_allocsz <= ptssp->st_allocsz);
	ASSERT(lwpp->l_tssp == NULL ||
	       lwpp->l_tssp->st_refcnt == 1);
	ASSERT(lwpp->l_tssp != ptssp);

	ltssp = lwpp->l_tssp;

	if (ptssp->st_refcnt == 1) {
		/*
		 * Nobody else is using the reference copy;
		 * grab it for our use instead of making another copy.
		 */
		if (ltssp != NULL)
			kmem_free(ltssp, ltssp->st_allocsz);

		/* Initialize kernel stack pointer in TSS */
		ptssp->st_tss.t_esp0 = (ulong_t)&u - KSTACK_RESERVE;
		ASSERT(ptssp->st_tss.t_ss0 == KDSSEL);

		/*
		 * Set pointer to new TSS; must come after fully initialized;
		 * while we're at it, increment the reference count.
		 */
		(lwpp->l_tssp = ptssp)->st_refcnt++;

		DISABLE_PRMPT();
		/* Load descriptor into GDT */
		u.u_dt_infop[DT_GDT]->di_table[seltoi(KPRIVTSSSEL)] =
							ptssp->st_tssdesc;
		/* Set the Task Register */
		loadtr(KPRIVTSSSEL);
		l.special_lwp = (lwpp->l_special |= SPECF_PRIVTSS);
		ENABLE_PRMPT();
		return;
	}

	bitsz = (allocsz = ptssp->st_allocsz) - sizeof(struct stss);

	/*
	 * We need to make a copy of the reference bitmap (in ptssp).
	 * First, make sure we have enough space allocated.
	 */
	if (ltssp == NULL || ltssp->st_allocsz < allocsz) {
		if (ltssp != NULL)
			kmem_free(ltssp, ltssp->st_allocsz);
		ltssp = kmem_alloc(allocsz, KM_SLEEP);
		ltssp->st_allocsz = (ushort_t)allocsz;
		ltssp->st_tss = ptssp->st_tss;
		ltssp->st_refcnt = 1;

		/* Create a descriptor for this TSS */
		if (bitsz != 0) {
			BUILD_SYS_DESC(&ltssp->st_tssdesc, &ltssp->st_tss,
				       sizeof(struct tss386) + bitsz - 1,
				       TSS3_KACC1, TSS_ACC2);
		} else {
			BUILD_SYS_DESC(&ltssp->st_tssdesc, &ltssp->st_tss,
				       sizeof(struct tss386),
				       TSS3_KACC1, TSS_ACC2);
		}

		/* Initialize kernel stack pointer in TSS */
		ltssp->st_tss.t_esp0 = (ulong_t)&u - KSTACK_RESERVE;
		ASSERT(ltssp->st_tss.t_ss0 == KDSSEL);

		/* Set pointer to new TSS; must come after fully initialized */
		lwpp->l_tssp = ltssp;

		DISABLE_PRMPT();
		/* Load descriptor into GDT */
		u.u_dt_infop[DT_GDT]->di_table[seltoi(KPRIVTSSSEL)] =
							ltssp->st_tssdesc;
		/* Set the Task Register */
		loadtr(KPRIVTSSSEL);
		l.special_lwp = (u.u_lwpp->l_special |= SPECF_PRIVTSS);
		ENABLE_PRMPT();
	} else {
		/* Copy ring 1 and 2 stack pointers */
		ltssp->st_tss.t_stkbase[1] = ptssp->st_tss.t_stkbase[1];
		ltssp->st_tss.t_stkbase[2] = ptssp->st_tss.t_stkbase[2];
	}

	/* Copy the I/O bitmap */
	if (bitsz != 0) {
		bcopy(ptssp + 1, ltssp + 1, bitsz);
		ltssp->st_tss.t_bitmapbase = sizeof(struct tss386);
	}

	/* Now we're up-to-date */
	ltssp->st_gencnt = ptssp->st_gencnt;
}


/*
 * int
 * set_special_sp(ushort_t sel, ulong_t esp)
 *	Set stack pointer and selector for privilege ring 1 or 2.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
set_special_sp(ushort_t sel, ulong_t esp)
{
	uint_t ring;
	struct stss *tssp;
	lwp_t *lwpp2;

	if ((ring = (sel & RPL_MASK)) != 1 && ring != 2)
		return EINVAL;

	if (!SINGLE_THREADED())
		SLEEP_LOCK(&u.u_procp->p_tsslock, PRIMED);

	tssp = u.u_procp->p_tssp;

	/*
	 * If no private TSS yet, create one.
	 */
	if (tssp == NULL) {
		tssp = kmem_alloc(sizeof(struct stss), KM_SLEEP);
		tssp->st_tss = l.tss;
		tssp->st_allocsz = (ushort_t)sizeof(struct stss);
		tssp->st_refcnt = 1;
		tssp->st_gencnt = 0;

		/* Create a descriptor for this TSS */
		BUILD_SYS_DESC(&tssp->st_tssdesc, &tssp->st_tss,
			       sizeof(struct tss386),
			       TSS3_KACC1, TSS_ACC2);

		/* Initialize kernel stack pointer in TSS */
		tssp->st_tss.t_esp0 = (ulong_t)&u - KSTACK_RESERVE;
		ASSERT(tssp->st_tss.t_ss0 == KDSSEL);

		/* Install the new TSS */
		u.u_procp->p_tssp = tssp;
	}

	/*
	 * Set the stack selector and pointer for the desired ring.
	 */
	tssp->st_tss.t_stkbase[ring].t_esp = esp;
	tssp->st_tss.t_stkbase[ring].t_ss = sel;

	/*
	 * Copy to other LWPs, if any.
	 */
	if (!SINGLE_THREADED()) {
		(void) LOCK(&u.u_procp->p_mutex, PLHI);
		for (lwpp2 = u.u_procp->p_lwpp; lwpp2 != NULL;
		     lwpp2 = lwpp2->l_next) {
			if (lwpp2->l_tssp && lwpp2->l_tssp != tssp) {
				lwpp2->l_tssp->st_tss.t_stkbase[ring] =
						tssp->st_tss.t_stkbase[ring];
			}
		}
		UNLOCK(&u.u_procp->p_mutex, PLBASE);
	}

	/*
	 * Bring our LWP up to date.
	 */
	if (u.u_lwpp->l_tssp != tssp)
		iobitmap_sync_l();

	if (!SINGLE_THREADED())
		SLEEP_UNLOCK(&u.u_procp->p_tsslock);

	return 0;
}
