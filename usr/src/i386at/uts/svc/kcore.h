#ifndef _SVC_KCORE_H	/* wrapper symbol for kernel use */
#define _SVC_KCORE_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:svc/kcore.h	1.3.3.1"
#ident	"$Header$"

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define KCORMAG		0x6f636572	/*  "core" */
#define KCORVER		2		/* v2 has kl1pt, pae, symtab */

/*	
 * Kernel Core Dump Header
 */

typedef struct kcore {
	ulong_t	k_magic;	/* magic number: KCORMAG */
	ulong_t	k_version;	/* kcore file version number: KCORVER */
	time_t	k_dtime;	/* time of day when dump was taken */
	size_t	k_align;	/* byte alignment of subsequent structures */
	size_t  k_size;		/* size of the dump */
	/*
	 * below are extra fields when k_version == 2
	 */
	ulong_t	k_pae_enabled;  /* is physical address extension enabled */
	paddr64_t	k_kl1pt; /* physical address of l1 page table */
	off64_t	k_symtab_off; 	/* offset of symbol table within the dump */
} kcore_t;

/*
 * Following the kcore header, at the next file offset multiple of k_align,
 * is an mregion chunk header (mreg_chunk_t), which contains an array of
 * mregion descriptors (mregion_t) that comprise a directory of memory areas
 * that are either written or not written to the dump file.
 *
 * Areas are not written because they represent gaps in the memory
 * space (e.g memory space holes), because they are zeroed memory,
 * or because they are not mapped in kernel space and sysdump_selective
 * is true.
 *
 * The first mregion descriptor corresponds to physical address 0.
 * Subsequent mregions are physically contiguous.
 *
 * The mregion descriptor contains the length, in bytes, of the mregion,
 * and its type.  Unused mregion descriptors (which will always be at the
 * end of the list) will have a length of zero.
 *
 * Following the mregion chunk, at the next file offset multiple of k_align,
 * is the image data for all mregions of type MREG_IMAGE, stored contiguously
 * in the file.
 *
 * Immediately following the image data, at the next file offset multiple
 * of k_align, will be another mregion chunk (even if there are no more
 * mregions).  The file will always end with an mregion chunk that has a
 * zero-length mregion for its first (and every) mregion; this allows for
 * detection of incomplete dumps.
 */

typedef ulong_t mregion_t;	/* mregion descriptor */

#define MREG_LENGTH_SHIFT	4
#define MREG_LENGTH_MASK	(~0x0FUL)	/* extract region length,
						   in bytes */
#define MREG_TYPE_MASK		0x03		/* extract region type */

/* macros to access mregion descriptors: */
#define MREG_LENGTH(mreg)	(((mreg) & MREG_LENGTH_MASK) >> MREG_LENGTH_SHIFT)

#define MREG_TYPE(mreg)	((mreg) & MREG_TYPE_MASK)
#define MREG_MAX_LENGTH	((int)((1 << ((NBPW * NBBY) - MREG_LENGTH_SHIFT)) - 1))

/* mregion types: */
#define MREG_IMAGE	0	/* image data for the region follows */
#define MREG_ZERO	1	/* memory in the region was all zero */
#define MREG_HOLE	2	/* there was no memory in this region */
#define MREG_USER       3       /* image at this region was user data 
				   and was not dumped */


#define MRMAGIC		0x13315555

typedef union mreg_chunk {
	struct {
		ulong_t	  _mc_magic;	/* magic number: MRMAGIC */
		mregion_t _mc_mreg[1];	/* array of mregion descriptors */
	} _mcs;
	char _mc_space[512];
} mreg_chunk_t;

#define mc_magic	_mcs._mc_magic
#define mc_mreg		_mcs._mc_mreg

#define NMREG	((sizeof(mreg_chunk_t) - sizeof(ulong_t)) / sizeof(mregion_t))
#endif /* _UTIL_KCORE_H */

