#ident	"@(#)ihvkit:net/dlpi_ether/el16hrdw.c	1.1"
#ident  "$Header$"

/*	Copyright (c) 1991  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ifdef	_KERNEL_HEADERS

#include <io/dlpi_ether/dlpi_el16.h>
#include <io/dlpi_ether/dlpi_ether.h>
#include <io/dlpi_ether/el16.h>
#include <io/strlog.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <net/dlpi.h>
#include <net/tcpip/if.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#include <io/ddi.h>
#include <io/ddi_i386at.h>

#elif defined(_KERNEL)

#include <sys/dlpi_el16.h>
#include <sys/dlpi_ether.h>
#include <sys/el16.h>
#include <sys/strlog.h>
#include <sys/immu.h>
#include <sys/kmem.h>
#include <sys/dlpi.h>
#include <net/if.h>
#include <sys/errno.h>
#include <sys/cmn_err.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/debug.h>
#include <sys/ddi.h>
#include <sys/ddi_i386at.h>

#endif /* _KERNEL_HEADERS */

#define BCOPY(from, to, len)	bcopy((caddr_t)(from), (caddr_t)(to), \
	(size_t)(len))
#define BCMP(s1, s2, len)	bcmp((char *)(s1), (char *)(s2), \
	(size_t)(len))

extern	DL_bdconfig_t	DLconfig[];
extern	DL_sap_t	DLsaps[];
extern	struct ifstats	*DLifstats;
extern	int	bus_p;

extern	int	DLstrlog;
extern	char	DLid_string[];
extern	int	DLboards;
extern	char	DLcopyright[];
extern	int	el16intr_to_index[];
extern	int	el16timer_id;
extern	int	el16wait_scb();
extern	ushort	ntohs(), htons();

extern	int	el16recv(), el16ack_586(), el16config_586();
extern	int	el16wait_scb(), el16init_586();

STATIC	void	el16tx_done(), el16sched(), el16chk_ring();

int	el16call_sched;

#ifdef DEBUG
struct	debug	el16debug = { 0, 0, 0, 0, 0};
#endif

/* 
 *  The following is for Berkely Packet Filter (BPF) support.
 */
#ifdef NBPFILTER
#include <net/bpf.h>
#include <net/bpfdesc.h>

STATIC	struct	ifnet	*el16ifp;
STATIC	int	el16bpf_ioctl(), el16bpf_output(), el16bpf_promisc();

extern	void	bpf_tap(), outb();
#endif

/* ARGSUSED */
/* 
 * el16bdspecclose(queue_t *q)
 *
 * Calling/Exit State:
 *
 * Description :
 *      el16bdspecclose is called from DLclose->dlpi_ether.c
 */
void
el16bdspecclose(q)
queue_t *q;
{
	return;
} /* end of el16bdspecioctl */

/* ARGSUSED */
/* 
 * el16bdspecioctl(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *
 * Description :
 *       el16bdspecioctl is called from DLioctl->dlpi_ether.c
 *
 */
void
el16bdspecioctl(q, mp)
queue_t *q;
mblk_t *mp;
{
	/* LINTED pointer alignment */
	struct iocblk *ioctl_req = (struct iocblk *)mp->b_rptr;

	ioctl_req->ioc_error = EINVAL;
	ioctl_req->ioc_count = 0;
	mp->b_datap->db_type = M_IOCNAK;
} /* end of el16bdspecioctl */

/* 
 *  el16xmit_packet()
 *
 * Calling/Exit State:
 *
 * Description :
 *  This routine is called from DLunitdata_req() and el16tx_done(). It assumes
 *  we are at STREAMS spl interrupt level.
 *
 */
el16xmit_packet (bd, mp)
DL_bdconfig_t *bd;
mblk_t	*mp;
{
	/* LINTED pointer alignment */
	bdd_t *bdd = (bdd_t *) bd->bd_dependent1;

	scb_t	*p_scb = bdd->scb;
	tbd_t	*p_tbd;
	cmd_t	*p_cmd;
	char	*p_txb;

	mblk_t	*mp_tmp;

	int	start_cu = 0;
	int	tx_size = 0;
	int	msg_size = 0;
	int	old;
	uchar_t	ctrl_reg_val;

DL_LOG(strlog(DL_ID, 100, 3, SL_TRACE,"el16xmit_packet on board %x",(int)bd));

	/*
	 * the bdd->head_cmd always points to the first unacknowledged cmd.
	 * the bdd->tail_cmd always points to the last  unacknowledged cmd.
	 *
	 * If ring buffer is empty
	 */
	if (bdd->head_cmd == NULL) {
DL_LOG(strlog(DL_ID, 200, 0, SL_TRACE,"el16xmit_packet: ring EMPTY"));
		bdd->head_cmd = bdd->tail_cmd = bdd->ring_buff;
		start_cu = 1;
	}

	/*
	 * else there's at least one command pending in ring.
	 * Note that we don't check whether queue is full or not.
	 * When the queue is full, we flag the interface to TX_BUSY.
	 * Here we trust the dlpi_ether.c to put the next message into queue.
	 */
	else {
		/* LINTED pointer alignment */
		bdd->tail_cmd = bdd->tail_cmd->next;

		if (bdd->tail_cmd->next == bdd->head_cmd) {

DL_LOG(strlog(DL_ID,200,1,SL_TRACE,"el16xmit_packet: ring FULL"));

#ifdef DEBUG
			el16debug.ring_full++;
#endif
			bd->flags |= TX_BUSY;
		}
	}

	/*
	 * set up pointers
	 */
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
	
	/*
	 * copy data to tx buffer.
	 */
	EL16_SET_16BIT_ACCESS(bd, ctrl_reg_val, old);
	for (mp_tmp = mp; mp_tmp != NULL; mp_tmp = mp_tmp->b_cont) {
		msg_size = mp_tmp->b_wptr - mp_tmp->b_rptr;
		mybcopy ((caddr_t) mp_tmp->b_rptr, p_txb, msg_size);

		p_txb          += msg_size;
		tx_size        += msg_size;
	}
	EL16_RESET_16BIT_ACCESS(bd, ctrl_reg_val, old);

	p_tbd->tbd_count = tx_size | CS_EOF;
	
#ifdef NBPFILTER
	if (bd->bpf) {
	    mblk_t	*mp_tap;

	    if ((mp_tap = allocb(tx_size, BPRI_MED)) != NULL) {
		for (mp_tmp=mp->b_cont; mp_tmp; mp_tmp=mp_tmp->b_cont) {
		    msg_size = mp_tmp->b_wptr - mp_tmp->b_rptr;
		    /* LINTED pointer alignment */
		    mybcopy((caddr_t)mp_tmp->b_rptr,(caddr_t)mp_tap->b_wptr,msg_size);
		    mp_tap->b_wptr += msg_size;
		}

		bpf_tap(bd->bpf, mp_tap->b_rptr, msg_size);
		freemsg(mp_tap);
	    }
	}
#endif

	bd->mib.ifOutOctets += tx_size;	/* SNMP */

	/*
	 * if we need to start the 82586, issue a channel attention
	 */
	if ( start_cu ) {
		if (el16wait_scb (p_scb)) {
			/*
	 		 *+ debug message
			 */
			cmn_err (CE_WARN, "el16xmit_packet: scb command not cleared");
			bd->timer_val = 0;	/* force board to reset */
			freemsg(mp);
			return (-1);
		}
		p_scb->scb_cmd      = SCB_CUC_STRT;
		p_scb->scb_cbl_ofst = bdd->tail_cmd->ofst_cmd;

		if (bus_p & BUS_MCA) {
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
el16intr (level)
int	level;
{
	DL_bdconfig_t	*bd;
	bdd_t		*bdd;
	scb_t		*scb;
	cmd_t		*cmd;
	int		base_io;
	int		index;
	int		old;
	ushort		scb_status;
	uchar_t		ctrl_reg_val;

	/*
	 * map irq level to the proper board. Make sure it's ours.
	 */
	if ((index = el16intr_to_index [level]) == -1) {
		/*
		 *+ debug message
		 */
		cmn_err (CE_WARN, "%s spurious interrupt", el16id_string);
		return;
	}

	/*
	 * get pointers to structures
	 */
	bd = &el16config [index];
	base_io = bd->io_start;
	/* LINTED pointer alignment */
	bdd = (bdd_t *) bd->bd_dependent1;
	/* LINTED pointer alignment */
	scb = bdd->scb;

	/* disable board interrupt */
	ctrl_reg_val = inb( base_io + CTRL_REG_OFST );
	ctrl_reg_val &= ~CR_IEN;
	outb( base_io + CTRL_REG_OFST, ctrl_reg_val );

	/* clear interrupt */
	if (!(bus_p & BUS_MCA))
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
		ctrl_reg_val = inb( base_io + CTRL_REG_OFST );
		ctrl_reg_val |= CR_IEN;
		outb( base_io + CTRL_REG_OFST, ctrl_reg_val );
		return;
	}

	scb_status = scb->scb_status;
DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"EL16INTR: SCB status: %x;",scb_status));

	/*
	 * acknowledge 82586 interrupt
	 */
	if ((scb->scb_cmd = scb_status & SCB_INT_MSK) != 0) {
		if (bus_p & BUS_MCA) {
			outb(base_io+CTRL_REG_OFST, CR_CA|CR_RST|CR_IEN|CR_BS);
			outb(base_io+CTRL_REG_OFST, CR_RST|CR_IEN|CR_BS);
		}
		else
			outb (base_io + CHAN_ATN_OFST, 1);
	}

	if (scb_status & (SCB_INT_FR | SCB_INT_RNR)) {
		EL16_SET_16BIT_ACCESS(bd, ctrl_reg_val, old);
		el16chk_ring (bd);
		EL16_RESET_16BIT_ACCESS(bd,ctrl_reg_val, old);
	}

	if (bdd->head_cmd != 0) {
		el16call_sched = 0;

		if (scb_status & (SCB_INT_CX | SCB_INT_CNA))
			el16tx_done (bd);
		if( el16call_sched && (bd->flags & TX_QUEUED) )
			el16sched(bd);
	}

	/* enable board interrupt */
	ctrl_reg_val = inb( base_io + CTRL_REG_OFST );
	ctrl_reg_val |= CR_IEN;
	outb( base_io + CTRL_REG_OFST, ctrl_reg_val );
}

/* 
 *  el16tx_done()
 *
 * Calling/Exit State:
 *
 * Description :
 *  transmission complete processing.
 *
 */
STATIC void
el16tx_done (bd)
DL_bdconfig_t	*bd;
{
	/* LINTED pointer alignment */
	bdd_t		*bdd = (bdd_t *) bd->bd_dependent1;
	cmd_t		*cmd;
	scb_t		*scb = bdd->scb;
	ushort		status;

DL_LOG(strlog(DL_ID,100,3,SL_TRACE,"el16tx_done: for board %d", bd->bd_number));

        /* Reset timer_val as transmit is complete */
        bd->timer_val = -1;

	bd->flags &= ~TX_BUSY;

	do {

	/* LINTED pointer alignment */
	cmd = (cmd_t *) (bdd->virt_ram + bdd->head_cmd->ofst_cmd);

	/*
	 *  Read the tx status reg and see if there were any problems.
	 */
	status = cmd->cmd_status;

	if ( !(status & CS_CMPLT) ) {
		/*
		 * should not get into this, just be cautious
		 */
		if( scb->scb_status & SCB_INT_CNA )
			goto el16_start_cu;
		else
			return;
	}

	bd->ifstats->ifs_opackets++;

	if (!(status & CS_OK)) {
		bd->mib.ifOutErrors++;
		bd->ifstats->ifs_oerrors++;
		if (status & CS_CARRIER)
			bd->mib.ifSpecific.etherCarrierLost++;
	}

	if (status & CS_COLLISIONS) {
		bd->mib.ifSpecific.etherCollisions++;
		bd->ifstats->ifs_collisions++;
	}

	el16call_sched++;

	if ( cmd->cmd_cmd & (CS_INT | CS_EL) )
		break;

	bdd->head_cmd = bdd->head_cmd->next;

	} while (1);

	/*
	 * if last command in ring buffer
	 */
	if (bdd->head_cmd == bdd->tail_cmd) {
DL_LOG(strlog(DL_ID,200,0,SL_TRACE,"el16tx_done: last command"));
		bdd->head_cmd = bdd->tail_cmd = 0;
		return;
	}

	/*
	 * else there are more commands pending to be xmit
	 */
	else {
		ring_t  *next_cmd;

		next_cmd = bdd->head_cmd = bdd->head_cmd->next;
		while (next_cmd != bdd->tail_cmd) {
			/* LINTED pointer alignment */
			cmd = (cmd_t *) (bdd->virt_ram + next_cmd->ofst_cmd);
			cmd->cmd_cmd &= ~(CS_EL | CS_INT);
			cmd->cmd_nxt_ofst = next_cmd->next->ofst_cmd;
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
		scb->scb_cmd = SCB_CUC_STRT;
		scb->scb_cbl_ofst = bdd->head_cmd->ofst_cmd;
		if (bus_p & BUS_MCA) {
		    outb(bd->io_start+CTRL_REG_OFST, CR_CA|CR_RST|CR_BS|CR_IEN);
		    outb(bd->io_start+CTRL_REG_OFST, CR_RST|CR_BS|CR_IEN);
		}
		else
		    outb (bd->io_start+CHAN_ATN_OFST, 1);
	}
}

STATIC void
el16sched( DL_bdconfig_t *bd )
{
	bdd_t	  *bdd	    = (bdd_t *) bd->bd_dependent1;
	DL_sap_t  *next_sap = bdd->next_sap;
	mblk_t	  *mp;
	int	  i;

	/*
	 *  Indicate outstanding transmit is done and
	 *  see if there is work waiting on our queue.
	 */

	for ( i = bd->ttl_valid_sap; i; i-- ) {

		if ( next_sap == (DL_sap_t *)NULL )
			next_sap = bd->valid_sap;
	
		if ( next_sap->state == DL_IDLE ) {
			while ( mp = getq(next_sap->write_q) ) {
				if( bd->flags & TX_BUSY ) {
					putbq(next_sap->write_q, mp);
					bdd->next_sap = next_sap->next_sap;
					return;
				}
				(void) el16xmit_packet(bd, mp);
				bd->mib.ifOutQlen--;
			}
		}
		next_sap = next_sap->next_sap;
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
 *  el16chk_ring (bd)
 *
 * Calling/Exit State:
 * 	caller should have lock.
 *
 * Description :
 *  check the tx/rx ring.  
 *
 */
STATIC void
el16chk_ring (bd)
DL_bdconfig_t *bd;
{
	/* LINTED pointer alignment */
	bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
	fd_t	*fd;
	rbd_t	*first_rbd, *last_rbd;
	mblk_t	*mp;
	int	length, nrbd;


DL_LOG (strlog (DL_ID, 103, 3, SL_TRACE, "el16chk_ring:"));

	/* LINTED pointer alignment */
	for (fd=bdd->begin_fd; fd->fd_rbd_ofst != 0xFFFF; fd=(fd_t *)(fd->fd_nxt_ofst+bdd->virt_ram)) {

		if (!(fd->fd_status & CS_CMPLT_OK)) {
DL_LOG(strlog(DL_ID,103,3,SL_TRACE, "fd status: 0x%x", fd->fd_status));
			break;
		}

		length = nrbd = 0;

		/* LINTED pointer alignment */
		first_rbd = last_rbd = (rbd_t *)(bdd->virt_ram+fd->fd_rbd_ofst);

		/*
		 * find len of data and last rbd holding the rcv frame.
		 */
		while ( !(last_rbd->rbd_status & CS_EOF) ) {
			nrbd++;
			length += last_rbd->rbd_status & CS_RBD_CNT_MSK;

			if ( !(last_rbd->rbd_size & CS_EL) )

			    /* LINTED pointer alignment */
			    last_rbd = (rbd_t *)(bdd->virt_ram+last_rbd->rbd_nxt_ofst);
			else {
			    /*
			     *+ no more receive buffers
			     */
			    cmn_err(CE_WARN, "out of recv buf\n");
			    bd->mib.ifInDiscards++;	/* SNMP */
			    bd->mib.ifSpecific.etherRcvResources++;
			    bd->ifstats->ifs_ipackets++;
			    goto requeue_586;
			}
		}
		length += last_rbd->rbd_status & CS_RBD_CNT_MSK;
		nrbd++;

		if (fd->fd_status & CS_OK) {

			if ((mp = allocb(length, BPRI_MED)) == NULL) {
			    cmn_err(CE_WARN,"no receive resources");
			    bd->mib.ifInDiscards++;	/* SNMP */
			    bd->mib.ifSpecific.etherRcvResources++;

			} else {
			    /* fill in the data from rcv buffers */

			    rbd_t	*temp_rbd;
			    int		i, len;

			    for (i=0, temp_rbd=first_rbd; i < nrbd; i++) {
			    	len = temp_rbd->rbd_status & CS_RBD_CNT_MSK;	
			    	mybcopy ((caddr_t)bdd->virt_ram + temp_rbd->rbd_buff, (caddr_t)mp->b_wptr, len);
			    	mp->b_wptr += len;
			    	temp_rbd->rbd_status = 0;

				/* LINTED pointer alignment */
				temp_rbd = (rbd_t *)(bdd->virt_ram+temp_rbd->rbd_nxt_ofst);
			    }
			    bd->ifstats->ifs_ipackets++;

			    if (!DLrecv(mp, bd->sap_ptr))
			    	bd->mib.ifInOctets += length;
			}

		} else
			bd->ifstats->ifs_ierrors++;

		fd->fd_rbd_ofst = 0xFFFF;
	}

requeue_586:
	/* re-queue rbd */
	bdd->end_rbd->rbd_size	&= ~CS_EL;

	/* LINTED pointer alignment */
	bdd->begin_rbd = (rbd_t *) (bdd->virt_ram + last_rbd->rbd_nxt_ofst);
	bdd->end_rbd = last_rbd;
	last_rbd->rbd_size |=  CS_EL;

	/* re-queue fd */
	bdd->end_fd->fd_cmd = 0;	/* clear EL bit */
	bdd->begin_fd	= fd;
	bdd->end_fd	= (--fd);
	fd->fd_cmd      = CS_EL;

	(void) el16ru_restart (bd);
}


/* 
 * el16ru_restart ()
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 * Description :
 *
 */
STATIC int
el16ru_restart (bd)
DL_bdconfig_t *bd;
{
	/* LINTED pointer alignment */
	bdd_t	*bdd    	= (bdd_t *) bd->bd_dependent1;
	/* LINTED pointer alignment */
	scb_t	*scb		= bdd->scb;
	fd_t	*begin_fd	= bdd->begin_fd;


	/*
	 * RU already running -- leave it alone
	 */
	if ((scb->scb_status & SCB_RUS_READY) == SCB_RUS_READY)
		return (1);

	/*
	 * if we get here, then RU is not ready and no completed fd's are avail.
	 * therefore, follow RU start procedures listed under RUC on page 2-15
	 */
#ifdef DEBUG
	el16debug.rcv_restart_count++;
#endif
	begin_fd->fd_rbd_ofst =(ushort)((char *)bdd->begin_rbd - bdd->virt_ram);

	if (el16wait_scb (scb)) {
		/*
		 *+ debug message
		 */
		cmn_err (CE_WARN, "ru-restart: scb command not cleared");
		bd->timer_val = 0;		/* force board to reset */
		return (0);
	}

	scb->scb_rfa_ofst = (ushort)((char *)bdd->begin_fd - bdd->virt_ram);
	scb->scb_cmd      = SCB_RUC_STRT;

	/*
	 * if the RU just went not ready and it just completed an fd --
	 * do NOT restart RU -- this will wipe out the just completed fd.
	 * There will be a second interrupt that will remove the fd via
	 * el16chk_ring () and thus calls el16ru_restart() which will
	 * then start the RU if necessary.
	 */
	if (begin_fd->fd_status & CS_CMPLT_OK)
		return(1);

	if (bus_p & BUS_MCA) {
		outb (bd->io_start + CTRL_REG_OFST, CR_CA|CR_RST|CR_BS|CR_IEN);
		outb (bd->io_start + CTRL_REG_OFST, CR_RST|CR_BS|CR_IEN);
	}
	else
		outb (bd->io_start + CHAN_ATN_OFST, 1);

	DL_LOG(strlog(DL_ID, 100, 3,SL_TRACE,"RU re-started"));
	return (1);
}

/* 
 * el16watch_dog (bd)
 *
 * Calling/Exit State:
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
	int		i;


	for (i=0, bd=el16config; i < el16boards; bd++, i++) {

		/* LINTED pointer alignment */
		bdd = (bdd_t *)bd->bd_dependent1;

		/* LINTED pointer alignment */
		scb = bdd->scb;

		/*
		 *  Store error statistics in the MIB structure.
		 */
		bd->mib.ifSpecific.etherAlignErrors   += scb->scb_aln_err;
		bd->mib.ifSpecific.etherCRCerrors     += scb->scb_crc_err;
		bd->mib.ifSpecific.etherMissedPkts    += scb->scb_rsc_err;
		bd->mib.ifSpecific.etherOverrunErrors += scb->scb_ovrn_err;
		bd->mib.ifInErrors =	scb->scb_aln_err + scb->scb_crc_err +
					scb->scb_rsc_err + scb->scb_ovrn_err;
		bd->ifstats->ifs_ierrors = bd->mib.ifInErrors;

		scb->scb_aln_err = 0;
		scb->scb_crc_err = 0;
		scb->scb_rsc_err = 0;
		scb->scb_ovrn_err = 0;

		if (bd->timer_val > 0)
			bd->timer_val--;

		if (bd->timer_val == 0) {
			bd->timer_val = -1;
                        bd->flags &= ~(TX_BUSY | TX_QUEUED);
			/*
	 		 *+ debug message
			 */
			cmn_err (CE_NOTE,"%s board %d timed out.",el16id_string,bd->bd_number);
			el16init_586 (bd);
			el16timer_id = timeout (el16watch_dog, 0, EL16_TIMEOUT);
			return;
		}
	}

	/*
	 * reset timeout to 5 seconds
	 */
	el16timer_id = timeout (el16watch_dog, 0, EL16_TIMEOUT);
        return;
}

/* 
 * el16promisc_on (bd)
 *
 * Calling/Exit State:
 *
 * Description :
 *  turn on promiscuous mode.
 *
 */
el16promisc_on (bd)
DL_bdconfig_t	*bd;
{
	int	old;

	/*
	 *  If already in promiscuous mode, just return.
	 */
	if (bd->promisc_cnt++)
		return (0);

	old = splstr();
	if (el16config_586 (bd, PRO_ON)) {
		splx(old);
		return (1);
	}

	splx(old);
	return (0);
}

/* 
 * el16promisc_off (bd)
 *
 * Calling/Exit State:
 *
 * Description :
 *  turn off promiscuous mode.
 *
 */
el16promisc_off (bd)
DL_bdconfig_t	*bd;
{
	int	old;

	/*
	 *  If the board is not in a promiscuous mode, just return;
	 */
	if (!bd->promisc_cnt)
		return (0);

	/*
	 *  If this is not the last promiscuous SAP, just return;
	 */
	if (--bd->promisc_cnt)
		return (0);

	/*
	 *  Save the current page then go to page two and read the current
	 *  receive configuration.
	 */

	old = splstr();
	if (el16config_586 (bd, PRO_OFF))
	{
		splx(old);
		return (1);
	}

	splx(old);
	return (0);
}

#ifdef ALLOW_SET_EADDR
/* 
 *  el16set_eaddr()
 *
 * Calling/Exit State:
 *
 * Description :
 *  set physical address.
 *
 */
el16set_eaddr (bd, eaddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*eaddr;
{
	return (1);	/* not supported yet */
}
#endif

/* 
 *  el16add_multicast()
 *
 * Calling/Exit State:
 *
 * Description :
 *  add multicast address.
 *
 */
el16add_multicast (bd, maddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*maddr;
{
	/* LINTED pointer alignment */
	bdd_t   *bdd = (bdd_t *)bd->bd_dependent1;
	mcat_t  *mcp = &(bdd->el16_multiaddr[0]);
	int i;
	int rval, old;

	old = splstr();
	if ((bd->multicast_cnt >= MULTI_ADDR_CNT) || (!maddr->bytes[0] & 0x01)){
		splx(old);
		return 1;
	}
	if ( el16is_multicast(bd,maddr)) {
		splx(old);
		return 0;
	}
	for (i = 0; i < MULTI_ADDR_CNT; i++,mcp++) {
		if (!mcp->status)
			break;
	}
	mcp->status = 1;
	bd->multicast_cnt++;
	bcopy((caddr_t)maddr->bytes,(caddr_t)mcp->entry,DL_MAC_ADDR_LEN);
	rval = el16set_multicast(bd);
	splx(old);
	return rval;
}

/* ARGSUSED */
/* 
 * el16set_multicast(DL_bdconfig_t *bd)
 *
 * Calling/Exit State:
 *
 * Description :
 *  set multicast address hash table.
 *
 */
el16set_multicast(bd)
DL_bdconfig_t *bd;
{
	/* LINTED pointer alignment */
	bdd_t	*bdd	 = (bdd_t *) bd->bd_dependent1;
	mcad_t	mcad;
	mcat_t	*mcp	 = &(bdd->el16_multiaddr[0]);
	/* LINTED pointer alignment */
	scb_t	*scb	 = bdd->scb;
	/* LINTED pointer alignment */
	cmd_t	*gen_cmd = (cmd_t *) (bdd->virt_ram + bdd->ofst_gen_cmd);
	ushort	base_io = bd->io_start;
	int		i, j;

	if (el16wait_scb (scb))
		return (1);

	scb->scb_status   = 0;
	scb->scb_cmd      = SCB_CUC_STRT;
	scb->scb_cbl_ofst = bdd->ofst_gen_cmd;

	/* LINTED pointer alignment */
	gen_cmd->cmd_status   = 0;
	gen_cmd->cmd_cmd      = CS_CMD_MCSET | CS_EL;
	gen_cmd->cmd_nxt_ofst = 0xFFFF;

	for (i = 0, j = 0; i < MULTI_ADDR_CNT; i++, mcp++)
		if (mcp->status) {
			bcopy((caddr_t)mcp->entry, (caddr_t)&(mcad.mc_addr[j]), DL_MAC_ADDR_LEN);
			j += DL_MAC_ADDR_LEN;
		}
	/* end for */
	mcad.mc_cnt = (ushort)j;
	for (i=0; i<j; i++)
		gen_cmd->prmtr.prm_mcad.mc_addr[i] = mcad.mc_addr[i];

	if (bus_p & BUS_MCA) {
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
 *
 * Description :
 *  get multicast addresses.
 *
 */
el16get_multicast(bd,mp)
DL_bdconfig_t *bd;
mblk_t *mp;
{
	/* LINTED pointer alignment */
	bdd_t *bdd = (bdd_t *)bd->bd_dependent1;
	mcat_t  *mcp = &(bdd->el16_multiaddr[0]);
	int i;
	unsigned char *dp;
	int found = 0;

	if((int)(mp->b_wptr - mp->b_rptr) == 0)
		found = bd->multicast_cnt;
	else {
		dp = mp->b_rptr;
		for (i = 0;(i < MULTI_ADDR_CNT) && (dp < mp->b_wptr);i++,mcp++)
			if (mcp->status) {
				BCOPY(mcp->entry,dp,DL_MAC_ADDR_LEN);
				dp += DL_MAC_ADDR_LEN;
				found++;
			}
		mp->b_wptr = dp;
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
el16del_multicast (bd, maddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*maddr;
{
	int i;
	/* LINTED pointer alignment */
	bdd_t   *bdd = (bdd_t *)bd->bd_dependent1;
	mcat_t  *mcp = &(bdd->el16_multiaddr[0]);
	int rval, old ;

	old = splstr();

	if (!el16is_multicast(bd,maddr))
		rval = 1;
	else {
		for (i = 0; i < MULTI_ADDR_CNT; i++,mcp++)
			if ( (mcp->status) && (BCMP(maddr->bytes,mcp->entry,DL_MAC_ADDR_LEN) == 0) )
				break;
		mcp->status = 0;
		bd->multicast_cnt--;
		rval = el16set_multicast(bd);
	}
	splx(old);
	return rval;
}

/* ARGSUSED */
/* 
 * el16disable
 *
 * Calling/Exit State:
 *
 * Description :
 *  disable the el16.
 *
 */
el16disable (bd)
DL_bdconfig_t	*bd;
{
	return (1);	/* not supported yet */
}

/* 
 *  el16enable()
 *
 * Calling/Exit State:
 *
 * Description :
 * 	enable el16 board.
 *
 */
/* ARGSUSED */
el16enable (bd)
DL_bdconfig_t	*bd;
{
	return (1);	/* not supported yet */
}

/* 
 *  el16reset()
 *
 * Calling/Exit State:
 *
 * Description :
 *  reset el16 board.
 *
 */
/* ARGSUSED */
el16reset (bd)
DL_bdconfig_t	*bd;
{
	return (1);	/* not supported yet */
}

/* 
 *  el16is_multicast()
 *
 * Calling/Exit State:
 *
 * Description :
 *  check if the address is a multicast address.
 *
 */
/*ARGSUSED*/
el16is_multicast (bd, eaddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*eaddr;
{
	/* LINTED pointer alignment */
	bdd_t *bdd = (bdd_t *)bd->bd_dependent1;
	mcat_t *mcp = &bdd->el16_multiaddr[0];
	int i;
	int rval = 0;
	int old;

	old = splstr();
	if (bd->multicast_cnt) {
		for (i = 0; i < MULTI_ADDR_CNT;i++,mcp++) {
			if ( (mcp->status) && (BCMP(eaddr->bytes,mcp->entry,DL_MAC_ADDR_LEN) == 0) ) {
				rval = 1;
				break;
			}
		}
	}
	splx(old);
	return (rval);
}

/******************************************************************************
 *  Support routines for Berkeley Packet Filter (BPF).
 */
#ifdef NBPFILTER

/* 
 * el16bpf_ioctl (ifp, cmd, addr)
 *
 * Calling/Exit State:
 *
 * Description :
 *  BPF ioctl.
 *
 */
STATIC
el16bpf_ioctl (ifp, cmd, addr)
struct	ifnet	*ifp;
int	cmd;
{
	return(EINVAL);
}

/* 
 * el16bpf_output (ifp, buf, dst)
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 * Description :
 *  bpf output.
 *
 */
STATIC
el16bpf_output (ifp, buf, dst)
struct	ifnet	*ifp;
uchar_t	*buf;
struct	sockaddr *dst;
{
	return(EINVAL);
}

/* 
 * el16bpf_promisc (ifp, flag)
 *
 * Calling/Exit State:
 *
 * Description :
 *  set/reset bpf promiscuous mode.
 *
 */
STATIC
el16bpf_promisc (ifp, flag)
struct	ifnet	*ifp;
int	flag;
{
	if (flag)
		return(el16promisc_on((DL_bdconfig_t*)ifp->if_next));
	else
		return(el16promisc_off((DL_bdconfig_t*)ifp->if_next));
}
#endif

#ifdef C_PIO
/* ARGSUSED */
/* 
 * mybcopy(char *, char*, int )
 *
 * Calling/Exit State:
 *
 * Description :
 *  copy util for el16
 *
 */
void
mybcopy(src, dest, count)
char *src;
char *dest;
int  count;
{
	int i;
	for (i=0; i< count; i++)
		dest[i] = src[i];
}
#endif
