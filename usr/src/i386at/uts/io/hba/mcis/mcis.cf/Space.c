/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1991
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ident	"@(#)kern-i386at:io/hba/mcis/mcis.cf/Space.c	1.11.3.1"
#ident	"$Header$"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/immu.h>
#include <sys/errno.h>
#include <sys/cmn_err.h>
#include <sys/buf.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/bootinfo.h>
#include <sys/dma.h>
#include <sys/vtoc.h>
#include <sys/sdi_edt.h>
#include <sys/scsi.h>
#include <sys/sdi.h>
#include "config.h"

#ifndef MCIS__CNTLS
#define MCIS__CNTLS 1
#endif

/*
** Control the use of cache on the adapter:
**
** - if 1, read and write through the adapter cache
** - if 0, read and write bypass the cache
**
** NOTE: this flag applies only to read and write commands on direct access
** devices.  The cache is not used for tape devices because of their variable
** block size value; for other scsi commands, they all bypass the adapter
** cache
*/

int	mcis_cache = 0;	
int	mcis_global_tout = 600;	/* Number of seconds used for global	*/
				/* timeout on the mcis SCSI HBA	board.	*/
				/* The timeout value is increased from	*/
				/* the default of 45 sec. to account for*/
				/* the length of time needed for SCSI	*/
				/* tape retensioning.			*/

#ifdef  MCIS_CPUBIND

#define MCIS_VER        HBA_UW21_IDATA | HBA_IDATA_EXT

struct  hba_ext_info    mcis_extinfo = {
	0, MCIS_CPUBIND
};

#else

#define MCIS_VER        HBA_UW21_IDATA

#endif  /* MCIS_CPUBIND */

struct	hba_idata_v5	_mcis_idata[]	= {
	{ MCIS_VER, "(mcis,1) MCIS MCA SCSI",
	  7, 0, -1, 0, 0, 0,
	  0 }
#ifdef  MCIS_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&mcis_extinfo }
#endif
};

int	mcis__cntls	= MCIS__CNTLS;

/*
** The remainder of this space.c file is strictly for debugging
*/

/*
#define MCIS_PRDEBUG
*/

#ifdef MCIS_PRDEBUG
#define WATERM_H        100
#define WATERM_L        -100

mcisp_index(addr)
int	addr;
{
	cmn_err(CE_CONT, "mcis: print list\n");
	cmn_err(CE_CONT, "mcisp_xsb 	mcisp_scs	mcisp_scm\n");
	cmn_err(CE_CONT, "mcisp_dcb	mcisp_dev	mcisp_scb\n");
}

struct dev_blk *mcis_devp;
mcisp_dev(addr)
int	addr;
{
	struct dev_blk *devp;

	if (addr==NULL) {
		if (mcis_devp == NULL)
			return;
		addr = (int)mcis_devp;
	}

	mcis_devp = devp = (struct dev_blk *)addr;
	cmn_err(CE_CONT, "dev_blk (size= 0x%x)\n", sizeof(struct dev_blk));
	cmn_err(CE_CONT,
		"DEV_BLK tp_flag= 0x%x tp_devtype= 0x%x tp_devsubtype= 0x%x\n",
		devp->DTP.tp_flag, devp->DTP.tp_devtype, 
		devp->DTP.tp_devsubtype);
	cmn_err(CE_CONT,
		"secsiz= 0x%x maxblk= 0x%x minblk= 0x%x density= 0x%x\n",
		devp->DTP.tp_secsiz, devp->DTP.tp_maxblk, devp->DTP.tp_minblk, 
		devp->DTP.tp_density);
	cmn_err(CE_CONT, "tp_msp= 0x%x tp_opsp= 0x%x \n", 
		devp->DTP.tp_msp, devp->DTP.tp_opsp);
}

gdev_dcbp mcis_dcbp;
mcisp_dcb(addr)
int	addr;
{
	gdev_dcbp dcbp;
	struct	mcis_blk *blkp;
	int	i;

	if ((addr>WATERM_L) && (addr < WATERM_H)) {
		if (mcis_dcbp == NULL)
			return;
		addr = (int)mcis_dcbp + (addr * sizeof(struct gdev_ctl_block));
	}

	mcis_dcbp = dcbp = (gdev_dcbp)addr;
	cmn_err(CE_CONT, "DCB conf=0x%x ops=0x%x private=0x%x targetid=0x%x\n",
		dcbp->dcb_hba_conf, dcbp->dcb_hba_ops, dcbp->dcb_hba_private,
		dcbp->dcb_hba_targetid);
	blkp = dcb_mcis_blkp(dcbp);
	cmn_err(CE_CONT,
		"MCIS intr_code= 0x%x intr_dev= 0x%x allocmem= 0x%x\n",
		blkp->b_intr_code, blkp->b_intr_dev, blkp->b_allocmem);
	cmn_err(CE_CONT,
	"targetid= %d btarg= %d ioaddr= 0x%x dmachan= 0x%x b_intr= %d\n",
		blkp->b_targetid, blkp->b_boottarg, blkp->b_ioaddr, 
		blkp->b_dmachan, blkp->b_intr);
	cmn_err(CE_CONT, "ldcnt= %d numdev= %d scb_cnt= 0x%x b_scbp= 0x%x\n",
		blkp->b_ldcnt, blkp->b_numdev, blkp->b_scb_cnt, blkp->b_scbp);
	cmn_err(CE_CONT, "LDEV HEAD INDEX=(%d) ld_avfw= 0x%x ld_avbk= 0x%x\n",
		((int)ldmhd_fw(blkp)-(int)(blkp->b_ldm))/
		sizeof(struct mcis_ldevmap), ldmhd_fw(blkp), ldmhd_bk(blkp));
	for (i=0; i<blkp->b_ldcnt; i++) {
		cmn_err(CE_CONT,
		"LD[%d,x%x] avfw= 0x%x avbk= 0x%x, scbp= 0x%x <%d,%d>\n",
			i, &blkp->b_ldm[i], blkp->b_ldm[i].ld_avfw, 
			blkp->b_ldm[i].ld_avbk, blkp->b_ldm[i].ld_scbp, 
			blkp->b_ldm[i].LD_CB.a_targ,blkp->b_ldm[i].LD_CB.a_lun);
	}
}


struct	gdev_parm_block *mcis_dpbp;
mcisp_xsb(addr)
int	addr;
{
	struct	gdev_parm_block *dpbp;
	struct	xsb *xsbp;

	cmn_err(CE_CONT, "xsb (size= 0x%x)\n", sizeof(struct xsb));
	if ((addr>WATERM_L) && (addr < WATERM_H)) {
		if (mcis_dpbp == NULL)
			return;
		addr = (int)mcis_dpbp + (addr * sizeof(struct gdev_parm_block));
	}

	mcis_dpbp = dpbp = (struct gdev_parm_block *)addr;
	cmn_err(CE_CONT,
	"DPB=0x%x scep=0x%x cmdxsbp= 0x%x fltxsbp= 0x%x sc_addr= 0x%x\n",
		dpbp, dpbp->dpb_hba_sce, dpbp->dpb_hba_cmdxsb, dpbp->dpb_hba_fltxsb, dpbp->dpb_hba_addr);
	cmn_err(CE_CONT, "DPB xfer= 0x%x dcbp= 0x%x devp= 0x%x\n",
		dpbp->dpb_hba_xfer, dpbp->dpb_hba_dcb, dpbp->dpb_hba_dev);
	xsbp= dpb_hba_cmdxsbp(dpbp);
	cmn_err(CE_CONT, "CMDXSB dpb_p= 0x%x\n", HBA_dpb_p(xsbp));
	cmn_err(CE_CONT,
	"sc_cmdpt=0x%x sc_datapt= 0x%x sc_cmdsz= 0x%x sc_datasz= 0x%x\n",
		xsbp->SCB.sc_cmdpt, xsbp->SCB.sc_datapt, xsbp->SCB.sc_cmdsz,
		xsbp->SCB.sc_datasz);
	cmn_err(CE_CONT,
	"sc_comp_code=0x%x sc_cb= 0x%x sa_lun= 0x%x target= 0x%x\n",
		xsbp->SCB.sc_comp_code, xsbp->SCB.sc_cb, 
		XSB_lun(xsbp), XSB_targ(xsbp));
	xsbp= dpb_hba_fltxsbp(dpbp);
	cmn_err(CE_CONT, "FLTXSB dpb_p= 0x%x\n", HBA_dpb_p(xsbp));
	cmn_err(CE_CONT,
	"sc_cmdpt=0x%x sc_datapt= 0x%x sc_cmdsz= 0x%x sc_datasz= 0x%x\n",
		xsbp->SCB.sc_cmdpt, xsbp->SCB.sc_datapt, xsbp->SCB.sc_cmdsz,
		xsbp->SCB.sc_datasz);
	cmn_err(CE_CONT,
	"sc_comp_code=0x%x sc_cb= 0x%x sa_lun= 0x%x target= 0x%x\n",
		xsbp->SCB.sc_comp_code, xsbp->SCB.sc_cb, 
		XSB_lun(xsbp), XSB_targ(xsbp));
}


struct	mcis_scb *mcis_scbp;
mcisp_scb(addr)
int	addr;
{
	struct mcis_scb *bp;
	struct mcis_tsb *tsbp;

	cmn_err(CE_CONT, "mcis_scb (size= 0x%x)\n", sizeof(struct mcis_scb));
	cmn_err(CE_CONT, "mcis_tsb (size= 0x%x)\n", sizeof(struct mcis_tsb));

	if ((addr>WATERM_L) && (addr < WATERM_H)) {
		if (mcis_scbp == NULL)
			return;
		addr = (int)mcis_scbp + (addr * sizeof(struct mcis_scb));
	}
	mcis_scbp = bp = (struct  mcis_scb *)addr;
	tsbp = (struct mcis_tsb *)&(bp->s_tsb);

	cmn_err(CE_CONT, "SCB cmdop= 0x%x ns= %d nd=%d op= 0x%x baddr= 0x%x\n",
		bp->s_cmdop, bp->s_ns, bp->s_nd, bp->MSCB_op, bp->s_baddr);
	cmn_err(CE_CONT, "hostaddr= 0x%x hostcnt= 0x%x bcnt= 0x%x blen= %d\n",
		bp->s_hostaddr, bp->s_hostcnt, 
		*((ushort *)&bp->s_cdb[0]), *((ushort *)&bp->s_cdb[2]));
	cmn_err(CE_CONT,
		"ADDR tsb= 0x%x dmaseg= 0x%x cdb= 0x%x <%d:%d> blksz=%d\n",
		bp->s_tsbp, &bp->s_dmaseg[0], bp->s_cdb, bp->s_targ, bp->s_lun,
		bp->s_blksz);
	cmn_err(CE_CONT,
		"ownerp= 0x%x status= 0x%x retry= %d s_dmp= 0x%x sp= 0x%x\n",
		bp->s_ownerp, bp->s_status, bp->s_retry, bp->s_dmp, bp->s_sp);

	cmn_err(CE_CONT,
		"\nTSB status=0x%x resid=0x%x targlen= 0x%x targstat= 0x%x\n",
		tsbp->t_status, tsbp->t_resid,tsbp->t_targlen,tsbp->t_targstat);
	cmn_err(CE_CONT, "hastat= 0x%x targerr= 0x%x haerr= 0x%x\n",
		tsbp->t_hastat, tsbp->t_targerr, tsbp->t_haerr);
	cmn_err(CE_CONT, "crate= 0x%x cstat= 0x%x\n",
		tsbp->t_crate, tsbp->t_cstat);

}
struct	scs *mcis_scsp;
mcisp_scs(addr)
int	addr;
{
	struct	scs *scsp;

	cmn_err(CE_CONT, "scs (size= 0x%x)\n", sizeof(struct scs));
	mcis_scsp = scsp = (struct  scs *)addr;
	cmn_err(CE_CONT,
	"SCS op= 0x%x addr1= 0x%x addr= 0x%x lun=0x%x len= 0x%x cont= 0x%x\n",
		scsp->ss_op, scsp->ss_addr1, gsc_fromsc_short(scsp->ss_addr), 
		scsp->ss_lun, scsp->ss_len, scsp->ss_cont);
		
}

struct	scm *mcis_scmp;
mcisp_scm(addr)
int	addr;
{
	struct	scm *scmp;

	cmn_err(CE_CONT, "scm (size= 0x%x)\n", sizeof(struct scm));
	mcis_scmp = scmp = (struct  scm *)addr;
	cmn_err(CE_CONT,
		"SCS op= 0x%x addr= 0x%x lun=0x%x len= 0x%x cont= 0x%x\n",
		scmp->sm_op, gsc_fromsc_int(scmp->sm_addr), scmp->sm_lun, 
		gsc_fromsc_short(scmp->sm_len), scmp->sm_cont);
		
}


struct	mcis_scb *mcis_scbp;
mcisp_ccb(addr)
int	addr;
{
	struct	mcis_scb *scbp;

	cmn_err(CE_CONT, "mcis_scb (size= 0x%x)\n", sizeof(struct mcis_scb));
	if ((addr>WATERM_L) && (addr < WATERM_H)) {
		if (mcis_scbp == NULL)
			return;
		addr = (int)mcis_scbp + (addr * sizeof(struct mcis_scb));
	}

	mcis_scbp = scbp = (struct mcis_scb *)addr;
	cmn_err(CE_CONT, "MCIS SCB=0x%x  cmdop=0x%x cmdsup=0x%x op= 0x%x \n",
		mcis_scbp, scbp->s_cmdop, scbp->s_cmdsup, scbp->MSCB_op);
}

#endif
