/* 
 * tc_nsubsbind.c : The Test Case bind source
 */
#include <stdio.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/dlpi.h>
#include <sys/errno.h>
#include <sys/dlpi_ether.h>
#include <dl.h>
#include <tc_res.h>
#include <tet_api.h>
#include <config.h>
#include "tc_msg.h"

#include "tc_subsbind_msg.h"

extern int	errno;
extern int	dl_errno;

void tc_ssubsbind();
void tc_esubsbind();

void ic_subsbind0();
void ic_subsbind1();
void ic_subsbind2();
void ic_subsbind3();
void ic_subsbind4();

/*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_ssubsbind; /* Test case start bind */
void (*tet_cleanup)() = tc_esubsbind; /* Test case end bind */

struct tet_testlist tet_testlist[] = {
	{ic_subsbind0, 1},
	{ic_subsbind1, 2},
	{ic_subsbind2, 3},
	{ic_subsbind3, 4},
	{ic_subsbind4, 5},
	{NULL, -1}
};

/*
 * Test Case: DL_SUBS_BIND_REQ/ DL_SUBS_BIND_ACK transaction 
 */
void
tc_ssubsbind()
{
	int	i;
	int	uid;

	tet_infoline(TC_NSUBSBIND);
	/*
	 * tc_nbind should be run from a normal user... (otherwise error!!!)
	 */
	uid = getuid();
	if (uid == 0) {
		tet_infoline(SUITE_USER);
		for (i = 1; i < IC_MAX; i++)
			tet_delete(i, SUITE_USER);
	}
	if (getdevname() < 0) {
		tet_infoline(GEN_CONFIG_FAIL);
		for (i = 0; i < IC_MAX; i++)
			tet_delete(i, GEN_CONFIG_FAIL);
	}
}

/*  
 *  Test DL_SUBS_BIND_REQ with proper argument and  in 
 *  proper DLP state.
 */ 
void
ic_subsbind0() 
{
	int			fd;
	dl_bind_req_t		bindreq;
	dl_bind_ack_t		bindret;
	struct	snap_sap	snap_sap;

	tet_infoline(SUBSBIND_PASS);

	/*
	 * open a DLPI stream..
	 */
	if ((fd = dl_open(netdev, O_RDWR)) <  0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		return; 
	} 

	/*
	 * bind the SNAP sap ..
	 */
	bindreq.dl_primitive = DL_BIND_REQ;
	bindreq.dl_sap = DL_SNAP_SAP;
	bindreq.dl_max_conind = 0;
	bindreq.dl_service_mode = DL_CLDLS;
	bindreq.dl_conn_mgmt = 0;

	if (dl_bind(fd, &bindreq, &bindret) < 0) {
		tet_infoline(BIND_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}

	/*
	 * Try subsequent binding .... 
	 */
	snap_sap.snap_global = 0;
	snap_sap.snap_local = 0x8137;

	if (dl_subsbind(fd, snap_sap) < 0) {
		tet_infoline(SUBSBIND_FAIL);
		tet_result(TET_FAIL);	
	}
	else {
		tet_result(TET_PASS);
	}
	/*  close the DLPI stream..
 	 */
	close(fd); 
	return;
}

/* 
 * Test DL_SUBS_BIND_REQ to DL_ETHER SAP...
 */
void
ic_subsbind1()
{
	int			fd;
	dl_bind_req_t		bindreq;
	dl_bind_ack_t		bindret;
	struct	snap_sap	snap_sap;

	tet_infoline(BADADDR_PASS1);

	/*
	 * open a DLPI stream..
	 */
	if ((fd = dl_open(netdev, O_RDWR)) <  0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		return; 
	} 

	/*
	 * bind the ETHER sap..
	 */
	bindreq.dl_primitive = DL_BIND_REQ;
	bindreq.dl_sap = DL_ETHER_SAP;
	bindreq.dl_max_conind = 0;
	bindreq.dl_service_mode = DL_CLDLS;
	bindreq.dl_conn_mgmt = 0;

	if (dl_bind(fd, &bindreq, &bindret) < 0) {
		tet_infoline(BIND_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}

	/*
	 * Try subsequent binding .... 
	 */
	snap_sap.snap_global = 0;
	snap_sap.snap_local = 0x8137;

	if (dl_subsbind(fd, snap_sap) < 0) {
		if (dl_errno == DL_BADADDR) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(BADADDR_FAIL0,dl_errno);
			tet_result(TET_FAIL);	
		}
	}
	else {
		tet_infoline(BADADDR_FAIL1);
		tet_result(TET_FAIL);
	}
	/*  close the DLPI stream..
 	 */
	close(fd); 
	return;
}

/* 
 * Test DL_SUBS_BIND_REQ to DL_CSMACD sap ..
 */
void
ic_subsbind2()
{
	int			fd;
	dl_bind_req_t		bindreq;
	dl_bind_ack_t		bindret;
	struct	snap_sap	snap_sap;

	tet_infoline(BADADDR_PASS2);

	/*
	 * open a DLPI stream..
	 */
	if ((fd = dl_open(netdev, O_RDWR)) <  0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		return; 
	} 

	/*
	 * bind the CSMACD..
	 */
	bindreq.dl_primitive = DL_BIND_REQ;
	bindreq.dl_sap = DL_CSMACD_SAP;
	bindreq.dl_max_conind = 0;
	bindreq.dl_service_mode = DL_CLDLS;
	bindreq.dl_conn_mgmt = 0;

	if (dl_bind(fd, &bindreq, &bindret) < 0) {
		tet_infoline(BIND_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}

	/*
	 * Try subsequent binding .... 
	 */
	snap_sap.snap_global = 0;
	snap_sap.snap_local = 0x8137;

	if (dl_subsbind(fd, snap_sap) < 0) {
		if (dl_errno == DL_BADADDR) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(BADADDR_FAIL2, dl_errno);
			tet_result(TET_FAIL);	
		}
	}
	else {
		tet_infoline(BADADDR_FAIL3);
		tet_result(TET_FAIL);
	}
	/*  close the DLPI stream..
 	 */
	close(fd); 
	return;
}

/* 
 * Test DL_SUBS_BIND_REQ from an invalid state ..
 */
void
ic_subsbind3()
{
	int			fd;
	dl_bind_req_t		bindreq;
	dl_bind_ack_t		bindret;
	struct	snap_sap	snap_sap;

	tet_infoline(OUTSTATE_PASS);

	/*
	 * open a DLPI stream..
	 */
	if ((fd = dl_open(netdev, O_RDWR)) <  0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		return; 
	} 

	/*
	 * Try subsequent binding .... 
	 */
	snap_sap.snap_global = 0;
	snap_sap.snap_local = 0x8137;

	if (dl_subsbind(fd, snap_sap) < 0) {
		if (dl_errno == DL_OUTSTATE) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(OUTSTATE_FAIL0, dl_errno);
			tet_result(TET_FAIL);	
		}
	}
	else {
		tet_infoline(OUTSTATE_FAIL1);
		tet_result(TET_FAIL);
	}
	/*  close the DLPI stream..
 	 */
	close(fd); 
	return;
}

/* 
 * Test DL_BIND_REQ with an invalid address format..
 * with snap_sap length == 0...
 */
void
ic_subsbind4()
{
	char			reqbuf[256];	
	struct	strbuf		cbuf;
	dl_subs_bind_req_t	*sbindreqp;
	dl_subs_bind_ack_t	*sbindackp;
	int			flags;
	int			rval;
	int			fd;
	dl_bind_req_t		bindreq;
	dl_bind_ack_t		bindret;
	struct	snap_sap	snap_sap;

	tet_infoline(BADADDR_PASS3);

	/*
	 * open a DLPI stream..
	 */
	if ((fd = dl_open(netdev, O_RDWR)) <  0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		return; 
	} 

	/*
	 * bind the SNAP sap..
	 */
	bindreq.dl_primitive = DL_BIND_REQ;
	bindreq.dl_sap = DL_SNAP_SAP;
	bindreq.dl_max_conind = 0;
	bindreq.dl_service_mode = DL_CLDLS;
	bindreq.dl_conn_mgmt = 0;

	if (dl_bind(fd, &bindreq, &bindret) < 0) {
		tet_infoline(BIND_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}

	sbindreqp = (dl_subs_bind_req_t	*)reqbuf;
	/*
	 * Do the subsequent BIND
	 */
	sbindreqp->dl_primitive 	= DL_SUBS_BIND_REQ;
	sbindreqp->dl_subs_sap_offset	= sizeof (dl_subs_bind_req_t);
	/*
   	 * Give a wrong length say 0 instead of sizeof (struct snap_sap);
	 */
	sbindreqp->dl_subs_sap_length	= 0; 
	memcpy(&reqbuf[sizeof(dl_subs_bind_req_t)],
		&snap_sap, sizeof (struct snap_sap));

	cbuf.len    = 
	cbuf.maxlen = (sizeof(dl_subs_bind_req_t) + 
				sizeof (struct snap_sap));
	cbuf.buf    = (char *)&reqbuf;

	flags=0;

	if (putmsg(fd, &cbuf, NULL, flags)<0) {
		tet_msg(PUTMSG_FAIL,errno);
		tet_result(TET_UNRESOLVED);
		close(fd);
		return;
	}

	/*
	 * Make sure our bind request is ACKed.
	 */

	cbuf.len    = cbuf.maxlen	= sizeof(reqbuf);
	cbuf.buf    = reqbuf;

	flags = 0;

	rval = getmsg(fd, &cbuf, NULL, &flags);
	if (rval < 0) {
		tet_msg(GETMSG_FAIL,errno);
		tet_result(TET_UNRESOLVED);
		close(fd);
		return;
	}

	sbindackp = (dl_subs_bind_ack_t *)&reqbuf;

	switch (sbindackp->dl_primitive) {
		case DL_SUBS_BIND_ACK:
			tet_infoline(BADADDR_FAIL4);
			tet_result(TET_FAIL);
			break;
		case DL_ERROR_ACK:
			dl_errno = ((dl_error_ack_t *)&reqbuf)->dl_errno;
			if (dl_errno == DL_BADADDR) {
				tet_result(TET_PASS);
			}
			else {
				tet_msg(BADADDR_FAIL5, dl_errno);
				tet_result(TET_FAIL);
			}
			break;
		default:
			tet_result(TET_FAIL);
			break;
	}	 
	close(fd);
	return;
}


void
tc_esubsbind()
{
}
