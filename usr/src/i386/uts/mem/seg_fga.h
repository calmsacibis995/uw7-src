#ifndef _MEM_SEG_FGA_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_FGA_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/seg_fga.h	1.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ifdef _KERNEL_HEADERS

#include <util/ksynch.h> /* REQUIRED */
#include <util/types.h>	/* REQUIRED */
#include <proc/mman.h>	/* REQUIRED */
#include <mem/fgashm.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ksynch.h> /* REQUIRED */
#include <sys/types.h>	/* REQUIRED */
#include <sys/mman.h>	/* REQUIRED */
#include <vm/fgashm.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * A pointer to this structure is passed to segfga_create().
 */
struct segfga_crargs {
	struct fgashm_t *	fgap;
	uchar_t			prot;
	uchar_t			maxprot;
} ;

/*
 * (Semi) private data maintained by the seg_fga driver per segment mapping.
 */
struct segfga_data {
	fgavpg_t **		sfd_vpgpp;		/* vpg array */
	sv_t *			sfd_vpgsvp;		/* sv for page locking */
	lock_t *		sfd_vpglckp;		/* for page locking */
	hatshpt_t *		sfd_hatshptp;		/* shared page tables */
	fgashm_aff_head_t *	sfd_affp;		/* affinity info */
};

#define SEGFGA_PROT_MASK 	(PROT_READ | PROT_WRITE | PROT_USER)

#ifdef _KERNEL

extern int		segfga_create(struct seg *, void *);
extern struct seg_ops 	segfga_ops;

#define IS_SEGFGA(segp)		((segp)->s_ops == &segfga_ops)

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _MEM_SEG_FGA_H */
