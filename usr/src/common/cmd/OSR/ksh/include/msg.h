#ident	"@(#)OSRcmds:ksh/include/msg.h	1.1"
#pragma comment(exestr, "@(#) msg.h 25.3 94/04/14 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1994.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Modification History
 *
 * 26 Mar 93	scol!gregw	L000
 *	- Made getopts error messages consistent with Bourne shell.
 * 14 Apr 94	scol!anthonys	L001
 *	- Corrected a typo, S_XFSX should be S_XFSZ.
 */
/*
 * Definitions of default error messages
 */

#define	E_TIMEWARN	"\r\n\007shell will timeout in 60 seconds due to inactivity"
#define	E_TIMEOUT	"timed out waiting for input"
#define	E_MAILMSG	"you have mail in $_"
#define	E_QUERY	"no query process"
#define	E_HISTORY	"no history file"
#define	E_OPTION	"-- illegal option"			/* L000 */
#define	E_SPACE	"no space"
#define	E_ARGEXP	"-- option requires an argument"	/* L000 */
#define	E_BRACKET	"] missing"
#define	E_NUMBER	"bad number"
#define	E_NULLSET	"parameter null or not set"
#define	E_NOTSET	"parameter not set"
#define	E_SUBST	"bad substitution"
#define	E_CREATE	"cannot create"
#define	E_RESTRICTED	"restricted"
#define	E_FORK	"cannot fork: too many processes"
#define	E_PEXISTS	"process already exists"
#define	E_FEXISTS	"file already exists"
#define	E_SWAP	"cannot fork: no swap space"
#define	E_PIPE	"cannot make pipe"
#define	E_OPEN	"cannot open"
#define	E_LOGOUT	"Use 'exit' to terminate this shell"
#define	E_ARGLIST	"arg list too long"
#define	E_TXTBSY	"text busy"
#define	E_TOOBIG	"too big"
#define	E_EXEC	"cannot execute"
#define	E_PWD	"cannot access parent directories"
#define	E_FOUND	" not found"
#define	E_FLIMIT	"too many open files"
#define	E_ULIMIT	"exceeds allowable limit"
#define	E_SUBSCRIPT	"subscript out of range"
#define	E_NARGS	"bad argument count"
#define	E_LIBACC 	"can't access a needed shared library"
#define	E_LIBBAD	"accessing a corrupted shared library"
#define	E_LIBSCN	".lib section in a.out corrupted"
#define	E_LIBMAX	"attempting to link in too many libs"
#define	E_MULTIHOP	"multihop attempted"
#define	E_LONGNAME	"name too long"
#define	E_LINK	"remote link inactive"
#define	E_ACCESS	"permission denied"
#define	E_DIRECT	"bad directory"
#define	E_NOTDIR	"not a directory"
#define	E_FILE	"bad file unit number"
#define	E_TRAP	"bad trap"
#define	E_READONLY	"is read only"
#define	E_IDENT	"is not an identifier"
#define	E_ALINAME	"invalid alias name"
#define	E_TESTOP	"unknown test operator"
#define	E_ALIAS	" alias not found"
#define	E_FUNCTION	"unknown function"
#define	E_FORMAT	"bad format"
#define	E_ON	"on"
#define	E_OFF	"off"
#define	IS_RESERVED	" is a keyword"
#define	IS_BUILTIN	" is a shell builtin"
#define	IS_ALIAS	" is an alias for "
#define	IS_FUNCTION	" is a function"
#define	IS_XALIAS	" is an exported alias for "
#define	IS_TALIAS	" is a tracked alias for "
#define	IS_XFUNCTION	" is an exported function"
#define	IS_UFUNCTION	" is an undefined function"
#define	IS_	" is "
#define	E_NEWTTY	"Switching to new tty driver..."
#define	E_OLDTTY	"Reverting to old tty driver..."
#define	E_NO_START	"Cannot start job control"
#define	E_NO_JCTL	"No job control"
#define	E_TERMINATE	"You have stopped jobs"
#define	E_DONE	" Done"
#define	E_RUNNING	" Running"
#define	E_AMBIGUOUS	"Ambiguous"
#define	E_RUNNING_J	"You have running jobs"
#define	E_NO_JOB	"no such job"
#define	E_NO_PROC	"no such process"
#define	E_KILLCOLON	"kill: "
#define	E_JOBUSAGE	"Arguments must be %job or process ids"
#define	E_KILL	"kill"
#define	E_COREDUMP	"(coredump)"
#define	E_HEADING	"Current option settings"
#define	E_ENDOFFILE	"end of file"
#define	E_UNEXPECTED	" unexpected"
#define	E_UNMATCHED " unmatched"
#define	E_UNKNOWN 	"<command unknown>"
#define	E_ATLINE	" at line "
#define	E_PROHIBITED	"login setuid/setgid shells prohibited"
#define	E_UNLIMITED	"unlimited"
#define	E_TEST	"test"
#define	E_BLTFN	"function "
#define	E_INTBASE	"base"
#define	E_REAL	"\nreal"
#define	E_USER	"user"
#define	E_SYS	"sys"

#define	E_MORETOKENS	"more tokens expected"
#define	E_PAREN		"unbalanced parenthesis"
#define	E_BADCOLON	"invalid use of :"
#define	E_DIVZERO	"divide by zero"
#define	E_SYNBAD	"syntax error"
#define	E_NOTLVALUE	"assignment requires lvalue"
#define	E_RECURSIVE	"recursion too deep"

#ifdef	future
#  define	E_QUESTCOLON	": expected for ? operator"
#endif	/* future */

#ifdef	FLOAT
#  define	E_INCOMPATIBLE	"operands have incompatible types"
#endif	/* FLOAT */


/* signal messages */
#ifdef SIGABRT
#   define	S_ABRT	"Abort"
#endif /*SIGABRT */
#   define	S_ALRM	"Alarm call"
#ifdef SIGBUS
#   define	S_BUS	"Bus error"
#endif /* SIGBUS */
#ifdef SIGCHLD
#   define	S_CHLD	"Child stopped or terminated"
#   ifdef SIGCLD
#	if SIGCLD!=SIGCHLD
#	   define	S_CLD	"Death of Child"
#	endif
#   endif	/* SIGCLD */
#else
#   ifdef SIGCLD
#      define	S_CLD	"Death of Child"
#   endif	/* SIGCLD */
#endif	/* SIGCHLD */
#ifdef SIGCONT
#   define	S_CONT	"Stopped process continued"
#endif	/* SIGCONT */
#ifdef SIGEMT
#   define	S_EMT	"EMT trap"
#endif	/* SIGEMT */
#   define	S_FPE	"Floating exception"
#   define	S_HUP	"Hangup"
#   define	S_ILL	"Illegal instruction"
#ifdef JOBS
#   define	S_INT	"Interrupt"
#endif	/* JOBS */
#ifdef SIGIO
#   define	S_IO	"IO signal"
#endif	/* SIGIO */
#ifdef SIGIOT
#   define	S_IOT	"Abort"
#endif	/* SIGIOT */
#define		S_KILL	"Killed"
#define		S_QUIT	"Quit"
#ifdef JOBS
#define		S_PIPE	"Broken Pipe"
#endif	/* JOBS */
#ifdef SIGPROF
#   define	S_PROF	"Profiling time alarm"
#endif	/* SIGPROF */
#ifdef SIGPWR
#   if SIGPWR>0
#      define	S_PWR	"Power fail"
#   endif
#endif	/* SIGPWR */
#define		S_SEGV	"Memory fault"
#ifdef SIGSTOP
#define		S_STOP	"Stopped (signal)"
#endif	/* SIGSTOP */
#ifdef SIGSYS
#define		S_SYS	"Bad system call"
#endif	/* SIGSYS */
#define		S_TERM	"Terminated"
#ifdef SIGTINT
#   ifdef JOBS
#      define	S_TINT	"Interrupt"
#   endif /* JOBS */
#endif	/* SIGTINT */
#ifdef SIGTRAP
#define		S_TRAP	"Trace/BPT trap"
#endif	/* SIGTRAP */
#ifdef SIGTSTP
#define		S_TSTP	"Stopped"
#endif	/* SIGTSTP */
#ifdef SIGTTIN
#define		S_TTIN	"Stopped (tty input)"
#endif	/* SIGTTIN */
#ifdef SIGTTOU
#define		S_TTOU	"Stopped(tty output)"
#endif	/* SIGTTOU */
#ifdef SIGURG
#define		S_URG	"Socket interrupt"
#endif	/* SIGURG */
#ifdef SIGUSR1
#define		S_USR1	"User signal 1"
#endif	/* SIGUSR1 */
#ifdef SIGUSR2
#define		S_USR2	"User signal 2"
#endif	/* SIGUSR2 */
#ifdef SIGVTALRM
#define		S_VTALRM	"Virtual time alarm"
#endif	/* SIGVTALRM */
#ifdef SIGWINCH
#define		S_WINCH	"Window size change"
#endif	/* SIGWINCH */
#ifdef SIGXCPU
#define		S_XCPU	"Exceeded CPU time limit"
#endif	/* SIGXCPU */
#ifdef SIGXFSZ
#define		S_XFSZ	"Exceeded file size limit"		/* L001 */
#endif	/* SIGXFSZ */
#ifdef SIGLOST
#define		S_LOST	"Resources lost"
#endif	/* SIGLOST */
#ifdef SIGLAB
#define		S_LAB	"Security label changed"
#endif	/* SIGLAB */
