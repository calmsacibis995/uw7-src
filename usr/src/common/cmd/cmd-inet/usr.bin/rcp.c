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

#ident	"@(#)rcp.c	1.4"

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

/*  "error()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_ERROR>.
 */

/*
 * rcp
 */
#define	_FILE_OFFSET_BITS	64
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/sendv.h>

#include <netinet/in.h>

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>
#include <ctype.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <locale.h>
#include <pfmt.h>
#ifndef SYSV
#include <vfork.h>
#else
#define	rindex	strrchr
#define	index	strchr
#endif SYSV
#include "../usr.sbin/security.h"

int	rem;
char	*colon(), *index(), *rindex(), *malloc(), *strcpy();
int	errs;
void	lostconn();
extern	int	errno;
extern	char	*sys_errlist[];
int	iamremote, targetshouldbedirectory;
int	iamrecursive;
uid_t	myuid;		/* uid of invoker */
int	pflag;
struct	passwd *pwd;
struct	passwd *getpwuid();
uid_t	userid;
int	port;

struct buffer {
	int	cnt;
	char	*buf;
} *allocbuf();

/*VARARGS*/
int	error();

#define	ga()	 	(void) write(rem, "", 1)

/* based on ../usr.sbin/security.c:CLR_WORKPRIVS_NON_ADMIN
 * but is more restrictive since this is not a daemon.
 * ADMIN_WORKPRIVS turns on the maximum priv set when
 * we are in SUM mode, not LPM mode.  Note that no setuid's
 * are done in the SYSV code and the binary can not be
 * setuid root on execution because we need to turn privs
 * on and off with finer granularity.
 *
 * ADMIN_WORKPRIVS is used in conjunction with
 * ../usr.sbin/security.c:ENABLE_WORK_PRIVS.
 */
void
ADMIN_WORKPRIVS()
{
	int	_loc_priv_cmd = CLRPRV,		/* assume clear privs */
		_loc_olderrno;			/* restore the errno */

	uid_t	_loc_id_priv,			/* uid of administrator */
		_loc_uid;			/* current uid */

	_loc_olderrno	= errno;

	if (((_loc_id_priv = secsys(ES_PRVID, 0)) >= 0)
		&& ((_loc_uid = geteuid()) >= 0)
		&& (_loc_uid == _loc_id_priv)) {
			_loc_priv_cmd = SETPRV;	/* know we can maximise privs */
	}

	procprivl(_loc_priv_cmd, pm_work(P_ALLPRIVS), 0);
	errno = _loc_olderrno;
}

main(argc, argv)
	int argc;
	char *argv[];
{
	char *targ, *host, *src;
	char *suser, *tuser, *thost;
	int i;
	char buf[BUFSIZ], cmd[16];
	struct servent *sp;
	extern int optind;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxrcp");
	(void)setlabel("UX:rcp");

	ADMIN_WORKPRIVS();
	sp = getservbyname("shell", "tcp");
	if (sp == NULL) {
		pfmt(stderr, MM_ERROR,
			":1:shell/tcp: unknown service\n");
		exit(1);
	}
	port = sp->s_port;
	pwd = getpwuid(userid = getuid());
	if (pwd == 0) {
		pfmt(stderr, MM_ERROR, ":2:who are you?\n");
		exit(1);
	}
	/*
	 * This is a kludge to allow seteuid to user before touching
	 * files and seteuid root before doing rcmd so we can open
	 * the socket.
	 */
	myuid = getuid();
#ifndef SYSV
	/* not necessary in sysv; saved setuid will allow us to return */
	if (CLR_WORKPRIVS_NON_ADMIN(setruid(0)) < 0) {
		pfmt(stderr,MM_NOGET|MM_ERROR,
			"setruid root: %s\n",strerror(errno));
		exit(1);
	}
	if (CLR_WORKPRIVS_NON_ADMIN(seteuid(myuid)) < 0) {
		pfmt(stderr,MM_NOGET|MM_ERROR,
			"seteuid: %s\n",strerror(errno));
		exit(1);
	}
#endif !SYSV

	while ( (i = getopt(argc, argv, "rpdft")) != -1 ) {
		switch (i) {

		case 'r':
			iamrecursive++;
			break;

		case 'p':		/* preserve mtimes and atimes */
			pflag++;
			break;

		/* The rest of these are not for users. */
		case 'd':
			targetshouldbedirectory = 1;
			break;

		case 'f':		/* "from" */
			iamremote = 1;
			(void) response();
#ifndef SYSV
			if (CLR_WORKPRIVS_NON_ADMIN(setuid(userid)) < 0) {
				pfmt(stderr,MM_NOGET|MM_ERROR,
					"setuid: %s\n",strerror(errno));
				exit(1);
			}
#endif !SYSV
			source((argc - optind), &(argv[optind]));
			exit(errs);

		case 't':		/* "to" */
			iamremote = 1;
#ifndef SYSV
			if (CLR_WORKPRIVS_NON_ADMIN(setuid(userid)) < 0) {
				pfmt(stderr,MM_NOGET|MM_ERROR,
					"setuid: %s\n",strerror(errno));
				exit(1);
			}
#endif !SYSV
			sink((argc - optind), &(argv[optind]));
			exit(errs);

		default:
			usage();
			exit(1);
		}
	}
	if ((argc - optind) < 2) {
		usage();
		exit(1);
	}
	rem = -1;
	if ((argc - optind) > 2)
		targetshouldbedirectory = 1;
	(void) sprintf(cmd, "rcp%s%s%s",
	    iamrecursive ? " -r" : "", pflag ? " -p" : "", 
	    targetshouldbedirectory ? " -d" : "");
	(void) signal(SIGPIPE, (void (*)())lostconn);
	targ = colon(argv[argc - 1]);
	if (targ) {				/* ... to remote */
		*targ++ = 0;
		if (*targ == 0)
			targ = ".";
		thost = index(argv[argc - 1], '@');
		if (thost) {
			*thost++ = 0;
			tuser = argv[argc - 1];
			if (*tuser == '\0')
				tuser = NULL;
			else if (!okname(tuser))
				exit(1);
		} else {
			thost = argv[argc - 1];
			tuser = NULL;
		}
		for ( ; optind < argc - 1; ++optind ) {
			src = colon(argv[optind]);
			if (src) {		/* remote to remote */
				*src++ = 0;
				if (*src == 0)
					src = ".";
				host = index(argv[optind], '@');
				if (host) {
					*host++ = 0;
					suser = argv[optind];
					if (*suser == '\0')
						suser = pwd->pw_name;
					else if (!okname(suser))
						continue;
		(void) sprintf(buf, "rsh %s -l %s -n %s %s '%s%s%s:%s'",
					    host, suser, cmd, src,
					    tuser ? tuser : "",
					    tuser ? "@" : "",
					    thost, targ);
				} else
		(void) sprintf(buf, "rsh %s -n %s %s '%s%s%s:%s'",
					    argv[optind], cmd, src,
					    tuser ? tuser : "",
					    tuser ? "@" : "",
					    thost, targ);
				(void) susystem(buf);
			} else {		/* local to remote */
				if (rem == -1) {
					(void) sprintf(buf, "%s -t %s",
					    cmd, targ);
					host = thost;
#ifndef SYSV
					if (CLR_WORKPRIVS_NON_ADMIN(seteuid(0)) < 0) {
						pfmt(stderr, MM_NOGET|MM_ERROR,
						    "seteuid root: %s\n",strerror(errno));
						exit(1);
					}
#endif !SYSV
					ENABLE_WORK_PRIVS;
					rem = rcmd(&host, port, pwd->pw_name,
					    tuser ? tuser : pwd->pw_name,
					    buf, 0);
#ifdef SYSV
					if ((rem >= 0)&& is_enhancedsecurity()) {
						fd_set_to_my_level(rem);
					}
#endif SYSV
					ADMIN_WORKPRIVS();
#ifndef SYSV
					(void) CLR_WORKPRIVS_NON_ADMIN(seteuid(myuid));
#endif !SYSV
					if (rem < 0)
						exit(1);
					if (response() < 0)
						exit(1);
				}
				source(1, &argv[optind]);
			}
		}
	} else {				/* ... to local */
		if (targetshouldbedirectory)
			verifydir(argv[argc - 1]);
		for ( ; optind < argc - 1; optind++) {
			src = colon(argv[optind]);
			if (src == 0) {		/* local to local */
				(void) sprintf(buf, "/bin/cp%s%s %s %s",
				    iamrecursive ? " -r" : "",
				    pflag ? " -p" : "",
				    argv[optind], argv[argc - 1]);
				(void) susystem(buf);
			} else {		/* remote to local */
				*src++ = 0;
				if (*src == 0)
					src = ".";
				host = index(argv[optind], '@');
				if (host) {
					*host++ = 0;
					suser = argv[optind];
					if (*suser == '\0')
						suser = pwd->pw_name;
					else if (!okname(suser))
						continue;
				} else {
					host = argv[optind];
					suser = pwd->pw_name;
				}
				(void) sprintf(buf, "%s -f %s", cmd, src);
#ifndef SYSV
				if (CLR_WORKPRIVS_NON_ADMIN(seteuid(0)) < 0) {
					pfmt(stderr,MM_NOGET|MM_ERROR,
					   "seteuid root: %s\n",strerror(errno));
					exit(1);
				}
#endif !SYSV
				ENABLE_WORK_PRIVS;
				rem = rcmd(&host, port, pwd->pw_name, suser,
				    buf, 0);
#ifdef SYSV
				if ((rem >= 0)&& is_enhancedsecurity()) {
					fd_set_to_my_level(rem);
				}
#endif SYSV
				ADMIN_WORKPRIVS();
#ifndef SYSV
				(void) CLR_WORKPRIVS_NON_ADMIN(seteuid(myuid));
#endif !SYSV
				if (rem < 0) {
					errs++;
					continue;
				}
				sink(1, &argv[argc-1]);
				(void) close(rem);
				rem = -1;
			}
		}
	}
	exit(errs);
	/* NOTREACHED */
}

verifydir(cp)
	char *cp;
{
	struct stat stb;

	if (stat(cp, &stb) >= 0) {
		if ((stb.st_mode & S_IFMT) == S_IFDIR)
			return;
		errno = ENOTDIR;
	}
	error(":27:%s: %s.\n", cp, strerror(errno));
	exit(1);
}

char *
colon(cp)
	char *cp;
{

	while (*cp) {
		if (*cp == ':')
			return (cp);
		if (*cp == '/')
			return (0);
		cp++;
	}
	return (0);
}

okname(cp0)
	char *cp0;
{
	register char *cp = cp0;
	register int c;

	do {
		c = *cp;
		if (c & 0200)
			goto bad;
		if (!isalpha(c) && !isdigit(c) && c != '_' && c != '-')
			goto bad;
		cp++;
	} while (*cp);
	return (1);
bad:
	pfmt(stderr, MM_ERROR, ":3:invalid user name %s\n", cp0);
	return (0);
}

susystem(s)
	char *s;
{
	pid_t pid, w;
	int status;
	register void (*istat)(), (*qstat)();

	if ((pid = vfork()) == 0) {
#ifndef SYSV
		(void) CLR_WORKPRIVS_NON_ADMIN(setruid(myuid));
#endif /* SYSV */
		execl("/bin/sh", "sh", "-c", s, (char *)0);
		_exit(127);
	}
	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	while ((w = wait(&status)) != pid && w != (pid_t)-1)
		;
	if (w == (pid_t)-1)
		status = -1;
	(void) signal(SIGINT, (void (*)())istat);
	(void) signal(SIGQUIT, (void (*)())qstat);
	return (status);
}

source(argc, argv)
	int argc;
	char **argv;
{
	char *last, *name;
	struct stat stb;
	static struct buffer buffer;
	int x, sizerr, f;
	char buf[BUFSIZ];

	for (x = 0; x < argc; x++) {
		name = argv[x];
		/*
		 * Do the stat before open!
		 * rcp may hang on char device or fifo otherwise.
		 */
		if (stat(name, &stb) < 0)
			goto notreg;
		switch (stb.st_mode&S_IFMT) {

		case S_IFREG:
			break;

		case S_IFDIR:
			if (iamrecursive) {
				rsource(name, &stb);
				continue;
			}
			/* fall into ... */
		default:
			error(":4:%s: not a plain file\n", name);

			continue;
		}
		if ((f = open(name, 0)) < 0) {
notreg:
			error(":26:%s: %s\n", name, strerror(errno));

			continue;
		}
		last = rindex(name, '/');
		if (last == 0)
			last = name;
		else
			last++;
		if (pflag) {
			/*
			 * Make it compatible with possible future
			 * versions expecting microseconds.
			 */
			(void) sprintf(buf, "T%ld 0 %ld 0\n",
			    stb.st_mtime, stb.st_atime);
			(void) write(rem, buf, strlen(buf));
			if (response() < 0) {
				(void) close(f);
				continue;
			}
		}
		(void) sprintf(buf, "C%04o %lld %s\n",
		    stb.st_mode&07777, (long long)stb.st_size, last);
		(void) write(rem, buf, strlen(buf));
		if (response() < 0) {
			(void) close(f);
			continue;
		}
		sizerr = 0;
		if (stb.st_size) {
			struct sendv_iovec sv[1];
			off_t amt;

			sv[0].sendv_flags = SENDV_FD;
			sv[0].sendv_fd = f;
			sv[0].sendv_off = 0;
			sv[0].sendv_len = stb.st_size;
			amt = sendv(rem, sv, 1);
			if (amt != stb.st_size) {
				off_t i;
				struct buffer *bp;

				/* Try to write the expected number of bytes */
				if ((int)(bp = allocbuf(&buffer, f, BUFSIZ))
					!= -1) {
					memset(bp->buf, 0, bp->cnt);
					if (amt < 0)
						amt = 0;
					for (i = amt; i < stb.st_size;
					     i += bp->cnt) {
						amt = bp->cnt;
						if (i + amt > stb.st_size)
							amt = stb.st_size - i;
						(void) write(rem, bp->buf, amt);
					}
				}
				sizerr++;
			}
		}
		(void) close(f);
		if (sizerr == 0)
			ga();
		else
			error(":5:%s: file changed size\n", name);

		(void) response();
	}
}

#include <dirent.h>

rsource(name, statp)
	char *name;
	struct stat *statp;
{
	DIR *d = opendir(name);
	char *last;
	struct dirent *dp;
	char buf[BUFSIZ];
	char *bufv[1];

	if (d == 0) {
		error(":26:%s: %s\n", name, strerror(errno));
		return;
	}
	last = rindex(name, '/');
	if (last == 0)
		last = name;
	else
		last++;
	if (pflag) {
		(void) sprintf(buf, "T%ld 0 %ld 0\n",
		    statp->st_mtime, statp->st_atime);
		(void) write(rem, buf, strlen(buf));
		if (response() < 0) {
			closedir(d);
			return;
		}
	}
	(void) sprintf(buf, "D%04o %d %s\n", statp->st_mode&07777, 0, last);
	(void) write(rem, buf, strlen(buf));
	if (response() < 0) {
		closedir(d);
		return;
	}
	while (dp = readdir(d)) {
		if (dp->d_ino == 0)
			continue;
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		if (strlen(name) + 1 + strlen(dp->d_name) >= BUFSIZ - 1) {
			error(":6:%s/%s: Name too long.\n", name, dp->d_name);
			continue;
		}
		(void) sprintf(buf, "%s/%s", name, dp->d_name);
		bufv[0] = buf;
		source(1, bufv);
	}
	closedir(d);
	(void) write(rem, "E\n", 2);
	(void) response();
}

response()
{
	char resp, c, rbuf[BUFSIZ], *cp = rbuf;

	if (read(rem, &resp, 1) != 1)
		lostconn();
	switch (resp) {

	case 0:				/* ok */
		return (0);

	default:
		*cp++ = resp;
		/* fall into... */
	case 1:				/* error, followed by err msg */
	case 2:				/* fatal error, "" */
		do {
			if (read(rem, &c, 1) != 1)
				lostconn();
			*cp++ = c;
		} while (cp < &rbuf[BUFSIZ] && c != '\n');
		if (iamremote == 0)
			(void) write(2, rbuf, cp - rbuf);
		errs++;
		if (resp == 1)
			return (-1);
		exit(1);
	}
	/*NOTREACHED*/
}

void
lostconn()
{

	if (iamremote == 0)
		pfmt(stderr, MM_ERROR, ":7:lost connection\n");
	exit(1);
}

sink(argc, argv)
	int argc;
	char **argv;
{
	off_t i, j, size;
	char *targ, *whopp, *cp;
	mode_t mode;
	int of, wrerr, exists, first, count, amt;
	struct buffer *bp;
	static struct buffer buffer;
	struct stat stb;
	int targisdir = 0;
	char *myargv[1];
	char cmdbuf[BUFSIZ], nambuf[BUFSIZ];
	int setimes = 0;
	struct timeval tv[2];
#define atime	tv[0]
#define mtime	tv[1]
#define	SCREWUP(str)	{ whopp = str; goto screwup; }

#ifndef SYSV
	(void) CLR_WORKPRIVS_NON_ADMIN(seteuid(pwd->pw_uid));
#endif !SYSV
	if (pflag)
		(void) umask(0);
	if (argc != 1) {
		error(":8:ambiguous target\n");
		exit(1);
	}
	targ = *argv;
	if (stat(targ, &stb) == 0 && (stb.st_mode & S_IFMT) == S_IFDIR)
		targisdir = 1;
	else if (targetshouldbedirectory)
		verifydir(targ);
	ga();
	for (first = 1; ; first = 0) {
		cp = cmdbuf;
		if (read(rem, cp, 1) <= 0)
			return;
		if (*cp++ == '\n')
			SCREWUP(gettxt(":9", "unexpected '\\n'"));
		do {
			if (read(rem, cp, 1) != 1)
			    SCREWUP(gettxt(":10","lost connection"));
		} while (*cp++ != '\n');
		*cp = 0;
		if (cmdbuf[0] == '\01' || cmdbuf[0] == '\02') {
			if (iamremote == 0)
				(void) write(2, cmdbuf+1, strlen(cmdbuf+1));
			if (cmdbuf[0] == '\02')
				exit(1);
			errs++;
			continue;
		}
		*--cp = 0;
		cp = cmdbuf;
		if (*cp == 'E') {
			ga();
			return;
		}

#define getnum(t) (t) = 0; while (isdigit(*cp)) (t) = (t) * 10 + (*cp++ - '0');
		if (*cp == 'T') {
			setimes++;
			cp++;
			getnum(mtime.tv_sec);
			if (*cp++ != ' ')
			    SCREWUP(gettxt(":11","mtime.sec not delimited"));
			getnum(mtime.tv_usec);
			if (*cp++ != ' ')
			    SCREWUP(gettxt(":12","mtime.usec not delimited"));
			getnum(atime.tv_sec);
			if (*cp++ != ' ')
			    SCREWUP(gettxt(":13","atime.sec not delimited"));
			getnum(atime.tv_usec);
			if (*cp++ != '\0')
			    SCREWUP(gettxt(":14","atime.usec not delimited"));
			ga();
			continue;
		}
		if (*cp != 'C' && *cp != 'D') {
			/*
			 * Check for the case "rcp remote:foo\* local:bar".
			 * In this case, the line "No match." can be returned
			 * by the shell before the rcp command on the remote is
			 * executed so the ^Aerror_message convention isn't
			 * followed.
			 */
			if (first) {
				error(":25:%s\n", cp);
				exit(1);
			}
			SCREWUP(gettxt(":15","expected control record"));
		}
		cp++;
		mode = 0;
		for (; cp < cmdbuf+5; cp++) {
			if (*cp < '0' || *cp > '7')
				SCREWUP(gettxt(":16","bad mode"));
			mode = (mode << 3) | (*cp - '0');
		}
		if (*cp++ != ' ')
			SCREWUP(gettxt(":17","mode not delimited"));
		size = 0;
		while (isdigit(*cp))
			size = size * 10 + (*cp++ - '0');
		if (*cp++ != ' ')
			SCREWUP(gettxt(":18","size not delimited"));
		if (targisdir)
			(void) sprintf(nambuf, "%s%s%s", targ,
			    *targ ? "/" : "", cp);
		else
			(void) strcpy(nambuf, targ);
		exists = stat(nambuf, &stb) == 0;
		if (cmdbuf[0] == 'D') {
			short chmodflag = 0;

			if (exists) {
				if ((stb.st_mode&S_IFMT) != S_IFDIR) {
					errno = ENOTDIR;
					goto bad;
				}
				if (pflag) {
					if ((mode|0700) != mode)
						chmodflag++;
					(void) chmod(nambuf, mode|0700);
				}
			}
			else {
				if ((mode|0700) != mode)
					chmodflag++;
				if (mkdir(nambuf, mode|0700) < 0)
					goto bad;
			}
			myargv[0] = nambuf;
			sink(1, myargv);
			if (chmodflag)
				(void) chmod(nambuf, mode);
			if (setimes) {
				setimes = 0;
				if (utimes(nambuf, tv) < 0)
				    error(":19:cannot set times on %s: %s\n",
					nambuf, strerror(errno));
			}
			continue;
		}
		if ((of = open(nambuf, O_TRUNC|O_WRONLY|O_CREAT, mode)) < 0) {
	bad:
			error(":26:%s: %s\n", nambuf, strerror(errno));
			continue;
		}
		if (exists && pflag)
			(void) fchmod(of, mode);
		ga();
#define	NETBUFSIZ	4096
		if ((int)(bp = allocbuf(&buffer, of, NETBUFSIZ)) == -1) {
			(void) close(of);
			continue;
		}
		cp = bp->buf;
		count = 0;
		wrerr = 0;
		for (i = 0; i < size; i += NETBUFSIZ) {
			amt = NETBUFSIZ;
			if (i + amt > size)
				amt = size - i;
			count += amt;
			do {
				j = read(rem, cp, amt);
				if (j <= 0) {
					if (j == 0)
					    error(":20:dropped connection");
					else
					    error(":25:%s\n",
						strerror(errno));
					exit(1);
				}
				amt -= j;
				cp += j;
			} while (amt > 0);
			if (count == bp->cnt) {
				if (wrerr == 0 &&
				    write(of, bp->buf, count) != count)
					wrerr++;
				count = 0;
				cp = bp->buf;
			}
		}
		if (count != 0 && wrerr == 0 &&
		    write(of, bp->buf, count) != count)
			wrerr++;
		(void) close(of);
		(void) response();
		if (setimes) {
			setimes = 0;
			if (utimes(nambuf, tv) < 0)
				error(":19:cannot set times on %s: %s\n",
				    nambuf, strerror(errno));
		}				   
		if (wrerr) {
			error(":26:%s: %s\n", nambuf, strerror(errno));
			(void)unlink(nambuf);
		}
		else
			ga();
	}
screwup:
	error(":21:protocol screwup: %s\n", whopp);
	exit(1);
}

struct buffer *
allocbuf(bp, fd, blksize)
	struct buffer *bp;
	int fd, blksize;
{
	struct stat stb;
	int size;

	if (fstat(fd, &stb) < 0) {
		error(":22:fstat: %s\n", strerror(errno));
		return ((struct buffer *)-1);
	}
#ifndef roundup
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))
#endif /* !roundup */
	size = roundup(stb.st_blksize, blksize);
	if (size == 0)
		size = blksize;
	if (bp->cnt < size) {
		if (bp->buf != 0)
			free(bp->buf);
		bp->buf = (char *)malloc((unsigned) size);
		if (bp->buf == 0) {
			error(":23:malloc: out of memory\n");
			return ((struct buffer *)-1);
		}
	}
	bp->cnt = size;
	return (bp);
}

/*  "error()" has been internationalized. The string to
 *  be output must at least include the message number and optionally.
 *  The string is output using <MM_ERROR>.
 */

/*VARARGS1*/
error(fmt, a1, a2, a3, a4, a5)
	char *fmt;
	int a1, a2, a3, a4, a5;
{
	char buf[BUFSIZ], *cp = buf;
	char msg[BUFSIZ], *mp, *np = msg;

	errs++;
	for(mp = fmt; *mp != ':'; *np++ = *mp++);
	for(*np++ = *mp++; *mp != ':'; *np++ = *mp++);
	*np = '\0';
	++mp;
	*cp++ = '\001';
	strcpy(cp, "rcp: ");
	cp += 5;
	(void) sprintf(cp, gettxt(msg, mp), a1, a2, a3, a4, a5);
	(void) write(rem, buf, strlen(buf));
	if (iamremote == 0)
		pfmt(stderr, MM_ERROR, fmt, a1, a2, a3, a4, a5);
}

usage()
{
	pfmt(stderr, MM_ACTION,
		":24:Usage: rcp [-p] f1 f2; or: rcp [-rp] f1 ... fn d2\n");
}


#include <sys/utime.h>
/*
 * The error code is set by the utime() system call
 */ 
utimes(file,tvp)
        char    *file;
        struct  timeval *tvp;
{
        struct  utimbuf ut;
        int     error;

        if (tvp) {
                ut.actime = tvp->tv_sec;
                ut.modtime = (++tvp)->tv_sec;
                error = utime(file, &ut);
        } else {
                error = utime(file, 0);
        }
        return error;
}
