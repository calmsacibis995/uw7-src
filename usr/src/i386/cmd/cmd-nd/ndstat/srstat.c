#ident "@(#)srstat.c	11.2"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/scodlpi.h>
#include "ndstat.h"

extern int debug_flag;
union DL_primitives primbuf;
struct strbuf primptr = {
	0,	0,	(char *)&primbuf
};

int
ring(ushort r)
{
	ushort rs = (r >> 8) | (r << 8);
	return(rs >> 4);
}

int
bridge(ushort r)
{
	ushort rs = (r >> 8) | (r << 8);
	return (rs & 0x0f);
}

int
do_srtable_req(int fd)
{
	dl_srtable_req_t *req;
	dl_srtable_ack_t *ack;
	int flags = RS_HIPRI;
	int len;
	time_t	lbolt;
	
	req = (dl_srtable_req_t *)primptr.buf;
	req->dl_primitive = DL_SRTABLE_REQ;
	primptr.maxlen = sizeof(dl_srtable_req_t);
	primptr.len = sizeof(dl_srtable_req_t);;

	if (putmsg(fd, &primptr, NULL, RS_HIPRI) < 0) {
		if (debug_flag) {
			catfprintf(stderr, 34, "ndstat: putmsg(%s) failed\n",
				"DL_SRTABLE_REQ");
			perror("ndstat");
		}
		return(-1);
	}

	primptr.maxlen = sizeof(primbuf);
	primptr.len = 0;
	if (getmsg(fd, &primptr, NULL, &flags) < 0) {
		if (debug_flag) {
			catfprintf(stderr, 35, "ndstat: getmsg(%s) failed\n",
				"DL_SRTABLE_ACK");
			perror("ndstat");
		}
		return(-1);
	}
	if (flags != RS_HIPRI) {
		if (debug_flag)
			catfprintf(stderr, 36,
				"ndstat: getmsg(%s) OK, but flags is wrong (=0x%x)\n",
				"DL_SRTABLE_ACK", flags);
		return(-1);
	}
	ack = (dl_srtable_ack_t *)primptr.buf;

	if (ack->dl_primitive == DL_ERROR_ACK) {
		if (debug_flag)
			catfprintf(stdout, 56,
				"Source routing table information unavailable\n");
		return(-1);
	} else if (ack->dl_primitive != DL_SRTABLE_ACK) {
		if (debug_flag)
			catfprintf(stdout,38,
			"ndstat: getstats(%s) - Incorrect primitive returned (0x%x)\n",
			"DL_SRTABLE_ACK", ack->dl_primitive);
		return(-1);
	}
	len = ack->dl_srtable_len;
	catfprintf(stdout, 57, "\n\t\tSOURCE ROUTING TABLE\n");
	catfprintf(stdout, 59, "%d of %d Source Routes in use\n",
		ack->dl_routes_in_use, ack->dl_route_table_sz);
	if (ack->dl_ARE_disa == 0)
		catfprintf(stdout, 308, "Sending ARE frames is enabled (route optimization is on).\n");
	else
		catfprintf(stdout, 309, "Sending ARE frames is disabled (route optimization is off).\n");

	lbolt = ack->dl_time_now;
	if (!len) {
		catfprintf(stdout, 64,
			"No source routing table entries present\n");
		return(0);
	}

       	printf("\n");
	catfprintf(stdout, 327, "MAC Address       Maximum PDU Timeout (ticks) Route Length\n");
	catfprintf(stdout, 328, "----------------- ----------- --------------- ------------\n");
	primptr.maxlen = 10 * sizeof (struct sr_table_entry);
	primptr.buf = (char *) malloc(primptr.maxlen);
	primptr.len = 0;
	while (len > 0) {
		struct sr_table_entry *srp;

		flags=0;
		if (getmsg(fd, NULL, &primptr, &flags) < 0) {
			catfprintf(stderr, 35, "ndstat: getmsg(%s) failed",
				"DL_SRTABLE_ACK-DATA");
			perror("ndstat");
			return(-1);
		}
		srp = (struct sr_table_entry *)primptr.buf;
		for (;primptr.len; primptr.len -= sizeof(struct sr_table_entry),
				   srp++,
		  		   len -= sizeof(struct sr_table_entry))
		{
			int i;

			printf("%02x:%02x:%02x:%02x:%02x:%02x ", 
				srp->sr_remote_mac[0],srp->sr_remote_mac[1],
				srp->sr_remote_mac[2],srp->sr_remote_mac[3],
				srp->sr_remote_mac[4],srp->sr_remote_mac[5]);
			/* do not expose the state of the route */
			catfprintf(stdout, 310,"%11d ", srp->sr_max_pdu);
			catfprintf(stdout, 66, "%15d ",
				srp->sr_timeout>lbolt?srp->sr_timeout-lbolt:0);
			catfprintf(stdout, 67, "%12d\n",
				srp->sr_route_len);
			if (srp->sr_route_len) {
				catfprintf(stdout, 68, "\tRing(s):   %03d",
					ring(srp->sr_route[0]));
				for (i=1; i < (int)srp->sr_route_len; i++) {
					catfprintf(stdout, 69, "->%03d",
						ring(srp->sr_route[i]));
				}
				catfprintf(stdout, 70,
					"\n\tvia Bridge(s): %-2d",
					bridge(srp->sr_route[0]));
				for (i=1; i < (int)srp->sr_route_len-1; i++) {
					catfprintf(stdout, 71, "   %-2d",
						bridge(srp->sr_route[i]));
				}
			} else {
				catfprintf(stdout, 72, "\t[LOCAL RING]");
			}
			printf("\n\n");
		}
	}
	primptr.buf = (char *)&primbuf;
	return(0);
}

int
clear_sr(fd)
{
	dl_clr_sr_req_t *req;
	dl_ok_ack_t *ack;
	int flags = RS_HIPRI;
	unchar *cp;
	int addr = 0;		/* there is provision in the structure to
				 * clear the route table entry of a particular
				 * MAC address from the route table. But for
				 * now, the implementation is to clear the
				 * entire route table.
				 */

	if (type == ODIDLPI) {
		catfprintf(stderr, 355, "ndstat: this operation isn't applicable to DLPI or ODI drivers\n");
		return(-1);
	}

	req = (dl_clr_sr_req_t *)primptr.buf;
	memset(req, 0, sizeof(dl_clr_sr_req_t));
	req->dl_primitive = DL_CLR_SR_REQ;

	req->dl_addr_len = MAC_ADDR_SZ;
	req->dl_addr_offset = DL_CLR_SR_REQ_SIZE;
	cp = (unchar *)req;
	cp += DL_CLR_SR_REQ_SIZE;
	memcpy(cp, (caddr_t)addr, MAC_ADDR_SZ);
	primptr.len = DL_CLR_SR_REQ_SIZE + MAC_ADDR_SZ;
	primptr.maxlen = DL_CLR_SR_REQ_SIZE + MAC_ADDR_SZ;

	if (putmsg(fd, &primptr, (struct strbuf *)NULL, RS_HIPRI) < 0) {
		if (debug_flag) {
			catfprintf(stderr, 34, "ndstat: putmsg(%s) failed\n",
				"DL_CLR_SR_REQ");
			perror("ndstat");
		}
		return(-1);
	}

	/* get the ack */
	primptr.maxlen = primptr.len = sizeof(primbuf);
	if (getmsg(fd, &primptr, (struct strbuf *)0, &flags) < 0) {
		if (debug_flag) {
			catfprintf(stderr, 35, "ndstat: getmsg(%s) failed\n",
				"DL_CLR_SR_REQ");
			perror("ndstat");
		}
		return(-1);
	}

	ack = (dl_ok_ack_t *)primptr.buf;
	if (ack->dl_primitive == DL_ERROR_ACK) {
		dl_error_ack_t *err;

		err = (dl_error_ack_t *)ack;
		if (debug_flag)
			catfprintf(stdout, 311,
				"Request to clear route table failed, errno %d\n",
				err->dl_unix_errno);
		return(-1);
	} else if (ack->dl_primitive != DL_OK_ACK) {
		if (debug_flag)
			catfprintf(stdout, 38,
			"ndstat: getstats(%s) - Incorrect primitive returned (0x%x)\n",
			"DL_OK_ACK", ack->dl_primitive);
		return(-1);
	}
	return(1);
}
