/*
 * tc_pclose.c : Test case close .. source. 
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

#include "tc_close_msg.h"

extern int	errno;
extern int	dl_errno;

void tc_sclose();
void tc_eclose();

void ic_close2();
/*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_sclose; /* Test case start  */
void (*tet_cleanup)() = tc_eclose; /* Test case end  */

struct tet_testlist tet_testlist[] = {
	{ic_close2, 1},
	{NULL, -1}
};


void
tc_sclose()
{
	int	i, uid;

	tet_infoline(TC_PCLOSE);
	if (getdevname() < 0) {
		tet_infoline(GEN_CONFIG_FAIL);
		for (i = 0; i < IC_MAX; i++)
			tet_delete(i, GEN_CONFIG_FAIL);
	}
}


/*
 * Close of a stream bound to promiscuous sap takes the board out of
 * promiscuous mode 
 */
void
ic_close2()
{
	int		fd;
	dl_bind_req_t	bindreq;
	dl_bind_ack_t	bindack;
	int		flag;
	int		uid;

	tet_infoline(CLOSE_PROMISC_PASS);

	/*
	 * attach a promiscuous SAP to a DLPI stream.. 
	 */
	if ((fd = open(netdev, O_RDWR)) < 0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		setuid(uid);
		return;
	}

	/*
	 * bind promiscuous SAP to DL end point. 
	 */
	bindreq.dl_primitive = DL_BIND_REQ;
	bindreq.dl_sap = PROMISCUOUS_SAP;
	bindreq.dl_max_conind = 0;
	bindreq.dl_service_mode = DL_CLDLS;
	bindreq.dl_conn_mgmt = 0;
	bindreq.dl_xidtest_flg = 0;
	if (dl_bind(fd, &bindreq, &bindack) < 0) {
		tet_infoline(BIND_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}
	if (dl_promiscon(fd) < 0) {
		tet_infoline(IOCTL_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}
	/*
	 * get the promiscuous mode flag ..
	 */
	dl_gpromisc(fd, &flag);

	/*
	 * promiscuous mode should be set ..
    	 */
	if (flag) {
		/*
		 * close that sream..
		 */
		if (close(fd) < 0) {
			tet_infoline(CLOSE_FAIL);
			tet_result(TET_UNRESOLVED);
			return;
		}	
		if ((fd = open(netdev, O_RDWR)) < 0) {
			tet_infoline(OPEN_FAIL);
			tet_result(TET_UNRESOLVED);
			return;
		}
		if (dl_gpromisc(fd, &flag) < 0) {
			tet_infoline(IOCTL_FAIL);
			tet_result(TET_UNRESOLVED);
			return;
		}
		if (flag == 0) {
			tet_result(TET_PASS);
		}
		else {
			tet_infoline(CLOSE_PROMISC_FAIL);
			tet_result(TET_FAIL);
		}
	}	
	else {
		tet_infoline(SET_PROMISC_FAIL);
		tet_result(TET_UNRESOLVED);
	}
	return;
}


void
tc_eclose()
{
}
