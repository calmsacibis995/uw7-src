/*
 * tc_frame.c: Dataxfer with different frame types..
 */
#include <stdio.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/dlpi.h>
#include <sys/errno.h>
#include <sys/dlpi_ether.h>
#include <signal.h>
#include <dl.h>
#include <tc_res.h>
#include <tet_api.h>
#include <tc_net.h>
#include <config.h>
#include <tc_msg.h>

#include <tc_frame_msg.h>

extern int	errno;
extern int	dl_errno;

extern char *tn_init_err[];
extern char *tc_frames[];

void tc_sframe();
void tc_eframe();

void ic_frame0();
void ic_frame1();
void ic_frame2();
void ic_frame3();
void ic_frame4();
void ic_frame5();
/*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_sframe; /* Test case start snd */
void (*tet_cleanup)() = tc_eframe; /* Test case end snd */

struct tet_testlist tet_testlist[] = {
	{ic_frame0, 1},
	{ic_frame1, 2},
	{ic_frame2, 3},
	{ic_frame3, 4},
	{ic_frame4, 5},
	{ic_frame5, 6},
	{NULL, -1}
};

int	timeout;
void
tc_sframe()
{
	int		fd;
	dl_info_ack_t	infoack;
	char		*sval;
	int		ret;
	int		i;
	
	tet_infoline(TC_FRAME);

	/*
	 * Initialize the net configuration ..
	 */	
	if ((ret = tn_init()) < 0) {
		tet_infoline(NET_INIT_FAIL);
		ret = -ret;	 /* convert to positive val */
		if (ret > 0 && ret < NET_INIT_ERRS)	
			tet_infoline(tn_init_err[-ret]);
		for (i = 0; i < IC_MAX; i++)
			tet_delete(i+1, NET_INIT_FAIL); 
		return;
	}

	/*
	 * get the timeout value..
	 */
	if ((sval = tet_getvar("GETMSG_TIMEOUT")) == NULL) {
		tet_infoline(GETMSG_TIMEOUT_FAIL);
		/*
		 * default is 15 sec
		 */
		timeout = 15;
	}
	else
		timeout = atoi(sval);

	dl_info(tn_listen_fd, &infoack);
	if (infoack.dl_mac_type == DL_TPR) 
		for (i = 0; i < 4; i++)
			tet_delete(i+1, NOT_APPLICABLE); 
	else
		for (i = 4; i < IC_MAX; i++)
			tet_delete(i+1, NOT_APPLICABLE); 
		
	if (tn_node == 0) {
		/*
		 * Check if it can talk to the neighbours or not 
		 */
		tn_monitor(TN_LISTEN);
		if (tn_neighbours() != PASS) {
			tet_infoline(CONNECTIVITY_FAIL);
			for (i = 0; i < IC_MAX; i++)
				tet_delete(i + 1, CONNECTIVITY_FAIL);
		} 
		/*
		 * kill the listener ...
		 */
		kill(tn_listener, 9);
	}
	return;
}

/*
 * Check if the driver sends ETHERNET_II frames successfully
 */
void
ic_frame0()
{
	tet_msg(FRAME_INFO, FRAME_PASS, tc_frames[ETHERNET_II]);
	tn_frame(ETHERNET_II);
}

/* 
 * Check if the driver sends ETHERNET_802.2 frames successfully
 */
void
ic_frame1()
{
	tet_msg(FRAME_INFO, FRAME_PASS, tc_frames[ETHERNET_8022]);
	tn_frame(ETHERNET_8022);
}

/* 
 * Check if it sends ETHERNET_SNAP packets successfully in right
 * format
 */ 
void
ic_frame2()
{
	tet_msg(FRAME_INFO, FRAME_PASS, tc_frames[ETHERNET_SNAP]);
	tn_frame(ETHERNET_SNAP);
}

/* 
 * Check if it sends ETHERNET_802.3 RAW  packets successfully
 */ 
void
ic_frame3()
{
	tet_msg(FRAME_INFO, FRAME_PASS, tc_frames[ETHERNET_8023]);
	tn_frame(ETHERNET_8023);
}
/* 
 * Check if it sends TOKEN_RING packets successfully
 */ 
void
ic_frame4()
{
	tet_msg(FRAME_INFO, FRAME_PASS, tc_frames[TOKEN_RING]);
	tn_frame(TOKEN_RING);
}
/* 
 * Check if it sends TOKEN_RING_SNAP packets successfully
 */ 
void
ic_frame5()
{
	tet_msg(FRAME_INFO, FRAME_PASS, tc_frames[TOKEN_RING_SNAP]);
	tn_frame(TOKEN_RING_SNAP);
}

void
tc_eframe()
{
}

tn_frame(int frame)
{
	int		dl_retry;
	int		dl_timeout;
	dl_pkt_t	*dl_req;
	char		*dl_req_data;

	int		i;

	dl_req = (dl_pkt_t *)malloc(sizeof(dl_pkt_t) + strlen(dl_sig));
	if (dl_req == NULL) {
		tet_infoline("ERROR: malloc failed\n");
		tet_result(TET_UNRESOLVED);
		return;
	}

	/*
	 * start the listener .. 
	 */
	tn_monitor(TN_LISTEN);

	*tn_eventp = DL_NOEVENT;
	dl_retry = DL_RETRY_CNT;

	while ((*tn_eventp == DL_NOEVENT) && dl_retry--) {
		dl_timeout = DL_RETRY_TIMEOUT;
		/*
	 	* Request the listener on remote m/c to check the outgoing
	 	* frame format..
	 	*/
		dl_req->dp_primitive = DL_CHK_FRAME;
		dl_req->dp_test = frame;
		dl_req->dp_sender = tn_node;
		dl_req->dp_len = sizeof(dl_pkt_t) + strlen(dl_sig);
		dl_req_data = (char *)dl_req + sizeof(dl_pkt_t);
	
		for (i = 0; i < strlen(dl_sig); i++)
			dl_req_data[i] = dl_sig[i];
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
		while ((*tn_eventp == DL_NOEVENT) && dl_timeout--) 
			sleep(1);
	}

	if (*tn_eventp != DL_CHK_FRAME_OK) {
		tet_infoline(TEST_FRAME_UNRESOLVED);
		tet_result(TET_UNRESOLVED);
		goto kill_listener;
	}

	/* send a frame */
	if (dl_sndframe(frame,tn_net[TN_NEXT_TO_MON].dn_llc_addr) < 0) {
		tet_result(TET_UNRESOLVED);
		goto kill_listener;
	}

	DL_DEBUG1(2,"ic_frame1: event [%x]\n", *tn_eventp);
	switch (*tn_eventp) {
		case DL_FRFAIL:
			tet_infoline(TEST_FRAME_UNRESOLVED);
			tet_result(TET_UNRESOLVED);
			break;
		case DL_FROK:
			tet_result(TET_PASS);
			break;
		case DL_FRNOTOK:
			tet_msg(FRAME_INFO,FRAME_FAIL,tc_frames[frame]);
			tet_result(TET_FAIL);
			break;
		default:
			tet_infoline(TEST_FRAME_UNRESOLVED);
			tet_result(TET_UNRESOLVED);
			break;
	}
kill_listener:
	/*
	 * kill the listener ...
	 */
	kill(tn_listener, 9);
	return;
}

dl_sndframe(frame, llc)
int		frame;
union llc_bind_fmt	llc;
{
	int		i;
	int		fd;
	int		len;
	unsigned short  dlsap;
	char		buf[100];
	dl_bind_req_t	bindreq;
	dl_bind_ack_t	bindack;
	struct	snap_sap snap_sap;
	int		addr_len;
	int		ret_val = 0;

	int		dl_retry;
	int		dl_timeout;

	if ((fd = open(netdev, O_RDWR)) < 0) {
		tet_infoline(OPEN_FAIL);
		return(-1);
	}
	len = DL_FRTEST_LEN;
	for (i = 0; i < len; i++)
		buf[i] = 'a';

	/*
	 * Decide the dlsap to bind..
	 */
	switch (frame) {
		case ETHERNET_II:
		case TOKEN_RING_SNAP:
			dlsap = 0x8137;
			llc.llca.lbf_sap = 0x8137;
			addr_len = sizeof(struct llca);
			break;
		case TOKEN_RING:
		case ETHERNET_8022:
		case ETHERNET_8023:
			dlsap = 0xe0;
			llc.llcc.lbf_sap = 0xe0;
			addr_len = sizeof(struct llcc);
			break;
		case ETHERNET_SNAP:
			dlsap = 0xaa;
			llc.llcb.lbf_sap = 0xaa;
			addr_len = sizeof(struct llcc);
			break;
		default:
			tet_infoline("Error: Unknown Frame Type!!\n");
			return(-1);
	}
	
	/*
	 * bind the sap ..
	 */
	bindreq.dl_primitive = DL_BIND_REQ;
	bindreq.dl_sap = dlsap;
	bindreq.dl_max_conind = 0;
	bindreq.dl_service_mode = DL_CLDLS;
	bindreq.dl_conn_mgmt = 0;

	if (dl_bind(fd, &bindreq, &bindack) < 0) {
		tet_infoline("Error: Bind Failure: %x\n", dl_errno);
		return(-1);
	}

	/*
	 * Frametype specific action..
	 */
	switch (frame) {
	   case ETHERNET_8023:
		if (dl_ccsmacdmode(fd) < 0) {
			tet_infoline("Error: Could not set CSMACD mode\n");
			return(-1);
		}
		/* make a raw packet */
		buf[0] = 0xff;
		buf[1] = 0xff;
		break;
	   case ETHERNET_SNAP:
		/* do a subsequent bind..  */
		snap_sap.snap_global = 0;
		snap_sap.snap_local  = 0x8137;
		dl_subsbind(fd, snap_sap);			
		break;
	   default:
		break;
	}

	*tn_eventp = DL_NOEVENT;
	dl_retry = DL_RETRY_CNT;

	while ((*tn_eventp == DL_NOEVENT) && dl_retry--) {
		dl_timeout = DL_RETRY_TIMEOUT;

		/*
	 	* now send the data..
	 	*/
		if (dl_sndudata(fd, buf, len, &llc, addr_len) < 0) {
			tet_infoline(SEND_FAIL);
			ret_val = -1;
			break;
		}	
		while ((*tn_eventp == DL_NOEVENT) && dl_timeout--) 
			sleep(1);
	}

	/*
	 * unbind the stream ..
	 */
	dl_unbind(fd);
	close(fd);
	return(ret_val);
}
