#ident	"@(#)kern-i386:proc/cg.h	1.2.5.2"
#ident	"$Header$"

#ifndef _PROC_CG_H
#define _PROC_CG_H

#ifdef  _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/param.h>		/* REQUIRED */
#include <util/plocal.h>	/* REQUIRED */
#include <util/cglocal.h>	/* REQUIRED */
#include <util/processor.h>	/* REQUIRED */
#include <proc/cguser.h>		/* REQUIRED */

#else

#include <sys/types.h>  	/* REQUIRED */
#include <sys/ksynch.h>  	/* REQUIRED */
#include <sys/param.h>  	/* REQUIRED */
#include <sys/plocal.h>  	/* REQUIRED */
#include <sys/processor.h>  	/* REQUIRED */
#include <sys/cguser.h>		/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

#if defined(__cplusplus)
extern "C" {
#endif

#if defined (_KERNEL) || defined(_KMEMUSER)

#define CG_FULLONLINE		0x1
#define CG_CONFIGURED           0x4
#define CG_HALFONLINE		0x8

#define BOOTCG          	0 
#define CGDTIMEOUT              0
#define REMFORK                 1

/*
 * Per Processor Module "cg_t" structure. This structure represents 
 * the Hardware resource that consists of a set of processors that
 * have symmetric access to shared memory. 
 * It is allocated at boot time in an array.
 * The base address of the array is stored in a kernel variable
 * called "cg_array".
 */

typedef struct cpu_group {
		int cg_status;		/* CG status		*/
		int cg_bind;		/* CG bind status	*/
		int cg_cpunum;		/* Number of CPU in this CG */
                uint cg_kl1pt_phys;     /* Phys addr. of l1 page table */
		processorid_t cg_cpuid[MAXNUMCPU]; /* CPU id list */
} cg_t;

/*
 * This structure represents information about all the Processor Modules
 * in the system.
 */

typedef struct {
	cgnum_t cg_nconf;	/* number of CGs configured */
	cgnum_t cg_nonline;	/* number of CGs online */
	cgnum_t cg_nonlineu;	/* number of CGs online by the SA */
} global_cginfo_t; 

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern cg_t		cg_array[MAXNUMCG];
extern global_cginfo_t 	global_cginfo;
extern cgnum_t      	cpu_to_cg[MAXNUMCPU];
extern int		cg_kbindindex[MAXNUMCG]; 
extern int		cg_dtimeindex[MAXNUMCG]; 

extern cgid_t	cgnum2cgid(cgnum_t);
extern cgnum_t	cgid2cgnum(cgid_t);
extern cgnum_t	fork_targetcg(int, int *);
extern cgnum_t	exec_targetcg(void);
#ifdef CCNUMA
extern cgnum_t	cg_kbind(cgnum_t);
extern void	cg_kunbind(cgnum_t);
#endif
extern void	cglocal_init(cgnum_t);
extern engine_t	*nextcpu_incg(cgnum_t, int);

#define Ncg  			(global_cginfo.cg_nconf)
#define IsCGOnline(cgnum)	((cg_array[(cgnum)].cg_status & (CG_FULLONLINE|CG_HALFONLINE)) ? B_TRUE: B_FALSE)
#define IsCGOnforU(cgnum)	((cg_array[(cgnum)].cg_status & CG_FULLONLINE) ? B_TRUE: B_FALSE)
#define mycg 	        	(cg.cg_num) 	
#define Ncgonline  		(global_cginfo.cg_nonline)
#define NcgonlineU  		(global_cginfo.cg_nonlineu)
#define CurCPU 		       	(l.eng_num)
#define CPUtoCG(cpu)    	(cpu_to_cg[cpu])

extern vaddr_t cg_array_base;

extern fspin_t cg_mutex;

extern int cg_feng;
extern int cg_nonline;
extern int cg_rescpu;
extern int cg_resrq;

extern int static_bal_exec;

#define RESLOCK(cgnum) FSPIN_LOCK(CGVAR(&cg_mutex, fspin_t, (cgnum)))
#define RESUNLOCK(cgnum) FSPIN_UNLOCK(CGVAR(&cg_mutex, fspin_t, (cgnum)))

#define RESERVE_CPU(p, cnt) (*(CGVAR(&cg_rescpu, int, p)) += cnt)
#define RESERVE_CPU_L(p, cnt) (RESLOCK((p)),(*(CGVAR(&cg_rescpu, int, p)) += cnt), RESUNLOCK(p))


#define UNRESERVE_CPU(p, cnt) (*(CGVAR(&cg_rescpu, int, p)) -= cnt)
#define UNRESERVE_CPU_L(p, cnt) (RESLOCK((p)),(*(CGVAR(&cg_rescpu, int, p)) -= cnt), RESUNLOCK(p))

#define RESERVE_RQ(p, cnt) ((*(CGVAR(&cg_resrq, int, p)) += cnt))
#define RESERVE_RQ_L(p, cnt) (RESLOCK((p)),(*(CGVAR(&cg_resrq, int, p)) += cnt), RESUNLOCK(p))

#define UNRESERVE_RQ(p, cnt) (*(CGVAR(&cg_resrq, int, p)) -= cnt)
#define UNRESERVE_RQ_L(p, cnt) (RESLOCK((p)),(*(CGVAR(&cg_resrq, int, p)) -= cnt), RESUNLOCK(p))
#endif  /* _KERNEL */

#if defined(__cplusplus)
        }
#endif

#endif /* _PROC_CG_H */
