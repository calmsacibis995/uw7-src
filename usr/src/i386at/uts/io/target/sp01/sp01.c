#ident	"@(#)kern-pdi:io/target/sp01/sp01.c	1.4.5.2"
#ident	"$Header$"


/*
**	SCSI ID-PROCESSOR Target Driver.
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
#include <io/target/sdi/dynstructs.h>
#include <io/target/sp01/sp01.h>
#include <util/mod/moddefs.h>
#include <io/ddi.h>

#define DRVNAME         "sp01 - ID-PROCESSOR target driver"
#define SP01_TMPSIZE	1024

void sp01strategy(buf_t *);
void sp01strat0(buf_t *);
void sp01_swap24 (unsigned int, unsigned char *);

#ifdef SP01_DEBUG
#define SIZE    10
static char     sp01_debug[SIZE] = {0,0,0,0,0,0,0,0,0,0};
#define DPR(l)          if (sp01_debug[l]) cmn_err
#endif

void	sp01logerr(), sp01io(), sp01comp(), sp01sense(), 
	sp01rinit(), sp01strat0(); 
int	sp01start(), SP01ICMD(), phyiock(), sp01send();
int	sp01config(), sp01cmd(), sp01docmd(), SP01SEND(), sp01_spinit();
void	sp01intn();
void	sp01intf();
void	sp01intrq();
void	sp01queue_hi(struct job *, struct sp01_proc *);
void	sp01queue_low(struct job *, struct sp01_proc *);
struct job * sp01_dequeue(struct sp01_proc *);
void	sp01sendq(struct sp01_proc *, int);
void	sp01sendt(struct sp01_proc *);
void	sp01_aen(long, int, unsigned long);
void    sp01_free_locks();

/* Allocated in space.c */
extern	long	 	Sp01_bmajor;	/* Block major number	    */
extern	long	 	Sp01_cmajor;	/* Character major number   */
extern  struct head	lg_poolhead;	/* head for dyn struct routines */
extern	int 		sp01_rwbuf_slot_size;

int 		sp01_proccnt;	/* Num of procs configured  */
struct sp01_proc   *sp01_proc;	/* Array of proc structures */
static	caddr_t		sp01_tmp;	/* Pointer to temporary buffer*/
static	int		sp01_tmp_flag;	/* sp01_tmp control flag    */
static  struct owner   *sp01_ownerlist;	/* list of owner structs    */

int sp01_rwd_cnt = 0;			/* number of sp01_rwd's in use */
struct sp01_rwbuf_dir sp01_rwd[SP01_MAX_RWB];	/* Local node directories  */
struct sp01_rwd_def sp01_rwdd[SP01_MAX_RWB] = {
	{-1}, {-1}, {-1}, {-1}		/* Definition of the local */
};					/* node directories.  i.e. which   */
					/* ctl, chan has a directory, with */	
					/* a pointer to the sp01_rwd entry */

int	sp01devflag	= D_NOBRKUP | D_BLKOFF;

static  int     sp01_dynamic = 0;
static  size_t  mod_memsz = 0;
static  int	rinit_flag = 0;

STATIC LKINFO_DECL( sp_lkinfo, "IO:sp01:sp_lkinfo", 0);
STATIC LKINFO_DECL( sp01_lkinfo, "IO:sp01:sp01_lkinfo", 0);
STATIC LKINFO_DECL( sp01_flag_lkinfo, "IO:sp01:sp01_flag_lkinfo", 0);

static lock_t	*sp01_lock=NULL;
static pl_t     sp01_pl;
static sv_t	*sp01_sv;

static lock_t	*sp01_flag_lock=NULL;
static pl_t     sp01_flag_pl;
static sv_t	*sp01_flag_sv;

static int	sp01_lock_flag = 0;

#define SP01_LOCK(sp01_lock)	sp01_pl = LOCK(sp01_lock, pldisk)
#define SP01_UNLOCK(sp01_lock)  UNLOCK(sp01_lock, sp01_pl)

#define SP_LOCK(pcp) do {                                               \
				SP01_LOCK(sp01_lock);	                \
				if (sp01_lock_flag == 0) {               \
				  sp01_lock_flag = 1;                   \
				  SP01_UNLOCK(sp01_lock);               \
				  pcp->p_pl  = 		        \
					LOCK(pcp->p_lock, pldisk);     \
				  break; 			        \
	 		        }				        \
			        SP01_UNLOCK(sp01_lock);                 \
			        delay(1);			        \
			} while (1)

#define SP_UNLOCK(pcp) do {                                             \
				SP01_LOCK(sp01_lock);                   \
				sp01_lock_flag = 0;	                \
				SP01_UNLOCK(sp01_lock);                 \
			        UNLOCK(pcp->p_lock, pcp->p_pl);       \
			} while (0)


#define SP01_FLAG_LOCK(sp01_flag_lock)                                  \
			 do {                                           \
				SP01_LOCK(sp01_lock);	                \
				if (sp01_lock_flag == 0) {               \
				  sp01_lock_flag = 1;                   \
				  SP01_UNLOCK(sp01_lock);               \
				  sp01_flag_pl =                        \
					LOCK(sp01_flag_lock, pldisk);   \
				  break; 			        \
	 		        }				        \
			        SP01_UNLOCK(sp01_lock);                 \
			        delay(1);			        \
			} while (1)

#define SP01_FLAG_UNLOCK(sp01_flag_lock)                                \
		       do {                                             \
				SP01_LOCK(sp01_lock);                   \
				sp01_lock_flag = 0;	                \
				SP01_UNLOCK(sp01_lock);                 \
			        UNLOCK(sp01_flag_lock, sp01_flag_pl);   \
			} while (0)

STATIC  int     sp01_load(), sp01_unload();
MOD_DRV_WRAPPER(sp01, sp01_load, sp01_unload, NULL, DRVNAME);


/*
 * int sp01_load()
 *
 * Calling/Exit State:
 *
 * Descriptions: loading the sp01 module
 */
STATIC  int
sp01_load()
{
        sp01_dynamic = 1;
        if(sp01start()) {
		sdi_clrconfig(sp01_ownerlist,SDI_REMOVE|SDI_DISCLAIM, sp01rinit);
		return(ENODEV);
	}
        return(0);
}

/*
 * sp01_unload() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 */
STATIC  int
sp01_unload()
{
	sdi_clrconfig(sp01_ownerlist,SDI_REMOVE|SDI_DISCLAIM, sp01rinit);

        if(mod_memsz > (size_t )0)       {
		int i;
		struct sp01_proc *pcp;

		for (i=0, pcp = sp01_proc; i<sp01_proccnt; i++, pcp++) {
			sdi_freebcb(pcp->p_bcbp);
			if (pcp->p_lock)
                                LOCK_DEALLOC(pcp->p_lock);
                        if (pcp->p_sv)
                                SV_DEALLOC(pcp->p_sv);
		}
		sp01_ownerlist = NULL;
		sp01_free_locks();
                kmem_free((caddr_t)sp01_proc, mod_memsz);
		sp01_proc = NULL;
        }
        return(0);
}

/*
 * int
 * sp01start() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	Called by kernel to perform driver initialization.
 *	This function does not access any devices.
 */
int
sp01start()
{
	register struct sp01_proc  *pcp;	/* proc pointer	 */
	struct	owner	*op;
	struct drv_majors drv_maj;
	caddr_t	 base;			/* Base memory pointer	 */
	int  procsz,			/* proc size (in bytes) */
	     tc;			/* TC number		 */
	int sleepflag;
	int count = 0;

	sleepflag = sp01_dynamic ? KM_SLEEP : KM_NOSLEEP;

	if ( !(sp01_lock = LOCK_ALLOC(TARGET_HIER_BASE+1, pldisk,
		&sp01_lkinfo, sdi_sleepflag)) ||
	       !(sp01_sv = SV_ALLOC(sdi_sleepflag)) ) { 

		cmn_err(CE_WARN, "sp01: sp01start():  allocation failed\n");
		return(0);
	}

	if ( !(sp01_flag_lock = LOCK_ALLOC(TARGET_HIER_BASE, pldisk,
		&sp01_flag_lkinfo, sdi_sleepflag)) ||
	       !(sp01_flag_sv = SV_ALLOC(sdi_sleepflag)) ) { 

		cmn_err(CE_WARN, "sp01: sp01start():  allocation failed\n");
		return(0);
	}

	drv_maj.b_maj = Sp01_bmajor;
	drv_maj.c_maj = Sp01_cmajor;
	drv_maj.minors_per = SP_MINORS_PER;
	drv_maj.first_minor = 0;
	sp01_ownerlist = sdi_doconfig(SP01_dev_cfg, SP01_dev_cfg_size,
			"SP01 PROC Driver", &drv_maj, sp01rinit);
	sp01_proccnt = 0;
	for (op = sp01_ownerlist; op; op = op->target_link) {
		sp01_proccnt++;
	}

#ifdef SP01_DEBUG
	cmn_err(CE_CONT, "%d procs claimed\n", sp01_proccnt);
#endif

	/* Check if there are devices configured */
	if (sp01_proccnt == 0) {
		sp01_proccnt = -1;
		return(1);
	}

	/*
	 * Allocate the proc and job structures
	 */
	procsz = sp01_proccnt * sizeof(struct sp01_proc);
	mod_memsz = procsz;
        if ((base = (caddr_t)kmem_zalloc(mod_memsz, sleepflag)) == 0) {
		/*
		 *+ There was insuffcient memory for proc data
		 *+ structures at load-time.
		 */
		cmn_err(CE_WARN,
			"PROCESSOR Error: Insufficient memory to configure driver");
		cmn_err(CE_CONT,"!Could not allocate 0x%x bytes of memory\n",
			mod_memsz);
		sp01_proccnt = -1;
		mod_memsz = 0;
		sp01_free_locks();
		return(1);
	}

	sp01_proc = (struct sp01_proc *)(void *) base;
	/*
	 * Initialize the proc structures
	 */
	pcp = sp01_proc;
	for(tc = 0, op = sp01_ownerlist; tc < sp01_proccnt;
			tc++, op=op->target_link, pcp++) {

		op->maj.b_maj = Sp01_bmajor;
		op->maj.c_maj = Sp01_cmajor;
		op->maj.minors_per = SP_MINORS_PER;
		op->maj.first_minor = count * SP_MINORS_PER;

		if (!sp01_spinit(pcp, op, sleepflag)) {
			pcp->p_state |= SP_INIT;
			count++;
		}
	}
	if(count == 0) {
        	kmem_free(sp01_proc, mod_memsz);
		sp01_proccnt = -1;
		mod_memsz = 0;
		return(1);
	}
	sp01_dir_update();
	return(0);
}

/*
 * int
 * sp01_spinit(struct sp01_proc *pcp, struct owner *op, int sleepflag) 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	Called by sp01start/sp01rinit to perform pcp initialization
 *	for a new proc device.
 *	This function does not access any devices.
 */
int
sp01_spinit(struct sp01_proc *pcp, struct owner *op, int sleepflag) 
{
	struct sdi_edt *edtp;
	struct sp01_rwd_def *ddp;
	struct sp01_rwbuf_dir_entry *dp;
	int found = 0;
	int done = 0;
	int i, ctl, bus, target, lun;

	if ( !(pcp->p_lock = LOCK_ALLOC(TARGET_HIER_BASE, pldisk,
                        &sp_lkinfo, sdi_sleepflag)) ||
             !(pcp->p_sv = SV_ALLOC(sdi_sleepflag)) ) {

                cmn_err(CE_WARN, "sp01: spinit allocation failed\n");
                return(0);
        }

	/* Initialize the queue ptrs */
	pcp->p_queue = pcp->p_lastlow = pcp->p_lasthi = (struct job *) NULL;

#ifdef SP01_DEBUG
	cmn_err(CE_CONT, "proc: op 0x%x ", op);
	cmn_err(CE_CONT, "edt 0x%x ", op->edtp);
	cmn_err(CE_CONT,
		"hba %d scsi id %d lun %d bus %d\n",
		op->edtp->scsi_adr.scsi_ctl,op->edtp->scsi_adr.scsi_target,
		op->edtp->scsi_adr.scsi_lun,op->edtp->scsi_adr.scsi_bus);
#endif
	edtp = op->edtp;
	if (!(edtp->ex_iotype & F_TARGETMODE))
		return (1);
	ctl    = edtp->scsi_adr.scsi_ctl;
	bus    = edtp->scsi_adr.scsi_bus;
	target = edtp->scsi_adr.scsi_target;
	lun    = edtp->scsi_adr.scsi_lun;

	/*
   	 * Allocate a scsi_extended_adr structure
         */

	pcp->p_addr.extended_address =
		(struct sdi_extended_adr *)kmem_zalloc(sizeof(struct sdi_extended_adr), sdi_sleepflag);

	if ( ! pcp->p_addr.extended_address ) {
                cmn_err(CE_WARN, "SCSI ID Processor driver: insufficient memory"
                                        "to allocate extended address");
                return(1);
        }

	pcp->p_addr.pdi_adr_version =
		sdi_ext_address(op->edtp->scsi_adr.scsi_ctl);

	/* Initialize the SCSI address */
        if (! pcp->p_addr.pdi_adr_version ) {
		pcp->p_addr.sa_lun = lun;
		pcp->p_addr.sa_bus = bus;
		pcp->p_addr.sa_ct  = SDI_SA_CT(ctl, target);
		pcp->p_addr.sa_exta= (uchar_t)(target);
	}
	else {
		pcp->p_addr.extended_address->scsi_adr.scsi_ctl = ctl;
		pcp->p_addr.extended_address->scsi_adr.scsi_bus = bus;
		pcp->p_addr.extended_address->scsi_adr.scsi_target = target;
		pcp->p_addr.extended_address->scsi_adr.scsi_lun = lun;
	}

	pcp->p_addr.extended_address->version = SET_IN_TARGET;


	/* Initialize the new SCSI address */
	pcp->p_saddr.scsi_ctl = ctl;
	pcp->p_saddr.scsi_bus = bus;
	pcp->p_saddr.scsi_target = target;
	pcp->p_saddr.scsi_lun = lun;

	pcp->p_ha_chan_id = edtp->ha_chan_id;
	pcp->p_spec = sdi_findspec(edtp, sp01_dev_spec);
	pcp->p_iotype = edtp->iotype;
	pcp->p_ex_iotype = edtp->ex_iotype;

	/*
	 * Create a directory entry for this processor device.
	 * and put it in the local node's directory.
	 * (For remote nodes to read and find their id and offset.)
	 */
	for (ddp = sp01_rwdd, i=0; i< sp01_rwd_cnt; ddp++, i++) {
		if((ctl == ddp->rwdd_ctl) &&
		   (bus == ddp->rwdd_chan)) {
			found++;
			break;
		}
	}
	if (!found) {
		if (sp01_rwd_cnt >= SP01_MAX_RWB) {
 			cmn_err(CE_WARN, "PROCESSOR Error: Cannot initialize %d,%d,%d,%d - out of directories", ctl, bus, target, lun);
			return (1);
		}
		ddp = &sp01_rwdd[sp01_rwd_cnt];
		ddp->rwdd_ctl = ctl;
		ddp->rwdd_chan = bus;
		ddp->rwdd_dirp = &sp01_rwd[sp01_rwd_cnt];
		ddp->rwdd_flags |= RWDD_UPDATED;
		sp01_rwd_cnt++;
	}
	/*
	 * Enter this node in the directory at first available slot.
	 */
	
	for (dp = (struct sp01_rwbuf_dir_entry *)ddp->rwdd_dirp, i = 0; 
	    i < SP01_MAX_NODES; dp++, i++) {
		if (dp->rwd_flags & RWD_ASSIGNED)
			continue;
		dp->rwd_flags |= RWD_ASSIGNED;
		dp->rwd_size = sp01_rwbuf_slot_size;
		dp->rwd_offset = (i + 1) * sp01_rwbuf_slot_size;
		dp->rwd_magic = SP01_RW_MAGIC;
		dp->rwd_id = target;
		done++;
		break;
	}

	if (!done) {
 		cmn_err(CE_WARN, "PROCESSOR Error: Cannot initialize %d,%d,%d,%d - out of directory slots", ctl, bus, target, lun);
		return (1);
	}
	/*
 	 * Set AEN callback for write completion events.
 	 */
 	op->fault = sp01_aen;
 	op->flt_parm = (long)ddp->rwdd_dirp;

	/*
	 * Set LOCALNODE flag if this is the local HBA's ID
	 */
	if (pcp->p_ha_chan_id == target) {
		pcp->p_state |= SP_LOCALNODE;
		ddp->rwdd_addr = pcp->p_addr;
	}

	/*
	 * Call to initialize the breakup control block.
	 */
	if ((pcp->p_bcbp = sdi_xgetbcb(pcp->p_addr.pdi_adr_version,
			&pcp->p_addr, sleepflag)) == NULL) {
		/*
		 *+ PROCESSOR Driver:
		 *+ Insufficient memory to allocate breakup control
		 *+ block data structure when loading driver.
		 */
 		cmn_err(CE_NOTE, "PROCESSOR Error: Insufficient memory to allocate breakup control block.");
		return (1);
	}
	
	pcp->p_sense =
                (struct sense *)sdi_kmem_alloc_phys(sizeof(struct sense),
		pcp->p_bcbp->bcb_physreqp, 0);

        if (pcp->p_sense == NULL) {
                cmn_err(CE_WARN, "Target Mode driver: Insufficient memory to allocate sense block.");
                return(1);
        }

	
	pcp->p_bcbp->bcb_flags |= BCB_ONE_PIECE;
	if (!(pcp->p_bcbp->bcb_flags & BCB_PHYSCONTIG) &&
	    !(pcp->p_bcbp->bcb_addrtypes & BA_SCGTH) &&
	    !(pcp->p_iotype & F_PIO)) {
		/* Set parameters for drivers that are not using buf_breakup
		 * to break at every page, and not using the BA_SCGTH
		 * feature of buf_breakup, and not programmed I/O.
		 * (e.g. HBAs that are still doing there own scatter-
		 * gather lists.)
		 */
		if (pcp->p_bcbp->bcb_max_xfer > ptob(1))
			pcp->p_bcbp->bcb_max_xfer -= ptob(1);
	}
	return(0);
}

/*
 * sp01rinit() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	Called by sdi to perform driver initialization of additional
 *	devices found after the dynamic load of HBA drivers. This 
 *	routine is called only when sp01 is a static driver.
 *	This function does not access any devices.
 */
void
sp01rinit()
{
	register struct sp01_proc  *pcp, *opcp;	/* proc pointer	 */
	struct	owner	*ohp, *op;
	struct drv_majors drv_maj;
	caddr_t	 base;			/* Base memory pointer	 */
	pl_t prevpl;			/* prev process level for splx */
	int  procsz,			/* proc size (in bytes) */
	     new_proccnt,		/* number of additional devs found*/
	     oproccnt,			/* number of devs previously found*/
	     tmpcnt,			/* temp count of devs */
	     found,			/* seach flag variable */
	     pcnt,			/* proc instance	*/
	     sleepflag;			/* can sleep or not	*/
	int  match = 0;


	/* set rinit_flag to prevent any access to sp01_proc while were */
	/* updating it for new devices and copying existing devices      */
	SP01_LOCK(sp01_lock);
	rinit_flag = 1;
	SP01_UNLOCK(sp01_lock);
	new_proccnt= 0;
	drv_maj.b_maj = Sp01_bmajor;
	drv_maj.c_maj = Sp01_cmajor;
	drv_maj.minors_per = SP_MINORS_PER;
	drv_maj.first_minor = 0;
	sleepflag = sp01_dynamic ? KM_SLEEP : KM_NOSLEEP;

	/* call sdi_doconfig with NULL func so we don't get called again */
	ohp = sdi_doconfig(SP01_dev_cfg, SP01_dev_cfg_size,
				"SP01 PROC Driver", &drv_maj, NULL);
	for (op = ohp; op; op = op->target_link) {
		new_proccnt++;
	}
#ifdef SP01_DEBUG
	cmn_err(CE_CONT, "sp01rinit %d procs claimed\n", new_proccnt);
#endif
	/* Check if there are additional devices configured */
	if ((new_proccnt == sp01_proccnt) || (new_proccnt == 0)) {
		SP01_LOCK(sp01_lock);
		rinit_flag = 0;
		SP01_UNLOCK(sp01_lock);
		SV_BROADCAST(sp01_sv, 0);
		return;
	}
	/*
	 * Allocate the proc structures
	 */
	procsz = new_proccnt * sizeof(struct sp01_proc);
        if ((base = kmem_zalloc(procsz, KM_NOSLEEP)) == NULL) {
		/*
		 *+ There was insuffcient memory for proc data
		 *+ structures at load-time.
		 */
		cmn_err(CE_WARN,
			"PROCESSOR Error: Insufficient memory to configure driver");
		cmn_err(CE_CONT,
			"!Could not allocate 0x%x bytes of memory\n",procsz);
		SP01_LOCK(sp01_lock);
		rinit_flag = 0;
		SP01_UNLOCK(sp01_lock);
		SV_BROADCAST(sp01_sv, 0);
		return;
	}
	/*
	 * Initialize the proc structures
	 */
	if(sp01_proccnt == -1)
		oproccnt = 0;
	else
		oproccnt = sp01_proccnt;
	found = 0;
	prevpl = spldisk();
	for(pcp = (struct sp01_proc *)(void *)base, pcnt = 0, op = ohp; 
	    pcnt < new_proccnt;
	    pcnt++, op=op->target_link, pcp++) {

		/* Initialize new proc structs by copying existing proc */
		/* structs into new struct and initializing new instances */
		match = 0;
		if (oproccnt) {
			for (opcp = sp01_proc, tmpcnt = 0; 
			     tmpcnt < sp01_proccnt; opcp++,tmpcnt++) {
				if (! opcp->p_addr.pdi_adr_version)
					match =
						SDI_ADDRCMP(&opcp->p_addr,
							&op->edtp->scsi_adr);
				else
					match = SDI_ADDR_MATCH(&(opcp->p_addr.extended_address->scsi_adr), &op->edtp->scsi_adr);
				if (match) {
					found = 1;
					break;
				}
			}
			if (found) { /* copy opcp to pcp */
				*pcp = *opcp;
				found = 0;
				oproccnt--;
				continue;
			}
		}
		if (sp01_spinit(pcp, op, sleepflag)) {
			/*
			 *+ There was insuffcient memory for proc data
			 *+ structures at load-time.
			 */
			cmn_err(CE_WARN,
			"PROCESSOR Error: Insufficient memory to configure driver");
			kmem_free(base, procsz);
			splx(prevpl);
			SP01_LOCK(sp01_lock);
			rinit_flag = 0;
			SP01_UNLOCK(sp01_lock);
			SV_BROADCAST(sp01_sv, 0);
			return;
		}
	}
	sp01_dir_update();
	if (sp01_proccnt > 0)
		kmem_free(sp01_proc, mod_memsz);
	sp01_proccnt = new_proccnt;
	sp01_proc = (struct sp01_proc *)(void *)base;
	sp01_ownerlist = ohp;
	mod_memsz = procsz;
	splx(prevpl);
	SP01_LOCK(sp01_lock);
	rinit_flag = 0;
	SP01_UNLOCK(sp01_lock);
	SV_BROADCAST(sp01_sv, 0);
}

/*
 * int
 * sp01devinfo(dev_t dev, di_parm_t parm, void **valp)
 *	Get device information.
 *	
 * Calling/Exit State:
 *	The device must already be open.
 */
int
sp01devinfo(dev_t dev, di_parm_t parm, void **valp)
{
	struct sp01_proc	*pcp;
	unsigned long	unit;
	
	unit = NODE(getminor(dev));

	/* Check for non-existent device */
	if ((int)unit >= sp01_proccnt)
		return(ENODEV);

	pcp = &sp01_proc[unit];

	switch (parm) {
		case DI_BCBP:
			*(bcb_t **)valp = pcp->p_bcbp;
			return 0;
		case DI_MEDIA:
			*(char **)valp = "processor";
			return 0;
		default:
			return ENOSYS;
	}
}

/*
 * sp01getjob() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function will allocate a proc job structure from the free
 *	list.  The function will sleep if there are no jobs available.
 *	It will then get a SCSI block from SDI.
 */
struct job *
sp01getjob()
{
	register struct job *jp;

#ifdef	SP01_DEBUG
	DPR (4)(CE_CONT, "sp01: getjob\n");
#endif
	jp = (struct job *)sdi_get(&lg_poolhead, 0);
	/* Get an SB for this job */
	jp->j_sb = sdi_getblk(KM_SLEEP);
	return(jp);
}
	
/*
 * sp01freejob() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function returns a job structure to the free list. The
 *	SCSI block associated with the job is returned to SDI.
 */
void
sp01freejob(jp)
register struct job *jp;
{
#ifdef	SP01_DEBUG
	DPR (4)(CE_CONT, "sp01: freejob\n");
#endif
	sdi_xfreeblk(jp->j_sb->SCB.sc_dev.pdi_adr_version, jp->j_sb);
	sdi_free(&lg_poolhead, (jpool_t *)jp);
}


/*
 * int
 * sp01send(struct sp01_proc *pcp, struct job *, int sleepflag)
 *	This function sends a job to the host adapter driver.
 *	If the job cannot be accepted by the host adapter
 *	driver, the function will reschedule itself via the timeout
 *	mechanism. This routine must be called at pldisk.
 *
 * Called by: sp01cmd sp01io sp01sendt
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
 */
int
sp01send(struct sp01_proc *pcp, struct job *jp,  int sleepflag)
{
	int sendret;		/* sdi_xsend return value 	*/ 

	sendret = sdi_xtranslate(jp->j_cont->SCB.sc_dev.pdi_adr_version, 
				jp->j_cont, jp->j_bp->b_flags,
			jp->j_bp->b_proc, sleepflag);

	if (sendret == SDI_RET_OK) {

		sp01timeout(jp);

		if (jp->j_cont->sb_type == ISCB_TYPE)
			sendret = sdi_xicmd(jp->j_cont->SCB.sc_dev.pdi_adr_version,
					jp->j_cont, sleepflag);
		else
			sendret = sdi_xsend(jp->j_cont->SCB.sc_dev.pdi_adr_version,
					jp->j_cont, sleepflag);
	}
	if (sendret != SDI_RET_OK) {

		sp01untimeout(jp, SDI_ERROR);

		if (sendret == SDI_RET_RETRY) {

			/* relink back to the queue */
			sp01queue_hi(jp,pcp);

			/* do not hold the job queue busy any more */
			/* Call back later */
			if (!(pcp->p_state & SP_SEND)) {
				pcp->p_state |= SP_SEND;
				if ( !(pcp->p_sendid =
				sdi_timeout(sp01sendt, (caddr_t) pcp, 1, pldisk,
				   &pcp->p_addr))) {
					pcp->p_state &= ~SP_SEND;
					/*
					 *+ sp01 send routine could not
					 *+ schedule a job for retry
					 *+ via itimeout
					 */
					cmn_err(CE_WARN, "sp01send: itimeout failed");
				}
			}
			return 0;
		} else {
#ifdef SP01_DEBUG
			cmn_err(CE_CONT, "SP01 Driver: Bad type to HA");
#endif
			sp01comp(jp);
		}
	} else {
		pcp->p_npend++;
	}
	return 1;
}

/*
 * sp01open() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 * 	Driver open() entry point.  Determines the type of open being
 *	being requested.  On the first open to a device, the PD and
 *	VTOC information is read. 
 */
/* ARGSUSED */
sp01open(devp, flag, otyp, cred_p)
dev_t	*devp;
int	flag;
int	otyp;
struct cred	*cred_p;
{
	register struct sp01_proc	*pcp;
	unsigned		unit;
	pl_t			s;
	dev_t	dev = *devp;
	struct	xsb	*xsb;

	/* check if sp01rinit is in process of creating new sp01_proc struct*/
	SP01_LOCK(sp01_lock);
	while (rinit_flag) {
		SV_WAIT(sp01_sv, pridisk, sp01_lock);
		SP01_LOCK(sp01_lock);
	}

	SP01_UNLOCK(sp01_lock);

#ifdef	SP01_DEBUG
	if (geteminor(dev) & 0x08) {		/* Debug off */
		int	i;

		for (i = 0; i < SIZE; i++)
			sp01_Debug[i] = 0;
	} else if (geteminor(dev) & 0x10) {		/* Debug on */
		int	i;

		for (i = 0; i < SIZE; i++)
			sp01_Debug[i] = 1;
	}

	DPR (1)(CE_CONT, "sp01open: (dev=0x%x flag=0x%x otype=0x%x)\n",
			dev, flag, otyp);
#endif

	unit = NODE(getminor(dev));

	/* Check for non-existent device */
	if ((int)unit >= sp01_proccnt) {
#ifdef	SP01_DEBUG
		DPR (3)(CE_CONT, "sp01open: open error\n");
#endif
		return(ENXIO);
	}

	pcp = &sp01_proc[unit];

	if (!(pcp->p_state & SP_INIT))
		return(ENXIO);

	/* Wait if someone else already opening */
	SP_LOCK(pcp);
	while (pcp->p_state & SP_WOPEN) {
		SV_WAIT(pcp->p_sv, pridisk, pcp->p_lock);
		SP_LOCK(pcp);
	}

	/* Lock out other attempts */
	pcp->p_state |= SP_WOPEN;

	SP_UNLOCK(pcp);

	if (!(pcp->p_state & SP_INIT2))
	{
		pcp->p_fltreq = sdi_getblk(KM_SLEEP);  /* Request sense */
		pcp->p_fltres = sdi_getblk(KM_SLEEP);  /* Resume */

		pcp->p_fltreq->sb_type = ISCB_TYPE;
		xsb = (struct xsb *)pcp->p_fltreq;
		pcp->p_fltreq->SCB.sc_datapt = SENSE_AD(pcp->p_sense);
		xsb->extra.sb_datapt =
			(paddr32_t *) ((caddr_t)pcp->p_sense + 
				sizeof (struct sense));
		*(xsb->extra.sb_datapt) += 1;
		xsb->extra.sb_data_type = SDI_PHYS_ADDR;
		pcp->p_fltreq->SCB.sc_datasz = SENSE_SZ;
		pcp->p_fltreq->SCB.sc_mode   = SCB_READ;
		pcp->p_fltreq->SCB.sc_cmdpt  = SCS_AD(&pcp->p_fltcmd);
		pcp->p_fltreq->SCB.sc_dev    = pcp->p_addr;
		sdi_xtranslate(pcp->p_fltreq->SCB.sc_dev.pdi_adr_version,
				pcp->p_fltreq, B_READ, 0, KM_SLEEP);

		pcp->p_state |= SP_INIT2;
	}

	if (pcp->p_spec && pcp->p_spec->first_open) {
		(*pcp->p_spec->first_open)(pcp);
	}

	SP_LOCK(pcp);
	pcp->p_state &= ~SP_WOPEN;
	SP_UNLOCK(pcp);
	SV_BROADCAST(pcp->p_sv, 0);
	/*
	 * Make sure there is a proc device still there.
	 */

	if (sp01cmd(pcp, SS_TEST, 0, NULL, 0, 0, SCB_READ, 0, 0))
	{
		return( EIO );
	}

#ifdef	SP01_DEBUG
	DPR (1)(CE_CONT, "sp01open: end\n");
#endif
	return(0);
}

/*
 * sp01close() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 * 	Driver close() entry point.  Determine the type of close
 *	being requested.
 */
/* ARGSUSED */
sp01close(dev, flag, otyp, cred_p)
register dev_t	dev;
int		flag;
int		otyp;
struct cred	*cred_p;
{
	register struct sp01_proc	*pcp;
	unsigned		unit;

	/* check if sp01rinit is in process of creating new sp01_proc struct*/
	SP01_LOCK(sp01_lock);
	while (rinit_flag) {
		SV_WAIT(sp01_sv, pridisk, sp01_lock);
		SP01_LOCK(sp01_lock);
	}

	SP01_UNLOCK(sp01_lock);
	unit = NODE(getminor(dev));
	/* Check for non-existent device */
	if ((int)unit >= sp01_proccnt) {
#ifdef	SP01_DEBUG
		DPR (3)(CE_CONT, "sp01close: close error\n");
#endif
		return(ENXIO);
	}
	pcp = &sp01_proc[NODE(getminor(dev))];

	if (pcp->p_spec && pcp->p_spec->last_close) {
		(*pcp->p_spec->last_close)(pcp);
	}

	pcp->p_state &= ~SP_PARMS;

	return(0);
}


/*
 * sp01strategy() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 * 	Driver strategy() entry point.  Initiate I/O to the device.
 *	The buffer pointer passed from the kernel contains all the
 *	necessary information to perform the job.  This function only
 *	checks the validity of the request.  The breakup routines
 *	are called, and the driver is reentered at sp01strat0().
 */
void
sp01strategy(buf_t *bp)
{
	register struct sp01_proc	*pcp;

	/* check if sp01rinit is in process of creating new sp01_proc struct*/
	SP01_LOCK(sp01_lock);
	while (rinit_flag) {
		SV_WAIT(sp01_sv, pridisk, sp01_lock);
		SP01_LOCK(sp01_lock);
	}

	SP01_UNLOCK(sp01_lock);
	pcp = &sp01_proc[NODE(getminor(bp->b_edev))];

#ifdef	SP01_DEBUG
	DPR (1)(CE_CONT, "sp01: sp01strategy\n");
#endif

	/*
	 * Check that the directory info for the addressed node has been
	 * initialized.  If so, the offset and size of the message buffer
	 * is available.
	 */
	if (((pcp->p_state & SP_PARMS) == 0) && sp01config(pcp)) {
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		biodone(bp);
		return;
	}

	buf_breakup(sp01strat0, bp, pcp->p_bcbp);
}
/*
 * sp01strat0() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 * 	Driver entry point after breakup.  Initiate I/O to the device.
 *	The buffer pointer passed from the kernel contains all the
 *	necessary information to perform the job.
 */
void
sp01strat0(buf_t *bp)
{
	struct sp01_proc	*pcp;
	int			nblks;
	unsigned int	bufsize;
	unsigned int	blksize;

	/* check if sp01rinit is in process of creating new sp01_proc struct*/
	SP01_LOCK(sp01_lock);
	while (rinit_flag) {
		SV_WAIT(sp01_sv, pridisk, sp01_lock);
		SP01_LOCK(sp01_lock);
	}

	SP01_UNLOCK(sp01_lock);
	pcp = &sp01_proc[NODE(getminor(bp->b_edev))];

	bp->b_resid = bp->b_bcount;
	if (bp->b_bcount == 0) {
		biodone(bp);
		return;
	}

	if (bp->b_flags & B_READ) {
		bufsize = pcp->p_mbi_rwbufsize;
		blksize = pcp->p_mbi_rwblksize;
	} else {
		bufsize = pcp->p_mbo_rwbufsize;
		blksize = pcp->p_mbo_rwblksize;
	}

	if (bp->b_bcount >= bufsize) {
		/*
		 * Count must not exceed buffer size; truncate if necessary
		 */
		bp->b_bcount = bufsize;
	}

	nblks = bp->b_bcount / blksize;

	if ((nblks * blksize) != bp->b_bcount) {
		/*
		 * Count must be a multiple of blksize
		 */
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		biodone(bp);
		return;
	}

#ifdef	SP01_DEBUG
	if (bp->b_flags & B_READ)
		DPR (3)(CE_CONT, 
		"Node[%d]: READ Count 0x%x [rwbufsize 0x%x, rwblksize 0x%x]\n",
			NODE(bp->b_edev), bp->b_bcount, bufsize, blksize);
	else
		DPR (3)(CE_CONT, 
		"Node[%d]: WRITE Count 0x%x [rwbufsize 0x%x, rwblksize 0x%x]\n",
			NODE(bp->b_edev), bp->b_bcount, bufsize, blksize);
#endif

	sp01io(pcp, bp);
}

/*
 * sp01io() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function creates a SCSI I/O request from the information in
 *	the proc structure and the buffer header.  The request is queued
 *	according to an elevator algorithm.
 */
void
sp01io(struct sp01_proc *pcp, buf_t *bp)
{
	struct job	*jp;
	struct scb	*scb;
	struct rwb_scm	*cmd;
	pl_t		s;
	unsigned int offset;
	struct	xsb	*xsb;

#ifdef	SP01_DEBUG
	DPR (1)(CE_CONT, "sp01: sp01io\n");
#endif
	if (bp->b_flags & B_READ) {
		SP01_FLAG_LOCK(sp01_flag_lock);
		while (!(pcp->p_mbi_dp->rwd_flags & RWD_WRCOMP)) {
			SV_WAIT(sp01_flag_sv, pridisk, sp01_flag_lock);
			SP01_FLAG_LOCK(sp01_flag_lock);
		}
		pcp->p_mbi_dp->rwd_flags &= ~RWD_WRCOMP;
		SP01_FLAG_UNLOCK(sp01_flag_lock);
	}
		
	jp = sp01getjob();
	jp->j_bp = bp;
	jp->j_pcp = pcp;
	jp->j_errcnt = 0;
	jp->j_cont = jp->j_sb;

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
	scb->sc_cmdpt = (caddr_t)&jp->j_cmd;
	scb->sc_cmdsz = SCM_SZ;
	scb->sc_datapt = bp->b_un.b_addr;
	scb->sc_datasz = bp->b_bcount;
	scb->sc_link = NULL;

	scb->sc_int = sp01intn;
	scb->sc_time = JTIME;
	scb->sc_wd = (long) jp;

	/*
	 * Fill in the command for this job.
	 */
	cmd = &jp->j_cmd.sm;

	if (bp->b_flags & B_READ) {
		cmd->rwb_op = SM_READ_BUF;
		offset = pcp->p_mbi_rwoffset;
		scb->sc_mode = SCB_READ;
		scb->sc_dev = pcp->p_mbi_addr;
	} else {
		cmd->rwb_op = SM_WRITE_BUF;
		offset = pcp->p_mbo_rwoffset;
		scb->sc_mode = SCB_WRITE;
		scb->sc_dev = pcp->p_addr;
	}

	if (! pcp->p_addr.pdi_adr_version )
		cmd->rwb_lun = pcp->p_addr.sa_lun;
	else
		cmd->rwb_lun = SDI_LUN_32(&pcp->p_addr);
	cmd->rwb_resv = 0;
	cmd->rwb_mode = RW_BUF_DATA;
	cmd->rwb_bufid = 0;
	cmd->rwb_ctl = 0;
	sp01_swap24(offset, cmd->rwb_offset);
	sp01_swap24(bp->b_bcount, cmd->rwb_len);

	s = spldisk();
	pcp->p_count++;
	if (pcp->p_queue) {
		sp01queue_low(jp, pcp);
		sp01sendq(pcp, KM_SLEEP);
	}
	else {
		sp01send(pcp, jp, KM_SLEEP);
	}
	splx(s);
}

/*
 * sp01read() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 * 	Driver read() entry point.  Performs a "raw" read.
 */
/* ARGSUSED */
sp01read(dev, uio_p, cred_p)
dev_t dev;
struct uio	*uio_p;
struct cred	*cred_p;
{
	register struct sp01_proc	*pcp;

	/* check if sp01rinit is in process of creating new sp01_proc struct*/
	SP01_LOCK(sp01_lock);
	while (rinit_flag) {
		SV_WAIT(sp01_sv, pridisk, sp01_lock);
		SP01_LOCK(sp01_lock);
	}

	SP01_UNLOCK(sp01_lock);

	pcp = &sp01_proc[NODE(getminor(dev))];

	if (((pcp->p_state & SP_PARMS) == 0) && sp01config(pcp)) {
		return(ENXIO);
	}

	return(physiock(sp01strategy, NULL, dev, B_READ, 0, uio_p));
}
/*
 * sp01write() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 * 	Driver write() entry point.  Performs a "raw" write.
 */
/* ARGSUSED */
sp01write(dev, uio_p, cred_p)
dev_t dev;
struct uio	*uio_p;
struct cred	*cred_p;
{
	register struct sp01_proc	*pcp;

	/* check if sp01rinit is in process of creating new sp01_proc struct*/
	SP01_LOCK(sp01_lock);
	while (rinit_flag) {
		SV_WAIT(sp01_sv, pridisk, sp01_lock);
		SP01_LOCK(sp01_lock);
	}

	SP01_UNLOCK(sp01_lock);

	pcp = &sp01_proc[NODE(getminor(dev))];

	if (((pcp->p_state & SP_PARMS) == 0) && sp01config(pcp)) {
		return(ENXIO);
	}

	return(physiock(sp01strategy, NULL, dev, B_WRITE, 0, uio_p));
}
/*
 * sp01print() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 * 	Driver print() entry point.  Prints an error message on
 *	the system console.
 */
void
sp01print(dev, str)
dev_t	dev;
char	*str;
{
	register struct sp01_proc	*pcp;
	char	name[SDI_NAMESZ];
	int 	lun;

	pcp = &sp01_proc[NODE(getminor(dev))];

	if (! pcp->p_addr.pdi_adr_version )
		lun = pcp->p_addr.sa_lun;
	else
		lun = SDI_LUN_32(&pcp->p_addr);
	sdi_xname(pcp->p_addr.pdi_adr_version, &pcp->p_addr, name);
	/*
	 *+ print debug msg
	 */
	cmn_err(CE_WARN, "PROCESSOR Error: %s: Logical Unit %d - %s",
		name, lun, str);
}

/*
 * sp01size() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	Driver size() entry point.  Return the device size.
 */
sp01size(dev)
dev_t	dev;
{
	register struct sp01_proc	*pcp;
	int		unit = NODE(getminor(dev));

	/* check if sp01rinit is in process of creating new sp01_proc struct*/
	SP01_LOCK(sp01_lock);
	while (rinit_flag) {
		SV_WAIT(sp01_sv, pridisk, sp01_lock);
		SP01_LOCK(sp01_lock);
	}

	SP01_UNLOCK(sp01_lock);

	if ((int)unit >= sp01_proccnt)
		return -1;
	
	pcp = &sp01_proc[unit];

	if ((pcp->p_state & SP_PARMS) == 0) {

		if (sp01open(&dev, 0, OTYP_LYR, (struct cred *)0) ||
			sp01config(pcp))
		{
			return(-1);
		}
	}
	return(pcp->p_mbo_rwbufsize);
}

/*
 * sp01ioctl() 
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
 *    P_TESTUNIT	-  Test unit ready
 *    P_REZERO		-  Rezero unit
 *    P_INQUIR		-  Inquiry
 *    P_STARTUNIT	-  Start unit
 *    P_STOPUNIT	-  Stop unit
 *
 *  Group 1 commands
 *    P_TOT_RWBUFSIZ	-  Read/write buffer capacity
 *
 */
/* ARGSUSED */
sp01ioctl(dev, cmd, arg, mode, cred_p, rval_p)
dev_t	dev;
int	cmd;
caddr_t	arg;
int	mode;
struct cred	*cred_p;
int		*rval_p;
{
	register struct sp01_proc	*pcp;
	int uerror;

	/* check if sp01rinit is in process of creating new sp01_proc struct*/
	SP01_LOCK(sp01_lock);
	while (rinit_flag) {
		SV_WAIT(sp01_sv, pridisk, sp01_lock);
		SP01_LOCK(sp01_lock);
	}

	SP01_UNLOCK(sp01_lock);
	pcp = &sp01_proc[NODE(getminor(dev))];
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
		if (copyout("sp01", ((struct bus_type *)arg)->drv_name, 5))
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

			sdi_getdev(&pcp->p_addr, &pdev);
			if (copyout((caddr_t)&pdev, arg, sizeof(pdev))) {
				uerror = EFAULT;
			}
			break;
		}

#ifdef NOTYET
	case	SDI_RELEASE:
		/*
		 * allow another processor on the same SCSI bus to accsess a 
		 * reserved drive.
		 */
		if (sp01cmd(pcp, SS_RELES, 0, NULL, 0, 0, SCB_READ, 0, 0)) {
			uerror = ENXIO;
		}
		break;

	case	SDI_RESERVE:
		/*
		 * reserve a drive to a processor.
		 */
		if (sp01cmd(pcp, SS_RESERV, 0, NULL, 0, 0, SCB_READ, 0, 0)) {
			uerror = ENXIO;
		}
		break;
#endif /* NOTYET */

	/*
	 * The following ioctls are group 0 commamds
	 */
	case	P_TESTUNIT:
		/*
		 * Test Unit Ready
		 */
		if (sp01cmd(pcp, SS_TEST, 0, NULL, 0, 0, SCB_READ, 0, 0))
		{
			uerror = ENXIO;
		}
		break;

#ifdef NOTYET
	case	P_REZERO:
		/*
		 * Rezero Unit 
		 */
		if (sp01cmd(pcp, SS_REZERO, 0, NULL, 0, 0, SCB_READ, 0, 0))
		{
			uerror = ENXIO;
		}
		break;
#endif /* NOTYET */

	case	P_INQUIR:
		/*
		 * Inquire
		 */
	{
		struct sp01_proc_inq	*inqp;
		int			inqlen;

		if ((sp01_tmp = sdi_kmem_alloc_phys(SP01_TMPSIZE,
			pcp->p_bcbp->bcb_physreqp, 0)) == NULL) {
			cmn_err(CE_NOTE, "Target Mode Driver: Insufficient memory to complete operation");
			uerror = ENOMEM;
			break;
		}

		SP01_FLAG_LOCK(sp01_flag_lock);
		while (sp01_tmp_flag & B_BUSY) {
			sp01_tmp_flag |= B_WANTED;
			SV_WAIT(sp01_flag_sv, pridisk, sp01_flag_lock);
			SP01_FLAG_LOCK(sp01_flag_lock);
		}

		sp01_tmp_flag |= B_BUSY;
		SP01_FLAG_UNLOCK(sp01_flag_lock);

		inqp = (struct sp01_proc_inq *)(void *)&sp01_tmp[200];
		if (copyin((caddr_t)arg, (caddr_t)inqp,
			sizeof(struct sp01_proc_inq)) < 0)
		{
			uerror = EFAULT;
			goto INQUIR_EXIT;
		}
		if ((inqp->length > IDENT_SZ) || (inqp->length == 0))
			inqlen = IDENT_SZ;
		else
			inqlen = inqp->length;
#ifdef SP01_DEBUG
		DPR (3)(CE_CONT,
			"sp01:SS_INQUIR length=%x addr=%x inqlen %x\n",
				inqp->length, inqp->addr, inqlen);
#endif
		if (sp01cmd(pcp, SS_INQUIR, 0, (char *)sp01_tmp, inqlen,
			inqlen, SCB_READ, 0, 0))
		{
			uerror = ENXIO;
			goto INQUIR_EXIT;
		}
		if (copyout((caddr_t)sp01_tmp, inqp->addr, inqlen))
			uerror = EFAULT;
INQUIR_EXIT:
		SP01_FLAG_LOCK(sp01_flag_lock);
		sp01_tmp_flag &= ~B_BUSY;
		if (sp01_tmp_flag & B_WANTED) {
			sp01_tmp_flag &= ~B_WANTED;
			SP01_FLAG_UNLOCK(sp01_flag_lock);
			SV_BROADCAST(sp01_flag_sv, 0);
		}
		else
			SP01_FLAG_UNLOCK(sp01_flag_lock);

		kmem_free(sp01_tmp, SP01_TMPSIZE+SDI_PADDR_SIZE(pcp->p_bcbp->bcb_physreqp));
		break;
	}

#ifdef NOTYET		/* This would be nice some day */
	case	P_STARTUNIT:
	case	P_STOPUNIT:
		/*
		 * Start/Stop unit
		 */
		if (sp01cmd(pcp, SS_ST_SP, 0, NULL, 0, 
			(cmd == P_STARTUNIT) ? 1 : 0, SCB_READ, 0, 0))
		{
			uerror = ENXIO;
		}
		break;
#endif /* NOTYET  start/stop unit */

	/*
	 * The following ioctls are group 1 commamds
	 */
	case	P_TOT_RWBUFSIZ:
		/*
		 * Return total Read/Write buffer size
		 */
	{
		if (copyout((caddr_t)arg, &pcp->p_tot_rwbufsiz, sizeof(pcp->p_tot_rwbufsiz)))
			uerror = EFAULT;
		break;
	}

	case	P_IN_RWBUFSIZ:
		/*
		 * Return input Read/Write buffer slot size
		 */
	{
		if (copyout((caddr_t)arg, &pcp->p_mbi_rwbufsize, sizeof(pcp->p_mbi_rwbufsize)))
			uerror = EFAULT;
		break;
	}

	case	P_OUT_RWBUFSIZ:
		/*
		 * Return output Read/Write buffer slot size
		 */
	{
		if (copyout((caddr_t)arg, &pcp->p_mbo_rwbufsize, sizeof(pcp->p_mbo_rwbufsize)))
			uerror = EFAULT;
		break;
	}

	default:
		uerror = EINVAL;
	}
	return(uerror);
}

/*
 * sp01cmd() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function performs a SCSI command such as Mode Sense on
 *	the addressed proc.  The op code indicates the type of job
 *	but is not decoded by this function.  The data area is
 *	supplied by the caller and assumed to be in kernel space. 
 *	This function will sleep.
 */
sp01cmd(pcp, op_code, addr, buffer, size, length, mode, param, control)
register struct sp01_proc	*pcp;
unsigned char	op_code;		/* Command opcode		*/
unsigned int	addr;			/* Address field of command 	*/
char		*buffer;		/* Buffer for command data 	*/
unsigned int	size;			/* Size of the data buffer 	*/
unsigned int	length;			/* Block/alloc length in the CDB*/
unsigned short	mode;			/* Direction of the transfer 	*/
unsigned int	param;			/* Parameter bit		*/
unsigned int	control;		/* Control byte			*/
{
	register struct job *jp;
	register struct scb *scb;
	register buf_t *bp;
	int error;
	pl_t s;
	int lun;
	struct	xsb *xsb;

	bp = getrbuf(KM_SLEEP);
	jp = sp01getjob();
	scb = &jp->j_sb->SCB;
	
	bp->b_iodone = NULL;
	bp->b_sector = addr;
	bp->b_un.b_addr = buffer;
	bp->b_bcount = size;
	bp->b_flags |= mode & SCB_READ ? B_READ : B_WRITE;
	bp->b_error = 0;
	
	jp->j_bp = bp;
	jp->j_pcp = pcp;
	jp->j_errcnt = 0;
	jp->j_sb->sb_type = SCB_TYPE;
	jp->j_cont = jp->j_sb;

	if (! pcp->p_addr.pdi_adr_version )
		lun = pcp->p_addr.sa_lun;
	else
		lun = SDI_LUN_32(&pcp->p_addr);
	if (op_code == SM_READ_BUF || op_code == SM_WRITE_BUF) {
		struct rwb_scm	*cmd;

		cmd = (struct rwb_scm *)&jp->j_cmd.sm;
		cmd->rwb_op   = op_code;
		cmd->rwb_lun = lun;
		cmd->rwb_resv = 0;
		cmd->rwb_mode = param;
		cmd->rwb_bufid = 0;
		cmd->rwb_ctl = 0;
		sp01_swap24(addr, cmd->rwb_offset);
		sp01_swap24(length, cmd->rwb_len);

		scb->sc_cmdpt = (caddr_t)cmd;
		scb->sc_cmdsz = SCM_SZ;
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
	
	xsb = (struct xsb *)jp->j_sb;
	if (buffer) {
		xsb->extra.sb_datapt = (paddr32_t *)((char *)buffer + size);
                xsb->extra.sb_data_type = SDI_PHYS_ADDR;
        }

	/* Fill in the SCB */
	scb->sc_int = sp01intn;
	scb->sc_dev = pcp->p_addr;
	scb->sc_datapt = buffer;
	scb->sc_datasz = size;
	scb->sc_mode = mode;
	scb->sc_resid = 0;
	scb->sc_time = JTIME;
	scb->sc_wd = (long) jp;

	/* Add job to the queue at the end and batch the queue */
	s = spldisk();
	pcp->p_count++;
	sp01queue_low(jp, pcp);
	sp01sendq(pcp, KM_SLEEP);
	splx(s);
	biowait(bp);

	error = bp->b_flags & B_ERROR;
	freerbuf(bp);
	return(error);
}

/*
 * sp01config() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	Initializes the proc driver's proc parameter structure.
 */
sp01config(struct sp01_proc *pcp)
{
 	int	i;	
	struct read_buf_desc *desc;
	unsigned int dtmp;
	char *rwbufdir;
	int dirsiz;
	unsigned int bufcap;
	struct sp01_rwd_def *ddp;
	struct sp01_rwbuf_dir_entry *dp;
	struct sdi_edt *edtp;
	int found = 0;
	int lun;

	if (! pcp->p_addr.pdi_adr_version )
		lun = pcp->p_addr.sa_lun;
	else
		lun = SDI_LUN_32(&pcp->p_addr);

	desc = (struct read_buf_desc *) sdi_kmem_alloc_phys(
		sizeof(struct read_buf_desc),
		pcp->p_bcbp->bcb_physreqp, 0);

        if (desc == NULL) {
                cmn_err(CE_WARN, "Target Mode driver: Insufficient memory to complete operation");
                return(-1);
        }

	
	/* Send TEST UNIT READY */
	if (sp01cmd(pcp, SS_TEST, 0, ( char *)NULL, 0, 0, SCB_READ, 0, 0)) {
		/*
		 *+ print debug msg
		 */
		cmn_err(CE_WARN,
			"!PROCESSOR Error: Logical Unit %d - Device not ready",
				lun);
		return(-1);
	} /* Send READ BUFFER Descripter to obtain total buffer capacity */
	if (sp01cmd(pcp, SM_READ_BUF, 0, (char *)&desc, sizeof(struct read_buf_desc), sizeof(struct read_buf_desc), SCB_READ, RW_BUF_DESC, 0)) {
		cmn_err(CE_WARN, "!PROCESSOR Error: Target %d - Unable to send Read Buffer (descriptor) command", pcp->p_addr.sa_exta);
		return(EIO);
	}

	bufcap = (desc->rbd_bufcap[0] << 16);
	bufcap |= (desc->rbd_bufcap[1] << 8);
	bufcap |= desc->rbd_bufcap[2];

	pcp->p_tot_rwbufsiz = bufcap;

	cmn_err(CE_CONT, "!PROCESSOR: Target %d - read buffer size x%x",
		pcp->p_addr.sa_exta, pcp->p_tot_rwbufsiz);

	/*
	 * Check if the read buffer size is vaild
	 */
	if (pcp->p_tot_rwbufsiz <= 0) {
		/*
		 *+ print debug msg
		 */
		cmn_err(CE_WARN,
			"!PROCESSOR Error: Target %d - Invalid read buffer size x%x",
			pcp->p_addr.sa_exta, pcp->p_tot_rwbufsiz);
		pcp->p_state &= ~SP_PARMS;
		return(-1);
	}

	/* Send READ BUFFER Data to obtain buffer directory info */
	dirsiz = sizeof(struct sp01_rwbuf_dir);
        if ((rwbufdir = sdi_kmem_alloc_phys(dirsiz,
			 pcp->p_bcbp->bcb_physreqp, 0)) == 0) {
		cmn_err(CE_WARN, "!PROCESSOR Error: Target %d - Unable to allocate Read Buffer (dirsiz)",
			pcp->p_addr.sa_exta);
		return (EIO);
	}
	if (sp01cmd(pcp, SM_READ_BUF, 0, rwbufdir, dirsiz, dirsiz, SCB_READ,
		RW_BUF_DATA, 0)) {
		cmn_err(CE_WARN, "!PROCESSOR Error: Target %d - Unable to send Read Buffer (descriptor) command",
			pcp->p_addr.sa_exta);
		return(EIO);
	}
	/*
	 * Node directory info read.  Look for target node's id (mbo).
	 */
	
	for (dp = (struct sp01_rwbuf_dir_entry *)rwbufdir, i = 0; 
	    i < SP01_MAX_NODES; dp++, i++) {
		if (dp->rwd_magic != SP01_RW_MAGIC)
			continue;
		if ((dp->rwd_flags & RWD_ASSIGNED) && 
		    (dp->rwd_id == pcp->p_ha_chan_id)) {
			pcp->p_mbo_rwbufsize = dp->rwd_size;
			pcp->p_mbo_rwoffset = dp->rwd_offset;
			cmn_err(CE_CONT, "!PROCESSOR: Target %d directory info read - found my entry %d\n", pcp->p_saddr.scsi_target, pcp->p_ha_chan_id);
			cmn_err(CE_CONT, "!PROCESSOR: entry size, offset: (x%x,x%x)\n",  dp->rwd_size, dp->rwd_offset);
			break;
		}
	}

	/*
	 * Look for local directory (mbi)
	 */
	for (ddp = sp01_rwdd, i=0; i< sp01_rwd_cnt; ddp++, i++) {
		if((pcp->p_saddr.scsi_ctl == ddp->rwdd_ctl) &&
		   (pcp->p_saddr.scsi_bus == ddp->rwdd_chan)) {
			found++;
			break;
		}
	}
	for (dp = (struct sp01_rwbuf_dir_entry *)ddp->rwdd_dirp, i = 0; 
	    i < SP01_MAX_NODES; dp++, i++) {
		if (!(dp->rwd_flags & RWD_ASSIGNED) ||
		   (dp->rwd_id != pcp->p_saddr.scsi_target))
			continue;
		pcp->p_mbi_rwbufsize = dp->rwd_size;
		pcp->p_mbi_rwoffset = dp->rwd_offset;
		pcp->p_mbi_dp = dp;
		break;
	}
	pcp->p_mbi_addr = ddp->rwdd_addr;
	
        kmem_free(rwbufdir, dirsiz+SDI_PADDR_SIZE(pcp->p_bcbp->bcb_physreqp));
	if (i >= SP01_MAX_NODES ) {
		cmn_err(CE_WARN, "PROCESSOR Error: Target %d - Directory entry not found for %d\n",
			pcp->p_saddr.scsi_target, pcp->p_ha_chan_id);
		return (EIO);
	}

	if (pcp->p_mbo_rwbufsize <= 0 || pcp->p_mbo_rwoffset <= 0) {
		cmn_err(CE_WARN,
			"PROCESSOR Error: Target %d - Invalid directory entry for %d\n",
			pcp->p_addr.sa_exta, pcp->p_ha_chan_id);
		pcp->p_state &= ~SP_PARMS;
		return(-1);
	}
	pcp->p_mbo_rwblksize = pcp->p_mbi_rwblksize = 1;

	/*
	 * Set the breakup granularity to the blksize.
	 */
	pcp->p_bcbp->bcb_granularity = pcp->p_mbo_rwblksize;

	if (!(pcp->p_bcbp->bcb_flags & BCB_PHYSCONTIG) &&
	    !(pcp->p_bcbp->bcb_addrtypes & BA_SCGTH) &&
	    !(pcp->p_iotype & F_PIO)) {
		/* Set phys_align for drivers that are not using buf_breakup
		 * to break at every page, and not using the BA_SCGTH
		 * feature of buf_breakup, and not programmed I/O.
		 * (e.g. HBAs that are still doing there own scatter-
		 * gather lists.)
		 */
		pcp->p_bcbp->bcb_physreqp->phys_align = pcp->p_mbo_rwblksize;
		(void)physreq_prep(pcp->p_bcbp->bcb_physreqp, KM_SLEEP);
	}
#ifdef	SP01_DEBUG
	DPR (3)(CE_CONT, "PROCESSOR: p_mbo_rwbufsize = %d, p_mbo_rwoffset = 0x%x\n", 
		pcp->p_mbo_rwbufsize, pcp->p_mbo_rwoffset);
#endif
	/*
	 * Indicate parameters are set and valid
	 */
	pcp->p_state |= SP_PARMS; 
	return(0);
}
	
/*
 * sp01comp() 
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
sp01comp(jp)
register struct job *jp;
{
        register struct sp01_proc *pcp;
	register struct buf *bp;
        
        pcp = jp->j_pcp;
        bp = jp->j_bp;

	pcp->p_count--;
	pcp->p_npend--;

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
	sp01freejob(jp);

	/* Resume queue if suspended */
	if (pcp->p_state & SP_SUSP)
	{
		pcp->p_fltres->sb_type = SFB_TYPE;
		pcp->p_fltres->SFB.sf_int  = sp01intf;
		pcp->p_fltres->SFB.sf_dev  = pcp->p_addr;
		pcp->p_fltres->SFB.sf_wd = (long) pcp;
		pcp->p_fltres->SFB.sf_func = SFB_RESUME;
		if (SP01ICMD(pcp, pcp->p_fltres, KM_NOSLEEP) == SDI_RET_OK) {
			pcp->p_state &= ~SP_SUSP;
			pcp->p_fltcnt = 0;
		}
	}
}

/*
 * sp01intn()
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
sp01intn(sp)
register struct sb	*sp;
{
	register struct sp01_proc	*pcp;
	register struct job	*jp;

	jp = (struct job *)sp->SCB.sc_wd;
	pcp = jp->j_pcp;

	if (sp->SCB.sc_comp_code == SDI_ASW) {
		sp01comp(jp);
		return;
	}

	if (sp->SCB.sc_comp_code == SDI_CKSTAT && sp->SCB.sc_status == S_CKCON)
	{
		struct sense *sensep = sdi_sense_ptr(sp);
		pcp->p_fltjob = jp;

		if ((sensep->sd_key != SD_NOSENSE) || sensep->sd_fm 
		  || sensep->sd_eom || sensep->sd_ili) {
			bcopy(sensep, pcp->p_sense,sizeof(struct sense));
			sp01sense(pcp);
			return;
		}
		pcp->p_fltreq->sb_type = ISCB_TYPE;
		pcp->p_fltreq->SCB.sc_int = sp01intrq;
		pcp->p_fltreq->SCB.sc_cmdsz = SCS_SZ;
		pcp->p_fltreq->SCB.sc_time = JTIME;
		pcp->p_fltreq->SCB.sc_mode = SCB_READ;
		pcp->p_fltreq->SCB.sc_dev = sp->SCB.sc_dev;
		pcp->p_fltreq->SCB.sc_wd = (long) pcp;
		pcp->p_fltcmd.ss_op = SS_REQSEN;
		if (! sp->SCB.sc_dev.pdi_adr_version )
			pcp->p_fltcmd.ss_lun = sp->SCB.sc_dev.sa_lun;
		else
			pcp->p_fltcmd.ss_lun = SDI_LUN_32(&sp->SCB.sc_dev);
		pcp->p_fltcmd.ss_addr1 = 0;
		pcp->p_fltcmd.ss_addr = 0;
		pcp->p_fltcmd.ss_len = SENSE_SZ;
		pcp->p_fltcmd.ss_cont = 0;

		/* Clear old sense key */
		pcp->p_sense->sd_key = SD_NOSENSE;

		if (SP01ICMD(pcp, pcp->p_fltreq, KM_NOSLEEP) == SDI_RET_OK) {
			return;
		}
	}

	if (sp->SCB.sc_comp_code & SDI_RETRY && ++jp->j_errcnt <= MAXRETRY)
	{
		sp->sb_type = ISCB_TYPE;
		sp->SCB.sc_time = JTIME;
		if (SP01ICMD(pcp, sp, KM_NOSLEEP) == SDI_RET_OK) {
			return;
		}
	}

	sp01logerr(pcp, sp);
	sp01comp(jp);
}

/*
 * sp01intrq()
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function is called by the host adapter driver when a
 *	request sense job completes.  The job will be retied if it
 *	failed.  Calls sp01sense() on sucessful completions to
 *	examine the request sense data.
 */
void
sp01intrq(sp)
register struct sb *sp;
{
	register struct sp01_proc *pcp;

	pcp = (struct sp01_proc *)sp->SCB.sc_wd;

	if (sp->SCB.sc_comp_code != SDI_CKSTAT  &&
	    sp->SCB.sc_comp_code &  SDI_RETRY   &&
	    ++pcp->p_fltcnt <= MAXRETRY)
	{
		sp->SCB.sc_time = JTIME;
		if (SP01ICMD(pcp, sp, KM_NOSLEEP) == SDI_RET_OK) {
			return;
		}
	}

	if (sp->SCB.sc_comp_code != SDI_ASW) {
		sp01logerr(pcp, sp);
		sp01comp(pcp->p_fltjob);
		return;
	}

	sp01sense(pcp);
}

/*
 * sp01intf() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function is called by the host adapter driver when a host
 *	adapter function request has completed.  If there was an error
 *	the request is retried.  Used for resume function completions.
 */
void
sp01intf(sp)
register struct sb *sp;
{
	register struct sp01_proc *pcp;

	pcp = (struct sp01_proc *)sp->SFB.sf_wd;

	if (sp->SFB.sf_comp_code & SDI_RETRY && ++pcp->p_fltcnt <= MAXRETRY)
	{
		if (SP01ICMD(pcp, sp) == SDI_RET_OK) {
			return;
		}
	}

	if (sp->SFB.sf_comp_code != SDI_ASW) 
		sp01logerr(pcp, sp);
}

/*
 * sp01sense()
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function uses the Request Sense information to determine
 *	what to do with the original job.
 */
void
sp01sense(pcp)
register struct sp01_proc *pcp;
{
	register struct job *jp;
	register struct sb *sp;

	jp = pcp->p_fltjob;
	sp = jp->j_sb;

        switch(pcp->p_sense->sd_key)
	{
	case SD_NOSENSE:
	case SD_ABORT:
	case SD_VENUNI:
		sp01logerr(pcp, sp);

		/* FALLTHRU */
	case SD_UNATTEN:
		pcp->p_state &= ~SP_PARMS;	/* PROCESSOR exchanged ? */
		if (++jp->j_errcnt > MAXRETRY)
			sp01comp(jp);
		else {
			sp->sb_type = ISCB_TYPE;
			sp->SCB.sc_time = JTIME;
			if (SP01ICMD(pcp, sp, KM_NOSLEEP) != SDI_RET_OK) {
				sp01logerr(pcp, sp);
				sp01comp(jp);
			}
		}
		break;

	case SD_RECOVER:
		sp01logerr(pcp, sp);
		sp->SCB.sc_comp_code = SDI_ASW;
		sp01comp(jp);
		break;

	default:
		sp01logerr(pcp, sp);
		sp01comp(jp);
        }
}

/*
 * sp01logerr()
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function will print the error messages for errors detected
 *	by the host adapter driver.  No message will be printed for
 *	thoses errors that the host adapter driver has already reported.
 */
void
sp01logerr(pcp, sp)
register struct sp01_proc *pcp;
register struct sb *sp;
{
	if (sp->sb_type == SFB_TYPE)
	{
		sdi_errmsg("PROCESSOR",&pcp->p_addr,sp,pcp->p_sense,SDI_SFB_ERR,0);
		return;
	}

	if (sp->SCB.sc_comp_code == SDI_CKSTAT && sp->SCB.sc_status == S_CKCON)
	{
		sdi_errmsg("PROCESSOR",&pcp->p_addr,sp,pcp->p_sense,SDI_CKCON_ERR,0);
		return;
	}

	if (sp->SCB.sc_comp_code == SDI_CKSTAT)
	{
		sdi_errmsg("PROCESSOR",&pcp->p_addr,sp,pcp->p_sense,SDI_CKSTAT_ERR,0);
	}
}

extern sdi_xsend(), sdi_xicmd();

/*
 * SP01SEND() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 */
SP01SEND(jobp, sleepflag)
struct job *jobp;
int sleepflag;
{
	return sp01docmd(sdi_xsend, jobp->j_pcp, jobp->j_sb, sleepflag);
}

/*
 * SP01ICMD()
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 */
SP01ICMD(pcp, sbp, sleepflag)
struct sp01_proc *pcp;
struct sb *sbp;
int sleepflag;
{
	return sp01docmd(sdi_xicmd, pcp, sbp, sleepflag);
}

/*
 * int sp01docmd() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 */
int
sp01docmd(fcn, pcp, sbp, sleepflag)
int (*fcn)();
struct sp01_proc *pcp;
struct sb *sbp;
int sleepflag;
{
	struct dev_spec *dsp = pcp->p_spec;
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
			(*dsp->command)(pcp, sbp);
		}
	}

	return (*fcn)(pdi_adr_version, sbp, sleepflag);
}

void
sp01_swap24 (unsigned int val, unsigned char *ptr) 
{

	ptr[0] = val >> 16;
	ptr[1] = val >> 8;
	ptr[2] = val;

	return;
}

int
sp01_dir_update() 
{
	int i, j, dirsiz;
	struct sp01_rwd_def *ddp;
	struct sp01_rwbuf_dir_entry *dp;
	struct sp01_proc *pcp;
	int found;

	for (ddp = sp01_rwdd, i=0; i< sp01_rwd_cnt; ddp++, i++) {
	    found = 0;
	    if (ddp->rwdd_flags & RWDD_UPDATED) {
		for (pcp = sp01_proc, j=0;  j < sp01_proccnt; pcp++, j++) {
		    if ((pcp->p_saddr.scsi_ctl == ddp->rwdd_ctl) &&
		        (pcp->p_saddr.scsi_bus == ddp->rwdd_chan) &&
			(pcp->p_state & SP_LOCALNODE)) {
			   found++;
			   break;
		    }
		}
		if (found) {
		    /*
		     * Write it out (this is a write to our local ctl's buffer)
	 	    */
		    dp = (struct sp01_rwbuf_dir_entry *)ddp->rwdd_dirp;
		    dirsiz = sizeof(struct sp01_rwbuf_dir);
		    if (sp01cmd(pcp, SM_WRITE_BUF, 0, dp, dirsiz, dirsiz, 
			SCB_WRITE, RW_BUF_DATA, 0)) {
			cmn_err(CE_WARN, "!PROCESSOR Error: Target %d - Unable to write Read Buffer directory data",
				pcp->p_saddr.scsi_target);
			return(1);
		    }
		    ddp->rwdd_flags &= ~RWDD_UPDATED;
		}
	    }
	}
	return(0);
}

/*
 * STATIC struct job *
 * sp01_dequeue(struct sp01_proc *pcp)
 *
 * Remove and return the job at the head of the p queue
 *
 * Call/Exit State:
 *
 */
STATIC struct job *
sp01_dequeue(struct sp01_proc *pcp)
{
	struct job *jp = pcp->p_queue;
	if (jp) {
		pcp->p_queue = jp->j_next;
		if (jp == pcp->p_lastlow)
			pcp->p_lastlow = NULL;
		else if (jp == pcp->p_lasthi)
			pcp->p_lasthi = NULL;
	}
	return jp;
}

/* STATIC void
 * sp01sendq(struct sp01_proc *pcp, int sleepflag)
 *
 * Send all the jobs on the queue 
 *
 * Call/Exit State:
 */
STATIC void
sp01sendq(struct sp01_proc *pcp, int sleepflag)
{
	struct job *jp;

	if (pcp->p_state & SP_SEND) {
		long sid;
		sid = pcp->p_sendid;
		pcp->p_state &= ~SP_SEND;
		untimeout(sid);
	}
	while ((jp = sp01_dequeue(pcp)) != NULL) {
		if (!sp01send(pcp, jp, sleepflag))
			return;
	}
}

/*
 * void
 * sp01sendt(struct sp01_proc *pcp)
 *	This function call sp01sendq after it turns off the SP_SEND
 *	bit in the disk status work.
 * Called by: timeout
 * Side Effects: 
 *	The send function is called and the record of the pending
 *	timeout is erased.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
void
sp01sendt(struct sp01_proc *pcp)
{
	int state;

#ifdef SP01_DEBUG
        DPR (1)("sp01sendt: (pcp=0x%x)\n", pcp);
#endif

	state = pcp->p_state;
	pcp->p_state &= ~SP_SEND;
	

	/* exit if job queue is suspended or the timeout has been cancelled */
	if ((state & (SP_SUSP|SP_SEND)) != SP_SEND) {
		return;
	}
	sp01sendq(pcp, KM_NOSLEEP);

#ifdef SP01_DEBUG
        DPR (2)("sp01sendt: - exit\n");
#endif
}
/*
 * void
 * sp01queue_hi(struct job *jp, struct sp01_proc *pcp)
 *	Add high-priority job to queue
 *
 * Calling/Exit State:
 */
void
sp01queue_hi(struct job *jp, struct sp01_proc *pcp)
{
	/* No high priority jobs on queue: stick job in front
	 * in front of (possibly empty) queue.
	 */
	if (!pcp->p_lasthi) {
		jp->j_next = pcp->p_queue;
		pcp->p_lasthi = pcp->p_queue = jp;
		return;
	} 
	/* High priority jobs on queue: append this job to the
	 * existing high priority jobs.  This code should not
	 * be reached because sp01queue_hi() is only called for
	 * retry jobs, and we only retry one job at a time.
	 */
	jp->j_next = pcp->p_lasthi->j_next;
	pcp->p_lasthi->j_next = jp;
	pcp->p_lasthi = jp;
	return;
}

/*
 * void
 * sp01queue_low(struct job *jp, struct sp01_proc *pcp)
 *	add low priority job to end of queue
 *
 * Calling/Exit State:
 */
void
sp01queue_low(struct job *jp, struct sp01_proc *pcp)
{
	jp->j_next = NULL;
	if (!pcp->p_queue) {
		/* Add first job to queue */
		pcp->p_queue = pcp->p_lastlow = jp;
		return;
	}
	if (pcp->p_lastlow) {
		/* Append to low priority jobs */
		pcp->p_lastlow = pcp->p_lastlow->j_next = jp;
		return;
	}
	/* Add first low priority job to queue */
	pcp->p_lastlow = pcp->p_lasthi->j_next = jp;
	return;
}

sp01timeout(struct job *jp)
{
	/* Fill in later */
	return;
}
sp01untimeout(struct job *jp)
{
	/* Fill in later */
	return;
}


void
sp01_aen(long parm, int event, unsigned long offset)
{
	struct sp01_rwbuf_dir_entry *dp;
	int i, done = 0;

	if (event == SDI_TARMOD_WRCOMP) {
		
		for (i = 0, dp = (struct sp01_rwbuf_dir_entry *)parm;
		    i < SP01_MAX_NODES; dp++, i++) {
			if (dp->rwd_magic != SP01_RW_MAGIC)
				continue;
			if ((dp->rwd_flags & RWD_ASSIGNED) && 
			    (dp->rwd_offset == offset)) {
				done++;
				break;
			}
		}
		if (!done) {
			if (offset != 0)
 				cmn_err(CE_WARN, "PROCESSOR Error: Cannot wakeup on WRITE completion\n");
			return;
		}

		cmn_err(CE_CONT, "!PROCESSOR: Target %d write complete",
			dp->rwd_id);
		SP01_FLAG_LOCK(sp01_flag_lock);
		dp->rwd_flags |= RWD_WRCOMP;
		SP01_FLAG_UNLOCK(sp01_flag_lock);
		SV_BROADCAST(sp01_flag_sv, 0);
	}
}

void
sp01_printdir() 
{
	struct sp01_rwbuf_dir_entry *dp;
	struct sp01_rwd_def *ddp;
	int i, j;

	cmn_err(CE_CONT, "sp01_rwd_cnt %d", sp01_rwd_cnt);
	for (ddp = sp01_rwdd, i = 0; i < sp01_rwd_cnt; ddp++, i++) {
	  cmn_err(CE_CONT, "sp01_rwdd[%d].rwdd_ctl %d\n", i, ddp->rwdd_ctl);
	  cmn_err(CE_CONT, "sp01_rwdd[%d].rwdd_chan %d\n", i, ddp->rwdd_chan);
	  cmn_err(CE_CONT, "sp01_rwdd[%d].rwdd_dirp x%x\n", i, ddp->rwdd_dirp);
	  for (dp = (struct sp01_rwbuf_dir_entry *)ddp->rwdd_dirp, j = 0; 
		j < SP01_MAX_NODES; dp++, j++) {
	    if (!(dp->rwd_flags & RWD_ASSIGNED)) {
	      cmn_err(CE_CONT, "sp01_rwd[%d] NOT ASSIGNED\n", j);
	      continue;
	    }
	    cmn_err(CE_CONT, "sp01_rwd[%d].rwd_flags x%x\n", j, dp->rwd_flags);
	    cmn_err(CE_CONT, "sp01_rwd[%d].rwd_magic x%x\n", j, dp->rwd_magic);
	    cmn_err(CE_CONT, "sp01_rwd[%d].rwd_id x%x\n", j, dp->rwd_id);
	    cmn_err(CE_CONT, "sp01_rwd[%d].rwd_size x%x\n", j, dp->rwd_size);
	    cmn_err(CE_CONT, "sp01_rwd[%d].rwd_offset x%x\n", j, dp->rwd_offset);
	    }
	}
}

void
sp01_free_locks()
{
        if (sp01_lock)
                LOCK_DEALLOC(sp01_lock);
        if (sp01_sv)
                SV_DEALLOC(sp01_sv);
        if (sp01_flag_lock)
                LOCK_DEALLOC(sp01_flag_lock);
        if (sp01_flag_sv)
                SV_DEALLOC(sp01_flag_sv);
}

