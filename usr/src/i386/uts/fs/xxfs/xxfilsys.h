/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ifndef _FS_XX_XXFILSYS_H	/* wrapper symbol for kernel use */
#define _FS_XX_XXFILSYS_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/xxfs/xxfilsys.h	1.3"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Structure of the super-block
 */

#ifdef	_KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <fs/xxfs/xxparam.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/fs/xxparam.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

#pragma pack(2)

#define	XXBSIZE		1024	/* XENIXFS only support 1024 byte blocks */
#define	XXNSBFILL	371	/* aligns s_magic, .. at end of super block */
#define	XXNICFREE	100	/* number of superblock free blocks */

typedef	struct	xxfilsys {
	u_short	s_isize;	  	/* size in blocks of i-list */
	daddr_t	s_fsize;	  	/* size in blocks of entire volume */
	short	s_nfree;	  	/* number of addresses in s_free */
	daddr_t	s_free[XXNICFREE];  	/* free block list */
	short	s_ninode;	  	/* number of i-nodes in s_inode */
	o_ino_t	s_inode[XXNICINOD];	/* free i-node list */
	char	s_flock;	  	/* lock during free list manipulation */
	char	s_ilock;	  	/* lock during i-list manipulation */
	char  	s_fmod; 	  	/* super block modified flag */
	char	s_ronly;	  	/* mounted read-only flag */
	time_t	s_time; 	  	/* last super block update */
	daddr_t	s_tfree;	  	/* total free blocks*/
	o_ino_t	s_tinode;	  	/* total free inodes */
	short   s_dinfo[4];       	/* device information */
	char	s_fname[6];	  	/* file system name */
	char	s_fpack[6];	  	/* file system pack name */
	char   	s_clean;   	  	/* S_CLEAN on proper close */
	char    s_fill[XXNSBFILL];  	/* to make sizeof(xxfilsys) be BSIZE */
	long    s_magic;          	/* indicates version of xxfilsys */
	long	s_type;		  	/* type of new file system */
} xxfilsys_t;

#pragma pack()

#define	XXSBSIZE	sizeof(xxfilsys_t)
#define	S_CLEAN		0106        	/* arbitrary magic value  */

/* s_magic, magic value for file system version */
#define	S_S3MAGIC	0x2b5544	/* system 3 arbitrary magic value */

/* codes for file system version (for utilities) */
#define	S_V2		1		/* version 7 */
#define	S_V3		2		/* system 3 */

#define getfs(vfsp)	\
	((xxfilsys_t *)((struct xx_fs *)vfsp->vfs_data)->fs_bufp->b_addrp)

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_XX_XXFILSYS_H */
