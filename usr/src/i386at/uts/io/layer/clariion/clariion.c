#ident	"@(#)kern-pdi:io/layer/clariion/clariion.c	1.2.12.2"
#ident	"$Header$"

/*    Copyright (c) 1996 Data General Corp. All Rights Reserved. */

/*
 *            CLARIION - CLARiiON Multi-ported driver.
 *           (Applicable to FLARE code rev 7.50 or newer)
 *
 * WANING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 * This module contains Data General CLARiiON proprietery information.
 *
 * This driver provides the following services via its ioctl(2) interface:
 *        MP_IOCTL_TRESPASS - trespass a LUN.
 *        MP_IOCTL_GET_MULTI_PORT_INFO - get signature and state.
 *
 * CLARIION does not support normal I/O request ie. read/write.
 *
 * The CLARIION driver sits directly on top of the target driver layer,
 * for it must communicate directly with the CLARiiON H/W via pass-thru
 * channel.  The pass-thru channel has not been defined at this moment.
 *
 *
 *         *---------------*
 *         |  MPIO         |
 *         |               |
 *         *---------------*
 *         | |   |   |
 *         | |   |   |
 *         | | *-v---v-*
 *         | | |   MP  |
 *         | | |       |
 *         | | *-------*
 *         | |   |   |
 *        *v-v---v---v----*
 *        |    TARGET     |
 *        |               |
 *        *---------------*
 *               |     
 *        *------v--------*
 *        |     HBA       |
 *        |               |
 *        *---------------*
 *
 */

#define	_DDI	8

#include    <util/types.h>
#include    <util/mod/moddefs.h>    /* for DLM                     */
#include    <io/conf.h>             /* for devflag                 */
#include    <util/cmn_err.h>       
#include    <svc/errno.h>
#include    <util/debug.h>
#include    <util/sysmacros.h>
#include    <proc/cred.h>
#include    <io/target/sdi/sdi_comm.h>
#include    <io/target/sdi/sdi_edt.h>
#include    <io/target/sdi/sdi_layer.h>


#include    "clariion.h"
#include    "clariion_debug.h"

#include    <io/ddi.h>        /* must come last              */

/*
 * DDI8 entries.
 */

STATIC void clariionbiostart(void *, ulong_t, buf_t *);

int	clariionioctl(void *, ulong_t, int, void *, int, cred_t *, int *),
	clariionopen(void *, ulong_t *, int , cred_t *, queue_t *),
	clariionclose(void *, ulong_t , int , cred_t *, queue_t *),
	clariiondevinfo(void *, ulong_t , di_parm_t, void *),
	clariiondevctl(void *, ulong_t , int , void *),
	clariion_config_entry(cfg_func_t , void * , rm_key_t );

/*
 *  Local functions
 */
STATIC void clariioninit(void),
	clariionstart(void),
	clariionstop(void),
	clariion_deconfigure_path(void *);

STATIC int clariion_get_signature(path_p_t, void*),
	clariion_get_mp_info(path_p_t, get_mp_info_t *),
	clariion_trespass(path_p_t),
	clariion_configure_path(void *, rm_key_t);

STATIC sdi_device_t* clariion_register_device(sdi_device_t*, path_p_t, rm_key_t);
STATIC boolean_t clariion_is_clariion(sdi_device_t*);

clariion_driver_info_p_t    clariion_dip;        /* driver info    */
clariion_qm_anchor_t                 clariion_path_queue; /* Path queue     */

int clariiondevflag = D_NOBRKUP | D_MP | D_BLKOFF;  
extern char CLARIION_modname[];		/* defined in Space.c */

drvops_t	clariiondrvops = {
	clariion_config_entry,
	clariionopen,
	clariionclose,
	clariiondevinfo,
	clariionbiostart,
	clariionioctl,
	clariiondevctl,
	NULL };		/* no mmap entry point */

const drvinfo_t	clariiondrvinfo = {
	&clariiondrvops, CLARIION_modname, D_MP, NULL, 0 };

LKINFO_DECL(clariion_info_lkinfo,"clariion: driver info", 0);

/*
 * Error message dictionary.
 */
typedef struct{
    int        level;        /* of seriousness                 */
    char *    msg;        /* actual error message         */
} clariion_msg_t, *clariion_msg_p_t;

STATIC clariion_msg_t    clariion_english_dictionary[] = {
  {  0,        0 },
  {  CE_WARN,  "clariion: failed to init driver"},                      /*001*/
  {  CE_WARN,  "clariion: failed to register path with SDI"},           /*002*/
  {  CE_WARN,  "clariion: config enters with an unknown event"},        /*003*/
  {  CE_NOTE,  "!clariion: start driver"},                              /*004*/
  {  CE_NOTE,  "!clariion: stop driver"},                               /*005*/
  {  CE_WARN,  "clariion: path %d is not a CLARiiON device"},           /*006*/
  {  0,        0 }
};

/*
 * The current error dictionary pointer.
 */
STATIC clariion_msg_p_t clariion_dictionary_p = clariion_english_dictionary;

/*
 * int
 * clariion_load(void)
 *
 * Calling/Exit State:
 *    ...
 *
 * Description:
 *    Called by DLM when the CLARIION is to be loaded.
 *   
 * Note: ...
 *
 */
int
_load(void)
{
    CLARIION_DEBUG0(0,"Enter clariion_load");
	drv_attach(&clariiondrvinfo);
    clariionstart();
    return(0);
}

/*
 * int
 * clariion_unload(void)
 *
 * Calling/Exit State:
 *    ...
 *
 * Description:
 *    Called by DLM when the CLARIION is to be unloaded.
 *   
 * Note: ...
 *
 */
int
_unload(void)
{
    CLARIION_DEBUG0(0,"Enter clariion_unload.");
#ifdef notyet
    clariionstop();
    return(0);
#else
	return(EBUSY);
#endif
}


/*
 * void
 * clariionstart(void)
 *
 * Calling/Exit State:
 *    ...
 *
 * Description:
 *    Called by DLM when the module is loaded.
 *   
 * Note: ...
 */
void
clariionstart(void)
{
    int                        i;
    sdi_driver_desc_t *        clariion_descp;

    CLARIION_DEBUG0(0,"Enter clariionstart.");

    /*
     * Init clariion
     */
    err_log0(CLARIION_MSG_START);
    clariion_dip = (void *)kmem_zalloc(sizeof(clariion_driver_info_t), sdi_sleepflag);
    if (clariion_dip == NULL){
        goto dip_alloc_failure;
    }
    CLARIION_INIT_DRIVER_INFO(clariion_dip);

    clariion_qm_initialize(&clariion_path_queue, offsetof(path_t,links));

    /*
     * Register clariion with SDI.
     */
    clariion_descp = sdi_driver_desc_alloc (sdi_sleepflag);
    if (clariion_descp != NULL){
        clariion_dip->desc = clariion_descp;
		(void)strcpy(clariion_descp->sdd_modname, CLARIION_modname);
        clariion_descp->sdd_precedence = SDI_STACK_BETA;
        clariion_descp->sdd_dev_cfg = CLARIION_dev_cfg;
        clariion_descp->sdd_dev_cfg_size = CLARIION_dev_cfg_size;
        clariion_descp->sdd_config_entry = clariion_config_entry;
        clariion_descp->sdd_minors_per = 1;
        if (sdi_driver_desc_prep(clariion_descp, sdi_sleepflag)){
            clariion_dip->my_layer = sdi_driver_add(clariion_descp, 
						    sdi_sleepflag);
            if ( clariion_dip->my_layer == NULL){
                goto driver_register_failure;
            }
        }
    }
    goto done;
    /* NOTREACH */

driver_register_failure:
    sdi_driver_desc_free(clariion_descp);
    kmem_free(clariion_dip, sizeof(clariion_driver_info_t));

dip_alloc_failure:
    err_log0(CLARIION_MSG_DRV_INIT_FAILURE);

done:
    return;

}

#ifdef notyet
/*
 * void
 * clariionstop(void)
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
clariionstop(void)
{
    path_p_t    pathp;
    int            status;

    CLARIION_DEBUG0(0,"Enter clariionstop.");
    err_log0(CLARIION_MSG_STOP);


    /*
     *  Purge Paths.
     */
    while ((pathp = CLARIION_QM_HEAD(&clariion_path_queue)) != NULL){
    	sdi_device_rm(pathp->mydevicep->sdv_handle, KM_NOSLEEP);
        clariion_qm_remove(&clariion_path_queue,pathp);
		sdi_device_free(pathp->mydevicep);
		kmem_free(pathp->devicep, sizeof(sdi_device_t));
        kmem_free(pathp, sizeof(path_t));
    }

    /*
     * SDI deregistration 
     */
    if (clariion_dip != NULL && clariion_dip->my_layer != NULL){
        sdi_driver_deregister(clariion_dip->my_layer);
    }
    if (clariion_dip != NULL && clariion_dip->desc != NULL){
        sdi_driver_desc_free(clariion_dip->desc);
    }

    /*
     * Delete the driver info block.
     */
    CLARIION_DEINIT_DRIVER_INFO(clariion_dip);
    kmem_free(clariion_dip, sizeof(clariion_driver_info_t));

done:
    return;
}
#endif

/*
 * int clariiondevctl()
 *
 * Description:
 *    This is the front-end processor of devctl calls. If the devctl is not
 *  understood by CLARIION, it will be passed down to the next layer for it
 *  to decide.
 *
 * Calling/Exit State:
 *
 * Note: ...
 */
int
clariiondevctl(void *pathp, ulong_t channel, int cmd, void *arg)
{
	int	status;

	CLARIION_DEBUG0(0,"Enter clariiondevctl");

	switch (cmd) {
	case SDI_DEVICE_TRESPASS:
		status = clariion_trespass_path((path_p_t)pathp);
		break;

	case SDI_GET_SIGNATURE:
		status = clariion_get_signature((path_p_t)pathp, arg);
		break;

	default:
		return((((path_p_t)pathp)->devicep->sdv_drvops->d_drvctl) \
			(((path_p_t)pathp)->devicep->sdv_idatap, channel, cmd, arg));
	}

	/*
	 *  only interpret return values for our own functions
	 */
	if ( !status )
		return 0;
	else
		return ENODEV;
}

/*
 * int clariiondevinfo()
 *
 * Description:
 *    pass the ioctl thru to the next level after translating the idatap
 */
int
clariiondevinfo(void *pathp, ulong_t channel, di_parm_t cmd, void *arg)
{
    return((((path_p_t)pathp)->devicep->sdv_drvops->d_devinfo) \
	(((path_p_t)pathp)->devicep->sdv_idatap, channel, cmd, arg));
}

/*
 * int clariionioctl()
 *
 * Description:
 *    pass the ioctl thru to the next level after translating the idatap
 */
int
clariionioctl(void *pathp, ulong_t channel, int cmd, void *arg, int mode, cred_t *cred_p, int *rval_p)
{
    return((((path_p_t)pathp)->devicep->sdv_drvops->d_ioctl) \
	(((path_p_t)pathp)->devicep->sdv_idatap, channel, cmd, arg, mode, cred_p, rval_p));
}

/*
 * int clariionopen()
 *
 * Description:
 *    pass the open thru to the next level after translating the idatap
 */
int
clariionopen(void *pathp, ulong_t *channel, int cmd, cred_t *cred_p, queue_t *q)
{
    return((((path_p_t)pathp)->devicep->sdv_drvops->d_open) \
	(((path_p_t)pathp)->devicep->sdv_idatap, channel, cmd, cred_p, q));
}

int
clariionstrategy(void *pathp, ulong_t *channel, int cmd, cred_t *cred_p, queue_t *q)
{
	ASSERT(0);
}

/*
 * int clariionclose()
 *
 * Description:
 *    pass the close thru to the next level after translating the idatap
 */
int
clariionclose(void *pathp, ulong_t channel, int cmd, cred_t *cred_p, queue_t *q)
{
    return((((path_p_t)pathp)->devicep->sdv_drvops->d_close) \
	(((path_p_t)pathp)->devicep->sdv_idatap, channel, cmd, cred_p, q));
}

/*
 * void clariionbiostart()
 *
 * Description:
 *    pass the biostart thru to the next level after translating the idatap
 */
void
clariionbiostart(void *pathp, ulong_t channel, buf_t *buf)
{
    (((path_p_t)pathp)->devicep->sdv_drvops->d_biostart) \
	(((path_p_t)pathp)->devicep->sdv_idatap, channel, buf);
	return;
}

/*  
 * clariion_config_entry(rm_key_t rmkey, uchar_t reason)
 * 
 * Description:
 *    This function is called by the SDI to inform the CLARIION that
 *    a device (a path actually) needs service. Service types are
 *    add, delete and change status.
 */
int
clariion_config_entry(cfg_func_t reason, void *idata, rm_key_t rmkey)
{
	boolean_t    status = 0;

	switch (reason) {
	case CFG_ADD:
		clariion_configure_path(idata, rmkey);
		break;

	case CFG_REMOVE:
		clariion_deconfigure_path(idata);
		break;
#ifdef notyet
	case CFG_MODIFY:
	case CFG_VERIFY:
	case CFG_RESUME:
	case CFG_SUSPEND:
	/*
	*  TBD
	*/
		status = ENXIO; 
		break;
#endif
	default:
#ifdef DEBUG
		err_log0(CLARIION_MSG_CONFIG_UNKNOWN_EVENT);
#endif
		status = EOPNOTSUPP;
		break;
	}

	return status;
}

/*  
 * clariion_register_device()
 *
 * Description:
 *    This function is called by the CLARIION when it wants to
 *    register a new Path with the kernel.
 */
sdi_device_t *
clariion_register_device(sdi_device_t *newpath, path_p_t pathp, rm_key_t rmkey)
{
	sdi_device_t	*sdi_device;

	if ((sdi_device = sdi_device_alloc(sdi_sleepflag))){
		sdi_device->sdv_layer   = clariion_dip->my_layer;

		sdi_device->sdv_parent_handle   = rmkey;
		sdi_device->sdv_driver  = clariion_dip->desc;

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

		sdi_device->sdv_idatap = (void *)pathp;
		sdi_device->sdv_drvops = &clariiondrvops;

		if (sdi_device_prep(sdi_device, sdi_sleepflag)) {
			sdi_device->sdv_state = SDI_DEVICE_EXISTS | SDI_DEVICE_ONLINE;
			if (sdi_device_add(sdi_device, sdi_sleepflag)) {
				return (sdi_device);
			}
		}

		sdi_device_free(sdi_device);
	}

	err_log0(CLARIION_MSG_DEV_REGIS_FAILURE);
	return NULL;
}

/*
 * int
 * clariion_configure_path(rmkey_t rmkey)
 *
 * rmkey:    Pointer to a to-be registered device.
 *
 * Description:
 *    Create a new Path and put it under CLARIION jurisdiction.
 *
 * Calling/Exit State:
 *   
 */
int
clariion_configure_path(void *idatap, rm_key_t rmkey)
{
    int             status = 0;
    int			    rval;
    path_p_t        pathp;
    sdi_device_t    *rdevp;

    rdevp = kmem_zalloc(sizeof(sdi_device_t), sdi_sleepflag);
    if  ( !sdi_dev_read_opr( rmkey, rdevp ) ) {
        goto bailout;
    }

    if  ( !clariion_is_clariion( rdevp ) ) {
        goto bailout;
    }

    /*
     * Create and init a Path object.
     */
    pathp = kmem_zalloc (sizeof(path_t), sdi_sleepflag);
    if  ( pathp == NULL ){
        goto bailout;
    }

    pathp->open_count = 0;

    CLARIION_LOCK();
    clariion_qm_add_tail(&clariion_path_queue, pathp);
    CLARIION_UNLOCK();

    *(void **)idatap = (void *)pathp;

    pathp->devicep = rdevp;
    pathp->mydevicep = clariion_register_device(rdevp, pathp, rmkey);

    goto done;

bailout:
    status = ENXIO;
    kmem_free(rdevp, sizeof(sdi_device_t));

done:
    return status;
}

/*
 * void
 * clariion_deconfigure_path(rm_key_t)
 *
 * Description:
 *    Remove a path from CLARIION jurisdiction and delete the Path.
 *
 * Calling/Exit State:
 *   
 * Note:
 *    This routine assumes the caller has done all necessary actions
 *    to stop I/O and close the path before calling this function. If
 *  the open_count is not zero, the routine will fail.
 * 
 */
void
clariion_deconfigure_path(void *vpathp)
{
    path_p_t                pathp = vpathp;

    sdi_device_rm(pathp->mydevicep->sdv_handle, KM_NOSLEEP);

    CLARIION_LOCK();

    /*
     *  purge the Path.
     */
    clariion_qm_remove(&clariion_path_queue,pathp);

    sdi_device_free(pathp->mydevicep);
    kmem_free(pathp->devicep, sizeof(sdi_device_t));
    kmem_free(pathp, sizeof(path_t));

    CLARIION_UNLOCK();

    return;
}

/*
 * boolean_t
 * clariion_is_clariion()
 *
 * Description:
 *    Issue an Inquiry command and check for CLARiiON vendor signature.
 *
 * Calling/Exit State:
 *   
 * Note:
 * 
 */
boolean_t 
clariion_is_clariion(sdi_device_t *devicep)
{
    struct scs          *cdb6;
    sdi_devctl_t	devctl;
    struct sauna_inq    *sauna;
    physreq_t		*preqp;
    int			status = B_TRUE;

    if  ( !devicep ) {
        return B_FALSE;
    }

    sauna = (struct sauna_inq *)sdi_kmem_alloc_phys(sizeof(struct sauna_inq),
                devicep->sdv_bcbp->bcb_physreqp, 0);

    cdb6 = (struct scs *)sdi_kmem_alloc_phys(sizeof(struct scs),
		devicep->sdv_bcbp->bcb_physreqp, 0);

    /*
     * Issue an INQUIRY command and look for the string "SAUNA"
     * in the returned INQUIRY data.
     */
    
    cdb6->ss_op = SS_INQUIR;
    cdb6->ss_addr1 = 0;
    cdb6->ss_lun = devicep->sdv_lun;
    cdb6->ss_addr = 0;
    cdb6->ss_len = sizeof (struct sauna_inq);
    cdb6->ss_cont = 0;

    devctl.sdc_command = cdb6;
    devctl.sdc_csize = SCS_SZ;
    devctl.sdc_timeout = 10000;
    devctl.sdc_buffer = sauna;
    devctl.sdc_bsize = sizeof (struct sauna_inq);
    devctl.sdc_mode = SCB_READ;

    if  (clariion_kernel_devctl(devicep, SDI_DEVICE_SEND, &devctl)) {
        status = B_FALSE;
        goto bailout;
    }

    /*
     *  Check to see if the vendor specific string contains the word
     *  "SAUNA" and the product id field is not blank. If both are
     *  true then it is a CLARiiON disk.
     */
    if  ( ( strncmp( sauna->vendor_specific, "SAUNA", 5 ) == 0 ) &&
          ( strncmp( sauna->std_inq.id_prod , " ", 1 ) != 0 ) ) {
        status = B_TRUE;
    }
    else {
        err_log1(CLARIION_MSG_NOT_CLARIION, devicep->sdv_instance);
        status = B_FALSE;
    }

bailout:
    kmem_free(sauna, sizeof(struct sauna_inq) 
        + SDI_PADDR_SIZE(devicep->sdv_bcbp->bcb_physreqp));

    kmem_free(cdb6, sizeof(struct scs)
        + SDI_PADDR_SIZE(devicep->sdv_bcbp->bcb_physreqp));

    return status;
}

static char *gcp;

/*
 * scaled down version of sprintf.  Only handles %x
 */
static void
clariion_hex(ulong_t l)
{

	if (l > 15)
		clariion_hex(l >> 4);
	*gcp++ = "0123456789abcdef"[l & 0xf];
}

static void
clariion_ltoh(char *cp, ulong_t arg)
{
	gcp = cp;
	clariion_hex(arg);
	*gcp = '\0';
}

STATIC int 
clariion_get_signature(path_p_t pathp, void *arg)
{
	get_mp_info_t   get_mp_packet;
	int status;
	int i;
	clariion_sig_t *sigp = &pathp->clr_sig.clr_clariion_sig;
	int length;
	ulong_t *pt;

	status = clariion_get_mp_info(pathp, &get_mp_packet);

	if (status == EINVAL ) {
		return status;
	} else if (status != 0) {
		return -1;
	}

	/*
	 * It's a MP device. Unpack the signature. But first, verify
	 * that the returned data is valid. Punt if any of the fields
	 * is invalid.
	 */
	for ( i = 0 ; i < get_mp_packet.number_of_ports; i ++ ){
	    if (!get_mp_packet.info[i].signature_valid){
		/* FIX */
		/*err_log0(MPIO_MSG_INVALID_MPINFO_PACKET);*/
		return -1;
	    }
	}

	/*
	 * The first mp_info packet is of this port. Get port state.
	 */
	if ( get_mp_packet.info[0].active ){
		sigp->clsig_state = SDI_PATH_ACTIVE;
	} else {
		sigp->clsig_state = SDI_PATH_INACTIVE;
	}

	/*
	 * If both ports are available, we need to sort the signature
	 * halves. The lexicographically lower half will be put into 
	 * the upper half of the caller's signature container.
	 * 
	 * If only one port is available, the signature is put in
	 * the lower half. The upper half should be all 0's.
	 */

	switch (get_mp_packet.number_of_ports){
	    case 1:
		bcopy(&get_mp_packet.info[0].signature, 
			sigp->clsig_lower, MP_PORT_SIGNATURE_LEN);
		break;
    
	    case 2:
		if (strcmp(get_mp_packet.info[0].signature,
				get_mp_packet.info[1].signature) >= 0){
		    bcopy(&get_mp_packet.info[0].signature, 
			    sigp->clsig_lower, 
			    sizeof(sigp->clsig_lower));
		    bcopy(get_mp_packet.info[1].signature, 
			    sigp->clsig_upper, 
			    sizeof(sigp->clsig_upper));
		} else {
		    bcopy(get_mp_packet.info[1].signature, 
			    sigp->clsig_lower, 
			    sizeof(sigp->clsig_lower));
		    bcopy(get_mp_packet.info[0].signature, 
			    sigp->clsig_upper, 
			    sizeof(sigp->clsig_upper));
		}
		break;

	    default:
		/* 
		 * Outside valid port range. Too many or too few ports.
		 */
		/* FIX */
		/*err_log0(MPIO_MSG_INCOMPAT_MPDEV);*/
		status = -1;
		break;
	}    /* switch */

	sigp->clsig_lun = pathp->mydevicep->sdv_lun;
	sigp->clsig_size = sizeof(clariion_sig_t);
	* (sdi_signature_t **) arg = &pathp->clr_sig.clr_sdi_sig;

	return status;
}

/*
 * int
 * clariion_get_mp_info()
 *    dev:    device handle.
 *    arg:    the pointer to the returned client buffer.
 *
 * Description:
 *    Obtain multi-ported info by issuing mode sense commands to the
 *    LUN in question.
 *
 * Calling/Exit State:
 *       
 * Note:
 *    See Data General CLARiiON s/w interface spec.
 * 
 */
STATIC int 
clariion_get_mp_info(path_p_t pathp, get_mp_info_t *arg)
{
    int                                 status = 0;
    struct mode_sense_cdb               cdb6;
    sdi_devctl_t			devctl;
    clariion_peer_sp_page_p_t           page25p;
    clariion_unit_config_page_p_t       page2bp;
    clariion_scsi_unit_config_t         *config = NULL;
    clariion_scsi_peer_sp_t         	*peer = NULL;
    uint_t                              signature;
    get_mp_info_p_t                     mp_info_packet_p;
    uchar_t                             port_0_ownership;

    if (!pathp)
        return -1;

    if (!clariion_is_clariion(pathp->devicep)) {
        err_log1(CLARIION_MSG_NOT_CLARIION, pathp->devicep->sdv_instance);
        return -1;
	}

	config = (clariion_scsi_unit_config_t *)
				sdi_kmem_alloc_phys(sizeof(clariion_scsi_unit_config_t),
						pathp->devicep->sdv_bcbp->bcb_physreqp, 0);

    /*
     * Issue mode page 2B to obtain ownership state.
     */

    bzero(&cdb6, SCS_SZ);
    cdb6.opcode = SS_MSENSE;
    cdb6.pagecode = 0x2B;
    cdb6.len = sizeof(clariion_scsi_unit_config_t);
     
    devctl.sdc_command = &cdb6;
    devctl.sdc_csize = SCS_SZ;
    devctl.sdc_timeout = 10000;
    devctl.sdc_buffer = config;
    devctl.sdc_bsize = sizeof(clariion_scsi_unit_config_t);
    devctl.sdc_mode = SCB_READ;

    status = clariion_kernel_devctl(pathp->devicep, SDI_DEVICE_SEND, &devctl);
    if ( status != 0){
		goto bailout;
    }

    page2bp = &config->page;
    port_0_ownership = 0;

    CLARIION_GETBITS(&page2bp->RES_OWN_RES_BCV_AT_AA_WCE_RCE,
                        CLARIION_UNIT_CONFIG_PAGE_OWN_BV,
                        &port_0_ownership);
    CLARIION_DEBUG1(10,"ownership %x ", port_0_ownership);

	peer = (clariion_scsi_peer_sp_t *)
				sdi_kmem_alloc_phys(sizeof(clariion_scsi_peer_sp_t),
						pathp->devicep->sdv_bcbp->bcb_physreqp, 0);

    /*
     * Issue mode page 0x25 to obtain device signature.
     */
    bzero(&cdb6, SCS_SZ);
    cdb6.opcode = SS_MSENSE;
    cdb6.pagecode = 0x25;
    cdb6.len = sizeof(clariion_scsi_peer_sp_t);

    devctl.sdc_command = &cdb6;
    devctl.sdc_csize = SCS_SZ;
    devctl.sdc_timeout = 10000;
    devctl.sdc_buffer = peer;
    devctl.sdc_bsize = sizeof(clariion_scsi_peer_sp_t);
    devctl.sdc_mode = SCB_READ;

    status = clariion_kernel_devctl(pathp->devicep, SDI_DEVICE_SEND, &devctl);
    if ( status != 0){
        goto bailout;
    }
    page25p = &peer->page;

    /*
     * Unpack raw data from the mode sense data, format and place them
     * in the client returned buffer. Converse the signature(s) into
     * ASCII string(s).
     */
    mp_info_packet_p = (get_mp_info_p_t)arg;
    bzero(mp_info_packet_p, sizeof(get_mp_info_t));
    
    /*
     * Port 0 signature
     */
    signature = (page25p->signature[0] << 24 |
                 page25p->signature[1] << 16 |
                 page25p->signature[2] << 8 |
                 page25p->signature[3]) ;

    clariion_ltoh(mp_info_packet_p->info[0].signature, signature);
    mp_info_packet_p->info[0].signature_valid = 1;
    mp_info_packet_p->info[0].active = port_0_ownership;

    CLARIION_DEBUG1(10,"Port 0 raw signature %x", signature);
    CLARIION_DEBUG1(10,"Port 0 formatted signature %s", &mp_info_packet_p->info[0].signature);
    CLARIION_DEBUG1(10,"Port 0 ownership %x", mp_info_packet_p->info[0].active);
    CLARIION_DEBUG1(10,"Port 0 valid %x", mp_info_packet_p->info[0].signature_valid);
    /*
     * Port 1 signature if this is a dual SP subsystem. If the peer_id
     * field is "0xFF", this is a single SP system.
     */
    if (page25p->peer_id == 0xFF){
        mp_info_packet_p->number_of_ports = 1;
        mp_info_packet_p->info[1].signature_valid = 0;
    }
    else {
        mp_info_packet_p->number_of_ports = 2;

        signature = (page25p->peer_signature[0] << 24 |
                 page25p->peer_signature[1] << 16 |
                 page25p->peer_signature[2] << 8 |
                 page25p->peer_signature[3]) ;

        clariion_ltoh(mp_info_packet_p->info[1].signature, signature);
        mp_info_packet_p->info[1].signature_valid = 1;

        mp_info_packet_p->info[1].active = (port_0_ownership == 1)? 0: 1;

        CLARIION_DEBUG1(10,"Port 1 raw signature %x", signature);
        CLARIION_DEBUG1(10,"Port 1 formatted signature %s", &mp_info_packet_p->info[1].signature);
        CLARIION_DEBUG1(10,"Port 1 ownership %x", mp_info_packet_p->info[1].active);
        CLARIION_DEBUG1(10,"Port 1 valid %x", mp_info_packet_p->info[1].signature_valid);

    }

bailout:
	if (config)
		kmem_free(config, sizeof(clariion_scsi_unit_config_t)
		   + SDI_PADDR_SIZE(pathp->devicep->sdv_bcbp->bcb_physreqp));
	if (peer)
		kmem_free(peer, sizeof(clariion_scsi_peer_sp_t)
		   + SDI_PADDR_SIZE(pathp->devicep->sdv_bcbp->bcb_physreqp));

    return status;
}

/*
 * int
 * clariion_trespass_path()
 *
 * Description:
 *    Issue a mode select to Trespass the individual LUN.
 *
 * Calling/Exit State:
 *   
 * Note:
 *    See Data General CLARiiON s/w interface spec.
 *    Applicable to FLARE code rev 7.50 or newer
 * 
 */
int 
clariion_trespass_path(path_p_t pathp)
{
    struct scs                          cdb6;
    sdi_devctl_t			devctl;
    clariion_scsi_mode_trespass_t       *mode_select_blk;
    int status;

    if (!pathp)
        return -1;

    if (!clariion_is_clariion(pathp->devicep)){
        err_log1(CLARIION_MSG_NOT_CLARIION, pathp->devicep->sdv_instance);
        return -1;
    }

    mode_select_blk = (clariion_scsi_mode_trespass_t *)
			sdi_kmem_alloc_phys(sizeof(clariion_scsi_mode_trespass_t),
			pathp->devicep->sdv_bcbp->bcb_physreqp, 0);

    /*
     * Set up the mode select and issue it.
     */

    mode_select_blk->header.block_desc_len = 
        sizeof(clariion_scsi_mode_blk_descriptor_t);

    mode_select_blk->blk_descriptor.logical_blk_len[1] = 0x2;
    mode_select_blk->blk_descriptor.logical_blk_len[2] = 0x00;

    mode_select_blk->page.header.res_page_code = 0x22;
    mode_select_blk->page.header.page_len = 0x2;
    mode_select_blk->page.trespass_code = 0x1;
    mode_select_blk->page.lun = 0xFF;    /* FLARE rev 7.50 or newer */

    cdb6.ss_op = SS_MSELECT;
    cdb6.ss_addr1 = 0;
    cdb6.ss_lun = 0;
    cdb6.ss_addr = 0;
    cdb6.ss_len = sizeof (clariion_scsi_mode_trespass_t);
    cdb6.ss_cont = 0;

    devctl.sdc_command = &cdb6;
    devctl.sdc_csize = SCS_SZ;
    devctl.sdc_timeout = 10000;
    devctl.sdc_buffer = mode_select_blk;
    devctl.sdc_bsize = sizeof (clariion_scsi_mode_trespass_t);
    devctl.sdc_mode = SCB_WRITE;

    status = clariion_kernel_devctl(pathp->devicep, SDI_DEVICE_SEND, &devctl);

    kmem_free(mode_select_blk, sizeof(clariion_scsi_mode_trespass_t)
		   + SDI_PADDR_SIZE(pathp->devicep->sdv_bcbp->bcb_physreqp));

    return status;
}
