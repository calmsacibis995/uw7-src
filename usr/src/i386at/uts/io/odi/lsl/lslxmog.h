#ifndef _IO_ODI_LSL_LSLXMOG_H /* wrapper symbol for kernel use */
#define _IO_ODI_LSL_LSLXMOG_H /* subject to change without notice */

#ident	"@(#)lslxmog.h	2.1"
#ident  "$Header$"

#ifdef  _KERNEL_HEADERS

#include <io/autoconf/resmgr/resmgr.h>	/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/resmgr.h>			/* REQUIRED */

#endif

/*
 * lsxmog.h
 *
 * Description:
 *	This file contains the interface between the ODI transmogrifier
 *	utility (xmog) and the Link Support Layer shim (LSL).
 *
 */

/*
 * lsl_init_hsm must know the version of the xmog that was used.  This allows
 * extensibility in the structs below.
 */
#define LSL_XMOG_VERSION	210	/* ie, UnixWare 2.1 */

#define LSL_XMOG_PRE_INIT	0

struct lslInitHsmParams;		/* forward reference */

typedef struct {
    int		reason;			/* must be 1st element */
    struct lslInitHsmParams * hsm_params; /* init parameters */
    int		bd_num;			/* board number */
    int		card_bus_type;		/* CM_BUS_PCI, etc */
    ulong_t	io_start;		/* start of I/O base address */
    ulong_t	io_end;			/* end of I/O base address */
    paddr_t	mem_start;		/* start of base mem address */
    paddr_t	mem_end;		/* start of base mem address */
    int		irq_level;		/* interrupt request level */
    int		dmac;			/* DMA channel */
    ushort_t	pci_vendor_id;		/* PCI vendor ID */
    ushort_t	pci_device_id;		/* PCI device ID */
    uchar_t	pci_bus_num;		/* (PCI) bus number */
    uchar_t	pci_dev_num;		/* (PCI) device number (5 bits) */
    uchar_t	pci_func_num;		/* (PCI) function number (3 bits) */
    int		slot;
    char *	hsm_cmdline_buf;	/* PreInitFunc can optionally write */
    int		hsm_cmdline_size;	/* cmd line keywords into this buf */
    rm_key_t	key;			/* resmgr key */
    int		num_bds;		/* number of boards */
} LslPreInitCallbackData;

typedef union {
    int	reason;				/* pre-init, post-init, etc. */
    LslPreInitCallbackData	pre_init_data;
} LslInitHsmCallbackData;

typedef int (*LslInitHsmCallback)(LslInitHsmCallbackData *);

typedef struct lslInitHsmParams {
    int			version;	/* version of lsl/xmog */
    char *		hsm_name;
    ulong_t *		hsm_majors;
    int			hsm_num_saps;
    int			hsm_num_multicast;
    char *		hsm_ifname;
    char *		hsm_id_string;
    int			hsm_tsm_type;
    char *		hsm_frmw_name;
    int			hsm_dev_flag;
    int			hsm_load_flag;
    int			hsm_setup_flag;
    int			hsm_misc_flag;
    int			(*hsm_driver_init)();
    LslInitHsmCallback	hsm_init_callback;
    void *		hsm_init_cb_data;
} LslInitHsmParams;

extern int	lsl_init_hsm(LslInitHsmParams *);

#endif /* _IO_ODI_LSL_LSLXMOG_H */
