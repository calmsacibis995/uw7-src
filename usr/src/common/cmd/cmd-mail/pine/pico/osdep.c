#if	!defined(lint) && !defined(DOS)
static char rcsid[] = "$Id$";
#endif
/*
 * Program:	Operating system dependent routines - Ultrix 4.1
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
 *
 * Notes:
 *
 * - SGI IRIX 4.0.1 port by:
 *       johnb@edge.cis.mcmaster.ca,  2 April 1992
 *
 * - Dynix/PTX port by:
 *       Donn Cave, UCS/UW, 15 April 1992
 *
 * - 3B2, 3b1/7300, SCO ports by:
 *       rll@felton.felton.ca.us, 7 Feb. 1993
 *
 * - Altos System V (asv) port by:
 *	 Tim Rice <tim@trr.metro.net>    6 Mar 96
 *
 * - Probably have to break this up into separate os_type.c files since
 *   the #ifdef's are getting a bit cumbersome.
 *
 */

#include 	<stdio.h>
#include	<errno.h>
#include	<setjmp.h>
#include	<pwd.h>
#if	defined(sv4) || defined(ptx)
#include	<stropts.h>
#include	<poll.h>
#endif
#if	defined(POSIX)
#include	<termios.h>
#if	defined(a32) || defined(a41) || defined(cvx) || defined(osf)
#include	<sys/ioctl.h>
#endif
#else
#if	defined(sv3) || defined(sgi) || defined(isc) || defined(ct) || defined(asv)
#include	<termio.h>
#if	defined(isc)
#include	<sys/sioctl.h>
#include        <sys/bsdtypes.h>
#endif
#else
#include	<sgtty.h>
#endif
#endif	/* POSIX */

#include	"osdep.h"
#include        "pico.h"
#include	"estruct.h"
#include        "edef.h"
#include        "efunc.h"
#include	<fcntl.h>
#include	<sys/wait.h>
#include	<sys/file.h>
#include	<sys/types.h>
#include	<sys/time.h>
#if	defined(a32) || defined(a41) || defined(dpx)
#include	<sys/select.h>
#endif

int timeout = 0;

/*
 * Immediately below are includes and declarations for the 3 basic
 * terminal drivers supported; POSIX, SysVR3, and BSD
 */
#ifdef	POSIX

struct termios nstate,
		ostate;
#else
#if	defined(sv3) || defined(sgi) || defined(isc) || defined(asv)

struct termio nstate,
              ostate;

#else
struct  sgttyb  ostate;				/* saved tty state */
struct  sgttyb  nstate;				/* values for editor mode */
struct  ltchars	oltchars;			/* old term special chars */
struct  ltchars	nltchars = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
struct  tchars	otchars;			/* old term special chars */
struct  tchars	ntchars = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

#endif	/* sv3 || sgi || isc || asv */
#endif	/* POSIX */

#if	defined(sv3) || defined(ct)
/*
 * Windowing structure to support JWINSIZE/TIOCSWINSZ/TIOCGWINSZ 
 */
#define ENAMETOOLONG	78

struct winsize {
	unsigned short ws_row;       /* rows, in characters*/
	unsigned short ws_col;       /* columns, in character */
	unsigned short ws_xpixel;    /* horizontal size, pixels */
	unsigned short ws_ypixel;    /* vertical size, pixels */
};
#endif

#if	defined(bsd) || defined(lnx)
int	errno;					/* ya, I know... */
#endif

#if	defined(bsd) || defined(dyn) || defined(ct)
#define	SIGTYPE int
#else
#define	SIGTYPE	void
#endif

#ifdef	SIGCHLD
static jmp_buf pico_child_state;
static short   pico_child_jmp_ok, pico_child_done;
#endif

#ifdef	MOUSE
static int mexist = 0;		/* is the mouse driver installed? */
static unsigned mnoop;
#endif

#ifdef	ANSI
    int      kbseq(KBESC_T *, int (*)(), int *);
    SIGTYPE  do_hup_signal();
    SIGTYPE  rtfrmshell();
#ifdef	TIOCGWINSZ
    SIGTYPE  winch_handler();
#endif
#ifdef	SIGCHLD
    SIGTYPE  child_handler();
#endif

#else
    int      kbseq();
    SIGTYPE  do_hup_signal();
    SIGTYPE  rtfrmshell();
#ifdef	TIOCGWINSZ
    SIGTYPE  winch_handler();
#endif
#ifdef	SIGCHLD
    SIGTYPE  child_handler();
#endif

#endif	/* ANSI */


/*
 * for alt_editor arg[] building
 */
#define	MAXARGS	10

/*
 * ttopen - this function is called once to set up the terminal device 
 *          streams.  if called as pine composer, don't mess with
 *          tty modes, but set signal handlers.
 */
ttopen()
{
    if(Pmaster == NULL){
#ifdef	POSIX
	tcgetattr (0, &ostate);
	tcgetattr (0, &nstate);
	nstate.c_lflag &= ~(ISIG | ICANON | ECHO | IEXTEN);
	nstate.c_iflag &= ~ICRNL;
	nstate.c_oflag &= ~(ONLCR | OPOST);
	nstate.c_cc[VMIN] = 1;
	nstate.c_cc[VTIME] = 0;
	tcsetattr (0, TCSADRAIN, &nstate);
#else
#if	defined(sv3) || defined(sgi) || defined(isc) || defined(ct) || defined(asv)
	(void) ioctl(0, TCGETA, &ostate);
	(void) ioctl(0, TCGETA, &nstate);	/** again! **/

	nstate.c_lflag &= ~(ICANON | ISIG | ECHO);	/* noecho raw mode  */
	nstate.c_oflag &= ~(OPOST | ONLCR);
	nstate.c_iflag &= ~ICRNL;
	    
	nstate.c_cc[VMIN] = '\01';  /* minimum # of chars to queue  */
	nstate.c_cc[VTIME] = '\0'; /* minimum time to wait for input */
	(void) ioctl(0, TCSETA, &nstate);
#else
	ioctl(0, TIOCGETP, &ostate);		/* save old state */
	ioctl(0, TIOCGLTC, &oltchars);		/* Save old lcharacters */
	ioctl(0, TIOCGETC, &otchars);		/* Save old characters */
	ioctl(0, TIOCGETP, &nstate);		/* get base of new state */
	nstate.sg_flags |= RAW;
	nstate.sg_flags &= ~(ECHO|CRMOD);	/* no echo for now... */
	ioctl(0, TIOCSETP, &nstate);		/* set mode */

	ioctl(0, TIOCSLTC, &nltchars);		/* put new lcharacter into K */
	ioctl(0, TIOCSETC, &ntchars);		/* put new character into K */
#endif	/* sv3 */
#endif	/* POSIX */
#ifdef	MOUSE
	if(gmode & MDMOUSE)
	  init_mouse();
#endif	/* MOUSE */
    }

#ifdef	MOUSE
    if(mexist)
      kpinsert(&pico_kbesc, "\033[M", K_XTERM_MOUSE);
#endif	/* MOUSE */

    if(!Pmaster || (gmode ^ MDEXTFB))
      picosigs();

    return(1);
}


/*
 * ttresize - recompute the screen dimensions if necessary, and then
 *	      adjust pico's internal buffers accordingly.
 */
int
ttresize ()
{
    int row = -1, col = -1;

    ttgetwinsz(&row, &col);
    resize_pico(row, col);
}


/*
 * picosigs - Install any handlers for the signals we're interested
 *	      in catching.
 */
picosigs()
{
    signal(SIGHUP,  do_hup_signal);	/* deal with SIGHUP */
    signal(SIGTERM, do_hup_signal);	/* deal with SIGTERM */
#ifdef	SIGTSTP
    signal(SIGTSTP, SIG_DFL);
#endif
#ifdef	TIOCGWINSZ
    signal(SIGWINCH, winch_handler); /* window size changes */
#endif
}


/*
 * ttclose - this function gets called just before we go back home to 
 *           the command interpreter.  If called as pine composer, don't
 *           worry about modes, but set signals to default, pine will 
 *           rewire things as needed.
 */
ttclose()
{
    if(Pmaster){
	signal(SIGHUP, SIG_DFL);
#ifdef	SIGCONT
	signal(SIGCONT, SIG_DFL);
#endif
#ifdef	TIOCGWINSZ
	signal(SIGWINCH, SIG_DFL);
#endif
    }
    else{
#ifdef	POSIX
	tcsetattr (0, TCSADRAIN, &ostate);
#else
#if	defined(sv3) || defined(sgi) || defined(isc) || defined(ct) || defined(asv)
        ioctl(0, TCSETA, &ostate);
#else
	ioctl(0, TIOCSETP, &ostate);
	ioctl(0, TIOCSLTC, &oltchars);
	ioctl(0, TIOCSETC, &otchars);

	/*
	 * This works around a really weird problem.  On slow speed lines,
	 * if an exit happens with some number of characters still to be
	 * written in the terminal driver, one or more characters will 
	 * be changed when they finally get drained.  This can be reproduced
	 * on a 2400bps line, writing a multi-line buffer on exit using
	 * a vt100 type terminal.  It turns out the last char in the
	 * escape sequence turning off reverse video was getting changed
	 * from 'm' to ' '.  I said it was weird.
	 */
	if(ostate.sg_ospeed <= B2400)
	  sleep(1);
#endif
#endif	/* POSIX */
#ifdef	MOUSE
	end_mouse();
#endif
    }

    return(1);
}


/*
 * ttspeed - return TRUE if tty line speed < 9600 else return FALSE
 */
ttisslow()
{
#if	defined(POSIX)
    struct termios tty;

    return((tcgetattr (1, &tty) == 0) ? cfgetospeed (&tty) < B9600 : FALSE);
#else
#if	defined(sv3) || defined(sgi) || defined(isc) || defined(asv)
    struct termio tty;

#ifdef asv
    return((ioctl(1, TCGETA, &tty) == 0) ? (tty.c_cflag && CBAUD) < B9600 : FALSE);
#else /* asv */
    return((tcgetattr (1, &tty) == 0) ? cfgetospeed (&tty) < B9600 : FALSE);
#endif /* asv */
#else
    struct  sgttyb tty;

    return((ioctl(1, TIOCGETP, &tty) == 0) ? tty.sg_ospeed < B9600 : FALSE);
#endif
#endif
}


/*
 * ttgetwinsz - set global row and column values (if we can get them)
 *		and return.
 */
ttgetwinsz(row, col)
    int *row, *col;
{
    if(*row < 0)
      *row = NROW - 1;
    if(*col <= 0)
      *col = NCOL;
#ifdef TIOCGWINSZ
    {
	struct winsize win;

	if (ioctl(0, TIOCGWINSZ, &win) == 0) {	/* set to anything useful.. */
	    if(win.ws_row)			/* ... the tty drivers says */
	      *row = win.ws_row - 1;

	    if(win.ws_col)
	      *col = win.ws_col;
	}

	signal(SIGWINCH, winch_handler);	/* window size changes */
    }
#endif
}


/*
 * ttputc - Write a character to the display. 
 */
ttputc(c)
{
    return(putc(c, stdout));
}


/*
 * ttflush - flush terminal buffer. Does real work where the terminal 
 *           output is buffered up. A no-operation on systems where byte 
 *           at a time terminal I/O is done.
 */
ttflush()
{
    return(fflush(stdout));
}


/*
 * ttgetc - Read a character from the terminal, performing no editing 
 *          and doing no echo at all.
 */
ttgetc()
{
    unsigned char c;
    int i;

    if((i = read(0, &c, 1)) == 1)			/* success */
      return((int) c);
    else if(i == 0 || errno != EINTR)
      kill(getpid(), SIGHUP);				/* eof or bad error */

    return(NODATA);					/* acceptable error */
}


#if	TYPEAH
/* 
 * typahead - Check to see if any characters are already in the
 *	      keyboard buffer
 */
typahead()
{
    int x;	/* holds # of pending chars */

    return((ioctl(0,FIONREAD,&x) < 0) ? 0 : x);
}
#endif


/*
 * ReadyForKey - return true if there's no timeout or we're told input
 *		 is available...
 */
ReadyForKey(timeout)
    int timeout;
{
    if(timeout > 0){
	/*
	 * simple use of select/poll here to handle requested timeouts
	 * while waiting for keyboard input...
	 */
#if	defined(ptx) || defined(sv4)
	struct pollfd pollfd;
	int    rv;

	pollfd.fd     = 0;
	pollfd.events = POLLIN;
	while((rv = poll(&pollfd, 1, timeout * 1000)) < 0 && errno == EAGAIN)
	  ;
#else
	struct timeval ts;
	fd_set readfds;
	int    rv;

	FD_ZERO(&readfds);		/* blank out all bits */
	FD_SET(0, &readfds);		/* set stdin's bit */
	ts.tv_sec  = timeout;		/* set the timeout */
	ts.tv_usec = 0;

	rv = select(1, &readfds, 0, &readfds, &ts); /* read stdin */
#endif
	if(rv < 0){
	    if(errno == EINTR){		/* interrupted? */
		return(0);		/* return like we timed out */
	    }
	    else{
		emlwrite("\007Problem reading from keyboard!", NULL);
		kill(getpid(), SIGHUP);	/* Bomb out (saving our work)! */
	    }
	}
	else if(rv == 0)
	  return(0);			/* we really did time out */
    }

    return(1);
}


/*
 * GetKey - Read in a key.
 * Do the standard keyboard preprocessing. Convert the keys to the internal
 * character set.  Resolves escape sequences and returns no-op if global
 * timeout value exceeded.
 */
GetKey()
{
    int    c, status, cc;

    if(!ReadyForKey(timeout))
      return(NODATA);

    switch(status = kbseq(pico_kbesc, term.t_getchar, &c)){
      case 0: 	/* regular character */
	break;

      case K_DOUBLE_ESC:
	c = (*term.t_getchar)();

	if(isdigit((unsigned char)c)){
	    int n = 0, i = c - '0';

	    if(!strchr("012", c))
	      return(c);		/* bogus literal char value */

	    while(n++ < 2){
		c = (*term.t_getchar)();
					/* remember Horner? */
		if(!isdigit((unsigned char)c) || (n == 1 && i == 2 && c > '5'))
		  return(c);		/* bogus literal char value */

		i = (i * 10) + (c - '0');
	    }

	    c = i;
	    break;
	}
	else{
	    if(islower((unsigned char)c))	/* canonicalize c */
	      c = toupper((unsigned char)c);

	    return((isalpha((unsigned char)c) || c == '@'
		    || (c >= '[' && c <= '_'))
		    ? (CTRL | c) : ((c == ' ') ? (CTRL | '@') : c));
	}

#ifdef MOUSE
      case K_XTERM_MOUSE:
	{
	    static int down = 0;
	    int        x, y, button;
	    unsigned   ch;

	    button = (*term.t_getchar)() & 0x03;
	    c = (*term.t_getchar)();
	    x = c - '!';
	    c = (*term.t_getchar)();
	    y = c - '!';
	    if(button == 0){
		down = 1;
		if(checkmouse(&ch, 1, x, y))
		  return(ch);
	    }
	    else if(down && button == 3){
		down = 0;
		if(checkmouse(&ch, 0, x, y))
		  return(ch);
	    }

	    return(NODATA);
	}

	break;
#endif /* MOUSE */

      case  K_PAD_UP		:
      case  K_PAD_DOWN		:
      case  K_PAD_RIGHT		:
      case  K_PAD_LEFT		:
      case  K_PAD_PREVPAGE	:
      case  K_PAD_NEXTPAGE	:
      case  K_PAD_HOME		:
      case  K_PAD_END		:
      case  K_PAD_DELETE	:
      case F1  :
      case F2  :
      case F3  :
      case F4  :
      case F5  :
      case F6  :
      case F7  :
      case F8  :
      case F9  :
      case F10 :
      case F11 :
      case F12 :
	return(status);

      case K_SWALLOW_TIL_Z:
	status = BADESC;
      case K_SWALLOW_UP:
      case K_SWALLOW_DOWN:
      case K_SWALLOW_LEFT:
      case K_SWALLOW_RIGHT:
	do
	  if(!ReadyForKey(2)){
	      status = BADESC;
	      break;
	  }
	while(!strchr("~qz", (*term.t_getchar)()));

	return((status == BADESC)
		? status
		: status - (K_SWALLOW_UP - K_PAD_UP));
	break;

      case K_KERMIT:
	for(cc = 0; cc != '\033' || ((*term.t_getchar)() & 0x7f) != '\\';)
	  cc = (*term.t_getchar)() & 0x7f;

	c = NODATA;
	break;

      case BADESC:
	(*term.t_beep)();
	return(status);

      default:				/* punt the whole thing	*/
	(*term.t_beep)();
	break;
    }

    if (c>=0x00 && c<=0x1F)                 /* C0 control -> C-     */
      c = CTRL | (c+'@');

    return (c);
}



/* 
 * kbseq - looks at an escape sequence coming from the keyboard and 
 *         compares it to a trie of known keyboard escape sequences, and
 *         performs the function bound to the escape sequence.
 * 
 *         returns: BADESC, the escaped function, or 0 if a regular char.
 */
kbseq(trie, getcfunc, c)
    KBESC_T *trie;
    int    (*getcfunc)();
    int	    *c;
{
    register char     b;
    register int      first = 1;
    register KBESC_T *current = trie;

    if(trie == NULL)				/* bag it */
      return(BADESC);

    while(1){
	b = *c = (*getcfunc)();

	while(current->value != b){
	    if(current->left == NULL){		/* NO MATCH */
		if(first)
		  return(0);			/* regular char */
		else
		  return(BADESC);
	    }
	    current = current->left;
	}

	if(current->down == NULL)		/* match!!!*/
	  return(current->func);
	else
	  current = current->down;

	first = 0;
    }
}



#define	newnode()	(KBESC_T *)malloc(sizeof(KBESC_T))
/*
 * kpinsert - insert a keystroke escape sequence into the global search
 *	      structure.
 */
void
kpinsert(trie, kstr, kval)
    KBESC_T **trie;
    char     *kstr;
    int       kval;
{
    register	char	*buf;
    register	KBESC_T *temp;
    register	KBESC_T *trail;

    if(kstr == NULL)
      return;

#ifndef TERMCAP_WINS
    /*
     * Don't allow escape sequences that don't start with ESC unless
     * TERMCAP_WINS is defined.  This is to protect against mistakes
     * in termcap files.
     */
    if(*kstr != '\033')
      return;
#endif /* !TERMCAP_WINS */

    temp = trail = *trie;
    buf = kstr;

    for(;;){
	if(temp == NULL){
	    temp = newnode();
	    temp->value = *buf;
	    temp->func = 0;
	    temp->left = NULL;
	    temp->down = NULL;
	    if(*trie == NULL)
	      *trie = temp;
	    else
	      trail->down = temp;
	}
	else{				/* first entry */
	    while((temp != NULL) && (temp->value != *buf)){
		trail = temp;
		temp = temp->left;
	    }

	    if(temp == NULL){   /* add new val */
		temp = newnode();
		temp->value = *buf;
		temp->func = 0;
		temp->left = NULL;
		temp->down = NULL;
		trail->left = temp;
	    }
	}

	if(*(++buf) == '\0')
	  break;
	else{
	    /*
	     * Ignore attempt to overwrite shorter existing escape sequence.
	     * That means that sequences with higher priority should be
	     * set up first, so if we want termcap sequences to override
	     * hardwired sequences, put the kpinsert calls for the
	     * termcap sequences first.  (That's what you get if you define
	     * TERMCAP_WINS.)
	     */
	    if(temp->func != 0)
	      return;

	    trail = temp;
	    temp = temp->down;
	}
    }
    
    /*
     * Ignore attempt to overwrite longer sequences we are a prefix
     * of (down != NULL) and exact same sequence (func != 0).
     */
    if(temp != NULL && temp->down == NULL && temp->func == 0)
      temp->func = kval;
}



/*
 * kbdestroy() - kills the key pad function key search tree
 *		 and frees all lines associated with it
 */
void
kbdestroy(kb)
    KBESC_T *kb;
{
    if(kb){
	kbdestroy(kb->left);
	kbdestroy(kb->down);
	free((char *) kb);
    }
}



/*
 * alt_editor - fork off an alternate editor for mail message composition 
 *              if one is configured and passed from pine.  If not, only
 *              ask for the editor if advanced user flag is set, and 
 *              suggest environment's EDITOR value as default.
 */
alt_editor(f, n)
{
    char   eb[NLINE];				/* buf holding edit command */
    char   *fn;					/* tmp holder for file name */
    char   *cp;
    char   *args[MAXARGS];			/* ptrs into edit command */
    char   result[128];				/* result string */
    int	   child, pid, i, done = 0;
    long   l;
    FILE   *p;
    SIGTYPE (*ohup)(), (*oint)(), (*osize)(), (*ostop)(), (*ostart)();
#if	defined(POSIX) || defined(sv3) || defined(COHERENT) || defined(isc) || defined(neb)
    int    stat;
#ifndef	WIFEXITED
#define	WIFEXITED(X)	(!((X) & 0xff))		/* low bits, child killed */
#endif
#ifndef	WEXITSTATUS
#define	WEXITSTATUS(X)	((X) >> 8)		/* high bits, exit value */
#endif
#else
    union  wait stat;
#ifndef	WIFEXITED
#define	WIFEXITED(X)	(!(X).w_termsig)	/* nonzero if child killed */
#endif
#ifndef	WEXITSTATUS
#define	WEXITSTATUS(X)	X.w_retcode		/* child's exit value */
#endif
#endif

    if(Pmaster == NULL)
      return(-1);

    if(gmode&MDSCUR){
	emlwrite("Alternate %s not available in restricted mode",
		 f ? "speller" : "editor");
	return(-1);
    }

    strcpy(result, "Alternate %s complete.");

    if(f){
	if(alt_speller)
	  strcpy(eb, alt_speller);
	else
	  return(-1);
    }
    else{
	if(Pmaster->alt_ed == NULL){
	    if(!(gmode&MDADVN)){
		emlwrite("\007Unknown Command",NULL);
		return(-1);
	    }

	    if(getenv("EDITOR"))
	      strcpy(eb, (char *)getenv("EDITOR"));
	    else
	      *eb = '\0';

	    while(!done){
		pid = mlreplyd("Which alternate editor ? ", eb,
			       NLINE, QDEFLT, NULL);
		switch(pid){
		  case ABORT:
		    return(-1);
		  case HELPCH:
		    emlwrite("no alternate editor help yet", NULL);

/* take sleep and break out after there's help */
		    sleep(3);
		    break;
		  case (CTRL|'L'):
		    sgarbf = TRUE;
		    update();
		    break;
		  case TRUE:
		  case FALSE:			/* does editor exist ? */
		    if(*eb == '\0'){		/* leave silently? */
			mlerase();
			return(-1);
		    }

		    done++;
		    break;
		    default:
		    break;
		}
	    }
	}
	else
	  strcpy(eb, Pmaster->alt_ed);
    }

    if((fn=writetmp(0, 1)) == NULL){		/* get temp file */
	emlwrite("Problem writing temp file for alt editor", NULL);
	return(-1);
    }

    strcat(eb, " ");
    strcat(eb, fn);

    cp = eb;
    for(i=0; *cp != '\0';i++){			/* build args array */
	if(i < MAXARGS){
	    args[i] = NULL;			/* in case we break out */
	}
	else{
	    emlwrite("Too many args for command!", NULL);
	    return(-1);
	}

	while(isspace((unsigned char)(*cp)))
	  if(*cp != '\0')
	    cp++;
	  else
	    break;

	args[i] = cp;

	while(!isspace((unsigned char)(*cp)))
	  if(*cp != '\0')
	    cp++;
	  else
	    break;

	if(*cp != '\0')
	  *cp++ = '\0';
    }

    args[i] = NULL;

    for(i = 0; i <= ((Pmaster) ? term.t_nrow : term.t_nrow - 1); i++){
	movecursor(i, 0);
	if(!i){
	    fputs("Invoking alternate ", stdout);
	    fputs(f ? "speller..." : "editor...", stdout);
	}

	peeol();
    }

    (*term.t_flush)();
    if(Pmaster)
      (*Pmaster->raw_io)(0);			/* turn OFF raw mode */

#ifdef	SIGCHLD
    if(Pmaster){
	/*
	 * The idea here is to keep any mail connections our caller
	 * may have open while our child's out running around...
	 */
	pico_child_done = pico_child_jmp_ok = 0;
	(void) signal(SIGCHLD,  child_handler);
    }
#endif

    if((child = fork()) > 0){		/* wait for the child to finish */
	ohup = signal(SIGHUP, SIG_IGN);	/* ignore signals for now */
	oint = signal(SIGINT, SIG_IGN);
#ifdef	TIOCGWINSZ
        osize = signal(SIGWINCH, SIG_IGN);
#endif

#ifdef	SIGCHLD
	if(Pmaster){
	    while(!pico_child_done){
		(*Pmaster->newmail)(0, 0);
		if(!pico_child_done)
		  if(setjmp(pico_child_state) == 0){
		      pico_child_jmp_ok = 1;
		      sleep(600);
		  }

		pico_child_jmp_ok = 0;
	    }
	}
#endif

	while((pid = (int) wait(&stat)) != child)
	  ;

	signal(SIGHUP, ohup);	/* restore signals */
	signal(SIGINT, oint);
#ifdef	TIOCGWINSZ
        signal(SIGWINCH, osize);
#endif

	/*
	 * Report child's unnatural or unhappy exit...
	 */
	if(WIFEXITED(stat) && WEXITSTATUS(stat) == 0)
	  strcpy(result, "Alternate %s done");
	else
	  sprintf(result, "Alternate %%s abnormally terminated (%d)",
		  WIFEXITED(stat) ? WEXITSTATUS(stat) : -1);
    }
    else if(child == 0){		/* spawn editor */
	signal(SIGHUP, SIG_DFL);	/* let editor handle signals */
	signal(SIGINT, SIG_DFL);
#ifdef	TIOCGWINSZ
        signal(SIGWINCH, SIG_DFL);
#endif
#ifdef	SIGCHLD
	(void) signal(SIGCHLD,  SIG_DFL);
#endif
	if(execvp(args[0], args) < 0)
	  exit(-1);
    }
    else				/* error! */
      sprintf(result, "\007Can't fork %%s: %s", errstr(errno));

#ifdef	SIGCHLD
    (void) signal(SIGCHLD,  SIG_DFL);
#endif

    if(Pmaster)
      (*Pmaster->raw_io)(1);		/* turn ON raw mode */

    /*
     * replace edited text with new text 
     */
    curbp->b_flag &= ~BFCHG;		/* make sure old text gets blasted */
    readin(fn, 0);			/* read new text overwriting old */
    unlink(fn);				/* blast temp file */
    curbp->b_flag |= BFCHG;		/* mark dirty for packbuf() */
    ttopen();				/* reset the signals */
    refresh(0, 1);			/* redraw */
    update();
    emlwrite(result, f ? "speller" : "editor");
    return(0);
}



/*
 *  bktoshell - suspend and wait to be woken up
 */
bktoshell()		/* suspend MicroEMACS and wait to wake up */
{
#ifdef	SIGTSTP
    int pid;

    if(!(gmode&MDSSPD)){
	emlwrite("\007Unknown command: ^Z", NULL);
	return;
    }

    if(gmode&MDSPWN){
	char *shell;

	if(Pmaster){
	    (*Pmaster->raw_io)(0);	/* actually in pine source */

	    movecursor(0, 0);
	    (*term.t_eeop)();
	    printf("\n\n\nUse \"exit\" to return to Pine\n");
	}
	else{
	    vttidy();
	    movecursor(0, 0);
	    (*term.t_eeop)();
	    printf("\n\n\nUse \"exit\" to return to Pi%s\n",
		   (gmode & MDBRONLY) ? "lot" : "co");
	}

	system((shell = (char *)getenv("SHELL")) ? shell : "/bin/csh");
	rtfrmshell();			/* fixup tty */
    }
    else {
	if(Pmaster){
	    (*Pmaster->raw_io)(0);	/* actually in pine source */

	    movecursor(term.t_nrow, 0);
	    printf("\n\n\nUse \"fg\" to return to Pine\n");
	}
	else{
	    movecursor(term.t_nrow-1, 0);
	    peeol();
	    movecursor(term.t_nrow, 0);
	    peeol();
	    movecursor(term.t_nrow, 0);
	    printf("\n\n\nUse \"fg\" to return to Pi%s\n",
		   (gmode & MDBRONLY) ? "lot" : "co");
	    ttclose();
	}

	movecursor(term.t_nrow, 0);
	peeol();
	(*term.t_flush)();

	signal(SIGCONT, rtfrmshell);	/* prepare to restart */
	signal(SIGTSTP, SIG_DFL);			/* prepare to stop */
	kill(0, SIGTSTP);
    }
#endif
}


/* 
 * rtfrmshell - back from shell, fix modes and return
 */
SIGTYPE
rtfrmshell()
{
#ifdef	SIGCONT
    signal(SIGCONT, SIG_DFL);

    if(Pmaster){
	(*Pmaster->raw_io)(1);			/* actually in pine source */
	(*Pmaster->keybinit)(gmode&MDFKEY);	/* using f-keys? */
    }

    ttopen();
    ttresize();
    pclear(0, term.t_nrow);
    refresh(0, 1);
#endif
}



/*
 * do_hup_signal - jump back in the stack to where we can handle this
 */
SIGTYPE
do_hup_signal()
{
    signal(SIGHUP,  SIG_IGN);			/* ignore further SIGHUP's */
    signal(SIGTERM, SIG_IGN);			/* ignore further SIGTERM's */
    if(Pmaster){
	extern jmp_buf finstate;

	longjmp(finstate, COMP_GOTHUP);
    }
    else{
	/*
	 * if we've been interrupted and the buffer is changed,
	 * save it...
	 */
	if(anycb() == TRUE){			/* time to save */
	    if(curbp->b_fname[0] == '\0'){	/* name it */
		strcpy(curbp->b_fname, "pico.save");
	    }
	    else{
		strcat(curbp->b_fname, ".save");
	    }
	    writeout(curbp->b_fname);
	}
	vttidy();
	exit(1);
    }
}


/*
 * big bitmap of ASCII characters allowed in a file name
 * (needs reworking for other char sets)
 */
unsigned char okinfname[32] = {
      0,    0, 			/* ^@ - ^G, ^H - ^O  */
      0,    0,			/* ^P - ^W, ^X - ^_  */
      0x80, 0x17,		/* SP - ' ,  ( - /   */
      0xff, 0xc4,		/*  0 - 7 ,  8 - ?   */
      0x7f, 0xff,		/*  @ - G ,  H - O   */
      0xff, 0xe1,		/*  P - W ,  X - _   */
      0x7f, 0xff,		/*  ` - g ,  h - o   */
      0xff, 0xf6,		/*  p - w ,  x - DEL */
      0,    0, 			/*  > DEL   */
      0,    0,			/*  > DEL   */
      0,    0, 			/*  > DEL   */
      0,    0, 			/*  > DEL   */
      0,    0 			/*  > DEL   */
};


/*
 * fallowc - returns TRUE if c is allowable in filenames, FALSE otw
 */
fallowc(c)
char c;
{
    return(okinfname[c>>3] & 0x80>>(c&7));
}


/*
 * fexist - returns TRUE if the file exists with mode passed in m, 
 *          FALSE otherwise.  By side effect returns length of file in l
 */
fexist(file, m, l)
char *file;
char *m;			/* files mode: r,w,rw,t or x */
long *l;			/* t means use lstat         */
{
    struct stat	sbuf;
    extern int lstat();
    int		(*stat_f)() = (m && *m == 't') ? lstat : stat;
    int accessible;
/* define them here so we don't have to worry about what to include */
#define EXECUTE_ACCESS  01
#define WRITE_ACCESS    02
#define READ_ACCESS     04

    if(l)
      *l = 0L;
    
    if((*stat_f)(file, &sbuf) < 0){
	switch(errno){
	  case ENOENT :				/* File not found */
	    return(FIOFNF);
#ifdef	ENAMETOOLONG
	  case ENAMETOOLONG :			/* Name is too long */
	    return(FIOLNG);
#endif
	  case EACCES :				/* File not found */
	    return(FIOPER);
	  default:				/* Some other error */
	    return(FIOERR);
	}
    }

    if(l)
      *l = sbuf.st_size;

    if((sbuf.st_mode&S_IFMT) == S_IFDIR)
      return(FIODIR);
    else if(*m == 't'){
	struct stat	sbuf2;

	/*
	 * If it is a symbolic link pointing to a directory, treat
	 * it like it is a directory, not a link.
	 */
	if((sbuf.st_mode&S_IFMT) == S_IFLNK){
	    if(stat(file, &sbuf2) < 0){
		switch(errno){
		  case ENOENT :				/* File not found */
		    return(FIOSYM);			/* call it a link */
#ifdef	ENAMETOOLONG
		  case ENAMETOOLONG :			/* Name is too long */
		    return(FIOLNG);
#endif
		  case EACCES :				/* File not found */
		    return(FIOPER);
		  default:				/* Some other error */
		    return(FIOERR);
		}
	    }

	    if((sbuf2.st_mode&S_IFMT) == S_IFDIR)
	      return(FIODIR);
	}

	return(((sbuf.st_mode&S_IFMT) == S_IFLNK) ? FIOSYM : FIOSUC);
    }

    if(*m == 'r'){				/* read access? */
	if(*(m+1) == 'w')			/* and write access? */
	  return((access(file,READ_ACCESS)==0)
		 ? (access(file,WRITE_ACCESS)==0)
		    ? FIOSUC
		    : FIONWT
		 : FIONRD);
	else if(!*(m+1))			/* just read access? */
	  return((access(file,READ_ACCESS)==0) ? FIOSUC : FIONRD);
    }
    else if(*m == 'w' && !*(m+1))		/* write access? */
      return((access(file,WRITE_ACCESS)==0) ? FIOSUC : FIONWT);
    else if(*m == 'x' && !*(m+1))		/* execute access? */
      return((access(file,EXECUTE_ACCESS)==0) ? FIOSUC : FIONEX);
    return(FIOERR);				/* bad m arg */
}


/*
 * isdir - returns true if fn is a readable directory, false otherwise
 *         silent on errors (we'll let someone else notice the problem;)).
 */
isdir(fn, l)
char *fn;
long *l;
{
    struct stat sbuf;

    if(l)
      *l = 0;

    if(stat(fn, &sbuf) < 0)
      return(0);

    if(l)
      *l = sbuf.st_size;
    return((sbuf.st_mode&S_IFMT) == S_IFDIR);
}


#if	defined(bsd) || defined(nxt) || defined(dyn)
/*
 * getcwd - NeXT uses getwd()
 */
char *
getcwd(pth, len)
char *pth;
int   len;
{
    extern char *getwd();

    return(getwd(pth));
}
#endif


/*
 * gethomedir - returns the users home directory
 *              Note: home is malloc'd for life of pico
 */
char *
gethomedir(l)
int *l;
{
    static char *home = NULL;
    static short hlen = 0;

    if(home == NULL){
	char buf[NLINE];
	strcpy(buf, "~");
	fixpath(buf, NLINE);		/* let fixpath do the work! */
	hlen = strlen(buf);
	if((home = (char *)malloc((hlen + 1) * sizeof(char))) == NULL){
	    emlwrite("Problem allocating space for home dir", NULL);
	    return(0);
	}

	strcpy(home, buf);
    }

    if(l)
      *l = hlen;

    return(home);
}


/*
 * homeless - returns true if given file does not reside in the current
 *            user's home directory tree. 
 */
homeless(f)
char *f;
{
    char *home;
    int   len;

    home = gethomedir(&len);
    return(strncmp(home, f, len));
}



/*
 * errstr - return system error string corresponding to given errno
 *          Note: strerror() is not provided on all systems, so it's 
 *          done here once and for all.
 */
char *
errstr(err)
int err;
{
#if !defined(neb) && !defined(BSDI2)
    extern char *sys_errlist[];
#endif
    extern int  sys_nerr;

    return((err >= 0 && err < sys_nerr) ? sys_errlist[err] : NULL);
}



/*
 * getfnames - return all file names in the given directory in a single 
 *             malloc'd string.  n contains the number of names
 */
char *
getfnames(dn, pat, n, e)
char *dn, *pat, *e;
int  *n;
{
    long           l;
    char          *names, *np, *p;
    struct stat    sbuf;
#if	defined(ct)
    FILE          *dirp;
    char           fn[DIRSIZ+1];
#else
    DIR           *dirp;			/* opened directory */
#endif
#if	defined(POSIX) || defined(aix) || defined(COHERENT) || defined(isc) || defined(sv3) || defined(asv)
    struct dirent *dp;
#else
    struct direct *dp;
#endif

    *n = 0;

    if(stat(dn, &sbuf) < 0){
	switch(errno){
	  case ENOENT :				/* File not found */
	    if(e)
	      sprintf(e, "\007File not found: \"%s\"", dn);

	    break;
#ifdef	ENAMETOOLONG
	  case ENAMETOOLONG :			/* Name is too long */
	    if(e)
	      sprintf(e, "\007File name too long: \"%s\"", dn);

	    break;
#endif
	  default:				/* Some other error */
	    if(e)
	      sprintf(e, "\007Error getting file info: \"%s\"", dn);

	    break;
	}
	return(NULL);
    } 
    else{
	l = sbuf.st_size;
	if((sbuf.st_mode&S_IFMT) != S_IFDIR){
	    if(e)
	      sprintf(e, "\007Not a directory: \"%s\"", dn);

	    return(NULL);
	}
    }

    if((names=(char *)malloc(sizeof(char)*l)) == NULL){
	if(e)
	  sprintf(e, "\007Can't malloc space for file names", NULL);

	return(NULL);
    }

    errno = 0;
    if((dirp=opendir(dn)) == NULL){
	if(e)
	  sprintf(e, "\007Can't open \"%s\": %s", dn, errstr(errno));

	free((char *)names);
	return(NULL);
    }

    np = names;

#if	defined(ct)
    while(fread(&dp, sizeof(struct direct), 1, dirp) > 0) {
    /* skip empty slots with inode of 0 */
	if(dp.d_ino == 0)
	     continue;
	(*n)++;                     /* count the number of active slots */
	(void)strncpy(fn, dp.d_name, DIRSIZ);
	fn[14] = '\0';
	p = fn;
	while((*np++ = *p++) != '\0')
	  ;
    }
#else
    while((dp = readdir(dirp)) != NULL)
      if(!pat || !*pat || !strncmp(dp->d_name, pat, strlen(pat))){
	  (*n)++;
	  p = dp->d_name;
	  while(*np++ = *p++)
	    ;
      }
#endif

    closedir(dirp);					/* shut down */
    return(names);
}


/*
 * fioperr - given the error number and file name, display error
 */
void
fioperr(e, f)
int  e;
char *f;
{
    switch(e){
      case FIOFNF:				/* File not found */
	emlwrite("\007File \"%s\" not found", f);
	break;
      case FIOEOF:				/* end of file */
	emlwrite("\007End of file \"%s\" reached", f);
	break;
      case FIOLNG:				/* name too long */
	emlwrite("\007File name \"%s\" too long", f);
	break;
      case FIODIR:				/* file is a directory */
	emlwrite("\007File \"%s\" is a directory", f);
	break;
      case FIONWT:
	emlwrite("\007Write permission denied: %s", f);
	break;
      case FIONRD:
	emlwrite("\007Read permission denied: %s", f);
	break;
      case FIOPER:
	emlwrite("\007Permission denied: %s", f);
	break;
      case FIONEX:
	emlwrite("\007Execute permission denied: %s", f);
	break;
      default:
	emlwrite("\007File I/O error: %s", f);
    }
}



/*
 * pfnexpand - pico's function to expand the given file name if there is 
 *	       a leading '~'
 */
char *pfnexpand(fn, len)
char *fn;
int  len;
{
    struct passwd *pw;
    register char *x, *y, *z;
    char *home = NULL;
    char name[20];
    
    if(*fn == '~') {
        for(x = fn+1, y = name; *x != '/' && *x != '\0'; *y++ = *x++)
	  ;

        *y = '\0';
        if(x == fn + 1){			/* ~/ */
	    if (!(home = (char *) getenv("HOME")))
	      if (pw = getpwuid(geteuid()))
		home = pw->pw_dir;
	}
	else if(*name){				/* ~username/ */
	    if(pw = getpwnam(name))
	      home = pw->pw_dir;
	}

        if(!home || (strlen(home) + strlen(fn) > len))
	  return(NULL);

	/* make room for expanded path */
	for(z = x + strlen(x), y = fn + strlen(x) + strlen(home);
	    z >= x;
	    *y-- = *z--)
	  ;

	/* and insert the expanded address */
	for(x = fn, y = home; *y != '\0'; *x++ = *y++)
	  ;
    }
    return(fn);
}



/*
 * fixpath - make the given pathname into an absolute path
 */
fixpath(name, len)
char *name;
int  len;
{
    register char *shft;

    /* filenames relative to ~ */
    if(!((name[0] == '/')
          || (name[0] == '.'
              && (name[1] == '/' || (name[1] == '.' && name[2] == '/'))))){
	if(Pmaster && !(gmode&MDCURDIR)
                   && (*name != '~' && strlen(name)+2 <= len)){

	    if(gmode&MDTREE && strlen(name)+strlen(opertree)+1 <= len){
		int i, off = strlen(opertree);

		for(shft = strchr(name, '\0'); shft >= name; shft--)
		  shft[off+1] = *shft;

		strncpy(name, opertree, off);
		name[off] = '/';
	    }
	    else{
		for(shft = strchr(name, '\0'); shft >= name; shft--)
		  shft[2] = *shft;

		name[0] = '~';
		name[1] = '/';
	    }
	}

	pfnexpand(name, len);
    }
}


/*
 * compresspath - given a base path and an additional directory, collapse
 *                ".." and "." elements and return absolute path (appending
 *                base if necessary).  
 *
 *                returns  1 if OK, 
 *                         0 if there's a problem
 *                         new path, by side effect, if things went OK
 */
compresspath(base, path, len)
char *base, *path;
int  len;
{
    register int i;
    int  depth = 0;
    char *p;
    char *stack[32];
    char  pathbuf[NLINE];

#define PUSHD(X)  (stack[depth++] = X)
#define POPD()    ((depth > 0) ? stack[--depth] : "")

    if(*path == '~'){
	fixpath(path, len);
	strcpy(pathbuf, path);
    }
    else if(*path != C_FILESEP)
      sprintf(pathbuf, "%s%c%s", base, C_FILESEP, path);
    else
      strcpy(pathbuf, path);

    p = &pathbuf[0];
    for(i=0; pathbuf[i] != '\0'; i++){		/* pass thru path name */
	if(pathbuf[i] == '/'){
	    if(p != pathbuf)
	      PUSHD(p);				/* push dir entry */

	    p = &pathbuf[i+1];			/* advance p */
	    pathbuf[i] = '\0';			/* cap old p off */
	    continue;
	}

	if(pathbuf[i] == '.'){			/* special cases! */
	    if(pathbuf[i+1] == '.' 		/* parent */
	       && (pathbuf[i+2] == '/' || pathbuf[i+2] == '\0')){
		if(!strcmp(POPD(), ""))		/* bad news! */
		  return(0);

		i += 2;
		p = (pathbuf[i] == '\0') ? "" : &pathbuf[i+1];
	    }
	    else if(pathbuf[i+1] == '/' || pathbuf[i+1] == '\0'){
		i++;
		p = (pathbuf[i] == '\0') ? "" : &pathbuf[i+1];
	    }
	}
    }

    if(*p != '\0')
      PUSHD(p);					/* get last element */

    path[0] = '\0';
    for(i = 0; i < depth; i++){
	strcat(path, S_FILESEP);
	strcat(path, stack[i]);
    }

    return(1);					/* everything's ok */
}


/*
 * tmpname - return a temporary file name in the given buffer
 */
void
tmpname(name)
char *name;
{
    sprintf(name, "/tmp/pico.%d", getpid());	/* tmp file name */
}


/*
 * Take a file name, and from it
 * fabricate a buffer name. This routine knows
 * about the syntax of file names on the target system.
 * I suppose that this information could be put in
 * a better place than a line of code.
 */
void
makename(bname, fname)
char    bname[];
char    fname[];
{
    register char   *cp1;
    register char   *cp2;

    cp1 = &fname[0];
    while (*cp1 != 0)
      ++cp1;

    while (cp1!=&fname[0] && cp1[-1]!='/')
      --cp1;

    cp2 = &bname[0];
    while (cp2!=&bname[NBUFN-1] && *cp1!=0 && *cp1!=';')
      *cp2++ = *cp1++;

    *cp2 = 0;
}


/*
 * copy - copy contents of file 'a' into a file named 'b'.  Return error
 *        if either isn't accessible or is a directory
 */
copy(a, b)
char *a, *b;
{
    int    in, out, n, rv = 0;
    char   *cb;
    struct stat tsb, fsb;
    extern int  errno;

    if(stat(a, &fsb) < 0){		/* get source file info */
	emlwrite("Can't Copy: %s", errstr(errno));
	return(-1);
    }

    if(!(fsb.st_mode&S_IREAD)){		/* can we read it? */
	emlwrite("\007Read permission denied: %s", a);
	return(-1);
    }

    if((fsb.st_mode&S_IFMT) == S_IFDIR){ /* is it a directory? */
	emlwrite("\007Can't copy: %s is a directory", a);
	return(-1);
    }

    if(stat(b, &tsb) < 0){		/* get dest file's mode */
	switch(errno){
	  case ENOENT:
	    break;			/* these are OK */
	  default:
	    emlwrite("\007Can't Copy: %s", errstr(errno));
	    return(-1);
	}
    }
    else{
	if(!(tsb.st_mode&S_IWRITE)){	/* can we write it? */
	    emlwrite("\007Write permission denied: %s", b);
	    return(-1);
	}

	if((tsb.st_mode&S_IFMT) == S_IFDIR){	/* is it directory? */
	    emlwrite("\007Can't copy: %s is a directory", b);
	    return(-1);
	}

	if(fsb.st_dev == tsb.st_dev && fsb.st_ino == tsb.st_ino){
	    emlwrite("\007Identical files.  File not copied", NULL);
	    return(-1);
	}
    }

    if((in = open(a, O_RDONLY)) < 0){
	emlwrite("Copy Failed: %s", errstr(errno));
	return(-1);
    }

    if((out=creat(b, fsb.st_mode&0xfff)) < 0){
	emlwrite("Can't Copy: %s", errstr(errno));
	close(in);
	return(-1);
    }

    if((cb = (char *)malloc(NLINE*sizeof(char))) == NULL){
	emlwrite("Can't allocate space for copy buffer!", NULL);
	close(in);
	close(out);
	return(-1);
    }

    while(1){				/* do the copy */
	if((n = read(in, cb, NLINE)) < 0){
	    emlwrite("Can't Read Copy: %s", errstr(errno));
	    rv = -1;
	    break;			/* get out now */
	}

	if(n == 0)			/* done! */
	  break;

	if(write(out, cb, n) != n){
	    emlwrite("Can't Write Copy: %s", errstr(errno));
	    rv = -1;
	    break;
	}
    }

    free(cb);
    close(in);
    close(out);
    return(rv);
}


/*
 * Open a file for writing. Return TRUE if all is well, and FALSE on error
 * (cannot create).
 */
ffwopen(fn)
char    *fn;
{
    extern FILE *ffp;

    if ((ffp=fopen(fn, "w")) == NULL) {
        emlwrite("Cannot open file for writing", NULL);
        return (FIOERR);
    }

    return (FIOSUC);
}


/*
 * Close a file. Should look at the status in all systems.
 */
ffclose()
{
    extern FILE *ffp;

    fflush(ffp);
    if (fclose(ffp) == EOF) {
        emlwrite("\007Error closing file: %s", errstr(errno));
        return(FIOERR);
    }

    return(FIOSUC);
}


#if defined(asv) || defined(sco)

/*
 *	Tim Rice	tim@trr.metro.net	Mon Jun  3 16:57:26 PDT 1996
 *
 *	a quick and dirty trancate()
 *	Altos System V (5.3.1) does not have one
 *	neither does SCO Open Server Enterprise 3.0
 *
 */

truncate(fn, size)
char    *fn ;
long size ;
{
	int    fdes;
	int    rc = -1 ;

	if((fdes = open(fn, O_RDWR)) != -1)
	{
		rc = chsize(fdes, size) ;

		if(close(fdes))
			return(-1) ;
	}
	return(rc) ;
}

#endif /* defined(asv) || defined(sco) */


/*
 * ffelbowroom - make sure the destination's got enough room to receive
 *		 what we're about to write...
 */
ffelbowroom(fn)
char    *fn;
{
    register LINE *lp;
    register long  n;
    struct   stat  sbuf;
    FILE    *fp;
    int	     x, vapor = FALSE;
    char    *s, *err_str = NULL;
#define	EXTEND_BLOCK	1024

    /* Figure out how much room do we need */
    /* first, what's total */
    for(n=0L, lp=lforw(curbp->b_linep); lp != curbp->b_linep; lp=lforw(lp))
      n += llength(lp) + 1;

    if(stat(fn, &sbuf) < 0){
	if(errno == ENOENT)
	  vapor = TRUE;
	else
	  return(FALSE);
    }
    else
      n -= sbuf.st_size;

    if(n > 0L){			/* must be growing file, extend it */
	if(s = (char *) malloc(EXTEND_BLOCK * sizeof(char))){
	    memset(s, 0, EXTEND_BLOCK);
	    if(fp = fopen(fn, "a")){
		for( ; n > 0; n -= EXTEND_BLOCK){
		    x = (n < EXTEND_BLOCK) ? EXTEND_BLOCK - n : EXTEND_BLOCK;
		    if(fwrite(s, sizeof(char), x, fp) != x){
			err_str = errstr(errno);
			break;
		    }
		}
		    
		if(fclose(fp) == EOF)
		  err_str = errstr(errno);
	    }
	    else
	      err_str = errstr(errno);

	    free(s);
	}
	else
	  err_str = "No memory to extend file";
    }

    if(err_str){		/* clean up */
	if(vapor)
	  unlink(fn);
	else
	  truncate(fn, sbuf.st_size);

	emlwrite("No room for file: %s", err_str);
	return(FALSE);
    }

    return(TRUE);
}


/*
 * P_open - run the given command in a sub-shell returning a file pointer
 *	    from which to read the output
 *
 * note:
 *	For OS's other than unix, you will have to rewrite this function.
 *	Hopefully it'll be easy to exec the command into a temporary file, 
 *	and return a file pointer to that opened file or something.
 */
FILE *P_open(s)
char *s;
{
    return(popen(s, "r"));
}



/*
 * P_close - close the given descriptor
 *
 */
P_close(fp)
FILE *fp;
{
    return(pclose(fp));
}



/*
 * worthit - generic sort of test to roughly gage usefulness of using 
 *           optimized scrolling.
 *
 * note:
 *	returns the line on the screen, l, that the dot is currently on
 */
worthit(l)
int *l;
{
    int i;			/* l is current line */
    unsigned below;		/* below is avg # of ch/line under . */

    *l = doton(&i, &below);
    below = (i > 0) ? below/(unsigned)i : 0;

    return(below > 3);
}



/*
 * pico_new_mail - just checks mtime and atime of mail file and notifies user 
 *	           if it's possible that they have new mail.
 */
pico_new_mail()
{
    int ret = 0;
    static time_t lastchk = 0;
    struct stat sbuf;
    char   inbox[256], *p;

    if(p = (char *)getenv("MAIL"))
      sprintf(inbox, p);
    else
      sprintf(inbox,"%s/%s", MAILDIR, getlogin());

    if(stat(inbox, &sbuf) == 0){
	ret = sbuf.st_atime <= sbuf.st_mtime &&
	  (lastchk < sbuf.st_mtime && lastchk < sbuf.st_atime);
	lastchk = sbuf.st_mtime;
	return(ret);
    }
    else
      return(ret);
}



/*
 * time_to_check - checks the current time against the last time called 
 *                 and returns true if the elapsed time is > timeout
 */
time_to_check()
{
    static time_t lasttime = 0L;

    if(!timeout)
      return(FALSE);

    if(time((time_t *) 0) - lasttime > (time_t)timeout){
	lasttime = time((time_t *) 0);
	return(TRUE);
    }
    else
      return(FALSE);
}


/*
 * sstrcasecmp - compare two pointers to strings case independently
 */
sstrcasecmp(s1, s2)
QcompType *s1, *s2;
{
    register char *a, *b;

    a = *(char **)s1;
    b = *(char **)s2;
    while(toupper((unsigned char)(*a)) == toupper((unsigned char)(*b++)))
	if(*a++ == '\0')
	  return(0);

    return(toupper((unsigned char)(*a)) - toupper((unsigned char)(*--b)));
}


/*
 * chkptinit -- initialize anything we need to support composer
 *		checkpointing
 */
chkptinit(file, n)
    char *file;
    int   n;
{
    unsigned pid;
    char    *chp;

    if(!file[0]){
	long gmode_save = gmode;

	if(gmode&MDCURDIR)
	  gmode &= ~MDCURDIR;  /* so fixpath will use home dir */

	strcpy(file, "#picoXXXXX#");
	fixpath(file, NLINE);
	gmode = gmode_save;
    }
    else{
	int l = strlen(file);

	if(file[l-1] != '/'){
	    file[l++] = '/';
	    file[l]   = '\0';
	}

	strcpy(file + l, "#picoXXXXX#");
    }

    pid = (unsigned)getpid();
    for(chp = file+strlen(file) - 2; *chp == 'X'; chp--){
	*chp = (pid % 10) + '0';
	pid /= 10;
    }

    unlink(file);
}


#ifdef	TIOCGWINSZ
/*
 * winch_handler - handle window change signal
 */
SIGTYPE winch_handler()
{
    signal(SIGWINCH, winch_handler);
    ttresize();
}
#endif	/* TIOCGWINSZ */


#ifdef	SIGCHLD
/*
 * child_handler - handle child status change signal
 */
SIGTYPE child_handler()
{
    pico_child_done = 1;
    if(pico_child_jmp_ok)
      longjmp(pico_child_state, 1);
}
#endif	/* SIGCHLD */


#if	defined(sv3) || defined(ct)
/* Placed by rll to handle the rename function not found in AT&T */
rename(oldname, newname)
    char *oldname;
    char *newname;
{
    int rtn;

    if ((rtn = link(oldname, newname)) != 0) {
	perror("Was not able to rename file.");
	return(rtn);
    }

    if ((rtn = unlink(oldname)) != 0)
      perror("Was not able to unlink file.");

    return(rtn);
}
#endif


#ifdef	MOUSE

/* 
 * init_mouse - check for xterm and initialize mouse tracking if present...
 */
init_mouse()
{
    if(mexist)
      return(TRUE);

    if(getenv("DISPLAY")){
	mouseon();
	return(mexist = TRUE);
    }
    else
      return(FALSE);
}


/* 
 * end_mouse - clear xterm mouse tracking if present...
 */
void
end_mouse()
{
    if(mexist){
	mexist = 0;			/* just see if it exists here. */
	mouseoff();
    }
}


/*
 * mouseexist - function to let outsiders know if mouse is turned on
 *              or not.
 */
mouseexist()
{
    return(mexist);
}


/*
 * mouseon - call made available for programs calling pico to turn ON the
 *           mouse cursor.
 */
void
mouseon()
{
    fputs(XTERM_MOUSE_ON, stdout);
}


/*
 * mouseon - call made available for programs calling pico to turn OFF the
 *           mouse cursor.
 */
void
mouseoff()
{
    fputs(XTERM_MOUSE_OFF, stdout);
}


/* 
 * checkmouse - look for mouse events in key menu and return 
 *              appropriate value.
 */
int
checkmouse(ch, down, mcol, mrow)
unsigned *ch;
int	  down, mcol, mrow;
{
    static int oindex;
    register int k;			/* current bit/button of mouse */
    int i = 0, rv = 0;
    MENUITEM *mp;

    if(!mexist || mcol < 0 || mrow < 0)
      return(FALSE);

    if(down)			/* button down */
      oindex = -1;

    for(mp = mfunc; mp; mp = mp->next)
      if(mp->action && M_ACTIVE(mrow, mcol, mp))
	break;

    if(mp){
	unsigned long r;

	r = (*mp->action)(down ? M_EVENT_DOWN : M_EVENT_UP,
			  mrow, mcol, M_BUTTON_LEFT, 0);
	if(r & 0xffff){
	    *ch = (unsigned)((r>>16)&0xffff);
	    rv  = TRUE;
	}
    }
    else{
	while(1){			/* see if we understand event */
	    if(i >= 12){
		i = -1;
		break;
	    }

	    if(M_ACTIVE(mrow, mcol, &menuitems[i]))
	      break;

	    i++;
	}

	if(down){			/* button down */
	    oindex = i;			/* remember where */
	    if(i != -1
	       && menuitems[i].label_hiliter != NULL
	       && menuitems[i].val != mnoop)  /* invert label */
	      (*menuitems[i].label_hiliter)(1, &menuitems[i]);
	}
	else{				/* button up */
	    if(oindex != -1){
		if(i == oindex){
		    *ch = menuitems[i].val;
		    rv = TRUE;
		}
	    }
	}
    }

    /* restore label */
    if(!down
       && oindex != -1
       && menuitems[oindex].label_hiliter != NULL
       && menuitems[oindex].val != mnoop)
      (*menuitems[oindex].label_hiliter)(0, &menuitems[oindex]);

    return(rv);
}


/*
 * invert_label - highlight the label of the given menu item.
 */
void
invert_label(state, m)
int state;
MENUITEM *m;
{
    unsigned i, j;
    int   r, c, p, oldr, oldc, col_offset;
    char *lp;

    /*
     * Leave the command name bold
     */
    col_offset = (state || !(lp=strchr(m->label, ' '))) ? 0 : (lp - m->label);
    movecursor((int)m->tl.r, (int)m->tl.c + col_offset);
    (*term.t_rev)(state);

    for(i = m->tl.r; i <= m->br.r; i++)
      for(j = m->tl.c + col_offset; j <= m->br.c; j++)
	if(i == m->lbl.r && j == m->lbl.c + col_offset && m->label){
	    lp = m->label + col_offset;		/* show label?? */
	    while(*lp && j++ < m->br.c)
	      putc(*lp++, stdout);

	    continue;
	}
	else
	  putc(' ', stdout);

    if(state)
      (*term.t_rev)(0);  /* turn inverse back off */
}
#endif	/* MOUSE */
