#ident	"@(#)kern-i386at:io/pccard/pcic/pccardcs.h	1.2"

/* 
 * 	pccardcs.h 
 * 
 *	This header file is the repository for CardServices structure
 * 	definitions, error codes, and #defines.
 * 
 */

#ifndef PCCARDCS
#define PCCARDCS

#include "pccardss.h"

/* Card Services Function definitions */
#define DeregisterClient 	0x02
#define RegisterClient 		0x10
#define GetStatus 		0x0c
#define ResetCard		0x11 
#define RequestIRQ 		0x20
#define RequestIO 		0x1f
#define RequestWindow 		0x21
#define GetFirstTuple 		0x07
#define GetNextTuple 		0x0a
#define GetTupleData 		0x0d
#define RequestConfiguration 	0x30
#define MapMemPage      	0x14


/* 
 * The following definitions are of a strict layout and need to
 * be a particular size, the "pragma pack(1)" will ensure packing 
 * on byte boundaries.
 */

#pragma pack(1)


/* 
 * This structure is what is used in the CardServices GetStatus service code. 
 */
struct CSStatus {
	ushort LogicalSocket;			/* Logical socket to get status for */
	cardstate_t CardState;			/* State of the card in the socket */
	ss_socketstatus_t SocketState;		/* State of the socket */
};


/* 
 * This structure is what is used by the CardServices RequestIRQ service code  
 * Note:  In this release, CardServices expects the IRQ passed in is 
 * is available and valid.  THE FIRST IRQ REQUESTED VIA RequestIRQ WILL BE GRANTED.
 */
struct CSIrq {
	ushort LogicalSocket;			/* Logical socket to set IRQ for */
	ushort Attributes;		/* Attributes defined below */
#define CS_IRQATTR_EXCLUSIVE     0x0000 /* IRQ cannot be shared */
#define CS_IRQATTR_TIMESHARE     0x0001 /* IRQ can be shared */
#define CS_IRQATTR_DYNAMICSHARE  0x0002 
#define CS_IRQATTR_FORCEPULSE	 0x0004 
#define CS_IRQATTR_FIRSTSHARED	 0x0008 
#define CS_IRQATTR_PULSEALLOC	 0x0100
	char   AssignedIRQ;		/* Assigned IRQ by CardServices  */
	char   IRQInfo1;		
#define CS_IRQINFO_NMI		0x01	
#define CS_IRQINFO_IOCK		0x02
#define CS_IRQINFO_BERR		0x04
#define CS_IRQINFO_VEND		0x08
#define CS_IRQINFO_MASK		0x10    /* If set, irq is in first 4 bytes 0-3 else in IRQInfo2 */
#define CS_IRQINFO_LEVEL	0x20
#define CS_IRQINFO_PULSE	0x40
#define CS_IRQINFO_SHARE	0x80
	short  IRQInfo2;		/* If Bitmap of IRQ */
};


/* 
 * This structure is what is passed to the CardServices call RequestConfiguration 
 */
struct CSConfig {
	ushort LogicalSocket;			/* logical socket to be configured */
	ushort Attributes;		
#define CS_CONFIG_ENIRQ      0x0002	/* Enable IRQ */
#define CS_CONFIG_ENDMA      0x0040	/* Enable DMA */
#define CS_CONFIG_VSOVERRIDE 0x0200	/* Voltage override */
	uchar_t	Vcc;
	uchar_t	Vpp1;
	uchar_t	Vpp2;
	uchar_t IntType;
#define CS_CONFIG_TYPE_MEM	0x01	/* Card is type memory only */
#define CS_CONFIG_TYPE_MEMIO	0x02	/* Card is type memory and I/O only */
#define CS_CONFIG_CARDBUS	0x04	/* Card is type CardBus */
	ulong_t ConfigBase;		/* Base address of Card's COR in Attr memory */
	uchar_t Status;			/* Value to be written to Cards CSR */
	uchar_t Pin;			/* Value to be written to Cards CPR */
	uchar_t Copy;			/* Value to be written to Cards CCR */
	uchar_t ConfigIndex;		/* Value to be written to Cards COR */
	uchar_t Present;		/* Flags determine what registers to write to */
#define CS_CONFIG_OPTPRESENT    0x01	/* Write to the COR */
#define CS_CONFIG_STATPRESENT   0x02	/* Write to the CSR */
#define CS_CONFIG_PINPRESENT    0x04	/* Write to the CPR */
#define CS_CONFIG_COPYPRESENT   0x08	/* Write to the CCR */
#define CS_CONFIG_ESPRESENT     0x10	/* Write to the CESR */
	uchar_t ExtendedStatus;		/* Extended status register */
};


/* This structure is what is passed to the CardServices call RequestIO 
 * Note:  In this release, CardServices expects that the I/O range passed in
 * is already known to be available (not assigned anywhere else).   THE FIRST
 * I/O range REQUESTED VIA RequestIO WILL BE GRANTED.
 */
struct CSIo {
	ushort LogicalSocket;
	ushort Base1;		/* Base address 1 */
	uchar_t NumPorts1;	/* Number of ports in range 1 */
	uchar_t Attributes1;	
#define CS_IOSHARED 		0x01
#define CS_IOFIRST_SHARED 	0x02
#define CS_IOFORCED_ALIAS 	0x04
#define CS_IOPATH16	 	0x08
	ushort Base2;		/* Base address 2 */
	uchar_t NumPorts2;	/* Number of ports in range 2 */
	uchar_t Attributes2;  	/* See CS_IO* above */
	uchar_t IOAddrLines;	
};



/* This structure is what is passed to the CardServices call RequestWindow 
 * Note:  In this release, CardServices expects that the memory range passed in
 * is already known to be available (not assigned anywhere else).   THE FIRST
 * memory range REQUESTED VIA RequestWindow WILL BE GRANTED.
 */
struct CSMem {
	ushort LogicalSocket;	/* Logical socket for requested memory window */
	ushort Attributes;	
#define CS_MEMIO		0x01   /* CardBus I/O window */
#define CS_MEMATTRIBUTE		0x02   /* Request for attribute  memory */
#define CS_MEMENABLED  		0x04   /* Enable the memory ... or disable it if not set */
#define CS_MEMSIZE16  		0x08   /* 16 bit data path, if not set, 8 bit */
	ulong_t Base;		/* Card's Base address to map */
	ulong_t Size;		/* Size of window to map */
	union {
		uchar_t AccessSpeed; /* Memory Speed or... */
		uchar_t IOAddrLines; /* ...num ioaddr lines to interpret */
	}u;
};


/*  
 *  This is the CSTuple structure definition for use with CardServices 
 *  GetFirstTuple, and GetNextTuple service codes.  
 */
struct CSTuple  {
	ushort    LogicalSocket;	/* Logical Socket */
	ushort    Attribute;	/* =1 to return link tuples */
#define CS_TATTR_RETURNLINK 0x01
	uchar_t   DesiredTuple; /* Desired Tuple, 0xff if first tuple */
	uchar_t   Reserved;
	ushort    Flags;	/* Internal use only */
#define CS_TFLAGS_ATTRIBUTE 0x0001 /* CS_TFLAGS_COMMON is zero */ 
#define CS_TFLAGS_LLCOMMON  0x0002 /* The long link was to common memory */
#define CS_TFLAGS_LONGLINK  0x0004 /* LongLink encountered in chain */
#define CS_TFLAGS_ISLINK    0x0008 /* LastTuple read was a link tuple */
#define CS_TFLAGS_NOLINKS   0x0010 /* No links */
#define CS_TFLAGS_EXPECTLT  0x0020 /* Expect a Link Target */
#define CS_TFLAGS_NOMORE    0x0040 /* No more Tuples left */
#define CS_TFLAGS_PRIMARY   0x4000 /* Process primary tuple chain */
#define CS_TFLAGS_USED      0x8000 /* Set after first read */
	ulong_t   LinkOffset;	/* Internal use only */
	ulong_t   CISOffset;	/* Internal use only */
	uchar_t   TupleCode;	/* Tuple found */
	uchar_t   TupleLink;	/* Link value for Tuple found */
};


/*  
 *  This is the CSTuple structure definition for use with CardServices 
 *  GetTupleData service code.
 */
struct CSTupleData  {
	ushort    LogicalSocket;	/* Logical Socket */
	ushort    Attribute;	/* =1 to return link tuples */
	uchar_t   DesiredTuple; /* Desired Tuple, 0xff if first tuple */
	uchar_t   TupleOffset;  /* Offset into tuple to start transfer of TupleDataLen bytes to TupleData */
	ushort    Flags;	/* Internal use only (SEE ABOVE)*/
	ulong_t   LinkOffset;	/* Internal use only (SEE ABOVE)*/
	ulong_t   CISOffset;	/* Internal use only (SEE ABOVE)*/
	ushort_t  TupleDataMax; /* Maximum size of tuple data area */
	ushort_t  TupleDataLen; /* Number of bytes in tuple body */
	uchar_t   TupleData[1];	/* Location where data will be */
};

struct CSMemPage {
	ulong_t	  CardOffset;   /* Offset in Card memory to map in */
	uchar_t   Page;		/* Page number for the window (currently not used) */
};

#pragma pack()        /* Turn off structure packing */


/*  Error codes */
#define CS_UNSUPPORTED_SERVICE		0x15  /* Implementation does not support service */
#define CS_BAD_ADAPTER			0x01  /* Specified adapter s invalid */
#define CS_BAD_ARG_LENGTH		0x1b  /* ArgLength argument is invalid */
#define CS_BAD_ARGS			0x1c  /* Values in Argument package are invalid */
#define CS_BAD_SOCKET			0x0b  /* Specified socket is invalid (logical or physical) */
#define CS_BAD_HANDLE			0x21  /* ClientHandle is invalid */ 
#define CS_SUCCESS			0x00  /* The request succeeded */
#define CS_NO_MORE_ITEMS		0x1f  /* There are no more of the requested item */
#define CS_BAD_IRQ			0x06  /* Specified IRQ level is invalid */
#define CS_OUT_OF_RESOURCE 		0x20  /* CardServices has exhausted resource */
#define CS_BAD_SIZE			0x0a  /* Bad window size requested */
#define CS_BAD_OFFSET			0x07  /* Specified memory offset is invalid */
#define CS_BAD_BASE			0x03  /* Specified physical base address is invalid */


#define MAXMEMWIN       5	   /* Max number of memory windows any socket can have (ExCA spcified) */
#define MAXIOWIN        2	   /* Max number of I/O address windows any socket can have  (ExCA spcified) */
#define MAXTUPLESIZE    257	   /* Maximum size of any tuple  */

#define MAXTUPLENOLINK  25	   /* Max number of allowable to be parsed
				    * without hitting a no-link tuple 
				    */


/* 
 * This handle struture contains state information about any particular
 * CardServices client.   RegisterClient initializes the handle, 
 * RequestIO, RequestIRQ, RequestWindow calls will * populate most of this struct. 
 * Tuple Data is also stored here.
 */
typedef struct {
	uchar_t h_registered;           /* Is this handle active */
#define CS_HANDLE_ACTIVE      0x01	/* Handle in use */
#define CS_HANDLE_TUPLES_READ 0x02	/* If set, tuples have been read, otherwise it is the call */
#define CS_HANDLE_REQIRQ      0x04	/* IRQ has been requested */
#define CS_HANDLE_REQIO       0x08	/* IO window has been requested */
#define CS_HANDLE_REQMEM      0x10	/* Mem windows has been requested */
	uchar_t h_client;	        /* Client registered???  */
	uchar_t h_num_memwin;           /* Num mem windows allocated so far */
	ss_memwin_t h_mem[MAXMEMWIN];  	/* array for memory windows */
	ss_iowin_t h_io;	        /* io windows */
	ss_irq_t h_irq;	                /* Requested IRQ  */
	int h_tuplesize; 		/* Tuple data size */
	char h_tupledata[MAXTUPLESIZE]; /* Tuple data */
} handle_t;


/* 
 *  The window handle is an argument to the CardServices RequestWindow service code.
 *  win_handle_t is opaque to the caller.  We choose to store the index into the handle of 
 *  the clients h_mem array in the handle.  Thus, if the client needs to talk about an 
 *  particular window with CardServices, there is a mapping to the window's information in the handle.
 */
typedef struct {
	ushort_t   lsocket;    /* Socket requesting window */
	ushort_t   winnumber;  /* Window index for the socket (actually, its the h_mem index value) */
	handle_t   *handle;    /* Pointer to the Client's handle */
}win_handle_t;	/* Window handle for Client */

extern int CardServices( uchar_t Service, handle_t *handle, void *Pointer, uint_t ArgLength, void *ArgPointer);

/*   
 * TupleCode DEFINITIONS -  Please reference the CardServices Specification 
 * for descriptions of these  Tuple codes
 */
#define CISTPL_NULL		0x00
#define CISTPL_DEVICE		0x01
#define CISTPL_CHECKSUM		0x10
#define CISTPL_LONGLINK_A	0x11
#define CISTPL_LONGLINK_C	0x12
#define CISTPL_LINKTARGET	0x13
#define CISTPL_NO_LINK		0x14
#define CISTPL_VERS_1		0x15
#define CISTPL_ALTSTR		0x16
#define CISTPL_DEVICE_A		0x17
#define CISTPL_JEDEC_C		0x18
#define CISTPL_JEDEC_A		0x19
#define CISTPL_CONFIG		0x1a
#define CISTPL_CFTABLE_ENTRY	0x1b
#define CISTPL_DEVICE_OC	0x1c
#define CISTPL_DEVICE_OA	0x1d
#define CISTPL_DEVICE_GEO	0x1e
#define CISTPL_DEVICE_GEO_A	0x1f
#define CISTPL_MANFID		0x20
#define CISTPL_FUNCID		0x21
#define CISTPL_FUNCE		0x22
#define CISTPL_SWIL		0x23
#define CISTPL_VERS_2		0x40
#define CISTPL_FORMAT		0x41
#define CISTPL_GEOMETRY		0x42
#define CISTPL_BYTEORDER	0x43
#define CISTPL_DATE		0x44
#define CISTPL_BATTERY		0x45
#define CISTPL_ORG		0x46
#define CISTPL_END		0xFF

/*    Card Types   */
#define TPLFID_MULTIFUN		0x00   /* Multifunction card */
#define TPLFID_MEMORY  		0x01   /* Memory card */
#define TPLFID_SERIAL		0x02   /* Fax/Modem card, or just serial port */
#define TPLFID_PARALLEL		0x03   /* Parallel port */
#define TPLFID_FIXEDDISK	0x04   /* ATA (IDE) hard drive */
#define TPLFID_VIDEO	 	0x05   /* Video Adapter */
#define TPLFID_NETWORK		0x06   /* Network Adapter */
#define TPLFID_AIMS		0x07   /* Auto Indexing Mass Storage */
#define TPLFID_SCSI		0x08   /* SCSI Adapter */

#define SERIALDRIVERNAME "asyc"	  /* For use in the modem enabler when searching the resmgr database */
#define MINSERIALPCCARDUNIT  20
#define CS_STATUS_MODEMAUDIO 0x08

#define CISSPEED 500
#define CISWINSIZE 4096

#define CSHEIR 1



/*  For parsing the TPCE_INDX: Configuration table index byte */
#define TPCE_INDX_INTERFACE 	0x80
#define TPCE_INDX_DEFAULT 	0x40
#define TPCE_INDX_ENTRY_MASK 	0x3f
#define TPCE_INDX_HAS_INTERFACE( x ) (x & TPCE_INDX_INTERFACE)
#define TPCE_INDX_ISDEFAULT( x ) (x & TPCE_INDX_DEFAULT)

/*  For parsing the TPCE_IF: Interface Description Field */
#define TPCE_IF_MWAITREQ 	0x80  /* WAIT signal supported on mem cycles */
#define TPCE_IF_READYACTIVE 	0x40  /* READY signal supported */
#define TPCE_IF_WPACTIVE 	0x20  /* Write protect is supported */
#define TPCE_IF_BVDACTIVE	0x10  /* Battery Voltage detects are supported */
#define TPCE_IF_ITYPEMASK	0x0f  /* Interface type of the card */
#define TPCE_IF_ITYPEMEM 	0x00  /* Memory card */
#define TPCE_IF_ITYPEMEMIO 	0x01  /* Memory and IO card */
#define TPCE_IF_ITYPERES1   	0x02  /* Reserved */
#define TPCE_IF_ITYPERES2   	0x03  /* Reserved */
#define TPCE_IF_ITYPECUSTOM1    0x04  /* Custom type card */
#define TPCE_IF_ITYPECUSTOM2    0x05  /* Custom type card */
#define TPCE_IF_ITYPECUSTOM3    0x06  /* Custom type card */
#define TPCE_IF_ITYPECUSTOM4    0x07  /* Custom type card */

/* For parsing the TPCE_FS: Feature selection byte */
#define TPCE_FS_MISC		0x80  /* A misc field is present */
#define TPCE_FS_MEMMASK		0x60  /* Mem field mask */
#define TPCE_FS_IRQ		0x10  /* IRQ description fields are present */
#define TPCE_FS_IOSPACE		0x08  /* I/O Space field is present */
#define TPCE_FS_TIMING 		0x04  /* Timing field is present */
#define TPCE_FS_PWRMASK		0x03  /* Power field mask */

#define TPCE_MEMNONE		0x00  /* No memory fields */
#define TPCE_MEMSIMPLE		0x01  /* Single two byte length specified */
#define TPCE_MEMLENADDR		0x02  /* Length (2) and Address (2) specified */
#define TPCE_MEMDESCRIPTOR	0x03  /* Memory descriptor byte follows */

#define TPCE_POWERNONE		0x00  /* No power fields */
#define TPCE_POWERVCC 		0x01  /* Vcc description field */
#define TPCE_POWERVCCVPP_1	0x02  /* Vcc and Vpp[2::1] (Vpp1 == Vpp2) present */
#define TPCE_POWERVCCVPP_2	0x03  /* Vcc, Vpp1, and Vpp2 fields present */


/* 
 * TPCE_PD:  Power descriptor structure
 * This is a true bear to parse, and since we are not using any
 * of that information, here are some macros to walk the power
 * strucutres
 */
#define TPCE_NUMPOWER( x )  (x & TPCE_FS_PWRMASK)  /* Pass in the TPCE_FS */
/* Pass in a parameter selection byte into TPCE_CNTPARAM  ... count the bits */
#define TPCE_CNTPARAM(x) (((x >> 7) & 1)  + \
			((x >> 6) & 1)  + \
			((x >> 5) & 1)  + \
			((x >> 4) & 1)  + \
			((x >> 3) & 1)  + \
			((x >> 2) & 1)  + \
			((x >> 1) & 1)  + \
			(x & 1)  )   

#define TPCE_POWEREXT 0x80 

/* TPCE_TD:  Configuration Timing information (NOT USED) */
#define TPCE_TD_WAITMASK 	0x03  /* power 10 scale applied to MAXWAIT */
#define TPCE_TD_WAITNOTUSED(w) ( ( (w & TPCE_TD_WAITMASK) != 0x03) ? 1 : 0)
#define TPCE_TD_READYMASK 	0x1C  /* power */
#define TPCE_TD_READYNOTUSED(r) ( ( ((r & TPCE_TD_READYMASK) >> 2) != 0x07) ? 1 : 0)

/* TPCE_IO: I/O Space Addresses Required for this configuration */

#define TPCE_IO_RANGE 		0x80  /* If set, I/O range description follows */
#define TPCE_IO_BUSMASK		0x60  /* Mask to read bus capabilities */
#define TPCE_IO_IOLINESMASK	0x1f  /* Mask to read address decode information */

#define TPCE_IO_BUSRESERVED	0x00
#define TPCE_IO_BUS8BIT    	0x20  /* Supports 8bit io cycles */
#define TPCE_IO_BUS16BIT    	0x40  /* Supports 16bit io cycles */
#define TPCE_IO_BUS8_16BIT    	0x60  /* Supports 8 and 16 bit io cycles */

#define TPCE_IO_RANGE_SIZLENMASK	0xC0	/* Mask to read the range size */
#define TPCE_IO_RANGE_SIZADDRMASK	0x30    /* Mask to read address size */
#define TPCE_IO_RANGE_NUMRANGEMASK	0x0f	/* Mask to read number of ranges */

#define TPCE_IO_HASRANGE(x) (x & TPCE_IO_RANGE)
#define TPCE_IO_NUMRANGE(x) ( (x & TPCE_IO_RANGE_NUMRANGEMASK) + 1)
#define TPCE_IO_ADDRESS_SIZE(x) ((x & TPCE_IO_RANGE_SIZADDRMASK) >> 4)
#define TPCE_IO_LENGTH_SIZE(x) ((x & TPCE_IO_RANGE_SIZLENMASK) >> 6)

/* TPCE_IR:  Interrupt Request Description structure */
#define TPCE_IR_SHARE		0x80	/* card has interrupt sharing logic */
#define TPCE_IR_PULSE		0x40 	/* card support pulse interrupts */
#define TPCE_IR_LEVEL		0x20 	/* Card can do level interrupts */
#define TPCE_IR_MASK 		0x10 	/* if set, 4lsb of this byte are the IRQ, otherwise... 
					   VEND, BERR, IOCK, and NMI are valid, and the 
					   following two bytes are a mask of the IRQ
					*/
#define TPCE_IR_MASKVAL		0x0f	/* Value to mask with */

#define TPCE_IR_VEND		0x08	/* Vendor specific signal supported */
#define TPCE_IR_BERR		0x04	/* Bus Error signal supported */
#define TPCE_IR_IOCK		0x02	/* IO check signal supported */
#define TPCE_IR_NMI		0x01	/* NMI signal supported */

/* TPCE_MS:   Memory Space description structure */

#define TPCE_MS_HOSTADDR	0x80	 /* Host address field is present */
#define TPCE_MS_CARD_ADDRMASK	0x60	 /* Card address mask */
#define TPCE_MS_CARD_ADDRSIZE(x) ( (x & TPCE_MS_CARD_ADDRMASK) >> 5)   /* In 256b pages */
#define TPCE_MS_LENTH_MASK	0x18
#define TPCE_MS_CARD_ADDRLEN(x) ( (x & TPCE_MS_LENTH_MASK) >> 3)  	  /* In 256b pages */
#define TPCE_MS_NUMWINMASK	0x7
#define TPCE_MS_NUMWINDOWS(x) ((x & TPCE_MS_NUMWINMASK) + 1)        /* Actual # of windows */


#endif
