#ifndef _IO_HBA_MCIS_MCIS_H	/* wrapper symbol for kernel use */
#define _IO_HBA_MCIS_MCIS_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/mcis/mcis.h	1.16"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

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

#ifdef _KERNEL_HEADERS

#include <io/target/scsi.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/scsi.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

#define HBA_PREFIX	mcis	/* needed to fake of macro defs in hba.h */

extern int		mcis_init();
extern int		mcis_open();
extern int		mcis_close();
extern void		mcis_getinfo();
extern int		mcis_xlat();
extern int		mcis_ioctl();
extern void		mcis_intr();
extern struct hbadata	*mcis_getblk();
extern long		mcis_freeblk();
extern long		mcis_send();
extern long		mcis_icmd();

#pragma pack(1)

/*
** Reference Manual Used:
**
** IBM Personal System/2 Micro Channel SCSI Adapter
** with Cache Technical Reference (March 1990)
*/

/*
** A pool of these struct's is kept and allocated to the sdi module
** via a call to mcis_getblk() and returned via mcis_freeblk().
*/

struct mcis_srb 
{
	struct xsb	*s_sbp;
	struct mcis_srb	*s_next;	/* used for free list and queue */
	struct mcis_srb	*s_priv;	/* Private ptr for dynamic alloc */
					/* routines DON'T USE OR MODIFY */
	struct mcis_srb	*s_prev;	/* used to maintain queue       */
	paddr_t		s_addr;		/* xlated sc_datapt		*/
	int		s_size;
	struct proc	*s_proc;
};

/*
** This struct will contain the termination status information for a
** completed scsi command.  It is only populated if an error is detected
** or the s_es bit is set to 0 in the mcis_scb struct.
**
** This stuct is hardware specific, it is only used within struct mcis_scb.
*/

struct mcis_tsb
{
	ushort_t	t_status;	/* end status word		 */
	ushort_t	t_rtycnt;	/* retry count			 */
	uint_t		t_resid;	/* residual byte count		 */
	paddr_t		t_datapt;	/* scatter gather list addr	 */
	ushort_t	t_targlen;	/* target status length followed */
	unsigned	t_vres2    : 1;	/* vendor unique bit		 */
	unsigned	t_targstat : 4; /* target status code		 */
	unsigned	t_vres1    : 3;	/* vendor unique bit		 */
	uchar_t		t_hastat;	/* hba status			 */
	uchar_t		t_targerr;	/* target error code		 */
	uchar_t		t_haerr;	/* hba error code		 */
	ushort_t	t_res;		/* reserved			 */
	uchar_t		t_crate;	/* cache hit ratio		 */
	uchar_t		t_cstat;	/* cache status			 */
	paddr_t		t_mscb;		/* last processed scb addr	 */
};

/*
** This struct is used for scatter/gather list.
**
** It is only used within struct mcis_scb.
*/

struct mcis_dma
{
	paddr_t		d_addr;		/* dma buffer address		*/
	int		d_cnt;		/* buffer size			*/
};

#define	MAXDSEG	16

struct mcis_luq
{
	struct mcis_srb	*q_first;
	struct mcis_srb	*q_last;
	struct mcis_scb	*q_scbp;
	ushort_t	q_count;
	uchar_t		q_flag;
	uchar_t		q_retries;
	bcb_t		*q_bcbp;
	int		q_shift;
	unsigned int	q_addr;
	char		*q_sc_cmd;
};

#define MCIS_LU_Q( c, t, l )	(&(mcis_ha[c]->b_scb_array[(t << 3) | l ]->s_luq))
#define MCIS_QLUBUSY	0x01
#define MCIS_QSUSP	0x04
#define MCIS_QSENSE	0x08		/* Sense data cache valid */
#define MCIS_QPTHRU	0x10
#define MCIS_QSCHED	0x20

#define MCIS_QCLASS(x)	((x)->s_sbp->sb.sb_type)

#define MCIS_BB		0x02
#define MCIS_SS		0x04
#define MCIS_PT		0x10
#define MCIS_RE		0x20
#define MCIS_ES		0x40
#define MCIS_RD		0x80

/*
** This struct is used for sending commands to the host adapter.
** It is hardware dependent.  There is one per LU. 
*/

struct mcis_scb
{
	unsigned	s_cmdop	: 6;
	unsigned	s_ns	: 1;
	unsigned	s_nd	: 1;
	uchar_t		s_cmdsup;
	uchar_t		s_ch;
	uchar_t		s_opfld;

	daddr_t		s_baddr;	/* logical block address	*/
	paddr_t		s_hostaddr;	/* system address		*/
	uint_t		s_hostcnt;	/* system buffer count		*/
	paddr_t 	s_tsbp;		/* tsb address			*/
	paddr_t 	s_link;		/* link pointer			*/
	uchar_t		s_cdb[12];	/* scsi cmd descriptor block	*/

	/*
	** The data above here is sent to the host adapter.  The balance
	** is just piggy backed here for convenience.
	*/

	struct mcis_dma *s_dmaseg;	/* pointer to scatter gather list*/
	struct mcis_tsb s_tsb;		/* termination status block	*/

	/* driver's specific data	*/

	uchar_t		s_targ;		/* target number		*/
	uchar_t		s_lun;		/* lun				*/
	struct mcis_srb	*s_srbp;	/* ptr to active request	*/
	struct mcis_blk	*s_blkp;	/* owner pointer		*/
	struct mcis_ldevmap *s_ldmap;	/* logical device map entry	*/
	struct mcis_luq	s_luq;		/* q of requests		*/
	uchar_t		s_status;	/* status			*/
	struct ident	s_ident;	/* inquiry data from device	*/
};

/*
 * Read data scb
 */
struct mcis_rdscb
{
	unsigned	s_cmdop	: 6;
	unsigned	s_ns	: 1;
	unsigned	s_nd	: 1;
	uchar_t		s_cmdsup;
	uchar_t		s_ch;
	uchar_t		s_opfld;

	daddr_t		s_baddr;	/* logical block address	*/
	paddr_t		s_hostaddr;	/* system address		*/
	uint_t		s_hostcnt;	/* system buffer count		*/
	paddr_t 	s_tsbp;		/* tsb address			*/
	paddr_t 	s_link;		/* link pointer			*/
	unsigned short	s_blkcnt;	/* Block Count			*/
	unsigned short	s_blklen;	/* Block Length			*/
	uchar_t		s_pad[8];	/* not used for READ		*/

	/*
	** The data above here is sent to the host adapter.  The balance
	** is just piggy backed here for convenience.
	*/

	struct mcis_dma *s_dmaseg;	/* pointer to scatter gather list*/
	struct mcis_tsb s_tsb;		/* termination status block	*/

	/* driver's specific data	*/

	uchar_t		s_targ;		/* target number		*/
	uchar_t		s_lun;		/* lun				*/
	struct mcis_srb	*s_srbp;	/* ptr to active request	*/
	struct mcis_blk	*s_blkp;	/* owner pointer		*/
	struct mcis_ldevmap *s_ldmap;	/* logical device map entry	*/
	struct mcis_luq	s_luq;		/* q of requests		*/
	uchar_t		s_status;	/* status			*/
	struct ident	s_ident;	/* inquiry data from device	*/
};

/*
** This represents the feature control SCB 
**/
struct mcis_fcscb	
{
	uchar_t		fc_cmdop;
	uchar_t		fc_cmdsup;
	ushort_t	fc_gtv   : 13;		/* Global time value */
	ushort_t	fc_drate : 3;		/* Transfer rate */
};

/*	defines for s_status flag					*/

#define MCIS_OWNLD	0x01		/* own's a logical device 	*/
#define MCIS_WAITLD	0x02		/* waiting for a ld assignment	*/
#define MCIS_GOTLD	0x04		/* got a ld assignment		*/

/*
** This represents the Basic Control Register
*/

struct mcis_rctl
{
	unsigned	rc_eintr : 1;	/* enable interrupt		*/
	unsigned	rc_edma  : 1;	/* enable dma			*/
	unsigned	rc_res   : 5;	/* reserved			*/
	unsigned	rc_reset : 1;	/* hw reset			*/
};

/*
** This represents the Interrupt Status Register
*/

struct mcis_rintr
{
	unsigned	ri_ldevid : 4;	/* logical device id		*/
	unsigned	ri_code   : 4;	/* interrupt id	code		*/
};

#define	MCIS_rctlp(X)	((struct mcis_rctl *)(X))
#define	MCIS_rintrp(X)	((struct mcis_rintr *)(X))

/*
** This stuct gets information in response to a Get POS Info command.
**
** This struct is hardware dependent.
*/

struct mcis_pos
{
	ushort_t	p_hbaid;	/* hba id			*/
	unsigned	p3_dma    : 4;	/* pos reg 3 - arbitration level*/
	unsigned	p3_fair   : 1;	/* pos reg 3 - fairness		*/
	unsigned	p3_targid : 3;	/* pos reg 3 - hba target id	*/
	uchar_t		p2_ehba   : 1;	/* pos reg 2 - hba enable	*/
	uchar_t		p2_ioaddr : 3;	/* pos reg 2 - ioaddr		*/
	uchar_t		p2_romseg : 4;	/* pos reg 2 - rom addr		*/
	uchar_t		p_intr;		/* interrupt level		*/
	uchar_t		p_pos4;		/* pos reg 4			*/
	unsigned	p_rev	: 12;	/* revision level		*/
	unsigned	p_slotsz: 4;	/* 16 or 32 slot size		*/
	uchar_t		p_luncnt;	/* # of lun per target		*/
	uchar_t		p_targcnt;	/* # of targets			*/
	uchar_t		p_pacing;	/* dma pacing factor		*/
	uchar_t		p_ldcnt;	/* number of logical device	*/
	uchar_t		p_tmeoi;	/* time from eoi to intr off	*/
	uchar_t		p_tmreset;	/* time from reset to busy off	*/
	ushort_t	p_cache;	/* cache status			*/
	ushort_t	p_retry;	/* retry status			*/
};

/*
** This struct is used to create an Assign scsi command.  It is hardware
** dependent and is only used within struct mcis_assign.
*/

struct mcis_asgncmd
{
	ushort_t	a_cmdop;	/* command opcode		*/
	uint_t		a_ld   : 4;	/* logical device		*/
	uint_t		a_targ : 3;	/* target number		*/
	uint_t		a_rm   : 1;	/* remove assignment		*/
	uint_t		a_lun  : 3;	/* lun number			*/
	uint_t		a_resv : 5;	/* reserved			*/
};

/*
** This is used to send an Assign command.  It is only used within
** struct mcis_ldevmap.
*/

union mcis_assign
{
	struct mcis_asgncmd	a_blk;
	uint_t			a_cmd;
};

#pragma pack()

/* Register offsets from base address */

#define	MCIS_CMD	0x0
#define	MCIS_ATTN	0x4
#define	MCIS_CTRL	0x5
#define	MCIS_INTR	0x6
#define	MCIS_STAT	0x7

/* Attention register request codes */

#define	ISATTN_ICMD	0x10	/* start immediate command		*/
#define	ISATTN_SCB	0x30	/* start scb 				*/
#define	ISATTN_LSCB	0x40	/* start long scb 			*/
#define ISATTN_EOI	0xE0	/* end of interrupt			*/

#define MCIS_SENDEOI(X,LD)	(outb((X)+MCIS_ATTN, ISATTN_EOI | (LD)))

/* Basic control register */

#define	ISCTRL_RESET	0x80	/* hardware reset			*/
#define ISCTRL_RESERVE	0x7C	/* reserved bits			*/
#define	ISCTRL_EDMA	0x02	/* dma enable				*/
#define	ISCTRL_EINTR	0x01	/* enable interrupt			*/

/* basic status register */

#define ISSTAT_CMDFULL	0x8	/* command interface reg full		*/
#define ISSTAT_CMD0	0x4	/* command interface reg empty		*/
#define ISSTAT_INTRHERE 0x2	/* interrupt request active		*/
#define ISSTAT_BUSY	0x1	/* busy					*/

#define MCIS_BUSYWAIT(X)	mcis_wait((X), ISSTAT_BUSY, 0, ISSTAT_BUSY)

#define MCIS_INTRWAIT(X)	\
	mcis_wait((X), ISSTAT_INTRHERE, ISSTAT_INTRHERE, 0)

#define MCIS_CMDOUTWAIT(X)	\
	mcis_wait((X), ISSTAT_CMD0|ISSTAT_BUSY, ISSTAT_CMD0, ISSTAT_BUSY)


/* interrupt id */

#define ISINTR_SCB_OK	0x1	/* scb completed successfully		*/
#define ISINTR_SCB_OK2	0x5	/* scb completed ok after retry 	*/
#define ISINTR_HBA_FAIL	0x7	/* hba hw failed			*/
#define ISINTR_ICMD_OK	0xA	/* immediate command completed ok	*/
#define ISINTR_CMD_FAIL	0xC	/* cmd completed with failure		*/
#define ISINTR_CMD_INV	0xE	/* invalid command			*/
#define ISINTR_SEQ_ERR	0xF	/* sw sequencing error			*/

/* command opcode */

#define ISCMD_ABORT	0x0F	/* command abort			*/
#define ISCMD_ASSIGN	0x0E	/* logical device assignment		*/
#define ISCMD_DEVINQ	0x0B	/* device inquiry			*/
#define ISCMD_DMACTRL	0x0D	/* dma pacing control			*/
#define ISCMD_HBACTRL	0x0C	/* feature control			*/
#define ISCMD_FMTPREP	0x17	/* format prepare			*/
#define ISCMD_FMTUNIT	0x16	/* format unit				*/
#define ISCMD_CMDSTAT	0x07	/* get command complete status		*/
#define ISCMD_GETPOS	0x0A	/* get pos and adapter information	*/
#define ISCMD_READ	0x01	/* read data				*/
#define ISCMD_READCAP	0x09	/* read capacity			*/
#define ISCMD_PREREAD	0x31	/* read prefetch			*/
#define ISCMD_READVFY	0x03	/* read verify				*/
#define ISCMD_REASSIGN	0x18	/* reassign block			*/
#define ISCMD_REQSEN	0x08	/* request sense			*/
#define ISCMD_RESET	0x00	/* scsi (soft) reset			*/
#define ISCMD_SNDSCSI	0x1F	/* send scsi command			*/
#define ISCMD_WRITE	0x02	/* write data				*/
#define ISCMD_WRITEVFY	0x04	/* write verify				*/

#define ISCMD_ICMD	0x04	/* supplement code			*/
#define	ISCMD_SCB	0x1C	/* start scb 				*/
#define	ISCMD_LSCB	0x24	/* start long scb 			*/

/*
** This struct is used to maintain the LU to dev mapping for the
** host adapter.
*/

struct mcis_ldevmap
{
	struct mcis_ldevmap	*ld_avfw;	/* avail forward	*/
	struct mcis_ldevmap	*ld_avbk;	/* avail backward	*/
	struct mcis_scb		*ld_scbp;	/* scb ptr		*/
	union mcis_assign	ld_cmd;		/* ld assign command	*/
};

#define	LD_CMD	ld_cmd.a_cmd
#define	LD_CB	ld_cmd.a_blk

#define MCIS_MAXLD	16
#define MCIS_HBALD	0x0F
#define MAX_DEVICES	64

struct  mcis_blk
{
	struct mcis_ldevmap	b_ldmhd;
	uchar_t			b_boottarg;
	uchar_t			b_intr_code;
	uchar_t			b_intr_dev;
	uchar_t			b_numdev;
	ushort_t		b_scballoc;
	ushort_t		b_targetid;
	ushort_t		b_ioaddr;
	ushort_t		b_dmachan;
	ushort_t		b_intr;
	ushort_t		b_scb_cnt;
	int			b_npend;
	uchar_t			b_ldcnt;
	struct mcis_scb 	*b_scbp;	/* only used during init */
	struct mcis_scb 	*b_scb_array[ MAX_DEVICES ];
	struct mcis_ldevmap	b_ldm[ MCIS_MAXLD ];
};

#define DCB_MCIS_BLKP(X)    	((struct mcis_blk *)((X)->dcb_hba_private))
#define LDMHD_FW(X)		((X)->b_ldmhd.ld_avfw)
#define LDMHD_BK(X)		((X)->b_ldmhd.ld_avbk)

#define MCIS_HAN(minor)		(((minor) >> 5) & 0x07)
#define MCIS_TCN(minor)		(((minor) >> 2) & 0x07)
#define MCIS_LUN(minor)		((minor) & 0x03)

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_MCIS_MCIS_H */
