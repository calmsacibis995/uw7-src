#ifndef _Proctypes_h
#define _Proctypes_h

#ident	"@(#)debugger:inc/i386/Proctypes.h	1.2"

#include "Machine.h"
#include <sys/types.h>
#include <signal.h>
#include <sys/regset.h>
#include <sys/fault.h>

#ifndef PTRACE
#include <sys/procfs.h>
#endif

/* NOTE: uses C style comments so it may be included by C code */

/* System and machine specific definitions for access to
 * the process control mechanism.
 */

/* reasons for stopping  - numbers are same used by /proc */
#define	STOP_REQUESTED	1
#define	STOP_SIGNALLED	2
#define	STOP_SYSENTRY	3
#define	STOP_SYSEXIT	4
#define	STOP_JOBCONTROL	5
#define	STOP_FAULTED	6
#define	STOP_SUSPENDED	7

#define BKPTSIG			SIGTRAP
#define BKPTFAULT		FLTBPT

#ifdef OLD_PROC
#define TRACEFAULT		FLTBPT
#else
#define TRACEFAULT		FLTTRACE
#endif

#ifdef PTRACE
#define STOP_TRACE		BKPTSIG
#define STOP_BKPT		BKPTSIG
#define STOP_TYPE		STOP_SIGNALLED
#define CLEARSIG()		latestsig = 0
#else
#define STOP_TRACE		TRACEFAULT
#define STOP_BKPT		BKPTFAULT
#define STOP_TYPE		STOP_FAULTED
#define CLEARSIG()
#endif

#ifdef OLD_PROC
typedef prstatus_t	pstatus_t;
typedef prpsinfo_t	psinfo_t;
#endif

/* special signals that shouldn't be ignored */
#if PTRACE
#define SIG_SPECIAL(I) ((I) == BKPTSIG)
#else
#define SIG_SPECIAL(I) (0)
#endif

/* structure definitions for functions that write sets of data:
 * gregs, fpregs, dbregs, sigset, sysset and fltset.  These
 * structures have room at the beginning for a control word,
 * to make access to the new /proc more efficient.  The control
 * word is unused for ptrace or old /proc
 */

#ifndef PTRACE
typedef prmap_t map_ctl;

#else
typedef long map_ctl;
#endif


struct	greg_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	gregset_t	gregs;
};

struct	fpreg_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	fpregset_t	fpregs;
};

#ifdef HAS_SYSCALL_TRACING
#include <sys/syscall.h>
#define	SYSCALL_fork	SYS_fork
#define	SYSCALL_vfork	SYS_vfork
#define	SYSCALL_exec	SYS_exec
#define	SYSCALL_execve	SYS_execve
#ifdef DEBUG_THREADS	
#define SYSCALL_fork1	SYS_fork1
#define SYSCALL_forkall	SYS_forkall
#define SYSCALL_lwpcreate	SYS_lwpcreate
#endif

struct	sys_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	sysset_t	scalls;
};

#else

#define	SYSCALL_fork	1
#define	SYSCALL_vfork	2
#define	SYSCALL_exec	3
#define	SYSCALL_execve	4
#define SYSCALL_fork1	5
#define SYSCALL_forkall	6
#define SYSCALL_lwpcreate	7
#endif

struct	sig_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	sigset_t	signals;
};

struct	flt_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	fltset_t	faults;
};

struct	dbreg_ctl {
#if !PTRACE && !OLD_PROC
	int		ctl;
#endif
	dbregset_t	dbregs;
};

/* Local signal set manipulation routines 
 * assume sigset_t is declared as :
 * typedef struct {
 *  unsigned long sa_sigbits[4];
 *  }; sigset_t
 *  we only read as many words as are necessary for number of signals
 * currently implemented
 *
 * the routines prfillset, premptyset, prismember are declared
 * in procfs.h
 */

/* result = union(set1, set2) */
#define  mysigsetcombine(set1, set2, result) \
{\
	for (int _i_ = 0; _i_ <= ((NUMBER_OF_SIGS-1)/WORD_BIT); _i_++)\
	{ \
		(result)->sa_sigbits[_i_] = (set1)->sa_sigbits[_i_] | \
		(set2)->sa_sigbits[_i_];\
	}\
}


#if NUMBER_OF_SIGS <= WORD_BIT
#define mysigsetisempty(set) ((set)->sa_sigbits[0] == 0)

#elif NUMBER_OF_SIGS <= 2 * WORD_BIT
#define mysigsetisempty(set) ((set)->sa_sigbits[0] == 0 && \
	(set)->sa_sigbits[1] == 0)

#elif NUMBER_OF_SIGS <= 3 * WORD_BIT
#define mysigsetisempty(set) ((set)->sa_sigbits[0] == 0 && \
	(set)->sa_sigbits[1] == 0 && (set)->sa_sigbits[2] == 0)

#else
#define mysigsetisempty(set) ((set)->sa_sigbits[0] == 0 && \
	(set)->sa_sigbits[1] == 0 && (set)->sa_sigbits[2] == 0 && \
	(set)->sa_sigbits[3] == 0)

#endif

#if PTRACE
/* set manipulation routines defined in procfs.h */
/* turn on all flags in set */
#define	prfillset(sp) \
	{ register int _i_ = sizeof(*(sp))/sizeof(ulong_t); \
		while(_i_) ((ulong_t*)(sp))[--_i_] = 0xFFFFFFFF; }

/* turn off all flags in set */
#define	premptyset(sp) \
	{ register int _i_ = sizeof(*(sp))/sizeof(ulong_t); \
		while(_i_) ((ulong_t*)(sp))[--_i_] = 0L; }

/* turn on specified flag in set */
#define	praddset(sp, flag) \
	(((unsigned)((flag)-1) < 32*sizeof(*(sp))/sizeof(ulong_t)) ? \
	(((ulong_t*)(sp))[((flag)-1)/32] |= (1L<<(((flag)-1)%32))) : 0)

/* turn off specified flag in set */
#define	prdelset(sp, flag) \
	(((unsigned)((flag)-1) < 32*sizeof(*(sp))/sizeof(ulong_t)) ? \
	(((ulong_t*)(sp))[((flag)-1)/32] &= ~(1L<<(((flag)-1)%32))) : 0)

/* query: != 0 iff flag is turned on in set */
#define	prismember(sp, flag) \
	(((unsigned)((flag)-1) < 32*sizeof(*(sp))/sizeof(ulong_t)) \
	&& (((ulong_t*)(sp))[((flag)-1)/32] & (1L<<(((flag)-1)%32))))
#endif

#endif
