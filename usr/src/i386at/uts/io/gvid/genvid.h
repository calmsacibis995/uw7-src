#ifndef	_IO_GVID_GENVID_H	/* wrapper symbol for kernel use */
#define	_IO_GVID_GENVID_H	/* subject to change without notice */

#ident	"@(#)genvid.h	1.6"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#elif defined(_KERNEL) 

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


typedef struct gvid {
	unsigned long	gvid_num;
	dev_t		*gvid_buf;
	major_t		gvid_maj;
} gvid_t;


#define	GVID_SET	1
#define	GVID_ACCESS	2


#define	GVIOC		('G'<<8|'v')
#define	GVID_SETTABLE	((GVIOC << 16)|1)
#define	GVID_GETTABLE	((GVIOC << 16)|2)


#if defined(__cplusplus)
	}
#endif

#endif /* _IO_GVID_GENVID_H */
