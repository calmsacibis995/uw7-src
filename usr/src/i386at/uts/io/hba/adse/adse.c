#ident	"@(#)kern-pdi:io/hba/adse/adse.c	1.33.6.1"

/***************************************************************************
 *	Copyright (c) 1992  Adaptec Corporation				   *
 *	All Rights Reserved						   *
 *									   *
 *      ADAPTEC CORPORATION PROPRIETARY INFORMATION			   *
 *									   *
 *	This software is supplied under the terms of a license agreement   *
 *	or nondisclosure agreement with Adaptec Corporation and may not be *
 *	copied or disclosed except in accordance with the terms of that    *
 *	agreement.							   *
 ***************************************************************************
 *	V1.01 Changes							   *
 *	RAN - changed the init code to individually allocate memory for	   *
 *		each structure the driver needs.  this has the drawback of *
 *		not being able to free the memory at this time, but keeps  *
 *		memory allocation from ever crossing a page boundary.      *
 *	V1.4 12/11/92							   *
 *	- changed the adse_sgdone to correct re-starting commands correctly*
 *	V1.5 03/08/93							   *
 *	- added code to check before clearing status and request sense     *
 *	  structures arbitrarily					   *
 *	- added code to keep track of ECB's used in the HA structure	   *
 *	- moved the base interrupt to 12 from 9.  Some systems cannot	   *
 *	  handle 9 as it is a special interrupt which is cascaded from 2   *
 *	- changed the 3rd field from a 0 to a -1 in the system file to     *
 *	  allow the kernel to determine which adapter should be the boot   *
 *	  adapter.							   *
 *	V1.6 04/09/93							   *
 *	- fixed a bug where the ECB was being released free before the     *
 *	  system/kernel was actually done with it			   *
 *	- sent to the factory 04/10/93					   *
 *	V1.7 04/16/93							   *
 *	- took out the F_DMA setting as per USL's request.  According to   *
 *	  them this flag simply means that memory buffers must be below    *
 *	  the 16MB limit.  						   *
 *	- increasd the number of ECB's allocated to make sure there is     *
 *	  enough to run 7 devices with a couple of LUN's attached	   *
 *	- fixed a potential hole.  If the user set adse_io_per_tar to 0	   *
 *	  no ECB's would be allocated.  Forced this to a 2 if the user gets*
 *	  crazy and does this.						   *
 *	- increased the slots to be searched to 15			   *
 *	- sent to the factory and USL 04/19/93				   *
 *	V1.8 12/09/93							   *
 *	- added fixes as per USL instructions to correct the ioctl and	   *
 *	  pass_thru functions as they were loosing the bp pointer.	   *
 *	- switched from b_dev (the old major/minor scheme) to b_edev, which*
 *	  is the new extended major/minor number scheme in the ioctl/pass  *
 *	  thru fucntions.						   *
 *	- increased the timeout for the poll to 30 seconds from 1/2 seconds*
 *	- added the poll SCB structure to the host adapter structure so it *
 *	  can be cleared if a command timeout occurs during the poll	   *
 *	- added code to issue an ABORT to the adapter if, during the poll, *
 *	  a command timeout occurs.					   *
 *	- added the KM_SLEEP argument to the sdi_getblk call, as per ICL   *
 *	- took out the in port look for the interrupt in adseintr and just *
 *	  compared the vect argument with the adapter structure reference  *
 *	  ha->ha_vect to see what adapter caused the interrupt.		   *
 *	- added code to adseintr to handle shared interrupts		   *
 *	- changed a PANIC when level-triggered interrupts are detected to  *
 *	  a NOTE, as the released kernel ended up supporting both.	   *
 *	- added code to cause a timeout to occur on commands.  the timeout *
 *	  length is set to 30 seconds by default, but can be reset by the  *
 *	  user in the space.c. file.					   *
 *	- added 2 variables to the space.c to allow the user to enable or  *
 *	  disable the timeout code as well as control the amount of time   *
 *	  before a timeout will occur.					   *
 *	- saved the virtual pointers of the request sense data and status  *
 *	  block during init time so the drier would not constantly have to *
 *	  do a phystokv during runtime to get the address.		   *
 *	- saved the adapter in the ecb.  did this so the driver could get  *
 *	  the information if a timeout on the command occurs.		   *
 *	- moved the clearing of the sense data area and check from where   *
 *	  the command was started to be done after the sense data had been *
 *	  copied to the q.						   *
 *	- sent to the factory and USL on 12/21/93			   *
 ***************************************************************************/

/***************************************************************************
 *	Copyright (c) 1988, 1989  Intel Corporation			   *
 *	All Rights Reserved						   *
 *									   *
 *      INTEL CORPORATION PROPRIETARY INFORMATION			   *
 *									   *
 *	This software is supplied under the terms of a license agreement   *
 *	or nondisclosure agreement with Intel Corporation and may not be   *
 *	copied or disclosed except in accordance with the terms of that    *
 *	agreement.							   *
 ***************************************************************************/

/***************************************************************************
 * Copyrighted as an unpublished work.					   *
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990	   *
 * All rights reserved.							   *
 *									   *
 * RESTRICTED RIGHTS							   *
 *									   *
 * These programs are supplied under a license.  They may be used,	   *
 * disclosed, and/or copied only as permitted under such license	   *
 * agreement.  Any copy must contain the above copyright notice and	   *
 * this restricted rights notice.  Use, copying, and/or disclosure	   *
 * of the programs is strictly prohibited unless otherwise provided	   *
 * in the license agreement.						   *
 ***************************************************************************/
/***************************************************************************
 *									   *
 *	Enhanced mode SCSI Host Adapter Driver for Adaptec AHA-1740	   *
 *									   *
 ***************************************************************************/

#ifdef	_KERNEL_HEADERS

#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/cmn_err.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <util/debug.h>
#include <io/dma.h>
#include <util/mod/moddefs.h>
#include <util/ksynch.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi.h>
#include <io/target/sdi/sdi_hier.h>
#include <io/target/sdi/dynstructs.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/hba/adse/adse.h>

/* These must come last: */
#include <io/hba/hba.h>
#include <io/ddi.h>
#include <io/ddi_i386at.h>

#ifdef DDICHECK
#include <io/ddicheck.h>
#endif

#elif defined(_KERNEL)

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/cmn_err.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/kmem.h>
#include <sys/debug.h>
#include <sys/dma.h>
#include <sys/moddefs.h>
#include <sys/ksynch.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/sdi_hier.h>
#include <sys/dynstructs.h>
#include <sys/resmgr.h>
#include "adse.h"

/* These must come last: */
#include <sys/hba.h>
#include <sys/ddi.h>
#include <sys/ddi_i386at.h>

#endif			/* _KERNEL_HEADERS */

#define ADSE_BLKSHFT	  9	/* PLEASE NOTE:  Currently pass-thru	    */
#define ADSE_BLKSIZE	512	/* SS_READ/SS_WRITE, SM_READ/SM_WRITE	    */
				/* supports only 512 byte blocksize devices */

#define KMEM_ZALLOC adse_kmem_zalloc_physreq
#define KMEM_FREE kmem_free

#define ADSE_MEMALIGN	512
#define ADSE_BOUNDARY	0
#define ADSE_DMASIZE	32

STATIC void *adse_kmem_zalloc_physreq(size_t , int);
STATIC void	adse_pass_thru0(buf_t *);

STATIC struct ecb * adse_getblk(int);

STATIC	int 	adse_dmalist(sblk_t *, struct proc *, int),
		adse_illegal(short, uchar_t, uchar_t),
		adse_wait(int),
		adse_hainit(int);

STATIC void	 adse_flushq(struct scsi_lu *, int, int),
#ifdef NOT_YET
		adse_timeout(struct ecb *),
#endif
		adse_init_ecbs(int),
		adse_pass_thru(struct buf *),
		adse_int(struct sb *),
		adse_putq(struct scsi_lu *, sblk_t *),
		adse_next(struct scsi_lu *),
		adse_func(sblk_t *),
		adse_lockinit(int),
		adse_lockclean(int),
		adse_send(int, int, struct ecb *),
		adse_cmd(SGARRAY *, sblk_t *),
		adse_done(int, int, struct ecb *, int, struct sb *),
		adse_hadone(int, struct ecb *, int),
		adse_ha_immdone(int, struct ecb *, int),
		adse_sgdone(int, struct ecb *, int),
		adse_sctgth(struct scsi_lu *);

STATIC	long	adse_tol();

STATIC	SGARRAY	*adse_get_sglist(void);

STATIC	boolean_t		adseinit_time;	    /* Init time (poll) flag	*/
STATIC	volatile boolean_t		adse_waitflag = TRUE;
STATIC	int			adse_num_ecbs = 0;
STATIC	int			adse_nsg_lists = 0;
STATIC	int		adse_mod_dynamic = 0;
STATIC	int		adse_hacnt;	    /* # of ad1740 HBAs found   */
STATIC	adse_page_t			*adse_scatter;
STATIC	SGARRAY			*adse_sg_next;
STATIC	struct req_sense_def	*adse_req_sense;
STATIC	struct scsi_ha		*adse_sc_ha;	    /* SCSI HA structures	*/
STATIC	struct stat_def		*adse_ecb_stat;
STATIC	adse_page_t	*adse_dmalistp;	    /* pointer to DMA list pool	*/
STATIC	dma_t		*adse_dfreelistp;    /* List of free DMA lists	*/
STATIC	int		adse_ecb_structsize,
				adse_stat_structsize,
				adse_req_structsize,
				adse_luq_structsize,
				adse_dma_listsize,
				adse_ha_structsize,
				adse_sg_listsize;

/* Allocated in space.c */
extern struct ver_no	adse_sdi_ver;	    /* SDI version structure	*/
extern HBA_IDATA_STRUCT	_adseidata[];
extern int		adse_sg_enable;		/* s/g control byte */
extern int		adse_buson;		/* bus on time */
extern int		adse_io_per_tar;
#ifdef NOT_YET
extern int		adse_timeout_enable;
extern int		adse_timeout_count;
#endif
extern int		adse_cntls;
extern int		adse_slots;
extern int		adse_gtol[]; /* xlate global hba# to loc */
extern int		adse_ltog[]; /* local hba# to global     */

STATIC	sv_t   *adse_dmalist_sv;	/* sleep argument for UP	*/
STATIC int	adse_devflag = HBA_MP;
STATIC lock_t *adse_dmalist_lock;
STATIC lock_t *adse_sgarray_lock;
STATIC lock_t *adse_ecb_lock;
STATIC LKINFO_DECL(adse_lkinfo_dmalist, "IO:adse:adse_dmalist_lock", 0);
STATIC LKINFO_DECL(adse_lkinfo_sgarray, "IO:adse:adse_sgarray_lock", 0);
STATIC LKINFO_DECL(adse_lkinfo_ecb, "IO:adse:adse_ecb_lock", 0);
STATIC LKINFO_DECL(adse_lkinfo_hba, "IO:adse:ha->ha_hba_lock", 0);
STATIC LKINFO_DECL(adse_lkinfo_q, "IO:adse:q->q_lock", 0);

#ifdef AHA_DEBUG
int			adsedebug = 0;
#endif
#ifdef DEBUG
STATIC	int	adse_panic_assert = 0;
#endif

/*
 * This pointer will be used to reference the new idata structure
 * returned by sdi_hba_autoconf.  In the case of releases that do
 * not support autoconf, it will simply point to the idata structure
 * from the Space.c.
 */
HBA_IDATA_STRUCT	*adseidata;

/* The sm_poolhead struct is used for the dynamic struct allocation routines */
/* as pool to extarct from and return structs to. THis pool provides 28 byte */
/* structs (the size of sblk_t structs roughly).			     */
extern struct head	sm_poolhead;

#ifdef AHA_DEBUG_TRACE
#define AHA_DTRACE cmn_err(CE_NOTE,"%d-%s",__LINE__,__FILE__);
#else
#define AHA_DTRACE
#endif

#define		DRVNAME	"Adaptec Enhanced Mode SCSI HBA driver"

/* somewhere up top */
int adse_verify(rm_key_t);
MOD_ACHDRV_WRAPPER(adse, adse_load, adse_unload, NULL, adse_verify, DRVNAME);


HBA_INFO(adse, &adse_devflag, 0x10000);

int
adse_load(void)
{
	AHA_DTRACE;

	adse_mod_dynamic = 1;

	if ( adseinit()) {
		/*
		 * We may have allocated a new idata array at this point
		 * so attempt to free it before failing the load.
		 */
		sdi_acfree(adseidata, adse_cntls);
		return( ENODEV );
	}
	if ( adsestart()) {
		/*
		 * At this point we have allocated a new idata array,
		 * free it before failing.
		 */
		sdi_acfree(adseidata, adse_cntls);
		return( ENODEV );
	}
	return(0);
}

int
adse_unload(void)
{
	AHA_DTRACE;

	return(EBUSY);
}

/***************************************************************************
** Function name: adse_verify()						  **
** Description:								  **
**	This is the verify routine for the SCSI HA driver.		  **
**	It only checks for the existance of the boards, initialization	  **
**	and data structure allocation is done in the init routine.	  **
***************************************************************************/
int
adse_verify(rm_key_t key)
{ 
	HBA_IDATA_STRUCT        vfy_idata;
	int			j, rv;
	uint_t			port, slotnumber;

	AHA_DTRACE;

	/*
	 * Read the hardware parameters associated with the
	 * given Resource Manager key, and assign them to
	 * the idata structure.
	 */
	rv = sdi_hba_getconf(key, &vfy_idata);

	if(rv != 0)     {
		cmn_err(CE_WARN, "!adse_verify: sdi_hba_getconf failed");
		return(rv);
	}

	/*
	 * Verify hardware parameters in vfy_idata,
	 * return 0 on success, ENODEV otherwise.
	 */

	slotnumber = vfy_idata.ioaddr1 & ~ADSE_IOADDR_MASK;

	/*
	 * To be consistent with the rest of the adse driver at this
	 * point, we just and'ed off everything but the slot number
	 * and will or in the correct register where appropriate.
	 */

	port = slotnumber | HOSTID_PORT;

	j = inb(port);

	if(j != 0x04) {
		return ENODEV;
	}

	port = (slotnumber | HOSTID_PORT) + 1;
	j = inb(port);

	if(j != 0x90) {
		return ENODEV;
	}

	port = (slotnumber | HOSTID_PORT) + 2;
	j = inb(port);

	if(j != 0x00) {
		return ENODEV;
	}

	/*
	 * looks like we have found 'ADP0' in the EISA
	 * identifier ports, and that's us.  Now let's
	 * see if the adapter is in enhanced mode.
	 */

	port = slotnumber | IO_ADDRESS_PORT;
	j = inb(port);

	if(j & IO_ENHANCED) {
		/*
		 * Looks like we found us an adapter
		 * Need to save the slot it is in
		 * but first let's check to see if the adapter
		 * is alive.
		 */
		port = slotnumber | EXP_BD_PORT;
		j = inb(port);

		if(j & EXP_HAERR) {
			return ENODEV;
		}
	}

	/********************************************************
	 * Let's check to see if the interrupt matches what the	*
	 * system thinks the interrupt should be.  NOTE: If	*
	 * there is more than 1 host adapter, in enhanced mode,	*
 	 * in the system, it could be an ordering problem.	*
	 * I probably should change the search to look for the	*
	 * adapter by interrupt rather than slot address, but I	*
	 * need some time to think about this one.		*
	 ********************************************************/
		
	port = slotnumber | INT_DEF_PORT;
	j = inb (port);

	if(((j & I_MASK) + I_BASE) != vfy_idata.iov) {
		cmn_err(CE_WARN, "!adse_verify: Interrupt should be %d, not %d",
		   vfy_idata.iov, ((j & I_MASK) + I_BASE));
		return ENODEV;
	}
	/*****************************************************
	 * Now we have to see if a host adapter, in enhanced *
	 * mode, has it's interrupts enabled		     *
	 *****************************************************/
	if(!(j & I_INTEN)) {
		cmn_err(CE_WARN, "!adse_verify: Interrupts must be enabled");
		return ENODEV;
	}
	/*****************************************************
	 * Now we have to see if a host adapter, in enhanced *
	 * mode, has it's interrupts set to low true (native *
	 * EISA mode interrupts.		     *
	 *****************************************************/
	if(j & I_INTHIGH) {
		cmn_err(CE_NOTE, "!adse_verify: Interrupts are configured for standard mode");
		return ENODEV;
	}
	return(0);
}

#ifdef NOT_YET
STATIC void
adse_timeout(struct ecb *cp)
{
	AHA_DTRACE;

	adse_send(cp->c_ctlr, ABORT_COMMAND, cp);
}
#endif

/**************************************************************************
** adse_tol()								 **
**	This routine flips a 4 byte Intel ordered long to a 4 byte SCSI  **
**	style long.							 **
**************************************************************************/
STATIC long
adse_tol(char  adr[])
{
	union {
		char  tmp[4];
		long lval;
	}  x;

	x.tmp[0] = adr[3];
	x.tmp[1] = adr[2];
	x.tmp[2] = adr[1];
	x.tmp[3] = adr[0];
	return(x.lval);
}

/**************************************************************************
** Function name: adse_dma_freelist()					 **
** Description:								 **
**	Release a previously allocated scatter/gather DMA list.		 **
**************************************************************************/
STATIC void
adse_dma_freelist(dma_t *dmap)
{
	pl_t  oip;
	
	AHA_DTRACE;
	ASSERT(dmap);

	ADSE_DMALIST_LOCK(oip);
	dmap->d_next = adse_dfreelistp;
	adse_dfreelistp = dmap;
	if (dmap->d_next == NULL) {
		ADSE_DMALIST_UNLOCK(oip);
		SV_BROADCAST(adse_dmalist_sv, 0);
	} else
		ADSE_DMALIST_UNLOCK(oip);
}

/**************************************************************************
** Function name: adse_dmalist()					 **
** Description:								 **
**	Build the physical address(es) for DMA to/from the data buffer.	 **
**	If the data buffer is contiguous in physical memory, sp->s_addr	 **
**	is already set for a regular SB.  If not, a scatter/gather list	 **
**	is built, and the SB will point to that list instead.		 **
**************************************************************************/
STATIC int
adse_dmalist(sblk_t *sp, struct proc *procp, int sleepflag)
{
	struct dma_vect	tmp_list[SG_SEGMENTS];
	struct dma_vect  *pp;
	dma_t  *dmap;
	long   count, fraglen, thispage;
	caddr_t		vaddr;
	paddr_t		addr, base;
	int		i;
	pl_t	oip;

	AHA_DTRACE;

	vaddr = sp->sbp->sb.SCB.sc_datapt;
	count = sp->sbp->sb.SCB.sc_datasz;
	pp = &tmp_list[0];

	/* First build a scatter/gather list of physical addresses and sizes */

	for (i = 0; (i < SG_SEGMENTS) && count; ++i, ++pp) {
		base = vtop(vaddr, procp);	/* Phys address of segment */
		fraglen = 0;			/* Zero bytes so far */
		do {
			thispage = min(count, pgbnd(vaddr));
			fraglen += thispage;	/* This many more contiguous */
			vaddr += thispage;	/* Bump virtual address */
			count -= thispage;	/* Recompute amount left */
			if (!count)
				break;		/* End of request */
			addr = vtop(vaddr, procp); /* Get next page's address */
		} while (base + fraglen == addr);

		/* Now set up dma list element */
		pp->d_addr = base;
		pp->d_cnt = fraglen;
	}
	if (count != 0)
		cmn_err(CE_PANIC, "AHA-1740: Job too big for DMA list");

	/*
	 * If the data buffer was contiguous in physical memory,
	 * there was no need for a scatter/gather list; We're done.
	 */
	if (i > 1)
	{
		/*
		 * We need a scatter/gather list.
		 * Allocate one and copy the list we built into it.
		 */
		ADSE_DMALIST_LOCK(oip);
		if (!adse_dfreelistp && (sleepflag == KM_NOSLEEP)) {
			ADSE_DMALIST_UNLOCK(oip);
			return (1);
		}
		while ( !(dmap = adse_dfreelistp)) {
			SV_WAIT(adse_dmalist_sv, PRIBIO, adse_dmalist_lock);
			ADSE_DMALIST_LOCK(oip);
		}
		adse_dfreelistp = dmap->d_next;
		ADSE_DMALIST_UNLOCK(oip);

		sp->s_dmap = dmap;
		sp->s_addr = vtop((caddr_t) dmap->d_list, procp);
		dmap->d_size = i * sizeof(struct dma_vect);
		bcopy((caddr_t) &tmp_list[0],
			(caddr_t) dmap->d_list, dmap->d_size);
	}
	return (0);
}

/**************************************************************************
** Function name: adsexlat()						 **
** Description:								 **
**	Perform the virtual to physical translation on the SCB		 **
**	data pointer. 							 **
**************************************************************************/

/*ARGSUSED*/
HBAXLAT_DECL
HBAXLAT(struct hbadata *hbap, int flag, struct proc *procp, int sleepflag)
{
	sblk_t *sp = (sblk_t *) hbap;

	AHA_DTRACE;

	if(sp->s_dmap) {
		adse_dma_freelist(sp->s_dmap);
		sp->s_dmap = NULL;
	}

	if(sp->sbp->sb.SCB.sc_link) {
		cmn_err(CE_WARN, "Adaptec: Linked commands NOT available");
		sp->sbp->sb.SCB.sc_link = NULL;
	}

	if(sp->sbp->sb.SCB.sc_datapt && sp->sbp->sb.SCB.sc_datasz) {
		/*
		 * Do scatter/gather if data spans multiple pages
		 */
		sp->s_addr = vtop(sp->sbp->sb.SCB.sc_datapt, procp);
		if (sp->sbp->sb.SCB.sc_datasz > pgbnd(sp->s_addr))
			if (adse_dmalist(sp, procp, sleepflag))
				HBAXLAT_RETURN (SDI_RET_RETRY);
	} else
		sp->sbp->sb.SCB.sc_datasz = 0;

	HBAXLAT_RETURN (SDI_RET_OK);
}

/***************************************************************************
** Function name: adseinit()						  **
** Description:								  **
**	This is the initialization routine for the SCSI HA driver.	  **
**	All data structures are initialized, the boards are initialized	  **
**	and the EDT is built for every HA configured in the system.	  **
***************************************************************************/
int
adseinit()
{
	struct scsi_ha *ha;
	SGARRAY *sg_page, *sg_p;
	dma_t		*dp_page, *dp;
	int		ndma_list, ndma_per_page, ndma_pages;
	int		nsg_per_page, nsg_pages;
	int			c, i, j;
	int			sleepflag;
	uint_t			port, bus_type;

	AHA_DTRACE;

	/* if not running in an EISA machine, skip initialization */
	if (!drv_gethardware(IOBUS_TYPE, &bus_type) && (bus_type != BUS_EISA))
		return(-1);

	/*
	 * Allocate and populate a new idata array based on the
	 * current hardware configuration for this driver.
	 */
	adseidata = sdi_hba_autoconf("adse", _adseidata, &adse_cntls);
	if(adseidata == NULL)    {
		return (-1);
	}

	HBA_IDATA(adse_cntls);

	adseinit_time = TRUE;

	for(i = 0; i < MAX_HAS; i++)
		adse_gtol[i] = adse_ltog[i] = -1;

	sleepflag = adse_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;
	adse_sdi_ver.sv_release = 1;
	adse_sdi_ver.sv_machine = SDI_386_EISA;
	adse_sdi_ver.sv_modes   = SDI_BASIC1;

#ifdef NOT_YET
	if(adse_timeout_enable)
		adse_timeout_count *= 100;
#endif

	/* need to allocate space for adse_sc_ha, must be contiguous */
	adse_ha_structsize = adse_cntls*(sizeof (struct scsi_ha));
	for (i = 2; i < adse_ha_structsize; i *= 2);
	adse_ha_structsize = i;

	adse_sc_ha = (struct scsi_ha *)KMEM_ZALLOC(adse_ha_structsize, sleepflag);
	if (adse_sc_ha == NULL) {
		cmn_err(CE_WARN, "%s: %s command blocks", adseidata[0].name, AD_IERR_ALLOC);
		return(-1);
	}

	/*
	 * need to allocate space for mailboxes, ecb's, status_blocks, request
	 * sense blocks and lu queues they must all be contiguous
	 *
	 * To keep the user from over-flowing memory allocation, let's force
	 * adse_io_per_tar to a more resonable number if the user gets crazy
	 */
	if(adse_io_per_tar > 4)
		adse_io_per_tar = 4;
	else if(adse_io_per_tar == 0)
		adse_io_per_tar = 2;
	adse_num_ecbs = (adse_io_per_tar * 8) + 8;

	adse_ecb_structsize = adse_num_ecbs*(sizeof (struct ecb));
	adse_stat_structsize = adse_num_ecbs*(sizeof (struct stat_def));
	adse_req_structsize = adse_num_ecbs*(sizeof (struct req_sense_def));
	adse_luq_structsize = MAX_EQ*(sizeof (struct scsi_lu));

#ifdef	DEBUG
	if(adse_ecb_structsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: ECBs exceed pagesize (may cross physical page boundary)", adseidata[0].name);
	if(adse_stat_structsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: STATUS Blocks exceed pagesize (may cross physical page boundary)", adseidata[0].name);
	if(adse_req_structsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: Request Sense Blocks exceed pagesize (may cross physical page boundary)", adseidata[0].name);
#endif /* DEBUG */

	/*
	 * Allocate the space for the dma_list structures.
	 *
	 * We figure out how many dma_lists we need and how many fit
	 * on one page.  We then allocate enough pages to hold all we
	 * need.  This gives us more than we need but minimizes the total
	 * number of pages we use.  It also gaurantees that no single
	 * dma_list will cross a page boundary.
	 *
	 * NOTE: this list is not circular.  When we reach the NULL signifying
	 * EOL we sleep until one free's up.
	 */
	ndma_list = adse_cntls * adse_num_ecbs;
	adse_dma_listsize = sizeof(dma_t);
	ndma_per_page = PAGESIZE / adse_dma_listsize;
#ifdef DEBUG
	if (!ndma_per_page)
		cmn_err(CE_WARN, "%s: dmalist exceeds pagesize", adseidata[0].name);
#endif
	ndma_pages = ( ndma_list / ndma_per_page ) + 1;

	adse_dma_listsize = ndma_pages*PAGESIZE;
	adse_dmalistp = (adse_page_t *)KMEM_ZALLOC(adse_dma_listsize,sleepflag);
	if (adse_dmalistp == NULL) {
		cmn_err(CE_WARN, "%s: %s dmalist\n", adseidata[0].name, AD_IERR_ALLOC);
		KMEM_FREE(adse_sc_ha, adse_ha_structsize);
		return(-1);
	}

	adse_dfreelistp = NULL;

	for (i = 0; i < ndma_pages; i++) {
		dp_page = (dma_t *)(void *)&adse_dmalistp[i];
		/* Build list of free DMA lists */
		for (j = 0; j < ndma_per_page; j++) {
			dp = &dp_page[j];
			dp->d_next = adse_dfreelistp;
			adse_dfreelistp = dp;
		}
	}

/*
 * Now add the initialization for the scatter/gather lists and the pointers
 * for the sblk_t sp arrays to keep track of.  I have opted for circular
 * queue (linked lists) of each of these lists.  In the routine that returns
 * a free list I will keep a forward moving global that will keep pointing
 * to the next array.  This will almost always insure the free pointer will
 * be found in the first check, keeping overhead to a minimum. (i.e. atomic)
 */
	adse_nsg_lists = adse_cntls * adse_num_ecbs;
	adse_sg_listsize = sizeof(SGARRAY);
	nsg_per_page = PAGESIZE / adse_sg_listsize;
#ifdef DEBUG
	if(!nsg_per_page)
		cmn_err(CE_WARN, "%s: sg list exceeds pagesize", adseidata[0].name);
#endif
	nsg_pages = ( adse_nsg_lists / nsg_per_page ) + 1;

	adse_sg_listsize = nsg_pages*PAGESIZE;
	adse_scatter = (adse_page_t *)KMEM_ZALLOC(adse_sg_listsize, sleepflag);
	if(adse_scatter == NULL) {
		cmn_err(CE_WARN, "%s: %s sg array", adseidata[0].name, AD_IERR_ALLOC);
		KMEM_FREE(adse_dmalistp, adse_dma_listsize);
		KMEM_FREE(adse_sc_ha, adse_ha_structsize);
		return(-1);
	}

	adse_sg_next = SGNULL;

	for(i = 0; i < nsg_pages; i++) {
		sg_page = (SGARRAY *)(void *)&adse_scatter[i];

		for(j = 0; j < nsg_per_page; j++) {
			sg_p = &sg_page[j];
			sg_p->sg_flags = SG_FREE;
			sg_p->sg_next = adse_sg_next;
			adse_sg_next = sg_p;
		}
	}
	((SGARRAY *)(void *)adse_scatter)->sg_next = (SGARRAY *)adse_sg_next;

	/*
	 * allocate/initialize 'per controller' structures and HA
	 */
	for (c = 0; c < adse_cntls; c++) {
		ha = &adse_sc_ha[c];

		/* the physical slot number for this adapter */
		ha->ha_slot = (adseidata[c].ioaddr1 & ~ADSE_IOADDR_MASK);
		j = inb(ha->ha_slot | IO_ADDRESS_PORT);
		if(j & IO_ENHANCED) {
			/*
			 * board is in enhanced mode, make sure its alive
			 */
			j = inb(ha->ha_slot | EXP_BD_PORT);
			if(j & EXP_HAERR)
				continue;
		} else
			continue;

		ha->ha_ecb = (struct ecb *)KMEM_ZALLOC(adse_ecb_structsize, sleepflag);
		if (ha->ha_ecb == NULL) {
			cmn_err(CE_WARN, "%s: %s command blocks",
			    adseidata[c].name, AD_IERR_ALLOC);
			continue;
		}
		ha->ha_pecb = vtop((caddr_t)ha->ha_ecb, NULL);

		adse_ecb_stat = (struct stat_def *)KMEM_ZALLOC(adse_stat_structsize, sleepflag);
		if (adse_ecb_stat == NULL) {
			cmn_err(CE_WARN, "%s: %s status blocks",
			    adseidata[c].name, AD_IERR_ALLOC);
			continue;
		}

		adse_req_sense = (struct req_sense_def *)KMEM_ZALLOC(adse_req_structsize, sleepflag);
		if (adse_req_sense == NULL) {
			cmn_err(CE_WARN, "%s: %s request sense blocks",
			    adseidata[c].name, AD_IERR_ALLOC);
			continue;
		}

		ha->ha_dev = (struct scsi_lu *)KMEM_ZALLOC(adse_luq_structsize, sleepflag);
		if (ha->ha_dev == NULL) {
			cmn_err(CE_WARN, "%s: %s lu queues",
			    adseidata[c].name, AD_IERR_ALLOC);
			continue;
		}

		ha->ha_state = 0;
		ha->ha_id = adseidata[c].ha_id;

		/********************************************************
		 * Let's check to see if the interrupt matches what the	*
		 * system thinks the interrupt should be.  NOTE: If	*
		 * there is more than 1 host adapter, in enhanced mode,	*
	 	 * in the system, it could be an ordering problem.	*
		 * I probably should change the search to look for the	*
		 * adapter by interrupt rather than slot address, but I	*
		 * need some time to think about this one.		*
		 ********************************************************/
		
		j = inb(ha->ha_slot | INT_DEF_PORT);
		if(((j & I_MASK) + I_BASE) != adseidata[c].iov) {
			cmn_err(CE_WARN, "!%s: Interrupt should be %d, not %d",
			   adseidata[c].name, adseidata[c].iov, ((j & I_MASK) + I_BASE));
			continue;
		}
		/*****************************************************
		 * Now we have to see if a host adapter, in enhanced *
		 * mode, has it's interrupts enabled		     *
		 *****************************************************/
		if(!(j & I_INTEN)) {
			cmn_err(CE_WARN, "!%s: Interrupts must be enabled",
			   adseidata[c].name);
			continue;
		}
		/*****************************************************
		 * Now we have to see if a host adapter, in enhanced *
		 * mode, has it's interrupts set to low true (native *
		 * EISA mode interrupts.		     *
		 *****************************************************/
		if(j & I_INTHIGH) {
			cmn_err(CE_WARN, "%s: Interrupts are currently configured for standard mode", adseidata[c].name);
		}

		adse_sc_ha[c].ha_vect = adseidata[c].iov;

		adse_init_ecbs(c);

		if(adse_hainit(c)) { 	/* initialize HA communication */
			adseidata[c].active = 1;
			adse_lockinit(c);  /* allocate, and initialize locks */
		}
	}

	j = 0;
	for (c = 0; c < adse_cntls; c++) {
		if (adseidata[c].active)
			j++;
	}
	if(!j) {
		return(-1);
	}

	/*
	 * Attach interrupts for each "active" device. The driver
	 * must set the active field of the idata structure to 1
	 * for each device it has configured.
	 *
	 * Note: Interrupts are attached here in init() as opposed
	 * to load() because this call is required for both static
	 * and loadable autoconfig drivers.
 	 */
	sdi_intr_attach(adseidata, adse_cntls, adseintr, adse_devflag);
	return(0);
}

/****************************************************************************
** Function name: adsestart()						   **
** Description:								   **
**	Called by kernel to perform driver initialization		   **
**	after the kernel data area has been initialized.		   **
****************************************************************************/
int
adsestart()
{
	int	c, cntl_num;
	struct scsi_ha *ha;

	AHA_DTRACE;


	for (adse_hacnt = 0, c = 0; c < adse_cntls; c++) {
		if (!adseidata[c].active)
			continue;

		/* Get an HBA number from SDI and Register HBA with SDI */
		if( (cntl_num = sdi_gethbano(adseidata[c].cntlr)) <= -1) {
	     	cmn_err (CE_WARN,"%s: No HBA number available.", adseidata[c].name);
			adseidata[c].active = 0;
			continue;
		}

		adseidata[c].cntlr = cntl_num;
		adse_gtol[cntl_num] = c;
		adse_ltog[c] = cntl_num;

		if( (cntl_num = sdi_register(&adsehba_info, &adseidata[c])) < 0) {
	     	cmn_err (CE_WARN,"!%s: HA %d, SDI registry failure %d.",
				adseidata[c].name, c, cntl_num);
			adseidata[c].active = 0;
			continue;
		}

		adse_hacnt++;
	}

	for (c = 0; c < adse_cntls; c++) {
		adse_lockclean(c);
		if( !adseidata[c].active) {
			ha = &adse_sc_ha[c];

			if( ha->ha_ecb != NULL) {
				KMEM_FREE((void *)ha->ha_ecb->c_stat_ptr, adse_stat_structsize);
				KMEM_FREE((void *)ha->ha_ecb->c_s_ptr, adse_req_structsize);
				KMEM_FREE((void *)ha->ha_ecb, adse_ecb_structsize);
				ha->ha_ecb = NULL;
			}

			if( ha->ha_dev != NULL) {
				KMEM_FREE((void *)ha->ha_dev, adse_luq_structsize);
				ha->ha_dev = NULL;
			}
		}
	}

	/*
	/*
	 * Clear init time flag to stop the HA driver
	 * from polling for interrupts and begin taking
	 * interrupts normally.
	 */
	adseinit_time = FALSE;

	if (adse_hacnt) {
		return(0);
	} else {
		KMEM_FREE(adse_dmalistp, adse_dma_listsize);
		KMEM_FREE(adse_scatter, adse_sg_listsize);
		KMEM_FREE(adse_sc_ha, adse_ha_structsize);

		return(-1);
	}
}

/***************************************************************************
** adse_hainit()							  **
**	This is the guy that does all the hardware initialization of the  **
**	adapter after it has been determined to be in the system.	  **
***************************************************************************/
STATIC int
adse_hainit(int c)
{
	struct scsi_ha  *ha = &adse_sc_ha[c];
	int	i;
	unsigned char	status;

	AHA_DTRACE;

	status = inb(ha->ha_slot | STATUS1_PORT);
	if(status & ST1_BUSY_SET) {
		cmn_err(CE_CONT, "!%s %d BUSY at slot %d",
		    adseidata[c].name, adse_ltog[c],
		    (ha->ha_slot >> SLOT_SHIFT));
		return(0);
	} else {
		/* set the buson time */
		if(adse_buson != 0 && adse_buson != 4 && adse_buson != 8) {
			cmn_err(CE_NOTE,
			   "%s: Illegal bus on time of %d, setting to 0",
			   adseidata[c].name, adse_buson);
			adse_buson = 0;
		}
		i = inb(ha->ha_slot | BUS_DEF_PORT);
		i |= adse_buson;
		outb((ha->ha_slot | BUS_DEF_PORT), i);
		cmn_err(CE_NOTE, "!V1.8a %s: found in slot %d",
			adseidata[c].name, (ha->ha_slot >> SLOT_SHIFT));
	}
	return(1);
}

/***************************************************************************
** Function name: adse_init_ecbs()					  **
** Description:								  **
**	Initialize the adapter ECB free list.				  **
***************************************************************************/
STATIC void
adse_init_ecbs(int c)
{
	struct scsi_ha		*ha = &adse_sc_ha[c];
	struct ecb		*cp;
	int			i;

	AHA_DTRACE;

	for(i = 0; i < adse_num_ecbs; i++) {
		cp = &ha->ha_ecb[i];
		cp->c_addr = vtop((caddr_t)cp, NULL);
		cp->c_next = &ha->ha_ecb[i+1];
		cp->c_stat_ptr = &adse_ecb_stat[i];
		cp->status_blk_ptr = vtop((caddr_t)cp->c_stat_ptr, NULL);
		cp->c_s_ptr = &adse_req_sense[i];
		cp->sense_length = sizeof(struct req_sense_def);
		cp->sense_ptr = vtop((caddr_t)cp->c_s_ptr, NULL);
	}
	cp->c_next = &ha->ha_ecb[0];
	ha->ha_free = cp;
}

/**************************************************************************
** Function name: adseopen()						 **
** Description:								 **
** 	Driver open() entry point. It checks permissions, and in the	 **
**	case of a pass-thru open, suspends the particular LU queue.	 **
**************************************************************************/

/*ARGSUSED*/
int
HBAOPEN(dev_t *devp, int flags, int otype, cred_t *cred_p)
{
	dev_t	dev = *devp;
	int	c = adse_gtol[SC_HAN(dev)];
	int	t = SC_TCN(dev);
	int	l = SC_LUN(dev);
	struct scsi_lu *q;

	AHA_DTRACE;

	if (t == adse_sc_ha[c].ha_id)
		return(0);

	if (adse_illegal(SC_HAN(dev), t, l)) {
		return(ENXIO);
	}

	/* This is the pass-thru section */

	q = &ADSE_LU_Q(c, t, l);

	ADSE_SCSILU_LOCK(q->q_opri);
 	if ((q->q_outcnt > 0)  || (q->q_flag & (QBUSY | QSUSP | QPTHRU))) {
		ADSE_SCSILU_UNLOCK(q->q_opri);
		return(EBUSY);
	}

	q->q_flag |= QPTHRU;
	ADSE_SCSILU_UNLOCK(q->q_opri);
	return(0);
}

/***************************************************************************
** Function name: adseclose()						  **
** Description:								  **
** 	Driver close() entry point.  In the case of a pass-thru close	  **
**	it resumes the queue and calls the target driver event handler	  **
**	if one is present.						  **
***************************************************************************/

/*ARGSUSED*/
int
HBACLOSE(dev_t dev, int flags, int otype, cred_t *cred_p)
{
	int	c = adse_gtol[SC_HAN(dev)];
	int	t = SC_TCN(dev);
	int	l = SC_LUN(dev);
	struct scsi_lu *q;

	AHA_DTRACE;

	if (t == adse_sc_ha[c].ha_id)
		return(0);

	q = &ADSE_LU_Q(c, t, l);

	ADSE_CLEAR_QFLAG(q,QPTHRU);

#ifdef AHA_DEBUG1
	sdi_aen(SDI_FLT_PTHRU, c, t, l);
#else
	if (q->q_func != NULL)
		(*q->q_func) (q->q_param, SDI_FLT_PTHRU);
#endif

	ADSE_SCSILU_LOCK(q->q_opri);
	adse_next(q);
	return(0);
}

/**************************************************************************
** Function name: adseintr()						 **
** Description:								 **
**	Driver interrupt handler entry point.  Called by kernel when	 **
**	a host adapter interrupt occurs.				 **
**************************************************************************/
void
adseintr(uint vect)
{
	struct scsi_ha	*ha;
	struct ecb	*cp, *ecbchain, *ecbhead = NULL;
	int		adapter;
	int			i, int_status;
	struct scsi_lu		*q;
	char			target;
	paddr_t			pecb;
	pl_t		oip;
#ifdef AHA_DEBUG
	int				n;
#endif

	AHA_DTRACE;

	/*
	 * Determine which host adapter interrupted from
	 * the interrupt vector.
	 */
	for (adapter = 0; adapter < adse_cntls; adapter++) {
		ha = &adse_sc_ha[adapter];
		if(ha->ha_vect != vect)
			continue;

		ADSE_HBA_LOCK(oip);

		i = inb(ha->ha_slot | STATUS1_PORT);
		if (!(i & ST1_INT_PENDING)) {
			ADSE_HBA_UNLOCK(oip);
			continue;
		}

		if (!adseidata[adapter].active) {
			cmn_err(CE_WARN,"!%d/%s: Inactive HBA received an interrupt.",
				 adapter, adseidata[adapter].name);
			outb((ha->ha_slot | CONTROL_PORT), CLEAR_INTERRUPT);
			ADSE_HBA_UNLOCK(oip);
			continue;
		}

		int_status = inb(ha->ha_slot | INT_STATUS_PORT);
		target = int_status & INT_TAR_MASK;
		int_status &= ~INT_TAR_MASK;

		switch(int_status) {
		case INT_IMM_ERROR:
		case INT_IMM_SUCCES:
			q = &ADSE_LU_Q(adapter, target, 0);
			cp = q->q_func_cp;

			/*
			 * Chain completed ecb's for done loop.
			 */
			if (ecbhead == NULL) {
				ecbhead = ecbchain = cp;
			} else {
				ecbchain->c_donenext = cp;
				ecbchain = cp;
			}
			cp->c_donenext = NULL;
			cp->c_status = int_status;

			break;

		case INT_CCB_SUCCESS:
		case INT_CCB_RETRY:
		case INT_ADAPTER_BAD:
		case INT_CCB_ERROR:
			pecb = (paddr_t)inl(ha->ha_slot | MBOX_IN_PORT);
			cp = ADSE_ECB_PHYSTOKV(ha, pecb);
#ifdef NOT_YET
			if(adse_timeout_enable)
				untimeout(cp->c_tm_id);
#endif
			ha->ha_npend--;

			/*
			 * Chain completed ecb's for done loop.
			 */
			if (ecbhead == NULL) {
				ecbhead = ecbchain = cp;
			} else {
				ecbchain->c_donenext = cp;
				ecbchain = cp;
			}
			cp->c_donenext = NULL;
			cp->c_status = int_status;

			break;
		case INT_AEN:
#ifdef AHA_DEBUG
			n = inb(ha->ha_slot | MBOX_IN_PORT);
			cmn_err(CE_WARN,"AEN error target %d - code 0x%x", target, n);
#else
			(void)inb(ha->ha_slot | MBOX_IN_PORT);
#endif
			break;
		default:
#ifdef NOT_YET
			if(adse_timeout_enable) {
				pecb = (paddr_t)inl(ha->ha_slot | MBOX_IN_PORT);
				cp = ADSE_ECB_PHYSTOKV(ha, pecb);
				untimeout(cp->c_tm_id);
			}
#endif
			cmn_err(CE_WARN,"!%s: Invalid interrupt status - 0x%x",
				 adseidata[adapter].name, int_status);
			break;
		}
		outb((ha->ha_slot | CONTROL_PORT), CLEAR_INTERRUPT);
		ADSE_HBA_UNLOCK(oip);
	}
	/*
	 * Now perform the done processing for each completed ccb.
	 */
	ecbchain = ecbhead;
	while ((cp = ecbchain)) {
		ecbchain = cp->c_donenext;

		if (cp->c_status == INT_IMM_ERROR || cp->c_status == INT_IMM_SUCCES) {
			adse_ha_immdone(cp->c_ctlr, cp, cp->c_status);
		} else {
			q = &ADSE_LU_Q(cp->c_ctlr, cp->c_target, \
			              (cp->flag2 & FLAG2_LUN_MASK));

			ADSE_SCSILU_LOCK(q->q_opri);
			q->q_outcnt--;
			if(q->q_outcnt < adse_io_per_tar)
				q->q_flag &= ~QBUSY;
			ADSE_SCSILU_UNLOCK(q->q_opri);

			if((cp->command_word == INIT_SCSI_COMM) || 
			     (cp->flag1 & FLAG1_SG))
				adse_sgdone(cp->c_ctlr, cp, cp->c_status);
			else
				adse_hadone(cp->c_ctlr, cp, cp->c_status);
		}
	}
	adse_waitflag = FALSE;
}

/**************************************************************************
** Function: adse_sgdone()						 **
** Description:								 **
**	This routine is used as a front end to calling adse_done.	 **
**	This is the cleanest way I found to cleanup a local s/g command. **
**	One should note, however, with this scheme, if an error occurs	 **
**	during the local s/g command, the system will think an error	 **
**	will have occured on all the commands.				 **
**************************************************************************/
STATIC void
adse_sgdone(int c, struct ecb *cp, int status)
{
	sblk_t		*sb;
	struct sb	*sp;
	int		i;
	long			x;

	AHA_DTRACE;

	if((cp->flag1 & FLAG1_SG) && cp->c_sg_p != SGNULL) {
		x = cp->data_length;
		x /= SG_LENGTH;
		x--;
		for(i = 0; i < x; i++) {
			sb = cp->c_sg_p->spcomms[i];
			ASSERT(sb);
			adse_done(SG_NOSTART, c, cp, status, &sb->sbp->sb);
		}
		sb = cp->c_sg_p->spcomms[i];
		adse_done(SG_START, c, cp, status, &sb->sbp->sb);
		cp->c_sg_p->sg_flags = SG_FREE;
		cp->c_sg_p = SGNULL;
		FREEECB(cp);
	} else { /* call target driver interrupt handler */
		sp = cp->c_bind;
		adse_done(SG_START, c, cp, status, sp);
		FREEECB(cp);
	}
}

/**************************************************************************
** Function name: adse_hadone()					 **
** Description:								 **
**	This is the interrupt handler routine for SCSI jobs which have	 **
**	a controller CB and a SFB structure defining the job.		 **
**************************************************************************/
STATIC void
adse_hadone(int c, struct ecb *cp, int status)
{
	struct scsi_lu	*q;
	struct sb	*sp;
	char			t,l;

	AHA_DTRACE;

	t = cp->c_target;
	l = cp->flag2 & FLAG2_LUN_MASK;
	q = &ADSE_LU_Q(c, t, l);

	sp = cp->c_bind;

	ASSERT(sp);

	/* Determine completion status of the job */

	switch (status) {
		case INT_CCB_SUCCESS:
		case INT_CCB_RETRY:
			sp->SFB.sf_comp_code = SDI_ASW;
			adse_flushq(q, SDI_CRESET, 0);
			break;
		case INT_CCB_ERROR:
			sp->SCB.sc_comp_code = (unsigned long)SDI_ERROR;
			break;
		case INT_ADAPTER_BAD:
			sp->SFB.sf_comp_code = (unsigned long)SDI_HAERR;
			break;
		default:
			ASSERT(adse_panic_assert);
			sp->SCB.sc_comp_code = (unsigned long)SDI_ERROR;
			break;
	}

	/* call target driver interrupt handler */
	sdi_callback(sp);

	FREEECB(cp);

	ADSE_SCSILU_LOCK(q->q_opri);
	adse_next(q);
} 

/**************************************************************************
** Function name: adse_done()						 **
** Description:								 **
**	This is the interrupt handler routine for SCSI jobs which have	 **
**	a controller CB and a SCB structure defining the job.		 **
**************************************************************************/
STATIC void
adse_done(int flag, int c, struct ecb *cp, int status, struct sb *sp)
{
	struct scsi_lu	*q;
	int		i;
	char			target, lun;
	struct stat_def		*stat;

	AHA_DTRACE;

	target = cp->c_target;
	lun = cp->flag2 & FLAG2_LUN_MASK;
	q = &ADSE_LU_Q(c, target, lun);
	stat = cp->c_stat_ptr;

	ADSE_CLEAR_QFLAG(q,QSENSE);	/* Old sense data now invalid */

	/* Determine completion status of the job */
	switch(status) {
	case ADSE_SDI_RETRY:
		sp->SCB.sc_comp_code = (ulong_t)SDI_RETRY;
		break;

	case INT_CCB_SUCCESS:
	case INT_CCB_RETRY:
		sp->SCB.sc_comp_code = SDI_ASW;
		break;

	case INT_ADAPTER_BAD:
	case INT_CCB_ERROR:
		/********************************************************/
		/* not a good completion - so let's see what went wrong */
		/* or what the adapter thinks went wrong		*/
		/********************************************************/
		if((stat->status_word & STAT_DON) == 0) {
			if(stat->status_word & STAT_DU) {
				if((cp->flag1 & FLAG1_SES) == 0) {
					cmn_err(CE_WARN, "%s: Data Underrun Error",
						adseidata[c].name);
				}
			}
			if(stat->status_word & STAT_QF) {
				cmn_err(CE_WARN, "%s: Adapter Queue Full", 
					adseidata[c].name);
			}
			if(stat->status_word & STAT_SC) {
				cmn_err(CE_WARN, "%s: Specification Check",
					adseidata[c].name);
			}
			if(stat->status_word & STAT_DO) {
				cmn_err(CE_WARN, "%s: Data Overrun Error",
					adseidata[c].name);
			}
			if(stat->status_word & STAT_CH) {
				cmn_err(CE_WARN, "%s: Chaining Halted",
					adseidata[c].name);
			}
			if(stat->status_word & STAT_INI) {
				cmn_err(CE_WARN, "%s: Initialization Required",
					adseidata[c].name);
			}
			if(stat->status_word & STAT_ME) {
				if((cp->flag1 & FLAG1_SES) == 0 &&
				   (stat->status_word & STAT_DU)) {
					cmn_err(CE_WARN, "%s: Major Error/Exception",
						adseidata[c].name);
				}
			}
			if(stat->status_word & STAT_ECA) {
				cmn_err(CE_WARN, "%s: Extended Contigent Allegiance",
					adseidata[c].name);
			}
		}
		switch(stat->ha_stat) {
		case AD_NHASA:
			if (stat->target_stat == TAR_GOOD) {
				sp->SCB.sc_comp_code = SDI_ASW;
			} else {
				sp->SCB.sc_comp_code = (unsigned long)SDI_CKSTAT;
				sp->SCB.sc_status = stat->target_stat;
			/* Cache request sense info (note padding) */
				if(stat->status_word & STAT_SNS) {
					if (stat->sense_len < (sizeof(q->q_sense)-1))
						i = stat->sense_len;
					else
						i = sizeof(q->q_sense) - 1;
					bcopy((caddr_t)(cp->c_s_ptr), (caddr_t)(sdi_sense_ptr(sp))+1, i);

					ADSE_SCSILU_LOCK(q->q_opri);
					bcopy((caddr_t)(cp->c_s_ptr), (caddr_t)(&q->q_sense)+1, i);
					q->q_flag |= QSENSE;	/* sense data now valid */
					ADSE_SCSILU_UNLOCK(q->q_opri);
				}

			}
			break;
		case AD_ST:
			sp->SCB.sc_comp_code = (unsigned long)SDI_NOSELE;
			break;
		case AD_IBPD:
			sp->SCB.sc_comp_code = (unsigned long)SDI_TCERR; 
			break;
		case AD_CABH:
		case AD_CABHA:
			sp->SCB.sc_comp_code = (unsigned long)SDI_ABORT;
			break;
		case AD_FND:
			cmn_err(CE_PANIC, "%s: Adapter Firmware not downloaded",
				 adseidata[c].name);
			break;
		case AD_DOOUO:
			cmn_err(CE_WARN, "%s: Data over/under run error",
				 adseidata[c].name);
			sp->SCB.sc_comp_code = (unsigned long)SDI_HAERR;
			break;
		case AD_UBFO:
			cmn_err(CE_WARN, "%s: Unexpected Bus Free error",
				 adseidata[c].name);
			sp->SCB.sc_comp_code = (unsigned long)SDI_HAERR;
			break;
		case AD_IOC:
			cmn_err(CE_WARN, "%s: Unexpected Bus Phase error",
				 adseidata[c].name);
			sp->SCB.sc_comp_code = (unsigned long)SDI_HAERR;
			break;
		case AD_ICBP:
			cmn_err(CE_WARN, "%s: Invalid Control Block error",
				 adseidata[c].name);
			sp->SCB.sc_comp_code = (unsigned long)SDI_HAERR;
			break;
		case AD_ISGL:
			cmn_err(CE_WARN, "%s: Invalid Scatter/Gather list",
				 adseidata[c].name);
			sp->SCB.sc_comp_code = (unsigned long)SDI_HAERR;
			break;
		case AD_RSCF:
			cmn_err(CE_WARN, "%s: Auto-Request Sense Failed",
				 adseidata[c].name);
			sp->SCB.sc_comp_code = (unsigned long)SDI_HAERR;
			break;
		case AD_HAHE:
			cmn_err(CE_WARN, "%s: Host Adapter Hardware error",
				 adseidata[c].name);
			sp->SCB.sc_comp_code = (unsigned long)SDI_HAERR;
			break;
		case AD_TDNRTA:
			cmn_err(CE_WARN, "%s: Target did not respond to ATTN",
				 adseidata[c].name);
			sp->SCB.sc_comp_code = (unsigned long)SDI_HAERR;
			break;
		case AD_SBRBHA:
			cmn_err(CE_WARN, "%s: SCSI Bus Reset by Host Adapter",
				 adseidata[c].name);
			sp->SCB.sc_comp_code = (unsigned long)SDI_HAERR;
			break;
		case AD_SBRBOD:
			cmn_err(CE_WARN, "%s: SCSI Bus Reset by other device",
				 adseidata[c].name);
		default:
			sp->SCB.sc_comp_code = (unsigned long)SDI_HAERR;
			break;
		}
		break;

	default:
		cmn_err(CE_WARN,"!%s: Illegal status 0x%x!",adseidata[c].name, status);
		ASSERT(adse_panic_assert);
		sp->SCB.sc_comp_code = (unsigned long)SDI_ERROR;
		break;
	}
	if((stat->status_word & (STAT_DU | STAT_DO)) && 
	   (stat->residual_ptr == cp->data_ptr)) {
		sp->SCB.sc_resid = stat->residual_count;
	}
	
	stat->status_word = 0;
	stat->target_stat = 0;
	stat->ha_stat = AD_NHASA;
	cp->flag1 = 0 ;
	cp->flag2 = 0;

	/* call target driver interrupt handler */
	sdi_callback(sp);

	if(flag == SG_START) {
		ADSE_SCSILU_LOCK(q->q_opri);
		adse_next(q);
	}
} 

/**************************************************************************
** Function name: adse_int()						 **
** Description:								 **
**	This is the interrupt handler for pass-thru jobs.  It just	 **
**	wakes up the sleeping process.					 **
**************************************************************************/
STATIC void
adse_int(struct sb *sp)
{
	struct buf  *bp;

	AHA_DTRACE;

	bp = (struct buf *) sp->SCB.sc_wd;
	biodone(bp);
}

/**************************************************************************
** Function: adse_illegal()						 **
** Description: Used to verify commands					 **
**************************************************************************/
STATIC int
adse_illegal(short hba, uchar_t scsi_id, uchar_t lun)
{
	AHA_DTRACE;

	if(sdi_redt(hba, scsi_id, lun)) {
		return(0);
	} else {
		return(1);
	}
}

/**************************************************************************
** Function name: adse_func()						 **
** Description:								 **
**	Create and send an SFB associated command. 			 **
**************************************************************************/
STATIC void
adse_func(sblk_t *sp)
{
	struct scsi_ad	*sa;
	struct scsi_ha	*ha;
	struct scsi_lu	*q;
	int 	trigger, n, adapter, target;
	long	command;
	pl_t	oip;

	AHA_DTRACE;

	sa = &sp->sbp->sb.SFB.sf_dev;
	adapter = adse_gtol[SDI_HAN(sa)];
	ha = &adse_sc_ha[adapter];
	target = SDI_TCN(sa);
	q = &ADSE_LU_Q(adapter, target, 0);

	/*
	 *  Here we get an ECB to use to pass the callback pointer
	 *  along with the adapter, target, lun tuple to the intr
	 *  routine.  This ECB is hung on the LU Q for lun 0 of this target.
	 *
	 *  NOTE: only the ECB fields used in adseintr and adse_ha_immdone
	 *        are filled in.
	 */
	q->q_func_cp = adse_getblk(adapter);
	q->q_func_cp->c_bind = &sp->sbp->sb;
	q->q_func_cp->c_ctlr = adapter;
	q->q_func_cp->c_target = target;
	q->q_func_cp->flag2 = 0;

	command = ATTN_RES_DEV;
	command <<= 16;
	command |= ATTN_RESET;

	trigger = IMMED_COMMAND | target;

	ADSE_HBA_LOCK(oip);

	n = inb(ha->ha_slot | STATUS1_PORT);
	while(!(n & ST1_MBO_EMPTY))
		n = inb(ha->ha_slot | STATUS1_PORT);
	while(n & ST1_BUSY_SET)
		n = inb(ha->ha_slot | STATUS1_PORT);

	outl((ha->ha_slot | MBOX_OUT_PORT), command);

	outb((ha->ha_slot | ATTN_PORT), trigger);

	ADSE_HBA_UNLOCK(oip);

	cmn_err(CE_WARN,
	    "!%s: HA %d TC %d is being reset", adseidata[adapter].name, adapter, target);
}

/*
 * STATIC void
 * adse_pass_thru0(buf_t *bp)
 *	Calls buf_breakup to make sure the job is correctly broken up/copied.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
adse_pass_thru0(buf_t *bp)
{
	int	c = adse_gtol[SC_HAN(bp->b_edev)];
	int	t = SC_TCN(bp->b_edev);
	int	l = SC_LUN(bp->b_edev);
	struct scsi_lu	*q = &ADSE_LU_Q(c, t, l);

	AHA_DTRACE;

	if (!q->q_bcbp) {
		struct sb *sp;
		struct scsi_ad *sa;

		sp = bp->b_priv.un_ptr;
		sa = &sp->SCB.sc_dev;
		if ((q->q_bcbp = sdi_getbcb(sa, KM_SLEEP)) == NULL) {
			sp->SCB.sc_comp_code = SDI_ABORT;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return;
		}
		q->q_bcbp->bcb_addrtypes = BA_KVIRT;
		q->q_bcbp->bcb_flags = BCB_SYNCHRONOUS;
		q->q_bcbp->bcb_max_xfer = adsehba_info.max_xfer - ptob(1);
		q->q_bcbp->bcb_physreqp->phys_align = ADSE_MEMALIGN;
		q->q_bcbp->bcb_physreqp->phys_boundary = ADSE_BOUNDARY;
		q->q_bcbp->bcb_physreqp->phys_dmasize = ADSE_DMASIZE;
		q->q_bcbp->bcb_granularity = 1;
	}

	buf_breakup(adse_pass_thru, bp, q->q_bcbp);
}

/**************************************************************************
** Function name: adse_pass_thru()					 **
** Description:								 **
**	Send a pass-thru job to the HA board.				 **
**************************************************************************/
STATIC void
adse_pass_thru(struct buf *bp)
{
	int	c = adse_gtol[SC_HAN(bp->b_edev)];
	int	t = SC_TCN(bp->b_edev);
	int	l = SC_LUN(bp->b_edev);
	struct scsi_lu	*q;
	struct sb *sp;
	caddr_t *addr;
	char op;
	daddr_t blkno_adjust;

	AHA_DTRACE;

	sp = (struct sb *)bp->b_private;

	sp->SCB.sc_wd = (long)bp;
	sp->SCB.sc_datapt = (caddr_t) paddr(bp);
	sp->SCB.sc_datasz = bp->b_bcount;
	sp->SCB.sc_int = adse_int;

	SDI_TRANSLATE(sp, bp->b_flags, bp->b_proc, KM_SLEEP);

	q = &ADSE_LU_Q(c, t, l);

	/*
	 * (This is a workaround for the 2G limitation for physiock offset.)
	 * Set new block number in the SCSI command if breakup occurred
	 * by adjusting the starting block number with the adjustment 
	 * from b_blkno.  Starting block number was saved in b_priv2,
	 * and having told physiock that the offset was 0, we add
	 * the adjustment now.
	 */
	blkno_adjust = bp->b_blkno;
	op = q->q_sc_cmd[0];
	if (op == SS_READ || op == SS_WRITE) {
		struct scs *scs = (struct scs *)(void *)q->q_sc_cmd;
		daddr_t blkno;

		if (blkno_adjust) {
			blkno = bp->b_priv2.un_int + blkno_adjust;
			scs->ss_addr1 = (blkno & 0x1F0000) >> 16;
			scs->ss_addr  = sdi_swap16(blkno);
		}
		scs->ss_len   = (char)(bp->b_bcount >> ADSE_BLKSHFT);
	}
	if (op == SM_READ || op == SM_WRITE) {
		daddr_t blkno;
		struct scm *scm = (struct scm *)(void *)((char *)q->q_sc_cmd - 2);

		if (blkno_adjust) {
			blkno = bp->b_priv2.un_int + blkno_adjust;
			scm->sm_addr = sdi_swap32(blkno);
		}
		scm->sm_len  = sdi_swap16(bp->b_bcount >> ADSE_BLKSHFT);
	}

	sdi_icmd(sp, KM_SLEEP);
}

/**************************************************************************
** Function name: adse_next()						 **
** Description:								 **
**	Attempt to send the next job on the logical unit queue.		 **
**	All jobs are not sent if the Q is busy.				 **
**************************************************************************/
STATIC void
adse_next(struct scsi_lu *q)
{
	sblk_t	*sp;

	AHA_DTRACE;
	ASSERT(q);
	ASSERT(ADSE_SCSILU_IS_LOCKED);

	if (q->q_flag & QBUSY) {
		ADSE_SCSILU_UNLOCK(q->q_opri);
		return;
	}

	if((sp = q->q_first) == NULL) {	 /*  queue empty  */
		q->q_depth = 0;
		ADSE_SCSILU_UNLOCK(q->q_opri);
		return;
	}
/*
 * QBUSY is now a misnomer.  It really indicates if the queue of commands
 * has reached the lowat mark for building I/O's for a given target.  As a
 * matter of fact the lowat variable (which is set in space.c) is really the
 * high water mark for starting I/O's on a given target.  If the number of
 * commands falls below the lowat variable, then more I/O's are allowed to
 * start, until the number of I/O's running on a given target is above the
 * lowat variable.  Once the number of I/O's is above the lowat variable,
 * the commands are shoved in the queue for later work.  This is where s/g
 * (local) becomes possible, as long as the commands have been sorted in an
 * ascending order.
 */

	if (q->q_flag & QSUSP && sp->sbp->sb.sb_type == SCB_TYPE) {
		ADSE_SCSILU_UNLOCK(q->q_opri);
		return;
	}

	if(sp->sbp->sb.sb_type == SFB_TYPE) {		/* [1]  */
		if (!(q->q_first = sp->s_next))
			q->q_last = NULL;
		else
			q->q_first->s_prev = NULL;
		q->q_depth--;
		ADSE_SCSILU_UNLOCK(q->q_opri);
		adse_func(sp);				/* [1a] */
		return;
	}

	/*
	 * The test conditions to do a local scatter-gather:
	 * 1) there is more than 1 command in the queue.
	 * 2) the current command is not already a s/g command from
	 *    the page I/O stuff.
	 * 3) local scatter-gather is enabled.
	 */

	if(q->q_depth > 1 && !sp->s_dmap && adse_sg_enable == 1) {
		ASSERT(ADSE_SCSILU_IS_LOCKED);
		adse_sctgth(q);
	} else {
		if(!(q->q_first = sp->s_next))
			q->q_last = NULL;
		else
			q->q_first->s_prev = NULL;
		q->q_depth--;
		ADSE_SCSILU_UNLOCK(q->q_opri);
		adse_cmd(SGNULL , sp);
	}

	if(adseinit_time == TRUE) {		/* need to poll */
		if(adse_wait(30000) == LOC_FAILURE)  {
			cmn_err(CE_NOTE, "adse Initialization Problem - timeout with no interrupt\n");
			sp->sbp->sb.SCB.sc_comp_code = (unsigned long)SDI_TIME;
			sdi_callback(&sp->sbp->sb);
		}
	}
}

/**************************************************************************
** Function name: adse_putq()						 **
** Description:								 **
**	Put a job on a logical unit queue.  Jobs are enqueued		 **
**	on a priority basis.						 **
**************************************************************************/
STATIC void
adse_putq(struct scsi_lu *q, sblk_t *sp)
{
	int cls = ADSE_QUECLASS(sp);

	AHA_DTRACE;

	/* 
	 * If queue is empty or queue class of job is less than
	 * that of the last one on the queue, tack on to the end.
	 */
	if(!q->q_first || (cls <= ADSE_QUECLASS(q->q_last))) {
		if(q->q_first) {
			q->q_last->s_next = sp;
			sp->s_prev = q->q_last;
		} else {
			q->q_first = sp;
			sp->s_prev = NULL;
		}
		sp->s_next = NULL;
		q->q_last = sp;

	} else {
		sblk_t *nsp = q->q_first;

		while(ADSE_QUECLASS(nsp) >= cls)
			nsp = nsp->s_next;
		sp->s_next = nsp;
		sp->s_prev = nsp->s_prev;
		if(nsp->s_prev)
			nsp->s_prev->s_next = sp;
		else
			q->q_first = sp;
		nsp->s_prev = sp;
	}
	q->q_depth++;
}

/**************************************************************************
** Function name: adse_flushq()						 **
** Description:								 **
**	Empty a logical unit queue.  If flag is set, remove all jobs.	 **
**	Otherwise, remove only non-control jobs.			 **
**************************************************************************/
STATIC void
adse_flushq(struct scsi_lu *q, int  cc, int  flag)
{
	sblk_t  *sp, *nsp;

	AHA_DTRACE;
	ASSERT(q);

	ADSE_SCSILU_LOCK(q->q_opri);
	sp = q->q_first;
	q->q_first = q->q_last = NULL;
	q->q_depth = 0;
	ADSE_SCSILU_UNLOCK(q->q_opri);

	while (sp) {
		nsp = sp->s_next;
		if (!flag && (ADSE_QUECLASS(sp) > HBA_QNORM)) {
			ADSE_SCSILU_LOCK(q->q_opri);
			adse_putq(q, sp);
			ADSE_SCSILU_UNLOCK(q->q_opri);
		} else {
			sp->sbp->sb.SCB.sc_comp_code = (ulong)cc;
			sdi_callback(&sp->sbp->sb);
		}
		sp = nsp;
	}
}

/**************************************************************************
** Function name: adse_send()						 **
** Description:								 **
**	Send a command to the host adapter board.			 **
**************************************************************************/
STATIC void
adse_send(int c, int cmd, struct ecb *cp)
{
	struct scsi_ha *ha = &adse_sc_ha[c];
	int 		n;
	pl_t			oip;
	struct scsi_lu		*q;
	char			t;

	AHA_DTRACE;

	if (cmd == INIT_SCSI_COMM) {
		q = &ADSE_LU_Q(c, cp->c_target, (cp->flag2 & FLAG2_LUN_MASK));
		ADSE_SCSILU_LOCK(q->q_opri);
		q->q_outcnt++;
		if(q->q_outcnt >= adse_io_per_tar)
			q->q_flag |= QBUSY;
		ADSE_SCSILU_UNLOCK(q->q_opri);
	}

	ADSE_HBA_LOCK(oip);

	if (cmd == INIT_SCSI_COMM) {
		ha->ha_npend++;
		t = SEND_COMMAND | cp->c_target;
	} else
		t = ABORT_COMMAND | cp->c_target;

	n = inb(ha->ha_slot | STATUS1_PORT);
	while(!(n & ST1_MBO_EMPTY) && (n & ST1_BUSY_SET))
		n = inb(ha->ha_slot | STATUS1_PORT);

	/*
	 * Fill in the mailbox and inform host adapter
	 * of the new request.
	 */
	outl((ha->ha_slot | MBOX_OUT_PORT), cp->c_addr);

#ifdef NOT_YET
	if(adse_timeout_enable && cmd == INIT_SCSI_COMM)
		cp->c_tm_id = timeout(adse_timeout, (caddr_t)cp, adse_timeout_count);
#endif

	outb((ha->ha_slot | ATTN_PORT), t);

	ADSE_HBA_UNLOCK(oip);
}

/**************************************************************************
** Function name: adse_getblk()						 **
** Description:								 **
**	Allocate a controller command block structure.			 **
**************************************************************************/
STATIC struct ecb *
adse_getblk(int c)
{
	struct scsi_ha	*ha = &adse_sc_ha[c];
	struct ecb	*cp;
	pl_t		oip;
#ifdef DEBUG
	int cnt = 0;
#endif

	AHA_DTRACE;

	ADSE_ECB_LOCK(oip);
	cp = ha->ha_free;

	/*
	 * [1]	If the current ecb is not being used then (1a), advance the
	 *	counter, check to see if it needs to wrap (probably could use
	 *	x %= adse_num_ecbs but what the heck) and break out of the loop.
	 * [2]	The value of x shoud be at least one more than it was before.
	 *	Mark the ecb active(TRUE) and return the pointer.
	 * [3]	The current ecb is active, so let's increment a counter.  If
	 *	the counter is greater than than the number of mailboxes
	 *	(adse_num_ecbs), then we have searched through the entire list
	 *	of ecb's and didn't find one we could use (too bad).  Now we
	 *	have to PANIC the system.  This should really never occur unless
	 *	something has gone awry somewhere else.  But if we still have
	 *	some ecb's to look at, we advance to the next ecb (cp->c_next).
	 * [3a]	We would get here if we found a busy(TRUE) ecb, so we need to
	 *	move the x counter up one.
	 */
	for(;;) {
		if(cp->c_active == TRUE) {		/* [1] */
#ifdef DEBUG
			cnt++;				/* [3] */
#endif
			ASSERT(cnt < adse_num_ecbs);
			cp = cp->c_next;		/*  ^  */
		} else					/* [1a] */
			break;				/*  ^   */
	}
	ha->ha_free = cp->c_next;
	cp->c_active = TRUE;				/*  ^  */
	ADSE_ECB_UNLOCK(oip);
	ha->ha_poll = cp;
	return(cp);					/*  ^  */
}

/**************************************************************************
** Function name: adse_cmd()						 **
** Description:								 **
**	Create and send an SCB associated command, using a scatter	 **
**	gather list created by adse_sctgth(). 				 **
**************************************************************************/
STATIC void
adse_cmd(SGARRAY *sg_p, sblk_t *sp)
{
	struct scsi_ad *sa;
	struct ecb	*cp;
	struct scsi_lu	*q;
	int		i;
	struct stat_def		*stat;
	char			*p;
	long			bcnt, cnt;
	int			adapter, target;
	paddr_t			addr;

	AHA_DTRACE;

	if(sg_p != SGNULL)
		sp = sg_p->spcomms[0];

	sa = &sp->sbp->sb.SCB.sc_dev;
	adapter = adse_gtol[SDI_HAN(sa)];
	target = SDI_TCN(sa);

	cp = adse_getblk(adapter);

	/* fill in the controller command block */

	cp->command_word = INIT_SCSI_COMM;
	cp->c_target = target;
	cp->c_bind = &sp->sbp->sb;
	cp->flag1 = (short)(FLAG1_ARS|FLAG1_SES);
	cp->flag2 = (short)SDI_LUN(sa);
	cp->ccb_length = (uchar_t)sp->sbp->sb.SCB.sc_cmdsz;
	cp->c_ctlr = adapter;
	stat = cp->c_stat_ptr;
	if(stat->status_word)
		bzero((caddr_t)stat, sizeof(struct stat_def));
	
	/*
	 * If the sg_p pointer is not NULL or the sp->s_dmap contains a valid
	 * address, we have a s/g command to do.
	 * [1] Set the ecb opcode to a s/g type.
	 * [2] If s_dmap is valid, then set the length and addr.  If it is
	 * 	not valid, the length and addr will have been passed in the
	 *	call to adse_cmd, to indicate the driver built a local s/g
	 *	command. Setting the c_sg_p to a NULL will allow the interrupt
	 *	to recognize this command as a normal I/O or a page I/O s/g
	 *	command.
	 * [3] Must be a normal command. (shucks)
	 * [4] Set up the ecb for a normal command, set the length and addr.
	 */
	if(sp->s_dmap || sg_p != SGNULL) {
		cp->flag1 |= FLAG1_SG;			/* [1] */
		if(sp->s_dmap) {			/* [2] */
			cnt = sp->s_dmap->d_size;	/*  ^  */
			addr = sp->s_addr;		/*  ^  */
			cp->c_sg_p = SGNULL;		/*  ^  */
		} else {
			cnt = sg_p->sg_len;
			addr = sg_p->addr;
		}
	} else {					/* [3] */
		cnt = sp->sbp->sb.SCB.sc_datasz;	/* [4] */
		addr = sp->s_addr;			/*  ^  */
		cp->c_sg_p = SGNULL;
	}

	cp->data_ptr = addr;
	cp->data_length = cnt;

	/* copy SCB cdb to controller CB cdb */
 	p = sp->sbp->sb.SCB.sc_cmdpt;
	for (i = 0; i < sp->sbp->sb.SCB.sc_cmdsz; i++)
		cp->scsi_command[i] = *p++;

	/*
	 * [1]  Have to save the pointer to the s/g list, if it was built
	 *	locally so it can be released at interrupt time.
	 * [2]  Go ahead and divide by 512 to get the actual block count for the
	 *	SCSI command.  This *ASSUMES* the device we are sending the
	 *	command to is a 512 byte block device.  Should be changed to
	 *	get the block size of the device, as OPTICAL drives may not be
	 *	512 byte block sizes.  FIXIT?
	 * [3]  If the command was a locally built s/g command, we have to munge
	 *	the actual number of blocks requested in the SCSI command.
	 * [3a] I don't know if I should PANIC here or not.  If this code can be
	 *	reached and *not* be a READ or WRITE, then in adse_sctgth we
	 *	should explicitly look for READ or WRITE as part of the
	 *	determining factor whether a s/g command can be done or not.
	 */
	if(sg_p != SGNULL) {
		cp->c_sg_p = sg_p;				/* [1] */
		bcnt = sg_p->dlen >> 9;
		switch(cp->scsi_command[0]) {			/* [3] */
			case SS_READ:
				cp->scsi_command[4] = (unsigned char)bcnt;
				cp->flag2 |= (FLAG2_DAT|FLAG2_DIR);
				cp->flag1 &= ~FLAG1_SES;
				break;
			case SM_READ:
				cp->scsi_command[7] = (unsigned char)((bcnt >> 8) & 0xff);
				cp->scsi_command[8] = (unsigned char)(bcnt & 0xff);
				cp->flag2 |= (FLAG2_DAT|FLAG2_DIR);
				cp->flag1 &= ~FLAG1_SES;
				break;
			case SS_WRITE:
				cp->scsi_command[4] = (unsigned char)bcnt;
				cp->flag2 |= FLAG2_DAT;
				cp->flag2 &= ~FLAG2_DIR;
				cp->flag1 &= ~FLAG1_SES;
				break;
			case SM_WRITE:
				cp->scsi_command[7] = (unsigned char)((bcnt >> 8) & 0xff);
				cp->scsi_command[8] = (unsigned char)(bcnt & 0xff);
				cp->flag2 |= FLAG2_DAT;
				cp->flag2 &= ~FLAG2_DIR;
				cp->flag1 &= ~FLAG1_SES;
				break;
			default:				/* [3a] */
				cmn_err(CE_PANIC, "%s: Unknown SCSI command for s/g",
				    adseidata[adapter].name);
				break;
		}
	}

	q = &ADSE_LU_Q(adapter, target, SDI_LUN(sa));
	ADSE_SCSILU_LOCK(q->q_opri);
	if ((q->q_flag & QSENSE) && (cp->scsi_command[0] == SS_REQSEN)) {
		int copyfail;
		caddr_t toaddr;
		copyfail = 0;

		q->q_flag &= ~QSENSE;

		/*
		 * If the REQ SENSE data is going to a buffer within kernel
		 * space, then the data is just copied.  Otherwise, the
		 * data is going to a user-space buffer (Pass-thru) so the
		 * data must be copied to the individual segments of
		 * contiguous physical memory that make up the user's buffer.
		 * Note - The buffer address sent by the user
		 * (sp->sbp->sb.SCB.sc_datapt) can not be used directly 
		 * since this routine may be executing on the interrupt thread.
		 */
 		if((cp->flag1 & FLAG1_SG) == 0) {
			if (toaddr = physmap(sp->s_addr, cp->data_length, KM_NOSLEEP)) {
 				bcopy((caddr_t)(&q->q_sense)+1, toaddr, cp->data_length);
				physmap_free(toaddr, cp->data_length, 0);
			} else {
				copyfail++ ;
			}
 		} else {
			struct dma_vect	*VectPtr;
			int	VectCnt;
			caddr_t	Src, optr, Dest;
			int	Count;
		 
 			if (VectPtr = (struct dma_vect *)(void *)physmap(sp->s_addr, cnt, KM_NOSLEEP)) {
 				optr = (caddr_t)VectPtr;
 				VectCnt = cnt / sizeof(struct dma_vect);
 				Src = (caddr_t)(&q->q_sense) + 1;
 				for (i = 0; i < VectCnt; i++) {
 					Dest = (caddr_t)(VectPtr->d_addr);
 					Count = (int)(VectPtr->d_cnt);
	 
 					if (Dest = physmap((paddr_t)Dest, Count, KM_NOSLEEP)) {
 						bcopy (Src, Dest, Count);
 						Src += Count;
 						VectPtr++;
						physmap_free(Dest, Count, 0);
					} else {
						copyfail++;
						break;
					}
 				}
				physmap_free(optr, cnt, 0);
 			} else {
				copyfail++;
			}
 		}
		ADSE_SCSILU_UNLOCK(q->q_opri);
		adse_done(SG_START, adapter, cp, copyfail?ADSE_SDI_RETRY:INT_CCB_SUCCESS, cp->c_bind);
		FREEECB(cp);
		adse_waitflag = FALSE;
	} else {
		ADSE_SCSILU_UNLOCK(q->q_opri);
		adse_send(adapter, INIT_SCSI_COMM, cp);   /* send cmd */
	}
	return;
}

/**************************************************************************
** Function: adse_sctgth()						 **
** Description:								 **
**	This routine is responsible for building local s/g commands.  It **
**	depends on the driver being able to queue commands for any given **
**	target, as well as the adapter being able to support s/g types   **
**	of commands.							 **
**************************************************************************/
STATIC void
adse_sctgth(struct scsi_lu *q)
{
	SGARRAY	*sg_p;
	SG		*sg;
	sblk_t		*sp, *start_sp;
	char			current_command, command;
	int			segments;
	int			length;
	int			unlocked;
	long			start_block, current_block;
	long			total_count;
	caddr_t			p;

	AHA_DTRACE;

	ASSERT(ADSE_SCSILU_IS_LOCKED);
/*
 * This comment section is rather long.  I hope you don't mind how I like to
 * comment code.  If you do, tough.
 *
 * [1]	Right off the bat, see if we can get a s/g list.  If not, do the
 *	command in a normal manner.
 * [2]	Get the command to do.
 * [3]	Not real sure why this is done, but that is the way it was done in the
 *	original code.
 * [4]	Is the command a NULL?  If not, then doit, else return.
 * [5]	What type of command is it?
 * [6]	Just your normal SCSI command.
 * [7]	Must have gotten a s/g list.
 * [8]	Set the segment counter to 0.  segment is used to count the number of
 *	s/g segments we have done, as well as getting the pointer to the next
 *	s/g segment in the list.
 * [9]	While the command is not a NULL, try to build a s/g command.
 * [10]	Darn, the command is a function type of command.  Can't get a s/g
 *	list for that.
 * [11]	Get a pointer to the current segment in the s/g list of segments.
 * [12]	If segments == 0, then this is the first segment of the s/g list,
 *	so I need to setup the initial variables.  You may wonder why I don't
 *	do this outside of the while() loop.  The next step in the evolution
 *	of the code is to make it recursively look through the queued command
 *	list for more s/g stuff.
 * [13]	Have to get the SCSI command for a later compare.
 * [14]	Now we have to get the physical block address of the command.  I hope
 *	that number will be stored by sd01 in some element in the future
 *	FIXIT?
 * [15]	Got to get the total count also.  13, 14, and 15 become clear in a
 *	second.
 * [16]	Got to save the pointer to this command in two places.  One in
 *	start_sp, in case we don't get a hit and in spcomms, in case we do get
 *	a hit.
 * [17]	Shove the count and data address for this command into the first s/g
 *	segment.
 * [18]	Increment the segment variable so we can get the next s/g segment.
 * [19]	Set the length to the length of a s/g segment.
 * [20]	Point to the next command in the queue.
 * [21]	Start it again, at ([9]).
 * [22]	Okay, we have setup the first s/g segment, now the fun begins.  Get
 *	the block number and command for this command.
 * [23]	Now let's see if we have a potential s/g command.  If the command
 *	is the same as the first command, the block number + the total
 *	count = the first command block number, and the number of segments
 *	used does not exceed the s/g list >>>>we have a s/g hit!
 * [24]	Increment the total count of data by what is set in the current
 *	command.
 * [25]	Increment the length of the s/g list.
 *	NOTE: The stuff between 25 and 26 should be self explanatory.
 * [26] Darn, we did not have a s/g hit, or have hit the end of the s/g list.
 * [27]	We only get here if we have s/g hits and still have segments to use.
 *	So we need to make sure we are pointing to the next command.
 *
 * There are three conditions that will get us to this point (28).  1) We
 * have a s/g command to do.  2) There are no more commands in the queue to
 * look at.  3) We do not have a sequential access, so no s/g to do.
 *
 * [28]	Darn, no s/g hit, we'll have to do the command as a normal I/O.
 * [29]	Is it a real command?  Or am I just being paranoid?
 * [30] Oh BOY!! A s/g command to do.
 * [31]	Cleanup time.  There will always be at least one more command to do
 *	at this point unless, we are leaving due to running out of commands
 *	in the queue, so this check is not a paranoid check.
 * [32]	Make sure we are pointing to the next command before leaving, or you
 *	will be sorry.
 */

	if((sg_p = adse_get_sglist()) == SGNULL) {		/* [1] */
		sp = q->q_first;				/* [2] */
		if(!(q->q_first = sp->s_next))			/* [3] */
			q->q_last = NULL;			/*  ^  */
		else
			q->q_first->s_prev = NULL;
		if(sp != NULL) {				/* [4] */
			if (sp->sbp->sb.sb_type == SFB_TYPE) {	/* [5] */
				q->q_depth--;
				ADSE_SCSILU_UNLOCK(q->q_opri);
				adse_func(sp);
			} else {				/* [6] */
				q->q_depth--;
				ADSE_SCSILU_UNLOCK(q->q_opri);
				adse_cmd(SGNULL, sp);
			}
		} else
			ADSE_SCSILU_UNLOCK(q->q_opri);
		return;
	}							/* [7] */
	segments = 0;						/* [8] */
	start_sp = NULL;
	while((sp = q->q_first) != NULL) {			/* [9] */
		if (!(q->q_first = sp->s_next))		/* [20]*/
			q->q_last = NULL;
		else
			q->q_first->s_prev = NULL;
		if(sp->sbp->sb.sb_type == SFB_TYPE) {		/* [10]*/
			q->q_depth--;
			ADSE_SCSILU_UNLOCK(q->q_opri);
			adse_func(sp);
			if(segments == 0) {
				sg_p->sg_flags = SG_FREE;
				return;
			}
			ADSE_SCSILU_LOCK(q->q_opri);
			continue;
		}
 		p = sp->sbp->sb.SCB.sc_cmdpt;			/* [13]*/
		if((sp->sbp->sb.SCB.sc_datasz > 65535 || sp->s_dmap != NULL) ||
		   (p[0] != SS_READ && p[0] != SM_READ &&
		   p[0] != SS_WRITE && p[0] != SM_WRITE)) {
			break;
		}
		sg = &sg_p->sg_seg[segments];			/* [11]*/
		if(segments == 0) {				/* [12]*/
			command = p[0];
/*
 * This is how we get the disk block number from the stuff above me.
 */
			if(p[0] == SM_READ || p[0] == SM_WRITE)
				start_block = adse_tol(&p[2]);
			else {
				start_block = (p[1] & 0x1f);
				start_block <<= 8;
				start_block |= p[2];
				start_block <<= 8;
				start_block |= p[3];
			}
			/* [15] */
			total_count = sp->sbp->sb.SCB.sc_datasz;
			start_sp = sp;				/* [16]*/
			sg_p->spcomms[segments] = sp;
			sg->sg_size = total_count;
			sg->sg_addr = sp->s_addr;
			segments++;				/* [18] */
			length = SG_LENGTH;			/* [19] */
			continue;				/* [21]*/
		}
		current_command = p[0];				/*  ^  */
		if(p[0] == SM_READ || p[0] == SM_WRITE) {
			current_block = adse_tol(&p[2]);
		} else {
			current_block = (p[1] & 0x1f);
			current_block <<= 8;
			current_block |= p[2];
			current_block <<= 8;
			current_block |= p[3];
		}
								/* [23] */
		if(((start_block + (total_count >> 9)) == current_block) &&
		  command == current_command &&
		  segments < SG_SEGMENTS) {
			total_count += sp->sbp->sb.SCB.sc_datasz;   /* [24]*/
			length += SG_LENGTH;			/* [25] */
			sg_p->spcomms[segments] = sp;
			sg->sg_size = sp->sbp->sb.SCB.sc_datasz;
			sg->sg_addr = sp->s_addr;
			segments++;
		} else {
			break;					/* [26] */
		}
	}
	unlocked = 0;
	if(segments <= 1) {					/* [28] */
		sg_p->sg_flags = SG_FREE;
		if(start_sp != NULL)  {				/* [29] */
			q->q_depth--;
			ADSE_SCSILU_UNLOCK(q->q_opri);
			unlocked = 1;
			adse_cmd(SGNULL, start_sp);
		}
	} else {						/* [30] */
		sg_p->sg_len = length;
		sg_p->dlen = total_count;
		sg_p->addr = vtop((caddr_t)&sg_p->sg_seg[0].sg_addr, NULL);
		q->q_depth -= segments;
		ADSE_SCSILU_UNLOCK(q->q_opri);
		unlocked = 1;
		adse_cmd(sg_p, ((sblk_t *)0));
	}
	if(sp != NULL) {					/* [31] */
		if ( unlocked )
			ADSE_SCSILU_LOCK(q->q_opri);
		q->q_depth--;
		ADSE_SCSILU_UNLOCK(q->q_opri);
		adse_cmd(SGNULL, sp);
	}
}

/**************************************************************************
** Function: adse_get_sglist()						 **
** Description:								 **
**	Here is the routine that returns a pointer to the next free	 **
**	s/g list.  Each list is 17 segments long.  I have only allocated **
**	adse_num_ecb of these structures, instead of doing it on a per	 **
**	adapter basis.  Doesn't hurt if it returns a NULL as it just	 **
**	means the calling routine will just have to do it the		 **
**	old-fashion way.  This routine runs just like adse_getblk, which **
**	is commented very well.  So see those comments if you need to.	 **
**************************************************************************/
STATIC SGARRAY *
adse_get_sglist(void)
{
	SGARRAY	*sg;
	int		cnt;
	pl_t		oip;

	AHA_DTRACE;

	ADSE_SGARRAY_LOCK(oip);
	sg = adse_sg_next;
	cnt = 0;

	for(;;) {
		if(sg->sg_flags == SG_BUSY) {
			cnt++;
			if(cnt >= adse_nsg_lists) {
				ADSE_SGARRAY_UNLOCK(oip);
				return(SGNULL);
			}
			sg = sg->sg_next;
		} else
			break;
	}
	sg->sg_flags = SG_BUSY;
	adse_sg_next = sg->sg_next;
	ADSE_SGARRAY_UNLOCK(oip);
	return(sg);
}

/*
 * Function Name: adse_wait()
 * Description:
 *	Poll for a completion from the host adapter.  If an interrupt
 *	is seen, the HA's interrupt service routine is manually called.
 *  NOTE:
 *	This routine allows for no concurrency and as such, should
 *	be used selectively.
 */
STATIC int
adse_wait(int time)
{

	AHA_DTRACE;

	while (time > 0) {
		if (adse_waitflag == FALSE) {
			adse_waitflag = TRUE;
			return (LOC_SUCCESS);
		}
		drv_usecwait(1000);  /* wait 1 msec */
		time--;
	}
	return(LOC_FAILURE);	
}

/**************************************************************************
** Function name: adsefreeblk()						 **
** Description:								 **
**	Release previously allocated SB structure. If a scatter/gather	 **
**	list is associated with the SB, it is freed via			 **
**	adse_dma_freelist().						 **
**	A nonzero return indicates an error in pointer or type field.	 **
**************************************************************************/
long
HBAFREEBLK(struct hbadata *hbap)
{
	sblk_t *sp = (sblk_t *) hbap;

	AHA_DTRACE;

	if(sp->s_dmap) {
		adse_dma_freelist(sp->s_dmap);
		sp->s_dmap = NULL;
	}
	sdi_free(&sm_poolhead, (jpool_t *)sp);
	return (SDI_RET_OK);
}

/**************************************************************************
** Function name: adsegetblk()						 **
** Description:								 **
**	Allocate a SB structure for the caller.  The function will	 **
**	sleep if there are no SCSI blocks available.			 **
**************************************************************************/
struct hbadata *
HBAGETBLK(int sleepflag)
{
	sblk_t	*sp;

	AHA_DTRACE;

	sp = (sblk_t *)SDI_GET(&sm_poolhead, sleepflag);
	return((struct hbadata *)sp);
}

/**************************************************************************
** Function name: adsegetinfo()						 **
** Description:								 **
**	Return the name, etc. of the given device.  The name is copied	 **
**	into a string pointed to by the first field of the getinfo	 **
**	structure.							 **
**************************************************************************/
void
HBAGETINFO( struct scsi_ad *sa, struct hbagetinfo *getinfop)
{
	char  *s1, *s2;
	static char temp[] = "HA X TC X";

	AHA_DTRACE;

	s1 = temp;
	s2 = getinfop->name;
	temp[3] = SDI_HAN(sa) + '0';
	temp[8] = SDI_TCN(sa) + '0';

	while ((*s2++ = *s1++) != '\0')
		;

	getinfop->iotype = F_DMA_32 | F_SCGTH | F_RESID;

	if (getinfop->bcbp) {
		getinfop->bcbp->bcb_addrtypes = BA_KVIRT;
		getinfop->bcbp->bcb_flags = 0;
		getinfop->bcbp->bcb_max_xfer = adsehba_info.max_xfer;
		getinfop->bcbp->bcb_physreqp->phys_align = ADSE_MEMALIGN;
		getinfop->bcbp->bcb_physreqp->phys_boundary = ADSE_BOUNDARY;
		getinfop->bcbp->bcb_physreqp->phys_dmasize = ADSE_DMASIZE;
	}
}

/*
 * Function name: adseicmd()
 * Description:
 *	Send an immediate command.  If the logical unit is busy, the job
 *	will be queued until the unit is free.  SFB operations will take
 *	priority over SCB operations.
 */

/*ARGSUSED*/
long
HBAICMD(struct  hbadata *hbap, int sleepflag)
{
	struct scsi_ad *sa;
	struct scsi_lu *q;
	sblk_t *sp = (sblk_t *) hbap;
	int	c, t, l;
	struct scs	     *inq_cdb;
	struct sdi_edt	     *edtp;

	AHA_DTRACE;

	switch (sp->sbp->sb.sb_type) {
	case SFB_TYPE:
		sa = &sp->sbp->sb.SFB.sf_dev;
		c = adse_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &ADSE_LU_Q(c, t, l);

		sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;

		switch (sp->sbp->sb.SFB.sf_func) {
		case SFB_RESUME:
			ADSE_SCSILU_LOCK(q->q_opri);
			q->q_flag &= ~QSUSP;
			adse_next(q);
			break;
		case SFB_SUSPEND:
			ADSE_SET_QFLAG(q,QSUSP);
			break;
		case SFB_RESETM:
			edtp = sdi_redt(SDI_HAN(sa),t,l);
			if(edtp != NULL && edtp->pdtype == ID_TAPE) {
				/*  this is a NOP for tape devices */
				sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;
				break;
			}
			/*FALLTHRU*/
		case SFB_ABORTM:
			sp->sbp->sb.SFB.sf_comp_code = SDI_PROGRES;
			ADSE_SCSILU_LOCK(q->q_opri);
			adse_putq(q, sp);
			adse_next(q);
			return (SDI_RET_OK);
		case SFB_FLUSHR:
			adse_flushq(q, SDI_QFLUSH, 0);
			break;
		case SFB_NOPF:
			break;
		default:
			sp->sbp->sb.SFB.sf_comp_code = (unsigned long)SDI_SFBERR;
		}

		sdi_callback(&sp->sbp->sb);
		return (SDI_RET_OK);

	case ISCB_TYPE:
		sa = &sp->sbp->sb.SCB.sc_dev;
		c = adse_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &ADSE_LU_Q(c, t, l);
		inq_cdb = (struct scs *)(void *)sp->sbp->sb.SCB.sc_cmdpt;
		if ((t == adse_sc_ha[c].ha_id) && (l == 0) && (inq_cdb->ss_op == SS_INQUIR)) {
			struct ident inq;
			struct ident *inq_data;
			int inq_len;

			bzero(&inq, sizeof(struct ident));
			(void)strncpy(inq.id_vendor, adseidata[c].name, 
				VID_LEN+PID_LEN+REV_LEN);
			inq.id_type = ID_PROCESOR;

			inq_data = (struct ident *)(void *)sp->sbp->sb.SCB.sc_datapt;
			inq_len = sp->sbp->sb.SCB.sc_datasz;
			bcopy((char *)&inq, (char *)inq_data, inq_len);
			sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
			sdi_callback(&sp->sbp->sb);
			return (SDI_RET_OK);
		}

		sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
		sp->sbp->sb.SCB.sc_status = 0;

		ADSE_SCSILU_LOCK(q->q_opri);
		adse_putq(q, sp);
		adse_next(q);
		return (SDI_RET_OK);

	default:
		sdi_callback(&sp->sbp->sb);
		return (SDI_RET_ERR);
	}
}

/**************************************************************************
** Function name: adseioctl()						 **
** Description:								 **
**	Driver ioctl() entry point.  Used to implement the following 	 **
**	special functions:						 **
**									 **
**	SDI_SEND     -	Send a user supplied SCSI Control Block to	 **
**			the specified device.				 **
**	B_GETTYPE    -  Get bus type and driver name			 **
**	B_HA_CNT     -	Get number of HA boards configured		 **
**	REDT	     -	Read the extended EDT from RAM memory		 **
**	SDI_BRESET   -	Reset the specified SCSI bus 			 **
**									 **
**************************************************************************/

/*ARGSUSED*/
int
HBAIOCTL(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *cred_p, int *rval_p)
{
	int	c = adse_gtol[SC_HAN(dev)];
	int	t = SC_TCN(dev);
	int	l = SC_LUN(dev);
	struct sb *sp;
	pl_t  oip;
	int  uerror = 0;

	AHA_DTRACE;

	switch(cmd) {
	case SDI_SEND: {
		buf_t *bp;
		struct sb  karg;
		int  rw;
		char *save_priv;
		struct scsi_lu *q;

		if (t == adse_sc_ha[c].ha_id) { 	/* illegal ID */
			return(ENXIO);
		}
		if (copyin(arg, (caddr_t)&karg, sizeof(struct sb))) {
			return(EFAULT);
		}
		if ((karg.sb_type != ISCB_TYPE) ||
		    (karg.SCB.sc_cmdsz <= 0 )   ||
		    (karg.SCB.sc_cmdsz > MAX_CMDSZ )) { 
			return(EINVAL);
		}

		sp = SDI_GETBLK(KM_SLEEP);
		save_priv = sp->SCB.sc_priv;
		bcopy((caddr_t)&karg, (caddr_t)sp, sizeof(struct sb));

		bp = getrbuf(KM_SLEEP);
		bp->b_iodone = NULL;
		sp->SCB.sc_priv = save_priv;

		q = &ADSE_LU_Q(c, t, l);
		if (q->q_sc_cmd == NULL) {
			/*
			 * Allocate space for the SCSI command and add 
			 * sizeof(int) to account for the scm structure
			 * padding, since this pointer may be adjusted -2 bytes
			 * and cast to struct scm.  After the adjustment we
			 * still want to be pointing within our allocated space!
			 */
			q->q_sc_cmd = (char *)KMEM_ZALLOC(MAX_CMDSZ + 
					sizeof(int), KM_SLEEP) + sizeof(int);
		}
		sp->SCB.sc_cmdpt = q->q_sc_cmd;

		if (copyin(karg.SCB.sc_cmdpt, sp->SCB.sc_cmdpt,
		    sp->SCB.sc_cmdsz)) {
			uerror = EFAULT;
			goto done;
		}
		sp->SCB.sc_dev.sa_lun = (uchar_t)l;
		sp->SCB.sc_dev.sa_fill = (adse_ltog[c] << 3) | t;

		rw = (sp->SCB.sc_mode & SCB_READ) ? B_READ : B_WRITE;
		bp->b_private = (caddr_t)sp;

		/*
		 * If the job involves a data transfer then the
		 * request is done thru physiock() so that the user
		 * data area is locked in memory. If the job doesn't
		 * involve any data transfer then adse_pass_thru()
		 * is called directly.
		 */
		if (sp->SCB.sc_datasz > 0) { 
			struct iovec  ha_iov;
			struct uio    ha_uio;
			char op;

			ha_iov.iov_base = sp->SCB.sc_datapt;	
			ha_iov.iov_len = sp->SCB.sc_datasz;	
			ha_uio.uio_iov = &ha_iov;
			ha_uio.uio_iovcnt = 1;
			ha_uio.uio_offset = 0;
			ha_uio.uio_segflg = UIO_USERSPACE;
			ha_uio.uio_fmode = 0;
			ha_uio.uio_resid = sp->SCB.sc_datasz;
			op = q->q_sc_cmd[0];
			/*
			 * Save the starting block number, if r/w.
			 */
			if (op == SS_READ || op == SS_WRITE) {
				struct scs *scs = 
				   (struct scs *)(void *)q->q_sc_cmd;
				bp->b_priv2.un_int =
				   ((sdi_swap16(scs->ss_addr) & 0xffff)
				   + (scs->ss_addr1 << 16));
			}
			if (op == SM_READ || op == SM_WRITE) {
				struct scm *scm = 
				(struct scm *)(void *)((char *)q->q_sc_cmd - 2);
				bp->b_priv2.un_int =
				   sdi_swap32(scm->sm_addr);
			}

			if (uerror = physiock(adse_pass_thru0, bp, dev, rw, 
			    HBA_MAX_PHYSIOCK, &ha_uio)) {
				goto done;
			}
		} else {
			bp->b_un.b_addr = NULL;
			bp->b_bcount = 0;
			bp->b_blkno = 0;
			bp->b_edev = dev;
			bp->b_flags |= rw;

			adse_pass_thru(bp);  /* Bypass physiock call */
			biowait(bp);
		}

		/* update user SCB fields */

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
		if (copyout((caddr_t)&adse_sdi_ver,arg, sizeof(struct ver_no)))
			return(EFAULT);
		break;
	case SDI_BRESET: {
		struct scsi_ha *ha = &adse_sc_ha[c];

		ADSE_HBA_LOCK(oip);
		if (ha->ha_npend > 0) {     /* jobs are outstanding */
			ADSE_HBA_UNLOCK(oip);
			return(EBUSY);
		} else {
			cmn_err(CE_WARN,
				"!%s: HA %d - Bus is being reset",
				adseidata[c].name, adse_ltog[c]);
			outb((ha->ha_slot | CONTROL_PORT), HARD_RESET);
		}
		ADSE_HBA_UNLOCK(oip);
		break;
		}

	default:
		return(EINVAL);
	}
	return(uerror);
}

/*
 * Function name: adsesend()
 * Description:
 * 	Send a SCSI command to a controller.  Commands sent via this
 *	function are executed in the order they are received.
 */

/*ARGSUSED*/
long
HBASEND(struct hbadata *hbap, int sleepflag)
{
	struct scsi_ad *sa;
	struct scsi_lu *q;
	sblk_t *sp = (sblk_t *) hbap;
	int	c, t, l;

	AHA_DTRACE;

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = adse_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);

	if (sp->sbp->sb.sb_type != SCB_TYPE) {
		return (SDI_RET_ERR);
	}

	q = &ADSE_LU_Q(c, t, l);

	ADSE_SCSILU_LOCK(q->q_opri);
	if (q->q_flag & QPTHRU) {
		ADSE_SCSILU_UNLOCK(q->q_opri);
		return (SDI_RET_RETRY);
	}

	sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
	sp->sbp->sb.SCB.sc_status = 0;

	adse_putq(q, sp);
	adse_next(q);

	return (SDI_RET_OK);
}

/*
 * Function name: adse_ha_immdone()
 * Description:
 *	This is the interrupt handler routine for SCSI jobs which caused
 *	an immediate status in the interrupt register, as the result of
 *	an IMMEDIATE command sent to the adapter.
 */
STATIC void
adse_ha_immdone(int adapter, struct ecb *cp, int status)
{
	struct scsi_ha		*ha = &adse_sc_ha[adapter];
	struct sb	*sp;
	struct scsi_lu		*q;
	char			target;
	int			imm_stat;

	AHA_DTRACE;

	sp = cp->c_bind;
	target = status & INT_TAR_MASK;

	/* Determine completion status of the job */
	if((status & ~INT_TAR_MASK) == INT_IMM_ERROR) {
		imm_stat = inb(ha->ha_slot | MBOX_IN_PORT);
		switch (imm_stat) {
			case AD_FND:
				cmn_err(CE_PANIC, "%s: No firmware loaded in the adapter",
				    adseidata[adapter].name);
				break;
			case AD_ST:
				cmn_err(CE_WARN, "%s: Selection timeout during reset",
				    adseidata[adapter].name);
				sp->SCB.sc_comp_code = (unsigned long)SDI_HAERR;
				break;
			case AD_HAHE:
				cmn_err(CE_PANIC, "%s: Host adapter hardware failure",
				    adseidata[adapter].name);
				break;
			case AD_TDNRTA:
				cmn_err(CE_PANIC, "%s: Target %d did not respond to reset. SCSI BUS RESET!",
				    adseidata[adapter].name, target);
				break;
			default:
				cmn_err(CE_WARN,"!%s: Illegal status 0x%x!",
				    adseidata[adapter].name, imm_stat);
				sp->SCB.sc_comp_code = (unsigned long)SDI_ERROR;
				break;
		}
	} else {
		sp->SCB.sc_comp_code = (unsigned long)SDI_ASW;
	}
	FREEECB(cp);
	sdi_callback(sp);

	q = &ADSE_LU_Q(adapter, target, 0);
	ADSE_SCSILU_LOCK(q->q_opri);
	adse_next(q);
}

/*
 * STATIC void
 * adse_lockinit(int c)
 *	Initialize adse locks:
 *		1) device queue locks (one per queue),
 *		2) dmalists lock (one only),
 *		3) ecb lock (one only),
 *		4) scatter/gather lists lock (one only),
 *		5) and controller mailbox queue lock (one per controller).
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
adse_lockinit(int c)
{
	struct scsi_ha  *ha;
	struct scsi_lu	 *q;
	int	t,l;
	int		sleepflag;
	static		firsttime = 1;

	AHA_DTRACE;

	sleepflag = adse_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;

	if (firsttime) {

		adse_dmalist_sv = SV_ALLOC(sleepflag);
		adse_dmalist_lock = LOCK_ALLOC(ADSE_HIER, pldisk, &adse_lkinfo_dmalist, sleepflag);

		adse_sgarray_lock = LOCK_ALLOC(ADSE_HIER+1, pldisk, &adse_lkinfo_sgarray, sleepflag);

		adse_ecb_lock = LOCK_ALLOC(ADSE_HIER, pldisk, &adse_lkinfo_ecb, sleepflag);
		firsttime = 0;
	}

	ha = &adse_sc_ha[c];
	ha->ha_hba_lock = LOCK_ALLOC(ADSE_HIER, pldisk, &adse_lkinfo_hba, sleepflag);
	for (t = 0; t < MAX_TCS; t++) {
		for (l = 0; l < MAX_LUS; l++) {
			q = &ADSE_LU_Q(c,t,l);
			q->q_lock = LOCK_ALLOC(ADSE_HIER, pldisk, &adse_lkinfo_q, sleepflag);
		}
	}
}

/*
 * STATIC void
 * adse_lockclean(int c)
 *	Removes unneeded locks.  Controllers that are not active will
 *	have all locks removed.  Active controllers will have locks for
 *	all non-existant devices removed.
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
adse_lockclean(int c)
{
	struct scsi_ha  *ha;
	struct scsi_lu	 *q;
	int	t,l;
	static		firsttime = 1;

	AHA_DTRACE;

	if (firsttime && !adse_hacnt) {

		if (adse_dmalist_sv == NULL)
			return;
		SV_DEALLOC(adse_dmalist_sv);
		LOCK_DEALLOC(adse_dmalist_lock);
		LOCK_DEALLOC(adse_sgarray_lock);
		LOCK_DEALLOC(adse_ecb_lock);
		firsttime = 0;
	}

	if( !adseidata[c].active) {
		ha = &adse_sc_ha[c];
		if (ha->ha_hba_lock == NULL)
			return;
		LOCK_DEALLOC(ha->ha_hba_lock);
	}

	for (t = 0; t < MAX_TCS; t++) {
		for (l = 0; l < MAX_LUS; l++) {
			if (!adseidata[c].active || adse_illegal(adse_ltog[c], t, l)) {
				q = &ADSE_LU_Q(c,t,l);
				LOCK_DEALLOC(q->q_lock);
			}
		}
	}
}

/*
 * STATIC void *
 * adse_kmem_zalloc_physreq(size_t size, int flags)
 *	function to be used in place of kma_zalloc
 *	which allocates contiguous memory, using kmem_alloc_physreq,
 *	and zero's the memory.
 *
 * Calling/Exit State:
 *	None
 */
STATIC void *
adse_kmem_zalloc_physreq(size_t size, int flags)
{
	void *mem;
	physreq_t *preq;

	AHA_DTRACE;

	preq = physreq_alloc(flags);
	if (preq == NULL)
		return NULL;
	preq->phys_align = ADSE_MEMALIGN;
	preq->phys_boundary = ADSE_BOUNDARY;
	preq->phys_dmasize = ADSE_DMASIZE;
	preq->phys_flags |= PREQ_PHYSCONTIG;
	if (!physreq_prep(preq, flags)) {
		physreq_free(preq);
		return NULL;
	}
	mem = kmem_alloc_physreq(size, preq, flags);
	physreq_free(preq);
	if (mem)
		bzero(mem, size);

	return mem;
}
