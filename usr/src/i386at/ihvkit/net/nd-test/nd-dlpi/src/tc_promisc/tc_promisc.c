
/*
 * tc_promisc.c : Test case promisc addressing ..
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
#include <config.h>

#include "tc_promisc.h"

extern int	errno;
extern int	dl_errno;

extern char	*tn_init_err[];

void tc_spromisc();
void tc_epromisc();

void ic_promisc0();
void ic_promisc1();
void ic_promisc2();

 /*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_spromisc; /* Test case start */
void (*tet_cleanup)() = tc_epromisc; /* Test case end */

struct tet_testlist tet_testlist[] = {
	{ic_promisc0, 1},
	{ic_promisc1, 2},
	{ic_promisc2, 3},
	{NULL, -1}
};

dl_pkt_t	*dl_pkt;

void
tc_spromisc()
{
	int	i;
	int	ret;

	tet_infoline(TC_PROMISC);

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

	if (tn_node == 0) {
		/*
		 * Check if it can talk to the neighbours or not 
		 */
		tn_monitor(TN_LISTEN);
		if (tn_neighbours() != PASS) {
			tet_infoline(CONNECTIVITY_FAIL);
			for (i = 0; i < IC_MAX; i++)
				tet_delete(i + 1, CONNECTIVITY_FAIL);
			return;
		} 
		/*
		 * kill the listener ...
		 */
		kill(tn_listener, 9);
	}
	dl_pkt = (dl_pkt_t *)malloc(sizeof(dl_pkt_t) + strlen(dl_sig));
	if (dl_pkt == NULL) {
		tet_infoline("ERROR:malloc failed\n");
		for (i = 0; i < IC_MAX; i++)
			tet_delete(i + 1, CONNECTIVITY_FAIL);
	}
	return;
}


void tc_epromisc()
{
	free((char *)dl_pkt);
}

/* IC #0:
 * Previleged process can enable, get and diable the promiscuous mode.
 */
void
ic_promisc0()
{
	int		flag = 0;
	int		result = TET_PASS;
	int		fd;
	dl_bind_req_t	bindreq;
	dl_bind_ack_t	bindack;

	tet_infoline(PROMISCMODE_PASS);

	/* 
	 * open a DLPI stream..
	 */
	if ((fd = open(netdev, O_RDWR)) < 0) {
		tet_infoline(OPEN_FAIL);
		tet_result(TET_UNRESOLVED);
		return;	
	}	

	/* bind to a promiscous sap */
	bindreq.dl_primitive 	= DL_BIND_REQ;
	bindreq.dl_sap		= PROMISCUOUS_SAP;
	bindreq.dl_max_conind	= 0;
	bindreq.dl_service_mode	= DL_CLDLS;
	bindreq.dl_conn_mgmt	= 0;
	bindreq.dl_xidtest_flg	= 0;
	if (dl_bind(fd, &bindreq, &bindack) < 0) { 
		tet_infoline(BIND_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}

	/*
	 *  Set yourself in promiscuous mode 
	 */
	if (dl_promiscon(fd) < 0) { 
		tet_infoline(PROMISCON_FAIL);
		tet_result(TET_FAIL);
		return;
	}
	/*
	 *  get your promiscuous flag 
	 */
	if (dl_gpromisc(fd, &flag) < 0) { 
		tet_infoline(GPROMISC_FAIL);
		result = TET_FAIL;
	}
	/*
	 * check the returned flag
	 */
	else
		if (flag != 1) {
			tet_infoline(PROMISCMATCH_FAIL);
			result = TET_FAIL;
		}	

	/*
	 *  disable your promiscuous flag 
	 */
	if (dl_promiscoff(fd) < 0) { 
		tet_infoline(PROMISCOFF_FAIL);
		result = TET_FAIL;
	}

	tet_result(result);
	close(fd);
	return;
}

/* IC #1: 
 * test physical promiscuous mode
 */
void
ic_promisc1()
{
	char		*dl_pkt_data;
	int		dl_retry;
	int		ret;
	int		i;

	tet_infoline(PHYSPROMISC_PASS);

	/*
	 * start the physical level promiscuous thread..
	 */
	tn_monitor(TN_PHYS_PROMISCUOUS);

	/*
	 * Send a DL_RING_REQ to the neighbouring node and wait till it
	 * rings back to it.. 
	 */		
	
	dl_pkt->dp_primitive = DL_RING_REQ;
	dl_pkt->dp_sender    = tn_node;
	dl_pkt->dp_test      = DL_PHYSPROMISC_TEST;
	dl_pkt->dp_len	    = sizeof(dl_pkt_t) + strlen(dl_sig);
	dl_pkt_data = (char *)dl_pkt + sizeof(dl_pkt_t);

	for (i = 0; i < strlen(dl_sig); i++)
		dl_pkt_data[i] = dl_sig[i];
	
	*tn_eventp = DL_NOEVENT; 
	dl_retry = DL_RETRY_CNT;

	while ((*tn_eventp != DL_PROMISC) && dl_retry--) {
		/*
		 * Retry till the monitor node's promiscuous sap received
 		 * some packet with destination address different from it's
		 * own address.
		 */
		if (dl_sndudata(tn_listen_fd, (char *)dl_pkt, dl_pkt->dp_len,
			(char *)&tn_net[TN_NEXT_TO_MON].dn_llc_addr,
			sizeof(struct llcc)) < 0) {
			tet_infoline(SEND_FAIL);
			tet_result(TET_UNRESOLVED);
			break;
		} 
		/*
		 *  Wait for packet .. 
		 */
		sleep(DL_RETRY_TIMEOUT);
	}
	DL_DEBUG1(2,"tn_ring: event [%x]\n", *tn_eventp);
	if (*tn_eventp == DL_PROMISC) {
		tet_result(TET_PASS);
	}
	else {
		tet_infoline(PHYSPROMISC_FAIL);
		tet_result(TET_FAIL);
	}
	/*
	 * kill the promiscuous thread
	 */
	kill(tn_promisc, 9);
	return;
}

/* IC #2: 
 * test sap level promiscuous mode
 */
void
ic_promisc2()
{
	char		*dl_pkt_data;
	int		dl_retry;
	int		ret;
	int		i;

	tet_infoline(SAPPROMISC_PASS);

	/*
	 * start the promiscuous thread..
	 */
	tn_monitor(TN_SAP_PROMISCUOUS);

	/*
	 * Send a DL_RING_REQ to the neighbouring node and wait till it
	 * rings back to it.. 
	 */		
	
	dl_pkt->dp_primitive = DL_RING_REQ;
	dl_pkt->dp_sender    = tn_node;
	dl_pkt->dp_test      = DL_SAPPROMISC_TEST;
	dl_pkt->dp_len	    = sizeof(dl_pkt_t) + strlen(dl_sig);
	dl_pkt_data = (char *)dl_pkt + sizeof(dl_pkt_t);

	for (i = 0; i < strlen(dl_sig); i++)
		dl_pkt_data[i] = dl_sig[i];
	
	*tn_eventp = DL_NOEVENT; 
	dl_retry = DL_RETRY_CNT;

	while ((*tn_eventp != DL_PROMISC) && dl_retry--) {
		/*
		 * Retry till the monitor node's promiscuous sap received
 		 * some packet with destination address different from it's
		 * own address.
		 */
		if (dl_sndudata(tn_listen_fd, (char *)dl_pkt, dl_pkt->dp_len,
			(char *)&tn_net[TN_NEXT_TO_MON].dn_llc_addr,
			sizeof(struct llcc)) < 0) {
			tet_infoline(SEND_FAIL);
			tet_result(TET_UNRESOLVED);
			break;
		} 
		/*
		 *  Wait for packet .. 
		 */
		sleep(DL_RETRY_TIMEOUT);
	}
	DL_DEBUG1(2,"tn_ring: event [%x]\n", *tn_eventp);
	if (*tn_eventp == DL_PROMISC) {
		tet_result(TET_PASS);
	}
	else {
		tet_infoline(SAPPROMISC_FAIL);
		tet_result(TET_FAIL);
	}
	/*
	 * kill the promiscuous thread
	 */
	kill(tn_promisc, 9);
	return;
}
