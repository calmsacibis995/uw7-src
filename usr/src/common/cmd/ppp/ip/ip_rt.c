#ident	"@(#)ip_rt.c	1.10"

#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <synch.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <aas/aas.h>
#include <sys/un.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <netinet/if_ether.h>
#include <sys/ppp.h>
#include <sys/ppp_f.h>
#include <sys/ppp_ip.h>
#include <filter.h>

#include "ppp_type.h"
#include "ppp_cfg.h"
#include "psm.h"
#include "act.h"
#include "lcp_hdr.h"
#include "ip_cfg.h"
#include "ppp_proto.h"


/*
 * Filtering stuff
 */
#define FILTER_FILE "/usr/lib/ppp/psm/ipfilters"
#define FILTER_PROTOCOL "IP"
#define FILTER_FRAMETYPE DLT_RAW
#define FILTER_NETMASK	0xffffffff

/* Should get these from somewhere else ! */
#define DEV_IP "/dev/ip"
#define DEV_ROUTE "/dev/route"

/*
 * Name of the loadable IPCP kernel module
 */
#define PPPIP "pppip"

/*
 * IPCP Protocol number
 */
#define PROTO_IPCP		0x8021

enum {
	IPCPIF_UP = 0,
	IPCPIF_DOWN,
	IPCPIF_ADD,
	IPCPIF_DEL
};

/*
 * This table is indexed by the enumerated values above
 */
char *updown[] = {
	"up",
	"down",
	"add",
	"delete",
};

/*
 * IPCP Configuration Options
 */
#define IP_ADDRESES 1	/* For RFC 1172 */
#define IP_COMP_PROTO 2
#define IP_ADDRESS 3
#define IP_DNS 129
#define IP_DNS2 131

/*
 * IPCP on the wire Option formats
 */
#pragma pack(1)
struct co_comp_s {
	struct co_s h;
	u16b_t co_proto;
	u8b_t co_slotmax;
	u8b_t co_slotcomp;
};

struct co_address_s {
	struct co_s h;
	u32b_t co_addr;
};

struct co_dns_s {
	struct co_s h;
	u32b_t co_addr;
};
#pragma pack(4)


STATIC uint_t ipcp_getaddr(struct ipcp_s *ipcp, char *addrstr,
			   int addropt, int type);
int ip_log(proto_hdr_t *pr, uchar_t *dp, int len);

/*
 * Values for type .. to pass to ipcp_checkaddr, ipcp_getaddr
 */
enum { IP_LOCAL, IP_PEER };

/*
 * Service type for AAS
 */
#define IPCP_AAS_SERVICE "PPP(IPCP)"

#define AAS_CFGFILE NULL /*"/etc/inet/aas.conf"*/
#define AAS_PASSWDLEN 1024
/*
 * Globals, these are only modified in the load/unload routines
 */
int ipfd = -1;
AasServer aas_server;
int aas_addr;
char aas_pwd[AAS_PASSWDLEN];
int aas_available;

STATIC int
ipcp_getdns(proto_hdr_t *ph, struct ipcp_s *ip, struct cfg_ipcp *cp)
{
	struct __res_state *rp;
	int i;

	switch(cp->ip_advdnsopt) {
	case DNS_ADDR:
		ip->ip_advdns_addr.s_addr =
			ipcp_getaddr(ip, ucfg_str(&cp->ip_ch,
					      cp->ip_advdns), IPADDR_ANY, 0);

		if (ip->ip_advdns_addr.s_addr == -1) {
			psm_log(MSG_INFO_MED, ph,
		 "Advertised DNS address '%s' is invalid, using 0.0.0.0\n",
				 ucfg_str(&cp->ip_ch, cp->ip_advdns));
			ip->ip_advdns_addr.s_addr = 0;
		}

		ip->ip_advdns2_addr.s_addr =
			ipcp_getaddr(ip, ucfg_str(&cp->ip_ch, cp->ip_advdns2),
				     IPADDR_ANY, 0);

		if (ip->ip_advdns2_addr.s_addr == -1) {
			psm_log(MSG_INFO_MED, ph,
	 "Advertised Secondary DNS address '%s' is invalid, using 0.0.0.0\n",
				 ucfg_str(&cp->ip_ch, cp->ip_advdns2));
			ip->ip_advdns2_addr.s_addr = 0;
		}
		break;

	case DNS_DHCP:
		break;

	case DNS_LOCAL:
		res_init();

		rp = (struct __res_state *)get_rs__res();
		if (!rp) {
			psm_log(MSG_WARN, ph,
				"Failed to get DNS info from resolver\n");
			ip->ip_advdns_addr.s_addr = 0;
			ip->ip_advdns2_addr.s_addr = 0;
			break;
		}

		psm_log(MSG_DEBUG, ph,
			 "ipcp_getdns: Configured nameservers %d\n",
			 rp->nscount);

		for (i = 0; i < rp->nscount; i++) {
			psm_log(MSG_DEBUG, ph, 
				 "ipcp_getdns: namserver %d = %s\n",
				 i, inet_ntoa(rp->nsaddr_list[i].sin_addr));
		}
		
		if (rp->nscount > 0)
			ip->ip_advdns_addr = rp->nsaddr_list[0].sin_addr;
		else
			ip->ip_advdns_addr.s_addr = 0;

		if (rp->nscount > 1)
			ip->ip_advdns2_addr = rp->nsaddr_list[1].sin_addr;
		else
			ip->ip_advdns2_addr.s_addr = 0;
		break;

	case DNS_NONE:
		ip->ip_advdns_addr.s_addr = 0;
		ip->ip_advdns2_addr.s_addr = 0;
		break;
	}
}

/*
 * Free any allocated addresses
 */
STATIC int
ipcp_free_addr(struct proto_hdr_s *ph, int type)
{
	struct ipcp_s *ipcp = (struct ipcp_s *)ph->ph_priv;
	struct cfg_ipcp *cp = (struct cfg_ipcp *)ph->ph_cfg;
	int ret = 0;
	AasAddr aas_addr;
	
	if (!ipcp->ip_aasconn)
		return 0;

	/* Free any allocated addresses */
	if (type == IP_LOCAL && cp->ip_local_addropt == IPADDR_POOL) {

		aas_addr.addr = (void *)&ipcp->ip_local_addr;
		aas_addr.len = sizeof(ipcp->ip_local_addr);
		ret = aas_free(ipcp->ip_aasconn,
			       ucfg_str(&cp->ip_ch, cp->ip_local_addr),
			       AAS_ATYPE_INET,
			       &aas_addr,
			       IPCP_AAS_SERVICE,
			       &ipcp->ip_local_cid);

		if (ret) {
			psm_log(MSG_WARN, ph,
				 "Failed to free local address\n");
		}
	}

	if (type == IP_PEER && cp->ip_peer_addropt == IPADDR_POOL) {

		aas_addr.addr = (void *)&ipcp->ip_peer_addr;
		aas_addr.len = sizeof(ipcp->ip_peer_addr);
		ret = aas_free(ipcp->ip_aasconn,
			       ucfg_str(&cp->ip_ch, cp->ip_peer_addr),
			       AAS_ATYPE_INET,
			       &aas_addr,
			       IPCP_AAS_SERVICE,
			       &ipcp->ip_peer_cid);

		if (ret) {
			psm_log(MSG_WARN, ph,
				 "Failed to free peer address\n");
		}
	}
	return ret;
}

/*
 * This function will convert a string 
 *	a) hostname
 *	b) dotted address
 *	c) pool
 * into an int representing that address. (Network order)
 */
STATIC uint_t
ipcp_getaddr(struct ipcp_s *ipcp, char *addrstr, int addropt, int type)
{
	struct in_addr addr;
	struct hostent *he;
	int ret;
	AasAddr allocaddr;
	AasClientId *cid;

	if (addropt & IPADDR_POOL) {

		if (!ipcp->ip_aasconn) {
			psm_log(MSG_WARN, 0,
	"IPCP Couldn't get address from Adress Server - no connection\n");
			return -1;
		}

		cid = (type == IP_LOCAL) ?
			&ipcp->ip_local_cid : &ipcp->ip_peer_cid;

		allocaddr.addr = (void *)&addr.s_addr;
		allocaddr.len = sizeof(addr.s_addr);

		ret = aas_alloc(ipcp->ip_aasconn, addrstr, AAS_ATYPE_INET,
				NULL, NULL, NULL, 0, -1, IPCP_AAS_SERVICE,
				cid, &allocaddr);
		if (ret) {
			psm_log(MSG_DEBUG, 0,
			       "IPCP ipcp_getaddr: aas_alloc failed\n");
			abort();
/* SOMETHING TO DO HERE */
		} else {
			psm_log(MSG_DEBUG, 0,
			       "IPCP ipcp_getaddr: got addr %s\n",
			       inet_ntoa(addr));
		}
	} else {
		if (!addrstr || !*addrstr)
			return 0;

		/* See if the address is in dot notation */
		addr.s_addr = inet_addr(addrstr);
		if (addr.s_addr == -1) {
			/* Not in dot notation ... is it a host name .. */
			he = gethostbyname(addrstr);
			addr.s_addr = he ? *((uint_t *)*(he->h_addr_list)):-1;
		}
	}

	return addr.s_addr;
}

/*
 * Get all the interface info
 */
STATIC struct ifa_msghdr *
ifinfo_get(int *len)
{
	struct rt_giarg *gp;
	int sysctl_buf_size = 0, needed;
	char *sysctl_buf;
	int rtfd;

	rtfd = open(DEV_ROUTE, O_RDWR);
	if (rtfd < 0) {
		psm_log(MSG_ERROR, 0,
			"IPCP - Failed to open routing driver (%d)\n",
			errno);
		return NULL;
	}

	/* always need at least sizeof(rt_giarg) in
	 * the buffer
	 */
	sysctl_buf = (char *)malloc(sizeof(struct rt_giarg));
	sysctl_buf_size = sizeof(struct rt_giarg);

	for(;;) {
		gp = (struct rt_giarg *)sysctl_buf;
		gp->gi_op = KINFO_RT_DUMP;
		gp->gi_where = (caddr_t)sysctl_buf;
		gp->gi_size = sysctl_buf_size;
		gp->gi_arg = 0;

		if (ioctl(rtfd, RTSTR_GETIFINFO, sysctl_buf) < 0) {
			psm_log(MSG_ERROR, 0,
				"IPCP - RTSTR_GETIFINFO failed (%d)\n",
				errno);
			free(sysctl_buf);
			close(rtfd);
			return NULL;
		}

		needed = gp->gi_size;
		if (sysctl_buf_size >= needed)
			break;
		
		free(sysctl_buf);
		sysctl_buf_size = needed;
		sysctl_buf = (char *)malloc(sysctl_buf_size);
	}

	close(rtfd);

	needed -= sizeof(struct rt_giarg);
	*len = needed;
	return (struct ifa_msghdr *)(sysctl_buf + sizeof(struct rt_giarg));
}

/*
 * Free off allocated memory
 */
STATIC void
ifinfo_free(struct ifa_msghdr *p)
{
	free(((char *)p) - sizeof(struct rt_giarg));
}

/*
 * Round up to a word boundary
 */
void
xaddrs(struct rt_addrinfo *info, struct sockaddr *sa,
       struct sockaddr *lim, int addrs)
{
	int i;
	static struct sockaddr sa_zero;

#define ROUNDUP(a) ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) \
		    : sizeof(long))

	memset(info, 0, sizeof(*info));
	info->rti_addrs = addrs;
	for (i = 0; i < RTAX_MAX && sa < lim; i++) {
		if ((addrs & (1 << i)) == 0)
			continue;
		info->rti_info[i] = (sa->sa_len != 0) ? sa : &sa_zero;
		sa = (struct sockaddr *)((char*)(sa)
					 + ROUNDUP(sa->sa_len));
	}
}


/*
 * Obtain the address of any configured interface (even ppp, not loopback)
 */
STATIC int
ipcp_getifaddr()
{
	struct in_addr addr;
	struct ifa_msghdr *ifam_start, *ifam;
	int iflen;
	struct if_msghdr *ifm;
	struct sockaddr *sa;
	struct sockaddr_dl *sdl;
	int if_flags;
	struct rt_addrinfo ainfo;
	struct sockaddr_in *sin;

	ifam = ifam_start = ifinfo_get(&iflen);
	if (!ifam_start)
		return 0;

	addr.s_addr = 0;

	while ((char *)ifam < (char *)ifam_start + iflen && !addr.s_addr) {

		ASSERT(ifam->ifam_type == RTM_IFINFO);

		ifm = (struct if_msghdr *)ifam;

		sdl = (struct sockaddr_dl *)(ifm + 1);
		sdl->sdl_data[sdl->sdl_nlen] = 0;

#ifdef DEBUG_IPADDRS
		psm_log(MSG_DEBUG, 0,
			"ipcp_getifaddr:  if_index = %d, Interface : %s\n",
			ifm->ifm_index, sdl->sdl_data);
#endif

		/* Save the flags */

		if_flags = ifm->ifm_flags;

		/* Skip over this interface entry */

		ifam = (struct ifa_msghdr *)((char *)ifam + ifam->ifam_msglen);

		/* Check the list of addrs */

		while ((char *)ifam < (char *)ifam_start + iflen) {
			
			if (ifam->ifam_type != RTM_NEWADDR)
				break;
#ifdef DEBUG_IPADDRS
			psm_log(MSG_DEBUG, 0,
			"ipcp_getifaddr: RTM_NEWADDR, ifam_addrs 0x%x\n",
				ifam->ifam_addrs);
#endif
			if (if_flags & IFF_LOOPBACK) {
				/* Skip over this address entry */
				ifam = (struct ifa_msghdr *)
					((char *)ifam + ifam->ifam_msglen);
				continue;
			}

			xaddrs(&ainfo, (struct sockaddr *)(ifam+1),
			       (struct sockaddr *)
			       ((char *)ifam + ifam->ifam_msglen),
			       ifam->ifam_addrs);

			/* Get the address  */

			if (ifam->ifam_addrs & RTA_IFA &&
			    ainfo.rti_info[RTAX_IFA]->sa_family == AF_INET) {

				sin = (struct sockaddr_in *)
					ainfo.rti_info[RTAX_IFA];
#ifdef DEBUG_IPADDRS
				psm_log(MSG_DEBUG, 0,
				"ipcp_getifaddr: RTM_NEWADDR, IFA %s\n",
					(char *)inet_ntoa(sin->sin_addr));
#endif
				addr = sin->sin_addr;
				break;
			}

			ifam = (struct ifa_msghdr *)
				((char *)ifam + ifam->ifam_msglen);
		}
	}	     

	ifinfo_free(ifam_start);
	return addr.s_addr;
}

/*
 * Check an ip address is suitable
 *
 * Check that the peer address is not the same as any configured interfaces
 * source address.
 *
 * Check the local address is not the same as any configured
 * point-to-point  interface destination address
 *
 * Return 0 if acceptable, otherwise non-zero.
 */
STATIC int
ipcp_checkaddr(struct ipcp_s *ip, struct in_addr addr, int type)
{
	struct ifa_msghdr *ifam_start, *ifam;
	int iflen;
	struct if_msghdr *ifm;
	struct sockaddr *sa;
	struct sockaddr_dl *sdl;
	int if_flags;
	struct rt_addrinfo ainfo;
	struct sockaddr_in *lsin, *dsin;
	int skip;

	ifam = ifam_start = ifinfo_get(&iflen);
	if (!ifam_start)
		return 0;

	while ((char *)ifam < (char *)ifam_start + iflen) {

		ASSERT(ifam->ifam_type == RTM_IFINFO);

		ifm = (struct if_msghdr *)ifam;

		sdl = (struct sockaddr_dl *)(ifm + 1);
		sdl->sdl_data[sdl->sdl_nlen] = 0;
#ifdef DEBUG_IPADDRS
		psm_log(MSG_DEBUG, 0,
			"ipcp_checkaddr:  if_index = %d, Interface : %s\n",
			ifm->ifm_index, sdl->sdl_data);
#endif
		/* Save the flags */

		if_flags = ifm->ifm_flags;

		if (if_flags & IFF_LOOPBACK || 
		    strcmp(sdl->sdl_data, ip->ip_ifname) == 0)
			skip = 1;
		else
			skip = 0;

		/* Skip over this interface entry */

		ifam = (struct ifa_msghdr *)((char *)ifam + ifam->ifam_msglen);

		/* Check the list of addrs */

		while ((char *)ifam < (char *)ifam_start + iflen) {
			
			if (ifam->ifam_type != RTM_NEWADDR)
				break;
#ifdef DEBUG_IPADDRS
			psm_log(MSG_DEBUG, 0,
			"ipcp_checkaddr: RTM_NEWADDR, ifam_addrs 0x%x\n",
				ifam->ifam_addrs);
#endif
			if (skip) {
				/* Skip over this address entry */
				ifam = (struct ifa_msghdr *)
					((char *)ifam + ifam->ifam_msglen);
				continue;
			}

			xaddrs(&ainfo, (struct sockaddr *)(ifam+1),
			       (struct sockaddr *)
			       ((char *)ifam + ifam->ifam_msglen),
			       ifam->ifam_addrs);

			/* Get the address  */

			if (!(ifam->ifam_addrs & RTA_IFA &&
			    ainfo.rti_info[RTAX_IFA]->sa_family == AF_INET)) {
				ifam = (struct ifa_msghdr *)
					((char *)ifam + ifam->ifam_msglen);
				continue;
			}

			/* Check this address entry */
			
			lsin = (struct sockaddr_in *)
				ainfo.rti_info[RTAX_IFA];

			dsin = (struct sockaddr_in *)
				ainfo.rti_info[RTAX_BRD];
#ifdef DEBUG_IPADDRS
			psm_log(MSG_DEBUG, 0,
				"ipcp_checkaddr: RTM_NEWADDR, IFA %s\n",
				(char *)inet_ntoa(lsin->sin_addr));
#endif
			switch (type) {
			case IP_LOCAL:
				/*
				 * Check the local address is not the
				 * same as any configured point-to-point
				 * interface destination address
				 */
				if (!(if_flags & IFF_POINTOPOINT))
					break;

				if (dsin->sin_addr.s_addr == addr.s_addr) {
					psm_log(MSG_DEBUG, 0,
				"ipcp_checkaddr: Address %s conflicts\n",
						(char *)inet_ntoa(addr));
					ifinfo_free(ifam_start);
					return -1;
				}
				break;

			case IP_PEER:
				/*
				 * Check that the peer address is not the
				 * same as any configured interfaces
				 * source address.
				 */
				if (lsin->sin_addr.s_addr == addr.s_addr) {
					psm_log(MSG_DEBUG, 0,
				"ipcp_checkaddr: Address %s conflicts\n",
						(char *)inet_ntoa(addr));
					ifinfo_free(ifam_start);
					return -1;
				}
				/*
				 * Check the peer address is not the same as
				 * the peer address of any other point-to-point
				 * link
				 */
				if (!(if_flags & IFF_POINTOPOINT))
					break;
					
				if (dsin->sin_addr.s_addr == addr.s_addr) {
					psm_log(MSG_DEBUG, 0,
				"ipcp_checkaddr: Address %s conflicts\n",
						(char *)inet_ntoa(addr));
					ifinfo_free(ifam_start);
					return -1;
				}
				break;
			}

			/* Skip to the next address */

			ifam = (struct ifa_msghdr *)
				((char *)ifam + ifam->ifam_msglen);
		}
	}	     

	ifinfo_free(ifam_start);

	return 0;
}

/*
 * Returns, 0 success, -1 failure.
 */
STATIC int
ipcp_proxyadd(struct ipcp_s *ip)
{
	struct ifa_msghdr *ifam_start, *ifam;
	int iflen;
	struct if_msghdr *ifm;
	struct sockaddr *sa;
	struct sockaddr_dl *sdl;
	int if_flags;
	struct rt_addrinfo ainfo;
	struct sockaddr_in *sin;
	int skip;
	uint_t laddr, mask;
	uchar_t	a[23];
	int ret = -1;
	uint_t dst;
	uint_t src;

	psm_log(MSG_DEBUG, 0, "IPCP - proxyadd\n");

	dst = ip->ip_peer_addr.s_addr;

	ifam = ifam_start = ifinfo_get(&iflen);
	if (!ifam_start)
		return -1;

	while ((char *)ifam < (char *)ifam_start + iflen) {

		ASSERT(ifam->ifam_type == RTM_IFINFO);

		ifm = (struct if_msghdr *)ifam;

		sdl = (struct sockaddr_dl *)(ifm + 1);
		sdl->sdl_data[sdl->sdl_nlen] = 0;

		psm_log(MSG_DEBUG, 0,
			"ipcp_proxyadd: if_index = %d, Interface : %s\n",
			ifm->ifm_index, sdl->sdl_data);

		/* Save the flags */

		if_flags = ifm->ifm_flags;

		if (if_flags & IFF_LOOPBACK || if_flags & IFF_POINTOPOINT ||
		    strcmp(sdl->sdl_data, ip->ip_ifname) == 0)
			skip = 1;
		else
			skip = 0;

		/* Skip over this interface entry */

		ifam = (struct ifa_msghdr *)((char *)ifam + ifam->ifam_msglen);

		/* Check the list of addrs */

		while ((char *)ifam < (char *)ifam_start + iflen) {
			
			if (ifam->ifam_type != RTM_NEWADDR)
				break;

			psm_log(MSG_DEBUG, 0,
			"ipcp_proxyadd: RTM_NEWADDR, ifam_addrs 0x%x\n",
				ifam->ifam_addrs);

			if (skip) {
				/* Skip over this address entry */
				ifam = (struct ifa_msghdr *)
					((char *)ifam + ifam->ifam_msglen);
				continue;
			}

			xaddrs(&ainfo, (struct sockaddr *)(ifam+1),
			       (struct sockaddr *)
			       ((char *)ifam + ifam->ifam_msglen),
			       ifam->ifam_addrs);

			/* Get the address  */

			if (!(ifam->ifam_addrs & RTA_IFA &&
			    ainfo.rti_info[RTAX_IFA]->sa_family == AF_INET)) {
				ifam = (struct ifa_msghdr *)
					((char *)ifam + ifam->ifam_msglen);
				continue;
			}

			/* Check this address entry */
			
			mask = ((struct sockaddr_in *)
				ainfo.rti_info[RTAX_NETMASK])->sin_addr.s_addr;

			laddr = ((struct sockaddr_in *)
				ainfo.rti_info[RTAX_IFA])->sin_addr.s_addr;

			/* mask is in network order */
			/*mask = ntohl(mask);*/


			psm_log(MSG_DEBUG, 0,
			"ipcp_proxyadd: src %x dst %x laddr %x mask %x\n",
				src, dst, laddr, mask);

			if ((laddr & mask) == (dst & mask)) {

				psm_log(MSG_DEBUG, 0,
					"ipcp_proxyadd: MAC addr\n");
				psm_loghex(MSG_DEBUG, 0, (char *)LLADDR(sdl),
					   sdl->sdl_alen);

				if (arp_set(dst, LLADDR(sdl), sdl->sdl_alen) < 0) {
					ret = -1;
					goto exit;
				}
				ret = 0;
			}

			ifam = (struct ifa_msghdr *)
				((char *)ifam + ifam->ifam_msglen);
		}
	}	     

	if (ret)
		psm_log(MSG_DEBUG, 0, "ipcp_proxyadd: No match\n");

 exit:
	ifinfo_free(ifam_start);
	return ret;

}
 
/*
 * add proxy arp entry 
 * return
 *    -1: fail
 *    0: no need to add proxy entry 
 *    1: add proxy arp entry
 */

STATIC void
ipcp_proxydel(uint_t addr)
{
	arp_del(addr);
}

/*
 * IP Addresses (RFC 1172)
 */

/*
 * IP Compression
 */
STATIC int
rcv_comp(proto_hdr_t *ph, int action, db_t *db, db_t *ndb)
{
	struct cfg_ipcp *cp = (struct cfg_ipcp *)ph->ph_cfg;
	struct ipcp_s *ip = (struct ipcp_s *)ph->ph_priv;
	struct co_comp_s *comp;
	struct co_comp_s *nco;

	ip->ip_peer_compress = 0;

	switch (action) {
	case CFG_MKNAK:
		goto nakcomp;

	case CFG_DEFAULT:
		psm_log(MSG_INFO_MED, ph, "Default Compression (None)\n");
		ip->ip_peer_slotmax = 16; /* A sensible value */
		return CFG_ACK;
	default:
		break;
	}

	comp = (struct co_comp_s *)db->db_rptr;
	/* Check the protocol is VJ */

	if (ntohs(comp->co_proto) != PROTO_TCP_COMP) {
		/* Nak with VJ */
		psm_log(MSG_INFO_MED, ph,
			 "Peer requested compression protocol %4.4x (Nak)\n",
			 ntohs(comp->co_proto));
		goto nakcomp;
	}
	
	if (comp->h.co_len != 6) {
		psm_log(MSG_INFO_MED, ph, "Bad option length - Nak\n");
		goto nakcomp;
	}

	/* So we have vj compression, check the other options */

	ip->ip_peer_compress |= IPC_VJCOMPRESS;
	psm_log(MSG_INFO_MED, ph,
		 "Peer requests VJ Header Compression (Ack)\n");

	if (comp->co_slotcomp) {
		ip->ip_peer_compress |= IPC_VJSLOTCOMP;
		psm_log(MSG_INFO_MED, ph,
			 "Peer requests Slot Compression (Ack)\n");
	}
	
	ip->ip_peer_slotmax = comp->co_slotmax + 1;
/* Max slot .. any limit that we impose ???? */	

	psm_log(MSG_INFO_MED, ph, "Peer requests Max-Slot %d (Ack)\n",
		 ip->ip_peer_slotmax);
	return CFG_ACK;

nakcomp:
	psm_log(MSG_INFO_MED, ph,
		 "Nak Compression protocol with VJ Compression\n");
	nco = (struct co_comp_s *)ndb->db_wptr;
	nco->h.co_type = IP_COMP_PROTO;
	nco->h.co_len = 6;
	nco->co_proto = htons(PROTO_TCP_COMP);
	nco->co_slotmax = IP_DEFAULT_SLOT_MAX + 1;
	nco->co_slotcomp = IP_DEFAULT_SLOT_COMP;
	ndb->db_wptr += sizeof(struct co_comp_s);
	return CFG_NAK;
}

/*
 * Address Negotiation
 */
STATIC int
rcv_address(proto_hdr_t *ph, int action, db_t *db, db_t *ndb)
{
	struct in_addr ip_addr, new_addr;
	struct ipcp_s *ipcp = (struct ipcp_s *)ph->ph_priv;
	struct co_address_s *addr, *naddr;
	struct cfg_ipcp *cp = (struct cfg_ipcp *)ph->ph_cfg;
	int ret;
	AasAddr req_addr, alloc_addr;

	switch (action) {
	case CFG_MKNAK:
		ip_addr = ipcp->ip_cfg_peer;
		goto nakaddr;

	case CFG_DEFAULT:
		psm_log(MSG_INFO_MED, ph,
			 "Peer didn't request an IP Address (Nak)\n");
		ip_addr = ipcp->ip_cfg_peer;
		goto nakaddr;
	default:
		break;
	}

	addr = (struct co_address_s *)db->db_rptr;
	ip_addr.s_addr = addr->co_addr;

	/* Check if this address is acceptable */

	if (ip_addr.s_addr == 0) {
		/* Peer requires an address to be allocated for it */
		psm_log(MSG_INFO_MED, ph,
			 "Peer requires address allocation\n");

		if (ipcp->ip_cfg_peer.s_addr == 0) {
			/*
			 * We don't have an address to assign to the peer.
			 *
			 * If we were using address pools .. this wouldn't
			 * have been a zero address ...
			 */
			psm_log(MSG_WARN, ph,
				"Reject - No Peer address configured\n");
			return CFG_REJ;
		} else
			ip_addr = ipcp->ip_cfg_peer;
		goto nakaddr;
	} 

	/*
	 * Check acceptablity of the address
	 */
	switch(cp->ip_peer_addropt) {
	case IPADDR_FORCE:
		if (ipcp->ip_cfg_peer.s_addr != ip_addr.s_addr) {
			psm_log(MSG_INFO_MED, ph,
		      "Peer Address not as configured (Force) %s (Nak).\n",
				 (char *)inet_ntoa(ip_addr));

			ip_addr = ipcp->ip_cfg_peer;
			goto nakaddr;
		}
		break;

	case IPADDR_PREFER:
		if (ipcp->ip_cfg_peer.s_addr != ip_addr.s_addr) {
			/*
			 * If the configured addr matches that in the private
			 * area .. then it's the first attempt .. nak with 
			 * the prefered value
			 */
			if (ipcp->ip_cfg_peer.s_addr != 
			    ipcp->ip_peer_addr.s_addr)
				break;

			/* Ensure the address we suggest is available */
			if (ipcp_checkaddr(ipcp, ipcp->ip_cfg_peer,
					   IP_PEER) == 0) {
				psm_log(MSG_INFO_MED, ph,
					 "Peer Address not Prefered %s (Nak)\n",
					 (char *)inet_ntoa(ip_addr));
				ipcp->ip_peer_addr = ip_addr;
				ip_addr = ipcp->ip_cfg_peer;
				goto nakaddr;
			}
		}
		/* Allow it */
		break;

	case IPADDR_ANY:
		/* Allow any address */
		break;

	case IPADDR_POOL:
		if (!ipcp->ip_aasconn) {
			psm_log(MSG_WARN, ph,
	 "Failed to allocate address - no connection to Address Server\n");
			break;
		}
			
		/*
		 * Check if the address provided is in the pool and
		 * available
		 */
		if (ipcp->ip_cfg_peer.s_addr == ip_addr.s_addr)
			break;
		
		/* Free the old address */
		ipcp_free_addr(ph, IP_PEER);

		/* Try for the new address */
		req_addr.addr = (void *)&ip_addr.s_addr;
		req_addr.len = sizeof(ip_addr.s_addr);
		alloc_addr.addr = (void *)&new_addr.s_addr;
		alloc_addr.len = sizeof(new_addr.s_addr);

		ret = aas_alloc(ipcp->ip_aasconn,
				ucfg_str(&cp->ip_ch, cp->ip_peer_addr),
				AAS_ATYPE_INET,
				&req_addr, NULL, NULL, 0, -1, IPCP_AAS_SERVICE,
				&ipcp->ip_peer_cid, &alloc_addr);

		if (ret) {
			psm_log(MSG_WARN, ph,
				 "Failed to allocate Requested address %d\n",
				 ret);
/* WHAT NOW ??? */			
			break;
		}

		if (*(int *)alloc_addr.addr != ip_addr.s_addr) {
			psm_log(MSG_DEBUG, ph,
				 "Didn't get requested address from pool.\n");
			ip_addr.s_addr = *(int *)alloc_addr.addr;
			goto nakaddr;
		}
		psm_log(MSG_DEBUG, ph,
			 "AAS allocated requested peer address\n");
		break;
	}

	ipcp->ip_peer_addr = ip_addr;

	psm_log(MSG_INFO_MED, ph, "Peer requests IP Address %s (Ack)\n",
		 inet_ntoa(ip_addr));

	if (ipcp_checkaddr(ipcp, ipcp->ip_peer_addr, IP_PEER)) {
		psm_log(MSG_WARN, ph,
	 "Acking Peer address that conflicts with existing interface\n");
	}
	
	return CFG_ACK;

nakaddr:
	psm_log(MSG_INFO_MED, ph, "Nak IP Address with %s\n", 
		 inet_ntoa(ip_addr));

	naddr = (struct co_address_s *)ndb->db_wptr;
	naddr->h.co_type = IP_ADDRESS;
	naddr->h.co_len = 6;
	naddr->co_addr = ip_addr.s_addr;
	ndb->db_wptr += sizeof(struct co_address_s);

	if (ipcp_checkaddr(ipcp, ip_addr, IP_PEER)) {
		psm_log(MSG_WARN, ph,
	 "Naking Peer address that conflicts with existing interface\n");
	}

	return CFG_NAK;
	
}

STATIC int
rcv_dns_common(char *desc, struct in_addr dnsaddr, int code, proto_hdr_t *ph,
	int action, db_t *db, db_t *ndb, int state)
{
	struct co_dns_s *dns = (struct co_dns_s *)db->db_rptr;
	struct in_addr ip_addr;

	switch (action) {
	case CFG_CHECK:
		ip_addr.s_addr = dns->co_addr;
		psm_log(MSG_INFO_MED, ph, "Peer requests %s Address %s\n",
			 desc, inet_ntoa(ip_addr));

		if (state == CFG_ACK || state == CFG_REJ) {
			if (dnsaddr.s_addr == 0) {
				psm_log(MSG_INFO_MED, ph,
				 "%s Address NOT configured - Reject\n",
					 desc);
				return CFG_REJ;
			}
		}

		if (state == CFG_ACK || state == CFG_NAK) {

			if (dns->co_addr != dnsaddr.s_addr) {
				psm_log(MSG_INFO_MED, ph, 
					 "Nak with configured value %s\n",
					 inet_ntoa(dnsaddr));
				goto nak_dns;
			}
		}
		break;

	case CFG_MKNAK:
		goto nak_dns;

	case CFG_DEFAULT:
		psm_log(MSG_INFO_MED, ph,
			 "Default DNS Advertisement (None)\n");
		return CFG_ACK;
	}

	return CFG_ACK;

 nak_dns:
	dns = (struct co_dns_s *)ndb->db_wptr;
	dns->h.co_type = code;
	dns->h.co_len = 6;
	dns->co_addr = dnsaddr.s_addr;
	ndb->db_wptr += sizeof(struct co_dns_s);
	return CFG_NAK;
}


STATIC int
rcv_dns(proto_hdr_t *ph, int action, db_t *db, db_t *ndb, int state)
{
	struct ipcp_s *ipcp = (struct ipcp_s *)ph->ph_priv;

	return rcv_dns_common("DNS", ipcp->ip_advdns_addr, IP_DNS,
			      ph, action, db, ndb, state);
}

STATIC int
rcv_dns2(proto_hdr_t *ph, int action, db_t *db, db_t *ndb, int state)
{
	struct ipcp_s *ipcp = (struct ipcp_s *)ph->ph_priv;

	return rcv_dns_common("Secondary DNS", ipcp->ip_advdns2_addr, IP_DNS2,
			      ph, action, db, ndb, state);
}

/*
 * Send IP Compression options
 */
STATIC int
snd_comp(proto_hdr_t *ph, int state, struct co_s *co, db_t *db)
{
	struct cfg_ipcp *cp = (struct cfg_ipcp *)ph->ph_cfg;
	struct ipcp_s *ip = (struct ipcp_s *)ph->ph_priv;
	struct co_comp_s *comp = (struct co_comp_s *)db->db_wptr;
	struct co_comp_s *co_comp = (struct co_comp_s *)co;

	comp->h.co_type = IP_COMP_PROTO;
	comp->h.co_len = 6;

	switch(state) {
	case CFG_REJ:
		psm_log(MSG_INFO_MED, ph,
		       "Peer Rejected VJ Header Compression\n");
		ip->ip_local_compress &= ~IPC_VJCOMPRESS;
		break;

	case CFG_NAK:
		psm_log(MSG_INFO_MED, ph,
		       "Peer Nak'ed VJ Header Compression\n");

		/* Assume the worst */
		ip->ip_local_compress &= ~(IPC_VJCOMPRESS | IPC_VJSLOTCOMP);

		if (!cp->ip_vjcomp) {
			psm_log(MSG_INFO_MED, ph,
				"Compression not configured\n");
			break;
		}
		
		/* Check we like the compression protocol */
		if (co_comp->co_proto != htons(PROTO_TCP_COMP)) {
			psm_log(MSG_INFO_MED, ph,
				"Unknown Compression protocol( %4.4x)\n",
				ntohs(co_comp->co_proto));
			break;
		}

		/* Use the number of slots suggested if > 3 */
		if (co_comp->co_slotmax > 3) {
			comp->co_slotmax = co_comp->co_slotmax;

			/* Save the value */
			ip->ip_local_slotmax = co_comp->co_slotmax + 1;

			psm_log(MSG_INFO_MED, ph,
				"Peer request Max-Slot-Id %d\n",
				co_comp->co_slotmax);
		} else {
			psm_log(MSG_INFO_MED, ph,
				"Bad Max-Slot-Id requested %d\n",
				co_comp->co_slotmax);
			break;
		}

		/* Check if we like the requested slot comp */
		if (co_comp->co_slotcomp) {

			if (!cp->ip_vjslotcomp) {
				psm_log(MSG_INFO_MED, ph,
			"Slot compression requested - not configured\n",
					co_comp->co_slotmax);
				break;
			}

			ip->ip_local_compress |= IPC_VJSLOTCOMP;
		}

		ip->ip_local_compress |= IPC_VJCOMPRESS;
		db->db_wptr += sizeof(struct co_comp_s);
		break;

	case CFG_ACK:
		if (ip->ip_local_compress & IPC_VJCOMPRESS) {
			psm_log(MSG_INFO_MED, ph,
				 "Request VJ Header Compression\n");
			comp->co_proto = htons(PROTO_TCP_COMP);
		} else {
			ip->ip_local_compress &= ~IPC_VJSLOTCOMP;
			return;
		}

		if (ip->ip_local_compress & IPC_VJSLOTCOMP) {
			psm_log(MSG_INFO_MED, ph,
				 "Request Slot Compression\n");
			comp->co_slotcomp = 1;
		} else
			comp->co_slotcomp = 0;

		psm_log(MSG_INFO_MED, ph,
			 "Request Max-Slot-Id %d\n", ip->ip_local_slotmax - 1);

		comp->co_slotmax = ip->ip_local_slotmax - 1;
		db->db_wptr += sizeof(struct co_comp_s);
		break;
	}
}

/*
 * Send an ip_address option
 */
STATIC int
snd_address(proto_hdr_t *ph, int state, struct co_s *co, db_t *db)
{
	struct co_address_s *addr = (struct co_address_s *)db->db_wptr;
	struct cfg_ipcp *cp = (struct cfg_ipcp *)ph->ph_cfg;
	struct ipcp_s *ipcp = (struct ipcp_s *)ph->ph_priv;
	struct in_addr ip_addr, new_addr;
	int ret;
	AasAddr req_addr, alloc_addr;

	/* Get the configured local address */

	switch(state) {
	case CFG_REJ:
		ip_addr.s_addr = 0;
		psm_log(MSG_INFO_MED, ph,
		"Address Option was rejected - Cannot get local address\n");
		/* What now ??? - Terminate IPCP */
		return;

	case CFG_NAK:

		ip_addr.s_addr = ((struct co_address_s *)co)->co_addr;

		if (ipcp->ip_cfg_local.s_addr == 0) {
			/* We requested that the peer provide our address */
			psm_log(MSG_INFO_MED, ph, 
				 "Local Address provided is %s\n",
				 (char *)inet_ntoa(ip_addr));
			break;
		}

		/* See if the address provided is acceptable */
		switch (cp->ip_local_addropt) {
		case IPADDR_FORCE:
			ip_addr = ipcp->ip_cfg_local;
			psm_log(MSG_INFO_MED, ph,
			       "Local Address (Force) was Nak'ed.  Retry.\n",
			       (char *)inet_ntoa(ip_addr));

			break;

		case IPADDR_PREFER:
			psm_log(MSG_INFO_MED, ph,
	       "Local Address (Prefer) was Nak'ed with %s - Allowing\n",
			       (char *)inet_ntoa(ip_addr));
			/*
			 * If the address the peer has provided is
			 * unavailable ... then re-try the configured value
			 */
			if (ipcp_checkaddr(ipcp, ip_addr, IP_LOCAL)) {
				psm_log(MSG_INFO_MED, ph,
				       "Local Address provided is in use - retry configured value.\n");
				ip_addr = ipcp->ip_cfg_local;
			}
			break;

		case IPADDR_ANY:
			psm_log(MSG_INFO_MED, ph,
		       "Local Address (Any) was Nak'ed with %s - Allowing\n",
			       (char *)inet_ntoa(ip_addr));

			break;

		case IPADDR_POOL:
			psm_log(MSG_INFO_MED, ph,
				 "Local address was Nak'ed with %s\n",
				 (char *)inet_ntoa(ip_addr));

			if (!ipcp->ip_aasconn) {
				psm_log(MSG_WARN, ph,
	"Failed to allocate address - not connected to Address Server\n");
				break;
			}

			/* Free the old address */
			ipcp_free_addr(ph, IP_LOCAL);

			/* Try for the new address */
			req_addr.addr = (void *)&ip_addr.s_addr;
			req_addr.len = sizeof(ip_addr.s_addr);
			alloc_addr.addr = (void *)&new_addr.s_addr;
			alloc_addr.len = sizeof(new_addr.s_addr);

			ret = aas_alloc(ipcp->ip_aasconn,
					ucfg_str(&cp->ip_ch,
						 cp->ip_local_addr),
					AAS_ATYPE_INET,
					&req_addr, NULL, NULL, 0, -1,
					IPCP_AAS_SERVICE,
					&ipcp->ip_local_cid, &alloc_addr);

			if (ret) {
				psm_log(MSG_WARN, ph,
				 "Failed to allocate Nak'ed address %d\n",
					 ret);
/* WHAT NOW ??? */			
				break;
			}

			if (*(int *)alloc_addr.addr != ip_addr.s_addr) {
				psm_log(MSG_DEBUG, ph,
				 "Didn't get requested address from pool.\n");
				ip_addr.s_addr = *(int *)alloc_addr.addr;
			}

			psm_log(MSG_INFO_MED, ph,
				 "AAS allocated local address %s\n",
				 (char *)inet_ntoa(ip_addr));
			break;
		}
		break;

	case CFG_ACK:

		ip_addr = ipcp->ip_cfg_local;
		psm_log(MSG_INFO_MED, ph, "Request Local Address %s%s\n",
		       (char *)inet_ntoa(ip_addr),
		       ip_addr.s_addr == 0 ? " (Peer will provide)" : "");
		break;
	}

        addr->h.co_type = IP_ADDRESS;
	addr->h.co_len = 6;
	db->db_wptr += sizeof(struct co_address_s);

	addr->co_addr = ip_addr.s_addr;
	ipcp->ip_local_addr = ip_addr;

	if (ipcp_checkaddr(ipcp, ip_addr, IP_LOCAL)) {
		psm_log(MSG_WARN, ph,
   "Requesting local address that conflicts with existing interface\n");
	}
}

STATIC struct in_addr
snd_dns_common(char *desc, struct in_addr *ip_addr, int code,
	       proto_hdr_t *ph, int state, struct co_s *co, db_t *db)
{
	struct co_dns_s *dns = (struct co_dns_s *)db->db_wptr;
	struct cfg_ipcp *cp = (struct cfg_ipcp *)ph->ph_cfg;
	struct ipcp_s *ipcp = (struct ipcp_s *)ph->ph_priv;

	dns->h.co_type = code;
	dns->h.co_len = 6;

	/* Get the advertised address */
	switch(state) {
	case CFG_REJ:
		psm_log(MSG_INFO_MED, ph, "%s Option was rejected\n", desc);
		return;

	case CFG_NAK:
		ip_addr->s_addr = ((struct co_dns_s *)co)->co_addr;
		psm_log(MSG_INFO_MED, ph, "%s Option was Nak'ed with %s\n",
			 desc, (char *)inet_ntoa(*ip_addr));
		break;

	case CFG_ACK:

		/* We are configured to get this */
		if (!cp->ip_getdns) {
			psm_log(MSG_INFO_MED, ph,
				 "Skip Getting %s Address (not cfg'ed)\n",
				 desc);
			return;
		}

		if (ip_addr->s_addr == 0xffffffff) {
			psm_log(MSG_INFO_MED, ph,
				 "Skip Getting %s Address (was rejected)\n",
				 desc);
			return;
		}

		psm_log(MSG_INFO_MED, ph, "Request %s Address %s\n",
			 desc, (char *)inet_ntoa(*ip_addr));

		db->db_wptr += sizeof(struct co_dns_s);
		/*
		 * An invalid address should cause the peer to NAK
		 * with a valid address
		 */
		dns->co_addr = ip_addr->s_addr;
		break;
	}
}

STATIC int
snd_dns(proto_hdr_t *ph, int state, struct co_s *co, db_t *db)
{
	struct ipcp_s *ipcp = (struct ipcp_s *)ph->ph_priv;

	snd_dns_common("DNS", &ipcp->ip_dns_addr, IP_DNS, ph, state, co, db);
}

STATIC int
snd_dns2(proto_hdr_t *ph, int state, struct co_s *co, db_t *db)
{
	struct ipcp_s *ipcp = (struct ipcp_s *)ph->ph_priv;

	snd_dns_common("Secondary DNS", &ipcp->ip_dns2_addr, IP_DNS2,
					   ph, state, co, db);
}


STATIC int
ipcp_load()
{
	int ret;
	AasConnection *aas_conn;

	psm_log(MSG_INFO, 0, "IPCP Loaded\n");

	/*
	 * Ensure we have a channel to the IP driver ... used to peform
	 * ioctls when enquiring about interfaces
	 */

	ipfd = open(DEV_IP, O_RDWR | O_NDELAY);
	if (ipfd < 0) {
		psm_log(MSG_WARN, 0, "IPCP Load - Failed to open '%s'\n",
			DEV_IP);
		return errno;
	}

	/* Initialise arp code */

	arp_init();

	/* Assume AAS is available until we know otherwise */
	aas_available = 1;

	/* Read any ipcp system config (address of aas) */

	aas_server.server.addr = (void*)&aas_addr;
	aas_server.server.len = 4;
	aas_server.addr_type = AF_UNIX;
	aas_server.password = aas_pwd;
	aas_server.password_len = AAS_PASSWDLEN;

	ret = aas_get_server(AAS_CFGFILE, &aas_server);
	if (ret == AAS_FAILURE) {
		psm_log(MSG_WARN, 0,
		       "IPCP Failed to get AAS address %d %d\n",
			aas_errno, errno);
		aas_available = 0;
		return -1;
	}

	ret = aas_open(&aas_server, AAS_MODE_BLOCK, &aas_conn);
	if (ret == AAS_FAILURE) {
		psm_log(MSG_WARN, 0,
	       "IPCP Could not connect to Address Allocation Server. Address Pooling will not be available.\n");
		/*
		 * We can live with this failure ..
		 * may be the assd isn't running
		 */
		aas_available = 0;
		return 0;
	}

	ret = aas_free_all(aas_conn, "", IPCP_AAS_SERVICE);
	if (ret == AAS_FAILURE) {
		psm_log(MSG_WARN, 0,
	       "IPCP Address Allocation Server Failed to free all.\n");

		/*
		 * We can live with this failure ..
		 */
	}
	aas_close(aas_conn);
	return 0;
}

STATIC int
ipcp_unload()
{
	int addr, ret;
	AasConnection *aas_conn;

	if (aas_available) {
		/*
		 * Connect to the Address Allocation Server ... free
		 * all addresses for IPCP
		 */
		ret = aas_open(&aas_server, AAS_MODE_BLOCK, &aas_conn);
		if (ret == AAS_FAILURE) {
			psm_log(MSG_WARN, 0,
		"IPCP Could not connect to Address Allocation Server.\n");
			goto exit;
		}

		ret = aas_free_all(aas_conn, "", IPCP_AAS_SERVICE);

		if (ret == AAS_FAILURE) {
			psm_log(MSG_WARN, 0,
		"IPCP Address Allocation Server Failed to free all.\n");
		}
		aas_close(aas_conn);
	}

 exit:
	psm_log(MSG_INFO_LOW, 0, "IPCP Unloaded\n");
}

STATIC int
ipcp_alloc(struct proto_hdr_s *ph)
{
	int addr, ret;
	struct ipcp_s *ipcp;
	char id[MAXID+1];

	ph->ph_priv = (void *)malloc(sizeof(struct ipcp_s));
	if (!ph->ph_priv)
		return ENOMEM;
	ipcp = (struct ipcp_s *)ph->ph_priv;
	ipcp->ip_old_local.s_addr = 0;
	ipcp->ip_old_peer.s_addr = 0;
	ipcp->ip_dns_addr.s_addr = 0;
	ipcp->ip_dns2_addr.s_addr = 0;
	ipcp->ip_aasconn = NULL;
	ipcp->ip_reason = 0;

	if (aas_available) {
		/*
	 * Connect to the Address Allocation Server incase 
	 * we need to use pools
	 */
		ret = aas_open(&aas_server, AAS_MODE_BLOCK, &ipcp->ip_aasconn);
		if (ret == AAS_FAILURE) {
			psm_log(MSG_WARN, 0,
				"IPCP Could not connect to Address Allocation Server.\n");
			ipcp->ip_aasconn = NULL;
		}
	} else 
		ipcp->ip_aasconn = NULL;

	/*
	 * Allocate client id's
	 */
	ipcp->ip_local_cid.len = sizeof(AasClientId);
	ipcp->ip_local_cid.id = &ipcp->ip_local_cid;
	ipcp->ip_peer_cid.len = sizeof(AasClientId);
	ipcp->ip_peer_cid.id = &ipcp->ip_peer_cid;
	return 0;
}

STATIC int
ipcp_free(struct proto_hdr_s *ph)
{
	struct ipcp_s *ipcp = (struct ipcp_s *)ph->ph_priv;

	ipcp_free_addr(ph, IP_LOCAL);
	ipcp_free_addr(ph, IP_PEER);

	if (ipcp->ip_aasconn)
		aas_close(ipcp->ip_aasconn);

	free(ph->ph_priv);
	ph->ph_priv = NULL;
}

STATIC int
ipcp_decfg(struct proto_hdr_s *ph)
{
	struct ipcp_s *ip = (struct ipcp_s *)ph->ph_priv;
	struct cfg_ipcp *cp = (struct cfg_ipcp *)ph->ph_cfg;
	
	psm_log(MSG_INFO_LOW, ph, "Removing interface %s\n", ip->ip_ifname);

	if (cp->ip_proxyarp && ip->ip_peer_addr.s_addr != 0)
		ipcp_proxydel(ip->ip_peer_addr.s_addr);

	if (ip->ip_fd >= 0) {
		if (ioctl(ip->ip_fd, I_UNLINK, ip->ip_mux) < 0) {
			psm_log(MSG_WARN, ph,
				"Could not unlink PPP from IP (%d)\n",
				errno);
		}
		close(ip->ip_fd);
	}

	ip->ip_fd = -1;
	ip->ip_mux = -1;

	ipcp_exec(ph, IPCPIF_DEL);
	return 0;
}

/*
 * Tell the kernel about bringup/keepup/pass(in/out) filters
 */
STATIC int
ipcp_pushfilters(struct proto_hdr_s *ph, int fd, int type, char *fname)
{
	struct filter f;
	int blen;
	struct bpf_program b;
	struct ipcpioc_sf_s *s;

	f.tag = fname;

	psm_log(MSG_DEBUG, ph, "ipcp_pushfilter: getfilter %s, %s\n",
		FILTER_FILE, fname);

	if (getfilter(&f, FILTER_FILE, fname) < 0) {
		psm_log(MSG_WARN, ph,
			"ipcp_pushfilter: tag %s - getfilter set errno %d\n",
			fname, errno);
		return -1;
	}

	if (compilefilter(&b, FILTER_PROTOCOL,
			  &f, FILTER_FRAMETYPE, FILTER_NETMASK) < 0) {
		psm_log(MSG_WARN, ph,
		"ipcp_pushfilter: tag %s - compilefilter set errno %d\n",
			fname, errno);
		return -1;
	}
	
	psm_log(MSG_DEBUG, ph, "ipcp_pushfilter: Compiled %s\n", fname);

	/*
	 * Pass the compiled filter to the kernel
	 * First create the ioctl message
	 */
	blen = sizeof(struct bpf_insn) * b.bf_len;
	s = (struct ipcpioc_sf_s *)malloc(sizeof(struct ipcpioc_sf_s) + blen);
	if (!s) {
		free(b.bf_insns);
		psm_log(MSG_WARN, ph, "No memory.");
		return -1;
	}

	s->ipf_type = type;
	s->ipf_flen = b.bf_len;
	memcpy(s->ipf_filter, b.bf_insns, blen);

#ifdef DEBUG_FILTER
	psm_log(MSG_DEBUG, ph, "ipcp_pushfilter Filter, %d bytes ...\n", blen);
	psm_loghex(MSG_DEBUG, ph, (char *)s->ipf_filter, blen);
#endif


	if (sioctl(fd, IPCPIOC_SETFILTER, (caddr_t)s, sizeof(struct ipcpioc_sf_s) + blen) < 0){
		psm_log(MSG_ERROR, ph,
			"IPCPIOC_SETFILTER failed, %d\n", errno);
		free(b.bf_insns);
		free(s);
		return -1;
	}

	free(b.bf_insns);
	free(s);
	return 0;
}

STATIC int
ipcp_cfg(struct proto_hdr_s *ph)
{
	struct ipcp_s *ipcp = (struct ipcp_s *)ph->ph_priv;
	struct cfg_ipcp *cp = (struct cfg_ipcp *)ph->ph_cfg;
	struct ppp_bind_s *pb;
	struct strioctl ioc;
	struct ifreq ifr;
	struct sockaddr_in sockaddr;
	struct ipcpioc_s ipcpioc;
	struct ifaliasreq ifra;
	int pppfd, ipmux, run_exec = 0;
	char *nmask;

	/*
	 * Perform some sanity checking on the interface addresses
	 */
	psm_log(MSG_INFO_LOW, ph, "Configuring interface %s\n",
		 ipcp->ip_ifname);

	if (!ipcp->ip_local_addr.s_addr) {
		psm_log(MSG_WARN, ph, "Failed. No local address configured\n");
		ipcp->ip_reason |= IPR_NOLOCAL;
		return EINVAL;
	}

	if (!ipcp->ip_peer_addr.s_addr) {
		psm_log(MSG_WARN, ph, "Failed. No peer address configured\n");
		ipcp->ip_reason |= IPR_NOPEER;
		return EINVAL;
	}

	if (ipcp->ip_local_addr.s_addr == ipcp->ip_peer_addr.s_addr) {
		psm_log(MSG_WARN, ph,
			 "Failed. Local and Peer addresses are the same\n");
		ipcp->ip_reason |= IPR_SAME;
		return EINVAL;
	}

	/* Get and check the allocate local/peer addresses */

	if (ipcp_checkaddr(ipcp, ipcp->ip_local_addr, IP_LOCAL)) {
		psm_log(MSG_WARN, ph,
		 "Local address '%s' conflicts with existing interface\n",
			 (char *)inet_ntoa(ipcp->ip_local_addr));
		ipcp->ip_reason |= IPR_LCONFLICT;
		return EINVAL;
	}

	if (ipcp_checkaddr(ipcp, ipcp->ip_peer_addr, IP_PEER)) {
		psm_log(MSG_WARN, ph,
		 "Peer address '%s' conflicts with existing interface\n",
			 (char *)inet_ntoa(ipcp->ip_peer_addr));
		ipcp->ip_reason |= IPR_PCONFLICT;
		return EINVAL;
	}

	/* Build the upper stack ... then confgure it */

	strcpy(ifr.ifr_name, ipcp->ip_ifname);

	if (ipcp->ip_fd < 0) {
		char *fname;

		pppfd = psm_bind(ph);
		if (pppfd < 0) {
			psm_log(MSG_ERROR, ph, "Failed to psm_bind()\n");
			ipcp->ip_reason |= IPR_DEV;
			return ENODEV;
		}

		/* Push the NCP support module */

		if (ioctl(pppfd, I_PUSH, PPPIP) < 0) {
			psm_log(MSG_ERROR, ph, "Failed to push pppip\n");
			close(pppfd);
			ipcp->ip_reason |= IPR_DEV;
			return ENODEV;
		}

		psm_ncp_up(ph);

		/* Push any defined packet filters down to the ipcp module */
		fname = ucfg_str(&cp->ip_ch, cp->ip_bringup);
		if (*fname &&
		    ipcp_pushfilters(ph, pppfd, IPF_BRINGUP,fname) < 0) {
			ipcp->ip_reason |= IPR_FILTER;
			return ENODEV;
		}

		fname = ucfg_str(&cp->ip_ch, cp->ip_keepup);
		if (*fname &&
		    ipcp_pushfilters(ph, pppfd, IPF_KEEPUP, fname) < 0) {
			ipcp->ip_reason |= IPR_FILTER;
			return ENODEV;
		}

		fname = ucfg_str(&cp->ip_ch, cp->ip_passin);
		if (*fname &&
		    ipcp_pushfilters(ph, pppfd, IPF_PASSIN, fname) < 0) {
			ipcp->ip_reason |= IPR_FILTER;
			return ENODEV;
		}

		fname = ucfg_str(&cp->ip_ch, cp->ip_passout);
		if (*fname && 
		    ipcp_pushfilters(ph, pppfd, IPF_PASSOUT, fname) < 0) {
			ipcp->ip_reason |= IPR_FILTER;
			return ENODEV;
		}

		/* Open ip */

		ipcp->ip_fd = open(DEV_IP, O_RDWR | O_NDELAY);
		if (ipcp->ip_fd < 0) {
			psm_log(MSG_ERROR, ph, "Failed to open %s\n", DEV_IP);
			close(pppfd);
			ipcp->ip_reason |= IPR_DEV;
			return ENODEV;
		}

		/* Link IP over PPP */

		ipcp->ip_mux = ioctl(ipcp->ip_fd, I_LINK, pppfd);
		if (ipcp->ip_mux < 0) {
			close(pppfd);
			psm_log(MSG_ERROR, ph,
				"I_LINK of ppp below ip failed\n");
			ipcp->ip_reason |= IPR_DEV;
			return ENODEV;
		}

		close(pppfd);

		/* Create an interface */

		ifr.ifr_metric = ipcp->ip_mux;
		ifr.ifr_metric |= (IFF_POINTOPOINT | IFF_RUNNING |
			IFF_WANTIOCTLS) << 16;

		if (sioctl(ipcp->ip_fd, SIOCSIFNAME, (caddr_t)&ifr, sizeof(ifr)) < 0) {
			psm_log(MSG_ERROR, ph,
				"SIOCSIFNAME '%s' failed, %d.\n",
				ipcp->ip_ifname, errno);
			ipcp->ip_reason |= IPR_INF;
			goto unlink_exit;
		}

		ifr.ifr_metric = IFT_PPP;
		if (sioctl(ipcp->ip_fd, SIOCSIFTYPE, (caddr_t)&ifr, sizeof(ifr)) < 0) {
			psm_log(MSG_ERROR, ph,
				"SIOCSIFTYPE '%s' failed, %d.\n",
				ipcp->ip_ifname, errno);
			ipcp->ip_reason |= IPR_INF;
			goto unlink_exit;
		}

		run_exec = 1;
	} else
		psm_ncp_up(ph);

	/* Set the addresses */

	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = 0;

	/* set netmask */
	nmask =ucfg_str(&cp->ip_ch, cp->ip_netmask);

	if (*nmask) {
		sockaddr.sin_addr.s_addr = inet_addr(nmask);
		if (sockaddr.sin_addr.s_addr == 0)
			sockaddr.sin_addr.s_addr = 0xffffffff;
	} else 
		sockaddr.sin_addr.s_addr = 0xffffffff;

	memcpy(&ifr.ifr_addr, &sockaddr, sizeof(struct sockaddr_in));
	if (sioctl(ipcp->ip_fd, SIOCSIFNETMASK, (caddr_t)&ifr, sizeof(ifr)) < 0) {
		psm_log(MSG_ERROR, ph,
			"Interface %s - Failed to set netmask, %d\n",
			ifr.ifr_name, errno);
		ipcp->ip_reason |= IPR_INF;
		goto unlink_exit;
	}

	/* Set source address */

	sockaddr.sin_addr = ipcp->ip_local_addr;
	memcpy(&ifr.ifr_addr, &sockaddr, sizeof(struct sockaddr_in));
	if (sioctl(ipcp->ip_fd, SIOCSIFADDR, (caddr_t)&ifr, sizeof(ifr)) < 0) {
		psm_log(MSG_ERROR, ph,
			"Interface %s - Failed to set local addr, %d\n",
			ifr.ifr_name, errno);
		ipcp->ip_reason |= IPR_INF;
		goto unlink_exit;
	}

	/* Set the destination address */

	sockaddr.sin_addr = ipcp->ip_peer_addr;
	memcpy(&ifr.ifr_dstaddr, &sockaddr, sizeof(struct sockaddr_in));	 
	if (sioctl(ipcp->ip_fd, SIOCSIFDSTADDR, (caddr_t)&ifr, sizeof(ifr)) < 0) {
		psm_log(MSG_ERROR, ph,
			"Interface %s - Failed to set peer addr, %d\n",
			ifr.ifr_name, errno);
		ipcp->ip_reason |= IPR_INF;
		goto unlink_exit;
	}

	/* Set debug to configured value */

	ifr.ifr_metric = cp->ip_debug;

	if (sioctl(ipcp->ip_fd, SIOCSIFDEBUG, (caddr_t)&ifr, sizeof(ifr)) < 0) {
		psm_log(MSG_ERROR, ph, "SIOCSIFDEBUG failed, %d.\n", errno);
		ipcp->ip_reason |= IPR_INF;
		goto unlink_exit;
	}	
	
	/* Inform the ipcp module of the configration */

	strcpy(ipcpioc.ip_ifname, ipcp->ip_ifname);

	ipcpioc.ip_local_compress = ipcp->ip_local_compress;
	ipcpioc.ip_peer_compress = ipcp->ip_peer_compress;
	ipcpioc.ip_local_slotmax = ipcp->ip_local_slotmax;
	ipcpioc.ip_peer_slotmax = ipcp->ip_peer_slotmax;

	psm_log(MSG_DEBUG, ph,
		 "Configure - Local Compress %d, Slot Max %d\n",
		 ipcpioc.ip_local_compress, ipcpioc.ip_local_slotmax);

	psm_log(MSG_DEBUG, ph,
		 "Configure - Peer Compress %d, Slot Max %d\n",
		 ipcpioc.ip_peer_compress, ipcpioc.ip_peer_slotmax);

	if (sioctl(ipcp->ip_fd, IPCPIOC_CFG,
		   (caddr_t)&ipcpioc, sizeof(ipcpioc)) < 0){
		psm_log(MSG_ERROR, ph, "IPCPIOC_CFG failed, %d\n", errno);
		goto unlink_exit;
	}

	/* If proxy arp is required ... at it now */

	if (cp->ip_proxyarp &&
	    ipcp->ip_old_peer.s_addr != ipcp->ip_peer_addr.s_addr) {
		ipcp_proxyadd(ipcp);

		/* Remove the arp entry if we had one */

		if (ipcp->ip_old_peer.s_addr != 0)
			ipcp_proxydel(ipcp->ip_old_peer.s_addr);
	}

	if (run_exec)
		ipcp_exec(ph, IPCPIF_ADD);

	psm_log(MSG_DEBUG, ph, "ipcp_cfg: Complete.\n");
	return 0;

 unlink_exit:
	ipcp_decfg(ph);
	return ENODEV;
}

/*
 * This function is called when IPCP enters or leaves the OPENED state,
 * it runs a user defined program, and passes it several arguments on the 
 * command line:
 *	program [UP | DOWN] [interface] [local addr] [peer addr]
 */
#define MAXCMD 255
ipcp_exec(proto_hdr_t *ph, int action)
{
	struct ipcp_s *ipcp = (struct ipcp_s *)ph->ph_priv;
	struct cfg_ipcp *cp = (struct cfg_ipcp *)ph->ph_cfg;
	char cmdbuf[MAXCMD + 1];
	char *exec;
	int ret;

	exec = ucfg_str(&cp->ip_ch, cp->ip_exec);
	if (exec && *exec) {

		sprintf(cmdbuf, "%s %s %s %s ",
			exec, updown[action], ipcp->ip_ifname,
			(char *)inet_ntoa(ipcp->ip_local_addr));

		strcat(cmdbuf, (char *)inet_ntoa(ipcp->ip_peer_addr));
		strcat(cmdbuf, " ");
		strcat(cmdbuf, (char *)inet_ntoa(ipcp->ip_old_local));
		strcat(cmdbuf, " ");
		strcat(cmdbuf, (char *)inet_ntoa(ipcp->ip_old_peer));
		strcat(cmdbuf, " ");
		strcat(cmdbuf, (char *)inet_ntoa(ipcp->ip_dns_addr));
		strcat(cmdbuf, " ");
		strcat(cmdbuf, (char *)inet_ntoa(ipcp->ip_dns2_addr));

		strcat(cmdbuf, " ");
		if (cp->ip_defaultroute)
			strcat(cmdbuf, "default");
		else
			strcat(cmdbuf, "-");
			
		ipcp->ip_old_local = ipcp->ip_local_addr;
		ipcp->ip_old_peer = ipcp->ip_peer_addr;
		
		psm_log(MSG_INFO_MED, ph, "Program to Execute '%s'\n",
		       cmdbuf);

		ret = system(cmdbuf);

		if (ret)
			psm_log(MSG_WARN, ph,
				"Failed to execute '%s' - returned %d\n",
				cmdbuf, ret);
	}
}

/*
 *  This-Layer-Up (tlu)
 *
 *      This action indicates to the upper layers that the automaton is
 *      entering the Opened state.
 *
 *      Typically, this action is used by the LCP to signal the Up event
 *      to a NCP, Authentication Protocol, or Link Quality Protocol, or
 *      MAY be used by a NCP to indicate that the link is available for
 *      its network layer traffic.
 */
STATIC void
ipcp_up(proto_hdr_t *ph)
{
	struct ipcp_s *ipcp = (struct ipcp_s *)ph->ph_priv;
	struct cfg_ipcp *cp = (struct cfg_ipcp *)ph->ph_cfg;

	psm_log(MSG_INFO_LOW, ph, "Up\n");

	/* Convert ph_peer_opts to ip flags */
	if (!PH_TSTBIT(IP_COMP_PROTO, ph->ph_peer_opts))
		ipcp->ip_peer_compress = 0;
	if (!PH_TSTBIT(IP_ADDRESS, ph->ph_peer_opts))
		ipcp->ip_peer_addr.s_addr = 0;
	if (!PH_TSTBIT(IP_DNS, ph->ph_peer_opts))
		ipcp->ip_dns_addr.s_addr = 0;
	if (!PH_TSTBIT(IP_DNS2, ph->ph_peer_opts))
		ipcp->ip_dns_addr.s_addr = 0;

	psm_log(MSG_INFO_LOW, ph, "Local address %s\n",
		       (char *)inet_ntoa(ipcp->ip_local_addr));
	psm_log(MSG_INFO_LOW, ph, "Peer address %s\n",
		       (char *)inet_ntoa(ipcp->ip_peer_addr));

	if (ipcp->ip_local_compress & IPC_VJCOMPRESS) {
		psm_log(MSG_INFO_LOW, ph, "Local VJ Compression Enabled\n");
		psm_log(MSG_INFO_LOW, ph,
			 "    Max-Slot-Id %d, %sSlot Compression\n",
			 ipcp->ip_local_slotmax,
			 (ipcp->ip_local_compress & IPC_VJSLOTCOMP) ?
			 "" : "No ");
	} else
		psm_log(MSG_INFO_LOW, ph, "Local VJ Compression Disabled\n");

	if (ipcp->ip_peer_compress & IPC_VJCOMPRESS) {
		psm_log(MSG_INFO_LOW, ph, "Peer VJ Compression Enabled\n");
		psm_log(MSG_INFO_LOW, ph,
			 "    Max-Slot-Id %d, %sSlot Compression\n",
			 ipcp->ip_peer_slotmax,
			 (ipcp->ip_peer_compress & IPC_VJSLOTCOMP) ?
			 "" : "No ");
	} else
		psm_log(MSG_INFO_LOW, ph, "Peer VJ Compression Disabled\n");


	psm_log(MSG_INFO_LOW, ph, "Primary DNS Address %s\n",
		 ipcp->ip_dns_addr.s_addr ?
		 (char *)inet_ntoa(ipcp->ip_dns_addr) : "(Not obtained)");

	psm_log(MSG_INFO_LOW, ph, "Secondary DNS Address %s\n",
		 ipcp->ip_dns2_addr.s_addr ?
		 (char *)inet_ntoa(ipcp->ip_dns2_addr) : "(Not obtained)");

	if (ipcp_cfg(ph)) {
		fsm_state(ph, CLOSE, 0);
		return;
	}

	ipcp_exec(ph, IPCPIF_UP);
	psm_ncp_open(ph);
}

/*
 * This function is called to initialise the IPCP parameters
 * Called when IPCP is started or restarted.
 */
STATIC void
ipcp_protoinit(proto_hdr_t *ph)
{
	struct cfg_ipcp *cp = (struct cfg_ipcp *)ph->ph_cfg;
	struct ipcp_s *ip = (struct ipcp_s *)ph->ph_priv;

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	psm_log(MSG_DEBUG, ph, "Init Protocol\n");

	/* Load any per open cfg */
	ip->ip_reason = 0;
	ip->ip_local_compress = 0;
	ip->ip_peer_compress = 0;

	if (cp->ip_vjcomp)
		ip->ip_local_compress |= IPC_VJCOMPRESS;
	if (cp->ip_vjslotcomp)
		ip->ip_local_compress |= IPC_VJSLOTCOMP;

	ip->ip_local_slotmax = cp->ip_vjslotmax;

	/* Initialise DNS Stuff */
	ipcp_getdns(ph, ip, cp);
	ip->ip_dns_addr.s_addr = 0;
	ip->ip_dns2_addr.s_addr = 0;

	fsm_load_counters(ph);
	PH_RESETBITS(ph->ph_rej_opts);
}

/*
 *   This-Layer-Down (tld)
 *
 *      This action indicates to the upper layers that the automaton is
 *      leaving the Opened state.
 *
 *      Typically, this action is used by the LCP to signal the Down event
 *      to a NCP, Authentication Protocol, or Link Quality Protocol, or
 *      MAY be used by a NCP to indicate that the link is no longer
 *      available for its network layer traffic.
 */
STATIC void
ipcp_down(proto_hdr_t *ph)
{
	struct cfg_ipcp *cp = (struct cfg_ipcp *)ph->ph_cfg;
	struct ipcp_s *ip = (struct ipcp_s *)ph->ph_priv;

	psm_log(MSG_INFO_LOW, ph, "Down\n");

	ipcp_exec(ph, IPCPIF_DOWN);
	psm_ncp_down(ph);
	if (!(ph->ph_flags & PHF_AUTO))
		ipcp_decfg(ph);

	if (ph->ph_state.ps_state == REQSENT ||
	    ph->ph_state.ps_state == ACKSENT) {
		psm_log(MSG_DEBUG, ph, "Re-starting .. reset counters\n");
		ipcp_protoinit(ph);
	}
}

/*
 *  This-Layer-Started (tls)
 *
 *      This action indicates to the lower layers that the automaton is
 *      entering the Starting state, and the lower layer is needed for the
 *      link.  The lower layer SHOULD respond with an Up event when the
 *      lower layer is available.
 *
 *      This results of this action are highly implementation dependent.
 */
STATIC void
ipcp_start(proto_hdr_t *ph)
{
	struct cfg_ipcp *cp = (struct cfg_ipcp *)ph->ph_cfg;
	struct ipcp_s *ip = (struct ipcp_s *)ph->ph_priv;

	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	psm_log(MSG_INFO_LOW, ph, "Starting\n");

	ipcp_protoinit(ph);
	PH_RESETBITS(ph->ph_cod_rej);
	fsm_state(ph, UP, 0);
}

/*
 *   This-Layer-Finished (tlf)
 *
 *      This action indicates to the lower layers that the automaton is
 *      entering the Initial, Closed or Stopped states, and the lower
 *      layer is no longer needed for the link.  The lower layer SHOULD
 *      respond with a Down event when the lower layer has terminated.
 *
 *      Typically, this action MAY be used by the LCP to advance to the
 *      Link Dead phase, or MAY be used by a NCP to indicate to the LCP
 *      that the link may terminate when there are no other NCPs open.
 *
 *      This results of this action are highly implementation dependent.
 */
STATIC void
ipcp_finish(proto_hdr_t *ph)
{
	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	psm_log(MSG_INFO_LOW, ph, "Finished\n");
}

/*
 * Adminitrative UP when not in CLOSED or INITIAL
 */
STATIC void
ipcp_restart(proto_hdr_t *ph)
{
	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));

	psm_log(MSG_INFO_LOW, ph, "Restart\n");

	/* Down -> Up */
	fsm_state(ph, DOWN, 0);
	fsm_state(ph, UP, 0);
}

STATIC void
ipcp_crossed(proto_hdr_t *ph)
{
	ASSERT(MUTEX_LOCKED(&ph->ph_parent->ah_mutex));
	psm_log(MSG_ERROR, ph, "ipcp_crossed:\n");
	abort(0);
}

STATIC int
ipcp_init(struct act_hdr_s *ah, struct proto_hdr_s *ph)
{
	struct cfg_ipcp *cp = (struct cfg_ipcp *)ph->ph_cfg;
	struct ipcp_s *ip = (struct ipcp_s *)ph->ph_priv;
	/*char ifname[IFNAME_MAXLEN];*/

	ASSERT(ah->ah_type == DEF_BUNDLE);
	ASSERT(ipfd >= 0);

	ip->ip_fd = -1;

	/* Generate an interface name */
	sprintf(ip->ip_ifname, "ppp%d", bundle_number(ph->ph_parent));

	/* Get and check the configured local address */
	ip->ip_cfg_local.s_addr = ipcp_getaddr(ip, ucfg_str(&cp->ip_ch,
							cp->ip_local_addr),
					       cp->ip_local_addropt, IP_LOCAL);
	if (ip->ip_cfg_local.s_addr == -1) {
		/*
		 * Use another interfaces interface's ip addr
		 */
		ip->ip_cfg_local.s_addr = ipcp_getifaddr();
		
		psm_log(MSG_WARN, ph,
			 "Local address '%s' is invalid, using %s\n",
			 ucfg_str(&cp->ip_ch, cp->ip_local_addr),
			(char *)inet_ntoa(ip->ip_cfg_local));
	}

	if (ip->ip_cfg_local.s_addr == 0) {
		/*
		 * Use another interfaces interface's ip addr
		 */
		ip->ip_cfg_local.s_addr = ipcp_getifaddr();
		
		psm_log(MSG_WARN, ph,
			 "Local address not configured using %s\n",
			(char *)inet_ntoa(ip->ip_cfg_local));
	}

	if (ipcp_checkaddr(ip, ip->ip_cfg_local, IP_LOCAL)) {
		/*
		 * Use another interfaces interface's ip addr
		 */
		ip->ip_cfg_local.s_addr = ipcp_getifaddr();
		psm_log(MSG_WARN, ph,
			 "Local address '%s' conflicts, using %s\n",
			 ucfg_str(&cp->ip_ch, cp->ip_local_addr),
			(char *)inet_ntoa(ip->ip_cfg_local));
	}

	ip->ip_local_addr = ip->ip_cfg_local;

	/* Get and check the configured peer address */
	ip->ip_cfg_peer.s_addr = ipcp_getaddr(ip, ucfg_str(&cp->ip_ch,
						       cp->ip_peer_addr),
					      cp->ip_peer_addropt, IP_PEER);
	if (ip->ip_cfg_peer.s_addr == -1) {
		psm_log(MSG_WARN, ph,
			 "Peer address '%s' is invalid, using 0.0.0.0\n",
			 ucfg_str(&cp->ip_ch, cp->ip_peer_addr));
		ip->ip_cfg_peer.s_addr = 0;
	}

	if (ipcp_checkaddr(ip, ip->ip_cfg_peer, IP_PEER)) {
		psm_log(MSG_WARN, ph,
			 "Peer address '%s' conflicts, using 0.0.0.0\n",
			 ucfg_str(&cp->ip_ch, cp->ip_peer_addr));
		ip->ip_cfg_peer.s_addr = 0;
	}

	ip->ip_peer_addr = ip->ip_cfg_peer;

	/* Initialise the compression values */

	ip->ip_peer_compress = 0;
	ip->ip_peer_slotmax = 0;
	ip->ip_local_compress = 0;
	ip->ip_local_slotmax = 0;

	ip->ip_reason = 0;

	/* Initialise DNS Stuff */
	ipcp_getdns(ph, ip, cp);
	ip->ip_dns_addr.s_addr = 0;
	ip->ip_dns2_addr.s_addr = 0;

	fsm_init(ph);

	return 0;
}

STATIC int
ipcp_rcv(proto_hdr_t *ph, db_t *db)
{
	if (ph->ph_parent->ah_phase == PHASE_DEAD) {
		psm_log(MSG_WARN, ph,
			 "Got packet before NETWORK phase reached\n");
		return -1;
	}
	return psm_rcv(ph, db);
}

STATIC int
ipcp_snd(proto_hdr_t *ph, db_t *db)
{
	return psm_snd(ph, db);
}

STATIC int
ipcp_log(proto_hdr_t *ph, db_t *db, int proto)
{

	switch(proto) {
	case PROTO_IP:
	case PROTO_IPCP:
		ip_log(ph, db->db_rptr, db->db_wptr - db->db_rptr);
		psm_log(MSG_INFO_MED, ph, "TCP packet\n");
		psm_loghex(MSG_INFO_MED, ph, (char *)db->db_base,
			  db->db_wptr - db->db_rptr);
		break;

	case PROTO_TCP_COMP:
		psm_log(MSG_INFO_MED, ph, "TCP COMPRESSED packet\n");
		psm_loghex(MSG_INFO_MED, ph, (char *)db->db_base,
			  db->db_wptr - db->db_rptr);
		break;

	case PROTO_TCP_UNCOMP:
		psm_log(MSG_INFO_MED, ph, "TCP UNCOMPRESSED packet\n");
		psm_loghex(MSG_INFO_MED, ph, (char *)db->db_base,
			  db->db_wptr - db->db_rptr);
		break;

	default:
		psm_log(MSG_WARN, ph, "Unknown protocol to log 0x%x\n", proto);
	}

	db_free(db);
}

STATIC int
ipcp_status(proto_hdr_t *ph, struct ipcp_status_s *st)
{
	st->st_fsm = ph->ph_state;
	st->st_ip = *(struct ipcp_s *)ph->ph_priv;
}

/*
 * IPCP Configuration options table
 */
STATIC struct psm_opt_tab_s ipcp_opt_tab[] = {

	/* Type 0 */
	OP_NOSUPP("Reserved"),

	/* Type 1 */
	OP_NOSUPP("IP Addresses (1172)"),

	/* Type 2 */

	rcv_comp,
	snd_comp,
	OP_LEN_MIN,
	4,
	"IP Compression",

	/* Type 3 */
	rcv_address,
	snd_address,
	OP_LEN_EXACT,
	6,
	"IP Address",

	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),

	/* 10 */
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	/* 20 */
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	/* 30 */
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	/* 40 */
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	/* 50  */
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	/* 60 */
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	/* 70 */
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	/* 80 */
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	/* 90 */
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	/* 100 */
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	/* 110 */
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	/* 120 */
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"), 	OP_NOSUPP("Reserved"),
	OP_NOSUPP("Reserved"),

	/* 129 */
	rcv_dns,
	snd_dns,
	OP_LEN_EXACT,
	6,
	"Primary DNS Address",

	/* 130 */
	OP_NOSUPP("NBS Server Address"),

	/* 131 */
	rcv_dns2,
	snd_dns2,
	OP_LEN_EXACT,
	6,
	"Secondary DNS Address",
	
	/* 132 */
	OP_NOSUPP("Secondary NBS Server Address"),
};

#define NUM_IPCP_OP 132
	

/*
 * List of protocols that are supported by IPCP
 */
STATIC ushort_t ipcp_protolist[] = {
	PROTO_IP,
	PROTO_TCP_COMP,
	PROTO_TCP_UNCOMP,
	0,
};

/*
 * Table of PSM entry points
 */
struct psm_tab_s psm_entry = {
	PSM_API_VERSION,
	"IPCP",
	PROTO_IPCP,
	PT_NCP | PT_FSM | PT_BUNDLE,	/* flags */
	PT_PRI_LOW,

	ipcp_protolist,

	ipcp_load,
	ipcp_unload,
	ipcp_alloc,
	ipcp_free,
	ipcp_init,

	ipcp_rcv,
	ipcp_snd,
	ipcp_log,
	ipcp_status,

	/* FSM specific fields .. */
	ipcp_up,
	ipcp_down,
	ipcp_start,
	ipcp_finish,
	ipcp_restart,
	ipcp_crossed,

	ipcp_opt_tab,
	NUM_IPCP_OP,
	NULL,		/* codes */

	NULL,		/* k2d */

	ipcp_cfg,
	ipcp_decfg,
};
