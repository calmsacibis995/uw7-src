#ident	"@(#)kern-pdi:io/layer/mpio/mpio.c	1.1.13.2"
#ident	"$Header$"

/*
 *  This module contains the MPIO driver wrapper layer for UnixWare Gemini 
 *  release.
 *    
 */

#define	_DDI	8

#include    <util/types.h>
#include    <util/mod/moddefs.h>    /* for DLM		     */
#include    <io/conf.h>	     /* for devflag		 */
#include    <util/cmn_err.h>       
#include    <svc/errno.h>
#include    <util/debug.h>

#include    "mpio.h"
#include    "mpio_ioctl.h"
#include    "mpio_debug.h"
#include    "mpio_proto.h"

#include    <io/ddi.h>		/* must come last		*/

void    mpiostart();
int	mpioopen(dev_t *, int, int, struct cred *);
int	mpioclose(dev_t, int, int, struct cred *);
int	mpioioctl(dev_t, int, int, int, cred_t *, int *);
void    mpiostrategy(buf_t *);

/*
 * DDI8 entries.
 */
int
	mpio_config_entry(cfg_func_t, void *, rm_key_t),
	mpioopen8(void *, ulong_t *, int, cred_t *, queue_t *),
	mpioclose8(void *, ulong_t, int, cred_t *, queue_t *),
	mpiodevinfo8(void *, ulong_t, di_parm_t, void *),
	mpioioctl8(void *, ulong_t, int, void *, int, cred_t *, int *),
	mpiodevctl8(void *, ulong_t, int, void *);

void
	mpiobiostart8(void *, ulong_t, buf_t *);

/*
 * core entries.
 */
extern status_t
	mpio_open8(void *, ulong_t *, int, cred_t *, queue_t *),
	mpio_close8(void *, ulong_t, int, cred_t *, queue_t *);
extern void err_log0(int),
	err_log1(int,int),
	err_log2(int,int,int),
	err_log3(int,int,int,int);

/*
 *  Local functions
 */
STATIC    void mpioinit(void);
#ifdef notyet
STATIC    void mpiostop(void);
#endif

extern mpio_driver_info_t *	dip;
int mpiodevflag = D_NOBRKUP | D_MP | D_BLKOFF;  
    /*
      *  The devflag is PDI required driver parameter to
     *  specify the driver's characteristics to the system.
     */
extern char Mpio_modname[];		/* defined in Space.c */

drvops_t	mpiodrvops = {
	mpio_config_entry,
	mpioopen8,
	mpioclose8,
	mpiodevinfo8,
	mpiobiostart8,
	mpioioctl8,
	mpiodevctl8,
	NULL };		/* no mmap entry point */

const drvinfo_t	mpiodrvinfo = {
	&mpiodrvops, Mpio_modname, D_MP, NULL, 0 };

/*
 * int
 * mpio_load(void)
 *
 * Calling/Exit State:
 *    ...
 *
 * Description:
 *    Called by DLM when the module is loaded.
 *   
 * Note: ...
 *
 */
int
_load(void)
{
    MPIO_DEBUG0(0,"Enter mpio_load.");
	drv_attach(&mpiodrvinfo);
    mpiostart();
    return(0);
}

/*
 * int
 * mpio_unload(void)
 *
 * Calling/Exit State:
 *    ...
 *
 * Description:
 *    Called by DLM when the module is unloading.
 *   
 * Note: ...
 *
 */
int
_unload(void)
{
    MPIO_DEBUG0(0,"Enter mpio_unload.");
#ifdef notyet
    mpiostop();
    return(0);
#else
    return EBUSY;
#endif
}

/*
 * void
 * mpiostart(void)
 *
 * Calling/Exit State:
 *    ...
 *
 * Description:
 *    Called by DLM when the module is loaded.
 *   
 * Note: ...
 */
#include <util/kdb/xdebug.h>
void
mpiostart(void)
{
    int			i;

    MPIO_DEBUG0(0,"Enter mpiostart.");

    /*
     *  Init the driver.
     */
    mpio_start();
    mpioinit();
}
/* END mpiostart */

#ifdef notyet
/*
 * void
 * mpiostop(void)
 *
 * Calling/Exit State:
 *    ...
 *
 * Description:
 *    
 *   
 * Note: ...
 */
void
mpiostop(void)
{
    MPIO_DEBUG0(0,"Enter mpiostop.");

    /*
     *  Stop the MPIO core and unwind the SDI stuffs.
     */

    if ( dip != NULL && dip->my_layer != NULL){
	sdi_driver_deregister(dip->my_layer);
    }

    if ( dip != NULL && dip->desc != NULL){
	sdi_driver_desc_free(dip->desc);
    }

    mpio_stop();
}
#endif /* notyet */

/*
 * int
 * mpioopen(dev_t *fmdev_p, int flag, int type, struct cred *cred_p)
 *
 * Calling/Exit State:
 *    ...
 *
 * Description:
 *    
 *   
 * Note:
 *    The MPIO does not export device nodes directly to the user space.
 *     All access must be directed to the slice driver where permission
 *     is checked.
 */
/*ARGSUSED*/
int
mpioopen(dev_t *devp, int flag, int type, struct cred *credp)
{
    ASSERT(0);
}

/*ARGSUSED*/
int
mpioopen8(void *idatap, ulong_t *channelp, int flags, cred_t *credp, queue_t *dummy_arg)
{
    int status;

    MPIO_DEBUG3(0,"mpioopen8: idatap 0x%x channel %d flags 0x%x\n", idatap, *channelp, flags);


    /*
     * Map error code from MPIO to UnixWare error code.
     */
    status = (mpio_open8(idatap, channelp, flags, credp, dummy_arg) ==  OK)? 0 : ENODEV;
    return (status);
}


/*
 * int
 * mpioclose(dev_t dev, int flags, int otype, struct cred *cred_p)
 *
 * Calling/Exit State:
 *    ...
 *
 * Description:
 *    
 *   
 * Note: ...
 */
/*ARGSUSED*/
int
mpioclose(dev_t dev, int flags, int otype, struct cred *cred_p)
{
    ASSERT(0);
}

/*ARGSUSED*/
int
mpioclose8(void *idatap, ulong_t channel, int flags, cred_t *cred_p, queue_t *dummy_arg)
{
    int	status;

    MPIO_DEBUG0(0,"Enter mpioclose.");

    /*
     * Map error code from MPIO to UnixWare error code.
     */
    status = (mpio_close8(idatap, channel, flags, cred_p, dummy_arg) ==  OK) ?
             0 : ENODEV;
    return (status);
}

int
mpioioctl(dev_t dev, int cmd, int arg, int mode, cred_t *cred_p, int *rval_p)
{
    ASSERT(0);
}

/*ARGSUSED*/
int
mpioioctl8(void *idatap, ulong_t channel, int cmd, void *arg, int mode, cred_t *cred_p, int *rval_p)
/* NOTE: this function should never be called with a NULL idatap */
{
    int ret_val;
    vdev_p_t    vdevp;
    lpg_p_t	lpgp;
    path_p_t    pathp;
    
    MPIO_DEBUG0(0,"Enter mpioioctl8");

	vdevp = (vdev_p_t)idatap;
	ASSERT(idatap);

    switch (cmd) {
	case MPIO_IOCTL_FAIL_PATH:
	    if (ret_val = drv_priv(cred_p)) {
		return ret_val;
	    }
	    return (mpio_ioctl_fail_path8(vdevp, arg, rval_p));
	    break;

	case MPIO_IOCTL_REPAIR_PATH:
	    if (ret_val = drv_priv(cred_p)) {
		return ret_val;
	    }
	    return (mpio_ioctl_repair_path8(vdevp, arg, rval_p));
	    break;

	case MPIO_IOCTL_SWITCH_GROUP:
	    if (ret_val = drv_priv(cred_p)) {
		return ret_val;
	    }
	    return (mpio_ioctl_switch_group8(vdevp, arg, rval_p));
	    break;

	case MPIO_IOCTL_GET_PATHS:
	    if (ret_val = drv_priv(cred_p)) {
		return ret_val;
	    }
	    return (mpio_ioctl_get_paths8(vdevp, arg, rval_p));
	    break;

	case MPIO_IOCTL_GET_PATH_INFO:
	    if (ret_val = drv_priv(cred_p)) {
		return ret_val;
	    }
	    return (mpio_ioctl_get_path_info8(vdevp, arg, rval_p));
	    break;

	case MPIO_IOCTL_TUNEABLE:
	    if (ret_val = drv_priv(cred_p)) {
		return ret_val;
	    }
	    return (mpio_ioctl_tuneable((int)arg, rval_p));
	    break;

        case MPIO_IOCTL_INSERT_ERROR:
            if (ret_val = drv_priv(cred_p)) {
                return ret_val;
            }
            return (mpio_ioctl_insert_error8(idatap, arg, rval_p));
            break;

	default:
	    MPIO_VDEV_LOCK(vdevp);
/*
 *	    lpgp = vdevp->lpg_vector[0].working_lpg;
 *	    pathp = mpio_lpg_get_active_path(lpgp);
 */

            pathp = mpio_get_best_path( NULL, vdevp );

	    MPIO_VDEV_UNLOCK(vdevp);
	    if ( pathp == NULL){
		return ENODEV;
	    }
	    return(MPIO_KERNEL_IOCTL8(pathp->devicep, cmd, arg, mode, cred_p, rval_p));
	    break;
    }
} /* end mpioioctl8 */

/*ARGSUSED*/
int
mpiodevctl8(void *idatap, ulong_t channel, int cmd, void *arg)
/* NOTE: this function should never be called with a NULL idatap */
{
    vdev_p_t    vdevp;
    lpg_p_t	lpgp;
    path_p_t    pathp;

    MPIO_DEBUG0(0,"Enter mpiodevctl8");

    vdevp = (vdev_p_t)idatap;

    ASSERT(vdevp);

	MPIO_VDEV_LOCK(vdevp);
/*
 *	lpgp = vdevp->lpg_vector[0].working_lpg;
 *	pathp = mpio_lpg_get_active_path(lpgp);
 */

        pathp = mpio_get_best_path( NULL, vdevp );

	MPIO_VDEV_UNLOCK(vdevp);

	if ( pathp == NULL){
		return ENODEV;
	}

	return(MPIO_KERNEL_DEVCTL8(pathp->devicep, cmd, arg));

} /* end mpiodevctl8 */

void
mpiostrategy(buf_t *bp)
{
    ASSERT(0);
}

/*ARGSUSED*/
int
mpiodevinfo8(void *idata, ulong_t channel, di_parm_t parm, void *valp)
{
    vdev_p_t	vdevp = (vdev_p_t)idata;
    path_p_t    pathp;

    MPIO_DEBUG0(0,"Enter mpiodevinfo8");

    ASSERT(idata);

    switch (parm) {
        case DI_MEDIA:
            *(char **)valp = "path";
            return 0;

	default:
            MPIO_VDEV_LOCK(vdevp);
/*
 *          pathp = mpio_lpg_get_active_path(vdevp->lpg_vector[0].working_lpg);
 */
            pathp = mpio_get_best_path( NULL, vdevp );

            MPIO_VDEV_UNLOCK(vdevp);

            if  (!pathp)
                return ENODEV;

            return(MPIO_KERNEL_DEVINFO8(pathp->devicep, parm, valp));
    }
}

/*
 * void
 * mpioinit(void)
 *
 * Calling/Exit State:
 *    ...
 *
 * Description:
 *    
 *   
 * Note: ...
 * 
 */
void
mpioinit(void)
{
    sdi_driver_desc_t *	    mpio_descp;

    MPIO_DEBUG0(0,"Enter mpioinit");

    /*
     * Register this driver with SDI.
     */
    mpio_descp = sdi_driver_desc_alloc (sdi_sleepflag);
    if (mpio_descp != NULL){
	dip->desc = mpio_descp;
	(void)strcpy(mpio_descp->sdd_modname, Mpio_modname);
	mpio_descp->sdd_precedence = SDI_STACK_GAMMA;
	mpio_descp->sdd_dev_cfg = MPIO_dev_cfg;
	mpio_descp->sdd_dev_cfg_size = MPIO_dev_cfg_size;
	mpio_descp->sdd_config_entry = mpio_config_entry;
	mpio_descp->sdd_minors_per = 1;
	if (sdi_driver_desc_prep(mpio_descp, sdi_sleepflag)){
	    dip->my_layer = sdi_driver_add(mpio_descp, sdi_sleepflag);
	    if ( dip->my_layer == NULL){
		goto driver_register_failure;
	    }
	}
    }
    goto done;
    /* NOTREACH */

driver_register_failure:
    sdi_driver_desc_free(mpio_descp);
dip_alloc_failure:
    err_log0(MPIO_MSG_DRV_INIT_FAILURE);
done:
    return;
}

/*  
 *
 *
 */
void
mpio_debug_trap()
{
#ifdef    MPIO_DEBUG
    (*cdebugger) (DR_OTHER, NO_FRAME);
#endif

}

/*  
 * mpio_config_entry(cfg_func_t, void *, rm_key_t)
 * 
 * Description:
 *    This function is called by the SDI to inform the MPIO that
 *    a device (a path actually) needs service. Service types are
 *    add, delete and change status.
 */
/*ARGSUSED*/
int
mpio_config_entry(cfg_func_t reason, void *idata, rm_key_t rmkey)
{
    int	    status = 0;

    switch (reason) {
	case CFG_ADD:
	    status = mpio_configure_path(idata, rmkey);
	    break;

	case CFG_REMOVE:
	    status = mpio_deconfigure_path(idata);
	    break;
#ifdef notyet
	case CFG_MODIFY:
	case CFG_VERIFY:
	case CFG_RESUME:
	case CFG_SUSPEND:
	    /*
	     *  TBD
	     */
	    status = B_FALSE; 
	    break;
#endif
	default:
#ifdef DEBUG
	    err_log0(MPIO_MSG_CONFIG_UNKNOWN_EVENT);
cmn_err(CE_NOTE, "mpio: invalid instance command %x\n", reason);
#endif
	    status = EOPNOTSUPP;
	    break;
    }

    return status;
}

/*  
 * sdi_device_t*
 * mpio_register_device()
 *
 * Description:
 *    This function is called by the MPIO core when it wants to
 *    register a new Vdev with the kernel.
 */
sdi_device_t*
mpio_register_device(sdi_device_t *newpath, vdev_p_t vdevp, rm_key_t rmkey)
{
    sdi_device_t *    sdi_device;

    sdi_device = sdi_device_alloc (sdi_sleepflag);
    if (sdi_device == NULL){
	goto device_alloc_failure;
    }
    else {
        sdi_device->sdv_layer = dip->my_layer;

        sdi_device->sdv_parent_handle = rmkey;
        sdi_device->sdv_driver = dip->desc;

        sdi_device->sdv_order   = newpath->sdv_order;
        sdi_device->sdv_unit    = newpath->sdv_unit;
        sdi_device->sdv_bus     = newpath->sdv_bus;
        sdi_device->sdv_target  = newpath->sdv_target;
        sdi_device->sdv_lun     = newpath->sdv_lun;
        sdi_device->sdv_devtype = newpath->sdv_devtype;
        sdi_device->sdv_bcbp    = newpath->sdv_bcbp;
        sdi_device->sdv_cgnum   = newpath->sdv_cgnum;

        bcopy(newpath->sdv_inquiry, sdi_device->sdv_inquiry, INQ_EXLEN);
        bcopy(&(newpath->sdv_stamp), &(sdi_device->sdv_stamp), sizeof(struct pd_stamp));
 
	sdi_device->sdv_idatap = (void *)vdevp;
	sdi_device->sdv_drvops = &mpiodrvops;

	if (sdi_device_prep(sdi_device, sdi_sleepflag)) {
	    sdi_device->sdv_state = SDI_DEVICE_EXISTS | SDI_DEVICE_ONLINE;
	    if ( !sdi_device_add(sdi_device, sdi_sleepflag) ) {
		goto device_add_failure;
	    }
	}
	else { /* invalid device descriptor */
	    goto invalid_device_descriptor;
	}
    }
    goto done;
    /* NOTREACH */

device_add_failure:
invalid_device_descriptor:
    sdi_device_free(sdi_device);
    sdi_device = NULL;

device_alloc_failure:
    err_log0(MPIO_MSG_DEV_REGIS_FAILURE);
    sdi_device = NULL;

done:
    return (sdi_device);
}

/*  
 * mpio_deregister_device()
 *
 * Description:
 *    This function is called by the MPIO core when it wants to
 *     inform the kernel that the device is nolonger under its 
 *    jurisdiction.
 */
void
mpio_deregister_device(sdi_device_t *devicep)
{
    sdi_device_rm(devicep->sdv_handle, KM_NOSLEEP);
    sdi_device_free(devicep);
}

/* END driver.c */
