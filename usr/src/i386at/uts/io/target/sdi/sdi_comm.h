#ifndef _IO_TARGET_SDI_SDI_COMM_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SDI_SDI_COMM_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/sdi/sdi_comm.h	1.68.14.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <fs/buf.h>			/* REQUIRED */
#include <io/target/scsi.h>		/* REQUIRED */
#include <io/target/sdi/dynstructs.h>	/* REQUIRED */
#include <io/target/sdi/sdi_edt.h>	/* REQUIRED */
#include <io/vtoc.h>			/* REQUIRED */
#include <proc/cred.h>			/* REQUIRED */
#include <proc/proc.h>			/* REQUIRED */
#include <util/engine.h>		/* REQUIRED */
#include <util/ksynch.h>		/* REQUIRED */
#include <io/autoconf/resmgr/resmgr.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)
#include <sys/buf.h>			/* REQUIRED */
#include <sys/scsi.h>			/* REQUIRED */
#include <sys/dynstructs.h>		/* REQUIRED */
#include <sys/sdi_edt.h>		/* REQUIRED */
#include <sys/cred.h>			/* REQUIRED */
#include <sys/proc.h>			/* REQUIRED */
#include <sys/vtoc.h>			/* REQUIRED */
#include <sys/engine.h>			/* REQUIRED */
#include <sys/ksynch.h>			/* REQUIRED */
#include <sys/resmgr.h>			/* REQUIRED */

#endif /* _KERNEL_HEADER */


#define	SDI_EXTLUN	0x80		/* Indicates extended logical unit # */

#define	SCB_TYPE	1
#define	ISCB_TYPE	2

		/* mode field */
#define	SCB_WRITE	0x00		/* Non-read job                       */
#define	SCB_READ	0x01		/* Read data job                      */
#define	SCB_LINK	0x02		/* SCSI command linking is used       */
#define	SCB_HAAD	0x04		/* Address supplied by HA             */
#define SCB_PARTBLK	0x08		/* Partial block transfer	      */

		/* completion code field */
#define	SDI_NOALLOC	0x00000000	/* This block is not allocated      */
#define	SDI_ASW		0x00000001	/* Job completed normally           */
#define	SDI_LINKF0	0x00000002	/* Linked command done without flag */
#define	SDI_LINKF1	0x00000003	/* Linked command done with flag    */
#define	SDI_QFLUSH	0xE0000004	/* Job was flushed                  */
#define	SDI_ABORT	0xF0000005	/* Command was aborted              */
#define	SDI_RESET	0xF0000006	/* Reset was detected on the bus    */
#define	SDI_CRESET	0xD0000007	/* Reset was caused by this unit    */
#define	SDI_V2PERR	0xA0000008	/* vtop failed                      */
#define	SDI_TIME	0xD0000009	/* Job timed out                    */
#define	SDI_NOTEQ	0x8000000A	/* Addressed device not present     */
#define	SDI_HAERR	0xE000000B	/* Host adapter error               */
#define	SDI_MEMERR	0xA000000C	/* Memory fault                     */
#define	SDI_SBUSER	0xA000000D	/* SCSI bus error                   */
#define	SDI_CKSTAT	0xD000000E	/* Check the status byte  	    */
#define	SDI_SCBERR	0x8000000F	/* SCB error                        */
#define	SDI_OOS		0xA0000010	/* Device is out of service         */
#define	SDI_NOSELE	0x90000011	/* The SCSI bus select failed       */
#define	SDI_MISMAT	0x90000012	/* parameter mismatch               */
#define	SDI_PROGRES	0x00000013	/* Job in progress                  */
#define	SDI_UNUSED	0x00000014	/* Job not in use                   */
#define	SDI_ONEIC	0x80000017	/* More than one immediate request  */
#define SDI_SFBERR	0x80000019	/* SFB error			    */
#define SDI_TCERR	0x9000001A	/* Target protocol error detected   */
#define SDI_TIME_NOABORT 0xD000001B	/* Job timed out, but could not be aborted */
#define SDI_SBRESC	0xA000001C	/* SCSI bus reservation conflict    */

#define	SDI_ERROR	0x80000000	/* An error was detected         */
#define	SDI_RETRY	0x40000000	/* Retry the job                 */
#define	SDI_MESS	0x20000000	/* A message has been sent       */
#define	SDI_SUSPEND	0x10000000	/* Processing has been suspended */

#define	SFB_TYPE	3

	/* Defines for the command field */
#define	SFB_NOPF	0x00		/* No op function                  */
#define	SFB_RESETM	0x01		/* Send a bus device reset message */
#define	SFB_ABORTM	0x02		/* Send an abort message           */
#define	SFB_FLUSHR	0x03		/* Flush queue request             */
#define SFB_RESUME	0x04		/* Resume the normal job queue	   */
#define SFB_SUSPEND	0x05		/* Suspend the normal job queue    */
#define SFB_ADD_DEV     0x06	        /* Hot add a device */
#define SFB_RM_DEV      0x07	        /* hot remove a device */
#define SFB_PAUSE       0x08	        /* pause all activity on the SCSI bus */
#define SFB_CONTINUE    0x09	        /* continue after a pause */
#define SFB_TIMEOUT_OFF 0x0A		/* disable timeouts	  */
#define SFB_TIMEOUT_ON	0x0B		/* enable timeouts 	  */
#define SFB_RESET_BUS	0x0C		/* reset the bus 	  */
#define SFB_RESET_DEVICE	0x0D	/* reset a device  */

#define SDI_386_AT      0x06
#define SDI_386_MCA     0x07
#define SDI_386_EISA    0x08

#define SDI_BASIC1              0x0001
#define SDI_FLT_HANDLING        0x0002

	/* Return values for sdi_send and sdi_icmd */
#define SDI_RET_OK 	0
#define SDI_RET_ERR 	-1
#define SDI_RET_RETRY	1

	/*
	 * Return values for sdi_check_dev.
	 * They translate from the INQUIRY Peripheral.
	 */
#define SDI_INQ_UNSUPPORTED	-1
#define SDI_INQ_CONNECTED	0
#define SDI_INQ_DISCONNECTED	1

	/* Ioctl command codes */
#define SDI_SEND	0x0081		/* Send a SCSI command		*/
#define SDI_TRESET	0x0082		/* Reset a target controller	*/
#define SDI_BRESET	0x0084		/* Reset the SCSI bus		*/
#define HA_VER		0x0083		/* Get the host adapter version */
#define SDI_RESERVE     0x0085          /* Reserve the device           */
#define SDI_RELEASE     0x0086          /* Release the device           */
#define SDI_RESTAT      0x0087          /* Device Reservation Status    */
#define HA_GETPARMS	0x008a		/* Get HBA disk geometry        */
#define IHA_GETPARMS	0x008b		/* Get HBA disk geometry        */
#define HA_SETPARMS	0x008c		/* Set HBA disk geometry	*/
#define IHA_SETPARMS	0x008d		/* Set HBA disk geometry	*/
#define HA_GETPPARMS	0x008e		/* Get real disk geometry       */

	/* SDI ioctl prefix for hba specific ioctl's */

#define	SDI_IOC		(('S'<<24)|('D'<<16)|('I'<<8))
	/*
	 * The following ioctl is optionally implemented by
	 * an HBA to provice the caller with the HBA module name
	 * (taken from the idata structure).
	 */
#define SDI_HBANAME		((SDI_IOC)|0x14)    /* Get HBA module name */

	/* The following ioctl's are hba specific ioctl's. There is */
	/* NO GUARANTEE that a given ioctl will have the same */
	/* function across hba driver's. They are intended to allow */
	/* the hba vendors to maintain binary compatiblity across */
	/* future releases of Unixware. */

#define	SDI_IOC_HBA_IOCTL_00	((SDI_IOC)|0x00)
#define	SDI_IOC_HBA_IOCTL_01	((SDI_IOC)|0x01)
#define	SDI_IOC_HBA_IOCTL_02	((SDI_IOC)|0x02)
#define	SDI_IOC_HBA_IOCTL_03	((SDI_IOC)|0x03)
#define	SDI_IOC_HBA_IOCTL_04	((SDI_IOC)|0x04)
#define	SDI_IOC_HBA_IOCTL_05	((SDI_IOC)|0x05)
#define	SDI_IOC_HBA_IOCTL_06	((SDI_IOC)|0x06)
#define	SDI_IOC_HBA_IOCTL_07	((SDI_IOC)|0x07)
#define	SDI_IOC_HBA_IOCTL_08	((SDI_IOC)|0x08)
#define	SDI_IOC_HBA_IOCTL_09	((SDI_IOC)|0x09)
#define	SDI_IOC_HBA_IOCTL_0A	((SDI_IOC)|0x0A)
#define	SDI_IOC_HBA_IOCTL_0B	((SDI_IOC)|0x0B)
#define	SDI_IOC_HBA_IOCTL_0C	((SDI_IOC)|0x0C)
#define	SDI_IOC_HBA_IOCTL_0D	((SDI_IOC)|0x0D)
#define	SDI_IOC_HBA_IOCTL_0E	((SDI_IOC)|0x0E)
#define	SDI_IOC_HBA_IOCTL_0F	((SDI_IOC)|0x0F)

        /* AEN handler flags */
#define SDI_FLT_RESET   0x00000001      /* logical unit was reset       */
#define SDI_FLT_PTHRU   0x00000002      /* pass through was used        */
#define SDI_TARMOD_WRCOMP   0x00000003  /* target mode write completed	*/

#define	SDI_HAID_PARAM	"SDI_HA_ID"	/* parameter name for autoconfig */
#define SDI_CHANID_PARAM "SDI_CHANID_X"	/* SCSI channel ID 	 	 */

	/* delay period for sdi_cklu drv_usecwait */
#define SDI_CKLU_DELAY	(1 * 1000000)	/* 1 second 			*/

struct sdi_extended_adr {
	int	version;
	struct  scsi_adr scsi_adr; /* SCSI ctl/target/lun/bus      */
};

struct scsi_ad{
	int   pdi_adr_version;
	struct   sdi_extended_adr *extended_address;
	unsigned char	sa_lun;		/* logical unit number          */
	unsigned char	sa_bus:	 3;	/* bus number			*/
	unsigned char	sa_exta: 5;	/* extended target number	*/
	short		sa_ct;		/* Controller/target number	*/
					/*	target 3;		*/
					/*	controller 5;		*/
};
#define sa_fill sa_ct
#define sc_priv sc_extra
	
struct scb{
	unsigned long	sc_comp_code;	/* Current job status              */
	void		*sc_extra;	/* New information		   */
					/* DO NOT USE or MODIFY		   */
	void		(*sc_int)();	/* Target Driver interrupt handler */
	caddr_t		sc_cmdpt;	/* Target command                  */
	caddr_t		sc_datapt;	/* Data area			   */
	long		sc_wd;		/* Target driver word              */
	time_t		sc_time;	/* Time limit for job              */
	struct scsi_ad	sc_dev;		/* SCSI device address             */
	unsigned short	sc_mode;	/* Mode flags for current job      */
	unsigned char	sc_status;	/* Target status byte              */
	char		sc_fill;	/* Fill byte                       */
	struct sb	*sc_link;	/* Link to next scb command        */
	long		sc_cmdsz;	/* Size of command                 */
	long		sc_datasz;	/* Size of data			   */
	long		sc_resid;	/* Bytes to xfer after data 	   */
	clock_t		sc_start;	/* Start time (job to controller)  */
};

struct sfb{
	unsigned long	sf_comp_code;	/* Current job status              */
	char		*sf_priv;	/* private ptr for Dyn alloc routines*/
					/* DO NOT USE or MODIFY		   */
	void		(*sf_int)();	/* Target Driver interrupt handler */
	struct scsi_ad	sf_dev;		/* SCSI device address             */
	unsigned long	sf_func;	/* Function to be performed        */
	int		sf_wd;		/* Target driver word		   */    
};

struct ver_no {
	unsigned char	sv_release;	/* The release number */
	unsigned char	sv_machine;	/* The running machine */
	short		sv_modes;	/* Supported modes     */
};

struct sb {
	unsigned long	sb_type;
	union{
		struct scb	b_scb;
		struct sfb	b_sfb;
	}sb_b;
};


#define SCB sb_b.b_scb
#define SFB sb_b.b_sfb

extern int sdi_sleepflag;	/* KMEM_NOSLEEP if still in init/start;
				 * KMEM_SLEEP after that
				 */

/*
 * owner & edt lists are protected by either one of sdi_rinit_lock or
 *	sdi_edt_mutex for read;  must hold both to write.
 */
struct owner {
	struct owner *next;		/* next owner of the following edtp */
	struct sdi_edt *edtp;		/* back ptr to edt entry */
	struct drv_majors maj;		/* owner's first major numbers */
	void	(*fault)();		/* routine to call after asynch event */
	int     (*target_rm_dev)(); 	/* target driver func to disclaim dev */
	long	flt_parm;		/* parameter to (*fault)() */
	struct owner *target_link;	/* target driver's list of owners */
	struct sdi_driver_desc	*ow_sddp;	/* sdi driver desc */
	char	*name;			/* owner's name */
};

#define SDI_CLAIM	0x01		/* claim a device for driver access */
#define SDI_ADD		0x02		/* add an owner block to list */
#define SDI_DISCLAIM	0x04		/* release claim to device */
#define SDI_REMOVE	0x08		/* remove driver from owner list */

/*
 * hba_no, scsi_id, lun, iotype are read-only
 * owner & edt lists are protected by sdi_rinit_lock and sdi_edt_mutex
 *	as described above.
 * pdtype and inquiry protected by sdi_edt_mutex
 */
struct sdi_edt {
	struct sdi_edt *hash_p;		/* Next sdi_edt in hash list */
	short		hba_no;		/* HBA/path id (keep for binary comp) */
	uchar_t		scsi_id;	/* SCSI id (keep for binary comp) */
	uchar_t		lun;		/* Logical unit#(keep for binary comp)*/
	struct owner	*curdrv;	/* Pointer to current owner */
	struct owner	*owner_list;	/* Chain of owners of this edt */
	struct sdi_device	*se_devicep;		/* ptr to device description for this entry */
	int		pdtype;		/* SCSI periperial device type */
	uchar_t		iotype;		/* I/O capability such as DMA, PIO */
	char	inquiry[INQ_EXLEN];	/* INQUIRY string returned */
	struct scsi_adr	scsi_adr;	/* SCSI ctl/target/lun/bus	*/
	ulong_t		memaddr;	/* Controller memory (ROM BIOS) addr */
	uchar_t		ctlorder;	/* Controller order (if specified by */
					/*    HBA).			     */
	struct ident    edt_ident;      /* INQUIRY Data structure */
	ushort_t	*timeout;	/* Timeout values per command */
	unsigned int	ha_chan_id;	/* Host Adapter channel ID	*/
	unsigned int	ex_iotype;	/* Extended io type 	*/
	struct pd_stamp	stamp;		/* Device stamp */
};

/* The following defines are for the sdi_edt iotype field */
#define F_DMA	 	0x001		/* Device is a DMA device */
#define F_PIO	 	0x002		/* Device is a progammed I/O device */
#define F_SCGTH	 	0x004		/* Device supports scatter-gather DMA */
#define F_RMB		0x008		/* Device is removable media */

#define F_DMA_24	F_DMA		/* Device supports 24-bit DMA */
#define F_DMA_32	0x010		/* Device supports 32-bit DMA */
#define F_HDWREA	0x020		/* Device supports hardware reassign */
#define F_RESID		0x040		/* Dev returns sc_resid, residual cnt*/
#define F_EXIOTYPE	0x080		/* Dev has extended info	*/

/* The following defines are for the sdi_edt ex_iotype field */
#define F_TARGETMODE 	0x001		/* Target mode supported */

struct sdi_event	{
	int			event_type;
	int			(*event_handler)();
	void			*event_info;
	int			event_pdtype;
	char			event_inquiry[INQ_EXLEN];
	struct scsi_adr		event_scsi_adr;
	struct sdi_event	*event_next;
};

/*
 * Event handler function is called as
 * 	(*event_handler)(sdi_event *event, struct scsi_adr *sap, struct sb *sbp)
 */

/* sdi_event_list protected by sdi_edt_mutex */
struct sdi_event_list	{
	struct sdi_event_list	*next;
	struct sdi_event	event;
};

/*
 * Target driver event types
 */
#define	SDI_FIRSTOPEN		1
#define	SDI_LASTCLOSE		2
#define	SDI_LASTCLOSE_ERR	3


#if defined(_KERNEL) || defined(_KMEMUSER)

/* The following structure contains information that was
 * added after the structure of the sb was cast in stone
 * by compatibility concerns.  Any new field going into
 * the sb should be put in here.  If drivers must have
 * access to the field, sdi should provide an interface
 * so that size/field offset assumptions are not buried in
 * the driver.
 */

struct sb_extra {
	struct sense sb_sense;
	unsigned long sb_data_type;
	paddr_t *sb_datapt;
	buf_t *sb_bufp;
	void *sb_idatap;  /* pointer to target-driver specific instance pointer */
};

struct xsb {
	struct sb sb;			/* actual sb */
	struct hbadata *hbadata_p;	/* ptr to HBA driver's private data */
	struct owner *owner_p;		/* target driver's owner block */
	struct sb_extra extra;		/* Any further information goes here */
};


#define NOWNER		28		/* only old target drivers need these */

struct hbadata {
	struct xsb *sb;
	/* addition driver dependent stuff here */
};

struct hbagetinfo {	/* structure used to pass up hba-specific data from
			   hba to sdi, using p##getinfo() function. */
	char *name;	/* Name */
	char iotype;	/* Specifies the type of I/O device is capable of */
			/* i.e. DMA, PIO, scatter/gather, etc */
	bcb_t *bcbp;	/* Breakup control block defining device properties */
	unsigned int ex_iotype;	/* Extended capability flag */
};

#define	HBA_INFO(p, f, x)	\
	long		p##freeblk();	\
	struct hbadata	* p##getblk();	\
	long		p##icmd();	\
	void		p##getinfo();	\
	long		p##send();	\
	int		p##xlat();	\
	int		p##open();	\
	int		p##close();	\
	int		p##ioctl();	\
	struct	hba_info	p##hba_info	= { \
		f, x,	\
		p##freeblk,	\
		p##getblk,	\
		p##icmd,	\
		p##getinfo,	\
		p##send,	\
		p##xlat,	\
		p##open,	\
		p##close,	\
		p##ioctl	\
	}


#define DEFAULT_MAX_XFER        0x10000

/*
 * Per-module HBA information.
 */
struct	hba_info	{
	int	*hba_flag;
	ulong	max_xfer;
	/*
	 * Entry points.
	 */
	long		 (*hba_freeblk)();
	struct	hbadata	*(*hba_getblk)();
	long		 (*hba_icmd)();
	void		 (*hba_getinfo)();
	long		 (*hba_send)();
	int		 (*hba_xlat)();
	int		 (*hba_open)();
	int		 (*hba_close)();
	int		 (*hba_ioctl)();
};

/*
 * Values for hba_flag.
 */
#define	HBA_MP		0x01	/* HBA driver is MP */
#define HBA_HOT 	0x02	/* driver supports Hot insertion/removal */
#define HBA_TIMEOUT	0x04	/* driver supports command timeouts */
#define HBA_TIMEOUT_RESET 0x08	/* driver supports new command timeouts */
#define HBA_CALLBACK	0x10	/* driver calls the callback routine on */
				/* job completion during initialization */

#define SDI_TIMEOUT_VALUES 256

/*
 * Per-instance HBA information.
 */
#define SDI_MAX_CHANS	8	/* Maximum SCSI bus channels supported */

struct	hba_idata_v5	{
	int		version_num;	/* version num to determine contents */
					/* of idata struct by sdi driver     */
	char		*name;
	unsigned char	ha_id;
	ulong		ioaddr1;
	int		dmachan1;
	int		iov;
	int		cntlr;
	int		active;
					/*      UnixWare 2.0            */
	ulong_t		idata_memaddr;	/* ROM BIOS start address       */
	uchar_t		idata_ctlorder;	/* Controller order (for boot, etc)*/
	uchar_t		idata_nbus;	/* number of SCSI buses supported*/
	ushort_t	idata_ntargets;	/* number targets supported     */
	ushort_t	idata_nluns;	/* number of luns supported     */
	rm_key_t	idata_rmkey;	/* autoconfig resource manager key */
	void		*idata_intrcookie;	/* used by cm_intr_detach() */
	int		idata_cpubind;	/* CPU number, if bound         */
					/*      UnixWare 2.1            */
	char		ha_chan_id[SDI_MAX_CHANS];	/* SCSI bus ID's */
	ulong		ioaddr2;	/* secondary ioaddr */
};

struct	hba_idata_v4	{
	int		version_num;	/* version num to determine contents */
					/* of idata struct by sdi driver     */
	char		*name;
	unsigned char	ha_id;
	ulong		ioaddr1;
	int		dmachan1;
	int		iov;
	int		cntlr;
	int		active;
					/* 	UnixWare 2.0	 	*/
	ulong_t		idata_memaddr;	/* ROM BIOS start address	*/
	uchar_t		idata_ctlorder;	/* Controller order (for boot, etc)*/
	uchar_t		idata_nbus;	/* number of SCSI buses	supported*/
	ushort_t	idata_ntargets;	/* number targets supported	*/
	ushort_t	idata_nluns;	/* number of luns supported	*/
	rm_key_t	idata_rmkey;	/* autoconfig resource manager key */
	void		*idata_intrcookie;	/* used by cm_intr_detach() */
	int		idata_cpubind;	/* CPU number, if bound		*/
};

/*
 * Per-instance HBA information, structure version 1-3
 */
struct	hba_idata {
	int		version_num;	/* version num to determine contents */
					/* of idata struct by sdi driver     */
	char		*name;
	unsigned char	ha_id;
	ulong		ioaddr1;
	int		dmachan1;
	int		iov;
	int		cntlr;
	int		active;
};

/*
 * Extended HBA information.
 */
struct hba_ext_info	{
	int	hei_version;	/* version of this structure */
	int	hei_cpu;	/* CPU to bind HBA to */
};

/*
 * HBA_IDATA_STRUCT
 * Macro that is also defined in hba.h, but since hba.h is not
 * yet included we define it here also...
 */
#ifndef HBA_IDATA_STRUCT
#define HBA_IDATA_STRUCT struct hba_idata_v5
#endif

/*
 * hba_idata version number definitions
 */
#ifndef HBA_IDATA_INC_PTR
#define	HBA_SVR4_2	1	/* SVR4.2 driver release number */
#define	HBA_SVR4_2_2	2	/* SVR4.2.2 driver release number */
#define	HBA_SVR4_2MP	3	/* SVR4.2MP driver release number */
#define HBA_UW20_IDATA  4       /* Unixware 2.0 idata (ver. 4)  */
#define HBA_UW21_IDATA  5       /* Unixware 2.1 idata (ver. 5)  */

#define	HBA_VMASK	0xffff	/* mask out flags to get version number */

/*
 * 
 */
#define SET_IN_TARGET 0x1

/*
 * HBA_IDATA_INC_PTR
 * Macro that bumps the idata struct pointer depending on
 * the version number. This provides compatability with
 * drivers using a version 4 idata struct in Unixware 2.1.
 */
#define HBA_IDATA_INC_PTR(x) \
        (x) = (HBA_IDATA_STRUCT *)((char *)(x) +                \
              (((x)->version_num & HBA_VMASK) == HBA_UW21_IDATA ? \
              sizeof(struct hba_idata_v5) : sizeof(struct hba_idata_v4)))
#endif

/*
 * hba_idata version_num flags.
 */
#define	HBA_IDATA_EXT	0x10000	/* expect extended information at end of idata */
#define	HBA_EXT_INFO	0x20000	/* record contains pointer to hba_ext_info */
#define	HBA_AUTOCONF	0x40000 /* idata array has been allocated by autoconfig */
#define	HBA_IDATA_ALLOC	0x80000 /* idata allocated by sdi_idata_alloc */
#define HBA_EXT_ADDRESS 0x01000000 /* This HBA is using the extended addressing macros */


#define	IDP(p)	((HBA_IDATA_STRUCT *)(p))
#define	HIP(p)	((struct hba_info *)(p))

struct	hba_cfg	{
	struct	hba_info	*info;
	HBA_IDATA_STRUCT	*idata;
	ulong	active;
};

/*
 * structure defining info passed to aio/sdi_breakup for the purpose
 * of preparing the i/o request to match the device capability.
 */
struct sdi_devinfo {
	void	(*strat)();	/* strategy routine		*/
	size_t	max_xfer;	/* Max transfer size (bytes)	*/
	uint	iotype;		/* I/O capability		*/
	size_t	granularity;	/* multiple of, and alignment	*/
};

/*
 * Shadow HBAtbl for protection of uniprocessor
 * HBA drivers.
 */
struct	shad_hbatbl	{
	struct	hba_info *info;
	engine_t *enginep;
};
#endif /* _KERNEL || _KMEMUSER */

#define HBA_END_SENTINAL	"end sentinal"

/* HBA_tbl, sdi_rinits are protected by sdi_rinit_lock */
extern struct hba_cfg	*HBA_tbl;
extern void		(**sdi_rinits)();

struct dev_cfg {
	ulong_t	match_type;		/* action to take on a match */
	ulong_t hba_no;			/* match HBA, -1 if not used */
	ulong_t	scsi_id;		/* match SCSI id, 
						0xffffffff if not used */
	ulong_t	lun;			/* match LUN, 0xffffffff if not used */
	uchar_t devtype;		/* target device type */
	int	inq_len;		/* length of inquiry string to match */
	char	inquiry[INQ_EXLEN];	/* inquiry string to match */
	ulong_t	bus;			/* match BUS, 0xffffffff if not used */
};

#define DEV_CFG_UNUSED		0xffffffff
struct dev_spec {
	char	inquiry[INQ_LEN];	/* device specification */
	int	(*first_open)();	/* first open support */
	int	(*last_close)();	/* last close support */
	void	(*intr)();		/* interrupt support */
	ulong	cmd_sup[8];		/* bitmap of recognized commands */
	ulong	cmd_chk[8];		/* bitmap of cmds passed to *command */
	void	(*command)();		/* SCSI command helper */
};

/*
 * This is the structure for the table of targets that require special treatment
 * when probing for luns. 
 */
struct sdi_blist {
	char	*vendor;	/* Vendor id (directly from the inquiry data) */
	char	*prod;		/* Product identification */
	char	*rev;		/* Product revision level */
	int	lun;		/* lun Upper bound */
};

#define CMD_CHK(scsi_cmd, dev_specp)				\
		((dev_specp)->cmd_chk[ ((unsigned char)scsi_cmd)>>5 ] &	\
			(1 << ((((unsigned char)scsi_cmd) & 0x1f) -1) ))

#define CMD_SUP(scsi_cmd, dev_specp)				\
		((dev_specp)->cmd_sup[ ((unsigned char)scsi_cmd)>>5 ] &	\
			(1 << ((((unsigned char)scsi_cmd) & 0x1f) -1) ))

/*
 * This is the structure that defines the precedence order of each
 * layer in the SDI I/O stack.
 */
typedef struct sdi_layer_list {
	char	*layer_long_name;	/* descriptive name for messages */
	char	*layer_short_name;	/* shorthand name for messages */
	int 	layer_precedence;	/* precedence order of this layer */
	struct	sdi_layer *layer_info; /* everything else about the layer */
} sdi_layer_list_t ;

#if defined(__STDC__)
extern struct owner 	*sdi_doconfig(struct dev_cfg[], int, char *, 
				struct drv_majors *, void (*)());
extern struct owner	*sdi_unlink_target(struct owner *, struct owner *);
extern struct dev_spec	*sdi_findspec(struct sdi_edt *, struct dev_spec *[]);
#else
struct owner	*sdi_doconfig();	/* configure devices		*/
struct owner	*sdi_unlink_target();	/* unlink item on target_link list */
struct dev_spec *sdi_findspec();	/* find a dev_spec		*/
#endif

/*
 * the host adapter minor device number is interpreted as follows:
 *
 *           MAJOR           MINOR      
 *      -------------------------------
 *      |  mmmmmmmm  |  ccc  ttt ll   |
 *      -------------------------------
 *      
 *         m = major number assigned by idinstall
 *	   c = Host Adapter Card number (0-7)
 *         t = target controller ID (0-7)
 *         l = logical unit number (0-3)
 *
 */

extern int sdi_major;

/* 
 * These macros convert device to SCSI controller/target/lun
 * and take a dev_t as an argument.
 * The limitations assumed by these macros are 8/8/4 respectively.
 *
 * NOTE: THESE MACROS SHOULD NO LONGER BE USED.
 *	THE 'EX' MACROS REPLACE THEM, AND EXTEND THE DEFINITION OF 
 *	THE CONTROLLER/TARGET/LUN.
 */
#define SC_HAN(dev)	((geteminor(dev) >> 5) & 0x07)
#define SC_TCN(dev)	((geteminor(dev) >> 2) & 0x07)
#define SC_LUN(dev)	((geteminor(dev) & 0x03))

#define	SDI_HAN(x)	(((x)->sa_fill >> 3) & 0x07)
#define	SDI_TCN(x)	((x)->sa_fill & 0x07)
#define SDI_LUN(x)	((x)->sa_lun)
/* ... END (THESE MACROS SHOULD NO LONGER BE USED.) */

/*
 * These macros convert minor number to SCSI controller/target/lun/bus
 * and take the pass-through minor number as an argument: geteminor(dev_t).
 * The limitations assumed by these macros are 32/32/32/8 respectively.
 * These macros are upward compatible with SC_HAN/SC_TCN/SC_LUN and
 * should be used going forward.
 */
#define SC_EXHAN(minor)	(( minor>>5) & 0x1f)
#define SC_EXTCN(minor)	(((minor>>2) & 0x07) | ((minor>>7)  & 0x18))
#define SC_EXLUN(minor)	(( minor     & 0x03) | ((minor>>10) & 0x1C))
#define SC_BUS(minor)	(( minor>>15 & 0x07))

/*
 * These macros convert scsi_ad to SCSI controller/target/lun/bus
 * and take the scsi_ad pointer as an argument.
 * The limitations assumed by these macros are 32/32/32/8 respectively.
 * These macros are upward compatible with SDI_HAN/SDI_TCN/SDI_LUN and
 * should be used going forward.
 */
#define SDI_EXHAN(x)	(((x)->sa_ct>>3) & 0x1f)
#define SDI_EXTCN(x)	((x)->sa_exta)
#define SDI_EXLUN(x)	((x)->sa_lun)
#define SDI_BUS(x)	((x)->sa_bus)

/*
 * These are the new macros for the extended address format
 */
#define SDI_HAN_32(x)	((x)->extended_address->scsi_adr.scsi_ctl)
#define SDI_TCN_32(x)   ((x)->extended_address->scsi_adr.scsi_target)
#define SDI_LUN_32(x)	((x)->extended_address->scsi_adr.scsi_lun)
#define SDI_BUS_32(x)	((x)->extended_address->scsi_adr.scsi_bus)


/*
 * Misc macros
 */
#define SDI_CONTROL(x)	(((x)>>3) & 0x1f)
#define SDI_SA_CT(c,t)	(((c)<<3) | ((t) & 0x07))

#define SDI_PADDR_SIZE(x) (((((x)->phys_dmasize)+31)/32) * sizeof(paddr32_t))

/*
 * flags used by HBAs to find if a buffer pointer or
 * a physical address pointer is passed down in
 * the sb_extra field. 
 * the sdi_data_type() function can be used to retrieve
 * the type
 * the function sdi_datapt_ptr() retrieves the physical
 * address pointer
 * the function sdi_buf_ptr() retrieves the buffer pointer
 */
#define SDI_PHYS_ADDR	1
#define SDI_IO_BUFFER   2


/*
 * Check for SCSI address comparison - scsi_ad compared to scsi_adr
 */
#define SDI_ADDRCMP(X,Y) \
	(SDI_EXHAN(X) == (Y)->scsi_ctl && \
	 SDI_EXTCN(X) == (Y)->scsi_target && \
	 SDI_EXLUN(X) == (Y)->scsi_lun && \
	 SDI_BUS(X)   == (Y)->scsi_bus)

#define SDI_ADDR_MATCH(X,Y) \
        ((X)->scsi_ctl    == (Y)->scsi_ctl && \
         (X)->scsi_target == (Y)->scsi_target && \
         (X)->scsi_lun    == (Y)->scsi_lun && \
         (X)->scsi_bus    == (Y)->scsi_bus)

/*
 * Check for an illegal device
 */
#define	SDI_ILLEGAL(c,t,l)    (!sdi_redt(c,t,l))
#define	SDI_EXILLEGAL(c,t,l,b) (!sdi_rxedt(c,b,t,l))
#define SDI_MAX_HBAS	MAX_EXHAS	/* Maximum number of HBAs */

/*
 *	message format types for sdi_errmsg
 */
#define SDI_SFB_ERR		0
#define SDI_CKCON_ERR	1
#define SDI_CKSTAT_ERR	2
#define SDI_DEFAULT_ERR	3
#define SDI_NAMESZ	49

/*
 * sizes for small and large pools
 */
#define	SM_POOLSIZE	28	/* sizeof(struct dcdblk), but we don't
				 * want to include that header here
				 */
#define	LG_POOLSIZE	sizeof(struct xsb)

/*
 * only keep for backwards compat 
 */
#define	SDI_GET(p, a)	kmem_zalloc((p)->f_isize, a)
#define	SDI_FREE(p, a)	kmem_free(a, (p)->f_isize)

#if defined(_KERNEL) 

extern int 		sdi_access(struct sdi_edt *, int, struct owner *);
extern void		sdi_aen(int, int, int, int);
extern void 		sdi_blkio(buf_t *, unsigned int, void (*)());
extern int		sdi_edtindex(int);
extern void 		sdi_callback(struct sb *);
extern void 		sdi_clrconfig(struct owner *, int, void (*)());
extern struct owner	*sdi_doconfig(struct dev_cfg[], int, char *, 
				struct drv_majors *, void (*)());
extern struct owner	*sdi_unlink_target(struct owner *, struct owner *);
extern void		sdi_errmsg(char *, struct scsi_ad *, struct sb *,
				struct sense *, int, int);
extern struct dev_spec	*sdi_findspec(struct sdi_edt *, struct dev_spec *[]);
extern void		sdi_free(struct head *, struct jpool *);
extern long		sdi_freeblk(struct sb *);
extern long		sdi_xfreeblk(int, struct sb *);
extern struct jpool	*sdi_get(struct head *, int);
extern struct sb	*sdi_getblk(int);
extern struct sb	*sdi_xgetblk(int, int);
extern void		sdi_getdev(struct scsi_ad *, dev_t *);
extern int		sdi_gethbano(int);
extern int		sdi_hba_flag(int);
extern int		sdi_ext_address(int);
extern int		sdi_hba_version(int);
extern int		sdi_register(void *, void *);
extern int		sdi_icmd(struct sb *, int);
extern int		sdi_xicmd(int, struct sb *, int);
extern void		sdi_init(void);
extern void		sdi_name(struct scsi_ad *, char *);
extern void		sdi_xname(int, struct scsi_ad *, char *);
extern void		sdi_poolinit(struct head *);
extern int		sdi_send(struct sb *, int);
extern int		sdi_xsend(int, struct sb *, int);
extern unsigned short	sdi_swap16(uint);
extern int		sdi_swap24(uint);
extern long		sdi_swap32(ulong);
extern struct sdi_edt	*sdi_redt(int, int, int);
extern struct sdi_edt	*sdi_rxedt(int, int, int, int);
extern void		sdi_name(struct scsi_ad *, char *);
extern void             sdi_target_hotregister(int (*)(), struct owner *);
extern int		sdi_translate(struct sb *, int, proc_t *, int);
extern int		sdi_xtranslate(int, struct sb *, int, proc_t *, int);
extern int		sdi_wedt(struct sdi_edt *, int, char *);

extern int		sdi_open(dev_t *, int, int, cred_t *);
extern int		sdi_close(dev_t , int, int, cred_t *);
extern int		sdi_ioctl(dev_t, int, caddr_t, int, cred_t *, int *);
extern bcb_t		*sdi_getbcb(struct scsi_ad *, int);
extern bcb_t		*sdi_xgetbcb(int, struct scsi_ad *, int);
extern void		 sdi_freebcb(bcb_t *bcbp);

extern HBA_IDATA_STRUCT	*sdi_hba_autoconf(char *, HBA_IDATA_STRUCT *, int *);
extern int		sdi_hba_getconf(rm_key_t, HBA_IDATA_STRUCT *);
extern void		sdi_acfree(HBA_IDATA_STRUCT *, int);
extern void		sdi_intr_attach(HBA_IDATA_STRUCT *, int, void (*)(), int);
extern HBA_IDATA_STRUCT	*sdi_idata_alloc(rm_key_t, const HBA_IDATA_STRUCT *);
extern void		sdi_idata_free(HBA_IDATA_STRUCT *);
extern int		sdi_deregister(HBA_IDATA_STRUCT *);
extern int		sdi_idata_modify(HBA_IDATA_STRUCT *, rm_key_t);

extern struct sense *	sdi_sense_ptr(struct sb *);
extern ulong		sdi_data_type(struct sb *);
extern paddr32_t *      sdi_datapt_ptr(struct sb *);
extern buf_t *          sdi_buf_ptr(struct sb *);
extern void *sdi_get_idata_ptr(struct sb *);
extern void sdi_set_idata_ptr(struct sb *, void *);
extern void *		sdi_kmem_alloc_phys(size_t, const physreq_t *, int);

extern int sdi_device_reset(struct scsi_adr *, struct sb *);
extern int sdi_bus_reset(struct scsi_adr *, struct sb *);
extern int sdi_start_gauntlet(struct sb *, void (*)());
extern void sdi_end_gauntlet(struct scsi_adr *);

extern int pdi_timeout;

/*
 * To build drivers compatible with non-autoconfig kernels,
 * define SDI_NOAC_COMPAT. Drivers so compiled must be prepared
 * for these addresses being 0.
 */
#if	defined(SDI_NOAC_COMPAT) && !defined(_SDI_AUTOCONF)
#pragma	weak	sdi_hba_autoconf
#pragma	weak	sdi_hba_getconf
#pragma	weak	sdi_acfree
#pragma	weak	sdi_intr_attach
#endif	/* SDI_NOAC_COMPAT && !_SDI_AUTOCONF */

extern int		sdi_addevent(struct sdi_event *);
extern struct sdi_event	*sdi_event_alloc(int);
extern void		sdi_event_free(struct sdi_event *);
extern int		sdi_rmevent(struct sdi_event *);
extern int		sdi_notifyevent(int, struct scsi_adr *, struct sb *);
extern toid_t		sdi_timeout(void (*)(), void *, long, pl_t, 
				struct scsi_ad *);

#endif /* _KERNEL */
#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_SDI_SDI_COMM_H */
