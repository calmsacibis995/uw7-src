#ident	"@(#)in.rlogind.c	1.5"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */

/*
 * remote login server:
 *	\0
 *	remuser\0
 *	locuser\0
 *	terminal info\0
 *	data
 */

#include <unistd.h>
#include <locale.h>
#include <pfmt.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/file.h>
#ifdef SYSV
#include <sys/stream.h>
#include <stropts.h>
#include <poll.h>
#endif SYSV
#include "./security.h"

#include <netinet/in.h>

#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>
#include <syslog.h>
#include <string.h>
#include <utmp.h>
#include <arpa/nameser.h>
#include <resolv.h>
#ifdef SYSV
#include <sac.h>	/* for SC_WILDC */
#include <utmpx.h>
#include <sys/filio.h>

#define bzero(s,n)	  memset((s), 0, (n))
#define bcopy(a,b,c)	  memcpy(b,a,c)
#define signal(s,f)	  sigset(s,f)

#include <sys/termios.h>
#include <sys/termio.h>
#include <sys/tihdr.h>
#include <xti.h>
#define TIOCPKT_FLUSHWRITE      0x02    /* flush data read from
                                           controller but not yet
                                           processed */
#define TIOCPKT_NOSTOP          0x10    /* no more ^S, ^Q */
#define TIOCPKT_DOSTOP          0x20    /* now do ^S, ^Q */
#define TIOCPKT_WINDOW 		0x80

#include <sys/mkdev.h>

#define bzero(s,n)	  memset((s), 0, (n))
#define bcopy(a,b,c)	  memcpy(b,a,c)
#define	bcmp		  memcmp
#define signal(s,f)	  sigset(s,f)
#endif SYSV

static int	_check_rhosts_file = 1;
int	keepalive = 1;
int	check_all = 0;
int	reapchild();
struct	passwd *getpwnam();
char	*malloc();
char	*netname();


main(argc, argv)
	int argc;
	char **argv;
{
	extern int opterr, optind;
	int ch;
	int on = 1;
	size_t fromlen;
	struct sockaddr_in from;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxrlogin");
	(void)setlabel("UX:in.rlogind");

	openlog("rlogind", LOG_PID | LOG_CONS, LOG_AUTH);

	opterr = 0;
	while ((ch = getopt(argc, argv, "aln")) != EOF)
		switch (ch) {
		case 'a':
			check_all = 1;
			break;
		case 'l':
			_check_rhosts_file = 0;
			break;
		case 'n':
			keepalive = 0;
			break;
		case '?':
		default:
			syslog(LOG_ERR, gettxt(":13", "Usage: rlogind [-a] [-l] [-n]"));
			break;
		}
	argc -= optind;
	argv += optind;

	fromlen = sizeof (from);
	if (getpeername(0, (struct sockaddr *)&from, &fromlen) < 0) {
		syslog(LOG_ERR, gettxt(":14", "Couldn't get peer name of remote host: %m"));
		fatal(0, gettxt(":15", "Can't get peer name of remote host"));
	}
	if (keepalive &&
		setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof (on)) < 0) {
		syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");
	}
	doit(0, &from);
	/* NOTREACHED */
}

int	deadshell();
int	netf;
extern	errno;
char	*line;
extern	char	*inet_ntoa();

char	oobdata[] = {(char) TIOCPKT_WINDOW};
struct winsize win = { 0, 0, 0, 0 };
pid_t pid;

void
cleanup(int placeholder)
{

	rmut();
	/*
	 * the shutdown(netf, 2); is no longer needed since
	 * session.c:freectty() now does it
	 */
	exit(1);
}

doit(f, fromp)
	int f;
	struct sockaddr_in *fromp;
{
	int i, p, t, on = 1;
	register struct hostent *hp;
	struct hostent hostent;
	char c;
	char remotehost[2 * MAXHOSTNAMELEN + 1];
#ifdef SYSV
	extern char *ptsname();
	struct termios tp;
#endif SYSV
	int	stat_loc;

	alarm(60);
	read(f, &c, 1);
	if (c != 0)
		exit(1);
	alarm(0);
	fromp->sin_port = ntohs((u_short)fromp->sin_port);
	hp = gethostbyaddr((char *)&fromp->sin_addr, sizeof (struct in_addr),
		fromp->sin_family);
	if (hp == 0) {
		/*
		 * Only the name is used below.
		 */
		hp = &hostent;
		hp->h_name = inet_ntoa(fromp->sin_addr);
	} else if (check_all || local_domain(hp->h_name)) {
		/*
		 * If name returned by gethostbyaddr is in our domain,
		 * attempt to verify that we haven't been fooled by someone
		 * in a remote net; look up the name and check that this
		 * address corresponds to the name.
		 */
		strncpy(remotehost, hp->h_name, sizeof(remotehost) - 1);
		remotehost[sizeof(remotehost) - 1] = 0;
#ifdef RES_DNSRCH
		/* 
		 * gethostbyaddr() returns a FQDN, so now the domain search
		 * action must be turned off to avoid unwanted queries to
		 * the nameserver.
		 */
		 _res.options &= ~RES_DNSRCH;
#endif /* RES_DNSRCH */
		hp = gethostbyname(remotehost);
		if (hp == NULL) {
			syslog(LOG_INFO,
			    gettxt(":16", "Couldn't look up address for %s"),
			    remotehost);
			fatal(f, gettxt(":17", "Couldn't look up address for your host\n"));
			exit(1);
		}
#ifdef h_addr	/* 4.2 hack */
		for (; hp->h_addr_list[0]; hp->h_addr_list++) {
			if (!bcmp(hp->h_addr_list[0], (caddr_t)&fromp->sin_addr,
			    sizeof(fromp->sin_addr)))
				break;
		}
		if (hp->h_addr_list[0] == NULL) {
			syslog(LOG_NOTICE,
			  gettxt(":18", "Host addr %s not listed for host %s"),
			    inet_ntoa(fromp->sin_addr),
			    hp->h_name);
			fatal(f, gettxt(":19", "Host address mismatch\n"));
			exit(1);
		}
#else
		if (bcmp(hp->h_addr, (caddr_t)&fromp->sin_addr,
			sizeof(fromp->sin_addr))) {
			syslog(LOG_NOTICE,
			  gettxt(":18", "Host addr %s not listed for host %s"),
			    inet_ntoa(fromp->sin_addr),
			    hp->h_name);
			fatal(f, gettxt(":19", "Host address mismatch\n"));
				exit(1);
		}
#endif
	}
		

	if (fromp->sin_family != AF_INET ||
	    fromp->sin_port >= IPPORT_RESERVED ||
	    fromp->sin_port < (u_short) (IPPORT_RESERVED/2)) {
		syslog(LOG_NOTICE, gettxt(":20", "Connection from %s on illegal port"),
			inet_ntoa(fromp->sin_addr));
		fatal(f, gettxt(":21", "Permission denied"));
	}
#ifdef IP_OPTIONS
      {
	u_char optbuf[BUFSIZ/3], *cp;
	char lbuf[BUFSIZ], *lp;
	size_t optsize = sizeof(optbuf);
	int ipproto;
	struct protoent *ip;

	if ((ip = getprotobyname("ip")) != NULL)
		ipproto = ip->p_proto;
	else
		ipproto = IPPROTO_IP;
	if (getsockopt(0, ipproto, IP_OPTIONS, (char *)optbuf, &optsize) == 0 &&
	    optsize != 0) {
		lp = lbuf;
		for (cp = optbuf; optsize > 0; cp++, optsize--, lp += 3)
			sprintf(lp, " %2.2x", *cp);
		syslog(LOG_NOTICE,
		    gettxt(":22", "Connection received using IP options (ignored):%s"), lbuf);
		if (setsockopt(0, ipproto, IP_OPTIONS,
		    (char *)NULL, optsize) != 0) {
			syslog(LOG_ERR, gettxt(":23", "setsockopt IP_OPTIONS NULL: %m"));
			exit(1);
		}
	}
      }
#endif
	write(f, "", 1);
	if (ioctl (f, I_PUSH, "rlogin") == -1)
		fatalperror (f, "ioctl I_PUSH rlogin", errno);
	if (ioctl (f, I_PUSH, "ldterm") == -1)
		fatalperror (f, "ioctl I_PUSH ldterm", errno);
	if (ioctl (f, I_PUSH, "ttcompat") == -1)
		fatalperror (f, "ioctl I_PUSH ttcompat", errno);
        if ((line = netname(f)) == NULL)
                fatal (f, gettxt(":26", "could not enable slave pty"));
	/* 
	 * Make sure the pty doesn't modify the strings passed
	 * to login as part of the "rlogin protocol."  The login
	 * program should set these flags to apropriate values
	 * after it has read the strings.
	 */
	if (ioctl (f, TCGETS, &tp) == -1)
		fatalperror(f, "ioctl TCGETS", errno);
	tp.c_lflag &= ~(ECHO|ICANON);
	tp.c_oflag &= ~(XTABS|OCRNL);
	tp.c_iflag &= ~(IGNPAR|ICRNL /*|ISTRIP*/ );
	tp.c_cc[VMIN] = 1;
	tp.c_cc[VTIME] = 0;
	if (ioctl (f, TCSETS, &tp) == -1)
		fatalperror(f, "ioctl TCSETS", errno);

	/*
	 * System V ptys allow the TIOC{SG}WINSZ ioctl to be
	 * issued on the master side of the pty.  Luckily, that's
	 * the only tty ioctl we need to do do, so we can close the
	 * slave side in the parent process after the fork.
	 */
	(void) ioctl(f, TIOCSWINSZ, &win);
	netf = f;
	/* if the child we are about to fork dies, cleanup */
	(void) signal(SIGCHLD, (void (*)())cleanup);
	pid = fork();
	if (pid < 0)
		fatalperror(f, "fork", errno);
	if (pid == 0) {

		int tt;
		struct utmpx ut;

		ioctl(f, FIONBIO, &on);
		/* controlling tty */
		if ( setsid() == -1 )
			fatalperror(f, "setsid", errno);

		 if ((tt = open (line, O_RDWR)) == -1)
		 	fatalperror(f, line, errno);
		 

		/* System V login expects a utmp entry to already be there */
		bzero ((char *) &ut, sizeof (ut));
		(void) strncpy(ut.ut_user, ".rlogin", sizeof(ut.ut_user));
		(void) strncpy(ut.ut_line, line, sizeof(ut.ut_line));
		ut.ut_pid = (o_pid_t)getpid();
		ut.ut_id[0] = 'r';
		ut.ut_id[1] = 'l';
		ut.ut_id[2] = SC_WILDC;
		ut.ut_id[3] = SC_WILDC;
		ut.ut_type = LOGIN_PROCESS;
		ut.ut_exit.e_termination = 0;
		ut.ut_exit.e_exit = 0;
		(void) time (&ut.ut_tv.tv_sec);
		if (makeutx(&ut) == NULL) {
			syslog(LOG_INFO, gettxt(":29", "makeutx failed"));
			fatal (f, gettxt(":30", "cannot create utmp entry"));
		}

		close(f);
		dup2(tt, 0), dup2(tt, 1), dup2(tt, 2);
		close(tt);

		{
#define	ARG_COUNT	256
			char buf_invoke[ARG_COUNT];
			int  retval;
			struct T_data_req data_req;
			struct strbuf databuf, ctrlbuf;

			/*NOTE: the -R option replaces the SVR4.0 -r option*/
			strcpy(buf_invoke,
				"/usr/lib/iaf/in.login/scheme -R ");
			if (!_check_rhosts_file)
				strcat(buf_invoke, "-L ");
			strncat(buf_invoke,
				hp->h_name,
				ARG_COUNT-strlen(buf_invoke));
	
			retval = invoke(0, buf_invoke);

			if (0 == retval) {
				if (set_id((char *)0) != 0) {
					fatalperror(1,"set_id", errno);
					exit(2);
				}
				set_env();
				/*
				 * There appears to be a race here, that
				 * also shows up on the pty rlogin.  
				 * This oobdata gets lost, and the client
				 * doesn't think we support window size
				 * changes.
				 * The send indicates new rlogin
				 * supporting window size changes. 
				 */
				data_req.PRIM_type = T_EXDATA_REQ;
				data_req.MORE_flag = 0;
				databuf.len = 1;
				databuf.buf = &oobdata[0];
				ctrlbuf.len = sizeof (data_req);
				ctrlbuf.buf = (void *) &data_req;
				putmsg(1, &ctrlbuf, &databuf, 0);

				execl("/usr/bin/shserv", "shserv", 0);
				fatalperror(1,"exec", errno);
			}
		}

		exit(1);
		/*NOTREACHED*/
	}
	exit(0);
}

fatal(f, msg)
	int f;
	char *msg;
{
	char buf[BUFSIZ];

	buf[0] = '\01';		/* error indicator */
	(void) sprintf(buf + 1, "UX:in.rlogind: %s.\r\n", msg);
	(void) write(f, buf, strlen(buf));
	exit(1);
}

fatalperror(f, msg, errno)
	int f;
	char *msg;
	int errno;
{
	char buf[BUFSIZ];
	extern int sys_nerr;
	extern char *sys_errlist[];

	if ((unsigned)errno < sys_nerr)
		(void) sprintf(buf, "%s: %s", msg, strerror(errno));
	else
		(void) sprintf(buf, gettxt(":37", "%s: Error %d"), msg, errno);
	fatal(f, buf);
}


#ifdef SYSV
rmut()
{
	struct utmpx		*up;

	signal(SIGCHLD, SIG_IGN); /* while cleaning up don't allow disruption */

	setutxent();
	while ( (up = getutxent()) ) {
		if (up->ut_pid != (o_pid_t)pid)
			continue;
		up->ut_type = DEAD_PROCESS;
		up->ut_exit.e_termination = 0;
		up->ut_exit.e_exit = 0;
		(void) time (&up->ut_tv.tv_sec);
		if (modutx(up) == NULL)
			syslog(LOG_INFO, gettxt(":38", "modutx failed"));
		break;
	}
	endutxent();
	signal(SIGCHLD, (void (*)())cleanup);
}

#else /* !SYSV */

#define SCPYN(a, b)	strncpy(a, b, sizeof(a))
#define SCMPN(a, b)	strncmp(a, b, sizeof(a))

rmut()
{
	register f;
	int found = 0;
	struct utmp *u, *utmp;
	int nutmp;
	struct stat statbf;
	struct	utmp wtmp;
	char	wtmpf[]	= WTMP_FILE;
	char	utmpf[] = UTMP_FILE;

	signal(SIGCHLD, SIG_IGN); /* while cleaning up don't allow disruption */
	f = open(utmpf, O_RDWR);
	if (f >= 0) {
		fstat(f, &statbf);
		utmp = (struct utmp *)malloc(statbf.st_size);
		if (!utmp)
			syslog(LOG_ERR, gettxt(":39", "utmp malloc failed"));
		if (statbf.st_size && utmp) {
			nutmp = read(f, utmp, statbf.st_size);
			nutmp /= sizeof(struct utmp);
		
			for (u = utmp ; u < &utmp[nutmp] ; u++) {
				if (SCMPN(u->ut_line, line+5) ||
				    u->ut_name[0]==0)
					continue;
				lseek(f, ((long)u)-((long)utmp), L_SET);
				SCPYN(u->ut_name, "");
				SCPYN(u->ut_host, "");
				time(&u->ut_time);
				write(f, (char *)u, sizeof(wtmp));
				found++;
			}
		}
		close(f);
	}
	if (found) {
		f = open(wtmpf, O_WRONLY|O_APPEND);
		if (f >= 0) {
			SCPYN(wtmp.ut_line, line+5);
			SCPYN(wtmp.ut_name, "");
			SCPYN(wtmp.ut_host, "");
			time(&wtmp.ut_time);
			write(f, (char *)&wtmp, sizeof(wtmp));
			close(f);
		}
	}
	chmod(line, 0666);
	chown(line, 0, 0);
	line[strlen("/dev/")] = 'p';
	chmod(line, 0666);
	chown(line, 0, 0);
	signal(SIGCHLD, (void (*)())cleanup);
}
#endif /* SYSV */

/*
 * Check whether host h is in our local domain,
 * defined as sharing the last two components of the domain part,
 * or the entire domain part if the local domain has only one component.
 * If either name is unqualified (contains no '.'),
 * assume that the host is local, as it will be
 * interpreted as such.
 */
local_domain(h)
	char *h;
{
	char localhost[MAXHOSTNAMELEN];
	char *p1, *p2, *topdomain();

	localhost[0] = 0;
	(void) gethostname(localhost, sizeof(localhost));
	p1 = topdomain(localhost);
	p2 = topdomain(h);
	if (p1 == NULL || p2 == NULL || !strcasecmp(p1, p2))
		return(1);
	return(0);
}

char *
topdomain(h)
	char *h;
{
	register char *p;
	char *maybe = NULL;
	int dots = 0;

	for (p = h + strlen(h); p >= h; p--) {
		if (*p == '.') {
			if (++dots == 2)
				return (p);
			maybe = p;
		}
	}
	return (maybe);
}

#ifdef SYSV
/*
 * This array is designed for mapping upper and lower case letter
 * together for a case independent comparison.  The mappings are
 * based upon ascii character sequences.
 */
static char charmap[] = {
	'\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
	'\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
	'\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
	'\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
	'\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
	'\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
	'\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
	'\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
	'\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
	'\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
	'\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
	'\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
	'\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
	'\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
	'\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
	'\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
	'\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
	'\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
	'\300', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\333', '\334', '\335', '\336', '\337',
	'\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};

static
strcasecmp(s1, s2)
	register char *s1, *s2;
{
	register char *cm = charmap;

	while (cm[*s1] == cm[*s2++])
		if (*s1++ == '\0')
			return(0);
	return(cm[*s1] - cm[*--s2]);
}
#endif /* SYSV */


/* borrowed from ptsname.
 * netname will try to look in a standard location for a match,
 * then do the full ttyname search.
 */

/* minors are currently 2^18 == 262144, so devinet needs to be
 * 15 chars (strlen("/dev/_tcp/") + strlen(ntoa(2^18)) - 1).
 */

 /* This seach could be done by calling ttyname, however it
  * is most likely that the device is a tcp connection,
  * so search there first and if that fails then try ttyname()
  */

#define INETNAME  "/dev/_tcp/"
char devinet[32];

char *
netname(fd)
int fd;
{
        struct stat status, minor_status;
        struct strioctl istr;
        char *sname = devinet;

	if (fstat(fd, &status) < 0)
		return NULL;

	if ((status.st_mode & S_IFMT) != S_IFCHR)
		return NULL;

	sprintf(sname, "%s%d", INETNAME, minor(status.st_rdev));

	if (access(sname, R_OK | W_OK) == 0)	/* access allowed */
		return sname;

	/* from login.c:findttyname().
	 * Call ttyname(), but do not return syscon, systty,
	 * or sysconreal do not use syscon or systty if console
	 * is present, assuming they are links.
	 */
	if ((sname = (ttyname(fd))) != NULL) {
		if (((strcmp(sname, "/dev/syscon") == 0) ||
		     (strcmp(sname, "/dev/sysconreal") == 0) ||
		     (strcmp(sname, "/dev/systty") == 0)) &&
		     (access("/dev/console", F_OK) == 0))
			sname = "/dev/console";
	}
	return (sname);
}

