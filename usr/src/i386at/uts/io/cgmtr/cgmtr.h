/* copyright "%c%" */
#ident	"@(#)kern-i386at:io/cgmtr/cgmtr.h	1.2.1.1"
#ident	"$Header$"

/***********************************************
* Copyright International Computers Ltd 1996 
* Copyright (c) 1996 HAL Computer Systems
*/

#ifndef CGMTR_H  /* wrapper symbol for kernel use */
#define CGMTR_H  /* subject to change without notice */

/* interface definitions */

#define CGMTR_DEVICEFILE "/dev/cgmtr" /* device file */

/* ioctl(2) commands */
#define CGMTR_CMD            ('Q'<<8)
#define CGMTR_CMD_SETUP      (CGMTR_CMD|'S')
#define CGMTR_CMD_RDSAMPLES  (CGMTR_CMD|'R')
#define CGMTR_CMD_RESERVE    (CGMTR_CMD|'V')
#define CGMTR_CMD_RELEASE    (CGMTR_CMD|'L')
#define CGMTR_CMD_RDSTATS    (CGMTR_CMD|'D')

typedef dl_t           cgmtr_uint64_t; /* 64 bit unsigned integer */
typedef int            cgmtr_int32_t;  /* 32 bit signed integer */
typedef unsigned int   cgmtr_uint32_t; /* 32 bit unsigned integer */
typedef unsigned short cgmtr_uint16_t; /* 16 bit unsigned integer */
typedef short          cgmtr_int16_t;  /* 16 bit signed integer */
typedef unsigned char  cgmtr_uint8_t;  /* 8 bit unsigned integer */

#define CGMTR_NO_ERROR 0

/* processes with the driver open (=minor devices) */
#define CGMTR_PROCESS_NONE -1
#define CGMTR_PROCESS_GLOBAL 0
#define CGMTR_MAX_PROCESSES 16
typedef cgmtr_int16_t cgmtr_process_id_t;

#define CGMTR_TOID_NONE 0
#define CGMTR_ASAP ((long)1)

/* users of the driver */
typedef cgmtr_uint16_t cgmtr_user_id_t;

/* tracepoint codes */
#define CGMTR_AT_TIMEOUT   5
#define CGMTR_AT_RDSAMPLES 6
#define CGMTR_AT_RDSTATS   7
#define CGMTR_AT_SETUP     8
#define CGMTR_AT_START     9
#define CGMTR_AT_UPDATE   10
typedef cgmtr_uint16_t cgmtr_tracepoint_t;

/* CG types */
#define CGMTR_NOT_SUPPORTED 0   /* CG not supported by the driver */
#define CGMTR_PIU           1   /* DG PIU */
#define CGMTR_MCU           2   /* HAL MCU */
#define CGMTR_TEST          3   /* dummy for SMP-based testing */
typedef cgmtr_uint16_t cgmtr_cgtype_t;

#define CGMTR_MAX_NAME_LENGTH 40
typedef char cgmtr_name_t[CGMTR_MAX_NAME_LENGTH]; /* metric name (nul-term) */

/* CG meters are identified by 32-bit ids, defined as follows: */
typedef struct {
  /* which CG */
  cgmtr_uint32_t cgi_cg_num      :16; /* LSB */

  /* which instance of this type */
  cgmtr_uint32_t cgi_instance    :8;

  /* which type of CG meter */
  cgmtr_uint32_t cgi_type        :8;  /* MSB */
} cgmtr_meter_id_t;



/* Declarations for PIU-specific portions of the CGMTR driver. */

/* PIU meter types */
#define CGMTR_PIUMETERS_PC     0
#define CGMTR_PIUMETERS_MHB    1

#define CGMTR_PIU_NUM_PC       2
#define CGMTR_PIU_NUM_METERS   3

/* hardware setup for a single PIU PERF_COUNTER */
typedef struct {
  cgmtr_uint64_t ppp_compare;
  cgmtr_uint64_t ppp_mask;
} cgmtr_piupc_setup_t;

/* hardware setup for PIU METER_HISTORY_BUFFER */
typedef struct {
  cgmtr_uint64_t pcg_compare;
  cgmtr_uint64_t pcg_mask;
} cgmtr_piumhb_setup_t;



/* Declarations for MCU-specific portions of the CGMTR driver. */

/* MCU meter types */
#define CGMTR_MCUMETERS_CYC     0
#define CGMTR_MCUMETERS_BUSLAT  1
#define CGMTR_MCUMETERS_REMLAT  2
#define CGMTR_MCUMETERS_REM     3
#define CGMTR_MCUMETERS_BUS     4
#define CGMTR_MCUMETERS_MSG     5
#define CGMTR_MCUMETERS_PKT     6
#define CGMTR_MCUMETERS_IDWM    7
#define CGMTR_MCUMETERS_TBWM    8
#define CGMTR_MCUMETERS_QRWM    9
#define CGMTR_MCUMETERS_IHWM   10

#define CGMTR_MCU_NUM_BUS       4
#define CGMTR_MCU_NUM_MSG       4
#define CGMTR_MCU_NUM_PKT       6
#define CGMTR_MCU_NUM_METERS   22

/* hardware setup for a single MCU BUS_CNT */
typedef struct {
  cgmtr_uint32_t cgb_qual;
} cgmtr_mcubus_setup_t;

/* hardware setup for a single MCU MSG_CNT */
typedef struct {
  cgmtr_uint32_t cgm_quala;
  cgmtr_uint32_t cgm_qualb;
} cgmtr_mcumsg_setup_t;

/* hardware setup for a single MCU PKT_CNT */
typedef struct {
  cgmtr_uint32_t cgp_qual;
} cgmtr_mcupkt_setup_t;

/* hardware setup for a single MCU RPM_WM_CNT (idwm) */
typedef struct {
  cgmtr_uint8_t cgd_wm;
  cgmtr_uint8_t cgd_select;
} cgmtr_mcuidwm_setup_t;

/* hardware setup for a single MCU RPM_WM TBUF wm (tbwm) */
typedef struct {
  cgmtr_uint8_t cgt_wm;
} cgmtr_mcutbwm_setup_t;

/* hardware setup for a single MCU QRAM_WM (qrwm) */
typedef struct {
  cgmtr_uint8_t cgq_wm;
} cgmtr_mcuqrwm_setup_t;

/* hardware setup for a single MCU IHQ_WM_CNT wm (ihwm) */
typedef struct {
  cgmtr_uint8_t cgh_wm:3;
} cgmtr_mcuihwm_setup_t;

/*
 * BUS_CNT bit-fields
 */
#define  CGMTR_MCUBUS_CNT_R     0xff000000  /* reserved bits */
#define  CGMTR_MCUBUS_CNT_COUNT 0x00ffffff  /* bus count */

/*
 * BUS_LAT_TOT bit-fields
 */
#define  CGMTR_MCUBUS_LAT_TOT_R     0xf8000000  /* reserved bits */
#define  CGMTR_MCUBUS_LAT_TOT_COUNT 0x07ffffff  /* bus latency */

/*
 * BUS_QUAL bit-fields
 */
#define  CGMTR_MCUBUS_QUAL_R       0xc0008000  /* reserved bits */

#define  CGMTR_MCUBUS_QUAL_ANYREQ  0x3ffc0000  /* any request */
#define  CGMTR_MCUBUS_QUAL_IOREAD  0x20000000  /* I/O read */
#define  CGMTR_MCUBUS_QUAL_IOWRITE 0x10000000  /* I/O write */
#define  CGMTR_MCUBUS_QUAL_CAREAD  0x08000000  /* cached read */
#define  CGMTR_MCUBUS_QUAL_BI      0x04000000  /* invalidate */
#define  CGMTR_MCUBUS_QUAL_BIL     0x02000000  /* read and invalidate */
#define  CGMTR_MCUBUS_QUAL_UNREAD  0x01000000  /* uncached read */
#define  CGMTR_MCUBUS_QUAL_UNWRITE 0x00800000  /* uncached write */
#define  CGMTR_MCUBUS_QUAL_LKREAD  0x00400000  /* locked uncached read */
#define  CGMTR_MCUBUS_QUAL_LKWRITE 0x00200000  /* locked uncached write */
#define  CGMTR_MCUBUS_QUAL_WB      0x00100000  /* writeback */
#define  CGMTR_MCUBUS_QUAL_DEFREP  0x00080000  /* deferred reply */
#define  CGMTR_MCUBUS_QUAL_OTHREQ  0x00040000  /* other request */

#define  CGMTR_MCUBUS_QUAL_ANYRES  0x00037800  /* any response */
#define  CGMTR_MCUBUS_QUAL_RETRY   0x00020000  /* retry */
#define  CGMTR_MCUBUS_QUAL_DEFRES  0x00010000  /* deferred response */
#define  CGMTR_MCUBUS_QUAL_FAIL    0x00004000  /* hard failure */
#define  CGMTR_MCUBUS_QUAL_NODATA  0x00002000  /* no data response */
#define  CGMTR_MCUBUS_QUAL_IMPWB   0x00001000  /* implicit writeback */
#define  CGMTR_MCUBUS_QUAL_DATA    0x00000800  /* normal data */

#define  CGMTR_MCUBUS_QUAL_ANYDAT  0x000007c0  /* any amount of data */
#define  CGMTR_MCUBUS_QUAL_2LINES  0x00000400  /* two lines */
#define  CGMTR_MCUBUS_QUAL_LINEWD  0x00000200  /* line plus word */
#define  CGMTR_MCUBUS_QUAL_LINE    0x00000100  /* one line */
#define  CGMTR_MCUBUS_QUAL_WORD    0x00000080  /* one word */
#define  CGMTR_MCUBUS_QUAL_0WORDS  0x00000040  /* no data */

#define  CGMTR_MCUBUS_QUAL_ANYHOM  0x00000030  /* any home */
#define  CGMTR_MCUBUS_QUAL_LOCAL   0x00000020  /* home is local */
#define  CGMTR_MCUBUS_QUAL_REMOTE  0x00000010  /* home is remote */

#define  CGMTR_MCUBUS_QUAL_ANYINI  0x0000000c  /* any initiator */
#define  CGMTR_MCUBUS_QUAL_NOTMCU  0x00000008  /* initiator is not MCU */
#define  CGMTR_MCUBUS_QUAL_MCU     0x00000004  /* initiator is MCU */

#define  CGMTR_MCUBUS_QUAL_ANYCOL  0x00000003  /* any collision case */
#define  CGMTR_MCUBUS_QUAL_COL     0x00000002  /* ERAT collision */
#define  CGMTR_MCUBUS_QUAL_NOCOL   0x00000001  /* no ERAT collision */

/*
 * CONFIG bit-fields
 */
#define  CGMTR_MCUCONFIG_R        0xfffff060  /* reserved bits */
#define  CGMTR_MCUCONFIG_ERAT     0x00000800  /* ERAT size */
#define  CGMTR_MCUCONFIG_BINIT    0x00000400  /* BINIT observation */
#define  CGMTR_MCUCONFIG_BERR     0x00000200  /* BERR observation */
#define  CGMTR_MCUCONFIG_AERR     0x00000100  /* AERR observation */
#define  CGMTR_MCUCONFIG_IOQ      0x00000080  /* IOQ depth */
#define  CGMTR_MCUCONFIG_VERSION  0x0000001f  /* chip version number */

/*
 * CYCLE_CNT bit-fields
 */
#define  CGMTR_MCUCYCLE_CNT_R      0x00000000  /* reserved bits */
#define  CGMTR_MCUCYCLE_CNT_COUNT  ((unsigned)0xffffffff)  /* count */

/*
 * IDB_WM_CNT bit-fields
 */
#define  CGMTR_MCUIDB_WM_CNT_R     0x7f000000  /* reserved bits */
#define  CGMTR_MCUIDB_WM_CNT_MODE  0x80000000  /* mode select */
#define  CGMTR_MCUIDB_WM_CNT_COUNT 0x00ffffff  /* cycle count */

/*
 * IHQ_WM_CNT bit-fields
 */
#define  CGMTR_MCUIHQ_WM_CNT_R     0x8f000000  /* reserved bits */
#define  CGMTR_MCUIHQ_WM_CNT_MARK  0x70000000  /* watermark */
#define  CGMTR_MCUIHQ_WM_CNT_COUNT 0x00ffffff  /* count of enabled cycles */

/*
 * MSG_CNT bit-fields
 */
#define  CGMTR_MCUMSG_CNT_R      0xff000000  /* reserved bits */
#define  CGMTR_MCUMSG_CNT_COUNT  0x00ffffff  /* count of qualified msgs */

/*
 * MSG_QUAL part A bit-fields
 */
#define  CGMTR_MCUMSG_QUALA_R       0xcee0008c  /* reserved bits */

#define  CGMTR_MCUMSG_QUALA_ANYHOM  0x30000000  /* any home */
#define  CGMTR_MCUMSG_QUALA_REMOTE  0x20000000  /* remote home */
#define  CGMTR_MCUMSG_QUALA_LOCAL   0x10000000  /* local home */

#define  CGMTR_MCUMSG_QUALA_ANYSRC  0x01000000  /* any source */
#define  CGMTR_MCUMSG_QUALA_SRC     0x001f0000  /* source node id */

#define  CGMTR_MCUMSG_QUALA_ANYMSG  0x0000ff73  /* any message type */
#define  CGMTR_MCUMSG_QUALA_PSRT    0x00008000
#define  CGMTR_MCUMSG_QUALA_PSFL    0x00004000
#define  CGMTR_MCUMSG_QUALA_PSOK    0x00002000
#define  CGMTR_MCUMSG_QUALA_PSRE    0x00001000
#define  CGMTR_MCUMSG_QUALA_PLRT    0x00000800
#define  CGMTR_MCUMSG_QUALA_PLFL    0x00000400
#define  CGMTR_MCUMSG_QUALA_PLDT    0x00000200
#define  CGMTR_MCUMSG_QUALA_PLRE    0x00000100
#define  CGMTR_MCUMSG_QUALA_IRNK    0x00000040
#define  CGMTR_MCUMSG_QUALA_IRAK    0x00000020
#define  CGMTR_MCUMSG_QUALA_IRRE    0x00000010
#define  CGMTR_MCUMSG_QUALA_BINI    0x00000002
#define  CGMTR_MCUMSG_QUALA_BERR    0x00000001

/*
 * MSG_QUAL part B bit-fields
 */
#define  CGMTR_MCUMSG_QUALB_R       0xe0010001  /* reserved bits */

#define  CGMTR_MCUMSG_QUALB_ANYMSG  0x1ffefffe  /* any message type */
#define  CGMTR_MCUMSG_QUALB_CPBK    0x10000000
#define  CGMTR_MCUMSG_QUALB_WRBK    0x08000000
#define  CGMTR_MCUMSG_QUALB_FBDA    0x04000000
#define  CGMTR_MCUMSG_QUALB_CLTR    0x02000000
#define  CGMTR_MCUMSG_QUALB_CBDA    0x01000000
#define  CGMTR_MCUMSG_QUALB_UNDA    0x00800000
#define  CGMTR_MCUMSG_QUALB_FFDA    0x00400000
#define  CGMTR_MCUMSG_QUALB_CFDA    0x00200000
#define  CGMTR_MCUMSG_QUALB_DAXD    0x00100000
#define  CGMTR_MCUMSG_QUALB_DATX    0x00080000
#define  CGMTR_MCUMSG_QUALB_DATA    0x00040000
#define  CGMTR_MCUMSG_QUALB_UNST    0x00020000

#define  CGMTR_MCUMSG_QUALB_IACK    0x00008000
#define  CGMTR_MCUMSG_QUALB_INVL    0x00004000
#define  CGMTR_MCUMSG_QUALB_FERE    0x00002000
#define  CGMTR_MCUMSG_QUALB_FNAK    0x00001000
#define  CGMTR_MCUMSG_QUALB_DITR    0x00000800
#define  CGMTR_MCUMSG_QUALB_DONE    0x00000400
#define  CGMTR_MCUMSG_QUALB_RETR    0x00000200
#define  CGMTR_MCUMSG_QUALB_USTD    0x00000100
#define  CGMTR_MCUMSG_QUALB_USTR    0x00000080
#define  CGMTR_MCUMSG_QUALB_FBRE    0x00000040
#define  CGMTR_MCUMSG_QUALB_FFRE    0x00000020
#define  CGMTR_MCUMSG_QUALB_CBRE    0x00000010
#define  CGMTR_MCUMSG_QUALB_CFRE    0x00000008
#define  CGMTR_MCUMSG_QUALB_UNLD    0x00000004
#define  CGMTR_MCUMSG_QUALB_FXRE    0x00000002

/*
 * PKT_CNT bit-fields
 */
#define  CGMTR_MCUPKT_CNT_R      0xff000000  /* reserved bits */
#define  CGMTR_MCUPKT_CNT_COUNT  0x00ffffff  /* packet count */

/*
 * PKT_QUAL bit-fields
 */
#define  CGMTR_MCUPKT_QUAL_R     0x1fe0e288  /* reserved bits */
#define  CGMTR_MCUPKT_QUAL_MODE  0xe0000000  /* mode */
#define  CGMTR_MCUPKT_QUAL_NODE  0x001f0000  /* node ID */
#define  CGMTR_MCUPKT_QUAL_NE    0x00001000  /* enable node ID matching */

#define  CGMTR_MCUPKT_QUAL_ANYCLA  0x00000d00  /* any packet class */
#define  CGMTR_MCUPKT_QUAL_UNSEQ   0x00000800  /* unsequenced */
#define  CGMTR_MCUPKT_QUAL_SEQ     0x00000400  /* sequenced packets */
#define  CGMTR_MCUPKT_QUAL_ACK     0x00000100  /* ack packets */

#define  CGMTR_MCUPKT_QUAL_ANYSIZ  0x00000070  /* any packet size */
#define  CGMTR_MCUPKT_QUAL_HO      0x00000040  /* unsequenced */
#define  CGMTR_MCUPKT_QUAL_MDATA   0x00000020  /* medium-sized data packets */
#define  CGMTR_MCUPKT_QUAL_DATA    0x00000010  /* full-sized data packets */

#define  CGMTR_MCUPKT_QUAL_ANYIPS  0x00000007  /* any incoming packet status */
#define  CGMTR_MCUPKT_QUAL_DROPER  0x00000004  /* dropped with error */
#define  CGMTR_MCUPKT_QUAL_DROP    0x00000002  /* dropped without error */
#define  CGMTR_MCUPKT_QUAL_ACCEPT  0x00000001  /* accepted */

#define  CGMTR_MCUPKT_QUAL_ANYOPS  0x00000006  /* any outgoing packet status */
#define  CGMTR_MCUPKT_QUAL_FIRST   0x00000004  /* first attempt */
#define  CGMTR_MCUPKT_QUAL_RETR    0x00000002  /* retransmission */

/*
 * QRAM_WM bit-fields
 */
#define  CGMTR_MCUQRAM_WM_R     0xffffff00  /* reserved bits */
#define  CGMTR_MCUQRAM_WM_MARK  0x000000ff  /* watermark level */

/*
 * QRAM_WM_CNT bit-fields
 */
#define  CGMTR_MCUQRAM_WM_CNT_R      0xff000000  /* reserved bits */
#define  CGMTR_MCUQRAM_WM_CNT_COUNT  0x00ffffff  /* count of enabled cycles */

/*
 * REM_CNT bit-fields
 */
#define  CGMTR_MCUREM_CNT_R      0xff000000  /* reserved bits */
#define  CGMTR_MCUREM_CNT_COUNT  0x00ffffff  /* count of remote requests */

/*
 * REM_LAT_TOT bit-fields
 */
#define  CGMTR_MCUREM_LAT_TOT_R      0xf8000000  /* reserved bits */
#define  CGMTR_MCUREM_LAT_TOT_COUNT  0x07ffffff  /* total latency */

/*
 * RPM_WM bit-fields
 */
#define  CGMTR_MCURPM_WM_R     0xffff0000  /* reserved bits */
#define  CGMTR_MCURPM_WM_IBUF  0x0000ff00  /* IBUF watermark level */
#define  CGMTR_MCURPM_WM_TBUF  0x000000ff  /* TBUF watermark level */

/*
 * TB_WM_CNT bit-fields
 */
#define  CGMTR_MCUTB_WM_CNT_R      0xff000000  /* reserved bits */
#define  CGMTR_MCUTB_WM_CNT_COUNT  0x00ffffff  /* count of enabled cycles */



/* Declarations for TEST-specific portions of the CGMTR driver. */

/* TEST meter types */
#define CGMTR_TESTMETERS_PC     0
#define CGMTR_TESTMETERS_MHB    1

#define CGMTR_TEST_NUM_PC       2
#define CGMTR_TEST_NUM_METERS   3

/* hardware setup for a single TEST PERF_COUNTER */
typedef struct {
  cgmtr_uint64_t ppp_compare;
  cgmtr_uint64_t ppp_mask;
} cgmtr_testpc_setup_t;

/* hardware setup for TEST METER_HISTORY_BUFFER */
typedef struct {
  cgmtr_uint64_t pcg_compare;
  cgmtr_uint64_t pcg_mask;
} cgmtr_testmhb_setup_t;



/* hardware setup for a single CG meter */
typedef union {
  cgmtr_mcubus_setup_t mcubus;  /* MCU BUS_QUAL setup */
  cgmtr_mcumsg_setup_t mcumsg;  /* MCU MSG_QUAL setup */
  cgmtr_mcupkt_setup_t mcupkt;  /* MCU PKT_QUAL setup */
  cgmtr_mcuidwm_setup_t mcuidwm; /* MCU RPM_WM IBUF wm setup */
  cgmtr_mcutbwm_setup_t mcutbwm; /* MCU RPM_WM TBUF wm setup */
  cgmtr_mcuqrwm_setup_t mcuqrwm; /* MCU QRAM_WM setup */
  cgmtr_mcuihwm_setup_t mcuihwm; /* MCU IHQ_WM setup */

  cgmtr_piupc_setup_t piupc;    /* PIU PERF_COUNTER setup */
  cgmtr_piumhb_setup_t piumhb;  /* PIU METER_HISTORY_BUFFER setup */

  cgmtr_testpc_setup_t testpc;    /* TEST PERF_COUNTER setup */
  cgmtr_testmhb_setup_t testmhb;  /* TEST METER_HISTORY_BUFFER setup */
} cgmtr_meter_setup_t;

#define CGMTR_MAX(a,b) (((a)<(b))?b:a)
#define CGMTR_NUM_METERS   CGMTR_MAX(CGMTR_MCU_NUM_METERS,CGMTR_PIU_NUM_METERS)

/* structure definitions needed for IOCTLs: */

/* a single sample-setup for the CG meters */
typedef struct {
  /* hardware setups for each meter */
  cgmtr_meter_setup_t pss_meter_setup[CGMTR_NUM_METERS];

  /* max sample intervals, in microseconds */
  cgmtr_uint32_t      pss_max_interval[CGMTR_NUM_METERS];

  /* metric names (nul-terminated) */
  cgmtr_name_t        pss_name[CGMTR_NUM_METERS];
} cgmtr_sample_setup_t;

/* setup of a single CG */
typedef struct {
  /* which meter(s) to set up */
  cgmtr_meter_id_t      pps_meter_id;

  /* setup option flags */
  cgmtr_uint16_t        pps_setup_flags;
#define CGMTR_SETUP_NOOP 0x2
#define CGMTR_SETUP_ACTIVATE 0x1
#define CGMTR_SETUP_DEACTIVATE 0x0

  /* CG type */
  cgmtr_cgtype_t        pps_cgtype;

  /* number of samples to allocate trace storage for */
  cgmtr_uint32_t        pps_sampletable_size;

  /* number of sample_setups provided (=0 or 1) */
  cgmtr_uint16_t        pps_samplesetups_size;

  /* pointer to the sample setup for this CG */
  cgmtr_sample_setup_t *pps_sample_setup;
} cgmtr_cg_setup_t;

/* argument of CGMTR_CMD_SETUP ioctl */
typedef struct {
  /* size of the set-up array */
  cgmtr_uint16_t      ps_num_cg_setups;

  /* array of setups */
  cgmtr_cg_setup_t   *ps_cg_setups;
} cgmtr_setup_t;

/* a single trace sample from a single CG */
typedef struct {
  /* setup that was in force for this sample */ 
  cgmtr_int16_t        pa_setup_valid_flag;
  cgmtr_sample_setup_t pa_setup;

  /* 64-bit counter values */
  cgmtr_uint64_t       pa_count[CGMTR_NUM_METERS];

  /* lbolt values for start and end of sample */
  clock_t       pa_start_lbolt;
  clock_t       pa_end_lbolt;

  /* indication of why the sample was taken */
  cgmtr_tracepoint_t   pa_tracepoint;
} cgmtr_sample_t;

/* a trace containing multiple samples from a single CG */
typedef struct {
  /* which CG */
  cgnum_t          ppd_cg_num;		/* XXX - exposing cgnum_t */

  /* array of samples */
  cgmtr_sample_t  *ppd_samples;

  /* number of samples returned */
  cgmtr_uint32_t   ppd_num_samples;

  /* number of samples lost */
  cgmtr_uint32_t   ppd_num_samples_lost;
} cgmtr_cg_rdsamples_t;

/* argument of CGMTR_CMD_RDSAMPLES ioctl */
typedef struct {
  /* number of cg_rdsamples provided */
  cgmtr_uint32_t           pd_num_cgs;

  /* array of cg_samples */
  cgmtr_cg_rdsamples_t  *pd_cg_samples;

  /* reserved for rdsamples option flags (=CGMTR_RDSAMPLES_FLAGS) */
  cgmtr_uint16_t           pd_flags;
#define CGMTR_RDSAMPLES_FLAGS 0x3

  /* max_samples for each CG (size of ppd_samples) */
  cgmtr_uint32_t           pd_max_samples;

  /* number of samples before waking up the trace process */
  cgmtr_uint32_t           pd_block_until;
} cgmtr_rdsamples_t;

/* argument of CGMTR_CMD_RESERVE/RELEASE ioctls */
typedef struct {
  cgmtr_meter_id_t  pv_meter_id;
  cgmtr_user_id_t   pv_user_id;
} cgmtr_reserve_t;

/* the RDSTATS information for a single CG */
typedef struct {
  /* which CG */
  cgnum_t              pcs_cg_num;	/* XXX - exposing cgnum_t */

  /* CG type */
  cgmtr_cgtype_t       pcs_cgtype;

  /* 64-bit virtual counter values */
  cgmtr_uint64_t       pcs_count[CGMTR_NUM_METERS];

  /* setup currently in force */ 
  cgmtr_sample_setup_t pcs_setup;

  /* setup option flags from CGMTR_CMD_SETUP or 0x0 if no setup */
  cgmtr_uint16_t       pcs_setup_flags[CGMTR_NUM_METERS];

  /* user who activated each metric */
  cgmtr_uint32_t       pcs_userid[CGMTR_NUM_METERS];
} cgmtr_cg_stats_t;

/* argument of CGMTR_CMD_RDSTATS ioctl */
typedef struct {
  /* reserved for rdstats option flags (=CGMTR_RDSTATS_FLAGS) */
  cgmtr_uint16_t      pr_flags;
#define CGMTR_RDSTATS_FLAGS 0x0

  /* number of cg_stats provided */
  cgmtr_uint16_t      pr_num_cgs;

  /* array of CG information */
  cgmtr_cg_stats_t   *pr_cg_stats;
} cgmtr_rdstats_t;

#ifdef _KERNEL
/* The following structures are internal to the driver. */

typedef struct {
  /* lock and synch variable used for SV_SIGNAL calls */
  lock_t             *pp_lock;
  sv_t               *pp_svp;

  /* watchdog timeout function (CGMTR_TOID_NONE=none) */
  toid_t              pp_watchdog;

  /* count for setups etc */
  cgmtr_int32_t      pp_count;

  /* array of CG setups (during setup ioctl) */
  cgmtr_uint32_t     pp_num_cg_setups;
  cgmtr_cg_setup_t *pp_cg_setups;
} cgmtr_process_t;

typedef struct {
  /* reserving process or CGMTR_PROCESS_NONE */
  cgmtr_process_id_t   cg_reserving_process;

  /* user who reserved the metric (if reserved) */
  cgmtr_user_id_t      cg_reserving_user;

  /* setup option flags from CMD_SETUP, or CGMTR_SETUP_DEACTIVATE */
  cgmtr_uint16_t       cg_setup_flags;

  /* user who activated the metric (if active) */
  cgmtr_user_id_t      cg_activating_user;

  /* 64-bit counter values at start of current sample */
  cgmtr_uint64_t       cg_start_count;

  /* 64-bit virtual counter */
  cgmtr_uint64_t       cg_count;
} cgmtr_meter_t;

typedef struct {
  /* CG id */
  cgnum_t  pc_cg_num;

  /* lock for this CG structure */
  lock_t  *pc_lock;

  /* CG type */
  cgmtr_cgtype_t  pc_cgtype;

  /* per-meter data */
  cgmtr_meter_t   pc_meter[CGMTR_NUM_METERS];

  /* current setup */
  cgmtr_sample_setup_t *pc_setup;
  size_t                pc_setup_bytes; /* if allocated */

  /* number of samples taken */
  uint_t                 pc_num_samples_taken;

  /* number of samples kept in trace buffer */
  uint_t                 pc_num_samples_kept;

  /* capacity of trace buffer */
  uint_t                 pc_trace_buffer_size;  /* in samples */

  /* start/end of trace buffer */
  cgmtr_sample_t       *pc_trace;
  cgmtr_sample_t       *pc_trace_end;

  /* next sample */
  cgmtr_sample_t       *pc_sample;

  /* lbolt at start of current sample */
  clock_t                pc_start_lbolt;

  /* maximum update interval, in ticks (0=no update) */
  clock_t                pc_max_interval;

  /* timeout functions (CGMTR_TOID_NONE=none) */
  toid_t                 pc_setup_timer;
  toid_t                 pc_start_timer;
  toid_t                 pc_timeout_timer;
  toid_t                 pc_update_timer;

  /* argument to setup_cg() function */
  cgmtr_process_id_t    pc_setup_process;

  /* arguments to update_cg() function (if update_timer is set) */
  cgmtr_process_id_t    pc_update_process;
  cgmtr_tracepoint_t    pc_update_tracepoint;

  /* process waiting for trace */
  cgmtr_process_id_t    pc_trace_process;

  /* number of trace samples at which to wake process (if waiting) */
  uint_t                 pc_block_until;

  /* background error code (CGMTR_NO_ERROR = no error) */
  int                    pc_retcode;
} cgmtr_cg_t;

/* PIU-specific routines from cgmtr_piu.h */
int cgmtr_piu_num_setups(cgmtr_meter_id_t, uint_t *);
void cgmtr_piu_deactivate(cgmtr_meter_setup_t *, uint_t);
void cgmtr_piu_getsample(cgmtr_cg_t *, cgmtr_sample_t *);
void cgmtr_piu_readtsc(volatile dl_t *pointer);
void cgmtr_piu_setup(cgmtr_sample_setup_t *);
void cgmtr_piu_start_count(cgmtr_cg_t *);
bool_t cgmtr_piu_present(void);
int cgmtr_piu_init(void);

/* MCU-specific routines from cgmtr_mcu.h */
int cgmtr_mcu_num_setups(cgmtr_meter_id_t, uint_t *);
void cgmtr_mcu_deactivate(cgmtr_meter_setup_t *, uint_t);
void cgmtr_mcu_getsample(cgmtr_cg_t *, cgmtr_sample_t *);
void cgmtr_mcu_readtsc(volatile dl_t *pointer);
void cgmtr_mcu_setup(cgmtr_sample_setup_t *);
void cgmtr_mcu_start_count(cgmtr_cg_t *);
bool_t cgmtr_mcu_present(void);
int cgmtr_mcu_init(void);

/* TEST-specific routines from cgmtr_test.h */
int cgmtr_test_num_setups(cgmtr_meter_id_t, uint_t *);
void cgmtr_test_deactivate(cgmtr_meter_setup_t *, uint_t);
void cgmtr_test_getsample(cgmtr_cg_t *, cgmtr_sample_t *);
void cgmtr_test_readtsc(volatile dl_t *pointer);
void cgmtr_test_setup(cgmtr_sample_setup_t *);
void cgmtr_test_start_count(cgmtr_cg_t *);
int cgmtr_test_init(void);

#endif /* _KERNEL */

#endif /* CGMTR_H */
