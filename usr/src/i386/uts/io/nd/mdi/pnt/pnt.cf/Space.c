#ident	"@(#)Space.c	2.1"
#ident  "$Header$"

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/dlpi_ether.h>
#include <config.h>


#define NSAPS           8
#define MAXMULTI        8
#define INETSTATS       1
#define STREAMS_LOG     0
#define IFNAME          "pnt"

#define PNT_TX_BUF      16
#define PNT_RX_BUF      16

int pnt_MaxStreams      = NSAPS;
int pntstrlog           = STREAMS_LOG;
char *pnt_ifname        = IFNAME;
int pnt_nsaps           = NSAPS;
int pnt_cmajors         = PNT_CMAJORS;  /* # of possible major #'s */
int pnt_cmajor_0        = PNT_CMAJOR_0; /* first major # in range */
short pnt_tx_buf        = PNT_TX_BUF;
short pnt_rx_buf        = PNT_RX_BUF;

typedef struct pnt_ConfigStruct {
   short index;                     /* board index */
   short minors;                    /* minor devices configured */
   short vec;                       /* interrupt vector # */
   short iobase;                    /* boards base memory address */
   short ioend;                     /* Ending base I/O address */
   short dma;                       /* DMA channel used by the board */
   short tx_buffers;                   /* Buffer size for Transmit */
   short rx_buffers;                   /* Buffer size for Receive */
   long  led0;                      /* LED value */
   long  led1;                      /* LED value */
   long  led2;                      /* LED value */
   long  led3;                      /* LED value */
   long  dmarotate;
   long  tp;
   long  fdup;
   long  pcnet2;               /* if set to 1, use advanced P2, etc. features */
   long pci_bus;                    /* bus, device and function found */
   long pci_dev;
   long pci_func;
   long access_type;                /* access type */
   long diag;                      /* diag level for this board */
   long bustimer;                /* DMA bus access timer */
   long notxint;                /* no end-of-transmit interrupt */
}pnt_ConfigStruct_t;

struct pnt_ConfigStruct pnt_ConfigArray[4] = {
    {
        0,      /* index */
        NSAPS,  /* minors */
        0,      /* vec  */
        0,      /* iobase */
        0,      /* ioend */
        -1,     /* dma */
        PNT_TX_BUF,
        PNT_RX_BUF,
        0,      /* led 0 */
        0,      /* led 1 */
        0,      /* led 2 */
        0,      /* led 3 */
        0,      /* dmarotate */
        0,      /* TP */
        0,      /* full duplex */
				0,      /* pcnet2 */
        0,      /* pci bus */
        0,      /* pci device */
        0,      /* pci function */
				0,      /* access type  */
				0,      /* diag flag */
        6,      /* bus timer access */
        0       /* no end-of-transmit interrupt */
    }
    ,
    {
        1,      /* index */
        NSAPS,  /* minors */
        0,      /* vec  */
        0,      /* iobase */
        0,      /* ioend */
        -1,     /* dma */
        PNT_TX_BUF,
        PNT_RX_BUF,
        0,      /* led 0 */
        0,      /* led 1 */
        0,      /* led 2 */
        0,      /* led 3 */
        0,      /* dmarotate */
        0,      /* TP */
        0,      /* full duplex */
				0,      /* pcnet2 */
        0,      /* pci bus */
        0,      /* pci device */
        0,      /* pci function */
				0,      /* access type  */
				0,      /* diag flag */
        6,      /* bus timer access */
        0       /* no end-of-transmit interrupt */
    }
    ,
    {
        2,      /* index */
        NSAPS,  /* minors */
        0,      /* vec  */
        0,      /* iobase */
        0,      /* ioend */
        -1,     /* dma */
        PNT_TX_BUF,
        PNT_RX_BUF,
        0,      /* led 0 */
        0,      /* led 1 */
        0,      /* led 2 */
        0,      /* led 3 */
        0,      /* dmarotate */
        0,      /* TP */
        0,      /* full duplex */
				0,      /* pcnet2 */
        0,      /* pci bus */
        0,      /* pci device */
        0,      /* pci function */
				0,      /* access type  */
				0,      /* diag flag */
        6,      /* bus timer access */
        0       /* no end-of-transmit interrupt */
    }
    ,
    {
        3,      /* index */
        NSAPS,  /* minors */
        0,      /* vec  */
        0,      /* iobase */
        0,      /* ioend */
        -1,     /* dma */
        PNT_TX_BUF,
        PNT_RX_BUF,
        0,      /* led 0 */
        0,      /* led 1 */
        0,      /* led 2 */
        0,      /* led 3 */
        0,      /* dmarotate */
        0,      /* TP */
        0,      /* full duplex */
				0,      /* pcnet2 */
        0,      /* pci bus */
        0,      /* pci device */
        0,      /* pci function */
				0,      /* access type  */
				0,      /* diag flag */
        6,      /* bus timer access */
        0       /* no end-of-transmit interrupt */
    }
};


