#ifndef _UTIL_MERGE_MERGE386_H	/* wrapper symbol for kernel use */
#define _UTIL_MERGE_MERGE386_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/merge/merge386.h	1.3.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************

       Copyright (c) 1991 Locus Computing Corporation.
       All rights reserved.
       This is an unpublished work containing CONFIDENTIAL INFORMATION
       that is the property of Locus Computing Corporation.
       Any unauthorized use, duplication or disclosure is prohibited.

***************************************************************************/


/*
**  merge386.h
**		structure definitions for Merge hooks. 
*/

#if defined _KERNEL || defined _KMEMUSER

struct mrg_com_data {
	int (*com_ppi_func)();		/* Pointer to a Merge function  */
					/* to call depending on the VPI */
					/* currently attached to this   */
					/* device. NULL if no VPI is    */
					/* attached.		        */
	unsigned char *com_ppi_data; 	/* Pointer to a structure used	*/
					/* by the above function.	*/
					/* Set/used only by Merge.	*/
	long unused;			/* Reserved for future use	*/
	unsigned long baseport;		/* Base I/O port for this dev.  */
};

#endif /* _KERNEL || _KMEMUSER */

/*
 * New MKI for UnixWare 7 (SVR5)
 */
#ifdef _KERNEL_HEADERS
#include <svc/mki.h>
#elif defined(_KERNEL) || defined(_KMEMUSER)
#include <sys/mki.h>
#endif

#if defined _KERNEL || defined _KMEMUSER
#ifndef	_MKI_C			/* only defined in mki.c */
#define com_ppi_strioctl(a, b, c, d)	mhi_com_ppi_ioctl((a), (b), (c), (d))
#define portalloc(a, b)			mhi_port_alloc((a), (b))
#define portfree(a, b)			mhi_port_free((a), (b))
#define kdppi_ioctl(a, b, c, d)		mhi_kd_ppi_ioctl((a), (b), (c), (d))
#endif
#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_MERGE_MERGE386_H */
