#ident	"@(#)libc-i386:gen/siginfolst.c	1.3"

#include "synonyms.h"
#include <signal.h>
#include "siginfom.h"


static const char *const traplist[NSIGTRAP] =
{
	"process breakpoint",
	"process trace"
};

static const char *const illlist[NSIGILL] =
{
	"illegal opcode",
	"illegal operand",
	"illegal addressing mode",
	"illegal trap",
	"privileged opcode",
	"privileged register",
	"co-processor",
	"bad stack"
};

static const char *const fpelist[NSIGFPE] =
{
	"integer divide by zero",
	"integer overflow",
	"floating point divide by zero",
	"floating point overflow",
	"floating point underflow",
	"floating point inexact result",
	"invalid floating point operation",
	"floating point subscript out of range"
};

static const char *const segvlist[NSIGSEGV] =
{
	"address not mapped to object",
	"invalid permissions"
};

static const char *const buslist[NSIGBUS] =
{
	"invalid address alignment",
	"non-existent physical address",
	"object specific"
};

static const char *const cldlist[NSIGCLD] =
{
	"child has exited",
	"child was killed",
	"child has coredumped",
	"traced child has trapped",
	"child has stopped",
	"stopped child has continued"
};

static const char *const polllist[NSIGPOLL] =
{
	"input available",
	"output possible",
	"message available",
	"I/O error",
	"high priority input available",
	"device disconnected"
};

const struct siginfolist _sys_siginfolist[NSIG-1] =
{
	0,		0,		/* SIGHUP */
	0,		0,		/* SIGINT */
	0,		0,		/* SIGQUIT */
	NSIGILL,	illlist,	/* SIGILL */
	NSIGTRAP,	traplist,	/* SIGTRAP */
	0,		0,		/* SIGABRT */
	0,		0,		/* SIGEMT */
	NSIGFPE,	fpelist,	/* SIGFPE */
	0,		0,		/* SIGKILL */
	NSIGBUS,	buslist,	/* SIGBUS */
	NSIGSEGV,	segvlist,	/* SIGSEGV */
	0,		0,		/* SIGSYS */
	0,		0,		/* SIGPIPE */
	0,		0,		/* SIGALRM */
	0,		0,		/* SIGTERM */
	0,		0,		/* SIGUSR1 */
	0,		0,		/* SIGUSR2 */
	NSIGCLD,	cldlist,	/* SIGCLD */
	0,		0,		/* SIGPWR */
	0,		0,		/* SIGWINCH */
	0,		0,		/* SIGURG */
	NSIGPOLL,	polllist,	/* SIGPOLL */
	0,		0,		/* SIGSTOP */
	0,		0,		/* SIGTSTP */
	0,		0,		/* SIGCONT */
	0,		0,		/* SIGTTIN */
	0,		0,		/* SIGTTOU */
	0,		0,		/* SIGVTALRM */
	0,		0,		/* SIGPROF */
	0,		0,		/* SIGXCPU */
	0,		0,		/* SIGXFSZ */
};

#define MSG_ID_TRAP	36		/* Offset of msg in uxlibc catalog */
#define MSG_ID_ILL	(MSG_ID_TRAP + NSIGTRAP)
#define MSG_ID_FPE	(MSG_ID_ILL + NSIGILL)
#define MSG_ID_SEGV	(MSG_ID_FPE + NSIGFPE)
#define MSG_ID_BUS	(MSG_ID_SEGV + NSIGSEGV)
#define MSG_ID_CLD	(MSG_ID_BUS + NSIGBUS)
#define MSG_ID_POLL	(MSG_ID_CLD + NSIGCLD)

const int _siginfo_msg_offset[NSIG-1] =
{
	0,		/* SIGHUP */
	0,		/* SIGINT */
	0,		/* SIGQUIT */
	MSG_ID_ILL,	/* SIGILL */
	MSG_ID_TRAP,	/* SIGTRAP */
	0,		/* SIGABRT */
	0,		/* SIGEMT */
	MSG_ID_FPE,	/* SIGFPE */
	0,		/* SIGKILL */
	MSG_ID_BUS,	/* SIGBUS */
	MSG_ID_SEGV,	/* SIGSEGV */
	0,		/* SIGSYS */
	0,		/* SIGPIPE */
	0,		/* SIGALRM */
	0,		/* SIGTERM */
	0,		/* SIGUSR1 */
	0,		/* SIGUSR2 */
	MSG_ID_CLD,	/* SIGCLD */
	0,		/* SIGPWR */
	0,		/* SIGWINCH */
	0,		/* SIGURG */
	MSG_ID_POLL,	/* SIGPOLL */
	0,		/* SIGSTOP */
	0,		/* SIGTSTP */
	0,		/* SIGCONT */
	0,		/* SIGTTIN */
	0,		/* SIGTTOU */
	0,		/* SIGVTALRM */
	0,		/* SIGPROF */
	0,		/* SIGXCPU */
	0,		/* SIGXFSZ */
};
