#ident	"@(#)kern-pdi:io/hba/adsa/adsa.c	1.42.6.1"

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
 *	SCSI Host Adapter Driver for Adaptec AIC-7770			   *
 *	Base Code #: 556125-00  (Group Code #: 556124-00)		   *
 *									   *
 ***************************************************************************
 *	V1.01 changes RAN 11/06/92					   *
 *	- added the new frozen Alpha HIM code 11/04/92			   *
 *	- changed the adsa_wait routine to use the news HIM poll function  *
 *	- removed the poll code from the adsaintr function		   *
 *	- added code for init time to disable the BIOS			   *
 *	V1.02 changes RAN 11/09/92					   *
 *	- corrected the poll code to check for both interrupts set	   *
 *	V1.1 BETA starts 11/10/92					   *
 *	- took out the code that set the request sense length every time   *
 *	  as it is set in the adsa_init_scbs function and does not get	   *
 *	  altered,...ever.						   *
 *	- put a check in to see of the request sense data was valid	   *
 *	  before clearing the structure.  This keeps the code from clearing*
 *	  the structure when it isn't really needed.			   *
 *	V1.3 BETA 12/11/92						   *
 *	- fixed the adsa_sgdone routine.  A check to set SG_STAR was being *
 *	  done incorrectly.  Could cause a system hang under certain	   *
 *	  conditions.							   *
 *	V1.4 BETA 01/18/93						   *
 *	- incorporated him12 into the driver source			   *
 *	- sent to Unisys 01/20/93					   *
 *	V1.5 BETA 01/25/93						   *
 *	- added dual channel support					   *
 *	- re-wrote the initialization code to handle dual channel mode	   *
 *	- changed the startup message to reflect what channel is being	   *
 *	  used.  This simplistic statement cannot reflect the amount of    *
 *	  work involved with this change.				   *
 *	- fixed a problem with multiple physical adapters.  The init code  *
 *	  put all the SCSI ha structs at one address.  They needed to be   *
 *	  kept track of on a per host adapter basis.			   *
 *	- incorporated factory sent HIM code.  HIM code is not frozen as of*
 *	  this release date.						   *
 *	- released to the factory on 02/05/93 				   *
 *	V1.6 BETA 02/12/93						   *
 *	- added code to the initialization funtion to properly release	   *
 *	  all allocated memory when an adapter is not found.		   *
 *	- added code to track all resources for any given adapter in the   *
 *	  ha_struct.							   *
 *	- sent code to USL to check on dual channel support and see if	   *
 *	  they have any problems with what I did.			   *
 *	V1.7 BETA 03/05/93						   *
 *	- incorporated new HIM code from mchan.				   *
 *	- changed HARD_RST_DEV to SOFT_RST_DEV as chuck says its okay now  *
 ***************************************************************************
 * This marks the start of the release drivers.  Version numbers start over*
 ***************************************************************************
 *	V1.0 PILOT 03/08/93						   *
 *	- no changes from V1.7 BETA except the revision number as per mchan*
 *	V1.1 04/09/93							   *
 *	- fixed a bug where the SCB was being released before the system   *
 *	  was actually done with during scatter/gather commands		   *
 *	V1.2 04/13/93							   *
 *	- fixed a potential bug where the system could run out of SCB's    *
 *	  this was reported from Unisys and an IHVHBA disk was sent to	   *
 *	  Unisys							   *
 *	- not sent to the factory yet					   *
 *	V1.3 04/16/93							   *
 *	- defined the INTR_ON and INTR_OFF routines as NULL as they were   *
 *	  just calling NULL functions anyway.  Reduces the overhead for the*
 *	  driver.							   *
 *	- took out the F_DMA setting as this tells the kernel to double    *
 *	  buffer memory I/O requests above 16MB.  I was informed about     *
 *	  this from USL.						   *
 *	- altered the way the code set the -1 flag before calling	   *
 *	  sdi_gethbano.  Now it always sets it to a -1 unless the value was*
 *	  a 0.  This should allow better co-existence with other adapters? *
 *	- sent code to factory and USL 04/19/93				   *
 *	- sent IHVHBA floppy to Unisys					   *
 *	V1.4 05/03/93							   *
 *	- altered HIM code int_handler to not do an INPUT on the same	   *
 *	  register, but to do it once and store it in a variable.	   *
 *	  reduces overhead.						   *
 *	- added code to check to see if bit 7 is set in the variable	   *
 *	  returned from scb_findha.  If it is set I print a messsage	   *
 *	  stating the adapter is not configured into the system and	   *
 *	  then continue searching					   *
 *	- changed the SOFT_RST_DEV to a HARD_RST_DEV for target resets as  *
 *	  the SOFT_RST_DEV does nothing useful expect clear all commands   *
 *	  from the queue for the target.  I'm not sure what may happen if  *
 *	  there are commands in the Arrow queue, but it looks like this    *
 *	  function will clear the commands without doing a callback to the *
 *	  interrupt handler.  This would be very bad.  I need to check on  *
 *	  this one.  Also I am not clear if this command will generate an  *
 *	  interrupt.  It better or I am in deep trouble again.		   *
 *	V1.5 05/19/93							   *
 *	- new HIM code incorporated					   *
 *	- added code to detect which channel was booted from, basically    *
 *	  reversing the meanings of PRI and SEC channels		   *
 *	- delivered to the factory on 5/19/93				   *
 *	V1.5B 06/01/93							   *
 *	- corrected a bug in the poll routine which set the channel	   *
 *	  incorrectly.							   *
 *	- corrected a bug which did not allow the proper reporting of	   *
 *	  request sense data.  For some reason the virtual pointer and the *
 *	  physical pointer derived from the virtual pointer cannot be used *
 *	  assuming they point to the same area of memory.		   *
 *	- sent to UNISYS 06/01/93					   *
 *	V1.5C 06/02/93							   *
 *	- removed the SCB_ReqS virtual pointer from the scb structure	   *
 *	  as it was not needed anymore					   *
 *	- commented out some subroutines in the HIM code that were not	   *
 *	  being used.							   *
 *	- added code to free a SCB during the poll of the SCSI bus if there*
 *	  is a timeout.  This should never happen if the hardware is	   *
 *	  working properly.						   *
 *	- changed the default for the threshold from 50% to 100%	   *
 *	- sent to UNISYS 06/02/93					   *
 *	V1.5D 06/02/93							   *
 *	- incorporated new HIM code (v1.1beta2)				   *
 *	- reset the default threshold back to 50%, seems the Unisys parts  *
 *	  don't have the FIFO control fix (maybe?)			   *
 *	- sent to UNISYS 06/02/93					   *
 *	V1.5E 06/03/93							   *
 *	- backed in the previous HIM code				   *
 *	- commented out changes for the multi-channel boot to see if this  *
 *	  is causing a problem for Unisys				   *
 *	- sent to UNISYS 06/03/93					   *
 *	V1.5F 06/03/93							   *
 *	- installed the new HIM code (v1.1beta2)			   *
 *	- sent to UNISYS 06/03/93					   *
 *	V1.5G 06/03/93							   *
 *	- set the threshold level from 1 (50%) to 3 (100%)		   *
 *	- sent to UNISYS 06/03/93					   *
 *	V1.5H 06/04/93							   *
 *	- installed the new HIM code (v1.1beta3)			   *
 *	- added timeout code in the HIM int_handler routine while waiting  *
 *	  for PAUSE to become true.					   *
 *	- set the threshold level back to 1 (50%)			   *
 *	- sent to UNISYS 06/04/93					   *
 *	V1.6 06/07/93							   *
 *	- incorporated new him code (v1.1beta4)				   *
 *	- left the adsa_reinit bit set to force AIC-7770 reinit		   *
 *	- sent to the factory 06/07/93	for v1.1 release		   *
 *	V1.7 06/14/93							   *
 *  	- incorporated new him code 					   *
 *	- build in the factory at 06/14/93 for v1.11 release		   *
 *	V1.8 06/30/93							   *
 *	- rearranged the init code to use entries for all channels found   *
 *	  to eliminate the previously kludgy way the channels were added to*
 *	  the system.							   *
 *	- put in code to detect whether or not an adapter is configured    *
 *	  into the kernel.  previously an adapter in the system, but not   *
 *	  configured into the kernel would cause a PANIC at boot time	   *
 *	- added a line to the end of the System file to allow the above fix*
 *	  to work as USL does not insert an EOF mark at the end of the file*
 *	  when it is built into the kernel				   *
 *	- added code to allow the user to skip the addition of an empty	   *
 *	  channel B.  previoulsy this could not be done.		   *
 *	- removed some settings to the mem_release variable in the init    *
 *	  code which would have caused a PANIC as the memory had not been  *
 *	  allocated before calling to release it.			   *
 *	- corrected code for the channel swap feature.  the previous code  *
 *	  only worked if the BIOS was enabled.  If the BIOS was disabled   *
 *	  the system would either hang or PANIC.			   *
 *	- delivered to the factory on 6/30/93				   *
 *	V1.8a 12/12/93 by Novell
 *	- brought driver up-to-date with latest Novell changes vis-a-vis   *
 *	  changes made to the adsa driver since delta 1.38
 *	V1.8b 01/13/94 by Novell
 *	- Multi-threaded driver.                                           *
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
#include <io/hba/adsa/him_code/him_equ.h>
#include <io/hba/adsa/him_code/him_scb.h>
#include <io/hba/adsa/him_code/new_names.h>
#include <io/hba/adsa/adsa.h>
#include <io/autoconf/resmgr/resmgr.h>

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
#include "him_code/him_equ.h"
#include "him_code/him_scb.h"
#include "adsa.h"

/* These must come last: */
#include <sys/hba.h>
#include <sys/ddi.h>
#include <sys/ddi_i386at.h>

#endif			/* _KERNEL_HEADERS */

#define ADSA_BLKSHFT	  9	/* PLEASE NOTE:  Currently pass-thru	    */
#define ADSA_BLKSIZE	512	/* SS_READ/SS_WRITE, SM_READ/SM_WRITE	    */
				/* supports only 512 byte blocksize devices */

#define KMEM_ZALLOC adsa_kmem_zalloc_physreq
#define KMEM_FREE kmem_free

#define ADSA_MEMALIGN	512
#define ADSA_BOUNDARY	0
#define ADSA_DMASIZE	32

#define	KMEM_FREE_COND(A, B)  if (A) KMEM_FREE(A, B)

STATIC void *adsa_kmem_zalloc_physreq(size_t , int);
STATIC void	adsa_pass_thru0(buf_t *);

STATIC struct sequencer_ctrl_block *adsa_getblk(int);

STATIC int		adsa_dmalist(sblk_t *, struct proc *, int),
		adsa_hainit(struct scsi_ha *, int, int),
		adsa_illegal(short , int, uchar_t , uchar_t),
		adsa_alloc_sg(struct scsi_ha *),
		adsa_wait(int);

STATIC void		adsa_flushq(struct scsi_lu *, int, int),
		adsa_pass_thru(struct buf *),
		adsa_init_scbs(struct scsi_ha *, struct req_sense_def *),
		adsa_int(struct sb *),
		adsa_putq(struct scsi_lu *, sblk_t *),
		adsa_next(struct scsi_lu *),
		adsa_func(sblk_t *),
		adsa_lockclean(int),
		adsa_lockinit(int),
		adsa_send(int, struct sequencer_ctrl_block *),
		adsa_cmd(SGARRAY *, sblk_t *),
		adsa_done(int, int, struct sequencer_ctrl_block *, int, struct sb *, struct scsi_lu *),
		adsa_ha_done(struct scsi_lu *, struct sequencer_ctrl_block *, int),
		adsa_sgdone(int, struct sequencer_ctrl_block *, int, struct scsi_lu *),
		adsa_sctgth(struct scsi_lu *, int),
		adsa_switch(int),
		adsa_mem_release(struct scsi_ha **, int);

STATIC long		adsa_tol();

STATIC SGARRAY		*adsa_get_sglist(int);

STATIC boolean_t	adsainit_time;	    /* Init time (poll) flag	*/
STATIC boolean_t	adsa_waitflag = TRUE;	/* Init time wait flag, TRUE	*/
					/* until interrupt serviced	*/
STATIC int  		adsa_num_scbs,
			adsa_habuscnt,
			adsa_dma_listsize,
			adsa_req_structsize,
			adsa_ha_size,
			adsa_luq_structsize,
			adsa_scb_structsize,
			adsa_ha_configsize,
			adsa_him_structsize;
STATIC struct him_data_block	*adsa_ha_struct;
STATIC int  		adsa_mod_dynamic = 0;
STATIC adsa_page_t		*adsa_dmalistp;	    /* pointer to DMA list pool	*/
STATIC dma_t		*adsa_dfreelistp;    /* List of free DMA lists	*/
STATIC struct scsi_ha	*adsa_sc_ha[SDI_MAX_HBAS * ADSA_MAX_BPC];
					     /* SCSI bus structs */
STATIC int		adsa_ctob[SDI_MAX_HBAS];  /* Controller number to */
						  /* bus number */

STATIC sv_t   *adsa_dmalist_sv;	/* sleep argument for UP	*/
STATIC int	adsa_devflag = HBA_MP;
STATIC lock_t *adsa_dmalist_lock;
STATIC LKINFO_DECL(adsa_lkinfo_dmalist, "IO:adsa:adsa_dmalist_lock", 0);
STATIC LKINFO_DECL(adsa_lkinfo_HIM, "IO:adsa:ha->ha_HIM_lock", 0);
STATIC LKINFO_DECL(adsa_lkinfo_sgarray, "IO:adsa:ha->ha_sg_lock", 0);
STATIC LKINFO_DECL(adsa_lkinfo_scb, "IO:adsa:ha->ha_scb_lock", 0);
STATIC LKINFO_DECL(adsa_lkinfo_npend, "IO:adsa:ha->ha_npend_lock", 0);
STATIC LKINFO_DECL(adsa_lkinfo_q, "IO:adsa:q->q_lock", 0);

/*
 *  SCBCompleted is a routine defined in the adsa.c file that is used by
 *	the HIM layer to complete a SCSI job in the PDI layer.  For now,
 *	the HIM layer depends on the name being SCBCompleted.
 */

void	SCBCompleted(struct him_config_block *, struct sequencer_ctrl_block *);

/*
 *  The following extern routines are the interface into the HIM layer
 *	used by the main body of the adsa driver.
 */

extern void scb_send(struct him_config_block *, struct sequencer_ctrl_block *);
extern void scb_getconfig(struct him_config_block *);
extern void scb_enable_int(struct him_config_block *);
extern int scb_special(uchar_t, struct him_config_block *, struct sequencer_ctrl_block *);
extern int int_handler(struct him_config_block *);
extern uchar_t scb_findha(ushort_t);
extern uchar_t scb_initHA(struct him_config_block *);

/* Allocated in space.c */
extern struct ver_no	adsa_sdi_ver;	    /* SDI version structure	*/
extern int		adsa_sg_enable;		/* s/g control byte */
extern int		adsa_buson;		/* bus on time */
extern int		adsa_io_per_tar;
extern int		adsa_rediscntl[];
extern int		adsa_syncneg[];
extern int		adsa_wideneg;
extern char		adsa_access_mode;
extern char		adsa_reinit;
extern uchar_t	adsa_sync_rate[];
extern uchar_t	adsa_threshold;
extern uchar_t	adsa_bus_release;

extern int		adsa_cntls;
extern int		adsa_slots;
extern HBA_IDATA_STRUCT	_adsaidata[];
extern int		adsa_gtol[]; /* xlate global hba# to loc */
extern int		adsa_ltog[]; /* local hba# to global     */

/*
 * The name of this pointer is the same as that of the
 * original idata array. Once it it is assigned the
 * address of the new array, it can be referenced as
 * before and the code need not change.
 */
HBA_IDATA_STRUCT	*adsaidata;

/* The sm_poolhead struct is used for the dynamic struct allocation routines */
/* as pool to extract from and return structs to. This pool provides 28 byte */
/* structs (the size of sblk_t structs roughly).			     */
extern struct head	sm_poolhead;

#ifdef AHA_DEBUG1
int			adsadebug = 0;
#endif

#ifdef DEBUG
STATIC	int	adsa_panic_assert = 0;
#endif

#ifdef AHA_DEBUG_TRACE
#define AHA_DTRACE cmn_err(CE_NOTE,"%d-%s",__LINE__,__FILE__);
#else
#define AHA_DTRACE
#endif

#define		DRVNAME	"Adaptec AIC-7770 EISA SCSI HBA driver"

MOD_HDRV_WRAPPER(adsa, adsa_load, adsa_unload, NULL, DRVNAME);
HBA_INFO(adsa, &adsa_devflag, 0x10000);

int
adsa_load(void)
{
	AHA_DTRACE;

	adsa_mod_dynamic = 1;

	if( adsainit()) {
		/*
		 * We may have allocated a new idata array at this point
		 * so attempt to free it before failing the load.
		 */
		sdi_acfree(adsaidata, adsa_cntls);
		return( ENODEV );
	}
	if ( adsastart()) {
		/*
		 * At this point we have allocated a new idata array,
		 * free it before failing.
		 */
		sdi_acfree(adsaidata, adsa_cntls);
		return(ENODEV);
	}
	return(0);
}

int
adsa_unload(void)
{
	AHA_DTRACE;

	return(EBUSY);
}


/**************************************************************************
** adsa_tol()								 **
**	This routine flips a 4 byte Intel ordered long to a 4 byte SCSI  **
**	style long.							 **
**************************************************************************/
STATIC long
adsa_tol(char adr[])
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


/*
 * Function name: adsa_dma_freelist()
 * Description:
 *	Release a previously allocated scatter/gather DMA list.
 */
STATIC void
adsa_dma_freelist(dma_t *dmap)
{
	pl_t  oip;
	
	AHA_DTRACE;
	ASSERT(dmap);

	ADSA_DMALIST_LOCK(oip);
	dmap->d_next = adsa_dfreelistp;
	adsa_dfreelistp = dmap;
	if (dmap->d_next == NULL) {
		ADSA_DMALIST_UNLOCK(oip);
		SV_BROADCAST(adsa_dmalist_sv, 0);
	} else
		ADSA_DMALIST_UNLOCK(oip);
}


/**************************************************************************
** Function name: adsa_dmalist()					 **
** Description:								 **
**	Build the physical address(es) for DMA to/from the data buffer.	 **
**	If the data buffer is contiguous in physical memory, sp->s_addr	 **
**	is already set for a regular SB.  If not, a scatter/gather list	 **
**	is built, and the SB will point to that list instead.		 **
**************************************************************************/
STATIC int
adsa_dmalist(sblk_t *sp, struct proc *procp, int sleepflag)
{
	struct dma_vect			tmp_list[SG_SEGMENTS];
	register struct dma_vect  	*pp;
	register dma_t			*dmap;
	register long			count, fraglen, thispage;
	caddr_t				vaddr;
	paddr_t				addr, base;
	pl_t				oip;
	int				i;
#ifdef AHA_DEBUGSG_
	long				datal = 0;
#endif

	AHA_DTRACE;

	vaddr = sp->sbp->sb.SCB.sc_datapt;
	count = sp->sbp->sb.SCB.sc_datasz;
	pp = &tmp_list[0];

	/* First build a scatter/gather list of physical addresses and sizes */
#ifdef AHA_DEBUGSG1
	cmn_err(CE_CONT,"<Kernel - build s/g command>\n");
#endif

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
#ifdef AHA_DEBUGSG_
		datal += fraglen;
#endif
	}
	if (count != 0)
		cmn_err(CE_PANIC, "AIC-7770: Job too big for DMA list");

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
		ADSA_DMALIST_LOCK(oip);
		if (!adsa_dfreelistp && (sleepflag == KM_NOSLEEP)) {
			ADSA_DMALIST_UNLOCK(oip);
			return (1);
		}
		while ( !(dmap = adsa_dfreelistp)) {
			SV_WAIT(adsa_dmalist_sv, PRIBIO, adsa_dmalist_lock);
			ADSA_DMALIST_LOCK(oip);
		}
		adsa_dfreelistp = dmap->d_next;
		ADSA_DMALIST_UNLOCK(oip);

		sp->s_dmap = dmap;
		sp->s_addr = vtop((caddr_t) dmap->d_list, procp);
		dmap->d_size = i * sizeof(struct dma_vect);
#ifdef AHA_DEBUGSG_
		cmn_err(CE_CONT,"<S/G len %d DL%ld>\n", i, datal);
#endif
		bcopy((caddr_t) &tmp_list[0],
			(caddr_t) dmap->d_list, dmap->d_size);
	}
	return (0);
}


/**************************************************************************
** Function name: adsaxlat()						 **
** Description:								 **
**	Perform the virtual to physical translation on the SCB		 **
**	data pointer. 							 **
**************************************************************************/

/*ARGSUSED*/
HBAXLAT_DECL
HBAXLAT(struct hbadata *hbap, int flag, struct proc *procp, int sleepflag)
{
	register sblk_t *sp = (sblk_t *) hbap;

	AHA_DTRACE;

	if(sp->s_dmap) {
#ifdef AHA_DEBUGSG1
		cmn_err(CE_CONT,"s_dmap = %x, sc_link = %x\n",
		    sp->s_dmap,sp->sbp->sb.SCB.sc_link);
#endif
		adsa_dma_freelist(sp->s_dmap);
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
			if (adsa_dmalist(sp, procp, sleepflag))
				HBAXLAT_RETURN (SDI_RET_RETRY);
	} else
		sp->sbp->sb.SCB.sc_datasz = 0;

#ifdef AHA_DEBUGSG1
	cmn_err(CE_CONT,"adsaxlat: returning\n");
#endif
	HBAXLAT_RETURN (SDI_RET_OK);
}


/***************************************************************************
** Function name: adsainit()						  **
** Description:								  **
**	This is the initialization routine for the SCSI HA driver.	  **
**	All data structures are initialized, the boards are initialized	  **
**	and the EDT is built for every HA configured in the system.	  **
***************************************************************************/
int
adsainit(void)
{
	register struct scsi_ha	*ha;
	register dma_t		*dp_page, *dp;
	int			x, cntls, i, j, gb;
	uint_t			port, slot;
	int			sleepflag;
	int			mem_release;
	int			num_scsi_channels;
	int			idata_index;
	int			ndma_list, ndma_per_page, ndma_pages;
	uint_t bus_type;
	struct req_sense_def	*req_sense;

	AHA_DTRACE;

	/* if gethardware fails, skip initialization */
	if (drv_gethardware(IOBUS_TYPE, &bus_type) == -1)
		return(-1);

	/*
	 * Allocate and populate a new idata array based on the
	 * current hardware configuration for this driver.
	 */
	adsaidata = sdi_hba_autoconf("adsa", _adsaidata, &adsa_cntls);
	if(adsaidata == NULL)    {
		return (-1);
	}

	HBA_IDATA(adsa_cntls);

	adsainit_time = TRUE;

	for(i = 0; i < SDI_MAX_HBAS; i++)
		adsa_gtol[i] = adsa_ltog[i] = -1;

	sleepflag = adsa_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;
	adsa_sdi_ver.sv_release = 1;
	adsa_sdi_ver.sv_machine = SDI_386_EISA;
	adsa_sdi_ver.sv_modes   = SDI_BASIC1;

/*****************************************************************************
 * Because we can reliably searched the EISA bus for any 7770's 
 * we will look first and then allocate resources as needed.
 *****************************************************************************/
	if(adsa_slots < 2)
		adsa_slots = 2;
	if(adsa_slots > ADSA_MAX_SLOTS)
		adsa_slots = ADSA_MAX_SLOTS;
	/*
	 * To keep the user from over-flowing memory allocation, let's force
	 * adsa_io_per_tar to a more resonable number if the user gets crazy
	 */
	if(adsa_io_per_tar > 2 || adsa_io_per_tar <= 0)
		adsa_io_per_tar = 2;
	adsa_num_scbs = (adsa_io_per_tar * 8) + 8;

	adsa_ha_configsize = sizeof(struct him_config_block);
	adsa_scb_structsize = adsa_num_scbs * (sizeof(struct sequencer_ctrl_block));
	adsa_req_structsize = adsa_num_scbs * (sizeof(struct req_sense_def));
	adsa_luq_structsize = MAX_EQ * (sizeof (struct scsi_lu));
	adsa_ha_size = sizeof(struct scsi_ha);

#ifdef AHA_DEBUG1
	if(adsa_ha_size > PAGESIZE)
		cmn_err(CE_WARN, "%s: scsi_ha's exceed pagesize (may cross physical page boundary)", adsaidata[0].name);
	if(adsa_luq_structsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: LUQs exceed pagesize (may cross physical page boundary)", adsaidata[0].name);
	if(adsa_req_structsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: Request Sense Blocks exceed pagesize (may cross physical page boundary)", adsaidata[0].name);
	if(adsa_scb_structsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: CCBs exceed pagesize (may cross physical page boundary)", adsaidata[0].name);
	if(adsa_ha_configsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: him_config_block's exceed pagesize (may cross physical page boundary)", adsaidata[0].name);
	if(adsa_dma_listsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: dmalist exceeds pagesize (may cross physical page boundary)", adsaidata[0].name);
#endif

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
	ndma_list = adsa_cntls * adsa_num_scbs;
	adsa_dma_listsize = sizeof(dma_t);
	ndma_per_page = PAGESIZE / adsa_dma_listsize;
#ifdef AHA_DEBUG
	if(!ndma_per_page)
		cmn_err(CE_WARN, "%s: dmalist exceeds pagesize", adsaidata[0].name);
#endif
	ndma_pages = ( ndma_list / ndma_per_page ) + 1;

	adsa_dma_listsize = ndma_pages*PAGESIZE;
	adsa_dmalistp = (adsa_page_t *)KMEM_ZALLOC(adsa_dma_listsize, sleepflag);
	if(adsa_dmalistp == NULL) {
		cmn_err(CE_WARN, "%s: Cannot allocate adsa_dmalistp",
			 adsaidata[0].name);
		return(-1);
	}

	adsa_dfreelistp = NULL;
	for (i = 0; i < ndma_pages; i++) {
		dp_page = (dma_t *)(void *)&adsa_dmalistp[i];
		/* Build list of free DMA lists */
		for (j = 0; j < ndma_per_page; j++) {
			dp = &dp_page[j];
			dp->d_next = adsa_dfreelistp;
			adsa_dfreelistp = dp;
		}
	}

	/*
	 * Now, loop through all of the EISA slots and search for installed
	 * 7770-type host adapters.  We only do this for the number of HAs
	 * configured into the driver via the System file.
	 *
	 * Once we find an HA, adsaidata.idata_nbus is set to the number 
	 * of SCSI buses on the HA.
	 *
	 * First, null out the ha array.
	 */
	for(gb = 0; gb < (SDI_MAX_HBAS * ADSA_MAX_BPC); gb++)
		adsa_sc_ha[gb] = CNFNULL;

	adsa_habuscnt = 0;
	for(idata_index = 0; idata_index < adsa_cntls; idata_index++) {
		/*
		 * Have we max'ed out the operating systems limit of
		 * adapters?
		 */
		if(idata_index > SDI_MAX_HBAS) {
			break;
		}
		port =
		adsaidata[idata_index].ioaddr1 =
			(adsaidata[idata_index].ioaddr1 & (~ADSA_IOADDR_MASK)) | BASE_PORT;
		slot = (port & (~ADSA_IOADDR_MASK)) >> SLOT_SHIFT;
#ifdef AHA_DEBUGSLOT
		cmn_err(CE_NOTE,"searching slot %d", slot);
#endif
		num_scsi_channels = scb_findha(port);
#ifdef AHA_DEBUG2
	cmn_err(CE_CONT,"just found %d channels at port 0x%x\n",num_scsi_channels,port);
#endif
		/*
		 * No adapters in this slot or the adapter has not been
		 * configured into the EISA bus via the EISA config utility.
		 */
		if(num_scsi_channels == 0 || num_scsi_channels & 0x80)
			continue;
		/*
		 * Out of paranoia, make sure we only see the lower 2 bits
		 */
		adsaidata[idata_index].idata_nbus = num_scsi_channels &= 3;

		cmn_err(CE_NOTE,"!Found %d SCSI channels at port 0x%x", num_scsi_channels, port);

		mem_release = 0;

		adsa_ctob[idata_index] = adsa_habuscnt;

		for(x = 0; x < num_scsi_channels; x++) {

			ha = (struct scsi_ha *)KMEM_ZALLOC(adsa_ha_size, sleepflag);
			if(ha == NULL) {
				cmn_err(CE_WARN,
					"%s: slot %d CH%d Cannot allocate adapter blocks",
					adsaidata[idata_index].name, slot, x);
				goto adsa_release;
			}
			mem_release |= HA_REL;
			adsa_sc_ha[adsa_habuscnt] = ha;

			ha->ha_config = (struct him_config_block *)KMEM_ZALLOC(adsa_ha_configsize, sleepflag);
			if(ha->ha_config == NULL) {
				cmn_err(CE_WARN,
					"%s: slot %d CH%d Cannot allocate config blocks",
					adsaidata[idata_index].name, slot, x);
				goto adsa_release;
			}
			mem_release |= HA_CONFIG_REL;

			ha->ha_name = adsaidata[idata_index].name;
			cmn_err(CE_NOTE, "!V1.8c %s: Channel %d, slot %d",
			    ha->ha_name, x, slot);
			ha->ha_ctl = (uchar_t)idata_index;
			ha->ha_num_chan = (uchar_t)num_scsi_channels;
			ha->ha_chan_num = (char)x;
#ifdef AHA_DEBUG1
			/* the physical slot number for this adapter */
			ha->ha_slot = (slot << SLOT_SHIFT);
#endif
			ha->ha_config->Port_Address = (ushort_t)port;

			req_sense = (struct req_sense_def *)KMEM_ZALLOC(adsa_req_structsize, sleepflag);
			if(req_sense == NULL) {
				cmn_err(CE_WARN,
				    "%s: slot %d CH%d Cannot allocate request sense blocks",
				    adsaidata[idata_index].name, slot, x);
				goto adsa_release;
			}
			mem_release |= HA_REQS_REL;

			ha->ha_dev = (struct scsi_lu *)KMEM_ZALLOC(adsa_luq_structsize, sleepflag);
			if(ha->ha_dev == NULL) {
				cmn_err(CE_WARN, "%s: slot %d CH%d Cannot allocate lu queues",
				    adsaidata[idata_index].name, slot, x);
				goto adsa_release;
			}
			mem_release |= HA_DEV_REL;

			ha->ha_id = adsaidata[idata_index].ha_id;
			ha->ha_vect = adsaidata[idata_index].iov;

			ha->ha_scb = (struct sequencer_ctrl_block *)KMEM_ZALLOC(adsa_scb_structsize, sleepflag);
			if (ha->ha_scb == NULL) {
				cmn_err(CE_WARN,
				    "%s: slot %d CH%d Cannot allocate command blocks",
				    adsaidata[idata_index].name, slot, x);
				goto adsa_release;
			}
			mem_release |= HA_SCB_REL;

#ifdef AHA_DEBUG2
	cmn_err(CE_CONT,"about to adsa_init_scbs\n");
#endif
			adsa_init_scbs(ha, req_sense);

			/* init HA communication */
#ifdef AHA_DEBUG2
	cmn_err(CE_CONT,"about to adsa_hainit\n");
#endif
			if(adsa_hainit(ha, x, sleepflag)) {
#ifdef AHA_DEBUG2
	cmn_err(CE_CONT,"about to adsa_alloc_sg\n");
#endif
				if(!adsa_alloc_sg(ha)) {
					adsa_habuscnt++;
					adsaidata[idata_index].active = 1;
					adsa_lockinit(idata_index);
					continue;
				}
			}

adsa_release:
			if(num_scsi_channels == 1) {
				mem_release |= HA_STRUCT_REL;
			} else if(x == 0) {
				mem_release |= HA_CH0_REL;
			} else {
				mem_release |= HA_CH1_REL;
			}
			adsa_mem_release(&adsa_sc_ha[adsa_habuscnt], mem_release);
			break;
		}
	}
	if(adsa_habuscnt == 0) {
		cmn_err(CE_NOTE,"!%s: No HBA's found.",adsaidata[0].name);
		return(-1);
	}

	adsa_switch(adsa_habuscnt);

	/*
	 * Attach interrupts for each "active" device. The driver
	 * must set the active field of the idata structure to 1
	 * for each device it has configured.
	 *
	 * Note: Interrupts are attached here, in init(), as opposed
	 * to load(), because this is required for static autoconfig
	 * drivers as well as loadable.
 	 */
	sdi_intr_attach(adsaidata, adsa_cntls, adsaintr, adsa_devflag);

#ifdef AHA_DEBUG2
	cmn_err(CE_CONT,"about to return from adsainit\n");
#endif
	return(0);
}


/*
 * Function name: adsastart()
 * Description:	
 *	Called by kernel to perform driver initialization
 *	after the kernel data area has been initialized.
 */
int
adsastart(void)
{
	int			any_has, c, cntl_num, mem_release;
	register struct scsi_ha	*ha;
	int 			gb;

	AHA_DTRACE;

	/*
	 * For all configured controller channels, register them with SDI
	 * one at a time if they are currently marked active.
	 *
	 * The active flag is cleared so we can identify the ones that
	 * sdi_register correctly.
	 */
#ifdef AHA_DEBUG2
	cmn_err(CE_CONT,"adsastart, about to loop on habuscnt=%d\n",adsa_habuscnt);
#endif
	for(c = 0; c < adsa_cntls; c++) {
		if (!adsaidata[c].active)
			continue;

#ifdef AHA_DEBUG2
	cmn_err(CE_CONT,"about to sdi_gethbano cntlr %d\n", c);
#endif
		/* Get an HBA number from SDI and register HBA with SDI */
		if((cntl_num = sdi_gethbano(adsaidata[c].cntlr)) <= -1) {
			cmn_err(CE_WARN, "%s: C%d No HBA number available. %d", \
				adsaidata[c].name, c, cntl_num);
			adsaidata[c].active = 0;
			continue;
		}

		adsaidata[c].cntlr = cntl_num;
		adsa_gtol[cntl_num] = c;
		adsa_ltog[c] = cntl_num;

#ifdef AHA_DEBUG2
	cmn_err(CE_CONT,"about to sdi_register cntlr %d\n", c);
#endif
		if((cntl_num = sdi_register(&adsahba_info, &adsaidata[c])) < 0) {
			cmn_err(CE_WARN, "!%s: C%d, SDI registry failure %d.", \
			   adsaidata[c].name, c, cntl_num);
			adsaidata[c].active = 0;
			continue;
		}
	}

	/*
	 * For all configured controller channels, count the ones that
	 * are currently marked active.
	 *
	 * If one is not active, this means that either sdi_gethbano failed
	 * or sdi_register failed or it previously failed in adsainit.
	 * In any case, if there is still memory associated with it, free it.
	 */
	for(mem_release = 0, any_has = 0, c = 0; c < adsa_cntls; c++) {
		adsa_lockclean(c);
		if (adsaidata[c].active) {
			any_has++;
			continue;
		}
		for (gb = 0; gb < adsa_habuscnt; gb++) {

			ha = adsa_sc_ha[gb];
			if(ha == CNFNULL || ha->ha_ctl != c)
				continue;
			mem_release |= (HA_SG_REL|HA_REQS_REL|HA_DEV_REL|HA_SCB_REL|HA_CONFIG_REL|HA_REL);
			if(ha->ha_num_chan == 1) {
				mem_release |= HA_STRUCT_REL;
			} else if(ha->ha_chan_num == 0) {
				mem_release |= HA_CH0_REL;
			} else {
				mem_release |= HA_CH1_REL;
			}
			adsa_mem_release(&adsa_sc_ha[gb], mem_release);
		}
	}

	/*
	 * If there are no active HA channels, fail the start
	 */
	if ( !any_has )
		return(-1);

	/*
	 * Clear init time flag to stop the HA driver
	 * from polling for interrupts and begin taking
	 * interrupts normally and return SUCCESS.
	 */
	adsainit_time = FALSE;
#ifdef AHA_DEBUG2
	cmn_err(CE_CONT,"about to return from adsastart\n");
#endif
	return(0);
}


/*
 * adsa_hainit()
 *	This is the guy that does all the hardware initialization of the
 *	adapter after it has been determined to be in the system.
 */
STATIC int
adsa_hainit(struct scsi_ha *ha, int flag, int sleepflag)
{
	register struct him_config_block *conf;
	register int	i;
	uchar_t	z;
	int		x;

	AHA_DTRACE;

	conf = ha->ha_config;

	/***************************************************
	 * Now I know this looks kinda' silly, but it really
	 * is practical.  In order to call getconfig for
	 * channel A, you only need to have the port and
	 * channel designator filled in.
	 * For channel B you need the above and the pointer
	 * to the him_data_block.  But not the pointer that
	 * we will allocate later.  We need the one that is
	 * munged by the first call to initHA for channel
	 * A.  Sounds as clear as mud?  Keep looking.
	 ***************************************************/
	if(flag == 0)
		conf->Cfe_SCSIChannel = A_CHANNEL;
	else {
		conf->Cfe_SCSIChannel = B_CHANNEL;
		conf->Cfe_HaDataPtr = adsa_ha_struct;
		conf->Cfe_ScbParam.Prm_HimDataPhysaddr = vtop((caddr_t)adsa_ha_struct, NULL); 
	}
#ifdef AHA_DEBUG2
	cmn_err(CE_CONT,"getconfig (%x) channel %d, port %x\n", conf, conf->SCSIChannel, port);
#endif
	/***************************************************
	 * Let's get some config information
	 ***************************************************/
#ifdef AHA_DEBUG1
	cmn_err(CE_CONT,"calling scb_getconfig (%x) channel %d\n", conf, conf->SCSI_Channel);
#endif
	scb_getconfig(conf);

	/***************************************************
	 * Okay, with that first call, for channel A, the
	 * getconfig function was nice enough to provide us
	 * with the correct size of the him_data_block.  AS
	 * this block is used for both channels, it is only
	 * allocated when we talk to channel A.
	 ***************************************************/
	if(flag == 0) {
		adsa_him_structsize = conf->Cfe_ScbParam.Prm_HimDataSize;
#ifdef AHA_DEBUG2
		if(adsa_him_structsize > PAGESIZE)
			cmn_err(CE_WARN, "%s: him_data_blocks exceed pagesize (may cross physical page boundary)", adsaidata[0].name);
#endif
		adsa_ha_struct = (struct him_data_block *)KMEM_ZALLOC(adsa_him_structsize, sleepflag);
		if(adsa_ha_struct == NULL) {
			cmn_err(CE_WARN,
				"%s: CH0 Cannot allocate HIM struct blocks",
				adsaidata[0].name);
			return(0);
		}
		/*********************************************
		 * Remember, we need to store this for channel
		 * A only for right now.
		 *********************************************/
		conf->Cfe_HaDataPtr = adsa_ha_struct;
		conf->Cfe_ScbParam.Prm_HimDataPhysaddr = vtop((caddr_t)adsa_ha_struct, NULL); 
	}
	/***************************************************
	 * We need to know what our adapter TID is
	 ***************************************************/
	ha->ha_id = conf->Cfe_ScsiId;

	/***************************************************
	 * More config stuff.  Getting ready to do the init
	 * thing.
	 ***************************************************/
	conf->Cfe_ConfigFlags &= ~SAVE_SEQ;
	conf->Cfe_ScbParam.Prm_MaxTScbs = 1;
	conf->Cfe_ScbParam.Prm_MaxNTScbs = MAX_NONTAG;
	/***************************************************
	 * Are we OPTIMA or are we not?  That is the
	 * question!
	 ***************************************************/
	if((conf->Cfe_ReleaseLevel < 2) ||
	   (adsa_access_mode == SMODE_NOSWAP)) {
		conf->Cfe_ScbParam.Prm_AccessMode = SMODE_NOSWAP;
		conf->Cfe_ScbParam.Prm_NumberScbs = 4;
	} else {
		conf->Cfe_ScbParam.Prm_AccessMode = SMODE_SWAP;
		conf->Cfe_ScbParam.Prm_NumberScbs = (UWORD)adsa_num_scbs;
	}

	if(ha->ha_vect != conf->IRQ_Channel) {
		cmn_err(CE_WARN, "%s: CH%d Interrupt should be %d, currently set to %d", ha->ha_name, ha->ha_chan_num, ha->ha_vect, conf->IRQ_Channel);
		return(0);
	}
#ifdef AHA_DEBUG1
	if(conf->HA_Config_Flags & INIT_NEEDED)
		cmn_err(CE_CONT,"Intializing Adapter %d\n", ha->ha_chan_num);
#endif
	if(adsa_reinit || (conf->HA_Config_Flags & INIT_NEEDED)) {
		cmn_err(CE_NOTE, "!%s: CH%d Re-initializing based on space.c file", ha->ha_name, ha->ha_chan_num);
		conf->HA_Config_Flags |= INIT_NEEDED;
		if(adsa_threshold > 3)
			adsa_threshold = 1;
		conf->Threshold = adsa_threshold;
		conf->DMA_Channel = 0xff;
		conf->SCSI_ID = ha->ha_id;
		if(adsa_bus_release < 2 || adsa_bus_release > 60)
			adsa_bus_release = 60;
		conf->Bus_Release = adsa_bus_release;
		if(adsa_rediscntl[ha->ha_chan_num] & 0x80) {
#ifdef AHA_DEBUG1
			cmn_err(CE_CONT,"Allowing disconnect %x\n",
				adsa_rediscntl[ha->ha_chan_num]);
#endif
			conf->Allow_Dscnt = adsa_rediscntl[ha->ha_chan_num];
		} else
			conf->Allow_Dscnt = 0;
		if((adsa_wideneg & 0x80) && conf->Max_Targets > 8) {
			x = 1;
			for(z = 0; z < conf->Max_Targets; z++) {
				if(adsa_wideneg & x)
					conf->SCSI_Option[z] |= WIDE_MODE;
				else
					conf->SCSI_Option[z] &= ~WIDE_MODE;
				x <<= 1;
			}
		} else {
			for(z = 0; z < conf->Max_Targets; z++)
				conf->SCSI_Option[z] &= ~WIDE_MODE;
		}
		if(adsa_syncneg[ha->ha_chan_num] & 0x80) {
			x = 1;
			for(z = 0; z < conf->Max_Targets; z++) {
				if(adsa_syncneg[ha->ha_chan_num] & x) {
					conf->SCSI_Option[z] &= ~SYNC_RATE;
					conf->SCSI_Option[z] |= SYNC_MODE;
					switch(adsa_sync_rate[z]) {
						case SYNC_RATE_36:
							conf->SCSI_Option[z] |= SYNC_36;
							break;
						case SYNC_RATE_40:
							conf->SCSI_Option[z] |= SYNC_40;
							break;
						case SYNC_RATE_44:
							conf->SCSI_Option[z] |= SYNC_44;
							break;
						case SYNC_RATE_57:
							conf->SCSI_Option[z] |= SYNC_57;
							break;
						case SYNC_RATE_67:
							conf->SCSI_Option[z] |= SYNC_67;
							break;
						case SYNC_RATE_80:
							conf->SCSI_Option[z] |= SYNC_80;
							break;
						case SYNC_RATE_100:
							break;
						case SYNC_RATE_50:
						default:
							conf->SCSI_Option[z] |= SYNC_50;
							break;
					}
				} else
					conf->SCSI_Option[z] &= ~SYNC_MODE;
				x <<= 1;
			}
		} else {
			for(z = 0; z < conf->Max_Targets; z++) {
				conf->SCSI_Option[z] &= ~SYNC_RATE;
				conf->SCSI_Option[z] &= ~SYNC_MODE;
			}
		}
	}
	conf->HA_Config_Flags &= ~BIOS_ACTIVE;
#ifdef AHA_DEBUG1
	cmn_err(CE_CONT,"Config_Flags %x\n", conf->HA_Config_Flags);
#endif

#ifdef AHA_DEBUG1
	cmn_err(CE_CONT,"calling initHA\n");
#endif
	i = scb_initHA(conf);
	if(i) {
#ifdef AHA_DEBUG1
		cmn_err(CE_CONT,"INITHA FAILED!!!:scb_initHA %d\n", i);
#endif
		return(0);
	}

	if (flag == 0)
		adsa_ha_struct = conf->Cfe_HaDataPtr;

	scb_enable_int(conf);
#ifdef AHA_DEBUG1
	i = inb(ha->ha_slot | 0xc87);
	if(i != 0xa) {
		cmn_err(CE_CONT,"INT PORT STATUS 0x%x\n", i);
	}
#endif
#ifdef AHA_DEBUG1
	cmn_err(CE_CONT,"Found a good adapter \n");
#endif
	return(1);
}


/*
 * Function name: adsa_init_scbs()
 * Description:	
 *	Initialize the adapter scb free list.
 */
STATIC void
adsa_init_scbs(struct scsi_ha *ha, struct req_sense_def *req_sense)
{
	struct sequencer_ctrl_block	*cp;
	int			i, cnt;

	AHA_DTRACE;

	ha->ha_num_scb = cnt = adsa_num_scbs;

	cnt--;
	for(i = 0; i < cnt; i++) {
		cp = &ha->ha_scb[i];
#ifdef AHA_DEBUG1
		if(ha->ha_chan_num > 0)
			cmn_err(CE_CONT,"%x ", cp);
#endif
		cp->c_next = &ha->ha_scb[i+1];
		cp->SCB_CDBPtr = vtop((caddr_t)&cp->SCB_CDB[0], NULL);
		cp->SCB_SensePtr = vtop((caddr_t)&req_sense[i], NULL);
		cp->virt_SensePtr = &req_sense[i];
		cp->SCB_SenseLen = sizeof(struct req_sense_def);
	}
	cp = &ha->ha_scb[i];
#ifdef AHA_DEBUG1
	if(ha->ha_chan_num > 0)
		cmn_err(CE_CONT,"%x ", cp);
#endif
	cp->c_next = &ha->ha_scb[0];
	cp->SCB_CDBPtr = vtop((caddr_t)&cp->SCB_CDB[0], NULL);
	cp->SCB_SensePtr = vtop((caddr_t)&req_sense[i], NULL);
	cp->virt_SensePtr = &req_sense[i];
	cp->SCB_SenseLen = sizeof(struct req_sense_def);

	ha->ha_scb_next = cp;
#ifdef AHA_DEBUG1
	cmn_err(CE_CONT,"adsa_init_scbs returning\n");
#endif
}


/**************************************************************************
** Function name: adsaopen()						 **
** Description:								 **
** 	Driver open() entry point. It checks permissions, and in the	 **
**	case of a pass-thru open, suspends the particular LU queue.	 **
**************************************************************************/

/*ARGSUSED*/
int
HBAOPEN(dev_t *devp, int flags, int otype, cred_t *cred_p)
{
	dev_t	dev = *devp;
	minor_t		minor = geteminor(dev);
	register int	c = adsa_gtol[SC_EXHAN(minor)];
	register int	b = SC_BUS(minor);
	register int	t = SC_EXTCN(minor);
	register int	l = SC_EXLUN(minor);
	register struct scsi_lu *q;
	int		gb = adsa_ctob[c] + b;
	struct scsi_ha  *ha = adsa_sc_ha[gb];

	AHA_DTRACE;

	if (t == ha->ha_id)
		return(0);

	if (adsa_illegal(SC_EXHAN(minor), b, t, l)) {
		return(ENXIO);
	}

	/* This is the pass-thru section */

	q = &LU_Q(c, b, t, l);

	ADSA_SCSILU_LOCK(q->q_opri);
 	if ((q->q_outcnt > 0)  || (q->q_flag & (QBUSY | QSUSP | QPTHRU))) {
		ADSA_SCSILU_UNLOCK(q->q_opri);
		return(EBUSY);
	}

	q->q_flag |= QPTHRU;
	ADSA_SCSILU_UNLOCK(q->q_opri);
	return(0);
}


/***************************************************************************
** Function name: adsaclose()						  **
** Description:								  **
** 	Driver close() entry point.  In the case of a pass-thru close	  **
**	it resumes the queue and calls the target driver event handler	  **
**	if one is present.						  **
***************************************************************************/

/*ARGSUSED*/
int
HBACLOSE(dev_t dev, int flags, int otype, cred_t *cred_p)
{
	minor_t		minor = geteminor(dev);
	register int	c = adsa_gtol[SC_EXHAN(minor)];
	register int	b = SC_BUS(minor);
	register int	t = SC_EXTCN(minor);
	register int	l = SC_EXLUN(minor);
	register struct scsi_lu *q;
	int		gb = adsa_ctob[c] + b;
	struct scsi_ha  *ha = adsa_sc_ha[gb];

	AHA_DTRACE;

	if (t == ha->ha_id)
		return(0);

	q = &LU_Q(c, b, t, l);

	ADSA_CLEAR_QFLAG(q,QPTHRU);

#ifdef AHA_DEBUG1
	sdi_aen(SDI_FLT_PTHRU, c, t, l);
#else
	if (q->q_func != NULL)
		(*q->q_func) (q->q_param, SDI_FLT_PTHRU);
#endif

	ADSA_SCSILU_LOCK(q->q_opri);
	adsa_next(q);
	return(0);
}


/*
 * Function name: adsaintr()
 * Description:	
 *	Driver interrupt handler entry point.  Called by kernel when
 *	a host adapter interrupt occurs.
 */
void
adsaintr(uint vect)
{
	register struct scsi_ha	*ha;
	register struct him_config_block *conf;
	register int		bus;
#ifdef AHA_DEBUG2
	int			int_status;
	int			i, x;
#endif
#ifdef AHA_DEBUGSCB
	struct sequencer_ctrl_block *scb_ptr;
#endif

	AHA_DTRACE;

	for(bus = 0; bus < adsa_habuscnt; bus++) {
		ha = adsa_sc_ha[bus];
		if(ha == CNFNULL)
			continue;
		if(ha->ha_vect != vect)
			continue;
		conf = ha->ha_config;
#ifdef AHA_DEBUG1
		cmn_err(CE_CONT,"(%d, %d) ", bus, adsa_habuscnt);
#endif

#ifdef AHA_DEBUG1
		i = inb(ha->ha_slot | 0xc91);
		x = inb(ha->ha_slot | 0xc87);
		cmn_err(CE_CONT,"INTERRUPT STATUS 0x%x ", i);
		if(i & SCSIINT)
			cmn_err(CE_CONT,"SCSIINT ");
		if(i & CMDCMPLT)
			cmn_err(CE_CONT,"CMDCMPLT ");
		if(i & SEQINT)
			cmn_err(CE_CONT,"SEQINT");
 		cmn_err(CE_CONT,"\nHOST CONTROL 0x%x ", x);
		if(x & POWRDN)
			cmn_err(CE_CONT,"POWRDN ");
		if(x & SWINT)
			cmn_err(CE_CONT,"SWINT ");
		if(x & IRQMS)
			cmn_err(CE_CONT,"IRQMS ");
		if(x & PAUSEACK)
			cmn_err(CE_CONT,"PAUSEACK ");
		if(x & INTEN)
			cmn_err(CE_CONT,"INTEN ");
		if(x & CHIPRESET)
			cmn_err(CE_CONT,"CHIPRESET");
		cmn_err(CE_CONT,"\n");
#endif

#ifdef AHA_DEBUG1
		cmn_err(CE_CONT,"calling int_handler for adp%d/int%d\n", bus, vect);
#endif
		ADSA_HIM_LOCK;
#ifdef AHA_DEBUG1
		int_status = int_handler(conf);
#else
		int_handler(conf);
#endif
		ADSA_HIM_UNLOCK;

#ifdef AHA_DEBUG1
		cmn_err(CE_CONT,"int_handler returned %x\n", int_status);
#endif
#ifdef AHA_DEBUGSCB
		scb_ptr = conf->scb_ptr;
		cmn_err(CE_CONT,"int_handler returned %d %d %x\n",
			int_status, ha->ha_npend, scb_ptr->SCB_Stat);
#endif
	}
#ifdef AHA_DEBUG1
	cmn_err(CE_CONT,"adsaintr returning %d\n", int_status);
#endif
}

/*
 * void
 * SCBCompleted(struct him_config_block *config_ptr,
 *              struct sequencer_ctrl_block *scb_ptr)
 *
 *	Interface back into the PDI layer from the HIM layer.  Calls adsa_sgdone
 *	or adsa_ha_done as appropriate which then call sdi_callback.
 *
 * Calling/Exit State:
 *	ADSA_HIM_LOCK held on entry and reacquired before exit.
 *	ADSA_HIM_LOCK released during execution.
 *	Acquires ADSA_NPEND_LOCK.
 *	Acquires ADSA_SCSILU_LOCK.
 */

/*ARGSUSED*/
void
SCBCompleted(struct him_config_block *config_ptr, struct sequencer_ctrl_block *scb_ptr)
{
	register struct scsi_ha	*ha;
	register struct scsi_lu *q;
	int			gbus;
	int			int_status;
	pl_t		oip;

	AHA_DTRACE;

	gbus = scb_ptr->adapter;

	ha = adsa_sc_ha[gbus];
	if(!adsaidata[ha->ha_ctl].active) {
		return;
	}

	ADSA_HIM_UNLOCK;

	int_status = scb_ptr->SCB_Stat;
#ifdef AHA_DEBUG1
	cmn_err(CE_CONT,"target %x, int_status %x\n", scb_ptr->target, int_status);
#endif
	switch(int_status) {
		case SCB_COMP:
		case SCB_ERR:
		case SCB_PENDING:
		case SCB_ABORTED:
		case INV_SCB_CMD:
			ADSA_NPEND_LOCK(oip);
			ha->ha_npend--;
			ADSA_NPEND_UNLOCK(oip);

			q = &LU_Q_GB(gbus, scb_ptr->target, (scb_ptr->SCB_Tarlun & LUN));
			ADSA_SCSILU_LOCK(q->q_opri);
			q->q_outcnt--;
			if(q->q_outcnt < adsa_io_per_tar) {
				q->q_flag &= ~QBUSY;
			}
			ADSA_SCSILU_UNLOCK(q->q_opri);
		
			if(scb_ptr->SCB_Cmd == EXEC_SCB) 
				adsa_sgdone(gbus, scb_ptr, int_status, q);
			else
				adsa_ha_done(q, scb_ptr, int_status);
			break;
		default:
			cmn_err(CE_WARN, "%s: CH%d Invalid interrupt status - 0x%x",
				 ha->ha_name, ha->ha_chan_num, int_status);
			break;
	}

	ADSA_HIM_LOCK;

	adsa_waitflag = FALSE;
}


/**************************************************************************
** Function: adsa_sgdone()						 **
** Description:								 **
**	This routine is used as a front end to calling adsa_done.	 **
**	This is the cleanest way I found to cleanup a local s/g command. **
**	One should note, however, with this scheme, if an error occurs	 **
**	during the local s/g command, the system will think an error	 **
**	will have occured on all the commands.				 **
**************************************************************************/
STATIC void
adsa_sgdone(int gb, struct sequencer_ctrl_block *cp, int status, struct scsi_lu *q)
{
	register sblk_t		*sb;
	register struct sb	*sp;
	register int		i;
	long			x;

	AHA_DTRACE;

	if(cp->c_segcnt > 1 && cp->c_sg_p != SGNULL) {
		x = cp->data_len;
		x /= SG_LENGTH;
#ifdef AHA_DEBUGSG1
		cmn_err(CE_CONT,"<T%d -", x);
#endif
		x--;
		for(i = 0; i < x; i++) {
			sb = cp->c_sg_p->spcomms[i];
			adsa_done(SG_NOSTART, gb, cp, status, &sb->sbp->sb, q);
#ifdef AHA_DEBUGSG1
			cp->c_sg_p->spcomms[i] = ((sblk_t *)0);
			cmn_err(CE_CONT," %d", i);
#endif
		}
		sb = cp->c_sg_p->spcomms[i];
		adsa_done(SG_START, gb, cp, status, &sb->sbp->sb, q);
#ifdef AHA_DEBUGSG1
		cp->c_sg_p->spcomms[i] = ((sblk_t *)0);
		cmn_err(CE_CONT," %d\n", i);
#endif
#ifdef AHA_DEBUGSG1
		cmn_err(CE_CONT,"3) free s/g list %x\n", sg_p);
#endif
		cp->c_sg_p->sg_flags = SG_FREE;
		cp->c_sg_p = SGNULL;
		FREESCB(cp);
	} else { /* call target driver interrupt handler */
		sp = cp->c_bind;
		adsa_done(SG_START, gb, cp, status, sp, q);
		FREESCB(cp);
	}
#ifdef AHA_DEBUGSG1
	cmn_err(CE_CONT,"returning from adsa_sgdone\n");
#endif
}


/**************************************************************************
** Function name: adsa_ha_done()					 **
** Description:								 **
**	This is the interrupt handler routine for SCSI jobs which have	 **
**	a controller CB and a SFB structure defining the job.		 **
**************************************************************************/
STATIC void
adsa_ha_done(struct scsi_lu *q, struct sequencer_ctrl_block *cp, int status)
{
	register struct sb	*sp;

	AHA_DTRACE;

#ifdef AHA_DEBUG1
	cmn_err(CE_CONT,"entering adsa_ha_done(%x, %x, %x)\n", q, cp, status);
#endif

	sp = cp->c_bind;

	/* Determine completion status of the job */

	switch (status) {
		case SCB_COMP:
			sp->SFB.sf_comp_code = SDI_ASW;
			adsa_flushq(q, SDI_CRESET, 0);
			break;
		case SCB_ERR:
		case SCB_PENDING:
		case SCB_ABORTED:
		case INV_SCB_CMD:
			sp->SFB.sf_comp_code = (ulong_t)SDI_HAERR;
			break;
		default:
			ASSERT(adsa_panic_assert);
			sp->SFB.sf_comp_code = (ulong_t)SDI_ERROR;
			break;
	}

	/* call target driver interrupt handler */
	sdi_callback(sp);

	FREESCB(cp);

	ADSA_SCSILU_LOCK(q->q_opri);
	adsa_next(q);

#ifdef AHA_DEBUG1
	cmn_err(CE_CONT,"adsa_hadone returning\n");
#endif
} 


/**************************************************************************
** Function name: adsa_done()						 **
** Description:								 **
**	This is the interrupt handler routine for SCSI jobs which have	 **
**	a controller CB and a SCB structure defining the job.		 **
**************************************************************************/
STATIC void
adsa_done(int flag, int gb, struct sequencer_ctrl_block *cp, int status, struct sb *sp, struct scsi_lu *q)
{
	register int		i;

	AHA_DTRACE;

	ADSA_CLEAR_QFLAG(q,QSENSE); /* Old sense data now invalid */

	/* Determine completion status of the job */
	switch(status) {
	case SCB_COMP:
		sp->SCB.sc_comp_code = SDI_ASW;
		break;

	case SCB_RETRY:
		sp->SCB.sc_comp_code = (ulong_t)SDI_RETRY;
		break;

	case SCB_ABORTED:
	case SCB_ERR:
		switch(cp->SCB_HaStat) {
		case HOST_NO_STATUS:
			if (cp->SCB_TargStat == UNIT_GOOD) {
				sp->SCB.sc_comp_code = SDI_ASW;
			} else {
				sp->SCB.sc_comp_code = (ulong_t)SDI_CKSTAT;
				sp->SCB.sc_status = cp->SCB_TargStat;
			/* Cache request sense info (note padding) */
				if(cp->SCB_SenseLen < (sizeof(q->q_sense) - 1))
					i = cp->SCB_SenseLen;
				else
					i = sizeof(q->q_sense) - 1;
				bcopy((caddr_t)cp->virt_SensePtr, (caddr_t)(sdi_sense_ptr(sp))+1, i);

				ADSA_SCSILU_LOCK(q->q_opri);
				bcopy((caddr_t)cp->virt_SensePtr, (caddr_t)(&q->q_sense)+1, i);
				q->q_flag |= QSENSE;	/* sense data now valid */
				ADSA_SCSILU_UNLOCK(q->q_opri);
				bzero((caddr_t)cp->virt_SensePtr, cp->SCB_SenseLen);
			}
			break;
		case HOST_SEL_TO:
			sp->SCB.sc_comp_code = (ulong_t)SDI_NOSELE;
			break;
		case HOST_PHASE_ERR:
			sp->SCB.sc_comp_code = (ulong_t)SDI_TCERR; 
			break;
		case HOST_ABT_HOST:
		case HOST_ABT_HA:
			sp->SCB.sc_comp_code = (ulong_t)SDI_ABORT;
			break;
		case HOST_HW_ERROR:
			cmn_err(CE_PANIC,
				"%s: CH%d Host Adapter Hardware Failure",
				adsa_sc_ha[gb]->ha_name,
				adsa_sc_ha[gb]->ha_chan_num);
			break;
		case HOST_DU_DO:
			cmn_err(CE_WARN, "%s: CH%d Data over/under run error",
				 adsa_sc_ha[gb]->ha_name,
				 adsa_sc_ha[gb]->ha_chan_num);
			sp->SCB.sc_comp_code = (ulong_t)SDI_HAERR;
			break;
		case HOST_BUS_FREE:
			cmn_err(CE_WARN, "%s: CH%d Unexpected Bus Free error",
				 adsa_sc_ha[gb]->ha_name,
				 adsa_sc_ha[gb]->ha_chan_num);
			sp->SCB.sc_comp_code = (ulong_t)SDI_TCERR;
			break;
		case HOST_SNS_FAIL:
			cmn_err(CE_WARN, "%s: CH%d Auto-Request Sense Failed",
				 adsa_sc_ha[gb]->ha_name,
				 adsa_sc_ha[gb]->ha_chan_num);
			sp->SCB.sc_comp_code = (ulong_t)SDI_HAERR;
			break;
		case HOST_ABT_FAIL:
			cmn_err(CE_WARN,
				 "%s: CH%d Target did not respond to ATTN",
				 adsa_sc_ha[gb]->ha_name,
				 adsa_sc_ha[gb]->ha_chan_num);
			sp->SCB.sc_comp_code = (ulong_t)SDI_TCERR;
			break;
		case HOST_RST_HA:
			cmn_err(CE_WARN,
				 "%s: CH%d SCSI Bus Reset by Host Adapter",
				 adsa_sc_ha[gb]->ha_name,
				 adsa_sc_ha[gb]->ha_chan_num);
			sp->SCB.sc_comp_code = (ulong_t)SDI_HAERR;
			break;
		case HOST_RST_OTHER:
			cmn_err(CE_WARN,
				 "%s: CH%d SCSI Bus Reset by other device",
				 adsa_sc_ha[gb]->ha_name,
				 adsa_sc_ha[gb]->ha_chan_num);
			sp->SCB.sc_comp_code = (ulong_t)SDI_CRESET;
			break;
		case HOST_INV_LINK:
			cmn_err(CE_WARN,
				 "%s: CH%d Invalid SCSI Linking Operation",
				 adsa_sc_ha[gb]->ha_name,
				 adsa_sc_ha[gb]->ha_chan_num);
			sp->SCB.sc_comp_code = (ulong_t)SDI_HAERR;
			break;
		case HOST_TAG_REJ:
			cmn_err(CE_WARN, "%s: CH%d Target Rejected Tag Queuing",
				 adsa_sc_ha[gb]->ha_name,
				 adsa_sc_ha[gb]->ha_chan_num);
		default:
			sp->SCB.sc_comp_code = (ulong_t)SDI_HAERR;
			break;
		}
		break;

	case INV_SCB_CMD:
		cmn_err(CE_WARN, "%s: CH%d Illegal command",
			 adsa_sc_ha[gb]->ha_name,
			 adsa_sc_ha[gb]->ha_chan_num);
		sp->SCB.sc_comp_code = (ulong_t)SDI_ERROR;
		break;

	default:
		cmn_err(CE_WARN, "!%s: CH%d Illegal status 0x%x!",
			adsa_sc_ha[gb]->ha_name, adsa_sc_ha[gb]->ha_chan_num,
			status);
		ASSERT(adsa_panic_assert);
		sp->SCB.sc_comp_code = (ulong_t)SDI_ERROR;
		break;
	}
	cp->SCB_TargStat = UNIT_GOOD;
	cp->SCB_HaStat = HOST_NO_STATUS;

	sdi_callback(sp); /* call target driver interrupt handler */

	if(flag == SG_START) {
		ADSA_SCSILU_LOCK(q->q_opri);
		adsa_next(q);
	}
} 


/**************************************************************************
** Function name: adsa_int()						 **
** Description:								 **
**	This is the interrupt handler for pass-thru jobs.  It just	 **
**	wakes up the sleeping process.					 **
**************************************************************************/
STATIC void
adsa_int(struct sb *sp)
{
	struct buf  *bp;

	AHA_DTRACE;

	bp = (struct buf *) sp->SCB.sc_wd;
	biodone(bp);
}


/**************************************************************************
** Function: adsa_illegal()						 **
** Description: Used to verify commands					 **
**************************************************************************/
STATIC int
adsa_illegal(short hba, int bus, uchar_t scsi_id, uchar_t lun)
{
	AHA_DTRACE;

	if(sdi_rxedt(hba, bus, scsi_id, lun)) {
#ifdef AHA_DEBUG1
		cmn_err(CE_CONT,"adsa_illegal return good C%d/B%d/T%d/L%d\n", 
			hba, bus, scsi_id, lun);
#endif
		return(0);
	} else {
#ifdef AHA_DEBUG1
	cmn_err(CE_CONT,"adsa_illegal return bad (%d,%d,%d,%d)\n",hba,bus,scsi_id,lun);
#endif
		return(1);
	}
}


/**************************************************************************
** Function name: adsa_func()						 **
** Description:								 **
**	Create and send an SFB associated command. 			 **
**************************************************************************/
STATIC void
adsa_func(sblk_t *sp)
{
	register struct scsi_ad	*sa;
	register struct scsi_ha *ha;
	register struct sequencer_ctrl_block	*cp;
	struct him_config_block	*conf;
	int			c, b, gb, target, lun;

	AHA_DTRACE;

	sa = &sp->sbp->sb.SFB.sf_dev;
	c = adsa_gtol[SDI_EXHAN(sa)];
	b = SDI_BUS(sa);
	target = SDI_EXTCN(sa);
	lun = SDI_EXLUN(sa);
	gb = adsa_ctob[c] + b;
 	ha = adsa_sc_ha[gb];

	cp = adsa_getblk(gb);
	conf = ha->ha_config;
	cp->SCB_Cmd = HARD_RST_DEV;
	cp->SCB_Stat = 0;
	cp->SCB_Flags = (AUTO_SENSE|NO_UNDERRUN);
	cp->SCB_Tarlun = conf->SCSI_Channel;
	cp->SCB_Tarlun |= target << 4;
	cp->SCB_Tarlun |= lun;
	cp->c_segcnt = cp->SCB_SegCnt = 0;
	cp->SCB_SegPtr = 0;
	cp->SCB_CDBLen = 0;
	cp->SCB_HaStat = 0;
	cp->SCB_TargStat = 0;
	cp->data_len = 0;
	cp->adapter = gb;

#ifdef AHA_DEBUG1
	cmn_err(CE_CONT,"1) calling scb_send ha->ha_config %x cp %x\n", conf, cp);
#endif
	cmn_err(CE_WARN, "%s: CH%d TARGET%d LUN%d is being reset",
		ha->ha_name, ha->ha_chan_num, target, lun);
	ADSA_HIM_LOCK;
	scb_send(conf, cp);
	ADSA_HIM_UNLOCK;
#ifdef AHA_DEBUG_
	if(cp->SCB_HaStat != HOST_NO_STATUS)
		cmn_err(CE_CONT,"HOST ADAPTER STATUS %x\n", cp->SCB_HaStat);
	cmn_err(CE_CONT,"adsa_func end\n");
#endif
}



/*
 * STATIC void
 * adsa_pass_thru0(buf_t *bp)
 *	Calls buf_breakup to make sure the job is correctly broken up/copied.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
adsa_pass_thru0(buf_t *bp)
{
	minor_t	minor = geteminor(bp->b_edev);
	int	c = adsa_gtol[SC_EXHAN(minor)];
	int	b = SC_BUS(minor);
	int	t = SC_EXTCN(minor);
	int	l = SC_EXLUN(minor);
	struct scsi_lu	*q = &LU_Q(c, b, t, l);

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
		q->q_bcbp->bcb_max_xfer = adsahba_info.max_xfer - ptob(1);
		q->q_bcbp->bcb_physreqp->phys_align = ADSA_MEMALIGN;
		q->q_bcbp->bcb_physreqp->phys_boundary = ADSA_BOUNDARY;
		q->q_bcbp->bcb_physreqp->phys_dmasize = ADSA_DMASIZE;
		q->q_bcbp->bcb_granularity = 1;
	}

	buf_breakup(adsa_pass_thru, bp, q->q_bcbp);
}

/*
 * STATIC void
 * adsa_pass_thru(buf_t *bp)
 *	Send a pass-thru job to the HA board.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSA_SCSILU_LOCK(q->q_opri).
 */
STATIC void
adsa_pass_thru(buf_t *bp)
{
	minor_t	minor = geteminor(bp->b_edev);
	int	c = adsa_gtol[SC_EXHAN(minor)];
	int	b = SC_BUS(minor);
	int	t = SC_EXTCN(minor);
	int	l = SC_EXLUN(minor);
	register struct scsi_lu	*q;
	register struct sb *sp;
	caddr_t *addr;
	char op;
	daddr_t blkno_adjust;

	AHA_DTRACE;

	sp = (struct sb *)bp->b_private;

	sp->SCB.sc_wd = (long)bp;
	sp->SCB.sc_datapt = (caddr_t) paddr(bp);
	sp->SCB.sc_datasz = bp->b_bcount;
	sp->SCB.sc_int = adsa_int;

	SDI_TRANSLATE(sp, bp->b_flags, bp->b_proc, KM_SLEEP);

	q = &LU_Q(c, b, t, l);

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
		scs->ss_len   = (char)(bp->b_bcount >> ADSA_BLKSHFT);
	}
	if (op == SM_READ || op == SM_WRITE) {
		daddr_t blkno;
		struct scm *scm = (struct scm *)(void *)((char *)q->q_sc_cmd - 2);

		if (blkno_adjust) {
			blkno = bp->b_priv2.un_int + blkno_adjust;
			scm->sm_addr = sdi_swap32(blkno);
		}
		scm->sm_len  = sdi_swap16(bp->b_bcount >> ADSA_BLKSHFT);
	}

	sdi_icmd(sp, KM_SLEEP);
}


/*
 * Function name: adsa_next()
 * Description:
 *	Attempt to send the next job on the logical unit queue.
 *	All jobs are not sent if the Q is busy.
 */
STATIC void
adsa_next(struct scsi_lu *q)
{
	register sblk_t			*sp;
	register struct scsi_ad		*sa;
	register int			c, gb;

	AHA_DTRACE;
	ASSERT(q);

	if (q->q_flag & QBUSY) {
		ADSA_SCSILU_UNLOCK(q->q_opri);
		return;
	}

	if((sp = q->q_first) == NULL) {	 /*  queue empty  */
		q->q_depth = 0;
		ADSA_SCSILU_UNLOCK(q->q_opri);
		return;
	}

	if (q->q_flag & QSUSP && sp->sbp->sb.sb_type == SCB_TYPE) {
		ADSA_SCSILU_UNLOCK(q->q_opri);
		return;
	}

/*
 * The test conditions:
 * 1) If the command type is a function, instead of a normal SCSI command
 *    can't do s/g.  So do [1a].
 * 2) There is more than 1 command in the queue.
 * 3) If the current command is already a s/g command from the page I/O stuff
 *    just do it in a normal manner.
 * 4) All the test conditions were right, so we have a chance to do a s/g
 *    command.
 * 5) Shucks!  One of the test conditions (2, or 3) failed and we have to
 *    do the command normally.
 */
	if(sp->sbp->sb.sb_type == SFB_TYPE) {		/* [1]  */
		if (!(q->q_first = sp->s_next))
			q->q_last = NULL;
		else
			q->q_first->s_prev = NULL;
		q->q_depth--;
		ADSA_SCSILU_UNLOCK(q->q_opri);
		adsa_func(sp);				/* [1a] */
	} else if(q->q_depth > 1 && !sp->s_dmap && adsa_sg_enable == 1) {
		sa = &sp->sbp->sb.SCB.sc_dev;
		c = adsa_gtol[SDI_EXHAN(sa)];
		gb = adsa_ctob[c] + SDI_BUS(sa);
		adsa_sctgth(q, gb);			/* [4] */
	} else {
		if(!(q->q_first = sp->s_next))
			q->q_last = NULL;
		else
			q->q_first->s_prev = NULL;
		q->q_depth--;
		ADSA_SCSILU_UNLOCK(q->q_opri);
		adsa_cmd(SGNULL , sp);	/* [5]  */
	}

	if(adsainit_time == TRUE) {		/* need to poll */
		if(adsa_wait(30000) == LOC_FAILURE) {
			cmn_err(CE_NOTE, "ADSA Initialization Problem, timeout with no interrupt\n");
			sp->sbp->sb.SCB.sc_comp_code = (ulong_t)SDI_TIME;
			sdi_callback(&sp->sbp->sb);
		}
	}
}


/*
 * Function name: adsa_putq()
 *	Put a job on a logical unit queue.  Jobs are enqueued
 *	on a priority basis.
 */
STATIC void
adsa_putq(struct scsi_lu *q, sblk_t *sp)
{
	register cls = ADSA_QUECLASS(sp);

	AHA_DTRACE;

	/* 
	 * If queue is empty or queue class of job is less than
	 * that of the last one on the queue, tack on to the end.
	 */
	if(!q->q_first || (cls <= ADSA_QUECLASS(q->q_last))) {
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
		register sblk_t *nsp = q->q_first;

		while(ADSA_QUECLASS(nsp) >= cls)
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


/*
 * Function name: adsa_flushq()
 *	Empty a logical unit queue.  If flag is set, remove all jobs.
 *	Otherwise, remove only non-control jobs.
 */
STATIC void
adsa_flushq(struct scsi_lu *q, int cc, int flag)
{
	register sblk_t  *sp, *nsp;

	AHA_DTRACE;
	ASSERT(q);

	ADSA_SCSILU_LOCK(q->q_opri);
	sp = q->q_first;
	q->q_first = q->q_last = NULL;
	q->q_depth = 0;
	ADSA_SCSILU_UNLOCK(q->q_opri);

	while (sp) {
		nsp = sp->s_next;
		if (!flag && (ADSA_QUECLASS(sp) > HBA_QNORM)) {
			ADSA_SCSILU_LOCK(q->q_opri);
			adsa_putq(q, sp);
			ADSA_SCSILU_UNLOCK(q->q_opri);
		} else {
			sp->sbp->sb.SCB.sc_comp_code = (ulong)cc;
			sdi_callback(&sp->sbp->sb);
		}
		sp = nsp;
	}
}


/*
 * Function name: adsa_send()
 * Description:
 *	Send a command to the host adapter board.
 */
STATIC void
adsa_send(int gb, struct sequencer_ctrl_block *cp)
{
	register struct scsi_ha		*ha = adsa_sc_ha[gb];
	register struct scsi_lu		*q;
	register struct him_config_block *conf;
	pl_t				oip;

	AHA_DTRACE;

	q = &LU_Q_GB(gb, cp->target, (cp->SCB_Tarlun & LUN));
	ADSA_SCSILU_LOCK(q->q_opri);
	q->q_outcnt++;
	if(q->q_outcnt >= adsa_io_per_tar) {
		q->q_flag |= QBUSY;
	}
	ADSA_SCSILU_UNLOCK(q->q_opri);

	ADSA_NPEND_LOCK(oip);
	ha->ha_npend++;
	ADSA_NPEND_UNLOCK(oip);

	conf = ha->ha_config;
	ADSA_HIM_LOCK;
	scb_send(conf, cp);		/* send the command via HIM */
	ADSA_HIM_UNLOCK;
}


/*
 * Function name: adsa_getblk()
 * Description:	
 *	Allocate a controller command block structure.
 */
STATIC struct sequencer_ctrl_block *
adsa_getblk(int gb)
{
	struct scsi_ha	*ha = adsa_sc_ha[gb];
	struct sequencer_ctrl_block	*cp;
#ifdef DEBUG
	int		cnt;
#endif
	pl_t	oip;

	AHA_DTRACE;

	ADSA_SCB_LOCK(oip);
	cp = ha->ha_scb_next;
#ifdef DEBUG
	cnt = 0;
#endif

	while(cp->c_active) {
#ifdef DEBUG
		cnt++;
#endif
		ASSERT(cnt < ha->ha_num_scb);
		cp = cp->c_next;
	}
	ha->ha_scb_next = cp->c_next;
	cp->c_active = TRUE;
	ADSA_SCB_UNLOCK(oip);
	ha->ha_poll = cp;
	return(cp);
}


/*
 * Function name: adsa_cmd()
 * Description:	
 *	Create and send an SCB associated command, using a scatter
 *	gather list created by adsa_sctgth().
 */
STATIC void
adsa_cmd(SGARRAY *sg_p, sblk_t *sp)
{
	struct scsi_ad		*sa;
	struct sequencer_ctrl_block	*cp;
	struct scsi_lu		*q;
	int			i;
	char				*p;
	long				bcnt, cnt;
	int				c, b, target, lun, gb;
	paddr_t				addr;

	AHA_DTRACE;

	if(sg_p != SGNULL)
		sp = sg_p->spcomms[0];

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = adsa_gtol[SDI_EXHAN(sa)];
	b = SDI_BUS(sa);
	gb = adsa_ctob[c] + b;
	target = SDI_EXTCN(sa);
	lun = SDI_EXLUN(sa);

	cp = adsa_getblk(gb);


	/* fill in the controller command block */

	cp->SCB_Cmd = EXEC_SCB;
	cp->SCB_Stat = 0;
	cp->SCB_Flags = (AUTO_SENSE|NO_UNDERRUN);
	cp->SCB_Cntrl = 0;
	cp->SCB_Tarlun = adsa_sc_ha[gb]->ha_config->SCSI_Channel;
	cp->SCB_Tarlun |= target << 4;
	cp->SCB_Tarlun |= lun;
	cp->SCB_ResCnt = 0;
	cp->SCB_HaStat = 0;
	cp->SCB_TargStat = UNIT_GOOD;
	cp->SCB_CDBLen = sp->sbp->sb.SCB.sc_cmdsz;

	for(i = 0; i < 16; i++)
		cp->SCB_RsvdX[i] = 0;

	cp->target = target;
	cp->adapter = gb;
	cp->c_bind = &sp->sbp->sb;

	/*
	 * If the sg_p pointer is not NULL or the sp->s_dmap contains a valid
	 * address, we have a s/g command to do.
	 * [1] Set the scb opcode to a s/g type.
	 * [2] If s_dmap is valid, then set the length and addr.  If it is
	 * 	not valid, the length and addr will have been passed in the
	 *	call to adsa_cmd, to indicate the driver built a local s/g
	 *	command. Setting the c_sg_p to a NULL will allow the interrupt
	 *	to recognize this command as a normal I/O or a page I/O s/g
	 *	command.
	 * [3] Must be a normal command. (shucks)
	 * [4] Set up the scb for a normal command, set the length and addr.
	 */
	if(sp->s_dmap || sg_p != SGNULL) {
#ifdef AHA_DEBUGSG1
		cmn_err(CE_CONT,"<S/G command ");
#endif
		if(sp->s_dmap) {			/* [2] */
#ifdef AHA_DEBUGSG1
			cmn_err(CE_CONT,"KERNEL BUILT>\n");
#endif
			cnt = sp->s_dmap->d_size;	/*  ^  */
			addr = sp->s_addr;		/*  ^  */
			cp->c_sg_p = SGNULL;		/*  ^  */
		} else {
#ifdef AHA_DEBUGSG1
			cmn_err(CE_CONT,"HOMEMADE>\n");
#endif
			cnt = sg_p->sg_len;
			addr = sg_p->sg_paddr;
		}
		cp->data_len = (int)cnt;
#ifdef AHA_DEBUG1
		cmn_err(CE_CONT,"SG DATA LENGTH %x", cp->data_len);
#endif
		cp->SCB_SegPtr = addr;
		cp->c_segcnt = cp->SCB_SegCnt = cnt / SG_LENGTH;
		cp->SCB_Cntrl |= (REJECT_MDP|DIS_ENABLE);
	} else {					/* [3] */
		cp->SCB_Cntrl |= DIS_ENABLE;
		if(sp->sbp->sb.SCB.sc_datasz) {
#ifdef AHA_DEBUG1
			cmn_err(CE_CONT,"move data\n");
#endif
			cp->c_sg_p = SGNULL;
			cp->data_len = sp->sbp->sb.SCB.sc_datasz;
			cp->single_sg.sg_addr = sp->s_addr;
			cp->single_sg.sg_size = cp->data_len;
#ifdef AHA_DEBUG1
			cmn_err(CE_CONT,"1/DATA LENGTH %x", cp->data_len);
#endif
			cp->SCB_SegPtr = vtop((caddr_t)&cp->single_sg, NULL);
			cp->c_segcnt = cp->SCB_SegCnt = 1;
		} else {
#ifdef AHA_DEBUG1
			cmn_err(CE_CONT,"NO DATA");
#endif
			cp->data_len = 0;
			cp->c_segcnt = cp->SCB_SegCnt = 0;
			cp->SCB_SegPtr = sp->s_addr;
		}
	}

	/* copy SCB cdb to controller CB cdb */
 	p = sp->sbp->sb.SCB.sc_cmdpt;
	for(i = 0; i < (int)cp->SCB_CDBLen; i++)
		cp->SCB_CDB[i] = *p++;

#ifdef AHA_DEBUG1
	cmn_err(CE_CONT,"SCB command 0x%x datal %d ccb_length %d, sense_length %d\n", cp->SCB_CDB[0], cp->data_len, cp->SCB_CDBLen, cp->SCB_SenseLen);
#endif
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
	 *	reached and *not* be a READ or WRITE, then in adsa_sctgth we
	 *	should explicitly look for READ or WRITE as part of the
	 *	determining factor whether a s/g command can be done or not.
	 */
	if(sg_p != SGNULL) {
		cp->c_sg_p = sg_p;				/* [1] */
		bcnt = sg_p->dlen >> 9;
		switch(cp->SCB_CDB[0]) {			/* [3] */
			case SS_READ:
				cp->SCB_CDB[4] = (uchar_t)bcnt;
				cp->SCB_Flags &= ~NO_UNDERRUN;
				break;
			case SM_READ:
				cp->SCB_CDB[7] =
				    (uchar_t)((bcnt >> 8) & 0xff);
				cp->SCB_CDB[8] =
				    (uchar_t)(bcnt & 0xff);
				cp->SCB_Flags &= ~NO_UNDERRUN;
				break;
			case SS_WRITE:
				cp->SCB_CDB[4] = (uchar_t)bcnt;
				cp->SCB_Flags &= ~NO_UNDERRUN;
				break;
			case SM_WRITE:
				cp->SCB_CDB[7] =
				    (uchar_t)((bcnt >> 8) & 0xff);
				cp->SCB_CDB[8] =
				    (uchar_t)(bcnt & 0xff);
				cp->SCB_Flags &= ~NO_UNDERRUN;
				break;
			default:				/* [3a] */
				cmn_err(CE_PANIC,
					"%s: CH%d Unknown SCSI command for s/g",
					adsa_sc_ha[gb]->ha_name,
					adsa_sc_ha[gb]->ha_chan_num);
				break;
		}
	}

	q = &LU_Q(c, b, target, sa->sa_lun);
	ADSA_SCSILU_LOCK(q->q_opri);
	if ((q->q_flag & QSENSE) && (cp->SCB_CDB[0] == SS_REQSEN)) {
		int copyfail;
		caddr_t toaddr;
#ifdef AHA_DEBUGSG_
		cmn_err(CE_CONT,"<REQUEST SENSE COMMAND>\n");
#endif
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
 		if(cp->SCB_SegCnt <= 1) {
			if (toaddr = physmap(sp->s_addr, cp->data_len, KM_NOSLEEP)) {
 				bcopy((caddr_t)(&q->q_sense)+1, toaddr, cp->data_len);
				physmap_free(toaddr, cp->data_len, 0);
			} else {
				copyfail++ ;
			}
 		} else {
			struct dma_vect	*VectPtr;
			int	VectCnt;
			caddr_t	Src;
			caddr_t	optr, Dest;
			ulong_t	Count;
		 
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
		ADSA_SCSILU_UNLOCK(q->q_opri);
		adsa_done(SG_START, gb, cp, copyfail?SCB_RETRY:SCB_COMP, cp->c_bind, q);
		FREESCB(cp);
		adsa_waitflag = FALSE;
	} else {
		ADSA_SCSILU_UNLOCK(q->q_opri);
		adsa_send(gb, cp);
	}

	return;
}


/**************************************************************************
** Function: adsa_sctgth()						 **
** Description:								 **
**	This routine is responsible for building local s/g commands.  It **
**	depends on the driver being able to queue commands for any given **
**	target, as well as the adapter being able to support s/g types   **
**	of commands.							 **
**************************************************************************/
STATIC void
adsa_sctgth(struct scsi_lu *q, int gb)
{
	register SGARRAY	*sg_p;
	register SG		*sg;
	register sblk_t		*sp, *start_sp;
	char			current_command, command;
	int			segments;
	int			length;
	long			start_block, current_block;
	long			total_count;
	caddr_t			p;
	int			unlocked;

	AHA_DTRACE;

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

	if((sg_p = adsa_get_sglist(gb)) == SGNULL) {		/* [1] */
		sp = q->q_first;				/* [2] */
		if(!(q->q_first = sp->s_next))			/* [3] */
			q->q_last = NULL;			/*  ^  */
		else
			q->q_first->s_prev = NULL;
		if(sp != NULL) {				/* [4] */
			if (sp->sbp->sb.sb_type == SFB_TYPE) {	/* [5] */
				q->q_depth--;
				ADSA_SCSILU_UNLOCK(q->q_opri);
				adsa_func(sp);
			} else {				/* [6] */
				q->q_depth--;
				ADSA_SCSILU_UNLOCK(q->q_opri);
				adsa_cmd(SGNULL, sp);
			}
		} else
			ADSA_SCSILU_UNLOCK(q->q_opri);
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
			ADSA_SCSILU_UNLOCK(q->q_opri);
			adsa_func(sp);
			if(segments == 0) {
				sg_p->sg_flags = SG_FREE;
				return;
			}
			ADSA_SCSILU_LOCK(q->q_opri);
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
				start_block = adsa_tol(&p[2]);
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
			current_block = adsa_tol(&p[2]);
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
			ADSA_SCSILU_UNLOCK(q->q_opri);
			unlocked = 1;
			adsa_cmd(SGNULL, start_sp);
		}
	} else {						/* [30] */
		sg_p->sg_len = length;
		sg_p->dlen = total_count;
		sg_p->sg_paddr = vtop((caddr_t)&sg_p->sg_seg[0].sg_addr, NULL);
		q->q_depth -= segments;
		ADSA_SCSILU_UNLOCK(q->q_opri);
		unlocked = 1;
		adsa_cmd(sg_p, ((sblk_t *)0));
	}
	if(sp != NULL) {					/* [31] */
		if ( unlocked )
			ADSA_SCSILU_LOCK(q->q_opri);
		q->q_depth--;
		ADSA_SCSILU_UNLOCK(q->q_opri);
		adsa_cmd(SGNULL, sp);
	}
}


/*
 * Function: adsa_get_sglist()
 * Description:	
 *	Here is the routine that returns a pointer to the next free
 *	s/g list.  Each list is 17 segments long.  I have only allocated
 *	adsa_num_scb of these structures, instead of doing it on a per
 *	adapter basis.  Doesn't hurt if it returns a NULL as it just
 *	means the calling routine will just have to do it the
 *	old-fashion way.  This routine runs just like adsa_getblk, which
 *	is commented very well.  So see those comments if you need to.
 */
STATIC SGARRAY *
adsa_get_sglist(int gb)
{
	SGARRAY	*sg;
	int		cnt;
	pl_t	oip;
	struct scsi_ha  *ha = adsa_sc_ha[gb];

	AHA_DTRACE;

	ADSA_SGARRAY_LOCK(oip);
	sg = ha->ha_sgnext;
	cnt = 0;

	while(sg->sg_flags == SG_BUSY) {
		cnt++;
		if(cnt >= ha->ha_num_scb) {
			ADSA_SGARRAY_UNLOCK(oip);
			return(SGNULL);
		}
		sg = sg->sg_next;
	}
	sg->sg_flags = SG_BUSY;
	ha->ha_sgnext = sg->sg_next;
	ADSA_SGARRAY_UNLOCK(oip);
	return(sg);
}


/**************************************************************************
** Function Name: adsa_wait()						 **
** Description:								 **
**	Poll for a completion from the host adapter.  If an interrupt	 **
**	is seen, the HA's interrupt service routine is manually called.  **
**  NOTE:								 **
**	This routine allows for no concurrency and as such, should 	 **
**	be used selectively.						 **
**************************************************************************/
STATIC int
adsa_wait(int time)
{
	AHA_DTRACE;

	while (time > 0) {
		if (adsa_waitflag == FALSE) {
			adsa_waitflag = TRUE;
			return (LOC_SUCCESS);
		}
#ifdef AHA_DEBUG2
if (!(time%8)) cmn_err(CE_CONT,".");
#endif
		drv_usecwait(1000);
		time--;
	}

#ifdef AHA_DEBUG2
cmn_err(CE_NOTE,"Bad news, timeout with no interrupt\n");
#endif
	return (LOC_FAILURE);
}

/**************************************************************************
** Function name: adsafreeblk()						 **
** Description:								 **
**	Release previously allocated SB structure. If a scatter/gather	 **
**	list is associated with the SB, it is freed via			 **
**	adsa_dma_freelist().						 **
**	A nonzero return indicates an error in pointer or type field.	 **
**************************************************************************/
long
HBAFREEBLK(struct hbadata *hbap)
{
	register sblk_t *sp = (sblk_t *) hbap;

	AHA_DTRACE;

	if(sp->s_dmap) {
		adsa_dma_freelist(sp->s_dmap);
		sp->s_dmap = NULL;
	}
	sdi_free(&sm_poolhead, (jpool_t *)sp);
	return (SDI_RET_OK);
}


/**************************************************************************
** Function name: adsagetblk()						 **
** Description:								 **
**	Allocate a SB structure for the caller.  The function will	 **
**	sleep if there are no SCSI blocks available.			 **
**************************************************************************/
struct hbadata *
HBAGETBLK(int sleepflag)
{
	register sblk_t	*sp;

	AHA_DTRACE;

	sp = (sblk_t *)SDI_GET(&sm_poolhead, sleepflag);
	return((struct hbadata *)sp);
}


/**************************************************************************
** Function name: adsagetinfo()						 **
** Description:								 **
**	Return the name, etc. of the given device.  The name is copied	 **
**	into a string pointed to by the first field of the getinfo	 **
**	structure.							 **
**************************************************************************/
void
HBAGETINFO( struct scsi_ad *sa, struct hbagetinfo *getinfop)
{
	register char  *s1, *s2;
	static char temp[] = "HA X TC X";

	AHA_DTRACE;

	s1 = temp;
	s2 = getinfop->name;
	temp[3] = SDI_HAN(sa) + '0';
	temp[8] = SDI_TCN(sa) + '0';

	while ((*s2++ = *s1++) != '\0')
		;
#ifdef AHA_DEBUG1
	cmn_err(CE_CONT,"adsagetinfo(%x, %s)\n", sa, getinfop->name);
#endif

	getinfop->iotype = F_DMA_32 | F_SCGTH;

	if (getinfop->bcbp) {
		getinfop->bcbp->bcb_addrtypes = BA_KVIRT;
		getinfop->bcbp->bcb_flags = 0;
		getinfop->bcbp->bcb_max_xfer = adsahba_info.max_xfer;
		getinfop->bcbp->bcb_physreqp->phys_align = ADSA_MEMALIGN;
		getinfop->bcbp->bcb_physreqp->phys_boundary = ADSA_BOUNDARY;
		getinfop->bcbp->bcb_physreqp->phys_dmasize = ADSA_DMASIZE;
	}
}


/**************************************************************************
** Function name: adsaicmd()						 **
** Description:								 **
**	Send an immediate command.  If the logical unit is busy, the job **
**	will be queued until the unit is free.  SFB operations will take **
**	priority over SCB operations.					 **
**************************************************************************/

/*ARGSUSED*/
long
HBAICMD(struct  hbadata *hbap, int sleepflag)
{
	register struct scsi_ad *sa;
	register struct scsi_lu *q;
	register sblk_t *sp = (sblk_t *) hbap;
	register int	c, t, l, b, gb;
	struct scs	*inq_cdb;
	struct sdi_edt	*edtp;

	AHA_DTRACE;


	switch (sp->sbp->sb.sb_type) {
	case SFB_TYPE:
		sa = &sp->sbp->sb.SFB.sf_dev;
		c = adsa_gtol[SDI_EXHAN(sa)];
		t = SDI_EXTCN(sa);
		l = SDI_EXLUN(sa);
		b = SDI_BUS(sa);
		q = &LU_Q(c, b, t, l);
#ifdef AHA_DEBUG2
		cmn_err(CE_CONT,"adsa_icmd: SFB c=%d b=%d t=%d l=%d \n", c, b, t, l);
#endif

		sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;

		switch (sp->sbp->sb.SFB.sf_func) {
		case SFB_RESUME:
			ADSA_SCSILU_LOCK(q->q_opri);
			q->q_flag &= ~QSUSP;
			adsa_next(q);
			break;
		case SFB_SUSPEND:
			ADSA_SET_QFLAG(q,QSUSP);
			break;
		case SFB_RESETM:
			edtp = sdi_rxedt(adsa_ltog[c],b,t,l);
			if(edtp != NULL && edtp->pdtype == ID_TAPE) {
				/*  this is a NOP for tape devices */
				sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;
				break;
			}
			/*FALLTHRU*/
		case SFB_ABORTM:
			sp->sbp->sb.SFB.sf_comp_code = SDI_PROGRES;
			ADSA_SCSILU_LOCK(q->q_opri);
			adsa_putq(q, sp);
			adsa_next(q);
			return (SDI_RET_OK);
		case SFB_FLUSHR:
			adsa_flushq(q, SDI_QFLUSH, 0);
			break;
		case SFB_NOPF:
			break;
		default:
			sp->sbp->sb.SFB.sf_comp_code = (ulong_t)SDI_SFBERR;
		}

		sdi_callback(&sp->sbp->sb);
		return (SDI_RET_OK);

	case ISCB_TYPE:
		sa = &sp->sbp->sb.SCB.sc_dev;
		c = adsa_gtol[SDI_EXHAN(sa)];
		t = SDI_EXTCN(sa);
		l = SDI_EXLUN(sa);
		b = SDI_BUS(sa);
		gb = adsa_ctob[c] + b;
		q = &LU_Q(c, b, t, l);
#ifdef AHA_DEBUG2
		cmn_err(CE_CONT,"adsa_icmd: SCB c=%d b=%d t=%d l=%d \n", c, b, t, l);
#endif
		inq_cdb = (struct scs *)(void *)sp->sbp->sb.SCB.sc_cmdpt;

		if((t == adsa_sc_ha[gb]->ha_id) && (l == 0) && (inq_cdb->ss_op == SS_INQUIR)) {
			struct ident inq;
			struct ident *inq_data;
			int inq_len;

			bzero(&inq, sizeof(struct ident));
			(void)strncpy(inq.id_vendor, adsa_sc_ha[gb]->ha_name, 
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

		ADSA_SCSILU_LOCK(q->q_opri);
		adsa_putq(q, sp);
		adsa_next(q);
		return (SDI_RET_OK);

	default:
#ifdef AHA_DEBUG2
		cmn_err(CE_CONT,"adsa_icmd: SDI_RET_ERR\n");
#endif
		sdi_callback(&sp->sbp->sb);
		return (SDI_RET_ERR);
	}
}


/*
 * Function name: adsaioctl()
 * Description:	
 *	Driver ioctl() entry point.  Used to implement the following
 *	special functions:
 *
 *	SDI_SEND     -	Send a user supplied SCSI Control Block to
 *			the specified device.
 *	B_GETTYPE    -  Get bus type and driver name
 *	B_HA_CNT     -	Get number of HA boards configured
 *	REDT	     -	Read the extended EDT from RAM memory
 *	SDI_BRESET   -	Reset the specified SCSI bus
 *
 */

/*ARGSUSED*/
int
HBAIOCTL(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *cred_p, int *rval_p)
{
	minor_t		minor = geteminor(dev);
	register int	c = adsa_gtol[SC_EXHAN(minor)];
	register int	b = SC_BUS(minor);
	register int	t = SC_EXTCN(minor);
	register int	l = SC_EXLUN(minor);
	int		gb = adsa_ctob[c] + b;
	register struct sb *sp;
	pl_t  oip;
	int  uerror = 0;

	AHA_DTRACE;

	switch(cmd) {
	case SDI_SEND: {
		register buf_t *bp;
		struct sb  karg;
		int  rw;
		char *save_priv;
		struct scsi_lu *q;

		if (t == adsa_sc_ha[gb]->ha_id) { 	/* illegal ID */
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

		q = &LU_Q(c, b, t, l);
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
		sp->SCB.sc_dev.sa_bus = (uchar_t)b;
		sp->SCB.sc_dev.sa_exta = (uchar_t)t;
		sp->SCB.sc_dev.sa_ct = SDI_SA_CT(adsa_ltog[c],t);

		rw = (sp->SCB.sc_mode & SCB_READ) ? B_READ : B_WRITE;
		bp->b_private = (char *)sp;

		/*
		 * If the job involves a data transfer then the
		 * request is done thru physiock() so that the user
		 * data area is locked in memory. If the job doesn't
		 * involve any data transfer then adsa_pass_thru()
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
				struct scs *scs = (struct scs *)(void *)q->q_sc_cmd;
				bp->b_priv2.un_int =
				   ((sdi_swap16(scs->ss_addr) & 0xffff)
				   + (scs->ss_addr1 << 16));
			}
			if (op == SM_READ || op == SM_WRITE) {
				struct scm *scm = 
				(struct scm *)(void *)((char *)q->q_sc_cmd - 2);
				bp->b_priv2.un_int = sdi_swap32(scm->sm_addr);
			}

			if (uerror = physiock(adsa_pass_thru0, bp, dev, rw, 
			    HBA_MAX_PHYSIOCK, &ha_uio)) {
				goto done;
			}
		} else {
			bp->b_un.b_addr = NULL;
			bp->b_bcount = 0;
			bp->b_blkno = 0;
			bp->b_edev = dev;
			bp->b_flags |= rw;

			adsa_pass_thru(bp);  /* Bypass physiock call */
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
		if (copyout((caddr_t)&adsa_sdi_ver,arg, sizeof(struct ver_no)))
			return(EFAULT);
		break;
	case SDI_BRESET: {
		register struct scsi_ha *ha = adsa_sc_ha[gb];

		ADSA_NPEND_LOCK(oip);
		if (ha->ha_npend > 0) {     /* jobs are outstanding */
			ADSA_NPEND_UNLOCK(oip);
			return(EBUSY);
		} else {
			cmn_err(CE_WARN, "%s: CH%d - Bus is being reset",
				ha->ha_name, ha->ha_chan_num);
		}
		ADSA_NPEND_UNLOCK(oip);
		break;
		}

	default:
		return(EINVAL);
	}
	return(uerror);
}


/**************************************************************************
** Function name: adsasend()						 **
** Description:								 **
** 	Send a SCSI command to a controller.  Commands sent via this	 **
**	function are executed in the order they are received.		 **
**************************************************************************/

/*ARGSUSED*/
long
HBASEND(struct hbadata *hbap, int sleepflag)
{
	register struct scsi_ad *sa;
	register struct scsi_lu *q;
	register sblk_t *sp = (sblk_t *) hbap;
	register int	c, b, t, l;

	AHA_DTRACE;

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = adsa_gtol[SDI_EXHAN(sa)];
	b = SDI_BUS(sa);
	t = SDI_EXTCN(sa);
	l = SDI_EXLUN(sa);

	if (sp->sbp->sb.sb_type != SCB_TYPE) {
		return (SDI_RET_ERR);
	}

	q = &LU_Q(c, b, t, l);

	ADSA_SCSILU_LOCK(q->q_opri);
	if (q->q_flag & QPTHRU) {
		ADSA_SCSILU_UNLOCK(q->q_opri);
		return (SDI_RET_RETRY);
	}

	sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
	sp->sbp->sb.SCB.sc_status = 0;

	adsa_putq(q, sp);
	adsa_next(q);

	return (SDI_RET_OK);
}


STATIC int
adsa_alloc_sg(struct scsi_ha *ha)
{
	register int		i, j;
	SGARRAY *sg_page, *sg_p;
	int			sg_listsize;
	int			sleepflag;
	int		nsg_per_page, nsg_pages;
	SGARRAY			*adsa_sg_next;
	adsa_page_t		*adsa_scatter;

	AHA_DTRACE;

	sleepflag = adsa_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;

/*
 * Now add the initialization for the scatter/gather lists and the pointers
 * for the sblk_t sp arrays to keep track of.  I have opted for circular
 * queue (linked lists) of each of these lists.  In the routine that returns
 * a free list I will keep a forward moving pointer for each HA
 * that will keep pointing
 * to the next array.  This will almost always insure the free pointer will
 * be found in the first check, keeping overhead to a minimum. (i.e. atomic)
 */
	sg_listsize = sizeof(SGARRAY);
	nsg_per_page = PAGESIZE / sg_listsize;
#ifdef AHA_DEBUG
	if(!nsg_per_page)
		cmn_err(CE_WARN, "%s: sg list exceeds pagesize", ha->ha_name);
#endif
	nsg_pages = ( adsa_num_scbs / nsg_per_page ) + 1;

	sg_listsize = nsg_pages * PAGESIZE;
	adsa_scatter = (adsa_page_t *)KMEM_ZALLOC(sg_listsize, sleepflag);
	if(adsa_scatter == NULL) {
		cmn_err(CE_WARN, "%s: CH%d Cannot allocate s/g array",
			ha->ha_name, ha->ha_chan_num);
		return(-1);
	}

	for(i = 0, adsa_sg_next = SGNULL; i < nsg_pages; i++) {
		sg_page = (SGARRAY *)(void *)&adsa_scatter[i];

		for(j = 0; j < nsg_per_page; j++) {
			sg_p = &sg_page[j];
			sg_p->sg_flags = SG_FREE;
			sg_p->sg_next = adsa_sg_next;
			adsa_sg_next = sg_p;
		}
	}
	((SGARRAY *)(void *)adsa_scatter)->sg_next = (SGARRAY *)adsa_sg_next;

	ha->ha_sglist = (SGARRAY *)(void *)adsa_scatter;
	ha->ha_sglist_size = sg_listsize;
	ha->ha_sgnext = (SGARRAY *)adsa_sg_next;

	return(0);
}


/*
 * adsa_mem_release()
 *	This routine releases allocated memory back to the kernel based
 *	on the flag variable.
 */
STATIC void
adsa_mem_release(struct scsi_ha **hap, int flag)
{
	struct scsi_ha *ha = *hap;
	struct req_sense_def *req_sense = ha->ha_scb[0].virt_SensePtr;

	AHA_DTRACE;

	if(flag & HA_CONFIG_REL) {
		KMEM_FREE(ha->ha_config, adsa_ha_configsize);
		ha->ha_config = NULL;
		flag &= ~HA_CONFIG_REL;
	}
	if(flag & HA_DEV_REL) {
		KMEM_FREE(ha->ha_dev, adsa_luq_structsize);
		ha->ha_dev = NULL;
		flag &= ~HA_DEV_REL;
	}
	if(flag & HA_SG_REL) {
		KMEM_FREE(ha->ha_sglist, ha->ha_sglist_size);
		flag &= ~HA_SG_REL;
	}
	if(flag & HA_SCB_REL) {
		KMEM_FREE(ha->ha_scb, adsa_scb_structsize);
		ha->ha_scb = NULL;
		flag &= ~HA_SCB_REL;
	}
	if(flag & HA_REQS_REL) {
		KMEM_FREE(req_sense, adsa_req_structsize);
		flag &= ~HA_REQS_REL;
	}
	if(flag & HA_REL) {
		KMEM_FREE(ha, adsa_ha_size);
		*hap = CNFNULL;
		flag &= ~HA_REL;
	}
}

/**************************************************************************
** adsa_switch()							 **
**	This routine reverses the primary and secondary channels if the  **
**	PRI_CH_ID bit is set in the adapter config flags.		 **
**************************************************************************/
STATIC void
adsa_switch(int cnt)
{
	register int gb;
	register struct scsi_ha	*ha;
	struct him_config_block	*config, *first_config;

	AHA_DTRACE;

	for(gb = 0; gb < cnt; gb++) {
		ha = adsa_sc_ha[gb];
		config = ha->ha_config;
		if(ha->ha_chan_num == 0)
			first_config = config;
		else {
			if(config->HA_Config_Flags & PRI_CH_ID) {
				first_config->SCSI_Channel = B_CHANNEL;
#ifdef AHA_DEBUG2
	cmn_err(CE_CONT,"calling scb_getconfig\n");
#endif
				scb_getconfig(first_config);
				config->SCSI_Channel = A_CHANNEL;
#ifdef AHA_DEBUG2
	cmn_err(CE_CONT,"calling scb_getconfig again\n");
#endif
				scb_getconfig(config);
				adsa_sc_ha[gb-1]->ha_chan_num = 1;
				ha->ha_chan_num = 0;
			}
		}
	}
#ifdef AHA_DEBUG2
	cmn_err(CE_CONT,"returning from adsa_switch\n");
#endif
}

/*
 * STATIC void
 * adsa_lockinit(int c)
 *	Initialize adsa locks:
 *		1) device queue locks (one per queue),
 *		2) dmalists lock (one only),
 *		3) ccb lock (one only),
 *		4) scatter/gather lists lock (one only),
 *		5) and controller mailbox queue lock (one per controller).
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
adsa_lockinit(int c)
{
	register struct scsi_ha  *ha, *other_ha;
	register struct scsi_lu	 *q;
	register int	gb,t,l;
	int		sleepflag;
	static		firsttime = 1;

	AHA_DTRACE;

	sleepflag = adsa_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;

	if (firsttime) {

		adsa_dmalist_sv = SV_ALLOC (sleepflag);
		adsa_dmalist_lock = LOCK_ALLOC (ADSA_HIER, pldisk, &adsa_lkinfo_dmalist, sleepflag);
		firsttime = 0;
	}

	other_ha = CNFNULL;
	for (gb = 0; gb < adsa_habuscnt; gb++) {
		if (((ha = adsa_sc_ha[gb]) == CNFNULL) || (ha->ha_ctl != c))
			continue;
		ha->ha_npend_lock = LOCK_ALLOC (ADSA_HIER, pldisk, &adsa_lkinfo_npend, sleepflag);
		ha->ha_sg_lock = LOCK_ALLOC (ADSA_HIER+1, pldisk, &adsa_lkinfo_sgarray, sleepflag);
		ha->ha_scb_lock = LOCK_ALLOC (ADSA_HIER, pldisk, &adsa_lkinfo_scb, sleepflag);
		if ( other_ha ) {
			ha->ha_HIM_lock = other_ha->ha_HIM_lock;
		} else {
			ha->ha_HIM_lock =
				(adsa_lock_t *)kmem_zalloc(sizeof(adsa_lock_t), sleepflag);
			ha->ha_HIM_lock->al_HIM_lock =
				LOCK_ALLOC (ADSA_HIER, pldisk, &adsa_lkinfo_HIM, sleepflag);
			other_ha = ha;
		}
		for (t = 0; t < MAX_TCS; t++) {
		    for (l = 0; l < MAX_LUS; l++) {
			q = &LU_Q_GB (gb,t,l);
			q->q_lock = LOCK_ALLOC (ADSA_HIER, pldisk, &adsa_lkinfo_q, sleepflag);
		    }
		}
	}
}

/*
 * STATIC void
 * adsa_lockclean(int c)
 *	Removes unneeded locks.  Controllers that are not active will
 *	have all locks removed.  Active controllers will have locks for
 *	all non-existant devices removed.
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
adsa_lockclean(int c)
{
	register struct scsi_ha  *ha;
	register struct scsi_lu	 *q;
	register int	b,gb,t,l;
	static		firsttime = 1;

	AHA_DTRACE;

	if (firsttime && !adsa_habuscnt) {

		if (adsa_dmalist_sv == NULL)
			return;
		SV_DEALLOC (adsa_dmalist_sv);
		LOCK_DEALLOC (adsa_dmalist_lock);
		firsttime = 0;
	}

	for (gb = 0, b = 0; gb < adsa_habuscnt; gb++) {
		if (((ha = adsa_sc_ha[gb]) == NULL) || (ha->ha_ctl != c))
			continue;
		if( !adsaidata[c].active) {

			if (ha->ha_npend_lock) {
				LOCK_DEALLOC (ha->ha_npend_lock);
				LOCK_DEALLOC (ha->ha_sg_lock);
				LOCK_DEALLOC (ha->ha_scb_lock);
#ifdef NOT_YET
				LOCK_DEALLOC (ha->ha_HIM_lock);
#endif
			}
		}

		for (t = 0; t < MAX_TCS; t++) {
		    for (l = 0; l < MAX_LUS; l++) {
			if (!adsaidata[c].active || adsa_illegal(adsa_ltog[c],
			    b, t, l)) {
				q = &LU_Q_GB (gb,t,l);
				LOCK_DEALLOC (q->q_lock);
			}
		    }
		}
		b++;
	}
}

/*
 * STATIC void *
 * adsa_kmem_zalloc_physreq(size_t size, int flags)
 *	function to be used in place of kma_zalloc
 *	which allocates contiguous memory, using kmem_alloc_physreq,
 *	and zero's the memory.
 *
 * Calling/Exit State:
 *	None
 */
STATIC void *
adsa_kmem_zalloc_physreq(size_t size, int flags)
{
	void *mem;
	physreq_t *preq;

	AHA_DTRACE;
	preq = physreq_alloc(flags);
	if (preq == NULL)
		return NULL;
	preq->phys_align = ADSA_MEMALIGN;
	preq->phys_boundary = ADSA_BOUNDARY;
	preq->phys_dmasize = ADSA_DMASIZE;
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
