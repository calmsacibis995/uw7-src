/*
 * tc_unbind.c: Testcase DL_UNBIND_REQ/DL_OK_ACK transactions..
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
#include <tc_msg.h>
#include <config.h>

#include "tc_unbind_msg.h"

extern int	errno;
extern int	dl_errno;

void tc_sunbind();
void tc_eunbind();

void ic_unbind0();
void ic_unbind1();

/*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_sunbind; /* Test case start bind */
void (*tet_cleanup)() = tc_eunbind; /* Test case end bind */

struct tet_testlist tet_testlist[] = {
	{ic_unbind0, 1},
	{ic_unbind1, 2},
	{NULL, -1}
};


/*
 * Test the DL_UNBIND_REQ primitive..
 */
void
tc_sunbind()
{
	int	i, uid;

	tet_infoline(TC_UNBIND);	
	if (getdevname() < 0) {
		tet_infoline(GEN_CONFIG_FAIL);
		for (i = 0; i < IC_MAX; i++)
			tet_delete(i, GEN_CONFIG_FAIL);
	}
}

/* IC #0:
 * DL_UNBIND_REQ should be successful for proper arguments
 * in proper DLP state..
 */ 
void
ic_unbind0()
{
	int			fd;
	dl_bind_req_t 		bindreq;
	dl_bind_ack_t		bindret;
	unsigned short		dl_sap;

	tet_infoline(UNBIND_PASS);

	dl_sap = DL_TEST_SAP;		
	/*
	 * open a DLPI stream..
	 */
	if ((fd = dl_open(netdev, O_RDWR))  < 0) {
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

	if (dl_bind(fd, &bindreq, &bindret) < 0) {
		tet_infoline(BIND_FAIL);
		tet_infoline(UNRESOLVED);
		close(fd);
		return;
	}

	if (dl_unbind(fd) < 0) {
		tet_infoline(UNBIND_FAIL);
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

/* IC #1:
 * DL_UNBIND_REQ  issued from an invalid state .. (DL_OUTSTATE)
 */
void
ic_unbind1()
{
	int			fd;
	dl_bind_req_t 		bindreq;
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

	if (dl_unbind(fd) < 0) {
		if (dl_errno == DL_OUTSTATE) {
			tet_result(TET_PASS);
		}
		else {
			tet_infoline(OUTSTATE_FAIL0);
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

void
tc_eunbind()
{
}
