#ident	"@(#)kern-pdi:io/target/st01/st01.c	1.88.9.7"
#ident	"$Header$"

/*	Copyright (c) 1988, 1989  Intel Corporation	*/
/*	All Rights Reserved	*/

/*      INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied under the terms of a license agreement   */
/*	or nondisclosure agreement with Intel Corporation and may not be   */
/*	copied or disclosed except in accordance with the terms of that    */
/*	agreement.							   */

/*
 *	SCSI Tape Target Driver.
 */

#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <svc/systm.h>
#include <fs/file.h>
#include <io/open.h>
#include <io/ioctl.h>
#include <util/debug.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <io/target/scsi.h>
#include <io/target/st01/st01.h>
#include <io/target/st01/tape.h>
#include <io/target/sdi/dynstructs.h>
#include <io/target/sdi/sdi.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi_hier.h>
#include <util/mod/moddefs.h>
#include <io/ddi.h>

#define	DRVNAME		"st01 - tape target driver"

#if PDI_VERSION >= PDI_UNIXWARE20
#define KMEM_ZALLOC(X,Y,Z) sdi_kmem_alloc_phys(X,Y,Z)
#define KMEM_FREE kmem_free

#else /* PDI_VERSION < PDI_UNIXWARE20 */
#define KMEM_ZALLOC(X,Y,Z) kmem_zalloc(X,Y)
#define KMEM_FREE kmem_free

#endif /* PDI_VERSION < PDI_UNIXWARE20 */

void	st01io(), st01freejob(), st01rinit(),
	st01_tp_unmark(), st01_tp_gc(), st01_tape_free();
int	st01cmd(), ST01ICMD(), st01config(), ST01SEND(), st01ioctl(),
	st01docmd(), st01rm_dev(), st01_find_tape(),
	st01_claim(), st01_tapeinit();
int	sdi_xsend(), sdi_xicmd();

STATIC void	st01initialisebcb( struct tape * );			/*LXXX*/
STATIC void	st01MungeRead( uio_t *, ulong_t );			/*LXXX*/
STATIC int	st01getcomp( struct tape *, ulong_t * );		/*LXXX*/
STATIC int	st01setcomp( struct tape *, ulong_t );			/*LXXX*/
STATIC int	st01sense(struct tape *, int);				/*LXXX*/
STATIC void	st01intrq1( struct sb * );				/*LXXX*/
STATIC void	st01dosense(struct tape *, struct sb *, void (*)());	/*LXXX*/
STATIC void	st01intrrc( struct sb * );				/*LXXX*/
STATIC void	st01intsc( struct sb * );				/*LXXX*/
STATIC void	st01retryread( struct tape * );				/*LXXX*/

struct owner	*sdi_unlink_target();
char	*st01_uinttostr();


/* Allocated in space.c */
extern	long	 	St01_cmajor;	/* Character major number   */
extern	int		St01_reserve;	/* Flag for reserving tape on open */
/* Work around for MET statistics deficiences: begin */
extern	int		St01_stats_enabled;/* Flag for turning on tape */
					   /* tape stats and locking   */
					   /* driver into memory       */
/* Work around for MET statistics deficiences: end */
extern  struct  head	lg_poolhead;

/*
 * Tape structure pointer
 * Only st01rinit() will insert entries into st01_tp[].
 * Only st01rm_dev(), st01_unload(), and st01_tp_gc() will delete entries from
 * st01_tp[].
 * st01rm_dev() will fail if the device is open.
 */
struct tape		*st01_tp[TP_MAX_TAPES] = { NULL };
#define ST01_TP_LOCK() do {						\
				   pl_t pl; 				\
			           pl = LOCK(st01_tp_mutex, pldisk);	\
			           if (st01_tp_lock == 0) {		\
				        st01_tp_lock = 1; 		\
				        UNLOCK(st01_tp_mutex, pl);	\
				        break;				\
			           }					\
			        UNLOCK(st01_tp_mutex, pl);		\
				delay(1);				\
			   } while (1)

#define ST01_TP_UNLOCK() do {						\
				pl_t pl; 				\
			        pl = LOCK(st01_tp_mutex, pldisk);	\
				st01_tp_lock = 0;			\
			        UNLOCK(st01_tp_mutex, pl);		\
			 } while (0)

STATIC lock_t	*st01_tp_mutex=NULL;
STATIC int	st01_tp_lock=0;

STATIC LKINFO_DECL( st01_tp_lkinfo, "IO:st01:st01_tp_lkinfo", 0);

/* Number of tapes configured, maintained strictly for debugging */
int 			st01_tapecnt;

STATIC  struct owner   *st01_ownerlist = NULL;	/* List of owner structs from sdi_doconfig */
STATIC	int	st01_dynamic = 0;

STATIC void st01resumeq();

int st01start();
void st01comp(), st01logerr();

STATIC	int	st01_load(), st01_unload();
MOD_DRV_WRAPPER(st01, st01_load, st01_unload, NULL, DRVNAME);
/*
 * st01_load() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 */
STATIC	int
st01_load()
{
	st01_dynamic = 1;
	if (st01start()) {
		sdi_clrconfig(st01_ownerlist, SDI_DISCLAIM|SDI_REMOVE, st01rinit);
		return(ENODEV);
	}
/* Work around for MET statistics deficiences: begin */
	if( St01_stats_enabled ) {
		int i;

		for (i = 0; i < TP_MAX_TAPES; i++) 
		{
			/*
			 * Allocate a t_stat structure if one has not been
			 * allocated for the tape drive.
			 */
			if ((st01_tp[i])&&(st01_tp[i]->t_stat == NULL))
			{
				char string[50];
				int j;
				if (! st01_tp[i]->t_addr.pdi_adr_version )
				{
					strcpy(string,"c");
					strcat(string,st01_uinttostr(
						SDI_CONTROL(st01_tp[i]->t_addr.sa_ct)));
					strcat(string,"b");
					strcat(string,st01_uinttostr(st01_tp[i]->t_addr.sa_bus));
					strcat(string,"t");
					strcat(string,st01_uinttostr(st01_tp[i]->t_addr.sa_exta));
					strcat(string,"l");
					strcat(string,st01_uinttostr(st01_tp[i]->t_addr.sa_lun));
				}
				else
				{
					strcpy(string,"c");
					strcat(string,st01_uinttostr(st01_tp[i]->t_addr.extended_address->scsi_adr.scsi_ctl));
					strcat(string,"b");
					strcat(string,st01_uinttostr(st01_tp[i]->t_addr.extended_address->scsi_adr.scsi_bus));
					strcat(string,"t");
					strcat(string,st01_uinttostr(st01_tp[i]->t_addr.extended_address->scsi_adr.scsi_target));
					strcat(string,"l");
					strcat(string,st01_uinttostr(st01_tp[i]->t_addr.extended_address->scsi_adr.scsi_lun));
				}

				/* Pad out string to the full size of the name */
				for (j=strlen(string);j<MET_DS_NAME_SZ;j++) 
					string[j]=' ';
				string[MET_DS_NAME_SZ]='\0';

				/* Register the stat */
				st01_tp[i]->t_stat  = met_ds_alloc_stats(string, 0, 0, 
					(uint)(MET_DS_BLK|MET_DS_NO_RESP_HIST|MET_DS_NO_ACCESS_HIST));
			}
		}
	}
/* Work around for MET statistics deficiences: end */


	return(0);
}

/*
 * st01_unload() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 */
STATIC	int
st01_unload()
{
	int i;
	struct tape *tp;

/* Work around for MET statistics deficiences: begin */
	/*
	 * If we have statistics enabled, then stop the unload to work around a
	 * MET deficience requiring kernel and user level statistic
	 * registration information to be always in sync.
	 */
	if (St01_stats_enabled)
		return(EBUSY);
/* Work around for MET statistics deficiences: end */

	ST01_TP_LOCK();

	/*
	 * De-register st01rm_dev(), disclaim all tape devices from EDT, and
	 * remove pointer to st01rinit() from sdi_rinits[], but preserve other
	 * information in EDT across driver load-cycle.  In this way,
	 * st01rinit() may use the preserved EDT information to re-create the
	 * st01_tp[] tape structure array.
	 */
	sdi_target_hotregister(NULL, st01_ownerlist);
	sdi_clrconfig(st01_ownerlist, SDI_DISCLAIM, st01rinit);
	for (i = 0; i < TP_MAX_TAPES; i++) {
		if ((tp = st01_tp[i]) == NULL)
			continue;
		st01_tp[i] = NULL;
		st01_tape_free(tp);
	}
	st01_ownerlist = NULL;

	ST01_TP_UNLOCK();
	LOCK_DEALLOC(st01_tp_mutex);
	return(0);
}

/* Aliases - see scsi.h */
#define ss_code		ss_addr1
#define ss_mode		ss_addr1
#define ss_parm		ss_len
#define ss_len1		ss_addr
#define ss_len2		ss_len
#define ss_cnt1		ss_addr
#define ss_cnt2		ss_len

#define	GROUP0		0
#define	GROUP1		1
#define	GROUP6		6
#define	GROUP7		7

#define	msbyte(x)	(((x) >> 16) & 0xff)
#define	mdbyte(x)	(((x) >>  8) & 0xff)
#define	lsbyte(x)	((x) & 0xff)

#define SPL     	spldisk

void	st01intn(), st01intf(), st01intrq(), st01strategy(), st01strat0();
void	st01breakup();
int	st01reserve();
int	st01release();

#ifndef PDI_SVR42
int	st01devflag = D_TAPE | D_BLKOFF;
#else /* PDI_SVR42 */
int	st01devflag = D_TAPE;
#endif /* PDI_SVR42 */

/*
 * st01start() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Called by kernel to perform driver initialization.
 *	This function does not access any devices.
 */
int
st01start()
{
	int sleepflag;

#ifdef DEBUG
	cmn_err(CE_CONT, "\n\t\tST01 DEBUG DRIVER INSTALLED\n");
#endif

	st01_tapecnt = 0;
	sleepflag = st01_dynamic ? KM_SLEEP : KM_NOSLEEP;
        st01_tp_mutex = LOCK_ALLOC(TARGET_HIER_BASE+1, pldisk,
				&st01_tp_lkinfo, sleepflag);

        if (!st01_tp_mutex)
        {
		LOCK_DEALLOC(st01_tp_mutex);
                /*
		 *+ There was insufficient memory for tape data structures
		 *+ at boot-time.  This probably means that the system is
		 *+ configured with too little memory.
		 */
		cmn_err(CE_WARN,
			"Insufficient memory to configure tape driver.\n");
		return(1);
	}

	/*
	 * Do not acquire sdi_rinit_lock across st01rinit(), since
	 * sdi_doconfig() (called by st01rinit()) will acquire sdi_rinit_lock
	 * during dynamic loading of the driver (i.e. sdi_sleepflag == KM_SLEEP
	 * and sdi_in_rinit == 0), and no locking is required during static
	 * loading of the driver (i.e during system init/start time).
	 */
	st01rinit();
	return(0);
}

#ifndef PDI_SVR42
/*
 * int
 * st01devinfo(dev_t dev, di_parm_t parm, void **valp)
 *	Get device information.
 *	
 * Calling/Exit State:
 *	The device must already be open.
 */
int
st01devinfo(dev_t dev, di_parm_t parm, void **valp)
{
	struct tape	*tp;	/* Tape pointer */

	tp = TPPTR(geteminor(dev));
	if (tp == NULL)
		return(ENODEV);

	switch (parm) {
		case DI_BCBP:
			*(bcb_t **)valp = tp->t_bcbp;
			return 0;

		case DI_MEDIA:
			*(char **)valp = "tape";
			return 0;

		default:
			return ENOSYS;
	}
}
#endif

/*
 * void st01rinit() 
 *	Called by sdi to perform driver initialization of additional
 *	devices found after the dynamic load of HBA drivers.
 *	This function does not access any devices.
 *
 * Calling/Exit State:
 *	Caller must decide whether to held sdi_rinit_lock or not.
 */
void
st01rinit()
{
	struct drv_majors drv_maj;
	struct owner *op;

	int new_st01_tapecnt;		/* number of tapes found */

	ST01_TP_LOCK();

	drv_maj.b_maj = St01_cmajor;		/* st01 has no block major */
	drv_maj.c_maj = St01_cmajor;
	drv_maj.minors_per = TP_MINORS_PER;
	drv_maj.first_minor = TP_MAX_MINOR + 1;	/* minor# assignment pending */
	st01_ownerlist = sdi_doconfig(ST01_dev_cfg, ST01_dev_cfg_size,
				DRVNAME, &drv_maj, st01rinit);

	sdi_target_hotregister(st01rm_dev, st01_ownerlist);

	/* Unmark all the tape structures */
	st01_tp_unmark();

	new_st01_tapecnt = 0;

	/* Claim all existing tapes before claiming new tapes */
	for (op = st01_ownerlist; op; op = op->target_link)
	{
		if ((op->maj.first_minor <= TP_MAX_MINOR) &&
			(st01_claim(op) == 0))
		{
			new_st01_tapecnt++;
#ifdef ST01_DEBUG
			cmn_err(CE_CONT,"op 0x%x ", op);
			cmn_err(CE_CONT,"edt 0x%x ", op->edtp);
			cmn_err(CE_CONT,"hba %d scsi id %d lun %d bus %d\n",
			op->edtp->scsi_adr.scsi_ctl,
			op->edtp->scsi_adr.scsi_target,
			op->edtp->scsi_adr.scsi_lun,
			op->edtp->scsi_adr.scsi_bus);
#endif
		}
	}
	/* Claim all new tapes */
	for (op = st01_ownerlist; op; op = op->target_link)
	{
		if ((op->maj.first_minor > TP_MAX_MINOR) &&
			(st01_claim(op) == 0))
		{
			new_st01_tapecnt++;
#ifdef ST01_DEBUG
			cmn_err(CE_CONT,"op 0x%x ", op);
			cmn_err(CE_CONT,"edt 0x%x ", op->edtp);
			cmn_err(CE_CONT,"hba %d scsi id %d lun %d bus %d\n",
			op->edtp->scsi_adr.scsi_ctl,
			op->edtp->scsi_adr.scsi_target,
			op->edtp->scsi_adr.scsi_lun,
			op->edtp->scsi_adr.scsi_bus);
#endif
		}
	}
#ifdef ST01_DEBUG
        cmn_err(CE_CONT,"st01rinit %d tapes claimed, previously %d\n",
		new_st01_tapecnt, st01_tapecnt);
#endif
	st01_tapecnt = new_st01_tapecnt;

	/* free any unclaimed tapes */
	st01_tp_gc();

	ST01_TP_UNLOCK();
}

/*
 * int 
 * st01rm_dev()
 * the function to support hot removal of tape drives.
 * This will spin down the tape and remove it from its internal
 * structures, st01_tp.
 *
 * returns SDI_RET_ERR or SDI_RET_OK
 * will fail if the device doesn't exist, is not owned by st01,
 * is open.
 */
int
st01rm_dev(struct scsi_adr *sa)
{
	struct sdi_edt *edtp;
	struct tape *tp;
	int index;

	ASSERT(sa);
	edtp = sdi_rxedt(sa->scsi_ctl, sa->scsi_bus,
			 sa->scsi_target, sa->scsi_lun);
	
	if ((edtp == NULL) ||	/* no device */
	    (edtp->curdrv == NULL) || /* no owner */
	    (edtp->curdrv->maj.c_maj != St01_cmajor)) /* not mine */
		return SDI_RET_ERR;

	ST01_TP_LOCK();

	index = TPINDEX(edtp->curdrv->maj.first_minor);
	tp = st01_tp[index];

	if (tp == NULL)
	{
		ST01_TP_UNLOCK();
		return SDI_RET_ERR;
	}

	/* Check if the tape is open */
	if (tp->t_state & T_OPENED) {
		ST01_TP_UNLOCK();
		return SDI_RET_ERR;
	}

	/*
	 * For drivers that unload it is necessary to create the list
	 * of devices using information returned by sdi_doconfig() whenever the
	 * driver is loaded.  When a device is added or removed from the
	 * system it is necessary to add/remove the device from the list.
	 */
	st01_ownerlist = sdi_unlink_target(st01_ownerlist, edtp->curdrv);
	sdi_clrconfig(edtp->curdrv, SDI_REMOVE|SDI_DISCLAIM, NULL);

	st01_tp[index]=NULL;
	st01_tape_free(tp);

	ST01_TP_UNLOCK();
	return SDI_RET_OK;
	
}

/*
 * Unmarks all the tapes so that garbage collection can
 * take place later
 */
void
st01_tp_unmark(void)
{
	int index;

	for (index = 0; index < TP_MAX_TAPES; index++) {
		if (st01_tp[index] == NULL)
			continue;
		st01_tp[index]->t_marked = 0;
		
	}
}

/*
 * Free any tape structures that aren't marked
 */
void 
st01_tp_gc(void)
{
	int index;

	for (index = 0; index < TP_MAX_TAPES; index++) {
		if (st01_tp[index] == NULL)
			continue;
		if (st01_tp[index]->t_marked)
			continue;

#ifdef ST01_DEBUG
		cmn_err(CE_CONT, "st01_tp_gc() freeing st01_tp[%d]=0x%x\n",
			index, st01_tp[index]);
#endif
		st01_tape_free(st01_tp[index]);
		st01_tp[index] = NULL;
	}
}

/*
 * Returns the index into the st01_tp[] associated with the passed 
 * owner pointer. Returns -1 if it can't be found.
 */
int 
st01_find_tape(struct owner *op)
{
	int index;
	struct tape *tp;

	if (op->maj.first_minor > TP_MAX_MINOR)
		return -1;
	index = TPINDEX(op->maj.first_minor);
	if (st01_tp[index] == NULL)
		return -1;
	if (! st01_tp[index]->t_addr.pdi_adr_version )
		ASSERT(SDI_ADDRCMP(&(st01_tp[index]->t_addr), &(op->edtp->scsi_adr)));
	else
		ASSERT(SDI_ADDR_MATCH(&(st01_tp[index]->t_addr.extended_address->scsi_adr), &(op->edtp->scsi_adr)));

	return index;
}

/*
 * Creates, initializes and marks a new tape entry.
 * If the entry already exists it is simply marked but this
 * is not an error.
 *
 * Returns: 0 success, -1 failure
 */
int 
st01_claim(struct owner *op)
{
	struct tape *tp;
	int index;
	int sleepflag;

	index = st01_find_tape(op);
	if (index != -1)
	{
		st01_tp[index]->t_marked = 1;
		return 0;
	}

	/* Determine the st01_tp slot to be used for claiming the tape */
	if (op->maj.first_minor > TP_MAX_MINOR) {
		for (index = 0; index < TP_MAX_TAPES; index++) {
			if (st01_tp[index] == NULL)
				break;
		}
	} else {
		index = TPINDEX(op->maj.first_minor);
	}
	
	if (index >= TP_MAX_TAPES)
	{
		
		/*
		 *+ All of the available spaces in st01_tp
		 *+ have been used.  One is needed for each
		 *+ tape.  Increase the size of TP_MAX_TAPES
		 *+ in /usr/include/sys/st01.h
		 */
		cmn_err(CE_WARN, "Tape Driver: Maximum tape "
			"configuration exceeded.");
		return -1;
	}

	sleepflag = st01_dynamic ? KM_SLEEP : KM_NOSLEEP;
	if ((tp = (struct tape *) kmem_zalloc(sizeof(struct tape),
					     sleepflag)) == NULL)
	{
		/*
		 *+ There was insuffcient memory for tape data structures.
		 *+ This probably means that the system is
		 *+ configured with too little memory. 
		 */
		cmn_err(CE_WARN, "Tape Driver: Insufficient memory to configure driver.");
		return -1;
	}

	if (!st01_tapeinit(tp, op))
	{
		return -1;
	}
	/* set major/minor #'s please */
	op->maj.b_maj = St01_cmajor;	/* st01 has no block major */
	op->maj.c_maj = St01_cmajor;
	op->maj.minors_per = TP_MINORS_PER;
	op->maj.first_minor = TPINDEX2MINOR(index);

	st01_tp[index] = tp;
	st01_tp[index]->t_marked = 1;

	return 0;
}

/*
 * int
 * st01_tapeinit(struct tape *tp, struct owner *op)
 *	Initialize tape structure
 */
int
st01_tapeinit(struct tape *tp, struct owner *op)
{
	int sleepflag, kmflag;
	size_t maxbiosize;

	kmflag = st01_dynamic ? 0 : KM_FAIL_OK;

	/* Allocate the fault SBs */
	sleepflag = st01_dynamic ? KM_SLEEP : KM_NOSLEEP;
	tp->t_fltreq = sdi_getblk(sleepflag);  /* Request sense */
	tp->t_fltres = sdi_getblk(sleepflag);  /* Resume */
	tp->t_fltrst = sdi_getblk(sleepflag);  /* Reset */
	tp->t_fltread= sdi_getblk(sleepflag);  /* Short-length read recovery */
#ifdef DEBUG
	cmn_err(CE_CONT, "tape: op 0x%x edt 0x%x ", op, op->edtp);
	cmn_err(CE_CONT, "hba %d scsi id %d lun %d bus %d\n",
		op->edtp->scsi_adr.scsi_ctl,op->edtp->scsi_adr.scsi_target,
		op->edtp->scsi_adr.scsi_lun,op->edtp->scsi_adr.scsi_bus);
#endif

	/*
         * Allocate a scsi_extended_adr structure
         */
        tp->t_addr.extended_address =
		(struct sdi_extended_adr *)kmem_zalloc(sizeof(struct sdi_extended_adr), sdi_sleepflag);

        if ( ! tp->t_addr.extended_address ) {
                cmn_err(CE_WARN, "Tape driver: insufficient memory"
                                        "to allocate extended address");
                return(0);
        }

	tp->t_addr.pdi_adr_version = 
		sdi_ext_address(op->edtp->scsi_adr.scsi_ctl);


	if (! tp->t_addr.pdi_adr_version ) {
		tp->t_addr.sa_ct  = SDI_SA_CT(op->edtp->scsi_adr.scsi_ctl, 
			      	op->edtp->scsi_adr.scsi_target);
		tp->t_addr.sa_exta= (uchar_t)(op->edtp->scsi_adr.scsi_target);
		tp->t_addr.sa_lun = op->edtp->scsi_adr.scsi_lun;
		tp->t_addr.sa_bus = op->edtp->scsi_adr.scsi_bus;
	}
	else		
		tp->t_addr.extended_address->scsi_adr =
			op->edtp->scsi_adr;

	tp->t_addr.extended_address->version = SET_IN_TARGET;
	tp->t_spec = sdi_findspec(op->edtp, st01_dev_spec);
	tp->t_iotype = op->edtp->iotype;

	/*
	 * Call to initialize the breakup control block.
	 */
	if ((tp->t_bcbp = sdi_xgetbcb(tp->t_addr.pdi_adr_version,
			&tp->t_addr, sleepflag)) == NULL) {
		/*
		 *+ Insufficient memory to allocate tape breakup control
		 *+ block data structure when loading driver.
		 */
 		cmn_err(CE_NOTE, "Tape Driver: insufficient memory to allocate breakup control block.");
		if (tp->t_fltreq)
			sdi_xfreeblk(tp->t_fltreq->SCB.sc_dev.pdi_adr_version, tp->t_fltreq);  /* Request sense */
		if (tp->t_fltres)
			sdi_xfreeblk(tp->t_fltres->SCB.sc_dev.pdi_adr_version, tp->t_fltres);  /* Resume */
		if (tp->t_fltrst)
			sdi_xfreeblk(tp->t_fltrst->SCB.sc_dev.pdi_adr_version, tp->t_fltrst);  /* Reset */
		if (tp->t_fltread)
			sdi_xfreeblk(tp->t_fltread->SCB.sc_dev.pdi_adr_version, tp->t_fltread); /* Short length recovery */
		return (0);
	}
	/*
	 * LXXX: There is a potential problem caused by user requests not
	 *	 having to be page-aligned. If we simply make the physreqp
	 *	 force page alignment, the user read/write requests will be
	 *	 failed when buf_breakup() is called. To avoid this (common)
	 *	 case, we allocate a separate bcb which _is_ aligned and then
	 *	 we decouple the user i/o requests by copy the data to/from
	 *	 this aligned buffer and using it as the source/detination
	 *	 for buf_breakup() calls.
	 *	 The most common aspect of this is trying to write 64k blocks
	 *	 in variable block mode.
	 */
	if ((tp->t_abcbp = bcb_alloc( sleepflag )) ) {
		/* 
		 * Now get the physreq to use with this bcb
		 */
		if ( (tp->t_abcbp->bcb_physreqp = physreq_alloc(sleepflag)) ==
			NULL ) {
			/*
			 * No memory for physreq - release bcb too
			 */
			bcb_free( tp->t_abcbp );
		} else {
			/*
			 * Now allocate the buffer to be page aligned and as
			 * large as the underlying HBA and the rest of kernel
			 * can handle in one I/O transfer.
			 */
			tp->t_abcbp->bcb_addrtypes = tp->t_bcbp->bcb_addrtypes;
			tp->t_abcbp->bcb_flags     = tp->t_bcbp->bcb_flags;
			tp->t_abcbp->bcb_flags    |= BCB_ONE_PIECE|BCB_SYNCHRONOUS;
			tp->t_abcbp->bcb_flags    &= ~BCB_EXACT_SIZE;
			tp->t_abcbp->bcb_granularity = 1;
			if ( ! tp->t_addr.pdi_adr_version )
				tp->t_abcbp->bcb_max_xfer =
				HIP(HBA_tbl[SDI_EXHAN(&tp->t_addr)].info)->max_xfer;
			else
				tp->t_abcbp->bcb_max_xfer =
				HIP(HBA_tbl[SDI_HAN_32(&tp->t_addr)].info)->max_xfer;
			drv_getparm(DRV_MAXBIOSIZE, (size_t *) &maxbiosize);
			if (tp->t_abcbp->bcb_max_xfer > maxbiosize)
				tp->t_abcbp->bcb_max_xfer = maxbiosize;
			tp->t_abcbp->bcb_physreqp->phys_align = ptob(1);
			tp->t_abcbp->bcb_physreqp->phys_boundary =
				tp->t_bcbp->bcb_physreqp->phys_boundary;
			tp->t_abcbp->bcb_physreqp->phys_dmasize =
				tp->t_bcbp->bcb_physreqp->phys_dmasize;
			tp->t_abcbp->bcb_physreqp->phys_max_scgth =
				tp->t_bcbp->bcb_physreqp->phys_max_scgth;
			if ( physreq_prep(tp->t_abcbp->bcb_physreqp,sleepflag)){
			    tp->t_abuff = KMEM_ZALLOC(tp->t_abcbp->bcb_max_xfer,
				tp->t_abcbp->bcb_physreqp, KM_FAIL_OK);
			}
			if ( tp->t_abuff == NULL ) {
				physreq_free(tp->t_abcbp->bcb_physreqp);
				bcb_free(tp->t_abcbp);
			} else {
				tp->t_uiop = kmem_zalloc(sizeof(uio_t),
							 sleepflag);
				if ( tp->t_uiop == NULL ) {
					KMEM_FREE(tp->t_abuff, 
						tp->t_abcbp->bcb_max_xfer +
						SDI_PADDR_SIZE(tp->t_abcbp->bcb_physreqp));
					physreq_free(tp->t_abcbp->bcb_physreqp);
					bcb_free(tp->t_abcbp);
					tp->t_abuff = NULL;
				} else {
					tp->t_iovec.iov_base = tp->t_abuff;
					tp->t_uiop->uio_iovcnt = 1;
					tp->t_uiop->uio_segflg=UIO_SYSSPACE;
					tp->t_uiop->uio_iov = &tp->t_iovec;
				}
			}
		}
	}
			
#ifndef PDI_SVR42
	tp->t_bcbp->bcb_flags |= BCB_SYNCHRONOUS;
#endif
	tp->t_sense = (struct sense *) KMEM_ZALLOC(sizeof(struct sense),
			tp->t_bcbp->bcb_physreqp, kmflag);
	tp->t_mode = (struct mode *) KMEM_ZALLOC(sizeof(struct mode),
			tp->t_bcbp->bcb_physreqp, kmflag);
	tp->t_blklen = (struct blklen *) KMEM_ZALLOC(RDBLKLEN_SZ,
			tp->t_bcbp->bcb_physreqp, kmflag);
	tp->t_ident = (struct ident *) KMEM_ZALLOC(sizeof(struct ident),
			tp->t_bcbp->bcb_physreqp, kmflag);
	tp->t_dcc = (struct mode_dcc_page *) 				/*LXXX*/
			KMEM_ZALLOC(sizeof(struct mode_dcc_page), 	/*LXXX*/
			tp->t_bcbp->bcb_physreqp, kmflag);		/*LXXX*/

/* Work around for MET statistics deficiences: begin */
	tp->t_stat=NULL;
/* Work around for MET statistics deficiences: end */
	return (1);
}

/*
 *  Frees an existing tape structure and associated structures.
 */
void
st01_tape_free(struct tape *tp)
{
	ulong paddr_size = SDI_PADDR_SIZE(tp->t_bcbp->bcb_physreqp);

	KMEM_FREE(tp->t_sense, sizeof(struct sense)+paddr_size);
	KMEM_FREE(tp->t_mode, sizeof(struct mode)+paddr_size);
	KMEM_FREE(tp->t_blklen, RDBLKLEN_SZ+paddr_size);
	KMEM_FREE(tp->t_ident, sizeof(struct ident)+paddr_size);
	if (tp->t_dcc) {
		KMEM_FREE(tp->t_dcc, sizeof(struct mode_dcc_page)+paddr_size);
	}
	/*
	 * LXXX: Free up any aligned buffer space which was allocated
	 *	 flag the unit as having no aligned buffer space
	 */
	if (tp->t_abuff) {
		/* No extra paddr field is allocated with tp->t_uiop */
		KMEM_FREE(tp->t_uiop, sizeof(uio_t));
		KMEM_FREE(tp->t_abuff, tp->t_abcbp->bcb_max_xfer +
			SDI_PADDR_SIZE(tp->t_abcbp->bcb_physreqp));
		physreq_free(tp->t_abcbp->bcb_physreqp);
		bcb_free(tp->t_abcbp);
	}
	sdi_freebcb(tp->t_bcbp);

/* Work around for MET statistics deficiences: begin */
	/*
	 * Although the MET work around will keep st01_unload() from calling
	 * st01_tape_free() when tape statistics is enabled, st01rm_dev() will
	 * still call st01_tape_free().  Thus, need to de-allocate tape
	 * statistics for completeness.
	 */
	if (tp->t_stat)
		met_ds_dealloc_stats(tp->t_stat);
/* Work around for MET statistics deficiences: end */

	kmem_free(tp , sizeof(struct tape));
}

/*
 * st01getjob() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function will allocate a tape job structure from the free
 *	list.  The function will sleep if there are no jobs available.
 *	It will then get a SCSI block from SDI.
 */
struct job *
st01getjob()
{
	register struct job *jp;

	jp = (struct job *)sdi_get(&lg_poolhead, 0);

	/* Get an SB for this job */
	jp->j_sb = sdi_getblk(KM_SLEEP);
	return(jp);
}

/*
 * st01freejob() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function returns a job structure to the free list. The
 *	SCSI block associated with the job is returned to SDI.
 */
void
st01freejob(jp)
register struct job *jp;
{
	sdi_xfreeblk(jp->j_sb->SCB.sc_dev.pdi_adr_version, jp->j_sb);
	sdi_free(&lg_poolhead, (jpool_t *)jp);
}

#define ST01IOCTL(cmd, arg) st01ioctl(dev, cmd, (caddr_t) arg, oflag, cred_p, (int*)0)

/*
 * abort_open()
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Clean up flags, so a failed open call can be aborted.
 *	Clears T_OPENING flag, and wakes up anyone sleeping
 *	for the current unit.
 */
void
abort_open(tp)
struct tape *tp;
{
    (void) st01release(tp);
    tp->t_state &= ~T_OPENING;
}

/*
 * st01open() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 * 	Driver open() entry point.  Opens the device and reserves
 *	it for use by that process only.
 */
/* ARGSUSED */
st01open(devp, oflag, otyp, cred_p)
dev_t	*devp;
cred_t	*cred_p;
int oflag, otyp;
{
	dev_t		dev = *devp;
	register struct tape *tp;
	struct	xsb	*xsb;
	int		error; /* save rval of subfunctions */
	struct mode	*mp;				/* LXXX */

	if (oflag & FAPPEND) {
		return(ENXIO);
	}

	ST01_TP_LOCK();

	tp = TPPTR(geteminor(dev));
	if (tp == NULL) {
		ST01_TP_UNLOCK();
		return(ENXIO);
	}

	/*
	 * Allow single open. If there is an outstanding aborted
	 * command, hold up the open unil the command terminates.
	 */
	tp->t_state |= T_OPENING;

	if (tp->t_state & T_OPENED) {
		tp->t_state &= ~T_OPENING;
		ST01_TP_UNLOCK();
		return(EBUSY);
	}

	tp->t_state &= (T_PARMS | T_OPENING | T_COMPRESSION | T_COMPRESSED);
		/* clear all bits except T_PARMS and T_OPENING */

	/* Initialize the fault SBs */
	tp->t_fltcnt = 0;
	tp->t_fltres->SFB.sf_dev    = tp->t_addr;
	tp->t_fltrst->SFB.sf_dev    = tp->t_addr;

	tp->t_fltreq->sb_type = ISCB_TYPE;
	tp->t_fltreq->SCB.sc_datapt = SENSE_AD(tp->t_sense);
	tp->t_fltreq->SCB.sc_datasz = SENSE_SZ;
	tp->t_fltreq->SCB.sc_mode   = SCB_READ;
	tp->t_fltreq->SCB.sc_cmdpt  = SCS_AD(&tp->t_fltcmd);
	tp->t_fltreq->SCB.sc_dev    = tp->t_addr;
	xsb = (struct xsb *)tp->t_fltreq;
	xsb->extra.sb_datapt =
		(paddr32_t *) ((char *)tp->t_sense + sizeof (struct sense));
        xsb->extra.sb_data_type = SDI_PHYS_ADDR;
	*(xsb->extra.sb_datapt) += 1;
	sdi_xtranslate(tp->t_fltreq->SCB.sc_dev.pdi_adr_version,
			tp->t_fltreq, B_READ, 0, KM_SLEEP);

	/*
	 * LXXX: Initialise the ILI short-length read SB. Note we hijack the
	 *	 command block (tp->t_fltcmd) when performing this recovery.
	 */
	if ( tp->t_abuff ) {
		tp->t_fltread->sb_type 		= ISCB_TYPE;
		tp->t_fltread->SCB.sc_datapt	= tp->t_abuff;
		tp->t_fltread->SCB.sc_datasz	= tp->t_abcbp->bcb_max_xfer;
		tp->t_fltread->SCB.sc_mode	= SCB_READ;
		tp->t_fltread->SCB.sc_cmdpt	= SCS_AD(&tp->t_fltcmd);
		tp->t_fltread->SCB.sc_cmdsz	= SCS_SZ;
		tp->t_fltread->SCB.sc_dev	= tp->t_addr;
		xsb = (struct xsb *)tp->t_fltread;
		xsb->extra.sb_datapt = (paddr32_t *)
			((char *)tp->t_sense + sizeof (struct sense));
        	xsb->extra.sb_data_type = SDI_PHYS_ADDR;
		*(xsb->extra.sb_datapt) += 1;
		sdi_xtranslate(tp->t_fltread->SCB.sc_dev.pdi_adr_version,
			tp->t_fltread, B_READ, 0, KM_SLEEP);
		tp->t_fltread->SCB.sc_wd	= (long)tp;
	}

	if (St01_reserve) {
		if (error=st01reserve(tp)) {
			abort_open(tp);
			ST01_TP_UNLOCK();
			return(error);
		}
	}

	if (error=st01config(tp)) { 
		abort_open(tp);
		ST01_TP_UNLOCK();
		return(error);
	}

	/***
	** Return an access error if tape is
	** write protected, and user wants to write.
	***/
	mp = (struct mode *)tp->t_mode;			/* LXXX */
	if (mp->md_wp && (oflag & FWRITE)) {		/* LXXX */
		tp->t_state &= ~T_PARMS;
		abort_open(tp);
		ST01_TP_UNLOCK();
		return(EACCES);
	}

	if (RETENSION_ON_OPEN(dev))
		ST01IOCTL(T_RETENSION, 0);
	tp->t_state |= T_OPENED;
	tp->t_state &= ~T_OPENING;
	ST01_TP_UNLOCK();
	return(0);
}

/*
 * st01close() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 * 	Driver close() entry point.  If the device has been opened
 *	for writing, a file mark will be written.  If the device
 *	has been opened to rewind on close, a rewind will be
 *	performed; otherwise the tape head will be positioned after
 *	the first filemark.
 */
/* ARGSUSED */
st01close(dev, oflag, otyp, cred_p)
cred_t	*cred_p;
register dev_t dev;
int oflag, otyp;
{
	register struct tape *tp;

	tp = TPPTR(geteminor(dev));
	ASSERT(tp);

	/*
	 * LXXX: We need to invalidate any stale data that has not been
	 *	 consumed from the in-kernel uio structure allocated for
	 *	 variable block devices. If we don't do this here, we can
	 *	 easily get unpredictable results when writing and then
	 *	 reading a variable-blocked tape
	 */
	if ( tp->t_uiop ) {
		bzero( (caddr_t)tp->t_uiop, sizeof(uio_t) );
	}

	if (NOREWIND(dev)) {
		/* move past next file mark */
		if ((tp->t_state & T_READ)
		&& !(tp->t_state & (T_FILEMARK | T_TAPEEND)))
			ST01IOCTL(T_SFF, 1);
		/* write filemark */
		if (tp->t_state & T_WRITTEN)
			ST01IOCTL(T_WRFILEM, 1);
	} else {
		/* rewind the tape */
		ST01IOCTL(T_RWD, 0);
		tp->t_state &= ~T_PARMS;
	}

	if (DOUNLOAD(dev)) {
		ST01IOCTL(T_UNLOAD,0);
		tp->t_state &= ~T_PARMS;
	}

	if (St01_reserve) {
		if (st01release(tp)) {
			ST01_TP_UNLOCK();
			return(ENXIO);
		}
	}

	/*
	 * Only need to protect the resetting of T_OPENED in st01close(), since
	 * st01rm_dev() will not remove tp as long as its T_OPENED flag is
	 * set.
	 */
	ST01_TP_LOCK();
	tp->t_state &= ~T_OPENED;
	ST01_TP_UNLOCK();
	return(0);
}

/*
 * void
 * st01strategy(buf_t *bp)
 *	Driver strategy entry point.
 *	st01strategy  determines the flow of control through
 *	restricted dma code, by checking the device's I/O capability,
 *	then sends the request on to st01strat0.
 * Called by: kernel
 * Side Effects: 
 *	DMA devices have data moved to dmaable memory when necessary.
 *
 * Calling/Exit State:
 *	None.
 */
void
st01strategy(buf_t *bp)
{
	register struct tape *tp;
#ifdef PDI_SVR42
	struct scsi_ad  *dev;
	struct sdi_devinfo info;
#endif
	size_t bsize;

	tp = TPPTR(geteminor(bp->b_edev));
	ASSERT(tp);

#ifdef PDI_SVR42
	dev = &tp->t_addr;
	info.strat = st01strat0;
	info.iotype = tp->t_iotype;
	if ( ! dev->pdi_adr_version )
		info.max_xfer =
                   (HIP(HBA_tbl[SDI_EXHAN(dev)].info)->max_xfer - ptob(1));
	else
		info.max_xfer =
                   (HIP(HBA_tbl[SDI_HAN_32(dev)].info)->max_xfer - ptob(1));

	info.granularity = bsize;
	sdi_breakup(bp, &info);
#else
	/*
	 * LXXX: If the transfer is larger than the maximum non-aligned size
	 *	 and the aligned bcb (tp->t_abcbp) is available, we will use
	 *	 the driver buffer instead of the user buffer.
	 */
	if (bp->b_bcount > tp->t_bcbp->bcb_max_xfer && tp->t_abcbp != NULL) {
		buf_breakup(st01strat0, bp, tp->t_abcbp);
	} else {
		buf_breakup(st01strat0, bp, tp->t_bcbp);
	}
#endif

        return;
}

/*
 * void
 * st01strat0(buf_t *bp)
 * Called by: sdi_breakup2
 * 	Initiate I/O to the device. This function only checks the 
 *	validity of the request.  Most of the work is done by st01io().
 * Side Effects: 
 *
 * Calling/Exit State:
 *	None.
 */
void
st01strat0(buf_t *bp)
{
	register struct tape *tp;

	tp = TPPTR(geteminor(bp->b_edev));
	ASSERT(tp);

	tp->t_state &= ~T_SHORT_READ;				/* LXXX */

	bp->b_resid = bp->b_bcount;

	if (tp->t_state & T_TAPEEND) {
		bp->b_flags |= B_ERROR;
		bp->b_error = ENOSPC;
#ifdef PDI_SVR42
		biodone(bp);
#else
		if( tp->t_state & T_ABORTED ) {
			tp->t_state &= ~T_ABORTED;
			freerbuf(bp);
		}       
		else {
			biodone(bp);
		}
#endif
		return;
	}

	if ((tp->t_state & T_FILEMARK) && (bp->b_flags & B_READ)) {
#ifdef PDI_SVR42
		biodone(bp);
#else
		if( tp->t_state & T_ABORTED ) {
			tp->t_state &= ~T_ABORTED;
			freerbuf(bp);
		}       
		else {
			biodone(bp);
		}
#endif
		return;
	}

	bp->b_resid = 0;

	st01io(tp, bp);
}

/*
 * st01read() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 * Description:
 * 	Driver read() entry point.  Performs a raw read from the
 *	device.  The function calls physio() which locks the user
 *	buffer into core.
 */
/* ARGSUSED */
st01read(dev, uio_p, cred_p)
uio_t	*uio_p;
cred_t	*cred_p;
register dev_t dev;
{
	int		cnt;
	ulong_t		xfer_req, max_xfer, amount;
	struct iovec	*iov;
	struct tape	*tp;
	ulong_t		xfer_count;
	int		rval;

	tp = TPPTR(geteminor(dev));
	ASSERT(tp);

	if ( ! tp->t_bsize ) {

		xfer_req = (ulong_t)0;
		for (cnt = 0, iov = uio_p->uio_iov; cnt++ < uio_p->uio_iovcnt; iov++) {
			xfer_req += (ulong_t)iov->iov_len;
		}

		/*
		 * The tape is in Variable Length Mode. We need to make sure that
		 * the requested transfer size is not greater than what can be
		 * handled by the HBA.
		 * uiophysio(), called out of physiock(), breaks requests into
		 * multiple bcb_max_xfer size transfers. 
		 * LXXX:
		 * To allow a best effort at returning data if the request is
		 * too large, we shrink the request to fit the HBA supplied
		 * maximum transfer. If the physically recorded data is larger
		 * than this amount, st01sense() will fail with ILI. There is
		 * currently no work-around for this problem.
		 */
		if ( tp->t_abuff == NULL ) {
			max_xfer = tp->t_bcbp->bcb_max_xfer;
		} else {
			max_xfer = tp->t_abcbp->bcb_max_xfer;
		}
		if ( xfer_req > max_xfer ) {
			st01MungeRead( uio_p, max_xfer );
			xfer_req = max_xfer;
		}

		/*
		 * We've now got the uio request limited to the maximum
		 * physical size the HBA supports. If there is any data in
		 * the aligned buffer we must fill the user request with it
		 * before we get more data from the tape
		 */
		
		if ( tp->t_abuff ) {
		   if (xfer_req > tp->t_bcbp->bcb_max_xfer) {
			/*
			 * First drain any data which is still present in the
			 * buffer (caused by physical block-size being larger
			 * than requested user block-size)
			 */
			if ( tp->t_uiop->uio_offset ) {
				caddr_t	bufaddr = tp->t_iovec.iov_base -
					tp->t_uiop->uio_offset;

				xfer_count = MIN(tp->t_uiop->uio_offset,
						xfer_req);
#if DEBUG
				cmn_err(CE_NOTE, 
				"st01read:Buffer drain %x,%x\n", bufaddr,
				xfer_count);
#endif

				if (uiomove(bufaddr,xfer_count,UIO_READ,uio_p)){
					return( EFAULT );
				}
				tp->t_uiop->uio_offset -= xfer_count;
				xfer_req -= xfer_count;
			}
			/* 
			 * If there is more data to be read to user space,
			 * set-up the kernel uiop if it's >> non-aligned size
			 * Otherwise, just let the uiophysck() handle the
			 * transfer
			 */
			if ( xfer_req == 0 ) {
				return( 0 );
			}
			if ( xfer_req > tp->t_bcbp->bcb_max_xfer ) {
				tp->t_uiop->uio_resid 	= xfer_req;
				tp->t_iovec.iov_base 	= tp->t_abuff;
				tp->t_iovec.iov_len  	= xfer_req;
				tp->t_uiop->uio_offset 	= 0;
				tp->t_uiop->uio_iov 	= &tp->t_iovec;
				tp->t_uiop->uio_iovcnt 	= 1;
				tp->t_uiop->uio_segflg  = UIO_SYSSPACE;
			} else
				goto normal_read;

			rval = physiock(st01strategy,0,dev,B_READ,0,tp->t_uiop);
			if ( rval ) {
				return(rval);
			}
			/*
			 * OK, we now have some data in the kernel buffer, so
			 * just copy it out to the user process and update our
			 * internal uio fields for the next read. Note that
			 * amount of data transferred = xfer_req - resid
			 */
			amount = xfer_req - tp->t_uiop->uio_resid;
			
			/*
			 * Check for end of record condition (0 bytes)
			 */
			if ( amount == 0 ) {
				return( 0 );
			}

#if DEBUG
			cmn_err(CE_NOTE, "st01read: uiomove(%x,%x)\n",
				tp->t_abuff, amount);
#endif
			if ( uiomove(tp->t_abuff, amount, UIO_READ, uio_p) ){
				tp->t_uiop->uio_offset = 0;
				return(EFAULT);
			}
			return(0);
		    } else {
			/*
			 * Drain any data from a previous read into the user
			 * request and then stage the new read if any data is
			 * left.
			 */
			if ( tp->t_uiop->uio_offset ) {
				caddr_t	bufaddr = tp->t_iovec.iov_base -
					tp->t_uiop->uio_offset;

				xfer_count = MIN(tp->t_uiop->uio_offset,
						xfer_req);
#if DEBUG
				cmn_err(CE_NOTE, 
				"st01read:Buffer drain %x,%x\n", bufaddr,
				xfer_count);
#endif

				if (uiomove(bufaddr,xfer_count,UIO_READ,uio_p)){
					return( EFAULT );
				}
				tp->t_uiop->uio_offset -= xfer_count;
				xfer_req -= xfer_count;
				if ( xfer_req == 0 ) {
					return(0);
				}
			}
		    }
		}
	}

normal_read:
	return (physiock(st01strategy, 0, dev, B_READ, 0, uio_p));
}

/*
 * st01write() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 * 	Driver write() entry point.  Performs a raw write to the
 *	device.  The function calls physio() which locks the user
 *	buffer into core.
 */
/* ARGSUSED */
st01write(dev, uio_p, cred_p)
uio_t	*uio_p;
cred_t	*cred_p;
register dev_t dev;
{
	int		cnt;
	ulong_t		xfer_req;
	struct iovec	*iov;
	struct tape	*tp;
	uio_t		*l_uio_p = uio_p;	/* LXXX */

	tp = TPPTR(geteminor(dev));
	ASSERT(tp);

	if ( ! tp->t_bsize ) {

		xfer_req = (ulong_t)0;
		for (cnt = 0, iov = uio_p->uio_iov; cnt++ < uio_p->uio_iovcnt; iov++) {
			xfer_req += (ulong_t)iov->iov_len;
		}

		/*
		 * The tape is in Variable Length Mode. We need to make sure that
		 * the requested transfer size is not greater than what can be
		 * handled by the HBA.
		 * uiophysio(), called out of physiock(), breaks requests into
		 * multiple bcb_max_xfer size transfers. Since we cannot break up
		 * requests in Variable Length Mode, fail the reqeust.
		 *
		 * LXXX: If the request is too large for the non-aligned case
		 *	 we try using the guaranteed page-aligned buffer as
		 *	 the source for the data. This means we copy the data
		 *	 from user-space and then try sending it to the HBA
		 */

		if ( xfer_req > tp->t_bcbp->bcb_max_xfer  ) {
			if ( tp->t_abuff == NULL ) {
				return(EINVAL);
			}
			if ( xfer_req > tp->t_abcbp->bcb_max_xfer ) {
				return(EINVAL);
			}
			/*
		 	 * Copy the user supplied data into our kernel buffer
		 	 * (t_abuff) and fix up a kernel uio to use for the
		 	 * physiock call
		 	 */
			if ( uiomove(tp->t_abuff, xfer_req, UIO_WRITE, uio_p)) {
				return(EFAULT);
			}
			tp->t_uiop->uio_segflg		= UIO_SYSSPACE;
			tp->t_uiop->uio_iovcnt		= 1;
			tp->t_uiop->uio_offset 		= 0;
			tp->t_uiop->uio_resid  		= xfer_req;
			tp->t_uiop->uio_iov		= &tp->t_iovec;
			tp->t_uiop->uio_iov->iov_len 	= xfer_req;
			tp->t_uiop->uio_iov->iov_base	= tp->t_abuff;
			l_uio_p 			= tp->t_uiop;
		}
	}

	return (physiock(st01strategy, 0, dev, B_WRITE, 0, l_uio_p));	/*LXXX*/
}

/* Work around for MET statistics deficiences: begin */
/*
 * Returns decimal string of value
 */
char *
st01_uinttostr(uint num)
{
	static char strbuf[11], *str;

	str = strbuf + (11 - 1);
	*str = '\0';
	do {
		--str;
		*str = '0' + (num % 10);
		num /= 10;
	} while (num > 0);
	return(str);
}
/* Work around for MET statistics deficiences: end */

/*
 * st01ioctl() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Driver ioctl() entry point.  Used to implement the following
 *	special functions:
 *
 *    B_GETTYPE		-  Get bus type and driver name
 *    B_GETDEV		-  Get pass-through major/minor numbers
 *
 *    T_RWD		-  Rewind tape to BOT
 *    T_WRFILEM		-  Write file mark
 *    T_SFF/SFB		-  Space filemarks forward/backwards
 *    T_SBF/SBB		-  Space blocks forward/backwards
 *    T_LOAD/UNLOAD	-  Medium load/unload
 *    T_ERASE		-  Erase tape
 *    T_RETENSION	-  Retension tape
 *    T_RST		-  Reset tape
 *
 *    T_PREVMV		-  Prevent medium removal
 *    T_ALLOMV		-  Allow medium removal
 *    T_RDBLKLEN	-  Read block length limits
 *    T_WRBLKLEN	-  Write block length to be used
 *    T_STD		-  Set tape density
 *
 *    T_GETCOMP		-  Get compression/decompression status		LXXX
 *    T_SETCOMP		-  Set compression/decompression status		LXXX
 *
 */
/* ARGSUSED */
st01ioctl(dev, cmd, arg, oflag, cred_p, rval_p)
cred_t	*cred_p;
int	*rval_p;
dev_t dev;
int cmd, oflag;
caddr_t arg;
{
	int cnt;
	int code;
	int dc;
	int ret_code = 0;
	pl_t opri;
	int i;
	dev_t pdev;
	unsigned maxblen;

	struct tape *tp;
#ifdef T_INQUIR
	struct ident *idp;
#endif
	struct blklen *cp;
	struct mode *mp;
#ifdef T_GETCOMP						/* LXXX */
	int comp_status;					/* LXXX */
#endif								/* LXXX */

	tp = TPPTR(geteminor(dev));
	ASSERT(tp);

	switch(cmd) {

	case T_RST:	/* Reset tape */
#ifdef TARGET_RESET
		if (tp->t_state & T_WRITTEN) {
			/* write double filemark */
			ST01IOCTL(T_WRFILEM, 2);
		}

		tp->t_fltrst->sb_type =     SFB_TYPE;
		tp->t_fltrst->SFB.sf_int =  st01intf;
		tp->t_fltrst->SFB.sf_dev =  tp->t_addr;
		tp->t_fltrst->SFB.sf_wd =   (long) tp;
		tp->t_fltrst->SFB.sf_func = SFB_RESETM;
		ret_code = st01docmd (sdi_xicmd, tp, tp->t_fltrst, NULL, KM_SLEEP);

		if (ret_code != SDI_RET_OK) {
			return(EIO);
		}

		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
#endif
		break;

	case T_RWD:	/* Rewind to beginning of tape */

		if (tp->t_state & T_WRITTEN) {
			ST01IOCTL(T_WRFILEM, 2);
		}

		while ((ret_code = st01cmd(tp, SS_REWIND, 0, NULL, 0, 0,
		    SCB_READ, 0, 0)) == EBUSY ) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;

	case T_WRFILEM:		/* Write filemarks */

		cnt = (int) arg;
		if (cnt < 0) {
			return(EINVAL);
		}

		while ((ret_code = st01cmd(tp, SS_FLMRK, cnt>>8, NULL, 0,
		    cnt, SCB_WRITE, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		tp->t_state |= T_FILEMARK;
		tp->t_state &= ~T_WRITTEN;
		break;

	case T_SFF:	/* Space filemarks forward   */
	case T_SFB:	/* Space filemarks backwards */
	case T_SBF:	/* Space blocks forward    */
	case T_SBB:	/* Space blocks backwards  */

		cnt = (int)arg;

		if (tp->t_state & T_WRITTEN) {
			return(EINVAL);
		}

		if( cnt & ~0xffffff ) {
			/*
			 * 'arg' is required to be positive,
			 * and 'cnt' has a 24-bit size limit
			 */
			return(EINVAL);
		}

		if (cmd == T_SFF || cmd == T_SFB) {
			code = FILEMARKS;
		}
		else {
			code = BLOCKS;
		}

		if (cmd == T_SFB || cmd == T_SBB) {
			cnt = (-1) * cnt;
		}

		while ((ret_code = st01cmd(tp, SS_SPACE, cnt>>8, NULL, 0,
		    cnt, SCB_READ, code, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;

	case T_LOAD:	/* Medium load/unload */

		if (tp->t_state & T_WRITTEN) {
			ST01IOCTL(T_WRFILEM, 2);
		}

		while ((ret_code = st01cmd(tp, SS_LOAD, 0, NULL, 0, LOAD,
		    SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;

	case T_UNLOAD:		/* Unload media */

		if (tp->t_state & T_WRITTEN) {
			/* write double filemark */
			ST01IOCTL(T_WRFILEM, 2);
		}

		while ((ret_code = st01cmd(tp, SS_LOAD, 0, NULL, 0, UNLOAD,
		    SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		tp->t_state &= ~T_PARMS;
		break;

	case T_ERASE:		/* Erase Tape */

		ST01IOCTL(T_RWD, 1);	/* Must rewind first */

		while ((ret_code = st01cmd(tp, SS_ERASE, 0, NULL, 0, 0,
		    SCB_WRITE, LONG, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;

	case T_RETENSION:	/* Retension tape */

		if (tp->t_state & T_WRITTEN) {
			ST01IOCTL(T_WRFILEM, 2);
		}

		while ((ret_code = st01cmd(tp, SS_LOAD, 0, NULL, 0, RETENSION,
		    SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;

	case B_GETTYPE:		/* Tell user bus and driver name */

		if (copyout("scsi", ((struct bus_type *)arg)->bus_name, 5)) {
			return(EFAULT);
		}

		if (copyout("st01", ((struct bus_type *)arg)->drv_name, 5)) {
			return(EFAULT);
		}

		break;

	case B_GETDEV:			/* Return pass-thru device       */
					/* major/minor to user in 'arg'. */

		sdi_getdev(&tp->t_addr, &pdev);
		if (copyout((caddr_t)&pdev, arg, sizeof(pdev)))
			return(EFAULT);
		break;

	/*
	 * The following ioctls are group 0 commands
	 */
#ifdef T_TESTUNIT
	case T_TESTUNIT:		/* Test Unit Ready */

		while ((ret_code = st01cmd(tp, SS_TEST, 0, NULL, 0, 0,
		    SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		break;
#endif

	case T_PREVMV:			/* Prevent media removal */

		while ((ret_code = st01cmd(tp, SS_LOCK, 0, NULL, 0, 1,
		    SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		break;

	case T_ALLOMV:			/* Allow media removal */

		while ((ret_code = st01cmd(tp, SS_LOCK, 0, NULL, 0, 0,
		    SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		break;

#ifdef T_INQUIR
	case T_INQUIR:			/* Inquiry */

		idp = (struct ident *)tp->t_ident;

		while ((ret_code = st01cmd(tp, SS_INQUIR, 0, idp,
		    sizeof(struct ident), sizeof(struct ident),
		    SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		if (copyout((char *)idp, arg, sizeof(struct ident))) {
			return(EFAULT);
		}
		break;
#endif

	case T_RDBLKLEN:		/* Read block length limits */

		cp = tp->t_blklen;
		while ((ret_code = st01cmd(tp, SS_RDBLKLEN, 0, cp,
		    RDBLKLEN_SZ, 0, SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		cp->max_blen = sdi_swap24(cp->max_blen);
		cp->min_blen = sdi_swap16(cp->min_blen);
		if (copyout((char *)cp, arg, RDBLKLEN_SZ)) {
			return(EFAULT);
		}

		break;

	case T_WRBLKLEN:		/* Write block length to be used */

		mp = (struct mode *)tp->t_mode;		/*LXXX*/
		cp = tp->t_blklen;
		if (copyin(arg, (char *)cp, RDBLKLEN_SZ) < 0) {
			return(EFAULT);
		}

		while ((ret_code = st01cmd(tp, SS_MSENSE, 0, mp,
		    sizeof(struct mode), sizeof(struct mode), SCB_READ, 0,
		    0)) == EBUSY ) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		/*
		 * If the requested legnth is greater than the HBA's Maximum
		 * Transfer, fail the requet.
		 */
		if ( ! tp->t_addr.pdi_adr_version )	
			maxblen = (HIP(HBA_tbl[SDI_EXHAN(&tp->t_addr)].info)->max_xfer - ptob(1));
		else
			maxblen = (HIP(HBA_tbl[SDI_HAN_32(&tp->t_addr)].info)->max_xfer - ptob(1));
		if( cp->max_blen > maxblen ) {
			return(EINVAL);
		}

		mp->md_len = 0;				 /* Reserved field */
		mp->md_media = 0;			 /* Reserved field */
		mp->md_wp = 0;				 /* Reserved field */
		mp->md_nblks = 0;			 /* Reserved field */
		mp->md_bsize = sdi_swap24(cp->max_blen); /* Fix block size */

		while ((ret_code = st01cmd(tp, SS_MSELECT, 0, mp,
		    sizeof(struct mode), sizeof(struct mode), SCB_WRITE,
		    0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EINVAL);
		}

		tp->t_bsize = mp->md_bsize;	/* Note the new block size */

		st01initialisebcb( tp );	/* Update bcb fields	   */
		break;

	case T_STD:			/* Set Tape Density */

		dc = (int) arg;
		mp = (struct mode *)tp->t_mode;			/*LXXX*/

		if (dc < 0 || dc > 0xff) {
			return(EINVAL);
		}

		while ((ret_code = st01cmd(tp, SS_MSENSE, 0, mp,
		    sizeof(struct mode), sizeof(struct mode), SCB_READ,
		    0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		mp->md_len = 0;			/* Reserved field 	*/
		mp->md_media = 0;		/* Reserved field 	*/
		mp->md_wp = 0;			/* Reserved field 	*/
		mp->md_nblks = 0;		/* Reserved field 	*/
		mp->md_dens = dc;		/* User's density code */
		while ((ret_code = st01cmd(tp, SS_MSELECT, 0, mp,
		    sizeof(struct mode), sizeof(struct mode), SCB_WRITE,
		    0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		break;

	case T_EOD:			/* Position to end of recorded data */

		code = EORD;

		if (tp->t_state & T_WRITTEN) {
			return(EINVAL);
		}

		while ((ret_code = st01cmd(tp, SS_SPACE, 0, NULL, 0, 0,
		    SCB_READ, code, 0)) == EBUSY ) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;

#ifdef T_MSENSE
	case T_MSENSE:			/* Mode sense */

		/*
		 * (Application has to swap all multi-byte fields)
		 */
		mp = (struct mode *)tp->t_mode;

		if (copyin(arg, (char *)mp, sizeof(struct mode)) < 0) {
			return(EFAULT);
		}

		while ((ret_code = st01cmd(tp, SS_MSENSE, 0, mp,
		    sizeof(struct mode), sizeof(struct mode), SCB_READ,
		    0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		if (copyout((char *)mp, arg, sizeof(struct mode))) {
			return(EFAULT);
		}

		break;
#endif
#ifdef T_MSELECT
	case T_MSELECT:			/* Mode select */

		/*
		 * (Application has to swap all multi-byte fields)
		 */
		mp = (struct mode *)tp->t_mode;

		if (copyin(arg, (char *)mp, sizeof(struct mode)) < 0) {
			return(EFAULT);
		}

		while ((ret_code = st01cmd(tp, SS_MSELECT, 0, mp,
		    sizeof(struct mode), sizeof(struct mode), SCB_WRITE,
		    0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		break;
#endif
	/*
	 * LXXX: The following ioctls allow compression status to be queried
	 *	 and set on compression-capable devices (e.g. Exabyte 8500C,
	 *	 HP 1533A). This is an essential feature to allow backups
	 *	 to be made which are transportable between different physical
	 *	 devices.
	 *	 Trying to access compression settings on non-compressing
	 *	 devices will result in EINVAL being returned to the app.
	 */
#ifdef T_GETCOMP
	case T_GETCOMP:			/* Get Compression status */
		if ( ! (tp->t_state & T_COMPRESSION) ) {
			return(EINVAL);
		}
		if( ret_code = st01getcomp(tp, (ulong_t *)&comp_status) ) {
			return(EIO);
		}
		if ( copyout((char *)&comp_status, arg, sizeof(comp_status)) ){
			return(EFAULT);
		}
		break;
#endif
#ifdef T_SETCOMP
	case T_SETCOMP:			/* Set Compression status */
		if ( ! (tp->t_state & T_COMPRESSION) ) {
			return(EINVAL);
		}
		return( st01setcomp(tp, (ulong_t)arg) );
#endif

	default:
		ret_code = EINVAL;
	}

	return(ret_code);
}

/*
 * st01print() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function prints the error message provided by the kernel.
 */
int
st01print(dev, str)
dev_t dev;
register char *str;
{
	char name[SDI_NAMESZ];
	register struct tape *tp;

	tp = TPPTR(geteminor(dev));
	ASSERT(tp);
	sdi_xname(tp->t_addr.pdi_adr_version, &tp->t_addr, name);
		/*
	  	 *+ display err msg
		 */
        cmn_err(CE_WARN, "Tape driver: %s, %s\n", name, str);
	return(0);
}

/*
 * st01io() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function creates and sends a SCSI I/O request.
 */
void
st01io(tp, bp)
register struct tape *tp;
register buf_t *bp;
{
	register struct job *jp;
	register struct scb *scb;
	register struct scs *cmd;
	register int nblk;
	struct	 xsb	*xsb;

	jp = st01getjob();
	jp->j_tp = tp;
	jp->j_bp = bp;

	jp->j_sb->sb_type = SCB_TYPE;
	jp->j_time = JTIME;

	/*
	 * Fill in the scb for this job.
	 */

	scb = &jp->j_sb->SCB;
	scb->sc_cmdpt = SCS_AD(&jp->j_cmd);
	scb->sc_cmdsz = SCS_SZ;
	scb->sc_datapt = bp->b_un.b_addr;
	scb->sc_datasz = bp->b_bcount;
	scb->sc_link = NULL;
	scb->sc_mode = (bp->b_flags & B_READ) ? SCB_READ : SCB_WRITE;
	scb->sc_dev = tp->t_addr;

	xsb = (struct xsb *)jp->j_sb;
	if (xsb->extra.sb_data_type != SDI_PHYS_ADDR) {
		if (bp->b_addrtype == SDI_PHYS_ADDR) {
			xsb->extra.sb_data_type = SDI_PHYS_ADDR;
			xsb->extra.sb_datapt =
			  (paddr32_t *) ((char *)paddr(bp) + bp->b_bcount);
		}
		else {
			xsb->extra.sb_data_type = SDI_IO_BUFFER;
			xsb->extra.sb_bufp = bp;
		}
	}

	sdi_xtranslate(jp->j_sb->SCB.sc_dev.pdi_adr_version, 
		jp->j_sb, bp->b_flags, bp->b_proc, KM_SLEEP);

	scb->sc_int = st01intn;
	scb->sc_time = JTIME;
	scb->sc_wd = (long) jp;

	/*
	 * Fill in the command for this job.
	 */

	cmd = (struct scs *)&jp->j_cmd;
	cmd->ss_op = (bp->b_flags & B_READ) ? SS_READ : SS_WRITE;
	if (! tp->t_addr.pdi_adr_version )
		cmd->ss_lun  = tp->t_addr.sa_lun;
	else
		cmd->ss_lun  = tp->t_addr.extended_address->scsi_adr.scsi_lun;

	if (tp->t_bsize) {	 /* Fixed block transfer mode	*/
		cmd->ss_mode = FIXED;
		nblk = bp->b_bcount / tp->t_bsize;
		cmd->ss_len1 = mdbyte(nblk) << 8 | msbyte(nblk);
		cmd->ss_len2 = lsbyte(nblk);
	} else {		/* Variable block transfer mode */
		cmd->ss_mode = VARIABLE;
		cmd->ss_len1 = mdbyte(bp->b_bcount) << 8 | msbyte(bp->b_bcount);
		cmd->ss_len2 = lsbyte(bp->b_bcount);
	}
	cmd->ss_cont = 0;

	st01docmd(sdi_xsend, jp->j_tp, jp->j_sb, jp, KM_SLEEP);	/* send it */
}

/*
 * st01comp() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Called on completion of a job, both successful and failed.
 *	The function de-allocates the job structure used and calls
 *	biodone().  Restarts the logical unit queue if necessary.
 */
void
st01comp(jp)
register struct job *jp;
{
        register struct tape *tp;
	register struct buf *bp;

        tp = jp->j_tp;
	bp = jp->j_bp;

	/* Check if job completed successfully */
	if (jp->j_sb->SCB.sc_comp_code == SDI_ASW) {
		tp->t_lastop = jp->j_cmd.ss.ss_op;
		if (tp->t_lastop == SS_READ)
			tp->t_state |= T_READ;
		if (tp->t_lastop == SS_WRITE) {
			tp->t_state &= ~T_FILEMARK;
			tp->t_state |= T_WRITTEN;
		}
	} else {
		bp->b_flags |= B_ERROR;

		if (bp->b_resid == 0)
			bp->b_resid = bp->b_bcount;

		if (jp->j_sb->SCB.sc_status == S_BUSY ) {
			bp->b_error = EBUSY;
		}
		else {

			if (jp->j_sb->SCB.sc_comp_code == SDI_NOSELE) {
				bp->b_error = ENODEV;
			}
			else {
				bp->b_error = EIO;
			}

			if (tp->t_state & T_TAPEEND)	{
				bp->b_error = ENOSPC;
			}
		}
	}

/* Work around for MET statistics deficiences: begin */
	if (tp->t_stat)
	{
		pl_t opri;

		opri=SPL(); 
 		met_ds_iodone(tp->t_stat, bp->b_flags,
				(bp->b_bcount - bp->b_resid));
		splx(opri);
	}
/* Work around for MET statistics deficiences: end */

#ifdef PDI_SVR42
	biodone(bp);
#else
	if( tp->t_state & T_ABORTED ) {
		tp->t_state &= ~T_ABORTED;
		freerbuf(bp);
	}
	else {
		biodone(bp);
	}
#endif

	st01freejob(jp);

	/* Resume queue if suspended */
	if (tp->t_state & T_SUSPEND)
	{
		tp->t_fltres->sb_type = SFB_TYPE;
		tp->t_fltres->SFB.sf_int  = st01intf;
		tp->t_fltres->SFB.sf_dev  = tp->t_addr;
		tp->t_fltres->SFB.sf_wd = (long) tp;
		tp->t_fltres->SFB.sf_func = SFB_RESUME;
		if (st01docmd (sdi_xicmd, tp, tp->t_fltres, NULL, KM_NOSLEEP) ==
		    SDI_RET_OK) {
			tp->t_state &= ~T_SUSPEND;
			tp->t_fltcnt = 0;
		}
	}

	return;
}

/*
 * st01intn() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function is called by the host adapter driver when an
 *	SCB job completes.  If the job completed with an error, the
 *	appropriate error handling is performed.
 */
void
st01intn(sp)
register struct sb *sp;
{
	register struct tape *tp;
	register struct job *jp;

	jp = (struct job *)sp->SCB.sc_wd;
	tp = jp->j_tp;

	if (sp->SCB.sc_comp_code == SDI_ASW) {
		st01comp(jp);
		return;
	}

	if (sp->SCB.sc_comp_code & SDI_SUSPEND)
		tp->t_state |= T_SUSPEND;

	if (sp->SCB.sc_comp_code == SDI_CKSTAT &&
	    sp->SCB.sc_status == S_CKCON)
	{
		struct sense *sensep = sdi_sense_ptr(sp);
		tp->t_fltjob = jp;

		if ((sensep->sd_key != SD_NOSENSE) || sensep->sd_fm 
		  || sensep->sd_eom || sensep->sd_ili) {
			bcopy(sensep, tp->t_sense, sizeof(struct sense));
			st01sense(tp,1);
			return;
		}
		tp->t_fltreq->sb_type = ISCB_TYPE;
		tp->t_fltreq->SCB.sc_int = st01intrq;
		tp->t_fltreq->SCB.sc_cmdsz = SCS_SZ;
		tp->t_fltreq->SCB.sc_time = JTIME;
		tp->t_fltreq->SCB.sc_mode = SCB_READ;
		tp->t_fltreq->SCB.sc_dev = sp->SCB.sc_dev;
		tp->t_fltreq->SCB.sc_wd = (long) tp;
		tp->t_fltreq->SCB.sc_datapt = SENSE_AD(tp->t_sense);
		tp->t_fltreq->SCB.sc_datasz = SENSE_SZ;
		tp->t_fltcmd.ss_op = SS_REQSEN;
		if (! sp->SCB.sc_dev.pdi_adr_version )
			tp->t_fltcmd.ss_lun = sp->SCB.sc_dev.sa_lun;
		else
			tp->t_fltcmd.ss_lun = 
				sp->SCB.sc_dev.extended_address->scsi_adr.scsi_lun;
		tp->t_fltcmd.ss_addr1 = 0;
		tp->t_fltcmd.ss_addr = 0;
		tp->t_fltcmd.ss_len = SENSE_SZ;
		tp->t_fltcmd.ss_cont = 0;

		/* clear old sense key */
		tp->t_sense->sd_key = SD_NOSENSE;

		if (st01docmd (sdi_xicmd, tp, tp->t_fltreq, NULL, KM_NOSLEEP) !=
		    SDI_RET_OK) {
			/* Fail the job */
			st01logerr(tp, sp);
			st01comp(jp);
		}
	}
	else
	{
		/* Fail the job */
		st01logerr(tp, sp);
		st01comp(jp);
	}

	return;
}

/*
 * st01intrq() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function is called by the host adapter driver when a
 *	request sense job completes.  The job will be retried if it
 *	failed.  Calls st01sense() on successful completions to
 *	examine the request sense data.
 */
void
st01intrq(sp)
register struct sb *sp;
{
	register struct tape *tp;

	tp = (struct tape *)sp->SCB.sc_wd;

	if (sp->SCB.sc_comp_code != SDI_CKSTAT  &&
	    sp->SCB.sc_comp_code &  SDI_RETRY   &&
	    ++tp->t_fltcnt <= MAX_RETRY)
	{
		sp->SCB.sc_time = JTIME;
		if (st01docmd (sdi_xicmd, tp, sp, NULL, KM_NOSLEEP) != SDI_RET_OK) {
			st01logerr(tp, sp);
			st01comp(tp->t_fltjob);
		}
		return;
	}

	if (sp->SCB.sc_comp_code != SDI_ASW) {
		st01logerr(tp, sp);
		st01comp(tp->t_fltjob);
		return;
	}

	st01sense(tp, 1);
}

/*
 * st01intf() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function is called by the host adapter driver when a host
 *	adapter function request has completed.  If there was an error
 *	the request is retried.  Used for resume function completions.
 */
void
st01intf(sp)
register struct sb *sp;
{
	register struct tape *tp;

	tp = (struct tape *)sp->SFB.sf_wd;

	if (sp->SFB.sf_comp_code & SDI_RETRY  &&
	    ++tp->t_fltcnt <= MAX_RETRY)
	{
		if (st01docmd (sdi_xicmd, tp, sp, NULL, KM_NOSLEEP) != SDI_RET_OK) {
			st01logerr(tp, sp);
		}
		return;
	}

	if (sp->SFB.sf_comp_code != SDI_ASW)
		st01logerr(tp, sp);
}

/*
 * st01sense() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Performs error handling based on SCSI sense key values.
 *
 * LXXX: For ILI reads where the on-tape data is of a larger block size, we
 *	 start a recovery chain to attempt to read the data into the per-unit
 *	 aligned buffer. Basic steps are:
 *		1. Space 1 block back
 *		2. Issue READ with correct block size
 *		3. Complete original job
 *	 Note: it is essential that st01comp() is not called until this chain
 *	 has completed. See st01retryread(), st01intsc, st01intrrc for details.
 *
 * Side Effects:
 *	If <complete_job> is set, the associated user request will be completed
 *	by calling st01comp(). If clear, the calling routine is responsible for
 *	terminating the user request itself.
 *
 * Return Values:
 *	0 if a hard error occurred
 *	1 if an informational error occurred (e.g. FileMark, E-O-D)
 */
STATIC int
st01sense(struct tape *tp, int complete_job)
{
	register struct job *jp;
	register struct sb *sp;
	register int	noerr = 0;

	jp = tp->t_fltjob;
	sp = jp->j_sb;

        switch(tp->t_sense->sd_key) {
	case SD_VOLOVR:
		tp->t_state |= T_TAPEEND;
		/* FALLTHROUGH */

	case SD_NOSENSE:
	{
		register struct buf *bp = jp->j_bp;

		if ( (jp->j_cmd.ss.ss_op == SS_READ  ||
		      jp->j_cmd.ss.ss_op == SS_WRITE) &&
		   (tp->t_sense->sd_valid) ) {
			if (tp->t_bsize) {	/* Fixed Block Len */
				bp->b_resid = tp->t_bsize * 
						sdi_swap32(tp->t_sense->sd_ba);
			} else {		/* Variable	   */
				bp->b_resid = sdi_swap32(tp->t_sense->sd_ba);
			}

			if (tp->t_sense->sd_ili) {

			    if ( tp->t_state & T_SHORT_READ ) {
				st01logerr(tp,sp);
				cmn_err(CE_WARN, 
			"Tape driver: Unrecoverable Block length mismatch.");
				cmn_err(CE_CONT,
			"Driver requested %d, available %d\n",
			bp->b_bcount, bp->b_bcount - (int) bp->b_resid);
				bp->b_resid = bp->b_bcount;
			    } else {
				if (tp->t_bsize) {	/* Fixed Block Len */
					/*
					 * The t_sense->sd_ba information field
					 * now contains:
				 	 *
					 * (signed)((bp->b_bcount)-
					 * 	(actual number of blocks read))
					 */
					st01logerr(tp, sp);
					cmn_err(CE_WARN,
			"Tape driver: Block length mismatch.");
					cmn_err(CE_CONT,
			"Driver requested %d blocks, available %d blocks\n",
			bp->b_bcount, bp->b_bcount - (int) bp->b_resid);
				}
				else {
					/*
					 * This is from SCSI-2 document:
					 *
					 * If the fixed bit is zero, the 
					 * information field shall be set to 
					 * the requested transfer length minus
					 * the actual block length. 
					 */

					if ( (int)bp->b_resid >= 0 ) {
						/* 
						 * This is not an error, as 
						 * we requested more than the 
						 * actual record length, the
						 * request has been filled
						 */
						noerr = 1;
						/*
						 * Leave b_resid alone, read()
						 * must return the correct
						 * amount
						 */
#if 0								/* LXXX */
						bp->b_resid = 0;
#endif								/* LXXX */
					}
					else {
						/* 
						 * LXXX: To fix this we must
						 * use the unit-specific page
						 * aligned buffer and see if
						 * we can do the read transfer
						 * into it. 
						 * We must reposition the tape
						 * 1 block back and then retry
						 * the read with the correct
						 * block length. Flag the
						 * condition with T_SHORT_READ
						 * to stop recursion if an ILI
						 * happens on the re-read
						 */
						/* 
						 * overlength condition -
						 *
						 * This is an error, as 
						 * the actual record size was
						 * larger than the requested 
						 * amount, requested data has
						 * been read, but we have 
						 * skipped over the rest of 
						 * the record.
						 */
#if DEBUG							/* LXXX */
						st01logerr(tp, sp);
						cmn_err(CE_WARN,
			"Tape driver: Block length mismatch (overlength).");
						cmn_err(CE_CONT,
			"Driver requested %d bytes, Tape block length = %d bytes\n",
			bp->b_bcount, bp->b_bcount - (int) bp->b_resid);
#endif								/* LXXX */
						tp->t_state |= T_SHORT_READ;
#if 0								/* LXXX */
						bp->b_resid = bp->b_bcount;
#endif								/* LXXX */
						noerr = 1;
						st01retryread(tp); /* LXXX */
						return noerr;	   /* LXXX */
					}
				}
			    }	/* T_SHORT_READ */
			}
			
			if ((int)bp->b_resid < 0)
				bp->b_resid = 0;
		}

		if (tp->t_sense->sd_fm) {
			tp->t_state |= T_FILEMARK;
			noerr = 1;
		}

		if (tp->t_sense->sd_eom) {
			/*
			 * Kennedy GCR gives early warning EOM error on READ,
			 * which we want to ignore up to the point where
			 * we encounter the filemark.
			 */
			noerr = 1;
			if (jp->j_cmd.ss.ss_op != SS_READ) {
			     tp->t_state |= T_TAPEEND;
			}
			if (tp->t_state & T_FILEMARK)   {
				tp->t_state |= T_TAPEEND;
				noerr = 0;
			}
		}

		if (tp->t_state & T_TAPEEND)	{
			if(bp->b_bcount == 0 || bp->b_resid != bp->b_bcount) {
				noerr = 1;
			}
		}

		if (noerr) {
			sp->SCB.sc_comp_code = SDI_ASW;
		}
			
		if ( complete_job ) {				/* LXXX */
			st01comp(jp);
		}						/* LXXX */
		break;
	}

	case SD_UNATTEN:
		if (++tp->t_fltcnt > MAX_RETRY) {
			st01logerr(tp, sp);
			if ( complete_job ) {			/* LXXX */
				st01comp(jp);
			}					/* LXXX */
		} else {
			sp->sb_type = ISCB_TYPE;
			sp->SCB.sc_time = JTIME;
			if (st01docmd (sdi_xicmd, tp, sp, NULL, KM_NOSLEEP) != 
			    SDI_RET_OK) {
				st01logerr(tp, sp);
				if ( complete_job ) {		/* LXXX */
					st01comp(jp);
				}				/* LXXX */
			}
		}
		break;

	case SD_RECOVER:
		st01logerr(tp, sp);
		sp->SCB.sc_comp_code = SDI_ASW;
		if ( complete_job ) {				/* LXXX */
			st01comp(jp);
		}						/* LXXX */
		noerr = 1;					/* LXXX */
		break;

	/* Some drives give a blank check during positioning */
	case SD_BLANKCK:
		/*
		if ((jp->j_cmd.ss.ss_op == SS_READ)
		||  (jp->j_cmd.ss.ss_op == SS_WRITE))
			st01logerr(tp, sp);
		else
			sp->SCB.sc_comp_code = SDI_ASW;
		*/
		st01logerr(tp, sp);
		if ( complete_job ) {				/* LXXX */
			st01comp(jp);
		}						/* LXXX */
		break;

	default:
		st01logerr(tp, sp);
		if ( complete_job ) {				/* LXXX */
			st01comp(jp);
		}						/* LXXX */
	}
	return noerr;						/* LXXX */
}

/*
 * st01logerr() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function will print the error messages for errors detected
 *	by the host adapter driver.  No message will be printed for
 *	those errors that the host adapter driver has already reported.
 */
void
st01logerr(tp, sp)
register struct tape *tp;
register struct sb *sp;
{
	if (sp->sb_type == SFB_TYPE) {
		if ((tp->t_state & T_OPENING) == 0 ) {
			sdi_errmsg("Tape",&tp->t_addr,sp,tp->t_sense,SDI_SFB_ERR,0);
		}
		return;
	}

	if (sp->SCB.sc_comp_code == SDI_CKSTAT  && sp->SCB.sc_status == S_CKCON) {
		if ((tp->t_state & T_OPENING) == 0 ) {
			sdi_errmsg("Tape",&tp->t_addr,sp,tp->t_sense,SDI_CKCON_ERR,0);
		}
		return;
	}

	if (sp->SCB.sc_comp_code == SDI_CKSTAT) {
		if ((tp->t_state & T_OPENING) == 0 ) {
			sdi_errmsg("Tape",&tp->t_addr,sp,tp->t_sense,SDI_CKSTAT_ERR,0);
		}
	}
}

/*
 * st01config() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Initializes the tape driver's tape parameter structure. To
 *	support Autoconfig, the command sequence should be:
 *
 *	if T_PARMS is set
 *
 *		TEST UNIT READY
 *
 *	else
 *
 *		INQUIRY*
 *		LOAD
 *		TEST UNIT READY
 *		MODE SENSE
 *		MODE SELECT*
 *
 *	Autoconfig is not implemented at this point. In this case, both
 *	INQUIRY and MODE SELECT are not called.	 The driver will use
 *	whatever settings returned by MODE SENSE as default.
 */
st01config(tp)
register struct tape *tp;
{
	int	rtcde;
	struct	blklen	*cp;
	struct	mode	*mp;
	struct	ident	*ip;				/* LXXX */
	struct  mode_dcc_page	*mdccp;			/* LXXX */
	struct	mode_dcc_data	*mdccdp;		/* LXXX */
	ulong_t	compress = 0;				/* LXXX */

	cp = tp->t_blklen;
	ip = tp->t_ident;				/* LXXX */

	/* 
	 * If the T_PARMS is set, just check if the volume is
	 * mounted. All else is well.
	 */
	if (tp->t_state & T_PARMS) {
		while ((rtcde = st01cmd(tp, SS_TEST, 0, NULL, 0, 0, SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if( rtcde != 0 ) {
			return( EIO );
		}
		st01initialisebcb( tp );			/* LXXX */
		/*
		 * LXXX: Restore compression settings in case they have been
		 *	 changed from what we last asked for.
		 */
		if ( tp->t_state & T_COMPRESSED ) {
			mdccp = tp->t_dcc;
			if ( mdccp->h.md_bdl ) {
				mdccdp = &mdccp->p.md_bd_dcc.md_dcc;
			} else {
				mdccdp = &mdccp->p.md_dcc;
			}

			compress  = mdccdp->mp_dce ? 1 : 0;
			compress |= mdccdp->mp_dde ? 2 : 0;

			return( st01setcomp(tp, compress) );
		}
		return 0;
	}

	/*
	 * T_PARMS is not set, do a complete checking
	 */

	/*
 	 * LXXX: We must issue an INQUIRY to determine exactly what flavour of
	 * tape device is out there. If it's SCSI-2 we will attempt to get
	 * the compression characteristics (using the mode_dcc_page). If it
	 * does not support SCSI-2 we will fall back to using the old SCSI-1
	 * 'mode' definition (original behaviour)
	 */
	
	while ((rtcde = st01cmd(tp, SS_INQUIR, 0, (char *)ip, 
		sizeof(struct ident), sizeof(struct ident), 
		SCB_READ, 0, 0)) == EBUSY ) {
		delay(1 * HZ);
	}

	if ( rtcde != 0 ) {
		return( EIO );
	}

	while ((rtcde = st01cmd(tp, SS_LOAD, 0, NULL, 0, LOAD, SCB_READ, 0, 0)) == EBUSY) {
		delay(1 * HZ);
	}

	if( rtcde != 0 ) {
		return( EIO );
	}

	while ((rtcde = st01cmd(tp, SS_TEST, 0, NULL, 0, 0, SCB_READ, 0, 0)) == EBUSY) {
		delay(1 * HZ);
	}

	if( rtcde != 0 ) {
		return( EIO );
	}

	while ((rtcde = st01cmd(tp, SS_RDBLKLEN, 0, (char *)cp, RDBLKLEN_SZ, 0, SCB_READ, 0, 0)) == EBUSY) {
		delay(1 * HZ);
	}

	if( rtcde != 0 ) {
		return( EIO );
	}

	cp->max_blen = sdi_swap24(cp->max_blen);
	cp->min_blen = sdi_swap16(cp->min_blen);

	/*
	 * LXXX: If the minimum and maximum block lengths are the same, we are
	 *	 a fixed block device. This means we can free up any aligned
	 *	 buffers as we will never be issuing variable block read/write
	 *	 calls.
	 */
	if ( cp->max_blen == cp->min_blen ) {
		if ( tp->t_abuff ) {
			/* No extra paddr field is allocated with tp->t_uiop */
			KMEM_FREE(tp->t_uiop, sizeof(uio_t));
			KMEM_FREE(tp->t_abuff, tp->t_abcbp->bcb_max_xfer +
				SDI_PADDR_SIZE(tp->t_abcbp->bcb_physreqp));
			physreq_free(tp->t_abcbp->bcb_physreqp);
			bcb_free(tp->t_abcbp);
			tp->t_abuff = NULL;
			tp->t_abcbp = NULL;
			tp->t_uiop  = NULL;
		}
	}

	/* Send Mode Sense	*/

	mp = tp->t_mode;

	/*
	 * LXXX: For a SCSI-2 compliant device, we will try to issue a MODE
	 * SENSE for the data compression page. If this fails, we fall back
	 * to using the original mode sense data (i.e. header + block descr).
	 * This is a useful place to determine if the drive can support
	 * compression (tp->t_state & T_COMPRESSION)
	 * If the drive was set into a particular compression mode beforehand,
	 * the t_dcc page contents are valid. We must extract the settings
	 * and restore the drive to these compression characteristics.
	 */

	if ( tp->t_state & T_COMPRESSED ) {
		mdccp = tp->t_dcc;
		if ( mdccp->h.md_bdl ) {
			mdccdp = &mdccp->p.md_bd_dcc.md_dcc;
		} else {
			mdccdp = &mdccp->p.md_dcc;
		}

		compress  = mdccdp->mp_dce ? 1 : 0;
		compress |= mdccdp->mp_dde ? 2 : 0;
	}

	tp->t_state &= ~T_COMPRESSION;

	if ( ((ip->id_ver & 0x7) > 1) && tp->t_dcc ) {
		/*
		 * SCSI-2, so we have a chance of compression
		 */
		mdccp = tp->t_dcc;
		while ((rtcde = st01cmd(tp, SS_MSENSE, 0x0F00, mdccp,
		   sizeof(struct mode_dcc_page), sizeof(struct mode_dcc_page),
		   SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}
		if ( rtcde == 0 ) {
			tp->t_state |= T_COMPRESSION;
			if ( tp->t_state & T_COMPRESSED ) {
				rtcde = st01setcomp(tp, compress);
			}
		} else {
			/*
			 * Free up the allocated compression page as it is
			 * of no use to this unit
			 */
			if ( tp->t_dcc ) {
				KMEM_FREE(tp->t_dcc, sizeof(struct mode_dcc_page) +
					SDI_PADDR_SIZE(tp->t_bcbp->bcb_physreqp));
				tp->t_dcc = NULL;
			}
		}
	} 
	
	while ((rtcde = st01cmd(tp, SS_MSENSE, 0, mp, sizeof(struct mode),
	    sizeof(struct mode), SCB_READ, 0, 0)) == EBUSY) {
		delay(1 * HZ);
	}

	if( rtcde != 0 ) {
		return( EIO );
	}

	mp->md_bsize = sdi_swap24(mp->md_bsize);
	if ((mp->md_bsize < cp->min_blen) && (mp->md_bsize != 0)) {
		return(ENXIO);
	}

	if ((mp->md_bsize > cp->max_blen) && (mp->md_bsize != 0) &&
	    (cp->max_blen !=0)) {
		return(ENXIO);
	}

	tp->t_bsize = mp->md_bsize;

	st01initialisebcb( tp );				/* LXXX */


	/*
	 * Indicate parameters are set and valid
	 */
	tp->t_state |= T_PARMS;
	return(0);
}

/*
 * st01initialisebcb()
 *
 * Calling/Exit State:
 *
 * Description:
 *	This function is called to initialise the buffer control block
 *	contents for the specified tape structure. This process has to
 *	be carried out whenever the block size of the tape is changed
 *	as this may affect the bcb_granularity and maximum transfer size
 *	supported
 *
 * Side Effects:
 *	bcb_granularity, bcb_flags, bcb_max_xfer may all be changed
 */
STATIC void
st01initialisebcb( struct tape *tp )
{
#ifndef PDI_SVR42
	unsigned long sys_max_xfer;
	size_t maxbiosize;

	if (tp->t_bsize) {
		tp->t_bcbp->bcb_granularity = tp->t_bsize;
		tp->t_bcbp->bcb_flags |= BCB_EXACT_SIZE;
		tp->t_bcbp->bcb_flags &= ~BCB_ONE_PIECE;
	}
	else {
		tp->t_bcbp->bcb_granularity = 1;
		tp->t_bcbp->bcb_flags &= ~BCB_EXACT_SIZE;
		tp->t_bcbp->bcb_flags |= BCB_ONE_PIECE;
	}

	/*
	 * We use 1 Page less than the HBA's MaxTransfer in case an I/O request
	 * comes down that is not aligned on a bcb_granularity boundary.
	 * This is a common occurrence when writing/reading variable blocked
	 * data (e.g. 64kb data blocks). The normal course of action is to
	 * allow any i/o which will fit into the HBAs DMA list to be passed
	 * straight through. If the transfer is larger than the modified
	 * limit, we use the guaranteed page-aligned buffer to transfer up
	 * to sys_max_xfer bytes. This involves copying data between user
	 * and kernel buffers so is _slow_.
	 */
	if ( ! tp->t_addr.pdi_adr_version )
		sys_max_xfer = HIP(HBA_tbl[SDI_EXHAN(&tp->t_addr)].info)->max_xfer;
	else
		sys_max_xfer = HIP(HBA_tbl[SDI_HAN_32(&tp->t_addr)].info)->max_xfer;
	drv_getparm(DRV_MAXBIOSIZE, (size_t *) &maxbiosize);
	if (sys_max_xfer > maxbiosize)
		sys_max_xfer = maxbiosize;

	if ( sys_max_xfer > ptob(1) ) {
	    if( tp->t_bcbp->bcb_granularity > (sys_max_xfer - ptob(1)) ) {
		/*
		 * Granularity is greater than the system's max_xfer, 
		 * use the system's limit.
		 */
		tp->t_bcbp->bcb_max_xfer = sys_max_xfer - ptob(1);
	    }
	    else {
		/*
		 * Find the largest multiple of the granularity
		 * that will fit in the system's max_fer.
		 *
		 * We use 1 Page less than the system's MaxTransfer in case an I/O request
		 * comes down that is not aligned on a bcb_granularity boundary.
		 */
		tp->t_bcbp->bcb_max_xfer = tp->t_bcbp->bcb_granularity;
		while( (tp->t_bcbp->bcb_max_xfer + tp->t_bcbp->bcb_granularity) <=
		        (sys_max_xfer - ptob(1)) ) {

			tp->t_bcbp->bcb_max_xfer += tp->t_bcbp->bcb_granularity;
		}
	    }
	} else {
	    /*
	     * Pathological case of page size >> max xfer size. We assume
	     * that any transfer will fit within one page of memory
	     */
	    if ( tp->t_bcbp->bcb_granularity > sys_max_xfer ) {
		/*
		 * Granularity is greater than the system's max_xfer, 
		 * use the system's limit.
		 */
		tp->t_bcbp->bcb_max_xfer = sys_max_xfer;
	    } else {
		/*
		 * Find the largest multiple of the granularity
		 * that will fit in the system's max_fer.
		 */
		tp->t_bcbp->bcb_max_xfer = tp->t_bcbp->bcb_granularity;
		while ( (tp->t_bcbp->bcb_max_xfer + tp->t_bcbp->bcb_granularity) <= sys_max_xfer ) {
			tp->t_bcbp->bcb_max_xfer += tp->t_bcbp->bcb_granularity;
		}
	    }
	}

#ifdef DEBUG
	cmn_err(CE_NOTE,"st01config: max_xfer = %d",sys_max_xfer);
	cmn_err(CE_NOTE,"st01config: bcb_granularity = %d",tp->t_bcbp->bcb_granularity);
	cmn_err(CE_NOTE,"st01config: bcb_max_xfer = %d",tp->t_bcbp->bcb_max_xfer);
#endif
#endif /* !PDI_SVR42 */
}

/*
 * st01cmd() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function performs a SCSI command such as Mode Sense on
 *	the addressed tape.  The op code indicates the type of job
 *	but is not decoded by this function.  The data area is
 *	supplied by the caller and assumed to be in kernel space.
 *	This function will sleep.
 */
st01cmd(tp, op_code, addr, buffer, size, length, mode, param, control)
register struct tape	*tp;
unsigned char	op_code;		/* Command opcode		*/
unsigned int	addr;			/* Address field of command 	*/
char		*buffer;		/* Buffer for command data 	*/
unsigned int	size;			/* Size of the data buffer 	*/
unsigned int	length;			/* Block length in the CDB	*/
unsigned short	mode;			/* Direction of the transfer 	*/
unsigned int	param;
unsigned int	control;
{
	register struct job *jp;
	register struct scb *scb;
	register buf_t *bp;
	struct	xsb	*xsb;
	int error;

	bp = getrbuf(KM_SLEEP);

	jp = st01getjob();
	scb = &jp->j_sb->SCB;

	bp->b_iodone = NULL;
	bp->b_blkno = addr;
	bp->b_flags |= mode & SCB_READ ? B_READ : B_WRITE;
	bp->b_error = 0;

	jp->j_bp = bp;
	jp->j_tp = tp;
	jp->j_sb->sb_type = SCB_TYPE;

	switch(op_code >> 5){
	case	GROUP7:
	{
		register struct scm *cmd;

		cmd = (struct scm *)&jp->j_cmd.sm;
		cmd->sm_op   = op_code;
		if (! tp->t_addr.pdi_adr_version )
			cmd->sm_lun  = tp->t_addr.sa_lun;
		else
			cmd->sm_lun  = tp->t_addr.extended_address->scsi_adr.scsi_lun;
		cmd->sm_res1 = 0;
		cmd->sm_addr = sdi_swap32(addr);
		cmd->sm_res2 = 0;
		cmd->sm_len  = sdi_swap16(length);
		cmd->sm_res1 = param;
		cmd->sm_cont = control;

		scb->sc_cmdpt = SCM_AD(cmd);
		scb->sc_cmdsz = SCM_SZ;
	}
		break;
	case	GROUP6:
	{
		register struct scs *cmd;

		cmd = (struct scs *)&jp->j_cmd.ss;
		cmd->ss_op    = op_code;
		if (! tp->t_addr.pdi_adr_version )
			cmd->ss_lun   = tp->t_addr.sa_lun;
		else
			cmd->ss_lun = tp->t_addr.extended_address->scsi_adr.scsi_lun;
		cmd->ss_addr1 = ((addr & 0x1F0000) >> 16);
		cmd->ss_addr  = sdi_swap16(addr & 0xFFFF);
		cmd->ss_len   = (char)length;
		cmd->ss_cont  = (char)control;

		scb->sc_cmdpt = SCS_AD(cmd);
		scb->sc_cmdsz = SCS_SZ;
	}
		break;
	case	GROUP1:
	{
		register struct scm *cmd;

		cmd = (struct scm *)&jp->j_cmd.sm;
		cmd->sm_op   = op_code;
		if (! tp->t_addr.pdi_adr_version )
			cmd->sm_lun  = tp->t_addr.sa_lun;
		else
			cmd->sm_lun = 
				tp->t_addr.extended_address->scsi_adr.scsi_lun;
		cmd->sm_res1 = param;
		cmd->sm_addr = sdi_swap32(addr);
		cmd->sm_res2 = 0;
		cmd->sm_len  = sdi_swap16(length);
		cmd->sm_cont = 0;

		scb->sc_cmdpt = SCM_AD(cmd);
		scb->sc_cmdsz = SCM_SZ;
	}
		break;
	case	GROUP0:
	{
		register struct scs *cmd;

		cmd = (struct scs *)&jp->j_cmd.ss;
		cmd->ss_op    = op_code;
		if (! tp->t_addr.pdi_adr_version )
			cmd->ss_lun   = tp->t_addr.sa_lun;
		else
			cmd->ss_lun   = tp->t_addr.extended_address->scsi_adr.scsi_lun;
		cmd->ss_addr1 = param;
		cmd->ss_addr  = sdi_swap16(addr & 0xFFFF);
		cmd->ss_len   = (char)length;
		cmd->ss_cont  = 0;

		scb->sc_cmdpt = SCS_AD(cmd);
		scb->sc_cmdsz = SCS_SZ;
	}
		break;
#ifdef DEBUG
        default:
				/*
			  	 *+ display err msg
				 */
                cmn_err(CE_WARN,"Tape driver: Unknown op_code = %x\n",op_code);
#endif
	}

	/* Fill in the SCB */
	scb->sc_int = st01intn;
	scb->sc_dev = tp->t_addr;
	scb->sc_datapt = buffer;
	scb->sc_datasz = size;
	scb->sc_mode = mode;
	scb->sc_resid = 0;
	scb->sc_time = 180 * ONE_MIN;
	scb->sc_wd = (long) jp;
	
	xsb = (struct xsb *)jp->j_sb;
	if (buffer) {
                xsb->extra.sb_datapt = (paddr32_t *)((char *)buffer + size);
                xsb->extra.sb_data_type = SDI_PHYS_ADDR;
        }
	sdi_xtranslate(jp->j_sb->SCB.sc_dev.pdi_adr_version,
		 jp->j_sb, bp->b_flags, NULL, KM_SLEEP);

	st01docmd(sdi_xsend, jp->j_tp, jp->j_sb, jp, KM_SLEEP);

#ifdef PDI_SVR42
	biowait(bp);
#else
	if( biowait_sig(bp) == B_FALSE ) {
		tp->t_state |= T_ABORTED;			/* LXXX */
		return(EINTR);
	}
#endif

	if (bp->b_error == EBUSY) {
		error = EBUSY;
	}
	else if (bp->b_flags & B_ERROR) {
		error = EIO;
	}
	else {
		error = 0;
	}

	freerbuf(bp);
	return(error);
}

/*
 * st01docmd() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 */
int
st01docmd(fcn, tp, sbp, jp, sleepflag)
int (*fcn)();
struct tape *tp;
struct sb *sbp;
struct job *jp;
int sleepflag;
{
	struct dev_spec *dsp = tp->t_spec;
	int cmd;
	int ret;
	pl_t opri;
	int pdi_adr_version;

/* Work around for MET statistics deficiences: begin */
	/* Only if have a jp will st01comp be called to get stats */
	if (jp && tp->t_stat)
	{
		/* must block interrupts around this call */
		opri=SPL();
		met_ds_queued(tp->t_stat, jp->j_bp->b_flags);
		splx(opri);
	}
/* Work around for MET statistics deficiences: end */

	/* Only queue jobs if we have a job pointer */
	if (jp && tp->t_head && !(tp->t_state & T_RESUMEQ)) {
		opri = SPL();
		tp->t_tail->j_next = jp;
		tp->t_tail = jp;
		jp->j_next = NULL;
		jp->j_func = fcn;
		if (!(tp->t_state & T_SEND)) {
			(void) sdi_timeout(st01resumeq, (caddr_t)tp, 1, pldisk,
				&tp->t_addr);
			tp->t_state |= T_SEND;
		}
		splx(opri);
		return SDI_RET_RETRY;
	}
	if (sbp->sb_type == SFB_TYPE)
		pdi_adr_version = sbp->SFB.sf_dev.pdi_adr_version;
	else
		pdi_adr_version = sbp->SCB.sc_dev.pdi_adr_version;	
	if (dsp && sbp->sb_type != SFB_TYPE) {
		cmd = ((struct scs *)(void *)sbp->SCB.sc_cmdpt)->ss_op;
		if (!CMD_SUP(cmd, dsp)) {
			return SDI_RET_ERR;
		} else if (dsp->command && CMD_CHK(cmd, dsp)) {
			(*dsp->command)(tp, sbp);
		}
	}
	ret = (*fcn)(pdi_adr_version, sbp, sleepflag);
	if (ret == SDI_RET_RETRY) {
		if (!jp) {
			/*
			 *+ Tape driver cannot retry job
			 */
			cmn_err(CE_WARN, "!st01: Cannot retry job");
			return SDI_RET_RETRY;
		}
		opri = SPL();
		if (tp->t_head == NULL)
			tp->t_tail = jp;
		jp->j_next = tp->t_head;
		tp->t_head = jp;
		jp->j_func = fcn;
		if (!(tp->t_state & T_SEND)) {
			(void) sdi_timeout(st01resumeq, (caddr_t)tp, 1, pldisk,
				&tp->t_addr);
			tp->t_state |= T_SEND;
		}
		splx(opri);
		return SDI_RET_RETRY;
	}
	return ret;
}

/*
 * STATIC void
 * st01resumeq()
 *
 * Calling/Exit State:
 *
 * Description:
 *	This function is called by timeout to resume a queue.
 */
STATIC void
st01resumeq(tp)
struct tape *tp;
{
	int ret;

	tp->t_state &= ~T_SEND;
	tp->t_state |= T_RESUMEQ;
	while (tp->t_head) {
		struct job *jp = tp->t_head;
		tp->t_head = tp->t_head->j_next;
		ret = st01docmd(jp->j_func, jp->j_tp, jp->j_sb, jp, KM_NOSLEEP);
		if (ret == SDI_RET_RETRY)
			break;
	}
	tp->t_state &= ~T_RESUMEQ;
}

/*
 * st01reserve() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Reserve a device.
 */
int
st01reserve(tp)
register struct tape *tp;
{
	int ret_val;

	if (tp->t_state & T_RESERVED) {
		return(0);
	}

	while ((ret_val = st01cmd(tp, SS_RESERV, 0, (char *)NULL, 0, 0, SCB_WRITE, 0, 0)) == EBUSY) {
		delay(1 * HZ);
	}

	if( ret_val != 0 ) {
		return( EIO );
	}

	tp->t_state |= T_RESERVED;
	return(0);
}

/*
 * st01release() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Release a device.
 */
int
st01release(tp)
register struct tape *tp;
{
	int ret_val;

	if (! (tp->t_state & T_RESERVED)) {	/* if already released */
		return(0);
	}

	while ((ret_val = st01cmd(tp, SS_RELES, 0, (char *)NULL, 0, 0, SCB_WRITE, 0, 0)) == EBUSY) {
		delay(1 * HZ);
	}

	if( ret_val != 0 ) {
		/*
		 * release cmd returns an error only when device was reserved
		 * and release failed to release the device.  In this case,
		 * the device is still reserved, so don't clear the
		 * T_RESERVED flag.
		 */
		return( EIO );
	}

	tp->t_state &= ~T_RESERVED;
	return(0);
}

/*
 * st01MungeRead()
 *
 * Calling/Exit State:
 *
 * Description:
 *	This routine decreases the size of the passed in user read request
 *	to the maximum amount supported by the HBA. This is a last-ditch
 *	attempt to allow variable-blocked data to be read from a tape
 *	device in an arbitrarily large amount. Without this code the user
 *	process has to know the size of the data block before reading it.
 *	This is not always possible.
 *
 * Side Effects:
 *	The user iovec structure is modified to reflect the maximum transfer
 *	request.
 */
STATIC void
st01MungeRead( uio_t *uio_p, ulong_t max_xfer )
{
	int		cnt, first = 1;
	struct iovec	*iov;
	ulong_t		tot1 = 0;

	/*
	 * Traverse the list of iovecs until we encounter the one
	 * which exceeds the max_xfer size. Update this iov_len field
	 * and zero any subsequent iovecs.
	 */
	
	for ( cnt = 0, iov = uio_p->uio_iov; cnt++ < uio_p->uio_iovcnt; iov++ ){
		if ( tot1 + (ulong_t)iov->iov_len >= max_xfer ) {
			if (first) {
				iov->iov_len = max_xfer - tot1;
				first = 0;
			} else {
				iov->iov_len = 0;
			}
		}
		tot1 += (ulong_t)iov->iov_len;
	}
}

/*
 * st01getcomp( tape unit, return value )
 * -----------
 *
 * 	This routine gets the current compression/decompression status of
 *	the specified unit. This is only called for units which support the
 *	Data Compression Characteristics page (0xF).
 *
 *	Return Values:
 *
 *		0 on success
 *			*comp_status will be set to
 *			000000xx
 *			      ^^
 *			      ||______	compression status (1=>enabled)
 *			      |_______	decompression status (1=>enabled)
 *
 *		non-zero
 *			Error code to be returned to user application
 */
STATIC int
st01getcomp( struct tape *tp, ulong_t *comp_status )
{
	struct mode_dcc_page	*mp;
	struct mode_dcc_data	*mdp;
	int			ret_code = 0;

	/* Get current settings and disable block-descriptor */
	mp = tp->t_dcc;
	while ((ret_code = st01cmd(tp, SS_MSENSE, 0x0F00, mp, 
		sizeof(struct mode_dcc_page), sizeof(struct mode_dcc_page),
		SCB_READ, 8, 0)) == EBUSY) {
		delay(1*HZ);
	}
	if (ret_code)
		return(ret_code);
	
	/* Extract the DCC data from the returned mode-sense block */
	if (mp->h.md_bdl) {
		mdp = &mp->p.md_bd_dcc.md_dcc;
	} else {
		mdp = &mp->p.md_dcc;
	}

	if ( mdp->mp_dcc ) {
		*comp_status  = (mdp->mp_dce) ? 1 : 0;
		*comp_status |= (mdp->mp_dde) ? 2 : 0;
	} else {
		ret_code = EINVAL;
	}
	return ret_code;
}

/*
 * st01setcomp( tape unit, value )
 * -----------
 *
 *	This routine will set the compression and decompression enable bits
 *	in the Data Compression Characteristics page of the Mode Select Data.
 *	The value contents are interpreted as follows:
 *
 *		000000xx
 *		      ^^
 *		      ||________ compression enable (1=>enable)
 *		      |_________ decompression enable (1=>enable)
 *
 *	With Compression enabled all data sent to the drive will be compressed
 *	before being written to the medium. With compression disabled, all data
 *	will be written in an uncompressed format.
 *	Note:   without data compression the capacity of the drive may be
 *		seriously reduced.
 *
 *	With Decompression enabled any compressed entities on the tape will be
 *	automatically decompressed before being returned to the user.
 *	With Decompression disabled the tape contents will be returned to the
 *	user without any decompression being attempted. This may require
 *	software decompression before the data is of any use.
 *
 *	Return Values:
 *
 *		0 on success
 *		non-zero  => error code to be returned to user application
 *
 *	Side Effects:
 *		The tp->t_state field will have T_COMPRESSED set so that we
 *		can restore the compression settings on error.
 */
STATIC int
st01setcomp( struct tape *tp, ulong_t value )
{
	ulong_t		temp1;
	struct mode_dcc_page	*mp;
	struct mode_dcc_data	*mdp;
	int			len;
	int			ret_code = 0;

	/* Grab the current settings - checks that the unit supports DCC */
	if ( st01getcomp(tp, &temp1) ) {
		return( EINVAL );
	}

	mp = tp->t_dcc;
	if (mp->h.md_bdl) {
		mdp = &mp->p.md_bd_dcc.md_dcc;
	} else {
		mdp = &mp->p.md_dcc;
	}

	mdp->mp_dce = (value & 1) ? 1 : 0;	/* Compression Enable */
	mdp->mp_dde = (value & 2) ? 1 : 0;	/* Decompression Enable */
	mdp->mp_r1  = 0;			/* Clear PS bit */

	len = mp->h.md_len + 1;		/* Mode Data length */

	mp->h.md_len   	= 0;
	mp->h.md_media 	= 0;
	mp->h.md_wp	= 0;

	while ((ret_code = st01cmd(tp, SS_MSELECT, 0, mp, 
		len, len,
		SCB_WRITE, 0x10, 0)) == EBUSY) {
		delay(1*HZ);
	}
	if ( ret_code == 0 ) {
		tp->t_state |= T_COMPRESSED;
	}
	return ret_code;
}

/*
 * st01retryread()
 *
 * Calling/Exit State:
 *	Called from st01sense at interrupt time. Cannot sleep.
 *
 * Description:
 *	This routine starts the read recovery chain to allow variably blocked
 *	data which is larger than the user request to be successfully read and
 *	returned to user space.
 *
 *	1. Check that actual data size will fit into aligned buffer
 *	2. Build a SPACE -1 BLOCK command to position tape before large block
 *	3. Issue command	(completion occurs in st01intsc)
 *
 * Side Effects:
 *	the t_fltcmd SCSI block will be overwritten with the SPACE command
 */
STATIC void
st01retryread( struct tape *tp )
{
	struct buf	*bp;

	bp = tp->t_fltjob->j_bp;	/* Original request */

	if ( bp->b_bcount - bp->b_resid > tp->t_abcbp->bcb_max_xfer ) {
		tp->t_state &= ~T_SHORT_READ;
#if DEBUG
		cmn_err(CE_WARN, 
		"Tape driver: Requested %d block, Maximum size = %d\n",
		bp->b_bcount - bp->b_resid, tp->t_abcbp->bcb_max_xfer);
#endif
		bp->b_resid = bp->b_bcount;
		st01logerr(tp, tp->t_fltjob->j_sb);
		st01comp(tp->t_fltjob);
		return;
	}
	/*
	 * Transfer will fit into aligned buffer. Now we can build the
	 * SPACE -1 BLOCKS command and send it to the HBA. Next step will
	 * be st01intsc. Hijack the tp->t_fltcmd SCB for the actual command.
	 */
	
	tp->t_fltcmd.ss_op    = SS_SPACE;
	tp->t_fltcmd.ss_addr1 = BLOCKS;
	tp->t_fltcmd.ss_lun   = tp->t_addr.sa_lun;
	tp->t_fltcmd.ss_addr  = 0xFFFF;
	tp->t_fltcmd.ss_len   = 0xFF;
	tp->t_fltcmd.ss_cont  = 0;

	tp->t_fltread->SCB.sc_time   = 180 * ONE_MIN;
	tp->t_fltread->SCB.sc_mode   = SCB_READ;
	tp->t_fltread->SCB.sc_datasz = 0;
	tp->t_fltread->SCB.sc_int    = st01intsc;

#if DEBUG
	cmn_err( CE_CONT, "st01retryread: Space -1 blocks...\n");
#endif

	if ( st01docmd (sdi_xicmd, tp, tp->t_fltread, NULL, KM_NOSLEEP) !=
		SDI_RET_OK ) {
#if DEBUG
		cmn_err(CE_WARN, "st01retryread: Space command failed!!\n");
#endif

		/* Fail the original job */
		bp->b_resid = bp->b_bcount;
		tp->t_state &= ~T_SHORT_READ;
		st01logerr(tp, tp->t_fltjob->j_sb);
		st01comp(tp->t_fltjob);
	}
	return;
}

/*
 * st01intsc()
 *
 * Calling/Exit State:
 *	Called at interrupt time from the HBA 
 *
 * Description:
 *	This routine is called when the SPACE -1 BLOCK command has completed.
 *	If successful, we can now stage the READ command for the correct
 *	block size.
 *
 *	1. Check for successful completion
 *	2. Build READ command, destination already setup in t_fltread
 *	3. Issue command	(completion occurs in st01intrrc)
 *
 * Side Effects:
 *	the t_fltcmd SCSI block will be overwritten with the READ command
 */
STATIC void
st01intsc( struct sb *sp )
{
	register struct tape *tp;
	register struct buf  *bp;
	long		     blksize;

#if DEBUG
	cmn_err(CE_CONT, "st01intsc:\n");
#endif

	tp = (struct tape *)sp->SCB.sc_wd;
	bp = tp->t_fltjob->j_bp;		/* Original request */

	if ( sp->SCB.sc_comp_code != SDI_ASW ) {
		/*
		 * Space failed - we'll just trash the original request
		 * as the Sense info is academic
		 */
#if DEBUG
		cmn_err(CE_CONT, "st01intsc: Completion code = 0x%x\n",
			sp->SCB.sc_comp_code);
#endif

		bp->b_resid = bp->b_bcount;	/* 0 bytes transferred */
		st01logerr(tp, sp);
		st01comp(tp->t_fltjob);
		return;
	}

	/*
	 * Note: we have already sdi_translate'd the maximum transfer size
	 * in st01open. This means we can just decrease the size fields as
	 * necessary and throw the command at the HBA. We hijack the
	 * t_fltcmd to hold the actual READ SCB.
	 */
	blksize = bp->b_bcount - bp->b_resid;
	tp->t_fltcmd.ss_op = SS_READ;
	tp->t_fltcmd.ss_lun = tp->t_addr.sa_lun;
	tp->t_fltcmd.ss_mode = VARIABLE;
	tp->t_fltcmd.ss_len1 = mdbyte(blksize) << 8 | msbyte(blksize);
	tp->t_fltcmd.ss_len2 = lsbyte(blksize);
	tp->t_fltcmd.ss_cont = 0;
	
	tp->t_fltread->SCB.sc_int    = st01intrrc;
	tp->t_fltread->SCB.sc_datasz = blksize;
	tp->t_fltread->SCB.sc_time   = 180 * ONE_MIN;

	if ( st01docmd(sdi_xicmd, tp, tp->t_fltread, NULL, KM_NOSLEEP) !=
		SDI_RET_OK ) {
		/* Fail the job */
		bp->b_resid = bp->b_bcount;
		tp->t_state &= ~T_SHORT_READ;
		st01logerr(tp, sp);
		st01comp(tp->t_fltjob);
		return;
	}
}

/*
 * st01intrrc()
 *
 * Calling/Exit State:
 *	Called at interrupt time from the HBA
 *
 * Description:
 *	This routine is called when the READ command has completed. The data
 *	should now be present in the aligned buffer and can be copied out to
 *	the user buffer if necessary.
 *	Once the copy has been done, the kernel buffer residual count must be
 *	updated to reflect that there is still some data left which will be
 *	passed to the user on the next read() request.
 *
 *	1. Check for successful completion
 *	2. Copy data to user buffer (if appropriate)
 *	3. Update uiop fields for extra data amount
 *	4. Complete the original (much-delayed) user request
 */
STATIC void
st01intrrc( struct sb *sp )
{
	register struct tape	*tp;
	register struct job	*jp;
	register struct buf	*bp;
	long			resid;

#if DEBUG
	cmn_err(CE_CONT, "st01intrrc:\n");
#endif

	tp = (struct tape *)sp->SCB.sc_wd;
	jp = tp->t_fltjob;			/* Original job */
	bp = jp->j_bp;				/* Original request */

	resid = -1 * bp->b_resid;		/* Amount of overrun */

	if ( sp->SCB.sc_comp_code != SDI_ASW ) {
		/*
		 * Command didn't complete successfully. Check to see if
		 * we hit a filemark in which case we have valid data to
		 * transfer
		 * Note: this is very similar to the st01intn() code path
		 */
#if DEBUG
		cmn_err(CE_CONT, "st01intrrc: comp_code = 0x%x, status=0x%x\n",
			sp->SCB.sc_comp_code, sp->SCB.sc_status );
#endif

		if ( sp->SCB.sc_comp_code == SDI_CKSTAT &&
		     sp->SCB.sc_status == S_CKCON ) {
			struct sense *sensep = sdi_sense_ptr(sp);

			if ((sensep->sd_key != SD_NOSENSE) || sensep->sd_fm ||
			     sensep->sd_eom || sensep->sd_ili ) {
				bcopy(sensep,tp->t_sense,sizeof(struct sense));
				if ( st01sense(tp, 0) ) {
					/*
					 * No error => data available
					 */
					if ( bp->b_un.b_addr != tp->t_abuff ) {
						/*
						 * Map buffer into kernel virtual
						 */
						bp_mapin(bp);
						bcopy(tp->t_abuff, 
					 	      bp->b_un.b_addr, 
						      bp->b_bcount);
						tp->t_iovec.iov_base   = 
						 (caddr_t)tp->t_abuff + bp->b_bcount;
						tp->t_iovec.iov_len    = 
						 bp->b_bcount + resid;
						tp->t_uiop->uio_iov    = 
						 &tp->t_iovec;
						tp->t_uiop->uio_iovcnt = 1;
						tp->t_uiop->uio_offset = 0;
					}
					tp->t_uiop->uio_offset += resid;
					tp->t_iovec.iov_base   += resid;
				}
				tp->t_state &= ~T_SHORT_READ;
				bp->b_resid = 0;
				jp->j_sb->SCB.sc_comp_code = SDI_ASW;
				st01comp(jp);
				return;
			}
			/*
			 * Now the unpleasant bit...we have to stage a SENSE
			 * command and then handle it's completion etc, etc
			 */
			st01dosense(tp, sp, st01intrq1);
			return;
		} else {
			bp->b_resid = bp->b_bcount;
			tp->t_state &= ~T_SHORT_READ;
			st01logerr(tp, sp);
			st01comp(jp);
			return;
		}
	} else {
		/*
		 * READ succeeded. The data is now available (in tp->t_abuff)
		 * to be copied to the original destination (bp->b_un.b_addr)
		 * After the copy, we must update the uio_offset field in the
		 * tp->t_uiop so that subsequent read()s will use this data
		 * before accessing the device.
		 */
#if DEBUG
		cmn_err( CE_CONT, "st01intrrc: Read OK, xfering %d bytes\n",
			bp->b_bcount );
		cmn_err( CE_CONT, "Address = 0x%x, Resid = %d, b_resid = %d\n",
			bp->b_un.b_addr, resid, bp->b_resid );
		cmn_err( CE_CONT, "Error = %d\n", bp->b_error );
#endif

		if ( bp->b_un.b_addr != tp->t_abuff ) {
			/*
			 * Map buffer into kernel virtual
			 */
			bp_mapin(bp);
			bcopy(tp->t_abuff, bp->b_un.b_addr, bp->b_bcount);
			tp->t_iovec.iov_base   = (caddr_t)tp->t_abuff + bp->b_bcount;
			tp->t_iovec.iov_len    = bp->b_bcount + resid;
			tp->t_uiop->uio_iov    = &tp->t_iovec;
			tp->t_uiop->uio_iovcnt = 1;
			tp->t_uiop->uio_offset = 0;
		}
		bp->b_resid = 0;
		tp->t_uiop->uio_offset += resid;
		tp->t_iovec.iov_base   += resid;
		tp->t_state &=~T_SHORT_READ;
		jp->j_sb->SCB.sc_comp_code = SDI_ASW;
		st01comp(jp);
		return;
	}
}

/*
 * st01dosense()
 *
 * Calling/Exit States:
 *	Called from interrupt time
 *
 * Description:
 *	This routine fills in the necessary fields in the tp to stage a
 *	REQUEST SENSE command to the device.
 *	On completion of the command, the specified <rtn> will be called
 *
 * Side Effects:
 *	If the SENSE command cannot be issued, fail the requesting job
 *	(tp->t_fltjob). In this case, the b_resid field will be set to the
 *	original b_bcount value (i.e. no data will be returned to user).
 */
STATIC void
st01dosense( struct tape *tp, struct sb *sp, void (*rtn)() )
{
	struct buf	*bp;

	bp = tp->t_fltjob->j_bp;	/* Original request */

	tp->t_fltreq->sb_type 	   	= ISCB_TYPE;
	tp->t_fltreq->SCB.sc_int   	= rtn;
	tp->t_fltreq->SCB.sc_cmdsz 	= SCS_SZ;
	tp->t_fltreq->SCB.sc_time  	= JTIME;
	tp->t_fltreq->SCB.sc_mode  	= SCB_READ;
	tp->t_fltreq->SCB.sc_dev   	= sp->SCB.sc_dev;
	tp->t_fltreq->SCB.sc_wd	   	= (long)tp;
	tp->t_fltreq->SCB.sc_datapt	= SENSE_AD(tp->t_sense);
	tp->t_fltreq->SCB.sc_datasz	= SENSE_SZ;
	tp->t_fltcmd.ss_op		= SS_REQSEN;
	tp->t_fltcmd.ss_lun		= sp->SCB.sc_dev.sa_lun;
	tp->t_fltcmd.ss_addr1		= 0;
	tp->t_fltcmd.ss_addr		= 0;
	tp->t_fltcmd.ss_len		= SENSE_SZ;
	tp->t_fltcmd.ss_cont		= 0;

	tp->t_sense->sd_key = SD_NOSENSE;

	if ( st01docmd (sdi_xicmd, tp, tp->t_fltreq, NULL, KM_NOSLEEP) !=
		SDI_RET_OK ) {
		/* Fail the job */
		tp->t_state &= ~T_SHORT_READ;
		bp->b_resid = bp->b_bcount;
		st01logerr(tp, sp);
		st01comp(tp->t_fltjob);
	}
	return;
}

/*
 * st01intrq1()
 *
 * Calling/Exit State:
 *	Called on completion of a REQUEST_SENSE command as part of error
 *	recovery during a short length ILI read recovery sequence.
 *	Called at interrupt time from HBA
 *
 * Description:
 *	This routine handles the completion of a REQUEST_SENSE command that
 *	was issued during a recovery sequence for a short length ILI read.
 *	Normal situation would be that user requested a short block on the
 *	last data block of the tape. Performing the recovery would cause a
 *	FileMark to be encountered when the restaged READ completed. This
 *	information has to be recorded in the tp->t_state field.
 */
STATIC void
st01intrq1( struct sb *sp )
{
	register struct tape	*tp;
	register struct buf	*bp;
	long 			resid;

	tp = (struct tape *)sp->SCB.sc_wd;
	bp = tp->t_fltjob->j_bp;		/* Original request */
	resid = -1 * bp->b_resid;		/* Extra data */

	if ( sp->SCB.sc_comp_code != SDI_ASW ) {
		bp->b_resid = bp->b_bcount;	/* 0 bytes transferred */
		tp->t_state &= ~T_SHORT_READ;
		st01logerr(tp, sp);
		st01comp(tp->t_fltjob);
		return;
	} 
	/*
	 * Check to see if there is any data available to transfer
	 */
	if ( st01sense(tp, 0) ) {
		/*
		 * No error => data available
		 */
		if ( bp->b_un.b_addr != tp->t_abuff ) {
			/*
			 * Map buffer into kernel virtual
			 */
			bp_mapin(bp);
			bcopy(tp->t_abuff, bp->b_un.b_addr, bp->b_bcount);
			tp->t_iovec.iov_base   = (caddr_t)tp->t_abuff + bp->b_bcount;
			tp->t_iovec.iov_len    = bp->b_bcount + resid;
			tp->t_uiop->uio_iov    = &tp->t_iovec;
			tp->t_uiop->uio_iovcnt = 1;
			tp->t_uiop->uio_offset = 0;
		}
		tp->t_uiop->uio_offset += resid;
		tp->t_iovec.iov_base   += resid;
	}
	tp->t_state &= ~T_SHORT_READ;
	bp->b_resid = 0;
	tp->t_fltjob->j_sb->SCB.sc_comp_code = SDI_ASW;
	st01comp(tp->t_fltjob);
	return;
}
