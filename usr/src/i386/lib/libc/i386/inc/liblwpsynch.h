#if !defined(_LIBLWPSYNCH_H)
#define _LIBLWPSYNCH_H
#ident	"@(#)libc-i386:inc/liblwpsynch.h	1.1"
/*
 * Private header file for LWP synchronization primitives.
 */
#include "synonyms.h"
#include <lwpsynch.h>
#include <sys/usync.h>
#include <errno.h>

#if defined(LWPSYNC_DEBUG)
#define STATIC
#define PRINTF3(f, a1, a2, a3) \
	printf(f" %s:%d\n", a1, a2, a3, __FILE__, __LINE__)
#define PRINTF4(f, a1, a2, a3, a4) \
	printf(f" %s:%d\n", a1, a2, a3, a4, __FILE__, __LINE__)
#define PRINTF5(f, a1, a2, a3, a4, a5) \
	printf(f" %s:%d\n", a1, a2, a3, a4, a5, __FILE__, __LINE__)
#define PRINTF6(f, a1, a2, a3, a4, a5, a6) \
	printf(f" %s:%d\n", a1, a2, a3, a4, a5, a6, __FILE__, __LINE__)
#else
#define STATIC static
#define PRINTF3(f, a1, a2, a3)
#define PRINTF4(f, a1, a2, a3, a4)
#define PRINTF5(f, a1, a2, a3, a4, a5)
#define PRINTF6(f, a1, a2, a3, a4, a5, a6)
#endif /* LWPSYNC_DEBUG */
#endif /* _LIBLWPSYNCH_H */
