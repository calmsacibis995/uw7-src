#ifndef	_IO_CHAR_CHAR_H	/* wrapper symbol for kernel use */
#define	_IO_CHAR_CHAR_H	/* subject to change without notice */

#ident	"@(#)char.h	1.7"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/mouse.h>		/* REQUIRED */
#include <io/stream.h>		/* REQUIRED */
#include <io/ws/ws.h>		/* REQUIRED */
#include <io/xque/xque.h>	/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/mouse.h>		/* REQUIRED */
#include <sys/stream.h>		/* REQUIRED */
#include <sys/ws/ws.h>		/* REQUIRED */
#include <sys/xque.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


#define	IBSIZE		16	/* "standard" input data block size */
#define	OBSIZE		64	/* "standard" output data block size */
#define	EBSIZE		16	/* "standard" echo data block size */


#ifndef MIN
#define	MIN(a, b)	(((a) < (b)) ? (a) : (b))
#endif


#define	MAXCHARPSZ	1024
#define	CHARPSZ		64


typedef struct copystate {
	ulong_t cpy_arg;
	ulong_t cpy_state;
} copy_state_t;


/*
 * cpy_state flags.
 */
#define CHR_IN_0	0x0
#define CHR_IN_1	0x1
#define CHR_OUT_0	0xF000
#define CHR_OUT_1	0xF001


/*
 * The char_stat structure contains information about scancode-to-character
 * set translation and mouse event processing on its read side. It has
 * pointers to the shared character mapping tables and screen mapping tables.
 * These tables are passed to the char from the underlying principal 
 * (the KD driver) for that channel in the form of a M_PCPROTO message
 * that is sent upstream upon an open of that channel.
 *
 * It also has a pointer to xqInfo data structure which is needed for
 * X Windows support. The X queue is a kernel data area that is mapped
 * by the X server process into its address space so that mouse and
 * keyboard I/O events can be written directly into the X server address
 * space. Only one process per channel can have the X queue enabled.
 *
 * The stream can be set to raw mode (K_RAW) and the character are sent
 * upstream unprocessed. This is required for MERGE/386. A pointer to
 * the process that set the stream to raw mode is saved in char_stat.
 */
struct char_stat {
	lock_t	*c_mutex;	/* 0x00: char_stat basic mutex lock */
	queue_t	*c_rqp;		/* 0x04: saved pointer to read queue */
	queue_t *c_wqp;		/* 0x08: saved pointer to write queue */
	ulong_t	c_state;	/* 0x0C: internal state of tty module */
	mblk_t	*c_rmsg;	/* 0x10: ptr to read-side msg being built */
	mblk_t	*c_wmsg;	/* 0x14: ptr to write-side msg being built */
	charmap_t *c_map_p;	/* 0x18: ptr to shared charmap_t w/ principal stream */
	scrn_t	*c_scrmap_p;	/* 0x1C: ptr to shared scrn_t w/ principal stream */
	xqInfo	*c_xqinfo;	/* 0x20: ptr to XWIN event queue */
	void	*c_rawprocp;	/* 0x24: process that put stream in raw mode */
	kbstate_t c_kbstat;	/* 0x28: ptr to keyboard state struct */
	copy_state_t c_copystate; /* 0x3C: used for ioctl processing */
	xqEvent c_xevent;	/* 0x44: */
	struct mouseinfo c_mouseinfo; /* 0x4C: next 3 for mouse processing */
	mblk_t	*c_heldmseread;	/* 0x50: */
	int	c_oldbutton;	/* 0x54: */
#ifdef MERGE386
	void	(*c_merge_kbd_ppi)(); /* Merge keyboard ppi function pointer */
	void	(*c_merge_mse_ppi)(); /* Merge mouse ppi function pointer */
	struct mcon *c_merge_mcon; /* ptr to merge console structure */
#endif /* MERGE386 */
};

typedef struct char_stat charstat_t;


#ifdef MERGE386
struct chr_merge {
	void (*merge_kbd_ppi)();
	void (*merge_mse_ppi)();
	struct mcon *merge_mcon;
};

typedef struct chr_merge chr_merge_t;
#endif /* MERGE386 */
	

/*
 * Internal state bits.
 */
#define	C_RAWMODE	0x00000001
#define C_XQUEMDE	0x00000002
#define C_FLOWON	0x00000004
#define	C_MSEBLK	0x00000008
#define C_MSEINPUT	0x00000010

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_CHAR_CHAR_H */
