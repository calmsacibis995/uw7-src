/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:oh/if_help.c	1.63.3.7"

#include	<stdio.h>
#include	<string.h>
#include        <curses.h>
#include 	<errno.h>
#include 	<ctype.h>	/* abs s14 */
#include	"inc.types.h"	/* abs s14 */
#include	"wish.h"
#include	"vtdefs.h"
#include	"ctl.h"
#include	"token.h"
#include	"winp.h"
#include	"form.h"
#include	"slk.h"
#include	"actrec.h"
#include	"typetab.h"
#include	"fm_mn_par.h"
#include	"objhelp.h"
#include	"var_arrays.h"
#include	"terror.h"
#include	"moremacros.h"
#include	"interrupt.h"
#include 	"vt.h"		/* abs for headers */
#include 	"sizes.h"
#include 	"message.h"

#define HL_INTR		PAR_INTR  
#define HL_ONINTR	PAR_ONINTR
#define HL_DONE		PAR_DONE
#define HL_TITLE	3 
#define HL_TEXT		4 
#define HL_WRAP		5 
#define HL_EDIT		6 
#define HL_INIT		7 
#define HL_LIFE		8 
#define HL_ROWS		9 
#define HL_COLUMNS	10
#define HL_BEGROW	11
#define HL_BEGCOL	12
#define HL_HELP		13
#define HL_REREAD	14
#define HL_CLOSE	15
#define HL_ALTSLKS	16
#define HL_FRMMSG	17
#define HL_HEADER       18	/* abs */

/* defined above
#define HL_INTR		PAR_INTR
#define HL_ONINTR	PAR_ONINTR
*/
#define HL_ACTI		PAR_ACTION
#define HL_NAME		PAR_NAME
#define HL_BUTT		4
#define HL_SHOW		5

#define HL_KEYS 19
/*** Third element message identifier for i18n message retrieval 
     side-effect: change of filldef() in partab.c
***/

static struct attribute Hl_tab[HL_KEYS] = {
	{ "interrupt",	RET_STR|EVAL_ALWAYS,    NULL, NULL, NULL, 0 },
	{ "oninterrupt",RET_STR|EVAL_ALWAYS,	NULL, NULL, NULL, 0 },
	{ "done",	RET_ARGS|EVAL_ALWAYS,	NULL, "", NULL, 0 }, /* abs */
	{ "title",	RET_STR|EVAL_ONCE,	":113", "Text", NULL, 0 },
	{ "text",	RET_STR|EVAL_ONCE,	NULL, NULL, NULL, 0 },
	{ "wrap",	RET_BOOL|EVAL_ONCE,	NULL, "", NULL, 0 },
	{ "edit",	RET_BOOL|EVAL_ONCE,	NULL, NULL, NULL, 0 },
	{ "init",	RET_BOOL|EVAL_ALWAYS,	NULL, "", NULL, 0 },
	{ "lifetime",	RET_STR|EVAL_ALWAYS,	NULL, "longterm", NULL, 0 },
	{ "rows",	RET_INT|EVAL_ONCE,	NULL, "-10", NULL, 0 }, /* abs s12 */
	{ "columns",	RET_INT|EVAL_ONCE,	NULL, "30", NULL, 0 },
	{ "begrow",	RET_STR|EVAL_ONCE,	NULL, "any", NULL, 0 },
	{ "begcol",	RET_STR|EVAL_ONCE,	NULL, "any", NULL, 0 },
	{ "help",	RET_ARGS|EVAL_ALWAYS,	NULL, NULL, NULL, 0 },
	{ "reread",	RET_BOOL|EVAL_ALWAYS,	NULL, NULL, NULL, 0 },
	{ "close",	RET_BOOL|EVAL_ONCE,	NULL, NULL, NULL, 0 },
	{ "altslks",	RET_BOOL|EVAL_ONCE,	NULL, NULL, NULL, 0 },
	{ "framemsg",	RET_STR|EVAL_ONCE,	NULL, "",   NULL, 0 },
        { "header",     RET_STR|EVAL_ONCE,      NULL, NULL, NULL, 0 } /* abs */
};

#define HL_FLD_KEYS 6
static struct attribute Hl_fld_tab[HL_FLD_KEYS] = {
	{ "interrupt",	RET_STR|EVAL_ALWAYS,    NULL,	NULL, NULL, 0 },
	{ "oninterrupt",RET_STR|EVAL_ALWAYS,	NULL,	NULL, NULL, 0 },
	{ "action",	RET_ARGS|EVAL_ALWAYS	,NULL,	NULL, NULL, 0 },
	{ "name",	RET_STR|EVAL_ONCE,	NULL,	NULL, NULL, 0 },
	{ "button",	RET_INT|EVAL_ONCE,	NULL,	"0", NULL, 0 },
	{ "show",	RET_BOOL|EVAL_SOMETIMES,NULL,	"", NULL, 0 }
};

#define CURhelp() (&(((helpinfo *) Cur_rec->odptr)->fm_mn))
#define CURhelpinfo() ((helpinfo *) Cur_rec->odptr)
#define ARGS() (((helpinfo *) Cur_rec->odptr)->args)
#define PTRS() (((helpinfo *) Cur_rec->odptr)->holdptrs)

extern int    Vflag;		/* abs k15 */
extern char  *strnsave();
extern char  *shrink_str();
static struct actrec *Cur_rec;
struct fm_mn parse_help();

static token bighelp_stream();

/*
** Returns a token so that the help object can be brought up.
*/
static token 
objhelp_help(a) 
struct actrec *a; 
{ 
    return(setaction(sing_eval(CURhelp(), HL_HELP)));
}

/*
** Frees up the structures and calls the close function.
*/
static int 
objhelp_close(a) 
struct actrec *a; 
{ 
    register int i, lcv;
    char *p, *strchr();

    Cur_rec = a;
    copyAltenv(ARGS());		/* in case HL_CLOSE references $ARGs abs k14*/
    form_close(a->id);		/* free the form FIRST */
    sing_eval(CURhelp(), HL_CLOSE);
    objhelp_noncur(a, FALSE);       /* remove ARGs from Altenv */

    /*
     * free information IN the helpinfo structure
     */
    freeitup(CURhelp());	/* the text parse table */ 
    if (PTRS())			/* holdptrs array */
	free(PTRS());
    lcv = array_len(ARGS());	/* the object specific variable */
    for (i = 0; i < lcv; i++) {	/* (e.g., $TEXT) */
	char namebuf[BUFSIZ];

	if (p = strchr(ARGS()[0], '='))
	    *p = '\0';
	strncpy(namebuf, ARGS()[0], BUFSIZ);
	namebuf[BUFSIZ-1] = '\0';
	if (p)
	    *p = '=';
	delaltenv(&ARGS(), namebuf);
	delAltenv(namebuf);
    }
    array_destroy(ARGS());	/* the object variable array */

    /*
     * Free information in the activation record structure
     */
    free(a->odptr);		/* the helpinfo structure itself */
    free(a->slks);		/* the object specific SLKS */
    free(a->path);		/* the definition file path */

    return(SUCCESS);
}

/*
** Checks to see whether to reread and if so, calls reread.
*/
static int
objhelp_reinit(a)
struct actrec *a;
{
    Cur_rec = a;
    if (sing_eval(CURhelp(), HL_REREAD))
	return(objhelp_reread(a));
    return(SUCCESS);
}

/* 
** count the number of lines needed to display string.  abs s13
*/
static long
count_lines(string, wrap, max_row, last_col)
unsigned char *string;
bool wrap;
long max_row, last_col;
{
    register unsigned char *sptr;
    register long row, col, done;
    unsigned char  *save_ptr;		/* abs s16 */
    long i, numspaces;

    col = 0;
    row = 1;
    sptr = string;
    done = FALSE;

    while (!done) {
	switch(*sptr) {
	case '\\':
	    switch(*(++sptr)) {
	    case 'b':
		if (col != 0)
		    col--;
		sptr++;
		break;
	    case '-':
		sptr += 2;
		continue;
		break;
	    case 'n':
	    case 'r':
		if (++row >= max_row)
		    return(max_row);
		col = 0;
		sptr++;
		break;
	    case '+':
		sptr += 2;
		continue;
		break;
	    case 't':
		numspaces = ((col + 8) & ~7) - col;
		for (i = 0; i < numspaces && col <= last_col;
		     i++, col++)
		    ;
		sptr++;
		break;
	    case EOS:
		done = TRUE;
		continue;
	    default:
		col++;
		sptr++;
		break;
	    }
	    break;		/* abs s17 */
	case '\n':
	case '\r':
	    if (++row >= max_row)
		return(max_row);
	    col = 0;
	    sptr++;
	    break;
	case '\b':
	    if (col != 0)
		col--;
	    sptr++;
	    break;
	case '\t':
	    numspaces = ((col + 8) & ~7) - col;
	    for (i = 0; i < numspaces && col <= last_col;
		 i++, col++)
		;
	    sptr++;
	    break;
	case EOS:
	    done = TRUE;
	    continue;
	default:
	    col++;
	    sptr++;
	    break;
	}
	if (col > last_col)
	{
	    if (wrap) 
	    {
		save_ptr = sptr; /* abs s16 */
		while (!isspace(*sptr) &&
		       sptr > string && col > 0) /* abs s16 */
		{		/* abs s16 */
		    col--;	/* abs s16 */
		    sptr--;
		}
		/* if word is longer than a line...
		 * we can't word wrap. 			   abs s16 */
		if (col == 0)	/* abs s16 */
		    sptr = save_ptr; /* abs s16 */
		col = 0;
		if (++row >= max_row)
		    return(max_row);
	    }
	    else
	    {
		if (*sptr != '\n')
		{
		    col = 0;
		    if (++row >= max_row)
			return(max_row);
		}
	    }
	}
    }
    return(row);
}




/*
** Front-end to parser(), which sets up defaults.
*/
static struct fm_mn
parse_help(flags, info_or_file, fp)
int flags;
char *info_or_file;
FILE *fp;
{
    struct fm_mn fm_mn;
    int i, j;
    long default_rows = 0L, table_rows, last_col;	/* abs s13 */
    long text_rows = 1L, header_rows = 0L;		/* abs s13 */
    bool wrap;						/* abs s13 */
    char *header_string, *text_string;         		/* abs s14 */

    fm_mn.single.attrs = NULL;
    fm_mn.multi = NULL;
    fm_mn.seqno = 1;		/*  Ensures that text and header descriptors
				    are not evaluated multiple times. */
    filldef(&fm_mn.single, Hl_tab, HL_KEYS);
    parser(flags, info_or_file, Hl_tab, HL_KEYS, &fm_mn.single,
	   Hl_fld_tab, HL_FLD_KEYS, &fm_mn.multi, fp);
    fm_mn.in_use_flag = Fm_mn_seq++;                   /* abs s18 */

    /*  If rows was not defined, default to the minimum of the
     *  lines of text plus header and negative of default from Hl_tab. abs s13
     */

    if (strtol(sing_eval(&fm_mn, HL_ROWS), (char **)NULL, 10) <= 0)
    {
	table_rows = -strtol(Hl_tab[HL_ROWS].def, (char **)NULL, 10);
	header_string = sing_eval(&fm_mn, HL_HEADER); 		/* abs s14 */
	text_string = sing_eval(&fm_mn, HL_TEXT); 		/* abs s14 */
	wrap = sing_eval(&fm_mn, HL_WRAP) ? TRUE : FALSE;
	last_col = strtol(sing_eval(&fm_mn, HL_COLUMNS), (char **)NULL, 10);
	if (header_string && *header_string)
	{
	    /* HL_WRAP doesn't apply to header */
	    header_rows= count_lines(header_string,FALSE, table_rows, last_col);
	    if (header_rows >= table_rows)
		default_rows = table_rows;
	}
	if (default_rows == 0L)	/* i.e. if not already == table_rows */
	{
	    text_rows = count_lines(text_string, wrap, table_rows - header_rows,
				    last_col);
	    /* note we are still limited by table_rows because of the
	     * subtraction in the count_lines call above...*/
	    default_rows = header_rows + text_rows; 
	}
	set_single_default(&fm_mn, HL_ROWS, itoa((int)default_rows, 10));
    }

    return(fm_mn);
}

/*
** Frees contents of old help, and sets new one.  Note:  odptr
** is set either way since freeitup will not free anything if
** the single array is empty
*/
static int
objhelp_reread(a)
register struct actrec *a;
{
    extern struct slk Defslk[MAX_SLK + 1];
    extern struct slk Textslk[];
    register int i;
    register struct fm_mn *fm_mn;
    register helpinfo *hi;
    char *label, *intr, *onintr, *get_def();
    int   lcv, but;
    FILE *fp = NULL;

    Cur_rec = a;
    fm_mn = CURhelp();
    hi = CURhelpinfo();

    /* make sure file exists and is readable (if there is a file) 
     * The "flags" say if a->path is  the information
     * itself or the file of where the information sits.  abs k15
     */
    if (!(hi->flags & INLINE))
	if ((fp = fopen(a->path, "r")) == NULL)
	{
	    if (a->id >= 0)	/* if frame is already posted */
		if (errno == EACCES)   /* No permissions */
			warn(UPDAT_NOPERM, a->path);
		else if (errno == ENOENT) /* File does not exist */
			warn(UPDAT_NOENT, a->path);
		else
			warn(NOT_UPDATED, a->path); /*Original message */
	    else
		if (errno == EACCES)   /* file exists, but no permissions */
			warn(FRAME_NOPERM, a->path);
		else if (errno == ENOENT) /* File does not exist */
			warn(FRAME_NOENT, a->path);
		else
			warn(FRAME_NOPEN, a->path); /*Original message */
	    return(FAIL);
	}
    if (a->id >= 0) {
        copyAltenv(ARGS());
	freeitup(fm_mn);
    } 
    hi->fm_mn = parse_help(hi->flags, a->path, fp);	/* abs k14.0 */
    if (fm_mn->single.attrs == NULL) {
	/*
	 * very strange indeed ...
	 *
	 if (a->id < 0)
	 sing_eval(fm_mn, HL_CLOSE);
	 */
	return(FAIL);
    }
    if (PTRS())
	free(PTRS());
    lcv = sing_eval(fm_mn, HL_HEADER) ? 2:1;
    if ((PTRS() = (char **) calloc(lcv, sizeof(char *))) == NULL)
	fatal(NOMEM, nil);
    for (i = 0; i < lcv; i++)
	PTRS()[i] = (char *) NULL;	
    fm_mn->seqno = 1;
    hl_vislist(hi);

    /*
     * If "init=false" then clean-up
     */
    if (!sing_eval(CURhelp(), HL_INIT))
    {
	if (a->id >= 0)		/* form is already posted */
	{
	    if (a->lifetime == AR_INITIAL)
	    {
		(void)mess_err( gettxt(":108","can't close this frame") );
	    }
	    else
	    {
		ar_close(a, FALSE);
		return(FAIL);
	    }
	}
	else
	{
	    sing_eval(CURhelp(),HL_CLOSE);
	    objhelp_noncur(a, TRUE); /* remove ARGs from Altenv */
	    freeitup(CURhelp());
	    return(FAIL);
	}
    }
    /*
     * update the interrupt descriptors in the activation rec
     */
    ar_ctl(Cur_rec, CTSETINTR, get_sing_def(CURhelp(), HL_INTR));
    ar_ctl(Cur_rec, CTSETONINTR, get_sing_def(CURhelp(), HL_ONINTR));

    /*
     * Set up object's SLK array
     */
    set_top_slks(Textslk);
    memcpy((char *)a->slks, (char *)Defslk, sizeof(Defslk));
    lcv = array_len(hi->slks);
    for (i = 0; i < lcv; i++) {
	but = atoi(multi_eval(fm_mn, hi->slks[i], HL_BUTT)) - 1;
	if (but <  0 || but >= MAX_SLK)	/* abs */
	    continue;
	label = multi_eval(fm_mn, hi->slks[i], HL_NAME);
	intr  = get_def(CURhelp(),hi->slks[i], HL_INTR);
	onintr  = get_def(CURhelp(),hi->slks[i], HL_ONINTR);
	set_obj_slk(&(a->slks[but]), label, TOK_SLK1 + but, intr, onintr);
    }
    if (a->id >= 0)
    {
	wclrwin((vt_id)form_ctl(a->id, CTGETVT)); /* clear old text. abs s13 */
	form_ctl(a->id, CTSETDIRTY);
    }
    (void) ar_ctl(Cur_rec, CTSETMSG, FALSE); /* was AR_cur.  abs k15 */
    return(SUCCESS);
}

/*
** Takes this object's information out of the major altenv.
*/
static int 
objhelp_noncur(a, all) 
struct actrec *a;
bool all;
{
    register int i;
    register char *p;
    int	lcv;

    Cur_rec = a;
    lcv = array_len(ARGS());
    for (i = 0; i < lcv; i++) {
	char namebuf[BUFSIZ];

	if (p = strchr(ARGS()[0], '='))      /* 0 since delAltenv.. abs s15 */
	    *p = '\0';
	strncpy(namebuf, ARGS()[0], BUFSIZ); /* ..shifts the array. abs s15 */
	namebuf[BUFSIZ - 1] = '\0';
	if (p)
	    *p = '=';
	delAltenv(namebuf);
    }
    if (all)
	return(form_noncurrent(a->id));
    else
	return(SUCCESS);
}

/*
** Puts this object's altenv() into the major altenv().
*/
static int 
objhelp_current(a) 
struct actrec *a; 
{
    int *Olist, *list;
    int ret;

    Cur_rec = a;
    copyAltenv(ARGS());
    ret = form_current(a->id);
    form_ctl(a->id, CTSETPOS, 1, 0, 0); /* `1' is "text=" field  abs*/
    return(ret);
}

/*
** Sets up SLK array, based on show functions.
*/
hl_vislist(hi)
helpinfo *hi;
{
    int i;
    struct fm_mn *ptr;
    int	lcv;
	
    ptr = &(hi->fm_mn);
    if (!hi->slks)
	hi->slks = (int *) array_create(sizeof(int), array_len(ptr->multi));
    else
	array_trunc(hi->slks);

    lcv = array_len(ptr->multi);
    for (i = 0; i < lcv; i++)
	if (multi_eval(ptr, i, HL_SHOW))
	    hi->slks = (int *) array_append(hi->slks, (char *) &i);
}
#define MIN_ROWS_TEXT 2		/* abs s18 */

/* Size a text header */
header_size(m)     /* , vt_exists) abs s16 */
formfield m;
/* int vt_exists;		   abs s16 */
{
    register struct vt *v;
    int  rows, cols;
    int lines, max_rows;
    int rows_4_text;		/* abs s18 */
    bool wrap;			/* abs s18 */

    if (*m.value == 0)		/* null string */
	return(0);

/*    if (vt_exists)						 abs s14 
**    {
**	v = &VT_array[VT_curid];
**	getmaxyx(v->win, rows, cols);
**	max_rows = rows - MIN_ROWS_TEXT;			 abs s13 
**    }
**    else
**    {
**	max_rows = LINES - RESERVED_LINES - MIN_ROWS_TEXT;	 abs s14 
**	cols = COLS - 2;					 abs s14
**    }
abs s16  header is constrained by rows & columns descriptors not by frame size.
*/
    /* figure out how many rows to leave for text;
     * leave at least MIN_ROWS_TEXT rows unless less are needed.
     * abs s18
     */
    wrap = sing_eval(CURhelp(), HL_WRAP) ? TRUE : FALSE; 	  /* abs s18 */
    rows_4_text = (int)count_lines(sing_eval(CURhelp(), HL_TEXT), /* abs s18 */
				   wrap, MIN_ROWS_TEXT, m.cols);  /* abs s18 */
    max_rows = atoi(sing_eval(CURhelp(), HL_ROWS)) - rows_4_text; /* abs s18 */

    /* HL_WRAP doesn't apply to header hence FALSE below           abs s13 */
    lines = (int)count_lines(m.value, FALSE, max_rows, m.cols);	/* abs s16 */
    return(lines);		     				/* abs s13 */

/*  inaccurate. abs s13
**  for (linefeeds = 1, c = m.value; *c != NULL; c++)
**	if (*c == '\n')
**	    linefeeds++;
**  return(linefeeds > rows - MIN_ROWS_TEXT) ? rows - MIN_ROWS_TEXT : linefeeds;
*/
}


/*
** Gives header and text as only fields, fields that have no names.
*/
static formfield
objhelp_disp(n, hi)
int n;
helpinfo *hi;
{
    register int i;
    struct ott_entry *entry;
    struct fm_mn *ptr;
/**    char *readfile(); unused abs s14 */
    formfield m;
    static int header_rows;

    ptr = &(hi->fm_mn);
    switch (n)
    {
    case 0:			/* non-scrolling header field. abs8/88 */
	m.name = strsave("");
	m.value = sing_eval(ptr, HL_HEADER);
	m.frow = 0;
	m.fcol = 0;
	m.nrow = VT_UNDEFINED;
	m.ncol = VT_UNDEFINED;
	m.cols = atoi(sing_eval(CURhelp(), HL_COLUMNS));
	m.rows = header_rows =
	    header_size(m); /*, hi->flags & VT_EXISTS); abs s16 */ /* abs s14 */
	m.flags = I_FANCY | I_NOEDIT | I_TEXT;
	m.ptr = PTRS();
	break;
    case 1:			/* text field */
	m.name = strsave("");
	m.value = sing_eval(ptr, HL_TEXT);
	m.frow = header_rows;	/* header has rows 0 -> header_rows - 1 */
	m.fcol = 0;
	m.nrow = VT_UNDEFINED;
	m.ncol = VT_UNDEFINED;
	m.rows = atoi(sing_eval(CURhelp(), HL_ROWS)) - header_rows;
	m.cols = atoi(sing_eval(CURhelp(), HL_COLUMNS));
	m.flags = I_FANCY|I_SCROLL|I_TEXT;
	if (header_rows == 0)	    /* curses optimization.. */
	    m.flags |= I_FULLWIN;   /* ..if no subwindows needed */
	if (!sing_eval(CURhelp(), HL_EDIT))
	    m.flags |= I_NOEDIT;
	if (sing_eval(CURhelp(), HL_WRAP))
	    m.flags |= I_WRAP;
	m.ptr = PTRS() + 1;
	break;
    default:
	m.name = NULL;
    }
    return(m);
}


/*
** There are no args, so return FAIL. Otherwise, pass it on.
*/
int
objhelp_ctl(rec, cmd, arg1, arg2, arg3, arg4, arg5, arg6)
struct actrec *rec;
int cmd;
int arg1, arg2, arg3, arg4, arg5, arg6;
{
    if (cmd == CTGETARG)
	return(FAIL);
    else if (cmd == CTSETMSG) {
	if (arg1 == TRUE) {
	    /* 
	     * if arg1 == TRUE then the frame message was
	     * generated "externally" (i.e., via the message
	     * built-it).  Update the "framemsg" descriptor
	     * accordingly.
	     */
	    char *newmsg, *get_mess_frame();

	    newmsg = get_mess_frame();
	    set_single_default(CURhelp(), HL_FRMMSG, newmsg);
	}
	else
	{
	    mess_frame(sing_eval(CURhelp(), HL_FRMMSG));
	}

	return(SUCCESS);
    }
    if (cmd == CTSETLIFE) {
	char *life;

	life = sing_eval((&(((helpinfo *) rec->odptr)->fm_mn)), HL_LIFE);
	setlifetime(rec, life);
	return(SUCCESS);
    }
    return(form_ctl(rec->id, cmd, arg1, arg2, arg3, arg4, arg5, arg6));
}

/*
** Uses path_to_ar and nextpath_to_ar to see if it is a reopen.  If
** so, make it current.  Otherwise, set up the actrec and call 
** ar_create.
*/
int
IF_helpopen(args)
register char **args;
{
    register int i;
    int type, startrow, startcol;
    char *begrow, *begcol;
    struct actrec a, *first_rec, *ar_create(), *path_to_ar(), *nextpath_to_ar();
    int inline;
    struct fm_mn *fm_mn;
    extern struct slk Defslk[MAX_SLK + 1];
    extern  char *filename();
    helpinfo *hi;
    char *life;
    char buf[BUFSIZ], envbuf[6];
    char *ptr;

    if (strCcmp(args[0], "-i") == 0)
    {
	inline = TRUE;
	Cur_rec = path_to_ar(args[1]);
    }
    else
    {
	inline = FALSE;
	Cur_rec = path_to_ar(args[0]);
    }

    for (first_rec = Cur_rec; Cur_rec; ) {
	char *env, *getaltenv();

	strcpy(envbuf, "ARG1");
	for (i = inline ? 2 : 1; (env = getaltenv(ARGS(), envbuf)) && args[i];
	     envbuf[3]++, i++)
	    if (strcmp(args[i], env))
		break;
	if (!args[i] && !env) {
	    ar_current(Cur_rec, TRUE); /* abs k15 */
	    return(SUCCESS);
	}
	Cur_rec = nextpath_to_ar(Cur_rec);
	if (Cur_rec == first_rec) /* circular list */
	    break;
    }
    hi = (helpinfo *)new(helpinfo);
    hi->flags = inline ? INLINE : 0;
    hi->args = NULL;
    a.id = -1;
    a.odptr = (char *) hi;
    fm_mn = &(hi->fm_mn);
    fm_mn->single.attrs = NULL;
    if (inline)
	a.path = strsave(args[1]);
    else
	a.path = strsave(args[0]);
    if ((a.slks = (struct slk *) malloc(sizeof(Defslk))) == NULL)
	fatal(NOMEM, nil);
    a.fcntbl[AR_CLOSE] = objhelp_close;
    a.fcntbl[AR_REREAD] = objhelp_reread;
    a.fcntbl[AR_REINIT] = objhelp_reinit;
    a.fcntbl[AR_CURRENT] = objhelp_current;
    a.fcntbl[AR_TEMP_CUR] = objhelp_current; /* abs k15. optimize later */
    a.fcntbl[AR_NONCUR] = objhelp_noncur;
    a.fcntbl[AR_ODSH] = (int (*)())bighelp_stream; /* added cast abs */
    a.fcntbl[AR_HELP] = (int (*)())objhelp_help; /* added cast abs */
    a.fcntbl[AR_CTL] = objhelp_ctl;
    Cur_rec = &a;
    setupenv(hi->flags, args, &ARGS());
    if (objhelp_reread(&a) == FAIL)
	return(FAIL);
    ptr = strnsave("TEXT=", (int)strlen(life = sing_eval(fm_mn, HL_TEXT)) + 6);
    strcat(ptr, life);
    putaltenv(&ARGS(), ptr);
    putAltenv(ptr);
    free(ptr);
    begrow = sing_eval(fm_mn, HL_BEGROW);
    begcol = sing_eval(fm_mn, HL_BEGCOL);
    life = sing_eval(fm_mn, HL_LIFE);
    life_and_pos(&a, life, begrow, begcol, &startrow, &startcol, &type);

    if (Vflag)			/* abs k15 */
	strcpy(buf, shrink_str(filename(sing_eval(fm_mn, HL_TITLE)), MAX_TITLE));
    else			/* abs k15 */
	strcpy(buf, shrink_str(sing_eval(fm_mn, HL_TITLE), MAX_TITLE));
    a.id = form_default(buf, type, startrow, startcol,
			objhelp_disp, (char *)hi);
    if (a.id == FAIL)
	return(FAIL);

    hi->flags |= VT_EXISTS;	/* abs a14 */

    if (sing_eval(fm_mn, HL_ALTSLKS))
	a.flags = AR_ALTSLKS;
    else
	a.flags = 0;
    return(ar_current(Cur_rec = ar_create(&a), FALSE));	/* abs k15 */
}

/*
** Intercepts SLKs after the editor. Also, TOK_SAVE is an exit.
*/
token
help_stream(tok)
register token tok;
{
    int line; 
    char *lininfo;
    char *buf, *s;
    int *slks;
    int	lcv;
	

    s = NULL;
    if (tok >= TOK_SLK1 && tok <= TOK_SLK16) {
	int num;
	int i;

	slks = CURhelpinfo()->slks;
	num = tok - TOK_SLK1 + 1;
	lcv = array_len(slks);
	for (i = 0; i < lcv; i++)
	    if (atoi(multi_eval(CURhelp(), slks[i], HL_BUTT)) == num) {
		form_ctl(Cur_rec->id, CTGETARG, &s);
		if (sing_eval(CURhelp(), HL_EDIT))
		    set_sing_cur(CURhelp(), HL_TEXT, strsave(s));
		buf = strnsave("TEXT=", (int)strlen(s) + 6);
		strcat(buf, s);
		putaltenv(&ARGS(), buf);
		putAltenv(buf);
		tok = setaction(multi_eval(CURhelp(), slks[i], HL_ACTI));
		free(buf);
		break;
	    }
    }
    if (tok == TOK_SAVE)
	tok = TOK_CLOSE;
    if (tok == TOK_CLOSE) {
	if (!s) {
	    form_ctl(Cur_rec->id, CTGETARG, &s);
	    buf = strnsave("TEXT=", (int)strlen(s) + 6);
	    strcat(buf, s);
	    putaltenv(&ARGS(), buf);
	    putAltenv(buf);
	    free(buf);
	}
	/* tok = sing_eval(CURhelp(), HL_DONE) ? TOK_CLOSE : TOK_BADCHAR; abs */
	tok = make_action(sing_eval(CURhelp(), HL_DONE));
    }
    return(tok);
}

/*
** Sets up stream and calls stream.
*/
static token
bighelp_stream(a, t)
struct actrec *a;
register token t;
{
    token (*func[3])();
    extern int field_stream();
    register int olifetime;

    Cur_rec = a;
    olifetime = Cur_rec->lifetime;
    Cur_rec->lifetime = AR_PERMANENT;
    func[0] = (token (*)())field_stream; /* added cast  abs */
    func[1] = help_stream;
    func[2] = NULL;
    t = stream(t, func);
    Cur_rec->lifetime = olifetime;
    return(t);
}

