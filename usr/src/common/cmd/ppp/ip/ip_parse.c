#ident	"@(#)ip_parse.c	1.3"

#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <aas/aas.h>

#include "ppp_type.h"
#include "ppp_cfg.h"
#include "ppptalk.h"
#include "ip_cfg.h"

STATIC struct str2val_s addr_opt[] = {
	"force",	(void *)IPADDR_FORCE,
	"prefer",	(void *)IPADDR_PREFER,
	"any",		(void *)IPADDR_ANY,
	"pool",		(void *)IPADDR_POOL,
	NULL, NULL,
};

STATIC struct str2val_s dns_opt[] = {
	"addr",		(void *)DNS_ADDR,
	"dhcp",		(void *)DNS_DHCP,
	"local",	(void *)DNS_LOCAL,
	"none",		(void *)DNS_NONE,
};

/*
 * This module provides all the IPCP specific functions required to
 * parse (and list) the users configuration
 */
/*
 * IP specific protocol options
 */
STATIC struct psm_opttab ip_opts[] = {
	"protocol",	STRING, MANDATORY, NULL, NULL,

	/* First we must include the standard FSM options */
	FSM_OPT_NAMES,

	/* Then any protocol specific options */
	"localaddr",	STRING, OPTIONAL, NULL, NULL,
	"peeraddr",	STRING, OPTIONAL, NULL, NULL,
	"localopt",	NUMERIC_F, OPTIONAL,
		(void *)def_lookval, (void *)addr_opt,
	"peeropt", 	NUMERIC_F, OPTIONAL,
		(void *)def_lookval, (void *)addr_opt,
	"vjcompress",	BOOLEAN, OPTIONAL, NULL, NULL,
	"vjslotcomp",	BOOLEAN, OPTIONAL, NULL, NULL,
	"vjmaxslot", 	NUMERIC_R, OPTIONAL, (void *)3, (void *)255,
	"exec",		STRING, OPTIONAL, NULL, NULL,
	"bringup",	STRING, OPTIONAL, NULL, NULL,
	"keepup",	STRING, OPTIONAL, NULL, NULL,
	"passin",	STRING, OPTIONAL, NULL, NULL,
	"passout",	STRING, OPTIONAL, NULL, NULL,
	"proxyarp",	BOOLEAN, OPTIONAL, NULL, NULL,
	"advdns",	STRING, OPTIONAL, NULL, NULL,
	"advdns2",	STRING, OPTIONAL, NULL, NULL,
	"advdnsopt",	NUMERIC_F, OPTIONAL,
		(void *)def_lookval, (void *)dns_opt,
	"getdns", 	BOOLEAN, OPTIONAL, NULL, NULL,
	"defaultroute",	BOOLEAN, OPTIONAL, NULL, NULL,
	"debug",	NUMERIC_R, OPTIONAL, (void *)0, (void *)5,
	"netmask",	STRING, OPTIONAL, NULL, NULL,
	NULL, 0, 0, NULL, NULL,
};

STATIC void
ip_set_defaults(struct cfg_ipcp *p)
{
	p->ip_ch.ch_len = p->ip_var - (char *)p;
	p->ip_ch.ch_stroff = p->ip_var - (char *)p;

	def_insert_str(&(p->ip_ch), &(p->ip_local_addr), IP_DEFAULT_LOCALADDR);
	def_insert_str(&(p->ip_ch), &(p->ip_peer_addr), IP_DEFAULT_PEERADDR);
	p->ip_local_addropt = IP_DEFAULT_LOCALOPT;
	p->ip_peer_addropt = IP_DEFAULT_PEEROPT;
	p->ip_vjcomp = IP_DEFAULT_VJCOMPRESS;
	p->ip_vjslotcomp = IP_DEFAULT_SLOT_COMP;
	p->ip_vjslotmax = IP_DEFAULT_SLOT_MAX;

	def_insert_str(&(p->ip_ch), &(p->ip_exec), IP_DEFAULT_EXEC);

	def_insert_str(&(p->ip_ch), &(p->ip_keepup), IP_DEFAULT_KEEPUP);
	def_insert_str(&(p->ip_ch), &(p->ip_bringup), IP_DEFAULT_BRINGUP);
	def_insert_str(&(p->ip_ch), &(p->ip_passin), IP_DEFAULT_PASSIN);
	def_insert_str(&(p->ip_ch), &(p->ip_passout), IP_DEFAULT_PASSOUT);

	p->ip_proxyarp = IP_DEFAULT_PROXYARP;

	def_insert_str(&(p->ip_ch), &(p->ip_advdns), IP_DEFAULT_ADVDNS);
	def_insert_str(&(p->ip_ch), &(p->ip_advdns2), IP_DEFAULT_ADVDNS2);
	p->ip_advdnsopt = IP_DEFAULT_ADVDNSOPT;

	p->ip_getdns = IP_DEFAULT_GETDNS;
	p->ip_defaultroute = IP_DEFAULT_DEFAULTROUTE;
	p->ip_debug = IP_DEFAULT_DEBUG;

	def_insert_str(&(p->ip_ch), &(p->ip_netmask), IP_DEFAULT_NETMASK);

	FSM_OPT_DEFAULTS(&p->ip_fsm);
}

STATIC void
ip_list(FILE *fp, struct cfg_ipcp *p)
{
	int i;

	fprintf(fp, "	protocol = ip\n");
	fprintf(fp, "	localaddr = %s\n", 
		def_get_str(&(p->ip_ch), p->ip_local_addr));
	fprintf(fp, "	peeraddr = %s\n", 
		def_get_str(&(p->ip_ch), p->ip_peer_addr));
	fprintf(fp, "	localopt = %s\n", addr_opt[p->ip_local_addropt - 1].tv_str);
	fprintf(fp, "	peeropt = %s\n", addr_opt[p->ip_peer_addropt - 1].tv_str);
	fprintf(fp, "	netmask = %s\n",
 		def_get_str(&(p->ip_ch), p->ip_netmask));
	fprintf(fp, "	vjcompress = %s\n", def_get_bool(p->ip_vjcomp));
	fprintf(fp, "	vjslotcomp = %s\n", def_get_bool(p->ip_vjslotcomp));
	fprintf(fp, "	vjmaxslot = %d\n", p->ip_vjslotmax);
	fprintf(fp, "	exec = %s\n", def_get_str(&(p->ip_ch), p->ip_exec));
	fprintf(fp, "	defaultroute = %s\n",
		def_get_bool(p->ip_defaultroute));
	fprintf(fp, "	bringup = %s\n",
		def_get_str(&(p->ip_ch), p->ip_bringup));
	fprintf(fp, "	passin = %s\n",
		def_get_str(&(p->ip_ch), p->ip_passin));
	fprintf(fp, "	passout = %s\n",
		def_get_str(&(p->ip_ch), p->ip_passout));
	fprintf(fp, "	keepup = %s\n",
		def_get_str(&(p->ip_ch), p->ip_keepup));
	fprintf(fp, "	proxyarp = %s\n", def_get_bool(p->ip_proxyarp));
	for(i = 0; dns_opt[i].tv_str && 
		    (uint_t)dns_opt[i].tv_val != p->ip_advdnsopt; i++);
	fprintf(fp, "	advdnsopt = %s\n", dns_opt[i].tv_str);

	fprintf(fp, "	advdns = %s\n",
		def_get_str(&(p->ip_ch), p->ip_advdns));
	fprintf(fp, "	advdns2 = %s\n",
		def_get_str(&(p->ip_ch), p->ip_advdns2));

	fprintf(fp, "	getdns = %s\n", def_get_bool(p->ip_getdns));
	fprintf(fp, "	debug = %d\n", p->ip_debug);
	FSM_OPT_DISPLAY(fp, &p->ip_fsm);
}

char *ipcp_reasons[] = {
	"No local addr negotiated/configured",
	"No peer addr negotiated/configured",
	"Local and Peer addrs same",
	"Local addr conflicts with existing interface",
	"Peer addr conflicts with existing interface",
	"Device problem",
	"Interface problem",
	"Filter problem",
};

char *ipcp_compress[] = {
	"VJ Compression",
	"VJ Slot Compressed",
};

STATIC int
ip_status(struct ipcp_status_s *st, int depth, int v, int (*pf)(...))
{
	struct ipcp_s *ip = &st->st_ip;
	char *ds;
	extern char *fsm_states[];

	if (depth > 0)
		ds = "\t";
	else
		ds = "";

	switch(v) {

	case PSM_V_SILENT:
		break;
	
        case PSM_V_SHORT:
		(*pf)("\t%sIPCP\tState:%s\n", ds,
		      fsm_state(&st->st_fsm));
		break;

	case PSM_V_LONG: 
		(*pf)("\t%sIPCP State    : %s\n", ds,
		       fsm_state(&st->st_fsm));
		(*pf)("\t%sReason        : %s\n", ds,
		      fsm_reason(&st->st_fsm));
		(*pf)("\t%sPacket counts : %s\n", ds,
		      fsm_pktcnts(&st->st_fsm));
		(*pf)("\t%sInterface     : %s\n", ds,
		       ip->ip_ifname);
		(*pf)("\t%sLocal address : %s\n", ds,
		       inet_ntoa(ip->ip_local_addr));
		(*pf)("\t%sLocal options : %s\n", ds,
		      psm_display_flags(2, ipcp_compress,
					ip->ip_local_compress));
		(*pf)("\t%sPeer address  : %s\n", ds,
		       inet_ntoa(ip->ip_peer_addr));
		(*pf)("\t%sPeer options  : %s\n", ds,
		      psm_display_flags(2, ipcp_compress,
					ip->ip_peer_compress));
		(*pf)("\t%sFailures      : %s\n", ds,
		      psm_display_flags(8, ipcp_reasons, ip->ip_reason));
		(*pf)("\n");
		break;
	}

        return st->st_fsm.ps_state;

}

struct psm_entry_s psm_entry = {
	ip_opts,
	ip_set_defaults,
	ip_list,
	ip_status,
	NULL,
	PSMF_BUNDLE | PSMF_NCP | PSMF_FSM,
};
