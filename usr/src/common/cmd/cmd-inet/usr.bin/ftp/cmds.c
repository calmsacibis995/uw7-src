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

#ident	"@(#)cmds.c	1.3"
#ident	"$Header$"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

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
 * @(#)cmds.c	5.18 (Berkeley) 4/20/89
 */


/*
 * FTP User Program -- Command Routines.
 */
#include <sys/types.h>
#include <pfmt.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <arpa/ftp.h>

#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <ctype.h>
#include <time.h>
#include <pwd.h>
#include <unistd.h>
#include <netinet/in.h>

#include "ftp_var.h"
#include "pathnames.h"

#include "../../usr.sbin/security.h"

extern	char *globerr;
extern	char **glob();
extern	char *home;
extern	char *remglob();
extern	char *getenv();
extern	char *index();
extern	char *rindex();
extern	int allbinary;
extern off_t restart_point;
extern char reply_string[];
extern u_short ftp_port;

char *mname;
jmp_buf jabort;
char *dotrans(), *domap();

extern void (*Signal())();

/*
 * Connect to peer server and
 * auto-login, if possible.
 */
setpeer(argc, argv)
	int argc;
	char *argv[];
{
	char *host, *hookup();
	u_short port;

	if (connected) {
		pfmt(stdout,
			MM_NOSTD, ":12:Already connected to %s, use close first.\n",
				hostname);
		code = -1;
		return;
	}
	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout,
			MM_NOSTD, ":13:(to) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc > 3 || argc < 2) {
		pfmt(stdout,
			MM_NOSTD, ":14:usage: %s host-name [port]\n", argv[0]);
		code = -1;
		return;
	}
	port = ftp_port;
	if (argc > 2) {
		port = atoi(argv[2]);
		if (port <= 0) {
			pfmt(stdout,
				MM_NOSTD, ":15:%s: bad port number-- %s\n", argv[1],
					argv[2]);
			pfmt(stdout,
				MM_NOSTD, ":14:usage: %s host-name [port]\n", argv[0]);
			code = -1;
			return;
		}
		port = htons(port);
	}
	host = hookup(argv[1], port);
	if (host) {
		int overbose;

		connected = 1;
		if (autologin)
			(void) login(argv[1]);

#if defined(unix) && NBBY == 8
/*
 * this ifdef is to keep someone form "porting" this to an incompatible
 * system and not checking this out. This way they have to think about it.
 */
		overbose = verbose;
		if (debug == 0)
			verbose = -1;
		allbinary = 0;
		if (!compat && command("SYST") == COMPLETE && overbose) {
			register char *cp, c;
			cp = index(reply_string+4, ' ');
			if (cp == NULL)
				cp = index(reply_string+4, '\r');
			if (cp) {
				if (cp[-1] == '.')
					cp--;
				c = *cp;
				*cp = '\0';
			}

			pfmt(stdout,
				MM_NOSTD, ":16:Remote system type is %s.\n",
					reply_string+4);
			if (cp)
				*cp = c;
		}
		if (!strncmp(reply_string, "215 UNIX Type: L8", 17)) {
			setbinary();
			/* allbinary = 1; this violates the RFC */
			if (overbose)
			    pfmt(stdout,
				MM_NOSTD, ":17:Using %s mode to transfer files.\n",
					typename);
		} else if (overbose && 
		    !strncmp(reply_string, "215 TOPS20", 10)) {
			pfmt(stdout,
				MM_NOSTD, ":18:Remember to set tenex mode when transfering binary files from this machine.\n");
		}
		{
			char *envptr;
			char *argv[] = { "SITE", "LANG" , 0 };

			if (((envptr = getenv("LANG")) != NULL) &&
			    (*envptr != '\0')) {
				argv[2] = envptr;
				site(3, argv);
			}
		}
		verbose = overbose;
#endif /* unix */
	}
}

struct	types {
	char	*t_name;
	char	*t_mode;
	int	t_type;
	char	*t_arg;
} types[] = {
	{ "ascii",	"A",	TYPE_A,	0 },
	{ "binary",	"I",	TYPE_I,	0 },
	{ "image",	"I",	TYPE_I,	0 },
	{ "ebcdic",	"E",	TYPE_E,	0 },
	{ "tenex",	"L",	TYPE_L,	bytename },
	0
};

/*
 * Set transfer type.
 */
settype(argc, argv)
	char *argv[];
{
	register struct types *p;
	int comret;

	if (argc > 2) {
		char *sep;

		pfmt(stdout,
			MM_NOSTD, ":19:usage: %s [", argv[0]);
		sep = " ";
		for (p = types; p->t_name; p++) {
			pfmt(stdout,
				MM_NOSTD, ":20:%s%s", sep, p->t_name);
			if (*sep == ' ')
				sep = " | ";
		}
		pfmt(stdout,
			MM_NOSTD, ":21: ]\n");
		code = -1;
		return;
	}
	if (argc < 2) {
		pfmt(stdout,
			MM_NOSTD, ":17:Using %s mode to transfer files.\n", typename);
		code = 0;
		return;
	}
	for (p = types; p->t_name; p++)
		if (strcmp(argv[1], p->t_name) == 0)
			break;
	if (p->t_name == 0) {
		pfmt(stdout,
			MM_NOSTD, ":22:%s: unknown mode\n", argv[1]);
		code = -1;
		return;
	}
	if ((p->t_arg != NULL) && (*(p->t_arg) != '\0'))
		comret = command ("TYPE %s %s", p->t_mode, p->t_arg);
	else
		comret = command("TYPE %s", p->t_mode);
	if (comret == COMPLETE) {
		(void) strcpy(typename, p->t_name);
		type = p->t_type;
	}
}

char *stype[] = {
	"type",
	"",
	0
};

/*
 * Set binary transfer type.
 */
/*VARARGS*/
setbinary()
{
	stype[1] = "binary";
	settype(2, stype);
}

/*
 * Set ascii transfer type.
 */
/*VARARGS*/
setascii()
{
	stype[1] = "ascii";
	settype(2, stype);
}

/*
 * Set tenex transfer type.
 */
/*VARARGS*/
settenex()
{
	stype[1] = "tenex";
	settype(2, stype);
}

/*
 * Set ebcdic transfer type.
 */
/*VARARGS*/
setebcdic()
{
	stype[1] = "ebcdic";
	settype(2, stype);
}

/*
 * Set file transfer mode.
 */
/*ARGSUSED*/
setmode(argc, argv)
	char *argv[];
{

	pfmt(stdout,
		MM_NOSTD, ":23:We only support %s mode, sorry.\n", modename);
	code = -1;
}

/*
 * Set file transfer format.
 */
/*ARGSUSED*/
setform(argc, argv)
	char *argv[];
{

	pfmt(stdout,
		MM_NOSTD, ":24:We only support %s format, sorry.\n", formname);
	code = -1;
}

/*
 * Set file transfer structure.
 */
/*ARGSUSED*/
setstruct(argc, argv)
	char *argv[];
{

	pfmt(stdout,
		MM_NOSTD, ":25:We only support %s structure, sorry.\n", structname);
	code = -1;
}

/*
 * Send a single file.
 */
put(argc, argv)
	int argc;
	char *argv[];
{
	char *cmd;
	int loc = 0;
	char *oldargv1, *oldargv2;

	if (argc == 2) {
		argc++;
		argv[2] = argv[1];
		loc++;
	}
	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":26:(local-file) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
usage:
		pfmt(stdout, MM_NOSTD, ":27:usage:%s local-file remote-file\n", argv[0]);
		code = -1;
		return;
	}
	if (argc < 3) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":28:(remote-file) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 3) 
		goto usage;
	oldargv1 = argv[1];
	oldargv2 = argv[2];
	if (!globulize(&argv[1])) {
		code = -1;
		return;
	}
	/*
	 * If "globulize" modifies argv[1], and argv[2] is a copy of
	 * the old argv[1], make it a copy of the new argv[1].
	 */
	if (argv[1] != oldargv1 && argv[2] == oldargv1) {
		argv[2] = argv[1];
	}
	cmd = (argv[0][0] == 'a') ? "APPE" : ((sunique) ? "STOU" : "STOR");
	if (loc && ntflag) {
		argv[2] = dotrans(argv[2]);
	}
	if (loc && mapflag) {
		argv[2] = domap(argv[2]);
	}
	sendrequest(cmd, argv[1], argv[2],
	    argv[1] != oldargv1 || argv[2] != oldargv2);
}

/*
 * Send multiple files.
 */
mput(argc, argv)
	char *argv[];
{
	register int i;
	int ointer, mabort();
	void (*oldintr)();
	extern jmp_buf jabort;
	char *tp;

	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":29:(local-files) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		pfmt(stdout, MM_NOSTD, ":30:usage:%s local-files\n", argv[0]);
		code = -1;
		return;
	}
	mname = argv[0];
	mflag = 1;
	oldintr = Signal(SIGINT, mabort);
	(void) setjmp(jabort);
	if (proxy) {
		char *cp, *tp2, tmpbuf[MAXPATHLEN];

		while ((cp = remglob(argv,0)) != NULL) {
			if (*cp == 0) {
				mflag = 0;
				continue;
			}
			if (mflag && confirm(argv[0], cp)) {
				tp = cp;
				if (mcase) {
					while (*tp && !islower(*tp)) {
						tp++;
					}
					if (!*tp) {
						tp = cp;
						tp2 = tmpbuf;
						while ((*tp2 = *tp) != '\0') {
						     if (isupper(*tp2)) {
						        *tp2 = 'a' + *tp2 - 'A';
						     }
						     tp++;
						     tp2++;
						}
						tp = tmpbuf;
					}
					else {
						tp = cp;
					}
				}
				if (ntflag) {
					tp = dotrans(tp);
				}
				if (mapflag) {
					tp = domap(tp);
				}
				sendrequest((sunique) ? "STOU" : "STOR",
				    cp, tp, cp != tp || !interactive);
				if (!mflag && fromatty) {
					ointer = interactive;
					interactive = 1;
					if (confirm(gettxt(":31", "Continue with"),
							"mput")) {
						mflag++;
					}
					interactive = ointer;
				}
			}
		}
		(void) Signal(SIGINT, oldintr);
		mflag = 0;
		return;
	}
	for (i = 1; i < argc; i++) {
		register char **cpp, **gargs;

		if (!doglob) {
			if (mflag && confirm(argv[0], argv[i])) {
				tp = (ntflag) ? dotrans(argv[i]) : argv[i];
				tp = (mapflag) ? domap(tp) : tp;
				sendrequest((sunique) ? "STOU" : "STOR",
				    argv[i], tp, tp != argv[i] || !interactive);
				if (!mflag && fromatty) {
					ointer = interactive;
					interactive = 1;
					if (confirm(gettxt(":31", "Continue with"),
							"mput")) {
						mflag++;
					}
					interactive = ointer;
				}
			}
			continue;
		}
		gargs = glob(argv[i]);
		if (globerr != NULL) {
			pfmt(stdout, MM_NOSTD, ":32:%s\n", globerr);
			if (gargs)
				blkfree(gargs, 0);
			continue;
		}
		for (cpp = gargs; cpp && *cpp != NULL; cpp++) {
			if (mflag && confirm(argv[0], *cpp)) {
				tp = (ntflag) ? dotrans(*cpp) : *cpp;
				tp = (mapflag) ? domap(tp) : tp;
				sendrequest((sunique) ? "STOU" : "STOR",
				    *cpp, tp, *cpp != tp || !interactive);
				if (!mflag && fromatty) {
					ointer = interactive;
					interactive = 1;
					if (confirm(gettxt(":31", "Continue with"),
							"mput")) {
						mflag++;
					}
					interactive = ointer;
				}
			}
		}
		if (gargs != NULL)
			blkfree(gargs, 0);
	}
	(void) Signal(SIGINT, oldintr);
	mflag = 0;
}

reget(argc, argv)
	char *argv[];
{
	(void) getit(argc, argv, 1, "r+w");
}

get(argc, argv)
	char *argv[];
{
	(void) getit(argc, argv, 0, restart_point ? "r+w" : "w" );
}

/*
 * Receive one file.
 */
getit(argc, argv, restartit, mode)
	char *argv[];
	char *mode;
{
	int loc = 0;
	char *oldargv1, *oldargv2;

	if (argc == 2) {
		argc++;
		argv[2] = argv[1];
		loc++;
	}
	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":28:(remote-file) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
usage:
		pfmt(stdout, MM_NOSTD, ":33:usage: %s remote-file [ local-file ]\n",
			argv[0]);
		code = -1;
		return (0);
	}
	if (argc < 3) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":26:(local-file) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 3) 
		goto usage;
	oldargv1 = argv[1];
	oldargv2 = argv[2];
	if (!globulize(&argv[2])) {
		code = -1;
		return (0);
	}
	if (loc && mcase) {
		char *tp = argv[1], *tp2, tmpbuf[MAXPATHLEN];

		while (*tp && !islower(*tp)) {
			tp++;
		}
		if (!*tp) {
			tp = argv[2];
			tp2 = tmpbuf;
			while ((*tp2 = *tp) != '\0') {
				if (isupper(*tp2)) {
					*tp2 = 'a' + *tp2 - 'A';
				}
				tp++;
				tp2++;
			}
			argv[2] = tmpbuf;
		}
	}
	if (loc && ntflag)
		argv[2] = dotrans(argv[2]);
	if (loc && mapflag)
		argv[2] = domap(argv[2]);
	if (restartit) {
		struct stat stbuf;
		int ret;

		ret = stat(argv[2], &stbuf);
		if (restartit == 1) {
			if (ret < 0) {
				pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n",
					argv[2], strerror(errno));
				return (0);
			}
			restart_point = stbuf.st_size;
		} else {
			if (ret == 0) {
				int overbose;

				overbose = verbose;
				if (debug == 0)
					verbose = -1;
				if (command("MDTM %s", argv[1]) == COMPLETE) {
					int yy, mo, day, hour, min, sec;
					struct tm *tm;
					verbose = overbose;
					sscanf(reply_string,
					    "%*s %04d%02d%02d%02d%02d%02d",
					    &yy, &mo, &day, &hour, &min, &sec);
					tm = gmtime(&stbuf.st_mtime);
					tm->tm_mon++;
					if (tm->tm_year > yy%100)
						return (1);
					else if (tm->tm_year == yy%100) {
						if (tm->tm_mon > mo)
							return (1);
					} else if (tm->tm_mon == mo) {
						if (tm->tm_mday > day)
							return (1);
					} else if (tm->tm_mday == day) {
						if (tm->tm_hour > hour)
							return (1);
					} else if (tm->tm_hour == hour) {
						if (tm->tm_min > min)
							return (1);
					} else if (tm->tm_min == min) {
						if (tm->tm_sec > sec)
							return (1);
					}
				} else {
					fputs(reply_string, stdout);
					verbose = overbose;
					return (0);
				}
			}
		}
	}

	recvrequest("RETR", argv[2], argv[1], mode,
	    argv[1] != oldargv1 || argv[2] != oldargv2);
	restart_point = 0;
	return (0);
}

mabort()
{
	int ointer;
	extern jmp_buf jabort;

	pfmt(stdout, MM_NOSTD, ":34:\n");
	(void) fflush(stdout);
	if (mflag && fromatty) {
		ointer = interactive;
		interactive = 1;
		if (confirm(gettxt(":31", "Continue with"), mname)) {
			interactive = ointer;
			sigrelse(SIGINT);
			longjmp(jabort,0);
		}
		interactive = ointer;
	}
	mflag = 0;
	sigrelse(SIGINT);
	longjmp(jabort,0);
}

/*
 * Get multiple files.
 */
mget(argc, argv)
	char *argv[];
{
	char *cp, *tp, *tp2, tmpbuf[MAXPATHLEN];
	int ointer, mabort();
	void (*oldintr)();
	extern jmp_buf jabort;

	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":35:(remote-files) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		pfmt(stdout, MM_NOSTD, ":36:usage:%s remote-files\n", argv[0]);
		code = -1;
		return;
	}
	mname = argv[0];
	mflag = 1;
	oldintr = Signal(SIGINT,mabort);
	(void) setjmp(jabort);
	while ((cp = remglob(argv,proxy)) != NULL) {
		if (*cp == '\0') {
			mflag = 0;
			continue;
		}
		if (mflag && confirm(argv[0], cp)) {
			tp = cp;
			if (mcase) {
				while (*tp && !islower(*tp)) {
					tp++;
				}
				if (!*tp) {
					tp = cp;
					tp2 = tmpbuf;
					while ((*tp2 = *tp) != '\0') {
						if (isupper(*tp2)) {
							*tp2 = 'a' + *tp2 - 'A';
						}
						tp++;
						tp2++;
					}
					tp = tmpbuf;
				}
				else {
					tp = cp;
				}
			}
			if (ntflag) {
				tp = dotrans(tp);
			}
			if (mapflag) {
				tp = domap(tp);
			}
			recvrequest("RETR", tp, cp, "w",
			    tp != cp || !interactive);
			if (!mflag && fromatty) {
				ointer = interactive;
				interactive = 1;
				if (confirm(gettxt(":31","Continue with"),"mget")) {
					mflag++;
				}
				interactive = ointer;
			}
		}
	}
	(void) Signal(SIGINT,oldintr);
	mflag = 0;
}

char *
remglob(argv,doswitch)
	char *argv[];
	int doswitch;
{
	char temp[16];
	static char buf[MAXPATHLEN];
	static FILE *ftemp = NULL;
	static char **args;
	int oldverbose, oldhash;
	char *cp, *mode;

	if (!mflag) {
		if (!doglob) {
			args = NULL;
		}
		else {
			if (ftemp) {
				(void) fclose(ftemp);
				ftemp = NULL;
			}
		}
		return(NULL);
	}
	if (!doglob) {
		if (args == NULL)
			args = argv;
		if ((cp = *++args) == NULL)
			args = NULL;
		return (cp);
	}
	if (ftemp == NULL) {
		(void) strcpy(temp, _PATH_TMP);
		(void) mktemp(temp);
		oldverbose = verbose, verbose = 0;
		oldhash = hash, hash = 0;
		if (doswitch) {
			pswitch(!proxy);
		}
		for (mode = "w"; *++argv != NULL; mode = "a")
			recvrequest ("NLST", temp, *argv, mode, 0);
		if (doswitch) {
			pswitch(!proxy);
		}
		verbose = oldverbose; hash = oldhash;
		ftemp = fopen(temp, "r");
		(void) unlink(temp);
		if (ftemp == NULL) {
			pfmt(stdout,
				MM_NOSTD, ":37:can't find list of remote files, oops\n");
			return (NULL);
		}
	}
	if (fgets(buf, sizeof (buf), ftemp) == NULL) {
		(void) fclose(ftemp), ftemp = NULL;
		return (NULL);
	}
	if ((cp = index(buf, '\n')) != NULL)
		*cp = '\0';
	return (buf);
}

char *
onoff(bool)
	int bool;
{

	return (bool ? gettxt("uxue:120", "on") : gettxt("uxue:121", "off"));
}

/*
 * Show status.
 */
/*ARGSUSED*/
status(argc, argv)
	char *argv[];
{
	int i;

	if (connected)
		pfmt(stdout, MM_NOSTD, ":38:Connected to %s.\n", hostname);
	else
		pfmt(stdout, MM_NOSTD, ":6:Not connected.\n");
	if (!proxy) {
		pswitch(1);
		if (connected) {
		  pfmt(stdout,
		    MM_NOSTD, ":39:Connected for proxy commands to %s.\n", hostname);
		}
		else {
			pfmt(stdout, MM_NOSTD, ":40:No proxy connection.\n");
		}
		pswitch(0);
	}
	pfmt(stdout, MM_NOSTD, ":41:Mode: %s; Type: %s; Form: %s; Structure: %s\n",
		modename, typename, formname, structname);
#ifndef NO_PASSIVE_MODE
	pfmt(stdout, MM_NOSTD, ":400:Passive mode %s.\n", onoff(passivemode));
#endif
	pfmt(stdout, MM_NOSTD, ":42:Verbose: %s; Bell: %s; Prompting: %s; Globbing: %s\n", 
		onoff(verbose), onoff(bell), onoff(interactive),
		onoff(doglob));
	pfmt(stdout, MM_NOSTD, ":43:Store unique: %s; Receive unique: %s\n",
		onoff(sunique), onoff(runique));
	pfmt(stdout, MM_NOSTD, ":44:Case: %s; CR stripping: %s\n",
		onoff(mcase),onoff(crflag));
	if (ntflag) {
		pfmt(stdout, MM_NOSTD, ":45:Ntrans: (in) %s (out) %s\n",
			ntin,ntout);
	}
	else {
		pfmt(stdout, MM_NOSTD, ":46:Ntrans: off\n");
	}
	if (mapflag) {
		pfmt(stdout, MM_NOSTD, ":47:Nmap: (in) %s (out) %s\n", mapin, mapout);
	}
	else {
		pfmt(stdout, MM_NOSTD, ":48:Nmap: off\n");
	}
	pfmt(stdout, MM_NOSTD, ":49:Hash mark printing: %s; Use of PORT cmds: %s\n",
		onoff(hash), onoff(sendport));
	if (macnum > 0) {
		pfmt(stdout, MM_NOSTD, ":50:Macros:\n");
		for (i=0; i<macnum; i++) {
			pfmt(stdout, MM_NOSTD, ":51:\t%s\n",macros[i].mac_name);
		}
	}
	code = 0;
}

/*
 * Set beep on cmd completed mode.
 */
/*VARARGS*/
setbell()
{

	bell = !bell;
	pfmt(stdout, MM_NOSTD, ":52:Bell mode %s.\n", onoff(bell));
	code = bell;
}

/*
 * Turn on packet tracing.
 */
/*VARARGS*/
settrace()
{

	trace = !trace;
	pfmt(stdout, MM_NOSTD, ":53:Packet tracing %s.\n", onoff(trace));
	code = trace;
}

/*
 * Toggle hash mark printing during transfers.
 */
/*VARARGS*/
sethash()
{

	hash = !hash;
	pfmt(stdout, MM_NOSTD, ":54:Hash mark printing %s", onoff(hash));
	code = hash;
	if (hash)
		pfmt(stdout, MM_NOSTD, ":55: (%d bytes/hash mark)", 1024);
	printf(".\n");
}

/*
 * Turn on printing of server echo's.
 */
/*VARARGS*/
setverbose()
{

	verbose = !verbose;
	pfmt(stdout, MM_NOSTD, ":56:Verbose mode %s.\n", onoff(verbose));
	code = verbose;
}

/*
 * Toggle PORT cmd use before each data connection.
 */
/*VARARGS*/
setport()
{

	sendport = !sendport;
	pfmt(stdout, MM_NOSTD, ":57:Use of PORT cmds %s.\n", onoff(sendport));
	code = sendport;
}

/*
 * Turn on interactive prompting
 * during mget, mput, and mdelete.
 */
/*VARARGS*/
setprompt()
{

	interactive = !interactive;
	pfmt(stdout, MM_NOSTD, ":58:Interactive mode %s.\n", onoff(interactive));
	code = interactive;
}

/*
 * Toggle metacharacter interpretation
 * on local file names.
 */
/*VARARGS*/
setglob()
{
	
	doglob = !doglob;
	pfmt(stdout, MM_NOSTD, ":59:Globbing %s.\n", onoff(doglob));
	code = doglob;
}

/*
 * Set debugging mode on/off and/or
 * set level of debugging.
 */
/*VARARGS*/
setdebug(argc, argv)
	char *argv[];
{
	int val;

	if (argc > 1) {
		val = atoi(argv[1]);
		if (val < 0) {
			pfmt(stdout,
				MM_NOSTD, ":60:%s: bad debugging value.\n", argv[1]);
			code = -1;
			return;
		}
	} else
		val = !debug;
	debug = val;
	if (debug)
		options |= SO_DEBUG;
	else
		options &= ~SO_DEBUG;
	pfmt(stdout,
		MM_NOSTD, ":61:Debugging %s (debug=%d).\n", onoff(debug), debug);
	code = debug > 0;
}

/*
 * Set current working directory
 * on remote machine.
 */
cd(argc, argv)
	char *argv[];
{
	int overbose = verbose;

	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout,
			MM_NOSTD, ":62:(remote-directory) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		pfmt(stdout,
			MM_NOSTD, ":63:usage:%s remote-directory\n", argv[0]);
		code = -1;
		return;
	}
	if (!verbose) verbose = -2;
	if (command("CWD %s", argv[1]) == ERROR && code == 500) {
		verbose = overbose;
		if (verbose)
			pfmt(stdout,
				MM_NOSTD, ":64:CWD command not recognized, trying XCWD\n");
		(void) command("XCWD %s", argv[1]);
	}
	verbose = overbose;
}

/*
 * Set current working directory
 * on local machine.
 */
lcd(argc, argv)
	char *argv[];
{
	char buf[MAXPATHLEN];

	if (argc < 2)
		argc++, argv[1] = home;
	if (argc != 2) {
		pfmt(stdout, MM_NOSTD, ":65:usage:%s local-directory\n", argv[0]);
		code = -1;
		return;
	}
	if (!globulize(&argv[1])) {
		code = -1;
		return;
	}
	if (chdir(argv[1]) < 0) {
		pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n", argv[1],
			strerror(errno));
		code = -1;
		return;
	}
	pfmt(stdout,
		MM_NOSTD, ":66:Local directory now %s\n", getcwd(buf, sizeof(buf)));
	code = 0;
}

/*
 * Delete a single file.
 */
delete(argc, argv)
	char *argv[];
{

	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout,
			MM_NOSTD, ":28:(remote-file) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		pfmt(stdout, MM_NOSTD, ":67:usage:%s remote-file\n", argv[0]);
		code = -1;
		return;
	}
	(void) command("DELE %s", argv[1]);
}

/*
 * Delete multiple files.
 */
mdelete(argc, argv)
	char *argv[];
{
	char *cp;
	int ointer, mabort();
	void (*oldintr)();
	extern jmp_buf jabort;

	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":35:(remote-files) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		pfmt(stdout, MM_NOSTD, ":36:usage:%s remote-files\n", argv[0]);
		code = -1;
		return;
	}
	mname = argv[0];
	mflag = 1;
	oldintr = Signal(SIGINT, mabort);
	(void) setjmp(jabort);
	while ((cp = remglob(argv,0)) != NULL) {
		if (*cp == '\0') {
			mflag = 0;
			continue;
		}
		if (mflag && confirm(argv[0], cp)) {
			(void) command("DELE %s", cp);
			if (!mflag && fromatty) {
				ointer = interactive;
				interactive = 1;
				if (confirm(gettxt(":31", "Continue with"), "mdelete")) {
					mflag++;
				}
				interactive = ointer;
			}
		}
	}
	(void) Signal(SIGINT, oldintr);
	mflag = 0;
}

/*
 * Rename a remote file.
 */
renamefile(argc, argv)
	char *argv[];
{

	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":68:(from-name) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
usage:
		pfmt(stdout, MM_NOSTD, ":69:%s from-name to-name\n", argv[0]);
		code = -1;
		return;
	}
	if (argc < 3) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":70:(to-name) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 3) 
		goto usage;
	if (command("RNFR %s", argv[1]) == CONTINUE)
		(void) command("RNTO %s", argv[2]);
}

/*
 * Get a directory listing
 * of remote files.
 */
ls(argc, argv)
	char *argv[];
{
	char *cmd;

	if (argc < 2)
		argc++, argv[1] = NULL;
	if (argc < 3)
		argc++, argv[2] = "-";
	if (argc > 3) {
		pfmt(stdout, MM_NOSTD, ":71:usage: %s remote-directory local-file\n",
			argv[0]);
		code = -1;
		return;
	}
	cmd = argv[0][0] == 'n' ? "NLST" : "LIST";
	if (strcmp(argv[2], "-") && !globulize(&argv[2])) {
		code = -1;
		return;
	}
	if (strcmp(argv[2], "-") && *argv[2] != '|')
		if (!globulize(&argv[2]) || !confirm(gettxt(
			":240", "output to local-file:"), argv[2])) {
			code = -1;
			return;
	}
	recvrequest(cmd, argv[2], argv[1], "w", 0);
}

/*
 * Get a directory listing
 * of multiple remote files.
 */
mls(argc, argv)
	char *argv[];
{
	char *cmd, mode[1], *dest;
	int ointer, i, mabort();
	void (*oldintr)();
	extern jmp_buf jabort;

	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":35:(remote-files) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 3) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":26:(local-file) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 3) {
		pfmt(stdout,
			MM_NOSTD, ":72:usage:%s remote-files local-file\n", argv[0]);
		code = -1;
		return;
	}
	dest = argv[argc - 1];
	argv[argc - 1] = NULL;
	if (strcmp(dest, "-") && *dest != '|')
		if (!globulize(&dest) || !confirm(
			gettxt(":240", "output to local-file:"), dest)) {
			code = -1;
			return;
	}
	cmd = argv[0][1] == 'l' ? "NLST" : "LIST";
	mname = argv[0];
	mflag = 1;
	oldintr = Signal(SIGINT, mabort);
	(void) setjmp(jabort);
	for (i = 1; mflag && i < argc-1; ++i) {
		*mode = (i == 1) ? 'w' : 'a';
		recvrequest(cmd, dest, argv[i], mode, 0);
		if (!mflag && fromatty) {
			ointer = interactive;
			interactive = 1;
			if (confirm(gettxt(":31", "Continue with"), argv[0])) {
				mflag ++;
			}
			interactive = ointer;
		}
	}
	(void) Signal(SIGINT, oldintr);
	mflag = 0;
}

/*
 * Do a shell escape
 */
/*ARGSUSED*/
shell(argc, argv)
	char *argv[];
{
	int pid;
	void (*old1)(), (*old2)();
	char *shell, *namep; 
#ifdef SYSV
	int	status;
#else
	union wait status;
#endif

	old1 = Signal (SIGINT, SIG_IGN);
	old2 = Signal (SIGQUIT, SIG_IGN);
	if ((pid = fork()) == 0) {
		for (pid = 3; pid < 20; pid++)
			(void) close(pid);
		(void) Signal(SIGINT, SIG_DFL);
		(void) Signal(SIGQUIT, SIG_DFL);
		shell = getenv("SHELL");
		if (shell == NULL)
			shell = _PATH_BSHELL;
		namep = rindex(shell,'/');
		if (namep == NULL)
			namep = shell;
		if (argc > 1) {
			if (debug) {
				pfmt(stdout,MM_NOSTD, ":73:%s -c %s\n", shell, altarg);
				(void) fflush (stdout);
			}
			CLR_MAXPRIVS_FOR_EXEC execl(shell,namep,"-c",altarg,(char *)0);
		} else {
			if (debug) {
				pfmt(stdout, MM_NOSTD, ":32:%s\n", shell);
				(void) fflush (stdout);
			}
			CLR_MAXPRIVS_FOR_EXEC execl(shell,namep,(char *)0);
		}
		pfmt(stderr, MM_NOGET | MM_ERROR, "%s: %s\n", shell, strerror(errno));
		code = -1;
		exit(1);
	}
	if (pid > 0)
		while (wait(&status) != pid)
			;
	(void) Signal(SIGINT, old1);
	(void) Signal(SIGQUIT, old2);
	if (pid == -1) {
		pfmt(stderr, MM_ERROR, ":239:Try again later: %s\n", strerror(errno));
		code = -1;
	} else {
		code = 0;
	}
	return (0);
}

/*
 * Send new user information (re-login)
 */
user(argc, argv)
	int argc;
	char **argv;
{
	char acct[80], *mygetpass();
	int n, aflag = 0;

	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":74:(username) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc > 4) {
		pfmt(stdout, MM_NOSTD, ":75:usage: %s username [password] [account]\n", argv[0]);
		code = -1;
		return (0);
	}
	n = command("USER %s", argv[1]);
	if (n == CONTINUE) {
		if (argc < 3 )
			argv[2] = mygetpass("Password: "), argc++;
		n = command("PASS %s", argv[2]);
	}
	if (n == CONTINUE) {
		if (argc < 4) {
			pfmt(stdout, MM_NOSTD, ":76:Account: "); (void) fflush(stdout);
			(void) fgets(acct, sizeof(acct) - 1, stdin);
			acct[strlen(acct) - 1] = '\0';
			argv[3] = acct; argc++;
		}
		n = command("ACCT %s", argv[3]);
		aflag++;
	}
	if (n != COMPLETE) {
		pfmt(stdout, MM_NOSTD, ":77:Login failed.\n");
		return (0);
	}
	if (!aflag && argc == 4) {
		(void) command("ACCT %s", argv[3]);
	}
	return (1);
}

/*
 * Print working directory.
 */
/*VARARGS*/
pwd()
{
	int oldverbose = verbose;

	/*
	 * If we aren't verbose, this doesn't do anything!
	 */
	verbose = 1;
	if (command("PWD") == ERROR && code == 500) {
		pfmt(stdout, MM_NOSTD, ":78:PWD command not recognized, trying XPWD\n");
		(void) command("XPWD");
	}
	verbose = oldverbose;
}

/*
 * Make a directory.
 */
makedir(argc, argv)
	char *argv[];
{
	int overbose = verbose;

	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":79:(directory-name) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		pfmt(stdout, MM_NOSTD, ":80:usage: %s directory-name\n", argv[0]);
		code = -1;
		return;
	}
	if (!verbose) verbose = -2;
	if (command("MKD %s", argv[1]) == ERROR && code == 500) {
		verbose = overbose;
		if (verbose)
			pfmt(stdout,
				MM_NOSTD, ":81:MKD command not recognized, trying XMKD\n");
		(void) command("XMKD %s", argv[1]);
	}
	verbose = overbose;
}

/*
 * Remove a directory.
 */
removedir(argc, argv)
	char *argv[];
{
	int overbose = verbose;

	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":79:(directory-name) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		pfmt(stdout, MM_NOSTD, ":80:usage: %s directory-name\n", argv[0]);
		code = -1;
		return;
	}
	if (!verbose) verbose = -2;
	if (command("RMD %s", argv[1]) == ERROR && code == 500) {
		verbose = overbose;
		if (verbose)
			pfmt(stdout,
				MM_NOSTD, ":82:RMD command not recognized, trying XRMD\n");
		(void) command("XRMD %s", argv[1]);
	}
	verbose = overbose;
}

/*
 * Send a line, verbatim, to the remote machine.
 */
quote(argc, argv)
	char *argv[];
{
	int i;
	char buf[BUFSIZ];

	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":83:(command line to send) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		pfmt(stdout, MM_NOSTD, ":84:usage: %s line-to-send\n", argv[0]);
		code = -1;
		return;
	}
	(void) strcpy(buf, argv[1]);
	for (i = 2; i < argc; i++) {
		(void) strcat(buf, " ");
		(void) strcat(buf, argv[i]);
	}
	if (command(buf) == PRELIM) {
		while (getreply(0) == PRELIM);
	}
}

/*
 * Send a SITE command to the remote machine.  The line
 * is sent almost verbatim to the remote machine, the
 * first argument is changed to SITE.
 */

site(argc, argv)
	char *argv[];
{
	int i;
	char buf[BUFSIZ];

	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":85:(arguments to SITE command) ");
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		pfmt(stdout, MM_NOSTD, ":84:usage: %s line-to-send\n", argv[0]);
		code = -1;
		return;
	}
	(void) strcpy(buf, "SITE ");
	(void) strcat(buf, argv[1]);
	for (i = 2; i < argc; i++) {
		(void) strcat(buf, " ");
		(void) strcat(buf, argv[i]);
	}
	if (command(buf) == PRELIM) {
		while (getreply(0) == PRELIM);
	}
}

do_chmod(argc, argv)
	char *argv[];
{
	if (argc == 2) {
		pfmt(stdout, MM_NOSTD, ":86:usage: %s mode file-name\n", argv[0]);
		code = -1;
		return;
	}
	if (argc < 3) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":87:(mode and file-name) ");
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc != 3) {
		pfmt(stdout, MM_NOSTD, ":86:usage: %s mode file-name\n", argv[0]);
		code = -1;
		return;
	}
	(void)command("SITE CHMOD %s %s", argv[1], argv[2]);
}

do_umask(argc, argv)
	char *argv[];
{
	int oldverbose = verbose;

	verbose = 1;
	(void) command(argc == 1 ? "SITE UMASK" : "SITE UMASK %s", argv[1]);
	verbose = oldverbose;
}

idle(argc, argv)
	char *argv[];
{
	int oldverbose = verbose;

	verbose = 1;
	(void) command(argc == 1 ? "SITE IDLE" : "SITE IDLE %s", argv[1]);
	verbose = oldverbose;
}

/*
 * Ask the other side for help.
 */
rmthelp(argc, argv)
	char *argv[];
{
	int oldverbose = verbose;

	verbose = 1;
	(void) command(argc == 1 ? "HELP" : "HELP %s", argv[1]);
	verbose = oldverbose;
}

/*
 * Terminate session and exit.
 */
/*VARARGS*/
quit()
{

	if (connected)
		disconnect();
	pswitch(1);
	if (connected) {
		disconnect();
	}
	exit(0);
}

/*
 * Terminate session, but don't exit.
 */
disconnect()
{
	extern FILE *cout;
	extern int data;

	if (!connected)
		return;
	(void) command("QUIT");
	if (cout) {
		(void) fclose(cout);
	}
	cout = NULL;
	connected = 0;
	data = -1;
	if (!proxy) {
		macnum = 0;
	}
}

confirm(cmd, file)
	char *cmd, *file;
{
	char line[BUFSIZ];

	if (!interactive)
		return (1);
	pfmt(stdout, MM_NOSTD, ":88:%s %s? ", cmd, file);
	(void) fflush(stdout);
	(void) fgets(line,BUFSIZ,stdin);
	return (*line != 'n' && *line != 'N');
}

fatal(msg)
	char *msg;
{

	pfmt(stderr, MM_ERROR,  ":89:%s\n", msg);
	exit(1);
}

/*
 * Glob a local file name specification with
 * the expectation of a single return value.
 * Can't control multiple values being expanded
 * from the expression, we return only the first.
 */
globulize(cpp)
	char **cpp;
{
	char **globbed;

	if (!doglob)
		return (1);
	globbed = glob(*cpp);
	if (globerr != NULL) {
		pfmt(stdout, MM_NOSTD, ":90:%s: %s\n", *cpp, globerr);
		if (globbed)
			blkfree(globbed, 0);
		return (0);
	}
	if (globbed) {
		*cpp = *globbed++;
		/* don't waste too much memory */
		if (*globbed)
			blkfree(globbed, 1);
	}
	return (1);
}

account(argc,argv)
	int argc;
	char **argv;
{
	char acct[50], *mygetpass(), *ap;

	if (argc > 1) {
		++argv;
		--argc;
		(void) strncpy(acct,*argv,49);
		acct[49] = '\0';
		while (argc > 1) {
			--argc;
			++argv;
			(void) strncat(acct,*argv, 49-strlen(acct));
		}
		ap = acct;
	}
	else {
		ap = mygetpass("Account:");
	}
	(void) command("ACCT %s", ap);
}

jmp_buf abortprox;

proxabort()
{
	extern int proxy;

	if (!proxy) {
		pswitch(1);
	}
	if (connected) {
		proxflag = 1;
	}
	else {
		proxflag = 0;
	}
	pswitch(0);
	sigrelse(SIGINT);
	longjmp(abortprox,1);
}

doproxy(argc,argv)
	int argc;
	char *argv[];
{
	void (*oldintr)();
	int proxabort();
	register struct cmd *c;
	struct cmd *getcmd();
	extern struct cmd cmdtab[];
	extern jmp_buf abortprox;

	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":91:(command) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		pfmt(stdout, MM_NOSTD, ":92:usage:%s command\n", argv[0]);
		code = -1;
		return;
	}
	c = getcmd(argv[1]);
	if (c == (struct cmd *) -1) {
		pfmt(stdout, MM_NOSTD, ":4:?Ambiguous command\n");
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (c == 0) {
		pfmt(stdout, MM_NOSTD, ":5:?Invalid command\n");
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (!c->c_proxy) {
		pfmt(stdout, MM_NOSTD, ":93:?Invalid proxy command\n");
		(void) fflush(stdout);
		code = -1;
		return;
	}
	if (setjmp(abortprox)) {
		code = -1;
		return;
	}
	oldintr = Signal(SIGINT, proxabort);
	pswitch(1);
	if (c->c_conn && !connected) {
		pfmt(stdout, MM_NOSTD, ":6:Not connected.\n");
		(void) fflush(stdout);
		pswitch(0);
		(void) Signal(SIGINT, oldintr);
		code = -1;
		return;
	}
	(*c->c_handler)(argc-1, argv+1);
	if (connected) {
		proxflag = 1;
	}
	else {
		proxflag = 0;
	}
	pswitch(0);
	(void) Signal(SIGINT, oldintr);
}

setcase()
{
	mcase = !mcase;
	pfmt(stdout, MM_NOSTD, ":94:Case mapping %s.\n", onoff(mcase));
	code = mcase;
}

setcr()
{
	crflag = !crflag;
	pfmt(stdout, MM_NOSTD, ":95:Carriage Return stripping %s.\n", onoff(crflag));
	code = crflag;
}

setntrans(argc,argv)
	int argc;
	char *argv[];
{
	if (argc == 1) {
		ntflag = 0;
		pfmt(stdout, MM_NOSTD, ":96:Ntrans off.\n");
		code = ntflag;
		return;
	}
	ntflag++;
	code = ntflag;
	(void) strncpy(ntin, argv[1], 16);
	ntin[16] = '\0';
	if (argc == 2) {
		ntout[0] = '\0';
		return;
	}
	(void) strncpy(ntout, argv[2], 16);
	ntout[16] = '\0';
}

char *
dotrans(name)
	char *name;
{
	static char new[MAXPATHLEN];
	char *cp1, *cp2 = new;
	register int i, ostop, found;

	for (ostop = 0; *(ntout + ostop) && ostop < 16; ostop++);
	for (cp1 = name; *cp1; cp1++) {
		found = 0;
		for (i = 0; *(ntin + i) && i < 16; i++) {
			if (*cp1 == *(ntin + i)) {
				found++;
				if (i < ostop) {
					*cp2++ = *(ntout + i);
				}
				break;
			}
		}
		if (!found) {
			*cp2++ = *cp1;
		}
	}
	*cp2 = '\0';
	return(new);
}

setnmap(argc, argv)
	int argc;
	char *argv[];
{
	char *cp;

	if (argc == 1) {
		mapflag = 0;
		pfmt(stdout, MM_NOSTD, ":97:Nmap off.\n");
		code = mapflag;
		return;
	}
	if (argc < 3) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":98:(mapout) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 3) {
		pfmt(stdout, MM_NOSTD, ":99:Usage: %s [mapin mapout]\n",argv[0]);
		code = -1;
		return;
	}
	mapflag = 1;
	code = 1;
	cp = index(altarg, ' ');
	if (proxy) {
		while(*++cp == ' ');
		altarg = cp;
		cp = index(altarg, ' ');
	}
	*cp = '\0';
	(void) strncpy(mapin, altarg, MAXPATHLEN - 1);
	while (*++cp == ' ');
	(void) strncpy(mapout, cp, MAXPATHLEN - 1);
}

char *
domap(name)
	char *name;
{
	static char new[MAXPATHLEN];
	register char *cp1 = name, *cp2 = mapin;
	char *tp[9], *te[9];
	int i, toks[9], toknum = 0, match = 1;

	for (i=0; i < 9; ++i) {
		toks[i] = 0;
	}
	while (match && *cp1 && *cp2) {
		switch (*cp2) {
			case '\\':
				if (*++cp2 != *cp1) {
					match = 0;
				}
				break;
			case '$':
				if (*(cp2+1) >= '1' && (*cp2+1) <= '9') {
					if (*cp1 != *(++cp2+1)) {
						toks[toknum = *cp2 - '1']++;
						tp[toknum] = cp1;
						while (*++cp1 && *(cp2+1)
							!= *cp1);
						te[toknum] = cp1;
					}
					cp2++;
					break;
				}
				/* FALLTHROUGH */
			default:
				if (*cp2 != *cp1) {
					match = 0;
				}
				break;
		}
		if (match && *cp1) {
			cp1++;
		}
		if (match && *cp2) {
			cp2++;
		}
	}
	if (!match && *cp1) /* last token mismatch */
	{
		toks[toknum] = 0;
	}
	cp1 = new;
	*cp1 = '\0';
	cp2 = mapout;
	while (*cp2) {
		match = 0;
		switch (*cp2) {
			case '\\':
				if (*(cp2 + 1)) {
					*cp1++ = *++cp2;
				}
				break;
			case '[':
LOOP:
				if (*++cp2 == '$' && isdigit(*(cp2+1))) { 
					if (*++cp2 == '0') {
						char *cp3 = name;

						while (*cp3) {
							*cp1++ = *cp3++;
						}
						match = 1;
					}
					else if (toks[toknum = *cp2 - '1']) {
						char *cp3 = tp[toknum];

						while (cp3 != te[toknum]) {
							*cp1++ = *cp3++;
						}
						match = 1;
					}
				}
				else {
					while (*cp2 && *cp2 != ',' && 
					    *cp2 != ']') {
						if (*cp2 == '\\') {
							cp2++;
						}
						else if (*cp2 == '$' &&
   						        isdigit(*(cp2+1))) {
							if (*++cp2 == '0') {
							   char *cp3 = name;

							   while (*cp3) {
								*cp1++ = *cp3++;
							   }
							}
							else if (toks[toknum =
							    *cp2 - '1']) {
							   char *cp3=tp[toknum];

							   while (cp3 !=
								  te[toknum]) {
								*cp1++ = *cp3++;
							   }
							}
						}
						else if (*cp2) {
							*cp1++ = *cp2++;
						}
					}
					if (!*cp2) {
						pfmt(stdout,
						  MM_NOSTD, ":100:nmap: unbalanced brackets\n");
						return(name);
					}
					match = 1;
					cp2--;
				}
				if (match) {
					while (*++cp2 && *cp2 != ']') {
					      if (*cp2 == '\\' && *(cp2 + 1)) {
							cp2++;
					      }
					}
					if (!*cp2) {
						pfmt(stdout, MM_NOSTD, ":100:nmap: unbalanced brackets\n");
						return(name);
					}
					break;
				}
				switch (*++cp2) {
					case ',':
						goto LOOP;
					case ']':
						break;
					default:
						cp2--;
						goto LOOP;
				}
				break;
			case '$':
				if (isdigit(*(cp2 + 1))) {
					if (*++cp2 == '0') {
						char *cp3 = name;

						while (*cp3) {
							*cp1++ = *cp3++;
						}
					}
					else if (toks[toknum = *cp2 - '1']) {
						char *cp3 = tp[toknum];

						while (cp3 != te[toknum]) {
							*cp1++ = *cp3++;
						}
					}
					break;
				}
				/* intentional drop through */
			default:
				*cp1++ = *cp2;
				break;
		}
		cp2++;
	}
	*cp1 = '\0';
	if (!*new) {
		return(name);
	}
	return(new);
}

setsunique()
{
	sunique = !sunique;
	pfmt(stdout, MM_NOSTD, ":101:Store unique %s.\n", onoff(sunique));
	code = sunique;
}

setrunique()
{
	runique = !runique;
	pfmt(stdout, MM_NOSTD, ":102:Receive unique %s.\n", onoff(runique));
	code = runique;
}

/* change directory to perent directory */
cdup()
{
	int overbose = verbose;
	if (!verbose) verbose = -2;
	if (command("CDUP") == ERROR && code == 500) {
		verbose = overbose;
		if (verbose)
			pfmt(stdout,
				MM_NOSTD, ":103:CDUP command not recognized, trying XCUP\n");
		(void) command("XCUP");
	}
	verbose = overbose;
}

/* restart transfer at specific point */
restart(argc, argv)
	int argc;
	char *argv[];
{
	extern long atol();
	if (argc != 2)
		pfmt(stdout, MM_NOSTD, ":104:restart: offset not specified\n");
	else {
		restart_point = atol(argv[1]);
		pfmt(stdout, MM_NOSTD, ":105:restarting at %ld. %s\n", restart_point,
		    gettxt(":106", "execute get, put or append to initiate transfer"));
	}
}

/* show remote system type */
syst()
{
	(void) command("SYST");
}

macdef(argc, argv)
	int argc;
	char *argv[];
{
	char *tmp;
	int c;

	if (macnum == 16) {
		pfmt(stdout, MM_NOSTD, ":107:Limit of 16 macros have already been defined\n");
		code = -1;
		return;
	}
	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":108:(macro name) ");
		(void) fgets(&line[strlen(line)],LINSIZ-strlen(line),stdin);
		line[strlen(line)-1]='\0';
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc != 2) {
		pfmt(stdout, MM_NOSTD, ":109:Usage: %s macro_name\n",argv[0]);
		code = -1;
		return;
	}

	for (c = 0; c < macnum; c++) {
		if (!strncmp(argv[1], macros[c].mac_name, 9)) {
			pfmt(stdout, MM_NOSTD, ":110:Macro name %s already in use\n",argv[1]);
			code = -1;
			return;			
		}
	}

	if (interactive) {
		pfmt(stdout, MM_NOSTD, ":111:Enter macro line by line, terminating it with a null line\n");
	}
	(void) strncpy(macros[macnum].mac_name, argv[1], 8);
	if (macnum == 0) {
		macros[macnum].mac_start = macbuf;
	}
	else {
		macros[macnum].mac_start = macros[macnum - 1].mac_end + 1;
	}
	tmp = macros[macnum].mac_start;
	while (tmp != macbuf+4096) {
		if ((c = getchar()) == EOF) {
			pfmt(stdout, MM_NOSTD, ":112:macdef:end of file encountered\n");
			code = -1;
			return;
		}
		if ((*tmp = c) == '\n') {
			if (tmp == macros[macnum].mac_start) {
				macros[macnum++].mac_end = tmp;
				code = 0;
				return;
			}
			if (*(tmp-1) == '\0') {
				macros[macnum++].mac_end = tmp - 1;
				code = 0;
				return;
			}
			*tmp = '\0';
		}
		tmp++;
	}
	while (1) {
		while ((c = getchar()) != '\n' && c != EOF)
			/* LOOP */;
		if (c == EOF || getchar() == '\n') {
			pfmt(stdout, MM_NOSTD, ":113:Macro not defined - 4k buffer exceeded\n");
			code = -1;
			return;
		}
	}
}

/*
 * get size of file on remote machine
 */
sizecmd(argc, argv)
	char *argv[];
{

	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":114:(filename) ");
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		pfmt(stdout, MM_NOSTD, ":115:usage:%s filename\n", argv[0]);
		code = -1;
		return;
	}
	(void) command("SIZE %s", argv[1]);
}

/*
 * get last modification time of file on remote machine
 */
modtime(argc, argv)
	char *argv[];
{
	int overbose;

	if (argc < 2) {
		(void) strcat(line, " ");
		pfmt(stdout, MM_NOSTD, ":114:(filename) ");
		(void) gets(&line[strlen(line)]);
		makeargv();
		argc = margc;
		argv = margv;
	}
	if (argc < 2) {
		pfmt(stdout, MM_NOSTD, ":115:usage:%s filename\n", argv[0]);
		code = -1;
		return;
	}
	overbose = verbose;
	if (debug == 0)
		verbose = -1;
	if (command("MDTM %s", argv[1]) == COMPLETE) {
		int yy, mo, day, hour, min, sec;
		sscanf(reply_string, "%*s %04d%02d%02d%02d%02d%02d", &yy, &mo,
			&day, &hour, &min, &sec);
		/* might want to print this in local time */
		pfmt(stdout,
			MM_NOSTD, ":116:%s\t%02d/%02d/%04d %02d:%02d:%02d GMT\n",
				argv[1], mo, day, yy, hour, min, sec);
	} else
		fputs(reply_string, stdout);
	verbose = overbose;
}

/*
 * show status on reomte machine
 */
rmtstatus(argc, argv)
	char *argv[];
{
	(void) command(argc > 1 ? "STAT %s" : "STAT" , argv[1]);
}

/*
 * get file if modtime is more recent than current file
 */
newer(argc, argv)
	char *argv[];
{
	if (getit(argc, argv, -1, "w"))
		pfmt(stdout,
			MM_NOSTD, ":117:Local file \"%s\" is newer than remote file \"%s\"\n",
				argv[1], argv[2]);
}

#ifndef NO_PASSIVE_MODE
/*
 * Start up passive mode interaction
 */

/*VARARGS*/
setpassive()
{

	passivemode = !passivemode;
	pfmt(stdout, MM_NOSTD, ":400:Passive mode %s.\n", onoff(passivemode));
	code = passivemode;
}
#endif
