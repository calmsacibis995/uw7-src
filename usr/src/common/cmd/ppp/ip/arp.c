#ident	"@(#)arp.c	1.2"

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

#include "ppp_type.h"
#include "ppp_cfg.h"
#include "psm.h"
#include "act.h"
#include "lcp_hdr.h"
#include "ip_cfg.h"
#include "ppp_proto.h"

/* Should get these from somewhere else ! */
#define DEV_ROUTE "/dev/route"


int	s = -1;

/* READ ONLY ! */
struct sockaddr_in so_mask = {
	sizeof(struct sockaddr_in),
	AF_INET,
	0,
	{
		0xff, 0xff, 0xff, 0xff
	}
};

struct sockaddr_inarp blank_sin = {
	sizeof(struct sockaddr_inarp),
	AF_INET
};

struct sockaddr_inarp sin_m;

struct sockaddr_dl blank_sdl = {
	sizeof(struct sockaddr_dl),
	AF_LINK
};

struct sockaddr_dl sdl_m;

int	flags, export_only, doing_proxy;

struct	{
	struct	rt_msghdr m_rtm;
	char	m_space[512];
}	m_rtmsg;


/*
 * Set an individual arp entry
 *
 * 	cmd - the route command
 *	host - the address to arp for
 *	a - the mac address to advertise
 */
STATIC
setarp(u_long host, char *a)
{
	struct hostent *hp;
	struct sockaddr_inarp *sin = &sin_m;
	struct rt_msghdr *rtm = &(m_rtmsg.m_rtm);
	u_char *ea;
	int ret;

	s = open(DEV_ROUTE, O_RDWR, 0);
	if (s < 0) {
		psm_log(MSG_ERROR, 0,
			"IPCP setarp, failed to open %s\n", DEV_ROUTE);
		return -1;
	}
	(void) ioctl(s, I_SRDOPT, RMSGD);

	sdl_m = blank_sdl;
	sin_m = blank_sin;
	sin->sin_addr.s_addr = host;
	flags = 0;
	export_only = 1;

	ea = (u_char *)LLADDR(&sdl_m);
	bcopy(a, ea, 6);
	sdl_m.sdl_alen = 6;
	doing_proxy = SIN_PROXY;

	ret = rtmsg(RTM_ADD);
	close(s);
	return ret;
}

/*
 * Delete an arp entry 
 */
STATIC
deletearp(ulong host)
{
	register struct sockaddr_inarp *sin = &sin_m;
	register struct rt_msghdr *rtm = &m_rtmsg.m_rtm;
	struct sockaddr_dl *sdl;
	u_char *ea;
	char *eaddr;
	struct in_addr in;

	s = open(DEV_ROUTE, O_RDWR, 0);
	if (s < 0) {
		psm_log(MSG_ERROR, 0,
			"IPCP setarp, failed to open %s\n", DEV_ROUTE);
		return -1;
	}
	(void) ioctl(s, I_SRDOPT, RMSGD);

	sin_m = blank_sin;
	sin->sin_addr.s_addr = host;

tryagain:
	if (rtmsg(RTM_GET) < 0) {
		return (1);
	}
	sin = (struct sockaddr_inarp *)(rtm + 1);
	sdl = (struct sockaddr_dl *)(sizeof(*sin) + (char *)sin);
	if (sin->sin_addr.s_addr == sin_m.sin_addr.s_addr) {
		if (sdl->sdl_family == AF_LINK &&
		    (rtm->rtm_flags & RTF_LLINFO) &&
		    !(rtm->rtm_flags & RTF_GATEWAY)) switch (sdl->sdl_type) {
		case IFT_ETHER: case IFT_FDDI: case IFT_ISO88023:
		case IFT_ISO88024: case IFT_ISO88025:
			goto delete;
		}
	}
	if (sin_m.sin_other & SIN_PROXY) {
		in.s_addr = host;
		psm_log(MSG_WARN, 0, "Arp entry not found for '%s'\n",
			inet_ntoa(in));
		return (1);
	} else {
		sin_m.sin_other = SIN_PROXY;
		goto tryagain;
	}
delete:
	if (sdl->sdl_family != AF_LINK) {
		in.s_addr = host;
		psm_log(MSG_WARN, 0, "Arp entry not found for '%s'\n",
			inet_ntoa(in));
		close(s);
		return (1);
	}
	rtmsg(RTM_DELETE);
	close(s);
}

/*
 * return
 *    -1: fail
 *    0: succeed 
 *
 * cmd - one of RTM_ADD, RTM_GET or RTM_DELETE
 *
 */
STATIC int
rtmsg(int cmd)
{
	static int seq;
	int rlen;
	register struct rt_msghdr *rtm = &m_rtmsg.m_rtm;
	register char *cp = m_rtmsg.m_space;
	register int l;
	struct strioctl si;

	errno = 0;
	if (cmd == RTM_DELETE)
		goto doit; 
	bzero((char *)&m_rtmsg, sizeof(m_rtmsg));
	rtm->rtm_flags = flags;
	rtm->rtm_version = RTM_VERSION;

	switch (cmd) {
	default:
		psm_log(MSG_FATAL, 0, "IPCP rtmsg internal error.\n");
		/*NOTREACHED*/
		return -1;

	case RTM_ADD:
		rtm->rtm_addrs |= RTA_GATEWAY;
		rtm->rtm_rmx.rmx_expire = 0;
		rtm->rtm_inits = RTV_EXPIRE;
		rtm->rtm_flags |= (RTF_HOST | RTF_STATIC);
		sin_m.sin_other = 0;
		if (doing_proxy) {
			if (export_only)
				sin_m.sin_other = SIN_PROXY;
			else {
				rtm->rtm_addrs |= RTA_NETMASK;
				rtm->rtm_flags &= ~RTF_HOST;
			}
		}
	case RTM_GET:
		rtm->rtm_addrs |= RTA_DST;
	}
#define NEXTADDR(w, s) \
	if (rtm->rtm_addrs & (w)) { \
		bcopy((char *)&s, cp, sizeof(s)); cp += sizeof(s);}

	NEXTADDR(RTA_DST, sin_m);
	NEXTADDR(RTA_GATEWAY, sdl_m);
	NEXTADDR(RTA_NETMASK, so_mask);

	rtm->rtm_msglen = cp - (char *)&m_rtmsg;
doit:
	l = rtm->rtm_msglen;
	rtm->rtm_seq = ++seq;
	rtm->rtm_type = cmd;
	si.ic_cmd = RTSTR_SEND;
	si.ic_dp = (char *)&m_rtmsg;
	si.ic_len = l;
	si.ic_timout = 0;
	if ((rlen = ioctl(s, I_STR, (char *)&si)) < 0) {
		if (errno != ESRCH || cmd != RTM_DELETE) {
			psm_log(MSG_WARN, 0,
				"rtmsg: RTSTR_SEND failed %d\n", errno);
			return (-1);
		}
	}
	do {
		l = read(s, (char *)&m_rtmsg, sizeof(m_rtmsg));
	} while (l > 0 && (rtm->rtm_seq != seq));
	if (l < 0)
		psm_log(MSG_WARN, 0,
			"rtmsg: read from routing socket failed\n");
	return (0);
}

/*
 * Do a STREAMS ioctl call
 */
STATIC int
sioctl(fd, cmd, dp, len)
	int	fd, cmd, len;
	caddr_t	dp;
{
	struct strioctl	iocb;

	iocb.ic_cmd = cmd;
	iocb.ic_timout = 15;
	iocb.ic_dp = dp;
	iocb.ic_len = len;
	return ioctl(fd, I_STR, &iocb);
}

STATIC
bcopy(caddr_t f, caddr_t t, int l)
{
	memcpy(t, f, l);
}

STATIC
bzero(caddr_t a, int len)
{
	memset(a, 0, len);
}

static mutex_t arp_mutex;
/*
 * Initialise anything for Proxy ARP
 */
void
arp_init()
{
	mutex_init(&arp_mutex,USYNC_PROCESS, NULL);
}

/*
 * Add an arp entry with the specified characteristics
 */
int
arp_set(uint_t addr, caddr_t lladdr, int alen)
{
	int ret;

	/* take lock */
	MUTEX_LOCK(&arp_mutex);

	ret = setarp(addr, lladdr);

	/* Free lock */
	MUTEX_UNLOCK(&arp_mutex);
}

/*
 * Delete an arp entry with the specified address
 */
int
arp_del(uint_t addr)
{
	int ret;

	/* take lock */
	MUTEX_LOCK(&arp_mutex);

	deletearp(addr);

	/* Free lock */
	MUTEX_UNLOCK(&arp_mutex);
}

