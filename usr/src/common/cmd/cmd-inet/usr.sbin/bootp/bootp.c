#ident	"@(#)bootp.c	1.4"
#ident	"$Header:  "
/*      SCCS IDENTIFICATION        */
/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1991 INTERACTIVE Systems Corporation
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

/* cc -g -lnsl -lsocket -lresolv bootp.c */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <locale.h>
#include <pfmt.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <signal.h>

#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>
#include <sys/ioccom.h>

#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <sys/stropts.h>
#include <arpa/bootp.h>
#include <arpa/inet.h>
#include <string.h>
#include <macros.h>
#include <tiuser.h>
#include <sys/sockio.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <malloc.h>
#include <netdb.h>

#define MAXDNAME        256             /* maximum domain name */
#define NAMESERVER_PORT 53
#include <resolv.h>

#ifdef SVR4
#define bzero(p,l)	memset(p,0,l)
#define bcopy(a,b,c)	memcpy(b,a,c)
#define bcmp(a,b,c)	memcmp(a,b,c)
#endif /* SVR4 */

#define IP_DEVICE "/dev/ip"

static int ip = -1;	/* fd of IP_DEVICE */

static char	*ifname = NULL;
static char	*devname = NULL;
static char	*hostname = NULL;

/* state used for cleanup */
static unsigned long	old_ip_address = 0;
static unsigned long	old_broadcast_address = 0;
static unsigned long	old_netmask = 0;
static struct ortentry	route;
static int 		addrt_ret = -1;

static int	maxtries = 5;	/* Max number of times to send bootp request */
static unsigned char vendor_magic[] = VM_RFC1048;
static char 	byname_magic[] = BYNAME_MAGIC;

#define ETHER_ADDR_SIZE 6
#define LSIZE		40
#define DNSBUFSZ	2048


static struct bootp bp_req, bp_rep;

struct hostent * _rs_gethostbyaddr();

/* General exit cleanup and signal handler */
void getout(int signum)
{
	if (ifname) {
		/* Delete any route we added */
		if (addrt_ret == 0)
		  str_ioctl(ip, SIOCDELRT, (char *) & route, sizeof(route));

		/* Undo any interface reconfiguration */
		if (old_ip_address)
		  if (sifaddr(ifname, SIOCSIFADDR,
			      old_ip_address) < 0)
		    pfmt(stderr, MM_STD, ":1:Can't restore IP address\n");

		if (old_netmask)
		  if (sifaddr(ifname, SIOCSIFNETMASK,
			      old_netmask) < 0)
		    pfmt(stderr, MM_STD, ":24:Can't restore netmask\n");

		if (old_broadcast_address)
		  if (sifaddr(ifname, SIOCSIFBRDADDR,
			      old_broadcast_address) < 0) 
		    pfmt(stderr, MM_STD,
			 ":2:Can't restore broadcast address\n");
	}
	exit(signum);
}

/* resolver library overlay to avoid use of /etc/hosts */
struct hostent *
_rs__gethtbyaddr(addr, len, type)
     char *addr;
     int len, type;
{
	return ((struct hostent *) NULL);
}


/* packaged I_STR ioctl for convenience */
int str_ioctl(fd, cmd, data, size)
int	fd;
long	cmd;
char *	data;
int	size;
{
	struct strioctl		iocb;

	iocb.ic_cmd = cmd;
	iocb.ic_timout = 15;
	iocb.ic_len = size;
	iocb.ic_dp = data;

	return(ioctl(fd, I_STR, (char *)&iocb));
}


/* Set interface address */
int sifaddr(ifname, type, addr)
long	      type;
char	    * ifname;
long	      addr;
{
	struct ifreq		ifr;
	struct sockaddr_in	sin;
	int			rc;

	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	sin.sin_addr.s_addr = htonl(addr);
	bzero(&sin.sin_zero, 8);
	strcpy(ifr.ifr_name, ifname);
	ifr.ifr_addr = *(struct sockaddr *)&sin;
	rc = str_ioctl(ip, type, (char *)&ifr, sizeof(ifr));
	return (rc);
}


/* Get interface address */
long gifaddr(ifname, type)
long	      type;
char	    * ifname;
{
	struct ifreq		ifr;
	struct sockaddr_in	sin;
	int			rc;

	strcpy(ifr.ifr_name, ifname);
	rc = str_ioctl(ip, type, (char *)&ifr, sizeof(ifr));
	sin = *(struct sockaddr_in *) & ifr.ifr_addr;
	return (ntohl(sin.sin_addr.s_addr));
}

/* tli version of setsockopt for convenience */
setsockopt_tli(s, level, optname, optval, optlen)
int s, level, optname;
char *optval;
int optlen;
{

        struct t_optmgmt req;
        struct opthdr *opt;
        int ret;
        char buf[sizeof(struct opthdr) + LSIZE];

        if (optlen > LSIZE)             /* return error if optlen is */
                return(-1);             /* greater than LSIZE bytes. */
        bzero((char *)buf, sizeof(buf));
        opt = (struct opthdr *)buf;
        opt->level = level;
        opt->name = optname;
        opt->len = OPTLEN(optlen);
        bcopy(optval, OPTVAL(opt), optlen);

        req.flags = T_NEGOTIATE;
        req.opt.maxlen = req.opt.len = OPTLEN(sizeof(struct opthdr) + optlen);
        req.opt.buf = buf;

        ret = t_optmgmt(s, &req, &req);
        return(ret);
}

static getsocket(sin)
	struct sockaddr_in *sin;
{
	struct t_bind b;
	int sock, on;

	on = 1;
	if ((sock = t_open("/dev/udp", O_RDWR|O_NDELAY, 0)) < 0) {
		pfmt(stderr, MM_STD, ":3:open /dev/udp failed: %s\n", t_strerror(t_errno));
		return (-1);
	}
#ifdef SO_BROADCAST
	if (setsockopt_tli(sock, SOL_SOCKET, SO_BROADCAST, (char *)&on,
			   sizeof (on)) < 0) {
		pfmt(stderr, MM_STD, ":4:setsockopt SO_BROADCAST: %s\n", t_strerror(t_errno));
		t_close(sock);
		return (-1);
	}
#endif

	b.addr.len = b.addr.maxlen = sizeof(*sin);
	b.qlen = 0;
	b.addr.buf = (char *) sin;

	if (t_bind(sock, &b, &b) < 0) {
		pfmt(stderr, MM_STD, ":5:t_bind failed: %s\n", t_strerror(t_errno));
		t_close(sock);
		return(-1);
	}

	if (ntohs(sin->sin_port) != IPPORT_BOOTPC) {
		pfmt(stderr, MM_STD,
		     ":6:t_bind did not return the port requested.\n");
		t_unbind(sock);
		t_close(sock);
		return(-1);
	}

	return (sock);
}

/* format ethernet address for printing */
static char *
fmt_ether_addr(addr)
char	*addr;
{
	static char	 buf[20];
	char		*hexnum;
	int		 i;

	hexnum = "0123456789abcdef";
	for (i = 0; i < 17; addr++) {
		buf[i++] = hexnum[(0xf0 & *addr) >> 4];
		buf[i++] = hexnum[0x0f & *addr];
		buf[i++] = ':';
	}
	buf[--i] = 0;
	return((char *)buf);
}

static union DL_primitives primbuf;
static struct strbuf primptr = { 0, 0, (char *)&primbuf };

#define DLGADDR (('D' << 8) | 5)

/* Get ethernet address for device devname */
int getether(etheraddr)
u_char 	*etheraddr;
{
	int fd;
	struct strioctl strioc;
	unsigned char llc_mc[8];
	int ret, flags = 0;
	register dl_phys_addr_req_t *brp;
	register dl_phys_addr_ack_t *bap;
	dl_error_ack_t *bep;
	unsigned char   buf[80];                /* place for received table */
	unsigned char *cp;

	if ((fd = open(devname, O_RDWR|O_NONBLOCK)) == -1) {
		return(-1);
	}

	brp = (dl_phys_addr_req_t *)primptr.buf;
	memset(brp, 0, sizeof(dl_phys_addr_req_t));
	brp->dl_primitive = DL_PHYS_ADDR_REQ;
	brp->dl_addr_type = DL_CURR_PHYS_ADDR;
	primptr.len = sizeof(dl_phys_addr_req_t);

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(-1);

	primptr.maxlen = primptr.len = sizeof(primbuf) + 6;
	primptr.buf = (char *)buf;
	ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags);

	primptr.buf = (char *)&primbuf;

	if (ret < 0)
		return(-1);

	bap = (dl_phys_addr_ack_t *)buf;
	if (bap->dl_primitive == DL_PHYS_ADDR_ACK) {
		cp = buf + bap->dl_addr_offset;
	} else {
		strioc.ic_cmd = DLIOCGENADDR;
		strioc.ic_timout = 15;
		strioc.ic_len = LLC_ADDR_LEN;
		strioc.ic_dp = (char *)llc_mc;
		if (ioctl(fd, I_STR, &strioc) < 0) {
			/*
			 * dlpi token driver does not recognize DLIOCGENADDR,
			 * so try DLGADDR
			 */
			strioc.ic_cmd = DLGADDR;
			strioc.ic_timout = 15;
			strioc.ic_len = LLC_ADDR_LEN;
			strioc.ic_dp = (char *)llc_mc;
			if (ioctl(fd, I_STR, &strioc) < 0) {
				return(-1);
			}
		}
		cp = llc_mc;
	}
	bcopy(cp, etheraddr, ETHER_ADDR_SIZE);
#ifdef DEBUG
	pfmt(stderr, MM_INFO, ":8:Local ether address: %s\n",
	     fmt_ether_addr((char *)etheraddr));
#endif
	close(fd);
	return(0);
}

int main(argc, argv)
int argc;
char *argv[];
{
	int	i;
	int	n;
	int	fd;
	int	ntries;
	long	l, subnet_mask = 0, myaddr, gateway = 0;
	struct hostent *hent;
	char * dname;
	struct timeval	tim;
	int	tmask, timeout;
	int	flags;
	int c, usage=0, nodns=0, gotdomain=0, gothost=0;
	struct pollfd pfd;
	
	struct in_addr tmparg;
	struct sockaddr_in addrsend;
	struct sockaddr_in addrrec;
	struct t_unitdata udsend;
	struct t_unitdata udrecv;
	
	struct __res_state * rp;
	struct __res_state * get_rs__res();
	
	extern char *optarg;
	extern int optind;
	extern double strtod();

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxbootp");
	(void)setlabel("UX:bootp");

	signal(SIGHUP, getout);
	signal(SIGINT, getout);
	signal(SIGKILL, getout);
	signal(SIGPIPE, getout);
	signal(SIGTERM, getout);
	  
	while ((c = getopt(argc, argv, "da:c:")) != -1)
	  switch (c) {
		case 'd':	/* Don't use DNS */
		    nodns++;
		    break;
		case 'c':	/* maximum retry Count */
		    maxtries = strtod(optarg);
		    break;
		case 'a':	/* reset IP Address */
		    ifname = optarg;
		    break;
		case '?':
		    usage++;
	  }
	if (optind < argc)
	  devname=argv[optind++];
	else
	  usage++;

	if (optind < argc)
	  hostname=argv[optind++];
	
	if (usage) {
		pfmt(stderr,
		     MM_ACTION,
		     ":12: %s [-d] [-a interface_name] [-c number_of_tries] device [hostname]\n",
		     argv[0]);
		exit (1);
	}

	/* ip is need for several ioctl calls */
	if ((ip = open(IP_DEVICE, O_RDONLY)) < 0)
	  pfmt(stderr, MM_STD, gettxt(":25:open IP device: %s\n",
				      strerror(errno)));

	/* Prepare to send request packet */
	ntries = 1;
	bzero (&bp_req, sizeof(bp_req));	/* most fields default to 0 */
	
	bp_req.bp_op = BOOTREQUEST;
	
	bp_req.bp_htype = ARPHRD_ETHER;
	
	bp_req.bp_hlen = ETHER_ADDR_SIZE;
	
        (void)gettimeofday(&tim, NULL);
	srand((int)(tim.tv_sec + tim.tv_usec));
	bp_req.bp_xid = (long)rand() << 16 + rand();
	
	bp_req.bp_secs = 0;
	
	if (getether(bp_req.bp_chaddr))
	  getout(-1);   /* Could try to get by without ethernet address but
			 * RFC 951 does not seem to allow that. */

	/* request rfc 1084 vendor info */
	bcopy(vendor_magic, bp_req.bp_vend, 4);
	i=4;

	/* special magic to allow UnixWare server to find us by name */
	if (hostname) {
		bp_req.bp_vend[i++] = BYNAME_TAG; /* UnixWare magic tag */
		n = strlen(byname_magic);	/* magic tag length */
		bp_req.bp_vend[i++] = n;
		bcopy(byname_magic, bp_req.bp_vend+i, n);
		i += n+1;
		bp_req.bp_vend[i++] = TAG_HOST_NAME;
		n=strlen(hostname);
		if (n > 62-i) {
			n = 62-i;
			hostname[n] = '\0';
			pfmt(stderr, MM_WARNING,
			     ":13: hostname truncated to: %s\n",
			     hostname);
		}
		bp_req.bp_vend[i++] = n;
		bcopy(hostname, bp_req.bp_vend+i, n);
		i += n+1;
	}

	bp_req.bp_vend[i++] = TAG_END;

	/* receive from address */
	addrrec.sin_port = htons(IPPORT_BOOTPC);
	addrrec.sin_family = AF_INET;
	addrrec.sin_addr.s_addr = INADDR_ANY;
	bzero(&addrrec.sin_zero, 8);
	
	fd = getsocket((struct sockaddr_in *)&addrrec);
	if (fd < 0)
	  exit (-1);
	
	/* send to address */
	addrsend.sin_port = htons(IPPORT_BOOTPS);
	addrsend.sin_family = AF_INET;
	addrsend.sin_addr.s_addr = INADDR_BROADCAST; /* don't know net mask */
	bzero(&addrsend.sin_zero, 8);

	/* receive structure */
	udrecv.addr.maxlen = sizeof(addrrec);
	udrecv.addr.buf = (char *)&addrrec;
	udrecv.opt.maxlen = 0;
	udrecv.udata.maxlen = sizeof(bp_rep);
	udrecv.udata.buf = (char *)&bp_rep;

	/* send structure */
	udsend.addr.len = sizeof(addrsend);
	udsend.addr.buf = (char *)&addrsend;
	udsend.opt.maxlen = udsend.opt.len = 0;
	udsend.udata.buf = (char *)&bp_req;
	udsend.udata.maxlen = udsend.udata.len = sizeof(bp_req);
	
	tmask = 0x3ff;	/* Ave timeout of about 1 - 2 seconds to start */
	
	/* -a option indicates our current IP address may not be correct  */
	/* Set our IP address to zero.  This will cause IP to process all */
	/* IP packets the driver passes to it.  Even if the IP address    */
	/* does not match.						  */
	if (ifname) {
		old_ip_address = gifaddr(ifname, SIOCGIFADDR);
		old_broadcast_address = gifaddr(ifname, SIOCGIFBRDADDR);
		if (sifaddr(ifname, SIOCSIFADDR, 0UL) < 0 ||
		    sifaddr(ifname, SIOCSIFBRDADDR, 0xffffffffUL) < 0) 
			pfmt(stderr, MM_STD,
			     ":14:Can't configure interface\n");
	}
	
	while (1) {
		
		if (ntries++ > maxtries) {
			printf("INET_BOOTP_TIMEDOUT=yes\n");
			getout(-2);
		}

		if (t_sndudata(fd, (struct t_unitdata *)&udsend) < 0) {
			getout(-1);
		}

		timeout = (rand() & tmask) + tmask;
		bp_req.bp_secs += timeout / 1000;    /* no need for precision*/
		pfd.fd = fd;
		pfd.events = POLLIN;

get_response:
		switch (poll(&pfd,1UL,timeout)) {
		      case -1:
			pfmt(stderr, MM_STD, ":15:poll: %s\n",
			     strerror(errno));
			getout(-1);
		      case 0:
			/* timed out */
#ifdef DEBUG
			pfmt(stderr, MM_INFO, ":16:timeout = %d\n" ,timeout);
#endif
			tmask = (tmask << 1 | 1) & 0x3fff;
			/* send another request */
			continue;
		      case 1:
			if (pfd.revents != POLLIN) {
				pfmt(stderr, MM_STD,
				     ":17:unexpected event(s) %d\n",
				     pfd.revents);
				getout(-1);
			}
			/* get the response */
			if (t_rcvudata(fd,(struct t_unitdata *)&udrecv,&flags)
			    < 0) {
				if (t_look(fd) == T_UDERR) {
					struct t_uderr *uderr;
					uderr = (struct t_uderr *)
					  t_alloc(fd, T_UDERROR, 0);
					if (uderr == NULL) {
						pfmt(stderr, MM_STD,
						     ":18:t_alloc: %s\n",
						     strerror(errno));
						getout(-1);
					}
					t_rcvuderr (fd, uderr);
					errno = uderr->error;
					pfmt(stderr, MM_STD,
					     ":19:t_rcvudata: %s\n",
					     strerror(errno));
					t_free ((char *)uderr, T_UDERR);
				}
				else
				  pfmt(stderr, MM_STD, ":20:t_rcvudata: %s\n", t_strerror(t_errno));
				getout(-1);
			}

			/* verify it was our request */
			if (bp_req.bp_xid != bp_rep.bp_xid ||
			    bp_req.bp_hlen != bp_rep.bp_hlen ||
			    bp_rep.bp_op != BOOTREPLY ||
			    bcmp((char *)bp_req.bp_chaddr,
				    (char *)bp_rep.bp_chaddr,
				    bp_rep.bp_hlen) ||
			    bcmp((char *)bp_rep.bp_vend,
				    (char *)vendor_magic, 4))
			  continue;

			/*
			 * Kludge to avoid bogus response sent by NetWare LWG
			 * BOOTP server because of "Automatic IP Address
			 * Assignment":  Pare vend area looking for hostname
			 * starting with "UNKNOWN".  The current LWG
			 * bootp server places this in the vend area
			 * because it does not know our host name.  It
			 * does not know our host name because it
			 * only expects the host name field to be the
			 * first tag.  LWG responses generated because
			 * of static assignments will not have the
			 * "UNKNOWN" hostname since they will get the
			 * hostname from the bootptab file.  We have
			 * spoken with the netware devlopers and
			 * requested that they not change this behavior
			 * without taking measures to avoid further
			 * interoperability problems.
			 */
			for(i=4; i<64; i++)
			  switch (bp_rep.bp_vend[i]) {
				case TAG_PAD:
				  break;

				case TAG_HOST_NAME:
				  if (!bcmp(bp_rep.bp_vend+i+2, "UNKNOWN", 7)) {
#ifdef DEBUG
					  pfmt(stderr, MM_INFO,
					    ":26:Ignoring response from %s\n",
					    inet_ntoa(bp_rep.bp_siaddr));
#endif
					  goto get_response;
				  }
				  /* Fall through.  We don't need to examine */
				  /* any more tags. */
				case TAG_END:
				  i=64;
				  break;
				default: /* unused tag */
				  i+=bp_rep.bp_vend[i+1]+1;
				  break;
			  }

			myaddr=bp_rep.bp_yiaddr.s_addr;
			printf("INET_YOUR_IP_ADDRESS=%s\n",
			  inet_ntoa(bp_rep.bp_yiaddr));
			printf("INET_SERVER_IP_ADDRESS=%s\n",
			  inet_ntoa(bp_rep.bp_siaddr));
			bp_rep.bp_file[BP_FILE_LEN-1] = '\0';/* just in case */
			printf("INET_BOOT_FILE_NAME=%s\n", bp_rep.bp_file);

			/* parse some RFC 1533 vend subfields */
			for(i=4; i<64; i++)
			  switch (bp_rep.bp_vend[i]) {
				case TAG_PAD:
				  break;
				case TAG_SUBNET_MASK:
				  /* copy to avoid alignment problems */
				  bcopy(bp_rep.bp_vend+i+2, &subnet_mask, 4);
				  printf("INET_SUBNET_MASK=0x%lX\n",
					 ntohl(subnet_mask));
				  i+=5;
				  tmparg.s_addr
				      = (subnet_mask & myaddr) | ~subnet_mask;
				  printf("INET_BROADCAST_ADDRESS=%s\n",
				         inet_ntoa(tmparg));
				  break;
				case TAG_DOMAIN_SERVER:
				  res_init();
				  rp = get_rs__res();
				  /* other rp members could also be set here */
				  rp->nscount = 0;
				  rp->retrans = 1;
				  if (maxtries < 2)
					rp->retry = 2;
				  else if (maxtries > 4)
					rp->retry = 4;
				  else 
				  	rp->retry   = maxtries;

				  for(n=1; n<(int)bp_rep.bp_vend[i+1]; n+=4) {
					  int j;

					  /* avoid alignment problems */
					  bcopy(bp_rep.bp_vend+i+1+n, &l, 4);
					  tmparg.s_addr = l;
					  j = n / 4; /* index to nsaddr_list */
					  printf("INET_DNS_SERVER[%i]=%s\n",
						 j, inet_ntoa(tmparg));
					  if (j < 3) {
						  rp->nsaddr_list[j].sin_addr.s_addr = l;
						  rp->nsaddr_list[j].sin_family = AF_INET;
						  rp->nsaddr_list[j].sin_port = htons(NAMESERVER_PORT);
						  rp->nscount++;
					  }
				  }
				  i += bp_rep.bp_vend[i+1]+1;
				  break;
				case TAG_GATEWAY:
				  for(n=1; n<(int)bp_rep.bp_vend[i+1]; n+=4) {
					  /* avoid alignment problems */
					  bcopy(bp_rep.bp_vend+i+1+n, &gateway,
						4);
					  tmparg.s_addr = gateway;
					  printf("INET_ROUTER[%i]=%s\n",
						 n/4, inet_ntoa(tmparg));
				  }
				  i += bp_rep.bp_vend[i+1]+1;
				  break;
				case TAG_TIME_SERVER:
				  for(n=1; n<(int)bp_rep.bp_vend[i+1]; n+=4) {
					  /* avoid alignment problems */
					  bcopy(bp_rep.bp_vend+i+1+n, &l, 4);
					  tmparg.s_addr = l;
					  printf("INET_TIME_SERVER[%i]=%s\n",
						 n/4, inet_ntoa(tmparg));
				  }
				  i += bp_rep.bp_vend[i+1]+1;
				  break;
				case TAG_TIME_OFFSET:
				  /* copy to avoid alignment problems */
				  bcopy(bp_rep.bp_vend+i+2, &l, 4);
				  printf("INET_TIME_OFFSET=%ld\n",
					 ntohl(l));
				  i+=5;
				  break;
				case TAG_HOST_NAME:
				  printf("INET_HOSTNAME=%s\n",
					 bp_rep.bp_vend+i+2);
				  i+=bp_rep.bp_vend[i+1]+1;
				  gothost++;
				  break;
				case TAG_DOMAIN_NAME:
				  printf("INET_DOMAIN_NAME=%s\n",
					 bp_rep.bp_vend+i+2);
				  i+=bp_rep.bp_vend[i+1]+1;
				  gotdomain++;
				  break;

#define TAG_ETHER_ENCAPSULATION	((unsigned char)  36)
				case TAG_ETHER_ENCAPSULATION:
				  printf("INET_ETHER_ENCAPSULATION=%s\n",
					 bp_rep.bp_vend[i+2] ?
					 "ETHER_SNAP" :
					 "ETHER_II");
				  i+=bp_rep.bp_vend[i+1]+1;
				  break;

				case 40: /* NIS_DOMAIN_NAME */
				  printf("INET_NIS_DOMAIN_NAME=%s\n",
					 bp_rep.bp_vend+i+2);
				  i+=bp_rep.bp_vend[i+1]+1;
				  break;
				case TAG_END:
#ifdef DEBUG
				  pfmt(stderr, MM_INFO,
				       ":21:%d/64 vend bytes used\n",i);
#endif
				  i=64;
				  break;
				default: /* unused tag */
#ifdef DEBUG
				  pfmt(stderr, MM_INFO, ":22:unused tag %d\n",
					  bp_rep.bp_vend[i]);
#endif
				  i+=bp_rep.bp_vend[i+1]+1;
				  break;
			  }

			/* if the response did not include domain or host */
			/* name and if we are allowed to use dns then get */
			/* names via reverse dns lookup 		  */
			   
			if (!nodns && (!gotdomain || !gothost)) {
				if (ifname) {
					old_netmask = gifaddr(ifname,
							      SIOCGIFNETMASK);
					if (sifaddr(ifname, SIOCSIFADDR,
						    ntohl(myaddr)) < 0 ||
					    (subnet_mask && sifaddr(ifname, SIOCSIFNETMASK,
						    ntohl(subnet_mask)) < 0))
					  pfmt(stderr, MM_STD,
					       ":14:Can't configure interface\n");

					/* Add default route if available */
					if (gateway) {
						bzero(&route, sizeof(route));
						((struct sockaddr_in *)
						 (&route.rt_dst))->sin_family =
						   AF_INET;
						/* default route */
						((struct sockaddr_in *)
						 (&route.rt_dst))->sin_addr =
						   inet_makeaddr(0,
								 INADDR_ANY);
						((struct sockaddr_in *)
						 (&route.rt_gateway))
						  ->sin_family = AF_INET;
						/* gateway address */
						((struct sockaddr_in *)
						 (&route.rt_gateway))->sin_addr
						   .s_addr = gateway;
						route.rt_flags = RTF_UP
						  | RTF_GATEWAY;
						route.rt_proto = RTP_LOCAL;
						addrt_ret = str_ioctl(ip,SIOCADDRT,
							    (char *) & route,
							    sizeof(route));
					}

				}
				if (hent=_rs_gethostbyaddr((char *)&myaddr, 4,
						       AF_INET)) {
					dname=strchr(hent->h_name, '.');
					if (dname) {
						*dname='\0';
						printf("INET_DOMAIN_NAME=%s\n",
						       dname+1);
					}
					printf("INET_HOSTNAME=%s\n"
					       ,hent->h_name);
				}
			}

			getout(0);
		      default:
			pfmt(stderr,
			     MM_STD, ":23:unexpected return code events=%d\n",
			     pfd.revents);
			getout(-1);
		}
	}
}
