/*		copyright	"%c%" 	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* Copyright (c) 1981 Regents of the University of California */
#ident	"@(#)vi:port/ex_tty.h	1.10.1.5"
#ident  "$Header$"
/*
 * Capabilities from termcap
 *
 * The description of terminals is a difficult business, and we only
 * attempt to summarize the capabilities here;  for a full description
 * see the paper describing termcap.
 *
 * Capabilities from termcap are of three kinds - string valued options,
 * numeric valued options, and boolean options.  The string valued options
 * are the most complicated, since they may include padding information,
 * which we describe now.
 *
 * Intelligent terminals often require padding on intelligent operations
 * at high (and sometimes even low) speed.  This is specified by
 * a number before the string in the capability, and has meaning for the
 * capabilities which have a P at the front of their comment.
 * This normally is a number of milliseconds to pad the operation.
 * In the current system which has no true programmable delays, we
 * do this by sending a sequence of pad characters (normally nulls, but
 * specifiable as "pc").  In some cases, the pad is better computed
 * as some number of milliseconds times the number of affected lines
 * (to bottom of screen usually, except when terminals have insert modes
 * which will shift several lines.)  This is specified as '12*' e.g.
 * before the capability to say 12 milliseconds per affected whatever
 * (currently always line).  Capabilities where this makes sense say P*.
 */

/*
 * From the tty modes...
 */
var	bool	NONL;		/* Terminal can't hack linefeeds doing a CR */
var	bool	UPPERCASE;
var	short	OCOLUMNS;	/* Save columns for a hack in open mode */

var	short	outcol;		/* Where the cursor is */
var	short	outline;

var	short	destcol;	/* Where the cursor should be */
var	short	destline;

var	struct	termios tty;	/* Use this one structure to change modes */
typedef	struct	termios ttymode;	/* Mode to contain tty flags */

var	ttymode	normf;		/* Restore tty flags to this (someday) */
var	bool	normtty;	/* Have to restore normal mode from normf */

ttymode ostart(), setty(), unixex();

var	short	costCM;	/* # chars to output a typical cursor_address, with padding etc. */
var	short	costSR;	/* likewise for scroll reverse */
var	short	costAL;	/* likewise for insert line */
var	short	costDP;	/* likewise for parm_down_cursor */
var	short	costLP;	/* likewise for parm_left_cursor */
var	short	costRP;	/* likewise for parm_right_cursor */
var	short	costCE;	/* likewise for clear to end of line */
var	short	costCD;	/* likewise for clear to end of display */

#ifdef VMUNIX
#define MAXNOMACS	128	/* max number of macros of each kind */
#define MAXCHARMACS	2048	/* max # of chars total in macros */
#else
#define MAXNOMACS	32	/* max number of macros of each kind */
#define MAXCHARMACS	512	/* max # of chars total in macros */
#endif
struct maps {
	unsigned char *cap;	/* pressing button that sends this.. */
	unsigned char *mapto;	/* .. maps to this string */
	unsigned char *descr;	/* legible description of key */
};
var	struct maps arrows[MAXNOMACS];	/* macro defs - 1st 5 built in */
var	struct maps immacs[MAXNOMACS];	/* for while in insert mode */
var	struct maps abbrevs[MAXNOMACS];	/* for word abbreviations */
var	int 	abbrepcnt;		/* Repeating an abbreviation */
var	int	ldisc;			/* line discipline for ucb tty driver */
var	unsigned char	mapspace[MAXCHARMACS];
var	unsigned char	*msnext;	/* next free location in mapspace */
var	int	maphopcnt;	/* check for infinite mapping loops */
var	bool	anyabbrs;	/* true if abbr or unabbr has been done */
var	unsigned char	ttynbuf[20];	/* result of ttyname() */
var	int	ttymesg;	/* original mode of users tty */
