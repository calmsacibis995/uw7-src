/*
 * tc_info.c: Test case DL_INFO_REQ/DL_INFO_ACK transaction..
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

#include "tc_info_msg.h"

extern int	errno;
extern int	dl_errno;

void tc_sinfo();
void tc_einfo();

void ic_info0();

/*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_sinfo; /* Test case start */
void (*tet_cleanup)() = tc_einfo; /* Test case end */

struct tet_testlist tet_testlist[] = {
	{ic_info0, 1},
	{NULL, -1}
};

void
tc_sinfo()
{
	int	i, uid;

	tet_infoline(TC_INFO);
	if (getdevname() < 0) {
		tet_infoline(GEN_CONFIG_FAIL);
		for (i = 0; i < IC_MAX; i++)
			tet_delete(i, GEN_CONFIG_FAIL);
	}
}

/* IC #0:
 * Request of DL_INFO_REQ should result in DL_INFO_ACK
 * primitive with DLP informations..
 */
void
ic_info0()
{
	int		fd;
	dl_info_ack_t 	infoack;

	tet_infoline(INFO_PASS);

	if ((fd = open(netdev, O_RDWR)) < 0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
	}

	if (dl_info(fd, &infoack) < 0) {
		tet_infoline(INFO_FAIL);
		tet_result(TET_FAIL);
	}
	else {
		tet_result(TET_PASS);
	}
	return;
}

void
tc_einfo()
{
}                                                                                                                                                               
