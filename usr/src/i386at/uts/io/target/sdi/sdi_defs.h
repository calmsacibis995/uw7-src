#ident	"@(#)kern-pdi:io/target/sdi/sdi_defs.h	1.1"
#ident	"$Header$"

#ifndef _IO_TARGET_SDI_SDI_LAYERDEFS_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SDI_SDI_LAYERDEFS_H	/* subject to change without notice */

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * values for sdd_precedence, note that the actual ordering
 * of the layers is set in sdi.cf/Space.c
 */
#define	SDI_STACK_BASE	10	/* base or target level */
#define	SDI_STACK_ALPHA	30
#define	SDI_STACK_BETA	50
#define	SDI_STACK_GAMMA	70
#define	SDI_STACK_DELTA	90
#define	SDI_STACK_VTOC	100	/* highest level, just under traditional devsw */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_SDI_SDI_LAYERDEFS_H */
