#ident	"@(#)ftpd.c	1.15"

/* Copyright (c) 1985, 1988, 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution. 3. All advertising
 * materials mentioning features or use of this software must display the
 * following acknowledgement: This product includes software developed by the
 * University of California, Berkeley and its contributors. 4. Neither the
 * name of the University nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE. 
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1985, 1988, 1990 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)$Id$ based on ftpd.c  5.40 (Berkeley) 7/2/91";
#endif /* not lint */

#define SPT_NONE	0	/* don't use it at all */
#define SPT_REUSEARGV	1	/* cover argv with title information */
#define SPT_BUILTIN	2	/* use libc builtin */
#define SPT_PSTAT	3	/* use pstat(PSTAT_SETCMD, ...) */
#define SPT_PSSTRINGS	4	/* use PS_STRINGS->... */
#define SPT_SYSMIPS	5	/* use sysmips() supported by NEWS-OS 6 */
#define SPT_SCO		6	/* write kernel u. area */

/* FTP server. */
#include "config.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/wait.h>

#ifdef AIX
#include <sys/id.h>
#include <sys/priv.h>
#endif

#ifdef AUX
#include <compat.h>
#endif

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#define FTP_NAMES
#include <arpa/ftp.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>

#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <setjmp.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>

/*
 *  Arrange to use either varargs or stdargs
 *
 */

#ifdef __STDC__

#include <stdarg.h>

#define VA_LOCAL_DECL	va_list ap;
#define VA_START(f)	va_start(ap, f)
#define VA_END		va_end(ap)

#else

#include <varargs.h>

#define VA_LOCAL_DECL	va_list ap;
#define VA_START(f)	va_start(ap)
#define VA_END		va_end(ap)

#endif


#ifdef SYSSYSLOG
#include <sys/syslog.h>
#else
#include <syslog.h>
#endif
#include <time.h>
#include "conversions.h"
#include "extensions.h"
#include "pathnames.h"

#ifdef M_UNIX
#include <arpa/nameser.h>
#include <resolv.h>
#endif

#if defined(SVR4) || defined(ISC)
#include <fcntl.h>
#endif

#ifdef HAVE_SYSINFO
#include <sys/systeminfo.h>
#endif

#ifdef SHADOW_PASSWORD
#include <shadow.h>
#endif

#ifdef KERBEROS
#include <sys/types.h>
#include <auth.h>
#include <krb.h>
#endif

#ifdef ULTRIX_AUTH
#include <auth.h>
#include <sys/svcinfo.h>
#endif

#ifndef HAVE_SYMLINK
#define lstat stat
#endif

#ifdef HAVE_DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif

#if (defined(_BSDI_VERSION) && (_BSDI_VERSION < 199501)) /* before version 2 */
#define LONGOFF_T  /* sizeof(off_t) == sizeof(long) */
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64  /* may be too big */
#endif

#ifndef TRUE
#define  TRUE   1
#endif

#ifndef FALSE
#define  FALSE  !TRUE
#endif

#if defined(_SCO_DS) && !defined(SIGURG)
#define SIGURG	SIGUSR1
#endif

/* File containing login names NOT to be used on this machine. Commonly used
 * to disallow uucp. */
extern int errno;
extern int pidfd;
#ifdef __STDC__
extern char *ctime(const time_t *);
#ifndef NO_CRYPT_PROTO
extern char *crypt(const char *, const char *);
#endif
extern FILE *ftpd_popen(char *program, char *type, int closestderr),
 *fopen(const char *, const char *),
 *freopen(const char *, const char *, FILE *);
extern int ftpd_pclose(FILE *iop),
  fclose(FILE *);
extern char *getline(),
 *realpath(const char *pathname, char *result);
#else
extern char *ctime();
#ifndef NO_CRYPT_PROTO
#ifdef _M_UNIX
    extern char *crypt(char *, char *);
#else
    extern char *crypt();
#endif
#endif
extern FILE *ftpd_popen(), *fopen(),*freopen();
extern int ftpd_pclose(), fclose();
extern char *getline(), *realpath();
#endif
extern char version[];
extern char *home;              /* pointer to home directory for glob */
extern char cbuf[];
extern off_t restart_point;
extern yyerrorcalled;

struct sockaddr_in ctrl_addr;
struct sockaddr_in data_source;
struct sockaddr_in data_dest;
struct sockaddr_in his_addr;
struct sockaddr_in pasv_addr;

#ifdef VIRTUAL
int virtual_mode=0;
char virtual_root[MAXPATHLEN];
char virtual_banner[MAXPATHLEN];
#endif

int data;
jmp_buf errcatch,
  urgcatch;
int logged_in = 0;
struct passwd *pw;
int debug;
int timeout = 900;              /* timeout after 15 minutes of inactivity */
int maxtimeout = 7200;          /* don't allow idle time to be set beyond 2
                                 * hours */

/* previously defaulted to 1, and -l or -L set them to 1, so that there was
   no way to turn them *off*!  Changed so that the manpage reflects common
   sense.  -L is way noisy; -l we'll change to be "just right".  _H*/
int logging = 0;
int log_commands = 0;

#ifdef SECUREOSF
#define SecureWare
#include <sys/acl.h>
#include <prot.h>
#endif

#ifdef	INTL
#  include <locale.h>
#  include "ftpd_msg.h"
   nl_catd catd;
#else
#  define MSGSTR(num, str)	(str)
#endif	/* INTL */

int anonymous = 1;
int guest;
int type;
int form;
int stru;                       /* avoid C keyword */
int mode;
int usedefault = 1;             /* for data transfers */
int pdata = -1;                 /* for passive mode */
int transflag;
int ftwflag;
off_t file_size;
off_t byte_count;

#if !defined(CMASK) || CMASK == 0
#undef CMASK
#define CMASK 022
#endif
mode_t defumask = CMASK;           /* default umask value */
char tmpline[7];
char hostname[MAXHOSTNAMELEN];
char remotehost[MAXHOSTNAMELEN];
char remoteaddr[MAXHOSTNAMELEN];

/* log failures 	27-apr-93 ehk/bm */
#ifdef LOG_FAILED
#define MAXUSERNAMELEN	32
char the_user[MAXUSERNAMELEN];
#endif

/* Access control and logging passwords */
/* OFF by default.  _H*/
int use_accessfile = 0;
char guestpw[MAXHOSTNAMELEN];
char privatepw[MAXHOSTNAMELEN];
int nameserved = 0;
extern char authuser[];
extern int authenticated;

/* File transfer logging */
int xferlog = 0;
int log_outbound_xfers = 0;
int log_incoming_xfers = 0;
char logfile[MAXPATHLEN];

/* Allow use of lreply(); this is here since some older FTP clients don't
 * support continuation messages.  In violation of the RFCs... */
int dolreplies = 1;

/* Spontaneous reply text.  To be sent along with next reply to user */
char *autospout = NULL;
int autospout_free = 0;

/* allowed on-the-fly file manipulations (compress, tar) */
int mangleopts = 0;

/* number of login failures before attempts are logged and FTP *EXITS* */
int lgi_failure_threshold = 5;

/* Timeout intervals for retrying connections to hosts that don't accept PORT
 * cmds.  This is a kludge, but given the problems with TCP... */
#define SWAITMAX    90          /* wait at most 90 seconds */
#define SWAITINT    5           /* interval between retries */

int swaitmax = SWAITMAX;
int swaitint = SWAITINT;

#ifdef __STDC__
SIGNAL_TYPE lostconn(int sig);
SIGNAL_TYPE randomsig(int sig);
SIGNAL_TYPE myoob(int sig);
FILE *getdatasock(char *mode),
 *dataconn(char *name, off_t size, char *mode);
void setproctitle(const char *fmt, ...);
void reply(int, char *fmt, ...);
void lreply(int, char *fmt, ...);
#else
SIGNAL_TYPE lostconn();
SIGNAL_TYPE randomsig();
SIGNAL_TYPE myoob();
FILE *getdatasock(), *dataconn();
void setproctitle();
void reply();
void lreply();
#endif

#ifdef NEED_SIGFIX
extern sigset_t block_sigmask;  /* defined in sigfix.c */
#endif

char **Argv = NULL;             /* pointer to argument vector */
char *LastArgv = NULL;          /* end of argv */
char proctitle[BUFSIZ];         /* initial part of title */

#ifdef SKEY
#include <skey.h>
int	pwok = 0;
#endif

#ifdef KERBEROS
void init_krb();
void end_krb();
char krb_ticket_name[100];
#endif /* KERBEROS */

#ifdef ULTRIX_AUTH
int ultrix_check_pass(char *passwd, char *xpasswd);
#endif

/* ls program commands and options for lreplies on and off */
char  ls_long[50];
char  ls_short[50];
struct aclmember *entry = NULL;

#ifdef __STDC__
void end_login(void);
void send_data(FILE *, FILE *, off_t);
void dolog(struct sockaddr_in *);
void dologout(int);
void perror_reply(int, char *);
#else
void end_login();
void send_data();
void dolog();
void dologout();
void perror_reply();
#endif


void
#ifdef __STDC__
main(int argc, char **argv, char **envp)
#else
main(argc,argv,envp)
int argc; char **argv; char **envp;
#endif
{
#ifdef UNIXWARE
    size_t addrlen;
#else
    int addrlen;
#endif
    int  on = 1;
#ifdef IPTOS_LOWDELAY
    int tos;
#endif
    int c;
    extern int optopt;
    extern char *optarg;
    struct hostent *shp;
#ifdef VIRTUAL
    int virtual_len;
    struct sockaddr_in virtual_addr;
    struct sockaddr_in *virtual_ptr;
#endif

#ifdef	INTL
    setlocale(LC_ALL, "");
    catd = catopen(MF_FTPD, MC_FLAGS);
#endif	/* INTL */

#ifdef AUX
    setcompat(COMPAT_POSIX | COMPAT_BSDSETUGID);
#endif

#ifdef FACILITY
    openlog("ftpd", LOG_PID | LOG_NDELAY, FACILITY);
#else
    openlog("ftpd", LOG_PID);
#endif

#ifdef SecureWare
    setluid(1);                         /* make sure there is a valid luid */
    set_auth_parameters(argc,argv);
    setreuid(0, 0);
#endif
#if defined(M_UNIX) && !defined(_M_UNIX)
    res_init();                         /* bug in old (1.1.1) resolver     */
    _res.retrans = 20;                  /* because of fake syslog in 3.2.2 */
    setlogmask(LOG_UPTO(LOG_INFO));
#endif

    addrlen = sizeof(his_addr);
    if (getpeername(0, (struct sockaddr *) &his_addr, &addrlen) < 0) {
        syslog(LOG_ERR, "getpeername (%s): %m", argv[0]);
#ifndef DEBUG
        exit(1);
#endif
    }
    addrlen = sizeof(ctrl_addr);
    if (getsockname(0, (struct sockaddr *) &ctrl_addr, &addrlen) < 0) {
        syslog(LOG_ERR, "getsockname (%s): %m", argv[0]);
#ifndef DEBUG
        exit(1);
#endif
    }
#ifdef IPTOS_LOWDELAY
    tos = IPTOS_LOWDELAY;
    if (setsockopt(0, IPPROTO_IP, IP_TOS, (char *) &tos, sizeof(int)) < 0)
          syslog(LOG_WARNING, "setsockopt (IP_TOS): %m");
#endif

    data_source.sin_port = htons(ntohs(ctrl_addr.sin_port) - 1);
    debug = 0;

    /* Save start and extent of argv for setproctitle. */
    Argv = argv;
    while (*envp)
        envp++;
    LastArgv = envp[-1] + strlen(envp[-1]);

    while ((c = getopt(argc, argv, ":aAvdlLiot:T:u:")) != -1) {
        switch (c) {

        case 'a':
            use_accessfile = 1;
            break;

        case 'A':
            use_accessfile = 0;
            break;

        case 'v':
            debug = 1;
            break;

        case 'd':
            debug = 1;
            break;

        case 'l':
            logging = 1;
            break;

        case 'L':
            log_commands = 1;
            break;

        case 'i':
            log_incoming_xfers = 1;
            break;

        case 'o':
            log_outbound_xfers = 1;
            break;

        case 't':
            timeout = atoi(optarg);
            if (maxtimeout < timeout)
                maxtimeout = timeout;
            break;

        case 'T':
            maxtimeout = atoi(optarg);
            if (timeout > maxtimeout)
                timeout = maxtimeout;
            break;

        case 'u':
            {
                unsigned int val = 0;

                while (*optarg && *optarg >= '0' && *optarg <= '9')
                    val = val * 8 + *optarg++ - '0';
                if (*optarg || val > 0777)
                    syslog(LOG_ERR, "bad value for -u");
                else
                    defumask = val;
                break;
            }

        case ':':
            syslog(LOG_ERR, "option -%c requires an argument", optopt);
            break;

        default:
            syslog(LOG_ERR, "unknown option -%c ignored", optopt);
            break;
        }
    }
    (void) freopen(_PATH_DEVNULL, "w", stderr);

    /* Checking for random signals ... */
#ifdef NEED_SIGFIX
sigemptyset(&block_sigmask);
#endif
#ifndef SIG_DEBUG
#ifdef SIGHUP
    (void) signal(SIGHUP, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGHUP);
#endif
#endif
#ifdef SIGINT
    (void) signal(SIGINT, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGINT);
#endif
#endif
#ifdef SIGQUIT
    (void) signal(SIGQUIT, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGQUIT);
#endif
#endif
#ifdef SIGILL
    (void) signal(SIGILL, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGILL);
#endif
#endif
#ifdef SIGTRAP
    (void) signal(SIGTRAP, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGTRAP);
#endif
#endif
#ifdef SIGIOT
    (void) signal(SIGIOT, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGIOT);
#endif
#endif
#ifdef SIGEMT
    (void) signal(SIGEMT, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGEMT);
#endif
#endif
#ifdef SIGFPE
    (void) signal(SIGFPE, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGFPE);
#endif
#endif
#ifdef SIGKILL
    (void) signal(SIGKILL, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGKILL);
#endif
#endif
#ifdef SIGBUS
    (void) signal(SIGBUS, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGBUS);
#endif
#endif
#ifdef SIGSEGV
    (void) signal(SIGSEGV, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGSEGV);
#endif
#endif
#ifdef SIGSYS
    (void) signal(SIGSYS, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGSYS);
#endif
#endif
#ifdef SIGALRM
    (void) signal(SIGALRM, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGALRM);
#endif
#endif
#ifdef SIGSTOP
    (void) signal(SIGSTOP, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGSTOP);
#endif
#endif
#ifdef SIGTSTP
    (void) signal(SIGTSTP, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGTSTP);
#endif
#endif
#ifdef SIGTTIN
    (void) signal(SIGTTIN, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGTTIN);
#endif
#endif
#ifdef SIGTTOU
    (void) signal(SIGTTOU, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGTTOU);
#endif
#endif
#ifdef SIGIO
    (void) signal(SIGIO, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGIO);
#endif
#endif
#ifdef SIGXCPU
    (void) signal(SIGXCPU, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGXCPU);
#endif
#endif
#ifdef SIGXFSZ
    (void) signal(SIGXFSZ, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGXFSZ);
#endif
#endif
#ifdef SIGWINCH
    (void) signal(SIGWINCH, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGWINCH);
#endif
#endif
#ifdef SIGVTALRM
    (void) signal(SIGVTALRM, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGVTALRM);
#endif
#endif
#ifdef SIGPROF
    (void) signal(SIGPROF, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGPROF);
#endif
#endif
#ifdef SIGUSR1
    (void) signal(SIGUSR1, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGUSR1);
#endif
#endif
#ifdef SIGUSR2
    (void) signal(SIGUSR2, randomsig);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGUSR2);
#endif
#endif

#ifdef SIGPIPE
    (void) signal(SIGPIPE, lostconn);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGPIPE);
#endif
#endif
#ifdef SIGCHLD
    (void) signal(SIGCHLD, SIG_IGN);
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGCHLD);
#endif
#endif

#ifdef SIGURG
    if ((int) signal(SIGURG, myoob) < 0)
        syslog(LOG_ERR, "signal: %m");
#ifdef NEED_SIGFIX
    sigaddset(&block_sigmask, SIGURG);
#endif
#endif
#endif /* SIG_DEBUG */
    /* Try to handle urgent data inline */
#ifdef SO_OOBINLINE
    if (setsockopt(0, SOL_SOCKET, SO_OOBINLINE, (char *)&on, sizeof(int)) < 0)
        syslog(LOG_ERR, "setsockopt (SO_OOBINLINE): %m");
#endif

#ifdef  F_SETOWN
    if (fcntl(fileno(stdin), F_SETOWN, getpid()) == -1)
        syslog(LOG_ERR, "fcntl F_SETOWN: %m");
#elif defined(SIOCSPGRP)
    {
        int pid;
        pid = getpid();
        if (ioctl(fileno(stdin), SIOCSPGRP, &pid) == -1)
            syslog(LOG_ERR, "ioctl SIOCSPGRP: %m");
    }
#endif
    dolog(&his_addr);
    /* Set up default state */
    data = -1;
    type = TYPE_A;
    form = FORM_N;
    stru = STRU_F;
    mode = MODE_S;
    tmpline[0] = '\0';
    yyerrorcalled = 0;

#ifdef HAVE_SYSINFO
    sysinfo(SI_HOSTNAME, hostname, sizeof (hostname));
#else
    (void) gethostname(hostname, sizeof (hostname));
#endif
/* set the FQDN here */
    shp = gethostbyname(hostname);
    if (shp != NULL)
      (void) strncpy(hostname, shp->h_name, sizeof(hostname));

    access_init();
    authenticate();
    conv_init();

#ifdef VIRTUAL
    virtual_len = sizeof(virtual_addr);
    if (getsockname(0, (struct sockaddr *) &virtual_addr, &virtual_len) == 0) {
        virtual_ptr = (struct sockaddr_in *) &virtual_addr;
        entry = (struct aclmember *) NULL;
        while (getaclentry("virtual", &entry)) {
            if (!ARG0 || !ARG1 || !ARG2)
                continue;
            if (!strcmp(ARG0, inet_ntoa(virtual_ptr->sin_addr))) {
                if(!strcmp(ARG1, "root")) {
		    syslog(LOG_NOTICE, "VirtualFTP Connect to: %s",
                           inet_ntoa(virtual_ptr->sin_addr));
                    virtual_mode = 1;
                    strncpy(virtual_root, ARG2, MAXPATHLEN);
		    /* reset hostname to this virtual name */
		    shp = gethostbyaddr((char *) &virtual_ptr->sin_addr,
                                sizeof (struct in_addr), AF_INET);
		    if (shp != NULL)
		      (void) strncpy(hostname, shp->h_name, sizeof(hostname));
                }
                if(!strcmp(ARG1, "banner")) 
                    strncpy(virtual_banner, ARG2, MAXPATHLEN);
                if(!strcmp(ARG1, "logfile")) 
                    strncpy(logfile, ARG2, MAXPATHLEN);
            }
        }
    }
    if (!virtual_mode || logfile[0] == '\0')
#endif
    strcpy(logfile, _PATH_XFERLOG);

    if (is_shutdown(1, 1) != 0) {
        syslog(LOG_INFO, "connection refused (server shut down) from %s [%s]",
               remotehost, remoteaddr);
        reply(500, MSGSTR(MSG_SERVER_SHUTDOWN,
              "%s FTP server shut down -- please try again later."),
              hostname);
        exit(0);
    }

    show_banner(220);

    entry = (struct aclmember *) NULL;
    if (getaclentry("lslong", &entry) && ARG0 && (int)strlen(ARG0) > 0) {
          strcpy(ls_long,ARG0);
      if (ARG1 && strlen(ARG1)) {
             strcat(ls_long," ");
         strcat(ls_long,ARG1);
          }
    } else {
#if defined(SVR4) || defined(ISC)
#ifndef AIX
          strcpy(ls_long,"/bin/ls -la");
#else
          strcpy(ls_long,"/bin/ls -lA");
#endif
#else
          strcpy(ls_long,"/bin/ls -lgA");
#endif
    }
    strcat(ls_long," %s");

    entry = (struct aclmember *) NULL;
    if (getaclentry("lsshort", &entry) && ARG0 && (int)strlen(ARG0) > 0) {
          strcpy(ls_short,ARG0);
      if (ARG1 && strlen(ARG1)) {
             strcat(ls_short," ");
             strcat(ls_short,ARG1);
      }
    } else {
#if defined(SVR4) || defined(ISC)
#ifndef AIX
          strcpy(ls_short,"/bin/ls -la");
#else
          strcpy(ls_short,"/bin/ls -lA");
#endif
#else
          strcpy(ls_short,"/bin/ls -lgA");
#endif
    }
    strcat(ls_short," %s");

    reply(220, MSGSTR(MSG_SERVER_READY,
          "%s FTP server (%s) ready."), hostname, version);
    (void) setjmp(errcatch);

    for (;;)
        (void) yyparse();
    /* NOTREACHED */
}

SIGNAL_TYPE
#ifdef __STDC__
randomsig(int sig)
#else
randomsig(sig)
int sig;
#endif
{
#ifdef HAVE_SIGLIST
    syslog(LOG_ERR, "exiting on signal %d: %s", sig, sys_siglist[sig] );
#else
    syslog(LOG_ERR, "exiting on signal %d", sig);
#endif
    chdir("/");
    signal(SIGIOT, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    exit (1);
    /* dologout(-1); *//* NOTREACHED */
}

SIGNAL_TYPE
#ifdef __STDC__
lostconn(int sig)
#else
lostconn(sig)
int sig;
#endif
{
    if (debug)
        syslog(LOG_DEBUG, "lost connection to %s [%s]", remotehost, remoteaddr);
    dologout(-1);
}

static char ttyline[20];

/* Helper function for sgetpwnam(). */
char *
#ifdef __STDC__
sgetsave(char *s)
#else
sgetsave(s)
char *s;
#endif
{
    char *new;
    
    new = (char *) malloc(strlen(s) + 1);

    if (new == NULL) {
        perror_reply(421, MSGSTR(MSG_RESOURCE_FAILURE,
                     "Local resource failure: malloc"));
        dologout(1);
        /* NOTREACHED */
    }
    (void) strcpy(new, s);
    return (new);
}

#if defined(SHADOW_PASSWORD) && defined(UXW)
#include <ia.h>
#endif /* SHADOW_PASSWORD && UXW */

/* Save the result of a getpwnam.  Used for USER command, since the data
 * returned must not be clobbered by any other command (e.g., globbing). */
struct passwd *
#ifdef __STDC__
sgetpwnam(char *name)
#else
sgetpwnam(name)
char *name;
#endif
{
    static struct passwd save;
    register struct passwd *p;
#ifdef M_UNIX
    struct passwd *ret = (struct passwd *) NULL;
#endif
#ifdef __STDC__
    char *sgetsave(char *s);
#else
    char *sgetsave();
#endif
#ifdef KERBEROS
    register struct authorization *q;
#endif /* KERBEROS */

#ifdef SecureWare
    struct pr_passwd *pr;
#endif

#ifdef KERBEROS
    init_krb();
    q = getauthuid(p->pw_uid);
    end_krb();
#endif /* KERBEROS */

#ifdef M_UNIX
# ifdef SecureWare
    if ((pr = getprpwnam(name)) == NULL)
        goto DONE;
# endif /* SecureWare */
    if ((p = getpwnam(name)) == NULL)
        goto DONE;
#else   /* M_UNIX */
# ifdef SecureWare
    if ((pr = getprpwnam(name)) == NULL)
        return((struct passwd *) pr);
# endif /* SecureWare */
    if ((p = getpwnam(name)) == NULL)
        return (p);
#endif  /* M_UNIX */

    if (save.pw_name)   free(save.pw_name);
    if (save.pw_gecos)  free(save.pw_gecos);
    if (save.pw_dir)    free(save.pw_dir);
    if (save.pw_shell)  free(save.pw_shell);
    if (save.pw_passwd) free(save.pw_passwd);

    save = *p;

    save.pw_name = sgetsave(p->pw_name);

#ifdef KERBEROS
    save.pw_passwd = sgetsave(q->a_password);
#elif defined(SecureWare)
    if (pr->uflg.fg_encrypt && pr->ufld.fd_encrypt && *pr->ufld.fd_encrypt)
       save.pw_passwd = sgetsave(pr->ufld.fd_encrypt);
    else
       save.pw_passwd = sgetsave("");
#else
    save.pw_passwd = sgetsave(p->pw_passwd);
#endif
#ifdef SHADOW_PASSWORD
#ifdef UXW
	if (p && strcmp(name, "ftp")) {
		struct spwd *spw;
		setspent();
		if ((spw = getspnam(p->pw_name)) != NULL) {
			free(save.pw_passwd);
			if (login_allowed(p->pw_uid, spw)) {
				uinfo_t uinfo;
				char *ia_passwd;

				if (ia_openinfo(p->pw_name, &uinfo) == 0) {
					ia_get_logpwd(uinfo, &ia_passwd);
					save.pw_passwd = sgetsave(ia_passwd);
					ia_closeinfo(uinfo);
				}
				else
					save.pw_passwd = sgetsave(spw->sp_pwdp);
			}
			else
				save.pw_passwd = sgetsave("");
		}
#else  /* UXW */
        if (p) {
           struct spwd *spw;
	   setspent();
           if ((spw = getspnam(p->pw_name)) != NULL) {
               int expired = 0;
	       /*XXX Does this work on all Shadow Password Implementations? */
	       /* it is supposed to work on Solaris 2.x*/
               time_t now;
               long today;
               
               now = time((time_t*) 0);
               today = now / (60*60*24);
               
               if ((spw->sp_expire > 0) && (spw->sp_expire < today)) expired++;
               if ((spw->sp_max > 0) && (spw->sp_lstchg > 0) &&
		   (spw->sp_lstchg + spw->sp_max < today)) expired++;
	       free(save.pw_passwd);
               save.pw_passwd = sgetsave(expired?"":spw->sp_pwdp);
           }
#endif /* UXW */
/* Don't overwrite the password if the shadow read fails, getpwnam() is NIS
   aware but getspnam() is not. */
/* Shadow passwords are optional on Linux.  --marekm */
#if !defined(LINUX) && !defined(UNIXWARE)
           else{
	     free(save.pw_passwd);
	     save.pw_passwd = sgetsave("");
	   }
#endif
/* marekm's fix for linux proc file system shadow passwd exposure problem */
	   endspent();		
        }
#endif
    save.pw_gecos = sgetsave(p->pw_gecos);
    save.pw_dir = sgetsave(p->pw_dir);
    save.pw_shell = sgetsave(p->pw_shell);
#ifdef M_UNIX
    ret = &save;
DONE:
    endpwent();
#endif
#ifdef SecureWare
    endprpwent();
#endif
#ifdef M_UNIX
    return(ret);
#else
    return(&save);
#endif
}
#ifdef SKEY
/*
 * From Wietse Venema, Eindhoven University of Technology. 
 */
/* skey_challenge - additional password prompt stuff */
#ifdef __STDC__
char   *skey_challenge(char *name, struct passwd *pwd, int pwok)
#else
char   *skey_challenge(name, pwd, pwok)
char   *name;
struct passwd *pwd;
int    pwok;
#endif
{
    static char buf[128];
    char sbuf[40];
    struct skey skey;

    /* Display s/key challenge where appropriate. */

    if (pwd == NULL || skeychallenge(&skey, pwd->pw_name, sbuf))
	sprintf(buf, "Password required for %s.", name);
    else
	sprintf(buf, "%s %s for %s.", sbuf,
		pwok ? "allowed" : "required", name);
    return (buf);
}
#endif
int login_attempts;             /* number of failed login attempts */
int askpasswd;                  /* had user command, ask for passwd */

/* USER command. Sets global passwd pointer pw if named account exists and is
 * acceptable; sets askpasswd if a PASS command is expected.  If logged in
 * previously, need to reset state.  If name is "ftp" or "anonymous", the
 * name is not in _PATH_FTPUSERS, and ftp account exists, set anonymous and
 * pw, then just return.  If account doesn't exist, ask for passwd anyway.
 * Otherwise, check user requesting login privileges.  Disallow anyone who
 * does not have a standard shell as returned by getusershell().  Disallow
 * anyone mentioned in the file _PATH_FTPUSERS to allow people such as root
 * and uucp to be avoided. */

void
#ifdef __STDC__
user(char *name)
#else
user(name)
char *name;
#endif
{
    register char *cp;
    char *shell;
    char *getusershell();

/* H* fix: if we're logged in at all, we can't log in again. */
    if (logged_in) {
	reply(530, MSGSTR(MSG_LOGGED_IN, "Already logged in."));
	return;
    }

#ifdef HOST_ACCESS                     /* 19-Mar-93    BM              */
    if (!rhost_ok(name, remotehost, remoteaddr))
    {
            reply(530, MSGSTR(MSG_USER_ACCESS_DENIED,
                  "User %s access denied."), name);
            syslog(LOG_NOTICE,
                    "FTP LOGIN REFUSED (name in %s) FROM %s [%s], %s",
                     _PATH_FTPHOSTS, remotehost, remoteaddr, name);
            return;
    }
#endif

#ifdef LOG_FAILED                       /* 06-Nov-92    EHK             */
    strncpy(the_user, name, MAXUSERNAMELEN - 1);
#endif

    if (logged_in) {			/* Now a no-op.  _H*/
        if (anonymous || guest) {
            reply(530, MSGSTR(MSG_CANT_CHANGE_USER,
                  "Can't change user from guest login."));
            return;
        }
        end_login();
    }

    guest = 0;
    anonymous = 0;
    acl_remove();

    if (!strcasecmp(name, "ftp") || !strcasecmp(name, "anonymous")) {
      struct aclmember *entry = NULL;
      int machineok=1;
      char guestservername[MAXHOSTNAMELEN];
      guestservername[0]='\0';

      if (checkuser("ftp") || checkuser("anonymous")) {
          reply(530, MSGSTR(MSG_USER_ACCESS_DENIED,
                "User %s access denied."), name);
          syslog(LOG_NOTICE,
	       "FTP LOGIN REFUSED (ftp in %s) FROM %s [%s], %s",
	       _PATH_FTPUSERS, remotehost, remoteaddr, name);
          return;
          
        /*
        ** Algorithm used:
        ** - if no "guestserver" directive is present,
        **     anonymous access is allowed, for backward compatibility.
        ** - if a "guestserver" directive is present,
        **     anonymous access is restricted to the machines listed,
        **     usually the machine whose CNAME on the current domain
        **     is "ftp"...
        **
        ** the format of the "guestserver" line is
        ** guestserver [<machine1> [<machineN>]]
        ** that is, "guestserver" will forbid anonymous access on all machines
        ** while "guestserver ftp inf" will allow anonymous access on
        ** the two machines whose CNAMES are "ftp.enst.fr" and "inf.enst.fr".
        **
        ** if anonymous access is denied on the current machine,
        ** the user will be asked to use the first machine listed (if any)
        ** on the "guestserver" line instead:
        ** 530- Guest login not allowed on this machine,
        **      connect to ftp.enst.fr instead.
        **
        ** -- <Nicolas.Pioch@enst.fr>
        */
      } else if (getaclentry("guestserver", &entry)
                 && ARG0 && (int)strlen(ARG0) > 0) {
        struct hostent *tmphostent;

        /*
        ** if a "guestserver" line is present,
        ** default is not to allow guest logins
        */
        machineok=0;

        if (hostname[0]
            && ((tmphostent=gethostbyname(hostname)))) {

          /*
          ** hostname is the only first part of the FQDN
          ** this may or may not correspond to the h_name value
          ** (machines with more than one IP#, CNAMEs...)
          ** -> need to fix that, calling gethostbyname on hostname
          **
          ** WARNING!
          ** for SunOS 4.x, you need to have a working resolver in the libc
          ** for CNAMES to work properly.
          ** If you don't, add "-lresolv" to the libraries before compiling!
          */
          char dns_localhost[MAXHOSTNAMELEN];
          int machinecount;

          strncpy(dns_localhost,
                  tmphostent->h_name,
                  sizeof(dns_localhost));
          dns_localhost[sizeof(dns_localhost)-1]='\0';

          for (machinecount=0;
               entry->arg[machinecount] && (entry->arg[machinecount])[0];
               machinecount++) {

            if ((tmphostent=gethostbyname(entry->arg[machinecount]))) {
              /*
              ** remember the name of the first machine for redirection
              */

              if ((!machinecount) && tmphostent->h_name) {
                strncpy(guestservername, entry->arg[machinecount],
                        sizeof(guestservername));
                guestservername[sizeof(guestservername)-1]='\0';
              }

              if (!strcasecmp(tmphostent->h_name, dns_localhost)) {
                machineok++;
                break;
              }
            }
          }
        }
      }
      if (!machineok) {
        if (guestservername[0])
          reply(530, MSGSTR(MSG_GUEST_CONNECT_INSTEAD,
             "Guest login not allowed on this machine, connect to %s instead."),
                guestservername);
        else
          reply(530, MSGSTR(MSG_GUEST_NOT_ALLOWED,
                "Guest login not allowed on this machine."));
        syslog(LOG_NOTICE,
               "FTP LOGIN REFUSED (localhost not in guestservers) FROM %s [%s], %s",
               remotehost, remoteaddr, name);
        /* End of the big patch -- Nap */

        } else if ((pw = sgetpwnam("ftp")) != NULL) {
            anonymous = 1;      /* for the access_ok call */
            if (access_ok(530) < 1) {
                reply(530, MSGSTR(MSG_USER_ACCESS_DENIED,
                      "User %s access denied."), name);
                syslog(LOG_NOTICE,
                       "FTP LOGIN REFUSED (access denied) FROM %s [%s], %s",
                       remotehost, remoteaddr, name);
                dologout(0);
            } else {
                askpasswd = 1;
/* H* fix: obey use_accessfile a little better.  This way, things set on the
   command line [like xferlog stuff] don't get stupidly overridden.
   XXX: all these checks maybe should be in acl.c and access.c */
		if (use_accessfile)
                    acl_setfunctions();
                reply(331, MSGSTR(MSG_GUEST_OK_EMAIL,
	"Guest login ok, send your complete e-mail address as password."));
            }
        } else {
            reply(530, MSGSTR(MSG_USER_UNKNOWN, "User %s unknown."), name);
            syslog(LOG_NOTICE,
              "FTP LOGIN REFUSED (ftp not in /etc/passwd) FROM %s [%s], %s",
                   remotehost, remoteaddr, name);
        }
        return;
    }
#ifdef ANON_ONLY
/* H* fix: define the above to completely DISABLE logins by real users,
   despite ftpusers, shells, or any of that rot.  You can always hang your
   "real" server off some other port, and access-control it. */

    else {  /* "ftp" or "anon" -- MARK your conditionals, okay?! */
      reply(530, MSGSTR(MSG_USER_UNKNOWN, "User %s unknown."), name);
      syslog (LOG_NOTICE,
	"FTP LOGIN REFUSED (not anonymous) FROM %s [%s], %s",
	  remotehost, remoteaddr, name);
      return;
    }
/* fall here if username okay in any case */
#endif /* ANON_ONLY */

    if ((pw = sgetpwnam(name)) != NULL) {
        if ((shell = pw->pw_shell) == NULL || *shell == 0)
            shell = _PATH_BSHELL;
        while ((cp = getusershell()) != NULL)
            if (strcmp(cp, shell) == 0)
                break;
        endusershell();
        if (cp == NULL || checkuser(name)) {
            reply(530, MSGSTR(MSG_USER_ACCESS_DENIED,
                  "User %s access denied."), name);
/*            if (logging)	-- inconsistent, removed.  _H*/
                syslog(LOG_NOTICE,
                       "FTP LOGIN REFUSED (bad shell or username in %s) FROM %s [%s], %s",
                       _PATH_FTPUSERS, remotehost, remoteaddr, name);
            pw = (struct passwd *) NULL;
            return;
        }
        /* if user is a member of any of the guestgroups, cause a chroot() */
        /* after they log in successfully                                  */
	if (use_accessfile)		/* see above.  _H*/
            guest = acl_guestgroup(pw);
    }
    if (access_ok(530) < 1) {
        reply(530, MSGSTR(MSG_USER_ACCESS_DENIED,
              "User %s access denied."), name);
        syslog(LOG_NOTICE, "FTP LOGIN REFUSED (access denied) FROM %s [%s], %s",
               remotehost, remoteaddr, name);
        return;
    } else
	if (use_accessfile)		/* see above.  _H*/
            acl_setfunctions();

#ifdef SKEY
#ifdef SKEY_NAME
    /* this is the old way, but freebsd uses it */
    pwok = skeyaccess(name, NULL, remotehost, remoteaddr);
#else
    /* this is the new way */
    pwok = skeyaccess(pw, NULL, remotehost, remoteaddr);
#endif
    reply(331, "%s", skey_challenge(name, pw, pwok));
#else
    reply(331, MSGSTR(MSG_PASSWD_REQUIRED, "Password required for %s."), name);
#endif
    askpasswd = 1;
    /* Delay before reading passwd after first failed attempt to slow down
     * passwd-guessing programs. */
    if (login_attempts)
        sleep((unsigned) login_attempts);
    return;
}

/* Check if a user is in the file _PATH_FTPUSERS */

int
#ifdef __STDC__
checkuser(char *name)
#else
checkuser(name)
char *name;
#endif
{
    register FILE *fd;
    register char *p;
    char line[BUFSIZ];

    if ((fd = fopen(_PATH_FTPUSERS, "r")) != NULL) {
        while (fgets(line, sizeof(line), fd) != NULL)
            if ((p = strchr(line, '\n')) != NULL) {
                *p = '\0';
                if (line[0] == '#')
                    continue;
                if (strcmp(line, name) == 0) {
                    (void) fclose(fd);
                    return (1);
                }
            }
        (void) fclose(fd);
    }
    return (0);
}

/* Terminate login as previous user, if any, resetting state; used when USER
 * command is given or login fails. */

void
#ifdef __STDC__
end_login(void)
#else
end_login()
#endif
{

    delay_signaling(); /* we can't allow any signals while euid==0: kinch */
    (void) seteuid((uid_t) 0);
    if (logged_in)
        logwtmp(ttyline, "", "");
    pw = NULL;
    logged_in = 0;
    anonymous = 0;
    guest = 0;
}

int
#ifdef __STDC__
validate_eaddr(char *eaddr)
#else
validate_eaddr(eaddr)
char *eaddr;
#endif
{
    int i,
      host,
      state;

    for (i = host = state = 0; eaddr[i] != '\0'; i++) {
        switch (eaddr[i]) {
        case '.':
            if (!host)
                return 0;
            if (state == 2)
                state = 3;
            host = 0;
            break;
        case '@':
            if (!host || state > 1 || !strncasecmp("ftp", eaddr + i - host, host))
                return 0;
            state = 2;
            host = 0;
            break;
        case '!':
        case '%':
            if (!host || state > 1)
                return 0;
            state = 1;
            host = 0;
            break;
        case '-':
            break;
        default:
            host++;
        }
    }
    if (((state == 3) && host > 1) || ((state == 2) && !host) ||
        ((state == 1) && host > 1))
        return 1;
    else
        return 0;
}

void
#ifdef __STDC__
pass(char *passwd)
#else
pass(passwd)
char *passwd;
#endif
{
    char *xpasswd,
     *salt;
    int passwarn = 0;
    int rval = 1;

#ifdef ULTRIX_AUTH
    int numfails;
#endif /* ULTRIX_AUTH */
    if (logged_in || askpasswd == 0) {
        reply(503, MSGSTR(MSG_LOGIN_USER_FIRST, "Login with USER first."));
        return;
    }
    askpasswd = 0;

    /* Disable lreply() if the first character of the password is '-' since
     * some hosts don't understand continuation messages and hang... */

    if (*passwd == '-')
        dolreplies = 0;
    else
        dolreplies = 1;
/* ******** REGULAR/GUEST USER PASSWORD PROCESSING ********** */
    if (!anonymous) {    /* "ftp" is only account allowed no password */
        if (*passwd == '-')
            passwd++;
        *guestpw = '\0';
	if (pw == NULL) 
	  salt = "xx";
	else
	  salt = pw->pw_passwd;
#ifdef SECUREOSF
	  xpasswd = bigcrypt(passwd, salt);
#else
#ifdef KERBEROS
	  xpasswd = crypt16(passwd, salt);
#else
#ifdef SKEY
	  xpasswd = skey_crypt(passwd, salt, pw, pwok);
	  pwok = 0;
#else
	  xpasswd = crypt(passwd, salt);
#endif
#endif
#endif
#ifdef ULTRIX_AUTH
        if ((numfails = ultrix_check_pass(passwd, xpasswd)) >= 0) {
#else
        /* The strcmp does not catch null passwords! */
      if (pw !=NULL && *pw->pw_passwd != '\0' &&
#ifdef HAS_PW_EXPIRE
	  (pw->pw_expire && time(NULL) < pw->pw_expire) &&
#endif
          strcmp(xpasswd, pw->pw_passwd) == 0) {
#endif
	    rval = 0;
           } 
        if(rval){
	  reply(530, MSGSTR(MSG_LOGIN_INCORRECT, "Login incorrect."));

#ifdef LOG_FAILED                       /* 27-Apr-93    EHK/BM             */
/* H* add-on: yell about attempts to use the trojan.  This may alarm you
   if you're "stringsing" the binary and you see "NULL" pop out in just
   about the same place as it would have in 2.2c! */
	    if (! strcmp (passwd, "NULL"))
		syslog(LOG_NOTICE, "REFUSED \"NULL\" from %s [%s], %s",
			remotehost, remoteaddr, the_user);
	    else
            syslog(LOG_INFO, "failed login from %s [%s], %s",
                              remotehost, remoteaddr, the_user);
#endif
            acl_remove();

            pw = NULL;
            if (++login_attempts >= lgi_failure_threshold) {
                syslog(LOG_NOTICE, "repeated login failures from %s [%s]",
                       remotehost, remoteaddr);
                exit(0);
            }
            return;
        }
/* ANONYMOUS USER PROCESSING STARTS HERE */
   } else { 
        char *pwin,
         *pwout = guestpw;
        struct aclmember *entry = NULL;
        int valid;

        if (getaclentry("passwd-check", &entry) &&
            ARG0 && strcasecmp(ARG0, "none")) {

            if (!strcasecmp(ARG0, "rfc822"))
                valid = validate_eaddr(passwd);
            else if (!strcasecmp(ARG0, "trivial"))
                valid = (strchr(passwd, '@') == NULL) ? 0 : 1;
            else
                valid = 1;

            if (!valid && ARG1 && !strcasecmp(ARG1, "enforce")) {
                lreply(530, MSGSTR(MSG_RESPONSE_NOT_VALID,
                       "The response '%s' is not valid"), passwd);
                lreply(530, MSGSTR(MSG_EMAIL_PASSWD,
                       "Please use your e-mail address as your password"));
                lreply(530, MSGSTR(MSG_FOR_EXAMPLE_OR,
                       "   for example: %s@%s or %s@"),
                       authenticated ? authuser : "joe", remotehost,
                       authenticated ? authuser : "joe");
                lreply(530, MSGSTR(MSG_WILL_BE_ADDED,
                       "[%s will be added if password ends with @]"),
                       remotehost);
                reply(530, MSGSTR(MSG_LOGIN_INCORRECT, "Login incorrect."));
		acl_remove();	
                if (++login_attempts >= lgi_failure_threshold) {
                    syslog(LOG_NOTICE, "repeated login failures from %s [%s]",
                           remotehost, remoteaddr);
                    exit(0);
                }
                return;
            } else if (!valid)
                passwarn = 1;
        }
        if (!*passwd) {
            strcpy(guestpw, "[none_given]");
        } else {
            int cnt = sizeof(guestpw) - 2;

            for (pwin = passwd; *pwin && cnt--; pwin++)
                if (!isgraph(*pwin))
                    *pwout++ = '_';
                else
                    *pwout++ = *pwin;
        }
    }

    /* if logging is enabled, open logfile before chroot or set group ID */
    if (log_outbound_xfers || log_incoming_xfers) {
        xferlog = open(logfile, O_WRONLY | O_APPEND | O_CREAT, 0660);
        if (xferlog < 0) {
            syslog(LOG_ERR, "cannot open logfile %s: %s", logfile,
                   strerror(errno));
            xferlog = 0;
        }
    }

#ifdef DEBUG
/* I had a lot of trouble getting xferlog working, because of two factors:
   acl_setfunctions making stupid assumptions, and sprintf LOSING.  _H*/
/* 
 * Actually, sprintf was not losing, but the rules changed... next release
 * this will be fixed the correct way, but right now, it works well enough
 * -- sob 
 */
      syslog (LOG_INFO, "-i %d,-o %d,xferlog %s: %d", 
	log_incoming_xfers, log_outbound_xfers, logfile, xferlog);
#endif
    enable_signaling(); /* we can allow signals once again: kinch */
    /* if autogroup command applies to user's class change pw->pw_gid */
    if (anonymous && use_accessfile)	/* see above.  _H*/
        (void) acl_autogroup(pw);
/* END AUTHENTICATION */
    login_attempts = 0;         /* this time successful */
/* SET GROUP ID STARTS HERE */
#ifndef AIX
    (void) setegid((gid_t) pw->pw_gid);
#else
    (void) setgid((gid_t)pw->pw_gid);
#endif
     (void) initgroups(pw->pw_name, pw->pw_gid);
#ifdef DEBUG
      syslog (LOG_DEBUG, "initgroups has been called");
#endif
/* WTMP PROCESSING STARTS HERE */
    /* open wtmp before chroot */
#if (defined(BSD) && (BSD >= 199103))
    (void) sprintf(ttyline, "ftp%ld", getpid());
#else
    (void) sprintf(ttyline, "ftpd%d", getpid());
#endif
#ifdef DEBUG
      syslog (LOG_DEBUG, "about to call wtmp");
#endif
    logwtmp(ttyline, pw->pw_name, remotehost);
    logged_in = 1;

    expand_id();

    if (anonymous || guest) {
        char *sp;
        /* We MUST do a chdir() after the chroot. Otherwise the old current
         * directory will be accessible as "." outside the new root! */
#ifdef VIRTUAL
        if (virtual_mode && !guest) {
            if (pw->pw_dir)
                free(pw->pw_dir);
            pw->pw_dir = sgetsave(virtual_root);
        }
#endif 
        /* determine root and home directory */

        if ((sp = strstr(pw->pw_dir, "/./")) == NULL) {
            if (chroot(pw->pw_dir) < 0 || chdir("/") < 0) {
                reply(530, MSGSTR(MSG_CANT_SET_GUEST_PRIV,
                      "Can't set guest privileges."));
                goto bad;
            }
        } else{
	  *sp++ = '\0';
	  if (chroot(pw->pw_dir) < 0 || chdir(++sp) < 0) {
	    reply(550, MSGSTR(MSG_CANT_SET_GUEST_PRIV,
                  "Can't set guest privileges."));
	    goto bad;
	  }
        }
    }
#ifdef AIX
    {
       /* AIX 3 lossage.  Don't ask.  It's undocumented.  */
       priv_t priv;

       priv.pv_priv[0] = 0;
       priv.pv_priv[1] = 0;
/*       setgroups(NULL, NULL);*/
       if (setpriv(PRIV_SET|PRIV_INHERITED|PRIV_EFFECTIVE|PRIV_BEQUEATH,
                   &priv, sizeof(priv_t)) < 0 ||
           setuidx(ID_REAL|ID_EFFECTIVE, (uid_t)pw->pw_uid) < 0 ||
           seteuid((uid_t)pw->pw_uid) < 0) {
               reply(530, "Can't set uid (AIX3).");
               goto bad;
       }
    }
#ifdef UID_DEBUG
    lreply(230, "ruid=%d, euid=%d, suid=%d, luid=%d", getuidx(ID_REAL),
         getuidx(ID_EFFECTIVE), getuidx(ID_SAVED), getuidx(ID_LOGIN));
    lreply(230, "rgid=%d, egid=%d, sgid=%d, lgid=%d", getgidx(ID_REAL),
         getgidx(ID_EFFECTIVE), getgidx(ID_SAVED), getgidx(ID_LOGIN));
#endif
#else
#ifdef HAVE_SETREUID
    if (setreuid(-1, (uid_t) pw->pw_uid) < 0) {
#else
    if (seteuid((uid_t) pw->pw_uid) < 0) {
#endif
        reply(530, MSGSTR(MSG_CANT_SET_UID, "Can't set uid."));
        goto bad;
    }
#endif
    if (!anonymous && !guest) {
        if (chdir(pw->pw_dir) < 0) {
            if (chdir("/") < 0) {
                reply(530, MSGSTR(MSG_CANT_CHANGE_DIR,
                      "User %s: can't change directory to %s."),
                      pw->pw_name, pw->pw_dir);
                goto bad;
            } else
                lreply(230, MSGSTR(MSG_NO_DIR,
                       "No directory! Logging in with home=/"));
        }
    }

    if (passwarn) {
        lreply(230, MSGSTR(MSG_RESPONSE_NOT_VALID,
               "The response '%s' is not valid"), passwd);
        lreply(230, MSGSTR(MSG_EMAIL_PASSWD_NEXT,
               "Next time please use your e-mail address as your password"));
        lreply(230, MSGSTR(MSG_FOR_EXAMPLE, "        for example: %s@%s"),
               authenticated ? authuser : "joe", remotehost);
    }

    /* following two lines were inside the next scope... */

    show_message(230, LOG_IN);
    show_readme(230, LOG_IN);

#ifdef ULTRIX_AUTH
    if (!anonymous && numfails > 0) {
        lreply(230, MSGSTR(MSG_UNSUCCESSFUL_LOGIN,
            "There have been %d unsuccessful login attempts on your account"),
            numfails);
    }
#endif /* ULTRIX_AUTH */    

    if (anonymous) {
        (void) is_shutdown(0, 0);  /* display any shutdown messages now */

        reply(230, MSGSTR(MSG_GUEST_LOGIN_OK,
              "Guest login ok, access restrictions apply."));
        sprintf(proctitle, "%s: anonymous/%.*s", remotehost,
                    (int) (sizeof(proctitle) - sizeof(remotehost) -
                    sizeof(": anonymous/")), passwd);
        setproctitle("%s", proctitle);
        if (logging)
            syslog(LOG_INFO, "ANONYMOUS FTP LOGIN FROM %s [%s], %s",
                   remotehost, remoteaddr, passwd);
    } else {
        reply(230, MSGSTR(MSG_USER_LOGGED_IN,
              "User %s logged in.%s"), pw->pw_name, guest ?
              MSGSTR(MSG_ACCESS_RESTRICT, "  Access restrictions apply.") : "");
        sprintf(proctitle, "%s: %s", remotehost, pw->pw_name);
        setproctitle(proctitle);
        if (logging)
            syslog(LOG_INFO, "FTP LOGIN FROM %s [%s], %s",
                   remotehost, remoteaddr, pw->pw_name);
/* H* mod: if non-anonymous user, copy it to "authuser" so everyone can
   see it, since whoever he was @foreign-host is now largely irrelevant. */
	strcpy (authuser, pw->pw_name);
    } /* anonymous */
    home = pw->pw_dir;          /* home dir for globbing */
    (void) umask(defumask);
    return;
  bad:
    /* Forget all about it... */
    if (xferlog)
        close(xferlog);
    xferlog = 0;
    end_login();
    return;
}

char *
#ifdef __STDC__
opt_string(int options)
#else
opt_string(options)
int options;
#endif
{
    static char buf[100];
    char *ptr = buf;

    if ((options & O_COMPRESS) != 0)	/* debian fixes: NULL -> 0 */
        *ptr++ = 'C';
    if ((options & O_TAR) != 0)
        *ptr++ = 'T';
    if ((options & O_UNCOMPRESS) != 0)
        *ptr++ = 'U';
    if (options == 0)
        *ptr++ = '_';
    *ptr++ = '\0';
    return (buf);
}

void
#ifdef __STDC__
retrieve(char *cmd, char *name)
#else
retrieve(cmd,name)
char *cmd;
char *name;
#endif
{    FILE *fin,
     *dout;
    struct stat st,
      junk;
    int (*closefunc) () = NULL;
    int options = 0;
    time_t start_time = time(NULL);
    char *logname;
    char namebuf[MAXPATHLEN];
    char fnbuf[MAXPATHLEN];
    struct convert *cptr;
#ifdef __STDC__
    extern int checknoretrieve (char *);
#else
    extern int checknoretrieve ();
#endif
    int stat_ret;

    if (cmd == NULL && (stat_ret = stat(name, &st)) == 0)
        /* there isn't a command and the file exists */
	if (use_accessfile && checknoretrieve(name))	/* see above.  _H*/
	    return;
    logname = (char *)NULL;
    if (cmd == NULL && stat_ret != 0) { /* file does not exist */
         char *ptr;

        for (cptr = cvtptr; cptr != NULL; cptr = cptr->next) {
            if (!(mangleopts & O_COMPRESS) && (cptr->options & O_COMPRESS))
                continue;
            if (!(mangleopts & O_UNCOMPRESS) && (cptr->options & O_UNCOMPRESS))
                continue;
            if (!(mangleopts & O_TAR) && (cptr->options & O_TAR))
                continue;

            if ( (cptr->stripfix) && (cptr->postfix) ) {
                int pfxlen = strlen(cptr->postfix);
		int sfxlen = strlen(cptr->stripfix);
                int namelen = strlen(name);
                (void) strcpy(fnbuf, name);

                if (namelen <= pfxlen)
                    continue;
		if ((namelen - pfxlen + sfxlen) >= sizeof(fnbuf))
		    continue;

		if (strcmp(fnbuf + namelen - pfxlen, cptr->postfix))
		    continue;
                *(fnbuf + namelen - pfxlen) = '\0';
                (void) strcat(fnbuf, cptr->stripfix);
                if (stat(fnbuf, &st) != 0) 
                    continue;
            } else if (cptr->postfix) {
                int pfxlen = strlen(cptr->postfix);
                int namelen = strlen(name);

                if (namelen <= pfxlen)
                    continue;
                (void) strcpy(fnbuf, name);
                if (strcmp(fnbuf + namelen - pfxlen, cptr->postfix))
                    continue;
                *(fnbuf + namelen - pfxlen) = (char) NULL;
                if (stat(fnbuf, &st) != 0)
                    continue;
            } else if (cptr->stripfix) {
                (void) strcpy(fnbuf, name);
                (void) strcat(fnbuf, cptr->stripfix);
                if (stat(fnbuf, &st) != 0)
                    continue;
            } else {
                continue;
            }

            if (S_ISDIR(st.st_mode)) {
                if (!(cptr->types & T_DIR)) {
                    reply(550, MSGSTR(MSG_CANNOT_DIR,
                          "Cannot %s directories."), cptr->name);
                    return;
                }
                if (cptr->options & O_TAR) {
                    strcpy(namebuf, fnbuf);
                    strcat(namebuf, "/.notar");
                    if (stat(namebuf, &junk) == 0) {
                        reply(550, MSGSTR(MSG_SORRY_MAY_NOT_TAR,
                              "Sorry, you may not TAR that directory."));
                        return;
                    }
                }
            }
/* XXX: checknoretrieve() test is weak in that if I can't get /etc/passwd
   but I can tar /etc or /, I still win.  Be careful out there... _H*
   but you could put .notar in / and /etc and stop that ! */
	    if (use_accessfile && checknoretrieve(fnbuf)) return;

            if (S_ISREG(st.st_mode) && (cptr->types & T_REG) == 0) {
                reply(550, MSGSTR(MSG_CANNOT_PLAIN,
                      "Cannot %s plain files."), cptr->name);
                return;
            }
            if (S_ISREG(st.st_mode) != 0 && S_ISDIR(st.st_mode) != 0) {
                reply(550, MSGSTR(MSG_CANNOT_SPECIAL,
                      "Cannot %s special files."), cptr->name);
                return;
            }
            if (!(cptr->types & T_ASCII) && deny_badasciixfer(550, ""))
                return;

            logname = &fnbuf[0];
            options |= cptr->options;

            strcpy(namebuf, cptr->external_cmd);
            if ((ptr = strchr(namebuf, ' ')) != NULL)
                *ptr = '\0';
            if (stat(namebuf, &st) != 0) {
                syslog(LOG_ERR, "external command %s not found",
                       namebuf);
                reply(550, MSGSTR(MSG_NO_CONVERSION_PROG,
                "Local error: conversion program not found. Cannot %s file."),
                             cptr->name);
                return;
            }
            (void) retrieve(cptr->external_cmd, logname);

            goto dolog;  /* transfer of converted file completed */
        }
    } 

    if (cmd == NULL) { /* no command */
        fin = fopen(name, "r"), closefunc = fclose;
        st.st_size = 0;
    } else {           /* run command */
        static char line[BUFSIZ];

        (void) snprintf(line, sizeof line, cmd, name), name = line;
        fin = ftpd_popen(line, "r", 1), closefunc = ftpd_pclose;
        st.st_size = -1;
#ifdef HAVE_ST_BLKSIZE
        st.st_blksize = BUFSIZ;
#endif
    }
    if (fin == NULL) {
        if (errno != 0)
            perror_reply(550, name);
        return;
    }
    if (cmd == NULL &&
        (fstat(fileno(fin), &st) < 0 || (st.st_mode & S_IFMT) != S_IFREG)) {
        reply(550, MSGSTR(MSG_NOT_PLAIN, "%s: not a plain file."), name);
        goto done;
    }
    if (restart_point) {
        if (type == TYPE_A) {
            register int i,
              n,
              c;

            n = restart_point;
            i = 0;
            while (i++ < n) {
                if ((c = getc(fin)) == EOF) {
                    perror_reply(550, name);
                    goto done;
                }
                if (c == '\n')
                    i++;
            }
        } else if (lseek(fileno(fin), restart_point, L_SET) < 0) {
            perror_reply(550, name);
            goto done;
        }
    }
    dout = dataconn(name, st.st_size, "w");
    if (dout == NULL)
        goto done;
#ifdef UXW
#define MAX(a, b)	((a) < (b) ? (b) : (a))
#define FTPBUFSIZ	16384
    send_data(fin, dout, MAX(st.st_blksize*2, FTPBUFSIZ));
#else  /* UXW */
#define FTPBUFSIZ	BUFSIZ
#ifdef HAVE_ST_BLKSIZE
    send_data(fin, dout, st.st_blksize*2);
#else
    send_data(fin, dout, BUFSIZ);
#endif
#endif /* UXW */
    (void) fclose(dout);

  dolog:
    if (log_outbound_xfers && xferlog && (cmd == 0)) {
        char msg[MAXPATHLEN];
	char *msg2;		/* for stupid_sprintf */
        int xfertime = time(NULL) - start_time;
        time_t curtime = time(NULL);
        int loop;

        if (!xfertime)
            xfertime++;
        realpath((logname != NULL) ? logname : name, &namebuf[0]); 
        for (loop = 0; namebuf[loop]; loop++)
            if (isspace(namebuf[loop]) || iscntrl(namebuf[loop]))
                namebuf[loop] = '_';

#ifdef STUPID_SPRINTF
/* Some sprintfs can't deal with a lot of arguments, so we split this */
#if (defined(BSD) && (BSD >= 199103)) && !defined(LONGOFF_T)
        sprintf(msg, "%.24s %d %s %qd ",
#else
        sprintf(msg, "%.24s %d %s %d ",
#endif
                ctime(&curtime),
                xfertime,
                remotehost,
                byte_count
	    );
	msg2 = msg + strlen(msg);	/* sigh */
        sprintf(msg2, "%s %c %s %c %c %s ftp %d %s\n",
                namebuf,
                (type == TYPE_A) ? 'a' : 'b',
                opt_string(options),
                'o',
                anonymous ? 'a' : (guest ? 'g' : 'r'),
                anonymous ? guestpw : pw->pw_name,
                authenticated,
                authenticated ? authuser : "*"
            );
#else
#if (defined(BSD) && (BSD >= 199103)) && !defined(LONGOFF_T)
        sprintf(msg, "%.24s %d %s %qd %s %c %s %c %c %s ftp %d %s\n",
#else
        sprintf(msg, "%.24s %d %s %d %s %c %s %c %c %s ftp %d %s\n",
#endif
                ctime(&curtime),
                xfertime,
                remotehost,
                byte_count,
                namebuf,
                (type == TYPE_A) ? 'a' : 'b',
                opt_string(options),
                'o',
                anonymous ? 'a' : (guest ? 'g' : 'r'),
                anonymous ? guestpw : pw->pw_name,
                authenticated,
                authenticated ? authuser : "*"
            );
#endif /* stupid_sprintf */
        write(xferlog, msg, strlen(msg));
    }
    data = -1;
    pdata = -1;
  done:
    if (closefunc)
        (*closefunc) (fin);
}

void
#ifdef __STDC__
store(char *name, char *mode, int unique)
#else
store(name,mode,unique)
char *name;
char *mode;
int unique;
#endif
{
    FILE *fout, *din;
    struct stat st;
    int (*closefunc) ();
#ifdef __STDC__
    char *gunique(char *local);
#else
    char *gunique();
#endif
    time_t start_time = time(NULL);

    struct aclmember *entry = NULL;

    int fdout;

#ifdef OVERWRITE
    int overwrite = 1;

#endif /* OVERWRITE */

#ifdef UPLOAD
    int open_flags = (O_RDWR | O_CREAT |
		      ((mode != NULL && *mode == 'a') ? O_APPEND : O_TRUNC));

    mode_t oldmask;
    int f_mode = -1,
      dir_mode,
      match_value = -1;
    uid_t uid;
    gid_t gid;
    uid_t oldid;
    int valid = 0;

#endif /* UPLOAD */

    if (unique && stat(name, &st) == 0 &&
        (name = gunique(name)) == NULL)
        return;

    /*
     * check the filename, is it legal?
     */
    if ( (fn_check(name)) <= 0 )
        return;

#ifdef OVERWRITE
    /* if overwrite permission denied and file exists... then deny the user
     * permission to write the file. */
    while (getaclentry("overwrite", &entry) && ARG0 && ARG1 != NULL) {
        if (type_match(ARG1))
            if (strcmp(ARG0, "yes") != 0) {
                overwrite = 0;
                open_flags |= O_EXCL;
            }
    }

#ifdef PARANOID
    overwrite = 0;
#endif
    if (!overwrite && !stat(name, &st)) {
        reply(553, MSGSTR(MSG_PERM_DENIED_OVERWRITE,
              "%s: Permission denied. (Overwrite)"), name);
        return;
    }
#endif /* OVERWRITE */

#ifdef UPLOAD
    if ( (match_value = upl_check(name, &uid, &gid, &f_mode, &valid)) < 0 )
        return;

    /* do not truncate the file if we are restarting */
    if (restart_point)
        open_flags &= ~O_TRUNC;

    /* if the user has an explicit new file mode, than open the file using
     * that mode.  We must take care to not let the umask affect the file
     * mode.
     * 
     * else open the file and let the default umask determine the file mode. */
    if (f_mode >= 0) {
        oldmask = umask(0000);
        fdout = open(name, open_flags, f_mode);
        umask(oldmask);
    } else
        fdout = open(name, open_flags, 0666);

    if (fdout < 0) {
        perror_reply(553, name);
        return;
    }
    /* if we have a uid and gid, then use them. */

    if (valid > 0) {
        oldid = geteuid();
        delay_signaling(); /* we can't allow any signals while euid==0: kinch */
        (void) seteuid((uid_t) 0);
        if ((fchown(fdout, uid, gid)) < 0) {
            (void) seteuid(oldid);
            enable_signaling(); /* we can allow signals once again: kinch */
            perror_reply(550, "fchown");
            return;
        }
        (void) seteuid(oldid);
        enable_signaling(); /* we can allow signals once again: kinch */
    }
#endif /* UPLOAD */

    if (restart_point && (open_flags & O_APPEND) == 0)
        mode = "r+";

#ifdef UPLOAD
    fout = fdopen(fdout, mode);
#else
    fout = fopen(name, mode);
#endif /* UPLOAD */

    closefunc = fclose;
    if (fout == NULL) {
        perror_reply(553, name);
        return;
    }
    if (restart_point) {
        if (type == TYPE_A) {
            register int i,
              n,
              c;

            n = restart_point;
            i = 0;
            while (i++ < n) {
                if ((c = getc(fout)) == EOF) {
                    perror_reply(550, name);
                    goto done;
                }
                if (c == '\n')
                    i++;
            }
            /* We must do this seek to "current" position because we are
             * changing from reading to writing. */
            if (fseek(fout, 0L, L_INCR) < 0) {
                perror_reply(550, name);
                goto done;
            }
        } else if (lseek(fileno(fout), restart_point, L_SET) < 0) {
            perror_reply(550, name);
            goto done;
        }
    }
    din = dataconn(name, (off_t) - 1, "r");
    if (din == NULL)
        goto done;
    if (receive_data(din, fout) == 0) {
        if (unique)
            reply(226, MSGSTR(MSG_TRANSFER_COMP_UNIQ,
                  "Transfer complete (unique file name:%s)."),
                  name);
        else
            reply(226, MSGSTR(MSG_TRANSFER_COMP, "Transfer complete."));
    }
    (void) fclose(din);

  dolog:
    if (log_incoming_xfers && xferlog) {
        char namebuf[MAXPATHLEN],
          msg[MAXPATHLEN];
	char *msg2;		/* for stupid_sprintf */
        int xfertime = time(NULL) - start_time;
        time_t curtime = time(NULL);
        int loop;

        if (!xfertime)
            xfertime++;
        realpath(name, namebuf);
        for (loop = 0; namebuf[loop]; loop++)
            if (isspace(namebuf[loop]) || iscntrl(namebuf[loop]))
                namebuf[loop] = '_';

#ifdef STUPID_SPRINTF
/* see above */
#if (defined(BSD) && (BSD >= 199103)) && !defined(LONGOFF_T)
        sprintf(msg, "%.24s %d %s %qd ",
#else
        sprintf(msg, "%.24s %d %s %d ",
#endif
                ctime(&curtime),
                xfertime,
                remotehost,
                byte_count
	    );
	msg2 = msg + strlen(msg);	/* sigh */
        sprintf(msg2, "%s %c %s %c %c %s ftp %d %s\n",
                namebuf,
                (type == TYPE_A) ? 'a' : 'b',
                opt_string(0),
                'i',
                anonymous ? 'a' : (guest ? 'g' : 'r'),
                anonymous ? guestpw : pw->pw_name,
                authenticated,
                authenticated ? authuser : "*"
            );
#else
#if (defined(BSD) && (BSD >= 199103)) && !defined(LONGOFF_T)
        sprintf(msg, "%.24s %d %s %qd %s %c %s %c %c %s ftp %d %s\n",
#else
        sprintf(msg, "%.24s %d %s %d %s %c %s %c %c %s ftp %d %s\n",
#endif
                ctime(&curtime),
                xfertime,
                remotehost,
                byte_count,
                namebuf,
                (type == TYPE_A) ? 'a' : 'b',
                opt_string(0),
                'i',
                anonymous ? 'a' : (guest ? 'g' : 'r'),
                anonymous ? guestpw : pw->pw_name,
                authenticated,
                authenticated ? authuser : "*"
            );
#endif /* STUPID_SPRINTF */
        write(xferlog, msg, strlen(msg));
    }
    data = -1;
    pdata = -1;
  done:
    (*closefunc) (fout);
}

FILE *
#ifdef __STDC__
getdatasock(char *mode)
#else
getdatasock(mode)
char *mode;
#endif
{
      int s,
      on = 1,
      tries;

    if (data >= 0)
        return (fdopen(data, mode));
    delay_signaling(); /* we can't allow any signals while euid==0: kinch */
    (void) seteuid((uid_t) 0);
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        goto bad;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                   (char *) &on, sizeof(on)) < 0)
        goto bad;
    /* anchor socket to avoid multi-homing problems */
    data_source.sin_family = AF_INET;
    data_source.sin_addr = ctrl_addr.sin_addr;

#if defined(VIRTUAL) && defined(CANT_BIND) /* can't bind to virtual address */
    data_source.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
    for (tries = 1;; tries++) {
        if (bind(s, (struct sockaddr *) &data_source,
                 sizeof(data_source)) >= 0)
            break;
        if (errno != EADDRINUSE || tries > 10)
            goto bad;
        sleep(tries);
    }
#if defined(M_UNIX) && !defined(_M_UNIX)  /* bug in old TCP/IP release */
    {
        struct linger li;
        li.l_onoff = 1;
        li.l_linger = 900;
        if (setsockopt(s, SOL_SOCKET, SO_LINGER,
          (char *)&li, sizeof(struct linger)) < 0) {
            syslog(LOG_WARNING, "setsockopt (SO_LINGER): %m");
            goto bad;
        }
    }
#endif
    (void) seteuid((uid_t) pw->pw_uid);
    enable_signaling(); /* we can allow signals once again: kinch */

#ifdef IPTOS_THROUGHPUT
    on = IPTOS_THROUGHPUT;
    if (setsockopt(s, IPPROTO_IP, IP_TOS, (char *) &on, sizeof(int)) < 0)
          syslog(LOG_WARNING, "setsockopt (IP_TOS): %m");
#endif
#ifdef TCP_NOPUSH
	/*
	 * Turn off push flag to keep sender TCP from sending short packets
	 * at the boundaries of each write().  Should probably do a SO_SNDBUF
	 * to set the send buffer size as well, but that may not be desirable
	 * in heavy-load situations.
	 */
	on = 1;
	if (setsockopt(s, IPPROTO_TCP, TCP_NOPUSH, (char *)&on, sizeof on) < 0)
		syslog(LOG_WARNING, "setsockopt (TCP_NOPUSH): %m");
#endif

    return (fdopen(s, mode));
  bad:
    on = errno; /* hold errno for return */
    (void) seteuid((uid_t) pw->pw_uid);
    enable_signaling(); /* we can allow signals once again: kinch */
    if (s != -1)
        (void) close(s);
    errno = on; 
    return (NULL);
}

FILE *
#ifdef __STDC__
dataconn(char *name, off_t size, char *mode)
#else
dataconn(name,size,mode)
char *name;
off_t size; 
char *mode;
#endif
{
    char sizebuf[32];
    FILE *file;
    int retry = 0;
#ifdef IPTOS_LOWDELAY
    int tos;
#endif

    file_size = size;
    byte_count = 0;
    if (size != (off_t) - 1)
#if (defined(BSD) && (BSD >= 199103)) && !defined(LONGOFF_T)
        (void) sprintf(sizebuf, " (%qd bytes)", size);
#else
	(void) sprintf(sizebuf, MSGSTR(MSG_BYTES, " (%d bytes)"), size);
#endif
    else
        (void) strcpy(sizebuf, "");
    if (pdata >= 0) {
        struct sockaddr_in from;
#ifdef UNIXWARE 
        size_t fromlen = sizeof(from);
#else
        int fromlen = sizeof(from);
#endif
        int s;
#ifdef FD_ZERO
        struct timeval timeout;
        fd_set set;
 
        FD_ZERO(&set);
        FD_SET(pdata, &set);
 
        timeout.tv_usec = 0;
        timeout.tv_sec = 120;
#ifdef hpux
        if (select(pdata+1, (int *)&set, NULL, NULL, &timeout) == 0 ||
#else
        if (select(pdata+1, &set, (fd_set *) 0, (fd_set *) 0, &timeout) == 0 ||
#endif
            (s = accept(pdata, (struct sockaddr *) &from, &fromlen)) < 0) {
#else      
        alarm(120);
        s = accept(pdata, (struct sockaddr *) &from, &fromlen);
        alarm(0);
        if (s < 0) {
#endif
            reply(425, MSGSTR(MSG_CANT_OPEN_DATA,
                  "Can't open data connection."));
            (void) close(pdata);
            pdata = -1;
            return (NULL);
        }
        (void) close(pdata);
        pdata = s;
#ifdef IPTOS_LOWDELAY
        tos = IPTOS_LOWDELAY;
        (void) setsockopt(s, IPPROTO_IP, IP_TOS, (char *) &tos,
                          sizeof(int));

#endif
        reply(150, MSGSTR(MSG_OPENING_DATA,
              "Opening %s mode data connection for %s%s."),
              type == TYPE_A ? "ASCII" : "BINARY", name, sizebuf);
        return (fdopen(pdata, mode));
    }
    if (data >= 0) {
        reply(125, MSGSTR(MSG_USING_EXISTING,
              "Using existing data connection for %s%s."),
              name, sizebuf);
        usedefault = 1;
        return (fdopen(data, mode));
    }
    if (usedefault)
        data_dest = his_addr;
    usedefault = 1;
    file = getdatasock(mode);
    if (file == NULL) {
        reply(425, MSGSTR(MSG_CANT_CREATE_SOCK,
              "Can't create data socket (%s,%d): %s."),
              inet_ntoa(data_source.sin_addr),
              ntohs(data_source.sin_port), strerror(errno));
        return (NULL);
    }
    data = fileno(file);
    while (connect(data, (struct sockaddr *) &data_dest,
                   sizeof(data_dest)) < 0) {
        if ((errno == EADDRINUSE || errno == EINTR) && retry < swaitmax) {
            sleep((unsigned) swaitint);
            retry += swaitint;
            continue;
        }
        perror_reply(425, MSGSTR(MSG_CANT_BUILD,"Can't build data connection"));
        (void) fclose(file);
        data = -1;
        return (NULL);
    }
    reply(150, MSGSTR(MSG_OPENING_DATA,
          "Opening %s mode data connection for %s%s."),
          type == TYPE_A ? "ASCII" : "BINARY", name, sizebuf);
    return (file);
}

#ifdef SENDV
#include <sys/sendv.h>
#endif /* SENDV */

/* Tranfer the contents of "instr" to "outstr" peer using the appropriate
 * encapsulation of the data subject to Mode, Structure, and Type.
 *
 * NB: Form isn't handled. */

void
#ifdef __STDC__
send_data(FILE *instr, FILE *outstr, off_t blksize)
#else
send_data(instr,outstr,blksize)
FILE *instr;
FILE *outstr;
off_t blksize;
#endif
{
    register int c,
      cnt;
    register char *buf;
    int netfd,
      filefd;
#ifdef SENDV
    struct stat sbuf;
#endif /* SENDV */

    if (setjmp(urgcatch)) {
        transflag = 0;
        return;
    }
    transflag++;
    switch (type) {

    case TYPE_A:
        while ((c = getc(instr)) != EOF) {
            byte_count++;
            if (c == '\n') {
                if (ferror(outstr))
                    goto data_err;
                (void) putc('\r', outstr);
            }
            (void) putc(c, outstr);
        }
        fflush(outstr);
        transflag = 0;
        if (ferror(instr))
            goto file_err;
        if (ferror(outstr))
            goto data_err;
        reply(226, MSGSTR(MSG_TRANSFER_COMP, "Transfer complete."));
        return;

    case TYPE_I:
    case TYPE_L:
#ifdef SENDV
    if ((fstat(fileno(instr), &sbuf) != -1) &&
	((sbuf.st_mode & S_IFMT) == S_IFREG)) {
	/*
	 * sendv only works on regular files.
	 *
	 * The file is transferred a block at a time to allow ftpd to respond
	 * to ABOR and STAT requests.
	 *
	 * The lseek is necessary because if we are restarting a transfer we
	 * may not be at the start of the file.
	 */
	struct sendv_iovec sv[1];

	netfd = fileno(outstr);
	filefd = fileno(instr);
	sv[0].sendv_flags = SENDV_FD;
	sv[0].sendv_fd = filefd;
	sv[0].sendv_off = lseek(filefd, 0L, SEEK_CUR);
	sv[0].sendv_len = blksize;
	while ((cnt = sendv(netfd, sv, 1)) > 0) {
		sv[0].sendv_off += cnt;
		byte_count += cnt;
	}
	transflag = 0;
    } else {
#endif /* SENDV */
        if ((buf = (char *) malloc((u_int) blksize)) == NULL) {
            transflag = 0;
            perror_reply(451, MSGSTR(MSG_RESOURCE_FAILURE,
                         "Local resource failure: malloc"));
            return;
        }
        netfd = fileno(outstr);
        filefd = fileno(instr);
/* Debian fix: this seems gratuitous somehow, testing ... XXX: */
#ifdef bogus__linux__
	while ((cnt = read(filefd, buf, (u_int)blksize)) > 0)
	{
	int outcnt=0, newcnt=0;
	while ((outcnt=write(netfd, buf+newcnt, cnt-newcnt))!= cnt-newcnt)
		newcnt+=outcnt;
	byte_count += cnt;				
	}
#else
        while ((cnt = read(filefd, buf, (u_int) blksize)) > 0 &&
               write(netfd, buf, cnt) == cnt)
            byte_count += cnt;
#endif
        transflag = 0;
        (void) free(buf);
#ifdef SENDV
    }
#endif /* SENDV */
        if (cnt != 0) {
            if (cnt < 0)
                goto file_err;
            goto data_err;
        }
        reply(226, MSGSTR(MSG_TRANSFER_COMP, "Transfer complete."));
        return;
    default:
        transflag = 0;
        reply(550, MSGSTR(MSG_UNIMPL_SEND,
              "Unimplemented TYPE %d in send_data"), type);
        return;
    }

  data_err:
    transflag = 0;
    perror_reply(426, MSGSTR(MSG_DATA_CONN, "Data connection"));
    return;

  file_err:
    transflag = 0;
    perror_reply(551, MSGSTR(MSG_ERROR_INPUT, "Error on input file"));
}

/* Transfer data from peer to "outstr" using the appropriate encapulation of
 * the data subject to Mode, Structure, and Type.
 *
 * N.B.: Form isn't handled. */

int
#ifdef __STDC__
receive_data(FILE *instr, FILE *outstr)
#else
receive_data(instr,outstr)
FILE *instr;
FILE *outstr;
#endif
{
    register int c;
    int cnt,
      bare_lfs = 0;
    char *buf;
    int netfd,
      filefd;

    if (setjmp(urgcatch)) {
        transflag = 0;
        return (-1);
    }
    transflag++;
    switch (type) {

    case TYPE_I:
    case TYPE_L:
        if ((buf = (char *) malloc(FTPBUFSIZ)) == NULL) {
            transflag = 0;
            perror_reply(451, MSGSTR(MSG_RESOURCE_FAILURE,
                         "Local resource failure: malloc"));
            return (-1);
        }
        netfd = fileno(instr);
        filefd = fileno(outstr);
        while ((cnt = read(netfd, buf, FTPBUFSIZ)) > 0 &&
               write(filefd, buf, cnt) == cnt)
            byte_count += cnt;

        transflag = 0;
        (void) free(buf);
        if (cnt != 0) {
            if (cnt < 0)
                goto data_err;
            goto file_err;
        }
        return (0);

    case TYPE_E:
        reply(553, MSGSTR(MSG_TYPE_E_NOT, "TYPE E not implemented."));
        transflag = 0;
        return (-1);

    case TYPE_A:
        while ((c = getc(instr)) != EOF) {
            byte_count++;
            if (c == '\n')
                bare_lfs++;
            while (c == '\r') {
                if (ferror(outstr))
                    goto data_err;
                if ((c = getc(instr)) != '\n') {
                    (void) putc('\r', outstr);
                    if (c == EOF) /* null byte fix, noid@cyborg.larc.nasa.gov */
                        goto contin2;
                }
            }
            (void) putc(c, outstr);
          contin2:;
        }
        fflush(outstr);
        if (ferror(instr))
            goto data_err;
        if (ferror(outstr))
            goto file_err;
        transflag = 0;
        if (bare_lfs) {
            lreply(226, MSGSTR(MSG_WARNING_BARE,
                "WARNING! %d bare linefeeds received in ASCII mode"), bare_lfs);
            printf(MSGSTR(MSG_FILE_MAY_NOT,
                   "   File may not have transferred correctly.\r\n"));
        }
        return (0);
    default:
        reply(550, MSGSTR(MSG_UNIMP_RECV,
              "Unimplemented TYPE %d in receive_data"), type);
        transflag = 0;
        return (-1);
    }

  data_err:
    transflag = 0;
    perror_reply(426, MSGSTR(MSG_DATA_CONNECT, "Data Connection"));
    return (-1);

  file_err:
    transflag = 0;
    perror_reply(452, MSGSTR(MSG_ERROR_WRITING, "Error writing file"));
    return (-1);
}

void
#ifdef __STDC__
statfilecmd(char *filename)
#else
statfilecmd(filename)
char *filename;
#endif
{
    char line[BUFSIZ];
    FILE *fin;
    int c;

    if (anonymous && dolreplies)
        (void) snprintf(line, sizeof(line), ls_long, filename);
    else
        (void) snprintf(line, sizeof(line), ls_short, filename);
    fin = ftpd_popen(line, "r", 0);
    lreply(213, MSGSTR(MSG_STATUS_OF, "status of %s:"), filename);
    while ((c = getc(fin)) != EOF) {
        if (c == '\n') {
            if (ferror(stdout)) {
                perror_reply(421, MSGSTR(MSG_CONTROL_CONN,
                             "control connection"));
                (void) ftpd_pclose(fin);
                dologout(1);
                /* NOTREACHED */
            }
            if (ferror(fin)) {
                perror_reply(551, filename);
                (void) ftpd_pclose(fin);
                return;
            }
            (void) putc('\r', stdout);
        }
        (void) putc(c, stdout);
    }
    (void) ftpd_pclose(fin);
    reply(213, MSGSTR(MSG_END_OF_STATUS, "End of Status"));
}

void
#ifdef __STDC__
statcmd(void)
#else
statcmd()
#endif
{
    struct sockaddr_in *sin;
    u_char *a,
     *p;

    lreply(211, MSGSTR(MSG_SERVER_STATUS, "%s FTP server status:"), hostname);
    printf("     %s\r\n", version);
    printf(MSGSTR(MSG_CONNECTED_TO, "     Connected to %s"), remotehost);
    if (!isdigit(remotehost[0]))
        printf(" (%s)", inet_ntoa(his_addr.sin_addr));
    printf("\r\n");
    if (logged_in) {
        if (anonymous)
            printf(MSGSTR(MSG_LOGGED_IN_ANON,
                   "     Logged in anonymously\r\n"));
        else
            printf(MSGSTR(MSG_LOGGED_IN_AS,
                   "     Logged in as %s\r\n"), pw->pw_name);
    } else if (askpasswd)
        printf(MSGSTR(MSG_WAITING_PASSWD, "     Waiting for password\r\n"));
    else
        printf(MSGSTR(MSG_WAITING_USER, "     Waiting for user name\r\n"));
    printf(MSGSTR(MSG_TYPE, "     TYPE: %s"), typenames[type]);
    if (type == TYPE_A || type == TYPE_E)
        printf(MSGSTR(MSG_FORM, ", FORM: %s"), formnames[form]);
    if (type == TYPE_L)
#ifdef NBBY 
        printf(" %d", NBBY);
#else
        printf(" %d", bytesize);/* need definition! */
#endif
    printf(MSGSTR(MSG_STRUCTURE, "; STRUcture: %s; transfer MODE: %s\r\n"),
           strunames[stru], modenames[mode]);
    if (data != -1)
        printf(MSGSTR(MSG_DATA_CONN_OPEN, "     Data connection open\r\n"));
    else if (pdata != -1) {
        printf(MSGSTR(MSG_IN_PASSIVE, "     in Passive mode"));
        sin = &pasv_addr;
        goto printaddr;
    } else if (usedefault == 0) {
        printf(MSGSTR(MSG_PORT, "     PORT"));
        sin = &data_dest;
      printaddr:
        a = (u_char *) & sin->sin_addr;
        p = (u_char *) & sin->sin_port;
#define UC(b) (((int) b) & 0xff)
        printf(" (%d,%d,%d,%d,%d,%d)\r\n", UC(a[0]),
               UC(a[1]), UC(a[2]), UC(a[3]), UC(p[0]), UC(p[1]));
#undef UC
    } else
        printf(MSGSTR(MSG_NO_DATA_CONN, "     No data connection\r\n"));
    reply(211, MSGSTR(MSG_END_STATUS, "End of status"));
}

void
#ifdef __STDC__
fatal(char *s)
#else
fatal(s)
char *s;
#endif
{
    reply(451, MSGSTR(MSG_ERROR_IN_SERVER, "Error in server: %s\n"), s);
    reply(221, MSGSTR(MSG_CLOSING_CONN,
          "Closing connection due to server error."));
    dologout(0);
    /* NOTREACHED */
}

void
#if defined (HAVE_VPRINTF)
/* VARARGS2 */
#ifdef __STDC__
reply(int n, char *fmt, ...)
#else
reply(n, fmt, va_alist)
     int n;
     char * fmt;
     va_dcl
#endif
{
    VA_LOCAL_DECL
    
    VA_START(fmt); 

    if (autospout != NULL) {
        char *ptr = autospout;

        printf("%d-", n);
        while (*ptr) {
            if (*ptr == '\n') {
                fputs("\r\n", stdout);
                if (*(++ptr))
                    printf("%03d-", n);
            } else {
                putc(*ptr++,stdout);
            }
        }
        if (*(--ptr) != '\n')
            printf("\r\n");
        if (autospout_free) {
            (void) free(autospout);
            autospout_free = 0;
        }
        autospout = 0;
    }
    printf("%d ", n);
    vprintf(fmt, ap);
    printf("\r\n");
    (void) fflush(stdout);

    if (debug) {
        char buf[BUFSIZ];
        (void) vsprintf(buf, fmt, ap);

        syslog(LOG_DEBUG, "<--- %d ", n);
        syslog(LOG_DEBUG, "%s", buf);
    }

    VA_END;
}

void
/* VARARGS2 */
#ifdef __STDC__
lreply(int n, char *fmt,...)
#else
lreply(n, fmt, va_alist)
     int n;
     char * fmt;
     va_dcl
#endif
{
    VA_LOCAL_DECL

    VA_START(fmt);

    if (!dolreplies)
        return;
    printf("%d-", n);
    vprintf(fmt, ap);
    printf("\r\n");
    (void) fflush(stdout);

    if (debug) {
        char buf[BUFSIZ];
        (void) vsprintf(buf, fmt, ap);

        syslog(LOG_DEBUG, "<--- %d- ", n);
        syslog(LOG_DEBUG, "%s",buf);
    }

    VA_END;
}

#else
/* VARARGS2 */
void
reply(int n, char *fmt, int p0, int p1, int p2, int p3, int p4, int p5)
{
    if (autospout != NULL) {
        char *ptr = autospout;

        printf("%d-", n);
        while (*ptr) {
            if (*ptr == '\n') {
                printf("\r\n");
                if (*(++ptr))
                    printf("%d-", n);
            } else {
                putc(*ptr++,stdout);
            }
        }
        if (*(--ptr) != '\n')
            printf("\r\n");
        if (autospout_free) {
            (void) free(autospout);
            autospout_free = 0;
        }
        autospout = 0;
    }
    printf("%d ", n);
    printf(fmt, p0, p1, p2, p3, p4, p5);
    printf("\r\n");
    (void) fflush(stdout);
    if (debug) {
        syslog(LOG_DEBUG, "<--- %d ", n);
        syslog(LOG_DEBUG, fmt, p0, p1, p2, p3, p4, p5);
    }
}

void
/* VARARGS2 */
void
lreply(int n, char *fmt, int p0, int p1, int p2, int p3, int p4, int p5)
{
    if (!dolreplies)
        return;
    printf("%d-", n);
    printf(fmt, p0, p1, p2, p3, p4, p5);
    printf("\r\n");
    (void) fflush(stdout);
    if (debug) {
        syslog(LOG_DEBUG, "<--- %d- ", n);
        syslog(LOG_DEBUG, fmt, p0, p1, p2, p3, p4, p5);
    }
}
#endif

void
#ifdef __STDC__
ack(char *s)
#else
ack(s)
char *s;
#endif
{
    reply(250, MSGSTR(MSG_CMD_SUCCESS, "%s command successful."), s);
}

void
#ifdef __STDC__
nack(char *s)
#else
nack(s)
char *s;
#endif
{
    reply(502, MSGSTR(MSG_CMD_NOT_IMPL, "%s command not implemented."), s);
}

void
#ifdef __STDC__
yyerror(char *s)
#else
yyerror(s)
char *s;
#endif
{
    char *cp;
    if (s == NULL || yyerrorcalled != 0) return;
    if ((cp = strchr(cbuf, '\n')) != NULL)
        *cp = '\0';
    reply(500, MSGSTR(MSG_NOT_UNDERSTOOD,
          "'%s': command not understood."), cbuf);
    yyerrorcalled = 1;
    return;
}

void
#ifdef __STDC__
delete(char *name)
#else
delete(name)
char *name;
#endif
{
    struct stat st;

    /*
     * delete permission?
     */

    if ( (del_check(name)) == 0 )
        return;

    if (lstat(name, &st) < 0) {
        perror_reply(550, name);
        return;
    }
    if ((st.st_mode & S_IFMT) == S_IFDIR) {
        uid_t uid;
        gid_t gid;
        int valid;

        /*
         * check the directory, can we rmdir here?
         */
        if ( (dir_check(name, &uid, &gid, &valid)) <= 0 )
            return;

        if (rmdir(name) < 0) {
            perror_reply(550, name);
            return;
        }
        goto done;
    }
    if (unlink(name) < 0) {
        perror_reply(550, name);
        return;
    }
  done:
    {
        char path[MAXPATHLEN];

        realpath(name, path);

        if (anonymous) {
            syslog(LOG_NOTICE, "%s of %s [%s] deleted %s", guestpw, remotehost,
                   remoteaddr, path);
        } else {
            syslog(LOG_NOTICE, "%s of %s [%s] deleted %s", pw->pw_name,
                   remotehost, remoteaddr, path);
        }
    }

    ack("DELE");
}

void
#ifdef __STDC__
cwd(char *path)
#else
cwd(path)
char *path;
#endif
{
    struct aclmember *entry = NULL;
    char cdpath[MAXPATHLEN + 1];

    if (chdir(path) < 0) {
        /* alias checking */
        while (getaclentry("alias", &entry) && ARG0 && ARG1 != NULL) {
            if (!strcasecmp(ARG0, path)) {
                if (chdir(ARG1) < 0)
                    perror_reply(550, path);
                else {
                    show_message(250, C_WD);
                    show_readme(250, C_WD);
                    ack("CWD");
                }
                return;
            }
        }
    /* check for "cdpath" directories. */
    entry = (struct aclmember *) NULL;
        while (getaclentry("cdpath", &entry) && ARG0 != NULL) {
            snprintf(cdpath, sizeof cdpath, "%s/%s", ARG0, path);
            if (chdir(cdpath) >= 0) {
                show_message(250, C_WD);
                show_readme(250, C_WD);
                ack("CWD");
                return;
            }
        }
        perror_reply(550,path);
    } else {
        show_message(250, C_WD);
        show_readme(250, C_WD);
        ack("CWD");
    }
}

void
#ifdef __STDC__
makedir(char *name)
#else
makedir(name)
char *name;
#endif
{
	uid_t uid;
	gid_t gid;
	int   valid;

    /*
     * check the directory, can we mkdir here?
     */
    if ( (dir_check(name, &uid, &gid, &valid)) <= 0 )
        return;

    /*
     * check the filename, is it legal?
     */
    if ( (fn_check(name)) <= 0 )
        return;

    if (mkdir(name, 0777) < 0) {
        if (errno == EEXIST)
            perror_reply(553, name);
        else
            perror_reply(550, name);
	return;
    }
    reply(257, MSGSTR(MSG_MKD_CMD_SUCCESS, "MKD command successful."));
}

void
#ifdef __STDC__
removedir(char *name)
#else
removedir(name)
char *name;
#endif
{
    uid_t uid;
    gid_t gid;
    int valid;

    /*
     * delete permission?
     */

    if ( (del_check(name)) == 0 )
        return;
    /*
     * check the directory, can we rmdir here?
     */
    if ( (dir_check(name, &uid, &gid, &valid)) <= 0 )
        return;

    if (rmdir(name) < 0) {
        if (errno == EBUSY)
            perror_reply(450, name);
        else
            perror_reply(550, name);
    }
    else
        ack("RMD");
}

void
#ifdef __STDC__
pwd(void)
#else
pwd()
#endif
{
    char path[MAXPATHLEN + 1];
#ifdef HAVE_GETCWD
    extern char *getcwd();
#else
#ifdef __STDC__
    extern char *getwd(char *);
#else
    extern char *getwd();
#endif
#endif

#ifdef HAVE_GETCWD
    if (getcwd(path,MAXPATHLEN) == (char *) NULL)
#else
    if (getwd(path) == (char *) NULL)
#endif
/* Dink!  If you couldn't get the path and the buffer is now likely to
   be undefined, why are you trying to PRINT it?!  _H*
        reply(550, "%s.", path); */
    {
	realpath (".", path);	/* realpath_on_steroids can deal */
    }
    reply(257, MSGSTR(MSG_CURRENT_DIR, "\"%s\" is current directory."), path);
}

char *
#ifdef __STDC__
renamefrom(char *name)
#else
renamefrom(name)
char *name;
#endif
{
    struct stat st;
    struct aclmember *entry = NULL;	/* Added: fixes a bug.  _H*/

    if (lstat(name, &st) < 0) {
        perror_reply(550, name);
        return ((char *) 0);
    }

    /* if rename permission denied and file exists... then deny the user
     * permission to rename the file. 
     */
    while (getaclentry("rename", &entry) && ARG0 && ARG1 != NULL) {
        if (type_match(ARG1))
            if (strcmp(ARG0, "yes")) {
                reply(553, MSGSTR(MSG_PERM_DENIED_RENAME,
                      "%s: Permission denied. (rename)"), name);
                return ((char *) 0);
            }
    }

    reply(350, MSGSTR(MSG_FILE_EXISTS,
          "File exists, ready for destination name"));
    return (name);
}

void
#ifdef __STDC__
renamecmd(char *from, char *to)
#else
renamecmd(from,to)
char *from;
char *to;
#endif
{
    struct stat st;

    /*
     * check the filename, is it legal?
     */
    if ( (fn_check(to)) == 0 )
        return;

#ifdef PARANOID
/* Almost forgot about this.  Don't allow renaming TO existing files --
   otherwise someone can rename "trivial" to "warez", and "warez" is gone!
   XXX: This part really should do the same "overwrite" check as store(). */
    if (!stat(to, &st)) {
      reply (550, MSGSTR(MSG_PERM_DENIED_RENAME,
             "%s: Permission denied. (rename)"), to);
      return;
    }
#endif

    if (rename(from, to) < 0)
        perror_reply(550, "rename");
    else
        ack("RNTO");
}

void
#ifdef __STDC__
dolog(struct sockaddr_in *sin)
#else
dolog(sin)
struct sockaddr_in *sin;
#endif
{
    struct hostent *hp;
	char *blah;

#ifdef	DNS_TRYAGAIN
    int num_dns_tries = 0;
    /*
     * 27-Apr-93    EHK/BM
     * far away connections might take some time to get their IP address
     * resolved. That's why we try again -- maybe our DNS cache has the
     * PTR-RR now. This code is sloppy. Far better is to check what the
     * resolver returned so that in case of error, there's no need to
     * try again.
     */
dns_again:
     hp = gethostbyaddr((char *) &sin->sin_addr,
                                sizeof (struct in_addr), AF_INET);

     if ( !hp && ++num_dns_tries <= 1 ) {
        sleep(3);
        goto dns_again;         /* try DNS lookup once more     */
     }
#else
    hp = gethostbyaddr((char *)&sin->sin_addr, sizeof(struct in_addr), AF_INET);
#endif

    blah = inet_ntoa(sin->sin_addr);

    (void) strncpy(remoteaddr, blah, sizeof(remoteaddr));

    if (!strcmp(remoteaddr, "0.0.0.0")) {
        nameserved = 1;
        strncpy(remotehost, "localhost", sizeof(remotehost));
    } else {
        if (hp) {
            nameserved = 1;
            (void) strncpy(remotehost, hp->h_name, sizeof(remotehost));
        } else {
            nameserved = 0;
            (void) strncpy(remotehost, remoteaddr, sizeof(remotehost));
        }
    }

    sprintf(proctitle, MSGSTR(MSG_CONNECTED, "%s: connected"), remotehost);
    setproctitle(proctitle);

#if 0	/* this is redundant unless the caller doesn't do *anything*, and
	   tcpd will pick it up and deal with it better anyways. _H*/
    if (logging)
        syslog(LOG_INFO, "connection from %s [%s]", remotehost,
               remoteaddr);
#endif
}

/* Record logout in wtmp file and exit with supplied status. */

void
#ifdef __STDC__
dologout(int status)
#else
dologout(status)
int status;
#endif
{
     /*
      * Prevent reception of SIGURG from resulting in a resumption
      * back to the main program loop.
      */
     transflag = 0;
 
    if (logged_in) {
        delay_signaling(); /* we can't allow any signals while euid==0: kinch */
        (void) seteuid((uid_t) 0);
        logwtmp(ttyline, "", "");
    }
    if (logging)
	syslog(LOG_INFO, "FTP session closed");
    if (xferlog)
        close(xferlog);
    acl_remove();
    close (data);		/* H* fix: clean up a little better */
    close (pdata);
    /* beware of flushing buffers after a SIGPIPE */
    _exit(status);
}

SIGNAL_TYPE
#ifdef __STDC__
myoob(int sig)
#else
myoob(sig)
int sig;
#endif
{
    char *cp;

    /* only process if transfer occurring */
    if (!transflag) {
#ifdef SIGURG
        (void) signal(SIGURG, myoob);
#endif
        return;
    }
    cp = tmpline;
    if (getline(cp, sizeof(tmpline)-1, stdin) == NULL) {
        reply(221, MSGSTR(MSG_SAY_GOODBYE, "You could at least say goodbye."));
        dologout(0);
    }
    upper(cp);
    if (strcmp(cp, "ABOR\r\n") == 0) {
        tmpline[0] = '\0';
        reply(426, MSGSTR(MSG_TRANSFER_ABORT,
              "Transfer aborted. Data connection closed."));
        reply(226, MSGSTR(MSG_ABORT_SUCCESS, "Abort successful"));
#ifdef SIGURG
        (void) signal(SIGURG, myoob);
#endif
        if (ftwflag > 0) {
            ftwflag++;
            return;
        }
        longjmp(urgcatch, 1);
    }
    if (strcmp(cp, "STAT\r\n") == 0) {
        tmpline[0] = '\0';
        if (file_size != (off_t) - 1)
		reply(213, MSGSTR(MSG_STATUS_OF_BYTES,
                      "Status: %lu of %lu bytes transferred"),
                  (unsigned long)byte_count, (unsigned long)file_size);
        else
            reply(213, MSGSTR(MSG_STATUS,
                  "Status: %lu bytes transferred"), (unsigned long)byte_count);
    }
#ifdef SIGURG
    (void) signal(SIGURG, myoob);
#endif
}

/* Note: a response of 425 is not mentioned as a possible response to the
 * PASV command in RFC959. However, it has been blessed as a legitimate
 * response by Jon Postel in a telephone conversation with Rick Adams on 25
 * Jan 89. */

void
#ifdef __STDC__
passive(void)
#else
passive()
#endif
{
#ifdef UNIXWARE
    size_t len;
#else
    int len;
#endif
    register char *p,
     *a;

/* H* fix: if we already *have* a passive socket, close it first.  Prevents
   a whole variety of entertaining clogging attacks. */
    if (pdata > 0)
	close (pdata);
    if (!logged_in) {
       reply(530, MSGSTR(MSG_LOGIN_USER_FIRST, "Login with USER first."));
       return;
    }
    pdata = socket(AF_INET, SOCK_STREAM, 0);
    if (pdata < 0) {
        perror_reply(425, MSGSTR(MSG_CANT_OPEN_PASSIVE,
                     "Can't open passive connection"));
        return;
    }
    pasv_addr = ctrl_addr;
    pasv_addr.sin_port = 0;
    delay_signaling(); /* we can't allow any signals while euid==0: kinch */
    (void) seteuid((uid_t) 0);		/* XXX: not needed if > 1024 */
    if (bind(pdata, (struct sockaddr *) &pasv_addr, sizeof(pasv_addr)) < 0) {
        (void) seteuid((uid_t) pw->pw_uid);
        enable_signaling(); /* we can allow signals once again: kinch */
        goto pasv_error;
    }
    (void) seteuid((uid_t) pw->pw_uid);
    enable_signaling(); /* we can allow signals once again: kinch */
    len = sizeof(pasv_addr);
    if (getsockname(pdata, (struct sockaddr *) &pasv_addr, &len) < 0)
        goto pasv_error;
    if (listen(pdata, 1) < 0)
        goto pasv_error;
    a = (char *) &pasv_addr.sin_addr;
    p = (char *) &pasv_addr.sin_port;

#define UC(b) (((int) b) & 0xff)

    reply(227, MSGSTR(MSG_ENTERING_PASSIVE,
          "Entering Passive Mode (%d,%d,%d,%d,%d,%d)"), UC(a[0]),
          UC(a[1]), UC(a[2]), UC(a[3]), UC(p[0]), UC(p[1]));
    return;

  pasv_error:
    (void) close(pdata);
    pdata = -1;
    perror_reply(425, MSGSTR(MSG_CANT_OPEN_PASSIVE,
                 "Can't open passive connection"));
    return;
}

/* Generate unique name for file with basename "local". The file named
 * "local" is already known to exist. Generates failure reply on error. */
char *
#ifdef __STDC__
gunique(char *local)
#else
gunique(local)
char *local;
#endif
{
    static char new[MAXPATHLEN];
    struct stat st;
    char *cp = strrchr(local, '/');
    int count = 0;

    if (cp)
        *cp = '\0';
    if (stat(cp ? local : ".", &st) < 0) {
        perror_reply(553, cp ? local : ".");
        return ((char *) 0);
    }
    if (cp)
        *cp = '/';
    (void) strncpy(new, local, (sizeof new) - 3);
    new[sizeof(new) - 3] = '\0';
    cp = new + strlen(new);
    *cp++ = '.';
    for (count = 1; count < 100; count++) {
        if (count == 10) {
            cp-=2;
            *cp++ = '.';
        }
        (void) sprintf(cp, "%d", count);
        if (stat(new, &st) < 0)
            return (new);
    }
    reply(452, MSGSTR(MSG_UNIQUE_FILE, "Unique file name cannot be created."));
    return ((char *) 0);
}

/* Format and send reply containing system error number. */

void
#ifdef __STDC__
perror_reply(int code, char *string)
#else
perror_reply(code,string)
int code;
char *string;
#endif
{
    reply(code, "%s: %s.", string, strerror(errno));
}

static char *onefile[] =
{"", 0};

void
#ifdef __STDC__
send_file_list(char *whichfiles)
#else
send_file_list(whichfiles)
char *whichfiles;
#endif
{
    /* static so not clobbered by longjmp(), volatile would also work */
    static FILE *dout;
    static DIR *dirp;
    static char **sdirlist;

    struct stat st;

#ifdef HAVE_DIRENT
    struct dirent *dir;
#else
    struct direct *dir;
#endif

    register char **dirlist,
     *dirname;
    int simple = 0;
#ifdef __STDC__
    char *strpbrk(const char *, const char *);
#else
    char *strpbrk();
#endif

    dout = NULL;
    dirp = NULL;
    sdirlist = NULL;
    if (strpbrk(whichfiles, "~{[*?") != NULL) {
#ifdef __STDC__
        extern char **ftpglob(register char *v),
#else
        extern char **ftpglob(),
#endif
         *globerr;

        globerr = NULL;
        dirlist = ftpglob(whichfiles);
        sdirlist = dirlist;  /* save to free later */
        if (globerr != NULL) {
            reply(550, globerr);
            goto globfree;
        } else if (dirlist == NULL) {
            errno = ENOENT;
            perror_reply(550, whichfiles);
            return;
        }
    } else {
        onefile[0] = whichfiles;
        dirlist = onefile;
        simple = 1;
    }

    if (setjmp(urgcatch)) {
        transflag = 0;
        if (dout != NULL)
            (void) fclose(dout);
        if (dirp != NULL)
            (void) closedir(dirp);
        data = -1;
        pdata = -1;
        goto globfree;
    }
    while ((dirname = *dirlist++) != NULL) {
        if (stat(dirname, &st) < 0) {
            /* If user typed "ls -l", etc, and the client used NLST, do what
             * the user meant. */
            if (dirname[0] == '-' && *dirlist == NULL && transflag == 0) {
                retrieve("/bin/ls %s", dirname);
                goto globfree;
            }
            perror_reply(550, dirname);
            if (dout != NULL) {
                (void) fclose(dout);
                transflag = 0;
                data = -1;
                pdata = -1;
            }
            goto globfree;
        }
        if ((st.st_mode & S_IFMT) == S_IFREG) {
            if (dout == NULL) {
                dout = dataconn(MSGSTR(MSG_FILE_LIST, "file list"),
                                (off_t) - 1, "w");
                if (dout == NULL)
                    goto globfree;
                transflag++;
            }
            fprintf(dout, "%s%s\n", dirname,
                    type == TYPE_A ? "\r" : "");
            byte_count += strlen(dirname) + 1;
            continue;
        } else if ((st.st_mode & S_IFMT) != S_IFDIR)
            continue;

        if ((dirp = opendir(dirname)) == NULL)
            continue;

        while ((dir = readdir(dirp)) != NULL) {
            char nbuf[MAXPATHLEN];

#ifndef HAVE_DIRENT    /* does not have d_namlen */
            if (dir->d_name[0] == '.' && dir->d_namlen == 1)
#else
            if (dir->d_name[0] == '.' && (strlen(dir->d_name) == 1))
#endif
                continue;
#ifndef HAVE_DIRENT    /* does not have d_namlen */
            if (dir->d_namlen == 2 && dir->d_name[0] == '.' &&
                dir->d_name[1] == '.')
#else
            if ((strlen(dir->d_name) == 2) && dir->d_name[0] == '.' &&
                dir->d_name[1] == '.')
#endif
                continue;

            snprintf(nbuf, sizeof nbuf, "%s/%s", dirname, dir->d_name);

            /* We have to do a stat to insure it's not a directory or special
             * file. */
            if (simple || (stat(nbuf, &st) == 0 &&
                           (st.st_mode & S_IFMT) == S_IFREG)) {
                if (dout == NULL) {
                    dout = dataconn(MSGSTR(MSG_FILE_LIST, "file list"),
                                    (off_t) - 1, "w");
                    if (dout == NULL) {
                        (void) closedir(dirp);
                        goto globfree;
                    }
                    transflag++;
                }
                if (nbuf[0] == '.' && nbuf[1] == '/')
                    fprintf(dout, "%s%s\n", &nbuf[2],
                            type == TYPE_A ? "\r" : "");
                else
                    fprintf(dout, "%s%s\n", nbuf,
                            type == TYPE_A ? "\r" : "");
                byte_count += strlen(nbuf) + 1;
            }
        }
        (void) closedir(dirp);
        dirp = NULL;
    }

    if (dout == NULL)
        reply(550, MSGSTR(MSG_NO_FILES, "No files found."));
    else if (ferror(dout) != 0)
        perror_reply(550, MSGSTR(MSG_DATA_CONN, "Data connection"));
    else
        reply(226, MSGSTR(MSG_TRANSFER_COMP, "Transfer complete."));

    transflag = 0;
    if (dout != NULL)
        (void) fclose(dout);
    data = -1;
    pdata = -1;
globfree:
    if (sdirlist) {
        blkfree(sdirlist);
        free((char *) sdirlist);
    }
}

/*
**  SETPROCTITLE -- set process title for ps (from sendmail 8)
**
**	Parameters:
**		fmt -- a printf style format string.
**		a, b, c -- possible parameters to fmt.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Clobbers argv of our main procedure so ps(1) will
**		display the title.
*/

#ifndef SPT_TYPE
# define SPT_TYPE	SPT_REUSEARGV  /* default type */
#endif

#if SPT_TYPE != SPT_NONE && SPT_TYPE != SPT_BUILTIN

#if SPT_TYPE == SPT_PSTAT
#include <sys/pstat.h>
#endif
#if SPT_TYPE == SPT_PSSTRINGS
#include <machine/vmparam.h>
#include <sys/exec.h>
#ifndef PS_STRINGS	/* hmmmm....  apparently not available after all */
#undef SPT_TYPE
#define SPT_TYPE	SPT_REUSEARGV
#else
#ifndef NKPDE			/* FreeBSD 2.0 */
#define NKPDE 63
typedef unsigned int	*pt_entry_t;
#endif
#endif
#endif

#if SPT_TYPE == SPT_PSSTRINGS
#define SETPROC_STATIC	static
#else
#define SETPROC_STATIC
#endif

#if SPT_TYPE == SPT_SYSMIPS
#include <sys/sysmips.h>
#include <sys/sysnews.h>
#endif

#if SPT_TYPE == SPT_SCO
#ifdef UXW
#include <sys/exec.h>
#include <sys/ksym.h>
#include <sys/proc.h>
#include <sys/user.h>
#else  /* UXW */
#include <sys/immu.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/fs/s5param.h>
#endif /* UXW */
#if PSARGSZ > 2048
#define SPT_BUFSIZE	PSARGSZ
#endif
#endif

#ifndef SPT_PADCHAR
#define SPT_PADCHAR	' '
#endif

#ifndef SPT_BUFSIZE
#define SPT_BUFSIZE	2048
#endif

#endif /* SPT_TYPE != SPT_NONE && SPT_TYPE != SPT_BUILTIN */

#if SPT_TYPE != SPT_BUILTIN

/*VARARGS1*/
void
#ifdef __STDC__
setproctitle(const char *fmt, ...)
#else
setproctitle(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
#if SPT_TYPE != SPT_NONE
	register char *p;
	register int i;
	SETPROC_STATIC char buf[SPT_BUFSIZE];
	VA_LOCAL_DECL
#if SPT_TYPE == SPT_PSTAT
	union pstun pst;
#endif
#if SPT_TYPE == SPT_SCO
#ifdef UXW
	static off_t seek_off;
#else  /* UXW */
	off_t seek_off;
#endif /* UXW */
	static int kmem = -1;
	static int kmempid = -1;
#endif
#if SPT_TYPE == SPT_REUSEARGV
	extern char **Argv;
	extern char *LastArgv;
#endif

	p = buf;

	/* print ftpd: heading for grep */
	(void) strcpy(p, "ftpd: ");
	p += strlen(p);

	/* print the argument string */
	VA_START(fmt);
	(void) vsnprintf(p, sizeof buf - (p - buf), fmt, ap);
	VA_END;

	i = strlen(buf);

#if SPT_TYPE == SPT_PSTAT
	pst.pst_command = buf;
	pstat(PSTAT_SETCMD, pst, i, 0, 0);
#endif
#if SPT_TYPE == SPT_PSSTRINGS
	if (PS_STRINGS->ps_argvstr) {
                static char *ps_argv[2];
                ps_argv[0] = buf;
                ps_argv[1] = NULL;
                PS_STRINGS->ps_nargvstr = 1;
                PS_STRINGS->ps_argvstr = ps_argv;
        } else {
                PS_STRINGS->ps_nargvstr = 1;
                PS_STRINGS->ps_argvstr = buf;
        }
#endif
#if SPT_TYPE == SPT_SYSMIPS
	sysmips(SONY_SYSNEWS, NEWS_SETPSARGS, buf);
#endif
#if SPT_TYPE == SPT_SCO
	if (kmem < 0 || kmempid != getpid())
	{
		if (kmem >= 0)
			close(kmem);
		kmem = open(_PATH_KMEM, O_RDWR, 0);
		if (kmem < 0)
			return;
		(void) fcntl(kmem, F_SETFD, 1);
		kmempid = getpid();
#ifdef UXW
	{
		off_t offset;
		void *ptr;
		struct mioc_rksym rks;

		seek_off = 0;
		rks.mirk_symname = "upointer";
		rks.mirk_buf = &ptr;
		rks.mirk_buflen = sizeof(ptr);
		if (ioctl(kmem, MIOC_READKSYM, &rks) < 0)
			return;
		offset = (off_t)ptr + (off_t)&((struct user *)0)->u_procp;
		if (lseek(kmem, offset, SEEK_SET) != offset)
			return;
		if (read(kmem, &ptr, sizeof(ptr)) != sizeof(ptr))
			return;
		offset = (off_t)ptr + (off_t)&((struct proc *)0)->p_execinfo;
		if (lseek(kmem, offset, SEEK_SET) != offset)
			return;
		if (read(kmem, &ptr, sizeof(ptr)) != sizeof(ptr))
			return;
		seek_off = (off_t)ptr + (off_t)((struct execinfo *)0)->ei_psargs;
	}
#endif /* UXW */
	}
	buf[PSARGSZ - 1] = '\0';
#ifdef UXW
	if (seek_off == 0)
		return;
#else  /* UXW */
	seek_off = UVUBLK + (off_t) &((struct user *)0)->u_psargs;
#endif /* UXW */
	if (lseek(kmem, seek_off, SEEK_SET) == seek_off)
		(void) write(kmem, buf, PSARGSZ);
#endif
#if SPT_TYPE == SPT_REUSEARGV
/* This may fix problems in AIX, I don't really know since I don't have AIX */
/* Maybe someone will check it out and mail me, or someone will get me      */
/* ready access to AIX */
#ifndef AIX
	if (i > LastArgv - Argv[0] - 2)
	{
		i = LastArgv - Argv[0] - 2;
		buf[i] = '\0';
	}
#endif
	(void) strcpy(Argv[0], buf);
#ifndef AIX
	p = &Argv[0][i];
	while (p < LastArgv)
		*p++ = SPT_PADCHAR;
	Argv[1] = NULL;
#endif
#endif
#endif /* SPT_TYPE != SPT_NONE */
}

#endif /* SPT_TYPE != SPT_BUILTIN */

#ifdef KERBEROS
/* thanks to gshapiro@wpi.wpi.edu for the following kerberosities */

void
init_krb()
{
    char hostname[100];

#ifdef HAVE_SYSINFO
    if (sysinfo(SI_HOSTNAME, hostname, sizeof (hostname)) < 0) {
        perror("sysinfo");
#else
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        perror("gethostname");
#endif
        exit(1);
    }
    if (strchr(hostname, '.'))
        *(strchr(hostname, '.')) = 0;

    sprintf(krb_ticket_name, "/var/dss/kerberos/tkt/tkt.%d", getpid());
    krb_set_tkt_string(krb_ticket_name);

    config_auth();

    if (krb_svc_init("hesiod", hostname, (char *) NULL, 0, (char *) NULL,
                     (char *) NULL) != KSUCCESS) {
        fprintf(stderr, "Couldn't initialize Kerberos\n");
        exit(1);
    }
}

void
end_krb()
{
    unlink(krb_ticket_name);
}
#endif /* KERBEROS */

#ifdef ULTRIX_AUTH
static int
ultrix_check_pass(char *passwd, char *xpasswd)
{
    struct svcinfo *svp;
    int auth_status;

    if ((svp = getsvc()) == (struct svcinfo *) NULL) {
        syslog(LOG_WARNING, "getsvc() failed in ultrix_check_pass");
        return -1;
    }
    if (pw == (struct passwd *) NULL) {
        return -1;
    }
    if (((svp->svcauth.seclevel == SEC_UPGRADE) &&
        (!strcmp(pw->pw_passwd, "*")))
        || (svp->svcauth.seclevel == SEC_ENHANCED)) {
        if ((auth_status=authenticate_user(pw, passwd, "/dev/ttypXX")) >= 0) {
            /* Indicate successful validation */
            return auth_status;
        }
        if (auth_status < 0 && errno == EPERM) {
            /* Log some information about the failed login attempt. */
            switch(abs(auth_status)) {
            case A_EBADPASS:
                break;
            case A_ESOFTEXP:
                syslog(LOG_NOTICE, "password will expire soon for user %s",
                    pw->pw_name);
                break;
            case A_EHARDEXP:
                syslog(LOG_NOTICE, "password has expired for user %s",
                    pw->pw_name);
                break;
            case A_ENOLOGIN:
                syslog(LOG_NOTICE, "user %s attempted login to disabled acct",
                    pw->pw_name);
                break;
            }
        }
    }
    else {
        if ((*pw->pw_passwd != '\0') && (!strcmp(xpasswd, pw->pw_passwd))) {
            /* passwd in /etc/passwd isn't empty && encrypted passwd matches */
            return 0;
        }
    }
    return -1;
}
#endif /* ULTRIX_AUTH */

#if defined(SHADOW_PASSWORD) && defined(UXW)
#include <lastlog.h>
#define LASTLOG		"/var/adm/lastlog"
#define DAY_SECS	(24L * 60 * 60)

int
login_allowed(uid_t uid, struct spwd *spw)
{
	time_t today;
	int llfd;
	struct lastlog ll;

	today = time((time_t *) 0) / DAY_SECS;

	if ((spw->sp_expire > 0) && (spw->sp_expire < today))
		return 0;

	if ((spw->sp_max > 0) && (spw->sp_lstchg > 0) &&
	    (spw->sp_lstchg + spw->sp_max < today))
		return 0;

	if (spw->sp_inact <= 0)
		return 1;

	if ((llfd = open(LASTLOG, O_RDONLY)) == -1) {
		syslog(LOG_WARNING, "Unable to open %s: %m (skipping check)",
			LASTLOG);
		return 1;
	}
	(void) lseek(llfd, uid * sizeof(struct lastlog), SEEK_SET);
	if (read(llfd, &ll, sizeof(ll)) != sizeof(ll)) {
		syslog(LOG_WARNING,
	"Unable to read last login info for UID %d: %m (skipping check)", uid);
		(void) close(llfd);
		return 1;
	}
	(void) close(llfd);
	if ((ll.ll_time > 0) &&
	    ((ll.ll_time / DAY_SECS) + spw->sp_inact < today))
		return 0;
	return 1;
}
#endif /* SHADOW_PASSWORD && UXW */
