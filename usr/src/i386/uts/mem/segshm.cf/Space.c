#ident	"@(#)kern-i386:mem/segshm.cf/Space.c	1.1"

#include <sys/types.h>
#include <sys/param.h>

/*
 * The default granularity to use for placing fine grained affinity shared
 * memory segments.  This count is measured in bytes, should be non-zero,
 * and should be a multiple of the system page size.
 */
uint_t fgashm_def_granularity = PAGESIZE;
