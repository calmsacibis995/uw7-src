#ifndef _SVC_ISC_H	/* wrapper symbol for kernel use */
#define _SVC_ISC_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/isc.h	1.5"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/* Enhanced Application Compatibility Support */

#define ISC_SIGCONT	23
#define ISC_SIGSTOP	24
#define ISC_SIGTSTP	25

/* POSIX waitpid() ISC Defines */
#define ISC_WNOHANG	1
#define ISC_WUNTRACED	2

/* POSIX TIOC  ISC Defines */
#define ISC_TIOC	('T' << 8)
#define ISC_TCSETPGRP	(ISC_TIOC | 20)
#define ISC_TCGETPGRP	(ISC_TIOC | 21)

/* End Enhanced Application Compatibility Support */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_ISC_H */
