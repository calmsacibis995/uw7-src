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
      other.c

      This implements the "setup" screen of miscellaneous commands such
  as keyboard lock, and disk usage

  ====*/

#include "headers.h"

#define	BODY_LINES(X)	((X)->ttyo->screen_rows -HEADER_ROWS(X)-FOOTER_ROWS(X))

#define	CONFIG_SCREEN_TITLE		"SETUP CONFIGURATION"
#define	CONFIG_SCREEN_HELP_TITLE	"HELP FOR SETUP CONFIGURATION"
#define	R_SELD				'*'
#define	EXIT_PMT "Commit changes (\"Yes\" replaces settings, \"No\" abandons changes)"
static char *empty_val  = "Empty Value";
static char *empty_val2 = "<Empty Value>";
#define EMPTY_VAL_LEN     11
static char *no_val     = "No Value Set";
#define NO_VAL_LEN        12
static char *fixed_val  = "Value is Fixed";


typedef struct conf_line {
    char	     *varname,			/* alloc'd var name string   */
		     *value;			/* alloc'd var value string  */
    short	      varoffset;		/* offset from screen left   */
    short	      valoffset;		/* offset from screen left   */
    struct variable   *var;			/* pointer to pinerc var     */
    short	      varmem;			/* value's index, if list    */
    int		      (*tool)();		/* tool to manipulate values */
    struct key_menu  *keymenu;			/* tool-specific  keymenu    */
    HelpType	      help;			/* variable's help text      */
    unsigned          flags;
    void	     *scrap;
    struct conf_line *varnamep;		/* pointer to varname        */
    struct conf_line *headingp;		/* pointer to heading        */
    struct conf_line *next, *prev;
} CONF_S;

/*
 * Valid for flags argument of config screen tools or flags field in CONF_S
 */
#define	CF_CHANGES	0x01	/* Have been earlier changes */
#define	CF_NOSELECT	0x02	/* This line is unselectable */
#define	CF_NOHILITE	0x04	/* Don't highlight varname   */
#define	CF_NUMBER	0x08	/* Input should be numeric   */
#define	CF_INVISIBLEVAR	0x10	/* Don't show the varname    */

typedef struct save_config {
    union {
	char  *p;
	char **l;
	bitmap_t features;
    } user_val;
} SAVED_CONFIG_S;

/*
 *
 */
typedef struct conf_screen {
    CONF_S  *current,
	    *prev,
	    *top_line,
	    *first_line;
} OPT_SCREEN_S;

typedef enum {Config, PrintConfig, NoPrint} ConfigType;


static OPT_SCREEN_S *opt_screen;
static char **def_printer_line;
#if defined(DOS) || defined(OS2)
static char *config_colors[] = {"black", "blue", "green", "cyan", "red",
				"magenta", "yellow", "white", NULL};
#endif
static char no_ff[] = "-no-formfeed";

#define next_confline(p)  ((p) ? (p)->next : NULL)
#define prev_confline(p)  ((p) ? (p)->prev : NULL)

/*
 * Internal prototypes
 */
void	 draw_klocked_body PROTO((char *, char *));
void	 update_option_screen PROTO((struct pine *, OPT_SCREEN_S *, Pos *));
void	 print_option_screen PROTO((OPT_SCREEN_S *, char *));
void	 option_screen_redrawer PROTO(());
int	 conf_scroll_screen PROTO((struct pine *,CONF_S *,char *,ConfigType));
HelpType config_help PROTO((int, int));
int      text_tool PROTO((struct pine *, int, CONF_S **, unsigned));
int	 checkbox_tool PROTO((struct pine *, int, CONF_S **, unsigned));
int	 flag_checkbox_tool PROTO((struct pine *, int, CONF_S **, unsigned));
int	 radiobutton_tool PROTO((struct pine *, int, CONF_S **, unsigned));
int	 yesno_tool PROTO((struct pine *, int, CONF_S **, unsigned));
int	 print_select_tool PROTO((struct pine *, int, CONF_S **, unsigned));
int	 print_edit_tool PROTO((struct pine *, int, CONF_S **, unsigned));
void	 set_def_printer_value PROTO((char *));
int	 gripe_tool PROTO((struct pine *, int, CONF_S **, unsigned));
void	 toggle_feature_bit PROTO((struct pine *, int, struct variable *,
				    char *));
void	 config_add_list PROTO((struct pine *, CONF_S **, char **,
				char ***, int));
void	 config_del_list_item PROTO((CONF_S **, char ***));
char    *pretty_value PROTO((struct pine *, CONF_S *));
int      offer_to_fix_pinerc PROTO((struct pine *));
CONF_S	*new_confline PROTO((CONF_S **));
void	 free_confline PROTO((CONF_S **));
CONF_S	*first_confline PROTO((CONF_S *));
CONF_S  *first_sel_confline PROTO((CONF_S *));
CONF_S	*last_confline PROTO((CONF_S *));
int      flag_exit_cmd PROTO((unsigned));
int      config_exit_cmd PROTO((unsigned));
int	 screen_exit_cmd PROTO((unsigned, char *));
int	 config_scroll_up PROTO((long));
int	 config_scroll_down PROTO((long));
int	 config_scroll_to_pos PROTO((long));
CONF_S  *config_top_scroll PROTO((struct pine *, CONF_S *));
char	*printer_name PROTO ((char *));
#ifdef	_WINDOWS
int	 config_scroll_callback PROTO((int, long));
#endif
void     fix_side_effects PROTO ((struct pine *, struct variable *, int));
SAVED_CONFIG_S *save_config_vars PROTO((struct pine *));
void            revert_to_saved_config PROTO((struct pine *, SAVED_CONFIG_S *));
void            free_saved_config PROTO((struct pine *, SAVED_CONFIG_S **));
void	 att_cur_drawer PROTO((void));
char	*sigedit_exit_for_pico PROTO(());
int	 exclude_config_var PROTO((struct pine *, struct variable *));


static char *klockin, *klockame;

void
redraw_kl_body()
{
#ifndef NO_KEYBOARD_LOCK
    ClearScreen();

    set_titlebar("KEYBOARD LOCK", ps_global->mail_stream,
		 ps_global->context_current, ps_global->cur_folder, NULL,
		 1, FolderName, 0, 0);

    PutLine0(6,3 ,
       "You may lock this keyboard so that no one else can access your mail");
    PutLine0(8, 3 ,
       "while you are away.  The screen will be locked after entering the ");
    PutLine0(10, 3 ,
       "password to be used for unlocking the keyboard when you return.");
    fflush(stdout);
#endif
}


void
redraw_klocked_body()
{
#ifndef NO_KEYBOARD_LOCK
    ClearScreen();

    set_titlebar("KEYBOARD LOCK", ps_global->mail_stream,
		 ps_global->context_current, ps_global->cur_folder, NULL,
		 1, FolderName, 0, 0);

    PutLine2(6, 3, "This keyboard is locked by %s <%s>.",klockame, klockin);
    PutLine0(8, 3, "To unlock, enter password used to lock the keyboard.");
    fflush(stdout);
#endif
}


#ifndef NO_KEYBOARD_LOCK
/*----------------------------------------------------------------------
          Execute the lock keyboard command

    Args: None

  Result: keyboard is locked until user gives password
  ---*/

lock_keyboard()
{
    struct pine *ps = ps_global;
    char inpasswd[80], passwd[80], pw[80];
    HelpType help = NO_HELP;
    SigType (*hold_quit)();
    int i, times, old_suspend;

    passwd[0] = '\0';
    redraw_kl_body();
    ps->redrawer = redraw_kl_body;

    times = atoi(ps->VAR_KBLOCK_PASSWD_COUNT);
    if(times < 1 || times > 5){
	dprint(2, (debugfile,
	"Kblock-passwd-count var out of range (1 to 5) [%d]\n", times));
	times = 1;
    }

    inpasswd[0] = '\0';

    for(i = 0; i < times; i++){
	pw[0] = '\0';
	while(1){			/* input pasword to use for locking */
	    int rc;
	    char prompt[50];

	    sprintf(prompt,
		"%s password to LOCK keyboard %s: ",
		i ? "Retype" : "Enter",
		i > 1 ? "(Yes, again) " : "");

	    rc =  optionally_enter(pw, -FOOTER_ROWS(ps), 0, 30, 0, 1,
				    prompt, NULL, help, 0);

	    if(rc == 3)
	      help = help == NO_HELP ? h_kb_lock : NO_HELP;
	    else if(rc == 1 || pw[0] == '\0'){
		q_status_message(SM_ORDER, 0, 2, "Keyboard lock cancelled");
		return(-1);
	    }
	    else if(rc != 4)
	      break;
	}

	if(!inpasswd[0])
	  strcpy(inpasswd, pw);
	else if(strcmp(inpasswd, pw)){
	    q_status_message(SM_ORDER, 0, 2,
		"Mismatch with initial password: keyboard lock cancelled");
	    return(-1);
	}
    }

    if(want_to("Really lock keyboard with entered password", 'y', 'n',
	       NO_HELP, 0, 0) != 'y'){
	q_status_message(SM_ORDER, 0, 2, "Keyboard lock cancelled");
	return(-1);
    }

    draw_klocked_body(ps->VAR_USER_ID ? ps->VAR_USER_ID : "<no-user>",
		  ps->VAR_PERSONAL_NAME ? ps->VAR_PERSONAL_NAME : "<no-name>");

    ps->redrawer = redraw_klocked_body;
    if(old_suspend = F_ON(F_CAN_SUSPEND, ps_global))
      F_TURN_OFF(F_CAN_SUSPEND, ps_global);

    while(strcmp(inpasswd, passwd)){
	if(passwd[0])
	  q_status_message(SM_ORDER | SM_DING, 3, 3,
		     "Password to UNLOCK doesn't match password used to LOCK");
        
        help = NO_HELP;
        while(1){
	    int rc;
	    rc =  optionally_enter(passwd, -FOOTER_ROWS(ps), 0, 30, 0, 1, 
				   "Enter password to UNLOCK keyboard : ",NULL,
				   help, OE_DISALLOW_CANCEL);
	    if(rc == 3) {
		help = help == NO_HELP ? h_oe_keylock : NO_HELP;
		continue;
	    }

	    if(rc != 4)
	      break;
        }
    }

    if(old_suspend)
      F_TURN_ON(F_CAN_SUSPEND, ps_global);

    q_status_message(SM_ORDER, 0, 3, "Keyboard Unlocked");
    return(0);
}


void
draw_klocked_body(login, username)
    char *login, *username;
{
    klockin = login;
    klockame = username;
    redraw_klocked_body();
}
#endif /* !NO_KEYBOARD_LOCK */



/*----------------------------------------------------------------------
    Serve up the current signature within pico for editing

    Args: file to edit

  Result: signature changed or not.
  ---*/
void
signature_edit(sigfile)
    char *sigfile;
{
    int	     editor_result;
    char     sig_path[MAXPATH+1], *errstr = NULL;
    STORE_S *msgso, *tmpso = NULL;
    gf_io_t  gc, pc;
    PICO     pbuf;

    if(!signature_path(sigfile, sig_path, MAXPATH)){
        q_status_message(SM_ORDER, 3, 4, "No signature file defined.");
	return;
    }

    memset(&pbuf, 0, sizeof(PICO));

    pbuf.raw_io        = Raw;
    pbuf.showmsg       = display_message_for_pico;
    pbuf.newmail       = new_mail_for_pico;
    pbuf.ckptdir       = checkpoint_dir_for_pico;
    pbuf.exittest      = sigedit_exit_for_pico;
    pbuf.upload	       = (ps_global->VAR_UPLOAD_CMD
			  && ps_global->VAR_UPLOAD_CMD[0])
			   ? upload_msg_to_pico : NULL;
    pbuf.keybinit      = init_keyboard;
    pbuf.helper        = helper;
    pbuf.resize	       = resize_for_pico;
    pbuf.alt_ed        = (ps_global->VAR_EDITOR && ps_global->VAR_EDITOR[0])
			     ? ps_global->VAR_EDITOR : NULL;
    pbuf.alt_spell     = ps_global->VAR_SPELLER;
    pbuf.fillcolumn    = ps_global->composer_fillcol;
    pbuf.menu_rows     = FOOTER_ROWS(ps_global) - 1;
    pbuf.composer_help = h_composer_sigedit;
    pbuf.ins_help      = h_composer_ins;
    pbuf.search_help   = h_composer_search;
    pbuf.browse_help   = h_composer_browse;

    pbuf.pine_anchor   = set_titlebar("SIGNATURE EDITOR",
				      ps_global->mail_stream,
				      ps_global->context_current,
				      ps_global->cur_folder,
				      ps_global->msgmap,
				      0, FolderName, 0, 0);
    pbuf.pine_version  = pine_version;
    pbuf.pine_flags    = 0;
    pbuf.pine_flags   |= F_ON(F_CAN_SUSPEND,ps_global)	    ? P_SUSPEND : 0;
    pbuf.pine_flags   |= F_ON(F_USE_FK,ps_global)	    ? P_FKEYS : 0;
    pbuf.pine_flags   |= ps_global->restricted		    ? P_SECURE : 0;
    pbuf.pine_flags   |= (F_ON(F_ENABLE_ALT_ED,ps_global) ||
			  F_ON(F_ALT_ED_NOW,ps_global) ||
			  (ps_global->VAR_EDITOR
			      && ps_global->VAR_EDITOR[0])) ? P_ADVANCED : 0;
    pbuf.pine_flags   |= F_ON(F_ALT_ED_NOW,ps_global)	    ? P_ALTNOW : 0;
    pbuf.pine_flags   |= F_ON(F_USE_CURRENT_DIR,ps_global)  ? P_CURDIR : 0;
    pbuf.pine_flags   |= F_ON(F_SUSPEND_SPAWNS,ps_global)   ? P_SUBSHELL : 0;
    pbuf.pine_flags   |= F_ON(F_COMPOSE_MAPS_DEL,ps_global) ? P_DELRUBS : 0;
    pbuf.pine_flags   |= F_ON(F_ENABLE_TAB_COMPLETE,ps_global)
							    ? P_COMPLETE : 0;

    pbuf.pine_flags   |= F_ON(F_SHOW_CURSOR, ps_global)     ? P_SHOCUR : 0;
    pbuf.pine_flags   |= F_ON(F_DEL_FROM_DOT, ps_global)    ? P_DOTKILL : 0;
    pbuf.pine_flags   |= (!ps_global->VAR_CHAR_SET
			  || !strucmp(ps_global->VAR_CHAR_SET, "US-ASCII"))
			  ? P_HIBITIGN: 0;
    pbuf.pine_flags   |= F_ON(F_ENABLE_DOT_FILES, ps_global)? P_DOTFILES : 0;
    if(ps_global->VAR_OPER_DIR){
	pbuf.oper_dir    = ps_global->VAR_OPER_DIR;
	pbuf.pine_flags |= P_TREE;
    }

    /* NOTE: at this point, alot of pico struct fields are null'd out
     * thanks to the leading memset; in particular "headents" which tells
     * pico to behave like a normal editor (though modified slightly to
     * let the caller dictate the file to edit and such)...
     */

    /*
     * Now alloc and init the text to pass pico
     */
    if(!(msgso = so_get(PicoText, NULL, EDIT_ACCESS))){
        q_status_message(SM_ORDER | SM_DING, 3, 4,
			 "Error allocating space for signature file");
	dprint(1, (debugfile, "Can't alloc space for signature_edit"));
	return;
    }
    else
      pbuf.msgtext = so_text(msgso);

    if(can_access(sig_path, READ_ACCESS) == 0
       && !(tmpso = so_get(FileStar, sig_path, READ_ACCESS))){
	char *problem = error_description(errno);
	q_status_message2(SM_ORDER | SM_DING, 3, 3, "Error editing %s: %s",
			  sig_path, problem ? problem : "<NULL>");
	dprint(1, (debugfile, "signature_edit: can't open %s: %s", sig_path,
		   problem ? problem : "<NULL>"));
	return;
    }
    else if(tmpso){			/* else, fill pico's edit buffer */
	gf_set_so_readc(&gc, tmpso);	/* read from file, write pico buf */
	gf_set_so_writec(&pc, msgso);
	gf_filter_init();		/* no filters needed */
	if(errstr = gf_pipe(gc, pc)){
	    q_status_message1(SM_ORDER | SM_DING, 3, 5,
			      "Error reading signature \"%s\"", errstr);
	}

	so_give(&tmpso);
    }

    if(!errstr){
#ifdef _WINDOWS
	mswin_setwindowmenu (MENU_COMPOSER);
#endif

	/*------ OK, Go edit the signature ------*/
	editor_result = pico(&pbuf);

#ifdef _WINDOWS
	mswin_setwindowmenu (MENU_DEFAULT);
#endif
	if(editor_result & COMP_GOTHUP){
	    hup_signal();		/* do what's normal for a hup */
	}
	else{
	    fix_windsize(ps_global);
	    init_signals();
	}

	if(editor_result & (COMP_SUSPEND | COMP_GOTHUP | COMP_CANCEL)){
	}
	else{
            /*------ Must have an edited buffer, write it to .sig -----*/
	    unlink(sig_path);		/* blast old copy */
	    if(tmpso = so_get(FileStar, sig_path, WRITE_ACCESS)){
		so_seek(msgso, 0L, 0);
		gf_set_so_readc(&gc, msgso);	/* read from pico buf */
		gf_set_so_writec(&pc, tmpso);	/* write sig file */
		gf_filter_init();		/* no filters needed */
		if(errstr = gf_pipe(gc, pc)){
		    q_status_message1(SM_ORDER | SM_DING, 3, 5,
				      "Error writing signature \"%s\"",
				      errstr);
		}

		so_give(&tmpso);
	    }
	    else{
		q_status_message1(SM_ORDER | SM_DING, 3, 3,
				  "Error writing %s", sig_path);
		dprint(1, (debugfile, "signature_edit: can't write %s",
			   sig_path));
	    }
	}
    }

    so_give(&msgso);
}



/*
 *
 */
char *
sigedit_exit_for_pico()
{
    int	      rv;
    char     *rstr = NULL;
    void    (*redraw)() = ps_global->redrawer;
    static ESCKEY_S opts[] = {
	{'y', 'y', "Y", "Yes"},
	{'n', 'n', "N", "No"},
	{-1, 0, NULL, NULL}
    };

    ps_global->redrawer = NULL;
    fix_windsize(ps_global);

    while(1){
	rv = radio_buttons("Exit editor and apply changes? ",
			   -FOOTER_ROWS(ps_global), opts,
			   'y', 'x', NO_HELP, RB_NORM);
	if(rv == 'y'){				/* user ACCEPTS! */
	    break;
	}
	else if(rv == 'n'){			/* Declined! */
	    rstr = "No Changes Saved";
	    break;
	}
	else if(rv == 'x'){			/* Cancelled! */
	    rstr = "Exit Cancelled";
	    break;
	}
    }

    ps_global->redrawer = redraw;
    return(rstr);
}



/*
 *  * * * * *    Start of Config Screen Support Code   * * * * * 
 */

static struct key config_text_keys[] = 
       {{"?","Help",KS_SCREENHELP},	{NULL,NULL,KS_NONE},
	{"E","Exit Config",KS_EXITMODE},{"C","[Change Val]",KS_NONE},
	{"P","Prev",KS_NONE},		{"N","Next",KS_NONE},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{"A","Add Value",KS_NONE},	{"D","Delete Val",KS_NONE},
	{"Y","prYnt",KS_PRINT},		{"W","WhereIs",KS_WHEREIS}};
INST_KEY_MENU(config_text_keymenu, config_text_keys);

static struct key config_checkbox_keys[] = 
       {{"?","Help",KS_SCREENHELP},	{NULL,NULL,KS_NONE},
	{"E","Exit Config",KS_EXITMODE},{"X","[Set/Unset]",KS_NONE},
	{"P","Prev",KS_NONE},		{"N","Next",KS_NONE},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{"Y","prYnt",KS_PRINT},		{"W","WhereIs",KS_WHEREIS}};
INST_KEY_MENU(config_checkbox_keymenu, config_checkbox_keys);

static struct key config_radiobutton_keys[] = 
       {{"?","Help",KS_SCREENHELP},	{NULL,NULL,KS_NONE},
	{"E","Exit Config",KS_EXITMODE},{"*","[Select]",KS_NONE},
	{"P","Prev",KS_NONE},		{"N","Next",KS_NONE},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{"Y","prYnt",KS_PRINT},		{"W","WhereIs",KS_WHEREIS}};
INST_KEY_MENU(config_radiobutton_keymenu, config_radiobutton_keys);

static struct key config_yesno_keys[] = 
       {{"?","Help",KS_SCREENHELP},	{NULL,NULL,KS_NONE},
	{"E","Exit Config",KS_EXITMODE},{"C","[Change]",KS_NONE},
	{"P","Prev", KS_NONE},		{"N","Next", KS_NONE},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{"Y","prYnt",KS_PRINT},		{"W","WhereIs",KS_WHEREIS}};
INST_KEY_MENU(config_yesno_keymenu, config_yesno_keys);

/*
 * Test to indicate what should be saved in case user wants to abandon
 * changes
 */
#define	SAVE_INCLUDE(P,V) (!exclude_config_var((P), (V))		  \
		    || ((V)->is_user && (V)->is_used && !(V)->is_obsolete \
		      && ((V) == &(P)->vars[V_PERSONAL_PRINT_COMMAND])))

/*
 * Test to return if the given feature should be included
 * in the config screen.
 */
/* First, standard exclusions... */
#define	F_STD_EX(F)	((F) == F_OLD_GROWTH			\
			 || (F) == F_DISABLE_CONFIG_SCREEN	\
			 || (F) == F_DISABLE_PASSWORD_CMD	\
			 || (F) == F_DISABLE_KBLOCK_CMD		\
			 || (F) == F_DISABLE_SIGEDIT_CMD	\
			 || (F) == F_DISABLE_UPDATE_CMD		\
			 || (F) == F_DISABLE_DFLT_IN_BUG_RPT	\
			 || (F) == F_DISABLE_ALARM		\
			 || (F) == F_AGG_SEQ_COPY)

/* Then, os-dependent/feature-relative exclusions... */
#if	defined(DOS) || defined(OS2)
#define	F_SPEC_EX(F)	((F) == F_BACKGROUND_POST)
#else
#ifdef	MOUSE
#define	MOUSE_EX(F)	(0)
#else
#define	MOUSE_EX(F)	((F) == F_ENABLE_MOUSE)
#endif	/* MOUSE */

#ifdef	BACKGROUND_POST
#define	BG_EX(F)	(0)
#else
#define	BG_EX(F)	((F) == F_BACKGROUND_POST)
#endif


#define	F_SPEC_EX(F)	((F) == F_USE_FK || MOUSE_EX(F) || BG_EX(F))
#endif


#define	F_INCLUDE(F)	(!(F_STD_EX(F) || F_SPEC_EX(F)))



/*----------------------------------------------------------------------
    Present pinerc data for manipulation

    Args: None

  Result: help edit certain pinerc fields.
  ---*/
void
option_screen(ps)
    struct pine *ps;
{
    char	  tmp[MAXPATH+1];
    int		  i, j, ln = 0, lv;
    struct	  variable  *vtmp;
    CONF_S	 *ctmpa = NULL, *ctmpb, *first_line = NULL;
    NAMEVAL_S	 *f;
    SAVED_CONFIG_S *vsave;

    mailcap_free(); /* free resources we won't be using for a while */

    if(ps->fix_fixed_warning){
	set_titlebar(CONFIG_SCREEN_TITLE, ps->mail_stream,
		     ps->context_current,
		     ps->cur_folder, ps->msgmap, 1, FolderName, 0, 0);
        if(offer_to_fix_pinerc(ps))
          write_pinerc(ps);
    }

    /*
     * First, find longest variable name
     */
    for(vtmp = ps->vars; vtmp->name; vtmp++){
	if(exclude_config_var(ps, vtmp))
	  continue;

	if((i = strlen(vtmp->name)) > ln)
	  ln = i;
    }

    /*
     * Next, allocate and initialize config line list...
     */
    for(vtmp = ps->vars; vtmp->name; vtmp++){
	if(exclude_config_var(ps, vtmp))
	  continue;

	new_confline(&ctmpa)->var = vtmp;
	if(!first_line)
	  first_line = ctmpa;

	ctmpa->valoffset = ln + 3;
	ctmpa->keymenu	 = &config_text_keymenu;
	ctmpa->help	 = config_help(vtmp - ps->vars, 0);
	ctmpa->tool	 = text_tool;

	sprintf(tmp, "%-*s =", ln, vtmp->name);
	ctmpa->varname  = cpystr(tmp);
	ctmpa->varnamep = ctmpb = ctmpa;
	if(vtmp == &ps->vars[V_FEATURE_LIST]){	/* special checkbox case */
	    ctmpa->flags       |= CF_NOSELECT;
	    ctmpa->keymenu      = &config_checkbox_keymenu;
	    ctmpa->tool		= NULL;

	    /* put a nice delimiter before list */
	    new_confline(&ctmpa)->var = NULL;
	    ctmpa->varnamep	      = ctmpb;
	    ctmpa->keymenu	      = &config_checkbox_keymenu;
	    ctmpa->help		      = NO_HELP;
	    ctmpa->tool		      = checkbox_tool;
	    ctmpa->valoffset	      = 12;
	    ctmpa->flags             |= CF_NOSELECT;
	    ctmpa->value = cpystr("Set        Feature Name");

	    new_confline(&ctmpa)->var = NULL;
	    ctmpa->varnamep	      = ctmpb;
	    ctmpa->keymenu	      = &config_checkbox_keymenu;
	    ctmpa->help		      = NO_HELP;
	    ctmpa->tool		      = checkbox_tool;
	    ctmpa->valoffset	      = 12;
	    ctmpa->flags             |= CF_NOSELECT;
	    ctmpa->value = cpystr("---   ----------------------");

	    /* find longest value's name */
	    for(lv = 0, i = 0; f = feature_list(i); i++)
	      if(F_INCLUDE(f->value))
		if(lv < (j = strlen(f->name)))
		  lv = j;
	    
	    for(i = 0; f = feature_list(i); i++){
		if(F_INCLUDE(f->value)){
		    new_confline(&ctmpa)->var = vtmp;
		    ctmpa->varnamep	      = ctmpb;
		    ctmpa->keymenu	      = &config_checkbox_keymenu;
		    ctmpa->help		      = config_help(vtmp-ps->vars,
							    f->value);
		    ctmpa->tool		      = checkbox_tool;
		    ctmpa->valoffset	      = 12;
		    ctmpa->varmem	      = i;
		    sprintf(tmp, "[%c]  %-*.*s",
			    F_ON(f->value, ps) ? 'X' : ' ', lv, lv, f->name);
		    ctmpa->value = cpystr(tmp);
		}
	    }
	}
	else if(vtmp == &ps->vars[V_SAVED_MSG_NAME_RULE]){ /* radio case */
	    ctmpa->flags       |= CF_NOSELECT;
	    ctmpa->keymenu      = &config_radiobutton_keymenu;
	    ctmpa->tool		= NULL;

	    /* put a nice delimiter before list */
	    new_confline(&ctmpa)->var = NULL;
	    ctmpa->varnamep	      = ctmpb;
	    ctmpa->keymenu	      = &config_radiobutton_keymenu;
	    ctmpa->help		      = NO_HELP;
	    ctmpa->tool		      = radiobutton_tool;
	    ctmpa->valoffset	      = 12;
	    ctmpa->flags             |= CF_NOSELECT;
	    ctmpa->value = cpystr("Set       Rule Values");

	    new_confline(&ctmpa)->var = NULL;
	    ctmpa->varnamep	      = ctmpb;
	    ctmpa->keymenu	      = &config_radiobutton_keymenu;
	    ctmpa->help		      = NO_HELP;
	    ctmpa->tool		      = radiobutton_tool;
	    ctmpa->valoffset	      = 12;
	    ctmpa->flags             |= CF_NOSELECT;
	    ctmpa->value = cpystr("---   ----------------------");

	    /* find longest value's name */
	    for(lv = 0, i = 0; f = save_msg_rules(i); i++)
	      if(lv < (j = strlen(f->name)))
		lv = j;
	    
	    for(i = 0; f = save_msg_rules(i); i++){
		new_confline(&ctmpa)->var = vtmp;
		ctmpa->varnamep		  = ctmpb;
		ctmpa->keymenu		  = &config_radiobutton_keymenu;
		ctmpa->help		  = config_help(vtmp - ps->vars, 0);
		ctmpa->tool		  = radiobutton_tool;
		ctmpa->valoffset	  = 12;
		ctmpa->varmem		  = i;
		sprintf(tmp, "(%c)  %-*.*s",
			(ps->save_msg_rule == f->value) ? R_SELD : ' ',
			lv, lv, f->name);
		ctmpa->value = cpystr(tmp);
	    }
	}
	else if(vtmp == &ps->vars[V_FCC_RULE]){		/* radio case */
	    ctmpa->flags       |= CF_NOSELECT;
	    ctmpa->keymenu      = &config_radiobutton_keymenu;
	    ctmpa->tool		= NULL;

	    /* put a nice delimiter before list */
	    new_confline(&ctmpa)->var = NULL;
	    ctmpa->varnamep	      = ctmpb;
	    ctmpa->keymenu	      = &config_radiobutton_keymenu;
	    ctmpa->help		      = NO_HELP;
	    ctmpa->tool		      = radiobutton_tool;
	    ctmpa->valoffset	      = 12;
	    ctmpa->flags             |= CF_NOSELECT;
	    ctmpa->value = cpystr("Set       Rule Values");

	    new_confline(&ctmpa)->var = NULL;
	    ctmpa->varnamep	      = ctmpb;
	    ctmpa->keymenu	      = &config_radiobutton_keymenu;
	    ctmpa->help		      = NO_HELP;
	    ctmpa->tool		      = radiobutton_tool;
	    ctmpa->valoffset	      = 12;
	    ctmpa->flags             |= CF_NOSELECT;
	    ctmpa->value = cpystr("---   ----------------------");

	    /* find longest value's name */
	    for(lv = 0, i = 0; f = fcc_rules(i); i++)
	      if(lv < (j = strlen(f->name)))
		lv = j;
	    
	    for(i = 0; f = fcc_rules(i); i++){
		new_confline(&ctmpa)->var = vtmp;
		ctmpa->varnamep		  = ctmpb;
		ctmpa->keymenu		  = &config_radiobutton_keymenu;
		ctmpa->help		  = config_help(vtmp - ps->vars, 0);
		ctmpa->tool		  = radiobutton_tool;
		ctmpa->valoffset	  = 12;
		ctmpa->varmem		  = i;
		sprintf(tmp, "(%c)  %-*.*s",
			(ps->fcc_rule == f->value) ? R_SELD : ' ',
			lv, lv, f->name);
		ctmpa->value = cpystr(tmp);
	    }
	}
	else if(vtmp == &ps->vars[V_SORT_KEY]){ /* radio case */
	    ctmpa->flags       |= CF_NOSELECT;
	    ctmpa->keymenu      = &config_radiobutton_keymenu;
	    ctmpa->tool		= NULL;

	    /* put a nice delimiter before list */
	    new_confline(&ctmpa)->var = NULL;
	    ctmpa->varnamep	      = ctmpb;
	    ctmpa->keymenu	      = &config_radiobutton_keymenu;
	    ctmpa->help		      = NO_HELP;
	    ctmpa->tool		      = radiobutton_tool;
	    ctmpa->valoffset	      = 12;
	    ctmpa->flags             |= CF_NOSELECT;
	    ctmpa->value = cpystr("Set       Sort Options");

	    new_confline(&ctmpa)->var = NULL;
	    ctmpa->varnamep	      = ctmpb;
	    ctmpa->keymenu	      = &config_radiobutton_keymenu;
	    ctmpa->help		      = NO_HELP;
	    ctmpa->tool		      = radiobutton_tool;
	    ctmpa->valoffset	      = 12;
	    ctmpa->flags             |= CF_NOSELECT;
	    ctmpa->value = cpystr("---   ----------------------");

	    /* find longest value's name */
	    for(lv = 0, i = 0; ps->sort_types[i] != EndofList; i++)
	      if(lv < (j = strlen(sort_name(i))))
		lv = j;
	    
	    for(j = 0; j < 2; j++){
		for(i = 0; ps->sort_types[i] != EndofList; i++){
		    new_confline(&ctmpa)->var = vtmp;
		    ctmpa->varnamep  = ctmpb;
		    ctmpa->keymenu   = &config_radiobutton_keymenu;
		    ctmpa->help	     = config_help(vtmp - ps->vars, 0);
		    ctmpa->tool	     = radiobutton_tool;
		    ctmpa->valoffset = 12;

		    /*
		     * varmem == sort_type index (reverse doubles index)
		     */
		    ctmpa->varmem = i + (j * EndofList);
		    sprintf(tmp, "(%c)  %s%-*s%*s",
			    (ps->def_sort == (SortOrder) i
						  && ps->def_sort_rev == j)
			      ? R_SELD : ' ',
			    (j) ? "Reverse " : "",
			    lv, sort_name(i),
			    (j) ? 0 : 8, "");
		    ctmpa->value = cpystr(tmp);
		}
	    }
	}
	else if(vtmp == &ps->vars[V_AB_SORT_RULE]){	/* radio case */
	    ctmpa->flags       |= CF_NOSELECT;
	    ctmpa->keymenu      = &config_radiobutton_keymenu;
	    ctmpa->tool		= NULL;

	    /* put a nice delimiter before list */
	    new_confline(&ctmpa)->var = NULL;
	    ctmpa->varnamep	      = ctmpb;
	    ctmpa->keymenu	      = &config_radiobutton_keymenu;
	    ctmpa->help		      = NO_HELP;
	    ctmpa->tool		      = radiobutton_tool;
	    ctmpa->valoffset	      = 12;
	    ctmpa->flags             |= CF_NOSELECT;
	    ctmpa->value = cpystr("Set       Rule Values");

	    new_confline(&ctmpa)->var = NULL;
	    ctmpa->varnamep	      = ctmpb;
	    ctmpa->keymenu	      = &config_radiobutton_keymenu;
	    ctmpa->help		      = NO_HELP;
	    ctmpa->tool		      = radiobutton_tool;
	    ctmpa->valoffset	      = 12;
	    ctmpa->flags             |= CF_NOSELECT;
	    ctmpa->value = cpystr("---   ----------------------");

	    /* find longest value's name */
	    for(lv = 0, i = 0; f = ab_sort_rules(i); i++)
	      if(lv < (j = strlen(f->name)))
		lv = j;
	    
	    for(i = 0; f = ab_sort_rules(i); i++){
		new_confline(&ctmpa)->var = vtmp;
		ctmpa->varnamep		  = ctmpb;
		ctmpa->keymenu		  = &config_radiobutton_keymenu;
		ctmpa->help		  = config_help(vtmp - ps->vars, 0);
		ctmpa->tool		  = radiobutton_tool;
		ctmpa->valoffset	  = 12;
		ctmpa->varmem		  = i;
		sprintf(tmp, "(%c)  %-*.*s",
			(ps->ab_sort_rule == f->value) ? R_SELD : ' ',
			lv, lv, f->name);
		ctmpa->value = cpystr(tmp);
	    }
	}
	else if(vtmp == &ps->vars[V_GOTO_DEFAULT_RULE]){ /* radio case */
	    ctmpa->flags       |= CF_NOSELECT;
	    ctmpa->keymenu      = &config_radiobutton_keymenu;
	    ctmpa->tool		= NULL;

	    /* put a nice delimiter before list */
	    new_confline(&ctmpa)->var = NULL;
	    ctmpa->varnamep	      = ctmpb;
	    ctmpa->keymenu	      = &config_radiobutton_keymenu;
	    ctmpa->help		      = NO_HELP;
	    ctmpa->tool		      = radiobutton_tool;
	    ctmpa->valoffset	      = 12;
	    ctmpa->flags             |= CF_NOSELECT;
	    ctmpa->value = cpystr("Set       Rule Values");

	    new_confline(&ctmpa)->var = NULL;
	    ctmpa->varnamep	      = ctmpb;
	    ctmpa->keymenu	      = &config_radiobutton_keymenu;
	    ctmpa->help		      = NO_HELP;
	    ctmpa->tool		      = radiobutton_tool;
	    ctmpa->valoffset	      = 12;
	    ctmpa->flags             |= CF_NOSELECT;
	    ctmpa->value = cpystr("---   ----------------------");

	    /* find longest value's name */
	    for(lv = 0, i = 0; f = goto_rules(i); i++)
	      if(lv < (j = strlen(f->name)))
		lv = j;
	    
	    for(i = 0; f = goto_rules(i); i++){
		new_confline(&ctmpa)->var = vtmp;
		ctmpa->varnamep		  = ctmpb;
		ctmpa->keymenu		  = &config_radiobutton_keymenu;
		ctmpa->help		  = config_help(vtmp - ps->vars, 0);
		ctmpa->tool		  = radiobutton_tool;
		ctmpa->valoffset	  = 12;
		ctmpa->varmem		  = i;
		sprintf(tmp, "(%c)  %-*.*s",
			(ps->goto_default_rule == f->value) ? R_SELD : ' ',
			lv, lv, f->name);
		ctmpa->value = cpystr(tmp);
	    }
	}
#if defined(DOS) || defined(OS2)
	else if(vtmp == &ps->vars[V_NORM_FORE_COLOR] /* radio case */
		|| vtmp == &ps->vars[V_NORM_BACK_COLOR]
		|| vtmp == &ps->vars[V_REV_FORE_COLOR]
		|| vtmp == &ps->vars[V_REV_BACK_COLOR]){
	    ctmpa->flags       |= CF_NOSELECT;
	    ctmpa->keymenu      = &config_radiobutton_keymenu;
	    ctmpa->tool		= NULL;

	    /* put a nice delimiter before list */
	    new_confline(&ctmpa)->var = NULL;
	    ctmpa->varnamep	      = ctmpb;
	    ctmpa->keymenu	      = &config_radiobutton_keymenu;
	    ctmpa->help		      = NO_HELP;
	    ctmpa->tool		      = radiobutton_tool;
	    ctmpa->valoffset	      = 12;
	    ctmpa->flags             |= CF_NOSELECT;
	    ctmpa->value = cpystr("Set       Color Options");

	    new_confline(&ctmpa)->var = NULL;
	    ctmpa->varnamep	      = ctmpb;
	    ctmpa->keymenu	      = &config_radiobutton_keymenu;
	    ctmpa->help		      = NO_HELP;
	    ctmpa->tool		      = radiobutton_tool;
	    ctmpa->valoffset	      = 12;
	    ctmpa->flags             |= CF_NOSELECT;
	    ctmpa->value = cpystr("---   ----------------------");

	    /* find longest value's name */
	    for(lv = 0, i = 0; config_colors[i]; i++)
	      if(lv < (j = strlen(config_colors[i])))
		lv = j;
	    
	    for(i = 0; config_colors[i]; i++){
		new_confline(&ctmpa)->var = vtmp;
		ctmpa->varnamep		  = ctmpb;
		ctmpa->keymenu		  = &config_radiobutton_keymenu;
		ctmpa->help		  = config_help(vtmp - ps->vars, 0);
		ctmpa->tool		  = radiobutton_tool;
		ctmpa->valoffset	  = 12;

		/*
		 * varmem == sort_type index (reverse doubles index)
		 */
		ctmpa->varmem = i;
		sprintf(tmp, "(%c)  %-*.*s",
			!strucmp(ps->vars[vtmp - ps->vars].current_val.p,
				 config_colors[i]) ? R_SELD : ' ',
			lv, lv, config_colors[i]);
		ctmpa->value = cpystr(tmp);
	    }
	}
#endif
	else if(vtmp == &ps->vars[V_USE_ONLY_DOMAIN_NAME]){ /* yesno case */
	    ctmpa->keymenu = &config_yesno_keymenu;
	    ctmpa->tool	   = yesno_tool;
	    if(vtmp->user_val.p && !strucmp(vtmp->user_val.p, "yes")
	       || (!vtmp->user_val.p && vtmp->current_val.p
		   && !strucmp(vtmp->current_val.p, "yes")))
	      sprintf(tmp, "Yes%*s", ps->ttyo->screen_cols - ln - 3, "");
	    else
	      sprintf(tmp, "No%*s", ps->ttyo->screen_cols - ln - 2, "");

	    ctmpa->value = cpystr(tmp);
	}
	else if(vtmp->is_list){
	    if(vtmp->user_val.l){
		for(i = 0; vtmp->user_val.l[i]; i++){
		    if(i)
		      (void)new_confline(&ctmpa);

		    ctmpa->var       = vtmp;
		    ctmpa->varmem    = i;
		    ctmpa->valoffset = ln + 3;
		    ctmpa->value     = pretty_value(ps, ctmpa);
		    ctmpa->keymenu   = &config_text_keymenu;
		    ctmpa->help      = config_help(vtmp - ps->vars, 0);
		    ctmpa->tool      = text_tool;
		    ctmpa->varnamep  = ctmpb;
		}
	    }
	    else{
		ctmpa->varmem = 0;
		ctmpa->value  = pretty_value(ps, ctmpa);
	    }
	}
	else{
	    if(vtmp == &ps->vars[V_FILLCOL]
	       || vtmp == &ps->vars[V_OVERLAP]
	       || vtmp == &ps->vars[V_MARGIN]
	       || vtmp == &ps->vars[V_STATUS_MSG_DELAY]
	       || vtmp == &ps->vars[V_MAILCHECK])
	      ctmpa->flags |= CF_NUMBER;

	    ctmpa->value = pretty_value(ps, ctmpa);
	}
    }

    vsave = save_config_vars(ps);
    first_line = first_sel_confline(first_line);
    switch(conf_scroll_screen(ps, first_line, CONFIG_SCREEN_TITLE, Config)){
      case 0:
	break;

      case 1:
	write_pinerc(ps);
	break;
    
      case 10:
	revert_to_saved_config(ps, vsave);
	break;
      
      default:
	q_status_message(SM_ORDER,7,10,
	    "conf_scroll_screen bad ret, not supposed to happen");
	break;
    }

    if(vsave[V_SORT_KEY].user_val.p && ps->vars[V_SORT_KEY].user_val.p
       && strcmp(vsave[V_SORT_KEY].user_val.p,
		 ps->vars[V_SORT_KEY].user_val.p)){
	clear_index_cache();
	sort_current_folder(0, SortArrival, 0);
    }

    free_saved_config(ps, &vsave);
#ifdef _WINDOWS
    mswin_set_quit_confirm (F_OFF(F_QUIT_WO_CONFIRM, ps_global));
#endif
}


/*
 * test whether or not a var is 
 *
 * returns:  1 if it should be excluded, 0 otw
 */
int
exclude_config_var(ps, var)
    struct pine *ps;
    struct variable *var;
{
    switch(var - ps->vars){
      case V_MAIL_DIRECTORY :
      case V_INCOMING_FOLDERS :
      case V_PRINTER :
      case V_PERSONAL_PRINT_COMMAND :
      case V_PERSONAL_PRINT_CATEGORY :
      case V_STANDARD_PRINTER :
      case V_LAST_TIME_PRUNE_QUESTION :
      case V_LAST_VERS_USED :
      case V_OPER_DIR :
      case V_TCPOPENTIMEO :
      case V_RSHOPENTIMEO :
      case V_SENDMAIL_PATH :
      case V_NEW_VER_QUELL :
#if defined(DOS) || defined(OS2)
      case V_UPLOAD_CMD :
      case V_UPLOAD_CMD_PREFIX :
      case V_DOWNLOAD_CMD :
      case V_DOWNLOAD_CMD_PREFIX :
#ifdef	_WINDOWS
      case V_FONT_NAME :
      case V_FONT_SIZE :
      case V_FONT_STYLE :
      case V_PRINT_FONT_NAME :
      case V_PRINT_FONT_SIZE :
      case V_PRINT_FONT_STYLE :
      case V_WINDOW_POSITION :
#endif	/* _WINDOWS */
#endif	/* DOS */
	return(1);

      default:
	break;
    }

    return(!(var->is_user && var->is_used && !var->is_obsolete));
}


#ifndef	DOS
static struct key printer_edit_keys[] = 
       {{"?","Help",KS_SCREENHELP},	{"Y","prYnt",KS_PRINT},
	{"E","Exit Config",KS_EXITMODE},{"S","[Select]",KS_NONE},
	{"P","Prev",KS_NONE},		{"N","Next",KS_NONE},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{"A","Add Printer",KS_NONE},	{"D","DeletePrint",KS_NONE},
	{"C","Change",KS_SELECT},	{"W","WhereIs",KS_WHEREIS}};
INST_KEY_MENU(printer_edit_keymenu, printer_edit_keys);

static struct key printer_select_keys[] = 
       {{"?","Help",KS_SCREENHELP},	{"Y","prYnt",KS_PRINT},
	{"E","Exit Config",KS_EXITMODE},{"S","[Select]",KS_NONE},
	{"P","Prev",KS_NONE},		{"N","Next",KS_NONE},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{NULL,NULL,KS_NONE},	        {NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},	        {"W","WhereIs",KS_WHEREIS}};
INST_KEY_MENU(printer_select_keymenu, printer_select_keys);

/*
 * Information used to paint and maintain a line on the configuration screen
 */
/*----------------------------------------------------------------------
    The printer selection screen

   Draws the screen and prompts for the printer number and the custom
   command if so selected.

 ----*/
void
select_printer(ps) 
    struct pine *ps;
{
    struct        variable  *vtmp;
    CONF_S       *ctmpa = NULL, *ctmpb = NULL, *heading = NULL,
		 *start_line = NULL;
    int j, i, saved_printer_cat;
    char tmp[MAXPATH+1];
    SAVED_CONFIG_S *vsave;
    char *saved_printer, *p;
    char *nick, *cmd;

    if(ps_global->vars[V_PRINTER].is_fixed){
	q_status_message(SM_ORDER,3,5,
	    "Sys. Mgmt. does not allow changing printer");
	return;
    }

    saved_printer = cpystr(ps->VAR_PRINTER);
    saved_printer_cat = ps->printer_category;

    new_confline(&ctmpa);
    ctmpa->valoffset = 2;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_select_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
#ifdef OS2
      = cpystr("\"Select\" a port or |pipe-command as your default printer.");
#else
      = cpystr("You may \"Select\" a print command as your default printer.");
#endif

    new_confline(&ctmpa);
    ctmpa->valoffset = 2;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_select_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
#ifdef OS2
      = cpystr("You may also add alternative ports or !pipes to the list in the \"Personally\"");
#else
      = cpystr("You may also add custom print commands to the list in the \"Personally\"");
#endif

    new_confline(&ctmpa);
    ctmpa->valoffset = 2;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_select_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
#ifdef OS2
      = cpystr("selected port or |pipe\" section below.");
#else
      = cpystr("selected print command\" section below.");
#endif

    new_confline(&ctmpa);
    ctmpa->valoffset = 2;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_select_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("");

    new_confline(&ctmpa);
    ctmpa->valoffset = 4;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_select_tool;
    ctmpa->flags    |= CF_NOSELECT;
    def_printer_line = &ctmpa->value;
    set_def_printer_value(ps->VAR_PRINTER);

    new_confline(&ctmpa);
    ctmpa->valoffset = 2;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_select_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("");

#ifndef OS2
    new_confline(&ctmpa);
    heading = ctmpa;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_select_tool;
    ctmpa->varname
	= cpystr(" Printer attached to IBM PC or compatible, Macintosh");
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value     = cpystr("");
    ctmpa->headingp  = heading;

    new_confline(&ctmpa);
    ctmpa->valoffset = 6;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_select_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("This may not work with all attached printers, and will depend on the");
    ctmpa->headingp  = heading;

    new_confline(&ctmpa);
    ctmpa->valoffset = 6;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_select_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("terminal emulation/communications software in use. It is known to work");
    ctmpa->headingp  = heading;

    new_confline(&ctmpa);
    ctmpa->valoffset = 6;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_select_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("with Kermit and the latest UW version of NCSA telnet on Macs and PCs,");
    ctmpa->headingp  = heading;

    new_confline(&ctmpa);
    ctmpa->valoffset = 6;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_select_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("Versaterm Pro on Macs, and WRQ Reflections on PCs.");
    ctmpa->headingp  = heading;

    new_confline(&ctmpa);
    start_line = ctmpb = ctmpa; /* default start line */
    ctmpa->valoffset = 17;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = h_config_set_att_ansi;
    ctmpa->tool      = print_select_tool;
    ctmpa->flags    |= CF_NOHILITE;
    ctmpa->varoffset = 8;
    ctmpa->varname   = cpystr("Printer:");
    ctmpa->value
      = cpystr(ANSI_PRINTER);
    ctmpa->varnamep  = ctmpb;
    ctmpa->headingp  = heading;

    new_confline(&ctmpa);
    ctmpa->valoffset = 17;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = h_config_set_att_ansi2;
    ctmpa->tool      = print_select_tool;
    ctmpa->flags    |= CF_NOHILITE;
    ctmpa->varoffset = 8;
    ctmpa->value     = (char *)fs_get(strlen(ANSI_PRINTER)+strlen(no_ff)+1);
    ctmpa->varnamep  = ctmpb;
    ctmpa->headingp  = heading;
    strcat(strcpy(ctmpa->value, ANSI_PRINTER), no_ff);
#endif

    new_confline(&ctmpa);
    ctmpa->valoffset = 0;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_select_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value = cpystr("");
    ctmpa->var = &ps->vars[V_STANDARD_PRINTER];


    new_confline(&ctmpa);
    heading = ctmpa;
    ctmpa->valoffset = 6;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_select_tool;
    ctmpa->varname
#ifdef OS2
        = cpystr(" Standard OS/2 printer port");
#else
	= cpystr(" Standard UNIX print command");
#endif
    ctmpa->value = cpystr("");
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->headingp  = heading;
    ctmpa->var = &ps->vars[V_STANDARD_PRINTER];

    new_confline(&ctmpa);
    ctmpa->valoffset = 6;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_select_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
#ifdef OS2
      = cpystr("Using this option may require you to use the OS/2 \"MODE\" command to");
#else
      = cpystr("Using this option may require setting your \"PRINTER\" or \"LPDEST\"");
#endif
    ctmpa->headingp  = heading;
    ctmpa->var = &ps->vars[V_STANDARD_PRINTER];

    new_confline(&ctmpa);
    ctmpa->valoffset = 6;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_select_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
#ifdef OS2
      = cpystr("direct printer output to the correct port.");
#else
      = cpystr("environment variable using the standard UNIX utilities.");
#endif
    ctmpa->headingp  = heading;
    ctmpa->var = &ps->vars[V_STANDARD_PRINTER];

    vtmp = &ps->vars[V_STANDARD_PRINTER];
    for(i = 0; vtmp->current_val.l[i]; i++){
	new_confline(&ctmpa);
	ctmpa->valoffset = 22;
	ctmpa->keymenu   = &printer_select_keymenu;
	ctmpa->help      = NO_HELP;
	ctmpa->help      = h_config_set_stand_print;
	ctmpa->tool      = print_select_tool;
	if(i == 0){
	    ctmpa->varoffset = 8;
	    ctmpa->varname   = cpystr("Printer List:");
	    ctmpa->flags    |= CF_NOHILITE;
#ifdef OS2
	    start_line = ctmpb = ctmpa; /* default start line */
#else
	    ctmpb = ctmpa;
#endif
	}

	ctmpa->varnamep  = ctmpb;
	ctmpa->headingp  = heading;
	ctmpa->varmem = i;
	ctmpa->var = vtmp;
	ctmpa->value = printer_name(vtmp->current_val.l[i]);
    }

    new_confline(&ctmpa);
    ctmpa->valoffset = 0;
    ctmpa->keymenu   = &printer_select_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_select_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value = cpystr("");

    new_confline(&ctmpa);
    heading = ctmpa;
    ctmpa->valoffset = 0;
    ctmpa->keymenu   = &printer_edit_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_edit_tool;
    ctmpa->varname
#ifdef OS2
        = cpystr(" Personally selected port or |command");
#else
	= cpystr(" Personally selected print command");
#endif
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value = cpystr("");
    ctmpa->headingp  = heading;
    ctmpa->var = &ps->vars[V_PERSONAL_PRINT_COMMAND];


    new_confline(&ctmpa);
    ctmpa->valoffset = 6;
    ctmpa->keymenu   = &printer_edit_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_edit_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
#ifdef OS2
      = cpystr("The text to be printed will be sent to the printer or command given here.");
#else
      = cpystr("The text to be printed will be piped into the command given here. The");
#endif
    ctmpa->headingp  = heading;
    ctmpa->var = &ps->vars[V_PERSONAL_PRINT_COMMAND];

    new_confline(&ctmpa);
    ctmpa->valoffset = 6;
    ctmpa->keymenu   = &printer_edit_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_edit_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
#ifdef OS2
      = cpystr("The printer port or |pipe is in the 2nd column, the printer name is in");
#else
      = cpystr("command is in the 2nd column, the printer name is in the first column. Some");
#endif
    ctmpa->headingp  = heading;
    ctmpa->var = &ps->vars[V_PERSONAL_PRINT_COMMAND];

    new_confline(&ctmpa);
    ctmpa->valoffset = 6;
    ctmpa->keymenu   = &printer_edit_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_edit_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
#ifdef OS2
      = cpystr("the first column. Examples: \"LPT1\", \"COM2\", \"|enscript\". A command may");
#else
      = cpystr("examples are: \"prt\", \"lpr\", \"lp\", or \"enscript\". The command may be given");
#endif
    ctmpa->headingp  = heading;
    ctmpa->var = &ps->vars[V_PERSONAL_PRINT_COMMAND];

    new_confline(&ctmpa);
    ctmpa->valoffset = 6;
    ctmpa->keymenu   = &printer_edit_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_edit_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
#ifdef OS2
      = cpystr("be given options, for example \"|ascii2ps -p LPT1\" or \"|txt2hplc -2\". Use");
#else
      = cpystr("with options, for example \"enscript -2 -r\" or \"lpr -Plpacc170\". The");
#endif
    ctmpa->headingp  = heading;
    ctmpa->var = &ps->vars[V_PERSONAL_PRINT_COMMAND];

    new_confline(&ctmpa);
    ctmpa->valoffset = 6;
    ctmpa->keymenu   = &printer_edit_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = print_edit_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
#ifdef OS2
      = cpystr("the |command method for printers that require conversion from ASCII.");
#else
      = cpystr("commands and options on your system may be different from these examples.");
#endif
    ctmpa->headingp  = heading;
    ctmpa->var = &ps->vars[V_PERSONAL_PRINT_COMMAND];

    vtmp = &ps->vars[V_PERSONAL_PRINT_COMMAND];
    if(vtmp->current_val.l){
	for(i = 0; vtmp->current_val.l[i]; i++){
	    new_confline(&ctmpa);
	    ctmpa->valoffset = 22;
	    ctmpa->keymenu   = &printer_edit_keymenu;
	    ctmpa->help      = h_config_set_custom_print;
	    ctmpa->tool      = print_edit_tool;
	    if(i == 0){
		ctmpa->varoffset = 8;
		ctmpa->varname   = cpystr("Printer List:");
		ctmpa->flags    |= CF_NOHILITE;
		ctmpb = ctmpa;
	    }

	    ctmpa->varnamep  = ctmpb;
	    ctmpa->headingp  = heading;
	    ctmpa->varmem = i;
	    ctmpa->var = vtmp;
	    ctmpa->value = printer_name(vtmp->current_val.l[i]);
	}
    }
    else{
	new_confline(&ctmpa);
	ctmpa->valoffset = 22;
	ctmpa->keymenu   = &printer_edit_keymenu;
	ctmpa->help      = h_config_set_custom_print;
	ctmpa->tool      = print_edit_tool;
	ctmpa->flags    |= CF_NOHILITE;
	ctmpa->varoffset = 8;
	ctmpa->varname   = cpystr("Printer List:");
	ctmpa->varnamep  = ctmpa;
	ctmpa->headingp  = heading;
	ctmpa->varmem    = 0;
	ctmpa->var       = vtmp;
	ctmpa->value     = cpystr("");
    }

    vsave = save_config_vars(ps);
    switch(conf_scroll_screen(ps, start_line, "SELECT PRINTER", PrintConfig)){
      case 0:
	break;
    
      case 1:
	write_pinerc(ps);
	break;
    
      case 10:
	revert_to_saved_config(ps, vsave);
	ps->printer_category = saved_printer_cat;
	set_variable(V_PRINTER, saved_printer, 0);
	set_variable(V_PERSONAL_PRINT_CATEGORY, 
	    comatose(ps->printer_category), 0);
	break;
    }

    def_printer_line = NULL;
    free_saved_config(ps, &vsave);
    fs_give((void **)&saved_printer);
}
#endif	/* !DOS */


void
set_def_printer_value(printer)
    char *printer;
{
    char *p, *nick, *cmd;
    int set;

    if(!def_printer_line)
      return;

    parse_printer(printer, &nick, &cmd, NULL, NULL, NULL, NULL);
    p = *nick ? nick : cmd;
    set = *p;
    if(*def_printer_line)
      fs_give((void **)def_printer_line);

    *def_printer_line = fs_get(36 + strlen(p) + 1);
    sprintf(*def_printer_line, "Default printer currently %s%s%s",
	set ? "set to \"" : "unset", set ? p : "", set ? "\"." : "."); 

    fs_give((void **)&nick);
    fs_give((void **)&cmd);
}


static struct key flag_keys[] = 
       {{"?","Help",KS_SCREENHELP},	{NULL,NULL,KS_NONE},
	{"E","Exit Flags",KS_EXITMODE}, {"X","[Set/Unset]",KS_NONE},
	{"P","Prev",KS_NONE},		{"N","Next",KS_NONE},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{"Y","prYnt",KS_PRINT},		{"W","WhereIs",KS_WHEREIS}};
INST_KEY_MENU(flag_keymenu, flag_keys);

/*----------------------------------------------------------------------
   Function to control flag set/clearing

   Basically, turn the flags into a fake list of features...

 ----*/
void
flag_maintenance_screen(ps, flags)
    struct pine	       *ps;
    struct flag_screen *flags;
{
    int		  i, lv;
    char	  tmp[256], **p;
    CONF_S	 *ctmpa = NULL, *ctmpb = NULL, *first_line = NULL;
    struct	  flag_table  *fp;
    MESSAGECACHE *mc = NULL;

    for(p = flags->explanation; p && *p; p++){
	new_confline(&ctmpa);
	ctmpa->keymenu   = &flag_keymenu;
	ctmpa->help      = NO_HELP;
	ctmpa->tool      = flag_checkbox_tool;
	ctmpa->flags    |= CF_NOSELECT;
	ctmpa->valoffset = 0;
	ctmpa->value     = cpystr(*p);
    }

    /* Now wire flags checkboxes together */
    for(lv = 0, fp = flags->flag_table; fp->name; fp++)	/* longest name */
      if(lv < (i = strlen(fp->name)))
	lv = i;

    for(fp = flags->flag_table; fp->name; fp++){	/* build the list */
	new_confline(&ctmpa);
	if(!first_line)
	  first_line = ctmpa;

	ctmpa->varnamep		  = ctmpb;
	ctmpa->keymenu		  = &flag_keymenu;
	ctmpa->help		  = fp->help;
	ctmpa->tool		  = flag_checkbox_tool;
	ctmpa->scrap		  = fp;
	ctmpa->valoffset	  = 12;

	sprintf(tmp, "[%c]  %-*.*s",
		(fp->set == 0) ? ' ' : (fp->set == 1) ? 'X' : '?',
		lv, lv, fp->name);
	ctmpa->value = cpystr(tmp);
    }
      
    (void) conf_scroll_screen(ps, first_line, "FLAG MAINTENANCE", Config);
    ps->mangled_screen = 1;
}


/*
 * Handles screen painting and motion.  Passes other commands to
 * custom tools.
 *
 * Tool return values:  Tools should return the following:
 *     0 nothing changed
 *    -1 unrecognized command
 *     1 something changed, conf_scroll_screen should remember that
 *     2 tells conf_scroll_screen to return with value 1 or 0 depending
 *       on whether or not it has previously gotten a 1 from some tool.
 *     3 tells conf_scroll_screen to return 1 (like 1 and 2 combined)
 *     ? Other tool-specific values can be used.  They will cause
 *       conf_scroll_screen to return that value.
 *
 * Return values:
 *     0 if nothing happened.  That is, a tool returned 2 and we hadn't
 *       previously noted a return of 1
 *     1 if something happened.  That is, a tool returned 2 and we had
 *       previously noted a return of 1
 *     ? Tool-returned value different from -1, 0, 1, or 2.  This is it.
 */
int
conf_scroll_screen(ps, start_line, title, style)
    struct pine *ps;
    CONF_S      *start_line;
    char        *title;
    ConfigType   style;
{
    char	  tmp[MAXPATH+1];
    int		  i, j, ch = 'x', orig_ch, done = 0, changes = 0;
    int		  retval = 0;
    int		  km_popped = 0;
    struct	  key_menu  *km = NULL;
    CONF_S	 *ctmpa = NULL, *ctmpb;
    OPT_SCREEN_S  screen;
    NAMEVAL_S	 *f;
    Pos           cursor_pos;

    memset(&screen, 0, sizeof(OPT_SCREEN_S));
    screen.first_line = first_sel_confline(start_line);
    screen.current    = start_line;
    opt_screen	       = &screen;
    ps->mangled_screen = 1;
    ps->redrawer       = option_screen_redrawer;

    while(!done){
	if(km_popped){
	    km_popped--;
	    if(km_popped == 0){
		clearfooter(ps);
		ps->mangled_body = 1;
	    }
	}

	if(ps->mangled_screen){
	    ps->mangled_header = 1;
	    ps->mangled_footer = 1;
	    ps->mangled_body   = 1;
	    ps->mangled_screen = 0;
	}

	/*----------- Check for new mail -----------*/
        if(new_mail(0, NM_TIMING(ch), 1) >= 0)
          ps->mangled_header = 1;

	if(ps->mangled_header){
	    set_titlebar(title, ps->mail_stream,
			 ps->context_current,
			 ps->cur_folder, ps->msgmap, 1, FolderName, 0, 0);
	    ps->mangled_header = 0;
	}

	update_option_screen(ps, &screen, &cursor_pos);

	if(F_OFF(F_SHOW_CURSOR, ps)){
	    cursor_pos.row = ps->ttyo->screen_rows - FOOTER_ROWS(ps);
	    cursor_pos.col = 0;
	}

	/*---- This displays new mail notification, or errors ---*/
	if(km_popped){
	    FOOTER_ROWS(ps) = 3;
	    mark_status_unknown();
	}

        display_message(ch);
	if(km_popped){
	    FOOTER_ROWS(ps) = 1;
	    mark_status_unknown();
	}

	if(ps->mangled_footer || km != screen.current->keymenu){
	    bitmap_t	 bitmap;

	    setbitmap(bitmap);

	    ps->mangled_footer = 0;
	    km                 = screen.current->keymenu;
	    if(km_popped){
		FOOTER_ROWS(ps) = 3;
		clearfooter(ps);
	    }

	    draw_keymenu(km, bitmap, ps->ttyo->screen_cols,
		1-FOOTER_ROWS(ps), 0, FirstMenu,0);

	    if(km_popped){
		FOOTER_ROWS(ps) = 1;
		mark_keymenu_dirty();
	    }
	}

	MoveCursor(cursor_pos.row, cursor_pos.col);
#ifdef	MOUSE
	mouse_in_content(KEY_MOUSE, -1, -1, 0, 0);	/* prime the handler */
	register_mfunc(mouse_in_content, HEADER_ROWS(ps_global), 0,
		       ps_global->ttyo->screen_rows -(FOOTER_ROWS(ps_global)+1),
		       ps_global->ttyo->screen_cols);
#endif
#ifdef	_WINDOWS
	mswin_setscrollcallback(config_scroll_callback);
#endif
        /*------ Read the command from the keyboard ----*/
	ch = orig_ch = read_command();
#ifdef	MOUSE
	clear_mfunc(mouse_in_content);
#endif
#ifdef	_WINDOWS
	mswin_setscrollcallback(NULL);
#endif

        if(ch <= 0xff && isupper((unsigned char)ch))
          ch = tolower((unsigned char)ch);

	if(km_popped)
	  switch(ch){
	    case NO_OP_IDLE:
	    case NO_OP_COMMAND: 
	    case KEY_RESIZE:
	    case ctrl('L'):
	      km_popped++;
	      break;
	    
	    default:
	      clearfooter(ps);
	      break;
	  }

	switch(ch){
	  case '?' :				/* help! */
	  case ctrl('G'):
	  case PF1 :
	    if(FOOTER_ROWS(ps) == 1 && km_popped == 0){
		km_popped = 2;
		ps->mangled_footer = 1;
		break;
	    }

	    if(screen.current->help != NO_HELP){
		helper(screen.current->help, CONFIG_SCREEN_HELP_TITLE, 1);
		ps->mangled_screen = 1;
	    }
	    else
	      q_status_message(SM_ORDER,0,3,"No help yet!");

	    break;


	  case 'n' :				/* next list element */
	  case PF6 :
	  case '\t' :
	  case ctrl('F') :
	  case KEY_RIGHT :
	  case ctrl('N'):			/* down arrow */
	  case KEY_DOWN:
	    for(ctmpa = next_confline(screen.current), i = 1;
		ctmpa && (ctmpa->flags & CF_NOSELECT);
		ctmpa = next_confline(ctmpa), i++)
	      ;

	    if(ctmpa){
		screen.current = ctmpa;
		if(ch == ctrl('N') || ch == KEY_DOWN){
		    for(ctmpa = screen.top_line,
			j = BODY_LINES(ps) - 1 - HS_MARGIN(ps);
			j > 0 && ctmpa && ctmpa != screen.current;
			ctmpa = next_confline(ctmpa), j--)
		      ;

		    if(!j && ctmpa){
			for(i = 0;
			    ctmpa && ctmpa != screen.current;
			    ctmpa = next_confline(ctmpa), i++)
			  ;

			if(i)
			  config_scroll_up(i);
		    }
		}
	    }
	    else
	      q_status_message(SM_ORDER,0,1,"Already at end of screen");

	    break;

	  case 'p' :				/* previous list element */
	  case PF5 :
	  case ctrl('B') :
	  case KEY_LEFT :
	  case ctrl('P') :			/* up arrow */
	  case KEY_UP :
	    ctmpa = screen.current;
	    i = 0;
	    do
	      if(ctmpa == config_top_scroll(ps, screen.top_line))
		i = 1;
	      else if(i)
		i++;
	    while((ctmpa = prev_confline(ctmpa))
		  && (ctmpa->flags&CF_NOSELECT));

	    if(ctmpa){
		screen.current = ctmpa;
		if((ch == ctrl('P') || ch == KEY_UP) && i)
		  config_scroll_down(i);
	    }
	    else
	      q_status_message(SM_ORDER, 0, 1,
			       "Already at start of screen");

	    break;

	  case '+' :				/* page forward */
	  case ' ' :
	  case ctrl('V') :
	  case PF8 :
	  case KEY_PGDN:
	    for(ctmpa = screen.top_line, i = BODY_LINES(ps);
		i > 0 && ctmpa;
		ctmpa = next_confline(ctmpa), i--)
	      ;

	    if(ctmpa){
		ps->mangled_body = 1;
		for(screen.top_line = ctmpa;
		    ctmpa && (ctmpa->flags & CF_NOSELECT);
		    ctmpa = next_confline(ctmpa))
		  ;
	    }
	    else{  /* on last screen */
		for(ctmpa = screen.top_line, i = BODY_LINES(ps) - i - 1;
		    i > 0 && ctmpa;
		    ctmpa = next_confline(ctmpa), i--)
		  ;
		
		/* ctmpa is pointing at last line now */
		if(ctmpa){
		    for(; ctmpa && (ctmpa->flags & CF_NOSELECT);
			ctmpa = prev_confline(ctmpa))
		      ;

		    if(ctmpa == screen.current)
		      q_status_message(SM_ORDER,0,1,
				  "Already at end of screen");
		    else
		      ps->mangled_body = 1;
		}
	    }

	    if(ctmpa)
	      screen.current = ctmpa;

	    break;

	  case '-' :				/* page backward */
	  case ctrl('Y') :
	  case PF7 :
	  case KEY_PGUP:
	    ps->mangled_body = 1;
	    if(!(ctmpa=prev_confline(screen.top_line)))
	      ctmpa = screen.current;

	    for(i = BODY_LINES(ps) - 1;
		i > 0 && prev_confline(ctmpa);
		i--, ctmpa = prev_confline(ctmpa))
	      ;

	    for(screen.top_line = ctmpa;
	        ctmpa && (ctmpa->flags & CF_NOSELECT);
		ctmpa = next_confline(ctmpa))
	      ;

	    if(ctmpa){
		if(ctmpa == screen.current)
		  q_status_message(SM_ORDER, 0, 1,
				 "Already at start of screen");

		screen.current = ctmpa;
	    }

	    break;

#ifdef MOUSE	    
	  case KEY_MOUSE:
	    {   
		MOUSEPRESS mp;

		mouse_get_last (NULL, &mp);
		mp.row -= HEADER_ROWS(ps);
		ctmpa = screen.top_line;
		if (mp.doubleclick) {
		    if(screen.current->tool){
			unsigned flags;

			flags  = screen.current->flags;
			flags |= (changes ? CF_CHANGES : 0);

			switch(i=(*screen.current->tool)(ps, ctrl('M'),
			    &screen.current, flags)){
			  case -1:
			  case 0:
			    break;

			  case 1:
			    changes = 1;
			    break;

			  case 2:
			    retval = changes;
			    done++;
			    break;

			  case 3:
			    retval = 1;
			    done++;
			    break;

			  default:
			    retval = i;
			    done++;
			    break;
			}
		    }
		}
		else {
		    while (mp.row && ctmpa != NULL) {
			--mp.row;
			ctmpa = ctmpa->next;
		    }

		    if (ctmpa != NULL && !(ctmpa->flags & CF_NOSELECT))
		      screen.current = ctmpa;
		}
	    }
	    break;
#endif

	  case 'y' :				/* print screen */
	  case PF11:
	  case PF2 :
	    if((style == NoPrint)
	      || (ch == PF11 && style == PrintConfig)
	      || (ch == PF2 && style != PrintConfig))
	      goto let_tool_handle_it;

	    print_option_screen(&screen, style==Config ? "configuration "
		: style==PrintConfig ? "printer config " : "");
	    break;

	  case 'w' :				/* whereis */
	  case ctrl('W') :
	  case PF12 :
	    /*--- get string  ---*/
	    {int   rc, found = 0;
	     char *result = NULL, buf[64];
	     static char last[64];
	     HelpType help;
	     static ESCKEY_S ekey[] = {
		{0, 0, "", ""},
		{ctrl('Y'), 10, "^Y", "Top"},
		{ctrl('V'), 11, "^V", "Bottom"},
		{-1, 0, NULL, NULL}};

	     ps->mangled_footer = 1;
	     buf[0] = '\0';
	     sprintf(tmp, "Word to find %s%s%s: ",
		     (last[0]) ? "[" : "",
		     (last[0]) ? last : "",
		     (last[0]) ? "]" : "");
	     help = NO_HELP;
	     while(1){
		 rc = optionally_enter(buf,-FOOTER_ROWS(ps),0,63,1,0,
					 tmp,ekey,help,0);
		 if(rc == 3)
		   help = help == NO_HELP ? h_config_whereis : NO_HELP;
		 else if(rc == 0 || rc == 1 || rc == 10 || rc == 11 || !buf[0]){
		     if(rc == 0 && !buf[0] && last[0])
		       strcpy(buf, last);

		     break;
		 }
	     }

	     if(rc == 0 && buf[0]){
		 ch   = KEY_DOWN;
		 ctmpa = screen.current;
		 while(ctmpa = next_confline(ctmpa))
		   if(srchstr(ctmpa->varname, buf)
		      || srchstr(ctmpa->value, buf)){
		       while(ctmpa && (ctmpa->flags & CF_NOSELECT))
			 ctmpa = next_confline(ctmpa);

		       if(ctmpa)
			 found++;

		       break;
		   }

		 if(!found){
		     ctmpa = first_confline(screen.current);

		     while(ctmpa != screen.current)
		       if(srchstr(ctmpa->varname, buf)
			  || srchstr(ctmpa->value, buf)){
			   while(ctmpa && ctmpa != screen.current
				 && (ctmpa->flags & CF_NOSELECT))
			     ctmpa = next_confline(ctmpa);

			   if(ctmpa && ctmpa != screen.current)
			     found++;

			   break;
		       }
		       else
			 ctmpa = next_confline(ctmpa);
		 }
	     }
	     else if(rc == 10){
		 screen.current = first_confline(screen.current);
		 result = "Searched to top";
	     }
	     else if(rc == 11){
		 screen.current = last_confline(screen.current);
		 result = "Searched to bottom";
	     }
	     else
	       result = "WhereIs cancelled";

	     if(found && ctmpa){
		 strcpy(last, buf);
		 result  = "Word found";
		 screen.current = ctmpa;
	     }

	     q_status_message(SM_ORDER,0,3,result ? result : "Word not found");
	    }

	    break;

	  case ctrl('L'):			/* redraw the display */
          case KEY_RESIZE:
	    ClearScreen();
	    ps->mangled_screen = 1;
	    break;

	  let_tool_handle_it:
	  default:
	    if(ps_global->restricted || ps_global->readonly_pinerc){
		q_status_message1(SM_ORDER, 0, 3,
		     "%s can't change options or settings",
		     ps_global->restricted ? "Pine demo"
					   : "Config file not editable,");
		if(ch == 'e' || ch == PF3){
		    retval = 0;
		    done++;
		}
	    }
	    else if(screen.current->tool){
		unsigned flags;

		flags  = screen.current->flags;
		flags |= (changes ? CF_CHANGES : 0);

		switch(i=(*screen.current->tool)(ps, ch,
		    &screen.current, flags)){
		  case -1:
		    q_status_message2(SM_ORDER, 0, 2,
		      "Command \"%s\" not defined here.%s",
		      pretty_command(orig_ch),
		      F_ON(F_BLANK_KEYMENU,ps) ? "" : "  See key menu below.");
		    break;

		  case 0:
		    break;

		  case 1:
		    changes = 1;
		    break;

		  case 2:
		    retval = changes;
		    done++;
		    break;

		  case 3:
		    retval = 1;
		    done++;
		    break;

		  default:
		    retval = i;
		    done++;
		    break;
		}
	    }

	    break;

	  case NO_OP_IDLE:			/* simple timeout */
	  case NO_OP_COMMAND:
	    break;
	}
    }

    for(screen.current = first_confline(screen.current); screen.current;){
	ctmpa = screen.current->next;		/* clean up */
	free_confline(&screen.current);
	screen.current = ctmpa;
    }

    return(retval);
}


/*
 *
 */
int
config_scroll_up(n)
    long n;
{
    CONF_S *ctmp = opt_screen->top_line;
    int     cur_found = 0, rv = 1;

    if(n < 0)
      return(config_scroll_down(-n));
    else if(n){
	while(n-- && (ctmp = next_confline(ctmp)))
	  if(prev_confline(ctmp) == opt_screen->current)
	    cur_found++;

	if(ctmp){
	    opt_screen->top_line = ctmp;
	    rv = ps_global->mangled_body = 1;
	    if(cur_found){
		for(ctmp = opt_screen->top_line;
		    ctmp && (ctmp->flags & CF_NOSELECT);
		    ctmp = next_confline(ctmp))
		  ;

		if(ctmp)
		  opt_screen->current = opt_screen->prev = ctmp;
	    }
	}
	else
	  rv = 0;
    }

    return(rv);
}


/*
 * config_scroll_down -
 */
int
config_scroll_down(n)
    long n;
{
    CONF_S *ctmp = opt_screen->top_line, *last_sel = NULL;
    int     i, rv = 1;

    if(n < 0)
      return(config_scroll_up(-n));
    else if(n){
	while(n-- && (ctmp = prev_confline(ctmp)))
	  ;

	if(ctmp){
	    opt_screen->top_line = ctmp;
	    rv = ps_global->mangled_body = 1;
	    for(ctmp = opt_screen->top_line, i = BODY_LINES(ps_global);
		i > 0 && ctmp && ctmp != opt_screen->current;
		ctmp = next_confline(ctmp), i--)
	      if(!(ctmp->flags & CF_NOSELECT))
		last_sel = ctmp;

	    if(!i && last_sel)
	      opt_screen->current = opt_screen->prev = last_sel;
	}
	else
	  rv = 0;
    }

    return(rv);
}


/*
 * config_scroll_to_pos -
 */
int
config_scroll_to_pos(n)
    long n;
{
    CONF_S *ctmp;

    for(ctmp = first_confline(opt_screen->current);
	n && ctmp && ctmp != opt_screen->top_line;
	ctmp = next_confline(ctmp), n--)
      ;

    if(n == 0)
      while(ctmp && ctmp != opt_screen->top_line)
	if(ctmp = next_confline(ctmp))
	  n--;

    return(config_scroll_up(n));
}


/*
 * config_top_scroll - return pointer to the 
 */
CONF_S *
config_top_scroll(ps, topline)
    struct pine *ps;
    CONF_S *topline;
{
    int     i;
    CONF_S *ctmp;

    for(ctmp = topline, i = HS_MARGIN(ps);
	ctmp && i;
	ctmp = next_confline(ctmp), i--)
      ;

    return(ctmp ? ctmp : topline);
}


/*
 *
 */
HelpType
config_help(var, feature)
    int var, feature;
{
    switch(var){
      case V_FEATURE_LIST :
	switch(feature){
	  case F_ENABLE_FULL_HDR :
	    return(h_config_enable_full_hdr);
	  case F_ENABLE_PIPE :
	    return(h_config_enable_pipe);
	  case F_ENABLE_TAB_COMPLETE :
	    return(h_config_enable_tab_complete);
	  case F_QUIT_WO_CONFIRM :
	    return(h_config_quit_wo_confirm);
	  case F_ENABLE_JUMP :
	    return(h_config_enable_jump);
	  case F_ENABLE_ALT_ED :
	    return(h_config_enable_alt_ed);
	  case F_ENABLE_BOUNCE :
	    return(h_config_enable_bounce);
	  case F_ENABLE_AGG_OPS :
	    return(h_config_enable_agg_ops);
	  case F_ENABLE_FLAG :
	    return(h_config_enable_flag);
	  case F_FLAG_SCREEN_DFLT :
	    return(h_config_flag_screen_default);
	  case F_CAN_SUSPEND :
	    return(h_config_can_suspend);
	  case F_EXPANDED_ADDRBOOKS :
	    return(h_config_expanded_addrbooks);
	  case F_EXPANDED_DISTLISTS :
	    return(h_config_expanded_distlists);
	  case F_FROM_DELIM_IN_PRINT :
	    return(h_config_print_from);
	  case F_EXPANDED_FOLDERS :
	    return(h_config_expanded_folders);
	  case F_USE_FK :
	    return(h_config_use_fk);
	  case F_INCLUDE_HEADER :
	    return(h_config_include_header);
	  case F_SIG_AT_BOTTOM :
	    return(h_config_sig_at_bottom);
	  case F_DEL_SKIPS_DEL :
	    return(h_config_del_skips_del);
	  case F_AUTO_EXPUNGE :
	    return(h_config_auto_expunge);
	  case F_AUTO_READ_MSGS :
	    return(h_config_auto_read_msgs);
	  case F_READ_IN_NEWSRC_ORDER :
	    return(h_config_read_in_newsrc_order);
	  case F_SELECT_WO_CONFIRM :
	    return(h_config_select_wo_confirm);
	  case F_COMPOSE_TO_NEWSGRP :
	    return(h_config_compose_news_wo_conf);
	  case F_USE_CURRENT_DIR :
	    return(h_config_use_current_dir);
	  case F_USE_SENDER_NOT_X :
	    return(h_config_use_sender_not_x);
	  case F_SAVE_WONT_DELETE :
	    return(h_config_save_wont_delete);
	  case F_SAVE_ADVANCES :
	    return(h_config_save_advances);
	  case F_FORCE_LOW_SPEED :
	    return(h_config_force_low_speed);
	  case F_ALT_ED_NOW :
	    return(h_config_alt_ed_now);
	  case F_SHOW_DELAY_CUE :
	    return(h_config_show_delay_cue);
	  case F_DISABLE_CONFIG_SCREEN :
	    return(h_config_disable_config_screen);
	  case F_DISABLE_PASSWORD_CMD :
	    return(h_config_disable_password_cmd);
	  case F_DISABLE_UPDATE_CMD :
	    return(h_config_disable_update_cmd);
	  case F_DISABLE_KBLOCK_CMD :
	    return(h_config_disable_kblock_cmd);
	  case F_QUOTE_ALL_FROMS :
	    return(h_config_quote_all_froms);
	  case F_AUTO_OPEN_NEXT_UNREAD :
	    return(h_config_auto_open_unread);
	  case F_AUTO_INCLUDE_IN_REPLY :
	    return(h_config_auto_include_reply);
	  case F_SELECTED_SHOWN_BOLD :
	    return(h_config_select_in_bold);
	  case F_NO_NEWS_VALIDATION :
	    return(h_config_post_wo_validation);
	  case F_ENABLE_INCOMING :
	    return(h_config_enable_incoming);
	  case F_ATTACHMENTS_IN_REPLY :
	    return(h_config_attach_in_reply);
	  case F_QUELL_LOCAL_LOOKUP :
	    return(h_config_quell_local_lookup);
	  case F_PRESERVE_START_STOP :
	    return(h_config_preserve_start_stop);
	  case F_COMPOSE_REJECTS_UNQUAL:
	    return(h_config_compose_rejects_unqual);
	  case F_FAKE_NEW_IN_NEWS:
	    return(h_config_news_uses_recent);
	  case F_SUSPEND_SPAWNS:
	    return(h_config_suspend_spawns);
	  case F_ENABLE_8BIT:
	    return(h_config_8bit_smtp);
	  case F_ENABLE_8BIT_NNTP:
	    return(h_config_8bit_nntp);
	  case F_COMPOSE_MAPS_DEL:
	    return(h_config_compose_maps_del);
	  case F_BACKGROUND_POST:
	    return(h_config_compose_bg_post);
	  case F_AUTO_ZOOM:
	    return(h_config_auto_zoom);
	  case F_AUTO_UNZOOM:
	    return(h_config_auto_unzoom);
	  case F_DEL_FROM_DOT:
	    return(h_config_del_from_dot);
	  case F_PRINT_INDEX:
	    return(h_config_print_index);
#if !defined(DOS) && !defined(OS2)
	  case F_ALLOW_TALK:
	    return(h_config_allow_talk);
#endif
	  case F_ENABLE_MOUSE:
	    return(h_config_enable_mouse);
	  case F_ENABLE_XTERM_NEWMAIL:
	    return(h_config_enable_xterm_newmail);
	  case F_ENABLE_DOT_FILES:
	    return(h_config_enable_dot_files);
	  case F_ENABLE_DOT_FOLDERS:
	    return(h_config_enable_dot_folders);
	  case F_AGG_PRINT_FF:
	    return(h_config_ff_between_msgs);
	  case F_FIRST_SEND_FILTER_DFLT:
	    return(h_config_send_filter_dflt);
	  case F_CUSTOM_PRINT:
	    return(h_config_custom_print);
	  case F_BLANK_KEYMENU:
	    return(h_config_blank_keymenu);
	  case F_FCC_ON_BOUNCE:
	    return(h_config_fcc_on_bounce);
	  case F_PASS_CONTROL_CHARS:
	    return(h_config_pass_control);
	  case F_SHOW_CURSOR:
	    return(h_config_show_cursor);
	  case F_VERT_FOLDER_LIST:
	    return(h_config_vert_list);
	  case F_VERBOSE_POST:
	    return(h_config_verbose_post);
	  case F_AUTO_REPLY_TO:
	    return(h_config_auto_reply_to);
	  case F_TAB_TO_NEW:
	    return(h_config_tab_new_only);
	  case F_QUELL_DEAD_LETTER:
	    return(h_config_quell_dead_letter);
	  case F_QUELL_BEEPS:
	    return(h_config_quell_beeps);
	  case F_QUELL_LOCK_FAILURE_MSGS:
	    return(h_config_quell_lock_failure_warnings);
	  case F_ENABLE_SPACE_AS_TAB :
	    return(h_config_cruise_mode);
	  case F_ENABLE_TAB_DELETES :
	    return(h_config_cruise_mode_delete);
	  case F_ALLOW_GOTO :
	    return(h_config_allow_goto);
	  default :
	    return(NO_HELP);
        }

	break;

      case V_PERSONAL_NAME :
	return(h_config_pers_name);
      case V_USER_ID :
	return(h_config_user_id);
      case V_USER_DOMAIN :
	return(h_config_user_dom);
      case V_SMTP_SERVER :
	return(h_config_smtp_server);
      case V_NNTP_SERVER :
	return(h_config_nntp_server);
      case V_INBOX_PATH :
	return(h_config_inbox_path);
      case V_FOLDER_SPEC :
	return(h_config_folder_spec);
      case V_PRUNED_FOLDERS :
	return(h_config_pruned_folders);
      case V_NEWS_SPEC :
	return(h_config_news_spec);
      case V_DEFAULT_FCC :
	return(h_config_default_fcc);
      case V_DEFAULT_SAVE_FOLDER :
	return(h_config_def_save_folder);
      case V_POSTPONED_FOLDER :
	return(h_config_postponed_folder);
      case V_READ_MESSAGE_FOLDER :
	return(h_config_read_message_folder);
      case V_ARCHIVED_FOLDERS :
	return(h_config_archived_folders);
      case V_SIGNATURE_FILE :
	return(h_config_signature_file);
      case V_GLOB_ADDRBOOK :
	return(h_config_global_addrbook);
      case V_ADDRESSBOOK :
	return(h_config_addressbook);
      case V_INIT_CMD_LIST :
	return(h_config_init_cmd_list);
      case V_COMP_HDRS :
	return(h_config_comp_hdrs);
      case V_CUSTOM_HDRS :
	return(h_config_custom_hdrs);
      case V_VIEW_HEADERS :
	return(h_config_viewer_headers);
      case V_SAVED_MSG_NAME_RULE :
	return(h_config_saved_msg_name_rule);
      case V_FCC_RULE :
	return(h_config_fcc_rule);
      case V_SORT_KEY :
	return(h_config_sort_key);
      case V_AB_SORT_RULE :
	return(h_config_ab_sort_rule);
      case V_CHAR_SET :
	return(h_config_char_set);
      case V_EDITOR :
	return(h_config_editor);
      case V_SPELLER :
	return(h_config_speller);
      case V_DISPLAY_FILTERS :
	return(h_config_display_filters);
      case V_SEND_FILTER :
	return(h_config_sending_filter);
      case V_ALT_ADDRS :
	return(h_config_alt_addresses);
      case V_ABOOK_FORMATS :
	return(h_config_abook_formats);
      case V_INDEX_FORMAT :
	return(h_config_index_format);
      case V_OVERLAP :
	return(h_config_viewer_overlap);
      case V_MARGIN :
	return(h_config_scroll_margin);
      case V_FILLCOL :
	return(h_config_composer_wrap_column);
      case V_REPLY_STRING :
	return(h_config_reply_indent_string);
      case V_EMPTY_HDR_MSG :
	return(h_config_empty_hdr_msg);
      case V_STATUS_MSG_DELAY :
	return(h_config_status_msg_delay);
      case V_MAILCHECK :
	return(h_config_mailcheck);
      case V_NEWS_ACTIVE_PATH :
	return(h_config_news_active);
      case V_NEWS_SPOOL_DIR :
	return(h_config_news_spool);
      case V_IMAGE_VIEWER :
	return(h_config_image_viewer);
      case V_USE_ONLY_DOMAIN_NAME :
	return(h_config_domain_name);
      case V_LAST_TIME_PRUNE_QUESTION :
	return(h_config_prune_date);
      case V_UPLOAD_CMD:
	return(h_config_upload_cmd);
      case V_UPLOAD_CMD_PREFIX:
	return(h_config_upload_prefix);
      case V_DOWNLOAD_CMD:
	return(h_config_download_cmd);
      case V_DOWNLOAD_CMD_PREFIX:
	return(h_config_download_prefix);
      case V_GOTO_DEFAULT_RULE:
	return(h_config_goto_default);
      case V_MAILCAP_PATH :
	return(h_config_mailcap_path);
      case V_MIMETYPE_PATH :
	return(h_config_mimetype_path);
      case V_NEWSRC_PATH :
	return(h_config_newsrc_path);
#if defined(DOS) || defined(OS2)
      case V_FOLDER_EXTENSION :
	return(h_config_folder_extension);
      case V_NORM_FORE_COLOR :
	return(h_config_normal_fg);
      case V_NORM_BACK_COLOR :
	return(h_config_normal_bg);
      case V_REV_FORE_COLOR :
	return(h_config_reverse_fg);
      case V_REV_BACK_COLOR :
	return(h_config_reverse_bg);
#endif
      default :
	return(NO_HELP);
    }
}


/*
 * simple text variable handler
 *
 * note, things get a little involved due to the
 *	 screen struct <--> variable mapping. (but, once its
 *       running it shouldn't need changing ;).
 * 
 * returns:  -1 on unrecognized cmd, 0 if no change, 1 if change
 *           returns what conf_exit_cmd returns for exit command.
 */
int
text_tool(ps, cmd, cl, flags)
    struct pine  *ps;
    int		  cmd;
    CONF_S      **cl;
    unsigned      flags;
{
    char	     prompt[81], sval[MAXPATH+1], *tmp, **newval = NULL;
    int		     rv = 0, skip_to_next = 0, after = 0, i = 4, j, k;
    int		     lowrange, hirange, incr;
    int		     numval, repeat_key = 0;
    CONF_S	    *ctmp;
    HelpType         help;
    ESCKEY_S         ekey[6];

    if(flags&CF_NUMBER){ /* only happens if !is_list */
	incr = 1;
	if((*cl)->var == &ps->vars[V_FILLCOL]){
	    lowrange = 1;
	    hirange  = MAX_FILLCOL;
	}
	else if((*cl)->var == &ps->vars[V_OVERLAP]
		|| (*cl)->var == &ps->vars[V_MARGIN]){
	    lowrange = 0;
	    hirange  = 20;
	}
	else if((*cl)->var == &ps->vars[V_STATUS_MSG_DELAY]){
	    lowrange = 0;
	    hirange  = 30;
	}
	else if((*cl)->var == &ps->vars[V_MAILCHECK]){
	    lowrange = 0;
	    hirange  = 25000;
	    incr     = 15;
	}

	ekey[0].ch    = -2;
	ekey[0].rval  = 'x';
	ekey[0].name  = "";
	ekey[0].label = "";
	ekey[1].ch    = ctrl('P');
	ekey[1].rval  = ctrl('P');
	ekey[1].name  = "^P";
	ekey[1].label = "Decrease";
	ekey[2].ch    = ctrl('N');
	ekey[2].rval  = ctrl('N');
	ekey[2].name  = "^N";
	ekey[2].label = "Increase";
	ekey[3].ch    = KEY_DOWN;
	ekey[3].rval  = ctrl('P');
	ekey[3].name  = "";
	ekey[3].label = "";
	ekey[4].ch    = KEY_UP;
	ekey[4].rval  = ctrl('N');
	ekey[4].name  = "";
	ekey[4].label = "";
	ekey[5].ch    = -1;
    }

    sval[0] = '\0';
    switch(cmd){
      case 'a' :				/* add to list */
      case PF9 :
	if((*cl)->var->is_fixed){
	    q_status_message(SM_ORDER, 3, 3,
			     "Can't add to sys-admin defined value.");
	}
	else if(!(*cl)->var->is_list && (*cl)->var->user_val.p){
	    q_status_message(SM_ORDER, 3, 3,
			    "Only single value allowed.  Use \"Change\".");
	}
	else{
	    int maxwidth =min(80,ps->ttyo->screen_cols) - 15;
	    char *p;

	    if((*cl)->var->is_list
	       && (*cl)->var->user_val.l
	       && (*cl)->var->user_val.l[0]
	       && (*cl)->var->user_val.l[0][0]
	       && (*cl)->value){
		char tmpval[101];
		/* regular add to an existing list */

		strncpy(tmpval, (*cl)->value, 100);
		removing_trailing_white_space(tmpval);
		/* 33 is the number of chars other than the value */
		k = min(18, max(maxwidth-33,0));
		if(strlen(tmpval) > k && k >= 3){
		    tmpval[k-1] = tmpval[k-2] = tmpval[k-3] = '.';
		    tmpval[k] = '\0';
		}

		sprintf(prompt,
		    "Enter text to insert before \"%.*s\": ",k,tmpval);
	    }
	    else if((*cl)->var->is_list
		    && !(*cl)->var->user_val.l
		    && (*cl)->var->current_val.l){
		/* Add to list which doesn't exist, but default does exist */
		ekey[0].ch    = 'r';
		ekey[0].rval  = 'r';
		ekey[0].name  = "R";
		ekey[0].label = "Replace";
		ekey[1].ch    = 'a';
		ekey[1].rval  = 'a';
		ekey[1].name  = "A";
		ekey[1].label = "Add To";
		ekey[2].ch    = -1;
		strcpy(prompt, "Replace or Add To default value ? ");
		switch(radio_buttons(prompt, -FOOTER_ROWS(ps), ekey, 'a', 'x',
				     h_config_replace_add, RB_NORM)){
		  case 'a':
		    p = sval;
		    for(j = 0; (*cl)->var->current_val.l[j]; j++){
			strcpy(p, (*cl)->var->current_val.l[j]);
			p += strlen(p);
			*p++ = ',';
			*p++ = ' ';
			*p = '\0';
		    }

add_text:
		    sprintf(prompt, "Enter the %stext to be added : ",
			flags&CF_NUMBER ? "numeric " : "");
		    break;
		    
		  case 'r':
replace_text:
		    sprintf(prompt, "Enter the %sreplacement text : ",
			flags&CF_NUMBER ? "numeric " : "");
		    break;
		    
		  case 'x':
		    i = 1;
		    q_status_message(SM_ORDER,0,3,"Add cancelled");
		    break;
		}
	    }
	    else
	      sprintf(prompt, "Enter the %stext to be added : ",
		    flags&CF_NUMBER ? "numeric " : "");

	    ps->mangled_footer = 1;
	    help = NO_HELP;
	    while(1){
		if((*cl)->var->is_list
		    && (*cl)->var->user_val.l
		    && (*cl)->var->user_val.l[0]
		    && (*cl)->var->user_val.l[0][0]
		    && (*cl)->value){
		    ekey[0].ch    = ctrl('W');
		    ekey[0].rval  = 5;
		    ekey[0].name  = "^W";
		    ekey[0].label = after ? "InsertBefore" : "InsertAfter";
		    ekey[1].ch    = -1;
		}
		else if(!(flags&CF_NUMBER))
		  ekey[0].ch    = -1;

		i = optionally_enter(sval, -FOOTER_ROWS(ps), 0, MAXPATH, 1,
			 0, prompt, (ekey[0].ch != -1) ? ekey : NULL, help, 0);
		if(i == 0){
		    rv = ps->mangled_body = 1;
		    removing_leading_white_space(sval);
		    removing_trailing_white_space(sval);
		    /*
		     * Coerce "" and <Empty Value> to empty string input.
		     * Catch <No Value Set> as a substitute for deleting.
		     */
		    if((*sval == '\"' && *(sval+1) == '\"' && *(sval+2) == '\0')
		        || !struncmp(sval, empty_val, EMPTY_VAL_LEN) 
			|| (*sval == '<'
			    && !struncmp(sval+1, empty_val, EMPTY_VAL_LEN)))
		      *sval = '\0';
		    else if(!struncmp(sval, no_val, NO_VAL_LEN)
		        || (*sval == '<'
			    && !struncmp(sval+1, no_val, NO_VAL_LEN)))
		      goto delete;

		    if((*cl)->var->is_list){
			if(*sval || !(*cl)->var->user_val.l){
			    char **ltmp;
			    int    i;

			    i = 0;
			    for(tmp = sval; *tmp; tmp++)
			      if(*tmp == ',')
				i++;	/* conservative count of ,'s */

			    if(!i){
				ltmp    = (char **)fs_get(2 * sizeof(char *));
				ltmp[0] = cpystr(sval);
				ltmp[1] = NULL;
			    }
			    else
			      ltmp = parse_list(sval, i + 1, NULL);

			    if(ltmp[0]){
				config_add_list(ps, cl, ltmp, &newval, after);
				if(after)
				  skip_to_next = 1;
			    }
			    else{
				q_status_message1(SM_ORDER, 0, 3,
					 "Can't add %s to list", empty_val);
				rv = ps->mangled_body = 0;
			    }

			    fs_give((void **)&ltmp);
			}
			else{
			    q_status_message1(SM_ORDER, 0, 3,
					 "Can't add %s to list", empty_val);
			}
		    }
		    else{
			if(flags&CF_NUMBER && sval[0]
			  && !(isdigit((unsigned char)sval[0])
			       || sval[0] == '-' || sval[0] == '+')){
			    q_status_message(SM_ORDER,3,3,
				  "Entry must be numeric");
			    i = 3; /* to keep loop going */
			    continue;
			}

			if((*cl)->var->user_val.p)
			  fs_give((void **)&(*cl)->var->user_val.p);

			(*cl)->var->user_val.p = cpystr(sval);
			newval = &(*cl)->value;
		    }
		}
		else if(i == 1){
		    q_status_message(SM_ORDER,0,3,"Add cancelled");
		}
		else if(i == 3){
		    help = help == NO_HELP ? h_config_add : NO_HELP;
		    continue;
		}
		else if(i == 4){		/* no redraw, yet */
		    continue;
		}
		else if(i == 5){ /* change from/to prepend to/from append */
		    char tmpval[101];

		    after = after ? 0 : 1;
		    strncpy(tmpval, (*cl)->value, 100);
		    removing_trailing_white_space(tmpval);
		    /* 33 is the number of chars other than the value */
		    k = min(18, max(maxwidth-33,0));
		    if(strlen(tmpval) > k && k >= 3){
			tmpval[k-1] = tmpval[k-2] = tmpval[k-3] = '.';
			tmpval[k] = '\0';
		    }

		    sprintf(prompt,
			"Enter text to insert %s \"%.*s\": ",
			after ? "after" : "before", k, tmpval);
		    continue;
		}
		else if(i == ctrl('P')){
		    if(sval[0])
		      numval = atoi(sval);
		    else{
		      if((*cl)->var->current_val.p)
			numval = atoi((*cl)->var->current_val.p);
		      else
			numval = lowrange + 1;
		    }

		    if(numval == lowrange){
			/*
			 * Protect user from repeating arrow key that
			 * causes message to appear over and over.
			 */
			if(++repeat_key > 0){
			    q_status_message1(SM_ORDER,3,3,
				"Minimum value is %s", comatose(lowrange));
			    repeat_key = -5;
			}
		    }
		    else
		      repeat_key = 0;

		    numval = max(numval - incr, lowrange);
		    sprintf(sval, "%d", numval);
		    continue;
		}
		else if(i == ctrl('N')){
		    if(sval[0])
		      numval = atoi(sval);
		    else{
		      if((*cl)->var->current_val.p)
			numval = atoi((*cl)->var->current_val.p);
		      else
			numval = lowrange + 1;
		    }

		    if(numval == hirange){
			if(++repeat_key > 0){
			    q_status_message1(SM_ORDER,3,3,
				"Maximum value is %s", comatose(hirange));
			    repeat_key = -5;
			}
		    }
		    else
		      repeat_key = 0;

		    numval = min(numval + incr, hirange);
		    sprintf(sval, "%d", numval);
		    continue;
		}

		break;
	    }
	}

	break;

      case 'd' :				/* delete */
      case PF10 :
delete:
	if(!(*cl)->var->is_list
	    && !(*cl)->var->user_val.p
	    && (*cl)->var->current_val.p){
	    char pmt[40];

	    sprintf(pmt, "Override default with %s", empty_val2);
	    if(want_to(pmt, 'n', 'n', NO_HELP, 0, 1) == 'y'){
		sval[0] = '\0';
		(*cl)->var->user_val.p = cpystr(sval);
		newval = &(*cl)->value;
		rv = ps->mangled_body = 1;
	    }
	}
	else if((*cl)->var->is_list
		&& !(*cl)->var->user_val.l
		&& (*cl)->var->current_val.l){
	    char pmt[40];

	    sprintf(pmt, "Override default with %s", empty_val2);
	    if(want_to(pmt, 'n', 'n', NO_HELP, 0, 1) == 'y'){
		char **ltmp;

		sval[0] = '\0';
		ltmp    = (char **)fs_get(2 * sizeof(char *));
		ltmp[0] = cpystr(sval);
		ltmp[1] = NULL;
		config_add_list(ps, cl, ltmp, &newval, 0);
		fs_give((void **)&ltmp);
		rv = ps->mangled_body = 1;
	    }
	}
	else if(((*cl)->var->is_list && !(*cl)->var->user_val.l)
		|| (!(*cl)->var->is_list && !(*cl)->var->user_val.p)){
	    q_status_message(SM_ORDER, 0, 3, "No set value to delete");
	}
	else{
	    if((*cl)->var->is_fixed)
	        sprintf(prompt, "Delete (unused) %.30s from %.20s ",
		    (*cl)->var->is_list
		      ? (!*(*cl)->var->user_val.l[(*cl)->varmem])
			  ? empty_val2
			  : (*cl)->var->user_val.l[(*cl)->varmem]
		      : ((*cl)->var->user_val.p)
			  ? (!*(*cl)->var->user_val.p)
			      ? empty_val2
			      : (*cl)->var->user_val.p
		 	  : "<NULL VALUE>",
		    (*cl)->var->name);
	    else
	        sprintf(prompt, "Really delete %s%.20s from %.30s ",
		    (*cl)->var->is_list ? "item " : "", 
		    (*cl)->var->is_list
		      ? int2string((*cl)->varmem + 1)
		      : ((*cl)->var->user_val.p)
			  ? (!*(*cl)->var->user_val.p)
			      ? empty_val2
			      : (*cl)->var->user_val.p
		 	  : "<NULL VALUE>",
		    (*cl)->var->name);

	    ps->mangled_footer = 1;
	    if(want_to(prompt, 'n', 'n', NO_HELP, 0, 1) == 'y'){
		rv = ps->mangled_body = 1;
		if((*cl)->var->is_list){
		    fs_give((void **)&(*cl)->var->user_val.l[(*cl)->varmem]);
		    config_del_list_item(cl, &newval);
		}
		else{
		    fs_give((void **)&(*cl)->var->user_val.p);
		    newval = &(*cl)->value;
		}
	    }
	    else
	      q_status_message(SM_ORDER, 0, 3, "Value not deleted");
	}

	break;

      case ctrl('M') :
      case ctrl('J') :
      case 'c' :				/* edit/change list option */
      case PF4 :
	if((*cl)->var->is_fixed){
	    q_status_message(SM_ORDER, 3, 3,
			     "Can't change sys-admin defined value.");
	}
	else if(((*cl)->var->is_list
		    && !(*cl)->var->user_val.l
		    && (*cl)->var->current_val.l)
		||
		(!(*cl)->var->is_list
		    && !(*cl)->var->user_val.p
		    && (*cl)->var->current_val.p)){
	    goto replace_text;
	}
	else if(((*cl)->var->is_list
		    && !(*cl)->var->user_val.l
		    && !(*cl)->var->current_val.l)
		||
		(!(*cl)->var->is_list
		    && !(*cl)->var->user_val.p
		    && !(*cl)->var->current_val.p)){
	    goto add_text;
	}
	else{
	    HelpType help;

	    if((*cl)->var->is_list){
		sprintf(prompt, "Change field %.30s list entry : ",
			(*cl)->var->name);
		sprintf(sval, "%s",
			(*cl)->var->user_val.l[(*cl)->varmem]
			  ? (*cl)->var->user_val.l[(*cl)->varmem] : "");
	    }
	    else{
		sprintf(prompt, "Change %sfield %.35s value : ",
			flags&CF_NUMBER ? "numeric " : "",
			(*cl)->var->name);
		sprintf(sval, "%s", (*cl)->var->user_val.p
				     ? (*cl)->var->user_val.p : "");
	    }

	    ps->mangled_footer = 1;
	    help = NO_HELP;
	    while(1){
		if(!(flags&CF_NUMBER))
		  ekey[0].ch = -1;

		i = optionally_enter(sval, -FOOTER_ROWS(ps), 0, MAXPATH, 1,
			 0, prompt, (ekey[0].ch != -1) ? ekey : NULL, help, 0);
		if(i == 0){
		    removing_leading_white_space(sval);
		    removing_trailing_white_space(sval);
		    /*
		     * Coerce "" and <Empty Value> to empty string input.
		     * Catch <No Value Set> as a substitute for deleting.
		     */
		    if((*sval == '\"' && *(sval+1) == '\"' && *(sval+2) == '\0')
		        || !struncmp(sval, empty_val, EMPTY_VAL_LEN) 
			|| (*sval == '<'
			    && !struncmp(sval+1, empty_val, EMPTY_VAL_LEN)))
		      *sval = '\0';
		    else if(!struncmp(sval, no_val, NO_VAL_LEN)
			|| (*sval == '<'
			    && !struncmp(sval+1, no_val, NO_VAL_LEN)))
		      goto delete;

		    rv = ps->mangled_body = 1;
		    if((*cl)->var->is_list){
			char **ltmp = NULL;
			int    i;

			if((*cl)->var->user_val.l[(*cl)->varmem])
			  fs_give((void **)&(*cl)->var->user_val.l[
							       (*cl)->varmem]);

			i = 0;
			for(tmp = sval; *tmp; tmp++)
			  if(*tmp == ',')
			    i++;	/* conservative count of ,'s */

			if(i)
			  ltmp = parse_list(sval, i + 1, NULL);

			if(!i || (ltmp && !ltmp[1])){	/* only one item */
			    (*cl)->var->user_val.l[(*cl)->varmem] =
								  cpystr(sval);
			    newval = &(*cl)->value;

			    if(ltmp && ltmp[0])
			      fs_give((void **)&ltmp[0]);
			}
			else if(ltmp){
			    /*
			     * Looks like the value was changed to a 
			     * list, so delete old value, and insert
			     * new list...
			     *
			     * If more than one item in existing list and
			     * current is end of existing list, then we
			     * have to delete and append instead of
			     * deleting and prepending.
			     */
			    if(((*cl)->varmem > 0 || (*cl)->var->user_val.l[1])
			       && !((*cl)->var->user_val.l[(*cl)->varmem+1])){
				after = 1;
				skip_to_next = 1;
			    }

			    config_del_list_item(cl, &newval);
			    config_add_list(ps, cl, ltmp, &newval, after);
			}

			if(ltmp)
			  fs_give((void **)&ltmp);
		    }
		    else{
			if(flags&CF_NUMBER && sval[0]
			  && !(isdigit((unsigned char)sval[0])
			       || sval[0] == '-' || sval[0] == '+')){
			    q_status_message(SM_ORDER,3,3,
				  "Entry must be numeric");
			    continue;
			}

			if((*cl)->var->user_val.p)
			  fs_give((void **)&(*cl)->var->user_val.p);

			if(sval[0])
			  (*cl)->var->user_val.p = cpystr(sval);

			newval = &(*cl)->value;
		    }
		}
		else if(i == 1){
		    q_status_message(SM_ORDER,0,3,"Change cancelled");
		}
		else if(i == 3){
		    help = help == NO_HELP ? h_config_change : NO_HELP;
		    continue;
		}
		else if(i == 4){		/* no redraw, yet */
		    continue;
		}
		else if(i == ctrl('P')){
		    numval = atoi(sval);
		    if(numval == lowrange){
			/*
			 * Protect user from repeating arrow key that
			 * causes message to appear over and over.
			 */
			if(++repeat_key > 0){
			    q_status_message1(SM_ORDER,3,3,
				"Minimum value is %s", comatose(lowrange));
			    repeat_key = -5;
			}
		    }
		    else
		      repeat_key = 0;

		    numval = max(numval - incr, lowrange);
		    sprintf(sval, "%d", numval);
		    continue;
		}
		else if(i == ctrl('N')){
		    numval = atoi(sval);
		    if(numval == hirange){
			if(++repeat_key > 0){
			    q_status_message1(SM_ORDER,3,3,
				"Maximum value is %s", comatose(hirange));
			    repeat_key = -5;
			}
		    }
		    else
		      repeat_key = 0;

		    numval = min(numval + incr, hirange);
		    sprintf(sval, "%d", numval);
		    continue;
		}

		break;
	    }
	}

	break;

      case 'e' :				/* exit */
      case PF3 :
	rv = config_exit_cmd(flags);
	break;

      default :
	rv = -1;
	break;
    }

    if(skip_to_next)
      *cl = next_confline(*cl);

    /*
     * At this point, if changes occurred, var->user_val.X is set.
     * So, fix the current_val, and handle special cases...
     *
     * NOTE: we don't worry about the "fixed variable" case here, because
     *       editing such vars should have been prevented above...
     */
    if(rv == 1){
	/*
	 * Now go and set the current_val based on user_val changes
	 * above.  Turn off command line settings...
	 */
	set_current_val((*cl)->var, TRUE, FALSE);
	fix_side_effects(ps, (*cl)->var, 0);

	/*
	 * Delay setting the displayed value until "var.current_val" is set
	 * in case current val get's changed due to a special case above.
	 */
	if(newval){
	    if(*newval)
	      fs_give((void **)newval);

	    *newval = pretty_value(ps, *cl);
	}
    }

    return(rv);
}


int
config_exit_cmd(flags)
    unsigned flags;
{
    return(screen_exit_cmd(flags, "Configuration"));
}


flag_exit_cmd(flags)
    unsigned flags;
{
    return(2);
}


/*
 * screen_exit_cmd - basic config/flag screen exit logic
 */
int
screen_exit_cmd(flags, cmd)
    unsigned  flags;
    char     *cmd;
{
    if(flags & CF_CHANGES){
      switch(want_to(EXIT_PMT, 'y', 'x', h_config_undo, 0, 1)){
	case 'y':
	  q_status_message1(SM_ORDER,0,3,"%s changes saved", cmd);
	  return(2);

	case 'n':
	  q_status_message1(SM_ORDER,3,5,"No %s changes saved", cmd);
	  return(10);

	case 'x':  /* ^C */
	  q_status_message(SM_ORDER,3,5,"Changes not yet saved");
	  return(0);
      }
    }
    else
      return(2);
}


/*
 *
 */
void
config_add_list(ps, cl, ltmp, newval, after)
    struct pine *ps;
    CONF_S     **cl;
    char       **ltmp, ***newval;
    int		 after;
{
    int	    items, i;
    char   *tmp;
    CONF_S *ctmp;

    for(items = 0, i = 0; ltmp[i]; i++)		/* count list items */
      items++;

    if((*cl)->var->user_val.l){
	if((*cl)->var->user_val.l[0]
	   && (*cl)->var->user_val.l[0][0]){
	    /*
	     * Since we were already a list, make room
	     * for the new member[s] and fall thru to
	     * actually fill them in below...
	     */
	    for(i = 0; (*cl)->var->user_val.l[i]; i++)
	      ;

	    fs_resize((void **)&(*cl)->var->user_val.l,
		      (i + items + 1) * sizeof(char *));
	    /*
	     * move the ones that will be bumped down to the bottom of the list
	     */
	    for(; i >= (*cl)->varmem + (after?1:0); i--)
	      (*cl)->var->user_val.l[i+items] =
		(*cl)->var->user_val.l[i];

	    i = 0;
	}
	else{
	    (*cl)->varmem = 0;
	    if((*cl)->var->user_val.l[0])
	      fs_give((void **)&(*cl)->var->user_val.l[0]);

	    (*cl)->var->user_val.l[0] = ltmp[0];
	    *newval = &(*cl)->value;
	    if((*cl)->value)
	      fs_give((void **)&(*cl)->value);

	    i = 1;
	}
    }
    else{
	/*
	 * since we were previously empty, we want
	 * to replace the first CONF_S's value with
	 * the first new value, and fill the other
	 * in below if there's a list...
	 */
	(*cl)->var->user_val.l = (char **)fs_get((items+1)*sizeof(char *));
	memset((void *)(*cl)->var->user_val.l, 0, (items+1) * sizeof(char *));
	(*cl)->var->user_val.l[(*cl)->varmem=0] = ltmp[0];
	*newval = &(*cl)->value;
	if((*cl)->value)
	  fs_give((void **)&(*cl)->value);

	i = 1;
    }

    /*
     * Make new cl's to fit in the new space.  Move the value from the current
     * line if inserting before it, else leave it where it is.
     */
    for(; i < items ; i++){
	(*cl)->var->user_val.l[i+(*cl)->varmem + (after?1:0)] = ltmp[i];
	tmp = (*cl)->value;
	new_confline(cl);
	if(after)
	  (*cl)->value   = NULL;
	else
	  (*cl)->value   = tmp;

	(*cl)->var       = (*cl)->prev->var;
	(*cl)->valoffset = (*cl)->prev->valoffset;
	(*cl)->varoffset = (*cl)->prev->varoffset;
	(*cl)->headingp  = (*cl)->prev->headingp;
	(*cl)->keymenu   = (*cl)->prev->keymenu;
	(*cl)->help      = (*cl)->prev->help;
	(*cl)->tool      = (*cl)->prev->tool;
	(*cl)->varnamep  = (*cl)->prev->varnamep;
	*cl		 = (*cl)->prev;
	if(!after)
	  (*cl)->value   = NULL;

	if(after)
	  *newval	 = &(*cl)->next->value;
	else
	  *newval	 = &(*cl)->value;
    }

    /*
     * now fix up varmem values and fill in new values that have been
     * left NULL
     */
    for(ctmp = (*cl)->varnamep, i = 0;
	(*cl)->var->user_val.l[i];
	ctmp = ctmp->next, i++){
	ctmp->varmem = i;
	if(!ctmp->value)
	  ctmp->value = pretty_value(ps, ctmp);
    }
}


/*
 *
 */
void
config_del_list_item(cl, newval)
    CONF_S  **cl;
    char   ***newval;
{
    char   **bufp;
    int	     i;
    CONF_S  *ctmp;

    if((*cl)->var->user_val.l[(*cl)->varmem + 1]){
	for(bufp = &(*cl)->var->user_val.l[(*cl)->varmem];
	    *bufp = *(bufp+1); bufp++)
	  ;

	if(*cl == (*cl)->varnamep){		/* leading value */
	    if((*cl)->value)
	      fs_give((void **)&(*cl)->value);

	    ctmp = (*cl)->next;
	    (*cl)->value = ctmp->value;
	    ctmp->value  = NULL;
	}
	else{
	    ctmp = *cl;			/* blast the confline */
	    *cl = (*cl)->next;
	    if(ctmp == opt_screen->top_line)
	      opt_screen->top_line = *cl;
	}

	free_confline(&ctmp);

	for(ctmp = (*cl)->varnamep, i = 0;	/* now fix up varmem values */
	    (*cl)->var->user_val.l[i];
	    ctmp = ctmp->next, i++)
	  ctmp->varmem = i;
    }
    else if((*cl)->varmem){			/* blasted last in list */
	ctmp = *cl;
	*cl = (*cl)->prev;
	if(ctmp == opt_screen->top_line)
	  opt_screen->top_line = *cl;

	free_confline(&ctmp);
    }
    else{					/* blasted last remaining */
	fs_give((void **)&(*cl)->var->user_val.l);
	*newval = &(*cl)->value;
    }
}


/*
 * feature list manipulation tool
 * 
 * 
 * returns:  -1 on unrecognized cmd, 0 if no change, 1 if change
 */
int
checkbox_tool(ps, cmd, cl, flags)
    struct pine  *ps;
    int		  cmd;
    CONF_S	**cl;
    unsigned      flags;
{
    int  rv = 0;

    switch(cmd){
      case ctrl('M') :
      case ctrl('J') :
      case 'x' :				/* mark/unmark feature */
      case PF4 :
	if((*cl)->var == &ps->vars[V_FEATURE_LIST]){
	    rv = 1;
	    toggle_feature_bit(ps, (*cl)->varmem, (*cl)->var, (*cl)->value);
	}
	else
	  q_status_message(SM_ORDER | SM_DING, 3, 6,
			   "Programmer botch!  Unknown checkbox type.");

	break;

      case 'e' :				/* exit */
      case PF3 :
	rv = config_exit_cmd(flags);
	break;

      default :
	rv = -1;
	break;
    }

    return(rv);
}


/*
 * Message flag manipulation tool
 * 
 * 
 * returns:  -1 on unrecognized cmd, 0 if no change, 1 if change
 */
int
flag_checkbox_tool(ps, cmd, cl, flags)
    struct pine  *ps;
    int		  cmd;
    CONF_S	**cl;
    unsigned      flags;
{
    int  rv = 0, state;

    switch(cmd){
      case ctrl('M') :
      case ctrl('J') :
      case 'x' :				/* mark/unmark feature */
      case PF4 :
	state = ((struct flag_table *)(*cl)->scrap)->set;
	state = (state == 1)
		  ? 0
		  : (state == 0 && (((struct flag_table *)(*cl)->scrap)->ukn))
		      ? 2 : 1;
	(*cl)->value[1] = (state == 0) ? ' ' : ((state == 1) ? 'X': '?');
	((struct flag_table *)(*cl)->scrap)->set = state;
	rv = 1;
	break;

      case 'e' :				/* exit */
      case PF3 :
	rv = flag_exit_cmd(flags);
	break;

      default :
	rv = -1;
	break;
    }

    return(rv);
}


/*
 * simple radio-button style variable handler
 */
int
radiobutton_tool(ps, cmd, cl, flags)
    struct pine  *ps;
    int	          cmd;
    CONF_S      **cl;
    unsigned      flags;
{
    int	       rv = 0;
    CONF_S    *ctmp;

    if((*cl)->var->is_fixed
       && (cmd == ctrl('M') || cmd == ctrl('J') || cmd == '*' || cmd == PF4)){
	q_status_message(SM_ORDER, 3, 3,
			 "Can't change sys-admin defined value.");
	if((*cl)->var->user_val.p){
	    if(want_to("Delete old unused personal option setting", 'y', 'n',
		        NO_HELP, 0, 1) == 'y'){
		fs_give((void **)&(*cl)->var->user_val.p);
		q_status_message(SM_ORDER, 0, 3, "Deleted");
		rv = 1;
	    }
	}
	return(rv);
    }

    switch(cmd){
      case ctrl('M') :
      case ctrl('J') :
      case '*' :				/* set/unset feature */
      case PF4 :
	/* hunt backwards, turning off old values */
	for(ctmp = *cl; ctmp && !(ctmp->flags & CF_NOSELECT) && !ctmp->varname;
	    ctmp = prev_confline(ctmp))
	  ctmp->value[1] = ' ';

	/* hunt forwards, turning off old values */
	for(ctmp = *cl; ctmp && !ctmp->varname; ctmp = next_confline(ctmp))
	  ctmp->value[1] = ' ';

	/* turn on current value */
	(*cl)->value[1] = R_SELD;

	if((*cl)->var == &ps->vars[V_SAVED_MSG_NAME_RULE]
	   || (*cl)->var == &ps->vars[V_FCC_RULE]
	   || (*cl)->var == &ps->vars[V_GOTO_DEFAULT_RULE]
	   || (*cl)->var == &ps->vars[V_AB_SORT_RULE]){
	    NAMEVAL_S *rule;

	    if((*cl)->var == &ps->vars[V_SAVED_MSG_NAME_RULE]){
		rule		  = save_msg_rules((*cl)->varmem);
		ps->save_msg_rule = rule->value;
	    }
	    else if((*cl)->var == &ps->vars[V_FCC_RULE]){
		rule	     = fcc_rules((*cl)->varmem);
		ps->fcc_rule = rule->value;
	    }
	    else if((*cl)->var == &ps->vars[V_GOTO_DEFAULT_RULE]){
		rule		      = goto_rules((*cl)->varmem);
		ps->goto_default_rule = rule->value;
	    }
	    else{
		rule	         = ab_sort_rules((*cl)->varmem);
		ps->ab_sort_rule = rule->value;
		addrbook_reset();
	    }

	    if((*cl)->var->user_val.p)
	      fs_give((void **)&(*cl)->var->user_val.p);

	    (*cl)->var->user_val.p = cpystr(rule->name);

	    ps->mangled_body = 1;	/* BUG: redraw it all for now? */
	    rv = 1;
	}
	else if((*cl)->var == &ps->vars[V_SORT_KEY]){
	    ps->def_sort_rev  = (*cl)->varmem >= (short) EndofList;
	    ps->def_sort      = (SortOrder) ((*cl)->varmem - (ps->def_sort_rev
								 * EndofList));
	    if((*cl)->var->user_val.p)
	      fs_give((void **)&(*cl)->var->user_val.p);

	    sprintf(tmp_20k_buf, "%s%s%s", sort_name(ps->def_sort),
		    (ps->def_sort_rev) ? "/" : "",
		    (ps->def_sort_rev) ? "Reverse" : "");

	    (*cl)->var->user_val.p = cpystr(tmp_20k_buf);

	    ps->mangled_body = 1;	/* BUG: redraw it all for now? */
	    rv = 1;
	}
#if defined(DOS) || defined(OS2)
	else if((*cl)->var == &ps->vars[V_NORM_FORE_COLOR]
		|| (*cl)->var == &ps->vars[V_NORM_BACK_COLOR]
		|| (*cl)->var == &ps->vars[V_REV_FORE_COLOR]
		|| (*cl)->var == &ps->vars[V_REV_BACK_COLOR]){
	    if((*cl)->var->user_val.p)
	      fs_give((void **)&(*cl)->var->user_val.p);

	    (*cl)->var->user_val.p = cpystr(config_colors[(*cl)->varmem]);

	    if((*cl)->var == &ps->vars[V_NORM_FORE_COLOR])
	      pico_nfcolor((*cl)->var->user_val.p);
	    else if((*cl)->var == &ps->vars[V_NORM_BACK_COLOR])
	      pico_nbcolor((*cl)->var->user_val.p);
	    else if((*cl)->var == &ps->vars[V_REV_FORE_COLOR])
	      pico_rfcolor((*cl)->var->user_val.p);
	    else
	      pico_rbcolor((*cl)->var->user_val.p);
	    
	    ps->mangled_screen = 1;
	    rv = 1;
	}
#endif
	else
	  q_status_message(SM_ORDER | SM_DING, 3, 6,
			   "Programmer botch!  Unknown radiobutton type.");

	break;

      case 'e' :				/* exit */
      case PF3 :
	rv = config_exit_cmd(flags);
	break;

      default :
	rv = -1;
	break;
    }

    return(rv);
}



/*
 * simple yes/no style variable handler
 */
int
yesno_tool(ps, cmd, cl, flags)
    struct pine  *ps;
    int	          cmd;
    CONF_S      **cl;
    unsigned      flags;
{
    int  rv = 0;

    if((*cl)->var->is_fixed
       && (cmd == ctrl('M') || cmd == ctrl('J') || cmd == 'c' || cmd == PF4)){
	q_status_message(SM_ORDER, 3, 3,
			 "Can't change sys-admin defined value.");
	if((*cl)->var->user_val.p){
	    if(want_to("Delete old unused personal option setting", 'y', 'n',
		        NO_HELP, 0, 1) == 'y'){
		fs_give((void **)&(*cl)->var->user_val.p);
		q_status_message(SM_ORDER, 0, 3, "Deleted");
		rv = 1;
	    }
	}
	return(rv);
    }

    switch(cmd){
      case ctrl('M') :
      case ctrl('J') :
      case 'c' :				/* toggle yes to no and back */
      case PF4 :
	rv = 1;
	fs_give((void **)&(*cl)->value);
	if((*cl)->var->user_val.p)
	  fs_give((void **)&(*cl)->var->user_val.p);

	if((*cl)->var->user_val.p && !strucmp((*cl)->var->user_val.p, "yes")
	   || (!(*cl)->var->user_val.p && (*cl)->var->current_val.p
	       && !strucmp((*cl)->var->current_val.p, "yes")))
	  (*cl)->var->user_val.p = cpystr("No");
	else
	  (*cl)->var->user_val.p = cpystr("Yes");

	sprintf(tmp_20k_buf, "%-*s", ps->ttyo->screen_cols - (*cl)->valoffset,
		(*cl)->var->user_val.p);

	(*cl)->value = cpystr(tmp_20k_buf);

	if((*cl)->var == &ps->vars[V_USE_ONLY_DOMAIN_NAME]){
	    set_current_val((*cl)->var, FALSE, FALSE);
	    init_hostname(ps);
	}

	break;

      case 'e' :				/* exit */
      case PF3 :
	rv = config_exit_cmd(flags);
	break;

      default :
	rv = -1;
	break;
    }

    return(rv);
}


int
print_select_tool(ps, cmd, cl, flags)
    struct pine *ps;
    int          cmd;
    CONF_S     **cl;
    unsigned     flags;
{
    int rc, i, retval;
    char *p;
    struct variable *vtmp;

    switch(cmd){
      case 'e':
      case PF3:
        retval = config_exit_cmd(flags);
	break;

      case 's':
      case PF4:
      case ctrl('M'):
      case ctrl('J'):
	if(cl && *cl){
	    if((*cl)->var){
		vtmp = (*cl)->var;
		i = vtmp->current_val.l
		    && vtmp->current_val.l[(*cl)->varmem]
		    && vtmp->current_val.l[(*cl)->varmem][0];
		rc = set_variable(V_PRINTER,
			vtmp->current_val.l
			  ? vtmp->current_val.l[(*cl)->varmem] : NULL, 1);
		if(rc == 0){
		    if(vtmp == &ps->vars[V_STANDARD_PRINTER])
		      ps->printer_category = 2;
		    else if(vtmp == &ps->vars[V_PERSONAL_PRINT_COMMAND])
		      ps->printer_category = 3;

		    set_variable(V_PERSONAL_PRINT_CATEGORY, 
			comatose(ps->printer_category), 0);

		    p = NULL;
		    if(i){
			char *nick, *q;

			parse_printer(vtmp->current_val.l[(*cl)->varmem],
			    &nick, &q, NULL, NULL, NULL, NULL);
			p = cpystr(*nick ? nick : q);
			fs_give((void **)&nick);
			fs_give((void **)&q);
		    }

		    q_status_message3(SM_ORDER,0,3, "Default printer %s%s%s",
			p ? "set to \"" : "unset", p ? p : "", p ? "\"" : ""); 

		    if(p)
		      fs_give((void **)&p);
		}
		else
		  q_status_message(SM_ORDER,3,5,
			"Trouble setting default printer");

		retval = 1;
	    }
	    else if(!strcmp((*cl)->value,ANSI_PRINTER)){
		rc = set_variable(V_PRINTER, ANSI_PRINTER, 1);
		if(rc == 0){
		    ps->printer_category = 1;
		    set_variable(V_PERSONAL_PRINT_CATEGORY, 
			comatose(ps->printer_category), 0);
		    q_status_message1(SM_ORDER,0,3,
			"Default printer set to \"%s\"", ANSI_PRINTER);
		}
		else
		  q_status_message(SM_ORDER,3,5,
			"Trouble setting default printer");

		retval = 1;
	    }
	    else{
		char aname[100];

		strcat(strcpy(aname, ANSI_PRINTER), no_ff);
		if(!strcmp((*cl)->value,aname)){
		    rc = set_variable(V_PRINTER, aname, 1);
		    if(rc == 0){
			ps->printer_category = 1;
			set_variable(V_PERSONAL_PRINT_CATEGORY, 
			    comatose(ps->printer_category), 0);
			q_status_message1(SM_ORDER,0,3,
			    "Default printer set to \"%s\"", aname);
		    }
		    else
		      q_status_message(SM_ORDER,3,5,
			    "Trouble setting default printer");

		    retval = 1;
		}
		else
		  retval = 0;
	    }
	}
	else
	  retval = 0;

	if(retval){
	    ps->mangled_body = 1;	/* BUG: redraw it all for now? */
	    set_def_printer_value(ps->VAR_PRINTER);
	}

	break;

      default:
	retval = -1;
	break;
    }

    return(retval);
}


int
print_edit_tool(ps, cmd, cl, flags)
    struct pine *ps;
    int          cmd;
    CONF_S     **cl;
    unsigned     flags;
{
    char	     prompt[81], sval[MAXPATH+1], name[MAXPATH+1];
    char            *nick, *p, *tmp, **newval = NULL;
    int		     rv = 0, skip_to_next = 0, after = 0, i = 4, j, k;
    int		     changing_selected = 0;
    CONF_S	    *ctmp;
    HelpType         help;
    ESCKEY_S         ekey[6];

    if(cmd == 's' || cmd == PF4 || cmd == ctrl('M') || cmd == ctrl('J'))
      return(print_select_tool(ps, cmd, cl, flags));

    if(!(cl && *cl && (*cl)->var))
      return(0);

    switch(cmd){
      case 'a':					/* add to list */
      case PF9:
	sval[0] = '\0';
	if((*cl)->var->is_fixed)
	  q_status_message(SM_ORDER, 3, 3,
			     "Can't add to sys-admin defined value.");
	else{
	    int maxwidth = min(80,ps->ttyo->screen_cols) - 15;

	    if((*cl)->var->user_val.l && (*cl)->value){
		strcpy(prompt, "Enter printer name : ");
	    }
	    else if(!(*cl)->var->user_val.l && (*cl)->var->current_val.l){
		/* Add to list which doesn't exist, but default does exist */
		ekey[0].ch    = 'r';
		ekey[0].rval  = 'r';
		ekey[0].name  = "R";
		ekey[0].label = "Replace";
		ekey[1].ch    = 'a';
		ekey[1].rval  = 'a';
		ekey[1].name  = "A";
		ekey[1].label = "Add To";
		ekey[2].ch    = -1;
		strcpy(prompt, "Replace or Add To default value ? ");
		switch(i = radio_buttons(prompt, -FOOTER_ROWS(ps), ekey, 'a',
					 'x', h_config_replace_add, RB_NORM)){
		  case 'a':
		    p = sval;
		    for(j = 0; (*cl)->var->current_val.l[j]; j++){
			strcpy(p, (*cl)->var->current_val.l[j]);
			p += strlen(p);
			*p++ = ',';
			*p++ = ' ';
			*p = '\0';
		    }

add_text:
		    strcpy(prompt, "Enter name of printer to be added : ");
		    break;
		    
		  case 'r':
replace_text:
		    strcpy(prompt,
			"Enter the name for replacement printer : ");
		    break;
		    
		  case 'x':
		    q_status_message(SM_ORDER,0,3,"Add cancelled");
		    break;
		}

		if(i == 'x')
		  break;
	    }
	    else
	      strcpy(prompt, "Enter name of printer to be added : ");

	    ps->mangled_footer = 1;
	    help = NO_HELP;

	    name[0] = '\0';
	    i = 2;
	    while(i != 0 && i != 1){
		if((*cl)->var->user_val.l && (*cl)->value){
		    ekey[0].ch    = ctrl('W');
		    ekey[0].rval  = 5;
		    ekey[0].name  = "^W";
		    ekey[0].label = after ? "InsertBefore" : "InsertAfter";
		    ekey[1].ch    = -1;
		}
		else
		  ekey[0].ch    = -1;

		i = optionally_enter(name, -FOOTER_ROWS(ps), 0, MAXPATH, 1,
			 0, prompt, (ekey[0].ch != -1) ? ekey : NULL, help, 0);
		if(i == 0){
		    rv = ps->mangled_body = 1;
		    removing_leading_white_space(name);
		    removing_trailing_white_space(name);
		}
		else if(i == 1){
		    q_status_message(SM_ORDER,0,3,"Add cancelled");
		}
		else if(i == 3){
		    help = (help == NO_HELP) ? h_config_insert_after : NO_HELP;
		}
		else if(i == 4){		/* no redraw, yet */
		}
		else if(i == 5){ /* change from/to prepend to/from append */
		    after = after ? 0 : 1;
		}
	    }

	    if(i == 0)
	      i = 2;

#ifdef OS2
	    strcpy(prompt, "Enter port or |command : ");
#else
	    strcpy(prompt, "Enter command for printer : ");
#endif
	    while(i != 0 && i != 1){
		i = optionally_enter(sval, -FOOTER_ROWS(ps), 0, MAXPATH, 1,
			 0, prompt, (ekey[0].ch != -1) ? ekey : NULL, help, 0);
		if(i == 0){
		    rv = ps->mangled_body = 1;
		    removing_leading_white_space(sval);
		    removing_trailing_white_space(sval);
		    if(*sval || !(*cl)->var->user_val.l){
			char **ltmp;

			for(tmp = sval; *tmp; tmp++)
			  if(*tmp == ',')
			    i++;	/* conservative count of ,'s */

			if(!i){	/* only one item */
			    ltmp    = (char **)fs_get(2 * sizeof(char *));
			    ltmp[1] = NULL;
			    if(*name){
				ltmp[0] = (char *)fs_get(strlen(name) + 1
						+ 2 + 1 + strlen(sval) + 1);
				sprintf(ltmp[0], "%s [] %s", name, sval);
			    }
			    else
			      ltmp[0] = cpystr(sval);
			}
			else{
			    /*
			     * Don't allow input of multiple entries at once.
			     */
			    q_status_message(SM_ORDER,3,5,
				"No commas allowed in command");
			    i = 2;
			    continue;
			}

			config_add_list(ps, cl, ltmp, &newval, after);
			if(after)
			  skip_to_next = 1;

			fs_give((void **)&ltmp);
		    }
		    else
		      q_status_message1(SM_ORDER, 0, 3,
					 "Can't add %s to list", empty_val);
		}
		else if(i == 1){
		    q_status_message(SM_ORDER,0,3,"Add cancelled");
		}
		else if(i == 3){
		    help = help == NO_HELP ? h_config_print_cmd : NO_HELP;
		}
		else if(i == 4){		/* no redraw, yet */
		}
		else if(i == 5){ /* change from/to prepend to/from append */
		    after = after ? 0 : 1;
		}
	    }
	}

	break;

      case 'd':					/* delete */
      case PF10:
	if((*cl)->var->current_val.l
	  && (*cl)->var->current_val.l[(*cl)->varmem]
	  && !strucmp(ps->VAR_PRINTER,(*cl)->var->current_val.l[(*cl)->varmem]))
	    changing_selected = 1;

	if(!(*cl)->var->user_val.l && (*cl)->var->current_val.l){
	    char pmt[40];

	    sprintf(pmt, "Override default with %s", empty_val2);
	    if(want_to(pmt, 'n', 'n', NO_HELP, 0, 1) == 'y'){
		char **ltmp;

		sval[0] = '\0';
		ltmp    = (char **)fs_get(2 * sizeof(char *));
		ltmp[0] = cpystr(sval);
		ltmp[1] = NULL;
		config_add_list(ps, cl, ltmp, &newval, 0);
		fs_give((void **)&ltmp);
		rv = ps->mangled_body = 1;
	    }
	}
	else if(!(*cl)->var->user_val.l){
	    q_status_message(SM_ORDER, 0, 3, "No set value to delete");
	}
	else{
	    if((*cl)->var->is_fixed){
		parse_printer((*cl)->var->user_val.l[(*cl)->varmem],
		    &nick, &p, NULL, NULL, NULL, NULL);
	        sprintf(prompt, "Delete (unused) printer %.30s ",
		    *nick ? nick : (!*p) ? empty_val2 : p);
		fs_give((void **)&nick);
		fs_give((void **)&p);
	    }
	    else
	      sprintf(prompt, "Really delete item %.20s from printer list ",
		    int2string((*cl)->varmem + 1));

	    ps->mangled_footer = 1;
	    if(want_to(prompt, 'n', 'n', h_config_print_del, 0, 1) == 'y'){
		rv = ps->mangled_body = 1;
		fs_give((void **)&(*cl)->var->user_val.l[(*cl)->varmem]);
		config_del_list_item(cl, &newval);
	    }
	    else
	      q_status_message(SM_ORDER, 0, 3, "Printer not deleted");
	}

	break;

      case 'c':					/* edit/change list option */
      case PF11:
	if((*cl)->var->current_val.l
	  && (*cl)->var->current_val.l[(*cl)->varmem]
	  && !strucmp(ps->VAR_PRINTER,(*cl)->var->current_val.l[(*cl)->varmem]))
	    changing_selected = 1;

	if((*cl)->var->is_fixed)
	  q_status_message(SM_ORDER, 3, 3,
			     "Can't change sys-admin defined printer.");
	else if(!(*cl)->var->user_val.l && (*cl)->var->current_val.l)
	  goto replace_text;
	else if(!(*cl)->var->user_val.l && !(*cl)->var->current_val.l)
	  goto add_text;
	else{
	    HelpType help;

	    ekey[0].ch    = 'n';
	    ekey[0].rval  = 'n';
	    ekey[0].name  = "N";
	    ekey[0].label = "Name";
	    ekey[1].ch    = 'c';
	    ekey[1].rval  = 'c';
	    ekey[1].name  = "C";
	    ekey[1].label = "Command";
	    ekey[2].ch    = 'o';
	    ekey[2].rval  = 'o';
	    ekey[2].name  = "O";
	    ekey[2].label = "Options";
	    ekey[3].ch    = -1;
	    strcpy(prompt, "Change Name or Command or Options ? ");
	    i = radio_buttons(prompt, -FOOTER_ROWS(ps), ekey, 'c', 'x',
			      h_config_print_name_cmd, RB_NORM);

	    if(i == 'x'){
		q_status_message(SM_ORDER,0,3,"Change cancelled");
		break;
	    } 
	    else if(i == 'c'){
		char *all_but_cmd;

		parse_printer((*cl)->var->user_val.l[(*cl)->varmem],
		    NULL, &p, NULL, NULL, NULL, &all_but_cmd);
		
		strcpy(prompt, "Change command : ");
		strcpy(sval, p ? p : "");
		fs_give((void **)&p);

		ps->mangled_footer = 1;
		help = NO_HELP;
		while(1){
		    i = optionally_enter(sval, -FOOTER_ROWS(ps), 0, MAXPATH, 1,
			     0, prompt, NULL, help, 0);
		    if(i == 0){
			removing_leading_white_space(sval);
			removing_trailing_white_space(sval);
			rv = ps->mangled_body = 1;
			if((*cl)->var->user_val.l[(*cl)->varmem])
			  fs_give((void **)&(*cl)->var->user_val.l[
							       (*cl)->varmem]);

			i = 0;
			for(tmp = sval; *tmp; tmp++)
			  if(*tmp == ',')
			    i++;	/* count of ,'s */

			if(!i){	/* only one item */
			    (*cl)->var->user_val.l[(*cl)->varmem]
			      = (char *)fs_get(strlen(all_but_cmd) +
						strlen(sval) + 1);
			    strcpy((*cl)->var->user_val.l[(*cl)->varmem],
				    all_but_cmd);
			    strcat((*cl)->var->user_val.l[(*cl)->varmem],
				    sval);

			    newval = &(*cl)->value;
			}
			else{
			    /*
			     * Don't allow input of multiple entries at once.
			     */
			    q_status_message(SM_ORDER,3,5,
				"No commas allowed in command");
			    continue;
			}
		    }
		    else if(i == 1){
			q_status_message(SM_ORDER,0,3,"Change cancelled");
		    }
		    else if(i == 3){
			help = help == NO_HELP ? h_config_change : NO_HELP;
			continue;
		    }
		    else if(i == 4){		/* no redraw, yet */
			continue;
		    }

		    break;
		}
	    }
	    else if(i == 'n'){
		char *all_but_nick;

		parse_printer((*cl)->var->user_val.l[(*cl)->varmem],
		    &p, NULL, NULL, NULL, &all_but_nick, NULL);
		
		strcpy(prompt, "Change name : ");
		strcpy(name, p ? p : "");
		fs_give((void **)&p);

		ps->mangled_footer = 1;
		help = NO_HELP;
		while(1){
		    i = optionally_enter(name, -FOOTER_ROWS(ps), 0, MAXPATH, 1,
			     0, prompt, NULL, help, 0);
		    if(i == 0){
			rv = ps->mangled_body = 1;
			removing_leading_white_space(name);
			removing_trailing_white_space(name);
			if((*cl)->var->user_val.l[(*cl)->varmem])
			  fs_give((void **)&(*cl)->var->user_val.l[
							       (*cl)->varmem]);

			(*cl)->var->user_val.l[(*cl)->varmem]
			  = (char *)fs_get(strlen(name) + 1
					+ ((*all_but_nick == '[') ? 0 : 3)
					+ strlen(all_but_nick) + 1);
			sprintf((*cl)->var->user_val.l[(*cl)->varmem],
			    "%s %s%s", name,
			    (*all_but_nick == '[') ? "" : "[] ",
			    all_but_nick);
			
			newval = &(*cl)->value;
		    }
		    else if(i == 1){
			q_status_message(SM_ORDER,0,3,"Change cancelled");
		    }
		    else if(i == 3){
			help = help == NO_HELP ? h_config_change : NO_HELP;
			continue;
		    }
		    else if(i == 4){		/* no redraw, yet */
			continue;
		    }

		    break;
		}
		
		fs_give((void **)&all_but_nick);
	    }
	    else if(i == 'o'){
		HelpType help;

		ekey[0].ch    = 'i';
		ekey[0].rval  = 'i';
		ekey[0].name  = "I";
		ekey[0].label = "Init";
		ekey[1].ch    = 't';
		ekey[1].rval  = 't';
		ekey[1].name  = "T";
		ekey[1].label = "Trailer";
		ekey[2].ch    = -1;
		strcpy(prompt, "Change Init string or Trailer string ? ");
		j = radio_buttons(prompt, -FOOTER_ROWS(ps), ekey, 'i', 'x',
				  h_config_print_opt_choice, RB_NORM);

		if(j == 'x'){
		    q_status_message(SM_ORDER,0,3,"Change cancelled");
		    break;
		} 
		else{
		    char *init, *trailer;

		    parse_printer((*cl)->var->user_val.l[(*cl)->varmem],
			&nick, &p, &init, &trailer, NULL, NULL);
		    
		    sprintf(prompt, "Change %s string : ",
			(j == 'i') ? "INIT" : "TRAILER");
		    strcpy(sval, (j == 'i') ? init : trailer);

		    tmp = string_to_cstring(sval);
		    strcpy(sval, tmp);
		    fs_give((void **)&tmp);
		    
		    ps->mangled_footer = 1;
		    help = NO_HELP;
		    while(1){
			i = optionally_enter(sval, -FOOTER_ROWS(ps), 0,
			    MAXPATH, 1, 0, prompt, NULL, help, 0);
			if(i == 0){
			    removing_leading_white_space(sval);
			    removing_trailing_white_space(sval);
			    rv = 1;
			    if((*cl)->var->user_val.l[(*cl)->varmem])
			      fs_give((void **)&(*cl)->var->user_val.l[
							       (*cl)->varmem]);
			    if(j == 'i'){
				init = cstring_to_hexstring(sval);
				tmp = cstring_to_hexstring(trailer);
				fs_give((void **)&trailer);
				trailer = tmp;
			    }
			    else{
				trailer = cstring_to_hexstring(sval);
				tmp = cstring_to_hexstring(init);
				fs_give((void **)&init);
				init = tmp;
			    }

			    (*cl)->var->user_val.l[(*cl)->varmem]
			      = (char *)fs_get(strlen(nick) + 1
				  + 2 + strlen("INIT=") + strlen(init)
				  + 1 + strlen("TRAILER=") + strlen(trailer)
				  + 1 + strlen(p) + 1);
			    sprintf((*cl)->var->user_val.l[(*cl)->varmem],
				"%s%s%s%s%s%s%s%s%s%s%s",
	    /* nick */	    nick,
	    /* space */	    *nick ? " " : "",
	    /* [ */		    (*nick || *init || *trailer) ? "[" : "",
	    /* INIT= */	    *init ? "INIT=" : "",
	    /* init */	    init,
	    /* space */	    (*init && *trailer) ? " " : "",
	    /* TRAILER= */	    *trailer ? "TRAILER=" : "",
	    /* trailer */	    trailer,
	    /* ] */		    (*nick || *init || *trailer) ? "]" : "",
	    /* space */	    (*nick || *init || *trailer) ? " " : "",
	    /* command */	    p);
	    
			    newval = &(*cl)->value;
			}
			else if(i == 1){
			    q_status_message(SM_ORDER,0,3,"Change cancelled");
			}
			else if(i == 3){
			    help=(help == NO_HELP)?h_config_print_init:NO_HELP;
			    continue;
			}
			else if(i == 4){		/* no redraw, yet */
			    continue;
			}

			break;
		    }

		    fs_give((void **)&nick);
		    fs_give((void **)&p);
		    fs_give((void **)&init);
		    fs_give((void **)&trailer);
		}
	    }
	}

	break;

      case 'e':					/* exit */
      case PF3:
	rv = config_exit_cmd(flags);
	break;

      default:
	rv = -1;
	break;
    }

    if(skip_to_next)
      *cl = next_confline(*cl);

    /*
     * At this point, if changes occurred, var->user_val.X is set.
     * So, fix the current_val, and handle special cases...
     */
    if(rv == 1){
	set_current_val((*cl)->var, TRUE, FALSE);
	fix_side_effects(ps, (*cl)->var, 0);

	if(newval){
	    if(*newval)
	      fs_give((void **)newval);
	    
	    if((*cl)->var->current_val.l)
	      *newval = printer_name((*cl)->var->current_val.l[(*cl)->varmem]);
	    else
	      *newval = cpystr("");
	}

	if(changing_selected)
	  print_select_tool(ps, 's', cl, flags);
    }

    return(rv);
}



/*
 * Manage display of the config/options menu body.
 */
void
update_option_screen(ps, screen, cursor_pos)
    struct pine  *ps;
    OPT_SCREEN_S *screen;
    Pos          *cursor_pos;
{
    int		   dline;
    CONF_S	  *top_line, *ctmp;

#ifdef _WINDOWS
    mswin_beginupdate();
#endif
    if(cursor_pos){
	cursor_pos->col = 0;
	cursor_pos->row = -1;		/* to tell us if we've set it yet */
    }

    /*
     * calculate top line of display for reframing if the current field
     * is off the display defined by screen->top_line...
     */
    if(ctmp = screen->top_line)
      for(dline = BODY_LINES(ps);
	  dline && ctmp && ctmp != screen->current;
	  ctmp = next_confline(ctmp), dline--)
	;

    if(!ctmp || !dline){		/* force reframing */
	dline = 0;
	ctmp = top_line = first_confline(screen->current);
	do
	  if(((dline++)%BODY_LINES(ps)) == 0)
	    top_line = ctmp;
	while(ctmp != screen->current && (ctmp = next_confline(ctmp)));
    }
    else
      top_line = screen->top_line;

#ifdef _WINDOWS
    /*
     * Figure out how far down the top line is from the top and how many
     * total lines there are.  Dumb to loop every time thru, but
     * there aren't that many lines, and it's cheaper than rewriting things
     * to maintain a line count in each structure...
     */
    for(dline = 0, ctmp = top_line; ctmp; ctmp = prev_confline(ctmp))
      dline++;

    scroll_setpos(dline - 1L);

    for(ctmp = next_confline(top_line); ctmp ; ctmp = next_confline(ctmp))
      dline++;

    scroll_setrange(dline);
#endif

    /* mangled body or new page, force redraw */
    if(ps->mangled_body || screen->top_line != top_line)
      screen->prev = NULL;

    /* loop thru painting what's needed */
    for(dline = 0, ctmp = top_line;
	dline < BODY_LINES(ps);
	dline++, ctmp = next_confline(ctmp)){

	/*
	 * only fall thru painting if something needs painting...
	 */
	if(!(!screen->prev || ctmp == screen->prev || ctmp == screen->current
	     || ctmp == screen->prev->varnamep
	     || ctmp == screen->current->varnamep
	     || ctmp == screen->prev->headingp
	     || ctmp == screen->current->headingp))
	  continue;

	ClearLine(dline + HEADER_ROWS(ps));

	if(ctmp && ctmp->varname && !(ctmp->flags & CF_INVISIBLEVAR)){
	    if(ctmp == screen->current && cursor_pos)
	      cursor_pos->row  = dline + HEADER_ROWS(ps);

	    if((ctmp == screen->current || ctmp == screen->current->varnamep
	           || ctmp == screen->current->headingp)
	       && !(ctmp->flags & CF_NOHILITE))
		  StartInverse();

	    if(ctmp->varoffset)
	      MoveCursor(dline+HEADER_ROWS(ps), ctmp->varoffset);

	    Write_to_screen(ctmp->varname);
	    if((ctmp == screen->current || ctmp == screen->current->varnamep
	           || ctmp == screen->current->headingp)
	       && !(ctmp->flags & CF_NOHILITE))
		  EndInverse();
	}

	if(ctmp && ctmp->value){
	    char *p = tmp_20k_buf;
	    int   i, j;
	    if(ctmp == screen->current){
		StartInverse();
		if(cursor_pos)
		  cursor_pos->row  = dline + HEADER_ROWS(ps);
	    }

	    /*
	     * Copy the value to a temp buffer expanding tabs, and
	     * making sure not to write beyond screen right...
	     */
	    for(i = 0, j = ctmp->valoffset;
		ctmp->value[i] && j < ps->ttyo->screen_cols;
		i++){
		if(ctmp->value[i] == ctrl('I')){
		    do
		      *p++ = ' ';
		    while(j < ps_global->ttyo->screen_cols && ((++j)&0x07));
			  
		}
		else{
		    *p++ = ctmp->value[i];
		    j++;
		}
	    }

	    *p = '\0';
	    if(ctmp == screen->current && cursor_pos){
		cursor_pos->col = ctmp->valoffset;
		if(ctmp->tool==radiobutton_tool || ctmp->tool==checkbox_tool)
		  cursor_pos->col++;
	    }

	    PutLine0(dline+HEADER_ROWS(ps), ctmp->valoffset, tmp_20k_buf);
	    if(ctmp == screen->current)
	      EndInverse();
	}
    }

    ps->mangled_body = 0;
    screen->top_line = top_line;
    screen->prev     = screen->current;
#ifdef _WINDOWS
    mswin_endupdate();
#endif
}



/*
 * 
 */
void
print_option_screen(screen, prompt)
    OPT_SCREEN_S *screen;
    char *prompt;
{
    CONF_S *ctmp;
    int     so_far;
    char    line[500];

    if(open_printer(prompt) == 0){
	for(ctmp = first_confline(screen->current);
	    ctmp;
	    ctmp = next_confline(ctmp)){

	    so_far = 0;
	    if(ctmp->varname && !(ctmp->flags & CF_INVISIBLEVAR)){

		sprintf(line, "%*s%s", ctmp->varoffset, "", ctmp->varname);
		print_text(line);
		so_far = ctmp->varoffset + strlen(ctmp->varname);
	    }

	    if(ctmp && ctmp->value){
		char *p = tmp_20k_buf;
		int   i, j, spaces;

		/* Copy the value to a temp buffer expanding tabs. */
		for(i = 0, j = ctmp->valoffset; ctmp->value[i]; i++){
		    if(ctmp->value[i] == ctrl('I')){
			do
			  *p++ = ' ';
			while((++j) & 0x07);
			      
		    }
		    else{
			*p++ = ctmp->value[i];
			j++;
		    }
		}

		*p = '\0';
		removing_trailing_white_space(tmp_20k_buf);

		spaces = max(ctmp->valoffset - so_far, 0);
		sprintf(line, "%*s%s\n", spaces, "", tmp_20k_buf);
		print_text(line);
	    }
	}

	close_printer();
    }
}



/*
 *
 */
void
option_screen_redrawer()
{
    ps_global->mangled_body = 1;
    update_option_screen(ps_global, opt_screen, (Pos *)NULL);
}



/*
 * pretty_value - given the variable and, if list, member, return an
 *                alloc'd string containing var's value...
 */
char *
pretty_value(ps, cl)
    struct pine *ps;
    CONF_S      *cl;
{
    char tmp[MAXPATH];

    if(cl->var->is_list){
	if(!cl->var->is_fixed && cl->var->user_val.l){
	    sprintf(tmp, "%-*s", ps->ttyo->screen_cols - cl->valoffset,
		    (*cl->var->user_val.l[cl->varmem])
		       ? cl->var->user_val.l[cl->varmem]
		       : empty_val2);
	}
	else{
	    char *p = tmp;
	    *p++ = '<';
	    *p   = '\0';
	    sstrcpy(&p, cl->var->is_fixed ? fixed_val : no_val);
	    if(cl->var->current_val.l){
		int i, l, l2;

		sstrcpy(&p, ": using \"");
		for(i = 0; cl->var->current_val.l[i]; i++){
		    if(i)
		      *p++ = ',';

		    if((l=ps->ttyo->screen_cols-cl->valoffset-(p-tmp)-2) > 0){
			strncpy(p, cl->var->current_val.l[i], l);
			if(l < (l2 = strlen(cl->var->current_val.l[i]))){
			    strncpy(p += (l - 4), " ...", 4);
			    p += 4;
			    break;
			}
			else
			  p += l2;
		    }
		    else
		      break;
		}

		*p++ = '\"';
	    }

	    sprintf(p, ">%*s", max(0, ps->ttyo->screen_cols - cl->valoffset
								 - (p - tmp)),
		    "");
	}

    }
    else if(cl->var->is_fixed || !cl->var->user_val.p){
	sprintf(tmp, cl->var->is_fixed
			? "<%s%s%s%s>%*s" : "<%s%s%s%s>%*s", 
		cl->var->is_fixed ? fixed_val : no_val,
		(cl->var->current_val.p) ? ": using \"" : "",
		(cl->var->current_val.p) ? cl->var->current_val.p : "",
		(cl->var->current_val.p) ? "\"" : "",
		max(0, ps->ttyo->screen_cols - cl->valoffset - 13
				  - ((cl->var->current_val.p) ? 9 : 0)
				  - ((cl->var->current_val.p)
				       ? strlen(cl->var->current_val.p) : 0)
				  - ((cl->var->current_val.p) ? 1 : 0)),
		"");
    }
    else
      sprintf(tmp, "%-*s", ps->ttyo->screen_cols - cl->valoffset,
	      (*cl->var->user_val.p) ? cl->var->user_val.p
					: empty_val2);

    return(cpystr(tmp));
}


/*
 * test_feature - runs thru a feature list, and returns:
 *                 1 if feature explicitly set and matches 'v'
 *                 0 if feature not explicitly set *or* doesn't match 'v'
 */
int
test_feature(l, f, g, v)
    char **l;
    char  *f;
    int    g, v;
{
    char *p;
    int   rv = 0, forced_off;

    for(; l && *l; l++){
	p = (forced_off = !struncmp(*l, "no-", 3)) ? *l + 3 : *l;
	if(!strucmp(p, f))
	  rv = (v == !forced_off);
	else if(g && !strucmp(p, "old-growth"))
	  rv = (v == forced_off);
    }

    return(rv);
}


void
clear_feature(l, f)
    char ***l;
    char   *f;
{
    char **list = l ? *l : NULL, newval[256];
    int    count = 0;

    for(; list && *list; list++, count++){
	if(f && !strucmp(((!struncmp(*list,"no-",3)) ? *list + 3 : *list), f)){
	    fs_give((void **)list);
	    f = NULL;
	}

	if(!f)					/* shift  */
	  *list = *(list + 1);
    }

    /*
     * this is helpful to keep the array from growing if a feature
     * get's set and unset repeatedly
     */
    if(!f)
      fs_resize((void **)l, count * sizeof(char *));
}


void
set_feature(l, f, v)
    char ***l;
    char   *f;
    int     v;
{
    char **list = l ? *l : NULL, newval[256];
    int    count = 0;

    sprintf(newval, "%s%s", v ? "" : "no-", f);
    for(; list && *list; list++, count++)
      if(!strucmp(((!struncmp(*list, "no-", 3)) ? *list + 3 : *list), f)){
	  fs_give((void **)list);		/* replace with new value */
	  *list = cpystr(newval);
	  return;
      }

    /*
     * if we got here, we didn't find it in the list, so grow the list
     * and add it..
     */
    if(!*l)
      *l = (char **)fs_get((count + 2) * sizeof(char *));
    else
      fs_resize((void **)l, (count + 2) * sizeof(char *));

    (*l)[count]     = cpystr(newval);
    (*l)[count + 1] = NULL;
}


/*
 *
 */
void
toggle_feature_bit(ps, index, var, value)
    struct pine     *ps;
    int		     index;
    struct variable *var;
    char            *value;
{
    NAMEVAL_S  *f;
    char      **vp, *p;
    int		i, og;

    f  = feature_list(index);
    og = test_old_growth_bits(ps, f->value);

    /*
     * if this feature is in the fixed set, or old-growth is in the fixed
     * set and this feature is in the old-growth set, don't alter it...
     */
    for(vp = var->fixed_val.l; vp && *vp; vp++){
	p = (struncmp(*vp, "no-", 3)) ? *vp : *vp + 3;
	if(!strucmp(p, f->name) || (og && !strucmp(p, "old-growth"))){
	    q_status_message(SM_ORDER, 3, 3,
			     "Can't change value fixed by sys-admin.");
	    return;
	}
    }

    F_SET(f->value, ps, !F_ON(f->value, ps));	/* flip the bit */
    if(value)
      value[1] = F_ON(f->value, ps) ? 'X' : ' ';

    /*
     * fix up the user's feature list based on global and current
     * settings..
     *
     * Note, we only care if "old-growth" is set or not in as much as
     * we don't want to add redundant feature entries.  we won't add or 
     * remove "old-growth" in that the set it defines may change in the
     * future...
     */
    if(test_feature(var->global_val.l,f->name,og,F_ON(f->value,ps))
       || test_feature(var->user_val.l,f->name,og,!F_ON(f->value,ps)))
      clear_feature(&var->user_val.l, f->name);
    else
      set_feature(&var->user_val.l, f->name, F_ON(f->value, ps));

    /*
     * Handle any features that need special attention here...
     */
    if(f->value == F_QUOTE_ALL_FROMS)
      mail_parameters(NULL,SET_FROMWIDGET,(void *)(F_ON(f->value,ps) ? 1 : 0));
    else if(f->value == F_QUELL_LOCK_FAILURE_MSGS)
      mail_parameters(NULL, SET_LOCKEACCESERROR,
		      (void *)(F_ON(f->value,ps) ? 1 : 0));
    else if(f->value == F_ENABLE_INCOMING &&  F_ON(f->value, ps)){
	q_status_message(SM_ORDER | SM_DING, 3, 4,
	    "Folder List changes will take effect your next pine session.");
    }
    else if(f->value == F_PRESERVE_START_STOP){
	/* toggle raw mode settings to make tty driver aware of new setting */
	Raw(0);
	Raw(1);
    }
    else if(f->value == F_BLANK_KEYMENU){
	clearfooter(ps);
	if(F_ON(f->value,ps)){
	    FOOTER_ROWS(ps) = 1;
	    ps->mangled_body = 1;
	}
	else{
	    FOOTER_ROWS(ps) = 3;
	    ps->mangled_footer = 1;
	}
    }
#if !defined(DOS) && !defined(OS2)
    else if(f->value == F_ALLOW_TALK){
	if(F_ON(f->value,ps))
	  allow_talk(ps);
	else
	  disallow_talk(ps);
    }
#endif
#ifdef	MOUSE
    else if(f->value == F_ENABLE_MOUSE){
	if(F_ON(f->value,ps))
	  init_mouse();
	else
	  end_mouse();
    }
#endif
}


/*
 * new_confline - create new CONF_S zero it out, and insert it after current.
 *                NOTE current gets set to the new CONF_S too!
 */
CONF_S *
new_confline(current)
    CONF_S **current;
{
    CONF_S *p;

    p = (CONF_S *)fs_get(sizeof(CONF_S));
    memset((void *)p, 0, sizeof(CONF_S));
    if(current){
	if(*current){
	    p->next	     = (*current)->next;
	    (*current)->next = p;
	    p->prev	     = *current;
	    if(p->next)
	      p->next->prev = p;
	}

	*current = p;
    }

    return(p);
}


/*
 *
 */
void
free_confline(p)
    CONF_S **p;
{
    if(p){
	if((*p)->varname)
	  fs_give((void **)&(*p)->varname);

	if((*p)->value)
	  fs_give((void **)&(*p)->value);

	if((*p)->prev)
	  (*p)->prev->next = (*p)->next;

	if((*p)->next)
	  (*p)->next->prev = (*p)->prev;

	fs_give((void **)p);
    }
}


/*
 *
 */
CONF_S *
first_confline(p)
    CONF_S *p;
{
    while(p && p->prev)
      p = p->prev;

    return(p);
}


/*
 * First selectable confline.
 */
CONF_S *
first_sel_confline(p)
    CONF_S *p;
{
    for(p = first_confline(p); p && (p->flags&CF_NOSELECT); p=next_confline(p))
      ;/* do nothing */

    return(p);
}


/*
 *
 */
CONF_S *
last_confline(p)
    CONF_S *p;
{
    while(p && p->next)
      p = p->next;

    return(p);
}


int
offer_to_fix_pinerc(ps)
    struct pine *ps;
{
    struct variable *v;
    char             prompt[300];
    char            *p, *q;
    char           **list;
    char           **list_fixed;
    int              rv = 0;
    int              i, k, need;
    char            *clear = ": delete it";

    ps->fix_fixed_warning = 0;  /* so we only ask first time */

    if(ps->readonly_pinerc)
      return(rv);

    if(want_to("Some of your options conflict with site policy.  Investigate",
	'y', 'n', NO_HELP, 0, 1) != 'y')
      return(rv);
    
/* space want_to requires in addition to the string you pass in */
#define WANTTO_SPACE 6
    need = WANTTO_SPACE + strlen(clear);

    for(v = ps->vars; v->name; v++){
	if(!v->is_fixed ||
	   !v->is_user ||
	    v->is_obsolete ||
	    v == &ps->vars[V_FEATURE_LIST]) /* handle feature-list below */
	  continue;
	
	prompt[0] = '\0';
	
	if(v->is_list && v->user_val.l){
	    if(*v->user_val.l){
		sprintf(prompt, "Your setting for %s is ", v->name);
		p = prompt + strlen(prompt);
		for(i = 0; v->user_val.l[i]; i++){
		    if(p - prompt > ps->ttyo->screen_cols - need)
		      break;
		    if(i)
		      *p++ = ',';
		    sstrcpy(&p, v->user_val.l[i]);
		}
		*p = '\0';
	    }
	    else
	      sprintf(prompt, "Your setting for %s is %s", v->name, empty_val2);
	}
	else{
	    if(v->user_val.p){
		if(*v->user_val.p){
		    sprintf(prompt, "Your setting for %s is %s",
			v->name, v->user_val.p);
		}
		else{
		    sprintf(prompt, "Your setting for %s is %s",
			v->name, empty_val2);
		}
	    }
	}
	if(*prompt){
	    if(strlen(prompt) > ps->ttyo->screen_cols - need)
	      (void)strcpy(prompt + max(ps->ttyo->screen_cols - need - 3, 0),
			  "...");

	    (void)strcat(prompt, clear);
	    if(want_to(prompt, 'y', 'n', NO_HELP, 0, 0) == 'y'){
		if(v->is_list){
		    if(v->user_val.l){
			rv++;
			for(i = 0; v->user_val.l[i]; i++)
			  fs_give((void **)&v->user_val.l[i]);

			fs_give((void **)&v->user_val.l);
		    }
		}
		else if(v->user_val.p){
		    rv++;
		    fs_give((void **)&v->user_val.p);
		}
	    }
	}
    }

    /*
     * As always, feature-list has to be handled separately.
     */
    v = &ps->vars[V_FEATURE_LIST];
    list = v->user_val.l;
    list_fixed = v->fixed_val.l;
    if(list){
      for(i = 0; list[i]; i++){
	p = list[i];
	if(!struncmp(p, "no-", 3))
	  p += 3;
	for(k = 0; list_fixed && list_fixed[k]; k++){
	  q = list_fixed[k];
	  if(!struncmp(q, "no-", 3))
	    q += 3;
	  if(!strucmp(q, p)){
	    sprintf(prompt, "Your %s is %s, fixed value is %s",
		p, p == list[i] ? "ON" : "OFF",
		q == list_fixed[k] ? "ON" : "OFF");

	    if(strlen(prompt) > ps->ttyo->screen_cols - need)
	      (void)strcpy(prompt + max(ps->ttyo->screen_cols - need - 3, 0),
			  "...");

	    (void)strcat(prompt, clear);
	    if(want_to(prompt, 'y', 'n', NO_HELP, 0, 0) == 'y'){
		rv++;
		/*
		 * Clear the feature from the user's pinerc
		 * so that we'll stop bothering them when they
		 * start up Pine.
		 */
		clear_feature(&v->user_val.l, p);

		/*
		 * clear_feature scoots the list up, so if list[i] was
		 * the last one going in, now it is the end marker.  We
		 * just decrement i so that it will get incremented and
		 * then test == 0 in the for loop.  We could just goto
		 * outta_here to accomplish the same thing.
		 */
		if(!list[i])
		  i--;
	    }
	  }
	}
      }
    }

    return(rv);
}


/*
 * Compare saved user_val with current user_val to see if it changed.
 * If any have changed, change it back and take the appropriate action.
 */
void
revert_to_saved_config(ps, vsave)
struct pine *ps;
SAVED_CONFIG_S *vsave;
{
    struct variable *vreal;
    SAVED_CONFIG_S  *v;
    int i, n;

    v = vsave;
    for(vreal = ps->vars; vreal->name; vreal++,v++){
	if(!SAVE_INCLUDE(ps, vreal))
	  continue;
	
	if(vreal == &ps->vars[V_FEATURE_LIST]){
	    /* handle feature changes */
	    if(memcmp(v->user_val.features, ps->feature_list, BM_SIZE)){
		NAMEVAL_S *f;

		/*
		 * At least one feature was changed.  Go through the
		 * list of eligible features and toggle them back to what
		 * they were.
		 */

		for(i = 0; f = feature_list(i); i++){
		    if(F_INCLUDE(f->value)
			&& ((F_ON(f->value, ps)
			      && !bitnset(f->value,v->user_val.features))
			   ||
			   (F_OFF(f->value, ps)
			      && bitnset(f->value,v->user_val.features)))){

			toggle_feature_bit(ps, i, vreal, NULL);
		    }
		}
	    }
	}
	else{
	    int changed = 0;

	    if(vreal->is_list){
		if((v->user_val.l && !vreal->user_val.l)
		   || (!v->user_val.l && vreal->user_val.l))
		  changed++;
		else if(!v->user_val.l && !vreal->user_val.l)
		  ;/* no change, nothing to do */
		else
		  for(i = 0; v->user_val.l[i] || vreal->user_val.l[i]; i++)
		    if((v->user_val.l[i]
			  && (!vreal->user_val.l[i]
			     || strcmp(v->user_val.l[i], vreal->user_val.l[i])))
		       ||
			 (!v->user_val.l[i] && vreal->user_val.l[i])){
			changed++;
			break;
		    }
		
		if(changed){
		    char **list;

		    /* free the changed value */
		    if(vreal->user_val.l){
			for(i = 0; vreal->user_val.l[i]; i++)
			  fs_give((void **)&vreal->user_val.l[i]);
			
			fs_give((void **)&vreal->user_val.l);
		    }

		    /* copy back the original one */
		    if(v->user_val.l){
			list = v->user_val.l;
			n = 0;
			/* count how many */
			while(list[n])
			  n++;

			vreal->user_val.l
				= (char **)fs_get((n+1) * sizeof(char *));
			for(i = 0; i < n; i++)
			  vreal->user_val.l[i] = cpystr(v->user_val.l[i]);

			vreal->user_val.l[n] = NULL;
		    }
		}
	    }
	    else{
		if((v->user_val.p
		      && (!vreal->user_val.p
			  || strcmp(v->user_val.p, vreal->user_val.p)))
		   ||
		     (!v->user_val.p && vreal->user_val.p)){
		    /* It changed, fix it */
		    changed++;
		    /* free the changed value */
		    if(vreal->user_val.p)
		      fs_give((void **)&vreal->user_val.p);
		    
		    /* copy back the original one */
		    vreal->user_val.p = cpystr(v->user_val.p);
		}
	    }

	    if(changed){
		set_current_val(vreal, TRUE, FALSE);
		fix_side_effects(ps, vreal, 1);
	    }
	}
    }
}


/*
 * Adjust side effects that happen because variable changes values.
 *
 * Var->user_val should be set to the new value before calling this.
 */
void
fix_side_effects(ps, var, revert)
struct pine     *ps;
struct variable *var;
int              revert;
{
    int    i;
    char **v, *q;

    /* move this up here so we get the Using default message */
    if(var == &ps->vars[V_PERSONAL_NAME]){
	if(!var->user_val.p && ps->ui.fullname){
	    if(var->current_val.p)
	      fs_give((void **)&var->current_val.p);

	    var->current_val.p = cpystr(ps->ui.fullname);
	}
    }

    if(!revert
      && ((!var->is_fixed
	    && !var->is_list
	    && !var->user_val.p
	    && var->current_val.p)
	 ||
	 (!var->is_fixed
	    && var->is_list
	    && !var->user_val.l
	    && var->current_val.l)))
      q_status_message(SM_ORDER,0,3,"Using default value");

    if(var == &ps->vars[V_USER_DOMAIN]){
	char *p, *q;

	if(ps->VAR_USER_DOMAIN
	   && ps->VAR_USER_DOMAIN[0]
	   && (p = strrindex(ps->VAR_USER_DOMAIN, '@'))){
	    if(*(++p)){
		if(!revert)
		  q_status_message2(SM_ORDER, 3, 5,
		    "User-domain (%s) cannot contain \"@\"; using %s",
		    ps->VAR_USER_DOMAIN, p);
		q = ps->VAR_USER_DOMAIN;
		while((*q++ = *p++) != '\0')
		  ;/* do nothing */
	    }
	    else{
		if(!revert)
		  q_status_message1(SM_ORDER, 3, 5,
		    "User-domain (%s) cannot contain \"@\"; deleting",
		    ps->VAR_USER_DOMAIN);
		fs_give((void **)&ps->USR_USER_DOMAIN);
		set_current_val(&ps->vars[V_USER_DOMAIN], TRUE, TRUE);
	    }
	}

	/*
	 * Reset various pointers pertaining to domain name and such...
	 */
	init_hostname(ps);
    }
    else if(var == &ps->vars[V_INBOX_PATH]){
	/*
	 * fixup the inbox path based on global/default values...
	 */
	init_inbox_mapping(ps->VAR_INBOX_PATH, ps->context_list);

	if(!strucmp(ps->cur_folder, ps->inbox_name) && ps->mail_stream
	   && strcmp(ps->VAR_INBOX_PATH, ps->mail_stream->mailbox)){
	    /*
	     * If we currently have "inbox" open and the mailbox name
	     * doesn't match, reset the current folder's name...
	     */
	    strcpy(ps->cur_folder, ps->mail_stream->mailbox);
	    ps->inbox_stream   = NULL;
	    ps->mangled_header = 1;
	}
	else if(ps->inbox_stream
		&& strcmp(ps->VAR_INBOX_PATH, ps->inbox_stream->mailbox)){
	    /*
	     * if we don't have inbox directly open, but have it
	     * open for new mail notification, close the stream like
	     * any other ordinary folder, and clean up...
	     */
	    MAILSTREAM *s = ps->inbox_stream;
	    ps->inbox_stream = NULL;
	    mn_give(&ps->inbox_msgmap);
	    expunge_and_close(s, NULL, s->mailbox, NULL);
	}
    }
    else if(var == &ps->vars[V_FOLDER_SPEC]
       || var == &ps->vars[V_NEWS_SPEC]){
	if(!revert)
	  q_status_message(SM_ORDER | SM_DING, 3, 4,
	   "Folder List changes will take effect your next pine session.");
    }
    else if(var == &ps->vars[V_ADDRESSBOOK] ||
	    var == &ps->vars[V_GLOB_ADDRBOOK] ||
	    var == &ps->vars[V_ABOOK_FORMATS]){
	addrbook_reset();
    }
    else if(var == &ps->vars[V_INDEX_FORMAT]){
	init_index_format(ps->VAR_INDEX_FORMAT, &ps->index_disp_format);
	clear_index_cache();
    }
    else if(var == &ps->vars[V_DEFAULT_FCC]){
	init_save_defaults();
    }
    else if(var == &ps->vars[V_INIT_CMD_LIST]){
	if(!revert)
	  q_status_message(SM_ASYNC, 0, 3,
	    "Initial command changes will affect your next pine session.");
    }
    else if(var == &ps->vars[V_VIEW_HEADERS]){
	ps->view_all_except = 0;
	if(ps->VAR_VIEW_HEADERS)
	  for(v = ps->VAR_VIEW_HEADERS; (q = *v) != NULL; v++)
	    if(q[0]){
		char *p;

		removing_leading_white_space(q);
		/* look for colon or space or end */
		for(p = q; *p && !isspace((unsigned char)*p) && *p != ':'; p++)
		  ;/* do nothing */
		
		*p = '\0';
		if(strucmp(q, ALL_EXCEPT) == 0)
		  ps->view_all_except = 1;
	    }
    }
    else if(var == &ps->vars[V_OVERLAP]){
	int old_value = ps->viewer_overlap;

	if(SVAR_OVERLAP(ps, old_value, tmp_20k_buf)){
	    if(!revert)
	      q_status_message(SM_ORDER, 3, 5, tmp_20k_buf);
	}
	else
	  ps->viewer_overlap = old_value;
    }
    else if(var == &ps->vars[V_MARGIN]){
	int old_value = ps->scroll_margin;

	if(SVAR_MARGIN(ps, old_value, tmp_20k_buf)){
	    if(!revert)
	      q_status_message(SM_ORDER, 3, 5, tmp_20k_buf);
	}
	else
	  ps->scroll_margin = old_value;
    }
    else if(var == &ps->vars[V_FILLCOL]){
	if(SVAR_FILLCOL(ps, ps->composer_fillcol, tmp_20k_buf)){
	    if(!revert)
	      q_status_message(SM_ORDER, 3, 5, tmp_20k_buf);
	}
    }
    else if(var == &ps->vars[V_STATUS_MSG_DELAY]){
	if(SVAR_MSGDLAY(ps, ps->status_msg_delay, tmp_20k_buf)){
	    if(!revert)
	      q_status_message(SM_ORDER, 3, 5, tmp_20k_buf);
	}
    }
    else if(var == &ps->vars[V_MAILCHECK]){
	timeout = 15;
	if(SVAR_MAILCHK(ps, timeout, tmp_20k_buf)){
	    if(!revert)
	      q_status_message(SM_ORDER, 3, 5, tmp_20k_buf);
	}
	else if(timeout == 0L && !revert){
	    q_status_message(SM_ORDER, 4, 6,
"Warning: automatic new mail checking and mailbox checkpointing is disabled");
	    if(ps->VAR_INBOX_PATH && ps->VAR_INBOX_PATH[0] == '{')
	      q_status_message(SM_ASYNC, 3, 6,
"Warning: mail-check-interval=0 may cause IMAP server connection to time out");
	}
    }
#if defined(DOS) || defined(OS2)
    else if(var == &ps->vars[V_FOLDER_EXTENSION]){
	mail_parameters(NULL, SET_EXTENSION,
			(void *)var->current_val.p);
    }
    else if(var == &ps->vars[V_NEWSRC_PATH]){
	if(var->current_val.p && var->current_val.p[0])
	  mail_parameters(NULL, SET_NEWSRC,
			  (void *)var->current_val.p);
    }
#endif
    else if(revert
	  && (var == &ps->vars[V_SAVED_MSG_NAME_RULE]
              || var == &ps->vars[V_FCC_RULE]
              || var == &ps->vars[V_GOTO_DEFAULT_RULE]
              || var == &ps->vars[V_AB_SORT_RULE])){
	NAMEVAL_S *rule;

	if(var == &ps->vars[V_SAVED_MSG_NAME_RULE]){
	    for(i = 0; rule = save_msg_rules(i); i++)
	      if(!strucmp(var->user_val.p, rule->name)){
		  ps->save_msg_rule = rule->value;
		  break;
	      }
	}
	else if(var == &ps->vars[V_FCC_RULE]){
	    for(i = 0; rule = fcc_rules(i); i++)
	      if(!strucmp(var->user_val.p, rule->name)){
		  ps->fcc_rule = rule->value;
		  break;
	      }
	}
	else if(var == &ps->vars[V_GOTO_DEFAULT_RULE]){
	    for(i = 0; rule = goto_rules(i); i++)
	      if(!strucmp(var->user_val.p, rule->name)){
		  ps->goto_default_rule = rule->value;
		  break;
	      }
	}
	else{
	    for(i = 0; rule = ab_sort_rules(i); i++)
	      if(!strucmp(var->user_val.p, rule->name)){
		  ps->ab_sort_rule = rule->value;
		  break;
	      }

	    addrbook_reset();
	}
    }
    else if(revert && var == &ps->vars[V_SORT_KEY]){
	decode_sort(ps, var->user_val.p);
    }
#if defined(DOS) || defined(OS2)
    else if(revert
	   && (var == &ps->vars[V_NORM_FORE_COLOR]
	    || var == &ps->vars[V_NORM_BACK_COLOR]
	    || var == &ps->vars[V_REV_FORE_COLOR]
	    || var == &ps->vars[V_REV_BACK_COLOR])){
	if(var == &ps->vars[V_NORM_FORE_COLOR])
	  pico_nfcolor(var->user_val.p);
	else if(var == &ps->vars[V_NORM_BACK_COLOR])
	  pico_nbcolor(var->user_val.p);
	else if(var == &ps->vars[V_REV_FORE_COLOR])
	  pico_rfcolor(var->user_val.p);
	else
	  pico_rbcolor(var->user_val.p);
    }
#endif
    else if(var == &ps->vars[V_USE_ONLY_DOMAIN_NAME]){
	init_hostname(ps);
    }
}


SAVED_CONFIG_S *
save_config_vars(ps)
struct pine *ps;
{
    struct variable *vreal;
    SAVED_CONFIG_S *vsave, *v;

    vsave = (SAVED_CONFIG_S *)fs_get((V_LAST_VAR+1)*sizeof(SAVED_CONFIG_S));
    memset((void *)vsave, 0, (V_LAST_VAR+1)*sizeof(SAVED_CONFIG_S));
    v = vsave;
    for(vreal = ps->vars; vreal->name; vreal++,v++){
	if(!SAVE_INCLUDE(ps, vreal))
	  continue;
	
	if(vreal == &ps->vars[V_FEATURE_LIST]){
	    /* save copy of feature bitmap */
	    memcpy(v->user_val.features, ps->feature_list, BM_SIZE);
	}
	else if(vreal->is_list){  /* save user_val.l */
	    int n, i;
	    char **list;

	    if(vreal->user_val.l){
		/* count how many */
		n = 0;
		list = vreal->user_val.l;
		while(list[n])
		  n++;

		v->user_val.l = (char **)fs_get((n+1) * sizeof(char *));
		memset((void *)v->user_val.l, 0, (n+1)*sizeof(char *));
		for(i = 0; i < n; i++)
		  v->user_val.l[i] = cpystr(vreal->user_val.l[i]);

		v->user_val.l[n] = NULL;
	    }
	}
	else{  /* save user_val.p */
	    if(vreal->user_val.p)
	      v->user_val.p = cpystr(vreal->user_val.p);
	}
    }

    return(vsave);
}


void
free_saved_config(ps, vsavep)
struct pine *ps;
SAVED_CONFIG_S **vsavep;
{
    struct variable *vreal;
    SAVED_CONFIG_S *v;

    v = *vsavep;
    for(vreal = ps->vars; vreal->name; vreal++,v++){
	if(!SAVE_INCLUDE(ps, vreal))
	  continue;
	
	if(vreal == &ps->vars[V_FEATURE_LIST]) /* nothing to free */
	  continue;

	if(vreal->is_list){  /* free user_val.l */
	    int i;

	    if(v->user_val.l){
		for(i = 0; v->user_val.l[i]; i++)
	          fs_give((void **)&v->user_val.l[i]);

	        fs_give((void **)&v->user_val.l);
	    }
	}
	else if(v->user_val.p)
	  fs_give((void **)&v->user_val.p);
    }

    fs_give((void **)vsavep);
}


/*
 * Given a single printer string from the config file, returns pointers
 * to alloc'd strings containing the printer nickname, the command,
 * the init string, the trailer string, everything but the nickname string,
 * and everything but the command string.  All_but_cmd includes the trailing
 * space at the end (the one before the command) but all_but_nick does not
 * include the leading space (the one before the [).
 * If you pass in a pointer it is guaranteed to come back pointing to an
 * allocated string, even if it is just an empty string.  It is ok to pass
 * NULL for any of the six return strings.
 */
void
parse_printer(input, nick, cmd, init, trailer, all_but_nick, all_but_cmd)
    char  *input;
    char **nick,
	 **cmd,
	 **init,
	 **trailer,
	 **all_but_nick,
	 **all_but_cmd;
{
    char *p, *q, *start, *saved_options = NULL;
    int tmpsave, cnt;

    if(!input)
      input = "";

    if(nick || all_but_nick){
	if(p = srchstr(input, " [")){
	    if(all_but_nick)
	      *all_but_nick = cpystr(p+1);

	    if(nick){
		while(p-1 > input && isspace((unsigned char)*(p-1)))
		  p--;

		tmpsave = *p;
		*p = '\0';
		*nick = cpystr(input);
		*p = tmpsave;
	    }
	}
	else{
	    if(nick)
	      *nick = cpystr("");

	    if(all_but_nick)
	      *all_but_nick = cpystr(input);
	}
    }

    if(p = srchstr(input, "] ")){
	do{
	    ++p;
	}while(isspace((unsigned char)*p));

	tmpsave = *p;
	*p = '\0';
	saved_options = cpystr(input);
	*p = tmpsave;
    }
    else
      p = input;
    
    if(cmd)
      *cmd = cpystr(p);

    if(init){
	if(saved_options && (p = srchstr(saved_options, "INIT="))){
	    start = p + strlen("INIT=");
	    for(cnt=0, p = start;
		*p && *(p+1) && isxdigit((unsigned char)*p)
		   && isxdigit((unsigned char)*(p+1));
		p += 2)
	      cnt++;
	    
	    q = *init = (char *)fs_get((cnt + 1) * sizeof(char));
	    for(p = start;
		*p && *(p+1) && isxdigit((unsigned char)*p)
		   && isxdigit((unsigned char)*(p+1));
		p += 2)
	      *q++ = read_hex(p);
	    
	    *q = '\0';
	}
	else
	  *init = cpystr("");
    }

    if(trailer){
	if(saved_options && (p = srchstr(saved_options, "TRAILER="))){
	    start = p + strlen("TRAILER=");
	    for(cnt=0, p = start;
		*p && *(p+1) && isxdigit((unsigned char)*p)
		   && isxdigit((unsigned char)*(p+1));
		p += 2)
	      cnt++;
	    
	    q = *trailer = (char *)fs_get((cnt + 1) * sizeof(char));
	    for(p = start;
		*p && *(p+1) && isxdigit((unsigned char)*p)
		   && isxdigit((unsigned char)*(p+1));
		p += 2)
	      *q++ = read_hex(p);
	    
	    *q = '\0';
	}
	else
	  *trailer = cpystr("");
    }

    if(all_but_cmd){
	if(saved_options)
	  *all_but_cmd = saved_options;
	else
	  *all_but_cmd = cpystr("");
    }
    else if(saved_options)
      fs_give((void **)&saved_options);
}


/*
 * Given a single printer string from the config file, returns an allocated
 * copy of the friendly printer name, which is
 *      "Nickname"  command
 */
char *
printer_name(input)
    char *input;
{
    char *nick, *cmd;
    char *ret;

    parse_printer(input, &nick, &cmd, NULL, NULL, NULL, NULL);
    ret = (char *)fs_get((2+22+1+strlen(cmd)) * sizeof(char));
    sprintf(ret, "\"%.21s\"%*s%s",
	*nick ? nick : "",
	22 - min(strlen(nick), 21),
	"",
	cmd);
    fs_give((void **)&nick);
    fs_give((void **)&cmd);

    return(ret);
}


#ifdef _WINDOWS
/*----------------------------------------------------------------------
     MSWin scroll callback.  Called during scroll message processing.
	     


  Args: cmd - what type of scroll operation.
	scroll_pos - paramter for operation.  
			used as position for SCROLL_TO operation.

  Returns: TRUE - did the scroll operation.
	   FALSE - was not able to do the scroll operation.
 ----*/
int
config_scroll_callback (cmd, scroll_pos)
int	cmd;
long	scroll_pos;
{
    int paint = TRUE;
    
    switch (cmd) {
      case MSWIN_KEY_SCROLLUPLINE:
	paint = config_scroll_down (1);
	break;

      case MSWIN_KEY_SCROLLDOWNLINE:
	paint = config_scroll_up (1);
	break;

      case MSWIN_KEY_SCROLLUPPAGE:
	paint = config_scroll_down (BODY_LINES(ps_global));
	break;

      case MSWIN_KEY_SCROLLDOWNPAGE:
	paint = config_scroll_up (BODY_LINES(ps_global));
	break;

      case MSWIN_KEY_SCROLLTO:
	paint = config_scroll_to_pos (scroll_pos);
	break;
    }

    if(paint){
	option_screen_redrawer();
	fflush(stdout);
    }

    return(paint);
}
#endif	/* _WINDOWS */


static struct key gripe_keys[] = 
       {{"?","Help",KS_SCREENHELP},	{"^C","Cancel",KS_NONE},
	{NULL,NULL,KS_NONE},            {"S",NULL,KS_NONE},
	{"P","Prev",KS_NONE},		{"N","Next",KS_NONE},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{NULL,NULL,KS_NONE},	        {NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},	        {"W","WhereIs",KS_WHEREIS}};
INST_KEY_MENU(gripe_keymenu, gripe_keys);
#define SELECT_KEY 3


/*----------------------------------------------------------------------
    Bug report screen
 ----*/
void
gripe(ps) 
    struct pine *ps;
{
    CONF_S  *ctmpb, *heading, *start_line, *ctmpa = NULL;

    if(F_ON(F_DISABLE_DFLT_IN_BUG_RPT,ps_global))
      gripe_keys[SELECT_KEY].label = "Select";
    else
      gripe_keys[SELECT_KEY].label = "[Select]";

    new_confline(&ctmpa);
    heading = ctmpa;
    ctmpa->varoffset = 10;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->varname
      = cpystr("Reporting a bug:  choose one of the following options...");
    ctmpa->value     = NULL;

    new_confline(&ctmpa);
    ctmpa->valoffset = 0;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("");


    new_confline(&ctmpa);
    start_line = ctmpb = ctmpa;
    ctmpa->valoffset = 1;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->varname   = cpystr("L");
    ctmpa->flags    |= CF_INVISIBLEVAR;
    ctmpa->value
      = cpystr("o  Send a question to your local support staff.");
    ctmpa->varnamep  = ctmpb;
    ctmpa->headingp  = heading;

    new_confline(&ctmpa);
    ctmpa->valoffset = 4;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("If you have a question about how Pine works, or how some other aspect");

    new_confline(&ctmpa);
    ctmpa->valoffset = 4;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("of your computer system works, it is probably best if you ask your local");

    new_confline(&ctmpa);
    ctmpa->valoffset = 4;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("support staff.  They are likely to be able to answer more quickly than");

    new_confline(&ctmpa);
    ctmpa->valoffset = 4;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("the U.W. Pine development team, and they know much more about their system.");

    new_confline(&ctmpa);
    ctmpa->valoffset = 0;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("");


    new_confline(&ctmpa);
    ctmpb = ctmpa;
    ctmpa->valoffset = 1;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->varoffset = 1;
    ctmpa->varname   = cpystr("B");
    ctmpa->flags    |= CF_INVISIBLEVAR;
    ctmpa->value
      = cpystr("o  Send a bug report to the Pine development team at the Univ. of Wash.");
    ctmpa->varnamep  = ctmpb;
    ctmpa->headingp  = heading;

    new_confline(&ctmpa);
    ctmpa->valoffset = 4;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("The more precise a description you give the more likely");

    new_confline(&ctmpa);
    ctmpa->valoffset = 4;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("we'll be able to fix it for the next release.  Best of all is a way");

    new_confline(&ctmpa);
    ctmpa->valoffset = 4;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("to reproduce the problem, if you know of one, but we also want to know");

    new_confline(&ctmpa);
    ctmpa->valoffset = 4;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("about unreproducible bugs.");

    new_confline(&ctmpa);
    ctmpa->valoffset = 0;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("");

    new_confline(&ctmpa);
    ctmpb = ctmpa;
    ctmpa->valoffset = 1;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->varoffset = 1;
    ctmpa->varname   = cpystr("S");
    ctmpa->flags    |= CF_INVISIBLEVAR;
    ctmpa->value
      = cpystr("o  Send a suggestion to the Pine development team at the Univ. of Wash.");
    ctmpa->varnamep  = ctmpb;
    ctmpa->headingp  = heading;

    new_confline(&ctmpa);
    ctmpa->valoffset = 4;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("");

    new_confline(&ctmpa);
    ctmpa->valoffset = 2;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("...or, use \"Compose\" to post a message to \"pine-info@cac.washington.edu\", a");

    new_confline(&ctmpa);
    ctmpa->valoffset = 4;
    ctmpa->keymenu   = &gripe_keymenu;
    ctmpa->help      = NO_HELP;
    ctmpa->tool      = gripe_tool;
    ctmpa->flags    |= CF_NOSELECT;
    ctmpa->value
      = cpystr("world-wide mailing-list read by thousands of Pine users and administrators.");

    (void)conf_scroll_screen(ps, start_line, "BUG REPORT", NoPrint);
}


/*
 * standard type of storage object used for body parts...
 */
#ifdef	DOS
#define		  PART_SO_TYPE	TmpFileStar
#else
#define		  PART_SO_TYPE	CharStar
#endif

int
gripe_tool(ps, cmd, cl, flags)
    struct pine *ps;
    int          cmd;
    CONF_S     **cl;
    unsigned     flags;
{
    BODY      *body = NULL, *pb;
    ENVELOPE  *outgoing = NULL;
    PART     **pp;
    gf_io_t    pc;
    char       tmp[MAX_ADDRESS], *p, *sig;
    int	       i, ch = 0, ourcmd;
    char       composer_title[80];
    long       fake_reply = -1L,
	       msgno = mn_m2raw(ps->msgmap, mn_get_cur(ps->msgmap));
    static char *err    = "Problem creating space for message text.";

    ourcmd = 'X';
    switch(cmd){
      case ctrl('C'):
      case PF2:
      default:  /* cancel on bad input to reduce accidental bug reports */
	break;

      case 's':
      case PF4:
      case ctrl('M'):
      case ctrl('J'):
	if(F_ON(F_DISABLE_DFLT_IN_BUG_RPT,ps_global)
	   && (cmd == ctrl('M') || cmd == ctrl('J')))
	  break;

	if(cl && *cl && (*cl)->varname)
	  ourcmd = (*cl)->varname[0];
	
	break;
    }

    switch(ourcmd){
      case 'X':
	q_status_message(SM_ORDER, 0, 3, "Bug report cancelled.");
	goto bomb;

      case 'B':
	if(!ps->VAR_BUGS_ADDRESS){
	    q_status_message(SM_ORDER, 3, 3,
		"Bug report cancelled: no bug address configured.");
	    goto bomb;
	}

	sprintf(tmp, "%s%s%s%s%s",
		ps->VAR_BUGS_FULLNAME ? "\"" : "",
		ps->VAR_BUGS_FULLNAME ? ps->VAR_BUGS_FULLNAME : "",
		ps->VAR_BUGS_FULLNAME ? "\" <" : "",
		ps->VAR_BUGS_ADDRESS,
		ps->VAR_BUGS_FULLNAME ? ">" : "");

	strcpy(composer_title, "COMPOSE BUG REPORT");
	dprint(1, (debugfile, "\n\n    -- REPORTING BUG(%s) --\n", tmp));
	break;

      case 'S':
	if(!ps->VAR_SUGGEST_ADDRESS){
	    q_status_message(SM_ORDER, 3, 3,
		"Suggestion cancelled: no suggestion address configured.");
	    goto bomb;
	}

	sprintf(tmp, "%s%s%s%s%s",
		ps->VAR_SUGGEST_FULLNAME ? "\"" : "",
		ps->VAR_SUGGEST_FULLNAME ? ps->VAR_SUGGEST_FULLNAME : "",
		ps->VAR_SUGGEST_FULLNAME ? "\" <" : "",
		ps->VAR_SUGGEST_ADDRESS,
		ps->VAR_SUGGEST_FULLNAME ? ">" : "");

	strcpy(composer_title, "COMPOSE SUGGESTION");
	dprint(1, (debugfile, "\n\n    -- Sending suggestion(%s) --\n", tmp));
	break;

      case 'L':
	if(!ps->VAR_LOCAL_ADDRESS){
	    q_status_message(SM_ORDER, 3, 3,
		"Cancelled: no local support address configured.");
	    goto bomb;
	}

	sprintf(tmp, "%s%s%s%s%s",
		ps->VAR_LOCAL_FULLNAME ? "\"" : "",
		ps->VAR_LOCAL_FULLNAME ? ps->VAR_LOCAL_FULLNAME : "",
		ps->VAR_LOCAL_FULLNAME ? "\" <" : "",
		ps->VAR_LOCAL_ADDRESS,
		ps->VAR_LOCAL_FULLNAME ? ">" : "");

	strcpy(composer_title, "COMPOSE TO LOCAL SUPPORT");
	dprint(1, (debugfile, "\n\n   -- Send to local support(%s) --\n", tmp));
	break;
    
      default:
	break;
    }

    outgoing = mail_newenvelope();
    rfc822_parse_adrlist(&outgoing->to, tmp, ps->maildomain);
    outgoing->message_id = generate_message_id(ps);

    /*
     * Build our contribution to the subject; part constant string
     * and random 4 character alpha numeric string.
     */
    tmp_20k_buf[0] = '\0';
    if(ourcmd == 'B' || ourcmd == 'S')
	sprintf(tmp_20k_buf, "%cug (ID %c%c%d%c%c): ", ourcmd,
	    ((i = (int)(random() % 36L)) < 10) ? '0' + i : 'A' + (i - 10),
	    ((i = (int)(random() % 36L)) < 10) ? '0' + i : 'A' + (i - 10),
	    (int)(random() % 10L),
	    ((i = (int)(random() % 36L)) < 10) ? '0' + i : 'A' + (i - 10),
	    ((i = (int)(random() % 36L)) < 10) ? '0' + i : 'A' + (i - 10));
    outgoing->subject = cpystr(tmp_20k_buf);

    body       = mail_newbody();

    if(ourcmd != 'B'){
	body->type = TYPETEXT;
	/* Allocate an object for the body */
	if(body->contents.binary=(void *)so_get(PicoText,NULL,EDIT_ACCESS)){
	    if((sig = get_signature()) && *sig){
		so_puts((STORE_S *)body->contents.binary, sig);
		fs_give((void **)&sig);
	    }
	}
	else{
	    q_status_message(SM_ORDER | SM_DING, 3, 4, err);
	    goto bomb;
	}
    }
    else{  /* ourcmd == 'B' */
	body->type = TYPEMULTIPART;
	/*---- The TEXT part/body ----*/
	body->contents.part            = mail_newbody_part();
	body->contents.part->body.type = TYPETEXT;
	/* Allocate an object for the body */
	if(body->contents.part->body.contents.binary = 
				    (void *)so_get(PicoText,NULL,EDIT_ACCESS)){
	    pp = &(body->contents.part->next);
	    if((sig = get_signature()) && *sig){
		so_puts((STORE_S *)body->contents.part->body.contents.binary,
		    sig);
		fs_give((void **)&sig);
	    }
	}
	else{
	    q_status_message(SM_ORDER | SM_DING, 3, 4, err);
	    goto bomb;
	}

	/*---- create object, and write current config into it ----*/
	*pp			     = mail_newbody_part();
	pb			     = &((*pp)->body);
	pp			     = &((*pp)->next);
	pb->type		     = TYPETEXT;
	pb->id		     = generate_message_id(ps);
	pb->description          = cpystr("Pine Configuration Data");
	pb->parameter	     = mail_newbody_parameter();
	pb->parameter->attribute = cpystr("name");
	pb->parameter->value     = cpystr("config.txt");
	pb->contents.msg.env     = NULL;
	pb->contents.msg.body    = NULL;

	if(pb->contents.binary = (void *) so_get(CharStar, NULL, EDIT_ACCESS)){
	    extern char datestamp[], hoststamp[];

	    gf_set_so_writec(&pc, (STORE_S *)pb->contents.binary);
	    gf_puts("Pine built ", pc);
	    gf_puts(datestamp, pc);
	    gf_puts(" on host: ", pc);
	    gf_puts(hoststamp, pc);
	    gf_puts("\n", pc);
	    dump_pine_struct(ps, pc);
	    dump_config(ps, pc);
	    /* dump last n keystrokes */
	    gf_puts("========== Latest keystrokes ==========\n", pc);
	    while((i = key_recorder(0, 1)) != -1){
		sprintf(tmp, "\t%s\t(0x%04.4x)\n", pretty_command(i), i);
		gf_puts(tmp, pc);
	    }

	    pb->size.bytes = strlen((char *)so_text(
					      (STORE_S *)pb->contents.binary));
	}
	else{
	    q_status_message(SM_ORDER | SM_DING, 3, 4, err);
	    goto bomb;
	}

	/* check for local debugging info */
	if(ps_global->VAR_BUGS_EXTRAS
	   && can_access(ps_global->VAR_BUGS_EXTRAS, EXECUTE_ACCESS) == 0){
	    char *error	         = NULL;
	    *pp		         = mail_newbody_part();
	    pb		         = &((*pp)->body);
	    pb->type	         = TYPETEXT;
	    pb->id		         = generate_message_id(ps);
	    pb->description	         = cpystr("Local Configuration Data");
	    pb->parameter	         = mail_newbody_parameter();
	    pb->parameter->attribute = cpystr("name");
	    pb->parameter->value     = cpystr("lconfig.txt");
	    pb->contents.msg.env     = NULL;
	    pb->contents.msg.body    = NULL;

	    if(pb->contents.binary =
			    (void *) so_get(CharStar, NULL, EDIT_ACCESS)){
		PIPE_S  *syspipe;
		gf_io_t  gc;

		gf_set_so_writec(&pc, (STORE_S *)pb->contents.binary);
		if(syspipe = open_system_pipe(ps_global->VAR_BUGS_EXTRAS,
					 NULL, NULL,
					 PIPE_READ | PIPE_STDERR | PIPE_USER)){
		    gf_set_readc(&gc, (void *)syspipe->in.f, 0, FileStar);
		    gf_filter_init();
		    error = gf_pipe(gc, pc);
		    (void) close_system_pipe(&syspipe);
		}
		else
		  error = "executing config collector";
	    }

	    if(error){
		q_status_message1(SM_ORDER | SM_DING, 3, 4,
				  "Problem %s", error);
		goto bomb;
	    }
	    else			/* fixup attachment's size */
	      pb->size.bytes = strlen((char *)so_text(
					  (STORE_S *)pb->contents.binary));
	}

	if(mn_get_total(ps->msgmap) > 0L){
	    ps->redrawer = att_cur_drawer;
	    att_cur_drawer();
	}

	if(mn_get_total(ps->msgmap) > 0L
	   && (ch = one_try_want_to("Attach current message to report",
				    'y','x',NO_HELP,0,1)) == 'y'){
	    *pp		      = mail_newbody_part();
	    pb		      = &((*pp)->body);
	    pb->type	      = TYPEMESSAGE;
	    pb->id		      = generate_message_id(ps);
	    sprintf(tmp, "Problem Message (%ld of %ld)",
		    mn_get_cur(ps->msgmap), mn_get_total(ps->msgmap));
	    pb->description	      = cpystr(tmp);

	    /*---- Package each message in a storage object ----*/
	    if(!(pb->contents.binary = (void *)so_get(PART_SO_TYPE, NULL,
						      EDIT_ACCESS))){
		q_status_message(SM_ORDER | SM_DING, 3, 4, err);
		goto bomb;
	    }

	    /* write the header */
	    if((p = mail_fetchheader(ps->mail_stream, msgno)) && *p)
	      so_puts((STORE_S *)pb->contents.binary, p);
	    else
	      goto bomb;

#if	defined(DOS) && !defined(WIN32)
	    /* write fetched text to disk */
	    mail_parameters(ps->mail_stream, SET_GETS, (void *)dos_gets);
	    append_file = (FILE *)so_text((STORE_S *)pb->contents.binary);

	    /* HACK!  See mailview.c:format_message for details... */
	    ps->mail_stream->text = NULL;
	    /* write the body */
	    if(!mail_fetchtext(ps->mail_stream, msgno))
	      goto bomb;

	    pb->size.bytes = ftell(append_file);
	    /* next time body may stay in core */
	    mail_parameters(ps->mail_stream, SET_GETS, (void *)NULL);
	    append_file   = NULL;
	    mail_gc(ps->mail_stream, GC_TEXTS);
	    so_release((STORE_S *)pb->contents.binary);
#else
	    pb->size.bytes = strlen(p);
	    so_puts((STORE_S *)pb->contents.binary, "\015\012");
	    if((p = mail_fetchtext(ps->mail_stream, msgno)) &&  *p)
	      so_puts((STORE_S *)pb->contents.binary, p);
	    else
	      goto bomb;

	    pb->size.bytes += strlen(p);
#endif
	}
	else if(ch == 'x'){
	    q_status_message(SM_ORDER, 0, 3, "Bug report cancelled.");
	    goto bomb;
	}
    }

    pine_send(outgoing, &body, composer_title, NULL, &fake_reply, NULL, NULL,
	      NULL, NULL, 0);

  bomb:
    ps->mangled_screen = 1;
    if(outgoing)
      mail_free_envelope(&outgoing);
    if(body)
      pine_free_body(&body);

    return(10);
}


static char att_cur_msg[] = "\
         Reporting a bug...\n\
\n\
  If you think that the \"current\" message may be related to the bug you\n\
  are reporting you may include it as an attachment.  If you want to\n\
  include a message but you aren't sure if it is the current message,\n\
  cancel this bug report, go to the folder index, place the cursor on\n\
  the message you wish to include, then return to the main menu and run\n\
  the bug report command again.  Answer \"Y\" when asked the question\n\
  \"Attach current message to report?\"\n\
\n\
  This bug report will also automatically include your pine\n\
  configuration file, which is helpful when investigating the problem.";

/*
 * Used by gripe_tool.
 */
void
att_cur_drawer()
{
    int	       i, dline, j;
    char       buf[256];

    /* blat helpful message to screen */
    ClearBody();
    j = 0;
    for(dline = 2;
	dline < ps_global->ttyo->screen_rows - FOOTER_ROWS(ps_global);
	dline++){
	for(i = 0; i < 256 && att_cur_msg[j] && att_cur_msg[j] != '\n'; i++)
	  buf[i] = att_cur_msg[j++];

	buf[i] = '\0';
	if(att_cur_msg[j])
	  j++;
	else if(!i)
	  break;

        PutLine0(dline, 1, buf);
    }
}
