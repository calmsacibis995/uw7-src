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
#ident	"@(#)fmli:wish/global.c	1.106.6.2"

#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	<time.h>	/* for glob_time() */
#include	"wish.h"
#include	"token.h"
#include	"slk.h"
#include	"actrec.h"
#include	"ctl.h"
#include	"terror.h"
#include	"moremacros.h"
#include	"message.h"
#include        "inc.types.h"   /* abs s14 */
#include        <unistd.h> 

#define MAX_OBJNAME	15

/*
 * Caution: MAX_ARGS is defined in other files and should ultimately reside 
 * in wish.h 
 */
#define MAX_ARGS	25	

static char Objtype[MAX_OBJNAME];
/* dmd TCB */
static char Release[] = "FMLI Release 4.2MP Version UW2.1";

/* The global stream handler.  Handles most tokens */

/* the number of arguements for the current object or command */
int Arg_count;
/* the arguements for the current object or command */
char *Args[MAX_ARGS];

/* some strings that are used several times in this source file */

static char Open[] = "OPEN";
static char Menu[] = "MENU";
static char Form[] = "FORM";
static char Text[] = "TEXT";
static char Any[]  = "Any";
static char Extra_args[] = "Extra arguments ignored";

static void glob_time();

char *path_to_full(), *cur_path();
struct actrec *window_arg(), *wdw_to_ar();
extern time_t	  time();	/* EFT abs k16 */
extern struct tm *localtime();  /* abs k16 */
extern int        mess_err();	/* abs s15 */

token
global_stream(t)
token t;
{
    register int i;
    char *path, *name, *p;
    struct actrec *a;
    extern char *Filesys;
    char *tok_to_cmd();
    char	*bsd_path_to_title();
    struct actrec *path_to_ar();
    char *expand(), *path_arg();
    token gotoarg();

    char *i18n_cmd;
    char *i18n_msg;
    int  i18n_length;

#ifdef _DEBUG5
    _debug5(stderr, "global_stream(%o)\n", t);
#endif
    /*
     * no more flashing token message (yuk!)
     *
     * if (p = tok_to_cmd(t))
     * {
     *	mess_flash(p);
     *	doupdate();
     * }
     */
    switch (t) {
	/* commands which process objects */
    case TOK_HELP:
	working(TRUE);
	if (Arg_count <= 1) {
	    extern char	*Pending_op;

	    if (Pending_op)
	      {
		/* 
		** Text.help may expect the 3-rd parameter to be the
		** language dependend title. Do I have one here?
		** Added a 3-rd "Pending_op" to the parameter list of
		** objop.
		*/
		objop(Open, Text, "$VMSYS/OBJECTS/Text.help",
		      Pending_op, Pending_op, Pending_op, NULL);
	      }
	    else
		t = (token) ar_help(ar_get_current());
	}
	else 
	    t = cmd_help(Args[1]);
	if (t == TOK_OPEN)
	    goto openit;
	else if (t == TOK_TEXTFRAME) 		/* abs s16 */
	    goto textframe;			/* abs s16 */
	return TOK_NOP;
    case TOK_SHOW_PATH:		/* checkprint */
	working(TRUE);

    	if (Arg_count > 1)
	    (void) (void)mess_temp(Extra_args); 		/* abs s16 */
    	if (ar_ctl(ar_get_current(), CTGETARG, &p) != FAIL && p && *p)
	    (void) objop(Open, Text, "$VMSYS/OBJECTS/Text.show", p, NULL);
	else
	    (void) (void)mess_err( gettxt(":307","The current frame must be a non-empty folder to use this command.") );	/* abs s15 */

	return TOK_NOP;
    case TOK_SECURITY:		/* security */
	working(TRUE);
	if (path = path_arg()) {
	    if ( access(path,00) == FAIL )
    
		(void)mess_err( gettxt(":308","object does not exist") ); 	
                /* abs s15 */
	    else
		objop(Open, Form, "$VMSYS/OBJECTS/Form.sec", 
		      path, bsd_path_to_title(path, 0), NULL);
	    return TOK_NOP;
	}
	break;
    case TOK_ORGANIZE:		/* organize */
	working(TRUE);
	if (a = window_arg(Arg_count - 1, Args + 1, 1)) {
	    bool b;

	    if (ar_ctl(a, CTISDEST, &b) == FAIL || b == FALSE)

		(void)mess_err( gettxt(":309","You may only organize File folders") ); /* abs s15 */
	    else if (access(a->path,02) == FAIL) 
      
		(void)mess_err( gettxt(":310","You do not have write permission to organize this folder") ); /* abs s15 */
	    else {
		objop(Open, Form, "$VMSYS/OBJECTS/Form.org", a->path, 
		      bsd_path_to_title(a->path, 0), NULL);
		return TOK_NOP;
	    }
	} else

	    (void)mess_err( gettxt(":311","You must open the File folder to organize") ); /* abs s15 */
	break;
    case TOK_FIND:		/* find */
	working(TRUE);
	if (parse_n_in_fold(&name, &path) == SUCCESS) {
	    if (name == NULL)
		objop(Open, Form, "$VMSYS/OBJECTS/Form.find", path, NULL);
	    else
		objop(Open, Menu, "$VMSYS/OBJECTS/Menu.find", path,
		      name, Any, Any, Any, NULL);
	    return TOK_NOP;
	}
	break;

	/* commands which are activation record functions */

    case TOK_CLEANUP:		/* cleanup */
	working(TRUE);
	mess_init();    /* remove temporary/frame messages  */
	(void) ar_cleanup(AR_LONGTERM);
	return TOK_NOP;
    case TOK_GOTO:		/* goto */
	if (Arg_count <= 1)

	    get_string(gotoarg, gettxt(":312","Enter a frame number or path: "),
		       nil, 0, FALSE, "goto", "goto");
	else 
	    gotoarg(NULL, t);
	return TOK_NOP;
/*  case TOK_TAB:		   tab  Removed this mapping abs k16 */
    case TOK_NEXT_WDW:		/* next_wdw */
	ar_cur_next();
	return TOK_NOP;
/*  case TOK_BTAB:		   backtab  Removed this mapping abs k16 */
    case TOK_PREV_WDW:		/* prev_wdw */
	ar_cur_prev();
	return TOK_NOP;
    case TOK_LOGOUT:		/* logout */
	working(TRUE);
	arf_noncur(AR_cur, AR_cur);	/* clean up $ARGs */
	ar_cleanup(AR_INITIAL);
	break;
    case TOK_DEBUG:
	/* les
	   mdump();
	   */
	return TOK_NOP;
    case TOK_REREAD:
	working(TRUE);
	if (Arg_count <= 1)
	    ar_reread(ar_get_current());
	else {
	    struct actrec *destar, *savear;
	    int save_life;

	    savear = ar_get_current();
	    if (destar = window_arg(Arg_count - 1, Args + 1, 2)) {
		if (destar == savear)
		    ar_reread(savear);
		else {
		    bool restore_ar = FALSE;

		    if ((Arg_count < 3) || strCcmp(Args[2], "true")) /* abs k15 */
		    {
			restore_ar = TRUE;
			/* don't let lifetime re-evaluate or frame close if
			   shortterm  when destar is made current. abs k15*/
			save_life = savear->lifetime;
			savear->lifetime = AR_INITIAL;
		    }
		    /*
		     * - make the destination AR current
		     *   (to update the window immediately) 
		     * - if there is no third argument then
		     *   make the save AR current
		     */
		    ar_current(destar, FALSE); /* abs k15 */
		    ar_reread(destar);
		    if (restore_ar == TRUE)
		    {
			ar_current(savear, FALSE); /* abs k15 */
			savear->lifetime = save_life;
		    }
		}
	    }
	}
	return TOK_NOP;
    case TOK_CLOSE:
    {
	struct actrec *a;
	register int i;
	int close_cur, count;
	char buf[BUFSIZ];

	working(TRUE);
	if (Arg_count <= 1) {
	    a = ar_get_current();
	    if (!a) 
		(void)mess_err( gettxt(":313","Can't find any frames\n") );
                /* abs s15 */

	    /*  commented out by njp for F15 - this code was
		moved to actrec.c in ar_close.

	    else if (a->lifetime == AR_IMMORTAL ||
		     a->lifetime == AR_INITIAL)
		mess_temp("Can't close this frame\n", Args + i);
	     */

	    else {
		ar_close(a, FALSE);
		init_modes();
		ar_checkworld(TRUE);
	    }
	    return(TOK_NOP);
	}
	for (i = 1; i < Arg_count; i++) {
	    a = window_arg(1, Args + i, 1);
	    if (!a) {

              sprintf(buf, gettxt(":314","Can't find frame \"%s\"\n"), Args[i] );
              (void)mess_err(buf); 		/* abs s15 */
	    }
/** test is now in ar_close. abs k17
            else if (a->lifetime == AR_IMMORTAL ||
		     a->lifetime == AR_INITIAL) {
              sprintf(buf, "Can't close frame \"%s\"\n", Args[i]);
              mess_temp(buf);
	    }
**/
	    else {
		ar_close(a, FALSE);
		init_modes();
		ar_checkworld(TRUE);
	    }
	}
	return TOK_NOP;
    }
	break;
    case TOK_CHECKWORLD:	/* force the world to be checked */
	ar_checkworld(TRUE);
	return TOK_NOP;
    case TOK_UNK_CMD:	 /* unknown command, if number, goto, else like open */
	if (!Args[0])
		return(TOK_NOP); /* carey M2 */
	if ((i = atoi(Args[0])) && wdw_to_ar(i) &&
	    strspn(Args[0], "0123456789") == strlen(Args[0]))
	{
	    ar_current(wdw_to_ar(i), TRUE); /* abs k15 */
	    return(TOK_NOP);
	} else {
	    if (Args[MAX_ARGS-1])
		free(Args[MAX_ARGS-1]); /* les */

	    for (i = MAX_ARGS-1; i > 0; i--)
		Args[i] = Args[i-1];

	    Args[0] = strsave(Open);

	    Arg_count++;
	    t = TOK_OPEN;
	    /* fall through to open - no break!! */
	}

	/* object operations */

    openit:
    case TOK_OPEN:		/* open */
    case TOK_ENTER:
    case TOK_COPY:		/* copy */
    case TOK_MOVE:		/* move */
    case TOK_REPLACE:		/* rename */
    case TOK_SCRAMBLE:		/* scramble */
    case TOK_UNSCRAMBLE:	/* unscramble */
    case TOK_PRINT:		/* print */
    case TOK_DELETE:		/* delete */
    case TOK_UNDELETE:		/* undelete */
    case TOK_OBJOP:		/* any other object operation */
	working(TRUE);
#ifdef _DEBUG
	_debug(stderr, "In global handling object operation\n");
#endif
	if (objop_args(t) != FAIL)
	    if (objopv(tok_to_cmd(t), Objtype, &Args[1]) == FAIL)
	        ar_current(AR_cur, FALSE);   /* undo damage done. abs k14,k15 */
	ar_checkworld(TRUE);
	return TOK_NOP;
    case TOK_SREPLACE:		/* redescribe */
	working(TRUE);
	if (objop_args(t) != FAIL) {
	    if (Arg_count <= 2)
		enter_getname("redescribe", NULL, &Args[1]);
	    else {
		redescribe(&Args[1]);
		ar_checkworld(TRUE);
	    }
	}
	return TOK_NOP;
    case TOK_DISPLAY:		/* display */
	working(TRUE);
	if (objop_args(t) != FAIL)
	    (void) glob_display(Args[1]);
	return TOK_NOP;
    case TOK_RUN:
	working(TRUE);
	if (objop_args(t) != FAIL) {
	    objopv(Open, "EXECUTABLE", &Args[1]);
	    ar_checkworld(TRUE);
	    return(TOK_NOP);
	}
	break;
    case TOK_CREATE:		/* create */
	working(TRUE);
	return glob_create();

	/* system functions */

    case TOK_SELECT:		/* select */
	glob_select();
	return TOK_NOP;
    case TOK_CANCEL:		/* cancel out of a browse */
	mess_perm(NULL);
	glob_browse_cancel();
	return TOK_NOP;
    case TOK_REFRESH:		/* refresh the screen */
	vt_redraw();
	return TOK_NOP;
    case TOK_TIME:		/* time */
	glob_time();
	return TOK_NOP;
    case TOK_UNIX:		/* unix */
	working(TRUE);

        i18n_msg = gettxt (":315","\"To return, type 'exit' or control-d\nYou are in `pwd`\"");
        i18n_length = strlen (i18n_msg);
        i18n_cmd = malloc(5 + i18n_length + 25);  
                                                  /*
                                                     5 -> length of "echo "
                                                    25 -> length of "; exec ..."
                                                          + 1 for '\0'
                                                  */

        strcpy( i18n_cmd, "echo ");
        strcat( i18n_cmd, i18n_msg);
        strcat( i18n_cmd, "; exec ${SHELL:-/bin/sh}");

	(void) proc_open(0, "UNIX_System", NULL, "sh", "-c",i18n_cmd, NULL);
	ar_checkworld(TRUE);	/* always check after a unix escape! */
        free(i18n_cmd);
	return TOK_NOP;
    case TOK_WDWMGMT:		/* wdw_mgmt */
	working(TRUE);
	if (Arg_count <= 1)
	    mgmt_create(NULL);
	else
	    mgmt_create(Args[1]);
	return TOK_NOP;
    case TOK_CMD:
	working(TRUE);
	cmd_create();
	return TOK_NOP;
    case TOK_SET:
	working(TRUE);
	if (Arg_count < 3) {

	    (void)mess_err( gettxt(":316","Not enough arguments") );
            /* abs s15 */
	    break;
	} else {
	    char buf[MESSIZ];

	    strcpy(buf, Args[2]);
	    for (i = 3; Args[i]; i++) {
		strcat(buf, " ");
		strcat(buf, Args[i]);
	    }
	    chgepenv(Args[1], buf);
	    mess_temp(nstrcat(Args[1], " ==> ", buf, NULL));
	    return TOK_NOP;
	}
    case TOK_TOGSLK:
	slk_toggle();
	return TOK_NOP;
    case TOK_RELEASE:
	working(TRUE);
	(void)mess_err(Release); 		/* abs s15 */
	return TOK_NOP;
    textframe:					/* abs s16 */
    case TOK_TEXTFRAME:				/* abs s15 */
	working(TRUE);				/* abs s15 */
	cmd_textframe();			/* abs s15 */
	return(global_stream(TOK_OPEN)); 	/* abs s15 */
    case TOK_NOP:
    default:
	break;
    }
    return t;
}


/*
 * This function looks at its arglist, and parses its argument into an
 * activation record.  The argument could be either a window number or a path
 */
struct actrec *
window_arg(argc, argv, maxargs)
int	argc;
char	*argv[];
int	maxargs;
{
    char	*p;
    int n;
    struct actrec	*ret;
    struct actrec	*path_to_ar();
    struct actrec	*wdw_to_ar();

    if (argc <= 0) {
	ret = ar_get_current();
	return(ret);
    }
    else {
	if (argc > maxargs)
	    (void)mess_temp(Extra_args);			/* abs s16 */
	if ((n = atoi(argv[0])) &&
	    (strspn(argv[0], "0123456789") == strlen(argv[0])) &&
	    (ret = wdw_to_ar(n)))
	      return(ret);
	else {
	    p = path_to_full(argv[0]);
	    ret = path_to_ar(p);
	    free(p);
	    return ret;
	}
    }
}

/*
 * this is used for those functions that require an argument that is
 * a path inside a window by default, or a full path.
 */
static char *
path_arg()
{
    char	*p;
    extern char	*Filecabinet;

    if (Arg_count > 2)
	(void)mess_temp(Extra_args); 		/* abs s16 */
    if (Arg_count >= 2)
	return path_to_full(Args[1]);
    else if (ar_ctl(ar_get_current(), CTGETARG, &p) != FAIL && p && *p)
	return path_to_full(p);
    return Filecabinet;
}

/*
 * These should really go in separate source files when they are 
 * finally finished.
 */
/* re-written for internationalization... see below.  abs s13
 * static
 * glob_time()
 * {
 *     char	buf[12];
 *     time_t	t;		
 *     register struct tm	*tp;
 * 
 *     t = time((time_t)0L);
 *     tp = localtime(&t);
 *     sprintf(buf, "%d:%02.2d:%02.2d %cM",
 * 	    tp->tm_hour % 12 ? tp->tm_hour % 12 : 12,
 * 	    tp->tm_min, tp->tm_sec, tp->tm_hour >= 12 ? 'P' : 'A');
 *     mess_temp(buf);
 * }
*/

/* FACE time command.  Display time of day on message line */
static void
glob_time()
{
    time_t t;
    char *ictime();

    time(&t);
    (void)mess_err(ictime(&t));	/* abs s15; internationalize.  abs s13 */
}

/* prepare the argument array for an impending object operation */

static int
objop_args(t)
token t;
{
    char *p;
    register int i;

    strcpy(Objtype, "OBJECT");
    if (Arg_count <= 1) {	/* current object is the arg */
	if (ar_ctl(ar_get_current(), CTGETARG, &Args[1]) == FAIL) {
	    glob_mess_nosrc(t);
	    return(FAIL);
	}

	Args[1] = strsave(Args[1]);
	if ( Args[2] )
	    free( Args[ 2 ] );	/* les */
	Args[2] = NULL;
	Arg_count = 2;
	return(SUCCESS);
    }
    if (Arg_count > 2 && 
	(is_objtype(Args[1]) || strCcmp("OBJECT", Args[1]) == 0)) {
	strncpy(Objtype, Args[1], MAX_OBJNAME);
	for (p = &Objtype[0]; *p; p++)
	    if (islower(*p))
		*p = toupper(*p);
	free(Args[1]);
	for (i = 1; Args[i] = Args[i+1]; i++)
	    ;
	Arg_count--;
    }
    if (Arg_count >= 3 && strCcmp(Args[1], "to") == 0) {
	free(Args[1]);
	if (ar_ctl(ar_get_current(), CTGETARG, &Args[1]) == FAIL) {
	    glob_mess_nosrc(t);
	    return(FAIL);
	}
    }

    p = Args[1];
    Args[1] = path_to_full(Args[1]);
    if (p)
	free(p);
    return SUCCESS;
}

static token
gotoarg(s, t)
char *s;
token t;
{
    struct actrec *a;

    if (t == TOK_CANCEL)
	return t;
    if (s) {
	if (Args[1] && Arg_count > 1)
	    free(Args[1]);
	Args[1] = strsave(s);
	if (Arg_count < 2)
	    Arg_count = 2;
    }
    if ((a = window_arg(Arg_count - 1, Args + 1, 1)) == NULL) {

	(void)mess_err( gettxt(":317","Unable to find a frame with that name") ); 
        /* abs s15 */

	return t;
    }
    else
	ar_current(a, TRUE);	/* abs k15 */
    return TOK_NOP;
}

/* parse for arguments of the form: command [name] [in folder] */

int
parse_n_in_fold(name, folder)
char **name, **folder;
{
    struct actrec *a;

    switch (Arg_count) {
    case 0:
    case 1:
	*name = NULL;
	*folder = cur_path();
	break;
    case 2:
	*name = Args[1];
	*folder = cur_path();
	break;
    default:
	(void)mess_temp(Extra_args); 		/* abs s16 */
	sleep(2);
    case 4:
	if (strCcmp(Args[2], "in") != 0) {
	    (void)mess_temp(Extra_args);		/* abs s16 */
	    return(FAIL);
	}
	*folder = path_to_full(Args[3]);
	*name = Args[1];
	break;
    case 3:
	*folder = path_to_full(Args[2]);
	if (strCcmp(Args[1], "in") == 0)
	    *name = NULL;
	else
	    *name = Args[1];
	break;
    }
    return SUCCESS;
}

static char *
cur_path()
{
    bool arg;
    char *path;
    extern char *Filecabinet;

    if (ar_ctl(ar_get_current(), CTISDEST, &arg) == FAIL || arg == FALSE)
	path = Filecabinet;
    else
	path = ar_get_current()->path;
    return(path);
}
