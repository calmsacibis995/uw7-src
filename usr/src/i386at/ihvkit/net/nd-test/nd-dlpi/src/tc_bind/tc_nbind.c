/* 
 * tc_bind.c : The Test Case bind source
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

#include "tc_bind_msg.h"

extern int	errno;
extern int	dl_errno;

void tc_sbind();
void tc_ebind();

void ic_bind0();
void ic_bind1();
void ic_bind2();
void ic_bind3();
void ic_bind4();
#if 0
void ic_bind5();
void ic_bind6();
void ic_bind7();
void ic_bind8();
#endif /* 0 */
/*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_sbind; /* Test case start bind */
void (*tet_cleanup)() = tc_ebind; /* Test case end bind */

struct tet_testlist tet_testlist[] = {
	{ic_bind0, 1},
	{ic_bind1, 2},
	{ic_bind2, 3},
	{ic_bind3, 4},
	{ic_bind4, 5},
#if 0	/* these tests are not supported yet */
	{ic_bind5, 6},
	{ic_bind6, 7},
	{ic_bind7, 8},
#endif /* 0 */
	{NULL, -1}
};

/*
 * Test Case: DL_BIND_REQ/ DL_BIND_ACK transaction 
 * 
 */
void
tc_sbind()
{
	int	i;
	int	uid;

	tet_infoline(TC_NBIND);
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
 *  Test DL_BIND_REQ with proper argument and  in 
 *  proper DLP state.
 */ 
void
ic_bind0() 
{
	int			fd;
	dl_bind_req_t		bindreq;
	dl_bind_ack_t		bindret;
	unsigned short		dl_sap;

	tet_infoline(BIND_PASS);

	dl_sap = DL_TEST_SAP;		
	/*
	 * open a DLPI stream..
	 */
	if ((fd = dl_open(netdev, O_RDWR)) <  0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		return; 
	} 

	/*
	 * bind the SAP address to DL end point. 
	 */
	bindreq.dl_primitive = DL_BIND_REQ;
	bindreq.dl_sap = dl_sap;
	bindreq.dl_max_conind = 0;
	bindreq.dl_service_mode = DL_CLDLS;
	bindreq.dl_conn_mgmt = 0;
	bindreq.dl_xidtest_flg = 0;
	if (dl_bind(fd, &bindreq, &bindret) < 0) {
		tet_infoline(BIND_FAIL);
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
 * Test DL_BIND_REQ from an invalid state ..
 */
void
ic_bind1()
{
	int			fd;
	dl_bind_req_t		bindreq;
	dl_bind_ack_t		bindret;
	unsigned short		dl_sap;

	tet_infoline(OUTSTATE_PASS);

	dl_sap = DL_TEST_SAP;
	/*
	 * open a DLPI stream..
	 */
	if ((fd = dl_open(netdev, O_RDWR)) < 0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	} 

	/*
	 * bind the SAP address to DL end point. 
	 */
	bindreq.dl_primitive = DL_BIND_REQ;
	bindreq.dl_sap = dl_sap;
	bindreq.dl_max_conind = 0;
	bindreq.dl_service_mode = DL_CLDLS;
	bindreq.dl_conn_mgmt = 0;
	bindreq.dl_xidtest_flg = 0;
	if (dl_bind(fd, &bindreq, &bindret) < 0) {
		tet_infoline(BIND_FAIL);
		tet_result(TET_UNRESOLVED);
		close(fd);
		return;
	}
	/* Try bind again..
 	 */
	if (dl_bind(fd, &bindreq, &bindret) < 0) {
		if (dl_errno  == DL_OUTSTATE) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(OUTSTATE_FAIL0);
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
 * Test DL_BIND_REQ to bind DLSAP which is already bound to another
 * stream .. it should fail with DL_BOUND for a normal user..
 */
void
ic_bind2()
{
	int			fd,fd2;
	dl_bind_req_t		bindreq;
	dl_bind_ack_t		bindret;
	unsigned short		dl_sap;

	tet_infoline(NORMAL_BOUND_PASS);

	dl_sap = DL_TEST_SAP;		
	/*
	 * open a DLPI stream..
	 */
	if ((fd = dl_open(netdev, O_RDWR)) < 0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	} 

	/*
	 * bind the SAP address to DL end point. 
	 */
	bindreq.dl_primitive = DL_BIND_REQ;
	bindreq.dl_sap = dl_sap;
	bindreq.dl_max_conind = 0;
	bindreq.dl_service_mode = DL_CLDLS;
	bindreq.dl_conn_mgmt = 0;
	bindreq.dl_xidtest_flg = 0;
	if (dl_bind(fd, &bindreq, &bindret) < 0) {
		tet_infoline(BIND_FAIL);
		tet_result(TET_UNRESOLVED);
		close(fd);
		return;
	}
	/*
	 * Try binding same DLSAP to the another stream..
	 */
	if ((fd2 = dl_open(netdev, O_RDWR)) < 0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		close(fd);
		return;
	} 
	
	if (dl_bind(fd2, &bindreq, &bindret) < 0) {
		if (dl_errno == DL_BOUND) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(NORMAL_BOUND_FAIL0, dl_errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(NORMAL_BOUND_FAIL);
		tet_result(TET_FAIL);
	}
	/*  close the DLPI stream..
 	 */
	close(fd); 
	close(fd2);
	return;
}

/* 
 * Test DL_BIND_REQ with an invalid address format..
 */
void
ic_bind3()
{
	int			fd;
	dl_bind_req_t		bindreq;
	dl_bind_ack_t		bindret;
	unsigned short		dl_sap;
	int			i;

	tet_infoline(BADADDR_PASS);

	dl_sap = 0; 
	dl_errno = 0;
	/*
	 * open a DLPI stream..
	 */
	if ((fd = dl_open(netdev, O_RDWR)) < 0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	} 
	/*
	 * bind the SAP address to DL end point. 
	 */
	bindreq.dl_primitive = DL_BIND_REQ;
	bindreq.dl_sap = dl_sap;
	bindreq.dl_max_conind = 0;
	bindreq.dl_service_mode = DL_CLDLS;
	bindreq.dl_conn_mgmt = 0;
	bindreq.dl_xidtest_flg = 0;
	if (dl_bind(fd, &bindreq, &bindret) < 0) {
		if (dl_errno == DL_BADADDR) {
			tet_result(TET_PASS);
		}
		else {
			tet_infoline(BADADDR_FAIL0);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(BADADDR_FAIL1);
		tet_result(TET_FAIL);
	}
	close(fd);
	return;
}

/*
 * Try bind with invalid DL_BIND_REQ message size.. expected EINVAL
 */
void
ic_bind4()
{
	int			fd;
	int			i, flags;
	struct strbuf		ctlbuf;
	union DL_primitives	rcvbuf;
	dl_bind_req_t		bindreq;
	dl_bind_ack_t		bindret;
	dl_error_ack_t		errack;

	tet_infoline(WRONG_REQ_PASS);
	/*
	 * initialize rcvbuf with impossible value..
	 */ 
	rcvbuf.dl_primitive = -1;

	if ((fd = open(netdev, O_RDWR)) < 0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}
	
	/*
	 * pass only the primitive .. so that it goes to 
	 * DLbind_req.. driver should not misbehave..
	 */
	bindreq.dl_primitive = DL_BIND_REQ;

	ctlbuf.len = sizeof(long);
	ctlbuf.buf = (char *)&bindreq;

	if (putmsg(fd, &ctlbuf, NULL, 0) < 0) {
		tet_infoline(PUTMSG_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}	

	/*
	 * wait for ack of bind request..
	 */
	ctlbuf.maxlen = sizeof(union DL_primitives);
	ctlbuf.len = 0;
	ctlbuf.buf = (char *)&rcvbuf; 

	flags = RS_HIPRI;
	
	if (getmsg(fd, &ctlbuf, NULL, &flags) < 0) {
		tet_infoline(GETMSG_FAIL);
		tet_result(TET_UNRESOLVED);
		return;	
	}
	
	switch (rcvbuf.dl_primitive) {
		case DL_BIND_ACK:
			tet_infoline(WRONG_REQ_ACKD);
			tet_result(TET_FAIL);
			break;
		case DL_ERROR_ACK:
			dl_errno = rcvbuf.error_ack.dl_errno;
			tet_result(TET_PASS);
			break;
	}	 
	close(fd);
	return;
}

#if 0 /* not supported yet - right now any user can bind to any sap */
/*
 * Bind request to PROMISCUOUS_SAP from a Normal user..
 */
void
ic_bind5()
{
	int			fd;
	unsigned short		dl_sap;
	dl_bind_req_t		bindreq;
	dl_bind_ack_t		bindret;
	int			ret;

	tet_infoline(ACCESS_PASS0);

	dl_sap = PROMISCUOUS_SAP; 
	dl_errno = 0;
	/*
	 * open a DLPI stream..
	 */
	if ((fd = dl_open(netdev, O_RDWR)) < 0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	} 
	/*
	 * bind the SAP address to DL end point. 
	 */
	bindreq.dl_primitive = DL_BIND_REQ;
	bindreq.dl_sap = dl_sap;
	bindreq.dl_max_conind = 0;
	bindreq.dl_service_mode = DL_CLDLS;
	bindreq.dl_conn_mgmt = 0;
	bindreq.dl_xidtest_flg = 0;
	if (dl_bind(fd, &bindreq, &bindret) < 0) {
		tet_infoline(ACCESS_FAIL1);
		tet_result(TET_FAIL);	
	}
	else {
		if (dl_errno == DL_ACCESS) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(ACCESS_FAIL2);
			tet_result(TET_FAIL);	
		}
	}
	close(fd);	
	return;
		
}
#endif /* 0 */

#if 0 /* this test is not supported yet */
/* 
 * Test DL_BIND_REQ with improper service mode request...
 */
void
ic_bind6()
{
	int			fd;
	dl_bind_req_t		bindreq;
	dl_bind_ack_t		bindret;
	dl_info_ack_t		infoack;
	unsigned short		dl_sap;

	tet_infoline(UNSUPPORTED_PASS);

	dl_sap = DL_TEST_SAP;
	/*
	 * open a DLPI stream..
	 */
	if ((fd = dl_open(netdev, O_RDWR)) < 0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	} 

	if (dl_info(fd, &infoack) < 0) {
		tet_infoline(INFO_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	} 

	/*
	 * bind the SAP address to DL end point. 
	 */
	bindreq.dl_primitive = DL_BIND_REQ;
	bindreq.dl_sap = dl_sap;
	bindreq.dl_max_conind = 0;
	bindreq.dl_service_mode =  ~(infoack.dl_service_mode);
	bindreq.dl_conn_mgmt = 0;
	bindreq.dl_xidtest_flg = 0;
	if (dl_bind(fd, &bindreq, &bindret) == NULL) {
		if (dl_errno  == DL_UNSUPPORTED) { 
			tet_result(TET_PASS);
		}
		else {
			tet_msg(UNSUPPORTED_FAIL0, dl_errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(UNSUPPORTED_FAIL1);
		tet_result(TET_FAIL);
	}
	/*  
  	 * close the DLPI stream..
 	 */
	close(fd);
	return;
}
#endif /* 0 */
#if 0 /* this test is not supported yet */
/* 
 * Test DL_BIND_REQ without proper PPA initialization..
 */
void
ic_bind7()
{
	tet_infoline(PPA_INIT_FAIL);
	tet_infoline(CANT_TEST);
	tet_result(TET_UNTESTED);
	return;	
}
#endif /* 0 */
void
tc_ebind()
{
}
