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

#include "tc_ioc_msg.h"

extern int	errno;
extern int	dl_errno;

extern char *	tn_init_err[];

void tc_sioc();
void tc_eioc();

void ic_gmib0();
void ic_gmib1();
void ic_ccsmacdmode0();
void ic_smib0();
void ic_genaddr0();
void ic_genaddr1();
void ic_glpcflg0();
void ic_slpcflg0();
void ic_gpromisc0();
void ic_spromisc0();
void ic_senaddr0();
void ic_addmulti0();
void ic_delmulti0();
void ic_reset0();
void ic_enable0();
void ic_disable0();

 /*
 * tet setup .. ICs ..
 */
void (*tet_startup)() = tc_sioc; /* Test case start */
void (*tet_cleanup)() = tc_eioc; /* Test case end */

struct tet_testlist tet_testlist[] = {
	{ic_gmib0, 1},
	{ic_gmib1, 2},
	{ic_smib0, 3},
	{ic_ccsmacdmode0, 4},
	{ic_genaddr0, 5},
	{ic_genaddr1, 6},
	{ic_addmulti0, 7},
	{ic_delmulti0, 8},
	{ic_senaddr0, 9},
	{ic_glpcflg0, 10},
	{ic_slpcflg0, 11},
	{ic_spromisc0, 12},
	{ic_gpromisc0, 13},
	{ic_disable0, 14},
	{ic_enable0, 15},
	{ic_reset0, 16},
	{NULL, -1}
};

int	fd;

void
tc_sioc()
{
	int	i;
	int	ret;

	tet_infoline(TC_NIOC);

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
 * DLIOCGMIB: Get MIB variables
 */
void
ic_gmib0()
{
	DL_mib_t	mib;

	tet_infoline(GMIB_PASS);

	if (dl_gmib(fd, &mib) < 0) {
		tet_msg(IocFail(DLIOCGMIB), errno);
		tet_result(TET_FAIL);
	}
	else {
		tet_result(TET_PASS);
	}
	return; 
}

/*
 * The ioctl fails with EINVAL, if inavalid user buffer is given
 */
void
ic_gmib1()
{
	struct strioctl	cmd;
	DL_mib_t	mib;
	int		ret;

	tet_infoline(EinvalPass(DLIOCGMIB));

	cmd.ic_cmd = DLIOCGMIB;
	cmd.ic_timout = 15;
	cmd.ic_len = 0;
	cmd.ic_dp = (char *)&mib;

	ret = ioctl(fd, I_STR, &cmd);

	if (ret < 0) {
		if (errno == EINVAL) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(EinvalFail(DLIOCGMIB), errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(EinvalFail0(DLIOCGMIB));
		tet_result(TET_FAIL);
	}
	return;	
}

/* 
 * DLIOCCSMACDMODE: Normal user fails with EPERM.
 */
void
ic_ccsmacdmode0()
{
	tet_infoline(CCSMACDMODE_PASS);

	if (dl_ccsmacdmode(fd) < 0) {
		if (errno == EPERM) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(EpermFail(DLIOCCSMACDMODE), errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(EpermFail0(DLIOCCSMACDMODE));
		tet_result(TET_FAIL);	
	}
	return;
}

/* 
 * DLIOCSMIB: Normal user fails with EPERM.
 */
void
ic_smib0()
{
	DL_mib_t	mib;	

	tet_infoline(SMIB_PASS0);

	/*
	 * Get the mib values .. and set it back..
	 */
	if (dl_gmib(fd, &mib) < 0) {
		tet_msg(IocFail(DLIOCGMIB), errno);
		tet_result(TET_UNRESOLVED);
		return;	
	}
	if (dl_smib(fd, &mib) < 0) {
		if (errno == EPERM) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(EpermFail(DLIOCSMIB), errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(EpermFail0(DLIOCSMIB));
		tet_result(TET_FAIL);	
	}
	return;
}

/* 
 * DLIOCGENADDR: Gets the Ethernet Address in network order.
 */
void
ic_genaddr0()
{
	char		dl_enaddr[DL_MAC_ADDR_LEN];
	dl_pkt_t	*dl_pkt;
	char		*dl_pkt_data;
	int		dl_retry;
	int		i;

	tet_infoline(GENADDR_PASS);
	
	dl_pkt = (dl_pkt_t *)malloc(sizeof(dl_pkt_t) + DL_MAC_ADDR_LEN + strlen(dl_sig));
	if (dl_pkt == NULL) {
		tet_infoline("ic_genaddr0: ERROR:malloc failed\n");
		tet_result(TET_UNRESOLVED);
		return;
	}

	if (dl_genaddr(tn_listen_fd, dl_enaddr) < 0) {
		tet_infoline(GENADDR_FAIL);
		tet_result(TET_FAIL);
		return;
	}
	
	dl_pkt->dp_primitive = DL_ECHO_REQ;
	dl_pkt->dp_sender    = tn_node;
	dl_pkt->dp_test      = DL_EADDR_TEST;
	dl_pkt->dp_len = sizeof(dl_pkt_t) + DL_MAC_ADDR_LEN + strlen(dl_sig);
	dl_pkt_data = (char *)dl_pkt + sizeof(dl_pkt_t);
	/*
	 * Fill in the ethernet address returned by the IOCTL call..
	 */	
	for (i = 0; i < DL_MAC_ADDR_LEN; i++)
		dl_pkt_data[i] = dl_enaddr[i];

	for (i = 0; i < strlen(dl_sig); i++)
		dl_pkt_data[DL_MAC_ADDR_LEN + i] = dl_sig[i];
	
	*tn_eventp = DL_NOEVENT;
	dl_retry = DL_RETRY_CNT;

	while ((*tn_eventp != DL_EADDR) && dl_retry--) {
		/*
		 * Retry till the monitor node receives the ECHO_REPLY pkt
		 */
		if (dl_sndudata(tn_listen_fd, (char *)dl_pkt, dl_pkt->dp_len,
			(char *)&tn_net[TN_NEXT_TO_MON].dn_llc_addr, 
			sizeof (struct llcc)) < 0) {

			tet_infoline(SEND_FAIL);
			tet_result(TET_UNRESOLVED);
			break;
		} 
		i=0;
		while (( i++ < 5 ) && (*tn_eventp != DL_EADDR))
			sleep(DL_RETRY_TIMEOUT);
		/*
		 *  Wait for packet .. 
		 */
	}
	DL_DEBUG1(2,"ic_genaddr0: event [%x]\n", *tn_eventp);
	if (*tn_eventp == DL_EADDR) {
		tet_result(TET_PASS);
	}
	else {
		/*
 		 * Test if tn_link passes or not ..
		 */
		if (tn_link(tn_net[TN_NEXT_TO_MON]) != PASS) {
			DL_DEBUG(1,"ic_genaddr0: tn_link also failed..\n");
			tet_infoline(LINK_FAIL);
			tet_result(TET_UNRESOLVED);
		}
		else {	
			DL_DEBUG(1,"ic_genaddr0: Failed\n");
			tet_infoline(GENADDR_FAIL);
			tet_result(TET_FAIL);
		}
	}
	return;
}

/*
 * The ioctl return EINVAL if invalid user buffer is given.
 */
void
ic_genaddr1()
{
	struct strioctl	cmd;
	char		eaddr[DL_MAC_ADDR_LEN];
	int		ret;

	tet_infoline(EinvalPass(DLIOCGENADDR));

	cmd.ic_cmd = DLIOCGENADDR;
	cmd.ic_timout = 15;
	cmd.ic_len = 0;
	cmd.ic_dp = eaddr; 

	ret = ioctl(fd, I_STR, &cmd);

	if (ret < 0) {
		if (errno == EINVAL) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(EinvalFail(DLIOCGENADDR), errno);
			tet_result(TET_FAIL);
		}
	}
	else {
			tet_infoline(EinvalFail0(DLIOCGENADDR));
			tet_result(TET_FAIL);
	}	
	return;
}

/* 
 * DLIOCGLPCFLG: Gets the value of local packet copy flag.
 */
void
ic_glpcflg0()
{
	int	lpcflg;

	tet_infoline(GLPCFLG_PASS);

	if (dl_glpcflg(fd, &lpcflg) < 0) {
		tet_msg(GLPCFLG_FAIL0, errno);
		tet_result(TET_FAIL);
	}
	else {
		tet_result(TET_PASS);	
	}
	return;
}

/*
 * DLIOCSLPCFLG: Normal user fails with EPERM error.
 */
void
ic_slpcflg0()
{
	tet_infoline(SLPCFLG_PASS0);

	if (dl_slpcflg(fd) < 0) {
		if (errno == EPERM) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(EpermFail(DLIOCSLPCFLG), errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(EpermFail0(DLIOCSLPCFLG));
		tet_result(TET_FAIL);	
	}
	return;
}


/* 
 * DLIOCSPROMISC: Normal user fails with EPERM error.
 */
void
ic_spromisc0()
{
	tet_infoline(SPROMISC_PASS0);

	if (dl_slpcflg(fd) < 0) {
		if (errno == EPERM) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(EpermFail(DLIOCSPROMISC), errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(EpermFail0(DLIOCSPROMISC));
		tet_result(TET_FAIL);	
	}
	return;
}

/* 
 * DLIOCGPROMISC: Gets the value of the promiscuous flag.
 */
void
ic_gpromisc0()
{
	int	promiscflg;

	tet_infoline(GPROMISC_PASS);

	if (dl_glpcflg(fd, &promiscflg) < 0) {
		tet_msg(GPROMISC_FAIL0, errno);
		tet_result(TET_FAIL);
	}
	else {
		tet_result(TET_PASS);	
	}
	return;
}

/* 
 * DLIOCSENADDR: Normal user fails with EPERM.
 */
void
ic_senaddr0()
{
	int	i;
	char	eaddr[DL_MAC_ADDR_LEN];

	tet_infoline(SENADDR_PASS0);

	if (dl_senaddr(tn_listen_fd, tn_test_eaddr) < 0) {
		if (errno == EPERM) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(EpermFail(DLIOCSENADDR), errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(EpermFail0(DLIOCSENADDR));
		tet_result(TET_FAIL);	
	}
	return;
}

/*
 * DLIOCADDMULTI:  Normal user fails with EPERM.
 */
void
ic_addmulti0()
{
	dl_pkt_t	dl_req;
	int		i, ret;
	int		uid;
	int		dl_retry;
	int		found;
	char		maddr[6];

	tet_infoline(ADDMULTI_PASS0);

	/*
	 * Add yourself to the multicast group..
 	 */
	if (dl_addmulti(tn_listen_fd,  tn_test_maddr) < 0) {
		if (errno == EPERM) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(EpermFail(DLIOCADDMULTI), errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(EpermFail0(DLIOCADDMULTI));
		tet_result(TET_FAIL);	
	}
	return;	
}

/* 
 * DLIOCDELMULTI: Normal user fails with EPERM. 
 */
void
ic_delmulti0()
{
	dl_pkt_t	dl_req;
	int		i, ret;
	int		dl_retry;

	tet_infoline(DELMULTI_PASS0);

	if (dl_delmulti(tn_listen_fd,  tn_test_maddr) < 0) {
		if (errno == EPERM) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(EpermFail(DLIOCDELMULTI), errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(EpermFail0(DLIOCDELMULTI));
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
	tet_infoline(RESET_PASS0);

	if (dl_reset(fd) < 0) {
		if (errno == EPERM) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(EpermFail(DLIOCRESET), errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(EpermFail0(DLIOCRESET));
		tet_result(TET_FAIL);	
	}
	return;
}

/* 
 * DLIOCENABLE: Previledge process can enable the controller.
 */
void
ic_enable0()
{
	tet_infoline(ENABLE_PASS0);

	if (dl_enable(fd) < 0) {
		if (errno == EPERM) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(EpermFail(DLIOCENABLE), errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(EpermFail0(DLIOCENABLE));
		tet_result(TET_FAIL);	
	}
	return;
}

/* 
 * DLIOCDISABLE: Previleged process can disable the controller.
 */
void
ic_disable0()
{
	tet_infoline(DISABLE_PASS0);

	if (dl_disable(fd) < 0) {
		if (errno == EPERM) {
			tet_result(TET_PASS);
		}
		else {
			tet_msg(EpermFail(DLIOCDISABLE), errno);
			tet_result(TET_FAIL);
		}
	}
	else {
		tet_infoline(EpermFail0(DLIOCDISABLE));
		tet_result(TET_FAIL);	
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
	 * close the stream..
	 */
	close(fd);
}



