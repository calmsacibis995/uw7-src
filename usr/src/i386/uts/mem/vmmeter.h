#ifndef _MEM_VMMETER_H	/* wrapper symbol for kernel use */
#define _MEM_VMMETER_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/vmmeter.h	1.8"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * Virtual memory related instrumentation and parameters.
 */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Note that all the vmmeter entries between v_first and v_last
 *  must be unsigned [int], as they are used as such in vmmeter().
 */
struct vmmeter {
#define	v_first	v_swtch
	unsigned v_swtch;	/* context switches */
	unsigned v_trap;	/* calls to trap */
	unsigned v_syscall;	/* calls to syscall() */
	unsigned v_intr;	/* device interrupts */
	unsigned v_pdma;	/* pseudo-dma interrupts XXX: VAX only */
	unsigned v_pswpin;	/* pages swapped in */
	unsigned v_pswpout;	/* pages swapped out */
	unsigned v_pgin;	/* pageins */
	unsigned v_pgout;	/* pageouts */
	unsigned v_pgpgin;	/* pages paged in */
	unsigned v_pgpgout;	/* pages paged out */
	unsigned v_intrans;	/* intransit blocking page faults */
	unsigned v_pgrec;	/* total page reclaims (includes pageout) */
	unsigned v_xsfrec;	/* found in free list rather than on swapdev */
	unsigned v_xifrec;	/* found in free list rather than in filsys */
	unsigned v_exfod;	/* pages filled on demand from executables */
			/* XXX: above entry currently unused */
	unsigned v_zfod;	/* pages zero filled on demand */
	unsigned v_vrfod;	/* fills of pages mapped by vread() */
			/* XXX: above entry currently unused */
	unsigned v_nexfod;	/* number of exfod's created */
			/* XXX: above entry currently unused */
	unsigned v_nzfod;	/* number of zfod's created */
			/* XXX: above entry currently unused */
	unsigned v_nvrfod;	/* number of vrfod's created */
			/* XXX: above entry currently unused */
	unsigned v_pgfrec;	/* page reclaims from free list */
	unsigned v_faults;	/* total page faults taken */
	unsigned v_scan;	/* page examinations in page out daemon */
	unsigned v_rev;		/* revolutions of the paging daemon hand */
	unsigned v_seqfree;	/* pages taken from sequential programs */
			/* XXX: above entry currently unused */
	unsigned v_dfree;	/* pages freed by daemon */
	unsigned v_fastpgrec;	/* fast reclaims in locore XXX: VAX only */
#define	v_last v_fastpgrec
	unsigned v_swpin;	/* swapins */
	unsigned v_swpout;	/* swapouts */
};

/*
 * Systemwide totals computed every five seconds.
 * All these are snapshots, except for t_free.
 */
struct vmtotal {
	short	t_rq;		/* length of the run queue */
	short	t_dw;		/* jobs in ``disk wait'' (neg priority) */
	short	t_pw;		/* jobs in page wait */
	short	t_sl;		/* ``active'' jobs sleeping in core */
	short	t_sw;		/* swapped out ``active'' jobs */
	int	t_vm;		/* total virtual memory */
			/* XXX: above entry currently unused */
	int	t_avm;		/* active virtual memory */
			/* XXX: above entry currently unused */
	short	t_rm;		/* total real memory in use */
	short	t_arm;		/* active real memory */
	int	t_vmtxt;	/* virtual memory used by text */
			/* XXX: above entry currently unused */
	int	t_avmtxt;	/* active virtual memory used by text */
			/* XXX: above entry currently unused */
	short	t_rmtxt;	/* real memory used by text */
			/* XXX: above entry currently unused */
	short	t_armtxt;	/* active real memory used by text */
			/* XXX: above entry currently unused */
	short	t_free;		/* free memory pages (60 second average) */
};

/* 
 * mem_avail_state:
 *
 * Global variable to inform all subsystems how memory loaded the system 
 * has become. All subsystems are expected to take a common view of the
 * level of memory criticality, and thereby work co-operatively to increase
 * memory availability when the need arises instead of each subsystem 
 * directly consulting the count of free pages.
 *
 * The value assumed by mem_avail_state will vary from 0 (least critical)
 * to 4 (most desperate). It will be set every second.
 */
extern	int	mem_avail_state;	

#define MEM_AVAIL_EXTRA_PLENTY	0
#define	MEM_AVAIL_PLENTY	1
#define MEM_AVAIL_NORMAL	2
#define MEM_AVAIL_FAIR		3
#define MEM_AVAIL_POOR		4
#define MEM_AVAIL_DESPERATE	5

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_VMMETER_H */
