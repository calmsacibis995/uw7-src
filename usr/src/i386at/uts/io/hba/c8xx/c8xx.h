#ident	"@(#)kern-pdi:io/hba/c8xx/c8xx.h	1.1.4.3"

/*
 **************************************************************************
 *                                                                         *
 * Copyright 1995 Symbios Logic Inc.  All rights reserved.                 *
 *                                                                         *
 * This file is confidential and a trade secret of Symbios Logic Inc.      *
 * The receipt of or possession of this file does not convey  any rights   *
 * to reproduce or disclose its contents or to manufacture, use, or sell   *
 * anything it may describe, in whole, or in part, without the specific    *
 * written consent of Symbios Logic Inc.                                   *
 *                                                                         *
 **************************************************************************
 */

#ifndef C8XX_H
#define C8XX_H

#define HBA_PREFIX c8xx
#include <sys/sdi.h>
#include <sys/hba.h>
#ifdef	_KERNEL_HEADERS

#include <io/target/scsi.h>		/* REQUIRED */
#include <util/types.h>			/* REQUIRED */
#if (PDI_VERSION > 1)
#include <io/target/sdi/sdi_hier.h>	/* REQUIRED */
#endif /* (PDI_VERSION > 1) */

#elif defined(_KERNEL)

#include <sys/scsi.h>			/* REQUIRED */
#include <sys/types.h>			/* REQUIRED */
#if (PDI_VERSION > 1)
#include <sys/sdi_hier.h>		/* REQUIRED */
#endif /* (PDI_VERSION > 1) */

#endif	/* _KERNEL_HEADERS */



#define 	KMEM_ZALLOC c8xx_kmem_zalloc_physreq
#define 	KMEM_FREE kmem_free
#define 	C8XX_MEMALIGN	0x100
#define 	C8XX_BOUNDARY	0
#define 	UNIXWARE_VERSION "2.XX"


#define	C8XX_MAXXFER	0x20000

#define	MAX_CMDSZ	12
#define MAX_FRAG 	16
#define C8XX_POLL_TIME	18000		/* Time to poll in msecs	*/
#define C8XX_QUEUE_FULL_DELAY	2000	/* Delay for queue full msg.	*/

#define DRIVER_VERSION "40201"

#include "c8xxalias.h"
#include "sim.h"
/*
 * Values for scatter/gather flags.
 */
#define ALLOW_SCATTER_GATHER	0
#define DISALLOW_SCATTER_GATHER 1
#define SDMS_DSK	0x5f44534b		/* "_DSK" */

#define SENSE_LENGTH 18		

#define SENSE_NO_SENSE          0X00
#define SENSE_RECOVERED_ERROR   0X01
#define SENSE_NOT_READY         0X02
#define SENSE_MEDIUM_ERROR      0X03
#define SENSE_HARDWARE_ERROR    0X04
#define SENSE_ILLEGAL_REQUEST   0X05
#define SENSE_UNIT_ATTENTION    0X06
#define SENSE_DATA_PROTECT      0X07
#define SENSE_BLANK_CHECK       0X08
#define SENSE_VENDOR_UNIQUE     0X09
#define SENSE_COPY_ABORTED      0X0A
#define SENSE_ABORTED_COMMAND   0X0B
#define SENSE_EQUAL             0X0C
#define SENSE_VOLUME_OVERFLOW   0X0D
#define SENSE_MISCOMPARE        0X0E

#define MAX_CD_RETRIES 6

#define PCI_CONFIG_SIZE	64
#define MAX_DEVID_LEN   11
typedef struct c8xx_pci_idata
{
	int	valid;			/* -1 if not used or valid */
	int	brdid_match;		/* Flag to show board
					   id match or not */
	int	brdid;			/* integer board id */
	union
	{
		uchar	config[PCI_CONFIG_SIZE]; /* pci config space  */
		ulong	configl[PCI_CONFIG_SIZE/4];
	} pci;
	char	resmgr_devid[MAX_DEVID_LEN];
} c8xx_pci_idata_t;

extern c8xx_pci_idata_t  c8xx_pci_idata[];
	

typedef struct c8xx_srb {
	struct xsb     	*sbp;		/* Target drv definition of SB	*/
	struct proc	*procp;
} c8xx_srb;

#define HBAP_2_SDEVP(hp)	&(hp->sb->sb.SCB.sc_dev)
#define CCB_LENGTH	192

/*
 * Locking Hierarchy Definition
 */
#define C8XX_HIER	HBA_HIER_BASE	/* Locking hierarchy base for hba */

/*
 * Synchronization macros.
 */
#define LOCK_CCB()	c8xx_ccb_opri = LOCK(c8xx_ccb_lock, pldisk)
#define UNLOCK_CCB()	UNLOCK(c8xx_ccb_lock, c8xx_ccb_opri)

#define C8XX_BLKSIZE	512
#define C8XX_BLKSHFT	9
#define	C8XX_QBUSY	0x01
#define	C8XX_QSUSP	0x04
#define	C8XX_QSENSE	0x08		/* Sense data cache valid */
#define	C8XX_QPTHRU	0x10
#define C8XX_QSCHED	0x20		/* Queue may be scheduled */
#define	C8XX_QREMOVED	0x40


/*
 * Some macros for the portable code.
 */
#define DisableInterrupts	spldisk
#define EnableInterrupts	spl0
#define RestoreInterrupts(s)	splx(s)
/*
#define DisableInterrupts()	NULL
#define EnableInterrupts()	NULL
#define RestoreInterrupts(s)	
 */

#if PDI_VERSION >= PDI_SVR42MP
#ifdef PUTBUF
#define PutString(str)		cmn_err(CE_CONT, "!C8XX:%s", str)
#define PutChar(chr)		cmn_err(CE_CONT, "!C8XX:%c", chr)
#define PrintNexus(p, i, l)	cmn_err( CE_CONT, "!C8XX:PATH %d, ID %d, LUN %d ", p, i, l)
#else /*PUTBUF*/
#define PutString(str)		
#define PutChar(chr)	
#define PrintNexus(p, i, l)
#endif /*PUTBUF*/
#else
#define PutString(str)		cmn_err(CE_CONT, "%s", str)
#define PutChar(chr)		cmn_err(CE_CONT, "%c", chr)
#define PrintNexus(p, i, l)	cmn_err(CE_CONT, "PATH %d, ID %d, LUN %d ", p, i, l)
#endif	/* PDI_VERSION >= PDI_SVR42MP */
#define DelaySec(t)		drv_usecwait(t * 1000 * 1000)

extern int		sleepflag;	/* Mem allocation flag.	*/
extern int		DEBUG_PRINT;
extern int 		c8xx_initflag;	/* initialization flag. */
/*
 * Function prototypes.
 */
void		c8xx_xsimptr( SIMCCBPtr simptr);
int 		c8xxinit();
int 		c8xxstart();
void 		c8xxhalt();
void 		c8xxintr(unsigned int vect);
void 		c8xx_done(SIMCCB *rp);
void 		c8xx_complete_sense(SIMCCB *rp);
void    	c8xx_releaseSIMQueue( SIMCCB * cp );
 
int 		c8xx_bus_reset(int path);
long 		c8xx_hbap2ccb(struct hbadata *hp, ulong flags);
void 		c8xx_lockinit(int c);
void 		c8xx_lockfree(int c, int found);
int 		c8xx_path_exists (int path);
void 		c8xx_iocompleted(SIMCCB *rp);
void 		c8xx_restoreCCB( SIMCCB * cp);
void 		c8xx_saveCCB(SIMCCB *rp);
void		c8xx_wait_for_hbap_completion(struct hbadata * hbap, int path, DEVPtr dp, int poll_time);
void		c8xx_mem_free();
void 		c8xx_pass_thru(struct buf *bp);
void 		c8xx_int(struct sb *sp);
#if PDI_VERSION >= PDI_SVR42MP
void 		*c8xx_kmem_zalloc_physreq (size_t size, int flags);
void 		c8xx_pass_thru0(struct buf *bp);
#endif /* PDI_VERSION >= PDI_SVR42MP */
long		c8xx_atol(char *string, int length);
extern   SIMCCBPtr       xpt_ccb_alloc( void );
extern	long	XPTAction(SIMCCBPtr rp);


extern	int	c8xx_QDEPTH;
extern	ulong	c8xx_Total_conc_reqs;

#endif
