#ifndef _IO_TARGET_SDI_SDI_HIER_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SDI_SDI_HIER_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/sdi/sdi_hier.h	1.4.4.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#define	VTOC_HIER_BASE  	 0
#define	MPIO_HIER_BASE  	 5
#define	TARGET_HIER_BASE	10
#define	SDI_HIER_BASE		15
#define	HBA_HIER_BASE		20

#define	ITIMEOUT(f, a, t, p) itimeout(f, a, t, p)

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_SDI_SDI_HIER_H */
