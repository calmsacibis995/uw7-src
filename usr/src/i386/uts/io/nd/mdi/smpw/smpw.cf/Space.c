/*
** Copyright (C) 1994-1997 by
** Digital Equipment Corporation, All rights reserved.
**
** This software is furnished under a license and may be used and copied
** only  in  accordance  with  the  terms  of such  license and with the
** inclusion of the above copyright notice. This software or  any  other
** copies thereof may not be provided or otherwise made available to any
** other person. No title to and ownership of  the  software  is  hereby
** transferred.
**
** The information in this software is subject to change without  notice
** and  should  not be  construed  as  a commitment by Digital Equipment
** Corporation.
**
** Digital assumes no responsibility for the use or  reliability  of its
** software on equipment which is not supplied by Digital.
**
*/

#include "sys/types.h"
#ifdef REMOVE_ME_LATER /* app for LLI/MDI */
#include "config.h"
#endif
#ifndef GEMINI_INTERFACE
#define GEMINI_INTERFACE 1
#endif


#ifdef MDI_INTERFACE  /*ANTANT ditto in src_space.h */
#include "sys/pci.h"
#endif

#define XXX_MAX_MEDIA_TABLE              SMPW_MAX_MEDIA_TABLE
#define XXX_media_types_t                SMPW_media_types_t
#define XXX_burst_length_t               SMPW_burst_length_t
#define XXX_CACHE_ILLEGAL                SMPW_CACHE_ILLEGAL
#define XXX_cache_alignment_t            SMPW_cache_alignment_t
#define XXX_10MB_threshold_t             SMPW_10MB_threshold_t
#define XXX_crc_calc_t                   SMPW_crc_calc_t
#define xxx_turbo                        smpw_turbo
#define XXX_board_config_t               SMPW_board_config_t
#define XXX_pci_devinfo_t                SMPW_pci_devinfo_t
#define XXX_info                         smpw_info
#define xxx_debug                        smpw_debug
#define xxx_max_boards                   smpw_max_boards
#define xxx_Boards_table                 smpw_Boards_table
#define xxxinit                          smpwinit
#define xxxintr                          smpwintr
#define xxx1intr                         smpw1intr
#define xxx2intr                         smpw2intr
#define xxx3intr                         smpw3intr
#define xxxpoll                          smpwpoll
#define xxx1poll                         smpw1poll
#define xxx2poll                         smpw2poll
#define xxx3poll                         smpw3poll
#define xxx_start_adapter                smpw_start_adapter
#define xxx_stop_adapter                 smpw_stop_adapter
#define xxx_broken_adapter               smpw_broken_adapter
#define xxx_init_cntrs                   smpw_init_cntrs

#define xxx_init_sia_regs                smpw_init_sia_regs
/*
#define xxx_initialize_gep_register      smpw_initialize_gep_register */
#define xxx_WriteCSR6					 smpw_WriteCSR6

#define xxx_ptok                         smpw_ptok
#define xxx_ktop                         smpw_ktop
#define xxxpoll_disabled                 smpwpoll_disabled
#define xxxpoll_prev                     smpwpoll_prev
#define xxx_crctable                     smpw_crctable
#define xxx_macioc_setmca                smpw_macioc_setmca
#define xxx_macioc_delmca                smpw_macioc_delmca
#define xxx_macioc_getmca                smpw_macioc_getmca
#define xxx_macioc_getaddr               smpw_macioc_getaddr
#define xxx_macioc_setaddr               smpw_macioc_setaddr
#define xxx_macioc_promisc               smpw_macioc_promisc
#define xxx_macioc_getmcsiz              smpw_macioc_getmcsiz
#define xxx_handle_rx_filter_mode        smpw_handle_rx_filter_mode
#define xxx_update_csr6                  smpw_update_csr6
#define xxx_create_setupacket            smpw_create_setupacket
#define xxx_poll_on_setupacket           smpw_poll_on_setupacket
#define xxx_find_addr_in_board_table     smpw_find_addr_in_board_table
#define xxx_setupacket_handler           smpw_setupacket_handler
#define xxx_compare_addrs                smpw_compare_addrs
#define xxx_m_data                       smpw_m_data
#define xxx_process_rx_pkts              smpw_process_rx_pkts
#define xxx_alloc_rx_bufs                smpw_alloc_rx_bufs
#define xxx_bufcall_rx_bufs              smpw_bufcall_rx_bufs
#define xxx_proc_and_free_tx_bufs        smpw_proc_and_free_tx_bufs
#define xxx_check_ioctl_message          smpw_check_ioctl_message
#define xxx_iocblk_ack                   smpw_iocblk_ack
#define xxx_flush_handler                smpw_flush_handler
#define xxx_ioctl_handler                smpw_ioctl_handler
#define xxx_proto_pcproto_handler        smpw_proto_pcproto_handler
#define xxx_mac_hwfail_ind               smpw_mac_hwfail_ind
#define xxx_check_pci_bus                smpw_check_pci_bus
#define xxx_parse_srom                   smpw_parse_srom
#define xxx_calculate_checksum           smpw_calculate_checksum
#define xxx_check_if_board_in_system     smpw_check_if_board_in_system
#define xxx_init_board                   smpw_init_board
#define xxx_handle_pci_turbo             smpw_handle_pci_turbo
#define xxx_autosense_intr_handler       smpw_autosense_intr_handler
#define xxx_dc21041_dynamic_autosense    smpw_dc21041_dynamic_autosense
#define xxx_dc21140_dynamic_autosense    smpw_dc21140_dynamic_autosense
#define xxx_autodetection                smpw_autodetection
#define xxx_free_rx_resources            smpw_free_rx_resources
#define xxx_free_tx_resources            smpw_free_tx_resources
#define xxx_alloc_rx_list                smpw_alloc_rx_list
#define xxx_alloc_tx_list                smpw_alloc_tx_list
#define xxx_alloc_setup_buf              smpw_alloc_setup_buf
#define xxx_alloc_dummy_pkt              smpw_alloc_dummy_pkt
#define xxx_init_config_reg_vals         smpw_init_config_reg_vals
#define xxx_init_config_regs_map         smpw_init_config_regs_map
#define xxx_init_regs_map                smpw_init_regs_map
#define xxx_init_ctrl_reg_vals           smpw_init_ctrl_reg_vals
#define XXX_100MB_threshold_t            SMPW_100MB_threshold_t
#define XXX_module_prefix		 SMPW_module_prefix

#define SMPW_0_PCI_BUS -1
#define SMPW_0_PCI_DEV -1
#define SMPW_0_PCI_FUNC -1

#define SMPW_1_PCI_BUS -1
#define SMPW_1_PCI_DEV -1
#define SMPW_1_PCI_FUNC -1

#define SMPW_2_PCI_BUS -1
#define SMPW_2_PCI_DEV -1
#define SMPW_2_PCI_FUNC -1

#define SMPW_3_PCI_BUS -1
#define SMPW_3_PCI_DEV -1
#define SMPW_3_PCI_FUNC -1


#define SMPW_0_MEDIA     ((ushort)-1)
#define SMPW_1_MEDIA     ((ushort)-1) 
#define SMPW_2_MEDIA     ((ushort)-1) 
#define SMPW_3_MEDIA     ((ushort)-1)
	
#define XXX_TRUE     1
#define XXX_FALSE    0


/*
** DC21X4 debug's level
*/

typedef enum
    {
    XXX_LEVEL_0,
    XXX_LEVEL_1,
    XXX_LEVEL_2,
    XXX_LEVEL_3
    } XXX_debug_level;



/*
** Media types for DC21040, DC2104, DC21140 and DC21142
*/

typedef enum
    {
    XXX_TP,
    XXX_BNC,
    XXX_AUI,
    XXX_100TX,
    XXX_TP_FDX,
    XXX_100TX_FDX,
    XXX_100T4,
    XXX_100FX,
    XXX_100FX_FDX,
    XXX_MAX_MEDIA_TABLE
    } XXX_media_types_t;



/* Phy  Mediums */

#define XXX_MII_TP                  0x0009
#define XXX_MII_TP_FDX              0x000A
#define XXX_MII_BNC                 0x000B
#define XXX_MII_AUI                 0x000C
#define XXX_MII_100TX               0x000D
#define XXX_MII_100TX_FDX           0x000E
#define XXX_MII_100T4               0x000F
#define XXX_MII_100FX               0x0010
#define XXX_MII_100FX_FDX           0x0011


#define XXX_NWAY           0x100
#define XXX_FULL_DUPLEX    0x200
#define XXX_LINK_DISABLE   0x400
#define XXX_AUTOSENSE      0x800


/*
** DC21X4 - SIA/serial modes
*/

#define XXX_TP_FULL_DUPLEX              (XXX_TP_FDX | XXX_FULL_DUPLEX)


/*
** DC2104X - added SIA modes
*/

#define XXX_TP_LINK_DISABLE             (XXX_TP | XXX_LINK_DISABLE)
/* #if LLI_INTERFACE
#define XXX_DC2104X_DEF_SERIAL_MODE     0xFFFE
!!!   #endif
 */


/*
** DC2114X - added serial modes
*/

#define XXX_100TX_FULL_DUPLEX          (XXX_100TX_FDX | XXX_FULL_DUPLEX)
#define XXX_100FX_FULL_DUPLEX          (XXX_100FX_FDX | XXX_FULL_DUPLEX)
/* #if LLI_INTERFACE
#define XXX_DC2114X_DEF_SERIAL_MODE     0xFFFE
 !!!  #endif
 */

#define XXX_MII_TP_FULL_DUPLEX      (XXX_MII_TP_FDX | XXX_FULL_DUPLEX)
#define XXX_MII_100TX_FULL_DUPLEX   (XXX_MII_100TX_FDX | XXX_FULL_DUPLEX)
#define XXX_MII_100FX_FULL_DUPLEX   (XXX_MII_100FX_FDX | XXX_FULL_DUPLEX)
#define XXX_MII_AUTOSENSE           (XXX_MII_TP | XXX_AUTOSENSE)

/* #if MDI_INTERFACE */
#define XXX_DC21X4_DEF_SERIAL_MODE      0xFFFE
/* #endif */


/*
** Burst Length
*/

typedef enum
    {
    XXX_PBL_0  = 0,
    XXX_PBL_1  = 1,
    XXX_PBL_2  = 2,
    XXX_PBL_4  = 4,
    XXX_PBL_8  = 8,
    XXX_PBL_16 = 16,
    XXX_PBL_32 = 32,
    XXX_PBL_DEFAULT = 0xFF
    } XXX_burst_length_t;


/*
** Cache Alignment
*/

typedef enum
    {
    XXX_CACHE_ILLEGAL = 0,
    XXX_CACHE_8 = 1,
    XXX_CACHE_16 = 2,
    XXX_CACHE_32 = 3,
    XXX_CACHE_DEFAULT = 0xFF
    } XXX_cache_alignment_t;

/*
** TX FIFO Threshold for 10MB
*/

typedef enum
    {
    XXX_10MB_THRESHOLD_72 = 0,
    XXX_10MB_THRESHOLD_96,
    XXX_10MB_THRESHOLD_128,
    XXX_10MB_THRESHOLD_160
    } XXX_10MB_threshold_t;


/*
** TX FIFO Threshold for 100MB
*/

typedef enum
    {
    XXX_100MB_THRESHOLD_128 = 0,
    XXX_100MB_THRESHOLD_256,
    XXX_100MB_THRESHOLD_512,
    XXX_100MB_THRESHOLD_1024,
    XXX_100MB_THRESHOLD_FULL = 0xFFFF
    } XXX_100MB_threshold_t;




/*
** HW/SW CRC calculation
*/

typedef enum
    {
    XXX_SW_CRC_ALWAYS = 0,
    XXX_SW_CRC_NEVER
    } XXX_crc_calc_t;


/*
** DC2114X's general purpose register - use the default vale
*/

#define XXX_GPR_DEFAULT      0xFFFF

/*
**  This is to emulate the MDI's PCI API scheme
**  These slotnum, funcnum, busnum are filled when ...find_d21x4_board ()
**  is called.
**  Once SCO implement these APIs in GEMINI then it can be removed
*/

typedef struct pci_devinfo_tag
   {
	short slotnum;
	short funcnum;
	short busnum;
   }XXX_pci_devinfo_t;


typedef struct
   {
   ulong                     board_num;
   char                      *board_id;
   uint                      vector;
   ulong                     iobase;
   ulong                     ioend;
   int                       pci_bus;
   int                       pci_dev;
   int                       pci_fun;
   uint                      rx_bufs_to_post;
/* #if MDI_INTERFACE */
   ushort                    DC21X4_serial_mode;
/* #endif */
   uint                      bus_arbitration;
   XXX_burst_length_t        burst_length;
   XXX_cache_alignment_t     cache_alignment;
   uint                      pass_bad_frames;
   uint                      backoff_cntr;
   XXX_10MB_threshold_t      threshold;
   XXX_100MB_threshold_t     threshold_100;
   uint                      parity_error_response;
   uint                      latency_timer;
#ifdef MDI_INTERFACE  /*ANTANT ditto in src_space.h */
   struct pci_devinfo        pci_device_info;
#endif
#ifdef GEMINI_INTERFACE  /*ANTANT need to put in src_space.h and space.c*/
   XXX_pci_devinfo_t        pci_device_info; /* need slotnum to CreateNIC */
#endif
   XXX_crc_calc_t            crc_calc;
   uint                      dc2114x_gpr_ctrl;
   uint                      dc2114x_gpr_data;
   uint                      xxx_turbo;
   uint                      snooze_mode;
   unchar					 mwi_key;
   unchar					 mrm_key;
   unchar					 mrl_key;
   unchar                    mac_addr[6];
#ifdef GEMINI_INTERFACE  /*ANTANT need to put in src_space.h and space.c*/
   char                      pci_id_string[11];
#endif
   } XXX_board_config_t;

/*
** Enable/disable diagnostic messages. Allowed values are:
**    XXX_LEVEL_3  - enable debug message in level 1,2,3.
**    XXX_LEVEL_2  - enable debug message in level 1,2.
**    XXX_LEVEL_1  - enable debug message in level 1.
**    XXX_LEVEL_0 - disable debug message
*/
#define XXX_DEBUG      XXX_LEVEL_0


#ifdef REMOVE_ME_LATER
#define XXX_HW_ACCESSES  XXX_USE_PCI_BIOS
#endif
/*
** Highest vector number supported. Needed to allocate large enough array
** Should NOT be modified
*/
#define XXX_MAX_VECTOR_NUMS            16

/*
** Number of buffers pre-allocated on the receive list
*/
#define XXX_RX_BUFS_TO_POST            6



/*
** Serial mode for All boards ! . Can have one of the
** following values:
** for DC21040 and DC21041 :
**     XXX_TP                          - 10 MB/sec TP connection
**     XXX_BNC                         - 10 MB/sec BNC connection
**     XXX_AUI                         - 10 MB/sec AUI connection
**     XXX_TP_FULL_DUPLEX              - point to point full duplex, 10 MB/sec
**                                       TP connection
**     XXX_TP_LINK_DISABLE             - Twisted pair connection with linkfail
**                                       test ignore & set polarity plus
**     XXX_DC21X4_DEF_SERIAL_MODE     - choose the value that was read from
**                                       the SROM if read, else choose the
**                                       AUTOSENSE mode
**
**
** for DC21140 and DC21142 :
**     XXX_TP                          - 10 MB/sec TP connection
**     XXX_BNC                         - 10 MB/sec BNC connection
**     XXX_AUI                         - 10 MB/sec AUI connection
**     XXX_TP_FULL_DUPLEX              - point to point full duplex, 10 MB/sec
**                                       TP connection
**     XXX_TP_LINK_DISABLE             - Twisted pair connection with linkfail
**                                       test ignore & set polarity plus - for
**                                       DC21142 only !
**     XXX_100TX                       - 100TX MB/sec mode
**     XXX_100TX_FULL_DUPLEX           - point to point full duplex, 100TX MB/sec
**                                       mode
**     XXX_100FX                       - 100 MB/sec FX mode
**     XXX_100FX_FULL_DUPLEX           - point to point full duplex, 100 MB/sec
**                                       FX mode
**     XXX_100T4                       - 100T4 MB/sec mode
**     XXX_AUTOSENSE                   - Automatic detection between all
**                                       supported media (except FULL DUPLEX).
**     XXX_DC21X4_DEF_SERIAL_MODE     - choose the value that was read from
**                                       the SROM if read, else choose the
**                                       AUTOSENSE mode
**
** Important Note!
** This definition is a global definition for all installed boards.
** If you want to set a different mode for each board, please modify
** the 'DC21X4_serial_mode' field in the appropriate part of the XXX_info
** structure below (the values to use are those listed above).
*/
#define XXX_DC21X4_SERIAL_MODE          XXX_AUTOSENSE

/*
** Selects internal bus arbitration between the receive and transmit
** processing. Allowed values:
**    XXX_TRUE  - round robin arbitration, equal sharing
**    XXX_FALSE - receive process has priority over the transmit process
**                (default)
*/
#define XXX_BUS_ARBITRATION   XXX_FALSE

/*
** Maximum number of longwords transferred in a DMA transaction.
** Allowed values are:
**     XXX_PBL_0 (0),
**     XXX_PBL_1 (1),
**     XXX_PBL_2 (2),
**     XXX_PBL_4 (4),
**     XXX_PBL_8 (8),
**     XXX_PBL_16 (16),
**     XXX_PBL_32 (32),
**     XXX_PBL_DEFAULT (32 for DC21140/DC21142 and 16 for DC21040/DC21041).
** NOTE : Burst length of XXX_PBL_0 means the only limit is the amount
**        of data stored.
** NOTE : Burst length of XXX_PBL_0 or XXX_PBL_32 are not supported in
**        21040/21041 boards due to chips' limitation. If the user
**        will choose one of those values, the driver will change the
**        burst length to 16. No warning is issued about this.
**
** NOTE : When the driver is working on a PCI machine with the CDC chip
**        (rev 3) or with the CDC chip (rev 2) it initializes the DC21X4 burst
**        length to XXX_PBL_8 and ignores the value that is given by the user
**        in space.c . No warning is issued about this (SCO Unix boot info
**        limitations)
*/
#define XXX_BURST_LENGTH      XXX_PBL_DEFAULT

/*
** Address boundary for data burst stop. Allowed values are:
**    XXX_CACHE_8
**    XXX_CACHE_16
**    XXX_CACHE_32
**    XXX_CACHE_DEFAULT (will be the same as the burst length)
*/
#define XXX_CACHE_ALIGNMENT   XXX_CACHE_DEFAULT

/*
** Enable/disable passing of bad frames (runt frames, truncated frames,
** collided frames) from the adapter to the driver.
**    XXX_TRUE  - enables passing of bad frames
**    XXX_FALSE - disable passingof bad frames (default)
*/
#define XXX_PASS_BAD_FRAMES   XXX_FALSE

/*
** Backoff counter algorithm.
**    XXX_TRUE  - stop counter when any carrier activity is detected
**    XXX_FALSE - backoff counter is not affected by the carrier activity
**                (default)
*/
#define XXX_BACKOFF_CNTR      XXX_FALSE

/*
** Transmit FIFO threshold for starting packet transmission to line (for
** 10MB mode).
** Allowed values are:
**    XXX_10MB_THRESHOLD_72
**    XXX_10MB_THRESHOLD_96 (default)
**    XXX_10MB_THRESHOLD_128
**    XXX_10MB_THRESHOLD_160
*/
#define XXX_10MB_THRESHOLD         XXX_10MB_THRESHOLD_96

/*
** Transmit FIFO threshold for starting packet transmission to line (for
** 100MB mode).
** Allowed values are:
**    XXX_100MB_THRESHOLD_128
**    XXX_100MB_THRESHOLD_256
**    XXX_100MB_THRESHOLD_512
**    XXX_100MB_THRESHOLD_1024
**    XXX_100MB_THRESHOLD_FULL (default) - full packet
*/
#define XXX_100MB_THRESHOLD         XXX_100MB_THRESHOLD_FULL

/*
** Enable/disable parity checking on PCI bus
**    XXX_TRUE  - enable parity checking
**    XXX_FALSE - disable parity checking (default)
*/
#define XXX_PARITY_ERROR_RESPONSE XXX_FALSE

/*
** DC21040/DC21140/DC21041/DC21142 PCI Latency Timer, in units of PCI bus
** clocks. Default is 50h
**
** Note : If the user chooses a value for the PCI Latency Timer that is < 50h
**        the driver changes this value to 50h, since the driver works with
**        the minimum value - 50h.
*/
#define XXX_LATENCY_TIMER     0x50

/*
** Enable/Disable SW's CRC calculation (in transmit packets)
**    XXX_SW_CRC_ALWAYS - CRC will be calculated by the software for every
**                        transmit packet longer than the threshold, for
**                        transmit packets shorter than the threshold the HW
**                        will calculate the CRC
**    XXX_SW_CRC_NEVER  - CRC will be calculated by the hardware for every
**                        transmit packet
*/
#define XXX_CRC_CALC          XXX_SW_CRC_ALWAYS

/*
** The control and data values to be written to the DC21140/DC21142 general
** purpose register.
**    XXX_GPR_DEFAULT - use the value found in the SROM. If no value
**                      is found in the SROM the driver uses its default
**                      values.
** The user can override the SROM default by setting specific values to
** XXX_DC2114X_CTRL/DATA.
**
** NOTE: The SROM values can't be over-ridden if the XXX_DC2114X_SERIAL_MODE
**       parameter is set to AUTOSENSE (or if it is set to use the SROM
**       default and the SROM value is AUTOSENSE). In this case these values
**       are ignored and no message is given.
*/
#define XXX_DC2114X_GPR_CTRL  XXX_GPR_DEFAULT
#define XXX_DC2114X_GPR_DATA  XXX_GPR_DEFAULT


/*
** This constant is used to enable/disable PCI priority over EISA.
**
** XXX_TRUE  - PCI has priority over EISA
** XXX_FALSE - EISA has priority over PCI
**
*/

#define XXX_TURBO             XXX_TRUE


/*
** Enable/disable snooze mode.
**    XXX_TRUE  - enable snooze mode
**    XXX_FALSE - disable snooze mode (default)
*/
#define XXX_SNOOZE            XXX_FALSE


#define XXX_UNDEF_USER		0xFF

/* 
** Enable/disable Extended PCI command Memory Write Invalidate (MWI)
**    XXX_TRUE  - enable the use of MWI commnad
**    XXX_FALSE - disable the use of MWI command
**
**    XXX_UNDEF_USER - (default)
*/
#define XXX_MWI				XXX_UNDEF_USER

/* 
** Enable/disable Extended PCI command Memory Read Multiple (MRM)
**    XXX_TRUE  - enable the use of MRM commnad
**    XXX_FALSE - disable the use of MRM command
**
**    XXX_UNDEF_USER - (default)
*/
#define XXX_MRM				XXX_UNDEF_USER

/* 
** Enable/disable Extended PCI command Memory Read Line (MRL)
**    XXX_TRUE  - enable the use of MRL commnad
**    XXX_FALSE - disable the use of MRL command
**
**    XXX_UNDEF_USER - (default)
*/
#define XXX_MRL				XXX_UNDEF_USER


/*
** Enable to change the default MAC address.
** If the address is 00-00-00-00-00-00 then the mac address is taken from the
** SROM else it taken from here
*/
#define XXX_0_MAC_ADDR  0x00,0x00,0x00,0x00,0x00,0x00
#define XXX_1_MAC_ADDR  0x00,0x00,0x00,0x00,0x00,0x00
#define XXX_2_MAC_ADDR  0x00,0x00,0x00,0x00,0x00,0x00
#define XXX_3_MAC_ADDR  0x00,0x00,0x00,0x00,0x00,0x00
#ifdef GEMINI_INTERFACE		/* ANTANT */
#define XXX_MAX_BOARDS        4
#endif


XXX_board_config_t XXX_info[XXX_MAX_BOARDS] =
{
   {
   0,                            /* board_num */
   "smpw0",                      /* board_id */
   0,                   /* vector */
   0,                   /* iobase */
   0,                   /* ioend */
   SMPW_0_PCI_BUS,               /* PCI bus number */
   SMPW_0_PCI_DEV,               /* PCI device number */
   SMPW_0_PCI_FUNC,              /* PCI function number */
   XXX_RX_BUFS_TO_POST,          /* rx_bufs_to_post */
   SMPW_0_MEDIA,                 /* DC21X4_serial_mode */
   XXX_BUS_ARBITRATION,          /* bus_arbitration */
   XXX_BURST_LENGTH,             /* burst_length */
   XXX_CACHE_ALIGNMENT,          /* cache_alignment */
   XXX_PASS_BAD_FRAMES,          /* pass_bad_frames */
   XXX_BACKOFF_CNTR,             /* backoff_cntr */
   XXX_10MB_THRESHOLD,           /* threshold */
   XXX_100MB_THRESHOLD,          /* threshold_100 */
   XXX_PARITY_ERROR_RESPONSE,    /* parity_error_response */
   XXX_LATENCY_TIMER,            /* latency_timer */
   {SMPW_0_PCI_DEV, SMPW_0_PCI_FUNC, SMPW_0_PCI_BUS},  /* PCI device info */
   XXX_CRC_CALC,                 /* SW/HW CRC calculation */
   XXX_DC2114X_GPR_CTRL,         /* DC2114X's general purpose register control
                                    value */
   XXX_DC2114X_GPR_DATA,         /* DC2114X's general purpose register data
                                    value */
   XXX_TURBO,                    /* Enable/disable PCI priority over EISA */
   XXX_SNOOZE,                   /* snooze_mode */
   XXX_MWI,						 /* Enable/Disable MWI extended PCI command */
   XXX_MRM,						 /* Enable/Disable MRM extended PCI command */
   XXX_MRL,						 /* Enable/Disable MRL extended PCI command */
   XXX_0_MAC_ADDR                /* Mac address */
   }



   ,{
   1,                            /* board_num */
   "smpw1",                      /* board_id */
   0,                   /* vector */
   0,                   /* iobase */
   0,                   /* ioend */
   SMPW_1_PCI_BUS,               /* PCI bus number */
   SMPW_1_PCI_DEV,               /* PCI device number */
   SMPW_1_PCI_FUNC,              /* PCI function number */
   XXX_RX_BUFS_TO_POST,          /* rx_bufs_to_post */
   SMPW_1_MEDIA,                 /* DC21X4_serial_mode */
   XXX_BUS_ARBITRATION,          /* bus_arbitration */
   XXX_BURST_LENGTH,             /* burst_length */
   XXX_CACHE_ALIGNMENT,          /* cache_alignment */
   XXX_PASS_BAD_FRAMES,          /* pass_bad_frames */
   XXX_BACKOFF_CNTR,             /* backoff_cntr */
   XXX_10MB_THRESHOLD,           /* threshold */
   XXX_100MB_THRESHOLD,          /* threshold_100 */
   XXX_PARITY_ERROR_RESPONSE,    /* parity_error_response */
   XXX_LATENCY_TIMER,            /* latency_timer */
   {SMPW_1_PCI_DEV, SMPW_1_PCI_FUNC, SMPW_1_PCI_BUS},  /* PCI device info */
   XXX_CRC_CALC,                 /* SW/HW CRC calculation */
   XXX_DC2114X_GPR_CTRL,         /* DC2114X's general purpose register control
                                    value */
   XXX_DC2114X_GPR_DATA,         /* DC2114X's general purpose register data
                                    value */
   XXX_TURBO,                    /* Enable/disable PCI priority over EISA */
   XXX_SNOOZE,                   /* snooze_mode */
   XXX_MWI,						 /* Enable/Disable MWI extended PCI command */
   XXX_MRM,						 /* Enable/Disable MRM extended PCI command */
   XXX_MRL,						 /* Enable/Disable MRL extended PCI command */
   XXX_1_MAC_ADDR                /* Mac address */
   }





   ,{
   2,                            /* board_num */
   "smpw2",                      /* board_id */
   0,                   /* vector */
   0,                   /* iobase */
   0,                   /* ioend */
   SMPW_2_PCI_BUS,               /* PCI bus number */
   SMPW_2_PCI_DEV,               /* PCI device number */
   SMPW_2_PCI_FUNC,              /* PCI function number */
   XXX_RX_BUFS_TO_POST,          /* rx_bufs_to_post */
   SMPW_2_MEDIA,                 /* DC21X4_serial_mode */
   XXX_BUS_ARBITRATION,          /* bus_arbitration */
   XXX_BURST_LENGTH,             /* burst_length */
   XXX_CACHE_ALIGNMENT,          /* cache_alignment */
   XXX_PASS_BAD_FRAMES,          /* pass_bad_frames */
   XXX_BACKOFF_CNTR,             /* backoff_cntr */
   XXX_10MB_THRESHOLD,           /* threshold */
   XXX_100MB_THRESHOLD,          /* threshold_100 */
   XXX_PARITY_ERROR_RESPONSE,    /* parity_error_response */
   XXX_LATENCY_TIMER,            /* latency_timer */
   {SMPW_2_PCI_DEV, SMPW_2_PCI_FUNC, SMPW_2_PCI_BUS},  /* PCI device info */
   XXX_CRC_CALC,                 /* SW/HW CRC calculation */
   XXX_DC2114X_GPR_CTRL,         /* DC2114X's general purpose register control
                                    value */
   XXX_DC2114X_GPR_DATA,         /* DC2114X's general purpose register data
                                    value */
   XXX_TURBO,                    /* Enable/disable PCI priority over EISA */
   XXX_SNOOZE,                  /* snooze_mode */
   XXX_MWI,						 /* Enable/Disable MWI extended PCI command */
   XXX_MRM,						 /* Enable/Disable MRM extended PCI command */
   XXX_MRL,						 /* Enable/Disable MRL extended PCI command */
   XXX_2_MAC_ADDR                /* Mac address */
   }








   ,{
   3,                            /* board_num */
   "smpw3",                      /* board_id */
   0,                   /* vector */
   0,                   /* iobase */
   0,                   /* ioend */
   SMPW_3_PCI_BUS,               /* PCI bus number */
   SMPW_3_PCI_DEV,               /* PCI device number */
   SMPW_3_PCI_FUNC,              /* PCI function number */
   XXX_RX_BUFS_TO_POST,          /* rx_bufs_to_post */
   SMPW_3_MEDIA,                 /* DC21X4_serial_mode */
   XXX_BUS_ARBITRATION,          /* bus_arbitration */
   XXX_BURST_LENGTH,             /* burst_length */
   XXX_CACHE_ALIGNMENT,          /* cache_alignment */
   XXX_PASS_BAD_FRAMES,          /* pass_bad_frames */
   XXX_BACKOFF_CNTR,             /* backoff_cntr */
   XXX_10MB_THRESHOLD,           /* threshold */
   XXX_100MB_THRESHOLD,          /* threshold_100 */
   XXX_PARITY_ERROR_RESPONSE,    /* parity_error_response */
   XXX_LATENCY_TIMER,            /* latency_timer */
   {SMPW_3_PCI_DEV, SMPW_3_PCI_FUNC, SMPW_3_PCI_BUS},  /* PCI device info */
   XXX_CRC_CALC,                 /* SW/HW CRC calculation */
   XXX_DC2114X_GPR_CTRL,         /* DC2114X's general purpose register control
                                    value */
   XXX_DC2114X_GPR_DATA,         /* DC2114X's general purpose register data
                                    value */
   XXX_TURBO,                    /* Enable/disable PCI priority over EISA */
   XXX_SNOOZE,                   /* snooze_mode */
   XXX_MWI,						 /* Enable/Disable MWI extended PCI command */
   XXX_MRM,						 /* Enable/Disable MRM extended PCI command */
   XXX_MRL,						 /* Enable/Disable MRL extended PCI command */
   XXX_3_MAC_ADDR                /* Mac address */
   },





};

unchar          xxx_debug       = XXX_DEBUG;
uint            xxx_max_boards  = XXX_MAX_BOARDS;
void            *xxx_Boards_table[XXX_MAX_BOARDS];

