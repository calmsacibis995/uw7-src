#ident "@(#)main.c	1.6"
#ident "$Header$"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <nlist.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stream.h>
#include <sys/stat.h>
#include <netinet/in.h>
#undef NOERROR
#include <arpa/nameser.h>
#include <resolv.h>
#include <paths.h>
#include <sys/strstat.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <sys/ksym.h>
#include <sys/elf.h>

#define ROUTE_SYMBOL "radix_node_head"	/* symbol for head of routing tree */
off_t route_addr;		/* address of radix_node_head in /dev/kmem */

char didproto[256];

struct protox {
	u_char   pr_wanted;		/* 1 if wanted, 0 otherwise */
	int      (*pr_cblocks) ();	/* control blocks printing routine */
	int      (*pr_stats) ();	/* statistics printing routine */
	char     *pr_name;		/* well-known name */
};

#include "proto.h"
struct protox protox[] = {
	{ 1, protopr, tcp_stats, "tcp" },
	{ 1, protopr, udp_stats, "udp" },
	{ 1, protopr, ip_stats, "ip" },
	{ 1, 0, icmp_stats, "icmp" },
	{ 1, 0, igmp_stats, "igmp" },
	{ 0, 0, 0, 0 }
};

char	       *kmemf = _PATH_KMEM;
int             kmem;
char	       *wantpr;
int             Aflag = 0;
int             aflag = 0;
int		dflag = 0;
int		gflag = 0;
int             iflag = 0;
int		Lflag = 0;
int             nflag = 0;
int		pflag = 0;
int             rflag = 0;
int             sflag = 0;
int		tflag = 0;

int             interval;
char           *interface = (char *)0;
int             af = AF_UNSPEC;

usage()
{
	fprintf(stderr,"usage: netstat [");
	fprintf(stderr,"-Aan] [-u] [-f family] [-p proto]\n");
	fprintf(stderr,"               [-gLinrs] [-u] [-f family] [-p proto]\n");
	fprintf(stderr,"               [-n] [-I interface] [-w interval]\n");
	exit(1);
	/* NOTREACHED */
}

main(argc, argv)
	int             argc;
	char           **argv;
{
	register struct protoent *p;
	register struct protox *tp;
	extern char *optarg;
	extern int optind;
	int c;

	while ((c = getopt(argc, argv, "Aauginrsp:f:I:w:L")) != EOF) {
		switch (c) {
		case 'A':
			Aflag++;
			break;

		case 'a':
			aflag++;
			break;

		case 'g':
			gflag++;
			break;

		case 'u':
			af = AF_UNIX;
			break;

		case 'i':
			iflag++;
			break;

		case 'L':
			Lflag++;
			break;

		case 'n':
			nflag++;
			break;

		case 'r':
			rflag++;
			break;

		case 's':
			sflag++;
			break;

		case 'p':
			wantpr = optarg;
			if (!getprotobyname(wantpr)) {
				fprintf(stderr,
				 "netstat: unknown or uninstrumented protocol: %s\n", wantpr);
				exit(1);
			}
			af = AF_INET;
			pflag++;
			break;

		case 'f':
			if (strcmp(optarg, "inet") == 0) {
				af = AF_INET;
			}
			else {
				if (strcmp(optarg, "unix") == 0)
					af = AF_UNIX;
				else {
					fprintf(stderr, "netstat: %s: unknown address family\n",
							optarg);
					exit(1);
				}
			}	
			break;

		case 'I':
			dflag++;
			interface = optarg;
			break;

		case 'w':
			interval = atoi(optarg);
			if (interval <= 0)
				usage();
			iflag++;
			break;

		case '?':
		default:
			usage();
				/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;
	if (argc > 0 && isdigit(argv[0][0])) {
		interval = atoi(argv[0]);
		if (interval <= 0)
			usage();
		argv++, argc--;
		iflag++;
	}
	if (Lflag || gflag || (Aflag && rflag))
		vaddrinit(kmemf, "netstat", 0);

	if (Lflag) {
		lockstats();
		exit(0);
	}

	/*
	 * Keep file descriptors open to avoid overhead of open/close on each
	 * call to get* routines. 
	 */
	sethostent(1);
	setnetent(1);
	if (iflag || dflag) {
		intpr(interval);
		exit(0);
	}
	if (rflag) {
		unsigned long info;

		if (Aflag && getksym(ROUTE_SYMBOL,
				     (unsigned long *)&route_addr,
				     &info) < 0) {
			fprintf(stderr, "netstat: cannot find address of %s symbol\n",
				ROUTE_SYMBOL);
			exit(1);
		}
		if (sflag)
			rt_stats();
		else
			routepr(route_addr);
		exit(0);
	}
	if (gflag) {
		if (sflag)
			mrt_stats();
		else
			mroutepr();
		exit(0);
	}
	if (af == AF_INET || af == AF_UNSPEC) {
		setprotoent(1);
		setservent(1);
		while (p = getprotoent()) {

			if (pflag) {
				if (strcmp(wantpr, p->p_name))
					continue;
			}
			for (tp = protox; tp->pr_name; tp++)
				if (strcmp(tp->pr_name, p->p_name) == 0)
					break;
			if (tp->pr_name == 0 || tp->pr_wanted == 0)
				continue;
			if (didproto[p->p_proto] == 1) {
				continue;
			}
			didproto[p->p_proto] = 1;
			if (sflag) {
				if (tp->pr_stats)
					(*tp->pr_stats) (p->p_name);
			} else if (tp->pr_cblocks)
				(*tp->pr_cblocks) (p->p_name);
		}
		endprotoent();
	}
	if (af == AF_UNIX || af == AF_UNSPEC) {
		if (!sflag && !pflag) {
			pr_ux_stats();
		}	
	}
	exit(0);
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


int             memfd;
#define vtop(x,y)	x

/*ARGSUSED*/
vaddrinit(mem, name, flag)
	char           *mem;
	char           *name;
{
	if ((memfd = open(mem, 0)) < 0)
		error("%s: can't open %s\n", name, mem);
}

/*ARGSUSED*/
seekmem(addr, mode, proc)
	off_t            addr;
	int             mode, proc;
{
	long            paddr;
	extern long     lseek();

	if (mode)
		paddr = vtop(addr, proc);
	else
		paddr = addr;
	if (paddr == -1)
		error("%x is an invalid address\n", addr);
	if (lseek(memfd, paddr, 0) == -1)
		error("seek error on address %x\n", addr);
}

/* lseek and read */
int
readmem(addr, mode, proc, buffer, size, name)
	off_t            addr;
	int             mode, proc;
	char           *buffer;
	unsigned        size;
	char           *name;
{
	seekmem(addr, mode, proc);
	if (read(memfd, buffer, size) != size)
		error("read error on %s (addr 0x%x, size %d)\n", name, addr, size);
	return 0;
}

#ifdef __STDC__
error(char *string, ...)
#else
error(string, va_list)
	char           *string;
	va_dcl
#endif
{
	va_list args;
#ifdef __STDC__
	va_start(args, string);
#else
	va_start(args);
#endif
	vfprintf(stderr, string, args);
	va_end(args);
	exit(1);
}
