/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Lachman Technology, Inc.
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

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)rlogin.c	1.4"
#ident	"$Header$"

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

/*  "prf()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catlog name.
 *  The catalog name the string is output using <MM_WARNING>.
 */

/*
 * rlogin - remote login
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/wait.h>
#ifdef SYSV
#include <sys/stropts.h>
#include <sys/termios.h>
#include <sys/ttold.h>
#else
#include <sys/ioctl.h>
#endif SYSV

#include <netinet/in.h>

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <signal.h>
#include <setjmp.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/sockio.h>
#include <ctype.h>
#include <locale.h>
#include <pfmt.h>
#include "../usr.sbin/security.h"

# ifndef TIOCPKT_WINDOW
# define TIOCPKT_WINDOW 0x80
# endif TIOCPKT_WINDOW

#ifdef SYSV

#define	RAW	O_RAW
#define	CBREAK	O_CBREAK
#define	TBDELAY	O_TBDELAY
#define	CRMOD	O_CRMOD

/*
 * XXX - SysV ptys don't have BSD packet mode, but these should still
 * be defined in some header file.
 */
#define		TIOCPKT_DATA		0x00	/* data packet */
#define		TIOCPKT_FLUSHREAD	0x01	/* flush data not yet written to controller */
#define		TIOCPKT_FLUSHWRITE	0x02	/* flush data read from controller but not yet processed */
#define		TIOCPKT_STOP		0x04	/* stop output */
#define		TIOCPKT_START		0x08	/* start output */
#define		TIOCPKT_NOSTOP		0x10	/* no more ^S, ^Q */
#define		TIOCPKT_DOSTOP		0x20	/* now do ^S, ^Q */
#define		TIOCPKT_IOCTL		0x40	/* "ioctl" packet */

#define rindex	strrchr
#define index	strchr
#define bcopy(a,b,c)	  memcpy(b,a,c)
#define bcmp(a,b,c)	  memcmp(b,a,c)
#define bzero(s,n)        memset((s), 0, (n))

#ifndef sigmask
#define sigmask(m)      (1 << ((m)-1))
#endif

#define set2mask(setp) ((setp)->sa_sigbits[0])
#define mask2set(mask, setp) \
	((mask) == -1 ? sigfillset(setp) : (((setp)->sa_sigbits[0]) = (mask)))
	
/* Taken from in.login.c */
#define	SCPYN(a, b)		(void) strncpy((a), (b), (sizeof((a))-1))
#define	ENVSTRNCAT(to, from)	{int deflen; deflen = strlen(to);\
				(void) strncpy((to) + deflen, (from),\
				 sizeof(to) - (1 + deflen));}

static sigsetmask(mask)
	int mask;
{
	sigset_t oset;
	sigset_t nset;

	(void) sigprocmask(0, (sigset_t *)0, &nset);
	mask2set(mask, &nset);
	(void) sigprocmask(SIG_SETMASK, &nset, &oset);
	return set2mask(&oset);
}

static sigblock(mask)
	int mask;
{
	sigset_t oset;
	sigset_t nset;

	(void) sigprocmask(0, (sigset_t *)0, &nset);
	mask2set(mask, &nset);
	(void) sigprocmask(SIG_BLOCK, &nset, &oset);
	return set2mask(&oset);
}

#endif /* SYSV */


char	*index(), *rindex(), *malloc(), *getenv();
char	*errmsg();
char	*name;
int	port_number = IPPORT_LOGINSERVER;
int	rem;
char	cmdchar = '~';
int     bits_per_char;
int	litout;
char	*speeds[] =
    { "0", "50", "75", "110", "134", "150", "200", "300",
      "600", "1200", "1800", "2400", "4800", "9600", "19200", "38400" };
char	term[256] = "network";
char	thisspeed[10];/* size must be >= (size of largest speeds[] entry) + 1 */
extern	int errno;
int	lostpeer();
int	dosigwinch = 0;
#ifndef sigmask
#define sigmask(m)	(1 << ((m)-1))
#endif
struct	winsize winsize;
int	sigwinch(), oob();

struct termios  ttyatt;	
struct termios  satt;  

main(argc, argv)
	int argc;
	char **argv;
{
	char *host, *cp;
	struct passwd *pwd;
	struct passwd *getpwuid();
	int options = 0, oldmask;
	int on = 1;
	int ret;
	int c;
	int ac;
	char **av;
	extern int optind;
	extern char *optarg;
	struct linger linger;

	DISABLE_WORK_PRIVS;
	linger.l_onoff = 1;
	linger.l_linger = 60;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxrlogin");
	(void)setlabel("UX:rlogin");

	/*
	 * must be compatible with old-style command line parsing,
	 * which allowed hostname to come before arguments, so
	 *	rlogin host -l user
	 * is permissible, even tho' it doesn't fit the command
	 * syntax standard.
	 */
	if ( argc > 1 && argv[1][0] != '-' ) {
		ac = argc -1; av = &argv[1];
	} else {
		ac = argc; av = argv;
	}
	while ( (c = getopt(ac, av, "dl:e:78L")) != -1 ) {
		switch(c ) {
		
		case 'd':
			options |= SO_DEBUG;
			break;
		case 'l':
			name = optarg;
			break;
		case 'e':
			cmdchar = *optarg;
			break;
		case '7':
			bits_per_char = 7;
			break;
		case '8':
			bits_per_char = 8;
			break;
		case 'L':
			litout = 1;
			break;
		}
	}
	host = rindex(argv[0], '/');
	if (host)
		host++;
	else
		host = argv[0];
	if ( strcmp(host, "rlogin") == 0 ) {
		if ( ac == argc ) {
			if ( optind >= argc )
				goto usage;
			host = argv[optind];
		} else {
			host = argv[1];
		}
		++optind;
	}
	if (optind < argc)
		goto usage;
	pwd = getpwuid(getuid());
	if (pwd == 0) {
		pfmt(stderr, MM_ERROR, ":1:Who are you?\n");
		exit(1);
	}
	cp = getenv("TERM");

        if ((ret = tcgetattr(0, &ttyatt)) == 0) {                    
        	SCPYN(thisspeed, speeds[ttyatt.c_cflag & CBAUD]); 
		if (cp)
			strncpy(term, cp, sizeof(term)-2-strlen(thisspeed));
                ENVSTRNCAT(term, "/");                            
                ENVSTRNCAT(term, thisspeed);
	} else if (cp)
		SCPYN(term, cp);

	if (ret == -1)
		pfmt(stderr, MM_NOGET|MM_ERROR,
			"tcgetattr: %s\n", strerror(errno));

	(void) ioctl(0, TIOCGWINSZ, &winsize);

	(void) signal(SIGPIPE, (void (*)())lostpeer);
	/* will use SIGUSR1 for window size hack, so hold it off */
	oldmask = sigblock(sigmask(SIGURG) | sigmask(SIGUSR1));
	ENABLE_WORK_PRIVS;
        rem = rcmd(&host, htons(port_number), pwd->pw_name,
	    name ? name : pwd->pw_name, term, 0);
#ifdef SYSV
	if ((rem >= 0) && is_enhancedsecurity()) {
		fd_set_to_my_level(rem);
	}
#endif SYSV
	DISABLE_WORK_PRIVS;
        if (rem < 0)
		goto pop;
	ENABLE_WORK_PRIVS;
	if (options & SO_DEBUG &&
	    setsockopt(rem, SOL_SOCKET, SO_DEBUG, &on, sizeof (on)) < 0)
		pfmt(stderr, MM_NOGET|MM_ERROR,
			"SO_DEBUG (ignored): %s\n", strerror(errno));
	DISABLE_WORK_PRIVS;
#ifndef SYSV
	if (CLR_WORKPRIVS_NON_ADMIN(setuid(getuid()) < 0) {
		pfmt(stderr, MM_NOGET|MM_ERROR,
			"setuid: %s\n", strerror(errno));
		goto pop;
	}
#endif !SYSV
	doit(oldmask);
	/*NOTREACHED*/
usage:
	pfmt(stderr, MM_ACTION,
	  ":2:Usage: rlogin [ -ex ] [ -l username ] [ -7 ] [ -8 ] [ -L ] host\n");

pop:
	exit(1);
}

#define CRLF "\r\n"

pid_t	child;
int	catchild();
int	copytochild(), writeroob();

char	deferase, defkill;

doit(oldmask)
{
        deferase = ttyatt.c_cc[VERASE]; 
        defkill  = ttyatt.c_cc[VKILL];  

	(void) signal(SIGINT, SIG_IGN);
	setsignal(SIGHUP, exit);
	setsignal(SIGQUIT, exit);
	mode(1);
	ENABLE_WORK_PRIVS;
	child = fork();
	DISABLE_WORK_PRIVS;
	if (child == (pid_t)-1) {
		pfmt(stderr, MM_NOGET|MM_ERROR,
			"fork: %s\n", strerror(errno));
		done(1);
	}
	if (child == 0) {
		if (reader(oldmask) == 0) {
			prf(MM_NOSTD,":3:Connection closed.");
			exit(0);
		}
		sleep(1);
		prf(MM_NOSTD,":4:\007Connection closed.");
		exit(3);
	}

/*	This seems to cause duplicate signal delivery in child */
/*	(void) signal(SIGURG, (void (*)())copytochild); */

	(void) signal(SIGUSR1, (void (*)())writeroob);
	(void) sigsetmask(oldmask);
	(void) signal(SIGCHLD, (void (*)())catchild);
	writer();
	prf(MM_NOSTD,":5:Closed connection.");
	done(0);
}

/*
 * Trap a signal, unless it is being ignored.
 */
setsignal(sig, act)
	int sig, (*act)();
{
	int omask = sigblock(sigmask(sig));

	if (signal(sig, (void (*)())act) == SIG_IGN)
		(void) signal(sig, SIG_IGN);
	(void) sigsetmask(omask);
}
 
done(status)
	int status;
{
	pid_t w;

	mode(0);
	if (child > 0) {
		/* make sure catchild does not snap it up */
		(void) signal(SIGCHLD, SIG_DFL);
		if (kill(child, SIGKILL) >= 0)
			while ((w = wait(0)) > 0 && w != child)
				/*void*/;
	}
	exit(status);
}

/*
 * Copy SIGURGs to the child process.
 */
copytochild()
{

	(void) signal(SIGURG, (void (*)())copytochild);
	(void) kill(child, SIGURG);
}

/*
 * This is called when the reader process gets the out-of-band (urgent)
 * request to turn on the window-changing protocol.
 */
writeroob()
{
	(void) signal(SIGUSR1, (void (*)())writeroob);
	if (dosigwinch == 0) {
		sendwindow();
		(void)signal(SIGWINCH, (void (*)())sigwinch);
	}
	dosigwinch = 1;
}

catchild()
{
#ifdef SYSV
#include <sys/siginfo.h>
	int status;
	int pid;
	int options;
	siginfo_t       info;
	int error;

	while(1){
		options = WNOHANG | WEXITED;
		error = waitid(P_ALL, 0, &info, options); 
		if ( error != 0 )  return(error);
		if  (info.si_pid == 0) return (0);
		if (info.si_code ==  CLD_TRAPPED) continue;
		if (info.si_code ==  CLD_STOPPED) continue;
		done(info.si_status);
	}
#else
	union wait status;
	pid_t pid;

again:
	pid = wait3(&status, WNOHANG|WUNTRACED, (struct rusage *)0);
	if (pid == 0)
		return;
	/*
	 * if the child (reader) dies, just quit
	 */
	if (pid < 0 || pid == child && !WIFSTOPPED(status.w_status))
		done((int)(status.w_termsig | status.w_retcode));
	goto again;
#endif /* SYSV */
}

/*
 * writer: write to remote: 0 -> line.
 * ~.	terminate
 * ~!	execute subshell
 * ~^Z	suspend rlogin process.
 * ~^Y  suspend rlogin process, but leave reader alone.
 */
writer()
{
	char c;
	register n;
	register bol = 1;               /* beginning of line */
	register local = 0;

	for (;;) {
		n = read(0, &c, 1);
		if (n <= 0) {
			if (n == 0)
				break;
			if (errno == EINTR)
				continue;
			else {
				prf(MM_ERROR,
				    ":6:Read error from terminal: %s",
					errmsg(errno));
				break;
			}
                }
		/*
		 * If we're at the beginning of the line
		 * and recognize a command character, then
		 * we echo locally.  Otherwise, characters
		 * are echo'd remotely.  If the command
		 * character is doubled, this acts as a 
		 * force and local echo is suppressed.
		 */
		if (bol) {
			bol = 0;
			if (cmdchar && (c == cmdchar)) {
				bol = 0;
				local = 1;
				continue;
			}
		} else if (local) {
			local = 0;
			if (c == '.' || c == ttyatt.c_cc[VEOF]) {
				echo(c);
				break;
			}
			if (c == ttyatt.c_cc[VSUSP]
#ifdef VDSUSP
			    || c == ttyatt.c_cc[VDSUSP]
#endif
			    ) {
				bol = 1;
				echo(c);
				stop(c);
				continue;
			}
			if (c == '!') {
				register int proc;
				register void (*oldsig)();

				write(0, CRLF, sizeof(CRLF) - 1);
				write(1, "!\r\n", 3);
				oldsig = signal(SIGCHLD, SIG_DFL);
				proc = fork();
				if (proc == 0) {
					extern char *getenv();
					char *shell = getenv("SHELL");
					if (shell == 0) shell = "/bin/sh";
					close(1);
					close(2);
					dup(0);
					dup(0);
					mode(0);
					signal(SIGINT, SIG_DFL);
					signal(SIGQUIT, SIG_DFL);
					signal(SIGHUP, SIG_DFL);
					signal(SIGPIPE, SIG_DFL);
					CLR_MAXPRIVS_FOR_EXEC;
					execl(shell, shell, "-i", 0);
					prf(MM_ERROR,":7:Cannot execute shell");

					exit(-1);
				}
				if (proc != -1)
					while (wait((int *)0) != proc)
						continue;
				signal(SIGCHLD, oldsig);
				mode(1);
				sigwinch();      /* check for size changes */
				write(1, "\r\n!\r\n", 5);
				continue; 
			}
			if (c != cmdchar) {
				if (write(rem, &cmdchar, 1) < 0) {
					prf(MM_ERROR,
					    ":8:Write error to network: %s",
						errmsg(errno));
					break;
				}
			}
		}
		if ((n = write(rem, &c, 1)) <= 0) {
			if (n == 0)
				prf(MM_ERROR,":9:line gone");
			else
				prf(MM_ERROR,
				    ":8:Write error to network: %s",
					errmsg(errno));
			break;
		}
		bol = c == defkill || c == ttyatt.c_cc[VEOF] ||
		    c == ttyatt.c_cc[VINTR] ||
		    c == '\r' || c == '\n';
	}
}

echo(c)
register char c;
{
	char buf[8];
	register char *p = buf;

	c &= 0177;
	*p++ = cmdchar;
	if (c < ' ') {
		*p++ = '^';
		*p++ = c + '@';
	} else if (c == 0177) {
		*p++ = '^';
		*p++ = '?';
	} else
		*p++ = c;
	*p++ = '\r';
	*p++ = '\n';
	if (write(1, buf, p - buf) < 0)
		prf(MM_ERROR,
		    ":10:Write error to terminal: %s", errmsg(errno));
}

stop(cmdc)
	char cmdc;
{
	mode(0);
	(void) signal(SIGCHLD, SIG_IGN);
	(void) kill(cmdc == ttyatt.c_cc[VSUSP] ? 0 : getpid(), SIGTSTP);
	(void) signal(SIGCHLD, (void (*)())catchild);
	mode(1);
	sigwinch();			/* check for size changes */
}
	
sigwinch()
{
	struct winsize ws;

	if (dosigwinch && ioctl(0, TIOCGWINSZ, &ws) == 0 &&
	    bcmp(&ws, &winsize, sizeof (ws))) {
		winsize = ws;
		sendwindow();
	}
	(void)signal(SIGWINCH, (void (*)())sigwinch);
}

/*
 * Send the window size to the server via the magic escape
 */
sendwindow()
{
	char obuf[4 + sizeof (struct winsize)];
	struct winsize *wp = (struct winsize *)(obuf+4);

	obuf[0] = 0377;
	obuf[1] = 0377;
	obuf[2] = 's';
	obuf[3] = 's';
	wp->ws_row = htons(winsize.ws_row);
	wp->ws_col = htons(winsize.ws_col);
	wp->ws_xpixel = htons(winsize.ws_xpixel);
	wp->ws_ypixel = htons(winsize.ws_ypixel);
	if (write(rem, obuf, sizeof(obuf)) < 0)
		prf(MM_ERROR,
		    ":8:Write error to network: %s", errmsg(errno));
}


/*
 * reader: read from remote: remote -> stdout
 */
#define	READING	1
#define	WRITING	2

char	rcvbuf[8 * 1024];
int	rcvcnt;
int	rcvstate;
pid_t	ppid;
jmp_buf	rcvtop;

oob()
{
	int out = FWRITE, atmark, n;
	int rcvd = 0;
	char waste[BUFSIZ], mark;

	(void) signal(SIGURG, (void (*)())oob);
	while (recv(rem, &mark, 1, MSG_OOB) < 0)
		switch (errno) {
		
		case EWOULDBLOCK:
			/*
			 * Urgent data not here yet.
			 * It may not be possible to send it yet
			 * if we are blocked for output
			 * and our input buffer is full.
			 */
			if (rcvcnt < sizeof(rcvbuf)) {
				n = read(rem, rcvbuf + rcvcnt,
					sizeof(rcvbuf) - rcvcnt);
				if (n <= 0)
					return;
				rcvd += n;
			} else {
				n = read(rem, waste, sizeof(waste));
				if (n <= 0)
					return;
			}
			continue;
				
		default:
			return;
	}
	if (mark & TIOCPKT_WINDOW) {
		/*
		 * Let server know about window size changes
		 */
		(void) kill(ppid, SIGUSR1);
	}
	if (mark & TIOCPKT_NOSTOP) {
                satt.c_iflag &= ~IXON;
		if(tcsetattr(0, TCSANOW, &satt) == -1)
                        pfmt(stderr, MM_NOGET|MM_ERROR,
				"tcsetattr: %s\n", strerror(errno));
	}
	if (mark & TIOCPKT_DOSTOP) {
                satt.c_iflag |= IXON;
		if(tcsetattr(0, TCSANOW, &satt) == -1)
                        pfmt(stderr, MM_NOGET|MM_ERROR,
				"tcsetattr: %s\n", strerror(errno));
	}
	if (mark & TIOCPKT_FLUSHWRITE) {
		if (ioctl(1, TIOCFLUSH, (char *)&out) == -1)
                        pfmt(stderr, MM_NOGET|MM_ERROR,
				"ioctl TIOCFLUSH: %s\n", strerror(errno));
		for (;;) {
			if (ioctl(rem, SIOCATMARK, &atmark) < 0) {
                        	pfmt(stderr, MM_NOGET|MM_ERROR,
					"ioctl SIOCATMARK: %s\n", strerror(errno));
				break;
			}
			if (atmark)
				break;
			n = read(rem, waste, sizeof (waste));
			if (n <= 0) {
				if (n < 0)
					prf(MM_ERROR,
					    ":11:Read error from network: %s",
						errmsg(errno));
				break;
			}
		}
		/*
		 * Don't want any pending data to be output,
		 * so clear the recv buffer.
		 * If we were hanging on a write when interrupted,
		 * don't want it to restart.  If we were reading,
		 * restart anyway.
		 */
		rcvcnt = 0;
		longjmp(rcvtop, 1);
	}
	/*
	 * If we filled the receive buffer while a read was pending,
	 * longjmp to the top to restart appropriately.  Don't abort
	 * a pending write, however, or we won't know how much was written.
	 */
	if (rcvd && rcvstate == READING)
		longjmp(rcvtop, 1);
}

/*
 * reader: read from remote: line -> 1
 */
reader(oldmask)
	int oldmask;
{
	/*
	 * 4.3bsd or later and SunOS 4.0 or later use the posiitive
	 * pid; otherwise use the negative.
	 */
	pid_t pid = getpid();
	int n, remaining;
	char *bufp = rcvbuf;

	(void) signal(SIGTTOU, SIG_IGN);
	(void) signal(SIGURG, (void (*)())oob);
	ppid = getppid();
	if (fcntl(rem, F_SETOWN, pid) == -1)
                pfmt(stderr, MM_NOGET|MM_ERROR,
			"fcntl F_SETOWN: %s\n", strerror(errno));
	(void) setjmp(rcvtop);
	(void) sigsetmask(oldmask);
	for (;;) {
		while ((remaining = rcvcnt - (bufp - rcvbuf)) > 0) {
			rcvstate = WRITING;
			n = write(1, bufp, remaining);
			if (n < 0) {
				if (errno != EINTR) {
					prf(MM_ERROR,
			    		    ":10:Write error to terminal: %s",
						errmsg(errno));
					return(-1);
				}
				continue;
			}
			bufp += n;
		}
		bufp = rcvbuf;
		rcvcnt = 0;
		rcvstate = READING;
		rcvcnt = read(rem, rcvbuf, sizeof (rcvbuf));
		if (rcvcnt == 0)
			return (0);
		if (rcvcnt < 0) {
			if (errno == EINTR)
				continue;
			prf(MM_ERROR,
			    ":11:Read error from network: %s", errmsg(errno));
			return (-1);
		}
	}
}

mode(f)
{
	switch (f) {

	case 0:
		if (tcsetattr(0, TCSANOW, &ttyatt) == -1)
                	pfmt(stderr, MM_NOGET|MM_ERROR,
				"tcsetattr: %s\n", strerror(errno));
		break;

	case 1:
                if (tcgetattr(0, &satt) == -1)
                	pfmt(stderr, MM_NOGET|MM_ERROR,
				"tcgetattr: %s\n", strerror(errno));
                satt.c_iflag &= ~(INLCR | ICRNL | IGNCR | IXOFF | IUCLC);

		switch (bits_per_char) {
		case 7:
			satt.c_iflag |= ISTRIP;
		case 8:
			satt.c_iflag &= ~ISTRIP;
		}
		if (litout)
                        satt.c_oflag &= ~OPOST;
                else
                        satt.c_oflag |= OPOST;

                satt.c_oflag &= ~(OLCUC | ONLCR | OCRNL | ONOCR | ONLRET);
                satt.c_lflag &= ~(ICANON | ISIG | ECHO); 
                satt.c_cc[VMIN]  = '\01';
                satt.c_cc[VTIME] = '\0';
      
		if (tcsetattr(0, TCSANOW, &satt) == -1)
                	pfmt(stderr, MM_NOGET|MM_ERROR,
				"tcsetattr: %s\n", strerror(errno));
		break;

	default:
		return;
	}
}

/*  "prf()" has been internationalized. The string require the string to
 *  be output must at least include the message number and optionary.
 *  The catalog name the string is output using <severity>.
 */

/*VARARGS*/
prf(s, f, a1, a2, a3, a4, a5)
long	s;
char *f;
{
	pfmt(stderr, s, f, a1, a2, a3, a4, a5);
	fprintf(stderr, CRLF);
}

lostpeer()
{
	(void) signal(SIGPIPE, SIG_IGN);
	prf(MM_NOSTD,":4:\007Connection closed.");
	done(1);
}

char *
errmsg(errcode)
	int errcode;
{
	extern int sys_nerr;

	if (errcode < 0 || errcode > sys_nerr)
		return(gettxt(":12","Unknown error"));
	else
		return(strerror(errcode));
}
