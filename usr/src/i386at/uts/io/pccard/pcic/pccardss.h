#ident	"@(#)kern-i386at:io/pccard/pcic/pccardss.h	1.1"


/* 
 *   	pccardss.h
 *   	This file contains the structure definitions, error codes and #define's
 *	for the SocketServices portion of the pcic driver.
 * 
 */
#ifndef PCCARDSS

#define PCCARDSS

#define MAXPCCARDADAPTERS 4
#define MAXPCCARDSOCKETS (MAXPCCARDADAPTERS * 4)

#define PCCARDMAXNAME 32



/*  
 * This structure is used to in the set_state socket services call 
 */
typedef struct {
	ushort_t pwrinfo;		/* Power attributes for the socket */
#define POWERENA      0x0001
#define AUTOPOWER     0x0002
#define OUTPUTENA     0x0004
	uchar_t  flags;			
#define SOCKET_MEMIO 0x1		/* Is the card in the socket memory, or memory and I/O */
} ss_socket_state_t;


/* 
 * Pragma pack(1) is used for force alignment on byte boundaries 
 */
#pragma pack(1)

/*  
 * cardstate_t is the part of the data to be returned by the 
 * CardServices GetStatus call.   This structure is here as a 
 * place holder for future versions of this software, and is currently
 * unused.   It contains information related to the state of the card
 * in the socket
 */
typedef struct {
	unsigned WriteProtected:1;		/* Card is write protected */
	unsigned CardLocked:1;			/* Card is locked in place */
	unsigned EjectionRequest:1;		/* Ejection Request pending */
	unsigned InsertionRequest:1;		/* Insertion pending */
	unsigned Bvd1:1;			
	unsigned Bvd2:1;
	unsigned Ready:1;			/* Card is ready */
	unsigned CardDetected:1;		/* Card is detected */
	unsigned ReqAttn:1;			/* Card requires attention */
	unsigned RsvdEvt1:1;			
	unsigned RsvdEvt2:1;
	unsigned RsvdEvt3:1;
	unsigned VccLevel:2;
#define SS_VCC5V   0
#define SS_VCC3V   1
#define SS_VCCRES1 2
#define SS_VCCRES2 3
	unsigned Reserved:2;
} cardstate_t;


/* 
 * ss_socketstatus_t is the part of the data to be returned by the 
 * CardServices GetStatus call.   Only the CardDetect flag is used 
 * in this version of the software.  
 */
typedef struct {
	unsigned WriteProtect:1;	
	unsigned CardLock:1;	
	unsigned EjectionRequest:1;		
	unsigned InsertionRequest:1;		
	unsigned BatteryDead:1;	
	unsigned BatteryWarning:1;	
	unsigned Ready:1;	
	unsigned CardDetect:1;	
	unsigned Reserved:8;	
} ss_socketstatus_t;

#pragma pack()   /* Turn packing off */



/* 
 * this is an internal socket services strucutre which defines
 * the i/o address windows for a card in a socket
 */
typedef struct {
	unsigned short io_baseport1;   /* I/O Window 1 base address */
	unsigned char  io_numport1;    /* I/O Window 1 size */
	unsigned char  io_attr1;       /* I/O Window 1 attributes (see IOWIN* below) */
	unsigned short io_baseport2;   /* I/O Window 2 base address */
	unsigned char  io_numport2;    /* I/O Window 2 size */
	unsigned char  io_attr2;       /* I/O Window 2 attributes (see IOWIN* below) */
	unsigned char  io_addrlines;   /* Number of address lines to interpret */
#define IOWIN_SHARED	    0x1
#define IOWIN_FIRSTSHARED   0x2
#define IOWIN_FORCEDALIAS   0x4
#define IOWIN_WIDTH16       0x8
#define IOWIN_MASK(x) (x & 0x0f)
} ss_iowin_t;

/* 
 * this is an internal socket services strucutre which defines
 * the memory address windows for a card in a socket
 */
typedef struct {
	unsigned short mem_lsocket;	/* Logical socket */
	unsigned short mem_attributes;
#define MEMWIN_ADDRSPACEIO   0x0001	/* CardBus memory window to be for I/O purposes */
#define MEMWIN_ATTRIBUTE     0x0002	/* Window is in attribute memory */
#define MEMWIN_ENABLED	     0x0004	/* Enable the window */
#define MEMWIN_WIDTH16	     0x0008	/* There will be 16 bit accesses */
#define MEMWIN_WRTPROT	     0x0010	/* Write protect the window */
#define MEMWIN_ZEROWS	     0x0020	/* No wait states */
	unsigned int mem_ibase;  	/* Offset from base in CIS */
	paddr_t      mem_pbase;  	/* physaddr where mem request lands */
	unsigned int mem_size;		/* Size of the window */
	unsigned int mem_speed;  	/* In nanoseconds */
	unsigned int winnum;		/* Which window to map  (max = MAXMEMWIN) */
} ss_memwin_t;


/* 
 * Socket's irq information
 */
typedef struct {
	int irq;	/* The IRQ to be set to a PC Card in socket */
	uint_t flags;   /* Currently unused */
} ss_irq_t;


#define SS_GETSTATUS   0
#define SS_SETIOWIN    1
#define SS_SETMEMWIN   2
#define SS_SETIRQ      3
#ifdef CS_EVENT
#define SS_SETCALLBACK 4
#endif



typedef struct {
	int pcca_boardnum;			/* Board number of type name */
        char pcca_name[PCCARDMAXNAME];  	/* Name of adapter */
        int  pcca_numsockets;			/* Number of sockets adapter has */
	struct socket_services *pcca_services;	/* Pointer to socket services */
	int  pcca_num_memwin;			/* Number of memory windows per socket */
	int  pcca_num_iowin;			/* Number of I/O windows per socket */
} pccard_adapter_t;


/* Implemented socket services functions */
struct socket_services {
	int (*get_status)(int adapter, int socket , ss_socketstatus_t *s);
	int (*set_iowin)(int adapter, int socket, ss_iowin_t *io);
	int (*set_memwin)(int adapter, int socket, int win, ss_memwin_t *mem);
	int (*set_irqinfo)(int adapter, int socket, ss_irq_t *irq);
	int (*get_state)(int adapter, int socket, ss_socket_state_t *s);
	int (*set_state)(int adapter, int socket, ss_socket_state_t *s);
#ifdef CS_EVENT
	int (*set_callback)(int adapter, int socket, int);
#endif
};


typedef struct {
	pccard_adapter_t  *adapterp;   /* Pointer to adapter of this socket */
	int     realsock;  	       /* Real socket on adapter   	    */
	ss_socketstatus_t   old;       /* old status */
	ss_socketstatus_t   new;       /* new status */
#ifdef CSEVENT
	event_t		events;	       /* Events pending */
#endif
} pccard_socket_t;

#endif
