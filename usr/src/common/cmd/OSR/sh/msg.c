#ident	"@(#)OSRcmds:sh/msg.c	1.1"
#pragma comment(exestr, "@(#) msg.c 25.2 92/09/16 ")
/*
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1992 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)sh:msg.c	1.5.1.1" */
/*
 *	UNIX shell
 */

/*
 * Modification History
 *
 *	L000	scol!markhe	15th March, 1992
 *	- added message for bad umask
 */

#include	"defs.h"
#include	"sym.h"

/*
 * error messages
 */
unsigned char	badopt[]	= "bad option(s)";
unsigned char	mailmsg[]	= "you have mail\n";
unsigned char	nospace[]	= "no space";
unsigned char	nostack[]	= "no stack space";
unsigned char	synmsg[]	= "syntax error";

unsigned char	badnum[]	= "bad number";
unsigned char	badparam[]	= "parameter null or not set";
unsigned char	unset[]		= "parameter not set";
unsigned char	badsub[]	= "bad substitution";
unsigned char	badcreate[]	= "cannot create";
unsigned char	nofork[]	= "fork failed - too many processes";
unsigned char	noswap[]	= "cannot fork: no swap space";
unsigned char	restricted[]	= "restricted";
unsigned char	piperr[]	= "cannot make pipe";
unsigned char	badopen[]	= "cannot open";
unsigned char	coredump[]	= " - core dumped";
unsigned char	arglist[]	= "arg list too long";
unsigned char	txtbsy[]	= "text busy";
unsigned char	toobig[]	= "too big";
unsigned char	badexec[]	= "cannot execute";
unsigned char	notfound[]	= "not found";
unsigned char	badfile[]	= "bad file number";
unsigned char	badshift[]	= "cannot shift";
unsigned char	baddir[]	= "bad directory";
unsigned char	badtrap[]	= "bad trap";
unsigned char	wtfailed[]	= "is read only";
unsigned char	notid[]		= "is not an identifier";
unsigned char 	badulimit[]	= "Bad ulimit";
unsigned char	badreturn[] 	= "cannot return when not in function";
unsigned char	badexport[] 	= "cannot export functions";
unsigned char	badunset[] 	= "cannot unset";
unsigned char	nohome[]	= "no home directory";
unsigned char 	badperm[]	= "execute permission denied";
unsigned char	longpwd[]	= "sh error: pwd too long";
unsigned char	mssgargn[]	= "missing arguments";
unsigned char	libacc[] 	= "can't access a needed shared library";
unsigned char	libbad[]	= "accessing a corrupted shared library";
unsigned char	libscn[]	= ".lib section in a.out corrupted";
unsigned char	libmax[]	= "attempting to link in too many libs";
unsigned char	badumask[]	= "Bad umask";			/* L000 */
unsigned char    emultihop[]     = "Multihop attempted";
unsigned char    nulldir[]       = "null directory";
unsigned char    enotdir[]       = "not a directory";
unsigned char    enoent[]        = "does not exist";
unsigned char    eacces[]        = "permission denied";
unsigned char    enolink[]       = "remote link inactive";
unsigned char	openmax[]	= "cannot open any more files";

/*
 * messages for 'builtin' functions
 */
unsigned char	btest[]		= "test";
unsigned char	badop[]		= "unknown operator ";
/*
 * built in names
 */
unsigned char	pathname[]	= "PATH";
/* VPIX */
char    dpathname[]     = "DOSPATH";
/* VPIX */
unsigned char	cdpname[]	= "CDPATH";
unsigned char	homename[]	= "HOME";
unsigned char	mailname[]	= "MAIL";
unsigned char	ifsname[]	= "IFS";
unsigned char	ps1name[]	= "PS1";
unsigned char	ps2name[]	= "PS2";
unsigned char	mchkname[]	= "MAILCHECK";
unsigned char	acctname[]  	= "SHACCT";
unsigned char	mailpname[]	= "MAILPATH";

/*
 * string constants
 */
unsigned char	nullstr[]	= "";
unsigned char	sptbnl[]	= " \t\n";
unsigned char	defpath[]	= ":/bin:/usr/bin";
unsigned char	colon[]		= ": ";
unsigned char	minus[]		= "-";
unsigned char	endoffile[]	= "end of file";
unsigned char	unexpected[] 	= " unexpected";
unsigned char	atline[]	= " at line ";
unsigned char	devnull[]	= "/dev/null";
unsigned char	execpmsg[]	= "+ ";
unsigned char	readmsg[]	= "> ";
unsigned char	stdprompt[]	= "$ ";
unsigned char	supprompt[]	= "# ";
unsigned char	profile[]	= ".profile";
unsigned char	sysprofile[]	= "/etc/profile";
/* VPIX */
unsigned char    *vpixdirname;
char    vpix[]          = "vpix";
char    vpixflag[]      = "-c";
char    dotcom[]        = ".com";
char    dotexe[]        = ".exe";
char    dotbat[]        = ".bat";
/* VPIX */

/*
 * tables
 */

struct sysnod reserved[] =
{
	{ (unsigned char *)"case",	CASYM	},
	{ (unsigned char *)"do",		DOSYM	},
	{ (unsigned char *)"done",	ODSYM	},
	{ (unsigned char *)"elif",	EFSYM	},
	{ (unsigned char *)"else",	ELSYM	},
	{ (unsigned char *)"esac",	ESSYM	},
	{ (unsigned char *)"fi",		FISYM	},
	{ (unsigned char *)"for",	FORSYM	},
	{ (unsigned char *)"if",		IFSYM	},
	{ (unsigned char *)"in",		INSYM	},
	{ (unsigned char *)"then",	THSYM	},
	{ (unsigned char *)"until",	UNSYM	},
	{ (unsigned char *)"while",	WHSYM	},
	{ (unsigned char *)"{",		BRSYM	},
	{ (unsigned char *)"}",		KTSYM	}
};

int no_reserved = 15;

unsigned char	*sysmsg[] =
{
	0,
	(unsigned char *)"Hangup",
	0,	/* Interrupt */
	(unsigned char *)"Quit",
	(unsigned char *)"Illegal instruction",
	(unsigned char *)"Trace/BPT trap",
	(unsigned char *)"abort",
	(unsigned char *)"EMT trap",
	(unsigned char *)"Floating exception",
	(unsigned char *)"Killed",
	(unsigned char *)"Bus error",
	(unsigned char *)"Memory fault",
	(unsigned char *)"Bad system call",
	0,	/* Broken pipe */
	(unsigned char *)"Alarm call",
	(unsigned char *)"Terminated",
	(unsigned char *)"Signal 16",
	(unsigned char *)"Signal 17",
	(unsigned char *)"Child death",
	(unsigned char *)"Power Fail"
};

unsigned char	export[] = "export";
unsigned char	duperr[] = "cannot dup";
unsigned char	readonly[] = "readonly";


struct sysnod commands[] =
{
	{ (unsigned char *)".",		SYSDOT	},
	{ (unsigned char *)":",		SYSNULL	},

#ifndef RES
	{ (unsigned char *)"[",		SYSTST },
#endif

	{ (unsigned char *)"break",	SYSBREAK },
	{ (unsigned char *)"cd",		SYSCD	},
	{ (unsigned char *)"continue",	SYSCONT	},
	{ (unsigned char *)"echo",	SYSECHO },
	{ (unsigned char *)"eval",	SYSEVAL	},
	{ (unsigned char *)"exec",	SYSEXEC	},
	{ (unsigned char *)"exit",	SYSEXIT	},
	{ (unsigned char *)"export",	SYSXPORT },
	{ (unsigned char *)"getopts",	SYSGETOPT },
	{ (unsigned char *)"hash",	SYSHASH	},

#ifdef RES
	{ (unsigned char *)"login",	SYSLOGIN },
	{ (unsigned char *)"newgrp",	SYSLOGIN },
#else
	{ (unsigned char *)"newgrp",	SYSNEWGRP },
#endif

	{ (unsigned char *)"pwd",	SYSPWD },
	{ (unsigned char *)"read",	SYSREAD	},
	{ (unsigned char *)"readonly",	SYSRDONLY },
	{ (unsigned char *)"return",	SYSRETURN },
	{ (unsigned char *)"set",	SYSSET	},
	{ (unsigned char *)"shift",	SYSSHFT	},
	{ (unsigned char *)"test",	SYSTST },
	{ (unsigned char *)"times",	SYSTIMES },
	{ (unsigned char *)"trap",	SYSTRAP	},
	{ (unsigned char *)"type",	SYSTYPE },


#ifndef RES		
	{ (unsigned char *)"ulimit",	SYSULIMIT },
	{ (unsigned char *)"umask",	SYSUMASK },
#endif

	{ (unsigned char *)"unset", 	SYSUNS },
	{ (unsigned char *)"wait",	SYSWAIT	}
};

#ifdef RES
	int no_commands = 26;
#else
	int no_commands = 28;
#endif
