#ident	"@(#)kern-i386at:io/pccard/pcic/pcic.h	1.1"
/*
 *
 *	pcic.h
 *
 *	#defines and structure definitions specific to the Intel 82365SL PCIC Chip
 */

#ifndef PCIC
#define PCIC

#define	PCIC_SOCKET_SIZE	0x40		/* size of each socket */
#define	PCIC_SOCKETS		0x01		/* 2 sockets/chip */

#define	PCIC_INDEX		0x00		/* index register */
#define	PCIC_DATA		0x01		/* data register */

#define	PCIC_IDREV		0x00		/* id/revision */
#define	PCIC_STATUS		0x01		/* interface status */
#define	PCIC_POWER		0x02		/* power and RESETDRV control */
#define	PCIC_INTR		0x03		/* interrupt and general ctrl */
#define	PCIC_STSCHNG		0x04		/* card status change */
#define	PCIC_STSCHNG_INTR	0x05		/* card status change int cfg */
#define	PCIC_WINENABLE		0x06		/* address window enable */
#define	PCIC_IOCTRL		0x07		/* io control */
#define	PCIC_IO0		0x08		/* io 0 start */
#define	PCIC_IO0STRTLB		0x08		/* io 0 start low byte */
#define	PCIC_IO0STRTHB		0x09		/* io 0 start high byte */
#define	PCIC_IO0STOPLB		0x0a		/* io 0 stop low byte */
#define	PCIC_IO0STOPHB		0x0b		/* io 0 stop high byte */
#define	PCIC_IO1		0x0c		/* io 1 start */
#define	PCIC_IO1STRTLB		0x0c		/* io 1 start low byte */
#define	PCIC_IO1STRTHB		0x0d		/* io 1 start high byte */
#define	PCIC_IO1STOPLB		0x0e		/* io 1 stop low byte */
#define	PCIC_IO1STOPHB		0x0f		/* io 1 stop high byte */
#define	PCIC_MEM0		0x10		/* memory 0 start */
#define	PCIC_MEM0STRTLB		0x10		/* mem 0 start low byte */
#define	PCIC_MEM0STRTFB		0x11		/* mem 0 start high byte */
#define	PCIC_MEM0STOPLB		0x12		/* mem 0 stop low byte */
#define	PCIC_MEM0STOPHB		0x13		/* mem 0 stop high byte */
#define	PCIC_MEM0OFFSETLB	0x14		/* mem 0 card offset low byte */
#define	PCIC_MEM0OFFSETHB	0x15		/* mem 0 card offset high byte */
#define	PCIC_CD_GCR		0x16		/* reserved */
#define	PCIC_RESERVED01		0x17		/* reserved */
#define	PCIC_MEM1		0x18		/* memory 1 start */
#define	PCIC_MEM1STRTLB		0x18		/* mem 1 start low byte */
#define	PCIC_MEM1STRTFB		0x19		/* mem 1 start high byte */
#define	PCIC_MEM1STOPLB		0x1a		/* mem 1 stop low byte */
#define	PCIC_MEM1STOPHB		0x1b		/* mem 1 stop high byte */
#define	PCIC_MEM1OFFSETLB	0x1c		/* mem 1 card offset low byte */
#define	PCIC_MEM1OFFSETHB	0x1d		/* mem 1 card offset high byte */
#define	PCIC_GCR		0x1e		/* reserved */
#define	PCIC_RESERVED11		0x1f		/* reserved */
#define	PCIC_MEM2		0x20		/* memory 2 start */
#define	PCIC_MEM2STRTLB		0x20		/* mem 2 start low byte */
#define	PCIC_MEM2STRTFB		0x21		/* mem 2 start high byte */
#define	PCIC_MEM2STOPLB		0x22		/* mem 2 stop low byte */
#define	PCIC_MEM2STOPHB		0x23		/* mem 2 stop high byte */
#define	PCIC_MEM2OFFSETLB	0x24		/* mem 2 card offset low byte */
#define	PCIC_MEM2OFFSETHB	0x25		/* mem 2 card offset high byte */
#define	PCIC_RESERVED20		0x26		/* reserved */
#define	PCIC_RESERVED21		0x27		/* reserved */
#define	PCIC_MEM3		0x28		/* memory 3 start */
#define	PCIC_MEM3STRTLB		0x28		/* mem 3 start low byte */
#define	PCIC_MEM3STRTFB		0x29		/* mem 3 start high byte */
#define	PCIC_MEM3STOPLB		0x2a		/* mem 3 stop low byte */
#define	PCIC_MEM3STOPHB		0x2b		/* mem 3 stop high byte */
#define	PCIC_MEM3OFFSETLB	0x2c		/* mem 3 card offset low byte */
#define	PCIC_MEM3OFFSETHB	0x2d		/* mem 3 card offset high byte */
#define	PCIC_RESERVED30		0x2e		/* reserved */
#define	PCIC_RESERVED31		0x2f		/* reserved */
#define	PCIC_MEM4		0x30		/* memory 4 start */
#define	PCIC_MEM4STRTLB		0x30		/* mem 4 start low byte */
#define	PCIC_MEM4STRTFB		0x31		/* mem 4 start high byte */
#define	PCIC_MEM4STOPLB		0x32		/* mem 4 stop low byte */
#define	PCIC_MEM4STOPHB		0x33		/* mem 4 stop high byte */
#define	PCIC_MEM4OFFSETLB	0x34		/* mem 4 card offset low byte */
#define	PCIC_MEM4OFFSETHB	0x35		/* mem 4 card offset high byte */
#define	PCIC_RESERVED40		0x36		/* reserved */
#define	PCIC_RESERVED41		0x37		/* reserved */
#define	PCIC_RESERVED42		0x38		/* reserved */
#define	PCIC_RESERVED43		0x39		/* reserved */
#define	PCIC_RESERVED44		0x3a		/* reserved */
#define	PCIC_RESERVED45		0x3b		/* reserved */
#define	PCIC_RESERVED46		0x3c		/* reserved */
#define	PCIC_RESERVED47		0x3d		/* reserved */
#define	PCIC_RESERVED48		0x3e		/* reserved */
#define	PCIC_RESERVED49		0x3f		/* reserved */

/*
 *	io offsets
 */

#define	PCIC_IOSTARTLB		0x00		/* io start low byte */
#define	PCIC_IOSTARTHB		0x01		/* io start high byte */
#define	PCIC_IOSTOPLB		0x02		/* io stop low byte */
#define	PCIC_IOSTOPHB		0x03		/* io stop high byte */

#define PCIC_STATUS_BVD1	0x01
#define PCIC_STATUS_BVD2	0x02
#define PCIC_STATUS_CD1		0x04
#define PCIC_STATUS_CD2		0x08
#define PCIC_STATUS_WP		0x10
#define PCIC_STATUS_READY	0x20
#define PCIC_STATUS_PWR		0x40
#define PCIC_STATUS_GPI		0x80

/*
 *  Card status change flags
 */ 
#define PCIC_GPICHG     0x10	
#define PCIC_DETECTCHG  0x08
#define PCIC_READYCHG   0x04
#define PCIC_BATWARN    0x02
#define PCIC_BATDEAD    0x01
#define PCIC_STSCHG     0x01

/*
 *	io "info" field bit defines
 */

#define	PCIC_IO_SIZE8		0x00		/* 8 bit datapath */
#define	PCIC_IO_SIZE16		0x01		/* 16 bit datapath */
#define	PCIC_IO_IOCS16		0x02		/* IOCS16 source */
#define	PCIC_IO_ZWS		0x04		/* zero wait state */
#define	PCIC_IO_WAIT1		0x08		/* add 1 wait state */

/*
 *	memory offsets
 */

#define	PCIC_MEMSTARTLB		0x00		/* mem start low byte */
#define	PCIC_MEMSTARTHB		0x01		/* mem start high byte */
#define	PCIC_MEMSTOPLB		0x02		/* mem stop low byte */
#define	PCIC_MEMSTOPHB		0x03		/* mem stop high byte */
#define	PCIC_MEMOFFSETLB	0x04		/* mem card offset low byte */
#define	PCIC_MEMOFFSETHB	0x05		/* mem card offset high byte */

/* 
 *   Global Control Register bitmaps 
 */
#define PCIC_GCR_POWERDOWN       0x01
#define PCIC_GCR_LEVEL_INTR_EN   0x02
#define PCIC_GCR_EX_WR_CSC       0x04
#define PCIC_GCR_IRQ14_PM_ENABLE 0x08

/*
 *	memory "info" field bit defines
 */

#define	PCIC_MEM_SIZE8		0x00		/* 8 bit datapath */
#define	PCIC_MEM_SIZE16		0x01		/* 16 bit datapath */
#define	PCIC_MEM_ZWS		0x02		/* zero wait state bit */
#define	PCIC_MEM_WAIT1		0x04		/* add 1 wait state */
#define	PCIC_MEM_WAIT2		0x08		/* add 2 wait states */
#define	PCIC_MEM_WAIT3		0x0c		/* add 3 wait states */
#define	PCIC_MEM_ATTRIBUTE	0x10		/* access attribute card mem */
#define	PCIC_MEM_WRTPROT	0x20		/* write protect the memory */

/*
 *	valid revs
 */

#define	PCIC_REVA		0x82		/* i/o and mem -- silicon rev 2 */
#define	PCIC_REVB		0x83		/* i/o and mem -- silicon rev 3 */
#define	PCIC_REVE		0x84		/* i/o and mem -- silicon rev 4 */
#define	PCIC_REVC		0x88		/* i/o and mem -- ibm clone */
#define	PCIC_REVD		0x89		/* i/o and mem -- ibm clone */
#define	PCIC_REVMASK		0xf0		/* simple check */
#define	PCIC_REVCHK		0x80		/* for generic operation */

/*
 *	Enable Register defs
 */

#define	PCIC_EN_MEM0		0x01		/* enable memory 0 */
#define	PCIC_EN_MEM1		0x02		/* enable memory 1 */
#define	PCIC_EN_MEM2		0x04		/* enable memory 2 */
#define	PCIC_EN_MEM3		0x08		/* enable memory 3 */
#define	PCIC_EN_MEM4		0x10		/* enable memory 4 */
#define	PCIC_EN_IOCS16		0x20		/* enable IOCS16 */
#define	PCIC_EN_IO0		0x40		/* enable io 0 */
#define	PCIC_EN_IO1		0x80		/* enable io 1 */

/*
 *	Interrupt defs
 */

#define	PCIC_IRQ_NONE		0x00		/* no interrupt selected */
#define	PCIC_IRQ_3		0x03		/* interrupt 3 */
#define	PCIC_IRQ_4		0x04		/* interrupt 4 */
#define	PCIC_IRQ_5		0x05		/* interrupt 5 */
#define	PCIC_IRQ_7		0x07		/* interrupt 7 */
#define	PCIC_IRQ_9		0x09		/* interrupt 9 (2) */
#define	PCIC_IRQ_10		0x0a		/* interrupt 10 */
#define	PCIC_IRQ_11		0x0b		/* interrupt 11 */
#define	PCIC_IRQ_12		0x0c		/* interrupt 12 */
#define	PCIC_IRQ_14		0x0e		/* interrupt 14 */
#define	PCIC_IRQ_15		0x0f		/* interrupt 15 */
#define	PCIC_IRQ_ENABLE		0x10		/* interrupt enable */
#define	PCIC_IRQ_TYPE_IO	0x20		/* io card */
#define	PCIC_IRQ_NOTRESET	0x40		/* inverted reset line */
#define	PCIC_IRQ_RING		0x80		/* ring indicate line */

/*
 *	Power defs
 */

#define	PCIC_PWR_VPP1_0		0x01		/* Vpp1 control bit 0 */
#define	PCIC_PWR_VPP1_1		0x02		/* Vpp1 control bit 1 */
#define	PCIC_PWR_VPP2_0		0x04		/* Vpp2 control bit 0 */
#define	PCIC_PWR_VPP2_1		0x08		/* Vpp2 control bit 1 */
#define	PCIC_PWR_ENABLE		0x10		/* pc card power enable */
#define	PCIC_PWR_AUTO		0x20		/* auto pwr switch enable */
#define	PCIC_PWR_DIS_RESETDRV	0x40		/* disable RESETDRV */
#define	PCIC_PWR_OENABLE	0x80		/* output enable */

/* Chip Identification stuff */
#define PCIC_TYPEPD67XX_IDREGISTER 0x1f
#define PCIC_TYPEPD67XX_IDMASK     0xc0
#define PCIC_TYPEPD67XX_REVMASK    0x1e
#define PCIC_TYPEPD67XX_SLOTMASK   0x20
#define PCIC_TYPEVADEM_DMAREG	   0x3a
#define PCIC_TYPEVADEM_UNLOCK 	   0x80
#define PCIC_TYPEVADEM_VADEMREV	   0x40


#define PCICWINNUM_TO_MEMREG(x) (0x10 + ( x << 3 ) )  /* Converts a window number to pcic register */
 
/*
 *
 *	STRUCTURES
 *
 */

/*
 *	PCIC Socket Structure
 */

typedef struct {

	ulong_t			iobase;		/* base io address */
	ulong_t			regbase;	/* register base on pcic */

} socket_t;

/*
 *	PCIC Card Structure
 */

typedef struct {
	ulong_t        pcic_iobase;     /* io base location */
	ulong_t        pcic_irq;        /* irq ( optional ) */
	ulong_t        pcic_memstart;   /* io addr start */
	ulong_t        pcic_memsize;    /* io addr end */
	uchar_t        pcic_revlevel;  	/* revision level */
	uchar_t        pcic_numsockets;	/* number of sockets */
	uchar_t        pcic_model;      /* Who makes this puppy? */
#define MODELNA		0x00
#define VADEMVG		0x01
#define CLPD67XX 	0x02
#define VLSIVL82C	0x04
	socket_t       *pcic_socketp;	/* array of sockets on this bd */
#ifdef CS_EVENT
	void 	       *pcic_intr_cookiep; /* Interrupt cookie ... mmm */
#endif
} pcic_t;

#define PCIC_MIN_CMMEMORYRANGE 8191

#define PCIC_NUMMEMWIN 5	/* Number of Mem Windows */
#define PCIC_NUMIOWIN 2		/* Number of IO Windows */
#define PCICIRQMAP  0xdeb8      /* Bitmap of supported IRQs */

#define PCIC_MAXCYCLE 210  /* Maximum value of SYSCLK period (in nanoseconds) */


/*   
 *   This structure will allow us to map cards that may not have 
 *   a CISTPL_FUNCID in their CIS.   If the CISTPL_FUNCID is not found
 *   parse the CIS for the CISTPL_VERS_1 tuple.  Combine the manufacturer
 *   string, and the name of product string, and look them up in this list
 */
struct pcic_cardmap {
	char *ident;
	int type;
};

extern struct pcic_cardmap cardmap[];
extern int pcic_cardmap_entries;

#define PCICHEIR 1

#endif
