#ifndef _FS_DOSFS_HIER_H	/* wrapper symbol for kernel use */
#define _FS_DOSFS_HIER_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/dosfs/dosfs_hier.h	1.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ipl.h>	/* REQUIRED */
#include <fs/fs_hier.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

#ifdef _KERNEL

/*
 * This header file has all the hierarchy and minipl information 
 * pertaining to the dosfs_ file system. Note that all lock hierarchies in 
 * this file will be expressed as an offset from a base hierarchy value that
 * will be associated with the dosfs_ file system. Clearly, locks that can be 
 * held across subsystem boundaries need to be dealt with separately.
 * These "global" locks will have their hierarchy values defined in the 
 * ghier.h file under the util directory.
 *
 */

#define DOSFS_HIER_BASE	FS_HIER_BASE

/*
 * Hierarchy values used by the file system subsystem:
 *
 *	PLHI:	DOSFS_HIER_BASE to DOSFS_HIER_BASE + 10
 *	PL6:	DOSFS_HIER_BASE to DOSFS_HIER_BASE + 5
 */

/*
 * The hierarchy values will be checked amongst locks that have identical
 * minipl and hence the hierarchy namespace can be shared among locks that
 * have different minipls.
 *
 */

#define FS_DOSFSLISTHIER	DOSFS_HIER_BASE + 5 	/* dosfs:dosfs_table_mutex */
#define FS_DOSFSLISTPL	PLFS

#define FS_DOSFSINOHIER	DOSFS_HIER_BASE + 5	/* dosfs:i_mutex */
#define FS_DOSFSINOPL	PLFS

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_DOSFS_HIER_H */
