#ifndef	_UTIL_ENGINE_H	/* wrapper symbol for kernel use */
#define	_UTIL_ENGINE_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:util/engine.h	1.31.3.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ksynch.h>	/* REQUIRED */
#include <svc/clock.h>		/* REQUIRED */	/* XXX - not required? */
#include <svc/time.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/clock.h>		/* REQUIRED */	/* XXX - not required? */
#include <sys/time.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Per-processor basic "engine" structure.  Fundamental representation
 * of a processor for dispatching and initialization.
 * Allocated per-processor at boot-time in an array.
 * Base address stored in kernel variable "engine".
 */

#if defined(_KERNEL) || defined(_KMEMUSER)

typedef struct engine {
	int		e_cpu_speed;	/* cpu rate in MHz              */
	volatile int	e_flags;	/* processor flags - see below	*/
	struct ppriv_pages *e_local;	/* virtual address of local stuff */
	struct ppriv_pages_pae *e_local_pae;	/* virtual addr. of local pae */
	struct cgpriv_pages *e_cglocal; /* virtual address of pm-local stuff */
	volatile int	e_pri;		/* current priority engine is running */
	volatile int	e_npri;		/* priority engine was nudged at */
	int	e_count;		/* number lwp's user-bound to engine */
	int	e_nsets;		/* number of cache sets */
	int	e_setsize;		/* cache set size in kbytes */
	struct callout *e_local_todo;	/* list of pending local callouts */
	struct runque **e_lastpick;	/* last run queue we scheduled from */
#define	MAXRQS	2
	struct runque *e_rqlist[MAXRQS+1];/* run queues this engine is scheduling
					   from */
	timestruc_t e_smodtime;		/* last time processor was turned
					 * online/offline  */
	struct engine *e_next;		/* next engine in this CG */
} engine_t;

/* currently defined flag bits */
#define	E_OFFLINE	0x01		/* processor is off-line	*/
#define E_BAD		0x02		/* processor is bad		*/
#define	E_SHUTDOWN	0x04		/* shutdown has been requested	*/ 
#define E_DRIVER	0x08		/* processor has driver bound	*/
#define	E_DEFAULTKEEP	0x100		/* keep processor in the default set */
#define	E_DRIVERBOUND	0x200		/* uniprocessor driver bound to engine */
#define E_EXCLUSIVE	0x400		/* for compatibility with symmetry (MP) */
#define E_CGLEADER	0x800		/* engine is boot cpu of a CG */
#define E_NOWAY		(E_OFFLINE|E_BAD|E_SHUTDOWN)

/* defined for state field */
#define	E_BOUND		0x01		/* processor is running bound lwp */
#define	E_GLOBAL	0x00		/* processor not running bound lwp */

/* Cannot switch lwp to Engine - see runme */
#define E_UNAVAIL	-1

#ifdef _KERNEL

extern	int online_engine(int);		/* online an engine */
extern	int offline_engine(int);	/* offline an engine */
extern boolean_t engine_disable_offline(int engno);
					/* test and disable engine offline */
extern int engine_state(int, uint_t, void *);
					/* test the state of the engine */

/*
 * The set of states of the engine that can be queried.
 */
#define	ENGINE_ONLINE	1

extern	struct engine	*engine;	/* Engine Array Base */
extern	struct engine	*engine_Nengine;/* just past Engine Array Base */
extern	uint_t		myengnum;	/* This engine's number (per-engine) */
extern	int 		Nengine;	/* # Engines to alloc at boot */
extern	int		nonline;	/* count of online engines */
extern	event_t eng_wait;		/* wait on this during online/offline */
extern	lock_t eng_tbl_mutex;		/* held when modifying the engine table */
extern	lkinfo_t eng_tbl_lkinfo;	/* information about the engine */
extern  fspin_t eng_count_mutex;        /* mutex all "e_count" fields */

/*
 * Map a processor-id to an engine pointer.
 */
#define	PROCESSOR_MAP(id)	(((id) < 0 || (id) >= Nengine)? \
					NULL : engine + (id))
#define PROCESSOR_UNMAP(e)	((e) - engine)

/*
 * Check if engine is online?
 */
#define PROCESSOR_NOWAY(eng)	(((eng)->e_flags & E_NOWAY))

#define BOOTENG			0

#endif /* _KERNEL */

#endif /*  defined(_KERNEL) || defined(_KMEMUSER) */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_ENGINE_H */
