#ident  "@(#)kern-pdi:io/hba/qlc1020/qlc1020.h	1.5"
/*
 *  QLogic ISP1020 SCO UnixWare version X.X (Gemini)
 *  Host Bus Adapter driver include file.
 */

/*************************************************************************
**                                                                      **
**                             NOTICE                                   **
**              COPYRIGHT (C) 1997 QLOGIC CORPORATION                   **
**                        ALL RIGHTS RESERVED                           **
**                                                                      **
** This computer program is CONFIDENTIAL and contains TRADE SECRETS of  **
** QLOGIC CORPORATION.  The receipt or possession of this program does  **
** not convey any rights to reproduce or disclose its contents, or to   **
** manufacture, use, or sell anything that it may describe, in whole or **
** in part, without the specific written consent of QLOGIC CORPORATION. **
** Any reproduction of this program without the express written consent **
** of QLOGIC CORPORATION is a violation of the copyright laws and may   **
** subject you to civil liability and criminal prosecution.             **
**                                                                      **
*************************************************************************/

#ifndef _IO_HBA_QLC1020_H          /* wrapper symbol for kernel use */
#define _IO_HBA_QLC1020_H          /* subject to change without notice */

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Enable define statement to ignore Data Underrun Errors,
 * remove define statement to enable detection.
 */
#define  DATA_UNDERRUN_ERROR_DISABLE

/*
 * Driver debug definitions.
 */
/* #define QL_DEBUG_LEVEL_1 */      /* Output register accesses to COM2. */
/* #define QL_DEBUG_LEVEL_2 */      /* Output error msgs to COM2. */
/* #define QL_DEBUG_LEVEL_3 */      /* Output function trace msgs to COM2. */
/* #define QL_DEBUG_LEVEL_4 */      /* Output NVRAM trace msgs to COM2. */
/* #define QL_DEBUG_LEVEL_5 */      /* Output ring trace msgs to COM2. */
/* #define QL_DEBUG_LEVEL_6 */      /* Output WATCHDOG timer trace to COM2. */
/* #define QL_DEBUG_LEVEL_7 */      /* Output RISC load trace msgs to COM2. */

/* #define QL_DEBUG_CONSOLE */      /* Output to console instead of COM2. */

/*
 * Data bit definitions.
 */
#define BIT_0   0x1
#define BIT_1   0x2
#define BIT_2   0x4
#define BIT_3   0x8
#define BIT_4   0x10
#define BIT_5   0x20
#define BIT_6   0x40
#define BIT_7   0x80
#define BIT_8   0x100
#define BIT_9   0x200
#define BIT_10  0x400
#define BIT_11  0x800
#define BIT_12  0x1000
#define BIT_13  0x2000
#define BIT_14  0x4000
#define BIT_15  0x8000
#define BIT_16  0x10000
#define BIT_17  0x20000
#define BIT_18  0x40000
#define BIT_19  0x80000
#define BIT_20  0x100000
#define BIT_21  0x200000
#define BIT_22  0x400000
#define BIT_23  0x800000
#define BIT_24  0x1000000
#define BIT_25  0x2000000
#define BIT_26  0x4000000
#define BIT_27  0x8000000
#define BIT_28  0x10000000
#define BIT_29  0x20000000
#define BIT_30  0x40000000
#define BIT_31  0x80000000

/*
 *  Local Macro Definitions.
 */
#if defined(QL_DEBUG_LEVEL_1) || defined(QL_DEBUG_LEVEL_2) || \
    defined(QL_DEBUG_LEVEL_3) || defined(QL_DEBUG_LEVEL_4) || \
    defined(QL_DEBUG_LEVEL_5) || defined(QL_DEBUG_LEVEL_6) || \
    defined(QL_DEBUG_LEVEL_7)
    #define QL_DEBUG_ROUTINES
#endif

#ifndef TRUE
    #define TRUE  1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

/*
 * I/O register
*/
#ifdef QL_DEBUG_LEVEL_1
    #define RD_REG_BYTE(addr)         qlc1020_getbyte((uchar_t *)addr)
    #define RD_REG_WORD(addr)         qlc1020_getword((ushort_t *)addr)
    #define RD_REG_DWORD(addr)        qlc1020_getdword((ulong_t *)addr)
    #define WRT_REG_BYTE(addr, data)  qlc1020_putbyte((uchar_t *)addr, data)
    #define WRT_REG_WORD(addr, data)  qlc1020_putword((ushort_t *)addr, data)
    #define WRT_REG_DWORD(addr, data) qlc1020_putdword((ulong_t *)addr, data)
#else
    #define RD_REG_BYTE(addr)         (*((volatile uchar_t *)addr))
    #define RD_REG_WORD(addr)         (*((volatile ushort_t *)addr))
    #define RD_REG_DWORD(addr)        (*((volatile ulong_t *)addr))
    #define WRT_REG_BYTE(addr, data)  (*((volatile uchar_t *)addr) = data)
    #define WRT_REG_WORD(addr, data)  (*((volatile ushort_t *)addr) = data)
    #define WRT_REG_DWORD(addr, data) (*((volatile ulong_t *)addr) = data)
#endif

/*
 * Host adapter default definitions.
 */
#define MAX_TARGETS     16
#define MAX_LUNS        32

/*
 * Watchdog time quantum
 */
#define QLC1020_WDG_TIME_QUANTUM   5    /* In seconds */

/* Command retry count (0-65535) */
#define COMMAND_RETRY_COUNT   255

/* Maximum outstanding commands in ISP queues (1-256) */
#define MAX_OUTSTANDING_COMMANDS   256

/* ISP request and response entry counts (37-65535) */
#define REQUEST_ENTRY_CNT       256     /* Number of request entries. */
#define RESPONSE_ENTRY_CNT      64      /* Number of response entries. */

/*
 * UnixWare required definitions.
 */
#define HBA_PREFIX qlc1020
#define DRVNAME "qlc1020 (64 bit DMA) - QLogic QLA10XX HBA Driver"

/* Physical DMA memory requirements */
#define QLC1020_MEMALIGN   4
#define QLC1020_BOUNDARY   0x80000000     /* 2GB */
#define QLC1020_DMASIZE    64

/* Maximum equipage per controller */
#define MAX_EQ          (MAX_TARGETS * MAX_LUNS)

/* Number of segments 1 - 65535 */
#define SG_SEGMENTS     32             /* Cmd entry + 6 continuations */

/*
 * UnixWare SCSI Request Block structure
 */
typedef struct srb
{
    struct xsb  *sbp;                   /* Target drv definition of SB  */
    struct srb  *s_next;                /* Next block on LU queue */
    struct srb  *s_priv;                /* Private ptr for dynamic alloc */
    struct srb  *s_prev;                /* Previous block on LU queue */
    scgth_t     *scgth;                 /* Scatter/gather structure pointer */
    paddr32_t   s_addr[2];              /* Physical data pointer */
    uchar_t     flags;                  /* Status flags. */
    ushort_t    retry_count;            /* Retry count. */
    struct srb  *wdg_prev;              /* Watchdog previous job */
    struct srb  *wdg_next;              /* Watchdog next job */
    time_t      q_time;                 /* Quantum timeout */
    time_t      timeout;                /* Watchdog timeout (quantum count) */
}srb_t;

/*
 * SRB flag definitions
 */
#define SRB_TIMEOUT     BIT_0           /* Command timed out */
#define SRB_SENT        BIT_1           /* Command sent to ISP */
#define SRB_WATCHDOG    BIT_2           /* Command on watchdog list */
#define SRB_QTAG        BIT_3           /* Queue tags enabled on device */

/*
 * UnixWare Logical Unit Queue structure
 */
typedef struct scsi_lu
{
    srb_t           *q_first;           /* First block on LU queue */
    srb_t           *q_last;            /* Last block on LU queue */
    int             q_flag;             /* LU queue state flags */
    struct sense    q_sense;            /* sense data */
    ushort_t        q_outcnt;           /* Pending jobs for this LU */
    ushort_t        abort_count;        /* Count of commands to be aborted. */
    void            (*q_func)();        /* Target driver event handler */
    long            q_param;            /* Target driver event param */
    /*
     * Local data
     */
    lock_t          *q_lock;            /* Device Queue Lock */
    pl_t            q_opri;             /* Saved Priority Level */
    bcb_t           *q_bcbp;            /* Device breakup control block */
    char            *q_sc_cmd;          /* SCSI cmd for pass-through */
}scsi_lu_t;

/*
 * UnixWare Logical Unit q_flag definitions
 */
#define QLC1020_QBUSY      BIT_0
#define QLC1020_QSUSP      BIT_1
#define QLC1020_QSENSE     BIT_2       /* Sense data cache valid */
#define QLC1020_QPTHRU     BIT_3
#define QLC1020_QHBA       BIT_4

/*
 *  ISP PCI Configuration Register Set
 */
typedef volatile struct
{
    ushort_t vendor_id;                 /* 0x0 */
        #define PCI_VENDER_ID   0x1077
    ushort_t device_id;                 /* 0x2 */
        #define PCI_DEVICE_ID   0x1020
    ushort_t command;                   /* 0x4 */
    ushort_t status;                    /* 0x6 */
    uchar_t revision_id;                /* 0x8 */
    uchar_t programming_interface;      /* 0x9 */
    uchar_t sub_class;                  /* 0xa */
    uchar_t base_class;                 /* 0xb */
    uchar_t cache_line;                 /* 0xc */
    uchar_t latency_timer;              /* 0xd */
    uchar_t header_type;                /* 0xe */
    uchar_t bist;                       /* 0xf */
    ulong_t base_port;                  /* 0x10 */
    ulong_t mem_base_addr;              /* 0x14 */
    ulong_t base_addr[4];               /* 0x18-0x24 */
    ulong_t reserved_1[2];              /* 0x28-0x2c */
    ushort_t expansion_rom;             /* 0x30 */
    ulong_t reserved_2[2];              /* 0x34-0x38 */
    uchar_t interrupt_line;             /* 0x3c */
    uchar_t interrupt_pin;              /* 0x3d */
    uchar_t min_grant;                  /* 0x3e */
    uchar_t max_latency;                /* 0x3f */
}config_reg_t;

/*
 *  ISP I/O Register Set structure definitions.
 */
typedef volatile struct
{
    ushort_t id_l;                      /* ID low */
    ushort_t id_h;                      /* ID high */
    ushort_t cfg_0;                     /* Configuration 0 */
    ushort_t cfg_1;                     /* Configuration 1 */
    ushort_t ictrl;                     /* Interface control */
        #define ISP_RESET       BIT_0   /* ISP soft reset */
        #define ISP_EN_INT      BIT_1   /* ISP enable interrupts. */
        #define ISP_EN_RISC     BIT_2   /* ISP enable RISC interrupts. */
    ushort_t istatus;                   /* Interface status */
        #define RISC_INT        BIT_2   /* RISC interrupt */
        #define SBUS_INT        BIT_1   /* SBUS interrupt */
    ushort_t semaphore;                 /* Semaphore */
    ushort_t nvram;                     /* NVRAM register. */
        #define NV_DESELECT     0
        #define NV_CLOCK        BIT_0
        #define NV_SELECT       BIT_1
        #define NV_DATA_OUT     BIT_2
        #define NV_DATA_IN      BIT_3
    ushort_t unused_1[8];               /* Gap */
    ushort_t cmd_cfg;                   /* Command DMA configuration */
    ushort_t cmd_ctrl;                  /* Command DMA control */
    ushort_t cmd_sts;                   /* Command DMA status */
    ushort_t cmd_fsts;                  /* Command DMA FIFO status */
    ushort_t cmd_cnt_l;                 /* Command DMA transfer cnt low */
    ushort_t unused_2;                  /* Reserved */
    ushort_t cmd_addr_l;                /* Command DMA address low */
    ushort_t cmd_addr_h;                /* Command DMA address high */
    ushort_t unused_3[8];               /* Gap */
    ushort_t data_cfg;                  /* Data DMA configuration */
    ushort_t data_ctrl;                 /* Data DMA control */
        #define CLEAR_FIFO      BIT_2   /* Clear FIFO. */
    ushort_t data_status;               /* Data DMA status */
    ushort_t data_fsts;                 /* Data DMA FIFO status */
    ushort_t data_cnt_l;                /* Data DMA transfer cnt low */
    ushort_t data_cnt_h;                /* Data DMA transfer cnt high */
    ushort_t data_addr_l;               /* Data DMA address low */
    ushort_t data_addr_h;               /* Data DMA address high */
    ushort_t unused_4[8];               /* Gap */
    ushort_t cmd_fifo;                  /* Command FIFO */
    ushort_t data_fifo;                 /* Data FIFO */
    ushort_t unused_5[6];               /* Gap */
    ushort_t mailbox0;                  /* Mailbox 0 */
    ushort_t mailbox1;                  /* Mailbox 1 */
    ushort_t mailbox2;                  /* Mailbox 2 */
    ushort_t mailbox3;                  /* Mailbox 3 */
    ushort_t mailbox4;                  /* Mailbox 4 */
    ushort_t mailbox5;                  /* Mailbox 5 */
    ushort_t mailbox6;                  /* Mailbox 6 */
    ushort_t mailbox7;                  /* Mailbox 7 */
    ushort_t unused_6[0x16];            /* Gap */
    ushort_t risc_pc;                   /* RISC program counter */
    ushort_t unused_7[9];               /* Gap */
    ushort_t host_cmd;                  /* Host command and control */
        #define HOST_INT      BIT_7     /* host interrupt bit */
        #define BIOS_ENABLE   BIT_0
}device_reg_t;

#define MAILBOX_REGISTER_COUNT  8

/*
 *  ISP product identification definitions in mailboxes after reset.
 */
#define PROD_ID_1           0x4953
#define PROD_ID_2           0x0000
#define PROD_ID_2a          0x5020
#define PROD_ID_3           0x2020
#define PROD_ID_4           0x1

/*
 * ISP host command and control register command definitions
 */
#define HC_RESET_RISC       0x1000      /* Reset RISC */
#define HC_PAUSE_RISC       0x2000      /* Pause RISC */
#define HC_RELEASE_RISC     0x3000      /* Release RISC from reset. */
#define HC_SET_HOST_INT     0x5000      /* Set host interrupt */
#define HC_CLR_HOST_INT     0x6000      /* Clear HOST interrupt */
#define HC_CLR_RISC_INT     0x7000      /* Clear RISC interrupt */
#define HC_DISABLE_BIOS     0x9000      /* Disable BIOS. */

/*
 * ISP mailbox Self-Test status codes
 */
#define MBS_FRM_ALIVE       0           /* Firmware Alive. */
#define MBS_CHKSUM_ERR      1           /* Checksum Error. */
#define MBS_SHADOW_LD_ERR   2           /* Shadow Load Error. */
#define MBS_BUSY            4           /* Busy. */

/*
 * ISP mailbox command complete status codes
 */
#define MBS_CMD_CMP         0x4000      /* Command Complete. */
#define MBS_INV_CMD         0x4001      /* Invalid Command. */
#define MBS_HOST_INF_ERR    0x4002      /* Host Interface Error. */
#define MBS_TEST_FAILED     0x4003      /* Test Failed. */
#define MBS_CMD_ERR         0x4005      /* Command Error. */
#define MBS_CMD_PARAM_ERR   0x4006      /* Command Parameter Error. */

/*
 * ISP mailbox asynchronous event status codes
 */
#define MBA_ASYNC_EVENT         0x8000  /* Asynchronous event. */
#define MBA_BUS_RESET           0x8001  /* SCSI Bus Reset. */
#define MBA_SYSTEM_ERR          0x8002  /* System Error. */
#define MBA_REQ_TRANSFER_ERR    0x8003  /* Request Transfer Error. */
#define MBA_RSP_TRANSFER_ERR    0x8004  /* Response Transfer Error. */
#define MBA_WAKEUP_THRES        0x8005  /* Request Queue Wake-up. */
#define MBA_TIMEOUT_RESET       0x8006  /* Execution Timeout Reset. */
#define MBA_DEVICE_RESET        0x8007  /* Bus Device Reset. */
#define MBA_SCSI_COMPLETION     0x8020  /* Completion response. */

/*
 * ISP mailbox commands
 */
#define MBC_NOP_CMD             0       /* No Operation. */
#define MBC_LD_RAM_CMD          1       /* Load RAM. */
#define MBC_EXC_FIRM_CMD        2       /* Execute firmware. */
#define MBC_WRT_WORD_CMD        4       /* Write ram word. */
#define MBC_RD_WORD_CMD         5       /* Read ram word. */
#define MBC_WRAP_MBX_CMD        6       /* Wrap incoming mailboxes */
#define MBC_VFY_CHKSUM_CMD      7       /* Verify checksum. */
#define MBC_ABOUT_FIRMWARE      8       /* Get firmware revision. */
#define MBC_INIT_REQ_QUE_CMD    0x10    /* Initialize request queue. */
#define MBC_INIT_RSP_QUE_CMD    0x11    /* Initialize response queue. */
#define MBC_EXC_IOCB_CMD        0x12    /* Execute IOCB command. */
#define MBC_ABORT_IOCB_CMD      0x15    /* Abort IOCB command. */
#define MBC_ABORT_DEVICE_CMD    0x16    /* Abort device (ID/LUN). */
#define MBC_ABORT_TARGET_CMD    0x17    /* Abort target (ID). */
#define MBC_BUS_RESET_CMD       0x18    /* SCSI bus reset. */
#define MBC_GET_RETRY_COUNT     0x22    /* Get retry count and delay. */
#define MBC_GET_TARGET_PARAM    0x28    /* Get target parameters. */
#define MBC_SET_INIT_ID_CMD     0x30    /* Set initiator SCSI ID. */
#define MBC_SET_SELECT_TIMEOUT  0x31    /* Set selection timeout. */
#define MBC_SET_RETRY_COUNT     0x32    /* Set retry count and delay. */
#define MBC_SET_TAG_AGE_LIMIT   0x33    /* Set tag age limit. */
#define MBC_SET_CLOCK_RATE      0x34    /* Set clock rate. */
#define MBC_SET_ACTIVE_NEGATION 0x35    /* Set active negation state. */
#define MBC_SET_ASYNC_TIME      0x36    /* Set async data setup time. */
#define MBC_SET_PCI_CONTROL     0x37    /* Set BUS control parameters. */
#define MBC_SET_TARGET_PARAM    0x38    /* Set target parameters. */
#define MBC_SET_DEVICE_QUEUE    0x39    /* Set device queue parameters */
#define MBC_GET_BUF_ADDR_CMD    0x40    /* Get BIOS ram address. */
#define MBC_WRT_4WDS_CMD        0x41    /* Write 4 words. */
#define MBC_EXC_BIOS_IOCB       0x42    /* Execute BIOS IOCB command. */
#define MBC_SET_SYS_PARAMETER   0x45    /* Set system parameter word. */
#define MBC_SET_FRM_FEATURES    0x4A    /* Set firmware feature word. */
#define MBC_INIT_REQ_QUE_A64    0x52    /* Initialize request queue A64. */
#define MBC_INIT_RSP_QUE_A64    0x53    /* Initialize response queue A64. */
#define MBC_ENABLE_TARGET       0x55    /* Enable target mode. */

/*
 * ISP Get/Set Target Parameters mailbox command control flags.
 */
#define TP_RENEGOTIATE      BIT_8       /* Renegotiate on error. */
#define TP_STOP_QUE         BIT_9       /* Stop que on check condition */
#define TP_AUTO_REQ         BIT_10      /* Automatic request sense. */
#define TP_TAGGED_QUE       BIT_11      /* Tagged queuing. */
#define TP_SYNC             BIT_12      /* Synchronous data transfers. */
#define TP_WIDE             BIT_13      /* Wide data transfers. */
#define TP_PARITY           BIT_14      /* Parity checking. */
#define TP_DISCONNECT       BIT_15      /* Disconnect privilege. */

/*
 * NVRAM Command values.
 */
#define NV_START_BIT            BIT_2
#define NV_WRITE_OP             (BIT_24+BIT_22)
#define NV_READ_OP              (BIT_24+BIT_23)
#define NV_ERASE_OP             (BIT_24+BIT_23+BIT_22)
#define NV_MASK_OP              (BIT_24+BIT_23+BIT_22)
#define NV_DELAY_COUNT          10

/*
 *  QLogic ISP1020 NVRAM structure definition.
 */
typedef struct
{
    uchar_t id[4];                                  /* 0, 1, 2, 3 */
    uchar_t version;                                /* 4 */

    struct
    {
        uchar_t fifo_threshold      :2;
        uchar_t bios_disable        :1;
        uchar_t host_enable         :1;
        uchar_t initiator_id        :4;
    }cntr_flags_1;                                  /* 5 */

    uchar_t bus_reset_delay;                        /* 6 */
    uchar_t retry_count;                            /* 7 */
    uchar_t retry_delay;                            /* 8 */

    struct
    {
        uchar_t async_data_setup_time       :4;
        uchar_t req_ack_active_negation     :1;
        uchar_t data_line_active_negation   :1;
        uchar_t data_dma_burst              :1;
        uchar_t cmd_dma_burst               :1;
    }cntr_flags_2;                                  /* 9 */

    uchar_t  tag_aging;                             /* 10 */

    struct
    {
        uchar_t termination_enable      :2;
        uchar_t pcmc_enable             :1;
        uchar_t enable_60mhz            :1;
        uchar_t scsi_reset_disable      :1;
        uchar_t auto_termination_enable :1;
        uchar_t fifo_128_threshold      :1;
        uchar_t unused_7                :1;
    }cntr_flags_3;                                  /* 11 */

    ushort_t selection_timeout;                     /* 12, 13 */
    ushort_t max_queue_depth;                       /* 14, 15 */

    struct
    {
        uchar_t scsi_bus_size           :1;
        uchar_t scsi_bus_type           :1;
        uchar_t adapter_clock_speed     :1;
        uchar_t software_termination    :1;
        uchar_t flash_broad             :1;
        uchar_t oem_field               :3;
    }cntr_flags_4;                                  /* 16 */

    struct
    {
        uchar_t apple_connector             :3;
        uchar_t unused_3                    :1;
        uchar_t disable_loading_risc_code   :1;
        uchar_t bios_config_mode            :2;
        uchar_t unused_7                    :1;
    }apple_config;                                  /* 17 */

    ushort_t subsystem_id[2];                       /* 18, 19, 20, 21 */

    ushort_t isp_parameter;                         /* 22, 23 */

    union
    {
        ushort_t w;
        struct
        {
            uchar_t enable_fast_posting :1;
            uchar_t unused_1            :1;
            uchar_t unused_2            :1;
            uchar_t unused_3            :1;
            uchar_t unused_4            :1;
            uchar_t unused_5            :1;
            uchar_t unused_6            :1;
            uchar_t unused_7            :1;
            uchar_t unused_8            :1;
            uchar_t unused_9            :1;
            uchar_t unused_10           :1;
            uchar_t unused_11           :1;
            uchar_t unused_12           :1;
            uchar_t unused_13           :1;
            uchar_t unused_14           :1;
            uchar_t unused_15           :1;
        }f;
    }firmware_feature;                              /* 24, 25 */

    ushort_t unused_26;                             /* 26, 27 */

    struct
    {
        union
        {
            uchar_t c;
            struct
            {
                uchar_t renegotiate_on_error :1;
                uchar_t stop_queue_on_check  :1;
                uchar_t auto_request_sense   :1;
                uchar_t tag_queuing          :1;
                uchar_t sync_data_transfers  :1;
                uchar_t wide_data_transfers  :1;
                uchar_t parity_checking      :1;
                uchar_t disconnect_allowed   :1;
            }f;
        }parameter;                             /* 0 */

        uchar_t execution_throttle;             /* 1 */
        uchar_t sync_period;                    /* 2 */

        struct
        {
            uchar_t sync_offset   :4;
            uchar_t device_enable :1;
            uchar_t lun_disable   :1;
            uchar_t unused_6      :1;
            uchar_t unused_7      :1;
        }flags;                                 /* 3 */

        ushort_t unused_1;                      /* 4, 5 */
    }target[MAX_TARGETS];

    ushort_t unused_13;                         /* 124, 125 */

    uchar_t  system_id_ptr;                     /* 126 */

    uchar_t chksum;                             /* 127 */
}nvram_t;

/*
 * ISP queue - command entry structure definition.
 */
#define MAX_CMDSZ   12                  /* SCSI maximum CDB size. */
typedef struct
{
    uchar_t  entry_type;                /* Entry type. */
        #define COMMAND_TYPE    1       /* Command entry */
    uchar_t  entry_count;               /* Entry count. */
    uchar_t  sys_define;                /* System defined. */
    uchar_t  entry_status;              /* Entry Status. */
    ulong_t  handle;                    /* System handle. */
    uchar_t  lun;                       /* SCSI LUN */
    uchar_t  target;                    /* SCSI ID */
    ushort_t cdb_len;                   /* SCSI command length. */
    ushort_t control_flags;             /* Control flags. */
    ushort_t reserved;
    ushort_t timeout;                   /* Command timeout. */
    ushort_t dseg_count;                /* Data segment count. */
    uchar_t  scsi_cdb[MAX_CMDSZ];       /* SCSI command words. */
    ulong_t  dseg_0_address;            /* Data segment 0 address. */
    ulong_t  dseg_0_length;             /* Data segment 0 length. */
    ulong_t  dseg_1_address;            /* Data segment 1 address. */
    ulong_t  dseg_1_length;             /* Data segment 1 length. */
    ulong_t  dseg_2_address;            /* Data segment 2 address. */
    ulong_t  dseg_2_length;             /* Data segment 2 length. */
    ulong_t  dseg_3_address;            /* Data segment 3 address. */
    ulong_t  dseg_3_length;             /* Data segment 3 length. */
}cmd_entry_t;

/*
 * ISP queue - continuation entry structure definition.
 */
typedef struct
{
    uchar_t  entry_type;                /* Entry type. */
        #define CONTINUE_TYPE   2       /* Continuation entry. */
    uchar_t  entry_count;               /* Entry count. */
    uchar_t  sys_define;                /* System defined. */
    uchar_t  entry_status;              /* Entry Status. */
    ulong_t  reserved;                  /* Reserved */
    ulong_t  dseg_0_address;            /* Data segment 0 address. */
    ulong_t  dseg_0_length;             /* Data segment 0 length. */
    ulong_t  dseg_1_address;            /* Data segment 1 address. */
    ulong_t  dseg_1_length;             /* Data segment 1 length. */
    ulong_t  dseg_2_address;            /* Data segment 2 address. */
    ulong_t  dseg_2_length;             /* Data segment 2 length. */
    ulong_t  dseg_3_address;            /* Data segment 3 address. */
    ulong_t  dseg_3_length;             /* Data segment 3 length. */
    ulong_t  dseg_4_address;            /* Data segment 4 address. */
    ulong_t  dseg_4_length;             /* Data segment 4 length. */
    ulong_t  dseg_5_address;            /* Data segment 5 address. */
    ulong_t  dseg_5_length;             /* Data segment 5 length. */
    ulong_t  dseg_6_address;            /* Data segment 6 address. */
    ulong_t  dseg_6_length;             /* Data segment 6 length. */
}cont_entry_t;

/*
 * ISP queue - status entry structure definition.
 */
typedef struct
{
    uchar_t  entry_type;                /* Entry type. */
        #define STATUS_TYPE     3       /* Status entry. */
    uchar_t  entry_count;               /* Entry count. */
    uchar_t  sys_define;                /* System defined. */
    uchar_t  entry_status;              /* Entry Status. */
        #define RF_CONT         BIT_0   /* Continuation. */
        #define RF_FULL         BIT_1   /* Full */
        #define RF_BAD_HEADER   BIT_2   /* Bad header. */
        #define RF_BAD_PAYLOAD  BIT_3   /* Bad payload. */
    ulong_t  handle;                    /* System handle. */
    ushort_t scsi_status;               /* SCSI status. */
    ushort_t comp_status;               /* Completion status. */
    ushort_t state_flags;               /* State flags. */
    ushort_t status_flags;              /* Status flags. */
    ushort_t time;                      /* Time. */
    ushort_t req_sense_length;          /* Request sense data length. */
    ulong_t  residual_length;           /* Residual transfer length. */
    ushort_t reserved[4];
    uchar_t  req_sense_data[32];        /* Request sense data. */
}sts_entry_t, response_t;

/*
 * ISP queue - marker entry structure definition.
 */
typedef struct
{
    uchar_t  entry_type;                /* Entry type. */
        #define MARKER_TYPE     4       /* Marker entry. */
    uchar_t  entry_count;               /* Entry count. */
    uchar_t  sys_define;                /* System defined. */
    uchar_t  entry_status;              /* Entry Status. */
    ulong_t  reserved;
    uchar_t  lun;                       /* SCSI LUN */
    uchar_t  target;                    /* SCSI ID */
    uchar_t  modifier;                  /* Modifier (7-0). */
        #define MK_SYNC_ID_LUN      0   /* Synchronize ID/LUN */
        #define MK_SYNC_ID          1   /* Synchronize ID */
        #define MK_SYNC_ALL         2   /* Synchronize all ID/LUN */
    uchar_t  reserved_1[53];
}mrk_entry_t;

/*
 * ISP queue - extended command entry structure definition.
 */
typedef struct
{
    uchar_t  entry_type;                /* Entry type. */
        #define EXTENDED_CMD_TYPE  5    /* Extended command entry. */
    uchar_t  entry_count;               /* Entry count. */
    uchar_t  sys_define;                /* System defined. */
    uchar_t  entry_status;              /* Entry Status. */
    ulong_t  handle;                    /* System handle. */
    uchar_t  lun;                       /* SCSI LUN */
    uchar_t  target;                    /* SCSI ID */
    ushort_t cdb_len;                   /* SCSI command length. */
    ushort_t control_flags;             /* Control flags. */
    ushort_t reserved;
    ushort_t timeout;                   /* Command timeout. */
    ushort_t dseg_count;                /* Data segment count. */
    uchar_t  scsi_cdb[88];              /* SCSI command words. */
}ecmd_entry_t;

/*
 * ISP queue - 64-Bit addressing, command entry structure definition.
 */
typedef struct
{
    uchar_t  entry_type;                /* Entry type. */
        #define COMMAND_A64_TYPE 9      /* Command A64 entry */
    uchar_t  entry_count;               /* Entry count. */
    uchar_t  sys_define;                /* System defined. */
    uchar_t  entry_status;              /* Entry Status. */
    ulong_t  handle;                    /* System handle. */
    uchar_t  lun;                       /* SCSI LUN */
    uchar_t  target;                    /* SCSI ID */
    ushort_t cdb_len;                   /* SCSI command length. */
    ushort_t control_flags;             /* Control flags. */
    ushort_t reserved;
    ushort_t timeout;                   /* Command timeout. */
    ushort_t dseg_count;                /* Data segment count. */
    uchar_t  scsi_cdb[MAX_CMDSZ];       /* SCSI command words. */
    ulong_t  reserved_1[2];             /* unused */
    ulong_t  dseg_0_address[2];         /* Data segment 0 address. */
    ulong_t  dseg_0_length;             /* Data segment 0 length. */
    ulong_t  dseg_1_address[2];         /* Data segment 1 address. */
    ulong_t  dseg_1_length;             /* Data segment 1 length. */
}cmd_a64_entry_t, request_t;

/*
 * ISP queue - 64-Bit addressing, continuation entry structure definition.
 */
typedef struct
{
    uchar_t  entry_type;                /* Entry type. */
        #define CONTINUE_A64_TYPE 0xA   /* Continuation A64 entry. */
    uchar_t  entry_count;               /* Entry count. */
    uchar_t  sys_define;                /* System defined. */
    uchar_t  entry_status;              /* Entry Status. */
    ulong_t  dseg_0_address[2];         /* Data segment 0 address. */
    ulong_t  dseg_0_length;             /* Data segment 0 length. */
    ulong_t  dseg_1_address[2];         /* Data segment 1 address. */
    ulong_t  dseg_1_length;             /* Data segment 1 length. */
    ulong_t  dseg_2_address[2];         /* Data segment 2 address. */
    ulong_t  dseg_2_length;             /* Data segment 2 length. */
    ulong_t  dseg_3_address[2];         /* Data segment 3 address. */
    ulong_t  dseg_3_length;             /* Data segment 3 length. */
    ulong_t  dseg_4_address[2];         /* Data segment 4 address. */
    ulong_t  dseg_4_length;             /* Data segment 4 length. */
}cont_a64_entry_t;

/*
 * ISP queue - enable LUN entry structure definition.
 */
typedef struct
{
    uchar_t  entry_type;                /* Entry type. */
        #define ENABLE_LUN_TYPE 0xB     /* Enable LUN entry. */
    uchar_t  entry_count;               /* Entry count. */
    uchar_t  reserved_1;
    uchar_t  entry_status;              /* Entry Status. */
    ulong_t  reserved_2;
    uchar_t  lun;
    uchar_t  reserved_3;
    ushort_t reserved_4;
    ulong_t  option_flags;
    uchar_t  status;
    uchar_t  reserved_5;
    uchar_t  command_count;             /* Number of ATIOs allocated. */
    uchar_t  immed_notify_count;        /* Number of Immediate Notify */
                                        /* entries allocated. */
    uchar_t  group_6_length;            /* SCSI CDB length for group 6 */
                                        /* commands (2-26). */
    uchar_t  group_7_length;            /* SCSI CDB length for group 7 */
                                        /* commands (2-26). */
    ushort_t timeout;                   /* 0 = 30 seconds, 0xFFFF = disable */
    ushort_t reserved_6[20];
}elun_entry_t;

/*
 * ISP queue - modify LUN entry structure definition.
 */
typedef struct
{
    uchar_t  entry_type;                /* Entry type. */
        #define MODIFY_LUN_TYPE 0xC     /* Modify LUN entry. */
    uchar_t  entry_count;               /* Entry count. */
    uchar_t  reserved_1;
    uchar_t  entry_status;              /* Entry Status. */
    ulong_t  reserved_2;
    uchar_t  lun;                       /* SCSI LUN */
    uchar_t  reserved_3;
    uchar_t  operators;
    uchar_t  reserved_4;
    ulong_t  option_flags;
    uchar_t  status;
    uchar_t  reserved_5;
    uchar_t  command_count;             /* Number of ATIOs allocated. */
    uchar_t  immed_notify_count;        /* Number of Immediate Notify */
                                        /* entries allocated. */
    ushort_t reserved_6;
    ushort_t timeout;                   /* 0 = 30 seconds, 0xFFFF = disable */
    ushort_t reserved_7[20];
}modify_lun_entry_t;

/*
 * ISP queue - immediate notify entry structure definition.
 */
typedef struct
{
    uchar_t  entry_type;                /* Entry type. */
        #define IMMED_NOTIFY_TYPE 0xD   /* Immediate notify entry. */
    uchar_t  entry_count;               /* Entry count. */
    uchar_t  reserved_1;
    uchar_t  entry_status;              /* Entry Status. */
    ulong_t  reserved_2;
    uchar_t  lun;
    uchar_t  initiator_id;
    uchar_t  reserved_3;
    uchar_t  target_id;
    ulong_t  option_flags;
    uchar_t  status;
    uchar_t  reserved_4;
    uchar_t  tag_value;                 /* Received queue tag message value */
    uchar_t  tag_type;                  /* Received queue tag message type */
                                        /* entries allocated. */
    ushort_t seq_id;
    uchar_t  scsi_msg[8];               /* SCSI message not handled by ISP */
    ushort_t reserved_5[8];
    uchar_t  sense_data[18];
}notify_entry_t;

/*
 * ISP queue - notify acknowledge entry structure definition.
 */
typedef struct
{
    uchar_t  entry_type;                /* Entry type. */
        #define NOTIFY_ACK_TYPE 0xE     /* Notify acknowledge entry. */
    uchar_t  entry_count;               /* Entry count. */
    uchar_t  reserved_1;
    uchar_t  entry_status;              /* Entry Status. */
    ulong_t  reserved_2;
    uchar_t  lun;
    uchar_t  initiator_id;
    uchar_t  reserved_3;
    uchar_t  target_id;
    ulong_t  option_flags;
    uchar_t  status;
    uchar_t  event;
    ushort_t seq_id;
    ushort_t reserved_4[22];
}nack_entry_t;

/*
 * ISP queue - Accept Target I/O (ATIO) entry structure definition.
 */
typedef struct
{
    uchar_t  entry_type;                /* Entry type. */
        #define ACCEPT_TGT_IO_TYPE 6    /* Accept target I/O entry. */
    uchar_t  entry_count;               /* Entry count. */
    uchar_t  reserved_1;
    uchar_t  entry_status;              /* Entry Status. */
    ulong_t  reserved_2;
    uchar_t  lun;
    uchar_t  initiator_id;
    uchar_t  cdb_len;
    uchar_t  target_id;
    ulong_t  option_flags;
    uchar_t  status;
    uchar_t  scsi_status;
    uchar_t  tag_value;                 /* Received queue tag message value */
    uchar_t  tag_type;                  /* Received queue tag message type */
    uchar_t  cdb[26];
    uchar_t  sense_data[18];
}atio_entry_t;

/*
 * ISP queue - Continue Target I/O (CTIO) entry structure definition.
 */
typedef struct
{
    uchar_t  entry_type;                /* Entry type. */
        #define CONTINUE_TGT_IO_TYPE 7  /* CTIO entry */
    uchar_t  entry_count;               /* Entry count. */
    uchar_t  reserved_1;
    uchar_t  entry_status;              /* Entry Status. */
    ulong_t  reserved_2;
    uchar_t  lun;                       /* SCSI LUN */
    uchar_t  initiator_id;
    uchar_t  reserved_3;
    uchar_t  target_id;
    ulong_t  option_flags;
    uchar_t  status;
    uchar_t  scsi_status;
    uchar_t  tag_value;                 /* Received queue tag message value */
    uchar_t  tag_type;                  /* Received queue tag message type */
    ulong_t  transfer_length;
    ulong_t  residual;
    ushort_t timeout;                   /* 0 = 30 seconds, 0xFFFF = disable */
    ushort_t dseg_count;                /* Data segment count. */
    ulong_t  dseg_0_address;            /* Data segment 0 address. */
    ulong_t  dseg_0_length;             /* Data segment 0 length. */
    ulong_t  dseg_1_address;            /* Data segment 1 address. */
    ulong_t  dseg_1_length;             /* Data segment 1 length. */
    ulong_t  dseg_2_address;            /* Data segment 2 address. */
    ulong_t  dseg_2_length;             /* Data segment 2 length. */
    ulong_t  dseg_3_address;            /* Data segment 3 address. */
    ulong_t  dseg_3_length;             /* Data segment 3 length. */
}ctio_entry_t;

/*
 * ISP queue - CTIO returned entry structure definition.
 */
typedef struct
{
    uchar_t  entry_type;                /* Entry type. */
        #define CTIO_RET_TYPE   7       /* CTIO return entry */
    uchar_t  entry_count;               /* Entry count. */
    uchar_t  reserved_1;
    uchar_t  entry_status;              /* Entry Status. */
    ulong_t  reserved_2;
    uchar_t  lun;                       /* SCSI LUN */
    uchar_t  initiator_id;
    uchar_t  reserved_3;
    uchar_t  target_id;
    ulong_t  option_flags;
    uchar_t  status;
    uchar_t  scsi_status;
    uchar_t  tag_value;                 /* Received queue tag message value */
    uchar_t  tag_type;                  /* Received queue tag message type */
    ulong_t  transfer_length;
    ulong_t  residual;
    ushort_t timeout;                   /* 0 = 30 seconds, 0xFFFF = disable */
    ushort_t dseg_count;                /* Data segment count. */
    ulong_t  dseg_0_address;            /* Data segment 0 address. */
    ulong_t  dseg_0_length;             /* Data segment 0 length. */
    ulong_t  dseg_1_address;            /* Data segment 1 address. */
    ushort_t dseg_1_length;             /* Data segment 1 length. */
    uchar_t  sense_data[18];
}ctio_ret_entry_t;

/*
 * ISP queue - CTIO A64 entry structure definition.
 */
typedef struct
{
    uchar_t  entry_type;                /* Entry type. */
        #define CTIO_A64_TYPE 0xF       /* CTIO A64 entry */
    uchar_t  entry_count;               /* Entry count. */
    uchar_t  reserved_1;
    uchar_t  entry_status;              /* Entry Status. */
    ulong_t  reserved_2;
    uchar_t  lun;                       /* SCSI LUN */
    uchar_t  initiator_id;
    uchar_t  reserved_3;
    uchar_t  target_id;
    ulong_t  option_flags;
    uchar_t  status;
    uchar_t  scsi_status;
    uchar_t  tag_value;                 /* Received queue tag message value */
    uchar_t  tag_type;                  /* Received queue tag message type */
    ulong_t  transfer_length;
    ulong_t  residual;
    ushort_t timeout;                   /* 0 = 30 seconds, 0xFFFF = disable */
    ushort_t dseg_count;                /* Data segment count. */
    ulong_t  reserved_4[2];
    ulong_t  dseg_0_address[2];         /* Data segment 0 address. */
    ulong_t  dseg_0_length;             /* Data segment 0 length. */
    ulong_t  dseg_1_address[2];         /* Data segment 1 address. */
    ulong_t  dseg_1_length;             /* Data segment 1 length. */
}ctio_a64_entry_t;

/*
 * ISP queue - CTIO returned entry structure definition.
 */
typedef struct
{
    uchar_t  entry_type;                /* Entry type. */
        #define CTIO_A64_RET_TYPE 0xF   /* CTIO A64 returned entry */
    uchar_t  entry_count;               /* Entry count. */
    uchar_t  reserved_1;
    uchar_t  entry_status;              /* Entry Status. */
    ulong_t  reserved_2;
    uchar_t  lun;                       /* SCSI LUN */
    uchar_t  initiator_id;
    uchar_t  reserved_3;
    uchar_t  target_id;
    ulong_t  option_flags;
    uchar_t  status;
    uchar_t  scsi_status;
    uchar_t  tag_value;                 /* Received queue tag message value */
    uchar_t  tag_type;                  /* Received queue tag message type */
    ulong_t  transfer_length;
    ulong_t  residual;
    ushort_t timeout;                   /* 0 = 30 seconds, 0xFFFF = disable */
    ushort_t dseg_count;                /* Data segment count. */
    ushort_t reserved_4[7];
    uchar_t  sense_data[18];
}ctio_a64_ret_entry_t;

/*
 * ISP request and response queue entry sizes
 */
#define RESPONSE_ENTRY_SIZE     (sizeof(response_t))
#define REQUEST_ENTRY_SIZE      (sizeof(request_t))

/*
 * ISP status entry - completion status definitions.
 */
#define CS_COMPLETE         0x0         /* No errors */
#define CS_INCOMPLETE       0x1         /* Incomplete transfer of cmd. */
#define CS_DMA              0x2         /* A DMA direction error. */
#define CS_TRANSPORT        0x3         /* Transport error. */
#define CS_RESET            0x4         /* SCSI bus reset occurred */
#define CS_ABORTED          0x5         /* System aborted command. */
#define CS_TIMEOUT          0x6         /* Timeout error. */
#define CS_DATA_OVERRUN     0x7         /* Data overrun. */
#define CS_COMMAND_OVERRUN  0x8         /* Command Overrun. */
#define CS_STATUS_OVERRUN   0x9         /* Status Overrun. */
#define CS_BAD_MSG          0xA         /* Bad msg after status phase. */
#define CS_NO_MSG_OUT       0xB         /* No msg out after selection. */
#define CS_EXT_ID           0xC         /* Extended ID failed. */
#define CS_IDE_MSG          0xD         /* Target rejected IDE msg. */
#define CS_ABORT_MSG        0xE         /* Target rejected abort msg. */
#define CS_REJECT_MSG       0xF         /* Target rejected reject msg. */
#define CS_NOP_MSG          0x10        /* Target rejected NOP msg. */
#define CS_PARITY_MSG       0x11        /* Target rejected parity msg. */
#define CS_DEV_RESET_MSG    0x12        /* Target rejected dev rst msg. */
#define CS_ID_MSG           0x13        /* Target rejected ID msg. */
#define CS_FREE             0x14        /* Unexpected bus free. */
#define CS_DATA_UNDERRUN    0x15        /* Data Underrun. */
#define CS_TRANACTION_1     0x18        /* Transaction error 1 */
#define CS_TRANACTION_2     0x19        /* Transaction error 2 */
#define CS_TRANACTION_3     0x1a        /* Transaction error 3 */
#define CS_INV_ENTRY_TYPE   0x1b        /* Invalid entry type */
#define CS_DEV_QUEUE_FULL   0x1c        /* Device queue full */
#define CS_PHASED_SKIPPED   0x1d        /* SCSI phase skipped */
#define CS_ARS_FAILED       0x1e        /* ARS failed */
#define CS_BAD_PAYLOAD      0x80        /* Driver defined */
#define CS_UNKNOWN          0x81        /* Driver defined */
#define CS_RETRY            0x82        /* Driver defined */
#define CS_DRIVER_ABORT     0x83        /* Driver defined */

/*
 * ISP status entry - SCSI status byte bit definitions.
 */
#define SS_CHECK_CONDITION  BIT_1
#define SS_CONDITION_MET    BIT_2
#define SS_BUSY_CONDITION   BIT_3
#define SS_RESERVE_CONFLICT (BIT_4 | BIT_3)

/*
 * ISP target entries - Option flags bit definitions.
 */
#define OF_ENABLE_TAG       BIT_1       /* Tagged queue action enable */
#define OF_DATA_IN          BIT_6       /* Data in to initiator */
                                        /*  (data from target to initiator) */
#define OF_DATA_OUT         BIT_7       /* Data out from initiator */
                                        /*  (data from initiator to target) */
#define OF_NO_DATA          (BIT_7 | BIT_6)
#define OF_DISC_DISABLED    BIT_15      /* Disconnects disabled */
#define OF_DISABLE_SDP      BIT_24      /* Disable sending save data ptr */
#define OF_SEND_RDP         BIT_26      /* Send restore data pointers msg */
#define OF_FORCE_DISC       BIT_30      /* Disconnects mandatory */
#define OF_SSTS             BIT_31      /* Send SCSI status */

/*
 * Target Read/Write buffer structure.
 */
#define TARGET_DATA_OFFSET  4
#define TARGET_DATA_SIZE    0x2000      /* 8K */
#define TARGET_INQ_OFFSET   (TARGET_DATA_OFFSET + TARGET_DATA_SIZE)
#define TARGET_SENSE_SIZE   18
#define TARGET_BUF_SIZE     36

typedef struct
{
    uchar_t         hdr[4];
    uchar_t         data[TARGET_DATA_SIZE];
    struct ident    inq;
}tgt_t;

/*
 * UnixWare Host Adapter structure
 */
typedef struct
{
    int             global_number;      /* Controller global number. */

    /* Adapter configurations data */
    device_reg_t    *iobase;            /* Base I/O address */
    ushort_t        id;                 /* Host adapter SCSI id */
    ushort_t        bus_reset_delay;    /* SCSI bus reset delay. */
    ushort_t        device_enables;     /* Device enable bits. */
    ushort_t        lun_disables;       /* LUN disable bits. */
    ushort_t        qtag_enables;       /* Tag queue enables. */

    /* Device LUN queues. */
    ushort_t        hiwat;              /* High water mark per device. */
    scsi_lu_t       **dev;              /* Logical unit queues */

    /* Interrupt lock, and data */
    lock_t          *intr_lock;         /* Lock for interrupt locking */
    pl_t            intr_opri;          /* Saved Priority Level */
    uint_t          io_vector;          /* Interrupt vector number. */

    /* Received ISP mailbox data. */
    volatile ushort_t mailbox_out[MAILBOX_REGISTER_COUNT];

    /* Outstandings ISP commands. */
    srb_t           *outstanding_cmds[MAX_OUTSTANDING_COMMANDS];

    /* ISP ring lock, rings, and indexes */
    lock_t          *ring_lock;         /* ISP ring lock */
    pl_t            ring_opri;          /* Saved Priority Level */

    paddr32_t       request_dma[2];     /* Physical address. */
    request_t       *request_ring;      /* Base virtual address */
    request_t       *request_ring_ptr;  /* Current address. */
    ushort_t        req_ring_index;     /* Current index. */
    ushort_t        req_q_cnt;          /* Number of available entries. */

    paddr32_t       response_dma[2];    /* Physical address. */
    response_t      *response_ring;     /* Base virtual address */
    response_t      *response_ring_ptr; /* Current address. */
    ushort_t        rsp_ring_index;     /* Current index. */

    /* Target buffer and sense data. */
    paddr32_t       tbuf_dma[2];        /* Physical address. */
    tgt_t           *tbuf;
    paddr32_t       tsense_dma[2];      /* Physical address. */
    uchar_t         *tsense;

    /* Watchdog queue, lock and total timer */
    lock_t          *watchdog_q_lock;   /* Lock for watchdog queue */
    srb_t           *wdg_q_first;       /* First job on watchdog queue */
    srb_t           *wdg_q_last;        /* Last job on watchdog queue */
    time_t          total_timeout;      /* Total timeout (quantum count) */

    volatile struct
    {
        uchar_t     watchdog_enabled        :1;
        uchar_t     mbox_int                :1;
        uchar_t     mbox_busy               :1;
        uchar_t     online                  :1;
        uchar_t     reset_marker            :1;
        uchar_t     sdi_aen                 :1;
        uchar_t     isp_abort_needed        :1;
        uchar_t     disable_host_adapter    :1;
        uchar_t     abort_isp_active        :1;
        uchar_t     disable_risc_code_load  :1;
        uchar_t     disable_scsi_reset      :1;
    }flags;
}scsi_ha_t;

/*
 * Macros to help code, maintain, etc.
 */
#define SUBDEV(t, l)    (t << 5 | l)
#define LU_Q(t, l)      (ha->dev[SUBDEV(t, l)])
/*
 * Locking Macro Definitions
 *
 * LOCK/UNLOCK definitions are lock/unlock primitives for multi-processor
 * or spl/splx for uniprocessor.
 */
#define QLC1020_HIER   HBA_HIER_BASE  /* Locking hierarchy base for hba */
#define QLC1020_SCSILU_LOCK(p)        p = LOCK(q->q_lock, pldisk)
#define QLC1020_INTR_LOCK(p)          p = LOCK(ha->intr_lock, pldisk)
#define QLC1020_RING_LOCK(p)          p = LOCK(ha->ring_lock, pldisk)
#define QLC1020_WATCHDOG_Q_LOCK(p)    p = LOCK(ha->watchdog_q_lock, pldisk)

#define QLC1020_SCSILU_UNLOCK(p)      UNLOCK(q->q_lock, p)
#define QLC1020_INTR_UNLOCK(p)        UNLOCK(ha->intr_lock, p)
#define QLC1020_RING_UNLOCK(p)        UNLOCK(ha->ring_lock, p)
#define QLC1020_WATCHDOG_Q_UNLOCK(p)  UNLOCK(ha->watchdog_q_lock, p)

#if defined(__cplusplus)
}
#endif

#endif /* _IO_HBA_QLC1020_H */
