/*
 * $Id$
 *
 * Program:	pico.h - definitions for Pine's composer library
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
 */

#ifndef	PICO_H
#define	PICO_H
/*
 * Defined for attachment support
 */
#define	ATTACHMENTS	1


/*
 * defs of return codes from pine mailer composer.
 */
#define	BUF_CHANGED	0x01
#define	COMP_CANCEL	0x02
#define	COMP_EXIT	0x04
#define	COMP_FAILED	0x08
#define	COMP_SUSPEND	0x10
#define	COMP_GOTHUP	0x20


/*
 * top line from the top of the screen for the editor to do 
 * its stuff
 */
#define	COMPOSER_TOP_LINE	2
#define	COMPOSER_TITLE_LINE	0



/*
 * definitions of Mail header structures 
 */
struct hdr_line {
        char text[256];
        struct  hdr_line        *next;
        struct  hdr_line        *prev;
};
 
 
/* 
 *  This structure controls the header line items on the screen.  An
 * instance of this should be created and passed in as an argument when
 * pico is called. The list is terminated by an entry with the name
 * element NULL.
 */
 
struct headerentry {
        char     *prompt;
	char	 *name;
#if defined(DOS) || defined(HELPFILE)
	short	  help;
#else
	char	**help;
#endif
        int	  prlen;
        int	  maxlen;
        char    **realaddr;
        int     (*builder)();        /* Function to verify/canonicalize val */
	struct headerentry        *affected_entry, *next_affected;
				     /* entry builder's 4th arg affects     */
        char   *(*selector)();       /* Browser for possible values         */
        char     *key_label;         /* Key label for key to call browser   */
        unsigned  display_it:1;	     /* field is to be displayed by default */
        unsigned  break_on_comma:1;  /* Field breaks on commas              */
        unsigned  is_attach:1;       /* Special case field for attachments  */
        unsigned  rich_header:1;     /* Field is part of rich header        */
        unsigned  only_file_chars:1; /* Field is a file name                */
        unsigned  single_space:1;    /* Crush multiple spaces into one      */
        unsigned  sticky:1;          /* Can't change this via affected_entry*/
        unsigned  dirty:1;           /* We've changed this entry            */
	unsigned  start_here:1;	     /* begin composer on first on lit      */
#ifdef	KS_OSDATAVAR
	KS_OSDATAVAR		     /* Port-Specific keymenu data */
#endif
	void     *bldr_private;      /* Data managed by builders            */
        struct    hdr_line        *hd_text;
};


/*
 * Structure to pass as arg to builders
 */
typedef struct bld_arg {
	char           *tptr;	/* pointer to malloc'd text 		     */
	void          **xtra;	/* builder keeps track of private info here  */
	struct bld_arg *next;	/* next one in list if more than one affected*/
} BUILDER_ARG;


/*
 * structure to keep track of header display
 */
struct on_display {
    int			 p_off;			/* offset into line */
    int			 p_len;			/* length of line   */
    int			 p_line;		/* physical line on screen */
    int			 top_e;			/* topline's header entry */
    struct hdr_line	*top_l;			/* top line on display */
    int			 cur_e;			/* current header entry */
    struct hdr_line	*cur_l;			/* current hd_line */
};						/* global on_display struct */


/*
 * Structure to handle attachments
 */
typedef struct pico_atmt {
    char *description;			/* attachment description */
    char *filename;			/* file/pseudonym for attachment */
    char *size;				/* size of attachment */
    char *id;				/* attachment id */
    unsigned short flags;
    struct pico_atmt *next;
} PATMT;


/*
 * Flags for attachment handling
 */
#define	A_FLIT	0x0001			/* Accept literal file and size      */
#define	A_ERR	0x0002			/* Problem with specified attachment */
#define	A_TMP	0x0004			/* filename is temporary, delete it  */


/*
 * Master pine composer structure.  Right now there's not much checking
 * that any of these are pointing to something, so pine must have them pointing
 * somewhere.
 */
typedef struct pico_struct {
    void  *msgtext;			/* ptrs to malloc'd arrays of char */
    char  *pine_anchor;			/* ptr to pine anchor line */
    char  *pine_version;		/* string containing Pine's version */
    char  *alt_ed;			/* name of alternate editor or NULL */
    char  *alt_spell;			/* Checker to use other than "spell" */
    char  *oper_dir;			/* Operating dir (confine to tree) */
    char  *quote_str;			/* prepended to lines of quoted text */
    int    fillcolumn;			/* where to wrap */
    int    menu_rows;			/* number of rows in menu (0 or 2) */
    PATMT *attachments;			/* linked list of attachments */
    long   pine_flags;			/* entry mode flags */
    void  (*helper)();			/* Pine's help function  */
    int   (*showmsg)();			/* Pine's display_message */
    void  (*keybinit)();		/* Pine's keyboard initializer  */
    int   (*raw_io)();			/* Pine's Raw() */
    long  (*newmail)();			/* Pine's report_new_mail */
    long  (*msgntext)();		/* callback to get msg n's text */
    int   (*upload)();			/* callback to rcv uplaoded text */
    char *(*ckptdir)();			/* callback for checkpoint file dir */
    char *(*exittest)();		/* callback to verify exit request */
    char *(*canceltest)();		/* callback to verify cancel request */
    int   (*mimetype)();		/* callback to display mime type */
    void  (*expander)();		/* callback to expand address lists */
    void  (*resize)();			/* callback handling screen resize */
#if defined(DOS) || defined(HELPFILE)
    short search_help;
    short ins_help;
    short composer_help;
    short browse_help;
#else
    char  **search_help;
    char  **ins_help;
    char  **composer_help;
    char  **browse_help;
#endif
    struct headerentry *headents;
} PICO;


#ifdef	MOUSE
/*
 * Mouse buttons.
 */
#define M_BUTTON_LEFT		0
#define M_BUTTON_MIDDLE		1
#define M_BUTTON_RIGHT		2


/*
 * Flags.  (modifier keys)
 */
#define M_KEY_CONTROL		0x01	/* Control key was down. */
#define M_KEY_SHIFT		0x02	/* Shift key was down. */


/*
 * Mouse Events
 */
#define M_EVENT_DOWN		0x01	/* Mouse went down. */
#define M_EVENT_UP		0x02	/* Mouse went up. */
#define M_EVENT_TRACK		0x04	/* Mouse tracking */

/*
 * Mouse event information.
 */
typedef struct mouse_struct {
	char	mevent;		/* Indicates type of event:  Down, Up or 
				   Track */
	char	down;		/* TRUE when mouse down event */
	char	doubleclick;	/* TRUE when double click. */
	int	button;		/* button pressed. */
	int	flags;		/* What other keys pressed. */
	int	row;
	int	col;
} MOUSEPRESS;



#ifdef	ANSI
typedef	unsigned long (*mousehandler_t)(int, int, int, int, int);
#else
typedef	unsigned long (*mousehandler_t)();
#endif

typedef struct point {
    unsigned	r:8;		/* row value				*/
    unsigned	c:8;		/* column value				*/
} MPOINT;


typedef struct menuitem {
    unsigned	     val;	/* return value				*/
    mousehandler_t    action;	/* action to perform			*/
    MPOINT	     tl;	/* top-left corner of active area	*/
    MPOINT	     br;	/* bottom-right corner of active area	*/
    MPOINT	     lbl;	/* where the label starts		*/
    char	    *label;
    void            (*label_hiliter)();
    struct menuitem *next;
} MENUITEM;
#endif


/*
 * Structure used to manage keyboard input that comes as escape
 * sequences (arrow keys, function keys, etc.)
 */
typedef struct  KBSTREE {
	char	value;
        int     func;              /* Routine to handle it         */
	struct	KBSTREE *down; 
	struct	KBSTREE	*left;
} KBESC_T;

/*
 * Protos for functions used to manage keyboard escape sequences
 * NOTE: these may ot actually get defined under some OS's (ie, DOS, WIN)
 */
extern	int	kbseq();
extern	void	kpinsert();
extern	void	kbdestroy();


/*
 * various flags that they may passed to PICO
 */
#define	P_DELRUBS	0x40000000	/* map ^H to forwdel		   */
#define	P_LOCALLF	0x20000000	/* use local vs. NVT EOL	   */
#define	P_BODY		0x10000000	/* start composer in body	   */
#define	P_HEADEND	0x08000000	/* start composer at end of header */
#define	P_VIEW		MDVIEW		/* read-only			   */
#define	P_FKEYS		MDFKEY		/* run in function key mode 	   */
#define	P_SECURE	MDSCUR		/* run in restricted (demo) mode   */
#define	P_TREE		MDTREE		/* restrict to a subtree	   */
#define	P_SUSPEND	MDSSPD		/* allow ^Z suspension		   */
#define	P_ADVANCED	MDADVN		/* enable advanced features	   */
#define	P_CURDIR	MDCURDIR	/* use current dir for lookups	   */
#define	P_ALTNOW	MDALTNOW	/* enter alt ed sans hesitation	   */
#define	P_SUBSHELL	MDSPWN		/* spawn subshell for suspend	   */
#define	P_COMPLETE	MDCMPLT		/* enable file name completion     */
#define	P_DOTKILL	MDDTKILL	/* kill from dot to eol		   */
#define	P_SHOCUR	MDSHOCUR	/* cursor follows hilite in browser*/
#define	P_HIBITIGN	MDHBTIGN	/* ignore chars with hi bit set    */
#define	P_DOTFILES	MDDOTSOK	/* browser displays dot files	   */
#define	P_ABOOK		MDHDRONLY	/* called as address book editor   */
#define	P_ALLOW_GOTO	MDGOTO		/* support "Goto" in file browser  */

/*
 * definitions for various PICO modes 
 */
#define	MDWRAP		0x00000001	/* word wrap			*/
#define	MDSPELL		0x00000002	/* spell error parcing		*/
#define	MDEXACT		0x00000004	/* Exact matching for searches	*/
#define	MDVIEW		0x00000008	/* read-only buffer		*/
#define MDOVER		0x00000010	/* overwrite mode		*/
#define MDFKEY		0x00000020	/* function key  mode		*/
#define MDSCUR		0x00000040	/* secure (for demo) mode	*/
#define MDSSPD		0x00000080	/* suspendable mode		*/
#define MDADVN		0x00000100	/* Pico's advanced mode		*/
#define MDTOOL		0x00000200	/* "tool" mode (quick exit)	*/
#define MDBRONLY	0x00000400	/* indicates standalone browser	*/
#define MDCURDIR	0x00000800	/* use current dir for lookups	*/
#define MDALTNOW	0x00001000	/* enter alt ed sans hesitation */
#define	MDSPWN		0x00002000	/* spawn subshell for suspend	*/
#define	MDCMPLT		0x00004000	/* enable file name completion  */
#define	MDDTKILL	0x00008000	/* kill from dot to eol		*/
#define	MDSHOCUR	0x00010000	/* cursor follows hilite in browser */
#define	MDHBTIGN	0x00020000	/* ignore chars with hi bit set */
#define	MDDOTSOK	0x00040000	/* browser displays dot files   */
#define	MDEXTFB		0x00080000	/* stand alone File browser     */
#define	MDTREE		0x00100000	/* confine to a subtree         */
#define	MDMOUSE		0x00200000	/* allow mouse (part. in xterm) */
#define	MDONECOL	0x00400000	/* single column browser        */
#define	MDHDRONLY	0x00800000	/* header editing exclusively   */
#define	MDGOTO		0x01000000	/* support "Goto" in file browser */

/*
 * Main defs 
 */
#ifdef	maindef
PICO	*Pmaster = NULL;		/* composer specific stuff */
char	*version = "2.9";		/* PICO version number */

#else
extern	PICO *Pmaster;			/* composer specific stuff */
extern	char *version;			/* pico version! */

#endif	/* maindef */


/*
 * Flags for FileBrowser call
 */
#define FB_READ		0x0001		/* Looking for a file to read. */
#define FB_SAVE		0x0002		/* Looking for a file to save. */


/*
 * number of keystrokes to delay removing an error message, or new mail
 * notification, or checkpointing
 */
#define	MESSDELAY	25
#define	NMMESSDELAY	60
#ifndef CHKPTDELAY
#define	CHKPTDELAY	200
#endif


/*
 * defs for keypad and function keys...
 */
#define K_PAD_UP		0x0811
#define K_PAD_DOWN		0x0812
#define K_PAD_RIGHT		0x0813
#define K_PAD_LEFT		0x0814
#define K_PAD_PREVPAGE		0x0815
#define K_PAD_NEXTPAGE		0x0816
#define K_PAD_HOME		0x0817
#define K_PAD_END		0x0818
#define K_PAD_DELETE		0x0819
#define BADESC			0x0820
#define K_MOUSE			0x0821
#define K_SCROLLUPLINE		0x0822
#define K_SCROLLDOWNLINE	0x0823
#define K_SCROLLTO		0x0824
#define K_XTERM_MOUSE		0x0825
#define K_DOUBLE_ESC		0x0826
#define K_SWALLOW_TIL_Z		0x0827
#define K_SWALLOW_UP		0x0828
#define K_SWALLOW_DOWN		0x0829
#define K_SWALLOW_RIGHT		0x0830
#define K_SWALLOW_LEFT		0x0831
#define K_KERMIT		0x0832
#define NODATA			0x08FF
 
/*
 * defines for function keys
 */
#define F1      0x1001                  /* Functin key one              */
#define F2      0x1002                  /* Functin key two              */
#define F3      0x1003                  /* Functin key three            */
#define F4      0x1004                  /* Functin key four             */
#define F5      0x1005                  /* Functin key five             */
#define F6      0x1006                  /* Functin key six              */
#define F7      0x1007                  /* Functin key seven            */
#define F8      0x1008                  /* Functin key eight            */
#define F9      0x1009                  /* Functin key nine             */
#define F10     0x100A                  /* Functin key ten              */
#define F11     0x100B                  /* Functin key eleven           */
#define F12     0x100C                  /* Functin key twelve           */


/*
 * useful function definitions
 */
#ifdef	ANSI
int   pico(PICO *);
int   pico_file_browse(PICO *, char *, char *, char *, int);
void *pico_get(void);
void  pico_give(void *);
int   pico_readc(void *, unsigned char *);
int   pico_writec(void *, int);
int   pico_puts(void *, char *);
int   pico_seek(void *, long, int);
int   pico_replace(void *, char *);
int   pico_fncomplete(char *, char *, int);
#if defined(DOS) || defined(OS2)
int   pico_nfsetcolor(char *);
int   pico_nbsetcolor(char *);
int   pico_rfsetcolor(char *);
int   pico_rbsetcolor(char *);
#endif
#ifdef	MOUSE
int   init_mouse();
void  end_mouse();
int   mouseexist();
int   register_mfunc(mousehandler_t, int, int, int, int);
void  clear_mfunc(mousehandler_t);
unsigned long mouse_in_content(int, int, int, int, int);
unsigned long mouse_in_pico(int, int, int, int, int);
void  mouse_get_last(mousehandler_t *, MOUSEPRESS *);
int   checkmouse(unsigned *, int, int, int);
void  invert_label(int, MENUITEM *);
void  mouseon();
void  mouseoff();
#endif	/* MOUSE */

#else
int   pico();
int   pico_file_browse();
void *pico_get();
void  pico_give();
int   pico_readc();
int   pico_writec();
int   pico_puts();
int   pico_seek();
int   pico_replace();
int   pico_fncomplete();
#ifdef	MOUSE
int   init_mouse();
void  end_mouse();
int   mouseexist();
int   register_mfunc();
void  clear_mfunc();
unsigned long mouse_in_content();
void  mouse_get_last();
int   checkmouse();
void  invert_label();
void  mouseon();
void  mouseoff();
#endif	/* MOUSE */
#endif

#endif	/* PICO_H */
