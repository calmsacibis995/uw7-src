#ifndef _IO_TARGET_SDI_SDI_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SDI_SDI_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/target/sdi/sdi.h	1.8.4.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * PDI_VERSION is defined as follows for the versions:
 *      UnixWare 1.1    1
 *      SVR4.2 DTMT     undefined by O/S; use 2
 *      SVR4.2 MP       3
 *      UnixWare 2.0    4
 *      UnixWare 2.1    5
 * 
 * These are defined in both sdi.h and hba.h 
 */
#ifndef PDI_UNIXWARE11
#define PDI_UNIXWARE11  1
#endif
#ifndef PDI_SVR42_DTMT
#define PDI_SVR42_DTMT  2
#endif
#ifndef PDI_SVR42MP
#define PDI_SVR42MP     3
#endif
#ifndef PDI_UNIXWARE20
#define PDI_UNIXWARE20  4
#endif
#ifndef PDI_UNIXWARE21
#define PDI_UNIXWARE21  5
#endif

#define PDI_VERSION	PDI_UNIXWARE21	/* Version number for UnixWare 2.1 */

#ifdef PDI_SVR42
#undef PDI_SVR42
#endif  /* PDI_SVR42 */

#ifdef _KERNEL_HEADERS
#include <io/target/sdi/sdi_comm.h>
#else
#include <sys/sdi_comm.h>
#endif	/* _KERNEL_HEADERS */

#if defined(__cplusplus)
	}
#endif

#endif	/* ! _IO_TARGET_SDI_SDI_H */
