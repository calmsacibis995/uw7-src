#ident	"@(#)kern-pdi:io/target/sdi/sdi_layer.c	1.1.19.1"
#ident	"$Header$"

#include <util/cmn_err.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <fs/buf.h>
#include <io/vtoc.h>
#include <io/target/sdi/sdi.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi_hier.h>
#include <io/target/sdi/sdi_layer.h>
#include <io/target/sdi/sdi_handle.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/target/sd01/fdisk.h>
#include <io/layer/vtoc/vtocdef.h>
#include <mem/kmem.h>
#include <proc/user.h>
#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>

#include <io/ddi.h>	/* must come last */

extern sdi_layer_list_t sdi_layer_order[];
extern int sdi_layer_entries;

sdi_device_t	*sdi_device_stack;
rm_key_t	sdi_rmkey;

lock_t	*sdi_layer_stack_mutex;
pl_t	 sdi_layer_stack_pl;

lock_t	*sdi_device_stack_mutex;
pl_t	 sdi_device_stack_pl;

lock_t	*sdi_work_queue_mutex;
pl_t	 sdi_work_queue_pl;
sdi_device_t	*sdi_work_queue;

/* the lkinfo for sdi driver stack lock */
LKINFO_DECL( sdi_driver_stack_lkinfo, "IO:sdi:sdi_driver_stack_lkinfo", 0);

/* the lkinfo for sdi device stack lock */
LKINFO_DECL( sdi_device_stack_lkinfo, "IO:sdi:sdi_device_stack_lkinfo", 0);

/* the lkinfo for sdi config work_queue lock */
LKINFO_DECL( sdi_work_queue_lkinfo, "IO:sdi:sdi_work_queue_lkinfo", 0);

STATIC uint_t sdi_layer_offset( int );

STATIC sdi_layer_t *sdi_add_driver_to_stack( sdi_driver_desc_t *, int );

STATIC void sdi_rm_device_from_stack( sdi_device_t * );

STATIC void sdi_dev_add_opr( sdi_device_t *, int );

STATIC void sdi_scan_stack_for_devices(sdi_driver_desc_t *, int );
STATIC void sdi_scan_stack_for_drivers(sdi_device_t *, int );

STATIC boolean_t sdi_dev_cfg_match(sdi_driver_desc_t *, sdi_device_t *);

#ifdef SDI_LAYER_DEBUG_TRACE
#define SDI_DTRACE printf("%s(%d)\n",__FILE__,__LINE__)
#else
#define SDI_DTRACE
#endif

/*
 * void
 * sdi_layers_init()
 *	Perform init time functions for sdi I/O stack
 */
void
sdi_layers_init()
{
	int i;
	sdi_layer_t	*lp;

	SDI_DTRACE;

	lp = kmem_zalloc(sizeof(sdi_layer_t)*sdi_layer_entries, KM_NOSLEEP);

	sdi_layer_stack_mutex = LOCK_ALLOC(SDI_HIER_BASE+3,
	                             pldisk, &sdi_driver_stack_lkinfo, KM_NOSLEEP);

	sdi_device_stack_mutex = LOCK_ALLOC(SDI_HIER_BASE+3,
	                             pldisk, &sdi_device_stack_lkinfo, KM_NOSLEEP);

	sdi_work_queue_mutex = LOCK_ALLOC(SDI_HIER_BASE+2,
	                             pldisk, &sdi_work_queue_lkinfo, KM_NOSLEEP);

	if ( !sdi_work_queue_mutex || !sdi_layer_stack_mutex  || !lp || !sdi_device_stack_mutex)
		/*
		 *+ Allocation of sdi data structures failed at
		 *+ boot time, when there should be plenty of
		 *+ memory.  Check system configuration.
		 */
		cmn_err(CE_PANIC,"sdi_layers_init: cannot allocate data structures\n");

	for (i = 0; i < sdi_layer_entries; i++)
		sdi_layer_order[i].layer_info = lp++;

	sdi_device_stack = (sdi_device_t *)NULL;

	sdi_work_queue = (sdi_device_t *)NULL;
}

STATIC void
sdi_dev_add_opr( sdi_device_t *next , int flags )
{
	rm_key_t 		key;
	cm_args_t     	cma;

	SDI_DTRACE;

	ASSERT(next);

	/*
	 *  First, add the device to SDI's list of devices
	 */
	sdi_device_stack_pl = LOCK(sdi_device_stack_mutex, pldisk);

	next->sdv_next = sdi_device_stack;
	sdi_device_stack = next;

	UNLOCK(sdi_device_stack_mutex, sdi_device_stack_pl);

	/*
	 *  Now lets add it to the database
	 */
	key = cm_newkey(NULL, B_FALSE);

	sdi_device_stack_pl = LOCK(sdi_device_stack_mutex, pldisk);
	next->sdv_handle = key;
	UNLOCK(sdi_device_stack_mutex, sdi_device_stack_pl);

	cma.cm_key = key;
	cma.cm_param = SDI_DEVICEID_PARAM;

	cma.cm_val = next;
	cma.cm_vallen = sizeof(struct sdi_device);

	(void)cm_addval(&cma);

	cm_end_trans(key);
}

STATIC void
sdi_rm_device_from_stack( sdi_device_t *next )
{
	sdi_device_t	*device;

	SDI_DTRACE;

	ASSERT(next);

	/*
	 *  remove the device from SDI's list of devices,
	 *  the caller is expected to free the storage used
	 *  by the device on the stack.
	 */
	sdi_device_stack_pl = LOCK(sdi_device_stack_mutex, pldisk);

	device = sdi_device_stack;

	/*
	 * Special case the first one.  Damn, I knew I should have made
	 *  the head of this list a list element instead of a plain pointer.
	 */
	if ( device ) {
		if ( device->sdv_handle == next->sdv_handle ) {
			sdi_device_stack = device->sdv_next;
			UNLOCK(sdi_device_stack_mutex, sdi_device_stack_pl);
			return;
		}
	} else {
		UNLOCK(sdi_device_stack_mutex, sdi_device_stack_pl);
		return;
	}

	/*
	 * now do the rest of the list
	 */
	while ( device->sdv_next ) {
		if ( device->sdv_next->sdv_handle == next->sdv_handle ) {
			device->sdv_next = device->sdv_next->sdv_next;
			break;
		}
		device = device->sdv_next;
	}

	UNLOCK(sdi_device_stack_mutex, sdi_device_stack_pl);
}

boolean_t
sdi_dev_read_opr( rm_key_t key, sdi_device_t *next )
{
	cm_args_t	cma;
	uint_t   	instance;

	SDI_DTRACE;

	cma.cm_key = key;

	cma.cm_param = SDI_DEVICEID_PARAM;
	cma.cm_n = 0;
	cma.cm_val = next;
	cma.cm_vallen = sizeof(struct sdi_device);

	cm_begin_trans(key, RM_READ);

	if ( cm_getval(&cma) == 0 ) {
		cma.cm_param = SDI_INSTANCE_PARAM;
		cma.cm_n = 0;
		cma.cm_val = &instance;
		cma.cm_vallen = sizeof(instance);

		if ( cm_getval(&cma) == 0 )
			next->sdv_instance = instance;
		else
			next->sdv_instance = 0;

		cm_end_trans(key);
		return B_TRUE;
	}
	else {
		cm_end_trans(key);
		return B_FALSE;
	}
}

boolean_t
sdi_dev_write_opr( rm_key_t key, sdi_device_t *next )
{
	cm_args_t     	cma;

	SDI_DTRACE;

	cma.cm_key = key;
	cma.cm_param = SDI_DEVICEID_PARAM;

	cma.cm_n = 0;
	cma.cm_val = next;
	cma.cm_vallen = sizeof(struct sdi_device);

	cm_begin_trans(key, RM_RDWR);

	if ( !cm_delval(&cma) ) {
		if ( !cm_addval(&cma) ) {
			cm_end_trans(key);
			return B_TRUE;
		}
	}

	cm_end_trans(key);
	return B_FALSE;
}

/*
 * sdi_driver_desc_t *
 * sdi_driver_desc_alloc(int flags)
 *	Allocate an sdi_driver_desc_t structure.
 *
 * Calling/Exit State:
 *	If flags is KM_NOSLEEP, NULL will be returned if allocation fails;
 *	otherwise, this routine may block, so no locks may be held on entry.
 */
sdi_driver_desc_t *
sdi_driver_desc_alloc(int flags)
{
	sdi_driver_desc_t *sddp;

	SDI_DTRACE;

	sddp = kmem_zalloc(sizeof(sdi_driver_desc_t), flags);

	if ( sddp )
		sddp->sdd_version = SDI_SDD_VERSION;

	return sddp;
}

/*
 * void
 * sdi_driver_desc_free(sdi_driver_desc_t *sddp)
 *	Free a sdi_driver_desc_t structure.
 *
 * Calling/Exit State:
 *	sddp must be a sdi_driver_desc_t structure
 *	returned by sdi_driver_desc_alloc
 */
void
sdi_driver_desc_free(sdi_driver_desc_t *sddp)
{
	SDI_DTRACE;

	kmem_free(sddp, sizeof(sdi_driver_desc_t));
}

/*
 * boolean_t
 * sdi_driver_desc_prep(sdi_driver_desc_t *sddp, int flags)
 *	Prepare a sdi_driver_desc_t for use.
 *
 * Calling/Exit State:
 *	Must be called after all fields in sddp have been set but before
 *	this sdi_driver_desc_t is passed to any SDI routine (e.g.
 *	sdi_driver_add, sdi_driver_rm).
 *
 *	flags is KM_SLEEP or KM_NOSLEEP.
 *	Return value is B_FALSE if the sdi_driver_desc cannot be successfully
 *	prepped (either due to allocation failure in KM_NOSLEEP case or
 *	due to unsupportable values).
 *
 *	It is legal to call sdi_driver_desc_prep multiple times
 *	on the same sdi_driver_desc_t.
 */
/* ARGSUSED */
boolean_t
sdi_driver_desc_prep(sdi_driver_desc_t *sddp, int flags)
{

	int i;
	SDI_DTRACE;

#ifdef DEBUG
	sddp->sdd_flags |= SDI_SDD_PREPPED;
#endif

	if (sddp->sdd_version != SDI_SDD_VERSION) {
		cmn_err(CE_WARN, "!sdi_driver_desc_prep: invalid sdd_version value %d",sddp->sdd_version);
#ifdef DEBUG
		sddp->sdd_flags &= ~SDI_SDD_PREPPED;
#endif
		return B_FALSE;
	}

	if ((i = sdi_layer_offset(sddp->sdd_precedence)) == sdi_layer_entries) {
		cmn_err(CE_WARN, "!sdi_driver_desc_prep: invalid sdd_precedence value %d",sddp->sdd_precedence);
		cmn_err(CE_WARN, "!sdi_driver_desc_prep: offset is %d, max is %d",i, sdi_layer_entries);
#ifdef DEBUG
		sddp->sdd_flags &= ~SDI_SDD_PREPPED;
#endif
		return B_FALSE;
	}

	return B_TRUE;
}

/*
 * sdi_device_t *
 * sdi_device_alloc(int flags)
 *	Allocate an sdi_device_t structure.
 *
 * Calling/Exit State:
 *	If flags is KM_NOSLEEP, NULL will be returned if allocation fails;
 *	otherwise, this routine may block, so no locks may be held on entry.
 */
sdi_device_t *
sdi_device_alloc(int flags)
{
	sdi_device_t *sdvp;

	SDI_DTRACE;

	sdvp = kmem_zalloc(sizeof(sdi_device_t), flags);

	if ( sdvp ) {
		sdvp->sdv_version = SDI_SDV_VERSION;
	}

	return sdvp;
}

/*
 * void
 * sdi_device_copy(sdi_device_t *src, sdi_device_t *dest)
 *	Copy an sdi_device_t structure.
 *
 * Calling/Exit State:
 *	none
 */
void
sdi_device_copy(sdi_device_t *src, sdi_device_t *dest)
{
	SDI_DTRACE;

	if ( !src || !dest )
		return;

	dest->sdv_order   = src->sdv_order;
	dest->sdv_unit    = src->sdv_unit;
	dest->sdv_bus     = src->sdv_bus;
	dest->sdv_target  = src->sdv_target;
	dest->sdv_lun     = src->sdv_lun;
	dest->sdv_devtype = src->sdv_devtype;
	dest->sdv_bcbp    = src->sdv_bcbp;
	dest->sdv_cgnum   = src->sdv_cgnum;

	bcopy(src->sdv_inquiry, dest->sdv_inquiry, INQ_EXLEN);
	bcopy(&(src->sdv_stamp), &(dest->sdv_stamp), sizeof(struct pd_stamp));
}

/*
 * void
 * sdi_device_free(sdi_device_t *sdvp)
 *	Free a sdi_device_t structure.
 *
 * Calling/Exit State:
 *	sdvp must be a sdi_device_t structure
 *	returned by sdi_device_alloc
 */
void
sdi_device_free(sdi_device_t *sdvp)
{
	SDI_DTRACE;

	kmem_free(sdvp, sizeof(sdi_device_t));
}

/*
 * boolean_t
 * sdi_device_prep(sdi_device_t *sdvp, int flags)
 *	Prepare a sdi_device_t for use.
 *
 * Calling/Exit State:
 *	Must be called after all fields in sdvp have been set but before
 *	this sdi_device_t is passed to any SDI routine (e.g.
 *	sdi_driver_add, sdi_driver_rm).
 *
 *	flags is KM_SLEEP or KM_NOSLEEP.
 *	Return value is B_FALSE if the sdi_device cannot be successfully
 *	prepped (either due to allocation failure in KM_NOSLEEP case or
 *	due to unsupportable values).
 *
 *	It is legal to call sdi_device_prep multiple times
 *	on the same sdi_device_t.
 */
/* ARGSUSED */
boolean_t
sdi_device_prep(sdi_device_t *sdvp, int flags)
{

	SDI_DTRACE;

#ifdef DEBUG
	sdvp->sdv_flags |= SDI_SDV_PREPPED;
#endif

	if (sdvp->sdv_version != SDI_SDV_VERSION) {
#ifdef DEBUG
		sdvp->sdv_flags &= ~SDI_SDV_PREPPED;
#endif
		return B_FALSE;
	}

#ifdef NOT_YET
	if (!sdvp->sdv_layer || !sdvp->sdv_driver) {
		cmn_err(CE_WARN, "!sdi_device_prep: value must be supplied for layer and driver");
#ifdef DEBUG
		sdvp->sdv_flags &= ~SDI_SDV_PREPPED;
#endif
		return B_FALSE;
	}
#endif

	return B_TRUE;
}

/*
 * sdi_layer_t *
 * sdi_driver_add(sdi_driver_desc_t *driverp, int flags)
 *	Add an I/O driver to the SDI I/O stack
 *
 * Calling/Exit State:
 *	If flags is KM_NOSLEEP, NULL will be returned if allocation fails;
 *	otherwise, this routine may block, so no locks may be held on entry.
 */
sdi_layer_t *
sdi_driver_add(sdi_driver_desc_t *driverp, int flags)
{
	sdi_layer_t *layerp;

	SDI_DTRACE;

	ASSERT(driverp);
	ASSERT(driverp->sdd_flags & SDI_SDV_PREPPED);

	/*
	 * install the driver in the I/O stack data structures
	 */
	layerp = sdi_add_driver_to_stack( driverp, flags );

	/*
	 * match the driver with any devices of interest, setting
	 *   modname on the resmgr record for each device matched.
	 *   also invoke the _config entrypoint of the supplier of the device.
	 */
	if ( layerp )
		sdi_scan_stack_for_devices( driverp, flags );

	return layerp;
}

STATIC uint_t
sdi_layer_offset( int precedence )
{
	int i;

	SDI_DTRACE;

	for ( i = 0; i < sdi_layer_entries; i++ )
		if ( precedence == sdi_layer_order[i].layer_precedence )
			return i;

	return sdi_layer_entries;
}

STATIC sdi_layer_t *
sdi_add_driver_to_stack( sdi_driver_desc_t *driverp, int flags)
{
	uint_t offset;
	sdi_layer_t 	*lp;

	SDI_DTRACE;

	ASSERT(driverp);

	offset = sdi_layer_offset(driverp->sdd_precedence);

	ASSERT(offset < sdi_layer_entries);

	lp = sdi_layer_order[offset].layer_info;

	sdi_layer_stack_pl = LOCK(sdi_layer_stack_mutex, pldisk);

	driverp->sdd_next = sdi_layer_order[offset].layer_info->la_driver_list;
	sdi_layer_order[offset].layer_info->la_driver_list = driverp;

	driverp->sdd_layer = lp;

	UNLOCK(sdi_layer_stack_mutex, sdi_layer_stack_pl);

	return lp;
}

/*
 * boolean_t
 * sdi_device_add_rinit(sdi_device_t *devicep, int flags)
 *	Add an I/O device to the SDI device queue for
 *	processing later.
 *
 * Calling/Exit State:
 *	If flags is KM_NOSLEEP, NULL will be returned if allocation fails;
 *	otherwise, this routine may block, so no locks may be held on entry.
 */
boolean_t
sdi_device_add_rinit(sdi_device_t *devicep, int flags)
{
	SDI_DTRACE;

	ASSERT(devicep);
	ASSERT(devicep->sdv_flags & SDI_SDV_PREPPED);

	devicep->sdv_work_queue_operation = SDI_WORKQ_DEVICE_ADD;

	sdi_work_queue_pl = LOCK(sdi_work_queue_mutex, pldisk);

	devicep->sdv_work_queue_next = sdi_work_queue;
	sdi_work_queue = devicep;

	UNLOCK(sdi_work_queue_mutex, sdi_work_queue_pl);

	return B_TRUE;
}

/*
 * boolean_t
 * sdi_device_add(sdi_device_t *devicep, int flags)
 *	Add an I/O device to the SDI I/O stack
 *
 * Calling/Exit State:
 *	If flags is KM_NOSLEEP, NULL will be returned if allocation fails;
 *	otherwise, this routine may block, so no locks may be held on entry.
 */
boolean_t
sdi_device_add(sdi_device_t *devicep, int flags)
{
	SDI_DTRACE;

	ASSERT(devicep);
	ASSERT(devicep->sdv_flags & SDI_SDV_PREPPED);

	sdi_dev_add_opr( devicep , flags );
	/*
	 * match the device with any drivers of interest, setting
	 *   modname on the resmgr record for each device matched.
	 *   also invoke the _config entrypoint of those drivers.
	 */
	sdi_scan_stack_for_drivers( devicep, flags );

	return B_TRUE;
}

/*
 * boolean_t
 * sdi_device_rm_rinit(rm_key_t key)
 *	queue a Remove opr for an SDI I/O device
 *
 * Calling/Exit State:
 */
boolean_t
sdi_device_rm_rinit(rm_key_t key, int flags)
{
	sdi_device_t *devicep;

	SDI_DTRACE;

	devicep = kmem_zalloc(sizeof(sdi_device_t), flags);

	if ( !devicep )
		return B_FALSE;

	if ( !sdi_dev_read_opr( key, devicep ) ) {
		kmem_free(devicep, sizeof(sdi_device_t));
		return B_FALSE;
	}

	devicep->sdv_work_queue_operation = SDI_WORKQ_DEVICE_RM;
	sdi_work_queue_pl = LOCK(sdi_work_queue_mutex, pldisk);

	devicep->sdv_work_queue_next = sdi_work_queue;
	sdi_work_queue = devicep;

	UNLOCK(sdi_work_queue_mutex, sdi_work_queue_pl);

	/*
	 * NOTE: the memory allocated above is freed by
	 *       sdi_enable_all as it processes the queue
	 */

	return B_TRUE;
}

/*
 * boolean_t
 * sdi_device_rm(rm_key_t key)
 *	Remove an I/O device from the SDI I/O stack
 *
 * Calling/Exit State:
 */
boolean_t
sdi_device_rm(rm_key_t key, int flags)
{
	sdi_device_t *devicep;

	SDI_DTRACE;

	devicep = kmem_zalloc(sizeof(sdi_device_t), flags);

	if ( !devicep )
		return B_FALSE;

	if ( !sdi_dev_read_opr( key, devicep ) ) {
		kmem_free(devicep, sizeof(sdi_device_t));
		return B_FALSE;
	}

	sdi_rm_device_from_stack(devicep);

	cm_delkey(devicep->sdv_handle);

	kmem_free(devicep, sizeof(sdi_device_t));

	return B_TRUE;
}

/*
 * void
 * sdi_enable_all(void)
 *	process all queued sdi device operations.
 */
void
sdi_enable_all(void)
{
	sdi_device_t	*current, *next;

	SDI_DTRACE;

	sdi_work_queue_pl = LOCK(sdi_work_queue_mutex, pldisk);

	next = sdi_work_queue;
	sdi_work_queue = (sdi_device_t *)NULL;

	UNLOCK(sdi_work_queue_mutex, sdi_work_queue_pl);

	while ( next ) {

		current = next;
		next = current->sdv_work_queue_next;

		switch (current->sdv_work_queue_operation) {
		case SDI_WORKQ_DEVICE_ADD:

			sdi_dev_add_opr( current , KM_SLEEP );
			/*
			 * match the device with any drivers of interest, setting
			 *   modname on the resmgr record for each device matched.
			 *   also invoke the _config entrypoint of those drivers.
			 */
			sdi_scan_stack_for_drivers( current, KM_SLEEP );

			break;

		case SDI_WORKQ_DEVICE_RM:

			/*
			 * First, remove the device from SDI's data structures
			 */
			sdi_rm_device_from_stack(current);

			/*
			 * Next, remove the device from the RESMGR database.
			 * This will trigger a config(remove_instance) call
			 * on any owner of this device.
			 */
			cm_delkey(current->sdv_handle);

			kmem_free(current, sizeof(sdi_device_t));

			break;
		case SDI_WORKQ_DEVICE_MOD:
		case SDI_WORKQ_DRIVER_ADD:
		case SDI_WORKQ_DRIVER_RM:
		case SDI_WORKQ_DRIVER_MOD:
		default:
			break;
		}
	}
}

/*
 * void
 * sdi_enable_instance(HBA_IDATA_STRUCT *idatap)
 *	trigger the queued sdi device ops for idatap instance
 */
void
sdi_enable_instance(HBA_IDATA_STRUCT *idatap)
{
	SDI_DTRACE;

	sdi_enable_all();
}

STATIC boolean_t
sdi_write_modname_and_instance(sdi_driver_desc_t *driverp, sdi_device_t *devicep)
{
	cm_args_t	cma;
	uint_t   	instance;

	/*
	 * NOTE: we actually calculate instance HERE
	 */
	sdi_layer_stack_pl = LOCK(sdi_layer_stack_mutex, pldisk);
		instance = driverp->sdd_instance++;
	UNLOCK(sdi_layer_stack_mutex, sdi_layer_stack_pl);

	cma.cm_key = devicep->sdv_handle;

	cma.cm_param = SDI_INSTANCE_PARAM;
	cma.cm_val = &instance;
	cma.cm_vallen = sizeof(instance);

	cm_begin_trans(devicep->sdv_handle, RM_RDWR);

	(void)cm_addval(&cma);

	cma.cm_param = CM_MODNAME;
	cma.cm_val = driverp->sdd_modname;
	cma.cm_vallen = strlen(driverp->sdd_modname) + 1;

	(void)cm_addval(&cma);

	cm_end_trans(devicep->sdv_handle);

	return B_TRUE;
}

/*
 * void
 * sdi_scan_stack_for_devices(sdi_driver_desc_t *driverp, int flags)
 *	Match this driver with any devices in the stack
 *
 * Calling/Exit State:
 *	If flags is KM_NOSLEEP, NULL will be returned if allocation fails;
 *	otherwise, this routine may block, so no locks may be held on entry.
 */
STATIC void
sdi_scan_stack_for_devices(sdi_driver_desc_t *driverp, int flags)
{
	sdi_device_t	*device, *list;
	int         	driver_offset;
	void     		*config_arg;

	SDI_DTRACE;

	ASSERT(driverp);
	ASSERT(driverp->sdd_flags & SDI_SDD_PREPPED);

	list = NULL;

	driver_offset = sdi_layer_offset(driverp->sdd_precedence);

	sdi_device_stack_pl = LOCK(sdi_device_stack_mutex, pldisk);

	device = sdi_device_stack;

	while ( device ) {
		if ( !(device->sdv_flags & SDI_SDV_CLAIMED) ) {
			if ( sdi_layer_offset(device->sdv_driver->sdd_precedence) < driver_offset ) {
				if ( sdi_dev_cfg_match(driverp, device) ) {
					device->sdv_flags |= SDI_SDV_CLAIMED;
					device->sdv_consumer = driverp;
					device->sdv_drvadd_next = list;
					list = device;
					cmn_err(CE_NOTE, "!dev_cfg match between %s and %s.\n",
							driverp->sdd_modname,device->sdv_inquiry);
				}
			}
		}
		device = device->sdv_next;
	}

	UNLOCK(sdi_device_stack_mutex, sdi_device_stack_pl);

	/*
	 *  Next, set the modname appropriately.
	 */

	device = list;

	while ( device ) {
		(void)sdi_write_modname_and_instance(driverp, device);

		device = device->sdv_drvadd_next;
	}
}

STATIC boolean_t
sdi_dev_cfg_match(sdi_driver_desc_t *driverp, sdi_device_t *devicep)
{
	int i, isaddr, istype;

	SDI_DTRACE;

	ASSERT(driverp);
	ASSERT(devicep);

	if (!driverp->sdd_dev_cfg_size)
		return B_FALSE;

	for (i=0; i < driverp->sdd_dev_cfg_size; i++) {

		isaddr =(driverp->sdd_dev_cfg[i].hba_no == DEV_CFG_UNUSED ||
					driverp->sdd_dev_cfg[i].hba_no == devicep->sdv_unit) &&
				(driverp->sdd_dev_cfg[i].scsi_id == DEV_CFG_UNUSED ||
					driverp->sdd_dev_cfg[i].scsi_id == devicep->sdv_target) &&
				(driverp->sdd_dev_cfg[i].lun == DEV_CFG_UNUSED ||
					driverp->sdd_dev_cfg[i].lun == devicep->sdv_lun) &&
				(driverp->sdd_dev_cfg[i].bus == DEV_CFG_UNUSED ||
					driverp->sdd_dev_cfg[i].bus == devicep->sdv_bus);

		if (!isaddr) {
			continue;
		}

		istype =(driverp->sdd_dev_cfg[i].devtype == 0xff ||
			        driverp->sdd_dev_cfg[i].devtype == devicep->sdv_devtype) &&
			    (!driverp->sdd_dev_cfg[i].inq_len ||
			        !strncmp(devicep->sdv_inquiry,
							 driverp->sdd_dev_cfg[i].inquiry,
					         driverp->sdd_dev_cfg[i].inq_len));

		if (istype) {
			return B_TRUE;
		}

	}

	return B_FALSE;
}

STATIC void
sdi_scan_stack_for_drivers(sdi_device_t *device, int flags)
{
	int		offset;
	sdi_driver_desc_t	*driverp, *tdriverp;
	cm_args_t     	cma;
	void     		*config_arg;

	SDI_DTRACE;

	ASSERT(device);
	ASSERT(device->sdv_flags & SDI_SDV_PREPPED);

	if ( device->sdv_handle == RM_NULL_KEY )
		return;

	offset = sdi_layer_offset(device->sdv_driver->sdd_precedence);

	sdi_layer_stack_pl = LOCK(sdi_layer_stack_mutex, pldisk);

	for (offset++; offset < sdi_layer_entries; offset++) {

		driverp = sdi_layer_order[offset].layer_info->la_driver_list;

		while ( driverp ) {
			if ( sdi_dev_cfg_match(driverp, device) ) {
				device->sdv_flags |= SDI_SDV_CLAIMED;
				device->sdv_consumer = tdriverp = driverp;
				goto commit;
			}

			driverp = driverp->sdd_next;
		}
	}
	UNLOCK(sdi_layer_stack_mutex, sdi_layer_stack_pl);
	return;

commit:
	UNLOCK(sdi_layer_stack_mutex, sdi_layer_stack_pl);

	(void)sdi_write_modname_and_instance(tdriverp, device);
}

/*
 * sdi_device_t *
 * sdi_addr_lookup(struct scsi_adr *sap)
 *	search the device stack for the specified entry.
 *
 *  NOTE: This routine is used by B_RXEDT in sdi_ioctl.
 *        Don't change it unless you want to break pdimkdev.
 *
 * Calling/Exit State:
 *	sdi_edt_mutex is held across the call
 */
sdi_device_t *
sdi_addr_lookup(struct scsi_adr *sap)
{
	sdi_device_t *devicep;

	ASSERT(sap);

	sdi_device_stack_pl = LOCK(sdi_device_stack_mutex, pldisk);

	devicep = sdi_device_stack;

	while ( devicep ) {
		if (devicep->sdv_unit   == sap->scsi_ctl &&
	 		devicep->sdv_bus    == sap->scsi_bus &&
	 		devicep->sdv_target == sap->scsi_target &&
	 		devicep->sdv_lun    == sap->scsi_lun) {
			if (!(devicep->sdv_flags & SDI_SDV_CLAIMED)) {
				UNLOCK(sdi_device_stack_mutex, sdi_device_stack_pl);
				return devicep;
			}
		}
		devicep = devicep->sdv_next;
	}

	UNLOCK(sdi_device_stack_mutex, sdi_device_stack_pl);

	return (sdi_device_t *)NULL;
}

/*
 * SDI rouines to manipulate the buf_t structure
 */
void
sdi_buf_store(buf_t *bp, daddr_t blkno)
{
	struct sdi_buf_save *save_bufp;

	save_bufp =  kmem_alloc(sizeof (struct sdi_buf_save), KM_SLEEP);
	save_bufp->prev_misc = bp->b_misc;
	save_bufp->prev_iodone = bp->b_iodone;
	/*save_bufp->prev_flags = bp->b_flags;*/
	bp->b_flags |= B_TAPE;
	save_bufp->blkno = blkno == -1 ?
			((struct sdi_buf_save *) (bp->b_misc))->blkno : blkno;
	bp->b_misc = save_bufp;
}

/*
 * This routine will be invoked before the call
 * to biodone
 */
void
sdi_buf_restore(buf_t *bp)
{
	struct sdi_buf_save *save_bufp = bp->b_misc;

	if (!(bp->b_flags & B_TAPE)) {
		return;
	}

	bp->b_misc = save_bufp->prev_misc;
	bp->b_iodone = save_bufp->prev_iodone;
	/*bp->b_flags = save_bufp->prev_flags;*/
	kmem_free(save_bufp, sizeof(struct sdi_buf_save));

	if (bp->b_misc == NULL) {
		bp->b_flags &= ~B_TAPE;
	}
}

/*
 * Retrieve stored blkno.
 */
daddr_t
sdi_get_blkno(buf_t *bp)
{
	struct sdi_buf_save *save_bufp = (struct sdi_buf_save *) bp->b_misc;
	daddr_t blkno;

	if (bp->b_flags & B_TAPE) {
		blkno = save_bufp->blkno;
	} else {
		blkno = bp->b_blkno;
	}

	return blkno;
}

void
sdi_print_drivers()
{
	int		offset;
	sdi_driver_desc_t	*driver;

	for (offset = 0; offset < sdi_layer_entries; offset++) {

		driver = sdi_layer_order[offset].layer_info->la_driver_list;
		if ( !driver ) {
			cmn_err(CE_CONT, "Layer %d is empty.\n",offset);
			continue;
		}

		cmn_err(CE_CONT, "Layer %d contains:\n",offset);

		do {
			if ( driver->sdd_modname ) {
				cmn_err(CE_CONT,"modname: %s, ", driver->sdd_modname);
			} else {
				cmn_err(CE_CONT,"modname: %s, ", "is not yet set");
			}
			cmn_err(CE_CONT,"precedence: %d, ", driver->sdd_precedence);
			cmn_err(CE_CONT,"dev_cfg_size: %d, ", driver->sdd_dev_cfg_size);
			cmn_err(CE_CONT,"flags: 0x%x, ", driver->sdd_flags);
			cmn_err(CE_CONT,"next: 0x%x\n", driver->sdd_next);

		} while ( (driver = driver->sdd_next) );
	}
}

void
sdi_print_devices()
{
	sdi_device_t	*device;

	device = sdi_device_stack;

	while ( device ) {
		cmn_err(CE_CONT,"inquiry: %s, ", device->sdv_inquiry);
		cmn_err(CE_CONT,"rmkey: %d, ", device->sdv_handle);
		cmn_err(CE_CONT,"instance: %d\n", device->sdv_instance);
		cmn_err(CE_CONT,"supplier: %s, ", device->sdv_driver->sdd_modname);
		cmn_err(CE_CONT,"stamp: %s, ", &device->sdv_stamp);
		cmn_err(CE_CONT,"order: %d\n", device->sdv_order);

		device = device->sdv_next;
	}
}

STATIC sdi_device_t *
sdi_scan_stack_for_stamp(struct pd_stamp *stamp)
{
	sdi_device_t	*device;
	int         	top_layer;

	SDI_DTRACE;

	if (!stamp)
		return NULL;

	/*
	 * figure out the offset at the top of the stack.
	 */
	top_layer = sdi_layer_offset(sdi_layer_order[sdi_layer_entries-1].layer_precedence);

	sdi_device_stack_pl = LOCK(sdi_device_stack_mutex, pldisk);

	device = sdi_device_stack;

	while ( device ) {
		if ( sdi_layer_offset(device->sdv_driver->sdd_precedence) == top_layer ) {
			if ( PD_STAMPMATCH(stamp, &device->sdv_stamp) ) {
				UNLOCK(sdi_device_stack_mutex, sdi_device_stack_pl);
				return(device);
			}
		}
		device = device->sdv_next;
	}
	UNLOCK(sdi_device_stack_mutex, sdi_device_stack_pl);

	return NULL;
}

/*
 * ulong_t
 * sdi_calc_partition()
 *	return the partition of the root filesystem
 *
 *  partition is a 1-based integer
 *  0 returned for no partition table yet
 *
 * Calling/Exit State:
 *	acquires/releases no locks
 */
ulong_t
sdi_calc_root_partition()
{
	char *bootdev, *part;

	/*
	 * get the BOOTDEV string
	 */
	bootdev = bs_getval("BOOTDEV");

	/*
	 * no BOOTDEV string, return default ( 0 )
	 */
	if (!bootdev)
		return 0;

	/*
	 * make sure we are booting from hd
	 */
	if (bootdev[0] != 'h' || bootdev[1] != 'd')
		return 0;

	part = strchr(bootdev, ',');

	/*
	 * no partition value in string, return default ( 0 )
	 */
	if ( !part )
		return 0;

	part++;

	return strtoul(part, NULL, 10);
}

#define DEFAULT_BOOT_SLICE	1

/*
 * ulong_t
 * sdi_calc_slice()
 *	return the slice of the root filesystem
 *
 *  slice is a 0-based integer
 *
 * Calling/Exit State:
 *	acquires/releases no locks
 */
STATIC ulong_t
sdi_calc_root_slice()
{
	char *bootdev, *part;

	/*
	 * get the BOOTDEV string
	 */
	bootdev = bs_getval("BOOTDEV");

	/*
	 * no BOOTDEV string, return default ( 1 )
	 */
	if (!bootdev)
		return DEFAULT_BOOT_SLICE;

	/*
	 * make sure we are booting from hd
	 */
	if (bootdev[0] != 'h' || bootdev[1] != 'd')
		return DEFAULT_BOOT_SLICE;

	part = strchr(bootdev, ',');
	if ( !part )
		return DEFAULT_BOOT_SLICE;
	part++;

	bootdev = strchr(part, ',');
	if ( !bootdev )
		return DEFAULT_BOOT_SLICE;
	bootdev++;

	return strtoul(bootdev, NULL, 10);
}

/*
 * sdi_device_t *
 * sdi_boot_device()
 *	calculate the sdi_device_t of the boot device
 *
 * Calling/Exit State:
 *	acquires/releases no locks
 */
sdi_device_t *
sdi_boot_device(void)
{
	/*
	 * get the disk stamp of the boot disk and see
	 * if we can find it on top of the I/O stack.
	 */

	return sdi_scan_stack_for_stamp((struct pd_stamp *)bs_getval("BOOTSTAMP"));
}

/*
 * sdi_device_t *
 * sdi_swap_device()
 *	calculate the sdi_device_t of the swap device
 *
 * Calling/Exit State:
 *	acquires/releases no locks
 */
sdi_device_t *
sdi_swap_device(void)
{
	/*
	 * get the disk stamp of the swap disk and see
	 * if we can find it on top of the I/O stack.
	 */

	return sdi_scan_stack_for_stamp((struct pd_stamp *)bs_getval("SWAPSTAMP"));
}

/*
 * sdi_device_t *
 * sdi_stand_device()
 *	calculate the sdi_device_t of the stand device
 *
 * Calling/Exit State:
 *	acquires/releases no locks
 */
sdi_device_t *
sdi_stand_device(void)
{
	/*
	 * get the disk stamp of the stand disk and see
	 * if we can find it on top of the I/O stack.
	 */

	return sdi_scan_stack_for_stamp((struct pd_stamp *)bs_getval("STANDSTAMP"));
}

/*
 * sdi_device_t *
 * sdi_dump_device()
 *	calculate the sdi_device_t of the dump device
 *
 * Calling/Exit State:
 *	acquires/releases no locks
 */
sdi_device_t *
sdi_dump_device(void)
{
	/*
	 * get the disk stamp of the dump disk and see
	 * if we can find it on top of the I/O stack.
	 */

	return sdi_scan_stack_for_stamp((struct pd_stamp *)bs_getval("DUMPSTAMP"));
}

extern void *dumpcookie;

/*
 * void
 * sdi_calc_handles()
 *	calculate the handles of various well-known storage devices
 *
 * Calling/Exit State:
 *	acquires/releases no locks
 */
STATIC void
sdi_calc_handles(dev_t *root, dev_t *dump, dev_t *swap, dev_t *stand)
{
	dev_t root_basedev, basedev;
	sdi_device_t *rootdevice, *mydevice;
	sdi_slice_ck_t cmd_args;
	boolean_t	result;
	ulong_t  	channel;

	/*
	 * Find the boot disk in the I/O stack based on
	 * the value of the BOOTSTAMP boot parameter.
	 */
	rootdevice = sdi_boot_device();

	if (!rootdevice)
		return;

	(void)find_basedev_on_rmkey(rootdevice->sdv_parent_handle, &root_basedev);


	*root = root_basedev + sdi_calc_root_slice();

	/*
	 *  Now we need to find the channels for stand, swap and dump.  We
	 *  assume that these slices, if they exist, exist on the same
	 *  device as root did unless told otherwise by an appropriate stamp..
	 */
	channel = 0;

	/*
	 *  First, we look for a slice marked stand on the specified disk.
	 */
	if ( (mydevice = sdi_stand_device()) ) {
		(void)find_basedev_on_rmkey(mydevice->sdv_parent_handle, &basedev);
	}
	else {
		mydevice = rootdevice;
		basedev = root_basedev;
	}

	result = (mydevice->sdv_drvops->d_open) \
			(mydevice->sdv_idatap, &channel, FREAD, sys_cred, NULL);

	if (result)
		return;

	cmd_args.ssc_ptag = V_STAND;

	result = (mydevice->sdv_drvops->d_drvctl) \
			(mydevice->sdv_idatap, channel, SDI_DEVICE_SLICE_CK, &cmd_args);

	if (!result && cmd_args.ssc_exists)
		*stand = basedev + cmd_args.ssc_channel;
	else
		*stand = NODEV;

	(void)(mydevice->sdv_drvops->d_close) \
			(mydevice->sdv_idatap, channel, FREAD, sys_cred, NULL);

	/*
	 *  Next, we look for a slice marked swap on the specified disk.
	 */
	if ( (mydevice = sdi_swap_device()) ) {
		(void)find_basedev_on_rmkey(mydevice->sdv_parent_handle, &basedev);
	}
	else {
		mydevice = rootdevice;
		basedev = root_basedev;
	}

	result = (mydevice->sdv_drvops->d_open) \
			(mydevice->sdv_idatap, &channel, FREAD, sys_cred, NULL);

	if (result)
		return;

	cmd_args.ssc_ptag = V_SWAP;

	result = (mydevice->sdv_drvops->d_drvctl) \
			(mydevice->sdv_idatap, channel, SDI_DEVICE_SLICE_CK, &cmd_args);

	if (!result && cmd_args.ssc_exists)
		*swap = basedev + cmd_args.ssc_channel;
	else
		*swap = NODEV;

	(void)(mydevice->sdv_drvops->d_close) \
			(mydevice->sdv_idatap, channel, FREAD, sys_cred, NULL);

	/*
	 *  Next, we look for a slice on the specified disk marked as dump.
	 *  If none exists, we make it's channel the same as swap's.
	 */
	if ( (mydevice = sdi_dump_device()) ) {
		(void)find_basedev_on_rmkey(mydevice->sdv_parent_handle, &basedev);
	}
	else {
		mydevice = rootdevice;
		basedev = root_basedev;
	}

	result = (mydevice->sdv_drvops->d_open) \
			(mydevice->sdv_idatap, &channel, FREAD, sys_cred, NULL);

	if (result)
		return;

	cmd_args.ssc_ptag = V_DUMP;

	result = (mydevice->sdv_drvops->d_drvctl) \
			(mydevice->sdv_idatap, channel, SDI_DEVICE_SLICE_CK, &cmd_args);

	if (!result && cmd_args.ssc_exists)
		*dump = basedev + cmd_args.ssc_channel;
	else
		*dump = *swap;

	if((IDP(HBA_tbl[mydevice->sdv_unit].idata)->version_num & HBA_VMASK) >= HBA_UW20_IDATA) {
		dumpcookie = IDP(HBA_tbl[mydevice->sdv_unit].idata)->idata_intrcookie;
	}
	else {
		dumpcookie = NULL;
	}

	(void)(mydevice->sdv_drvops->d_close) \
			(mydevice->sdv_idatap, channel, FREAD, sys_cred, NULL);
}

/*
 * dev_t
 * sdi_return_handle()
 *	return the handles of various well-known storage devices
 *
 * Calling/Exit State:
 *	acquires/releases no locks
 */
dev_t
sdi_return_handle(sdi_hcmd_t cmd)
{
	static dev_t root=NODEV, dump=NODEV, swap=NODEV, stand=NODEV;

	if (root == NODEV || dump == NODEV || swap == NODEV || stand == NODEV)
		sdi_calc_handles(&root, &dump, &swap, &stand);

	switch(cmd) {
	case SDI_ROOTDEV:
		return(root);
	case SDI_DUMPDEV:
		return(dump);
	case SDI_SWAPDEV:
		return(swap);
	case SDI_STANDDEV:
		return(stand);
	}
}

/*
 * cgnum_t
 * sdi_get_cgnum()
 *	return the cgnum of the specified HBA
 *
 * Calling/Exit State:
 *	acquires/releases sdi_rinit_lock
 */
cgnum_t
sdi_get_cgnum(int unit_number)
{
	cm_args_t   cma;
	rm_key_t	rm_key;
	cgid_t	cgid;
	cgnum_t	result;
	HBA_IDATA_STRUCT *idatap;
	extern int sdi_hacnt;

	if ( unit_number < 0 || unit_number >= sdi_hacnt )
		return 0;

	idatap = IDP(HBA_tbl[unit_number].idata);

	if ( !idatap )
		return 0;

	if ((idatap->version_num & HBA_VMASK) < PDI_UNIXWARE20)
		return 0;

	cma.cm_key = rm_key = idatap->idata_rmkey;
	cma.cm_n   = 0;
	cma.cm_val = &cgid;
	cma.cm_vallen = sizeof(cgid);
	cma.cm_param = CM_CGID;

	cm_begin_trans(rm_key, RM_READ);
	if (cm_getval(&cma)) {
		cm_end_trans(rm_key);
		return 0;
	}
	cm_end_trans(rm_key);

	result = cgid2cgnum(cgid);

	if ( result < 0 )
		return 0;

	return result;
}
