#ident "@(#)Space.c	26.1"
/*
 * space.c
 * (c) 1994-1997, Intel.
 *
 */
#include <sys/types.h>
#include <sys/stream.h>
#include <net/if.h>
#include <config.h>
#include <sys/dlpi_ether.h>

char *eeE_oem_copyright="(c) 1997, Intel Corporation. All Rights Reserved";

/*
 *  The N_SAPS define determines how many protocol drivers can bind to a single
 *  EEE  board.  A TCP/IP environment requires a minimum of two (IP and ARP).
 *  Putting an excessively large value here would waste memory.  A value that
 *  is too small could prevent a system from supporting a desired protocol.
 */
#define	N_SAPS		8

/*
 *  The STREAMS_LOG define determines if STREAMS tracing will be done in the
 *  driver.  A non-zero value will allow the strace(1M) command to follow
 *  activity in the driver.  The driver ID used in the strace(1M) command is
 *  equal to the ENET_ID value (generally 2101).
 *
 *  NOTE:  STREAMS tracing can greatly reduce the performance of the driver
 *		 and should only be used for trouble shooting.
 */
#define	STREAMS_LOG	0

/*
 *  The IFNAME define determines the name of the the internet statistics
 *  structure for this driver and only has meaning if the inet package is
 *  installed.  It should match the interface prefix specified in the strcf(4)
 *  file and ifconfig(1M) command used in rc.inet.  The unit number of the
 *  interface will match the board number (i.e eeE0, eeE1, eeE2) and is not
 *  defined here.
 */
#define	IFNAME	"eeE"

/*
 * Configure the PCI Configuration Type
 *
 */
#define PCI_CONF_T1	1
#define PCI_CONF_T2	2
#define PCI_CONF_AUTO	0

#define PCI_CONF_TYPE	PCI_CONF_AUTO

/*
 *  Configure parameters for buffers per controller.
 *  If the machine this is being used on is a faster machine (i.e. > 150MHz)
 *  and running on a 10MBS network then more queueing of data occurs. This
 *  may indicate the some of the numbers below should be adjusted.  Here are
 *  some typical numbers:
 *                             MAX_TCB 100
 *                             MAX_TBD_PER_TCB 12
 *                             MAX_RFD 100
 *                             MAX_PRCOUNT 20
 *  The default numbers give work well on most systems tests so no real
 *  adjustments really need to take place.  Also, if the machine is connected
 *  to a 100MBS network the numbers described above can be lowered from the
 *  defaults as considerably less data will be queued.
 */                                                   

#define MAX_TCB		60	/* number of transmit control blocks */
#define MAX_TBD_PER_TCB	10
#define MAX_TBD		(MAX_TCB*MAX_TBD_PER_TCB)
#define MAX_RFD		60
#define EEE_CNTLS	4       /* number of receive control blocks */
#define EEE_MAJORS      1
#define TX_CONG_DFLT	0       /* congestion enable flag for National TX Phy */
#define TX_FIFO_LMT	0
#define RX_FIFO_LMT	8
#define TX_DMA_CNT 	0
#define RX_DMA_CNT 	0
#define TX_UR_RETRY	1
#define TX_THRSHLD	8
#define MAX_PRCOUNT	15      /* number of frames to process per interrupt */

/* These are special #defs NOT TO TOUCHED */
#define DEFAULT_TX_FIFO_LIMIT	   0x00
#define DEFAULT_RX_FIFO_LIMIT	   0x08
#define CB_557_CFIG_DEFAULT_PARM4   0x00
#define CB_557_CFIG_DEFAULT_PARM5   0x00
#define DEFAULT_TRANSMIT_THRESHOLD	  12	  /* 12 -> 96 bytes */
#define ADAPTIVE_IFS                0
/*end of specials */

/********************** STOP!  DON'T TOUCH THAT DIAL ************************
 *
 *  The following values are set by the kernel build utilities and should not
 *  be modified by mere motals.
 */
int		eeEboards;
int		eeEstrlog = STREAMS_LOG;
int	  	eeE_max_saps=N_SAPS;
char		*eeE_ifname = IFNAME;
int		eeE_pci_type=PCI_CONF_TYPE;
int      eeE_adaptive_ifs=ADAPTIVE_IFS;

uchar_t		eeE_rx_fifo_lmt= ( RX_FIFO_LMT < 0 )?  DEFAULT_RX_FIFO_LIMIT :
((RX_FIFO_LMT > 15 )? DEFAULT_RX_FIFO_LIMIT : 
RX_FIFO_LMT );

uchar_t		eeE_tx_fifo_lmt= ( TX_FIFO_LMT < 0 )?  DEFAULT_TX_FIFO_LIMIT :
((TX_FIFO_LMT > 7 )?  DEFAULT_TX_FIFO_LIMIT : 
TX_FIFO_LMT );

uchar_t		eeE_rx_dma_cnt= ( RX_DMA_CNT < 0 )? CB_557_CFIG_DEFAULT_PARM4 :
((RX_DMA_CNT  > 63 )? 
CB_557_CFIG_DEFAULT_PARM4 : RX_DMA_CNT );

uchar_t		eeE_tx_dma_cnt= ( TX_DMA_CNT < 0 )? CB_557_CFIG_DEFAULT_PARM5 :
((TX_DMA_CNT  > 63 )? 
CB_557_CFIG_DEFAULT_PARM5 : TX_DMA_CNT );

uchar_t		eeE_urun_retry= ( TX_UR_RETRY < 0 )? CB_557_CFIG_DEFAULT_PARM5:
((TX_UR_RETRY  > 3 )?  
CB_557_CFIG_DEFAULT_PARM5 : TX_UR_RETRY );

uchar_t		eeE_tx_thld= ( TX_THRSHLD < 0 )? DEFAULT_TRANSMIT_THRESHOLD :
((TX_THRSHLD > 200 )? DEFAULT_TRANSMIT_THRESHOLD :
TX_THRSHLD );

/* maximum # of receive buffs processed in each call to eeE ISR */
int			eeE_max_prcount= ( MAX_PRCOUNT  < 1 )?   3 :
((MAX_PRCOUNT  > MAX_RFD )? 3 : MAX_PRCOUNT );

struct ifstats	eeEifstats[ EEE_CNTLS ];
major_t 	eeE_majors[ EEE_MAJORS ] =
{
	EEE_CMAJOR_0,		/* Major number			*/
};

/* The Transmit Area */
uint_t		eeE_max_tcb	 = MAX_TCB;
uint_t		eeE_max_tbd	 = MAX_TBD;
uint_t		eeE_tbd_per_tcb = MAX_TBD_PER_TCB;

/* The Receive Area */
uint_t		eeE_max_rfd = MAX_RFD;

/* Congestion enable flag for National Phy */
uint_t eeE_cong_enbl = TX_CONG_DFLT;

#ifdef IP
int	eeEinetstats = 1;
#else
int	eeEinetstats = 0;
#endif


/*
 * Set the line speed and duplex mode of the controller.
 *  0 indicates autodetection for both speed and duplex mode
 *  1 indicates a speed of 10MBS and a duplex mode of half
 *  2 indicates a speed of 10MBS and a duplex mode of full
 *  3 indicates a speed of 100MBS and a duplex mode of half
 *  4 indicates a speed of 100MBS and a duplex mode of full
 */
int eeE_speed_duplex = 0;         

/* 
 * Auto-polarity enable/disable
 * eeE_autopolarity = 0 => disable auto-polarity
 * eeE_autopolarity = 1 => enable auto-polarity
 * eeE_autopolarity = 2 => let software determine
 */
int eeE_autopolarity = 2;
