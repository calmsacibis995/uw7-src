#ident "@(#)options.c	1.2"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1996 Computer Associates International, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

/*
 * This file contains the table that defines the DHCP options that can
 * appear in the config file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/dhcp.h>
#include "dhcpd.h"
#include "proto.h"

#ifndef NULL
#define NULL 0
#endif

/*
 * Values pointed to by table entries
 */

static u_short min_dg_size = MIN_DG_SIZE;
static u_short min_mtu = MIN_MTU;
static u_char min_ttl = MIN_TTL;

/*
 * Maximum lengths for options
 */

#define MAX_2BYTE_LEN	(MAX_OPT_LEN / 2)
#define MAX_4BYTE_LEN	(MAX_OPT_LEN / 4)

/*
 * Option Table
 * This must be sorted by option name.
 */

OptionDesc options[] = {
    "arp_timeout", OPT_ARP_TIMEOUT, OTYPE_UINT32, 0, 0, 0, NULL, NULL,
    "boot_file", OPT_BOOT_FILE_NAME, OTYPE_STRING, 0, 1, DHCP_FILE_LEN, NULL, NULL,
    "boot_file_dir", OPT_BOOT_DIR, OTYPE_STRING, 0, 1, DHCP_FILE_LEN, NULL, NULL,
    "boot_file_size", OPT_BOOT_SIZE, OTYPE_UINT16|OTYPE_AUTO, 0, 0, 0, NULL, NULL,
    "boot_server", OPT_BOOT_SERVER, OTYPE_ADDR, 0, 0, 0, NULL, NULL,
    "broadcast_addr", OPT_BCAST_ADDR, OTYPE_ADDR, 0, 0, 0, NULL, NULL,
    "cookie_servers", OPT_COOKIE_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "dns_servers", OPT_DOMAIN_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "domain", OPT_DOMAIN_NAME, OTYPE_STRING, 0, 1, MAX_OPT_LEN, NULL, NULL,
    "dump_file", OPT_DUMP_FILE, OTYPE_STRING, 0, 1, MAX_OPT_LEN, NULL, NULL,
    "ethernet_enc", OPT_ETHER_ENC, OTYPE_BOOLEAN, 0, 0, 0, NULL, NULL,
    "finger_servers", OPT_FINGER_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "host_name", OPT_HOST_NAME, OTYPE_STRING, 0, 1, MAX_OPT_LEN, NULL, NULL,
    "impress_servers", OPT_IMPRESS_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "interface_mtu", OPT_IF_MTU, OTYPE_UINT16, 0, 0, 0, &min_mtu, NULL,
    "ip_forward", OPT_IP_FORWARD, OTYPE_BOOLEAN, 0, 0, 0, NULL, NULL,
    "ip_ttl", OPT_IP_TTL, OTYPE_UINT8, 0, 0, 0, &min_ttl, NULL,
    "irc_servers", OPT_IRC_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "keepalive_garb", OPT_TCP_KEEP_GARB, OTYPE_BOOLEAN, 0, 0, 0, NULL, NULL,
    "keepalive_intvl", OPT_TCP_KEEP_INT, OTYPE_UINT32, 0, 0, 0, NULL, NULL,
    "log_servers", OPT_LOG_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "lpr_servers", OPT_LPR_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "mask_discovery", OPT_MASK_DISC, OTYPE_BOOLEAN, 0, 0, 0, NULL, NULL,
    "mask_supplier", OPT_MASK_SUP, OTYPE_BOOLEAN, 0, 0, 0, NULL, NULL,
    "max_reass", OPT_MAX_DG_REASS, OTYPE_UINT16, 0, 0, 0, &min_dg_size, NULL,
    "mip_home_agent", OPT_MIP_HOME, OTYPE_ADDR, 1, 0, MAX_4BYTE_LEN, NULL, NULL,
    "name_servers", OPT_NAME_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "nb_dd_servers", OPT_NB_DD, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "nb_name_servers", OPT_NB_NS, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "nb_node_type", OPT_NB_NODE, OTYPE_NB_NODE, 0, 0, 0, NULL, NULL,
    "nb_scope", OPT_NB_SCOPE, OTYPE_STRING, 0, 1, MAX_OPT_LEN, NULL, NULL,
    "nis_domain", OPT_NIS_DOMAIN, OTYPE_STRING, 0, 1, MAX_OPT_LEN, NULL, NULL,
    "nis_servers", OPT_NIS_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "nisplus_domain", OPT_NISPLUS_DOMAIN, OTYPE_STRING, 0, 1, MAX_OPT_LEN, NULL, NULL,
    "nisplus_servers", OPT_NISPLUS_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "nntp_servers", OPT_NNTP_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "non_loc_src_rt", OPT_NON_LOC_SRC_RT, OTYPE_BOOLEAN, 0, 0, 0, NULL, NULL,
    "ntp_servers", OPT_NTP_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "pmtu_plateau", OPT_PMTU_PLATEAU, OTYPE_UINT16, 1, 1, MAX_2BYTE_LEN, &min_mtu, NULL,
    "pmtu_timeout", OPT_PMTU_AGING, OTYPE_UINT32, 0, 0, 0, NULL, NULL,
    "policy_filter", OPT_POLICY_FILTER, OTYPE_ADDR, 1, 2, MAX_4BYTE_LEN, NULL, NULL,
    "pop3_servers", OPT_POP3_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "res_loc_servers", OPT_RLP_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "root_path", OPT_ROOT_PATH, OTYPE_STRING, 0, 1, MAX_OPT_LEN, NULL, NULL,
    "router_discovery", OPT_ROUTER_DISC, OTYPE_BOOLEAN, 0, 0, 0, NULL, NULL,
    "router_sol_addr", OPT_ROUTER_SOL, OTYPE_ADDR, 0, 0, 0, NULL, NULL,
    "routers", OPT_GATEWAY, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "smtp_servers", OPT_SMTP_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "stalk_servers", OPT_STALK_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "static_route", OPT_STATIC_ROUTE, OTYPE_ADDR, 1, 2, MAX_4BYTE_LEN, NULL, NULL,
    "stda_servers", OPT_STDA_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "subnets_local", OPT_SUBNETS_LOC, OTYPE_BOOLEAN, 0, 0, 0, NULL, NULL,
    "swap_servers", OPT_SWAP_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "tcp_ttl", OPT_TCP_TTL, OTYPE_UINT8, 0, 0, 0, &min_ttl, NULL,
    "tftp_dir", OPT_TFTP_DIR, OTYPE_STRING, 0, 1, MAX_OPT_LEN, NULL, NULL,
    "time_offset", OPT_TIME_OFFSET, OTYPE_INT32|OTYPE_AUTO, 0, 0, 0, NULL, NULL,
    "time_servers", OPT_TIME_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "trailer", OPT_TRAILER_ENC, OTYPE_BOOLEAN, 0, 0, 0, NULL, NULL,
    "vendor_spec", OPT_VENDOR_SPEC, OTYPE_BINARY, 0, 1, MAX_OPT_LEN, NULL, NULL,
    "www_servers", OPT_WWW_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "x_display_mgr", OPT_XDM, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
    "x_font_servers", OPT_X_FONT_SERVER, OTYPE_ADDR, 1, 1, MAX_4BYTE_LEN, NULL, NULL,
};

#define NUM_OPTIONS	(sizeof(options) / sizeof(OptionDesc))

/*
 * The following contain default values for those options that have them.
 * If a default value (4th field) is NULL, the option has a method for
 * automatically determining the value, and this value is the default
 * Note that the opt (3rd) field of the OptionSetting structure is filled
 * in by init_option_defaults().
 * Note that any multi-byte values must be converted to network order.
 * This is also done in init_option_defaults().  If a default value is
 * added, make sure code is added to convert it to network order.
 */

static OptionSetting dflt_time_offset = {
	NULL, NULL, NULL, NULL, 0, 4
};

static OptionSetting dflt_boot_size = {
	NULL, NULL, NULL, NULL, 0, 2
};

static OptionSetting dflt_bcast_addr = {
	NULL, NULL, NULL, NULL, 0, 4
};

static u_char dflt_router_disc_val = 1;
static OptionSetting dflt_router_disc = {
	NULL, NULL, NULL, &dflt_router_disc_val, 0, 1
};

static u_char dflt_ip_forward_val = 0;
static OptionSetting dflt_ip_forward = {
	NULL, NULL, NULL, &dflt_ip_forward_val, 0, 1
};

static u_char dflt_non_loc_src_rt_val = 0;
static OptionSetting dflt_non_loc_src_rt = {
	NULL, NULL, NULL, &dflt_non_loc_src_rt_val, 0, 1
};

static u_char dflt_tcp_keep_garb_val = 0;
static OptionSetting dflt_tcp_keep_garb = {
	NULL, NULL, NULL, &dflt_tcp_keep_garb_val, 0, 1
};

static u_char dflt_ip_ttl_val = 64;
static OptionSetting dflt_ip_ttl = {
	NULL, NULL, NULL, &dflt_ip_ttl_val, 0, 1
};

static u_char dflt_ether_enc_val = 0;
static OptionSetting dflt_ether_enc = {
	NULL, NULL, NULL, &dflt_ether_enc_val, 0, 1
};

static u_long dflt_tcp_keep_int_val = 0;
static OptionSetting dflt_tcp_keep_int = {
	NULL, NULL, NULL, &dflt_tcp_keep_int_val, 0, 4
};

/*
 * option_defaults is a table of default option values indexed by the option
 * code.
 */

OptionSetting *option_defaults[MAX_OPT_CODE + 1] = {
	NULL,			/* OPT_PAD */
	NULL,			/* OPT_SUBNET_MASK */
	&dflt_time_offset,	/* OPT_TIME_OFFSET */
	NULL,			/* OPT_GATEWAY */
	NULL,			/* OPT_TIME_SERVER */
	NULL,			/* OPT_NAME_SERVER */
	NULL,			/* OPT_DOMAIN_SERVER */
	NULL,			/* OPT_LOG_SERVER */
	NULL,			/* OPT_COOKIE_SERVER */
	NULL,			/* OPT_LPR_SERVER */
	NULL,			/* OPT_IMPRESS_SERVER */
	NULL,			/* OPT_RLP_SERVER */
	NULL,			/* OPT_HOST_NAME */
	&dflt_boot_size,	/* OPT_BOOT_SIZE */
	NULL,			/* OPT_DUMP_FILE */
	NULL,			/* OPT_DOMAIN_NAME */
	NULL,			/* OPT_SWAP_SERVER */
	NULL,			/* OPT_ROOT_PATH */
	NULL,			/* OPT_EXTEN_FILE */
	&dflt_ip_forward,	/* OPT_IP_FORWARD */
	&dflt_non_loc_src_rt,	/* OPT_NON_LOC_SRC_RT */
	NULL,			/* OPT_POLICY_FILTER */
	NULL,			/* OPT_MAX_DG_REASS */
	&dflt_ip_ttl,		/* OPT_IP_TTL */
	NULL,			/* OPT_PMTU_AGING */
	NULL,			/* OPT_PMTU_PLATEAU */
	NULL,			/* OPT_IF_MTU */
	NULL,			/* OPT_SUBNETS_LOC */
	&dflt_bcast_addr,	/* OPT_BCAST_ADDR */
	NULL,			/* OPT_MASK_DISC */
	NULL,			/* OPT_MASK_SUP */
	&dflt_router_disc,	/* OPT_ROUTER_DISC */
	NULL,			/* OPT_ROUTER_SOL */
	NULL,			/* OPT_STATIC_ROUTE */
	NULL,			/* OPT_TRAILER_ENC */
	NULL,			/* OPT_ARP_TIMEOUT */
	&dflt_ether_enc,	/* OPT_ETHER_ENC */
	NULL,			/* OPT_TCP_TTL */
	&dflt_tcp_keep_int,	/* OPT_TCP_KEEP_INT */
	&dflt_tcp_keep_garb,	/* OPT_TCP_KEEP_GARB */
	NULL,			/* OPT_NIS_DOMAIN */
	NULL,			/* OPT_NIS_SERVER */
	NULL,			/* OPT_NTP_SERVER */
	NULL,			/* OPT_VENDOR_SPEC */
	NULL,			/* OPT_NB_NS */
	NULL,			/* OPT_NB_DD */
	NULL,			/* OPT_NB_NODE */
	NULL,			/* OPT_NB_SCOPE */
	NULL,			/* OPT_X_FONT_SERVER */
	NULL,			/* OPT_XDM */
	NULL,			/* OPT_REQ_ADDR */
	NULL,			/* OPT_LEASE_TIME */
	NULL,			/* OPT_OVERLOAD */
	NULL,			/* OPT_MSG_TYPE */
	NULL,			/* OPT_SERVER_ID */
	NULL,			/* OPT_PARAM_REQ */
	NULL,			/* OPT_MESSAGE */
	NULL,			/* OPT_MAX_DHCP_MSG_SIZE */
	NULL,			/* OPT_T1 */
	NULL,			/* OPT_T2 */
	NULL,			/* OPT_VENDOR_CLASS */
	NULL,			/* OPT_CLIENT_ID */
	NULL,			/* 62 */
	NULL,			/* 63 */
	NULL,			/* OPT_NISPLUS_DOMAIN */
	NULL,			/* OPT_NISPLUS_SERVER */
	NULL,			/* OPT_TFTP_SERVER */
	NULL,			/* OPT_BOOT_FILE */
	NULL,			/* OPT_MIP_HOME */
	NULL,			/* OPT_SMTP_SERVER */
	NULL,			/* OPT_POP3_SERVER */
	NULL,			/* OPT_NNTP_SERVER */
	NULL,			/* OPT_WWW_SERVER */
	NULL,			/* OPT_FINGER_SERVER */
	NULL,			/* OPT_IRC_SERVER */
	NULL,			/* OPT_STALK_SERVER */
	NULL,			/* OPT_STDA_SERVER */
	NULL,			/* OPT_USER_CLASS */
};

static int
option_compare(const void *v1, const void *v2)
{
	return strcmp(((OptionDesc *) v1)->name, ((OptionDesc *) v2)->name);
}

OptionDesc *
lookup_option(char *name)
{
	OptionDesc key;

	key.name = name;
	return (OptionDesc *) bsearch(&key, options, NUM_OPTIONS,
		sizeof(OptionDesc), option_compare);
}

/*
 * init_option_defaults -- set opt pointers in elements of option_defaults
 * table
 * This must be called before the option_defaults table is used.
 */

void
init_option_defaults(void)
{
	int i, o;
	OptionSetting *osp;

	for (i = 0; i < NUM_OPTIONS; i++) {
		if ((o = options[i].code) > MAX_OPT_CODE) {
			continue;
		}
		if (osp = option_defaults[o]) {
			osp->opt = &options[i];
		}
	}

	/*
	 * Any multi-byte default values must be put in network order.
	 */
	
	dflt_tcp_keep_int_val = htonl(dflt_tcp_keep_int_val);
}
