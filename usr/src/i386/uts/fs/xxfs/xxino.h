#ifndef _FS_XXFS_XXINO_H	/* wrapper symbol for kernel use */
#define _FS_XXFS_XXINO_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/xxfs/xxino.h	1.2.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Inode structure as it appears on a disk block.  Of the 40 address
 * bytes, 39 are used as disk addresses (13 addresses of 3 bytes each)
 * and the 40th is used as a file generation number.
 */

#ifdef _KERNEL_HEADERS

#include <fs/fski.h>	/* REQUIRED */
#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/fski.h>	/* REQUIRED */
#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

typedef	struct	xxdinode {
	o_mode_t	di_mode;	/* mode and type of file */
	o_nlink_t	di_nlink;    	/* number of links to file */
	o_uid_t		di_uid;      	/* owner's user id */
	o_gid_t		di_gid;      	/* owner's group id */
	off_t		di_size;     	/* number of bytes in file */
	char  		di_addr[39];	/* disk block addresses */
	unsigned char	di_gen;		/* file generation number */
	time_t		di_atime;   	/* time last accessed */
	time_t		di_mtime;   	/* time last modified */
	time_t		di_ctime;   	/* time created */
} xxdinode_t;

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_XXFS_XXINO_H */
