#ifndef _IO_XQUE_XQUE_H	/* wrapper symbol for kernel use */
#define _IO_XQUE_XQUE_H	/* subject to change without notice */

#ident	"@(#)xque.h	1.11"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Keyboard/mouse event queue entries.
 */
typedef struct xqEvent {
	unchar	xq_type;	/* event type (see below) */
	unchar	xq_code;	/* when xq_type is XQ_KEY, => scan code; 
				 * when xq_type is XQ_MOTION or XQ_BUTTON, => 
				 *	bit 0 clear if right button pushed; 
				 *	bit 1 clear if middle button pushed;
				 *	bit 2 clear if left button pushed; 
				 */
	char	xq_x;		/* delta x movement (mouse motion only) */
	char	xq_y;		/* delta y movement (mouse motion only) */
	time_t	xq_time; 	/* event timestamp in "milliseconds" */
} xqEvent;


/*
 * xq_type values.
 */
#define XQ_BUTTON	0	/* button state change only */
#define XQ_MOTION	1	/* mouse movement (and maybe button change) */
#define XQ_KEY		2	/* key pressed or released */

/*
 * The event queue. The size of xqEventQueue structure is 0x1C (28) bytes. 
 */
typedef struct xqEventQueue {
	char	xq_sigenable;	/* 0x00: allow signal when queue becomes non-empty 
				 *	0 => don't send signals 
				 *	non-zero => send a signal if 
				 *		    queue is emptry 
				 * and a new event is added 
				 */
	int	xq_head;	/* 0x04: index into q of next event to be dequeued */
	int	xq_tail;	/* 0x08: index into q of next event slot to be filled */
	time_t	xq_curtime;	/* 0x0C: time in milliseconds */
	int	xq_size;	/* 0x10: no. of elements in xq_events array */
	xqEvent	xq_events[1];	/* 0x14: configurable-size array of events */
} xqEventQueue;


#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * The driver's private data structure to keep track of xqEventQueue.
 * The size of xqInfo structure is 0x34 (52) bytes.
 */
typedef struct xqInfo {
	xqEventQueue *xq_queue;	/* 0x00: ptr to the xqEventQueue structure */
	caddr_t xq_private;	/* 0x04: */
	caddr_t xq_qaddr;	/* 0x08: ptr to the SCO QUEUE structure */
	char	xq_qtype;	/* 0x0C: xque or SCO que */
	char	xq_buttons;	/* 0x0D: */
	char	xq_devices;	/* 0x0E: devices that uses the SCO que */
	char	xq_xlate;	/* 0x0F: Should we translate scancodes? */
	int     (*xq_addevent)(); /* 0x10: xque or SCO que addevent routine*/
	int	xq_ptail;	/* 0x14: private copy of xq_tail */
	int	xq_psize;	/* 0x18: private copy of xq_size */
	int	xq_signo;	/* 0x1C: signal no. to send for xq_sigenable */
	void	*xq_proc;	/* 0x20: reference to the X server process */
	struct xqInfo *xq_next;	/* 0x24: next xqInfo structure in list */
	struct xqInfo *xq_prev;	/* 0x28: previous xqInfo structure in list */
	addr_t	xq_uaddr;	/* 0x2C: user virtual addr of shared xq space */
	uint_t	xq_npages;	/* 0x30: size of shared xq space in unit of pages */
} xqInfo;


#define	QUE_KEYBOARD		1
#define	QUE_MOUSE		2

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

/*
 * external functions
 */
struct evchan;
extern addr_t xq_allocate_scoq(struct evchan *evchanp, int *errorp);
extern void xq_close(xqInfo *qinfo);
extern int xq_close_scoq(xqInfo *qinfo);
extern int xq_enqueue(xqInfo *qinfo, xqEvent *ev);
extern caddr_t xq_init(xqInfo *qinfo, int qsize, int signo, int *errorp);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_XQUE_XQUE_H */
