#ident	"@(#)OSRcmds:ksh/sh/catload.c	1.1"
#pragma comment(exestr, "@(#) catload.c 25.2 94/04/14 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1992-1994.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/*
 *  Modification History
 *
 *	created		scol!markhe	30 Nov 92
 *	- if built internationlaized, catload() is called when the
 *	  shell starts up and each time LANG, LC_MESSGAES or LC_ALL is changed
 */

#ifdef	INTL

#include	"defs.h"
#include	"builtins.h"
#include	"test.h"
#include	"history.h"
#include	"jobs.h"
#include	"sym.h"
#undef	LPAREN		/* these are also defined in streval.h, but neither */
#undef	RPAREN		/* is used in this module, so is safe to undef them */
#include	"timeout.h"
#include	"streval.h"
#include	"msg.h"

/*
 * if the new message catalogue cannot be opened, should we carry on
 * using the old one or fall-back and use the builtin messages?
 * at present the defaults are used, but uncommenting the line
 * below will give the alternative action
 */
/* #define	KEEP_OLD */

#ifdef	KEEP_OLD
static int	once;
#endif	/* KEEP_OLD */


int
catload(void)
{
	register int i = 0;
#ifdef	KEEP_OLD
	int	temp_catd = catd;
#else
	/*
	 * iff message catalogue is open, close it!
	 */
	if ((int)catd != (int)-1)
		catclose(catd);
#endif	/* KEEP_OLD */

	catd = catopen(MF_KSH, MC_FLAGS);

#ifdef	KEEP_OLD
	/*
	 * if first time, we must initialise
	 * the message strings, even if we
	 * cannot open a catalogue
	 */
	if (once) {
		once++;
		if (catd == -1) {
			catd = temp_catd;
			return(-1);
		} else
			if (temp_catd != -1)
				catclose(temp_catd);
	}
#endif	/* KEEP_OLD */

	/*
	 * this looks ugly, but keeping original variable names (rather
	 * than sticking them in an array - which would turn loading into
	 * a for(;;) loop) reduces the number of changes needed in
	 * other modules
	 */
	e_timewarn = MSGSTR(KSH_E_TIMEWARN, E_TIMEWARN);
	e_timeout = MSGSTR(KSH_E_TIMEOUT, E_TIMEOUT);
	e_mailmsg = MSGSTR(KSH_E_MAILMSG, E_MAILMSG);
	e_query = MSGSTR(KSH_E_QUERY, E_QUERY);
	e_history = MSGSTR(KSH_E_HISTORY, E_HISTORY);
	e_option = MSGSTR(KSH_E_OPTION, E_OPTION);
	e_space = MSGSTR(KSH_E_SPACE, E_SPACE);
	e_argexp = MSGSTR(KSH_E_ARGEXP, E_ARGEXP);
	e_bracket = MSGSTR(KSH_E_BRACKET, E_BRACKET);
	e_number = MSGSTR(KSH_E_NUMBER, E_NUMBER);
	e_nullset = MSGSTR(KSH_E_NULLSET, E_NULLSET);
	e_notset = MSGSTR(KSH_E_NOTSET, E_NOTSET);
	e_subst = MSGSTR(KSH_E_SUBST, E_SUBST);
	e_create = MSGSTR(KSH_E_CREATE, E_CREATE);
	e_restricted = MSGSTR(KSH_E_RESTRICTED, E_RESTRICTED);
	e_fork = MSGSTR(KSH_E_FORK, E_FORK);
	e_pexists = MSGSTR(KSH_E_PEXISTS, E_PEXISTS);
	e_fexists = MSGSTR(KSH_E_FEXISTS, E_FEXISTS);
	e_swap = MSGSTR(KSH_E_SWAP, E_SWAP);
	e_pipe = MSGSTR(KSH_E_PIPE, E_PIPE);
	e_open = MSGSTR(KSH_E_OPEN, E_OPEN);
	e_logout = MSGSTR(KSH_E_LOGOUT, E_LOGOUT);
	e_arglist = MSGSTR(KSH_E_ARGLIST, E_ARGLIST);
	e_txtbsy = MSGSTR(KSH_E_TXTBSY, E_TXTBSY);
	e_toobig = MSGSTR(KSH_E_TOOBIG, E_TOOBIG);
	e_exec = MSGSTR(KSH_E_EXEC, E_EXEC);
	e_pwd = MSGSTR(KSH_E_PWD, E_PWD);
	e_found = MSGSTR(KSH_E_FOUND, E_FOUND);
	e_flimit = MSGSTR(KSH_E_FLIMIT, E_FLIMIT);
	e_ulimit = MSGSTR(KSH_E_ULIMIT, E_ULIMIT);
	e_subscript = MSGSTR(KSH_E_SUBSCRIPT, E_SUBSCRIPT);
	e_nargs = MSGSTR(KSH_E_NARGS, E_NARGS);
#ifdef	ELIBACC
	/* sahred library error messgaes */
	e_libacc = MSGSTR(KSH_E_LIBACC, E_LIBACC);
	e_libbad = MSGSTR(KSH_E_LIBBAD, E_LIBBAD);
	e_Libscn = MSGSTR(KSH_E_LIBSCN, E_LIBSCN);
	e_libmax = MSGSTR(KSH_E_LIBMAX, E_LIBMAX);
#endif	/* ELIBACC */

#ifdef	EMULTIHOP
	e_multihop = MSGSTR(KSH_E_MULTIHOP, E_MULTIHOP);
#endif	/* EMULTIHOP */

#ifdef	ENAMETOOLONG
	e_longname = MSGSTR(KSH_E_LONGNAME, E_LONGNAME);
#endif	/* ENAMETOOLONG */

#ifdef	ENOLINK
	e_link = MSGSTR(KSH_E_LINK, E_LINK);
#endif	/* ENOLINK */

	e_access = MSGSTR(KSH_E_ACCESS, E_ACCESS);
	e_direct = MSGSTR(KSH_E_DIRECT, E_DIRECT);
	e_notdir = MSGSTR(KSH_E_NOTDIR, E_NOTDIR);
	e_file = MSGSTR(KSH_E_FILE, E_FILE);
	e_trap = MSGSTR(KSH_E_TRAP, E_TRAP);
	e_readonly = MSGSTR(KSH_E_READONLY, E_READONLY);
	e_ident = MSGSTR(KSH_E_IDENT, E_IDENT);
	e_aliname = MSGSTR(KSH_E_ALINAME, E_ALINAME);
	e_testop = MSGSTR(KSH_E_TESTOP, E_TESTOP);
	e_alias = MSGSTR(KSH_E_ALIAS, E_ALIAS);
	e_function = MSGSTR(KSH_E_FUNCTION, E_FUNCTION);
	e_format = MSGSTR(KSH_E_FORMAT, E_FORMAT);
	e_on = MSGSTR(KSH_E_ON, E_ON);
	e_off = MSGSTR(KSH_E_OFF, E_OFF);
	is_reserved = MSGSTR(KSH_IS_RESERVED, IS_RESERVED);
	is_builtin = MSGSTR(KSH_IS_BUILTIN, IS_BUILTIN);
	is_alias = MSGSTR(KSH_IS_ALIAS, IS_ALIAS);
	is_function = MSGSTR(KSH_IS_FUNCTION, IS_FUNCTION);
	is_xalias = MSGSTR(KSH_IS_XALIAS, IS_XALIAS);
	is_talias = MSGSTR(KSH_IS_TALIAS, IS_TALIAS);
	is_xfunction = MSGSTR(KSH_IS_XFUNCTION, IS_XFUNCTION);
	is_ufunction = MSGSTR(KSH_IS_UFUNCTION, IS_UFUNCTION);

#ifdef	JOBS
#  ifdef	SIGTSTP
	is_ = MSGSTR(KSH_IS_, IS_);
	e_no_start = MSGSTR(KSH_E_NO_START, E_NO_START);
	e_no_jctl = MSGSTR(KSH_E_NO_JCTL, E_NO_JCTL);
	e_terminate = MSGSTR(KSH_E_TERMINATE, E_TERMINATE);
#  endif	/* SIGTSP */
	e_Done = MSGSTR(KSH_E_DONE, E_DONE);
	e_Running = MSGSTR(KSH_E_RUNNING, E_RUNNING);
	e_ambiguous = MSGSTR(KSH_E_AMBIGUOUS, E_AMBIGUOUS);
	e_running = MSGSTR(KSH_E_RUNNING_J, E_RUNNING_J);
	e_no_job = MSGSTR(KSH_E_NO_JOB, E_NO_JOB);
	e_no_proc = MSGSTR(KSH_E_NO_PROC, E_NO_PROC);
	e_killcolon = MSGSTR(KSH_E_KILLCOLON, E_KILLCOLON);
	e_jobusage = MSGSTR(KSH_E_JOBUSAGE, E_JOBUSAGE);
	e_kill = MSGSTR(KSH_E_KILL, E_KILL);
#endif	/* JOBS */

	e_coredump = MSGSTR(KSH_E_COREDUMP, E_COREDUMP);

#ifdef	LDYNAMIC
	e_badinlib = MSGSTR(KSH_E_BADINLIB, E_BADINLIB);
#endif	/* LDYNAMIC */

	e_heading = MSGSTR(KSH_E_HEADING, E_HEADING);
	e_endoffile = MSGSTR(KSH_E_ENDOFFILE, E_ENDOFFILE);
	e_unexpected = MSGSTR(KSH_E_UNEXPECTED, E_UNEXPECTED);
	e_unmatched = MSGSTR(KSH_E_UNMATCHED, E_UNMATCHED);
	e_unknown = MSGSTR(KSH_E_UNKNOWN, E_UNKNOWN);
	e_atline = MSGSTR(KSH_E_ATLINE, E_ATLINE);

#ifndef	INT16
	e_prohibited = MSGSTR(KSH_E_PROHIBITED, E_PROHIBITED);
#endif	/* INT16 */

	e_unlimited = MSGSTR(KSH_E_UNLIMITED, E_UNLIMITED);
	e_test = MSGSTR(KSH_E_TEST, E_TEST);
	e_bltfn = MSGSTR(KSH_E_BLTFN, E_BLTFN);
	e_intbase = MSGSTR(KSH_E_INTBASE, E_INTBASE);
	e_real = MSGSTR(KSH_E_REAL, E_REAL);
	e_user = MSGSTR(KSH_E_USER, E_USER);
	e_sys = MSGSTR(KSH_E_SYS, E_SYS);

	e_moretokens = MSGSTR(KSH_E_MORETOKENS, E_MORETOKENS);
	e_paren = MSGSTR(KSH_E_PAREN, E_PAREN);
	e_badcolon = MSGSTR(KSH_E_BADCOLON, E_BADCOLON);
	e_divzero = MSGSTR(KSH_E_DIVZERO, E_DIVZERO);
	e_synbad = MSGSTR(KSH_E_SYNBAD, E_SYNBAD);
	e_notlvalue = MSGSTR(KSH_E_NOTLVALUE, E_NOTLVALUE);
	e_recursive = MSGSTR(KSH_E_RECURSIVE, E_RECURSIVE);

/* signal messages */

#ifdef	SIGABRT
	sig_messages[i++].sysnam = MSGSTR(KSH_S_ABRT, S_ABRT);
#endif	/* SIGABRT */

	sig_messages[i++].sysnam = MSGSTR(KSH_S_ALRM, S_ALRM);

#ifdef	SIGBUS
	sig_messages[i++].sysnam = MSGSTR(KSH_S_BUS, S_BUS);
#endif	/* SIGBUS */

#ifdef	SIGCHLD
	sig_messages[i++].sysnam = MSGSTR(KSH_S_CHLD, S_CHLD);
#   ifdef SIGCLD
#      if SIGCLD != SIGCHLD
	sig_messages[i++].sysnam = MSGSTR(KSH_S_CLD, S_CLD);
#      endif
#   endif /* SIGCLD */
#else
#   ifdef SIGCLD
	sig_messages[i++].sysnam = MSGSTR(KSH_S_CLD, S_CLD);
#   endif /* SIGCLD */
#endif	/* SIGCHLD */

	sig_messages[i++].sysnam = MSGSTR(KSH_S_CONT, S_CONT);
#ifdef	SIGEMT
	sig_messages[i++].sysnam = MSGSTR(KSH_S_EMT, S_EMT);
#endif /* ISGEMT */

	sig_messages[i++].sysnam = MSGSTR(KSH_S_FPE, S_FPE);
	sig_messages[i++].sysnam = MSGSTR(KSH_S_HUP, S_HUP);
	sig_messages[i++].sysnam = MSGSTR(KSH_S_ILL, S_ILL);

#ifdef	JOBS
	sig_messages[i++].sysnam = MSGSTR(KSH_S_INT, S_INT);
#else
	sig_messages[i++].sysnam = e_nullstr;
#endif	/* JOBS */

#ifdef	SIGIO
	sig_messages[i++].sysnam = MSGSTR(KSH_S_IO, S_IO);
#endif	/* SIGIO */

#ifdef	SIGIOT
	sig_messages[i++].sysnam = MSGSTR(KSH_S_IOT, S_IOT);
#endif	/* SIGIOT */

	sig_messages[i++].sysnam = MSGSTR(KSH_S_KILL, S_KILL);
	sig_messages[i++].sysnam = MSGSTR(KSH_S_QUIT, S_QUIT);

#ifdef	JOBS
	sig_messages[i++].sysnam = MSGSTR(KSH_S_PIPE, E_PIPE);
#else
	sig_messages[i++].sysnam = e_nullstr;
#endif	/* JOBS */

#ifdef	SIGPROF
	sig_messages[i++].sysnam = MSGSTR(KSH_S_PROF, S_PROF);
#endif	/* SIGPROF */

#ifdef	SIGPWR
#   if SIGPWR >0
	sig_messages[i++].sysnam = MSGSTR(KSH_S_PWR, S_PWR);
#   endif
#endif	/* SIGPWR */

	sig_messages[i++].sysnam = MSGSTR(KSH_S_SEGV, S_SEGV);

#ifdef	SIGSTOP
	sig_messages[i++].sysnam = MSGSTR(KSH_S_STOP, S_STOP);
#endif	/* SIGSTOP */

#ifdef	SIGSYS
	sig_messages[i++].sysnam = MSGSTR(KSH_S_SYS, S_SYS);
#endif	/* SIGSYS */

	sig_messages[i++].sysnam = MSGSTR(KSH_S_TERM, S_TERM);

#ifdef	SIGTINT
#   ifdef	JOBS
	sig_messages[i++].sysnam = MSGSTR(KSH_S_TINIT, S_TINIT);
#   else
	sig_messages[i++].sysnam = e_nullstr;
#   endif	/* JOBS */
#endif	/* SIGTINT */

#ifdef	SIGTRAP
	sig_messages[i++].sysnam = MSGSTR(KSH_S_TRAP, S_TRAP);
#endif	/* SIGTRAP */

#ifdef	SIGTSTP
	sig_messages[i++].sysnam = MSGSTR(KSH_S_TSTP, S_TSTP);
#endif	/* SIGTSTP */

#ifdef	SIGTTIN
	sig_messages[i++].sysnam = MSGSTR(KSH_S_TTIN, S_TTIN);
#endif	/* SIGTTIN */

#ifdef	SIGTTOU
	sig_messages[i++].sysnam = MSGSTR(KSH_S_TTOU, S_TTOU);
#endif	/* SIGTTOU */

#ifdef	SIGURG
	sig_messages[i++].sysnam = MSGSTR(KSH_S_URG, S_URG);
#endif	/* SIGURG */

#ifdef	SIGUSR1
	sig_messages[i++].sysnam = MSGSTR(KSH_S_USR1, S_USR1);
#endif	/* SIGUSR1 */

#ifdef	SIGUSR2
	sig_messages[i++].sysnam = MSGSTR(KSH_S_USR2, S_USR2);
#endif	/* SIGUSR2 */

#ifdef	SIGVTALRM
	sig_messages[i++].sysnam = MSGSTR(KSH_S_VTALRM, S_VTALRM);
#endif	/* SIGVTALRM */

#ifdef	SIGWINCH
	sig_messages[i++].sysnam = MSGSTR(KSH_S_WINCH, S_WINCH);
#endif	/* SIGWINCH */

#ifdef	SIGXCPU
	sig_messages[i++].sysnam = MSGSTR(KSH_S_XCPU, S_XCPU);
#endif	/* SIGXCPU */

#ifdef	SIGXFSZ
	sig_messages[i++].sysnam = MSGSTR(KSH_S_XFSZ, S_XFSZ);
#endif	/* SIGXFSZ */

#ifdef	SIGLOST
	sig_messages[i++].sysnam = MSGSTR(KSH_S_LOST, S_LOST);
#endif	/* SIGLOST */

#ifdef	SIGLAB
	sig_messgaes[i++].sysnam = MSGSTR(KSH_S_SIGLAB, S_SIGLAB);
#endif	/* SIGLAB */

	sig_messages[i].sysnam = e_nullstr;
}

#endif	/* INTL */
