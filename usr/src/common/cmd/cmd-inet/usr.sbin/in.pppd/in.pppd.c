#ident	"@(#)in.pppd.c	1.2"
#ident	"$Header$"
extern char *gettxt();
#ident "@(#) $Id: pppd.c,v 1.8 1994/12/16 15:14:06 stevea Exp"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Legent Corporation
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
#include <sys/types.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/param.h>
#include <stdio.h>
#include <sys/timeb.h>
#include <errno.h>
#include <syslog.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <termios.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
/* STREAMS includes */
#include <setjmp.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/socket.h>

#include <sys/sockio.h>
#include <stdlib.h>
#include <pfmt.h>
#include <locale.h>
#include <unistd.h>


#include <poll.h>
#include <string.h>
#include <dirent.h>
/* inet includes */
#include <netdb.h>
#ifdef _DLPI
#include <sys/dlpi.h>
#endif
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>

#ifndef _KERNEL
#define _KERNEL
#include <sys/ksynch.h>
#undef _KERNEL
#else
#include <sys/ksynch.h>
#endif

#include <netinet/ppp_lock.h>
#include <netinet/ppp.h>
#include <netinet/asyhdlc.h>
#undef TRUE
#undef FALSE
#include <dial.h>
#undef DIAL
#include <sys/time.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/un.h>

#include <sys/filio.h>

#include <net/bpf.h>
#include "md5.h"


#include "pathnames.h"

#include "pppd.h"
/* Address Pooling API */
#include "pool.h"
#include "pool_proto.h"
#include "pppu_proto.h"


struct	bpf_program *fparse();

/* pppd waits at most 30 seconds before exiting */
#define PPPD_TIMEOUT    30     


#define	PPP_PREF	gettxt(":105", "ppp")

/* assume chap name size is 1k, if 
 * this is not big enough, bump this value up 
 */
#define	CHAP_NAMELEN	1024	

/* ppp filter file keywords */ 
#define	BRINGUP		"bringup"
#define	PASS		"pass"
#define	KEEPUP		"keepup"

/* struct holds PPP filter */
struct filter {
	char	tag[NAME_SIZE+1];		/* filter tag */
	char	bringup[PPPBUFSIZ];		/* bringup filter */	
	char	pass[PPPBUFSIZ];		/* pass filter */	
	char	keepup[PPPBUFSIZ];		/* keepup filter */
	unchar	flag;			
	struct filter	*next;			/* next filter on the list */
};
#define	F_MO		1			/* filter modified */		
#define	F_DF		2			/* filter defined */

struct filter	*flist = NULL;

struct pollfd fds[]={
	0,POLLIN | POLLPRI,0,
	0,POLLIN | POLLPRI,0,
};

char *program_name;
struct ppp_ppc_inf_ctl_s ctlbuf;
struct ppp_ppc_inf_dt_s databuf;

struct strbuf ctlsb = {
	sizeof(ctlbuf),0,(char*) &ctlbuf,
};

struct strbuf datasb = {
	sizeof(databuf),0,(char*) &databuf,
};

/* Array to hold ppp link information */
struct conn_made_s  *conn_made = NULL;

/* Array to hold outgoing or static ppp interface configuration information */
struct conn_conf_s *conn_conf = NULL;

int gppid;			/* the parent process id	*/
int ppppid;			/* ppp daemon child process pid */
int ppcidfd;			/* file descriptor for /dev/ppcid */
int pppfd;			/* file descriptor for /dev/ppp */
int ipfd;			/* file descriptor for /dev/ip */
int stat_sock;			/* socket for ppp msg. */
int so;				/* unix domain socket */
fd_set infds, outfds, exfds;	/* file descriptor for select */

char    IFNAME_PREF[80];
int num_ppp_interfaces = 0;


char *pppdev = _PATH_PPP;
char *ipdev = _PATH_IP;
static	struct ip_made_s ipmade;
char ifname[IFNAMSIZ];

extern int errno;

int Debug;
int Exitop;

int debug_fd = -1;
int reconf   = 0;
char debug_buf[512];
char debug_fmt_buf[512];
#define MON_ERR_MSG  gettxt(":109", "pppd already being monitored\n")

#define USAGE() pfmt(stderr, MM_ERROR, ":110:usage: %s [-d level] [-w max_wait]\n", program_name)

#define bcmp(s1, s2, len)	memcmp(s1, s2, len)
#define bcopy(s1, s2, len)	memcpy(s2, s1, len)
#define bzero(str,n)            memset((str), (char) 0, (n))

/*
 * Macro used to free addresses 
 *
 * Free the addresses.
 * Note: If the addresses were not
 *       allocated using pool_addr_alloc(),
 *       then the free routine will silently
 *       ignore them.
 */

#define FREE_ADDR(pool, a) { \
			pool_addr_free((pool), POOL_IP, (void *) &(a), \
					sizeof(struct sockaddr_in)); \
		     }

/* signal processor for parent to exit */
void
ppp_exit(sig)
int sig;
{
	exit(0);
}

/* signal processor for parent when it gets SIGHUP, SIGUSR1, SIGUSR2 */
void
ntfy_child(sig)
int sig;
{
	ppp_syslog(LOG_INFO, "sig_process: ntfy_child %d", sig);
	kill((pid_t)ppppid, sig);
}

/* signal processor for parent when child die */
void
remove_self(sig)
int sig;
{
	ppp_syslog(LOG_INFO, "sig_process: remove_self");
	exit(0);
}

/* signal processor for parent when it is killed */
void
remove_child(sig)
int sig;
{
	ppp_syslog(LOG_INFO, "sig_process: remove_child");
	kill((pid_t)ppppid, SIGINT);
	exit(0);
}

/* signal processor for child when it is killed */
void
sig_rm_conn(sig)
int sig;
{
	struct conn_made_s *cm;
	struct conn_conf_s *cf;

	ppp_syslog(LOG_INFO, "sig_process: sig_rm_conn");

	if (!ppppid)
		ppppid = 1;
	else
		return;

	for (cm = conn_made; cm; cm = cm->next) {
		 if (cm->muxid) {
			ppp_rm_conn(cm);
		}
	}

	for (cf = conn_conf; cf; cf = cf->next) {
		if (cf->ip.ipfd) {
			ppp_rm_inf(&cf->ip);
		}
	}
	exit(0);
}

/* signal processor for child when it gets SIGUSR1 */
void
sig_usr1(sig)
int sig;
{
	Debug++;
}

/* signal processor for child when it gets SIGUSR2 */
void
sig_usr2(sig)
int sig;
{
	Debug = 0;
}

/* signal processor for child when it gets SIGPOLL */
void
sig_poll(sig)
int sig;
{
	ppcid_msg(fds[0].fd);
}


jmp_buf	openabort;
/* signal processor when alarmed */
void
sig_alarm(sig)
int sig;
{
	ppp_syslog(LOG_INFO, gettxt(":111", "sig_alarm: get alarm signal"));
	longjmp(openabort, 1);
}

/*
 * configure PPP interface
 */
void
config()
{
	int	i = 0, j, fd, ret, perm, filter_modified, killgp=0;
	struct	ppphostent *hp, host;
	struct conn_made_s *cm;
	struct conn_conf_s *cf;
	struct ip_made_s *im;
	char	dev_name[TTY_SIZE + 1], *ifptr, tmp[16];
	struct	ifreq	ifr;
	FILE	*fp;
	struct	filter *f_ptr;
	
	sighold(SIGHUP);

	/* 
	 * initialize the address pool 
         *
	 *
	 * If reconf == 0, then the this is our first time being
	 * configured.
	 */

	if (pool_init(reconf) == -1) 
	  ppp_syslog(LOG_ALERT, gettxt(":112", "Address pooling subsystem failed to initialize"));

	if ( (Exitop==0) && reconf == 0)
		killgp = 1;
	reconf = 1;

	/* parse pppfilter file */
	getfilter();

	/* delete configured outgoing/dedicated PPP interface */
	for (cf = conn_conf; cf ; cf = cf->next) {
		if(!cf->ip.ipfd)
			continue;

		hp = getppphostbyaddr((char *)&(cf->remote.sin_addr),
			sizeof(struct in_addr), cf->remote.sin_family);

		if (hp) {
			/* check if its filter specification has changed */
			filter_modified = 0;
			for (f_ptr = flist; f_ptr; f_ptr = f_ptr->next) {
				if (!strcmp(f_ptr->tag, hp->tag)&&(f_ptr->flag & F_MO))
					filter_modified = 1;
			}
		}
 
		if (!hp || 
		   cf->remote.sin_addr.s_addr != 
			hp->ppp_cnf.remote.sin_addr.s_addr ||
		   cf->local.sin_addr.s_addr != 
			hp->ppp_cnf.local.sin_addr.s_addr ||
		   cf->mask.sin_addr.s_addr != 
			hp->ppp_cnf.mask.sin_addr.s_addr ||
		   strcmp(cf->device, hp->device) ||
		   strcmp(cf->speed, hp->speed) ||
		   (hp->device[0] != '\0' && cf->flow != hp->flow) ||
		   strcmp(cf->tag, hp->tag) ||
		   filter_modified) {
			ppp_syslog(LOG_INFO, gettxt(":113", "config: delete interface: '%s'"),
				inet_ntoa(cf->remote.sin_addr));
			if(cf->made) {
				ppp_close_conn(cf->made);
				if (cf->made->type == STATIC_LINK)
					ppp_rm_conn(cf->made);
			}
			ppp_rm_inf(&cf->ip);
			CONN_CONF_REMOVE(cf);
		}
	}	

	/* add configured outgoing and dedicated PPP interfaces */
	fp = setppphostent();
	while ((ret = getppphostent(fp, &host))) {
		if (ret == -1)
			continue;

		/* make sure it is a transparent outgoing PPP interface */
		if (host.loginname[0] != '\0' || host.attach[0] != '\0')
			continue;

		/* is this a permanent link? */
		perm = (host.device[0] == '\0') ? 0 : 1 ;
		
		/* create PPP interface */
		if (!(im = ppp_add_inf(host.ppp_cnf.remote, host.ppp_cnf.local,
			host.ppp_cnf.mask, &ifptr, host.tag, perm, host.ppp_cnf.debug))){
			ppp_syslog(LOG_WARNING, gettxt(":114", "ppp_add_inf fail"));
			continue;
		}

		/* record the interface */

		if(cf = CONN_CONF_ADD()) {
		        bcopy((char *)im, (char *)&cf->ip, 
			sizeof(struct ip_made_s));
			bcopy(ifptr, cf->ifname, sizeof(cf->ifname));
			bcopy(host.device, cf->device, sizeof(cf->device));
			bcopy(host.speed, cf->speed, sizeof(cf->speed));
			bcopy(host.tag, cf->tag, sizeof(cf->tag));
			cf->flow = host.flow;
			bcopy((char *)&host.ppp_cnf.remote, (char *)
			      &cf->remote, sizeof(cf->remote));
			bcopy((char *)&host.ppp_cnf.local, (char *)
			      &cf->local, sizeof(cf->local));
			bcopy((char *)&host.ppp_cnf.mask, (char *)
			      &cf->mask, sizeof(cf->mask));
		}		
		else {
			ppp_syslog(LOG_WARNING, gettxt(":115", "Can't allocate conn_conf_s structure"));
			ppp_rm_inf(im);
			break;
		}
			
		strcpy(ifr.ifr_name, ifptr);
		if (sioctl(im->ipfd, SIOCGIFFLAGS, (caddr_t)&ifr, 
			sizeof(struct ifreq)) < 0 ) { 
			ppp_syslog(LOG_WARNING, gettxt(":116", "%s SIOCGIFFLAGS fail: %m"),ifr.ifr_name);
			ppp_rm_inf(im);
			continue;
		}
		/*
		 * The 4.x stack will not send packets tot the
		 * PPP driver unless it is up.  So we
		 * have to keep it marked up all the time.
		 */
		ifr.ifr_flags |= IFF_UP;
		if (sioctl(im->ipfd, SIOCSIFFLAGS, (caddr_t)&ifr,
			sizeof(struct ifreq)) < 0 ) { 
			ppp_syslog(LOG_WARNING, gettxt(":117", "SIOCSIFFLAGS fail: %m"));
			ppp_rm_inf(im);
			continue;
		}

		if (!perm)
			continue;
 
		/* set up static PPP link */
		ppp_syslog(LOG_INFO, gettxt(":118", "Set static PPP on %s"), host.device);
		if (strstr(host.device, _PATH_DEV) == (char *)NULL) {
			if ((strlen(host.device) + strlen(_PATH_DEV)) >= 
			   sizeof(dev_name)) {
				ppp_syslog(LOG_WARNING, gettxt(":119", "device name too long\n"));
				ppp_rm_inf(im);
				continue;
			}
			sprintf(dev_name, "%s%s", _PATH_DEV, host.device);
			bcopy(dev_name, host.device, strlen(dev_name) + 1);
		}

		/* If we can't open dev in 5 second, fail it */
		if (setjmp(openabort)) {
			ppp_syslog(LOG_WARNING, gettxt(":120", "open %s: timeout"), host.device);
			ppp_rm_inf(im);
			continue;
		}
		(void) signal(SIGALRM, sig_alarm);
		(void) alarm(5);
		if (host.clocal)
			fd = open(host.device, O_RDWR|O_NDELAY);
		else
			fd = open(host.device, O_RDWR);

		alarm(0);
		if (fd < 0) {
			ppp_syslog(LOG_WARNING, gettxt(":121", "can't open %s: %m"), host.device);
			ppp_rm_inf(im);
			continue;
		}

		/* record the PPP link */
		if (cm = ppp_add_conn(fd, &host)) {
			cm->type = STATIC_LINK;
			cm->conf = cf;
			cf->made = cm;
			bcopy(ifptr, cm->ifname, strlen(ifptr));
			memcpy(&databuf.di_cnf, &host.ppp_cnf, 
			 		sizeof(struct ppp_configure));

			strcpy(tmp, inet_ntoa(host.ppp_cnf.local.sin_addr));
			ppp_syslog(LOG_INFO, gettxt(":123", "Assigned link id for dedicated link (%s->%s) is %d"), tmp, inet_ntoa(host.ppp_cnf.remote.sin_addr), cm->muxid);

			ctlbuf.function = PPCID_PERM;
			ctlbuf.l_index = cm->muxid;
			ctlsb.len = sizeof(struct ppp_ppc_inf_ctl_s);
			datasb.len = sizeof(struct ppp_ppc_inf_dt_s);
			if (putmsg(ppcidfd,&ctlsb,&datasb,RS_HIPRI) < 0) {
				ppp_syslog(LOG_WARNING, gettxt(":124", "ppcid put msg fail: %m"));
				ppp_rm_conn(cm);
				ppp_rm_inf(im);
				continue;
			}
		} else {	 
			ppp_syslog(LOG_WARNING, gettxt(":125", "ppp_add_conn fail")); 
			ppp_rm_inf(im);
		}
	}

	endppphostent(fp);

	sigrelse(SIGHUP);

	if (killgp){
		killgp = 0;
		kill(gppid, SIGUSR1);
	}
}

/* 
 * Parse ppp filter file
 *	return: -1: fail to parse the filter file 
 *		0 : succeed 
 */			
int
getfilter()
{
	char	*cmdbuf, nullstring[1], line[3*PPPBUFSIZ], *bringup, *pass, *keepup;
	struct filter 	*f_ptr, *p_ptr, *ud_ptr;
	FILE	*fp;
	int	i;

	nullstring[0] = '\0';

	fp = fopen(_PATH_PPPFILTER,"r");
	if (!fp) {
		ppp_syslog(LOG_WARNING, gettxt(":126", "can't fopen %s: %m "),_PATH_PPPFILTER);
		return(-1);
	}

	while((cmdbuf = pppfgets(line, 3*PPPBUFSIZ, fp))) {

		if (*cmdbuf == '\0')
			continue;

		/* search for tag */ 
		cmdbuf = strtok(cmdbuf," \t");
		
		/* Is this tag defined? */
		for (f_ptr = flist; f_ptr; f_ptr = f_ptr->next) {
			if (!strcmp(f_ptr->tag, cmdbuf))
				break;
		}

		if (!f_ptr) {
			/* allocate memory for this tag */
			if ((f_ptr = (struct filter *) malloc(sizeof(struct filter))) == NULL){
				ppp_syslog(LOG_WARNING, gettxt(":127", "getfilter:malloc fail %m"));
				return(-1);
			}
			strcpy(f_ptr->tag, cmdbuf);
			*f_ptr->bringup = '\0'; 
			*f_ptr->pass = '\0'; 
			*f_ptr->keepup = '\0'; 
			f_ptr->next = flist;
			flist = f_ptr;
			f_ptr->flag = 0;
		}
	
		/* skip the tag part */
		for(; *cmdbuf != '\0'; cmdbuf++);
		cmdbuf++;

		/* find filter specifications */
		bringup = strstr(cmdbuf, BRINGUP);
		pass = strstr(cmdbuf, PASS);
		keepup = strstr(cmdbuf, KEEPUP);
	
		if (bringup) {
			*bringup = '\0';
			bringup += strlen(BRINGUP);
		} else 
			bringup = nullstring;

		if (pass) {
			*pass = '\0';		
			pass += strlen(PASS);
		} else
			pass = nullstring;

		if (keepup) {
			*keepup = '\0';
			keepup += strlen(KEEPUP);
		} else
			keepup = nullstring;

		/* skip to the first character of the specification */
		for ( ; *bringup == ' ' || *bringup == '\t'; bringup++);
		for ( ; *pass == ' ' || *pass == '\t'; pass++);
		for ( ; *keepup == ' ' || *keepup == '\t'; keepup++);
	
		/* check if filter specification has been modified */
		if (strncmp(bringup, f_ptr->bringup, PPPBUFSIZ) ||  	
		   strncmp(pass, f_ptr->pass, PPPBUFSIZ) || 
		   strncmp(keepup, f_ptr->keepup, PPPBUFSIZ)) {
			f_ptr->flag |= F_MO;
		} else
			f_ptr->flag &= ~F_MO;
		
		/* copy in new specification */  
		if (*bringup == '\0'){
			ppp_syslog(LOG_WARNING, 
				gettxt(":128", "Tag:%s: all packets are significant for %s"), 
				f_ptr->tag, BRINGUP);
			*f_ptr->bringup = '\0';
		} else {
			strncpy(f_ptr->bringup, bringup, PPPBUFSIZ);
			f_ptr->bringup[PPPBUFSIZ -1] = '\0';
		}	

		if (*pass == '\0'){
			ppp_syslog(LOG_WARNING, 
				gettxt(":128", "Tag:%s: all packets are significant for %s"), 
				f_ptr->tag, PASS);
			*f_ptr->pass = '\0';
		} else {
			strncpy(f_ptr->pass, pass, PPPBUFSIZ);
			f_ptr->pass[PPPBUFSIZ -1] = '\0';
		}
	
		if (*keepup == '\0'){
			ppp_syslog(LOG_WARNING,
				gettxt(":128", "Tag:%s: all packets are significant for %s"), 
				f_ptr->tag, KEEPUP);
			*f_ptr->keepup = '\0';
		} else {
			strncpy(f_ptr->keepup, keepup, PPPBUFSIZ);
			f_ptr->keepup[PPPBUFSIZ -1] = '\0';
		}
		
		/* mark the filter as defined */
		f_ptr->flag |= F_DF;	
	}
	
	fclose(fp);

	/* delete those undefined filter from the filter list */
	p_ptr = NULL;
	for (f_ptr = flist; f_ptr;) {
		if (! (f_ptr->flag & F_DF)) {
			if (!p_ptr)
				flist = f_ptr->next;
			else
				p_ptr->next = f_ptr->next;
			ud_ptr = f_ptr;	
			f_ptr = f_ptr->next;
			free(ud_ptr);
		} else {
			f_ptr->flag &= ~F_DF;
			p_ptr = f_ptr; 	
			f_ptr = f_ptr->next;
		}
	}

}

jmp_buf	filterabort;

/*
 * Send the filter to kernel
 */			
void
dofilter(fd, spec, netmask, key)
	int	fd;
	char	*spec;
	ulong	netmask;
	char	*key;
{
	char	*cmdbuf;
	struct bpf_program *fcode;
	struct strioctl si;
	int	i, n;
	struct bpf_insn *insn;

	if (setjmp(filterabort)) {
		ppp_syslog(LOG_WARNING, gettxt(":129", "filter abort"));
		ppp_syslog(LOG_WARNING, gettxt(":130", "all packets are significant for %s"), key);
		return;
	}

	if (!*spec) {
		ppp_syslog(LOG_WARNING, gettxt(":130", "all packets are significant for %s"), key);
		return;
	}
		
	cmdbuf = spec;

	fcode = fparse(cmdbuf, 1, DLT_EN10MB, netmask);
	
	if(!fcode) 
		return;

	insn = fcode->bf_insns;
	n = fcode->bf_len;

	if (Debug) {
		ppp_syslog(LOG_INFO, gettxt(":131", "Internal filtering code for %s:"), key);
		for (i=0; i< n; ++insn, ++i)
			ppp_syslog(LOG_INFO, "{ 0x%x, %d, %d, 0x%08x }",
				insn->code, insn->jt, insn->jf, insn->k);
	}

	if (!strcmp(key, BRINGUP)) {
		if (sioctl(fd, P_SETBF, (char *)fcode->bf_insns,
			fcode->bf_len * sizeof(struct bpf_insn)) < 0) {
			ppp_syslog(LOG_WARNING, gettxt(":132", "sioctl P_SETBF fail: %m"));
			return;
		}
	}

	if (!strcmp(key, KEEPUP)) {
		if (sioctl(fd, P_SETKF, (char *)fcode->bf_insns,
			fcode->bf_len * sizeof(struct bpf_insn)) < 0) {
			ppp_syslog(LOG_WARNING, gettxt(":133", "sioctl P_SETKF fail: %m"));
			return;
		}
	}

	if (!strcmp(key, PASS)) {
		if (ioctl(fd, I_PUSH, "ipf") < 0) { 
			ppp_syslog(LOG_WARNING,
			 gettxt(":135", "ioctl I_PUSH failed when trying to push module ipf: %m"));
			return;
		}
		if (sioctl(fd, BIOCSETF, (char *)fcode->bf_insns,
			fcode->bf_len * sizeof(struct bpf_insn)) < 0) {
			ppp_syslog(LOG_WARNING, gettxt(":136", "sioctl BIOCSETF fail: %m"));
			return;
		}
	}

}

#ifdef __STDC__
filtererr(char *fmt, ...)
#else
filtererr(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
	va_list args;
	char temp[512];

#ifdef __STDC__
	va_start(args,fmt);
#else
	va_start(args);
#endif
	vsprintf(temp, fmt, args);
	ppp_syslog(LOG_WARNING,gettxt(":137", "Error enabling Packet filtering: %s"), temp);

        /*
         * Free memory that may have been allocated
         * by the filter code in gencode.c.  There are places in
         * the code where if we longjump here, we could longjump
         * without de-allocating memory. It does not hurt to call
         * freechunks() more than once, because it will simply do
         * nothing if all of the chunks are alread freed.
         *
         */
        freechunks();                                                        
	longjmp(filterabort, 1);
}

main (argc,argv)
int argc;
char *argv[];
{
	int i;
	struct ifreq	ifr;
	int	confd;
	struct conn_made_s *cm;
	struct conn_conf_s *cf;
	extern char	*optarg;
	int	maxwait = 5;	/* default number of backlogged conn's	*/
	short	w;
	int	r;
	FILE	*fp;
	struct	sockaddr_un sin;
	int	sinlen, msg_sock, timeout=PPPD_TIMEOUT;

        (void)setlocale(LC_ALL,"");     
        (void)setcat("uxppp");
        (void)setlabel("UX:pppd");     

	program_name = strrchr(argv[0],'/');
	program_name = program_name ? program_name + 1 : argv[0];

	Debug= 0;
	Exitop = 0;
	gppid = getpid();
	while ((i = getopt(argc, argv, "d:et:w:")) != EOF) {
		switch (i) {
		case 'd':
			Debug = atoi(optarg);
			break;
		case 'e':
			Exitop = 1;
			break;
		case 't':
			if (atoi(optarg)>=0)
			     timeout = atoi(optarg);
			break;
		case 'w':
			maxwait = atoi(optarg);
			break;
		default:
			USAGE();
			exit(1);
		}
	}


#ifdef LOG_DAEMON
	openlog(program_name, LOG_PID|LOG_NDELAY, LOG_DAEMON); 
#else
	openlog(program_name, LOG_PID);
#endif


	/* fork a child so the parent can exit creating a daemon */
	if ((ppppid = fork()) < 0) {
		ppp_syslog(LOG_ALERT, gettxt(":138", "fork() fail: %m"));
		exit(-1);
	}
	if (ppppid){
		if (Exitop == 1){
			exit(0);
		}
		else{
			signal(SIGUSR1, ppp_exit);
			signal(SIGALRM,ppp_exit);
			alarm(timeout);
			waitpid(-1, (int *)0, 0);
			exit(0);
		}
	}

	close(0); 
	if ((confd = open(_PATH_CONSOLE,O_WRONLY)) < 0) {
		ppp_syslog(LOG_ALERT, gettxt(":139", "open %s fail: %m"), _PATH_CONSOLE);
		exit(-1);
	}
	close(1);
	close(2);
	dup(confd);
	dup(confd); 

	setpgrp();
	sigset(SIGCLD, remove_self);
	sigset(SIGHUP, SIG_IGN);
	sigset(SIGPIPE, SIG_IGN);
	sigset(SIGINT, remove_child);
	sigset(SIGQUIT, remove_child);
	sigset(SIGTERM, remove_child);
	sigset(SIGUSR1, ntfy_child);
	sigset(SIGUSR2, ntfy_child);

	/* fork a child to handle the work. A child  must be forked so that the
	 * tty's which the daemon is handling do not get attached as controlling
	 * terminals
	 * DISCLAIMER: letting a parent process hang around with nothing to do
	 * seems ugly but can't figure out a better way to prevent the
	 * controlling terminal crap
	 */
	if ((ppppid = fork()) < 0) {
		ppp_syslog(LOG_ALERT, gettxt(":138", "fork() fail: %m"));
		exit(-1);
	}

	if (ppppid) {
		/* parent daemon logs all debugging messages */
		/* open a ppp driver to get logging messages */
		if ((pppfd = open(_PATH_PPP, O_RDWR|O_NDELAY)) < 0) {
			ppp_syslog(LOG_ALERT, gettxt(":139", "open %s fail: %m"), _PATH_PPP);
			kill((pid_t)ppppid, SIGINT);
			exit(-1);
		} else {
			/* set this as a logging driver */
			if (sioctl(pppfd, SIOCSLOG, (caddr_t)NULL, 0) < 0) {
				ppp_syslog(LOG_ALERT, gettxt(":140", "sioctl(SIOCLOG) fail: %m"));
				kill((pid_t)ppppid, SIGINT);
				(void)close(pppfd);
				exit(-1);
			}
		}

		/* before listen on the logging driver, send a SIHUP
		 * to child process to configure the outgoing interface 
		 */
		sleep(2);
		kill((pid_t)ppppid, SIGHUP);

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
				ppp_syslog(LOG_ERR,gettxt(":141", "select: %m"));
                        	continue;
                	}
	                if (FD_ISSET(pppfd, &infds) || FD_ISSET(pppfd, &exfds)) 
       		                 ppplog(pppfd);
                }

	}

	ppp_syslog(LOG_INFO, gettxt(":142", "restarted"));
	sigset(SIGHUP, (void(*)())config);
	sigset(SIGINT, sig_rm_conn);
	sigset(SIGQUIT, sig_rm_conn);
	sigset(SIGTERM, sig_rm_conn);
	sigset(SIGPOLL, sig_poll);
	sigset(SIGCLD, SIG_IGN);
	sigset(SIGUSR1, sig_usr1);
	sigset(SIGUSR2, sig_usr2);


syslog(LOG_INFO, "(1)conn_made = 0x%x", conn_made);
	/* initialize link structure array */	
	conn_made = NULL;
syslog(LOG_INFO, "(2)conn_made = 0x%x", conn_made);

	/* initialize interface structure array */	
	conn_conf = NULL;

	/* open ppcid driver */
	if ((fds[0].fd = ppcidfd = open(_PATH_PPCID, O_RDWR|O_NDELAY)) < 0) {
		ppp_syslog(LOG_ALERT, gettxt(":139", "open %s fail: %m"), _PATH_PPCID);
		exit(-1);
	}

	/* generate SIGPOLL when message comes up */ 
	ioctl(ppcidfd, I_SETSIG, S_INPUT|S_HIPRI);

	/* open ip driver */
	if ((ipfd = open(ipdev, O_RDWR|O_NDELAY)) < 0) {
		ppp_syslog(LOG_ALERT, gettxt(":139", "open %s fail: %m"), ipdev);
		exit(-1);
	}

	/* open a unix domain socket */
	so = pppd_sockinit(maxwait);

	/* tuck my process id away */
	fp = fopen(PIDFILE_PATH, "w");
        if (fp != NULL) {
                fprintf(fp, gettxt(":143", "%d\n"), getpid());
                (void) fclose(fp);
        } else
		ppp_syslog(LOG_ALERT, gettxt(":144", "fopen %s fail: %m"), PIDFILE_PATH);
	
	while(1) {

		FD_ZERO(&outfds);
       		FD_ZERO(&infds);
        	FD_ZERO(&exfds);

		FD_SET(so, &infds);

		r = select(20, &infds, &outfds, &exfds, (struct timeval *)0);
		if (r < 0) {
			if (errno == EINTR) {
				continue;
			}
			ppp_syslog(LOG_ERR, gettxt(":145", "select fail: %m"));
			continue;
		}
		if (FD_ISSET(so, &infds)) {
			ppp_syslog(LOG_INFO, gettxt(":146", "accept on socket %d"), so);
			sinlen = sizeof(sin);
			if ((msg_sock = accept(so, (struct sockaddr *)&sin,
					       (size_t *) &sinlen)) == -1) {
				if (errno == EINTR) {
					continue;
				}
				ppp_syslog(LOG_ERR,gettxt(":147", "accept: %m"));
				continue;
			}

			/* We close socket if the connection is from ppp shell
			 * if it is from pppstat, we will keep the socket
			 * until we write back the ppp status info.
			 */
			if (!pppd_sockread(msg_sock)) {
				close(msg_sock);
			}
		}
	}
}

/*
 * process ppp debugging messages from /dev/ppp
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
		ppp_syslog(LOG_WARNING, gettxt(":148", "ppplog() getmsg failed: %m"));
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
	
		ppp_syslog(LOG_WARNING, fmt, databuf.arg1, databuf.arg2, databuf.arg3, databuf.arg4);
		break;
	default:
		ppp_syslog(LOG_WARNING, gettxt(":150", "Get a unknown log message")); 
		break;
	}
}
 
/*
 * link the tty under the ppp stack.
 */
struct conn_made_s *
ppp_add_conn(fd, hp)
int fd;
struct	ppphostent	*hp;
{
	struct conn_made_s *cm;
	struct termios trm;
	struct strioctl iocb;
	int	unit, i, dfd, speed, baudrate, linedis;
	char mod_name[PPP_MOD_NM_LN];
	char *tty_name;
	pppd_mod_list_t *lp;

	ppp_syslog(LOG_INFO, gettxt(":151", "ppp_add_conn"));
	

	tty_name = ttyname(fd);

	/* ditch any modules that were auto pushed onto the tty */
	if ((lp = pop_modules(fd)) == NULL) {
	        ppp_syslog(LOG_INFO,gettxt(":152", "Failed to pop modules: fd = %d"), fd);
		return(NULL);
        }

	dfd = fd;

	if (tcgetattr(fd, &trm) < 0) {
		ppp_syslog(LOG_WARNING, gettxt(":155", "tcgetattr() failed: %m"));
		restore_popped_mods(fd, lp);
		close(dfd);
		return(NULL);
	}
	
	trm.c_iflag = IGNBRK;
	if (hp->device[0] != '\0') { 
		/* if this is a static link, set line speed */ 
		baudrate = getbaud(hp->speed);
		if (baudrate == 0) {
			ppp_syslog(LOG_WARNING, 
				gettxt(":156", "%s baud rate not supported"), hp->speed);
                	restore_popped_mods(fd, lp);
	
			close(dfd);
			return(NULL);
		}
	} else {
		baudrate = cfgetispeed(&trm);
		if (baudrate == 0) {
		  baudrate = cfgetospeed(&trm);
		  syslog(LOG_INFO, gettxt(":157", "Baud rate set to zero, hanging up modem"));
		}
	}

	if (hp->clocal) {
		trm.c_cflag = CS8|HUPCL|CREAD|CLOCAL;
		ppp_syslog(LOG_INFO, gettxt(":158", "Ignoring Carrier (clocal)"));
	} 
	else
		trm.c_cflag = CS8|HUPCL|CREAD;

	if (cfsetispeed(&trm, baudrate) < 0) {
		ppp_syslog(LOG_WARNING, gettxt(":159", "cfsetispeed fail: %m"));
                restore_popped_mods(fd, lp);

		close(dfd);
		return(NULL);
	}
	if (cfsetospeed(&trm, baudrate) < 0) {
		ppp_syslog(LOG_WARNING, gettxt(":160", "cfsetospeed fail: %m"));
                restore_popped_mods(fd, lp);
		close(dfd);
		return(NULL);
	}

	/*
	 * First disable hardware flow control just in case it was 
	 * previosl enabled
	 */

	if (control_hwd_flow(fd, 0) == -1)
	  ppp_syslog(LOG_ERR, gettxt(":161", "Failed to disable hardware flow control: %m"));

	/* set line flow control mode */	
	if (hp->flow == XONXOFF) {
		trm.c_iflag |= IXON|IXOFF;
		ppp_syslog(LOG_INFO, gettxt(":162", "flow control: XONXOFF"));
	} 
	else {
		if (hp->flow == RTSCTS) { 
                        if (control_hwd_flow(fd, 1) == -1)
			  ppp_syslog(LOG_ERR, gettxt(":163", "Failed to enable hardware flow control: %m"));
			ppp_syslog(LOG_INFO, gettxt(":164", "flow control: RTSCTS"));
		}
	}

	trm.c_oflag = trm.c_lflag = trm.c_cc[VINTR] =
		trm.c_cc[VQUIT] = trm.c_cc[VERASE] = trm.c_cc[VKILL] =
		trm.c_cc[VTIME] = trm.c_cc[VEOL2] = 0;
	trm.c_cc[VMIN] = 1;


#ifdef DEBUG
	print_termios(&trm); 
#endif
	/* set the new termios */
	if (tcsetattr(fd, TCSANOW, &trm) < 0) {
                restore_popped_mods(fd, lp);
		close(dfd);
		ppp_syslog(LOG_WARNING, gettxt(":165", "tcsetattr() failed: %m"));
		return(NULL);
	}
	/* push on the asyhdlc module */

	if (ioctl(dfd, I_PUSH, "asyh") < 0) {
                restore_popped_mods(fd, lp);
		close(dfd);
		ppp_syslog(LOG_WARNING, gettxt(":168", "ioctl(I_PUSH, \"asyh\") failed: %m"));
		return(NULL);
	}

	/* tell ppp driver the speed */	
	speed = getbaudrate(baudrate);
	if (sioctl(ppcidfd, P_SETSPEED, (caddr_t)&speed, sizeof(speed))<0) {
                restore_popped_mods(fd, lp);
		close(dfd);
		ppp_syslog(LOG_WARNING, gettxt(":169", "sioctl(P_SETSPEED) failed: %m"));
		return(NULL);
	}

	/* link tty under ppp */

	if ((i = ioctl(ppcidfd, I_LINK, dfd)) < 0 ){
                restore_popped_mods(fd, lp);
		close(dfd);	
		ppp_syslog(LOG_WARNING, gettxt(":170", "sioctl(I_LINK) failed: %m"));
		return(NULL);
	}



	/* record the ppp link */
	if (cm = CONN_MADE_ADD()) { 
	         char *ptr;
		 cm->fd = dfd;
		 cm->cfd = fd;
		 cm->muxid = i;
		 cm->pgrp = 0;
		 cm->speed = speed;
		 cm->flow =  hp->flow;
		 strcpy(cm->tty_name, tty_name ? tty_name : gettxt(":171", "UNKNOWN"));
		 
		 bcopy((char *)&hp->ppp_cnf.remote, (char *) & cm->dst,
		       sizeof(struct sockaddr_in)); 
		 bcopy((char *)&hp->ppp_cnf.local, (char *) & cm->src,
		       sizeof(struct sockaddr_in)); 
		 bcopy(hp->tag, cm->tag, strlen(hp->tag)); 
		 cm->proxy = hp->proxy;	
		 cm->debug = hp->ppp_cnf.debug;	
		 cm->mod_list = lp;
		 strcpy(cm->pool_tag_local, hp->pool_tag_local);
		 strcpy(cm->pool_tag_remote, hp->pool_tag_remote);

		 return cm;
		 
       }


	ppp_syslog(LOG_WARNING, gettxt(":172", "Can't allocate conn_made_s structure"));
        restore_popped_mods(fd, lp);

	return(NULL);
}

/*
 * unlink the tty from under the ppp stack.
 */
ppp_rm_conn(cm)
struct conn_made_s *cm;
{
	char	*calltype = gettxt(":173", "Dedicated");
	char	*tty;

	ppp_syslog(LOG_INFO, gettxt(":174", "ppp_rm_conn: I_UNLINK muxid = %d"), cm->muxid);


	/* unlink the tty from ppp */
	if (cm->muxid && ioctl(ppcidfd, I_UNLINK, cm->muxid) < 0 ) {
		ppp_syslog(LOG_WARNING,
			   gettxt(":175", "ioctl(I_UNLINK) fail: ppcidfd  = %d  cm->muxid = %d: %m"),
			   ppcidfd, cm->muxid);

		return;
	}
	cm->muxid = 0;

	switch (cm->type) {
		case INCOMING:
			/*
			 * Free allocated addresses. 
			 * 
			 * If pool_tag_local or pool_tag_remote[0] != '\0'
			 * then we are guaranteed that cm->src contains
			 * and address.  The link will never make it this
			 * far if the address pool allocation failes.
			 */ 
			
			if (cm->pool_tag_local[0] != '\0')
				FREE_ADDR(cm->pool_tag_local, cm->src);
			if (cm->pool_tag_remote[0] != '\0')
			        FREE_ADDR(cm->pool_tag_remote, cm->dst);

			if (cm->ip.ipfd) {
				ppp_rm_inf(&cm->ip);	
			      }
			calltype = gettxt(":176", "Incoming");
			break;
		case OUTATTACH:
			if (cm->ip.ipfd) {
				ppp_rm_inf(&cm->ip);	
			      }
			calltype = gettxt(":177", "Attach");
			break;
		case OUTGOING:
			calltype = gettxt(":178", "Outgoing");
			break;
		case STATIC_LINK:
			calltype = gettxt(":173", "Dedicated");
			break;
		default:
			break;
	}	
 	cm->type = 0;
	bzero((caddr_t)&cm->ip, sizeof(struct ip_made_s));
	
	/* 
	 * Restore Popped Modules
	 */
	
	if (restore_popped_mods(cm->fd, cm->mod_list) == -1) {
	  ppp_syslog(LOG_WARNING, 
		     gettxt(":179", "ppp_rm_conn: Unable to restore popped modules (fd = %d)"), 
		     cm->fd);
	}


	tty = ttyname(cm->cfd);

	close(cm->fd);
	cm->fd = 0;
	
	myundial(cm->cfd); 
	cm->cfd = 0;
	
	if(cm->conf)
		cm->conf->made = NULL;

	ppp_syslog(LOG_INFO, gettxt(":180", "%s call on '%s' disconnected"), calltype, tty);

	/* kill the PPP shell/dial process */ 
	if (cm->pgrp) {
		ppp_syslog(LOG_INFO, gettxt(":181", "ppp_rm_conn sending pid %d SIGUSR1"), cm->pgrp);
		kill(cm->pgrp, SIGUSR1);
	}
	CONN_MADE_REMOVE(cm);
	return;
}

/*
 * gracefully close a ppp connection.
 */
ppp_close_conn(cm)
struct conn_made_s *cm;
{
	ppp_syslog(LOG_INFO, gettxt(":182", "ppp_close_conn: P_CLOSE muxid = %d"), cm->muxid);
	
	/* inform PPP kernel to close the connection */	
	if(sioctl(ppcidfd, P_CLOSE, (caddr_t)&cm->muxid, sizeof(cm->muxid))<0) {
		ppp_syslog(LOG_WARNING, gettxt(":183", "P_CLOSE fail: %m"));
		ppp_rm_conn(cm);
	}

	/* sleep 3 seconds (how long should we sleep?) 
	 * to wait for connection close down
	 */
	sleep(3); 
}

/*
 * link ppp under ip.
 */
struct	ip_made_s *
ppp_add_inf(dst, src, mask, name, tag, perm, debug)
	struct sockaddr_in	dst;
	struct sockaddr_in	src;
	struct sockaddr_in	mask;
	char	**name;
	char	*tag;
	int	perm;
	int	debug;
{
	int	ppp, ipmux;
	struct	ifreq	ifr;
	struct conn_made_s *cm;
	struct conn_conf_s *cf;
	char	*ifptr, tmp[16];
	struct	filter *f_ptr;
	int fd;
	int fail = 0;

	if (Debug) {
		strcpy(tmp, inet_ntoa(src.sin_addr));
		ppp_syslog(LOG_INFO, "ppp_add_inf: %s --> %s", tmp, 
			inet_ntoa(dst.sin_addr)); 
	}

	/* Make sure the interface we want to creat doesn't exist */
	for (cf = conn_conf; cf; cf = cf->next) {
		if (cf->ip.ipfd && cf->remote.sin_addr.s_addr == 
			dst.sin_addr.s_addr) {
			ppp_syslog(LOG_WARNING, gettxt(":184", "the interface:%s already exists"),
				inet_ntoa(dst.sin_addr));
			return(NULL);
		}
	}

	for (cm = conn_made; cm ; cm = cm->next) {
		if(cm->ip.ipfd && cm->dst.sin_addr.s_addr == 
			dst.sin_addr.s_addr) {
			ppp_syslog(LOG_WARNING, gettxt(":184", "the interface:%s already exists"),
				inet_ntoa(dst.sin_addr));
			return(NULL);
		}
	}

	/* open ppp driver */
	if ((ppp = open(pppdev, O_RDWR)) < 0) {
		ppp_syslog(LOG_WARNING, gettxt(":139", "open %s fail: %m"),pppdev);
		return(NULL);
	}

	if (dlbind(ppp, 0x800) < 0) {
		ppp_syslog(LOG_WARNING, gettxt(":185", "dlbind fail"));
		close(ppp);
		return(NULL);
	}

	if (perm) {
		/* set this as a permanet driver */
		if (sioctl(ppp, SIOCSPERM, (caddr_t)NULL, 0) < 0) {
			ppp_syslog(LOG_WARNING, gettxt(":186", "sioctl(SIOCSPERM) fail: %m"));
			close(ppp);
			return(NULL);
		}
	}

	/* configure ppp filtering */
	/* find its filter specification */
	for (f_ptr = flist; f_ptr; f_ptr = f_ptr->next) {
		if (!strcmp(f_ptr->tag, tag))
			break;
	}

	if (!f_ptr) {
		ppp_syslog(LOG_WARNING, gettxt(":187", "filter tag:%s not defined"), tag);
		ppp_syslog(LOG_WARNING, gettxt(":188", "all packets are significant"));
	} else {
		dofilter(ppp, f_ptr->bringup, mask.sin_addr.s_addr, BRINGUP);
		dofilter(ppp, f_ptr->pass, mask.sin_addr.s_addr, PASS);
		dofilter(ppp, f_ptr->keepup, mask.sin_addr.s_addr, KEEPUP);
	}

	if ((ipmux = ioctl(ipfd, I_LINK, ppp)) < 0) {
		ppp_syslog(LOG_WARNING, gettxt(":189", "I_LINK ppp under ip fail: %m"));
		close(ppp);
		return(NULL);
	}

	/* name the interface */
	strcpy(IFNAME_PREF, PPP_PREF);

	ifptr = ppp_getifname();

	if (ifptr == NULL) {
		ppp_syslog(LOG_WARNING, gettxt(":190", "getifname fail"));
		ioctl(ipfd, I_UNLINK, ipmux); 
		close(ppp);
		return(NULL);
	}
	strcpy(ifr.ifr_name, ifptr);
	ifr.ifr_metric = ipmux;
	/* name the interface */
	if (sioctl(ipfd, SIOCSIFNAME, (caddr_t)&ifr, sizeof(ifr)) < 0) {
		ppp_syslog(LOG_WARNING, gettxt(":191", "SIOCSIFNAME fail: %m (name = %s)"), 
			   ifr.ifr_name);
		ioctl(ipfd, I_UNLINK, ipmux); 
		close(ppp);
		return(NULL);
	}

	/* inform ip the debugging level */
	if (debug) {
		ifr.ifr_metric = debug;
		if (sioctl(ipfd, SIOCSIFDEBUG, (caddr_t)&ifr,
			sizeof(struct ifreq)) < 0 ) { 
			ppp_syslog(LOG_WARNING, gettxt(":192", "SIOCSIFDEBUG fail: %m"));
			ioctl(ipfd, I_UNLINK, ipmux); 
			close(ppp);
			return(NULL);
		}
	}
	
	/* Set destination address */
	bcopy((char *)&dst, (char *)&ifr.ifr_dstaddr,sizeof(struct sockaddr_in));	 

	if (sioctl(ipfd, SIOCSIFDSTADDR, (caddr_t)&ifr, sizeof(ifr)) < 0) {
	  ppp_syslog(LOG_WARNING, gettxt(":196", "%s SIOCSIFDSTADDR fail: %m"), inet_ntoa(dst.sin_addr));
	  goto err_out;
	}

	/* Set source address */
	bcopy((char *)&src, (char *)&ifr.ifr_addr,sizeof(struct sockaddr_in));

	if (sioctl(ipfd, SIOCSIFADDR, (caddr_t)&ifr, sizeof(ifr)) < 0) {
	  ppp_syslog(LOG_WARNING, gettxt(":195", "%s SIOCSIFADDR fail: %m"), inet_ntoa(src.sin_addr));
	  goto err_out;
	}

        /* set netmask */
	bcopy((char *)&mask, (char *)&ifr.ifr_addr,sizeof(struct sockaddr_in));
	if (sioctl(ipfd, SIOCSIFNETMASK, (caddr_t)&ifr, sizeof(ifr)) < 0) {
	  ppp_syslog(LOG_WARNING, gettxt(":197", "%s SIOCSIFNETMASK fail: %m"), inet_ntoa(mask.sin_addr));
	  goto err_out;
	}


out:	*name = ifptr;
	ipmade.ipfd = ipfd;
	ipmade.pppfd = ppp;
	ipmade.ipmuxid = ipmux;
	num_ppp_interfaces++;
	return(&ipmade);

err_out:
        ioctl(ipfd, I_UNLINK, ipmux); 
        close(ppp);
        return(NULL);
}

/*
 * unlink ppp under ip.
 */
int
ppp_rm_inf(im)
	struct ip_made_s *im;
{
	struct conn_conf_s *cf;

	ppp_syslog(LOG_INFO, gettxt(":198", "ppp_rm_inf: remove interface"));

	if (im->ipmuxid && ioctl(im->ipfd, I_UNLINK, im->ipmuxid) < 0) 
		ppp_syslog(LOG_WARNING, gettxt(":199", "I_UNLINK fail: %m"));
	close(im->pppfd);


	/* delete the entry from conn_conf[] */	
	for (cf = conn_conf; cf; cf = cf->next) {
		if (cf->ip.ipfd && cf->ip.ipmuxid == im->ipmuxid) {
			if(cf->made)
				cf->made->conf = NULL;
			CONN_CONF_REMOVE(cf);
			break;
		}
	}

	num_ppp_interfaces--;
	return(0);
} 

/*
 * process messages from the ppp login shell or pppstat.
 * return 0 for ppp login shell, return 1 for pppstat
 */
int
pppd_sockread(s)
	int	s;
{
	struct	conn_made_s	*cm;
	msg_t	msg;
	int	fd, totread, rval, flgs=0, dpid, nread;
	char	*p, ack, tmp[16];
	struct  ppphostent *hp, hostent;
	int tries = 0;
	int msg_sz = 0;
	FILE *fp;
	int r;

	totread = 0; 

	p = (char *)&msg;
	memset(p, 0, sizeof(msg));
	do {
		if ((rval = read(s, p, sizeof(msg) - totread)) <= 0) {
			ppp_syslog(LOG_ERR, gettxt(":200", "pppd_sockread: read: %m"));
			return 0;
		}
		p += rval;
		totread += rval;
	} while (totread < sizeof(msg));

	ppp_syslog(LOG_INFO, "pppd_sockread s=%d", s);

	switch (msg.m_type) {
	case MTTY:
		ppp_syslog(LOG_INFO,gettxt(":201", "Incoming call on '%s', pid=%d, name=%s"),
			msg.m_tty, msg.m_pid, msg.m_name);
		
		/* If we can't open dev in 5 second, fail it */
		if (setjmp(openabort)) {
			ppp_syslog(LOG_WARNING, gettxt(":120", "open %s: timeout"), msg.m_tty);
			kill(msg.m_pid, SIGUSR1);
			break;
		}
		(void) signal(SIGALRM, sig_alarm);
		(void) alarm(5);
		fd = open(msg.m_tty, O_RDWR);
		alarm(0);
		if (fd < 0) {
			ppp_syslog(LOG_WARNING, gettxt(":121", "can't open %s: %m"), msg.m_tty);
			kill(msg.m_pid, SIGUSR1);
			break;
		}

		hp = getppphostbyname(msg.m_name);
		if (!hp) {
			ppp_syslog(LOG_WARNING, gettxt(":202", "no parameter for '%s'"), msg.m_name);
			kill(msg.m_pid, SIGUSR1);
			close(fd);
			return 0;
		}
	
		if ((hp->pool_tag_remote[0] != '\0' ) && 
		    (getaddress(hp->pool_tag_remote, &hp->ppp_cnf.remote) == -1)) {
			ppp_syslog(LOG_ERR, 
				   gettxt(":203", "Invalid address pool specified for remote address: %s\n"),
				   hp->pool_tag_remote);
                        kill(msg.m_pid, SIGUSR1);
                        close(fd);
                        return 0;
		}

		if ((hp->pool_tag_local[0] != '\0' ) && 
		    (getaddress(hp->pool_tag_local, &hp->ppp_cnf.local) == -1)) {
			ppp_syslog(LOG_ERR, 
				   gettxt(":204", "Invalid address pool specified for local address: %s\n"),
				   hp->pool_tag_local);
                        kill(msg.m_pid, SIGUSR1);
                        close(fd);
                        return 0;
		}

		if (cm = ppp_add_conn(fd, hp)) {
			cm->pgrp = msg.m_pid;
			cm->type = INCOMING;
			memcpy(&databuf.di_cnf, &hp->ppp_cnf, 
					sizeof(struct ppp_configure));
			ppp_syslog(LOG_INFO, gettxt(":205", "Assigned link id for incoming link (login:%s pid:%d) is %d"), msg.m_name, msg.m_pid, cm->muxid);

			ctlbuf.function = PPCID_CNF_INCOMING;
			ctlbuf.l_index = cm->muxid;
			ctlsb.len = sizeof(struct ppp_ppc_inf_ctl_s);
			datasb.len = sizeof(struct ppp_ppc_inf_dt_s);
			if(putmsg(ppcidfd, &ctlsb, &datasb, RS_HIPRI) < 0) {
				ppp_syslog(LOG_WARNING, gettxt(":124", "ppcid put msg fail: %m"));
				close(fd);
				kill(msg.m_pid, SIGUSR1);
			}	
		} else {
			close(fd);
			kill(msg.m_pid, SIGUSR1);
		        if (cm->pool_tag_local[0] != '\0' )
			        FREE_ADDR(hp->pool_tag_local, hp->ppp_cnf.local);
		        if (cm->pool_tag_remote[0] != '\0' )
			        FREE_ADDR(hp->pool_tag_remote, hp->ppp_cnf.remote);
		}
		break;
	
	case MATTACH:
		ppp_syslog(LOG_INFO,gettxt(":206", "pppattach to '%s'"), msg.m_attach);

		hp = getppphostbyattach(msg.m_attach);
		if (!hp) {
			ppp_syslog(LOG_WARNING, gettxt(":207", "pppattach fail: no parameter for '%s'"), msg.m_attach);
			kill(msg.m_pid, SIGUSR1);
			return 0;
		}

                /*
                 * Make sure that we have not already attached to this
                 * host.
                 */

                for (cm = conn_made; cm; cm = cm->next) {
                        if (strcmp(cm->attach_name, msg.m_attach) == 0) {
                                /* Already attached to this guy */
                                msg.m_type = MDATTACH_BUSY;
                                r = write(s, &msg, sizeof(msg));
                                if (r < 0)
                                        ppp_syslog(LOG_WARNING,
                                                gettxt(":208", "Failed to contact pppattach command"));

                                return(0);
                        }
                }
		bcopy(hp, &hostent, sizeof(hostent)); 
		/* fork a child process to do the dialing */
		if ((dpid = fork()) < 0) {
			ppp_syslog(LOG_ALERT, gettxt(":138", "fork() fail: %m"));
			ppp_syslog(LOG_WARNING, gettxt(":209", "pppattach %s fail"), msg.m_attach);
			return 0;
		}
		if (dpid == 0) {
			pppdial(hostent, s);
		}
		return(1);
		break;

	case MDIAL:
		ppp_syslog(LOG_INFO, gettxt(":210", "Connected to remote system: tty=%s, pid=%d,addr=%s"),
		msg.m_tty, msg.m_pid, inet_ntoa(msg.m_remote.sin_addr));

		ctlbuf.function = PPCID_RSP;
		
		/* If we can't open dev in 5 second, fail it */
		if (setjmp(openabort)) {
			ppp_syslog(LOG_WARNING, gettxt(":120", "open %s: timeout"), msg.m_tty);
			kill(msg.m_pid, SIGUSR1);
			ctlbuf.l_index = -1;
			memcpy(&databuf.di_ia.ia_dstaddr, &msg.m_remote,
				sizeof(struct sockaddr_in));
			ctlsb.len = sizeof(struct ppp_ppc_inf_ctl_s);
			datasb.len = sizeof(struct ppp_ppc_inf_dt_s);
			if (putmsg(ppcidfd, &ctlsb, &datasb, flgs) < 0 ) {
				if(cm)	
					ppp_rm_conn(cm);
				ppp_syslog(LOG_WARNING, gettxt(":124", "ppcid put msg fail: %m"));
			}
			break;
		}
		(void) signal(SIGALRM, sig_alarm);
		(void) alarm(5);
		fd = open(msg.m_tty, O_RDWR);
		(void) alarm(0);
		if (fd < 0) {
			ppp_syslog(LOG_WARNING, gettxt(":121", "can't open %s: %m"), msg.m_tty);
			kill(msg.m_pid, SIGUSR1);
			ctlbuf.l_index = -1;
			goto ppcid_rsp;
		}

		hp = getppphostbyaddr((char *)&(msg.m_remote.sin_addr), 
			sizeof(struct in_addr), msg.m_remote.sin_family);
	
		if (!hp) {
			ppp_syslog(LOG_WARNING, gettxt(":202", "no parameter for '%s'"),
				inet_ntoa(msg.m_remote.sin_addr));
			kill(msg.m_pid, SIGUSR1);
			ctlbuf.l_index = -1;
			goto ppcid_rsp;
		}

		if (cm = ppp_add_conn(fd, hp)){ 
			ctlbuf.l_index = cm->muxid;
			cm->pgrp = msg.m_pid;
			cm->type = OUTGOING;
			strcpy(tmp, inet_ntoa(hp->ppp_cnf.local.sin_addr));
			ppp_syslog(LOG_INFO, gettxt(":211", "Assigned link id for outgoing link (%s->%s) is %d"), tmp, inet_ntoa(hp->ppp_cnf.remote.sin_addr), cm->muxid);

		} else {
			kill(msg.m_pid, SIGUSR1);
			ctlbuf.l_index = -1;
			goto ppcid_rsp;
		}
	
		/* Pass configurable parameters to kernel (Active open side) */
		memcpy(&databuf.di_cnf, &hp->ppp_cnf,
			sizeof(struct ppp_configure));
ppcid_rsp:	memcpy(&databuf.di_ia.ia_dstaddr, &msg.m_remote,
			sizeof(struct sockaddr_in));
		ctlsb.len = sizeof(struct ppp_ppc_inf_ctl_s);
		datasb.len = sizeof(struct ppp_ppc_inf_dt_s);
		if (putmsg(ppcidfd, &ctlsb, &datasb, flgs) < 0 ) {
			if(cm)	
				ppp_rm_conn(cm);
			ppp_syslog(LOG_WARNING, gettxt(":124", "ppcid put msg fail: %m"));
		}
		return 0;

	case MDATTACH:
		ppp_syslog(LOG_INFO, gettxt(":212", "Connected to remote system: tty=%s, pid=%d,attach=%s"),
		msg.m_tty, msg.m_pid, msg.m_attach);
		
		if ((fd = open(msg.m_tty, O_RDWR)) < 0) {
			ppp_syslog(LOG_WARNING, gettxt(":121", "can't open %s: %m"), msg.m_tty);
			kill(msg.m_pid, SIGUSR1);
			return 0;
		}
		hp = getppphostbyattach((char *)&(msg.m_attach)); 
	
		if (!hp) {
			ppp_syslog(LOG_WARNING, gettxt(":202", "no parameter for '%s'"),
				msg.m_attach);
			kill(msg.m_pid, SIGUSR1);
			return 0;
		}

		if (cm = ppp_add_conn(fd, hp)){ 
			ctlbuf.l_index = cm->muxid;
			cm->pgrp = msg.m_pid;
                        cm->attach_fd = msg.m_fd;
                        strcpy(cm->attach_name, msg.m_attach);
			cm->type = OUTATTACH;
			ppp_syslog(LOG_INFO, gettxt(":213", "Assigned link id for attach link (%s) is %d"), msg.m_attach, cm->muxid);
		} else {
			kill(msg.m_pid, SIGUSR1);
			return 0;
		}
	
		/* Pass configurable parameters to kernel */
		memcpy(&databuf.di_cnf, &hp->ppp_cnf, 
			sizeof(struct ppp_configure));
		ctlbuf.function = PPCID_CNF_ATTACH;
		ctlsb.len = sizeof(struct ppp_ppc_inf_ctl_s);
		datasb.len = sizeof(struct ppp_ppc_inf_dt_s);
		if (putmsg(ppcidfd, &ctlsb, &datasb, RS_HIPRI) < 0 ) {
			if(cm)	
				ppp_rm_conn(cm);
			ppp_syslog(LOG_WARNING, gettxt(":124", "ppcid put msg fail: %m"));
		}
		return 0;

       case MDATTACH_FAIL:
                /*
                 * Tell pppattach command that connection failed
                 */
                r = write(msg.m_fd, &msg, sizeof(msg));
                if (r < 0)
                        ppp_syslog(LOG_WARNING,
                                gettxt(":208", "Failed to contact pppattach command"));

		kill(msg.m_pid, SIGUSR1);
                close(msg.m_fd);
		return(0);


	case MPID:
		ppp_syslog(LOG_INFO, gettxt(":214", "PPP shell pid=%d died"), msg.m_pid);
		for (cm = conn_made; cm ; cm = cm->next) {
			if (cm->pgrp == msg.m_pid) {
				ppp_rm_conn(cm);
				return 0;
			}
		}
		return 0;

	case MSTAT:
		ppp_syslog(LOG_INFO, gettxt(":215", "Request from pppstat pid=%d"), msg.m_pid);
		ctlbuf.function = PPCID_STAT;
		ctlsb.len = sizeof(struct ppp_ppc_inf_ctl_s);
		datasb.len = sizeof(struct ppp_ppc_inf_dt_s);
		stat_sock = s;
		if (putmsg(ppcidfd, &ctlsb, &datasb, RS_HIPRI) < 0) {
			ppp_syslog(LOG_WARNING, gettxt(":124", "ppcid put msg fail: %m"));
			return 0;
		}
		return 1;
		
       case CSTAT:
		/* Send conn_made list to pppstat command */
		ppp_syslog(LOG_INFO, gettxt(":215", "Request from pppstat pid=%d"), msg.m_pid);
		
		for (cm = conn_made; cm; cm = cm->next) {
		  write(s, cm, sizeof(struct conn_made_s));
		}
		return(0);

       case POOLSTATS:
		if ((fp = fdopen(s, "r+")) == NULL) 
		  return(0);

		print_pool(fp);
		fflush(fp);
		return(0);

       case MONITOR:
		sigignore(SIGPIPE);
		if (debug_fd != -1) {
		  write(s, gettxt(":109", "pppd already being monitored\n"), sizeof(MON_ERR_MSG));
		  return(0);
		}

		debug_fd = s;

		return(1);

      case MONITOR_CLOSE:
		if (debug_fd != -1) {
		  close(debug_fd);
		  debug_fd = -1;
		}
		
		return(0);

	default:
		ppp_syslog(LOG_WARNING, gettxt(":216", "pppd_sockread: bad m_type 0x%x."),
					 msg.m_type);
		return 0;
	}
	return 0;

}

/*
 * process messages from the ppp protocol stack (/dev/ppcid)
 */
ppcid_msg(ppcidfd)
	int ppcidfd;
{
	int	i, flgs = 0, ip, dpid, fd, len, ret;
	struct ppphostent *hp, hostent;
	struct conn_made_s *cm = 0;
	struct conn_conf_s *cf;
	struct ip_made_s *im;
	struct ifreq ifr;
	char	pid[PID_SIZE], pwd[PWD_SIZE], *ifptr, errmsg[30];
	char	msg[1 + CHA_SIZE + SECRET_SIZE], secret[SECRET_SIZE], *rsp;
	struct	sockaddr_in	remote;
	
do_over:
	datasb.maxlen += CHAP_NAMELEN;
	if (getmsg(ppcidfd, &ctlsb, &datasb, &flgs) < 0) {
		if (errno = EAGAIN)
			return;
		ppp_syslog(LOG_WARNING, gettxt(":217", "ppcid_msg() getmsg failed: %m"));
		exit(errno);
	}

	if (ctlsb.len < sizeof(int)) {
		goto do_over;
	}
	
	switch (ctlbuf.function) {

	case PPCID_PAP:
		if (datasb.len < sizeof(struct inf_dt3)) {
			goto do_over;
		}
		ppp_syslog(LOG_INFO, "PPCID_PAP");

		errmsg[0] = '\0';
		(void)sprintf(pid, "%s", databuf.di_pid);
		/* get password */
		if (papgetpwd(pid, pwd) < 0)
			strcpy(errmsg, gettxt(":218", "no password"));
		else {	
			/* if password not match, fail pap auth */

			if (strcmp(pwd, databuf.di_pwd)) 
				strcpy(errmsg, gettxt(":219", "PWD not match"));
		}
		
		if (errmsg[0] == '\0') {
			databuf.di_pid[0] = '\0';
		} else {		
			strcpy(databuf.di_pid, errmsg);
			ppp_syslog(LOG_WARNING, gettxt(":220", "PAP fail: %s"), errmsg);
		}
	
		if (putmsg(ppcidfd, &ctlsb, &datasb,RS_HIPRI) < 0)
			ppp_syslog(LOG_WARNING, gettxt(":124", "ppcid put msg fail: %m"));
		break;

#if defined(DOCHAP)
	case PPCID_CHAP:
		if (datasb.len < sizeof(struct inf_dt5)) {
			goto do_over;
		}
		ppp_syslog(LOG_INFO, "PPCID_CHAP");

		errmsg[0] = '\0';
		secret[0] = '\0';
	
		if (databuf.di_secret[0] == '\0') {
			/* no SecuritySecret mibs */
			if (databuf.di_namelen > CHAP_NAMELEN) { 
				ppp_syslog(LOG_WARNING, gettxt(":221", "name field size %d too big"), databuf.di_namelen);
				strcpy(errmsg, gettxt(":222", "name too big"));
			}

			if (papgetpwd(databuf.di_name, secret) < 0)
				strcpy(errmsg, gettxt(":218", "no password"));
		} else
			/* Use SecuritySecret mibs */
			strcpy(secret, databuf.di_secret);
		
		(void)sprintf(msg, "%c%s", databuf.di_cid, secret);
		len = strlen(msg);
		bcopy(databuf.di_cha, msg + len, databuf.di_chalen);

		rsp = MD5String(msg, len + databuf.di_chalen);

		if (databuf.di_namelen == 0) { /* challenge peer */
			if (memcmp(rsp, databuf.di_rsp, RSP_SIZE)) 
				strcpy(errmsg, gettxt(":223", "wrong rsp"));
			if (errmsg[0] == '\0') {
				databuf.di_rsp[0] = '\0';
				ppp_syslog(LOG_INFO, gettxt(":224", "PPCID_CHAP auth success"));
			} else {
				strcpy(databuf.di_rsp, errmsg);
				ppp_syslog(LOG_INFO, gettxt(":225", "PPCID_CHAP auth fail: %s"), errmsg);
			}
		} else { /* challenged by the peer */
			/*
			 * Get the default local host id, and send it back in the
			 * name field of the chap request packet.  This is to make
			 * PPP implementations that use the name 
			 * field to indentify the host sending the response
			 * packet.
			 */
			pid[0] = '*';
			pid[1] = '\0';
			pwd[0] = '\0';
			
			if (papgetpwd(pid, pwd) == -1) {
		 	   ppp_syslog(LOG_WARNING, 
				gettxt(":367", 
			    "Unable to obtain default local ID to send in CHAP response packet"));
			}
			else {
			   memcpy(databuf.di_name, pid, strlen(pid));
			   databuf.di_namelen = strlen(pid);	
			}

			
			memcpy(databuf.di_rsp, rsp, RSP_SIZE);
		}

		if (putmsg(ppcidfd, &ctlsb, &datasb, RS_HIPRI) < 0)
			ppp_syslog(LOG_WARNING, gettxt(":124", "ppcid put msg fail: %m"));
		break;
#endif

	case PPCID_STAT:
		if (datasb.len < sizeof(struct inf_dt4)) {
			ppp_syslog(LOG_INFO, gettxt(":226", "PPCID_STAT do_over"));
			goto do_over;
		}
		ppp_syslog(LOG_INFO, "PPCID_STAT");
		
		if (write(stat_sock, (char *)&databuf.di_stat, 
			sizeof(struct ppp_stat)) < 0)
			ppp_syslog(LOG_WARNING, gettxt(":227", "socket write fail: %m"));
		close(stat_sock); 
		break;

	case PPCID_REQ:
		if (datasb.len < sizeof(struct in_ifaddr)) {
			goto do_over;
		}
		ppp_syslog(LOG_INFO, "PPCID_REQ");

		bcopy(&databuf.di_ia.ia_dstaddr, &remote, 
			sizeof(struct sockaddr_in));

		hp = getppphostbyaddr((char *)&(remote.sin_addr), 
			sizeof(struct in_addr), remote.sin_family);
	
		if (!hp) {
			ppp_syslog(LOG_WARNING, gettxt(":202", "no parameter for '%s'"),
				inet_ntoa(remote.sin_addr));
			ctlbuf.function = PPCID_RSP;
			ctlbuf.l_index = -1;
			if (putmsg(ppcidfd, &ctlsb, &datasb, flgs) < 0)
				ppp_syslog(LOG_WARNING, gettxt(":228", "ppcid putmsg fail: %m"));
			break;	
		}

		bcopy(hp, &hostent, sizeof(hostent)); 
		/* fork a child process to do the dialing */
		if ((dpid = fork()) < 0) {
			ppp_syslog(LOG_ALERT, gettxt(":138", "fork() fail: %m"));
			ctlbuf.function = PPCID_RSP;
			ctlbuf.l_index = -1;
			if (putmsg(ppcidfd, &ctlsb, &datasb, flgs) < 0)
				ppp_syslog(LOG_WARNING, gettxt(":228", "ppcid putmsg fail: %m"));
			break;	
		}
		if (dpid == 0) {
			pppdial(hostent, -1);
		}
		break;

	case PPCID_PREQ:
		if (datasb.len < sizeof(struct in_ifaddr)) {
			goto do_over;
		}
		ppp_syslog(LOG_INFO, "PPCID_PREQ");
		
		ctlbuf.function = PPCID_PCNF;
		hp = getppphostbyaddr((char *)&(((struct sockaddr_in *)
				&(databuf.di_ia.ia_dstaddr))->sin_addr),
			              sizeof(struct in_addr),
			              (int) databuf.di_ia.ia_dstaddr.sa_family);
		if (!hp) {
			ppp_syslog(LOG_WARNING, gettxt(":202", "no parameter for '%s'"),inet_ntoa(((struct sockaddr_in *)&(databuf.di_ia.ia_dstaddr))->sin_addr));
			ctlbuf.l_index = -1;
			goto ppcid_prsp;
		}

		/* set up proxy flag, debug value */
		for (cm = conn_made; cm; cm = cm->next) {
			if (cm->muxid != ctlbuf.l_index) 
				continue;
			cm->proxy = hp->proxy;
			cm->debug = hp->ppp_cnf.debug;
			break;
		}
			
		/* Pass configurable parameters to kernel (permanent link) */
		memcpy(&databuf.di_cnf, &hp->ppp_cnf, sizeof(struct ppp_configure));

ppcid_prsp:	ctlsb.len = sizeof(struct ppp_ppc_inf_ctl_s);
		datasb.len = sizeof(struct ppp_ppc_inf_dt_s);
		if (putmsg(ppcidfd, &ctlsb, &datasb, flgs) < 0 )
			ppp_syslog(LOG_WARNING, gettxt(":124", "ppcid put msg fail: %m"));
		break;

	case PPCID_CLOSE:
		if (ctlsb.len < sizeof(struct ppp_ppc_inf_ctl_s)) {
			ppp_syslog(LOG_INFO, gettxt(":229", "PPCID_CLOSE: do_over"));
			goto do_over;
		}
		ppp_syslog(LOG_INFO, gettxt(":230", "PPCID_CLOSE: muxid = %d"),ctlbuf.l_index);
		for (cm = conn_made; cm; cm = cm->next) {

			if (cm->muxid != ctlbuf.l_index) 
				continue;
			/* delete proxy arp entry */
			if (cm->proxy) {
				deletearp(cm->src.sin_addr.s_addr);
				deletearp(cm->dst.sin_addr.s_addr);
			}
			cf = cm->conf;
			/* unlink tty for incoming/outgoing call;
			 * mark down interface for outgoing and
			 * static interface.
			 */ 
			switch (cm->type) {
				case INCOMING:
				case OUTATTACH:
				case OUTGOING:
					ppp_rm_conn(cm);
					break;
				case STATIC_LINK: break;
				default: continue;
			};

			if (cf == NULL)
				break;

			/* mark the interface 'down' */
			(void)sprintf(ifr.ifr_name, "%s", cf->ifname);
			if (sioctl(cf->ip.ipfd, SIOCGIFFLAGS, (caddr_t)&ifr, 
				sizeof(struct ifreq)) < 0 ) { 
				ppp_syslog(LOG_WARNING, gettxt(":116", "%s SIOCGIFFLAGS fail: %m"), ifr.ifr_name);
				break;
			}

			/*
			 * The 4.x stack will not send packets tot the
			 * PPP driver unless it is up.  So we
			 * have to keep it marked up all the time.
			 */
			ifr.ifr_flags |= IFF_UP;

			if (sioctl(cf->ip.ipfd, SIOCSIFFLAGS, (caddr_t)&ifr,
				sizeof(struct ifreq)) < 0 ) { 
				ppp_syslog(LOG_WARNING, gettxt(":117", "SIOCSIFFLAGS fail: %m"));
				break;
			}

			/* reset the debugging level */
			ifr.ifr_metric = 0;
			if (sioctl(cf->ip.ipfd, SIOCSIFDEBUG, (caddr_t)&ifr,
				sizeof(struct ifreq)) < 0 ) 
				ppp_syslog(LOG_WARNING, gettxt(":192", "SIOCSIFDEBUG fail: %m"));
			break;	
		}
		break;

	case PPCID_UP:
		if (datasb.len < sizeof(struct inf_dt2)) {
			ppp_syslog(LOG_INFO, gettxt(":231", "PPCID_UP: do_over"));
			goto do_over;
		}
		ppp_syslog(LOG_INFO, gettxt(":232", "PPCID_UP: muxid = %d"), ctlbuf.l_index);

		for (cm = conn_made; cm; cm = cm->next) {
			if (cm->muxid == ctlbuf.l_index) 
				break;
		}

		if (cm == NULL) {
			ppp_syslog(LOG_WARNING, gettxt(":233", "PPCID_UP: not in conn_made[]"));
			break;
		}

		/* Configure the interface */	
		if (cm->type == INCOMING || cm->type == OUTATTACH) {
			/* create PPP interface for
			 * incoming call or pppattach link
			 */
			
			if(!(im = ppp_add_inf(databuf.di_remote, 
				databuf.di_local, databuf.di_mask, 
				&ifptr, cm->tag, 0, cm->debug))) {
				ppp_close_conn(cm);
				break;
			}
			bcopy((char *)im, (char *)&cm->ip,
				 sizeof(struct ip_made_s));
			bcopy((char *)&databuf.di_remote, (char *)&cm->dst,
				sizeof(struct sockaddr_in));
			bcopy((char *)&databuf.di_local, (char *)&cm->src,
				sizeof(struct sockaddr_in));
			bcopy(ifptr, cm->ifname, strlen(ifptr));

                        /*
                         * Inform pppattach that the interface is
                         * up.
                         */
                        if (cm->type == OUTATTACH) {
                                int rval;
                                msg_t msg;
                                memset(&msg, 0, sizeof(msg));
                                msg.m_pid = getpid();
                                msg.m_type = MDATTACH;  
                                rval = write(cm->attach_fd, &msg, sizeof(msg));
                                rval = write(cm->attach_fd, cm, 
                                        sizeof(struct conn_made_s));
                                close(cm->attach_fd);
                                cm->attach_fd = -1;
                        }
		} else {

			/* transparent outgoing call and static link */
			for (cf = conn_conf; cf; cf = cf->next) {
				if(cf->ip.ipfd && cf->remote.sin_addr.s_addr 
					== cm->dst.sin_addr.s_addr)
					break;
			}
			if (!cf) {
				ppp_close_conn(cm);
				ppp_syslog(LOG_WARNING, gettxt(":234", "interface not configured"));
				break;
			}
	
			bcopy((char *)&cf->ip, (char *)&cm->ip,
				 sizeof(struct ip_made_s));
			bcopy(cf->ifname, cm->ifname, sizeof(cf->ifname));
			cm->conf = cf;
			cf->made = cm;
			ifptr = cf->ifname;

			/* mark the interface up */
			(void)sprintf(ifr.ifr_name, "%s", cf->ifname);
			if (sioctl(cf->ip.ipfd, SIOCGIFFLAGS, (caddr_t)&ifr,
				sizeof(struct ifreq)) < 0 ) { 
				ppp_close_conn(cm);
				ppp_syslog(LOG_WARNING, gettxt(":235", "SIOCGIFFLAGS fail: %m"));
				break;
			}

			ifr.ifr_flags |= IFF_UP;
			if (sioctl(cf->ip.ipfd, SIOCSIFFLAGS, (caddr_t)&ifr, 
				sizeof(struct ifreq)) < 0 ) { 
				ppp_close_conn(cm);
				ppp_syslog(LOG_WARNING, gettxt(":117", "SIOCSIFFLAGS fail: %m"));
				break;
			}
		}

		/* inform ppp driver that the link is up now */
		if (sioctl(ppcidfd, P_LINK, (caddr_t)&cm->muxid,
				sizeof(cm->muxid))<0) {
			ppp_syslog(LOG_WARNING, gettxt(":236", "P_LINK fail: %m"));
			ppp_close_conn(cm);
		}

		/* set up proxy arp */
		if (cm->proxy) {
			ret = proxyadd(ipfd, cm->dst.sin_addr.s_addr, 
				cm->src.sin_addr.s_addr);
			if (ret < 0)
				ppp_syslog(LOG_WARNING, gettxt(":237", "proxyadd fail: %m"));
			if (ret < 1)
				cm->proxy = 0;
		}

		/* inform ip the debugging level */
		if (cm->debug) {
			(void)sprintf(ifr.ifr_name, "%s", ifptr);
			ifr.ifr_metric = cm->debug;
			if (sioctl(ipfd, SIOCSIFDEBUG, (caddr_t)&ifr,
				sizeof(struct ifreq)) < 0 ) 
				ppp_syslog(LOG_WARNING, gettxt(":192", "SIOCSIFDEBUG fail: %m"));
		}
		break;
	}
	goto do_over;
}

util_syslog(message)
char *message;
{
        ppp_syslog(LOG_ERR,message);
}


#ifdef __STDC__
ppp_syslog(int pri, char *fmt, ...)
#else
ppp_syslog(pri, fmt, va_alist)
	int pri;
	char *fmt;
	va_dcl
#endif
{
	va_list args;

#ifdef __STDC__
	va_start(args,fmt);
#else
	va_start(args);
#endif
	
	/*
	 * See if pppstat is monitoring ... (pppstat -m)
	 */

	if (debug_fd != -1) {
	  char *ptr;

	  memset(debug_buf, 0, sizeof(debug_buf));	  

	  /*
	   * Check fmt for %m
	   */

	  if ((ptr = strstr(fmt, "%m")) != NULL) {
	    memset(debug_fmt_buf, 0, sizeof(debug_fmt_buf));	  
	    strncat(debug_fmt_buf, fmt, (u_long) ptr - (u_long) fmt);
	    strcat(debug_fmt_buf, strerror(errno));
	    strcat(debug_fmt_buf, ptr+2);
	    fmt = debug_fmt_buf;
	  }

	  vsprintf(debug_buf, fmt, args);
	  debug_buf[strlen(debug_buf)] = '\n';

	  if (write(debug_fd, debug_buf, sizeof(debug_buf)) == -1) {
	    /* Connection was closed */
	    debug_fd = -1;
	  }
	}

	if (pri == LOG_INFO && Debug == 0)
		return;	

	/* 
	 * If daemon opens too many files, syslog will fail.
	 * write the message to console
	 */
	if (pri <= LOG_WARNING && errno == EMFILE) {
		pfmt(stderr, MM_ERROR, ":238:pppd(Too many open files): ");
		vfprintf(stderr, fmt, args);
		fprintf(stderr, "\n");
		fflush(stderr);
		errno = 0;
	}
	vsyslog(pri, fmt, args);
}

/* Global variables shared between dial process and its signal handler */
int	dialfd = 0, ntfy_kern = 1;
struct sockaddr_in	rifaddr;

/* signal processor for dial process when gets SIGUSER1 from the main daemon
 */
void
sig_undial(sig)
int sig;
{
	ppp_syslog(LOG_INFO, gettxt(":240", "sig_undial: kill dial process"));
	if (dialfd) {
		myundial(dialfd);
		dialfd = 0;
	}
	exit(0);
}

/* signal processor for dial process : SIGHUP, SIGINT, SIGQUIT and SIGTERM */
void
sig_ntfy(sig)
int sig;
{
	int	flgs = 0, s, rval;
	msg_t	msg;
	
	ppp_syslog(LOG_INFO, gettxt(":241", "sig_ntfy: kill dial process"));
	if (dialfd) {
		myundial(dialfd);
		dialfd = 0;
	}
	if (ntfy_kern) { 
		/* get a sig before finish dialing 
		 * inform the kernel
		 */
		ntfy_kern = 0;
		ctlbuf.function = PPCID_RSP;
		ctlbuf.l_index = -1;
		memcpy(&databuf.di_ia.ia_dstaddr, &rifaddr, sizeof(struct sockaddr_in));
		ctlsb.len = sizeof(struct ppp_ppc_inf_ctl_s);
		datasb.len = sizeof(struct ppp_ppc_inf_dt_s);
		if (putmsg(ppcidfd, &ctlsb, &datasb, flgs) < 0)
			ppp_syslog(LOG_WARNING, gettxt(":124", "ppcid put msg fail: %m"));
	} else { 
		/* get a sig after finish dialing
		 * inform the daemon
		 */ 
		s = ppp_sockinit();
		if (s < 0) {
			ppp_syslog(LOG_INFO, gettxt(":82", "can't connect to pppd"));
			exit(0);
		}
		memset((char *)&msg, 0, sizeof(msg));
		msg.m_type = MPID;
		msg.m_pid = getpid();
		rval = write(s, (char *)&msg, sizeof(msg));
		if (rval < 0) {
			ppp_syslog(LOG_INFO, gettxt(":83", "write to socket failed: %m"));
			exit(0);
		}
		close(s);
		ppp_syslog(LOG_INFO, gettxt(":84", "notify_daemon sig=%d pid=%d"), sig, msg.m_pid);
	}	
	exit(0);
}

/* a process dial the remote side */
pppdial(hostent, s_attach)
	struct ppphostent hostent;
        int s_attach;  /* Socket connected to pppattach cmd if s_attach != -1 */
{
	int	i, retry = 0, s, rval, flgs=0, nsysfd;
	char	*p; 
	msg_t	msg;
	nsysfd = getdtablesize();

	/* close all open file descriptor */
	for (i = 3; i < nsysfd; i++) {
	        if ((i == ppcidfd) || ((s_attach != -1) && (i == s_attach)))
			continue;
		close(i);
	}

	/* If this is a attach(non-transparent) link, no need to inform
 	 * kernel on failure
	 */
	if (*hostent.attach)
		ntfy_kern = 0;
	
	memcpy(&rifaddr, &hostent.ppp_cnf.remote, sizeof(struct sockaddr_in));	
	sigset(SIGCLD, SIG_DFL);
	sigset(SIGHUP, sig_ntfy);
	sigset(SIGINT, sig_ntfy);
	sigset(SIGQUIT, sig_ntfy);
	sigset(SIGTERM, sig_ntfy);
	sigset(SIGUSR1, sig_undial);

tryagain:

	ppp_syslog(LOG_INFO, gettxt(":242", "Dialing host %s - uucp system %s"),
		inet_ntoa(hostent.ppp_cnf.remote.sin_addr),
		hostent.uucp_system);

	/* dial and do the chat script */
	if ((dialfd = ppp_conn(hostent.uucp_system)) < 0) {
	  
	  ppp_syslog(LOG_WARNING, gettxt(":244", "Dial failure host '%s' using uucp system '%s' errno = %d (%d)"),
		     inet_ntoa(hostent.ppp_cnf.remote.sin_addr),
		     hostent.uucp_system, errno, i);
		
	  if (++retry < (int)hostent.uucpretry)
	    goto tryagain;
	  goto fail;
	}
	if (!(p = ttyname(dialfd))){
		ppp_syslog(LOG_WARNING, gettxt(":245", "dialfd returned from conn() not tty"));
		goto fail;
	}

	/* If dial succeeds, send a MDIAL or MDATTACH message to 
         * the main daemon
	 */	
	memset((char *)&msg, 0, sizeof(msg));
	msg.m_pid = getpid();
	strncpy(msg.m_tty, p, TTY_SIZE);
	if (*hostent.attach) {
		msg.m_type = MDATTACH;
		msg.m_fd = s_attach;
		memcpy(msg.m_attach, hostent.attach, strlen(hostent.attach));
	} else {
		msg.m_type = MDIAL;
		memcpy(&msg.m_remote, &hostent.ppp_cnf.remote, 
			sizeof(struct sockaddr_in));
	}

	s = ppp_sockinit();
	if (s < 0) {
		ppp_syslog(LOG_WARNING, gettxt(":246", "dial process: can't connect to pppd"));
		goto fail;
	}
	rval = write(s, (char *)&msg, sizeof(msg));
	if (rval < 0) {
		ppp_syslog(LOG_WARNING, gettxt(":247", "dial process: write to socket failed: %m"));
		goto fail;
	}
	ntfy_kern = 0;

	close(s);

	ppp_syslog(LOG_INFO, gettxt(":248", "dial process sent pppd m_type=%d, m_pid=%d, m_tty='%s'"),
			msg.m_type, msg.m_pid, msg.m_tty);
	/*
	 * Wait for daemon to notify (signal)
	 * us that we can go away.
	 */

	while (pause() == -1);

	exit(0);

fail:   
	if (s_attach != -1) {
                /*
                 * Tell pppd that dial failed.
                 */

                s = ppp_sockinit();
                if (s < 0) {
                        ppp_syslog(LOG_WARNING,
                                gettxt(":246", "dial process: can't connect to pppd"));
                        exit(0);
                }
                memset((char *)&msg, 0, sizeof(msg));
                msg.m_pid = getpid();
                msg.m_type = MDATTACH_FAIL;
                msg.m_fd = s_attach;
                rval = write(s, (char *)&msg, sizeof(msg));
                if (rval < 0)
                        ppp_syslog(LOG_WARNING,
                        gettxt(":249", "pppdial: failed to send failure message to pppd"));
                /*
                 * Wait for daemon to notify (signal)
                 * us that we can go away.
                 */

                while (pause() == -1);


        }

	if (!ntfy_kern)
		exit(0);
	
	/* For a transparent link:
         * If dial fails, send a message to the kernel directly.
	 * Note it is possible the link between dial process
         * and main daemon could be broken
	 */	
	ntfy_kern = 0;	
	ctlbuf.function = PPCID_RSP;
	ctlbuf.l_index = -1;
	memcpy(&databuf.di_ia.ia_dstaddr, &hostent.ppp_cnf.remote, 
		sizeof(struct sockaddr_in));
	ctlsb.len = sizeof(struct ppp_ppc_inf_ctl_s);
	datasb.len = sizeof(struct ppp_ppc_inf_dt_s);
	if (putmsg(ppcidfd, &ctlsb, &datasb, flgs) < 0)
		ppp_syslog(LOG_WARNING, gettxt(":124", "ppcid put msg fail: %m"));
	exit(0);
}

/*
 * Return an unused interface name
 * 
 */

char *
ppp_getifname()
{
  struct conn_made_s *cm;
  struct conn_conf_s *cf;
  int ifnum = 0;
  u_char *map;
  int gap = 0;
  int i;

  /*
   * Allocate Array the to check for gaps in the interface
   * numbering.
   */
  if ((map = malloc(sizeof(u_char) * num_ppp_interfaces)) == NULL) {
    ppp_syslog(LOG_ERR, gettxt(":250", "ppp_getifname: malloc failed"));    
    return(NULL);
  }
   
  bzero(map, sizeof(u_char) * num_ppp_interfaces);


  /* Search for an unused interface name */
  for (cm = conn_made; cm; cm = cm->next) {

    if (cm->ifname[0] == 0)
	continue; /* Interface not named yet */ 

    if ((ifnum = strtoul(&(cm->ifname[strlen(PPP_PREF)]), 
			 NULL, 10)) < num_ppp_interfaces) {
      map[ifnum] = 1;
    }
    else 
      gap++; 
  }

  for (cf = conn_conf; cf; cf = cf->next) {

    if (cf->ifname[0] == 0)
	continue; /* Interface not named yet */ 

    if ((ifnum = strtoul(&(cf->ifname[strlen(PPP_PREF)]), 
			 NULL, 10)) < num_ppp_interfaces) {
      map[ifnum] = 1;
    }
    else 
      gap++; 
  }

  /*
   * If gap is set, then there is a gap in the interface numbering 
   * from 0 --> (num_ppp_interfaces -1). So let's reuse one of theses numbers
   * If there is not a gap, then just use num_ppp_interfaces.
   */

  if (gap) {
    for (i=0; i<num_ppp_interfaces; i++) {
	ppp_syslog(LOG_INFO, "map[%d] = %d", i, map[i]);
    }
    for (i=0; i<num_ppp_interfaces; i++) {
      if (map[i] == 0) {
	ifnum = i;
	break;
      }
    }
  }
  else 
    ifnum = num_ppp_interfaces;

  free(map);
  
  sprintf(ifname, "%s%d", PPP_PREF, ifnum);

  return(ifname);
}

  
      


pppd_mod_list_t *
pop_modules(fd)
int fd;
{
  pppd_mod_list_t *lp, *lp_prev;
  char mod_name[PPP_MOD_NM_LN];

  /*
   * Create a linked list of modules that were popped off
   * of the fd.  Create the linked list backwards so that 
   * it will be in the order that the modules should be
   * pushed back onto the fd at a later time.
   */

  lp_prev = NULL;

  while (ioctl(fd, I_LOOK, mod_name) >= 0) {
    
    ppp_syslog(LOG_INFO, gettxt(":251", "pop_modules: Popping Module %s\n"), mod_name);

    if (ioctl(fd, I_POP, mod_name) < 0) {
      ppp_syslog(LOG_INFO, gettxt(":252", "pop_modules: ioctl I_POP failed: %m"));
      return(NULL);
    }

    /* Record the module that was popped */
    if ((lp = malloc(sizeof(pppd_mod_list_t))) == NULL) {
      ppp_syslog(LOG_WARNING,gettxt(":253", "pop_modules: Could not create module list for fd = %d"), fd);
      return(NULL);
    }

    strcpy(lp->mod_name, mod_name);

    lp->next = lp_prev;
 
    lp_prev = lp;
  }

  return(lp);
}


int 
restore_popped_mods(fd, lp)
int fd;
pppd_mod_list_t *lp;
{
  char *mod_name[PPP_MOD_NM_LN];
  pppd_mod_list_t *free_lp;

  if (lp == NULL) {
    ppp_syslog(LOG_WARNING, gettxt(":254", "restore_popped_mods: lp == NULL for fd == %d"), fd);
    return(-1);
  }

  /* 
   * POP any modules that we may have pushed
   */

  while (ioctl(fd, I_LOOK, mod_name) >= 0) {
    ppp_syslog(LOG_INFO,gettxt(":255", "restore_popped_mods: Popping Module %s\n"), mod_name);
    if (ioctl(fd, I_POP, mod_name) < 0) {
      ppp_syslog(LOG_WARNING, 
		 gettxt(":256", "restore_popped_mods: ioctl I_POP failed: %m (fd = %d)"), fd);
      return(-1);
    }
  }

  while (lp != NULL) {
    ppp_syslog(LOG_INFO,
	       gettxt(":257", "restore_popped_mods: Pushing Module %s\n"), lp->mod_name);

    if (ioctl(fd, I_PUSH, lp->mod_name) == -1) { 
      ppp_syslog(LOG_WARNING, 
		 gettxt(":258", "restore_popped_mods: ioctl I_PUSH fail: %m (fd = %d)"), fd);
      return(-1);
    }
    
    free_lp = lp;
    lp = lp->next;
    free(free_lp);
  }

  return(0);
}


int
ppp_conn(name)
char *name;
{
  CALL call;
  CALL_EXT  call_ext;
  int dial_fd;
  char            service[] = "uucico";


  call.attr = NULL;               /* termio attributes */
  call.baud = 0;                  /* unused */
  call.speed = -1;                /* any speed */
  call.line = NULL;
  call.telno = name;              /* give name, not number */
  call.modem = 0;                 /* modem control not required */
  call.device = (char *)&call_ext;
  call.dev_len = 0;               /* unused */
  
  call_ext.service = service;
  call_ext.class = NULL;
  call_ext.protocol = NULL;
  
  if ((dialfd = dial(call)) < 0) 
    return(-1);
  
  return(dialfd);
}


int
print_termios(term)
struct termios *term;
{
  ppp_syslog(LOG_INFO, gettxt(":259", "term->c_iflag = Octal: %o\n"), term->c_iflag);
  ppp_syslog(LOG_INFO, gettxt(":260", "term->c_oflag = Octal: %o\n"), term->c_oflag);
  ppp_syslog(LOG_INFO, gettxt(":261", "term->c_cflag = Octal: %o\n"), term->c_cflag);
  ppp_syslog(LOG_INFO, gettxt(":262", "term->c_lflag = Octal: %o\n"), term->c_lflag);
  ppp_syslog(LOG_INFO, gettxt(":263", "term->c_cc =  Not printed\n"));

  ppp_syslog(LOG_INFO, gettxt(":264", "c_cflags decoded:\n"));
  
  /* CSIZE */
  ppp_syslog(LOG_INFO, "c_cflag: ");
  switch (CSIZE & term->c_cflag) {
  case CS5:
    ppp_syslog(LOG_INFO, "CS5 ");
    break;

  case CS6:
    ppp_syslog(LOG_INFO, "CS6 ");
    break;

  case CS7:
    ppp_syslog(LOG_INFO, "CS7 ");
    break;

  case CS8:
    ppp_syslog(LOG_INFO, "CS8 ");
    break;
    
  default:
    ppp_syslog(LOG_INFO, gettxt(":265", "INVALID "));
    break;
  } /* switch */

  if (term->c_cflag & CSTOPB)
    ppp_syslog(LOG_INFO, "CSTOP ");

  if (term->c_cflag & CREAD)
    ppp_syslog(LOG_INFO, "CREAD ");

  if (term->c_cflag & PARENB)
    ppp_syslog(LOG_INFO, "PARENB ");

  if (term->c_cflag & PARODD)
    ppp_syslog(LOG_INFO, "PARODD ");

  if (term->c_cflag & PARENB)
    ppp_syslog(LOG_INFO, "PARENB ");

  if (term->c_cflag & HUPCL)
    ppp_syslog(LOG_INFO, "HUPCL ");

  if (term->c_cflag & CLOCAL)
    ppp_syslog(LOG_INFO, "CLOCAL ");

  ppp_syslog(LOG_INFO, "CBAUD: ");
  switch (term->c_cflag & CBAUD) {
  case B0:
    ppp_syslog(LOG_INFO, "B0 ");
    break;

  case B50:
    ppp_syslog(LOG_INFO, "B50 ");
    break;

  case B75:
    ppp_syslog(LOG_INFO, "B75 ");
    break;

  case B110:
    ppp_syslog(LOG_INFO, "B110 ");
    break;

  case B134:
    ppp_syslog(LOG_INFO, "B134 ");
    break;

  case B150:
    ppp_syslog(LOG_INFO, "B150 ");
    break;

  case B200:
    ppp_syslog(LOG_INFO, "B200 ");
    break;

  case B300:
    ppp_syslog(LOG_INFO, "B300 ");
    break;

  case B600:
    ppp_syslog(LOG_INFO, "B600 ");
    break;

  case B1200:
    ppp_syslog(LOG_INFO, "B1200 ");
    break;

  case B1800:
    ppp_syslog(LOG_INFO, "B1800 ");
    break;

  case B2400:
    ppp_syslog(LOG_INFO, "B2400 ");
    break;

  case B4800:
    ppp_syslog(LOG_INFO, "B4800 ");
    break;

  case B9600:
    ppp_syslog(LOG_INFO, "B9600 ");
    break;

  case B19200:
    ppp_syslog(LOG_INFO, "B19200 ");
    break;

  case B38400:
    ppp_syslog(LOG_INFO, "B38400 ");
    break;

  default:
    ppp_syslog(LOG_INFO, gettxt(":265", "INVALID "));
    break;
  }

  ppp_syslog(LOG_INFO, "CIBAUD: ");
  switch ((term->c_cflag & CIBAUD) >> 16) {
  case B0:
    ppp_syslog(LOG_INFO, "B0 ");
    break;

  case B50:
    ppp_syslog(LOG_INFO, "B50 ");
    break;

  case B75:
    ppp_syslog(LOG_INFO, "B75 ");
    break;

  case B110:
    ppp_syslog(LOG_INFO, "B110 ");
    break;

  case B134:
    ppp_syslog(LOG_INFO, "B134 ");
    break;

  case B150:
    ppp_syslog(LOG_INFO, "B150 ");
    break;

  case B200:
    ppp_syslog(LOG_INFO, "B200 ");
    break;

  case B300:
    ppp_syslog(LOG_INFO, "B300 ");
    break;

  case B600:
    ppp_syslog(LOG_INFO, "B600 ");
    break;

  case B1200:
    ppp_syslog(LOG_INFO, "B1200 ");
    break;

  case B1800:
    ppp_syslog(LOG_INFO, "B1800 ");
    break;

  case B2400:
    ppp_syslog(LOG_INFO, "B2400 ");
    break;

  case B4800:
    ppp_syslog(LOG_INFO, "B4800 ");
    break;

  case B9600:
    ppp_syslog(LOG_INFO, "B9600 ");
    break;

  case B19200:
    ppp_syslog(LOG_INFO, "B19200 ");
    break;

  case B38400:
    ppp_syslog(LOG_INFO, "B38400 ");
    break;

  default:
    ppp_syslog(LOG_INFO, gettxt(":265", "INVALID "));
    break;
  }



  return(0);
}


/* 
 * Digests a string and prints the result.
 *
 * NOTE: This string also exists in libmd5, but was also
 *       placed here because with UnixWare we do not use 
 *       libmd5.  We do this because UnixWare already 
 *       has a md5 object that we can link against.
 */

unsigned char digest[16];

char *
MD5String (string, len)
char *string;
int len;
{
        MD5_CTX context;

        MD5Init (&context);
        MD5Update (&context, (unsigned char *)string, len);
        MD5Final (digest, &context);

        return((char *)digest);
}

  
