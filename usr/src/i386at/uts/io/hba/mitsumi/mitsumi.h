#ident	"@(#)kern-pdi:io/hba/mitsumi/mitsumi.h	1.2.3.1"
#ident	"$Header$"
/*
 * MITSUMI.H
 * 
 * MITSUMI Include file
 * 
 */

#ifndef	_IO_HBA_MITSUMI_H
#define	_IO_HBA_MITSUMI_H

#ifndef TRUE
#define TRUE	1
#define	FALSE	0
#endif

#define HBA_PREFIX	mitsumi
#define	DRVNAME		"mitsumi - Mitsumi CD-ROM Drive"

/******************************************************************************
 *
 *	MITSUMI Data Junk
 *
 *****************************************************************************/

#define	HA_UNINIT		0
#define	HA_INIT			1

#define	SUCCESS			0x00	/* cool */
#define	FAILURE			0x01	/* uncool */
#define	ABORTED			0x02	/* job aborted */

/* configuration bits */

#define MITSUMI_SELECT_IO_TYPE		(1<<1)
#define MITSUMI_SELECT_PIO		0
#define MITSUMI_SELECT_DMA		1

#define MITSUMI_SELECT_INTERRUPT_TYPE	(1<<4)
#define MITSUMI_INTERRUPT_ONERROR 	1
#define MITSUMI_POST_INTERRUPT		(1<<1)
#define MITSUMI_PRE_INTERRUPT		(1<<2)


/* status bits */

#define MITSUMI_DOOR_OPEN	(1<<7)
#define MITSUMI_DISK_SET		(1<<6)
#define MITSUMI_DISK_CHANGED	(1<<5)
#define MITSUMI_DISK_ROTATE	(1<<4)
#define MITSUMI_AUDIO_TRACK	(1<<3)
#define MITSUMI_READ_ERROR	(1<<2)
#define MITSUMI_PLAY_AUDIO	(1<<1)
#define MITSUMI_COMMAND_ERROR	(1)
#define MITSUMI_ERROR	( MITSUMI_DOOR_OPEN|MITSUMI_DISK_CHANGED| MITSUMI_AUDIO_TRACK| MITSUMI_READ_ERROR| MITSUMI_PLAY_AUDIO| MITSUMI_COMMAND_ERROR)

#define MITSUMI_STOP_DISK_CMD		0xF0 /* 1 bytes input, 0 output */
#define MITSUMI_SEEK_N_READ_CMD		0xC0 /* 7 bytes input, 1 output */
#define MITSUMI_SET_DATA_MODE_CMD	0xA0 /* 2 bytes input, 1 output */
#define MITSUMI_GET_VERSION_CMD		0xDC /* 1 bytes input, 3 output */
#define MITSUMI_SET_INACTIVITY_TIME	0x80 /* 2 bytes input, 1 output */
#define MITSUMI_HOLD_HEAD_CMD		0x70 /* 1 bytes input, 1 output */
#define MITSUMI_RESET_DRIVE_CMD		0x60 /* 1 bytes input, 1 output */
#define MITSUMI_SET_DRIVE_MODE		0x50 /* 2 bytes input, 1 output */
#define MITSUMI_GET_DRIVE_STATUS		0x40 /* 1 bytes input, 1 output */
#define MITSUMI_REQUEST_SENSE_CMD	0x30 /* 1 bytes input, 2 output */
#define MITSUMI_REQUEST_SUBQ_CODE	0x20 /* 1 bytes input, 11 output */
#define MITSUMI_REQUEST_TOC_CMD		0x10 /* 1 bytes input, 9 output */
#define MITSUMI_CONFIGURE_DRIVE		0x90 /* 4 bytes input, 1 output */
#define MITSUMI_SET_ATTENATOR		0xAE /* 5 bytes input, 5 output */
#define MITSUMI_SET_INTERLEAVE		0xC8 /* 2 bytes input, 2 output */
#define MITSUMI_READ_UPC_CODE		0xA2 /* 1 bytes input, 10 output */
#define MITSUMI_GET_MULTIDISK_INFO	0x11 /* 1 bytes input, 5 output */

#define	MITSUMI_BLKSIZE	2048

#define MITSUMI_USE_STATUS_REG_FOR_DATA		04
#define MITSUMI_USE_STATUS_REG_FOR_STATUS	0xc

#define MITSUMI_STATUS_REG 	1
#define MITSUMI_HCON_REG 	2
#define MITSUMI_DATA_REG 	0
#define MITSUMI_COMMAND_REG	0

#define MITSUMI_STATUS_BIT	04
#define MITSUMI_DATA_BIT		02
#define MITSUMI_MAX_TRIES	9

#define	MITSUMI_MAX_DRIVES	1

/* translate from/to BCD macros */

#define BCD2BIN(b) ((unchar) (((unchar)(b) >> 4) * 10 + ((unchar)(b) & 15)))
#define BIN2BCD(b) ((unchar) ((((unchar)(b) / 10) << 4) | ((unchar)(b) % 10)))

#define	MAX_MITSUMI		1	/* max mitsumi controllers/sys */
#define	MAX_JOBS_DRIVE		1	/* max jobs/drive */
#define	LOWATER_JOBS		1	/* low water mark for jobs */
#define	HIWATER_JOBS		1	/* high water mark for jobs */

#define	MITSUMITIMOUT		500000	/* 1/4 sec setup time out */

#define	READ_DATA		0x01	/* reading data */

#define	SCM_RAD(x)	((char *)x - 2)	/* re-adjust 8 byte SCSI cmd */

/******************************************************************************
 *
 *	Pseudo SCSI <-> MITSUMI Drive Structures
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

typedef struct drv_info {
	unchar	trk_low;
	unchar	trk_high;
	unchar	end_sect_m;
	unchar	end_sect_s;
	unchar	end_sect_f;
	unchar	start_sect_m;
	unchar	start_sect_s;
	unchar	start_sect_f;
	int	starting_track;
	int	ending_track;
	int	startsect;
	int	lastsect;
	int	drvsize;
	int	status, lasterr;
	uint_t		flags;
	struct		sense mitsumi_sense;
	struct mcd_qchninfo toc[MCD_MAXTOCS];
}               drv_info_t;

/*
 * Controller Info Structure -- 1 per controller
 */

#define	MITSUMI_MAX_DRIVES	1

typedef struct ctrl_info {
	uint_t		flags;
	uint_t          num_drives;	/* number of active drives */
	uint_t          iobase;	/* io base of controller */
	drv_info_t     *drive[MITSUMI_MAX_DRIVES];	/* ptrs to drives */
}               ctrl_info_t;

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

#define pgbnd(a)		(NBPP - ((NBPP - 1) & (int)(a)))
#define	SUBDEV(t,l)		((t << 3) | l)
#define	LU_Q(c,t,l)		mitsumi_sc_ha[c].ha_queue[SUBDEV(t,l)]

#define MITSUMI_MAX_IDLE_TIME	(5*60*HZ)	/* stop after 5 munites */
#define MITSUMI_CHKTIME		(20*HZ)		/* every 20 seconds */
#define MITSUMI_CMD_TIMEOUT	(10*HZ)		/* timeout command */

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
}               scsi_ha_t;

extern void mitsumi_next( scsi_lu_t *q );
extern void mitsumi_flushq( scsi_lu_t *q, int cc );

extern void mitsumi_func(sblk_t * sp);
extern void mitsumi_cmd(sblk_t * sp);
extern int mitsumi_wait( int c, int timeout );
extern void mitsumiintr(unsigned int);

#endif				/* _IO_HBA_MITSUMI_H */
