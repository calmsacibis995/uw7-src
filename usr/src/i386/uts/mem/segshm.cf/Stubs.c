#ident	"@(#)kern-i386:mem/segshm.cf/Stubs.c	1.2.1.2"
#ident	"$Header$"

/* Stubs for segumap module */

#include <sys/errno.h>

int segumap_create() { return ENOSYS; }
int segumap_create_obj() { return EUNATCH; }
int segumap_delete_obj() { return ENOSYS; }
int segumap_load_map(){ return 0; }
int segumap_unload_map(){ return 0; }
int segumap_alloc_mem(){ return 0; }
int segumap_ctl_memloc(){ return 0; }

int segumap_ops;

/* Stubs for seg_fga module */

int segfga_create() { return ENOSYS; }
int shm_makefga() { return ENOSYS; }
void shm_rm_fga() { return; }
int fgashm_reaffine(){ return ENOSYS; }
void fgashm_lockmem() { return; }

int segfga_ops;
