#ident	"@(#)kern-pdi:io/layer/mpio/mpio_core.c	1.1.11.4"

/*
 *  This module contains the MPIO Driver core.
 *
 *  On generic architecture, mpio provides High Availability
 *  and load balancing functionalities.
 *
 *  On NUMA architecture, in addition to the above features, mpio will
 *  enhance I/O performance by selecting the closest paths to issue I/O
 *  requests when possible. This is also referred to as fully-connected 
 *  I/O (FCIO).
 *
 *
 *  Functionality:
 *
 *  The MPIO functional organization can be categorized as follow:
 *
 *  Registration:
 *  - register paths
 *  - obtain paths signature.
 *  - identify active/inactive paths
 *  - group paths connected to same disks.
 *  - export disks.
 * 
 *  IO Management:
 *  - Obtain async resource (b_misc)
 *  - queue I/O if no resource avail.
 *  - select the optimal path based on I/O request's locality and 
 *    available paths.
 *  - attach resource to the I/O
 *  - issue I/O to path.
 * 
 *  Completion Management:
 *  - Activate error manager if request failed.
 *  - detach async resource if async I/O
 *  - upcall completion
 *  - start another I/O if one is waiting for an async resource to start.
 * 
 *  Error Management:
 *  - fail the failing path.
 *  - select a new path to reissue the I/O.
 *  - if no active path is available, start trespass process
 * 
 *  Trespass Management:
 *  - quiesce and lock the disk throughout the cluster.
 *  - validate the new path (read its current signature and compare 
 *    with the in-core signature).
 *  - Compute the trespass rate. If rate exceeds limit, fail this path to
 *    prevent pingpong effect between cluster nodes that are simultaneously
 *    trying to recover from a path failure.
 *  - issue tresspass.
 *  - update the state ie. active/inactive of all paths to this disk.
 *  - communicate the trespass to other cluster nodes (not yet implemented)
 *  - unlock the disk.
 *
 *  Design:
 *
 *  The MPIO object model is based on the following hierarchy:
 *  the mpio, Vdev, the Locale Path Group (LPG) and the Path objects.
 *    Misc. objects not shown in the picture are: the Map table, Signature, 
 *    I/O Resource Manager (resmgr), Message queue (mque) etc.
 *
 *	       --------
 *	       |mpio|				Driver answers.
 *	       |      |
 *	     N |driver|
 *    -------- | stuff|
 *    |	       |      |
 *    |	       --------
 *    |
 *    |-----> 1--------			     Virtual device answers.
 *	       | vdev |		
 *	       |      |
 *	       |      |
 *    --------N|lpg_p |
 *    |	       |      |
 *    V 1      --------
 *    ------- ->------- ->------- ->------- ->....  Locale answers.
 *    | lpg |   | lpg |   | lpg |   | lpg |
 *   N|     |   |     |   |     |   |     |
 * ---|active   |active   |active   |active
 * |  |inacti   |inacti   |inacti   |inacti
 * |  |failed   |failed   |failed   |failed
 * |  -------   -------   -------   -------
 * V 1
 *  ------ ------				   Phys path answers.
 *  |path| |path| ...........
 *  |    | |    |
 *  |    | |    |
 *  ------ ------
 *
 *    One mpio object per driver. Each mpio manages N Vdevs.
 *    One Vdev per physical device. Each Vdev knows about N LPGs, 
 *    One Lpg per locale, each Lpg manages 4 groups of paths 
 *     active/inactive/failed. The lpg "knows" the optimal path 
 *     and redundant paths.
 *    One Path per physical channel to the physical device.
 *
 *    The mapping table is an array of vdev pointers. This table is used
 *    to look up the Vdev of an MPIO device referred to by a dev_t. This
 *    table simply assumes that the minor of the MPIO device is [0-n].
 *
 *    The Signature is one per Vdev. This object manage different types of
 *    signature ie. device stamp, WID and multi-ported device signature.
 *
 *    Resmgr object regulates I/O traffic based on the resource availability.
 *    Since we must attach a piece of resource to each I/O before move it
 *    to the next layer of the I/O stack, and this resource is limited, the
 *    resmgr must regulate (queue) I/O requests if necessary.
 */

#define	_DDI	8

#include    <svc/systm.h>

#include    "mpio_core.h"
#include    "mpio_os.h"
#include    "mpio_proto.h"
#include    "mpio.h"
#include    "mpio_debug.h"

#include	<io/ddi.h>

mpio_driver_info_t *    dip;		/* top most data structure    */
mpio_qm_anchor_t	     mpio_vdev_queue;    /* Vdev queue		*/

sdi_device_t *mpio_register_device(sdi_device_t*, vdev_p_t, rm_key_t);

/*
 * mpio implementation section.
 */

/*
 * void
 * mpio_start(void)
 *
 * Description:
 *    Allocate MPIO resources.
 *
 * Calling/Exit State:
 *   The earliest entry point of the MPIO.
 *   
 */
void
mpio_start(void)
{
    int		 key;
    vdev_p_t	    vdevp, firstvdevp;

    MPIO_DEBUG0(0,"Enter mpio_start.");

    /*
     *    Allocate and init driver information block (dip)
     */

    dip = VKMEM_ALLOC(sizeof(mpio_driver_info_t));
    if ( dip == NULL){
	err_log0(MPIO_MSG_NOMEM);
	goto done;
    }
    MPIO_INIT_DRIVER_INFO(dip);
    mpio_qm_initialize(&mpio_vdev_queue,MEMBER_OFFSET(vdev_t,links));

    dip->ready = B_TRUE;

done:
    return;
}
/* END mpio_start */

#ifdef notyet
/*
 * void mpio_stop(void)
 *
 * Description:
 *    Deregister Vdevs, deallocate resources and tear down the recovery LWP.
 *
 * Calling/Exit State:
 *     Called by driver xxxstop function when OS wants to purge MPIO.
 *
 */
void 
mpio_stop(void)
{
    status_t    status;

    /*
     * Deregister devices, deallocate resources and terminate
     * the recovery thread.
     */

    status = mpio_deconfigure_devices();
    if (status == FAILURE){
	goto done;
    }
    MPIO_DEINIT_DRIVER_INFO(dip);
    VKMEM_FREE(dip ,sizeof(mpio_driver_info_t));

done:
    return;
}
/* END mpio_stop */
#endif /* notyet */

/*
 * int
 * mpio_configure_path(rm_key_t rm_key)
 *
 * rdevp:   Pointer to a registered device. This object contains the
 *	    relevant information about a path that the sdi has given us.
 *
 * Description:
 *    Put a new path under MPIO jurisdiction.
 *
 * Calling/Exit State:
 *   
 */
int
mpio_configure_path(void *idata, rm_key_t rm_key)
{
    int			    size;
    const sdi_signature_t	*signature = NULL;
    sig_compare_result_t    result;
    vdev_p_t		    vdevp;
    sdi_device_t            *rdevp;
    int			    status = 0;

    rdevp = VKMEM_ALLOC(sizeof(sdi_device_t));

    *(void **)idata = (void *)rdevp;

    if (rdevp == NULL){
        err_log0(MPIO_MSG_NOMEM);
        status = MPIO_KMEM_ALLOC_FAILURE;
        goto vkmem_alloc_failure;
    }

    if (!sdi_dev_read_opr(rm_key, rdevp)){
        goto bailout;
    }

    /*
     *  Get the path signature and compare it with existing Vdevs' signature.
     */
    if (mpio_signature_and_state_get(rdevp, &signature) != OK ){
		goto bailout;
    }

    MPIO_DRIVER_LOCK();

    vdevp = MPIO_QM_HEAD(&mpio_vdev_queue);
    while ( vdevp != NULL){
	if ((result = mpio_signature_compare(signature, 
				vdevp->signature)) == FSIG_MATCHED){
	    /*
	     *  Fully matched - put the new path under this 
	     *    Vdev jurisdiction.
	     */
	    MPIO_DRIVER_UNLOCK();
	
	    /*
	     * Once the signature is extracted we MUST process the path.
	     * We must not loose the signature state that was just extracted.
	     */
	    vdevp->signature->sig_state = signature->sig_state;
	    mpio_vdev_put(rdevp, vdevp);
	    goto done;
	} else {
	    /*  no match - skip to the next vdev */
	    vdevp = MPIO_QM_NEXT(&mpio_vdev_queue,vdevp);
	}
    }
    /*
     * When we're here, we've found a path which has a fresh new signature.
     * This must be a path of a new device. Create a Vdev and bind it to this
     * Rdev then register the Vdev with the kernel.
     */

    MPIO_DRIVER_UNLOCK();

    if (!(vdevp = mpio_vdev_create())){
	goto bailout;
    }

    vdevp->signature = mpio_signature_copy(signature);
    mpio_vdev_put(rdevp, vdevp);

    MPIO_DRIVER_LOCK();

    dip->num_of_vdevs++;
    mpio_qm_add_tail(&mpio_vdev_queue,vdevp);

    MPIO_DRIVER_UNLOCK();

    vdevp->devicep = mpio_register_device(rdevp, vdevp, rm_key);
    goto done;
    /* NOT REACHED */

bailout:
    status = ENXIO;
    VKMEM_FREE(rdevp, sizeof(sdi_device_t));
    *(void **)idata = (void *)NULL;

vkmem_alloc_failure:
done:
    return status;
}
/* END mpio_configure_path */

#ifdef DEBUG
mpio_print(vdev_t *vdevp)
{
	int i;
	path_t *pathp;
	lpg_t *lpg;
	

	printf("vdevp 0x%x signature 0x%x (opens %d - paths %d)\n", vdevp,
vdevp->signature, vdevp->open_count, vdevp->number_of_paths);

	printf("Path list\n");
        pathp = MPIO_QM_HEAD(&vdevp->paths);
        while (pathp != NULL){
		printf("pathp = 0x%x\t", pathp);
		printf("state = %d\t", pathp->state);
		printf("fail_reason= %d\n", pathp->failed_reason);
		pathp = (path_p_t)MPIO_QM_NEXT(&vdevp->paths,pathp);
	}

	for (i = 0 ; i < MAX_LOCALES_PER_SYSTEM ; i++) {
		int j;

		printf("local%d->home_lpg = 0x%x working_lpg = 0x%x\n",i, vdevp->lpg_vector[i].home_lpg, vdevp->lpg_vector[i].working_lpg);

		if (vdevp->lpg_vector[i].home_lpg == NULL) {
			continue;
		}

		lpg = vdevp->lpg_vector[i].home_lpg;

		printf("active:\t");
		pathp =MPIO_QM_HEAD(&lpg->active_list);
		while (pathp != NULL){
			printf("0x%x\t", pathp);
			pathp = (path_p_t)MPIO_QM_NEXT(&lpg->active_list,pathp);
		}


		printf("inactive:\t");
		pathp=MPIO_QM_HEAD(&lpg->inactive_list);
		while (pathp != NULL){
			printf("0x%x\t", pathp);
			pathp = (path_p_t)MPIO_QM_NEXT(&lpg->inactive_list,pathp);
		}

		printf("failed:\t");
		pathp=MPIO_QM_HEAD(&lpg->failed_list) ;
		while (pathp != NULL){
			printf("0x%x\t", pathp);
			pathp = (path_p_t)MPIO_QM_NEXT(&lpg->failed_list,pathp);
		}
		printf("\n");
	}
}
#endif

/*
 * int
 * mpio_deconfigure_path(void)
 *
 * Description:
 *    Remove a path from MPIO jurisdiction.
 *
 * Calling/Exit State:
 *   
 * Note:
 *     Execute this routine may result in deleting the Vdev, and therefore
 *     the caller must be responsible for quiesce the Vdev to prevent
 *     inconsistency which may result in panic. eg. threads waiting on
 *     Vdev's lock when the lock got deleted together with Vdev deletion.
 * 
 *    This routine assumes the caller has done all necessary actions
 *    to stop I/O and close the path before calling this function. If
 *  the open_count is not zero, the routine will fail.
 * 
 *    If the path is the only working path of the Vdev, deconfigure it
 *    will cause subsequence I/O requests to fail.
 * 
 */
int
mpio_deconfigure_path(void *idatap)
{
    sig_compare_result_t    result;
    path_state_t	    state;
    vdev_p_t		    vdevp;
    path_p_t		    pathp;
    sdi_device_t    	    *newdevice;
    sdi_device_t	    *rdevp;
    int			    status = 0;

    rdevp = (sdi_device_t *)idatap;

    /*
     * Lock driver to single thread deconfiguration
     */
    MPIO_DRIVER_LOCK();

    vdevp = MPIO_QM_HEAD(&mpio_vdev_queue);
    while (vdevp != NULL){
        /*
         * Verify that this path is actually under this Vdev jurisdiction
         * by comparing the new path device handle with ours.
         */
        pathp = MPIO_QM_HEAD(&vdevp->paths);
        while (pathp != NULL){
            if (MPIO_COMPARE_DEVICE(pathp->devicep,rdevp))
                goto found_it;

            pathp = (path_p_t)MPIO_QM_NEXT(&vdevp->paths,pathp);
        }
	vdevp = MPIO_QM_NEXT(&mpio_vdev_queue,vdevp);
    }

    /*
     * There is no Vdev that contains this rdev.  This Rdev is not
     * under MPIO jurisdiction. Punt.
     */
    err_log1(MPIO_MSG_DEREG_UNKNOWN_PATH, MPIO_MAKE_HANDLE(rdevp));
    goto bailout;

    /*
     *    Disassociate the Path from the Vdev and delete the Path.
     */
found_it:
    mpio_vdev_remove_path(pathp, vdevp);

    /*
     * If this is the lone path of the Vdev, delete the Vdev and
     * report device gone to the kernel also.
     */
    if (vdevp->number_of_paths == 0){
	ASSERT(mpio_qm_is_member(&mpio_vdev_queue,vdevp));
	newdevice = vdevp->devicep;
	mpio_qm_remove(&mpio_vdev_queue,vdevp);
	if (!mpio_vdev_delete(vdevp)){
	    mpio_qm_add_head(&mpio_vdev_queue,vdevp);
	    goto bailout;
	}
        MPIO_DRIVER_UNLOCK();
	mpio_deregister_device(newdevice);    
    }
    else {
        MPIO_DRIVER_UNLOCK();
    }
    VKMEM_FREE(rdevp, sizeof(sdi_device_t));
    return 0;

bailout:
    status = ENXIO;
    MPIO_DRIVER_UNLOCK();
    return status;
}
/* END mpio_deconfigure_path */

void
mpiobiostart8(void *idatap, ulong_t channel, os_io_op_record_p_t bp)
{
    vdev_p_t    vdevp;

    MPIO_DRIVER_INFO_CHECK_IN();    /* debug stuff */

    vdevp = (vdev_p_t)idatap;

	ASSERT(vdevp);

	mpio_issue_blkio(vdevp, bp);
}
/* END mpiobiostart8 */

/*
 * void mpio_issue_blkio(vdev_p_t vdevp, os_io_op_record_p_t bp)
 *
 * Description:
 *    Issue an I/O request to this Vdev using the optimal path.
 *
 * Calling/Exit State:
 * 
 * Note:
 *    Vdev lock is obtained inside. 
 *
 *    This is the main I/O path. Performance is everything. Try to 
 *    use the lock as short as possible, before calling to the lower 
 *    layer so that we won't be blackout for longer than necessary.
 */
	
void
mpio_issue_blkio(vdev_p_t vdevp, os_io_op_record_p_t bp)
{
    path_p_t	        pathp;
    lpg_p_t		lpgp;

    /*
     *  Submit this I/O to the I/O resource manager. If there
     *  was a failure, the request cannot be started, but it's
     *  on the pended request queue, so it will eventually get
     *  started when the conditions change.
     *
     *  Get the current optimal path from the LPG.  If there is
     *  no path, then the I/O can't be done, so return an error.
     */
    MPIO_VDEV_LOCK(vdevp);
/*
 *  lpgp = vdevp->lpg_vector[MPIO_GET_BUFFER_CGNUM(bp)].working_lpg;
 *  pathp = mpio_lpg_get_active_path(lpgp);
 */

    pathp = mpio_get_best_path( bp, vdevp );

    MPIO_VDEV_UNLOCK(vdevp);
    if (pathp == NULL){
	/*
	 * Because of the mpio_vdev_put() logic, we may have paths 
	 * available for this LPG, but they aren't in active state, 
	 * so try the recovery once.
	 */
	mpio_vdev_recover_blkio_failure(pathp, vdevp, bp);
	goto done;
    }

    /*
     * Attach the I/O request to the io resource object and 
     * issue the request to the Path.
     */
    sdi_buf_store(bp, sdi_get_blkno(bp));
    ((struct sdi_buf_save *)(bp->b_misc))->mpio_vdev = (void *)vdevp;
    ((struct sdi_buf_save *)(bp->b_misc))->mpio_rdev = (void *)pathp;
    bp->b_iodone = mpio_complete_blkio;
    MPIO_KERNEL_START_BLKIO8(bp, pathp->devicep);
done:
    return;
}
/* END mpio_issue_blkio */

/*
 * void
 * mpio_complete_blkio (int, int)
 *
 * Description:
 *    Complete a block I/O request.
 *
 * Calling/Exit State:
 * 
 */
void
mpio_complete_blkio(os_io_op_record_p_t bp)
{
    vdev_p_t	    vdevp;
    path_p_t	    pathp;

    vdevp = (vdev_p_t)(((struct sdi_buf_save *)(bp->b_misc))->mpio_vdev);
    pathp = (path_p_t)(((struct sdi_buf_save *)(bp->b_misc))->mpio_rdev);

    ASSERT( vdevp != NULL );

    if (OS_IO_OP_STATUS_GET(bp) != MPIO_EIO_NO_ERROR) {
	/*
	 *  We may be able to recover from this failure. Choose another
	 *  path.  We will do this using the recovery thread because
	 *  we don't want to drag this thread which very likely is the
	 *  interrupt thread. In some cases, dragging this thread may
	 *    cause a system panic or dead lock if the recovery involves 
	 *    sleep or lenghthy operations.
	 *
	 *    Send a message to the recovery thread.
	 */
	if (biocanblock(bp)){
	    sdi_buf_restore(bp);
	    mpio_vdev_recover_blkio_failure(pathp, vdevp, bp);
	}
	return;
    }

#ifdef MPIO_DEBUG
    /*
     * Look for signal to tell us to do recovery.
     */
    ASSERT( pathp != NULL );
    if ((OS_IO_OP_STATUS_GET(bp) != MPIO_EIO_NO_ERROR)
		    || pathp->assert_error ){
	if (biocanblock(bp)){
	    sdi_buf_restore(bp);
	    mpio_vdev_recover_blkio_failure(pathp, vdevp, bp);
	}
	return;
    }
#endif

    /*
     * The I/O has completed. Free the io resource then signal a
     * completion to the OS.
     */
    MPIO_VDEV_LOCK(vdevp);
    mpio_acct_update(&pathp->acct, bp);
    MPIO_VDEV_UNLOCK(vdevp);

	sdi_buf_restore(bp);

    mpio_complete_upcall(bp);

    return;
}
/* END mpio_complete_blkio */

#ifdef notyet
/*
 * status_t  mpio_deconfigure_devices(void)
 *
 * Description:
 *    Scan the MPIO queue for Vdevs and remove them from the MPIO
 *    management.
 * 
 * Calling/Exit State:
 *   OK	    All devices are deregistered.
 *   FAILURE    One or more vdev could not be deregistered. Process aborted.
 * 
 */
status_t
mpio_deconfigure_devices()
{
    vdev_p_t		vdevp;
    sdi_device_t        *devicep;

    MPIO_DEBUG0(0,"enter mpio_deconfigure_devices");

    vdevp = MPIO_QM_HEAD(&mpio_vdev_queue);
    while (vdevp != NULL){
	ASSERT(mpio_qm_is_member(&mpio_vdev_queue,vdevp));
	mpio_qm_remove(&mpio_vdev_queue,vdevp);
	devicep = vdevp->devicep;
	if (!mpio_vdev_delete(vdevp)){
	    mpio_qm_add_head(&mpio_vdev_queue, vdevp);
	    return FAILURE;
	}
	mpio_deregister_device(devicep);
	vdevp = MPIO_QM_HEAD(&mpio_vdev_queue);
    }
    return OK;
}
/* END mpio_deconfigure_devices */
#endif /* notyet */

/*
 * status_t
 * mpio_openAll(vdev_t *vdevp)
 *
 * Do an open to the underlying layer with the least restrictions.
 *
 * Assumptions:
 * Some other layer above this one will check that the open is legal.
 */
status_t
mpio_openAll(vdev_t *vdevp)
{
	status_t status = OK;
	ulong_t lower_channel = 0;
	lpg_p_t     lpgp;
	path_p_t    pathp;
	cred_t *localcred;
	queue_t *dummy_arg;

	/*
	 * Only one open is made to the underlying layer at any time. That
	 * open is the first one received. We use all-powerful creds here
	 * assuming that some layer above has checked.
	 */
	if (drv_getparm(SYSCRED, &localcred) == -1) {
		return FAILURE;
	}

	/*
	 * We are only supporting one lpg hence lpg_vector is always 0.
	 * This may change to open all paths on all lpgs.
	 */
	lpgp = vdevp->lpg_vector[0].working_lpg;

	if ((pathp = MPIO_QM_HEAD(&lpgp->active_list)) == NULL) {
		return FAILURE;
	}

	for (; pathp != NULL; pathp = MPIO_QM_NEXT(&lpgp->active_list,pathp)) {
		if (MPIO_KERNEL_OPEN8(pathp->devicep, &lower_channel,
				FREAD|FWRITE, localcred, dummy_arg) != 0){
			status = FAILURE;
		}
	}
	return status;
}

status_t
mpio_closeAll(vdev_t *vdevp)
{
	status_t    status = OK;
	lpg_p_t     lpgp;
	path_p_t    pathp;
	cred_t	*localcred;
	queue_t *dummy_arg;

	/*
	 * Use all-powerful creds because we opened with it.
	 */
	if (drv_getparm(SYSCRED, &localcred) == -1) {
		return FAILURE;
	}

	/*
	 * We are only supporting one lpg hence lpg_vector is always 0.
	 * This may change to open all paths on all lpgs.
	 */
	lpgp = vdevp->lpg_vector[0].working_lpg;
	for (pathp = MPIO_QM_HEAD(&lpgp->active_list); pathp != NULL; 
			pathp = MPIO_QM_NEXT(&lpgp->active_list,pathp)) {
		if (MPIO_KERNEL_CLOSE8(pathp->devicep, FREAD|FWRITE,
				localcred, dummy_arg) != 0){
			status = FAILURE;
		}
	}
	return status;
}

/*
 *
 * Assumption: The paths cannot change state while were doing open.
 */
status_t
mpio_open8(void *idatap, ulong_t *channelp, int flags, cred_t *credp,
           queue_t *dummy_arg)
{
    vdev_p_t    vdevp;
    status_t    status = OK;

    MPIO_DEBUG1(0,"open8 Vdev:0x%x",vdevp);
     
    /*
     * Obtain the handle of the this Vdev object.
     */
    vdevp = (vdev_p_t)idatap;
    
    ASSERT(vdevp);

    MPIO_VDEV_LOCK(vdevp);
    if (vdevp->open_count++ == 0) {
	    MPIO_VDEV_UNLOCK(vdevp);

	    MPIO_VDEV_SLEEP_LOCK(vdevp);
	    if (mpio_openAll(vdevp) != OK) {
		mpio_closeAll(vdevp);

	        MPIO_VDEV_LOCK(vdevp);
	        vdevp->open_count--;
	        MPIO_VDEV_UNLOCK(vdevp);

		status = FAILURE;
	    }
	    MPIO_VDEV_SLEEP_UNLOCK(vdevp);
    } else {
    	MPIO_VDEV_UNLOCK(vdevp);
    }

    return status;
}
/* END mpio_open8 */

status_t
mpio_close8(void *idatap, ulong_t channel, int flags, cred_t *credp,
            queue_t *dummy_arg)
{
    vdev_p_t    vdevp;
    status_t    status = OK;

    MPIO_DEBUG1(0,"close Vdev:0x%x",vdevp);
     
    /*
     * Obtain the handle of the this Vdev object.
     */
    vdevp = (vdev_p_t)idatap;
    
    ASSERT(vdevp);

    MPIO_VDEV_LOCK(vdevp);
    vdevp->open_count = 0;
    MPIO_VDEV_UNLOCK(vdevp);

    MPIO_VDEV_SLEEP_LOCK(vdevp);
    mpio_closeAll(vdevp);
    MPIO_VDEV_SLEEP_UNLOCK(vdevp);

    return (status);
}
/* END mpio_close8 */

/*
 * mpio_is_a_recoverable_error(int status)
 *		    )
 * Description
 *    See if the given status from an I/O operation indicates that a
 *    new path should be chosen because of a recoverable failure.
 */

boolean_t
mpio_is_a_recoverable_error(status)
{
    if ((status == MPIO_EIO_PHYSICAL_UNIT_FAILURE) ||
	(status == MPIO_EIO_DEVICE_TIMED_OUT) ||
	(status == MPIO_EIO_DEVICE_SELECT_TIMED_OUT) ||
	(status == MPIO_EBUSY_UNIT_ASSIGNED_TO_ANOTHER_CONTROLLER)){
	return (TRUE);
    }
    else {
	return (FALSE);
    }
}
/* END mpio_is_a_recoverable_error */


/*
 * Vdev implementation section begins.
 */

/*
 * mpio_vdev_create()
 *
 * Description:
 *
 *
 * Calling/Exit State:
 *   NULL    Memory allocation failed.
 *   Value    Pointer to the Vdev is returned.
 */
vdev_p_t
mpio_vdev_create()
{
    vdev_p_t	    vdevp;
    int		    i;

    MPIO_DEBUG0(0,"enter mpio_vdev_create");
    /*
     *    Allocate neutral memory (as oppose to NUMAized mem) for the Vdev.
     *    Initialize its data elements.
     *    Bind the new path to this Vdev.
     */
    vdevp = (vdev_p_t) VKMEM_ALLOC (sizeof(vdev_t));
    if (vdevp == NULL) {
	MPIO_DEBUG0(0,"no mem");
	   err_log0(MPIO_MSG_NOMEM);
	   return NULL ;
    }
    vdevp->open_count = 0;
    vdevp->state = FVS_NORMAL;
    vdevp->number_of_paths = 0;
    mpio_qm_initialize(&vdevp->paths,MEMBER_OFFSET(path_t,next_path));
    VSPIN_LOCK_ALLOC(1, &vdevp->lock, &mpio_vdev_lkinfo);
    vdevp->vdev_sleeplock = SLEEP_ALLOC(0, &mpio_vdev_sleep_lkinfo, KM_SLEEP);

    for (i = 0 ; i < MAX_LOCALES_PER_SYSTEM ; i++){
	vdevp->lpg_vector[i].home_lpg = NULL ;
	vdevp->lpg_vector[i].working_lpg = NULL ;
    }

    return (vdevp);
}
/* END mpio_vdev_create */

/*
 * mpio_vdev_delete(vdev_p_t rdevp)
 *
 * Description:
 *    Destroy the Vdev object if the device is in a sane state eg.
 *    open_count = 0;
 *
 * Calling/Exit State:
 *    B_TRUE    All paths were deleted.
 *    B_FALSE    Deletion cannot be done due to open_count != 0 or one of
 *	    Path deletion failed.
 * 
 */
boolean_t
mpio_vdev_delete(vdev_p_t vdevp)
{
    int	i;
    lpg_p_t    lpgp;

    MPIO_DEBUG0(0,"enter mpio_vdev_delete");

    if (vdevp->open_count){
	return B_FALSE;
    }
    for (i = 0 ; i < MAX_LOCALES_PER_SYSTEM ; i++){
	lpgp = vdevp->lpg_vector[i].home_lpg;
	if (lpgp != NULL){
	    if (!mpio_lpg_delete(lpgp)){
		return B_FALSE;
	    }
	} 
    }
    SLEEP_DEALLOC(vdevp->vdev_sleeplock);
    VSPIN_LOCK_DEALLOC(&vdevp->lock);
    sdi_sigFree(vdevp->signature);
    VKMEM_FREE(vdevp, sizeof(vdev_t));
    return B_TRUE;
}
/* END mpio_vdev_delete */

/*
 * mpio_vdev_put(sdi_device_t *rdevp, vdev_p_t vdevp) 
 * 
 * Description:
 *     Add a path to the existing Vdev.
 * 
 * Calling/Exit State:
 *   B_TRUE	The path was inserted successfully.
 *   B_FALSE    The path could not be inserted.
 * 
 * Note:
 *     This function may cause a LPG to lose its active path(s).
 *     In certain situations, this may kick off recovery activity.
 */
boolean_t
mpio_vdev_put(sdi_device_t *rdevp, vdev_p_t vdevp)
{
    boolean_t	status = B_TRUE;
    path_p_t	pathp;
    lpg_p_t	lpgp;

    /*
     *  Create a Path for this Rdev.
     *  Get Path signature
     *  Create a Locale Path Group (LPG) object if one is not present.
     *  Put Path into LPG
     */

    MPIO_DEBUG0(0,"enter mpio_vdev_put");

    if ((pathp = mpio_path_create(rdevp)) == NULL){
	return B_FALSE;
    }

    pathp->state = vdevp->signature->sig_state;

    MPIO_VDEV_SLEEP_LOCK(vdevp);

    /*
     * Create a LPG if necessary and then add the path to the new LPG.
     */
    if ((lpgp = vdevp->lpg_vector[pathp->cpugroup].home_lpg) == NULL){
	lpgp = mpio_lpg_create(pathp-> cpugroup);
	if (lpgp == NULL){
	    goto unlock;
	}
	vdevp->lpg_vector[pathp->cpugroup].home_lpg = lpgp;
	/*
	 * If this locale was switched to somewhere else, move it home.
	 * Doing this can cause a LPG to lose its active paths all
	 * of a sudden if the new path is the first path of this LPG,
	 * and not an active path either. If this happens, a recovery
	 * process will kick off automtically.
	 */
	if (vdevp->lpg_vector[pathp->cpugroup].working_lpg 
			!= vdevp->lpg_vector[pathp->cpugroup].home_lpg){
	    vdevp->lpg_vector[pathp->cpugroup].working_lpg = lpgp;
	}
    }

    /*
     * Add the path to the LPG. If fail, delete the path.
     */
    if (!mpio_lpg_put(pathp, lpgp)){
	mpio_path_delete(pathp);
	status = B_FALSE;
	goto unlock;
    }

    mpio_qm_add_tail(&vdevp->paths, pathp);
    vdevp->number_of_paths++;

unlock:
    MPIO_VDEV_SLEEP_UNLOCK(vdevp);
    return (status);
}
/* END mpio_vdev_put */

/*
 * mpio_vdev_remove_path(path_p_t pathp, vdev_p_t vdevp) 
 * 
 * Description:
 *     Remove a path from Vdev jurisdiction.
 * 
 * Calling/Exit State:
 *   B_TRUE	The path was removed successfully.
 *   B_FALSE    The path could not be removed.
 * 
 */
boolean_t
mpio_vdev_remove_path(path_p_t pathp, vdev_p_t vdevp) 
{
    lpg_p_t	    lpgp;
    status_t	status;

    MPIO_DEBUG0(0,"enter mpio_vdev_remove");

    MPIO_VDEV_SLEEP_LOCK(vdevp);

    /*
     * remove the path from the LPG
     */
    if ( !mpio_lpg_remove_path(
	    pathp,vdevp->lpg_vector[pathp->cpugroup].home_lpg)){
	status = B_FALSE;
	goto unlock_label;
    }
    /*
     * Remove and delete the path from the Vdev.
     */
    ASSERT(mpio_qm_is_member(&vdevp->paths, pathp));
    mpio_qm_remove(&vdevp->paths, pathp);
    mpio_path_delete(pathp);
    vdevp->number_of_paths--;
    status = B_TRUE;

unlock_label:
    MPIO_VDEV_SLEEP_UNLOCK(vdevp);
    return status;
}
/* END mpio_vdev_remove_path */

/*
 *    path_p_t
 *    mpio_vdev_get_path_by_rmkey(rm_key_t rm_key, vdev_p_t vdevp)
 *
 *    Description:
 *	Returns the path of the vdev matching rm_key.
 *
 */
path_p_t
mpio_vdev_get_path_by_rmkey(rm_key_t rm_key, vdev_p_t vdevp)
{
    path_p_t	pathp;

    pathp = MPIO_QM_HEAD(&vdevp->paths);
    while (pathp != NULL){
	if (pathp->devicep->sdv_handle == rm_key)
	    break;
        pathp = MPIO_QM_NEXT(&vdevp->paths, pathp);
	  }
    return pathp;
}
/* END mpio_vdev_get_path_by_rmkey */

/*
 * Locale path group (LPG) implementation.
 */

/*
 * mpio_lpg_create(int cpugroup)
 * 
 * Description:
 *    Create a Locale Path Group (LPG) object.
 *
 * Calling/Exit State:
 *    On return, the return value can be:
 *	NULL:	The memory allocation failed or mpio_lpg_put() failed.
 *	non-NULL:    This is the pointer to the new LPG.
 */
lpg_p_t
mpio_lpg_create(int cpugroup)
{
    lpg_p_t    lpgp;

    /*
     *    allocate NUMAized memory.
     *    initilize its data elements.
     */
    MPIO_DEBUG0(0,"enter mpio_lpg_create");
    lpgp = VNUMA_KMEM_ALLOC(sizeof(lpg_t), cpugroup);
    if (lpgp == NULL){
	MPIO_DEBUG0(0,"no mem");
	err_log0(MPIO_MSG_NOMEM);
	return NULL ;
    }
    mpio_qm_initialize(&lpgp->active_list,MEMBER_OFFSET(path_t,next_in_queue));
    mpio_qm_initialize(&lpgp->inactive_list,MEMBER_OFFSET(path_t,next_in_queue));
    mpio_qm_initialize(&lpgp->failed_list,MEMBER_OFFSET(path_t,next_in_queue));

    lpgp->cpugroup = cpugroup;
    return lpgp;
}
/* END mpio_lpg_create */

/*
 * mpio_lpg_delete(lpg_p_t)
 *
 * Description:
 *    Delete this Locale Path Group (LPG) object.
 *
 * Calling/Exit State:
 *    B_TRUE    The object was purged. All queues were close successfully.
 *    B_FALSE    Could not purged the object cleanly.
 *	    Object is still alive.
 *  
 */
boolean_t
mpio_lpg_delete(lpg_p_t lpgp)
{
    path_p_t    pathp;

    /*
     *    Only if all queue can be delete then we can delete
     *    this LPG. Otherwise, return failure status;
     */
    MPIO_DEBUG0(0,"enter mpio_lpg_delete");
    if (mpio_lpg_delete_queue(&lpgp->active_list) &&
	mpio_lpg_delete_queue(&lpgp->inactive_list) &&
	mpio_lpg_delete_queue(&lpgp->failed_list)){
	VKMEM_FREE(lpgp,sizeof(lpg_t));
	return B_TRUE;
    }
    else {
	return B_FALSE;
    }
}
/* END mpio_lpg_delete */


/*
 * mpio_lpg_put(path_p_t pathp, lpg_p_t lpgp)
 *
 * Description:
 *    Insert a path into this Locale Path Group (LPG).
 *
 * Calling/Exit State:
 *    B_TRUE    Path was put into this LPG.
 *    B_FALSE    Could not put. Path state is bad.
 *       
 */
boolean_t
mpio_lpg_put(path_p_t pathp, lpg_p_t lpgp)
{

    MPIO_DEBUG0(0,"enter mpio_lpg_put");
    switch (pathp->state){
	case SDI_PATH_ACTIVE:
	    mpio_qm_add_head(&lpgp->active_list, pathp);
	    break;
	case SDI_PATH_INACTIVE:
	    mpio_qm_add_head(&lpgp->inactive_list,pathp);
	    break;
	case SDI_PATH_FAILED:
	    mpio_qm_add_head(&lpgp->failed_list, pathp);
	    break;

	default:
	    return B_FALSE;;
    }
    return B_TRUE;
}
/* END mpio_lpg_put */


/*
 * mpio_lpg_remove(path_p_t pathp, lpg_p_t lpgp)
 *
 * Description:
 *    Remove a path from this Locale Path Group (LPG).
 *
 * Calling/Exit State:
 *    B_TRUE    Path was removed into this LPG.
 *    B_FALSE    Could not be removed. Path state is bad.
 *       
 */
boolean_t
mpio_lpg_remove_path(path_p_t pathp, lpg_p_t lpgp)
{

    MPIO_DEBUG0(0,"enter mpio_lpg_remove");
    switch (pathp->state){
	case SDI_PATH_ACTIVE:
	    ASSERT(mpio_qm_is_member(&lpgp->active_list, pathp));
	    mpio_qm_remove(&lpgp->active_list, pathp);
	    break;
	case SDI_PATH_INACTIVE:
	    ASSERT(mpio_qm_is_member(&lpgp->inactive_list, pathp));
	    mpio_qm_remove(&lpgp->inactive_list, pathp);
	    break;
	case SDI_PATH_FAILED:
	    ASSERT(mpio_qm_is_member(&lpgp->failed_list, pathp));
	    mpio_qm_remove(&lpgp->failed_list, pathp);
	    break;

	default:
	    return B_FALSE;;
    }
    return B_TRUE;
}
/* END mpio_lpg_remove_path */

/*
 * mpio_lpg_delete_queue(mpio_qm_anchor_t *)
 *
 * Description:
 *    Delete a queue of this LPG and its paths.
 *    If the path deletion fails, abort and return.
 */
boolean_t
mpio_lpg_delete_queue(mpio_qm_anchor_t * q)
{
    path_p_t    pathp, nextpathp;

    MPIO_DEBUG0(0,"mpio_lpg_delete_queue");
    while ((pathp = mpio_qm_remove_head(q))!= NULL){
	mpio_path_delete(pathp);
    }
    return B_TRUE;
}
/* END mpio_lpg_delete_queue */

/*
 * mpio_lpg_switch_state_after_trespass(lpg_p_t lpgp);
 *  
 * Description:
 *  Make the active queue to become inactive and the
 *  inactive queue to become active.
 *
 */
void
mpio_lpg_switch_state_after_trespass(lpg_p_t lpgp)
{
    mpio_qm_anchor_t	temp_q;
    path_p_t	next_pathp;

    mpio_qm_initialize (&temp_q, MEMBER_OFFSET(path_t,next_in_queue));

    /* switch the active and inacive queues */

    mpio_qm_move (&lpgp->active_list, &temp_q);
    mpio_qm_move(&lpgp->inactive_list, &lpgp->active_list);
    mpio_qm_move(&temp_q, &lpgp->inactive_list);

    /* update new active path state */

    next_pathp = MPIO_QM_HEAD( &lpgp->active_list);
    while (next_pathp != NULL){
	next_pathp->state = SDI_PATH_ACTIVE;
	next_pathp = MPIO_QM_NEXT(&lpgp->active_list, next_pathp);
    }

    /* update new inactive paths state */

    next_pathp = MPIO_QM_HEAD( &lpgp->inactive_list);
    while (next_pathp != NULL){
	next_pathp->state = SDI_PATH_INACTIVE;
	next_pathp = MPIO_QM_NEXT(&lpgp->inactive_list, next_pathp);
    }

    return;
}
/* END mpio_lpg_switch_state_after_trespass */

/*
 * mpio_lpg_get_active_path()
 * 
 * Description
 *     Return the next active path based on the current load balancing
 *     policy.
 * 
 * Currently, we only implement round-robin load balancing policy. Other
 * feasible policies are: path performance and/or instantaneous path load.
 * 
 * Calling/Exit State:
 */
path_p_t
mpio_lpg_get_active_path(lpg_p_t lpgp)
{
    return((path_p_t)mpio_qm_next_and_rotate(&lpgp->active_list));
}

/*
 * mpio_lpg_get_inactive_path()
 * 
 * Description
 *     Return the next inactive path of this LPG.
 * 
 * Calling/Exit State:
 */
path_p_t
mpio_lpg_get_inactive_path(lpg_p_t lpgp)
{
    return((path_p_t)MPIO_QM_HEAD(&lpgp->inactive_list));
}

/*
 * void
 * mpio_lpg_fail_path(pathp, lpgp)
 *
 * Description:
 *    Mark the path of this LPG as failed and move it to the failed queue.
 *
 * Calling/Exit State:
 *     Path state must be something else but SDI_PATH_FAILED.
 */
void
mpio_lpg_fail_path(path_p_t pathp, lpg_p_t lpgp, path_fail_reason_t reason)
{
    MPIO_DEBUG0(0,"enter mpio_lpg_fail_path");

    switch(pathp->state){
	case SDI_PATH_FAILED:
	    /*
	     * The path is already failed. just return.
	     */
	    break;
	case SDI_PATH_ACTIVE:
	    if (mpio_qm_is_member(&lpgp->active_list, pathp)){
		mpio_qm_remove(&lpgp->active_list,pathp);
		mpio_qm_add_head((&lpgp->failed_list), pathp);
		pathp->state = SDI_PATH_FAILED;
		pathp->failed_reason = reason;
	    }
	    break;
	case SDI_PATH_INACTIVE:
	    if (mpio_qm_is_member(&lpgp->inactive_list, pathp)){
		mpio_qm_remove(&lpgp->inactive_list,pathp);
		mpio_qm_add_head((&lpgp->failed_list),pathp);
		pathp->state = SDI_PATH_FAILED;
		pathp->failed_reason = reason;
	    }
	    break;
	default:
	    break;
    }
    return;
}
/* END mpio_lpg_fail_path */

/*
 * sig_compare_result_t
 * mpio_lpg_repair_path(path_p_t pathp, lpg_p_t lpgp, signature_p_t sigp)
 *
 * pathp:	pointer to the Path
 *
 * lpgp:	pointer to the LPG of the path inquestion
 *
 * sigp:	pointer to this path's expected signature.
 *
 * Description:
 *    Issuing a special ioctl to the device driver commanding it to
 *    reinit the interface of this path. Verify that that path is
 *    still connected to the same device (checking signature again)
 *
 *    If the path is connected to an inactive port, it will be placed
 *    int the inactive queue.
 *
 * Calling/Exit State:
 *     Lock is held by the caller to protect the path while it
 *     is manipulated.
 * 
 * Note:
 * 
 */
sig_compare_result_t
mpio_lpg_repair_path(path_p_t pathp, lpg_p_t lpgp, sdi_signature_p_t sigp)
{
    sdi_signature_t	    *temp_sig;
    sig_compare_result_t    result;

    MPIO_DEBUG0(0,"enter mpio_lpg_repair_path");

    if( pathp->state != SDI_PATH_FAILED || !mpio_qm_is_member(&lpgp->failed_list, pathp)){
	return;
    }
    /*
     * Issue an icmd to repair the path and find out what port it's
     * connected to. Move the path from the failed state to the new 
     * state ie. inactive or active???.
     */
#ifdef notyet
    if (MPIO_REPAIR_PATH(pathp->devicep) != OK){
	err_log1(MPIO_MSG_REPAIR_PATH_FAILURE, pathp);
	return;
    }
#endif
    result = mpio_path_verify(pathp, sigp);
    pathp->state = sigp->sig_state;
    switch (result){
	case FSIG_NO_MATCHED:
	    mpio_lpg_fail_path(pathp, lpgp, MISC_CAUSE);
	    break;
	case FSIG_MATCHED:
	case FSIG_HALF_MATCHED:
	/*
	 * What can we do if we found a half matched signature?
	 * Well, we leave it up to the client to verify the signature
	 * if it wants to. As far as we (this routine) concern, a 
	 * half matched signature is reliable enough.
	 * 
	 * Move this path to the according group ie. active vs inactive.
	 */
	    if (mpio_qm_is_member(&lpgp->failed_list, pathp)){
		mpio_qm_remove(&lpgp->failed_list,pathp);
		if ( pathp->state == SDI_PATH_ACTIVE){
		    mpio_qm_add_tail(&lpgp->active_list, pathp);
		}
		else {
		    mpio_qm_add_tail((&lpgp->inactive_list), pathp);
		}
	    }
	    break;
    }
    return result;
}
/* END mpio_lpg_repair_path  */

/*
 * void
 * mpio_lpg_activate_path(pathp, lpgp, vdevp)
 *
 * Description:
 *    Initiate a trespass if necessary to activate this path.
 *    Shutdown the path if the trespassing rate exceeds the limit.
 *
 * Calling/Exit State:
 *
 *    If the path is in inactive, the whole locale state will
 *    be swapped active/inactive.
 *
 * Note:
 *    We may pend here for a while waiting for the trespass
 *    to complete depending on how the driver implmemented 
 *    the ioctl for tresspass.
 */
status_t
mpio_lpg_activate_path(
	    path_p_t pathp, 
	    lpg_p_t lpgp, 
	    vdev_p_t vdevp,
	    path_activation_reason_t cause)
{
    os_time_t    now;
    ulong	per_minute_rate;
    int	    count, status;

    MPIO_DEBUG0(10,"enter mpio_lpg_activate_path");

    status = OK;
    /*
     * We activate an inactive path by issuing a trespass then switch
     * the active/inactive state of all paths of this LPG.
     * 
     * We activate an already active path by doing nothing. There shouldn't
     * be this situation.
     * 
     * We don't activate a failed path. This is an error on the caller part.
     * 
     */
    switch (pathp->state) {
	case SDI_PATH_INACTIVE:
	    err_log1(MPIO_MSG_TRESPASS_PATH, pathp);
	    if (cause == EXTERN_ACTIVATE_CAUSE){
		/*
		 *  See if we're experiencing excessive trespasses. Shutdown
		 *  this path if we are.
		 */	
		pathp->activate_count++;
		if (pathp->activate_count == 1){
		    /*
		     * The lapse begins. Restart the clock.
		     */	
		    MPIO_GET_WALL_CLOCK(&now);
		    pathp->clock_begin = now;
		    per_minute_rate = 0;
		}
		if (pathp->activate_count == MPIO_ACTIVATE_COUNT_LIMIT ){
		    /*
		     * We have enough samples. Compute the rate.
		     * If the rate exceeds the limit, fail this path.
		     */	
		    MPIO_GET_WALL_CLOCK(&now);
		    per_minute_rate =  pathp->activate_count * 60 
				/ (ulong)(now - pathp->clock_begin);
		    pathp->activate_count = 0;
		    if ( per_minute_rate > MPIO_ACTIVATE_RATE_LIMIT){
			err_log0(MPIO_MSG_EXCESSIVE_TRESPASS, vdevp);
			mpio_lpg_fail_path (pathp, lpgp, EXCESSIVE_TRESPASS);
			status = FAILURE;
			break;
		    }
		}
	    }
	    /*
	     * Issue a trespass then rearranging path states of the this
	     * LPG. The caller will have to fix other LPGs because this is
	     * a LPG level method, and therefore it does not know about other
	     * LPGs or Vdev.
	     */	
	    if (MPIO_TRESPASS(pathp->devicep) != OK){
		err_log1(MPIO_MSG_TRESPASS_PATH_FAILURE,pathp);
		status =  FAILURE;
	    } else {
		/*
		 * The trespass has succeeded, previously unreachable devices
		 * are now accessible. In particular opens had never succeeded.
		 * Issue the opens after the appropiate closes.
		 */
	    	mpio_closeAll(vdevp);
	    	mpio_lpg_switch_state_after_trespass(lpgp);
	    	if (mpio_openAll(vdevp) != OK) {
	    		mpio_closeAll(vdevp);
			status =  FAILURE;
		}
	    }
	    break;
	case SDI_PATH_ACTIVE:
	    break;
	case SDI_PATH_FAILED:
	default:
	    status = FAILURE;
	    break;
    }
    return status;
}
/* END mpio_lpg_activate_path */

/*
 * Path implementation section.
 */

/*
 * mpio_path_create(sdi_device_t rdevp) -  Create a Path object.
 * 
 * Description
 *     The Path is instantiated and all channels are open ready for use.
 * 
 * Calling/Exit State:
 *   NULL    Memory allocation failed or opens failed.
 *   Value    Pointer to the new path
 */

path_p_t
mpio_path_create(sdi_device_t *rdevp)
{
    int 		    locale_id;
    path_p_t		    pathp;
    int			    status;

    MPIO_DEBUG0(0,"enter mpio_path_create");

/*
 *  locale_id = MPIO_GET_PATH_LOCALE(rdevp);
 */
    locale_id = rdevp->sdv_cgnum;
    pathp = VNUMA_KMEM_ALLOC(sizeof(path_t), locale_id);
    if (pathp == NULL){
	MPIO_DEBUG0(0,"no mem");
	err_log0(MPIO_MSG_NOMEM);
    }
    else {
	pathp->devicep = rdevp;
	pathp->state = SDI_PATH_ACTIVE;
	pathp->cpugroup = locale_id;
	pathp->assert_error = B_FALSE;
	mpio_acct_create(&pathp->acct);
    }

    return(pathp);
}
/* END mpio_path_create */

/*
 * mpio_path_delete(path_p_t pathp) -  Delete a Path object.
 * 
 * Description
 *    Close the path and deallocate memory. If the path closure failed,
 *    we display a console message, but will continue. It's possible that
 *     the closure failed due to device has gone. It's really upset the
 *     users if we persistently refuse to complete the operation.
 * 
 * Calling/Exit State:
 */
void
mpio_path_delete(path_p_t pathp)
{
    MPIO_DEBUG0(0,"enter mpio_path_delete");

    mpio_acct_delete(&pathp->acct);
    VKMEM_FREE(pathp, sizeof(path_t));
}
/* END mpio_path_delete */

/*
 * mpio_path_verify(path_p_t pathp, signature_p_t sigp)
 *
 * Description:
 *    Obtain and compare the current signature with the given one.
 *
 * Calling/Exit State:
 *    OK:	    signatures match or half match.
 *    FAILURE:    signatures don't match.
 *
 *    New signature is returned if partially match.
 */
sig_compare_result_t
mpio_path_verify(path_p_t pathp, sdi_signature_p_t sigp)
{
    const sdi_signature_t	 *new_sig = NULL;
    sig_compare_result_t    result;
    int		    status;

    status = mpio_signature_and_state_get( pathp->devicep, &new_sig);
    if (status != OK){
		return FSIG_NO_MATCHED;
    }
    return mpio_signature_compare(sigp, new_sig);
}
/* END mpio_path_verify */


/*
 * Vdev implementation section.
 */

/*
 * void
 * mpio_vdev_recover_blkio_failure(path_p_t pathp, 
 *				   vdev_p_t vdevp, 
 *				   os_io_op_record_p_t bp)
 *
 * Description
 *    A path failure was discovered by the blkio completion thread. 
 *     This routine initiates the recovery process.
 *
 * Note
 *    We are running on the recovery LWP.
 *
 *    Vdev lock is held and released inside.
 */
void
mpio_vdev_recover_blkio_failure(path_p_t pathp, 
				vdev_p_t vdevp,
				os_io_op_record_p_t bp)
{
    int		    status;
    lpg_p_t	    lpgp;


    err_log2(MPIO_MSG_PATH_FAILURE, pathp, vdevp);

    /*
     * We now need to try and recover from the failed path.  We will
     * fail the path and look for another path.
     */
    status = mpio_vdev_recover_from_path_failure(&pathp, vdevp);
    if (status != OK){
	/*
	 * The path recovery was UNsuccessful. Just give up this I/O.
	 */
	mpio_signal_start_blkio_failure(bp);
	return;
    }
    /*
     * The recovery was successful. Attach the I/O request to the io 
     * resource and issue the request to the new Path. By the way,
     * update the recovery ticket number also. 
     *
     * Obtain the vdev lock just to bump the ticket is not pretty in term
     * of performance, but what the heck. We are doing failure recovery.
     */
    bioerror(bp, OK);
    sdi_buf_store(bp, sdi_get_blkno(bp));
    ((struct sdi_buf_save *)(bp->b_misc))->mpio_vdev = (void *)vdevp;
    ((struct sdi_buf_save *)(bp->b_misc))->mpio_rdev = (void *)pathp;
    bp->b_iodone = mpio_complete_blkio;
    MPIO_KERNEL_START_BLKIO8(bp, pathp->devicep);
}
/* End mpio_vdev_recover_blkio_failure */

/*
 * int mpio_vdev_recover_from_path_failure(
 *	    path_p_t *pathpp, vdev_p_t vdevp)
 *
 *    pathpp:	    pointer-pointer to the failed path. The new path
 *		    is returned in this object if found.
 *
 *    vdevp:	    pointer to the vdev involved.
 *
 * Description
 *    Declare this path as failed if we are asked to do so. Perform path 
 *    switch if necessary to find another working path.
 *
 *    If we run out of working paths, we'll attempt to repair failed paths
 *    and use them. However, we will obey the user's orders strictly as 
 *    such we will rather die than attempt to use the user failed paths
 *    even though these paths may be working. This is MPIO functionalilty.
 *
 * Calling/Exit:
 *    pathp:    the failing path. It could be NULL and that's why the
 *	    caller need help.
 *
 *    Status = OK and a new path will be returned if found.
 *
 *
 * Note
 *    We are running on the recovery LWP.
 *    Vdev lock and driver lock are obtained and released inside.
 */
int
mpio_vdev_recover_from_path_failure( path_p_t *pathpp, vdev_p_t vdevp)
{
    os_io_op_record_p_t bp;
    lpg_p_t		lpgp, this_lpgp;
    path_p_t	    newpathp, tmp_pathp;
    int		    i, locale, failed_locale, status, mismatch;
    sdi_signature_t	    *sig = NULL;
    path_state_t	old_state;

    status = !OK;

    
    if ( *pathpp != NULL){
	failed_locale = locale = (*pathpp)->cpugroup;
    }
    else {
	/*
	 * We're here because the caller found no active path available
	 * to start this I/O. This is a specical case of recovery for no
	 * path has failed anything.
	 */
	failed_locale = locale = 0;
    }

    MPIO_VDEV_LOCK(vdevp);
    if (vdevp->state == FVS_FAILED){
    	MPIO_VDEV_UNLOCK(vdevp);
    	return status;
    }
    MPIO_VDEV_UNLOCK(vdevp);

    this_lpgp = vdevp->lpg_vector[locale].home_lpg;
    /*
     *  Fail the failing path if the caller ask for.
     */
    MPIO_VDEV_SLEEP_LOCK(vdevp);
    if (*pathpp != NULL){
	mpio_lpg_fail_path(*pathpp, this_lpgp, MISC_CAUSE);
    }
    /*
     * Find another path according to the current fully connected policy.
     * Currently, we have two policies. The below table shows the steps
     * taken for each of them.
     * 
     *    LOCAL INACTIVE FIRST	    FAR FIRST
     * -----------------------------------------------------
     * 1. a local active path	    local active
     * 2. a local inactive	    far active
     * 3. a far active		    local inactive
     * 4. a far inactive       	    far inactive
     * 5. repair all failed paths.  repair failed paths.
     * 
     */
    for(;;){
	/*
	 * Step 1. Try another local active path
	 */
	MPIO_DEBUG0(10,"mpio: recovery - find another local optimal path");
	if ((newpathp = mpio_lpg_get_active_path(this_lpgp)) != NULL){
	    goto found_path_label;
	}
	/*
	 * Step 2. 
	 */
	if (dip->fc_policy == FC_ROUTING_INACTIVE_FIRST){
	    /*
	     * Try any local inactive path
	     */
	    MPIO_DEBUG0(10,"mpio: recovery - find a local inactive path");
	    if ((newpathp = mpio_lpg_get_inactive_path(this_lpgp)) != NULL){
		goto found_path_label;    
	    }
	}
	else {
	    /*
	     * Try any far active path
	     */
	    MPIO_DEBUG0(10,"mpio: recovery - find any far active path");
	    for( i = 0, locale = failed_locale;
		 i  < (dip->max_locales -1);
		 i++){
		locale = (locale +1) % dip->max_locales;
		lpgp = vdevp->lpg_vector[locale].working_lpg;
		if (lpgp != NULL){
		    if ((newpathp = mpio_lpg_get_active_path(lpgp)) != NULL){
			this_lpgp = lpgp;
			goto found_path_label;
		    }
		}
	    }
	}
	/*
	 * Step 3.
	 */
	if ( dip->fc_policy == FC_ROUTING_INACTIVE_FIRST ){
	    /*
	     * Try any far active path
	     */
	    MPIO_DEBUG0(10,"mpio: recovery - find any far active path");
	    for(i = 0, locale = failed_locale;
		    i < (dip->max_locales -1 ); 
		    i++){
		locale = (locale +1) % dip->max_locales;
		lpgp = vdevp->lpg_vector[locale].working_lpg;
		if (lpgp != NULL){
		    if ((newpathp = mpio_lpg_get_active_path(lpgp)) != NULL){
			this_lpgp = lpgp;
			goto found_path_label;
		    }
		}
	    }
	}
	else {
	    /*
	     * Try any local inactive path
	     */
	    MPIO_DEBUG0(10,"mpio: recovery - find a local inactive path");
	    if ((newpathp = mpio_lpg_get_inactive_path(this_lpgp)) != NULL){
		goto found_path_label;    
	    }
	}
	/*
	 * Step 4.
	 */
	MPIO_DEBUG0(10,"mpio: recovery - find any far inactive path");
	for(    i = 0, locale = failed_locale;
		i < (dip->max_locales-1); 
		i++){
	    locale = (locale +1) % dip->max_locales;
	    lpgp = vdevp->lpg_vector[locale].working_lpg;
	    if (lpgp != NULL){
		if((newpathp = mpio_lpg_get_inactive_path(lpgp)) != NULL){
		    this_lpgp = lpgp;
		    goto found_path_label;
		}
	    }
	}
	/*
	 * Step 4.
	 *
	 * Last chance. Try to repair all failed paths once then restart
	 * the search over again.
	 */
	if (vdevp->state != FVS_NORMAL){
	    err_log1(MPIO_MSG_FAIL_VDEV, vdevp);
	    vdevp->state = FVS_FAILED;
	    goto update_state_return_label;
	} else {
	    MPIO_DEBUG0(10,"mpio: recovery - start to repair all paths");
	    err_log1(MPIO_MSG_AUTO_REPAIR, vdevp);

	    vdevp->state = FVS_REPAIRING;
	    sig = vdevp->signature;

#ifdef OUT
	    for(i = 0; i < dip->max_locales; i++){
		lpgp = vdevp->lpg_vector[i].home_lpg;
		if (lpgp != NULL){
		    newpathp = MPIO_QM_HEAD(&lpgp->failed_list);
		    while(newpathp != NULL){;
			tmp_pathp = newpathp;
			newpathp = MPIO_QM_NEXT(&lpgp->failed_list,newpathp);
			if (tmp_pathp->failed_reason == MISC_CAUSE){
			    mpio_lpg_repair_path(tmp_pathp, lpgp, sig);
			}
		    }
		}
	    }
#endif

	}
    } /* end for() */

found_path_label:
    /*
     *  We found a path that seems a good candidate for trying again.
     *  Arrange for the home locale to be redirected to the one
     *  containing the path we've found. Then activate the path.
     */

    mpio_vdev_switch_lpg_vector(failed_locale, newpathp->cpugroup, vdevp);
    old_state = newpathp->state;
    if (mpio_lpg_activate_path (newpathp,
				    this_lpgp,
				    vdevp,
				    EXTERN_ACTIVATE_CAUSE) == OK){
	if ( old_state == SDI_PATH_INACTIVE){

	    /*
	     * The path was in inactive state. The activation has caused a
	     * trespass and the whole LPG state has switched. Switch others
	     * LPGs to reflect this state change also.
	     */

	    for(i = 0, locale = newpathp->cpugroup;i<dip->max_locales -1; i++){
		locale = (locale +1) % dip->max_locales;
		lpgp = vdevp->lpg_vector[locale].home_lpg;
		if (lpgp != NULL){
		    mpio_lpg_switch_state_after_trespass(lpgp);
		}
	    }
	}

        MPIO_VDEV_LOCK(vdevp);
	vdevp->state = FVS_NORMAL;
        MPIO_VDEV_UNLOCK(vdevp);

	*pathpp = newpathp;
	status = OK;
    }
    else if (vdevp->state == FVS_REPAIRING){
	/*
	 *  We did an auto path repair, but the path activation was failed.
	 *    Fail this device.
	 */
        MPIO_VDEV_LOCK(vdevp);
	vdevp->state = FVS_FAILED;
        MPIO_VDEV_UNLOCK(vdevp);

	*pathpp = NULL;
	status = FAILURE;
    }
    else {
	/*
	 *  We failed path activation, but more paths are available.
	 *  Just say OK. Let the I/O fail. We'll retry until all
	 *  paths are dead or the I/O goes thru.
	 */
	*pathpp = newpathp;
	status = OK;
	
    }

update_state_return_label:
    /*
     * Update the state transition count so that host applications know
     * that they need to update the system view.
     */
    MPIO_VDEV_SLEEP_UNLOCK(vdevp);

    MPIO_DRIVER_LOCK();
    dip->state_change_count++;
    MPIO_DRIVER_UNLOCK();
    return status;

}
/* End mpio_vdev_recover_from_path_failure */

/*
 * void
 * mpio_vdev_switch_lpg_vector(int this_cpugroup , int new_cpugroup, vdev_p_t);
 *
 * Description: 
 *    Redirect a LPG and all its payload (other LPGs that switched to it)
 *    to a new locale.
 *
 *    Notice that, we don't keep the history state, and therefore if
 *    it's switched back, we don't know about the payload to bring them
 *    back also.
 *
 * Calling/Exit State:
 * 
 */
void
mpio_vdev_switch_lpg_vector(int this_cpugroup, int new_cpugroup, vdev_p_t vdevp)
{
    lpg_p_t	    lpgp;
    lpg_vector_p_t    this_lpg_vectorp;
    lpg_vector_p_t    new_lpg_vectorp;
    lpg_vector_p_t    other_lpg_vectorp;
    int		i;

    MPIO_DEBUG0(0,"enter mpio_vdev_switch_lpg_vector");

    /*
     * Verify that we're not switching this LPG to itself.
     */
    if (this_cpugroup == new_cpugroup){
	return;
    }

    /*
     * Switch this LPG and its payload to the new LPG. The payload
     * are those which previously switched to it.
     */
    this_lpg_vectorp = &vdevp->lpg_vector[this_cpugroup];
    new_lpg_vectorp = &vdevp->lpg_vector[new_cpugroup];

    /*
     * But first, verify that we're not switching to a locale that 
     * currently switched to this one to prevent circular linkage.
     */
    if (new_lpg_vectorp->working_lpg->cpugroup == this_cpugroup){
	return;
    }

    /*
      *  Follow the direction of the new locale, meaning if the new
      *  locale is redirected to somewhere, we'll go there too.
     */
    this_lpg_vectorp->working_lpg->cpugroup = 
			    new_lpg_vectorp->working_lpg->cpugroup;
    this_lpg_vectorp->working_lpg = new_lpg_vectorp->working_lpg;

    for ( i = 0; i < MAX_LOCALES_PER_SYSTEM; i++) {
	if ((i != this_cpugroup) && (i != new_cpugroup)){
	    other_lpg_vectorp = &vdevp->lpg_vector[i];
	    if (other_lpg_vectorp->working_lpg->cpugroup == this_cpugroup){
		other_lpg_vectorp->working_lpg->cpugroup = new_cpugroup;
		other_lpg_vectorp->working_lpg = 
				new_lpg_vectorp->working_lpg;
	    }
	}
    }
    
    return;
}
/* end mpio_vdev_switch_lpg_vector */

/*
 * Signature implementation section begins.
 */

/*
 * sig_compare_result_t
 * mpio_signature_compare(signature_p_t sig0p, signature_p_t sig1p)
 */
sig_compare_result_t
mpio_signature_compare(const sdi_signature_t *sig0p, const sdi_signature_t *sig1p)
{
	if (bcmp(sig0p->sig_string, sig1p->sig_string,
		min(sdi_sigGetSize(sig0p), sdi_sigGetSize(sig1p))) == 0)
		return FSIG_MATCHED;
	else
		return FSIG_NO_MATCHED;
}
/* END mpio_signature_compare */

/*
 * void
 * mpio_signature_copy(signature_p_t sig0p, signature_p_t sig1p)
 *
 * Description:
 *
 * Calling/Exit State:
 *
 */
sdi_signature_t *
mpio_signature_copy(const sdi_signature_t *sig0p)
{
	sdi_signature_t *sigAux;

	sigAux = kmem_alloc(sig0p->sig_size, KM_SLEEP);
	bcopy(sig0p, sigAux, sig0p->sig_size);
	return sigAux;
}
/* END mpio_signature_copy */

/*
 * boolean_t
 * mpio_vdev_is_signature_consistent(signature_p_t usersigp, vdev_p_t vdevp)
 *
 * Description:
 *    Verify that all active and inactive paths of this vdev have the
 *    same signature.
 *
 * Calling/Exit State:
 *
 */
boolean_t
mpio_vdev_is_signature_consistent(const sdi_signature_p_t user_sigp, vdev_p_t vdevp)
{
    lpg_p_t		lpgp;
    path_p_t	    newpathp, tmp_pathp;
    int		    i;
    const sdi_signature_t *sig;

    /*
     * The for loop goes thru each locales once.
     */
    for(i = 0; i < dip->max_locales; i++){
	lpgp = vdevp->lpg_vector[i].home_lpg;
	if (lpgp != NULL){
	    newpathp = MPIO_QM_HEAD(&lpgp->active_list);
	    /*
	     * The while loop walks the active_list of the locale.
	     */
	    while ( newpathp != NULL){
		sig = NULL;
		if (mpio_signature_and_state_get(newpathp->devicep, &sig)
		    != OK){
		/*
		 * We failed to get its signature. The device
		 * is useless w/o signature. Fail it.
		 */
		    tmp_pathp = newpathp;
		    newpathp = MPIO_QM_NEXT(&lpgp->active_list,newpathp);
		    mpio_lpg_fail_path(tmp_pathp, lpgp,MISC_CAUSE);
		    continue;
		}

		if (mpio_signature_compare(user_sigp, sig) != FSIG_MATCHED){
		    return FALSE;
		}

		newpathp = MPIO_QM_NEXT(&lpgp->active_list,newpathp);
	    } /* while */
	} /* if (lpgp != NULL) */
    } /* for */

    /*
     * Continue to verify signature of the inactive paths.
     * The for loop goes thru each locale once.
     */
    for(i = 0; i < dip->max_locales; i++){
	lpgp = vdevp->lpg_vector[i].home_lpg;
	if (lpgp != NULL){
	    newpathp = MPIO_QM_HEAD(&lpgp->inactive_list);
	    /*
	     * The while loop walks the active_list of the locale.
	     */
	    while (newpathp != NULL){
		sig = NULL;
		if (mpio_signature_and_state_get(newpathp->devicep,&sig) != OK){
		    tmp_pathp = newpathp;
		    newpathp = MPIO_QM_NEXT(&lpgp->active_list,newpathp);
		    mpio_lpg_fail_path(tmp_pathp, lpgp, MISC_CAUSE);
		    continue;
		}

		if (mpio_signature_compare(user_sigp, sig) != FSIG_MATCHED){
		    return FALSE;
		}
		newpathp = MPIO_QM_NEXT(&lpgp->inactive_list,newpathp);
	    } /* while */
	} /* if */
    } /* for */

    return TRUE;
}
/* END mpio_vdev_is_signature_consistent */

/*
 * path_p_t
 * mpio_get_best_path( buf_p_t bp, vdev_p_t vdevp)
 *
 * Description:
 *    Verify that all active and inactive paths of this vdev have the
 *    same signature.
 *
 * Calling/Exit State:
 *
 */

path_p_t
mpio_get_best_path( buf_t	*bp, 
                    vdev_p_t	vdevp )
{
    int         i, cg;
    lpg_p_t     lpgp;

    /*
     * If there is no lpg to favor choose 0.
     */
    cg   = bp == NULL? 0 :  buf_best_cg( bp );

    lpgp = vdevp->lpg_vector[ cg ].working_lpg;

    if  ( lpgp != NULL ) {
        return( mpio_lpg_get_active_path( lpgp ) );
    }

    for ( i = 0; i <= MAX_LOCALES_PER_SYSTEM; i++ ) {
        lpgp = vdevp->lpg_vector[ i ].working_lpg;

        if  ( lpgp != NULL ) {
            vdevp->lpg_vector[ cg ].working_lpg =
                vdevp->lpg_vector[ i ].working_lpg;
            return( mpio_lpg_get_active_path( lpgp ) );
        }
    }

    return( NULL );
}
/* END mpio_get_best_path() */

/**** End mpio_core.c ****/
