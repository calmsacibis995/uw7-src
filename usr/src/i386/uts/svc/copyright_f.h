#ifndef _SVC_COPYRIGHT_F_H	/* wrapper symbol for kernel use */
#define _SVC_COPYRIGHT_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/copyright_f.h	1.8"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Strings for title and copyright messages
 * printed during system initialization.
 */

#ifdef _KERNEL_HEADERS

#include <svc/copyright_p.h> /* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

#define COPYRIGHT_FAMILY	/* none */

#if 0

#define SYS_TITLE \
	"UNIX(r) System V Release %r Version %v for the Intel386(tm) Family"

#else				/* UnixWare licensee */

#define SYS_TITLE \
	"UnixWare %v for the Intel386(tm) Family"

#endif

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_COPYRIGHT_F_H */
