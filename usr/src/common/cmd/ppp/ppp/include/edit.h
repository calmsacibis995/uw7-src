#ifndef _edit_h
#define _edit_h
#ident "@(#)edit.h	1.2"
#ident "$Header$"


#define OLDTERMIO 1
#define NULL	0
#define KSHELL	1
#define ESH_APPEND	1

/*
 *  edit.h -  common data structure for vi and emacs edit options
 *
 */

#ifdef __cplusplus
extern "C" {
#endif


#define LOOKAHEAD	80
#define READAHEAD	LOOKAHEAD

#define TABSIZE		8
#define SEARCHSIZE	80
#define PRSIZE		80
#define CHARSIZE	1

typedef char genchar;

struct edit
{
	int	e_kill;
	int	e_erase;
	int	e_eof;
	int	e_intr;
	int	e_fchar;
	char	e_plen;		/* length of prompt string */
	char	e_crlf;		/* zero if cannot return to beginning of line */
	int	e_llimit;	/* line length limit */
	int	e_hline;	/* current history line number */
	int	e_hloff;	/* line number offset for command */
	int	e_hismin;	/* minimum history line number */
	int	e_hismax;	/* maximum history line number */
	int	e_raw;		/* set when in raw mode or alt mode */
	int	e_cur;		/* current line position */
	int	e_eol;		/* end-of-line position */
	int	e_pcur;		/* current physical line position */
	int	e_peol;		/* end of physical line position */
	int	e_mode;		/* edit mode */
	int	e_index;	/* index in look-ahead buffer */
	int	e_repeat;
	int	e_saved;
	int	e_fcol;		/* first column */
	int	e_ucol;		/* column for undo */
	int	e_addnl;	/* set if new-line must be added */
	int	e_wsize;	/* width of display window */
	char	*e_outbase;	/* pointer to start of output buffer */
	char	*e_outptr;	/* pointer to position in output buffer */
	char	*e_outlast;	/* pointer to end of output buffer */
	genchar	*e_inbuf;	/* pointer to input buffer */
	char	*e_prompt;	/* pointer to buffer containing the prompt */
	genchar	*e_ubuf;	/* pointer to the undo buffer */
	genchar	*e_killbuf;	/* pointer to delete buffer */
	char	e_search[SEARCHSIZE];	/* temporary workspace buffer */
	genchar	*e_Ubuf;	/* temporary workspace buffer */
	genchar	*e_physbuf;	/* temporary workspace buffer */
	int	e_lbuf[LOOKAHEAD];/* pointer to look-ahead buffer */
	int	*e_globals;	/* global variables */
	genchar	*e_window;	/* display window  image */
	char	e_inmacro;	/* processing macro expansion */
	char	e_prbuff[PRSIZE]; /* prompt buffer */
};

#undef  MAXWINDOW
#define MAXWINDOW	160	/* maximum width window */
#define MINWINDOW	15	/* minimum width window */
#define DFLTWINDOW	80	/* default window width */
#define	MAXPAT		100	/* maximum length for pattern word */
#define	YES	1
#define NO	0
#define DELETE	'\177'
#define BELL	'\7'
#define ESC	033
#define	UEOF	-2			/* user eof char synonym */
#define	UERASE	-3			/* user erase char synonym */
#define	UINTR	-4			/* user intr char synonym */
#define	UKILL	-5			/* user kill char synonym */
#define	UQUIT	-6			/* user quit char synonym */

#define	cntl(x)		(x&037)

#   define STRIP	0377
#   define TO_PRINT	0100
#   define GMACS	1
#   define EMACS	2
#   define VIRAW	4
#   define EDITVI	8
#   define EDITMASK	15
#   define is_option(m)	(opt_flag&(m))
    extern char opt_flag;

extern struct edit editb;

extern genchar edit_killbuf[];

#ifdef __cplusplus
}
#endif


#endif /* _edit_h */
