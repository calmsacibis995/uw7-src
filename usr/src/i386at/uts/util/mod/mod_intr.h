#ifndef _UTIL_MOD_MOD_INTR_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MOD_INTR_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:util/mod/mod_intr.h	1.2.7.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#include <svc/psm.h>

/*
 * Doublely-linked list for interrupt sharing
 */
struct	mod_shr_v	{
	struct mod_shr_v	*msv_next; 	/* next */
	struct mod_shr_v	*msv_prev; 	/* previous */
	int		(*msv_handler)();	/* pointers to intr handlers */
	void		*msv_idata; 		/* idata for intr handlers */
};

/*
 * Information about a specific interrupt vector. This structure contains the
 * intr_dist_t structure shared with the PSM as well as remaining information
 * used by the kernel to process an interrupt.
 */
struct intr_vect_t {
	ms_intr_dist_t		iv_idt;		/* the idt shared with PSM */
						/* MUST BE FIRST */
	int			(*iv_ivect)();  /* driver entry point */
	struct mod_shr_v	*iv_mod_shr_p;	/* head of list of info about
						   drivers */
	int 			iv_mod_shr_cnt;	/* # of hanlders sharing intr */
	ms_cpu_t		iv_bindcpu;	/* cpu driver is bound to */
	int			iv_itype;	/* type of interrupt */
	int			iv_upcount;	/* number of non-mp safe drivers */
	unsigned char		iv_intpri;	/* interrupt priority */
	ms_event_t		iv_osevent;	/* os event that corresponds to entry 
						 * (0 = external interrupt) */
	int			*iv_idata; 	/* idata for the handler */
};

/*
 * Macros to convert between intr_vect_t pointers and ms_intr_dist_t pointers. 
 * Assumes the IDT is first in the structure.
 */
#define IDTP2IVP(p)	((struct intr_vect_t*) (p))
#define IVP2IDTP(p)	((ms_intr_dist_t*) (p))
extern struct intr_vect_t *intr_vect;
extern struct _Compat_intr_vect_t *_Compat_intr_vect;


#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_MOD_MOD_INTR_H */
