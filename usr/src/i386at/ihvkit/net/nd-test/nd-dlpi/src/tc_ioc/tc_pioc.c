/*
 * tc_pioc.c : Test Case IOCTL calls
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

#include "tc_ioc_msg.h"

extern int	errno;
extern int	dl_errno;

extern char *	tn_init_err[];

void tc_sioc();
void tc_eioc();

void ic_ccsmacdmode0();
void ic_ccsmacdmode1();
void ic_ccsmacdmode2();
void ic_ccsmacdmode3();
void ic_getmulti0();
void ic_getmulti1();
void ic_smib0();
void ic_smib1();
void ic_slpcflg0();
void ic_senaddr0();
void ic_senaddr1();
void ic_addmulti0();
void ic_addmulti1();
void ic_addmulti2();
void ic_delmulti0();
void ic_delmulti1();
void ic_delmulti2();
void ic_reset0();
void ic_enable0();
void ic_disable0();

/*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_sioc; /* Test case start */
void (*tet_cleanup)() = tc_eioc; /* Test case end */

struct tet_testlist tet_testlist[] = {
	{ic_smib0, 1},
	{ic_smib1, 2},
	{ic_ccsmacdmode0, 3},
	{ic_ccsmacdmode1, 4},
	{ic_ccsmacdmode2, 5},
	{ic_ccsmacdmode3, 6},
	{ic_addmulti0, 7},
	{ic_addmulti1, 8},
	{ic_addmulti2, 9},
	{ic_getmulti0, 10},
	{ic_getmulti1, 11},
	{ic_delmulti0, 12},
	{ic_delmulti1, 13},
	{ic_delmulti2, 14},
	{ic_senaddr0, 15},
	{ic_senaddr1, 16},
	{ic_slpcflg0, 17},
	{ic_disable0, 18},
	{ic_enable0, 19},
	{ic_reset0, 20},
	{NULL, -1}
};

int	fd;

void
tc_sioc()
{
	int	i;
	int	ret;

	tet_infoline(TC_PIOC);

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

	/*
	 * promiscuous test requires 3 machines !!
	 */
	if (tn_max_nodes < 3) {
		tet_delete(19, NOT_ENOUGH_MACHINES);		
	}	

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

/* 
 * DLIOCCSMACDMODE: Attempt to toggle SAP type for a stream not in
 * DL_IDLE fails with EINVAL 
 */
void
ic_ccsmacdmode0()
{
	tet_infoline(CCSMACDMODE_INVAL_PASS);

	if (dl_ccsmacdmode(fd) < 0) {
		if (errno == EINVAL) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(CCSMACDMODE_INVAL_FAIL0, errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(CCSMACDMODE_INVAL_FAIL);
		tet_result(TET_FAIL);
	}
	return;
}

/*
 * DLIOCCSMACDMODE: Fails with EINVAL for DL_ETHER sap ..
 */
void
ic_ccsmacdmode1()
{
	dl_bind_req_t	bindreq;
	dl_bind_ack_t	bindret;

	tet_infoline(CCSMACDMODE_INVAL1_PASS);

	/*
	 * bind a SAP > 0xff .. 
	 */
	bindreq.dl_primitive = DL_BIND_REQ;
	bindreq.dl_sap = DL_ETHER_SAP;
	bindreq.dl_max_conind = 0;
	bindreq.dl_service_mode = DL_CLDLS;
	bindreq.dl_conn_mgmt = 0;

	if (dl_bind(fd, &bindreq, &bindret) < 0) {
		tet_infoline(BIND_FAIL);
		tet_result(TET_UNRESOLVED);	
		return;
	}

	if (dl_ccsmacdmode(fd) < 0) {
		if (errno == EINVAL) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(CCSMACDMODE_INVAL1_FAIL0, errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(CCSMACDMODE_INVAL1_FAIL);
		tet_result(TET_FAIL);
	}

	/*
	 * Unbind the stream..
	 */
	dl_unbind(fd);
	return;
}

/*
 * DLIOCCSMACDMODE: Fails with EINVAL for SNAP sap ..
 */
void
ic_ccsmacdmode2()
{
	dl_bind_req_t	bindreq;
	dl_bind_ack_t	bindret;

	tet_infoline(CCSMACDMODE_INVAL2_PASS);

	/*
	 * bind a SAP = 0xaa .. 
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

	if (dl_ccsmacdmode(fd) < 0) {
		if (errno == EINVAL) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(CCSMACDMODE_INVAL2_FAIL0, errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(CCSMACDMODE_INVAL2_FAIL);
		tet_result(TET_FAIL);
	}

	/*
	 * Unbind the stream..
	 */
	dl_unbind(fd);
	return;
}

/*
 * DLIOCCSMACDMODE: Ioctl allows the previledge process to toggle
 * the SAP type      
 */
void 
ic_ccsmacdmode3()
{
	dl_bind_req_t	bindreq;
	dl_bind_ack_t	bindret;

	tet_infoline(CCSMACDMODE_PASS);

	/*
	 * bind CSMACD sap ( < 0xff )
	 */
	bindreq.dl_primitive = DL_BIND_REQ;
	bindreq.dl_sap = DL_CSMACD_SAP;
	bindreq.dl_max_conind = 0;
	bindreq.dl_service_mode = DL_CLDLS;
	bindreq.dl_conn_mgmt = 0;

	if (dl_bind(fd, &bindreq, &bindret) < 0) {
		tet_infoline(BIND_FAIL);
		tet_result(TET_UNRESOLVED);	
		return;
	}
	if (dl_ccsmacdmode(fd) < 0) {
		tet_infoline(CCSMACDMODE_FAIL);
		tet_result(TET_FAIL);
	}
	else {
		tet_result(TET_PASS);
	}

	/*
	 * Unbind the stream..
	 */
	dl_unbind(fd);
	return;
}

/*
 * DLIOCGETMULTI: Gets the list muticast address success fully
 */
void
ic_getmulti0()
{
	int	i;
	int	found;
	char	maddr[DL_MAC_ADDR_LEN];

	tet_infoline(GETMULTI_PASS);

	found = dl_getmulti(fd, maddr);
	if (found < 0) {
		tet_msg(IocFail(DLIOCGETMULTI), errno);
		tet_result(TET_FAIL);
		return;
	}
	/*
 	 * This IC is called only if ic_addmulti is successful ..
 	 * so it should get back the added multicast address ..
	 */
	if (found == 0) {
		tet_infoline(GETMULTI_FAIL);
		tet_result(TET_FAIL);
		return;		
	}
	/*
 	 * is it same as the added one..
	 */
	for (i = 0; i < DL_MAC_ADDR_LEN; i++) { 
		if (maddr[i] != tn_test_maddr[i])
			break;
	}
	if (i != DL_MAC_ADDR_LEN) {
		tet_infoline(GETMULTI_FAIL0);
		tet_result(TET_FAIL);
	}
	else {
		tet_result(TET_PASS);
	}
	return;
}

/*
 * The ioctl fails with EINVAL if an invalid user buffer is given
 */
void
ic_getmulti1()
{
	struct strioctl	cmd;
	char		maddr[DL_MAC_ADDR_LEN]; 
	int		ret;

	tet_infoline(EinvalPass(DLIOCGETMULTI));

	cmd.ic_cmd = DLIOCGETMULTI;
	cmd.ic_timout = 15;
	cmd.ic_len = 0;
	cmd.ic_dp = maddr;

	ret = ioctl(fd, I_STR, &cmd);

	if (ret < 0) {
		if (errno == EINVAL) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(EinvalFail(DLIOCGETMULTI), errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(EinvalFail0(DLIOCGETMULTI));
		tet_result(TET_FAIL);
	}
	return;	
}

/* 
 * DLIOCSMIB: Only previleged process can set values for MIB variable.
 */
void
ic_smib0()
{
	int		uid;
	DL_mib_t	mib;	

	tet_infoline(SMIB_PASS);

	/*
	 * Get the mib values .. and set it back..
	 */
	if (dl_gmib(fd, &mib) < 0) {
		tet_msg(IocFail(DLIOCGMIB), errno);
		tet_result(TET_UNRESOLVED);
		return;	
	}
	if (dl_smib(fd, &mib) < 0) {
		tet_infoline(SMIB_FAIL);
		tet_result(TET_FAIL);	
	}
	else {
		tet_result(TET_PASS);
	}
	return;
}

/*
 * The ioctl fails with EINVAL if an invalid user buffer is given..
 */
void
ic_smib1()
{
	struct strioctl	cmd;
	DL_mib_t	mib;
	int		ret;

	tet_infoline(EinvalPass(DLIOCSMIB));

	cmd.ic_cmd = DLIOCSMIB;
	cmd.ic_timout = 15;
	cmd.ic_len = 0;
	cmd.ic_dp = (char *)&mib;	

	ret = ioctl(fd, I_STR, &cmd);
	
	if (ret < 0) {
		if (errno == EINVAL) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(EinvalFail(DLIOCSMIB), errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(EinvalFail0(DLIOCSMIB));
		tet_result(TET_FAIL);
	}
	return;	
}

/*
 * DLIOCSLPCFLG: Only previleged process can set the lpc flag.
 */
void
ic_slpcflg0()
{
	dl_pkt_t	*dl_pkt;
	char		*dl_pkt_data;
	int		dl_retry;
	int		ret;
	int		i;

	tet_infoline(SLPCFLG_PASS);

	dl_pkt = (dl_pkt_t *)malloc(sizeof(dl_pkt_t) + strlen(dl_sig));
	if (dl_pkt == NULL) {
		tet_infoline("ERROR:malloc failed\n");
		tet_result(TET_UNRESOLVED);
		return;
	}
	/*
	 * Set the listener in Promiscuous mode 
	 */
	if (setlistener(DL_SET_PROMISC, tn_net[TN_NEXT_TO_MON]) != PASS) {
		tet_infoline(PROMISC_MODE_FAIL);
		tet_result(TET_UNRESOLVED);
		return;
	}
	/*
	 * Set itself to Local Packet Copy mode 
	 */
	if (dl_slpcflg(fd) < 0) {
		tet_msg(IocFail(DLIOCSLPCFLG), errno);
		tet_result(TET_FAIL);
		return;
	}
	/*
	 * Send Echo Request Packet to itself.. and wait for a
	 * echo reply from the listener ..
	 */

	dl_pkt->dp_primitive = DL_ECHO_REQ;
	dl_pkt->dp_test      = DL_SLPCFLG_TEST;
	dl_pkt->dp_sender    = tn_node;
	dl_pkt->dp_len	    = sizeof(dl_pkt_t) + strlen(dl_sig);
	dl_pkt_data = (char *)dl_pkt + sizeof(dl_pkt_t);

	for (i = 0; i < strlen(dl_sig); i++)
		dl_pkt_data[i] = dl_sig[i];
	
	*tn_eventp = DL_NOEVENT; 
	dl_retry = DL_RETRY_CNT;

	while ((*tn_eventp != DL_SLPCFLG) && dl_retry--) {
		/*
		 * Retry till the monitor node receives thr ECHO_REP packet
		 */
		if (dl_sndudata(tn_listen_fd, (char *)dl_pkt, dl_pkt->dp_len,
			(char *)&tn_net[0].dn_llc_addr, 
			sizeof(struct llcc)) < 0) {
			ret = FAIL;
			break;
		} 
		i=0;
		while (( i++ < 5 ) && (*tn_eventp != DL_SLPCFLG))
			sleep(DL_RETRY_TIMEOUT);
		/*
		 *  Wait for packet .. 
		 */
	}
	if (*tn_eventp == DL_SLPCFLG) {
		tet_result(TET_PASS);
	}
	else {
		/* retry did't help 
		 */
		tet_infoline(SLPCFLG_FAIL);
		tet_result(TET_FAIL);
	}

	/*
	 * Take the listener of the promiscuous mode
	 */
	if (setlistener(DL_CLEAR_PROMISC, tn_net[TN_NEXT_TO_MON]) != PASS) {
		tet_infoline(PROMISC_MODE_FAIL);
	}
	/*
	 * Try link test .. to find out whether board got disabled or 
	 * not ..
	 */
	if (tn_link(tn_net[TN_NEXT_TO_MON]) == PASS) {
		tet_infoline(LINK_OK);
	}
	else {
		tet_infoline(LINK_NOTOK_DISABLED);
	}
	return;
}

/* 
 * DLIOCSENADDR: Only Previleged process can set the Ethernet address.
 */
void
ic_senaddr0()
{
	int	i;
	char	eaddr[DL_MAC_ADDR_LEN];

	tet_infoline(SENADDR_PASS);

	if (dl_senaddr(tn_listen_fd, tn_test_eaddr) < 0) {
		tet_infoline(SENADDR_FAIL);
		tet_result(TET_FAIL);
	}
	else {
		if (dl_genaddr(tn_listen_fd, tn_test_eaddr) < 0) {
			tet_infoline(GENADDR_FAIL);
			tet_result(TET_UNRESOLVED);
		}
		else {
			/*
			 * compare with the set address .. 
			 */	
			for (i = 0; i < DL_MAC_ADDR_LEN; i++) 
				if (tn_test_eaddr[i] != eaddr[i]) 
					break;
			if (i == DL_MAC_ADDR_LEN) {
				tet_result(TET_PASS);
			}
			else { 
				tet_infoline(SENADDR_FAIL0);
				tet_result(TET_FAIL);
			}
		} 		
	}
	return;
}

/*
 * The ioctl return EINVAL if invalid user buffer is given.
 */
void
ic_senaddr1()
{
	struct strioctl	cmd;
	char		eaddr[DL_MAC_ADDR_LEN];
	int		ret;

	tet_infoline(EinvalPass(DLIOCSENADDR));

	cmd.ic_cmd = DLIOCSENADDR;
	cmd.ic_timout = 15;
	cmd.ic_len = 0;
	cmd.ic_dp = eaddr; 

	ret = ioctl(fd, I_STR, &cmd);

	if (ret < 0) {
		if (errno == EINVAL) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(EinvalFail(DLIOCSENADDR), errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(EinvalFail0(DLIOCSENADDR));
		tet_result(TET_FAIL);
	}	
	return;
}


/*
 * DLIOCADDMULTI:  Previlege user can add a multicast address successfully.
 */
void
ic_addmulti0()
{
	dl_pkt_t	*dl_req;
	char		*dl_req_data;
	int		i, ret;
	int		dl_retry;
	int		found;
	char		maddr[6];

	tet_infoline(ADDMULTI_PASS);

	dl_req = (dl_pkt_t *)malloc(sizeof(dl_pkt_t) + DL_MAC_ADDR_LEN +
				strlen(dl_sig));
	if (dl_req == NULL) {
		tet_infoline("ERROR:malloc failed\n");
		tet_result(TET_UNRESOLVED);
		return;
	}
	/*
	 * Add yourself to the multicast group..
 	 */
	if (dl_addmulti(tn_listen_fd,  tn_test_maddr) < 0) {
		DL_DEBUG(1,"ic_addmulti0: dl_addmulti failed\n");
		tet_msg(IocFail(DLIOCADDMULTI), errno);
		tet_result(TET_FAIL);
		/*
		 * delete the ic_getmulti and ic_delmulti ICs ..
		 */
		for (i = 11; i <= 15; i++)
			tet_delete(i, ADDMULTI_FAIL); 
		return;
	}

	found = dl_getmulti(fd, maddr);
	if (found <= 0) {
		DL_DEBUG(1,"Get multicast failed\n");
		return;
	}	
	else {
		DL_DEBUG(2,"Multicast Address added: [");
		for (i = 0; i < 6; i++)
			DL_DEBUG1(2," %x", maddr[i] & 0x0ff);
		DL_DEBUG(2,"]\n"); 
	}

	dl_req->dp_primitive = DL_ECHO_REQ;
	dl_req->dp_sender    = tn_node;
	dl_req->dp_test      = DL_MADDR_TEST;
	dl_req->dp_len	= sizeof(dl_pkt_t) + DL_MAC_ADDR_LEN + strlen(dl_sig);
	dl_req_data = (char *)dl_req + sizeof(dl_pkt_t);

	for (i = 0; i < DL_MAC_ADDR_LEN; i++)
		dl_req_data[i] = tn_test_maddr[i];

	for (i = 0; i < strlen(dl_sig); i++)
		dl_req_data[DL_MAC_ADDR_LEN + i] = dl_sig[i];

	*tn_eventp = DL_NOEVENT;
	dl_retry = DL_RETRY_CNT;

	while ((*tn_eventp != DL_MADDR) && dl_retry--) {
		/*
		 * Retry till the monitor node receives the ECHO_REPLY pkt
		 */
		if (dl_sndudata(tn_listen_fd, (char *)dl_req, dl_req->dp_len,
			(char *)&tn_net[TN_NEXT_TO_MON].dn_llc_addr,
			sizeof(struct llcc)) < 0) {
			tet_infoline(SEND_FAIL);
			tet_result(TET_UNRESOLVED);
			break;
		} 
		i=0;
		while (( i++ < 5 ) && (*tn_eventp != DL_MADDR))
			sleep(DL_RETRY_TIMEOUT);
		/*
		 *  Wait for packet .. 
		 */
	}
	DL_DEBUG1(2,"ic_addmulti: event [%x]\n", *tn_eventp);
	if (*tn_eventp == DL_MADDR) {
		tet_result(TET_PASS);
	}
	else {
		/*
 		 * Test if tn_link passes or not ..
		 */
		if (tn_link(tn_net[TN_NEXT_TO_MON]) != PASS) {
			tet_infoline(LINK_FAIL);
			tet_result(TET_UNRESOLVED);
		}
		else {	
			tet_infoline(ADDMULTI_FAIL);
			tet_result(TET_FAIL);
		}
	}
	/* delete the multicast */
	return;	
}

/*
 * The ioctl fails with EINVAL if an invalid user buffer is given.
 */
void
ic_addmulti1()
{
	struct strioctl	cmd;
	char		maddr[DL_MAC_ADDR_LEN];
	int		ret;

	tet_infoline(EinvalPass(DLIOCADDMULTI));

	cmd.ic_cmd = DLIOCADDMULTI;
	cmd.ic_timout = 15;
	cmd.ic_len = 0;
	cmd.ic_dp = maddr; 

	ret = ioctl(fd, I_STR, &cmd);

	if (ret < 0) {
		if (errno == EINVAL) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(EinvalFail(DLIOCADDMULTI), errno);
			tet_result(TET_FAIL);
		}
	}
	else {
			tet_infoline(EinvalFail0(DLIOCADDMULTI));
			tet_result(TET_FAIL);
	}	

	return;
}

/*
 * The ioctl fails with EINVAL if an invalid multicast address given.
 */
void 
ic_addmulti2()
{
	int		i;
	struct strioctl	cmd;
	char		maddr[DL_MAC_ADDR_LEN];
	int		ret;

	tet_infoline(INVALMULTI_PASS);

	/*
	 * try giving a wrong multicast address .. even lsb..
	 */
	for (i = 0; i < DL_MAC_ADDR_LEN; i++)
		maddr[i] = 0xa + i;

	cmd.ic_cmd = DLIOCADDMULTI;
	cmd.ic_timout = 15;
	cmd.ic_len = DL_MAC_ADDR_LEN;
	cmd.ic_dp = maddr; 

	ret = ioctl(fd, I_STR, &cmd);

	if (ret < 0) {
		if (errno == EINVAL) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(INVALMULTI_FAIL, errno);
			tet_result(TET_FAIL);
		}
	}
	else {
			tet_infoline(INVALMULTI_FAIL0);
			tet_result(TET_FAIL);
	}	

	return;
}

/* 
 * DLIOCDELMULTI: Previleged process can delete a multicast address. 
 */
void
ic_delmulti0()
{
	dl_pkt_t	*dl_req;
	char		*dl_req_data;
	int		i, ret;
	int		dl_retry;

	tet_infoline(DELMULTI_PASS);

	dl_req = (dl_pkt_t *)malloc(sizeof(dl_pkt_t) + DL_MAC_ADDR_LEN + 
					strlen(dl_sig));
	if (dl_req == NULL) {
		tet_infoline("ERROR:malloc failed\n");
		tet_result(TET_UNRESOLVED);
		return;
	}
	/*
	 * Add yourself to the multicast group..
 	 */
	if (dl_delmulti(tn_listen_fd,  tn_test_maddr) < 0) {
		tet_msg(IocFail(DLIOCDELMULTI), errno);
		tet_result(TET_FAIL);
		return;
	}
	
	dl_req->dp_primitive = DL_ECHO_REQ;
	dl_req->dp_sender    = tn_node;
	dl_req->dp_test      = DL_MADDR_TEST;
	dl_req->dp_len = sizeof(dl_pkt_t) + DL_MAC_ADDR_LEN + strlen(dl_sig);
	dl_req_data = (char *)dl_req + sizeof(dl_pkt_t);

	for (i = 0; i < DL_MAC_ADDR_LEN; i++)
		dl_req_data[i] = tn_test_maddr[i];

	for (i = 0; i < strlen(dl_sig); i++)
		dl_req_data[DL_MAC_ADDR_LEN + i] = dl_sig[i];

	*tn_eventp = DL_NOEVENT;
	dl_retry = DL_RETRY_CNT;

	while ((*tn_eventp != DL_MADDR) && dl_retry--) {
		/*
		 * Retry till the monitor node receives the ECHO_REPLY pkt
		 */
		if (dl_sndudata(tn_listen_fd, (char *)dl_req, dl_req->dp_len,
			(char *)&tn_net[TN_NEXT_TO_MON].dn_llc_addr,
			sizeof(struct llcc)) < 0) {
			tet_infoline(SEND_FAIL);	
			tet_result(TET_UNRESOLVED);
			break;
		} 
		i=0;
		while (( i++ < 5 ) && (*tn_eventp != DL_MADDR))
			sleep(DL_RETRY_TIMEOUT);
		/*
		 *  Wait for packet .. 
		 */
	}
	DL_DEBUG1(2,"tn_iocdelmulti: event [%x]\n", *tn_eventp);
	if (*tn_eventp == DL_MADDR) {
		tet_result(TET_PASS);
	}
	else {
		/*
 		 * Test if tn_link passes or not ..
		 */
		if (tn_link(tn_net[TN_NEXT_TO_MON]) != PASS) {
			tet_infoline(LINK_FAIL);
			tet_result(TET_UNRESOLVED);
		}
		else {	
			tet_infoline(DELMULTI_FAIL);
			tet_result(TET_FAIL);	
		}
	}

	return;	
}


/*
 * The ioctl fails with EINVAL if an invalid multicast address is given
 */
void
ic_delmulti1()
{
	struct strioctl	cmd;
	char		maddr[DL_MAC_ADDR_LEN];
	int		ret;

	tet_infoline(EinvalPass(DLIOCDELMULTI));

	cmd.ic_cmd = DLIOCDELMULTI;
	cmd.ic_timout = 15;
	cmd.ic_len = 0;
	cmd.ic_dp = maddr; 

	ret = ioctl(fd, I_STR, &cmd);

	if (ret < 0) {
		if (errno == EINVAL) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(EinvalFail(DLIOCDELMULTI), errno);
			tet_result(TET_FAIL);
		}
	}
	else {
			tet_infoline(EinvalFail0(DLIOCDELMULTI));
			tet_result(TET_FAIL);
	}	

	return;
}

/*
 * The ioctl fails with EINVAl if an invalid user buffer is given
 */
void
ic_delmulti2()
{
	int		i;
	struct strioctl	cmd;
	char		maddr[DL_MAC_ADDR_LEN];
	int		ret;
	int		uid;

	tet_infoline(INVALMULTI_PASS);

	/*
	 * try giving a wrong multicast address .. even lsb..
	 */
	for (i = 0; i < DL_MAC_ADDR_LEN; i++)
		maddr[i] = 0xa + i;

	cmd.ic_cmd = DLIOCDELMULTI;
	cmd.ic_timout = 15;
	cmd.ic_len = DL_MAC_ADDR_LEN;
	cmd.ic_dp = maddr; 

	ret = ioctl(fd, I_STR, &cmd);

	if (ret < 0) {
		if (errno == EINVAL) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(INVALMULTI_FAIL, errno);
			tet_result(TET_FAIL);
		}
	}
	else {
			tet_infoline(INVALMULTI_FAIL0);
			tet_result(TET_FAIL);
	}	
	return;
}


/* 
 * DLIOCRESET: Previledge process can reset the controller.
 */
void
ic_reset0()
{
	tet_infoline(RESET_PASS);

	if (dl_reset(fd) < 0) {
		tet_infoline(RESET_FAIL);
		tet_result(TET_FAIL);
	}	
	else {
		/*
		 * check it has really reset the board or not
		 */
		tet_result(TET_PASS);
	}
	return;
}

/* 
 * DLIOCENABLE: Previledge process can enable the controller.
 */
void
ic_enable0()
{
	tet_infoline(ENABLE_PASS);

	if (dl_enable(fd) < 0) {
		tet_infoline(ENABLE_FAIL);
		tet_result(TET_FAIL);
	}	
	else {
		/*
		 * check it has really enabled the board or not
		 */
		tet_result(TET_PASS);
	}
	return;
}

/* 
 * DLIOCDISABLE: Previleged process can disable the controller.
 */
void
ic_disable0()
{
	tet_infoline(DISABLE_PASS);

	if (dl_disable(fd) < 0) {
		tet_infoline(DISABLE_FAIL);
		tet_result(TET_FAIL);
	}	
	else {
		/*
		 * check it has really disabled the board or not
		 */
		tet_result(TET_PASS);
	}
	return;
}

void
tc_eioc()
{
	/*
	 * kill the listener
	 */
	if (tn_listener)
		kill(tn_listener, 9);

	/*
	 * close the stream
	 */
	close(fd);
}
