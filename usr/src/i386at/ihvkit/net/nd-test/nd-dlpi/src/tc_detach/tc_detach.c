/*
 * tc_detach.c: Test case DL_DETACH_REQ/DL_OK_ACK transaction..
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

#include "tc_detach_msg.h"

extern int	errno;
extern int	dl_errno;

void tc_sdetach();
void tc_edetach();

void ic_detach0();

/*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_sdetach; /* Test case start */
void (*tet_cleanup)() = tc_edetach; /* Test case end */

struct tet_testlist tet_testlist[] = {
	{ic_detach0, 1},
	{NULL, -1}
};

void
tc_sdetach()
{
	int		i;
	int		fd;
	dl_info_ack_t	infoack;	

	tet_infoline(TC_DETACH);
	if (getdevname() < 0) {
		tet_infoline(GEN_CONFIG_FAIL);
		for (i = 0; i < IC_MAX; i++)
			tet_delete(i, GEN_CONFIG_FAIL);
	}

	if ((fd = open(netdev, O_RDWR)) < 0) {
		tet_infoline(OPEN_FAIL);
		for (i = 0; i < IC_MAX; i++)
			tet_delete(i, GEN_CONFIG_FAIL);
		return;
	}

	/*
	 * Check DLPI provider style..
	 */
	if (dl_info(fd, &infoack) < 0) {
		tet_infoline(INFO_FAIL);
		return;
	}

	if (infoack.dl_provider_style == DL_STYLE1) {
		tet_delete(1, NOT_STYLE2);
	}
	return;
}

/* 
 * This IC is required only for STYLE 2 provider .. currently
 * i am aware of only style 1 provider in Unixware .. heard of
 * style 2 provider for solaris though ..
 */
void
ic_detach0()
{
	return;
}

void
tc_edetach()
{
}                                                                                                                                                               
