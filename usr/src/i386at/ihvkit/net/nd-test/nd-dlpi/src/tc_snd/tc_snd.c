/*
 * tc_snd.c: Testcase DL_UNITDATA_REQ transactions..
 */
#include <stdio.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/dlpi.h>
#include <sys/errno.h>
#include <sys/dlpi_ether.h>
#include <signal.h>
#include <dl.h>
#include <tc_res.h>
#include <tet_api.h>
#include <tc_net.h>
#include <config.h>
#include <tc_msg.h>

#include "tc_snd_msg.h"

extern int	errno;
extern int	dl_errno;

extern char *	tn_init_err[];

void tc_ssnd();
void tc_esnd();

void ic_snd0();
void ic_snd1();
void ic_snd2();
void ic_snd3();
void ic_snd4();
void ic_snd5();
void ic_snd6();
void ic_snd7();
void ic_snd8();
/*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_ssnd; /* Test case start snd */
void (*tet_cleanup)() = tc_esnd; /* Test case end snd */

struct tet_testlist tet_testlist[] = {
	{ic_snd0, 1},
	{ic_snd1, 2},
	{ic_snd2, 3},
	{ic_snd3, 4},
	{ic_snd4, 5},
	{ic_snd5, 6},
	{ic_snd6, 7},
	{ic_snd7, 8},
	{ic_snd8, 9},
	{NULL, -1}
};

extern void	handler();

int	timeout;
int	timer;
int	maxsdu;
int	minsdu;

void
tc_ssnd()
{
	dl_info_ack_t	infoack;
	char		*sval;
	int		ret;
	int		i;
	
	tet_infoline(TC_SEND);
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
	 * get the DLSDU min/max limit info..
	 */
	if (dl_info(tn_listen_fd, &infoack) < 0) {
		/*
		 * delete DL_BADDATA test ..
		 */
		tet_delete(3, LENINFO_FAIL);
		tet_delete(4, LENINFO_FAIL);
	}
	else {
		maxsdu = infoack.dl_max_sdu;
		minsdu = infoack.dl_min_sdu;
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
 * DL_UNITDATA_REQ should be successful for proper arguments
 * in proper DLP state..
 */ 
void
ic_snd0()
{
	tet_infoline(UNITDATAREQ_PASS);

	/*
	 * start the listener .. also
	 */
	tn_monitor(TN_LISTEN);
	if (tn_link(tn_net[TN_NEXT_TO_MON]) == PASS) {
		tet_result(TET_PASS);
	}
	else {
		tet_infoline(UNITDATAREQ_FAIL);
		tet_result(TET_FAIL);
	}
	/*
	 * kill the listener ...
	 */
	kill(tn_listener, 9);
	return;
}

/* 
 * DL_UNITDATA_REQ with bad DLSAP address format .. (DL_BADADDR) 
 */
void
ic_snd1()
{
	dl_uderror_ind_t	*uderrind;
	char			buf[DL_MAX_PKT];
	int			indlen, datalen;
	char			addr[1];

	tet_infoline(BADADDR_PASS);

	if (dl_sndudata(tn_listen_fd, buf, DL_TEST_LEN, addr, 0) < 0) {	
		tet_msg(PUTMSG_FAIL, errno);
		tet_result(TET_UNRESOLVED);
		return;
	}
	/*
	 * Expecting DL_UDERR_IND with DL_BADADDR..
	 */

	datalen = DL_TEST_LEN;
	if (dl_rcvudata(tn_listen_fd, buf, &datalen, uderrind, &indlen) < 0) {
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
	 * check for DL_UDERROR_IND..
	 */	

	if (uderrind->dl_primitive == DL_UDERROR_IND) {
		/*
		 * check for expected error value..
		 */
		if (uderrind->dl_errno == DL_BADADDR) {
			tet_result(TET_PASS);	
		}
		else { 
			tet_msg(BADADDR_FAIL, uderrind->dl_errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(BADADDR_FAIL0);
		tet_result(TET_FAIL);
	}	
	if (uderrind) 
		free((char *)uderrind);
	return;
}

/*
 * DL_UNITDATA_REQ with DLSDU exceeding DLSDU's limit .. (DL_BADDATA)
 * try packet length < min_sdu
 * expected error DL_BADDATA .. 
 */
void
ic_snd2()
{
	tet_infoline(BADDATA_PASS);

	/*
 	 * Try sending DATAGRAMS with minsdu - 1..
	 */
	if (minsdu == 0) 
		sndud(minsdu);
	else
		sndud(minsdu - 1);
	return;
}

/*
 * try packet length > max_sdu
 * expected error DL_BADDATA .. 
 */
void
ic_snd3()
{
	tet_infoline(BADDATA_PASS);

	/*
 	 * Try sending DATAGRAM with maxsdu + 1 length ..
	 */
	sndud(maxsdu + 1);
	return;
}

/*
 * send Datgram with the given length ..
 * finds out the DL_UDERR_IND ..
 */
sndud(length)
int	length;
{
	struct	strbuf		ctlbuf;
	struct	strbuf		databuf;
	dl_uderror_ind_t	uderrind;
	char			*dest, *src;
	int			i, flag, retval;
	int			len;
	char			buf[DL_MAX_PKT];

	/*
	 * attach alarm handelr
	 */
	signal(SIGALRM, (void (*)())handler);
	/*
	 * try sending datagram with len = length 
	 */
	tet_msg(SNDUD_MSG, length);
	if (dl_sndudata(tn_listen_fd, buf, length, 
		(char *)&tn_net[1].dn_llc_addr, sizeof(struct llcc)) < 0) {
		switch (dl_errno) {
			case DL_EALLOC:	
				tet_infoline(ALLOC_FAIL);
				tet_result(TET_UNRESOLVED);
				break;
			case DL_EPUTMSG:
				tet_msg(PUTMSG_FAIL, errno);
				tet_result(TET_UNRESOLVED);
				break;
			default:
				tet_infoline(SEND_FAIL);
				tet_result(TET_UNRESOLVED);
				break;
		}
		return;
	} 
	/*
	 * Expecting DL_UDERR_IND with DL_BADADDR..
	 */
	ctlbuf.maxlen = sizeof(dl_uderror_ind_t);
	ctlbuf.len = 0;
	ctlbuf.buf = (char *)&uderrind;

	/* databuf.maxlen = DL_TEST_LEN; */
	/* changed in Version 2.1 */
	databuf.maxlen = sizeof(buf);
	databuf.len = 0; 
	databuf.buf = buf; 

	/*
	 * start a alarm..
	 */	
	timer = 0;
	alarm(timeout);
	flag = 0; 
	if ((retval = getmsg(tn_listen_fd, &ctlbuf, &databuf, &flag)) < 0) {
		if ((errno == EINTR) && (timer)) {
			tet_infoline(GETMSG_TIMEOUT);
			tet_infoline(SNDUD_TIMEOUT);
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
	 * cancel the alarm..
	 */ 
	alarm(0);
	/*
	 * check for DL_UDERROR_IND..
	 */	
	if (uderrind.dl_primitive == DL_UDERROR_IND) {
		/*
		 * check for expected error value..
		 */
		if (uderrind.dl_errno == DL_BADDATA) {
			tet_result(TET_PASS);	
		}
		else { 
			tet_msg(BADDATA_FAIL, uderrind.dl_errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(BADDATA_FAIL0);
		tet_result(TET_FAIL);
	}	
	/* added in Version  2.1 */
	/*
	 *	make sure we read the entire message. -- duck
	 */
	while (retval & (MOREDATA|MORECTL)) {
		/* printf( "in the loop, retval=%d\n", retval); */
		retval = getmsg(tn_listen_fd, &ctlbuf, &databuf, &flag);
	}
	
	return;
}



/*
 * DL_UNITDATA_REQ with invalid argument .. (EINVAL and not PANIC!!!!!!)
 */
void
ic_snd4()
{
	struct	strbuf		ctlbuf;
	struct	strbuf		databuf;
	dl_unitdata_req_t	*dl_reqp;
	dl_uderror_ind_t	uderrind;
	int			i, flag, retval;
	long			badaddr;
	char			buf[DL_MAX_PKT];

	tet_infoline(BADADDR_PASS);

	/*
	 * attach alarm handelr
	 */
	signal(SIGALRM, (void (*)())handler);
	dl_reqp = (dl_unitdata_req_t *)malloc(sizeof(dl_unitdata_req_t) + 
							sizeof(struct llcc));	
	if (dl_reqp == NULL) {
		tet_infoline(ALLOC_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}
	
	dl_reqp->dl_primitive = DL_UNITDATA_REQ;
	dl_reqp->dl_dest_addr_length = sizeof(struct llcc);
	/*
 	 * get a bad addr..
	 */
	if (getpanicaddr(&badaddr) < 0) {
		tet_infoline(GETPANICADDR_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}
	dl_reqp->dl_dest_addr_offset = badaddr;

	ctlbuf.len = sizeof(dl_unitdata_req_t) + sizeof(struct llcc);
	ctlbuf.buf = (char *)dl_reqp;

	databuf.len = DL_TEST_LEN; 
	databuf.buf = (char *)buf;
	if (putmsg(tn_listen_fd, &ctlbuf, &databuf, 0) < 0) {
		if (dl_reqp)
			free((char *)dl_reqp);
		tet_msg(PUTMSG_FAIL, errno);
		tet_result(TET_UNRESOLVED);
		return;
	}
	/*
	 * Expecting DL_UDERR_IND with DL_BADADDR..
	 */
	ctlbuf.maxlen = sizeof(dl_uderror_ind_t);
	ctlbuf.len = 0;
	ctlbuf.buf = (char *)&uderrind;

	/* databuf.maxlen = DL_TEST_LEN; */
	/* changed in Version 2.1 */
	databuf.maxlen = sizeof(buf);
	databuf.len = 0; 
	databuf.buf = buf; 
	/*
	 * start a alarm..
	 */	
	timer = 0;
	alarm(timeout);
	flag = 0; 
	if ((retval = getmsg(tn_listen_fd, &ctlbuf, &databuf, &flag)) < 0) {
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
	 * cancel the alarm..
	 */ 
	alarm(0);
	/*
	 * check for DL_UDERROR_IND..
	 */	
	if (uderrind.dl_primitive == DL_UDERROR_IND) {
		/*
		 * check for expected error value..
		 */
		if (uderrind.dl_errno == DL_BADADDR) {
			tet_result(TET_PASS);	
		}
		else { 
			tet_msg(BADADDR_FAIL, uderrind.dl_errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(BADADDR_FAIL0);
		tet_result(TET_FAIL);
	}	
	while (retval & (MOREDATA|MORECTL)) {
		/* printf( "in the loop, retval=%d\n", retval); */
		retval = getmsg(tn_listen_fd, &ctlbuf, &databuf, &flag);
	}
	if (dl_reqp) 
		free((char *)dl_reqp);
	return;
}

/*
 * Try sending DL_UNITDATA_REQ with no data block associated ..
 * the driver should not PANIC!! (currently it is so for WD driver.. )
 */
void 
ic_snd5()
{
	struct	strbuf		ctlbuf;
	struct	strbuf		databuf;
	dl_unitdata_req_t	*dl_reqp;
	dl_uderror_ind_t	uderrind;
	char			*dest, *src;
	int			i, flag, retval;
	int			len;
	char			buf[DL_MAX_PKT];

	tet_infoline(UNITDATA_BCONT_NULL_PASS);

	dl_reqp = (dl_unitdata_req_t *)malloc(sizeof(dl_unitdata_req_t)
				       	    + sizeof(dl_addr_t)); 	
	if (dl_reqp == NULL) {
		tet_infoline(ALLOC_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}
	
	/*	
	 * make a request with invalid DLSDU limit..
	 */
	dl_reqp->dl_primitive = DL_UNITDATA_REQ;
	dl_reqp->dl_dest_addr_length = sizeof(struct llcc); 
	dl_reqp->dl_dest_addr_offset = sizeof(dl_unitdata_req_t);
	/*
	 * Send it to the next neighbour ..
	 */
	dest = (char *)dl_reqp + dl_reqp->dl_dest_addr_offset;
	src = (char *)&tn_net[TN_NEXT_TO_MON].dn_llc_addr;
	for (i = 0; i < sizeof(struct llcc); i++)  
		*dest++ = *src++;
	ctlbuf.len = sizeof(dl_unitdata_req_t) + sizeof(struct llcc);
	ctlbuf.buf = (char *)dl_reqp;

	/*
	 * set databuf.len = -1, so that the stream head does not 
 	 * pass the datapart to the driver.. i.e driver with get
	 * a DL_UNITDATA_REQ message with b_cont = NULL
	 * ..watch for PANIC!!!
	 */
	databuf.len =  -1;
	databuf.buf = (char *)buf;
	if (putmsg(tn_listen_fd, &ctlbuf, &databuf, 0) < 0) {
		if (dl_reqp)
			free((char *)dl_reqp);
		tet_msg(PUTMSG_FAIL, errno);
		tet_result(TET_UNRESOLVED);
		return;
	}
	else {
		tet_result(TET_PASS);
	}	
	if (dl_reqp) 
		free((char *)dl_reqp);
	return;

}

/*
 * Try DL_UNITDATA_REQ with invalid size message .. should not PANIC.
 */
void 
ic_snd6()
{
	struct	strbuf		ctlbuf;
	struct	strbuf		databuf;
	dl_unitdata_req_t	*dl_reqp;
	dl_uderror_ind_t	uderrind;
	char			*dest, *src;
	int			i, flag, retval;
	int			len;
	char			buf[DL_MAX_PKT];

	tet_infoline(UNITDATA_WRONG_SZ_PASS);

	dl_reqp = (dl_unitdata_req_t *)malloc(sizeof(dl_unitdata_req_t)
				       	    + sizeof(struct llcc)); 	
	if (dl_reqp == NULL) {
		tet_infoline(ALLOC_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}
	
	/*	
	 * make a request with invalid DLSDU limit..
	 */
	dl_reqp->dl_primitive = DL_UNITDATA_REQ;
	dl_reqp->dl_dest_addr_length = sizeof(struct llcc); 
	dl_reqp->dl_dest_addr_offset = sizeof(dl_unitdata_req_t);
	/*
	 * Send it to the next neighbour ..
	 */
	dest = (char *)dl_reqp + dl_reqp->dl_dest_addr_offset;
	src = (char *)&tn_net[TN_NEXT_TO_MON].dn_llc_addr;
	for (i = 0; i < sizeof(struct llcc); i++)  
		*dest++ = *src++;
	/*
	 * set ctlbuf.len = sizeof(long), so that the stream head does not 
 	 * pass the entire control part to the driver.. but good enough to 
	 * take the control to DLunitdata_req routine .. 
	 * driver should not misbehave ...
	 */
	ctlbuf.len = sizeof(long);
	ctlbuf.buf = (char *)dl_reqp;

	databuf.len = DL_TEST_LEN;
	databuf.buf = (char *)buf;
	if (putmsg(tn_listen_fd, &ctlbuf, &databuf, 0) < 0) {
		if (dl_reqp)
			free((char *)dl_reqp);
		tet_msg(PUTMSG_FAIL, errno);
		tet_result(TET_UNRESOLVED);
		return;
	}
	else {
		tet_result(TET_PASS);
	}	

	if (dl_reqp) 
		free((char *)dl_reqp);
	return;
}

/* 
 * DL_UNITDATA_REQ to own (local address)should be successful for 
 * proper arguments in proper DLP state..
 */ 
void
ic_snd7()
{
	tet_infoline(UNITDATAREQ_LOOP_PASS);

	/*
	 * start the listener .. also
	 */
	tn_monitor(TN_LISTEN);
	if (tn_loop(tn_net[0]) == PASS) {
		tet_result(TET_PASS);
	}
	else {
		tet_infoline(UNITDATAREQ_LOOP_FAIL);
		tet_result(TET_FAIL);
	}
	/*
	 * kill the listener ...
	 */
	kill(tn_listener, 9);
	return;
}

/* 
 * DL_UNITDATA_REQ  issued from an invalid state .. (DL_OUTSTATE)
 * Keep it as the last IC .. it takes stream to a UNBOUND state
 * and does not restore 
 */
void
ic_snd8()
{
	struct	strbuf		ctlbuf;
	struct	strbuf		databuf;
	dl_unitdata_req_t	*dl_reqp;
	dl_uderror_ind_t	uderrind;
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

	dl_reqp = (dl_unitdata_req_t *)malloc(sizeof(dl_unitdata_req_t)
				       	    + sizeof(struct llcc)); 	
	if (dl_reqp == NULL) {
		tet_infoline(ALLOC_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}
	
	/*	
	 * make a request with invalid DLSDU limit..
	 */
	dl_reqp->dl_primitive = DL_UNITDATA_REQ;
	dl_reqp->dl_dest_addr_length = sizeof(struct llcc); 
	dl_reqp->dl_dest_addr_offset = sizeof(dl_unitdata_req_t);
	/*
	 * Send it to the next neighbour ..
	 */
	dest = (char *)dl_reqp + dl_reqp->dl_dest_addr_offset;
	src = (char *)&tn_net[TN_NEXT_TO_MON].dn_llc_addr;
	for (i = 0; i < sizeof(struct llcc); i++)  
		*dest++ = *src++;
	ctlbuf.len = sizeof(dl_unitdata_req_t) + sizeof(struct llcc);
	ctlbuf.buf = (char *)dl_reqp;

	databuf.len = DL_TEST_LEN;
	databuf.buf = (char *)buf;
	if (putmsg(tn_listen_fd, &ctlbuf, &databuf, 0) < 0) {
		if (dl_reqp)
			free((char *)dl_reqp);
		tet_msg(PUTMSG_FAIL, errno);
		tet_result(TET_UNRESOLVED);
		return;
	}
	
	/*
	 * Expecting DL_UNDERR_IND with DL_OUTSTATE..
	 */
	ctlbuf.maxlen = sizeof(union DL_primitives);
	ctlbuf.len = 0;
	ctlbuf.buf = (char *)&uderrind;

	databuf.maxlen = DL_TEST_LEN;
	databuf.len = 0; 
	databuf.buf = buf; 

	/*
	 * start a alarm..
	 */	
	timer = 0;
	alarm(timeout);
	flag = 0; 
	if ((retval = getmsg(tn_listen_fd, &ctlbuf, &databuf, &flag)) < 0) {
		if (dl_reqp)
			free((char *)dl_reqp);
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
	 * check for DL_UDERROR_IND..
	 */	

	if (uderrind.dl_primitive == DL_UDERROR_IND) {
		/*
		 * check for expected error value..
		 */
		if (uderrind.dl_errno == DL_OUTSTATE) {
			tet_result(TET_PASS);	
		}
		else { 
			tet_msg(OUTSTATE_FAIL,uderrind.dl_errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(OUTSTATE_FAIL0);
		tet_result(TET_FAIL);
	}	
	if (dl_reqp) 
		free((char *)dl_reqp);
	return;
}

void
tc_esnd()
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
