#ifndef _IO_HBA_ICTHA_ICTHA_H
#define _IO_HBA_ICTHA_ICTHA_H

#ident	"@(#)kern-pdi:io/hba/ictha/ictha.h	1.8"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#define HBA_PREFIX	ictha
#define MAX_ICTHA_DRIVES 	1

#define ICTHA_ERASE_CMD		1
#define ICTHA_FLMRK_CMD		2
#define ICTHA_LOAD_CMD		3
#define ICTHA_MSENSE_CMD	4
#define ICTHA_READ_CMD		5
#define ICTHA_RELEASE_CMD	6
#define ICTHA_RELES_CMD		7
#define ICTHA_REQSEN_CMD	8
#define ICTHA_RESETM_CMD	9
#define ICTHA_RETENSION_CMD	10
#define ICTHA_REWIND_CMD	11
#define ICTHA_SPACE_CMD		12
#define ICTHA_WRITE_CMD		13	

/***
** Supported Controller Boards
***/
#define	ICTHA_WANGTEK		0
#define	ICTHA_ARCHIVE		1
#define	ICTHA_MCA_ARCHIVE	2

/***                                  ****
** Common Control Port Bit Definitions **
****                                  ***/

#define	ICTHA_ONLINE	0x01
#define	ICTHA_DMA1_2	0x08

/*
 * Wangtek Status Port Bit Definitions 
 */
#define  WANGTEK_READY		0x01
#define  WANGTEK_EXCEPTION	0x02

/*
 * Wangtek Control Port Bit Definitions 
 */
#define  WANGTEK_RESET		0x02
#define  WANGTEK_REQUEST	0x04
#define  WANGTEK_INTENAB	0x08

/*
 * Archive Control Port Bit Definitions 
 */
#define  ARCHIVE_RESET		0x80
#define  ARCHIVE_REQUEST	0x40
#define  ARCHIVE_INTENAB	0x20

/*
 * Archive Status Port Bit Definitions 
 */
#define  ARCHIVE_READY		0x40
#define  ARCHIVE_EXCEPTION	0x20

/*
 * Archive MCA Control Port Bit Definitions 
 */
#define ARCHIVE_MCA_INTENAB 	0x48
#define ARCHIVE_MCA_DMAENAB 	0x09

/***          ****
** QIC Commands **
****          ***/
#define ICTHA_SELECT0	0x01
#define ICTHA_SELECT1	0x02
#define ICTHA_SELECT2	0x04
#define ICTHA_REWIND	0x21
#define ICTHA_ERASE	0x22
#define	ICTHA_RETENSION	0x24
#define ICTHA_SEL24	0x27
#define ICTHA_SEL120	0x28
#define ICTHA_SEL150	0x29
#define ICTHA_WRITE	0x40
#define ICTHA_WRFILEM	0x60
#define ICTHA_READ	0x80
#define ICTHA_RDFILEM	0xA0
#define ICTHA_RD_STATUS	0xC0

/*
 * Emulated Commands 
 */
#define	ICTHA_MSENSE	0x100
#define	ICTHA_REQSEN	0x101
#define	ICTHA_RESETM	0x102

#define	ICTHA_ERROR		(-1)
#define	ICTHA_EXCEPTION		1

#define	ICTHA_POR		0x01
#define	ICTHA_SBYTE1		0x80

#define	ICTHA_FAILURE		(-1)
#define ICTHA_SUCCESS		0
#define	ICTHA_INPROGRESS	1

#define	ICTHA_STATBUF_SZ	6

#define ICTHA_WAIT_POLL_LIMIT	0x100000
#define ICTHA_INIT_WAIT_LIMIT	500000

#define ICTHA_WR_MAXTOUT	90	/* Maximum seconds to wait for write */
					/* interrupt when at load point	*/
					/* (worst case known is WANGTEK PC02) */


#define ICTHA_STATUSBUF_SZ 	6
#define ICTHA_BLOCKSIZE 	512

#define	ICTHA_HBA_TARGET	7
#define	ICTHA_DEVICE_TARGET	0

#define	ICTHA_NOT_READY	5

#define	ICTHA_DMA_READ	0x45
#define	ICTHA_DMA_WRITE	0x49

/* Controller Exceptions */
#define	ICTHA_NCT	1		/* No Cartridge */
#define	ICTHA_EOF	2		/* Read a Filemark */
#define	ICTHA_EOM	3		/* End of Media */
#define	ICTHA_WRP	4		/* Write Protected */
#define	ICTHA_DFF	5		/* Device Fault Flag */
#define	ICTHA_RWA	6		/* Read or Write Abort */
#define	ICTHA_BBX	7		/* Read Error, Bad Block Xfer */
#define	ICTHA_FBX	8		/* Read Error, Filler Block Xfer */
#define	ICTHA_NDT	9		/* Read Error, No Data */
#define	ICTHA_NDE	10		/* Read Error, No Data & EOM */
#define	ICTHA_ILC	11		/* Illegal Command */
#define	ICTHA_PRR	12		/* Power On/Reset */
#define	ICTHA_MBD	13		/* Marginal Block Detected */
#define	ICTHA_UND	14		/* Undetermined Error */

#define ICTHA_MAX_XFER	(132*1024) /* sdi_register will decrement by 4K */
#define	SCM_RAD(x)	((char *) x - 2)

/*
 * Misc defines 
 */
#define	ICTHA_FILEMARKS	0x01		/* Space file marks		*/
#define ICTHA_UNLOAD	0x00		/* Medium to be unloaded	*/
#define	ICTHA_LOAD	0x01		/* Medium to be loaded		*/
#define ICTHA_RETENSN	0x03		/* Retension (and load) tape	*/

/*
 * The following is the "struct hbadata" for the ICTHA hba. 
 */
struct  ictha_xsb {
	struct 	sb 	*sb;
	struct	proc   	*procp;
	struct  ictha_xsb *next;
	};
/*
 * ictha_block is used for memory breakup of requests that
 * are not block aligned.
 */
struct  ictha_block {
	caddr_t	blk_vaddr1; /* virtual addr of the block */
	paddr_t	blk_paddr;  /* the physical addr of the block
			     * if it is all within the same page, otherwise
			     * this is the address of drv_aligned_block
			     */
	char	blk_copyflag;/* Is a fragmented block? */
	};


struct	ictha_req {
	long	req_count; /* total count in blocks for read/write
			    * and the number of filemarks for
			    * read/write file marks. */

	long	req_resid;  /* # of blocks not read/written yet */
	caddr_t	req_addr;   /* virtual address of the request */
	struct  ictha_block *req_block; /* blocks of request */
	struct  ictha_block *req_blkptr; /* current block being read/written */
	struct	sb *	req_sb; /* the sb associated with the current request */
	};

typedef struct ictha_drive	*DrivePtr;
typedef struct ictha_ctrl	*CtrlPtr;

struct ictha_drive {
	struct ictha_req	drv_req;
	struct  ictha_block 	*drv_aligned_block; /* used for copying
						     * in/out non-aligned blocks
						     */
	struct ident	        drv_inqdata;
	int			drv_state;
	int			drv_sleepflag;
	int			drv_flags;
	int			drv_drverror;
	int			drv_drvcmd;
	int			drv_savecmd;
	int			drv_exception;
	int			drv_at_filemark;
	int			drv_at_eom;
	int			drv_at_bot;
	int			drv_nocartridge;
	int			drv_max_xfer_blks;
	ulong			drv_cmds_sent;
	ulong			drv_cmds_ack;
	void			(*drv_timeout)();
	int			drv_timeid;
	ulong			drv_duration;
	caddr_t			drv_dmavbuf;
	paddr_t			drv_dmapbuf;
	clock_t			drv_starttime;
	struct	proc   		*drv_procp;
	struct ictha_ctrl	*drv_ctrlptr;
	struct  ictha_xsb 	*drv_headque;
	struct  ictha_xsb 	*drv_tailque;
	int			drv_busy;
	};

/* drv_state */

#define	 DSTA_IDLE		0x00 /* idle state */
#define	 DSTA_NORMIO		0x01 /* read/write request */
#define	 DSTA_RECAL		0x02 /* non read/write requet */
#define	 DSTA_READ_AHEAD	0x04 /* read ahead for the next read request */
#define	 DSTA_WAIT_NEXTJOB	0x08 /* cannot read ahead any more */
#define	 DSTA_WAIT_INTR		0x10 /* wait for read ahead interrupt */

/* drv_cflags */

#define	 CFLG_CMD		0x01 /* a request being processed */
#define	 CFLG_INT		0x02 /* expecting for an interrupt */
#define	 CFLG_READ_AHEAD	0x04 /* reading ahead */
#define	 CFLG_STOPPED		0x08 /* stopped reading ahead */

struct ictha_ctrl {
	struct	ictha_drive	ictha_drives[MAX_ICTHA_DRIVES];
	int	ictha_ndrives;
	/*
	 * Port Addresses 
	 */
	int	ictha_status;
	int	ictha_control;
	int	ictha_command;
	int	ictha_data;

	/*
	 * Control Port Bit Definitions 
	 */
	unchar	ictha_online;
	unchar	ictha_reset;
	unchar	ictha_request;
	unchar	ictha_dma1_2;
	unchar	ictha_intr_enable;
	unchar	ictha_dma_enable;

	/*
	 * Status Port Bit Definitions 
	 */
	unchar	ictha_ready;
	unchar	ictha_exception;

	/*
	 * Set the Power On Reset time 
	 */
	int	ictha_por_delay;

	/*
	 * The next two entries are for the supported
	 * ARCHIVE controllers.                       
	 */
	int	ictha_dma_go;
	int	ictha_reset_dma;

	/*
	 * Type of controller, currently supported
	 */
	int	ictha_type;

	char	ictha_cntrl_mask;
	unchar	ictha_status_buf[ICTHA_STATUSBUF_SZ];
	unchar	ictha_status_new[ICTHA_STATUSBUF_SZ];
	unchar	ictha_cntr_status;

	int	ictha_init_time;
	int	ictha_datardy;
	int	ictha_statrdy;

	int	ictha_dma_stat;
	int	ictha_newstatus;
	int	ictha_reles;

	clock_t	ictha_laststart;

	int	ictha_dmachan;

#ifdef ICTHA_STAT
	int	ictha_savecmd;
	int	ictha_drvcmd;
	int	ictha_errabort;
	int	ictha_writecnt;
	int	ictha_readcnt;
	int	ictha_nblocks;
	int	ictha_prev_nblocks;
#endif

};

#if (PDI_VERSION < 4)
/*
 * HBA_IDATA_STRUCT is defined in hba.h, but hba.h is not yet included
 * so define it the same way here...
 */
#define HBA_IDATA_STRUCT struct hba_idata
#endif

#ifdef __STDC__
extern int ictha_bdinit(CtrlPtr, HBA_IDATA_STRUCT *);
extern int ictha_drvinit(CtrlPtr, DrivePtr);
extern int ictha_cmd(int, CtrlPtr, DrivePtr);
#else /* __STDC__ */
extern int ictha_bdinit();
extern int ictha_drvinit();
extern int ictha_cmd();
#endif /* __STDC__ */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_ICTHA_ICTHA_H */
