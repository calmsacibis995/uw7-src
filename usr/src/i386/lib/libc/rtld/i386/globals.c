#ident	"@(#)rtld:i386/globals.c	1.14"

#include "machdep.h"
#include "rtld.h"
#include "externs.h"
#include <sys/types.h>
#include <sys/types.h>
#include "stdlock.h"

/* Global Lock for reentrant rtld */
#ifdef _REENTRANT
StdLock	_rtld_lock;
#endif

/* declarations of global variables used in ld.so */

rt_map	*_rt_map_head = 0;	/* head of rt_map list*/
rt_map	*_rt_map_tail = 0;		/* tail of rt_map list */
rt_map	*_rtld_map = 0;		/* rtld rt_map */

struct r_debug _r_debug = { 
	LD_DEBUG_VERSION, 
	0,
	(ulong_t)_r_debug_state,
	RT_CONSISTENT,
	0
}; /* debugging information */

int	_rt_devzero_fd = -1;	/* file descriptor for /dev/zero */
char	*_rt_error = 0;		/* string describing last error */
char	*_rt_proc_name = 0;	/* file name of executing process */

size_t		_rt_syspagsz = 0;	/* system page size */
ulong_t		_rt_flags = 0;	/* machine specific file flags */
int		_nd = 0;	/* store value of _end */

CONST char	_rt_name[] = "dynamic linker";

/* fini list for startup objects */
mlist		*_rt_fini_list = 0;

int		_rt_dlsym_order_compat = 0;  /* set compatibility mode
					      * for dlsym searches
					      */

/* control flags */
#ifndef GEMINI_ON_OSR5
ulong_t		_rt_control = CTL_COPY_CTYPE|CTL_NO_DELETE;
#else
ulong_t		_rt_control = CTL_COPY_CTYPE|CTL_NO_DELETE|CTL_NOTELESS_SHUNT;
#endif

struct rel_copy *_rt_copy_entries = 0;	/* list of copy relocs */

void (*_rt_event)() = 0;	/* used by profilers */

#ifdef DEBUG
int	_rt_debugflag;		/* debugging level */
#endif
