#ifndef _IO_MSE_MSE_H	/* wrapper symbol for kernel use */
#define _IO_MSE_MSE_H	/* subject to change without notice */

#ident	"@(#)mse.h	1.7"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/autoconf/confmgr/cm_i386at.h>		/* REQUIRED */
#include <io/mouse.h>		/* REQUIRED */
#include <io/stream.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/cm_i386at.h>		/* REQUIRED */
#include <sys/mouse.h>		/* REQUIRED */
#include <sys/stream.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


#define MAX_MSE_UNIT	15

/*
 * The following set of #defines are for either the Logitech Bus
 * Mouse Board or version 8255 of the Microsoft Bus Mouse Board.
 */

/*
 * Hardware interface ports 
 */
#define DATA_PORT		BASE_IOA	/* Read only */
#define SIGNATURE_PORT		(BASE_IOA + 1)	/* Read/write */
#define INTERRUPT_PORT		(BASE_IOA + 2)	/* Read only */
#define CONTROL_PORT		(BASE_IOA + 2)	/* Write only */
#define CONFIGURATOR_PORT	(BASE_IOA + 3)	/* Read/write */

/*
 * Control port bits 
 */
#define INTR_ENABLE	0x00
#define INTR_DISABLE	0x10
#define HIGH_NIBBLE	0x20
#define LOW_NIBBLE	0x00
#define X_COUNTER	0x00
#define Y_COUNTER	0x40
#define HC		0x80

/*
 * Macros to make accessing the BUS mouse ports easier 
 */
#define data_port	(inb(DATA_PORT))
#define control_port(x)	(outb(CONTROL_PORT, (x)))

/*
 * End of #defines for Logitech and Microsoft Version 8255 Bus Mouse Boards 
 */

/*
 * The following #defines are for the Microsoft InPort Bus Mouse Board.
 * There are more #defines here than used in the bus mouse driver,
 * however they are retained for completeness.
 */

/*
 * Base I/O addresses for primary and secondary BUS mouse InPort 
 */
#define MOUSE1		0x23c
#define MOUSE2		0x238

/*
 * Offsets of I/O registers from base 
 */
#define ADDRREG		BASE_IOA	/* Address register */
#define DATAREG		(BASE_IOA + 1)	/* Data register */
#define IDENTREG	(BASE_IOA + 2)	/* Identification register */
#define TESTREG		(BASE_IOA + 3)	/* Test register */

/*
 * Address register definitions 
 */
#define REGSEL		7       /* Mask for register select bits */
#define MSTATUS		0       /* Select mouse status register */
#define DATA1		1       /* Select data register 1 */
#define DATA2		2       /* Select data register 2 */
#define DATA3		3       /* Select data register 3 */
#define DATA4		4       /* Select data register 4 */
#define ISTATUS		5       /* Select interface status register */
#define ICNTRL		6       /* Select interface control register */
#define MODE		7       /* Select mode register */
#define TESTEN		0x40    /* Enable test register */
#define RESET		0x80    /* Reset InPort chip */

#define BUTMASK		0x7
#define MSEMOTION	0x40
#define XMOTION		DATA1
#define YMOTION		DATA2

/*
 * Identification register definitions 
 */
#define SIGN		0xde		/* InPort chip signature */

#define VERS(x)		(((x)>>4)&15)   /* InPort chip version number */
#define REV(x)		((x)&15)        /* InPort chip revision number */

/*
 * Interface status/control register definitions 
 */
#define SW3		1
#define SW2		2
#define SW1		4
#define XA		0x10
#define XB		0x20
#define YA		0x40
#define YB		0x80

/*
 * Mode register definitions 
 */
#define RATEMASK        7
#define HZ0NOINTR       0
#define HZ30            1
#define HZ50            2
#define HZ100           3
#define HZ200           4
#define HZ0INTR         6
#define HZEXT           7
#define DATAINT         8
#define TIMERINT        0x10
#define HOLD            0x20
#define QUADMODE        0
#define SYNCHMODE       0x40
#define ASYNMODE        0x80
#define DIRMODE         0xc0

/*
 * End of #defines for Microsoft InPort Bus Mouse Board 
 */

#define MSE_UNIT(dev)	(getminor(dev) % 15)
#define MSE_MINOR(minor_offset, vt)				\
	((((vt) & 7) << 5) | (((vt) & 8) << 1) | (minor_offset))
#define DISP_UNIT(dev)	(getminor(dev) / 15)

/* 
 * These minors define the mouse device function: CLONE is a standard 
 * open of /dev/mouse.
 */

#define MSE_CLONE	MSE_MINOR(0, VTMAX)
#define MSE_MON		MSE_MINOR(1, VTMAX)
#define MSE_CFG		MSE_MINOR(2, VTMAX)

#define SNDERR		0xfe	
#define	FAILED		0xff

struct msecopy {
	int	state;
};

/*
 * Mouse button number definitions.
 */
#define MSE_3_BUTTON	3	/* mouse talks logitech MM data format */
#define MSE_DEFAULT	4		/* no button definition needed */

#ifdef _KERNEL

/*
 * STREAMS mouse info structure 
 */

struct strmseinfo {       
	int	msetimeid;
	queue_t	*rqp;
	queue_t	*wqp;
	struct msecopy	copystate;
	char 	state;
	uchar_t type;
	struct mouseinfo mseinfo;
	char 	x;
	char	y;
	uchar_t button;
	uchar_t	old_buttons;			
	dev_t	dev;
	void    (*smseparse)();
	char    report_not_sent;
	char    middle_button_down;
	mblk_t	*stash_event; 			/* L999 saved event packet	*/	
	toid_t	timeout_id; 			/* L999	timer running ID	*/
	int		timed_out;				/* L999	timer semaphore		*/
	int		fsm_state; 				/* L999	state machine var	*/	
	int		fsm_timeout;			/* L999	per mouse emulation	*/
};

/* L999 begin */

#define STATE_0		0
#define STATE_1		1				
#define STATE_2		2

#define DISCARD_EVENT 	0
#define SEND_EVENT 		1
#define UNSTASH_EVENT 	2

#define DISABLE_3BEMUL	0
#define DEFAULT_DELAY	-1

/* L999 end 	*/

#endif /* _KERNEL */

#define BUS_MAJOR	0
#define MCA_MAJOR	1

#define	MBUS		1
#define	M320		2
#define	MSERIAL		3

/*
 * Mouse configuration 
 */
struct mse_cfg {
	struct mousemap	*mapping;	/* Pointer to mapping table */
	unsigned	count;		/* # of entries in mapping table */
};

/* 
 * Mouse 3rd button emulation ioctl.
 * Send to hardware devices, ie. dev/bmse|m320|smse to set emulation mode.
 * Arg = int, 0 disables, -1 sets default delay, m > 0 sets m ticks delay.
 */

#define MOUSEIOC3BE		(('M'<<8)|110)

/*
 * Mouse monitor daemon 
 */
struct mse_mon {
	int		cmd;		/* Command from driver to monitor */
	dev_t		dev;		/* Display station for mouse */
	dev_t		mdev;		/* Mouse Device */
	uchar_t		errno;		/* Error status from action */
};

#define MSE_MGR_OPEN	1		/* Command to open device */
#define MSE_MGR_CLOSE	2		/* Command to close device */
#define MSE_MGR_LCLOSE	4		/* Command to last close device */
#define MGR_WAITING	8

/*
 * Structure for mouse information pseudo-ioctl (from mse to display driver) 
 */
#define MOUSE_INFO	(('M'<<16)|('I'<<8)|99)

struct mcastat {
	int mode;		/* stream or prompt mode */
	int present;
	int map_unit;
};

/*
 * Per-unit bus mouse configuration info (in mse/space.c) 
 */
struct mouseconfig {
	struct cm_addr_rng	io_addr;	/* Base/End I/O addresses */
	cm_num_t		ivect;		/* Interrupt vector */
	int			present;	/* Set by detection routine */
	int			map_unit;	/* Index into mse_mapping[] */
	void			*cookie;	/* cookie for interrupts */
};

typedef struct mouseconfig MOUSECNF;

/*
 * Display-to-mouse mapping table (sent by mouseadmin) 
 */
struct mousemap {
	dev_t	disp_dev,	/* Display device id */
		mse_dev;	/* Serial mouse device id;
				 * or makedev(0,unit #) for bus mouse 
				 */
	int	type;		/* type of mouse (MBUS, M320, etc) */
};

typedef struct mousemap MOUSEMAP;

/*
 * AT&T 320 mouse (8042 controller) I/O port addresses
 */
#define MSE_OUT		 0x60	/* output buffer R/O */
#define MSE_IDAT	 0x60	/* input buffer data write W/O */
#define MSE_STAT	 0x64	/* 8042 controller status R/O */
#define MSE_ICMD	 0x64	/* input buffer command write W/O */

/*
 * Mouse driver internal status structure kept for each virtual mouse 
 */
typedef struct {
	struct proc *u_procp;
	int	mse_pid;
	int	rupted;
	int	isopen;
	char	old_buttons;
} MOUSE_STRUCT;

/*
 * Mouse driver internal status structure kept for each physical mouse 
 */
typedef struct {
	MOUSEMAP	map;		/* Device assignment info */
	int		n_vts;
	MOUSE_STRUCT	*ms;		/* Mouse structs for this mouse */
	struct tty	*ttyp;		/* TTY struct of serial mouse */
	int		old;		/* Save old line discipline here */
	int		state;		/* Serial mse input parsing state */
	int		x_ovr;		/* 320 overflow state */
	int		y_ovr;		/* 320 overflow state */
	int		status;		/* Status of manager command */
} MOUSE_UNIT;

/*
 * Serial mouse protocol types.
 */

#define MM_MSE		1
#define MSC_MSE		2
#define MPLUS_MSE	3


#if defined(__cplusplus)
	}
#endif

#endif /* _IO_MSE_MSE_H */
