#ident	"@(#)ksh93:src/lib/libast/features/signal.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * generate signal features
 */

#include <signal.h>

extern char	*gettxt();

struct _m_
{
	char*		text;
	char*		name;
	int		value;
};

#define elementsof(x)	(sizeof(x)/sizeof(x[0]))

static struct _m_ map[] =
{
#ifdef SIGABRT
gettxt(":144", "Abort"),			"ABRT",		SIGABRT,
#endif
#ifdef SIGAIO
gettxt(":145", "Asynchronous I/O"),		"AIO",		SIGAIO,
#endif
#ifdef SIGALRM
gettxt(":146", "Alarm call"),			"ALRM",		SIGALRM,
#endif
#ifdef SIGAPOLLO
gettxt(":147", "Apollo",			"APOLLO",	SIGAPOLLO,
#endif
#ifdef SIGBUS
gettxt(":148", "Bus error"),			"BUS",		SIGBUS,
#endif
#ifdef SIGCHLD
gettxt(":149", "Child status change"),		"CHLD",		SIGCHLD,
#endif
#ifdef SIGCLD
gettxt(":149", "Death of child"), 		"CLD",		SIGCLD,
#endif
#ifdef SIGCONT
gettxt(":150", "Stopped process continued"),	"CONT",		SIGCONT,
#endif
#ifdef SIGDANGER
gettxt(":185", "System crash soon"),		"DANGER",	SIGDANGER,
#endif
#ifdef SIGDEBUG
gettxt(":376", "Debug trap"),			"DEBUG",	SIGDEBUG,
#endif
#ifdef SIGDIL
gettxt(":151", "DIL trap"),			"DIL",		SIGDIL,
#endif
#ifdef SIGEMT
gettxt(":152", "EMT trap"),			"EMT",		SIGEMT,
#endif
#ifdef SIGERR
gettxt(":377", "ERR trap"),			"ERR",		SIGERR,
#endif
#ifdef SIGEXIT
gettxt(":378", "Exit"),				"EXIT",		SIGEXIT,
#endif
#ifdef SIGFPE
gettxt(":153", "Floating exception"),		"FPE",		SIGFPE,
#endif
#ifdef SIGFREEZE
gettxt(":154", "CPR freeze"),			"FREEZE",	SIGFREEZE,
#endif
#ifdef SIGHUP
gettxt(":155", "Hangup"),			"HUP",		SIGHUP,
#endif
#ifdef SIGILL
gettxt(":156", "Illegal instruction"),		"ILL",		SIGILL,
#endif
#ifdef SIGINT
gettxt(":157", "Interrupt"),			"INT",		SIGINT,
#endif
#ifdef SIGIO
gettxt(":158", "IO possible"),			"IO",		SIGIO,
#endif
#ifdef SIGIOT
gettxt(":379", "IOT trap"),			"IOT",		SIGIOT,
#endif
#ifdef SIGKILL
gettxt(":159", "Killed"),			"KILL",		SIGKILL,
#endif
#ifdef SIGLAB
gettxt(":160", "Security label changed"),	"LAB",		SIGLAB,
#endif
#ifdef SIGLOST
gettxt(":161", "Resources lost"),		"LOST",		SIGLOST,
#endif
#ifdef SIGLWP
gettxt(":162", "Thread event"),			"LWP",		SIGLWP,
#endif
#ifdef SIGMIGRATE
gettxt(":184", "Migrate process"),		"MIGRATE",	SIGMIGRATE,
#endif
#ifdef SIGPHONE
gettxt(":163", "Phone status change"),		"PHONE",	SIGPHONE,
#endif
#ifdef SIGPIPE
gettxt(":164", "Broken pipe"),			"PIPE",		SIGPIPE,
#endif
#ifdef SIGPOLL
gettxt(":165", "Poll event"),			"POLL",		SIGPOLL,
#endif
#ifdef SIGPROF
gettxt(":166", "Profile timer alarm"),		"PROF",		SIGPROF,
#endif
#ifdef SIGPWR
gettxt(":167", "Power fail"),			"PWR",		SIGPWR,
#endif
#ifdef SIGQUIT
gettxt(":168", "Quit"),				"QUIT",		SIGQUIT,
#endif
#ifdef SIGSEGV
gettxt(":171", "Memory fault"),			"SEGV",		SIGSEGV,
#endif
#ifdef SIGSOUND
gettxt(":186", "Sound completed"),		"SOUND",	SIGSOUND,
#endif
#ifdef SIGSSTOP
gettxt(":380", "Sendable stop"),		"SSTOP",	SIGSSTOP,
#endif
#ifdef gould
gettxt(":381", "Stack overflow"),		"STKOV",	28,
#endif
#ifdef SIGSTOP
gettxt(":172", "Stopped (signal)"),		"STOP",		SIGSTOP,
#endif
#ifdef SIGSYS
gettxt(":173", "Bad system call"), 		"SYS",		SIGSYS,
#endif
#ifdef SIGTERM
gettxt(":174", "Terminated"),			"TERM",		SIGTERM,
#endif
#ifdef SIGTHAW
gettxt(":382", "CPR thaw"),			"THAW",		SIGTHAW,
#endif
#ifdef SIGTINT
gettxt(":383", "Interrupt (terminal)"),		"TINT",		SIGTINT,
#endif
#ifdef SIGTRAP
gettxt(":175", "Trace trap"),			"TRAP",		SIGTRAP,
#endif
#ifdef SIGTSTP
gettxt(":176", "Stopped"),			"TSTP",		SIGTSTP,
#endif
#ifdef SIGTTIN
gettxt(":177", "Stopped (tty input)"),		"TTIN",		SIGTTIN,
#endif
#ifdef SIGTTOU
gettxt(":178", "Stopped (tty output)"),		"TTOU",		SIGTTOU,
#endif
#ifdef SIGURG
gettxt(":179", "Urgent IO"),			"URG",		SIGURG,
#endif
#ifdef SIGUSR1
gettxt(":180", "User signal 1"),		"USR1",		SIGUSR1,
#endif
#ifdef SIGUSR2
gettxt(":181", "User signal 2"),		"USR2",		SIGUSR2,
#endif
#ifdef SIGVTALRM
gettxt(":182", "Virtual timer alarm"),		"VTALRM",	SIGVTALRM,
#endif
#ifdef SIGWAITING
gettxt(":187", "All threads blocked"),		"WAITING",	SIGWAITING,
#endif
#ifdef SIGWINCH
gettxt(":183", "Window change"), 		"WINCH",	SIGWINCH,
#endif
#ifdef SIGWIND
gettxt(":183", "Window change"),		"WIND",		SIGWIND,
#endif
#ifdef SIGWINDOW
gettxt(":183", "Window change"),		"WINDOW",	SIGWINDOW,
#endif
#ifdef SIGXCPU
gettxt(":188", "CPU time limit"),		"XCPU",		SIGXCPU,
#endif
#ifdef SIGXFSZ
gettxt(":189", "File size limit"),		"XFSZ",		SIGXFSZ,
#endif
0
};

#define RANGE_MIN	(1<<14)
#define RANGE_MAX	(1<<13)
#define RANGE_RT	(1<<12)

#define RANGE_SIG	(~(RANGE_MIN|RANGE_MAX|RANGE_RT))

static int		index[64];

extern int		printf(const char*, ...);

main()
{
	register int	i;
	register int	j;
	register int	k;
	int		n;

	k = 0;
	for (i = 0; map[i].name; i++)
		if ((j = map[i].value) > 0 && j < elementsof(index) && !index[j])
		{
			if (j > k) k = j;
			index[j] = i;
		}
#ifdef SIGRTMIN
	i = SIGRTMIN;
#ifdef SIGRTMAX
	j = SIGRTMAX;
#else
	j = i;
#endif
	if (j >= elementsof(index)) j = elementsof(index) - 1;
	if (i <= j && i > 0 && i < elementsof(index) && j > 0 && j < elementsof(index))
	{
		if (j > k) k = j;
		index[i] = RANGE_MIN | RANGE_RT;
		n = 1;
		while (++i < j)
			index[i] = RANGE_RT | n++;
		index[j] = RANGE_MAX | RANGE_RT | n;
	}
#endif
	printf("#pragma prototyped\n");
	printf("#define SIG_MAX	%d\n", k);
	printf("\n");
	printf("static const char* const	sig_name[] =\n");
	printf("{\n");
	for (i = 0; i <= k; i++)
		if (!(j = index[i])) printf("	\"%d\",\n", i);
		else if (j & RANGE_RT)
		{
			if (j & RANGE_MIN) printf("	\"RTMIN\",\n");
			else if (j & RANGE_MAX) printf("	\"RTMAX\",\n");
			else printf("	\"RT%d\",\n", j & RANGE_SIG);
		}
		else printf("	\"%s\",\n", map[j].name);
	printf("	0\n");
	printf("};\n");
	printf("\n");
	printf("static const char* const	sig_text[] =\n");
	printf("{\n");
	for (i = 0; i <= k; i++)
		if (!(j = index[i])) printf("	\"Signal %d\",\n", i);
		else if (j & RANGE_RT) printf("	\"Realtime priority %d%s\",\n", j & RANGE_SIG, (j & RANGE_MIN) ? " (lo)" : (j & RANGE_MAX) ? " (hi)" : "");
		else printf("	\"%s\",\n", map[j].text);
	printf("	0\n");
	printf("};\n");
	return(0);
}
