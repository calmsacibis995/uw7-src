#if !defined(lint) && !defined(DOS)
static char rcsid[] = "$Id$";
#endif
/*----------------------------------------------------------------------

            T H E    P I N E    M A I L   S Y S T E M

   Laurence Lundblade and Mike Seibel
   Networks and Distributed Computing
   Computing and Communications
   University of Washington
   Administration Builiding, AG-44
   Seattle, Washington, 98195, USA
   Internet: lgl@CAC.Washington.EDU
             mikes@CAC.Washington.EDU

   Please address all bugs and comments to "pine-bugs@cac.washington.edu"


   Pine and Pico are registered trademarks of the University of Washington.
   No commercial use of these trademarks may be made without prior written
   permission of the University of Washington.

   Pine, Pico, and Pilot software and its included text are Copyright
   1989-1996 by the University of Washington.

   The full text of our legal notices is contained in the file called
   CPYRIGHT, included with this distribution.


   Pine is in part based on The Elm Mail System:
    ***********************************************************************
    *  The Elm Mail System  -  Revision: 2.13                             *
    *                                                                     *
    * 			Copyright (c) 1986, 1987 Dave Taylor              *
    * 			Copyright (c) 1988, 1989 USENET Community Trust   *
    ***********************************************************************
 

  ----------------------------------------------------------------------*/

/*======================================================================
       ttyout.c
       Routines for painting the screen
          - figure out what the terminal type is
          - deal with screen size changes
          - save special output sequences
          - the usual screen clearing, cursor addressing and scrolling


     This library gives programs the ability to easily access the
     termcap information and write screen oriented and raw input
     programs.  The routines can be called as needed, except that
     to use the cursor / screen routines there must be a call to
     InitScreen() first.  The 'Raw' input routine can be used
     independently, however. (Elm comment)

     Not sure what the original source of this code was. It got to be
     here as part of ELM. It has been changed significantly from the
     ELM version to be more robust in the face of inconsistent terminal
     autowrap behaviour. Also, the unused functions were removed, it was
     made to pay attention to the window size, and some code was made nicer
     (in my opinion anyways). It also outputs the terminal initialization
     strings and provides for minimal scrolling and detects terminals
     with out enough capabilities. (Pine comment, 1990)


This code used to pay attention to the "am" auto margin and "xn"
new line glitch fields, but they were so often incorrect because many
terminals can be configured to do either that we've taken it out. It
now assumes it dosn't know where the cursor is after outputing in the
80th column.
*/

#ifdef OS2
#define INCL_BASE
#define INCL_VIO
#define INCL_DOS
#define INCL_NOPM
#include <os2.h>
#undef ADDRESS
#endif

#include "headers.h"

#define FARAWAY 1000
#define	PUTLINE_BUFLEN	256

static int   _lines, _columns;
static int   _line  = FARAWAY;
static int   _col   = FARAWAY;
static int   _in_inverse;


#if	!(defined(DOS) ||defined(OS2))
/* Beginning of giant ifdef to switch between UNIX and DOS */

#ifdef _
#include <sys/tty.h>
#endif


/*
 * Internal prototypes
 */
static void moveabsolute PROTO((int, int));
static void CursorUp PROTO((int));
static void CursorDown PROTO((int));
static void CursorLeft PROTO((int));
static void CursorRight PROTO((int));


static char *_clearscreen, *_moveto, *_up, *_down, *_right, *_left,
            *_setinverse, *_clearinverse,
            *_setunderline, *_clearunderline,
            *_setbold,     *_clearbold,
            *_cleartoeoln, *_cleartoeos,
            *_startinsert, *_endinsert, *_insertchar, *_deletechar,
            *_deleteline, *_insertline,
            *_scrollregion, *_scrollup, *_scrolldown,
            *_termcap_init, *_termcap_end;
char  term_name[40];
#ifndef USE_TERMINFO
static char  _terminal[1024];         /* Storage for terminal entry */
static char  _capabilities[1024];     /* String for cursor motion */
static char *ptr = _capabilities;     /* for buffering         */

char  *tgetstr();     		       /* Get termcap capability */

#endif

static enum  {NoScroll,UseScrollRegion,InsertDelete} _scrollmode;

char  *tgoto();				/* and the goto stuff    */



/*----------------------------------------------------------------------
      Initialize the screen for output, set terminal type, etc

   Args: tt -- Pointer to variable to store the tty output structure.

 Result:  terminal size is discovered and set pine state
          termcap entry is fetched and stored in local variables
          make sure terminal has adequate capabilites
          evaluate scrolling situation
          returns status of indicating the state of the screen/termcap entry

      Returns:
        -1 indicating no terminal name associated with this shell,
        -2..-n  No termcap for this terminal type known
	-3 Can't open termcap file 
        -4 Terminal not powerful enough - missing clear to eoln or screen
	                                       or cursor motion

  ----*/
int
config_screen(tt, kbesc)
     struct ttyo **tt;
     KBESC_T     **kbesc;
{
    struct ttyo *ttyo;
    char *getenv();
    char *_ku, *_kd, *_kl, *_kr,
	 *_kppu, *_kppd, *_kphome, *_kpend, *_kpdel,
	 *_kf1, *_kf2, *_kf3, *_kf4, *_kf5, *_kf6,
	 *_kf7, *_kf8, *_kf9, *_kf10, *_kf11, *_kf12;

#ifdef USE_TERMINFO
    int setupterm();
    char *tigetstr();
    char *ttnm;
    int err;

    *tt = NULL;
    ttnm = getenv ("TERM");
    if (!ttnm) return -1;
    strcpy (term_name, ttnm);
    setupterm (term_name, 1 /* (ignored) */, &err);
    switch (err) {
    case -1 : return -3;
    case 0 : return -2;
    }

    ttyo = (struct ttyo *)fs_get(sizeof (struct ttyo));

    _line  =  0;		/* where are we right now?? */
    _col   =  0;		/* assume zero, zero...     */

    /* load in all those pesky values */
    _clearscreen       = tigetstr("clear");
    _moveto            = tigetstr("cup");
    _up                = tigetstr("cuu1");
    _down              = tigetstr("cud1");
    _right             = tigetstr("cuf1");
    _left              = tigetstr("cub1");
    _setinverse        = tigetstr("smso");
    _clearinverse      = tigetstr("rmso");
    _setunderline      = tigetstr("smul");
    _clearunderline    = tigetstr("rmul");
    _setbold           = tigetstr("bold");
    _clearbold         = tigetstr("sgr0");
    _cleartoeoln       = tigetstr("el");
    _cleartoeos        = tigetstr("ed");
    _deletechar        = tigetstr("dch1");
    _insertchar        = tigetstr("ich1");
    _startinsert       = tigetstr("smir");
    _endinsert         = tigetstr("rmir");
    _deleteline        = tigetstr("dl1");
    _insertline        = tigetstr("il1");
    _scrollregion      = tigetstr("csr");
    _scrolldown        = tigetstr("ind");
    _scrollup          = tigetstr("ri");
    _termcap_init      = tigetstr("smcup");
    _termcap_end       = tigetstr("rmcup");
    _lines	       = tigetnum("lines");
    _columns	       = tigetnum("cols");
    _ku                = tigetstr("kcuu1");
    _kd                = tigetstr("kcud1");
    _kl                = tigetstr("kcub1");
    _kr                = tigetstr("kcuf1");
    _kppu              = tigetstr("kpp");
    _kppd              = tigetstr("knp");
    _kphome            = tigetstr("khome");
    _kpend             = tigetstr("kend");
    _kpdel             = tigetstr("kdch1");
    _kf1               = tigetstr("kf1");
    _kf2               = tigetstr("kf2");
    _kf3               = tigetstr("kf3");
    _kf4               = tigetstr("kf4");
    _kf5               = tigetstr("kf5");
    _kf6               = tigetstr("kf6");
    _kf7               = tigetstr("kf7");
    _kf8               = tigetstr("kf8");
    _kf9               = tigetstr("kf9");
    _kf10              = tigetstr("kf10");
    _kf11              = tigetstr("kf11");
    _kf12              = tigetstr("kf12");
#else
    char *ttnm;			/* tty name */
    int   tgetent(),		/* get termcap entry */
          err;

    *tt = NULL;
    if (!(ttnm = getenv("TERM")) || !strcpy(term_name, ttnm))
      return(-1);

    if ((err = tgetent(_terminal, term_name)) != 1)
    	return(err-2);

    ttyo = (struct ttyo *)fs_get(sizeof (struct ttyo));

    _line  =  0;		/* where are we right now?? */
    _col   =  0;		/* assume zero, zero...     */

    /* load in all those pesky values */
    ptr                = _capabilities;
    _clearscreen       = tgetstr("cl", &ptr);
    _moveto            = tgetstr("cm", &ptr);
    _up                = tgetstr("up", &ptr);
    _down              = tgetstr("do", &ptr);
    _right             = tgetstr("nd", &ptr);
    _left              = tgetstr("bs", &ptr);
    _setinverse        = tgetstr("so", &ptr);
    _clearinverse      = tgetstr("se", &ptr);
    _setunderline      = tgetstr("us", &ptr);
    _clearunderline    = tgetstr("ue", &ptr);
    _setbold           = tgetstr("md", &ptr);
    _clearbold         = tgetstr("me", &ptr);
    _cleartoeoln       = tgetstr("ce", &ptr);
    _cleartoeos        = tgetstr("cd", &ptr);
    _deletechar        = tgetstr("dc", &ptr);
    _insertchar        = tgetstr("ic", &ptr);
    _startinsert       = tgetstr("im", &ptr);
    _endinsert         = tgetstr("ei", &ptr);
    _deleteline        = tgetstr("dl", &ptr);
    _insertline        = tgetstr("al", &ptr);
    _scrollregion      = tgetstr("cs", &ptr);
    _scrolldown        = tgetstr("sf", &ptr);
    _scrollup          = tgetstr("sr", &ptr);
    _termcap_init      = tgetstr("ti", &ptr);
    _termcap_end       = tgetstr("te", &ptr);
    _lines	       = tgetnum("li");
    _columns	       = tgetnum("co");
    _ku                = tgetstr("ku", &ptr);
    _kd                = tgetstr("kd", &ptr);
    _kl                = tgetstr("kl", &ptr);
    _kr                = tgetstr("kr", &ptr);
    _kppu              = tgetstr("kP", &ptr);
    _kppd              = tgetstr("kN", &ptr);
    _kphome            = tgetstr("kh", &ptr);
    _kpend             = tgetstr("kE", &ptr);
    _kpdel             = tgetstr("kD", &ptr);
    _kf1               = tgetstr("k1", &ptr);
    _kf2               = tgetstr("k2", &ptr);
    _kf3               = tgetstr("k3", &ptr);
    _kf4               = tgetstr("k4", &ptr);
    _kf5               = tgetstr("k5", &ptr);
    _kf6               = tgetstr("k6", &ptr);
    _kf7               = tgetstr("k7", &ptr);
    _kf8               = tgetstr("k8", &ptr);
    _kf9               = tgetstr("k9", &ptr);
    if((_kf10          = tgetstr("k;", &ptr)) == NULL)
      _kf10            = tgetstr("k0", &ptr);
    _kf11              = tgetstr("F1", &ptr);
    _kf12              = tgetstr("F2", &ptr);
#endif

    if(_lines == -1) {
	char *el;
	int   ll;

        /* tgetnum failed, try $LINES */
	el = getenv("LINES");
	if(el && (ll = atoi(el)) > 0)
	  _lines = ll;
	else
          _lines = DEFAULT_LINES_ON_TERMINAL;
    }
    if(_columns == -1) {
	char *ec;
	int   cc;

        /* tgetnum failed, try $COLUMNS */
	ec = getenv("COLUMNS");
	if(ec && (cc = atoi(ec)) > 0)
          _columns = cc;
	else
          _columns = DEFAULT_COLUMNS_ON_TERMINAL;
    }

    get_windsize(ttyo);

    ttyo->header_rows = 2;
    ttyo->footer_rows = 3;

    /*---- Make sure this terminal has the capability.
        All we need is cursor address, clear line, and 
        reverse video.
      ---*/
    if(_moveto == NULL || _cleartoeoln == NULL ||
       _setinverse == NULL || _clearinverse == NULL) {
          return(-4);
    }

    dprint(1, (debugfile, "Terminal type: %s\n", term_name));

    /*------ Figure out scrolling mode -----*/
    if(_scrollregion != NULL && _scrollregion[0] != '\0' &&
    	  _scrollup != NULL && _scrollup[0] != '\0'){
        _scrollmode = UseScrollRegion;
    } else if(_insertline != NULL && _insertline[0] != '\0' &&
       _deleteline != NULL && _deleteline[0] != '\0') {
        _scrollmode = InsertDelete;
    } else {
        _scrollmode = NoScroll;
    }
    dprint(7, (debugfile, "Scroll mode: %s\n",
               _scrollmode==NoScroll ? "No Scroll" :
               _scrollmode==InsertDelete ? "InsertDelete" : "Scroll Regions"));


    if (!_left) {
    	_left = "\b";
    }

    *tt = ttyo;

#ifndef TERMCAP_WINS
    /*
     * Add default keypad sequences to the trie.
     * Since these come first, they will override any conflicting termcap
     * or terminfo escape sequences defined below.  An escape sequence is
     * considered conflicting if one is a prefix of the other.
     * So, without TERMCAP_WINS, there will likely be some termcap/terminfo
     * escape sequences that don't work, because they conflict with default
     * sequences defined here.
     */
    setup_dflt_esc_seq(kbesc);
#endif /* !TERMCAP_WINS */

    /*
     * add termcap/info escape sequences to the trie...
     */

    if(_ku != NULL && _kd != NULL && _kl != NULL && _kr != NULL){
	kpinsert(kbesc, _ku, KEY_UP);
	kpinsert(kbesc, _kd, KEY_DOWN);
	kpinsert(kbesc, _kl, KEY_LEFT);
	kpinsert(kbesc, _kr, KEY_RIGHT);
    }

    if(_kppu != NULL && _kppd != NULL){
	kpinsert(kbesc, _kppu, KEY_PGUP);
	kpinsert(kbesc, _kppd, KEY_PGDN);
    }

    kpinsert(kbesc, _kphome, KEY_HOME);
    kpinsert(kbesc, _kpend,  KEY_END);
    kpinsert(kbesc, _kpdel,  KEY_DEL);

    kpinsert(kbesc, _kf1,  PF1);
    kpinsert(kbesc, _kf2,  PF2);
    kpinsert(kbesc, _kf3,  PF3);
    kpinsert(kbesc, _kf4,  PF4);
    kpinsert(kbesc, _kf5,  PF5);
    kpinsert(kbesc, _kf6,  PF6);
    kpinsert(kbesc, _kf7,  PF7);
    kpinsert(kbesc, _kf8,  PF8);
    kpinsert(kbesc, _kf9,  PF9);
    kpinsert(kbesc, _kf10, PF10);
    kpinsert(kbesc, _kf11, PF11);
    kpinsert(kbesc, _kf12, PF12);

#ifdef TERMCAP_WINS
    /*
     * Add default keypad sequences to the trie.
     * Since these come after the termcap/terminfo escape sequences above,
     * the termcap/info sequences will override any conflicting default
     * escape sequences defined here.
     * So, with TERMCAP_WINS, some of the default sequences will be missing.
     * This means that you'd better get all of your termcap/terminfo entries
     * correct if you define TERMCAP_WINS.
     */
    setup_dflt_esc_seq(kbesc);
#endif /* TERMCAP_WINS */

    return(0);
}



/*----------------------------------------------------------------------
   Initialize the screen with the termcap string 
  ----*/
void
init_screen()
{
    if(_termcap_init)			/* init using termcap's rule */
      tputs(_termcap_init, 1, outchar);

    /* and make sure there are no scrolling surprises! */
    BeginScroll(0, ps_global->ttyo->screen_rows - 1);
    /* and make sure icon text starts out consistent */
    icon_text(NULL);
    fflush(stdout);
}
        



/*----------------------------------------------------------------------
       Get the current window size
  
   Args: ttyo -- pointer to structure to store window size in

  NOTE: we don't override the given values unless we know better
 ----*/
int
get_windsize(ttyo)
struct ttyo *ttyo;     
{
#ifdef RESIZING 
    struct winsize win;

    /*
     * Get the window size from the tty driver.  If we can't fish it from
     * stdout (pine's output is directed someplace else), try stdin (which
     * *must* be associated with the terminal; see init_tty_driver)...
     */
    if(ioctl(1, TIOCGWINSZ, &win) >= 0			/* 1 is stdout */
	|| ioctl(0, TIOCGWINSZ, &win) >= 0){		/* 0 is stdin */
	if(win.ws_row)
	  _lines = min(win.ws_row, MAX_SCREEN_ROWS);

	if(win.ws_col)
	  _columns  = min(win.ws_col, MAX_SCREEN_COLS);

        dprint(2, (debugfile, "new win size -----<%d %d>------\n",
                   _lines, _columns));
    }
    else
      /* Depending on the OS, the ioctl() may have failed because
	 of a 0 rows, 0 columns setting.  That happens on DYNIX/ptx 1.3
	 (with a kernel patch that happens to involve the negotiation
	 of window size in the telnet streams module.)  In this case
	 the error is EINVARG.  Leave the default settings. */
      dprint(1, (debugfile, "ioctl(TIOCWINSZ) failed :%s\n",
		 error_description(errno)));
#endif

    ttyo->screen_cols = min(_columns, MAX_SCREEN_COLS);
    ttyo->screen_rows = min(_lines, MAX_SCREEN_ROWS);
    return(0);
}


/*----------------------------------------------------------------------
      End use of the screen.
      Print status message, if any.
      Flush status messages.
  ----*/
void
end_screen(message)
    char *message;
{
    int footer_rows_was_one = 0;

    dprint(9, (debugfile, "end_screen called\n"));

    if(FOOTER_ROWS(ps_global) == 1){
	footer_rows_was_one++;
	FOOTER_ROWS(ps_global) = 3;
	mark_status_unknown();
    }

    flush_status_messages(1);
    blank_keymenu(_lines - 2, 0);
    MoveCursor(_lines - 2, 0);
    if(_termcap_end != NULL)
      tputs(_termcap_end, 1, outchar);

    if(message){
	printf("%s\r\n", message);
    }

    fflush(stdout);

    if(footer_rows_was_one){
	FOOTER_ROWS(ps_global) = 1;
	mark_status_unknown();
    }
}
    


/*----------------------------------------------------------------------
     Clear the terminal screen

 Result: The screen is cleared
         internal cursor position set to 0,0
  ----*/
void
ClearScreen()
{
    _line = 0;	/* clear leaves us at top... */
    _col  = 0;

    if(ps_global->in_init_seq)
      return;

    mark_status_unknown();
    mark_keymenu_dirty();
    mark_titlebar_dirty();

    if(!_clearscreen){
	ClearLines(0, _lines-1);
        MoveCursor(0, 0);
    }
    else{
	tputs(_clearscreen, 1, outchar);
        moveabsolute(0, 0);  /* some clearscreens don't move correctly */
    }
}


/*----------------------------------------------------------------------
            Internal move cursor to absolute position

  Args: col -- column to move cursor to
        row -- row to move cursor to

 Result: cursor is moved (variables, not updates)
  ----*/

static void
moveabsolute(col, row)
{

	char *stuff, *tgoto();

	stuff = tgoto(_moveto, col, row);
	tputs(stuff, 1, outchar);
}


/*----------------------------------------------------------------------
        Move the cursor to the row and column number
  Args:  row number
         column number

 Result: Cursor moves
         internal position updated
  ----*/
void
MoveCursor(row, col)
     int row, col;
{
    /** move cursor to the specified row column on the screen.
        0,0 is the top left! **/

    int scrollafter = 0;

    /* we don't want to change "rows" or we'll mangle scrolling... */

    if(ps_global->in_init_seq)
      return;

    if (col < 0)
      col = 0;
    if (col >= ps_global->ttyo->screen_cols)
      col = ps_global->ttyo->screen_cols - 1;
    if (row < 0)
      row = 0;
    if (row > ps_global->ttyo->screen_rows) {
      if (col == 0)
        scrollafter = row - ps_global->ttyo->screen_rows;
      row = ps_global->ttyo->screen_rows;
    }

    if (!_moveto)
    	return;

    if (row == _line) {
      if (col == _col)
        return;				/* already there! */

      else if (abs(col - _col) < 5) {	/* within 5 spaces... */
        if (col > _col && _right)
          CursorRight(col - _col);
        else if (col < _col &&  _left)
          CursorLeft(_col - col);
        else
          moveabsolute(col, row);
      }
      else 		/* move along to the new x,y loc */
        moveabsolute(col, row);
    }
    else if (col == _col && abs(row - _line) < 5) {
      if (row < _line && _up)
        CursorUp(_line - row);
      else if (_line > row && _down)
        CursorDown(row - _line);
      else
        moveabsolute(col, row);
    }
    else if (_line == row-1 && col == 0) {
      putchar('\n');	/* that's */
      putchar('\r');	/*  easy! */
    }
    else 
      moveabsolute(col, row);

    _line = row;	/* to ensure we're really there... */
    _col  = col;

    if (scrollafter) {
      while (scrollafter--) {
        putchar('\n');
        putchar('\r');

      }
    }

    return;
}



/*----------------------------------------------------------------------
         Newline, move the cursor to the start of next line

 Result: Cursor moves
  ----*/
void
NewLine()
{
   /** move the cursor to the beginning of the next line **/

    Writechar('\n', 0);
    Writechar('\r', 0);
}



/*----------------------------------------------------------------------
        Move cursor up n lines with terminal escape sequence
 
   Args:  n -- number of lines to go up

 Result: cursor moves, 
         internal position updated

 Only for ttyout use; not outside callers
  ----*/
static void
CursorUp(n)
int n;
{
	/** move the cursor up 'n' lines **/
	/** Calling function must check that _up is not null before calling **/

    _line = (_line-n > 0? _line - n: 0);	/* up 'n' lines... */

    while (n-- > 0)
      tputs(_up, 1, outchar);
}



/*----------------------------------------------------------------------
        Move cursor down n lines with terminal escape sequence
 
    Arg: n -- number of lines to go down

 Result: cursor moves, 
         internal position updated

 Only for ttyout use; not outside callers
  ----*/
static void
CursorDown(n)
     int          n;
{
    /** move the cursor down 'n' lines **/
    /** Caller must check that _down is not null before calling **/

    _line = (_line+n < ps_global->ttyo->screen_rows ? _line + n
             : ps_global->ttyo->screen_rows);
                                               /* down 'n' lines... */

    while (n-- > 0)
    	tputs(_down, 1, outchar);
}



/*----------------------------------------------------------------------
        Move cursor left n lines with terminal escape sequence
 
   Args:  n -- number of lines to go left

 Result: cursor moves, 
         internal position updated

 Only for ttyout use; not outside callers
  ----*/
static void 
CursorLeft(n)
int n;
{
    /** move the cursor 'n' characters to the left **/
    /** Caller must check that _left is not null before calling **/

    _col = (_col - n> 0? _col - n: 0);	/* left 'n' chars... */

    while (n-- > 0)
      tputs(_left, 1, outchar);
}


/*----------------------------------------------------------------------
        Move cursor right n lines with terminal escape sequence
 
   Args:  number of lines to go right

 Result: cursor moves, 
         internal position updated

 Only for ttyout use; not outside callers
  ----*/
static void 
CursorRight(n)
int n;
{
    /** move the cursor 'n' characters to the right (nondestructive) **/
    /** Caller must check that _right is not null before calling **/

    _col = (_col+n < ps_global->ttyo->screen_cols? _col + n :
             ps_global->ttyo->screen_cols);	/* right 'n' chars... */

    while (n-- > 0)
      tputs(_right, 1, outchar);

}



/*----------------------------------------------------------------------
       Start painting inverse on the screen
 
 Result: escape sequence to go into inverse is output
         returns 1 if it was done, 0 if not.
  ----*/
int
StartInverse()
{
    /** set inverse video mode **/

    if (!_setinverse)
    	return(0);
    
    if(_in_inverse)
      return(1);

    _in_inverse = 1;
    tputs(_setinverse, 1, outchar);
    return(1);
}



/*----------------------------------------------------------------------
      End painting inverse on the screen
 
 Result: escape sequence to go out of inverse is output
         returns 1 if it was done, 0 if not.
  ----------------------------------------------------------------------*/
void
EndInverse()
{
    /** compliment of startinverse **/

    if (!_clearinverse)
    	return;

    if(_in_inverse){
	_in_inverse = 0;
	tputs(_clearinverse, 1, outchar);
    }
}


int
StartUnderline()
{
    if (!_setunderline)
    	return(0);

    tputs(_setunderline, 1, outchar);
    return(1);
}


void
EndUnderline()
{
    if (!_clearunderline)
    	return;

    tputs(_clearunderline, 1, outchar);
}

int
StartBold()
{
    if (!_setbold)
    	return(0);

    tputs(_setbold, 1, outchar);
    return(1);
}

void
EndBold()
{
    if (!_clearbold)
    	return;

    tputs(_clearbold, 1, outchar);
}



/*----------------------------------------------------------------------
       Insert character on screen pushing others right

   Args: c --  character to insert

 Result: charcter is inserted if possible
         return -1 if it can't be done
  ----------------------------------------------------------------------*/
InsertChar(c)
     int c;
{
    if(_insertchar != NULL && *_insertchar != '\0') {
	tputs(_insertchar, 1, outchar);
	Writechar(c, 0);
    } else if(_startinsert != NULL && *_startinsert != '\0') {
	tputs(_startinsert, 1, outchar);
	Writechar(c, 0);
	tputs(_endinsert, 1, outchar);
    } else {
	return(-1);
    }
    return(0);
}



/*----------------------------------------------------------------------
         Delete n characters from line, sliding rest of line left

   Args: n -- number of characters to delete


 Result: characters deleted on screen
         returns -1 if it wasn't done
  ----------------------------------------------------------------------*/
DeleteChar(n)
     int n;
{
    if(_deletechar == NULL || *_deletechar == '\0')
      return(-1);

    while(n) {
	tputs(_deletechar, 1, outchar);
	n--;
    }
    return(0);
}



/*----------------------------------------------------------------------
  Go into scrolling mode, that is set scrolling region if applicable

   Args: top    -- top line of region to scroll
         bottom -- bottom line of region to scroll
	 (These are zero-origin numbers)

 Result: either set scrolling region or
         save values for later scrolling
         returns -1 if we can't scroll

 Unfortunately this seems to leave the cursor in an unpredictable place
 at least the manuals don't say where, so we force it here.
-----*/
static int __t, __b;

BeginScroll(top, bottom)
     int top, bottom;
{
    char *stuff;

    if(_scrollmode == NoScroll)
      return(-1);

    __t = top;
    __b = bottom;
    if(_scrollmode == UseScrollRegion){
        stuff = tgoto(_scrollregion, bottom, top);
        tputs(stuff, 1, outchar);
        /*-- a location  very far away to force a cursor address --*/
        _line = FARAWAY;
        _col  = FARAWAY;
    }
    return(0);
}



/*----------------------------------------------------------------------
   End scrolling -- clear scrolling regions if necessary

 Result: Clear scrolling region on terminal
  -----*/
void
EndScroll()
{
    if(_scrollmode == UseScrollRegion && _scrollregion != NULL){
	/* Use tgoto even though we're not cursor addressing because
           the format of the capability is the same.
         */
        char *stuff = tgoto(_scrollregion, ps_global->ttyo->screen_rows -1, 0);
	tputs(stuff, 1, outchar);
        /*-- a location  very far away to force a cursor address --*/
        _line = FARAWAY;
        _col  = FARAWAY;
    }
}


/* ----------------------------------------------------------------------
    Scroll the screen using insert/delete or scrolling regions

   Args:  lines -- number of lines to scroll, positive forward

 Result: Screen scrolls
         returns 0 if scroll succesful, -1 if not

 positive lines goes foward (new lines come in at bottom
 Leaves cursor at the place to insert put new text

 0,0 is the upper left
 -----*/
ScrollRegion(lines)
    int lines;
{
    int l;

    if(lines == 0)
      return(0);

    if(_scrollmode == UseScrollRegion) {
	if(lines > 0) {
	    MoveCursor(__b, 0);
	    for(l = lines ; l > 0 ; l--)
	      tputs((_scrolldown == NULL || _scrolldown[0] =='\0') ? "\n" :
		    _scrolldown, 1, outchar);
	} else {
	    MoveCursor(__t, 0);
	    for(l = -lines; l > 0; l--)
	      tputs(_scrollup, 1, outchar);
	}
    } else if(_scrollmode == InsertDelete) {
	if(lines > 0) {
	    MoveCursor(__t, 0);
	    for(l = lines; l > 0; l--) 
	      tputs(_deleteline, 1, outchar);
	    MoveCursor(__b, 0);
	    for(l = lines; l > 0; l--) 
	      tputs(_insertline, 1, outchar);
	} else {
	    for(l = -lines; l > 0; l--) {
	        MoveCursor(__b, 0);
	        tputs(_deleteline, 1, outchar);
		MoveCursor(__t, 0);
		tputs(_insertline, 1, outchar);
	    }
	}
    } else {
	return(-1);
    }
    fflush(stdout);
    return(0);
}



/*----------------------------------------------------------------------
    Write a character to the screen, keeping track of cursor position

   Args: ch -- character to output

 Result: character output
         cursor position variables updated
  ----*/
Writechar(ch, new_esc_len)
     register unsigned int ch;
     int      new_esc_len;
{
    register int nt;
    static   int esc_len = 0;

    if(ps_global->in_init_seq				/* silent */
       || (F_ON(F_BLANK_KEYMENU, ps_global)		/* or bottom, */
	   && !esc_len					/* right cell */
	   && _line + 1 == ps_global->ttyo->screen_rows
	   && _col + 1 == ps_global->ttyo->screen_cols))
      return;

    if(!iscntrl(ch & 0x7f)){
	putchar(ch);
	if(esc_len > 0)
	  esc_len--;
	else
	  _col++;
    }
    else{
	switch(ch){
	  case LINE_FEED:
	    /*-- Don't have to watch out for auto wrap or newline glitch
	      because we never let it happen. See below
	      ---*/
	    putchar('\n');
	    _line = min(_line+1,ps_global->ttyo->screen_rows);
	    esc_len = 0;
	    break;

	  case RETURN :		/* move to column 0 */
	    putchar('\r');
	    _col = 0;
	    esc_len = 0;
	    break;

	  case BACKSPACE :	/* move back a space if not in column 0 */
	    if(_col != 0) {
		putchar('\b');
		_col--;
	    }			/* else BACKSPACE does nothing */

	    break;

	  case BELL :		/* ring the bell but don't advance _col */
	    putchar(ch);
	    break;

	  case TAB :		/* if a tab, output it */
	    do			/* BUG? ignores tty driver's spacing */
	      putchar(' ');
	    while(_col < ps_global->ttyo->screen_cols - 1
		  && ((++_col)&0x07) != 0);
	    break;

	  case ESCAPE :
	    /* If we're outputting an escape here, it may be part of an iso2022
	       escape sequence in which case take up no space on the screen.
	       Unfortunately such sequences are variable in length.
	       */
	    esc_len = new_esc_len - 1;
	    putchar(ch);
	    break;

	  default :		/* Change remaining control characters to ? */
	    if(F_ON(F_PASS_CONTROL_CHARS, ps_global))
	      putchar(ch);
	    else
	      putchar('?');

	    if(esc_len > 0)
	      esc_len--;
	    else
	      _col++;

	    break;
	}
    }


    /* Here we are at the end of the line. We've decided to make no
       assumptions about how the terminal behaves at this point.
       What can happen now are the following
           1. Cursor is at start of next line, and next character will
              apear there. (autowrap, !newline glitch)
           2. Cursor is at start of next line, and if a newline is output
              it'll be ignored. (autowrap, newline glitch)
           3. Cursor is still at end of line and next char will apear
              there over the top of what is there now (no autowrap).
       We ignore all this and force the cursor to the next line, just 
       like case 1. A little expensive but worth it to avoid problems
       with terminals configured so they don't match termcap
       */
    if(_col == ps_global->ttyo->screen_cols) {
        _col = 0;
        if(_line + 1 < ps_global->ttyo->screen_rows)
	  _line++;

	moveabsolute(_col, _line);
    }

    return(0);
}



/*----------------------------------------------------------------------
       Write string to screen at current cursor position

   Args: string -- strings to be output

 Result: Line written to the screen
  ----*/
void
Write_to_screen(string)				/* UNIX */
      register char *string; 
{
    while(*string)
      Writechar((unsigned char) *string++, 0);
}



/*----------------------------------------------------------------------
    Clear screen to end of line on current line

 Result: Line is cleared
  ----*/
void
CleartoEOLN()
{
    if(!_cleartoeoln)
      return;

    tputs(_cleartoeoln, 1, outchar);
}



/*----------------------------------------------------------------------
     Clear screen to end of screen from current point

 Result: screen is cleared
  ----*/
CleartoEOS()
{
    if(!_cleartoeos){
        CleartoEOLN();
	ClearLines(_line, _lines-1);
    }
    else
      tputs(_cleartoeos, 1, outchar);
}



/*----------------------------------------------------------------------
     function to output character used by termcap

   Args: c -- character to output

 Result: character output to screen via stdio
  ----*/
void
outchar(c)
int c;
{
	/** output the given character.  From tputs... **/
	/** Note: this CANNOT be a macro!              **/

	putc((unsigned char)c, stdout);
}



/*----------------------------------------------------------------------
     function to output string such that it becomes icon text

   Args: s -- string to write

 Result: string indicated become our "icon" text
  ----*/
void
icon_text(s)
    char *s;
{
    if(F_ON(F_ENABLE_XTERM_NEWMAIL, ps_global) && getenv("DISPLAY")){
	fputs("\033]1;", stdout);
	fputs((s) ? s : ps_global->pine_name, stdout);
	fputs("\007", stdout);
	fflush(stdout);
    }
}



#else /* DOS && OS2 */  /* Middle of giant ifdef for DOS & OS2 drivers */


/*----------------------------------------------------------------------
      Initialize the screen for output, set terminal type, etc

 Input: TERM and TERMCAP environment variables
        Pointer to variable to store the tty output structure.

 Result:  terminal size is discovered and set pine state
          termcap entry is fetched and stored in local variables
          make sure terminal has adequate capabilites
          evaluate scrolling situation
          returns status of indicating the state of the screen/termcap entry
  ----------------------------------------------------------------------*/
void
init_screen()					/* DOS */
{
    return;					/* NO OP */
}



/*----------------------------------------------------------------------
      End use of the screen. 
  ----------------------------------------------------------------------*/
void
end_screen(message)				/* DOS */
    char *message;
{
    extern void exit_text_mode();
    int footer_rows_was_one = 0;

    if(FOOTER_ROWS(ps_global) == 1){
	footer_rows_was_one++;
	FOOTER_ROWS(ps_global) = 3;
	mark_status_unknown();
    }

    flush_status_messages(1);

    blank_keymenu(_lines - 2, 0);

    if(message){
	StartInverse();
	PutLine0(_lines - 2, 0, message);
    }
    
    EndInverse();

    MoveCursor(_lines - 1, 0);
#ifndef _WINDOWS
    exit_text_mode(NULL);
#endif
    ibmclose();
    if(footer_rows_was_one){
	FOOTER_ROWS(ps_global) = 1;
	mark_status_unknown();
    }
}

#ifdef OS2
static void
get_dimen(int *width, int *length)
{
    VIOMODEINFO vm;
    vm.cb = sizeof vm;
    VioGetMode(&vm, 0);
    *width = vm.col;
    *length = vm.row;
}
#endif


/*----------------------------------------------------------------------
      Initialize the screen for output, set terminal type, etc

   Args: tt -- Pointer to variable to store the tty output structure.

 Result:  terminal size is discovered and set pine state
          termcap entry is fetched and stored in local variables
          make sure terminal has adequate capabilites
          evaluate scrolling situation
          returns status of indicating the state of the screen/termcap entry

      Returns:
        -1 indicating no terminal name associated with this shell,
        -2..-n  No termcap for this terminal type known
	-3 Can't open termcap file 
        -4 Terminal not powerful enough - missing clear to eoln or screen
	                                       or cursor motion

  ----*/
int
config_screen(tt, kbesc)			/* DOS */
     struct ttyo **tt;
     KBESC_T     **kbesc;
{
    struct ttyo *ttyo;
#ifndef _WINDOWS
    extern void enter_text_mode();
#endif

    _line  =  0;		/* where are we right now?? */
    _col   =  0;		/* assume zero, zero...     */

#ifdef _WINDOWS
    mswin_getscreensize(&_lines, &_columns);
    if (_lines > MAX_SCREEN_ROWS)
	_lines = MAX_SCREEN_ROWS;
    if (_columns > MAX_SCREEN_COLS)
	_columns = MAX_SCREEN_COLS;
#else
#ifdef OS2
    get_dimen(&_columns, &_lines);
#else
    _lines   = DEFAULT_LINES_ON_TERMINAL;
    _columns = DEFAULT_COLUMNS_ON_TERMINAL;
#endif
#endif
    ttyo = (struct ttyo *)fs_get(sizeof(struct ttyo));
    ttyo->screen_cols = _columns;
    ttyo->screen_rows = _lines ;
    ttyo->header_rows = 2;
    ttyo->footer_rows = 3;

#ifndef _WINDOWS
    enter_text_mode(NULL);
#endif
    ibmopen();

    *tt = ttyo;

    return(0);
}


    
/*----------------------------------------------------------------------
       Get the current window size
  
   Args: ttyo -- pointer to structure to store window size in

  NOTE: we don't override the given values unless we know better
 ----*/
int
get_windsize(ttyo)				/* DOS */
struct ttyo *ttyo;     
{
#ifdef _WINDOWS
    char	fontName[LF_FACESIZE+1];
    char	fontSize[12];
    char	fontStyle[64];
    char	windowPosition[32];
    int		newRows, newCols;
    
	    
    /* Get the new window parameters and update the 'pinerc' variables. */
    mswin_getwindow (fontName, fontSize, fontStyle, windowPosition);
    if(!ps_global->VAR_FONT_NAME
       || strucmp(ps_global->VAR_FONT_NAME, fontName))
      set_variable(V_FONT_NAME, fontName, 0);

    if(!ps_global->VAR_FONT_SIZE
       || strucmp(ps_global->VAR_FONT_SIZE, fontSize))
      set_variable(V_FONT_SIZE, fontSize, 0);

    if(!ps_global->VAR_FONT_STYLE
       || strucmp(ps_global->VAR_FONT_STYLE, fontStyle))
      set_variable(V_FONT_STYLE, fontStyle, 0);

    if(!ps_global->VAR_WINDOW_POSITION
       || strucmp(ps_global->VAR_WINDOW_POSITION, windowPosition))
      set_variable(V_WINDOW_POSITION, windowPosition, 0);

    mswin_getprintfont (fontName, fontSize, fontStyle);
    if(!ps_global->VAR_PRINT_FONT_NAME
       || strucmp(ps_global->VAR_PRINT_FONT_NAME, fontName))
      set_variable(V_PRINT_FONT_NAME, fontName, 0);

    if(!ps_global->VAR_PRINT_FONT_SIZE
       || strucmp(ps_global->VAR_PRINT_FONT_SIZE, fontSize))
      set_variable(V_PRINT_FONT_SIZE, fontSize, 0);

    if(!ps_global->VAR_PRINT_FONT_STYLE
       || strucmp(ps_global->VAR_PRINT_FONT_STYLE, fontStyle))
      set_variable(V_PRINT_FONT_STYLE, fontStyle, 0);

    /* Get new window size.  Compare to old.  The window may have just
     * moved, in which case we don't bother updating the size. */
    mswin_getscreensize(&newRows, &newCols);
    if (newRows == ttyo->screen_rows && newCols == ttyo->screen_cols)
	    return (NO_OP_COMMAND);

    /* True resize. */
    ttyo->screen_rows = newRows;
    ttyo->screen_cols = newCols;

    if (ttyo->screen_rows > MAX_SCREEN_ROWS)
	ttyo->screen_rows = MAX_SCREEN_ROWS;
    if (ttyo->screen_cols > MAX_SCREEN_COLS)
	ttyo->screen_cols = MAX_SCREEN_COLS;

    return(KEY_RESIZE);
#else
#ifdef OS2
    get_dimen(&_columns, &_lines);
#endif

    ttyo->screen_cols = _columns;
    ttyo->screen_rows = _lines;
    return(0);
#endif
}
    



/*----------------------------------------------------------------------
     Clear the terminal screen

 Input:  none

 Result: The screen is cleared
         internal cursor position set to 0,0
  ----------------------------------------------------------------------*/
void
ClearScreen()					/* DOS */
{
    _line = 0;			/* clear leaves us at top... */
    _col  = 0;

    if(ps_global->in_init_seq)
      return;

    mark_status_unknown();
    mark_keymenu_dirty();
    mark_titlebar_dirty();

    ibmmove(0, 0);
    ibmeeop();
}



/*----------------------------------------------------------------------
        Move the cursor to the row and column number
 Input:  row number
         column number

 Result: Cursor moves
         internal position updated
  ----------------------------------------------------------------------*/
void
MoveCursor(row, col)				/* DOS */
     int row, col;
{
    /** move cursor to the specified row column on the screen.
        0,0 is the top left! **/

    if(ps_global->in_init_seq)
      return;

    ibmmove(row, col);
    _line = row;
    _col = col;
}



/*----------------------------------------------------------------------
         Newline, move the cursor to the start of next line

 Input:  none

 Result: Cursor moves
  ----------------------------------------------------------------------*/
void
NewLine()					/* DOS */
{
    /** move the cursor to the beginning of the next line **/

    MoveCursor(_line+1, 0);
}




/*----------------------------------------------------------------------
       Start painting inverse on the screen
 
 Input:  none

 Result: escape sequence to go into inverse is output
         returns 1 if it was done, 0 if not.
  ----------------------------------------------------------------------*/
int
StartInverse()					/* DOS */
{
    if(_in_inverse)
      return(1);

    _in_inverse = 1;
    ibmrev(1);					/* libpico call */
    return(1);
}


/*----------------------------------------------------------------------
      End painting inverse on the screen
 
 Input:  none

 Result: escape sequence to go out of inverse is output
         returns 1 if it was done, 0 if not.
  ----------------------------------------------------------------------*/
void
EndInverse()					/* DOS */
{
    if(_in_inverse){
	_in_inverse = 0;
	ibmrev(0);				/* libpico call */
    }
}


/*
 * Character attriute stuff that could use some work
 * MS 92.05.18
 */

int
StartUnderline()				/* DOS */
{
    return(0);
}

void						/* DOS */
EndUnderline()
{
}

int
StartBold()					/* DOS */
{
    return(0);
}

void
EndBold()					/* DOS */
{
}


/*----------------------------------------------------------------------
       Insert character on screen pushing others right

 Input:  character to output
         termcap escape sequences

 Result: charcter is inserted if possible
         return -1 if it can't be done
  ----------------------------------------------------------------------*/
InsertChar(c)					/* DOS */
     int c;
{
#ifdef _WINDOWS
    mswin_inschar (c);
    return (0);
#else
    return(-1);
#endif
}


/*----------------------------------------------------------------------
         Delete n characters from line, sliding rest of line left

 Input:  number of characters to delete
         termcap escape sequences

 Result: characters deleted on screen
         returns -1 if it wasn't done
  ----------------------------------------------------------------------*/
DeleteChar(n)					/* DOS */
     int n;
{
    char c;
    int oc;				/* original column */

#ifdef _WINDOWS
    mswin_delchar ();
#else
    oc = _col;
/* cursor OFF */
    while(_col < 79){
	ibmmove(_line, _col+1);
    	readscrn(&c);
	ibmmove(_line, _col++);
	ibmputc(c);
    }
    ibmputc(' ');
    ibmmove(_line, oc);
/* cursor ON */
#endif
}



/*----------------------------------------------------------------------
  Go into scrolling mode, that is set scrolling region if applicable

 Input: top line of region to scroll
        bottom line of region to scroll

 Result: either set scrolling region or
         save values for later scrolling
         returns -1 if we can't scroll

 Unfortunately this seems to leave the cursor in an unpredictable place
 at least the manuals don't say were, so we force it here.
----------------------------------------------------------------------*/
static int __t, __b;
BeginScroll(top, bottom)			/* DOS */
     int top, bottom;
{
    __t = top;
    __b = bottom;
}

/*----------------------------------------------------------------------
   End scrolling -- clear scrolling regions if necessary

 Input:  none

 Result: Clear scrolling region on terminal
  ----------------------------------------------------------------------*/
void
EndScroll()					/* DOS */
{
}


/* ----------------------------------------------------------------------
    Scroll the screen using insert/delete or scrolling regions

 Input:  number of lines to scroll, positive forward

 Result: Screen scrolls
         returns 0 if scroll succesful, -1 if not

 positive lines goes foward (new lines come in at bottom
 Leaves cursor at the place to insert put new text

 0,0 is the upper left
 ----------------------------------------------------------------------*/
ScrollRegion(lines)				/* DOS */
    int lines;
{
    return(-1);
}


/*----------------------------------------------------------------------
    Write a character to the screen, keeping track of cursor position

 Input:  charater to write

 Result: character output
         cursor position variables updated
  ----------------------------------------------------------------------*/
Writechar(ch, new_esc_len)				/* DOS */
     register unsigned int ch;
     int      new_esc_len;
{
    register int nt;

    if(ps_global->in_init_seq)
      return(0);

    switch(ch){
      case LINE_FEED :
	_line = min(_line+1,ps_global->ttyo->screen_rows);
        _col =0;
        ibmmove(_line, _col);
	break;

      case RETURN :		/* move to column 0 */
	_col = 0;
        ibmmove(_line, _col);

      case BACKSPACE :		/* move back a space if not in column 0 */
	if(_col > 0)
          ibmmove(_line, --_col);

	break;
	
      case BELL :		/* ring the bell but don't advance _col */
	ibmbeep();		/* libpico call */
	break;

      case TAB:			/* if a tab, output it */
	do
	  ibmputc(' ');
	while(((++_col)&0x07) != 0);
	break;

      default:
	/*if some kind of control or  non ascii character change to a '?'*/
	if(iscntrl(ch & 0x7f))
	  ch = '?';

	ibmputc(ch);
	_col++;
    }

    if(_col == ps_global->ttyo->screen_cols) {
	  _col  = 0;
	  if(_line + 1 < ps_global->ttyo->screen_rows)
	    _line++;

	  ibmmove(_line, _col);
    }

    return(0);
}


/*----------------------------------------------------------------------
      Printf style write directly to the terminal at current position

 Input: printf style control string
        number of printf style arguments
        up to three printf style arguments

 Result: Line written to the screen
  ----------------------------------------------------------------------*/
void
Write_to_screen(string)				/* DOS */
      register char *string;
{
    if(ps_global->in_init_seq)
      return;

#ifdef _WINDOWS
    mswin_puts (string);
#else
    while(*string)
      Writechar(*string++, 0);
#endif
}



/*----------------------------------------------------------------------
    Clear screen to end of line on current line

 Input:  none

 Result: Line is cleared
  ----------------------------------------------------------------------*/
void
CleartoEOLN()					/* DOS */
{
    ibmeeol();					/* libpico call */
}


/*----------------------------------------------------------------------
          Clear screen to end of screen from current point

 Input: none

 Result: screen is cleared
  ----------------------------------------------------------------------*/
CleartoEOS()					/* DOS */
{
    ibmeeop();
}


/*----------------------------------------------------------------------
     function to output string such that it becomes icon text

   Args: s -- string to write

 Result: string indicated become our "icon" text
  ----*/
void
icon_text(s)					/* DOS */
    char *s;
{
#ifdef _WINDOWS
    mswin_newmailtext ((s) ? s : ps_global->pine_name);
#endif
    return;					/* NO OP */
}


#ifndef	_WINDOWS
/*
 * readscrn - DOS magic to read the character on the display at the
 *            current cursor position
 */
readscrn(c)					/* DOS */
char *c;
{
    ibmgetc(c);
}
#endif	/* !_WINDOWS */
#endif	/* DOS -- End of giant ifdef for DOS drivers */


/*
 * Generic tty output routines...
 */

/*----------------------------------------------------------------------
      Printf style output line to the screen at given position, 0 args

  Args:  x -- column position on the screen
         y -- row position on the screen
         line -- line of text to output

 Result: text is output
         cursor position is update
  ----*/
void
PutLine0(x, y, line)
    int            x,y;
    register char *line;
{
    MoveCursor(x,y);
    Write_to_screen(line);
}



/*----------------------------------------------------------------------
  Output line of length len to the display observing embedded attributes

 Args:  x      -- column position on the screen
        y      -- column position on the screen
        line   -- text to be output
        length -- length of text to be output

 Result: text is output
         cursor position is updated
  ----------------------------------------------------------------------*/
void
PutLine0n8b(x, y, line, length)
    int            x, y, length;
    register char *line;
{
    unsigned char c;

    MoveCursor(x,y);
    while(length-- && (c = (unsigned char)*line++))
      if(c == (unsigned char)TAG_EMBED && length){
	  length--;
	  switch(*line++){
	    case TAG_INVON :
	      StartInverse();
	      break;
	    case TAG_INVOFF :
	      EndInverse();
	      break;
	    case TAG_BOLDON :
	      StartBold();
	      break;
	    case TAG_BOLDOFF :
	      EndBold();
	      break;
	    case TAG_ULINEON :
	      StartUnderline();
	      break;
	    case TAG_ULINEOFF :
	      EndUnderline();
	      break;
	    /* default: other embedded vals are handles, just ignore 'em */
	  }					/* tag with handle, skip it */
      }	
      else if(c == '\033')			/* check for iso-2022 escape */
	Writechar(c, match_escapes(line));
      else
	Writechar(c, 0);
}


/*----------------------------------------------------------------------
      Printf style output line to the screen at given position, 1 arg

 Input:  position on the screen
         line of text to output

 Result: text is output
         cursor position is update
  ----------------------------------------------------------------------*/
void
/*VARARGS2*/
PutLine1(x, y, line, arg1)
    int   x, y;
    char *line;
    void *arg1;
{
    char buffer[PUTLINE_BUFLEN];

    sprintf(buffer, line, arg1);
    PutLine0(x, y, buffer);
}


/*----------------------------------------------------------------------
      Printf style output line to the screen at given position, 2 args

 Input:  position on the screen
         line of text to output

 Result: text is output
         cursor position is update
  ----------------------------------------------------------------------*/
void
/*VARARGS3*/
PutLine2(x, y, line, arg1, arg2)
    int   x, y;
    char *line;
    void *arg1, *arg2;
{
    char buffer[PUTLINE_BUFLEN];

    sprintf(buffer, line, arg1, arg2);
    PutLine0(x, y, buffer);
}


/*----------------------------------------------------------------------
      Printf style output line to the screen at given position, 3 args

 Input:  position on the screen
         line of text to output

 Result: text is output
         cursor position is update
  ----------------------------------------------------------------------*/
void
/*VARARGS4*/
PutLine3(x, y, line, arg1, arg2, arg3)
    int   x, y;
    char *line;
    void *arg1, *arg2, *arg3;
{
    char buffer[PUTLINE_BUFLEN];

    sprintf(buffer, line, arg1, arg2, arg3);
    PutLine0(x, y, buffer);
}


/*----------------------------------------------------------------------
      Printf style output line to the screen at given position, 4 args

 Args:  x -- column position on the screen
        y -- column position on the screen
        line -- printf style line of text to output

 Result: text is output
         cursor position is update
  ----------------------------------------------------------------------*/
void
/*VARARGS5*/
PutLine4(x, y, line, arg1, arg2, arg3, arg4)
     int   x, y;
     char *line;
     void *arg1, *arg2, *arg3, *arg4;
{
    char buffer[PUTLINE_BUFLEN];

    sprintf(buffer, line, arg1, arg2, arg3, arg4);
    PutLine0(x, y, buffer);
}



/*----------------------------------------------------------------------
      Printf style output line to the screen at given position, 5 args

 Args:  x -- column position on the screen
        y -- column position on the screen
        line -- printf style line of text to output

 Result: text is output
         cursor position is update
  ----------------------------------------------------------------------*/
void
/*VARARGS6*/
PutLine5(x, y, line, arg1, arg2, arg3, arg4, arg5)
     int   x, y;
     char *line;
     void *arg1, *arg2, *arg3, *arg4, *arg5;
{
    char buffer[PUTLINE_BUFLEN];

    sprintf(buffer, line, arg1, arg2, arg3, arg4, arg5);
    PutLine0(x, y, buffer);
}



/*----------------------------------------------------------------------
       Output a line to the screen, centered

  Input:  Line number to print on, string to output
  
 Result:  String is output to screen
          Returns column number line is output on
  ----------------------------------------------------------------------*/
int
Centerline(line, string)
    int   line;
    char *string;
{
    register int length, col;

    length = strlen(string);

    if (length > ps_global->ttyo->screen_cols)
      col = 0;
    else
      col = (ps_global->ttyo->screen_cols - length) / 2;

    PutLine0(line, col, string);
    return(col);
}



/*----------------------------------------------------------------------
     Clear specified line on the screen

 Result: The line is blanked and the cursor is left at column 0.

  ----*/
void
ClearLine(n)
    int n;
{
    if(ps_global->in_init_seq)
      return;

    MoveCursor(n, 0);
    CleartoEOLN();
}



/*----------------------------------------------------------------------
     Clear specified lines on the screen

 Result: The lines starting at 'x' and ending at 'y' are blanked
	 and the cursor is left at row 'x', column 0

  ----*/
void
ClearLines(x, y)
    int x, y;
{
    int i;

    for(i = x; i <= y; i++)
      ClearLine(i);

    MoveCursor(x, 0);
}



/*----------------------------------------------------------------------
    Indicate to the screen painting here that the position of the cursor
 has been disturbed and isn't where these functions might think.
 ----*/
void
clear_cursor_pos()
{
    _line = FARAWAY;
    _col  = FARAWAY;
}


/*----------------------------------------------------------------------
      Return current inverse state
 
 Result: returns 1 if in inverse state, 0 if not.
  ----------------------------------------------------------------------*/
int
InverseState()
{
    return(_in_inverse);
}
