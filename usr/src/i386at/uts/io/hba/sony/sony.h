#ident	"@(#)kern-pdi:io/hba/sony/sony.h	1.7.2.1"
#ident	"$Header$"
/*
 * SONY31A.H
 * 
 * SONY31A Include file
 * 
 */

#ifndef	_IO_HBA_SONY31A_H
#define	_IO_HBA_SONY31A_H

#ifndef TRUE
#define TRUE	1
#define	FALSE	0
#endif

#define HBA_PREFIX	sony
#define	DRVNAME		"sony - Sony CD-ROM Drive"

/******************************************************************************
 *
 *	SONY Data Junk
 *
 *****************************************************************************/

#define	HA_UNINIT		0
#define	HA_INIT			1

#define	SUCCESS			0x00
#define	FAILURE			0x01
#define	ABORTED			0x02	/* job aborted */


/***************************************************************************/
/********************************** 31A stuff ******************************/


#define SONY31A_INTERRUPT_MASK (SC_ATTN_INT_EN_BIT|SC_RES_RDY_INT_EN_BIT|SC_DATA_RDY_INT_EN_BIT)

/* Bits returned by sony31a_stat() */

#define SONY31A_DISK        0x0001      /* disk is definitely present */
#define SONY31A_NODISK      0x0002      /* disk is definitely absent */
#define SONY31A_CHANGE      0x0004      /* disk has been changed */
#define SONY31A_SPINNING    0x0008      /* disk believed to be spinning */
#define SONY31A_ADISK       0x0010      /* current disk is audio-only */
#define SONY31A_ATRACK      0x0020      /* current track is audio */
#define SONY31A_HAVEINTRPTS 0x0040      /* interrupts work on this device */
#define SONY31A_STATED      0x0080      /* already tested status */
#define SONY31A_33A    	    0x0100      /* a 33a */



/*
 *	control reg bits
 */	

#define 	 SC_ATTN_CLR_BIT        0x01
#define		 SC_RES_RDY_CLR_BIT     0x02
#define		 SC_DATA_RDY_CLR_BIT    0x04
#define		 SC_ATTN_INT_EN_BIT     0x08
#define		 SC_RES_RDY_INT_EN_BIT  0x10
#define		 SC_DATA_RDY_INT_EN_BIT 0x20
#define		 SC_PARAM_CLR_BIT       0x40
#define		 SC_DRIVE_RESET_BIT	0x80

/*
 *	status reg bits
 */

#define		 ST_ATTN_BIT            0x01
#define		 ST_RES_RDY_BIT         0x02
#define		 ST_DATA_RDY_BIT        0x04
#define		 ST_ATTN_INT_ST_BIT     0x08
#define		 ST_RES_RDY_INT_ST_BIT  0x10
#define		 ST_DATA_RDY_INT_ST_BIT 0x20
#define		 ST_DATA_REQUEST_BIT    0x40
#define		 ST_BUSY_BIT            0x80

/*
 *	fifo status reg bits
 */

#define		 SF_PARAM_WRITE_RDY_BIT 0x01
#define		 SF_PARAM_REG_EMPTY_BIT 0x02
#define		 SF_RES_REG_NOT_EMP_BIT 0x04
#define		 SF_RES_REG_FULL_BIT    0x08


/*
 *	attention codes - NON-ERROR
 */

#define A_LOADED              0x80     /* mechanisim loaded */
#define A_EJECT_BUTTON        0x81     /* eject pushed */
#define A_PLAY_DONE           0x90     /* audio play complete */
#define A_SPIN_UP             0x24     /* auto spin-up complete */
#define A_TOC_READ            0x62     /* auto TOC read */
#define A_SPIN_DOWN           0x27     /* spin-down complete */
#define A_EJECT_DONE          0x28     /* eject complete */

/*
 *	attention codes - ERROR
 */

#define A_HDWR_ERR            0x70
#define A_EMER_EJECT_ERR      0x2C
#define A_LEAD_IN_ERR         0x91
#define A_LEAD_OUT_ERR        0x92
#define A_DATA_ERR            0x93
#define A_AUDIO_ERR           0x94
#define A_SPIN_SERVO_ERR      0x25
#define A_FOCUS_SERVO_ERR     0x26
#define A_TOC_FOCUS_ERR       0x63
#define A_TOC_SYNC_ERR        0x64
#define A_TOC_DATA_ERR        0x65
#define A_EJECT_ERR           0x29

/*
 *	command register commands
 */

#define SONY31A_GET_CFG            0x00
#define SONY31A_GET_PARAM          0x02
#define SONY31A_GET_MECH_STAT      0x03
#define SONY31A_SET_PARAM          0x10
#define SONY31A_GET_TOCDATA        0x20
#define SONY31A_GET_SUBCODE_ADDR   0x21
#define SONY31A_GET_TOC_DATA_SPEC  0x24
#define SONY31A_READ_TOC           0x30
#define SONY31A_SEEK               0x31
#define SONY31A_READ               0x32
#define SONY31A_READ_BLKERR        0x34
#define SONY31A_ABORT              0x35
#define SONY31A_READ_TOC_SPEC      0x36
#define SONY31A_AUDIO_PLAY         0x40
#define SONY31A_AUDIO_STOP         0x41
#define SONY31A_AUDIO_SCAN         0x42
#define SONY31A_SPIN_UP_DISK       0x51
#define SONY31A_SPIN_DOWN_DISK     0x52

/*
 *	mechanical status bits (byte 0)
 */

#define MS0_MECH_LOADED       0x01
#define MS0_DISK_LOADED       0x02
#define MS0_DIODE_ON          0x04
#define MS0_ROTATING          0x08
#define MS0_TOC_READ          0x10
#define MS0_LED_ON            0x20

/*
 *	result buffer error codes 
 */

#define	ERR_NO_ERROR		0x00		/* no error */
#define	ERR_TIMEOUT		0x01		/* timeout error */
#define	ERR_CMD			0x10		/* invalid command */
#define	ERR_PARAM		0x11		/* invalid param */
#define	ERR_LOAD_MECH_ERR	0x20		/* loadmech failure */
#define	ERR_NODISC		0x21		/* no disk */
#define	ERR_NOSPIN		0x22		/* not spinning */
#define	ERR_SPIN		0x23		/* spinning */
#define	ERR_SPINDLE_SERVO	0x25		/* spindle servo failure */
#define	ERR_FOCUS_SERVO		0x26		/* focus servo failure */
#define	ERR_AUDIO_PLAY		0x2a		/* audio currently playing */
#define	ERR_EMERGENCY_EJECT	0x2c		/* emergency eject */

#define SONY31A_ERR_BIT 0x20

#define SONY_BLKSIZE  2048
#define SONY31A_MAX_TRIES        7

#define SONY31A_ERROR       1  /* this is going into ha_status ??? */

/* interrupt state machine */
#define		SONY31A_RDDATA		1
#define		SONY31A_RECONFTOCSPINUP	2
#define		SONY31A_RECONFTOCREAD	3
#define		SONY31A_RECONFTOCDATA	4
#define		SONY31A_CONFCASES	4

#define		SONY31A_RDCAPTOCSPINUP	6
#define		SONY31A_RDCAPTOCREAD	7
#define		SONY31A_RDCAPTOCDATA	8
	
#define		SONY31A_CHKMEDIASPIN1	10
#define		SONY31A_CHKMEDIASPIN2	11

#define SONY31A_MAX_IDLE_TIME	(5*60*HZ)	/* stop after 5 minutes */
#define SONY31A_CHKTIME		(2*HZ)		/* every 2 seconds */
#define SONY31A_DATA_TIMEOUT	(15*HZ)		/* timeout command */
#define SONY31A_CONTROL_TIMEOUT	(30*HZ)		/* timeout command */

#define	SONY31A_WAITATT		40000	/* paranoid amount */
#define SONY31A_TOCSIZE		20	/* size of TOC data */
#define SONY31A_RESSIZE		36	/* maximum result bytes */
#define SONY31A_WANTINTR	1	/* use interrupt with command */
#define SONY31A_POLL		0
#define SONY31A_FROMRDCAP	1	/* _readtoc called from rdcap */
#define SONY31A_FROMRECONF	0	/* _readtoc called from reconf */


/****************************************************************************/
/********************Sony 535 stuff *****************************************/

#define SONY535_ERROR 1

#define SONY535_MAX_TRIES        1

#define SONY535_RESSIZE 511  /* max may be smaller ?? */
#define SONY535_RESHEAD 36
#define SONY535_TOCSIZE 20
#define SONY535_INQUIRSIZE 29

#define SONY535_INTERRUPT_MASK 0x01

/* Bits returned by sony535_stat() */
#define SONY535_DISK        0x0001      /* disk is definitely present */
#define SONY535_NODISK      0x0002      /* disk is definitely absent */
#define SONY535_CHANGE      0x0004      /* disk has been changed */
#define SONY535_SPINNING    0x0008      /* disk believed to be spinning */
#define SONY535_ADISK       0x0010      /* current disk is audio-only */
#define SONY535_ATRACK      0x0020      /* current track is audio */

#define SONY535_NOTEXPECTINT  0x0040   /* interrupts not expected right now */


/* cmd reg commands */
#define SONY535_GET_DRIVE_STAT1    0x80
#define SONY535_GET_SENSE          0x82
#define SONY535_GET_DRIVE_STAT2    0x84
#define SONY535_GET_ERROR_STAT     0x86
#define SONY535_INQUIRY            0x8A

#define SONY535_SET_INACTIVITY     0x90

#define SONY535_READ1              0xA0
#define SONY535_READ2              0xA4
#define SONY535_PLAY_AUDIO         0xA6

#define SONY535_GET_CAPACITY       0xB0
#define SONY535_GET_TOC_DATA       0xB2
#define SONY535_GET_SUBQ_DATA      0xB4
#define SONY535_SET_MODE           0xC0
#define SONY535_SET_RETRY          0xC4
#define SONY535_DIAG1              0xC6
#define SONY535_DIAG4              0xCC
#define SONY535_DIAG5              0xCE

#define SONY535_DISABLE_EJECT      0xD2
#define SONY535_ENABLE_EJECT       0xD4

#define SONY535_HOLD               0xE0
#define SONY535_AUDIO_PAUSE        0xE2
#define SONY535_AUDIO_VOLUME       0xE8

#define SONY535_STOP               0xF0
#define SONY535_SPIN_UP            0xF2
#define SONY535_SPIN_DOWN          0xF4

#define SONY535_CLEAR_PARMS        0xF6
#define SONY535_CLEAR_END_ADDR     0xF8

/* status bits */
#define S1B1_TWO_BYTES        0x80
#define S1B1_NO_CADDY         0x40
#define S1B1_EJECT_PRESSED    0x20
#define S1B1_NOT_SPINNING     0x10
#define S1B1_DISK_ERR         0x08
#define S1B1_SEEK_ERR         0x04
#define S1B1_DATA_ERR         0x02
#define S1B1_CMD_ERR          0x01

#define MODE_AUDIO            0x00        /* audio play mode */
#define MODE_DATA             0xF0        /* data mode */

/* stat 2  */
#define S2B1_NO_DISK	      0xD0

/* misc */
#define FRAMES_PER_SEC        75
#define SECS_PER_MIN          60
#define INTERRUPT_MASK        0x01
#define BURST_MASK            0x02
#define ANY_ERROR             0x7f        /* any error */
#define SECTOR_SIZE           2048

/* flag register bits */
#define F_DATA                0x01
#define F_STATUS              0x02


/* interrupt state machine */
#define         SONY535_RDDATA          1
#define         SONY535_RECONFTOCSPINUP 2
#define         SONY535_RECONFTOCREAD   3
#define         SONY535_RECONFTOCDATA   4
#define         SONY535_CONFCASES       4

#define         SONY535_RDCAPTOCSPINUP  6
#define         SONY535_RDCAPTOCREAD    8
#define         SONY535_RDCAPTOCDATA    9

#define SONY535_MAX_IDLE_TIME   (16*HZ)         /* stop after 16 secs */
#define SONY535_READCHKTIME     (1)             /* every 1 tick */
#define SONY535_CONTROLCHKTIME  (5*HZ)          /* every 5 seconds */
#define SONY535_CONTROL_TIMEOUT (13*HZ)         /* timeout command */
#define SONY535_DATA_TIMEOUT    (1*HZ)         /* timeout command */

#define SONY535_WANTINTR        1       /* use interrupt with command */
#define SONY535_POLL            0       /* poll with command */
#define SONY535_FROMRDCAP       1       /* _readtoc called from rdcap */
#define SONY535_FROMRECONF      0       /* _readtoc called from reconf */



/****************************************************************************/

/* translate from/to BCD macros */

#define BCD2BIN(b) ((unchar) (((unchar)(b) >> 4) * 10 + ((unchar)(b) & 15)))
#define BIN2BCD(b) ((unchar) ((((unchar)(b) / 10) << 4) | ((unchar)(b) % 10)))

#define	MAX_SONY		1	/* max sony controllers/sys */
#define	MAX_JOBS_DRIVE		1	/* max jobs/drive */
#define	LOWATER_JOBS		1	/* low water mark for jobs */
#define	HIWATER_JOBS		1	/* high water mark for jobs */

#define	READ_DATA		0x01	/* reading data */

#define	SCM_RAD(x)	((char *)x - 2)	/* re-adjust 8 byte SCSI cmd */


/******************************************************************************
 *
 *	Pseudo SCSI <-> SONY Drive Structures
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

struct mcd_qchninfo {
	unchar	ctrl_adr;
	unchar	trk_no;
	unchar	idx_no;
	unchar	trk_size_msf[3];
	unchar	:8;
	unchar	hd_pos_msf[3];
};

/*
 * Drive Info Structure -- 1 per drive
 */

typedef struct sony_toc_info {
	unchar  control0;
	unchar  point0;
	unchar	trk_low;
	unchar  disktype; /* for a 535 this is a reserved byte */
	unchar  dummy0;
	unchar  control1;
	unchar  point1;
	unchar	trk_high;
	unchar  dummy1;
	unchar  dummy2;
	unchar  control2;
	unchar  point2;
	unchar	end_sect_m; /* rename to leadout */
	unchar	end_sect_s;
	unchar	end_sect_f;
	unchar  control3;
	unchar  trkcnt;
	unchar	start_sect_m;
	unchar	start_sect_s;
	unchar	start_sect_f;
} sony_toc_info_t;


typedef struct drv_info {
	sony_toc_info_t drvtoc;
	int	starting_track;
	int	ending_track;
	int	startsect;
	int	lastsect;
	int	drvsize;
	int	status, lasterr;
	uint_t		flags;
	struct		sense sony_sense;
	struct mcd_qchninfo toc[MCD_MAXTOCS];
} drv_info_t;

/*
 * Controller Info Structure -- 1 per controller
 */

#define	SONY_MAX_DRIVES	1

typedef struct ctrl_info {
	uint_t		flags;
	uint_t          num_drives;	/* number of active drives */
	uint_t          iobase;	/* io base of controller */
	drv_info_t     *drive[SONY_MAX_DRIVES];	/* ptrs to drives */
} ctrl_info_t;

/******************************************************************************
 *
 *	SCSI Stuff
 *
 *****************************************************************************/

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
	ulong_t         s_paddr;
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

#define	SONY_QUECLASS(x)		((x)->sbp->sb.sb_type)

/*
 * Neat Functions
 */

#define	SONY_SUBDEV(t,l)		((t << 3) | l)
#define	SONY_LU_Q(c,t,l)		sony_sc_ha[c].ha_queue[SONY_SUBDEV(t,l)]


/* drive type object */
typedef struct sonyvector {
	int	(*sonybdinit)();
	void	(*sonyintr)();
	void	(*sony_rdcap)();
	int	(*sony_chkmedia)();
	int	(*sony_issue_rdcmd)();
	void    (*sony_error)();
#if PDI_VERSION >= PDI_UNIXWARE20
	int     (*sonyverify)();
#endif 
} sonyvector_t;

/*
 * SCSI Host Adapter structure
 */

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
        sblk_t		*ha_cmd_sp;
	clock_t		ha_cmd_time;
        int		ha_tid;
	int		ha_status;
	int		ha_driveStatus; /* should be in drv_info ?? */
	int		ha_lasterr; /* should be in drv_info ?? */
	int       	ha_casestate;	/* state machine */
	uchar_t		*ha_resbuf;  /* pointer to result buffer in drv_info?*/
	int             ha_statsize;
	sonyvector_t	*ha_devicevect; /* pointer to device object */
	char            ha_sony_vendor[ 10 ];
	char            ha_sony_drive_ver[ 9 ];
	char            ha_sony_diskprod[ 17 ];
	char            ha_sony_prod[ 17 ];
	int             ha_dmachan;
#if PDI_VERSION >= PDI_SVR42MP
        struct  dma_cb  *ha_cb;
#endif
} scsi_ha_t;

#endif				/* _IO_HBA_SONY31A_H */
