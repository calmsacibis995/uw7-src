#ifndef _IO_HBA_IDE_GDEV_H	/* wrapper symbol for kernel use */
#define _IO_HBA_IDE_GDEV_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/ide/gdev.h	1.1"

#if defined(__cplusplus)
extern "C" {
#endif


/*
 * Define for Reassign Blocks defect list size.
 */

#define RABLKSSZ        8       /* Defect list in bytes         */

/*
 * Define for Read Capacity data size.
 */

#define RDCAPSZ         8       /* Length of data area          */

/*
 * Defines for Mode sense data command.
 */

#define FPGSZ           0x1C    /* Length of page 3 data area   */
#define RPGSZ           0x18    /* Length of page 4 data area   */
#define SENSE_PLH_SZ    4       /* Length of page header        */

/*
 * Define the Read Capacity Data Header format.
 */

typedef struct capacity {
        int cd_addr;            /* Logical Block Address        */
        int cd_len;             /* Block Length                 */
} CAPACITY_T;


/*  
 * Define the Direct Access Device Format Parameter Page format.
 */

typedef struct dadf {
	int pg_pc	: 6;	/* Page Code			*/
	int pg_res1	: 2;	/* Reserved			*/
	uchar_t pg_len;		/* Page Length			*/
	int pg_trk_z	: 16;	/* Tracks per Zone		*/
	int pg_asec_z	: 16;	/* Alternate Sectors per Zone	*/
	int pg_atrk_z	: 16;	/* Alternate Tracks per Zone	*/
	int pg_atrk_v	: 16;	/* Alternate Tracks per Volume	*/
	int pg_sec_t	: 16;	/* Sectors per Track		*/
	int pg_bytes_s	: 16;	/* Bytes per Physical Sector	*/
	int pg_intl	: 16;	/* Interleave Field		*/
	int pg_trkskew	: 16;	/* Track Skew Factor		*/
	int pg_cylskew	: 16;	/* Cylinder Skew Factor		*/
	int pg_res2	: 27;	/* Reserved			*/
	int pg_ins	: 1;	/* Inhibit Save			*/
	int pg_surf	: 1;	/* Allocate Surface Sectors	*/
	int pg_rmb	: 1;	/* Removable			*/
	int pg_hsec	: 1;	/* Hard Sector Formatting	*/
	int pg_ssec	: 1;	/* Soft Sector Formatting	*/
} DADF_T;

/*  
 * Define the Rigid Disk Drive Geometry Parameter Page format.
 */

typedef struct rddg {
	int pg_pc	: 6;	/* Page Code			 */
	int pg_res1	: 2;	/* Reserved			 */
	uchar_t pg_len;		/* Page Length			 */
	int pg_cylu	: 16;	/* Number of Cylinders (Upper)	 */
	uchar_t pg_cyll;	/* Number of Cylinders (Lower)	 */
	uchar_t pg_head;	/* Number of Heads		 */
	int pg_wrpcompu	: 16;	/* Write Precompensation (Upper) */
	uchar_t pg_wrpcompl;	/* Write Precompensation (Lower) */
	int pg_redwrcur	: 24;	/* Reduced Write Current	 */
	int pg_drstep	: 16;	/* Drive Step Rate		 */
	int pg_landu	: 16;	/* Landing Zone Cylinder (Upper) */
	uchar_t pg_landl;	/* Landing Zone Cylinder (Lower) */
	int pg_res2	: 24;	/* Reserved			 */
} RDDG_T;

struct blk_desc {
        uchar_t bd_dencode;
        uchar_t bd_nblks1;
        uchar_t bd_nblks2;
        uchar_t bd_nblks3;
        uchar_t bd_res;
        uchar_t bd_blen1;
        uchar_t bd_blen2;
        uchar_t bd_blen3;
};

/*
 *  Define the Mode Sense Parameter List Header format.
 */

typedef struct sense_plh {
        uchar_t plh_len;        /* Data Length                  */
        uchar_t plh_type;       /* Medium Type                  */
        uint_t  plh_res : 7;    /* Reserved                     */
        uint_t  plh_wp : 1;     /* Write Protect                */
        uchar_t plh_bdl;        /* Block Descriptor Length      */
} SENSE_PLH_T;


struct mdata {
        SENSE_PLH_T plh;
        struct blk_desc blk_desc;
        union {
                struct pdinfo pg0;
                DADF_T  pg3;
                RDDG_T  pg4;
        } pdata;
};

struct scs_format {
        uchar_t fmt_op;                 /* Opcode              */
        uchar_t fmt_defectlist : 3;
        uchar_t fmt_cmplst : 1;
        uchar_t fmt_fmtdata : 1;
        uchar_t fmt_lun : 5;
        uchar_t reserv;
        uchar_t fmt_intlmsb;
        uchar_t fmt_intllsb;
        uchar_t fmt_cont;               /* Control field       */
};

void gdev_test(struct control_area *);
int gdev_valid_drive(struct control_area *);
void gdev_inquir(struct control_area *);
void gdev_msense(struct control_area *);
void gdev_reqsen(struct control_area *);
void gdev_mselect(struct control_area *);
void gdev_rdcap(struct control_area *);
void gdev_format(struct control_area *);
void gdev_reasgn(struct control_area *);
void gdev_verify(struct control_area *);
int gdev_sb2drq(struct control_area *, int );

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_IDE_GDEV_H */
