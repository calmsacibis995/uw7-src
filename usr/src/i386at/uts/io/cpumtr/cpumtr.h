/* copyright "%c%" */
#ident	"@(#)kern-i386at:io/cpumtr/cpumtr.h	1.2"
#ident	"$Header$"


/***********************************************
* Copyright International Computers Ltd 1996 
* Copyright (c) 1996 HAL Computer Systems
*/

#ifndef CPUMTR_H  /* wrapper symbol for kernel use */
#define CPUMTR_H  /* subject to change without notice */

/* interface definitions */

#define CPUMTR_DEVICEFILE "/dev/cpumtr" /* device file */

/* ioctl(2) commands */
#define CPUMTR_CMD            ('Q'<<8)
#define CPUMTR_CMD_SETUP      (CPUMTR_CMD|'S')
#define CPUMTR_CMD_RDSAMPLES  (CPUMTR_CMD|'R')
#define CPUMTR_CMD_RESERVE    (CPUMTR_CMD|'V')
#define CPUMTR_CMD_RELEASE    (CPUMTR_CMD|'L')
#define CPUMTR_CMD_RDSTATS    (CPUMTR_CMD|'D')
#define CPUMTR_CMD_KON        (CPUMTR_CMD|'1')
#define CPUMTR_CMD_KOFF       (CPUMTR_CMD|'2')
#define CPUMTR_CMD_PON        (CPUMTR_CMD|'3')
#define CPUMTR_CMD_POFF       (CPUMTR_CMD|'4')

typedef dl_t           cpumtr_uint64_t; /* 64 bit unsigned integer */
typedef int            cpumtr_int32_t;  /* 32 bit signed integer */
typedef unsigned int   cpumtr_uint32_t; /* 32 bit unsigned integer */
typedef unsigned short cpumtr_uint16_t; /* 16 bit unsigned integer */
typedef short          cpumtr_int16_t;  /* 16 bit signed integer */

#define CPUMTR_NO_ERROR 0

/* processes with the driver open (=minor devices) */
#define CPUMTR_PROCESS_NONE -1
#define CPUMTR_PROCESS_GLOBAL 0
#define CPUMTR_MAX_PROCESSES 16
typedef cpumtr_int16_t cpumtr_process_id_t;

#define CPUMTR_TOID_NONE 0
#define CPUMTR_ASAP ((long)1)

/* users of the driver */
typedef cpumtr_uint16_t cpumtr_user_id_t;

/* CPU/LWP states */
#define CPUMTR_STATE_UNKNOWN 0
#define CPUMTR_STATE_USER    1
#define CPUMTR_STATE_SYSTEM  2
#define CPUMTR_STATE_IDLE    3
#define CPUMTR_NUM_LWP_STATES   3
#define CPUMTR_NUM_STATES       4
typedef cpumtr_uint16_t cpumtr_state_t;

/* tracepoint codes */
#define CPUMTR_AT_SYSENTRY  0
#define CPUMTR_AT_SYSEXIT   1
#define CPUMTR_AT_TRAPENTRY 2
#define CPUMTR_AT_TRAPEXIT  3
#define CPUMTR_AT_SWTCH     4
#define CPUMTR_AT_TIMEOUT   5
#define CPUMTR_AT_RDSAMPLES 6
#define CPUMTR_AT_RDSTATS   7
#define CPUMTR_AT_SETUP     8
#define CPUMTR_AT_START     9
#define CPUMTR_AT_UPDATE   10
typedef cpumtr_uint16_t cpumtr_tracepoint_t;

/* CPU types */
#define CPUMTR_NOT_SUPPORTED 0   /* CPU not supported by the driver */
#define CPUMTR_P6            6   /* P6 CPU */
typedef cpumtr_uint16_t cpumtr_cputype_t;


/* Declarations for P6-specific portions of the CPUMTR driver. */

/* P6 meter types */
#define CPUMTR_P6METERS_ALL  0     /* all meters on P6 CPU; instance ignored */
#define CPUMTR_P6METERS_TSC  1     /* TSC; instance ignored */
#define CPUMTR_P6METERS_CNT  2     /* PerfEvtSel/PerfCnt; instance = 0 or 1 */

/* P6 meters with setups */
#define CPUMTR_P6_CNT0_SETUP     0  /* PerfEvtSel0 */
#define CPUMTR_P6_CNT1_SETUP     1  /* PerfEvtSel1 */
#define CPUMTR_P6_NUM_METERS     2

/* hardware setup for a single P6 PerfCnt */
typedef struct {
  cpumtr_uint32_t c6c_evtsel;
} cpumtr_p6cnt_setup_t;

typedef struct {
  cpumtr_p6cnt_setup_t setup;
  cpumtr_uint32_t padding;
} cpumtr_p6controlreg_t;

/* P6 PerfEvtSel event_select values */
#define CPUMTR_P6EVENT_LD_BLOCKS                     0x03 
#define CPUMTR_P6EVENT_SB_DRAINS                     0x04 
#define CPUMTR_P6EVENT_MISALIGN_MEM_REF              0x05 
#define CPUMTR_P6EVENT_SEGMENT_REG_LOADS             0x06 
#define CPUMTR_P6EVENT_FP_COMP_OPS_EXE               0x10 
#define CPUMTR_P6EVENT_FP_ASSIST                     0x11 
#define CPUMTR_P6EVENT_MUL                           0x12 
#define CPUMTR_P6EVENT_DIV                           0x13 
#define CPUMTR_P6EVENT_CYCLES_DIV_BUSY               0x14 
#define CPUMTR_P6EVENT_L2_ADS                        0x21 
#define CPUMTR_P6EVENT_L2_DBUS_BUSY                  0x22
#define CPUMTR_P6EVENT_L2_DBUS_BUSY_RD               0x23 
#define CPUMTR_P6EVENT_L2_LINES_IN                   0x24 
#define CPUMTR_P6EVENT_L2_M_LINES_INM                0x25 
#define CPUMTR_P6EVENT_L2_LINES_OUT                  0x26 
#define CPUMTR_P6EVENT_L2_M_LINES_OUTM               0x27 
#define CPUMTR_P6EVENT_L2_IFETCH                     0x28 
#define CPUMTR_P6EVENT_L2_LD                         0x29 
#define CPUMTR_P6EVENT_L2_ST                         0x2a 
#define CPUMTR_P6EVENT_L2_RQSTS                      0x2e 
#define CPUMTR_P6EVENT_DATA_MEM_REFS                 0x43 
#define CPUMTR_P6EVENT_DCU_LINES_IN                  0x45 
#define CPUMTR_P6EVENT_DCU_M_LINES_IN                0x46 
#define CPUMTR_P6EVENT_DCU_M_LINES_OUT               0x47 
#define CPUMTR_P6EVENT_DCU_MISS_OUTSTANDING          0x48 
#define CPUMTR_P6EVENT_BUS_REQ_OUTSTANDING           0x60 
#define CPUMTR_P6EVENT_BUS_BNR_DRV                   0x61 
#define CPUMTR_P6EVENT_BUS_DRDY_CLOCKS               0x62 
#define CPUMTR_P6EVENT_BUS_LOCK_CLOCKS               0x63 
#define CPUMTR_P6EVENT_BUS_DATA_RCV                  0x64 
#define CPUMTR_P6EVENT_BUS_TRAN_BRD                  0x65 
#define CPUMTR_P6EVENT_BUS_TRAN_RFO                  0x66 
#define CPUMTR_P6EVENT_BUS_TRAN_WB                   0x67 
#define CPUMTR_P6EVENT_BUS_TRAN_IFETCH               0x68 
#define CPUMTR_P6EVENT_BUS_TRAN_INVAL                0x69 
#define CPUMTR_P6EVENT_BUS_TRAN_PWR                  0x6a 
#define CPUMTR_P6EVENT_BUS_TRAN_P                    0x6b 
#define CPUMTR_P6EVENT_BUS_TRAN_IO                   0x6c 
#define CPUMTR_P6EVENT_BUS_TRAN_DEF                  0x6d 
#define CPUMTR_P6EVENT_BUS_TRAN_BURST                0x6e 
#define CPUMTR_P6EVENT_BUS_TRAN_MEM                  0x6f 
#define CPUMTR_P6EVENT_BUS_TRAN_ANY                  0x70 
#define CPUMTR_P6EVENT_CPU_CLK_UNHALTED              0x79 
#define CPUMTR_P6EVENT_BUS_HIT_DRV                   0x7a 
#define CPUMTR_P6EVENT_BUS_HITM_DRV                  0x7b 
#define CPUMTR_P6EVENT_BUS_SNOOP_STALL               0x7e 
#define CPUMTR_P6EVENT_IFU_IFETCH                    0x80 
#define CPUMTR_P6EVENT_IFU_IFETCH_MISS               0x81 
#define CPUMTR_P6EVENT_ITLB_MISS                     0x85 
#define CPUMTR_P6EVENT_IFU_MEM_STALL                 0x86 
#define CPUMTR_P6EVENT_ILD_STALL                     0x87 
#define CPUMTR_P6EVENT_RESOURCE_STALLS               0xa2 
#define CPUMTR_P6EVENT_FLOPS                         0xc1 
#define CPUMTR_P6EVENT_INST_RETIRED                  0xc0 
#define CPUMTR_P6EVENT_UOPS_RETIRED                  0xc2 
#define CPUMTR_P6EVENT_BR_INST_RETIRED               0xc4 
#define CPUMTR_P6EVENT_BR_MISS_PRED_RETIRED          0xc5 
#define CPUMTR_P6EVENT_CYCLES_INT_MASKED             0xc6 
#define CPUMTR_P6EVENT_CYCLES_INT_PENDING_AND_MASKED 0xc7 
#define CPUMTR_P6EVENT_HW_INT_RX                     0xc8 
#define CPUMTR_P6EVENT_BR_TAKEN_RETIRED              0xc9 
#define CPUMTR_P6EVENT_BR_MISS_PRED_TAKEN_RET        0xca
#define CPUMTR_P6EVENT_INST_DECODED                  0xd0 
#define CPUMTR_P6EVENT_PARTIAL_RAT_STALLS            0xd2 
#define CPUMTR_P6EVENT_BR_INST_DECODED               0xe0 
#define CPUMTR_P6EVENT_BTB_MISSES                    0xe2 
#define CPUMTR_P6EVENT_BR_BOGUS                      0xe4 
#define CPUMTR_P6EVENT_BACLEARS                      0xe6 

/* P6 PerfEvtSel unit_mask values */ 
#define CPUMTR_P6UMASK_ALL_STATE 0x0F00 /* all cache line states */
#define CPUMTR_P6UMASK_I_STATE   0x0100 /* cache line Invalid */
#define CPUMTR_P6UMASK_S_STATE   0x0200 /* cache line Shared */
#define CPUMTR_P6UMASK_E_STATE   0x0400 /* cache line Exclusive */
#define CPUMTR_P6UMASK_M_STATE   0x0800 /* cache line Modified */
#define CPUMTR_P6UMASK_BUS_SELF  0x0000 /* bus events generated by this CPU */ 
#define CPUMTR_P6UMASK_BUS_ANY   0x2000 /* bus events generated by any CPU */

/* P6 PerfEvtSel user_mode options */
#define CPUMTR_P6USERMODE  0x10000

/* P6 PerfEvtSel os_mode options */
#define CPUMTR_P6OSMODE  0x20000

/* P6 PerfEvtSel edge_detect options */
#define CPUMTR_P6EDGEDETECT   0x40000

/* P6 PerfEvtSel pin_control options */
#define CPUMTR_P6PINCOUNTROL 0x80000 

/* P6 PerfEvtSel apic_interrupt options */
#define CPUMTR_P6APICINT 0x100000

/* P6 PerfEvtSel reserved bits */
#define CPUMTR_P6CNT_RESERVED 0x200000

/* P6 PerfEvtSel0 enable options */
#define CPUMTR_P6ENABLE 0x400000

/* P6 PerfEvtSel invert_counter_mask options */
#define CPUMTR_P6INVERTCOUNTERMASK  0x800000

/* P6 PerfEvtSel counter_mask options */
#define CPUMTR_P6COUNTERMASK(x)  (((x)&0xff)<<24)

#define CPUMTR_MAX_NAME_LENGTH 40
typedef char cpumtr_name_t[CPUMTR_MAX_NAME_LENGTH]; /* metric name (nul-term) */

/* CPU meters are identified by 32-bit ids, defined as follows: */
typedef struct {
  /* which CPU */
  cpumtr_uint32_t cmi_cpu_id      :16; /* LSB */

  /* which instance of this type */
  cpumtr_uint32_t cmi_instance    :8;

  /* which type of CPU meter */
  cpumtr_uint32_t cmi_type        :8;  /* MSB */
} cpumtr_meter_id_t;


/* hardware setup for a single CPU meter */
typedef union {
  cpumtr_p6cnt_setup_t p6cnt;  /* P6 PerfCnt */
} cpumtr_meter_setup_t;

#define CPUMTR_NUM_METERS   (CPUMTR_P6_NUM_METERS)


/* structure definitions needed for IOCTLs: */

/* a single sample-setup for the CPU meters */
typedef struct {
  /* hardware setups for each meter */
  cpumtr_meter_setup_t css_meter_setup[CPUMTR_NUM_METERS];

  /* max sample intervals, in microseconds */
  cpumtr_uint32_t      css_max_interval[CPUMTR_NUM_METERS];

  /* metric names (nul-terminated) */
  cpumtr_name_t        css_name[CPUMTR_NUM_METERS];
} cpumtr_sample_setup_t;

/* setup of a single CPU */
typedef struct {
  /* which meter(s) to set up */
  cpumtr_meter_id_t      cps_meter_id;

  /* setup option flags */
  cpumtr_uint16_t        cps_setup_flags;
#define CPUMTR_SETUP_ACTIVATE 0x1
#define CPUMTR_SETUP_DEACTIVATE 0x0

  /* CPU type */
  cpumtr_cputype_t       cps_cputype;

  /* number of samples to allocate trace storage for */
  cpumtr_uint32_t        cps_sampletable_size;

  /* number of sample_setups provided (=0 or 1) */
  cpumtr_uint16_t        cps_samplesetups_size;

  /* pointer to the sample setup for this CPU */
  cpumtr_sample_setup_t *cps_sample_setup;
} cpumtr_cpu_setup_t;

/* argument of CPUMTR_CMD_SETUP ioctl */
typedef struct {
  /* size of the set-up array */
  cpumtr_uint16_t        cs_num_cpu_setups;

  /* array of setups */
  cpumtr_cpu_setup_t    *cs_cpu_setups;
} cpumtr_setup_t;

/* a single trace sample from a single CPU */
typedef struct {
  /* setup that was in force for this sample */ 
  cpumtr_int16_t        ca_setup_valid_flag;
  cpumtr_sample_setup_t ca_setup;

  /* 64-bit counter values */
  cpumtr_uint64_t       ca_count[CPUMTR_NUM_METERS];

  /* TSC values for start and end of sample */
  cpumtr_uint64_t       ca_start_time;
  cpumtr_uint64_t       ca_end_time;

  /* lbolt values for start and end of sample */
  clock_t               ca_start_lbolt;
  clock_t               ca_end_lbolt;

  /* pid and lwpid of executing LWP (-1=unknown) */
  pid_t                 ca_pid;
  lwpid_t               ca_lwpid;

  /* processor state */
  cpumtr_state_t        ca_state;

  /* indication of why the sample was taken */
  cpumtr_tracepoint_t   ca_tracepoint;

  /* qualifier for tracepoint (trap type or syscall number) */
  cpumtr_uint16_t       ca_qual;
} cpumtr_sample_t;

/* a trace containing multiple samples from a single CPU */
typedef struct {
  /* which CPU */
  processorid_t     cpd_cpu_id;

  /* array of samples */
  cpumtr_sample_t  *cpd_samples;

  /* number of samples returned */
  cpumtr_uint32_t   cpd_num_samples;

  /* number of samples lost */
  cpumtr_uint32_t   cpd_num_samples_lost;
} cpumtr_proc_rdsamples_t;

/* argument of CPUMTR_CMD_RDSAMPLES ioctl */
typedef struct {
  /* number of proc_rdsamples provided */
  cpumtr_uint32_t           cd_num_cpus;

  /* array of cpu_samples */
  cpumtr_proc_rdsamples_t  *cd_cpu_samples;

  /* reserved for rdsamples option flags (=CPUMTR_RDSAMPLES_FLAGS) */
  cpumtr_uint16_t           cd_flags;
#define CPUMTR_RDSAMPLES_FLAGS 0x3

  /* max_samples for each CPU (size of cpd_samples) */
  cpumtr_uint32_t           cd_max_samples;

  /* number of samples before waking up the trace process */
  cpumtr_uint32_t           cd_block_until;
} cpumtr_rdsamples_t;

/* argument of CPUMTR_CMD_RESERVE/RELEASE ioctls */
typedef struct {
  cpumtr_meter_id_t  cv_meter_id;
  cpumtr_user_id_t   cv_user_id;
} cpumtr_reserve_t;

/* the RDSTATS information for a single CPU */
typedef struct {
  /* which CPU */
  processorid_t         ccs_cpu_id;

  /* CPU type */
  cpumtr_cputype_t      ccs_cputype;

  /* CPU speed in Hz (0=unknown) */
  cpumtr_uint32_t       ccs_hz;

  /* 64-bit virtual counter values */
  cpumtr_uint64_t       ccs_count[CPUMTR_NUM_STATES][CPUMTR_NUM_METERS];

  /* virtual TSC values */
  cpumtr_uint64_t       ccs_time[CPUMTR_NUM_STATES];

  /* setup currently in force */ 
  cpumtr_sample_setup_t ccs_setup;

  /* setup option flags from CPUMTR_CMD_SETUP or 0x0 if no setup */
  cpumtr_uint16_t       ccs_setup_flags[CPUMTR_NUM_METERS];

  /* user who activated each metric */
  cpumtr_uint32_t       ccs_userid[CPUMTR_NUM_METERS];
} cpumtr_cpu_stats_t;

/* the RDSTATS information for a single LWP */
typedef struct{
  /* 64-bit virtual counter values */
  cpumtr_uint64_t  cls_count[CPUMTR_NUM_LWP_STATES][CPUMTR_NUM_METERS];

  /* virtual TSC values */
  cpumtr_uint64_t  cls_time[CPUMTR_NUM_LWP_STATES];

  /* pid and lwpid of LWP */
  pid_t            cls_pid;
  lwpid_t          cls_lwpid;

  /* CPU this LWP last ran on */
  processorid_t    cls_cpu;
} cpumtr_lwp_stats_t;

/* argument of CPUMTR_CMD_RDSTATS ioctl */
typedef struct {
  /* reserved for rdstats option flags (=CPUMTR_RDSTATS_FLAGS) */
  cpumtr_uint16_t      cr_flags;
#define CPUMTR_RDSTATS_FLAGS 0x0

  /* number of cpu_stats provided */
  cpumtr_uint16_t      cr_num_cpus;

  /* array of CPU information */
  cpumtr_cpu_stats_t  *cr_cpu_stats;

  /* number of lwp_stats provided */
  cpumtr_uint16_t      cr_num_lwps;

  /* array of LWP information */
  cpumtr_lwp_stats_t  *cr_lwp_stats;

  /* number of lwp_stats lost */
  cpumtr_uint16_t      cr_num_lwps_lost;
} cpumtr_rdstats_t;

#ifdef _KERNEL
/* The following structures are internal to the driver. */

typedef struct cpumtr_lwp {
  /* 64-bit virtual counter values */
  cpumtr_uint64_t    cl_count[CPUMTR_NUM_LWP_STATES][CPUMTR_NUM_METERS];

  /* virtual TSC values */
  cpumtr_uint64_t    cl_time[CPUMTR_NUM_LWP_STATES];

  /* pid and lwpid of LWP */
  pid_t              cl_pid;
  lwpid_t            cl_lwpid;

  /* CPU this LWP last ran on (or -1 if unknown) */
  processorid_t      cl_cpu;

  /* next LWP in this hash bucket */
  struct cpumtr_lwp *cl_next;
} cpumtr_lwp_t;

typedef struct {
  /* lock and synch variable used for SV_SIGNAL calls */
  lock_t             *cp_lock;
  sv_t               *cp_svp;

  /* watchdog timeout function (CPUMTR_TOID_NONE=none) */
  toid_t              cp_watchdog;

  /* count for setups etc */
  cpumtr_int32_t      cp_count;

  /* array of CPU setups (during setup ioctl) */
  cpumtr_uint32_t     cp_num_cpu_setups;
  cpumtr_cpu_setup_t *cp_cpu_setups;
} cpumtr_process_t;

typedef struct {
  /* reserving process or CPUMTR_PROCESS_NONE */
  cpumtr_process_id_t   cm_reserving_process;

  /* user who reserved the metric (if reserved) */
  cpumtr_user_id_t      cm_reserving_user;

  /* setup option flags from CMD_SETUP, or CPUMTR_SETUP_DEACTIVATE */
  cpumtr_uint16_t       cm_setup_flags;

  /* user who activated the metric (if active) */
  cpumtr_user_id_t      cm_activating_user;

  /* 64-bit counter values at start of current sample */
  cpumtr_uint64_t       cm_start_count;

  /* 64-bit virtual counter values */
  cpumtr_uint64_t       cm_count[CPUMTR_NUM_STATES];
} cpumtr_meter_t;

typedef struct {
  /* CPU id */
  processorid_t          cc_cpu_id;

  /* lock for this CPU structure */
  lock_t                *cc_lock;

  /* CPU type */
  cpumtr_cputype_t       cc_cputype;

  /* CPU clock frequency in Hz (0=unknown) */
  uint_t                 cc_hz;

  /* per-meter data */
  cpumtr_meter_t         cc_meter[CPUMTR_NUM_METERS];

  /* pid and lwpid of executing LWP (-1=unknown) */
  pid_t                  cc_pid;
  lwpid_t                cc_lwpid;

  /* processor state */
  cpumtr_state_t         cc_state;

  /* current setup */
  cpumtr_sample_setup_t *cc_setup;
  size_t                 cc_setup_bytes; /* if allocated */

  /* number of samples taken */
  uint_t                 cc_num_samples_taken;

  /* number of samples kept in trace buffer */
  uint_t                 cc_num_samples_kept;

  /* capacity of trace buffer */
  uint_t                 cc_trace_buffer_size;  /* in samples */

  /* start/end of trace buffer */
  cpumtr_sample_t       *cc_trace;
  cpumtr_sample_t       *cc_trace_end;

  /* next sample */
  cpumtr_sample_t       *cc_sample;

  /* timestamp and lbolt at start of current sample */
  cpumtr_uint64_t        cc_start_time;
  clock_t                cc_start_lbolt;

  /* maximum update interval, in ticks (0=no update) */
  clock_t                cc_max_interval;

  /* timeout functions (CPUMTR_TOID_NONE=none) */
  toid_t                 cc_setup_timer;
  toid_t                 cc_start_timer;
  toid_t                 cc_timeout_timer;
  toid_t                 cc_update_timer;

  /* argument to setup_cpu() function */
  cpumtr_process_id_t    cc_setup_process;

  /* arguments to update_cpu() function (if update_timer is set) */
  cpumtr_process_id_t    cc_update_process;
  cpumtr_tracepoint_t    cc_update_tracepoint;

  /* virtual TSC values */
  cpumtr_uint64_t        cc_time[CPUMTR_NUM_STATES];

  /* number of LWP samples lost due to memory shortage */
  uint_t                 cc_lwp_samples_lost;

  /* process waiting for trace */
  cpumtr_process_id_t    cc_trace_process;

  /* number of trace samples at which to wake process (if waiting) */
  uint_t                 cc_block_until;

  /* background error code (CPUMTR_NO_ERROR = no error) */
  int                    cc_retcode;
} cpumtr_cpu_t;

/* P6-specific routines from cpumtr_p6.h */
int cpumtr_p6_num_setups(cpumtr_meter_id_t, uint_t *);
void cpumtr_p6_deactivate(cpumtr_meter_setup_t *, uint_t);
void cpumtr_p6_getsample(cpumtr_cpu_t *, cpumtr_sample_t *);
void cpumtr_p6_readtsc(volatile dl_t *pointer);
void cpumtr_p6_setup(cpumtr_sample_setup_t *);
void cpumtr_p6_start_count(cpumtr_cpu_t *);

#endif /* _KERNEL */

#endif /* CPUMTR_H */
