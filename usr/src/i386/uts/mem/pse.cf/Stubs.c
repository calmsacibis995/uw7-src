#ident	"@(#)kern-i386:mem/pse.cf/Stubs.c	1.5.5.1"
#ident	"$Header$"

/* Stubs for segpse module */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/vnode.h>
#include <sys/vmparam.h>

#ifndef	NOWORKAROUND
uint_t pse_major = UINT_MAX;
#endif
asm(".globl segpse_create ; segpse_create: ; jmp segdev_create;");
ppid_t pse_mmap() { return NOPAGE; }
vnode_t *pse_makevp() { return NULL; }
void pse_freevp() {}
caddr_t pse_physmap() { return NULL; }
boolean_t pse_physmap_free() { return B_FALSE; }

uint_t segkmem_pse_bytes = 0;
uint_t segkmem_pse_size = 0;

void segkmem_pse_pagepool_init() {}
void segkmem_pse_calloc() {}
void segkmem_pse_create() {}
void segkmem_pse_init() {}
vaddr_t segkmem_pse_alloc() { return NULL; }
boolean_t segkmem_pse_free() { return B_FALSE; }
vaddr_t segkmem_pse_alloc_physreq() { return NULL; }

boolean_t pse_hat_chgprot() { return B_FALSE; }
boolean_t pse_pae_hat_chgprot() { return B_FALSE; }
void pse_hat_devload() {}
void pse_pae_hat_devload() {}
void pse_hat_unload() {}
void pse_pae_hat_unload() {}
void pse_hat_statpt_devload() {}
void pse_pae_hat_statpt_devload() {}
void pse_hat_statpt_unload() {}
void pse_pae_hat_statpt_unload() {}
