/*
 * tc_open: Test case open .. source
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

#include "tc_open_msg.h"

extern int	errno;
extern int	dl_errno;

void tc_sopen();
void tc_eopen();

void ic_open0();
void ic_open1();
/*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_sopen; /* Test case start  */
void (*tet_cleanup)() = tc_eopen; /* Test case end  */

struct tet_testlist tet_testlist[] = {
	{ic_open0, 1},
	{ic_open1, 2},
	{NULL, -1}
};

#define	OPEN_LIMIT	128

void
tc_sopen()
{
	int	i;

	tet_infoline(TC_OPEN);
	if (getdevname() < 0) {
		tet_infoline(GEN_CONFIG_FAIL);
		for (i = 0; i < IC_MAX; i++)
			tet_delete(i, GEN_CONFIG_FAIL);
	}
}

/* IC #0: 
 * Open is successful.
 */
void
ic_open0()
{
	tet_infoline(OPEN_PASS);

	if (open(netdev, O_RDWR) < 0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_FAIL);
	}
	else {
		tet_result(TET_PASS);
	}
	return;
}

/* IC #1:
 * Succesive attempt of open fails with ECHRNG after a finite number of times.
 */
void
ic_open1()
{
	int	i;

	tet_infoline(ECHRNG_ASSERT);

	for (i = 0; i < OPEN_LIMIT; i++) {
		if (open(netdev, O_RDWR) < 0) {
			if (errno == ECHRNG) {
				tet_msg(ECHRNG_PASS, i);	
				tet_result(TET_PASS);
				break;
			}
			else {
				tet_msg(ECHRNG_FAIL0, errno, i);
				tet_result(TET_FAIL);
				break;
			}
		}
	}
	if (i == OPEN_LIMIT) {
		tet_msg(ECHRNG_FAIL1,i);
		tet_result(TET_FAIL);
	}
	return;
}

void
tc_eopen()
{
}
