#ifndef _IO_TARGET_SD01_SD01_IOCTL_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SD01_SD01_IOCTL_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/sd01/sd01_ioctl.h	1.4.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#define SD_CHAR		('D' << 8)
#define	SD_ELEV		(SD_CHAR | 0x1)		/* Elevator Algorithm */
#define	SD_PDLOC	(SD_CHAR | 0x2)		/* Absolute PD sector */
#define	SD_GETSTAMP	(SD_CHAR | 0x3)		/* return stamp of device */
#define	SD_NEWSTAMP	(SD_CHAR | 0x4)		/* return new stamp for device */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_SD01_SD01_IOCTL_H */
