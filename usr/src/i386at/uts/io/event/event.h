#ifndef _IO_EVENT_EVENT_H	/* wrapper symbol for kernel use */
#define _IO_EVENT_EVENT_H	/* subject to change without notice */

#ident	"@(#)event.h	1.9"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/xque/xque.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/xque.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define QPAGE 4096
#define XQUE    1
#define SCOQ    2

/*
 * This number chosen to make an event 16 bytes long 
 */
#define		EV_STR_BUFSIZE	8

typedef struct {
	long	timestamp;
	short	tag;
	union {
		char	buttons;
		char	bufcnt;
	} u1;
	union {
		unsigned char buf[EV_STR_BUFSIZE];
		union {
			struct {
				unsigned long	x,y;
			} abs;
			struct {
				long	dx,dy;
			} rel;
		} loc;
	} un;
} EVENT;

/*
 * Values for event tag 
 */
#define	T_OTHER		0x0001
#define	T_BUTTON	0x0002
#define	T_STRING	0x0004
#define	T_ABS_LOCATOR	0x0008
#define	T_REL_LOCATOR	0x0010
#define	T_LOCATOR	(T_ABS_LOCATOR | T_REL_LOCATOR)

/*
 * Shorthand notations 
 */
#define	EV_TIME(x)	((x).timestamp)
#define	EV_TAG(x)	((x).tag)		/* device making event*/
#define	EV_BUFCNT(x)	((x).u1.bufcnt)		/* num bytes in buffr */
#define	EV_BUTTONS(x)	((x).u1.buttons)
#define	EV_BUF(x)	((x).un.buf)		/* pointer to buffer  */
#define	EV_DX(v)	((v).un.loc.rel.dx)
#define	EV_DY(v)	((v).un.loc.rel.dy)
#define	EV_X(v)		((v).un.loc.abs.x)
#define	EV_Y(v)		((v).un.loc.abs.y)

/*
 * Bit definitions within the character reserved for button state 
 */
#define	BUTTON1		0x01
#define	BUTTON2		0x02
#define	BUTTON3		0x04
#define	BUTTON4		0x08
#define	RT_BUTTON	BUTTON1
#define	MD_BUTTON	BUTTON2
#define	LT_BUTTON	BUTTON3

/*
 * This number makes a queue one page (4K bytes) long 
 */
#define QSIZE		((QPAGE - 3 * sizeof(long))/sizeof(EVENT))

typedef struct {
	long	overrun;
	long	head;
	long	tail;
	EVENT	queue[QSIZE];
} QUEUE;

/* 
 * Locator events may be ratioed as they enter the queue.
 * The user supplies a multiplication factor, then the driver
 * right shifts by a constant amount.
 */
#define	LOC_RSHIFT	13

#define	R_ONEHALF	0x1000
#define	R_TIMES1	0x2000
#define	R_TIMES2	0x4000
#define	R_TIMES4	0x8000


#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * The type of an event mask (called emask_t from user level)
 */
typedef unsigned short	event_mask_t;

struct evchan {
	dev_t		eq_ttyd;	/* & of tp of controlling tty if any */
	dev_t     	eq_rdev;	/* associated vnode rdev */
	uchar_t  	eq_state;	/* UNUSED, OPEN, ACTIVE, SUSPENDED */
	event_mask_t	eq_emask;
	void    	(*eq_notify)();
	void		*eq_chp;
	caddr_t		eq_rqp;
	caddr_t		eq_block_msg;
	xqInfo		eq_xqinfo;
};

struct event_getq_info {
        caddr_t		einfo_addr;
        dev_t		einfo_rdev;
};

#else /* !(_KERNEL || _KMEMUSER) */

/*
 * The type of an event mask 
 */
typedef unsigned short	emask_t;

/*
 * The type of a device mask 
 */
typedef unsigned short	dmask_t;

#endif /* _KERNEL || _KMEMUSER */

/*
 * Values for the status flag per channel 
 */
#define EVCH_CLOSE        0x00    /* A currently unused channel       */
#define EVCH_OPEN         0x01    /* Queue open but not yet allocated */
#define EVCH_ACTIVE       0x02    /* Channel currently active         */
#define EVCH_SUSPENDED    0x03    /* Channel currently suspended      */


/*
 * event driver ioctls
 */
#define	EQIOC		('Q' << 8)
#define	EQIO_GETQP	(EQIOC | 1)
#define	EQIO_SETEMASK	(EQIOC | 2)
#define	EQIO_GETEMASK	(EQIOC | 3)
#define	EQIO_SUSPEND	(EQIOC | 4)
#define	EQIO_RESUME	(EQIOC | 5)
#define	EQIO_BLOCK	(EQIOC | 6)

/* 
 * line discipline specific ioctls to the mouse line discipline 
 */
#define	EVLD_IOC	(LDIOC)
#define	LDEV_SETTYPE	(EVLD_IOC | 13)		/* set mouse type	*/
#define	LDEV_GETEV	(EVLD_IOC | 14)		/* get an event		*/
#define	LDEV_ATTACHQ	(EVLD_IOC | 15)		/* activate mouse	*/
#define	LDEV_SETRATIO	(EVLD_IOC | 16)		/* set a device ratio	*/

#define	LDEV_MOUSE	('x' << 24 | EVLD_IOC)
#define	LDEV_MSESETTYPE	(LDEV_MOUSE | 13)
#define	LDEV_MSEATTACHQ	(LDEV_MOUSE | 15)

#ifdef _KERNEL

extern int event_check_que(xqInfo *qp, dev_t rdev, void *chp, int cmd);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_EVENT_EVENT_H */
