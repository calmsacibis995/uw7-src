#ident	"@(#)ksh93:src/cmd/ksh93/data/signals.c	1.3"
#include	<ast.h>
#include	"shtable.h"
#include	"fault.h"

#if defined(SIGCLD) && !defined(SIGCHLD)
#   define SIGCHLD	SIGCLD
#endif

#define VAL(sig,mode)	((sig+1)|(mode)<<SH_SIGBITS)
#define TRAP(n)		(((n)|SH_TRAP)-1)

/*
 * This is a table that gives numbers and default settings to each signal
 * The signal numbers go in the low bits and the attributes go in the high bits
 */

const struct shtable4 shtab_signals[] =
{
#ifdef SIGABRT
	"ABRT",		VAL(SIGABRT,SH_SIGDONE), 	"Abort", "ksh93:144",
#endif /*SIGABRT */
#ifdef SIGAIO
	"AIO",		VAL(SIGAIO,SH_SIGIGNORE), 	"Asynchronous I/O", "ksh93:145",
#endif /*SIGAIO */
#ifdef SIGALRM
	"ALRM",		VAL(SIGALRM,SH_SIGFAULT),	"Alarm call", "ksh93:146",
#endif /* SIGALRM */
#ifdef SIGAPOLLO
	"APOLLO",	VAL(SIGAPOLLO,0),		"SIGAPOLLO", "ksh93:147",
#endif /* SIGAPOLLO */
#ifdef SIGBUS
	"BUS",		VAL(SIGBUS,SH_SIGDONE),		"Bus error", "ksh93:148",
#endif /* SIGBUS */
#ifdef SIGCHLD
	"CHLD",		VAL(SIGCHLD,SH_SIGFAULT), 	"Death of Child", "ksh93:149",
#   ifdef SIGCLD
#	if SIGCLD!=SIGCHLD
	    "CLD",	VAL(SIGCLD,SH_SIGFAULT),	"Death of Child", "ksh93:149",
#	endif
#   endif	/* SIGCLD */
#else
#   ifdef SIGCLD
	"CLD",		VAL(SIGCLD,SH_SIGFAULT),	"Death of Child", "ksh93:149",
#   endif	/* SIGCLD */
#endif	/* SIGCHLD */
#ifdef SIGCONT
	"CONT",		VAL(SIGCONT,SH_SIGIGNORE),	"Stopped process continued", "ksh93:150",
#endif	/* SIGCONT */
	"DEBUG",	VAL(TRAP(SH_DEBUGTRAP),0),	"", "ksh93:0",
#ifdef SIGDIL
	"DIL",		VAL(SIGDIL,0),			"DIL signal", "ksh93:151",
#endif	/* SIGDIL */
#ifdef SIGEMT
	"EMT",		VAL(SIGEMT,SH_SIGDONE),		"EMT trap", "ksh93:152",
#endif	/* SIGEMT */
	"ERR",		VAL(TRAP(SH_ERRTRAP),0),	"", "ksh93:0",
	"EXIT",		VAL(0,0),			"", "ksh93:0",
	"FPE",		VAL(SIGFPE,SH_SIGDONE),		"Floating exception", "ksh93:153",
#ifdef SIGFREEZE
	"FREEZE",	VAL(SIGFREEZE,SH_SIGIGNORE),	"Special signal used by CPR", "ksh93:154",
#endif	/* SIGFREEZE */
	"HUP",		VAL(SIGHUP,SH_SIGDONE),		"Hangup", "ksh93:155",
	"ILL",		VAL(SIGILL,SH_SIGDONE),		"Illegal instruction", "ksh93:156",
#ifdef JOBS
	"INT",		VAL(SIGINT,SH_SIGINTERACTIVE),	"Interrupt", "ksh93:157",
#else
	"INT",		VAL(SIGINT,SH_SIGINTERACTIVE),	"", "ksh93:0",
#endif /* JOBS */
#ifdef SIGIO
	"IO",		VAL(SIGIO,SH_SIGIGNORE),	"IO signal", "ksh93:158",
#endif	/* SIGIO */
#ifdef SIGIOT
	"IOT",		VAL(SIGIOT,SH_SIGDONE),		"Abort", "ksh93:144",
#endif	/* SIGIOT */
	"KEYBD",	VAL(TRAP(SH_KEYTRAP),0),	"", "ksh93:0",
#ifdef SIGKILL
	"KILL",		VAL(SIGKILL,0),			"Killed", "ksh93:159",
#endif /* SIGKILL */
#ifdef SIGLAB
	"LAB",		VAL(SIGLAB,0),			"Security label changed", "ksh93:160",
#endif	/* SIGLAB */
#ifdef SIGLOST
	"LOST",		VAL(SIGLOST,SH_SIGDONE),	"Resources lost", "ksh93:161",
#endif	/* SIGLOST */
#ifdef SIGLWP
	"LWP",		VAL(SIGLWP,SH_SIGIGNORE),	"Special signal used by thread library", "ksh93:162",
#endif	/* SIGLWP */
#ifdef SIGPHONE
	"PHONE",	VAL(SIGPHONE,0),		"Phone interrupt", "ksh93:163",
#endif	/* SIGPHONE */
#ifdef SIGPIPE
#ifdef JOBS
	"PIPE",		VAL(SIGPIPE,SH_SIGDONE),	"Broken Pipe", "ksh93:164",
#else
	"PIPE",		VAL(SIGPIPE,SH_SIGDONE),	 "", ":0",
#endif /* JOBS */
#endif /* SIGPIPE */
#ifdef SIGPOLL
	"POLL",		VAL(SIGPOLL,SH_SIGDONE),	"Polling alarm", "ksh93:165",
#endif	/* SIGPOLL */
#ifdef SIGPROF
	"PROF",		VAL(SIGPROF,SH_SIGDONE), 	"Profiling time alarm", "ksh93:166",
#endif	/* SIGPROF */
#ifdef SIGPWR
#   if SIGPWR>0
	"PWR",		VAL(SIGPWR,SH_SIGIGNORE),	"Power fail", "ksh93:167",
#   endif
#endif	/* SIGPWR */
#ifdef SIGQUIT
	"QUIT",		VAL(SIGQUIT,SH_SIGDONE|SH_SIGINTERACTIVE),	"Quit", "ksh93:168",
#ifdef _SIGRTMAX
	"RTMAX",	VAL(_SIGRTMAX,0),		"Lowest priority realtime signal", "ksh93:169",
#else
#   ifdef SIGRTMAX
	"RTMAX",	VAL(SIGRTMAX,0),		"Lowest priority realtime signal", "ksh93:169",
#   endif /* SIGRTMAX */
#endif	/* _SIGRTMAX */
#ifdef _SIGRTMIN
	"RTMIN",	VAL(_SIGRTMIN,0),		"Highest priority realtime signal", "ksh93:170",
#else
#   ifdef SIGRTMIN
	"RTMIN",	VAL(SIGRTMIN,0),		"Highest priority realtime signal", "ksh93:170",
#   endif /* SIGRTMIN */
#endif	/* _SIGRTMIN */
#endif	/* SIGQUIT */
	"SEGV",		VAL(SIGSEGV,0),			"Memory fault", "ksh93:171",
#ifdef SIGSTOP
	"STOP",		VAL(SIGSTOP,0),			"Stopped (SIGSTOP)", "ksh93:172",
#endif	/* SIGSTOP */
#ifdef SIGSYS
	"SYS",		VAL(SIGSYS,SH_SIGDONE),		"Bad system call", "ksh93:173",
#endif	/* SIGSYS */
	"TERM",		VAL(SIGTERM,SH_SIGDONE|SH_SIGINTERACTIVE),	"Terminated", "ksh93:174",
#ifdef SIGTINT
#   ifdef JOBS
	"TINT",		VAL(SIGTINT,0),			"Interrupt", "ksh93:157",
#   else
	"TINT",		VAL(SIGTINT,0),			"", ":0",
#   endif /* JOBS */
#endif	/* SIGTINT */
#ifdef SIGTRAP
	"TRAP",		VAL(SIGTRAP,SH_SIGDONE),	"Trace/BPT trap", "ksh93:175",
#endif	/* SIGTRAP */
#ifdef SIGTSTP
	"TSTP",		VAL(SIGTSTP,0),			"Stopped", "ksh93:176",
#endif	/* SIGTSTP */
#ifdef SIGTTIN
	"TTIN",		VAL(SIGTTIN,0),			"Stopped (SIGTTIN)", "ksh93:177",
#endif	/* SIGTTIN */
#ifdef SIGTTOU
	"TTOU",		VAL(SIGTTOU,0),			"Stopped(SIGTTOU)", "ksh93:178",
#endif	/* SIGTTOU */
#ifdef SIGURG
	"URG",		VAL(SIGURG,SH_SIGIGNORE),	"Socket interrupt", "ksh93:179",
#endif	/* SIGURG */
#ifdef SIGUSR1
	"USR1",		VAL(SIGUSR1,SH_SIGDONE),	 "User signal 1", "ksh93:180",
#endif	/* SIGUSR1 */
#ifdef SIGUSR2
	"USR2",		VAL(SIGUSR2,SH_SIGDONE),	 "User signal 2", "ksh93:181",
#endif	/* SIGUSR2 */
#ifdef SIGVTALRM
	"VTALRM",	VAL(SIGVTALRM,SH_SIGDONE),	"Virtual time alarm", "ksh93:182",
#endif	/* SIGVTALRM */
#ifdef SIGWINCH
	"WINCH",	VAL(SIGWINCH,SH_SIGIGNORE),	"Window size change", "ksh93:183",
#endif	/* SIGWINCH */
#ifdef SIGWINDOW
	"WINDOW",	VAL(SIGWINDOW,SH_SIGIGNORE),	"Window size change", "ksh93:183",
#endif	/* SIGWINDOW */
#ifdef SIGWIND
	"WIND",		VAL(SIGWIND,SH_SIGIGNORE),	"Window size change", "ksh93:183",
#endif	/* SIGWIND */
#ifdef SIGMIGRATE
	"MIGRATE",		VAL(SIGMIGRATE,0),	"Migrate process", "ksh93:184",
#endif	/* SIGMIGRATE */
#ifdef SIGDANGER
	"DANGER",		VAL(SIGDANGER,0),	"System crash soon", "ksh93:185",
#endif	/* SIGDANGER */
#ifdef SIGSOUND
	"SOUND",		VAL(SIGSOUND,0),	"Sound completed", "ksh93:186",
#endif	/* SIGSOUND */
#ifdef SIGTHAW
	"THAW",			VAL(SIGTHAW,SH_SIGIGNORE),	"Special signal used by CPR", "ksh93:154",
#endif	/* SIGTHAW */
#ifdef SIGWAITING
	"WAITING",		VAL(SIGWAITING,SH_SIGIGNORE),	"All threads blocked", "ksh93:384",
#endif	/* SIGWAITING */
#ifdef SIGXCPU
	"XCPU",		VAL(SIGXCPU,SH_SIGDONE|SH_SIGINTERACTIVE),	"Exceeded CPU time limit", "ksh93:188",
#endif	/* SIGXCPU */
#ifdef SIGXFSZ
	"XFSZ",		VAL(SIGXFSZ,SH_SIGDONE|SH_SIGINTERACTIVE),	"Exceeded file size limit", "ksh93:189",
#endif	/* SIGXFSZ */
	"",	0,	0,	0
};
