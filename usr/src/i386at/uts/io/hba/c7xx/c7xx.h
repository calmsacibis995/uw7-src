#ident	"@(#)kern-pdi:io/hba/c7xx/c7xx.h	1.1.2.1"

/***************************************************************************
 *                                                                         *
 * Copyright 1995 Symbios Logic Inc.  All rights reserved.                 *
 *                                                                         *
 * This file is confidential and a trade secret of Symbios Logic Inc.      *
 * The receipt of or possession of this file does not convey  any rights   *
 * to reproduce or disclose its contents or to manufacture, use, or sell   *
 * anything it may describe, in whole, or in part, without the specific    *
 * written consent of Symbios Logic Inc.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef C7XX_H	/*{*/
#define C7XX_H

/*
 * kmem parameters
 */
#define	KMEM_ZALLOC		c7xx_kmem_zalloc_physreq
#define	KMEM_FREE		kmem_free
#define	C7XX_MEMALIGN		512
#define	C7XX_BOUNDARY		0
#define	C7XX_DMASIZE		32

/*
 * HBA parameters 
 */
#define	C7XX_MAXXFER		0x20000	/* Max data xfer */
#define C7XX_CNTLR_ID		7	/* Default scsi id */

/*
 * Sense keys
 */
#define SENSE_LENGTH (sizeof(struct sense) - 1)		
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


/************************************************************************** 
 * CCB Definitions
 **************************************************************************/

/*
 * The chunks in which we grow the free ccb list. This ensures we allocate
 * one full page at a time.
 */
#define CCB_ALLOC_GROW	((ptob(1)) / sizeof(c7xx_ccb))

/*
 * Maximum number of ccb's to be allocated per adapter.
 */
#define MAX_ALLOC_CCB	300

/*
 * Length of a CCB 
 */
#define CCB_LENGTH	192

typedef struct c7xx_ccb
{
	int		sig;		/* Signature of the request	*/
	struct		c7xx_ccb *forw,
			*back;
	DEVPtr		dp;		/* pointer to the device	*/
	struct 		hbadata  *hbap;	/* Pointer to  the request.	*/
	CCBSCSIIORequest    c;  	/* CCB we use to do request 	*/
	uchar               pad[sizeof(SIMCCB)-sizeof(CCBSCSIIORequest)];
	SGListElement 	sgl[MAX_FRAG];
	struct 		c7xx_ccb *ccblist[MAX_FRAG];
	uchar		sgflag;
	uchar   	SaveCAMStatus;
	uchar   	SaveSCSIStatus;
	ulong   	SaveResidual;
	uchar   	SaveCamflags[4];
	uchar		SaveCDB[12];
	uchar 		SaveCDBLength;
	ulong 		SaveDataLength;
} c7xx_ccb, * c7xx_ccbptr;

typedef struct c7xx_srb {
	struct xsb     	*sbp;		/* Target drv definition of SB	*/
	struct proc	*procp;
} c7xx_srb;


/************************************************************************** 
 * Locking macros/definitions
 **************************************************************************/

/*
 * Locking Hierarchy Definition
 */
#define C7XX_HIER	HBA_HIER_BASE	/* Locking hierarchy base for hba */

/*
 * Synchronization macros.
 */
#define LOCK_CCB()	c7xx_ccb_opri = LOCK(c7xx_ccb_lock, pldisk)
#define UNLOCK_CCB()	UNLOCK(c7xx_ccb_lock, c7xx_ccb_opri)

/*
 * defines for pass-thru processing
 */
#define C7XX_BLKSIZE			512
#define C7XX_BLKSHFT			9
#define	MAX_CMDSZ			12
#define	C7XX_PASSTHRU_CDB_SIZE		(MAX_CMDSZ + sizeof(int))

/*
 * Queue flags
 */
#define	C7XX_QBUSY	0x01
#define	C7XX_QSUSP	0x04
#define	C7XX_QSENSE	0x08		/* Sense data cache valid */
#define	C7XX_QPTHRU	0x10
#define C7XX_QSCHED	0x20		/* Queue may be scheduled */
#define	C7XX_QREMOVED	0x40

/*
 * Some macros for the portable code.
 */
#define PGBND(a)        (ptob(1) - ((ptob(1) - 1) & (int)(a)))
#define HBAP_2_SDEVP(hp)	&(hp->sb->sb.SCB.sc_dev)
#define DELAYSEC(t)		drv_usecwait(t * 1000 * 1000)

extern int		c7xx_sleepflag;	/* Mem allocation flag.	*/
extern int 		c7xx_initflag;	/* initialization flag. */
extern int		c7xx_waitflag;	/* polled i/o flag */

/************************************************************************** 
 * Function Prototypes
 **************************************************************************/

/* Hardware */
int	c7xx_sfb_match(HBA_IDATA_STRUCT *idp,int count,HBA_IDATA_STRUCT sfb[]);
int	c7xx_sfb_scan( HBA_IDATA_STRUCT idp[]);
int	c7xx_sni_match(HBA_IDATA_STRUCT *idp,int count,HBA_IDATA_STRUCT sni[]);
int	c7xx_sni_scan( HBA_IDATA_STRUCT idp[]);
ushort	c7xx_mca_board_id( rm_key_t key);
int	c7xx_mca_conf( HBA_IDATA_STRUCT *idp, int *ioalen, int *memalen);
#ifdef	C7XX_DEBUG
void	c7xx_idata_sort(void);
#endif	
int 	c7xx_bus_reset(int path);

/* Device Queue Utils */
void	c7xx_pause_func( DEVPtr dp);
void 	c7xx_resumeq(int path, int id, int lun);
void 	c7xx_suspendq(int path, int id, int lun);
void	c7xx_pauseq( int path, int id, int lun);
void 	c7xx_continueq(int path, int id, int lun);
void    c7xx_releaseSIMQueue( c7xx_ccb * cp );

/* CCB Utils */
int	c7xx_grow_freeccb( int sleepflag);
c7xx_ccb *c7xx_allocccb( int sleepflag);
void 	c7xx_freeccb(c7xx_ccb * cp);
void 	c7xx_saveCCB(CCBSCSIIORequest *rp);
void 	c7xx_restoreCCB( c7xx_ccb * cp);
void 	c7xx_hbap2ccb(c7xx_ccb * ccb, struct hbadata *hp);

/* I/O Completion */
void 	c7xx_done(CCBSCSIIORequest *rp);
void 	c7xx_complete_sense(CCBSCSIIORequest *rp);
void 	c7xx_iocompleted(CCBSCSIIORequest *rp);
void 	c7xx_wait_for_hbap_completion(struct hbadata *hbap,int path);

/* Memory/Lock Allocations */
void 	c7xx_lockinit(int c);
void 	c7xx_lockfree(int c, int found);
void	c7xx_mem_free( void);
void 	*c7xx_kmem_zalloc_physreq (size_t size, size_t align, int sleepflag);

/* Pass-Thru Utilities */
void 	c7xx_pass_thru0(struct buf *bp);
void 	c7xx_pass_thru(struct buf *bp);
void 	c7xx_int(struct sb *sp);
int	c7xx_sdi_send( dev_t devp, caddr_t arg);

/* Misc Utilities */
int	c7xx_illegal( int c, int t, int l, int b);

/* Entry Points */
int	c7xx_load(void);
int	c7xx_unload(void);
int 	c7xxinit(void);
int 	c7xxstart(void);
int	c7xxverify(rm_key_t key);
void 	c7xxintr(unsigned int vect);
int	HBAOPEN( dev_t *devp, int flags, int otype, cred_t *cred_p);
int	HBACLOSE( dev_t dev, int flags, int otype, cred_t *cred_p);
int	HBAIOCTL(dev_t dev,int cmd,caddr_t arg,int mode,cred_t *cred_p,int *rval_p);
long	HBASEND( struct hbadata *hbap, int sleepflag);
long	HBAICMD( struct hbadata *hbap, int sleepflag);
HBAXLAT_DECL HBAXLAT(struct hbadata *hbap, int flag, struct proc *procp, int sleepflag);
struct	hbadata *HBAGETBLK(int sleepflag);
long	HBAFREEBLK(struct hbadata *);
void	HBAGETINFO(struct scsi_ad *sa, struct hbagetinfo *getinfop);

/*
 * Tuning parameters in Space.c
 */
extern  ulong	c7xx_MAX_DLP;
extern	int	c7xx_QDEPTH;
extern	char	c7xx_WIDE;
extern	char	c7xx_SORT;
extern	ulong	c7xx_MaxBlocks;
extern	char	c7xx_CONCAT;
extern	int	c7xx_ASYNC;
extern	int	c7xx_DHP;
extern	ulong	c7xx_Total_conc_reqs;

#endif	/*}*/
