#ifndef _FS_XXFS_XXMACROS_H	/* wrapper symbol for kernel use */
#define _FS_XXFS_XXMACROS_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/xxfs/xxmacros.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#define blkoff(fs, loc)		((loc) & ~(fs)->fs_bmask)
#define blkrounddown(fs, loc)	((loc) & (fs)->fs_bmask)
#define blkroundup(fs, size)	(((size) + (fs)->fs_bsize - 1) & (fs)->fs_bmask)
#define lblkno(fs, loc)		((loc) >> (fs)->fs_bshift)

#define FsLTOP(fs, b)	((b) << (fs)->fs_ltop)
#define FsPTOL(fs, b)	((b) >> (fs)->fs_ltop)
#define FsINOS(fs, x)	((((x) >> (fs)->fs_inoshift) << (fs)->fs_inoshift) + 1)
#define FsITOD(fs, x)	\
	(daddr_t)(((unsigned)(x)+(2*(fs)->fs_inopb-1)) >> (fs)->fs_inoshift)
#define FsITOO(fs, x)	\
	(daddr_t)(((unsigned)(x)+(2*(fs)->fs_inopb-1)) & ((fs)->fs_inopb-1))
#define LTOPBLK(blkno, bsize)	(blkno * ((bsize>>SCTRSHFT)))
#define NINDIR(fs)		((fs)->fs_nindir)

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_XXFS_XXMACROS_H */
