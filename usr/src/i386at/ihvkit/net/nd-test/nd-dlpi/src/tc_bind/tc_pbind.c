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

/*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_sbind; /* Test case start bind */
void (*tet_cleanup)() = tc_ebind; /* Test case end bind */

struct tet_testlist tet_testlist[] = {
	{ic_bind0, 1},
	{ic_bind1, 2},
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

	tet_infoline(TC_PBIND);
	/*
	 * tc_bind should be run from a normal user... (otherwise error!!!)
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
 * Test DL_BIND_REQ without proper permission to use 
 * the requested DLSAP address.. PROMISCUOUS_SAP can be
 * used only by previledged user..
 */
void
ic_bind0()
{
	int			fd;
	unsigned short		dl_sap;
	int			uid;
	dl_bind_req_t		bindreq;
	dl_bind_ack_t		bindret;
	int			ret;

	tet_infoline(ACCESS_PASS);

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
		tet_infoline(ACCESS_FAIL0);
		tet_result(TET_FAIL);	
	}
	else {
		tet_result(TET_PASS);	
	}
	close(fd);	
	return;
}

/* 
 * Previledged user is able to bind A dlsap address to an already bound
 * stream. 
 */
void
ic_bind1()
{
	int			fd1, fd2;
	dl_bind_req_t		bindreq;
	dl_bind_ack_t		bindret;
	unsigned short		dl_sap;
	int			uid;

	tet_infoline(BOUND_PASS);

	dl_sap = DL_TEST_SAP; 
	dl_errno = 0;
	/*
	 * open a DLPI stream..
	 */
	if ((fd1 = dl_open(netdev, O_RDWR)) < 0) {
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
	if (dl_bind(fd1, &bindreq, &bindret) < 0) {
		tet_infoline(BIND_FAIL);
		tet_result(TET_UNRESOLVED);
		close(fd1);
		return; 
	}
	/*
	 * open a DLPI stream..
	 */
	if ((fd2 = dl_open(netdev, O_RDWR)) < 0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	} 

	if (dl_bind(fd2, &bindreq, &bindret) < 0) {
		tet_infoline(BOUND_FAIL0);
		tet_result(TET_FAIL);
	}
	else {
		tet_result(TET_PASS);
	}
	close(fd1);
	close(fd2);
	return;
}

void
tc_ebind()
{
}
