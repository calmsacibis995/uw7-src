/*
 * $Id$
 *
 * Program:	Global definitions and initializations
 *
 *
 * Michael Seibel
 * Networks and Distributed Computing
 * Computing and Communications
 * University of Washington
 * Administration Builiding, AG-44
 * Seattle, Washington, 98195, USA
 * Internet: mikes@cac.washington.edu
 *
 * Please address all bugs and comments to "pine-bugs@cac.washington.edu"
 *
 *
 * Pine and Pico are registered trademarks of the University of Washington.
 * No commercial use of these trademarks may be made without prior written
 * permission of the University of Washington.
 * 
 * Pine, Pico, and Pilot software and its included text are Copyright
 * 1989-1996 by the University of Washington.
 * 
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this distribution.
 *
 */
/*	EDEF:		Global variable definitions for
			MicroEMACS 3.2

			written by Dave G. Conroy
			modified by Steve Wilhite, George Jones
			greatly modified by Daniel Lawrence
*/

#ifndef	EDEF_H
#define	EDEF_H

#ifdef	maindef

/* for MAIN.C */

/* initialized global definitions */

int     fillcol = 72;                   /* Current fill column          */
int     userfillcol = -1;               /* Fillcol set from cmd line    */
char    pat[NPAT];                      /* Search pattern		*/
int	eolexist = TRUE;		/* does clear to EOL exist	*/
int	optimize = FALSE;		/* optimize flag(cf line speed)	*/
int	scrollexist = TRUE;		/* does insert line exist	*/
int	inschar = TRUE;			/* does insert character exist	*/
int	delchar = TRUE;			/* does delete character exist	*/
int     sgarbk = TRUE;                  /* TRUE if keyhelp garbaged     */
int     sup_keyhelp = FALSE;            /* TRUE if keyhelp is suppressed*/
int     mline_open = FALSE;             /* TRUE if message line is open */
int	ComposerTopLine = 2;		/* TRUE if message line is open */
int	ComposerEditing = FALSE;	/* TRUE if message line is open */
int	revexist = FALSE;		/* does reverse video exist?	*/
char	modecode[] = "WCSEVO";		/* letters to represent modes	*/
long	gmode = MDWRAP;			/* global editor mode		*/
int     sgarbf  = TRUE;                 /* TRUE if screen is garbage	*/
int     mpresf  = FALSE;                /* TRUE if message in last line */
int	clexec	= FALSE;		/* command line execution flag	*/
char   *alt_speller = NULL;		/* alt spell checking command   */

/* uninitialized global definitions */
int     currow;                 /* Cursor row                   */
int     curcol;                 /* Cursor column                */
int     thisflag;               /* Flags, this command          */
int     lastflag;               /* Flags, last command          */
int     curgoal;                /* Goal for C-P, C-N            */
char	opertree[NLINE+1];	/* operate within this tree     */
WINDOW  *curwp;                 /* Current window               */
BUFFER  *curbp;                 /* Current buffer               */
WINDOW  *wheadp;                /* Head of list of windows      */
BUFFER  *bheadp;                /* Head of list of buffers      */
BUFFER  *blistp;                /* Buffer for C-X C-B           */

BUFFER  *bfind();               /* Lookup a buffer by name      */
WINDOW  *wpopup();              /* Pop up window creation       */
LINE    *lalloc();              /* Allocate a line              */
int	 km_popped;		/* menu popped up               */
#if	defined(TERMCAP) || defined(TERMINFO) || defined(VMS)
KBESC_T *pico_kbesc;		/* keyboard esc sequence trie   */
#endif /* TERMCAP/TERMINFO/VMS */

#else  /* maindef */

/* for all the other .C files */

/* initialized global external declarations */

extern  int     fillcol;                /* Fill column                  */
extern  int     userfillcol;            /* Fillcol set from cmd line    */
extern  char    pat[];                  /* Search pattern               */
extern	int	eolexist;		/* does clear to EOL exist?	*/
extern	int	optimize;		/* optimize flag(cf line speed)	*/
extern	int	scrollexist;		/* does insert line exist	*/
extern	int	inschar;		/* does insert character exist	*/
extern	int	delchar;		/* does delete character exist	*/
extern  int     sgarbk;
extern  int     sup_keyhelp;
extern  int     mline_open;             /* Message line is open         */
extern	int	ComposerTopLine;	/* TRUE if message line is open */
extern	int	ComposerEditing;	/* TRUE if message line is open */
extern	int	timeout;		/* how long we wait in GetKey	*/
extern	int	revexist;		/* does reverse video exist?	*/
extern	char	modecode[];		/* letters to represent modes	*/
extern	KEYTAB	keytab[];		/* key bind to functions table	*/
extern	KEYTAB	pkeytab[];		/* pico's function table	*/
extern	long	gmode;			/* global editor mode		*/
extern  int     sgarbf;                 /* State of screen unknown      */
extern  int     mpresf;                 /* Stuff in message line        */
extern	int	clexec;			/* command line execution flag	*/
extern	char   *alt_speller;		/* alt spell checking command   */
/* initialized global external declarations */
extern  int     currow;                 /* Cursor row                   */
extern  int     curcol;                 /* Cursor column                */
extern  int     thisflag;               /* Flags, this command          */
extern  int     lastflag;               /* Flags, last command          */
extern  int     curgoal;                /* Goal for C-P, C-N            */
extern  char	opertree[];		/* operate within this tree     */
extern  WINDOW  *curwp;                 /* Current window               */
extern  BUFFER  *curbp;                 /* Current buffer               */
extern  WINDOW  *wheadp;                /* Head of list of windows      */
extern  BUFFER  *bheadp;                /* Head of list of buffers      */
extern  BUFFER  *blistp;                /* Buffer for C-X C-B           */

extern  BUFFER  *bfind();               /* Lookup a buffer by name      */
extern  WINDOW  *wpopup();              /* Pop up window creation       */
extern  LINE    *lalloc();              /* Allocate a line              */
extern  int	 km_popped;		/* menu popped up               */
#ifdef	MOUSE
extern	MENUITEM menuitems[];		/* key labels and functions */
extern	MENUITEM *mfunc;		/* single generic function  */
extern	mousehandler_t mtrack;		/* func used to track the mouse */
#endif	/* MOUSE */

#if	defined(TERMCAP) || defined(TERMINFO) || defined(VMS)
extern	KBESC_T *pico_kbesc;		/* keyboard esc sequence trie   */
#endif /* TERMCAP/TERMINFO/VMS */

#endif /* maindef */

/* terminal table defined only in TERM.C */

#ifndef	termdef
#if	defined(VMS) && !defined(__ALPHA)
globalref
#else
extern
#endif /* VMS */
       TERM    term;                   /* Terminal information.        */
#endif /* termdef */

#endif	/* EDEF_H */
