#ifndef _FS_XXFS_XXDIR_H	/* wrapper symbol for kernel use */
#define _FS_XXFS_XXDIR_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/xxfs/xxdir.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <fs/xxfs/xxparam.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/fs/xxparam.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

typedef struct xxdirect {
	o_ino_t	d_ino;		/* xx inode type */
	char	d_name[XXDIRSIZ];
} xxdirect_t;

#define XXSDSIZ	(sizeof(xxdirect_t))

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_XXFS_XXDIR_H */
