#ifndef _IO_POSTWAIT_H    /* wrapper symbol for kernel use */
#define _IO_POSTWAIT_H    /* subject to change without notice */

#ident	"@(#)kern-i386:io/postwait/postwait.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#define PWIOC		('P'<<8)
#define PWIOC_POST	(PWIOC|1)
#define PWIOC_WAIT	(PWIOC|2)

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_POSTWAIT_H */
