#ident	"@(#)kern-pdi:io/layer/mpio/mpio_os.c	1.2.8.4"

/*
 *  This module contains the Multi Path Disk I/O driver OS dependent stuff.
 */

#define _DDI 8

#include <util/types.h>
#include <fs/buf.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi_layer.h>
#include <util/ksynch.h>

#include <proc/proc.h>
#include <util/param.h>
#include <svc/errno.h>

#include "mpio_os.h"
#include "mpio_core.h"
#include "mpio.h"
#include "mpio_proto.h"
#include "mpio_debug.h"

#include <io/ddi.h>

LKINFO_DECL(mpio_info_lkinfo,"IO:mpio: driver info", 0);
LKINFO_DECL(mpio_vdev_lkinfo,"IO:mpio:vdev lock",0);
LKINFO_DECL(mpio_vdev_sleep_lkinfo,"IO:mpio:vdev sleep lock",0);

lwp_t   *mpio_butler_lwp = 0;    
boolean_t    mpio_stop_threads = B_FALSE;
int	    mpio_thread_count = 0;

/*
 * mpio_signature_and_state_get(dev_t dev, signature_p_t sigp,
 *				path_state_p_t statep,
 * 
 * Description
 *    Issue command to the kernel to obtain device signature and state
 *    as to whether this path is connected to an active or inactive
 *    port if the device is a multi-ported one.
 *
 * Calling/Exit State:
 * 
 *    OK	Signature is return in the buffer.
 *    FAILURE    Could not obtained signature.
 *
 * Note:
 *    On entrance, the device must be opened, ready for processing
 *    ioctl/icmd.
 *
 *    We can only deal with single or dual-ported device. Need to modify
 *    to handle higher port count which today (1996) we don't know about.
 */
status_t
mpio_signature_and_state_get(sdi_device_t *rdevp, const sdi_signature_t **sigp)
{
    status_t	    status;
    int		    i;
    int		    rval;
    rm_key_t	    key;

    MPIO_DEBUG0(0,"enter mpio_get_device_signature");


	/*
	 * Insure that the pointer is cleared by the caller, otherwise we may be
	 * leaking memory.
	 */
    ASSERT(*sigp == NULL);

    /*
     * Issue an ioctl or icmd to get the port signature. If the
     * the device does not understand the ioctl opcode, it must be
     * a single-ported device in which case the resmgr should know
     * the answer.
     */

    status = MPIO_OS_ICMD(rdevp, SDI_GET_SIGNATURE, sigp);

	if (status == OK) {
		if (((*sigp)->sig_state == SDI_PATH_ACTIVE)
				|| ((*sigp)->sig_state == SDI_PATH_INACTIVE)) {
			status = OK;
		} else {
			status = FAILURE;
			err_log0(MPIO_MSG_MPDRV_FAIL_SIG_GET);
		}
	} else {
		status = FAILURE;
		err_log0(MPIO_MSG_MPDRV_FAIL_SIG_GET);
	}

    return status;
}
/* END mpio_signature_and_state_get */

/*
 * void
 * mpio_signal_start_blk_failure(os_io_op_record_p_t bp)
 *
 * Calling/Exit State: 
 *    The MPIO has aborted this request before it started.
 *
 * Description:
 *    This routine performs necessary error code setting and initiates
 *    the appropriate kernel completion callback.
 */
void
mpio_signal_start_blkio_failure(os_io_op_record_p_t bp)
{
    bioerror(bp, EIO);
    biodone(bp);
}

/******************************************************************
/* Accounting implementation section.
/*
/******************************************************************/

/*
 * mpio_acct_create()
 *  
 * Description:
 *    Create an accounting object that will be used for SAR.
 *  
 */
void
mpio_acct_create(os_acct_data_p_t acct)
{
    acct->num_reads = 0;
    acct->num_writes = 0;
    return;
}
/* END mpio_acct_create */

/*
 * mpio_acct_delete()
 *  
 * Description:
 *    Delte the accounting object that is used for SAR.
 *  
 */
void
mpio_acct_delete(os_acct_data_p_t acct)
{
    return;
}
/* END mpio_acct_delete */

/*
 * mpio_acct_start()
 *  
 * Description:
 *    Start accounting measurement.
 */
void
mpio_acct_start(os_acct_data_p_t acct, os_io_op_record_p_t bp)
{
    acct->num_reads = 0;
    acct->num_writes = 0;
    return;
}
/* END mpio_acct_start */

/*
 * mpio_acct_stop()
 *  
 * Description:
 *    Back out measurement because the I/O was failed.
 */
void
mpio_acct_stop(os_acct_data_p_t acct, os_io_op_record_p_t bp)
{
    return;
}
/* END mpio_acct_stop */

/*
 * mpio_acct_update()
 *  
 * Description:
 *    Stop measurment and add new transaction.
 *  
 * Note
 */
void
mpio_acct_update(os_acct_data_p_t acct, os_io_op_record_p_t bp)
{
    if (bp->b_flags & B_READ) 
	acct->num_reads++;
    else
	acct->num_writes++;

    return;
}
/* END mpio_acct_update */

/* end mpio_os.c */
