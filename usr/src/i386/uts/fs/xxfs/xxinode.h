#ifndef _FS_XXFS_XXINODE_H	/* wrapper symbol for kernel use */
#define _FS_XXFS_XXINODE_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/xxfs/xxinode.h	1.2.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <svc/clock.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */
#include <fs/vnode.h>		/* REQUIRED */
#include <util/ipl.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <fs/fs_hier.h>		/* REQUIRED */
#include <fs/xxfs/xxhier.h>	/* REQUIRED */
#include <proc/cred.h>		/* REQUIRED */
#include <fs/buf.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/clock.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */
#include <sys/ipl.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/fs/xxhier.h>	/* REQUIRED */
#include <sys/cred.h>		/* REQUIRED */
#include <sys/buf.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifndef NADDR
#define NADDR 13
#endif
#define NIADDR  3               /* number of indirect block pointers */
#define NDADDR  (NADDR-NIADDR)  /* number of direct block pointers */
#define IB(i)   (NDADDR + (i))  /* index of i'th indirect block ptr */
#define SINGLE  0               /* single indirect block ptr */
#define DOUBLE  1               /* double indirect block ptr */
#define TRIPLE  2               /* triple indirect block ptr */

#define	NSADDR	(NADDR*sizeof(daddr_t)/sizeof(short))

/*
 * The I-node is the focus of all local file activity in UNIX.
 * There is a unique inode allocated for each active file,
 * each current directory, each mounted-on file, and the root.
 * An inode is `named' by its dev/inumber pair. Data in icommon
 * is initialized from the on-disk inode.
 *
 * Inode Locking:
 *      There are 2 lock objects in the XX inode. They are
 *      referred to as the 'rwlock', and 'spin lock'.
 *
 *      rwlock (r/w sleep lock)
 *          It is a long term lock and may be held while blocking. A
 *          file's global state is preserved by holding this lock
 *          minimally *shared*. An r/w lock is used to allow concurrency
 *          where possible. In general, operations which modify either
 *          a file's or directory's data and/or attributes require
 *          holding the lock *exclusive*. Most other operations acquire
 *          the lock *shared*. When held in *shared* mode this lock
 *          guarantees the holder that:
 *              o There are no write operations in progress for this
 *                file and furthermore, if a directory, it is not being
 *                modified by any other LWP(s).
 *              o The file's size will not change.
 *              o The attributes protected by this lock (below) will
 *                not change.
 *          When held in *exclusive* mode, this lock guarantees to the
 *          holder that:
 *              o There are no read or write operations in progress for
 *                the file, or, if a directory, there aren't any
 *                concurrent directory search/modification operations.
 *              o The holder may change the file's size
 *                (e.g., truncate or write).
 *          and the holder of the rwlock may change any of the attributes
 *          protected by this lock (below).
 *
 *          VOP_RWWRLOCK acquires rwlock in exclusive mode; VOP_RWRDLOCK
 *          acquires rwlock in shared mode; VOP_RWUNLOCK releases this
 *          lock.
 *
 *          This lock should be acquired/released as:
 *              RWSLEEP_RDLOCK(&ip->i_rwlock, PRINOD)
 *              RWSLEEP_WRLOCK(&ip->i_rwlock, PRINOD)
 *              RWSLEEP_UNLOCK(&ip->i_rwlock)
 *
 *          The following fields are protected by the rwlock:
 *              i_nlink         i_mode          i_uid
 *              i_gid           i_size
 *
 *      spin lock (spin lock)
 *          The inode fields which are updated and/or accessed frequently
 *          are covered by this lock.
 *          Holding this lock allows the holder to:
 *              o examine i_daddr[]
 *              o change the above fields to fill in holes without
 *                changing the file's size.
 *          Holding this lock in conjunction with the rwlock
 *          (held in *exclusive* mode*) allows the holder to:
 *              o truncate the file down.
 *          If the holder of the spin lock needs to block, for example,
 *          to retrieve indirect block information from disk, the spin
 *          lock is dropped. After the LWP resumes, it must re-acquire
 *          the spin lock and re-verify the disk block information because
 *          other LWPs may have run while the LWP was blocked and change
 *          backing store information. In this case, the new backing
 *          store information is used. This approach reduces lock
 *          acquisition overhead and provides greater concurrency.
 *
 *          This lock should be acquired/released as:
 *              s = LOCK(&ip->i_mutex, FS_XXINOPL)
 *              UNLOCK(&ip->i_mutex, s)
 *
 *          The following fields are protected by the spin lock:
 *              i_atime         i_ctime         i_mtime
 *              i_daddr[]       i_flag          i_nextr
 *
 *          *NOTE that IFREE may only be set/cleared in i_flag while holding
 *           the inode table lock. If IFREE is set, than no other LWP has
 *           a reference to the inode. To clear IFREE, the same condition
 *           (no other references to inode) must hold true. Thus, the
 *           spin lock is not necessary when setting or clearing IFREE.
 *
 *      Several of the inode members require no locking since they're
 *      invariant while an inode is referenced by at least 1 LWP. They
 *      are:
 *              i_dev           i_rdev         
 *              i_number        i_gen           
 */

/* incore inode */

#if defined(_KERNEL) || defined(_KMEMUSER)

typedef struct inode {
	struct inode	*i_forw;	/* hash chain, forward */
	struct inode	*i_back;	/* hash chain, back */
	struct inode	*av_forw;	/* free chain, forward */
	struct inode	**av_back;	/* free chain, back */
	struct vnode i_vnode;	/* Contains an instance of a vnode */
	lock_t  i_mutex;        /* spin lock - see above */
        rwsleep_t i_rwlock;     /* r/w sleep lock - see above */
	u_short	i_flag;		/* flags */
	o_ino_t	i_number;	/* inode number */
	dev_t	i_dev;		/* device where inode resides */
	o_mode_t i_mode;	/* file mode and type */
	o_uid_t	i_uid;		/* owner */
	o_gid_t	i_gid;		/* group */
	o_nlink_t i_nlink;	/* number of links */
	off_t	i_size;		/* size in bytes */
	time_t	i_atime;	/* last access time */
	time_t	i_mtime;	/* last modification time */
	time_t	i_ctime;	/* last "inode change" time */
	daddr_t	i_addr[NADDR];	/* block address list */
	daddr_t	i_nextr;	/* next byte read offset (read-ahead) */
	u_char 	i_gen;		/* generation number */
	ulong_t	i_vcode;	/* version code attribute */
	dev_t	i_rdev;		/* rdev field for block/char specials */
} inode_t;

/*
 * inode hashing.
 */

#define NHINO   128
struct  hinode  {
        struct  inode   *i_forw;
        struct  inode   *i_back;
};
extern struct hinode hinode[];  /* XX Hash table */

#define	i_oldrdev	i_addr[0]
#define i_bcflag	i_addr[1]	/* block/char special flag occupies
					** bytes 3-5 in di_addr 
					*/

#define NDEVFORMAT	0x1	  /* device number stored in new area */
#define i_major		i_addr[2] /* maj component in bytes 6-8 of di_addr */
#define i_minor		i_addr[3] /* min component in bytes 9-11 of di_addr */

#endif	/* _KERNEL || _KMEMUSER */

/* Flags */

#define	IUPD		0x0001		/* file has been modified */
#define	IACC		0x0002		/* inode access time to be updated */
#define	ICHG		0x0004		/* inode has been changed */
#define IFREE		0x0008		/* inode on the free list */
#define	ISYN		0x0010		/* do synchronous write for iupdat */
#define	IMOD		0x0020		/* inode times have been modified */
#define	INOACC		0x0040		/* no access time update */
#define	ISYNC		0x0080		/* do blocks allocation synchronously */
#define	IMODTIME 	0x0100		/* mod time already set */

/*
 * File types.
 */

#define	IFMT	0xF000	/* type of file */
#define	IFIFO	0x1000	/* fifo special */
#define	IFCHR	0x2000	/* character special */
#define	IFDIR	0x4000	/* directory */
#define	IFNAM	0x5000	/* XENIX special named file */
#define	IFBLK	0x6000	/* block special */
#define	IFREG	0x8000	/* regular */
#define	IFLNK	0xA000	/* symbolic link */

/*
 * File modes.
 */
#define	ISUID	VSUID		/* set user id on execution */
#define	ISGID	VSGID		/* set group id on execution */
#define ISVTX	VSVTX		/* save swapped text even after use */

/*
 * Permissions.
 */
#define	IREAD		VREAD	/* read permission */
#define	IWRITE		VWRITE	/* write permission */
#define	IEXEC		VEXEC	/* execute permission */

#ifdef _KERNEL_HEADERS

#include <fs/fski.h> /* REQUIRED */
#include <util/param.h> /* REQUIRED -- PINOD */
#include <util/debug.h> /* REQUIRED -- ASSERT */
#include <svc/systm.h> /* REQUIRED -- PRMPT */

#elif defined(_KERNEL)

#include <sys/fski.h> /* REQUIRED */
#include <sys/param.h> /* REQUIRED -- PINOD */
#include <sys/debug.h> /* REQUIRED -- ASSERT */
#include <sys/systm.h> /* REQUIRED -- PRMPT */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

extern vnodeops_t xx_vnodeops;
extern int xx_iget();
extern void xx_iinactive();

/*
 * inode-to-vnode conversion.
 */
#define	ITOV(ip)	((struct vnode *)&(ip)->i_vnode)
#define VTOI(vp)	((struct inode *)(vp)->v_data)

#define ESAME	(-1)		/* Special KLUDGE error for rename */

enum de_op	{ DE_CREATE, DE_MKDIR, DE_LINK, DE_RENAME }; /* direnter ops */
enum dr_op	{ DR_REMOVE, DR_RMDIR, DR_RENAME }; /* dirremove ops */

/*
 * This overlays the fid structure (see vfs.h).
 */
typedef struct ufid {
	u_short	ufid_len;
	o_ino_t	ufid_ino;
	long	ufid_gen;
} ufid_t;

/*
 * XX VFS private data.
 */
typedef struct xx_fs {
	struct vnode	*fs_root;	/* root vnode */
	struct buf	*fs_bufp;	/* buffer containing superblock */
	struct vnode	*fs_devvp;	/* block device vnode */
	sleep_t		fs_sblock;	/* Superblock lock */
	sleep_t		fs_renamelock;	/* Rename lock */
	long		fs_nindir;	/* bsize/sizeof(daddr_t) */
	long		fs_inopb;	/* bsize/sizeof(dinode) */
	long		fs_bsize;	/* bsize */
	long		fs_bmask;	/* bsize-1 */
	long		fs_nmask;	/* nindir-1 */
	long		fs_ltop;	/* ltop or ptol shift constant */
	long		fs_bshift;	/* log2(bsize) */
	long		fs_nshift;	/* log2(nindir) */
	long		fs_inoshift;	/* log2(inopb) */
} xx_fs_t;

#define XXFS(vfsp) ((xx_fs_t *)((vfsp)->vfs_data))

/* 
 * Remove an inode from the hash chain it's on. 
 * The calling LWP must hold the inode table lock.
*/

#define xx_iunhash(ip) {			\
	ip->i_back->i_forw = ip->i_forw;	\
	ip->i_forw->i_back = ip->i_back;	\
	ip->i_forw = ip->i_back = ip;		\
}

/*
 * Mark an inode with the current (unique) timestamp.
 */ 
#define XXIMARK(ip, flags) {				\
        timestruc_t ltime;				\
        (ip)->i_flag |= flags;				\
        GET_HRESTIME(&ltime);				\
        if ((ip)->i_flag & IUPD) {			\
                (ip)->i_mtime  = ltime.tv_sec;		\
                (ip)->i_flag |= IMODTIME;		\
	}						\
        if ((ip)->i_flag & IACC) {			\
                (ip)->i_atime  = ltime.tv_sec;		\
	}						\
	if ((ip)->i_flag & ICHG) {			\
		(ip)->i_ctime  = ltime.tv_sec;		\
	}						\
	if ((ip)->i_flag & (IUPD|IACC|ICHG)) {		\
		(ip)->i_flag |= IMOD;			\
		(ip)->i_flag &= ~(IACC|IUPD|ICHG);	\
	}						\
}
/*
 * Only check for IACC after read as the file has not been modified 
 */

#define XXACC_TIMES(ip) { 			\
	timestruc_t ltime;		        \
	pl_t s;					\
	s = LOCK(&(ip)->i_mutex, FS_XXINOPL);	\
	if ((ip)->i_flag & IACC) {		\
		(ip)->i_flag |= IMOD; 		\
		GET_HRESTIME(&ltime); 		\
                (ip)->i_atime = ltime.tv_sec;   \
		(ip)->i_flag &= ~IACC; 		\
	}					\
	UNLOCK(&(ip)->i_mutex, s); 		\
}

/*
 * Initialize an inode for first time use
 */

#define XX_INIT_INODE(ip, vfsp, type, dev) {			\
	struct vnode *vp;					\
	vp = ITOV(ip);						\
	(ip)->i_forw = (ip);					\
	(ip)->i_back = (ip);					\
	(ip)->i_flag = IFREE;					\
	(ip)->i_vnode.v_data = vp;				\
	(ip)->i_vnode.v_op = &xx_vnodeops;			\
	LOCK_INIT(&(ip)->i_mutex, FS_XXINOHIER,			\
		FS_XXINOPL, &xx_ino_spin_lkinfo, KM_SLEEP);	\
	RWSLEEP_INIT(&((ip)->i_rwlock), (uchar_t) 0,		\
		&xx_ino_rwlock_lkinfo, KM_SLEEP);		\
	VN_INIT(vp, vfsp, type, dev, 0, KM_SLEEP);		\
	vp->v_count = 0;					\
	vp->v_softcnt = 1;					\
}

/*
 * Deinitialize an inode.
 */

#define XX_DEINIT_INODE(ip) {			\
	struct vnode *vp;			\
	LOCK_DEINIT(&((ip)->i_mutex));		\
	RWSLEEP_DEINIT(&((ip)->i_rwlock));	\
	vp = ITOV(ip);				\
	VN_DEINIT(vp);				\
}

/*
 * Macros to define locking/unlocking/checking/releasing of
 *	the XENIX inode's read/write sleep lock (i_rwlock).
 */
#define XX_IRWLOCK_RDLOCK(ip)	RWSLEEP_RDLOCK(&(ip)->i_rwlock, PRINOD)

#define	XX_IRWLOCK_WRLOCK(ip)	RWSLEEP_WRLOCK(&(ip)->i_rwlock, PRINOD)

#define	XX_IRWLOCK_UNLOCK(ip)	RWSLEEP_UNLOCK(&(ip)->i_rwlock)

#define	XX_IRWLOCK_IDLE(ip)	RWSLEEP_IDLE(&(ip)->i_rwlock)
#define	XX_IRWLOCK_BLKD(ip)	RWSLEEP_LOCKBLKD(&(ip)->i_rwlock)

#define	XX_ITRYWRLOCK(ip) 	(RWSLEEP_TRYWRLOCK(&(ip)->i_rwlock) ? \
				B_TRUE : B_FALSE) 

#define	XX_ITRYRDLOCK(ip) 	(RWSLEEP_TRYRDLOCK(&(ip)->i_rwlock) ? \
				B_TRUE : B_FALSE) 

#define	XX_IWRLOCK_RELLOCK(ip)	\
	RWSLEEP_WRLOCK_RELLOCK(&(ip)->i_rwlock, PRINOD, &xx_inode_table_mutex)

#define	XX_IRDLOCK_RELLOCK(ip)	\
	RWSLEEP_RDLOCK_RELLOCK(&(ip)->i_rwlock, PRINOD, &xx_inode_table_mutex)

/*
 * Macros to define locking/unlocking for XENIX
 *	inode's spin lock (i_mutex).
 */
#define	XX_ILOCK(ip)		LOCK(&(ip)->i_mutex, FS_XXINOPL)
#define	XX_IUNLOCK(ip, pl)	UNLOCK(&(ip)->i_mutex, (pl))

struct	ifreelist {	/* must match struct inode */
	struct inode	*pad[2];
	struct inode	*av_forw;
	struct inode	*av_back;
};

extern struct ifreelist ifreelist;

#define	XX_HOLE	(-1)	/* Value used when no block allocated */

#endif	/* _KERNEL */

#define NODEISUNLOCKED  0
#define NODEISLOCKED    1

/*
 * Iupdate modes Enums:
 */
enum iupmode { IUP_SYNC, IUP_DELAY, IUP_LAZY };

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_XXFS_XXINODE_H */
