#ifndef _FS_XXFS_XXPARAM_H	/* wrapper symbol for kernel use */
#define _FS_XXFS_XXPARAM_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/xxfs/xxparam.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Filesystem parameters.
 */
#define	XXSUPERB	((daddr_t)2)	/* block number of the super block */
#define	XXDIRSIZ	14		/* max characters per directory */
#define	XXNICINOD	100		/* number of superblock inodes */
#define	XXROOTINO	2		/* i-number of all roots */

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_XXFS_XXPARAM_H */
