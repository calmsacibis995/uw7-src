#ifndef _FS_DOSFS_DOSFS_FILSYS_H       /* wrapper symbol for kernel use */
#define _FS_DOSFS_DOSFS_FILSYS_H       /* subject to change without notice */

#ident	"@(#)kern-i386:fs/dosfs/dosfs_filsys.h	1.1"
#ident  "$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *
 *  Adapted for System V Release 4	(ESIX 4.0.4)	
 *
 *  Gerard van Dorth	(gdorth@nl.oracle.com)
 *  Paul Bauwens	(paul@pphbau.atr.bso.nl)
 *
 *  May 1993
 *
 *  Originally written by Paul Popelka (paulp@uts.amdahl.com)
 *
 *  You can do anything you want with this software,
 *    just don't say you wrote it,
 *    and don't remove this notice.
 *
 *  This software is provided "as is".
 *
 *  The author supplies this software to be publicly
 *  redistributed on the understanding that the author
 *  is not responsible for the correct functioning of
 *  this software in any circumstances and is not liable
 *  for any damages caused by this software.
 *
 *  October 1992
 *
 */

struct dosfsmnt_args {

	uid_t	uid;
	gid_t	gid;
	int	mask;

};

typedef struct dosfsmnt_args	dosfsmnt_args_t;

#define	DEFUSR	(-1)
#define DEFGRP	(-1)



#if defined(_KERNEL)


/*
 *  Layout of the file system control block for a msdos.
 */
struct dosfs_vfs {
	struct vfs* vfs_vfsp;		/* vfs struct for this fs	*/
	struct vnode *vfs_devvp;	/* vnode for block device mntd	*/
	struct vnode *vfs_root;		/* vnode for root directory     */
	struct bpb50 vfs_bpb;		/* BIOS parameter blk for this fs */
	uid_t	vfs_uid;		/* uid (default 0) */
	gid_t	vfs_gid;		/* gid (default 0) */
	int	vfs_mask;		/* mode mask (default 0) */
	char path[32];			/* mount path for this fs       */
	dev_t vfs_dev;			/* block device mounted         */
	long vfs_fatblk;		/* block # of first FAT		*/
	long vfs_rootdirblk;		/* block # of root directory	*/
	long vfs_rootdirsize;		/* size in blocks (not clusters) */
	long vfs_firstcluster;		/* block number of first cluster */
	long vfs_nmbrofclusters;	/* # of clusters in filesystem	*/
	long vfs_maxcluster;		/* maximum cluster number	*/
	long vfs_freeclustercount;	/* number of free clusters	*/
	long vfs_lookhere;		/* start free cluster search here */
	long vfs_bnshift;		/* shift file offset right this
					 *  amount to get a block number */
	long vfs_brbomask;		/* and a file offset with this
					 *  mask to get block rel offset */
	long vfs_cnshift;		/* shift file offset right this
					 *  amount to get a cluster number */
	long vfs_crbomask;		/* and a file offset with this
					 *  mask to get cluster rel offset */
	long vfs_bpcluster;		/* bytes per cluster (blocksize) */
	long vfs_depclust;		/* directory entries per cluster */
	long vfs_fmod;			/* ~0 if fs is modified, this can
					 * rollover to 0		*/
	char vfs_ronly;			/* read only if non-zero	*/
	char vfs_waitonfat;		/* wait for writes of the fat to complt,
					 * when 0 use bdwrite, else bwrite */
	fspin_t vfs_mutex;		/* fast spin lock */
	sleep_t vfs_rename_lock;		/* sleep lock for dir rename */
	unsigned char *vfs_inusemap;	/* ptr to bitmap of in-use clusters */

};

typedef struct dosfs_vfs dosfs_vfs_t;

#define DOSFS_VFS(vfsp) ((struct dosfs_vfs *)((vfsp)->vfs_data))

/*
 *  How to compute vfs_cnshift and vfs_crbomask.
 *
 *  vfs_crbomask = (vfs_SectPerClust * vfs_BytesPerSect) - 1
 *  if (bytesperclust == 0) return EBADBLKSZ;
 *  bit = 1;
 *  for (i = 0; i < 32; i++) {
 *    if (bit & bytesperclust) {
 *      if (bit ^ bytesperclust) return EBADBLKSZ;
 *      vfs_cnshift = i;
 *      break;
 *    }
 *    bit <<= 1;
 * }
 */

/*
 *  Shorthand for fields in the bpb contained in
 *  the dosfsmount structure.
 */
#define	vfs_BytesPerSec	vfs_bpb.bpbBytesPerSec
#define	vfs_SectPerClust	vfs_bpb.bpbSecPerClust
#define	vfs_ResSectors	vfs_bpb.bpbResSectors
#define	vfs_FATs	vfs_bpb.bpbFATs
#define	vfs_RootDirEnts	vfs_bpb.bpbRootDirEnts
#define	vfs_Sectors	vfs_bpb.bpbSectors
#define	vfs_Media	vfs_bpb.bpbMedia
#define	vfs_FATsecs	vfs_bpb.bpbFATsecs
#define	vfs_SecPerTrack	vfs_bpb.bpbSecPerTrack
#define	vfs_Heads	vfs_bpb.bpbHeads
#define	vfs_HiddenSects	vfs_bpb.bpbHiddenSecs
#define	vfs_HugeSectors	vfs_bpb.bpbHugeSectors

/*
 *  Map a cluster number into a filesystem relative
 *  block number.
 */
#define	cntobn(pmp, cn) \
	((((cn)-CLUST_FIRST) * (pmp)->vfs_SectPerClust) + (pmp)->vfs_firstcluster)

/*
 *  Map a filesystem relative block number back into
 *  a cluster number.
 */
#define	bntocn(pmp, bn) \
	((((bn) - pmp->vfs_firstcluster)/ (pmp)->vfs_SectPerClust) + CLUST_FIRST)

#endif /* _KERNEL */

#if defined(__cplusplus)
        }
#endif

#endif /* _FS_DOSFS_DOSFS_FILSYS_H */

