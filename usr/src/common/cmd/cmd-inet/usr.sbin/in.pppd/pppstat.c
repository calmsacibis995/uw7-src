#ident	"@(#)pppstat.c	1.2"
#ident	"$Header$"
extern char *gettxt();
#ident "@(#) $Id: pppstat.c,v 1.4 1994/11/29 23:36:44 neil Exp"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1995 Legent Corporation
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
/*      SCCS IDENTIFICATION        */


#ifndef _KMEMUSER
#define _KMEMUSER
#endif

#include <sys/types.h>
#include <stdio.h>
#include <pwd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stropts.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/signal.h>
#include <sys/syslog.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>

#ifndef _KERNEL
#define _KERNEL
#include <sys/ksynch.h>
#undef _KERNEL
#else
#include <sys/ksynch.h>
#endif

#include <netinet/ppp_lock.h>
#include <netinet/ppp.h>
#include "pppd.h"
#include "pppu_proto.h"
#include <termios.h>
#include "pathnames.h"
#include <errno.h>

#include <pfmt.h>
#include <locale.h>
#include <unistd.h>


char *program_name;

int	pid, s ;

#define TIMEOUT  10

extern int errno;

/* SIGNAL Processing
 */

int cflag = 0;
int mflag = 0;
int kflag = 0;
int pflag = 0;

#define L_TYPE_MSG_NO   0
#define L_TYPE_MSG_STR  1
char *link_type_strings[][2] = { {":266", "INVALID TYPE"},
			      {":267", "Dynamic Incoming Link"},
			      {":268", "Dynamic Outgoing Link"},
			      {":269", "Static Link"},
			      {":270", "Outgoing Manual Bringup"}};


#define C(x)    ((x) & 0xff)

/*
 * sig_alm - SIGALM signal handler
 */
sig_alm()
{
	pfmt(stderr, MM_ERROR, 
		":271:No response from pppd, try later\n");
	syslog(LOG_INFO, "sig_alm");
	close(s);
	exit(0);
}

/*
 * sig_term - SIG_TERM signal Handler
 */

sig_term()
{
  msg_t msg;
  int fd;

  printf(gettxt(":272", "Disconnecting from pppd\n"));
  fflush(stdout);
  
  if ((fd = ppp_sockinit()) == -1) {
    pfmt(stderr, MM_ERROR, ":273:Can't connect to pppd to shut down monitor\n");
    exit(1);
  }

  msg.m_type = MONITOR_CLOSE;

  write(fd, &msg, sizeof(msg_t));

  exit(0);
}
  
  

main (argc,argv)
	int	argc;
	char	*argv[];
{
	msg_t msg;
	int	c, port;
	int	rval,totread;
	struct	ppp_stat stat;
	int size;
	struct termios term;
	char *p;
	int msg_sz;
	char ch;
	char debug_buf[512];
	int n;
	int num_flags = 0;

        (void)setlocale(LC_ALL,"");     
        (void)setcat("uxppp");
        (void)setlabel("UX:pppstat");     

	program_name = strrchr(argv[0],'/');
	program_name = program_name ? program_name + 1 : argv[0];

#if defined(LOG_DAEMON)
	openlog(program_name, LOG_PID|LOG_CONS|LOG_NDELAY, LOG_DAEMON);
#else
	openlog(program_name, LOG_PID);
#endif


	while ((ch = getopt(argc, argv, "cmkp")) != EOF) {
	  switch (ch) {
	  case 'c':   /* Print Link Connection Status */
	    num_flags++;
	    cflag++;
	    break;
	    
	  case 'm':
	    num_flags++;
	    mflag++;
	    break;

	  case 'k':
	    num_flags++;
	    kflag++;
	    break;

	  case 'p':
	    num_flags++;
	    pflag++;
	    break;
	    
	  default:
	    pfmt(stderr, MM_ERROR, ":274:usage: pppstat [-c | -m | -k | -p]\n");
	    exit(1);
	  }
	}

	if (num_flags > 1) {
	  pfmt(stderr, MM_ERROR, ":275:Can't use -m, -p, -c, or -k option together\n");
	  exit(1);
	}

	if (kflag) {
	  print_kernel_msgs();
	  exit(0);
	}

	sigset(SIGALRM, sig_alm);

	memset((char *)&msg, 0, sizeof(msg));
	
	if (cflag)
	  msg.m_type = CSTAT; /* Get Connectin Status */
	else if (mflag)
	  msg.m_type = MONITOR; /* Print Debugging info */
	else if (pflag)
	  msg.m_type = POOLSTATS;
	else
	  msg.m_type = MSTAT;

	s = ppp_sockinit();
	if (s < 0) {
		syslog(LOG_INFO, gettxt(":82", "can't connect to pppd"));
		pfmt(stderr, MM_ERROR, ":276:%s can't connect to pppd\n", program_name);
		exit(1);
	}

	/* Send message to pppd */

	rval = write(s, (char *)&msg, sizeof(msg));

	if (rval < 0) {
		syslog(LOG_INFO, gettxt(":83", "write to socket failed: %m"));
		pfmt(stderr, MM_ERROR, ":94:%s write to socket fail\n",program_name);
		exit(1);
	}

	/*
	 * If we are monitoring, print messages to standard out
	 */

	if (mflag) {
	  signal(SIGTERM, sig_term);
	  signal(SIGINT, sig_term);

	  printf(gettxt(":277", "Connected to pppd: Printing debug messages\n"));
	  while (1) {
	    fflush(stdout);
	    if ((n = read(s, &debug_buf, sizeof(debug_buf))) <= 0) {
	      pfmt(stderr,MM_ERROR, ":278:Lost Connection to pppd\n");
	      exit(1);
	    }

	    write(1, debug_buf, n);
	    write(1, "\n", 1);
	  }
	}
	  
	/* Get address pool status from deamon */

	if (pflag) {
	  signal(SIGTERM, sig_term);
	  signal(SIGINT, sig_term);

	  while (1) {
	    if ((n = read(s, &debug_buf, sizeof(debug_buf))) <= 0) {
	      exit(0);
	    }
	    write(1, debug_buf, n);
	    
	  }
	}

	/*
	 * Print connection status
	 */

	if (cflag) {
	  print_conn_stat(s);
	  exit(0);
	}


	/* Prepare to receive a pppstat message from pppd 
	 * We quit after TIMEOUT sec.
	 */	
	totread =0;
	
	p = (char *)& stat;
	memset(p, 0 , size = sizeof(stat));

	alarm(TIMEOUT);   
	do {
		if((rval = read(s,p, size - totread)) <= 0){
			syslog(LOG_ERR,gettxt(":279", "read socket fail: %m"));
			pfmt(stderr, MM_ERROR, ":280:%s read socket fail: %s\n",
				program_name, strerror(errno));
			close(s);
			exit(1);
		}
		p += rval;
		totread += rval;
	} while (totread < size);

	close(s);

	/*
	 * Dump PPP statistics structure. 
	 */
	printf("%s:\n", "ppp");
	
	printf(gettxt(":281", "\t%lu outbound connection requests\n"),stat.out_req);
	printf(gettxt(":282", "\t%lu inbound connection requests\n"),stat.in_req);
	printf(gettxt(":283", "\t%lu connections established\n"),stat.estab_con);
	printf(gettxt(":284", "\t%lu connections closed\n"),stat.closed_con);
	printf(gettxt(":285", "\t%lu password authentication failures\n"),stat.fail_pap);
	printf(gettxt(":286", "\t%lu chap authentication failures\n"),stat.fail_chap);
	printf(gettxt(":287", "\t%lu packets sent\n"),stat.opkts);
	printf(gettxt(":288", "\t%lu received packets with bad FCS\n"),stat.fcs);
	printf(gettxt(":289", "\t%lu received packets with bad address\n"),stat.addr);
	printf(gettxt(":290", "\t%lu received packets with bad control\n"),stat.ctrl);
	printf(gettxt(":291", "\t%lu received packets with bad protocol\n"),stat.proto);
	printf(gettxt(":292", "\t%lu correct packets received\n"),stat.ipkts);
	printf(gettxt(":293", "\t\t%lu packets with bad id field\n"),stat.id);
	printf(gettxt(":294", "\t\t%lu loopback packets\n"),stat.loopback);
	printf(gettxt(":295", "\t%lu state table errors\n"),stat.stattbl);

}


int
print_address(in)
struct	sockaddr_in *in;
{
  struct hostent *hp;

  in->sin_addr.s_addr = ntohl(in->sin_addr.s_addr);

  hp = gethostbyaddr((char *) in, sizeof(struct sockaddr_in), AF_INET);

  if (hp) 
    printf("%s", hp->h_name);
  else
    printf("%u.%u.%u.%u", C(in->sin_addr.s_addr >> 24),
	                    C(in->sin_addr.s_addr >> 16),
	                    C(in->sin_addr.s_addr >> 8),
		            C(in->sin_addr.s_addr));

  return(0);
}

int
print_kernel_msgs()
{
  int pppfd;
  fd_set infds, outfds, exfds;	
  int r;

  printf(gettxt(":296", "Printing debug messages from the ppp driver\n"));
  fflush(stdout);

  if ((pppfd = open(_PATH_PPP, O_RDWR|O_NDELAY)) < 0) {
    pfmt(stderr, MM_ERROR, ":297:open of %s fail: %s", _PATH_PPP, strerror(errno));
    exit(1);
  } else {
    /* set up to recv logging messages */
    if (ioctl(pppfd, SIOCSLOG, (caddr_t)NULL, 0) < 0) {
      if (errno == EBUSY) {
	pfmt(stderr, MM_ERROR, ":298:Someone already printing kernel messages\n");
	exit(1);
      }
      else {
    	pfmt(stderr, MM_ERROR, ":299:sioctl(SIOCSMGMT) fail: %s" , strerror(errno));
	exit(1);
      }
    }
  }

  while (1) {
    FD_ZERO(&outfds);
    FD_ZERO(&infds);
    FD_ZERO(&exfds);

    FD_SET(pppfd, &infds);
    FD_SET(pppfd, &exfds);

    r = select(20, &infds, &outfds, &exfds, (struct timeval *)0);
    if (r < 0) {
      if (errno == EINTR) {
	continue;
      }
      perror(gettxt(":300", "select fail"));
      continue;
    }
    if (FD_ISSET(pppfd, &infds) || FD_ISSET(pppfd, &exfds)) 
      ppplog(pppfd);
  }

}

/*
 * ppp debugging messages from /dev/ppp
 */
ppplog(pppfd)
	int pppfd;
{
	struct ppp_log_ctl_s ctlbuf;
	struct ppp_log_dt_s databuf;
	struct strbuf ctlsb, datasb;
	char	fmt[LOGSIZE + 20];
	int	flgs = 0;

do_over:
	ctlsb.maxlen = sizeof(ctlbuf);
	ctlsb.len = 0;
	ctlsb.buf = (char *) &ctlbuf;
	datasb.maxlen = sizeof(databuf);
	datasb.len = 0;
	datasb.buf = (char *) &databuf;
	
	if (getmsg(pppfd, &ctlsb, &datasb, &flgs) < 0) {
		if (errno = EAGAIN)
			return;
		perror(gettxt(":301", "ppplog() getmsg failed"));
		return;
	}

	if (ctlsb.len < sizeof(int)) {
		goto do_over;
	}
	
	switch (ctlbuf.function) {

	case PPP_LOG:
		if (datasb.len < sizeof(databuf)) {
			goto do_over;
		}

		sprintf(fmt, gettxt(":149", "Link id(%d):"), ctlbuf.l_index);
	
		strcat(fmt, databuf.fmt);
	
		printf(fmt, databuf.arg1, databuf.arg2, databuf.arg3, databuf.arg4);
		printf("\n");
		break;
	default:
		printf(gettxt(":302", "Get a unknown log message\n")); 
		break;
	}
	fflush(stdout);
}

/*
 * Routine to print connection stats
 */
struct  conn_made_s *conn_made = NULL;
struct  conn_conf_s *conn_conf = NULL;
int
print_conn_stat(fd)
int fd;
{
  int cnt = 0;
  struct hostent *hp;
  struct conn_conf_s *cf;
  struct conn_made_s *cm;
  u_char *p;
  int conn_lost = 0;
  int r;
  char buf[sizeof(struct conn_made_s)];
  struct conn_made_s *next, *prev;

  /*
   * Get conn_conf and conn_made lists from ppp deamon
   */
  
  cm = CONN_MADE_ADD();
  next = cm->next;
  prev = cm->prev;
  p = (u_char *) cm;
  cnt = 0;

  while (1) {
    while ((r = read(fd, buf, sizeof(struct conn_made_s))) > 0){

      if ((cnt+r) > sizeof(struct conn_made_s)) {
	/* 
	 * Remainder of current conn_made_s struct and
	 * start of a new one
	 */

	/* Copy remainder of current conn_made_s */
	memcpy(p, buf, sizeof(struct conn_made_s) - cnt);

	/* Restore next and prev pointers */
	cm->prev = prev;
	cm->next = next;
	
	/* Allocate new conn_made_s structure */
	cm = CONN_MADE_ADD();
	p = (u_char *) cm;
	next = cm->next;
	prev = cm->prev;
	
	/* Copy portion (or all) of next conn_made_s */
	memcpy(p, &buf[sizeof(struct conn_made_s) - cnt],
	       r - (sizeof(struct conn_made_s) - cnt));
	
	cnt = r - (sizeof(struct conn_made_s) - cnt);
	p += cnt;
      }
      else {
	/* cnt <= sizeof(struct conn_made_s) */
	memcpy(p, buf, r);
	cnt += r;
	p += r;

	if (cnt == sizeof(struct conn_made_s)) {
	  /* Restore next and prev pointers */
	  cm->prev = prev;
	  cm->next = next;

	  /* Allocate new conn_made_s structure */
	  cm = CONN_MADE_ADD();
	  p = (u_char *) cm;
	  next = cm->next;
	  prev = cm->prev;
	  cnt = 0;
	}
      }
	
    }
    
    if (r <= 0) {
      CONN_MADE_REMOVE(cm);
      break; /* We have read everything that we could */
    }
  }
  
  /*
   * Dump Connection Stats
   */
  printf(gettxt(":303", "Established PPP Connections:\n"));
  for (cm = conn_made; cm; cm = cm->next) {
    
    printf("%s:\n", cm->ifname);
    
    printf(gettxt(":304", "\tLocal Address:\t\t\t"));
    print_address(&cm->src);
    printf("\n");
    
    printf(gettxt(":305", "\tRemote Address:\t\t\t"));
    print_address(&cm->dst);
    printf("\n");
	      
    printf(gettxt(":306", "\tTTY Device:\t\t\t%s\n"), cm->tty_name);
    printf(gettxt(":307", "\tSpeed:\t\t\t\t%d\n"), cm->speed);
    
    printf(gettxt(":308", "\tFlow Control:"));
    if (cm->flow == XONXOFF)
      printf(gettxt(":309", "\t\t\tXONXOFF\n"));
    else if (cm->flow == RTSCTS)
      printf(gettxt(":310", "\t\t\tHARDWARE\n"));
    else
      printf(gettxt(":311", "\t\t\tNONE\n"));
    
    printf(gettxt(":312", "\tLink Type:\t\t\t%s\n"), 
		gettxt(link_type_strings[cm->type][L_TYPE_MSG_NO], 
			link_type_strings[cm->type][L_TYPE_MSG_STR]));
    printf(gettxt(":313", "\tLINK ID:\t\t\t%d\n"), cm->muxid);
    printf(gettxt(":314", "\tPPP Shell or Dial Process PID:"));
    if (cm->pgrp) 
      printf(gettxt(":315", "\t%d\n"), cm->pgrp);
    else
      printf(gettxt(":316", "\tNONE\n"));
    
    printf(gettxt(":317", "\tFilter Tag:\t\t\t%s\n"), cm->tag);
    printf(gettxt(":318", "\tProxy Arp:\t\t\t%s\n"), cm->proxy ? gettxt(":319", "YES") : gettxt(":320", "NO"));
    printf(gettxt(":321", "\tDebug Level:\t\t\t%d\n"), cm->debug);
    if (cm->attach_name[0] == '\0' )
    	printf(gettxt(":322", "\tAttach Name:\t\t\t%s\n"),gettxt(":323", "NONE") );
    else
    	printf(gettxt(":322", "\tAttach Name:\t\t\t%s\n"), cm->attach_name);
  } /* for */

  return(0);
}
  

