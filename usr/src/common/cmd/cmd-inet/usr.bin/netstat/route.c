#ident "@(#)route.c	1.5"
#ident "$Header$"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/strstat.h>

#include <net/if.h>
#include <net/if_dl.h>
#define  INKERNEL
#include <net/route.h>
#undef INKERNEL
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip_var.h>

#include <netdb.h>

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <paths.h>
#include <errno.h>
#include "proto.h"

extern	int nflag, aflag, Aflag, af;
int do_rtent;
extern	char *malloc();
#define kget(p, d) \
	(readmem((off_t)(p), 1, 0, (char *)&(d), sizeof (d), "kget"))

/*
 * Definitions for showing gateway flags.
 */
struct bits {
	u_long	b_mask;
	char	b_val;
} bits[] = {
	{ RTF_UP,	'U' },
	{ RTF_GATEWAY,	'G' },
	{ RTF_HOST,	'H' },
	{ RTF_REJECT,	'R' },
	{ RTF_DYNAMIC,	'D' },
	{ RTF_MODIFIED,	'M' },
	{ RTF_MASK,	'm' },
	{ RTF_CLONING,	'C' },
	{ RTF_XRESOLVE,	'X' },
	{ RTF_LLINFO,	'L' },
	{ RTF_STATIC,	'S' },
	{ RTF_LOSING,	'-' },
	{ RTF_PMTU,	'P' },
	{ RTF_PMTUMOD,	'N' },
	{ 0 }
};

/*
 * Print routing tables.
 */
routepr(treeaddr)
	off_t treeaddr;
{
	mblk_t mb;
	register struct ortentry *rt;
	register mblk_t  *m;
	char name[16], *flags;
	mblk_t **routehash;
	int hashsize;
	int i, doinghost = 1;

	printf("Routing tables\n");
	if (Aflag)
		printf("%-8.8s ","Address");
	printf("%-16.16s %-18.18s %-6.6s %6.6s %8.8s  %s\n",
		"Destination", "Gateway",
		"Flags", "Refs", "Use", "Interface");
	if (treeaddr || !Aflag)
		return treestuff(treeaddr);
	return;
}

static union {
	struct	sockaddr u_sa;
	u_short	u_data[128];
} pt_u;
int do_rtent = 0;
struct rtentry rtentry;
struct radix_node rnode;
struct radix_mask rmask;

int NewTree = 1;
treestuff(rtree)
	off_t rtree;
{
	struct radix_node_head *rnh, head;

	if (Aflag == 0 && NewTree)
		return(ntreestuff());
	for (kget(rtree, rnh); rnh; rnh = head.rnh_next) {
		kget(rnh, head);
		if (head.rnh_af == 0) {
			if (Aflag || af == AF_UNSPEC) { 
				printf("Netmasks:\n");
				p_tree(head.rnh_treetop);
			}
		} else if (af == AF_UNSPEC || af == head.rnh_af) {
			printf("\nRoute Tree for Protocol Family %d:\n",
								head.rnh_af);
			do_rtent = 1;
			p_tree(head.rnh_treetop);
		}
	}
}

struct sockaddr *
kgetsa(dst)
	register struct sockaddr *dst;
{
	kget(dst, pt_u.u_sa);
	return (&pt_u.u_sa);
}

p_tree(rn)
	struct radix_node *rn;
{

again:
	kget(rn, rnode);
	if (rnode.rn_b < 0) {
		if (Aflag)
			printf("%-8.8x ", rn);
		if (rnode.rn_flags & RNF_ROOT)
			printf("(root node)%s",
				    rnode.rn_dupedkey ? " =>\n" : "\n");
		else if (do_rtent) {
			kget(rn, rtentry);
			p_rtentry(&rtentry);
			if (Aflag)
				p_rtnode();
		} else {
			p_sockaddr(kgetsa((struct sockaddr *)rnode.rn_key),
				   NULL, 0, 44);
			putchar('\n');
		}
		if (rn = rnode.rn_dupedkey)
			goto again;
	} else {
		if (Aflag && do_rtent) {
			printf("%-8.8x ", rn);
			p_rtnode();
		}
		rn = rnode.rn_r;
		p_tree(rnode.rn_l);
		p_tree(rn);
	}
}
char nbuf[20];

p_rtnode()
{

	struct radix_mask *rm = rnode.rn_mklist;
	if (rnode.rn_b < 0) {
		if (rnode.rn_mask) {
			printf("\t  mask ");
			p_sockaddr(kgetsa((struct sockaddr *)rnode.rn_mask),
				   NULL, 0, -1);
		} else if (rm == 0)
			return;
	} else {
		sprintf(nbuf, "(%d)", rnode.rn_b);
		printf("%6.6s %8.8x : %8.8x", nbuf, rnode.rn_l, rnode.rn_r);
	}
	while (rm) {
		kget(rm, rmask);
		sprintf(nbuf, " %u refs, ", rmask.rm_refs);
		printf(" mk = %8.8x {(%d),%s",
			rm, -1 - rmask.rm_b, rmask.rm_refs ? nbuf : " ");
		p_sockaddr(kgetsa((struct sockaddr *)rmask.rm_mask),
			   NULL, 0, -1);
		putchar('}');
		if (rm = rmask.rm_mklist)
			printf(" ->");
	}
	putchar('\n');
}

ntreestuff()
{
	char *buf, *next, *lim;
	register struct rt_msghdr *rtm;
	int fd;
	int r;
	struct rt_giarg gi_arg, *gp;

	fd = open(_PATH_ROUTE, O_RDONLY);
	if (fd < 0) { 
		fprintf(stderr,"netstat: ");
		perror(_PATH_ROUTE);
		exit(1);
	}

	gi_arg.gi_op = KINFO_RT_DUMP;
	gi_arg.gi_where = (caddr_t)0;
	gi_arg.gi_size = 0;
	gi_arg.gi_arg = 0;
	r = ioctl(fd, RTSTR_GETROUTE, &gi_arg);
	if (r < 0) {
		fprintf(stderr,"netstat: ");
		perror("GETROUTE 1"); 
		exit(1);
	}
	/* gi_size includes sizeof(gi_arg) */
	if ((buf = malloc(gi_arg.gi_size)) == 0)
		{ fprintf(stderr,"out of space\n"); exit(1);}
	gp = (struct rt_giarg *)buf;
	gp->gi_size = gi_arg.gi_size;
	gp->gi_op = KINFO_RT_DUMP;
	gp->gi_where = (caddr_t)buf;
	gp->gi_arg = 0;
	r = ioctl(fd, RTSTR_GETROUTE, buf);
	if (r < 0) { 
		fprintf(stderr,"netstat: ");
		perror("GETROUTE 2"); 
		exit(1);
	}
	lim = buf + gp->gi_size;
	buf += sizeof(gi_arg);
	for (next = buf; next < lim; next += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *)next;
		np_rtentry(rtm);
	}
}

static cur_ifindex = -1;
static char cur_ifname[IFNAMSIZ];

np_rtentry(rtm)
	register struct rt_msghdr *rtm;
{
	register struct sockaddr *sa = (struct sockaddr *)(rtm + 1);
	static int masks_done, old_af, banner_printed;
	int af = 0, interesting = RTF_REJECT | RTF_LOSING | RTF_DYNAMIC |
		RTF_UP | RTF_GATEWAY | RTF_HOST | RTF_CLONING |
		RTF_STATIC | RTF_LLINFO | RTF_MASK;
	
	
	if (aflag) {
		interesting |= RTF_MODIFIED | RTF_PMTU | RTF_PMTUMOD;
	}

#ifdef notdef
	if (!banner_printed) {
		printf("Netmasks:\n");
		banner_printed = 1;
	}
	if (masks_done == 0) {
		if (rtm->rtm_addrs != RTA_DST ) {
			masks_done = 1;
			af = sa->sa_family;
		}
	} else
#endif /* notdef */
		af = sa->sa_family;
	if (af != old_af) {
		old_af = af;
	}
	if ((rtm->rtm_flags & RTF_LLINFO) && !aflag)
		return;
	if (rtm->rtm_addrs == RTA_DST)
		p_sockaddr(sa, NULL, RTF_HOST, 36);
	else {
		struct sockaddr *mask = NULL;

		/* ughhh. */
		if (rtm->rtm_addrs & RTA_NETMASK) {
			mask = sa;
			if (rtm->rtm_addrs & RTA_GATEWAY)
				mask++;
			mask++;
		}

		p_sockaddr(sa, mask, rtm->rtm_flags, 16);
		if (rtm->rtm_addrs & RTA_GATEWAY) {
			sa++;
			p_sockaddr(sa, NULL, RTF_HOST, 18);
		}
	}
	p_flags(rtm->rtm_flags & interesting, "%-6.6s ");
	printf("%6u %8u ", rtm->rtm_refcnt, rtm->rtm_use);
	if (rtm->rtm_index == 0) {
		putchar('\n');
		return;
	}
	if (rtm->rtm_index == cur_ifindex)
		printf(" %.15s\n", cur_ifname);
	else {
		int   cc;
		char  if_name[16];
		cc = get_if_name_for_route(if_name, rtm->rtm_index);

		if (cc == 1) {
			cur_ifindex = rtm->rtm_index;
			memcpy(cur_ifname, if_name, sizeof(cur_ifname));
			printf(" %.15s\n", cur_ifname);
		} else
			putchar('\n');
	}
}

/* Find out which interface name corresponds to this route */
int
get_if_name_for_route (if_name, lindex)
	char *if_name;
	int   lindex;
{
	int  fd;
	struct ifreq_all  if_all;
	struct strioctl strioc;

	if ((fd = open(_PATH_IP, O_RDONLY)) < 0) {
		perror("netstat: get_if_name_for_route: open");
		return(0);
	}

	if_all.if_number = 0;
	if_all.if_entry.if_index = lindex;

	strioc.ic_cmd = SIOCGIFALL;
	strioc.ic_dp = (char *)&if_all;
	strioc.ic_len = sizeof(struct ifreq_all);
	strioc.ic_timout = -1;

	if (ioctl(fd, I_STR, &strioc) < 0) {
		if (errno != ENXIO) {
			perror("netstat: get_if_name_for_route: SIOCGIFALL");
			(void) close(fd);
			return(0);
		}
		strcpy(if_all.if_entry.if_name, "<defunct>");
	}

	memcpy(if_name, if_all.if_entry.if_name, IFNAMSIZ);
	if_name[IFNAMSIZ - 1] = '\0';

	(void) close(fd);
	return(1);
}

p_sockaddr(sa, mask, flags, width)
	struct sockaddr *sa;
	struct sockaddr *mask;
	int flags, width;
{
	char format[20], workbuf[128], *cp, *cplim;
	register char *cpout;

	switch(sa->sa_family) {
	case AF_INET:
	case AF_UNSPEC:
	    {
		register struct sockaddr_in *sin = (struct sockaddr_in *)sa;
#ifdef	__NEW_SOCKADDR__
		/* figure out how many bits in the mask IP addr are set
		 * skip the first 4 sockaddr bytes (sa_len, family, proto)
		 * of sa_len for the number of bytes used for the mask
		 */
		int slen = mask ?
			((struct sockaddr_in *)mask)->sin_len == 0 ? 0 :
			(((struct sockaddr_in *)mask)->sin_len - 4) * 8 : 32;
#else
		int slen = 32;
#endif

		cp = (sin->sin_addr.s_addr == 0) ? "default" :
			((flags & RTF_HOST) ?
		       routename(sin->sin_addr) :
		       netname(sin->sin_addr,
			       mask ? ntohl(((struct sockaddr_in *)mask)->sin_addr.s_addr) : 0L, slen));
	    }
		break;

	case AF_LINK: {
		struct sockaddr_dl *sdl = (struct sockaddr_dl *)sa;
		if (sdl->sdl_alen == 0)
			sprintf(workbuf, "(incomplete)");
		else
			sprintf(workbuf, "%02x:%02x:%02x:%02x:%02x:%02x",
				sdl->sdl_data[0] & 0xff,
				sdl->sdl_data[1] & 0xff,
				sdl->sdl_data[2] & 0xff,
				sdl->sdl_data[3] & 0xff,
				sdl->sdl_data[4] & 0xff,
				sdl->sdl_data[5] & 0xff);
		}
		cp = workbuf;
		break;
	default:
	    {
		register u_short *s = ((u_short *)sa->sa_data), *slim;

#ifdef	__NEW_SOCKADDR__
		slim = (u_short *) sa + (sa->sa_len - 1) /
		    sizeof(u_short);
#else
		slim = (u_short *) sa + (sizeof(struct sockaddr) - 1) /
		    sizeof(u_short);
#endif
		cp = workbuf;
		cplim = cp + sizeof(workbuf) - 6;
		cp += sprintf(cp, "(%d)", sa->sa_family);
		while (s < slim && cp < cplim)
			cp += sprintf(cp, " %x", *s++);
		cp = workbuf;
	    }
	}
	if (width < 0 )
		printf("%s ", cp);
	else {
		if (nflag)
			printf("%-*s ", width, cp);
		else
			printf("%-*.*s ", width, width, cp);
	}
}

p_flags(f, format)
register int f;
char *format;
{
	char name[33], *flags;
	register struct bits *p = bits;
	for (flags = name; p->b_mask; p++)
		if (p->b_mask & f)
			*flags++ = p->b_val;
	*flags = '\0';
	printf(format, name);
}

p_rtentry(rt)
register struct rtentry *rt;
{
	char name[IFNAMSIZ];
	register struct sockaddr *sa;
	struct ifnet ifnet;

	p_sockaddr(kgetsa(rt_key(rt)),
		   rt_mask(rt) ? kgetsa(rt_mask(rt)) : NULL, rt->rt_flags, 16);
/*SCA*/	p_sockaddr(kgetsa(rt->rt_gateway), NULL, RTF_HOST, 18);
	p_flags(rt->rt_flags, "%-6.6s ");
	printf("%6u %8u ", rt->rt_refcnt, rt->rt_use);
	if (rt->rt_ifp == 0) {
		putchar('\n');
		return;
	}
	kget(rt->rt_ifp, ifnet);
	printf(" %.15s%s", ifnet.if_name, 
		rt->rt_nodes[0].rn_dupedkey ? " =>\n" : "\n");
}

p_ortentry(rt)
register struct ortentry *rt;
{
	char name[IFNAMSIZ], *flags;
	register struct bits *p;
	register struct sockaddr_in *sin;
	struct ifnet ifnet;

	p_sockaddr(&rt->rt_dst, NULL, rt->rt_flags, 16);
	p_sockaddr(&rt->rt_gateway, NULL, 0, 18);
	p_flags(rt->rt_flags, "%-6.6s ");
	printf("%6u %8u ", rt->rt_refcnt, rt->rt_use);
	if (rt->rt_ifp == 0) {
		putchar('\n');
		return;
	}
	kget(rt->rt_ifp, ifnet);
	printf(" %.15s\n", ifnet.if_name);
}

char *
routename(in)
	struct in_addr in;
{
	register char *cp;
	static char line[MAXHOSTNAMELEN + 1];
	struct hostent *hp;
	static char domain[MAXHOSTNAMELEN + 1];
	static int first = 1;
	char *strchr();

	if (first) {
		first = 0;
		if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
		    (cp = strchr(domain, '.')))
			(void) strcpy(domain, cp + 1);
		else
			domain[0] = 0;
	}
	cp = 0;
	if (!nflag) {
		hp = gethostbyaddr((char *)&in, sizeof (struct in_addr),
			AF_INET);
		if (hp) {
			if ((cp = strchr(hp->h_name, '.')) &&
			    !strcasecmp(cp + 1, domain))
				*cp = 0;
			cp = hp->h_name;
		}
	}
	if (cp)
		strncpy(line, cp, sizeof(line) - 1);
	else {
#define C(x)	((x) & 0xff)
		in.s_addr = ntohl(in.s_addr);
		sprintf(line, "%u.%u.%u.%u", C(in.s_addr >> 24),
			C(in.s_addr >> 16), C(in.s_addr >> 8), C(in.s_addr));
	}
	return (line);
}

static u_long
forgemask(
	  u_long a
	  )
{
        u_long m;

        if (IN_CLASSA(a))
                m = IN_CLASSA_NET;
        else if (IN_CLASSB(a))
                m = IN_CLASSB_NET;
        else
                m = IN_CLASSC_NET;
        return (m);
}

static void
domask(
       char *dst,
       u_long addr,
       u_long mask,
       int len
       )
{
        register int b, i;

	if (len < 0) return;

        if (!mask || (forgemask(addr) == mask)) {
                *dst = '\0';
                return;
        }
        i = 0;
	if (len > 32) len=32;
        for (b = 32 - len; b < 32; b++)
                if (mask & (1 << b)) {
                        register int bb;

                        i = b;
                        for (bb = b+1; bb < 32; bb++)
                                if (!(mask & (1 << bb))) {
                                        i = -1; /* noncontig */
                                        break;
                                }
                        break;
                }
        if (i == -1)
                sprintf(dst, "&0x%lx", mask);
        else
                sprintf(dst, "/%d", 32-i);
}

/*
 * Return the name of the network whose address is given.
 * The address is assumed to be that of a net or subnet, not a host.
 */
char *
netname(in, mask, len)
	struct in_addr in;
	u_long mask;
	int len;
{
	char *cp = 0;
	static char line[MAXHOSTNAMELEN + 1];
	struct netent *np = 0;
	u_long net, omask;
	register i;
	int subnetshift;

	i = ntohl(in.s_addr);
	omask = mask;
	if (!nflag && i) {
		if (mask == 0) {
			if (IN_CLASSA(i)) {
				mask = IN_CLASSA_NET;
				subnetshift = 8;
			} else if (IN_CLASSB(i)) {
				mask = IN_CLASSB_NET;
				subnetshift = 8;
			} else if (IN_CLASSC(i)) {
				mask = IN_CLASSC_NET;
				subnetshift = 4;
			} else {
				/* treate it as host address, no shifting necessary */
				mask = 0xffffffff;
				subnetshift = 0;
			}
			/*
			 * If there are more bits than the standard mask
			 * would suggest, subnets must be in use.
			 * Guess at the subnet mask, assuming reasonable
			 * width subnet fields.
			 */
			while (i &~ mask)
				mask = (long)mask >> subnetshift;
		}
		net = i & mask;
		while ((mask & 1) == 0)
			mask >>= 1, net >>= 1;
		np = getnetbyaddr(net, AF_INET);
		if (np)
			cp = np->n_name;
	}	
	if (cp)
		strncpy(line, cp, sizeof(line) - 1);
	else if ((i & 0xffffff) == 0)
		sprintf(line, "%u", C(i >> 24));
	else if ((i & 0xffff) == 0)
		sprintf(line, "%u.%u", C(i >> 24) , C(i >> 16));
	else if ((i & 0xff) == 0)
		sprintf(line, "%u.%u.%u", C(i >> 24), C(i >> 16), C(i >> 8));
	else
		sprintf(line, "%u.%u.%u.%u", C(i >> 24),
			C(i >> 16), C(i >> 8), C(i));
	if (!Aflag)
		domask(line+strlen(line), i, omask, len);
	return (line);
}

/*
 * Print routing statistics
 */
rt_stats()
{
	int  fd;
	struct ip_stuff ipstuff;
#define	rtstat	ipstuff.rt_stat

	struct strioctl strioc;

	if ((fd = open(_PATH_IP, O_RDONLY)) < 0) {
		fprintf(stderr,"netstat: ");
		perror(_PATH_IP);
		exit(1);
	}

	strioc.ic_cmd = SIOCGIPSTUFF;
	strioc.ic_dp = (char *)&ipstuff;
	strioc.ic_len = sizeof(ipstuff);
	strioc.ic_timout = -1;

	if(ioctl(fd, I_STR, &strioc) < 0) {
		perror("netstat: rt_stats: ioctl: SIOCGIPSTUFF");
		exit(1);
	}

	printf("routing:\n");
	printf("\t%lu bad routing redirect%s\n",
		rtstat.rts_badredirect, plural(rtstat.rts_badredirect));
	printf("\t%lu dynamically created route%s\n",
		rtstat.rts_dynamic, plural(rtstat.rts_dynamic));
	printf("\t%lu new gateway%s due to redirects\n",
		rtstat.rts_newgateway, plural(rtstat.rts_newgateway));
	printf("\t%lu destination%s found unreachable\n",
		rtstat.rts_unreach, plural(rtstat.rts_unreach));
	printf("\t%lu use%s of a wildcard route\n",
		rtstat.rts_wildcard, plural(rtstat.rts_wildcard));
	printf("\t%lu route%s created by PMTU discovery\n",
		rtstat.rts_pmtumade, plural(rtstat.rts_pmtumade));
	printf("\t%lu route%s modified by PMTU discovery\n",
		rtstat.rts_pmtumod, plural(rtstat.rts_pmtumod));
	printf("\t%lu route%s discarded by PMTU discovery\n",
		rtstat.rts_discard, plural(rtstat.rts_discard));
}
