/*
 * tc_close.c : Test case close .. source. 
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

void ic_close0();
void ic_close1();
/*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_sclose; /* Test case start  */
void (*tet_cleanup)() = tc_eclose; /* Test case end  */

struct tet_testlist tet_testlist[] = {
	{ic_close0, 1},
	{ic_close1, 2},
	{NULL, -1}
};


void
tc_sclose()
{
	int	i;

	tet_infoline(TC_NCLOSE);
	if (getdevname() < 0) {
		tet_infoline(GEN_CONFIG_FAIL);
		for (i = 0; i < IC_MAX; i++)
			tet_delete(i, GEN_CONFIG_FAIL);
	}
}

/*
 * Close is successfull
 */
void
ic_close0()
{
	int	fd;

	tet_infoline(CLOSE_PASS);

	if ((fd = open(netdev, O_RDWR)) < 0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}

	if (close(fd) < 0) {
		tet_infoline(CLOSE_FAIL);
		tet_result(TET_FAIL);
	}	
	else {
		tet_result(TET_PASS);
	}
}


/*
 * Attempt of any further operation on a closed stream fails with an error.
 */
void
ic_close1()
{
	int	fd;
	int	flag;

	tet_infoline(AFTER_CLOSE_PASS);

	if ((fd = open(netdev, O_RDWR)) < 0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}

	if (close(fd) < 0) {
		tet_infoline(CLOSE_FAIL);
		tet_result(TET_UNRESOLVED);
	}	
	else {
		/* try doing a ioctl .. 
		 */
		if (dl_gpromisc(fd, &flag) < 0) {
			tet_result(TET_PASS);
		}
		else {
			tet_infoline(AFTER_CLOSE_FAIL);
			tet_result(TET_FAIL);
		}
	}
	return;
}

void
tc_eclose()
{
}
