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

#ident	"@(#)ftp.c	1.4"
#ident	"$Header$"

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *	Copyrighted as an unpublished work.
 *      (c) Copyright 1990 INTERACTIVE Systems Corporation
 *      All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */
/*
 * Copyright (c) 1985, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @(#)ftp.c	5.28 (Berkeley) 4/20/89
 */


#include <sys/param.h>
#include <sys/types.h>
#include <pfmt.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/file.h>

#include <netinet/in.h>
#include <arpa/ftp.h>
#include <arpa/telnet.h>

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <pwd.h>
#include <varargs.h>
#include <unistd.h>

#include "ftp_var.h"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 8192
#endif
#define SENT	0
#define RECEIVED 1

struct	sockaddr_in hisctladdr;
struct	sockaddr_in data_addr;
int	data = -1;
int	abrtflag = 0;
int	ptflag = 0;
int	connected;
int	allbinary;
struct	sockaddr_in myctladdr;
struct passwd *getpwuid();
off_t	restart_point = 0;
extern char *strerror();

extern void (*Signal())();

FILE	*cin, *cout;
FILE	*dataconn();

char *
hookup(host, port)
	char *host;
	u_short port;
{
	register struct hostent *hp = 0;
	int s;
	size_t len;
	static char hostnamebuf[80];

	bzero((char *)&hisctladdr, sizeof (hisctladdr));
	hisctladdr.sin_addr.s_addr = inet_addr(host);
	if (hisctladdr.sin_addr.s_addr != -1) {
		hisctladdr.sin_family = AF_INET;
		(void) strncpy(hostnamebuf, host, sizeof(hostnamebuf));
	} else {
		hp = gethostbyname(host);
		if (hp == NULL) {
			pfmt(stderr, MM_ERROR, ":121: ");
			herror(host);
			code = -1;
			return((char *) 0);
		}
		hisctladdr.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr_list[0],
		    (caddr_t)&hisctladdr.sin_addr, hp->h_length);
		(void) strncpy(hostnamebuf, hp->h_name, sizeof(hostnamebuf));
	}
	hostname = hostnamebuf;
	s = socket(hisctladdr.sin_family, SOCK_STREAM, 0);
	if (s < 0) {
		pfmt(stderr, MM_ERROR, ":151:socket: %s\n", strerror(errno));
		code = -1;
		return (0);
	}
	hisctladdr.sin_port = port;
	while (connect(s, (struct sockaddr *) &hisctladdr, sizeof (hisctladdr)) < 0) {
		if (hp && hp->h_addr_list[1]) {
			int oerrno = errno;

			pfmt(stderr, MM_ERROR, ":122:connect to address %s: ",
				inet_ntoa(hisctladdr.sin_addr));
			errno = oerrno;
			fprintf(stderr, strerror(errno));
			hp->h_addr_list++;
			bcopy(hp->h_addr_list[0],
			     (caddr_t)&hisctladdr.sin_addr, hp->h_length);
			pfmt(stdout, MM_NOSTD, ":123:Trying %s...\n",
				inet_ntoa(hisctladdr.sin_addr));
			(void) close(s);
			s = socket(hisctladdr.sin_family, SOCK_STREAM, 0);
			if (s < 0) {
				pfmt(stderr, MM_ERROR, ":151:socket: %s\n",
						strerror(errno));
				code = -1;
				return (0);
			}
			continue;
		}
		pfmt(stderr, MM_ERROR, ":152:connect: %s\n", strerror(errno));
		code = -1;
		goto bad;
	}
	len = sizeof (myctladdr);
	if (getsockname(s, (struct sockaddr *)&myctladdr, &len) < 0) {
		pfmt(stderr, MM_ERROR, ":153:getsockname: %s\n", strerror(errno));
		code = -1;
		goto bad;
	}
	cin = fdopen(s, "r");
	cout = fdopen(dup(s), "w");
	if (cin == NULL || cout == NULL) {
		pfmt(stderr, MM_ERROR, ":124:fdopen failed: %s\n",
			strerror(errno));
		if (cin)
			(void) fclose(cin);
		if (cout)
			(void) fclose(cout);
		code = -1;
		goto bad;
	}
	if (verbose)
		pfmt(stdout, MM_NOSTD, ":38:Connected to %s.\n", hostname);
	if (getreply(0) > 2) { 	/* read startup message from server */
		if (cin)
			(void) fclose(cin);
		if (cout)
			(void) fclose(cout);
		code = -1;
		goto bad;
	}
#ifdef SO_OOBINLINE
	{
	int on = 1;

	if (setsockopt(s, SOL_SOCKET, SO_OOBINLINE, (char *) &on, sizeof(on))
		< 0 && debug) {
			pfmt(stderr, MM_ERROR, ":154:setsockopt: %s\n",
				strerror(errno));
		}
	}
#endif /* SO_OOBINLINE */

	return (hostname);
bad:
	(void) close(s);
	return ((char *)0);
}

login(host)
	char *host;
{
	char tmp[80];
	char *user, *pass, *acct, *getlogin(), *mygetpass();
	int n, aflag = 0;

	tmp[0] = '\0';
	user = acct = 0;
	pass = 0;
	if (ruserpass(host, &user, &pass, &acct) < 0) {
		code = -1;
		return(0);
	}
	while (user == NULL) {
		char *myname = getlogin();

		if (myname == NULL) {
			struct passwd *pp = getpwuid(getuid());

			if (pp != NULL)
				myname = pp->pw_name;
		}
		if (myname)
			pfmt(stdout, MM_NOSTD, ":125:Name (%s:%s): ", host, myname);
		else
			pfmt(stdout, MM_NOSTD, ":126:Name (%s): ", host);
		if (fgets(tmp, sizeof(tmp) - 1, stdin) != NULL)
			tmp[strlen(tmp) - 1] = '\0';
		else
			tmp[0] = '\0';
		if (tmp[0] == '\0')
			user = myname;
		else
			user = tmp;
	}
	n = command("USER %s", user);
	if (n == CONTINUE) {
		if (pass == NULL)
			pass = mygetpass("Password:");
		n = command("PASS %s", pass);
	}
	if (n == CONTINUE) {
		aflag++;
		if (acct == NULL)
			acct = mygetpass("Account:");
		n = command("ACCT %s", acct);
	}
	if (n != COMPLETE) {
		pfmt(stderr, MM_ERROR, ":77:Login failed.\n");
		return (0);
	}
	if (!aflag && acct != NULL)
		(void) command("ACCT %s", acct);
	if (proxy)
		return(1);
	for (n = 0; n < macnum; ++n) {
		if (!strcmp("init", macros[n].mac_name)) {
			(void) strcpy(line, "$init");
			makeargv();
			domacro(margc, margv);
			break;
		}
	}
	return (1);
}

void
cmdabort()
{
	extern jmp_buf ptabort;

	pfmt(stdout, MM_NOSTD, ":34:\n");
	(void) fflush(stdout);
	abrtflag++;
	if (ptflag) {
		sigrelse(SIGINT);
		longjmp(ptabort,1);
	}
}

/*VARARGS1*/
command(fmt, va_alist)
	char *fmt;
va_dcl
{
	va_list args;
	int r;
	void (*oldintr)(), cmdabort();

	abrtflag = 0;
	if (debug) {
		pfmt(stdout, MM_NOSTD, ":127:---> ");
		va_start(args);
		vfprintf(stdout, fmt, args);
		va_end(args);
		pfmt(stdout, MM_NOSTD, ":34:\n");
		(void) fflush(stdout);
	}
	if (cout == NULL) {
		pfmt(stderr, MM_ERROR,
			":155:No control connection for command: %s\n", strerror(errno));
		code = -1;
		return (0);
	}
	oldintr = Signal(SIGINT,cmdabort);
	va_start(args);
	vfprintf(cout, fmt, args);
	va_end(args);
	fprintf(cout, "\r\n");
	(void) fflush(cout);
	cpend = 1;
	r = getreply(!strcmp(fmt, "QUIT"));
	if (abrtflag && oldintr != SIG_IGN)
		(*oldintr)();
	(void) Signal(SIGINT, oldintr);
	return(r);
}

char reply_string[BUFSIZ];		/* last line of previous reply */

#include <ctype.h>

getreply(expecteof)
	int expecteof;
{
	register int c, n;
	register int dig;
	register char *cp;
	int originalcode = 0, continuation = 0;
	void (*oldintr)(), cmdabort();
	int pflag = 0;
	char *pt = pasv;

	oldintr = Signal(SIGINT,cmdabort);
	for (;;) {
		dig = n = code = 0;
		cp = reply_string;
		while ((c = getc(cin)) != '\n') {
			if (c == IAC) {     /* handle telnet commands */
				switch (c = getc(cin)) {
				case WILL:
				case WONT:
					c = getc(cin);
					fprintf(cout, "%c%c%c",IAC,DONT,c);
					(void) fflush(cout);
					break;
				case DO:
				case DONT:
					c = getc(cin);
					fprintf(cout, "%c%c%c",IAC,WONT,c);
					(void) fflush(cout);
					break;
				default:
					break;
				}
				continue;
			}
			dig++;
			if (c == EOF) {
				if (expecteof) {
					(void) Signal(SIGINT,oldintr);
					code = 221;
					return (0);
				}
				lostpeer();
				if (verbose)
					pfmt(stdout, MM_NOSTD, ":128:421 Service not available, remote server has closed connection\n");
				else
					pfmt(stdout, MM_NOSTD, ":129:Lost connection.\n");
				(void) fflush(stdout);
				code = 421;
				return(4);
			}
			if (c != '\r' && (verbose > 0 ||
			    (verbose > -1 && n == '5' && dig > 4) ||
			    (verbose == -2 && n == '5' && dig > 4 && code != 500))) {
				if (proxflag &&
				   (dig == 1 || dig == 5 && verbose == 0))
					pfmt(stdout, MM_NOSTD, ":130:%s:",hostname);
				(void) putchar(c);
			}
			if (dig < 4 && isdigit(c))
				code = code * 10 + (c - '0');
			if (!pflag && code == 227)
				pflag = 1;
			if (dig > 4 && pflag == 1 && isdigit(c))
				pflag = 2;
			if (pflag == 2) {
				if (c != '\r' && c != ')')
					*pt++ = c;
				else {
					*pt = '\0';
					pflag = 3;
				}
			}
			if (dig == 4 && c == '-') {
				if (continuation)
					code = 0;
				continuation++;
			}
			if (n == 0)
				n = c;
			if (cp < &reply_string[sizeof(reply_string) - 1])
				*cp++ = c;
		}
		if (verbose > 0 || verbose > -1 && n == '5' || verbose == -2 && n == '5' && code != 500) {
			(void) putchar(c);
			(void) fflush (stdout);
		}
		if (continuation && code != originalcode) {
			if (originalcode == 0)
				originalcode = code;
			continue;
		}
		*cp = '\0';
		if (n != '1')
			cpend = 0;
		(void) Signal(SIGINT,oldintr);
		if (code == 421 || originalcode == 421)
			lostpeer();
		if (abrtflag && oldintr != cmdabort && oldintr != SIG_IGN)
			(*oldintr)();
		return (n - '0');
	}
}

empty(mask, sec)
	struct fd_set *mask;
	int sec;
{
	struct timeval t;

	t.tv_sec = (long) sec;
	t.tv_usec = 0;
	return(select(32, mask, (struct fd_set *) 0, (struct fd_set *) 0, &t));
}

jmp_buf	sendabort;

abortsend()
{

	mflag = 0;
	abrtflag = 0;
	pfmt(stdout, MM_NOSTD, ":131:\nsend aborted\n");
	(void) fflush(stdout);
	longjmp(sendabort, 1);
}

#define HASHBYTES 1024

sendrequest(cmd, local, remote, printnames)
	char *cmd, *local, *remote;
	int printnames;
{
	FILE *fin, *dout = 0, *popen();
	int (*closefunc)(), pclose(), fclose();
	void (*oldintr)(), (*oldintp)();
	int abortsend();
	char buf[BUFFER_SIZE], *bufp;
	long bytes = 0, hashbytes = HASHBYTES;
	register int c, d;
	struct stat st;
	struct timeval start, stop;
	char *mode;

	if (verbose && printnames) {
		if (local && *local != '-')
			pfmt(stdout, MM_NOSTD, ":132:local: %s ", local);
		if (remote)
			pfmt(stdout, MM_NOSTD, ":133:remote: %s\n", remote);
	}
	if (proxy) {
		proxtrans(cmd, local, remote);
		return;
	}
	closefunc = NULL;
	oldintr = NULL;
	oldintp = NULL;
	mode = "w";
	if (setjmp(sendabort)) {
		while (cpend) {
			(void) getreply(0);
		}
		if (data >= 0) {
			(void) close(data);
			data = -1;
		}
		if (oldintr)
			(void) Signal(SIGINT,oldintr);
		if (oldintp)
			(void) Signal(SIGPIPE,oldintp);
		code = -1;
		return;
	}
	oldintr = Signal(SIGINT, abortsend);
	if (strcmp(local, "-") == 0)
		fin = stdin;
	else if (*local == '|') {
		oldintp = Signal(SIGPIPE,SIG_IGN);
		fin = popen(local + 1, "r");
		if (fin == NULL) {
			pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n", local + 1,
				strerror(errno));
			(void) Signal(SIGINT, oldintr);
			(void) Signal(SIGPIPE, oldintp);
			code = -1;
			return;
		}
		closefunc = pclose;
	} else {
		fin = fopen(local, "r");
		if (fin == NULL) {
			pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n", local,
				strerror(errno));
			(void) Signal(SIGINT, oldintr);
			code = -1;
			return;
		}
		closefunc = fclose;
		if (fstat(fileno(fin), &st) < 0 ||
		    (st.st_mode&S_IFMT) != S_IFREG) {
			pfmt(stdout, MM_NOSTD, ":134:%s: not a plain file.\n", local);
			(void) Signal(SIGINT, oldintr);
			fclose(fin);
			code = -1;
			return;
		}
	}
	if (initconn()) {
		(void) Signal(SIGINT, oldintr);
		if (oldintp)
			(void) Signal(SIGPIPE, oldintp);
		code = -1;
		if (closefunc != NULL)
			(*closefunc)(fin);
		return;
	}
	if (setjmp(sendabort))
		goto abort;

	if (restart_point &&
	    (strcmp(cmd, "STOR") == 0 || strcmp(cmd, "APPE") == 0)) {
		if (fseek(fin, (long) restart_point, 0) < 0) {
			pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n", local,
				strerror(errno));
			restart_point = 0;
			if (closefunc != NULL)
				(*closefunc)(fin);
			return;
		}
		if (command("REST %ld", (long) restart_point)
			!= CONTINUE) {
			restart_point = 0;
			if (closefunc != NULL)
				(*closefunc)(fin);
			return;
		}
		restart_point = 0;
		mode = "r+w";
	}
	if (remote) {
		if (command("%s %s", cmd, remote) != PRELIM) {
			(void) Signal(SIGINT, oldintr);
			if (oldintp)
				(void) Signal(SIGPIPE, oldintp);
			if (closefunc != NULL)
				(*closefunc)(fin);
			return;
		}
	} else
		if (command("%s", cmd) != PRELIM) {
			(void) Signal(SIGINT, oldintr);
			if (oldintp)
				(void) Signal(SIGPIPE, oldintp);
			if (closefunc != NULL)
				(*closefunc)(fin);
			return;
		}
	dout = dataconn(mode);
	if (dout == NULL)
		goto abort;
	(void) gettimeofday(&start, (struct timezone *)0);
	oldintp = Signal(SIGPIPE, SIG_IGN);
	switch (type) {

	case TYPE_I:
	case TYPE_L:
		errno = d = 0;
		while ((c = read(fileno(fin), buf, sizeof (buf))) > 0) {
			bytes += c;
			for (bufp = buf; c > 0; c -= d, bufp += d)
				if ((d = write(fileno(dout), bufp, c)) <= 0)
					break;
			if (hash) {
				while (hashbytes <= bytes) {
					(void) putchar('#');
					hashbytes += HASHBYTES;
				}
				(void) fflush(stdout);
			}
		}
		if (hash) {
			if (bytes % HASHBYTES)
				(void) putchar('#');
			if (bytes > 0)
				(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (c < 0)
			pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n", local,
				strerror(errno));
		if (d <= 0) {
			if (d == 0)
				pfmt(stderr, MM_ERROR, ":135:netout: write returned 0?\n");
			else if (errno != EPIPE) 
				pfmt(stderr, MM_ERROR, ":156:netout: %s\n",
					strerror(errno));
			bytes = -1;
		}
		break;

	case TYPE_A:
		while ((c = getc(fin)) != EOF) {
			if (c == '\n') {
				if (hash) {
					while (hashbytes <= bytes) {
						(void) putchar('#');
						hashbytes += HASHBYTES;
					}
					(void) fflush(stdout);
				}
				if (ferror(dout))
					break;
				(void) putc('\r', dout);
				bytes++;
			}
			(void) putc(c, dout);
			bytes++;
	/*		if (c == '\r') {			  	*/
	/*		(void)	putc('\0', dout);  /* this violates rfc */
	/*			bytes++;				*/
	/*		}                          			*/	
		}
		if (hash) {
			if (bytes % HASHBYTES)
				(void) putchar('#');
			if (bytes > 0)
				(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (ferror(fin))
			pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n", local,
				strerror(errno));
		if (ferror(dout)) {
			if (errno != EPIPE)
				pfmt(stderr, MM_ERROR, "191:netout: %s\n",
					strerror(errno));
			bytes = -1;
		}
		break;
	}
	(void) gettimeofday(&stop, (struct timezone *)0);
	if (closefunc != NULL)
		(*closefunc)(fin);
	(void) fclose(dout);
	(void) getreply(0);
	(void) Signal(SIGINT, oldintr);
	if (oldintp)
		(void) Signal(SIGPIPE, oldintp);
	if (bytes > 0)
		ptransfer(SENT, bytes, &start, &stop);
	return;
abort:
	(void) gettimeofday(&stop, (struct timezone *)0);
	(void) Signal(SIGINT, oldintr);
	if (oldintp)
		(void) Signal(SIGPIPE, oldintp);
	if (!cpend) {
		code = -1;
		return;
	}
	if (data >= 0) {
		(void) close(data);
		data = -1;
	}
	if (dout)
		(void) fclose(dout);
	(void) getreply(0);
	code = -1;
	if (closefunc != NULL && fin != NULL)
		(*closefunc)(fin);
	if (bytes > 0)
		ptransfer(SENT, bytes, &start, &stop);
}

jmp_buf	recvabort;

abortrecv()
{

	mflag = 0;
	abrtflag = 0;
	pfmt(stdout, MM_NOSTD, ":34:\n");
	(void) fflush(stdout);
	sigrelse(SIGINT);
	longjmp(recvabort, 1);
}

recvrequest(cmd, local, remote, mode, printnames)
	char *cmd, *local, *remote, *mode;
{
	FILE *fout, *din = 0, *popen();
	int (*closefunc)(), pclose(), fclose();
	void (*oldintr)(), (*oldintp)(); 
	int abortrecv(), oldverbose, oldtype = 0, is_retr, tcrflag, nfnd;
	char *bufp, *gunique(), msg;
	static char *buf;
	static int bufsize;
	long bytes = 0, hashbytes = HASHBYTES;
	struct fd_set mask;
	register int c, d;
	struct timeval start, stop;
	struct stat st;
	extern char *malloc();

	is_retr = strcmp(cmd, "RETR") == 0;
	if (is_retr && verbose && printnames) {
		if (local && *local != '-')
			pfmt(stdout, MM_NOSTD, ":132:local: %s ", local);
		if (remote)
			pfmt(stdout, MM_NOSTD, ":133:remote: %s\n", remote);
	}
	if (proxy && is_retr) {
		proxtrans(cmd, local, remote);
		return;
	}
	closefunc = NULL;
	oldintr = NULL;
	oldintp = NULL;
	tcrflag = !crflag && is_retr;
	if (setjmp(recvabort)) {
		while (cpend) {
			(void) getreply(0);
		}
		if (data >= 0) {
			(void) close(data);
			data = -1;
		}
		if (oldintr)
			(void) Signal(SIGINT, oldintr);
		code = -1;
		return;
	}
	oldintr = Signal(SIGINT, abortrecv);
	if (strcmp(local, "-") && *local != '|') {
		if (access(local, 2) < 0) {

			if (errno != ENOENT && errno != EACCES) {
				pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n", local,
					strerror(errno));
				(void) Signal(SIGINT, oldintr);
				code = -1;
				return;
			}
			d = check_dir_access(local);
			if (d < 0) {
				pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n", local,
					strerror(errno));
				(void) Signal(SIGINT, oldintr);
				code = -1;
				return;
			}
			if (!runique && errno == EACCES &&
			    chmod(local, 0600) < 0) {
				pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n", local,
					strerror(errno));
				(void) Signal(SIGINT, oldintr);
				code = -1;
				return;
			}
			if (runique && errno == EACCES &&
			   (local = gunique(local)) == NULL) {
				(void) Signal(SIGINT, oldintr);
				code = -1;
				return;
			}
		}
		else if (runique && (local = gunique(local)) == NULL) {
			(void) Signal(SIGINT, oldintr);
			code = -1;
			return;
		}
	}
	if (initconn()) {
		(void) Signal(SIGINT, oldintr);
		code = -1;
		return;
	}
	if (setjmp(recvabort))
		goto abort;
	if (!is_retr) {
		if (type != TYPE_A && (allbinary == 0 || type != TYPE_I)) {
			oldtype = type;
			oldverbose = verbose;
			if (!debug)
				verbose = 0;
			setascii();
			verbose = oldverbose;
		}
	} else if (restart_point) {
		if (command("REST %ld", (long) restart_point) != CONTINUE)
			return;
	}
	if (remote) {
		if (command("%s %s", cmd, remote) != PRELIM) {
			(void) Signal(SIGINT, oldintr);
			if (oldtype) {
				if (!debug)
					verbose = 0;
				switch (oldtype) {
					case TYPE_I:
						setbinary();
						break;
					case TYPE_E:
						setebcdic();
						break;
					case TYPE_L:
						settenex();
						break;
				}
				verbose = oldverbose;
			}
			return;
		}
	} else {
		if (command("%s", cmd) != PRELIM) {
			(void) Signal(SIGINT, oldintr);
			if (oldtype) {
				if (!debug)
					verbose = 0;
				switch (oldtype) {
					case TYPE_I:
						setbinary();
						break;
					case TYPE_E:
						setebcdic();
						break;
					case TYPE_L:
						settenex();
						break;
				}
				verbose = oldverbose;
			}
			return;
		}
	}
	din = dataconn("r");
	if (din == NULL)
		goto abort;
	if (strcmp(local, "-") == 0)
		fout = stdout;
	else if (*local == '|') {
		oldintp = Signal(SIGPIPE, SIG_IGN);
		fout = popen(local + 1, "w");
		if (fout == NULL) {
			pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n", local + 1,
				strerror(errno));
			goto abort;
		}
		closefunc = pclose;
	} else {
		fout = fopen(local, mode);
		if (fout == NULL) {
			pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n", local,
				strerror(errno));
			goto abort;
		}
		closefunc = fclose;
	}

	/* BSD FFS: The Fast File System stores files based on
	 * a computed "optimal" block size.  FTP tries to take
	 * advantage of this block size by stat()ing the file
	 * (to find out the block size), and then doing operations
	 * with this block size in mind.  Since System V does not
	 * support the fast file system, and doesn't support the
	 * st_blksize field in struct stat, FTP will do operations
	 * of size BUFSIZ.
	 *
	 * SVR4 supports this -- #ifdef BSD removed
	 */
	if (fstat(fileno(fout), &st) < 0 || st.st_blksize == 0)
		st.st_blksize = BUFSIZ;
	if (st.st_blksize > bufsize) {
		if (buf)
			(void) free(buf);
		buf = malloc(st.st_blksize);
		if (buf == NULL) {
			pfmt(stderr, MM_ERROR, ":157:malloc: %s\n",
				strerror(errno));
			bufsize = 0;
			goto abort;
		}
		bufsize = st.st_blksize;
	}

	(void) gettimeofday(&start, (struct timezone *)0);
	switch (type) {

	case TYPE_I:
	case TYPE_L:
		if (restart_point &&
		    lseek(fileno(fout), (long) restart_point, SEEK_SET) < 0) {
			pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n", local,
				strerror(errno));
			if (closefunc != NULL)
				(*closefunc)(fout);
			return;
		}
		errno = d = 0;
		while ((c = read(fileno(din), buf, bufsize)) > 0) {
			if ((d = write(fileno(fout), buf, c)) != c)
				break;
			bytes += c;
			if (hash) {
				while (hashbytes <= bytes) {
					(void) putchar('#');
					hashbytes += HASHBYTES;
				}
				(void) fflush(stdout);
			}
		}
		if (hash) {
			if (bytes % HASHBYTES)
				(void) putchar('#');
			if (bytes > 0)
				(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (c < 0) {
			if (errno != EPIPE)
				pfmt(stderr, MM_ERROR, ":158:netin: %s\n",
					strerror(errno));
			bytes = -1;
		}
		if (d < c) {
			if (d < 0)
				pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n", local,
					strerror(errno));
			else
				pfmt(stderr, MM_ERROR, ":136:%s: short write\n", local);
		}
		break;

	case TYPE_A:
		if (restart_point) {
			register int i, n, c;

			if (fseek(fout, 0L, SEEK_SET) < 0)
				goto done;
			n = restart_point;
			i = 0;
			while (i++ < n) {
				if ((c=getc(fout)) == EOF)
					goto done;
				if (c == '\n')
					i++;
			}
			if (fseek(fout, 0L, SEEK_CUR) < 0) {
done:
				pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n", local,
					strerror(errno));
				if (closefunc != NULL)
					(*closefunc)(fout);
				return;
			}
		}
		while ((c = getc(din)) != EOF) {
			while (c == '\r') {
				if (hash) {
					while (hashbytes <= bytes) {
						(void) putchar('#');
						hashbytes += HASHBYTES;
					}
					(void) fflush(stdout);
				}
				bytes++;
				if ((c = getc(din)) != '\n' || tcrflag) {
					if (ferror(fout))
						goto brk2;
					(void) putc('\r', fout);
					if (c == '\0') {
						bytes++;
						goto contin2;
					}
					if (c == EOF)
						goto contin2;
				}
			}
			(void) putc(c, fout);
			bytes++;
	contin2:	;
		}
brk2:
		if (hash) {
			if (bytes % HASHBYTES)
				(void) putchar('#');
			if (bytes > 0)
				(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (ferror(din)) {
			if (errno != EPIPE)
				pfmt(stderr, MM_ERROR, ":158:netin: %s\n",
					strerror(errno));
			bytes = -1;
		}
		if (ferror(fout))
			pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n", local,
				strerror(errno));
		break;
	}
	if (closefunc != NULL)
		(*closefunc)(fout);
	(void) Signal(SIGINT, oldintr);
	if (oldintp)
		(void) Signal(SIGPIPE, oldintp);
	(void) gettimeofday(&stop, (struct timezone *)0);
	(void) fclose(din);
	(void) getreply(0);
	if (bytes > 0 && is_retr)
		ptransfer(RECEIVED, bytes, &start, &stop);
	if (oldtype) {
		if (!debug)
			verbose = 0;
		switch (oldtype) {
			case TYPE_I:
				setbinary();
				break;
			case TYPE_E:
				setebcdic();
				break;
			case TYPE_L:
				settenex();
				break;
		}
		verbose = oldverbose;
	}
	return;
abort:

/* abort using RFC959 recommended IP,SYNC sequence  */

	(void) gettimeofday(&stop, (struct timezone *)0);
	if (oldintp)
		(void) Signal(SIGPIPE, oldintr);
	(void) Signal(SIGINT,SIG_IGN);
	if (!cpend) {
		code = -1;
		(void) Signal(SIGINT,oldintr);
		return;
	}

	fprintf(cout,"%c%c",IAC,IP);
	(void) fflush(cout); 
	msg = IAC;
/* send IAC in urgent mode instead of DM because UNIX places oob mark */
/* after urgent byte rather than before as now is protocol            */
	if (send(fileno(cout),&msg,1,MSG_OOB) != 1) {
		pfmt(stderr, MM_ERROR, ":159:abort: %s\n", strerror(errno));
	}
	pfmt(cout, MM_NOSTD, ":137:%cABOR\r\n",DM);
	(void) fflush(cout);
	FD_ZERO(&mask);
	FD_SET(fileno(cin), &mask);
	if (din) { 
		FD_SET(fileno(din), &mask);
	}
	if ((nfnd = empty(&mask,10)) <= 0) {
		if (nfnd < 0) {
			pfmt(stderr, MM_ERROR, ":159:abort: %s\n", strerror(errno));
		}
		code = -1;
		lostpeer();
	}
	if (din && FD_ISSET(fileno(din), &mask)) {
		while ((c = read(fileno(din), buf, bufsize)) > 0)
			;
	}
	if ((c = getreply(0)) == ERROR && code == 552) { /* needed for nic style abort */
		if (data >= 0) {
			(void) close(data);
			data = -1;
		}
		(void) getreply(0);
	}
	(void) getreply(0);
	if (oldtype) {
		if (!debug)
			verbose = 0;
		switch (oldtype) {
			case TYPE_I:
				setbinary();
				break;
			case TYPE_E:
				setebcdic();
				break;
			case TYPE_L:
				settenex();
				break;
		}
		verbose = oldverbose;
	}
	code = -1;
	if (data >= 0) {
		(void) close(data);
		data = -1;
	}
	if (closefunc != NULL && fout != NULL)
		(*closefunc)(fout);
	if (din)
		(void) fclose(din);
	if (bytes > 0)
		ptransfer(RECEIVED, bytes, &start, &stop);
	(void) Signal(SIGINT,oldintr);
}

/*
 * Need to start a listen on the data channel
 * before we send the command, otherwise the
 * server's connect may fail.
 */
int sendport = -1;

initconn()
{
	register char *p, *a;
	int result, tmpno = 0;
	size_t len;
	struct linger linger;
	int on = 1;

#ifndef NO_PASSIVE_MODE
	int a1,a2,a3,a4,p1,p2;

	if (passivemode) {
		data = socket(AF_INET, SOCK_STREAM, 0);
		if (data < 0) {
			pfmt(stderr, MM_ERROR, ":151:socket: %s\n",
				strerror(errno));
			return(1);
		}
		if (options & SO_DEBUG &&
		    setsockopt(data, SOL_SOCKET, SO_DEBUG, (char *)&on, sizeof (on)) < 0)
			pfmt(stderr, MM_ERROR, ":162:setsockopt (ignored): %s\n",
			     strerror(errno));
		if (command("PASV") != COMPLETE) {
			pfmt(stdout, MM_NOSTD, ":401:Passive mode refused.\n");
			return(1);
		}

/*
 * What we've got at this point is a string of comma separated
 * one-byte unsigned integer values, separated by commas.
 * The first four are the an IP address. The fifth is the MSB
 * of the port number, the sixth is the LSB. From that we'll
 * prepare a sockaddr_in.
 */

		if (sscanf(pasv,"%d,%d,%d,%d,%d,%d",&a1,&a2,&a3,&a4,&p1,&p2) != 6) {
			pfmt(stdout, MM_NOSTD, ":402:Passive mode address scan failure. Shouldn't happen!\n");
			return(1);
		};

		data_addr.sin_family = AF_INET;
		data_addr.sin_addr.S_un.S_un_b.s_b1 = a1;
		data_addr.sin_addr.S_un.S_un_b.s_b2 = a2;
		data_addr.sin_addr.S_un.S_un_b.s_b3 = a3;
		data_addr.sin_addr.S_un.S_un_b.s_b4 = a4;
		data_addr.sin_port = htons((p1<<8)|p2);

		if (connect(data, (struct sockaddr *) &data_addr, sizeof(data_addr))<0) {
			pfmt(stderr, MM_ERROR, ":152:connect: %s\n", strerror(errno));
			return(1);
		}
#ifdef IP_TOS
#ifndef IPTOS_THROUGHPUT
#define IPTOS_THROUGHPUT 0x08
#endif
	on = IPTOS_THROUGHPUT;
	if (setsockopt(data, IPPROTO_IP, IP_TOS, (char *)&on, sizeof(int)) < 0)
		pfmt(stderr, MM_ERROR, ":403:setsockopt TOS (ignored): %s\n", strerror(errno));
#endif
		return(0);
	}
#endif

	linger.l_onoff = 1;
	linger.l_linger = 60;
noport:
	data_addr = myctladdr;
	if (sendport)
		data_addr.sin_port = 0;	/* let system pick one */ 
	if (data != -1)
		(void) close (data);
	data = socket(AF_INET, SOCK_STREAM, 0);
	if (data < 0) {
		pfmt(stderr, MM_ERROR, ":151:socket: %s\n", strerror(errno));
		if (tmpno)
			sendport = 1;
		return (1);
	}
	if (!sendport)
		if (setsockopt(data, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof (on)) < 0) {
			pfmt(stderr, MM_ERROR,
				":160:setsockopt (reuse address): %s\n",
					strerror(errno));
			goto bad;
		}
	if (bind(data, (struct sockaddr *)&data_addr, sizeof (data_addr)) < 0) {
		pfmt(stderr, MM_ERROR, ":161:bind: %s\n", strerror(errno));
		goto bad;
	}
	if (options & SO_DEBUG &&
	    setsockopt(data, SOL_SOCKET, SO_DEBUG, (char *)&on, sizeof (on)) < 0)
		pfmt(stderr, MM_ERROR, ":162:setsockopt (ignored): %s\n",
			strerror(errno));

	len = sizeof (data_addr);
	if (getsockname(data, (struct sockaddr *)&data_addr, &len) < 0) {
		pfmt(stderr, MM_ERROR, ":153:getsockname: %s\n",
			strerror(errno));
		goto bad;
	}
	if (listen(data, 1) < 0)
		pfmt(stderr, MM_ERROR, ":164:listen: %s\n", strerror(errno));
	if (sendport) {
		a = (char *)&data_addr.sin_addr;
		p = (char *)&data_addr.sin_port;
#define	UC(b)	(((int)b)&0xff)
		result =
		    command("PORT %d,%d,%d,%d,%d,%d",
		      UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]),
		      UC(p[0]), UC(p[1]));
		if (result == ERROR && sendport == -1) {
			sendport = 0;
			tmpno = 1;
			goto noport;
		}
		return (result != COMPLETE);
	}
	if (tmpno)
		sendport = 1;
	return (0);
bad:
	(void) close(data), data = -1;
	if (tmpno)
		sendport = 1;
	return (1);
}

FILE *
dataconn(mode)
	char *mode;
{
	struct sockaddr_in from;
	int s;
	size_t fromlen = sizeof (from);

#ifndef NO_PASSIVE_MODE
	if (passivemode)
		return (fdopen(data, mode));
#endif
	s = accept(data, (struct sockaddr *)&from, &fromlen);
	if (s < 0) {
		pfmt(stderr, MM_ERROR, ":165:accept: %s\n",
			strerror(errno));
		(void) close(data), data = -1;
		return (NULL);
	}
	(void) close(data);
	data = s;
	return (fdopen(data, mode));
}

ptransfer(direction, bytes, t0, t1)
	int direction;
	long bytes;
	struct timeval *t0, *t1;
{
	struct timeval td;
	float s, bs;

	if (verbose) {
		tvsub(&td, t1, t0);
		s = td.tv_sec + (td.tv_usec / 1000000.);
#define	nz(x)	((x) == 0 ? 1 : (x))
		bs = bytes / nz(s);
		if(direction == SENT)
			pfmt(stdout, MM_NOSTD, ":397:%ld bytes sent in %.2g seconds (%.2g Kbytes/s)\n",
		    bytes, s, bs / 1024.);
		else
			pfmt(stdout, MM_NOSTD, ":398:%ld bytes received in %.2g seconds (%.2g Kbytes/s)\n",
		    bytes, s, bs / 1024.);
	}
}

/*tvadd(tsum, t0)
	struct timeval *tsum, *t0;
{

	tsum->tv_sec += t0->tv_sec;
	tsum->tv_usec += t0->tv_usec;
	if (tsum->tv_usec > 1000000)
		tsum->tv_sec++, tsum->tv_usec -= 1000000;
} */

tvsub(tdiff, t1, t0)
	struct timeval *tdiff, *t1, *t0;
{

	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

psabort()
{
	extern int abrtflag;

	abrtflag++;
}

pswitch(flag)
	int flag;
{
	extern int proxy, abrtflag;
	void (*oldintr)();
	static struct comvars {
		int connect;
		char name[MAXHOSTNAMELEN];
		struct sockaddr_in mctl;
		struct sockaddr_in hctl;
		FILE *in;
		FILE *out;
		int tpe;
		int cpnd;
		int sunqe;
		int runqe;
		int mcse;
		int ntflg;
		char nti[17];
		char nto[17];
		int mapflg;
		char mi[MAXPATHLEN];
		char mo[MAXPATHLEN];
		} proxstruct, tmpstruct;
	struct comvars *ip, *op;

	abrtflag = 0;
	oldintr = Signal(SIGINT, psabort);
	if (flag) {
		if (proxy)
			return;
		ip = &tmpstruct;
		op = &proxstruct;
		proxy++;
	}
	else {
		if (!proxy)
			return;
		ip = &proxstruct;
		op = &tmpstruct;
		proxy = 0;
	}
	ip->connect = connected;
	connected = op->connect;
	if (hostname) {
		(void) strncpy(ip->name, hostname, sizeof(ip->name) - 1);
		ip->name[strlen(ip->name)] = '\0';
	} else
		ip->name[0] = 0;
	hostname = op->name;
	ip->hctl = hisctladdr;
	hisctladdr = op->hctl;
	ip->mctl = myctladdr;
	myctladdr = op->mctl;
	ip->in = cin;
	cin = op->in;
	ip->out = cout;
	cout = op->out;
	ip->tpe = type;
	type = op->tpe;
	if (!type)
		type = 1;
	ip->cpnd = cpend;
	cpend = op->cpnd;
	ip->sunqe = sunique;
	sunique = op->sunqe;
	ip->runqe = runique;
	runique = op->runqe;
	ip->mcse = mcase;
	mcase = op->mcse;
	ip->ntflg = ntflag;
	ntflag = op->ntflg;
	(void) strncpy(ip->nti, ntin, 16);
	(ip->nti)[strlen(ip->nti)] = '\0';
	(void) strcpy(ntin, op->nti);
	(void) strncpy(ip->nto, ntout, 16);
	(ip->nto)[strlen(ip->nto)] = '\0';
	(void) strcpy(ntout, op->nto);
	ip->mapflg = mapflag;
	mapflag = op->mapflg;
	(void) strncpy(ip->mi, mapin, MAXPATHLEN - 1);
	(ip->mi)[strlen(ip->mi)] = '\0';
	(void) strcpy(mapin, op->mi);
	(void) strncpy(ip->mo, mapout, MAXPATHLEN - 1);
	(ip->mo)[strlen(ip->mo)] = '\0';
	(void) strcpy(mapout, op->mo);
	(void) Signal(SIGINT, oldintr);
	if (abrtflag) {
		abrtflag = 0;
		(*oldintr)();
	}
}

jmp_buf ptabort;
int ptabflg;

void
abortpt()
{
	pfmt(stdout, MM_NOSTD, ":34:\n");
	(void) fflush(stdout);
	ptabflg++;
	mflag = 0;
	abrtflag = 0;
	sigrelse(SIGINT);
	longjmp(ptabort, 1);
}

proxtrans(cmd, local, remote)
	char *cmd, *local, *remote;
{
	void (*oldintr)(), abortpt();
	int tmptype, oldtype = 0, secndflag = 0, nfnd;
	extern jmp_buf ptabort;
	char *cmd2;
	struct fd_set mask;

	if (strcmp(cmd, "RETR"))
		cmd2 = "RETR";
	else
		cmd2 = runique ? "STOU" : "STOR";
	if (command("PASV") != COMPLETE) {
		pfmt(stdout, MM_NOSTD, ":139:proxy server does not support third part transfers.\n");
		return;
	}
	tmptype = type;
	pswitch(0);
	if (!connected) {
		pfmt(stdout, MM_NOSTD, ":140:No primary connection\n");
		pswitch(1);
		code = -1;
		return;
	}
	if (type != tmptype) {
		oldtype = type;
		switch (tmptype) {
			case TYPE_A:
				setascii();
				break;
			case TYPE_I:
				setbinary();
				break;
			case TYPE_E:
				setebcdic();
				break;
			case TYPE_L:
				settenex();
				break;
		}
	}
	if (command("PORT %s", pasv) != COMPLETE) {
		switch (oldtype) {
			case 0:
				break;
			case TYPE_A:
				setascii();
				break;
			case TYPE_I:
				setbinary();
				break;
			case TYPE_E:
				setebcdic();
				break;
			case TYPE_L:
				settenex();
				break;
		}
		pswitch(1);
		return;
	}
	if (setjmp(ptabort))
		goto abort;
	oldintr = Signal(SIGINT, abortpt);
	if (command("%s %s", cmd, remote) != PRELIM) {
		(void) Signal(SIGINT, oldintr);
		switch (oldtype) {
			case 0:
				break;
			case TYPE_A:
				setascii();
				break;
			case TYPE_I:
				setbinary();
				break;
			case TYPE_E:
				setebcdic();
				break;
			case TYPE_L:
				settenex();
				break;
		}
		pswitch(1);
		return;
	}
	sleep(2);
	pswitch(1);
	secndflag++;
	if (command("%s %s", cmd2, local) != PRELIM)
		goto abort;
	ptflag++;
	(void) getreply(0);
	pswitch(0);
	(void) getreply(0);
	(void) Signal(SIGINT, oldintr);
	switch (oldtype) {
		case 0:
			break;
		case TYPE_A:
			setascii();
			break;
		case TYPE_I:
			setbinary();
			break;
		case TYPE_E:
			setebcdic();
			break;
		case TYPE_L:
			settenex();
			break;
	}
	pswitch(1);
	ptflag = 0;
	pfmt(stdout, MM_NOSTD, ":141:local: %s remote: %s\n", local, remote);
	return;
abort:
	(void) Signal(SIGINT, SIG_IGN);
	ptflag = 0;
	if (strcmp(cmd, "RETR") && !proxy)
		pswitch(1);
	else if (!strcmp(cmd, "RETR") && proxy)
		pswitch(0);
	if (!cpend && !secndflag) {  /* only here if cmd = "STOR" (proxy=1) */
		if (command("%s %s", cmd2, local) != PRELIM) {
			pswitch(0);
			switch (oldtype) {
				case 0:
					break;
				case TYPE_A:
					setascii();
					break;
				case TYPE_I:
					setbinary();
					break;
				case TYPE_E:
					setebcdic();
					break;
				case TYPE_L:
					settenex();
					break;
			}
			if (cpend) {
				char msg[2];

				fprintf(cout,"%c%c",IAC,IP);
				(void) fflush(cout); 
				*msg = IAC;
				*(msg+1) = DM;
				if (send(fileno(cout),msg,2,MSG_OOB) != 2)
					pfmt(stderr, MM_ERROR,
					    ":159:abort: %s\n",
						strerror(errno));
				pfmt(cout, MM_NOSTD, ":142:ABOR\r\n");
				(void) fflush(cout);
				FD_ZERO(&mask);
				FD_SET(fileno(cin), &mask);
				if ((nfnd = empty(&mask,10)) <= 0) {
					if (nfnd < 0) {
						pfmt(stderr, MM_ERROR,
						   ":159:abort: %s\n",
							strerror(errno));
					}
					if (ptabflg)
						code = -1;
					lostpeer();
				}
				(void) getreply(0);
				(void) getreply(0);
			}
		}
		pswitch(1);
		if (ptabflg)
			code = -1;
		(void) Signal(SIGINT, oldintr);
		return;
	}
	if (cpend) {
		char msg[2];

		fprintf(cout,"%c%c",IAC,IP);
		(void) fflush(cout); 
		*msg = IAC;
		*(msg+1) = DM;
		if (send(fileno(cout),msg,2,MSG_OOB) != 2)
			pfmt(stderr, MM_ERROR, ":159:abort: %s\n",
				strerror(errno));
		pfmt(cout, MM_NOSTD, ":142:ABOR\r\n");
		(void) fflush(cout);
		FD_ZERO(&mask);
		FD_SET(fileno(cin), &mask);
		if ((nfnd = empty(&mask,10)) <= 0) {
			if (nfnd < 0) {
				pfmt(stderr, MM_ERROR,
				    ":159:abort: %s\n",
					strerror(errno));
			}
			if (ptabflg)
				code = -1;
			lostpeer();
		}
		(void) getreply(0);
		(void) getreply(0);
	}
	pswitch(!proxy);
	if (!cpend && !secndflag) {  /* only if cmd = "RETR" (proxy=1) */
		if (command("%s %s", cmd2, local) != PRELIM) {
			pswitch(0);
			switch (oldtype) {
				case 0:
					break;
				case TYPE_A:
					setascii();
					break;
				case TYPE_I:
					setbinary();
					break;
				case TYPE_E:
					setebcdic();
					break;
				case TYPE_L:
					settenex();
					break;
			}
			if (cpend) {
				char msg[2];

				fprintf(cout,"%c%c",IAC,IP);
				(void) fflush(cout); 
				*msg = IAC;
				*(msg+1) = DM;
				if (send(fileno(cout),msg,2,MSG_OOB) != 2)
					pfmt(stderr, MM_ERROR,
					    ":159:abort: %s\n",
						strerror(errno));
				pfmt(cout, MM_NOSTD, ":142:ABOR\r\n");
				(void) fflush(cout);
				FD_ZERO(&mask);
				FD_SET(fileno(cin), &mask);
				if ((nfnd = empty(&mask,10)) <= 0) {
					if (nfnd < 0) {
						pfmt(stderr, MM_ERROR,
						   ":159:abort: %s\n",
							strerror(errno));
					}
					if (ptabflg)
						code = -1;
					lostpeer();
				}
				(void) getreply(0);
				(void) getreply(0);
			}
			pswitch(1);
			if (ptabflg)
				code = -1;
			(void) Signal(SIGINT, oldintr);
			return;
		}
	}
	if (cpend) {
		char msg[2];

		fprintf(cout,"%c%c",IAC,IP);
		(void) fflush(cout); 
		*msg = IAC;
		*(msg+1) = DM;
		if (send(fileno(cout),msg,2,MSG_OOB) != 2)
			pfmt(stderr, MM_ERROR, ":159:abort: %s\n",
				strerror(errno));
		pfmt(cout, MM_NOSTD, ":142:ABOR\r\n");
		(void) fflush(cout);
		FD_ZERO(&mask);
		FD_SET(fileno(cin), &mask);
		if ((nfnd = empty(&mask,10)) <= 0) {
			if (nfnd < 0) {
				pfmt(stderr, MM_ERROR,
				    ":159:abort: %s\n",
					strerror(errno));
			}
			if (ptabflg)
				code = -1;
			lostpeer();
		}
		(void) getreply(0);
		(void) getreply(0);
	}
	pswitch(!proxy);
	if (cpend) {
		FD_ZERO(&mask);
		FD_SET(fileno(cin), &mask);
		if ((nfnd = empty(&mask,10)) <= 0) {
			if (nfnd < 0) {
				pfmt(stderr, MM_ERROR,
				    ":159:abort: %s\n",
					strerror(errno));
			}
			if (ptabflg)
				code = -1;
			lostpeer();
		}
		(void) getreply(0);
		(void) getreply(0);
	}
	if (proxy)
		pswitch(0);
	switch (oldtype) {
		case 0:
			break;
		case TYPE_A:
			setascii();
			break;
		case TYPE_I:
			setbinary();
			break;
		case TYPE_E:
			setebcdic();
			break;
		case TYPE_L:
			settenex();
			break;
	}
	pswitch(1);
	if (ptabflg)
		code = -1;
	(void) Signal(SIGINT, oldintr);
}

reset()
{
	struct fd_set mask;
	int nfnd = 1;

	FD_ZERO(&mask);
	while (nfnd > 0) {
		FD_SET(fileno(cin), &mask);
		if ((nfnd = empty(&mask,0)) < 0) {
			pfmt(stderr, MM_ERROR,
			    ":166: reset: %s\n", strerror(errno));
			code = -1;
			lostpeer();
		}
		else if (nfnd) {
			int r;
			r = getreply(0);
			if (r == 4 && code == 421) {
				/*
				 * Remote went away.
				 * Lostpeer was called in getreply, so
				 * no point in doing it now
				 */
				 return;
			}
		}
	}
}

char *
gunique(local)
	char *local;
{
	static char new[MAXPATHLEN];
	char *cp;
	int d, count=0;
	char ext = '1';

	d = check_dir_access(local);
	if (d < 0) {
		pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n", local, strerror(errno));
		return((char *) 0);
	}
	(void) strcpy(new, local);
	cp = new + strlen(new);
	*cp++ = '.';
	while (!d) {
		if (++count == 100) {
			pfmt(stdout, MM_NOSTD, ":143:runique: can't find unique file name.\n");
			return((char *) 0);
		}
		*cp++ = ext;
		*cp = '\0';
		if (ext == '9')
			ext = '0';
		else
			ext++;
		if ((d = access(new, 0)) < 0)
			break;
		if (ext != '0')
			cp--;
		else if (*(cp - 2) == '.')
			*(cp - 1) = '1';
		else {
			*(cp - 2) = *(cp - 2) + 1;
			cp--;
		}
	}
	return(new);
}

check_dir_access(local)
	char *local;
{
	char *dir, savechar;
	int d;

	dir = rindex(local, '/');

	if (dir) {
		savechar = *++dir;
		*dir = '\0';
	}
	d = access(dir ? local : ".",2);
	if (d < 0)
		return (-1);
	/*
	 * previously this test was before the return code
	 * of access() was checked. I think it makes far more
	 * sense to have the caller's perror() call print
	 * just the directory name (with trailing '/'), since
	 * that is what we can't access.	-mre
	 */
	if (dir != NULL)
		*dir = savechar;
	return (0);
}
