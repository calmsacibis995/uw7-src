#ifndef _FS_DOSFS_DOSFSDATA_H	/* wrapper symbol for kernel use */
#define _FS_DOSFS_DOSFSDATA_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/dosfs/dosfs_data.h	1.1"
#ident  "$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ipl.h>
#include <util/ksynch.h>

#elif defined(_KERNEL) 

#include <sys/ipl.h>
#include <sys/ksynch.h>

#endif /* _KERNEL_HEADERS */

extern struct denode *denode;                 /* the denode table itself */
extern long int ndenode;

/*
 * Inode Hash List/Free List
 */
extern lock_t dosfs_denode_table_mutex;
extern lkinfo_t dosfs_denode_table_lkinfo;

/*
 * Update lock
 */
extern sleep_t dosfs_updlock;
extern lkinfo_t dosfs_updlock_lkinfo;

/*
 * Per-Inode Lockinfo Structures
 */
extern lkinfo_t dosfs_deno_lock_lkinfo;

/*
 * Per Mounted-fs Lockinfo Structure
 */
extern lkinfo_t dosfs_rename_lkinfo;

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_DOSFS_DOSFSDATA_H */
