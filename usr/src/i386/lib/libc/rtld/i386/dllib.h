#ifndef _dllib_h_
#define _dllib_h_
#ident	"@(#)rtld:i386/dllib.h	1.8"

#include "machdep.h"
#include "rtld.h"
#include <sys/types.h>

/* information for dlopen, dlsym, dlclose on libraries linked by
 * rtld
 * Each shared object referred to in a dlopen call has an associated
 * dllib structure.  For each such structure there is a list of
 * the link objects dependent on that shared object. 
 */

typedef struct dllib {
	rt_map		*dl_object; /* "main" object for this group */
	int		dl_refcnt;  /* group reference count */
	ulong_t 	dl_group;   /* group id of this dlopen */
	mlist		*dl_fini;   /* first fini for this group */
	mlist		*dl_order;  /* ordering for dlsym searches */
	struct dllib	*dl_next;   /* next dllib struct */
} DLLIB;
#endif
