/*
 * listener : The secondary node's thread in the test net 
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

extern int	errno;
extern int	dl_errno;

extern char	*tn_init_err[];

void tc_sioc();
void tc_eioc();

/*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_sioc; /* Test case start */
void (*tet_cleanup)() = tc_eioc; /* Test case end */

struct tet_testlist tet_testlist[] = {
	{NULL, -1}
};

void
tc_sioc()
{
	int	i, ret;
	int	found;
	char	maddr[DL_MAC_ADDR_LEN];

	if (getuid() == 0) {
		tet_infoline(SUITE_USER);
		for (i = 0; i < 9; i++)
			tet_delete(i+1, SUITE_USER);
		return;
	}

	/*
	 * Initialize the net configuration..
	 */
	if ((ret = tn_init()) < 0) {

		tet_infoline(NET_INIT_FAIL);
		ret -= ret;	 /* convert to positive val */
		if (ret > 0 && ret < NET_INIT_ERRS)	{
			tet_infoline(tn_init_err[-ret]);
		}

		for (i = 0; i < IC_MAX; i++)
			tet_delete(i+1, NET_INIT_FAIL); 

		return;
	}
	printf("Node [%x]\n", tn_node);	
	if (tn_node != 0) 
		printf("Secondary Test Node\n");
	else {
		printf("Error: Run listener on Secondary Nodes\n");
		return;
	}
	tn_listen();
	return;
}

void
tc_eioc()
{
}
