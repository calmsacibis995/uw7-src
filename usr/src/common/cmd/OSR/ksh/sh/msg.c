#ident	"@(#)OSRcmds:ksh/sh/msg.c	1.1"
#pragma comment(exestr, "@(#) msg.c 26.1 95/07/11 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1995.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*									*/
/*		   Copyright (C) AT&T, 1984-1992			*/
/*			All Rights Reserved				*/
/*									*/
/*	  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.		*/
/*	    The copyright notice above does not evidence any		*/
/*	   actual or intended publication of such source code.		*/
/*									*/

/*
 *	UNIX shell
 *	S. R. Bourne
 *	Rewritten by David Korn
 *
 *	AT&T Bell Laboratories
 *
 */

/*
 * Modification History
 *
 *	L000	scol!markhe	4 Nov 92
 *	- added cd spell-checking support
 *	  (carried across from previous version)
 *	L001	scol!markhe	11 Nov 92
 *	- declare error messages differently for message catalogues
 *	  (not diff marked)
 *	- added LC_ALL, LC_COLLATE and LC_MESSAGES to builtin names
 *	- messages from ../shlib/strdata.c have been moved into this
 *	  module.
 *	L002	scol!olafvb	1 Jun 93
 *	- added M and H to test_unops string to support additional 
 *	  options to test/[ in test.c
 *	L003	scol!anthonys	29 Jun 94
 *	- Added the "-e" test operator.
 *	L004	scol!anthonys	28 Jun 95
 *	- OPTARG is now handled like other special parameters.
 */

#include	<errno.h>
#include	"defs.h"
#include	"sym.h"
#include	"builtins.h"
#include	"test.h"
#include	"timeout.h"
#include	"history.h"
#include	"msg.h"						/* L001 */

#ifdef MULTIBYTE
#include	"national.h"
    MSG e_version = "\n@(#)Version M-11/16/88g\0\n";
#else
    MSG e_version = "\n@(#)Version 11/16/88g\0\n";
#endif /* MULTIBYTE */

extern struct Bfunction sh_randnum;
extern struct Bfunction sh_seconds;
extern struct Bfunction sh_errno;
extern struct Bfunction line_numbers;

/* error messages */
MSG	ENTRY(e_timewarn, E_TIMEWARN);
MSG	ENTRY(e_timeout, E_TIMEOUT);
MSG	ENTRY(e_mailmsg, E_MAILMSG);
MSG	ENTRY(e_query, E_QUERY);
MSG	ENTRY(e_history, E_HISTORY);
MSG	ENTRY(e_option, E_OPTION);
MSG	ENTRY(e_space, E_SPACE);
MSG	ENTRY(e_argexp, E_ARGEXP);
MSG	ENTRY(e_bracket, E_BRACKET);
MSG	ENTRY(e_number, E_NUMBER);
MSG	ENTRY(e_nullset, E_NULLSET);
MSG	ENTRY(e_notset, E_NOTSET);
MSG	ENTRY(e_subst, E_SUBST);
MSG	ENTRY(e_create, E_CREATE);
MSG	ENTRY(e_restricted, E_RESTRICTED);
MSG	ENTRY(e_fork, E_FORK);
MSG	ENTRY(e_pexists, E_PEXISTS);
MSG	ENTRY(e_fexists, E_FEXISTS);
MSG	ENTRY(e_swap, E_SWAP);
MSG	ENTRY(e_pipe, E_PIPE);
MSG	ENTRY(e_open, E_OPEN);
MSG	ENTRY(e_logout, E_LOGOUT);
MSG	ENTRY(e_arglist, E_ARGLIST);
MSG	ENTRY(e_txtbsy, E_TXTBSY);
MSG	ENTRY(e_toobig, E_TOOBIG);
MSG	ENTRY(e_exec, E_EXEC);
MSG	ENTRY(e_pwd, E_PWD);
MSG	ENTRY(e_found, E_FOUND);
MSG	ENTRY(e_flimit, E_FLIMIT);
MSG	ENTRY(e_ulimit, E_ULIMIT);
MSG	ENTRY(e_subscript, E_SUBSCRIPT);
MSG	ENTRY(e_nargs, E_NARGS);
#ifdef ELIBACC
    /* shared library error messages */
    MSG	ENTRY(e_libacc, E_LIBACC);
    MSG	ENTRY(e_libbad, E_LIBBAD);
    MSG	ENTRY(e_libscn, E_LIBSCN);
    MSG	ENTRY(e_libmax, E_LIBMAX);
#endif	/* ELIBACC */
#ifdef EMULTIHOP
    MSG   ENTRY(e_multihop, E_MULTIHOP);
#endif /* EMULTIHOP */
#ifdef ENAMETOOLONG
    MSG   ENTRY(e_longname, E_LONGNAME);
#endif /* ENAMETOOLONG */
#ifdef ENOLINK
    MSG   ENTRY(e_link, E_LINK);
#endif /* ENOLINK */
MSG	ENTRY(e_access, E_ACCESS);
MSG	ENTRY(e_direct, E_DIRECT);
MSG	ENTRY(e_notdir, E_NOTDIR);
MSG	ENTRY(e_file, E_FILE);
MSG	ENTRY(e_trap, E_TRAP);
MSG	ENTRY(e_readonly, E_READONLY);
MSG	ENTRY(e_ident, E_IDENT);
MSG	ENTRY(e_aliname, E_ALINAME);
MSG	ENTRY(e_testop, E_TESTOP);
MSG	ENTRY(e_alias, E_ALIAS);
MSG	ENTRY(e_function, E_FUNCTION);
MSG	ENTRY(e_format, E_FORMAT);
MSG	ENTRY(e_on, E_ON);
MSG	ENTRY(e_off, E_OFF);
MSG	ENTRY(is_reserved, IS_RESERVED);
MSG	ENTRY(is_builtin, IS_BUILTIN);
MSG	ENTRY(is_alias, IS_ALIAS);
MSG	ENTRY(is_function, IS_FUNCTION);
MSG	ENTRY(is_xalias, IS_XALIAS);
MSG	ENTRY(is_talias, IS_TALIAS);
MSG	ENTRY(is_xfunction, IS_XFUNCTION);
MSG	ENTRY(is_ufunction, IS_UFUNCTION);
MSG	ENTRY(is_, IS_);
MSG	e_fnhdr = "\n{\n";
MSG	e_runvi = "fc -e \"${VISUAL:-${EDITOR:-vi}}\" ";
#ifdef JOBS
#   ifdef SIGTSTP
	MSG	ENTRY(e_newtty, E_NEWTTY);
	MSG	ENTRY(e_oldtty, E_OLDTTY);
	MSG	ENTRY(e_no_start, E_NO_START);
	MSG	ENTRY(e_no_jctl, E_NO_JCTL);
	MSG	ENTRY(e_terminate, E_TERMINATE);
#   endif /*SIGTSTP */
   MSG	ENTRY(e_Done, E_DONE);
   MSG	e_nlspace = "\n      ";
   MSG	ENTRY(e_Running, E_RUNNING);
   MSG	ENTRY(e_ambiguous, E_AMBIGUOUS);
   MSG	ENTRY(e_running, E_RUNNING_J);
   MSG	ENTRY(e_no_job, E_NO_JOB);
   MSG	ENTRY(e_no_proc, E_NO_PROC);
   MSG	ENTRY(e_killcolon, E_KILLCOLON);
   MSG	ENTRY(e_jobusage, E_JOBUSAGE);
   MSG	ENTRY(e_kill, E_KILL);
#endif /* JOBS */
MSG	ENTRY(e_coredump, E_COREDUMP);
#ifdef DEVFD
    MSG	e_devfd = "/dev/fd/";
#endif /* DEVFD */
#ifdef VPIX
    MSG	e_vpix = "/vpix";
    MSG	e_vpixdir = "/usr/bin";
#endif /* VPIX */
#ifdef apollo
    MSG ENTRY(e_rootnode, "Bad root node specification");
    MSG ENTRY(e_nover, "Version not defined");
    MSG ENTRY(e_badver, "Unrecognized version");
#endif /* apollo */
#ifdef LDYNAMIC
    MSG ENTRY(e_badinlib, E_BADINLIB);
#endif /* LDYNAMIC */

/* string constants */
MSG	test_unops = "LHMSVOGCaoherwxdcbfugkpsnzt";    /* L002 */ /* L003 */
MSG	ENTRY(e_heading, E_HEADING);
MSG	e_nullstr = "";
MSG	e_sptbnl = " \t\n";
MSG	e_defpath = "/bin:/usr/bin:";
MSG	e_defedit = "/bin/ed";
MSG	e_colon = ": ";
MSG	e_minus = "-";
MSG	ENTRY(e_endoffile, E_ENDOFFILE);
MSG	ENTRY(e_unexpected, E_UNEXPECTED);
MSG	ENTRY(e_unmatched, E_UNMATCHED);
MSG	ENTRY(e_unknown, E_UNKNOWN);
MSG	ENTRY(e_atline, E_ATLINE);
MSG	e_devnull = "/dev/null";
MSG	e_traceprompt = "+ ";
MSG	e_supprompt = "# ";
MSG	e_stdprompt = "$ ";
MSG	e_profile = "${HOME:-.}/.profile";
MSG	e_sysprofile = "/etc/profile";
MSG	e_suidprofile = "/etc/suid_profile";
MSG	e_crondir = "/usr/spool/cron/atjobs";
#ifndef INT16
   MSG	ENTRY(e_prohibited, "login setuid/setgid shells prohibited");
#endif /* INT16 */
#ifdef SUID_EXEC
   MSG	e_suidexec = "/etc/suid_exec";
#endif /* SUID_EXEC */
MSG	e_devfdNN = "/dev/fd/+([0-9])";
MSG	hist_fname = "/.sh_history";
MSG	ENTRY(e_unlimited, E_UNLIMITED);
#ifdef ECHO_N
   MSG	e_echobin = "/bin/echo";
   MSG	e_echoflag = "-R";
#endif	/* ECHO_N */
MSG	ENTRY(e_test, E_TEST);
MSG	e_dot = ".";
MSG	ENTRY(e_bltfn, E_BLTFN);
MSG	ENTRY(e_intbase, E_INTBASE);
MSG	e_envmarker = "A__z";
#ifdef FLOAT
    MSG	ENTRY(e_precision, E_PRECISION);
#endif /* FLOAT */
#ifdef PDUBIN
        MSG	e_setpwd = "PWD=`/usr/pdu/bin/pwd 2>/dev/null`");
#else
        MSG	e_setpwd = "PWD=`/bin/pwd 2>/dev/null`";
#endif /* PDUBIN */
MSG	ENTRY(e_real, E_REAL);
MSG	ENTRY(e_user, E_USER);
MSG	ENTRY(e_sys , E_SYS);

MSG	ENTRY(e_moretokens, E_MORETOKENS);
MSG	ENTRY(e_paren,	E_PAREN);
MSG	ENTRY(e_badcolon, E_BADCOLON);
MSG	ENTRY(e_divzero, E_DIVZERO);
MSG	ENTRY(e_synbad, E_SYNBAD);
MSG	ENTRY(e_notlvalue, E_NOTLVALUE);
MSG	ENTRY(e_recursive, E_RECURSIVE);

#ifdef	future
    MSG ENTRY(e_questcolon, E_QUESTCOLON);
#endif	/* future */

#ifdef	FLOAT
    MSG	ENTRY(e_incompatible, E_INCOMPATIBLE);
#endif	/* FLOAT */

MSG	e_hdigits = "00112233445566778899aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ";

#ifdef apollo
#   undef NULL
#   define NULL ""
#   define e_nullstr	""
#else
#   ifdef	INTL
#	undef	e_nullstr
#	undef	e_sptbnl
#	define	e_nullstr	""
#	define	e_sptbnl	" \t\n"
#    endif	/* INTL */
#endif	/* apollo */

/* built in names */
const struct name_value node_names[] =
{
	"PATH",		NULL,	0,
	"PS1",		NULL,	0,
	"PS2",		"> ",	N_FREE,
#ifdef apollo
	"IFS",		" \t\n",	N_FREE,
#else
#  ifdef UW
	"IFS",		" \t\n",	N_FREE,
#  else
	"IFS",		e_sptbnl,	N_FREE,
#  endif
#endif	/* apollo */
	"PWD",		NULL,	0,
	"HOME",		NULL,	0,
	"MAIL",		NULL,	0,
	"REPLY",	NULL,	0,
	"SHELL",	"/bin/sh",	N_FREE,
	"EDITOR",	NULL,	0,
#ifdef apollo
	"MAILCHECK",	NULL,	N_FREE|N_INTGER,
	"RANDOM",	NULL,	N_FREE|N_INTGER,
#else
	"MAILCHECK",	(char*)(&sh_mailchk),	N_FREE|N_INTGER,
	"RANDOM",	(char*)(&sh_randnum),	N_FREE|N_INTGER|N_BLTNOD,
#endif	/* apollo */
	"ENV",		NULL,	0,
	"HISTFILE",	NULL,	0,
	"HISTSIZE",	NULL,	0,
	"FCEDIT",	"/bin/ed",	N_FREE,
	"CDPATH",	NULL,	0,
	"MAILPATH",	NULL,	0,
	"PS3",		"#? ",	N_FREE,
	"OLDPWD",	NULL,	0,
	"VISUAL",	NULL,	0,
	"COLUMNS",	NULL,	0,
	"LINES",	NULL,	0,
#ifdef apollo
	"PPID",		NULL,	N_FREE|N_INTGER,
	"_",		NULL,	N_FREE|N_INDIRECT|N_EXPORT,
	"TMOUT",	NULL,	N_FREE|N_INTGER,
	"SECONDS",	NULL,	N_FREE|N_INTGER|N_BLTNOD,
	"ERRNO",	NULL,	N_FREE|N_INTGER|N_BLTNOD,
	"LINENO",	NULL,	N_FREE|N_INTGER|N_BLTNOD,
	"OPTIND",	NULL,	N_FREE|N_INTGER,
	"OPTARG",	NULL,	N_FREE|N_INDIRECT,
#else
	"PPID",		(char*)(&sh.ppid),	N_FREE|N_INTGER,
	"_",		(char*)(&sh.lastarg),	N_FREE|N_INDIRECT|N_EXPORT,
	"TMOUT",	(char*)(&sh_timeout),	N_FREE|N_INTGER,
	"SECONDS",	(char*)(&sh_seconds),	N_FREE|N_INTGER|N_BLTNOD,
	"ERRNO",	(char*)(&sh_errno),	N_FREE|N_INTGER|N_BLTNOD,
	"LINENO",	(char*)(&line_numbers),	N_FREE|N_INTGER|N_BLTNOD,
	"OPTIND",	(char*)(&opt_index),	N_FREE|N_INTGER,
	"OPTARG",	NULL,	N_FREE,				/* L004 */
#endif	/* apollo */
	"PS4",		NULL,	0,
	"FPATH",	NULL,	0,
	"LANG",		NULL,	0,
	"LC_CTYPE",	NULL,	0,
#ifdef VPIX
	"DOSPATH",	NULL,	0,
	"VPIXDIR",	NULL,	0,
#endif	/* VPIX */
#ifdef ACCT
	"SHACCT",	NULL,	0,
#endif	/* ACCT */
#ifdef MULTIBYTE
	"CSWIDTH",	NULL,	0,
#endif /* MULTIBYTE */
#ifdef apollo
	"SYSTYPE",	NULL,	0,
#endif /* apollo */
	"CDSPELL",	NULL,	0,				/* L000 */
#ifdef	INTL							/* L001 begin */
	"LC_ALL",	NULL,	0,
	"LC_COLLATE",	NULL,	0,
	"LC_MESSAGES",	NULL,	0,
#endif	/* INTL */						/* L001 end */
#ifdef UW
	"",		NULL,	0
#else
	e_nullstr,	NULL,	0
#endif
};

#ifdef VPIX
#  ifdef UW
   	const char *suffix_list[] = { ".com", ".exe", ".bat", "" };
#  else
   	const char *suffix_list[] = { ".com", ".exe", ".bat", e_nullstr };
#  endif
#endif	/* VPIX */

/* built in aliases - automatically exported */
const struct name_value alias_names[] =
{
#ifdef FS_3D
	"2d",		"set -f;_2d ",	N_FREE|N_EXPORT,
#endif /* FS_3D */
	"autoload",	"typeset -fu",	N_FREE|N_EXPORT,
#ifdef POSIX
	"command",	"command ",	N_FREE|N_EXPORT,
#endif /* POSIX */
	"functions",	"typeset -f",	N_FREE|N_EXPORT,
/*	"hash",		"alias -t -",	N_FREE|N_EXPORT,	/* L001 */
	"history",	"fc -l",	N_FREE|N_EXPORT,
	"integer",	"typeset -i",	N_FREE|N_EXPORT,
#ifdef POSIX
	"local",	"typeset",	N_FREE|N_EXPORT,
#endif /* POSIX */
	"nohup",	"nohup ",	N_FREE|N_EXPORT,
	"r",		"fc -e -",	N_FREE|N_EXPORT,
	"type",		"whence -v",	N_FREE|N_EXPORT,
#ifdef SIGTSTP
	"stop",		"kill -STOP",	N_FREE|N_EXPORT,
	"suspend",	"kill -STOP $$",	N_FREE|N_EXPORT,
#endif /*SIGTSTP */
#ifdef UW
	"",		NULL,	0
#else
	e_nullstr,	NULL,	0
#endif
};

const struct name_value tracked_names[] =
{
	"cat",		"/bin/cat",	N_FREE|N_EXPORT|T_FLAG,
	"chmod",	"/bin/chmod",	N_FREE|N_EXPORT|T_FLAG,
	"cc",		"/bin/cc",	N_FREE|N_EXPORT|T_FLAG,
	"cp",		"/bin/cp",	N_FREE|N_EXPORT|T_FLAG,
	"date",		"/bin/date",	N_FREE|N_EXPORT|T_FLAG,
	"ed",		"/bin/ed",	N_FREE|N_EXPORT|T_FLAG,
#ifdef _bin_grep_
	"grep",		"/bin/grep",	N_FREE|N_EXPORT|T_FLAG,
#else
#  ifdef _usr_ucb_
	"grep",		"/usr/ucb/grep",N_FREE|N_EXPORT|T_FLAG,
#  endif /* _usr_ucb_ */
#endif	/* _bin_grep */
#ifdef _usr_bin_lp
	"lp",		"/usr/bin/lp",	N_FREE|N_EXPORT|T_FLAG,
#endif /* _usr_bin_lpr */
#ifdef _usr_bin_lpr
	"lpr",		"/usr/bin/lpr",	N_FREE|N_EXPORT|T_FLAG,
#endif /* _usr_bin_lpr */
	"ls",		"/bin/ls",	N_FREE|N_EXPORT|T_FLAG,
	"make",		"/bin/make",	N_FREE|N_EXPORT|T_FLAG,
	"mail",		"/bin/mail",	N_FREE|N_EXPORT|T_FLAG,
	"mv",		"/bin/mv",	N_FREE|N_EXPORT|T_FLAG,
	"pr",		"/bin/pr",	N_FREE|N_EXPORT|T_FLAG,
	"rm",		"/bin/rm",	N_FREE|N_EXPORT|T_FLAG,
	"sed",		"/bin/sed",	N_FREE|N_EXPORT|T_FLAG,
	"sh",		"/bin/sh",	N_FREE|N_EXPORT|T_FLAG,
#ifdef _usr_bin_vi_
	"vi",		"/usr/bin/vi",	N_FREE|N_EXPORT|T_FLAG,
#else
#  ifdef _usr_ucb_
	"vi",		"/usr/ucb/vi",	N_FREE|N_EXPORT|T_FLAG,
#  endif /* _usr_ucb_ */
#endif	/* _usr_bin_vi_ */
	"who",		"/bin/who",	N_FREE|N_EXPORT|T_FLAG,
#ifdef UW
	"",		NULL,	0
#else
	e_nullstr,	NULL,	0
#endif
};

/* tables */
SYSTAB tab_reserved =
{
#ifdef POSIX
		{"!",		NOTSYM},
#endif /* POSIX */
#ifdef NEWTEST
		{"[[",		BTSTSYM},
#endif /* NEWTEST */
		{"case",	CASYM},
		{"do",		DOSYM},
		{"done",	ODSYM},
		{"elif",	EFSYM},
		{"else",	ELSYM},
		{"esac",	ESSYM},
		{"fi",		FISYM},
		{"for",		FORSYM},
		{"function",	PROCSYM},
		{"if",		IFSYM},
		{"in",		INSYM},
		{"select",	SELSYM},
		{"then",	THSYM},
		{"time",	TIMSYM},
		{"until",	UNSYM},
		{"while",	WHSYM},
		{"{",		BRSYM},
		{"}",		KTSYM},
#ifdef UW
		{"",		0}
#else
		{e_nullstr,	0}
#endif
};

/*
 * The signal numbers go in the low bits and the attributes go in the high bits
 */

SYSTAB	sig_names =
{
#ifdef SIGABRT
		{"ABRT",	(SIGABRT+1)|(SIGDONE<<SIGBITS)},
#endif /*SIGABRT */
		{"ALRM",	(SIGALRM+1)|((SIGCAUGHT|SIGFAULT)<<SIGBITS)},
#ifdef SIGAPOLLO
		{"APOLLO",	(SIGAPOLLO+1)},
#endif /* SIGAPOLLO */
#ifdef SIGBUS
		{"BUS",		(SIGBUS+1)|(SIGDONE<<SIGBITS)},
#endif /* SIGBUS */
#ifdef SIGCHLD
		{"CHLD",	(SIGCHLD+1)|((SIGCAUGHT|SIGFAULT)<<SIGBITS)},
#   ifdef SIGCLD
#	if SIGCLD!=SIGCHLD
		{"CLD",		(SIGCLD+1)|((SIGCAUGHT|SIGFAULT)<<SIGBITS)},
#	endif
#   endif	/* SIGCLD */
#else
#   ifdef SIGCLD
		{"CLD",		(SIGCLD+1)|((SIGCAUGHT|SIGFAULT)<<SIGBITS)},
#   endif	/* SIGCLD */
#endif	/* SIGCHLD */
#ifdef SIGCONT
		{"CONT",	(SIGCONT+1)},
#endif	/* SIGCONT */
		{"DEBUG",	(DEBUGTRAP+1)},
#ifdef SIGEMT
		{"EMT",		(SIGEMT+1)|(SIGDONE<<SIGBITS)},
#endif	/* SIGEMT */
		{"ERR",		(ERRTRAP+1)},
		{"EXIT",	1},
		{"FPE",		(SIGFPE+1)|(SIGDONE<<SIGBITS)},
		{"HUP",		(SIGHUP+1)|(SIGDONE<<SIGBITS)},
		{"ILL",		(SIGILL+1)|(SIGDONE<<SIGBITS)},
		{"INT",		(SIGINT+1)|(SIGCAUGHT<<SIGBITS)},
#ifdef SIGIO
		{"IO",		(SIGIO+1)},
#endif	/* SIGIO */
#ifdef SIGIOT
		{"IOT",		(SIGIOT+1)|(SIGDONE<<SIGBITS)},
#endif	/* SIGIOT */
		{"KILL",	(SIGKILL+1)},
#ifdef SIGLAB
		{"LAB",		(SIGLAB+1)},
#endif	/* SIGLAB */
#ifdef SIGLOST
		{"LOST",	(SIGLOST+1)},
#endif	/* SIGLOST */
#ifdef SIGPHONE
		{"PHONE",	(SIGPHONE+1)},
#endif	/* SIGPHONE */
#ifdef SIGPIPE
		{"PIPE",	(SIGPIPE+1)|(SIGDONE<<SIGBITS)},
#endif	/* SIGPIPE */
#ifdef SIGPOLL
		{"POLL",	(SIGPOLL+1)},
#endif	/* SIGPOLL */
#ifdef SIGPROF
		{"PROF",	(SIGPROF+1)},
#endif	/* SIGPROF */
#ifdef SIGPWR
#   if SIGPWR>0
		{"PWR",		(SIGPWR+1)},
#   endif
#endif	/* SIGPWR */
		{"QUIT",	(SIGQUIT+1)|((SIGCAUGHT|SIGIGNORE)<<SIGBITS)},
		{"SEGV",	(SIGSEGV+1)},
#ifdef SIGSTOP
		{"STOP",	(SIGSTOP+1)},
#endif	/* SIGSTOP */
#ifdef SIGSYS
		{"SYS",		(SIGSYS+1)|(SIGDONE<<SIGBITS)},
#endif	/* SIGSYS */
		{"TERM",	(SIGTERM+1)|(SIGDONE<<SIGBITS)},
#ifdef SIGTINT
		{"TINT",	(SIGTINT+1)},
#endif	/* SIGTINT */
#ifdef SIGTRAP
		{"TRAP",	(SIGTRAP+1)|(SIGDONE<<SIGBITS)},
#endif	/* SIGTRAP */
#ifdef SIGTSTP
		{"TSTP",	(SIGTSTP+1)},
#endif	/* SIGTSTP */
#ifdef SIGTTIN
		{"TTIN",	(SIGTTIN+1)},
#endif	/* SIGTTIN */
#ifdef SIGTTOU
		{"TTOU",	(SIGTTOU+1)},
#endif	/* SIGTTOU */
#ifdef SIGURG
		{"URG",		(SIGURG+1)},
#endif	/* SIGURG */
#ifdef SIGUSR1
		{"USR1",	(SIGUSR1+1)|(SIGDONE<<SIGBITS)},
#endif	/* SIGUSR1 */
#ifdef SIGUSR2
		{"USR2",	(SIGUSR2+1)|(SIGDONE<<SIGBITS)},
#endif	/* SIGUSR2 */
#ifdef SIGVTALRM
		{"VTALRM",	(SIGVTALRM+1)},
#endif	/* SIGVTALRM */
#ifdef SIGWINCH
		{"WINCH",	(SIGWINCH+1)},
#endif	/* SIGWINCH */
#ifdef SIGWINDOW
		{"WINDOW",	(SIGWINDOW+1)},
#endif	/* SIGWINDOW */
#ifdef SIGWIND
		{"WIND",	(SIGWIND+1)},
#endif	/* SIGWIND */
#ifdef SIGXCPU
		{"XCPU",	(SIGXCPU+1)},
#endif	/* SIGXCPU */
#ifdef SIGXFSZ
		{"XFSZ",	(SIGXFSZ+1)|((SIGCAUGHT|SIGIGNORE)<<SIGBITS)},
#endif	/* SIGXFSZ */
#ifdef UW
		{"",		0}
#else
		{e_nullstr,	0}
#endif
};

SYSTAB	sig_messages =
{
#ifdef SIGABRT
		{S_ABRT,			(SIGABRT+1)},
#endif /*SIGABRT */
		{S_ALRM,			(SIGALRM+1)},
#ifdef SIGBUS
		{S_BUS,				(SIGBUS+1)},
#endif /* SIGBUS */
#ifdef SIGCHLD
		{S_CHLD,			(SIGCHLD+1)},
#   ifdef SIGCLD
#	if SIGCLD!=SIGCHLD
		{S_CLD,		 		(SIGCLD+1)},
#	endif
#   endif	/* SIGCLD */
#else
#   ifdef SIGCLD
		{S_CLD,		 		(SIGCLD+1)},
#   endif	/* SIGCLD */
#endif	/* SIGCHLD */
#ifdef SIGCONT
		{S_CONT,			(SIGCONT+1)},
#endif	/* SIGCONT */
#ifdef SIGEMT
		{S_EMT,				(SIGEMT+1)},
#endif	/* SIGEMT */
		{S_FPE,				(SIGFPE+1)},
		{S_HUP,				(SIGHUP+1)},
		{S_ILL,				(SIGILL+1)},
#ifdef JOBS
		{S_INT,				(SIGINT+1)},
#else
		{e_nullstr,			(SIGINT+1)},
#endif	/* JOBS */
#ifdef SIGIO
		{S_IO,				(SIGIO+1)},
#endif	/* SIGIO */
#ifdef SIGIOT
		{S_IOT,				(SIGIOT+1)},
#endif	/* SIGIOT */
		{S_KILL,			(SIGKILL+1)},
		{S_QUIT,			(SIGQUIT+1)},
#ifdef JOBS
		{S_PIPE,			(SIGPIPE+1)},
#else
		{e_nullstr,			(SIGPIPE+1)},
#endif	/* JOBS */
#ifdef SIGPROF
		{S_PROF,			(SIGPROF+1)},
#endif	/* SIGPROF */
#ifdef SIGPWR
#   if SIGPWR>0
		{S_PWR,				(SIGPWR+1)},
#   endif
#endif	/* SIGPWR */
		{S_SEGV,			(SIGSEGV+1)},
#ifdef SIGSTOP
		{S_STOP,			(SIGSTOP+1)},
#endif	/* SIGSTOP */
#ifdef SIGSYS
		{S_SYS,				(SIGSYS+1)},
#endif	/* SIGSYS */
		{S_TERM,			(SIGTERM+1)},
#ifdef SIGTINT
#   ifdef JOBS
		{S_TINT,			(SIGTINT+1)},
#   else
		{e_nullstr,			(SIGTINT+1)},
#   endif /* JOBS */
#endif	/* SIGTINT */
#ifdef SIGTRAP
		{S_TRAP,			(SIGTRAP+1)},
#endif	/* SIGTRAP */
#ifdef SIGTSTP
		{S_TSTP,			(SIGTSTP+1)},
#endif	/* SIGTSTP */
#ifdef SIGTTIN
		{S_TTIN,			(SIGTTIN+1)},
#endif	/* SIGTTIN */
#ifdef SIGTTOU
		{S_TTOU,			(SIGTTOU+1)},
#endif	/* SIGTTOU */
#ifdef SIGURG
		{S_URG,				(SIGURG+1)},
#endif	/* SIGURG */
#ifdef SIGUSR1
		{S_USR1,			(SIGUSR1+1)},
#endif	/* SIGUSR1 */
#ifdef SIGUSR2
		{S_USR2,			(SIGUSR2+1)},
#endif	/* SIGUSR2 */
#ifdef SIGVTALRM
		{S_VTALRM,			(SIGVTALRM+1)},
#endif	/* SIGVTALRM */
#ifdef SIGWINCH
		{S_WINCH, 			(SIGWINCH+1)},
#endif	/* SIGWINCH */
#ifdef SIGXCPU
		{S_XCPU,			(SIGXCPU+1)},
#endif	/* SIGXCPU */
#ifdef SIGXFSZ
		{S_XFSZ,			(SIGXFSZ+1)},
#endif	/* SIGXFSZ */
#ifdef SIGLOST
		{S_LOST,	 		(SIGLOST+1)},
#endif	/* SIGLOST */
#ifdef SIGLAB
		{S_LAB,				(SIGLAB+1)},
#endif	/* SIGLAB */
#ifdef UW
		{"",		0}
#else
		{e_nullstr,	0}
#endif
};

SYSTAB tab_options=
{
	{"allexport",		Allexp},
	{"bgnice",		Bgnice},
	{"emacs",		Emacs},
	{"errexit",		Errflg},
	{"gmacs",		Gmacs},
	{"ignoreeof",		Noeof},
	{"interactive",		Intflg},
	{"keyword",		Keyflg},
	{"markdirs",		Markdir},
	{"monitor",		Monitor},
	{"noexec",		Noexec},
	{"noclobber",		Noclob},
	{"noglob",		Noglob},
	{"nolog",		Nolog},
	{"nounset",		Noset},
#ifdef apollo
	{"physical",		Aphysical},
#endif /* apollo */
	{"privileged",		Privmod},
	{"restricted",		Rshflg},
	{"trackall",		Hashall},
	{"verbose",		Readpr},
	{"vi",			Editvi},
	{"viraw",		Viraw},
	{"xtrace",		Execpr},
#ifdef UW
	{"",			0}
#else
	{e_nullstr,		0}
#endif
};

#ifdef _sys_resource_
#   ifndef included_sys_time_
#	include <sys/time.h>
#   endif
#   include	<sys/resource.h>/* needed for ulimit */
#   define	LIM_FSIZE	RLIMIT_FSIZE
#   define	LIM_DATA	RLIMIT_DATA
#   define	LIM_STACK	RLIMIT_STACK
#   define	LIM_CORE	RLIMIT_CORE
#   define	LIM_CPU		RLIMIT_CPU
#   ifdef RLIMIT_RSS
#	define	LIM_MAXRSS	RLIMIT_RSS
#   endif /* RLIMIT_RSS */
#else
#   ifdef VLIMIT
#	include	<sys/vlimit.h>
#   endif /* VLIMIT */
#endif	/* _sys_resource_ */

#ifdef LIM_CPU
#   define size_resource(a,b) ((a)|((b)<<11))	
SYSTAB limit_names =
{
	{"time(seconds)       ",	size_resource(1,LIM_CPU)},
	{"file(blocks)        ",	size_resource(512,LIM_FSIZE)},
	{"data(kbytes)        ",	size_resource(1024,LIM_DATA)},
	{"stack(kbytes)       ",	size_resource(1024,LIM_STACK)},
#   ifdef LIM_MAXRSS
	{"memory(kbytes)      ",	size_resource(1024,LIM_MAXRSS)},
#   else
	{"memory(kbytes)      ",	size_resource(1024,0)},
#   endif /* LIM_MAXRSS */
	{"coredump(blocks)    ",	size_resource(512,LIM_CORE)},
#   ifdef RLIMIT_NOFILE
	{"nofiles(descriptors)",	size_resource(1,RLIMIT_NOFILE)},
#   else
	{"nofiles(descriptors)",	size_resource(1,0)},
#   endif /* RLIMIT_NOFILE */
#   ifdef RLIMIT_VMEM
	{"vmemory(kbytes)     ",	size_resource(1024,RLIMIT_VMEM)}
#   else
	{"vmemory(kbytes)     ",	size_resource(1024,0)}
#   endif /* RLIMIT_VMEM */
};
#endif	/* LIM_CPU */

#ifdef cray
    const struct name_fvalue built_ins[] =
#   define VALPTR(x)	x
#else
#   define VALPTR(x)	((char*)x)
    const struct name_value built_ins[] =
#endif /* cray */
{
		{"login",	VALPTR(b_login),	N_BLTIN|BLT_ENV},
		{"exec",	VALPTR(b_exec),		N_BLTIN|BLT_ENV},
		{"set",		VALPTR(b_set),		N_BLTIN},
		{":",		VALPTR(b_null),		N_BLTIN|BLT_SPC},
		{"true",	VALPTR(b_null),		N_BLTIN},
#ifdef _bin_newgrp_
		{"newgrp",	VALPTR(b_login),	N_BLTIN|BLT_ENV},
#endif	/* _bin_newgrp_ */
		{"false",	VALPTR(b_null),		N_BLTIN},
#ifdef apollo
		{"rootnode",	VALPTR(b_rootnode),	N_BLTIN},
		{"ver",		VALPTR(b_ver),		N_BLTIN},
#endif	/* apollo */
#ifdef LDYNAMIC
		{"inlib",	VALPTR(b_inlib),	N_BLTIN},
#   ifndef apollo
		{"builtin",	VALPTR(b_builtin),	N_BLTIN},
#   endif	/* !apollo */
#endif	/* LDYNAMIC */
		{".",		VALPTR(b_dot),		N_BLTIN|BLT_SPC|BLT_FSUB},
		{"readonly",	VALPTR(b_readonly),	N_BLTIN|BLT_SPC|BLT_DCL},
		{"typeset",	VALPTR(b_typeset),	N_BLTIN|BLT_SPC|BLT_DCL},
		{"return",	VALPTR(b_ret_exit),	N_BLTIN|BLT_SPC},
		{"export",	VALPTR(b_export),	N_BLTIN|BLT_SPC|BLT_DCL},
		{"eval",	VALPTR(b_eval),		N_BLTIN|BLT_SPC|BLT_FSUB},
		{"fc",		VALPTR(b_fc),		N_BLTIN|BLT_FSUB},
		{"shift",	VALPTR(b_shift),	N_BLTIN|BLT_SPC},
		{"cd",		VALPTR(b_chdir),	N_BLTIN},
#ifdef OLDTEST
		{"[",		VALPTR(b_test),		N_BLTIN},
#endif /* OLDTEST */
		{ "alias",	VALPTR(b_alias),	N_BLTIN|BLT_SPC|BLT_DCL},
		{"break",	VALPTR(b_break),	N_BLTIN|BLT_SPC},
		{"continue",	VALPTR(b_continue),	N_BLTIN|BLT_SPC},
#ifdef ECHOPRINT
		{"echo",	VALPTR(b_print),	N_BLTIN},
#else
		{"echo",	VALPTR(b_echo),		N_BLTIN},
#endif /* ECHOPRINT */
		{"exit",	VALPTR(b_ret_exit),	N_BLTIN|BLT_SPC},
#ifdef JOBS
# ifdef SIGTSTP
		{"bg",		VALPTR(b_bgfg),		N_BLTIN},
		{"fg",		VALPTR(b_bgfg),		N_BLTIN},
		{"hash",	VALPTR(b_hash),		N_BLTIN},   /* L001 */
# endif	/* SIGTSTP */
		{"jobs",	VALPTR(b_jobs),		N_BLTIN},
		{"kill",	VALPTR(b_kill),		N_BLTIN},
#endif	/* JOBS */
		{"let",		VALPTR(b_let),		N_BLTIN},
		{"print",	VALPTR(b_print),	N_BLTIN},
		{"pwd",		VALPTR(b_pwd),		N_BLTIN},
		{"read",	VALPTR(b_read),		N_BLTIN},
#ifdef SYSCOMPILE
		{"shcomp",	VALPTR(b_shcomp),	N_BLTIN},
#endif /* SYSCOMPILE */
#ifdef SYSSLEEP
		{"sleep",	VALPTR(b_sleep),	N_BLTIN},
#endif /* SYSSLEEP */
#ifdef OLDTEST
		{"test",	VALPTR(b_test),		N_BLTIN},
#endif /* OLDTEST */
		{"times",	VALPTR(b_times),	N_BLTIN|BLT_SPC},
		{"trap",	VALPTR(b_trap),		N_BLTIN|BLT_SPC},
		{"ulimit",	VALPTR(b_ulimit),	N_BLTIN},
		{"umask",	VALPTR(b_umask),	N_BLTIN},
		{"unalias",	VALPTR(b_unalias),	N_BLTIN},
		{"unset",	VALPTR(b_unset),	N_BLTIN},
		{"wait",	VALPTR(b_wait),		N_BLTIN|BLT_SPC},
		{"whence",	VALPTR(b_whence),	N_BLTIN},
		{"getopts",	VALPTR(b_getopts),	N_BLTIN},
#ifdef UNIVERSE
		{"universe",	VALPTR(b_universe),	N_BLTIN},
#endif /* UNIVERSE */
#ifdef FS_3D
		{"vpath",	VALPTR(b_vpath_map),	N_BLTIN},
		{"vmap",	VALPTR(b_vpath_map),	N_BLTIN},
#endif /* FS_3D */
#ifdef UW
		{"",			0, 0 }
#else
		{e_nullstr,		0, 0 }
#endif
};

SYSTAB	test_optable =
{
		{"!=",		TEST_SNE},
		{"-a",		TEST_AND},
		{"-ef",		TEST_EF},
		{"-eq",		TEST_EQ},
		{"-ge",		TEST_GE},
		{"-gt",		TEST_GT},
		{"-le",		TEST_LE},
		{"-lt",		TEST_LT},
		{"-ne",		TEST_NE},
		{"-nt",		TEST_NT},
		{"-o",		TEST_OR},
		{"-ot",		TEST_OT},
		{"=",		TEST_SEQ},
		{"==",		TEST_SEQ},
#ifdef NEWTEST
		{"<",		TEST_SLT},
		{">",		TEST_SGT},
		{"]]",		TEST_END},
#endif /* NEWTEST */
#ifdef UW
		{"",		0}
#else
		{e_nullstr,	0}
#endif
};

SYSTAB	tab_attributes =
{
		{"export",	N_EXPORT},
		{"readonly",	N_RDONLY},
		{"tagged",	T_FLAG},
#ifdef FLOAT
		{"exponential",	(N_DOUBLE|N_INTGER|N_EXPNOTE)},
		{"float",	(N_DOUBLE|N_INTGER)},
#endif /* FLOAT */
		{"long",	(L_FLAG|N_INTGER)},
		{"unsigned",	(N_UNSIGN|N_INTGER)},
		{"function",	(N_BLTNOD|N_INTGER)},
		{"integer",	N_INTGER},
		{"filename",	N_HOST},
		{"lowercase",	N_UTOL},
		{"zerofill",	N_ZFILL},
		{"leftjust",	N_LJUST},
		{"rightjust",	N_RJUST},
		{"uppercase",	N_LTOU},
#ifdef UW
		{"",	0}
#else
		{e_nullstr,	0}
#endif
};


#ifndef IODELAY
#   undef _SELECT5_
#endif /* IODELAY */
#ifdef _sgtty_
#   ifdef _SELECT5_
	const int tty_speeds[] = {0, 50, 75, 110, 134, 150, 200, 300,
			600,1200,1800,2400,9600,19200,0};
#   endif /* _SELECT5_ */
#endif /* _sgtty_ */
