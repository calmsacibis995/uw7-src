/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:oh/cmd.c	1.49.3.6"

#include <stdio.h>
#include <string.h>
#include <curses.h>		/* abs s16 */
#include <unistd.h>	/* for declaration of gettxt() */
#include "wish.h"
#include "token.h"
#include "slk.h"
#include "actrec.h"
#include "terror.h"
#include "ctl.h"
#include "menudefs.h"
#include "vtdefs.h"
#include "fm_mn_par.h"
#include "moremacros.h"
#include "eval.h"
#include "interrupt.h"
#include "sizes.h"
#include "helptext.h"
 

extern	menu_id menu_make();
extern char *Args[];
extern int Arg_count;

static struct cmdspec {
	char *name;
	char *label;	/* labels of cmd menu */
	token tok;
	int  helpindex;
	char *tokstr;
	char *helpaction;
	char *intr;
	char *onintr;
};

#define NOTEXT	((int) -1)
 
/*
 * NODISP is used for internal commands not to be displayed on the
 * command menu.
*/

#define NODISP	((int) -2)

/*
 * Table from which command defaults are selected
 */
static struct cmdspec Defaults[] = {
   {"cancel",	":45",	TOK_CLOSE,	HT_CANCEL,	NULL, NULL, NULL, NULL },
   {"cleanup",	":46",	TOK_CLEANUP,	HT_CLEANUP,	NULL, NULL, NULL, NULL },
   {"copy",	":47",	TOK_COPY,	NOTEXT,		NULL, NULL, NULL, NULL },
   {"create",	":48",	TOK_CREATE,	NOTEXT,		NULL, NULL, NULL, NULL },
   {"delete",	":49",	TOK_DELETE,	NOTEXT,		NULL, NULL, NULL, NULL },
   {"display",	":50",	TOK_DISPLAY,	NOTEXT,		NULL, NULL, NULL, NULL },
   {"exit",	":51",	TOK_LOGOUT,	HT_EXIT,	NULL, NULL, NULL, NULL },
   {"find",	":52",	TOK_FIND,	NOTEXT,		NULL, NULL, NULL, NULL },
   {"frm-mgmt",	":53",	TOK_WDWMGMT,	HT_FRM_MGMT,	NULL, NULL, NULL, NULL },
   {"goto",	":54",	TOK_GOTO,	HT_GOTO,	NULL, NULL, NULL, NULL },
   {"help",	":55",	TOK_HELP,	HT_HELP,	NULL, NULL, NULL, NULL },
   {"move",	":56",	TOK_MOVE,	NOTEXT,		NULL, NULL, NULL, NULL },
   {"next-frm",	":57",	TOK_NEXT_WDW,	HT_NEXT_FRM,	NULL, NULL, NULL, NULL },
   {"organize",	":58",	TOK_ORGANIZE,	NOTEXT,		NULL, NULL, NULL, NULL },
   {"prev-frm",	":59",	TOK_PREV_WDW,	HT_PREV_FRM,	NULL, NULL, NULL, NULL },
   {"print",	":60",	TOK_PRINT,	NOTEXT,		NULL, NULL, NULL, NULL }, 
   {"redescribe", ":61", TOK_SREPLACE,	NOTEXT,		NULL, NULL, NULL, NULL },
   {"refresh",	":62",	TOK_REFRESH,	HT_REFRESH,	NULL, NULL, NULL, NULL },
   {"rename",	":63",	TOK_REPLACE,	NOTEXT,		NULL, NULL, NULL, NULL },
   {"run",	":64",	TOK_RUN,	NOTEXT,		NULL, NULL, NULL, NULL },
   {"security",	":65",	TOK_SECURITY,	NOTEXT,		NULL, NULL, NULL, NULL },
   {"show-path",":66",	TOK_SHOW_PATH,  NOTEXT,		NULL, NULL, NULL, NULL },
   {"time",	":67",	TOK_TIME,	NOTEXT,		NULL, NULL, NULL, NULL },
   {"undelete", ":68",	TOK_UNDELETE,	NOTEXT,		NULL, NULL, NULL, NULL },
   {"unix-system",":69", TOK_UNIX,	HT_UNIX,	NULL, NULL, NULL, NULL },
   {"update",	":70",	TOK_REREAD,	HT_UPDATE,	NULL, NULL, NULL, NULL },
   {"unix",	":71",	TOK_UNIX,	NODISP,		NULL, NULL, NULL, NULL },
   {NULL, 	NULL,	TOK_NOP,	NOTEXT,		NULL, NULL, NULL, NULL }
};

/*
 * Commands with NODISP have to be last in the above table befor the
 * NULL command.
*/

#define MAX_CMD	64

/*
 * Command table, presented to the user via the command menu.
 * This table, once initialized, is kept in alphabetical order.
 */
static struct cmdspec Commands[MAX_CMD];

/*
 * Commands that the user doesn't see in the cmd menu, but exist
 * none-the-less, (most are used for token translations from within
 * the FMLI language).
 */
static struct cmdspec Interncmd[] = {
	{"badchar",	NULL,	TOK_BADCHAR,	NOTEXT,  NULL, NULL, NULL, NULL },
	{"choices",	NULL,	TOK_OPTIONS,	NOTEXT,  NULL, NULL, NULL, NULL },
	{"checkworld",	NULL,	TOK_CHECKWORLD,	NOTEXT,  NULL, NULL, NULL, NULL },
	{"close",	NULL,	TOK_CLOSE,	NOTEXT,  NULL, NULL, NULL, NULL },
	{"cmd-menu",	NULL,	TOK_CMD,	NOTEXT,	 NULL, NULL, NULL, NULL },
	{"done",	NULL,	TOK_DONE,	NOTEXT,  NULL, NULL, NULL, NULL },
	{"enter",	NULL,	TOK_RETURN,	NOTEXT,  NULL, NULL, NULL, NULL },
	{"exit_now",	NULL,	TOK_LOGOUT,	NOTEXT,  NULL, NULL, NULL, NULL },
	{"mark",	NULL,	TOK_MARK,	NOTEXT,  NULL, NULL, NULL, NULL },
	{"nextpage",	NULL,	TOK_NPAGE,	NOTEXT,  NULL, NULL, NULL, NULL },
	{"nop",		NULL,	TOK_NOP,	NOTEXT,  NULL, NULL, NULL, NULL },
	{"nunique", 	NULL,	TOK_NUNIQUE,	NOTEXT,  NULL, NULL, NULL, NULL },
	{"objop",	NULL,	TOK_OBJOP,	NOTEXT,  NULL, NULL, NULL, NULL },
	{"open",	NULL,	TOK_OPEN,	NOTEXT,  NULL, NULL, NULL, NULL },	
	{"prevpage",	NULL,	TOK_PPAGE,	NOTEXT,  NULL, NULL, NULL, NULL },
	{"release",	NULL,	TOK_RELEASE,	NOTEXT,  NULL, NULL, NULL, NULL },
	{"reset",	NULL,	TOK_RESET,	NOTEXT,  NULL, NULL, NULL, NULL },
	{"run",		NULL,	TOK_RUN,	NOTEXT,  NULL, NULL, NULL, NULL },
/*s15*/ {"textframe",	NULL,	TOK_TEXTFRAME,	NOTEXT,  NULL, NULL, NULL, NULL },
	{"togslk",	NULL,	TOK_TOGSLK,	NOTEXT,  NULL, NULL, NULL, NULL },

	/* Secret commands, they wouldn't let us document them... */
	/*{"?",		NULL,	TOK_REDO,	NOTEXT,  NULL, NULL, NULL, NULL },*/
	/*{"%",		NULL,	TOK_DEBUG,	NOTEXT,  NULL, NULL, NULL, NULL },*/
	{"=",		NULL,	TOK_SET,	NOTEXT,  NULL, NULL, NULL, NULL },

	{NULL,		NULL,	TOK_NOP,	NOTEXT,  NULL, NULL, NULL, NULL }
};

static int Numdefaults = sizeof(Defaults)/sizeof(struct cmdspec);
static int Numcommands = sizeof(Commands)/sizeof(struct cmdspec);
static struct actrec *Cmd_ar;
static char *Tokstr;
static int Cmd_index;

extern int Vflag;
extern char *init_ctl();	/* in if_init.c */


/***********************************************************************
* cmdcmp (c1, c2)
* compares to cmdspec entries corresponding to their label.
************************************************************************
*/

static int
cmdcmp (c1, c2)
struct cmdspec *c1, *c2;
{
   int i;
   i=strcoll (c1->label, c2->label);
   if ( i == 0) {
      i=strcmp(c1->label,c2->label);
      return(i);
   }
   else
   return(i);
}

cmd_table_init()
{
	register int i, j;

	for (i = 0, j = 0; i < Numdefaults; i++) {
		if (Vflag || Defaults[i].helpindex != NOTEXT)
		  {
		    /* 
		    ** translate the command label if there is a
		    ** message identifier in the label field 
		    */
		    if ( Defaults[i].label != NULL )
			Defaults[i].label = gettxt (Defaults[i].label,
					            Defaults[i].name);
		    else
			Defaults[i].label = Defaults[i].name;
		    Commands[j++] = Defaults[i];
		  }
	}
	Commands[j].name = NULL;

	/* 
	** now sort the Commands by their label fields
	*/
	qsort (Commands, j-2, sizeof(struct cmdspec), cmdcmp);
}

static struct menu_line
cmd_disp(n, ptr)
int n;
char *ptr;
{
	struct menu_line m;

	m.description = NULL;
	m.flags = 0;
 
/* Commands marked as NODISP do not go on the command menu */

	if (n >= Numcommands  || Commands[n].helpindex == NODISP)
		m.highlight = NULL;
	else
	  {
	    /* 
	    ** return the translated command label if it exists
	    ** else return the command name in m.highlight.
	    */
	    if ( Commands[n].label )
		m.highlight = Commands[n].label;
	    else
		m.highlight = Commands[n].name;
	  }
	return m;
}

static int
cmd_odsh(a, t)
struct actrec *a;
token t;
{
	extern int Arg_count;
	char **actstr, **eval_string();
	token tok, make_action();
	int flags;
	char *intr, *onintr;
	t = menu_stream(t);
	if (t == TOK_OPEN && Arg_count <= 1) {
		int line;

		(void) menu_ctl(a->id, CTGETPOS, &line);
		if (Commands[line].tok >= 0)		/* internal */
			tok = Commands[line].tok;
		else {
		    /* 	update the interrupt structures based on 
			the values for the current command, if
			defined else with the inherited values.
        	    */
		    Cur_intr.skip_eval =  FALSE;
		    if ((intr = Commands[line].intr) == NULL)
			intr = init_ctl(CTGETINTR);
		    flags = RET_BOOL;
		    Cur_intr.interrupt = FALSE;	/* dont intrupt eval of intr */
		    Cur_intr.interrupt = (bool)eval_string(intr, &flags);

		    if ((onintr = Commands[line].onintr) == NULL)
			onintr = init_ctl(CTGETONINTR);
		    Cur_intr.oninterrupt = onintr;

		    flags = RET_ARGS;
		    actstr = eval_string(Commands[line].tokstr, &flags);
		    tok = make_action(actstr);
		}
		t = arf_odsh(a->backup, tok);
		(void) ar_close(a, FALSE);  /* Command execution causes close */
	}
	else if (t == TOK_NEXT)
		t = TOK_NOP;		/* eat it up */
	else if (t == TOK_CANCEL) {
		ar_backup();
		t = TOK_NOP;
	}
	return t;
}

static int
cmd_close(a)
struct actrec *a;
{
    Cmd_ar = NULL;
    return(AR_MEN_CLOSE(a));
}

cmd_help(cmd)
char *cmd;
{
    char help[PATHSIZ];
    int flags;
    char **helpaction, **eval_string();
    token tok, make_action(), generic_help();
    extern char *Filesys;
    char *cur_cmd(), *tok_to_cmd();
    struct cmdspec *command, *get_cmd();
    extern int Vflag;
    static char *help_errmess = NULL;
    char *label;
    int i;

    if ( help_errmess == NULL )
	help_errmess = gettxt (":72", "Could not find help on that command");
    if (cmd && *cmd)
    {
/*      below cannot destinguish between user defined cmds.  abs k17
**	cmd = tok_to_cmd(cmd_to_tok(cmd));
*/
	if (cmd_to_tok(cmd) == TOK_NUNIQUE) 			/* abs k17 */
	{
	    (void)mess_err (help_errmess); /* abs s15; abs k17 */
	    return(SUCCESS);					/* abs k17 */
	}

    }
    else
	cmd = cur_cmd();
    if (!cmd || ((command = get_cmd(cmd)) == NULL)) {
	(void)mess_err (help_errmess); /* abs s15 */
	return(SUCCESS);
    }
	
    /*
     * If there is a help action defined then do it ...
     * else if there is a "hardcoded" help string use that
     * else if FACE is running use the FACE help files
     * else there is no help available ....
     */
    if (command->helpaction && command->helpaction[0] != '\0') {
	flags = RET_ARGS;
	helpaction = eval_string(command->helpaction, &flags);
	tok = make_action(helpaction);
	return(tok);
    }
    /*
    ** get the language dependend label of this command
    */
    label = cmd;
    for (i = 0; Commands[i].name; i++) 
      {
        if ( strcmp(cmd, Commands[i].name) == 0 ) 
	  {
	    label = Commands[i].label;
	    break;
	  }
      }

    if (command->helpindex >= 0)                    /* abs k18 */
	return(generic_help(label, command->helpindex));
    else if (Vflag) {		/* FACE has its own help file setup */
	sprintf(help, "%s/OBJECTS/Text.help", Filesys);
	objop("OPEN", "TEXT", help, cmd, cmd, label, NULL);
	return(SUCCESS);
    }
    else
	(void)mess_err (help_errmess); /* abs s15 */
    return SUCCESS;
}

/* dmd TCB
extern char *Help_text[];   moved to header file helptext.h
*/

/* dmd TCB */
static char *Help_args[3] = {
	"OPEN",
	"TEXT",
	"-i"
};

token
generic_help(name, helpindex)
char *name;
int  helpindex;
{
	extern char	*Args[];
	extern int	Arg_count;
	extern int	Vflag;
	register IOSTRUCT *out;
	char buf[512];
	static char *msg1 = NULL;
	static char *msg2 = NULL;
        
        char *i18n_string;

	out = io_open(EV_USE_STRING, NULL);
	/* 
	** natinalisation of the title
	** putastr("title=Help Facility: \"", out);
	** break this one call of putastr into three calls.
	*/
	if ( msg1 == NULL )
	    msg1 = gettxt(":73", "Help Facility: \"%s\"");
	sprintf (buf, msg1, name);
	putastr ("title=", out);
	putastr (buf, out);
	putac ('\n', out);
	/* end of I18n */
#ifdef notdef
	putastr (" \"", out);
	putastr(name, out);
	putastr("\"\n", out);
#endif
	putastr("lifetime=shortterm\n", out); /* was longterm abs k18 */
	putastr("rows=12\n", out);
	putastr("columns=72\n", out);
	putastr("begrow=distinct\n", out);
	putastr("begcol=distinct\n", out);
	putastr("text=\"", out);

        i18n_string=gettxt(Help_textid[helpindex],Help_text[helpindex]);
	putastr(i18n_string, out);

	putastr("\"\n", out);
	if (Vflag) {
		/*
		** internationalisation of the title
		** putastr("name=\"CONTENTS\"\n",out);
		** break this one call of putastr into three calls.
		*/
		if ( msg2 == NULL )
		    msg2 = gettxt (":74", "CONTENTS");
		putastr("name=\"", out);
		putastr (msg2, out);
		putastr ("\"\n", out);
		/* end of I18n */
		putastr("button=8\n",out);
		putastr("action=OPEN MENU OBJECTS/Menu.h0.toc\n",out);
	}
	for (Arg_count = 0; Arg_count < 3; Arg_count++)
	{
		if ( Args[Arg_count])
			free( Args[Arg_count]); /* les 12/4 */
		Args[Arg_count] = strsave(Help_args[Arg_count]);
	}
	if ( Args[Arg_count])
		free( Args[Arg_count]); /* les 12/4 */
	Args[Arg_count++] = (char *) io_string(out);
	if ( Args[Arg_count])
		free( Args[Arg_count]); /* les 12/4 */
	Args[Arg_count] = NULL;
	io_close(out);
	return(TOK_OPEN);
}

struct actrec *
cmd_create()
{
	struct actrec a;
	vt_id vid;
	struct actrec *ar_create(), *ar_current();

	for (Numcommands = 0; Commands[Numcommands].name; Numcommands++)
		;

	if (Numcommands == 0) {
		(void)mess_err(gettxt (":75", "There are no commands in the command menu")); /* abs s15 */
		return(NULL);
	}

	/* 
	** internationalisation of the title of the cmd-menu
	*/
	a.id = (int) menu_make(-1, gettxt (":76", "Command Menu"),
			VT_NONUMBER | VT_CENTER, 
			VT_UNDEFINED, VT_UNDEFINED, 0, 0, cmd_disp, NULL);

	ar_menu_init(&a);
	a.fcntbl[AR_CLOSE] = cmd_close;
	a.fcntbl[AR_ODSH] = cmd_odsh;
	a.fcntbl[AR_HELP] = cmd_help;
	a.flags = 0;

	/* theres no  frame level interrupt or oninterrupt  descriptors.. */
	/* .. so set up values in the  actrec now since they'll only ..   */
	/* .. change on a re-init. */
	ar_ctl(&a, CTSETINTR, init_ctl(CTGETINTR));
	ar_ctl(&a, CTSETONINTR, init_ctl(CTGETONINTR));

	Cmd_ar = ar_create(&a);
	return(ar_current(Cmd_ar, FALSE)); /* abs k15 */
}

token
_cmd_to_tok(cmd, partial, slk)
char *cmd;
bool partial;
bool slk;
{
    register int i;
    register int size;
    register int cmdnumatch = 0, slknumatch = 0;	/* number of matches */
    register int cmdmatch= -1, slkmatch = -1;		/* index of last match */
    extern struct slk SLK_array[MAX_SLK];
    int strnCcmp(), strCcmp();
    
    Tokstr = NULL;
    Cmd_index = -1;
    if (!cmd)		/* no input (^j <return>) */
	return(TOK_CANCEL);
    size = strlen(cmd);
    if (slk) {
	for (i = 0; i < MAX_SLK; i++) {
	    if ((partial ? strnCcmp : strCcmp)(SLK_array[i].label, cmd, size) == 0) {
		/*
		 * If there is another match BUT ...
		 *    the command token is the same
		 *    OR the name strings match exactly
		 * then ignore the 'ith' SLK 
		 */
		if (slknumatch == 1 &&
		    (SLK_array[i].tok == SLK_array[slkmatch].tok ||
		     strCcmp(SLK_array[slkmatch].label, SLK_array[i].label) == 0))
		    continue;
		slknumatch++;
		slkmatch = i;
	    }
	}
    }
    
    for (i = 0; i < Numcommands; i++) {
	if ((partial ? strnCcmp : strCcmp)(Commands[i].name, cmd, size) == 0) {
	    /*
	     * if there is an exact match then break
	     */
	    if (partial && strCcmp(Commands[i].name, cmd) == 0) { 
		cmdmatch = i;
		cmdnumatch = 1;
		break;
	    }
	    cmdnumatch++;
	    cmdmatch = i;
	}
    }
/* since "unix" is unadvertised, don't get confused by 2 partial matches 
 * for unix and unix-system. mek k17
 */
    if ((slknumatch == 0) && (cmdnumatch == 2) && 
	(strcmp(Commands[cmdmatch].name, "unix") == 0))
	    return(Commands[cmdmatch].tok);
    
    if (slknumatch + cmdnumatch == 0) {
	/*
	 * no matches, check internal command table 
	 */
	for (i = 0; Interncmd[i].name; i++)
	    if (strCcmp(Interncmd[i].name, cmd) == 0)
		return(Interncmd[i].tok);
	return(TOK_NOP);
    }
    else if (slknumatch > 1 || cmdnumatch > 1)	/* input not unique */
	return(TOK_NUNIQUE);
    else if (slknumatch == 1 && cmdnumatch == 0) {	/* matched slk only */
	Tokstr = SLK_array[slkmatch].tokstr;
	return(SLK_array[slkmatch].tok);
    }
    else if (cmdnumatch == 1 && slknumatch == 0) {  /* matched cmd only */
	Tokstr = Commands[cmdmatch].tokstr;
	Cmd_index = cmdmatch;
	return(Commands[cmdmatch].tok);
    }
    else {
	/*
	 * If there is only ONE match in both the
	 * SLKS and the Command Menu then
	 *  - the SLK takes precedence if both match exactly 
	 *  - match is not unique if both match "partially"
	 */
	if (strCcmp(SLK_array[slkmatch].label, Commands[cmdmatch].name) == 0) {
	    Tokstr = SLK_array[slkmatch].tokstr;
	    return(SLK_array[slkmatch].tok);
	}
	else
	    return(TOK_NUNIQUE);
    }	
}

/* LES: replace with MACRO's

token
cmd_to_tok(cmd)
char *cmd;
{
	return(_cmd_to_tok(cmd, TRUE, TRUE));
}

		NEVER CALLED
token
fullcmd_to_tok(cmd)
char *cmd;
{
	return(_cmd_to_tok(cmd, FALSE, TRUE));
}

token
mencmd_to_tok(cmd)
char *cmd;
{
	return(_cmd_to_tok(cmd, FALSE, FALSE));
}
*/

char *
tok_to_cmd(tok)
token tok;
{
	register int i;
	extern struct slk SLK_array[];

	/*  Most frequently referenced command is open, make it QUICK !!! */
	if (tok == TOK_OPEN)
		return("open");

	for (i = 0; i < Numcommands; i++)
		if (Commands[i].tok == tok)
			return Commands[i].name;
	for (i = 0; SLK_array[i].label; i++)
		if (SLK_array[i].tok == tok)
			return SLK_array[i].label;
	for (i = 0; Interncmd[i].name; i++)
		if (Interncmd[i].tok == tok)
			return Interncmd[i].name;
	return NULL;
}

char *
cur_cmd()
{
	int	line;
	/* char *cur_hist(); */

	if (ar_get_current() != Cmd_ar)
		return(NULL);
	menu_ctl(Cmd_ar->id, CTGETPOS, &line);
	return Commands[line].name;
}

/*
 * ADD_CMD will add a command to the command list preserving
 * alphabetical ordering
 */
add_cmd(name, tokstr, help, intr, onintr)
char *name;
char *tokstr;
char *help;
char *intr;
char *onintr;
{
	register int ii, i, j, comp;

	extern int i18n_local_strings;

	for (i = 0; Interncmd[i].name; i++) {
		if (strcmp(Interncmd[i].name, name) == 0)
			return;		/* internal command conflict */
	}

        if ( Commands[MAX_CMD -1].name )
            return; /* Comannd menu is full. Return or report error */
                    /* Function calling this routine does not check ret code */

	/***************************************************************
	* First we need to compare the new command name with all
	* names in the Commands array. For collation of the commands
	* we use the label fields.
	****************************************************************
	*/

	for (i = 0; Commands[i].name; i++) 
	  {
	    if ( strcmp(name, Commands[i].name) == 0 ) 
	      {
		/*
		 * Command already exists
		 */
		if (Commands[i].tok >= 0) 
		  {
			/*
			 * Name conflict with a generic command,
			 * only accept redefinitions for helpaction
			 */
			if (help && (*help != '\0')) 
			  {
				Commands[i].helpindex = NOTEXT;
				Commands[i].helpaction= strsave(help);
			  }
		  }
		else 
		  {
			/*
			 * Redefine a previous definition
			 */
			Commands[i].name = strsave(name);
			Commands[i].label = Commands[i].name;
			Commands[i].tok = -1;	/* no token */ 
			Commands[i].helpindex = NOTEXT;
			Commands[i].tokstr = strsave(tokstr);
			Commands[i].helpaction = strsave(help);
			Commands[i].intr = strsave(intr);
			Commands[i].onintr = strsave(onintr);
		  }
		return;
	      } /* END OF IF (strcmp () == 0) */
	  } /* END OF FOR LOOP */

	for (i = 0; Commands[i].name; i++) {
                ii = i;

	    /*	comp = strcmp(name, Commands[i].name);  I18n of collation */
		/* next line */

		comp = strcoll (name, Commands[i].label);
                if (comp == 0)
                   comp = strcmp (name, Commands[i].label);
		if (comp < 0) {
			/*
			 * shift list to make room for new entry
			 */
                        ii = 0;
			for (j = MAX_CMD - 1; j > i; j--)
				Commands[j] = Commands[j - 1];

			Commands[i].name = strsave(name);
			/*
			** for added commands the label is the same
			** as the name.
			*/
			Commands[i].label = Commands[i].name;
			Commands[i].tok = -1;	/* no token */ 
			Commands[i].helpindex = NOTEXT; 
			Commands[i].tokstr = strsave(tokstr);
			Commands[i].helpaction = strsave(help);
			Commands[i].intr = strsave(intr);
			Commands[i].onintr = strsave(onintr);
			return;
		} /* END OF IF (comp < 0) */
		else if (comp == 0 && i18n_local_strings) {
			/*
			 * Command already exists
			 */
                        ii=0;
			if (Commands[i].tok >= 0) {
				/*
				 * Name conflict with a generic command,
				 * only accept redefinitions for helpaction
				 */
				if (help && (*help != '\0')) {
					Commands[i].helpindex = NOTEXT;
					Commands[i].helpaction= strsave(help);
				}
			}
			else {
				/*
				 * Redefine a previous definition
				 */
				Commands[i].name = strsave(name);
				Commands[i].label = Commands[i].name;
				Commands[i].tok = -1;	/* no token */ 
				Commands[i].helpindex = NOTEXT;
				Commands[i].tokstr = strsave(tokstr);
				Commands[i].helpaction = strsave(help);
				Commands[i].intr = strsave(intr);
				Commands[i].onintr = strsave(onintr);
			}
			return;
		} /* END OF ELSE IF (comp == 0) */
	} /* END OF FOR LOOP */

    /* 
    ** append command at the end of the Commands array
    ** but before (!) the "unix" item.
    */
    if ( ii > 0 )
      {
	Commands[ii+1] = Commands[ii];       /* Move the NODISP unix command */
					     /* ahead one place              */
/*
	Commands[ii] = Commands[ii-1];       
	--ii;
*/
	Commands[ii].name = strsave(name);
	Commands[ii].label = Commands[ii].name;
	Commands[ii].tok = -1;	/* no token */ 
	Commands[ii].helpindex = NOTEXT; 
	Commands[ii].tokstr = strsave(tokstr);
	Commands[ii].helpaction = strsave(help);
	Commands[ii].intr = strsave(intr);
	Commands[ii].onintr = strsave(onintr);
      }
    return;
}


/*
 * DEL_CMD will remove a command from the command menu
 * (shifting the command menu accordingly)
 */
del_cmd(name)
char *name;
{
	register int i, j, comp;
	extern int i18n_local_strings; 

	for (i = 0; Commands[i].name; i++) {	/* if not end of list */
		/* 
		** do also compare with the label if the application
		** uses NLS strings in its description files
		*/
		if ((comp = strcmp(name, Commands[i].name)) == 0 ||
		    (i18n_local_strings && strcmp(name, Commands[i].label) == 0)) 
		{
			/*
			 * scrunch list to remove entry
			 */
			for (j = i; j < MAX_CMD - 1; j++)
				Commands[j] = Commands[j + 1];
			break;
		}
	}
}

static struct cmdspec *
get_cmd(cmdstr)
char *cmdstr;
{
	register int i;

	for (i = 0; i < Numcommands && Commands[i].name; i++)
		if (strcmp(Commands[i].name, cmdstr) == 0)
			return(&(Commands[i]));
	return(NULL);
}
	
token
do_app_cmd()
{
	char **strlist, **eval_string();
	token t, make_action();
	int flags;
	char *intr, *onintr;

	if (Tokstr) 		  /* set in _cmd_to_tok */
	{
	    if (Cmd_index  >= 0)  /* set in _cmd_to_tok */
	    {
		/* 	update the interrupt structures based on 
			the values for the current command, if
			defined else with the inherited values.
        	*/
		Cur_intr.skip_eval =  FALSE;
		if ((intr = Commands[Cmd_index].intr) == NULL)
		    intr = init_ctl(CTGETINTR);
		flags = RET_BOOL;
		Cur_intr.interrupt = FALSE;	/* dont intrupt eval of intr */
		Cur_intr.interrupt = (bool)eval_string(intr, &flags);
		
		if ((onintr = Commands[Cmd_index].onintr) == NULL)
		    onintr = init_ctl(CTGETONINTR);
		Cur_intr.oninterrupt = onintr;
	    }
	    flags = RET_ARGS; 
	    strlist = eval_string(Tokstr, &flags);
	    t = make_action(strlist);
	}
	else
	    t = TOK_NOP;
}



cmd_reinit(argc, argv, instring, outstring, errstring) 		/* abs s19 */
int argc;
char *argv[];
IOSTRUCT *instring;						/* abs s19 */
IOSTRUCT *outstring;						/* abs s19 */
IOSTRUCT *errstring;						/* abs s19 */
{
	if (argv[1] && (*argv[1] != '\0') && (access(argv[1], 2) == 0))
	{
		read_inits(argv[1]);
		init_sfk(FALSE);  /* download PFK's for terms like 630. k17 */
		set_def_colors(); /* moved above next line. k17 */
		set_def_status();
		ar_ctl(Cmd_ar, CTSETINTR, init_ctl(CTGETINTR));
		ar_ctl(Cmd_ar, CTSETONINTR, init_ctl(CTGETONINTR));
		return(SUCCESS);
	}
	else
		return(FAIL);
}


	
cmd_textframe()
{
    extern char *optarg;
    extern int optind, opterr, optopt;
    extern int Arg_count;
    extern char *Args[];

    IOSTRUCT *script;
    int   option;
    int   life_flag = 0;
    char *ptr;
    int   num;			/* abs s16 */

    script = io_open(EV_USE_STRING, NULL);
    optind = 1;
    opterr = 0;

    while ((option = getopt(Arg_count, Args, "ac:f:l:p:r:t:")) != EOF)
	switch(option) {
	    case 'a':
	        
	        putastr("altslks=true\n", script);
		break;

	    case 'c':

		/* make sure the arg is an integer */
		num = strtol(optarg, &ptr, 10);			/* abs s16 */
		if (ptr == optarg || *ptr != EOS )
		{		    
		    mess_err(gettxt (":77", "textframe: non-numeric columns ignored. "));
		    break;					/* abs s17 */
		}
		else						/* abs s16 */
                    /* -3 => 2 for borders + 1 space on right. */
		    if (num <= 0 || num > (COLS  - 3)) 		/* abs s17 */
		    {
			mess_err(gettxt(":78", "textframe: out-of-range columns ignored. "));
			break;					/* abs s17 */
		    }
		putastr("columns=", script);
		putastr(optarg, script);
		putac('\n', script);
		break;

	    case 'f':

		putastr("framemsg=", script);
		putastr(optarg, script);
		putac('\n', script);
		break;

	    case 'l':

		if (strCcmp(optarg, "longterm")  == 0 ||
		    strCcmp(optarg, "shortterm") == 0 ||
		    strCcmp(optarg, "permanent") == 0 ||
		    strCcmp(optarg, "immortal")  == 0 )
		    
		{
		    life_flag++;
		    putastr("lifetime=", script);
		    putastr(optarg, script);
		    putac('\n', script);
		}
		else
		    mess_err(gettxt(":79", "textframe: invalid lifetime ignored. "));
		break;

	    case 'p':

		
		if (strCcmp(optarg, "center")  == 0 ||
		    strCcmp(optarg, "current") == 0)
		{
		    putastr("begrow=", script);
		    putastr(optarg, script);
		    putac('\n', script);
		}
		else
		    mess_err(gettxt (":80", "textframe: invalid position ignored. "));
		break;

	    case 'r':

		/* make sure the arg is an integer */
		num = strtol(optarg, &ptr, 10);			/* abs s16 */
		if (ptr == optarg || *ptr != EOS )
		{
		    mess_err(gettxt(":81", "textframe: non-numeric rows ignored. "));
		    break;					/* abs s17 */
		}
		else						/* abs s16 */
		    /* -2 for the frame border.  abs s17 */
		    if (num <= 0 || num > (LINES - RESERVED_LINES -2))
		    {
			mess_err(gettxt(":82", "textframe: out-of-range rows ignored. "));
			break;					/* abs s17 */
		    }
		putastr("rows=", script);
		putastr(optarg, script);
		putac('\n', script);
		break;

	    case 't':

		putastr("title=", script);
		putastr(optarg, script);
		putac('\n', script);
		break;

	    default:

		mess_err(gettxt (":83", "textframe: illegal option ignored. "));
		break;
	    }

    if (life_flag == 0)
	putastr("lifetime=shortterm\n", script);

    if (optind >= Arg_count)	         /* oops, no args left, i.e. no text */
    {
	mess_err(gettxt (":84", "textframe: no text specified. "));
	putastr("text=\n", script);
    }
    else if (Arg_count > optind + 1)     /* oops, more than one arg left */
    {
	mess_err(gettxt (":85", "textframe: warning: extra arguments ignored. "));
	putastr("text=\"", script); 
	putastr(Args[optind], script);	 /* use 1st remaining arg as text */
	putastr("\"\n", script);
    }
    else                                 /* one arg left (good) */
    {
	putastr("text=\"", script);
	putastr(Args[optind], script);
	putastr("\"\n", script);
    }
    for (Arg_count = 4; Arg_count >= 0; Arg_count--)
    {
	if (Args[Arg_count])
	    free(Args[Arg_count]);
    }
	
    Args[0] = strsave("OPEN");
    Args[1] = strsave("TEXT");
    Args[2] = strsave("-i");
    Args[3] = io_string(script);
    Args[4] = NULL;
    Arg_count = 4;
    io_close(script);
    
    return;
}
