/*	Copyright (c) 1993 UNIVEL					*/

#ident	"@(#)sr_str.c	10.1"

#ifdef _KERNEL
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <mem/kmem.h>
#include <io/dma.h>
#include <fs/file.h>
#include <mem/immu.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <proc/signal.h>
#include <io/conf.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <net/dlpi.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <io/ddi.h>
#include <io/ddi_i386at.h>
#include <util/mod/moddefs.h>
#endif /*_KERNEL */

#include <io/sr/sr.h>

#define		MODNAME		"sr- Loadable source routing module"


#ifdef	DBG
int	sr_debug = 0;
#endif


int sr_waiting = 0;
ulong sr_timerid = -1;
ulong srdevflag = 0;

extern void sr_timeout();
mblk_t *sr_get_queue();
sr_elem_t *sr_tab_lookup();
int sr_load(), sr_unload();

extern sr_wait_elem_t sr_wait_tab[];
ulong sr_wait_tab_sem = 0;

/*
 * Space.c variables
 */
extern int	sr_insert_age();		/* update table with new source
						 * route information only if
						 * if the entry has aged sr_age
						 * ticks
						 */
extern int	sr_insert_latest();		/* update table with latest
						 * source route information
						 */

extern int	(*sr_insert)();			/* by default set in Space.c to
						 * sr_insert_latest
						 */

extern int	sr_age;				/* used only if sr_insert is
						 * set to sr_insert_age
						 */

extern int	sr_broadcast;			/* by default set in Space.c to
						 * SINGLE_ROUTE_BCAST
						 */

/*
 * End of Space.c candidates
 */


MOD_STR_WRAPPER(sr, sr_load, sr_unload, MODNAME);


/* STREAMS related definitions. */
static struct module_info minfo = {
          0, "sr", 0, MAXPKT, HIWAT, LOWAT,
};

int sropen(), srclose(), srwput(), srwsrv(), srrput(), srrsrv();

void sr_wait_tab_init(),sr_tab_uninit();
int  sr_tab_init();

/* read queue initial values */
static struct qinit rinit = {
	srrput, NULL, sropen, srclose, NULL, &minfo, NULL,
};

/* write queue initial values */
static struct qinit winit = {
	srwput, srwsrv, NULL, NULL, NULL, &minfo, NULL,
};

struct streamtab srinfo = { &rinit, &winit, NULL, NULL };
/* End of streams definitions. */

struct srdev srdevs[MAXMINORS];

ushort sr_not_init = 1;

int
sr_load()
{
	return srinit();
}

int
sr_unload()
{
	untimeout(sr_timerid);
	sr_tab_uninit();
	return 0;
}

srinit()
{
	if (sr_tab_init() == -1)
		return ENXIO;
	(void)sr_wait_tab_init();
	sr_not_init = 0;
	cmn_err(CE_CONT,"SR module initialized successfully\n");
	return 0;
}

sropen(q, dev, flag, sflag,credp)
queue_t *q;
dev_t	*dev;
int	flag;
int	sflag;
struct cred	*credp;
{ 
struct srdev *srp = srdevs;
register int i;

	if (sr_not_init)
		return ENXIO;

	for (i = MAXMINORS; i; i--,srp++) {
		if (!srp->sr_qptr)
			break;
	}
	if (!i)
		return ECHRNG;

	if (q->q_ptr)
		return 0;
	WR(q)->q_ptr = (caddr_t) srp;
	q->q_ptr = (caddr_t)srp;
	srp->sr_qptr = WR(q);

#ifdef	DBG
	if (sr_debug)
		cmn_err(CE_CONT,"SR: open succeeded, minor no: %d\n",MAXMINORS - i);
#endif

	return 0;
}

int
srclose(q)
queue_t *q;
{
struct srdev *srp;
int old;

	flushq(q,FLUSHALL);
	flushq(OTHERQ(q),FLUSHALL);
	srp = (struct srdev *)q->q_ptr;
	srp->sr_qptr = NULL;

	old = splhi();
	(void)sr_wait_tab_cleanup(WR(q));
	splx(old);

#ifdef	DBG
	if (sr_debug)
		cmn_err(CE_CONT,"SR: close completed, minor no: %d\n", srp - srdevs);
#endif

	return 0;
}

int
srwput(q,mp)
queue_t *q;
mblk_t *mp;
{
	register struct srdev	*srp = (struct srdev *)q->q_ptr;
	union DL_primitives	*dlp;
	unsigned char		*destaddr;
	unsigned char		*routep;
	int			old;
	sr_elem_t		*sr_elemp;
	dl_unitdata_req_t	*unitdata;


	if (!canput(q->q_next)) {
		freemsg(mp);
		return;
	}
	switch(mp->b_datap->db_type) {
	case M_PROTO:
		dlp = (union DL_primitives *)mp->b_rptr;
		if (dlp->dl_primitive == DL_UNITDATA_REQ) {
			unitdata = (dl_unitdata_req_t *)mp->b_rptr;
			destaddr = (unsigned char *)unitdata + 
				unitdata->dl_dest_addr_offset;

			/*
			 * What about multicast/group addresses?
			 */
			if (BROADCAST(destaddr)) {
				sr_proc_broadcast(mp);
			} else if ((sr_elemp= sr_tab_lookup(destaddr))!= NULL) {
				if (sr_elemp->sr_route_size) {
					routep = (unsigned char *)(destaddr +
				      		unitdata->dl_dest_addr_length);
					bcopy(sr_elemp->sr_route,routep,
						     sr_elemp->sr_route_size);
					mp->b_wptr += sr_elemp->sr_route_size;
					unitdata->dl_dest_addr_length +=
						sr_elemp->sr_route_size;
				}
			} else {
				old = splhi();
				if (sr_add_wait_tab(destaddr,q,mp) == -1)
					freemsg(mp);
				splx(old);
				return;
			}
		}
	break;
	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW)
			flushq(q,FLUSHDATA);
	break;
	case M_IOCTL:
		if (srioctl(q,mp) == 0)
			return;
	break;
	default:
		break;
	}
	putnext(q,mp);
}

srrput(q,mp)
queue_t *q;
mblk_t *mp;
{
	register struct srdev	*srp = (struct srdev *)(q->q_ptr);
	union DL_primitives	*dlp;
	unsigned char		*destaddr;
	unsigned char		*srcaddr;
	unsigned char		*sr_route;
	unsigned char		*routep;
	int			old;
	int			sr_size;
	dl_unitdata_ind_t	*unitdata;
	sr_elem_t		*sr_elemp = NULL;


        switch (mp->b_datap->db_type) {
	case M_PROTO:
	case M_PCPROTO:
		dlp = (union DL_primitives *)mp->b_rptr;
		switch(dlp->dl_primitive) {
		case DL_UNITDATA_IND: {
			unitdata = (dl_unitdata_ind_t *)mp->b_rptr;
			srcaddr = (unsigned char *)unitdata +
					unitdata->dl_src_addr_offset;
			srcaddr[0] &= ~(SOURCE_ROUTE_BIT);
			sr_size = unitdata->dl_src_addr_length -
							 srp->std_addr_length;

			/* adjust length to drop route info for upper layer */
			unitdata->dl_src_addr_length = srp->std_addr_length;

#ifdef	DBG
			if (sr_debug > 1)	{
				printf("SR: srrput DL_UNITDATA_IND, sr_size %d, ", sr_size);
				print_macaddr(srcaddr);
			}
#endif

			/*
			 * Try a small performance improvement, unfortunately,
			 * for IPX and ARP we must look at every incoming
			 * frame, unless we add code there to look for the
			 * source route info
			 */
			if (srp->sr_sap == IP_SAP)
				break;

			/*
			 * another performance improvement? if it's from
			 * myself I don't have to look at the source route
			 */
			if (SAME_MACADDR(srcaddr, srp->macaddr))
				break;

			if (sr_size > 0)	{
				sr_route = (unsigned char *)unitdata +
						unitdata->dl_src_addr_offset +
						srp->std_addr_length;

				/* Change the bcast to a directed value tag */
				*sr_route &= 0x1F;

				/* Reset the direction bit*/
				*(sr_route + 1) ^= SOURCE_ROUTE_DIR_BIT; 

#ifdef	DBG
				if (sr_debug > 1)
					print_route(sr_route, sr_size);
#endif

			}

			/*
			 * should we save only if broadcast destaddr? (ARP or
			 * IPX route request)
			 */
			/*
			destaddr = (unsigned char *)unitdata +
					unitdata->dl_dest_addr_offset;
			*/
			(void)(*sr_insert)(srcaddr,sr_route,sr_size);

			break;
		}
		case DL_TEST_CON: {
			dl_test_con_t *test_con = (dl_test_con_t *)mp->b_rptr;


			srcaddr = (unsigned char *)test_con +
					test_con->dl_src_addr_offset;
			srcaddr[0] &= ~(SOURCE_ROUTE_BIT);
			sr_size = test_con->dl_src_addr_length - 
					srp->std_addr_length; 

#ifdef	DBG
			if (sr_debug)	{
				printf("SR: srrput DL_TEST_CON, sr_size %d, ", sr_size);
				print_macaddr(srcaddr);
			}
#endif

			if (sr_size > 0) {
				sr_route = (unsigned char *) (srcaddr + 
						srp->std_addr_length);
				/* Change the bcast to a directed value tag */
				*sr_route &= 0x1F;
				/* Reset the direction bit*/
				*(sr_route + 1) ^= SOURCE_ROUTE_DIR_BIT;

#ifdef	DBG
				if (sr_debug)
					print_route(sr_route, sr_size);
#endif

			}

			old = splhi();
			(void)sr_setup_route_info(srcaddr,sr_route,sr_size);
			splx(old);
			freemsg(mp);
			return;
                	break;
		}
		case DL_BIND_ACK: {
			dl_bind_ack_t *bind_ack = (dl_bind_ack_t *)mp->b_rptr;
			unsigned char *macp;
			int k;


#ifdef	DBG
			if (sr_debug > 1)
				printf("SR: srrput DL_BIND_ACK\n");
#endif

			srp->sr_sap = bind_ack->dl_sap;

			srp->std_addr_length = bind_ack->dl_addr_length;

			/*
			 * This really needs to be fixed in the ibmtok driver,
			 * which sets bind_ack->dl_addr_length to 8 regardless
			 * of the type (DL_802 or DL_SNAP)
			 */
#define	MAXSAPVALUE	0xff
#define	LLC_SAP_LEN	1
			if (srp->sr_sap <= MAXSAPVALUE)
				srp->std_addr_length = MAC_ADDR_LEN + LLC_SAP_LEN;

			macp = (unsigned char *)bind_ack +
				bind_ack->dl_addr_offset;
			COPY_MACADDR(macp, srp->macaddr);

			/* This cannot and should not fail */
			(void)(*sr_insert)(macp,NULL,0);
			break;
		}
		case DL_XID_IND:

#ifdef	DBG
			if (sr_debug > 1)
				printf("SR: srrput DL_XID_IND\n");
#endif

			freemsg(mp);
			return;
		break;
		case DL_XID_CON:

#ifdef	DBG
			if (sr_debug > 1)
				printf("SR: srrput DL_XID_CON\n");
#endif

			freemsg(mp);
			return;
		break;
		default:
		break;
		}
	break;
	case M_FLUSH:
		if (*mp->b_rptr & FLUSHR)
			flushq(q,FLUSHDATA);
	default:
                break;
	}
        putnext(q,mp);
}

/*
   Need to verify whether we need a service procedure. This operation can 
   probably be done in the read side put procedure itself.
*/

void
sr_queue_flush(sr_elemp)
sr_wait_elem_t *sr_elemp;
{
mblk_t *mp;


	while ((mp = sr_get_queue(&(sr_elemp->sr_waitq))) != NULL) 
		freemsg(mp);
}

srwsrv(q)
queue_t *q;
{
register int i;
sr_wait_elem_t *sr_elemp;
int old;
mblk_t *mp;
unsigned char *routep;
dl_unitdata_req_t *unitdata;


	old = splhi();
	sr_wait_tab_sem = 1;
	splx(old);

	sr_elemp = sr_wait_tab;
	for (i = MAX_WAIT_TAB_SIZE; i; i--,sr_elemp++) {
		if (sr_elemp->sr_state == SR_ROUTE_UNKNOWN) {

#ifdef	DBG
			if (sr_debug)	{
				printf("SR: srwsrv flushing data to unknown address ");
				print_macaddr(sr_elemp->sr_wait_macaddr);
			}
#endif

			sr_waiting--;
			sr_queue_flush(sr_elemp);
			sr_elemp->sr_state = SR_UNUSED;
			continue;
		} else if (sr_elemp->sr_state == SR_ROUTE_KNOWN) {

#ifdef	DBG
			if (sr_debug)
				printf("SR: srwsrv moving outbound and resolved data\n");
#endif

			sr_waiting--;
			if ((*sr_insert)(sr_elemp->sr_wait_macaddr,
			    sr_elemp->sr_route, sr_elemp->sr_route_size)== -1) {
				sr_queue_flush(sr_elemp);
				sr_elemp->sr_state = SR_UNUSED;
				continue;
			}
			while ((mp = sr_get_queue(&(sr_elemp->sr_waitq))) 
							        != NULL){

#ifdef	DBG
				if (sr_debug)	{
					printf("SR: srwsrv sending to ");
					print_macaddr(sr_elemp->sr_wait_macaddr);
				}
#endif

				if (sr_elemp->sr_route_size) {
					/*
					  Assuming the last 7/8 bytes are the
					  the mac address and dsap. The src
					  route is tagged to the end of the
					  destination address.
 					*/
					routep = mp->b_wptr; 
                                       	bcopy(sr_elemp->sr_route,routep,
                                                    sr_elemp->sr_route_size);
                                       	mp->b_wptr += 
						sr_elemp->sr_route_size;
					unitdata = (dl_unitdata_req_t *)mp->b_rptr;
					unitdata->dl_dest_addr_length +=
						sr_elemp->sr_route_size;

#ifdef	DBG
					if (sr_debug)
						print_route(routep, sr_elemp->sr_route_size);
#endif

				}
				putnext(sr_elemp->sr_streamq,mp);
			}
			sr_elemp->sr_state = SR_UNUSED;
		}
	}
	old = splhi();
	sr_wait_tab_sem = 0;
	splx(old);
}


int
srioctl(q,mp)
queue_t *q;
mblk_t *mp;
{
struct iocblk *iocp = (struct iocblk *)mp->b_rptr;

	if (iocp->ioc_cmd == SR_DUMP_ROUTE_TABLE) {
		iocp->ioc_error = 0;
		if ( (iocp->ioc_count == 0) || 
			(iocp->ioc_count % BASIC_ROUTE_INFO_SIZE) ) {
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
		} else {
			iocp->ioc_rval = sr_dump_route_table(mp->b_cont); 
			iocp->ioc_count = iocp->ioc_rval * 
						BASIC_ROUTE_INFO_SIZE;
			iocp->ioc_error = 0;
			mp->b_datap->db_type = M_IOCACK;
		}
		qreply(q,mp);
		return 0;
	}
	return -1;
}
