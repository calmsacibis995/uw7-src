#ident "@(#)arp.c	1.5"
#ident "$Header$"

/*
 * arp - display, set, and delete arp table entries
 */


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/time.h>
#include <fcntl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>

#include <netinet/in.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <paths.h>

extern int errno;
static int pid;
static int nflag = 0;
int	Sflag = 0;
int	Fflag = 0;
int	aflag = 0;
int	dflag = 0;
int	sflag = 0;
int	fflag = 0;
static int s = -1;
char *filename;

main(argc, argv)
	int argc;
	char **argv;
{
	int ch;
	extern int optind;
	extern char *optarg;

	pid = getpid();
	while ((ch = getopt(argc, argv, "aFf:ndsS")) != EOF) {
		switch((char)ch) {
		case 'a':
			aflag++;
			break;
		case 'd':
			if (argc < 3 || argc > 4)
				usage();
			dflag++;
			break;
		case 'F':
			Fflag++;
			break;
		case 'f':
			filename = optarg;
			fflag++;
			break;
		case 'n':
			nflag++;
			continue;
		case 'S':
			Sflag++;
			continue;
		case 's':
			if (argc < 4 || argc > 7)
				usage();
			sflag++;
			exit(set(argc-2, &argv[2]) ? 1 : 0);
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (Sflag) {
		stats();
		exit(0);
	}
	if (fflag + aflag + dflag + sflag + Fflag > 1) {
		fprintf(stderr,"arp: only one of -a, -d, -f, -F, or -s allowed.\n");
		exit(1);
	}
	if (aflag || Fflag) {
		dump(0);
		exit(0);
	}
	if (dflag) {
		exit(delete(argv[0], argv[1]));
	}
	if (sflag) {
		exit(set(argc, &argv[0]) ? 1 : 0);
	}
	if (fflag) {
		exit(file(filename) ? 1 : 0);
	}

	if (argc != 1)
		usage();

	get(argv[0]);
	exit(0);
}

/*
 * Process a file to set standard arp entries
 */
file(name)
	char *name;
{
	FILE *fp;
	int i, retval;
	char line[100], arg[5][50], *args[5];

	if ((fp = fopen(name, "r")) == NULL) {
		fprintf(stderr, "arp: cannot open %s\n", name);
		exit(1);
	}
	args[0] = &arg[0][0];
	args[1] = &arg[1][0];
	args[2] = &arg[2][0];
	args[3] = &arg[3][0];
	args[4] = &arg[4][0];
	retval = 0;
	while(fgets(line, 100, fp) != NULL) {
		i = sscanf(line, "%s %s %s %s %s", arg[0], arg[1], arg[2],
		    arg[3], arg[4]);
		if (i < 2) {
			fprintf(stderr, "arp: bad line: %s\n", line);
			retval = 1;
			continue;
		}
		if (set(i, args))
			retval = 1;
	}
	fclose(fp);
	return (retval);
}

getsocket() {
	if (s < 0) {
		s = open(_PATH_ROUTE, O_RDWR, 0);
		if (s < 0) {
			(void) fprintf(stderr,"arp: ");
			perror(_PATH_ROUTE);
			exit(1);
		}
		(void) ioctl(s, I_SRDOPT, RMSGD);
	}
}

#ifdef __NEW_SOCKADDR__
struct	sockaddr_in so_mask = { sizeof(struct sockaddr_in), 0, 0,
				{ 0xff, 0xff, 0xff, 0xff} };
struct	sockaddr_inarp blank_sin = { sizeof(struct sockaddr_inarp),
				     AF_INET }, sin_m;
struct	sockaddr_dl blank_sdl = { sizeof(struct sockaddr_dl), AF_LINK }, sdl_m;
#else
struct	sockaddr_in so_mask = {0, 0, { 0xffffffff}};
struct	sockaddr_inarp blank_sin = {AF_INET }, sin_m;
struct	sockaddr_dl blank_sdl = {AF_LINK }, sdl_m;
#endif
int	expire_time, flags, export_only, doing_proxy, found_entry;
struct	{
	struct	rt_msghdr m_rtm;
	char	m_space[512];
}	m_rtmsg;

/*
 * Set an individual arp entry 
 */
set(argc, argv)
	int argc;
	char **argv;
{
	struct hostent *hp;
	register struct sockaddr_inarp *sin = &sin_m;
	register struct sockaddr_dl *sdl;
	register struct rt_msghdr *rtm = &(m_rtmsg.m_rtm);
	u_char *ea;
	char *host = argv[0], *eaddr = argv[1];

	getsocket();
	argc -= 2;
	argv += 2;
	sdl_m = blank_sdl;
	sin_m = blank_sin;
	sin->sin_addr.s_addr = inet_addr(host);
	if (sin->sin_addr.s_addr == -1) {
		if (!(hp = gethostbyname(host))) {
			fprintf(stderr, "arp: %s: ", host);
			herror((char *)NULL);
			return (1);
		}
		bcopy((char *)hp->h_addr, (char *)&sin->sin_addr,
		    sizeof sin->sin_addr);
	}
	ea = (u_char *)LLADDR(&sdl_m);
	if (arp_ether_aton(eaddr, ea) == 0)
		sdl_m.sdl_alen = 6;
	doing_proxy = flags = export_only = expire_time = 0;
	while (argc-- > 0) {
		if (strncmp(argv[0], "temp", 4) == 0) {
			struct timeval time;
			gettimeofday(&time, (struct timezone *)0);
			expire_time = time.tv_sec + 20 * 60;
		}
		else if (strncmp(argv[0], "pub", 3) == 0)
			doing_proxy = SIN_PROXY;
#if 0
		else if (strncmp(argv[0], "trail", 5) == 0)
			flags = RTF_PROTO1;
#endif
		argv++;

	}
tryagain:
	if (rtmsg(RTM_GET) < 0) {
		perror(host);
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
			goto overwrite;
		}
		if (doing_proxy == 0) {
			printf("set: can only proxy for %s\n", host);
			return (1);
		}
		if (sin_m.sin_other & SIN_PROXY) {
			printf("set: proxy entry exists for non 802 device\n");
			return(1);
		}
		sin_m.sin_other = SIN_PROXY;
		export_only = 1;
		goto tryagain;
	}
overwrite:

	sdl_m.sdl_type = sdl->sdl_type;
	sdl_m.sdl_index = sdl->sdl_index;
	return (rtmsg(RTM_ADD));
}

/*
 * Display an individual arp entry
 */
get(host)
	char *host;
{
	struct hostent *hp;
	struct sockaddr_inarp *sin = &sin_m;
	u_char *ea;

	sin_m = blank_sin;
	sin->sin_addr.s_addr = inet_addr(host);
	if (sin->sin_addr.s_addr == -1) {
		if (!(hp = gethostbyname(host))) {
			fprintf(stderr, "arp: %s: ", host);
			herror((char *)NULL);
			exit(1);
		}
		bcopy((char *)hp->h_addr, (char *)&sin->sin_addr,
		    sizeof sin->sin_addr);
	}
	dump(sin->sin_addr.s_addr);
	if (found_entry == 0) {
		printf("%s (%s) -- no entry\n", host, inet_ntoa(sin->sin_addr));
		exit(1);
	}
}

/*
 * Delete an arp entry 
 */
delete(host, info)
	char *host;
	char *info;
{
	struct hostent *hp;
	register struct sockaddr_inarp *sin = &sin_m;
	register struct rt_msghdr *rtm = &m_rtmsg.m_rtm;
	struct sockaddr_dl *sdl;
	u_char *ea;
	char *eaddr;

	if (info && strncmp(info, "pro", 3) )
		export_only = 1;
	getsocket();
	sin_m = blank_sin;
	sin->sin_addr.s_addr = inet_addr(host);
	if (sin->sin_addr.s_addr == -1) {
		if (!(hp = gethostbyname(host))) {
			fprintf(stderr, "arp: %s: ", host);
			herror((char *)NULL);
			return (1);
		}
		bcopy((char *)hp->h_addr, (char *)&sin->sin_addr,
		    sizeof sin->sin_addr);
	}
tryagain:
	if (rtmsg(RTM_GET) < 0) {
		perror(host);
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
		fprintf(stderr, "delete: can't locate %s\n",host);
		return (1);
	} else {
		sin_m.sin_other = SIN_PROXY;
		goto tryagain;
	}
delete:
	if (sdl->sdl_family != AF_LINK) {
		printf("cannot locate %s\n", host);
		return (1);
	}
	if (rtmsg(RTM_DELETE) == 0)
		printf("%s (%s) deleted\n", host, inet_ntoa(sin->sin_addr));
	return 0;
}

struct if_type {
	int it_type;
	char *it_name;
} if_types[] = {
	{ IFT_OTHER, "other" },
	{ IFT_ETHER, "ether" },
	{ IFT_ISO88023, "802.3" },
	{ IFT_ISO88024, "802.4" },
	{ IFT_ISO88025, "802.5" },
	{ IFT_FDDI, "fddi" }
};
int n_types = sizeof(if_types) / sizeof(struct if_type);

char *
fixtype(type)
	int type;
{
	static char buf[28];
	int i;

	for (i = 0; i < n_types; i++) {
		if (if_types[i].it_type == type) {
			return if_types[i].it_name;
		}
	}
	sprintf(buf, "type %d", type);
	return buf;
}

/*
 * Dump the entire arp table (or flush non-permanent entries)
 */
dump(addr)
	u_long addr;
{
	int sz, needed, rlen;
	long op = KINFO_RT_FLAGS | (((long)AF_INET) << 16);
	char *host, *malloc(), *lim, *buf, *next;
	struct rt_msghdr *rtm;
	struct sockaddr_inarp *sin;
	struct sockaddr_dl *sdl;
	extern int h_errno;
	struct hostent *hp;
	struct rt_giarg gi, *gp;

	getsocket();

	gi.gi_op = op;
	gi.gi_where = (caddr_t)0;
	gi.gi_size = 0;
	gi.gi_arg = RTF_LLINFO;
	rlen = ioctl(s, RTSTR_GETROUTE, &gi);
	if (rlen < 0)
		quit("route-ioctl-estimate");
	if ((buf = malloc(gi.gi_size)) == NULL)
		quit("malloc");
	gp = (struct rt_giarg *)buf;
	gp->gi_size = gi.gi_size;
	gp->gi_op = op;
	gp->gi_arg = RTF_LLINFO;
	gp->gi_where = (caddr_t)buf;
	rlen = ioctl(s, RTSTR_GETROUTE, buf);
	if (rlen < 0)
		quit("actual retrieval of routing table");
	lim = buf + gp->gi_size;
	buf += sizeof(gi);
	for (next = buf; next < lim; next += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *)next;
		sin = (struct sockaddr_inarp *)(rtm + 1);
		sdl = (struct sockaddr_dl *)(sin + 1);
		if (addr) {
			if (addr != sin->sin_addr.s_addr)
				continue;
			found_entry = 1;
		}
		/* flush non-permanent entries */
		if (Fflag) {
			if (rtm->rtm_rmx.rmx_expire) {
				delete(inet_ntoa(sin->sin_addr), (char *)0);
			}
			continue;
		}
		if (nflag == 0)
			hp = gethostbyaddr((caddr_t)&(sin->sin_addr),
			    sizeof sin->sin_addr, AF_INET);
		else
			hp = 0;
		if (hp)
			host = hp->h_name;
		else {
			host = "?";
			if (h_errno == TRY_AGAIN)
				nflag = 1;
		}
		printf("%s (%s) at ", host, inet_ntoa(sin->sin_addr));
		if (sdl->sdl_alen)
			ether_print((u_char *)LLADDR(sdl));
		else
			printf("(incomplete)");
		printf(" (%s)",fixtype(sdl->sdl_type));
		if (rtm->rtm_rmx.rmx_expire == 0)
			printf(" permanent");
		if (sin->sin_other & SIN_PROXY)
			printf(" published (proxy only)");
		if (rtm->rtm_flags & RTF_PROTO1)
			printf(" trailers?");
		if (rtm->rtm_addrs & RTA_NETMASK) {
			sin = (struct sockaddr_inarp *)
				(sizeof(*sdl) + (char *)sdl);
			if (sin->sin_addr.s_addr == 0xffffffff)
				printf(" published");
#if 0
			if (sin->sin_len != 8)
				printf("(wierd)");
#endif
		}
		printf("\n");
	}
}

ether_print(cp)
	u_char *cp;
{
	printf("%x:%x:%x:%x:%x:%x", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
}

arp_ether_aton(a, n)
	char *a;
	u_char *n;
{
	int i, o[6];

	i = sscanf(a, "%x:%x:%x:%x:%x:%x", &o[0], &o[1], &o[2],
					   &o[3], &o[4], &o[5]);
	if (i != 6) {
		fprintf(stderr, "arp: invalid Ethernet address '%s'\n", a);
		return (1);
	}
	for (i=0; i<6; i++)
		n[i] = o[i];
	return (0);
}

usage()
{
	printf("usage: arp hostname\n");
	printf("       arp -a\n");
	printf("       arp -d hostname\n");
	printf("       arp -s hostname ether_addr [temp] [pub]\n");
	printf("       arp -f filename\n");
	printf("       arp -S\n");
	printf("       arp -F\n");
	exit(1);
}

rtmsg(cmd)
	int	cmd;
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
		fprintf(stderr, "arp: internal wrong cmd\n");
		exit(1);
	case RTM_ADD:
		rtm->rtm_addrs |= RTA_GATEWAY;
		rtm->rtm_rmx.rmx_expire = expire_time;
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
			perror("arp: ioctl");
			return (-1);
		}
	}
	do {
		l = read(s, (char *)&m_rtmsg, sizeof(m_rtmsg));
#if 0
	} while (l > 0 && (rtm->rtm_seq != seq || rtm->rtm_pid != pid));
#else
	} while (l > 0 && (rtm->rtm_seq != seq));
#endif
	if (l < 0)
		(void) fprintf(stderr, "arp: read from routing stream: %s\n",
		    strerror(errno));
	return (0);
}

quit(msg)
	char *msg;
{
	fprintf(stderr, "arp: %s\n", msg);
	exit(1);
}

char           *
waswere(n)
	int             n;
{

	return (n != 1 ? "were" : "was");
}

char           *
plural(n)
	int             n;
{

	return (n != 1 ? "s" : "");
}

char           *
pluraly(n)
	int             n;
{

	return (n != 1 ? "ies" : "y");
}

stats()
{
	int fd, r;
	struct strioctl si;
	struct arpstat arpstat;
	u_long t;

	fd = open(_PATH_ARP, O_RDWR);

	if (fd < 0) {
		fprintf(stderr, "arp: ");
		perror(_PATH_ARP);
		exit(1);
	}

	si.ic_cmd = SIOCGARPSTAT;
	si.ic_len = sizeof(arpstat);
	si.ic_dp = (char *)&arpstat;
	si.ic_timout = -1;

	r = ioctl(fd, I_STR, (char *)&si);
	if (r < 0) {
		fprintf(stderr, "arp: ");
		perror("SIOCGARPSTAT");
		exit(1);
	}
	printf("arp statistics:\n");
	printf("\t%lu frame%s sent\n", arpstat.as_sent, plural(arpstat.as_sent));
	printf("\t%lu frame%s received\n", arpstat.as_recv, 
		plural(arpstat.as_recv));
	printf("\t\t%lu had a bad hardware type\n", arpstat.as_badhtype);
	printf("\t\t%lu had a bad MAC address\n", arpstat.as_badmac);
	printf("\t\t%lu had a bad IP address\n", arpstat.as_badaddr);
	printf("\t\t%lu had our address\n", arpstat.as_duped);
	printf("\t\t%lu updated an existing entry\n", arpstat.as_updated);
	t = arpstat.as_ours + arpstat.as_proxies + arpstat.as_us;
	printf("\t%lu lookup%s\n", t, plural(t));
	printf("\t\t%lu %s generated locally\n", arpstat.as_ours, 
		waswere(arpstat.as_ours));
	printf("\t\t%lu %s received\n", t - arpstat.as_ours, 
		waswere(t - arpstat.as_ours));
	printf("\t\t\t%lu %s for us\n", arpstat.as_us, waswere(arpstat.as_us));
	printf("\t\t\t%lu %s for prox%s\n", arpstat.as_proxies, 
		waswere(arpstat.as_proxies), pluraly(arpstat.as_proxies));
	printf("\t\t%lu failed\n", arpstat.as_failed);
	printf("\t%lu entr%s went into reject state\n",arpstat.as_rejected,
		pluraly(arpstat.as_rejected));
	printf("\t\t%lu entr%s %s revived\n",arpstat.as_revived,
		pluraly(arpstat.as_revived), waswere(arpstat.as_revived));
	printf("\t%lu entr%s %s expired\n",arpstat.as_aged,
		pluraly(arpstat.as_aged), waswere(arpstat.as_aged));
}
