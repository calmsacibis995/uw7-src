#ifndef _IO_TARGET_SDI_SDI_LAYER_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SDI_SDI_LAYER_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/sdi/sdi_layer.h	1.1.15.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/conf.h>
#include <util/types.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/target/sdi/sdi_defs.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/conf.h>
#include <sys/types.h>
#include <sys/confmgr.h>
#include <sys/sdi_defs.h>

#endif /* _KERNEL_HEADERS */

/*
 *	A set of structs to define the layers above the target drivers
 *	in PDI.
 */

typedef struct dev_cfg attr_t;

#define SDI_DEVICEID_PARAM 	"SDI_DEVICE_ID"
#define SDI_INSTANCE_PARAM 	"SDI_MOD_INSTANCE"

typedef struct sdi_device {
	/*
	 * public fields describing this logical device
	 */
	rm_key_t	sdv_handle;
	rm_key_t	sdv_parent_handle;
	ulong_t 	sdv_state;  /* value supplied by user */
	major_t 	sdv_major;  /* value supplied by user */
	minor_t 	sdv_minor;  /* value supplied by user */
	ulong_t 	sdv_order;  /* value supplied by user */
	ulong_t 	sdv_instance;  /* value supplied by user */
	cgnum_t 	sdv_cgnum;  /* value supplied by SDI */
	void    	*sdv_idatap;	
	/*
	 * public fields describing the supplier of this device
	 */
	struct sdi_layer	*sdv_layer;  /* value supplied by user */
	struct sdi_driver_desc	*sdv_driver; /* value supplied by user */
	struct __drv_ops	*sdv_drvops;
	/*
	 * public fields describing the physical device
	 */
	struct pd_stamp	sdv_stamp;		/* value supplied by user */
	ulong_t 	sdv_unit;
	ulong_t 	sdv_bus;
	ulong_t 	sdv_target;
	ulong_t 	sdv_lun;
	char    	sdv_inquiry[INQ_EXLEN];
	uchar_t 	sdv_devtype;
	bcb_t   	*sdv_bcbp;
	/*
	 * private fields for use by the supplier of this device
	 */
	void    	*sdv_gp;
	struct	sdi_device	*sdv_supplier_next;
	/*
	 * private fields for use by SDI driver
	 */
	struct	sdi_device	*sdv_next;
	int 	sdv_version;
	uint_t	sdv_flags;
	/*
	 * private field describing the consumer of this device
	 */
	struct sdi_driver_desc	*sdv_consumer;
	/*
	 * private fields for use by SDI work queue
	 */
	struct	sdi_device	*sdv_drvadd_next;
	struct	sdi_device	*sdv_work_queue_next;
	uint_t	sdv_work_queue_operation;
} sdi_device_t;

/*
 * values for sdv_state
 */

#define	SDI_DEVICE_EMPTY  	0x00000000	/* device is not initialized */
#define	SDI_DEVICE_EXISTS 	0x00000001	/* device is available */
#define	SDI_DEVICE_ONLINE 	0x00000002	/* device is on-line */
#define	SDI_DEVICE_OFFLINE	0x00000004	/* device is off-line */
#define	SDI_DEVICE_FAULT  	0x00000008	/* device has a fault */
#define	SDI_DEVICE_RCVR   	0x00000010	/* device is in error recovery */

/*
 * values for sdv_version
 */

#define SDI_SDV_VERSION1	1
#define SDI_SDV_VERSION 	SDI_SDV_VERSION1

/*
 * values for sdv_flags
 */

#define SDI_SDV_CLAIMED 	0x00000001  /* device has been claimed */
#define SDI_SDV_PREPPED 	0x80000000  /* device structure has been prep'd */

/*
 * values for sdv_work_queue_operation
 */

#define	SDI_WORKQ_DEVICE_ADD  	0x0001
#define	SDI_WORKQ_DEVICE_RM  	0x0002
#define	SDI_WORKQ_DEVICE_MOD  	0x0004
#define	SDI_WORKQ_DRIVER_ADD  	0x0010
#define	SDI_WORKQ_DRIVER_RM  	0x0020
#define	SDI_WORKQ_DRIVER_MOD  	0x0040

#define	SDI_WORKQ_DEVICE_OP  	0x000F
#define	SDI_WORKQ_DRIVER_OP  	0x00F0

typedef struct upcall {
	int	(*up_event)(); 	/* a layer event requires upper layer update */
} upcall_t ;

typedef struct downcall {
	struct bdevsw	*down_block_entries; /* block entry points from below */
	struct cdevsw	*down_char_entries;  /* char entry points from below */
	int		(*down_event)();     /* a layer event requires lower layer action */
} downcall_t ;

#define SDI_SDD_PREPPED 	0x80000000

#define SDI_SDD_VERSION1	1
#define SDI_SDD_VERSION 	SDI_SDD_VERSION1

typedef struct sdi_driver_desc {
	/*
	 * public fields for use by client drivers
	 */
	char	sdd_modname[ CM_MODNAME_MAX ];
	int 	sdd_precedence;
	struct	dev_cfg *sdd_dev_cfg;
	uint_t	sdd_dev_cfg_size;
	int 	(*sdd_config_entry)();
	ulong_t sdd_minors_per;
	/*
	 * private fields for use by SDI driver
	 */
	struct sdi_layer    	*sdd_layer;
	struct sdi_driver_desc	*sdd_next;
	int 	sdd_version;
	uint_t	sdd_flags;
	uint_t	sdd_instance;
} sdi_driver_desc_t;

/*
 * values for sdd_precedence are defined in sdi_defs.h
 */

typedef struct sdi_layer {
	sdi_driver_desc_t	*la_driver_list;/* list of drivers at this level */
	ulong_t 	la_flags;    	/* state of this layer */
	rm_key_t	la_handle;	/* resmgr key for this layer ? */
	upcall_t	*la_upcalls;	/* interesting entry points from above */
	downcall_t	*la_downcalls;	/* interesting entry points from below */
} sdi_layer_t ;

/*
 *	A set of functions to manage layers above the target drivers
 *	in PDI.
 */

extern sdi_driver_desc_t *sdi_driver_desc_alloc(int);
extern void sdi_driver_desc_free(sdi_driver_desc_t *);
extern boolean_t sdi_driver_desc_prep(sdi_driver_desc_t *, int);

extern sdi_layer_t *sdi_driver_add(sdi_driver_desc_t *, int);
extern boolean_t sdi_driver_rm(const sdi_layer_t);

extern sdi_device_t *sdi_device_alloc(int);
extern void sdi_device_free(sdi_device_t *);
extern boolean_t sdi_device_prep(sdi_device_t *, int);

extern boolean_t sdi_device_add(sdi_device_t *, int);
extern boolean_t sdi_device_add_rinit(sdi_device_t *, int);
extern boolean_t sdi_device_rm(rm_key_t, int);
extern boolean_t sdi_device_rm_rinit(rm_key_t, int);
extern boolean_t sdi_device_state(rm_key_t, ulong_t);

extern void sdi_layers_init();
extern sdi_device_t *sdi_addr_lookup(struct scsi_adr *);

extern cgnum_t sdi_get_cgnum(int);

/*
 * structure used to save elements of buf structure between layers
 */

struct sdi_buf_save {
	void *prev_misc;	/* previous bp->b_misc */
	void (*prev_iodone)();  /* previous bp->b_iodone */
	daddr_t blkno;		/* previous blkno */
	uint_t prev_flags;	/* to indicate that we are stacking */
	void *mpio_vdev;		/* mpio dev */
	void *mpio_rdev;		/* mpio dev */
};

extern void sdi_buf_store(buf_t *, daddr_t);
extern void sdi_buf_restore(buf_t *);
extern daddr_t sdi_get_blkno(buf_t *);

/*
 * Structure used to communicate between the vtoc layer and sd01.
 * This information is required by sd01 so that bad block remapping
 * may be enabled.
 * Used in conjunction with SDI_DEVICE_BADBLOCK.
 */
typedef struct sdi_badBlockmsg {
	void *vtoc;
	struct pdinfo *pd;
} sdi_badBlockMsg_t;

/*
 * Structure used to communicate between the vtoc layer and sd01.
 * It is used to retrieve the geometry of the underlying disk.
 * Used in conjunction with SDI_DEVICE_GEOMETRY.
 */
typedef struct sdi_diskparmMsg {
	char dp_type;
	ushort dp_secsiz;
} sdi_diskparmMsg_t;

/*
 * Structure used to communicate between the vtoc layer and sdi.
 * SDI uses this to get the channel number of a particular slice
 */
typedef struct sdi_slice_ck {
	boolean_t 	ssc_exists;
	ulong_t 	ssc_channel;
	ushort_t	ssc_ptag;
} sdi_slice_ck_t;

/*
 *  A set of structures and defines for the SDI_DEVICE_SEND devctl cmd
 *
 *  look in scsi.h for scsi command definitions.
 */

typedef struct sdi_devctl {
	void 	 *sdc_command; 	/* SCSI command */
	uint_t	 sdc_csize;   	/* size of SCSI command in bytes */
	time_t	 sdc_timeout;  	/* timeout for this job in ms */
	void	 *sdc_buffer; 	/* I/O buffer */
	uint_t	 sdc_bsize;   	/* I/O buffer size in bytes */
	ushort_t sdc_mode;   	/* SCB_READ or SCB_WRITE */
} sdi_devctl_t;

/*
 * The generic device signature. 
 * Notice the signature field is variable. The actual size of it is obtained
 * by using the macro sig_getsize.
 */
typedef struct sdi_signature { 
	uint	sig_size;				/* Size of the next field */
	uint    sig_state;			    /* state of the path */
	char	sig_string[sizeof(struct pd_stamp)];	/* This is variable! */
} sdi_signature_t,  * sdi_signature_p_t;

#define sdi_sigGetSize(sig)	((sig)->sig_size - (2 * sizeof(uint)))
#define sdi_sigFree(sig)		kmem_free((sig), (sig)->sig_size); \
							ASSERT((sig) = NULL)

/*
 * Possible states of a path.
 */
typedef enum {
        SDI_PATH_ACTIVE,
        SDI_PATH_INACTIVE,
        SDI_PATH_FAILED
} path_state_t, *path_state_p_t;



#define	SDC_IOC		(('S'<<24)|('D'<<16)|('C'<<8))

#define	SDI_DEVICE_IOCTL_00	((SDC_IOC)|0x00)
#define	SDI_DEVICE_IOCTL_01	((SDC_IOC)|0x01)
#define	SDI_DEVICE_IOCTL_02	((SDC_IOC)|0x02)
#define	SDI_DEVICE_IOCTL_03	((SDC_IOC)|0x03)
#define	SDI_DEVICE_IOCTL_04	((SDC_IOC)|0x04)
#define	SDI_DEVICE_IOCTL_05	((SDC_IOC)|0x05)
#define	SDI_DEVICE_IOCTL_06	((SDC_IOC)|0x06)
#define	SDI_DEVICE_IOCTL_07	((SDC_IOC)|0x07)
#define	SDI_DEVICE_IOCTL_08	((SDC_IOC)|0x08)
#define	SDI_DEVICE_IOCTL_09	((SDC_IOC)|0x09)
#define	SDI_DEVICE_IOCTL_0A	((SDC_IOC)|0x0A)
#define	SDI_DEVICE_IOCTL_0B	((SDC_IOC)|0x0B)
#define	SDI_DEVICE_IOCTL_0C	((SDC_IOC)|0x0C)
#define	SDI_DEVICE_IOCTL_0D	((SDC_IOC)|0x0D)
#define	SDI_DEVICE_IOCTL_0E	((SDC_IOC)|0x0E)
#define	SDI_DEVICE_IOCTL_0F	((SDC_IOC)|0x0F)

#define SDI_DEVICE_SEND		((SDC_IOC)|0x10)    /* send a SCSI command */
#define	SDI_DEVICE_BADBLOCK	((SDC_IOC)|0x11)	/* pass bb info between vtoc and sd01 */
#define	SDI_DEVICE_GMPINFO	((SDC_IOC)|0x12)	/* get multi-path info */
#define	SDI_DEVICE_TRESPASS	((SDC_IOC)|0x13)	/* trespass from one port to another */
#define	SDI_DEVICE_RPAIRPTH	((SDC_IOC)|0x14)	/* repair a path */
#define	SDI_DEVICE_SLICE_CK	((SDC_IOC)|0x15)	/* check the flags of a slice */
#define	SDI_DEVICE_GEOMETRY	((SDC_IOC)|0x16)	/* retrieve the geometry */
#define	SDI_GET_SIGNATURE	((SDC_IOC)|0x17)	/* retrieve the signature */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_SDI_SDI_LAYER_H */
