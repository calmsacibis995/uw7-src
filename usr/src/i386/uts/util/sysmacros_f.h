#ifndef _UTIL_SYSMACROS_F_H	/* wrapper symbol for kernel use */
#define _UTIL_SYSMACROS_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/sysmacros_f.h	1.11"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Various family-specific system macros.
 */

#if !defined(_DDI)

/*
 * WARNING: The device number macros defined here should not be used by device
 * drivers or user software.  Device drivers should use the device functions
 * defined in the DDI/DKI interface (see also ddi.h).  Application software
 * should make use of the library routines available in makedev(3).  A set of
 * new device macros are provided to operate on the expanded device number
 * format supported in SVR4.  Macro versions of the DDI device functions are
 * provided for use by kernel proper routines only.
 */

#define O_BITSMAJOR     7       /* # of SVR3 major device bits */
#define O_BITSMINOR     8       /* # of SVR3 minor device bits */
#define O_MAXMAJ        0x7f    /* SVR3 max major value */
#define O_MAXMIN        0xff    /* SVR3 max major value */

#define L_BITSMAJOR     14      /* Current # of major device bits */
#define L_BITSMINOR     18      /* Current # of minor device bits */
#define L_MAXMAJ	0x1fff	/* Current maximum major number;
				 * the high bit is reserved
				 */
#define L_MAXMIN        0x3ffff /* Current maximum minor number */

/*
 * Get internal major and minor device
 * components from expanded device number.
 */
#define _GETMAJOR(dev)	(major_t)(((dev_t)(dev) >> L_BITSMINOR) & L_MAXMAJ)
#define getmajor(dev)	_GETMAJOR(dev)
#define _GETMINOR(dev)	(minor_t)((dev_t)(dev) & L_MAXMIN)
#define getminor(dev)	_GETMINOR(dev)

/*
 * Get external major and minor device 
 * components from expanded device number.
 * For the i386 family, external == internal.
 * These interfaces return NODEV if values are out of range.
 */
#define _GETEMAJOR(dev)	(major_t)(((dev_t)(dev) >> L_BITSMINOR) > L_MAXMAJ ? \
				  NODEV : _GETMAJOR(dev))
#define getemajor(dev)	_GETEMAJOR(dev)
#define _GETEMINOR(dev)	_GETMINOR(dev)
#define geteminor(dev)	_GETEMINOR(dev)

/*
 * Macros for working with old device numbers (o_dev_t).
 */
#define o_getemajor(x)	(major_t)((((o_dev_t)(x) >> O_BITSMINOR) > O_MAXMAJ) ? \
			 NODEV : \
			 (((o_dev_t)(x) >> O_BITSMINOR) & O_MAXMAJ))
#define o_geteminor(x)	(minor_t)((o_dev_t)(x) & O_MAXMIN)

#endif /* !_DDI */

/*
 * Alignment of basic integral data types.
 */

	/*
	**	for i386
	*/
#define	SALIGN(p)		(char *)(((int)p+1) & ~1)
#define	IALIGN(p)		(char *)(((int)p+3) & ~3)
#define	LALIGN(p)		(char *)(((int)p+3) & ~3)

	/*
	**	some others (for ref)
	#ifdef	pdp11
	#define	SALIGN(p)		(char *)(((int)p+1) & ~1)
	#define	IALIGN(p)		(char *)(((int)p+1) & ~1)
	#define LALIGN(p)		(char *)(((int)p+1) & ~3)
	#endif
	#ifdef vax
	#define	SALIGN(p)		(char *)(((int)p+1) & ~1)
	#define	IALIGN(p)		(char *)(((int)p+3) & ~3)
	#define	LALIGN(p)		(char *)(((int)p+3) & ~3)
	#endif
	#ifdef	u3b2
	#define	SALIGN(p)		(char *)(((int)p+1) & ~1)
	#define	IALIGN(p)		(char *)(((int)p+3) & ~3)
	#define	LALIGN(p)		(char *)(((int)p+3) & ~3)
	#endif
	*/

/*
 * Relationship between pages and disk blocks.
 */
#define NDPP		8		/* Number of disk blocks per page */
#define DPPSHFT		3		/* Shift for disk blocks per page. */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_SYSMACROS_F_H */
