#ifndef _IO_ANSI_AT_ANSI_H	/* wrapper symbol for kernel use */
#define _IO_ANSI_AT_ANSI_H	/* subject to change without notice */

#ident	"@(#)at_ansi.h	1.7"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


/*
 * definitions for PC AT x3.64 terminal emulator
 */
#define ANSI_MAXPARAMS	5	/* maximum number of ANSI paramters */
#define ANSI_MAXTAB	40	/* maximum number of tab stops */
#define ANSI_MAXFKEY	30	/* max length of function key with <ESC>Q */

#define	ANSI_MOVEBASE	0x0001	/* if set move base when scrolling */

/*
 * Font values for ansistate
 */
#define	ANSI_FONT0	0	/* Primary font (default) */
#define	ANSI_FONT1	1	/* First alternate font */
#define	ANSI_FONT2	2	/* Second alternate font */

/*
 * directions for moving bytes in screen memory.
 * UP means toward higher addresses.
 * DOWN means toward lower addresses.
 */
#define UP 		0
#define DOWN 		1

struct attrmask {
	unchar attr;		/* new attribute to turn on */
	unchar mask;		/* old attributes to leave on */
};

/*
 * state for ansi x3.64 emulator 
 */
struct ansistate {	
	ushort	*scraddr;	/* pointer to char/attribute buffer */
	short	ansiid;		/* unique id for this x3.64 terminal */
	ushort	flags;		/* flags for this x3.64 terminal */
	ushort	width;		/* number of characters horizontally */
	ushort	height;		/* number of characters vertically */
	ushort	scrsize;	/* number of characters on screen */
	struct	attrmask	*attrmask; /* attribute masks/values ptr */
	char	nattrmsks;	/* size of attrmask array */
	unchar	normattr;	/* "normal" attribute */
	unchar	undattr;	/* attribute to use when underlining */
	unchar	font;		/* font type */
	int 	(*bell)();	/* ptr to bell function */
	int 	(*clrdisplay)(); /* ptr to clrdisplay function */
	int 	(*moveit)();	/* ptr to moveit function */
	int 	(*setcursor)();	/* ptr to setcursor function */
	int 	(*setbase)();	/* ptr to setbase function */
	int 	(*storeword)();	/* ptr to storeword function */
	int 	(*shiftset)();	/* ptr to shiftset function */
	int	(*undattrset)(); /* ptr to undattrset function */
	void	(*sendscreen)(); /* ptr to sendscreen function */
	void	(*setlock)();	/* ptr to setlock function */
	int	(*addstring)();	/* ptr to add function key string function */
	caddr_t	dspec;		/* device specific information */
	ushort	curbase;	/* upper left hand corner of screen in buffer */
	short	line;		/* current line number */
	short	column;		/* current column number */
	ushort	cursor;		/* current address of cursor, 0-based */
	unchar	state;		/* state in output esc seq processing */
	unchar	undstate;	/* underline processing state */
	unchar	attribute;	/* current attribute for characters */
	unchar	gotparam;	/* does output esc seq have a param */
	ushort	curparam;	/* current param # of output esc seq */
	ushort	paramval;	/* value of current param */
	short	params[ANSI_MAXPARAMS];	/* parameters of output esc seq */
	unchar	ntabs;		/* number of tab stops set */
	unchar	tabs[ANSI_MAXTAB];	/* list of tab stops */
	char	fkey[ANSI_MAXFKEY];	/* work space for function key */
};

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_ANSI_AT_ANSI_H */
