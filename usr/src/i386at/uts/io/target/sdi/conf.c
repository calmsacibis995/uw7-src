#ident	"@(#)kern-pdi:io/target/sdi/conf.c	1.22.5.1"
#ident	"$Header$"

#include <util/cmn_err.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <io/target/sdi/sdi.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi_hier.h>
#include <mem/kmem.h>
#include <util/param.h>
#include <util/types.h>

#include <io/ddi.h>	/* must come last */

/*
#define SDI_DEBUG	1
*/

extern sleep_t	*sdi_rinit_lock;
extern int	sdi_in_rinits;
extern lock_t	*sdi_edt_mutex;
extern pl_t 	sdi_edt_pl;

extern void  (**sdi_rinits)();

extern int   sdi_rtabsz;
extern int   sdi_hacnt;
STATIC struct owner *addrmatch();
STATIC struct owner *getowner();

/*
 * struct owner *
 * sdi_doconfig(struct dev_cfg dev_cfg[], int dev_cfg_size, char *drvrname,
 * 			struct drv_majors *drv_maj, void (*rinit)())
 *	Find devices specified by the target driver in the EDT.
 *
 * Calling/Exit State:
 *	if called from system init/start, no locks are held on entry/exit
 *		and none are acquired.
 *	else if are called from the sdi_register() -> target_rinit() path,
 *		(e.g., dynamic load), in_sdi_rinits is set and sdi_rinit_lock
 *		is held across the call.
 *	else, no locks are held on entry or exit, and we acquire
 *		the sdi_rinit_lock here.
 */
struct owner *
sdi_doconfig(struct dev_cfg dev_cfg[], int dev_cfg_size, char *drvrname,
		struct drv_majors *drv_maj, void (*rinit)())
{
	int h, s, l, b, i;
	struct owner *op, *head = NULL, *tail = NULL;
	int nbus, ntargets, nluns;
	HBA_IDATA_STRUCT *idatap;

	if ( sdi_sleepflag == KM_SLEEP && !sdi_in_rinits )
		SLEEP_LOCK(sdi_rinit_lock, pridisk);

#ifdef SDI_DEBUG
	cmn_err(CE_CONT,"sdi_doconfig( %x, %x, %x)\n",
		dev_cfg, dev_cfg_size, drvrname, drv_maj);
#endif

	for (h=0; h < sdi_hacnt; h++) {
		if (!(idatap = IDP(HBA_tbl[h].idata)))
			continue;
		if ((idatap->version_num & HBA_VMASK) >= PDI_UNIXWARE20) {
			nbus = idatap->idata_nbus;
			ntargets = idatap->idata_ntargets;
			nluns = idatap->idata_nluns;
		} else {
			nbus = 1;
			ntargets = MAX_TCS;
			nluns = MAX_LUS;
		}
	    for (b=0; b < nbus; b++) {
		for (s=0; s < ntargets; s++) {
			for (l=0; l < nluns; l++) {
				if (op = addrmatch(h,s,l,b, dev_cfg, 
					  dev_cfg_size, drvrname, drv_maj)) {
					op->target_link = NULL;
					if (!tail) {
						head = tail = op;
					} else {
						tail->target_link = op;
						tail = op;
					}
				}
			}
		}
	    }
	}

#ifdef SDI_DEBUG
	cmn_err(CE_CONT,"sdi_doconfig return %x\n", head);
#endif
	if (rinit != NULL) {
		if (dev_cfg[0].match_type & SDI_REMOVE) {
			for (i=0; i < sdi_rtabsz; i++) {
				if (sdi_rinits[i] == rinit) {
					sdi_rinits[i] = NULL;
					break;
				}
			}
		}
		else {
			for (i=0; i < sdi_rtabsz; i++) {
				if (sdi_rinits[i] == rinit)
					break;

				if (sdi_rinits[i] == NULL) {
					sdi_rinits[i] = rinit;
					break;
				}
			}
		}
	}
	if ( sdi_sleepflag == KM_SLEEP && !sdi_in_rinits )
		SLEEP_UNLOCK(sdi_rinit_lock);
	return head;
}

/*
 * void
 * sdi_clrconfig(struct owner *op, int flags, void (*rinit)())
 *	disclaim/remove target driver as owner of device
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires sdi_rinit_lock.
 */
void
sdi_clrconfig(struct owner *op, int flags, void (*rinit)())
{
	struct	owner	*tmpop;
	int	i;

	for (; op != NULL; op = tmpop) {
		/* sdi_access() clears target_link */
		tmpop = op->target_link;
		sdi_access(op->edtp, flags, op);
	}

	SLEEP_LOCK(sdi_rinit_lock, pridisk);
	if (rinit != NULL) {
		for (i=0; i < sdi_rtabsz; i++) {
			if (sdi_rinits[i] == rinit) {
				sdi_rinits[i] = NULL;
				break;
			}
		}
	}
	SLEEP_UNLOCK(sdi_rinit_lock);
}

/*
 * struct owner *
 * sdi_unlink_target(struct owner *oplist, struct owner *op)
 *	Unlink the op owner structure from the oplist list of owner structures
 * linked via the target_link field and return the resulting owner list.  It
 * set the op->target_link field to NULL, but it does not free up memory used
 * by the op owner structure.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
struct owner *
sdi_unlink_target(struct owner *oplist, struct owner *op)
{
	struct	owner	*tmpop;

	if (op == NULL) {
		return (oplist);
	}
	if (oplist == NULL) {
		op->target_link = NULL;
		return (oplist);
	}
	if (oplist == op) {
		oplist = op->target_link;
		op->target_link = NULL;
		return (oplist);
	}
	for (tmpop = oplist; tmpop != NULL; tmpop = tmpop->target_link) {
		if (tmpop->target_link == op) {
			tmpop->target_link = op->target_link;
			break;
		}
	}
	op->target_link = NULL;
	return (oplist);
}

/*
 * struct owner *
 * addrmatch(int h, int s, int l, int b, struct dev_cfg dev_cfg[], 
 *	int dev_cfg_size, char *drvrname, struct drv_majors *drv_maj)
 *
 * Calling/Exit State:
 *	sdi_rinit_lock held across call, unless called at init/start.
 *	acquires sdi_edt_mutex.
 */
struct owner *
addrmatch(int h, int s, int l, int b, struct dev_cfg dev_cfg[], 
	int dev_cfg_size, char *drvrname, struct drv_majors *drv_maj)
{
	int i;
	struct sdi_edt dummy_edt;
	struct sdi_edt *edtp;
	struct owner *op;
	int matchidx = -1;

	if (!(edtp = sdi_rxedt(h, b, s, l)) ) {
		edtp = &dummy_edt;
		edtp->scsi_adr.scsi_ctl =  h;
		edtp->scsi_adr.scsi_target =  s;
		edtp->scsi_adr.scsi_lun =  l;
		edtp->scsi_adr.scsi_bus =  b;
		sdi_edt_pl = LOCK(sdi_edt_mutex, pldisk); /* protect pdtype */
		edtp->pdtype = -1;
		UNLOCK(sdi_edt_mutex, sdi_edt_pl);
	}

	for (i=0; i < dev_cfg_size; i++) {
		int isaddr, istype;

		isaddr =(dev_cfg[i].hba_no == DEV_CFG_UNUSED ||
			(dev_cfg[i].hba_no == h &&
				(dev_cfg[i].scsi_id == DEV_CFG_UNUSED ||
				(dev_cfg[i].scsi_id == s &&
					(dev_cfg[i].lun == DEV_CFG_UNUSED ||
					(dev_cfg[i].lun == l &&
					(dev_cfg[i].bus == DEV_CFG_UNUSED ||
						(dev_cfg[i].bus == b))))))));

		if (!isaddr) {
			continue;
		}

		istype =(dev_cfg[i].devtype == 0xff ||
			    dev_cfg[i].devtype == edtp->pdtype) &&
			(!dev_cfg[i].inq_len ||
			    strncmp(edtp->inquiry, dev_cfg[i].inquiry,
					    dev_cfg[i].inq_len) == 0);

		if (!istype) {
			continue;
		}

		if (dev_cfg[i].match_type & SDI_REMOVE) {
			return (struct owner *)NULL;
		}

		if (matchidx == -1) {
			matchidx = i;
		}
	}

	if (matchidx != -1) {
#ifdef SDI_DEBUG
		cmn_err(CE_CONT,
			"addrmatch: i = %x  match_idx = %x\n", i, matchidx);
		cmn_err(CE_CONT,"dev_cfg  = %x match_type = %x\n",
		 	dev_cfg, dev_cfg[matchidx].match_type  );
#endif
		op = getowner(edtp, drvrname, drv_maj,
				dev_cfg[matchidx].match_type | SDI_ADD);
		if (op) {
#ifdef SDI_DEBUG
			cmn_err(CE_CONT,"addrmatch: return %x\n", op);
#endif
			return op;
		}
	}
	return (struct owner *)NULL;
}

/*
 * struct owner *
 * getowner(struct sdi_edt *edtp, char *drvrname, struct drv_majors *drv_maj,
 * 							int access)
 *
 * Calling/Exit State:
 * 	sdi_rinit_lock held across call, unless called at init/start.
 */
struct owner *
getowner(struct sdi_edt *edtp, char *drvrname, struct drv_majors *drv_maj,
							int access)
{
	struct owner *op;
#ifdef SDI_DEBUG
	cmn_err(CE_CONT,
		"getowner(%x, %x, %x, %x)\n",edtp, drvrname, drv_maj, access);
#endif

	if(edtp->pdtype >= 0)	{
		for(op = edtp->owner_list; op; op = op->next)	{
			if(op->maj.b_maj == drv_maj->b_maj &&
			   op->maj.c_maj == drv_maj->c_maj)	{
				break;
			}
		}
		if(op)	{
			if(edtp->curdrv != op)	{
				if(sdi_access(edtp,
				(access&~SDI_ADD)|SDI_CLAIM, op)
				!= SDI_RET_OK)  {
					return (struct owner *)NULL;
				}
			}
			return op;
		}
	}

	if ( !(op = kmem_zalloc(sizeof(struct owner), sdi_sleepflag)) ) {
		return( (struct owner *)NULL );
	}

	if ( !(op->name = kmem_alloc(strlen(drvrname)+1, sdi_sleepflag)) ) {
		return( (struct owner *)NULL );
	}
	strcpy(op->name, drvrname);
	op->maj = *drv_maj;
	op->edtp = edtp;

	if (sdi_access(edtp, access, op) != SDI_RET_OK) {
#ifdef SDI_DEBUG
		cmn_err(CE_CONT,"getowner: sdi_access failed\n");
#endif
		kmem_free(op->name, strlen(op->name)+1);
		kmem_free(op, sizeof(struct owner));
		return (struct owner *)NULL;
	}

#ifdef SDI_DEBUG
	cmn_err(CE_CONT,"getowner return %x\n", op);
#endif
	return op;
}

/*
 * struct dev_spec *
 * sdi_findspec(struct sdi_edt *edtp, struct dev_spec *dev_spec[])
 *	find matching device specification routine.
 *
 * Calling/Exit State:
 *	None.
 */
struct dev_spec *
sdi_findspec(struct sdi_edt *edtp, struct dev_spec *dev_spec[])
{
	register int i;

	for (i = 0; dev_spec[i]; i++) {
		if (!strncmp(dev_spec[i]->inquiry, edtp->inquiry, INQ_LEN)) {
#ifdef SDI_DEBUG
			cmn_err(CE_CONT,"sdi_findspec: %s: return %x\n",
		 		dev_spec[i]->inquiry, dev_spec[i]);
#endif
			return dev_spec[i];
		}
	}

	return (struct dev_spec *)NULL;
}
