#ifndef _SVC_PROCTL_H	/* wrapper symbol for kernel use */
#define _SVC_PROCTL_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/proctl.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * XENIX proctl() requests. Both of these functions are currently
 * implemented as no-ops.
 */

#define PRHUGEX		1	/* allow process > swapper size to execute */
#define PRNORMEX 	2	/* remove PRHUGEX permission */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_PROCTL_H */
