#ident "@(#)mdiioctl.c	29.1"
#ident "$Header$"

/*
 *	  Copyright (C) The Santa Cruz Operation, 1993-1996.
 *	  This Module contains Proprietary Information of
 *	  The Santa Cruz Operation and should be treated
 *	  as Confidential.
 */

#ifdef  _KERNEL_HEADERS

#include <io/nd/dlpi/include.h>

#else

#include "include.h"

#endif

/******************************************************************************
 *
 * MDI ioctl functions
 *
 ******************************************************************************/

/* DDI8 CFG_SUSPEND note:  since ioctls don't *necessarily* impact hardware,
 *                         we allow a direct putnext without checking
 *                         cp->issuspended.  The MDI driver must NAK with
 *                         EAGAIN any ioctl that needs to poke the hardware 
 *                         in order to answer the ioctl.
 *                         For this reason we allow a blind putnext to
 *                         down_queue without checking issuspended first
 */
STATIC void
mdi_send_MACIOC_generic(per_sap_info_t *sp, int ioctl_cmd, int size,
			void (*handler)(), mblk_t *datamp)
{
	per_card_info_t *cp = sp->card_info;
	mblk_t		*iocmp;
	struct iocblk	*ip;

	cp->ioctl_sap		= sp;
	cp->ioctl_handler	= handler;

	if (!(iocmp = allocb(sizeof(struct iocblk), BPRI_LO)) ) {
		cp->ioctl_handler(sp, (mblk_t *)0);
		return;
	}

	iocmp->b_datap->db_type = M_IOCTL;
	iocmp->b_cont = datamp;

	ip = (struct iocblk *)iocmp->b_rptr;
	bzero(ip, sizeof(struct iocblk));
	iocmp->b_wptr += sizeof(struct iocblk);

	ip->ioc_cmd	= ioctl_cmd;
	ip->ioc_cr	= sp->crp;
	ip->ioc_id	= 1;
	ip->ioc_count	= size;

	DLPI_PRINTF100(("MDI: Sending ioctl(0x%x) to MDI Driver\n", ip->ioc_cmd));
	dlpi_putnext_down_queue(cp,iocmp); /* Ignore flow control */
}

void
mdi_send_MACIOC_dup(per_sap_info_t *sp, int ioctl_cmd, int size,
			void (*handler)(), mblk_t *datamp)
{
	mblk_t *x;

	if (datamp) {
		if (!(x = dupmsg(datamp))) {
			(*handler)(sp, (mblk_t *)0);
			return;
		}
	} else {
		x = (mblk_t *)0;
	}
	mdi_send_MACIOC_generic(sp, ioctl_cmd, size, handler, x);
}

STATIC void
mdi_send_MACIOC_alloc(per_sap_info_t *sp, int ioctl_cmd, int size,
			void (*handler)())
{
	mblk_t *x;

	if (!(x = allocb(size, BPRI_MED))) {
		(*handler)(sp, (mblk_t *)0);
		return;
	}
	bzero(x->b_rptr, size);
	x->b_wptr += size;
	mdi_send_MACIOC_generic(sp, ioctl_cmd, size, handler, x);
}

void
mdi_send_MACIOC_copy(per_sap_info_t *sp, int ioctl_cmd, int size,
			void (*handler)(), unchar *data)
{
	mblk_t *x;

	if (!(x = allocb(size, BPRI_MED))) {
		(*handler)(sp, (mblk_t *)0);
		return;
	}
	bcopy(data, x->b_wptr, size);
	x->b_wptr += size;
	mdi_send_MACIOC_generic(sp, ioctl_cmd, size, handler, x);
}

void
mdi_send_MACIOC_cp_queue(per_sap_info_t *sp, int ioctl_cmd, int size,
			void (*handler)(), unchar *data)
{
	mblk_t *datamp, *iocmp, *cp_mp, *tmp;
	struct iocblk	*ip;
	cp_ioctl_blk_t	*cp_iblk;
	pl_t		s;
	per_card_info_t *cp = sp->card_info;

	if (cp == NULL)
		return;

	if (!(datamp = allocb(size, BPRI_MED))) {
		(*handler)(sp, (mblk_t *)0);
		return;
	}
	bcopy(data, datamp->b_wptr, size);
	datamp->b_wptr += size;

	if (!(iocmp = allocb(sizeof(struct iocblk), BPRI_LO)) ) {
		cp->ioctl_handler(sp, (mblk_t *)0);
		freemsg(datamp);
		return;
	}

	iocmp->b_datap->db_type = M_IOCTL;
	iocmp->b_cont = datamp;

	ip = (struct iocblk *)iocmp->b_rptr;
	bzero(ip, sizeof(struct iocblk));
	iocmp->b_wptr += sizeof(struct iocblk);

	ip->ioc_cmd	= ioctl_cmd;
	ip->ioc_cr	= sp->crp;
	ip->ioc_id	= 1;
	ip->ioc_count	= size;

	s = LOCK_CARDINFO(cp);
	if (cp->dlpi_iocblk) { /* ioctl in progress, queue ioctl */
		if (!(cp_mp = allocb(sizeof(cp_ioctl_blk_t), BPRI_LO)) ) {
			UNLOCK_CARDINFO(cp, s);
			cp->ioctl_handler(sp, (mblk_t *)0);
			freemsg(iocmp);
			return;
		}
		cp_iblk = (cp_ioctl_blk_t *)cp_mp->b_rptr;
		cp_mp->b_wptr += sizeof(cp_ioctl_blk_t);
		cp_iblk->ioctl_sap = sp;
		cp_iblk->ioctl_handler = handler;
		cp_mp->b_cont = iocmp;
		cp_mp->b_next = NULL;

		if (cp->ioctl_queue == NULL) /* queue empty, queue at head */
			cp->ioctl_queue = cp_mp;
		else { /* queue at end */
			tmp = cp->ioctl_queue;
			while (tmp->b_next != NULL)
				tmp = tmp->b_next;
			tmp->b_next = cp_mp;
		}

		UNLOCK_CARDINFO(cp, s);

	} else { /* issue ioctl now */
		cp->dlpi_iocblk = iocmp;
		UNLOCK_CARDINFO(cp, s);

		cp->ioctl_sap = sp;
		cp->ioctl_handler = handler;
		DLPI_PRINTF100(("MDI: Sending ioctl(0x%x) to MDI Driver\n", ip->ioc_cmd));
		dlpi_putnext_down_queue(cp,iocmp); /* Ignore flow control */
	}

}

void
mdi_send_MACIOC_SETADDR(per_sap_info_t *sp, void (*handler)(), unchar *addr)
{
	mdi_send_MACIOC_copy(sp, MACIOC_SETADDR, MAC_ADDR_SZ, handler, addr);
}

void
mdi_send_MACIOC_GETRADDR(per_sap_info_t *sp, void (*handler)())
{
	mdi_send_MACIOC_alloc(sp, MACIOC_GETRADDR, MAC_ADDR_SZ, handler);
}

void
mdi_send_MACIOC_GETADDR(per_sap_info_t *sp, void (*handler)())
{
	mdi_send_MACIOC_alloc(sp, MACIOC_GETADDR, MAC_ADDR_SZ, handler);
}

void
mdi_send_MACIOC_GETSTAT(per_sap_info_t *sp, void (*handler)())
{
	mdi_send_MACIOC_alloc(sp,
			MACIOC_GETSTAT,
			sp && sp->card_info && sp->card_info->media_specific
			? sp->card_info->media_specific->stats_sz : 0,
			handler);
}

void
mdi_send_MACIOC_SETMCA(per_sap_info_t *sp, void (*handler)(), unchar *addr)
{
	mdi_send_MACIOC_copy(sp, MACIOC_SETMCA, MAC_ADDR_SZ, handler, addr);
}

void
mdi_send_MACIOC_DELMCA(per_sap_info_t *sp, void (*handler)(), unchar *addr)
{
	mdi_send_MACIOC_copy(sp, MACIOC_DELMCA, MAC_ADDR_SZ, handler, addr);
}

void
mdi_send_MACIOC_CLRSTAT(per_sap_info_t *sp, void (*handler)())
{
	mdi_send_MACIOC_generic(sp, MACIOC_CLRSTAT, 0, handler, (mblk_t *)0);
}

void
mdi_send_MACIOC_SETALLMCA(per_sap_info_t *sp, void (*handler)())
{
	mdi_send_MACIOC_generic(sp, MACIOC_SETALLMCA, 0, handler, (mblk_t *)0);
}

void
mdi_send_MACIOC_DELALLMCA(per_sap_info_t *sp, void (*handler)())
{
	mdi_send_MACIOC_generic(sp, MACIOC_DELALLMCA, 0, handler, (mblk_t *)0);
}

