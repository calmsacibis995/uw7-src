#ifndef _UTIL_PARAM_H	/* wrapper symbol for kernel use */
#define _UTIL_PARAM_H	/* subject to change without notice */
#define _SYS_PARAM_H	/* SVR4.0COMPAT */

#ident	"@(#)kern-i386:util/param.h	1.40.7.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * Definitions for various kernel constants.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* SVR4.0COMPAT */
#include <util/param_p.h>	/* PORTABILITY */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* SVR4.0COMPAT */
#include <sys/param_p.h>	/* PORTABILITY */

#else

#include <sys/types.h>		/* SVR4.0COMPAT */
#include <sys/param_p.h>	/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#if !defined(_DDI)

/*
 * Fundamental variables; don't change too often.
 */

#ifndef _POSIX_VDISABLE
#define _POSIX_VDISABLE 0 /* Disable special character functions */
#endif

#ifndef MAX_INPUT
#define MAX_INPUT 5120    /* approx. maximum bytes stored in the input queue */
#endif

#ifndef MAX_CANON
#define MAX_CANON 256     /* Maximum bytes in a line for canoical processing */
#endif

#define UID_NOBODY	60001	/* user ID no body */
#define GID_NOBODY	UID_NOBODY

#define UID_NOACCESS	60002	/* user ID no access */


#define MAXPID	30000		/* max process id */
#define MAXUID	60002		/* max user id */

#define NOFILE	20		/* this define is here for	*/
				/* compatibility purposes only	*/
				/* and will be removed in a	*/
				/* later release		*/

/*
 * These define the maximum and minimum allowable values of the
 * configurable parameter NGROUPS_MAX.
 */
#define NGROUPS_UMAX	32
#define NGROUPS_UMIN	0

#define CMASK		0	/* default mask for file creation */
#define CDLIMIT	(1L<<14)	/* default max write address */

#define MAXSUSE	255

#define lobyte(X)	(((unsigned char *)&(X))[0])
#define hibyte(X)	(((unsigned char *)&(X))[1])
#define loword(X)	(((ushort *)&(X))[0])
#define hiword(X)	(((ushort *)&(X))[1])

/*
 * Sleep priority values.  Higher numbers represent higher priorities.
 * XXX - These are tentative values.
 */

#define PRINPRIS	40	/* number of discrete priority slots */
#define PRIMEM 		35	
#define PRINOD		29
#define PRIBUF		20
#define PRIMED		20
#define PRIPIPE		25
#define PRIVFS		23
#define PRIWAIT		10
#define PRIREMOTE	10
#define PRISLEP		5
#define PRIZERO		0
#define PRIDLE		-1

/* REMOTE -- whether machine is primary, secondary, or regular */
#define SYSNAME 9		/* # chars in system name */
#define PREMOTE 39

/*
 * MAXPATHLEN defines the longest permissible path length,
 * including the terminating null, after expanding symbolic links.
 * MAXSYMLINKS defines the maximum number of symbolic links
 * that may be expanded in a path name. It should be set high
 * enough to allow all legitimate uses, but halt infinite loops
 * reasonably quickly.
 * MAXNAMELEN is the length (including the terminating null) of
 * the longest permissible file (component) name.
 */
#define MAXPATHLEN	1024
#define MAXSYMLINKS	20
#define MAXNAMELEN	256

/* LFS Support */
/*
 * the FILESIZEBITS  constant is defined as a Pathname Variable Value
 * to represent the minimum number of bits needed to represent as a
 * signed integer value, the maximum size of a regular file allowed
 * in the specified directory 
 */
#define FILESIZE32BITS	32
#define FILESIZE64BITS	64

#ifndef NADDR
#define NADDR 13
#endif

/*
 * The following are defined to be the same as
 * defined in /usr/include/limits.h.  They are
 * needed for pipe and FIFO compatibility.
 */
#ifndef PIPE_BUF	/* max # bytes atomic in write to a pipe */
#define PIPE_BUF	5120
#endif	/* PIPE_BUF */

#ifndef PIPE_MAX	/* max # bytes written to a pipe in a write */
#define PIPE_MAX	5120
#endif	/* PIPE_MAX */

/*
 * File system parameters and macros.
 *
 * MAXBIOSIZE gives an upper bound on the size of an I/O.
 * 
 * The file system is made out of blocks of at most MAXBSIZE units,
 * with smaller units (fragments) only in the last direct block.
 * MAXBSIZE primarily determines the size of buffers in the buffer
 * pool. It may be made larger without any effect on existing
 * file systems; however making it smaller may make some file
 * systems unmountable.
 *
 * Note that the blocked devices are assumed to have DEV_BSIZE
 * "sectors" and that fragments must be some multiple of this size.
 */
#define MAXBIOSIZE	0x20000

#define MAXBSIZE	8192
#define MAXBSHIFT	13		/* log2(MAXBSIZE) */
#define MAXBOFFSET	(MAXBSIZE - 1)
#define MAXBMASK	(~MAXBOFFSET)

#define DEV_BSIZE	512
#define DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define DEV_BOFFSET	(DEV_BSIZE - 1)
#define DEV_BMASK	(~DEV_BOFFSET)

#ifndef MAXFRAG
#define MAXFRAG 	8
#endif

#define btodb(bytes)	 		/* calculates (bytes / DEV_BSIZE) */ \
	((unsigned)(bytes) >> DEV_BSHIFT)
#define dbtob(db)			/* calculates (db * DEV_BSIZE) */ \
	((unsigned)(db) << DEV_BSHIFT)

#define btodb64(bytes)	 		/* calculates (bytes / DEV_BSIZE) */ \
	((ullong_t)(bytes) >> DEV_BSHIFT)
#define dbtob64(db)			/* calculates (db * DEV_BSIZE) */ \
	((ullong_t)(db) << DEV_BSHIFT)

/*
 * MMU_PAGES* describes the physical page size used by the mapping hardware.
 * PAGES* describes the logical page size used by the system.
 */

#define MMU_PAGESIZE	0x1000		/* 4096 bytes */
#define MMU_PAGESHIFT	12		/* log2(MMU_PAGESIZE) */
#define MMU_PAGEOFFSET	(MMU_PAGESIZE-1)/* Mask of address bits in page */
#define MMU_PAGEMASK	(~MMU_PAGEOFFSET)

#define PAGESIZE	0x1000		/* All of the above, for logical */
#define PAGESHIFT	12		/* log2(PAGESIZE) */
#define PAGEOFFSET	(PAGESIZE - 1)
#define PAGEMASK	(~PAGEOFFSET)

/*
 * Some random macros for units conversion.
 */

/*
 * MMU pages to bytes, and back (with and without rounding)
 */
#define mmu_ptob(x)	((ulong_t)(x) << MMU_PAGESHIFT)
#define mmu_btop(x)	((ulong_t)(x) >> MMU_PAGESHIFT)
#define mmu_btopr(x)	(((ulong_t)(x) + MMU_PAGEOFFSET) >> MMU_PAGESHIFT)

#define mmu_ptob64(x)	((ullong_t)(x) << MMU_PAGESHIFT)
#define mmu_btop64(x)	((ullong_t)(x) >> MMU_PAGESHIFT)
#define mmu_btopr64(x)	(((ullong_t)(x) + MMU_PAGEOFFSET) >> MMU_PAGESHIFT)

/*
 * pages to bytes, and back (with and without rounding)
 */
#define _PTOB(x)	((ulong_t)(x) << PAGESHIFT)
#define ptob(x)		_PTOB(x)
#define _BTOP(x)	((ulong_t)(x) >> PAGESHIFT)
#define btop(x)		_BTOP(x)
#define _BTOPR(x)	(((ulong_t)(x) + PAGEOFFSET) >> PAGESHIFT)
#define btopr(x)	_BTOPR(x)

#define ptob64(x)	((ullong_t)(x) << PAGESHIFT)
#define btop64(x)	((ullong_t)(x) >> PAGESHIFT)
#define btopr64(x)	(((ullong_t)(x) + PAGEOFFSET) >> PAGESHIFT)

#if defined(_KERNEL) || defined(_KMEMUSER)

#define SSIZE	1		/* initial stack size (* PAGESIZE bytes) */
#define SINCR	1		/* increment of stack (* PAGESIZE bytes) */

#define USIZE	1		/* per-LWP ublock size (* PAGESIZE bytes) */
#define UVSIZE	3		/* virtual needed per ublock */
#define UVOFF	2		/* offset of pageable ublock w/in virtual */
#define KSE_PAGES   1	/* size of stack extension (*MMU_PAGESIZE bytes) */

#define KL1PT_PAGES	1			/* # HW pages for KL1PT */
#define KL1PT_BYTES	mmu_ptob(KL1PT_PAGES)	/* # bytes for KL1PT */

#ifdef _KERNEL

#define DELAY(n)	{ register int N = calc_delay(n); \
			  while (--N > 0) continue; }

#endif /* _KERNEL */
 
#undef NBITPOFF
#if defined(_FSKI) && _FSKI == 1
#define NBITPOFF	32	/* number of bits in an off32_t */
#else
#define NBITPOFF	64	/* number of bits in an off64_t */
#endif
#endif /* _KERNEL || _KMEMUSER */

#endif /* !_DDI */

/*
 **************************************************************
 * The remainder of the file is accessible to DDI/DKI drivers.
 * Be careful what you add after this point.
 **************************************************************
 */

#ifdef _KERNEL

/*
 * SLEEP and NOSLEEP are common flags passed to many kernel routines
 * to indicate whether the caller can (SLEEP) or cannot (NOSLEEP)
 * tolerate blocking.
 */
#define SLEEP		0
#define NOSLEEP		1

#endif /* _KERNEL */

/*
 * Fundamental constants of the implementation--cannot be changed easily.
 */

#undef NBBY
#define NBBY		8	/* number of bits per byte */
#undef NBPW
#define NBPW	sizeof(int)	/* number of bytes in an integer */
#undef NBPSCTR
#define NBPSCTR         512     /* Bytes per LOGICAL disk sector. */
#undef SCTRSHFT
#define SCTRSHFT	9	/* Shift for NBPSCTR. */
#undef UBSIZE
#define UBSIZE		512	/* UNIX block size: unit for df/du. */

/* Basic NULL values, if not defined elsewhere */

#ifndef NODEV
#define NODEV	(dev_t)(-1)
#endif

#ifndef NULL
#define NULL	0
#endif

/*
 * DDI8+ NODE CREATION DEFINES.  ONLY TO BE USED BY
 * DDI providers!!!!!
 */
#ifdef _DDI_C

#define	NODE_ACTION	0x80
#define	NODE_CREATE	(0x01 | NODE_ACTION)
#define NODE_DELETE	(0x02 | NODE_ACTION)
#define	NODE_NO_ACTION	0x00
#define	NODE_ACTION_ERROR	-1
#define	NODE_ACTION_SUCCESS	0
#define	NODE_ACTION_REQUEUE	1
#define	NODE_ACTION_IGNORED	2

#endif /* _DDI_C */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_PARAM_H */
