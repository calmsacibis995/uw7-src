#ifndef _IO_TARGET_SDI_SDI_HANDLE_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SDI_SDI_HANDLE_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/sdi/sdi_handle.h	1.1.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
	SDI_ROOTDEV,	/* return device handle for root filesystem */
	SDI_DUMPDEV,	/* return device handle for dump device */
	SDI_SWAPDEV,	/* return device handle for swap device */
	SDI_STANDDEV,	/* return device handle for stand device */
} sdi_hcmd_t;

extern dev_t sdi_return_handle(sdi_hcmd_t);

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_SDI_SDI_HANDLE_H */
