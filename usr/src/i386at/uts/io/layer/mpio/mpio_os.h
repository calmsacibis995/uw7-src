#ifndef _IO_MPIO_MPIO_OS_H    /* wrapper symbol for kernel use */
#define _IO_MPIO_MPIO_OS_H    /* subject to change without notice */

#ident	"@(#)kern-pdi:io/layer/mpio/mpio_os.h	1.2.10.5"

/*
 *  This module contains the MPIO driver OS dependent stuff.
 */

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include    <util/types.h>
#include    <mem/kmem.h>        /* Included for DDI kmem_alloc routines */
#include    <io/conf.h>		/* for devflag and bdevsw		*/
#include    <util/cmn_err.h>     /* Included for PRINTF routines	 	*/
#include    <io/open.h>		/* open flags			        */
#include    <proc/cred.h>       /* Included for open etc.	        */
#include    <util/ksynch.h>
#include    <svc/errno.h>
#include    <util/debug.h>       /* for ASSERT			        */
#include    <io/target/sd01/sd01_ioctl.h>
#include    <io/target/sdi/sdi_comm.h>
#include    <io/target/sdi/sdi_edt.h>
#include    <io/target/sdi/sdi_layer.h>
#include    <io/autoconf/resmgr/resmgr.h>
#include    <util/sysmacros.h>
#include    <io/conf.h>
#include    <io/autoconf/resmgr/resmgr.h>
#include    <fs/buf.h>
#include    <io/target/sdi/sdi_comm.h>
#include    <io/target/sdi/sdi_edt.h>
#include    <io/target/sdi/sdi_layer.h>
#include    <io/target/scsi.h>
#include    <io/target/sdi/sdi_hier.h>

#endif /* _KERNEL_HEADERS */

/*
 * Tunable parameter defined in Space.c
 * Defines system wide redundant path recovery policy.
 */
extern int mpio_inactive_first;

/*
 * The spin lock object.
 */
typedef struct {
    lock_t *	    p;	        /* pointer to Unixware lock */
    pl_t	    old_ipl;    /* interrupt priority level */
} VSPIN_LOCK_T;

/*
 *  OS-specific for accounting and statistic.
 */
typedef struct {
    int			   num_reads;
    int			   num_writes;
} os_acct_data_t, * os_acct_data_p_t;

/*
 * Locks
 */
extern lkinfo_t  mpio_iores_lkinfo;
extern lkinfo_t  mpio_vdev_lkinfo;
extern lkinfo_t  mpio_vdev_sleep_lkinfo;
extern lkinfo_t  mpio_info_lkinfo;

/*
 * This OS wallclock time object.
 */
typedef time_t os_time_t;

/*
 *  This OS spin lock parameters suitable for MPIO.
 */
#define    MPIO_IPL		pldisk
#define    MPIO_PRIO		pridisk
#define    MPIO_HIER		(MPIO_HIER_BASE)

/*
 *  This OS spin lock primitives.
 */
#define VSPIN_LOCK_ALLOC(hier, lock, lk_info_p)\
    (lock)->p = LOCK_ALLOC(MPIO_HIER+(hier), MPIO_IPL, lk_info_p, KM_NOSLEEP);

#define VSPIN_LOCK_DEALLOC(lock) LOCK_DEALLOC((lock)->p)
#define VSPIN_LOCK(lock)	 (lock)->old_ipl = LOCK((lock)->p,pldisk)
#define VSPIN_UNLOCK(lock)	 UNLOCK((lock)->p, (lock)->old_ipl)

/*
 * This OS synchronous variable primitives.
 */
typedef    sv_t VSV_T;
#define VSV_ALLOC(flag)	        SV_ALLOC(flag /* KM_SLEEP/KM_NOSLEEP */ )
#define VSV_DEALLOC(svp)	SV_DEALLOC(svp)
#define VSV_WAIT(svp,lock)	SV_WAIT(svp,pridisk,(lock)->p)
#define VSV_SIGNAL(svp)	        SV_BROADCAST(svp,0)

/*
 *  This OS block device I/O operation record block.
 */
typedef struct buf    os_io_op_record_t, *os_io_op_record_p_t;

/*
 *  This OS methods to invoke its block device and raw device interface.
 */
#define MPIO_COMPARE_DEVICE(one,two)	(one->sdv_handle == two->sdv_handle)

#define MPIO_MAKE_HANDLE(rdevp)	(rdevp->sdv_handle)

#define MPIO_KERNEL_OPEN8(rdevp, channelp, flags, credp, dummy_arg)\
    (rdevp->sdv_drvops->d_open)(rdevp->sdv_idatap, channelp, flags, credp, \
                                dummy_arg)

#define MPIO_KERNEL_CLOSE8(rdevp, flags, credp, dummy_arg)\
    (rdevp->sdv_drvops->d_close)(rdevp->sdv_idatap, 0, flags, credp, dummy_arg)

#define MPIO_KERNEL_IOCTL8(rdevp, cmd, arg, mode, cred_p, rval_p)\
    (rdevp->sdv_drvops->d_ioctl)(rdevp->sdv_idatap, 0, cmd, arg, mode, cred_p, rval_p)

#define MPIO_KERNEL_DEVCTL8(rdevp, cmd, arg)\
    (rdevp->sdv_drvops->d_drvctl)(rdevp->sdv_idatap, 0, cmd, arg)

#define MPIO_KERNEL_DEVINFO8(rdevp, cmd, arg)\
    (rdevp->sdv_drvops->d_devinfo)(rdevp->sdv_idatap, 0, cmd, arg)

#define MPIO_KERNEL_START_BLKIO8(bp, rdevp)\
    (rdevp->sdv_drvops->d_biostart)(rdevp->sdv_idatap, 0, bp)

/*
 * This OS method for upcalling its block device I/O completion
 */
#define mpio_complete_upcall(bp)\
    {\
    MPIO_DRIVER_INFO_CHECK_OUT();\
    biodone(bp);\
    }

/*
 *  MPIO_GET_LOCALE_COUNT stub for Gemini.
 */
#define MPIO_GET_LOCALE_COUNT()		MAX_LOCALES_PER_SYSTEM

/*
 *  MPIO_GET_DEVICE_LOCALE() stub for Gemini
 */
#define MPIO_GET_PATH_LOCALE(rdev)     0x0

/*
 * Construct a CG number from an OS I/O operation record.
 * (stub for Gemini)
 */
#define    MPIO_GET_BUFFER_CGNUM(bp)		0

/*
 * Defines this OS kernel memory functions.
 */
#define    VKMEM_ALLOC(x)	    kmem_alloc(x, sdi_sleepflag)
#define    VKMEM_FREE		    kmem_free
#define    VNUMA_KMEM_ALLOC(a,b)    kmem_zalloc(a, sdi_sleepflag)

#define VCOPYIN		        copyin	/* bring user mem to kernel    */
#define VCOPYOUT		copyout	/* bring kernel mem to user    */

#define VMEMCOPY(x,y,len)	bcopy(x,y,len)
#define VCOPY_FAILURE	        (-1)	/* operation failure code    */

#define    VKM_NOSLEEP		KM_NOSLEEP
#define    VKM_SLEEP		KM_SLEEP
#define VKMEM_PAGE_SIZE	        0x1000	/* 4k page size */

/*
 * Device type define.
 */
#define    MPIO_TYPE_DISK    ID_RANDOM	/* in scsi.h  */

/*
 * The I/O request queue is used when MPIO runs out of io 
 * resource. For UnixWare, the buf_t has the av_form and av_back 
 * elements that are compatible with our mpio_qm. We use them for links.
 */
#define MPIO_INIT_PEND_IOREQ_QUEUE(q)\
    mpio_qm_initialize(q,offsetof(struct buf, av_forw));

/*
 * Get and set the error code from this OS I/O record block.
 */
#define OS_IO_OP_STATUS_GET(bp)		geterror(bp)
#define OS_IO_OP_STATUS_SET(bp,errno)	bioerror(bp,errno)

/*
 * This OS unrecoverable I/O error codes
 */
#define    OS_EIO_ERROR					EIO
#define    OS_EIO_NO_PATH					EIO

/*
 * Device driver error codes - Need to put it into target drivers
 */
#define    MPIO_EIO_NO_ERROR				    0x000
#define    MPIO_EIO_PHYSICAL_UNIT_FAILURE		    0x001
#define    MPIO_EIO_DEVICE_TIMED_OUT			    0x002
#define    MPIO_EIO_DEVICE_SELECT_TIMED_OUT		    0x003
#define    MPIO_EBUSY_UNIT_ASSIGNED_TO_ANOTHER_CONTROLLER   0x004

/*
 * OS standard error code
 */
#define    MPIO_KMEM_ALLOC_FAILURE    ENOMEM    /* kernel mem alloc	      */
#define    MPIO_COPYIN_FAILURE	EFAULT          /* user mem to kernel mem     */
 
/*
 * MPIO specific error codes - needed to mapped to OS error codes.
 */
#define MPIO_NO_PENDING_IO_REQUESTS	    0x0001
#define MPIO_NO_FREE_INTERNAL_OP_RECORDS    0x0002
#define MPIO_PHYS_UNIT_FAILURE		    0x0003
#define MPIO_INVAL_IOCTL_PKT		    0x0003
#define MPIO_UNKNOWN_VDEV		    0x0003
#define MPIO_INVAL_PATH			    0x0003
#define MPIO_INVAL_PATH_SWITCH_STATES	    0x0003
/*
 * Either the state of the source path or the destination path 
 * of the path switch ioctl() packet is not in the expected state.
 * The source path must be in ACTIVE and the destination must be
 * in INACTIVE state.
 */

/*
 * OS standard internal ioctl interface
 */
#define MPIO_OS_ICMD(rdevp, opcode, arg)\
	MPIO_KERNEL_DEVCTL8(rdevp, opcode, arg)

#define MPIO_TRESPASS(rdevp)\
    MPIO_OS_ICMD(rdevp, SDI_DEVICE_TRESPASS, NULL)

#define MPIO_REPAIR_PATH(rdevp)\
    MPIO_OS_ICMD(rdevp, SD_PATH_REPAIRED, NULL)


/*
 * OS wallclock time in second
 */
#define    MPIO_GET_WALL_CLOCK(now)	drv_getparm(TIME, now);

/*
 * Macro to compute offset into a member of a structure.
 */
#define MEMBER_OFFSET    offsetof

/*
 * This OS error logging 
 */
#define    ERR_LOG	cmn_err

#if defined(__cplusplus)
    }

#endif

#endif /* _IO_MPIO_MPIO_OS_H */
