/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_UTIL_CGLOCAL_H	/* wrapper symbol for kernel use */
#define	_UTIL_CGLOCAL_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:util/cglocal.h	1.3.3.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Definitions of per-processor data structures.
 */

#ifdef _KERNEL_HEADERS
#include <mem/rff.h>		/* REQUIRED */
#include <mem/anon.h>		/* REQUIRED */
#include <mem/seg_kvn.h>
#include <mem/as.h>
#include <mem/page.h>
#include <mem/zbm.h>
#include <io/strsubr.h>		/* REQUIRED */
#include <mem/immu.h>		/* REQUIRED */
#include <mem/kma.h>		/* REQUIRED */
#include <mem/kmalist.h>	/* REQUIRED */
#include <mem/kmem.h>		/* REQUIRED */
#include <mem/vmmeter.h>	/* REQUIRED */
#include <proc/seg.h>		/* REQUIRED */
#include <proc/tss.h>		/* REQUIRED */
#include <svc/fp.h>		/* REQUIRED */
#include <util/emask.h>		/* REQUIRED */
#include <util/ipl.h> 		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/locktest.h>	/* REQUIRED */
#include <util/metrics.h>	/* REQUIRED */
#include <util/param.h>		/* REQUIRED */
#include <util/sysmacros.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <vm/rff.h>		/* REQUIRED */
#include <vm/anon.h>		/* REQUIRED */
#include <sys/strsubr.h>	/* REQUIRED */
#include <sys/immu.h>		/* REQUIRED */
#include <vm/kma.h>		/* REQUIRED */
#include <vm/kmalist.h>		/* REQUIRED */
#include <sys/kmem.h>		/* REQUIRED */
#include <sys/vmmeter.h>	/* REQUIRED */
#include <sys/seg.h>		/* REQUIRED */
#include <sys/tss.h>		/* REQUIRED */
#include <sys/fp.h>		/* REQUIRED */
#include <sys/emask.h>		/* REQUIRED */
#include <sys/ipl.h> 		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/locktest.h>	/* REQUIRED */
#include <sys/metrics.h>	/* REQUIRED */
#include <sys/param.h>		/* REQUIRED */
#include <sys/sysmacros.h>	/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * The per-processor-group private data is allocated at boot time.
 * It consists of:
 *
 *		Data-Structure		Size (HW Pages)
 *		==============		===============
 *		CG Local Data		whatever, rounded to page size
 *
 * A virtual address pointer to this is saved in the cg structure of
 * the cpu group.
 */

#if defined _KERNEL || defined _KMEMUSER

struct cglocalmet {
    /* TODO: optimize layout for 32-byte cache-lines */

    /*
     * After init, metcg_wait_locks are touched only by the
     * local CPU-group via the MET_IOWAIT macros in fs/bio.c
     * and the MET_PHYSIOWAIT macros in io/physio.c.
     */
    struct mets_wait_locks  metcg_wait_locks;

    /*
     * fast spin locks used in fio.c and xsem.c
     */
    fspin_t metcg_file_list_mutex;


    /*
     * After init, metcg_wait is touched only when mets_wait_locks
     * are touched or uniformly via the MET_IO_OUTSTANDING macro
     * in lclclock().
     */
    struct mets_wait        metcg_wait;

    struct mets_files       metcg_files;              /* file & flck table metrics */
    struct mets_inodes      metcg_inodes[MET_FSTYPES];/* inode table metrics */
#ifdef PERF
    struct mets_fsinfo      metcg_fsinfo[MET_FSTYPES];/* fstype paging info */
#endif /* PERF */
};

/*
 * Cpu Group Local Data.
 * Fields are addressed by "cg." -- eg, cg.cg_num.  See sim_setup_cglocal() 
 * for how this is set up.
 */

struct cglocal {
    struct engine  *cg_feng;          	/* First engine in CG */
    struct engine  *cg_leng;  		/* One beyond last engine in CG */
    int 	   cg_nonline;		/* CPUS online within CG */

    /*--------  from proc subsystem  ---------------------*/    
    cgnum_t 	   cg_num;             	/* Cpu group number */
    struct proc	  *proc_sys;
    struct cred	  *sys_cred;
    struct rlimit  *sys_rlimits;
    struct runque  *cg_rq;       	/* Per CG level run Q */
    int 	   cg_nidle;            /* number of idle CPUs in CG */
    int 	   cg_avgqlen;          /* average run queue length in CG */

    /* ------ for timeout/callout handling --------- */
    struct callout *cg_scofree;
    struct callout *cg_dcofree;
    uint_t cg_ncofree;
    uint_t cg_sncofree;
    uint_t cg_dncofree;
    uint_t cg_conactive;
    uint_t cg_condead;
    list_t cg_coid_hash[NCALLOUT_HASH];
    toid_t cg_timeid;
    struct callout *cg_todo;		/* CG level callout table */
    fspin_t cg_time_mutex;

    /* ----- for remote fork policy ------ */

    fspin_t cg_mutex;              /* mutex for protecting reservations */
    ulong_t cg_rescpu;             /* number of CPU reservations */
    ulong_t cg_resrq;              /* number of run queue reservations */
    ulong_t cg_totalqlen;          /* total number of runnable lwps */

    /* --------- from streams ------------------*/

    int strnsched;                  /* Max No. of svc procs on a CG */
    struct qsvc qsvc;               /* CG wise svc proc queue */
    int Nsched;                     /* #  schedulers running on the CG */
    struct engine *lastpick;	/* last cpu scheduled on the CG */
    
    struct bclist bcall;            /* list of waiting bufcalls on CG */
    toid_t strbcid;                 /* CG wise bufcall ids */

    /*--------  from vm_page.c ---------------------*/

    pl_t	vm_pagefreelockpl; /* pl for current vm_pagefreelock holder */
    
    int	mem_freemem[NPAGETYPE];/* # current free pages per type */
    int	maxfreemem[NPAGETYPE];	/* max # free pages per type */
    
    page_t *page_freelist[NPAGETYPE];  /* free, clean, non-file pages */
    page_t *page_cachelist[NPAGETYPE]; /* free, clean, file pages */
    page_t *page_anon_cachelist[NPAGETYPE]; /* anon_decref()ed pages */
    page_t *page_dirtyflist;	  /* free, dirty, non-anon file pages */
    page_t *page_dirtyalist;	  /* free, dirty, anon file pages */
    
    int page_freelist_size[NPAGETYPE];
    int page_cachelist_size[NPAGETYPE];
    int page_anon_cachelist_size[NPAGETYPE];
    
#ifdef DEBUG
    int page_dirtyflist_size;
    int page_dirtyalist_size;
#endif

    int page_dirtylists_size[NPAGETYPE];

    /* Page slice table data structures */
    page_slice_t *pslice_table;
    
    page_t *pages;			/* 1st pagepool page per type */
    page_t *epages;			/* last pagepool page per type (+1) */
    
    page_t **page_cs_table;
#ifdef DEBUG
    int page_cs_table_size;	/* number of entries in the page_cs_table */
#endif
    
    page_t *page_swapreclaim_nextpp; /* pp at which to begin next scan*/
    /* Set in page_init */
    /* Lock serializing swapreclaim scans */
    lock_t page_swapreclaim_lck; /* Initialized in page_init */
    
#ifdef DEBUG
    /*
     * additional DEUBG only counters
     */
    uint_t page_swapreclaim_reclaims;       /* count of reclaims done */
    uint_t page_swapreclaim_desperate;      /* times we were desperate */
#endif
    
    /*--------  from vm_anon.c ---------------------*/

    anoninfo_t  anoninfo;	/* anonfs reservation info */
    ulong_t	anon_allocated;	/* anon_t(s) currently allocated */
    fspin_t	anon_free_lck;	/* Protects the free list of anon_t */
    /* structures and anon_allocated */
    
    /*	The following are protected by the anon_table_lck.	*/
    uint_t      anon_tabgrow;	/* the anon_table entry currently growing */
    ulong_t	anon_max;	/* maximum potentially needed */
    ulong_t	anon_clmax;	/* capacity of the cluster table */
    ulong_t	anon_clcur;	/* currently in the cluster table */
    ulong_t     anon_clfail;	/* index at cluster table at failure time  */
    anon_t	*anon_free_listp;
    
    /*--------  from memresv.c ---------------------*/
    fspin_t vm_memlock;
    rmeminfo_t rmeminfo;

    /*--------  from kma.c ---------------------*/
    kmlist_t blist[KM_NPOOLS + NSPECIALBUF];
    kmlist_t *km_grow_start;

    kmlist_t *kmlistp[KM_N_LISTIDX];
    kmlocal_t kmlocal_speclist;
    lock_t kma_giveback_lock;
    
    /* last time forced giveback from kma_force_shrink */
    clock_t kma_lastgive;	
    enum giveback_action kma_giveback_action;
    sv_t kma_giveback_sv;
    uint_t kma_giveback_seq;	/* Incremented for each completed action */

    sv_t kma_completion_sv;
    int kma_spawn_error;	/* Error, if any, during daemon spawning */
    
    engine_t * volatile kma_new_eng;
    emask_t kma_live_daemons;
    emask_t kma_targets;
#ifdef DEBUG
    uint_t kma_shrinkcount;	/* Statistic: # times KMA pools were shrunk */
#endif
    uint_t kma_shrinktime;	/* Next time to shrink pools */
    fspin_t kma_lock;		/* Mutex for kma_shrinktime and kma_waitcnt */
    
    rff_t kma_rff;	/* Private pageout daemon pool; mutexed by kma_lock */
    vaddr_t kma_pageout_pool;	/* Address of pageout private pool */

    /*--------  from vm_pvn.c  ---------------------*/    
    struct pageheader pvn_pageout_markers[2];

    /*--------  from pageout   ---------------------*/    
    lwp_t *lwp_pageout;	/* LWP of the pageout daemon */
    int pushes;  /* number of pages pushed on any one run of pageout */
    int mem_lotsfree[NPAGETYPE]; /* threshold values for lotsfree */
    int mem_minfree[NPAGETYPE];	/* threshold values for minfree */
    int mem_desfree[NPAGETYPE];	/* threshold values for desfree */
    event_t	  pageout_event; /* event pageout waits on */ 

    boolean_t hat_static_cg_pallocup;	/* B_TRUE => palloc() enabled */

    /*--------  from fsflush_pagesync   ---------------------*/    
    lwp_t *lwp_fsflush_pagesync; /* LWP of the fsflush_pagesync daemon */

    /*--------------- sysinit.c ------------------------*/
    struct desctab_info global_dt_info[NDESCTAB];
    struct desctab std_idt_desc;
    struct segment_desc *global_ldt;

    /* --------  from lwpscalls.c  ---------------------*/    
    event_t		spawn_sys_lwp_spawnevent;
    event_t		spawn_sys_lwp_waitevent;
    sleep_t		spawn_mutex;
    enum spawn_action   spawn_sys_lwp_action;
    u_long		spawn_sys_lwp_flags;
    void 		*spawn_sys_lwp_argp;
    k_lwpid_t 	        spawn_sys_lwp_lwpid;
    int 		spawn_sys_lwp_return;

    /* --------  from vm_hat.c  ---------------------*/    
    pl_t ptpool_pl;
    pl_t mcpool_pl;
    pl_t TLBSoldpl;

    /* --------  from pooldaemon.c  ---------------------*/    
    event_t poolrefresh_event;
    clock_t poolrefresh_lasttime;
    boolean_t poolrefresh_pending;

    /* --------  from pagesync daemon ---------------------*/
    event_t fspsync_event;
    int     fsf_toscan;          /* subset of the pagepool to be scanned */
    page_t *fsf_next_page;      /* next page for fsflush to scan */
    clock_t fsf_age_time;    /* aging interval for the dirty pages */

    /* --------  from vm_sched.c  ---------------------*/
    uint_t       cg_ocpu_tsum;
    uint_t       cg_ocpu_user_sys_sum;

    int          cg_mem_avail_state;
    int          cg_clfree_avail_state;
    uint_t       cg_busyratio;
    int          cg_avefree;
    int          cg_ave_clfree;
    int          cg_mem_avail;

    struct cglocalmet cglocalmet;
};

/*
 * There must be an integral # pages allocated to the cglocal structure,
 * so it can be mapped separate from other data-structures.
 */

#define	CGL_PAGES	mmu_btopr(sizeof(struct cglocal))
/*
 * Processor Module private pages layout.
 *
 * This just provides a simple way to allocate and locate the
 * per-CG memory during CG initialization (boot).
 *
 */

struct	cgpriv_pages {
	char	cg_local[CGL_PAGES][MMU_PAGESIZE];  /* CG-local vars */
};

#define	SZCGPRIV	sizeof(struct cgpriv_pages)
#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL
extern struct cglocal cg;
#define CGVAR_INVAR (KVCGLOCAL - cg_array_base)
#define CGVAR(addr, deftype, cgno) \
    ((deftype *)((caddr_t)addr - CGVAR_INVAR + (cgno * mmu_ptob(CGL_PAGES))))

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_CGLOCAL_H */
