#ifndef _MEM_PSE_HAT_H	/* wrapper symbol for kernel use */
#define _MEM_PSE_HAT_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/pse_hat.h	1.1"
#ident	"$Header$"

#ifdef	_KERNEL

#include <util/types.h>	/* REQUIRED */
#include <mem/seg.h>

/*
 * PSE hat layer support routine, used by segpse and segkpse.
 */
void pse_hat_ptfree(hatpt_t *);
boolean_t pse_hat_chgprot(struct seg *, vaddr_t, ulong_t, uint_t, boolean_t);
void pse_hat_devload(struct seg *, vaddr_t, ppid_t, uint_t);
void pse_hat_unload(struct seg *, vaddr_t, ulong_t);
void pse_hat_statpt_devload(vaddr_t, ulong_t, ppid_t, uint_t);
void pse_hat_statpt_unload(vaddr_t, ulong_t);

#endif	/* _KERNEL */

#endif /* _MEM_PSE_HAT_H */
