/* 
 * tc_subsunbind.c : The Test Case DL_SUBS_UNBIND_REQ ..  
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

#include "tc_subsunbind_msg.h"

extern int	errno;
extern int	dl_errno;

void tc_ssubsunbind();
void tc_esubsunbind();

void ic_subsunbind0();
/*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_ssubsunbind; /* Test case start bind */
void (*tet_cleanup)() = tc_esubsunbind; /* Test case end bind */

struct tet_testlist tet_testlist[] = {
	{ic_subsunbind0, 1},
	{NULL, -1}
};

/*
 * Test Case: DL_SUBS_UNBIND_REQ/ DL_OK_ACK transaction 
 */
void
tc_ssubsunbind()
{
	int	i;
	int	uid;

	tet_infoline(TC_SUBSUNBIND);
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
ic_subsunbind0() 
{
	int			fd;
	dl_bind_req_t		bindreq;
	dl_bind_ack_t		bindret;
	struct	snap_sap	snap_sap;

	tet_infoline(SUBSUNBIND_PASS);

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
		tet_result(TET_UNRESOLVED);	
		close(fd);
		return;
	}
	
	/*
 	 * Try dl_subsunbind .. 
	 */
	if (dl_subsunbind(fd, snap_sap) < 0) {
		tet_msg(SUBSUNBIND_FAIL, dl_errno);
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

void
tc_esubsunbind()
{
}
