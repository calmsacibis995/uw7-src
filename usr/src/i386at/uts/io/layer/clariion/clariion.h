#ident	"@(#)kern-pdi:io/layer/clariion/clariion.h	1.1.2.4"
/*    Copyright (c) 1996 Data General Corp. All Rights Reserved.    */

#ifndef _IO_LAYER_CLARIION_CLARIION_H    /* wrapper symbol for kernel use */
#define _IO_LAYER_CLARIION_CLARIION_H    /* subject to change without notice */

/*
 *    This module contains the CLARiiON Multi-ported Driver
 *    defines and declarations.
 */

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include    <io/autoconf/resmgr/resmgr.h>
#include    <io/target/sdi/sdi_hier.h>
#include    <io/target/sdi_comm.h>        /* sdi_flag definition */
#include    <io/target/scsi.h>            /* SCSI inquiry struct definition */
#include    <util/ksynch.h>
#include    <svc/errno.h>
#include    "clariion_qm.h"

#endif /* _KERNEL_HEADERS */

#define    DRVNAME            "CLARiiON MP driver"

extern struct dev_cfg CLARIION_dev_cfg[];
extern int CLARIION_dev_cfg_size;

/*
 *  Error message indices 
 */
#define    CLARIION_MSG_DRV_INIT_FAILURE                        1
#define    CLARIION_MSG_DEV_REGIS_FAILURE                       2
#define    CLARIION_MSG_CONFIG_UNKNOWN_EVENT                    3
#define    CLARIION_MSG_START                                   4
#define    CLARIION_MSG_STOP                                    5
#define    CLARIION_MSG_NOT_CLARIION                            6

#define MP_PORT_SIGNATURE_LEN	64

typedef struct clariion_sig {
	uint	clsig_size;
	uint	clsig_state;
	uchar_t clsig_upper[MP_PORT_SIGNATURE_LEN];
	uchar_t clsig_lower[MP_PORT_SIGNATURE_LEN];
	uint	clsig_lun;
} clariion_sig_t;

/*
 * The Path - The object associates with a physical I/O path.
 */
typedef struct {
    clariion_qm_link_t      links;			/* to path queue              */
	int                     open_count;     /*                            */
    sdi_device_t*   	    mydevicep;      /* my sdi_device_t            */
    sdi_device_t*   	    devicep;        /* the device below us        */
    union {
		clariion_sig_t	clr_clariion_sig;
		sdi_signature_t	clr_sdi_sig;
    } clr_sig;
} clariion_path_t, path_t, *path_p_t;

/*
 * The spin lock object.
 */
typedef struct {
    lock_t     	       *p;              /* pointer to Unixware lock */
    pl_t                old_ipl;        /* interrupt priority level */
} SPIN_LOCK_T;

/*
 *  Spin lock parameters suitable for CLARIION.
 */
#define    CLARIION_IPL                 pldisk
#define    CLARIION_HIER                (MPIO_HIER_BASE)

/*
 *  Spin lock primitives.
 */
#define CLARIION_LOCK_ALLOC(lock, lk_info_p)\
    (lock)->p = LOCK_ALLOC(CLARIION_HIER, CLARIION_IPL, lk_info_p, KM_NOSLEEP);

#define CLARIION_LOCK_DEALLOC(lock)    LOCK_DEALLOC((lock)->p)
#define SPIN_LOCK(lock)\
    (lock)->old_ipl = LOCK((lock)->p,CLARIION_IPL)
#define SPIN_UNLOCK(lock)              UNLOCK((lock)->p, (lock)->old_ipl)

/*
 * Driver lock for admin activity that touchs the gut of the driver.
 */
#define    CLARIION_LOCK()                SPIN_LOCK(&clariion_dip->lock);
#define    CLARIION_UNLOCK()              SPIN_UNLOCK(&clariion_dip->lock);

/*
 *  The dip - driver info object that knows everything about the CLARIION
 */
typedef struct {
    SPIN_LOCK_T             lock;             /* regulate access to dip   */

    sdi_layer_t *           my_layer;
    sdi_driver_desc_t *     desc;
} clariion_driver_info_t, * clariion_driver_info_p_t;

/*
 * Initializing driver information block (dip)
 */
#define    CLARIION_INIT_DRIVER_INFO(dip) \
    CLARIION_LOCK_ALLOC(&dip->lock, &clariion_info_lkinfo);    

/*
 * De-initialize driver information block.
 */
#define    CLARIION_DEINIT_DRIVER_INFO(dip) \
    CLARIION_LOCK_DEALLOC(&dip->lock);

/*
 *  Methods to invoke its block device and raw device interface.
 */
#define clariion_kernel_devctl(rdevp, cmd, arg)\
    (rdevp->sdv_drvops->d_drvctl)(rdevp->sdv_idatap, 0, cmd, arg)

#define err_log0(ar0)\
{\
    clariion_msg_p_t    msg_p;\
    msg_p = &clariion_dictionary_p[ar0];\
    cmn_err(msg_p->level, msg_p->msg);\
}

#define err_log1(ar0, ar1)\
{\
    clariion_msg_p_t    msg_p;\
    msg_p = &clariion_dictionary_p[ar0];\
    cmn_err(msg_p->level, msg_p->msg,ar1);\
}

#define err_log2(ar0, ar1, ar2)\
{\
    clariion_msg_p_t    msg_p;\
    msg_p = &clariion_dictionary_p[ar0];\
    cmn_err(msg_p->level, msg_p->msg,ar1,ar2);\
}

#define err_log3(ar0, ar1, ar2, ar3)\
{\
    clariion_msg_p_t    msg_p;\
    msg_p = &clariion_dictionary_p[ar0];\
    cmn_err(msg_p->level, msg_p->msg,ar1,ar2,ar3);\
}

/*
 * The hacked new icmd pass-thru command packet. This packet
 * should be defined in /io/target/icmd.h perhaps.
 */
typedef struct target_pass_thru {
    int           version;
    uint          dcb_p;                   /* device control block   */
    uint          length;                  /* dcb size               */
    char *        buffer;                  /* caller data buffer     */
    uint          size;                    /* buffer size            */
    ushort        mode;                    /* 0 in, 1 out            */
} target_pass_thru_t, * target_pass_thru_p_t;

/*
 * The SAUNA inquiry data
 */
struct sauna_inq {
    struct ident    std_inq;
    char            vendor_specific[20];
};

#define MP_PORT_SIGNATURE_LEN			64
/*
 * Per port multi-ported information packet. This data structure
 * is used as part of the MP_IOCTL_MULTIPORT_INFO command
 */
typedef struct multi_port_info_packet {
	uchar_t		signature_valid;	/* set if 'signature' field is valid	*/
	uchar_t		active;				/* set if this port is active			*/
	char		signature [MP_PORT_SIGNATURE_LEN];

} mp_info_t, * mp_info_p_t;

/*
 * Multi-ported (MP) definitions - clariion specific
 */

#define	MAX_NUMBER_OF_PORTS		0x2	/* max # of ports MP device can have	*/

/*
 * One of the arguments of the MP ioctl(2)
 */
typedef struct get_multi_port_info_packet {
	uint        lun;				/* the lun number of this port			*/
	uint		number_of_ports;	/* 1 if single port, 2 if dual-ported	*/
	mp_info_t 	info[MAX_NUMBER_OF_PORTS];

} get_mp_info_t, * get_mp_info_p_t;

/*
 * The SCSI mode sense CDB.
 */
struct mode_sense_cdb {
    uchar_t        opcode;
    int            res1     : 3;
    int            DBD      : 1;
    int            rsvd     : 1;
    int            lun      : 3;
    int            pagecode : 6;
    int            pc       : 2;
    int            res2     : 8;
    uchar_t        len;
    uchar_t        cont;
};

/*
 *  This structure defines a Mode Sense/Select parameter header.
 *
 *  ("mode_data_len") Not used, must be zero.
 *
 *  ("medium_type") Medium type, indicates the device type.
 *
 *  ("device_specific_param") Not used, must be zero.
 *
 *  ("block_desc_len") Number of bytes in block descriptor.
 */
typedef struct {
    uchar_t        mode_data_len;
    uchar_t        medium_type;
    uchar_t        device_specific_param;
    uchar_t        block_desc_len;

}  clariion_scsi_mode_param_header_t;

/*
 *  Mode sense/select block descriptor for a direct access device.
 *
 *  ("density") Defines the density of the media on the addressed
 *  logical unit.
 *
 *  ("num_logical_blks") Specifies the total number of logical blocks
 *  that are addressable by the user. This field applies only to mode
 *  sense and has no meaning for mode select.
 *
 *  ("reserved") Not used, must be set to zero.
 *
 *  ("logical_blk_len") Specifies the length of the logical block
 *  in bytes. The block length must be equal to, or an exact
 *  multiple of the physical sector size.
 */
typedef struct {
    uchar_t        density;
    uchar_t        num_logical_blks[3];
    uchar_t        reserved;
    uchar_t        logical_blk_len[3];

} clariion_scsi_mode_blk_descriptor_t;

/*
 * This structure defines the page header that precedes each
 * mode select/sense page.
 *
 * ("res_page_code") This byte has two fields: 
 *   1. Bits 6-7 ("Res") Reserved, not used, must be set to zero.
 *   2. Bits 0-5 ("PageCode") Page code of mode page to retrieve.
 *
 * ("page_length") The length of the page in bytes.
 */
typedef struct {

    uchar_t            res_page_code;
    uchar_t            page_len;

} clariion_scsi_mode_buffer_page_header_t ;

/*
 * CLARiiON mode page 0x2B - Unit Config Page. 
 * See CLARiiON Disk-Array SCSI Interface for details.
 */
typedef struct {
    uchar_t        reserved1[2];
    uchar_t        RES_OWN_RES_BCV_AT_AA_WCE_RCE;
    uchar_t        reserved2;
    uchar_t        idle_threshold;
    uchar_t        idle_delay_time;
    uchar_t        write_aside_size[2];
    uchar_t        default_ownership;
    uchar_t        max_rebuild_time;
    uchar_t        prefetch_type_retention_SRCP;
    uchar_t        segment_size_mult;
    uchar_t        prefetch_size_mult[2];
    uchar_t        max_prefetch[2];
    uchar_t        prefetch_disable[2];
    uchar_t        prefetch_idle_count[2];
    uchar_t        reserved3[10];

} clariion_unit_config_page_t, * clariion_unit_config_page_p_t;

/*
 * Bit vector of the OWN field.
 */
#define CLARIION_UNIT_CONFIG_PAGE_OWN_BV            6,1

/*
 * CLARiiON mode page 0x25 - Peer SP Page. See CLARiiON Disk-Array SCSI
 * Interface for details.
 */
typedef struct
    {
    uchar_t        target_id;
    uchar_t        peer_id;
    uchar_t        signature[4];
    uchar_t        peer_signature[4];
    uchar_t        cabinet_id;
    uchar_t        controller_id;
    uchar_t        reserved[2];
} clariion_peer_sp_page_t, * clariion_peer_sp_page_p_t;

/*
 * Value of peer_id when no peer SP exists.
 */
#define CLARIION_NO_PEER_SP_PEER_ID                0xFF
 
typedef struct {
    clariion_scsi_mode_param_header_t           header;
    clariion_scsi_mode_blk_descriptor_t         blk_descriptor;
    clariion_scsi_mode_buffer_page_header_t     page_header;
    clariion_unit_config_page_t		            page;
} clariion_scsi_unit_config_t;

typedef struct {
    clariion_scsi_mode_param_header_t           header;
    clariion_scsi_mode_blk_descriptor_t         blk_descriptor;
    clariion_scsi_mode_buffer_page_header_t     page_header;
    clariion_peer_sp_page_t			            page;
} clariion_scsi_peer_sp_t;

/*
 * Definitions of structures for clariion tresspass.
 */
typedef struct {
    clariion_scsi_mode_buffer_page_header_t     header;
    uchar_t                                     trespass_code;
    uchar_t                                     lun;
} clariion_scsi_mode_trespass_page_t;

typedef struct {
    clariion_scsi_mode_param_header_t            header;
    clariion_scsi_mode_blk_descriptor_t          blk_descriptor;
    clariion_scsi_mode_trespass_page_t           page;
   
} clariion_scsi_mode_trespass_t;

/*
 * Masks off the least significant len bits.
 */
#define CLARIION_MASK(len) (0xff >> 8-len)

/*
 * Get from the byte beginning at the least significant bit position loc,
 * len number of bits and store them in the byte at value_ptr.
 */
#define CLARIION_LOC(loc,len) loc

/*
 * For a comma separated pair of values return the first member.
 */
#define CLARIION_LEN(loc,len) len

/*
 * Get from the byte beginning at the least significant bit position loc,
 * len number of bits and store them in the byte at value_ptr.
 */
#define CLARIION_GET(ptr,loc,len,value_ptr) *value_ptr = ( *ptr &    \
        (CLARIION_MASK(len) << loc) ) >> loc

/* 
 * This macro is designed to get the value from a field of a
 * SCSI-specific structure and copy that value to a local variable.
 * SCSI structures are defined according to the SCSI specification,
 * i.e., they are groups of consecutive bytes with occasional "fields"
 * which are subsets of a byte.  Command Descriptor Blocks and Sense
 * Pages are examples of such structures.
 * Although the bit-vector could be supplied to CLARIION_GETBITS
 * in the x,y form, it is normally a #defined value whose name is specific
 * to a SCSI structure and some field within it.
 */
#define CLARIION_GETBITS(ptr,field,value_ptr) \
	CLARIION_GET(ptr,CLARIION_LOC(field), CLARIION_LEN(field),value_ptr)

#if defined(__cplusplus)
    }

#endif

#endif
