/*	Copyright (c) 1993 UNIVEL					*/

#ident	"@(#)sr_wait.c	10.1"

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

extern int sr_debug;
extern int sr_wait_tab_sem;
extern int sr_waiting;
extern ulong sr_timerid;

sr_wait_elem_t sr_wait_tab[MAX_WAIT_TAB_SIZE];

void send_local_test();
void send_remote_test();
void sr_timeout();


sr_wait_tab_init()
{
register int i;
sr_wait_elem_t *sr_elemp = sr_wait_tab;


	sr_waiting = 0;

	for (i =0; i < MAX_WAIT_TAB_SIZE; i++,sr_elemp++) {
		sr_elemp->sr_state = SR_UNUSED;
		sr_elemp->sr_streamq = NULL;
		sr_elemp->sr_waitq.q_first = NULL;
		sr_elemp->sr_waitq.q_last = NULL;
	}
}

sr_send_local_test(q,macaddr)
queue_t *q;
unsigned char *macaddr;
{
mblk_t *mp;
unsigned char *tmp;
dl_test_req_t *test_req;
struct srdev *srp = q->q_ptr;


	if ((mp = allocb(TEST_DATA_SIZE,BPRI_MED)) == NULL)
		return;
	mp->b_datap->db_type = M_PROTO;
	test_req = (dl_test_req_t *)mp->b_rptr;
	test_req->dl_primitive = DL_TEST_REQ;
	test_req->dl_dest_addr_offset = DL_TEST_REQ_SIZE;
	test_req->dl_flag = 0;
	tmp = mp->b_rptr + DL_TEST_REQ_SIZE;
	COPY_MACADDR(macaddr,tmp);
	tmp += MAC_ADDR_LEN;
	if (srp->std_addr_length == (MAC_ADDR_LEN + 2)) {
		test_req->dl_dest_addr_length = MAC_ADDR_LEN + SR_NULL_SAP_SIZE
						+ SR_NULL_SAP_SIZE;
		*(ushort *)tmp = LLC_NULL_SAP;
		mp->b_wptr = mp->b_rptr + DL_TEST_REQ_SIZE + MAC_ADDR_LEN +
					SR_NULL_SAP_SIZE + SR_NULL_SAP_SIZE;
	} else {
		test_req->dl_dest_addr_length = MAC_ADDR_LEN + SR_NULL_SAP_SIZE;
		*tmp = LLC_NULL_SAP;
		mp->b_wptr = mp->b_rptr + DL_TEST_REQ_SIZE + MAC_ADDR_LEN +
					SR_NULL_SAP_SIZE;
	}
	putnext(q,mp);

#ifdef	DBG
	if (sr_debug)	{
		printf("SR: sent local test to ");
		print_macaddr(macaddr);
	}
#endif

}

void
sr_send_remote_test(q,macaddr)
queue_t *q;
unsigned char *macaddr;
{
mblk_t *mp;
unsigned char *tmp;
dl_test_req_t *test_req;
struct srdev *srp = q->q_ptr;


	if ((mp = allocb(TEST_DATA_SIZE,BPRI_MED)) == NULL)
		return;
	mp->b_datap->db_type = M_PROTO;
	test_req = (dl_test_req_t *)mp->b_rptr;
	test_req->dl_primitive = DL_TEST_REQ;
	test_req->dl_dest_addr_offset = DL_TEST_REQ_SIZE;
	test_req->dl_flag = 0;
	tmp = mp->b_rptr + DL_TEST_REQ_SIZE;
	COPY_MACADDR(macaddr,tmp);
	tmp += MAC_ADDR_LEN;
	if (srp->std_addr_length == (MAC_ADDR_LEN + 2)) {
		test_req->dl_dest_addr_length = MAC_ADDR_LEN + SR_NULL_SAP_SIZE                                      + SR_NULL_SAP_SIZE + INIT_ROUTE_HDR_SIZE;
		*((ushort *)tmp) = LLC_NULL_SAP;
		tmp += 2;
		mp->b_wptr = mp->b_rptr + DL_TEST_REQ_SIZE + MAC_ADDR_LEN +
				SR_NULL_SAP_SIZE + SR_NULL_SAP_SIZE
				+ INIT_ROUTE_HDR_SIZE;
	} else {
		test_req->dl_dest_addr_length = MAC_ADDR_LEN + SR_NULL_SAP_SIZE
				+ INIT_ROUTE_HDR_SIZE;
		*tmp++ = LLC_NULL_SAP;
		mp->b_wptr = mp->b_rptr + DL_TEST_REQ_SIZE + MAC_ADDR_LEN +
				SR_NULL_SAP_SIZE + INIT_ROUTE_HDR_SIZE;
	}
	*tmp++ = SINGLE_ROUTE_BCAST | INIT_ROUTE_HDR_SIZE;
	*tmp = LARGEST_FRAME_SIZE;	/* The direction bit is 0 */
	putnext(q,mp);

#ifdef	DBG
	if (sr_debug)	{
		printf("SR: sent remote test to ");
		print_macaddr(macaddr);
	}
#endif

}

sr_wait_tab_cleanup(q)
queue_t *q;
{
register int i;
sr_wait_elem_t *sr_elemp = sr_wait_tab;


	for (i = MAX_WAIT_TAB_SIZE; i; i--,sr_elemp++) {
		if (sr_elemp->sr_state == SR_UNUSED)
			continue;
		if (sr_elemp->sr_streamq == q) {
			(void)sr_queue_flush(sr_elemp);
			sr_elemp->sr_streamq = NULL;
			sr_elemp->sr_state = SR_UNUSED;
			sr_waiting--;
		}
	}

}

/* The calling function should protect using splhi()s and splx()s */
sr_add_wait_tab(macaddr,q,mp)
unsigned char *macaddr;
queue_t *q;
mblk_t *mp;
{
register int i;
sr_wait_elem_t *sr_elemp = sr_wait_tab;
sr_wait_elem_t *freep = (sr_wait_elem_t *)NULL;


	for (i = MAX_WAIT_TAB_SIZE; i; i--,sr_elemp++) {
		if (sr_elemp->sr_state == SR_UNUSED)	{
			if (!freep)
				freep = sr_elemp;
			continue;
		}
		if (SAME_MACADDR(macaddr,sr_elemp->sr_wait_macaddr)) {
			(void)sr_put_queue(&(sr_elemp->sr_waitq),mp);

#ifdef	DBG
			if (sr_debug)	{
				printf("SR: queued message on wait table entry ");
				print_macaddr(macaddr);
			}
#endif

			return 0;
		}
	}
	if (freep) {
		freep->sr_state = SR_WAITING_FOR_LOCAL_REPLY;
		freep->sr_timer_val = LOCAL_REPLY_WAIT;
		freep->sr_streamq = q;
		COPY_MACADDR(macaddr,freep->sr_wait_macaddr);
		(void)sr_put_queue(&(freep->sr_waitq),mp);
		(void)sr_send_local_test(q,macaddr);

#ifdef	DBG
		if (sr_debug)	{
			printf("SR: added wait table entry ");
			print_macaddr(macaddr);
		}
#endif

		/* first entry added, fire off timer */
		if (sr_waiting++ == 0)
			sr_timerid = timeout(sr_timeout,0,SR_TIMEOUT * HZ);

		return 0;
	}
	return -1;
}

void
sr_timeout()
{
register int i;
sr_wait_elem_t *sr_elemp;


	if (sr_wait_tab_sem)
		goto finish;
	sr_elemp = sr_wait_tab;
	for (i = MAX_WAIT_TAB_SIZE; i; i--,sr_elemp++) {
		if (sr_elemp->sr_state & (SR_WAITING_FOR_LOCAL_REPLY |
						SR_WAITING_FOR_REMOTE_REPLY)) {
			if (sr_elemp->sr_timer_val > 0)
				sr_elemp->sr_timer_val--;
			if (sr_elemp->sr_timer_val == 0) {
				if (sr_elemp->sr_state == 
						SR_WAITING_FOR_LOCAL_REPLY) {
					sr_elemp->sr_state = 
						SR_WAITING_FOR_REMOTE_REPLY;
					sr_elemp->sr_timer_val =
						REMOTE_REPLY_WAIT;

#ifdef	DBG
					if (sr_debug)
						printf("SR: timed out waiting for local reply\n");
#endif

					(void)sr_send_remote_test(sr_elemp->sr_streamq,sr_elemp->sr_wait_macaddr);
				} else {
					printf("SR: unable to obtain  route for ");
					print_macaddr(sr_elemp->sr_wait_macaddr);
					sr_elemp->sr_state = 
						SR_ROUTE_UNKNOWN;
					sr_elemp->sr_timer_val = -1;
					qenable(sr_elemp->sr_streamq);
				}
			}
		}
	}

finish: 
	if (sr_waiting > 0)
		sr_timerid = timeout(sr_timeout,0,SR_TIMEOUT * HZ);
}

 
/* The calling function should raise the priority to splhi */
void
sr_setup_route_info(macaddr,src_route,sr_size)
unsigned char *macaddr,*src_route;
int sr_size;
{
register int i;
sr_wait_elem_t *sr_elemp = sr_wait_tab;


	for (i = MAX_WAIT_TAB_SIZE; i; i--,sr_elemp++) {
		if (sr_elemp->sr_state & (SR_WAITING_FOR_LOCAL_REPLY |
						SR_WAITING_FOR_REMOTE_REPLY)) {
			if (SAME_MACADDR(sr_elemp->sr_wait_macaddr,macaddr)) {
				sr_elemp->sr_timer_val = -1;
				sr_elemp->sr_state = SR_ROUTE_KNOWN;
				break;
			}
		}
	}
	if (i) {
		if (sr_size)
			bcopy(src_route,sr_elemp->sr_route,sr_size);
		sr_elemp->sr_route_size = sr_size;
		qenable(sr_elemp->sr_streamq);

#ifdef	DBG
		if (sr_debug)	{
			printf("SR: added wait table route info ");
			print_macaddr(macaddr);
			print_route(src_route, sr_size);
		}
#endif

	}
	else	{
		cmn_err(CE_WARN,
			"SR: entry not found, couldn't add wait table route info for ");
		print_macaddr(macaddr);
	}

	return;
}


/*
   The following two routines are used to add and remove message blocks from
   the streams queues associated with every macaddr whose route has not been
   resolved.
*/

sr_put_queue(q,bp)
queue_t *q;
mblk_t *bp;
{
mblk_t *tmp;


	if (!q->q_first) {
		bp->b_next = NULL;
		bp->b_prev = NULL;
		q->q_first = bp;
		q->q_last = bp;
	} else {
		tmp = q->q_last;
		bp->b_next = NULL;
		bp->b_prev = tmp;
		tmp->b_next = bp;
		q->q_last = bp;
	}
	return;
}

mblk_t *
sr_get_queue(q)
queue_t *q;
{
mblk_t *bp = NULL;


	if ((bp = q->q_first)) {
                if (!(q->q_first = bp->b_next))
			q->q_last = NULL;
		else
			q->q_first->b_prev = NULL;
		bp->b_next = NULL;
		bp->b_prev = NULL;
	}
	return bp;
}
