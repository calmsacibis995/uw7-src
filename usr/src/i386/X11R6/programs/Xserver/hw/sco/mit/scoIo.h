/*
 *	@(#) scoIo.h 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989, 1990.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/* Copyright Massachusetts Institute of Technology 1985 */

/* $Header$ */

/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Mon Feb 04 21:36:34 PST 1991	mikep@sco.com
 *	- Created file from mwinput.c
 *
 *	S001	Mon Feb 04 21:37:16 PST 1991	mikep@sco.com
 *	- Added scancode translation struct.  It should go away when
 *	the scancode API is used.  It may stick around for 3.2v2
 *	compatibility.
 *
 *	S002	Tue Mar 26 16:36:43 PST 1991	staceyc@sco.com
 *	- Structure defs added.
 *
 *	S003	Tue Apr 02 14:56:30 PST 1991	staceyc@sco.com
 *	- Changed scancode translation struct to something that will
 *	work with xsconfig.
 *
 *	S004	Tue Jun 11 18:27:40 PDT 1991	mikep@sco.com
 *	- Added MAXEVENTS define
 *
 *	S005	Sat Jun 22 20:41:59 PDT 1991	mikep@sco.com
 *	- Removed devInfo struct and other dead code.
 *
 *	S006	Thu Jul 25 23:16:11 PDT 1991	mikep@sco.com
 *	- Added DEVE_MDELTA and DEVE_MABSOLUTE for absolute device
 *	support.
 * 	
 *	S007	Tue Sep 24 13:27:43 PDT 1996	kylec@sco.com
 *	- Remove scancode translation struct.  Use struct defined
 *	by xsconfig/config.h.
 */

/*
 * Unix event queue entries
 */

#ifndef _SCOIO_H_
#define _SCOIO_H_

#ifdef SYSV
#ifndef u_short
#define u_char	unchar
#define u_short	ushort
#endif
#endif /* SYSV */

#define MAXEVENTS 32						/* S004 */

typedef struct  _dev_event {
        short	deve_x;          /* x position */
        short	deve_y;          /* y position */
	long	deve_time;	/* 1 millisecond units (all events) */
        char    deve_type;       /* button or motion? */
        u_char  deve_key;        /* the key (button only) */
        char    deve_direction;  /* which direction (button only) */
        char    deve_device;     /* which device (button only) */
} devEvent;

/* deve_type field */
#define DEVE_BUTTON      0               /* button moved */
#define DEVE_MDELTA      1               /* mouse moved (delta) */
#define DEVE_MABSOLUTE	 2		 /* mouse moved (absolute) */
#define	DEVE_NOP	 3		 /* null, no operation, event */

/* deve_direction field */
#define DEVE_KBTUP       0               /* up */
#define DEVE_KBTDOWN     1               /* down */
#define DEVE_KBTRAW	2		/* undetermined */

/* egae_device field */
#define DEVE_MOUSE       1               /* mouse */
#define DEVE_DKB         2               /* main keyboard */
#define DEVE_TABLET      3               /* graphics tablet */
#define DEVE_AUX         4               /* auxiliary */
#define DEVE_CONSOLE     5               /* console */

/*
 * The server helpfully insists that key codes be greater than 5;
 * anything less is interpreted as a mouse button.  For safety, we will
 * limit things to 8, not 5.  We do this by just adding an offset factor
 * to the keyboard scan code.  This is actually done in the server, not
 * the driver.  (This define really belongs elsewhere, but if I put it
 * in egakeys.h, ega_io.c won't compile because of too much defining...).
 */
#define DEV_KEY_BASE		7

/* The event queue */

typedef struct _sco_eventqueue {
	devEvent *events;	/* input event buffer */
	int size;		/* size of event buffer */
	int head;		/* index into events */
	int tail;		/* index into events */
} scoEventQueue;

#if !defined(usl)

typedef struct ScanTranslation {		/* S001 */
    unsigned short oldScan;			/* S003 */
    short newScan;				/* S003 */
} ScanTranslation;

/* S002 vvvv */

typedef struct ConfigRec {
    short headerSize;			/* size of ConfigRec */
    short buttonOffset;			/* offset to pointer button map */
    short buttonSize;			/* size of pointer button map */
    short modmapOffset;			/* offset to modifier map */
    short modmapSize;			/* size of modifier map */
    short keymapOffset;			/* offset to keymap record */
    short keymapSize;			/* size of keymap in bytes */
    short keymapWidth;			/* entries per key in keymap */
    short scanBias;			/* add this to hardware scan codes */
    short minScan;			/* lowest scan code (include bias) */
    short maxScan;			/* highest scan code (include bias) */
    short translateOffset;		/* offset to scan translate table */
    short translateSize;		/* size of scan translate table */
    short screenWidth;			/* screen width in millimeters */
    short screenHeight;			/* screen height in millimeters */
    short pixelOffset;			/* offset to pixel values */
    short pixelSize;			/* size of pixel values */
    short keyctrlOffset;		/* offset to key control table */
    short keyctrlSize;			/* size of key control table */
    short pad;				/* to doubleword boundary */
} ConfigRec;

typedef struct PixelValue {
    unsigned long pixel;
    unsigned short red, green, blue;
    unsigned short type;
} PixelValue;

/* S002 ^^^^ */

#endif /* usl */

# endif /* _SCOIO_H_ */
