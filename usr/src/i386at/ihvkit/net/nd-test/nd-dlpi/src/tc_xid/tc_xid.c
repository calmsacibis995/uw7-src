/*
 * tc_xid.c: Testcase DL_XID_REQ transactions..
 */
#include <stdio.h>
#include <fcntl.h>
#include <stropts.h>
#include <errno.h>
#include <signal.h>
#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>
#include <dl.h>
#include <tc_res.h>
#include <tet_api.h>
#include <tc_net.h>
#include <config.h>
#include <tc_msg.h>

#include "tc_xid_msg.h"

extern int	dl_errno;

extern char *	tn_init_err[];

void tc_sxid();
void tc_exid();

void ic_xid0();
void ic_xid1();
void ic_xid2();
void ic_xid3();
void ic_xid4();
/*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_sxid; /* Test case start snd */
void (*tet_cleanup)() = tc_exid; /* Test case end snd */

struct tet_testlist tet_testlist[] = {
	{ic_xid0, 1},
	{ic_xid1, 2},
	{ic_xid2, 3},
	{ic_xid3, 4},
	{ic_xid4, 5},
	{NULL, -1}
};

extern void	handler();

int	timeout;
int	timer;
int	avoid_panic;
/*
 * Test the DL_XID_REQ primitive..
 */
void
tc_sxid()
{
	dl_info_ack_t	infoack;
	char		*sval;
	int		ret;
	int		i;
	
	tet_infoline(TC_XID);
	/*
	 * Initialize the net configuration ..
	 */	
	if ((ret = tn_init()) < 0) {
		tet_infoline(NET_INIT_FAIL);
		ret -= ret;	 /* convert to positive val */
		if (ret > 0 && ret < NET_INIT_ERRS)	
			tet_infoline(tn_init_err[-ret]);
		for (i = 0; i < IC_MAX; i++)
			tet_delete(i+1, NET_INIT_FAIL); 
		return;
	}
	/*
	 * Check if it can talk to it's neighbours or not ..
	 */
	if (tn_node == 0) {
		/*
		 * Check if it can talk to the neighbours or not 
		 */
		tn_monitor(TN_LISTEN);
		if (tn_neighbours() != PASS) {
			tet_infoline(CONNECTIVITY_FAIL);
			for (i = 0; i < IC_MAX; i++)
				tet_delete(i + 1, CONNECTIVITY_FAIL);
		} 
		/*
		 * kill the listener ...
		 */
		kill(tn_listener, 9);
	}

	/*
	 * get the timeout value..
	 */
	if ((sval = tet_getvar("GETMSG_TIMEOUT")) == NULL) {
		tet_infoline(GETMSG_TIMEOUT_FAIL);
		/*
		 * default is 1 sec
		 */
		timeout = 1;
	}
	else
		timeout = atoi(sval);
	return;
}

/* 
 * DL_XID_REQ should be successful for proper arguments
 * in proper DLP state..
 */ 
void
ic_xid0()
{
	int	dl_retry;
	int	i, retval;

	tet_infoline(XIDREQ_PASS);

	/*
	 * start the listener .. also
	 */
	tn_monitor(TN_LISTEN);
	/*
	 *  initiate the test ..
	 */
	*tn_eventp = DL_NOEVENT;
	dl_retry = DL_RETRY_CNT;
	
	while ((*tn_eventp != DL_XIDCON) && dl_retry--) {
		/*
		 * Retry till the monitor node receives thr XIDCON packet
		 */
		dl_xidreq(tn_listen_fd, tn_net[TN_NEXT_TO_MON].dn_llc_addr,
				sizeof(struct llcc));
		sleep(DL_RETRY_TIMEOUT);
		/*
		 *  Wait for packet .. 
		 */
	}
	DL_DEBUG1(2,"dl_xidreq: event [%x]\n", *tn_eventp);
	if (*tn_eventp == DL_XIDCON) {
		tet_result(TET_PASS);
	}
	else {
		/*
 		 * Test if tn_link passes or not ..
		 */
		if (tn_link(tn_net[TN_NEXT_TO_MON]) != PASS) {
			tet_infoline(LINK_FAIL);
			tet_result(TET_UNRESOLVED);
		}
		else {	
			tet_infoline(XIDREQ_FAIL);
			tet_result(TET_FAIL);
		}
	}
	/*
	 * kill the listener ...
	 */
	kill(tn_listener, 9);
	return;
}

/*
 * DL_XID_REQ with invalid argument .. could be PANIC!!!
 */
void
ic_xid1()
{
	int			i, flags;
	struct strbuf		ctlbuf;
	union DL_primitives	rcvbuf;
	dl_xid_req_t		xidreq;
	dl_error_ack_t		errack;
	long			badaddr;
	int			retval;

	tet_infoline(XIDREQ_INVALARG_PASS1);

	ctlbuf.len = sizeof(dl_xid_req_t);
	ctlbuf.buf = (char *)&xidreq; 
	
	xidreq.dl_primitive = DL_XID_REQ;
	xidreq.dl_dest_addr_length = -1;	
	/*
	 * get a bad address .. to expose a potential panic for sure..
	 */
	if (getpanicaddr(&badaddr) < 0) {
		tet_infoline(GETPANICADDR_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}
	xidreq.dl_dest_addr_offset = badaddr;

	if (putmsg(tn_listen_fd, &ctlbuf, NULL, 0) < 0) {
		tet_msg(PUTMSG_FAIL, errno);
		tet_result(TET_UNRESOLVED);
		return;
	}	

	/*
	 * wait for confirmation  of test request..DL_XID_CON   
	 */
	ctlbuf.maxlen = sizeof(union DL_primitives);
	ctlbuf.len = 0;
	ctlbuf.buf = (char *)&rcvbuf;

	flags = RS_HIPRI;

	if (getmsg(tn_listen_fd, &ctlbuf, NULL, &flags) < 0) {
		tet_msg(GETMSG_FAIL, errno);
		tet_result(TET_UNRESOLVED);
		return;
	}
	
	/*
	 * DL_ERROR_ACK is expected..
	 */
	switch (rcvbuf.dl_primitive) {
		case DL_ERROR_ACK:
			tet_result(TET_PASS);
		case DL_UDERROR_IND:
			tet_infoline(XIDREQ_INVALARG_PASS1);
			tet_result(TET_PASS);
			return;
		default:
			return;		
	}	 
}


/*
 * Try DL_XID_REQ with invalid size message .. should not PANIC.
 */
void 
ic_xid2()
{
	struct	strbuf		ctlbuf;
	dl_xid_req_t		testreq;
	int			i, flag, retval;
	int			len;
	char			buf[DL_MAX_PKT];

	
	tet_infoline(XID_WRONG_SZ_PASS);

	/*	
	 * make a request with truncated DL_XID_REQ message ..
	 */
	testreq.dl_primitive = DL_XID_REQ;
	testreq.dl_flag	 = DL_POLL_FINAL;
	/*
	 * set ctlbuf.len = sizeof(long), so that the stream head does not 
 	 * pass the entire control part to the driver.. but good enough to 
	 * take the control to DLxid_req routine .. 
	 * driver should not misbehave ...
	 */
	ctlbuf.len = sizeof(long);
	ctlbuf.buf = (char *)&testreq;

	if (putmsg(tn_listen_fd, &ctlbuf, NULL, 0) < 0) {
		tet_msg(PUTMSG_FAIL, errno);
		tet_result(TET_UNRESOLVED);
		return;
	}
	else {
		tet_result(TET_PASS);
	}	

	return;
}

/* 
 * DL_XID_REQ with bad DLSAP address format .. (DL_BADADDR) 
 */
void
ic_xid3()
{
	struct	strbuf		ctlbuf;
	dl_xid_req_t		testreq;
	dl_error_ack_t		errack;
	int			i, flag, retval;
	int			len;
	char			buf[DL_MAX_PKT];

	tet_infoline(BADADDR_PASS);

	/*
	 * attach alarm handelr
	 */
	signal(SIGALRM, (void (*)())handler);
	
	/*	
	 * make a request with invalid DLSAP address format ..
	 */
	testreq.dl_primitive = DL_XID_REQ;
	testreq.dl_flag	= DL_POLL_FINAL;
	testreq.dl_dest_addr_length = 0; /* not sizeof(dl_addr_t); */
	testreq.dl_dest_addr_offset = 0; /* not sizeof(dl_xid_req_t); */

	ctlbuf.len = sizeof(dl_xid_req_t);
	ctlbuf.buf = (char *)&testreq;

	if (putmsg(tn_listen_fd, &ctlbuf, NULL, 0) < 0) {
		tet_msg(PUTMSG_FAIL, errno);
		tet_result(TET_UNRESOLVED);
		return;
	}
	
	/*
	 * Expecting DL_ERROR_ACK with DL_BADADDR..
	 */
	ctlbuf.maxlen = sizeof(dl_error_ack_t);
	ctlbuf.len = 0;
	ctlbuf.buf = (char *)&errack;

	/*
	 * start a alarm..
	 */	
	timer = 0;
	alarm(timeout);
	flag = 0;  
	if ((retval = getmsg(tn_listen_fd, &ctlbuf, NULL, &flag)) < 0) {
		if ((errno == EINTR) && (timer)) {
			tet_infoline(GETMSG_TIMEOUT);
			tet_infoline(BADADDR_TIMEOUT);
			tet_result(TET_FAIL);
			return;
		}	
		/*
		 * not enough to conclude..
		 */
		tet_msg(GETMSG_FAIL, errno);
		tet_infoline(TET_UNRESOLVED);
		return;
	}	
	/*
	 * cancel the timer..
	 */
	alarm(0); 
	/*
	 * check for DL_ERROR_ACK..
	 */	

	if (errack.dl_primitive == DL_ERROR_ACK) {
		/*
		 * check for expected error value..
		 */
		if (errack.dl_errno == DL_BADADDR) {
			tet_result(TET_PASS);	
		}
		else { 
			tet_msg(BADADDR_FAIL, errack.dl_errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(BADADDR_FAIL0);
		tet_result(TET_FAIL);
	}	
	return;
}

/* 
 * DL_XID_REQ  issued from an invalid state .. (DL_OUTSTATE)
 * Keep it as the last IC .. it takes stream to a UNBOUND state
 * and does not restore 
 */
void
ic_xid4()
{
	struct	strbuf		ctlbuf;
	dl_xid_req_t		testreq;
	dl_error_ack_t		errack;
	char			*dest, *src;
	int			i, flag, retval;
	int			len;
	char			buf[DL_MAX_PKT];

	tet_infoline(OUTSTATE_PASS);

	/*
	 * attach alarm handelr
	 */
	signal(SIGALRM, (void (*)())handler);
	/*
	 * put the stream out of state ..use unbind ..
	 */
	if (dl_unbind(tn_listen_fd) < 0) {
		tet_infoline(UNBIND_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}

	/*	
	 * make a request in invalid state..
	 */
	testreq.dl_primitive = DL_XID_REQ;
	testreq.dl_flag	 = DL_POLL_FINAL;
	testreq.dl_dest_addr_length = sizeof(dl_addr_t); 
	testreq.dl_dest_addr_offset = sizeof(dl_xid_req_t);
	/*
	 * Send it to the next neighbour ..
	 */
	dest = (char *)&testreq + testreq.dl_dest_addr_offset;
	src = (char *)&tn_net[TN_NEXT_TO_MON].dn_llc_addr;
	for (i = 0; i < sizeof(dl_addr_t); i++)  
		*dest++ = *src++;

	ctlbuf.len = sizeof(dl_xid_req_t) + sizeof(dl_addr_t);
	ctlbuf.buf = (char *)&testreq;

	if (putmsg(tn_listen_fd, &ctlbuf, NULL, 0) < 0) {
		tet_msg(PUTMSG_FAIL, errno);
		tet_result(TET_UNRESOLVED);
		return;
	}
	
	/*
	 * Expecting DL_ERROR_ACK with DL_OUTSTATE..
	 */
	ctlbuf.maxlen = sizeof(union DL_primitives);
	ctlbuf.len = 0;
	ctlbuf.buf = (char *)&errack;

	/*
	 * start a alarm..
	 */	
	timer = 0;
	alarm(timeout);
	flag = 0; 
	if ((retval = getmsg(tn_listen_fd, &ctlbuf, NULL, &flag)) < 0) {
		if ((errno == EINTR) && (timer)) {
			tet_infoline(GETMSG_TIMEOUT);
			tet_result(TET_FAIL);
			return;
		}	
		/*
		 * not enough to conclude..
		 */
		tet_msg(GETMSG_FAIL, errno);
		tet_infoline(TET_UNRESOLVED);
		return;
	}	
 
	/*
	 * check for DL_ERROR_ACK..
	 */	

	if (errack.dl_primitive == DL_ERROR_ACK) {
		/*
		 * check for expected error value..
		 */
		if (errack.dl_errno == DL_OUTSTATE) {
			tet_result(TET_PASS);	
		}
		else { 
			tet_msg(OUTSTATE_FAIL,errack.dl_errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(OUTSTATE_FAIL0);
		tet_result(TET_FAIL);
	}	
	return;
}

void
tc_exid()
{
}

void
handler()
{
	timer = 1;
	/*
	 * for a change don't need to put back the handler ..
	 */
}

