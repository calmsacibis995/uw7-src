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
       ttyin.c
       Things having to do with reading from the tty driver and keyboard
          - initialize tty driver and reset tty driver
          - read a character from terminal with keyboard escape seqence mapping
          - initialize keyboard (keypad or such) and reset keyboard
          - prompt user for a line of input
          - read a command from keyboard with timeouts.

 ====*/


/*
 * Helpful definitions
 */
#define	RETURN_CH(X)	return(key_recorder((X), 0))


#if	!(defined(DOS) || defined(OS2))
/* Beginning of giant switch between UNIX and DOS input driver */

#include "headers.h"

/*
 * Internal prototypes
 */
void line_paint PROTO((int, int *));
int  process_config_input PROTO((int *));
int  getchar_for_kbseq PROTO ((void));
int  check_for_timeout PROTO((int));
int  read_a_char PROTO((void));
void read_bail PROTO((void));

#ifdef USE_POLL
#include <stropts.h>
#include <poll.h>
#endif

#ifdef HAVE_TERMIOS
struct termios _raw_tty, _original_tty;
#else
#ifdef HAVE_TERMIO
static struct termio _raw_tty, _original_tty;
#else /* HAVE_TERMIO */
static struct sgttyb  _raw_tty,     _original_tty;
static struct ltchars _raw_ltchars, _original_ltchars;
static struct tchars  _raw_tchars,  _original_tchars;
static int            _raw_lmode,   _original_lmode;
#endif /* HAVE_TERMIO  */
#endif /* HAVE_TERMIOS */

/*
 * current raw state
 */
static short _inraw = 0;


#define STDIN_FD	0
#define STDOUT_FD	1

/*----------------------------------------------------------------------
    Initialize the tty driver to do single char I/O and whatever else  (UNIX)

   Args:  struct pine

 Result: tty driver is put in raw mode so characters can be read one
         at a time. Returns -1 if unsuccessful, 0 if successful.

Some file descriptor voodoo to allow for pipes across vforks. See 
open_mailer for details.
  ----------------------------------------------------------------------*/
init_tty_driver(ps)
     struct pine *ps;
{
#ifdef	MOUSE
    if(F_ON(F_ENABLE_MOUSE, ps_global))
      if(init_mouse())
	kpinsert(&ps->kbesc, "\033[M", KEY_XTERM_MOUSE);
#endif	/* MOUSE */

    /* turn off talk permission by default */
    
    if(F_ON(F_ALLOW_TALK, ps))
      allow_talk(ps);
    else
      disallow_talk(ps);

    return(Raw(1));
}


/*----------------------------------------------------------------------
    Add the hardwired escape sequences to the keyboard escape sequence	(UNIX)
    tree.  This only needs to be here for people who don't have correct
    termcap/info entries for these, which is almost everybody.  Since
    these get added after the termcap/info versions of the same thing,
    an escape sequence here that is the same as one there, or is a prefix
    of one, or is a superset of one, will override the one from termcap.
    So an incorrect termcap won't break things.  A correct termcap entry
    that gets overwrote by an unfortunate coincidence will break things.
    In order to reverse the priority and have termcap/info entries override
    hardwired ones, define TERMCAP_WINS for os_unix.c in the
    pico subdirectory.

   Args:  kbesc -- the trie used to store the escape sequences in

 Result: a fixed set of known escape sequences are added to the trie
  ----------------------------------------------------------------------*/
void
setup_dflt_esc_seq(kbesc)
    KBESC_T     **kbesc;
{
    /*
     * this is sort of a hack [no kidding], but it allows us to use
     * the function keys on pc's running telnet
     *
     * UW-NDC/UCS vt10[01] application mode.
     */
    kpinsert(kbesc, "\033OP", PF1);
    kpinsert(kbesc, "\033OQ", PF2);
    kpinsert(kbesc, "\033OR", PF3);
    kpinsert(kbesc, "\033OS", PF4);
    kpinsert(kbesc, "\033Op", PF5);
    kpinsert(kbesc, "\033Oq", PF6);
    kpinsert(kbesc, "\033Or", PF7);
    kpinsert(kbesc, "\033Os", PF8);
    kpinsert(kbesc, "\033Ot", PF9);
    kpinsert(kbesc, "\033Ou", PF10);
    kpinsert(kbesc, "\033Ov", PF11);
    kpinsert(kbesc, "\033Ow", PF12);

    /*
     * DEC vt100, ANSI and cursor key mode.
     */
    kpinsert(kbesc, "\033OA", KEY_UP);
    kpinsert(kbesc, "\033OB", KEY_DOWN);
    kpinsert(kbesc, "\033OC", KEY_RIGHT);
    kpinsert(kbesc, "\033OD", KEY_LEFT);

    /*
     * special keypad functions
     */
    kpinsert(kbesc, "\033[4J", KEY_PGUP);
    kpinsert(kbesc, "\033[3J", KEY_PGDN);
    kpinsert(kbesc, "\033[2J", KEY_HOME);
    kpinsert(kbesc, "\033[N",  KEY_END);

    /*
     * ANSI mode.
     */
    kpinsert(kbesc, "\033[=a", PF1);
    kpinsert(kbesc, "\033[=b", PF2);
    kpinsert(kbesc, "\033[=c", PF3);
    kpinsert(kbesc, "\033[=d", PF4);
    kpinsert(kbesc, "\033[=e", PF5);
    kpinsert(kbesc, "\033[=f", PF6);
    kpinsert(kbesc, "\033[=g", PF7);
    kpinsert(kbesc, "\033[=h", PF8);
    kpinsert(kbesc, "\033[=i", PF9);
    kpinsert(kbesc, "\033[=j", PF10);
    kpinsert(kbesc, "\033[=k", PF11);
    kpinsert(kbesc, "\033[=l", PF12);

    /*
     * DEC vt100, ANSI and cursor key mode reset
     */
    kpinsert(kbesc, "\033[A", KEY_UP);
    kpinsert(kbesc, "\033[B", KEY_DOWN);
    kpinsert(kbesc, "\033[C", KEY_RIGHT);
    kpinsert(kbesc, "\033[D", KEY_LEFT);

    /*
     * DEC vt52 mode.
     */
    kpinsert(kbesc, "\033A", KEY_UP);
    kpinsert(kbesc, "\033B", KEY_DOWN);
    kpinsert(kbesc, "\033C", KEY_RIGHT);
    kpinsert(kbesc, "\033D", KEY_LEFT);

    /*
     * DEC vt52 application keys, and some Zenith 19.
     */
    kpinsert(kbesc, "\033?r", KEY_DOWN);
    kpinsert(kbesc, "\033?t", KEY_LEFT);
    kpinsert(kbesc, "\033?v", KEY_RIGHT);
    kpinsert(kbesc, "\033?x", KEY_UP);

    /*
     * Sun Console sequences.
     */
    kpinsert(kbesc, "\033[1",   KEY_SWALLOW_Z);
    kpinsert(kbesc, "\033[215", KEY_SWAL_UP);
    kpinsert(kbesc, "\033[217", KEY_SWAL_LEFT);
    kpinsert(kbesc, "\033[219", KEY_SWAL_RIGHT);
    kpinsert(kbesc, "\033[221", KEY_SWAL_DOWN);

    /*
     * Kermit App Prog Cmd, gobble until ESC \ (kermit should intercept this)
     */
    kpinsert(kbesc, "\033_", KEY_KERMIT);

    /*
     * Fake a control character.
     */
    kpinsert(kbesc, "\033\033", KEY_DOUBLE_ESC);
}



/*----------------------------------------------------------------------
   Set or clear the specified tty mode

   Args: ps --  struct pine
	 mode -- mode bits to modify
	 clear -- whether or not to clear or set

 Result: tty driver mode change. 
  ----------------------------------------------------------------------*/
void
tty_chmod(ps, mode, func)
    struct pine *ps;
    int		 mode;
    int		 func;
{
    char	*tty_name;
    int		 new_mode;
    struct stat  sbuf;
    static int   saved_mode = -1;

    /* if no problem figuring out tty's name & mode? */
    if((((tty_name = (char *) ttyname(STDIN_FD)) != NULL
	 && fstat(STDIN_FD, &sbuf) == 0)
	|| ((tty_name = (char *) ttyname(STDOUT_FD)) != NULL
	    && fstat(STDOUT_FD, &sbuf) == 0))
       && !(func == TMD_RESET && saved_mode < 0)){
	new_mode = (func == TMD_RESET)
		     ? saved_mode
		     : (func == TMD_CLEAR)
			? (sbuf.st_mode & ~mode)
			: (sbuf.st_mode | mode);
	/* assign tty new mode */
	if(chmod(tty_name, new_mode) == 0){
	    if(func == TMD_RESET)		/* forget we knew */
	      saved_mode = -1;
	    else if(saved_mode < 0)
	      saved_mode = sbuf.st_mode;	/* remember original */
	}
    }
}



/*----------------------------------------------------------------------
       End use of the tty, put it back into it's normal mode     (UNIX)

   Args: ps --  struct pine

 Result: tty driver mode change. 
  ----------------------------------------------------------------------*/
void
end_tty_driver(ps)
     struct pine *ps;
{
    ps = ps; /* get rid of unused parameter warning */

#ifdef	MOUSE
    end_mouse();
#endif	/* MOUSE */
    fflush(stdout);
    dprint(2, (debugfile, "about to end_tty_driver\n"));

    tty_chmod(ps, 0, TMD_RESET);
    Raw(0);
}



/*----------------------------------------------------------------------
    Actually set up the tty driver                             (UNIX)

   Args: state -- which state to put it in. 1 means go into raw, 0 out of

  Result: returns 0 if successful and -1 if not.
  ----*/

Raw(state)
int state;
{
    /** state is either ON or OFF, as indicated by call **/
    /* Check return code only on first call. If it fails we're done for and
       if it goes OK the other will probably go OK too. */

    if (state == 0 && _inraw) {
        /*----- restore state to original -----*/
#ifdef	HAVE_TERMIOS
	if (tcsetattr (STDIN_FD, TCSADRAIN, &_original_tty) < 0)
		return -1;
#else	/* HAVE_TERMIOS */
#ifdef	HAVE_TERMIO
        if(ioctl(STDIN_FD, TCSETAW, &_original_tty) < 0)
          return(-1);
#else
	if(ioctl(STDIN_FD, TIOCSETP, &_original_tty) < 0)
          return(-1);
	(void) ioctl(STDIN_FD, TIOCSLTC, &_original_ltchars);
	(void) ioctl(STDIN_FD, TIOCSETC, &_original_tchars);
        (void) ioctl(STDIN_FD, TIOCLSET, &_original_lmode);
#endif	/* HAVE_TERMIO */
#endif	/* HAVE_TERMIOS */
        _inraw = 0;
    } else if (state == 1 && ! _inraw) {
        /*----- Go into raw mode (cbreak actually) ----*/

#ifdef	HAVE_TERMIOS
	if (tcgetattr (STDIN_FD, &_original_tty) < 0)
		return -1;
	tcgetattr (STDIN_FD, &_raw_tty);
	_raw_tty.c_lflag &= ~(ICANON | ECHO | IEXTEN); /* noecho raw mode    */

 	_raw_tty.c_lflag &= ~ISIG;		/* disable signals           */
 	_raw_tty.c_iflag &= ~ICRNL;		/* turn off CR->NL on input  */
 	_raw_tty.c_oflag &= ~ONLCR;		/* turn off NL->CR on output */

	/* Only go into 8 bit mode if we are doing something other
	 * than plain ASCII. This will save the folks that have
	 * their parity on their serial lines wrong thr trouble of
	 * getting it right
	 */
        if(ps_global->VAR_CHAR_SET && ps_global->VAR_CHAR_SET[0] &&
	   strucmp(ps_global->VAR_CHAR_SET, "us-ascii"))
	  _raw_tty.c_iflag &= ~ISTRIP;		/* turn off hi bit stripping */

	_raw_tty.c_cc[VMIN]  = '\01';		/* min # of chars to queue  */
	_raw_tty.c_cc[VTIME] = '\0';		/* min time to wait for input*/
	_raw_tty.c_cc[VINTR] = ctrl('C');	/* make it our special char */
	_raw_tty.c_cc[VQUIT] = 0;
	_raw_tty.c_cc[VSUSP] = 0;
	ps_global->low_speed = cfgetospeed(&_raw_tty) < B4800;
	tcsetattr (STDIN_FD, TCSADRAIN, &_raw_tty);
#else
#ifdef	HAVE_TERMIO
        if(ioctl(STDIN_FD, TCGETA, &_original_tty) < 0)
          return(-1);
	(void) ioctl(STDIN_FD, TCGETA, &_raw_tty);    /** again! **/

	_raw_tty.c_lflag &= ~(ICANON | ECHO);	/* noecho raw mode  */

 	_raw_tty.c_lflag &= ~ISIG;		/* disable signals */

 	_raw_tty.c_iflag &= ~ICRNL;		/* turn off CR->NL on input  */
 	_raw_tty.c_oflag &= ~ONLCR;		/* turn off NL->CR on output */
	_raw_tty.c_cc[VMIN]  = 1;		/* min # of chars to queue   */
	_raw_tty.c_cc[VTIME] = 0;		/* min time to wait for input*/
	_raw_tty.c_cc[VINTR] = ctrl('C');	/* make it our special char */
	_raw_tty.c_cc[VQUIT] = 0;
        ps_global->low_speed = (_raw_tty.c_cflag&CBAUD) < (unsigned int)B4800;
	(void) ioctl(STDIN_FD, TCSETAW, &_raw_tty);

#else	/* HAVE_TERMIO */
        if(ioctl(STDIN_FD, TIOCGETP, &_original_tty) < 0)
          return(-1);
	(void) ioctl(STDIN_FD, TIOCGETP, &_raw_tty);   
        (void) ioctl(STDIN_FD, TIOCGETC, &_original_tchars);
	(void) ioctl(STDIN_FD, TIOCGETC, &_raw_tchars);
	(void) ioctl(STDIN_FD, TIOCGLTC, &_original_ltchars);
	(void) ioctl(STDIN_FD, TIOCGLTC, &_raw_ltchars);
        (void) ioctl(STDIN_FD, TIOCLGET, &_original_lmode);
        (void) ioctl(STDIN_FD, TIOCLGET, &_raw_lmode);

	_raw_tty.sg_flags &= ~(ECHO);	/* echo off */
	_raw_tty.sg_flags |= CBREAK;	/* raw on    */
        _raw_tty.sg_flags &= ~CRMOD;	/* Turn off CR -> LF mapping */

	_raw_tchars.t_intrc = -1;	/* Turn off ^C and ^D */
	_raw_tchars.t_eofc  = -1;

#ifdef	DEBUG
	if(debug < 9)			/* only on if full debugging set */
#endif
	_raw_tchars.t_quitc = -1;	/* Turn off ^\ */

	_raw_ltchars.t_lnextc = -1;	/* Turn off ^V so we can use it */
	_raw_ltchars.t_dsuspc = -1;	/* Turn off ^Y so we can use it */
	_raw_ltchars.t_suspc  = -1;	/* Turn off ^Z; we just read 'em */
	_raw_ltchars.t_werasc = -1;	/* Turn off ^w word erase */
	_raw_ltchars.t_rprntc = -1;	/* Turn off ^R reprint line */
        _raw_ltchars.t_flushc = -1;	/* Turn off ^O output flush */

	/* Only go into 8 bit mode if we are doing something other
	 * than plain ASCII. This will save the folks that have
	 * their parity on their serial lines wrong thr trouble of
	 * getting it right
	 */
        if(ps_global->VAR_CHAR_SET && ps_global->VAR_CHAR_SET[0] &&
	   strucmp(ps_global->VAR_CHAR_SET, "us-ascii")) {
	    _raw_lmode |= LPASS8;
#ifdef	NXT				/* Hope there aren't many like this! */
            _raw_lmode |= LPASS8OUT;
#endif
	}
            
	(void) ioctl(STDIN_FD, TIOCSETP, &_raw_tty);
	(void) ioctl(STDIN_FD, TIOCSLTC, &_raw_ltchars);
        (void) ioctl(STDIN_FD, TIOCSETC, &_raw_tchars);
        (void) ioctl(STDIN_FD, TIOCLSET, &_raw_lmode);
        ps_global->low_speed =  (int)_raw_tty.sg_ispeed < B4800;

#endif
#endif
	_inraw = 1;
	crlf_proc(0);
	xonxoff_proc(F_ON(F_PRESERVE_START_STOP, ps_global));
    }
    return(0);
}



/*----------------------------------------------------------------------
    Set up the tty driver to use XON/XOFF flow control         (UNIX)

   Args: state -- True to make sure XON/XOFF turned on, FALSE off.

  Result: none.

  ----*/
void
xonxoff_proc(state)
    int state;
{
    if(_inraw){
	if(state){
#ifdef	HAVE_TERMIOS
	    if(!(_raw_tty.c_iflag & IXON)){
		_raw_tty.c_iflag |= IXON;
		tcsetattr (STDIN_FD, TCSADRAIN, &_raw_tty);
	    }
#else
#ifdef	HAVE_TERMIO
	    if(!(_raw_tty.c_iflag & IXON)){
		_raw_tty.c_iflag |= IXON;	/* turn ON ^S/^Q on input   */
		(void) ioctl(STDIN_FD, TCSETAW, &_raw_tty);
	    }
#else	/* HAVE_TERMIO */
	    if(_raw_tchars.t_startc == -1 || _raw_tchars.t_stopc == -1){
		_raw_tchars.t_startc = 'Q' - '@'; /* Turn ON ^S/^Q */
		_raw_tchars.t_stopc  = 'S' - '@';
		(void) ioctl(STDIN_FD, TIOCSETC, &_raw_tchars);
	    }
#endif
#endif
	}
	else if(F_OFF(F_PRESERVE_START_STOP, ps_global)){
#ifdef	HAVE_TERMIOS
	    if(_raw_tty.c_iflag & IXON){
		_raw_tty.c_iflag &= ~IXON;	/* turn off ^S/^Q on input   */
		tcsetattr (STDIN_FD, TCSADRAIN, &_raw_tty);
	    }
#else
#ifdef	HAVE_TERMIO
	    if(_raw_tty.c_iflag & IXON){
		_raw_tty.c_iflag &= ~IXON;	/* turn off ^S/^Q on input   */
		(void) ioctl(STDIN_FD, TCSETAW, &_raw_tty);
	    }
#else	/* HAVE_TERMIO */
	    if(!(_raw_tchars.t_startc == -1 && _raw_tchars.t_stopc == -1)){
		_raw_tchars.t_startc = -1;	/* Turn off ^S/^Q */
		_raw_tchars.t_stopc  = -1;
		(void) ioctl(STDIN_FD, TIOCSETC, &_raw_tchars);
	    }
#endif
#endif
	}
    }
}



/*----------------------------------------------------------------------
    Set up the tty driver to do LF->CR translation		(UNIX)

   Args: state -- True to turn on translation, false to write raw LF's

  Result: none.

  ----*/
void
crlf_proc(state)
    int state;
{
    if(_inraw){
	if(state){				/* turn ON NL->CR on output */
#ifdef	HAVE_TERMIOS
	    if(!(_raw_tty.c_oflag & ONLCR)){
		_raw_tty.c_oflag |= ONLCR;
		tcsetattr (STDIN_FD, TCSADRAIN, &_raw_tty);
	    }
#else
#ifdef	HAVE_TERMIO
	    if(!(_raw_tty.c_oflag & ONLCR)){
		_raw_tty.c_oflag |= ONLCR;
		(void) ioctl(STDIN_FD, TCSETAW, &_raw_tty);
	    }
#else	/* HAVE_TERMIO */
	    if(!(_raw_tty.sg_flags & CRMOD)){
		_raw_tty.sg_flags |= CRMOD;
		(void) ioctl(STDIN_FD, TIOCSETP, &_raw_tty);
	    }
#endif
#endif
	}
	else{					/* turn OFF NL-CR on output */
#ifdef	HAVE_TERMIOS
	    if(_raw_tty.c_oflag & ONLCR){
		_raw_tty.c_oflag &= ~ONLCR;
		tcsetattr (STDIN_FD, TCSADRAIN, &_raw_tty);
	    }
#else
#ifdef	HAVE_TERMIO
	    if(_raw_tty.c_oflag & ONLCR){
		_raw_tty.c_oflag &= ~ONLCR;
		(void) ioctl(STDIN_FD, TCSETAW, &_raw_tty);
	    }
#else	/* HAVE_TERMIO */
	    if(_raw_tty.sg_flags & CRMOD){
		_raw_tty.sg_flags &= ~CRMOD;
		(void) ioctl(STDIN_FD, TIOCSETP, &_raw_tty);
	    }
#endif
#endif
	}
    }
}


/*----------------------------------------------------------------------
    Set up the tty driver to hanle interrupt char		(UNIX)

   Args: state -- True to turn on interrupt char, false to not

  Result: tty driver that'll send us SIGINT or not

  ----*/
void
intr_proc(state)
    int state;
{
    if(_inraw){
	if(state){
#ifdef HAVE_TERMIOS
	    _raw_tty.c_lflag |= ISIG;		/* enable signals */
	    tcsetattr(STDIN_FD, TCSADRAIN, &_raw_tty);
#else
#ifdef HAVE_TERMIO
	    _raw_tty.c_lflag |= ISIG;		/* enable signals */
	    (void)ioctl(STDIN_FD, TCSETAW, &_raw_tty);
#else
	    _raw_tchars.t_intrc = ctrl('C');
	    (void)ioctl(STDIN_FD, TIOCSETC, &_raw_tchars);
#endif /* HAVE_TERMIO */
#endif /* HAVE_TERMIOS */
	}
	else{
#ifdef	HAVE_TERMIOS
	    _raw_tty.c_lflag &= ~ISIG;		/* disable signals */
	    tcsetattr(STDIN_FD, TCSADRAIN, &_raw_tty);
#else
#ifdef	HAVE_TERMIO
	    _raw_tty.c_lflag &= ~ISIG;		/* disable signals */
	    (void)ioctl(STDIN_FD, TCSETAW, &_raw_tty);
#else	/* HAVE_TERMIO */
	    _raw_tchars.t_intrc = -1;
	    (void)ioctl(STDIN_FD, TIOCSETC, &_raw_tchars);
#endif /* HAVE_TERMIO */
#endif /* HAVE_TERMIOS */
	}
    }
}


#ifdef RESIZING
jmp_buf winch_state;
int     winch_occured = 0;
int     ready_for_winch = 0;
#endif

/*----------------------------------------------------------------------
     This checks whether or not a character			(UNIX)
     is ready to be read, or it times out.

    Args:  time_out --  number of seconds before it will timeout

  Result: Returns a NO_OP_IDLE or a NO_OP_COMMAND if the timeout expires
	  before input is available, or a KEY_RESIZE if a resize event
	  occurs, or READY_TO_READ if input is available before the timeout.
  ----*/
int
check_for_timeout(time_out)
     int time_out;
{
#ifdef USE_POLL
     struct pollfd pollfd;
#else
     struct timeval tmo;
     fd_set         readfds, errfds;
#endif
     int	    res;
     unsigned char  c;

     fflush(stdout);

#ifdef RESIZING
     if(winch_occured || setjmp(winch_state) != 0) {
         ready_for_winch = 0;
	 fix_windsize(ps_global);

	 /*
	  * May need to unblock signal after longjmp from handler, because
	  * signal is normally unblocked upon routine exit from the handler.
	  */
	 if(!winch_occured)
	   pine_sigunblock(SIGWINCH);

         winch_occured   = 0;
         return(KEY_RESIZE);
     } else {
         ready_for_winch = 1;
     }
#endif /* RESIZING */

     if(time_out > 0) {
         /* Check to see if there's bytes to read with a timeout */
#ifdef USE_POLL
	 pollfd.fd = STDIN_FD;
	 pollfd.events = POLLIN;
	 dprint(9,(debugfile,"poll event %d, timeout %d\n", pollfd.events,
		   time_out));
	 res = poll (&pollfd, 1, time_out * 1000);
	 dprint(9, (debugfile, "poll on tty returned %d, events %d\n",
		    res, pollfd.revents));
	 if((pollfd.revents & (POLLERR | POLLHUP | POLLNVAL)) && res >= 0){
	     if(pollfd.revents & POLLHUP)
	       read_bail();		/* non tragic */
	     else
	   res = -1;		/* exit below! */
	 }
#else
	 FD_ZERO(&readfds);
	 FD_ZERO(&errfds);
	 FD_SET(STDIN_FD, &readfds);
	 FD_SET(STDIN_FD, &errfds);
	 tmo.tv_sec  = time_out;
         tmo.tv_usec = 0; 

         dprint(9, (debugfile,"Select readfds:%d timeval:%d,%d\n", readfds,
		   tmo.tv_sec,tmo.tv_usec));

         res = select(STDIN_FD+1, &readfds, 0, &errfds, &tmo);

         dprint(9, (debugfile, "Select on tty returned %d\n", res));
#endif

         if(res < 0) {
             if(errno == EINTR || errno == EAGAIN) {
#ifdef RESIZING
                 ready_for_winch = 0;
#endif
                 return(NO_OP_COMMAND);
             }
             panic1("Select error: %s\n", error_description(errno));
         }

         if(res == 0) { /* the select timed out */
	     if(getppid() == 1){
		 dprint(1, (debugfile,
			    "\n\n** Parent found to be init!?!\n\n"));
		 read_bail();
		 /* NO RETURN */
	     }
	       
#ifdef RESIZING
             ready_for_winch = 0;
#endif
             return(time_out > 25 ? NO_OP_IDLE: NO_OP_COMMAND);
         }
     }

#ifdef RESIZING
     ready_for_winch = 0;
#endif

     return(READY_TO_READ);
}


/*----------------------------------------------------------------------
     Lowest level read command. This reads one character.	(UNIX)

  Result: Returns the single character read or a NO_OP_COMMAND if the
          read was interrupted.
  ----*/
int
read_a_char()
{
     int	    res;
     unsigned char  c;

     res = read(STDIN_FD, &c, 1);

     if(res <= 0) {
	 /*
	  * Error reading from terminal!
	  * The only acceptable failure is being interrupted.  If so,
	  * return a value indicating such...
	  */
	 if(res < 0 && errno == EINTR){
	     dprint(2, (debugfile, "read_a_char interrupted\n"));
	     RETURN_CH(NO_OP_COMMAND);
	 }

	 /*
	  * Else we read EOF or otherwise encountered some catastrophic
	  * error.  Treat it like SIGHUP (i.e., clean up and exit).
	  */
	 dprint(1, (debugfile, "\n\n** Error reading from tty : %s\n\n",
		    (res == 0) ? "EOF" : error_description(errno)));

	 read_bail();
	 /* NO RETURN */
     }

     dprint(9, (debugfile, "read_a_char return '\\%03o'\n", (int)c));
     RETURN_CH((int)c);
}


/*----------------------------------------------------------------------
     Simple version of read_a_char with simple error handling

  Result: Returns read character or it never returns
  ----*/
int
getchar_for_kbseq()
{
    int		  res;
    unsigned char c;

    while((res = read(STDIN_FD, &c, 1)) <= 0)
      if(!(res < 0 && errno == EINTR))
	read_bail();
	/* NO RETURN */

    RETURN_CH((int) c);
}


/*----------------------------------------------------------------------
  Read input characters with lots of processing for arrow keys and such  (UNIX)

 Args:  time_out -- The timeout to for the reads 

 Result: returns the character read. Possible special chars.

    This deals with function and arrow keys as well. 

  The idea is that this routine handles all escape codes so it done in
  only one place. Especially so the back arrow key can work when entering
  things on a line. Also so all function keys can be disabled and not
  cause weird things to happen.
  ---*/
int
read_char(time_out)
    int time_out;
{
    int ch, status, cc;

    /* Get input from initial-keystrokes */
    if(process_config_input(&ch))
      return(ch);

    /*
     * We only check for timeouts at the start of read_char, not in the
     * middle of escape sequences.
     */
    ch = check_for_timeout(time_out);
    if(ch == NO_OP_IDLE || ch == NO_OP_COMMAND || ch == KEY_RESIZE)
      goto done;

    switch(status = kbseq(ps_global->kbesc, getchar_for_kbseq, &ch)){
      case KEY_DOUBLE_ESC:
	/*
	 * Special hack to get around comm devices eating control characters.
	 */
	if(check_for_timeout(5) != READY_TO_READ){
	    ch = KEY_JUNK;		/* user typed ESC ESC, then stopped */
	    goto done;
	}
	else
	  ch = read_a_char();

	ch &= 0x7f;
	if(isdigit((unsigned char)ch)){
	    int n = 0, i = ch - '0';

	    if(i < 0 || i > 2){
		ch = KEY_JUNK;
		goto done;		/* bogus literal char value */
	    }

	    while(n++ < 2){
		if(check_for_timeout(5) != READY_TO_READ
		   || (!isdigit((unsigned char) (ch = read_a_char()))
		       || (n == 1 && i == 2 && ch > '5')
		       || (n == 2 && i == 25 && ch > '5'))){
		    ch = KEY_JUNK;	/* user typed ESC ESC #, stopped */
		    goto done;
		}

		i = (i * 10) + (ch - '0');
	    }

	    ch = i;
	}
	else{
	    if(islower((unsigned char)ch))	/* canonicalize if alpha */
	      ch = toupper((unsigned char)ch);

	    ch = (isalpha((unsigned char)ch) || ch == '@'
		  || (ch >= '[' && ch <= '_'))
		   ? ctrl(ch) : ((ch == SPACE) ? ctrl('@'): ch);
	}

	goto done;

#ifdef MOUSE
      case KEY_XTERM_MOUSE:
	if(mouseexist()){
	    /*
	     * special hack to get mouse events from an xterm.
	     * Get the details, then pass it past the keymenu event
	     * handler, and then to the installed handler if there
	     * is one...
	     */
	    static int down = 0;
	    int        button, x, y;
	    unsigned   cmd;

	    clear_cursor_pos();
	    ch = read_a_char();
	    button = ch & 0x03;

	    ch = read_a_char();
	    x = ch - '!';

	    ch = read_a_char();

	    y = ch - '!';

	    ch = NO_OP_COMMAND;
	    if(button == 0){		/* xterm button 1 down */
		down = 1;
		if(checkmouse(&cmd, 1, x, y))
		  ch = (int)cmd;
	    }
	    else if (down && button == 3){
		down = 0;
		if(checkmouse(&cmd, 0, x, y))
		  ch = (int)cmd;
	    }

	    goto done;
	}

	break;
#endif /* MOUSE */

      case  KEY_UP	:
      case  KEY_DOWN	:
      case  KEY_RIGHT	:
      case  KEY_LEFT	:
      case  KEY_PGUP	:
      case  KEY_PGDN	:
      case  KEY_HOME	:
      case  KEY_END	:
      case  KEY_DEL	:
      case  PF1		:
      case  PF2		:
      case  PF3		:
      case  PF4		:
      case  PF5		:
      case  PF6		:
      case  PF7		:
      case  PF8		:
      case  PF9		:
      case  PF10	:
      case  PF11	:
      case  PF12	:
        dprint(9, (debugfile, "Read char returning: %d %s\n",
                   status, pretty_command(status)));
	return(status);

      case KEY_SWALLOW_Z:
	status = KEY_JUNK;
      case KEY_SWAL_UP:
      case KEY_SWAL_DOWN:
      case KEY_SWAL_LEFT:
      case KEY_SWAL_RIGHT:
	do
	  if(check_for_timeout(2) != READY_TO_READ){
	      status = KEY_JUNK;
	      break;
	  }
	while(!strchr("~qz", read_a_char()));
	ch = (status == KEY_JUNK) ? status : status - (KEY_SWAL_UP - KEY_UP);
	goto done;

      case KEY_KERMIT:
	do{
	    cc = ch;
	    if(check_for_timeout(2) != READY_TO_READ){
		status = KEY_JUNK;
		break;
	    }
	    else
	      ch = read_a_char();
	}
	while(cc != '\033' && ch != '\\');

	ch = KEY_JUNK;
	goto done;

      case BADESC:
	ch = KEY_JUNK;
	goto done;

      case 0: 	/* regular character */
      default:
	/*
	 * we used to strip (ch &= 0x7f;), but this seems much cleaner
	 * in the face of line noise and has the benefit of making it
	 * tougher to emit mistakenly labeled MIME...
	 */
	if((ch & 0x80) && (!ps_global->VAR_CHAR_SET
			   || !strucmp(ps_global->VAR_CHAR_SET, "US-ASCII"))){
	    dprint(9, (debugfile, "Read char returning: %d %s\n",
		       status, pretty_command(status)));
	    return(KEY_JUNK);
	}
	else if(ch == ctrl('Z')){
	    dprint(9, (debugfile, "Read char calling do_suspend\n"));
	    return(do_suspend(ps_global));
	}


      done:
        dprint(9, (debugfile, "Read char returning: %d %s\n",
                   ch, pretty_command(ch)));
        return(ch);
    }
}


/*----------------------------------------------------------------------
  Reading input somehow failed and we need to shutdown now

 Args:  none

 Result: pine exits

  ---*/
void
read_bail()
{
    end_signals(1);
    if(ps_global->inbox_stream){
	if(ps_global->inbox_stream == ps_global->mail_stream)
	  ps_global->mail_stream = NULL;

	if(!ps_global->inbox_stream->lock)		/* shouldn't be... */
	  mail_close(ps_global->inbox_stream);
    }

    if(ps_global->mail_stream && !ps_global->mail_stream->lock)
      mail_close(ps_global->mail_stream);

    end_keyboard(F_ON(F_USE_FK,ps_global));
    end_tty_driver(ps_global);
    if(filter_data_file(0))
      unlink(filter_data_file(0));

    exit(0);
}


extern char term_name[]; /* term_name from ttyout.c-- affect keyboard*/
/* -------------------------------------------------------------------
     Set up the keyboard -- usually enable some function keys  (UNIX)

    Args: struct pine 

So far all we do here is turn on keypad mode for certain terminals

Hack for NCSA telnet on an IBM PC to put the keypad in the right mode.
This is the same for a vtXXX terminal or [zh][12]9's which we have 
a lot of at UW
  ----*/
void
init_keyboard(use_fkeys)
     int use_fkeys;
{
    if(use_fkeys && (!strucmp(term_name,"vt102")
		     || !strucmp(term_name,"vt100")))
      printf("\033\133\071\071\150");
}



/*----------------------------------------------------------------------
     Clear keyboard, usually disable some function keys           (UNIX)

   Args:  pine state (terminal type)

 Result: keyboard state reset
  ----*/
void
end_keyboard(use_fkeys)
     int use_fkeys;
{
    if(use_fkeys && (!strcmp(term_name, "vt102")
		     || !strcmp(term_name, "vt100"))){
	printf("\033\133\071\071\154");
	fflush(stdout);
    }
}

    
/*----------------------------------------------------------------------
   Discard any pending input characters				(UNIX)

   Args:  none

 Result: pending input buffer flushed
  ----*/
void
flush_input()
{
#ifdef	HAVE_TERMIOS
    tcflush(STDIN_FD, TCIFLUSH);
#else
#ifdef	HAVE_TERMIO
    ioctl(STDIN_FD, TCFLSH, 0);
#else
#ifdef	TIOCFLUSH
#ifdef	FREAD
    int i = FREAD;
#else
    int i = 1;
#endif

    ioctl(STDIN_FD, TIOCFLUSH, &i);
#else
#endif	/* TIOCFLUSH */
#endif	/* HAVE_TERMIO */
#endif	/* HAVE_TERMIOS */
}

    
#else
/*
 * DOS && OS/2 specific code.
 * Middle of giant switch between UNIX and DOS/OS2 input drivers
 */


#ifdef OS2
#define INCL_BASE
#define INCL_DOS
#define INCL_VIO
#define INCL_KBD
#define INCL_NOPM
#include <os2.h>
#undef ADDRESS
#endif

#include "headers.h"


/*
 * Internal prototypes
 */
void line_paint PROTO((int, int *));
int  process_config_input PROTO((int *));



#if	!(defined(WIN32) || defined(OS2))
#include <dos.h>
#include <bios.h>
#endif

/* global to tell us if we have an enhanced keyboard! */
static int enhanced = 0;
/* global function to execute while waiting for character input */
void   (*while_waiting)() = NULL;

#ifdef _WINDOWS
/* global to tell us if the window was resized. */
static int DidResize = FALSE;


/*----------------------------------------------------------------------
    Flag the fact the window has resized.
*/
int
pine_window_resize_callback ()
{
    DidResize = TRUE;
}
#endif



/*----------------------------------------------------------------------
    Initialize the tty driver to do single char I/O and whatever else  (DOS)

 Input:  struct pine

 Result: tty driver is put in raw mode
  ----------------------------------------------------------------------*/
init_tty_driver(pine)
     struct pine *pine;
{
#ifdef _WINDOWS
    mswin_setresizecallback (pine_window_resize_callback);
    init_mouse ();			/* always a mouse under windows? */
#else
#ifdef OS2
    enhanced = 1;
#else
    /* detect enhanced keyboard */
    enhanced = enhanced_keybrd();	/* are there extra keys? */
#endif
#endif
    pine = pine;			/* Get rid of unused parm warning */
    return(Raw(1));
}



/*----------------------------------------------------------------------
       End use of the tty, put it back into it's normal mode          (DOS)

 Input:  struct pine

 Result: tty driver mode change
  ----------------------------------------------------------------------*/
void
end_tty_driver(pine)
     struct pine *pine;
{
    dprint(2, (debugfile, "about to end_tty_driver\n"));
#ifdef _WINDOWS
    mswin_clearresizecallback (pine_window_resize_callback);
#endif
}

/*----------------------------------------------------------------------
   translate IBM Keyboard Extended Functions to things pine understands.
   More work can be done to make things like Home, PageUp and PageDown work. 

/*
 * extended_code - return special key definition
 */
extended_code(kc)
unsigned  kc;
{
    switch(kc){
#ifdef	_WINDOWS
	case MSWIN_KEY_F1: return(PF1);
	case MSWIN_KEY_F2: return(PF2);
	case MSWIN_KEY_F3: return(PF3);
	case MSWIN_KEY_F4: return(PF4);
	case MSWIN_KEY_F5: return(PF5);
	case MSWIN_KEY_F6: return(PF6);
	case MSWIN_KEY_F7: return(PF7);
	case MSWIN_KEY_F8: return(PF8);
	case MSWIN_KEY_F9: return(PF9);
	case MSWIN_KEY_F10: return(PF10);
	case MSWIN_KEY_F11: return(PF11);
	case MSWIN_KEY_F12: return(PF12);

	case MSWIN_KEY_UP: return(KEY_UP);
	case MSWIN_KEY_DOWN: return(KEY_DOWN);
	case MSWIN_KEY_LEFT: return(KEY_LEFT);
	case MSWIN_KEY_RIGHT: return(KEY_RIGHT);
	case MSWIN_KEY_HOME: return(KEY_HOME);
	case MSWIN_KEY_END: return(KEY_END);
	case MSWIN_KEY_SCROLLUPPAGE:
	case MSWIN_KEY_PREVPAGE: return(KEY_PGUP);
	case MSWIN_KEY_SCROLLDOWNPAGE:
	case MSWIN_KEY_NEXTPAGE: return(KEY_PGDN);
	case MSWIN_KEY_DELETE: return(KEY_DEL);
	case MSWIN_KEY_SCROLLUPLINE: return (KEY_SCRLUPL);
	case MSWIN_KEY_SCROLLDOWNLINE: return (KEY_SCRLDNL);
	case MSWIN_KEY_SCROLLTO: return (KEY_SCRLTO);

	case MSWIN_KEY_NODATA:	return (NO_OP_COMMAND);
#else
	case 0x3b00 : return(PF1);
	case 0x3c00 : return(PF2);
	case 0x3d00 : return(PF3);
	case 0x3e00 : return(PF4);
	case 0x3f00 : return(PF5);
	case 0x4000 : return(PF6);
	case 0x4100 : return(PF7);
	case 0x4200 : return(PF8);
	case 0x4300 : return(PF9);
	case 0x4400 : return(PF10);
	case 0x8500 : return(PF11);
	case 0x8600 : return(PF12);

	case 0x4800 : return(KEY_UP);
	case 0x5000 : return(KEY_DOWN);
	case 0x4b00 : return(KEY_LEFT);
	case 0x4d00 : return(KEY_RIGHT);
	case 0x4700 : return(KEY_HOME);
	case 0x4f00 : return(KEY_END);
	case 0x4900 : return(KEY_PGUP);
	case 0x5100 : return(KEY_PGDN);
	case 0x5300 : return(KEY_DEL);
	case 0x48e0 : return(KEY_UP);			/* grey key version */
	case 0x50e0 : return(KEY_DOWN);			/* grey key version */
	case 0x4be0 : return(KEY_LEFT);			/* grey key version */
	case 0x4de0 : return(KEY_RIGHT);		/* grey key version */
	case 0x47e0 : return(KEY_HOME);			/* grey key version */
	case 0x4fe0 : return(KEY_END);			/* grey key version */
	case 0x49e0 : return(KEY_PGUP);			/* grey key version */
	case 0x51e0 : return(KEY_PGDN);			/* grey key version */
	case 0x53e0 : return(KEY_DEL);			/* grey key version */
#endif
	default   : return(NO_OP_COMMAND);
    }
}



/*----------------------------------------------------------------------
   Read input characters with lots of processing for arrow keys and such (DOS)

 Input:  none

 Result: returns the character read. Possible special chars defined h file


    This deals with function and arrow keys as well. 
  It returns ^T for up , ^U for down, ^V for forward and ^W for back.
  These are just sort of arbitrarily picked and might be changed.
  They are defined in defs.h. Didn't want to use 8 bit chars because
  the values are signed chars, though it ought to work with negative 
  values. 

  The idea is that this routine handles all escape codes so it done in
  only one place. Especially so the back arrow key can work when entering
  things on a line. Also so all function keys can be broken and not
  cause weird things to happen.
----------------------------------------------------------------------*/

int
read_char(tm)
int tm;
{
    unsigned   ch = 0;
    long       timein;
#ifndef	_WINDOWS
    unsigned   intrupt = 0;
    extern int win_multiplex();
#endif

    if(process_config_input((int *) &ch))
      RETURN_CH(ch);

#ifdef _WINDOWS
    if (DidResize) {
	 DidResize = FALSE;
	 RETURN_CH (get_windsize (ps_global->ttyo));
    }
#endif

#ifdef OS2
    vidUpdate();
#endif
#ifdef MOUSE
    mouseon();
#endif

    if(tm){
	timein = time(0L);
#ifdef _WINDOWS
	/* mswin_charavail() Yeilds control to other window apps. */
	while (!mswin_charavail()) {
#else
#ifdef OS2
	while (!kbd_ready()) {
#else
	while(!_bios_keybrd(enhanced ? _NKEYBRD_READY : _KEYBRD_READY)){
#endif
#endif
	    if(time(0L) >= timein+tm){
		ch = NO_OP_COMMAND;
		goto gotone;
	    }
#ifdef _WINDOWS
	    if (DidResize) {
		DidResize = FALSE;
		RETURN_CH( get_windsize (ps_global->ttyo));
	    }
#endif
#ifdef	MOUSE
 	    if(checkmouse(&ch,0,0,0))
	      goto gotone;
#endif
	    if(while_waiting)
	      (*while_waiting)();

#ifndef	_WINDOWS
	    /*
	     * the number "30" was not reached via experimentation
	     * or scientific analysis of any kind.
	     */
	    if(((intrupt++) % 30) == 0)	/* surrender CPU to windows */
	      win_multiplex();
#endif
	}
    }

#ifdef _WINDOWS
    ch = mswin_getc_fast();
#else
#ifdef OS2
    ch = kbd_getkey();
#else
    ch = _bios_keybrd(enhanced ? _NKEYBRD_READ : _KEYBRD_READ);
#endif
#endif

gotone:
#if defined(MOUSE)
    mouseoff();

    /* More obtuse key mapping.  If it is a mouse event, the return
     * may be KEY_MOUSE, which indicates to the upper layer that it
     * is a mouse event.  Return it here to avoid the code that
     * follows which would do a (ch & 0xff).
     */
    if (ch == KEY_MOUSE)
      RETURN_CH(ch);
#endif

    /*
     * WARNING: Hack notice.
     * the mouse interaction complicates this expression a bit as 
     * if function key mode is set, PFn values are setup for return
     * by the mouse event catcher.  For now, just special case them
     * since they don't conflict with any of the DOS special keys.
     */
#ifdef _WINDOWS

    if (ch >= MSWIN_RANGE_START && ch <= MSWIN_RANGE_END)
	    RETURN_CH (extended_code (ch));

    RETURN_CH (ch & KEY_MASK);
#else /* DOS */
    if((ch & 0xff) == ctrl('Z'))
      RETURN_CH(do_suspend(ps_global));

    RETURN_CH((ch >= PF1 && ch <= PF12)
	       ? ch
	       : ((ch&0xff) && ((ch&0xff) != 0xe0))
		  ? (ch&0xff)
		  : extended_code(ch));
#endif
}

#ifdef OS2
static KBDINFO initialKbdInfo;
#endif

/* -------------------------------------------------------------------
     Set up the keyboard -- usually enable some function keys     (DOS)

  Input: struct pine (terminal type)

  Result: keyboard set up

-----------------------------------------------------------------------*/
void
init_keyboard(use_fkeys)
     int use_fkeys;
{
#ifdef OS2
  KBDINFO kbdInfo;
  KbdGetStatus(&initialKbdInfo, 0);
  kbdInfo = initialKbdInfo;
  kbdInfo.fsMask &= ~(0x0001|0x0008|0x0100); /* echo cooked off */
  kbdInfo.fsMask |= (0x002|0x004|0x0100); /* noecho,raw,shiftrpt on */
  KbdSetStatus(&kbdInfo, 0);
#endif
}



/*----------------------------------------------------------------------
     Clear keyboard, usually disable some function keys            (DOS)

 Input:  pine state (terminal type)

 Result: keyboard state reset
  ----------------------------------------------------------------------*/
/* BUG shouldn't have to check for pine != NULL */
void
end_keyboard(use_fkeys)
     int use_fkeys;
{
#ifdef OS2
  KbdSetStatus(&initialKbdInfo, 0);
#endif
}



/*----------------------------------------------------------------------
   Discard any pending input characters				(DOS)

   Args:  none

 Result: pending input buffer flushed
  ----*/
void
flush_input()
{
#ifdef _WINDOWS
    while (mswin_charavail ())
        (void) mswin_getc ();
#else
#ifdef OS2
    kbd_flush();
#else
    while(_bios_keybrd(enhanced ? _NKEYBRD_READY : _KEYBRD_READY))
      (void) _bios_keybrd(enhanced ? _NKEYBRD_READ : _KEYBRD_READ);
#endif
#endif
}

    
/*----------------------------------------------------------------------
    Actually set up the tty driver                             (DOS)

   Args: state -- which state to put it in. 1 means go into raw, 0 out of

  Result: returns 0 if successful and -1 if not.
  ----*/

Raw(state)
int state;
{
#ifdef OS2
    KBDINFO ki = initialKbdInfo;
    if (state)
    {
        ki.fsMask &= ~(0x0001|0x0008|0x0100); /* echo cooked off */
        ki.fsMask |= (0x002|0x004|0x0100); /* noecho,raw,shiftrpt on */
    }
    KbdSetStatus(&ki, 0);
#endif

/* of course, DOS never runs at low speed!!! */
    ps_global->low_speed = 0;
    return(0);
}


/*----------------------------------------------------------------------
    Set up the tty driver to use XON/XOFF flow control		(DOS)

   Args: state -- True to make sure XON/XOFF turned on, FALSE default state

  Result: none.
  ----*/
void
xonxoff_proc(state)
    int state;
{
    return;					/* no op */
}


/*----------------------------------------------------------------------
    Set up the tty driver to do LF->CR translation		(DOS)

   Args: state -- True to turn on translation, false to write raw LF's

  Result: none.

  ----*/
void
crlf_proc(state)
    int state;
{
    return;					/* no op */
}


/*----------------------------------------------------------------------
    Set up the tty driver to hanle interrupt char		(DOS)

   Args: state -- True to turn on interrupt char, false to not

  Result: tty driver that'll send us SIGINT or not

  ----*/
void
intr_proc(state)
    int state;
{
    return;					/* no op */
}
#endif /* DOS End of giant switch between UNX and DOS input drivers */


/*----------------------------------------------------------------------
        Read a character from keyboard with timeout
 Input:  none

 Result: Returns command read via read_char
         Times out and returns a null command every so often

  Calculates the timeout for the read, and does a few other house keeping 
things.  The duration of the timeout is set in pine.c.
  ----------------------------------------------------------------------*/
int
read_command()
{
    int ch, tm = 0;
    long dtime; 

    cancel_busy_alarm(-1);
    tm = (messages_queued(&dtime) > 1) ? (int)dtime : timeout;

    ch = read_char(tm);
    dprint(9, (debugfile, "Read command returning: %d %s\n", ch,
              pretty_command(ch)));
    if(ch != NO_OP_COMMAND && ch != NO_OP_IDLE && ch != KEY_RESIZE)
      zero_new_mail_count();

#ifdef	BACKGROUND_POST
    /*
     * Any expired children to report on?
     */
    if(ps_global->post && ps_global->post->pid == 0){
	int   winner = 0;

	if(ps_global->post->status < 0){
	    q_status_message(SM_ORDER | SM_DING, 3, 3, "Abysmal failure!");
	}
	else{
	    (void) pine_send_status(ps_global->post->status,
				    ps_global->post->fcc, tmp_20k_buf,
				    &winner);
	    q_status_message(SM_ORDER | (winner ? 0 : SM_DING), 3, 3,
			     tmp_20k_buf);

	}

	if(!winner)
	  q_status_message(SM_ORDER, 0, 3,
	  "Re-send via \"Compose\" then \"Yes\" to \"Continue INTERRUPTED?\"");

	if(ps_global->post->fcc)
	  fs_give((void **) &ps_global->post->fcc);

	fs_give((void **) &ps_global->post);
    }
#endif

    return(ch);
}




/*
 *
 */
static struct display_line {
    int   row, col;			/* where display starts		 */
    int   dlen;				/* length of display line	 */
    char *dl;				/* line on display		 */
    char *vl;				/* virtual line 		 */
    int   vlen;				/* length of virtual line        */
    int   vused;			/* length of virtual line in use */
    int   vbase;			/* first virtual char on display */
} dline;



static struct key oe_keys[] =
       {{"^G","Help",KS_SCREENHELP},	{"^C","Cancel",KS_NONE},
	{"^T","xxx",KS_NONE},		{"Ret","Accept",KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE}};
static struct key_menu oe_keymenu =
	{sizeof(oe_keys)/(sizeof(oe_keys[0])*12), 0, 0,0,0,0, oe_keys};
#define	OE_HELP_KEY	0
#define	OE_CANCEL_KEY	1
#define	OE_CTRL_T_KEY	2
#define	OE_ENTER_KEY	3


/*---------------------------------------------------------------------- 
       Prompt user for a string in status line with various options

  Args: string -- the buffer result is returned in, and original string (if 
                   any) is passed in.
        y_base -- y position on screen to start on. 0,0 is upper left
                    negative numbers start from bottom
        x_base -- column position on screen to start on. 0,0 is upper left
        field_len -- Maximum length of string to accept
        append_current -- flag indicating string should not be truncated before
                          accepting input
        passwd -- a pass word is being fetch. Don't echo on screen
        prompt -- The string to prompt with
	escape_list -- pointer to array of ESCKEY_S's.  input chars matching
                       those in list return value from list.
        help   -- Arrary of strings for help text in bottom screen lines
        flags  -- flags

  Result:  editing input string
            returns -1 unexpected errors
            returns 0  normal entry typed (editing and return or PF2)
            returns 1  typed ^C or PF2 (cancel)
            returns 3  typed ^G or PF1 (help)
            returns 4  typed ^L for a screen redraw

  WARNING: Care is required with regard to the escape_list processing.
           The passed array is terminated with an entry that has ch = -1.
           Function key labels and key strokes need to be setup externally!
	   Traditionally, a return value of 2 is used for ^T escapes.

   Unless in escape_list, tabs are trapped by isprint().
This allows near full weemacs style editing in the line
   ^A beginning of line
   ^E End of line
   ^R Redraw line
   ^G Help
   ^F forward
   ^B backward
   ^D delete
----------------------------------------------------------------------*/

optionally_enter(string, y_base, x_base, field_len, append_current, passwd,
                 prompt, escape_list, help, flags)
     char       *string, *prompt;
     ESCKEY_S   *escape_list;
     HelpType	 help;
     int         x_base, y_base, field_len, append_current, passwd;
     unsigned	 flags;
{
    register char *s2;
    register int   field_pos;
    int            i, j, return_v, cols, ch, prompt_len, too_thin,
                   real_y_base, cursor_moved, km_popped;
    char          *saved_original = NULL, *k, *kb;
    char          *kill_buffer = NULL;
    char         **help_text;
    int		   fkey_table[12];
    struct	   key_menu *km;
    bitmap_t	   bitmap;

    dprint(5, (debugfile, "=== optionally_enter called ===\n"));
    dprint(9, (debugfile, "string:\"%s\"  y:%d  x:%d  length: %d append: %d\n",
               string, x_base, y_base, field_len, append_current));
    dprint(9, (debugfile, "passwd:%d   prompt:\"%s\"   label:\"%s\"\n",
               passwd, prompt, (escape_list && escape_list[0].ch != -1)
				 ? escape_list[0].label: ""));

#ifdef _WINDOWS
    if (mswin_usedialog ()) {
	MDlgButton		button_list[12];
	int			b;
	int			i;

	memset (&button_list, 0, sizeof (MDlgButton) * 12);
	b = 0;
	for (i = 0; escape_list && escape_list[i].ch != -1 && i < 11; ++i) {
	    if (escape_list[i].name != NULL
		&& escape_list[i].ch > 0 && escape_list[i].ch < 256) {
		button_list[b].ch = escape_list[i].ch;
		button_list[b].rval = escape_list[i].rval;
		button_list[b].name = escape_list[i].name;
		button_list[b].label = escape_list[i].label;
		++b;
	    }
	}
	button_list[b].ch = -1;


	help_text = get_help_text (help, NULL);
	return_v = mswin_dialog (prompt, string, field_len, 
		    append_current, passwd, button_list, help_text, flags);
	if (help_text != NULL) 
	    free_help_text (help_text);
        return (return_v);
    }
#endif

    suspend_busy_alarm();
    cols       = ps_global->ttyo->screen_cols;
    prompt_len = strlen(prompt);
    too_thin   = 0;
    km_popped  = 0;
    if(y_base > 0) {
        real_y_base = y_base;
    } else {
        real_y_base=  y_base + ps_global->ttyo->screen_rows;
        if(real_y_base < 2)
          real_y_base = ps_global->ttyo->screen_rows;
    }

    flush_ordered_messages();
    mark_status_dirty();
    if(append_current)			/* save a copy in case of cancel */
      saved_original = cpystr(string);

    /*
     * build the function key mapping table, skipping predefined keys...
     */
    memset(fkey_table, NO_OP_COMMAND, 12 * sizeof(int));
    for(i = 0, j = 0; escape_list && escape_list[i].ch != -1 && i+j < 12; i++){
	if(i+j == OE_HELP_KEY)
	  j++;

	if(i+j == OE_CANCEL_KEY)
	  j++;

	if(i+j == OE_ENTER_KEY)
	  j++;

	fkey_table[i+j] = escape_list[i].ch;
    }

#if defined(HELPFILE)
    help_text = (help != NO_HELP) ? get_help_text(help, NULL) : (char **)NULL;
#else
    help_text = help;
#endif
    if(help_text){			/*---- Show help text -----*/
	int width = ps_global->ttyo->screen_cols - x_base;

	if(FOOTER_ROWS(ps_global) == 1){
	    km_popped++;
	    FOOTER_ROWS(ps_global) = 3;
	    clearfooter(ps_global);

	    y_base = -3;
	    real_y_base = y_base + ps_global->ttyo->screen_rows;
	}

	for(j = 0; j < 2 && help_text[j]; j++){
	    MoveCursor(real_y_base + 1 + j, x_base);
	    CleartoEOLN();

	    if(width < strlen(help_text[j])){
		char *tmp = fs_get((width + 1) * sizeof(char));
		strncpy(tmp, help_text[j], width);
		tmp[width] = '\0';
		PutLine0(real_y_base + 1 + j, x_base, tmp);
		fs_give((void **)&tmp);
	    }
	    else
	      PutLine0(real_y_base + 1 + j, x_base, help_text[j]);
	}

#if defined(HELPFILE)
	free_help_text(help_text);
#endif

    } else {
	clrbitmap(bitmap);
	clrbitmap((km = &oe_keymenu)->bitmap);		/* force formatting */
	setbitn(OE_HELP_KEY, bitmap);
	setbitn(OE_ENTER_KEY, bitmap);
        if(!(flags & OE_DISALLOW_CANCEL))
	    setbitn(OE_CANCEL_KEY, bitmap);
	setbitn(OE_CTRL_T_KEY, bitmap);

        /*---- Show the usual possible keys ----*/
	for(i=0,j=0; escape_list && escape_list[i].ch != -1 && i+j < 12; i++){
	    if(i+j == OE_HELP_KEY)
	      j++;

	    if(i+j == OE_CANCEL_KEY)
	      j++;

	    if(i+j == OE_ENTER_KEY)
	      j++;

	    oe_keymenu.keys[i+j].label = escape_list[i].label;
	    oe_keymenu.keys[i+j].name = escape_list[i].name;
	    setbitn(i+j, bitmap);
	}

	for(i = i+j; i < 12; i++)
	  if(!(i == OE_HELP_KEY || i == OE_ENTER_KEY || i == OE_CANCEL_KEY))
	    oe_keymenu.keys[i].name = NULL;

	draw_keymenu(km, bitmap, cols, 1-FOOTER_ROWS(ps_global),
	    0, FirstMenu, 0);
    }
    
    
    StartInverse();  /* Always in inverse  */

    /*
     * if display length isn't wide enough to support input,
     * shorten up the prompt...
     */
    if((dline.dlen = cols - (x_base + prompt_len + 1)) < 5){
	prompt_len += (dline.dlen - 5);	/* adding negative numbers */
	prompt     -= (dline.dlen - 5);	/* subtracting negative numbers */
	dline.dlen  = 5;
    }

    dline.dl    = fs_get((size_t)dline.dlen + 1);
    memset((void *)dline.dl, 0, (size_t)(dline.dlen + 1) * sizeof(char));
    dline.row   = real_y_base;
    dline.col   = x_base + prompt_len;
    dline.vl    = string;
    dline.vlen  = --field_len;		/* -1 for terminating NULL */
    dline.vbase = field_pos = 0;

    PutLine0(real_y_base, x_base, prompt);
    /* make sure passed in string is shorter than field_len */
    /* and adjust field_pos..                               */

    while(append_current && field_pos < field_len && string[field_pos] != '\0')
      field_pos++;

    string[field_pos] = '\0';
    dline.vused = (int)(&string[field_pos] - string);
    line_paint(field_pos, &passwd);

#ifdef	_WINDOWS
    mswin_allowpaste(MSWIN_PASTE_LINE);
#endif

    /*----------------------------------------------------------------------
      The main loop
   
    here field_pos is the position in the string.
    s always points to where we are in the string.
    loops until someone sets the return_v.
      ----------------------------------------------------------------------*/
    return_v = -10;

    while(return_v == -10) {
	/* Timeout 5 min to keep imap mail stream alive */
        ch = read_char(600);

	/*
	 * Don't want to intercept all characters if typing in passwd.
	 * We select an ad hoc set that we will catch and let the rest
	 * through.  We would have caught the set below in the big switch
	 * but we skip the switch instead.  Still catch things like ^K,
	 * DELETE, ^C, RETURN.
	 */
	if(passwd)
	  switch(ch) {
            case ctrl('F'):  
	    case KEY_RIGHT:
            case ctrl('B'):
	    case KEY_LEFT:
            case ctrl('U'):
            case ctrl('A'):
	    case KEY_HOME:
            case ctrl('E'):
	    case KEY_END:
	    case TAB:
	      goto ok_for_passwd;
	  }

        if(too_thin && ch != KEY_RESIZE && ch != ctrl('Z'))
          goto bleep;

	switch(ch) {

	    /*--------------- KEY RIGHT ---------------*/
          case ctrl('F'):  
	  case KEY_RIGHT:
	    if(field_pos >= field_len || string[field_pos] == '\0')
              goto bleep;

	    line_paint(++field_pos, &passwd);
	    break;

	    /*--------------- KEY LEFT ---------------*/
          case ctrl('B'):
	  case KEY_LEFT:
	    if(field_pos <= 0)
	      goto bleep;

	    line_paint(--field_pos, &passwd);
	    break;

          /*-------------------- WORD SKIP --------------------*/
	  case ctrl('@'):
	    /*
	     * Note: read_char *can* return NO_OP_COMMAND which is
	     * the def'd with the same value as ^@ (NULL), BUT since
	     * read_char has a big timeout (>25 secs) it won't.
	     */

	    /* skip thru current word */
	    while(string[field_pos]
		  && isalnum((unsigned char) string[field_pos]))
	      field_pos++;

	    /* skip thru current white space to next word */
	    while(string[field_pos]
		  && !isalnum((unsigned char) string[field_pos]))
	      field_pos++;

	    line_paint(field_pos, &passwd);
	    break;

          /*--------------------  RETURN --------------------*/
	  case PF4:
	    if(F_OFF(F_USE_FK,ps_global)) goto bleep;
	  case ctrl('J'): 
	  case ctrl('M'): 
	    return_v = 0;
	    break;

          /*-------------------- Destructive backspace --------------------*/
	  case '\177': /* DEL */
	  case ctrl('H'):
            /*   Try and do this with by telling the terminal to delete a
                 a character. If that fails, then repaint the rest of the
                 line, acheiving the same much less efficiently
             */
	    if(field_pos <= 0) goto bleep;
	    field_pos--;
	    /* drop thru to pull line back ... */

          /*-------------------- Delete char --------------------*/
	  case ctrl('D'): 
	  case KEY_DEL: 
            if(field_pos >= field_len || !string[field_pos]) goto bleep;

	    dline.vused--;
	    for(s2 = &string[field_pos]; *s2 != '\0'; s2++)
	      *s2 = s2[1];

	    *s2 = '\0';			/* Copy last NULL */
	    line_paint(field_pos, &passwd);
	    break;


            /*--------------- Kill line -----------------*/
          case ctrl('K'):
            if(kill_buffer != NULL)
              fs_give((void **)&kill_buffer);

	    if(field_pos != 0 || string[0]){
		if(!passwd && F_ON(F_DEL_FROM_DOT, ps_global))
		  dline.vused -= strlen(&string[i = field_pos]);
		else
		  dline.vused = i = 0;

		kill_buffer = cpystr(&string[field_pos = i]);
		string[field_pos] = '\0';
		line_paint(field_pos, &passwd);
	    }

            break;

            /*------------------- Undelete line --------------------*/
          case ctrl('U'):
            if(kill_buffer == NULL)
              goto bleep;

            /* Make string so it will fit */
            kb = cpystr(kill_buffer);
            dprint(2, (debugfile,
		       "Undelete: %d %d\n", strlen(string), field_len));
            if(strlen(kb) + strlen(string) > field_len) 
                kb[field_len - strlen(string)] = '\0';
            dprint(2, (debugfile,
		       "Undelete: %d %d\n", field_len - strlen(string),
		       strlen(kb)));
                       
            if(string[field_pos] == '\0') {
                /*--- adding to the end of the string ----*/
                for(k = kb; *k; k++)
		  string[field_pos++] = *k;

                string[field_pos] = '\0';
            } else {
                goto bleep;
                /* To lazy to do insert in middle of string now */
            }

	    dline.vused = strlen(string);
            fs_give((void **)&kb);
	    line_paint(field_pos, &passwd);
            break;
            

	    /*-------------------- Interrupt --------------------*/
	  case ctrl('C'): /* ^C */ 
	    if(F_ON(F_USE_FK,ps_global) || flags & OE_DISALLOW_CANCEL)
	      goto bleep;

	    goto cancel;
	  case PF2:
	    if(F_OFF(F_USE_FK,ps_global) || flags & OE_DISALLOW_CANCEL)
	      goto bleep;

	  cancel:
	    return_v = 1;
	    if(saved_original)
	      strcpy(string, saved_original);

	    break;
	    

          case ctrl('A'):
	  case KEY_HOME:
            /*-------------------- Start of line -------------*/
	    line_paint(field_pos = 0, &passwd);
            break;


          case ctrl('E'):
	  case KEY_END:
            /*-------------------- End of line ---------------*/
	    line_paint(field_pos = dline.vused, &passwd);
            break;


	    /*-------------------- Help --------------------*/
	  case ctrl('G') : 
	  case PF1:
	    if(FOOTER_ROWS(ps_global) == 1 && km_popped == 0){
		km_popped++;
		FOOTER_ROWS(ps_global) = 3;
		clearfooter(ps_global);
		EndInverse();
		draw_keymenu(km, bitmap, cols, 1-FOOTER_ROWS(ps_global),
		    0, FirstMenu, 0);
		StartInverse();
		mark_keymenu_dirty();
		y_base = -3;
		dline.row = real_y_base = y_base + ps_global->ttyo->screen_rows;
		PutLine0(real_y_base, x_base, prompt);
		fs_resize((void **)&dline.dl, (size_t)dline.dlen + 1);
		memset((void *)dline.dl, 0, (size_t)(dline.dlen + 1));
		line_paint(field_pos, &passwd);
		break;
	    }

	    if(FOOTER_ROWS(ps_global) > 1){
		mark_keymenu_dirty();
		return_v = 3;
	    }
	    else
	      goto bleep;

	    break;

          case NO_OP_IDLE:
            if(new_mail(0, 2, 0) < 0)	/* Keep mail stream alive */
              break;			/* no changes, get on with life */
            /* Else fall into redraw */

	    /*-------------------- Redraw --------------------*/
	  case ctrl('L'):
            /*---------------- re size ----------------*/
          case KEY_RESIZE:
            
	    dline.row = real_y_base = y_base > 0 ? y_base :
					 y_base + ps_global->ttyo->screen_rows;
            EndInverse();
            ClearScreen();
            redraw_titlebar();
            if(ps_global->redrawer != (void (*)())NULL)
              (*ps_global->redrawer)();

            redraw_keymenu();
            StartInverse();
            
            PutLine0(real_y_base, x_base, prompt);
            cols     =  ps_global->ttyo->screen_cols;
            too_thin = 0;
            if(cols < x_base + prompt_len + 4) {
		Writechar(BELL, 0);
                PutLine0(real_y_base, 0, "Screen's too thin. Ouch!");
                too_thin = 1;
            } else {
		dline.col   = x_base + prompt_len;
		dline.dlen  = cols - (x_base + prompt_len + 1);
		fs_resize((void **)&dline.dl, (size_t)dline.dlen + 1);
		memset((void *)dline.dl, 0, (size_t)(dline.dlen + 1));
		line_paint(field_pos, &passwd);
            }
            fflush(stdout);

            dprint(9, (debugfile,
                    "optionally_enter  RESIZE new_cols:%d  too_thin: %d\n",
                       cols, too_thin));
            break;

	  case PF3 :		/* input to potentially remap */
	  case PF5 :
	  case PF6 :
	  case PF7 :
	  case PF8 :
	  case PF9 :
	  case PF10 :
	  case PF11 :
	  case PF12 :
	      if(F_ON(F_USE_FK,ps_global)
		 && fkey_table[ch - PF1] != NO_OP_COMMAND)
		ch = fkey_table[ch - PF1]; /* remap function key input */
  
          default:
	    if(escape_list){		/* in the escape key list? */
		for(j=0; escape_list[j].ch != -1; j++){
		    if(escape_list[j].ch == ch){
			return_v = escape_list[j].rval;
			break;
		    }
		}

		if(return_v != -10)
		  break;
	    }

	    if(iscntrl(ch & 0x7f)){
       bleep:
		putc(BELL, stdout);
		continue;
	    }

       ok_for_passwd:
	    /*--- Insert a character -----*/
	    if(dline.vused >= field_len)
	      goto bleep;

	    /*---- extending the length of the string ---*/
	    for(s2 = &string[++dline.vused]; s2 - string > field_pos; s2--)
	      *s2 = *(s2-1);

	    string[field_pos++] = ch;
	    line_paint(field_pos, &passwd);
		    
	}   /*---- End of switch on char ----*/
    }

#ifdef	_WINDOWS
    mswin_allowpaste(MSWIN_PASTE_DISABLE);
#endif
    fs_give((void **)&dline.dl);
    if(saved_original) 
      fs_give((void **)&saved_original);

    if(kill_buffer)
      fs_give((void **)&kill_buffer);

    removing_trailing_white_space(string);
    EndInverse();
    MoveCursor(real_y_base, x_base); /* Move the cursor to show we're done */
    fflush(stdout);
    resume_busy_alarm();
    if(km_popped){
	FOOTER_ROWS(ps_global) = 1;
	clearfooter(ps_global);
	ps_global->mangled_body = 1;
    }

    return(return_v);
}


/*
 * line_paint - where the real work of managing what is displayed gets done.
 *              The passwd variable is overloaded: if non-zero, don't
 *              output anything, else only blat blank chars across line
 *              once and use this var to tell us we've already written the 
 *              line.
 */
void
line_paint(offset, passwd)
    int   offset;			/* current dot offset into line */
    int  *passwd;			/* flag to hide display of chars */
{
    register char *pfp, *pbp;
    register char *vfp, *vbp;
    int            extra = 0;
#define DLEN	(dline.vbase + dline.dlen)

    /*
     * for now just leave line blank, but maybe do '*' for each char later
     */
    if(*passwd){
	if(*passwd > 1)
	  return;
	else
	  *passwd == 2;		/* only blat once */

	extra = 0;
	MoveCursor(dline.row, dline.col);
	while(extra++ < dline.dlen)
	  Writechar(' ', 0);

	MoveCursor(dline.row, dline.col);
	return;
    }

    /* adjust right margin */
    while(offset >= DLEN + ((dline.vused > DLEN) ? -1 : 1))
      dline.vbase += dline.dlen/2;

    /* adjust left margin */
    while(offset < dline.vbase + ((dline.vbase) ? 2 : 0))
      dline.vbase = max(dline.vbase - (dline.dlen/2), 0);

    if(dline.vbase){				/* off screen cue left */
	vfp = &dline.vl[dline.vbase+1];
	pfp = &dline.dl[1];
	if(dline.dl[0] != '<'){
	    MoveCursor(dline.row, dline.col);
	    Writechar(dline.dl[0] = '<', 0);
	}
    }
    else{
	vfp = dline.vl;
	pfp = dline.dl;
	if(dline.dl[0] == '<'){
	    MoveCursor(dline.row, dline.col);
	    Writechar(dline.dl[0] = ' ', 0);
	}
    }

    if(dline.vused > DLEN){			/* off screen right... */
	vbp = vfp + (long)(dline.dlen-(dline.vbase ? 2 : 1));
	pbp = pfp + (long)(dline.dlen-(dline.vbase ? 2 : 1));
	if(pbp[1] != '>'){
	    MoveCursor(dline.row, dline.col+dline.dlen);
	    Writechar(pbp[1] = '>', 0);
	}
    }
    else{
	extra = dline.dlen - (dline.vused - dline.vbase);
	vbp = &dline.vl[max(0, dline.vused-1)];
	pbp = &dline.dl[dline.dlen];
	if(pbp[0] == '>'){
	    MoveCursor(dline.row, dline.col+dline.dlen);
	    Writechar(pbp[0] = ' ', 0);
	}
    }

    while(*pfp == *vfp && vfp < vbp)			/* skip like chars */
      pfp++, vfp++;

    if(pfp == pbp && *pfp == *vfp){			/* nothing to paint! */
	MoveCursor(dline.row, dline.col + (offset - dline.vbase));
	return;
    }

    /* move backward thru like characters */
    if(extra){
	while(extra >= 0 && *pbp == ' ') 		/* back over spaces */
	  extra--, pbp--;

	while(extra >= 0)				/* paint new ones    */
	  pbp[-(extra--)] = ' ';
    }

    if((vbp - vfp) == (pbp - pfp)){			/* space there? */
	while((*pbp == *vbp) && pbp != pfp)		/* skip like chars */
	  pbp--, vbp--;
    }

    if(pfp != pbp || *pfp != *vfp){			/* anything to paint?*/
	MoveCursor(dline.row, dline.col + (int)(pfp - dline.dl));

	do
	  Writechar((unsigned char)((vfp <= vbp && *vfp)
		      ? ((*pfp = *vfp++) == TAB) ? ' ' : *pfp
		      : (*pfp = ' ')), 0);
	while(++pfp <= pbp);
    }

    MoveCursor(dline.row, dline.col + (offset - dline.vbase));
}



/*----------------------------------------------------------------------
    Check to see if the given command is reasonably valid
  
  Args:  ch -- the character to check

 Result:  A valid command is returned, or a well know bad command is returned.
 
 ---*/
validatekeys(ch)
     int  ch;
{
#ifndef _WINDOWS
    if(F_ON(F_USE_FK,ps_global)) {
	if(ch >= 'a' && ch <= 'z')
	  return(KEY_JUNK);
    } else {
	if(ch >= PF1 && ch <= PF12)
	  return(KEY_JUNK);
    }
#else
    /*
     * In windows menu items are bound to a single key command which
     * gets inserted into the input stream as if the user had typed
     * that key.  But all the menues are bonund to alphakey commands,
     * not PFkeys.  to distinguish between a keyboard command and a
     * menu command we insert a flag (KEY_MENU_FLAG) into the
     * command value when setting up the bindings in
     * configure_menu_items().  Here we strip that flag.
     */
    if(F_ON(F_USE_FK,ps_global)) {
	if(ch >= 'a' && ch <= 'z' && !(ch & KEY_MENU_FLAG))
	  return(KEY_JUNK);
	ch &= ~ KEY_MENU_FLAG;
    } else {
	ch &= ~ KEY_MENU_FLAG;
	if(ch >= PF1 && ch <= PF12)
	  return(KEY_JUNK);
    }
#endif

    return(ch);
}



/*----------------------------------------------------------------------
  Prepend config'd commands to keyboard input
  
  Args:  ch -- pointer to storage for returned command

 Returns: TRUE if we're passing back a useful command, FALSE otherwise
 
 ---*/
int
process_config_input(ch)
    int *ch;
{
    static char firsttime = (char) 1;

    /* commands in config file */
    if(ps_global->initial_cmds && *ps_global->initial_cmds) {
	/*
	 * There are a few commands that may require keyboard input before
	 * we enter the main command loop.  That input should be interactive,
	 * not from our list of initial keystrokes.
	 */
	if(ps_global->dont_use_init_cmds)
	  return(0);

	*ch = *ps_global->initial_cmds++;
	if(!*ps_global->initial_cmds && ps_global->free_initial_cmds){
	    fs_give((void **)&(ps_global->free_initial_cmds));
	    ps_global->initial_cmds = 0;
	}

	return(1);
    }

    if(firsttime) {
	firsttime = 0;
	if(ps_global->in_init_seq) {
	    ps_global->in_init_seq = 0;
	    ps_global->save_in_init_seq = 0;
	    clear_cursor_pos();
	    F_SET(F_USE_FK,ps_global,ps_global->orig_use_fkeys);
	    /* draw screen */
	    *ch = ctrl('L');
	    return(1);
	}
    }

    return(0);
}



/*----------------------------------------------------------------------
    record and playback user keystrokes
  
  Args:  ch -- the character to record
	 play -- flag to tell us to return first recorded char on tape

 Returns: either character recorded or played back or -1 to indicate
	  end of recording
 
 ---*/
#define	TAPELEN	256

int
key_recorder(ch, play)
    int  ch;
    int  play;
{
    static int	 tape[TAPELEN];
    static long  recorded = 0L;
    static short length  = 0;

    if(play){
	ch = length ? tape[(recorded + TAPELEN - length--) % TAPELEN] : -1;
    }
    else{
	tape[recorded++ % TAPELEN] = ch;
	if(length < TAPELEN)
	  length++;
    }

    return(ch);
}
