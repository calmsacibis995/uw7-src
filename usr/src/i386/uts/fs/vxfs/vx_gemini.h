/* @(#)src/uw/gemini/kernel/base/vx_gemini.h	3.11.9.3 12/01/97 20:30:57 -  */
/* #ident "@(#)vxfs:src/uw/gemini/kernel/base/vx_gemini.h	3.11.9.3" */
#ident	"@(#)kern-i386:fs/vxfs/vx_gemini.h	1.1.4.1"
/*
 * Copyright (c) 1997 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
 * UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
 * LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
 * IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
 * OR DISCLOSURE.
 *
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 * TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
 * OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
 * EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
 *
 *		RESTRICTED RIGHTS LEGEND
 * USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
 * SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
 * (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
 * COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
 *		VERITAS SOFTWARE
 * 1600 PLYMOUTH STREET, MOUNTAIN VIEW, CA 94043
 */

#ifndef	_FS_VXFS_VX_GEMINI_H
#define	_FS_VXFS_VX_GEMINI_H

/*
 * Miscellaneous definitions specific to UnixWare Gemini port.
 */

/*
 * Setup offset type so we can support large files.
 */

#define	_VXFS_BIG_OFFSETS	12345678
#define	_VXFS_LITTLE_OFFSETS	1234
#define	_VXFS_OFFSET_TYPE	_VXFS_BIG_OFFSETS

/*
 * Define basic data types.
 */

typedef unsigned char		uint8_t;	/* 8 bit unsigned int */
typedef unsigned long long	vx_u64_t;	/* 64 bit arithmetic (if any) */
typedef long long		vx_s64_t;	/* 64 bit signed math  */
typedef unsigned long long	vxoff_t;

/*
 * With big offsets, version 3 file systems are limited to file
 * sizes of 2^31 device blocks (sectors), to avoid overflowing the
 * dbd field in the vm code (limit is 2^28 pages).
 *
 * The file system size is limited to 2^31 - 1 blocks because
 * daddr_t is signed.
 */

#define	VX_MAX_OFF		(((vxoff_t)1 << (31 + VX_MINBSHIFT)) - 1)
#define	VX_MAX_BLKOFF(fs)	(vxoff_t)((VX_MAX_OFF + 1) >> (fs)->fs_bshift)
#define VX_MAX_BLOCKS(fs)	(long)VX_MAXFS_BLOCKS(fs)

#define	VX_MAXFS_SECTOFF	(daddr_t)(((vxoff_t)1 << 31) - 1)
#define	VX_MAXFS_BLKOFF(fs)	(vxoff_t)(VX_MAXFS_SECTOFF >> \
					  ((fs)->fs_bshift - DEV_BSHIFT))
#define VX_MAXFS_BLOCKS(fs)	(vxoff_t)(VX_MAXFS_BLKOFF(fs))

/*
 * The maximum file offset supported by the operating system.
 */

#define	VX_SYS_MAX_OFF(uiop, ioflag)	\
		(vxoff_t)(((ioflag) & IO_LARGEFILE) ? OFF64_MAX : OFF32_MAX)

/*
 * Define for page offset
 */

#define	VX_PGOFF(pp)			page_getoffset(pp)

/*
 * Define page create interfaces for Gemini
 */

#define VX_PAGE_LAZY_CREATE(seg, vp, off) \
	page_lazy_create((seg), (vp), (off));
#define VX_PAGE_LOOKUP_OR_CREATE(seg, vp, off) \
	page_lookup_or_create((seg), (vp), (off), 0);

/*
 * The maximum number of inodes supported on version 3 file systems
 * and beyond.  Version 1 file systems are limited by their geometry
 * and version 2 by VX_MAX_SMALLOFF.
 *
 * This limit of 2^30 inode is somewhat arbitrary and based on the
 * scalability of our algorithms.  The real limit is probably close
 * to (2^32 - fs_bsize).
 */

#define	VX_MAX_NINODE	(1024 * 1024 * 1024)

/*
 * Set up definitions for accessing an 8 byte data type.
 */

typedef struct vx_hyper {
	union vh_hyper {
		unsigned char		vh_bytes[8];
		unsigned long		vh_words[2];
		unsigned long long	vh_64bits;
	} vh_hyper;
} vxhyper_t;

typedef vxhyper_t	vx_blkcnt64_t;	/* 64 bit count of blocks */

#define	vh_low		vh_hyper.vh_words[0]
#define	vh_high		vh_hyper.vh_words[1]
#define	vh_highbyte	vh_hyper.vh_bytes[7]

#define	vh_value	vh_hyper.vh_64bits

#define VX_HYPER_LOWMASK	0x00ffffffffffffff
#define VX_HYPER_HIGHMASK	0xff00000000000000
#define VX_HYPER_GETOFF(hy)	((hy).vh_hyper.vh_64bits & VX_HYPER_LOWMASK)
#define VX_HYPER_GETTYP(hy)	((hy).vh_highbyte)
#define VX_HYPER_SETTYP(hy, val)					\
	((hy).vh_highbyte = (val))
#define VX_HYPER_SETOFF(hy, val)					\
	(hy).vh_hyper.vh_64bits = (((hy).vh_hyper.vh_64bits &		\
				     VX_HYPER_HIGHMASK) |		\
				    ((unsigned long long)(val) &	\
				     VX_HYPER_LOWMASK))

/*
 * Determine if two hypers are equal.
 */

#define VX_HYPER_EQ(h1, h2)	((h1)->vh_value == (h2)->vh_value)

/*
 * Determine if a hyper is zero.
 */

#define VX_HYPER_ZERO(h)	((h).vh_value == (unsigned long long)0)

/*
 * Non-zero iff h1 < h2.
 */

#define VX_HYPER_LT(h1, h2)	((h1)->vh_value < (h2)->vh_value)

/*
 * Non-zero iff h1 <= h2.
 */

#define VX_HYPER_LTE(h1, h2)	((h1)->vh_value <= (h2)->vh_value)

/*
 * Math operations on hypers.
 */

#define VX_HYPER_ADD(h1, h2, i)	(h1)->vh_value = (h2)->vh_value + (i)
#define VX_HYPER_SUB(h1, h2, i)	(h1)->vh_value = (h2)->vh_value - (i)
#define VX_HYPER_INC(h)		(++(h)->vh_value)
#define VX_HYPER_DEC(h)		(--(h)->vh_value)

/*
 * Convert a hyper to a disk address type.
 */

#define VX_HYPER_TO_DADDR(fs, h, d)					\
	((d) = (h).vh_value >> (fs)->fs_bshift)

/*
 * Check for pageout or flushing thread.
 */

#define	VX_PAGEPROC	(IS_PAGEOUT() ||		\
			 u.u_lwpp == vx_flush_lwp ||	\
			 IS_FSFLUSH_PAGESYNC())

/*
 * Set up buffer header.
 */

#define VX_SET_CHANNEL(bp, vp)	spec_tagbp_vp((buf_t *)(bp), vp)

/*
 * Call device strategy in vx_dio_physio().
 */

#define VX_PHYSIO_STRAT(nuiop, bflag, fsdev, fsdevvp) {			\
	buf_t	*bp;							\
	bp = uiobuf(NULL, (nuiop));					\
	bp->b_flags |= (bflag);						\
	bp->b_blkno = btodt((nuiop)->uio_offset);			\
	bp->b_blkoff = (nuiop)->uio_offset % NBPSCTR;			\
	bp->b_edev = (fsdev);						\
	VX_SET_CHANNEL(bp, (fsdevvp));					\
	VX_DO_STRATEGY(bp);						\
	err = biowait(bp);						\
	freerbuf(bp);							\
}

/*
 * Call device strategy for buffer.
 */

#define VX_DO_STRATEGY(bp)	do_strategy(bp)

/*
 * Stubs for device checking that isn't done on Gemini.
 */

#define VX_DEV_SANE(bvp)		(1)
#define VX_DEV_TAPE(dev)		(0)

#endif	/* _FS_VXFS_VX_GEMINI_H */
