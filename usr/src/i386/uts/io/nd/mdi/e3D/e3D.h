#ident "@(#)e3D.h	26.2"
#ident "$Header$"

/*
 *
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

/*
 * System V STREAMS TCP - Release 3.0 
 *
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI) 
 * All Rights Reserved. 
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.  The copyright above does not evidence any actual or intended
 * publication of this source code. 
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies. 
 */

/*
 *	(C) Copyright 1991 SCO Canada, Inc.
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	SCO Canada, Inc.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by SCO Canada, Inc.
 */

/* Header for 3com507 Ethernet board */

#define _DDI 8             /* to appease ddi.h */

#ifdef _KERNEL_HEADERS
 
#include <util/types.h>
#include <util/param.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strlog.h>
#include <io/log/log.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <io/nd/sys/mdi.h>
#include <util/mod/moddefs.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/conf.h>
#include <util/debug.h>
#include <proc/cred.h>
#include <io/ddi.h>

#else

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/strlog.h>
#include <sys/log.h>
#include <sys/errno.h>
#include <sys/cmn_err.h>
#include <sys/mdi.h>
#include <sys/moddefs.h>
#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#include <sys/conf.h>
#include <sys/debug.h>
#include <sys/cred.h>
#include <sys/ddi.h>

#endif

/*
 * The ethernet header data is contained in the 586 data structures
 * not in the received packet data, so these numbers do not include
 * either the checksum or the ethernet header.
 */
#define E3D_MINPACK	46	/* minimum output packet length */
#define E3D_MAXPACK	1500	/* maximum output packet length */

#define E3D_ETH_MINPACK	60	/* min with ethernet header */
#define E3D_ETH_MAXPACK	1514	/* max with ethernet header */

/*
 * Sizes of network controller data structure values
 */
#define RFD_MIN		20	/* minimum # rfds (in 16KB window) */
#define RB_SIZE		256	/* receive buffer size */

#define TCB_MIN		3	/* minimum # tcbs (in 16KB window) */
#define TBD_MIN		TCB_MIN	/* minimum # tbds (in 16KB window) */


/** 
 ** Start of 82586 network controller definitions -------------------------- 
 **/

/*
 * System Control Pointer
 */
struct scp {
	u_char	scp_sysbus;		/* system bus size */
	u_char	scp_reserved[5];	/* not used */
	u_short	scp_iscpaddrl;	/* address of iscp (low bits) */
	u_short	scp_iscpaddrh;	/* address of iscp (high bits) */
};

/* scp_sysbus bit mappings */
#define SYSBUS_8BIT	1
#define SYSBUS_16BIT	0

/*
 * Immediate System Control Pointer
 */
struct iscp {
	u_char	iscp_busy;		/* initialization indicator */
	u_char	iscp_reserved;		/* not used */
	u_short	iscp_scboffset;		/* offset of scb in memory */
	u_short	iscp_scbbasel;		/* scb base in memory (low bits) */
	u_short	iscp_scbbaseh;		/* scb base in memory (high bits) */
};


/*
 * System Control Block
 */
struct scb {
	u_short scb_status;	        /* status of 82586 */
	u_short scb_command;	        /* command register */
	u_short scb_cbl_ofst;		/* command block offset */
	u_short scb_rfd_ofst;		/* receive frame descriptor offset */
	u_short scb_crc_errs;		/* crc error counter */
	u_short scb_aln_errs;		/* alignment error counter */
	u_short scb_rsc_errs;		/* no of resources counter */
	u_short scb_ovr_errs;		/* over-run error counter */
};

/* scb_status bit mappings */
#define SCB_CX_BIT	0x8000		/* command completed */
#define SCB_FR_BIT	0x4000		/* received a packet */
#define SCB_CNA_BIT	0x2000		/* command unit not active */
#define SCB_RNR_BIT	0x1000		/* no more resources */
#define SCB_INT_BITS	0xF000		/* interrupt bits */

#define SCB_CUS_MASK	0x0F00		/* command unit mask */
#define SCB_CUS_IDLE	0x0000		/* command unit idle */
#define SCB_CUS_SUSP	0x0100		/* command unit suspended */
#define SCB_CUS_ACTIVE	0x0200		/* command unit active */

#define SCB_RUS_MASK	0x00F0		/* receive unit mask */
#define SCB_RUS_IDLE	0x0000		/* receive unit idle */
#define SCB_RUS_SUSP	0x0010		/* receive unit suspended */
#define SCB_RUS_NO_RES	0x0020		/* no resources for receive unit */
#define SCB_RUS_READY	0x0040		/* receive unit ready */

/* scb_command bit mappings */
#define SCB_CUC_START	0x0100		/* command to start command unit */
#define SCB_CUC_RESUME	0x0200		/* command to resume the unit */
#define SCB_CUC_SUSP	0x0300		/* command to suspend the unit */
#define SCB_CUC_ABORT	0x0400		/* command to abort active command */

#define SCB_RUC_START	0x0010		/* command to start receive unit */
#define SCB_RUC_RESUME	0x0020		/* command to resume the unit */
#define SCB_RUC_SUSP	0x0030		/* command to suspend the unit */
#define SCB_RUC_ABORT	0x0040		/* command to abort the unit */

#define SCB_RESET_BIT	0x0800		/* software reset bit */
#define RESET_OK_BITS	0xA000		/* reset status bits */
#define RECVUNIT_INT	0x5000		/* receive unit needs attention */
#define RECV_UNIT_ACK	0x5000		/* receive unit ack */
#define CMDUNIT_INT	0xA000		/* command unit needs attention */
#define CMD_UNIT_ACK	0xA000		/* command unit ack */

#define SCB_MAX_ERRS	0xffff		/* max value of error counters */

/*
 * Struct of a General Action Command Block
 */
struct cbl {
	u_short cbl_status;		/* command completion code */
	u_short cbl_command;		/* command code */
	u_short cbl_link;		/* next command block */
	u_char	cbl_data[100];		/* command specific data */
};

/* cbl_status bit mappings */
#define CBL_C_BIT	0x8000		/* completion bit */
#define CBL_B_BIT	0x4000		/* busy bit */
#define CBL_OK_BIT	0x2000		/* error-free bit */
#define CBL_A_BIT	0x1000		/* aborted bit */

/* cbl_command bit mappings */
#define CBL_EL_BIT	0x8000		/* last command block on list */
#define CBL_I_BIT	0x2000		/* interrupt host after completion */
#define CBL_S_BIT	0x4000		/* suspend cu after completion */
#define CBL_CMD_MASK	0x0007		/* command code */

/* cbl_command values */
#define CMD_NOP		0		/* no-operation */
#define CMD_IASETUP	1		/* individual address setup */
#define CMD_CONFIGURE	2		/* configuration (options) */
#define CMD_MULTICAST	3		/* multicast address setup */
#define CMD_TRANSMIT	4		/* transmit a packet */
#define CMD_TDR		5		/* Time Domain Reflectometry test */
#define CMD_DUMP	6		/* dump chip configuaration */
#define CMD_DIAGNOSE	7		/* internal diagnostic test */

/*
 * Individual Address command block
 */
struct iasetup {
	u_short ias_status;		/* command completion code */
	u_short ias_command;		/* command code */
	u_short ias_link;		/* next command block */
	macaddr_t ias_addr;		/* ethernet address */
};

/*
 * Multicast Control Block
 */

#ifdef NEVER
too big for bzero in e3Dhwinit:  must fit in devmem_mapin/physmap'ed area
so subtract a bunch to make all pointers and structures fit in shared memory
area.
#define	E3D_NMCADDR	2730		/* maximum # of multicast addresses */
#endif
#define	E3D_NMCADDR	1000		/* maximum # of multicast addresses */

struct mcsetup {
	u_short	mcs_status;		/* Command status */
	u_short	mcs_command;		/* Command */
	u_short	mcs_link;		/* Address of next command block */
	u_short	mcs_cnt;		/* # of bytes is list field */
	macaddr_t mcs_list[E3D_NMCADDR];/* The multicast addresses */
};

/*
 * Diagnose Command Block
 */
struct diag {
	u_short diag_status;		/* command completion code */
	u_short diag_command;		/* command code */
	u_short diag_link;		/* next command block */
};

/*
 * Dump Command Block
 */
struct dump {
	u_short dmp_status;		/* command completion code */
	u_short dmp_command;		/* command code */
	u_short dmp_link;		/* next command block */
	u_short	dmp_bufoffset;		/* dump buffer offset */
};

/*
 * Dump Buffer
 */
#define DUMPBUFSIZE	170
typedef char dump_buf[DUMPBUFSIZE];

/*
 * Transmit Control Block
 */
struct tcb {
	u_short	tcb_status;		/* command completion code */
	u_short	tcb_command;		/* command code */
	u_short	tcb_link;		/* next command block */
	u_short	tcb_tbdoffset;		/* offset of transmit buffer des */
	macaddr_t tcb_dstaddr;		/* Ethernet address of destination */
	u_short	tcb_len;		/* 16 bit length field */
};

/* tcb_status bit mappings */
#define TCB_C_BIT	0x8000		/* Command Completed */
#define TCB_B_BIT	0x4000		/* Busy with Execution */
#define TCB_OK_BIT	0x2000		/* Error free Completion */
#define TCB_ABORT	0x1000		/* Command Aborted */
#define TCB_CSLOST	0x0400		/* No Carrier Sense during xmit */
#define TCB_NOCTS	0x0200		/* Lost off Clear to Send */
#define TCB_DMAURUN	0x0100		/* DMA Underrun */
#define TCB_DEFER	0x0080		/* Traffic caused defer of send */
#define TCB_HEARTBEAT	0x0040		/* SQE test failed */
#define TCB_MAXCOLLSNS	0x0020		/* Too many collisions on xmit */
#define TCB_COLLSNS_CNT	0x000F		/* Number of collision */

/* tcb_command bit mappings */
#define TCB_EL_BIT	0x8000		/* End of command list */
#define TCB_I_BIT	0x2000		/* Interrupt after completion */

/*
 * Transmit Buffer Descriptor
 */
struct tbd {
	u_short	tbd_count;		/* size of xmit buffer + EOF bit */
	u_short	tbd_link;		/* next tbd */
	u_short	tbd_xbuffpl;		/* addr of xmit buffer (low bits) */
	u_short	tbd_xbuffph;		/* addr of xmit buffer (high bits) */
};

#define TBD_EOF_BIT	0x8000		/* Last buffer of this chain */

/*
 * Transmit Buffer
 */
struct tbuff {
	u_char tb_data[E3D_MAXPACK];	/* Max size of ethernet packet */
};

/*
 * Command Block Union
 */
union cbu {
	struct cbl	cbl;
	struct tcb	tcb;
	struct iasetup	ias;
	struct diag	diag;
	struct dump	dump;
	struct mcsetup	mcs;
};

/*
 * Receive Frame Descriptor
 */
struct rfd {
	u_short	rfd_status;		/* receive packet status */
	u_short	rfd_cntl;		/* control bits for 586 */
	u_short	rfd_link;		/* next rfd on the link-list */
	u_short	rfd_rbd_ofst;		/* offset of the rbd */
	macaddr_t rfd_dstaddr;		/* dest of received packet */
	macaddr_t rfd_srcaddr;		/* source of received packet */
	u_short rfd_length;		/* Length/Type field of packet */
};

/* rfd_status bit mappings */
#define RFD_C_BIT	0x8000		/* completion bit */
#define RFD_B_BIT	0x4000		/* busy bit */
#define RFD_OK_BIT	0x2000		/* error-free bit */
#define RFD_CRCERR_BIT	0x0800		/* crc error bit */
#define RFD_ALNERR_BIT	0x0400		/* alignment error bit */
#define RFD_RSCERR_BIT	0x0200		/* out-of-resources bit */
#define RFD_OVRERR_BIT	0x0100		/* over-run error bit */
#define RFD_SHORT_BIT	0x0080		/* short frame */
#define RFD_EOFERR_BIT	0x0040		/* No eof flag detected bit */

/* rfd_cntl bit mappings */
#define RFD_EL_BIT	0x8000		/* last rfd on the list */
#define RFD_S_BIT	0x4000		/* suspend after this recv */


/*
 * Receive Buffer Descriptor
 */
struct rbd {
	u_short rbd_status;		/* stat bit + actual frame size */
	u_short rbd_link;		/* next rbd on the link-list */
	u_short	rbd_rbaddrl;		/* addr of recv buffer (low bits) */
	u_short	rbd_rbaddrh;		/* addr of recv buffer (how bits) */
	u_short	rbd_rbuffsize;		/* receive buffer size */
};

#define RBD_EOF_BIT	0x8000		/* last buffer holding the packet */
#define RBD_F_BIT	0x4000		/* act count field valid */
#define RBD_PKTLEN_BITS	0x3FFF		/* mask for packet length */

/* rbd_rbuffsize bit mappings */
#define RBD_EL_BIT	0x8000		/* last rbd on the link-list */


/*
 * Receive Buffer
 */
struct rbuff {
	u_char	rbuff_data[RB_SIZE];	/* Maximum receiveable packet */
}; 

/*
 * The last byte of the RAM window accesible by the host always
 * appears at address 0xFFFF (highest) in the 82586's address space.
 */

#define SCB_OFFSET          0x0		/* control structure offset in mem */
#define SCP_BADDR           0xfff6	/* SCP address in 586 mem space */
#define START_BADDR(d)      ((u_long)(64*1024) - (u_long)(d)->ex_memsize)
#define BADDR_TO_VADDR(d,a) (((a) - START_BADDR(d)) + (u_long)(d)->ex_rambase) 
#define VADDR_TO_BADDR(d,a) ((u_short)(((u_long)(a) - \
                            (u_long)(d)->ex_rambase) + START_BADDR(d)))
#define BOFF_TO_VADDR(d,o)  (((o) - SCB_OFFSET) + (u_long)(d)->ex_rambase) 
#define VADDR_TO_BOFF(d,a)  ((u_short)((u_long)(a) - (u_long)(d)->ex_rambase) \
			    + SCB_OFFSET)
#define NULL_OFFSET         0xFFFF

/*
 * The following structure describes the usage of the RAM memory window.
 * These are some of the 82586 control structures.
 */
struct control {
	struct	iscp	iscp;		/* Immediate System Config Pointer */
	struct	scb	scb;		/* System Control Block */
	union 	cbu	cbu;		/* Command Block List */
};
/** 
 ** End of 82586 network controller definitions -------------------------- 
 **/

/* Multicast address */
#define ISMULTICAST(ea)	(((caddr_t)(ea))[0] & 1)

/* Broadcast address */
#define ISBROADCAST(ea)	((((caddr_t)(ea))[0] & 0xff) == 0xff && \
			 mdi_addrs_equal(e3D_broad, ea))

/* Ethernet frame */
typedef struct  {
	macaddr_t	eh_daddr;	/* 48-bit destination address */
	macaddr_t	eh_saddr;	/* 48-bit source address */
	unsigned short	eh_type;	/* 16-bit type field */
} e_frame_t;

typedef struct e3Ddev_s {
	short		ex_int;		/* interrupt vector number */
	short		ex_ioaddr;	/* I/O ports for device */
	caddr_t		ex_base;	/* phys address of board memory */
	u_short		ex_bnc;		/* 0 = AUI, 0x80 = BNC */
	long		ex_memsize;	/* size of board memory */
	caddr_t		ex_rambase;	/* Virtual address of board mem */
	caddr_t		ex_rombase;	/* phys address of ROM */
	u_long		ex_romsize;	/* size of ROM */
	u_short		ex_zerows;	/* Zero Wait State */
	u_short		ex_sad;		/* SA Address Decode */
	u_char		ex_partno[6];	/* part number (in BCD) */
	u_short		ex_creg2;	/* copy of board control register */
	u_short		ex_ramr2;	/* copy of ram configuration register */
	u_short		ex_romr2;	/* copy of rom configuration register */
	u_short		ex_intr2;	/* copy of interrupt config register */
	int		ex_open;	/* opened yet ? */
	queue_t		*ex_up_queue;	/* upstream queue */
	short		ex_present;	/* board present */
	short		ex_hwinited;	/* hardware initialized flag */
	macaddr_t	ex_eaddr;	/* ethernet addr */
	void		*dlpi_cookie;	/* passed from DLPI BIND_REQ */
	unsigned int	mccnt;
	struct control  *ex_control;	/* 586 control structures */
#ifdef DUMP_82586_REGS
	dump_buf	*ex_dumpbuf;	/* 586 register dump buffer */
#endif
	long		ex_ntcb;	/* # of Transmit Control Blocks */
	struct tcb	*ex_tcb;	/* Transmit Control Blocks */
	long		ex_ntbd;	/* # of Transmit Block Descriptors */
	struct tbd	*ex_tbd;	/* Transmit Block Descriptors */
	long		ex_ntbuff;	/* # of Transmit Buffers */
	struct tbuff	*ex_tbuff;	/* Transmit Buffers */
	long		ex_nrfd;	/* # of Receive Frame Descriptors */
	struct rfd	*ex_rfd;	/* Receive Frame Descriptors */
	long		ex_nrbd;	/* # of Receive Block Descriptors */
	struct rbd	*ex_rbd;	/* Receive Block Descriptors */
	long		ex_nrbuff;	/* # of Receive Buffers */
	struct rbuff	*ex_rbuff;	/* Receive Buffers */
	u_int		ex_flags;	/* private flags */
	struct tcb	*ex_tcbhead;	/* next xmit cmd to be executed */
	struct tcb	*ex_tcbnext;	/* next available unused xmit blk */
	struct tcb	*ex_tcbtail;	/* end of xmit block list */
	struct rfd	*ex_rfdhead;	/* First rfd on 586 list */
	struct rfd	*ex_rfdtail;	/* Last rfd on 586 list */
	struct rbd	*ex_rbdhead;	/* First rbd on 586 list */
	struct rbd	*ex_rbdtail;	/* Last rbd on 586 list */
#ifdef KEEP_LAST_RBD_EMPTY
	struct rbd	*ex_rbdtail2;	/* rbd before last on 586 list */
#endif
	union cbu	*ex_cbuhead;	/* next action cmnd to be executed */
	queue_t		*ex_iocq;	/* queue for ioctl reply */
	mblk_t 		*ex_iocmp; 	/* message block with ioctl request */
	mac_stats_eth_t	ex_macstats;	/* MAC statistics */
	macaddr_t	ex_hwaddr;	/* factory ethernet addr */
	void			*intr_cookie;  /* used with call to cm_intr_attach */
	rm_key_t		rmkey;			/* resmgr key associated with this device */
	u_int			issuspended;	/* currently suspended with CFG_SUSPEND? */
	u_int			dropsuspended;	/* how many frames dropped while suspended */
} e3Ddev_t;

/* Definitions for flags field */
#define	ED_BUFSBUSY	0x01		/* All buffers in use */
#define ED_CUWORKING	0x02		/* Xmit or Cmnd in progress */
#define	ED_CBLSBUSY	0x04		/* All cbls in use */
#define	ED_CBLREADY	0x08		/* Action command ready to execute */

/*
 * These defines describe the control ports on the 3com 507
 * The base address for these registers is obtained from
 * the device structure, thus there must be an initialized
 * pointer to the structure for this board in scope
 * when the define is used.
 */
#define ED_ID_PORT	0x100
#define ED_NM_DATA(d)	(((d)->ex_ioaddr)+0)
#define ED_CNTRL(d)	(((d)->ex_ioaddr)+6)
#define ED_INTR_CLR(d)	(((d)->ex_ioaddr)+0xA)
#define ED_CHAN_ATTN(d)	(((d)->ex_ioaddr)+0xB)
#define ED_ROM_CONF(d)	(((d)->ex_ioaddr)+0xD)
#define ED_RAM_CONF(d)	(((d)->ex_ioaddr)+0xE)
#define ED_INT_CONF(d)	(((d)->ex_ioaddr)+0xF)
#define ED_IO_SIZE	16

/*
 * Network management data banks
 */
#define	HNM_SIZE	6		/* # bytes of data */
    
/*
 * The bits in the ED_CNTRL register
 */
#define HCTL_VB		0x03		/* network management data bank */
#define HCTL_IEN	0x04		/* interrupt enable */
#define HCTL_INT	0x08		/* interrupt active */
#define HCTL_LAD	0x10		/* adapter as 8 or 16 bit device */
#define HCTL_LBK	0x20		/* loopback enable */
#define HCTL_RST	0x80		/* 82586 reset */

/*
 * The network management data bank settings (use HCTL_VB as a mask)
 */
#define HCTL_SIGNAT	0x00		/* 3Com signature = "*3COM*" */
#define HCTL_EADDR	0x01		/* ethernet address */
#define HCTL_PARTNO	0x02		/* adapter part numb and rev level */

/*
 * The bits in the ED_ROM_CONF register
 */
#define HROM_SIZE	0x03		/* ROM window size */
#define HROM_ADDR	0x3C		/* ROM window base address */
#define HROM_BNC	0x80		/* enable on-board transceiver */

/*
 * The ROM window size settings (use HROM_SIZE as a mask)
 */
#define HROM_NONE	0x00		/* no boot ROM present */
#define HROM_08KB	0x01		/* 08 KB */
#define HROM_16KB	0x02		/* 16 KB */
#define HROM_32KB	0x03		/* 32 KB */

/*
 * The bits in the ED_RAM_CONF register
 */
#define HRAM_CONF	0x3F		/* RAM configuration bits */
#define HRAM_SAD	0x40		/* disable SAD mode */
#define HRAM_0WS	0x80		/* enable zero wait state */

/*
 * The RAM window base and size settings (use HRAM_CONF as a mask)
 */
#define HRAM_0C0_16KB	0x00		/* 0x0C0000 base, 16KB size */
#define HRAM_0C0_32KB	0x01		/* 0x0C0000 base, 32KB size */
#define HRAM_0C0_48KB	0x02		/* 0x0C0000 base, 48KB size */
#define HRAM_0C0_64KB	0x03		/* 0x0C0000 base, 64KB size */

#define HRAM_0C8_16KB	0x08		/* 0x0C8000 base, 16KB size */
#define HRAM_0C8_32KB	0x09		/* 0x0C8000 base, 32KB size */
#define HRAM_0C8_48KB	0x0A		/* 0x0C8000 base, 48KB size */
#define HRAM_0C8_64KB	0x0B		/* 0x0C8000 base, 64KB size */

#define HRAM_0D0_16KB	0x10		/* 0x0D0000 base, 16KB size */
#define HRAM_0D0_32KB	0x11		/* 0x0D0000 base, 32KB size */
#define HRAM_0D0_48KB	0x12		/* 0x0D0000 base, 48KB size */
#define HRAM_0D0_64KB	0x13		/* 0x0D0000 base, 64KB size */
#define HRAM_0D8_16KB	0x18		/* 0x0D8000 base, 16KB size */
#define HRAM_0D8_32KB	0x19		/* 0x0D8000 base, 32KB size */

#define HRAM_F00_64KB	0x30		/* 0xF00000 base, 64KB size */
#define HRAM_F20_64KB	0x31		/* 0xF20000 base, 64KB size */
#define HRAM_F40_64KB	0x32		/* 0xF40000 base, 64KB size */
#define HRAM_F60_64KB	0x33		/* 0xF60000 base, 64KB size */
#define HRAM_F80_64KB	0x38		/* 0xF80000 base, 64KB size */

/*
 * The bits in the ED_INT_CONF register
 */
#define HINT_LEVEL	0x0F		/* interrupt level select */
#define HINT_RST	0x10		/* reset adapter */
#define HINT_EDO	0x10		/* serial data from EEPROM */
#define HINT_EDI	0x20		/* serial data into EEPROM */
#define HINT_ESK	0x40		/* shift clock to EEPROM */
#define HINT_ECS	0x80		/* chip select to EEPROM */

/*
 * The interrupt request levels
 */
#define HINT_IRL3	0x03		/* IR level 3 */
#define HINT_IRL5	0x05		/* IR level 5 */
#define HINT_IRL7	0x07		/* IR level 7 */
#define HINT_IRL9	0x09		/* IR level 9 */
#define HINT_IRL10	0x0A		/* IR level 10 */
#define HINT_IRL11	0x0B		/* IR level 11 */
#define HINT_IRL12	0x0C		/* IR level 12 */
#define HINT_IRL15	0x0F		/* IR level 15 */

/*
 * Defines for doing common board operations
 */
#define LOAD_IOADDR(v)      outb(ED_ID_PORT, ((((v) - 0x200) / 0x10) & 0x1f))
#define CONFIG_IOADDR(d,a)  (a) = (((d)->ex_ioaddr - 0x200) / 0x10) & 0x1f

#define SET_CNTRL(d,c)    {(d)->ex_creg2 = (c); \
			  outb(ED_CNTRL(d), (d)->ex_creg2);}
#define SELECT_SIGNAT(d)  {(d)->ex_creg2 &= ~HCTL_VB; \
			  (d)->ex_creg2 |= HCTL_SIGNAT; \
			  outb(ED_CNTRL(d), (d)->ex_creg2);}
#define SELECT_EADDR(d)   {(d)->ex_creg2 &= ~HCTL_VB; \
			  (d)->ex_creg2 |= HCTL_EADDR; \
			  outb(ED_CNTRL(d), (d)->ex_creg2);}
#define SELECT_PARTNO(d)  {(d)->ex_creg2 &= ~HCTL_VB; \
			  (d)->ex_creg2 |= HCTL_PARTNO; \
			  outb(ED_CNTRL(d), (d)->ex_creg2);}
#define ENABLE_586INT(d)  {(d)->ex_creg2 |= HCTL_IEN; \
			  outb(ED_CNTRL(d), (d)->ex_creg2);}
#define DISABL_586INT(d)  {(d)->ex_creg2 &= ~HCTL_IEN; \
			  outb(ED_CNTRL(d), (d)->ex_creg2);}
#define ACTIVE_586INT(d)  (inb(ED_CNTRL(d)) & HCTL_INT)
#define LOOPBK_ENABLE(d)  {(d)->ex_creg2 |= HCTL_LBK; \
			  outb(ED_CNTRL(d), (d)->ex_creg2);}
#define LOOPBK_DISABL(d)  {(d)->ex_creg2 &= ~HCTL_LBK; \
			  outb(ED_CNTRL(d), (d)->ex_creg2);}
#define DEVICE_8BIT(d)    {(d)->ex_creg2 &= ~HCTL_LAD; \
			  outb(ED_CNTRL(d), (d)->ex_creg2);}
#define DEVICE_16BIT(d)   {(d)->ex_creg2 |= HCTL_LAD; \
			  outb(ED_CNTRL(d), (d)->ex_creg2);}
#define RESET_586(d)      {outb(ED_CNTRL(d),(d)->ex_creg2 & ~HCTL_RST); \
			  (d)->ex_creg2 |= HCTL_RST; \
			  outb(ED_CNTRL(d), (d)->ex_creg2);}

#define SEND_586CA(d)     outb(ED_CHAN_ATTN(d), 1)
#define CLR_586INT(d)     outb(ED_INTR_CLR(d), 1)

#define SET_ROMREG(d,c)   {(d)->ex_romr2 = (c); \
			  outb(ED_ROM_CONF(d), (d)->ex_romr2);}
#define SET_ROMSIZE(d,s)  {(d)->ex_romr2 &= ~HROM_SIZE; \
			  (d)->ex_romr2 |= (HROM_SIZE & (s)); \
			  outb(ED_ROM_CONF(d), (d)->ex_romr2);}
#define SET_ROMADDR(d,a)  {(d)->ex_romr2 &= ~HROM_ADDR; \
			  (d)->ex_romr2 |= (HROM_ADDR & (a)); \
			  outb(ED_ROM_CONF(d), (d)->ex_romr2);}
#define BNC_ENABLE(d)     {(d)->ex_romr2 |= HROM_BNC; \
			  outb(ED_ROM_CONF(d), (d)->ex_romr2);}
#define BNC_DISABLE(d)    {(d)->ex_romr2 &= ~HROM_BNC; \
			  outb(ED_ROM_CONF(d), (d)->ex_romr2);}
 
#define SET_RAMREG(d,c)   {(d)->ex_ramr2 = (c); \
			  outb(ED_RAM_CONF(d), (d)->ex_ramr2);}
#define SET_RAMCONF(d,c)  {(d)->ex_ramr2 &= ~HRAM_CONF; \
			  (d)->ex_ramr2 |= (HRAM_CONF & (c)); \
			  outb(ED_RAM_CONF(d), (d)->ex_ramr2);}
#define GET_RAMCONF(d)    (inb(ED_RAM_CONF(d)) & HRAM_CONF)
#define ENABLE_0WS(d)     {(d)->ex_ramr2 |= HRAM_0WS; \
			  outb(ED_RAM_CONF(d), (d)->ex_ramr2);}
#define DISABLE_0WS(d)    {(d)->ex_ramr2 &= ~HRAM_0WS; \
			  outb(ED_RAM_CONF(d), (d)->ex_ramr2);}
#define ENABLE_SAD(d)     {(d)->ex_ramr2 &= ~HRAM_SAD; \
			  outb(ED_RAM_CONF(d), (d)->ex_ramr2);}
#define DISABLE_SAD(d)    {(d)->ex_ramr2 |= HRAM_SAD; \
			  outb(ED_RAM_CONF(d), (d)->ex_ramr2);}

#define SET_INTREG(d,c)   {(d)->ex_intr2 = (c); \
			  outb(ED_INT_CONF(d), (d)->ex_intr2);}
#define SET_INTLEVEL(d,l) {(d)->ex_intr2 &= HINT_LEVEL; \
			  (d)->ex_intr2 |= (HINT_LEVEL & (l)); \
			  outb(ED_INT_CONF(d), (d)->ex_intr2);}
#define GET_INTLEVEL(d)   (inb(ED_INT_CONF(d)) & HINT_LEVEL)
#define RESET_BOARD(d)    outb(ED_INT_CONF(d), (d)->ex_intr2 | HINT_RST)
#define GET_0WS(d)        inb(ED_RAM_CONF(d)) & HRAM_0WS
#define GET_ROMADDR(d)    ((inb(ED_ROM_CONF(d)) & HROM_ADDR) << 0xb) + 0xc0000
#define GET_ROMSIZE(d)    (inb(ED_ROM_CONF(d)) & HROM_SIZE)
#define CONFIG_ROMADDR(d,a) (a) = (u_char) ((((int)(d)->ex_rombase) - 0xc0000) >> 0xb)


#define	SET_ESK_EEPROM(d)   {(d)->ex_intr2 |= HINT_ESK; \
			    outb(ED_INT_CONF(d), (d)->ex_intr2);}
#define	SET_ECS_EEPROM(d)   {(d)->ex_intr2 |= HINT_ECS; \
			    outb(ED_INT_CONF(d), (d)->ex_intr2);}
#define	CLR_ESK_EEPROM(d)   {(d)->ex_intr2 &= ~HINT_ESK; \
			    outb(ED_INT_CONF(d), (d)->ex_intr2);}
#define	CLR_ECS_EEPROM(d)   {(d)->ex_intr2 &= ~HINT_ECS; \
			    outb(ED_INT_CONF(d), (d)->ex_intr2);}
#define GET_EDO(d)          (inb(ED_INT_CONF(d)) & HINT_EDO) >> 4
#define	SET_EDI(d)          {(d)->ex_intr2 |= HINT_EDI; \
			    outb(ED_INT_CONF(d), (d)->ex_intr2);}
#define	CLR_EDI(d)          {(d)->ex_intr2 &= ~HINT_EDI; \
			    outb(ED_INT_CONF(d), (d)->ex_intr2);}

/* EEPROM defines */
/* #define TEE_EW		5	/* Time eeprom Te/w (usec) */
#define NEEPROM_CMD		8	/* number op code & addr bits per cmd */
#define NEEPROM_ADDR_BITS	4	/* number of addr bits per eeprom cmd */
#define NEEPROM_DATA_BITS	16	/* number of data bits per eeprom cmd */
#define WORD_MASK		0xffff
#define EEPROM_READ0		0x80	/* read register 0	op code 10XX */
#define EEPROM_READ1		0x81	/* read register 1	op code 10XX */
#define EEPROM_WRITE0		0x40	/* write register 0	op code 01XX */
#define EEPROM_WRITE1		0x41	/* write register 1	op code 01XX */
#define EEPROM_ERASE0		0xc0	/* erase register 0	op code 11XX */
#define EEPROM_ERASE1		0xc1	/* erase register 1	op code 11XX */
#define EEPROM_EWEN		0x30	/* erase/write enable	op code 0011 */
#define EEPROM_EWDS		0x00	/* erase/write disable	op code 0000 */
#define EEPROM_ERAL		0x20	/* chip erase		op code 0010 */

/* miscellaneous */
#define BRD_MINWINSIZE	(16*1024)	/* 16k window minimum */
#define BRD_MAXWINSIZE	(64*1024)	/* 64k window maximum */
#define MEMTST_PATTERN	0x55CC		/* Pattern to use for memory test */
#define OK		1
#define NOT_OK		0

#define E3D_POLL        1               /* number of ticks to poll for */

/*
 * Global Variables:
 */
extern macaddr_t	e3D_broad;

/*
 * Macros to provide fine controls over debugging messages.
 */
extern unsigned long	e3Ddbgcntrl;

/*
 * Functions:
 */
#if defined(__STDC__) && !defined(_NO_PROTOTYPE)
extern char		*e3Daddrstr(macaddr_t ea);
extern void		e3Dhwclose(e3Ddev_t *dev);
extern int		e3Dhwinit(e3Ddev_t *dev);
extern void		e3Dhwput(e3Ddev_t *dv, mblk_t *m);
extern int		e3Dintr(void *idata);

extern void	 	e3Dioctlack(e3Ddev_t *dev, queue_t *q, 
			    	mblk_t *mp, int status, int release_b_cont);
extern int		e3Dmcaddrset(e3Ddev_t *dv, queue_t *q, mblk_t *mp);
extern int		e3Doktoput(e3Ddev_t *dev);
extern int		e3Dpresent(e3Ddev_t *dv, int);
extern void		e3Drunstate(void);
#endif


#if defined(DEBUG)
#define E3D_DEBUG
#endif

#ifdef E3D_DEBUG
#define	DEBUG00	if (e3Ddbgcntrl&0x00000001) cmn_err
#define	DEBUG01	if (e3Ddbgcntrl&0x00000002) cmn_err
#define	DEBUG02	if (e3Ddbgcntrl&0x00000004) cmn_err
#define	DEBUG03	if (e3Ddbgcntrl&0x00000008) cmn_err
#define	DEBUG04	if (e3Ddbgcntrl&0x00000010) cmn_err
#define	DEBUG05	if (e3Ddbgcntrl&0x00000020) cmn_err
#define	DEBUG06	if (e3Ddbgcntrl&0x00000040) cmn_err
#define	DEBUG07	if (e3Ddbgcntrl&0x00000080) cmn_err
#define	DEBUG08	if (e3Ddbgcntrl&0x00000100) cmn_err
#define	DEBUG09	if (e3Ddbgcntrl&0x00000200) cmn_err
#define	DEBUG10	if (e3Ddbgcntrl&0x00000400) cmn_err
#define	DEBUG11	if (e3Ddbgcntrl&0x00000800) cmn_err
#define	DEBUG12	if (e3Ddbgcntrl&0x00001000) cmn_err
#define	DEBUG13	if (e3Ddbgcntrl&0x00002000) cmn_err
#define	DEBUG14	if (e3Ddbgcntrl&0x00004000) cmn_err
#define	DEBUG15	if (e3Ddbgcntrl&0x00008000) cmn_err
#define	DEBUG16	if (e3Ddbgcntrl&0x00010000) cmn_err
#define	DEBUG17	if (e3Ddbgcntrl&0x00020000) cmn_err
#define	DEBUG18	if (e3Ddbgcntrl&0x00040000) cmn_err
#define	DEBUG19	if (e3Ddbgcntrl&0x00080000) cmn_err
#define	DEBUG20	if (e3Ddbgcntrl&0x00100000) cmn_err
#define	DEBUG21	if (e3Ddbgcntrl&0x00200000) cmn_err
#define	DEBUG22	if (e3Ddbgcntrl&0x00400000) cmn_err
#define	DEBUG23	if (e3Ddbgcntrl&0x00800000) cmn_err
#define	DEBUG24	if (e3Ddbgcntrl&0x01000000) cmn_err
#define	DEBUG25	if (e3Ddbgcntrl&0x02000000) cmn_err
#define	DEBUG26	if (e3Ddbgcntrl&0x04000000) cmn_err
#define	DEBUG27	if (e3Ddbgcntrl&0x08000000) cmn_err
#define	DEBUG28	if (e3Ddbgcntrl&0x10000000) cmn_err
#define	DEBUG29	if (e3Ddbgcntrl&0x20000000) cmn_err
#define	DEBUG30	if (e3Ddbgcntrl&0x40000000) cmn_err
#define	DEBUG31	if (e3Ddbgcntrl&0x80000000) cmn_err
#else
#define	DEBUG00	if (0) cmn_err
#define	DEBUG01	if (0) cmn_err
#define	DEBUG02	if (0) cmn_err
#define	DEBUG03	if (0) cmn_err
#define	DEBUG04	if (0) cmn_err
#define	DEBUG05	if (0) cmn_err
#define	DEBUG06	if (0) cmn_err
#define	DEBUG07	if (0) cmn_err
#define	DEBUG08	if (0) cmn_err
#define	DEBUG09	if (0) cmn_err
#define	DEBUG10	if (0) cmn_err
#define	DEBUG11	if (0) cmn_err
#define	DEBUG12	if (0) cmn_err
#define	DEBUG13	if (0) cmn_err
#define	DEBUG14	if (0) cmn_err
#define	DEBUG15	if (0) cmn_err
#define	DEBUG16	if (0) cmn_err
#define	DEBUG17	if (0) cmn_err
#define	DEBUG18	if (0) cmn_err
#define	DEBUG19	if (0) cmn_err
#define	DEBUG20	if (0) cmn_err
#define	DEBUG21	if (0) cmn_err
#define	DEBUG22	if (0) cmn_err
#define	DEBUG23	if (0) cmn_err
#define	DEBUG24	if (0) cmn_err
#define	DEBUG25	if (0) cmn_err
#define	DEBUG26	if (0) cmn_err
#define	DEBUG27	if (0) cmn_err
#define	DEBUG28	if (0) cmn_err
#define	DEBUG29	if (0) cmn_err
#define	DEBUG30	if (0) cmn_err
#define	DEBUG31	if (0) cmn_err
#endif

