#ident	"@(#)kern-pdi:io/layer/mpio/mpio.h	1.1.3.2"

#ifndef _IO_MPIO_MPIO_H    /* wrapper symbol for kernel use */
#define _IO_MPIO_MPIO_H    /* subject to change without notice */

/*
 *    This module contains the MPIO wrapper for UnixWare defines.
 */

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/autoconf/resmgr/resmgr.h>
#include <io/conf.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <fs/buf.h>
#include <io/target/sdi/sdi_comm.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi_layer.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#endif /* _KERNEL_HEADERS */

#define    DRVNAME	    "MPIOdriver"

/*
 *  Error message codes
 */
#define    MPIO_MSG_NOMEM                           1
#define    MPIO_MSG_PATH_FAILURE                    2
#define    MPIO_MSG_AUTO_REPAIR                     3
#define    MPIO_MSG_FAIL_VDEV                       4
#define    MPIO_MSG_REPAIR_PATH_FAILURE             5
#define    MPIO_MSG_TRESPASS_PATH                   6
#define    MPIO_MSG_TRESPASS_PATH_FAILURE           7
#define    MPIO_MSG_EXCESSIVE_TRESPASS              8
#define    MPIO_MSG_DEREG_UNKNOWN_PATH              9
#define    MPIO_MSG_DEV_REGIS_FAILURE               10
#define    MPIO_MSG_MPDRV_FAIL_SIG_GET              11
#define    MPIO_MSG_DRV_INIT_FAILURE                12
#define    MPIO_MSG_CONFIG_UNKNOWN_EVENT            13

extern struct dev_cfg MPIO_dev_cfg[];
extern int MPIO_dev_cfg_size;

#if defined(__cplusplus)
    }

#endif

#endif /* _IO_MPIO_MPIO_H */
