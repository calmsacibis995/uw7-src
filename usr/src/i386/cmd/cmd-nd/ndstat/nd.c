#ident "@(#)nd.c	26.1"
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
#include <sys/stream.h>
#include <stropts.h>
#include <sys/scodlpi.h>
#define LLI31_UNSUPPORTED   /* for obsolete MACIOC_GETIFSTAT */
#include <sys/mdi.h>
#include <sys/dlpimod.h>
#include <sys/dlpi_ether.h>
#include <net/if.h>   /* struct ifstats */
#include "ndstat.h"
#include "string.h"

extern char *optarg;
extern int optind, opterr;
extern char *hw_addr();
int mdi_mac_media_type = 0;
int set_id = 2;			/* for ../intl/intl.c */
extern void display_statistics(char *dev);

void
usage(char *name)
{
	catfprintf(stderr, 1, "Usage: %s [-clsCDLMRS] [dev...]\n", name);
	catfprintf(stderr, 6, "\tc - clear statistics\n");
	catfprintf(stderr, 3, "\tl - long\n");
	catfprintf(stderr, 2, "\ts - short\n");
	catfprintf(stderr, 300, "\tC - clear route table\n");
	catfprintf(stderr, 351, "\tD - only show DLPI or ODI drivers\n");
	catfprintf(stderr, 352, "\tL - only show LLI drivers\n");
	catfprintf(stderr, 350, "\tM - only show MDI drivers\n");
	catfprintf(stderr, 5, "\tR - Source Routing statistics\n");
	catfprintf(stderr, 4, "\tS - SAP statistics\n");
}

int debug_flag = 0;
int mdionly = 0;
int odidlpionly = 0;
int llionly = 0;
int list_info = LIST_NORMAL;
static char set_function = 0;
static dce_compat = 0;

#define CLR_STAT	1
#define CLR_SR		2

int
main(int argc, char **argv)
{
	char c;
	char buf[10];

	if (strlen(argv[0]) >= strlen("ndstat")) {
		strcpy(buf, &argv[0][strlen(argv[0]) - strlen("ndstat")]);
		if (!strcmp(buf, "ndstat")) {
			dce_compat = 1;
		}
	}
	while ((c = getopt(argc, argv, "SRslcCdMDL")) != -1) {
		switch (c) {
		case 's':
			list_info = LIST_SHORT;
			break;
		case 'l':
			list_info = LIST_LONG;
			break;
		case 'S':
			list_info = LIST_SAP;
			break;
		case 'R':
			list_info = LIST_SR;
			break;
		case 'c':
			if (getuid() != 0) {
				catfprintf(stdout, 331, "Must be root for -c option\n");
				exit(1);
			}
			set_function = CLR_STAT;
			break;
		case 'C':
			if (getuid() != 0) {
				catfprintf(stdout, 332, "Must be root for -C option\n");
				exit(1);
			}
			set_function = CLR_SR;
			break;
		case 'd':
			debug_flag = 1;
			break;
		case 'M':
			mdionly++;
			break;
		case 'D':
			odidlpionly++;
			break;
		case 'L':
			llionly++;
			break;
		default:
			usage(argv[0]);
			exit(1);
		}
	}

	/* if none set, it's 0.  otherwise only allow one parameter */
	if (mdionly + odidlpionly + llionly > 1) {
		catfprintf(stdout,353,"only one of -M, -D, and -L allowed\n");
	}
	
	if (optind < argc) {
		while (optind < argc) {
			display_statistics(argv[optind]);
			optind++;
		}
		exit(0);
	}
	search_for_drivers();
	exit(0);
}

/*
 * Returns non-zero if dev_uniq has not been called with this argument before,
 * and zero if it has.  It is used to stop the same driver information from
 * being displayed more than once.
 */
int
dev_uniq(char *s)
{
	static char dev_t[64][12];
	static int dev_t_cnt=0;

	int i,l;

	l = strlen(s);
	if (l >= 12)
		return(1);

	for (i=0; i<dev_t_cnt; i++)
		if (strncmp(dev_t[i], s, l)==0)
			return(0);
	strncpy(dev_t[dev_t_cnt], s, l);
	dev_t_cnt++;
	return(1);
}

char odidlpibuffer[sizeof(DL_mib_t) + 200];  /* bd_idstring < 200 chars */
DL_mib_t *mib=(DL_mib_t *)odidlpibuffer;

/* side note:  rtpm and snmpd also issue the DLIOCGMIB ioctl */
void
set_driver_type(int fd)
{
	int ret,flags;
	struct strioctl strioc;
	struct strbuf ctl, data;
	dl_get_statistics_req_t *req;
	dl_get_statistics_ack_t *ack;
	char stat_buf[DL_GET_STATISTICS_ACK_SIZE  + sizeof(struct dlpi_stats)];
	struct dlpi_stats *sp;
	struct ifstats ifs;
	char hwdep_buf[4096];

	strioc.ic_len = sizeof(odidlpibuffer);
	strioc.ic_timout = 0;
	strioc.ic_dp = (char *)&odidlpibuffer[0];
	strioc.ic_cmd = DLIOCGMIB;
	
	ret=ioctl(fd, I_STR, &strioc);
	if (ret < 0) {
		/* assume MDI or LLI */
		bzero(stat_buf, sizeof(stat_buf));
		ctl.buf = stat_buf;
		ctl.len = sizeof(dl_get_statistics_req_t);

		req = (dl_get_statistics_req_t *) stat_buf;
		req->dl_primitive = DL_GET_STATISTICS_REQ;

		if (putmsg(fd, &ctl, NULL, RS_HIPRI) < 0) {
			catfprintf(stderr, 34, "ndstat: putmsg(%s) failed\n", 
					"DL_GET_STATISTICS_REQ");
			perror("ndstat");
			close(fd);
			exit(1);
		}
		bzero(stat_buf, sizeof(stat_buf));
		bzero(hwdep_buf, sizeof(hwdep_buf));

		ctl.buf = stat_buf;
		ctl.maxlen = sizeof(stat_buf);
		ctl.len = 0;

		data.buf = hwdep_buf;
		data.maxlen = sizeof(hwdep_buf);
		data.len = 0;

		flags=RS_HIPRI;

		if (getmsg(fd, &ctl, &data, &flags) < 0) {
				catfprintf(stderr, 35, "ndstat: getmsg(%s) failed\n", 
				"DL_GET_STATISTICS_ACK");
			perror("ndstat");
			close(fd);
			exit(1);
		}
		ack = (dl_get_statistics_ack_t *) stat_buf;
		if (ack->dl_primitive == DL_GET_STATISTICS_ACK) {
			sp = (struct dlpi_stats *)&stat_buf[DL_GET_STATISTICS_ACK_SIZE];
			mdi_mac_media_type = sp->mac_media_type;
			type = MDI;
			return;
		}
		/* 
		 * we should have LLI at this point.
		 * issue the ancient MACIOC_GETIFSTAT
		 * ioctl to confirm this is true.  only LLI drivers
		 * will respond as that's how the old llistat command used to
		 * get its information from the driver.
		 * FYI, this will also be NAKed for MDI drivers by the 
		 * dlpi module.
		 */
		strioc.ic_cmd = MACIOC_GETIFSTAT;
		strioc.ic_timout = 0;
		strioc.ic_dp = (char *)&ifs;
		strioc.ic_len = sizeof(struct ifstats);

		if (ioctl(fd, I_STR, &strioc) < 0) {
			catfprintf(stderr, 354, "ndstat: unknown driver type\n");
			return;
		}

		/* we must have an LLI driver to make it this far */
		type = LLI;
		return;
	}
	/* ODI and DLPI drivers understand DLIOCGMIB */
	type=ODIDLPI;
	return;
}

char *driver_type_str;

void
display_statistics(char *dev)
{
	int fd, i;
	static displayed_headers=0;
	struct dlpi_stats *sp;
	struct per_card_info cp;
	char *in_use_addr, *factory_addr;
	char *tmp;

	if (!dev_uniq(dev))
		return;

	if ((fd = open(dev, O_RDWR)) <0 ) {
		perror(dev);
		return;
	}

	/* set global "type" variable */
	set_driver_type(fd);

	switch (type) {
		case MDI: 
			driver_type_str="MDI"; 
			break;
		case ODIDLPI: 
			driver_type_str="DLPI/ODI";
			break;
		case LLI:
			driver_type_str="LLI";
			break;
		default:
			driver_type_str="UNKNOWN";
			break;
	}

	if (mdionly > 0 && type != MDI) return;
	if (odidlpionly > 0 && type != ODIDLPI) return;
	if (llionly > 0 && type != LLI) return;

	switch (set_function) {
	case CLR_STAT:
		if (clear_stats(fd) == -1)
			catfprintf(stderr, 341, "Cannot access device %s - statistics not cleared\n", dev);	
		goto end;
	case CLR_SR:
		if (clear_sr(fd) == -1)
			catfprintf(stderr, 342, "Request to clear route table failed for device %s\n", dev);
		goto end;
	}

	if (list_info == LIST_SHORT) {
		printf("%s\n", dev);
		goto end;
	}

	if (displayed_headers)
		printf("\n\n");
	else
		displayed_headers = 1;

	if ((mdi_mac_media_type == MAC_ISDN_BRI)
			|| (mdi_mac_media_type == MAC_ISDN_PRI)) {
		/* display ISDN header */
		catfprintf(stdout, 356, "ISDN Device\n");
		catfprintf(stdout, 357, "-----------\n");
		catfprintf(stdout, 358, "%-17s\n", dev);
		fflush(stdout);

	} else { /* not ISDN */

		if (dce_compat && list_info == LIST_NORMAL) {
   			printf("Device            SNPA/MAC address      Factory Address\n");
		} else {
			catfprintf(stdout, 7,   "Device            MAC address in use    Factory MAC Address\n");
			catfprintf(stdout, 312, "------            ------------------    -------------------\n");
		}
		catfprintf(stdout, 8, "%-17s", dev);
		fflush(stdout);
		in_use_addr = hw_addr(fd, 0);
		if (in_use_addr != NULL)
			factory_addr = hw_addr(fd, 1);
		if ((in_use_addr != NULL) && (factory_addr != NULL))
			catfprintf(stdout, 333, " %-17s     %-17s\n", in_use_addr, factory_addr);
		else {
			printf("\n");
			catfprintf(stderr, 338, "Cannot get statistics for device %s\n", dev);
			goto end;
		}
		if (mca_table(fd) == -1) {
				catfprintf(stderr, 338, "Cannot get statistics for device %s\n", dev);
			goto end;
		}
	} /* end not ISDN */
	sp = get_hw_independent_stats(fd);
	if (!sp) {
		catfprintf(stderr, 338, "Cannot get statistics for device %s\n", dev);
		goto end;
	}

	catfprintf(stdout, 9, "\n\t                     FRAMES\n");
	catfprintf(stdout, 10,  "      Unicast  Multicast Broadcast  Error    Octets   Queue Length\n");
        catfprintf(stdout, 313, "    ---------- --------- --------- ------ ----------- ------------\n");
	catfprintf(stdout, 11, "In: ");
	catfprintf(stdout, 12, "%10u ", sp->mac_rx.mac_ucast);
	catfprintf(stdout, 13, "%9u ", sp->mac_rx.mac_mcast);
	catfprintf(stdout, 14, "%9u ", sp->mac_rx.mac_bcast);
	catfprintf(stdout, 15, "%6u ", sp->mac_rx.mac_error);
	catfprintf(stdout, 16, "%11u ", sp->mac_rx.mac_octets);
	catfprintf(stdout, 17, "%12u\n", sp->mac_rx.mac_queue_len);
	catfprintf(stdout, 18, "Out:");
	catfprintf(stdout, 12, "%10u ", sp->mac_tx.mac_ucast);
	catfprintf(stdout, 13, "%9u ", sp->mac_tx.mac_mcast);
	catfprintf(stdout, 14, "%9u ", sp->mac_tx.mac_bcast);
	catfprintf(stdout, 15, "%6u ", sp->mac_tx.mac_error);
	catfprintf(stdout, 16, "%11u ", sp->mac_tx.mac_octets);
	catfprintf(stdout, 17, "%12u\n", sp->mac_tx.mac_queue_len);

	/* worthy enough to include in normal ndstat output */
	if (sp->mac_suspended) {
		catfprintf(stdout, 440, 
			"driver is currently suspended(total of %u data frames dropped on output)\n",
			sp->mac_suspenddrops);
	}

	if (list_info == LIST_NORMAL)
		goto end;

	printf("\n");
	if (type == MDI) {
		catfprintf(stdout, 19, "DLPI Module Info: %d SAPs open, %d SAPs maximum",
			sp->dl_nsaps,sp->dl_maxsaps);
		if (sp->dl_maxroutes)
			catfprintf(stdout, 20, ", %d Source routes in use, %d Source routes maximum",
			sp->dl_nroutes, sp->dl_maxroutes);
		printf("\n");
		if (sp->dl_rxunbound)
			catfprintf(stdout, 21, "                 %d frames received destined for an unbound SAP\n", sp->dl_rxunbound);
		printf("\n");
	}
	catfprintf(stdout, 22, "MAC Driver Info: Media_type: ");
	switch (sp->mac_media_type) {
	case DL_CSMACD:	catfprintf(stdout, 23, "Ethernet"); break;
	case DL_TPR:	catfprintf(stdout, 24, "Token Ring"); break;
	case DL_FDDI:	catfprintf(stdout, 25, "FDDI"); break;
	case MAC_ISDN_BRI: catfprintf(stdout, 359, "ISDN BRI"); break;
	case MAC_ISDN_PRI: catfprintf(stdout, 360, "ISDN PRI"); break;

	default: catfprintf(stdout, 26, "Unknown_type(0x%x)", sp->mac_media_type);
	}
	printf("\n");
	catfprintf(stdout, 27, "                 Min_SDU: %d, Max_SDU: %d, Address length: %d\n", sp->mac_min_sdu, sp->mac_max_sdu, sp->mac_addr_length);
	catfprintf(stdout, 28, "                 Interface speed: ");
	/* this can be incorrect for ODI drivers as it is set from the MLID
	 * parameter MLIDCFG_LineSpeed which is an unsigned 16 bit value.
	 * See lsl/lslstr.c.
	 * speed will generally not be set for DLPI drivers.
	 */
	if ((sp->mac_ifspeed % 1000000) == 0)
		catfprintf(stdout, 29, "%d Mbits/sec\n", sp->mac_ifspeed/1000000);
	else
		catfprintf(stdout, 30, "%d bits/sec\n", sp->mac_ifspeed);
	if (type == MDI) {
		if (get_per_card_info(dev, &cp) != -1) {
			printf("\n");
			catfprintf(stdout, 442, "DLPI Restarts Info: Last mblk_t: 0x%x\n", cp.txmon.last_mblk);
			catfprintf(stdout, 335, "                    Last send time: %d\n", cp.txmon.last_send_time);
			catfprintf(stdout, 336, "                    Restart in progress: %d\n", cp.txmon.restart_in_progress);
			catfprintf(stdout, 337, "                    Number of restarts: %d\n", cp.txmon.nrestarts);
			catfprintf(stdout, 442, "                    Largest qsize found: %d\n", cp.txmon.largest_qsize_found);
		} else {
			if (debug_flag)
				fprintf(stderr, "Cannot get DLPI Restarts statistics for device %s\n", dev);
		}
	}

	printf("\n");
	catfprintf(stdout, 31, "Interface Version: %s %x\n",
			driver_type_str,
			sp->mac_driver_version);
	if (list_info == LIST_LONG) {
		display_hw_dep_stats();
	}
	if (list_info == LIST_SAP) {
		if (display_sapstats(fd) == -1) {
			catfprintf(stderr, 339, "Cannot get SAP statistics from device %s\n", dev);
			goto end;
		}
	}
	if (list_info == LIST_SR) {
		switch (sp->mac_media_type) {
			case DL_CSMACD:
				break;
			case DL_TPR:
			case DL_FDDI:
				if (do_srtable_req(fd) == -1) {
					catfprintf(stderr, 340, "Cannot get source routing table statistics from device %s\n", dev);
					goto end;
				}
				break;
		}
	}
end:
	close(fd);
}
