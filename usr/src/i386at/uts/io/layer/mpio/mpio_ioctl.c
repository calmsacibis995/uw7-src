#ident	"@(#)kern-pdi:io/layer/mpio/mpio_ioctl.c	1.1.6.3"

/*
 *  This module contains the MPIO Driver ioctl handlers.
 *     
 * The philosophy about the device driver ioctl should be: It provides the 
 * simplest primitives upon which the applications will be constructed. 
 * Any required intelligence should be in the applications, not the device 
 * driver.
 *     
 * Let look at the switch locale example. If system requirements state
 * that we must preserve history states, then the history database is
 * better be done at the user level rather than at kernel level. That is,
 * the adm command should have the full configuration entered in its
 * database every time it notices a change. That way after it switches
 * the locale back, it can compare against the previous configuration
 * and perform macro operations using those driver primitives to reconstruct
 * the old configuration. I have no problem to believe that the adm
 * command level can trace 10 or 100 steps back easily. Making the driver
 * to trace just one step back will require significant efforts.
 *
 * The above discussion is also true for activate/deactivate stuff. If
 * we really want to preserve user-demand activate/deactivate, do it at 
 * the adm command. Don't try to get hang up on kernel implementation.
 *
 * All of the necessary ioctls() to manipulate MPIO have not been
 * worked out completely yet. But here is my suggestion:
 *
 * mpio_ioctl_fail_path() - to fail a path
 *     
 * mpio_ioctl_switch_path() - to switch a path
 *     
 * mpio_ioctl_repair_path() - to repair a path
 *     
 * mpio_ioctl_get_paths() - to get all existing Vdevs. This is a
 *    driver ioctl() eg /dev/mpio instead of device ioctl().
 *     
 * mpio_ioctl_get_vdev_children() - get the path info of a particular
 *       Vdev. The info are the path handles.
 *
 * mpio_ioctl_get_path_info() - get detailed info about a particular path.
 *       eg. cpugroup, state etc.
 *
 * mpio_ioctl_switch_locale() - to switch a locale.
 *     
 */

#include     "mpio.h"
#include     "mpio_ioctl.h"
#include     "mpio_os.h"
#include     "mpio_qm.h"
#include     "mpio_proto.h"
#include     "mpio_debug.h"

/*
 * mpio_ioctl_tuneable(int arg, int *rvalp)
 *
 *     arg:	  The pointer to a user space ioctl package detailing
 *		    this ioctl request (can it ever be a kernel package ??).
 *     rvalp	  The pointer to the returned data container.
 *
 * Description:
 *     For debug. Change MPIO fully-connected routing policy and other tuneables.
 *     
 */
int
mpio_ioctl_tuneable ( int arg, int *rvalp)
{
     int				   status;
     mpio_ioctl_tuneable_p_t     mpio_tuneable_pkgp;

     /*
      * Bring the user packet into kernel space.
      */
     mpio_tuneable_pkgp = (mpio_ioctl_tuneable_p_t)
     VKMEM_ALLOC(sizeof(mpio_ioctl_tuneable_t));
     if (mpio_tuneable_pkgp == NULL){
	 err_log0(MPIO_MSG_NOMEM);
	  status = MPIO_KMEM_ALLOC_FAILURE;
	  goto return_label;
     }
     if (VCOPYIN((void *)arg, (void *)mpio_tuneable_pkgp,
			      sizeof(mpio_ioctl_tuneable_t))
	       == VCOPY_FAILURE){
	  status  = MPIO_COPYIN_FAILURE;
	  goto copy_failure_return_label;
     }
     /*
      * Process ioctl packet.
      */
     if (mpio_tuneable_pkgp->fc_policy == 0){
	  dip->fc_policy = FC_ROUTING_INACTIVE_FIRST;
     }
     else { /* non zero */
	  dip->fc_policy = FC_ROUTING_FAR_FIRST;
     }
     status = 0;

copy_failure_return_label:
     VKMEM_FREE(mpio_tuneable_pkgp, sizeof(mpio_ioctl_tuneable_t));     
return_label:
     return status;

}
/* END mpio_ioctl_tuneable */

/*
 * mpio_ioctl_insert_error8(int arg, int *rvalp)
 *
 *     arg:       The pointer to a user space ioctl package detailing
 *                  this ioctl request (can it ever be a kernel package ??).
 *     rvalp      The pointer to the returned data container.
 *
 * Description:
 *  For debug. Turn the error flag ON to activate recovery code path.
 *
 */
int
mpio_ioctl_insert_error8(vdev_p_t vdevp, void *arg, int *rvalp)
{
     int                    status = 0;
     mpio_ioctl_err_p_t     err_pkgp;
     path_p_t		    pathp;

     ASSERT(vdevp);

     /*
      * Bring the user packet into kernel space.
      */
     err_pkgp = (mpio_ioctl_err_p_t)
     VKMEM_ALLOC(sizeof(mpio_ioctl_err_t));
     if (err_pkgp == NULL){
         err_log0(MPIO_MSG_NOMEM);
          status = MPIO_KMEM_ALLOC_FAILURE;
          goto return_label;
     }
     if (VCOPYIN(arg, (void *)err_pkgp,sizeof(mpio_ioctl_err_t))
               == VCOPY_FAILURE){
          status  = MPIO_COPYIN_FAILURE;
          goto copy_failure_return_label;
     }

     /*
      * Process ioctl packet - here, just update error flag
      */

     MPIO_VDEV_LOCK(vdevp);
     pathp = mpio_vdev_get_path_by_rmkey(err_pkgp->path_rmkey, vdevp);
     if (pathp == NULL){
          status = MPIO_INVAL_PATH ;
          goto unknown_path;
     }

     pathp->assert_error = err_pkgp->assert_error;

unknown_path:
     MPIO_VDEV_UNLOCK(vdevp);
copy_failure_return_label:
     VKMEM_FREE(err_pkgp, sizeof(mpio_ioctl_err_t));

return_label:
     return status;

}
/* END  */

/*
 *  mpio_ioctl_fail_path(dev_t dev, int arg, int *rvalp)
 *
 *      dev:	The descriptor for the device.
 *
 *      arg:	The pointer to a user space ioctl package
 *		  detailing this ioctl request (can it ever be a
 *		  kernel package ??).
 *
 *      rvalp:      The pointer to the returned data container.
 *
 *  Description:
 *      This function controls the failing of a path as directed by
 *      the administrator. This function essentially moves the
 *      specified path onto the failed queue.
 *
 *     Note: not yet debugged.
 *
 */
int
mpio_ioctl_fail_path8(vdev_p_t vdevp,void *arg,int *rvalp)
{
    int		     status = 0;
    mpio_ioctl_path_p_t   fail_pkt_ptr;
    path_p_t		    pathp;
    lpg_p_t			 lpgp;

    MPIO_DEBUG0( 0, "Enter mpio_ioctl_fail_path()" );
    
     ASSERT(vdevp);

    /*
     *  Bring the user packet into kernel space.
     */
    fail_pkt_ptr = ( mpio_ioctl_path_p_t )
	kmem_alloc(sizeof( mpio_ioctl_path_t ), KM_SLEEP);
    
    status = VCOPYIN(arg, (void *)fail_pkt_ptr, sizeof(mpio_ioctl_path_t));
    
    if  ( status == VCOPY_FAILURE ) {
	status  = MPIO_COPYIN_FAILURE;
	goto bailout;
    }
    
    /*
     *  Start processing ioctl packet.
     */
     if ( fail_pkt_ptr -> version != MPIO_IOCTL_VERSION_1) {
	status  = MPIO_INVAL_IOCTL_PKT;
	goto bailout;
     }
      
     MPIO_VDEV_LOCK(vdevp);
     pathp = mpio_vdev_get_path_by_rmkey(fail_pkt_ptr->path_rmkey, vdevp);
     if (pathp == NULL){
	  status = MPIO_INVAL_PATH ;
     	  MPIO_VDEV_UNLOCK(vdevp);
	  goto bailout;
     }
     MPIO_VDEV_UNLOCK(vdevp);

     lpgp = vdevp->lpg_vector[pathp->cpugroup].home_lpg;

     MPIO_VDEV_SLEEP_LOCK(vdevp);
     mpio_lpg_fail_path(pathp, lpgp, USER_DEMAND);
     MPIO_VDEV_SLEEP_UNLOCK(vdevp);

bailout:
    VKMEM_FREE( fail_pkt_ptr, sizeof( mpio_ioctl_path_t ) );
    return( status );

} /* mpio_ioctl_fail_path() */


/*
 *  mpio_ioctl_repair_path(dev_t dev, int arg, int *rvalp)
 *
 *      dev:	The descriptor for the device.
 *
 *      arg:	The pointer to a user space ioctl package
 *		  detailing this ioctl request (can it ever be a
 *		  kernel package ??).
 *
 *      rvalp:      The pointer to the returned data container.
 *
 *  Description:
 *
 *      This function controls the repairing of a path as directed by
 *      the administrator.
 *
 *     Note: not yet debugged.
 */

int
mpio_ioctl_repair_path8(vdev_p_t vdevp,void *arg,int *rvalp)
{
    int		     status = 0;
    mpio_ioctl_path_p_t   repair_pkt_ptr;
     path_p_t		    pathp;
     lpg_p_t			 lpgp;
     sig_compare_result_t     result;
     
    MPIO_DEBUG0( 0, "Enter mpio_ioctl_repair_path()" );

     ASSERT(vdevp);

    /*
     *  Bring the user packet into kernel space.
     */
    repair_pkt_ptr = ( mpio_ioctl_path_p_t )
	kmem_alloc(sizeof( mpio_ioctl_path_t ), KM_SLEEP);
    
    status = VCOPYIN(arg, (void *)repair_pkt_ptr,sizeof( mpio_ioctl_path_t ));
    
    if  ( status == VCOPY_FAILURE ) {
	status  = MPIO_COPYIN_FAILURE;
	goto bailout;
    }
    
    /*
     *  Process ioctl packet - here,
     */
     if ( repair_pkt_ptr -> version != MPIO_IOCTL_VERSION_1) {
	status  = MPIO_INVAL_IOCTL_PKT;
	goto bailout;
     }

    /*
     *  Validate the signature to ensure it's good.
     */
     if (!mpio_vdev_is_signature_consistent(vdevp->signature, vdevp)){
	status = MPIO_INVAL_PATH;
	goto bailout;
     }
      
     MPIO_VDEV_LOCK(vdevp);
     pathp = mpio_vdev_get_path_by_rmkey(repair_pkt_ptr->path_rmkey, vdevp);
     if (pathp == NULL){
     	  MPIO_VDEV_UNLOCK(vdevp);
	  status = MPIO_INVAL_PATH ;
	  goto bailout;
     }
     MPIO_VDEV_UNLOCK(vdevp);

     lpgp = vdevp->lpg_vector[pathp->cpugroup].home_lpg;

     MPIO_VDEV_SLEEP_LOCK(vdevp);
     mpio_lpg_repair_path(pathp, lpgp, vdevp->signature);
     MPIO_VDEV_SLEEP_UNLOCK(vdevp);

     MPIO_VDEV_LOCK(vdevp);
     if ( vdevp->state == FVS_FAILED){
	  vdevp->state = FVS_NORMAL;
     }
     MPIO_VDEV_UNLOCK(vdevp);

bailout:
    VKMEM_FREE( repair_pkt_ptr, sizeof( mpio_ioctl_path_t ) );
    MPIO_DEBUG1(0, "Leave mpio_ioctl_repair_path(), status ( %x )", status);
    return( status );

} /* mpio_ioctl_repair_path() */



/*
 *  mpio_ioctl_switch_group(dev_t dev, int arg, int *rvalp)
 *
 *      dev:	The descriptor for the device.
 *
 *      arg:	The pointer to a user space ioctl package
 *		  detailing this ioctl request (can it ever be a
 *		  kernel package ??).
 *
 *      rvalp:      The pointer to the returned data container.
 *
 *  Description:
 *
 *      This function controls the switching of I/O from one CPU
 *      group to another for a specified disk as directed by the
 *      administrator.
 *
 *     Note: not yet debugged.
 */

int
mpio_ioctl_switch_group8(vdev_p_t vdevp,void *arg,int *rvalp)
{
    int status = 0;
    mpio_ioctl_switch_group_p_t   switch_pkt_ptr;

    MPIO_DEBUG0( 0, "Enter mpio_ioctl_switch_group()" );

     ASSERT(vdevp);

    /*
     *  Bring the user packet into kernel space.
     */
    switch_pkt_ptr = ( mpio_ioctl_switch_group_p_t )
	kmem_alloc(sizeof( mpio_ioctl_switch_group_t ), KM_SLEEP);
    
    status = VCOPYIN(arg, (void *)switch_pkt_ptr,
		      sizeof( mpio_ioctl_switch_group_t ) );
    
    if ( status == VCOPY_FAILURE ) {
	status  = MPIO_COPYIN_FAILURE;
	goto bailout;
    }
    
    /*
     * Process ioctl packet - here,
     */
    
bailout:
    VKMEM_FREE( switch_pkt_ptr, sizeof( mpio_ioctl_switch_group_t ) );
    MPIO_DEBUG1( 0, "Leave mpio_ioctl_switch_group(), status ( %x )", status );
    return( status );

} /* mpio_ioctl_switch_group() */



/*
 *  mpio_ioctl_get_paths(dev_t dev, int arg, int *rvalp)
 *
 *      dev:	The descriptor for the device.
 *
 *      arg:	The pointer to a user space ioctl package
 *		  detailing this ioctl request (can it ever be a
 *		  kernel package ??).
 *
 *      rvalp:      The pointer to the returned data container.
 *
 *  Description:
 *
 *      This function gets the list of paths that the mpio driver
 *      knows about for the disk described by dev and returns as 
 *	many as the user can handle. If there are more paths than 
 *	the caller allowed for, this functions sets errno to EAGAIN.
 *
 */

int
mpio_ioctl_get_paths8(vdev_p_t vdevp,void *arg,int *rvalp)
{
    int			      status;
    mpio_ioctl_get_paths_p_t  get_paths_pkt_ptr;
    path_p_t		      pathp;
    rm_key_t	              *user_array_p;
    rm_key_t           	      path_handle;
    int		      	      i;
    int		              num_user_slots;
    int		              key;
    int		              filled_slots;

    MPIO_DEBUG0( 0, "Enter mpio_ioctl_get_vdevs()" );

     ASSERT(vdevp);

    /*
     *  Bring the user packet into kernel space.
     */
    get_paths_pkt_ptr = ( mpio_ioctl_get_paths_p_t )
	kmem_alloc(sizeof( mpio_ioctl_get_paths_t ), KM_SLEEP);
    
    status = VCOPYIN(arg, (void *)get_paths_pkt_ptr,
		      sizeof( mpio_ioctl_get_paths_t ) );
    
    if  ( status == VCOPY_FAILURE ) {
	status  = MPIO_COPYIN_FAILURE;
	goto bailout;
    }
    
    /*
     * Process ioctl packet - Check version first. Next,
     * pace vdev queue to the nth entry according to the key 
      * that the client specified in the packet.
     */
     if ( get_paths_pkt_ptr -> version != MPIO_IOCTL_VERSION_1) {
	status  = MPIO_INVAL_IOCTL_PKT;
	goto bailout;
     }
      
    /*
     * Copy out one path resmgr key at a time up to the number of slots that 
      * the user has allocated or maximum number of paths we have, whichever
      * comes first.
     */

     num_user_slots = get_paths_pkt_ptr->num_input;
     user_array_p = (rm_key_t*) get_paths_pkt_ptr->input;

     MPIO_DRIVER_LOCK();
     pathp = MPIO_QM_HEAD(&vdevp->paths);
     for (     filled_slots = 0;
	       num_user_slots && pathp != NULL ; 
	       num_user_slots--, filled_slots++, user_array_p++){

	  path_handle = pathp->devicep->sdv_handle;

	  status = VCOPYOUT(     (void *)&path_handle,
				   (void *) user_array_p,
				   sizeof(rm_key_t) );
	  if  (status == VCOPY_FAILURE ) {
	       status  = MPIO_COPYIN_FAILURE;
	       goto copyout_failure_return_label;
	  }
    
	  pathp = MPIO_QM_NEXT(&vdevp->paths, pathp);
     } /* for */

    /*
      * Copy the ioctl packet back out to user space with the
      * number of slot that we filled and the key being updated.
     */

     if ( get_paths_pkt_ptr->num_input == 0 ) {
	get_paths_pkt_ptr->num_output = vdevp->number_of_paths;
     }
     else {
	 get_paths_pkt_ptr->num_output = filled_slots;
     }
       

     status = VCOPYOUT((void *)get_paths_pkt_ptr, arg,
			      sizeof(mpio_ioctl_get_paths_t) );
     if  (status == VCOPY_FAILURE ) {
	  status  = MPIO_COPYIN_FAILURE;
	  goto copyout_failure_return_label;
     }

    /*
     * If more paths to go, but the user didn't give us enough
     * buffer to hold all of them, return EAGAIN so that the user
     * come back for more.
     */
    if (num_user_slots == 0 && pathp != NULL){
    	status = EAGAIN;
    } else {
    	status = 0;
    }

copyout_failure_return_label:
     MPIO_DRIVER_UNLOCK();

bailout:
    VKMEM_FREE( get_paths_pkt_ptr, sizeof( mpio_ioctl_get_paths_t ) );
    MPIO_DEBUG1( 0, "Leave mpio_ioctl_get_paths(), status ( %x )", status );
    return( status );

} /* mpio_ioctl_get_paths() */

/*
 *  mpio_ioctl_get_path_info(dev_t dev, int arg, int *rvalp)
 *
 *      dev:	The descriptor for the device.
 *
 *      arg:	The pointer to a user space ioctl package
 *		  detailing this ioctl request (can it ever be a
 *		  kernel package ??).
 *
 *      rvalp:      The pointer to the returned data container.
 *
 *  Description:
 *
 *      This function gets all of the information about a particular
 *      path.
 *
 *     Note: not yet debugged.
 *
 */

int
mpio_ioctl_get_path_info8(vdev_p_t vdevp,void *arg,int *rvalp)
{
    int	status = 0;
    mpio_ioctl_get_path_info_p_t  get_path_info_pkt_ptr;
    path_p_t			      pathp;

    MPIO_DEBUG0( 0, "Enter mpio_ioctl_get_path_info()" );

    ASSERT(vdevp);

    /*
     *  Bring the user packet into kernel space.
     */
    get_path_info_pkt_ptr = ( mpio_ioctl_get_path_info_p_t )
	kmem_alloc( sizeof( mpio_ioctl_get_path_info_t ), KM_SLEEP);
    
    status = VCOPYIN(arg, (void *)get_path_info_pkt_ptr,
		      sizeof( mpio_ioctl_get_path_info_t ) );
    
    if  (status == VCOPY_FAILURE) {
	status  = MPIO_COPYIN_FAILURE;
	goto bailout;
    }
    
    /*
     * Process ioctl packet - here,
     */
     if ( get_path_info_pkt_ptr -> version != MPIO_IOCTL_VERSION_1) {
	status  = MPIO_INVAL_IOCTL_PKT;
	goto bailout;
     }
      
     /*
      * NOTE: make sure after integration with SCO. We have this logic
      * worked out. Basically we need to agree upon the returned data
      * type of the paths of the vdev in get_vdevs
      */
     MPIO_VDEV_LOCK(vdevp);

     pathp = mpio_vdev_get_path_by_rmkey(get_path_info_pkt_ptr->path_rmkey, vdevp);
     if (pathp == NULL){
	  status = MPIO_INVAL_PATH ;
	  goto unknown_path;
     }
     
     get_path_info_pkt_ptr->state = pathp->state;
     get_path_info_pkt_ptr->locale = pathp->cpugroup;
     get_path_info_pkt_ptr->num_reads = pathp->acct.num_reads;
     get_path_info_pkt_ptr->num_writes = pathp->acct.num_writes;

    /*
     *  Copy the updated packet back into user space.
     */
    status = VCOPYOUT((void *)get_path_info_pkt_ptr, arg,
		       sizeof( mpio_ioctl_get_path_info_t ) );
    
    if  ( status == VCOPY_FAILURE ) {
	status  = MPIO_COPYIN_FAILURE;
    }

     
unknown_path:
     MPIO_VDEV_UNLOCK(vdevp);

bailout:
    VKMEM_FREE( get_path_info_pkt_ptr, sizeof(mpio_ioctl_get_path_info_t) );
    MPIO_DEBUG1( 0, "Leave mpio_ioctl_get_path_info(), status ( %x )", status );
    return( status );

} /* mpio_ioctl_get_path_info() */

/**** end mpio_ioctl.c ****/
