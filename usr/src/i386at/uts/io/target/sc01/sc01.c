#ident	"@(#)kern-pdi:io/target/sc01/sc01.c	1.55.6.4"

/*	Copyright (c) 1988, 1989  Intel Corporation	*/
/*	All Rights Reserved	*/

/*      INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied under the terms of a license agreement   */
/*	or nondisclosure agreement with Intel Corporation and may not be   */
/*	copied or disclosed except in accordance with the terms of that    */
/*	agreement.							   */

/*	Copyright (c) 1989 TOSHIBA CORPORATION		*/
/*	All Rights Reserved	*/

/*	Copyright (c) 1989 SORD COMPUTER CORPORATION	*/
/*	All Rights Reserved	*/


/*
**	SCSI CD-ROM Target Driver.
*/

#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <mem/kmem.h>
#include <svc/systm.h>
#include <io/open.h>
#include <io/ioctl.h>
#include <fs/file.h>
#include <util/debug.h>
#include <io/conf.h>
#include <io/uio.h>
#include <io/target/scsi.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi_hier.h>
#include <io/target/sdi/sdi.h>
#include <io/target/sc01/cd_ioctl.h>
#include <io/target/sdi/dynstructs.h>
#include <io/target/sc01/sc01.h>
#include <util/mod/moddefs.h>
#include <io/ddi.h>

#define DRVNAME         "sc01 - CD-ROM target driver"
#define SC01_TMPSIZE	1024

void	sc01logerr(), sc01io(), sc01comp(), sc01sense(), sc01send(),
	sc01rinit(), sc01strat0(); 
int	sc01start(), SC01ICMD();
int	sc01config(), sc01cmd(), sc01docmd(), SC01SEND(), sc01_cdinit();
void     sc01_free_locks();

/* Allocated in space.c */
extern	long	 	Sc01_bmajor;	/* Block major number	    */
extern	long	 	Sc01_cmajor;	/* Character major number   */
extern  struct head	lg_poolhead;	/* head for dyn struct routines */

static	int 		sc01_cdromcnt;	/* Num of cdroms configured  */
static	struct cdrom   *sc01_cdrom;	/* Array of cdrom structures */
static	caddr_t		sc01_tmp;	/* Pointer to temporary buffer*/
static	int		sc01_tmp_flag;	/* sc01_tmp control flag    */
static  struct owner   *sc01_ownerlist;	/* list of owner structs    */

#ifndef PDI_SVR42
int	sc01devflag	= D_NOBRKUP | D_BLKOFF;
#else /* PDI_SVR42 */
int	sc01devflag	= D_NOBRKUP;
#endif /* PDI_SVR42 */

static  int     sc01_dynamic = 0;
static  size_t  mod_memsz = 0;
static  int	rinit_flag = 0;

STATIC LKINFO_DECL( cd_lkinfo, "IO:sc01:cd_lkinfo", 0);
STATIC LKINFO_DECL( sc01_lkinfo, "IO:sc01:sc01_lkinfo", 0);
STATIC LKINFO_DECL( sc01_flag_lkinfo, "IO:sc01:sc01_flag_lkinfo", 0);

static lock_t	*sc01_lock=NULL;
static pl_t     sc01_pl;
static sv_t	*sc01_sv;

static lock_t	*sc01_flag_lock=NULL;
static pl_t     sc01_flag_pl;
static sv_t	*sc01_flag_sv;

static int	sc01_lock_flag = 0;

#define SC01_LOCK(sc01_lock)	sc01_pl = LOCK(sc01_lock, pldisk)
#define SC01_UNLOCK(sc01_lock)  UNLOCK(sc01_lock, sc01_pl)

#define CD_LOCK(cdp) do {                                               \
				SC01_LOCK(sc01_lock);	                \
				if (sc01_lock_flag == 0) {               \
				  sc01_lock_flag = 1;                   \
				  SC01_UNLOCK(sc01_lock);               \
				  cdp->cd_pl  = 		        \
					LOCK(cdp->cd_lock, pldisk);     \
				  break; 			        \
	 		        }				        \
			        SC01_UNLOCK(sc01_lock);                 \
			        delay(1);			        \
			} while (1)

#define CD_UNLOCK(cdp) do {                                             \
				SC01_LOCK(sc01_lock);                   \
				sc01_lock_flag = 0;	                \
				SC01_UNLOCK(sc01_lock);                 \
			        UNLOCK(cdp->cd_lock, cdp->cd_pl);       \
			} while (0)


#define SC01_FLAG_LOCK(sc01_flag_lock)                                  \
			 do {                                           \
				SC01_LOCK(sc01_lock);	                \
				if (sc01_lock_flag == 0) {               \
				  sc01_lock_flag = 1;                   \
				  SC01_UNLOCK(sc01_lock);               \
				  sc01_flag_pl =                        \
					LOCK(sc01_flag_lock, pldisk);   \
				  break; 			        \
	 		        }				        \
			        SC01_UNLOCK(sc01_lock);                 \
			        delay(1);			        \
			} while (1)

#define SC01_FLAG_UNLOCK(sc01_flag_lock)                                \
		       do {                                             \
				SC01_LOCK(sc01_lock);                   \
				sc01_lock_flag = 0;	                \
				SC01_UNLOCK(sc01_lock);                 \
			        UNLOCK(sc01_flag_lock, sc01_flag_pl);   \
			} while (0)


STATIC  int     sc01_load(), sc01_unload();
MOD_DRV_WRAPPER(sc01, sc01_load, sc01_unload, NULL, DRVNAME);


/*
 * int sc01_load()
 *
 * Calling/Exit State:
 *
 * Descriptions: loading the sc01 module
 */
STATIC  int
sc01_load()
{
        sc01_dynamic = 1;
        if(sc01start()) {
		sdi_clrconfig(sc01_ownerlist,SDI_REMOVE|SDI_DISCLAIM, sc01rinit);
		return(ENODEV);
	}
        return(0);
}

/*
 * sc01_unload() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 */
STATIC  int
sc01_unload()
{
	ulong paddr_size;
	sdi_clrconfig(sc01_ownerlist,SDI_REMOVE|SDI_DISCLAIM, sc01rinit);

        if(mod_memsz > (size_t )0)       {
		int i;
		struct cdrom *cdp;

		for (i=0, cdp = sc01_cdrom; i<sc01_cdromcnt; i++, cdp++) {
			paddr_size = 
				SDI_PADDR_SIZE(cdp->cd_bcbp->bcb_physreqp);
			sdi_freebcb(cdp->cd_bcbp);
			if (cdp->cd_lock)
				LOCK_DEALLOC(cdp->cd_lock);
			if (cdp->cd_sv)
				SV_DEALLOC(cdp->cd_sv);
			if (cdp->cd_sense)
				kmem_free((caddr_t)cdp->cd_sense, 
					sizeof(struct sense) + paddr_size);
			if (cdp->cd_capacity)
				kmem_free((caddr_t)cdp->cd_capacity, 
					sizeof(struct capacity) + paddr_size);
		}
		sc01_ownerlist = NULL;
		sc01_free_locks();
                kmem_free((caddr_t)sc01_cdrom, mod_memsz);
		sc01_cdrom = NULL;
        }
        return(0);
}

#define	ARG	a0,a1,a2,a3,a4,a5

void	sc01sendt();
void	sc01intn();
void	sc01intf();
void	sc01intrq();
void	sc01strategy();


#ifdef DEBUG
#define SIZE    10
daddr_t 	sc01_Offset = 0;
static char     sc01_Debug[SIZE] = {0,0,0,0,0,0,0,0,0,0};
#define DPR(l)          if (sc01_Debug[l]) cmn_err
#endif

/*
 * int
 * sc01start() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	Called by kernel to perform driver initialization.
 *	This function does not access any devices.
 */
int
sc01start()
{
	register struct cdrom  *cdp;	/* cdrom pointer	 */
	struct	owner	*op;
	struct drv_majors drv_maj;
	caddr_t	 base;			/* Base memory pointer	 */
	int  cdromsz,			/* cdrom size (in bytes) */
	     tc;			/* TC number		 */
	int sleepflag;


	sleepflag = sc01_dynamic ? KM_SLEEP : KM_NOSLEEP;

	if ( !(sc01_lock = LOCK_ALLOC(TARGET_HIER_BASE+1, pldisk,
		&sc01_lkinfo, sdi_sleepflag)) ||
	       !(sc01_sv = SV_ALLOC(sdi_sleepflag)) ) { 

		cmn_err(CE_WARN, "sc01: sc01start():  allocation failed\n");
		return(0);
	}

	if ( !(sc01_flag_lock = LOCK_ALLOC(TARGET_HIER_BASE, pldisk,
		&sc01_flag_lkinfo, sdi_sleepflag)) ||
	       !(sc01_flag_sv = SV_ALLOC(sdi_sleepflag)) ) { 

		cmn_err(CE_WARN, "sc01: sc01start():  allocation failed\n");
		return(0);
	}

	sc01_cdromcnt = 0;
	drv_maj.b_maj = Sc01_bmajor;
	drv_maj.c_maj = Sc01_cmajor;
	drv_maj.minors_per = CD_MINORS_PER;
	drv_maj.first_minor = 0;
	sc01_ownerlist = sdi_doconfig(SC01_dev_cfg, SC01_dev_cfg_size,
			"SC01 CDROM Driver", &drv_maj, sc01rinit);
	sc01_cdromcnt = 0;
	for (op = sc01_ownerlist; op; op = op->target_link) {
		sc01_cdromcnt++;
	}

#ifdef DEBUG
	cmn_err(CE_CONT, "%d cdroms claimed\n", sc01_cdromcnt);
#endif

	/* Check if there are devices configured */
	if (sc01_cdromcnt == 0) {
		sc01_cdromcnt = CDNOTCS;
		return(1);
	}

	/*
	 * Allocate the cdrom and job structures
	 */
	cdromsz = sc01_cdromcnt * sizeof(struct cdrom);
	mod_memsz = cdromsz;
        if ((base = (caddr_t)kmem_zalloc(mod_memsz, sleepflag)) == 0) {
		/*
		 *+ There was insuffcient memory for cdrom data
		 *+ structures at load-time.
		 */
		cmn_err(CE_WARN,
			"CD-ROM Error: Insufficient memory to configure driver");
		cmn_err(CE_CONT,"!Could not allocate 0x%x bytes of memory\n",
			mod_memsz);
		sc01_cdromcnt = CDNOTCS;
		mod_memsz = 0;
		sc01_free_locks();
		return(1);
	}

	sc01_cdrom = (struct cdrom *)(void *) base;
	/*
	 * Initialize the cdrom structures
	 */
	cdp = sc01_cdrom;
	for(tc = 0, op = sc01_ownerlist; tc < sc01_cdromcnt;
			tc++, op=op->target_link, cdp++) {

		if (sc01_cdinit(cdp, op, sleepflag)) {
			if(tc == 0) {
        			kmem_free(sc01_cdrom, mod_memsz);
				sc01_free_locks();
				sc01_cdromcnt = CDNOTCS;
				mod_memsz = 0;
				return(1);
			}
			return(0);
		}
	}
	return(0);
}

/*
 * int
 * sc01_cdinit(struct cdrom *cdp, struct owner *op, int sleepflag) 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	Called by sc01start/sc01rinit to perform cdp initialization
 *	for a new cdrom device.
 *	This function does not access any devices.
 */
int
sc01_cdinit(struct cdrom *cdp, struct owner *op, int sleepflag) 
{

	if ( !(cdp->cd_lock = LOCK_ALLOC(TARGET_HIER_BASE, pldisk,
			&cd_lkinfo, sdi_sleepflag)) ||
	     !(cdp->cd_sv = SV_ALLOC(sdi_sleepflag)) ) {

		cmn_err(CE_WARN, "sc01: cdinit allocation failed\n");
		return(0);
	}

	/* Initialize the queue ptrs */
	cdp->cd_first = (struct job *) cdp;
	cdp->cd_last  = (struct job *) cdp;
	cdp->cd_next  = (struct job *) cdp;
	cdp->cd_batch = (struct job *) cdp;

#ifdef DEBUG
	cmn_err(CE_CONT, "cdrom: op 0x%x ", op);
	cmn_err(CE_CONT, "edt 0x%x ", op->edtp);
	cmn_err(CE_CONT,
		"hba %d scsi id %d lun %d bus %d\n",
		op->edtp->scsi_adr.scsi_ctl,op->edtp->scsi_adr.scsi_target,
		op->edtp->scsi_adr.scsi_lun,op->edtp->scsi_adr.scsi_bus);
#endif
	/*
         * Allocate a scsi_extended_adr structure
         */
        cdp->cd_addr.extended_address =
                (struct sdi_extended_adr *)kmem_zalloc(sizeof(struct sdi_extended_adr), sdi_sleepflag);

        if ( ! cdp->cd_addr.extended_address ) {
                cmn_err(CE_WARN, "cdrom driver: insufficient memory"
                                        "to allocate extended address");
                return(0);
        }

        cdp->cd_addr.pdi_adr_version =
                sdi_ext_address(op->edtp->scsi_adr.scsi_ctl);

	/* Initialize the SCSI address */
	if (! cdp->cd_addr.pdi_adr_version ) {
		cdp->cd_addr.sa_lun = op->edtp->scsi_adr.scsi_lun;
		cdp->cd_addr.sa_bus = op->edtp->scsi_adr.scsi_bus;
		cdp->cd_addr.sa_ct  = SDI_SA_CT(op->edtp->scsi_adr.scsi_ctl,
					op->edtp->scsi_adr.scsi_target);
		cdp->cd_addr.sa_exta= (uchar_t)(op->edtp->scsi_adr.scsi_target);
	}
	else
		cdp->cd_addr.extended_address->scsi_adr =
                        op->edtp->scsi_adr;

	cdp->cd_addr.extended_address->version = SET_IN_TARGET;
	cdp->cd_spec	    = sdi_findspec(op->edtp, sc01_dev_spec);
	cdp->cd_iotype = op->edtp->iotype;

	/*
	 * Call to initialize the breakup control block.
	 */
	if ((cdp->cd_bcbp = sdi_xgetbcb(cdp->cd_addr.pdi_adr_version,
			&cdp->cd_addr, sleepflag)) == NULL) {
		/*
		 *+ CD-ROM Driver:
		 *+ Insufficient memory to allocate disk breakup control
		 *+ block data structure when loading driver.
		 */
 		cmn_err(CE_NOTE, "CD-ROM Error: Insufficient memory to allocate breakup control block.");
		return (1);
	}

	cdp->cd_sense = 
		(struct sense *)sdi_kmem_alloc_phys(sizeof(struct sense),
		cdp->cd_bcbp->bcb_physreqp, 0);

	if (cdp->cd_sense == NULL) {
		cmn_err(CE_WARN, "CD-ROM driver: Insufficient memory to allocate sense block.");
		return(1);
	}
	
	cdp->cd_capacity = 
		(struct capacity *)sdi_kmem_alloc_phys(sizeof(struct capacity),
		cdp->cd_bcbp->bcb_physreqp, 0);

	if (cdp->cd_capacity == NULL) {
		cmn_err(CE_WARN, "CD-ROM driver: Insufficient memory to allocate capacity block.");
		return(1);
	}
	if (!(cdp->cd_bcbp->bcb_flags & BCB_PHYSCONTIG) &&
	    !(cdp->cd_bcbp->bcb_addrtypes & BA_SCGTH) &&
	    !(cdp->cd_iotype & F_PIO)) {
		/* Set parameters for drivers that are not using buf_breakup
		 * to break at every page, and not using the BA_SCGTH
		 * feature of buf_breakup, and not programmed I/O.
		 * (e.g. HBAs that are still doing there own scatter-
		 * gather lists.)
		 */
		if (cdp->cd_bcbp->bcb_max_xfer > ptob(1))
			cdp->cd_bcbp->bcb_max_xfer -= ptob(1);
	}
	return(0);
}

/*
 * sc01rinit() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	Called by sdi to perform driver initialization of additional
 *	devices found after the dynamic load of HBA drivers. This 
 *	routine is called only when sc01 is a static driver.
 *	This function does not access any devices.
 */
void
sc01rinit()
{
	register struct cdrom  *cdp, *ocdp;	/* cdrom pointer	 */
	struct	owner	*ohp, *op;
	struct drv_majors drv_maj;
	caddr_t	 base;			/* Base memory pointer	 */
	pl_t prevpl;                    /* prev process level for splx */
	int  cdromsz,			/* cdrom size (in bytes) */
	     new_cdromcnt,		/* number of additional devs found*/
	     ocdromcnt,			/* number of devs previously found*/
	     tmpcnt,			/* temp count of devs */
	     found,			/* seach flag variable */
	     romcnt,			/* cdrom instance	*/
	     sleepflag;			/* can sleep or not	*/
	int  match = 0;


	/* set rinit_flag to prevent any access to sc01_cdrom while were */
	/* updating it for new devices and copying existing devices      */
	SC01_LOCK(sc01_lock);
	rinit_flag = 1;
	SC01_UNLOCK(sc01_lock);	
	new_cdromcnt= 0;
	drv_maj.b_maj = Sc01_bmajor;
	drv_maj.c_maj = Sc01_cmajor;
	drv_maj.minors_per = CD_MINORS_PER;
	drv_maj.first_minor = 0;
	sleepflag = sc01_dynamic ? KM_SLEEP : KM_NOSLEEP;

	/* call sdi_doconfig with NULL func so we don't get called again */
	ohp = sdi_doconfig(SC01_dev_cfg, SC01_dev_cfg_size,
				"SC01 CDROM Driver", &drv_maj, NULL);
	for (op = ohp; op; op = op->target_link) {
		new_cdromcnt++;
	}
#ifdef DEBUG
	cmn_err(CE_CONT, "sc01rinit %d cdroms claimed\n", new_cdromcnt);
#endif
	/* Check if there are additional devices configured */
	if ((new_cdromcnt == sc01_cdromcnt) || (new_cdromcnt == 0)) {
		
		SC01_LOCK(sc01_lock);
		rinit_flag = 0;
		SC01_UNLOCK(sc01_lock);
		SV_BROADCAST(sc01_sv, 0);
		return;
	}
	/*
	 * Allocate the cdrom structures
	 */
	cdromsz = new_cdromcnt * sizeof(struct cdrom);
        if ((base = kmem_zalloc(cdromsz, KM_NOSLEEP)) == NULL) {
		/*
		 *+ There was insuffcient memory for cdrom data
		 *+ structures at load-time.
		 */
		cmn_err(CE_WARN,
			"CD-ROM Error: Insufficient memory to configure driver");
		cmn_err(CE_CONT,
			"!Could not allocate 0x%x bytes of memory\n",cdromsz);
		SC01_LOCK(sc01_lock);
		rinit_flag = 0;
		SC01_UNLOCK(sc01_lock);
		SV_BROADCAST(sc01_sv, 0);
		sc01_free_locks();
		return;
	}
	/*
	 * Initialize the cdrom structures
	 */
	if(sc01_cdromcnt == CDNOTCS)
		ocdromcnt = 0;
	else
		ocdromcnt = sc01_cdromcnt;
	found = 0;
	prevpl = spldisk();
	for(cdp = (struct cdrom *)(void *)base, romcnt = 0, op = ohp; 
	    romcnt < new_cdromcnt;
	    romcnt++, op=op->target_link, cdp++) {

		/* Initialize new cdrom structs by copying existing cdrom */
		/* structs into new struct and initializing new instances */
		match = 0;
		if (ocdromcnt) {
			for (ocdp = sc01_cdrom, tmpcnt = 0; 
			     tmpcnt < sc01_cdromcnt; ocdp++,tmpcnt++) {
				if (! ocdp->cd_addr.pdi_adr_version)
					match =
						SDI_ADDRCMP(&ocdp->cd_addr,
							&op->edtp->scsi_adr);
				else
					match = SDI_ADDR_MATCH(&(ocdp->cd_addr.extended_address->scsi_adr), &op->edtp->scsi_adr);
				if (match) {
					found = 1;
					break;
				}
			}
			if (found) { /* copy ocdp to cdp */
				*cdp = *ocdp;
				if (ocdp->cd_next == (struct job *)ocdp) {
					cdp->cd_first = (struct job *)cdp;
					cdp->cd_last  = (struct job *)cdp;
					cdp->cd_next  = (struct job *)cdp;
					cdp->cd_batch = (struct job *)cdp;
				}
				else {
					cdp->cd_last->j_next = (struct job *)cdp;
					cdp->cd_next->j_prev = (struct job *)cdp;
					if (ocdp->cd_batch == (struct job *)ocdp)
						cdp->cd_batch = (struct job *)cdp;
				}
				found = 0;
				ocdromcnt--;
				continue;
			}
		}
		if (sc01_cdinit(cdp, op, sleepflag)) {
			/*
			 *+ There was insuffcient memory for cdrom data
			 *+ structures at load-time.
			 */
			cmn_err(CE_WARN,
			"CD-ROM Error: Insufficient memory to configure driver");
			kmem_free(base, cdromsz);
			splx(prevpl);
			SC01_LOCK(sc01_lock);
			rinit_flag = 0;
			SC01_UNLOCK(sc01_lock);
			SV_BROADCAST(sc01_sv, 0);
			return;
		}
	}
	if (sc01_cdromcnt > 0)
		kmem_free(sc01_cdrom, mod_memsz);
	sc01_cdromcnt = new_cdromcnt;
	sc01_cdrom = (struct cdrom *)(void *)base;
	sc01_ownerlist = ohp;
	mod_memsz = cdromsz;
	splx(prevpl);
	SC01_LOCK(sc01_lock);
	rinit_flag = 0;
	SC01_UNLOCK(sc01_lock);
	SV_BROADCAST(sc01_sv, 0);
}

#ifndef PDI_SVR42
/*
 * int
 * sc01devinfo(dev_t dev, di_parm_t parm, void **valp)
 *	Get device information.
 *	
 * Calling/Exit State:
 *	The device must already be open.
 */
int
sc01devinfo(dev_t dev, di_parm_t parm, void **valp)
{
	struct cdrom	*cdp;
	unsigned long	unit;
	
	unit = UNIT(dev);

	/* Check for non-existent device */
	if ((int)unit >= sc01_cdromcnt)
		return(ENODEV);

	cdp = &sc01_cdrom[unit];

	switch (parm) {
		case DI_BCBP:
			*(bcb_t **)valp = cdp->cd_bcbp;
			return 0;
		case DI_MEDIA:
			*(char **)valp = "cd-rom";
			return 0;
		default:
			return ENOSYS;
	}
}
#endif

/*
 * sc01getjob() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function will allocate a cdrom job structure from the free
 *	list.  The function will sleep if there are no jobs available.
 *	It will then get a SCSI block from SDI.
 */
struct job *
sc01getjob()
{
	register struct job *jp;

#ifdef	DEBUG
	DPR (4)(CE_CONT, "sc01: getjob\n");
#endif
	jp = (struct job *)sdi_get(&lg_poolhead, 0);
	/* Get an SB for this job */
	jp->j_sb = sdi_getblk(KM_SLEEP);
	return(jp);
}
	
/*
 * sc01freejob() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function returns a job structure to the free list. The
 *	SCSI block associated with the job is returned to SDI.
 */
void
sc01freejob(jp)
register struct job *jp;
{
#ifdef	DEBUG
	DPR (4)(CE_CONT, "sc01: freejob\n");
#endif
	sdi_xfreeblk(jp->j_sb->SCB.sc_dev.pdi_adr_version, jp->j_sb);
	sdi_free(&lg_poolhead, (jpool_t *)jp);
}

/*
 * sc01send() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function sends jobs from the work queue to the host adapter.
 *	It will send as many jobs as available or the number required to
 *	keep the logical unit busy.  If the job cannot be accepted by SDI 
 *	the function will reschedule itself via the timeout mechanizism.
 */
void
sc01send(cdp)
register struct cdrom *cdp;
{
	register struct job *jp;
	register int rval;

#ifdef	DEBUG
	DPR (1)(CE_CONT, "sc01: sc01send\n");
#endif
	while (cdp->cd_npend < MAXPEND &&  cdp->cd_next != (struct job *)cdp)
	{
		jp = cdp->cd_next;
		cdp->cd_next = jp->j_next;

		if (jp == cdp->cd_batch) {
			/* Start new batch */
			cdp->cd_batch = (struct job *)cdp;
			cdp->cd_state ^= CD_DIRECTION;
		}

		if ((rval = SC01SEND(jp, KM_SLEEP)) != SDI_RET_OK)
		{
			if (rval == SDI_RET_RETRY) {
#ifdef DEBUG
				cmn_err(CE_NOTE,
					"!CD-ROM Error: SDI currently busy - Will retry later");
#endif
				/* Reset next position */
				cdp->cd_next = jp;

				if (cdp->cd_state & CD_SEND)
					return;

				/* Call back later */
				cdp->cd_state |= CD_SEND;
				cdp->cd_sendid =
					sdi_timeout((void(*)())sc01sendt, 
						(caddr_t)cdp, LATER, pldisk, 
						&cdp->cd_addr);
				return;
			} else {
				/*
		 		 *+ print debug msg
		 		 */
				cmn_err(CE_WARN,
					"!CD-ROM Error: Unknown SDI request - Return Value = 0x%x");
				cdp->cd_npend++;
				sc01comp(jp);
				continue;
			}
		}

		cdp->cd_npend++;
	}

	if (cdp->cd_state & CD_SEND) {
		cdp->cd_state &= ~CD_SEND;
		untimeout(cdp->cd_sendid);
	}
}

/*
 * sc01sendt() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function calls sc01send() after the record of the pending
 *	timeout is erased.
 */
void
sc01sendt(cdp)
register struct cdrom *cdp;
{
	cdp->cd_state &= ~CD_SEND;
	sc01send(cdp);
}

/*
 * sc01open() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 * 	Driver open() entry point.  Determines the type of open being
 *	being requested.  On the first open to a device, the PD and
 *	VTOC information is read. 
 */
/* ARGSUSED */
sc01open(devp, flag, otyp, cred_p)
dev_t	*devp;
int	flag;
int	otyp;
struct cred	*cred_p;
{
	register struct cdrom	*cdp;
	unsigned		unit;
	pl_t			s;
	struct	xsb 	*xsb;
	dev_t	dev = *devp;

	/* check if sc01rinit is in process of creating new sc01_cdrom struct*/
	SC01_LOCK(sc01_lock);
	while (rinit_flag) {
		SV_WAIT(sc01_sv, pridisk, sc01_lock);
		SC01_LOCK(sc01_lock);
	}

	SC01_UNLOCK(sc01_lock);
#ifdef	DEBUG
	if (geteminor(dev) & 0x08) {		/* Debug off */
		int	i;

		for (i = 0; i < SIZE; i++)
			sc01_Debug[i] = 0;
	} else if (geteminor(dev) & 0x10) {		/* Debug on */
		int	i;

		for (i = 0; i < SIZE; i++)
			sc01_Debug[i] = 1;
	}

	DPR (1)(CE_CONT, "sc01open: (dev=0x%x flag=0x%x otype=0x%x)\n",
			dev, flag, otyp);
#endif

	unit = UNIT(dev);

	/* Check for non-existent device */
	if ((int)unit >= sc01_cdromcnt) {
#ifdef	DEBUG
		DPR (3)(CE_CONT, "sc01open: open error\n");
#endif
		return(ENXIO);
	}

	cdp = &sc01_cdrom[unit];

	/*
	* Verify this is for reading only.
	*/

	if ((flag & FWRITE) != 0)
	{
		return(EACCES);
	}

	CD_LOCK(cdp);
	while (cdp->cd_state & CD_WOPEN) {
		SV_WAIT(cdp->cd_sv, pridisk, cdp->cd_lock);
		CD_LOCK(cdp);
	}

	/* Lock out other attempts */
	cdp->cd_state |= CD_WOPEN;

	CD_UNLOCK(cdp);

	if (!(cdp->cd_state & CD_INIT))
	{
		cdp->cd_fltreq = sdi_getblk(KM_SLEEP);  /* Request sense */
		cdp->cd_fltres = sdi_getblk(KM_SLEEP);  /* Resume */

		cdp->cd_fltreq->sb_type = ISCB_TYPE;
		cdp->cd_fltreq->SCB.sc_datapt = SENSE_AD(cdp->cd_sense);
		xsb = (struct xsb *)cdp->cd_fltreq;
		xsb->extra.sb_datapt =
                	(paddr32_t *)((char *)cdp->cd_sense + 
				sizeof(struct sense));
		*(xsb->extra.sb_datapt) += 1;
		xsb->extra.sb_data_type = SDI_PHYS_ADDR;
		cdp->cd_fltreq->SCB.sc_datasz = SENSE_SZ;
		cdp->cd_fltreq->SCB.sc_mode   = SCB_READ;
		cdp->cd_fltreq->SCB.sc_cmdpt  = SCS_AD(&cdp->cd_fltcmd);
		cdp->cd_fltreq->SCB.sc_dev    = cdp->cd_addr;
		sdi_xtranslate(cdp->cd_fltreq->SCB.sc_dev.pdi_adr_version,
					cdp->cd_fltreq, B_READ, 0, KM_SLEEP);

		cdp->cd_state |= CD_INIT;
	}

	if (cdp->cd_spec && cdp->cd_spec->first_open) {
		(*cdp->cd_spec->first_open)(cdp);
	}

	CD_LOCK(cdp);
	cdp->cd_state &= ~CD_WOPEN;
	CD_UNLOCK(cdp);
	SV_BROADCAST(cdp->cd_sv, 0);

	/*
	 * Make sure there is a cd in the device.
	 */

	if (sc01cmd(cdp, SS_TEST, 0, NULL, 0, 0, SCB_READ, 0, 0))
	{
		return( EIO );
	}

	if (sc01cmd(cdp, SS_LOCK, 0, NULL, 0, 1, SCB_READ, 0, 0))
	{
		/* return( EIO ); */
	}

#ifdef	DEBUG
	DPR (1)(CE_CONT, "sc01open: end\n");
#endif
	return(0);
}

/*
 * sc01close() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 * 	Driver close() entry point.  Determine the type of close
 *	being requested.
 */
/* ARGSUSED */
sc01close(dev, flag, otyp, cred_p)
register dev_t	dev;
int		flag;
int		otyp;
struct cred	*cred_p;
{
	register struct cdrom	*cdp;
#ifndef	BUG
	/*
	 * Normally, if prefixopen() is failed, prefixclose() isn't called.
	 * But prefixclose() is called from inner kernel in UNIX R4.0 B9.
	 */
	unsigned		unit;

	/* check if sc01rinit is in process of creating new sc01_cdrom struct*/
	SC01_LOCK(sc01_lock);
	while (rinit_flag) {
		SV_WAIT(sc01_sv, pridisk, sc01_lock);
		SC01_LOCK(sc01_lock);
	}
	SC01_UNLOCK(sc01_lock);
	unit = UNIT(dev);
	/* Check for non-existent device */
	if ((int)unit >= sc01_cdromcnt) {
#ifdef	DEBUG
		DPR (3)(CE_CONT, "sc01close: close error\n");
#endif
		return(ENXIO);
	}
	cdp = &sc01_cdrom[unit];
#else
	cdp = &sc01_cdrom[UNIT(dev)];
#endif
	if (sc01cmd(cdp, SS_LOCK, 0, NULL, 0, 0, SCB_READ, 0, 0))
	{
		/* return( EIO ); */
	}

	if (cdp->cd_spec && cdp->cd_spec->last_close) {
		(*cdp->cd_spec->last_close)(cdp);
	}

	cdp->cd_state &= ~CD_PARMS;

	return(0);
}

/*
 * sc01strategy() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 * 	Driver strategy() entry point.  Initiate I/O to the device.
 *	The buffer pointer passed from the kernel contains all the
 *	necessary information to perform the job.  This function only
 *	checks the validity of the request.  The breakup routines
 *	are called, and the driver is reentered at sc01strat0().
 */
void
sc01strategy(bp)
register struct buf	*bp;
{
	register struct cdrom	*cdp;
#ifdef PDI_SVR42
	register int		sectlen;	/* Sector length */
	struct scsi_ad		*addr;
	struct sdi_devinfo	info;
#endif PDI_SVR42

	/* check if sc01rinit is in process of creating new sc01_cdrom struct*/
	SC01_LOCK(sc01_lock);
	while (rinit_flag) {
		SV_WAIT(sc01_sv, pridisk, sc01_lock);
		SC01_LOCK(sc01_lock);
	}

	SC01_UNLOCK(sc01_lock);
	cdp = &sc01_cdrom[UNIT(bp->b_edev)];

#ifdef	DEBUG
	DPR (1)(CE_CONT, "sc01: sc01strategy\n");
#endif
	/*
	 * CD-ROM is a read-only device.
	 */
	if ((bp->b_flags & B_READ) == 0) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EACCES;
		biodone(bp);
		return;
	}

	if (((cdp->cd_state & CD_PARMS) == 0) && sc01config(cdp)) {
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		biodone(bp);
		return;
	}


#ifdef PDI_SVR42
	sectlen = cdp->cd_capacity->cd_len;
 	addr = &cdp->cd_addr;
	info.strat = sc01strat0;
	info.iotype = cdp->cd_iotype;
	if (! addr->pdi_adr_version )
		info.max_xfer = HIP(HBA_tbl[SDI_EXHAN(addr)].info)->max_xfer;
	else
		info.max_xfer = HIP(HBA_tbl[SDI_HAN_32(addr)].info)->max_xfer;
	info.granularity = sectlen;
	sdi_breakup(bp, &info);
#else
	buf_breakup(sc01strat0, bp, cdp->cd_bcbp);
#endif
}
/*
 * sc01strat0() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 * 	Driver entry point after breakup.  Initiate I/O to the device.
 *	The buffer pointer passed from the kernel contains all the
 *	necessary information to perform the job.
 */
void
sc01strat0(bp)
register struct buf	*bp;
{
	register struct cdrom	*cdp;
	register int		lastsect;	/* Last sector of device */
	register int		sectlen;	/* Sector length */
	unsigned		sectcnt;	/* Sector count */

	/* check if sc01rinit is in process of creating new sc01_cdrom struct*/
	SC01_LOCK(sc01_lock);
	while (rinit_flag) {
		SV_WAIT(sc01_sv, pridisk, sc01_lock);
		SC01_LOCK(sc01_lock);
	}

	SC01_UNLOCK(sc01_lock);

	cdp = &sc01_cdrom[UNIT(bp->b_edev)];

	lastsect = cdp->cd_capacity->cd_addr;
	sectlen = cdp->cd_capacity->cd_len;
	sectcnt = (bp->b_bcount + sectlen - 1) / sectlen;

	bp->b_resid = bp->b_bcount;

	if (bp->b_blkno < 0) {
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		biodone(bp);
		return;
	}
	bp->b_sector = bp->b_blkno / (sectlen / BLKSIZE);

#ifdef	DEBUG
	DPR (3)(CE_CONT, "Block 0x%x -> Sector 0x%x\n",
		bp->b_blkno, bp->b_sector);

	bp->b_sector += sc01_Offset;
	DPR (3)(CE_CONT, "Offset 0x%x -> Sector 0x%x\n",
		sc01_Offset, bp->b_sector);
#endif
	/*
	 * Check if request fits in CD-ROM device 
	 */
	if (bp->b_sector + sectcnt > lastsect) {
		if (bp->b_sector > lastsect) {
			bp->b_flags |= B_ERROR;
			bp->b_error = ENXIO;
			biodone(bp);
			return;
		}
		bp->b_resid = bp->b_bcount -
			((lastsect - bp->b_sector) * sectlen);
		if (bp->b_bcount == bp->b_resid) {
			biodone(bp);
			return;
		}
		bp->b_bcount -= bp->b_resid;
	}

	sc01io(cdp, bp);
}

/*
 * sc01io() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function creates a SCSI I/O request from the information in
 *	the cdrom structure and the buffer header.  The request is queued
 *	according to an elevator algorithm.
 */
void
sc01io(cdp, bp)
register struct cdrom	*cdp;
register buf_t		*bp;
{
	register struct job	*jp;
	register struct scb	*scb;
	register struct scm	*cmd;
	register int		sectlen;
	unsigned		sectcnt;
	pl_t			s;
	struct	xsb	*xsb;

#ifdef	DEBUG
	DPR (1)(CE_CONT, "sc01: sc01io\n");
#endif
	jp = sc01getjob();
	jp->j_bp = bp;
	jp->j_cdp = cdp;
	jp->j_errcnt = 0;

	jp->j_sb->sb_type = SCB_TYPE;
	xsb = (struct xsb *)jp->j_sb;
	if (xsb->extra.sb_data_type != SDI_PHYS_ADDR) {
		if (bp->b_addrtype == SDI_PHYS_ADDR) {
			xsb->extra.sb_data_type = SDI_PHYS_ADDR;
			xsb->extra.sb_datapt =
                             (paddr32_t *) ((char *)paddr(bp) + bp->b_bcount);
		}
		else {
			xsb->extra.sb_bufp = bp;
			xsb->extra.sb_data_type = SDI_IO_BUFFER;
		}
	}

	/*
	 * Fill in the scb for this job.
	 */
	scb = &jp->j_sb->SCB;
	scb->sc_cmdpt = SCM_AD(&jp->j_cmd);
	scb->sc_cmdsz = SCM_SZ;
	scb->sc_datapt = bp->b_un.b_addr;
	scb->sc_datasz = bp->b_bcount;
	scb->sc_link = NULL;
	scb->sc_mode = SCB_READ;
	scb->sc_dev = cdp->cd_addr;

	sdi_xtranslate(jp->j_sb->SCB.sc_dev.pdi_adr_version,
			jp->j_sb, bp->b_flags, bp->b_proc, KM_SLEEP);

	scb->sc_int = sc01intn;
	scb->sc_time = JTIME;
	scb->sc_wd = (long) jp;

	/*
	 * Fill in the command for this job.
	 */
	cmd = &jp->j_cmd.sm;
	cmd->sm_op = SM_READ;
	if (! cdp->cd_addr.pdi_adr_version )	
		cmd->sm_lun = cdp->cd_addr.sa_lun;
	else
		cmd->sm_lun = SDI_LUN_32(&cdp->cd_addr);
	cmd->sm_res1 = 0;
	cmd->sm_res2 = 0;
	cmd->sm_cont = 0;

	jp->j_addr = bp->b_sector;

	sectlen = cdp->cd_capacity->cd_len;
	sectcnt = (bp->b_bcount + sectlen - 1) / sectlen;

	cmd->sm_len  = sdi_swap16(sectcnt);
	cmd->sm_addr = sdi_swap32(jp->j_addr);

#ifdef	DEBUG
	DPR (3)(CE_CONT, "Sector = 0x%x,   Count = 0x%x\n",
		bp->b_sector, sectcnt);
#endif
	/* Is this a partial block? */
	if ((scb->sc_resid = (cmd->sm_len * sectlen) - bp->b_bcount) != 0)
		scb->sc_mode |= SCB_PARTBLK;

	drv_getparm(LBOLT, (ulong *)&bp->b_start);

	/*
 	 * Put the job onto the drive worklist using
 	 * an elevator algorithm.
 	 */
	s = spldisk();
	cdp->cd_count++;
        if (cdp->cd_next == (struct job *) cdp) {
                jp->j_next = (struct job *) cdp;
                jp->j_prev = cdp->cd_last;
                cdp->cd_last->j_next = jp;
		cdp->cd_last = jp;
		cdp->cd_next = jp;
	} else {
		register struct job *jp1 = cdp->cd_batch;

                if (cdp->cd_state & CD_DIRECTION) { 
                        while (jp1 != (struct job *) cdp) {
				if (jp1->j_addr < jp->j_addr)
					break;
                                jp1 = jp1->j_next;
			}
		} else {
                        while (jp1 != (struct job *) cdp) {
				if (jp1->j_addr > jp->j_addr)
					break;
                                jp1 = jp1->j_next;
			}
		}

                jp->j_next = jp1;
                jp->j_prev = jp1->j_prev;
                jp1->j_prev->j_next = jp;
                jp1->j_prev = jp;

                if (jp1 == cdp->cd_batch)
                        cdp->cd_batch = jp;
                if (jp1 == cdp->cd_next)
                        cdp->cd_next = jp;
	}

	sc01send(cdp);
	splx(s);
}

/*
 * sc01read() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 * 	Driver read() entry point.  Performs a "raw" read.
 */
/* ARGSUSED */
sc01read(dev, uio_p, cred_p)
dev_t dev;
struct uio	*uio_p;
struct cred	*cred_p;
{
	register struct cdrom	*cdp;

	/* check if sc01rinit is in process of creating new sc01_cdrom struct*/
	SC01_LOCK(sc01_lock);
	while (rinit_flag) {
		SV_WAIT(sc01_sv, pridisk, sc01_lock);
		SC01_LOCK(sc01_lock);
	}

	SC01_UNLOCK(sc01_lock);

	cdp = &sc01_cdrom[UNIT(dev)];

	if (((cdp->cd_state & CD_PARMS) == 0) && sc01config(cdp)) {
		return(ENXIO);
	}

	return(physiock(sc01strategy, NULL, dev, B_READ, 0, uio_p));
}

/*
 * sc01print() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 * 	Driver print() entry point.  Prints an error message on
 *	the system console.
 */
void
sc01print(dev, str)
dev_t	dev;
char	*str;
{
	register struct cdrom	*cdp;
	char	name[SDI_NAMESZ];
	int	lun;

	if (! cdp->cd_addr.pdi_adr_version )
		lun = cdp->cd_addr.sa_lun;
	else
		lun = SDI_LUN_32(&cdp->cd_addr);

	cdp = &sc01_cdrom[UNIT(dev)];

	sdi_xname(cdp->cd_addr.pdi_adr_version, &cdp->cd_addr, name);
	/*
	 *+ print debug msg
	 */
	cmn_err(CE_WARN, "CD-ROM Error: %s: Logical Unit %d - %s",
		name, lun, str);
}

/*
 * sc01size() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	Driver size() entry point.  Return the device size.
 */
sc01size(dev)
dev_t	dev;
{
	register struct cdrom	*cdp;
	int		unit = UNIT(dev);

	/* check if sc01rinit is in process of creating new sc01_cdrom struct*/
	SC01_LOCK(sc01_lock);
	while (rinit_flag) {
		SV_WAIT(sc01_sv, pridisk, sc01_lock);
		SC01_LOCK(sc01_lock);
	}

	SC01_UNLOCK(sc01_lock);

	if ((int)unit >= sc01_cdromcnt)
		return -1;
	
	cdp = &sc01_cdrom[unit];

	if ((cdp->cd_state & CD_PARMS) == 0) {

		if (sc01open(&dev, 0, OTYP_LYR, (struct cred *)0) ||
			sc01config(cdp))
		{
			return(-1);
		}
	}
	return(cdp->cd_capacity->cd_addr * cdp->cd_capacity->cd_len / BLKSIZE);
}

/*
 * sc01ioctl() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	Driver ioctl() entry point.  Used to implement the following 
 *	special functions:
 *
 *    B_GETTYPE		-  Get bus type and driver name
 *    B_GETDEV		-  Get pass-thru major/minor numbers 
 *    
 *  Group 0 commands
 *    C_TESTUNIT	-  Test unit ready
 *    C_REZERO		-  Rezero unit
 *    C_SEEK		-  Seek
 *    C_INQUIR		-  Inquiry
 *    C_STARTUNIT	-  Start unit
 *    C_STOPUNIT	-  Stop unit
 *    C_PREVMV		-  Prevent medium removal
 *    C_ALLOMV		-  Allow medium removal
 *
 *  Group 1 commands
 *    C_READCAPA	-  Read capacity
 *
 *  Group 6 commands
 *    C_AUDIOSEARCH	-  Audio track search
 *    C_PLAYAUDIO	-  Play audio
 *    C_STILL		-  Still
 *    C_TRAYOPEN	-  tray open
 *    C_TRAYCLOSE	-  tray close
 */
/* ARGSUSED */
sc01ioctl(dev, cmd, arg, mode, cred_p, rval_p)
dev_t	dev;
int	cmd;
caddr_t	arg;
int	mode;
struct cred	*cred_p;
int		*rval_p;
{
	register struct cdrom	*cdp;
	int uerror;

	/* check if sc01rinit is in process of creating new sc01_cdrom struct*/
	SC01_LOCK(sc01_lock);
	while (rinit_flag) {
		SV_WAIT(sc01_sv, pridisk, sc01_lock);
		SC01_LOCK(sc01_lock);
	}

	SC01_UNLOCK(sc01_lock);
	cdp = &sc01_cdrom[UNIT(dev)];
	uerror = 0;

	switch(cmd) {
	case	B_GETTYPE:
		/*
		 * Tell user bus and driver name
		 */
		if (copyout("scsi", ((struct bus_type *)arg)->bus_name, 5))
		{
			uerror = EFAULT;
			break;
		}
		if (copyout("sc01", ((struct bus_type *)arg)->drv_name, 5))
		{
			uerror = EFAULT;
			break;
		}
		break;

	case	B_GETDEV:
		/*
		 * Return pass-thru device major/minor 
		 * to user in arg.
		 */
		{
			dev_t	pdev;

			sdi_getdev(&cdp->cd_addr, &pdev);
			if (copyout((caddr_t)&pdev, arg, sizeof(pdev))) {
				uerror = EFAULT;
			}
			break;
		}

	case	SDI_RELEASE:
		/*
		 * allow another processor on the same SCSI bus to accsess a 
		 * reserved drive.
		 */
		if (sc01cmd(cdp, SS_RELES, 0, NULL, 0, 0, SCB_READ, 0, 0)) {
			uerror = ENXIO;
		}
		break;

	case	SDI_RESERVE:
		/*
		 * reserve a drive to a processor.
		 */
		if (sc01cmd(cdp, SS_RESERV, 0, NULL, 0, 0, SCB_READ, 0, 0)) {
			uerror = ENXIO;
		}
		break;

	/*
	 * The following ioctls are group 0 commamds
	 */
	case	C_TESTUNIT:
		/*
		 * Test Unit Ready
		 */
		if (sc01cmd(cdp, SS_TEST, 0, NULL, 0, 0, SCB_READ, 0, 0))
		{
			uerror = ENXIO;
		}
		break;

	case	C_REZERO:
		/*
		 * Rezero Unit 
		 */
		if (sc01cmd(cdp, SS_REZERO, 0, NULL, 0, 0, SCB_READ, 0, 0))
		{
			uerror = ENXIO;
		}
		break;

	case	C_SEEK:
		/*
		 * Seek 
		 */
		if (sc01cmd(cdp, SS_SEEK, (uint)arg, NULL, 0, 0, SCB_READ, 0, 0))
		{
			uerror = ENXIO;
		}
		break;

	case	C_INQUIR:
		/*
		 * Inquire
		 */
	{
		struct cdrom_inq	*inqp;
		int			inqlen;

		if ((sc01_tmp = sdi_kmem_alloc_phys(SC01_TMPSIZE,
			cdp->cd_bcbp->bcb_physreqp, 0)) == NULL) {
			cmn_err(CE_NOTE, "CD-ROM: :Insufficient memory to complete operation");
			uerror = ENOMEM;
			break;
		}

		SC01_FLAG_LOCK(sc01_flag_lock);
		while (sc01_tmp_flag & B_BUSY) {
			sc01_tmp_flag |= B_WANTED;
			SV_WAIT(sc01_flag_sv, pridisk, sc01_flag_lock);
			SC01_FLAG_LOCK(sc01_flag_lock);
		}

		sc01_tmp_flag |= B_BUSY;

		SC01_FLAG_UNLOCK(sc01_flag_lock);

		inqp = (struct cdrom_inq *)(void *)&sc01_tmp[200];
		if (copyin((caddr_t)arg, (caddr_t)inqp,
			sizeof(struct cdrom_inq)) < 0)
		{
			uerror = EFAULT;
			goto INQUIR_EXIT;
		}
		if ((inqp->length > IDENT_SZ) || (inqp->length == 0))
			inqlen = IDENT_SZ;
		else
			inqlen = inqp->length;
#ifdef DEBUG
		DPR (3)(CE_CONT,
			"sc01:SC_INQUIR length=%x addr=%x inqlen %x\n",
				inqp->length, inqp->addr, inqlen);
#endif
		if (sc01cmd(cdp, SS_INQUIR, 0, (char *)sc01_tmp, inqlen,
			inqlen, SCB_READ, 0, 0))
		{
			uerror = ENXIO;
			goto INQUIR_EXIT;
		}
		if (copyout((caddr_t)sc01_tmp, inqp->addr, inqlen))
			uerror = EFAULT;
INQUIR_EXIT:
		SC01_FLAG_LOCK(sc01_flag_lock);
		sc01_tmp_flag &= ~B_BUSY;
		if (sc01_tmp_flag & B_WANTED) {
			sc01_tmp_flag &= ~B_WANTED;
			SC01_FLAG_UNLOCK(sc01_flag_lock);
			SV_BROADCAST(sc01_flag_sv, 0);
		}
		else
			SC01_FLAG_UNLOCK(sc01_flag_lock);
		kmem_free(sc01_tmp, SC01_TMPSIZE+SDI_PADDR_SIZE(cdp->cd_bcbp->bcb_physreqp));
		break;
	}

	case	C_STARTUNIT:
	case	C_STOPUNIT:
		/*
		 * Start/Stop unit
		 */
		if (sc01cmd(cdp, SS_ST_SP, 0, NULL, 0, 
			(cmd == C_STARTUNIT) ? 1 : 0, SCB_READ, 0, 0))
		{
			uerror = ENXIO;
		}
		break;

	case	C_PREVMV:
	case	C_ALLOMV:
		/*
		 * Prevent/Allow media removal
		 */
		if (sc01cmd(cdp, SS_LOCK, 0, NULL, 0, 
			(cmd == C_PREVMV) ? 1 : 0, SCB_READ, 0, 0))
		{
			uerror = ENXIO;
		}
		break;

	/*
	 * The following ioctls are group 1 commamds
	 */
	case	C_READCAPA:
		/*
		 * Read capacity
		 */
	{
		register struct capacity	*cp;
		
		if ((sc01_tmp = sdi_kmem_alloc_phys(SC01_TMPSIZE,
			cdp->cd_bcbp->bcb_physreqp, 0)) == NULL) {
			 cmn_err(CE_NOTE, "CD-ROM: :Insufficient memory to complete operation");
			uerror = ENOMEM;
			break;
		}
	
		SC01_LOCK(sc01_flag_lock);
		while (sc01_tmp_flag & B_BUSY) {
			sc01_tmp_flag |= B_WANTED;
			SV_WAIT(sc01_flag_sv, pridisk, sc01_flag_lock);
			SC01_LOCK(sc01_flag_lock);
		}
		SC01_UNLOCK(sc01_flag_lock);

		sc01_tmp_flag |= B_BUSY;

		cp = (struct capacity *)(void *)sc01_tmp;
		if (sc01cmd(cdp, SM_RDCAP, 0, (char *)cp, RDCAP_SZ, 0,
			SCB_READ, 0, 0))
		{
			uerror = ENXIO;
			goto READCAPA_EXIT;
		}
		cp->cd_addr = sdi_swap32(cp->cd_addr);
		cp->cd_len  = sdi_swap32(cp->cd_len);
		/* drive didn't give us a valid sector size -- assume 2048 */
		if (cp->cd_len <= 0) {
			cp->cd_len = 2048;
			cdp->cd_capacity->cd_len = 2048;
		}
		if (copyout((caddr_t)cp, arg, sizeof(struct capacity)))
			uerror = EFAULT;

READCAPA_EXIT:
		SC01_LOCK(sc01_flag_lock);
		sc01_tmp_flag &= ~B_BUSY;
		if (sc01_tmp_flag & B_WANTED) {
			sc01_tmp_flag &= ~B_WANTED;
			SC01_UNLOCK(sc01_flag_lock);
			SV_BROADCAST(sc01_flag_sv, 0);
		}
		else
			SC01_UNLOCK(sc01_flag_lock);
		kmem_free(sc01_tmp, SC01_TMPSIZE+SDI_PADDR_SIZE(cdp->cd_bcbp->bcb_physreqp));
		break;
	}

	/*
	 * The following ioctls are group 6 commamds
	 */

	case	C_AUDIOSEARCH:
	case	C_PLAYAUDIO:
	{
		/*
		 * Audio track search &
		 * Play audio
		 */
		struct cdrom_audio	audio;

		if (copyin((caddr_t)arg, (caddr_t)&audio,
			sizeof(struct cdrom_audio)))
		{
			uerror = EFAULT;
			break;
		}
		if (sc01cmd(cdp, 
			(cmd == C_AUDIOSEARCH) ? SV_AUDIOSEARCH : SV_PLAYAUDIO,
			audio.addr_logical, NULL, 0, 0, SCB_READ, audio.play,
			(audio.type << 6)))
		{
			uerror = ENXIO;
		}
		break;
	}
	case	C_STILL:
		/*
		 * Still
		 */
		if (sc01cmd(cdp, SV_STILL, 0, NULL, 0, 0, SCB_READ, 0, 0))
		{
			uerror = ENXIO;
		}
		break;

	case	C_TRAYOPEN:
	case	C_TRAYCLOSE:
	{
		/*
		 * Tray open/close
		 */
		if (sc01cmd(cdp,
			(cmd == C_TRAYOPEN) ? SV_TRAYOPEN : SV_TRAYCLOSE,
			0, NULL, 0, 0, SCB_READ, (int)arg & 0x01, 0))
		{
			uerror = ENXIO;
		}
		break;
	}

	default:
		uerror = EINVAL;
	}
	return(uerror);
}

/*
 * sc01cmd() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function performs a SCSI command such as Mode Sense on
 *	the addressed cdrom.  The op code indicates the type of job
 *	but is not decoded by this function.  The data area is
 *	supplied by the caller and assumed to be in kernel space. 
 *	This function will sleep.
 */
sc01cmd(cdp, op_code, addr, buffer, size, length, mode, param, control)
register struct cdrom	*cdp;
unsigned char	op_code;		/* Command opcode		*/
unsigned int	addr;			/* Address field of command 	*/
char		*buffer;		/* Buffer for command data 	*/
unsigned int	size;			/* Size of the data buffer 	*/
unsigned int	length;			/* Block length in the CDB	*/
unsigned short	mode;			/* Direction of the transfer 	*/
unsigned int	param;			/* Parameter bit		*/
unsigned int	control;		/* Control byte			*/
{
	register struct job *jp;
	register struct scb *scb;
	register buf_t *bp;
	int error;
	pl_t s;
	int	lun;
	struct	xsb	*xsb;

	if (! cdp->cd_addr.pdi_adr_version )
		lun = cdp->cd_addr.sa_lun;
	else
		lun = SDI_LUN_32(&cdp->cd_addr);

	bp = getrbuf(KM_SLEEP);
	jp = sc01getjob();
	scb = &jp->j_sb->SCB;
	
	bp->b_iodone = NULL;
	bp->b_sector = addr;
	bp->b_blkno = addr * (cdp->cd_capacity->cd_len / BLKSIZE);
	bp->b_flags |= mode & SCB_READ ? B_READ : B_WRITE;
	bp->b_error = 0;
	
	jp->j_bp = bp;
	jp->j_cdp = cdp;
	jp->j_errcnt = 0;
	jp->j_sb->sb_type = SCB_TYPE;
	xsb = (struct xsb *)jp->j_sb;

	if (op_code & 0x60) {		/* Group 6 commands */
		register struct scv	*cmd;

		cmd = (struct scv *)&jp->j_cmd.sv;
		cmd->sv_op   = op_code;
		cmd->sv_lun  = lun;
		cmd->sv_res1 = 0;
		cmd->sv_param = param;
		cmd->sv_addr = sdi_swap32(addr);
		cmd->sv_res2 = 0;
		cmd->sv_cont = control & 0xc0;

		scb->sc_cmdpt = SCV_AD(cmd);
		scb->sc_cmdsz = SCV_SZ;
	} else if (op_code & 0x20) {	/* Group 1 commands */
		register struct scm	*cmd;

		cmd = (struct scm *)&jp->j_cmd.sm;
		cmd->sm_op   = op_code;
		cmd->sm_lun  = lun;
		cmd->sm_res1 = 0;
		cmd->sm_addr = sdi_swap32(addr);
		cmd->sm_res2 = 0;
		cmd->sm_len  = sdi_swap16(length);
		cmd->sm_cont = 0;

		scb->sc_cmdpt = SCM_AD(cmd);
		scb->sc_cmdsz = SCM_SZ;
	} else {		  /* Group 0 commands */
		register struct scs	*cmd;

		cmd = (struct scs *)&jp->j_cmd.ss;
		cmd->ss_op    = op_code;
		cmd->ss_lun   = lun;
		cmd->ss_addr1 = (addr & 0x1F0000);
		cmd->ss_addr = sdi_swap16(addr & 0xFFFF);
		cmd->ss_len   = ( short )length;
		cmd->ss_cont  = 0;

		scb->sc_cmdpt = SCS_AD(cmd);
		scb->sc_cmdsz = SCS_SZ;
	}
	
	/* Fill in the SCB */
	scb->sc_int = sc01intn;
	scb->sc_dev = cdp->cd_addr;
	if (buffer) {
		xsb->extra.sb_datapt = (paddr32_t *)
                        ((char *)buffer + size);
                xsb->extra.sb_data_type = SDI_PHYS_ADDR;
	}
	scb->sc_datapt = buffer;
	scb->sc_datasz = size;
	scb->sc_mode = mode;
	scb->sc_resid = 0;
	scb->sc_time = JTIME;
	scb->sc_wd = (long) jp;
	sdi_xtranslate(jp->j_sb->SCB.sc_dev.pdi_adr_version,
			jp->j_sb, bp->b_flags, NULL, KM_SLEEP);

	/* Add job to the queue at the end and batch the queue */
	s = spldisk();
	cdp->cd_count++;
	jp->j_next = (struct job *) cdp;
	jp->j_prev = cdp->cd_last;
	cdp->cd_last->j_next = jp;
	cdp->cd_last = jp;
	if (cdp->cd_next == (struct job *) cdp)
		cdp->cd_next = jp;
	cdp->cd_batch = (struct job *) cdp;
	
	sc01send(cdp);

	biowait(bp);
	splx(s);
	error = bp->b_flags & B_ERROR;
	freerbuf(bp);
	return(error);
}

/*
 * sc01config() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	Initializes the cdrom driver's cdrom paramerter structure.
 */
sc01config(cdp)
register struct cdrom *cdp;
{
	register struct capacity *cp;
 	int	i;	
	int	lun;

	if (! cdp->cd_addr.pdi_adr_version )
		lun = cdp->cd_addr.sa_lun;
	else
		lun = SDI_LUN_32(&cdp->cd_addr);
		

	cp = cdp->cd_capacity;

	/* Send TEST UNIT READY */
	if (sc01cmd(cdp, SS_TEST, 0, ( char *)NULL, 0, 0, SCB_READ, 0, 0))
	{
		/*
		 *+ print debug msg
		 */
		cmn_err(CE_WARN,
			"!CD-ROM Error: Logical Unit %d - Device not ready",
			lun);
		return(-1);
	}
	/* Send READ CAPACITY to obtain last sector address */
	if (sc01cmd(cdp, SM_RDCAP, 0, (char *)cp, RDCAP_SZ, 0, SCB_READ, 0, 0))
	{
#ifdef	DEBUG
		cmn_err(CE_WARN,
			"CD-ROM Error: Logical Unit %d - Unable to send Read Capacity command",
			lun);
#endif
		return(EIO);
	}

	cp->cd_addr = sdi_swap32(cp->cd_addr);
	cp->cd_len  = sdi_swap32(cp->cd_len);

	/*
	 * Check if the parameters are vaild
	 */
	if (cp->cd_addr <= 0)
	{
		/*
		 *+ print debug msg
		 */
		cmn_err(CE_WARN,
			"CD-ROM Error: Logical Unit %d - Invalid media format",
			lun);
		/*
		 *+ print debug msg
		 */
		cmn_err(CE_CONT, "!Invalid capacity value = 0x%x\n",
			cp->cd_addr);
		cdp->cd_state &= ~CD_PARMS;
		return(-1);
	}
	if (cp->cd_len <= 0)
	{
/*
		cmn_err(CE_WARN,
			"CD-ROM Error: Logical Unit %d - Invalid media format",
			lun);
		cmn_err(CE_CONT, "!Invalid logical block length = 0x%x\n",
			cp->cd_len);
		return(-1);
*/
	/* drive didn't give us a valid sector size -- assume 2048  JP */
	/* Are both assignments really needed? */
			cp->cd_len = 2048;
			cdp->cd_capacity->cd_len = 2048;
	}

	/*
	 * Initialize the Block <-> Logical Sector shift amount.
	 */
	for (i=0; i < 32; i++) {
		if ((BLKSIZE << i) == cp->cd_len) {
		     break;
		}
	}

	if ((BLKSIZE << i) != cp->cd_len) {
		/*
		 *+ print debug msg
		 */
		cmn_err(CE_WARN,
			"CD-ROM Error: Logical Unit %d - Invalid media format",
				lun);
		/*
		 *+ print debug msg
		 */
		cmn_err(CE_CONT,
			"!Logical block length (0x%x) not a power-of-two multiple of 0x%x\n",
			cp->cd_len, BLKSIZE);
		cdp->cd_state &= ~CD_PARMS;
		return(-1);
	}

	cdp->cd_blkshft = i;

#ifndef PDI_SVR42
	/*
	 * Set the breakup granularity to the device sector size.
	 */
	cdp->cd_bcbp->bcb_granularity = cp->cd_len;

	if (!(cdp->cd_bcbp->bcb_flags & BCB_PHYSCONTIG) &&
	    !(cdp->cd_bcbp->bcb_addrtypes & BA_SCGTH) &&
	    !(cdp->cd_iotype & F_PIO)) {
		/* Set phys_align for drivers that are not using buf_breakup
		 * to break at every page, and not using the BA_SCGTH
		 * feature of buf_breakup, and not programmed I/O.
		 * (e.g. HBAs that are still doing there own scatter-
		 * gather lists.)
		 */
		cdp->cd_bcbp->bcb_physreqp->phys_align = cp->cd_len;
		(void)physreq_prep(cdp->cd_bcbp->bcb_physreqp, KM_SLEEP);
	}
#endif /* !PDI_SVR42 */
#ifdef	DEBUG
	DPR (3)(CE_CONT, "CD-ROM: addr = 0x%x, len = 0x%x\n", cp->cd_addr, cp->cd_len);
#endif
	/*
	 * Indicate parameters are set and valid
	 */
	cdp->cd_state |= CD_PARMS; 
	return(0);
}
	
/*
 * sc01comp() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	On completion of a job, both successful and failed, this function
 *	removes the job from the work queue, de-allocates the job structure
 *	used, and calls biodone().  The function will restart the logical
 *	unit queue if necessary.
 */
void
sc01comp(jp)
register struct job *jp;
{
        register struct cdrom *cdp;
	register struct buf *bp;
        
        cdp = jp->j_cdp;
        bp = jp->j_bp;

	/* Remove job from the queue */
	jp->j_next->j_prev = jp->j_prev;
	jp->j_prev->j_next = jp->j_next;

	cdp->cd_count--;
	cdp->cd_npend--;

	/* Check if job completed successfully */
	if (jp->j_sb->SCB.sc_comp_code != SDI_ASW)
	{
		bp->b_flags |= B_ERROR;
		if (jp->j_sb->SCB.sc_comp_code == SDI_NOSELE)
			bp->b_error = ENODEV;
		else
			bp->b_error = EIO;
	} else {
		bp->b_resid -= bp->b_bcount;
	}

	biodone(bp);
	sc01freejob(jp);

	/* Resume queue if suspended */
	if (cdp->cd_state & CD_SUSP)
	{
		cdp->cd_fltres->sb_type = SFB_TYPE;
		cdp->cd_fltres->SFB.sf_int  = sc01intf;
		cdp->cd_fltres->SFB.sf_dev  = cdp->cd_addr;
		cdp->cd_fltres->SFB.sf_wd = (long) cdp;
		cdp->cd_fltres->SFB.sf_func = SFB_RESUME;
		if (SC01ICMD(cdp, cdp->cd_fltres, KM_NOSLEEP) == SDI_RET_OK) {
			cdp->cd_state &= ~CD_SUSP;
			cdp->cd_fltcnt = 0;
		}
	}

        sc01send(cdp); 
}

/*
 * sc01intn()
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function is called by the host adapter driver when a SCB
 *	job completes.  If the job completed with an error the job will
 *	be retried when appropriate.  Requests which still fail or are
 *	not retried are failed.
 */
void
sc01intn(sp)
register struct sb	*sp;
{
	register struct cdrom	*cdp;
	register struct job	*jp;

	jp = (struct job *)sp->SCB.sc_wd;
	cdp = jp->j_cdp;

	if (sp->SCB.sc_comp_code == SDI_ASW) {
		sc01comp(jp);
		return;
	}

	if (sp->SCB.sc_comp_code & SDI_SUSPEND)
		cdp->cd_state |= CD_SUSP;

	if (sp->SCB.sc_comp_code == SDI_CKSTAT && sp->SCB.sc_status == S_CKCON)
	{
		struct sense *sensep = sdi_sense_ptr(sp);
		cdp->cd_fltjob = jp;

		if ((sensep->sd_key != SD_NOSENSE) || sensep->sd_fm 
		  || sensep->sd_eom || sensep->sd_ili) {
			bcopy(sensep, cdp->cd_sense,sizeof(struct sense));
			sc01sense(cdp);
			return;
		}
		cdp->cd_fltreq->sb_type = ISCB_TYPE;
		cdp->cd_fltreq->SCB.sc_int = sc01intrq;
		cdp->cd_fltreq->SCB.sc_cmdsz = SCS_SZ;
		cdp->cd_fltreq->SCB.sc_time = JTIME;
		cdp->cd_fltreq->SCB.sc_mode = SCB_READ;
		cdp->cd_fltreq->SCB.sc_dev = sp->SCB.sc_dev;
		cdp->cd_fltreq->SCB.sc_wd = (long) cdp;
		cdp->cd_fltcmd.ss_op = SS_REQSEN;
		if (! sp->SCB.sc_dev.pdi_adr_version )
			cdp->cd_fltcmd.ss_lun = sp->SCB.sc_dev.sa_lun;
		else
			cdp->cd_fltcmd.ss_lun = SDI_LUN_32(&sp->SCB.sc_dev);
		cdp->cd_fltcmd.ss_addr1 = 0;
		cdp->cd_fltcmd.ss_addr = 0;
		cdp->cd_fltcmd.ss_len = SENSE_SZ;
		cdp->cd_fltcmd.ss_cont = 0;

		/* Clear old sense key */
		cdp->cd_sense->sd_key = SD_NOSENSE;

		if (SC01ICMD(cdp, cdp->cd_fltreq, KM_NOSLEEP) == SDI_RET_OK) {
			return;
		}
	}

	if (sp->SCB.sc_comp_code & SDI_RETRY && ++jp->j_errcnt <= MAXRETRY)
	{
		sp->sb_type = ISCB_TYPE;
		sp->SCB.sc_time = JTIME;
		if (SC01ICMD(cdp, sp, KM_NOSLEEP) == SDI_RET_OK) {
			return;
		}
	}

	sc01logerr(cdp, sp);
	sc01comp(jp);
}

/*
 * sc01intrq()
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function is called by the host adapter driver when a
 *	request sense job completes.  The job will be retied if it
 *	failed.  Calls sc01sense() on sucessful completions to
 *	examine the request sense data.
 */
void
sc01intrq(sp)
register struct sb *sp;
{
	register struct cdrom *cdp;

	cdp = (struct cdrom *)sp->SCB.sc_wd;

	if (sp->SCB.sc_comp_code != SDI_CKSTAT  &&
	    sp->SCB.sc_comp_code &  SDI_RETRY   &&
	    ++cdp->cd_fltcnt <= MAXRETRY)
	{
		sp->SCB.sc_time = JTIME;
		if (SC01ICMD(cdp, sp, KM_NOSLEEP) == SDI_RET_OK) {
			return;
		}
	}

	if (sp->SCB.sc_comp_code != SDI_ASW) {
		sc01logerr(cdp, sp);
		sc01comp(cdp->cd_fltjob);
		return;
	}

	sc01sense(cdp);
}

/*
 * sc01intf() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function is called by the host adapter driver when a host
 *	adapter function request has completed.  If there was an error
 *	the request is retried.  Used for resume function completions.
 */
void
sc01intf(sp)
register struct sb *sp;
{
	register struct cdrom *cdp;

	cdp = (struct cdrom *)sp->SFB.sf_wd;

	if (sp->SFB.sf_comp_code & SDI_RETRY && ++cdp->cd_fltcnt <= MAXRETRY)
	{
		if (SC01ICMD(cdp, sp) == SDI_RET_OK) {
			return;
		}
	}

	if (sp->SFB.sf_comp_code != SDI_ASW) 
		sc01logerr(cdp, sp);
}

/*
 * sc01sense()
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function uses the Request Sense information to determine
 *	what to do with the original job.
 */
void
sc01sense(cdp)
register struct cdrom *cdp;
{
	register struct job *jp;
	register struct sb *sp;

	jp = cdp->cd_fltjob;
	sp = jp->j_sb;

        switch(cdp->cd_sense->sd_key)
	{
	case SD_NOSENSE:
	case SD_ABORT:
	case SD_VENUNI:
		sc01logerr(cdp, sp);

		/* FALLTHRU */
	case SD_UNATTEN:
		cdp->cd_state &= ~CD_PARMS;	/* CD-ROM exchanged ? */
		if (++jp->j_errcnt > MAXRETRY)
			sc01comp(jp);
		else {
			sp->sb_type = ISCB_TYPE;
			sp->SCB.sc_time = JTIME;
			if (SC01ICMD(cdp, sp, KM_NOSLEEP) != SDI_RET_OK) {
				sc01logerr(cdp, sp);
				sc01comp(jp);
			}
		}
		break;

	case SD_RECOVER:
		sc01logerr(cdp, sp);
		sp->SCB.sc_comp_code = SDI_ASW;
		sc01comp(jp);
		break;

	default:
		sc01logerr(cdp, sp);
		sc01comp(jp);
        }
}

/*
 * sc01logerr()
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function will print the error messages for errors detected
 *	by the host adapter driver.  No message will be printed for
 *	thoses errors that the host adapter driver has already reported.
 */
void
sc01logerr(cdp, sp)
register struct cdrom *cdp;
register struct sb *sp;
{
	if (sp->sb_type == SFB_TYPE)
	{
		sdi_errmsg("CD-ROM",&cdp->cd_addr,sp,cdp->cd_sense,SDI_SFB_ERR,0);
		return;
	}

	if (sp->SCB.sc_comp_code == SDI_CKSTAT && sp->SCB.sc_status == S_CKCON)
	{
		sdi_errmsg("CD-ROM",&cdp->cd_addr,sp,cdp->cd_sense,SDI_CKCON_ERR,0);
		return;
	}

	if (sp->SCB.sc_comp_code == SDI_CKSTAT)
	{
		sdi_errmsg("CD-ROM",&cdp->cd_addr,sp,cdp->cd_sense,SDI_CKSTAT_ERR,0);
	}
}

extern sdi_xsend(), sdi_xicmd();

/*
 * SC01SEND() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 */
SC01SEND(jobp, sleepflag)
struct job *jobp;
int sleepflag;
{
	return sc01docmd(sdi_xsend, jobp->j_cdp, jobp->j_sb, sleepflag);
}

/*
 * SC01ICMD()
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 */
SC01ICMD(cdromp, sbp, sleepflag)
struct cdrom *cdromp;
struct sb *sbp;
int sleepflag;
{
	return sc01docmd(sdi_xicmd, cdromp, sbp, sleepflag);
}

/*
 * int sc01docmd() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 */
int
sc01docmd(fcn, cdromp, sbp, sleepflag)
int (*fcn)();
struct cdrom *cdromp;
struct sb *sbp;
int sleepflag;
{
	struct dev_spec *dsp = cdromp->cd_spec;
	int cmd;
	int pdi_adr_version;

	if (sbp->sb_type == SFB_TYPE)
                pdi_adr_version = sbp->SFB.sf_dev.pdi_adr_version;
        else
                pdi_adr_version = sbp->SCB.sc_dev.pdi_adr_version;
	if (dsp && sbp->sb_type != SFB_TYPE) {
		cmd = ((struct scs *)(void *) sbp->SCB.sc_cmdpt)->ss_op;
		if (!CMD_SUP(cmd, dsp)) {
			return SDI_RET_ERR;
		} else if (dsp->command && CMD_CHK(cmd, dsp)) {
			(*dsp->command)(cdromp, sbp);
		}
	}

	return (*fcn)(pdi_adr_version, sbp, sleepflag);
}

void
sc01_free_locks()
{
	if (sc01_lock)
		LOCK_DEALLOC(sc01_lock);
	if (sc01_sv)
		SV_DEALLOC(sc01_sv);
	if (sc01_flag_lock)
		LOCK_DEALLOC(sc01_flag_lock);
	if (sc01_flag_sv)
		SV_DEALLOC(sc01_flag_sv);
}
