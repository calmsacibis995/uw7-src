#ifndef _IO_TARGET_SCSI_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SCSI_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/scsi.h	1.12.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*==========================================================================*/
/* scsi.h - SCSI Header File						    */
/*==========================================================================*/

/* Operational codes for group zero commands which are used for control */

#define SS_TEST		0X00		/* Test unit ready             */
#define SS_REQSEN	0X03		/* Request sense               */
#define SS_READ		0X08		/* Read                        */
#define SS_WRITE	0X0A		/* Write                       */
#define SS_INQUIR	0X12		/* Inquire                     */
#define SS_MSELECT	0X15		/* Mode select                 */
#define SS_RESERV	0X16		/* Reserve unit                */
#define SS_RELES	0X17		/* Release unit                */
#define SS_MSENSE	0X1A		/* Mode Sense                  */
#define SS_SDDGN	0X1D		/* Send diagnostic             */
#define SS_LOCK		0X1E		/* Prevent/allow media removal */

/* Disk specific group zero commands */

#define SS_REZERO	0X01		/* Rezero Unit                 */
#define SS_REASGN	0X07		/* Reassign blocks             */
#define SS_SEEK		0X0B		/* Seek                        */
#define SS_ST_SP	0X1B		/* Start/Stop unit             */

/* Tape specific group zero commands */

#define SS_REWIND 	0X01		/* Rewind 		       */
#define	SS_RDBLKLEN	0X05		/* Read block length limits	   */
#define SS_TRKSEL 	0X0B		/* Track select                */
#define SS_FLMRK  	0X10		/* Write filemarks	       */
#define SS_SPACE  	0X11		/* Space 	               */
#define SS_ERASE  	0X19            /* Erase                       */
#define SS_LOAD   	0X1B		/* Load/unload                 */

/* Op codes for group 1 commands  */

#define SM_RDCAP	0X25		/* Read capacity      */
#define SM_READ		0X28		/* Read extended      */
#define SM_WRITE	0X2A		/* Write extended     */
#define SM_SEEK		0X2B		/* Seek extended      */
#define SM_SYNC         0X35	        /* Synchronize cache */
#define SM_RDDL		0X37		/* Read defect list   */
#define SM_RDDB		0X3C		/* Read data buffer   */
#define SM_WRDB		0X3B		/* Write data buffer  */

/* Op codes for Medium Changers */
#define       SS_INIT_STATUS  0x07            /* Initialize Element Status    */
#define       SS_MOVE_MEDIUM  0xA5            /* Move medium request          */
#define       SS_POSITION     0x2B            /* Position To Element          */
#define       SS_EXCHANGE     0xA6            /* Medium Exchange Command      */
#define       SS_RD_STATUS    0xB8            /* Read Element Status          */

#define SD_ERRHEAD	0X70		/* Expected error code */
#define SD_ERRHEAD1	0X71		/* Deferred expected error code */

/* Status values */
#define S_GOOD		0X00		/* Successful command completion    */
#define S_CKCON		0X02		/* Check condition		    */
#define S_METGD		0X04		/* Condition met good status	    */
#define S_BUSY		0X08		/* Busy				    */
#define S_INGD		0X10		/* Intermediate good status 	    */
#define S_INMET		0X12		/* Intermediate condition met good  */
#define S_RESER		0X18		/* Reservation conflict		    */

		/* Sense keys */
#define SD_NOSENSE	0X00		/* No Sense        */
#define SD_RECOVER	0X01		/* Recovered error */
#define SD_NREADY	0X02		/* Not Ready       */
#define SD_MEDIUM	0X03		/* Medium error    */
#define SD_HARDERR	0X04		/* Hardware error  */
#define SD_ILLREQ	0X05		/* Illegal request */
#define SD_UNATTEN	0X06		/* Unit attention  */
#define SD_PROTECT	0X07		/* Data protect    */
#define SD_BLANKCK	0X08		/* Blank check     */
#define SD_VENUNI	0X09		/* Vendor unique   */
#define SD_COPYAB	0X0A		/* Copy aborted    */
#define SD_ABORT	0X0B		/* Aborted command */
#define SD_EQUAL	0X0C		/* Equal           */
#define SD_VOLOVR	0X0D		/* Volume overflow */
#define SD_MISCOMP	0X0E		/* Miscompare      */
#define SD_RESERV	0X0F		/* Reserved        */

	/* Additional Sense codes for Direct Access Devices */
#define	SC_NOSENSE	0X00		/* No Additional Sense	      */
#define	SC_NOSGNL	0X01		/* No Index/Sector Signal     */
#define	SC_NOSEEK	0X02		/* No Seek Complete	      */
#define	SC_WRFLT	0X03		/* Write Fault		      */
#define	SC_DRVNTRDY	0X04		/* Drive Not Ready	      */
#define	SC_DRVNTSEL	0X05		/* Drive Not Selected	      */
#define	SC_NOTRKZERO	0X06		/* No Track Zero found	      */
#define	SC_MULTDRV	0X07		/* Mult Drives Selected	      */
#define	SC_LUCOMM	0X08		/* LU Communication Failure   */
#define	SC_TRACKERR	0X09		/* Track Following Error      */

#define	SC_IDERR	0X10		/* ID CRC or ECC error	      */
#define	SC_UNRECOVRRD	0X11		/* Unrecovrd Read error	      */
#define	SC_NOADDRID	0X12		/* No Addr Mark in ID	      */
#define	SC_NOADDRDATA	0X13		/* No Addr Mark in Data	      */
#define	SC_NORECORD	0X14		/* No record found	      */
#define	SC_SEEKERR	0X15		/* Seek Positioning error     */
#define	SC_DATASYNCMK	0X16		/* Data Sync Mark error       */
#define	SC_RECOVRRD	0X17		/* Recovrd Read with retry    */
#define	SC_RECOVRRDECC	0X18		/* Recovrd Read with ECC      */
#define	SC_DFCTLSTERR	0X19		/* Defect List error	      */
#define	SC_PARAMOVER	0X1A		/* Paramater Overrun	      */
#define	SC_SYNCTRAN	0X1B		/* Sync Transfer error	      */
#define	SC_NODFCTLST	0X1C		/* Prim Defect List not found */
#define	SC_CMPERR	0X1D		/* Compare error	      */
#define	SC_RECOVRIDECC	0X1E		/* Recovrd ID with ECC	      */

#define	SC_INVOPCODE	0X20		/* Invalid Command Opcode     */
#define	SC_ILLBLCK	0X21		/* Illegal Block Addr	      */
#define	SC_ILLFUNC	0X22		/* Illegal Function	      */
#define	SC_ILLCDB	0X24		/* Illegal Field in CDB	      */
#define	SC_INVLUN	0X25		/* Invalid LUN		      */
#define	SC_INVPARAM	0X26		/* Invalid Parameter List     */
#define	SC_WRPROT	0X27		/* Write Protected	      */
#define	SC_MEDCHNG	0X28		/* Medium Changed	      */
#define	SC_RESET	0X29		/* Reset Occured	      */
#define	SC_MDSELCHNG	0X2A		/* Mode Select Param changed  */

#define	SC_INCOMP	0X30		/* Incompatible Cartridge     */
#define	SC_FMTFAIL	0X31		/* Medium Format corrupted    */
#define	SC_NODFCT	0X32		/* No Defect Spares Avail     */

#define	SC_RAMFAIL	0X40		/* RAM Failure		      */
#define	SC_DATADIAG	0X41		/* Data Path Diag failure     */
#define	SC_POWFAIL	0X42		/* Power On Diag failure      */
#define	SC_MSGREJCT	0X43		/* Message Reject Error	      */
#define	SC_CONTRERR	0X44		/* Internal Controller error  */
#define	SC_SELFAIL	0X45		/* Select/Reselect failed     */
#define	SC_SOFTRESET	0X46		/* Unsuccessful Soft Reset    */
#define	SC_PARITY	0X47		/* SCSI Interface Parity error */
#define	SC_INITERR	0X48		/* Initiator Detected Error   */
#define	SC_ILLMSG	0X49		/* Illegal Message	      */

/* Peripheral device type */
#define ID_RANDOM	0X00
#define ID_TAPE		0X01
#define ID_PRINTER	0X02
#define ID_PROCESOR	0X03
#define ID_WORM		0X04		/* Write once, read often 	*/
#define ID_ROM		0X05		/* Read only disk 		*/
#define ID_SCANNER	0X06		/* Scanner device		*/
#define ID_OPTICAL	0X07		/* Optical memory device	*/
#define ID_CHANGER	0X08		/* Medium Changer device	*/
#define ID_COMMUNICATION 0X09		/* Communications device	*/
#define ID_NODEV	0x1f		/* Logical unit not present	*/

/* Peripheral qualifier */
#define ID_QOK		0X00		/* Peripheral currently connected */
#define ID_QNOLU	0X03		/* Target not capable of supporting */
					/*	a physical device	*/

#define ID_CCS		0X01		/* Expected data format number  */

/* Because of the use of bit fields and data padding the addresses and
 * sizes that are passed to the host adapter driver must be
 * adjusted. The following macros provide the size and address of
 * the structure for use by in SCB structures. For address the macro
 * should be passed the address of the structure.
 */

#define SCS_SZ 		6
#define SCS_AD(x)	(char *) x
#define SCM_SZ 		10
#define SCM_AD(x) 	((char *) x + 2)
#define       SCV_SZ          10
#define       SCV_AD(x)       ((char *) x + 2)
#define       SCE_SZ          12
#define       SCE_AD(x)       ((char *) x + 2)
#define SENSE_SZ 	18
#define SENSE_AD(x) 	((char *) x + 1)
#define IDENT_SZ 	36
#define IDENT_AD(x) 	(char *) x

/*  This structure defines group 6 commands. */

struct scs{
	unsigned char ss_op;		/* Opcode              */
	int ss_addr1 : 5;		/* Block address       */
	int ss_lun  : 3;		/* Logical unit number */
	int ss_addr : 16;		/* Block address       */
	unsigned char ss_len;		/* Transfer length     */
	unsigned char ss_cont;		/* Control field       */
};

/*  This structure defines group 10 commands.  There is a 16 bit pad
 *  at the beginning of the structure so that the 32 bit address
 *  field is properly aligned. Note the host adapter driver must be
 *  passed the address of the structure plus 2.
 */

struct scm{
	int sm_pad0 : 16;
	int sm_op   : 8;		/* Opcode              */
	int sm_res1 : 5;		/* Reserved field      */
	int sm_lun  : 3;		/* Logical unit number */
	unsigned sm_addr;		/* Block address       */
	int sm_res2 : 8;		/* Reserved field      */
	int sm_len  : 16;		/* Transfer length     */
	int sm_cont : 8;		/* Control byte        */
};

/*  This structure defines group 6 commands.  There is a 16 bit pad
 *  at the beginning of the structure so that the 32 bit address
 *  field is properly aligned. Note the host adapter driver must be
 *  passed the address of the structure plus 2.
 */
struct scv {
        int     sv_pad   : 16;          /* dummy                */
        int     sv_op    : 8;           /* Opcode               */
        int     sv_param : 3;           /* parameter bit        */
        int     sv_res1  : 2;           /* Reserved field       */
        int     sv_lun   : 3;           /* Logical unit number  */
        unsigned        sv_addr;        /* Block address        */
        int     sv_res2  : 24;          /* Reserved field       */
        int     sv_cont  : 8;           /* Control byte         */
};

/*  This structure defines group 5 commands.  There is a 16 bit pad
 *  at the beginning of the structure so that the 32 bit address
 *  field is properly aligned. Note the host adapter driver must be
 *  passed the address of the structure plus 2.
 */

 struct  sce {
      int             se_pad0 : 16;
      int             se_op : 8;
      int             se_resv1 : 5;
      int             se_lun : 3;
      int             se_lba;
      int             se_len;
      unsigned char   se_resv2;
      unsigned char   se_control;
};

/*  This structure defines READ/WRITE BUFFER commands.
 */

struct rwb_scm {
	unsigned char	rwb_op;
	unsigned char	rwb_mode:3;
	unsigned char	rwb_resv:2;
	unsigned char	rwb_lun: 3;
	unsigned char	rwb_bufid;
	unsigned char	rwb_offset[3];
	unsigned char	rwb_len[3];
	unsigned char	rwb_ctl;
};

/*
 * SM_RDDB/SM_WRDB (Read/Write Buffer) rwb_mode values
 */
#define RW_BUF_HDATA	0		/* Read/Write buffer header and data*/
#define RW_BUF_DATA	2		/* Read/Write buffer data	*/
#define RW_BUF_DESC	3		/* Read/Write buffer descriptor	*/

/*
 * Data returned for READ_BUF_DESC
 */
struct read_buf_desc {
	unsigned char rbd_res1;		/* 1st char reserved		*/
	unsigned char rbd_bufcap[3];	/* buffer capacity		*/
};

/*  Request sense data format.  This structure assumes extended sense
 *  in the CCS format. This structure has a eight bit pad to align
 *  the block address.
 */

struct sense{
	unsigned char sd_pad0;
	unsigned sd_errc    : 7;	/* Error code and class         */
	unsigned sd_valid   : 1;	/* Indicates data is valid      */

	unsigned char sd_res1;		/* Reserved field               */

	unsigned sd_key     : 4;	/* Sense key                    */
	unsigned sd_res2    : 1;	/* Reserved field               */
	unsigned sd_ili     : 1;	/* Incorrect length indicator   */
	unsigned sd_eom     : 1;	/* End of media                 */
	unsigned sd_fm      : 1;	/* File mark                    */

	unsigned sd_ba;			/* swap it - Block address      */

	unsigned char sd_len;		/* Additional sense data length */
	unsigned sd_res3    : 24;	/* swap it - Reserved field     */

	unsigned char sd_res4;		/* Reserved field               */
	unsigned char sd_sencode;	/* Additional sense code        */
	unsigned char sd_qualifier;	/* Additional sense code qualifier */
	unsigned char sd_fru;		/* Failing unit                 */
	unsigned sd_bitpt   : 3;	/* Bit pointer to error         */
	unsigned sd_bpv     : 1;	/* Bit pointer valid            */
	unsigned sd_res7    : 2;	/* Reserved field               */
	unsigned sd_cd      : 1;	/* Command/data                 */
	unsigned sd_res6    : 1;	/* Reserved field               */
	unsigned sd_field   : 16;	/* swap it Field pointer to error*/
	unsigned char sd_res8;		/* Reserved field               */
	unsigned sd_buffer  : 8;	/* swap it Field pointer to error*/
	unsigned sd_res9    : 16;	/* swap it Field pointer to error*/
};

#define	VID_LEN		8		/* inquiry vendor id length	*/
#define PID_LEN		16		/* inquiry product id length	*/
#define REV_LEN		4		/* inquiry revision num length	*/

/* The Inquiry data sturcture  */
struct ident{
	unsigned id_type    : 5;	/* Peripheral device type       */
	unsigned id_pqual   : 3;	/* Peripheral qualifier		*/
	unsigned id_qualif  : 7;	/* Device type qualifier        */
	unsigned id_rmb     : 1;	/* Removable media              */

	unsigned char id_ver;	/* Version                      */

	unsigned id_form    : 4;	/* Response data format         */
	unsigned id_res1    : 4;	/* Reserved field               */

	unsigned char id_len;	/* Length of additional data    */
	unsigned id_vu      : 24;	/* swapit Vendor unique                */
	char     id_vendor[VID_LEN];		/* Vendor ID                   */
	char     id_prod[PID_LEN];		/* Product ID                  */
	char     id_revnum[REV_LEN];		/* Revision number              */
};

/*
 * Defines for Inquiry data: peripheral qualifier 
 */
#define	SCSI_INQ_CON	0x0	/* Lun currently connected		*/
#define	SCSI_INQ_TC	0x1	/* Target capable of supporting luns,	*/
				/*   but no luns physically connected.	*/
#define	SCSI_INQ_TNC	0x3	/* Target not capable of supporting luns*/

/* 
 * Block length limit data
 */
struct blklen {
	unsigned res1:8;	   /* Reserved			*/
	unsigned max_blen:24;	   /* maximum block length	*/
	unsigned min_blen:16;	   /* minimum block length	*/
};

#define RDBLKLEN_SZ       6 
#define RDBLKLEN_AD(x)     ((char *)(x))

/*
 * Mode data structure
 */
struct mode {
	unsigned md_len   :8;		/* Sense data length		*/
	unsigned md_media :8;		/* Medium type			*/
	unsigned md_speed :4;		/* Tape speed			*/
	unsigned md_bm    :3;		/* Buffering mode		*/
	unsigned md_wp    :1;		/* Write protected		*/
	unsigned md_bdl   :8;		/* Block descriptor length	*/
	unsigned md_dens  :8;		/* Density code			*/
	unsigned md_nblks :24;		/* Number of blocks		*/
	unsigned md_res   :8;		/* Reserved field		*/
	unsigned md_bsize :24;		/* Block size			*/
};

/*
 * Mode Sense data header
 */
struct mode_hdr {
	unsigned md_len   :8;		/* Sense data length		*/
	unsigned md_media :8;		/* Medium type			*/
	unsigned md_speed :4;		/* Tape speed			*/
	unsigned md_bm    :3;		/* Buffering mode		*/
	unsigned md_wp    :1;		/* Write protected		*/
	unsigned md_bdl   :8;		/* Block descriptor length	*/
};

/*
 * Mode Sense Block Descriptor
 */
struct mode_bdescr {
	unsigned md_dens  :8;		/* Density code			*/
	unsigned md_nblks :24;		/* Number of blocks		*/
	unsigned md_res   :8;		/* Reserved field		*/
	unsigned md_bsize :24;		/* Block size			*/
};

/*
 * Mode Sense Data Compression Characteristics Page
 */
struct mode_dcc_data {
	unsigned mp_code	:6;	/* Page Code 0xF		*/
	unsigned mp_r1		:2;
	unsigned mp_len		:8;
	unsigned mp_r2		:6;
	unsigned mp_dcc		:1;	/* Data Compression Capable bit */
	unsigned mp_dce		:1;	/* Data Compression Enable	*/
	unsigned mp_r3		:5;
	unsigned mp_red		:2;	/* Report Error on Decompression*/
	unsigned mp_dde		:1;	/* Data Decompression Enable	*/
	unsigned mp_comp_alg;		/* Compression Algorithm	*/
	unsigned mp_decomp_alg;		/* Decompression Algorithm	*/
	unsigned mp_r4;
};

/*
 * Mode Sense Data Compression Page structure definition. This handles all
 * variations in returned data (i.e. with or without an associated block
 * descriptor as per SCSI-2 specification)
 */
struct mode_dcc_page {
	struct mode_hdr		h;		/* Sense Header		*/
	union {
	    struct {
		struct mode_bdescr  	md_bd;	/* Block Descriptor	*/
		struct mode_dcc_data	md_dcc;	/* Compression Page	*/
	    } md_bd_dcc;
	    struct mode_dcc_data	md_dcc;	/* Compression Page	*/
	}	p;
};

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_SCSI_H */
