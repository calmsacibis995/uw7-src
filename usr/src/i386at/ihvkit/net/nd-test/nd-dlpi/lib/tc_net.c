/*
 * tc_net.c : DLPI test net source.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <stropts.h>
#include <nlist.h>
#include <sys/types.h> 
#include <sys/ipc.h>
#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>
#include <errno.h>
#include <dl.h>
#include <tc_res.h>
#include <tc_net.h>
#include <tc_msg.h>
#include <tet_api.h>
#include <config.h>
#include <tc_frame_msg.h>

extern int   dl_errno;
extern int   dl_debug;

/*
 * The net configuration array.. the first one is the test monitor 
 */
dl_node_t	tn_net[DL_MAX_NODES];
int		tn_node;	/* my rank in the configuration array */
int		tn_max_nodes;
int		tn_listen_fd;	/* the DLPI listen stream fd */
int		tn_promis_fd;	/* the DLPI promiscuous stream fd */ 
int		*tn_eventp;	/* the monitor event */
int		*tn_pktcntp;	/* no of echoreply received in stress */
int		tn_listener;	/* the listener thread pid .. */
int		tn_promisc;	/* the promiscuous thread pid .. */
int		tn_mac_type;	/* ethernet or token ring */
char		tn_name[30];	/* the node name */
char		tn_test_maddr[] = { 0xd, 0xd, 0xa, 0xa, 0xd, 0xd }; 
char		tn_test_eaddr[] = { 0x0, 0xd, 0x0, 0xa, 0xd, 0xd }; 
char 		*nodenames[] = {"NODE0", "NODE1", "NODE2"};
unsigned char	eth_snap[] = {0x0aa, 0x0aa, 0x03, 0, 0, 0, 0x081, 0x037};
unsigned char	promisc_addr[] = { 0x00,0x01,0x02,0x03,0x04,0x05,0xff,0xff };
char		*dl_sig = "DLPI Certification Packet V2.2- Have Fun";

void 		tet_msg(char *format, ...);

char *tn_init_err[] = {
	OPEN_FAIL,
	BIND_FAIL,
	GETMACADDR_FAIL,
	NODE_NAME_FAIL,
	NOT_CONFIGURED,
	GEN_CONFIG_FAIL,
	SAME_PHYSNET_FAIL
	};

char *tc_frames[] = {
	"ETHERNET_II",
	"ETHERNET_802.2",
	"ETHERNET_SNAP",
	"ETHERNET_802.3_RAW",
	"TOKEN_RING",
	"TOKEN_RING_SNAP"
	};

/*
 * Load the configuration information to dl_net array.. fills in 
 * the DL address information also.. it find's out the node number 
 * also.. open the test stream also..
 */ 
tn_init()
{
	int		i,j;
	char		eaddr[DL_MAC_ADDR_LEN];
	unsigned char 	*sptr;
	char		*ptr;
	char		buf[30];
	dl_bind_req_t	bindreq;
	dl_bind_ack_t	bindack;
	dl_info_ack_t	infoack;
	
	if ((tn_max_nodes = getnodecnt()) <= 0) 
		return(-6);
	if (getdevname() < 0) 
		return(-6);	
	if (getlocalname(tn_name) < 0)
		return(-6);

	/*
	 * Get the net configuration and the machine name ..
	 */
	for (i = 0; i < tn_max_nodes; i++) {
		nodenames[i] = tet_getvar(nodenames[i]);
		if (nodenames[i] == NULL) {
			return(-4);
		}	
	}


	/*
 	 * Get each m/c name .. address .. status is DL_NODE_OK by default..
	 */	
	tn_node = -1;
	DL_DEBUG1(2,"tn_init: INFO: Nodes configured = %d\n", tn_max_nodes);
	for (i = 0; i < tn_max_nodes; i++) {
		strcpy(tn_net[i].dn_name, nodenames[i]); 
		if (getaddr(nodenames[i], tn_net[i].dn_llc_addr.lbf_addr) < 0) {
			DL_DEBUG(1,"tn_init: ERROR: getaddr failed\n");
			return (-3);
		}
		tn_net[i].dn_llc_addr.lbf_sap = DL_TEST_SAP;
		if (strcmp(tn_name, nodenames[i]) == 0)	
			tn_node = i;
	}
	/*
	 * The local host is not in configuration array ..
	 */
	if (tn_node == -1) {
		DL_DEBUG1(1,"tn_init: ERROR: %s not in configuration\n", tn_name);
		return(-5);
	}
	/*
	 * Display the current Configuration
	 */
	for (i = 0; i < tn_max_nodes; i++) {
		DL_DEBUG1(2,"tn_init: INFO:<%s [", tn_net[i].dn_name);
		for (j = 0; j < LLC_ADDR_LEN; j++) {
			DL_DEBUG1(2,"%02X:", 
				tn_net[i].dn_llc_addr.lbf_addr[j] & 0x0ff);
		}
		DL_DEBUG1(2," %x]>\n", tn_net[i].dn_status);
	}

	if ((tn_listen_fd = open(netdev, O_RDWR)) < 0) {
		DL_DEBUG(1,"tn_init: ERROR: open failed\n");
		tet_infoline(OPEN_FAIL);
		return(-1);
	}
	
	/*
	 * bind the DL_TEST_SAP address to DLPI test stream. 
	 */

	bindreq.dl_primitive 	= DL_BIND_REQ;
	bindreq.dl_sap		= DL_TEST_SAP;
	bindreq.dl_max_conind	= 0;
	bindreq.dl_service_mode	= DL_CLDLS;
	bindreq.dl_conn_mgmt	= 0;
	bindreq.dl_xidtest_flg	= 0;
	if (dl_bind(tn_listen_fd, &bindreq, &bindack) < 0) { 
		DL_DEBUG(1,"tn_init: ERROR: dl_bind failed\n");
		return(-2);
	}

	/* find out the media type - ethernet or token ring */

	if (dl_info(tn_listen_fd, &infoack) < 0) {
		DL_DEBUG(1,"tn_init: ERROR:dl_info failed\n");
		return(-1);
	}
	tn_mac_type = infoack.dl_mac_type;
	return(0);
}

/*
 * tn_monitor:
 */
tn_monitor(which)
int	which;	/* which thread to start */
{
	int	*shmp;
	int	fd;

	if (which == 0)
		return;
	/*
	 * create and attach the shared memory
	 * tn_listen reports the event in shared tn_eventp..
         */
	shmp = (int *)getshm(DL_SHM_KEY, 2 * sizeof(int));
	/*
	 * make them point to proper place ..
	 */
	tn_eventp = shmp++;
	tn_pktcntp = shmp;
	/*
	 * clean both the location ..
	 */
	*tn_eventp = 0;
	*tn_pktcntp = 0;
	
	if (which & TN_LISTEN) {
		if ((tn_listener = fork()) == 0) {
			tn_listen();
		}
	}

	if ((which & TN_PHYS_PROMISCUOUS) || (which & TN_SAP_PROMISCUOUS)) {
		if ((tn_promisc = fork()) == 0) {
			tn_promiscuous(which);
		}
	}
}


/*
 * tn_listen: Each node has a listener thread to respond to the test pkts
 * from the monitor node..
 */
tn_listen()
{
	dl_pkt_t	*dl_req;	
	char		*dl_pkt_data;
	union llc_bind_fmt *llcp;
	int		i, res;
	int		ret;
	int		setpromisc = 0;
	int		oncedone = 0;	/* implement exactly once semantic */
	int		flags;
	union DL_primitives *indp;
	int		indlen;
	int		datalen;
	char		*s;
	char	tmpbuf[sizeof(union DL_primitives) + 2 * sizeof(struct llcc)];
	char	pktdata[1500];
	/*
	 * Listener .. keep on listening may be echo back sometimes .. 
	 */
	DL_DEBUG(1,"tn_listen: listener started...\n");

	indp = (union DL_primitives *)tmpbuf;
	dl_req = (dl_pkt_t *)pktdata;
	dl_pkt_data = (char *)dl_req + sizeof(dl_pkt_t);

	while (1) {
		flags = 0;
		datalen = sizeof(pktdata);
		indlen = sizeof(union DL_primitives) + 2 * sizeof(struct llcc);
		if (dl_getmsg(tn_listen_fd, (char *)indp, &indlen,
			(char *)dl_req, &datalen, &flags, BLOCKING) < 0) {
			DL_DEBUG(1,"tn_listen: dl_getmsg failed\n");
			switch (dl_errno) {
			   case DL_EALLOC:	
				DL_DEBUG(1,ALLOC_FAIL);
				break;
			   case DL_EGETMSG:
				DL_DEBUG1(1,GETMSG_FAIL, errno);
				break;
			   case DL_EPROTO:
				DL_DEBUG(1,"Not DL_UNITDATA_IND\n");
				break;	
			   default:
				DL_DEBUG1(1,RCVMSG_FAIL, errno);
				break;
			}
			continue;
		}
		else {
			DL_DEBUG(2,"tn_listen: INFO:dl_getmsg successful\n");
			DL_DEBUG2(3,"tn_listen: INFO:datalen=%d, indlen=%d\n",datalen, indlen);
			DL_DEBUG(3,"tn_listen: INFO:data [");
			for (i = 0; i < datalen; i++)
				DL_DEBUG1(3,"%x ",((char *)dl_req)[i]);
			DL_DEBUG(3,"]\n");
			DL_DEBUG(3,"                cntl [");
			for (i=0;i < indlen;i++)
				DL_DEBUG1(3,"%x ", ((char *)indp)[i] & 0xFF);
			DL_DEBUG(3,"]\n");
		}

		switch(indp->dl_primitive) {
		   case DL_UNITDATA_IND:
			DL_DEBUG(2,"tn_listen: INFO:DL_UNITDATA_IND received\n");
			/* fill in the address of the sending DLS.. */
			/* If it is Token Ring, clear SR bit */
			llcp = (union llc_bind_fmt *)((char *)indp +
					indp->unitdata_ind.dl_src_addr_offset);
			if (tn_mac_type == DL_TPR)
				llcp->llcc.lbf_addr[0] &= ~0x80;
			break;
		   case DL_TEST_IND:
			DL_DEBUG(2,"tn_listen: INFO:DL_TEST_IND received\n");
			if (dl_testres(tn_listen_fd,
				(char *)indp+indp->test_ind.dl_src_addr_offset,
				(char *)indp+indp->test_ind.dl_src_addr_length)
					< 0) {
				DL_DEBUG(1,"tn_listen: dl_testres failed\n");
			} 
			DL_DEBUG(2,"dl_test_res:::");	
			break;
		   case DL_TEST_CON:
			DL_DEBUG(2,"tn_listen: INFO:DL_TEST_CON received\n");
			*tn_eventp = DL_TESTCON;
			break;
		   case DL_XID_IND:
			DL_DEBUG(2,"tn_listen: INFO:DL_XID_IND received\n");
			if (dl_xidres(tn_listen_fd,
				(char *)indp+indp->xid_ind.dl_src_addr_offset,
				(char *)indp+indp->xid_ind.dl_src_addr_length)
					< 0) {
				DL_DEBUG(1,"tn_listen: dl_xidres failed\n");
			} 
			break;
		   case DL_XID_CON:
			DL_DEBUG(2,"tn_listen: INFO:DL_XID_CON received\n");
			*tn_eventp = DL_XIDCON;
			break;	
		   case DL_UDERROR_IND:
			DL_DEBUG2(1,"tn_listen: INFO:DL_UDERROR_IND prim: %x err: %x\n", ((dl_uderror_ind_t *)indp)->dl_primitive, ((dl_uderror_ind_t *)indp)->dl_errno);
			break;
		   default:
			DL_DEBUG1(1,"tn_listen: ERROR:primitive: %x ", 
						indp->dl_primitive); 
			if (indp->dl_primitive == DL_ERROR_ACK) {
				DL_DEBUG2(1,"err_prim :%x err_no: %x\n",
				((dl_error_ack_t *)indp)->dl_error_primitive,
					((dl_error_ack_t *)indp)->dl_errno);
			}
			break;
		}

		if ( indp->dl_primitive != DL_UNITDATA_IND) 
			continue;

		switch (dl_req->dp_primitive) {
		   case DL_LOOP_REQ:
			DL_DEBUG(2,"Got LOOP request\n");
			res = PASS;
			if (datalen != dl_req->dp_len) { 
				tet_msg(WRONG_LEN_RECV, datalen,dl_req->dp_len);
				res = FAIL;
			}
			/* check for the address */
			for (i = 0; i < LLC_ADDR_LEN; i++)
				if (llcp->llcc.lbf_addr[i] != 
					tn_net[0].dn_llc_addr.lbf_addr[i])
					break;
			if (i != LLC_ADDR_LEN)  {
				tet_infoline(WRONG_SRC_ADDR);
				res = FAIL;
			}
			if (llcp->llcc.lbf_sap != DL_TEST_SAP) {
				tet_infoline(WRONG_SSAP);
				res = FAIL;
			}
			if (res == FAIL) {
				*tn_eventp = DL_LOOPFAIL;
			}
			else {
				*tn_eventp = DL_LOOPPASS;
			}
			break;
		   case DL_MODE_PASS:
			*tn_eventp = DL_MODEPASS;
			break;
		   case DL_MODE_FAIL:
			*tn_eventp = DL_MODEFAIL;
			break;
		   case DL_SET_PROMISC:
			/* set the listener in promiscuous mode */
			if (!oncedone) {
				oncedone = 1;
				if (dl_promiscon(tn_listen_fd) < 0) {
					setpromisc = 0;
				}
				else 
					setpromisc = 1;
			}
			DL_DEBUG1(2,"listen: %x\n", setpromisc);
			if (setpromisc)
				dl_req->dp_primitive = DL_MODE_PASS;
			else
				dl_req->dp_primitive = DL_MODE_FAIL;
			dl_req->dp_sender = tn_node;
			dl_req->dp_len = sizeof(dl_pkt_t) + strlen(dl_sig);
			DL_DEBUG(2,"tn_listen: Echo To: [");
			for (i = 0; i < LLC_ADDR_LEN; i++) {
				DL_DEBUG1(2,"%x ",llcp->llcc.lbf_addr[i]&0xff);
				}
			DL_DEBUG(2,"]\n");
			if (dl_sndudata(tn_listen_fd, (char *)dl_req, 
					dl_req->dp_len,
					(char *)&llcp,
					sizeof(struct llcc)) < 0) {
					DL_DEBUG(1,"tn_listen: Echo Failed\n");
				} 
				break;
		   case DL_CLEAR_PROMISC:
			/* reset the listener's promiscuous mode */
			if (setpromisc && dl_promiscoff(tn_listen_fd) >= 0) {
				dl_req->dp_primitive = DL_MODE_PASS;
				setpromisc = 0;
			}
			else {
				if (setpromisc)
					dl_req->dp_primitive = DL_MODE_FAIL;
				else
					dl_req->dp_primitive = DL_MODE_PASS;
			}
			dl_req->dp_sender = tn_node;
			dl_req->dp_len = sizeof(dl_pkt_t) + strlen(dl_sig);
			DL_DEBUG(2,"tn_listen: Echo To: [");
			for (i = 0; i < LLC_ADDR_LEN; i++) {
				DL_DEBUG1(2,"%x ", llcp->llcc.lbf_addr[i]&0xff);
			}
			DL_DEBUG(2,"]\n");
			if (dl_sndudata(tn_listen_fd, (char *)dl_req,
					dl_req->dp_len,
					(char *)&llcp,
					sizeof(struct llcc)) < 0) {
				DL_DEBUG(1,"tn_listen: Echo Failed\n");
			} 
			break;
		   case DL_FRAME_FAIL:
			DL_DEBUG(2,"tn_listen: INFO: got DL_FRAME_FAIL\n");
			*tn_eventp = DL_FRFAIL;
			 break;
		   case DL_FRAME_OK:
			*tn_eventp = DL_FROK;
			break;
		   case DL_FRAME_NOTOK:
			DL_DEBUG(2,"tn_listen: INFO: got DL_FRAME_NOTOK\n");
			*tn_eventp = DL_FRNOTOK;
			break;
		   case DL_CHK_FRAME_OK:
			*tn_eventp = DL_CHK_FRAME_OK;
			break;
		   case DL_CHK_FRAME:
			DL_DEBUG1(2,FRAME_REQ, tc_frames[dl_req->dp_test]);
			ret = tn_chkframe(dl_req->dp_test, llcp, dl_req );
			if (ret < 0) {
				dl_req->dp_primitive = DL_FRAME_FAIL;
			}
			else 
				dl_req->dp_primitive = ret; 

			dl_req->dp_sender = tn_node;
			dl_req->dp_len = sizeof(dl_pkt_t) + strlen(dl_sig);
			/* reply .. */	
			DL_DEBUG(2,"tn_listen: Echo To: [");
			for (i = 0; i < sizeof(struct llcc); i++) {
				DL_DEBUG1(2,"%x ", llcp->llcc.lbf_addr[i]&0xff);
			}
			DL_DEBUG(2,"]\n");
			if (dl_sndudata(tn_listen_fd, (char *)dl_req, 
					dl_req->dp_len,
					(char *)llcp,
					sizeof(struct llcc)) < 0) {
				DL_DEBUG(1,"tn_listen: Echo Failed\n");
			} 
			break;
		   case DL_START_REQ:
			dl_req->dp_primitive = DL_START_REP;
			dl_req->dp_sender = tn_node;
			dl_req->dp_len = sizeof(dl_pkt_t) + strlen(dl_sig);
			/* reply .. DL_START_REP and go in to
			 * co-operative mode .. which is test requset
			 * specific..
			 */	
			DL_DEBUG(2,"tn_listen: Echo To: [");
			for (i = 0; i < sizeof(struct llcc); i++) {
				DL_DEBUG1(2,"%x ", llcp->llcc.lbf_addr[i]&0xff);
			}
			DL_DEBUG(2,"]\n");
			if (dl_sndudata(tn_listen_fd, (char *)dl_req,
					dl_req->dp_len,
					(char *)&llcp,
					sizeof(struct llcc)) < 0) {
				DL_DEBUG(1,"tn_listen: Echo Failed\n");
			} 
			break;	
		   case DL_ECHO_REP: 
			DL_DEBUG(2,"tn_listen: got DL_ECHO_REP - Test case = ");
			if (tn_node != 0) {
				DL_DEBUG1(1,"tn_listen: Unexpected (%x)\n",
						dl_req->dp_test); 
				break;
			}
			switch (dl_req->dp_test) {
			   case DL_STRESS_TEST:
				DL_DEBUG(2,"DL_STRESS_TEST\n");
				*tn_pktcntp = *tn_pktcntp + 1;
				break;
			   case DL_LINK_TEST:
				DL_DEBUG(2,"DL_LINK_TEST\n");
				*tn_eventp = DL_LINK;
				break;
			   case DL_EADDR_TEST:
				DL_DEBUG(2,"DL_EADDR_TEST\n");
				*tn_eventp = DL_EADDR;
				break;
			   case DL_MADDR_TEST:
				DL_DEBUG(2,"DL_MADDR_TEST\n");
				*tn_eventp = DL_MADDR;
				break;
			   case DL_PROMISC_TEST:	
				DL_DEBUG(2,"DL_PROMISC_TEST\n");
				break;	
			   case DL_SLPCFLG_TEST:
				DL_DEBUG(2,"DL_SLPCFLG_TEST\n");
				/*
				 * is it from remote m/c
				 */
				if (!isme(llcp))
					*tn_eventp = DL_SLPCFLG;	
				/*
				 * else ignore 
				 */
				break;
			}
			break;
		   case DL_ECHO_REQ:
			DL_DEBUG(2,"tn_listen: got DL_ECHO_REQ - Test case = ");
			dl_req->dp_primitive = DL_ECHO_REP;
			dl_req->dp_sender = tn_node;
			/* 
			 * decide whom to send back..
			 */
			switch (dl_req->dp_test) {
			   case DL_EADDR_TEST:
			   case DL_MADDR_TEST:
				DL_DEBUG(2,"DL_EADDR_TEST/DL_MADDR_TEST\n");
				DL_DEBUG(2,"[");
				for (i = 0; i < LLC_ADDR_LEN; i++) {
					DL_DEBUG1(2,"%x ", 
					dl_pkt_data[i] & 0x0ff);
					llcp->llcc.lbf_addr[i] = dl_pkt_data[i];
				}
				DL_DEBUG(2,"]\n");
				break;
			   case DL_STRESS_TEST:
				DL_DEBUG(2,"DL_STRESS_TEST\n");
				break;
			   case DL_LINK_TEST:
				DL_DEBUG(2,"DL_LINK_TEST\n");
				break;
			   default:
				DL_DEBUG(1,"tn_listen: unknown test\n");
				break;
			 }  
			/*
			 * Echo it back .. 
			 */
			DL_DEBUG(2,"tn_listen: INFO:Echo To: [");
			for (i = 0; i < sizeof(struct llcc); i++) {
				DL_DEBUG1(2,"%x ", llcp->llcc.lbf_addr[i]&0xff);
			}
			DL_DEBUG(2,"]\n");
			if (dl_sndudata(tn_listen_fd, (char *)dl_req,
					dl_req->dp_len,
					(char *)llcp,
					sizeof(struct llcc)) < 0) {
				DL_DEBUG(2,"tn_listen: Echo Failed\n");
			} 
			break;
		   case DL_RING_REQ:
			if (tn_node == TN_NEXT_TO_MON) {
				/*
			 	 * Pass it to  the next neighbour
			 	 */
				DL_DEBUG(2,"tn_listen: DL_RING_REQ\n");
				DL_DEBUG(2,"tn_listen: INFO:Echo To: [");
				for (i = 0; i < sizeof(struct llcc); i++) {
				    if (dl_req->dp_test == DL_PHYSPROMISC_TEST)
					llcp->llcc.lbf_addr[i] =promisc_addr[i];
					DL_DEBUG1(2,"%x ", llcp->llcc.lbf_addr[i]&0xff);
				}
				DL_DEBUG(2,"]\n");
				if (dl_sndudata(tn_listen_fd, (char *)dl_req,
						dl_req->dp_len,
						(char *)llcp,
						sizeof(struct llcc)) < 0) {
					DL_DEBUG(1,"tn_listen:dl_sndudata failed\n");
				} 
			}
			break;
		   default:
			break;
		}
	}
	
}

/*
 * Monitor node also has a promiscuous thread .. attached to a PROMISCUOUS 
 * sap ..
 */
tn_promiscuous(int level)
{
	int		datalen, addrlen;
	dl_pkt_t	*dl_res;
	struct llcc	llcp;
	dl_bind_req_t	bindreq;
	dl_bind_ack_t	bindack;
	char		pktdata[DL_MAX_PKTLEN];
	int		i;

	if ((tn_promis_fd= open(netdev, O_RDWR)) < 0) {
		DL_DEBUG(1,"tn_promiscuous: DLPI open failure!!!\n");
		return(FAIL);
	}

	/*
	 * bind the PROMISCUOUS_SAP address to DLPI promiscuous stream. 
	 */
	bindreq.dl_primitive 	= DL_BIND_REQ;
	bindreq.dl_sap		= PROMISCUOUS_SAP;
	bindreq.dl_max_conind	= 0;
	bindreq.dl_service_mode	= DL_CLDLS;
	bindreq.dl_conn_mgmt	= 0;
	bindreq.dl_xidtest_flg	= 0;
	if (dl_bind(tn_promis_fd, &bindreq, &bindack) < 0) { 
		DL_DEBUG(1,"tn_promiscuous: dl_bind failed\n");
		return(FAIL);
	}
	/*
 	 * Put yourself in promiscuous mode..
	 */	
	if (level == TN_PHYS_PROMISCUOUS) 
		if (dl_promiscon(tn_promis_fd) < 0) {
			DL_DEBUG(1,"tn_promiscuous: Could n't set promiscuous mode\n");
			return(FAIL);
		}

	DL_DEBUG(2,"tn_promiscuous: Wait..\n");
	while (1) {
		dl_res = (dl_pkt_t *)pktdata;
		datalen = sizeof(pktdata);
		addrlen = sizeof(struct llcc);
		if (dl_rcvudata(tn_promis_fd, (char *)dl_res, &datalen, &llcp, &addrlen) < 0) {
			DL_DEBUG(2,"tn_promiscuous: dl_rcvudata failure\n");
		}
		else { 
			DL_DEBUG2(4,"tn_promiscuous: INFO:datalen=%d, addrlen=%d\n",datalen,addrlen);
			DL_DEBUG(4,"tn_promiscuous: INFO:data [");
			for (i = 0; i < datalen; i++)
				DL_DEBUG1(4,"%x ",((char *)dl_res)[i]);
			DL_DEBUG(4,"]\n");
			DL_DEBUG(4,"                addr [");
			for (i=0;i < addrlen;i++)
				DL_DEBUG1(4,"%x ", ((char *)&llcp)[i] & 0xFF);
			DL_DEBUG(4,"]\n");
			/*
			 * check if the the packet type is DL_RING_REQ..
			 * and destined for promisc address
			 */
			if (dl_res->dp_primitive == DL_RING_REQ) {
				DL_DEBUG(2,"tn_promiscuous: INFO: got DL_RING_REQ\n");
				*tn_eventp = DL_PROMISC;
			}
		}
	}
}

/* 
 * Test whether the local node is able to send packet to itself.
 */
tn_loop(node)
dl_node_t	node;
{
	dl_pkt_t	*dl_pkt;
	char		*dl_pkt_data;
	int		dl_retry;
	int		ret;
	int		i;
	
	*tn_eventp = DL_NOEVENT; 
	dl_retry = DL_RETRY_CNT;

	dl_pkt = (dl_pkt_t *)malloc(sizeof(dl_pkt_t) + strlen(dl_sig));
	if (dl_pkt == NULL) {
		DL_DEBUG(1,"tn_loop: ERROR:malloc failed\n");
		return(FAIL);
	}

	dl_pkt->dp_primitive	= DL_LOOP_REQ;
	dl_pkt->dp_len = sizeof(dl_pkt_t) + strlen(dl_sig);
	dl_pkt_data = (char *)dl_pkt + sizeof(dl_pkt_t);

	for (i = 0;i < strlen(dl_sig);i++)
		dl_pkt_data[i] = dl_sig[i];

	while ((*tn_eventp == DL_NOEVENT) && dl_retry--) {
		/*
		 * Retry till the monitor node receives thr ECHO_REP packet
		 */
		if (dl_sndudata(tn_listen_fd, (char *)dl_pkt, dl_pkt->dp_len,
			(char *)&node.dn_llc_addr, sizeof(struct llcc)) < 0) {
			ret = FAIL;
			break;
		} 
		sleep(DL_RETRY_TIMEOUT);
		/*
		 *  Wait for packet .. 
		 */
	}
	DL_DEBUG1(2,"tn_loop: event [%x]\n", *tn_eventp);
	if (*tn_eventp == DL_LOOPPASS) {
		DL_DEBUG(2,"tn_loop: Passed\n");
		ret = PASS; 
	}
	else {
		/* retry did't help */
		DL_DEBUG(2,"tn_loop: Failed\n");
		ret = FAIL;
	}
	return(ret);
}

/* 
 * Test whether the local node is connected to the given remote node.
 * Sends DL_ECHO_REQ packet and expect a ECHO_REPLY from the target.
 * It retransmits DL_RETRY_CNT before declaring it fatal..
 */
tn_link(node)
dl_node_t	node;
{
	dl_pkt_t	*dl_pkt;
	char		*dl_pkt_data;
	int		retry;
	int		retry_timeout;
	int		ret;
	int		i;
	
	DL_DEBUG(2,"tn_link: Test\n");

	dl_pkt = (dl_pkt_t *)malloc(sizeof(dl_pkt_t) + strlen(dl_sig));
	if (dl_pkt == NULL) {
		DL_DEBUG(1,"tn_link: ERROR:malloc failed\n");
		return(FAIL);
	}

	dl_pkt->dp_primitive = DL_ECHO_REQ;
	dl_pkt->dp_test      = DL_LINK_TEST;
	dl_pkt->dp_sender    = tn_node;
	dl_pkt->dp_len	     = sizeof(dl_pkt_t) + strlen(dl_sig);
	dl_pkt_data = (char *)dl_pkt + sizeof(dl_pkt_t);

	for (i = 0; i < (int)strlen(dl_sig); i++)
		dl_pkt_data[i] = dl_sig[i];
	
	*tn_eventp = DL_NOEVENT; 
	retry = DL_RETRY_CNT;

	while ((*tn_eventp != DL_LINK) && retry--) {
		retry_timeout = DL_RETRY_TIMEOUT;
		/*
		 * Retry till the monitor node receives thr ECHO_REP packet
		 */
		if (dl_sndudata(tn_listen_fd, (char *)dl_pkt, dl_pkt->dp_len,
			(char *)&node.dn_llc_addr, sizeof(struct llcc)) < 0) {
			DL_DEBUG(1,"tn_link: ERROR:dl_sndudata failed\n");
			ret = FAIL;
			break;
		} 
		/*  Wait for packet .. */
		while ((*tn_eventp != DL_LINK) && retry_timeout--) 
			sleep(1);
	}
	DL_DEBUG1(2,"tn_link: event [%x]\n", *tn_eventp);
	if (*tn_eventp == DL_LINK) {
		DL_DEBUG(2,"tn_link: INFO:Passed\n");
		ret = PASS; 
	}
	else {
		/* retry didn't help */
		DL_DEBUG(2,"tn_link: INFO:Failed\n");
		ret = FAIL;
	}
	free(dl_pkt);
	return(ret);
}


/*
 * Test whether the monitor node is connected to each of it's neighbour  
 * uses tn_link ..
 */
tn_neighbours()
{
	int	i;
	int	ret;
	
	DL_DEBUG(2,"tn_neighbours: Test\n");

	for (i = 0; i < tn_max_nodes; i++) {
		/*
		 * if it's not me .. 
		 */
		if ((i != tn_node) &&  (tn_link(tn_net[i]) != PASS)) {
			ret = FAIL;
			break;					
		}
	}
	/*
	 * if for all neighbouring node the link test passed ..
	 */
	if (i == tn_max_nodes) {
		DL_DEBUG(2,"tn_neighbours: INFO:All responded\n");
		ret = PASS;
	}

	return(ret);
}

/*
 * tn_start_req: Request the listener to co-operate with the test request...
 */
tn_start_req(id, node)
int		id;
dl_node_t	node;
{
	dl_pkt_t	*dl_pkt;
	char		*dl_pkt_data;
	int		dl_retry;
	int		i, ret;
	
	DL_DEBUG(2,"tn_start: Test\n");

	dl_pkt = (dl_pkt_t *)malloc(sizeof(dl_pkt_t) + strlen(dl_sig));
	if (dl_pkt == NULL) {
		DL_DEBUG(1,"tn_start_req: ERROR: malloc failed\n");
		return(FAIL);
	}

	dl_pkt->dp_primitive = DL_START_REQ;
	dl_pkt->dp_test      = id;
	dl_pkt->dp_sender    = tn_node;
	dl_pkt->dp_len	    = sizeof(dl_pkt_t) + strlen(dl_sig);
	dl_pkt_data = (char *)dl_pkt + sizeof(dl_pkt_t);

	for (i = 0; i < (int)strlen(dl_sig); i++)
		dl_pkt_data[i] = dl_sig[i];
	
	*tn_eventp = DL_NOEVENT; 
	dl_retry = DL_RETRY_CNT;

	while ((*tn_eventp != DL_START) && dl_retry--) {
		/*
		 * Retry till the monitor node receives thr ECHO_REP packet
		 */
		if (dl_sndudata(tn_listen_fd, (char *)dl_pkt, dl_pkt->dp_len, 
			(char *)&node.dn_llc_addr, sizeof(struct llcc)) < 0) {
			ret = FAIL;
			break;
		} 
		sleep(DL_RETRY_TIMEOUT);
		/*
		 *  Wait for packet .. 
		 */
	}
	DL_DEBUG1(2,"tn_start_req: event [%x]\n", *tn_eventp);
	if (*tn_eventp == DL_START) {
		DL_DEBUG(2,"tn_start_req: Passed\n");
		ret = PASS; 
	}
	else {
		/* retry did't help */
		DL_DEBUG(2,"tn_start_req: Failed\n");
		ret = FAIL;
	}
	return(ret);
}

/*
 * tn_stop_req: Request the listener to go out of the special mode ..
 * and go back to old mode...
 */
tn_stop_req(id, node)
int		id;
dl_node_t	node;
{
	dl_pkt_t	*dl_pkt;
	char		*dl_pkt_data;
	int		dl_retry;
	int		i, ret;
	
	DL_DEBUG(2,"tn_Stop_req: Test\n");

	dl_pkt = (dl_pkt_t *)malloc(sizeof(dl_pkt_t) + strlen(dl_sig));
	if (dl_pkt == NULL) {
		DL_DEBUG(1,"tn_stop_req: ERROR: malloc failed\n");
		return(FAIL);
	}

	dl_pkt->dp_primitive = DL_STOP_REQ;
	dl_pkt->dp_test      = id; 
	dl_pkt->dp_sender    = tn_node;
	dl_pkt->dp_len	    = sizeof(dl_pkt_t) + strlen(dl_sig);
	dl_pkt_data = (char *)dl_pkt + sizeof(dl_pkt_t);

	for (i = 0; i < (int)strlen(dl_sig); i++)
		dl_pkt_data[i] = dl_sig[i];
	
	*tn_eventp = DL_NOEVENT; 
	dl_retry = DL_RETRY_CNT;

	while ((*tn_eventp != DL_STOP) && dl_retry--) {
		/*
		 * Retry till the monitor node receives thr ECHO_REP packet
		 */
		if (dl_sndudata(tn_listen_fd, (char *)dl_pkt, dl_pkt->dp_len,
			(char *)&node.dn_llc_addr, sizeof(struct llcc)) < 0) {
			ret = FAIL;
			break;
		} 
		sleep(DL_RETRY_TIMEOUT);
		/*
		 *  Wait for packet .. 
		 */
	}
	DL_DEBUG1(2,"tn_stop_req: event [%x]\n", *tn_eventp);
	if (*tn_eventp == DL_STOP) {
		DL_DEBUG(2,"tn_stop_req: Passed\n");
		ret = PASS; 
	}
	else {
		/* retry did't help 
		 */
		DL_DEBUG(2,"tn_stop_req: Failed\n");
		ret = FAIL;
	}
	return(ret);
}

tn_chkframe(int frame, union llc_bind_fmt *llcp, dl_pkt_t *dl_req)
{
	int		i;
	int		fd;
	int		len;
	int		dlsap;
	int		nrecv;
	int		valid;
	dl_bind_req_t	bindreq;
	dl_bind_ack_t	bindack;
	struct snap_sap	snap_sap;

	unsigned char	dl_frame[100];
	union llc_bind_fmt	llc;
	int		addr_len;
	int		addrlen;
	int		datalen;
	int		partner;

	/*
	 * open a new stream ..
	 */
	if ((fd = open(netdev, O_RDWR)) < 0) {
		tet_infoline(OPEN_FAIL);
		return(-1);  
	}

	/*
	 * Decide the dlsap to bind..
	 */
	switch (frame) {
		case ETHERNET_II:
		case TOKEN_RING_SNAP:
			dlsap = 0x8137;
			addr_len = sizeof(struct llca);
			break;
		case TOKEN_RING:
		case ETHERNET_8022:
		case ETHERNET_8023:
			dlsap = 0xe0;
			addr_len = sizeof(struct llcc);
			break;
		case ETHERNET_SNAP:
			dlsap = 0xaa;
			addr_len = sizeof(struct llcb);
			break;
		default:
			DL_DEBUG1(1,"tn_chkframe: ERROR:unknown frame type = %d\n",frame);
			tet_infoline("Error: Unknown Frame Type!!\n");
			close(fd);
			return(-1);
	}
	
	/*
	 * bind the sap ..
	 */
	bindreq.dl_primitive 	= DL_BIND_REQ;
	bindreq.dl_sap		= dlsap;
	bindreq.dl_max_conind	= 0;
	bindreq.dl_service_mode	= DL_CLDLS;
	bindreq.dl_conn_mgmt	= 0;
	bindreq.dl_xidtest_flg	= 0;
	if (dl_bind(fd, &bindreq, &bindack) < 0) { 
		DL_DEBUG2(1,"tn_chkframe: ERROR:bind failed for %s dl_errno=%d\n", tc_frames[frame], dl_errno);
		tet_infoline("Error: Bind Failure: %x\n", dl_errno);
		close(fd);
		return(-1);
	}
	/*
	 * Frametype specific action..
	 */
	switch (frame) {
	   case ETHERNET_8023:
		if (dl_ccsmacdmode(fd) < 0) {
			tet_infoline("Error: Could not set CSMACD mode\n");
			close(fd);
			return(-1);
		}
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

	/* send a ready signal .. */	
	dl_req->dp_primitive = DL_CHK_FRAME_OK;
	dl_req->dp_sender = tn_node;
	dl_req->dp_len = sizeof(dl_pkt_t) + strlen(dl_sig);
	DL_DEBUG(2,"tn_listen: Echo To: [");
	for (i = 0; i < sizeof(struct llcc); i++) {
		DL_DEBUG1(2,"%x ", llcp->llcc.lbf_addr[i]&0xff);
	}
	DL_DEBUG(2,"]\n");
	if (dl_sndudata(tn_listen_fd, (char *)dl_req, dl_req->dp_len,
				(char *)llcp, sizeof(struct llcc)) < 0) {
		DL_DEBUG(1,"tn_listen: Echo Failed\n");
	} 
	
	DL_DEBUG1(2,"tn_chkframe: INFO:Wait for the test frame %s..\n", tc_frames[frame]);
	while (1) {
		datalen = sizeof(dl_frame);
		addrlen = addr_len;
		if ((nrecv = dl_rcvudata(fd, dl_frame, &datalen, 
					&llc, &addrlen)) < 0) {
			switch (dl_errno) {
			   case DL_EALLOC:	
				tet_infoline(ALLOC_FAIL);
				break;
			   case DL_EGETMSG:
				tet_msg(GETMSG_FAIL, errno);
				break;
			   case DL_EPROTO:
				tet_infoline("INFO:Not DL_UNITDATA_IND\n");
				break;	
			   default:
				tet_infoline(RCV_FAIL);
				break;
			}
			DL_DEBUG1(1,"tn_chkframe: ERROR: dl_rcvudata failed - dl_errno = %d\n", dl_errno);
			close(fd);
			return -1;
		}
		/* clear the source routing bit */
		if (tn_mac_type == DL_TPR)
			llc.llcc.lbf_addr[0] &= ~0x80;
		/*
		 * is it from the partner NODE i.e NODE1..
		 */
		valid = 1;
		for (i = 0; i < LLC_ADDR_LEN; i++) {
			partner = (tn_node == 0) ? 1 : 0;
			if (tn_net[partner].dn_llc_addr.lbf_addr[i] != 
					llc.llcc.lbf_addr[i]) {
				valid = 0;
				break;
			}
		}
		/*
		 * Try luck in the next packet..
		 */
		if (valid == 0) {
			DL_DEBUG(1,"tn_chkframe: INFO:Not from my partner!!\n");
			DL_DEBUG(2,"                  from[");
			for (i = 0;i < LLC_ADDR_LEN; i++) {
				DL_DEBUG1(2,"%x ", llc.llcc.lbf_addr[i]&0xff);
			}
			DL_DEBUG(2,"]\n");
			continue;
		}

/*
** If four bytes starting at 20 are not 'a's, this packet may not be
** the one, we are looking for. In that case just discard the packet
** and continue listening.
** This is done because of the netware traffic.
*/
		if ((nrecv < 25) || (*((int *)&dl_frame[20]) != 'aaaa')) {
			DL_DEBUG(1,"tn_chkframe: INFO:spurious packet from my partner!!\n");
			continue;	
		}
		/* check the format of the packet..  */
	   	if ((frame == ETHERNET_8023) && 
			(dl_frame[0] == 0xff) && (dl_frame[1] == 0xff)) {
				dl_frame[0] = 'a';
				dl_frame[1] = 'a';
			}

		DL_DEBUG1(2,"Frame is %s\n", tc_frames[frame]);
		for (i = 0; i < DL_FRTEST_LEN; i++) 
			if (dl_frame[i] != 'a') 
				break;
		/*
		 * Clean up the check frame stream..
		 */
		close(fd);

		/* Reply indicating that the answer was valid..  */
		if (i == DL_FRTEST_LEN) {
			tet_infoline(TEST_FRAME_SUCCESS);
			return(DL_FRAME_OK);
		} else { 
			tet_infoline(TEST_FRAME_FAILURE);
			return(DL_FRAME_NOTOK);
		}
	}
}

isme(llcp)
struct llcc	llcp;

{
	int i;

	for (i = 0; i < LLC_ADDR_LEN; i++)
		if (llcp.lbf_addr[i] != tn_net[tn_node].dn_llc_addr.lbf_addr[i])
			return(0);
	return(1);
}
/*
 * Set the listener to mode ..
 */
setlistener(mode, node)
int		mode;
dl_node_t	node;
{
	dl_pkt_t	*dl_pkt;
	char		*dl_pkt_data;
	int		dl_retry;
	int		i, ret;
	
	dl_pkt = (dl_pkt_t *)malloc(sizeof(dl_pkt_t) + strlen(dl_sig));
	if (dl_pkt == NULL) {
		DL_DEBUG(1,"setlistener: ERROR: malloc failed\n");
		return(FAIL);
	}

	dl_pkt->dp_primitive = mode;
	dl_pkt->dp_test      = DL_MODE_TEST;
	dl_pkt->dp_sender    = tn_node;
	dl_pkt->dp_len	    = sizeof(dl_pkt_t) + strlen(dl_sig);
	dl_pkt_data = (char *)dl_pkt + sizeof(dl_pkt_t);

	for (i = 0; i < (int)strlen(dl_sig); i++)
		dl_pkt_data[i] = dl_sig[i];
	
	*tn_eventp = DL_NOEVENT; 
	dl_retry = DL_RETRY_CNT;

	while ((*tn_eventp == DL_NOEVENT) && dl_retry--) {
		/*
		 * Retry till the monitor node receives thr ECHO_REP packet
		 */
		if (dl_sndudata(tn_listen_fd, (char *)dl_pkt, dl_pkt->dp_len,
			(char *)&node.dn_llc_addr, sizeof(struct llcc)) < 0) {
			ret = FAIL;
			break;
		} 
		sleep(DL_RETRY_TIMEOUT);
		/*
		 *  Wait for packet .. 
		 */
	}
	DL_DEBUG1(2,"setlistener: event [%x]\n", *tn_eventp);
	if (*tn_eventp == DL_MODEPASS) {
		ret = PASS; 
	}
	else {
		/* retry did't help 
		 */
		ret = FAIL;
	}
	return(ret);
}
