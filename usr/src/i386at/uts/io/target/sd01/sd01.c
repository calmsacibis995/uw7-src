#ident	"@(#)kern-pdi:io/target/sd01/sd01.c	1.172.32.3"
#ident	"$Header$"

#define _DDI 8

#include	<util/types.h>
#include	<io/vtoc.h>
#include	<io/target/sd01/fdisk.h>  /* Included for 386 disk layout */

#include	<fs/buf.h>		/* Included for dma routines   */
#include	<io/conf.h>
#include	<io/elog.h>
#include	<io/open.h>
#include	<io/target/altsctr.h>
#include	<io/target/alttbl.h>
#include	<io/target/scsi.h>
#include	<io/target/sd01/sd01.h>
#include	<io/target/sd01/sd01_ioctl.h>
#include	<io/target/sdi/dynstructs.h>
#include	<io/target/sdi/sdi.h>
#include	<io/target/sdi/sdi_edt.h>
#include	<io/target/sdi/sdi_hier.h>
#include	<io/target/sdi/sdi_layer.h>
#include	<io/layer/vtoc/vtocdef.h>
#include	<io/uio.h>	/* Included for uio structure argument*/
#include	<mem/kmem.h>	/* Included for DDI kmem_alloc routines */
#include	<proc/cred.h>	/* Included for cred structure arg   */
#include	<proc/proc.h>	/* Included for proc structure arg   */
#include	<svc/errno.h>
#include	<util/cmn_err.h>
#include	<util/debug.h>
#include	<util/param.h>

#include	<util/ipl.h>
#include	<util/ksynch.h>

#include	<io/ddi.h>	/* must come last */

#define		DRVNAME	"SD01 DISK driver"
#define		DRVNAMELEN	64
#define		SCSI_NAME	"scsi"

/*
 * DDI8 entry points
 */
STATIC int
	sd01open8(void *, ulong_t *, int, cred_t *, queue_t *),
	sd01devinfo8(void *, ulong_t, di_parm_t, void *),
	sd01ioctl8(void *, ulong_t, int, void *, int, cred_t *, int *),
	sd01close8(void *, ulong_t, int, cred_t *, queue_t *),
	sd01devctl8(void *, ulong_t, int, void *);

STATIC void
	sd01biostart8(void *, ulong_t, buf_t *);

/*
 * DDI8 helper entry points
 */
STATIC int
	sd01_firstOpen8(struct disk *),
	sd01config8(struct disk *),
	sd01layer_cmd(struct disk *, void *, uint, time_t, void *, uint, ushort);

int
	sd01_claim(struct owner *),
	sd01_dkinit(struct disk *, struct owner *, int),
	sd01_dmstamp(struct disk *),
	sd01_find_disk(struct sdi_edt *),
	sd01_find_stamp(struct pd_stamp *),
	sd01_first_open(struct disk *),
	sd01_last_close(struct disk *, int),
	sd01_putStamp(struct disk *, struct pd_stamp *, daddr_t),
	sd01_write_pdinfo(struct disk *),
	sd01cmd(struct disk *, char, uint, char *, uint, uint, ushort, int),
	sd01fillvtoc(struct disk *, struct pdinfo *),
	sd01gen_sjq(struct disk *, buf_t *, int , struct job *),
	sd01qresume(struct disk *),
	sd01phyrw(struct disk *, long, struct phyio *, int),
	sd01releasejob(struct job *),
	sd01rm_dev(struct scsi_adr *),
	sd01send(struct disk *, struct job *, int);

ulong_t sd01_random();
struct job * sd01getjob(struct disk *, int, struct buf *);

void 
	sd01_dk_free(struct disk *),
	sd01_dk_gc(void),
	sd01_dk_unmark(void),
	sd01_dm_free(struct disk_media *),
	sd01_newstamp(struct pd_stamp *),
	sd01_setseed(int, ulong_t),
	sd01comp1(struct job *),
	sd01done(struct job *),
	sd01flush_job(struct disk *),
	sd01intn(struct sb *),
	sd01queue_hi(struct job *, struct disk *),
	sd01queue_low(struct job *, struct disk *),
	sd01resume(struct disk *),
	sd01retry(struct job *),
	sd01rinit(),
	sd01sendt(struct disk *),
	sd01start(),
	sd01strat1(struct job *, struct disk *, buf_t *),
	sd01strategy(buf_t *),
	sd01_aen(long, int);

STATIC void sd01_dotimeout(struct job *, struct disk *, int);
STATIC void sd01Release(struct disk *);
STATIC void sd01Reserve(struct disk *);

#ifdef SD01_DEBUGPR 
void sd01prt_dsk(int),
     sd01prt_job(struct job *),
     sd01prt_jq(struct disk *);
#endif /* SD01_DEBUGPR */

extern void	sd01intb();

extern int	sd01_retrycnt;
extern int	sd01_lightweight;

/* Default stamp structure to be used in PD_STAMPMATCH() calls */
struct pd_stamp sd01_dfltstamp;

ulong_t sd01_rand[2];	/* Random variables for sd01_random() */

STATIC int sd01_timeslices;

/* Disk path pointer array
 * Certain assumptions are made for the locking of sd01_dp.
 * Only sd01rinit() will insert entries into sd01_dp[].
 * Only sd01rm_dev() will delete entries from sd01_dp[].
 * sd01rm_dev() will fail if the device is open.
 */
struct disk	*sd01_dp[DK_MAX_PATHS];
#define SD01_DP_LOCK() do {						\
				   pl_t pl; 				\
			           pl = LOCK(sd01_dp_mutex, pldisk);	\
			           if (sd01_dp_lock == 0) {		\
				        sd01_dp_lock = 1; 		\
				        UNLOCK(sd01_dp_mutex, pl);	\
				        break;				\
			           }					\
			        UNLOCK(sd01_dp_mutex, pl);		\
				delay(1);				\
			   } while (1)

/* sets x to 0 if the lock was not acquired, to 1 if it was */
#define SD01_DP_TRYLOCK(x) do {						\
				   pl_t pl; 				\
			           pl = LOCK(sd01_dp_mutex, pldisk);	\
 				   x = 0;				\
			           if (sd01_dp_lock == 0)		\
				        x = sd01_dp_lock = 1; 		\
			        UNLOCK(sd01_dp_mutex, pl);		\
			   } while (0)

#define SD01_DP_UNLOCK() do {						\
				pl_t pl; 				\
			        pl = LOCK(sd01_dp_mutex, pldisk);	\
				sd01_dp_lock = 0;			\
			        UNLOCK(sd01_dp_mutex, pl);		\
			 } while (0)

#define EDT_C edtp->scsi_adr.scsi_ctl
#define EDT_B edtp->scsi_adr.scsi_bus
#define EDT_T edtp->scsi_adr.scsi_target
#define EDT_L edtp->scsi_adr.scsi_lun
#define EDT_PD edtp->pdtype

STATIC lock_t	*sd01_dp_mutex=NULL;
STATIC pl_t	sd01_dp_pl;
STATIC int	sd01_dp_lock=0;

STATIC lock_t	*sd01_resume_mutex=NULL;
STATIC pl_t	sd01_resume_pl;


extern sleep_t	*sdi_rinit_lock;

STATIC LKINFO_DECL( sd01_resume_lkinfo, "IO:sd01:sd01_resume_lkinfo", 0);
STATIC LKINFO_DECL( sd01_dp_lkinfo, "IO:sd01:sd01_dp_lkinfo", 0);
STATIC LKINFO_DECL( sd01dm_lkinfo, "IO:sd01:sd01dm_lkinfo", 0);
STATIC LKINFO_DECL( sd01dk_lkinfo, "IO:sd01:sd01dk_lkinfo", 0);

#ifdef SD01_DEBUG
daddr_t sd01alts_badspot;
struct disk *dk_badspot;
uint sd01alts_debug = (DKD | HWREA);
#endif /* SD01_DEBUG */

#define	MAXFER_DEFAULT	0x10000
#define ALPHABET_SIZE	26
#define PRINTABLE_BASE	52

struct 	sb *sd01_fltsbp;		/* SB exclusively for RESUMEs 	*/
struct 	resume sd01_resume;		/* Linked list of disks waiting */

int 	sd01_diskcnt;			/* Number of disks configured,
					 * maintained strictly for debugging */

extern struct head	lg_poolhead;	/* head for dync struct routines */
extern char Sd01_modname[];		/* defined in Space.c */

int sd01devflag = D_NOBRKUP | D_MP | D_BLKOFF;

drvops_t	sd01drvops = {
	NULL,       	/* no config entry point */
	sd01open8,
	sd01close8,
	sd01devinfo8,
	sd01biostart8,
	sd01ioctl8,
	sd01devctl8,
	NULL };		/* no mmap entry point */

#define SD01_MAX_CHAN	0

const drvinfo_t	sd01drvinfo = {
	&sd01drvops, Sd01_modname, D_MP, NULL, SD01_MAX_CHAN };

/*===========================================================================*/
/* Debugging mechanism that is to be compiled only if -DDEBUG is included
/* on the compile line for the driver                                  
/* DPR provides levels 0 - 9 of debug information.                  
/*      0: No debugging.
/*      1: Entry points of major routines.
/*      2: Exit points of some major routines. 
/*      3: Variable values within major routines.
/*      4: Variable values within interrupt routine (inte, intn).
/*      5: Variable values within sd01logerr routine.
/*      6: Bad Block Handling
/*      7: Multiple Major numbers
/*      8: - 9: Not used
/*============================================================================*/

#define O_DEBUG         0100

STATIC sdi_layer_t      	*sd01_layerp;
STATIC sdi_driver_desc_t	*sd01_driver_descp;

/*
 * int
 * sd01_load(void)
 *
 * Calling/Exit State:
 *	None
 */
int
_load(void)
{
	drv_attach(&sd01drvinfo);
	sd01start();
	return(0);
}

/*
 * STATIC int
 * sd01_unload(void)
 *
 * Calling/Exit State:
 *	None
 */
int
_unload(void)
{
	return(EBUSY);
}

/*
 * void
 * sd01start(void)
 *	Initialize the SCSI disk driver. - 0dd01000
 *	This function allocates and initializes the disk driver's data 
 *	structures.  This function also initializes the disk drivers device 
 *	instance array. An instance number (starting with zero) will be 
 *	assigned for each set of block and character major numbers. Note: the
 *	system must allocate the same major number for both the block
 *	and character devices or disk corruption will occur.
 *	This function does not access any devices.
 *
 * Called by: Kernel
 * Side Effects: 
 *	The disk queues are set empty and all partitions are marked as
 *	closed. 
 * ERRORS:
 *	The disk driver can not determine equipage.  This is caused by
 *	insufficient virtual or physical memory to allocate the driver
 *	Equipped Device Table (EDT) data structure.
 *
 *	The disk driver is not fully configured.  This is caused by 
 *	insufficient virtual or physical memory to allocate internal
 *	driver structures.
 *
 * Calling/Exit State:
 *	Acquires sdi_rinit_lock if dynamic load
 */
void
sd01start(void)
{
	time_t time_sec;
	clock_t lbolt_tick;

#ifdef DEBUG
	DPR (1)("\n\t\tSD01 DEBUG DRIVER INSTALLED\n");
#endif
	/*
	 * Set the seed for the first random number generator to number of
	 * clock ticks since the Epoch.  The seed for the second random number
	 * generator is set in sd01rinit().
	 */
	drv_getparm(TIME, (time_t *) &time_sec);
	drv_getparm(LBOLT, (clock_t *) &lbolt_tick);
	sd01_setseed(0, (ulong_t) (time_sec*HZ + lbolt_tick%HZ));

	PD_SETSTAMP(&sd01_dfltstamp, PD_STAMP_DFLT, PD_STAMP_DFLT, PD_STAMP_DFLT);
	sd01_diskcnt = 0;

	/* If Sd01 log flag is invalid, set it to default mode. */
	if(Sd01log_marg > 2)
		Sd01log_marg = 0;
	
	/* Setup the linked list for Resuming LU queues */
	/* allocate and initialize its lock */
 	sd01_resume.res_head = (struct disk *) &sd01_resume;
 	sd01_resume.res_tail = (struct disk *) &sd01_resume;

	sd01_resume_mutex = LOCK_ALLOC(TARGET_HIER_BASE+2, pldisk,
				       &sd01_resume_lkinfo, sdi_sleepflag);
	sd01_dp_mutex = LOCK_ALLOC(TARGET_HIER_BASE+3, pldisk,
				   &sd01_dp_lkinfo, sdi_sleepflag);

	if (!sd01_resume_mutex || !sd01_dp_mutex)
	{
		if (sd01_resume_mutex)
			LOCK_DEALLOC(sd01_resume_mutex);
		if (sd01_dp_mutex)
			LOCK_DEALLOC(sd01_dp_mutex);
		/*
		 *+ There was insufficient memory for disk data structures
		 *+ at boot-time.  This probably means that the system is
		 *+ configured with too little memory. 
		 */
		cmn_err(CE_WARN,
			"Insufficient memory to configure disk driver.\n");
		return;
	}

	if ( !(sd01_fltsbp = sdi_getblk(sdi_sleepflag)) )
	{
		/*
		 *+ There was insufficient memory for disk data structures
		 *+ at boot-time.  This probably means that the system is
		 *+ configured with too little memory. 
		 */
		cmn_err(CE_WARN,
			"Insufficient memory to configure disk driver.\n");
		return;
	}

	/*
	 *	We must perform the layer initialization
	 *	here before the call to sd01rinit
	 */
	sd01_driver_descp = sdi_driver_desc_alloc( sdi_sleepflag );
	if ( sd01_driver_descp ) {
		(void)strcpy(sd01_driver_descp->sdd_modname, Sd01_modname);
		sd01_driver_descp->sdd_precedence = SDI_STACK_BASE;
		sd01_driver_descp->sdd_dev_cfg = NULL;
		sd01_driver_descp->sdd_dev_cfg_size = 0;
		sd01_driver_descp->sdd_config_entry = (int (*)())NULL;
		sd01_driver_descp->sdd_minors_per = SD01_MAX_CHAN + 1;
		if ( sdi_driver_desc_prep(sd01_driver_descp, sdi_sleepflag) ) {
			sd01_layerp = sdi_driver_add( sd01_driver_descp , sdi_sleepflag);
			if ( !sd01_layerp ) {
				sdi_driver_desc_free( sd01_driver_descp );
				/*
				 *+ There was a problem with the disk driver description
				 *+ structure during initialization.  This is probably
				 *+ caused by a corrupted image of the driver.
				 */
				cmn_err(CE_WARN,
					"Insufficient memory to register disk driver.\n");
				return;
			}
		} else {
			sdi_driver_desc_free( sd01_driver_descp );
			/*
			 *+ There was a problem with the disk driver description
			 *+ structure during initialization.  This is probably
			 *+ caused by a corrupted image of the driver.
			 */
			cmn_err(CE_WARN,
				"Invalid disk driver description.\n");
			return;
		}
	} else {
		/*
		 *+ There was insufficient memory for disk data structures
		 *+ at boot-time.  This probably means that the system is
		 *+ configured with too little memory. 
		 */
		cmn_err(CE_WARN,
			"Insufficient memory to configure disk driver.\n");
		return;
	}


	if (sdi_sleepflag == KM_SLEEP)
		SLEEP_LOCK(sdi_rinit_lock, pridisk);
	sd01rinit();
	if (sdi_sleepflag == KM_SLEEP)
		SLEEP_UNLOCK(sdi_rinit_lock);
}


/*
 * void
 * sd01rinit(void)
 *	Called by sdi to perform driver initialization of additional
 *	devices found after the dynamic load of HBA drivers. 
 *	This function does not access any devices.
 *
 * Calling/Exit State:
 *	sdi_rinit_lock must be held by caller
 */
void
sd01rinit(void)
{
	struct drv_majors drv_maj;
	struct owner *ownerlist;
	int rc, new_sd01_diskcnt;
	time_t time_sec;
	clock_t lbolt_tick;
	sdi_device_t	*layer_device, *new_device_stack = (sdi_device_t *)NULL;
	int index;
	struct disk *dp;

	SD01_DP_LOCK();

	drv_maj.b_maj = Sd01_Identity;
	drv_maj.c_maj = Sd01_Identity;
	ownerlist = sdi_doconfig(SD01_dev_cfg, SD01_dev_cfg_size, DRVNAME, &drv_maj, sd01rinit);

	sdi_target_hotregister(sd01rm_dev, ownerlist);

	/* unmark all the disk structures */
	sd01_dk_unmark();

	/*
	 * Set the seed for the second random number generator to number of
	 * clock ticks since the Epoch.  The seed for the first random number
	 * generator is set in sd01start().
	 */
	drv_getparm(TIME, (time_t *) &time_sec);
	drv_getparm(LBOLT, (clock_t *) &lbolt_tick);
	sd01_setseed(1, (ulong_t) (time_sec*HZ + lbolt_tick%HZ));

	/* add and new disks */
	new_sd01_diskcnt = 0;
	for (;
	     ownerlist;
	     ownerlist = ownerlist->target_link)
	{
		if ((rc = sd01_claim(ownerlist))) {
			new_sd01_diskcnt++;
#ifdef SD01_DEBUG
			cmn_err(CE_CONT,"ownerlist 0x%x ", ownerlist);
			cmn_err(CE_CONT,"edt 0x%x ", ownerlist->edtp);
			cmn_err(CE_CONT,"hba %d scsi id %d lun %d bus %d\n",
			ownerlist->EDT_C, ownerlist->EDT_T,
			ownerlist->EDT_L, ownerlist->EDT_B);
#endif
			if ( rc > 0 ) {
				/*
				 *  We have come across a disk we have not seen before.
				 *
				 *  Create a new sdi_device structure for this device and
				 *  register it with the sdi_layer manager.
				 */
				layer_device = sdi_device_alloc( sdi_sleepflag );
				if ( layer_device ) {
					layer_device->sdv_state   = SDI_DEVICE_EXISTS|SDI_DEVICE_ONLINE;
					layer_device->sdv_layer   = sd01_layerp;
					layer_device->sdv_driver  = sd01_driver_descp;
					layer_device->sdv_unit    = ownerlist->EDT_C;
					layer_device->sdv_bus     = ownerlist->EDT_B;
					layer_device->sdv_target  = ownerlist->EDT_T;
					layer_device->sdv_lun     = ownerlist->EDT_L;
					layer_device->sdv_devtype = ownerlist->EDT_PD;
					layer_device->sdv_cgnum   = sdi_get_cgnum(ownerlist->EDT_C);

					index = sd01_find_disk(ownerlist->edtp);
					layer_device->sdv_order = index - 1;
					dp = sd01_dp[index];
					dp->dk_sdi_device = layer_device;
					layer_device->sdv_bcbp    = dp->dk_bcbp;

					layer_device->sdv_idatap  = (void *)dp;
					layer_device->sdv_drvops  = &sd01drvops;

					(void)bcopy(ownerlist->edtp->inquiry, layer_device->sdv_inquiry, INQ_EXLEN);
					(void)bcopy(&(ownerlist->edtp->stamp), &(layer_device->sdv_stamp), sizeof(struct pd_stamp));

					if (sdi_device_prep(layer_device, sdi_sleepflag) ) {
						layer_device->sdv_gp = (void *)ownerlist->edtp;
						layer_device->sdv_supplier_next = new_device_stack;
						new_device_stack = layer_device;
					}
					else {
						cmn_err(CE_WARN,
							"sd01rinit: invalid device structure version");
						sdi_device_free(layer_device);
						continue;
					}
				}
				else {
					cmn_err(CE_WARN,
						"sd01rinit: insufficient memory for device structures");
					continue;
				}
			}
		}
	}
#ifdef SD01_DEBUG
        cmn_err(CE_CONT,"sd01rinit %d disks claimed, previously %d\n",
		new_sd01_diskcnt, sd01_diskcnt);
#endif
	sd01_diskcnt = new_sd01_diskcnt;

	/* free any unclaimed disks */
	sd01_dk_gc();

	SD01_DP_UNLOCK();

	layer_device = new_device_stack;

	while ( layer_device ) {
		if (sdi_device_add_rinit(layer_device, sdi_sleepflag) ) {
	/*
	 * I should probably lock this edt entry before I do this
	 * however, I am so deep in locks now the hierarchy problems
	 * would be enormous.  Anyway, I'm the owner here so there
	 * should never be any other writers.
	 */
			((struct sdi_edt *)(layer_device->sdv_gp))->se_devicep = layer_device;
			cmn_err(CE_NOTE,
				"!sd01rinit: just added device %s",layer_device->sdv_inquiry);
		}
		else {
			cmn_err(CE_WARN,
				"sd01rinit: unable to add device to I/O stack");
			layer_device = layer_device->sdv_supplier_next;
			sdi_device_free(layer_device);
			continue;
		}
		layer_device = layer_device->sdv_supplier_next;
	}
}

/*
 * int 
 * sd01rm_dev()
 * the function to support hot removal of disk drives.
 * This will spin down the disk and remove it from its internal
 * structures, sd01_dp.
 *
 * returns SDI_RET_ERR or SDI_RET_OK
 * will fail if the device doesn't exist, is not owned by sd01,
 * is open.
 */
int sd01rm_dev(struct scsi_adr *sa)
{
	struct sdi_edt *edtp;
	struct disk *dp;
	struct disk_media *dm;
	int index;
	int part;

	ASSERT(sa);
	edtp = sdi_rxedt(sa->scsi_ctl, sa->scsi_bus,
			 sa->scsi_target, sa->scsi_lun);
	
	if ((edtp == NULL) ||	/* no device */
	    (edtp->curdrv == NULL)) /* no owner */
		return SDI_RET_ERR;

	SD01_DP_LOCK();

	if ((index = sd01_find_disk(edtp)) < 0 ) {
		SD01_DP_UNLOCK();
		return SDI_RET_ERR;
	}

	if ((dp = sd01_dp[index]) == NULL) {
		SD01_DP_UNLOCK();
		return SDI_RET_ERR;
	}

	dm = dp->dk_dm;

	/* check if the disk is open */
	if (dm->dm_openCount) {
		SD01_DP_UNLOCK();
		return SDI_RET_ERR;
	}

	if (dp->dk_state & DKINIT) {
		/* flush cache and spin down the disk */
		if (sd01cmd(dp, SS_ST_SP, 0, NULL, 0, 0, SCB_READ, FALSE)) {
			cmn_err(CE_WARN, "!Disk Driver: Couldn't spin down disk for"
				" removal %d: %d,%d,%d\n", sa->scsi_ctl, sa->scsi_bus,
				sa->scsi_lun, sa->scsi_bus);
		}
	}

    sdi_device_rm(dp->dk_sdi_device->sdv_handle, KM_NOSLEEP);
    sdi_device_free(dp->dk_sdi_device);

	/*
	 * For drivers that unload it is necessary to keep the list
	 * of devices returned by sdi_doconfig().  Whenever a device
	 * is added or removed from the system it is necessary to
	 * add/remove the device from the list.  The item removed
	 * must have its target_link set to NULL.
	 * This is not necessary for SD01 because it is never
	 * unloaded.
	 */
	edtp->curdrv->target_link = NULL;

	sdi_clrconfig(edtp->curdrv, SDI_REMOVE|SDI_DISCLAIM, NULL);

	sd01_dp[index]=NULL;
	sd01_dk_free(dp);

	SD01_DP_UNLOCK();

	return SDI_RET_OK;
}

/************************************************************
 * Unmarks all the disks so that garbage collection can
 * take place later
 ************************************************************/
void
sd01_dk_unmark(void)
{
	int index;

	for (index=1;
	     index < DK_MAX_PATHS;
	     index++)
	{
		if (sd01_dp[index] == NULL) continue;
		sd01_dp[index]->marked = 0;
		
	}
}

/************************************************************
 * Free's any disk structures that aren't marked
 ************************************************************/
void 
sd01_dk_gc(void)
{
	int index;

	for (index=1;
	     index < DK_MAX_PATHS;
	     index++)
	{
		if (sd01_dp[index] == NULL) continue;
		if (sd01_dp[index]->marked) continue;

#ifdef SD01_DEBUG
		cmn_err(CE_CONT, "sd01_dk_gc() freeing sd01_dp[%d]=0x%x\n",
			index, sd01_dp[index]);
#endif
		sd01_dk_free(sd01_dp[index]);
		sd01_dp[index] = NULL;
	}
}

/*************************************************************
 *
 * Returns the index into the sd01_dp[] associated with the passed 
 * owner pointer. Returns -1 if it can't be found.
 * NOTE: this is fairly slow as it searches all
 * entries to find the entry that matches the 
 * [Controller, Bus, Target, Lun] in the owner pointer
 ************************************************************/
int 
sd01_find_disk(struct sdi_edt *edtp)
{
	int index;
	int ret;
	int match = -1;

	ret = -1;

	for (index=1;
	     index < DK_MAX_PATHS;
	     index++)
	{
		if (sd01_dp[index] == NULL) continue;

		
		if (! sd01_dp[index]->dk_addr.pdi_adr_version )
			match = SDI_ADDRCMP(&(sd01_dp[index]->dk_addr), 
					&(edtp->scsi_adr));
		else
			match =
				SDI_ADDR_MATCH(&(sd01_dp[index]->dk_addr.extended_address->scsi_adr), &(edtp->scsi_adr));
		if ( match == 1 ) {
			ret = index;
			break;
		}
	}

	return ret;
}

/*
 * int
 * sd01_claim(struct owner *op)
 *	Creates, initializes and marks a new disk entry.  If the entry already
 *	exists, it is simply marked but this is not an error.
 * Returns: 0 failure, -1 old disk, 1 new disk
 */
int 
sd01_claim(struct owner *op)
{
	struct disk *dp;
	struct disk_media *dm;
	struct pd_stamp *dmstamp = &sd01_dfltstamp;
	int index, dpindex = -1;
	int stamped = FALSE;

	index = sd01_find_disk(op->edtp);
	if (index != -1)
	{
		sd01_dp[index]->marked = 1;
		return(-1);
	}
	for (index=1;
	     index < DK_MAX_PATHS;
	     index++)
	{
		if (sd01_dp[index] == NULL)
			break;
	}
	if (index >= DK_MAX_PATHS)
	{
		/*
		 *+ All of the available spaces in sd01_dp
		 *+ have been used.  One is needed for each
		 *+ disk.  Increase the size of sd01_dp 
		 *+ in /etc/conf/pack.d/sd01.cf/Space.c
		 */
		cmn_err(CE_WARN, "Disk Driver: Maximum disk configuration exceeded.");
		return(0);
	}
	if ((dp = (struct disk *) kmem_zalloc(sizeof(struct disk),
		sdi_sleepflag)) == NULL ||
	    (dm = (struct disk_media *) kmem_zalloc(sizeof(struct disk_media),
		sdi_sleepflag)) == NULL)  {
		/*
		 *+ There was insuffcient memory for disk data structures.
		 *+ This probably means that the system is
		 *+ configured with too little memory. 
		 */
		cmn_err(CE_WARN, "Disk Driver: Insufficient memory to configure driver.");
		return(0);
	}

	dp->dk_dm = dm;

	/* Set major/minor #'s please */
	op->maj.b_maj = Sd01_Identity;
	op->maj.c_maj = Sd01_Identity;

	/* Set async callback handler */
	op->fault = sd01_aen;
	op->flt_parm = (long)dp;

	if (!sd01_dkinit(dp, op, index)) {
		cmn_err(CE_WARN, "Disk Driver: Cannot initialize disk: c%db%dt%dl%d\n",
			op->EDT_C, op->EDT_B, op->EDT_T, op->EDT_L);
		goto err;
	}

	sd01_dp[index] = dp;
	sd01_dp[index]->marked = 1;

	/*
	 * Open the disk to process disk stamp stored in PDINFO and determine
	 * path-equivalence.
	 */
	if (!(sd01_firstOpen8(dp))) {
		if (sd01_dmstamp(dp) == 0)
			stamped = TRUE;

		(void) sd01_last_close(dp, FALSE);

		/* Determine path-equivalence */
		if (stamped) {
			/* Treat serial field of incore PDINFO as disk stamp */
			dmstamp = &dm->dm_stamp;

			if ((dpindex = sd01_find_stamp(dmstamp)) != -1) {
				dp->dk_dm = sd01_dp[dpindex]->dk_dm;
				dp->dk_dm->dm_refcount++;
				sd01_dm_free(dm);
			}
		}
		else {
			/*
			 * Valid disk stamp is not available.  We must reserve the disk
			 * to prevent other controllers from sharing the disk.
			 */
			sd01Reserve(dp);
		}
	} 


	/*
	 * Signal close only if the dm structure was not freed.
	 */
	if (dpindex == -1) {
		dm->dm_state &= ~DM_FO_LC;
	}

	/* Populate incore PDINFO disk stamp into corresponding EDT entry */
	if ((sdi_setstamp(op->edtp, dmstamp)) != SDI_RET_OK) {
		cmn_err(CE_WARN, "!Disk Driver: Failed to set stamp in EDT for %d: %d,%d,%d\n",
			op->EDT_C, op->EDT_B, op->EDT_T, op->EDT_L);
		goto err;
	}

	return(1);

err:
	if (sd01_dp[index])
		sd01_dp[index] = NULL;

	kmem_free(dm, sizeof(struct disk_media));
	kmem_free(dp, sizeof(struct disk));

	return(0);
}

STATIC daddr_t
sd01_getStamp(struct disk *dp, struct pd_stamp *dmstamp)
{
	struct pdinfo *pdptr;
	struct phyio phyarg;
	daddr_t stampAddr = 0;
	struct ipart *part, *this_part;
	uchar_t *buff2;
	struct pd_stamp *boot_stamp, active_stamp;
	ulong_t	i, active_part;
	boolean_t active_found = B_FALSE;

	pdptr = (struct pdinfo *)sdi_kmem_alloc_phys(BLKSZ(dp), dp->dk_bcbp->bcb_physreqp, 0); 

	buff2 = sdi_kmem_alloc_phys(BLKSZ(dp), dp->dk_bcbp->bcb_physreqp, 0); 

	/*
	 * Fetch the fdisk table to determine where Unix Starts. We know it
	 * is the first block.
	 */
	phyarg.sectst = FDBLKNO;
	phyarg.memaddr = (long) buff2;
	phyarg.datasz = BLKSZ(dp);
	sd01phyrw(dp, V_RDABS, &phyarg, SD01_KERNEL);

	if (phyarg.retval != 0 ||
		((struct mboot *) buff2)->signature != MBB_MAGIC) {
		goto bailout;
	}

	boot_stamp = (struct pd_stamp *)bs_getval("BOOTSTAMP");

	part = (struct ipart *) (void *) ((struct mboot *)(void *) buff2) ->parts;

	for (i = 0; i < FD_NUMPART; i++) {
		this_part = &part[i];
		if ((this_part->systid & PTYP_MASK) == UNIXOS) {
			/*
			 * Fetch the pdinfo/vtoc to determine the stamp
			 */
			phyarg.sectst = HDPDLOC + this_part->relsect;
			phyarg.memaddr = (long) pdptr;
			phyarg.datasz = BLKSZ(dp);
			sd01phyrw(dp, V_RDABS, &phyarg, SD01_KERNEL);

			if (pdptr->sanity == VALID_PD) {
				if (boot_stamp &&
				   PD_STAMPMATCH((struct pd_stamp *)&pdptr->serial, boot_stamp)) {
					bcopy(pdptr->serial, dmstamp, sizeof(struct pd_stamp));
					stampAddr = HDPDLOC + this_part->relsect;
					break;
				}
				if ((this_part->bootid & 0xFF) == ACTIVE) {
					active_found = B_TRUE;
					active_part = i;
					bcopy(pdptr->serial, &active_stamp, sizeof(struct pd_stamp));
				}
			}
		}
	}

	if ( stampAddr == 0  && active_found ) {
		bcopy(&active_stamp, dmstamp, sizeof(struct pd_stamp));
		stampAddr = HDPDLOC + part[active_part].relsect;
	}

bailout:
	kmem_free(pdptr, BLKSZ(dp) + SDI_PADDR_SIZE(dp->dk_bcbp->bcb_physreqp));
	kmem_free(buff2, BLKSZ(dp) + SDI_PADDR_SIZE(dp->dk_bcbp->bcb_physreqp));

	return stampAddr;
}

/*
 * int
 * sd01_dmstamp(struct disk *dp)
 *	Determine the stamp to be used for the disk.  If the disk is stamped
 *	with a default stamp, sd01_dmstamp will attempt to write a new stamp
 *	to the disk.
 */
STATIC int
sd01_dmstamp(struct disk *dp)
{
	struct disk_media *dm = dp->dk_dm;
	daddr_t stampAddr;
	struct pd_stamp dmstamp;
	uchar_t *buff;
	int ret_val = 0;

	if ((stampAddr = sd01_getStamp(dp, &dm->dm_stamp)) == 0) {
		ret_val = EACCES;
		goto bailout;
	}

	if (PD_STAMPMATCH(&dm->dm_stamp, &sd01_dfltstamp)) {
		(void) sd01_newstamp(&dm->dm_stamp);
		ret_val = sd01_putStamp(dp, &dm->dm_stamp, stampAddr);
	}

bailout:
	return ret_val;
}

/*
 * void
 * sd01_newstamp(struct pd_stamp *stamp)
 *	Assign a 12-bytes (virtually) unique and non-default stamp using two
 *	32-bits pseudo-random numbers.
 * Note: This function will assign a unique and non-default stamp with respect
 *	to the current set of disks in the sd01_dp[] array, but this stamp is
 *	not a world-wide unique disk signature.  Thus, there is a small
 *	probability that this stamp may be the same as the stamp on another
 *	disk which is not yet configured into the system.
 */
void
sd01_newstamp(struct pd_stamp *stamp)
{
	int i, j;
	ulong_t rand[2];
	char *sptr, ch;

	do {
		do {
			rand[0] = sd01_random(0);
			rand[1] = sd01_random(1);
			sptr = (char *) stamp;
			for (i = sizeof(struct pd_stamp)-1; i >= 0; i--) {
				j = i / (sizeof(struct pd_stamp)/2);
				if ((ch = rand[j] % PRINTABLE_BASE) < ALPHABET_SIZE)
					sptr[i] = 'A' + ch;
				else
					sptr[i] = 'a' + (ch - ALPHABET_SIZE);
				rand[j] /= PRINTABLE_BASE;
			}
		} while (PD_STAMPMATCH(stamp, &sd01_dfltstamp));
	} while (sd01_find_stamp(stamp) != -1);
}

/*
 * ulong_t
 * sd01_random(int index)
 *	This function returns a 32-bits random number using a Linear
 *	Congruential Generator (LCG). I.e.
 *
 *	X    = (Multiplier * X  + 1)) MOD Modulus
 *	 i+1                  i
 *
 *	The index parameter indicates which LCG to use.
 *
 *	Notice that the modulus for these LCGs equal to 2^32, thus, no
 *	need to actually perform the modulo operation.
 */
ulong_t
sd01_random(int index)
{
	if (index)
		sd01_rand[index] = 69069U * sd01_rand[index] + 1U;
	else
		sd01_rand[index] = 1664525U * sd01_rand[index] + 1U;
	return(sd01_rand[index]);
}

/*
 * void
 * sd01_setseed(int index, ulong_t seed)
 *	Set the random variable of the sd01_rand[] array to seed, thus,
 *	reseeding the random number generator in sd01_random().
 */
void
sd01_setseed(int index, ulong_t seed)
{
	sd01_rand[index] = seed;
}

/*
 * FIX!!!!
 * int 
 * sd01_find_stamp(struct pd_stamp *stamp)
 *	Returns the index of a sd01_dp[] entry whose EDT stamp matches with the
 *	given stamp.
 * Returns: -1 if it can't be found.
 * NOTE: This is fairly slow as it searches all entries to find the entry
 *	that matches the given disk stamp.
 */
int 
sd01_find_stamp(struct pd_stamp *stamp)
{
	int index;
	int ret_val;
	struct sdi_edt *edtp;
	struct disk *dp;
	int c, b, t, l;

	ret_val = -1;
	for (index = 1; index < DK_MAX_PATHS; index++) {
		if ((dp = sd01_dp[index]) == NULL)
			continue;

		if (! dp->dk_addr.pdi_adr_version ) {
			c = SDI_EXHAN(&dp->dk_addr);
			b = SDI_BUS(&dp->dk_addr);
			t = SDI_EXTCN(&dp->dk_addr);
			l = SDI_EXLUN(&dp->dk_addr);
		}
		else {
			c = SDI_HAN_32(&dp->dk_addr);
			b = SDI_BUS_32(&dp->dk_addr);
			t = SDI_TCN_32(&dp->dk_addr);
			l = SDI_LUN_32(&dp->dk_addr);
		}
		if ((edtp = sdi_rxedt(c, b, t, l)) == NULL)
			continue;

		if (PD_STAMPMATCH(&edtp->stamp, stamp)) {
			ret_val = index;
			break;
		}
	}
	return(ret_val);
}

/*
 * int
 * sd01_putStamp(struct disk *dp, struct pd_stamp *dmstamp, daddr_t stampAddr)
 * 
 *	This function writes the incore physical descriptor information to the
 *	disk while preserving other data stored in the same disk sector as the
 *	PDINFO.
 */
int
sd01_putStamp(struct disk *dp, struct pd_stamp *dmstamp, daddr_t stampAddr)
{
	struct disk_media *dm = dp->dk_dm;
	uchar_t *secbuf;		/* Buffer for disk sector */
	struct phyio phyarg;		/* Physical I/O */
	struct pdinfo *pdptr;
	int ret_val;

	/* Read in the disk sector containing the PDINFO into sector buffer */
	if ((secbuf = sdi_kmem_alloc_phys(BLKSZ(dp),
			dp->dk_bcbp->bcb_physreqp, 0)) == NULL) {
		cmn_err(CE_WARN, "Disk Driver: Unable to allocate sector buffer for PDINFO");
		return(ENOMEM);
	}
	phyarg.memaddr = (ulong_t) secbuf;
	phyarg.sectst = stampAddr;
	phyarg.datasz = BLKSZ(dp);
	if ((ret_val = sd01phyrw(dp, (long)V_RDABS, &phyarg, SD01_KERNEL)) != 0) {
		kmem_free(secbuf, 
			BLKSZ(dp)+SDI_PADDR_SIZE(dp->dk_bcbp->bcb_physreqp));
		return(ret_val);
	}

	/* Lay down the new stamp */
	pdptr = (struct pdinfo *) secbuf;
	bcopy((caddr_t)dmstamp, pdptr->serial, sizeof(struct pd_stamp));

	/* Write out sector buffer to disk and return I/O status */
	phyarg.memaddr = (ulong_t) secbuf;
	phyarg.sectst = stampAddr;
	phyarg.datasz = BLKSZ(dp);
	if ((ret_val = sd01phyrw(dp, (long)V_WRABS, &phyarg, SD01_KERNEL)) != 0) {

		kmem_free(secbuf, 
			BLKSZ(dp)+SDI_PADDR_SIZE(dp->dk_bcbp->bcb_physreqp));
		return(ret_val);
	}

	kmem_free(secbuf, 
			BLKSZ(dp)+SDI_PADDR_SIZE(dp->dk_bcbp->bcb_physreqp));
	return(0);
}

/*
 * int
 * sd01_dkinit(struct disk *dp, struct owner *op, int diskidx)
 *	Allocate dm_lock, dk_lock, and initialize disk media and disk path
 *	structures.
 */
int
sd01_dkinit(struct disk *dk, struct owner *op, int diskidx)
{
	struct disk_media *dm = dk->dk_dm;
	struct sb *sbp;
	struct xsb *xsb;

	if ( !(dk->dk_lock = LOCK_ALLOC(TARGET_HIER_BASE, pldisk,
		&sd01dk_lkinfo, sdi_sleepflag)) ||
	     !(dm->dm_lock = LOCK_ALLOC(TARGET_HIER_BASE+1, pldisk,
		&sd01dm_lkinfo, sdi_sleepflag)) ||
	     !(dm->dm_sv = SV_ALLOC(sdi_sleepflag)) ) {
		/*
		 *+ Insufficient memory to allocate disk data structures
		 *+ when loading driver.
		 */
		cmn_err(CE_WARN, "sd01: dkinit allocation failed\n");
		return (0);
	}

	dm->dm_refcount = 1;

	dm->dm_ident = op->edtp->edt_ident;

	dk->dk_queue = dk->dk_lastlow = dk->dk_lasthi = (struct job *) NULL;
	dm->dm_parms.dp_secsiz    = KBLKSZ;
 	dk->dk_stat  = met_ds_alloc_stats("sd01", 
		diskidx,
		dm->dm_parms.dp_heads * 
		dm->dm_parms.dp_cyls *
		dm->dm_parms.dp_sectors,
		(uint)(MET_DS_BLK|MET_DS_NO_RESP_HIST|MET_DS_NO_ACCESS_HIST));
	dm->dm_iotype = op->edtp->iotype;

	/*
	 * Allocate a scsi_extended_adr structure
	 */
	dk->dk_addr.extended_address = 
		(struct sdi_extended_adr *)kmem_zalloc(sizeof(struct sdi_extended_adr), sdi_sleepflag);
	
	if ( ! dk->dk_addr.extended_address ) {
		cmn_err(CE_WARN, "Disk driver: insufficient memory" 
					"to allocate extended address");
		return(0);
	}
					
	dk->dk_addr.pdi_adr_version = sdi_ext_address(op->EDT_C);
	if (! dk->dk_addr.pdi_adr_version ) {
		dk->dk_addr.sa_ct = SDI_SA_CT(op->EDT_C, op->EDT_T);
		dk->dk_addr.sa_exta = (uchar_t)(op->EDT_T);
		dk->dk_addr.sa_lun = op->EDT_L;
		dk->dk_addr.sa_bus = op->EDT_B;
	}
	else
		dk->dk_addr.extended_address->scsi_adr = op->edtp->scsi_adr;

	dk->dk_addr.extended_address->version = SET_IN_TARGET;

	if (sdi_hba_flag(op->EDT_C) & HBA_TIMEOUT_RESET)
		dk->dk_timeout_table = op->edtp->timeout;

	/*
	 * Call to initialize the breakup control block.
	 */
	if ((dk->dk_bcbp = sdi_xgetbcb(dk->dk_addr.pdi_adr_version,
			&dk->dk_addr, sdi_sleepflag)) == NULL) {
		/*
		 *+ Insufficient memory to allocate disk breakup control
		 *+ block data structure when loading driver.
		 */
 		cmn_err(CE_NOTE, "Disk Driver: insufficient memory to allocate breakup control block.");
		return (0);
	}

	if ((dk->dk_bcbp_max = sdi_xgetbcb(dk->dk_addr.pdi_adr_version,
			&dk->dk_addr, sdi_sleepflag)) == NULL) {
		/*
		 *+ Insufficient memory to allocate maxxfer breakup control
		 *+ block data structure when loading driver.
		 */
 		cmn_err(CE_NOTE, "Disk Driver: insufficient memory to allocate maxxfer breakup control block.");
		return (0);
	}

	if ((dk->dk_ms_data = sdi_kmem_alloc_phys(FPGSZ,
			dk->dk_bcbp->bcb_physreqp, 0)) == NULL) {
		cmn_err(CE_NOTE, "Disk Driver: insufficient memory to allocate Mode Sense or Select data.");
                return (0);
	}


	if ((dk->dk_rc_data = sdi_kmem_alloc_phys(RDCAPSZ,
			dk->dk_bcbp->bcb_physreqp, 0)) == NULL) {
		cmn_err(CE_NOTE, "Disk Driver: insufficient memory to allocate Read Capacity data.");
                return (0);
	}

	if ((dk->dk_sense = sdi_kmem_alloc_phys(sizeof(struct sense),
                        dk->dk_bcbp->bcb_physreqp, 0)) == NULL) {
                cmn_err(CE_NOTE, 
			"Disk Driver: insufficient memory to allocate Sense structure.");
                return (0);
	}

	if ((dm->dm_dl_data = sdi_kmem_alloc_phys(RABLKSSZ,
			dk->dk_bcbp->bcb_physreqp, 0)) == NULL) {
		cmn_err(CE_NOTE, "Disk Driver: insufficient memory to allocate Defect List data.");
                return (0);
	}

	/* Initialize disk structure (done only once per dk */
	dk->dk_fltsus  = sdi_getblk(KM_SLEEP);	/* Suspend       */
	dk->dk_fltreq  = sdi_getblk(KM_SLEEP);	/* Request Sense */
	dk->dk_fltres  = sdi_getblk(KM_SLEEP);	/* Reserve       */
	dm->dm_bbhblk =	sdi_getblk(KM_SLEEP);	/* Read/Write Block*/
	dm->dm_bbhmblk = sdi_getblk(KM_SLEEP);	/* Reassign Block*/
	dk->dk_fltfqblk = sdi_getblk(KM_SLEEP);	/* Flush Queue   */

	dk->dk_gauntlet.g_req_sb = sdi_getblk(KM_SLEEP);
	dk->dk_gauntlet.g_tur_sb = sdi_getblk(KM_SLEEP);
	dk->dk_gauntlet.g_dev_sb = sdi_getblk(KM_SLEEP);
	dk->dk_gauntlet.g_bus_sb = sdi_getblk(KM_SLEEP);

	dm->dm_bbhjob = sd01getjob(dk, KM_SLEEP, NULL);
	dm->dm_bbhbuf = getrbuf(KM_SLEEP);

	dk->dk_gauntlet.g_job = sd01getjob(dk, KM_SLEEP, NULL);
	dk->dk_gauntlet.g_job->j_dk = dk;

	dk->dk_fltsus->sb_type  = SFB_TYPE;
	dk->dk_fltreq->sb_type  = ISCB_TYPE;
	dk->dk_fltres->sb_type  = ISCB_TYPE;
	dm->dm_bbhmblk->sb_type = ISCB_TYPE;
	dk->dk_fltfqblk->sb_type = SFB_TYPE;

	dk->dk_gauntlet.g_req_sb->sb_type = ISCB_TYPE;
	dk->dk_gauntlet.g_tur_sb->sb_type = ISCB_TYPE;
	dk->dk_gauntlet.g_dev_sb->sb_type = SFB_TYPE;
	dk->dk_gauntlet.g_bus_sb->sb_type = SFB_TYPE;
	
	xsb = (struct xsb *)dk->dk_fltreq;
	xsb->extra.sb_data_type = SDI_PHYS_ADDR;
	dk->dk_fltreq->SCB.sc_datapt = SENSE_AD(dk->dk_sense);
	dk->dk_fltreq->SCB.sc_datasz = SENSE_SZ;

	xsb->extra.sb_datapt = 
		(paddr32_t *) ( (char *)dk->dk_sense + sizeof(struct sense));
	*(xsb->extra.sb_datapt) += 1;
	dk->dk_fltreq->SCB.sc_mode   = SCB_READ;
	dk->dk_fltreq->SCB.sc_cmdpt  = SCS_AD(&dk->dk_fltcmd);
	dk->dk_fltreq->SCB.sc_dev    = dk->dk_addr;
	sdi_xtranslate(dk->dk_fltreq->SCB.sc_dev.pdi_adr_version,
			dk->dk_fltreq, B_READ, NULL, KM_SLEEP);

	dk->dk_fltres->SCB.sc_datapt = NULL;
	dk->dk_fltres->SCB.sc_datasz = 0;
	dk->dk_fltres->SCB.sc_mode   = SCB_WRITE;
	dk->dk_fltres->SCB.sc_cmdpt  = SCS_AD(&dk->dk_fltcmd);
	dk->dk_fltres->SCB.sc_dev    = dk->dk_addr;
	sdi_xtranslate(dk->dk_fltres->SCB.sc_dev.pdi_adr_version,
		dk->dk_fltres, B_WRITE,NULL, KM_SLEEP);

	xsb = (struct xsb *)dm->dm_bbhmblk;
	xsb->extra.sb_data_type = SDI_PHYS_ADDR;
	dm->dm_bbhmblk->SCB.sc_datapt = dm->dm_dl_data;
	dm->dm_bbhmblk->SCB.sc_datasz = RABLKSSZ;
	xsb->extra.sb_datapt = (paddr32_t *)
		((char *) dm->dm_dl_data + RABLKSSZ);
	dm->dm_bbhmblk->SCB.sc_mode   = SCB_WRITE;
	dm->dm_bbhmblk->SCB.sc_dev    = dk->dk_addr;
	sdi_xtranslate(dm->dm_bbhmblk->SCB.sc_dev.pdi_adr_version, 
				dm->dm_bbhmblk, B_WRITE, NULL, KM_SLEEP);

	xsb = (struct xsb *)dk->dk_gauntlet.g_req_sb;
	xsb->extra.sb_data_type = SDI_PHYS_ADDR;
	sbp = dk->dk_gauntlet.g_req_sb;
	sbp->SCB.sc_cmdsz  = SCS_SZ;
	sbp->SCB.sc_datapt = SENSE_AD(dk->dk_sense);
	sbp->SCB.sc_datasz = SENSE_SZ;
	xsb->extra.sb_datapt = (paddr32_t *)
		((char *)dk->dk_sense + sizeof (struct sense));

	sbp->SCB.sc_mode   = SCB_READ;
	sbp->SCB.sc_cmdpt  = SCS_AD(&dk->dk_gauntlet.g_scs);
	sbp->SCB.sc_dev    = dk->dk_addr;
	sbp->SCB.sc_wd     = (int) dk->dk_gauntlet.g_job;
	sdi_xtranslate(sbp->SCB.sc_dev.pdi_adr_version,sbp, B_READ, NULL, KM_SLEEP);

	sbp = dk->dk_gauntlet.g_tur_sb;
	sbp->SCB.sc_cmdsz = SCS_SZ;
	sbp->SCB.sc_datapt = 0;
	sbp->SCB.sc_datasz = 0;
	sbp->SCB.sc_mode   = SCB_READ;
	sbp->SCB.sc_cmdpt  = SCS_AD(&dk->dk_gauntlet.g_scs);
	sbp->SCB.sc_dev    = dk->dk_addr;
	sbp->SCB.sc_wd     = (int) dk->dk_gauntlet.g_job;
	sdi_xtranslate(sbp->SCB.sc_dev.pdi_adr_version,sbp, B_READ, NULL, KM_SLEEP);

	sbp = dk->dk_gauntlet.g_bus_sb;
	sbp->SFB.sf_dev = dk->dk_addr;
	sbp->SFB.sf_wd  = (int)dk->dk_gauntlet.g_job;

	sbp = dk->dk_gauntlet.g_dev_sb;
	sbp->SFB.sf_dev = dk->dk_addr;
	sbp->SFB.sf_wd  = (int)dk->dk_gauntlet.g_job;

	dk->dk_state |= DKINIT;

	return (1);
}

/*
 * Decrement disk media reference count.  When reference count is 0, frees the
 * disk media structure and all locks associated with it.
 */
void
sd01_dm_free(struct disk_media *dm)
{
	SD01_DM_LOCK(dm);
	--dm->dm_refcount;
	if (dm->dm_refcount <= 0) {
		SD01_DM_UNLOCK(dm);
		LOCK_DEALLOC(dm->dm_lock);
		SV_DEALLOC(dm->dm_sv);
 		if (dm->dm_partition)
			kmem_free(dm->dm_partition, dm->dm_nslices *
				sizeof(struct partition));
		if (dm->dm_altcount)
			kmem_free(dm->dm_altcount, dm->dm_nslices *
				sizeof(int));
		if (dm->dm_firstalt)
			kmem_free(dm->dm_firstalt, dm->dm_nslices *
				sizeof(struct alt_ent *));

		kmem_free(dm, sizeof(struct disk_media));
		return;
	}
	SD01_DM_UNLOCK(dm);
}

/************************************************************
 *  Frees a disk structure and all locks associated with it.
 ************************************************************/
void sd01_dk_free(struct disk *dp)
{
	struct disk_media *dm = dp->dk_dm;

	LOCK_DEALLOC(dp->dk_lock);
	met_ds_dealloc_stats(dp->dk_stat);
	sdi_freebcb(dp->dk_bcbp);
	if (dp->dk_timeout_id)
		untimeout(dp->dk_timeout_id);
	if (dp->dk_timeout_list != NULL)
		kmem_free(dp->dk_timeout_list, sizeof(struct job *) *sd01_timeslices);
	if (dp->dk_timeout_snapshot != NULL)
		kmem_free(dp->dk_timeout_snapshot, sizeof(struct job *) *sd01_timeslices);

	sd01_dm_free(dm);
	kmem_free(dp, sizeof(struct disk));
}

/*
 * struct job *
 * sd01getjob(struct disk *dk, int sleepflag, struct buf *bp)
 *	This function calls sdi_get for struct to be used as a job
 *	structure. If dyn alloc routines cannot alloc a struct the
 *	sdi_get routine will sleep until a struct is available. Upon a
 *	successful return from sdi_get, sd01getjob will then
 *	get a scb from the sdi using sdi_getblk.
 * Called by: sd01strategy and mdstrategy
 * Side Effects: 
 *	A job structure and SCSI control block is allocated.
 * Returns: A pointer to the allocated job structure.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
struct job *
sd01getjob(struct disk *dk, int sleepflag, struct buf *bp)
{
	register struct job *jp;

	if ((bp != NULL) && (bp->b_flags & _B_SDIBLKS)) {
		jp = (struct job *)(bp + 1);
		bzero(jp, 2 * lg_poolhead.f_isize);
		jp->j_cont = (struct sb *)((char *)jp + lg_poolhead.f_isize);
		sdi_initblk(jp->j_cont);
	} else {
		jp = (struct job *) sdi_getpair(sleepflag);
		jp->j_cont = (struct sb *)((char *)jp + lg_poolhead.f_isize);
	}
	jp->j_cont->sb_type = SCB_TYPE;
	jp->j_cont->SCB.sc_dev = dk->dk_addr;
	return(jp);
}

/*
 * void
 * sd01freejob(struct job *jp)
 *	The routine calls sdi_freeblk to return the scb attached to 
 *	the job structure driver. This function then returns the job 
 *	structure to the dynamic allocation free list by calling sdi_free. 
 * Called by: mddone sd01done
 * Side Effects:
 *	Allocated job and scb structures are returned.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
void
sd01freejob(struct job *jp)
{
	if ((jp->j_bp != NULL) && (jp->j_bp->b_flags & _B_SDIBLKS))
		sdi_hba_xfreeblk(jp->j_cont->SCB.sc_dev.pdi_adr_version, 
				jp->j_cont);
	else
		sdi_freepair((jpool_t *)jp);
}

/*
 * STATIC struct job *
 * sd01_dequeue(struct disk *dk)
 *
 * Remove and return the job at the head of the dk queue
 *
 * Call/Exit State:
 *
 *	The disks dk_lock must be held upon entry
 *	The lock is held upon exit
 */
STATIC struct job *
sd01_dequeue(struct disk *dk)
{
	struct job *jp = dk->dk_queue;
	if (jp) {
		dk->dk_queue = jp->j_next;
		if (jp == dk->dk_lastlow)
			dk->dk_lastlow = NULL;
		else if (jp == dk->dk_lasthi)
			dk->dk_lasthi = NULL;
	}
	return jp;
}


/* STATIC void
 * sd01sendq(struct disk *dk, int sleepflag)
 *
 * Send all the jobs on the queue 
 *
 * Call/Exit State:
 *      dk_lock must be held upon entry
 *      The lock released before exit
 */
STATIC void
sd01sendq(struct disk *dk, int sleepflag)
{
	struct job *jp;

	if (dk->dk_state & DKSEND) {
		long sid;
		sid = dk->dk_sendid;
		dk->dk_state &= ~DKSEND;
		SD01_DK_UNLOCK(dk);
		untimeout(sid);
		SD01_DK_LOCK(dk);
	}
	while ((jp = sd01_dequeue(dk)) != NULL) {
		if (!sd01send(dk, jp, sleepflag))
			return;
		SD01_DK_LOCK(dk);
	}
	SD01_DK_UNLOCK(dk);
}

/*
 * int
 * sd01send(struct disk *dk, struct job *, int sleepflag)
 *	This function sends a job to the host adapter driver.
 *	If the job cannot be accepted by the host adapter
 *	driver, the function will reschedule itself via the timeout
 *	mechanism. This routine must be called at pldisk.
 *
 * Called by: sd01cmd sd01strat1 sd01sendt
 * Side Effects: 
 *	Jobs are sent to the host adapter driver.
 * Return:
 *	non-zero if another job can be accepted
 *	zero if no more jobs should be sent immediately
 *
 * ERRORS:
 *	The host adapter rejected a request from the SCSI disk driver.
 *	This is caused by a parameter mismatch within the driver. The system
 *	should be rebooted.
 *
 * Calling/Exit State:
 *	The disks dk_lock must be held on entry
 *	Releases the disk struct's dk_lock.
 */
int
sd01send(struct disk *dk, struct job *jp,  int sleepflag)
{
	int sendret;		/* sdi_send return value 	*/ 
	struct xsb *xsb;
#ifdef SD01_DEBUG
	dk->dk_outcnt++;
	if (sd01alts_debug&DKADDR) 
		cmn_err(CE_CONT,"sd01send: sleepflag= %d \n", sleepflag);
#endif

	/* Update performance stats */
	met_ds_queued(dk->dk_stat, jp->j_bp->b_flags);
	SD01_DK_UNLOCK(dk);

	/* Swap bytes in the address field */
	if (jp->j_cont->SCB.sc_cmdsz == SCS_SZ)
		jp->j_cmd.cs.ss_addr = sdi_swap16(jp->j_cmd.cs.ss_addr);
	else {
		jp->j_cmd.cm.sm_addr = sdi_swap32(jp->j_cmd.cm.sm_addr);
		jp->j_cmd.cm.sm_len  = (short)sdi_swap16(jp->j_cmd.cm.sm_len);
	}

	xsb = (struct xsb *)jp->j_cont;
	if (xsb->extra.sb_data_type == SDI_PHYS_ADDR) {
		if (jp->j_cmd.cs.ss_op != SS_REQSEN) {
			xsb->extra.sb_datapt = (paddr32_t *)
					((char *)jp->j_cont->SCB.sc_datapt +
						jp->j_cont->SCB.sc_datasz);
		} else {
			xsb->extra.sb_datapt =
				(paddr32_t *) ( ((char *)jp->j_cont->SCB.sc_datapt - 1) + 
						jp->j_cont->SCB.sc_datasz);
		}
	}

	sendret = sdi_xtranslate(jp->j_cont->SCB.sc_dev.pdi_adr_version,
				jp->j_cont, jp->j_bp->b_flags,
			jp->j_bp->__b_proc, sleepflag);

	if (sendret == SDI_RET_OK) {

		if (jp->j_cont->sb_type == ISCB_TYPE)
			sendret = sdi_xicmd(jp->j_cont->SCB.sc_dev.pdi_adr_version, 				jp->j_cont, sleepflag);
		else
			sendret = sdi_xsend(jp->j_cont->SCB.sc_dev.pdi_adr_version,
				jp->j_cont, sleepflag);

	}
	if (sendret != SDI_RET_OK) {

		if (sendret == SDI_RET_RETRY) {
			SD01_DK_LOCK(dk);
#ifdef SD01_DEBUG
			dk->dk_outcnt--;
#endif
			met_ds_dequeued(dk->dk_stat, jp->j_bp->b_flags);
			/* Swap bytes back in the address field	*/
			if (jp->j_cont->SCB.sc_cmdsz == SCS_SZ)
				jp->j_cmd.cs.ss_addr = sdi_swap16(
						jp->j_cmd.cs.ss_addr);
			else {
				jp->j_cmd.cm.sm_addr = sdi_swap32(
						jp->j_cmd.cm.sm_addr);
				jp->j_cmd.cm.sm_len  = (short)sdi_swap16
						(jp->j_cmd.cm.sm_len);
			}
			/* relink back to the queue */
			sd01queue_hi(jp,dk);

			/* do not hold the job queue busy any more */
			/* Call back later */
			if (!(dk->dk_state & DKSEND)) {
				dk->dk_state |= DKSEND;
				if ( !(dk->dk_sendid =
				sdi_timeout(sd01sendt, (caddr_t) dk, 1, pldisk,
				   &dk->dk_addr))) {
					dk->dk_state &= ~DKSEND;
					/*
					 *+ sd01 send routine could not
					 *+ schedule a job for retry
					 *+ via itimeout
					 */
					cmn_err(CE_WARN, "sd01send: itimeout failed");
				}
			}

			SD01_DK_UNLOCK(dk);

			return 0;
		} else {
#ifdef SD01_DEBUG
			cmn_err(CE_CONT, "Disk Driver: Bad type to HA");
#endif
			sd01comp1(jp);
		}
	}
	return 1;
}

/*
 * void
 * sd01sendt(struct disk *dk)
 *	This function call sd01sendq after it turns off the DKSEND
 *	bit in the disk status work.
 * Called by: timeout
 * Side Effects: 
 *	The send function is called and the record of the pending
 *	timeout is erased.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 */
void
sd01sendt(struct disk *dk)
{
	int state;

#ifdef DEBUG
        DPR (1)("sd01sendt: (dk=0x%x)\n", dk);
#endif

	SD01_DK_LOCK(dk);

	state = dk->dk_state;
	dk->dk_state &= ~DKSEND;
	

	/* exit if job queue is suspended or the timeout has been cancelled */
	if ((state & (DKSUSP|DKSEND)) != DKSEND) {
		SD01_DK_UNLOCK(dk);
		return;
	}
	sd01sendq(dk, KM_NOSLEEP);

#ifdef DEBUG
        DPR (2)("sd01sendt: - exit\n");
#endif
}

/*
 * void
 * sd01strat1(struct job *jp, struct disk *dk, buf_t *bp)
 *	Level 1 (Core) strategy routine.
 *	This function takes the information included in the job
 *	structure and the buffer header, and creates a SCSI bus
 *	request.  The request is placed on the priority queue.
 *	This routine may be called at interrupt level. 
 *	The buffer and mode fields are filled in by the calling 
 *	functions.  If the partition argument is equal to 16 or
 *	greater the block address is assumed to be physical.
 * Called by: sd01strategy and sd01phyrw.
 * Side Effects: 
 *	A job is queued for the disk.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 */
void
sd01strat1(struct job *jp, struct disk *dk, buf_t *bp)
{
	register struct scb *scb;
	register struct scm *cmd;
	struct xsb *xsb;
	int sleepflag = KM_SLEEP;

#ifdef DEBUG
        DPR (1)("sd01strat1: jp=0x%x\n", jp);
#endif
	
	xsb = (struct xsb *)jp->j_cont;
	if (xsb->extra.sb_data_type != SDI_PHYS_ADDR) {
		if (bp->b_priv.un_int == SDI_PHYS_ADDR) {
			xsb->extra.sb_data_type = SDI_PHYS_ADDR;
			xsb->extra.sb_datapt =
		   	   (paddr32_t *) ((char *)paddr(bp) + bp->b_bcount);
		}
		else {
			xsb->extra.sb_bufp = bp;
			xsb->extra.sb_data_type = SDI_IO_BUFFER;
		}
	}
	scb = &jp->j_cont->sb_b.b_scb;
	cmd = &jp->j_cmd.cm;
	
	jp->j_dk = dk;
	jp->j_bp = bp;
	jp->j_done = sd01done;
	scb->sc_datapt = jp->j_memaddr;
	scb->sc_datasz = jp->j_seccnt << BLKSHF(dk);
	scb->sc_cmdpt = SCM_AD(cmd);

	/* Fill in the scb for this job */
	if (bp->b_flags & B_READ) {
		cmd->sm_op = SM_READ;
		scb->sc_mode = SCB_READ;
	} else {
		cmd->sm_op = SM_WRITE;
		scb->sc_mode = SCB_WRITE;
	}
	if (! dk->dk_addr.pdi_adr_version )
		cmd->sm_lun = dk->dk_addr.sa_lun; 
			/* dk_addr - read only once set */
	else
		cmd->sm_lun = dk->dk_addr.extended_address->scsi_adr.scsi_lun;
	cmd->sm_res1 = 0;
	cmd->sm_addr = jp->j_daddr;
	cmd->sm_res2 = 0;
	cmd->sm_len = jp->j_seccnt;
	cmd->sm_cont = 0;
	scb->sc_cmdsz = SCM_SZ;
	/* The data elements are filled in by the calling routine */
	scb->sc_link = 0;
	scb->sc_time = sd01_get_timeout(cmd->sm_op, dk);
	scb->sc_int = sd01intn;
	scb->sc_wd = (long) jp;
	scb->sc_dev = dk->dk_addr;
	
	if (jp->j_flags & J_FIFO)
		sleepflag = KM_NOSLEEP;

	SD01_DK_LOCK(dk);

	if (dk->dk_queue) {
		sd01queue_low(jp, dk);
		sd01sendq(dk, sleepflag);
	}
	else {
		sd01send(dk, jp, sleepflag);
	}
	

#ifdef DEBUG
        DPR (2)("sd01strat1: - exit\n");
#endif
}

/*
 * void
 * sd01queue_hi(struct job *jp, struct disk *dk)
 *	Add high-priority job to queue
 *
 * Calling/Exit State:
 *	caller holds the disk struct's dk_lock
 */
void
sd01queue_hi(struct job *jp, struct disk *dk)
{
	/* No high priority jobs on queue: stick job in front
	 * in front of (possibly empty) queue.
	 */
	if (!dk->dk_lasthi) {
		jp->j_next = dk->dk_queue;
		dk->dk_lasthi = dk->dk_queue = jp;
		return;
	} 
	/* High priority jobs on queue: append this job to the
	 * existing high priority jobs.  This code should not
	 * be reached because sd01queue_hi() is only called for
	 * retry jobs, and we only retry one job at a time.
	 */
	jp->j_next = dk->dk_lasthi->j_next;
	dk->dk_lasthi->j_next = jp;
	dk->dk_lasthi = jp;
	return;
}

/*
 * void
 * sd01queue_low(struct job *jp, struct disk *dk)
 *	add low priority job to end of queue
 *
 * Calling/Exit State:
 *	Caller holds disk struct's dk_lock.
 */
void
sd01queue_low(struct job *jp, struct disk *dk)
{
	jp->j_next = NULL;
	if (!dk->dk_queue) {
		/* Add first job to queue */
		dk->dk_queue = dk->dk_lastlow = jp;
		return;
	}
	if (dk->dk_lastlow) {
		/* Append to low priority jobs */
		dk->dk_lastlow = dk->dk_lastlow->j_next = jp;
		return;
	}
	/* Add first low priority job to queue */
	dk->dk_lastlow = dk->dk_lasthi->j_next = jp;
	return;
}

/*
 * void
 * sd01biostart8(void *idatap, ulong_t channel, buf_t *bp)
 *	sd01biostart8  generates the SCSI command needed to fulfill the
 *	I/O request.  The buffer pointer is passed from the kernel and
 *	contains the necessary information to perform the job.  Most of the
 *	work is done by sd01gen_sjq.
 * Called by: any ddi8 driver
 * Side Effects: 
 *	An I/O job is added to the work queue. 
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk media struct's dm_lock
 */
void
sd01biostart8(void *idatap, ulong_t channel, buf_t *bp)
{
	struct disk *dk;
	struct disk_media *dm;

	dk = (struct disk *)idatap;
	ASSERT(dk);
	dm = dk->dk_dm;
	ASSERT(dm);

	bp->b_resid = bp->b_bcount;

	if (sdi_get_blkno(bp) < 0) {
		bioerror(bp, ENXIO);
		biodone(bp);
		return;
	}

	if (bp->b_bcount == 0)
	{	/* The request is done */
		biodone(bp);
		return;
	}

#ifdef SD01_DEBUG
	if(sd01alts_debug & STFIN) {
		cmn_err(CE_CONT,"Entering sd01ck_badsec\n");
	}
#endif

	/* Lightweight check before loop.  If the lightweight check
	 * says DMMAPBAD we wait for the SV_BROADCAST.  If the lightweight
	 * check fails, then the locked check will be equally likely
	 * to fail.  We are not really synchronizing anything here.
	 */
	if (!sd01_lightweight || dm->dm_state & DMMAPBAD) {
		SD01_DM_LOCK(dm);
		/* Wait until dynamic bad sector mapping has been completed */
		while(dm->dm_state & DMMAPBAD)	{
			SV_WAIT(dm->dm_sv, pridisk, dm->dm_lock);
			SD01_DM_LOCK(dm);
		}
		SD01_DM_UNLOCK(dm);
	}

	/* Keep start time */
	drv_getparm(LBOLT, &(bp->b_start));

	sd01gen_sjq(dk, bp, SD01_NORMAL, NULL);

#ifdef SD01_DEBUG
	if(sd01alts_debug & STFIN) {
		cmn_err(CE_CONT,"Leaving sd01ck_badsec\n");
	}
#endif

#ifdef DEBUG
        DPR (2)("sd01biostart8: - exit\n");
#endif
}

/*
 * int
 * sd01close8(void *idatap, ulong_t channel, int flags, int otype, struct cred *cred_p)
 *	Clear the open flags.
 * Called by: Kernel
 * Side Effects: 
 *	The device is marked as unopened.
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 */
/*ARGSUSED*/
int
sd01close8(void *idatap, ulong_t channelp, int flags, cred_t *cred_p, queue_t *dummy_arg)
{
 	struct disk *dk;
 	struct disk_media *dm;
	int err;

 	dk = (struct disk *)idatap;

	ASSERT(dk);

	dm = dk->dk_dm;

	ASSERT(dm);

#ifdef DEBUG
	if (flags & O_DEBUG) { /* For DEBUGFLG ioctl */
		return(0);
	}
	DPR (1)("sd01close: (dk=0x%x flags=0x%x dm=0x%x cred_p=0x%x)\n", dk, flags, dm, cred_p);
#endif

	SD01_DM_LOCK(dm);

	dm->dm_openCount = 0;

check_last:
	/* This is last close */
	if (dm->dm_state & DM_FO_LC) {
		while (dm->dm_state & DM_FO_LC) {
			SV_WAIT(dm->dm_sv, pridisk, dm->dm_lock);
			SD01_DM_LOCK(dm);
		}
		goto check_last;
	}
	dm->dm_state |= DM_FO_LC;
	SD01_DM_UNLOCK(dm);

	/*
	 * This is here for Compaq ProLiant Storage System support.  It could
	 * be ifdef'ed for performance
	 */
	SD01_DK_LOCK(dk);
	if (dk->dk_state & DK_RDWR_ERR)	{
		err = TRUE;
		dk->dk_state &= ~DK_RDWR_ERR;
	}
	else
		err = FALSE;
	SD01_DK_UNLOCK(dk);

	(void)sd01_last_close(dk, err);

	SD01_DM_LOCK(dm);
	dm->dm_state &= ~DM_FO_LC;
	SD01_DM_UNLOCK(dm);

	SV_BROADCAST(dm->dm_sv, 0);	/* -> sd01open, sd01close */

#ifdef DEBUG
	DPR (2)("sd01close8: - exit(0)\n");
#endif

	return(0);
}

/*
 * int
 * sd01_firstOpen8(struct disk *dk)
 *
 * This routine sends the appropiate commands to the disk so that it is
 * accessible and accordingly updates the dk structure reflecting this
 * operation.
 *
 * Calling/Exit status.
 * The DM_FO_LC flag is set on exit in the dm structure. This flag is used to
 * insure single thread execution through the open and close entry points.
 */
int
sd01_firstOpen8(struct disk *dk)
{
	struct disk_media *dm = dk->dk_dm;
	int ret_val;

	SD01_DM_LOCK(dm);

recheck:
	if (!dm->dm_openCount) {
		if (dm->dm_state & DM_FO_LC) {
			SV_WAIT(dm->dm_sv, pridisk, dm->dm_lock);
			SD01_DM_LOCK(dm);
			goto recheck;
		}
		dm->dm_state |= DM_FO_LC;

		SD01_DM_UNLOCK(dm);
		(void)sd01_first_open(dk);
		ret_val = sd01config8(dk);
		SV_BROADCAST(dm->dm_sv, 0);
		return ret_val;
	} else {
		SD01_DM_UNLOCK(dm);
		return 0;
	}
}

/*
 * int
 * sd01open8(void *idata, ulong_t *channelp, int flags, int otype, struct cred *cred_p)
 *
 * Initialize the underlying disk and if successful keep track of the opens.
 * On first open the execution must be single threaded. For this the DM_FO_LC
 * flag is used.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/*ARGSUSED*/
int
sd01open8(void *idatap, ulong_t *channelp, int flags, cred_t *cred_p, queue_t *dummy_arg)
{
	struct disk *dk = (struct disk *)idatap;
	struct disk_media *dm;
	int ret_val;

#ifdef DEBUG
	DPR (1)("\nsd01open8: (dev=0x%x flags=0x%x cred_p=0x%x)\n",
		idatap, flags, cred_p);
#endif

	ASSERT(dk);
	dm = dk->dk_dm;
	ASSERT(dm);

	if (ret_val = sd01_firstOpen8(dk)) {
		SD01_DM_LOCK(dm);
		if (dm->dm_state & DM_FO_LC) {
			dm->dm_state &= ~DM_FO_LC;
			SD01_DM_UNLOCK(dm);
			SV_BROADCAST(dm->dm_sv, 0);	/* -> sd01open, sd01close */
		} else {
			SD01_DM_UNLOCK(dm);
		}
	} else {
		/* Succesful open! */
		SD01_DM_LOCK(dm);

		/* Count opens */
		dm->dm_openCount += DMLYR;
		dm->dm_openCount |= DMGEN;

		if (dm->dm_state & DM_FO_LC) {
			dm->dm_state &= ~DM_FO_LC;
			SD01_DM_UNLOCK(dm);
			SV_BROADCAST(dm->dm_sv, 0);	/* -> sd01open, sd01close */
		} else {
			SD01_DM_UNLOCK(dm);
		}
	}

#ifdef DEBUG
	DPR (2)("\nsd01open8: - exit\n");
#endif
	return ret_val;
}

/*
 * void
 * sd01done(struct job *jp)
 *	This function completes the I/O request after a job is returned by
 *	the host adapter. It will return the job structure and call
 *	biodone.
 * Called by: sd01comp1
 * Side Effects: 
 *	The kernel is notified that one of its requests completed.
 *
 * Calling/Exit State:
 *      The disk struct's dk_lock must be held upon entry.
 *	No locks held on exit.
 *	Acquires disk_media struct's dm_lock.
 */
void
sd01done(struct job *jp)
{
	register buf_t *bp = jp->j_bp;
	register struct disk *dk = jp->j_dk;
	struct disk_media *dm = dk->dk_dm;
	char ops;
	int flags, error = 0;

#ifdef DEBUG
        DPR (1)("sd01done: (jp=0x%x)\n", jp);
#endif

	if (jp->j_cont->SCB.sc_comp_code != SDI_ASW) {
		/* Record the error for a normal job */
		if (jp->j_cont->SCB.sc_comp_code == SDI_NOTEQ)
			error = ENODEV;
		else if (jp->j_cont->SCB.sc_comp_code == SDI_QFLUSH)
			error = EFAULT;
		else if (jp->j_cont->SCB.sc_comp_code == SDI_OOS)
			error = EIO;
		else if (jp->j_cont->SCB.sc_comp_code == SDI_NOSELE)
			error = ENODEV;
		else if (jp->j_cont->SCB.sc_comp_code == SDI_CKSTAT && 
			 jp->j_cont->SCB.sc_status == S_RESER) {
			error = EBUSY;	/* Reservation Conflict */

			SD01_DM_LOCK(dm);
			dm->dm_state |= DMCONFLICT;
			SD01_DM_UNLOCK(dm);
		} else
			error = EIO;

		bioerror(bp, error);
	} else {
		bp->b_resid -= jp->j_seccnt << BLKSHF(dk);
	}
	

	if (error==EFAULT) {
		SD01_DK_UNLOCK(dk);
		/*  job returned from queue flush */
		if (!jp->j_fwmate && !jp->j_bkmate) {
			sd01gen_sjq(dk, bp, SD01_NORMAL|SD01_FIFO, NULL);
		} else {
			sd01releasejob(jp);
		}
		return;
	}

	ops = (char) jp->j_cont->SCB.sc_cmdpt;
	flags = jp->j_flags;

	if (sd01releasejob(jp) && !(flags & J_FREEONLY)) {
		/* Update performance stat */
		/* If this is a read or write, also update the I/O queue */
 		met_ds_iodone(dk->dk_stat, bp->b_flags,
				(bp->b_bcount - bp->b_resid));

		/*
		 * This is here for Compaq ProLiant Storage System support.  It could
		 * be ifdef'ed for performance
		 */
		if (ops == SM_READ || ops ==  SM_WRITE)	{
			if (error)
				dk->dk_state |= DK_RDWR_ERR;
			else
				dk->dk_state &= ~DK_RDWR_ERR;
		}

		SD01_DK_UNLOCK(dk);
		biodone(bp);
		if (flags & J_TIMEOUT)
			jp->j_flags |= J_FREEONLY;
	} else
		SD01_DK_UNLOCK(dk);

#ifdef DEBUG
        DPR (2)("sd01done: - exit\n");
#endif
}
/*
 * int
 * sd01releasejob(struct job *jp)
 *	unchain job from forward/backward mate, 
 *	and remove the job structure.
 *	return = 0, was part of a chain,
 *	return = 1, was not part of a chain.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
sd01releasejob(struct job *jp)
{
	int notchained = 1;

	if (jp->j_fwmate) {
		notchained = 0;
		jp->j_fwmate->j_bkmate = jp->j_bkmate;
	}
	if (jp->j_bkmate) {
		notchained = 0;
		jp->j_bkmate->j_fwmate = jp->j_fwmate;
	}

	/* if this job is being timed out, do not free resources
	 * in case we get a later interrupt.  If we get the interrupt
	 * while we are processing the call, we may not free the
	 * the job; this is an acceptable race condition.
	 */
	if ((jp->j_flags & (J_TIMEOUT|J_FREEONLY)) != J_TIMEOUT)
		sd01freejob(jp);
	else
		jp->j_fwmate = jp->j_bkmate = 0;

	return notchained;
}

/*
 * void
 * sd01comp1(struct job *jp)
 *	This function removes the job from the queue.  Updates the disk
 *	counts.  Restarts the logical unit queue if necessary, and prints an
 *	error for failing jobs.
 * Called by: sd01intn 
 * Side Effects: 
 *	Removes the job from the disk queue.
 *
 *	An I/O request failed due to an error returned by the host adpater.
 *	All recovery action failed and the I/O request was returned to the 
 *	requestor.  The secondary error code is equal to the SDI return
 *	code.  See the SDI error codes for more information.
 *
 *	A bad block was detected.  The driver can not read this block. A 
 *	Error Correction Code (ECC) was unsuccessful.  The data in this block
 *	has been lost.  The data can only be retrived from a previous backup.
 *
 * Calling/Exit State:
 *	No locks held on entry.
 *	Acquires dk_lock and dm_lock.
 */
void
sd01comp1(struct job *jp)
{
	register struct disk *dk;
	register struct disk_media *dm;

	dk = jp->j_dk;
	dm = dk->dk_dm;

#ifdef  SD01_DEBUG
	SD01_DK_LOCK(dk);
	dk->dk_outcnt--;
	SD01_DK_UNLOCK(dk);

	if (jp->j_error > 1)
		cmn_err(CE_CONT, "sd01comp1: retry count %d\n",
			jp->j_error);
#endif

	/* Log error if necessary */
	if (jp->j_cont->SCB.sc_comp_code != SDI_ASW) {
		if(jp->j_flags & J_HDEECC) {
			if(jp->j_flags & J_OREM) {
				jp->j_flags = 0;
				jp->j_cont->SCB.sc_comp_code = SDI_ASW;
			} else {
				if (!(jp->j_flags & J_NOERR))
				{
					sd01logerr(jp->j_cont, dk, 0x6dd0e002);
				}
			}
		} else
		{
			if (!(jp->j_flags & (J_NOERR|J_TIMEOUT)))
				sd01logerr(jp->j_cont, dk, 0x6dd0e001);
		}
	}

	if (jp->j_flags & J_BADBLK) {
		/* ECC occurred on previous read.
		 * If this read succeeded, remap and restore data just read.
		 * If this is a write, remap and retry the job.
		 */
		if (jp->j_bp->b_flags & B_READ) {
			if (jp->j_flags & J_HDEBADBLK) {
				jp->j_flags &= ~J_HDEBADBLK;
			} else {
				sd01_bbhndlr1(dk, jp, SD01_ECCDRD,jp->j_memaddr, jp->j_daddr);
				return;
			}
		} else {
			sd01_bbhndlr1(dk, jp, SD01_ECCDWR, NULL, jp->j_daddr);
			return;
		}
	}

	SD01_DK_LOCK(dk);
	if (jp->j_flags & J_HDEBADBLK) {
		daddr_t hdesec;                 /* Bad sector number    */

		/*
		 * Bad sector detected - Q has been suspended
		 */

		hdesec = dk->dk_sense->sd_ba;

		/* Check if bad sector is in non-UNIX area of the disk
		 * or outside this UNIX partition.
		 * Also check if it's in a special fdisk partition
		 */
		SD01_DM_LOCK(dm);
		if (hdesec < dm->dm_partition[0].p_start ||
		    hdesec >(dm->dm_partition[0].p_start + dm->dm_partition[0].p_size)){
			SD01_DM_UNLOCK(dm);

			/* Resume the Q if suspended */
			if (dk->dk_state & DKSUSP)
				(void) sd01qresume(dk);

			/* Clear bad block state */
			jp->j_flags &= ~J_HDEBADBLK;
		} else {
			/* Let the bad block handler take this */
			SD01_DM_UNLOCK(dm);
			SD01_DK_UNLOCK(dk);
			sd01_bbhndlr1(dk, jp, SD01_ECCRDWR, NULL, hdesec);
			return;
		}
	} else {
		SD01_DM_LOCK(dm);
		if (!(dm->dm_state & DMMAPBAD) && (dk->dk_state & DKSUSP)) {
			SD01_DM_UNLOCK(dm);
			(void) sd01qresume(dk);        /* Resume Q     */
		} else {
			SD01_DM_UNLOCK(dm);
		}
	}
 
	/* Start next job if necessary */
	if (!(dk->dk_state & (DKSUSP|DKSEND)) && dk->dk_queue){
		/* Use timeout to restart the queue */
		dk->dk_state |= DKSEND;
		SD01_DK_UNLOCK(dk);
		if ( !(dk->dk_sendid = sdi_timeout(sd01sendt, dk, 1, pldisk,
			&dk->dk_addr)) ) {

			dk->dk_state &= ~DKSEND;
			/*
			 *+ sd01 completion routine  could not
			 *+ schedule a job for retry via itimeout
			 */
			cmn_err(CE_WARN, "sd01comp1: itimeout failed");
		}
		SD01_DK_LOCK(dk);
	}
	jp->j_done(jp);			/* Call the done routine 	*/
}

/*
 * void
 * sd01intf(struct sb *sbp)
 *	This function is called by the host adapter driver when a host
 *	adapter function request has completed.  If the request completed
 *	without error, then the block is marked as free.  If there was error
 *	the request is retried.  Used for resume function completions.
 * Called by: Host Adapter driver.
 * Side Effects: None
 *
 *	A SCSI disk driver function request was retried.  The retry 
 *	performed because the host adapter driver detected an error.  
 *	The SDI return code is the second error code word.  See
 *	the SDI return codes for more information.
 *
 *	The host adapter rejected a request from the SCSI disk driver.
 *	This is caused by a parameter mismatch within the driver. The system
 *	should be rebooted.
 *
 *	A SCSI disk driver function request failed because  the host
 *	adapter driver detected a fatal error or the retry count was
 *	exceeded.  This failure will cause the effected unit to
 *	hang.  The system must be rebooted. The SDI return code is
 *	the second error code word.  See the SDI return codes for
 *	more information.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 */
void
sd01intf(struct sb *sbp)
{
	struct disk *dk;

	dk = (struct disk *)sbp->SFB.sf_priv;
	ASSERT(dk);

#ifdef DEBUG
        DPR (1)("sd01intf: (sbp=0x%x) dk=0x%x\n", sbp, dk);
#endif

	SD01_DK_LOCK(dk);

	if (sbp->SFB.sf_comp_code & SDI_RETRY &&
		dk->dk_spcount < sd01_retrycnt)
	{				/* Retry the function request */

		dk->dk_spcount++;
		dk->dk_error++;

		sd01logerr(sbp, dk, 0x4dd0f001);
		SD01_DK_UNLOCK(dk);

		if (sdi_xicmd(sbp->SFB.sf_dev.pdi_adr_version, sbp,
					 KM_NOSLEEP) == SDI_RET_OK)
			return;
		SD01_DK_LOCK(dk);
	}
	
	if (sbp->SFB.sf_comp_code != SDI_ASW)
	{
		sd01logerr(sbp, dk, 0x6dd0f003);
	}


	dk->dk_spcount = 0;		/* Zero retry count */
 
	/*
	*  Currently, only RESUME SFB uses this interrupt handler
	*  so the following block of code is OK as is.
	*/

	sd01_resume_pl = LOCK(sd01_resume_mutex, pldisk);
 	/* This disk LU has just been resumed */
 	sd01_resume.res_head = dk->dk_fltnext;
 
	/* Are there any more disk queues needing resuming */
 	if (sd01_resume.res_head == (struct disk *) &sd01_resume)
	{	/* Queue is empty */

		/*
		*  There is a pending resume for this device so 
		*  since the Q is empty, just put the device back
		*  at the head of the Q.
		*/
		if (dk->dk_state & DKPENDRES)
		{
			dk->dk_state &= ~DKPENDRES;
			sd01_resume.res_head = dk;
			UNLOCK(sd01_resume_mutex, sd01_resume_pl);
			sd01resume(sd01_resume.res_head);
		}
		/*
		*  The Resume has finished for this device so clear
		*  the bit indicating that this device was on the Q.
		*/
		else
		{
			dk->dk_state &= ~DKONRESQ;
 			sd01_resume.res_tail = (struct disk *) &sd01_resume;
			UNLOCK(sd01_resume_mutex, sd01_resume_pl);
		}
	}
 	else
	{	/* Queue not empty */

		/*  Is there another RESUME pending for this disk */
		if (dk->dk_state & DKPENDRES)
		{
			dk->dk_state &= ~DKPENDRES;

			/* Move Next Resume for this disk to end of Queue */
			sd01_resume.res_tail->dk_fltnext = dk;
			sd01_resume.res_tail = dk;
			dk->dk_fltnext = (struct disk *) &sd01_resume;
		}
		else	/* Resume next disk */
			dk->dk_state &= ~DKONRESQ;

		UNLOCK(sd01_resume_mutex, sd01_resume_pl);
		SD01_DK_UNLOCK(dk);
		dk = sd01_resume.res_head;
		SD01_DK_LOCK(dk);
 		sd01resume(sd01_resume.res_head);
	}

	SD01_DK_UNLOCK(dk);

#ifdef DEBUG
        DPR (2)("sd01intf: - exit\n");
#endif
}

/*
 * void
 * sd01retry(struct job *jp)
 *	Retries a failed job. 
 * Called by: sd01intf, sd01intn, and sd01reset.c
 * Side Effects: If necessary the que suspended bit is set.
 *
 * Errors:
 *	The host adapter rejected a request from the SCSI disk driver.
 *	This is caused by a parameter mismatch within the driver. The system
 *	should be rebooted.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires disk struct's dk_lock.
 */
void
sd01retry(struct job *jp)
{
	register struct sb *sbp;
	struct disk *dk;

	sbp = jp->j_cont;
	dk = jp->j_dk;

#ifdef DEBUG
        DPR (1)("sd01retry: (jp=0x%x) dk=0x%x\n", jp, dk);
#endif

	SD01_DK_LOCK(dk);
	
	jp->j_error++;			/* Update the error counts */
	dk->dk_error++;
	
	if (sbp->SCB.sc_comp_code & SDI_SUSPEND)
	{
		dk->dk_state |= DKSUSP;
	}

	SD01_DK_UNLOCK(dk);
		
	sbp->SCB.sc_time = sd01_get_timeout(jp->j_cmd.cm.sm_op, dk);
	sbp->sb_type = ISCB_TYPE;
	
	if (sdi_xicmd(sbp->SCB.sc_dev.pdi_adr_version,
			sbp, KM_NOSLEEP) != SDI_RET_OK)
	{
#ifdef DEBUG
		cmn_err(CE_CONT, "Disk Driver: Bad type to HA");
#endif
					/* Fail the job */
		sd01comp1(jp);
	}

#ifdef DEBUG
        DPR (2)("sd01retry: - exit\n");
#endif
}

/*
 * void
 * sd01intn(struct sb *sbp)
 *	Normal interrupt routine for SCSI job completion.
 *	This function is called by the host adapter driver when a
 *	SCSI job completes.  If the job completed normally the job
 *	is removed from the disk job queue, and the requester's
 *	completion function is called to complete the request.  If
 *	the job completed with an error the job will be retried when
 *	appropriate.  Requests which still fail or are not retried
 *	are failed and the error number is set to EIO.    Device and
 *	controller errors are logged and printed to the user
 *	console.
 * Called by: Host adapter driver
 * Side Effects: 
 *	Requests are marked as complete.  The residual count and error
 *	number are set if required.
 *
 * Errors:
 *	The host adapter rejected a request sense job from the SCSI 
 *	disk driver.  The originally failing job will also be failed.
 *	This is caused by a parameter mismatch within the driver. The system
 *	should be rebooted.
 *
 *	The SCSI disk driver is retrying an I/O request because of a fault
 *	which was detected by the host adapter driver.  The second error
 *	code indicates the SDI return code.  See the SDI return code for 
 *	more information as to the detected fault.
 *
 *	The addressed SCSI disk returned an unusual status.  The job
 *	will be  retried later.  The second error code is the status
 *	which was  returned.  This condition is usually caused by a
 *	problem in  the target controller.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires disk struct's dk_lock.
 */
void
sd01intn(struct sb *sbp)
{
	register struct disk *dk;
	struct job *jp;

	jp = (struct job *)sbp->SCB.sc_wd;
	dk = jp->j_dk;

	if ((sbp->SCB.sc_comp_code == SDI_ASW)
	   || (sbp->SCB.sc_comp_code == SDI_QFLUSH))
	{
#ifdef SD01_DEBUG
		if (sd01alts_debug & DKD || sd01alts_debug & HWREA) {
			/*
			 * This is part of the bad block hook to simulate an
			 * ECC error.  A good completion occurred, however,
			 * if the block specified in sd01alts_badspot, on disk
			 * dk_badspot is part of this job, then make it look
			 * as though the completion was CKSTAT.  Enter
			 * the gauntlet, and proceed to suspend the queue.
			 * The next intercept is in sd01ints.
			 */
			if (dk_badspot == dk && sd01alts_badspot &&
		    	(sd01alts_badspot >= jp->j_daddr) &&
		    	(sd01alts_badspot<(jp->j_daddr+jp->j_seccnt))) {
				/* Simulate an error */
				jp->j_cont->SCB.sc_comp_code = SDI_CKSTAT;
				sbp->SCB.sc_status = S_CKCON;
			}
		} else
#endif
                {
			if ((jp->j_flags & J_BADBLK) &&
			    !(jp->j_bp->b_flags & B_READ)) {
				/* ECC occurred on a previous read, and this
			 	* is a write, so suspend the queue since we are
			 	* going to handle the reassign/remap now.
			 	*/
				sd01flts(dk, jp);
				return;
			}
			sd01comp1(jp);
			return;
		}
	}
	
#ifdef DEBUG
        DPR (1)("sd01intn: (sbp=0x%x) dk=0x%x\n", sbp, dk);
#endif

	SD01_DK_LOCK(dk);

	if (sbp->SCB.sc_comp_code & SDI_SUSPEND)
	{
		dk->dk_state |= DKSUSP;	/* Note the queue is suspended */
	}

	if (sbp->SCB.sc_comp_code == SDI_TIME ||
	    sbp->SCB.sc_comp_code == SDI_TIME_NOABORT ||
	    sbp->SCB.sc_comp_code == SDI_ABORT ||
	    sbp->SCB.sc_comp_code == SDI_RESET ||
	    sbp->SCB.sc_comp_code == SDI_CRESET) {

		/* Resets used to cause the bad block gauntlet (sd01flts)
		 * to be started.  Timeouts used to cause retries until
		 * the retry count was exhausted, at which point the job
		 * would be failed.
		 *
		 * Instead, we will start the reset gauntlet, which will
		 * lead to the job being retried.  It does not matter whether
		 * a reset was caused by the gauntlet or by a third-party
		 * at this point.
		 *
		 * If reset support is disabled in the kernel or is not
		 * supported by the hba, we fall through to the old code.
		 * These two cases are identified by sdi_start_gauntlet
		 * returning 0.
		 */
		if (jp->j_flags & J_DONE_GAUNTLET) {
			/* Gauntlet has already run */
			SD01_DK_UNLOCK(dk);
			sd01comp1(jp);
			return;
		} else {
			SD01_DK_UNLOCK(dk);
			if (sdi_start_gauntlet(sbp, sd01_gauntlet)) 
				return;
			SD01_DK_LOCK(dk);
		}
	}

	/* When a job timed out retry it immediately without issuing error */
	/* messages. If retries fail then switch to normal bad block comp  */
	/* code of SDI_CKSTAT and status code of S_CKCON so mechanism to   */
	/* remap bad block occurs                                          */
	if (sbp->SCB.sc_comp_code == SDI_TIME) {
		if (jp->j_error < sd01_retrycnt) {
			SD01_DK_UNLOCK(dk);
			sd01retry(jp);
			return;
		}
		else {
			sbp->SCB.sc_comp_code = SDI_CKSTAT;
			sbp->SCB.sc_status = S_CKCON;
		}
	}


	if (sbp->SCB.sc_comp_code == SDI_CKSTAT && 
		sbp->SCB.sc_status == S_CKCON &&
		jp->j_error < sd01_retrycnt)
	{				/* We need to do a request sense */
 		/*
		*  The job pointer is set here and cleared in the
		*  gauntlet when the job is eventually retried
		*  or when the gauntlet exits due to an error.
 		*/
		SD01_DK_UNLOCK(dk);

 		sd01flts(dk, jp);
#ifdef DEBUG
        DPR (2)("sd01intn: - return\n");
#endif
 		return;
	}
	
 	if (sbp->SCB.sc_comp_code == SDI_CRESET ||
	    sbp->SCB.sc_comp_code == SDI_RESET)
 	{
 		/*
		*  The job pointer will be cleared when the job
		*  eventually is retried or when the gauntlet
		*  decides to exit due to an error.
		*/
		SD01_DK_UNLOCK(dk);

 		sd01flts(dk, jp);
 		return;
 	}
	
	/*
	*  To get here, the failure of the job was not due to a bus reset
	*  nor to a Check Condition state.
	*
	*  Now check for the following conditions:
	*     -  The RETRY bit was not set on SDI completion code.
	*     -  Exceeded the retry count for this job.
	*     -  Job status indicates RESERVATION Conflict.
	*     -	 Job status indicates SELECTION Timeout.
	*
	*  If one of the conditions is TRUE, then return the failed job
	*  to the user.
	*/
	if ((sbp->SCB.sc_comp_code & SDI_RETRY) == 0 ||
		jp->j_error >= sd01_retrycnt ||
		sbp->SCB.sc_comp_code == SDI_NOSELE)
	{
		SD01_DK_UNLOCK(dk);

		sd01comp1(jp);
		return;
	}

	
	if (sbp->SCB.sc_comp_code == SDI_CKSTAT)
	{				/* Retry the job later */
		sd01logerr(sbp, dk, 0x4dd13003);
		sdi_timeout(sd01retry, (caddr_t)jp, drv_usectohz(LATER), pldisk,
			&dk->dk_addr);
		SD01_DK_UNLOCK(dk);
		return;
	}

	/* Retry the job */
	sd01logerr(sbp, dk, 0x4dd13002);
	SD01_DK_UNLOCK(dk);
	sd01retry(jp);

#ifdef DEBUG
        DPR (2)("sd01intn: - exit\n");
#endif
}

/*
 * int
 * sd01phyrw(struct disk *dk, long dir, struct phyio *phyarg, int mode)
 *	This function performs a physical read or write without
 *	regrad to the VTOC.   It is called by the ioctl.  The
 *	argument for the ioctl is  a pointer to a structure
 *	which indicates the physical block address and the address
 *	of the data buffer in which the data is to be transferred. 
 *	The mode indicates whether the buffer is located in user or
 *	kernel space.
 * Called by: sd01wrtimestamp, sd01ioctl
 * Side Effects: 
 *	 The requested physical sector is read or written. If the PD sector
 *	 or VTOC have been updated they will be read in-core on the next access 
 *	 to the disk (DMUP_VTOC set in sd01vtoc_ck). However, if the FDISK 
 *	 sector is updated it will be read in-core on the next open.
 * Return Values:
 *	Status is returned in the retval byte of the structure
 *	for driver routines only.
 *
 * Calling/Exit State:
 *	No locks held across entry or exit.
 *	Acquires dk_lock and dm_lock.
 */
int
sd01phyrw(struct disk *dk, long dir, struct phyio *phyarg, int mode)
{
	struct disk_media *dm = dk->dk_dm;
	register struct job *jp;
	register buf_t *bp;
	char *tmp_buf;
	int size;			/* Size of the I/O request 	*/
	int ret_val = 0;		/* Return value			*/
	struct  xsb   *xsb;
	unsigned long dksects;
	unsigned long data_size = phyarg->datasz;
	paddr32_t *physaddr;

#ifdef DEBUG
        DPR (1)("sd01phyrw: (dk=0x%x dir=0x%x mode=0x%x)\n", dk, dir, mode);
        DPR (1)("phyarg: sectst=0x%x memaddr=0x%x datasz=0x%x\n", phyarg->sectst, phyarg->memaddr, phyarg->datasz);
#endif

	/* prevent reading or writing beyond the end of the disk */
	dksects = DKSIZE(dk);
	if (phyarg->sectst >= dksects)	{
		phyarg->retval = dir == V_RDABS ? V_BADREAD:V_BADWRITE;
		return ENXIO;
	}
	else if (phyarg->sectst +
			(unsigned long)(phyarg->datasz >> BLKSHF(dk))
								>= dksects)
		phyarg->datasz = (dksects - phyarg->sectst) << BLKSHF(dk);


	bp = getrbuf(KM_SLEEP);
	bp->b_bcount = BLKSZ(dk);

	if (mode != SD01_KERNEL) {
		/* Note: OK to sleep since its not a Kernel I/O request */
		tmp_buf = (char *) sdi_kmem_alloc_phys(BLKSZ(dk),
					dk->dk_bcbp->bcb_physreqp, 0); 
		physaddr = (paddr32_t *) (tmp_buf + BLKSZ(dk));
	} else {
		physaddr = (paddr32_t *) ((char *)phyarg->memaddr + data_size);
	}
	
	phyarg->retval = 0;
	while(phyarg->datasz > 0) {
		size = phyarg->datasz > BLKSZ(dk) ? BLKSZ(dk) : phyarg->datasz;
		if ( mode == SD01_KERNEL)
			bp->b_un.b_addr = (char *) phyarg->memaddr;
		else
			bp->b_un.b_addr = tmp_buf;
		bp->b_bcount = size;
		bioreset(bp);
		bp->b_blkno = phyarg->sectst << BLKSEC(dk);

		if (dir == V_RDABS) 
			bp->b_flags |= B_READ;
		else {
			if (mode != SD01_KERNEL) {	/* Copy user's data */
				if(copyin((void *)phyarg->memaddr,
				    (caddr_t)bp->b_un.b_addr, size)) {
					phyarg->retval = V_BADWRITE;
					ret_val=EFAULT;
					freerbuf(bp);
					break;
				}
			}
		}

		jp = sd01getjob(dk, KM_SLEEP, bp); /* Job is returned by sd01done */
		set_sjq_daddr(jp,phyarg->sectst);
		set_sjq_memaddr(jp,paddr(bp));

		xsb = (struct xsb *)jp->j_cont;
		if (jp->j_memaddr) {
                        xsb->extra.sb_datapt = physaddr;
                        xsb->extra.sb_data_type = SDI_PHYS_ADDR;
                }

		/*
		 * Note:
		 * If size > BLKSZ(dk) this shift will make it 0.
		 */
		jp->j_seccnt = size >> BLKSHF(dk);

		sd01strat1(jp, dk, bp);
		ret_val = biowait(bp);
		if(ret_val) {			
			if (ret_val == EFAULT) { /* retry after software remap */
				continue;
			} else {		/* Fail the request */
				phyarg->retval = dir == V_RDABS ? V_BADREAD:V_BADWRITE;
				break;
			}
		}
		
		if(dir == V_RDABS && mode != SD01_KERNEL)	
		{			/* Update the requestor's buffer */
			if (copyout((void *)bp->b_un.b_addr, (void *)phyarg->memaddr, size))
			{
				phyarg->retval = V_BADREAD;
				ret_val=EFAULT;
				break;
			}
		}
		
		phyarg->memaddr += size;
		phyarg->datasz -= size;
		phyarg->sectst++;
		if (mode != SD01_KERNEL) {
			*physaddr += size;
		}
	}
	
	if (mode != SD01_KERNEL)
		kmem_free(tmp_buf, 
		BLKSZ(dk)+SDI_PADDR_SIZE(dk->dk_bcbp->bcb_physreqp));

	freerbuf(bp);

#ifdef DEBUG
        DPR (2)("sd01phyrw: - exit(%d)\n", ret_val);
#endif
	return(ret_val);
}

/*
 *
 * int
 * sd01layer_cmd(struct disk *dk, void *command, uint command_size,
 * void *buffer, uint buffer_size, ushort mode)
 *	This funtion performs the SCSI command specified by scb on the
 *	addressed disk.  The data area specified by buffer is supplied by
 *	the caller and assumed to be in kernel space. This function will
 *	sleep awaiting command completion.
 * Called by: sd01devctl
 * Side Effects: 
 *	A SCSI command is added to the job queue and sent to the host adapter.
 * Return values:
 *	Zero is returned if the command succeeds. 
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	SD01_DK_LOCK acquired during execution, released by sd01sendq
 */
STATIC int
sd01layer_cmd(struct disk *dk, void *command, uint command_size,
time_t timeout, void *buffer, uint buffer_size, ushort mode)
{
	struct job *jp;
	struct scb *scb;
	buf_t *bp;
	int error;

#ifdef DEBUG
        DPR (1)("sd01layer_cmd: (dk=0x%x)\n", dk);
#endif
	
	bp = getrbuf(KM_SLEEP);
	
	jp = sd01getjob(dk, KM_SLEEP, bp);
	scb = &jp->j_cont->SCB;

	bp->b_flags |= mode & SCB_READ ? B_READ : B_WRITE;
	bp->b_un.b_addr = (caddr_t)buffer;
	bp->b_bcount = (long)buffer_size;

	jp->j_dk = dk;
	jp->j_bp = bp;
	jp->j_done = sd01done;
	jp->j_cont->sb_type = ISCB_TYPE;
	jp->j_flags |= J_NOERR;  /* don't report errors on the console */

	scb->sc_cmdpt = (caddr_t)command;
	scb->sc_cmdsz = (long)command_size;

	scb->sc_int = sd01intn;
	scb->sc_dev = dk->dk_addr;	/* read only */
	scb->sc_datapt = (caddr_t)buffer;

	if (buffer) {
		struct xsb *xsb = (struct xsb *)jp->j_cont;
		xsb->extra.sb_datapt = (paddr32_t *)
			((char *)buffer + buffer_size);
		xsb->extra.sb_data_type = SDI_PHYS_ADDR;
	}

	scb->sc_datasz = (long)buffer_size;
	scb->sc_mode = mode;
	scb->sc_resid = 0;
	scb->sc_time = timeout;
	scb->sc_wd = (long) jp;

	sdi_xtranslate(jp->j_cont->SCB.sc_dev.pdi_adr_version,
		jp->j_cont, bp->b_flags,NULL, KM_SLEEP);

	/* Send the job */
	if (sdi_xicmd(jp->j_cont->SCB.sc_dev.pdi_adr_version, 
			jp->j_cont, KM_SLEEP) != SDI_RET_OK) {
		error = EINVAL;
	}
	else {
		error = biowait(bp);
	}

	freerbuf(bp);

#ifdef DEBUG
	DPR (2)("sd01layer_cmd: - exit(%d)\n", error);
#endif
	return (error);
}

/*
 * int
 * sd01devctl8(dev_t dev, int cmd, void *arg)
 *	This function provides a number of different functions for use by
 *	other drivers in the I/O stack.  Currently, the only one supported
 *	is SDI_DEVICE_SEND.
 *	"SDI_DEVICE_SEND"
 *	The SCSI command given in arg is sent to the specified
 *	disk device.
 * Called by: Kernel
 * Side Effects: 
 *	The requested action is performed.
 *	This function may SLEEP.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock and dm_lock.
 */
STATIC int
sd01devctl8(void *idatap, ulong_t channel, int cmd, void *arg)
{
	struct disk *dk;
	sdi_devctl_t *ctl = (sdi_devctl_t *)arg;
	struct disk_media *dm;
	int ret_val = 0;

 	dk = (struct disk *)idatap;
	ASSERT(dk);
	dm = dk->dk_dm;
	ASSERT(dm);

#ifdef DEBUG
	DPR (1)("sd01devctl8: (dev=0x%x cmd=0x%x, arg=0x%x)\n", dk, cmd, arg);
#endif

	switch(cmd) {
		case SDI_DEVICE_SEND:
			ret_val = sd01layer_cmd(dk, ctl->sdc_command, ctl->sdc_csize,
				ctl->sdc_timeout, ctl->sdc_buffer,
				ctl->sdc_bsize, ctl->sdc_mode);
			break;
		case SDI_DEVICE_BADBLOCK: {
			/*
			 * Process the vtoc message including the pdsector and the vtoc
			 * so that badblock may work.
			 */
			sdi_badBlockMsg_t *msg;
			vtoc_t *vtoc;
			int npart;
			struct partition *part;
			struct alt_ent **alt;
			int *altcount;

			msg = (sdi_badBlockMsg_t *) arg;
			vtoc = (vtoc_t *) msg->vtoc;
			npart = vtoc->vtoc_nslices;

			part = (struct partition *)kmem_alloc(npart * sizeof(struct partition), KM_SLEEP);
			alt = (struct alt_ent **)kmem_zalloc(npart * sizeof(struct alt_ent *), KM_SLEEP);
			altcount = (int *)kmem_zalloc(npart * sizeof(int), KM_SLEEP);

			SD01_DM_LOCK(dm);
			/* copy pdinfo */
			bcopy (msg->pd, &dm->dm_pdinfo, sizeof(struct pdinfo));

			/* copy vtoc related stuff */
			dm->dm_nslices = vtoc->vtoc_nslices;
			dm->dm_v_version = vtoc->vtoc_version;
			dm->dm_partition = part;
			bcopy (vtoc->vtoc_part, dm->dm_partition, npart * sizeof(struct partition));
			dm->dm_altcount = altcount;
			dm->dm_firstalt = alt;
			SD01_DM_UNLOCK(dm);

			sd01getalts(dk);
		} break;
		case SDI_DEVICE_GEOMETRY: {
			sdi_diskparmMsg_t *msg = (sdi_diskparmMsg_t *) arg;

			msg->dp_secsiz = dm->dm_parms.dp_secsiz;
			if (strncmp(IDP(HBA_tbl[SDI_EXHAN(&dk->dk_addr)].idata)->name,"(ide",4) == 0)
				msg->dp_type = DPT_WINI;
			else
				msg->dp_type = DPT_SCSI_HD;
		} break;
		case SDI_GET_SIGNATURE:{
			sdi_signature_t *sigp = &dm->dm_signature;
			
			sigp->sig_size = sizeof(sdi_signature_t);
			bcopy(&(dm->dm_stamp), sigp->sig_string,
				sizeof(struct pd_stamp));
			/*
			 * There is no notion of active or inactive here thus
			 * default to ACTIVE.
			 */
			sigp->sig_state = SDI_PATH_ACTIVE;
			* (sdi_signature_t **) arg = sigp;
		} break;
		default:
			ret_val = EINVAL;
	}

#ifdef DEBUG
	DPR (2)("sd01devctl8: - exit(%d)\n", ret_val);
#endif
	return(ret_val);
}

/*
 *
 * int
 * sd01cmd(struct disk *dk, char op_code, uint addr, char *buffer, uint size,
 * uint length, ushort mode, int fault)
 *	This funtion performs a SCSI command such as Mode Sense on
 *	the addressed disk. The op code indicates the type of job
 *	but is not decoded by this function. The data area is
 *	supplied by the caller and assumed to be in kernel space. 
 *	This function will sleep.
 * Called by: sd01wrtimestamp, sd01flt, sd01ioctl
 * Side Effects: 
 *	A Mode Sense command is added to the job queue and sent to
 *	the host adapter.
 * Return values:
 *	Zero is returned if the command succeeds. 
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
sd01cmd(struct disk *dk, char op_code, uint addr, char *buffer, uint size, uint length, ushort mode, int fault)
{
	register struct job *jp;
	register struct scb *scb;
	register buf_t *bp;
	struct xsb *xsb;
	int error;

#ifdef DEBUG
        DPR (1)("sd01cmd: (dk=0x%x)\n", dk);
#endif
	
	bp = getrbuf(KM_SLEEP);
	
	for (;;) 
	{
	jp   = sd01getjob(dk, KM_SLEEP, bp);
	scb  = &jp->j_cont->SCB;
	
	bp->b_flags |= mode & SCB_READ ? B_READ : B_WRITE;
	bp->b_un.b_addr = (caddr_t) buffer;	/* not used in sd01intn */
	bp->b_bcount = size;
	bioreset(bp);
	
	jp->j_dk = dk;
	jp->j_bp = bp;
	jp->j_done = sd01done;
	if (!fault)
		jp->j_flags |= J_NOERR;
	
	if (op_code & 0x20) { /* Group 1 commands */
		register struct scm *cmd;

		cmd = &jp->j_cmd.cm;
		cmd->sm_op   = op_code;
		if (! dk->dk_addr.pdi_adr_version )
			/* read only */
			cmd->sm_lun = dk->dk_addr.sa_lun; 
		else
			cmd->sm_lun = SDI_LUN_32(&dk->dk_addr); 
		cmd->sm_res1 = 0;
		cmd->sm_addr = addr;
		cmd->sm_res2 = 0;
		cmd->sm_len  = length;
		cmd->sm_cont = 0;

		scb->sc_cmdpt = SCM_AD(cmd);
		scb->sc_cmdsz = SCM_SZ;
	}
	else {	/* Group 0 commands */
		register struct scs *cmd;

		cmd = &jp->j_cmd.cs;
		cmd->ss_op    = op_code;
		if (! dk->dk_addr.pdi_adr_version )
			/* read only */
			cmd->ss_lun = dk->dk_addr.sa_lun; 
		else
			cmd->ss_lun = SDI_LUN_32(&dk->dk_addr);
		cmd->ss_addr1 = ((addr & 0x1F0000) >> 16);
		cmd->ss_addr  = (addr & 0xFFFF);
		cmd->ss_len   = (ushort) length;
		cmd->ss_cont  = 0;

		scb->sc_cmdpt = SCS_AD(cmd);
		scb->sc_cmdsz = SCS_SZ;
	}
	
	xsb = (struct xsb *)jp->j_cont;
	scb->sc_int = sd01intn;
	/* sd01getjob did this? */  scb->sc_dev = dk->dk_addr;	/* read only */

	if (buffer) {
		xsb->extra.sb_data_type = SDI_PHYS_ADDR;
		if (jp->j_cmd.cs.ss_op != SS_REQSEN) {
			xsb->extra.sb_datapt = (paddr32_t *) ((char *)buffer + size);
		} else {
			xsb->extra.sb_datapt = (paddr32_t *) (char *)(buffer - 1 + size);
			*(xsb->extra.sb_datapt) += 1;
		}
	}

	scb->sc_datapt = buffer;
	scb->sc_datasz = size;
	scb->sc_mode = mode;
	scb->sc_resid = 0;
	scb->sc_time = sd01_get_timeout(op_code, dk);
	scb->sc_wd = (long) jp;


	sdi_xtranslate(jp->j_cont->SCB.sc_dev.pdi_adr_version,
			jp->j_cont,
			 bp->b_flags,NULL, KM_SLEEP);

	SD01_DK_LOCK(dk);

	sd01queue_low(jp, dk);
	sd01sendq(dk, KM_SLEEP);
	error = biowait(bp);

	/* exit if success or if error not due to software bad sector remap */
	if (error != EFAULT)
		break;
	}

#ifdef DEBUG
	if (error)
        	DPR (2)("sd01cmd: - exit(%d)\n", error);
	else
        	DPR (2)("sd01cmd: - exit(0)\n");
#endif
	freerbuf(bp);
	return (error);
}

/*
 * int
 * sd01ioctl8(dev_t dev, int cmd, int arg, int mode, cred_t *cred_p, int *rval_p)
 *	This function provides a number of different functions for use by
 *	utilities. They are: physical read or write, and read or write
 *	physical descriptor sector.  
 *	"READ ABSOLUTE"
 *	The Absolute Read command is used to read a physical sector on the
 *	disk regardless of the VTOC.  The data is transferred into buffer
 *	specified by the argument structure.
 *	"WRITE ABSOLUTE"
 *	The Absolute Write command is used to write a physical sector on the
 *	disk regardless of the VTOC.  The data is transferred from a buffer
 *	specified by the argument structure.
 *	"PHYSICAL READ"
 *	The Physical Read command is used to read any size data block on the
 *	disk regardless of the VTOC or sector size.  The data is transferred 
 *	into buffer specified by the argument structure.
 *	"PHYSICAL WRITE"
 *	The Physical Write command is used to write any size data block on the
 *	disk regardless of the VTOC or sector size.  The data is transferred 
 *	from a buffer specified by the argument structure.
 *	"READ PD SECTOR"
 *	This function reads the physical descriptor sector on the disk.
 *	"WRITE PD SECTOR"
 *	This function writes the physical descriptor sector on the disk.
 *	"CONFIGURATION"
 *	The Configuration command is used by mkpart to reconfigure a drive.
 *	The driver will update the in-core disk configuration parameters.
 *	"GET PARAMETERS"
 *	The Get Parameters command is used by mkpart and fdisk to get 
 *	information about a drive.  The disk parameters are transferred
 *	into the disk_parms structure specified by the argument.
 *	"RE-MOUNT"
 *	The Remount command is used by mkpart to inform the driver that the 
 *	contents of the VTOC has been updated.  The driver will update the
 *	in-core copy of the VTOC on the next open of the device.
 *	"PD SECTOR NUMBER"
 *	The PD sector number command is used by 386 utilities that need to
 *	access the PD and VTOC information. The PD and VTOC information
 *	will always be located in the 29th block of the UNIX partition.
 *	"PD SECTOR LOCATION"
 *	The PD sector location command is used by SCSI utilities that need to
 *	access the PD and VTOC information. The absolute address of this
 *	sector is transferred into an integer specified by the argument.
 *	"ELEVATOR"
 *	This Elevator command allows the user to enable or disable the
 *	use of the elevator algorithm.
 *	"RESERVE"
 *	This Reserve command will reserve the addressed device so that no other
 *	initiator can use it.
 *	"RELEASE"
 *	This Release command releases a device so that other initiators can
 *	use the device.
 *	"RESERVATION STATUS"
 *	This Reservation Status command informs the host if a device is 
 *	currently reserved, reserved by another host or not reserved.
 * Called by: Kernel
 * Side Effects: 
 *	The requested action is performed.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock and dm_lock.
 */
/*ARGSUSED*/
int
sd01ioctl8(void *idatap, ulong_t channel, int cmd, void *arg, int mode, cred_t *cred_p, int *rval_p)
{
	struct disk *dk;
	struct disk_media *dm;
	int state;			/* Device's RESERVE Status 	*/
	struct phyio phyarg;		/* Physical block(s) I/O	*/
	struct absio absarg;		/* Absolute sector I/O only	*/
	dev_t pt_dev;			/* Pass through device number 	*/
	int ret_val = 0;
	struct pd_stamp new_stamp;

 	dk = (struct disk *)idatap;
	ASSERT(dk);
	dm = dk->dk_dm;
	ASSERT(dm);

#ifdef DEBUG
        DPR (1)("sd01ioctl8: (dev=0x%x cmd=0x%x, arg=0x%x)\n", dk, cmd, arg);
#endif

        if (cmd == SD01_DEBUGFLG) {
#ifdef SD01_DEBUG
                register short  dbg;

		if (copyin ((char *)arg, Sd01_debug, 10) != 0)
			return(EFAULT);

                cmn_err (CE_CONT, "\nNew debug values :");

                for (dbg = 0; dbg < 10; dbg++) {
                        cmn_err (CE_CONT, " %d", Sd01_debug[dbg]);
                }
                cmn_err (CE_CONT, "\n");
#endif
                return(0);
        }

	switch(cmd) {
		case V_WRABS: 	/* Write absolute sector number */
		case V_PWRITE:	/* Write physical data block(s)	*/

                	/* Make sure user has permission */
			if (ret_val = drv_priv(cred_p))
				break;
			/*FALLTHRU*/
		case V_RDABS: 	/* Read absolute sector number 	*/
		case V_PREAD:	/* Read physical data block(s)	*/

			if (cmd == V_WRABS || cmd == V_RDABS) {
				if (copyin((void *)arg, (void *)&absarg, sizeof(absarg))) {
					ret_val = EFAULT;
					break;
				}

				phyarg.sectst  = (unsigned long) absarg.abs_sec;
				phyarg.memaddr = (unsigned long) absarg.abs_buf;
				phyarg.datasz  = BLKSZ(dk);

				ret_val = sd01phyrw(dk, (long) cmd, &phyarg, SD01_USER);
			}
			else {
				if (copyin((void *)arg, (void *)&phyarg, sizeof(phyarg))) {
					ret_val = EFAULT;
					break;
				}

				cmd = cmd == V_PREAD ? V_RDABS : V_WRABS;

				if (phyarg.datasz & BLKMSK(dk)) {
					/*
					 * Non-multiple of sector size 
					 */
					ret_val = EINVAL;
					break;
				}
				ret_val = sd01phyrw(dk, (long) cmd, &phyarg, SD01_USER);
				/* Copy out return value to user */
				if (copyout((void *)&phyarg, (void *)arg, sizeof(phyarg))) {
					ret_val = EFAULT;
				}
			}

			break;

		/* Change drive configuration parameters. */
		case V_CONFIG: 
			break;

		case SD_ELEV:
			SD01_DK_LOCK(dk);

			if ((long) arg)
				dk->dk_state |= DKEL_OFF;
			else
				dk->dk_state &= ~DKEL_OFF;

			SD01_DK_UNLOCK(dk);
			break;

		case SDI_RESERVE:
			ret_val = sd01cmd(dk, SS_RESERV, 0, (char *) 0, 0, 0, SCB_WRITE, TRUE);
			if (ret_val == 0)
			{
				SD01_DM_LOCK(dm);
				dm->dm_res_dp = dk;
				dm->dm_state |= (DMRESERVE|DMRESDEVICE);
				SD01_DM_UNLOCK(dm);
			}
			break;

		case SDI_RELEASE:
			ret_val = sd01cmd(dk, SS_TEST, 0, (char *) 0, 0, 0, SCB_READ, TRUE);
			if (ret_val == 0) {
				ret_val = sd01cmd(dk, SS_RELES, 0, (char *) 0, 0, 0, SCB_WRITE, TRUE); 
				if (ret_val == 0)
				{
					SD01_DM_LOCK(dm);
					dm->dm_res_dp = NULL;
					dm->dm_state &= ~(DMRESERVE|DMRESDEVICE);
					SD01_DM_UNLOCK(dm);
				}
			}
			break;

		case SDI_RESTAT:
			SD01_DM_LOCK(dm);

			if (dm->dm_state & DMRESERVE && dm->dm_res_dp == dk) {
				SD01_DM_UNLOCK(dm);
				state = 1;	/* Currently Reserved */
			} else {
				SD01_DM_UNLOCK(dm);
				if(sd01cmd(dk, SS_TEST, 0, (char *) 0, 0, 0, SCB_READ, TRUE ) == EBUSY)
					/* Reserved by another initiator */
					state = 2;
				else
					/* Not Reserved */
					state = 0;
			}

			if (copyout((void *)&state, (void *)arg, sizeof(state)))
				ret_val = EFAULT;
			break;

		case B_GETTYPE:
			if (copyout((void *)SCSI_NAME, 
				((struct bus_type *) arg)->bus_name, 
				strlen(SCSI_NAME)+1))
			{
				ret_val = EFAULT;
			}
			else if (copyout((void *)Sd01_modname, 
				((struct bus_type *) arg)->drv_name, 
				strlen(Sd01_modname)+1))
			{
				ret_val = EFAULT;
			}
			break;

		case B_GETDEV:
			sdi_getdev(&dk->dk_addr, &pt_dev);
			if (copyout((void *)&pt_dev, (void *)arg, sizeof(pt_dev)))
				ret_val = EFAULT;
			break;	
		case SD_GETSTAMP:
			if (copyout((void *)&dm->dm_stamp, (void *)arg, sizeof(struct pd_stamp)))
				ret_val = EFAULT;
			break;	
		case SD_NEWSTAMP:
			sd01_newstamp(&new_stamp);
			if (copyout((void *)&new_stamp, (void *)arg, sizeof(struct pd_stamp)))
				ret_val = EFAULT;
			break;	
		default:
			ret_val = EINVAL;
			break;
	}

#ifdef DEBUG
        DPR (2)("sd01ioctl8: - exit(%d)\n", ret_val);
#endif
	return(ret_val);
}

/*
 * void
 * sd01resume(struct disk *dk)
 *	This function is called only if a queue has been suspended and must
 *	now be resumed. It is called by sd01comp1 when a job has been
 *	failed and a disk queue must be resumed or by sdflterr when there
 *	is no job to fail but the queue needs to be resumed anyway.
 * Called by: sd01comp1, sd01flterr,
 * Side Effects: THe LU queue will have been resumed.
 * Errors:
 *	The HAD rejected a Resume function request by the SCSI disk driver.
 *	This is caused by a parameter mismatch within the driver.
 *	The system should be rebooted.
 *
 * Calling/Exit State:
 *	Caller holds disk struct's dk_lock.
 */
void
sd01resume(struct disk *dk)
{
#ifdef DEBUG
        DPR (1)("sd01resume: (dk=0x%x)\n", dk);
#endif

	dk->dk_spcount = 0;	/* Reset special count */
	sd01_fltsbp->sb_type = SFB_TYPE;
	sd01_fltsbp->SFB.sf_int = sd01intf;
	sd01_fltsbp->SFB.sf_priv = (char *)dk; /* for use by sd01intf */
	sd01_fltsbp->SFB.sf_dev = dk->dk_addr;
	sd01_fltsbp->SFB.sf_func = SFB_RESUME;

	SD01_DK_UNLOCK(dk);
	sdi_xicmd(sd01_fltsbp->SFB.sf_dev.pdi_adr_version, 
				sd01_fltsbp, KM_NOSLEEP);
	SD01_DK_LOCK(dk);
	dk->dk_state &= ~DKSUSP;

#ifdef DEBUG
        DPR (2)("sd01resume: - exit\n");
#endif
}

/*
 * void
 * sd01qresume(struct disk *dk)
 *	Checks if the SB used for resuming a LU queue is currently busy.
 *	If it is busy, the current disk is added to the end of the list 
 *	of disks waiting for a resume to be issued.
 *	If the SB is not busy, this disk is put at the front of the
 *	list and the resume for this disk is started immediately.
 *
 *	Called by sd01comp1, sd01flterr
 *	Side effects: A disk structure is added to the Resume queue.
 *
 *	Calling/Exit State:
 *		Disk struct's dk_lock held on entry and return, but
 *		dropped in the middle.
 *		Acquires sd01_resume_mutex while modifying sd01_resume.
 */
int
sd01qresume(struct disk *dk)
{
#ifdef DEBUG
        DPR (1)("sd01qresume: (dk=0x%x)\n", dk);
#endif

	sd01_resume_pl = LOCK(sd01_resume_mutex, pldisk);
	/* Check if the Resume SB is currently being used */
	if (sd01_resume.res_head == (struct disk *) &sd01_resume)
	{	/* Resume Q not busy */

		dk->dk_state |= DKONRESQ;
		sd01_resume.res_head = dk;
		sd01_resume.res_tail = dk;
		dk->dk_fltnext = (struct disk *) &sd01_resume;
		UNLOCK(sd01_resume_mutex, sd01_resume_pl);
		/* ok to hold dk_lock across this */
		sd01resume(dk);
	}
	else
	{	/* Resume Q is Busy */

		/*
		*  This disk may already be on the Resume Queue.
		*  If it is, then set the flag to indicate that
		*  another Resume is pending for this disk.
		*/
		if (dk->dk_state & DKONRESQ)
		{
			dk->dk_state |= DKPENDRES;
		}
		else
		{	/* Not on Q, put it there */
			dk->dk_state |= DKONRESQ;
			sd01_resume.res_tail->dk_fltnext = dk;
			sd01_resume.res_tail = dk;
			dk->dk_fltnext = (struct disk *) &sd01_resume;
		}
		UNLOCK(sd01_resume_mutex, sd01_resume_pl);
	}

#ifdef DEBUG
        DPR (2)("sd01qresume: - exit\n");
#endif

	return 0;
}

/*
 * int
 * sd01config(struct disk *dk, minor_t minor)
 *	This function initializes the driver's disk parameter structure.
 *	If the READ CAPACITY or MODE SENSE commands fail, a non-fatal
 *	error status is returned so that sd01loadvtoc() routine does not
 *	fail.  In this case, the V_CONFIG ioctl can still be used to set
 *	the drive parameters.
 * Called by: sd01loadvtoc
 * Side Effects: The disk state flag will indicate if the drive parameters 
 *	 	  are valid.
 * Return Values:
 *	-1: Non-fatal error detected
 *	 0: Successfull completion
 *	>0: Fatal error detected - Error code is returned
 * Errors:
 *	The sectors per cylinder parameter calculates to less than or equal
 *	to zero.  This is caused by incorrect data returned by the MODE
 *	SENSE command or the drivers master file. This may not be an AT&T 
 *	supported device.
 *
 *	The number of cylinder calculates to less than or equal to zero.  
 *	This is caused by incorrect data returned by the READ CAPACITY
 *	command. This may not be an AT&T supported device.
 *
 *	The READ CAPACITY command failed.
 *
 *	The Logical block size returned by the READ CAPACITY command is
 *	invalid i.e. it is not a power-of-two multiple of the kernel
 *	block size (KBLKSZ).
 *
 *	Unable able to allocate memory for the Margianl Block data area
 *	for this device.
 *
 *	The MODE SENSE command for Page 3 failed.
 *
 *	The MODE SENSE command for Page 4 failed.
 *
 * Calling/Exit State:
 *	Access is serialized by dm_sv associated with the disk.
 */
int
sd01config8(struct disk *dk)
{
	struct disk_media *dm = dk->dk_dm;
	DADF_T *dadf = (DADF_T *) NULL;
	RDDG_T *rddg = (RDDG_T *) NULL;
	CAPACITY_T *cap = (CAPACITY_T *) NULL;
	uint pg_asec_z;
	long cyls = 0;
	long sec_cyl;
	int i;
	int gotparams = 0, disk_is_SCSI;
	int cntlr;
	
#ifdef DEBUG
	DPR (1)("sd01config8: (dk=0x%x)\n", dk);
#endif

	/* Send READ CAPACITY to obtain last sector address */
	if (sd01cmd(dk,SM_RDCAP,0,dk->dk_rc_data,RDCAPSZ,0,SCB_READ, TRUE)) {
		return(EIO);
	}

	cap = (CAPACITY_T *)(void *) (dk->dk_rc_data);

	cap->cd_addr = sdi_swap32(cap->cd_addr);
	cap->cd_len  = sdi_swap32(cap->cd_len);

	if (cap->cd_len <= 0) {
		cmn_err (CE_WARN, "Disk Driver: Sect size (%x) is invalid", cap->cd_len);
		return(-1);
	}

	/*
	 * Init the Block<->Logical Sector convertion values.
	 */
	for (i=0; i < (32-KBLKSHF); i++) {
		if ((KBLKSZ << i) == cap->cd_len) {
			break;
		}
	}
	if ((KBLKSZ << i) != cap->cd_len) {
		/*
		 *+ The Logical block size returned by the READ CAPACITY
		 *+ command is invalid i.e. it is not a power-of-two 
		 *+ multiple of the kernel block size (KBLKSZ).
		 *+ Probably caused by a bad value in the sd01diskinfo
		 *+ structure in the driver's Space.c
		 */
		cmn_err (CE_WARN, "Disk Driver: Sect size (%x) Not power-of-two of %x", cap->cd_len, KBLKSZ);
		return(-1);
	}

	/*
	 * In time, this whole geometry stuff should be removed from this
	 * driver. It is really used in vtoc. The problem today is that
	 * the size of the disk is computed from the setting calculated
	 * in the code. It should instead use cd_addr which is the precise
	 * size.
	 */

	dm->dm_parms.dp_secsiz = cap->cd_len;
	dm->dm_sect.sect_blk_shft = i;
	dm->dm_sect.sect_byt_shft = KBLKSHF + i;
	dm->dm_sect.sect_blk_mask = ((1 << i) - 1);

	if (! dk->dk_addr.pdi_adr_version )
		cntlr = SDI_EXHAN(&dk->dk_addr);
	else
		cntlr = SDI_HAN_32(&dk->dk_addr);
	disk_is_SCSI = strncmp(IDP(HBA_tbl[cntlr].idata)->name, "(ide,", 5);

	/*
	 * If this is a SCSI based drive, then use the virtual
	 * geometry if specified.  Otherwise, use the physical
	 * geometry as returned by the MODE SENSE Page 3 and 4.
	 */
	if (disk_is_SCSI && (Sd01diskinfo[i] != 0)) {
#ifdef DEBUG
		DPR (3)("Using a Virtual geometry\n");
#endif
		/*
	 	* Use the virtual geometry specified in the space.c file
	 	*/
		dm->dm_parms.dp_sectors  = (Sd01diskinfo[i] >> 8) & 0x00FF;
		dm->dm_parms.dp_heads    = Sd01diskinfo[i] & 0x00FF;

		sec_cyl = dm->dm_parms.dp_sectors * dm->dm_parms.dp_heads;
    	} else {
#ifdef DEBUG
		DPR (3)("Using the disks Physical geometry\n");
#endif
		/*
		 * Get the sectors/track value from MODE SENSE Page-3.
		 */
		if (sd01cmd(dk,SS_MSENSE,0x0300,dk->dk_ms_data,FPGSZ,FPGSZ,SCB_READ, TRUE)) {
			return(-1);
		}

		dadf = (DADF_T *)(void *) (dk->dk_ms_data + SENSE_PLH_SZ + 
	       		((SENSE_PLH_T *)(void *) dk->dk_ms_data)->plh_bdl);

		/* Swap Page-3 data */
		dadf->pg_sec_t   = sdi_swap16(dadf->pg_sec_t);
		dadf->pg_asec_z  = sdi_swap16(dadf->pg_asec_z);
		dadf->pg_bytes_s = sdi_swap16(dadf->pg_bytes_s);

		dm->dm_parms.dp_sectors =
			(dadf->pg_bytes_s * dadf->pg_sec_t) / cap->cd_len;  

		pg_asec_z = dadf->pg_asec_z;

		/*
		 * Get # of heads from MODE SENSE Page-4.
		 */
		if (sd01cmd(dk,SS_MSENSE,0x0400,dk->dk_ms_data,RPGSZ,RPGSZ,SCB_READ, TRUE)) {
			return(-1);
		}

		rddg = (RDDG_T *) (void *)(dk->dk_ms_data + SENSE_PLH_SZ + 
	       		((SENSE_PLH_T *)(void *) dk->dk_ms_data)->plh_bdl);

		sec_cyl = rddg->pg_head * (dm->dm_parms.dp_sectors - pg_asec_z);

		dm->dm_parms.dp_heads = rddg->pg_head;
	}

	/* Check sec_cly calculation */
	if (sec_cyl <= 0)
	{
		/*
		 *+ The sectors per cylinder parameter calculates to less
		 *+ than or equal to zero.  This is caused by incorrect data
		 *+ returned by the MODE SENSE command.  Possibly the CMOS
		 *+ parameters have an invalid value, or the SCSI drive
		 *+ has errors, or this is not a supported device.
		 */
       		cmn_err(CE_WARN, "!Disk Driver: Sectors per cylinder error cyls=0x%x\n",cyls);
		return(-1);
	}

	cyls = (cap->cd_addr + 1) / sec_cyl;

	/* Check cyls calculation */
	if (cyls <= 0)
	{
		/*
		 *+ The number of cylinder parameter calculates to less
		 *+ than or equal to zero.  This is caused by incorrect data
		 *+ returned by the MODE SENSE command.  Possibly the CMOS
		 *+ parameters have an invalid value, or the SCSI drive
		 *+ has errors, or this is not a supported device.
		 */
       		cmn_err(CE_NOTE, "!Disk Driver: Number of cylinders error.");
		return(-1);
	}

	/* Make room for diagnostic scratch area */
	if ((cap->cd_addr + 1) == (cyls * sec_cyl))
		cyls--;

	/* Assign cylinder parameter for V_GETPARMS */
	dm->dm_parms.dp_cyls = cyls;

	/*
	 * Set the breakup granularity to the device sector size.
	 */
	dk->dk_bcbp->bcb_granularity = BLKSZ(dk);

	if (!(dk->dk_bcbp->bcb_flags & BCB_PHYSCONTIG) &&
	    !(dk->dk_bcbp->bcb_addrtypes & BA_SCGTH) &&
	    !(dm->dm_iotype & F_PIO)) {
		/*
		 * Set up a bcb for maxxfer and maxxfer - ptob(1), and
		 * set phys_align for drivers that are not using buf_breakup
		 * to break at every page, and not using the BA_SCGTH
		 * feature of buf_breakup, and not programmed I/O.
		 * (e.g. HBAs that are still doing there own scatter-
		 * gather lists.)
		 */
		dk->dk_bcbp_max->bcb_granularity = BLKSZ(dk);
		dk->dk_bcbp_max->bcb_physreqp->phys_align = BLKSZ(dk);
		dk->dk_bcbp_max->bcb_max_xfer = 
		   HIP(HBA_tbl[cntlr].info)->max_xfer;

		dk->dk_bcbp->bcb_physreqp->phys_align = BLKSZ(dk);
		dk->dk_bcbp->bcb_max_xfer = dk->dk_bcbp_max->bcb_max_xfer
		   - ptob(1);

		(void)physreq_prep(dk->dk_bcbp->bcb_physreqp, KM_SLEEP);
		(void)physreq_prep(dk->dk_bcbp_max->bcb_physreqp, KM_SLEEP);
	}

	/* Indicate parameters are set and valid */
	dm->dm_state |= DMPARMS;

#ifdef SD01_DEBUG
	if(sd01alts_debug & DKADDR) {
        	cmn_err(CE_CONT,"sd01config8: sec/trk=0x%x secsiz=0x%x heads=0x%x cyls=0x%x ", 
		dm->dm_parms.dp_sectors, dm->dm_parms.dp_secsiz, 
		dm->dm_parms.dp_heads, dm->dm_parms.dp_cyls);
	}
#endif

#ifdef DEBUG
        DPR (3)("sec=0x%x siz=0x%x heads=0x%x cyls=0x%x\n", dm->dm_parms.dp_sectors, dm->dm_parms.dp_secsiz, dm->dm_parms.dp_heads, dm->dm_parms.dp_cyls);
#endif
	
#ifdef DEBUG
        DPR (2)("sd01config8: parms set - exit(0)\n");
#endif
	return(0);
}

/*
 * void
 * sd01flush_job(struct disk *dk)
 *	Flush all jobs in the disk queue
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 */
void
sd01flush_job(struct disk *dk)
{
	struct	job *jp;
	struct	job *jp_next;


	SD01_DK_LOCK(dk);

	jp = dk->dk_queue;
	dk->dk_queue = dk->dk_lastlow = dk->dk_lasthi = (struct job *)NULL;

	SD01_DK_UNLOCK(dk);

	while (jp) {
		jp_next = jp->j_next;
		jp->j_next = (struct job *)NULL;
		jp->j_cont->SCB.sc_comp_code = SDI_QFLUSH;
		sd01done(jp);
		jp = jp_next;
	}
}


/*
 * int
 * sd01gen_sjq(struct disk *dk, buf_t *bp, int  flag,
 *	struct job *prevjp)
 *		Generate the scsi job queue
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 *
 *	Returns 0 on successful job generation.
 *	        1 on failure.  The only failure is a result of no
 *		  memory for allocating job structures, so if the call
 *		  is not SD01_IMMED, then the routine will not fail.
 */
int
sd01gen_sjq(struct disk *dk, buf_t *bp, int  flag, struct job *prevjp)
{
	struct disk_media *dm = dk->dk_dm;
	register struct	job *diskhead = NULL;
	register struct	job *jp, *nextjp;
	int	ac, sleepflag;

	sleepflag = (flag & SD01_IMMED) ? KM_NOSLEEP:KM_SLEEP;

	/* generate a scsi disk job */
	if ((diskhead = (struct job *)sd01getjob(dk, sleepflag, bp)) == NULL)
		return (1);
	diskhead->j_seccnt = bp->b_bcount >> BLKSHF(dk);
	set_sjq_memaddr(diskhead,paddr(bp));
	set_sjq_daddr(diskhead,sdi_get_blkno(bp));

#ifdef SD01_DEBUG
	cmn_err(CE_NOTE,"j_seccnt = 0x%x j_daddr = 0x%x BLKSEC(dk) = 0x%x\n",
		diskhead->j_seccnt,diskhead->j_daddr,dm->dm_sect.sect_blk_shft);
#endif

#ifdef SD01_DEBUG
	if(sd01alts_debug & DXALT) {
		cmn_err(CE_CONT,
		"sd01gen_sjq: diskhead: part= %d seccnt= 0x%x memaddr= 0x%x daddr= 0x%x\n",
		part,diskhead->j_seccnt,sjq_memaddr(diskhead), 
		sjq_daddr(diskhead));
		cmn_err(CE_CONT,
		"buf: bcount= 0x%x paddr(bp)= 0x%x blkno= 0x%x\n",
		bp->b_bcount, paddr(bp), bp->b_blkno);
	}
#endif

	/* If we are are not doing lightweight checking or
	 * we have remapped a block, check for a bad sector.
	 *
	 * Lightweight case: No synchronization on dm_altcount[part].  
	 * We must be prepared to handle the case that we 
	 * discover a new bad block and then immediately 
	 * send another job to that block.
	 */
	if (dm->dm_nslices && (!sd01_lightweight || dm->dm_altcount[0])) {
 
		/* Check for bad sector in this disk request */

		SD01_DM_LOCK(dm);
		ac = dm->dm_altcount[0];
		SD01_DM_UNLOCK(dm);

		if (ac)
			if (sd01alt_badsec(dk, diskhead, 0, sleepflag)) {
				for (jp=diskhead; jp; jp=jp->j_fwmate) {
					sd01freejob(jp);
				}
				return (1);
			}
	}

	/* SD01_IMMED and jp indicate we're resending a write with a	*/
	/* newly remapped/reassigned bad sector. We attach the old	*/
	/* existing job structs so a biodone is done only once		*/
	/* only once when all the old and new jobs are completed	*/

	if ((flag & SD01_IMMED) && (prevjp)) {
		prevjp->j_fwmate = diskhead;
		diskhead->j_bkmate = prevjp;
	}

	for (jp=diskhead; jp; jp=nextjp) {
		nextjp = jp->j_fwmate;
		if (flag & SD01_IMMED)
			jp->j_cont->sb_type = ISCB_TYPE;	
		if (flag & SD01_FIFO)
			jp->j_flags |= J_FIFO;
		sd01strat1(jp,dk,bp);
	}
	return (0);
}

/*
 * int
 * sd01devinfo8(dev_t dev, di_parm_t parm, bcb_t *bcbp)
 *	Get device information.
 *	
 * Calling/Exit State:
 *	The device must already be open.
 */
int
sd01devinfo8(void *idatap, ulong_t channel, di_parm_t parm, void *valp)
{
	struct disk *dk;
	struct disk_media *dm;

	dk = (struct disk *)idatap;
	ASSERT(dk);

	switch (parm) {
		case DI_RBCBP:
		case DI_WBCBP:
			SD01_DK_LOCK(dk);
			*(bcb_t **)valp = dk->dk_bcbp;
			SD01_DK_UNLOCK(dk);
			return 0;
		case DI_SIZE:
			dm = dk->dk_dm;
			ASSERT(dm);
			SD01_DM_LOCK(dm);
			((devsize_t *)valp)->blkno = DMSIZE(dm);
			SD01_DM_UNLOCK(dm);
			((devsize_t *)valp)->blkoff = (ushort_t)0;
			return 0;
		case DI_MEDIA:
			*(char **)valp = "disk_path";
			return 0;
		case DI_PHYS_HINT:
			return EOPNOTSUPP;
		default:
			return ENOSYS;
	}
}

#ifdef DEBUG
/*
 * void
 * sd01prt_jq(struct disk *dk)
 *
 * Calling/Exit State:
 *	None
 */
void
sd01prt_jq(struct disk *dk)
{
	struct	job *jp;
	struct 	scm *cmd;
	int	i;

	if (dk == (struct disk *)NULL)
		dk = sd01_dp[0];

	cmn_err(CE_CONT,"sd01print_job dk=0x%x\n", dk);
	cmn_err(CE_CONT,"HIGH PRIORITY QUEUE\n");
	jp = dk->dk_queue;
	for (i=0; jp!= dk->dk_lasthi; jp=jp->j_next, i++) {

		cmd = &jp->j_cmd.cm;

		if (jp->j_cont->sb_type == SCB_TYPE) {
			cmn_err(CE_CONT,"JBQ[%d]0x%x: op=%s sec=0x%x cnt=%d mem=0x%x\n",
				i,jp,(cmd->sm_op==SM_READ)?"Rd":"Wr",
				cmd->sm_addr, cmd->sm_len,
				jp->j_cont->SCB.sc_datapt);
		}
	}
	cmn_err(CE_CONT,"LOW PRIORITY QUEUE\n");
	for (i=0; jp; jp=jp->j_next, i++) {

		cmd = &jp->j_cmd.cm;

		if (jp->j_cont->sb_type == SCB_TYPE) {	
			cmn_err(CE_CONT,"JBQ[%d]0x%x: op=%s sec=0x%x cnt=%d mem=0x%x\n",
				i,jp,(cmd->sm_op==SM_READ)?"Rd":"Wr", 
				cmd->sm_addr, cmd->sm_len, 
				jp->j_cont->SCB.sc_datapt);
		}
	}
}

/*
 * void
 * sd01prt_job(struct job *jp)
 *
 * Calling/Exit State:
 *	None
 */
void
sd01prt_job(struct job *jp)
{
	cmn_err(CE_CONT,"JOB jp= 0x%x j_cont= 0x%x j_done= 0x%x bp=0x%x dk= 0x%x\n",
		jp, jp->j_cont, jp->j_done, jp->j_bp, jp->j_dk);
	cmn_err(CE_CONT,"fwmate= 0x%x bkmate= 0x%x daddr=0x%x memaddr= 0x%x seccnt= 0x%x\n",
		jp->j_fwmate, jp->j_bkmate, jp->j_daddr,
		jp->j_memaddr, jp->j_seccnt);
}

/*
 * void
 * sd01prt_dsk(int idx)
 *
 * Calling/Exit State:
 *	None
 */
void
sd01prt_dsk(int idx)
{
	struct	disk	*dp;

	dp = sd01_dp[idx];

	if (! dp->dk_addr.pdi_adr_version )
		cmn_err(CE_CONT,
		"DISK dp= 0x%x dk_state= 0x%x dk_addr.sa_ct= 0x%x"
		" dk_addr.sa_lun= 0x%x\n",
		dp, dp->dk_state,
		dp->dk_addr.sa_ct, dp->dk_addr.sa_lun);
	else
		cmn_err(CE_CONT,
		"DISK dp= 0x%x dk_state= 0x%x Controller= 0x%x"
		" Logical unit= 0x%x\n",
		dp->dk_addr.extended_address->scsi_adr.scsi_ctl,
		dp->dk_addr.extended_address->scsi_adr.scsi_lun);
}
#endif /* DEBUG */


/*
 * int
 * sd01_first_open()
 * 	This routine is called the first time a disk is opened in any
 *	of its slices or partitions.
 * Called by: sd01_firstOpen8
 * Side Effects:
 *	Set the disk_media struct's dm_fo_dp to the disk path used for the
 *	first open.
 * Return values: SDI_RET_OK, SDI_RET_ERR
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock and disk_media struct's dm_lock.
 */
int
sd01_first_open(struct disk *dk)
{
	struct disk_media *dm = dk->dk_dm;
	int ret=SDI_RET_OK;
	struct scsi_adr	scsi_adr;

	if (((dm->dm_ident.id_ver & 0x07) > (unsigned)1) && 
				sd01cmd(dk, SS_ST_SP, 0, NULL, 0, 1, SCB_READ, FALSE)) {
		struct sense *sd01_sense;

		/*
		 * A REQUEST SENSE to clear any possible Unit Attentions or such
		 */
		sd01_sense = (struct sense *)sdi_kmem_alloc_phys(sizeof(struct sense),
				dk->dk_bcbp->bcb_physreqp, 0);
		if (sd01_sense == NULL) {
			cmn_err(CE_WARN, "disk driver: Unable to "
				"allocate memory to open disk.\n");
			return SDI_RET_ERR;
		}

		(void)sd01cmd(dk, SS_REQSEN, 0, SENSE_AD(sd01_sense), 
			sizeof(struct sense), SENSE_SZ, SCB_READ, TRUE);

		kmem_free(sd01_sense,
				sizeof(struct sense)+SDI_PADDR_SIZE(dk->dk_bcbp->bcb_physreqp));

		if (sd01cmd(dk, SS_ST_SP, 0, NULL, 0, 1, SCB_READ, FALSE))
		{
			/*EMPTY*/
#ifdef SD01_DEBUG
			if (! dk->dk_addr.pdi_adr_version ) 
				cmn_err(CE_WARN,
					 "!Disk Driver: Couldn't spin up "
					"disk for open %d: %d,%d,%d\n",
				SDI_EXHAN(dk->dk_addr), SDI_BUS(dk->dk_addr),
				SDI_EXTCN(dk->dk_addr), SDI_EXLUN(dk->dk_addr));
			else
				cmn_err(CE_WARN,
					"!Disk Driver: Couldn't spin up "
					"disk for open %d: %d,%d,%d\n",
					SDI_HAN_32(dk->dk_addr),
				        SDI_BUS_32(dk->dk_addr),
					SDI_TCN_32(dk->dk_addr),
					SDI_LUN_32((dk->dk_addr));
#endif
		}
	}

	if(dm->dm_iotype & F_RMB) {
		if (sd01cmd(dk, SS_LOCK, 0, NULL, 0, 1, SCB_READ, TRUE))
		{
			/*EMPTY*/
#ifdef SD01_DEBUG
			if (! dk->dk_addr.pdi_adr_version )
				cmn_err(CE_WARN,
					 "!Disk Driver: Couldn't spin up "
					"disk for open %d: %d,%d,%d\n",
				SDI_EXHAN(dk->dk_addr), SDI_BUS(dk->dk_addr),
				SDI_EXTCN(dk->dk_addr), SDI_EXLUN(dk->dk_addr));
			else
				cmn_err(CE_WARN,
					"!Disk Driver: Couldn't spin up "
					"disk for open %d: %d,%d,%d\n",
					SDI_HAN_32(dk->dk_addr),
				        SDI_BUS_32(dk->dk_addr),
					SDI_TCN_32(dk->dk_addr),
					SDI_LUN_32((dk->dk_addr));
#endif
		}
	}

	SD01_DM_LOCK(dm);
	dm->dm_fo_dp = dk;
	SD01_DM_UNLOCK(dm);

	/*
	 * This is here for Compaq ProLiant Storage System support.  It could
	 * be ifdef'ed for performance
	 */
	if (! dk->dk_addr.pdi_adr_version ) {
		scsi_adr.scsi_ctl = SDI_EXHAN(&dk->dk_addr);
		scsi_adr.scsi_target = SDI_EXTCN(&dk->dk_addr);
		scsi_adr.scsi_lun = SDI_EXLUN(&dk->dk_addr);
		scsi_adr.scsi_bus = SDI_BUS(&dk->dk_addr);
	}
	else {
		scsi_adr.scsi_ctl = SDI_HAN_32(&dk->dk_addr);
		scsi_adr.scsi_target = SDI_TCN_32(&dk->dk_addr);
		scsi_adr.scsi_lun = SDI_LUN_32(&dk->dk_addr);
		scsi_adr.scsi_bus = SDI_BUS_32(&dk->dk_addr);
	}
		
	sdi_notifyevent(SDI_FIRSTOPEN, &scsi_adr, (struct sb *)NULL);

	return ret;
}

/*
 * int
 * sd01_last_close()
 *
 * this routine is called when all slices have partitions on a disk have
 * been closed.
 *
 * return SDI_RET_OK, SDI_RET_ERR
 */
int
sd01_last_close(struct disk *dk, int err)
{
	struct disk_media *dm = dk->dk_dm;
	struct disk *fo_dp = dm->dm_fo_dp;
	int ret=SDI_RET_OK;
	struct scsi_adr	scsi_adr;

	if (sd01_sync && (((dm->dm_ident.id_ver & 0x07) <= (unsigned)1) ||
	    sd01cmd(dk, SM_SYNC, 0, NULL, 0, 0, SCB_READ, FALSE)))
	{
		/*EMPTY*/
#if SD01_DEBUG
		if ( ! dk->dk_addr.pdi_adr_version ) {
			cmn_err(CE_WARN, 
				"!Disk Driver: Couldn't spin up disk for open"
			" %d: %d,%d,%d\n",
			SDI_EXHAN(&dk->dk_addr), SDI_BUS(&dk->dk_addr),
			SDI_EXTCN(&dk->dk_addr), SDI_EXLUN(&dk->dk_addr));
		}
		else {
			cmn_err(CE_WARN, 
				"!Disk Driver: Couldn't spin up disk for open"
			" %d: %d,%d,%d\n",
			SDI_HAN_32(&dk->dk_addr), SDI_BUS_32(&dk->dk_addr),
			SDI_TCN_32(&dk->dk_addr), SDI_LUN_32(&dk->dk_addr));
		}
#endif
	}

	if(dm->dm_iotype & F_RMB) {
		/*
		 * Unlock/Unreserve drive via the disk path used on
		 * first-open so that media can be removed.
		 */
		if (sd01cmd(fo_dp, SS_LOCK, 0, NULL, 0, 0, SCB_READ, TRUE))
		{
			/*EMPTY*/
#if SD01_DEBUG
		if ( ! fo_dp->dk_addr.pdi_adr_version ) {
			cmn_err(CE_WARN, "!Disk Driver: Cannot unlock drive"
			" %d: %d,%d,%d\n",
			SDI_EXHAN(&fo_dp->dk_addr), SDI_BUS(&fo_dp->dk_addr),
			SDI_EXTCN(&fo_dp->dk_addr), SDI_EXLUN(&fo_dp->dk_addr));
		}
		else {
			cmn_err(CE_WARN, "!Disk Driver: Cannot unlock drive"
			" %d: %d,%d,%d\n",
			SDI_HAN_32(&fo_dp->dk_addr), SDI_BUS_32(&fo_dp->dk_addr),
			SDI_TCN_32(&fo_dp->dk_addr), SDI_LUN_32(&fo_dp->dk_addr));
		}
#endif
		}
	}

	/*
	 * This is here for Compaq ProLiant Storage System support.  It could
	 * be ifdef'ed out for performance
	 */
	if (! dk->dk_addr.pdi_adr_version ) {
		scsi_adr.scsi_ctl = SDI_EXHAN(&dk->dk_addr);
		scsi_adr.scsi_target = SDI_EXTCN(&dk->dk_addr);
		scsi_adr.scsi_lun = SDI_EXLUN(&dk->dk_addr);
		scsi_adr.scsi_bus = SDI_BUS(&dk->dk_addr);
	}
	else {
		scsi_adr.scsi_ctl = SDI_HAN_32(&dk->dk_addr);
		scsi_adr.scsi_target = SDI_TCN_32(&dk->dk_addr);
		scsi_adr.scsi_lun = SDI_LUN_32(&dk->dk_addr);
		scsi_adr.scsi_bus = SDI_BUS_32(&dk->dk_addr);
	}
	sdi_notifyevent(err ? SDI_LASTCLOSE_ERR : SDI_LASTCLOSE,
						&scsi_adr, (struct sb *)NULL);

	return ret;
}

/*
 * void
 * sd01Reserve(struct disk *dk)
 *
 * Issue a reserve to the target and flag it in the dk structure.
 */
STATIC void
sd01Reserve(struct disk *dk)
{
	struct disk_media *dm = dk->dk_dm;
	int retval;

	retval = sd01cmd(dk, SS_RESERV, 0, (char *) 0, 0, 0, SCB_WRITE, TRUE);

	if (retval == 0) {
		SD01_DM_LOCK(dm);
		dm->dm_res_dp = dk;
		dm->dm_state |= (DMRESERVE|DMRESDEVICE);
		SD01_DM_UNLOCK(dm);
	}
}

/*
 * void
 * sd01Release(struct disk *dk)
 *
 * Issue a release to the target and flag it in the dk structure.
 */
STATIC void
sd01Release(struct disk *dk)
{
	struct disk_media *dm = dk->dk_dm;
	int retval;

	retval = sd01cmd(dk, SS_TEST, 0, (char *) 0, 0, 0, SCB_READ, TRUE);
	if (retval == 0) {
		retval = sd01cmd(dk, SS_RELES, 0, (char *) 0, 0, 0, SCB_WRITE, TRUE); 
		if (retval == 0) {
			SD01_DM_LOCK(dm);
			dm->dm_res_dp = NULL;
			dm->dm_state &= ~(DMRESERVE|DMRESDEVICE);
			SD01_DM_UNLOCK(dm);
		}
	}
}

int 
sd01_get_timeout(int command, struct disk *dk)
{
	if (pdi_timeout && dk->dk_timeout_table && dk->dk_timeout_table[command]) 
		return dk->dk_timeout_table[command];
	return JTIME;
}

void
sd01_aen(long parm, int event)
{
	struct disk *dk;
	dk = (struct disk *)parm;

	if(event == SDI_FLT_RESET) {
		if (! dk->dk_addr.pdi_adr_version )
			cmn_err(CE_WARN,
				"sd01_aen: Reset on SCSI bus: (c%db%dt%dd%d)",
			SDI_EXHAN(&dk->dk_addr), SDI_BUS(&dk->dk_addr), 
			SDI_EXTCN(&dk->dk_addr), SDI_EXLUN(&dk->dk_addr));
		else
			cmn_err(CE_WARN,
				"sd01_aen: Reset on SCSI bus: (c%db%dt%dd%d)",
			SDI_HAN_32(&dk->dk_addr), SDI_BUS_32(&dk->dk_addr), 
			SDI_TCN_32(&dk->dk_addr), SDI_LUN_32(&dk->dk_addr));
			
	}
}

int
sd01open(dev_t *dev, int flags, int otype, struct cred *cred_p)
{
	ASSERT(0);
	return(ENODEV);
}
int
sd01close(dev_t dev, int flags, int otype, struct cred *cred_p)
{
	ASSERT(0);
	return(0);
}

int
sd01ioctl(dev_t dev, int cmd, int arg, int mode, cred_t *cred_p, int *rval_p)
{
	ASSERT(0);
	return(0);
}

void
sd01strategy(buf_t *bp)
{
	ASSERT(0);
}
