#ident "@(#)ifconfig.c	1.6"
#ident "$Header$"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <sys/stream.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <stropts.h>

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <string.h>
#include <paths.h>
#include <sys/dlpi.h>
#include <arpa/inet.h>
#include <stdlib.h>

int doall(void (*fp)(), int argc, char **argv, int af);
void doif(int argc, char **argv, int af, struct ifreq *ifp);
int setifaddr(char *addr, short param);
int setifnetmask(char *addr);
int setifbroadaddr(char *addr);
int setifipdst(char *addr);
int notealias(char *addr, int param);
int setifdstaddr(char *addr, int param);
int setifflags(char *vname, short value);
int setifmetric(char *);
int setifdebug(char *);
int setifmtu(char *val);
int setifonepacket(char *val1, char *val2);
int setifperf(char *, char *, char *);
int status(void);
int in_status(int force);
int Perror(char *cmd);
int in_getaddr(char *s, int which);
int printb(char *s, unsigned short v, char *bits);
int usage(void);
int ifioctl(int s, int cmd, char *arg);
int pr_macaddr(char *);
int donothing(char *);

extern int      errno;
char	lastifname[IFNAMSIZ + 1];
struct	ifreq    ifr, ridreq, oifr;
struct	ifaliasreq	addreq;
struct strioctl st;
#ifdef __NEW_SOCKADDR__
struct	sockaddr_in sin = { sizeof(struct sockaddr_in), AF_INET };
struct	sockaddr_in netmask = { sizeof(struct sockaddr_in), AF_INET };
struct	sockaddr_in ipdst = { sizeof(struct sockaddr_in), AF_INET };
#else
struct	sockaddr_in sin = {AF_INET};
struct	sockaddr_in netmask = {AF_INET};
struct	sockaddr_in ipdst = {AF_INET};
#endif
struct	sockaddr_in broadaddr;
char    name[IFNAMSIZ + 1];
int     flags;
int     metric;
int	mtu;
int	dbglvl;
int     setaddr;
int	doalias;
int     setmask;
int     setbroadaddr;
int     setipdst;
int	clearaddr = 0;
int	newaddr = 1;
int     s;
int	plumb_interfaces = 0;


#define	NEXTARG		0xffffff
#define NEXTTWO		0xfffffe
#define NEXTTHREE	0xfffffd
#define DEAD		0xfffffc

struct cmd {
	char           *c_name;
	int             c_parameter;	/* NEXTARG means next argv */
	int             (*c_func) ();
} cmds[] = {
	{ "up",		IFF_UP,		setifflags },
	{ "down",	-IFF_UP,	setifflags },
	{ "arp",	-IFF_NOARP,	setifflags },
	{ "-arp",	IFF_NOARP,	setifflags },
	{ "link0",	IFF_LINK0,	setifflags },
	{ "-link0",	-IFF_LINK0,	setifflags },
	{ "link1",	IFF_LINK1,	setifflags },
	{ "-link1",	-IFF_LINK1,	setifflags },
	{ "link2",	IFF_LINK2,	setifflags },
	{ "-link2",	-IFF_LINK2,	setifflags },
	{ "debug",	NEXTARG,	setifdebug },
	{ "trailers",	DEAD,		donothing },
	{ "-trailers",	DEAD,		donothing },
	{ "alias",	IFF_UP,		notealias },
	{ "-alias",	-IFF_UP,	notealias },
	{ "netmask",	NEXTARG,	setifnetmask },
	{ "metric",	NEXTARG,	setifmetric },
	{ "mtu",	NEXTARG,	setifmtu },
	{ "broadcast",	NEXTARG,	setifbroadaddr },
	{ "ipdst", 	NEXTARG,	setifipdst },
	{ "-onepacket", -IFF_ONEPACKET,	setifflags },
	{ "onepacket",	NEXTTWO,	setifonepacket },
	{ "perf",	NEXTTHREE,	setifperf },
	{ 0, 		0,		setifaddr },
	{ 0, 		0,		setifdstaddr },
};


int in_status(int), in_getaddr(char *, int), in_alias(int);

/* Known address families */
struct afswtch {
	char           *af_name;
	short           af_af;
	char           *af_dev;
	int             (*af_status) __P((int));
	int             (*af_getaddr) __P((char *, int));
	int             (*af_alias) __P((int));
	unsigned long	af_difaddr;
	unsigned long	af_aifaddr;
	caddr_t		af_ridreq;
	caddr_t		af_addreq;
} afs[] = {
#define C(x)	((caddr_t) &x)
	{
	  "inet", AF_INET, _PATH_IP, in_status, in_getaddr, in_alias,
		SIOCDIFADDR, SIOCAIFADDR, C(ridreq), C(addreq)
	} ,
	{ 0, 0, 0, 0, 0, 0 }
};

struct afswtch *afp;		/* the address family being set or asked
				 * about */
struct		afswtch *rafp;

main(argc, argv)
	int             argc;
	char           *argv[];
{
	int             af = AF_INET;

	if (argc < 2) {
		usage();
	}
	argc--, argv++;
	strncpy(name, *argv, sizeof(name));
	argc--, argv++;

	if (!strcmp(name, "-p")) {
	    plumb_interfaces = 1;
	    strncpy(name, *argv, sizeof(name));
	    argc--, argv++;
	}

	if (argc > 0) {
		for (afp = rafp = afs; rafp->af_name; rafp++)
			if (strcmp(rafp->af_name, *argv) == 0) {
				afp = rafp;
				argc--; argv++;
				break;
			}
		rafp = afp;
		af = ifr.ifr_addr.sa_family = afp->af_af;
	} else {
		afp = afs;
	}

	s = open(afp->af_dev, O_RDONLY);
	if (s < 0) {
		perror("ifconfig: open");
		exit(1);
	}
	if (!strcmp(name, "-?"))	/* pretend like we do getopt */
		usage();
	if (setup())
		exit(1);
	if (strcmp(name, "-a"))
		doif(argc, argv, af, (struct ifreq *)0);
	else
	    if (plumb_interfaces)
		usage();
	    else	
		doall(doif, argc, argv, af);
	free(st.ic_dp);	
	exit(0);
/* NOTREACHED */
}

setup()
{
	int nifs;
	char *buf;
	int r;
	struct ifreq ifr;

	st.ic_cmd = SIOCGIFANUM;
	st.ic_len = sizeof(struct ifreq);
	st.ic_dp = (char *)&ifr;
	st.ic_timout = -1;

	r = ioctl(s, I_STR, (char *)&st);
	if (r < 0) {
                perror("ioctl (get interface table size)");
                close(s);
                return -1;
        }
	nifs = ifr.ifr_nif;
	buf = (char *)malloc((nifs + 1) * sizeof(struct ifreq));
	if (buf == 0) {
                fprintf(stderr,"ioctl (no memory for if table)");
                close(s);
                return -1;
        }
	st.ic_cmd = SIOCGIFCONF;
	st.ic_len = (nifs + 1) * sizeof(struct ifreq);
	st.ic_dp = (char *)buf;
	st.ic_timout = 0;

        if (ioctl(s, I_STR, (char *)&st) < 0) {
                perror("ioctl (get interface configuration)");
                close(s);
                return -1;
        }
	return 0;
}

doall(fp, argc, argv, af)
	void	(*fp)();
	int	argc;
	char	**argv;
	int	af;
{
        int n;
        struct ifconf ifc;
        register struct ifreq *ifp;

        ifp = (struct ifreq *)st.ic_dp;
        ifc.ifc_buf = st.ic_dp;
	ifc.ifc_len = st.ic_len;

        for (n = ifc.ifc_len / sizeof (struct ifreq); n > 0; n--, ifp++) {
		if (af != ifp->ifr_addr.sa_family)
			continue;
                (void) close(s);
                s = open(afp->af_dev, O_RDONLY);
                if (s == -1) {
                        perror("ifconfig: open");
                        exit(1);
                }

                setaddr = setbroadaddr = setipdst = setmask = 0;

                (void) strncpy(name, ifp->ifr_name, sizeof(name));
                (*fp)(argc, argv, af, ifp);
		(void) strncpy(lastifname, ifp->ifr_name, sizeof(lastifname));
        }
}

void
doif(argc, argv, af, ifp)
	int	argc;
	char	**argv;
	int	af;
	struct	ifreq *ifp;
{
	bzero(ifr.ifr_name, sizeof(ifr.ifr_name));
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ifp)
		bcopy(ifp, (char *)&oifr, sizeof(*ifp));
	if (ifioctl(s, SIOCGIFFLAGS, (caddr_t) & ifr) < 0) {
	    if ((errno == ENXIO) && plumb_interfaces) {
		errno = 0;
		plumb(ifr.ifr_name);

		if (ifioctl(s, SIOCGIFFLAGS, (caddr_t) &ifr) < 0) {
		    Perror("ioctl (SIOCGIFFLAGS)");
		    exit(1);
		}
	    } else {
		Perror("ioctl (SIOCGIFFLAGS)");
		exit(1);
	    }
	}
	flags = ifr.ifr_flags;
	if (ifioctl(s, SIOCGIFMETRIC, (caddr_t) & ifr) < 0)
		perror("ioctl (SIOCGIFMETRIC)");
	else
		metric = ifr.ifr_metric;
	if (ifioctl(s, SIOCGIFDEBUG, (caddr_t) &ifr) < 0)
		perror("ioctl SIOCGIFDEBUG");
	else
		dbglvl = ifr.ifr_metric;
	if (ifioctl(s, SIOCGIFMTU, (caddr_t) &ifr) < 0)
		perror("ioctl SIOCGIFMTU");
	else
		mtu = ifr.ifr_metric;
	if (argc == 0) {
        	int n;
        	struct ifconf ifc;
        	register struct ifreq *tifp;

		if (ifp) {	/* -a option */
			status();
			return;
		}

		/* not -a option */
        	tifp = (struct ifreq *)st.ic_dp;
        	ifc.ifc_buf = st.ic_dp;
		ifc.ifc_len = st.ic_len;

       		for (n = ifc.ifc_len / sizeof (struct ifreq); n > 0; n--, tifp++) {
			if (strcmp(name, tifp->ifr_name) /* another interface */
			    || (af != tifp->ifr_addr.sa_family))
				continue;
			bcopy(tifp, (char *)&oifr, sizeof(*ifp));
			status();
			(void) strncpy(lastifname, tifp->ifr_name,
					sizeof(lastifname));
       		}
		return;
	}
	while (argc > 0) {
		register struct cmd *p;

		for (p = cmds; p->c_name; p++)
			if (strcmp(*argv, p->c_name) == 0)
				break;
		if (p->c_name == 0 && setaddr)
			p++;	/* got src, do dst */
		if (p->c_func) {
			if (p->c_parameter == NEXTARG) {
				if (argc < 2) {
					fprintf(stderr, 
					"ifconfig: one value required\n");
					usage();
				}
				(*p->c_func) (argv[1]);
				argc--, argv++;
			} else if (p->c_parameter == NEXTTWO) {
				if (argc < 3) {
					fprintf(stderr, 
					"ifconfig: two values required\n");
					usage();
				}
				(*p->c_func) (argv[1], argv[2]);
				argc -= 2;
				argv += 2;
			} else if (p->c_parameter == NEXTTHREE) {
				if (argc < 4) {
					fprintf(stderr, 
					"ifconfig: three values required\n");
					usage();
				}
				(*p->c_func) (argv[1], argv[2], argv[3]);
				argc -= 3;
				argv += 3;
			} else if (p->c_parameter == DEAD) {
				(*p->c_func) (*argv);
			} else {
				(*p->c_func) (*argv, p->c_parameter);
			}				
		}
		argc--, argv++;
	}
#if 0
	if ((setmask || setaddr) && (af == AF_INET)) {
		/*
		 * If setting the address and not the mask, clear any
		 * existing mask and the kernel will then assign the default.
		 * If setting both, set the mask first, so the address will
		 * be interpreted correctly. 
		 */
		ifr.ifr_addr = *(struct sockaddr *) & netmask;
		if (ifioctl(s, SIOCSIFNETMASK, (caddr_t) & ifr) < 0)
			Perror("ioctl (SIOCSIFNETMASK)");
	}
#endif
	if (clearaddr) {
		int ret;
		strncpy(rafp->af_ridreq, name, sizeof(ifr.ifr_name));
		if ((ret = ifioctl(s, rafp->af_difaddr, rafp->af_ridreq)) < 0) {
			if (errno == EADDRNOTAVAIL && (doalias >= 0)) {
				/* means no previous address for interface */
			} else
				Perror("ioctl (SIOCDIFADDR)");
		}
	}
	if (newaddr) {
		strncpy(rafp->af_addreq, name, sizeof(ifr.ifr_name));
		if (ifioctl(s, rafp->af_aifaddr, rafp->af_addreq) < 0) {
			if (errno != EEXIST) {
				Perror("ioctl (SIOCAIFADDR)");
			} else {
				fprintf(stderr,"ifconfig: warning -- a route via this interface may already exist.\n");
			}
		}
	}
}

#define RIDADDR 0
#define	ADDR	1
#define	MASK	2
#define	DSTADDR	3

/* ARGSUSED */
int
setifaddr(
	char           *addr,
	short           param
	)
{
	/*
	 * Delay the ioctl to set the interface addr until flags are all set.
	 * The address interpretation may depend on the flags, and the flags
	 * may change when the address is set. 
	 */
	setaddr++;
	if (doalias == 0)
		clearaddr = 1;
	(*afp->af_getaddr) (addr, (doalias >= 0 ? ADDR: RIDADDR));
	if (doalias >= 0)	/* if not unaliasing */
		newaddr = 1;
}

setifnetmask(addr)
	char           *addr;
{
	in_getaddr(addr, MASK);
}

setifbroadaddr(addr)
	char           *addr;
{
	(*afp->af_getaddr) (addr, DSTADDR);
}

setifipdst(addr)
	char           *addr;
{
	in_getaddr(addr, DSTADDR);
	setipdst++;
	clearaddr = 0;
	newaddr = 0;
}

#define rqtosa(x) (&(((struct ifreq *)(afp->x))->ifr_addr))
/*ARGSUSED*/
notealias(addr, param)
        char *addr;
{
        if (setaddr && doalias == 0 && param < 0)
                bcopy((caddr_t)rqtosa(af_addreq),
                      (caddr_t)rqtosa(af_ridreq),
                      sizeof(struct sockaddr));
        doalias = param;
        if (param < 0) {
                clearaddr = 1;
                newaddr = 0;
        } else
                clearaddr = 0;
}

/* ARGSUSED */
setifdstaddr(addr, param)
	char           *addr;
	int             param;
{

	(*afp->af_getaddr) (addr, DSTADDR);
	/*
	 * SCA -- disallow setting of destination address on a broadcast medium.
	 * ARP should always set IFF_BROADCAST, so this is a valid
	 * test.
	 */
	if (ifr.ifr_flags & IFF_BROADCAST) {
		(void)fprintf(stderr,
		  "ifconfig: setting destination address disallowed on broadcast media.\n");
		return;
	}
}

int
setifflags(
	char           *vname,
	short          value
	)
{
	if (ifioctl(s, SIOCGIFFLAGS, (caddr_t) & ifr) < 0) {
		Perror("ioctl (SIOCGIFFLAGS)");
		exit(1);
	}
	flags = ifr.ifr_flags;

	if (value < 0) {
		value = -value;
		flags &= ~value;
	} else
		flags |= value;
	ifr.ifr_flags = flags;
	if (ifioctl(s, SIOCSIFFLAGS, (caddr_t) & ifr) < 0)
		Perror(vname);
}

setifmetric(val)
	char           *val;
{
	ifr.ifr_metric = atoi(val);
	if (ifioctl(s, SIOCSIFMETRIC, (caddr_t) & ifr) < 0)
		perror("ioctl (set metric)");
}

setifdebug(val)
	char           *val;
{
	ifr.ifr_metric = atoi(val);

	if (ifioctl(s, SIOCSIFDEBUG, (caddr_t) & ifr) < 0) {
		perror("ioctl (set debug)");
		return;
	}
}

setifmtu(val)
	char           *val;
{
	ifr.ifr_metric = atoi(val);
	if (ifioctl(s, SIOCSIFMTU, (caddr_t) & ifr) < 0)
		perror("ioctl (set metric)");
}

setifonepacket(val1, val2)
	char *val1, *val2;
{
	setifflags("onepacket", IFF_ONEPACKET);
	ifr.ifr_onepacket.spsize = atoi(val1);
	ifr.ifr_onepacket.spthresh = atoi(val2);
	if (ifioctl(s, SIOCSIFONEP, (caddr_t) & ifr) < 0)
		perror("ioctl (set one-packet mode params)");
}

setifperf(val1, val2, val3)
	char *val1, *val2, *val3;
{
	ifr.ifr_perf.ip_recvspace = atoi(val1);
	ifr.ifr_perf.ip_sendspace = atoi(val2);
	ifr.ifr_perf.ip_fullsize = atoi(val3);
	if (ifioctl(s, SIOCSIFPERF, (caddr_t) &ifr) < 0)
		perror("ioctl (set performance params)");
}

#define	IFFBITS \
"\020\1UP\2BROADCAST\3DEBUG\4LOOPBACK\5POINTOPOINT\6WANTIOCTLS\7RUNNING\010NOARP\
\011LINK0\012ALLMULTI\013LINK1\014ONEPACKET\015LINK2\016SIMPLEX\
\017MULTICAST"

/*
 * Print the status of the interface.  If an address family was specified,
 * show it and it only; otherwise, show them all. 
 */
status()
{
	register struct afswtch *p = afp;
	short           af = ifr.ifr_addr.sa_family;

	if (strcmp(name, lastifname) == 0) {
		(*p->af_alias)(1);
		return;
	}
	printf("%s: ", name);
	printb("flags", (unsigned short)flags, IFFBITS);
	printf(" mtu %u", mtu);
	if (metric)
		printf(" metric %u", metric);
	if (dbglvl)
		printf(" debuglevel %u", dbglvl);
	putchar('\n');
	if ((p = afp) != NULL) {
		(*p->af_status) (1);
	} else {
		for (p = afs; p->af_name; p++) {
			ifr.ifr_addr.sa_family = p->af_af;
			afp = p;
			close(s);
			s = open(afp->af_dev, O_RDONLY);
			if (s < 0) {
				if (errno == EPROTONOSUPPORT || errno == ENODEV)
					continue;
				Perror("ifconfig: status open");
			}
			(*p->af_status) (0);
		}
	}
	(void) pr_macaddr(name);
}

pr_macaddr(name)
	char *name;
{
	int	fd;
	u_char 	a[23];
	int r;
	int i;
	char iface[40];
	union DL_primitives dl;
	int nomac = 0;
	int maclen = 0;
	dl_phys_addr_req_t *phys_addr_req;
	dl_phys_addr_ack_t *phys_addr_ack;
#if !defined(MAC_ADDR_SZ)
#define MAC_ADDR_SZ 6
#endif
	char phys_addr_buf[DL_PHYS_ADDR_ACK_SIZE + MAC_ADDR_SZ];
	struct strbuf ctl;
	char *pt;
	int flags;

	strcpy(iface, _PATH_DEV);
	strcat(iface, name);
	if ((fd = open(iface, O_RDWR, 0)) < 0) {
		return;
	}

	/*
	 * Otherwise, try using DLPI primitives.
	 * We really should try these first, but
	 * some of the 3.1.1 LLI drivers (like i3B0)
	 * are confused, and won't nak the request.
	 */

	bzero(phys_addr_buf, sizeof(phys_addr_buf));
	
	ctl.buf = phys_addr_buf;
	ctl.len = sizeof(dl_phys_addr_req_t);
	ctl.maxlen = 0;
	
	phys_addr_req = (dl_phys_addr_req_t *) phys_addr_buf;
	phys_addr_req->dl_primitive = DL_PHYS_ADDR_REQ;
	phys_addr_req->dl_addr_type = DL_CURR_PHYS_ADDR;

	if (putmsg(fd, &ctl, NULL, 0) < 0) {
		/*
		 * Something is seriously wrong.
		 */
		close(fd);
		return;
	}

	flags = 0;
	bzero(phys_addr_buf, sizeof(phys_addr_buf));
	
	ctl.buf = phys_addr_buf;
	ctl.len = 0;
	ctl.maxlen = sizeof(phys_addr_buf);

	if (getmsg(fd, &ctl, NULL, &flags) < 0) {
		goto info_req;
	}
	phys_addr_ack = (dl_phys_addr_ack_t *) phys_addr_buf;

	if (phys_addr_ack->dl_primitive != DL_PHYS_ADDR_ACK) {
		nomac = 1;
		goto info_req;
	}
	memcpy(a, phys_addr_buf + phys_addr_ack->dl_addr_offset,
					phys_addr_ack->dl_addr_length);
	maclen = phys_addr_ack->dl_addr_length;

info_req:

	ctl.len = ctl.maxlen = sizeof(dl_info_req_t);
	ctl.buf = (char *)&dl;
	dl.dl_primitive = DL_INFO_REQ;

	r = putmsg(fd, &ctl, (struct strbuf *)0, 0);
	if (r < 0) {
		perror("putmsg");
		dl.info_ack.dl_mac_type = 999;
	} else {
		ctl.maxlen = sizeof(dl_info_ack_t);
		ctl.buf = (char *)&dl;

		flags = 0;
		r = getmsg(fd, &ctl, (struct strbuf *)0, &flags);
		if (r < 0) {
			dl.info_ack.dl_mac_type = 999;
		}
	}	
	switch (dl.dl_primitive) {
	case DL_INFO_ACK:
		break;
	default:
		dl.info_ack.dl_mac_type = 999;
		break;
	}
	switch (dl.info_ack.dl_mac_type) {
	case DL_ETHER:
	case DL_CSMACD:
		if (maclen == MAC_ADDR_SZ) {
			pt = "ether";
		} else {
			pt = "mac";
		}
		break;
	case DL_TPR:
		pt = "token";
		break;
	case DL_FDDI:
		pt = "fddi";
		break;
	default:
		pt = "mac";
		break;
	}
	if (nomac == 0) {
		printf("\t%s ", pt);
		for (i = 0; i < maclen; i++) {
			if (i == maclen - 1) {
				printf("%02x\n", a[i]);
			} else {
				printf("%02x:", a[i]);
			}
		}
	}
	close(fd);
}

in_status(force)
	int             force;
{
	struct sockaddr_in *sin;

	if (ifioctl(s, SIOCGIFADDR, (caddr_t) & ifr) < 0) {
		if (errno == EADDRNOTAVAIL || errno == EPROTONOSUPPORT) {
			if (!force)
				return;
			memset((char *) &ifr.ifr_addr, '\0', 
				sizeof(ifr.ifr_addr));
		} else
			perror("ioctl (SIOCGIFADDR)");
	}
	sin = (struct sockaddr_in *) & ifr.ifr_addr;
	printf("\tinet %s ", inet_ntoa(sin->sin_addr));
	if (ifioctl(s, SIOCGIFNETMASK, (caddr_t) & ifr) < 0) {
		if (errno != EADDRNOTAVAIL)
			perror("ioctl (SIOCGIFNETMASK)");
		memset((char *) &ifr.ifr_addr, '\0', sizeof(ifr.ifr_addr));
	} else
		netmask.sin_addr =
			((struct sockaddr_in *) & ifr.ifr_addr)->sin_addr;
	if (flags & IFF_POINTOPOINT) {
		if (ifioctl(s, SIOCGIFDSTADDR, (caddr_t) & ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
				memset((char *) &ifr.ifr_addr, '\0',
					sizeof(ifr.ifr_addr));
			else
				perror("ioctl (SIOCGIFDSTADDR)");
		}
		sin = (struct sockaddr_in *) & ifr.ifr_addr;
		printf("--> %s ", inet_ntoa(sin->sin_addr));
	}
	printf("netmask %x ", ntohl(netmask.sin_addr.s_addr));
	if (flags & IFF_BROADCAST) {
		if (ifioctl(s, SIOCGIFBRDADDR, (caddr_t) & ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
				memset((char *) &ifr.ifr_addr, '\0', sizeof(ifr.ifr_addr));
			else
				perror("ioctl (SIOCGIFADDR)");
		}
		sin = (struct sockaddr_in *) & ifr.ifr_addr;
		if (sin->sin_addr.s_addr != 0)
			printf("broadcast %s", inet_ntoa(sin->sin_addr));
	}
	putchar('\n');
	if (flags & IFF_ONEPACKET) {
		if (ifioctl(s, SIOCGIFONEP, (caddr_t) &ifr) < 0) {
			perror("ioctl (SIOCGIFONEP)");
			ifr.ifr_onepacket.spsize = 0;
		}
		if (ifr.ifr_onepacket.spsize) {
			printf("\tone-packet mode params: packet size: %d; threshold: %d\n",
				ifr.ifr_onepacket.spsize, ifr.ifr_onepacket.spthresh);
		}
	}
	if (ifioctl(s, SIOCGIFPERF, (caddr_t) &ifr) < 0) {
		perror("ioctl (SIOCGIFPERF)");
		ifr.ifr_perf.ip_recvspace = 0;
	}
	if (ifr.ifr_perf.ip_recvspace) {
		printf("\tperf. params: recv size: %u; send size: %u; full-size frames: %d\n",
			ifr.ifr_perf.ip_recvspace, ifr.ifr_perf.ip_sendspace, 
			ifr.ifr_perf.ip_fullsize);
	}
}

in_alias(force)
	int             force;
	/* ARGSUSED */
{
	struct sockaddr_in *sin;

	struct in_aliasreq ifra;
	bcopy(&oifr.ifr_addr, &ifra.ifra_addr, sizeof(ifra.ifra_addr));
	bcopy(&oifr.ifr_name, &ifra.ifra_name, sizeof(ifra.ifra_name));

	if (ifioctl(s, SIOCGIFALIAS, (caddr_t) &ifra) < 0) {
		if (errno == EADDRNOTAVAIL || errno == EPROTONOSUPPORT) {
			return;
		} else {
			perror("ioctl (SIOCGIFALIAS)");
			return;
		}
	}
	sin = &ifra.ifra_addr;
	printf("\t(alias) inet %s ", inet_ntoa(sin->sin_addr));
	sin = &ifra.ifra_mask;
	printf("netmask %lx ", ntohl(sin->sin_addr.s_addr));
	sin = &ifra.ifra_broadaddr;
	if (flags & IFF_POINTOPOINT) {
		printf("\tdest %s ", inet_ntoa(sin->sin_addr));
	} else if (flags & IFF_BROADCAST) {
		printf("broadcast %s", inet_ntoa(sin->sin_addr));
	}
	printf("\n");
}

Perror(cmd)
	char           *cmd;
{
	extern int      errno;

	fprintf(stderr, "ifconfig: ");
	switch (errno) {

	case ENXIO:
		fprintf(stderr, "%s: no such interface\n", cmd);
		break;

	case EPERM:
		fprintf(stderr, "%s: permission denied\n", cmd);
		break;

	case ENODEV:
		fprintf(stderr, "%s: family not loaded\n", cmd);
		break;

	default:
		perror(cmd);
	}
	exit(1);
}

#define SIN(x) ((struct sockaddr_in *) &(x))
struct sockaddr_in *sintab[] = {
	SIN(ridreq.ifr_addr), SIN(addreq.ifra_addr),
	SIN(addreq.ifra_mask), SIN(addreq.ifra_broadaddr)
};

in_getaddr(s, which)
	char    *s;
	int	which;
{
	register struct sockaddr_in *sin = sintab[which];
	struct hostent *hp;
	struct netent  *np;
	struct in_addr  val;
	int	r;

	if (s == NULL) {
		fprintf(stderr, "ifconfig: address argument required\n");
		usage();
	}
	if (which != MASK)
		sin->sin_family = AF_INET;
	r = inet_aton(s, &val);
	if (r)
		sin->sin_addr = val;
	else if (hp = gethostbyname(s)) {
		sin->sin_family = hp->h_addrtype;
		memcpy((char *) &sin->sin_addr, hp->h_addr, hp->h_length);
	} else if (np = getnetbyname(s)) {
		sin->sin_family = np->n_addrtype;
		sin->sin_addr = inet_makeaddr(np->n_net, INADDR_ANY);
	} else {
		(void) fprintf(stderr, "ifconfig: %s: bad value\n", s);
		exit(1);
	}
}

/*
 * Print a value a la the %b format of the kernel's printf 
 */
int
printb(
	char	       *s,
	unsigned short	v,
	char	       *bits
	)
{
	register int    i, any = 0;
	register char   c;

	if (bits && *bits == 8)
		printf("%s=%o", s, v);
	else
		printf("%s=%x", s, v);
	bits++;
	if (bits) {
		putchar('<');
		while (i = *bits++) {
			if (v & (1 << (i - 1))) {
				if (any)
					putchar(',');
				any = 1;
				for (; (c = *bits) > 32; bits++)
					putchar(c);
			} else
				for (; *bits > 32; bits++);
		}
		putchar('>');
	}
}

usage()
{
	(void) fprintf(stderr, 
		"usage: ifconfig [-p] interface | -a [addr_family]\n%s%s%s%s%s%s%s%s%s%s%s%s",
		"\t[ address [ dest_addr ] ]\n",
		"\t[ up ] [ down ]\n",
		"\t[ netmask mask ]\n",
		"\t[ broadcast bc_addr ]\n",
		"\t[ metric n ]\n",
		"\t[ mtu n ]\n",
		"\t[ arp | -arp ]\n",
		"\t[ link0 | -link0 ] [ link1 | -link1 ] [ link2 | -link2 ]\n",
		"\t[ debug n ]\n",
		"\t[ alias addr | -alias addr ]\n",
		"\t[ perf recv send full-size ]\n",
		"\t[ onepacket size count | -onepacket ]\n");
	exit(1);
}

ifioctl(s, cmd, arg)
	char           *arg;
{
	struct strioctl ioc;

	ioc.ic_cmd = cmd;
	ioc.ic_timout = 0;
	switch(cmd) {
	case SIOCAIFADDR:
	case SIOCGIFALIAS:
		ioc.ic_len = sizeof(struct ifaliasreq);
		break;
	default:
		ioc.ic_len = sizeof(struct ifreq);
	}
	ioc.ic_dp = arg;
	return (ioctl(s, I_STR, (char *) &ioc));
}

donothing(name)
	char *name;
{
	fprintf(stderr,"ifconfig: attempt to set %s ignored\n",name);
	return;
}

#define _PATH_SLINK "/etc/slink"
#define FN_PREFIX "boot_"

plumb(char *name)
{
    pid_t pid;
    char *fn;

    switch (pid = fork()) {
    case -1:
	perror("fork");
	exit(1);
	/* NOTREACHED */
    case 0:
	/* child */
	fn = (char *)malloc(strlen(FN_PREFIX) + 1 + strlen(name));
	sprintf(fn, "%s%s", FN_PREFIX, name);

	execl(_PATH_SLINK, _PATH_SLINK, fn, NULL);
	perror("execl");
	exit(1);
	/* NOTREACHED */
    default:
	/* parent */
	(void)waitpid(pid, 0, 0);
	break;
    }
}
