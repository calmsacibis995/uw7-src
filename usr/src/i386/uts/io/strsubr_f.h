#ifndef _IO_STRSUBR_F_H	/* wrapper symbol for kernel use */
#define _IO_STRSUBR_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:io/strsubr_f.h	1.10"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * WARNING:
 * Everything in this file is private, belonging to the
 * STREAMS subsystem.  The only guarantee made about the
 * contents of this file is that if you include it, your
 * code will not port to the next release.
 *
 * This is all of the family specific stuff; in this case, mostly
 * support for SCO applications.
 */

#ifdef _KERNEL_HEADERS

#include <svc/sco.h>		/* REQUIRED */
#include <net/timod.h>		/* REQUIRED */
#include <proc/exec.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/sco.h>		/* REQUIRED */
#include <sys/timod.h>		/* REQUIRED */
#include <sys/exec.h>		/* REQUIRED */

#else

#include <sys/sco.h>		/* REQUIRED */
#include <sys/timod.h>		/* REQUIRED */
#include <sys/exec.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

extern int strmodpushed(stdata_t *, char *);

#define STRACCMAP(cmd)	(((cmd == SCO_XCGETA) || \
			 (cmd == (('K'<<8)|62))) ? JCGETP : JCSETP)

#define STRIOCMAP(stp, ioc)	if (IS_SCOEXEC && !SCO_USES_SHNSL && \
				    (ioc.ic_cmd & ~0xff) == ('T'<<8)) { \
					unsigned int low_byte; \
					low_byte = ioc.ic_cmd & 0xff; \
					if ((low_byte >= 100) && (low_byte <= 103) && \
					    (strmodpushed(stp, "timod") == 0)) \
						ioc.ic_cmd += 40; \
				}

#define STRACKMAP(stp, iop, ip)	if (IS_SCOEXEC && !SCO_USES_SHNSL && \
				    (iop->ic_cmd == TI_GETINFO) && \
				    (strmodpushed(stp, "timod") == 0)) \
					ip->ioc_count -= sizeof(long);

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_STRSUBR_F_H */
