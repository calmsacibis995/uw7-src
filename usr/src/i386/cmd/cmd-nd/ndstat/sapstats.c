#ident "@(#)sapstats.c	25.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <stropts.h>

#include <sys/types.h>
#include <sys/scodlpi.h>
#include "ndstat.h"

#define BUFSZ	4096
static char buf[BUFSZ];
extern int debug_flag;

int
display_sapstats(int fd)
{
	dl_sap_statistics_req_t	*req;
	dl_sap_statistics_ack_t	*ack;
	struct dlpi_sapstats *sp;
	int n;

	struct strbuf ctl;
	int flags;

	if (type != MDI) {
		return(0);
	}

	ctl.buf = buf;
	ctl.len = sizeof(dl_sap_statistics_req_t);

	req = (dl_sap_statistics_req_t *)buf;
	req->dl_primitive = DL_SAP_STATISTICS_REQ;

	if (putmsg(fd, &ctl, NULL, RS_HIPRI) < 0) {
		fflush(stdout);
		if (debug_flag) {
			catfprintf(stderr, 34, "ndstat: putmsg(%s) failed\n", "DL_SAP_STATISTICS_REQ");
			perror("ndstat");
		}
		return(-1);
	}

	ctl.maxlen = sizeof(buf);
	ctl.len = 0;

	flags=RS_HIPRI;
	if (getmsg(fd, &ctl, NULL, &flags) < 0) {
		fflush(stdout);
		if (debug_flag) {
			catfprintf(stderr, 35, "ndstat: getmsg(%s) failed\n", "DL_SAP_STATISTICS_ACK");
			perror("ndstat");
		}
		return(-1);
	}

	ack = (dl_sap_statistics_ack_t *)buf;
	if (ack->dl_primitive == DL_SAP_STATISTICS_ACK) {
		goto print_out;
	} else if (ack->dl_primitive != DL_ERROR_ACK) {
		if (debug_flag)
			catfprintf(stderr, 38, "ndstat: getstats(%s) - Incorrect primitive returned (0x%x)\n", "DL_SAP_STATISTICS_ACK", ack->dl_primitive);
		return(-1);
	}
	if (debug_flag)
		catfprintf(stdout, 40, "SAP Statistics Unavailable\n");
	return(-1);
print_out:
	sp = (struct dlpi_sapstats *)(buf+ack->dl_sapstats_offset);
	n = ack->dl_sapstats_len / sizeof(struct dlpi_sapstats);
	if (!n) {
		printf("\n");
		catfprintf(stdout, 41, "No SAPs open\n");
		return(0);
	}

	printf("\n");
	catfprintf(stdout, 42, "             FRAMES FOR EACH SERVICE ACCESS POINT (SAP)\n");
	catfprintf(stdout, 43, "        Unicast XID TEST Multicast Broadcast Error   Octets   Queue Length\n");
	catfprintf(stdout, 329,"       -------- --- ---- --------- --------- ----- ---------- ------------\n");
	while (n--) {
		printf("\n");
		if (sp->dl_saptype == FR_ETHER_II && sp->dl_sap == 0x800)
			printf("\"IP\"");
		else if (sp->dl_saptype == FR_ETHER_II && sp->dl_sap == 0x806)
			printf("\"ARP\"");
		else if (sp->dl_saptype == FR_SNAP && sp->dl_sap == 0x800)
			printf("\"IP\"");
		else if (sp->dl_saptype == FR_SNAP && sp->dl_sap == 0x806)
			printf("\"ARP\"");
		else if (sp->dl_saptype == FR_LLC && sp->dl_sap == 0xe0)
			printf("\"IPX/SPX\"");
		else if (strcmp(sp->dl_user, "pcid")==0)
			printf("\"PPP\" ISDN");
		else if (sp->dl_saptype == FR_ISDN)
			printf("ISDN");
		else if (strcmp(sp->dl_user, "dl")==0)
			printf("\"NetBEUI/Datalink\"");
		else
			printf("\"%s\"", sp->dl_user);
		printf("  ");
		switch (sp->dl_saptype) {
		case FR_ETHER_II:
				printf("Ethernet-II");
			break;
		case FR_ISDN:
			break;
		case FR_LLC:
			if (sp->dl_llcmode == LLC_OFF)
				printf("LLC");
			else
				catfprintf(stdout, 44, "LLC (below DLPI)");
			break;
		case FR_SNAP:
				printf("SNAP");
			break;
		case FR_XNS:
				printf("XNS");
			break;
		default:
				catfprintf(stdout, 45, "Unknown(0x%04x)", sp->dl_saptype);
		}
		printf("  ");
		switch (sp->dl_saptype) {
		case FR_ETHER_II:
			printf("type=0x%04x", sp->dl_sap);
			break;
		case FR_ISDN:
			catfprintf(stdout, 361, "Application Identifier=%d", sp->dl_sap);
			break;
		case FR_LLC:
			if (sp->dl_sap < 0x100)
				printf("SSAP=0x%02x", sp->dl_sap);
			else
				printf("SSAP=0x%8x", sp->dl_sap);
			break;
		case FR_SNAP:
			printf("type=0x%04x", sp->dl_sap);
			break;
		case FR_XNS:
			break;
		default:
			printf("SAP=0x%08x", sp->dl_sap);
		}
		printf(":\n");
		catfprintf(stdout, 46, "   In: ");
		catfprintf(stdout, 47, "%8u ", sp->dl_rx.dl_ucast);
		catfprintf(stdout, 48, "%3u ", sp->dl_rx.dl_ucast_xid);
		catfprintf(stdout, 49, "%4u ", sp->dl_rx.dl_ucast_test);
		catfprintf(stdout, 50, "%9u ", sp->dl_rx.dl_mcast);
		catfprintf(stdout, 51, "%9u ", sp->dl_rx.dl_bcast);
		catfprintf(stdout, 52, "%5u ", sp->dl_rx.dl_error);
		catfprintf(stdout, 53, "%10u ", sp->dl_rx.dl_octets);
		catfprintf(stdout, 54, "%12u\n", sp->dl_rx.dl_queue_len);
		catfprintf(stdout, 55, "   Out:");
		catfprintf(stdout, 47, "%8u ", sp->dl_tx.dl_ucast);
		catfprintf(stdout, 48, "%3u ", sp->dl_tx.dl_ucast_xid);
		catfprintf(stdout, 49, "%4u ", sp->dl_tx.dl_ucast_test);
		catfprintf(stdout, 50, "%9u ", sp->dl_tx.dl_mcast);
		catfprintf(stdout, 51, "%9u ", sp->dl_tx.dl_bcast);
		catfprintf(stdout, 52, "%5u ", sp->dl_tx.dl_error);
		catfprintf(stdout, 53, "%10u ", sp->dl_tx.dl_octets);
		catfprintf(stdout, 54, "%12u\n", sp->dl_tx.dl_queue_len);
		sp++;
	}
}
