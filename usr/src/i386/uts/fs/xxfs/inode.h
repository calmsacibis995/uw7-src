#ifndef _FS_XX_INODE_H	/* wrapper symbol for kernel use */
#define _FS_XX_INODE_H	/* subject to change without notice */

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)kern-i386:fs/xxfs/inode.h	1.5.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <fs/file.h>		/* REQUIRED */
#include <fs/fski.h>		/* REQUIRED */
#include <fs/vnode.h>		/* REQUIRED */
#include <svc/clock.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/file.h>		/* REQUIRED */
#include <sys/fski.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */
#include <sys/clock.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 *	The inode is the focus of all file activity in unix.
 *	There is a unique inode allocated for each active file,
 *	each current directory, each mounted-on file, text file,
 *	and the root. An inode is 'named' by its dev/inumber
 *	pair. (iget/iget.c) Data, from mode on, is read in from
 *	permanent inode on volume.
 */

struct iisem {			/* XENIX semaphore */
	short	i_scount;	/* current semaphore count */
	short	i_eflag;	/* err flg */
	struct	file *i_headw;	/* first waiter */
	struct	file *i_tailw;	/* last waiter */
	lock_t	isem_mutex;	/* semaphore mutex lock */
};

struct iisd {				  /* XENIX shared data */
	union {
		struct  region *i_region; /* Pointer to the shared region */
		struct  iisd *i_chain;	  /* next available iisd structure */
	} i_iun;
	unsigned	i_len;		  /* limit of segment (seg size - 1) */
	short		i_snum;		  /* serial # for sdgetv, sdwaitv */
	short		i_flags;	  /* ILOCK, etc. */
	sv_t		isd_sv;		  /* synchronization variable for
					   * waiting till x_snum changed */
};

typedef	struct	inode {
	struct	inode	*i_forw;	/* inode hash chain, forward */
	struct	inode	*i_back;	/* inode hash chain, back */
	struct	inode	*av_forw;	/* freelist chain, forward */
	struct	inode	*av_back;	/* freelist chain, back */
	lock_t	i_mutex;	/* inode spin lock */
	rwsleep_t i_rwlock;	/* inode r/w sleep lock */
	int	*i_fsptr;	/* "typeless" pointer to fs dependent */
	long	i_number;	/* i number, 1-to-1 with dev address */
	ushort	i_ftype;	/* file type = IFDIR, IFREG, etc. */
	short	i_fstyp;	/* File system type */
	off_t	i_size;		/* size of file */
	ushort	i_uid;		/* owner */
	ushort	i_gid;		/* group of owner */
	ushort	i_flag;		/* flags */
	ushort	i_fill;
	cnt_t	i_count;	/* reference count */
	short	i_nlink;	/* directory entries */
	dev_t	i_rdev;		/* Raw device number */
	dev_t	i_dev;		/* device where inode resides */
	struct	mount	*i_mntdev;	/* ptr to mount dev inode resides on */
	union i_u {
		struct stdata	*i_sp;  /* Associated stream.		*/
		struct iisem	*isem;	/* ptr to XENIX semaphores */
		struct iisd	*isd;	/* ptr to XENIX shared data */
	} i_un;
	long	*i_filocks;		/* ptr to filock (structure) list */
} inode_t;

#define	i_sptr	i_un.i_sp
#define	i_sem 	i_un.isem	/* for i_ftype==IFNAM && i_namtype==IFSEM */
#define	i_sd 	i_un.isd	/* for i_ftype==IFNAM && i_namtype==IFSHD */

/*
 * For IFNAM type files, the subtype is encoded in i_rdev.
 */
#define i_namtype i_rdev

struct	ifreelist {
	int	pad[2];		/* must match struct inode !*/
	struct inode	*av_forw;
	struct inode	*av_back;
} ;

#endif	/* _KERNEL || _KMEMUSER */

#ifdef	_KERNEL

extern struct inode xxinode[];		/* The inode table itself */
extern struct ifreelist ifreelist;	/* The inode freelist table */

#endif	/* _KERNEL */

/* flags */

#define	ILOCK	0x01		/* inode is locked */
#define	IUPD	0x02		/* file has been modified */
#define	IACC	0x04		/* inode access time to be updated */
#define	IMOUNT	0x08		/* inode is mounted on */
#define	IWANT	0x10		/* some process waiting on lock */
#define	ITEXT	0x20		/* inode is pure text prototype */
#define	ICHG	0x40		/* inode has been changed */
#define ISYN	0x80		/* do synchronous write for iupdate */
#define IWROTE	0x2000		/* write has happened since open */
#define IXLOCKED 0x4000		/* enforce file locks for XENIX compatibility */

/*
 * File types:
 * WARNING: The following defines should NOT change!
 * If more file types need to be added they should
 * be added in the low bits.
 */

#define	IFMT	0xf000	/* type of file */
#define	IFDIR	0x4000	/* directory */
#define	IFCHR	0x2000	/* character special */
#define	IFBLK	0x6000	/* block special */
#define	IFREG	0x8000	/* regular */
#define	IFMPC	0x3000	/* multiplexed char special */
#define	IFMPB	0x7000	/* multiplexed block special */
#define	IFIFO	0x1000	/* fifo special */
#define	IFNAM	0x5000	/* special named file - subtype in r_dev */

#define	IFSEM  1	/* XENIX semaphore subtype of IFNAM type file */
#define	IFSHD  2	/* XENIX shared data subtype of IFNAM type file */

/*
 * File modes:
 * the System V Rel 2 chmod system call only knows about
 * ISUID, ISGID, ISVTX. Therefore, the bit positions of
 * ISUID, ISGID, and ISVTX should not change.
 */
#define	ISUID	0x800		/* set user id on execution */
#define	ISGID	0x400		/* set group id on execution */
#define ISVTX	0x200		/* save swapped text even after use */

/*
 * access requests:
 * the System V Rel 2 chmod system call only knows about
 * IREAD, IWRITE, IEXEC. Therefore, the bit positions of
 * IREAD, IWRITE, and IEXEC should not change.
 */
#define	IREAD		0x100	/* read permission */
#define	IWRITE		0x080	/* write permission */
#define	IEXEC		0x040	/* execute permission */
#define	ICDEXEC		0x020	/* cd permission */
#define	IOBJEXEC	0x010	/* execute as an object file */
				/* i.e., 410, 411, 413 */
#define IMNDLCK		0x001	/* mandatory locking set */

#define	MODEMSK		0xfff	/* Nine permission bits - read/write/ */
				/* execute for user/group/others and */
				/* ISUID, ISGID, and ISVTX */	
				/* This is another way of saying: */
				/* (ISUID|ISGID|ISVTX| */
				/* (IREAD|IWRITE|IEXEC)| */
				/* ((IREAD|IWRITE|IEXEC)>>3)| */
				/* ((IREAD|IWRITE|IEXEC)>>6)) */
#define	PERMMSK		0x1ff	/* Nine permission bits: */
				/* ((IREAD|IWRITE|IEXEC)| */
				/* ((IREAD|IWRITE|IEXEC)>>3)| */
				/* ((IREAD|IWRITE|IEXEC)>>6)) */
#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_XX_INODE_H */
