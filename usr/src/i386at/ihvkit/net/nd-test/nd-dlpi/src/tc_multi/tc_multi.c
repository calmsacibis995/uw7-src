/*
 * tc_multi.c : Test case multicase addressing ..
 */
#include <stdio.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/dlpi.h>
#include <sys/errno.h>
#include <sys/dlpi_ether.h>
#include <dl.h>
#include <tc_res.h>
#include <tc_msg.h>
#include <tc_net.h>
#include <tet_api.h>
#include <tc_net.h>
#include <config.h>

#include "tc_multi.h"

extern int	errno;
extern int	dl_errno;

extern char	*tn_init_err[];

void tc_smulti();
void tc_emulti();

void ic_multi0();
void ic_multi1();
void ic_multi2();

 /*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_smulti; /* Test case start */
void (*tet_cleanup)() = tc_emulti; /* Test case end */

struct tet_testlist tet_testlist[] = {
	{ic_multi0, 1},
	{ic_multi1, 2},
	{ic_multi0, 3},
	{NULL, -1}
};

int	fd;

void
tc_smulti()
{
	int	i;
	int	ret;

	tet_infoline(TC_MULTI);

	if (getuid() == 0) {
		tet_infoline(SUITE_USER);
		tet_delete(0, SUITE_USER);
		tet_delete(1, SUITE_USER);
		tet_delete(2, SUITE_USER);
		return;
	}


	/*
	 * Initialize the net configuration..
	 */
	if ((ret = tn_init()) < 0) {
		tet_infoline(NET_INIT_FAIL);
		ret -= ret;	 /* convert to positive val */
		if (ret > 0 && ret < NET_INIT_ERRS)	
			tet_infoline(tn_init_err[-ret]);
		for (i = 0; i < IC_MAX; i++)
			tet_delete(i+1, NET_INIT_FAIL); 
		return;
	}
#if 0	/* Can be tested with just two machines */
	if (tn_max_nodes < 3) {
		tet_delete(0, NOT_ENOUGH_MACHINES);		
		tet_delete(1, NOT_ENOUGH_MACHINES);		
		tet_delete(2, NOT_ENOUGH_MACHINES);		
		return;
	}	
#endif

	DL_DEBUG1(2,"Node [%x]\n", tn_node);	
	if (tn_node == 0) {
		DL_DEBUG(2,"The Test Monitor\n");
		tn_monitor(TN_LISTEN);
		/*
		 * Initiate the tests..
		 */
		if (tn_neighbours() != PASS) {
			tet_infoline(CONNECTIVITY_FAIL);
			for (i = 0; i < IC_MAX; i++)
				tet_delete(i + 1, CONNECTIVITY_FAIL);
		} 
	}
	else {
		DL_DEBUG(2,"Secondary Test Node\n");
		tn_listen();
	}

	/* 
	 * open a DLPI stream..
	 */
	if ((fd = open(netdev, O_RDWR)) < 0) {
		tet_infoline(OPEN_FAIL);
		for (i = 0; i < IC_MAX; i++)
			tet_delete(i+1, SUITE_USER);
		return;	
	}	
	return;
}


void tc_emulti()
{
}

/* IC #0: 
 * add/delete multicast
 */
void ic_multi0()
{
}

/* IC #1:
 * Attempt to enable too many multicast addresses. (DL_TOOMANY).
 */
void ic_multi1()
{
}

/* IC #2:
 * Primitive issued from an invalid state..
 */
void ic_multi2()
{
}


