#if	!defined(lint) && !defined(DOS)
static char rcsid[] = "$Id$";
#endif
/*
 * Program:	Display routines
 *
 *
 * Donn Cave
 * Networks and Distributed Computing
 * Computing and Communications
 * University of Washington
 * Administration Builiding, AG-44
 * Seattle, Washington, 98195, USA
 * Internet: donn@cac.washington.edu
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
/*
 *      tinfo - substitute for tcap, on systems that have terminfo.
 */

#define	termdef	1			/* don't define "term" external */

#include	<stdio.h>
#include        <signal.h>
#include	"osdep.h"
#include	"estruct.h"
#include        "pico.h"
#include        "edef.h"

extern char *tigetstr ();

#define	MARGIN	8
#define	SCRSIZ	64
#define	MROW	2
#define BEL     0x07
#define ESC     0x1B

extern int      ttopen();
extern int      ttgetc();
extern int      ttputc();
extern int      ttflush();
extern int      ttclose();

static int      tinfomove();
static int      tinfoeeol();
static int      tinfoeeop();
static int      tinfobeep();
static int	tinforev();
static int      tinfoopen();
static int      tinfoclose();
static void     setup_dflt_pico_esc_seq();

extern int      tput();
extern char     *tgoto();

static int      putpad();

static char *UP, PC, *CM, *CE, *CL, *SO, *SE;
/* 
 * PICO extentions 
 */
static char *DL,			/* delete line */
	*AL,			/* insert line */
	*CS,			/* define a scrolling region, vt100 */
	*IC,			/* insert character, preferable to : */
	*IM,			/* set insert mode and, */
	*EI,			/* end insert mode */
	*DC,			/* delete character */
	*DM,			/* set delete mode and, */
	*ED,			/* end delete mode */
	*SF,			/* scroll text up */
	*SR,			/* scroll text down */
	*TI,			/* string to start termcap */
        *TE;			/* string to end termcap */

TERM term = {
        NROW-1,
        NCOL,
	MARGIN,
	SCRSIZ,
	MROW,
        tinfoopen,
        tinfoclose,
        ttgetc,
        ttputc,
        ttflush,
        tinfomove,
        tinfoeeol,
        tinfoeeop,
        tinfobeep,
        tinforev
};


/*
 * Add default keypad sequences to the trie.
 */
static void
setup_dflt_pico_esc_seq()
{
    /*
     * this is sort of a hack [no kidding], but it allows us to use
     * the function keys on pc's running telnet
     */

    /* 
     * UW-NDC/UCS vt10[02] application mode.
     */
    kpinsert(&pico_kbesc, "\033OP", F1);
    kpinsert(&pico_kbesc, "\033OQ", F2);
    kpinsert(&pico_kbesc, "\033OR", F3);
    kpinsert(&pico_kbesc, "\033OS", F4);
    kpinsert(&pico_kbesc, "\033Op", F5);
    kpinsert(&pico_kbesc, "\033Oq", F6);
    kpinsert(&pico_kbesc, "\033Or", F7);
    kpinsert(&pico_kbesc, "\033Os", F8);
    kpinsert(&pico_kbesc, "\033Ot", F9);
    kpinsert(&pico_kbesc, "\033Ou", F10);
    kpinsert(&pico_kbesc, "\033Ov", F11);
    kpinsert(&pico_kbesc, "\033Ow", F12);

    /*
     * DEC vt100, ANSI and cursor key mode.
     */
    kpinsert(&pico_kbesc, "\033OA", K_PAD_UP);
    kpinsert(&pico_kbesc, "\033OB", K_PAD_DOWN);
    kpinsert(&pico_kbesc, "\033OC", K_PAD_RIGHT);
    kpinsert(&pico_kbesc, "\033OD", K_PAD_LEFT);

    /*
     * special keypad functions
     */
    kpinsert(&pico_kbesc, "\033[4J", K_PAD_PREVPAGE);
    kpinsert(&pico_kbesc, "\033[3J", K_PAD_NEXTPAGE);
    kpinsert(&pico_kbesc, "\033[2J", K_PAD_HOME);
    kpinsert(&pico_kbesc, "\033[N",  K_PAD_END);

    /* 
     * ANSI mode.
     */
    kpinsert(&pico_kbesc, "\033[=a", F1);
    kpinsert(&pico_kbesc, "\033[=b", F2);
    kpinsert(&pico_kbesc, "\033[=c", F3);
    kpinsert(&pico_kbesc, "\033[=d", F4);
    kpinsert(&pico_kbesc, "\033[=e", F5);
    kpinsert(&pico_kbesc, "\033[=f", F6);
    kpinsert(&pico_kbesc, "\033[=g", F7);
    kpinsert(&pico_kbesc, "\033[=h", F8);
    kpinsert(&pico_kbesc, "\033[=i", F9);
    kpinsert(&pico_kbesc, "\033[=j", F10);
    kpinsert(&pico_kbesc, "\033[=k", F11);
    kpinsert(&pico_kbesc, "\033[=l", F12);

    /*
     * DEC vt100, ANSI and cursor key mode reset.
     */
    kpinsert(&pico_kbesc, "\033[A", K_PAD_UP);
    kpinsert(&pico_kbesc, "\033[B", K_PAD_DOWN);
    kpinsert(&pico_kbesc, "\033[C", K_PAD_RIGHT);
    kpinsert(&pico_kbesc, "\033[D", K_PAD_LEFT);

    /*
     * DEC vt52 mode.
     */
    kpinsert(&pico_kbesc, "\033A", K_PAD_UP);
    kpinsert(&pico_kbesc, "\033B", K_PAD_DOWN);
    kpinsert(&pico_kbesc, "\033C", K_PAD_RIGHT);
    kpinsert(&pico_kbesc, "\033D", K_PAD_LEFT);

    /*
     * DEC vt52 application keys, and some Zenith 19.
     */
    kpinsert(&pico_kbesc, "\033?r", K_PAD_DOWN);
    kpinsert(&pico_kbesc, "\033?t", K_PAD_LEFT);
    kpinsert(&pico_kbesc, "\033?v", K_PAD_RIGHT);
    kpinsert(&pico_kbesc, "\033?x", K_PAD_UP);

    /*
     * Sun Console sequences.
     */
    kpinsert(&pico_kbesc, "\033[1",   K_SWALLOW_TIL_Z);
    kpinsert(&pico_kbesc, "\033[215", K_SWALLOW_UP);
    kpinsert(&pico_kbesc, "\033[217", K_SWALLOW_LEFT);
    kpinsert(&pico_kbesc, "\033[219", K_SWALLOW_RIGHT);
    kpinsert(&pico_kbesc, "\033[221", K_SWALLOW_DOWN);

    /*
     * Kermit App Prog Cmd, gobble until ESC \ (kermit should intercept this)
     */
    kpinsert(&pico_kbesc, "\033_", K_KERMIT);

    /*
     * Fake a control character.
     */
    kpinsert(&pico_kbesc, "\033\033", K_DOUBLE_ESC);
}


static tinfoopen()
{
    char  *t;
    char  *getenv();
    int    row, col;
    char  *KU, *KD, *KL, *KR,
	  *KPPU, *KPPD, *KPHOME, *KPEND, *KPDEL,
	  *KF1, *KF2, *KF3, *KF4, *KF5, *KF6,
	  *KF7, *KF8, *KF9, *KF10, *KF11, *KF12;


    /*
     * determine the terminal's communication speed and decide
     * if we need to do optimization ...
     */
    optimize = ttisslow();

    if (Pmaster) {
	/*
	 *		setupterm() automatically retrieves the value
	 *		of the TERM variable.
	 */
	int err;
	setupterm (0, 1, &err);
	if (err != 1) return FALSE;
    }
    else {
	/*
	 *		setupterm() issues a message and exits, if the
	 *		terminfo data base is gone or the term type is
	 *		unknown, if arg2 is 0.
	 */
	setupterm (0, 1, 0);
    }

    t = tigetstr("pad");
    if(t)
      PC = *t;

    CL = tigetstr("clear");
    CM = tigetstr("cup");
    CE = tigetstr("el");
    UP = tigetstr("cuu1");
    SE = tigetstr("rmso");
    SO = tigetstr("smso");
    DL = tigetstr("dl1");
    AL = tigetstr("il1");
    CS = tigetstr("csr");
    IC = tigetstr("ich1");
    IM = tigetstr("smir");
    EI = tigetstr("rmir");
    DC = tigetstr("dch1");
    DM = tigetstr("smdc");
    ED = tigetstr("rmdc");
    SF = tigetstr("ind");
    SR = tigetstr("ri");
    TI = tigetstr("smcup");
    TE = tigetstr("rmcup");

    row = tigetnum("lines");
    if(row == -1){
	char *er;
	int   rr;

	/* tigetnum failed, try $LINES */
	er = getenv("LINES");
	if(er && (rr = atoi(er)) > 0)
	  row = rr;
    }
    if(row >= 0)
      row--;

    col = tigetnum("cols");
    if(col == -1){
	char *ec;
	int   cc;

	/* tigetnum failed, try $COLUMNS */
	ec = getenv("COLUMNS");
	if(ec && (cc = atoi(ec)) > 0)
	  col = cc;
    }
    
    ttgetwinsz(&row, &col);
    term.t_nrow = (short) row;
    term.t_ncol = (short) col;

    eolexist = (CE != NULL);	/* will we be able to use clear to EOL? */
    revexist = (SO != NULL);	/* will be able to use reverse video */
    if(DC == NULL && (DM == NULL || ED == NULL))
      delchar = FALSE;
    if(IC == NULL && (IM == NULL || EI == NULL))
      inschar = FALSE;
    if((CS==NULL || SF==NULL || SR==NULL) && (DL==NULL || AL==NULL))
      scrollexist = FALSE;

    if(CL == NULL || CM == NULL || UP == NULL){
	if(Pmaster == NULL){
	    puts("Incomplete terminfo entry\n");
	    exit(1);
	}
    }
    else{
	KPPU   = tigetstr("kpp");
	KPPD   = tigetstr("knp");
	KPHOME = tigetstr("khome");
	KPEND  = tigetstr("kend");
	KPDEL  = tigetstr("kdch1");
	KU = tigetstr("kcuu1");
	KD = tigetstr("kcud1");
	KL = tigetstr("kcub1");
	KR = tigetstr("kcuf1");
	KF1 = tigetstr("kf1");
	KF2 = tigetstr("kf2");
	KF3 = tigetstr("kf3");
	KF4 = tigetstr("kf4");
	KF5 = tigetstr("kf5");
	KF6 = tigetstr("kf6");
	KF7 = tigetstr("kf7");
	KF8 = tigetstr("kf8");
	KF9 = tigetstr("kf9");
	KF10 = tigetstr("kf10");
	KF11 = tigetstr("kf11");
	KF12 = tigetstr("kf12");

#ifndef TERMCAP_WINS
	/*
	 * Add default keypad sequences to the trie.
	 * Since these come first, they will override any conflicting termcap
	 * or terminfo escape sequences defined below.  An escape sequence is
	 * considered conflicting if one is a prefix of the other.
	 * So, without TERMCAP_WINS, there will likely be some termcap/terminfo
	 * escape sequences that don't work, because they conflict with default
	 * sequences.
	 */
	setup_dflt_pico_esc_seq();
#endif /* !TERMCAP_WINS */

	/*
	 * add terminfo escape sequences to the trie...
	 */
	if(KU != NULL && (KL != NULL && (KR != NULL && KD != NULL))){
	    kpinsert(&pico_kbesc, KU, K_PAD_UP);
	    kpinsert(&pico_kbesc, KD, K_PAD_DOWN);
	    kpinsert(&pico_kbesc, KL, K_PAD_LEFT);
	    kpinsert(&pico_kbesc, KR, K_PAD_RIGHT);
	}

	if(KPPU != NULL && KPPD != NULL){
	    kpinsert(&pico_kbesc, KPPU, K_PAD_PREVPAGE);
	    kpinsert(&pico_kbesc, KPPD, K_PAD_NEXTPAGE);
	}

	kpinsert(&pico_kbesc, KPHOME, K_PAD_HOME);
	kpinsert(&pico_kbesc, KPEND,  K_PAD_END);
	kpinsert(&pico_kbesc, KPDEL,  K_PAD_DELETE);

	kpinsert(&pico_kbesc, KF1,  F1);
	kpinsert(&pico_kbesc, KF2,  F2);
	kpinsert(&pico_kbesc, KF3,  F3);
	kpinsert(&pico_kbesc, KF4,  F4);
	kpinsert(&pico_kbesc, KF5,  F5);
	kpinsert(&pico_kbesc, KF6,  F6);
	kpinsert(&pico_kbesc, KF7,  F7);
	kpinsert(&pico_kbesc, KF8,  F8);
	kpinsert(&pico_kbesc, KF9,  F9);
	kpinsert(&pico_kbesc, KF10, F10);
	kpinsert(&pico_kbesc, KF11, F11);
	kpinsert(&pico_kbesc, KF12, F12);
    }

    /*
     * Initialize UW-modified NCSA telnet to use its functionkeys
     */
    if(gmode&MDFKEY && Pmaster == NULL)
      puts("\033[99h");

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
    setup_dflt_pico_esc_seq();
#endif /* TERMCAP_WINS */

    ttopen();

    if(TI && !Pmaster){
	putpad(TI);			/* any init terminfo requires */
	if(CS)
	  putpad(tgoto(CS, term.t_nrow, 0));
    }
}


static tinfoclose()
{
    if(!Pmaster){
	if(gmode&MDFKEY)
	  puts("\033[99l");		/* reset UW-NCSA telnet keys */

	if(TE)				/* any clean up terminfo requires */
	  putpad(TE);
    }

    kbdestroy(pico_kbesc);		/* clean up key board sequence trie */
    pico_kbesc = NULL;
    ttclose();
}


/*
 * tinfoinsert - insert a character at the current character position.
 *               IC takes precedence.
 */
tinfoinsert(ch)
register char	ch;
{
    if(IC != NULL){
	putpad(IC);
	ttputc(ch);
    }
    else{
	putpad(IM);
	ttputc(ch);
	putpad(EI);
    }
}


/*
 * tinfodelete - delete a character at the current character position.
 */
tinfodelete()
{
    if(DM == NULL && ED == NULL)
      putpad(DC);
    else{
	putpad(DM);
	putpad(DC);
	putpad(ED);
    }
}


/*
 * o_scrolldown() - open a line at the given row position.
 *               use either region scrolling or deleteline/insertline
 *               to open a new line.
 */
o_scrolldown(row, n)
register int row;
register int n;
{
    register int i;

    if(CS != NULL){
	putpad(tgoto(CS, term.t_nrow - (term.t_mrow+1), row));
	tinfomove(row, 0);
	for(i = 0; i < n; i++)
	  putpad( (SR != NULL && *SR != '\0') ? SR : "\n" );
	putpad(tgoto(CS, term.t_nrow, 0));
	tinfomove(row, 0);
    }
    else{
	/*
	 * this code causes a jiggly motion of the keymenu when scrolling
	 */
	for(i = 0; i < n; i++){
	    tinfomove(term.t_nrow - (term.t_mrow+1), 0);
	    putpad(DL);
	    tinfomove(row, 0);
	    putpad(AL);
	}
#ifdef	NOWIGGLYLINES
	/*
	 * this code causes a sweeping motion up and down the display
	 */
	tinfomove(term.t_nrow - term.t_mrow - n, 0);
	for(i = 0; i < n; i++)
	  putpad(DL);
	tinfomove(row, 0);
	for(i = 0; i < n; i++)
	  putpad(AL);
#endif
    }
}


/*
 * o_scrollup() - open a line at the given row position.
 *               use either region scrolling or deleteline/insertline
 *               to open a new line.
 */
o_scrollup(row, n)
register int row;
register int n;
{
    register int i;

    if(CS != NULL){
	putpad(tgoto(CS, term.t_nrow - (term.t_mrow+1), row));
	/* setting scrolling region moves cursor to home */
	tinfomove(term.t_nrow-(term.t_mrow+1), 0);
	for(i = 0;i < n; i++)
	  putpad((SF == NULL || SF[0] == '\0') ? "\n" : SF);
	putpad(tgoto(CS, term.t_nrow, 0));
	tinfomove(2, 0);
    }
    else{
	for(i = 0; i < n; i++){
	    tinfomove(row, 0);
	    putpad(DL);
	    tinfomove(term.t_nrow - (term.t_mrow+1), 0);
	    putpad(AL);
	}
#ifdef  NOWIGGLYLINES
	/* see note above */
	tinfomove(row, 0);
	for(i = 0; i < n; i++)
	  putpad(DL);
	tinfomove(term.t_nrow - term.t_mrow - n, 0);
	for(i = 0;i < n; i++)
	  putpad(AL);
#endif
    }
}



/*
 * o_insert - use terminfo to optimized character insert
 *            returns: true if it optimized output, false otherwise
 */
o_insert(c)
char c;
{
    if(inschar){
	tinfoinsert(c);
	return(1);			/* no problems! */
    }

    return(0);				/* can't do it. */
}


/*
 * o_delete - use terminfo to optimized character insert
 *            returns true if it optimized output, false otherwise
 */
o_delete()
{
    if(delchar){
	tinfodelete();
	return(1);			/* deleted, no problem! */
    }

    return(0);				/* no dice. */
}


static tinfomove(row, col)
register int row, col;
{
    putpad(tgoto(CM, col, row));
}


static tinfoeeol()
{
    putpad(CE);
}


static tinfoeeop()
{
        putpad(CL);
}


static tinforev(state)		/* change reverse video status */
int state;	                /* FALSE = normal video, TRUE = rev video */
{
    static int cstate = FALSE;

    if(state == cstate)		/* no op if already set! */
      return(0);

    if(cstate = state){		/* remember last setting */
	if (SO != NULL)
	  putpad(SO);
    } else {
	if (SE != NULL)
	  putpad(SE);
    }
}


static tinfobeep()
{
    ttputc(BEL);
}


static putpad(str)
char    *str;
{
    tputs(str, 1, ttputc);
}
