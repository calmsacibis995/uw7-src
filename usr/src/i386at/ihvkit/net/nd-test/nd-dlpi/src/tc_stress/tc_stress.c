/*
 * tc_ioc.c : Test Case IOCTL calls
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

#include "tc_stress_msg.h"

extern int	errno;
extern int	dl_errno;

extern char *	tn_init_err[];

void tc_sstress();
void tc_estress();

void ic_stress();
/*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_sstress; /* Test case start */
void (*tet_cleanup)() = tc_estress; /* Test case end */

struct tet_testlist tet_testlist[] = {
	{ic_stress, 1},
	{NULL, -1}
};

int	dl_pkt_cnt;
int	dl_pkt_len;
dl_pkt_t	*dl_pkt;

void
tc_sstress()
{
	int	i;
	char	*sptr;
	int	ret;

	tet_infoline(TC_STRESS);

	if (getuid() == 0) {
		tet_infoline(SUITE_USER);
		for (i = 0; i < IC_MAX; i++)
			tet_delete(i+1, SUITE_USER);
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

	if ((sptr = tet_getvar("PKTCOUNT")) == NULL) {
		tet_infoline(PKTCOUNT_FAIL);
		tet_delete(1, PKTCOUNT_FAIL);
		return;
	}
	dl_pkt_cnt = atoi(sptr);
	if ((sptr = tet_getvar("PKTLEN")) == NULL) {
		tet_infoline(PKTLEN_FAIL);
		tet_delete(1, PKTLEN_FAIL);
		return;
	}
	dl_pkt_len = atoi(sptr);

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

	dl_pkt = (dl_pkt_t *)malloc(sizeof(dl_pkt_t) + dl_pkt_len);
	if (dl_pkt == NULL) {
		DL_DEBUG(1,"malloc failed\n");
		tet_infoline("malloc failed\n");
			for (i = 0; i < IC_MAX; i++)
				tet_delete(i + 1, "malloc failed");
		} 
		
	return;
}

/*
 * sends configured number of ECHO_REQ packets .. does a link test 
 * and finds out the number of ECHO_REPLY packets received..
 */
void
ic_stress()
{
	int		i;
	int		ret;
	char 		*dl_pkt_data;
	
	DL_DEBUG(2,"tn_stress: Test\n");

	dl_pkt->dp_primitive = DL_ECHO_REQ;
	dl_pkt->dp_test      = DL_STRESS_TEST;
	dl_pkt->dp_sender    = TN_NEXT_TO_MON;
	dl_pkt->dp_len	    = sizeof(dl_pkt_t) + dl_pkt_len;
	dl_pkt_data = (char *)dl_pkt + sizeof(dl_pkt_t);

	for (i = 0; i < strlen(dl_sig); i++)
		dl_pkt_data[i] = dl_sig[i];
	
	*tn_pktcntp = 0; 

	for (i = dl_pkt_cnt; i--; ) {
		dl_sndudata(tn_listen_fd, (char *)dl_pkt, dl_pkt->dp_len,
			(char *)&tn_net[TN_NEXT_TO_MON].dn_llc_addr,
			sizeof(struct llcc)); 
		if (tn_max_nodes == 3) {
			dl_sndudata(tn_listen_fd, (char *)dl_pkt, 
				dl_pkt->dp_len,
				(char *)&tn_net[TN_NEXT_TO_MON + 1].dn_llc_addr,
				sizeof(struct llcc)); 
		}
			
	}	
	if (tn_link(tn_net[TN_NEXT_TO_MON]) < 0) {
		tet_infoline(STRESS_FAIL);
		tet_result(TET_FAIL);
		return;
	}	

	if (tn_max_nodes == 3) {
		if (tn_link(tn_net[TN_NEXT_TO_MON + 1]) < 0) {
			tet_infoline(STRESS_FAIL);
			tet_result(TET_FAIL);
			return;
		}	
	}

	if (tn_max_nodes == 3)
		dl_pkt_cnt *= 2;

	tet_msg(STRESS_STAT, dl_pkt_cnt, *tn_pktcntp);
	tet_result(TET_PASS);

	return;
}

void
tc_estress()
{
	/*
	 * kill the listener
	 */
	kill(tn_listener, 9);
	free(dl_pkt);
}
