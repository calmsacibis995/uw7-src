#ifndef _IO_TARGET_SDI_DYNSTRUCTS_H /* wrapper symbol for kernel use */
#define _IO_TARGET_SDI_DYNSTRUCTS_H  /* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/sdi/dynstructs.h	1.7.3.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

typedef	struct	jpool {
	struct jpool	*j_ff;	/* free chain, forward */
	struct jpool	*j_fb;	/* free chain, back */
	struct jdata	*j_data;/* pointer to the pool data */
} jpool_t ;

struct head {
	short		f_isize;	/* size of each job struct */
};

#ifdef __STDC__
extern struct jpool	*sdi_get(struct head *, int);
extern void 		sdi_free(struct head *, struct jpool *);
#else
extern struct jpool	*sdi_get();
extern void 		sdi_free();
#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_SDI_DYNSTRUCTS_H */
