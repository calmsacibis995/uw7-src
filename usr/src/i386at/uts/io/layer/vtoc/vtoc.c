#ident	"@(#)kern-pdi:io/layer/vtoc/vtoc.c	1.5.24.5"
#ident	"$Header$"

#define _DDI_C	/* We're a hybrid between ddi 7 and 8 */

#include <svc/errno.h>
#include <fs/buf.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/conf.h>
#include <io/ddi_i386at.h>
#include <io/open.h>	/* Unnecessary in DDI8 */
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi_layer.h>
#include <io/vtoc.h>
#include <io/uio.h>
#include <proc/cred.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/mod/moddefs.h>
#include <util/param.h>
#include <util/types.h>

/*
 * Certain include files used for ioctls remain under sd01.
 */
#include <io/target/sd01/fdisk.h>
#include <io/target/sd01/sd01.h>
#include <io/target/sd01/sd01_ioctl.h>

#include "vtocdebug.h"
#include "vtocdef.h"
#include "vtocos5.h"

#include	<io/ddi.h>	/* MUST come last */

#ifdef DEBUG
int vtocDbFlag = 0x00;
#endif

/*
 * Forward declarations.
 */
int
	vtocstart(void),
	vtocopen(dev_t *, int, int, cred_t *),
	vtocclose(dev_t, int, int, cred_t *),
	vtocdevinfo(dev_t, di_parm_t, void **),
	vtocioctl(dev_t, int, int, int, cred_t *, int *);

STATIC int
	vtoc_config(cfg_func_t, void *, rm_key_t),
	vtocopen8(void *, ulong_t *, int, cred_t *, queue_t *),
	vtocdevinfo8(void *, ulong_t, di_parm_t, void *),
	vtocioctl8(void *, ulong_t, int, void *, int, cred_t *, int *),
	vtocdevctl8(void *, ulong_t, int, void *),
	vtocclose8(void *, ulong_t, int, cred_t *, queue_t *);

STATIC void
vtocbiostart8(void *, ulong_t, buf_t *);

void vtocstrategy(buf_t *);
void vtocintn(buf_t *);
STATIC int vtoc_add_instance(void *, rm_key_t);
STATIC int vtoc_rm_instance(void *);
STATIC int vtoc_IsAnyOpen(vtoc_t *);
STATIC int vtoc_spec_parts(struct ipart *, vtoc_t *);
STATIC void vtocAlignDivvy(uchar_t *, struct partable *);
STATIC int vtocDivToVtoc(vtoc_t *, struct partable *);
STATIC int vtoc_FetchDivvy(vtoc_t *, uchar_t *);
STATIC void vtocunload(vtoc_t *);
STATIC void vtoc_biostart(buf_t *);
STATIC int vtocread_ext_vtoc(vtoc_t *, struct vtoc_ioctl *);
STATIC int vtocwrite_ext_vtoc(vtoc_t *, struct vtoc_ioctl *);
STATIC int vtocphyrw(vtoc_t *, long, struct phyio *, int);
STATIC void vtocphyrw_strat(buf_t *bp);
STATIC minor_t vtocToMinor(vtoc_t *);
STATIC void vtocCloneDevice(void);
STATIC void vtoc_sendSd01(vtoc_t *, struct pdinfo *);

/*
 * Global definitions.
 */
STATIC vtoc_t **vtoc_dp;
STATIC int vtoc_dp_count;
STATIC int vtoc_dynamic = 0;
STATIC lock_t *vtoc_mutex_p;
STATIC sdi_driver_desc_t *vtoc_drvDescp;
STATIC sdi_layer_t *vtoc_layerp;

STATIC vtoc_t *vtoc_boot_idatap;

STATIC LKINFO_DECL( vtoc_lkinfo, "IO:vtoc:vtoc_lkinfo", 0);

/*
 * Useful macros.
 */
#define MINOR_TO_INDEX(min)	((min) / VTOC_MAX_MINOR)
#define MINOR_TO_SLICE(min)	((min) % VTOC_MAX_MINOR)

#define CLONE_TO_REAL(channel)	((channel) - VTOC_FIRST_CLONE_CHANNEL)
#define CHAN_TO_SLICE(channel)	( ( (channel) < VTOC_FIRST_CLONE_CHANNEL ) ? \
                             		(channel) : CLONE_TO_REAL(channel) )

#define PNODE_SLICE(slice) (slice >= VTOC_FIRST_SPEC_PART)
#define CLONE_CHAN(channel) (channel >= VTOC_FIRST_CLONE_CHANNEL)
#define VPART(channel) (channel - VTOC_FIRST_SPEC_PART)
#define SIZEOF_VTOC_DP	(sizeof(vtoc_t) * VTOC_MAX_DISKS)

#define TRANSLATE_TO_CLONE(vtoc,channel)	if (CLONE_CHAN(channel)) { \
		vtoc = vtoc_boot_idatap; \
		channel = CLONE_TO_REAL(channel); }\

/* HACK: */
/* HACK 2 D_LFS */
int vtocdevflag = D_NOBRKUP | D_MP | D_LFS;

extern char VTOC_modname[];		/* defined in Space.c */

drvops_t	vtocdrvops = {
	vtoc_config,
	vtocopen8,
	vtocclose8,
	vtocdevinfo8,
	vtocbiostart8,
	vtocioctl8,
	vtocdevctl8,
	NULL };		/* no halt entry point */

const drvinfo_t	vtocdrvinfo = {
	&vtocdrvops, VTOC_modname, (D_MP|D_RANDOM), NULL, (VTOC_MAX_MINOR - 1) };


int
_load(void)
{
	vtoc_dynamic = 1;

	DBFLAG0(1,"vtoc_load\n");
	drv_attach_mp(&vtocdrvinfo);
	if (vtocstart()) {
		return ENODEV;
	}
	return 0;
}

int
_unload(void)
{
	int i;

	DBFLAG0(1,"vtoc_unload\n");

	for (i = 1; i < VTOC_MAX_DISKS; i++) {
		if (vtoc_IsAnyOpen(vtoc_dp[i])) {
			return EBUSY;
		}
	}

	for (i = 1; i < VTOC_MAX_DISKS; i++) {
		if (vtoc_dp[i] == (vtoc_t *) NULL) {
			continue;
		}

		/* Free-up all resources */
		if (vtoc_dp[i]->vtoc_lock) 
			LOCK_DEALLOC(vtoc_dp[i]->vtoc_lock);
		if (vtoc_dp[i]->vtoc_sv)
			SV_DEALLOC(vtoc_dp[i]->vtoc_sv);
		kmem_free(vtoc_dp[i], sizeof(vtoc_t));
	}

	LOCK_DEALLOC(vtoc_dp[i]->vtoc_lock);
	kmem_free(vtoc_dp, SIZEOF_VTOC_DP);
	drv_detach(&vtocdrvinfo);
	return 0;
}

int
vtocstart(void)
{
	int i;
	int sleepflag = vtoc_dynamic ? KM_SLEEP: KM_NOSLEEP;

	DBFLAG0(1,"vtocstart");

	vtoc_dp = kmem_zalloc(SIZEOF_VTOC_DP, sleepflag);
	if (vtoc_dp == NULL) {
		cmn_err(CE_WARN, "Insufficient memory for vtoc initialization.\n");
		goto bailout;
	}
	vtoc_dp_count = 0;

	vtoc_mutex_p = LOCK_ALLOC(VTOC_HIER_BASE, pldisk, &vtoc_lkinfo,
						sdi_sleepflag);
	if (vtoc_mutex_p == NULL) {
		cmn_err(CE_WARN, "Insufficient memory for vtoc initialization.\n");
		goto bailout;
	}

	vtoc_drvDescp = sdi_driver_desc_alloc(sdi_sleepflag);
	if (!vtoc_drvDescp) {
		cmn_err(CE_WARN, "Insufficient memory for vtoc initialization.\n");
		goto bailout;
	}
	(void)strcpy(vtoc_drvDescp->sdd_modname, VTOC_modname);
	vtoc_drvDescp->sdd_precedence = SDI_STACK_VTOC;
	vtoc_drvDescp->sdd_dev_cfg = VTOC_dev_cfg;
	vtoc_drvDescp->sdd_dev_cfg_size = VTOC_dev_cfg_size;
	vtoc_drvDescp->sdd_config_entry = vtoc_config;
	vtoc_drvDescp->sdd_minors_per = VTOC_MAX_MINOR;

	if (!sdi_driver_desc_prep(vtoc_drvDescp, sdi_sleepflag)) {
		/*
		 *+ There was a problem with the VTOC driver description
		 *+ structure during initialization.  This is probably
		 *+ caused by a corrupted image of the driver.
		 */
		cmn_err(CE_WARN, "Invalid TBD driver description.\n");
		goto bailout;
	}

	vtoc_layerp = sdi_driver_add(vtoc_drvDescp, sdi_sleepflag);
	if (!vtoc_layerp) {
		/*
		 *+ There was a problem with the disk driver description
		 *+ structure during initialization.  This is probably
		 *+ caused by a corrupted image of the driver.
		 */
		cmn_err(CE_WARN, "Insufficient memory to add vtoc driver.\n");
		goto bailout;
	}

	return 0;

bailout:
	if (vtoc_drvDescp)
		sdi_driver_desc_free(vtoc_drvDescp);
	if (vtoc_mutex_p)
		LOCK_DEALLOC(vtoc_mutex_p);
	if (vtoc_dp)
		kmem_free(vtoc_dp, SIZEOF_VTOC_DP);
	return 1;
}

int
vtoc_config(cfg_func_t func, void *idata, rm_key_t rm_key)
{
	int	retval = 0;

	switch (func) {
	case CFG_ADD:
		retval = vtoc_add_instance(idata, rm_key);
		vtocCloneDevice(); /* update the boot idatap */
		break;
	case CFG_REMOVE:
		retval = vtoc_rm_instance(idata);
		break;
	case CFG_MODIFY:
	case CFG_VERIFY:
	case CFG_RESUME:
	case CFG_SUSPEND:
		break;
	default:
		retval = EINVAL;
		break;
	}

	return retval;
}

int
vtocopen(dev_t *dev_p, int flags, int otype, cred_t *cred_p)
{
	minor_t emin = geteminor(*dev_p);
	ulong_t local_slice, down_slice;
	vtoc_t *local_vtoc, *down_vtoc;
	int ret_val;
	int *flag;

	down_slice = MINOR_TO_SLICE(emin); /* 0 ->204 */
	local_slice = CHAN_TO_SLICE(down_slice); /* 0 -> 188 */
	local_vtoc = down_vtoc = vtoc_dp[MINOR_TO_INDEX(emin)];

	ASSERT(local_vtoc);

	/*
	 * Is this a clone open?
	 */
	if (CLONE_CHAN(down_slice)) { /* 189 ->204 */
		DBFLAG0(0x20,"vtocopen: clone device");

		local_vtoc = vtoc_boot_idatap;
	}

	DBFLAG2(0x21,"vtocopen (down_slice=%d local_slice=%d)", down_slice, local_slice);

	if (local_vtoc == NULL || down_vtoc == NULL) {
		return ENODEV;
	}

	if ((ret_val = vtocopen8((void *)down_vtoc, &down_slice, flags, cred_p, NULL)))
		return ret_val;

#ifndef NEED_DDI_8_OPEN_BEHAVIOR
	/*
	 * This will go away with DDI8.
	 */
	flag = PNODE_SLICE(local_slice) ?
		&local_vtoc->vtoc_fdiskFlag[VPART(local_slice)] :
		&local_vtoc->vtoc_vtocFlag[local_slice];

	VTOC_LOCK(local_vtoc);

	switch (otype) {
	case OTYP_BLK:
		*flag |= DMBLK;
		break;
	case OTYP_MNT:
		if (PNODE_SLICE(local_slice)
				|| !(local_vtoc->vtoc_part[local_slice].p_flag & V_UNMNT)) {
			*flag |= DMMNT;
		}
		break;
	case OTYP_CHR:
		*flag |= DMCHR;
		break;
	case OTYP_SWP:
		*flag |= DMSWP;
		break;
	case OTYP_LYR:
		*flag += DMLYR;
		break;
	}

	VTOC_UNLOCK(local_vtoc);
#endif

	return 0;
}

/*ARGSUSED*/
int
vtocopen8(void *idata, ulong_t *channelp, int flags, cred_t *cred_p, queue_t *dummy_arg)
{
	vtoc_t *vtoc = (vtoc_t *)idata;
	ulong_t channel = *channelp;
	ulong_t lower_channel = 0;
	int ret_val = 0;

	DBFLAG2(0x21,"vtocopen8 (dev=0x%x slice=0x%x)", vtoc, channel);

	ASSERT(idata);
	ASSERT(channelp);

	/*
	 * This function is also called everytime an instance is added. It
	 * recalculates the clone device. We are making a call
	 * to it here because we need to insure that there is a call to it
	 * if after bmkdev has run. Notice that bmkdev only runs at
	 * installation. This means that this call does nothing at any other
	 * time.
	 *
	 * This is done to address the possible reordering of HBAs by bmkdev
	 * which will affect the boot device.
	 */
	vtocCloneDevice();

	/*
	 * Do the clone stuff.
	 */
	TRANSLATE_TO_CLONE(vtoc,channel);

	/*
	 * Make sure user has permission to access the whole disk
	 * special partition.
	 */
	if (channel == VTOC_FIRST_SPEC_PART && (ret_val = drv_priv(cred_p))) {
		return ret_val;
	}

	/*
	 *	Single thread through the open
	 */
	VTOC_LOCK(vtoc);
	while (vtoc->vtoc_state & VTOC_SINGLETHREAD) {
			SV_WAIT(vtoc->vtoc_sv, pridisk, vtoc->vtoc_lock);
			VTOC_LOCK(vtoc);
	}
	vtoc->vtoc_state |= VTOC_SINGLETHREAD;
	VTOC_UNLOCK(vtoc);

	/*
	 * Open underlying device
	 */
	if (ret_val = (*vtoc->vtoc_device.sdv_drvops->d_open)
			(vtoc->vtoc_device.sdv_idatap, &lower_channel, flags, cred_p, dummy_arg)) {
		goto openfail;
	}

	/*
	 * check for First open
	 */
	if (!vtoc_IsAnyOpen(vtoc)) {
		/*
		 * Load the fdisk/vtoc
		 */
		if (ret_val = vtocload(vtoc, channel)) {
			goto bailout;
		}
	}

	/*
	 * Check if its valid to open this slice or partition.
	 */
	if (PNODE_SLICE(channel)) {
		/* Handle special non-unix partitions */
		/* Disable access to the active UNIX partition. */
		if (vtoc->vtoc_fdiskTab[VPART(channel)].p_type == UNUSED ||
			((vtoc->vtoc_fdiskTab[VPART(channel)].p_type == UNIXOS) &&
			(vtoc->vtoc_fdiskTab[VPART(channel)].p_tag == ACTIVE))) {
			ret_val = ENODEV;
			goto bailout;
		}
	} else if (channel > 0 && (channel >= vtoc->vtoc_nslices ||
			   !(vtoc->vtoc_part[channel].p_flag & V_VALID) ||
			   vtoc->vtoc_part[channel].p_size <= 0)) {
		ret_val = ENODEV;	/* The slice we are opening is invalid */
		goto bailout;
	}

	/*
	 * Keep count of all opens
	 */
	if (PNODE_SLICE(channel)) {
		vtoc->vtoc_fdiskFlag[VPART(channel)]++;
	} else {
		vtoc->vtoc_vtocFlag[channel]++;
	}

	VTOC_LOCK(vtoc);
	vtoc->vtoc_state &= ~VTOC_SINGLETHREAD;
	VTOC_UNLOCK(vtoc);
	SV_BROADCAST(vtoc->vtoc_sv, 0);

	return 0;

bailout:
	/*
	 * Clean up the vtoc structure.
	 *
	 * Scenario:
	 * A disk has a valid fdisk and an invalid vtoc and we open a slice
	 * different from 0.
	 * 	The loading of the fdisk would succeed and modify the vtoc_t structure.
	 * 	The loading of the vtoc would fail. No modification.
	 * 	The check for slice != 0 would fail the open.
	 * On exit we HAVE to undo the modification of the data structure.
	 * The check to vtoc_IsAnyOpen is the key to detect the scenario.
	 */
	if (!(vtoc_IsAnyOpen(vtoc)))
		vtocunload(vtoc);

	(void)(*vtoc->vtoc_device.sdv_drvops->d_close)
			(vtoc->vtoc_device.sdv_idatap, 0, flags, cred_p, dummy_arg);

openfail:
	VTOC_LOCK(vtoc);
	vtoc->vtoc_state &= ~VTOC_SINGLETHREAD;
	VTOC_UNLOCK(vtoc);
	SV_BROADCAST(vtoc->vtoc_sv, 0);

	return ret_val;
}

int
vtocclose(dev_t dev, int flags, int otype, cred_t *cred_p)
{
	minor_t emin = geteminor(dev);
	ulong_t local_slice, down_slice;
	vtoc_t *local_vtoc, *down_vtoc;
	int *flag;

	DBFLAG2(0x21,"vtocclose (dev=0x%x slice=%d)\n", dev, local_slice);

	down_slice = MINOR_TO_SLICE(emin); /* 0 ->204 */
	local_slice = CHAN_TO_SLICE(down_slice); /* 0 -> 188 */
	local_vtoc = down_vtoc = vtoc_dp[MINOR_TO_INDEX(emin)];

	/*
	 * Is this a clone close?
	 */
	if (CLONE_CHAN(down_slice)) { /* 189 ->204 */
		local_vtoc = vtoc_boot_idatap;
	}

#ifndef NEED_DDI_8_OPEN_BEHAVIOR
	/* Single thread through the close */
	VTOC_LOCK(local_vtoc);

	flag = PNODE_SLICE(local_slice) ?
		&local_vtoc->vtoc_fdiskFlag[VPART(local_slice)] :
		&local_vtoc->vtoc_vtocFlag[local_slice];

	switch (otype) {
	case OTYP_BLK:
		*flag &= ~DMBLK;
		break;
	case OTYP_MNT:
		if (PNODE_SLICE(local_slice)
				|| !(local_vtoc->vtoc_part[local_slice].p_flag & V_UNMNT)) {
			*flag &= ~DMMNT;
		}
		break;
	case OTYP_CHR:
		*flag &= ~DMCHR;
		break;
	case OTYP_SWP:
		*flag &= ~DMSWP;
		break;
	case OTYP_LYR:
		*flag -= DMLYR;
		break;
	}

	VTOC_UNLOCK(local_vtoc);
#endif

	return vtocclose8((void *)down_vtoc, down_slice, flags, cred_p, NULL);
}

/*ARGSUSED*/
int
vtocclose8(void *idata, ulong_t channel, int flags, cred_t *cred_p, queue_t *dummy_arg)
{
	vtoc_t *vtoc = (vtoc_t *)idata;
	int ret_val = 0;

	DBFLAG2(0x21,"vtocclose8 (dev=0x%x slice=0x%x)\n", vtoc, channel);

	/*
	 * Do the clone stuff.
	 */
	TRANSLATE_TO_CLONE(vtoc,channel);

	/*
	 * Single thread through the close
	 */
	VTOC_LOCK(vtoc);
	while (vtoc->vtoc_state & VTOC_SINGLETHREAD) {
			SV_WAIT(vtoc->vtoc_sv, pridisk, vtoc->vtoc_lock);
			VTOC_LOCK(vtoc);
	}
	vtoc->vtoc_state |= VTOC_SINGLETHREAD;
	VTOC_UNLOCK(vtoc);

	/*
	 * Keep count of all opens/closes
	 */
	if (PNODE_SLICE(channel)) {
		ASSERT(vtoc->vtoc_fdiskFlag[VPART(channel)] > 0);
		vtoc->vtoc_fdiskFlag[VPART(channel)] = 0;
	} else {
		ASSERT(vtoc->vtoc_vtocFlag[channel] > 0);
		vtoc->vtoc_vtocFlag[channel] = 0;
	}

	/*
	 * invalidate vtoc so next open gets a new one from disk
	 */
	if (!(vtoc_IsAnyOpen(vtoc))) {
		vtocunload(vtoc);
	}

	ret_val = (*vtoc->vtoc_device.sdv_drvops->d_close)
			(vtoc->vtoc_device.sdv_idatap, 0, flags, cred_p, dummy_arg);

	VTOC_LOCK(vtoc);
	vtoc->vtoc_state &= ~VTOC_SINGLETHREAD;
	VTOC_UNLOCK(vtoc);
	SV_BROADCAST(vtoc->vtoc_sv, 0);

	return ret_val;
}

int
vtocread(dev_t dev, struct uio *uio_p, cred_t *cred_pt)
{
	minor_t emin = geteminor(dev);
	buf_t *bp;
	int error;
	ulong_t down_slice;
	vtoc_t *down_vtoc;

	DBFLAG0(1,"vtocread\n");

	down_slice = MINOR_TO_SLICE(emin); /* 0 ->204 */
	down_vtoc = vtoc_dp[MINOR_TO_INDEX(emin)];

	/*
	 * Is this a clone?
	 */
	if (CLONE_CHAN(down_slice)) { /* 189 ->204 */
		down_vtoc = vtoc_boot_idatap;
	}

	bp = uiobuf(NULL, uio_p);

	bp->b_flags |= B_READ;
	bp->b_blkno = btodt(uio_p->uio_offset);
	bp->b_blkoff = uio_p->uio_offset % NBPSCTR;
	bp->b_edev = dev;

	buf_breakup(vtoc_biostart, bp, down_vtoc->vtoc_bcbp);

	error = biowait(bp);

	freerbuf(bp);

	return error;
}

int
vtocwrite(dev_t dev, struct uio *uio_p, cred_t *cred_pt)
{
	minor_t emin = geteminor(dev);
	buf_t *bp;
	int error;
	ulong_t down_slice;
	vtoc_t *down_vtoc;

	DBFLAG0(1,"vtocwrite\n");

	down_slice = MINOR_TO_SLICE(emin); /* 0 ->204 */
	down_vtoc = vtoc_dp[MINOR_TO_INDEX(emin)];

	/*
	 * Is this a clone?
	 */
	if (CLONE_CHAN(down_slice)) { /* 189 ->204 */
		down_vtoc = vtoc_boot_idatap;
	}

	bp = uiobuf(NULL, uio_p);

	bp->b_flags |= B_WRITE;
	bp->b_blkno = btodt(uio_p->uio_offset);
	bp->b_blkoff = uio_p->uio_offset % NBPSCTR;
	bp->b_edev = dev;

	buf_breakup(vtoc_biostart, bp, down_vtoc->vtoc_bcbp);

	error = biowait(bp);

	freerbuf(bp);

	return error;
}

int
vtocioctl(dev_t dev, int cmd, int arg, int mode, cred_t *cred_p, int *rval_p)
{
	minor_t emin = geteminor(dev);
	int slice = MINOR_TO_SLICE(emin);
	vtoc_t *vtoc = vtoc_dp[MINOR_TO_INDEX(emin)];

	DBFLAG2(1,"vtocioctl %d (0x%x)\n", cmd, cmd);

	return(vtocioctl8((void *)vtoc, slice, cmd, (void *)arg, mode, cred_p, rval_p));
}

int
vtocioctl8(void *idata, ulong_t channel, int cmd, void *arg, int mode, cred_t *cred_p, int *rval_p)
{
	vtoc_t *vtoc = (vtoc_t *)idata;
	int ret_val = 0;
	struct phyio phyarg;

	DBFLAG2(1,"vtocioctl8 %d (0x%x)\n", cmd, cmd);

	/*
	 * Do the clone stuff.
	 */
	TRANSLATE_TO_CLONE(vtoc,channel);

	switch (cmd) {
	case V_WRITE_PDINFO:
		if (ret_val = drv_priv(cred_p)) {
			return ret_val;
		}
		/* Fall-through */
	case V_READ_PDINFO:{
		/*
		 * vtocphyrw() requires buffer size to be multiple of
		 * disk sector size.
		 */
		uchar_t *buffer = (uchar_t *)kmem_zalloc(vtoc->vtoc_blksz, KM_SLEEP); 

		phyarg.memaddr = (long)buffer;
		phyarg.sectst = vtoc->vtoc_unixst + HDPDLOC;
		phyarg.datasz = vtoc->vtoc_blksz;

		ret_val = vtocphyrw(vtoc,(long)V_RDABS,&phyarg,SD01_KERNEL);

		if (!ret_val) {
			if (cmd == V_READ_PDINFO) {
				ret_val = copyout((void *)buffer, (void *)arg, sizeof(struct pdinfo));
				if (ret_val)
					ret_val = EFAULT;
			} else {
				ret_val = copyin((void *) arg, (void *)buffer, sizeof(struct pdinfo));
				if (ret_val) {
					ret_val = EFAULT;
				} else {
					phyarg.memaddr = (long)buffer;
					phyarg.sectst = vtoc->vtoc_unixst + HDPDLOC;
					phyarg.datasz = vtoc->vtoc_blksz;
					ret_val = vtocphyrw(vtoc,(long)V_WRABS,&phyarg,SD01_KERNEL);
				}
			}
		}

		kmem_free(buffer, vtoc->vtoc_blksz);
		return ret_val;
	} 

	case V_READ_VTOC: {	/* Read extended VTOC */
		struct vtoc_ioctl *vioc = (struct vtoc_ioctl *)
			       kmem_zalloc(sizeof(struct vtoc_ioctl),KM_SLEEP);

		ret_val = vtocread_ext_vtoc(vtoc, vioc);

		if (!ret_val &&
			copyout((void *)vioc, arg, sizeof(struct vtoc_ioctl))) {
			ret_val = EFAULT;
		}

		kmem_free(vioc, sizeof(struct vtoc_ioctl));
	} break;

	case V_WRITE_VTOC: {	/* Write extended VTOC */
		struct vtoc_ioctl *vioc = (struct vtoc_ioctl *)
		       kmem_zalloc(sizeof(struct vtoc_ioctl),KM_SLEEP);

		if (ret_val = drv_priv(cred_p))
			goto bailout1;

		if (copyin(arg, (void *)vioc, sizeof(struct vtoc_ioctl))) {
			ret_val = EFAULT;
		} else {
			ret_val = vtocwrite_ext_vtoc(vtoc, vioc);
		}

bailout1:
		kmem_free(vioc, sizeof(struct vtoc_ioctl));

	} break;

	case V_WRABS:	/* Write absolute sector number */
	case V_PWRITE:	/* Write physical data block (s) */
	case V_PDWRITE: /* Write PD sector only */
		/* Make sure user has permission */
		if (ret_val = drv_priv(cred_p)) {
			return(ret_val);
		}
		/* Fall-through */
	case V_RDABS:
	case V_PREAD: 
	case V_PDREAD:  /* Read PD sector only */
	{
		struct absio absarg;

		if (cmd == V_WRABS || cmd == V_RDABS) {
			if (copyin(arg, (void *)&absarg, sizeof(absarg)))
				return(EFAULT);

			phyarg.sectst  = (unsigned long) absarg.abs_sec;
			phyarg.memaddr = (unsigned long) absarg.abs_buf;
			phyarg.datasz  = vtoc->vtoc_blksz;
		} else {
			if (copyin(arg, (void *)&phyarg, sizeof(struct phyio)))
				return(EFAULT);

			/* Assign PD sector address */
			if (cmd == V_PDREAD || cmd == V_PDWRITE)
				phyarg.sectst = HDPDLOC + vtoc->vtoc_unixst;

			if (phyarg.datasz & 0x1FF) { /* HACK  block mask */
				/*
				 * Non-multiple of sector size 
				 */
				ret_val = EINVAL;
			}

			cmd = (cmd == V_PREAD || cmd == V_PDREAD) ? V_RDABS : V_WRABS;
		}

		ret_val = vtocphyrw(vtoc, (long) cmd, &phyarg, SD01_USER);
	} break;

	/*
	 * Return location of PDINFO relative to the begining of active partition.
	 */
	case V_PDLOC: {
		unsigned long pdloc = HDPDLOC;

		if (vtoc->vtoc_active == VTOC_INVALID_FDISK) {
			ret_val = ENXIO;
		} else if (copyout((void *)&pdloc, arg, sizeof(pdloc))) {
			ret_val = EFAULT;
		}
	} break;

	/*
	 * Return absolute location of PDINFO.
	 */
	case SD_PDLOC: {
		unsigned long pdloc = HDPDLOC + vtoc->vtoc_unixst;

		if (vtoc->vtoc_active == VTOC_INVALID_FDISK) {
			ret_val = ENXIO;
		} else if (copyout((void *)&pdloc, arg, sizeof(pdloc))) {
			ret_val = EFAULT;
		}
	} break;

	case V_GETPARMS: {
		struct disk_parms	dk_parms;
		int sec_cyl, cyls;

		DBFLAG3(0x80,"V_GETPARMS heads=0x%x sectors=0x%x size=0x%x\n", vtoc->vtoc_heads, vtoc->vtoc_sectors, vtoc->vtoc_nblk);

		dk_parms.dp_heads    = 0x40;
		dk_parms.dp_sectors  = 0x20;
		if (vtoc == vtoc_boot_idatap) {
			extern struct hdparms hdparms[];

			/*
			 * Currently, the boot program supplies BIOS
			 * parameters for the two primary disks.
			 */

			if (hdparms[0].hp_nsects == 0 || hdparms[0].hp_nheads == 0) {
				cmn_err(CE_WARN, "Your boot disk parameters were not detected by the bootstrap.\n At this time, a default geometry will be used.");
			} else {
				dk_parms.dp_sectors = (unchar) hdparms[0].hp_nsects;
				dk_parms.dp_heads   = (unchar) hdparms[0].hp_nheads;
			}
		}

		/* Calculate cylinders */
		sec_cyl = dk_parms.dp_heads * dk_parms.dp_sectors;
		cyls = (vtoc->vtoc_nblk + 1) / sec_cyl;

		/* Make room for diagnostic scratch area */
		if ((vtoc->vtoc_nblk + 1) == (cyls * sec_cyl))
			cyls --;

		dk_parms.dp_cyls     = cyls;
		dk_parms.dp_secsiz   = vtoc->vtoc_blksz;
		dk_parms.dp_type     = vtoc->vtoc_dptype;

		if (!PNODE_SLICE(channel)) {
			dk_parms.dp_ptag = vtoc->vtoc_part[channel].p_tag;
			dk_parms.dp_pflag = vtoc->vtoc_part[channel].p_flag;
			dk_parms.dp_pstartsec = vtoc->vtoc_part[channel].p_start;
			dk_parms.dp_pnumsec = vtoc->vtoc_part[channel].p_size;
		} else	{
			channel -= VTOC_FIRST_SPEC_PART;
			dk_parms.dp_ptag = vtoc->vtoc_fdiskTab[channel].p_tag;
			dk_parms.dp_pflag = vtoc->vtoc_fdiskTab[channel].p_flag;
			dk_parms.dp_pstartsec = vtoc->vtoc_fdiskTab[channel].p_start;
			dk_parms.dp_pnumsec = vtoc->vtoc_fdiskTab[channel].p_size;
	
		}

		if (copyout((void *)&dk_parms, arg, sizeof(dk_parms))) {
				ret_val = EFAULT;
		}
	} break;

	/* Force read of vtoc on next open.*/
	case V_REMOUNT: /* Do nothing and return success */
	break;

	/* Pass the ioctl to the next layer */
	default:
		return (*vtoc->vtoc_device.sdv_drvops->d_ioctl)
			(vtoc->vtoc_device.sdv_idatap, 0, cmd, arg, mode, cred_p, rval_p);
	}/* switch */

	return ret_val;
}

int
vtocsize(dev_t dev)
{
	minor_t emin = geteminor(dev);
	ulong_t local_slice, down_slice;
	vtoc_t *vtoc;
	int size;

	down_slice = MINOR_TO_SLICE(emin); /* 0 ->204 */
	local_slice = CHAN_TO_SLICE(down_slice); /* 0 -> 188 */
	vtoc = vtoc_dp[MINOR_TO_INDEX(emin)];

	/*
	 * Is this a clone close?
	 */
	if (CLONE_CHAN(down_slice)) { /* 189 ->204 */
		vtoc = vtoc_boot_idatap;
	}

	if (vtoc == NULL) {
		return -1;
	}

	if (vtoc->vtoc_nslices == 0) {
		return 0;
	}

	if (PNODE_SLICE(local_slice)) {
		size = vtoc->vtoc_fdiskTab[VPART(local_slice)].p_size;
	} else {
		size = vtoc->vtoc_part[local_slice].p_size;
	}

	DBFLAG2(1,"vtocsize = %d (0x%x)\n", size, size);

	return size;
}

/*
 * HACK:
 * We need to do buf_breakup before we start going through the layers.
 */
void
vtocstrategy(buf_t *bp)
{
	minor_t emin = geteminor(bp->b_edev);
	vtoc_t *down_vtoc;
	ulong_t	down_slice;

	down_slice = MINOR_TO_SLICE(emin); /* 0 ->204 */
	down_vtoc = vtoc_dp[MINOR_TO_INDEX(emin)];

	/*
	 * Is this a clone?
	 */
	if (CLONE_CHAN(down_slice)) { /* 189 ->204 */
		down_vtoc = vtoc_boot_idatap;
	}

	buf_breakup(vtoc_biostart, bp, down_vtoc->vtoc_bcbp);
}

STATIC void
vtoc_biostart(buf_t *bp)
{
	minor_t emin = geteminor(bp->b_edev);
	int slice = MINOR_TO_SLICE(emin);
	vtoc_t *vtoc = vtoc_dp[MINOR_TO_INDEX(emin)];

	DBFLAG0(1,"vtoc_biostart\n");

	ASSERT(vtoc);

	vtocbiostart8((void *)vtoc, slice, bp);

	return;
}

void
vtocbiostart8(void *idata, ulong_t channel, buf_t *bp)
{
	vtoc_t *vtoc = (vtoc_t *)idata;
	daddr_t start;		/* Starting Logical Sector in part */
	daddr_t last;		/* Last Logical Sector in partition */
	daddr_t numblk;	/* # of Logical Sectors requested */
	daddr_t tblkno;

	DBFLAG0(1,"vtocbiostart8\n");

	ASSERT(vtoc);

	/*
	 * Do the clone stuff.
	 */
	TRANSLATE_TO_CLONE(vtoc,channel);

	bp->b_resid = bp->b_bcount;

	if (bp->b_blkno < 0) {
		bioerror(bp, ENXIO);
		biodone(bp);
		return;
	}

	/*
	 * Be careful with channel 0 and invalid VTOCs: we allow opens on channel
	 * 0 regardless of the state of the VTOC, therefore an I/O may be
	 * routed through here and PANIC the system when the VTOC is invalid
	 * and as such, the array of channel entries is not allocated.
	 */

	start = bp->b_blkno;
	numblk = (bp->b_bcount + vtoc->vtoc_blksz -1) / vtoc->vtoc_blksz;
	last = PNODE_SLICE(channel) ?
		vtoc->vtoc_fdiskTab[VPART(channel)].p_size
		: vtoc->vtoc_part[channel].p_size;

	DBFLAG4(8,"channel = %d blkno = %x last = %x numblk = %x\n", channel, start, last, numblk);

	/*
	 * Impose VTOC discipline
	 */
	if (bp->b_flags & B_READ) {
		int resid;

		if (start + numblk > last) {
			if (start > last) {
				bioerror(bp, ENXIO);
				biodone(bp);
				return;
			}
			
			resid =  bp->b_bcount - ((last - start) * vtoc->vtoc_blksz);
			bp->b_bcount -= resid;
			bp->b_resid = resid;
		}
	} else {	/* Write case */
		if ((unsigned)start + numblk >  last) {
			/*
			 * Return an error if entire request is beyond
			 * the End-Of-Media.
			 */
			if (start > last) {
				bioerror(bp, ENXIO);
				biodone(bp);
				return;
			}
			
			/*
			 * The request begins exactly at the End-Of-Media.
			 * A 0 length request is OK.  Otherwise, its an error.
			 */ 
			if (start == last) {
				if (bp->b_bcount != 0) {
					bioerror(bp, ENXIO);
				}
				biodone(bp);
				return;
			}

			/*
			 * Only part of the request is beyond the End-Of-
			 * Media, so adjust the count accordingly.
			 */ 
			bp->b_bcount -= (last - start) * vtoc->vtoc_blksz;
		}

		/*
		 * De-risked protection of PD and VTOC.
		 */
		if (!PNODE_SLICE(channel)
				&& vtoc->vtoc_part[channel].p_flag & V_RONLY) {

			/* Verify that partition is really read-only */
			VTOC_LOCK(vtoc);
			if (vtoc->vtoc_part[channel].p_flag & V_RONLY) {
				VTOC_UNLOCK(vtoc);
				bioerror(bp, EACCES);
				biodone(bp);
				return;
			}
			VTOC_UNLOCK(vtoc);
		}
			
	}

	if (bp->b_bcount == 0) {
		biodone(bp);
		return;
	}


	/*
	 * Make it an absolute offset
	 */
	tblkno = start + (PNODE_SLICE(channel) ? 
		vtoc->vtoc_fdiskTab[VPART(channel)].p_start
		: vtoc->vtoc_part[channel].p_start);
	sdi_buf_store(bp, tblkno);
	bp->b_iodone = vtocintn;

	DBFLAG2(8,"abs = 0x%x p_start  = 0x%x \n", tblkno,
	PNODE_SLICE(channel) ? 
		vtoc->vtoc_fdiskTab[VPART(channel)].p_start
		: vtoc->vtoc_part[channel].p_start);

	(*vtoc->vtoc_device.sdv_drvops->d_biostart)(vtoc->vtoc_device.sdv_idatap, 0, bp);
}

int
vtocprint(dev_t dev, char *str)
{
	DBFLAG0(1,"vtocprint\n");
	return 0;
}

int
vtocdevinfo(dev_t dev, di_parm_t parm, void **valp)
{
	minor_t emin = geteminor(dev);
	vtoc_t *vtoc = vtoc_dp[MINOR_TO_INDEX(emin)];

	DBFLAG0(1,"vtocdevinfo\n");

	if (vtoc == NULL) {
		return ENODEV;
	}

	switch (parm) {
		case DI_BCBP:
			*(bcb_t **)valp = vtoc->vtoc_bcbp;
			return 0;
		case DI_MEDIA:
#ifdef OUT
	/* Got to figure this one out */
			if (dm->dm_iotype & F_RMB)
				*(char **)valp = "disk";
			else
#endif
				*(char **)valp = NULL;
			return 0;
		default:
			return ENOSYS;
	}
}

int
vtocdevinfo8(void *idata, ulong_t channel, di_parm_t parm, void *valp)
{
	vtoc_t *vtoc = (vtoc_t *)idata;

	DBFLAG0(1,"vtocdevinfo8\n");

	ASSERT(vtoc);

	/*
	 * Do the clone stuff.
	 */
	TRANSLATE_TO_CLONE(vtoc,channel);

	switch (parm) {
		case DI_MEDIA:
			*(char **)valp = "disk";
			return 0;
		case DI_SIZE:
			if (vtoc->vtoc_nslices == 0) {
				return EOPNOTSUPP;
			}

			((devsize_t *)valp)->blkoff = (ushort_t)0;

			if (PNODE_SLICE(channel)) {
				((devsize_t *)valp)->blkno = vtoc->vtoc_fdiskTab[VPART(channel)].p_size;
			} else {
				((devsize_t *)valp)->blkno = vtoc->vtoc_part[channel].p_size;
			}
			return 0;
		default:
			return (*vtoc->vtoc_device.sdv_drvops->d_devinfo)
				(vtoc->vtoc_device.sdv_idatap, 0, parm, valp);
	}
}

void
vtocintn(buf_t *bp)
{
	DBFLAG0(1,"vtocintn\n");
	/* Bottom of stack */
	sdi_buf_restore(bp);
	biodone(bp);
}

void
vtoc_slice_ck(vtoc_t * vtoc, sdi_slice_ck_t *arg)
{
	int i;

	for (i = 0; i < vtoc->vtoc_nslices; i++) {
		if (vtoc->vtoc_part[i].p_tag == arg->ssc_ptag) {
			arg->ssc_channel = i;
			arg->ssc_exists = B_TRUE;
			return;
		}
	}

	arg->ssc_exists = B_FALSE;
}

int
vtocdevctl8(void *idata, ulong_t channel, int cmd, void *arg)
{
	vtoc_t *vtoc = (vtoc_t *)idata;

	ASSERT(vtoc);

	/*
	 * Do the clone stuff.
	 */
	TRANSLATE_TO_CLONE(vtoc,channel);

	switch(cmd) {
	case SDI_DEVICE_SLICE_CK:
		vtoc_slice_ck(vtoc, (sdi_slice_ck_t *)arg);
		return 0;

	default:
		return (*vtoc->vtoc_device.sdv_drvops->d_drvctl)
			(vtoc->vtoc_device.sdv_idatap, 0, cmd, arg);
	}
}

STATIC int
vtoc_add_instance(void *idata, rm_key_t rm_key)
{
	vtoc_t *vtoc;
	sdi_device_t *device = NULL;
	sdi_device_t *layer_device = NULL;
	minor_t	this_minor;
	int ret_val = 0;
	ulong_t channel;
	pl_t opri;
	int i;
	devsize_t size;
	sdi_diskparmMsg_t msg;

	DBFLAG0(0x40,"vtoc_add_instance\n");

	/*
	 * Allocate all resources for the vtoc structure.
	 */
	if (!((vtoc = kmem_zalloc(sizeof(vtoc_t), KM_SLEEP))
		&& (vtoc->vtoc_lock = LOCK_ALLOC(VTOC_HIER_BASE, pldisk,
			&vtoc_lkinfo, sdi_sleepflag))
		&& (vtoc->vtoc_sv = SV_ALLOC(sdi_sleepflag)))) {

		ret_val = ENOMEM;
		goto bailout;
	}

	*(void **)idata = (void *)vtoc;

	vtoc->vtoc_active = VTOC_INVALID_FDISK;
	vtoc->vtoc_nslices = 0;

	/*
	 * Link imported device.
	 */
	device = &vtoc->vtoc_device;
	if (!sdi_dev_read_opr(rm_key, device)) {
		/*
		 * If this fails there is an unworkable inconsistency between
		 * the resmgr and sdi.
		 */
		ASSERT(0);
		ret_val = ENXIO;
		goto bailout;
	}

	channel = 0;

	if ((*vtoc->vtoc_device.sdv_drvops->d_open) \
			(vtoc->vtoc_device.sdv_idatap, &channel, FREAD, sys_cred, NULL)) {
		vtoc->vtoc_nblk = -1;
		vtoc->vtoc_bcbp = (bcb_t *)NULL;
	}
	else {
		if ((*vtoc->vtoc_device.sdv_drvops->d_devinfo) \
				(vtoc->vtoc_device.sdv_idatap, channel, DI_SIZE, &size)) {
			vtoc->vtoc_nblk = -1;
		}
		else {
			vtoc->vtoc_nblk = size.blkno;
		}

		if ((*vtoc->vtoc_device.sdv_drvops->d_devinfo) \
				(vtoc->vtoc_device.sdv_idatap, channel, DI_RBCBP, &vtoc->vtoc_bcbp)) {
			vtoc->vtoc_bcbp = (bcb_t *)NULL;
		}

		(void)(*vtoc->vtoc_device.sdv_drvops->d_close) \
				(vtoc->vtoc_device.sdv_idatap, channel, FREAD, sys_cred, NULL);
	}

	/*
	 * Retrieve the block size and disk type from sd01
	 */
	(*vtoc->vtoc_device.sdv_drvops->d_drvctl)
		(vtoc->vtoc_device.sdv_idatap, 0, SDI_DEVICE_GEOMETRY, (void *)&msg);
	vtoc->vtoc_blksz = msg.dp_secsiz;
	vtoc->vtoc_dptype = msg.dp_type;

	/* Stick the newly initialized vtoc in the driver's global data array. */
	opri = LOCK(vtoc_mutex_p, pldisk);
	this_minor = vtoc_dp_count * VTOC_MAX_MINOR;
	vtoc_dp[vtoc_dp_count] = vtoc;
	vtoc->vtoc_offset = vtoc_dp_count++;
	UNLOCK(vtoc_mutex_p, opri);

	/*
	 * Export this device.
	 * 
	 * Note:
	 * It would seem that layer_device should be stored in the vtoc_t so
	 * that at removal time we may sdi_device_free.
	 */
	if (!(layer_device = sdi_device_alloc(sdi_sleepflag))) {
		ret_val = ENOMEM;
		goto bailout;
	}
	vtoc->vtoc_export = layer_device;

	layer_device->sdv_state   = SDI_DEVICE_EXISTS|SDI_DEVICE_ONLINE;
	layer_device->sdv_layer   = vtoc_layerp;
	layer_device->sdv_parent_handle  = rm_key;
	layer_device->sdv_driver  = vtoc_drvDescp;
	layer_device->sdv_order   = device->sdv_order;
	layer_device->sdv_unit    = device->sdv_unit;
	layer_device->sdv_bus     = device->sdv_bus;
	layer_device->sdv_target  = device->sdv_target;
	layer_device->sdv_lun     = device->sdv_lun;
	layer_device->sdv_devtype = device->sdv_devtype;
	layer_device->sdv_bcbp    = device->sdv_bcbp;
	layer_device->sdv_cgnum   = device->sdv_cgnum;

	layer_device->sdv_drvops  = &vtocdrvops;
	layer_device->sdv_idatap  = (void *)vtoc;

	(void)bcopy(device->sdv_inquiry, layer_device->sdv_inquiry, INQ_EXLEN);
	(void)bcopy(&(device->sdv_stamp), &(layer_device->sdv_stamp), sizeof(struct pd_stamp));

	if (!sdi_device_prep(layer_device, sdi_sleepflag) ) {
		cmn_err(CE_WARN, "vtoc_add_instance: invalid device structure version");
		ret_val = EINVAL;
		goto bailout;
	}

	if (!sdi_device_add(layer_device, sdi_sleepflag) ) {
		cmn_err(CE_WARN, "vtoc_add_instance: unable to add device to I/O stack");
		ret_val = EINVAL;
		goto bailout;
	}
	cmn_err(CE_NOTE, "!vtoc_add_instance: just added device %s",
			layer_device->sdv_inquiry);

	return ret_val; /* Successful return! */

bailout:
	if (layer_device)
		sdi_device_free(layer_device);
	if (vtoc) {
		if (vtoc->vtoc_lock) 
			LOCK_DEALLOC(vtoc->vtoc_lock);
		if (vtoc->vtoc_sv)
			SV_DEALLOC(vtoc->vtoc_sv);
		kmem_free(vtoc, sizeof(vtoc_t));
	}
	return ret_val;
}

STATIC int
vtoc_rm_instance(void *idata)
{
	vtoc_t *vtoc = idata;
	sdi_device_t *device = NULL;
	sdi_device_t *layer_device = NULL;
	minor_t	this_minor;
	int ret_val = 0;
	ulong_t channel;
	pl_t opri;
	int i;
	devsize_t size;

	DBFLAG0(0x40,"vtoc_rm_instance\n");

	sdi_device_rm(vtoc->vtoc_export->sdv_handle, KM_NOSLEEP);
	sdi_device_free(vtoc->vtoc_export);

	/*
	 * remove the vtoc instance from the driver's global data array.
	 */
	opri = LOCK(vtoc_mutex_p, pldisk);
	vtoc_dp[vtoc->vtoc_offset] = NULL;
	UNLOCK(vtoc_mutex_p, opri);

	/*
	 * de-allocate all resources for the vtoc structure.
	 */
	LOCK_DEALLOC(vtoc->vtoc_lock);
	SV_DEALLOC(vtoc->vtoc_sv);
	kmem_free(vtoc, sizeof(vtoc_t));

	return 0;
}

/*
 * STATIC void
 * vtocCloneDevice()
 *
 * Goes through the the vtoc disk entries and determine the first minor number
 * of the boot disk.  Note that during installation the definition of the boot disk
 * is different than during normal operation.  DUring install, the HBA_map will
 * remap controller numbers so we can determine the actual controller zero ( boot ctrlr ).
 *
 * What we really do is look for the device ( disk obviously ) with the lowest target ID
 * on the controller number that sdi_edtindex says is really controller number zero. By
 * convention inside of SDI, this is the boot controller.  Which one is controller zero
 * is controlled by HBA_map which is accessed using sdi_edtindex.
 *
 * NOTE: sdi_edtindex(0) always returns 0 in the non-ISL case.
 */
STATIC void
vtocCloneDevice(void)
{
	int i, saved_index;
	ulong_t lowest_lun, lowest_target, boot_ctrlr;

	i = sdi_edtindex(0);

	if (i < 0)
		return;

	boot_ctrlr = (ulong_t)i;

	for (lowest_lun=lowest_target=ULONG_MAX, saved_index=-1, i=0; i < vtoc_dp_count; i++) {

		if (!vtoc_dp[i])
			continue;

		if (vtoc_dp[i]->vtoc_device.sdv_unit == boot_ctrlr) {
			if (vtoc_dp[i]->vtoc_device.sdv_target < lowest_target) {
				lowest_target = vtoc_dp[i]->vtoc_device.sdv_target;
				lowest_lun = vtoc_dp[i]->vtoc_device.sdv_lun;
				saved_index = i;
			}
			else if (vtoc_dp[i]->vtoc_device.sdv_target == lowest_target) {
				if (vtoc_dp[i]->vtoc_device.sdv_lun < lowest_lun) {
					lowest_lun = vtoc_dp[i]->vtoc_device.sdv_lun;
					saved_index = i;
				}
			}
		}
	}

	if (saved_index < 0)
		return;

	vtoc_boot_idatap = (vtoc_dp[saved_index]);
}

/*
 * STATIC int
 * vtoc_IsAnyOpen(vtoc_t *vtoc)
 *
 * Checks if any partition or slice is open.
 */
STATIC int
vtoc_IsAnyOpen(vtoc_t *vtoc)
{
	int i;

	for (i = 0; i < VTOC_NUM_SPEC_PART; i++) {
		if (vtoc->vtoc_fdiskFlag[i])
			return 1;
	}

	for (i = 0; i < vtoc->vtoc_nslices; i++) {
		if (vtoc->vtoc_vtocFlag[i])
			return 1;
	}

	return 0;
}

/*
 * STATIC int
 * vtoc_FetchFdisk(vtoc_t *vtoc)
 *
 * Read the fdisk (block 0) from the disk and load it into memory.
 */
STATIC int
vtoc_FetchFdisk(vtoc_t *vtoc, uchar_t *buff)
{
	struct phyio phyarg;
	struct ipart *part;

	/* HACK */
	DBFLAG2(0x10,"vtoc_FetchFdisk = %d (0x%x)\n", FDBLKNO, FDBLKNO);
	phyarg.sectst = FDBLKNO;
	phyarg.memaddr = (long) buff;
	/*
	phyarg.datasz = sizeof(struct mboot);
	VASS
	*/
	phyarg.datasz = VTOC_SIZE;
	vtocphyrw(vtoc, V_RDABS, &phyarg, SD01_KERNEL);

	if (phyarg.retval != 0 ||
		((struct mboot *) (void *) buff)->signature != MBB_MAGIC) {
		return -1;
	}

	/* Get to partition table and store it in the vtoc structure */
	part = (struct ipart *) (void *) ((struct mboot *)(void *) buff)->parts;

	/*
	 * vtoc_spec_parts should give us the index to the active fdisk
	 * partition. If 0 is returned, it means no active partition was found.
	 */
	return vtoc_spec_parts(part, vtoc);
}

/*
 * Got to figure out why does this has to be so complicated.
 */
STATIC int
vtoc_spec_parts(struct ipart *table, vtoc_t *vtoc)
{
	int	i, j, s, n;
	int	o[FD_NUMPART];
	struct ipart	*p;
	int	activePart = 0;

	/*
	 *  NOTE: there is code in vtocload that depends on this sort happening.
	 */

	for (i = 0, s = 0; i < FD_NUMPART; i++)	{
		if (table[i].systid == UNUSED ||
				(table[i].bootid == NOTACTIVE &&
					table[i].beghead == 0 &&
					table[i].begsect == 0))
			continue;

		/* anything sorted yet */
		if ((n = s) > 0)	{

			/* if so, find spot for new entry */
			for (j = s - 1; j >= 0; j--)	{
				if (table[i].relsect >= table[o[j]].relsect)
					break;

				n = j;
			}

			/* shift down entries below this one */
			for (j = s; j > n; j--)
				o[j] = o[j - 1];
		}

		o[n] = i;
		s++;
	}

	/*
	 * first set up entry 0 to access the whole disk
	 */
	vtoc->vtoc_fdiskTab[0].p_start = 0;
	vtoc->vtoc_fdiskTab[0].p_size = vtoc->vtoc_nblk;
	vtoc->vtoc_fdiskTab[0].p_type = PTYP_WHOLE_DISK;

	/*
	 * and now entries 1-4 for the sorted fdisk partitions, pointed to
	 * by o[0]-o[3]
	 */
	for (i = 1; i <= s; i++)	{
		p = &table[o[i - 1]];
		vtoc->vtoc_fdiskTab[i].p_start = p->relsect;
		vtoc->vtoc_fdiskTab[i].p_size = p->numsect;
		vtoc->vtoc_fdiskTab[i].p_type = p->systid & PTYP_MASK;

		/* a little trick to keep track of the active partition */
		if (vtoc->vtoc_fdiskTab[i].p_tag = p->bootid & 0xFF) {
			activePart = i;
			/* This is needed for OSR5-divvy compatibility */
			vtoc->vtoc_heads = p->endhead + 1;
			vtoc->vtoc_sectors = p->endsect & 0x3F;
			DBFLAG4(2, "nheads = %d (0x%x) sectors = %d (0x%x)\n",vtoc->vtoc_heads, vtoc->vtoc_heads, vtoc->vtoc_sectors, vtoc->vtoc_sectors);
		}
	}

	for (; i < VTOC_NUM_SPEC_PART; i++)	{
		vtoc->vtoc_fdiskTab[i].p_type = UNUSED;
		vtoc->vtoc_fdiskTab[i].p_tag = NOTACTIVE;
	}

#ifdef DEBUG
	DBFLAG1(2, "valid non-unix partitions= %d\n", s);
	for (i = 0; i < VTOC_NUM_SPEC_PART; i++)	{
		struct xpartition	*x = &vtoc->vtoc_fdiskTab[i];
		DBFLAG5(2, "%d: p_tag= %d p_type= %d p_start= 0x%x p_size= 0x%x\n", i, x->p_tag, x->p_type, x->p_start, x->p_size);
	}
#endif

	return activePart;
}

/*
 * int
 * vtocload(vtoc_t *vtoc, int slice)
 *
 * Check if we have valid Fdisk and VTOC, if not, try to load them.
 * Once in, check that we are trying to access a valid slice.
 *
 * Calling/Exit State:
 *	The lock for the vtoc must be held on entry.
 */
int
vtocload(vtoc_t *vtoc, int slice)
{
	int ret_val = 0;
	uchar_t *buff;
	int buffsz = max(VTOC_SIZE, sizeof(struct mboot));
	struct pd_stamp *stamp;

	DBFLAG2(4,"vtocload: vtoc=0x%x slice = %d", vtoc, slice);


	if (vtoc->vtoc_active != VTOC_INVALID_FDISK) {
		return ret_val;
	}

	buff = kmem_alloc(buffsz, KM_SLEEP);

	vtoc->vtoc_active = vtoc_FetchFdisk(vtoc, buff);

	/*
	 * override the active partition if this is the BOOTSTAMP disk
	 *
	 * NOTE: this code depends on the sort in vtoc_spec_parts
	 *       otherwise, the number that sdi_calc_root_partition returns
	 *       can't be used for vtoc_active.
	 */
	stamp = (struct pd_stamp *)bs_getval("BOOTSTAMP");
	if (stamp && PD_STAMPMATCH(&vtoc->vtoc_device.sdv_stamp, stamp))
		vtoc->vtoc_active = sdi_calc_root_partition();

	if (vtoc->vtoc_active <= 0) {
		/* Bad fdisk */
		if (slice > 0) {
			if (vtoc->vtoc_active == VTOC_INVALID_FDISK) {
				cmn_err(CE_NOTE, "!Disk Driver: Invalid mboot signature.");
			} else if (vtoc->vtoc_active == 0) {
				cmn_err(CE_NOTE, "!Disk Driver: No ACTIVE partition.");
			}
			ret_val = ENODEV;
		}
	} else {
		vtoc->vtoc_unixst = vtoc->vtoc_fdiskTab[vtoc->vtoc_active].p_start;

		/*
		 * We're trying to open a slice.
		 */
		if (vtoc_FetchVtoc(vtoc,buff) && !PNODE_SLICE(slice) &&
				(slice > 0)){
			/* Bad vtoc */
			cmn_err(CE_NOTE, "!Disk Driver: Invalid VTOC.");
			ret_val = ENODEV;
		}

	}

	/*
	 * We must allow opening of slice 0 so that admin commands
	 * may work. For this reason allocate everything for 1 slice.
	 */
	if (vtoc->vtoc_sanity != VTOC_SANE) {
		vtoc->vtoc_vtocFlag = (int *)kmem_zalloc(sizeof(int), KM_SLEEP);
		vtoc->vtoc_part = (struct partition *)
				kmem_zalloc(sizeof(struct partition), KM_SLEEP);
		vtoc->vtoc_nslices = 1;
		vtoc->vtoc_part->p_start = 0;
		vtoc->vtoc_part->p_size = 0;
	}

	kmem_free(buff, buffsz);

	return ret_val;
}

STATIC int
vtoc_FetchVtoc(vtoc_t *vtoc, uchar_t *buff)
{
	struct phyio phyarg;
	int ret_val = 0;
	struct pdinfo *pdptr;
	
	phyarg.sectst = vtoc->vtoc_unixst + HDPDLOC;
	DBFLAG2(0x10,"vtoc_FetchVtoc = %d (0x%x)\n", phyarg.sectst, phyarg.sectst);
	phyarg.memaddr = (long) buff;
	phyarg.datasz = VTOC_SIZE;
	vtocphyrw(vtoc, V_RDABS, &phyarg, SD01_KERNEL);


	if (phyarg.retval != 0) {
		return ENODEV;
	}

	pdptr = (struct pdinfo *) buff;
	if (pdptr->sanity == VALID_PD) {
		int ret_val;

		ret_val = vtoc_fillVtoc(vtoc, pdptr, buff);

		/* Ugly bad block compatibility stuff */
		vtoc_sendSd01(vtoc, pdptr);

		return ret_val;
	} else {
		return vtoc_FetchDivvy(vtoc, buff);
	}
}

STATIC ulong_t
vtoc_ext_cksum(caddr_t sectp)
{
	ulong_t			cksum,
				*np;
	struct vtoc_ext_trailer	*vet;

	vet = &((struct vtoc_ext_sect *)sectp)->ve_trailer;

	/*
	 * assume sectp is ulong-aligned, should be the case since it's a
	 * pointer to sector buffer, with sector size at least 512
	 * checksum is XOR of everything up to not including checksum
	 * itself
	 */
	for (cksum = 0, np = (ulong_t *)sectp; np < &vet->ve_cksum; np++)
		cksum ^= *np;

	return cksum;
}

STATIC int
vtoc_ckvtoc_ext_sect(caddr_t sectp, daddr_t sect, daddr_t firstsect,
			ushort_t nsects)
{
	struct vtoc_ext_trailer	*vet;


	vet = &((struct vtoc_ext_sect *)sectp)->ve_trailer;

	/* Check this again. */
	if (sect < firstsect || sect >= firstsect + nsects ||
			vet->ve_firstslice !=
				(sect - firstsect) * VE_SLICES_PER_SECT)
		return 0;

	return vtoc_ext_cksum(sectp) == vet->ve_cksum;
}

STATIC int
vtoc_fillVtoc(vtoc_t *vtoc, struct pdinfo *pdptr, uchar_t *buff)
{
	struct vtoc *vtocptr;
	struct xpartition *xpart = &vtoc->vtoc_fdiskTab[vtoc->vtoc_active];
	struct vtoc_ext_hdr *ext;
	struct partition *tmppart;
	caddr_t sectp;
	daddr_t sect;
	int nslices;
	size_t flsz, ptsz;

	/* Find vtoc */
	vtocptr = (struct vtoc *) (buff + (pdptr->vtoc_ptr & 0x1FF));

	/* Check Sanity */
	if (vtocptr->v_sanity != VTOC_SANE) {
		return -1;
	}

	/* Make sure slice 0 is correct. */
	if ((vtocptr->v_part[0].p_start != xpart->p_start) ||
			(vtocptr->v_part[0].p_size != xpart->p_size)) {
		return -1;
	}

	ext = (struct vtoc_ext_hdr *)((caddr_t)vtocptr + sizeof(struct vtoc));

	if (vtocptr->v_version < XVTOCVERS) {
		bzero((caddr_t)ext, sizeof(struct vtoc_ext_hdr));
	} else if (ext->ve_sanity != VTOC_SANE ||
				VE_NSECTS(ext->ve_nslices) != ext->ve_nsects) {
			return -1;
	}

	/* Allocate the necessary memory.  */
	nslices = V_NUMPAR + ext->ve_nslices;
	flsz = nslices * sizeof(int);
	ptsz = nslices * sizeof(struct partition);
	vtoc->vtoc_vtocFlag = (int *) kmem_zalloc(flsz, KM_SLEEP);
	vtoc->vtoc_part = (struct partition *) kmem_zalloc(ptsz, KM_SLEEP);

	/* Initialize remaining fields */
	vtoc->vtoc_sanity = vtocptr->v_sanity;
	vtoc->vtoc_version = vtocptr->v_version;
	vtoc->vtoc_nslices = nslices;

	/* Copy at most the first 16 slices */
	bcopy((caddr_t)vtocptr->v_part, (caddr_t)vtoc->vtoc_part,
		(vtocptr->v_nparts < V_NUMPAR ? vtocptr->v_nparts : V_NUMPAR)
				* sizeof(struct partition));

	/*
	 * copy slice information from the the VTOC extension sectors to
	 * the in-core extended VTOC right after the information for the
	 * initial VTOC (v_part[V_NUMPAR], timestamp[V_NUMPAR])
	 */
	for (sect = ext->ve_firstsect, sectp = ((caddr_t) buff) + 512 /*HACK*/,
				tmppart = vtoc->vtoc_part + V_NUMPAR;
			sect < ext->ve_firstsect + ext->ve_nsects;
			sect++, sectp += 512,/* HACK */
				tmppart += VE_SLICES_PER_SECT)	{

		struct vtoc_ext_sect *extsect=((struct vtoc_ext_sect *) sectp);

		if (!vtoc_ckvtoc_ext_sect(sectp, sect, ext->ve_firstsect,
					ext->ve_nsects))	{
			return -1;
		}

		bcopy(sectp, (caddr_t)tmppart,
			extsect->ve_trailer.ve_nslices*sizeof(struct partition));
	}

	return 0;
}

STATIC int
vtocread_ext_vtoc(vtoc_t *vtoc,struct vtoc_ioctl *ioc)
{
	int			s, n, ret_val = 0;
	size_t			bufsz, sz;
	caddr_t			secbuf, sectp;
	struct vtoc		*vtocptr;
	struct vtoc_ext_hdr	*ext;
	struct vtoc_ext_sect	*vsect;
	struct partition	*slices;
	struct phyio		phyarg;


	/*
	 * zero out result structure
	 */
	bzero((caddr_t)ioc, sizeof(struct vtoc_ioctl));

	bufsz = VTOC_SIZE;
	secbuf = (caddr_t)kmem_alloc(bufsz, KM_SLEEP);

	phyarg.sectst  = vtoc->vtoc_unixst + VTOC_SEC; 
	phyarg.memaddr = (long)secbuf;
	phyarg.datasz  = bufsz;
	if (ret_val = vtocphyrw(vtoc, V_RDABS, &phyarg, SD01_KERNEL))
		goto bailout;

	/* Invalid PDINFO */
	if (((struct pdinfo *) secbuf)->sanity != VALID_PD) {
		ret_val = ENXIO;
		goto bailout;
	}

	vtocptr = (struct vtoc *) (secbuf +
		(((struct pdinfo *) secbuf)->vtoc_ptr & 0x1FF));

	/* Invalid VTOC */
	if (vtocptr->v_sanity != VTOC_SANE)	{
		ret_val = ENXIO;
		goto bailout;
	}

	bcopy((caddr_t)vtocptr->v_part, (caddr_t)ioc->v_slices,
				vtocptr->v_nparts * sizeof(struct partition));
	if (vtocptr->v_version < XVTOCVERS)	{
		ioc->v_nslices = vtocptr->v_nparts;
	}
	else	{
		ext = (struct vtoc_ext_hdr *)((caddr_t)vtocptr +
							sizeof(struct vtoc));
		if (ext->ve_sanity != VTOC_SANE ||
				VE_NSECTS(ext->ve_nslices) !=
							ext->ve_nsects)	{
			ret_val = ENXIO;
			goto bailout;
		}

		ioc->v_nslices = ext->ve_nslices + V_NUMPAR;
		sz = VE_SLICES_PER_SECT * sizeof(struct partition);

		for (s = ext->ve_firstsect, n = ext->ve_nslices,
					sectp = secbuf + 512, /* HACK */
					slices = &ioc->v_slices[V_NUMPAR];
				s < ext->ve_firstsect + ext->ve_nsects;
				s++, sectp += 512, /* HACK */
					n -= VE_SLICES_PER_SECT,
					slices += VE_SLICES_PER_SECT)	{

			if (!vtoc_ckvtoc_ext_sect(sectp, s, ext->ve_firstsect,
						ext->ve_nsects))	{
				ret_val = ENXIO;
				goto bailout;
			}

			vsect = (struct vtoc_ext_sect *)sectp;
			if (n < VE_SLICES_PER_SECT)
				sz = n * sizeof(struct partition);
			bcopy((caddr_t)vsect->ve_slices, (caddr_t)slices, sz);
		}
	}

bailout:
	kmem_free(secbuf, bufsz);
	return ret_val;
}


STATIC int
vtocwrite_ext_vtoc(vtoc_t *vtoc, struct vtoc_ioctl *ioc)
{
	int			s, n, ret_val = 0;
	size_t			bufsz, sz;
	caddr_t			secbuf, sectp;
	struct vtoc		*vtocptr;
	struct vtoc_ext_hdr	*ext;
	struct vtoc_ext_sect	*vsect;
	struct partition	*slices;
	struct phyio		phyarg;

	bufsz = VTOC_SIZE;
	secbuf = (caddr_t)kmem_alloc(bufsz, KM_SLEEP);

	/*
	 * Read in existing PDINFO/VTOC and VTOC extension sectors
	 */
	phyarg.sectst  = vtoc->vtoc_unixst + VTOC_SEC; 
	phyarg.memaddr = (long)secbuf;
	phyarg.datasz  = bufsz;
	if (ret_val = vtocphyrw(vtoc, V_RDABS, &phyarg, SD01_KERNEL)) {
		kmem_free(secbuf, bufsz);
		return ret_val;
	}

	vtocptr = (struct vtoc *) (secbuf +
		(((struct pdinfo *) secbuf)->vtoc_ptr & 0x1FF));

	/*
	 * make sure the initial VTOC and VTOC extension header are zeroed out
	 */
	bzero((caddr_t)vtocptr,
			sizeof(struct vtoc) + sizeof(struct vtoc_ext_hdr));

	vtocptr->v_sanity = VTOC_SANE;

	/*
	 * force initial VTOC to always have V_NUMPAR slices
	 */
	vtocptr->v_nparts = V_NUMPAR;

	/*
	 * copy initial VTOC information for the slices provided
	 */
	n = (ioc->v_nslices > V_NUMPAR) ? V_NUMPAR : ioc->v_nslices;
	bcopy((caddr_t)ioc->v_slices, (caddr_t)vtocptr->v_part,
					n * sizeof(struct partition));

	/*
	 * make sure all sectors in the VTOC extension are zeroed out
	 */
	sectp = secbuf + 512; /* HACK */
	bzero(sectp, bufsz - 512); /* HACK */

	if (ioc->v_nslices <= V_NUMPAR)	{
		vtocptr->v_version = XVTOCVERS - 1;
	}

	/*
	 * copy VTOC extension information for the slices provided
	 */
	else	{
		vtocptr->v_version = XVTOCVERS;
		ext = (struct vtoc_ext_hdr *)((caddr_t)vtocptr +
							sizeof(struct vtoc));
		ext->ve_sanity = VTOC_SANE;
		ext->ve_nslices = ioc->v_nslices - V_NUMPAR;
		ext->ve_firstsect = vtoc->vtoc_unixst + VE_FIRSTSECT;
		ext->ve_nsects = VE_NSECTS(ext->ve_nslices);

		sz = VE_SLICES_PER_SECT * sizeof(struct partition);

		/*
		 * For every sector that contains slices do:
		 *	Fix the trailer info.
		 *	Copy the partitions that should rest there.
		 */
		for (s = 0, n = ext->ve_nslices,
				slices = &ioc->v_slices[V_NUMPAR];
				s < ext->ve_nsects;
					s++, sectp += 512, /* HACK */
					n -= VE_SLICES_PER_SECT,
					slices += VE_SLICES_PER_SECT)	{
			sz = n < VE_SLICES_PER_SECT ? n : VE_SLICES_PER_SECT;

			vsect = (struct vtoc_ext_sect *)sectp;
			vsect->ve_trailer.ve_firstslice = s*VE_SLICES_PER_SECT;
			vsect->ve_trailer.ve_nslices = sz;

			sz *= sizeof(struct partition);
			bcopy((caddr_t)slices, (caddr_t)vsect->ve_slices, sz);

			vsect->ve_trailer.ve_cksum = vtoc_ext_cksum(sectp);
		}
	}

	phyarg.sectst  = vtoc->vtoc_unixst + VTOC_SEC; 
	phyarg.memaddr = (long)secbuf;
	phyarg.datasz  = bufsz;
	ret_val = vtocphyrw(vtoc, V_WRABS, &phyarg, SD01_KERNEL);

	kmem_free(secbuf, bufsz);
	return ret_val;
}

STATIC void
vtocunload(vtoc_t *vtoc)
{
	kmem_free(vtoc->vtoc_vtocFlag, vtoc->vtoc_nslices * sizeof(int));
	kmem_free(vtoc->vtoc_part, vtoc->vtoc_nslices * sizeof(struct partition));
	/* By setting this to NULL, I insure panics if improperly used. */
	vtoc->vtoc_vtocFlag = NULL;
	vtoc->vtoc_part = NULL;
	vtoc->vtoc_nslices = 0;
	vtoc->vtoc_sanity = 0;
	vtoc->vtoc_active = VTOC_INVALID_FDISK;
}

/*
 * int
 * vtoc_FetchDivvy(vtoc_t *vtoc, uchar_t *buff)
 *
 * Read divvy block of the disk.
 */
STATIC int
vtoc_FetchDivvy(vtoc_t *vtoc, uchar_t *buff)
{
	struct phyio phyarg;
	struct partable divvytab;

	/* Read in the divvy table */
	/* HACK */
	phyarg.sectst  = vtoc->vtoc_unixst + TWO2PARLOC; 
	DBFLAG1(0x10,"vtoc_FetchDivvy = %d\n", phyarg.sectst);
	phyarg.memaddr = (long) buff;
	phyarg.datasz  = VTOC_SIZE;
	vtocphyrw(vtoc, V_RDABS, &phyarg, SD01_KERNEL);

	if (((struct partable *) buff)->p_magic != PAMAGIC) {
		return 1;
	}

	vtocAlignDivvy(buff, &divvytab);
	return vtocDivToVtoc(vtoc, &divvytab);
}

/*
 * STATIC void
 * vtocAlignDivvy(uchar_t *buff, struct partable *divvytab)
 *
 * We cannot do a straight cast of the divvy table. The leading short of
 * the structure is padded in UnixWare's internal representation.
 */
STATIC void
vtocAlignDivvy(uchar_t *buff, struct partable *divvytab)
{
	ushort_t *p = (ushort_t *) buff;

	divvytab->p_magic = *p++;
	bcopy((uchar_t *)p, (uchar_t *) divvytab->p,
	      sizeof(struct parts) * MAXPARTS);
}

/*
 * STATIC void
 * vtocDivToVtoc(vtoc_t *vtoc, struct partable *divvytab)
 *
 * Save the divvy table info into the incore vtoc.
 */
STATIC int
vtocDivToVtoc(vtoc_t *vtoc, struct partable *divvytab)
{
	int i;
	struct parts *divpart = divvytab->p;
	struct partition *uwpart;
	size_t flsz, ptsz;
#define CYLSZ(vtoc) (vtoc->vtoc_heads * vtoc->vtoc_sectors)

	vtoc->vtoc_sanity = VTOC_SANE;
	vtoc->vtoc_version = V_VERSION_DIVVY;
	vtoc->vtoc_nslices = MAXPARTS;

	/* Allocate the necessary memory */
	flsz = MAXPARTS * sizeof(int);
	ptsz = MAXPARTS * sizeof(struct partition);
	if (!(vtoc->vtoc_vtocFlag = (int *) kmem_zalloc(flsz, KM_SLEEP))) {
		return -1;
	}
	if (!(uwpart = vtoc->vtoc_part = (struct partition *)
				kmem_zalloc(ptsz, KM_SLEEP))) {
		kmem_free(vtoc->vtoc_vtocFlag, flsz);
		return -1;
	}

	for (i = 0; i < MAXPARTS; i++, uwpart++, divpart++) {
		uwpart->p_flag = V_VALID;
		if (divpart->p_siz > 0) {
			uwpart->p_tag = V_USR;
			uwpart->p_start = divpart->p_off *2 +
						(vtoc->vtoc_unixst/CYLSZ(vtoc) * CYLSZ(vtoc)) + CYLSZ(vtoc);
			uwpart->p_size = divpart->p_siz * 2;
		} else {
			uwpart->p_tag = V_UNUSED;
			uwpart->p_start = 0;
			uwpart->p_size = 0;
		}
	}
	return 0;
}

STATIC int
vtocphyrw(vtoc_t *vtoc, long dir, struct phyio *phyarg, int mode)
{
	buf_t *bp;
	int ret_val = 0;
	iovec_t	iovec;
	uio_t	uio;

	/*
	 * Maybe we should insure that no attempt is made to read beyond the
	 * last block.
	 */

	/* Set the return code to success. If it fails it will change later */
	phyarg->retval = 0;

	bzero(&iovec, sizeof(iovec_t));
	bzero(&uio, sizeof(uio_t));
	iovec.iov_base = (caddr_t)phyarg->memaddr;
	iovec.iov_len = phyarg->datasz;
	uio.uio_iov = &iovec;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = (mode == SD01_KERNEL) ? UIO_SYSSPACE : UIO_USERSPACE;
	uio.uio_resid = phyarg->datasz;

	bp = uiobuf(NULL, &uio);

	bp->b_flags |= (dir == V_RDABS? B_READ : B_WRITE);
	bp->b_blkno = phyarg->sectst;
	bp->b_blkoff = 0;
	bp->b_channelp = vtoc;

	buf_breakup(vtocphyrw_strat, bp, vtoc->vtoc_bcbp);

	ret_val = biowait(bp);

	if (ret_val) {
		phyarg->retval = dir == V_RDABS ? V_BADREAD : V_BADWRITE;
	}

	freerbuf(bp);
	return ret_val;
}

STATIC void
vtocphyrw_strat(buf_t *bp)
{
	vtoc_t *vtoc = bp->b_channelp;

	(*vtoc->vtoc_device.sdv_drvops->d_biostart)(vtoc->vtoc_device.sdv_idatap, 0, bp);

}

STATIC void
vtoc_sendSd01(vtoc_t *vtoc, struct pdinfo *pd)
{
	sdi_badBlockMsg_t msg;

	msg.vtoc = vtoc;
	msg.pd = pd;

	(*vtoc->vtoc_device.sdv_drvops->d_drvctl)
		(vtoc->vtoc_device.sdv_idatap, 0, SDI_DEVICE_BADBLOCK, (void *)&msg);
}

#ifdef DEBUG
/*
 * STATIC void
 * vtocPrtDivvy(struct partable *divvytab)
 *
 * Used for debugging with kdb.
 */
STATIC void
vtocPrtDivvy(struct partable *divvytab)
{
	int i;
	struct parts *divpart = divvytab->p;

	printf("Sanity=0x%x\n", divvytab->p_magic);
	for (i = 0; i < MAXPARTS; i++, divpart++) {
		printf("off=%d (0x%x) sz=%d (0x%x)\n",
		divpart->p_off, divpart->p_off, divpart->p_siz, divpart->p_siz);
	}
}

void
vtocbp(buf_t *bp)
{
	cmn_err(CE_CONT,"b_flags: 0x%x, ",bp->b_flags);
	cmn_err(CE_CONT,"b_bcount: 0x%x\n",bp->b_bcount);
	cmn_err(CE_CONT,"b_edev: 0x%x, ",bp->b_edev);
	cmn_err(CE_CONT,"b_blkno: 0x%x, ",bp->b_blkno);
	cmn_err(CE_CONT,"b_blkoff: 0x%x\n",bp->b_blkoff);
	cmn_err(CE_CONT,"get_blk: 0x%x, ",sdi_get_blkno(bp));
	cmn_err(CE_CONT,"b_error: 0x%x\n",bp->b_error);
}
#endif
