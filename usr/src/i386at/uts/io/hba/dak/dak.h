#ident	"@(#)kern-pdi:io/hba/dak/dak.h	1.13.3.4"
#ident	"$Header$"

#ifndef _IO_HBA_DAK_H	/* wrapper symbol for kernel use */
#define _IO_HBA_DAK_H	/* subject to change without notice */

#include <sys/buf.h>

#define DAK_MAXCTLS	4		/* max # DAC960 Controllers */
#define DAK_INTR_OFF	0
#define DAK_INTR_ON	1

/*
 * EISA specific defines
 */
#define DAK_UNDEFINED	0xfff		/* DAK offset base address undef */
#define DAK_IOADDR_MASK	0xfff		/* DAK offset mask */
#define DAK_EISA_BASE	0xc80		/* DAK eisa base ioaddr */

/*
 * MCA specific defines
 */
#define DAK_MCA_BIOS_OFF 0x1890		/* DAK MCA BIOS offset for memory map */
#define DAK_MCA_MEMMAP_SZ    16		/* DAK MCA memory map size	      */

/* Structure Passed By ioctl calls */
struct dacimbx{
	uchar_t ioc_mbx0;	/* Dac command Opcode */
	uchar_t ioc_mbx2;	/* Command Identifier */
	uchar_t ioc_mbx3;	/* Chan,testno	     */
	uchar_t ioc_mbx4;	/* State,chan		*/
	caddr_t ioc_buf;	/* data  buffer */
	unsigned short	ioc_bufsz;	/* data  buffer size */
        uchar_t ioc_mbx5;
        uchar_t ioc_mbx6;
        uchar_t ioc_mbx7;
        uchar_t reserved[3];
        unsigned short status;
};

/* DAC Direct CDB Structure */
struct daccdb{
	uchar_t cdb_unit;	/*  Chan(upper 4 bits),target(lower 4) */
	uchar_t cdb_cmdctl;	/* Command control */
	ushort_t cdb_xfersz;	/* Transfer length */
	caddr_t cdb_databuf;	/* Data buffer in system Memory */
	uchar_t cdb_cdblen;	/*  Size of CDB in bytes */
	uchar_t cdb_senselen;	/*  Size of sense length in bytes */
	uchar_t cdb_data[12];	/* Pointer to CDB Data upto 12 bytes */
	uchar_t cdb_sensebuf[64];/* Pointer to Sense Data upto 64 bytes */
	uchar_t cdb_status;	/* SCSI Status */
	uchar_t cdb_reserved;	/* SCSI Status */
};

#define 	V_TT ('D'<<8)
#define IO 0
#define IOCTL 1
#define DATAOFST 400
#define DAC_RBLD 9
#define B_DCMD 1
#define B_DCDB 0
/*************************************************************************
**                 General Implementation Definitions                   **
*************************************************************************/
#define MAX_TCS_3X	16
#define MAX_LUS_3X	32
#define MAX_EQ_3X   MAX_TCS_3X * MAX_LUS_3X  /* Max equipage per controller  */

#define MAX_EQ   MAX_TCS * MAX_LUS      /* Max equipage per controller  */
#define NDMA		64		/* Number of DMA lists, was 20	*/
#define NCPS            64              /* Number of command packets    */
#define DAK_SCSI_ID	7		/* Default ID of controllers	*/

#define	ONE_MSEC	1
#define	ONE_SEC		1000
#define	DAK_ONE_MIN	60000
#define	DAK_TEN_SEC	10000

#if (PDI_VERSION >= 4)
#define DAK_NBUS	5		/* Number of SCSI Buses to register */
#else
#define DAK_NBUS	1		/* Number of SCSI Buses to register */
#endif

#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif
#define BYTE            unsigned char

#define PUCHAR unsigned char *
#define USHORT u_short
#define ULONG u_long
#define UCHAR u_char

#define START 0
#define ABORT 1

#define SUCCESS 0
#define FAILURE 1
#define CP_DMA_CMD 1
#define HA_AUTO_REQ_SEN 2
#define HA_COMMAND 3

#define PORT_READ	0x80
#define IRQNO		0x43
#define OP_DCDB		0x04
#define OP_ENQ		0x05
#define OP_FLUSH	0x0a
#define OP_GETDEV_STATE 0x14		/* Get device state */
#define LDAC_READ	2
#define LDAC_WRITE	3
#define FLUSH_ID	128 /* This should not conflict with any other ID */

#define LDAC_READ_EXT   0x33
#define LDAC_WRITE_EXT  0x34
#define OP_GETDEV_STATE_NEW	0x50
#define OP_ENQ_NEW		0x53 

/*
** Some DCDB Equates
*/

#define MAXCHANNEL 5
#define MAXTARGET 7
#define MAXTARGET_3X 16

#define DATA_IN 0x81    /* IN and DISCONNECT */
#define DATA_OUT 0x82  /* OUT and DISCONNECT */
#define DATA_NONE 0x80 /* NONE */
#define CHK_COND 2

#define DAC_MDRIVES 8

#define	HA_NAME		SDI_IOC_HBA_IOCTL_00	
#define	DIODCMD		SDI_IOC_HBA_IOCTL_01	/* Use Reserved ioctl's */
#define	DIODCDB		SDI_IOC_HBA_IOCTL_02
#define	DIOSTART_CHAN	SDI_IOC_HBA_IOCTL_03
#define	MIOC_ADP_INFO	SDI_IOC_HBA_IOCTL_04
#define	CDB_DISCON 0x80

#define NOMORE_ADAPTERS	0x01
#ifndef B_DCMD
#define B_DCMD 
#endif
#ifndef B_DCDB
#define B_DCDB 
#endif


struct ENQ_DATA
{

/* 
 * Information returned by  DAC_ENQUIRY command
*/
	ulong_t nsdrives;	/* Total Number of system drives */
	ulong_t sdrvsz[DAC_MDRIVES]; /* sizes of system drives */
	ushort_t en_fage;		/* age of the firmware flash eep */
	uchar_t	en_wbkerr;		/* flag indicating write back error */
	uchar_t	en_fchgcount;		/* # of free entries in chg list */	
	uchar_t dac_minor_fwver;	/* firmware minor version Number */
	uchar_t dac_major_fwver;	/* firmware major version Number */
	uchar_t stbyrbld;		/* standby drive being rebuilt to
					 replace a dead drive */
	uchar_t max_iop;		/*  Number of Max iop Q tables */
	ulong_t noffline;		/*  Number of drives offline */
	ulong_t ncritical;		/* number of drives critical */
	ushort_t ndead;			/* Number of drives dead */
	ushort_t wonly;			/* no.of write only scsi devices */
	ushort_t dead_drvlist[21+2]; /* List of dead scsi drives */
};

#define MAX_SYSDRIVES 32
struct ENQ_DATA_3X
{
/* 
** Information returned by  DAC_ENQUIRY command for fw 3.x
*/
        uchar_t nsdrives;       /* Total Number of system drives */
        uchar_t reserved1[3];
        ulong_t sdrvsz[MAX_SYSDRIVES]; /* sizes of system drives */
        ushort_t en_fage;               /* age of the firmware flash eep */
        uchar_t en_wbkerr;              /* flag indicating write back error */
        uchar_t en_fchgcount;           /* # of free entries in chg list */  
  
        uchar_t dac_minor_fwver;        /* firmware minor version Number */
        uchar_t dac_major_fwver;        /* firmware major version Number */
        uchar_t stbyrbld;               /* standby drive being rebuilt to
                                         replace a dead drive */
        uchar_t max_iop;                /*  Number of Max iop Q tables */
        ulong_t noffline;               /*  Number of drives offline */
        ulong_t ncritical;              /* number of drives critical */
        ushort_t ndead;                 /* Number of drives dead */
        ushort_t wonly;                 /* no.of write only scsi devices */
        ushort_t dead_drvlist[21+2]; /* List of dead scsi drives */
};

struct cp_bits {
	BYTE SReset:1;
	BYTE HBAInit:1;
	BYTE ReqSen:1;
	BYTE Scatter:1;
	BYTE Resrvd:2;
	BYTE DataOut:1;
	BYTE DataIn:1;
};

struct  DIRECT_CDB {
    unsigned char   device;     /* device -> chn(4):dev(4) */
    unsigned char   dir;        /* direction-> 0=>no xfr, 1=>IN, 2=>OUT */
    unsigned short  byte_cnt;   /* 64K max data xfr */
    unsigned long	ptr;       /* pointer to the data (in system memory) */
    unsigned char   cdb_len;    /* length of cdb */
    unsigned char   sense_len;  /* length of valid sense information */
    unsigned char   cdb[12];
    unsigned char   sense[64];
    unsigned char   status;
    unsigned char   fill;
/* 88-bytes */
};
/*
 * Controller Command Block
 */
struct dak_ccb {
	union {                        /*** EATA Packet sent to ctlr ***/
	  struct cp_bits bit;          /*                              */
	  unsigned char byte;          /*  Operation Control bits.     */
	} CPop;                        /*                              */
	BYTE CPID;                     /*  Target SCSI ID              */
	BYTE lun;                      /*  LUN                         */
	ULONG CPdataLen;                /*  Transfer Length.            */
	struct dak_ccb *vp;            /*  Command Packet Vir Address. */
	paddr_t CPdataDMA;             /*  Data Physical Address.      */
	BYTE no_breaks;

	BYTE CP_OpCode;
	BYTE CP_Controller_Status;
	BYTE CP_SCSI_Status;
	paddr_t         c_addr;         /* CB physical address          */
	paddr_t         c_cdb_addr;     /* CB dcdb physical address          */
	unsigned short	c_index;	/* CB array index		*/
	unsigned short	c_active;	/* Command sent to controller	*/
	time_t		c_start;	/* Timestamp for start of cmd	*/
	time_t		c_time;		/* Timeout count (msecs)	*/
	struct sb      *c_bind;		/* Associated SCSI block	*/
	struct dak_ccb *c_next;		/* Pointer to next free CB	*/
	struct DIRECT_CDB	dcdb;
};

#define MAX_CMDSZ	12

#define	NO_ERROR	0x00		/* No adapter detected error	*/
#define	NO_SELECT	0x11		/* Selection time out		*/
#define	TC_PROTO	0x14		/* TC protocol error		*/
#define RETRY		0xff		/* Resource error, must retry job */

#define MAX_DMASZ       32
#define pgbnd(a)        (ptob(1) - ((ptob(1) - 1) & (int)(a)))

typedef struct {
	ulong Phy;
	ulong Len;
} SG_vect;

struct ScatterGather {
	unsigned int SG_size;                /* List size (in bytes)        */
	struct   ScatterGather *d_next;      /* Points to next free list    */
	SG_vect  d_list[MAX_DMASZ];
};

typedef struct ScatterGather dak_dma_t;

/*
 * SCSI Request Block structure
 */
struct dak_srb {
	struct xsb     *sbp;		/* Target drv definition of SB	*/
	struct dak_srb *s_next;		/* Next block on LU queue	*/
	struct dak_srb *s_priv;		/* Private ptr for dynamic alloc*/
					/* routines DO NOT USE or MODIFY*/
	struct dak_srb *s_prev;		/* Previous block on LU queue	*/
	dak_dma_t      *s_dmap;		/* DMA scatter/gather list	*/
	paddr_t         s_addr;         /* Physical data pointer        */
	BYTE						no_breaks;      /* No of elements */
	BYTE            s_CPopCtrl;     /* Additional Control info	*/
};

typedef struct dak_srb dak_sblk_t;

#if (PDI_VERSION < 3)
#define bcb_t	char
#define pl_t	int
#endif
/*
 * Logical Unit Queue structure
 */
struct dak_scsi_lu {
	struct dak_srb *q_first;	/* First block on LU queue	*/
	struct dak_srb *q_last;		/* Last block on LU queue	*/
	struct dak_scsi_lu *q_next;	/* Next queue needing service	*/
	int		q_flag;		/* LU queue state flags		*/
	struct sense	q_sense;	/* Sense data			*/
	int		q_count;	/* Outstanding job counter	*/
	void	      (*q_func)();	/* Target driver event handler	*/
	long            q_param;        /* Target driver event param    */
	unsigned char controller;
	bcb_t		*q_bcbp;	/* Breakup controller block ptr */
	char		*q_sc_cmd;	/* SCSI command buffer for pass-thru */
	pl_t		q_opri;		/* System priority held across  */
					/* calls to putq/next		*/
        lock_t          *q_lock;        /* Device Que Lock              */ 
};

#define	DAK_QBUSY		0x01
#define	DAK_QSUSP		0x04
#define	DAK_QSENSE		0x08		/* Sense data cache valid */
#define	DAK_QPTHRU		0x10
#define	DAK_QNEEDSRV		0x20		/* Is on ha service queue */

#define dak_qclass(x)	((x)->sbp->sb.sb_type)
#define	QNORM		SCB_TYPE
#define	HBA_QNORM	SCB_TYPE

#define HBA_PREFIX	dak
#define DAK_NAME	"dak"

#define	FWV2X	0
#define	FWV3X	1
#define	FWVPG	2

/*
 * Function vectors for routines that differ for various bus types:
 *	EISA, MCA, PCI.
 */
struct dak_functions {
	int	(*dak_ha_init)();
	void	(*dak_intr)();
	void	(*dak_send)();
	int	(*dak_iocmd)();
	void	(*dak_halt)();
};
/*
 * Host Adapter structure
 */
struct dak_scsi_ha {
	unsigned short  ha_state;           /* Operational state           */
	unsigned short  ha_id;              /* Host adapter SCSI id        */
	int             ha_vect;            /* Interrupt vector number     */
	unsigned long   ha_base;            /* Base I/O address            */
	int             ha_npend;           /* # of jobs sent to HA        */
	struct dak_ccb  *ha_ccb;	    /* Controller command blocks   */
	struct dak_ccb  *ha_cblist;         /* Command block free list     */
	struct dak_scsi_lu **ha_dev;	    /* Logical unit queues	   */
	struct dak_scsi_lu *ha_common_luq;  /* Dummy Logical unit queue	   */
	struct dak_scsi_lu *ha_que;	    /* Chained queues needing service */
	struct buf* ioctl_que;
        unsigned char tape_channel[MAXTARGET_3X];
	int		ha_maxjobs;		/* Max concurrent cmds	      */
	ushort_t	ha_iobustype;		/* bus type		      */
	uchar_t		ha_itype;		/* interrupt type	      */
        uchar_t         ha_fwtype;	/* Fw typ FWV2X, or FWV3X, or FWVPG */
	caddr_t		ha_mem_base;		/* mailbox address	   */
	int		ha_ntargets;		/* Number of targets	      */
	uchar_t		ha_minor_fwver;		/* firmware minor version Num */
	uchar_t		ha_major_fwver;		/* firmware major version Num */
        uchar_t         ha_readopcode;          /* opcode for READ/READ EXT   */
        uchar_t         ha_writeopcode;         /* opcode for WRITE/WRITE EXT */
        uchar_t         ha_extended_ops;        /* use EXT read/wrt operations*/
        uchar_t         ha_slotnumber;          /* Slot number for the adapter*/
	struct dak_functions ha_func;		/* function vectors for routines
						 that vary w.r.t. bus types*/
	caddr_t		ha_mbx_addr;		/* mailbox address	   */
        lock_t          *ha_ctl_lock;		/* Controller Lock		*/ 
};

#define C_SANITY	0x8000

/*
**	Macros to help code, maintain, etc.
*/

#if (PDI_VERSION >= 4)
#define SUBDEV(b,t,l)   ((b << 9) | (t << 5) | l)
#else
#define SUBDEV(b,t,l)	((t << 3) | l)
#endif
#define PLU_Q(c,b,t,l)	dak_sc_ha[c].ha_dev[SUBDEV(b,t,l)]
#define SLOT_ID_ADDR(s)	((s) * 4096 + 0xC80)

/*
 * Locking Macro Definitions
 *
 * LOCK/UNLOCK definitions are lock/unlock primatives for multi-processor
 * or spl/splx for uniprocessor.
 */

#define DAK_DMALIST_LOCK(p) p = LOCK(dak_dmalist_lock, pldisk)
#define DAK_CCB_LOCK(p) p = LOCK(dak_ccb_lock, pldisk)
#define DAK_CTL_LOCK(p,ha) p = LOCK(ha->ha_ctl_lock, pldisk)
#define DAK_SCSILU_LOCK(p) p = LOCK(q->q_lock, pldisk)

#define DAK_DMALIST_UNLOCK(p) UNLOCK(dak_dmalist_lock, p)
#define DAK_CCB_UNLOCK(p) UNLOCK(dak_ccb_lock, p)
#define DAK_CTL_UNLOCK(p,ha) UNLOCK(ha->ha_ctl_lock, p)
#define DAK_SCSILU_UNLOCK(p) UNLOCK(q->q_lock, p)

/*
 * Locking Hierarchy Definition
 */
#define DAK_HIER	HBA_HIER_BASE	/* Locking hierarchy base for hba */

#endif /* _IO_HBA_DAK_H */
