#ident	"@(#)kern-pdi:io/hba/zl5380/hba.c	1.1.4.2"

/*
 * This file has the PDI specific functionality for 5380 based drivers
 */

#ifdef	_KERNEL_HEADERS

#include <svc/errno.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <io/target/scsi.h>
#include <io/target/sdi_edt.h>
#include <io/target/sdi.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <fs/buf.h>
#include "zl5380.h"
#include <io/hba/hba.h>
#include <util/mod/moddefs.h>

/* These must come last: */
#include <io/ddi.h>
#include <io/ddi_i386at.h>

#elif defined(_KERNEL)

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/cmn_err.h>
#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/debug.h>
#include <sys/confmgr.h>
#include "zl5380.h"
#include <sys/hba.h>
#include <sys/moddefs.h>
#include <sys/ddi.h>
#include <sys/ddi_i386at.h>

#endif	/* _KERNEL_HEADERS */

#define ZL5380_MEMALIGN		512
#define ZL5380_BOUNDARY		0
#define ZL5380_DMASIZE		0

#define ZL5380_BLKSHFT		9	/* Supports only 512 block size dev */

/*
 * Global array of pointers to ha structure
 */
STATIC struct 	zl5380ha	*zl5380scha[MAX_CNTLS];

STATIC 	int	zl5380_init_time;      	/* Initialization in progress */
STATIC 	int	zl5380_mod_dynamic = 0;	/* Driver is loaded dynamically */
STATIC	char	zl5380_init_flags = 0;	/* Already initialized */
STATIC	int	zl5380_num_scbs   = 0;	/* Number of scbs */
STATIC  int  	zl5380_io_per_tar = 1;  /* Number of jobs per target */
STATIC	int	zl5380_cntls;		/* Number of configured controllers */
STATIC  int	zl5380_hacnt;		/* Number of controllers in use */

STATIC	int	hacb_structsize;
STATIC	int	lucb_structsize;
STATIC	int	scb_structsize;
STATIC	int	luq_structsize;

STATIC 	int	zl5380devflag = HBA_MP;

STATIC 	char *zl5380adapter_strs[] =
{
	" UNKNOWN",
	" MV PAS16",
	" T160"
};

STATIC LKINFO_DECL(zl5380_lkinfo_scb, "IO:zl5380:zl5380ha->ha_scb_lock", 0);
STATIC LKINFO_DECL(zl5380_lkinfo_hba, "IO:zl5380:zl5380ha->ha_hba_lock", 0);
STATIC LKINFO_DECL(zl5380_lkinfo_q, "IO:zl5380:q->q_lock", 0);

/*
 *	Auto Configuration
 */
HBA_IDATA_DECL();
HBA_IDATA_DEF();

/*
 *	Variables defined in Space.c
 */
extern struct ver_no		zl5380_sdi_ver;
extern int			zl5380_gtol[];
extern int			zl5380_ltog[];

/*
 * sm_poolhead is used to allocate memory for the job requests from
 * the target driver
 */
extern struct head		sm_poolhead;

/*
 * Protypes for the routine in this file
 */

STATIC void 	zl5380_cleanup(int c, int mem_release);
STATIC void	zl5380_scb_init(zl5380ha_t *, int);
STATIC void	zl5380_next(struct zl5380scsi_lu *);
STATIC void	zl5380_func(sblk_t *);
STATIC void	zl5380_done(zl5380ha_t *, zl5380scb_t *);
STATIC void 	zl5380_ha_done(zl5380ha_t *, zl5380scb_t *);
STATIC void 	zl5380_flushq(struct zl5380scsi_lu *, int, int);
STATIC void 	zl5380_putq(struct zl5380scsi_lu *, sblk_t *);
STATIC void	zl5380_cmd(sblk_t *);
STATIC int 	zl5380_wait(int, int);
STATIC void 	zl5380_pass_thru0(buf_t *);
STATIC void 	zl5380_lockinit(int c);
STATIC void 	zl5380_lockclean(int c);
STATIC void 	zl5380_pass_thru(buf_t *);
STATIC void	zl5380_int(struct sb *);
STATIC zl5380scb_t	*zl5380_getblk(int);

void 		zl5380scb_done (zl5380ha_t *, zl5380scb_t *);


/*
 * Routines from the 'zl5380.c'
 */
extern int	zl5380findadapter (int);
extern void 	zl5380initialize (zl5380ha_t *);
extern void	zl5380transfer_scb (zl5380ha_t *, zl5380scb_t *); 
extern void	zl5380reset (zl5380ha_t *zl5380ha);
extern void	zl5380isr (zl5380ha_t *);
extern int	zl5380check_interrupt(zl5380ha_t *);

#define DRVNAME	"zl5380 - SCSI HBA Driver"

int	zl5380verify(rm_key_t);
MOD_ACHDRV_WRAPPER(zl5380, zl5380_load, zl5380_unload, NULL, 
			zl5380verify, DRVNAME);
HBA_INFO(zl5380, &zl5380devflag, 0x0);

/*
 * int
 * zl5380_load(void)
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 *	Returns 0 on success and ENODEV on failure
 *
 * Description:
 *	Dynamically loads the driver by invoking init and start routines
 *
 * Remarks:
 *	None
 */
int
zl5380_load(void)
{
	zl5380_mod_dynamic = 1;			/* Dynamically loaded */

	if (zl5380init() || zl5380start()) {
		SDI_ACFREE ();
		return(ENODEV);
	}

	return(0);
}


/*
 * int
 * zl5380_unload(void)
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Just returns EBUSY 
 *
 * Remarks:
 *	HBA drivers are not unloaded (see PDI manual). Hence nothing
 *	is done in this routine.
 */
int
zl5380_unload(void)
{
	return(EBUSY);
}

/*
 * int
 * zl5380verify(rm_key_t key)
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Verifies a board instance and returns an integer on success.
 *
 * Remarks:
 */
int
zl5380verify(rm_key_t key)
{
	HBA_IDATA_STRUCT	h_idata;
	int			rv;

	/*
	 * Read the hardware parameters associated with the given Resource
	 * Manager key, and assign them to the h_idata structure.
	 */
	rv = sdi_hba_getconf(key, &h_idata);
	if (rv != 0) {
		return(rv);
	}

	/*
	 * Check if the hardware is configuration is valid, i.e. the
	 * device exists at the specified address
	 */
	if (!zl5380findadapter(h_idata.ioaddr1)) {
	    /*
	     * Hardware not present
	     */
	    return (1);
	}

	return(0);
}

/*
 * int
 * zl5380init(void)
 *	Initialises the driver data structures.	
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 *	Returns 0 on success and -1 on failure
 *
 * Description:
 *	This routine is called at the time of initialization of the driver.
 *	It checks the hardware, and depending on the existence of the 
 *	controllers it initialises the relevant data. Attches interrupts.
 *
 * Remarks:
 *	None
 */
int
zl5380init(void)
{
	register zl5380ha_t	*zl5380ha;
	int			i;
	int			c;
	int			sleepflag;
	uint_t			bus_type;
	int			adapter_type;

	if(zl5380_init_flags) {
		/*
 		 *+ This driver has already been initialized
		 */
		cmn_err(CE_WARN, "%s - Already initialized.",
			zl5380idata[0].name);
		return(-1);
	}
	
	/*
	 * Initialize the idata with autoconfiguration information
	 */
	HBA_AUTOCONF_IDATA();

	zl5380_init_time = TRUE;	/* Initialization is in progress */

	for( i = 0 ; i < MAX_CNTLS ; ++i ) {
		zl5380_gtol[i] = zl5380_ltog[i] = -1;
	}
	
	sleepflag = zl5380_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;

	/*
	 * Initialize the sdi version variable 
	 */
	zl5380_sdi_ver.sv_release 	= 1;

	if (drv_gethardware(IOBUS_TYPE, &bus_type) == -1) {
		/*
		 *+ Failed to get the IOBUS type information
		 */
		cmn_err(CE_WARN, "%s: drv_gethardware() fails",
				zl5380idata[0].name);
		return(-1);
	}
	if (bus_type == BUS_EISA) {
		zl5380_sdi_ver.sv_machine 	= SDI_386_EISA;
	} else {
		zl5380_sdi_ver.sv_machine 	= SDI_386_AT;
	}	
	zl5380_sdi_ver.sv_modes 	= SDI_BASIC1;

	
	/*
	 * Only one job is being supported per target at a time.
	 * The rest will be queued.
	 * Maximum no of jobs with the driver is max no of targets (8)
	 * We add another 8 for those whose responses are being
	 * processed by the target
	 */

	zl5380_io_per_tar = 1;
	zl5380_num_scbs = (zl5380_io_per_tar * 8) + 8;

	hacb_structsize = sizeof(zl5380ha_t);
	lucb_structsize = MAX_EQ * sizeof(zl5380lucb_t);
	scb_structsize = zl5380_num_scbs * (sizeof(zl5380scb_t));
	luq_structsize = MAX_EQ * (sizeof(struct zl5380scsi_lu));

	zl5380_init_flags = 1;	/* Already initialized */
	zl5380_hacnt = 0;

	for( c = 0 ; c < zl5380_cntls ; c++ ) {
		adapter_type = zl5380findadapter(zl5380idata[c].ioaddr1);

		if(!adapter_type) {
			/*
			 *+ Adapter hardware is missing at the a configured
			 *+ location
			 */
			cmn_err(CE_NOTE, "!%s: No HBA's found at port 0x%x.",
				zl5380idata[c].name, zl5380idata[c].ioaddr1);
			continue;
		}
		PDI_FOUND(zl5380idata[c].name, zl5380idata[c].ioaddr1, c, 
			  zl5380idata[c].iov);

		/* ha structure */

		zl5380ha = (zl5380ha_t *)kmem_zalloc(hacb_structsize, 
						     sleepflag);
		if(zl5380ha == NULL) {
			/*
			 *+ Memory allocation failed during initialization
			 */
			cmn_err(CE_WARN, 
				"%s: Cannot allocate zl5380ha_t blocks",
				zl5380idata[c].name);
			continue;
		}

		zl5380scha[c] 		= zl5380ha;
		zl5380ha->ha_name      	= zl5380idata[c].name;
		zl5380ha->ha_base      	= zl5380idata[c].ioaddr1;
		zl5380ha->ha_vect      	= zl5380idata[c].iov;
		zl5380ha->ha_id        	= zl5380idata[c].ha_id;
		zl5380ha->ha_bitid        	= (1 << zl5380ha->ha_id);
		zl5380ha->ha_type       = adapter_type;

		/*
		 * By default these fields are 0(kmem_zalloc zeroes all bytes):
		 *	 zl5380ha->ha_currentscb
		 *	 zl5380ha->ha_eligiblescb
		 */

		/* lu queue */

		zl5380ha->ha_dev  = (struct zl5380scsi_lu *) 
		                  kmem_zalloc(luq_structsize, sleepflag);
		if(zl5380ha->ha_dev == NULL) {
			/*
			 *+ Memory allocation failed for LU queues
			 */
			cmn_err(CE_WARN, 
				"%s: Cannot allocate lu queues",
				zl5380idata[c].name);
			zl5380_cleanup (c, HA_ZL5380HA_REL);
			continue;
		}
	
		/*
		 * The fields of the queues are set to 0 by default 
		 * (kmem_zalloc initializes all fields to 0).
		 */

		/* scb queue */

		zl5380ha->ha_scb  = (struct zl5380scb *) 
		                        kmem_zalloc(scb_structsize, sleepflag);

		if(zl5380ha->ha_scb == NULL) {
			/*
			 *+ Memory allocation failed for SCB blocks
			 */
			cmn_err(CE_WARN, 
				"%s: Cannot allocate SCB blocks",
				zl5380idata[c].name);
			zl5380_cleanup (c, HA_ZL5380HA_REL|HA_DEV_REL);
			continue;
		}

		/* lucb queue */

		zl5380ha->ha_lucb = (zl5380lucb_t *)
				    kmem_zalloc(lucb_structsize, sleepflag);

		if(zl5380ha->ha_lucb == NULL) {
			/*
			 *+ Memory allocation failed for LUCB
			 */
			cmn_err(CE_WARN, 
				"%s: Cannot allocate lucb command blocks",
				zl5380idata[c].name);
			zl5380_cleanup(c, 
				 HA_ZL5380HA_REL|HA_DEV_REL|HA_ZL5380SCB_REL);
			continue;
		}

		zl5380_scb_init(zl5380ha, zl5380_num_scbs);
	
		/*
		 * Initialize the adapter hardware 
		 */
		zl5380initialize (zl5380ha);

		zl5380_lockinit(c);
		zl5380idata[c].active = 1;
		zl5380_hacnt ++;	/* One more cntlr that can be used */
	}

	SDI_INTR_ATTACH();
	/*
	 * sdi_intr_attach will set the active flag to false for those
	 * controllers that have been configured for no interrupts.
	 * Therefore to check for presence of controller, the scha entry
	 * for the controller must be non null
	 */

	if (zl5380_hacnt == 0) {
		/*
		 *+ Could not initialize for configured adapters 
		 */
		cmn_err (CE_WARN, "!ZL5380: No controllers initialized");
		zl5380_init_flags = 0;
		return -1;
	}

	return(0);
}

/*
 * int
 * zl5380start(void)
 *	Does further initialization after zl5380init.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 *	Returns 0 on success and -1 on failure
 *
 * Description:
 *	Called by kernel to perform driver registration after the kernel data
 *	area has been initialized. Registers adapters with SDI.
 *
 * Remarks:
 */
int
zl5380start(void)
{
	register int	cntl_num;
	register int	c;


	/*
	 * Initialize the version number field of the idata array
	 */
	HBA_IDATA (zl5380_cntls);	

	/*
	 * Get the next available hba no and register with the SDI
	 */
	for ( c=0; c < zl5380_cntls; c++ )
	{
		if (!zl5380scha[c]) {
			/* 
			 * No controller is present for this number
			 * Refer to comment on SDI_INTR_ATTACH also
			 */
			continue;
		}

		if (!zl5380check_interrupt(zl5380scha[c])) {
			/*
			 *+ The driver will operate in polling mode as 
			 *+ it cannot use interrupts.  This will degrade 
			 *+ the performance of the system.
			 */
			cmn_err (CE_WARN, "%s: interrupts not being used.  Enable interrupts for improved performance",
				zl5380idata[c].name);
		}

		if((cntl_num = sdi_gethbano(zl5380idata[c].cntlr)) <= -1) {
			/*
			 *+ No new HBA number is available from SDI
			 */
			cmn_err(CE_WARN, "!%s: No HBA number available. %d",
					zl5380idata[c].name, cntl_num);
			zl5380_cleanup(c, HA_ALL_REL);

			/* This controller cannot be used */
			zl5380_hacnt --;	

			continue;
		}

		/*
		 * Set up the local to global an inverse mappings for HBA nos
		 */
		zl5380idata[c].cntlr	= cntl_num;
		zl5380_gtol[cntl_num]	= c;
		zl5380_ltog[c]		= cntl_num;

		if((cntl_num = sdi_register(&zl5380hba_info, &zl5380idata[c]))
							< 0) {
			/*
			 *+ Failed to register adapter with SDI
			 */
			cmn_err(CE_WARN, "!%s: SDI registration failure %d.",
					zl5380idata[c].name, cntl_num);
			zl5380_cleanup(c, HA_ALL_REL);

			/* This controller cannot be used */
			zl5380_hacnt --;	

			continue;
		}
	} /* for loop */

	if(zl5380_hacnt == 0) {
		/*
		 *+ No Adapter hardware has been detected on the system
		 */
		cmn_err(CE_WARN, "!%s: No HBAs found.", zl5380idata[0].name);
		return(-1);
	}
	
	zl5380_init_time = FALSE;        	/* Initialisation is complete */

	return(0);
}


/*
 * STATIC void
 * zl5380_cleanup(int c, int mem_release)
 *	Cleans up the data structures allocated for a controller
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	c is the controller number, while mem_release indicates the
 * 	kind of cleaning up to be performed
 */
void zl5380_cleanup(int c, int mem_release)
{
	register zl5380ha_t	*zl5380ha = zl5380scha[c];

	if (mem_release & HA_LOCK_REL) {
		zl5380_lockclean(c);
	}

	if(mem_release & HA_LUCB_REL) {
		kmem_free(zl5380ha->ha_lucb, lucb_structsize);
		zl5380ha->ha_lucb = NULL;
	}
	if(mem_release & HA_DEV_REL) {
		kmem_free(zl5380ha->ha_dev, luq_structsize);
		zl5380ha->ha_dev = NULL;
	}
	if(mem_release & HA_ZL5380SCB_REL) {
		kmem_free(zl5380ha->ha_scb, scb_structsize);
		zl5380ha->ha_scb = NULL;
	}
	if(mem_release & HA_ZL5380HA_REL) {
		kmem_free(zl5380ha, hacb_structsize);
		zl5380scha[c] = NULL;
		zl5380idata[c].active = 0;
	}
}

/*
 * STATIC void
 * zl5380_lockinit(int c)
 *	Initialize zl5380 locks:
 *		1) Device queue locks (one per queue),
 *		2) scb lock (one per controller),
 *		3) and controller lock (one per controller).
 *
 * Calling/Exit Status:
 *	None
 */
 STATIC void
 zl5380_lockinit(int c)
 {
	zl5380ha_t	*zl5380ha = zl5380scha[c];
	struct zl5380scsi_lu	*q;
	int		t;
	int		l;
	int		sleepflag;

	sleepflag = zl5380_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;
	zl5380ha->ha_hba_lock = LOCK_ALLOC(ZL5380_HIER, pldisk,
					 &zl5380_lkinfo_hba, sleepflag);
	zl5380ha->ha_scb_lock = LOCK_ALLOC(ZL5380_HIER, pldisk,
					 &zl5380_lkinfo_scb, sleepflag);
	for (t = 0 ; t < MAX_TCS ; t++) {
		for (l = 0 ; l < MAX_LUS ; l++) {
			q = &LU_Q(c,t,l);
			q->q_lock = LOCK_ALLOC(ZL5380_HIER, pldisk,
					 &zl5380_lkinfo_q, sleepflag);
		}
	}
}


/*
 * STATIC void
 * zl5380_lockclean(int c)
 *	Removes locks which are not required. Controllers that are not active
 *	will have all locks removed. Active controllers will have locks for all
 *	non-existent devices removed.
 *
 * Calling/Exit Status:
 *	None
 */
STATIC void
zl5380_lockclean(int c)
{
	struct zl5380scsi_lu	*q;
	zl5380ha_t	*zl5380ha = zl5380scha[c];
	int		t;
	int		l;

	if (!zl5380ha || !zl5380ha->ha_hba_lock)
		return;
	LOCK_DEALLOC(zl5380ha->ha_scb_lock);
	LOCK_DEALLOC(zl5380ha->ha_hba_lock);

	for (t = 0; t < MAX_TCS ; t++) {
		for (l = 0; l < MAX_LUS; l++) {
			q = &LU_Q(c,t,l);
			LOCK_DEALLOC(q->q_lock);
		}
	}
}

/*
 * STATIC void
 * zl5380_scb_init(zl5380ha_t *, int)
 *	Initialize the adapter SCB free list
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 *	A pointer to the HBA structure (zl5380ha_t *) is sent along with
 *	the total number of SCBs which have to be initialized.
 *
 * Description:
 *	All the SCBs are linked to form a linked list
 *	Since SCBs have been allocated thru kmem_zalloc all other fields 
 *	by default 0.
 *
 * Remarks:
 */
STATIC void
zl5380_scb_init(zl5380ha_t *zl5380ha, int cnt)
{
	register int		i;

	/*
	 * All other fields have been set to zero by kmem_zalloc
	 */

	for( i = 0 ; i < cnt -1; i++ ) {
		zl5380ha->ha_scb[i].c_next = &zl5380ha->ha_scb[i+1];
	}

	zl5380ha->ha_scb_next = zl5380ha->ha_scb[i].c_next = 
					&zl5380ha->ha_scb[0];
}

/*ARGSUSED*/
/*
 * int
 * zl5380open(dev_t *, int, int, cred_t *)
 *	Open pass thru entry point.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *	Acquires ZL5380_SCSILU_LOCK (q).
 *
 *	Returns 0 on success.
 *
 * Description:
 * 	Driver open() entry point. It checks permissions, and in the
 *	case of a pass-thru open, suspends the particular LU queue.
 *
 * Remarks:
 */
int
HBAOPEN(dev_t *devp, int flags, int otype, cred_t *cred_p)
{
	register zl5380scsi_lu_t *q;			/* Device LU queue */
	dev_t			dev = *devp;		/* Device number */
	register int		c = zl5380_gtol[SC_HAN(dev)];	/* Cntl No. */
	register int		t = SC_TCN(dev);	/* Target number */
	register int		l = SC_LUN(dev);	/* LU number */
	zl5380ha_t		*zl5380ha = zl5380scha[c];

	/*
	 * Check whether the device number sent is valid.
	 */
	if (!sdi_redt(SC_HAN(dev), t, l)) {
		return(ENXIO);
	}

 	/*
	 * If the target value is the HBA controller then just return
	 */
	if (t == zl5380ha->ha_id)
		return(0);

	/*
	 * This is the pass-thru section
	 */

	q = &LU_Q(c, t, l);	/* Get the first element of the LU queue */

	ZL5380_SCSILU_LOCK(q);

 	if ((q->q_count > 0)  || (q->q_flag & (QBUSY | QSUSP | QPTHRU))) {
		/*
		 * The device is either busy or suspended or someone else has
		 * done a pass thru open earlier.
		 */

		ZL5380_SCSILU_UNLOCK(q);
		return(EBUSY);	/* Return the status as busy */
	}

	/*
	 * The device selected is free
	 * Mark the device LU queue for the pass thru open
	 */
	q->q_flag |= QPTHRU;

	ZL5380_SCSILU_UNLOCK(q);

	return(0);	/* Return success */
}


/*ARGSUSED*/
/*
 * int
 * zl5380close(dev_t dev, int flags, int otype, cred_t *cred_p)
 *	Driver close() entry poin.
 *
 * Calling/Exit Status:
 *	No locks are held on entry.
 *	ZL5380_SCSILU_LOCK (q) held on exit.
 *
 *	Returns 0 on success
 *
 * Description:
 * 	Driver close() entry point.  In the case of a pass-thru close
 *	it resumes the queue and calls the target driver event handler
 *	if one is present.
 *
 * Remarks:
 */
int
HBACLOSE(dev_t dev, int flags, int otype, cred_t *cred_p)
{
	register zl5380scsi_lu_t *q;		/* Device LU queue pointer  */
	register int	c = zl5380_gtol[SC_HAN(dev)];	/* Cntl No. */
	register int	t = SC_TCN(dev);	/* Target number    */
	register int	l = SC_LUN(dev);	/* LU number        */
	zl5380ha_t	*zl5380ha = zl5380scha[c];	/* HBA structure ptr */

	/*
	 * Check whether the device number sent is valid.
	 */
	if (!sdi_redt(SC_HAN(dev), t, l)) {
		return(ENXIO);
	}

 	/*
	 * If the target value is the HBA controller then just return
	 */
	if (t == zl5380ha->ha_id)
		return(0);

	/* 
	 * Getting the LU's relevant LU structure. The memory for which is
	 * allocated at the time of initialization.
	 */
	q = &LU_Q(c, t, l);

	ZL5380_CLEAR_QFLAG(q, QPTHRU);

	/*
	 * Call the target driver event handler.
	 */
	if (q->q_func != NULL)
		(*q->q_func) (q->q_param, SDI_FLT_PTHRU);

	ZL5380_SCSILU_LOCK(q);
	/*
	 * Now fire the next command in the queue through zl5380_next
	 */
	zl5380_next(q);

	return(0);	/* Return success */
}

/*
 * STATIC void
 * zl5380_next(struct zl5380scsi_lu *q)
 *
 * Calling/Exit Status:
 *	ZL5380_SCSILU_LOCK (q) is held on entry.
 *	No locks are held on exit.
 *
 *	q - Pointer to the SCB list.
 *
 *	Returns 0 on success and ENODEV on failure
 *
 * Description:
 *	Attempt to send the next job on the logical unit queue.
 *	All jobs are not sent if the Q is busy.
 *
 * Remarks:
 */
STATIC void
zl5380_next(struct zl5380scsi_lu *q)
{
	register sblk_t			*sp;

	/*
	 *+ q pointer must not be NULL
	 */
	ASSERT(q);

	
	if (q->q_flag & QBUSY) {
		ZL5380_SCSILU_UNLOCK(q);
		return;
	}

	if((sp = q->q_first) == NULL) {
		ZL5380_SCSILU_UNLOCK(q);	
		return;
	}

	if ((q->q_flag & QSUSP) && (sp->sbp->sb.sb_type == SCB_TYPE)) {
		ZL5380_SCSILU_UNLOCK(q);
		return;
	}

	/*
	 * Update the head and tail pointers of the queue
	 */
	q->q_first = sp->s_next;
	if (q->q_first) {
		/*
		 * There is an entry in the queue, q_last remains unchanged
		 */
		q->q_first->s_prev = NULL;
	} else {
		/*
	 	 * There are no more entries in the queue, q_last to be updated
		 */
		q->q_last = NULL;
	}	

	q->q_count++;
	if(q->q_count >= zl5380_io_per_tar)
		q->q_flag |= QBUSY;
	ZL5380_SCSILU_UNLOCK(q);

	if (sp->sbp->sb.sb_type == SFB_TYPE) {
		zl5380_func(sp);
	} else {
		zl5380_cmd(sp);
	}

	if(zl5380_init_time == TRUE) {
		/*
		 * Initialization in progress. Inquiry commands are processed
		 * when registration is done.  Therefore control is returned
		 * from here after ensuring that the request is completed
		 */
		if(zl5380_wait(zl5380_gtol[SDI_HAN(&sp->sbp->sb.SCB.sc_dev)], 100) ) {
			sp->sbp->sb.SCB.sc_comp_code = (unsigned long)SDI_TIME;
			sdi_callback(&sp->sbp->sb);
		}
	}
}


/*
 * STATIC void
 * zl5380_func(sblk_t *sp)
 *
 * Calling/Exit Status:
 *	No locks are held on entry or exit.
 * 	Acquires ZL5380_HBA_LOCK (oip).
 *
 * Description:
 *	Creates and sends an SFB associated command.
 *
 * Remarks:
 */
STATIC void
zl5380_func(sblk_t *sp)
{
	register struct scsi_ad	*sa;
	register zl5380ha_t	*zl5380ha;
	register zl5380scb_t	*cp;
	register int		c;

	sa 	= &sp->sbp->sb.SFB.sf_dev;
	c 	= zl5380_gtol[SDI_HAN(sa)];
	cp 	= zl5380_getblk(c);
	zl5380ha= zl5380scha[c];

	cp->c_opcode 	= SCB_BUS_DEVICE_RESET;
	cp->c_target 	= (unsigned char) SDI_TCN(sa);
	cp->c_lun 	= (unsigned char) (sa->sa_lun);
	cp->c_datalength= 0;
	cp->c_adapter 	= c;

	/*
	 *+ Device connected to this controller is being reset
	 */
	cmn_err(CE_WARN, "%s: TC%d LUN%d is being reset",
		zl5380ha->ha_name, cp->c_target, cp->c_lun);

	zl5380transfer_scb(zl5380ha, cp);

}


/*
 * void
 * zl5380scb_done (zl5380ha_t *zl5380ha, register zl5380scb_t *scb_ptr)
 *
 * Calling/Exit Status:
 *	No locks are held on entry or exit.
 *
 * Description:
 *
 * Remarks:
 */
void
zl5380scb_done (zl5380ha_t *zl5380ha, register zl5380scb_t *scb_ptr)
{
       	if(scb_ptr->c_opcode == SCB_EXECUTE) {
              	zl5380_done(zl5380ha, scb_ptr);
       	}
       	else {
	        zl5380_ha_done(zl5380ha, scb_ptr);
      	}
}

/*
 * STATIC void
 * zl5380_done(zl5380scb_t *cp)
 *
 * Calling/Exit status:
 *	No locks are held on entry.
 *	Acquires ZL5380_HBA_LOCK (oip), ZL5380_SCB_LOCK (oip).
 *
 * Description:
 *	This is the interrupt handler routine for SCSI jobs which have
 *	a controller CB and a SCB structure defining the job.
 *
 * Remarks:
 */
STATIC void
zl5380_done(zl5380ha_t *zl5380ha, zl5380scb_t *cp)
{
	register struct zl5380scsi_lu	*q;
	pl_t			oip;
	register struct sb *	sp = cp->c_bind;

	q = &LU_Q(cp->c_adapter, cp->c_target, cp->c_lun);
	ZL5380_CLEAR_QFLAG (q,QSENSE);

	switch(SCB_STATUS (cp->c_status)) {
	case SCB_INVALID_LUN:
		sp->SCB.sc_comp_code = (unsigned long) SDI_NOTEQ;
		break;
	case SCB_COMPLETED_OK:
		sp->SCB.sc_comp_code = (unsigned long) SDI_ASW;
		break;
	case SCB_ABORTED:
		sp->SCB.sc_comp_code = (unsigned long)SDI_ABORT;
		break;
	case SCB_TIMEOUT:
		sp->SCB.sc_comp_code = (unsigned long)SDI_RETRY;
		break;
	case SCB_SELECTION_TIMEOUT:
		sp->SCB.sc_comp_code = (unsigned long)SDI_RETRY;
		break;
	case SCB_ERROR:
		if(cp->c_status & SCB_SENSE_DATA_VALID) {
			sp->SCB.sc_comp_code = (unsigned long)SDI_CKSTAT;
			sp->SCB.sc_status = S_CKCON;
			HBA_SENSE_COPY (SENSE_AD (&cp->c_sense), sp);
			bcopy((caddr_t)(&cp->c_sense),
				(caddr_t)(&q->q_sense), sizeof (q->q_sense));
			ZL5380_SET_QFLAG(q,QSENSE);
		} else {
			sp->SCB.sc_comp_code = (unsigned long)SDI_ERROR;
		}
		break;
	default:
		sp->SCB.sc_comp_code = (unsigned long)SDI_ERROR;
		break;
	}

	sdi_callback(sp);
	ZL5380_SCB_LOCK (oip);
	FREEBLK(cp);
	ZL5380_SCB_UNLOCK (oip);
	ZL5380_SCSILU_LOCK(q);
	q->q_count--;
	q->q_flag &= ~QBUSY;

	zl5380_next(q);	/* ZL5380_SCSILU_LOCK(q) is released here */
} 



/*
 * STATIC void
 * zl5380_ha_done(zl5380scb_t *cp)
 *
 * Calling/Exit status:
 *	Acquires ZL5380_HBA_LOCK (oip), ZL5380_SCB_LOCK (oip), ZL5380_SCSILU_LOCK(q).
 *	No locks are held on entry or exit.
 *
 * Description:
 *	This is the interrupt handler routine for SCSI jobs which have
 *	a controller CB and a SFB structure defining the job.
 *
 * Remarks:
 */
STATIC void
zl5380_ha_done(zl5380ha_t *zl5380ha, zl5380scb_t *cp)
{
	register struct zl5380scsi_lu	*q;
	register struct sb	*sp = cp->c_bind;
	pl_t			oip;

	q = &LU_Q(cp->c_adapter, cp->c_target, cp->c_lun);

	switch(SCB_STATUS (cp->c_status)) {
	case SCB_INVALID_LUN:
		sp->SFB.sf_comp_code = (unsigned long)SDI_NOTEQ;
		break;
	case SCB_COMPLETED_OK:
		sp->SFB.sf_comp_code = (unsigned long) SDI_ASW;
		zl5380_flushq(q, SDI_CRESET, 0);
		break;
	case SCB_ABORTED:
		sp->SFB.sf_comp_code = (unsigned long)SDI_ABORT;
		break;
	case SCB_TIMEOUT:
		sp->SFB.sf_comp_code = (unsigned long)SDI_RETRY;
		break;
	case SCB_SELECTION_TIMEOUT:
		sp->SFB.sf_comp_code = (unsigned long)SDI_RETRY;
		break;
	case SCB_ERROR:
		sp->SFB.sf_comp_code = (unsigned long)SDI_CKSTAT;
		break;
	default:
		sp->SFB.sf_comp_code = (unsigned long)SDI_ERROR;
		break;
	}

	sdi_callback(sp);
	ZL5380_SCB_LOCK (oip);
	FREEBLK(cp);
	ZL5380_SCB_UNLOCK (oip);
	ZL5380_SCSILU_LOCK(q);
	q->q_count--;
	q->q_flag &= ~QBUSY;

	zl5380_next(q);	/* ZL5380_SCSILU_LOCK(q) is released here */
}



/*
 * STATIC void
 * zl5380_flushq(struct zl5380scsi_lu *q, int cc, int flag)	
 *
 * Calling/Exit status:
 *	No locks are held on entry or exit.
 *	Acquires ZL5380_SCSILU_LOCK (q).
 *
 * Description:
 *	Empty a logical unit queue.  If flag is set, remove all jobs.
 *	Otherwise, remove only non-control jobs.
 *
 * Remarks:
 */
STATIC void
zl5380_flushq(struct zl5380scsi_lu *q, int cc, int flag)	
{
	register sblk_t *sp;
	register sblk_t	*nsp;

	/*
	 *+ q pointer must not be NULL
	 */
	ASSERT(q);

	/*
	 * Remove all entries from the queue 
	 */
	ZL5380_SCSILU_LOCK(q);
	sp 		= q->q_first;
	q->q_first 	= q->q_last = NULL;
	q->q_count 	= 0;
	ZL5380_SCSILU_UNLOCK(q);

	/*
	 * Keep only the priority job in the queue
	 * Return error for the rest to the target
	 */
	while (sp) {
		nsp = sp->s_next;
		if (!flag && (ZL_QUECLASS(sp) > HBA_QNORM)) {
			ZL5380_SCSILU_LOCK(q);
			zl5380_putq(q, sp);
			ZL5380_SCSILU_UNLOCK(q);
		} else {
			sp->sbp->sb.SCB.sc_comp_code = (ulong)cc;
			sdi_callback(&sp->sbp->sb);
		}
		sp = nsp;
	}
}

/*
 * STATIC void
 * zl5380_putq(struct zl5380scsi_lu *q, sblk_t *sp)
 *
 * Calling/Exit status:
 *      ZL5380_SCSILU_LOCK (q) is held on entry and exit.
 *
 * Description:
 *	Put a job on a logical unit queue.  Jobs are enqueued
 *	on a priority basis.
 *
 * Remarks:
 */
STATIC void
zl5380_putq(struct zl5380scsi_lu *q, sblk_t *sp)
{
	int cls;
	sblk_t *ip = q->q_last;	

	cls = ZL_QUECLASS(sp);
	if (!ip) {
		/*
		 * Adding to an empty queue
		 */
		sp->s_prev = sp->s_next = NULL;
		q->q_first = q->q_last = sp;
		return;
	}
	cls = ZL_QUECLASS(sp);
	while(ip && ZL_QUECLASS(ip) < cls)
		ip = ip->s_prev;

	if (!ip) {
		/*
		 * Adding at the head of the queue
		 */
		sp->s_next = q->q_first;
		sp->s_prev = NULL;
		q->q_first = q->q_first->s_prev = sp;
	} else if (!ip->s_next) {
		/*
		 * Add at the end of the existing queue
		 */
		sp->s_next = NULL;
		sp->s_prev = q->q_last;
		q->q_last = ip->s_next = sp;
	} else {
		/*
		 * Adding in the middle of the queue
		 */
		sp->s_next = ip->s_next;
		sp->s_prev = ip;
		ip->s_next = ip->s_next->s_prev = sp;
	}
}
		
/*
 * STATIC void
 * zl5380_cmd(sblk_t *sp)
 *
 * Calling/Exit status:
 *	No locks are held on entry or exit.
 *	Acquires ZL5380_SCSILU_LOCK (q).
 *
 * Description:
 *	Create and send an SCB associated command
 *
 * Remarks:
 */
STATIC void
zl5380_cmd(sblk_t *sp)
{
	register struct zl5380scsi_lu    	*q;
	register struct scsi_ad		*sa;
	register zl5380scb_t		*cp;
	register int			c;

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = zl5380_gtol[SDI_HAN(sa)];
	cp = zl5380_getblk(c);


	cp->c_chain	= NULL;
	cp->c_opcode 	= SCB_EXECUTE;
	cp->c_status 	= 0;
	cp->c_target 	= SDI_TCN(sa);
	cp->c_lun 	= sa->sa_lun;
	cp->c_cmdsz 	= sp->sbp->sb.SCB.sc_cmdsz;
	cp->c_adapter 	= c;
	cp->c_bind 	= &sp->sbp->sb;

	if (sp->sbp->sb.SCB.sc_mode & SCB_READ) {
		cp->c_flags = SCB_DATA_IN;
	} else {
		cp->c_flags = SCB_DATA_OUT;
	}

	cp->c_datapointer = sp->sbp->sb.SCB.sc_datapt;
	cp->c_datalength = sp->sbp->sb.SCB.sc_datasz;

	/*
	 *+ Command size cannot exceed max command size
	 */
	ASSERT(cp->c_cmdsz <= MAX_CMDSZ);

	bcopy (sp->sbp->sb.SCB.sc_cmdpt, cp->c_cdb, cp->c_cmdsz);

	q = &LU_Q(c, cp->c_target, sa->sa_lun);
	ZL5380_SCSILU_LOCK(q); 
	if ((cp->c_cdb[0] == SS_REQSEN) && (q->q_flag & QSENSE)) {
		HBA_SENSE_COPY (SENSE_AD (&q->q_sense), cp->c_bind);
		q->q_flag &= ~QSENSE;
		ZL5380_SCSILU_UNLOCK(q); 
		cp->c_status = SCB_COMPLETED_OK;
		zl5380_done(zl5380scha[c], cp);
	} else {
		ZL5380_SCSILU_UNLOCK(q); 
		zl5380transfer_scb(zl5380scha[c], cp);
	}

}


/*
 * STATIC zl5380scb_t *
 * zl5380_getblk(int c)
 *
 * Calling/Exit status:
 *	No locks are held on entry or exit.
 *	Acquires ZL5380_SCB_LOCK (oip).
 *
 * Description:
 *	Allocate a controller command block structure.
 *
 * Remarks:
 */
STATIC zl5380scb_t *
zl5380_getblk(int c)
{
	register zl5380ha_t	*zl5380ha = zl5380scha[c];
	register zl5380scb_t	*cp;
	register int		cnt = 0;
	pl_t			oip;

	ZL5380_SCB_LOCK(oip);
	cp = zl5380ha->ha_scb_next;
	while(cp->c_active == TRUE) {
		if(++cnt >= zl5380_num_scbs) {	
			/*
			 *+ The no. of jobs is exceeding the maximum 
			 *+ supported
			 */
			cmn_err(CE_PANIC, "%s: Out of command blocks",
				 zl5380ha->ha_name);
		}
		cp = cp->c_next;
	}
	zl5380ha->ha_scb_next = cp->c_next;
	cp->c_active = TRUE;
	ZL5380_SCB_UNLOCK(oip);
	zl5380ha->ha_poll = cp;
	return(cp);
}



/*
 * STATIC int
 * zl5380_wait(int c, int time)
 *
 * Calling/Exit status:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Poll for a completion from the host adapter.
 *
 * Remarks:
 *	This routine allows for no concurrency and as such, should
 *	be used selectively.
 */
STATIC int
zl5380_wait(int c, int time)
{
	register zl5380ha_t	*zl5380ha = zl5380scha[c];
	zl5380scb_t	       	*cp;

	cp = zl5380ha->ha_poll;

	while(time > 0) {
		if(cp->c_active != TRUE) {
		        /*
			 * Job is complete
			 */
			return(0);
		}
		drv_usecwait(1000);
		time--;
	}
 	return (1);	
}

/*
 * void
 * zl5380intr(uint vect)
 *
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Driver interrupt handler entry point.  Called by kernel when
 *	a host adapter interrupt occurs.
 *
 * Remarks:
 */
void
zl5380intr(uint vect)
{
	register zl5380ha_t      	*zl5380ha;
	register int	       	adapter;

	for(adapter = 0; adapter < zl5380_cntls; adapter++) {
		if (!(zl5380ha = zl5380scha[adapter])) {
			continue;
		}
		if(zl5380ha->ha_vect == vect)
			zl5380isr(zl5380ha);
	}
}

/*ARGSUSED*/
/*
 * int
 * zl5380ioctl(dev_t dev,int cmd,caddr_t arg,int mode,cred_t *cred_p,int *rvalp)
 *
 *	No locks are held on entry or exit.
 *	Acquires ZL5380_HBA_LOCK (oip).
 *
 * Description:
 *	Driver ioctl() entry point.  Used to implement the following
 *	special functions:
 *
 *	SDI_SEND     -	Send a user supplied SCSI Control Block to
 *			the specified device.
 *	B_GETTYPE    -  Get bus type and driver name
 *	SDI_BRESET   -	Reset the specified SCSI bus 
 *	HA_VER	     -  Gets the HBA driver version number
 *
 * Remarks:
 */
int
HBAIOCTL(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *cred_p, int *rval_p)
{
	register int	c = zl5380_gtol[SC_HAN(dev)];
	register int	t = SC_TCN(dev);
	register int	l = SC_LUN(dev);
	register struct sb *sp;
	pl_t  oip;
	int  uerror = 0;

	switch(cmd) {
	case SDI_SEND: {
		register buf_t *bp;
		struct sb  karg;
		int  rw;
		char *save_priv;
		struct zl5380scsi_lu	*q;

		if (t == zl5380scha[c]->ha_id) { 	/* illegal ID */
			return(ENXIO);
		}
		if (copyin(arg, (caddr_t)&karg, sizeof(struct sb))) {
			return(EFAULT);
		}
		if ((karg.sb_type != ISCB_TYPE)||(karg.SCB.sc_cmdsz <= 0 )   ||
		    	(karg.SCB.sc_cmdsz > MAX_CMDSZ )) { 
			return(EINVAL);
		}

		sp = SDI_GETBLK(KM_SLEEP);
		save_priv = sp->SCB.sc_priv;
		bcopy((caddr_t)&karg, (caddr_t)sp, sizeof(struct sb));

		bp = getrbuf(KM_SLEEP);
		sp->SCB.sc_priv = save_priv;

		q = &LU_Q(c, t, l);
		if (q->q_sc_cmd == NULL) {
			/*
			 * Allocate space for the SCSI command and add 
			 * sizeof(int) to account for the scm structure
			 * padding, since this pointer may be adjusted -2 bytes
			 * and cast to struct scm.  After the adjustment we
			 * still want to be pointing within our allocated space!
			 */
			q->q_sc_cmd = (char *)kmem_zalloc(MAX_CMDSZ + 
					sizeof(int), KM_SLEEP) + sizeof(int);
		}
		sp->SCB.sc_cmdpt = q->q_sc_cmd;

		if (copyin(karg.SCB.sc_cmdpt, sp->SCB.sc_cmdpt,
		    	sp->SCB.sc_cmdsz)) {
			uerror = EFAULT;
			goto done;
		}
		sp->SCB.sc_dev.sa_lun = (uchar_t)l;
		sp->SCB.sc_dev.sa_fill = (zl5380_ltog[c] << 3) | t;

		rw = (sp->SCB.sc_mode & SCB_READ) ? B_READ : B_WRITE;
		bp->b_private = (char *)sp;

		/*
		 * If the job involves a data transfer then the
		 * request is done thru physiock() so that the user
		 * data area is locked in memory. If the job doesn't
		 * involve any data transfer then zl5380_pass_thru()
		 * is called directly.
		 */
		if (sp->SCB.sc_datasz > 0) { 
			struct iovec  ha_iov;
			struct uio    ha_uio;
			uchar_t	      op = sp->SCB.sc_cmdpt[0];

			ha_iov.iov_base = sp->SCB.sc_datapt;	
			ha_iov.iov_len = sp->SCB.sc_datasz;	
			ha_uio.uio_iov = &ha_iov;
			ha_uio.uio_iovcnt = 1;
			ha_uio.uio_offset = 0;

			/*
 			 * Getting around the 2GB limit of physiock by saving
			 * the starting address in the private pointer
			 */
			if ((op == SS_READ) || (op == SS_WRITE)) {
				struct scs *scs = (struct scs *) q->q_sc_cmd;
				bp->b_priv2.un_int =
                                           ((sdi_swap16(scs->ss_addr) & 0xffff)
                                           + (scs->ss_addr1 << 16));

			}
			else if ((op == SM_READ) || (op == SM_WRITE)) {
				struct scm *scm = (struct scm *) 
					((uchar_t *)(q->q_sc_cmd) - 2);
				bp->b_priv2.un_int =
					sdi_swap16(scm->sm_addr);
			}

			ha_uio.uio_segflg = UIO_USERSPACE;
			ha_uio.uio_fmode = 0;
			ha_uio.uio_resid = sp->SCB.sc_datasz;

			if (uerror = physiock(zl5380_pass_thru0, bp, dev, rw, 
			    HBA_MAX_PHYSIOCK, &ha_uio)) {
				goto done;
			}
		} else {
			bp->b_un.b_addr = NULL;
			bp->b_bcount = 0;
			bp->b_blkno = 0;
			bp->b_edev = dev;
			bp->b_flags |= rw;

			zl5380_pass_thru(bp);  /* Bypass physiock call */
			biowait(bp);
		}

		/*
		 * update user SCB fields
		 */

		karg.SCB.sc_comp_code = sp->SCB.sc_comp_code;
		karg.SCB.sc_status = sp->SCB.sc_status;
		karg.SCB.sc_time = sp->SCB.sc_time;

		if (copyout((caddr_t)&karg, arg, sizeof(struct sb)))
			uerror = EFAULT;

	   done:
		freerbuf(bp);
		sdi_freeblk(sp);
		break;
		}

	case B_GETTYPE:
		if(copyout("scsi", ((struct bus_type *)arg)->bus_name, 5)) {
			return(EFAULT);
		}
		if(copyout("scsi", ((struct bus_type *)arg)->drv_name, 5)) {
			return(EFAULT);
		}
		break;

	case	HA_VER:
		if(copyout((caddr_t)&zl5380_sdi_ver,arg,
			sizeof(struct ver_no))) {
			return(EFAULT);
		}
		break;
	case SDI_BRESET: {
		register zl5380ha_t *zl5380ha = zl5380scha[c];

		ZL5380_HBA_LOCK(oip);
		/*
		 * If a job has been delivered to him module
		 * it would be in the eligiblescb list till arbitration
		 * after that it would be stored in currentscb
		 * during the disconnected phase it would be present in 
		 * lucb
		 */
		if (zl5380ha->ha_eligiblescb || zl5380ha->ha_currentscb || 
		    zl5380ha->ha_disconnects) {     
		        /* jobs are outstanding */
			ZL5380_HBA_UNLOCK(oip);
			return(EBUSY);
		} else {
			cmn_err(CE_WARN, "%s: - Bus is being reset",
				zl5380ha->ha_name);
			zl5380reset (zl5380ha);
		}
		ZL5380_HBA_UNLOCK(oip);
		break;
		}

	default:
		return(EINVAL);
	}
	return(uerror);
}

/*
 * STATIC void
 * zl5380_pass_thru0(buf_t *bp)
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Description:
 *	Calls buf_breakup to make sure the job is correctly broken up/copied.
 *
 * Remarks:
 */
STATIC void
zl5380_pass_thru0(buf_t *bp)
{
	int	c = zl5380_gtol[SC_HAN(bp->b_edev)];
	int	t = SC_TCN(bp->b_edev);
	int	l = SC_LUN(bp->b_edev);

	struct zl5380scsi_lu	*q = &LU_Q(c, t, l);

	if (!q->q_bcbp) {
		struct sb *sp;
		struct scsi_ad *sa;

		sp = bp->b_priv.un_ptr;
		sa = &sp->SCB.sc_dev;
		if ((q->q_bcbp = sdi_getbcb(sa, KM_SLEEP)) == NULL) {
			sp->SCB.sc_comp_code = (unsigned long)SDI_ABORT;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return;
		}
		q->q_bcbp->bcb_granularity = 1;
		q->q_bcbp->bcb_addrtypes = BA_KVIRT;
		q->q_bcbp->bcb_flags = BCB_SYNCHRONOUS;
		q->q_bcbp->bcb_max_xfer = zl5380hba_info.max_xfer;
		q->q_bcbp->bcb_physreqp->phys_align = ZL5380_MEMALIGN;
		q->q_bcbp->bcb_physreqp->phys_boundary = ZL5380_BOUNDARY;
		q->q_bcbp->bcb_physreqp->phys_dmasize = ZL5380_DMASIZE;
	}

	buf_breakup(zl5380_pass_thru, bp, q->q_bcbp);
}

/*
 * STATIC void
 * zl5380_pass_thru(buf_t *bp)
 *
 * Calling/Exit status:
 *	No locks are held on entry.
 *	ZL5380_SCSILU_LOCK (q) is held on exit.
 *
 * Description:
 *	Send a pass-thru job to the HA board.
 *
 * Remarks:
 */
STATIC void
zl5380_pass_thru(buf_t *bp)
{
	int	c = zl5380_gtol[SC_HAN(bp->b_edev)];
	int	t = SC_TCN(bp->b_edev);
	int	l = SC_LUN(bp->b_edev);
	register struct zl5380scsi_lu    	*q;
	register struct sb 		*sp;
	uchar_t op;
	int	blk_no_adjust;

	sp = (struct sb *)bp->b_private;

	sp->SCB.sc_wd 		= (long)bp;
	sp->SCB.sc_datapt 	= (caddr_t) paddr(bp);
	sp->SCB.sc_datasz 	= bp->b_bcount;
	sp->SCB.sc_int 		= zl5380_int;

	SDI_TRANSLATE(sp, bp->b_flags, bp->b_proc, KM_SLEEP);

	q = &LU_Q(c, t, l);

	op = q->q_sc_cmd[0];
	blk_no_adjust = bp->b_blkno;
	if ((op == SS_READ) || (op == SS_WRITE)) {
		struct scs *scs = (struct scs *)q->q_sc_cmd;
		daddr_t	   blkno = bp->b_blkno;

		if (blk_no_adjust) {
			blkno += bp->b_priv2.un_int;
			scs->ss_addr1 = (blkno & 0x1F0000) >> 16;
			scs->ss_addr = sdi_swap16(blkno);
		}
		scs->ss_len = bp->b_bcount >> ZL5380_BLKSHFT;
	}
	else if ((op == SM_READ) || (op == SM_WRITE)) {
		struct scm *scm = (struct scm *)((uchar_t *)q->q_sc_cmd - 2);

		if (blk_no_adjust) {
			scm->sm_addr = 	
				sdi_swap16(bp->b_blkno + bp->b_priv2.un_int);
		}
		scm->sm_len = bp->b_bcount >> ZL5380_BLKSHFT;
	}
	sdi_icmd(sp, KM_SLEEP);
}



/*
 * STATIC void
 * zl5380_int(struct sb *sp)
 *
 * Calling/Exit status:
 *	No locks are held on entry.
 *
 * Description:
 *	This is the interrupt handler for pass-thru jobs. It just
 *	wakes up the sleeping process.
 *
 * Remarks:
 */
STATIC void
zl5380_int(struct sb *sp)
{
	buf_t  *bp;

	bp = (buf_t *)sp->SCB.sc_wd;
	biodone(bp);
}

/*ARGSUSED*/
/*
 * long
 * zl5380icmd(struct  hbadata *hbap, int sleepflag)
 *
 *	No locks are held on entry.
 *	ZL5380_SCSILU_LOCK (q) is held on exit.
 *
 * Description:
 *	Send an immediate command.  If the logical unit is busy, the job
 *	will be queued until the unit is free.  SFB operations will take
 *	priority over SCB operations.
 *
 * Remarks:
 */
long
HBAICMD(struct  hbadata *hbap, int sleepflag)
{
	register struct scsi_ad 	*sa;
	register struct zl5380scsi_lu 	*q;
	register sblk_t 		*sp = (sblk_t *) hbap;
	register int			c;
	register int			t;
	register int			l;
	struct ident			*inq_data;
	struct scs			*inq_cdb;

	switch (sp->sbp->sb.sb_type) {
	case SFB_TYPE:
		sa = &sp->sbp->sb.SFB.sf_dev;
		c = zl5380_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &LU_Q(c, t, l);

		if (l != 0) {
			sp->sbp->sb.SFB.sf_comp_code =(unsigned long)SDI_NOTEQ;
			sdi_callback(&sp->sbp->sb);
			return (SDI_RET_OK);
		}
		sp->sbp->sb.SFB.sf_comp_code = (unsigned long)SDI_ASW;

		switch (sp->sbp->sb.SFB.sf_func) {
		case SFB_RESUME:
			ZL5380_SCSILU_LOCK (q);
			q->q_flag &= ~QSUSP;
			zl5380_next (q);
			break;
		case SFB_SUSPEND:
			ZL5380_SET_QFLAG(q, QSUSP);
			break;
		case SFB_RESETM:
		case SFB_ABORTM:
			sp->sbp->sb.SFB.sf_comp_code = SDI_PROGRES;
			ZL5380_SCSILU_LOCK(q);
			zl5380_putq(q, sp);
			zl5380_next(q);
			return (SDI_RET_OK);
		case SFB_FLUSHR:
			zl5380_flushq(q, SDI_QFLUSH, 0);
			break;
		case SFB_NOPF:
			break;
		default:
			sp->sbp->sb.SFB.sf_comp_code =
			    (unsigned long)SDI_SFBERR;
			break;
		}

		sdi_callback(&sp->sbp->sb);
		return (SDI_RET_OK);

	case ISCB_TYPE:

		sa = &sp->sbp->sb.SCB.sc_dev;
		c = zl5380_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);

		if (l != 0) {
			sp->sbp->sb.SFB.sf_comp_code =(unsigned long)SDI_NOTEQ;
			sdi_callback(&sp->sbp->sb);
			return (SDI_RET_OK);
		}
		sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
		q = &LU_Q(c, t, sa->sa_lun);
		inq_cdb = (struct scs *)(void *)sp->sbp->sb.SCB.sc_cmdpt;

		if((t == zl5380scha[c]->ha_id) && (l == 0) &&
		  	(inq_cdb->ss_op == SS_INQUIR)) {
		        /*
			 * For the HBA itself
			 */
			inq_data = (struct ident *)(void *)
					sp->sbp->sb.SCB.sc_datapt;
			inq_data->id_type = ID_PROCESOR;
			(void)strncpy(inq_data->id_vendor,
					zl5380scha[c]->ha_name, 24);
			strcat (inq_data->id_vendor, 
				zl5380adapter_strs[zl5380scha[c]->ha_type]);
			inq_data->id_vendor[23] = NULL;
			sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
			sdi_callback(&sp->sbp->sb);
			return (SDI_RET_OK);
		}

		sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
		sp->sbp->sb.SCB.sc_status = 0;

		ZL5380_SCSILU_LOCK(q);
		zl5380_putq(q, sp);
		zl5380_next(q);
		return (SDI_RET_OK);

	default:
		sdi_callback(&sp->sbp->sb);
		return (SDI_RET_ERR);
	}
}

/*ARGSUSED*/
/*
 * long
 * zl5380send(struct hbadata *hbap, int sleepflag)
 *
 * Calling/Exit status:
 *	No locks are held on entry or exit.
 *
 * Description:
 * 	Send a SCSI command to a controller.  Commands sent via this
 *	function are executed in the order they are received.
 *
 * Remarks:
 */
long
HBASEND(struct hbadata *hbap, int sleepflag)
{
	register struct scsi_ad     *sa;
	register struct zl5380scsi_lu *q;
	register sblk_t *sp = (sblk_t *) hbap;

	sa = &sp->sbp->sb.SCB.sc_dev;

	if (sp->sbp->sb.sb_type != SCB_TYPE) {
		return (SDI_RET_ERR);
	}

	q = &LU_Q(zl5380_gtol[SDI_HAN(sa)], SDI_TCN(sa), SDI_LUN(sa));

	ZL5380_SCSILU_LOCK(q);
	if (q->q_flag & QPTHRU) {
		ZL5380_SCSILU_UNLOCK(q);
		return (SDI_RET_RETRY);
	}
	sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
	sp->sbp->sb.SCB.sc_status = 0;

	zl5380_putq(q, sp);
	zl5380_next(q);		/* ZL5380_SCSILU_LOCK(q) is released here */
	return (SDI_RET_OK);
}

/*
 * long
 * zl5380freeblk(struct hbadata *hbap)
 *
 * Calling/Exit status:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Release previously allocated SB structure.
 *	A nonzero return indicates an error in pointer or type field.
 *
 * Remarks:
 */
long
HBAFREEBLK(struct hbadata *hbap)
{
	sdi_free(&sm_poolhead, (jpool_t *)hbap);
	return (SDI_RET_OK);
}


/*
 * struct hbadata *
 * zl5380getblk(int sleepflag)
 *
 * Calling/Exit status:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Allocate a SB structure for the caller.  The function will
 *	sleep if there are no SCSI blocks available.
 *
 * Remarks:
 */
struct hbadata *
HBAGETBLK(int sleepflag)
{
	return ((struct hbadata *) SDI_GET(&sm_poolhead, sleepflag));
}



/*
 * void
 * zl5380getinfo(struct scsi_ad *sa, struct hbagetinfo *getinfop)
 *
 * Calling/Exit status:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Return the name, etc. of the given device.  The name is copied
 *	into a string pointed to by the first field of the getinfo
 *	structure.
 *
 * Remarks:
 */
void
HBAGETINFO(struct scsi_ad *sa, struct hbagetinfo *getinfop)
{
	register char  *s1, *s2;
	static char temp[] = "HA X TC X";

	s1 = temp;
	s2 = getinfop->name;
	temp[3] = SDI_HAN(sa) + '0';
	temp[8] = SDI_TCN(sa) + '0';
	while ((*s2++ = *s1++) != '\0')
		;

	getinfop->iotype = F_PIO;
	if (getinfop->bcbp) {
		getinfop->bcbp->bcb_addrtypes = BA_KVIRT;
		getinfop->bcbp->bcb_flags = 0;
		getinfop->bcbp->bcb_max_xfer = zl5380hba_info.max_xfer;
		getinfop->bcbp->bcb_physreqp->phys_align = ZL5380_MEMALIGN;
		getinfop->bcbp->bcb_physreqp->phys_boundary = ZL5380_BOUNDARY;
		getinfop->bcbp->bcb_physreqp->phys_dmasize = ZL5380_DMASIZE;
	}
}



/*ARGSUSED*/
/*
 * HBAXLAT_DECL
 * zl5380xlat(struct hbadata *hbap, int flag, struct proc *procp, int sleepflag)
 *
 * Calling/Exit status:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Perform the virtual to physical translation on the SCB
 *	data pointer.
 *
 * Remarks:
 */
HBAXLAT_DECL
HBAXLAT(struct hbadata *hbap, int flag, struct proc *procp, int sleepflag)
{
	register sblk_t *sp = (sblk_t *) hbap;

	if(sp->sbp->sb.SCB.sc_link) {
		/*
		 *+ Linked commands are not supported
		 */
		cmn_err(CE_WARN, "zl5380: Linked commands NOT available");
		sp->sbp->sb.SCB.sc_link = NULL;
	}
	if(!sp->sbp->sb.SCB.sc_datapt) {
		sp->sbp->sb.SCB.sc_datasz = 0;
	}
	HBAXLAT_RETURN (SDI_RET_OK);
}

