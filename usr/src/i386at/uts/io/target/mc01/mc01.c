#ident	"@(#)kern-pdi:io/target/mc01/mc01.c	1.10.5.2"
#ident	"$Header$"

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
#include <io/target/mc01/mc01.h>
#include <util/mod/moddefs.h>
#include <io/ddi.h>

#define DRVNAME         "mc01 - Medium Changer target driver"

void	mc01logerr();
void	mc01comp();
void	mc01sense();
void	mc01send();
void	mc01rinit();
void	mc01sendt();
void	mc01intn();
void	mc01intf();
void	mc01intrq();

int	mc01start();
int	mc01cmd();
int	mc01docmd();
int	mc01_chinit();
int	MC01ICMD();
int	MC01SEND();

/*
 * Allocated in space.c
 */
extern long	Mc01_cmajor;
extern struct head lg_poolhead;

STATIC int mc01_changer_cnt;
STATIC struct changer	*mc01_changer;
STATIC struct owner	*mc01_ownerlist;

#ifndef PDI_SVR42

int mc01devflag = D_NOBRKUP | D_BLKOFF;

#else /* PDI_SVR42 */

int mc01devflag	= D_NOBRKUP;

#endif /* PDI_SVR42 */

static size_t	mod_memsz = 0;
static int	mc01_dynamic = 0;
static int	rinit_flag = 0;

STATIC	int	mc01_load();
STATIC	int	mc01_unload();

struct mc_status_hdr *mc_status_hdr;

STATIC LKINFO_DECL( mc01_lkinfo, "IO:mc01:mc01_lkinfo", 0);
STATIC LKINFO_DECL( mc_lkinfo, "IO:mc:mc_lkinfo", 0);

static lock_t   *mc01_lock;
static pl_t     mc01_pl;
static sv_t     *mc01_sv;

#define MC01_LOCK(mc01_lock)    mc01_pl = LOCK(mc01_lock, pldisk)
#define MC01_UNLOCK(mc01_lock)  UNLOCK(mc01_lock, mc01_pl)

#define MC_LOCK(mcp)    	mcp->mc_pl = LOCK(mcp->mc_lock, pldisk)
#define MC_UNLOCK(mcp)  	UNLOCK(mcp->mc_lock, mcp->mc_pl)

MOD_DRV_WRAPPER(mc01, mc01_load, mc01_unload, NULL, DRVNAME);

/*
 * int mc01_load()
 *
 * Calling/Exit State:
 *
 * Descriptions: loading the mc01 module
 */
STATIC  int
mc01_load()
{
	mc01_dynamic = 1;

	if (mc01start()) {
		sdi_clrconfig(mc01_ownerlist, SDI_REMOVE | SDI_DISCLAIM, mc01rinit);
		return(ENODEV);
	}

	return(0);
}


/*
 * mc01_unload() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 */
STATIC  int
mc01_unload()
{
	sdi_clrconfig(mc01_ownerlist, SDI_REMOVE | SDI_DISCLAIM, mc01rinit);

	if (mod_memsz > (size_t )0) {
		mc01_ownerlist = NULL;
		kmem_free((caddr_t)mc01_changer, mod_memsz);
		if (mc01_lock)
			LOCK_DEALLOC(mc01_lock);
		if (mc01_sv)
			SV_DEALLOC(mc01_sv);
		mc01_changer = NULL;
	}

	return(0);
}


/*
 * int
 * mc01start() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	Called by kernel to perform driver initialization.
 *	This function does not access any devices.
 */
int
mc01start()
{
	struct changer *mcp;
	struct owner *op;
	struct drv_majors drv_maj;

	caddr_t	 base;

	int	changer_sz;
	int	tc;
	int	sleepflag;

	sleepflag = mc01_dynamic ? KM_SLEEP : KM_NOSLEEP;

	if ( !(mc01_lock = LOCK_ALLOC(TARGET_HIER_BASE+1, pldisk,
		&mc01_lkinfo, sdi_sleepflag)) ||
		!(mc01_sv = SV_ALLOC(sdi_sleepflag)) ) {

		cmn_err(CE_WARN, "mc01: mc01start():  allocation failed\n");
		return(0);
	}

	mc01_changer_cnt = 0;
	drv_maj.b_maj = Mc01_cmajor;	/* mc01 has no block major */
	drv_maj.c_maj = Mc01_cmajor;
	drv_maj.minors_per = MC_MINORS_PER;
	drv_maj.first_minor = 0;

	mc01_ownerlist = sdi_doconfig(MC01_dev_cfg, MC01_dev_cfg_size,
	    "MC01 CDROM Driver", &drv_maj, mc01rinit);

	for (op = mc01_ownerlist; op; op = (struct owner *)op->target_link) {
		mc01_changer_cnt++;
	}

	if (mc01_changer_cnt == 0) {
		mc01_changer_cnt = 0;
		return(1);
	}

#ifdef MC01_DEBUG
	cmn_err(CE_CONT, "Medium Changer: Claimed %d changers\n", mc01_changer_cnt);
#endif

	changer_sz = mc01_changer_cnt * sizeof(struct changer );
	mod_memsz = changer_sz;

	if ((base = (caddr_t)kmem_zalloc(mod_memsz, sleepflag)) == 0) {
		cmn_err(CE_WARN,
		    "Medium Changer Error: Insufficient memory to configure driver");
		cmn_err(CE_CONT, "!Could not allocate 0x%x bytes of memory\n",
		    mod_memsz);
		mc01_changer_cnt = 0;
		mod_memsz = 0;
		return(1);
	}

	mc01_changer = (struct changer *)(void *) base;

	mcp = mc01_changer;

	for (tc = 0, op = mc01_ownerlist; tc < mc01_changer_cnt; 
	    tc++, op = (struct owner *)op->target_link, mcp++) {

		if (mc01_chinit(mcp, op)) {
			if (tc == 0) {
				kmem_free(mc01_changer, mod_memsz);
				mc01_changer_cnt = 0;
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
 * mc01_chinit(struct changer *mcp, struct owner *op) 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	Called by mc01start/mc01rinit to perform mcp initialization
 *	for a new changer device.
 *	This function does not access any devices.
 */
int
mc01_chinit(struct changer *mcp, struct owner *op)
{
	caddr_t mc01_mem_alloc();

	mcp->mc_last = (struct mc_job *) mcp;
	mcp->mc_next = (struct mc_job *) mcp;

	if ( !(mcp->mc_lock = LOCK_ALLOC(TARGET_HIER_BASE, pldisk,
		&mc_lkinfo, sdi_sleepflag)) ||
		 !(mcp->mc_sv = SV_ALLOC(sdi_sleepflag)) ) {
		cmn_err(CE_WARN, "mc01: chinit: memory allocation failed\n");
		return(0);
	}

	mcp->mc_sense = 
		(struct sense *) (void *) mc01_mem_alloc (sizeof(struct sense));

	if (mcp->mc_sense == (struct sense *)NULL) {
		cmn_err(CE_NOTE, "mc01: failed to allocate sense structure");
		return(0);
	}

	mcp->mc_addr.extended_address =
		(struct sdi_extended_adr *)kmem_zalloc(sizeof(struct sdi_extended_adr), sdi_sleepflag);

	if ( ! mcp->mc_addr.extended_address ) {
		cmn_err(CE_WARN, "Medium Changer Driver: insufficient memory"
                                        "to allocate extended address");
                return(1);
        }

	mcp->mc_addr.pdi_adr_version = 
		sdi_ext_address(op->edtp->scsi_adr.scsi_ctl);

	if (! mcp->mc_addr.pdi_adr_version ) {
		mcp->mc_addr.sa_lun = op->edtp->scsi_adr.scsi_lun;
		mcp->mc_addr.sa_bus = op->edtp->scsi_adr.scsi_bus;
		mcp->mc_addr.sa_ct = SDI_SA_CT(op->edtp->scsi_adr.scsi_ctl,
	    		op->edtp->scsi_adr.scsi_target);
		mcp->mc_addr.sa_exta = 
			(uchar_t)(op->edtp->scsi_adr.scsi_target);
	}
	else
		mcp->mc_addr.extended_address->scsi_adr =
			op->edtp->scsi_adr;

	mcp->mc_addr.extended_address->version = SET_IN_TARGET;
	mcp->mc_spec = sdi_findspec(op->edtp, mc01_dev_spec);
	mcp->mc_iotype = op->edtp->iotype;
	mcp->mc_last_loaded = -1;

	return(0);
}


/*
 * mc01rinit() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	Called by sdi to perform driver initialization of additional
 *	devices found after the dynamic load of HBA drivers. This 
 *	routine is called only when mc01 is a static driver.
 *	This function does not access any devices.
 */
void
mc01rinit()
{
	struct changer *mcp;
	struct changer *ocdp;

	struct owner *ohp;
	struct owner *op;

	struct drv_majors drv_maj;

	caddr_t	 base;

	pl_t prevpl;

	int	changer_sz;
	int	new_changer_cnt;
	int	ochanger_cnt;
	int	tmpcnt;
	int	found;
	int	romcnt;
	int 	match = 0;

	MC01_LOCK(mc01_lock);
	rinit_flag = 1;
	MC01_UNLOCK(mc01_lock);

	drv_maj.b_maj = Mc01_cmajor;	/* mc01 has no block major */
	drv_maj.c_maj = Mc01_cmajor;
	drv_maj.minors_per = MC_MINORS_PER;
	drv_maj.first_minor = 0;

	ohp = sdi_doconfig(MC01_dev_cfg, MC01_dev_cfg_size,
	    "MC01 CDROM Driver", &drv_maj, NULL);

	for (new_changer_cnt = 0, op = ohp; op; op = (struct owner *)op->target_link) {
		new_changer_cnt++;
	}

	if ((new_changer_cnt == mc01_changer_cnt) || (new_changer_cnt == 0)) {
		MC01_LOCK(mc01_lock);
		rinit_flag = 0;
		MC01_UNLOCK(mc01_lock);
		SV_BROADCAST(mc01_sv, 0);
		return;
	}

	changer_sz = new_changer_cnt * sizeof(struct changer );

	if ((base = kmem_zalloc(changer_sz, KM_NOSLEEP)) == NULL) {
		/*
		 *+ There was insuffcient memory for changer data
		 *+ structures at load-time.
		 */
		cmn_err(CE_WARN,
		    "Medium Changer Error: Insufficient memory to configure driver");
		cmn_err(CE_CONT,
		    "!Could not allocate 0x%x bytes of memory\n", changer_sz);
		MC01_LOCK(mc01_lock);
		rinit_flag = 0;
		MC01_UNLOCK(mc01_lock);
		SV_BROADCAST(mc01_sv, 0);
		if (mc01_lock)
			LOCK_DEALLOC(mc01_lock);
		return;
	}

	ochanger_cnt = mc01_changer_cnt;

	found = 0;

	prevpl = spldisk();

	for (mcp = (struct changer *)(void *)base, romcnt = 0, op = ohp; 
	     romcnt < new_changer_cnt; 
	     romcnt++, op = (struct owner *)op->target_link, mcp++) {

		match = 0;
		if (ochanger_cnt) {
			for (ocdp = mc01_changer, tmpcnt = 0; 
			    tmpcnt < mc01_changer_cnt; ocdp++, tmpcnt++) {
				if (! ocdp->mc_addr.pdi_adr_version )
					match = SDI_ADDRCMP(&ocdp->mc_addr,
						&op->edtp->scsi_adr);
				else
					match = SDI_ADDR_MATCH(&(ocdp->mc_addr.extended_address->scsi_adr), &op->edtp->scsi_adr);
				if (match) {
					found = 1;
					break;
				}
			}

			if (found) {
				*mcp = *ocdp;
				if (ocdp->mc_next == (struct mc_job *)ocdp) {
					mcp->mc_last  = (struct mc_job *)mcp;
					mcp->mc_next  = (struct mc_job *)mcp;
				} else {
					mcp->mc_last->j_next = (struct mc_job *)mcp;
					mcp->mc_next->j_prev = (struct mc_job *)mcp;
				}

				found = 0;
				ochanger_cnt--;
				continue;
			}
		}

		if (mc01_chinit(mcp, op)) {
			/*
			 *+ There was insuffcient memory for changer data
			 *+ structures at load-time.
			 */
			cmn_err(CE_WARN,
			    "Medium Changer Error: Insufficient memory to configure driver");

			kmem_free(base, changer_sz);

			splx(prevpl);
			MC01_LOCK(mc01_lock);
			rinit_flag = 0;
			MC01_UNLOCK(mc01_lock);
			SV_BROADCAST(mc01_sv, 0);
			return;
		}
	}

	if (mc01_changer_cnt > 0) {
		kmem_free(mc01_changer, mod_memsz);
	}

	mc01_changer_cnt = new_changer_cnt;
	mc01_changer = (struct changer *)(void *)base;
	mc01_ownerlist = ohp;

	mod_memsz = changer_sz;

	splx(prevpl);

	MC01_LOCK(mc01_lock);
	rinit_flag = 0;
	MC01_UNLOCK(mc01_lock);

	SV_BROADCAST(mc01_sv, 0);
}


#ifndef PDI_SVR42
/*
 * int
 * mc01devinfo(dev_t dev, di_parm_t parm, void **valp)
 *	Get device information.
 *	
 * Calling/Exit State:
 *	The device must already be open.
 */
int
mc01devinfo(dev_t dev, di_parm_t parm, void **valp)
{
	unsigned long	unit;

	unit = UNIT(dev);

	if ((int)unit >= mc01_changer_cnt) {
		return(ENODEV);
	}

	switch (parm) {
	case DI_MEDIA:
		*(char **)valp = "changer";
		return 0;
	default:
		return ENOSYS;
	}
}


#endif

/*
 * mc01getjob() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function will allocate a changer job structure from the free
 *	list.  The function will sleep if there are no jobs available.
 *	It will then get a SCSI block from SDI.
 */
struct mc_job *
mc01getjob()
{
	struct mc_job *jp;

	jp = (struct mc_job *)sdi_get(&lg_poolhead, 0);

	jp->j_sb = sdi_getblk(KM_SLEEP);

	return(jp);
}



/*
 * mc01freejob() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function returns a job structure to the free list. The
 *	SCSI block associated with the job is returned to SDI.
 */
void
mc01freejob(jp)
struct mc_job *jp;
{
	sdi_xfreeblk(jp->j_sb->SCB.sc_dev.pdi_adr_version, jp->j_sb);
	sdi_free(&lg_poolhead, (jpool_t * )jp);
}


/*
 * mc01send() 
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
mc01send(mcp)
struct changer *mcp;
{
	struct mc_job *jp;
	int	rval;

	jp = mcp->mc_next;
	mcp->mc_next = jp->j_next;

	if ((rval = MC01SEND(jp, KM_SLEEP)) != SDI_RET_OK) {

		if (rval == SDI_RET_RETRY) {

			mcp->mc_next = jp;

			if (mcp->mc_state & MC_SEND) {
				return;
			}

			mcp->mc_state |= MC_SEND;
			mcp->mc_sendid = sdi_timeout((void (*)())mc01sendt, (caddr_t)mcp, MC_LATER, pldisk, &mcp->mc_addr);

			return;
		}
		else {
			cmn_err(CE_WARN, "!Medium Changer: MC01SEND Failed");
			mc01comp(jp);
		}
	}

	if (mcp->mc_state & MC_SEND) {
		mcp->mc_state &= ~MC_SEND;
		untimeout(mcp->mc_sendid);
	}
}


/*
 * mc01sendt() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function calls mc01send() after the record of the pending
 *	timeout is erased.
 */
void
mc01sendt(mcp)
struct changer *mcp;
{
	mcp->mc_state &= ~MC_SEND;
	mc01send(mcp);
}


/*
 * mc01open() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 * 	Driver open() entry point.  Determines the type of open being
 *	being requested.  On the first open to a device, the PD and
 *	VTOC information is read. 
 */
/* ARGSUSED */
mc01open(devp, flag, otyp, cred_p)
dev_t	*devp;
int	flag;
int	otyp;
struct cred *cred_p;
{
	struct changer *mcp;
	unsigned	unit;
	pl_t			s;
	dev_t	dev = *devp;
	struct xsb *xsb;

	MC01_LOCK(mc01_lock);
	while (rinit_flag) {
		SV_WAIT(mc01_sv, pridisk, mc01_lock);
		MC01_LOCK(mc01_lock);
	}

	MC01_UNLOCK(mc01_lock);

	unit = UNIT(dev);

	/* Check for non-existent device */
	if ((int)unit >= mc01_changer_cnt) {
		return(ENXIO);
	}

	mcp = &mc01_changer[unit];

	if ((flag & FWRITE) != 0) {
		return(EACCES);
	}

	MC_LOCK(mcp);
	while (mcp->mc_state & MC_WOPEN) {
		SV_WAIT(mcp->mc_sv, pridisk, mcp->mc_lock);
		MC_LOCK(mcp);
	}
	mcp->mc_state |= MC_WOPEN;

	MC_UNLOCK(mcp);
	if (!(mcp->mc_state & MC_INIT)) {
		mcp->mc_fltreq = sdi_getblk(KM_SLEEP);  /* Request sense */
		mcp->mc_fltres = sdi_getblk(KM_SLEEP);  /* Resume */

		xsb = (struct xsb *)mcp->mc_fltreq;
		xsb->extra.sb_data_type = SDI_PHYS_ADDR;

		mcp->mc_fltreq->sb_type = ISCB_TYPE;
		mcp->mc_fltreq->SCB.sc_datapt = SENSE_AD(mcp->mc_sense);
		mcp->mc_fltreq->SCB.sc_datasz = SENSE_SZ;
		xsb->extra.sb_datapt =
                	(paddr32_t *)((char *)mcp->mc_sense + 
				sizeof (struct sense));
		*(xsb->extra.sb_datapt) += 1;
		mcp->mc_fltreq->SCB.sc_mode   = SCB_READ;
		mcp->mc_fltreq->SCB.sc_cmdpt  = SCS_AD(&mcp->mc_fltcmd);

		mcp->mc_fltreq->SCB.sc_dev    = mcp->mc_addr;

		sdi_xtranslate(mcp->mc_addr.pdi_adr_version, 
			mcp->mc_fltreq, B_READ, 0, KM_SLEEP);

		mcp->mc_state |= MC_INIT;
	}

	MC_LOCK(mcp);
	mcp->mc_state &= ~MC_WOPEN;
	MC_UNLOCK(mcp);
	SV_BROADCAST(mcp->mc_sv, 0);

	return(0);
}


/*
 * mc01close() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 * 	Driver close() entry point.  Determine the type of close
 *	being requested.
 */
/* ARGSUSED */
mc01close(dev, flag, otyp, cred_p)
dev_t	dev;
int	flag;
int	otyp;
struct cred *cred_p;
{
	struct changer *mcp;

	unsigned	unit;

	MC01_LOCK(mc01_lock);
	while (rinit_flag) {
		SV_WAIT(mc01_sv, pridisk, mc01_lock);
		MC01_LOCK(mc01_lock);
	}

	 MC01_UNLOCK(mc01_lock);

	unit = UNIT(dev);

	if ((int)unit >= mc01_changer_cnt) {
		return(ENXIO);
	}

	mcp = &mc01_changer[unit];

	if (mcp->mc_spec && mcp->mc_spec->last_close) {
		(*mcp->mc_spec->last_close)(mcp);
	}

	return(0);
}


/*
 * mc01print() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 * 	Driver print() entry point.  Prints an error message on
 *	the system console.
 */
void
mc01print(dev, str)
dev_t	dev;
char	*str;
{
	struct changer *mcp;
	char	name[SDI_NAMESZ];
	int	lun;

	if (! mcp->mc_addr.pdi_adr_version ) 
		lun = mcp->mc_addr.sa_lun;
	else
		lun = SDI_LUN_32(&mcp->mc_addr);

	mcp = &mc01_changer[UNIT(dev)];

	sdi_xname(mcp->mc_addr.pdi_adr_version, &mcp->mc_addr, name);

	cmn_err(CE_WARN, "Medium Changer Error: %s: Logical Unit %d - %s",
	    name, lun, str);
}


/*
 * mc01ioctl() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	Driver ioctl() entry point.  Used to implement the following 
 *	special functions:
 *
 *    B_GETTYPE		-  Get bus type and driver name
 *    
 *  Group 0 commands
 *    C_TESTUNIT	-  Test unit ready
 *
 *  Group 1 commands
 *    C_READCAPA	-  Read capacity
 *
 *  Group 6 commands
 *    C_AUDIOSEARCH	-  Audio track search
 */

/* ARGSUSED */
mc01ioctl(dev, cmd, arg, mode, cred_p, rval_p)
dev_t	dev;
int	cmd;
caddr_t	arg;
int	mode;
struct cred *cred_p;
int	*rval_p;
{
	struct changer *mcp;
	int	uerror;
	int	element_count;

	struct mc_ex_medium ex;
	struct mc_mv_medium mv;
	struct mc_position pos;
	struct mc_rd_status rds;

	caddr_t mc01_mem_alloc();

	MC01_LOCK(mc01_lock);
	while (rinit_flag) {
		SV_WAIT(mc01_sv, pridisk, mc01_lock);
		MC01_LOCK(mc01_lock);
	}

	MC01_UNLOCK(mc01_lock);

	mcp = &mc01_changer[UNIT(dev)];

	uerror = 0;
	switch (cmd) {
	case MC_EXCHANGE:

		if ( copyin( arg, (char *) & ex, sizeof( struct mc_ex_medium ) ) < 0 ) {
			uerror = EFAULT;
			break;
		}

		if ( mc01cmd( mcp, SS_EXCHANGE, &ex, NULL, 0, SCB_READ, 0, 0 ) ) {
			uerror = EIO;
			break;
		}

		if ( uerror == 0 ) {
			mcp->mc_last_loaded = ex.source;
		}

		break;

	case MC_INIT_STATUS:

		if ( mc01cmd( mcp, SS_INIT_STATUS, 0, NULL, 0, 0, SCB_READ, 0, 0 ) ) {
			uerror = EIO;
			break;
		}

		break;

	case MC_MOVE_MEDIUM:

		if ( copyin( arg, (char *)&mv, sizeof( struct mc_mv_medium ) ) < 0 ) {
			uerror = EFAULT;
			break;
		}

		if ( mc01cmd( mcp, SS_MOVE_MEDIUM, &mv, NULL, 0, SCB_READ, 0, 0 ) ) {
			if( mcp->mc_door_closed == 1 ) {
				uerror = EAGAIN;
			}
			else {
				uerror = EIO;
			}
			break;
		}

		if ( uerror == 0 ) {
			mcp->mc_last_loaded = mv.source;
		}

		break;

	case MC_POSITION:

		if ( copyin( arg, (char *)&pos, sizeof( struct mc_position ) ) < 0 ) {
			uerror = EFAULT;
			break;
		}

		if ( mc01cmd( mcp, SS_POSITION, &pos, NULL, 0, 0, SCB_READ, 0, 0 ) ) {
			uerror = EIO;
			break;
		}

		break;

	case MC_ELEMENT_COUNT:

		rds.type = 0;
		rds.voltag = 0;
		rds.start_elem = 0;
		rds.num_elem = 0xFFF;
		rds.len = 8;

		if( (mc_status_hdr = (struct mc_status_hdr *)(void *)mc01_mem_alloc( 8 )) == (struct mc_status_hdr *)NULL ) {
			uerror = ENOMEM;
			break;
		}

		if ( mc01cmd( mcp, SS_RD_STATUS, &rds, mc_status_hdr, sizeof( struct mc_status_hdr ), 0, SCB_READ, 0, 0 ) ) {
			kmem_free( (caddr_t)mc_status_hdr, sizeof( struct mc_status_hdr )  + sizeof (paddr32_t));
			uerror = EIO;
			break;
		}

		element_count = sdi_swap16(mc_status_hdr->num_elem);
		if ( copyout( &element_count, arg, sizeof( element_count ) ) ) {
			kmem_free( (caddr_t)mc_status_hdr, sizeof( struct mc_status_hdr )  + sizeof(paddr32_t));
			uerror = EFAULT;
			break;
		}

		kmem_free( (caddr_t)mc_status_hdr, sizeof( struct mc_status_hdr ) + sizeof(paddr32_t));

		break;

	case MC_STATUS:

		rds.type = 0;
		rds.voltag = 0;
		rds.start_elem = 0;
		rds.num_elem = 0xFFF;
		rds.len = 8;

		if( (mc_status_hdr = (struct mc_status_hdr *)(void *)mc01_mem_alloc( 8 )) == (struct mc_status_hdr *)NULL) {
			uerror = ENOMEM;
			break;
		}

		if ( mc01cmd( mcp, SS_RD_STATUS, &rds, mc_status_hdr, sizeof( struct mc_status_hdr ), 0, SCB_READ, 0, 0 ) ) {
			kmem_free( (caddr_t)mc_status_hdr, sizeof( struct mc_status_hdr )  + sizeof(paddr32_t));
			uerror = EIO;
			break;
		}

		mc_status_hdr->first_elem = sdi_swap16(mc_status_hdr->first_elem);
		mc_status_hdr->num_elem = sdi_swap16(mc_status_hdr->num_elem);
		mc_status_hdr->byte_count = sdi_swap24(mc_status_hdr->byte_count);
		       
		if ( copyout( mc_status_hdr,  arg, sizeof( struct mc_status_hdr ) ) ) {
			kmem_free( (caddr_t)mc_status_hdr, sizeof( struct mc_status_hdr ) + sizeof(paddr32_t));
			uerror = EFAULT;
			break;
		}

		kmem_free( (caddr_t)mc_status_hdr, sizeof( struct mc_status_hdr ) + sizeof(paddr32_t));

		break;

	case MC_MAP:

		rds.type = 0;
		rds.voltag = 0;
		rds.start_elem = 0;
		rds.num_elem = 0xFFF;
		rds.len = 8;

		if( (mc_status_hdr = (struct mc_status_hdr *)(void *)mc01_mem_alloc( 8 )) == (struct mc_status_hdr *)NULL ) {
			uerror = ENOMEM;
			break;
		}

		if ( mc01cmd( mcp, SS_RD_STATUS, &rds, mc_status_hdr, sizeof( struct mc_status_hdr ), 0, SCB_READ, 0, 0 ) ) {
			kmem_free( (caddr_t)mc_status_hdr, sizeof( struct mc_status_hdr ) + sizeof(paddr32_t));
			uerror = EIO;
			break;
		}

		rds.type = 0;
		rds.voltag = 0;
		rds.start_elem = 0;
		rds.num_elem = sdi_swap16(mc_status_hdr->num_elem);
		rds.len = (sdi_swap24(mc_status_hdr->byte_count) + 8);

		kmem_free( (caddr_t)mc_status_hdr, sizeof( struct mc_status_hdr ) + sizeof(paddr32_t));

		if( (mc_status_hdr = (struct mc_status_hdr *)(void *)mc01_mem_alloc( rds.len )) == (struct mc_status_hdr *)NULL ) {
			uerror = ENOMEM;
			break;
		}

		if ( mc01cmd( mcp, SS_RD_STATUS, &rds, mc_status_hdr, rds.len, 0, SCB_READ, 0, 0 ) ) {
			kmem_free( (caddr_t)mc_status_hdr, rds.len + sizeof(paddr32_t) );
			uerror = EIO;
			break;
		}

		if ( copyout( mc_status_hdr, arg, rds.len) ) {
			kmem_free( (caddr_t)mc_status_hdr, rds.len+sizeof(paddr32_t));
			uerror = EFAULT;
			break;
		}

		kmem_free( (caddr_t)mc_status_hdr, rds.len+sizeof(paddr32_t) );
		break;

	case B_GETTYPE:

		if (copyout("scsi", ((struct bus_type *)arg)->bus_name, 5)) {
			uerror = EFAULT;
			break;
		}

		if (copyout("mc01", ((struct bus_type *)arg)->drv_name, 5)) {
			uerror = EFAULT;
			break;
		}

		break;

	case B_GETDEV:

		 {
			dev_t	pdev;

			sdi_getdev(&mcp->mc_addr, &pdev);

			if (copyout((caddr_t) & pdev, arg, sizeof(pdev))) {
				uerror = EFAULT;
			}
		}

		break;

	case MC_LAST_LOADED:

		if (copyout((caddr_t) & mcp->mc_last_loaded, arg, sizeof(mcp->mc_last_loaded))) {
			uerror = EFAULT;
		}

		break;

	case MC_PREVMR:
		
		if( mc01cmd(mcp, SS_LOCK, 0, NULL, 0, 1, SCB_READ, 0, 0)) {
			uerror = ENXIO;
		}

		break;

	case MC_ALLOWMR:
		
		if( mc01cmd(mcp, SS_LOCK, 0, NULL, 0, 0, SCB_READ, 0, 0)) {
			uerror = ENXIO;
		}

		break;

	case SDI_RELEASE:

		if (mc01cmd(mcp, SS_RELES, 0, NULL, 0, 0, SCB_READ, 0, 0)) {
			uerror = ENXIO;
		}

		break;

	case SDI_RESERVE:

		if (mc01cmd(mcp, SS_RESERV, 0, NULL, 0, 0, SCB_READ, 0, 0)) {
			uerror = ENXIO;
		}

		break;

	default:
		uerror = EINVAL;
	}

	return(uerror);
}


/*
 * mc01cmd() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function performs a SCSI command such as Mode Sense on
 *	the addressed changer.  The op code indicates the type of job
 *	but is not decoded by this function.  The data area is
 *	supplied by the caller and assumed to be in kernel space. 
 *	This function will sleep.
 */
mc01cmd(mcp, op_code, addr, buffer, size, length, mode, param, control)
struct changer *mcp;
unsigned char op_code;	/* Command opcode		*/
unsigned int addr;	/* Address field of command 	*/
char *buffer;		/* Buffer for command data 	*/
unsigned int size;	/* Size of the data buffer 	*/
unsigned int length;	/* Block length in the CDB	*/
unsigned short mode;	/* Direction of the transfer 	*/
unsigned int param;	/* Parameter bit		*/
unsigned int control;	/* Control byte			*/
{
	struct mc_job *jp;
	struct scb *scb;
	struct xsb *xsb;

	buf_t		 * bp;

	int	error;

	pl_t	s;

	bp = getrbuf(KM_SLEEP);
	jp = mc01getjob();

	scb = &jp->j_sb->SCB;

	xsb = (struct xsb *)jp->j_sb;
	bp->b_iodone = NULL;
	bp->b_blkno = addr;
	bp->b_flags |= mode & SCB_READ ? B_READ : B_WRITE;
	bp->b_error = 0;

	jp->j_bp = bp;
	jp->j_cdp = mcp;
	jp->j_errcnt = 0;
	jp->j_sb->sb_type = SCB_TYPE;

	mcp->mc_door_closed = 0;

	if ( op_code == 0xA6 ) {		/*  Exchange Medium */

		struct sce *cmd;
		struct mc_ex_medium *ex;

		ex = (struct mc_ex_medium *)addr;

		cmd = (struct sce *) & jp->j_cmd.se;

		cmd->se_op = op_code;
		cmd->se_resv1 = 0;
		if (! mcp->mc_addr.pdi_adr_version )
			cmd->se_lun  = mcp->mc_addr.sa_lun;
		else
			cmd->se_lun  = SDI_LUN_32(&mcp->mc_addr);

		cmd->se_lba = sdi_swap32( (ex->transport << 16) | ex->source );
		cmd->se_len = sdi_swap32( (ex->first_dest << 16) | ex->second_dest );

		cmd->se_resv2 = 0;
		cmd->se_control = control & 0xc0;

		scb->sc_cmdpt = SCE_AD(cmd);
		scb->sc_cmdsz = SCE_SZ;

	} else if ( op_code == 0xA5 ) {		/* Move Medium */

		struct sce *cmd;
		struct mc_mv_medium *mv;

		mv = (struct mc_mv_medium *)addr;

		cmd = (struct sce *) & jp->j_cmd.se;

		cmd->se_op = op_code;
		cmd->se_resv1 = 0;
		if (! mcp->mc_addr.pdi_adr_version )
			cmd->se_lun  = mcp->mc_addr.sa_lun;
		else
			cmd->se_lun  = SDI_LUN_32(&mcp->mc_addr);

		cmd->se_lba = sdi_swap32( (mv->transport << 16) | mv->source );
		cmd->se_len = sdi_swap32( (mv->dest << 16) );

		cmd->se_resv2 = 0;
		cmd->se_control = control & 0xc0;

		scb->sc_cmdpt = SCE_AD(cmd);
		scb->sc_cmdsz = SCE_SZ;

	} else if (op_code == 0x2B ) {		/* Position To Element */

		struct sce *cmd;
		struct mc_position *pos;

		pos = (struct mc_position *)addr;

		cmd = (struct sce *) & jp->j_cmd.se;

		cmd->se_op = op_code;
		cmd->se_resv1 = 0;
		if (! mcp->mc_addr.pdi_adr_version )
			cmd->se_lun  = mcp->mc_addr.sa_lun;
		else
			cmd->se_lun  = SDI_LUN_32(&mcp->mc_addr);

		cmd->se_lba = sdi_swap32( (pos->transport << 16) | pos->dest );
		cmd->se_len = 0;

		cmd->se_resv2 = 0;
		cmd->se_control = control & 0xc0;

		scb->sc_cmdpt = SCE_AD(cmd);
		scb->sc_cmdsz = SCE_SZ;

	} else if (op_code == 0xB8 ) {		/* Read Element Status */

		struct sce *cmd;
		struct mc_rd_status *st;

		st = (struct mc_rd_status *)addr;

		cmd = (struct sce *) & jp->j_cmd.se;

		cmd->se_op = op_code;

		cmd->se_resv1 = 0;
		if (! mcp->mc_addr.pdi_adr_version )
			cmd->se_lun  = mcp->mc_addr.sa_lun;
		else
			cmd->se_lun  = SDI_LUN_32(&mcp->mc_addr);
		cmd->se_lba = sdi_swap32( (st->start_elem << 16) | st->num_elem);
		cmd->se_len = sdi_swap32( st->len );

		cmd->se_resv2 = 0;
		cmd->se_control = control & 0xc0;

		scb->sc_cmdpt = SCE_AD(cmd);
		scb->sc_cmdsz = SCE_SZ;

	} else if (op_code & 0x60) {		/* Group 6 commands */
		struct scm *cmd;

		cmd = (struct scm *) & jp->j_cmd.sv;
		cmd->sm_op   = op_code;
		if (! mcp->mc_addr.pdi_adr_version )
			cmd->sm_lun  = mcp->mc_addr.sa_lun;
		else
			cmd->sm_lun  = SDI_LUN_32(&mcp->mc_addr);
		cmd->sm_res1 = 0;
		cmd->sm_res1 = param;
		cmd->sm_addr = sdi_swap32(addr);
		cmd->sm_res2 = 0;
		cmd->sm_cont = control & 0xc0;

		scb->sc_cmdpt = SCM_AD(cmd);
		scb->sc_cmdsz = SCM_SZ;
	} else if (op_code & 0x20) {
		struct scm *cmd;

		cmd = (struct scm *) & jp->j_cmd.sm;
		cmd->sm_op   = op_code;
		if (! mcp->mc_addr.pdi_adr_version )
			cmd->sm_lun  = mcp->mc_addr.sa_lun;
		else
			cmd->sm_lun  = SDI_LUN_32(&mcp->mc_addr);
		cmd->sm_res1 = 0;
		cmd->sm_addr = sdi_swap32(addr);
		cmd->sm_res2 = 0;
		cmd->sm_len  = sdi_swap16(length);
		cmd->sm_cont = 0;

		scb->sc_cmdpt = SCM_AD(cmd);
		scb->sc_cmdsz = SCM_SZ;
	} else {		  /* Group 0 commands */
		struct scs *cmd;

		cmd = (struct scs *) & jp->j_cmd.ss;
		cmd->ss_op    = op_code;
		if (! mcp->mc_addr.pdi_adr_version )	
			cmd->ss_lun   = mcp->mc_addr.sa_lun;
		else
			cmd->ss_lun   = SDI_LUN_32(&mcp->mc_addr);
		cmd->ss_addr1 = (addr & 0x1F0000);
		cmd->ss_addr = sdi_swap16(addr & 0xFFFF);
		cmd->ss_len   = ( short )length;
		cmd->ss_cont  = 0;

		scb->sc_cmdpt = SCS_AD(cmd);
		scb->sc_cmdsz = SCS_SZ;
	}

	scb->sc_int = mc01intn;
	scb->sc_dev = mcp->mc_addr;
	scb->sc_datapt = buffer;
	if (buffer) {
		xsb->extra.sb_datapt = (paddr32_t *)
                        ((char *)buffer + size);
                xsb->extra.sb_data_type = SDI_PHYS_ADDR;
        }
	scb->sc_datasz = size;
	scb->sc_mode = mode;
	scb->sc_resid = 0;
	scb->sc_time = MC_JTIME;
	scb->sc_wd = (long) jp;

	sdi_xtranslate(jp->j_sb->SCB.sc_dev.pdi_adr_version,
		 jp->j_sb, bp->b_flags, NULL, KM_SLEEP);

	s = spldisk();

	mcp->mc_count++;

	jp->j_next = (struct mc_job *) mcp;
	jp->j_prev = mcp->mc_last;

	mcp->mc_last->j_next = jp;
	mcp->mc_last = jp;

	if (mcp->mc_next == (struct mc_job *) mcp) {
		mcp->mc_next = jp;
	}

	splx(s);

	mc01send(mcp);

	biowait(bp);

	error = bp->b_flags & B_ERROR;

	freerbuf(bp);

	return(error);
}


/*
 * mc01comp() 
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
mc01comp(jp)
struct mc_job *jp;
{
	struct changer *mcp;
	struct buf *bp;

	mcp = jp->j_cdp;
	bp  = jp->j_bp;

	jp->j_next->j_prev = jp->j_prev;
	jp->j_prev->j_next = jp->j_next;

	mcp->mc_count--;

	if (jp->j_sb->SCB.sc_comp_code != SDI_ASW) {

		bp->b_flags |= B_ERROR;

		if (jp->j_sb->SCB.sc_comp_code == SDI_NOSELE) {
			bioerror( bp, ENODEV );
		} else {
			bioerror( bp, EIO );
		}
	}

	biodone(bp);

	mc01freejob(jp);

	if (mcp->mc_state & MC_SUSP) {

		mcp->mc_fltres->sb_type = SFB_TYPE;
		mcp->mc_fltres->SFB.sf_int  = mc01intf;
		mcp->mc_fltres->SFB.sf_dev  = mcp->mc_addr;
		mcp->mc_fltres->SFB.sf_wd = (long) mcp;
		mcp->mc_fltres->SFB.sf_func = SFB_RESUME;

		if (MC01ICMD(mcp, mcp->mc_fltres, KM_NOSLEEP) == SDI_RET_OK) {
			mcp->mc_state &= ~MC_SUSP;
			mcp->mc_fltcnt = 0;
		}
	}

	return;
}


/*
 * mc01intn()
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
mc01intn(sp)
struct sb *sp;
{
	struct changer *mcp;
	struct mc_job *jp;

	jp = (struct mc_job *)sp->SCB.sc_wd;
	mcp = jp->j_cdp;

	if (sp->SCB.sc_comp_code == SDI_ASW) {
		mc01comp(jp);
		return;
	}

	if (sp->SCB.sc_comp_code & SDI_SUSPEND) {
		mcp->mc_state |= MC_SUSP;
	}

	if (sp->SCB.sc_comp_code == SDI_CKSTAT && sp->SCB.sc_status == S_CKCON) {

		struct sense *sensep = sdi_sense_ptr(sp);
		mcp->mc_fltjob = jp;

		if ((sensep->sd_key != SD_NOSENSE) || sensep->sd_fm 
		  || sensep->sd_eom || sensep->sd_ili) {
			bcopy(sensep, mcp->mc_sense,sizeof(struct sense));
			mc01sense(mcp);
			return;
		}
		mcp->mc_fltreq->sb_type = ISCB_TYPE;
		mcp->mc_fltreq->SCB.sc_int = mc01intrq;
		mcp->mc_fltreq->SCB.sc_cmdsz = SCS_SZ;
		mcp->mc_fltreq->SCB.sc_time = MC_JTIME;
		mcp->mc_fltreq->SCB.sc_mode = SCB_READ;
		mcp->mc_fltreq->SCB.sc_dev = sp->SCB.sc_dev;
		mcp->mc_fltreq->SCB.sc_wd = (long) mcp;
		mcp->mc_fltcmd.ss_op = SS_REQSEN;
		if (! sp->SCB.sc_dev.pdi_adr_version )
			mcp->mc_fltcmd.ss_lun = sp->SCB.sc_dev.sa_lun;
		else
			mcp->mc_fltcmd.ss_lun = SDI_LUN_32(&sp->SCB.sc_dev);
		mcp->mc_fltcmd.ss_addr1 = 0;
		mcp->mc_fltcmd.ss_addr = 0;
		mcp->mc_fltcmd.ss_len = SENSE_SZ;
		mcp->mc_fltcmd.ss_cont = 0;

		/* Clear old sense key */
		mcp->mc_sense->sd_key = SD_NOSENSE;

		if (MC01ICMD(mcp, mcp->mc_fltreq, KM_NOSLEEP) == SDI_RET_OK) {
			return;
		}
	}

	if (sp->SCB.sc_comp_code & SDI_RETRY && ++jp->j_errcnt <= MC_MAXRETRY) {

		sp->sb_type = ISCB_TYPE;
		sp->SCB.sc_time = MC_JTIME;

		if (MC01ICMD(mcp, sp, KM_NOSLEEP) == SDI_RET_OK) {
			return;
		}
	}

	mc01logerr(mcp, sp);
	mc01comp(jp);
}


/*
 * mc01intrq()
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function is called by the host adapter driver when a
 *	request sense job completes.  The job will be retried if it
 *	failed.  Calls mc01sense() on sucessful completions to
 *	examine the request sense data.
 */
void
mc01intrq(sp)
struct sb *sp;
{
	struct changer *mcp;

	mcp = (struct changer *)sp->SCB.sc_wd;

	if (sp->SCB.sc_comp_code != SDI_CKSTAT  && 
	    sp->SCB.sc_comp_code &  SDI_RETRY   && 
	    ++mcp->mc_fltcnt <= MC_MAXRETRY) {

		sp->SCB.sc_time = MC_JTIME;

		if (MC01ICMD(mcp, sp, KM_NOSLEEP) == SDI_RET_OK) {
			return;
		}
	}

	if (sp->SCB.sc_comp_code != SDI_ASW) {

		mc01logerr(mcp, sp);
		mc01comp(mcp->mc_fltjob);
		return;
	}

	mc01sense(mcp);
}


/*
 * mc01intf() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function is called by the host adapter driver when a host
 *	adapter function request has completed.  If there was an error
 *	the request is retried.  Used for resume function completions.
 */
void
mc01intf(sp)
struct sb *sp;
{
	struct changer *mcp;

	mcp = (struct changer *)sp->SFB.sf_wd;

	if (sp->SFB.sf_comp_code & SDI_RETRY && ++mcp->mc_fltcnt <= MC_MAXRETRY) {
		if (MC01ICMD(mcp, sp) == SDI_RET_OK) {
			return;
		}
	}

	if (sp->SFB.sf_comp_code != SDI_ASW) {
		mc01logerr(mcp, sp);
	}
}


/*
 * mc01sense()
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function uses the Request Sense information to determine
 *	what to do with the original job.
 */
void
mc01sense(mcp)
struct changer *mcp;
{
	struct mc_job *jp;
	struct sb *sp;

	jp = mcp->mc_fltjob;
	sp = jp->j_sb;

	switch (mcp->mc_sense->sd_key) {
	case SD_NOSENSE:
	case SD_ABORT:
	case SD_VENUNI:

		mc01logerr(mcp, sp);

		/* FALLTHRU */
	case SD_UNATTEN:

		if (++jp->j_errcnt > MC_MAXRETRY) {
			mc01comp(jp);
		} else {
			sp->sb_type = ISCB_TYPE;
			sp->SCB.sc_time = MC_JTIME;

			if (MC01ICMD(mcp, sp, KM_NOSLEEP) != SDI_RET_OK) {
				mc01logerr(mcp, sp);
				mc01comp(jp);
			}
		}
		break;

	case SD_RECOVER:

		mc01logerr(mcp, sp);
		sp->SCB.sc_comp_code = SDI_ASW;
		mc01comp(jp);

		break;

	case SD_ILLREQ:

		if( mcp->mc_sense->sd_sencode == 0x3B && mcp->mc_sense->sd_qualifier == 0x83 ) {
			mcp->mc_door_closed = 1;
		}
		else {
			mc01logerr(mcp, sp);
		}

		mc01comp(jp);

		break;

	default:

		mc01logerr(mcp, sp);
		mc01comp(jp);
	}
}


/*
 * mc01logerr()
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 *	This function will print the error messages for errors detected
 *	by the host adapter driver.  No message will be printed for
 *	thoses errors that the host adapter driver has already reported.
 */
void
mc01logerr(mcp, sp)
struct changer *mcp;
struct sb *sp;
{
	if (sp->sb_type == SFB_TYPE) {
		sdi_errmsg("Medium Changer", &mcp->mc_addr, sp, mcp->mc_sense, SDI_SFB_ERR, 0);
		return;
	}

	if (sp->SCB.sc_comp_code == SDI_CKSTAT && sp->SCB.sc_status == S_CKCON) {
		sdi_errmsg("Medium Changer", &mcp->mc_addr, sp, mcp->mc_sense, SDI_CKCON_ERR, 0);
		return;
	}

	if (sp->SCB.sc_comp_code == SDI_CKSTAT) {
		sdi_errmsg("Medium Changer", &mcp->mc_addr, sp, mcp->mc_sense, SDI_CKSTAT_ERR, 0);
	}
}


extern sdi_xsend(), sdi_xicmd();

/*
 * MC01SEND() 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 */
MC01SEND(jobp, sleepflag)
struct mc_job *jobp;
int	sleepflag;
{
	return mc01docmd(sdi_xsend, jobp->j_cdp, jobp->j_sb, sleepflag);
}


/*
 * MC01ICMD()
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 */
MC01ICMD(mcp, sbp, sleepflag)
struct changer *mcp;
struct sb *sbp;
int	sleepflag;
{
	return( mc01docmd(sdi_xicmd, mcp, sbp, sleepflag) );
}


/*
 * int mc01docmd(int (*fcn)(), struct changer *mcp, struct sb *sbp, int sleepflag) 
 *
 * Calling/Exit State:
 *
 * Descriptions: 
 */
int
mc01docmd(fcn, mcp, sbp, sleepflag)
int	(*fcn)();
struct changer *mcp;
struct sb *sbp;
int	sleepflag;
{
	int	cmd;
	int	pdi_adr_version;

	struct dev_spec	*dsp;

	dsp = mcp->mc_spec;

	if (sbp->sb_type == SFB_TYPE)
		pdi_adr_version = sbp->SFB.sf_dev.pdi_adr_version;
	else
		pdi_adr_version = sbp->SCB.sc_dev.pdi_adr_version;
	if (dsp && sbp->sb_type != SFB_TYPE) {

		cmd = ((struct scs *)(void *) sbp->SCB.sc_cmdpt)->ss_op;

		if (!CMD_SUP(cmd, dsp)) {
			return SDI_RET_ERR;
		} else if (dsp->command && CMD_CHK(cmd, dsp)) {
			(*dsp->command)(mcp, sbp);
		}
	}

	return (*fcn)(pdi_adr_version, sbp, sleepflag);
}

/*
 * caddr_t mc01_mem_alloc( int size )
 *
 * Description:
 *	Allocate physically contiguous memory in a DDI compliant fashion.
 *	Returns NULL if the requested memory could not be allocated.
 *
 * Calling/Exit State:
 *	None.
 */

#if (PDI_VERSION <= 1)

caddr_t
mc01_mem_alloc( int size )
{
	caddr_t	ptr;

	kmem_alloc_physcontig (size, (paddr_t)ADSC_MEMALIGN, (paddr_t)ADSC_BOUNDARY, flags);

	if ( ptr ) {
		bzero ( ptr, size );
	}

	return( ptr );
#else

caddr_t
mc01_mem_alloc( int size )
{
	caddr_t	ptr;

	physreq_t *preq;

	preq = physreq_alloc(KM_NOSLEEP);

	if (preq == NULL) {
		return( NULL );
	}

	preq->phys_align = 512;
	preq->phys_boundary = 0;
	preq->phys_dmasize = 24;
	preq->phys_flags |= PREQ_PHYSCONTIG;

	if (!physreq_prep(preq, KM_NOSLEEP)) {
		physreq_free(preq);
		return( NULL );
	}

	ptr = (caddr_t)sdi_kmem_alloc_phys(size, preq, 0);

	physreq_free(preq);

	return( ptr );
}

#endif
