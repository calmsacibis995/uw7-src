#ifndef _IP_CFG_H
#define _IP_CFG_H

#ident	"@(#)ip_cfg.h	1.6"

#include <netinet/in.h>
#include "fsm.h"

/*
 * IP specific options
 * noipaddr, rfc1172addr, novj, maxslot, noslotcomp
 */
struct cfg_ipcp {
	struct cfg_hdr 	ip_ch;
	uint_t ip_name;	/* The protocol name */
	struct cfg_fsm	ip_fsm;
	uint_t ip_local_addr; 
	uint_t ip_peer_addr;
	uint_t ip_local_addropt;
	uint_t ip_peer_addropt;
	uint_t ip_vjcomp;
	uint_t ip_vjslotcomp;
	uint_t ip_vjslotmax;
	uint_t ip_exec;
	uint_t ip_bringup;
	uint_t ip_keepup;
	uint_t ip_passin;
	uint_t ip_passout;
	uint_t ip_proxyarp;
	uint_t ip_advdns;
	uint_t ip_advdns2;
	uint_t ip_advdnsopt;
	uint_t ip_getdns;	
	uint_t ip_defaultroute;
	uint_t ip_debug;
	uint_t ip_netmask;	/* For RIPv1 */
	char	ip_var[1]; /*Strings*/
};

/*
 * Type of ip address allocation
 */
#define IPADDR_FORCE 1
#define IPADDR_PREFER 2
#define IPADDR_ANY 3
#define IPADDR_POOL 4

/*
 * Values for ip_advdnsopt
 */
#define DNS_ADDR 0 /* Is address of DNS Server */
#define DNS_DHCP 1 /* Use DHCP to obtain this */
#define DNS_LOCAL 2 /* Use local resolv.conf info */
#define DNS_NONE 3 /* Don't provide DNS info */

/*
 * Default config values
 */
#define IP_DEFAULT_LOCALADDR ""
#define IP_DEFAULT_PEERADDR ""
#define IP_DEFAULT_LOCALOPT IPADDR_ANY
#define IP_DEFAULT_PEEROPT IPADDR_ANY
#define IP_DEFAULT_VJCOMPRESS 0
#define IP_DEFAULT_SLOT_MAX 15
#define IP_DEFAULT_SLOT_COMP 1
#define IP_DEFAULT_KEEPUP "keepup"
#define IP_DEFAULT_BRINGUP "bringup"
#define IP_DEFAULT_PASSIN ""
#define IP_DEFAULT_PASSOUT ""
#define IP_DEFAULT_PROXYARP 0
#define IP_DEFAULT_EXEC "/usr/lib/ppp/psm/ipexec.sh"
#define IP_DEFAULT_ADVDNS ""
#define IP_DEFAULT_ADVDNS2 ""
#define IP_DEFAULT_ADVDNSOPT DNS_NONE
#define IP_DEFAULT_GETDNS 0
#define IP_DEFAULT_DEFAULTROUTE 0
#define IP_DEFAULT_DEBUG 0
#define IP_DEFAULT_NETMASK "255.255.255.0"
#define IFNAME_MAXLEN 10

/*
 * Per connection ip status
 */
struct ipcp_s {
	int		ip_fd;	/* File descriptor for dev/ip */
	int		ip_mux;	/* Mux ID */
	char		ip_ifname[IFNAME_MAXLEN + 1];
	struct in_addr	ip_local_addr;
	struct in_addr	ip_peer_addr;
	uint_t		ip_local_compress;
	uint_t		ip_peer_compress;
	uint_t		ip_local_slotmax;
	uint_t		ip_peer_slotmax;
	struct in_addr	ip_cfg_local; /* Config local addr (net byte order) */
	struct in_addr	ip_cfg_peer; /* Config peer addr (net byte order) */
	struct in_addr	ip_old_local; /* Old local addr (net byte order) */
	struct in_addr	ip_old_peer; /* Old peer addr (net byte order) */
	struct in_addr	ip_dns_addr; /* Address of DNS server */
	struct in_addr	ip_dns2_addr; /* Address of Secondary DNS server */
	struct in_addr	ip_advdns_addr; /* Advertised Address of DNS server */
	struct in_addr	ip_advdns2_addr; /* Advertised Address of
					 * Secondary DNS server */
	AasConnection	*ip_aasconn;	/* Connection to AAS */
	AasClientId	ip_local_cid;	/* Local Client Id */
	AasClientId	ip_peer_cid;	/* Peer Client Id */
	ushort_t	ip_reason;	/* Failure codes */
};
/*
 * Flags for _compress fields
 */
#define IPC_VJCOMPRESS 0x0001	/* Perform vj header compression */
#define IPC_VJSLOTCOMP 0x0002	/* Allow slot compression */

/*
 * Flags for ip_reason
 */
#define IPR_NOLOCAL	0x0001	/* No local address */
#define IPR_NOPEER	0x0002	/* No peer address */
#define IPR_SAME	0x0004	/* Local and peer addresses are same */
#define IPR_LCONFLICT	0x0008	/* Local Address conflicts with another */
#define IPR_PCONFLICT	0x0010	/* Peer Address conflicts with another */
#define IPR_DEV		0x0020	/* Device error */
#define IPR_INF		0x0040	/* Interface error */
#define IPR_FILTER	0x0080	/* Filter error */


struct ipcp_status_s {
	proto_state_t st_fsm;
	struct ipcp_s st_ip;
};

#endif /* _IP_CFG_H */
