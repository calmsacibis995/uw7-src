/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ihvkit:net/dlpi_ether_2.x/el16hrdw.c	1.1"
#ident	"$Header$"

/*
 *  This file contains all of the hardware dependent code for
 *  the 3c507, 3c523.
 *  It is the companion file to ../../io/dlpi_ether.c
 */

#ifdef  _KERNEL_HEADERS

#include <io/dlpi_ether/dlpi_ether.h>
#ifdef ESMP
#include <io/dlpi_ether/el16/dlpi_el16.h>
#include <io/dlpi_ether/el16/el16.h>
#else
#include <io/dlpi_ether/dlpi_el16.h>
#include <io/dlpi_ether/el16.h>
#endif
#ifdef ESMP
#include <util/ksynch.h>
#endif
#include <mem/immu.h>
#include <mem/kmem.h>
#include <net/dlpi.h>
#ifdef ESMP
#include <net/inet/if.h>
#else
#include <net/tcpip/if.h>
#endif
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#include <io/ddi_i386at.h>
#include <io/ddi.h>

#else

#include <sys/dlpi_ether.h>
#include <sys/dlpi_el16.h>
#include <sys/el16.h>
#ifdef ESMP
#include <sys/ksynch.h>
#endif
#include <sys/immu.h>
#include <sys/kmem.h>
#include <sys/dlpi.h>
#include <net/if.h>
#include <sys/errno.h>
#include <sys/cmn_err.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/debug.h>
#include <sys/ddi_i386at.h>
#include <sys/ddi.h>

#endif /* _KERNEL_HEADERS */


#define BCOPY(from, to, len)	bcopy((caddr_t)(from), (caddr_t)(to), \
								(size_t)(len))
#define BCMP(s1, s2, len)	bcmp((char *)(s1), (char *)(s2), \
								(size_t)(len))
#define	MB_SIZE(p)		((p)->b_wptr - (p)->b_rptr)

extern	DL_bdconfig_t	*DLconfig;
extern	DL_sap_t	*DLsaps;
/*extern	struct ifstats *DLifstats;*/

extern	int	el16boards;
extern	int	el16intr_to_index[];
extern	int	el16timer_id;
extern	int	el16bus_p;

extern	int	el16init_586(), el16config_586(), el16ack_586();
extern	int	el16recv(), el16wait_scb(), el16ia_setup_586();

STATIC	void	el16tx_done( DL_bdconfig_t * ),
		el16sched( DL_bdconfig_t * ),
		el16chk_ring( DL_bdconfig_t * ),
		el16ru_restart( DL_bdconfig_t * );

STATIC	int	el16set_multicast( DL_bdconfig_t * );

#ifdef ESMP
int	el16devflag = D_MP;     /* Multi-threaded */
#endif

#ifdef DEBUG
struct	debug	el16debug = { 0, 0, 0, 0, 0};
#endif


/* ARGSUSED */
/* 
 * el16bdspecclose(queue_t *q)
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 * Description :
 *      el16bdspecclose is called from DLclose->dlpi_ether.c
 */
void
el16bdspecclose( queue_t *q )
{
	return;
}

/* ARGSUSED */
/* 
 * el16bdspecioctl(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 * Description :
 *       el16bdspecioctl is called from DLioctl in dlpi_ether.c
 *
 */
void
el16bdspecioctl(queue_t *q, mblk_t *mp)
{
	/* LINTED pointer alignment */
	struct iocblk *ioctl_req = (struct iocblk *)mp->b_rptr;

	ioctl_req->ioc_error = EINVAL;
	ioctl_req->ioc_count = 0;
	mp->b_datap->db_type = M_IOCNAK;
	qreply(q, mp);
}

/* ARGSUSED */
/* 
 *  el16xmit_packet()
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 * Description :
 *  This routine is called from DLunitdata_req() and el16tx_done().
 *  It assumes we are at STREAMS spl interrupt level.
 *
 */
int
el16xmit_packet ( DL_bdconfig_t *bd, mblk_t *mp )
{
	/* LINTED pointer alignment */
	bdd_t *bdd = (bdd_t *) bd->bd_dependent1;
	/* LINTED pointer alignment */
	DL_ether_hdr_t	*hdr = (DL_ether_hdr_t *)mp->b_rptr;

	scb_t	*p_scb  = bdd->scb;
	tbd_t	*p_tbd;
	cmd_t	*p_cmd;
	char	*p_txb;
	mblk_t	*mp_tmp;

	pack_ushort_t partial_short;
	int	byte_left_over = 0;

	int	start_cu = 0;
	int	tx_size  = 0;
	int	msg_size = 0;
	int	ctrl_reg = bd->io_start + CTRL_REG_OFST;
	int	opri;


	/*
	 * the bdd->head_cmd always points to the first unacknowledged cmd.
	 * the bdd->tail_cmd always points to the current end of CBL.
	 */

	/*
	 * If ring buffer is empty
	 */
	if (bdd->head_cmd == NULL) {
		bdd->head_cmd = bdd->tail_cmd = bdd->ring_buff;
		start_cu = 1;
	}

	/*
	 * else there's at least one command pending in ring.
	 * Note that: we don't do sanity check of whether the tx_ring is full
	 * or not. When the queue is full, we flag the interface to TX_BUSY.
	 * We trust the dlpi_ether.c to put the next messages into queue and
	 * we will pick them up later in the el16sched().
	 */
	else {
		bdd->tail_cmd	= bdd->tail_cmd->next;

		if (bdd->tail_cmd->next == bdd->head_cmd) {
#ifdef DEBUG
			el16debug.ring_full++;
#endif
			bd->flags |= TX_BUSY;
		}
	}

	/* LINTED pointer alignment */
	p_cmd = (cmd_t *) (bdd->virt_ram + bdd->tail_cmd->ofst_cmd);
	/* LINTED pointer alignment */
	p_tbd = (tbd_t *) (bdd->virt_ram + p_cmd->prmtr.prm_xmit.xmt_tbd_ofst);
	p_txb = bdd->virt_ram + p_tbd->tbd_buff;

	/*
	 * fill in 82586's transmit command block
	 */
	p_cmd->cmd_status   = 0;
	p_cmd->cmd_cmd      = CS_EL | CS_CMD_XMIT | CS_INT;
	p_cmd->cmd_nxt_ofst = 0xFFFF;

	EL16_SET_16BIT_ACCESS(ctrl_reg, opri);
#ifdef	C_PIO
	BCOPY(hdr->dst.bytes, (char *)p_cmd+8, 6);
#else
	mybcopy(hdr->dst.bytes, (char *)p_cmd+8, 6);
#endif
	EL16_RESET_16BIT_ACCESS(ctrl_reg, opri);
	p_cmd->prmtr.prm_xmit.xmt_length = hdr->len_type;
	mp->b_rptr += LLC_EHDR_SIZE;

	/*
	 * copy data to tx buffer.
	 */
	for ( mp_tmp = mp; mp_tmp; mp_tmp = mp_tmp->b_cont ) {

		if ((msg_size = MB_SIZE(mp_tmp)) == 0)
			continue;

		if ( byte_left_over ) {
			msg_size--;
			partial_short.c.a[1] = *mp_tmp->b_rptr++;
			*(ushort_t *)p_txb = partial_short.c.b;
			p_txb	+= 2;
			tx_size += 2;
			byte_left_over = 0;
		}

		if (msg_size & 0x01) {
			msg_size--;
			partial_short.c.a[0] = *(mp_tmp->b_wptr - 1);
			byte_left_over++;
		}

		EL16_SET_16BIT_ACCESS(ctrl_reg, opri);
#ifdef C_PIO
		BCOPY(mp_tmp->b_rptr, p_txb, msg_size);
#else
		mybcopy(mp_tmp->b_rptr, p_txb, msg_size);
#endif
		tx_size	+= msg_size;
		p_txb	+= msg_size;
		EL16_RESET_16BIT_ACCESS(ctrl_reg, opri);
	}

	if ( byte_left_over ) {
		*p_txb++ = partial_short.c.a[0];
		tx_size++;
		byte_left_over = 0;
	}

	p_tbd->tbd_count = tx_size | CS_EOF;
	
	bd->mib.ifOutOctets += tx_size;	/* SNMP */
	bd->ifstats->ifs_opackets++;

	/*
	 * if cu is idle, start the 82586, issue a channel attention
	 */
	if ( start_cu ) {
		if (el16wait_scb (p_scb)) {
		    /*
 		     *+ debug message
		     */
		    cmn_err(CE_WARN,"el16xmit_packet: scb command not cleared");
		    bd->timer_val = 0;	/* force board to reset */
		    freemsg(mp);
		    return (-1);
		}

		p_scb->scb_cmd      = SCB_CUC_STRT;
		p_scb->scb_cbl_ofst = bdd->tail_cmd->ofst_cmd;

		if (el16bus_p & BUS_MCA) {
		    outb(bd->io_start+CTRL_REG_OFST, CR_CA|CR_RST|CR_BS|CR_IEN);
		    outb(bd->io_start+CTRL_REG_OFST, CR_RST|CR_BS|CR_IEN);
		}
		else
		    outb (bd->io_start + CHAN_ATN_OFST, 1);

		bd->timer_val = 2; 
	}
	freemsg(mp);
	return (0);
}

/* ARGSUSED */
/* 
 *  el16intr()
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 * Description :
 *  el16 intr routine.
 *
 */
void
el16intr ( int level )
{
	DL_bdconfig_t	*bd;
	bdd_t		*bdd;
	scb_t		*scb;
	int		base_io;
	int		ctrl_reg;
	int		index;
	ushort		scb_status;
#ifdef ESMP
	int		opri ;
#endif

	/*
	 * map irq level to the proper board. Make sure it's ours.
	 */
	if ((index = el16intr_to_index [level]) == -1) {
		/*
		 *+ debug message
		 */
		cmn_err (CE_WARN, "el16: spurious interrupt: irq %d", level);
		return;
	}

	/*
	 * get pointers to structures
	 */
	bd = &el16config [index];
	base_io = bd->io_start;
	ctrl_reg = base_io + CTRL_REG_OFST;

	/* LINTED pointer alignment */
	bdd = (bdd_t *) bd->bd_dependent1;
	scb = bdd->scb;

#ifdef ESMP
	opri = DLPI_LOCK( bd->bd_lock, plstr );
#endif

	/* disable board interrupt */
	outb(ctrl_reg, inb(ctrl_reg) & ~CR_IEN );

	/* clear interrupt */
	if (!(el16bus_p & BUS_MCA))
		outb (base_io + CLEAR_INTR, 1);

	/*
	 * If scb command field doesn't get cleared, reset the board.
	 */
	if (el16wait_scb (scb)) {
		/*
		 *+ debug message
		 */
		cmn_err (CE_WARN, "el16intr: scb command not cleared");
		bd->timer_val = 0;		/* force board to reset */

		/* enable board interrupt */
		outb( ctrl_reg, inb(ctrl_reg) | CR_IEN );
#ifdef ESMP
		DLPI_UNLOCK( bd->bd_lock, opri );
#endif
		return;
	}

	scb_status = scb->scb_status;

	/*
	 * acknowledge 82586 interrupt
	 */
	if ((scb->scb_cmd = scb_status & SCB_INT_MSK) != 0) {
		if (el16bus_p & BUS_MCA) {
			outb(base_io+CTRL_REG_OFST, CR_CA|CR_RST|CR_IEN|CR_BS);
			outb(base_io+CTRL_REG_OFST, CR_RST|CR_IEN|CR_BS);
		}
		else
			outb (base_io + CHAN_ATN_OFST, 1);
	}

	/*
	 * check for transmit done
	 */
	if (scb_status & (SCB_INT_CX | SCB_INT_CNA)) {
		el16tx_done (bd);
		if ( bd->flags & TX_QUEUED )
			el16sched (bd);
	}

	/*
	 * check for any received packet?
	 */
	if (scb_status & (SCB_INT_FR | SCB_INT_RNR))
		el16chk_ring (bd);

	/* enable board interrupt */
	outb( ctrl_reg, inb(ctrl_reg) | CR_IEN );

#ifdef ESMP
	DLPI_UNLOCK( bd->bd_lock, opri );
#endif
}

/* ARGSUSED */
/* 
 *  el16tx_done()
 *
 * Calling/Exit State:
 * 	caller should have lock.
 *
 * Description :
 *  transmission complete processing.
 *
 */

STATIC void
el16tx_done ( DL_bdconfig_t *bd )
{
	/* LINTED pointer alignment */
	bdd_t		*bdd = (bdd_t *) bd->bd_dependent1;
	cmd_t		*cmd;
	scb_t		*scb = bdd->scb;
	ushort		status;


	/* if ring is empty, we have nothing to process */
	if (bdd->head_cmd == 0)
		return;

	do {
		/* LINTED pointer alignment */
		cmd = (cmd_t *) (bdd->virt_ram + bdd->head_cmd->ofst_cmd);

		status = cmd->cmd_status;	/* xmit status	*/

		/* the following condition should never happen, just in case. */
		if ( !(status & CS_CMPLT) ) {
			if( scb->scb_status & SCB_INT_CNA )
				goto el16_start_cu;
			else
				return;
		}

		if ( !(status & CS_OK) ) {
			bd->mib.ifOutErrors++;
			bd->ifstats->ifs_oerrors++;

			if (status & CS_CARRIER)
				bd->mib.ifSpecific.etherCarrierLost++;

			if (status & CS_MAX_COLLIS) {
				bd->mib.ifSpecific.etherCollisions += 16;
				bd->ifstats->ifs_collisions += 16;
			}
		}
		else {
			int val;

			if( val = (status & CS_COLLISIONS) ) {
				bd->mib.ifSpecific.etherCollisions += val;
				bd->ifstats->ifs_collisions += val;
			}
		}

		if ( cmd->cmd_cmd & (CS_INT | CS_EL) )
			break;

		bdd->head_cmd = bdd->head_cmd->next;

	} while (1);

        /* Reset timer_val as transmit is complete */
        bd->timer_val = -1;
	bd->flags &= ~TX_BUSY;

	/* if last command in ring buffer */
	if (bdd->head_cmd == bdd->tail_cmd) {
		bdd->head_cmd = bdd->tail_cmd = 0;
		return;
	}
	else {
		/* there are more commands pending to be xmit */
		ring_t  *next_cmd;

		next_cmd = bdd->head_cmd = bdd->head_cmd->next;
		while (next_cmd != bdd->tail_cmd) {
			/* LINTED pointer alignment */
			cmd = (cmd_t *) (bdd->virt_ram + next_cmd->ofst_cmd);
			cmd->cmd_nxt_ofst = next_cmd->next->ofst_cmd;
			cmd->cmd_cmd &= ~(CS_EL | CS_INT);
			next_cmd = next_cmd->next;
		}

el16_start_cu:
		if (el16wait_scb (scb)) {
			/*
	 		 *+ debug message
			 */
			cmn_err(CE_WARN,"el16tx_done: scb command not cleared");
			bd->timer_val = 0;	/* force board to reset */
			return;
		}

		bd->timer_val = 2;
		scb->scb_cmd = SCB_CUC_STRT;
		scb->scb_cbl_ofst = bdd->head_cmd->ofst_cmd;
		if (el16bus_p & BUS_MCA) {
		    outb(bd->io_start+CTRL_REG_OFST, CR_CA|CR_RST|CR_BS|CR_IEN);
		    outb(bd->io_start+CTRL_REG_OFST, CR_RST|CR_BS|CR_IEN);
		}
		else
		    outb (bd->io_start+CHAN_ATN_OFST, 1);
	}
}

/* ARGSUSED */
/* 
 * el16sched( DL_bdconfig_t *bd ) 
 *
 * Calling/Exit State:
 * 	caller should have lock.
 *
 * Description :
 *	called from intr(), de-queue the outgoing packets from write_q.
 *
 */
STATIC void
el16sched( DL_bdconfig_t *bd ) 
{
	bdd_t	  *bdd = (bdd_t *) bd->bd_dependent1;
	DL_sap_t  *sap = bdd->next_sap;
	mblk_t	  *mp;
	int	  i;

	/*
	 *  Indicate outstanding transmit is done and
	 *  see if there is work waiting on our queue.
	 */

	for ( i = bd->ttl_valid_sap; i; i-- ) {

		if ( sap == (DL_sap_t *)NULL )
			sap = bd->valid_sap;

		if ( sap->state == DL_IDLE ) {
			while ( mp = getq(sap->write_q) ) {
				(void) el16xmit_packet(bd, mp);
				bd->mib.ifOutQlen--;
				if( bd->flags & TX_BUSY ) {
					bdd->next_sap = sap->next_sap;
					return;
				}
			}
		}
		sap = sap->next_sap;
	}

	/*
	 *  Nobody's left to service, make the queue empty.
	 */
#ifdef DEBUG
	el16debug.q_cleared++;
#endif
	bd->flags &= ~TX_QUEUED;
	bd->mib.ifOutQlen = 0;
	bdd->next_sap = (DL_sap_t *)NULL;
}

/* 
 * el16chk_ring ( DL_bdconfig_t *bd )
 *
 * Calling/Exit State:
 * 	caller should have lock.
 *
 * Description :
 *  check the rx ring for completed inbound packets.
 *
 */
STATIC void
el16chk_ring ( DL_bdconfig_t *bd )
{
    /* LINTED pointer alignment */
    bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
    fd_t	*fd;
    rbd_t	*last_rbd;
    mblk_t	*mp;
    ushort_t    rbd_status;
    int		ctrl_reg = bd->io_start + CTRL_REG_OFST;
    int		opri;


    fd = (fd_t *)(bdd->virt_ram + bdd->end_fd->fd_nxt_ofst);

    /* check to see if a rbd is attached to the fd */
    while ( fd->fd_rbd_ofst != 0xFFFF ) {

    	if ( !(fd->fd_status & CS_CMPLT) )
	    /* frame recption is still going on. */
	    break;

    	/* LINTED pointer alignment */
    	last_rbd = (rbd_t *)(bdd->virt_ram+fd->fd_rbd_ofst);
	rbd_status = last_rbd->rbd_status;

	/*
	 * we programmed 82586 not to save bad frames, so, the
	 * chances for fd_status is not OK are RU out of resource.
	 */
    	if ( (fd->fd_status & CS_OK) && (rbd_status & CS_EOF) ) {
	    /*
	     * if end-of-frame bit is not set, we must have
	     * received an over-sized frame. Discard it.
	     */
	    int len = rbd_status & CS_RBD_CNT_MSK;

	    if ((mp = allocb(len + LLC_EHDR_SIZE, BPRI_HI)) == NULL) {
		cmn_err(CE_WARN,"el16: allocb failed for rcv buffer\n");
		bd->mib.ifInDiscards++;			/* SNMP */
		bd->mib.ifSpecific.etherRcvResources++;	/* SNMP */
    		/* discarding the frame */
	    }
	    else {
		/* fill in the data from rcv buffers */
		EL16_SET_16BIT_ACCESS(ctrl_reg, opri);
#ifdef C_PIO
		BCOPY(fd->fd_dest, mp->b_wptr, LLC_EHDR_SIZE);
#else
		mybcopy(fd->fd_dest, mp->b_wptr, LLC_EHDR_SIZE);
#endif
		mp->b_wptr += LLC_EHDR_SIZE;
#ifdef C_PIO
		BCOPY(bdd->virt_ram + last_rbd->rbd_buff, mp->b_wptr, len);
#else
		mybcopy(bdd->virt_ram + last_rbd->rbd_buff, mp->b_wptr, len);
#endif
		EL16_RESET_16BIT_ACCESS(ctrl_reg, opri);
		mp->b_wptr += len;

		if ( !DLrecv(mp, bd->sap_ptr) ) {
			bd->mib.ifInOctets += len;
			bd->ifstats->ifs_ipackets++;
       	    	}
	    }
    	}
    	else {
	    /*
    	     *+ RU may be running out of resources or recv errors,
	     *  discarding frame and reclaiming the rbd's.
    	     */
	    cmn_err(CE_WARN, "el16: rcv error %b\n", fd->fd_status);
	    bd->mib.ifInDiscards++;	/* SNMP */

	    while ( !(last_rbd->rbd_status & CS_EOF) &&
		    !(last_rbd->rbd_size   & CS_EL ) )
		    /* LINTED pointer alignment */
		    last_rbd = (rbd_t *)(bdd->virt_ram+last_rbd->rbd_nxt_ofst);
	}

	/* re-queue rbd */
	bdd->end_rbd->rbd_size = RBD_BUF_SIZ;	/* clear EL bit */
	bdd->end_rbd = last_rbd;		/* establish new end of ring */
	last_rbd->rbd_size |= CS_EL;		/* set EL bit at end of ring */

	/* re-queue fd */
	fd->fd_rbd_ofst = 0xFFFF;		/* unlink fd and rbd */
	bdd->end_fd->fd_cmd = 0;		/* clear EL bit */
	bdd->end_fd = fd;			/* establish new end of ring */
	fd->fd_cmd = CS_EL;			/* set EL bit at end of ring */

	/* LINTED pointer alignment */
	fd = (fd_t *)(bdd->virt_ram + fd->fd_nxt_ofst);
    }

    /*
     * if RU already running -- leave it alone
     */
    if ( (bdd->scb->scb_status & SCB_RUS_READY) == SCB_RUS_READY )
	return;

    el16ru_restart (bd);
}

/* 
 * el16ru_restart ( DL_bdconfig_t *bd )
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 * Description :
 *	restart 586 recv unit if necessary.
 *
 */
STATIC void
el16ru_restart ( DL_bdconfig_t *bd )
{
	/* LINTED pointer alignment */
	bdd_t	*bdd    	= (bdd_t *) bd->bd_dependent1;
	scb_t	*scb		= bdd->scb;
	fd_t	*begin_fd;


	begin_fd  =  (fd_t *)(bdd->virt_ram + bdd->end_fd->fd_nxt_ofst);

	/*
	 * if the RU just went not ready and it just completed an fd --
	 * do NOT restart RU -- this will wipe out the just completed fd.
	 * There will be a second interrupt that will remove the fd via
	 * el16chk_ring () and thus calls el16ru_restart() which will
	 * then start the RU if necessary.
	 */
	if ( ! (scb->scb_status & SCB_RUS_NORESRC) )
		if (begin_fd->fd_status & CS_CMPLT)
			return;

	/*
	 * if we get here, then RU is not ready and no completed fd's are avail.
	 * therefore, follow RU start procedures listed under RUC on page 2-15
	 */
#ifdef DEBUG
	el16debug.rcv_restart_count++;
#endif

	if (el16wait_scb (scb)) {
		/*
		 *+ debug message
		 */
		cmn_err (CE_WARN, "el16: ru-restart: scb command not cleared");
		bd->timer_val = 0;		/* force board to reset */
		return;
	}

	begin_fd->fd_rbd_ofst = bdd->end_rbd->rbd_nxt_ofst;
	scb->scb_rfa_ofst = bdd->end_fd->fd_nxt_ofst;
	scb->scb_cmd      = SCB_RUC_STRT;

	if (el16bus_p & BUS_MCA) {
		outb (bd->io_start + CTRL_REG_OFST, CR_CA|CR_RST|CR_BS|CR_IEN);
		outb (bd->io_start + CTRL_REG_OFST, CR_RST|CR_BS|CR_IEN);
	}
	else
		outb (bd->io_start + CHAN_ATN_OFST, 1);

	return;
}

/* 
 * el16watch_dog (bd)
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 * Description :
 * watchdog timer.
 *
 */
void
el16watch_dog ()
{	
    DL_bdconfig_t	*bd;
    bdd_t		*bdd;
    scb_t		*scb;
    int			i;
    int			ierror = 0;
    int			opri;


    for (i=0, bd=el16config; i < el16boards; bd++, i++) {

	/* LINTED pointer alignment */
	bdd = (bdd_t *)bd->bd_dependent1;
	scb = bdd->scb;

	/*
	 *  Store error statistics in the MIB structure.
	 */
	bd->mib.ifSpecific.etherAlignErrors   += scb->scb_aln_err;
	ierror += scb->scb_aln_err;
	bd->mib.ifSpecific.etherCRCerrors     += scb->scb_crc_err;
	ierror += scb->scb_crc_err;
	bd->mib.ifSpecific.etherMissedPkts    += scb->scb_rsc_err;
	ierror += scb->scb_rsc_err;
	bd->mib.ifSpecific.etherOverrunErrors += scb->scb_ovrn_err;
	ierror += scb->scb_ovrn_err;
	bd->mib.ifInErrors += ierror;
	bd->ifstats->ifs_ierrors = bd->mib.ifInErrors;

	opri = DLPI_LOCK( bd->bd_lock, plstr );

	/**
	 ** scb->scb_crc_err = scb->scb_aln_err = 0;
	 ** scb->scb_rsc_err = scb->scb_ovrn_err = 0;
	 **/
	*(double *)(&scb->scb_crc_err) = 0;

	if ( scb->scb_status & SCB_RUS_NORESRC )
		el16chk_ring(bd);

	if (bd->timer_val > 0)
	    bd->timer_val--;

	if (bd->timer_val == 0) {
	    bd->timer_val = -1;
	    bd->flags = 0;
	    DLPI_UNLOCK( bd->bd_lock, opri );
	    /*
	     *+ debug message
	     */
	    cmn_err (CE_NOTE,"el16: board %d timed out.", bd->bd_number);

	    (void)el16reset(bd);
	    bd->flags = BOARD_PRESENT;
	    opri = DLPI_LOCK( bd->bd_lock, plstr );
	}
	DLPI_UNLOCK( bd->bd_lock, opri );
    }

    /*
     * reset timeout to 5 seconds
     */
#ifdef ESMP
    el16timer_id = itimeout (el16watch_dog, 0, EL16_TIMEOUT, plstr);
#else
    el16timer_id = timeout (el16watch_dog, 0, EL16_TIMEOUT);
#endif
    return;
}

/* 
 * el16promisc_on (bd)
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 * Description :
 *  turn on promiscuous mode.
 *
 */
int
el16promisc_on ( DL_bdconfig_t	*bd )
{
	int opri;

	/*
	 *  If already in promiscuous mode, just return.
	 */
	opri = DLPI_LOCK( bd->bd_lock, plstr );
	if (bd->promisc_cnt++)
	{
		DLPI_UNLOCK( bd->bd_lock, opri );
		return (0);
	}

	if (el16config_586 (bd, EL16_PRO_ON))
	{
		DLPI_UNLOCK( bd->bd_lock, opri );
		return (1);
	}

	DLPI_UNLOCK( bd->bd_lock, opri );
	return (0);
}

/* 
 * el16promisc_off (bd)
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 * Description :
 *  turn off promiscuous mode.
 *
 */
int
el16promisc_off ( DL_bdconfig_t	*bd )
{
	int opri;

	opri = DLPI_LOCK( bd->bd_lock, plstr );
	/*
	 *  If the board is not in a promiscuous mode, just return;
	 */
	if (!bd->promisc_cnt)
	{
		DLPI_UNLOCK( bd->bd_lock, opri );
		return (0);
	}

	/*
	 *  If this is not the last promiscuous SAP, just return;
	 */
	if (--bd->promisc_cnt)
	{
		DLPI_UNLOCK( bd->bd_lock, opri );
		return (0);
	}

	/*
	 *  Save the current page then go to page two and read the current
	 *  receive configuration.
	 */

	if (el16config_586 (bd, EL16_PRO_OFF))
	{
		DLPI_UNLOCK( bd->bd_lock, opri );
		return (1);
	}

	DLPI_UNLOCK( bd->bd_lock, opri );
	return (0);
}

/* 
 *  el16set_eaddr()
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 * Description :
 *  set physical address.
 *
 */
int
el16set_eaddr ( DL_bdconfig_t *bd, DL_eaddr_t *eaddr )
{
	int  val;
	int opri;

	opri = DLPI_LOCK( bd->bd_lock, plstr );

	BCOPY( eaddr, bd->eaddr.bytes, DL_MAC_ADDR_LEN );

	/*
	 * do Individual Address Setup command
	 */
	val = el16ia_setup_586 (bd, bd->eaddr.bytes);

	DLPI_UNLOCK( bd->bd_lock, opri );
	return (val);
}

/* 
 *  el16add_multicast()
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 * Description :
 *  add multicast address.
 *
 */
int
el16add_multicast ( DL_bdconfig_t *bd, DL_eaddr_t *maddr )
{
	/* LINTED pointer alignment */
	bdd_t   *bdd = (bdd_t *)bd->bd_dependent1;
	mcat_t  *mcp = &(bdd->el16_multiaddr[0]);
	int i;
	int rval;
	int opri;


	opri = DLPI_LOCK( bd->bd_lock, plstr );
	if ( (bd->multicast_cnt >= MULTI_ADDR_CNT) || !(maddr->bytes[0] & 0x1)){                
		DLPI_UNLOCK( bd->bd_lock, opri );
		return 1;
	}

	if ( el16is_multicast(bd,maddr)) {
		DLPI_UNLOCK( bd->bd_lock, opri );
		return 0;
	}

	for (i = 0; i < MULTI_ADDR_CNT; i++,mcp++) {
		if (!mcp->status)
			break;
	}

	mcp->status = 1;
	bd->multicast_cnt++;
	BCOPY(maddr->bytes, mcp->entry, DL_MAC_ADDR_LEN);
	rval = el16set_multicast(bd);
	DLPI_UNLOCK( bd->bd_lock, opri );
	return rval;
}

/* ARGSUSED */
/* 
 * el16set_multicast(DL_bdconfig_t *bd)
 *
 * Calling/Exit State:
 * 	Caller should have lock
 *
 * Description :
 *  set multicast address hash table.
 *
 */
STATIC int
el16set_multicast( DL_bdconfig_t *bd )
{
	/* LINTED pointer alignment */
	bdd_t	*bdd	 = (bdd_t *) bd->bd_dependent1;
	/* LINTED pointer alignment */
	cmd_t	*gen_cmd = (cmd_t *) (bdd->virt_ram + bdd->ofst_gen_cmd);
	scb_t	*scb	 = bdd->scb;
	mcat_t	*mcp	 = &(bdd->el16_multiaddr[0]);
	mcad_t	mcad;
	ushort	base_io = bd->io_start;
	int	i, j;

	if (el16wait_scb (scb))
		return (1);

	scb->scb_status   = 0;
	scb->scb_cmd      = SCB_CUC_STRT;
	scb->scb_cbl_ofst = bdd->ofst_gen_cmd;

	/* LINTED pointer alignment */
	gen_cmd->cmd_status   = 0;
	gen_cmd->cmd_cmd      = CS_CMD_MCSET | CS_EL;
	gen_cmd->cmd_nxt_ofst = 0xFFFF;

	for (i = 0, j = 0; i < MULTI_ADDR_CNT; i++, mcp++) {
		if (mcp->status) {
			BCOPY(mcp->entry, &mcad.mc_addr[j], DL_MAC_ADDR_LEN);
			j += DL_MAC_ADDR_LEN;
		}
	}

	mcad.mc_cnt = (ushort)j;

	for (i=0; i<j; i++)
		gen_cmd->prmtr.prm_mcad.mc_addr[i] = mcad.mc_addr[i];

	if (el16bus_p & BUS_MCA) {
		outb (base_io + CTRL_REG_OFST, CR_CA|CR_RST|CR_BS);
		outb (base_io + CTRL_REG_OFST, CR_RST|CR_BS);
	}
	else
		outb (base_io + CHAN_ATN_OFST, 1);

	if (el16wait_scb (scb))
		return (1);

	if (el16ack_586 (scb, base_io))
		return (1);

	return (0);
}

/* ARGSUSED */
/* 
 * el16get_multicast(DL_bdconfig_t *bd, mblk_t *mp)
 *
 * Calling/Exit State:
 * 	Caller should have lock
 *
 * Description :
 *  get multicast addresses.
 *
 */
int
el16get_multicast( DL_bdconfig_t *bd, mblk_t *mp )
{
	/* LINTED pointer alignment */
	bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
	mcat_t  *mcp = &(bdd->el16_multiaddr[0]);
	uchar_t *dp;
	int i;
	int found = 0;
	int opri;

	if((int)(mp->b_wptr - mp->b_rptr) == 0)
		found = bd->multicast_cnt;
	else {
		opri = DLPI_LOCK( bd->bd_lock, plstr );
		dp = mp->b_rptr;
		for (i = 0;(i < MULTI_ADDR_CNT) && (dp < mp->b_wptr);i++,mcp++)
			if (mcp->status) {
				BCOPY(mcp->entry, dp, DL_MAC_ADDR_LEN);
				dp += DL_MAC_ADDR_LEN;
				found++;
			}
		mp->b_wptr = dp;
		DLPI_UNLOCK( bd->bd_lock, opri );
	}
	return found;
}

/* 
 *  el16del_multicast()
 *
 * Calling/Exit State:
 * 	none
 *
 * Description :
 *  del multicast address.
 *
 */
int
el16del_multicast ( DL_bdconfig_t *bd, DL_eaddr_t *maddr )
{
	/* LINTED pointer alignment */
	bdd_t   *bdd = (bdd_t *)bd->bd_dependent1;
	mcat_t  *mcp = &(bdd->el16_multiaddr[0]);
	int rval ;
	int i;
	int opri;


	opri = DLPI_LOCK( bd->bd_lock, plstr );
	if (!el16is_multicast(bd,maddr))
		rval = 1;
	else {
		for (i = 0; i < MULTI_ADDR_CNT; i++, mcp++)
			if ( (mcp->status) && (BCMP(maddr->bytes,mcp->entry,DL_MAC_ADDR_LEN) == 0) )
				break;

		mcp->status = 0;
		bd->multicast_cnt--;
		rval = el16set_multicast(bd);
	}
	DLPI_UNLOCK( bd->bd_lock, opri );
	return rval;
}

/* ARGSUSED */
/* 
 * el16disable
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 * Description :
 *  disable the el16.
 *
 */
int
el16disable ( DL_bdconfig_t *bd )
{
	return (1);	/* not supported yet */
}

/* 
 *  el16enable()
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 * Description :
 * 	enable el16 board.
 *
 */
/* ARGSUSED */
int
el16enable ( DL_bdconfig_t *bd )
{
	return (1);	/* not supported yet */
}

/* 
 *  el16reset()
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 * Description :
 *  reset el16 board.
 *
 */
/* ARGSUSED */
int
el16reset ( DL_bdconfig_t *bd )
{
	DL_sap_t *sap;
	int	  val;
	int	  opri;


	opri = DLPI_LOCK( bd->bd_lock, plstr );
	bd->timer_val = -1;

	sap = bd->valid_sap;
	while (sap) {
		if ( (sap->write_q != NULL) && (sap->state == DL_IDLE) )
			flushq(sap->write_q, FLUSHDATA);
		sap = sap->next_sap;
	}
      	bd->flags &= ~(TX_BUSY|TX_QUEUED);
	val = el16init_586(bd);

	DLPI_UNLOCK( bd->bd_lock, opri );
	return(val);
}

/* 
 *  el16is_multicast()
 *
 * Calling/Exit State:
 * 	Caller should have lock
 *
 * Description :
 *  check if the address is a multicast address.
 *
 */
/*ARGSUSED*/
int
el16is_multicast ( DL_bdconfig_t *bd, DL_eaddr_t *eaddr )
{
    /* LINTED pointer alignment */
    bdd_t *bdd = (bdd_t *)bd->bd_dependent1;
    mcat_t *mcp = &bdd->el16_multiaddr[0];
    int i;
    int rval = 0;

    if (bd->multicast_cnt) {
	for (i = 0; i < MULTI_ADDR_CNT; i++, mcp++) {
	    if ( (mcp->status) && (BCMP(eaddr->bytes,mcp->entry,DL_MAC_ADDR_LEN) == 0) ) {
		rval = 1;
		break;
	    }
	}
    }
    return (rval);
}
