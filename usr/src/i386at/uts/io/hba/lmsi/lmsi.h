#ident	"@(#)kern-pdi:io/hba/lmsi/lmsi.h	1.7.1.1"
#ident	"$Header$"
/*
 * LMSI.H
 * 
 * LMSI Include file
 * 
 */

#ifndef	_IO_HBA_LMSI_H
#define	_IO_HBA_LMSI_H

/*******************************************************************************
 *
 *	DEFINES
 *
 ******************************************************************************/

/* translate from/to BCD macros */

#define	FRAME	0
#define	SECOND 	1
#define	MINUTE 	2

#define BCD2BIN(b) ((unchar) (((unchar)(b) >> 4) * 10 + ((unchar)(b) & 15)))
#define BIN2BCD(b) ((unchar) ((((unchar)(b) / 10) << 4) | ((unchar)(b) % 10)))

#define	WAIT_FOR_ECHO	 1
#define CDR_PRI      ((PZERO+2)|PCATCH)         /* priority for sleeping */


/******************************************************************************
 * 
 *      Pseudo SCSI <-> CDR Drive Structures
 *
 *****************************************************************************/

/*
 *      Drive Info Structure -- 1 per drive 
 */

#define	CDR_TOCSZ	20

typedef struct drv_info {
	uint_t        	status;
	uint_t        	lasterr;
	ushort_t        block_size;             /* block size */
	ushort_t        reserved;               /* true if reserved */
	short           nbufs;                  /* # of buffers from cdr_init */
	short           bufcnt;                 /* # of filled buffers */
	unchar          *databuf;               /* buffer for 'nbufs' sectors */
	unchar          *bufp;                  /* ptr to next sector's data */
	struct sense    sense;        		/* sense data */
	ulong_t         nextsec;                /* next sector in buffer */
	ulong_t         drvsize;                /* next sector in buffer */
	unchar          firsttrack;             /* first track on disk */
	unchar          lastttrack;             /* last track on disk */
} drv_info_t;

/*
 *      Controller Info Structure -- 1 per controller
 */

typedef struct ctrl_info {
	uint_t          flags; 
	uint_t          num_drives;             /* number of active drives */
	uint_t          iobase;                 /* io base of controller */
	drv_info_t      *drive [ 8 ];           /* ptrs to drives */
} ctrl_info_t;

/*
 *	port definitions
 */

#define DataStatRegister(base)		(base+0)
#define ReceiveRegister(base)		(base+1)
#define DataFifoRegister(base)		(base+2)
#define DriveStatRegister(base)		(base+3)
#define DataControlRegister(base)	(base+4)
#define XmitRegister(base)		(base+5)
#define TestRegister(base)		(base+7)

/*
 *	DATSTAT bit definitions
 */

#define CRCERR  0x80    /* CRC error */
#define DATERR  0x40    /* data error */
#define FIFOV   0x20    /* FIFO overflow */
#define DATRDY  0x10    /* data ready */
#define SECCNT  0x0F    /* sector count */

/*
 *	DRVSTAT bit definitions
 */

#define TOCRDY  0x80    /* TOC ready in FIFO */
#define FIFOMTY 0x40    /* FIFO empty */
#define ATTN    0x10    /* drive wants attention */
#define PERROR  0x08    /* parity error */
#define OVERRUN 0x04    /* UART receiver overrun */
#define RXRDY   0x02    /* receiver ready */
#define TXRDY   0x01    /* transmitter ready */

/*
 *	Drive Status Byte definitions
 */
#define	DRVNOTRDY	0x08	/* Drive not ready */

/*
 *	DATCTRL bit definitions
 */

#define DRVRESET 0x80   /* reset drive */
#define INIT    0x40    /* initialize the host adapter */
#define MSKTXDR 0x20    /* mask Tx interrupt */
#define FLAGEN  0x10    /* enable transfer of flag data */
#define	DSECCNT	0x0f	/* sec count in bits 3..0 */

/*
 *	TCTRL bit definitions
 */

#define TOCENAB 0x80    /* enable TOC streaming */
#define TSTFLG  0x04    /* test flag bit */
#define TSTDAT  0x02    /* test data bit */
#define TESTEN  0x01    /* test enable bit */

typedef struct {
	unchar	cmd_revision;
	unchar	drive_type;
	unchar	data_rate;
	unchar	reserved1;
	unchar	microcode;
	unchar	reserved2;
	unchar	reserved3;
	unchar	ltimeout;
	unchar	stimeout;
	unchar	maxdisks;
	unchar	reserved4;
	} LmsiCharacteristics;

#ifndef TRUE
#define TRUE	1
#define	FALSE	0
#endif

#define HBA_PREFIX	lmsi
#define	DRVNAME		"lmsi - Philips CD-ROM Drive"

/******************************************************************************
 *
 *	LMSI Data Junk
 *
 *****************************************************************************/

#define	HA_UNINIT		0
#define	HA_INIT			1

#define	SUCCESS			0x00	/* cool */
#define	FAILURE			0x01	/* uncool */
#define	ABORTED			0x02	/* job aborted */

/* LMSI 205 commands */

#define	LMSI_CM205_READ_INFO		0x2d
#define	LMSI_CM205_READ_STATUS 		0x3a
#define	LMSI_CM205_CLEAR_ERRORS 	0x4e
#define	LMSI_CM205_SEEK 		0x59
#define	LMSI_CM205_SPIN_UP 		0x63
#define	LMSI_CM205_SPIN_DOWN 		0x74
#define	LMSI_CM205_BLOCK_READ 		0xa6
#define	LMSI_CM205_EXTEND_READ 		0xa7
#define	LMSI_CM205_PLAY_AUDIO		0xb1
#define	LMSI_CM205_AUDIO_MODE 		0xc5
#define	LMSI_CM205_READ_TOC 		0xe5
#define	LMSI_CM205_READ_UPC 		0xe7
#define	LMSI_CM205_PLAY_TRACK 		0xe8
#define	LMSI_CM205_PAUSE_AUDIO 		0xea
#define	LMSI_CM205_RESUME_AUDIO		0xeb

/* configuration bits */


/* status bits */

#define	LMSI_BLKSIZE	2048
#define	LMSI_MAXBUFFERS 13
#define	LMSI_MAXFER	(LMSI_BLKSIZE*LMSI_MAXBUFFERS)

#define LMSI_USE_STATUS_REG_FOR_DATA		04
#define LMSI_USE_STATUS_REG_FOR_STATUS	0xc

#define LMSI_MAX_TRIES	5

#define	LMSI_MAX_DRIVES	1

/* translate from/to BCD macros */

#define BCD2BIN(b) ((unchar) (((unchar)(b) >> 4) * 10 + ((unchar)(b) & 15)))
#define BIN2BCD(b) ((unchar) ((((unchar)(b) / 10) << 4) | ((unchar)(b) % 10)))

#define	MAX_LMSI		1	/* max lmsi controllers/sys */
#define	MAX_JOBS_DRIVE		1	/* max jobs/drive */
#define	LOWATER_JOBS		1	/* low water mark for jobs */
#define	HIWATER_JOBS		1	/* high water mark for jobs */

#define	LMSITIMOUT		500000	/* 1/4 sec setup time out */

#define	READ_DATA		0x01	/* reading data */

#define	SCM_RAD(x)	((char *)x - 2)	/* re-adjust 8 byte SCSI cmd */

/******************************************************************************
 *
 *	Pseudo SCSI <-> LMSI Drive Structures
 *
 *****************************************************************************/

/*
 * read capacity data header format
 */

typedef struct capacity {
	int             cd_addr;
	int             cd_len;
}               capacity_t;

#define	RDCAPSZ			0x08	/* size of capacity structure */

#define	MCD_MAXTOCS 104

/******************************************************************************
 *
 *	SCSI Stuff
 *
 *****************************************************************************/

/*
 * Cool constants
 */

#define	MAX_EQ			MAX_TCS*MAX_LUS	/* max ctlr equip */
#define	MAX_CMDSZ		12	/* max scsi cmd size */


/******************************************************************************
 *
 *	SCSI Structures
 *
 *****************************************************************************/

/*
 * SCSI Request Block structure
 * 
 * A pool of these structs is kept and allocated to the sdi module via a call to
 * xxx_getblk () and returned via xxx_freeblk ().
 */

typedef struct srb {
	struct xsb     *sbp;		/* Target drv definition of SB	*/
	struct srb *s_next;		/* Next block on LU queue	*/
	struct srb *s_priv;		/* Private ptr for dynamic alloc*/
					/* routines DO NOT USE or MODIFY*/
	struct srb *s_prev;		/* Previous block on LU queue	*/

	ulong_t         s_blkno;
	uchar_t	        *s_addr;
	ushort_t        s_size;	
	ushort_t        s_tries;
}               sblk_t;

/*
 * Logical Unit Queue structure
 */

typedef struct scsi_lu {
	struct srb     *q_first;/* first block on LU queue */
	struct srb     *q_last;	/* last block on LU queue */
	int             q_flag;	/* LU queue state flags */
	int             q_count;/* jobs running on this LU */
	int             q_depth;/* jobs in the queue */
	void            (*q_func) ();	/* target driver event handler */
	long            q_param;/* target driver event param */
	long            dev_depth;	/* device depth */
	long            dev_max_depth;	/* max device depth */
}               scsi_lu_t;

/*
 * flags for the Queues
 */

#define	QBUSY			0x01
#define	QSUSP			0x04	/* processing is suspended */
#define	QSENSE			0x08	/* sense data is valid */
#define	QPTHRU			0x10	/* queue is in pass-thru mode */

#define	QUECLASS(x)		((x)->sbp->sb.sb_type)

/*
 * Neat Functions
 */

#define	SUBDEV(t,l)		((t << 3) | l)
#define	LU_Q(c,t,l)		lmsi_sc_ha[c].ha_queue[SUBDEV(t,l)]

#define LMSI_MAX_IDLE_TIME	(5*60*HZ)	/* stop after 5 munites */
#define LMSI_CHKTIME		(20*HZ)		/* every 20 seconds */
#define LMSI_CMD_TIMEOUT	(10*HZ)		/* timeout command */

typedef struct {
	ulong	iflags;
	ulong	arg;
	int	(*preaction)();
	int	(*getanswer)();
	int	(*postaction)();
	} Action;

typedef struct {
	char	*cmd;
	Action	*action;
	int	maxactions;
	} SubCmd;

#define	NUMBER_OF_ACTIONS(LMSICMD)	(sizeof(LMSICMD)/sizeof(Action))
#define	MAX_SUBS_GROUPS	20
typedef struct {
	SubCmd	*subcmd[MAX_SUBS_GROUPS];
	unchar	nactions[MAX_SUBS_GROUPS];
	unchar	nsubcmds, action_no, subcmd_no, phase_no;
	char	*cmdname;
	} CM205Command;

typedef struct {
	unchar	trackno;
	unchar	frameno;
	unchar	secondno;
	unchar	minuteno;
	} CM205Trackinfo;

typedef struct {
	unchar	unit;
	unchar	ldcb1;
	unchar	ldcb2;
	unchar	ldcb3;
	unchar	ldcb4;
	unchar	drvstatus;
	unchar	drvcode;
	unchar	errcode;
	unchar	lframe;
	unchar	lsecond;
	unchar	lminute;
	unchar	firsttrk;
	unchar	ntracks;
	unchar	curtrk;
	unchar	rframe;
	unchar	rsecond;
	unchar	rminute;
	unchar	trkindx;
	unchar	xabyte;
	} CM205Status;

/*
 * SCSI Host Adapter structure
 */

#define	MAX_TOC_SIZE	(5*99+1)

typedef struct scsi_ha {
	ushort_t        ha_state;	/* state of the ha */
	ushort_t        ha_id;	/* HA scsi id (7) */
	int             ha_vect;/* int vector */
	ushort_t        ha_iobase;	/* io base address */
	int             ha_tjobs;	/* total jobs sent to ha */
	scsi_lu_t      *ha_queue;	/* logical unit queues */
	/*
	 * implementation specific stuff
	 */
        int		ha_flags;
        sblk_t		*ha_cmd_sp;
	clock_t		ha_cmd_time;
        int		ha_tid;
	int		ha_status;
	int		ha_error;
	CM205Command	ha_cmd;
	unchar		ha_toc_data[5*99+1];
	int		ha_toc_byte_no;

	CM205Status	ha_status_data;
	int		ha_status_byte_no;
	void		(*ha_repeat)();
	ulong		ha_repeat_arg;
	ulong		ha_tries;
	ulong		ha_startsec;
	ulong		ha_endsec;
	unchar		ha_interrupted;
}               scsi_ha_t;

extern void lmsi_next( scsi_lu_t *q );
extern void lmsi_flushq( scsi_lu_t *q, int cc );

extern void lmsi_func(sblk_t * sp);
extern void lmsi_cmd(sblk_t * sp);
extern int  lmsi_wait( int c, int timeout );
extern void lmsiintr(unsigned int);

#endif				/* _IO_HBA_LMSI_H */
