#ifndef _IO_MKDEV_H	/* wrapper symbol for kernel use */
#define _IO_MKDEV_H	/* subject to change without notice */

#ident	"@(#)kern-i386:io/mkdev.h	1.6"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * User-level interfaces for dev_t construction and examination.
 */

#if !defined(_KERNEL)

/*
 * In case sysmacros.h was #Included, undefine the macro versions
 * of these interfaces which were defined there.
 */

#undef makedev
#undef major
#undef minor

#if defined(__STDC__)

dev_t makedev(major_t, minor_t);
major_t major(dev_t);
minor_t minor(dev_t);
dev_t __makedev(int, major_t, minor_t);
major_t __major(int, dev_t);
minor_t __minor(int, dev_t);

#else

dev_t makedev();
major_t major();
minor_t minor();
dev_t __makedev();
major_t __major();
minor_t __minor();

#endif	/* defined(_STDC_) */

#define OLDDEV 0	/* old device format */
#define NEWDEV 1	/* new device format */

#ifdef _EFTSAFE
#define makedev(maj, min)	__makedev(NEWDEV, maj, min)
#else
static dev_t
#if defined(__cplusplus) || defined(__STDC__)
makedev(major_t maj, minor_t min)
#else
makedev(maj, min)
	major_t maj;
	minor_t min;
#endif
{
#if !defined(_STYPES)
	int ver = NEWDEV;
#else
	int ver = OLDDEV;
#endif
	return __makedev(ver, maj, min);
}
#endif	/* defined _EFTSAFE */

#ifdef _EFTSAFE
#define major(dev)	__major(NEWDEV, dev)
#else
static major_t 
#if defined(__cplusplus) || defined(__STDC__)
major(dev_t dev)
#else
major(dev)
	dev_t dev;
#endif
{
#if !defined(_STYPES)
	int ver = NEWDEV;
#else
	int ver = OLDDEV;
#endif
	return __major(ver, dev);
}
#endif	/* defined _EFTSAFE */

#ifdef _EFTSAFE
#define minor(dev)	__minor(NEWDEV, dev)
#else
static minor_t 
#if defined(__cplusplus) || defined(__STDC__)
minor(dev_t dev)
#else
minor(dev)
	dev_t dev;
#endif
{
#if !defined(_STYPES)
	int ver = NEWDEV;
#else
	int ver = OLDDEV;
#endif
	return __minor(ver, dev);
}
#endif	/* defined _EFTSAFE */

/*
 * The following symbols must be kept in sync with their counterparts
 * in sysmacros_f.h.  These are for use by the libc implentations of
 * makedev(), major(), and minor(), and should not be used directly by
 * applications.
 */

#define ONBITSMAJOR	7	/* # of SVR3 major device bits */
#define ONBITSMINOR	8	/* # of SVR3 minor device bits */
#define OMAXMAJ		0x7f	/* SVR3 max major value */
#define OMAXMIN		0xff	/* SVR3 max major value */

#define NBITSMAJOR	14	/* Current # of major device bits */
#define NBITSMINOR	18	/* Current # of minor device bits */
#define MAXMAJ		0x1fff	/* Current maximum major number;
				 * the high bit is reserved
				 */
#define MAXMIN		0x3ffff	/* Current maximum minor number */

#endif /* !defined(_KERNEL) */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_MKDEV_H */
