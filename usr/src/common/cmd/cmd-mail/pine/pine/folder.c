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


   USENET News reading additions in part by L Lundblade / NorthWestNet, 1993
   lgl@nwnet.net

   Pine is in part based on The Elm Mail System:
    ***********************************************************************
    *  The Elm Mail System  -  Revision: 2.13                             *
    *                                                                     *
    * 			Copyright (c) 1986, 1987 Dave Taylor              *
    * 			Copyright (c) 1988, 1989 USENET Community Trust   *
    ***********************************************************************
 

  ----------------------------------------------------------------------*/

/*======================================================================
     folder.c

  Screen to display and manage all the users folders

This puts up a list of all the folders in the users mail directory on
the screen spacing it nicely. The arrow keys move from one to another
and the user can delete the folder or select it to change to or copy a
message to. The dispay lets the user scroll up or down a screen full,
or search for a folder name.
 ====*/


#include "headers.h"

#define	CLICKHERE	"[ Select Here to See Expanded List ]"
#define	CLICKHERETOO	"[ ** Empty List **  Select Here to Try Re-Expanding ]"
#define	CLICKHERETOONEWS \
	"[ ** Empty List **  Use \"A Subscribe\" to subscribe to a newsgroup ]"
#define	ALL_FOUND(X)	(((X)->use&CNTXT_NOFIND) == 0 && \
			  ((X)->use&CNTXT_PARTFIND) == 0)
#define	FLDR_NAME(X)	((X) ? ((X)->nickname ? (X)->nickname : (X)->name) :"")
#define	SUBSCRIBE_PMT	\
		       "Enter newsgroup name (or partial name to get a list): "
    

/*----------------------------------------------------------------------
   The data needed to redraw the folders screen, including the case where the 
screen changes size in which case it may recalculate the folder_display.
  ----*/

/* BUG: this strategy is doomed to fail when resize happens during 
 *      printing that refigures line numbers and such as they're
 *      stored with actual folder data (ie only one postition at a time).
 */
typedef struct folder_screen_state
{
    CONTEXT_S *context_list;		/* list of context's to display */
    CONTEXT_S *context;			/* current context              */
    int        folder_index;		/* current index in context     */
    CONTEXT_S *prev_context;		/* previous context              */
    int        prev_index;		/* perv index in prev context     */
    int        display_cols;		/* number of columns on display   */
    int        display_rows;		/* number of rows on display      */
    int	       top_row;			/* folder list row at top left    */
    int        last_row;		/* last row of folder list        */
} FSTATE_S;

static FSTATE_S *fs;

typedef enum {NotChecked, NotInCache, Found, Missing, End} NgCacheReturns;


/*
 * Global's used by context_mailbox and context_bboard to tie foldernames
 * returned by c-client into the proper folder list
 */
extern void       *find_folder_list;
extern MAILSTREAM *find_folder_stream;
extern long        find_folder_count;
extern int	   find_folder_inbox;
#ifdef NEWBB
extern void       *newbb_folder_list;
#endif


/* short definition to keep compilers happy */
typedef    int (*QSFunc) PROTO((const QSType *, const QSType *));


/*
 * Internal prototypes
 */
void       redraw_folder_screen PROTO(());
void       display_folder PROTO((FSTATE_S *, CONTEXT_S *, int, CONTEXT_S *, \
				 int));
int	   folder_scroll_up PROTO((long));
int	   folder_scroll_down PROTO((long));
int	   folder_scroll_to_pos PROTO((long));
void       paint_folder_name PROTO((int, FSTATE_S *, int, CONTEXT_S *));
char      *folder_list_entry PROTO((FSTATE_S *, int, int *, CONTEXT_S **));
void       folder_insert_index PROTO((FOLDER_S *, int, void *));
int        folder_total PROTO((void *));
int        off_folder_display PROTO((FSTATE_S *, int, CONTEXT_S *));
char      *add_new_folder PROTO((int, CONTEXT_S *));
char      *group_subscription PROTO((int, CONTEXT_S *));
char      *rename_folder PROTO((int, int, CONTEXT_S *));
int        delete_folder PROTO((int, CONTEXT_S *, int *));
void       print_folders PROTO((FSTATE_S *));
int        search_folders PROTO((FSTATE_S *, int));
int        compare_names PROTO((const QSType *, const QSType *));
int        compare_sizes PROTO((const QSType *, const QSType *));
int        compare_folders PROTO((const QSType *, const QSType *));
int        compare_folders_new PROTO((const QSType *, const QSType *));
void       create_folder_display PROTO((FSTATE_S *, int));
void      *new_folder_list PROTO(());
void       free_folder_list PROTO((void **));
void      *find_folder_names PROTO((char *, char *));
void       free_folders_in_context PROTO((CONTEXT_S *));
FOLDER_S  *new_folder PROTO((char *));
void       folder_delete PROTO((int, void *));
void       update_news_prefix PROTO((MAILSTREAM *,  struct folder *));
char      *get_post_list PROTO((char **));
int        sort_folder_list PROTO((void  *, QSFunc));
int	   folder_has_recent PROTO((MAILSTREAM **, CONTEXT_S *, FOLDER_S *));
NgCacheReturns chk_newsgrp_cache PROTO((char *));
void       add_newsgrp_cache PROTO((char *, NgCacheReturns));
#ifdef	_WINDOWS
int	   folder_scroll_callback PROTO((int,long));
#endif
#ifdef NEWBB
void       clear_new_groups PROTO((void *));
#endif



static struct key folder_keys[] =
       {{"?","Help",KS_SCREENHELP},  	{"O","OTHER CMDS",KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{"P","PrevFldr",KS_NONE},	{"N","NextFldr",KS_NONE},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{"D","Delete",KS_NONE},		{"A","Add",KS_NONE},
	{"R","Rename",KS_NONE},		{"W","WhereIs",KS_NONE},

	{"?","Help",KS_NONE},		{"O","OTHER CMDS",KS_NONE},
	{"Q","Quit",KS_EXIT},		{"C","Compose",KS_COMPOSER},
	{NULL,NULL,KS_NONE},		{"G","GotoFldr",KS_GOTOFLDR},
	{"I","CurIndex",KS_FLDRINDEX},	{"W","WhereIs",KS_WHEREIS},
	{"Y","prYnt",KS_PRINT},		{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE}};
INST_KEY_MENU(folder_keymenu, folder_keys);
#define	MAIN_KEY  	2  /* Sometimes Main, sometimes Exit */
#define	SELECT_KEY	3  /* Sometimes View, sometimes Select */
#define	OTHER_KEY	1
#define	DELETE_KEY	8
#define	ADD_KEY	        9
#define	RENAME_KEY	10
#define	WHEREIS_KEY	11



/*----------------------------------------------------------------------
      Front end to folder lister when it's called from the main menu

 Args: pine_state -- The general pine_state data structure

 Result: runs folder_lister

  ----*/
void
folder_screen(pine_state)
    struct pine *pine_state;
{
    dprint(1, (debugfile, "=== folder_screen called ====\n"));
    mailcap_free(); /* free resources we won't be using for a while */
    pine_state->next_screen = main_menu_screen;
    folder_lister(pine_state, FolderMaint, NULL, NULL, NULL, NULL,
                  pine_state->context_list, NULL);
    pine_state->prev_screen = folder_screen;
}



/*----------------------------------------------------------------------
         Browse folders for ^T selection from the composer
  
 Args: error_mess -- pointer to place to return an error message
  
 Returns: result if folder selected, NULL if not
	  Composer expects the result to be alloc'd here 
 
  ----*/     
char *
folders_for_fcc(error_mess)
     char **error_mess;
{
    char       tmp[MAXPATH], *rs = NULL;
    int        rv;
    CONTEXT_S *cntxt = ps_global->context_current;

    /* Coming back from composer */
    fix_windsize(ps_global);
    init_sigwinch();

    /*
     * if folder_lister returns ambiguous name IF the chosen context
     * matches the default save context, OTHERWISE it returns the 
     * selected folder's fully qualified name.
     *
     * Note: this could stand to be cleaned up.  That is, it would be
     * nice for the user to be shown the context along with the 
     * folder name rather than the raw IMAP name.  Perhaps, someday
     * adding understanding of nicknames perhaps with <nickname>folder
     * syntax similar to the titlebar makes sense...
     */
    if(rv = folder_lister(ps_global, GetFcc, NULL, &cntxt, tmp, NULL,
			  ps_global->context_list, NULL)){
	if(context_isambig(tmp) && !((cntxt->use) & CNTXT_SAVEDFLT)){
	    context_apply(rs = tmp_20k_buf, cntxt->context, tmp);

	    /*
	     * A bit of a problem here.  If the foldername applied
	     * to the context is still relative, we can probably assume 
	     * the folder's relative to ~, so add it here for now.
	     * NOTE: this WILL break if a local driver is added, that
	     * isn't file system based...
	     */
	    if(context_isambig(rs))
	      build_path(rs = tmp, ps_global->ui.homedir, tmp_20k_buf);
	}
	else
	  rs = tmp;
    }

    return(rs ? cpystr(rs) : NULL);
}

    

/*----------------------------------------------------------------------
      Main routine for the list of folders screen, displays and execute cmds.

  Args:  ps            -- The pine_state data structure
         do_what       -- What function we're called as -- select, maint...
         return_string -- Buffer to return selected folder name in
         return_array  -- Return *alloced* array of selected folder names here
	 return_context -- Context that the return_string makes is applied to
	 start_context -- Context to first display

  Result: return 0 for abort, 1 for open folder, and 2 for save folder
          The name selected is copied into the given buffer

This code assume that the folder list will have one folder in it and
is likey to crash if there isn't one.

  ----*/
int 
folder_lister(ps, do_what, start_context, return_context, return_string,
              return_array, context_list, f_state)
    struct pine *ps;
    FolderFun    do_what;
    CONTEXT_S   *start_context;
    CONTEXT_S  **return_context;
    CONTEXT_S   *context_list;
    char        *return_string;
    char      ***return_array;
    void        *f_state;
{
    int              ch, orig_ch, mangled_footer, mangled_header, km_size,
                     rv, quest_line, rc, cur_row, cur_col, km_popped,
		     doing_listmode;
    unsigned short   new_col;
    CONTEXT_S       *tc;
    char            *new_file, *cur_name, *new_fold;
    FOLDER_S        *cur_f;
    struct key_menu *km;
    bitmap_t         bitmap;
    OtherMenu        what;
    FSTATE_S         default_fstate;

    dprint(1, (debugfile, "\n\n    ---- FOLDER SCREEN ----\n"));

    fs                 = f_state ? (FSTATE_S *)f_state : &default_fstate;
    quest_line         = -FOOTER_ROWS(ps);
    fs->prev_index     = -1;
    fs->prev_context   = NULL;
    fs->top_row	       = 0;
    mangled_footer     = 1;
    mangled_header     = 1;
    what               = FirstMenu;
    km_popped          = 0;
    doing_listmode     = 0;
    fs->context_list   = context_list;
    fs->context        = start_context ? start_context 
				       : (ps->context_current) 
					 ? ps->context_current
					 : context_list;

    /*
     * The subscription handler's already done the find, so
     * just expand if it's the default, there's only one context,
     * or we're here to select something...
     */
    if(do_what != Subscribe){
	if(F_ON(F_EXPANDED_FOLDERS,ps_global) || !context_list->next){
	    for(tc = context_list; tc ; tc = tc->next)
	      find_folders_in_context(NULL, tc, NULL);
	}
	else if(do_what != FolderMaint && do_what != Subscribe)
	  find_folders_in_context(NULL, fs->context, NULL);
    }

    if(do_what == Subscribe || do_what == PostNews
       || (fs->folder_index = folder_index(ps->cur_folder, 
					   fs->context->folders)) < 0)
      fs->folder_index = 0;

    ClearBody();

    create_folder_display(fs, ps->ttyo->screen_cols);

    ps->redrawer = redraw_folder_screen;

    for(ch = 'y' /* For display_message first time through */;;) {

	if(km_popped){
	    km_popped--;
	    if(km_popped == 0){
		clearfooter(ps);
		fs->prev_index = -1;
	    }
	}

        /*------------ New mail check ----------*/
        if(ps->ttyo->screen_rows > 1 && new_mail(0, NM_TIMING(ch), 1) >= 0)
	  mangled_header = 1;

        if(streams_died())
          mangled_header = 1;

        /*----------  screen painting -------------*/
	if(mangled_header && ps->ttyo->screen_rows > 0) {
	    mangled_header = 0;
            switch(do_what) {
              case FolderMaint:
	        set_titlebar("FOLDER LIST", ps->mail_stream,
			     ps->context_current,ps->cur_folder,
			     ps->msgmap, 1, FolderName, 0, 0);
                break;

              case OpenFolder:
	        set_titlebar("GOTO: SELECT FOLDER", ps->mail_stream,
			     ps->context_current, ps->cur_folder,
			     ps->msgmap, 1, FolderName, 0, 0);
                break;

              case SaveMessage:
	        set_titlebar("SAVE: SELECT FOLDER", ps->mail_stream,
			     ps->context_current, ps->cur_folder, ps->msgmap,
			     1, MessageNumber, 0, 0);
                break;

              case GetFcc:
                set_titlebar("FCC: SELECT FOLDER", ps->mail_stream,
			     ps->context_current, ps->cur_folder, ps->msgmap,
			     1, FolderName, 0, 0);
                break;

              case Subscribe:
                set_titlebar("SUBSCRIBE: SELECT FOLDER", ps->mail_stream,
			     ps->context_current, ps->cur_folder, ps->msgmap,
			     1, FolderName, 0, 0);
                break;

              case PostNews:
                set_titlebar("NWSGRP: SELECT GROUP", ps->mail_stream,
			     ps->context_current, ps->cur_folder, ps->msgmap,
			     1, FolderName, 0, 0);
                break;
            }
	}

	/*
	 * display_folders handles all display painting.
	 * only the first time thru (or if screen parms change)
	 * do we need to explicitly tell it to redraw.  Otherwise
	 * it handles framing the page and highlighting the current
	 * folder...
	 */
	if(fs->prev_index != fs->folder_index
	   || fs->prev_context != fs->context
	   || (do_what == Subscribe && ch == 'x')
	   || (do_what == Subscribe && doing_listmode
	       && (ch == ctrl('M') || ch == ctrl('J') || ch == KEY_MOUSE)))
	  display_folder(fs, fs->context, fs->folder_index,
			 fs->prev_context, fs->prev_index);

	if(fs->prev_context && fs->prev_context != fs->context){
	    q_status_message1(SM_ORDER, 0, 3, "Now in collection <%s>", 
			      fs->context->label[0]);

	    if(do_what == FolderMaint && (fs->prev_context 
		   && ((fs->prev_context->use&CNTXT_PSEUDO)
		       != (fs->context->use&CNTXT_PSEUDO))))
	      mangled_footer++;
            if((fs->prev_context->type & FTYPE_BBOARD) ^
               (fs->context->type & FTYPE_BBOARD))
              mangled_footer++;
	}

	if(mangled_footer && ps->ttyo->screen_rows > HEADER_ROWS(ps) + 1) {
	    setbitmap(bitmap);
	    km = &folder_keymenu;
            folder_keys[DELETE_KEY].name = "D";
            folder_keys[ADD_KEY].name = "A";
            folder_keys[RENAME_KEY].name = "R";
	    folder_keys[RENAME_KEY].label = "Rename";
            if(fs->context->type & FTYPE_BBOARD) {
                folder_keys[ADD_KEY].label = "Subscribe";
                folder_keys[DELETE_KEY].label = "UnSbscrbe";
		KS_OSDATASET(&folder_keys[DELETE_KEY], KS_NONE);
            } else {
                folder_keys[ADD_KEY].label = "Add";
                folder_keys[DELETE_KEY].label = "Delete";
		KS_OSDATASET(&folder_keys[DELETE_KEY], KS_NONE);
            }
	    if(do_what == FolderMaint){
	      km->how_many = 2;
	      folder_keys[MAIN_KEY].name = "M";
	      folder_keys[MAIN_KEY].label = "Main Menu";
	      KS_OSDATASET(&folder_keys[MAIN_KEY], KS_MAINMENU);
	      folder_keys[SELECT_KEY].name = "V";
	      folder_keys[SELECT_KEY].label = 
		   (fs->context->use & CNTXT_PSEUDO) ? "[Select]":"[ViewFldr]";
	      KS_OSDATASET(&folder_keys[SELECT_KEY], KS_NONE);
	      clrbitn(WHEREIS_KEY, bitmap); /* the one in the 1st menu */
	    }
	    else {
	      km->how_many = 1;
	      folder_keys[MAIN_KEY].name = "E";
	      folder_keys[MAIN_KEY].label = do_what != Subscribe ? 
                                               "ExitSelect" : "ExitSubscb";

	      folder_keys[SELECT_KEY].name = "S";
	      folder_keys[SELECT_KEY].label = do_what != Subscribe ?
                                               "[Select]" : "[Subscribe]";
	      KS_OSDATASET(&folder_keys[SELECT_KEY], KS_NONE);
	      clrbitn(OTHER_KEY, bitmap);
	      clrbitn(RENAME_KEY, bitmap);
	      if(do_what == Subscribe){
		  if(doing_listmode){
		      folder_keys[DELETE_KEY].name = "X";
		      folder_keys[DELETE_KEY].label = "[Set/Unset]";
		      folder_keys[SELECT_KEY].label = "Subscribe";
		  }
		  else{
		      folder_keys[DELETE_KEY].name = "L";
		      folder_keys[DELETE_KEY].label = "ListMode";
		  }
	      }
	      else
	        clrbitn(DELETE_KEY, bitmap);

#ifdef NEWBB
              if(do_what == Subscribe) {
                  folder_keys[ADD_KEY].name = "X";
                  folder_keys[ADD_KEY].label = "DismissNew";
              } else {
 	          clrbitn(ADD_KEY, bitmap);
              }
#else
	      clrbitn(ADD_KEY, bitmap);
#endif

	    }

	    if(km_popped){
		FOOTER_ROWS(ps) = 3;
		clearfooter(ps);
	    }

	    draw_keymenu(km, bitmap, ps->ttyo->screen_cols,
		1-FOOTER_ROWS(ps), 0, what, 0);
	    mangled_footer = 0;
	    what           = SameTwelve;
	    if(km_popped){
		FOOTER_ROWS(ps) = 1;
		mark_keymenu_dirty();
	    }
	}

        fs->prev_index   = fs->folder_index;
        fs->prev_context = fs->context;
	if(folder_total(fs->context->folders)){
	    cur_f = folder_entry(fs->folder_index, fs->context->folders);
	    cur_name = FLDR_NAME(cur_f);
	}
	else
	 cur_f  = NULL;

        /*------- display any status messages -----*/
	if(km_popped){
	    FOOTER_ROWS(ps) = 3;
	    mark_status_unknown();
	}

	display_message(ch);
	if(km_popped){
	    FOOTER_ROWS(ps) = 1;
	    mark_status_unknown();
	}

	if(F_ON(F_SHOW_CURSOR, ps) && cur_f){
	    cur_row = HEADER_ROWS(ps) + (cur_f->d_line - fs->top_row);
	    cur_col = cur_f->d_col;
	}
	else{
	    cur_row = ps->ttyo->screen_rows - FOOTER_ROWS(ps);
	    cur_col = 0;
	}

        /*----- Read and validate the next command ------*/
	MoveCursor(cur_row, cur_col);

#ifdef	MOUSE
	mouse_in_content(KEY_MOUSE, -1, -1, 0, 0);/* prime the handler */
	register_mfunc(mouse_in_content, HEADER_ROWS(ps_global), 0,
		      ps_global->ttyo->screen_rows-(FOOTER_ROWS(ps_global)+1),
		      ps_global->ttyo->screen_cols);
#endif
#ifdef	_WINDOWS
	mswin_setscrollcallback(folder_scroll_callback);
#endif
	ch = read_command();
#ifdef	MOUSE
	clear_mfunc(mouse_in_content);
#endif
#ifdef	_WINDOWS
	mswin_setscrollcallback(NULL);
#endif
        orig_ch = ch;
        
	if(ch < 'z' && isupper((unsigned char)ch))
	  ch = tolower((unsigned char)ch);

	if(km->which == 1)
          if(ch >= PF1 && ch <= PF12)
            ch = PF2OPF(ch);

	ch = validatekeys(ch);

        dprint(5, (debugfile, "folder command: %c (%d)\n",ch,ch));

	if(km_popped)
	  switch(ch){
	    case NO_OP_IDLE:
	    case NO_OP_COMMAND: 
	    case PF2:
	    case OPF2:
	    case 'o' :
	    case KEY_RESIZE:
	    case ctrl('L'):
	      km_popped++;
	      break;
	    
	    default:
	      clearfooter(ps);
	      break;
	  }

        /*----------- Execute command --------------*/
	switch(ch) {

            /*---------- display other key bindings ------*/
          case PF2:
          case OPF2:
          case 'o' :
            if(do_what != FolderMaint)
              goto bleep;

            if (ch == 'o')
	      warn_other_cmds();
            what = NextTwelve;
            mangled_footer++;
            break;


            /*---------------------- Key left --------------*/
	  case ctrl('B'):
	  case KEY_LEFT:
	  case PF5:
	  case 'p':
	    if(fs->folder_index > 0 && ALL_FOUND(fs->context)){
		fs->folder_index--;
	    }
	    else if(fs->context_list != fs->context){
		for(tc = fs->context_list; tc->next != fs->context;
		    tc = tc->next);

		fs->context = tc;
		if(ALL_FOUND(fs->context) && folder_total(fs->context->folders))
		  fs->folder_index = folder_total(fs->context->folders) - 1;
		else
		  fs->folder_index = 0;
	    }
	    else
	      q_status_message(SM_ORDER,0,1,"Already on first folder.");

	    break;
 

            /*--------------------- Key right -------------------*/
          case ctrl('F'): 
          case KEY_RIGHT:
	  case PF6:
	  case 'n':
          case '\t':
	    if(fs->folder_index + 1 < folder_total(fs->context->folders)
	       && ALL_FOUND(fs->context)){
		fs->folder_index++;
	    }
	    else if(fs->context->next){	/* next context?*/
		fs->context = fs->context->next;
		fs->folder_index = 0;
	    }
	    else
	      q_status_message(SM_ORDER,0,1,"Already on last folder.");

            break;


            /*--------------- Key up ---------------------*/
          case KEY_UP:
	  case ctrl('P'):
	    if(!folder_total(fs->context->folders))
	      break;

	    new_col = folder_entry(fs->folder_index, 
				   fs->context->folders)->d_col;
	    rc	    = folder_entry(fs->folder_index, 
				   fs->context->folders)->d_line - 1;

	    /* find next line */
	    while(rc >= 0 && folder_list_entry(fs, rc, &(fs->folder_index),
					       &(fs->context)))
	      rc--;

	    if(rc < 0){
		q_status_message(SM_ORDER, 0, 1, "Already on first line.");
		if(fs->top_row != 0){		/* make sure! */
		    fs->top_row    = 0;
		    fs->prev_index = -1;
		}

		break;
	    }

	    /* find the right column on it */
	    while((cur_f = folder_entry(fs->folder_index, 
				     fs->context->folders))->d_col < new_col){
		if(fs->folder_index+1 >= folder_total(fs->context->folders)
		   || folder_entry(fs->folder_index + 1,
				   fs->context->folders)->d_col == 0)
		  break;
		else
		  fs->folder_index++;
	    }

	    if((rv = fs->top_row - cur_f->d_line + HS_MARGIN(ps)) > 0){
		int i;

		for(i=cur_f->d_line-1, rc=fs->folder_index, tc=fs->context;
		    i >= 0 && folder_list_entry(fs, i, &rc, &tc);
		    i--, rv++)
		  ;

		folder_scroll_down(rv);
		fs->prev_index = -1;
	    }

            break;


            /*----------------- Key Down --------------------*/
          case KEY_DOWN:
	  case ctrl('N'):
	    if(!folder_total(fs->context->folders))
	      break;

	    new_col = folder_entry(fs->folder_index, 
				   fs->context->folders)->d_col;
	    rc      = folder_entry(fs->folder_index, 
				   fs->context->folders)->d_line + 1;

	    if(rc > fs->last_row){
		q_status_message(SM_ORDER, 0, 1, "Already on last line.");
		break;
	    }

	    /* find next line */
	    while(rc <= fs->last_row
		  && folder_list_entry(fs, rc, &(fs->folder_index),
				       &(fs->context)))
	      rc++;

	    /* find the right column on it */
	    while(folder_entry(fs->folder_index,
			       fs->context->folders)->d_col < new_col){
		if(fs->folder_index+1 >= folder_total(fs->context->folders)
		   || folder_entry(fs->folder_index + 1, 
				   fs->context->folders)->d_col == 0)
		  break;
		else
		  fs->folder_index++;
	    }

	    if((rc = folder_entry(fs->folder_index,
				  fs->context->folders)->d_line
				  - (fs->top_row + fs->display_rows
				     - HS_MARGIN(ps)) + 1) > 0){
		fs->prev_index = -1;
		folder_scroll_up(rc);
	    }

	    break;


            /*----------------- Beginning of Line --------------------*/
	  case ctrl('A'):
	    while(1){
		if(fs->folder_index == 0
		   || folder_entry(fs->folder_index, 
				   fs->context->folders)->d_col == 0)
		  break;
		else
		  fs->folder_index--;
	    }

	    break;


            /*----------------- End of Line --------------------*/
	  case ctrl('E'):
	    while(1){
		if(fs->folder_index+1 >= folder_total(fs->context->folders)
		   || folder_entry(fs->folder_index + 1, 
				   fs->context->folders)->d_col == 0)
		  break;
		else
		  fs->folder_index++;
	    }

	    break;


#ifdef	MOUSE
            /*-------------- Mouse down in content ----------------------*/
	  case KEY_MOUSE:
	    {
		MOUSEPRESS mp;
		int new_index, ncol, tcol;
		CONTEXT_S	*new_context;

		mouse_get_last (NULL, &mp);
		mp.row -= HEADER_ROWS(ps);

		/* Mouse down below last clickable line? */
		if (mp.row > fs->last_row)
		  break;

		/* Clicked on one of those seperator lines? */
		if (folder_list_entry(fs, mp.row + fs->top_row,
				      &new_index, &new_context))
		  break;

		/* folder_list_entry should have given us a new context
		 * and index for first folder on line.  Find folder under
		 * the mouse click by scanning across the line. */
		while((tcol = folder_entry(new_index, 
			    new_context->folders)->d_col) < mp.col) {
		    /* Reached end of folders?  */
		    if (new_index+1 >= folder_total(new_context->folders))
		      break;
		    /* Reached end of line? */
		    if ((ncol = folder_entry(new_index + 1, 
					new_context->folders)->d_col) == 0)
		      break;
		    /* Mouse click with in column range? */
		    if (mp.col >= tcol && mp.col < ncol)
		      break;
		    ++new_index;
		}

		/*
		 * On single click we move to new item.
		 * On double click we test that we are on highlighted item
		 * and the take action on it.
		 */
		if (!mp.doubleclick) {
		    fs->folder_index = new_index;
		    fs->context = new_context;
		}
		else {
		    if(do_what == Subscribe && doing_listmode){
			goto DoToggle;  /* different default in this case */
		    }
		    else if(fs->folder_index == new_index
			    && fs->context == new_context) 
		      goto DoSelect;
		}
	    }
	    break;
#endif	/* MOUSE */


            /*--------------Scroll Up ----------------------*/
          case PF7: 
	  case KEY_PGUP:
          case ctrl('Y'): 
	  case '-':
	    if(fs->top_row
		|| (fs->folder_index > 0 && ALL_FOUND(fs->context))
	        || (fs->context_list != fs->context)){
		rc = max(0, fs->top_row - fs->display_rows);

		while(folder_list_entry(fs, rc, &(fs->folder_index), 
					&(fs->context)))
		      rc++;
	    } else {
                q_status_message(SM_ORDER,0,1,"Already on first page.");
	    }

            break;


            /*---------- Scroll screenful ------------*/
	  case PF8:
	  case KEY_PGDN:
	  case SPACE: 
          case ctrl('V'): 
	  case '+':
	    if(!folder_total(fs->context->folders))
	      break;

	    if((rc = fs->top_row + fs->display_rows) > fs->last_row){
		if((int)folder_entry(fs->folder_index,
				fs->context->folders)->d_line >= fs->last_row){
		    q_status_message(SM_ORDER,0,1,"Already on last page.");
		    break;
		}
		else
		  rc = fs->last_row;
	    }

	    while(rc <= fs->last_row
		  && folder_list_entry(fs, rc, &(fs->folder_index),
				       &(fs->context)))
	      rc++;

	    break;


            /*------------------ Help ----------------------*/
	  case PF1:
	  case OPF1:
	  case '?':
	  case ctrl('G'):
	    if(FOOTER_ROWS(ps) == 1 && km_popped == 0){
		km_popped = 2;
		mangled_footer = 1;
		break;
	    }

            ps_global->next_screen = SCREEN_FUN_NULL;
            ps_global->redrawer = (void (*)())NULL;
	    km_size = FOOTER_ROWS(ps_global);
            switch(do_what) {
              case FolderMaint:
                helper(h_folder_maint, "HELP FOR FOLDERS", 0);
                break;
              case OpenFolder:
                helper(h_folder_open, "HELP FOR OPENING FOLDERS", 0);
                break;
              case SaveMessage:
                helper(h_folder_save,"HELP FOR SAVING MESSAGES TO FOLDERS", 0);
                break;
              case GetFcc:
                helper(h_folder_fcc, "HELP FOR SELECTING THE FCC", 1);
                break;
              case Subscribe:
                helper(h_folder_subscribe,
			    "HELP SELECTING NEWSGROUP TO SUBSCRIBE TO", 1);
                break;
              case PostNews:
                helper(h_folder_postnews,
			    "HELP FOR SELECTING NEWSGROUP TO POST TO", 1);
                break;
            }
            if(ps_global->next_screen != SCREEN_FUN_NULL) {
                /* So "m" to go back to main menu works */
	        return(0);
            }
            ps_global->redrawer = redraw_folder_screen;
	    mangled_header++;
	    mangled_footer++;
	    fs->prev_index = -1;
	    if(km_size != FOOTER_ROWS(ps_global))  /* keymenu came or went */
	      create_folder_display(fs, ps_global->ttyo->screen_cols);

	    break;


            /*---------- Select or View ----------*/
          case ctrl('M') :
          case ctrl('J') :
          case PF4:
          case 'v':
          case 's':
	    if((ch == ctrl('M') || ch == ctrl('J'))
	       && do_what == Subscribe && doing_listmode)
	      goto DoToggle;  /* different default in this case */
DoSelect:
	    if((do_what != FolderMaint && ch == 'v') 
	       || (do_what == FolderMaint && ch == 's'))
	      goto bleep;
	    if(!folder_total(fs->context->folders)){
		q_status_message(SM_ORDER | SM_DING, 3, 3, 
			      "Empty folder collection.  Nothing to select!");
	    } else if(!ALL_FOUND(fs->context) 
		      || (fs->context->use & CNTXT_PSEUDO)){
		if(fs->context->use & CNTXT_PSEUDO){
		    folder_delete(0, fs->context->folders);
		    fs->context->use &= ~CNTXT_PSEUDO;
		}

		if(!folder_total(fs->context->folders))
		  fs->context->use |= CNTXT_NOFIND; /* ok to try find */

		find_folders_in_context(NULL, fs->context, NULL);

		if(fs->context == ps->context_current
		   && (fs->folder_index = folder_index(ps->cur_folder, 
						    fs->context->folders)) < 0)
		  fs->folder_index = 0;

		create_folder_display(fs, ps_global->ttyo->screen_cols);

		fs->prev_index = -1;		  /* redraw display */

		if(do_what == FolderMaint)
		  mangled_footer++;
	    } else if(do_what == FolderMaint) {
		
                /*--- Open folder ---*/
		if(cur_name == NULL)
		  break;

                if(do_broach_folder(cur_name, fs->context) == 1) {
		    for(tc = fs->context_list; tc ; tc = tc->next)
		      free_folders_in_context(tc);

                    ps_global->redrawer = (void(*)())NULL;
                    ps_global->next_screen = mail_index_screen;
                    return(1); 
                }

                /* Open Failed. Message will be issued by do_broach_folder. */
                /* Stay here in folder lister and let the user try again. */
		mangled_footer++;
                break;
            } else {
                /*-- save message, subscribe or post --- */
                if((do_what == GetFcc || do_what == SaveMessage)
		   && (fs->context->type & FTYPE_BBOARD)) {
                    q_status_message(SM_ORDER | SM_DING, 3, 4,
		     "Can't save messages to bulletin boards or news groups!");
                    break;
                }

	        if(cur_name == NULL){
		    rv = 0;
	        }
		else{
		    if(do_what == Subscribe){
			int i, n=0, folder_n;

			if(return_context)
			  *return_context = fs->context;

			folder_n = folder_total(fs->context->folders);
			/* count X's */
			if(doing_listmode)
			  for(i=0; i < folder_n; i++)
			    if(folder_entry(i,
				    fs->context->folders)->prefix[1] == 'X')
			      n++;

			if(n == 0 && !strncmp(folder_entry(fs->folder_index,
				      fs->context->folders)->prefix,"SUB",3)){
			    q_status_message1(SM_ORDER,0,4,
				"Already subscribed to \"%s\"",
				cur_name == NULL ? "" : cur_name);
			    break;
			}

			if(n == 0 && doing_listmode){
			    q_status_message(SM_ORDER,0,1,
				"Use \"X\" to mark groups to subscribe to");
			    break;
			}

			/* if more than one name, return in the array */
			if(return_array && n > 1){
			    char question_buf[80];
			    int ans;

			    sprintf(question_buf, "Subscribe to %d new groups",
				    n);
			    if(F_OFF(F_SELECT_WO_CONFIRM,ps_global)
			      && (ans=want_to(question_buf,'n','x',NO_HELP,0,0))
				    != 'y'){
				if(ans == 'x')
				  q_status_message(SM_ORDER,0,1,
			    "Use \"ExitSubscb\" to cancel subscribe command");

				mangled_footer++;
				break;
			    }

			    *return_array = (char **)fs_get(sizeof(char *)
							    * (n+1));
			    memset((void *)*return_array,0,
				    sizeof(char *)*(n+1));
			    for(i=0,n=0; i < folder_n; i++){
				if(folder_entry(i,
				    fs->context->folders)->prefix[1] == 'X'){
				    (*return_array)[n++]
					= cpystr(folder_entry(i,
						  fs->context->folders)->name);
				}
			    }

			    rv = 17;
			}
			else if(return_string){
			    strcpy(return_string, cur_name);
			    rv = 1;
			}
			else
			  rv = 0;
		    }
		    else{
			if(return_context)
			  *return_context = fs->context;

			if(return_string)
			  strcpy(return_string,
				 (do_what == GetFcc && cur_f)
				   ? cur_f->name : cur_name);

			rv = 1;
			dprint(5, (debugfile,
				  "return \"%s\" in context \"%s\"\n",
				  (return_string) ? return_string : "NULL", 
				  (return_context) ? (*return_context)->context 
						    : "NULL"));
		    }
	        }

                if(do_what != PostNews)
		  for(tc = fs->context_list; tc ; tc = tc->next)
		    free_folders_in_context(tc);

                ps_global->redrawer = (void (*)())NULL;
	        return(rv);
            }

	    break;


            /*--------- Hidden "To Fldrs" command -----------*/
	  case 'l':
	  ListModeOn:
	    if(do_what == FolderMaint)
	      q_status_message(SM_ORDER, 0, 3, "Already in Folder List");
	    else if(do_what == Subscribe && !doing_listmode){
		int i, folder_n;

		doing_listmode++;
		folder_n = folder_total(fs->context->folders);
		for(i=0; i < folder_n; i++){
		    if(strncmp(folder_entry(i,fs->context->folders)->prefix,
			"SUB", 3) != 0){
			folder_entry(i, fs->context->folders)->prefix[0] = '[';
			folder_entry(i, fs->context->folders)->prefix[2] = ']';
		    }
		}

		redraw_folder_screen();
		q_status_message(SM_ORDER,0,1,
		    "Use \"X\" to mark groups to subscribe to");
		mangled_footer++;
	    }
	    else
	      goto bleep;

	    break;

    
            /*--------- EXIT menu -----------*/
	  case PF3:
	  case 'm':
	  case 'e':
	    if((do_what != FolderMaint && ch == 'm') 
	       || (do_what == FolderMaint && ch == 'e'))
	      goto bleep;

	    if(doing_listmode){
		int i, folder_n;

		folder_n = folder_total(fs->context->folders);

		/* any X's? */
		for(i=0; i < folder_n; i++)
		  if(folder_entry(i, fs->context->folders)->prefix[1] == 'X')
		    break;

		if(i < folder_n		/* some selections have been made */
		   && want_to("Really abandon your selections ",
			      'y', 'x', NO_HELP, 0, 0) != 'y'){
		    mangled_footer++;
		    break;
		}
	    }

	    ps_global->redrawer = (void (*)())NULL;
	    for(tc = fs->context_list; tc ; tc = tc->next)
	      free_folders_in_context(tc);

	    return(0);


            /*--------- QUIT pine -----------*/
          case OPF3:
	  case 'q':
            if(do_what != FolderMaint)
                goto bleep;

	    for(tc = fs->context_list; tc ; tc = tc->next)
	      free_folders_in_context(tc);

            ps_global->redrawer = (void (*)())NULL;
            ps_global->next_screen = quit_screen;
	    return(0);
	    

            /*--------- Compose -----------*/
          case OPF4:
	  case 'c':
            if(do_what != FolderMaint)
                goto bleep;

	    ps_global->redrawer = (void (*)())NULL;
	    for(tc = fs->context_list; tc ; tc = tc->next)
	      free_folders_in_context(tc);

            ps_global->next_screen = compose_screen;
	    return(0);
	    

            /*--------- Message Index -----------*/
	  case OPF7:
	  case 'i':
	    if(do_what != FolderMaint)
	      goto bleep;

	    ps_global->redrawer = (void (*)())NULL;
	    for(tc = fs->context_list; tc ; tc = tc->next)
	      free_folders_in_context(tc);

	    ps_global->next_screen = mail_index_screen;
            q_status_message(SM_ORDER, 0, 2, "Returning to current index");
	    return(0);


            /*----------------- Add a new folder name -----------*/
	  case PF10:
	  case 'a':
            /*--------------- Rename folder ----------------*/
	  case PF11:
	  case 'r':
	    if(do_what != FolderMaint)
	      goto bleep;

	    if(ch == 'r' || ch == PF11)
	      new_file =rename_folder(quest_line,fs->folder_index,fs->context);
	    else {
                if(fs->context->type & FTYPE_BBOARD)
                  new_file = group_subscription(quest_line, fs->context);
                else
		  new_file = add_new_folder(quest_line, fs->context);
            }

            if(new_file && ALL_FOUND(fs->context)) {
                /* place cursor on new folder! */
		create_folder_display(fs, ps->ttyo->screen_cols);
		fs->prev_index = -1;
		fs->folder_index = folder_index(new_file, fs->context->folders);
            }

	    if(fs->prev_index < 0)
	      mangled_header++;

	    mangled_footer++;
            break;
		     

            /*-------------- Delete --------------------*/
          case PF9:
	  case 'd':
	    if(do_what == Subscribe && ch == PF9){
		if(doing_listmode)
	          goto DoToggle;
		else
		  goto ListModeOn;
	    }

	    if(do_what != FolderMaint)
	      goto bleep;

	    if(!ALL_FOUND(fs->context) || (fs->context->use & CNTXT_PSEUDO)){
		q_status_message1(SM_ORDER | SM_DING, 0, 3,
				  "No folder selected to delete.  %s list.",
				  ALL_FOUND(fs->context) ? "Empty" : "Expand");
		break;
	    }

            if(delete_folder(fs->folder_index, fs->context, &mangled_header)){
		/* remove from file list */
                folder_delete(fs->folder_index, fs->context->folders);

		if(fs->folder_index >= folder_total(fs->context->folders))
		  fs->folder_index = max(0, fs->folder_index - 1);

		create_folder_display(fs, ps->ttyo->screen_cols);
                fs->prev_index = -1; /* Force redraw */
            }

            mangled_footer++;
	    break;


            /*-------------- Toggle subscribe checkbox --------------------*/
	  case 'x':
DoToggle:
	    if(!(do_what == Subscribe && doing_listmode))
	      goto bleep;

	    if(!strncmp(folder_entry(fs->folder_index,
				    fs->context->folders)->prefix, "SUB", 3)){
		q_status_message1(SM_ORDER,0,4,
		    "Already subscribed to \"%s\"",
		    cur_name == NULL ? "" : cur_name);
		break;
	    }
	    else{
		folder_entry(fs->folder_index,fs->context->folders)->prefix[1]
		    = (folder_entry(fs->folder_index,
				   fs->context->folders)->prefix[1] == 'X')
			    ? ' '
			    : 'X';
	    }

	    break;


             /*------------- Print list of folders ---------*/
          case OPF9:
	  case 'y':
	    if(do_what != FolderMaint)
	      goto bleep;

            print_folders(fs);
	    mangled_footer++;
	    break;


            /*---------- Look (Search) ----------*/
	  case OPF8:
	  case PF12:
	  case 'w':
	  case ctrl('W'):
	    if((do_what != FolderMaint && ch == OPF8) 
	       || (do_what == FolderMaint && ch == PF12))
	      goto bleep;
            mangled_footer++;
            rc = search_folders(fs, quest_line);
            if(rc == -1)
	      q_status_message(SM_ORDER, 0, 2, "Folder name search cancelled");
	    else if(rc == 0)
	      q_status_message(SM_ORDER | SM_DING, 0, 2, "Word not found");
	    else if(rc == 2)
              q_status_message(SM_ORDER, 0, 2, "Search wrapped to beginning");

	    break;


            /*---------- Go to Folder, ----------*/
          case OPF6:
          case 'g':
	    if(do_what != FolderMaint)
	      goto bleep;

            new_fold = broach_folder(-FOOTER_ROWS(ps), 0, &(fs->context));
            mangled_footer = 1;;
            if(new_fold && do_broach_folder(new_fold, fs->context) > 0){
		for(tc = fs->context_list; tc ; tc = tc->next)
		  free_folders_in_context(tc);

		ps_global->redrawer = (void (*)())NULL;
		ps_global->next_screen = mail_index_screen;
		return(1); 
	    }

            break;

#ifdef NEWBB
            /*------------ Dismiss the recently created news groups -----*/
          case 'x':
            if(do_what != Subscribe)
              goto bleep;
            clear_new_groups(fs->context->folders);
            redraw_folder_screen();
            q_status_message(SM_ORDER, 0, 2,
         "Mark on newly created groups cleared; groups sorted into full list");
            break;
#endif

            /*----------no op to check for new mail -------*/
          case NO_OP_IDLE:
	  case NO_OP_COMMAND :
	    break;


            /*------------ Redraw command -------------*/
          case KEY_RESIZE:
          case ctrl('L'):
            ClearScreen();
            mangled_footer++;
            mangled_header++;
            redraw_folder_screen();
	    break;


            /*--------------- Invalid Command --------------*/
	  default: 
          bleep:
	    bogus_command(orig_ch, F_ON(F_USE_FK,ps) ? "F1" : "?");
	    break;
	} /* End of switch */
    }
}



/*----------------------------------------------------------------------
     Adjust the folder list's display down one line

Uses the static fs data structure so it can be called almost from
any context. It will recalculate the screen size if need be.
  ----*/
int
folder_scroll_down(count)
    long count;
{
    if(count < 0)
	return(folder_scroll_up(-count));
    else if(count){
	while(count-- && fs->top_row)
	  fs->top_row--;

	if(folder_entry(fs->folder_index, fs->context->folders)->d_line
				>= (unsigned)(fs->top_row + fs->display_rows)){
	    for(count = fs->top_row + fs->display_rows - 1;
		count >= 0 && folder_list_entry(fs, (int)count,
			    &(fs->folder_index), &(fs->context)) && count >= 0;
		count--)
	      ; 

	    fs->prev_index   = fs->folder_index;
	    fs->prev_context = fs->context;
	}

    }

    return(1);
}



/*----------------------------------------------------------------------
     Adjust the folder list's display up one line

Uses the static fs data structure so it can be called almost from
any context. It will recalculate the screen size if need be.
  ----*/
int
folder_scroll_up(count)
    long count;
{
    if(count < 0)
      return(folder_scroll_down(-count));
    else if(count){
	while(count-- && fs->top_row < fs->last_row)
	  fs->top_row++;

	if(folder_entry(fs->folder_index,
			fs->context->folders)->d_line < (unsigned)fs->top_row){
	    for(count = fs->top_row;
		folder_list_entry(fs, (int)count, &(fs->folder_index),
				  &(fs->context))
		&& count <= folder_total(fs->context->folders);
		count++)
	      ;

	    fs->prev_index   = fs->folder_index;
	    fs->prev_context = fs->context;
	}
    }

    return(1);
}



/*----------------------------------------------------------------------
     Adjust the folder list's display so the given line starts the page

Uses the static fs data structure so it can be called almost from
any context. It will recalculate the screen size if need be.
  ----*/
int
folder_scroll_to_pos(line)
    long line;
{
    return(folder_scroll_up(line - fs->top_row));
}



/*----------------------------------------------------------------------
     Redraw the folders screen

Uses the static fs data structure so it can be called almost from
any context. It will recalculate the screen size if need be.
  ----*/
void
redraw_folder_screen()
{
    create_folder_display(fs, ps_global->ttyo->screen_cols);
    display_folder(fs, fs->context, fs->folder_index, NULL, -1);
}

                

/*----------------------------------------------------------------------
   Arrange and paint the lines of the folders directory on the screen

   Args: fd      -- The folder display structure
         lines   -- The number of folder lines to display on the screen
  
 Result: the lines are painted or repainted on the screen

    Paint folder list, or part of it.

Called to either paint or repaint the whole list, or just move the
cursor. If old_row is -1 then we are repainting. In this case have
to watch out for names that are blank as the data sometimes has blanks in
it. Go through the data row by row, column by column. Paint the item that's 
currently "it" in reverse.

When the changing the current one, just repaint the old one normal and the
new one reverse.

  ----*/
void
display_folder(fd, cntxt, index, old_cntxt, old_index)
     FSTATE_S  *fd;
     CONTEXT_S *cntxt, *old_cntxt;
     int        index, old_index;
{
    CONTEXT_S *c;
    char      *s;
    int        i, row;

    if(fd->display_rows <= 0)		/* room for display? */
      return;				/* nope. */

#ifdef _WINDOWS
    mswin_beginupdate();
#endif
    /*
     * Check the framing of the current context/folder...
     */
    if(old_index < 0 || off_folder_display(fd, index, cntxt)){
	/*------------ check framing, then... -----------*/
	while(off_folder_display(fd, index, cntxt) < 0)
	  fd->top_row = max(0, fd->top_row - fd->display_rows);

	while(off_folder_display(fd, index, cntxt) > 0)
	  fd->top_row = min(fd->last_row, fd->top_row + fd->display_rows);

	/*------------ repaint entire screen _-----------*/
	for(row = 0; row < fd->display_rows; row++){
	    ClearLine(HEADER_ROWS(ps_global) + row);

	    if(s = folder_list_entry(fd, row + fd->top_row, &i, &c)){
		/*-------- paint text centered --------*/
		if(*s){
			PutLine0(HEADER_ROWS(ps_global) + row,
				 max(0, (fd->display_cols/2)-(strlen(s)/2)),
				 s);
		}
	    }
	    else{			/* paint folder names on row */
		do
		  paint_folder_name((i == index && c == cntxt), fd, i, c);
		while(++i < folder_total(c->folders)
		      && folder_entry(i,c->folders)->d_line==row+fd->top_row
		      && !(c->use & CNTXT_PSEUDO));
	    }
	}
    }
    else{				/* restore old name to normal */
	if(folder_total(old_cntxt->folders) 
	   && !off_folder_display(fd, old_index, old_cntxt))
	  paint_folder_name(0, fd, old_index, old_cntxt);

	if(folder_total(cntxt->folders))
	  paint_folder_name(1, fd, index, cntxt); /* and hilite the new name */
    }
#ifdef _WINDOWS
    scroll_setrange(fd->last_row);
    scroll_setpos(fd->top_row);
    mswin_endupdate();
#endif
    fflush(stdout);
}



/*
 * paint_folder_name - paint the folder at the given index in the given
 *                     collection.  
 */
void
paint_folder_name(hilite, fd, index, context)
    int        hilite, index;
    FSTATE_S  *fd;
    CONTEXT_S *context;
{
    FOLDER_S *f;
    char     *s;

    if((f = folder_entry(index, context->folders)) == NULL || f->name == NULL)
      return;

    if(context->type&FTYPE_BBOARD || context->use&CNTXT_INCMNG)
      sprintf(s = tmp_20k_buf, "%s%s", f->prefix, FLDR_NAME(f));
    else
      s = FLDR_NAME(f);

    MoveCursor(HEADER_ROWS(ps_global) + (f->d_line - fd->top_row), f->d_col);
    if(hilite)
      StartInverse();

    Write_to_screen(s);
    if(hilite)
      EndInverse();
}


#if (defined(DOS) && !defined(_WINDOWS)) || defined(OS2)
#define LINECH	'\xC4'
#else
#define LINECH	'-'
#endif

/*
 * folder_list_entry - return either the string associated with the
 *                     current folder list line, or the first
 *                     folder index and context associated with it.
 *
 * NOTE: this is kind of dumb right now since it starts from the top 
 *       each time it's called.  Caching the last values would make it
 *       alot less costly.
 */
char *
folder_list_entry(fd, goal_row, index, context)
    FSTATE_S   *fd;
    int         goal_row;
    int        *index;
    CONTEXT_S **context;
{
    char     *s;
    int       row, label, ftotal;

    if(goal_row > fd->last_row) 	/* return blanks past last entry */
      return("");

    goal_row = max(0, goal_row);
    row      = 0;
    *index   = 0;
    for(*context = fs->context_list;
	(*context)->next && (*context)->next->d_line < goal_row;
	*context = (*context)->next)
      row = (*context)->next->d_line;

    s        = NULL;
    label    = (ps_global->context_list->next) ? 1 : 0;

    while(1){
	if(label > 0){
	    if(row < (*context)->d_line){
		memset((void *)tmp_20k_buf,LINECH,fd->display_cols*sizeof(char));
		tmp_20k_buf[fd->display_cols] = '\0';
		s = (row + 1 == (*context)->d_line) ? tmp_20k_buf : "" ;
	    }
	    else if(label++ < 2){
		s = tmp_20k_buf;
		if((*context)->use & CNTXT_INCMNG){
		    sprintf(tmp_20k_buf, "%s", (*context)->label[0]);
		}
		else{
		    memset((void *)tmp_20k_buf, ' ', 
			   fd->display_cols * sizeof(char));
		    tmp_20k_buf[fd->display_cols] = '\0';

		    sprintf(tmp_20k_buf + fd->display_cols + 2, 
			    "%s-collection <%s>  %s", 
			    ((*context)->type & FTYPE_BBOARD) ? "News"
			    				      : "Folder",
			    (*context)->label[0], 
			    ((*context)->use & CNTXT_SAVEDFLT)
			        ? "** Default for Saves **" : "");
		    strncpy(tmp_20k_buf, 
			    tmp_20k_buf + fd->display_cols + 2, 
			    strlen(tmp_20k_buf + fd->display_cols + 2));
		    strncpy(tmp_20k_buf + fd->display_cols 
			    - (((*context)->type & FTYPE_REMOTE) ? 8 : 7),
			    ((*context)->type & FTYPE_REMOTE)? "(Remote)"
			                                     : "(Local)",
			    ((*context)->type & FTYPE_REMOTE) ? 8 : 7);
		}
	    }
	    else{
		label = -1;
		continue;
	    }
	}
	else if((ftotal = folder_total((*context)->folders))
		&& row < (int)folder_entry(0, (*context)->folders)->d_line){
	    if(label < 0){
		memset((void *)tmp_20k_buf,LINECH,fd->display_cols*sizeof(char));
		tmp_20k_buf[fd->display_cols] = '\0';
		s = tmp_20k_buf;
		label = 0;
	    }
	    else
	      s = "";
	}
	else{
	    s          = NULL;
	    if(*index >= ftotal		/* maybe continue where we left off? */
	       || (int)folder_entry(*index,
				    (*context)->folders)->d_line > goal_row)
	      *index = 0;

	    for(; *index < ftotal; (*index)++)
	      if(goal_row <= (int)folder_entry(*index,
					       (*context)->folders)->d_line)
		break;			/* bingo! */

	    if(*index >= ftotal){
		*index = 0;
		if((*context)->next){
		    *context = (*context)->next;
		    label      = 1;
		    continue;		/* go take care of labels */
		}
		else{
		    s = "";			/* dropped off end of list */
		    break;
		}
	    }
	}

	if(row == goal_row)
	  break;
	else
	  row++;
    }

    *index = max(0, *index);
    return(s);
}



/*
 * off_folder_display - returns: 0 if given folder is on display,
 *                               1 if given folder below display, and
 *                              -1 if given folder above display
 */
int
off_folder_display(fd, index, context)
    FSTATE_S  *fd;
    int        index;
    CONTEXT_S *context;
{
    int l;

    if(index >= folder_total(context->folders))
      return(0);			/* no folder to display */

    if((l = folder_entry(index, context->folders)->d_line) < fd->top_row)
      return(-1);
    else if(l >= (fd->top_row + fd->display_rows))
      return(1);
    else
      return(0);
}



/*----------------------------------------------------------------------
      Create a new folder

   Args: quest_line  -- Screen line to prompt on
         folder_list -- The current list of folders

 Result: returns the name of the folder created

  ----*/

char *
add_new_folder(quest_line, cntxt)
     int        quest_line;
     CONTEXT_S *cntxt;
{
    static char  add_folder[MAXFOLDER+1];	/* needed after return!! */
    char	 tmp[MAXFOLDER+1], nickname[32], c, *return_val = NULL;
    HelpType     help;
    int          rc, offset, exists, cnt = 0;
    MAILSTREAM  *create_stream;
    FOLDER_S    *f;

    dprint(4, (debugfile, "\n - add_new_folder - \n"));
    
    add_folder[0] = '\0';
    nickname[0]   = '\0';
    if(cntxt->use & CNTXT_INCMNG){
	char inbox_host[MAXPATH], *beg, *end = NULL;
	ESCKEY_S *special_key;
	static ESCKEY_S host_key[] = {{ctrl('X'),12,"^X","Use Inbox Host"},
				      {-1, 0, NULL, NULL}};

	if(ps_global->readonly_pinerc){
	    q_status_message(SM_ORDER,3,5,
		"Addition cancelled: config file not editable");
	    return(NULL);
	}

	/*
	 * Prompt for the full pathname (with possible "news" subcommand),
	 * then fall thru to prompt for foldername, then prompt for 
	 * nick name.
	 * NOTE : Don't put it in a "[<default]" prompt since we 
	 * need a way to chose a local folder!
	 */
	inbox_host[0] = '\0';
	if((beg = ps_global->VAR_INBOX_PATH)
	   && (*beg == '{' || (*beg == '*' && *++beg == '{'))
	   && (end = strindex(ps_global->VAR_INBOX_PATH, '}'))){
	    strncpy(inbox_host, beg+1, end - beg);
	    inbox_host[end - beg - 1] = '\0';
	    special_key = host_key;
	}
	else
	  special_key = NULL;

	sprintf(tmp, "Name of server to contain added folder : ");
	help = NO_HELP;
	while(1){
	    rc = optionally_enter(add_folder, quest_line, 0, MAXFOLDER, 1,
					      0, tmp, special_key, help, 0);
	    removing_trailing_white_space(add_folder);
	    removing_leading_white_space(add_folder);
	    if(rc == 3){
		help = help == NO_HELP ? h_incoming_add_folder_host : NO_HELP;
	    }
	    else if(rc == 12){
		strcpy(add_folder, inbox_host);
		break;
	    }
	    else if(rc == 1){
		q_status_message(SM_ORDER,0,2,
		    "Addition of new folder cancelled");
		return(NULL);
	    }
	    else if(rc == 0)
	      break;
	}
    }

    if(offset = strlen(add_folder)){		/* must be host for incoming */
	int i;
	sprintf(tmp, "Folder on \"%s\" to add : ", add_folder);
	for(i = offset;i >= 0; i--)
	  add_folder[i+1] = add_folder[i];

	add_folder[0] = '{';
	add_folder[++offset] = '}';
	add_folder[++offset] = '\0';		/* +2, total */
    }
    else
      sprintf(tmp, "Name of folder to add : ");

    help = NO_HELP;
    while(1){
        rc = optionally_enter(&add_folder[offset], quest_line, 0, 
			      MAXFOLDER - offset, 1, 0, tmp, NULL, help, 0);
	removing_trailing_white_space(&add_folder[offset]);
	removing_leading_white_space(&add_folder[offset]);
        if(rc == 0 && add_folder[offset]){
	    if(!ps_global->show_dot_names && add_folder[offset] == '.'){
		if(cnt++ <= 0)
                  q_status_message(SM_ORDER,3,3,
		    "Folder name can't begin with dot");
		else{
		    NAMEVAL_S *feat;
		    int i;

		    for(i=0; (feat=feature_list(i))
				&& (feat->value != F_ENABLE_DOT_FOLDERS); i++)
		      ;/* do nothing */

		    q_status_message1(SM_ORDER,3,3,
		      "Config feature \"%s\" enables names beginning with dot",
		      feat && feat->name ? feat->name : "");
		}

                display_message(NO_OP_COMMAND);
                continue;
	    }

	    if(strucmp(ps_global->inbox_name, nickname))
	      break;
	    else
	      Writechar(BELL, 0);
	}

        if(rc == 3){
	    help = (help == NO_HELP)
			? ((cntxt->use & CNTXT_INCMNG)
			    ? h_incoming_add_folder_name
			    : h_oe_foldadd)
			: NO_HELP;
	}
	else if(rc == 1 || add_folder[0] == '\0') {
	    q_status_message(SM_ORDER,0,2, "Addition of new folder cancelled");
	    return(NULL);
	}
    }

    if(*add_folder == '{'			/* remote? */
       || (*add_folder == '*' && *(add_folder+1) == '{')){
	create_stream = context_same_stream(cntxt->context, add_folder,
					    ps_global->mail_stream);
	    if(!create_stream 
	       && ps_global->mail_stream != ps_global->inbox_stream)
	      create_stream = context_same_stream(cntxt->context, add_folder,
						  ps_global->inbox_stream);
    }
    else
      create_stream = NULL;

    help = NO_HELP;
    if(cntxt->use & CNTXT_INCMNG){
	sprintf(tmp, "Nickname for folder \"%s\" : ", &add_folder[offset]);
	while(1){
	    rc = optionally_enter(nickname, quest_line, 0, 31, 1, 0, tmp,
				  NULL, help, 0);
	    removing_leading_white_space(nickname);
	    removing_trailing_white_space(nickname);
	    if(rc == 0){
		if(strucmp(ps_global->inbox_name, nickname))
		  break;
		else
		  Writechar(BELL, 0);
	    }

	    if(rc == 3){
		help = help == NO_HELP
			? h_incoming_add_folder_nickname : NO_HELP;
	    }
	    else if(rc == 1 || (rc != 3 && !*nickname)){
		q_status_message(SM_ORDER,0,2,
		    "Addition of new folder cancelled");
		return(NULL);
	    }
	}

	/*
	 * Already exist?  First, make sure this name won't collide with
	 * anything else in the list.  Next, quickly test to see if it
	 * the actual mailbox exists so we know any errors from 
	 * context_create() are really bad...
	 */
	for(offset = 0; offset < folder_total(cntxt->folders); offset++){
	    f = folder_entry(offset, cntxt->folders);
	    if(!strucmp(FLDR_NAME(f), nickname[0] ? nickname : add_folder)){
		q_status_message1(SM_ORDER | SM_DING, 0, 3,
				  "Incoming folder \"%s\" already exists",
				  nickname[0] ? nickname : add_folder);
		return(NULL);
	    }
	}

	exists = folder_exists(cntxt->context, add_folder);
    }
    else
      exists = 0;

    if(exists < 0
       || (!exists && !context_create(cntxt->context,create_stream,add_folder)
	   && !((cntxt->use & CNTXT_INCMNG) && !context_isambig(add_folder))))
      return(NULL);		/* c-client should've reported error */

    if(cntxt->use & CNTXT_INCMNG){
	f = new_folder(add_folder);
	if(nickname[0]){
	    f->nickname = cpystr(nickname);
	    f->name_len = strlen(f->nickname);
	}

	folder_insert(folder_total(cntxt->folders), f, cntxt->folders);
	if(!ps_global->USR_INCOMING_FOLDERS){
	    offset = 0;
	    ps_global->USR_INCOMING_FOLDERS =
					     (char **)fs_get(2*sizeof(char *));
	}
	else{
	    for(offset=0;  ps_global->USR_INCOMING_FOLDERS[offset]; offset++)
	      ;

	    fs_resize((void **)&(ps_global->USR_INCOMING_FOLDERS),
		      (offset + 2) * sizeof(char *));
	}

	sprintf(tmp, "%s%s%s%s%s", nickname[0] ? "\"" : "",
		nickname[0] ? nickname : "", nickname[0] ? "\"" : "",
		nickname[0] ? " " : "", add_folder);
	ps_global->USR_INCOMING_FOLDERS[offset]   = cpystr(tmp);
	ps_global->USR_INCOMING_FOLDERS[offset+1] = NULL;
	write_pinerc(ps_global);		/* save new element */

	if(nickname[0])
	  strcpy(add_folder, nickname);		/* known by new name */

	q_status_message1(SM_ORDER, 0, 3, "Folder \"%s\" created",add_folder);
	return_val = add_folder;
    }
    else if(context_isambig(add_folder)){
	if(ALL_FOUND(cntxt)){
	    if(cntxt->use & CNTXT_PSEUDO){
		folder_delete(0, cntxt->folders);
		cntxt->use &= ~CNTXT_PSEUDO;
	    }

	    folder_insert(-1, new_folder(add_folder), cntxt->folders);
	    q_status_message1(SM_ORDER,0,3, "Folder \"%s\" created",add_folder);
	}

	return_val = add_folder;
    }
    else
      q_status_message1(SM_ORDER, 0, 3,
			"Folder \"%s\" created outside current collection",
			add_folder);

    return(return_val);
}

/*---
  subscribe context referenced here to mark appropriate entries as new
  newbb_context referenced in imap.c to know to call mark_folder_as_news
  ---*/
static	CONTEXT_S	subscribe_cntxt;
#ifdef NEWBB
static	CONTEXT_S	newbb_cntxt;
#endif

#ifdef NEWBB
/*----------------------------------------------------------------------
      Mark the named folder as "NEW" in subscribed_cntxt

Args: groups -- the name of the news group that is new

This is going to be inefficient on a DOS machine where folder_entry() is
expensive. 
 ----*/
void
mark_folder_as_new(group)
     char *group;
{
    int i;

    for(i = 0 ; i < folder_total(subscribe_cntxt.folders); i++) {
        if(strucmp(folder_entry(i, subscribe_cntxt.folders)->name,
                   group) == 0) {
            strcpy(folder_entry(i, subscribe_cntxt.folders)->prefix, "NEW ");
            break;
        }
    }
}
#endif    


/*----------------------------------------------------------------------
    Subscribe to a news group

   Args: quest_line  -- Screen line to prompt on
         cntxt       -- The context the subscription is for

 Result: returns the name of the folder subscribed too


This builds a complete context for the entire list of possible news groups. 
It also build a context to find the newly created news groups as 
determined by data kept in .pinerc.  When the find of these new groups is
done the subscribed context is searched and the items marked as new. 
A list of new board is never actually created.

  ----*/
char *
group_subscription(quest_line, cntxt)
     int        quest_line;
     CONTEXT_S *cntxt;
{
    char	   *add_folder, *full_name;
    char	  **folders;
    int		    rc, i, last_find_partial = 0, we_cancel = 0;
    long	    xsum;
    FOLDER_S	   *new_f;
    MAILSTREAM	   *create_stream;
    FSTATE_S	    sub_state, *push_state;
    HelpType        help;
    static char	    folder[MAXFOLDER+2];
    static ESCKEY_S subscribe_keys[] = {{ctrl('T'), 12, "^T", "To All Grps"},
					{-1, 0, NULL, NULL}};
    extern long	    line_hash();

    /*---- Build a context to find all news groups -----*/
    subscribe_cntxt         = *cntxt;
    subscribe_cntxt.use    |= CNTXT_FINDALL | CNTXT_NOFIND;
    subscribe_cntxt.use    &= ~CNTXT_PSEUDO;
    subscribe_cntxt.next    = NULL;
    subscribe_cntxt.folders = new_folder_list();

    /*
     * Prompt for group name.
     */
    add_folder  = &folder[1];			/* save position 0 */
    *add_folder = '\0';
    help = NO_HELP;
    while(1){
	xsum = line_hash(add_folder);
        rc = optionally_enter(add_folder, quest_line, 0, MAXFOLDER, 1, 0,
			      SUBSCRIBE_PMT, subscribe_keys, help, 0);
	removing_trailing_white_space(add_folder);
	removing_leading_white_space(add_folder);
        if((rc == 0 && *add_folder) || rc == 12){
	    we_cancel = busy_alarm(1, "Fetching newsgroup list", NULL, 0);

	    if(last_find_partial){
		/* clean up any previous find results */
		free_folders_in_context(&subscribe_cntxt);
		last_find_partial = 0;
	    }

	    if(rc == 12){			/* list the whole enchilada */
		find_folders_in_context(NULL, &subscribe_cntxt, NULL);
	    }
	    else if(i = strlen(add_folder)){
		/* clean up after any previous find */
		folder[0]	= '*';		/* insert preceding '*' */
		add_folder[i]   = '*';		/* and append '*' */
		add_folder[i+1] = '\0';
		find_folders_in_context(NULL, &subscribe_cntxt, folder);
		add_folder[i] = '\0';
	    }
	    else{
		q_status_message(SM_ORDER, 0, 2,
	       "No group substring to match! Use ^T to list all news groups.");
		continue;
	    }

	    /*
	     * If we did a partial find on matches, then we faked a full
	     * find which will cause this to just return.
	     */
	    if(i = folder_total(subscribe_cntxt.folders)){
		char *f;

		/*
		 * fake that we've found everything there is to find...
		 */
		subscribe_cntxt.use &= ~(CNTXT_NOFIND|CNTXT_PARTFIND);
		last_find_partial = 1;

		if(i == 1){
		    f = folder_entry(0, subscribe_cntxt.folders)->name;
		    if(!strcmp(f, add_folder)){
			rc = 1;			/* success! */
			break;
		    }
		    else{			/* else complete the group */
			strcpy(add_folder, f);
			continue;
		    }
		}
		else if(xsum == line_hash(add_folder)){
		    /*
		     * See if there wasn't an exact match in the lot.
		     */
		    while(i-- > 0){
			f = folder_entry(i,subscribe_cntxt.folders)->name;
			if(!strcmp(f, add_folder))
			  break;
			else
			  f = NULL;
		    }

		    /* if so, then the user picked it from the list the
		     * last time and didn't change it at the prompt.
		     * Must mean they're accepting it...
		     */
		    if(f){
			rc = 1;			/* success! */
			break;
		    }
		}
	    }
	    else{
		if(rc == 12)
		  q_status_message(SM_ORDER | SM_DING, 3, 3,
				   "No groups to select from!");
		else
		  q_status_message1(SM_ORDER, 3, 3,
			  "News group \"%s\" didn't match any existing groups",
			  add_folder);

		continue;
	    }

#ifdef NEWBB
	    /*----- build a context to find new news groups -------*/
	    newbb_cntxt = subscribe_cntxt;
	    newbb_cntxt.use |= CNTXT_NOFIND | CNTXT_NEWBB;
	    newbb_cntxt.use &= ~CNTXT_PSEUDO;
	    newbb_cntxt.next = NULL;
	    newbb_cntxt.folders = new_folder_list(); /* Not realy used */
	    newbb_folder_list = newbb_cntxt.folders;
	    find_folders_in_context(NULL, &newbb_cntxt, NULL);
#endif
	    /*----- Mark groups that are currently subscribed too ------*/
	    /* but first make sure they're found */
	    find_folders_in_context(NULL, cntxt, NULL);
	    for(i = 0 ; i < folder_total(subscribe_cntxt.folders); i++) {
		dprint(9, (debugfile, "PREFIX: \"%s\", %s\n",
			   folder_entry(i,subscribe_cntxt.folders)->prefix,
			   folder_entry(i,subscribe_cntxt.folders)->name));
		if(search_folder_list(cntxt->folders,
			       folder_entry(i,subscribe_cntxt.folders)->name))
		  strcpy(folder_entry(i, subscribe_cntxt.folders)->prefix,
			 "SUB ");
		else
		  if(strlen(folder_entry(i,
					 subscribe_cntxt.folders)->prefix)!= 4)
		    strcpy(folder_entry(i,subscribe_cntxt.folders)->prefix,
			   "    ");
	    }
#ifdef NEWBB
	    /*-- Get the newly created groups to the top of the list --*/
	    sort_folder_list(subscribe_cntxt.folders, compare_folders_new);
#endif

	    if(we_cancel)
	      cancel_busy_alarm(-1);

	    /*----- Call the folder lister to do all the work -----*/
	    push_state = fs;
	    folders = NULL;
	    rc = folder_lister(ps_global, Subscribe, &subscribe_cntxt,
			       NULL, add_folder, &folders, &subscribe_cntxt,
			       &sub_state);
	    fs = push_state;
	    redraw_folder_screen();

	    if(rc <= 0){
		rc = -1;
		break;
	    }
	    else if(rc == 17 || F_ON(F_SELECT_WO_CONFIRM,ps_global))
	      /*
	       * The 17 comes from folder_lister, which returns 17 if it
	       * passes back multiple groups in the folders array.
	       */
	      break;

	}
        else if(rc == 3){
            help = help == NO_HELP ? h_news_subscribe : NO_HELP;
	}
	else if(rc == 1 || add_folder[0] == '\0'){
	    rc = -1;
	    break;
	}
    }

    free_folder_list(&subscribe_cntxt.folders);

    if(rc < 0){
	if(rc == -1)
	  q_status_message(SM_ORDER, 0, 3, "Subscribe cancelled");

	return(NULL);
    }

    /*------ Actually do the subscription -----*/
    if(rc == 17){
	int i, n = 0, errors = 0;

	/* subscribe one at a time */
	for(i=0; folders[i]; i++){
	    context_apply(tmp_20k_buf, subscribe_cntxt.context, folders[i]);
	    full_name = cpystr(tmp_20k_buf+1);
	    rc = (int)mail_subscribe_bboard(NULL, full_name);
	    fs_give((void **)&full_name);
	    if(rc == 0){
		/*
		 * This message may not make it to the screen, because
		 * a c-client message about the failure will be there.
		 * Probably best not to string together a whole bunch of errors
		 * if there is something wrong.
		 */
		q_status_message1(errors?SM_INFO:SM_ORDER, errors ? 0 : 3, 3,
				  "Error subscribing to \"%s\"", folders[i]);
		errors++;
	    }
	    else{
		/*
		 * Save for updating cursor on display.  Arbitrarily choose
		 * the first one in the list for the cursor location.
		 */
		if(n == 0)
		  strcpy(add_folder, folders[i]);

		n++;
		/*---- Update the screen display data structures -----*/
		if(ALL_FOUND(cntxt)){  /* Not sure what this does... */
		    if(cntxt->use & CNTXT_PSEUDO){
			folder_delete(0, cntxt->folders);
			cntxt->use &= ~CNTXT_PSEUDO;
		    }

		    folder_insert(-1, new_folder(folders[i]), cntxt->folders);
		}
	    }
	}

	if(n == 0){
	    q_status_message(SM_ORDER | SM_DING, 3, 5,
		"Subscriptions failed, subscribed to no new groups");
	    add_folder = NULL;
	}
	else
	  q_status_message3(SM_ORDER | (errors ? SM_DING : 0), errors ? 3 : 0,3,
	      "Subscribed to %s new groups%s%s",
	      comatose((long)n),
	      errors ? ", failed on " : "",
	      errors ? comatose((long)errors) : "");

	for(i=0; folders[i]; i++)
	  fs_give((void **)&folders[i]);

	fs_give((void **)&folders);
    }
    else{
	context_apply(tmp_20k_buf, subscribe_cntxt.context, add_folder);
	full_name = cpystr(tmp_20k_buf+1);
	rc = (int)mail_subscribe_bboard(NULL, full_name);
	fs_give((void **)&full_name);
	if(rc == 0){
	    q_status_message1(SM_ORDER | SM_DING, 3, 3,
			      "Error subscribing to \"%s\"", add_folder);
	    return(NULL);
	}

	/*---- Update the screen display data structures -----*/
	if(ALL_FOUND(cntxt)){  /* Not sure what this does... */
	    if(cntxt->use & CNTXT_PSEUDO){
		folder_delete(0, cntxt->folders);
		cntxt->use &= ~CNTXT_PSEUDO;
	    }

	    folder_insert(-1, new_folder(add_folder), cntxt->folders);
	}

	q_status_message1(SM_ORDER, 0, 3, "Subscribed to \"%s\"", add_folder);
    }

    return(add_folder);
}



/*----------------------------------------------------------------------
      Rename folder
  
   Args: q_line     -- Screen line to prompt on
         index      -- index of folder in folder list to rename
         cntxt      -- collection of folders making up folder list

 Result: returns the new name of the folder, or NULL if nothing happened.

 When either the sent-mail or saved-message folders are renamed, immediately 
create a new one in their place so they always exist. The main loop above also
detects this and makes the rename look like an add of the sent-mail or
saved-messages folder. (This behavior may not be optimal, but it keeps things
consistent.

  ----*/
char *
rename_folder(q_line, index, cntxt)
     int        q_line, index;
     CONTEXT_S *cntxt;
{
    static char  new_foldername[MAXFOLDER+1];
    char        *folder, *prompt;
    HelpType     help;
    int          rc, ren_cur, cnt = 0;
    FOLDER_S	*new_f;
    MAILSTREAM  *ren_stream = NULL;

    dprint(4, (debugfile, "\n - rename folder -\n"));

    if(cntxt->type & FTYPE_BBOARD){
	q_status_message(SM_ORDER | SM_DING, 3, 3,
			 "Can't rename bulletin boards or news groups!");
	return(NULL);
    }
    else if(!ALL_FOUND(cntxt) || (cntxt->use & CNTXT_PSEUDO)){
	q_status_message1(SM_ORDER | SM_DING, 0, 3,
			  "No folder selected to rename.  %s list.",
			  ALL_FOUND(cntxt) ? "Empty" : "Expand");
	return(NULL);
    }
    else if((new_f = folder_entry(index, cntxt->folders))
	    && strucmp(FLDR_NAME(new_f), ps_global->inbox_name) == 0) {
        q_status_message1(SM_ORDER | SM_DING, 3, 4,
			  "Can't change special folder name \"%s\"",
			  ps_global->inbox_name);
        return(NULL);
    }
    else if(new_f->nickname){ 
	q_status_message(SM_ORDER | SM_DING, 3, 3,
			 "Can't rename folder nicknames at this time!");
	return(NULL);
    }

    folder  = new_f->name;
    ren_cur = strcmp(folder, ps_global->cur_folder) == 0;

    prompt = "Rename folder to : ";
    help   = NO_HELP;
    strcpy(new_foldername, folder);
    while(1) {
        rc = optionally_enter(new_foldername, q_line, 0, MAXFOLDER, 1, 0,
                              prompt, NULL, help, 0);
        if(rc == 3) {
            help = help == NO_HELP ? h_oe_foldrename : NO_HELP;
            continue;
        }

	removing_trailing_white_space(new_foldername);
	removing_leading_white_space(new_foldername);

        if(rc == 0 && *new_foldername) {
	    /* verify characters */
	    if(!ps_global->show_dot_names && *new_foldername == '.'){
		if(cnt++ <= 0)
                  q_status_message(SM_ORDER,3,3,
		    "Folder name can't begin with dot");
		else{
		    NAMEVAL_S *feat;
		    int i;

		    for(i=0; (feat=feature_list(i))
				&& (feat->value != F_ENABLE_DOT_FOLDERS); i++)
		      ;/* do nothing */

		    q_status_message1(SM_ORDER,3,3,
		      "Config feature \"%s\" enables names beginning with dot",
		      feat && feat->name ? feat->name : "");
		}

                display_message(NO_OP_COMMAND);
                continue;
	    }

	    if(folder_index(new_foldername, cntxt->folders) >= 0){
                q_status_message1(SM_ORDER,3,3, "Folder \"%s\" already exists",
                                  pretty_fn(new_foldername));
                display_message(NO_OP_COMMAND);
                continue;
            }
        }

        if(rc != 4) /* redraw */
          break;  /* no redraw */

    }

    if(rc==1 || new_foldername[0]=='\0' || strcmp(new_foldername, folder)==0){
        q_status_message(SM_ORDER, 0, 2, "Folder rename cancelled");
        return(0);
    }

    if(ren_cur && ps_global->mail_stream != NULL) {
        mail_close(ps_global->mail_stream);
        ps_global->mail_stream = NULL;
    }

    ren_stream = context_same_stream(cntxt->context, new_foldername,
				     ps_global->mail_stream);

    if(!ren_stream && ps_global->mail_stream != ps_global->inbox_stream)
      ren_stream = context_same_stream(cntxt->context, new_foldername,
				       ps_global->inbox_stream);
      
    if(rc = context_rename(cntxt->context,ren_stream,folder,new_foldername)){
	/* insert new name */
	new_f               = new_folder(new_foldername);
	new_f->prefix[0]    = '\0';
	new_f->msg_count    = 0;
	new_f->unread_count = 0;
	folder_insert(-1, new_f, cntxt->folders);

	if(strcmp(ps_global->VAR_DEFAULT_FCC, folder) == 0
	   || strcmp(ps_global->VAR_DEFAULT_SAVE_FOLDER, folder) == 0) {
	    /* renaming sent-mail or saved-messages */
	    if(context_create(cntxt->context, NULL, folder)){
		q_status_message3(SM_ORDER,0,3,
		     "Folder \"%s\" renamed to \"%s\". New \"%s\" created",
				  folder, new_foldername,
				  pretty_fn(
				    (strcmp(ps_global->VAR_DEFAULT_SAVE_FOLDER,
					    folder) == 0)
				    ? ps_global->VAR_DEFAULT_SAVE_FOLDER
				    : ps_global->VAR_DEFAULT_FCC));

	    }
	    else{
		if((index = folder_index(folder, cntxt->folders)) >= 0)
		  folder_delete(index, cntxt->folders); /* delete old struct */

		q_status_message1(SM_ORDER | SM_DING, 3, 4,
				  "Error creating new \"%s\"", folder);

		dprint(2, (debugfile, "Error creating \"%s\" in %s context\n",
			   folder, cntxt->context));
	    }
	}
	else{
	    q_status_message2(SM_ORDER, 0, 3, "Folder \"%s\" renamed to \"%s\"",
			      pretty_fn(folder), pretty_fn(new_foldername));

	    if((index = folder_index(folder, cntxt->folders)) >= 0)
	      folder_delete(index, cntxt->folders); /* delete old struct */
	}
    }

    if(ren_cur) {
        /* No reopen the folder we just had open */
        do_broach_folder(new_foldername, cntxt);
    }

    return(rc ? new_foldername : NULL);
}



/*----------------------------------------------------------------------
   Confirm and delete a folder

   Args: index -- Index of folder in collection to remove
         cntxt -- The particular collection the folder's to be remove from
         mangled_header -- Pointer to flag to set if the the anchor line
                           needs updating (deleted the open folder)

 Result: return 0 if not delete, 1 if deleted.

 NOTE: Currently disallows deleting open folder...
  ----*/
int
delete_folder(index, cntxt, mangled_header)
    int        index, *mangled_header;
    CONTEXT_S *cntxt;
{
    char       *folder, *full_folder, ques_buf[MAX_SCREEN_COLS+1];
    MAILSTREAM *del_stream = NULL;
    FOLDER_S   *fp;
    int         ret, close_opened = 0;

    if(cntxt->type & FTYPE_BBOARD){
	static char fmt[] = "Really unsubscribe from \"%.*s\"";
         
        folder = folder_entry(index, cntxt->folders)->name;
	/* 4 is strlen("%.*s") */
        sprintf(ques_buf, fmt, sizeof(ques_buf) - (sizeof(fmt)-4), folder);
    
        ret = want_to(ques_buf, 'n', 'x', NO_HELP, 0, 0);
        switch(ret) {
          /* ^C */
          case 'x':
            Writechar(BELL, 0);
            /* fall through */
          case 'n':
            return(0);
        }
    
        dprint(2, (debugfile, "deleting folder \"%s\" in context \"%s\"\n",
	       folder, cntxt->context));

        context_apply(tmp_20k_buf, cntxt->context, folder);
	full_folder = cpystr(tmp_20k_buf + 1);
	ret = (int)mail_unsubscribe_bboard(NULL, full_folder);
	fs_give((void **)&full_folder);
        if(ret == 0){
            q_status_message1(SM_ORDER | SM_DING, 3, 3,
			      "Error unsubscribing from \"%s\"", folder);
            return(0);
        }

	return(1);
    }

    if(!folder_total(cntxt->folders)){
	q_status_message(SM_ORDER | SM_DING, 0, 4,
			 "Empty folder collection.  No folder to delete!");
	return(0);
    }

    if(cntxt->use & CNTXT_INCMNG){
	if(ps_global->readonly_pinerc){
	    q_status_message(SM_ORDER,3,5,
		"Deletion cancelled: config file not editable");
	    return(0);
	}
    }

    fp     = folder_entry(index, cntxt->folders);
    folder = FLDR_NAME(fp);
    dprint(4, (debugfile, "=== delete_folder(%s) ===\n", folder));

    if(strucmp(folder, ps_global->inbox_name) == 0) {
	q_status_message1(SM_ORDER | SM_DING, 3, 4,
		 "Can't delete special folder \"%s\".", ps_global->inbox_name);
	return(0);
    }
    else if(cntxt == ps_global->context_current
	    && strcmp(folder, ps_global->cur_folder) == 0)
      close_opened++;

    sprintf(ques_buf, "Really delete \"%s\"%s", folder, 
	    close_opened ? " (the currently open folder)" : "");

    if((ret=want_to(ques_buf, 'n', 'x', NO_HELP, 0, 0)) != 'y'){
	q_status_message(SM_ORDER,0,3, (ret == 'x') ? "Delete cancelled" 
			 		     : "No folder deleted");
	return(0);
    }

    dprint(2, (debugfile, "deleting folder \"%s\" (%s) in context \"%s\"\n",
	       fp->name, fp->nickname ? fp->nickname : "", cntxt->context));

    /*
     * Use fp->name since "folder" may be a nickname...
     */
    if(close_opened){
	/*
	 * There *better* be a stream, but check just in case.  Then
	 * close it, NULL the pointer, and let do_broach_folder fixup
	 * the rest...
	 */
	if(ps_global->mail_stream){
	    mail_close(ps_global->mail_stream);
	    ps_global->mail_stream = NULL;
	    *mangled_header = 1;
	    do_broach_folder(ps_global->inbox_name, ps_global->context_list);
	}
    }
    else
      del_stream = context_same_stream(cntxt->context, fp->name,
				       ps_global->mail_stream);

    if(!del_stream && ps_global->mail_stream != ps_global->inbox_stream)
      del_stream = context_same_stream(cntxt->context, fp->name,
				       ps_global->inbox_stream);

    if(!((cntxt->use & CNTXT_INCMNG)
	 && !folder_exists(cntxt->context, fp->name))
       && !context_delete(cntxt->context, del_stream, fp->name)){
/*
 * BUG: what if sent-mail or saved-messages????
 */
	q_status_message1(SM_ORDER,3,3,"Delete of \"%s\" Failed!", folder);
	return(0);
    }

    if(cntxt->use & CNTXT_INCMNG){
	char *p;
	int   i;
	for(i = 0; ps_global->USR_INCOMING_FOLDERS[i]; i++){
	    /*
	     * Quite a test, non?
	     */
	    if((p = srchstr(ps_global->USR_INCOMING_FOLDERS[i], fp->name))
	       && p[strlen(fp->name)] == '\0'
	       && (p == ps_global->USR_INCOMING_FOLDERS[i]
		   || isspace((unsigned char)*(p-1)))){
		fs_give((void **)&(ps_global->USR_INCOMING_FOLDERS[i]));
		while(ps_global->USR_INCOMING_FOLDERS[i]
		      = ps_global->USR_INCOMING_FOLDERS[i+1])
		  i++;				/* rub it out */
	    }
	}

	write_pinerc(ps_global);		/* save new element */
    }

    q_status_message1(SM_ORDER, 0,3,"Folder \"%s\" deleted!", folder);
    return(1);
}



/*----------------------------------------------------------------------
      Print the list of folders on paper

   Args: list    --  The current list of folders
         lens    --  The list of lengths of the current folders
         display --  The current folder display structure

 Result: list printed on paper

If the display list was created for 80 columns it is used, otherwise
a new list is created for 80 columns

  ----*/

void
print_folders(display)
    FSTATE_S  *display;
{
    int i, index, l;
    CONTEXT_S *context;
    FOLDER_S  *f;
    char       buf[256];

    if(ps_global->ttyo->screen_cols != 80)
      create_folder_display(display, 80);

    if(open_printer("folder list ") != 0)
      return;

    context = display->context_list;
    index   = i = 0;
    while(context){
	for(;i < context->d_line;i++) /* leading spaces */
	  print_text(NEWLINE);

	memset((void *)tmp_20k_buf,LINECH, 80 * sizeof(char));
	tmp_20k_buf[80] = '\0';
	print_text(tmp_20k_buf);
	print_text(NEWLINE);
	i++;

	memset((void *)tmp_20k_buf,' ', 80 * sizeof(char));
	tmp_20k_buf[80] = '\0';
	if(context->use & CNTXT_INCMNG){
	    i = strlen(context->label[0]);
	    strncpy(tmp_20k_buf + max(40 - (i/2), 0), context->label[0], i);
	}
	else{
	    sprintf(tmp_20k_buf + 80 + 2, 
		    " %s-collection <%s>  %s", 
		    (context->type & FTYPE_BBOARD) ? "News" : "Mail",
		    context->label[0], 
		    (context->use & CNTXT_SAVEDFLT)
		                   ? "** Default for Saves **" : "");
	    strncpy(tmp_20k_buf, 
		    tmp_20k_buf + 80 + 2, 
		    strlen(tmp_20k_buf + 80 + 2));
	    strncpy(tmp_20k_buf + 80
		               - ((context->type & FTYPE_REMOTE) ? 9 : 8),
		    (context->type & FTYPE_REMOTE)?"(Remote)":"(Local)",
		    (context->type & FTYPE_REMOTE) ? 8 : 7);
	}
	    print_text(tmp_20k_buf);
	    print_text(NEWLINE);
	    i++;

	memset((void *)tmp_20k_buf,LINECH,80 * sizeof(char));
	tmp_20k_buf[80] = '\0';
	print_text(tmp_20k_buf);
	print_text(NEWLINE);
	i++;

	for(i++; folder_total(context->folders) 
	    && i < (int)folder_entry(0, context->folders)->d_line ; i++)
	  print_text(NEWLINE);

	*tmp_20k_buf = '\0';
	for(index=0; index < folder_total(context->folders); index++){
	    f = folder_entry(index, context->folders);
	    if(f->d_col == 0){
		i++;
		strcat(tmp_20k_buf, NEWLINE);
		print_text(tmp_20k_buf);
		tmp_20k_buf[0] = '\0';
	    }

	    l = strlen(tmp_20k_buf);
	    if(context->type & FTYPE_BBOARD)
	      sprintf(buf, "%*s%s", 
		      strlen(f->prefix) + max(0,((int)f->d_col - l)),
		      f->prefix,
		      f->name);
	    else
	      sprintf(buf, "%*s", f->name_len + max(0,((int)f->d_col - l)),
		      FLDR_NAME(f));
	    strcat(tmp_20k_buf, buf);
	}

	print_text(tmp_20k_buf);
	print_text(NEWLINE);
	context = context->next;
    }

    close_printer();
    if(ps_global->ttyo->screen_cols != 80)
      create_folder_display(display, ps_global->ttyo->screen_cols);
}

                     

/*----------------------------------------------------------------------
  Search folder list

   Args: fd       -- The folder display structure
         index    -- pointer to index of current folder (new folder if found)
         context  -- pointer to context of current folder (new folder if found)
         ask_line -- Screen line to prompt on

 Result: returns 
                -1 if aborted
                 0 if NOT found
		 1 if found
		 2 if found and wrapped
  ----------------------------------------------------------------------*/
int
search_folders(fd, ask_line)
    FSTATE_S   *fd;
    int         ask_line;
{
    char            prompt[MAX_SEARCH+50], nsearch_string[MAX_SEARCH+1];
    HelpType        help = NO_HELP;
    CONTEXT_S      *t_context;
    FOLDER_S       *f;
    int             rc, t_index;
    static char     search_string[MAX_SEARCH+1];
    static ESCKEY_S folder_search_keys[] = { { 0, 0, "", "" },
					    {ctrl('Y'), 10, "^Y", "First Fldr"},
					    {ctrl('V'), 11, "^V", "Last Fldr"},
					    {-1, 0, NULL, NULL} };

    nsearch_string[0] = '\0';
    if(!folder_total((t_context = fd->context)->folders)){
	q_status_message(SM_ORDER | SM_DING, 0, 4,
			 "Empty folder collection.  No folders to search!");
	return(0);
    }

    t_index           = fd->folder_index;
    sprintf(prompt, "Folder name to search for %s%s%s: ", 
	    (*search_string == '\0') ? "" : "[", 
	    search_string,
	    (*search_string == '\0') ? "" : "] ");

    while(1) {
        rc = optionally_enter(nsearch_string, ask_line, 0, MAX_SEARCH, 1,
			      0, prompt, folder_search_keys, help,0);
        if(rc == 3) {
            help = help == NO_HELP ? h_oe_foldsearch : NO_HELP;
            continue;
        }
	else if(rc == 10){
	    fd->context      = fd->context_list;
	    fd->folder_index = 0;
	    q_status_message(SM_ORDER, 0, 3, "Searched to First Folder.");
	    return(3);
	}
	else if(rc == 11){
	    for(fd->context = fd->context_list;
		fd->context->next; 
		fd->context = fd->context->next)
	      ;

	    fd->folder_index = (ALL_FOUND(fd->context)) 
				? folder_total(fd->context->folders) - 1: 0;
	    q_status_message(SM_ORDER, 0, 3, "Searched to Last Folder.");
	    return(3);
	}

        if(rc != 4)
          break;
    }

    if(rc == 1 || (search_string[0] == '\0' && nsearch_string[0] == '\0'))
      return(-1);			/* abort */

    if(nsearch_string[0] != '\0')
      strcpy(search_string, nsearch_string);

    /*----- Search the bottom half of list ------*/
    rc = 0;
    while(1){
	if(t_index + 1 >= folder_total(t_context->folders)){
	    t_index = 0;
	    if(!(t_context = t_context->next)){
		t_context = fd->context_list; 	/* wrap the search */
		rc = 1;
	    }
	}
	else
	  t_index++;

	if(t_index == fd->folder_index && t_context == fd->context)
	  return(0);

	f = folder_entry(t_index, t_context->folders);
        if(srchstr(FLDR_NAME(f), search_string)){
	    fd->folder_index   = t_index;
	    fd->context        = t_context;
	    return(rc + 1);
	}
    }
}

#ifdef NEWBB
/*----------------------------------------------------------------------
  Clears the "NEW " prefix off all the news groups that have it and
sorts them back into their normal positions in the list.  Also, resets
the time to check new groups against.

Args: flist  -- The folder list display structure to clear and sort

Returns: nothing

Using ctime format for the last time checked is OK, Probably should
use RFC-822 date format.  The important thing is that it is ASCII and
that code exists to parse it and convert it to the correct format.
  ----*/
void
clear_new_groups(flist)
     void *flist;
{
    int i;
    long now;


    /*------ Set the time to check for new groups with to now -----*/
    now    = time(0);
    set_variable(V_NNTP_NEW_GROUP_TIME, ctime(&now), 0);

    /*---- Change the "NEW " prefixes in the folder list to "    " -----*/
    for(i = 0; i < folder_total(flist); i++) {
        if(strcmp(folder_entry(i, flist)->prefix, "NEW ") == 0) {
            strcpy(folder_entry(i, flist)->prefix, "    ");
        } else {
            /* Done cause all new ones are at the top */
            break;
        }
    }

    /*--- Put the list in order, those that were new no longer at top ---*/
    sort_folder_list(flist, compare_folders);
}



/*----------------------------------------------------------------------
   Convert a date in ctime(3) format to the format required by NNTP
  NEWGROUP command (YYMMDD HHMMSS). 

Args: date -- Date string in ctime format 
       
Returns: date in NNTP format in static buffer 
  ----*/
char *
ctime2nntp(date)
     char *date;
{
    struct date d;
    static char timebuf[40];

    parse_date(date, &d);

    sprintf(timebuf, "%02d%02d%02d %02d%02d%02d",
            d.year % 100, d.month, d.day, d.hour, d.minute, d.sec);
    return(timebuf);
}
#endif    


/*----------------------------------------------------------------------
      compare two names for qsort, case independent

   Args: pointers to strings to compare

 Result: integer result of strcmp of the names.  Uses simple 
         efficiency hack to speed the string comparisons up a bit.

  ----------------------------------------------------------------------*/
int
compare_names(x, y)
    const QSType *x, *y;
{
    char *a = *(char **)x, *b = *(char **)y;
    int r;
#if defined(DOS) || defined(OS2)
#define	STRCMP	strucmp
#define	CMPI	UCMPI
#else
#define	STRCMP	strcmp
#define	CMPI(X,Y)	((X)[0] - (Y)[0])
#endif
#define	UCMPI(X,Y)	((isupper((unsigned char)((X)[0]))	\
				? (X)[0] - 'A' + 'a' : (X)[0])	\
			  - (isupper((unsigned char)((Y)[0]))	\
				? (Y)[0] - 'A' + 'a' : (Y)[0]))

    /*---- Inbox always sorts to the top ----*/
    if((UCMPI(b, ps_global->inbox_name)) == 0
       && strucmp(b, ps_global->inbox_name) == 0)
      return((CMPI(a, b) == 0 && STRCMP(a, b) == 0) ? 0 : 1);
    else if(CMPI(b, ps_global->VAR_DEFAULT_FCC) == 0
	    && STRCMP(b, ps_global->VAR_DEFAULT_FCC) == 0)
      return((CMPI(a, b) == 0 && STRCMP(a, b) == 0) ? 0 : 1);
    else if(CMPI(b, ps_global->VAR_DEFAULT_SAVE_FOLDER) == 0
	    && STRCMP(b, ps_global->VAR_DEFAULT_SAVE_FOLDER) == 0)
      return((CMPI(a, b) == 0 && STRCMP(a, b) == 0) ? 0 : 1);
    else if(UCMPI(a, ps_global->inbox_name) == 0
	    && strucmp(a, ps_global->inbox_name) == 0)
      return((CMPI(a, b) == 0 && STRCMP(a, b) == 0) ? 0 : -1);
    /*----- The sent-mail folder, is always next ---*/
    else if(CMPI(a, ps_global->VAR_DEFAULT_FCC) == 0
	    && STRCMP(a, ps_global->VAR_DEFAULT_FCC) == 0)
      return((CMPI(a, b) == 0 && STRCMP(a, b) == 0) ? 0 : -1);
    /*----- The saved-messages folder, is always next ---*/
    else if(CMPI(a, ps_global->VAR_DEFAULT_SAVE_FOLDER) == 0
	    && STRCMP(a, ps_global->VAR_DEFAULT_SAVE_FOLDER) == 0)
      return((CMPI(a, b) == 0 && STRCMP(a, b) == 0) ? 0 : -1);
    else
      return((r = CMPI(a, b)) ? r : STRCMP(a, b));
}



/*----------------------------------------------------------------------
  This code calculate the screen arrangement for the folders screen.
It fills in the line and col number for each entry in the list.

Args: f_list  -- The folder list
      f_list_size -- The number of entries in the folder list 
      screen_cols -- The number of columns on the screen


BUG - test this with one, two and three folders
Returns: The folder display structure
  ----*/

compare_sizes(f1, f2)
     const QSType *f1, *f2;
{
    return((*(struct folder **)f1)->name_len - 
           (*(struct folder **)f2)->name_len);
}

compare_folders(f1, f2)
     const QSType *f1, *f2;
{
    char *s1, *s2;

    s1 = (*(FOLDER_S **)f1)->name;
    s2 = (*(FOLDER_S **)f2)->name;

    return(compare_names(&s1, &s2));
}

#ifdef NEWBB
/*----------------------------------------------------------------------
   Folder comparison that puts those with "NEW " prefix at top of list
  ---*/
compare_folders_new(f1, f2)
     const QSType *f1, *f2;
{
    char *s1, *s2;
    int   is_new1, is_new2;

    s1      = (*((FOLDER_S **)(f1)))->name;
    s2      = (*((FOLDER_S **)(f2)))->name;
    is_new1 = strcmp(((*(FOLDER_S **)(f1)))->prefix, "NEW ") == 0;
    is_new2 = strcmp(((*(FOLDER_S **)(f2)))->prefix, "NEW ") == 0;

    if(!(is_new1 ^ is_new2))
      return(compare_names(&s1, &s2));
    else if(is_new1)
      return(-11);
    else
      return(1);
}
#endif

/*----------------------------------------------------------------------
   Calculate the arrangement on the screen. This fills in the rwo and column
 in the struct folders in the global Pine folder list. 

Args: fold_disp  -- folder menu state 
      screen_cols -- The width of the screen

Names are passed pre-sorted.
  ---*/

void
create_folder_display(fold_disp, screen_cols)
     FSTATE_S *fold_disp;
     int       screen_cols;
{
    register int       index;
    register FOLDER_S *f;
    CONTEXT_S         *c_list;
    int                length = 0, row, col, goal;

    if(!fold_disp){			/* what? */
	q_status_message(SM_ORDER,3,3,
	    "Programmer BOTCH: No folder state struct!");
	return;
    }

    row                      = (ps_global->context_list->next) ? 1 : 0;
    c_list                   = fold_disp->context_list;
    fold_disp->display_cols  = screen_cols;
    fold_disp->display_rows  = ps_global->ttyo->screen_rows 
                                 - FOOTER_ROWS(ps_global)
                                 - HEADER_ROWS(ps_global);
    while(c_list != NULL){
	/*--- 
	  Figure out the column width to use for display. What we want is a 
	  column width that looks nice for most of the folder names, but
	  not to make the columns super wide because of a few folder names
	  of exceptional length. This is done by sorting the lengths of the
	  existing folders and using a width that will suit 95% of the 
	  entries. 

	  The miminum columns width used is 20.
	 ----*/
	if(!ALL_FOUND(c_list)){
	    length = strlen(CLICKHERE);
	}
	else if(!folder_total(c_list->folders) || (c_list->use&CNTXT_PSEUDO)){
	    length = strlen(c_list->use&CNTXT_NEWS ? CLICKHERETOONEWS
						   : CLICKHERETOO);
	}
	else if(F_ON(F_VERT_FOLDER_LIST, ps_global)){
	    length = screen_cols; /* not used */
	}
	else{
	    length = 0;		/* start from scratch */
	    for(index = 0; index < folder_total(c_list->folders); index++){
		f = folder_entry(index, c_list->folders);
		length = max(length, (int)f->name_len
					+ (f->prefix ? strlen(f->prefix) : 0)
					+ 1);
	    }
	}

	length = max(20, length);


	/*---- 
	  Now we go through the display list and fill in the rows and columns.
	  If the entry is just a text entry then it is centered. If it is
	  a folder name it is fit in after the preceed one if possible. If 
	  not it is put on the line boundrieds
         ---*/

	if(ps_global->context_list->next){
	    /* leave a line for each label... */
	    for(index = 0; c_list->label[index] != NULL; index++){
		if(index == 0)
		  c_list->d_line = row;	/* remember which row to start on */

		row++;
	    }

	    row++;			/* one blank line after labels */
	    row++;			/* one more blank line after labels */
	}

	/* then assign positions for each folder name */
	col = 0;
	if(!ALL_FOUND(c_list)){
	    if(c_list->use & CNTXT_PSEUDO){
		f = folder_entry(0, c_list->folders);
	    }
	    else{
		f = new_folder(CLICKHERE);
		folder_insert(0, f, c_list->folders);
		c_list->use |= CNTXT_PSEUDO;
	    }

	    f->d_line = row;
	    f->d_col  = max(0,
                         (screen_cols - (int)f->name_len-strlen(f->prefix))/2);
	}
	else if(!folder_total(c_list->folders) || (c_list->use&CNTXT_PSEUDO)){
	    if(c_list->use & CNTXT_PSEUDO)
	      folder_delete(0, c_list->folders); /* may be CLICKHERE */

	    f = new_folder(c_list->use&CNTXT_NEWS ? CLICKHERETOONEWS
						  : CLICKHERETOO);
	    folder_insert(0, f, c_list->folders);
	    c_list->use |= CNTXT_PSEUDO; /* let others know entry's bogus */
	    f->d_line    = row;
	    f->d_col     = max(0, (screen_cols - (int)f->name_len -
                                   strlen(f->prefix))/2);
	}
	/* one folder per line */
	else if(F_ON(F_VERT_FOLDER_LIST, ps_global)){
	    for(index = 0; index < folder_total(c_list->folders); index++){
		f    = folder_entry(index, c_list->folders);
		if(index)
		  row++;

		f->d_line = (unsigned int) row;
		f->d_col  = (unsigned int) col;
	    }
	}
	else{
	    int plen;

	    for(index = 0; index < folder_total(c_list->folders); index++){
		f    = folder_entry(index, c_list->folders);
		plen = f->prefix ? strlen(f->prefix) : 0;

		if(col + (int)f->name_len + plen >= screen_cols){
		    col = 0;
		    row++;
		}

		f->d_line = (unsigned int) row;
		f->d_col  = (unsigned int) col;

		for(goal = 0; goal < (int) f->name_len + plen;
                    goal += length)
		  col += length;
	    }
	}

	fold_disp->last_row = row++;
	row++;				/* add a blank line */
	row++;				/* add another blank line */

	c_list = c_list->next; 	/* format the next section... */
    }
}



/*----------------------------------------------------------------------
      Format the given folder name for display for the user

   Args: folder -- The folder name to fix up

Not sure this always makes it prettier. It could do nice truncation if we
passed in a length. Right now it adds the path name of the mail 
subdirectory if appropriate.
 ----*/
      
char *
pretty_fn(folder)
     char *folder;
{
    static char  pfn[MAXFOLDER * 2 + 1];
    char        *p;

#if defined(DOS) || defined(OS2)
    if(!ps_global->show_folders_dir || *folder == '\\' || 
#else
    if(!ps_global->show_folders_dir || *folder == '/' || *folder == '~' ||
#endif
       *folder == '{' || *folder == '\0' 
       || !strucmp(folder, ps_global->inbox_name))  {
        if(ps_global->nr_mode) {
            if((p = strindex(folder, '}')) != NULL)
              return(p +1);
            else if((p = strindex(folder, '/')) != NULL) 
              return(p+1);
	    else
              return(folder);
        }
	else if(!strucmp(folder, ps_global->inbox_name)){
	    strcpy(pfn, ps_global->inbox_name);
	    return(pfn);
	}
	else
          return(folder);

    } else {
        build_path(pfn, ps_global->VAR_MAIL_DIRECTORY, folder);
        return(pfn);
    }
}



/*----------------------------------------------------------------------
       Check to see if folder exists in given context

  Args: c_string -- context to check for folder in
        file  -- name of folder to check
 
 Result: returns 1 if the folder exists
                 0 if not
		-1 on error

  Uses mail_find to sniff out the existance of the requested folder.
  The context string is just here for convenience.  Checking for
  folder's existance within a given context is probably more efficiently
  handled outside this function for now using find_folders_in_context().

    ----*/
int
folder_exists(c_string, file)
    char *c_string, *file;
{
    char         fn[MAXPATH+1], host[MAXPATH+1], mbox[MAXPATH+1], *find_string;
    int          ourstream = 0, old_dot_state, old_inbox_state, we_cancel = 0;
    MAILSTREAM  *find_stream = NULL;

    we_cancel = busy_alarm(1, NULL, NULL, 0);

    if(c_string && *c_string && context_isambig(file))
      context_apply(fn, c_string, file);
    else
      strcpy(fn, file);

    find_stream = mail_valid(NULL, fn, NULL) ? mail_open(NULL,fn,OP_PROTOTYPE)
					     : NULL;
    if(find_stream && mail_valid_net(fn, find_stream->dtb, host, mbox)){
	if(!(find_stream = same_stream(fn, ps_global->mail_stream))){
	    if(!(find_stream = same_stream(fn, ps_global->inbox_stream))){
		char tmp[MAXPATH], options[MAXPATH], *p = fn, *p2;
		while(*p && *p != '}' && *p != '/')
		  p++;

		options[0] = '\0';
		if(*p == '/'){
		    p2 = options;
		    while(*p != '}')
		      *p2++ = *p++;

		    *p2 = '\0';
		}

		sprintf(tmp, "%s{%s%s}", (fn[0] == '*') ? "*" : "", host,
			options);
		ourstream++;
		if(!(find_stream = mail_open(NULL, tmp, OP_HALFOPEN))){
		    if(we_cancel)
		      cancel_busy_alarm(-1);

		    return(-1);	/* mail_open should've displayed error */
		}
	    }
	}

	find_string = mbox;
    }
    else
      find_string = fn;

    find_folder_list	      = NULL;
    find_folder_count	      = 0L;
    old_dot_state	      = ps_global->show_dot_names;
    ps_global->show_dot_names = 1; 		/* look everywhere */
    old_inbox_state	      = find_folder_inbox;
    find_folder_inbox	      = 1;		/* including "inbox" */
    if(find_stream && find_stream->mailbox && find_stream->mailbox[0] == '*')
      context_find_all_bboard(NULL, find_stream, find_string);
    else
      context_find_all(NULL, find_stream, find_string);

    ps_global->show_dot_names = old_dot_state;
    find_folder_inbox	      = old_inbox_state;
    if(ourstream)
      mail_close(find_stream);

    if(we_cancel)
      cancel_busy_alarm(-1);

    return(find_folder_count > 0L);
}



/*----------------------------------------------------------------------
       Check to see if folder exists in given context

  Args: stream -- pointer to stream to use or create
	cntxt -- context to check for folder in
        folder -- name of folder to check for recent messages
 
 Result: returns 1 if there are recent messages (and *stream assigned)
                 0 if not

    ----*/
int
folder_has_recent(stream, cntxt, folder)
    MAILSTREAM **stream;
    CONTEXT_S	*cntxt;
    FOLDER_S	*folder;
{
    int         rv, we_cancel = 0;
    char       *fn;
    char        msg_buf[MAX_SCREEN_COLS+1];

    if(folder)
      fn = FLDR_NAME(folder);
    else
      return(0);

    strcat(strncat(strcpy(msg_buf, "Checking "), fn, 50),
	   " for recent messages");
    we_cancel = busy_alarm(1, msg_buf, NULL, 1);

    /* current folder can't have recent */
    rv = ((ps_global->context_current != ps_global->context_list
	   || strcmp(ps_global->cur_folder, fn))
	  && stream
	  && ((*stream) = context_open(cntxt->context, *stream,
				       folder->name, OP_READONLY))
	  && (*stream)->recent > 0L);

    if(we_cancel)
      cancel_busy_alarm(0);

    return(rv);
}



/*----------------------------------------------------------------------
 Initialize global list of contexts for folder collections.

 Interprets collections defined in the pinerc and orders them for
 pine's use.  Parses user-provided context labels and sets appropriate 
 use use flags and the default prototype for that collection. 
 (See find_folders for how the actual folder list is found).

  ----*/
void
init_folders(ps)
    struct pine *ps;
{
    CONTEXT_S  *tc, *top = NULL, **clist, *prime = NULL;
    FOLDER_S   *f;
    int         num = 1, i;

    clist = &top;

    /*
     * If no incoming folders are config'd, but the user asked for
     * them via feature, make sure at least "inbox" ends up there...
     */
    if(F_ON(F_ENABLE_INCOMING, ps) && !ps->VAR_INCOMING_FOLDERS){
	ps->VAR_INCOMING_FOLDERS    = (char **)fs_get(2 * sizeof(char *));
	ps->VAR_INCOMING_FOLDERS[0] = cpystr(ps->inbox_name);
	ps->VAR_INCOMING_FOLDERS[1] = NULL;
    }

    /*
     * Build context that's a list of folders the user's defined
     * as receiveing new messages.  At some point, this should
     * probably include adding a prefix with the new message count.
     */
    if(ps->VAR_INCOMING_FOLDERS && ps->VAR_INCOMING_FOLDERS[0]
       && (tc = new_context("Incoming-Folders []"))){ /* fake new context */
	tc->use    &= ~CNTXT_NOFIND; 	/* mark all entries found */
	tc->use    |= CNTXT_INCMNG;	/* mark this as incoming collection */
	tc->num     = 0;
	if(tc->label[0])
	  fs_give((void **)&tc->label[0]);

	tc->label[0] = cpystr("Incoming Message Folders");

	*clist = tc;
	clist  = &tc->next;

	for(i = 0; ps->VAR_INCOMING_FOLDERS[i] ; i++){
	    char *folder_string, *nickname;

	    /*
	     * Parse folder line for nickname and folder name.
	     * No nickname on line is OK.
	     */
	    get_pair(ps->VAR_INCOMING_FOLDERS[i], &nickname, &folder_string,0);

	    /*
	     * Allow for inbox to be specified in the incoming list, but
	     * don't let it show up along side the one magically inserted
	     * above!
	     */
	    if(!folder_string || !strucmp(ps->inbox_name, folder_string)){
		if(folder_string)
		  fs_give((void **)&folder_string);

		if(nickname)
		  fs_give((void **)&nickname);

		continue;
	    }

	    f = new_folder(folder_string);
	    fs_give((void **)&folder_string);
	    if(nickname){
		if(strucmp(ps->inbox_name, nickname)){
		    f->nickname = nickname;
		    f->name_len = strlen(f->nickname);
		}
		else
		  fs_give((void **)&nickname);
	    }

	    folder_insert(f->nickname 
			  && (strucmp(f->nickname, ps->inbox_name) == 0)
				? -1 : folder_total(tc->folders),
			  f, tc->folders);
	}
    }

    /*
     * Build list of folder collections.  Because of the way init.c
     * works, we're guaranteed at least a default.  Also write any
     * "bogus format" messages...
     */
    for(i = 0; ps->VAR_FOLDER_SPEC && ps->VAR_FOLDER_SPEC[i] ; i++){
	if(tc = new_context(ps->VAR_FOLDER_SPEC[i])){
	    if(!prime){
		prime       = tc;
		prime->use |= (CNTXT_PRIME | CNTXT_SAVEDFLT);
	    }
	    else
	      tc->use  |= CNTXT_SECOND;

	    tc->num     = num++;			/* place in the list */
	    *clist      = tc;				/* add it to list   */
	    clist       = &tc->next;			/* prepare for next */
	}
    }


    /*
     * Whoah cowboy!!!  Guess we couldn't find a valid folder
     * collection, so fall back on default...
     */
    if(!prime){
	char buf[MAXPATH];

	build_path(buf, ps->VAR_MAIL_DIRECTORY, "[]");
	tc          = new_context(buf);
	tc->use    |= (CNTXT_PRIME | CNTXT_SAVEDFLT);
	tc->num     = num++;				/* place in the list */
	*clist      = tc;				/* add it to list   */
	clist       = &tc->next;			/* prepare for next */
    }

    /*
     * At this point, insert the INBOX mapping as the leading
     * folder entry of the first collection...
     */
    init_inbox_mapping(ps->VAR_INBOX_PATH, top);

    /*
     * If no news collections spec'd, but an nntp-server defined, 
     * fake a default news collection.  Check only "user" and "global"
     * nntp_server values as the "current" value is influenced by
     *  env vars and other news config files (see init.c)...
     */
    if(!ps->VAR_NEWS_SPEC
       && ((ps->USR_NNTP_SERVER
	    && ps->USR_NNTP_SERVER[0])
	   || (ps->GLO_NNTP_SERVER
	       && ps->GLO_NNTP_SERVER[0]))){
	char buf[MAXPATH];

	ps->VAR_NEWS_SPEC    = (char **)fs_get(2 * sizeof(char *));
	sprintf(buf, "*{%s/nntp}[]", ps->VAR_NNTP_SERVER[0]);
	ps->VAR_NEWS_SPEC[0] = cpystr(buf);
	ps->VAR_NEWS_SPEC[1] = NULL;
    }

    /*
     * If news groups, loop thru list adding to collection list
     */
    for(i = 0; ps->VAR_NEWS_SPEC && ps->VAR_NEWS_SPEC[i] ; i++){
	if(ps->VAR_NEWS_SPEC[i][0]
	   && (tc = new_context(ps->VAR_NEWS_SPEC[i]))){
	    tc->use    |= CNTXT_NEWS;
	    tc->num     = num++;
	    *clist      = tc;			/* add it to list   */
	    clist       = &tc->next;		/* prepare for next */
	}
    }

    ps->context_list    = top;	/* init pointers */
    ps->context_current = (top->use & CNTXT_INCMNG) ? top->next : top;
#ifdef	DEBUG
    if(debug > 7 && do_debug(debugfile))
      dump_contexts(debugfile);
#endif
}



#ifdef	DEBUG
dump_contexts(fp)
    FILE *fp;
{
    FOLDER_S *f;
    int i = 0;
    CONTEXT_S *c = ps_global->context_list;

    while(fp && c != NULL){
	fprintf(fp, "\n***** context %s\n", c->context);
	for(i=0;c->label[i] != NULL;i++)
	  fprintf(fp,"LABEL: %s\n", c->label[i]);
	
	for(i = 0; i < folder_total(c->folders); i++)
	  fprintf(fp, "  %d) %s\n", i + 1, folder_entry(i,c->folders)->name);
	
	c = c->next;
    }
}
#endif


/*
 * Return malloc'd string containing only the context specifier given
 * a string containing the raw pinerc entry
 */
char *
context_string(s)
    char *s;
{
    CONTEXT_S *t = new_context(s);
    char      *rv = NULL;
    int	       i;

    if(t){
	/*
	 * Take what we want from context, then free the rest...
	 */
	rv = t->context;
	t->context = NULL;			/* don't free it! */
	free_context(&t);
    }

    return(rv ? rv : cpystr(""));
}


/*
 *  Release resources associated with global context list
 */
void
free_folders()
{
    CONTEXT_S  *blast;

    while(blast = ps_global->context_list){
	ps_global->context_list = ps_global->context_list->next;
	free_context(&blast);			/* blast the context */
    }

    ps_global->context_current = NULL;
}


/*
 *  Release resources associated with the given context structure
 */
void
free_context(cntxt)
    CONTEXT_S **cntxt;
{
    char **labels;

    if((*cntxt)->context)
      fs_give((void **)&(*cntxt)->context);
    
    for(labels = (*cntxt)->label; *labels; labels++)
      fs_give((void **)labels);
    
    if((*cntxt)->nickname)
      fs_give((void **)&(*cntxt)->nickname);

    if((*cntxt)->folders)
      free_folder_list((void **)&(*cntxt)->folders);

    fs_give((void **)cntxt);
}


/*
 *
 */
void
init_inbox_mapping(path, cntxt)
     char      *path;
     CONTEXT_S *cntxt;
{
    FOLDER_S *f;

    /*
     * If mapping already exists, blast it and replace it below...
     */
    if((f = folder_entry(0, cntxt->folders))
       && f->nickname && !strcmp(f->nickname, ps_global->inbox_name))
      folder_delete(0, cntxt->folders);

    if(path){
	f = new_folder(path);
	f->nickname = cpystr(ps_global->inbox_name);
	f->name_len = strlen(f->nickname);
    }
    else
      f = new_folder(ps_global->inbox_name);

    folder_insert(0, f, cntxt->folders);
}


/*
 * returns with the folder's type's set
 *
 * WARNING: c-client naming knowledge is hardcoded here!
 *          This is based on our understanding of c-client naming 
 *	    conventions:
 *
 *		leading '*' means local or remote bboard (news)
 *		leading '{' means remote access
 *		{XXX/nntp}  means remote nntp access
 *		{XXX/imap}  means remote imap access
 *		{XXX/anonymous}  means anonymous access
BUG? look into using mail.c:mail_valid_net()
 */
void
set_ftype(context, flags)
    char           *context;
    unsigned short *flags;
{
    char *p, tmp[MAXPATH];
    long  i;

    *flags = 0;					/* clear flags */

    if(!context || *context == '\0')
      return;

    if(*context == '*'){
	*flags |= FTYPE_BBOARD;
	context++;
    }

    if(*context == '{' && context[1] && context[1] != '}' 
       && (p = strindex(context, '}'))){
	*flags |= FTYPE_REMOTE;
	i = p - context;
	strncpy(tmp, context, (int)i);
	tmp[i] = '\0';
	if(*p == '*')
	  *flags |= FTYPE_BBOARD;

	if((p = strindex(tmp, '/')) && strucmp(p+1, "nntp") == 0)
	  *flags |= FTYPE_OLDTECH;

	if(p && strucmp(p+1, "anonymous") == 0)
	  *flags |= FTYPE_ANON;
    }
    else
      *flags |= FTYPE_LOCAL;		/* if it's not remote... */
}



/*
 * new_context - creates and fills in a new context structure, leaving
 *               blank the actual folder list (to be filled in later as
 *               needed).  Also, parses the context string provided 
 *               picking out any user defined label.  Context lines are
 *               of the form:
 *
 *               [ ["] <string> ["] <white-space>] <context>
 *
 */
CONTEXT_S *
new_context(cntxt_string)
    char *cntxt_string;
{
    CONTEXT_S  *c;
    char        host[MAXPATH], rcontext[MAXPATH], *nickname = NULL,
		*c_string = NULL, *p;

    /*
     * do any context string parsing (like splitting user-supplied 
     * label from actual context)...
     */
    get_pair(cntxt_string, &nickname, &c_string, 0);

    if(p = context_digest(c_string, NULL, host, rcontext, NULL)){
	q_status_message2(SM_ORDER | SM_DING, 3, 4,
			  "Bad context, %s : %s", p, c_string);
	if(nickname)
	  fs_give((void **)&nickname);

	return(NULL);
    }

    c = (CONTEXT_S *) fs_get(sizeof(CONTEXT_S)); /* get new context   */
    memset((void *) c, 0, sizeof(CONTEXT_S));    /* and initialize it */
    set_ftype(c_string, &(c->type));

    if(c->label[0] == NULL){
	if(!nickname){			/* make one up! */
	    sprintf(tmp_20k_buf, "%s%s%s",
		    (c->type & FTYPE_BBOARD) ? "News" : rcontext, 
		    (c->type & FTYPE_REMOTE) ? " on " : "",
		    (c->type & FTYPE_REMOTE) ? host : "");
	    c->label[0]   = cpystr(tmp_20k_buf);
	}
	else
	  c->label[0] = nickname;
    }

    c->context = c_string;
    c->use     = CNTXT_NOFIND;		/* do find later! */
    c->folders = new_folder_list();


    dprint(2, (debugfile, "Context %s type:%s%s%s%s%s%s\n", c->context,
	       (c->type&FTYPE_LOCAL)   ? " LOCAL"   : "",
	       (c->type&FTYPE_REMOTE)  ? " REMOTE"  : "",
	       (c->type&FTYPE_SHARED)  ? " SHARED"  : "",
	       (c->type&FTYPE_BBOARD)  ? " BBOARD"  : "",
	       (c->type&FTYPE_OLDTECH) ? " OLDTECH"  : "",
	       (c->type&FTYPE_ANON)    ? " ANON" : ""));

    return(c);
}



/*
 * find_folders_in_context - find the folders associated with the given context
 *                     and search pattern.  If the search pattern is not
 *                     the wild card ("*"), then some subset of all folders
 *                     is specified. So, don't mark context as completely
 *                     searched, but do set the bit to avoid recursive
 *                     calls...
 *
 *        If no search_string, we're being called to search the entire
 *        view within the given context.
 *
 *	NOTE: The first arg, "stream", is here for efficiency.
 *	      If it's set, then the caller want's the stream we opened
 *	      for the context_find(), so don't close it when we leave.
 *	      If we don't use it and it's set, mail_close and NULL it.
 *	      If we use another stream and it's set, close the old one
 *	      and reset it to the new one.  It's up to the caller to
 *	      make sure it get's closed properly.
 *
 */
void
find_folders_in_context(stream, context, search_string)
    MAILSTREAM **stream;
    CONTEXT_S	*context;
    char	*search_string;
{
    char  host[MAXPATH], rcontext[MAXPATH], view[MAXPATH],
         *search_context, *rv;
    int   local_open = 0, we_cancel = 0;

    if((context->use&CNTXT_NOFIND) == 0 || (context->use&CNTXT_PARTFIND))
      return;				/* find already done! */

    if(rv = context_digest(context->context, NULL, NULL, rcontext, view)){
	q_status_message2(SM_ORDER | SM_DING, 3, 3, "Bad context, %s : %s",
			  rv, context->context);
	return;
    }

    we_cancel = busy_alarm(1, NULL, NULL, 0);

    if(!search_string || (search_string[0]=='*' && search_string[1]=='\0')){
	context->use &= ~CNTXT_NOFIND;	/* let'em know we tried  */
	search_string = view;		/* return full view of context */
    }
    else
      context->use |= CNTXT_PARTFIND;	/* or are in a partial find */

    /* let context_mailbox know context! */
    current_context   = context->context;
    find_folder_list  = context->folders;

    dprint(7, (debugfile, "find_folders_in_context: %s\n",
	       context->context));

    /*
     * Here's where we have to be careful.  C-client requires different
     * search strings and inconsistently returns results based on 
     * what sort of technology you're interested in... (would be nice to 
     * see this cleaned up)
     *
     *     Type      Search           Search returns
     *     ----      ------           --------------
     * local mail    mail/*           /usr/staff/mikes/mail/foo
     * imap mail     {remote}mail/*   nothing (unless {} in mboxlist names)
     * imap mail     mail/*           {remote}mail/foo
     * local news    *                foo.bar (if news spool local!)
     * imap news     *                ????
     * nntp news     *                foo.bar
     * nntp news     [host]*          [host]foo.bar
     *
     */

    find_folder_stream = NULL;
    if((context->type)&FTYPE_REMOTE){
	char *stream_name;
	search_context = rcontext;

	sprintf(tmp_20k_buf, "%.*s}",
		strindex(context->context, '}') - context->context, 
		context->context);

	stream_name = cpystr(tmp_20k_buf);

	/*
	 * Try using a stream we've already got open...
	 */
	if(stream && *stream
	   && !(find_folder_stream = same_stream(stream_name, *stream))){
	    mail_close(*stream);
	    *stream = NULL;
	}

	if(!find_folder_stream)
	  find_folder_stream = same_stream(stream_name,
					   ps_global->mail_stream);

	if(!find_folder_stream 
	   && ps_global->mail_stream != ps_global->inbox_stream)
	  find_folder_stream = same_stream(stream_name,
					   ps_global->inbox_stream);

	/* gotta open a new one */
	if(!find_folder_stream){
	    if((context->type&FTYPE_OLDTECH) && !(context->use&CNTXT_FINDALL)){
		find_folder_stream = mail_open(NULL, stream_name,OP_PROTOTYPE);
	    }
	    else{
		local_open++;
		find_folder_stream = mail_open(NULL, stream_name, OP_HALFOPEN);
		if(stream)
		  *stream = find_folder_stream;
	    }
	}

	dprint(find_folder_stream ? 7 : 1,
	       (debugfile, "find_folders: mail_open(%s) %s.\n",
		stream_name, find_folder_stream ? "OK" : "FAILED"));
	fs_give((void **)&stream_name);
	if(!find_folder_stream){
	    context->use &= ~CNTXT_PARTFIND;	/* unset partial find bit */
	    if(we_cancel)
	      cancel_busy_alarm(-1);

	    return;
	}
    }
    else{
	search_context = rcontext;
	if(stream && *stream){
	    mail_close(*stream);
	    *stream = NULL;
	}
    }

    ps_global->show_dot_names = F_ON(F_ENABLE_DOT_FOLDERS, ps_global);

    if(context->type & FTYPE_BBOARD){			/* get the list */
#ifdef NEWBB
        if(context->use & CNTXT_NEWBB) 
          context_find_new_bboard(search_context, find_folder_stream,
                             search_string,
                             ctime2nntp(ps_global->VAR_NNTP_NEW_GROUP_TIME));
        else
#endif
        if(context->use & CNTXT_FINDALL)
          context_find_all_bboard(search_context, find_folder_stream,
                               search_string);
        else 
 	  context_find_bboards(search_context, find_folder_stream,
                                 search_string);

        if((F_OFF(F_READ_IN_NEWSRC_ORDER,ps_global) ||
            context->use & CNTXT_FINDALL)
#ifdef NEWBB
          && !(context->use & CNTXT_NEWBB)
#endif
            )
           sort_folder_list(context->folders, compare_folders);
    }
    else{
	/*
	 * For now use find_all as there's no distinguishing in pine
	 * between subscribed and not with regard to mailboxes.  At some
	 * point we may want to use the subscription mechanism to help 
	 * performance, but implicit subscription needs to be added and 
	 * folders created outside pine won't be automatically visible
	 *
	 * THOUGHT: the first find as pine starts does a find_all and then  
	 * a find then reconciles the two automatically subscribing
	 * any new folders.  We can then easily rebuild folder list on 
	 * every folder menu access much cheaper (locally anyway) 
	 * (probably only necessary to rebuild the section that has
	 * to do with the current context)
	 */
	context_find_all(search_context, find_folder_stream, search_string);
    }

    if(local_open && !stream)
      mail_close(find_folder_stream);

    context->use &= ~CNTXT_PARTFIND;	/* unset partial find bit */
    if(we_cancel)
      cancel_busy_alarm(-1);
}



/*
 * free_folders_in_context - loop thru the context's lists of folders
 *                     clearing all entries without nicknames
 *                     (as those were user provided) AND reset the 
 *                     context's find flag.
 *
 * NOTE: if fetched() information (e.g., like message counts come back
 *       in bboard collections), we may want to have a check before
 *       executing the loop and setting the FIND flag.
 */
void
free_folders_in_context(cntxt)
    CONTEXT_S *cntxt;
{
    int n, i;

    /*
     * In this case, don't blast the list as it was given to us by the
     * user and not the result of a mail_find* call...
     */
    if(cntxt->use & CNTXT_INCMNG)
      return;

    for(n = folder_total(cntxt->folders), i = 0; n > 0; n--)
      if(folder_entry(i, cntxt->folders)->nickname)
	i++;				/* entry wasn't a FIND result */
      else
	folder_delete(i, cntxt->folders);

    cntxt->use |= CNTXT_NOFIND;		/* do find next time...  */
    cntxt->use &= ~CNTXT_PSEUDO;	/* or add the fake entry */
}


/*
 * default_save_context - return the default context for saved messages
 */
CONTEXT_S *
default_save_context(cntxt)
    CONTEXT_S *cntxt;
{
    while(cntxt)
      if((cntxt->use) & CNTXT_SAVEDFLT)
	return(cntxt);
      else
	cntxt = cntxt->next;

    return(NULL);
}



/*----------------------------------------------------------------------
    Update the folder display structure for the number of unread
 messages for a news group
 
Args: stream   -- open mail stream to count unread messages on
      f_struct -- pointer to the structure to update

This is called when going into the folders screen or when closing a
news group to update the string that is displayed showing the number
of unread messages. The stream and f_struct passed in better be for
the same folder or things will get a little confused. 

This can also be a little slow because count_flagged() can be slow
due to current performance of the news driver.
 ----*/
void
update_news_prefix(stream, f_struct)
     MAILSTREAM *stream;
     struct folder *f_struct;
{
   int n;

   n = count_flagged(stream, "UNSEEN");
   sprintf(f_struct->prefix, "%4.4s ",  n ? int2string(n)  : "");
}



/*
 * folder_complete - foldername completion routine
 *              returns:
 *
 *   Result: returns 0 if the folder doesn't have a unique completetion
 *                   1 if so, and replaces name with completion
 */
folder_complete(context, name)
    CONTEXT_S *context;
    char      *name;
{
    int  i, match = -1, did_find;
    char tmp[MAXFOLDER], tmpname[MAXFOLDER], *a, *b; 
    FOLDER_S *f;
#if defined(DOS) || defined(OS2)
#define	STRUCMP	struncmp 		/* ignore case under DOS */
#else
#define	STRUCMP	strncmp
#endif
    
    if(*name == '\0' || !context_isambig(name))
      return(0);

    if(did_find = !ALL_FOUND(context)){
	sprintf(tmpname, "%s*", name);
	find_folders_in_context(NULL, context, tmpname);
    }
    else if(context->use & CNTXT_PSEUDO)
      return(0);			/* no folders to complete */

    *tmp = '\0';			/* find uniq substring */
    for(i = 0; i < folder_total(context->folders); i++){
	f = folder_entry(i, context->folders);
	if(STRUCMP(name, FLDR_NAME(f), strlen(name)) == 0){
	    if(match != -1){		/* oh well, do best we can... */
		a = FLDR_NAME(f);
		if(match >= 0){
		    f = folder_entry(match, context->folders);
		    strcpy(tmp, FLDR_NAME(f));
		}

		match = -2;
		b     = tmp;		/* remember largest common text */
		while(*a && *b && *a == *b)
		  *b++ = *a++;

		*b = '\0';
	    }
	    else		
	      match = i;		/* bingo?? */
	}
    }

    if(match >= 0){			/* found! */
	f = folder_entry(match, context->folders);
	strcpy(name, FLDR_NAME(f));
    }
    else if(match == -2)		/* closest we could find */
      strcpy(name, tmp);

    if(did_find){
	if(context->use & CNTXT_PSEUDO){
	    while(folder_total(context->folders) > 1)
	      folder_delete(strcmp(folder_entry(0, context->folders)->name,
				   CLICKHERE) ? 0 : 1,
			    context->folders);
	}
	else
	  free_folders_in_context(context);
    }

    return((match >= 0) ? 1 : 0);
}


/*
 *           FOLDER MANAGEMENT ROUTINES
 */


/*
 * Folder List Structure - provides for two ways to access and manage
 *                         folder list data.  One as an array of pointers
 *                         to folder structs or
 */
typedef struct folder_list {
    unsigned   used;
    unsigned   allocated;
#ifdef	DOSXXX
    FILE      *folders;		/* tmpfile of binary */
#else
    FOLDER_S **folders;		/* array of pointers to folder structs */
#endif
} FLIST;

#define FCHUNK  64


/*
 * new_folder_list - return a piece of memory suitable for attaching 
 *                   a list of folders...
 *
 */
void *
new_folder_list()
{
    FLIST *flist = (FLIST *) fs_get(sizeof(FLIST));
    flist->folders = (FOLDER_S **) fs_get(FCHUNK * sizeof(FOLDER_S *));
    memset((void *)flist->folders, 0, (FCHUNK * sizeof(FOLDER_S *)));
    flist->allocated = FCHUNK;
    flist->used      = 0;
    return((void *)flist);
}



/*
 * new_folder - return a brand new folder entry, with the given name
 *              filled in.
 *
 * NOTE: THIS IS THE ONLY WAY TO PUT A NAME INTO A FOLDER ENTRY!!!
 * STRCPY WILL NOT WORK!!!
 */
FOLDER_S *
new_folder(name)
    char *name;
{
#ifdef	DOSXXX
#else
    FOLDER_S *tmp;
    size_t    l = strlen(name);

    tmp = (FOLDER_S *)fs_get(sizeof(FOLDER_S) + (l * sizeof(char)));
    memset((void *)tmp, 0, sizeof(FOLDER_S));
    strcpy(tmp->name, name);
    tmp->name_len = (unsigned char) l;
    return(tmp);
#endif
}



/*
 * folder_entry - folder struct access routine.  Permit reference to
 *                folder structs via index number. Serves two purposes:
 *            1) easy way for callers to access folder data 
 *               conveniently
 *            2) allows for a custom manager to limit memory use
 *               under certain rather limited "operating systems"
 *               who shall renameless, but whose initials are DOS
 *
 *
 */
FOLDER_S *
folder_entry(i, flist)
    int   i;
    void *flist;
{
#ifdef	DOSXXX
    /*
     * Manage entries within a limited amount of core (what a drag).
     */

    fseek((FLIST *)flist->folders, i * sizeof(FOLDER_S) + MAXPATH, 0);
    fread(&folder, sizeof(FOLDER_S) + MAXPATH, (FLIST *)flist->folders);
#else
    return((i >= ((FLIST *)flist)->used) ? NULL:((FLIST *)flist)->folders[i]);
#endif
}



/*
 * free_folder_list - release all resources associated with the given 
 *                    folder list
 */
void
free_folder_list(flist)
    void **flist;
{
#ifdef	DOSXXX
    fclose((*((FLIST **)flist))->folders); 	/* close folder tmpfile */
#else
    register int i = (*((FLIST **)flist))->used;

    while(i--){
	if((*((FLIST **)flist))->folders[i]->nickname)
	  fs_give((void **)&(*((FLIST **)flist))->folders[i]->nickname);

	fs_give((void **)&((*((FLIST **)flist))->folders[i]));
    }

    fs_give((void **)&((*((FLIST **)flist))->folders));
#endif
    fs_give(flist);
}



/*
 * return the number of folders associated with the given folder list
 */
int
folder_total(flist)
    void *flist;
{
    return((int)((FLIST *)flist)->used);
}


/*
 * return the index number of the given name in the given folder list
 */
int
folder_index(name, flist)
    char *name;
    void *flist;
{
    register  int i = 0;
    FOLDER_S *f;
    char     *fname;

    while(f = folder_entry(i, flist)){
	fname = FLDR_NAME(f);
#if defined(DOS) || defined(OS2)
	if(toupper((unsigned char)(*name))
	    == toupper((unsigned char)(*fname)) && strucmp(name, fname) == 0)
#else
	if(*name == *fname && strcmp(name, fname) == 0)
#endif
	  return(i);
	else
	  i++;
    }

    return(-1);
}



/*
 * next_folder - given a current folder in a context, return the next in
 *               the list, or NULL if no more or there's a problem.
 */
char *
next_folder(stream, next, current, cntxt, find_recent)
    MAILSTREAM **stream;
    char	*current, *next;
    CONTEXT_S	*cntxt;
    int		 find_recent;
{
    int       index, are_recent = 0;
    FOLDER_S *f = NULL;

    /* note: find_folders may assign "stream" */
    find_folders_in_context(stream, cntxt, NULL);
    for(index = folder_index(current, cntxt->folders) + 1;
	index > 0
	&& index < folder_total(cntxt->folders)
	&& (f = folder_entry(index, cntxt->folders));
	index++)
      if(find_recent){
	  /* if we can't use this stream, close it up */
	  if(stream && *stream
	     && !context_same_stream(cntxt->context, f->name, *stream)){
	      mail_close(*stream);
	      *stream = NULL;
	  }

	  if(are_recent = folder_has_recent(stream, cntxt,f))
	    break;
      }

    if(f && (!find_recent || are_recent))
      strcpy(next, FLDR_NAME(f));
    else
      *next = '\0';

    /* BUG: how can this be made smarter so we cache the list? */
    free_folders_in_context(cntxt);
    return((*next) ? next : NULL);
}



/*
 * folder_is_nick - check to see if the given name is a nickname
 *                  for some folder in the given context...
 *
 *  NOTE: no need to check if find_folder_names has been done as 
 *        nicknames can only be set by configuration...
 */
char *
folder_is_nick(nickname, flist)
    char *nickname;
    void *flist;
{
    register  int  i = 0;
    FOLDER_S *f;

    while(f = folder_entry(i, flist)){
	if(f->nickname && STRCMP(nickname, f->nickname) == 0)
	  return(f->name);
	else
	  i++;
    }

    return(NULL);
}



/*----------------------------------------------------------------------
  Insert the given folder name into the sorted folder list
  associated with the given context.  Only allow ambiguous folder
  names IF associated with a nickname.

   Args: index  -- Index to insert at, OR insert in sorted order if -1
         folder -- folder structure to insert into list
	 flist  -- folder list to insert folder into

  **** WARNING ****
  DON'T count on the folder pointer being valid after this returns
  *** ALL FOLDER ELEMENT READS SHOULD BE THRU folder_entry() ***

  ----*/
int
folder_insert(index, folder, flist)
    int       index;
    FOLDER_S *folder;
    void     *flist;
{
    int i;

    if(index < 0){			/* add to sorted list */
	char     *sortname, *fsortname;
	FOLDER_S *f;

	sortname = FLDR_NAME(folder);
	index    = 0;
	while(f = folder_entry(index, flist)){
	    fsortname = FLDR_NAME(f);
	    if((i = compare_names(&sortname, &fsortname)) < 0)
	      break;
	    else if(i == 0)		/* same folder? just return index */
	      return(index);
	    else
	      index++;
	}
    }

    folder_insert_index(folder, index, flist);
    return(index);
}


/* 
 * folder_insert_index - Insert the given folder struct into the global list
 *                 at the given index.
 */
void
folder_insert_index(folder, index, flist)
    FOLDER_S *folder;
    int       index;
    void     *flist;
{
#ifdef	DOSXXX
    FOLDER *tmp;

    tmp = (FOLDER_S *)fs_get(sizeof(FOLDER_S) + (MAXFOLDER * sizeof(char)));


#else
    register FOLDER_S **flp, **iflp;

    /* if index is beyond size, place at end of list */
    index = min(index, ((FLIST *)flist)->used);

    /* grow array ? */
    if(((FLIST *)flist)->used + 1 > ((FLIST *)flist)->allocated){
	((FLIST *)flist)->allocated += FCHUNK;
	fs_resize((void **)&(((FLIST *)flist)->folders),
		  (((FLIST *)flist)->allocated) * sizeof(FOLDER_S *));
    }

    /* shift array left */
    iflp = &(((FOLDER_S **)((FLIST *)flist)->folders)[index]);
    for(flp = &(((FOLDER_S **)((FLIST *)flist)->folders)[((FLIST *)flist)->used]); 
	flp > iflp; flp--)
      flp[0] = flp[-1];

    ((FLIST *)flist)->folders[index] = folder;
    ((FLIST *)flist)->used          += 1;
#endif
}


/*----------------------------------------------------------------------
    Removes a folder at the given index in the given context's
    list.

Args: index -- Index in folder list of folder to be removed
      flist -- folder list
 ----*/
void
folder_delete(index, flist)
    int   index;
    void *flist;
{
    register int  i;
    FOLDER_S     *f;

    if(((FLIST *)flist)->used 
       && (index < 0 || index >= ((FLIST *)flist)->used))
      return;				/* bogus call! */

    if((f = folder_entry(index, flist))->nickname)
      fs_give((void **)&(f->nickname));
      
#ifdef	DOSXXX
    /* shift all entries after index up one.... */
#else
    fs_give((void **)&(((FLIST *)flist)->folders[index]));
    for(i = index; i < ((FLIST *)flist)->used - 1; i++)
      ((FLIST *)flist)->folders[i] = ((FLIST *)flist)->folders[i+1];


    ((FLIST *)flist)->used -= 1;
#endif
}



/*----------------------------------------------------------------------
    Find an entry in the folder list by matching names
  ----*/
search_folder_list(list, name)
     void *list;
     char *name;
{
    int i;
    char *n;

    for(i = 0; i < folder_total(list); i++) {
        n = folder_entry(i, list)->name;
        if(strucmp(name, n) == 0)
          return(1); /* Found it */
    }
    return(0);
}



/*----------------------------------------------------------------------
   Sort the given folder list
  ----*/
sort_folder_list(list, sort)
     void  *list;
     QSFunc sort;
{
    qsort((QSType *)(((FLIST *)list)->folders),
          (size_t) folder_total(list),
          sizeof(FOLDER_S *),
          sort);
}
    





static CONTEXT_S *post_cntxt = NULL;

/*----------------------------------------------------------------------
    Verify and canonicalize news groups names. 
    Called from the message composer

Args:  given_group    -- List of groups typed by user
       expanded_group -- pointer to point to expanded list, which will be
			 allocated here and freed in caller.  If this is
			 NULL, don't attempt to validate.
       error          -- pointer to store error message
       fcc            -- pointer to point to fcc, which will be
			 allocated here and freed in caller

Returns:  0 if all is OK
         -1 if addresses weren't valid

Test the given list of newstroups against those recognized by our nntp
servers.  Testing by actually trying to open the list is much cheaper, both
in bandwidth and memory, than yanking the whole list across the wire.
  ----*/
int
news_build(given_group, expanded_group, error, fcc)
    char	 *given_group,
		**expanded_group,
		**error;
    BUILDER_ARG	 *fcc;
{
    char	 ng_error[90], *p1, *p2, *name, *end, *ep, **server;
      /* I've got no idea where 90 comes from */
    int          expanded_len = 0, num_in_error = 0, cnt_errs, we_cancel = 0;
    MAILSTREAM  *stream = NULL;
    struct ng_list {
	char  *groupname;
	NgCacheReturns  found;
	struct ng_list *next;
    }*nglist = NULL, **ntmpp, *ntmp;
#ifdef SENDNEWS
    static int no_servers = 0;
#endif


    dprint(2, (debugfile,
	"- news_build - (%s)\n", given_group ? given_group : "nul"));

    if(error)
      *error = NULL;

    /*------ parse given entries into a list ----*/
    ntmpp = &nglist;
    for(name = given_group; *name; name = end){

	/* find start of next group name */
        while(*name && (isspace((unsigned char)*name) || *name == ','))
	  name++;

	/* find end of group name */
	end = name;
	while(*end && !isspace((unsigned char)*end) && *end != ',')
	  end++;

        if(end != name){
	    *ntmpp = (struct ng_list *)fs_get(sizeof(struct ng_list));
	    (*ntmpp)->next      = NULL;
	    (*ntmpp)->found     = NotChecked;
            (*ntmpp)->groupname = fs_get(end - name + 1);
            strncpy((*ntmpp)->groupname, name, end - name);
            (*ntmpp)->groupname[end - name] = '\0';
	    ntmpp = &(*ntmpp)->next;
	    if(!expanded_group)
	      break;  /* no need to continue if just doing fcc */
        }
    }

    /*
     * If fcc is not set or is set to default, then replace it if
     * one of the recipient rules is in effect.
     */
    if(fcc){
	if((ps_global->fcc_rule == FCC_RULE_RECIP ||
	    ps_global->fcc_rule == FCC_RULE_NICK_RECIP) &&
	       (nglist && nglist->groupname)){
	  if(fcc->tptr)
	    fs_give((void **)&fcc->tptr);

	  fcc->tptr = cpystr(nglist->groupname);
	}
	else if(!fcc->tptr) /* already default otherwise */
	  fcc->tptr = cpystr(ps_global->VAR_DEFAULT_FCC);
    }

    if(!nglist){
	if(expanded_group)
	  *expanded_group = cpystr("");
        return 0;
    }

    if(!expanded_group)
      return 0;

#ifdef	DEBUG
    for(ntmp = nglist; debug >= 9 && ntmp; ntmp = ntmp->next)
      dprint(9, (debugfile, "Parsed group: --[%s]--\n", ntmp->groupname));
#endif

    /* If we are doing validation */
    if(F_OFF(F_NO_NEWS_VALIDATION, ps_global)){
	int need_to_talk_to_server = 0;

	/*
	 * First check our cache of validated newsgroups to see if we even
	 * have to open a stream.
	 */
	for(ntmp = nglist; ntmp; ntmp = ntmp->next){
	    ntmp->found = chk_newsgrp_cache(ntmp->groupname);
	    if(ntmp->found == NotInCache)
	      need_to_talk_to_server++;
	}

	if(need_to_talk_to_server){

#ifdef SENDNEWS
	  if(no_servers == 0)
#endif
	    we_cancel = busy_alarm(1, "Validating newsgroup(s)", NULL, 1);

	    /*
	     * Build a stream to the first server that'll talk to us...
	     */
	    for(server = ps_global->VAR_NNTP_SERVER;
		server && *server && **server;
		server++){
		char name[MAXPATH];

		sprintf(name, "*{%s/nntp}", *server);
		if(stream = mail_open(stream, name, OP_HALFOPEN))
		  break;
	    }

	    if(!server || !stream){
		if(error)
#ifdef SENDNEWS
		{
		 /* don't say this over and over */
		 if(no_servers == 0){
		    if(!server || !*server || !**server)
		      no_servers++;

		    *error = cpystr(no_servers
			    ? "Can't validate groups.  No servers defined"
			    : "Can't validate groups.  No servers responding");
		 }
		}
#else
		  *error = cpystr((!server || !*server || !**server)
			    ? "No servers defined for posting to newsgroups"
			    : "Can't validate groups.  No servers responding");
#endif
		*expanded_group = cpystr(given_group);
		goto done;
	    }
	}

	/*
	 * Now, go thru the list, making sure we can at least open each one...
	 */
	for(ntmp = nglist; ntmp; ntmp = ntmp->next){
	    /*
	     * It's faster and easier right now just to open the stream and
	     * do our own finds than to use the current folder_exists()
	     * interface...
	     */
	    if(ntmp->found == NotInCache){
		find_folder_list  = NULL;
		find_folder_count = 0L;
		context_find_all_bboard(NULL, stream, ntmp->groupname);
		ntmp->found = (find_folder_count > 0L) ? Found : Missing;
	    }
	    add_newsgrp_cache(ntmp->groupname, ntmp->found);
	}

	mail_close(stream);
    }

    /* figure length of string for matching groups */
    for(ntmp = nglist; ntmp; ntmp = ntmp->next){
      if(ntmp->found == Found || F_ON(F_NO_NEWS_VALIDATION, ps_global))
	expanded_len += strlen(ntmp->groupname) + 2;
      else
	num_in_error++;
    }

    /*
     * allocate and write the allowed, and error lists...
     */
    p1 = *expanded_group = fs_get((expanded_len + 1) * sizeof(char));
    if(error && num_in_error){
	cnt_errs = num_in_error;
	memset((void *)ng_error, 0, (size_t)90);
	sprintf(ng_error, "Unknown news group%s: ", plural(num_in_error));
	ep = ng_error + strlen(ng_error);
    }
    for(ntmp = nglist; ntmp; ntmp = ntmp->next){
	p2 = ntmp->groupname;
	if(ntmp->found == Found || F_ON(F_NO_NEWS_VALIDATION, ps_global)){
	    while(*p2)
	      *p1++ = *p2++;

	    if(ntmp->next){
		*p1++ = ',';
		*p1++ = ' ';
	    }
	}
	else if (error){
	    while(*p2 && (ep - ng_error < 89))
	      *ep++ = *p2++;

	    if(--cnt_errs > 0 && (ep - ng_error < 87)){
		strcpy(ep, ", ");
		ep += 2;
	    }
	}
    }

    *p1 = '\0';

    if(error && num_in_error)
      *error = cpystr(ng_error);

done:
    while(ntmp = nglist){
	nglist = nglist->next;
	fs_give((void **)&ntmp->groupname);
	fs_give((void **)&ntmp);
    }

    if(we_cancel)
      cancel_busy_alarm(0);

    return(num_in_error ? -1 : 0);
}


typedef struct ng_cache {
    char          *name;
    NgCacheReturns val;
}NgCache;
static NgCache *ng_cache_ptr;
#if defined(DOS) && !defined(_WINDOWS)
#define MAX_NGCACHE_ENTRIES 15
#else
#define MAX_NGCACHE_ENTRIES 40
#endif
/*
 * Simple newsgroup validity cache.  Opening a newsgroup to see if it
 * exists can be very slow on a heavily loaded NNTP server, so we cache
 * the results.
 */
NgCacheReturns
chk_newsgrp_cache(group)
char *group;
{
    register NgCache *ngp;
    
    for(ngp = ng_cache_ptr; ngp && ngp->name; ngp++){
	if(strcmp(group, ngp->name) == 0)
	  return(ngp->val);
    }

    return NotInCache;
}


/*
 * Add an entry to the newsgroup validity cache.
 *
 * LRU entry is the one on the bottom, oldest on the top.
 * A slot has an entry in it if name is not NULL.
 */
void
add_newsgrp_cache(group, result)
char *group;
NgCacheReturns result;
{
    register NgCache *ngp;
    NgCache save_ngp;

    /* first call, initialize cache */
    if(!ng_cache_ptr){
	int i;

	ng_cache_ptr =
	    (NgCache *)fs_get((MAX_NGCACHE_ENTRIES+1)*sizeof(NgCache));
	for(i = 0; i <= MAX_NGCACHE_ENTRIES; i++){
	    ng_cache_ptr[i].name = NULL;
	    ng_cache_ptr[i].val  = NotInCache;
	}
	ng_cache_ptr[MAX_NGCACHE_ENTRIES].val  = End;
    }

    if(chk_newsgrp_cache(group) == NotInCache){
	/* find first empty slot or End */
	for(ngp = ng_cache_ptr; ngp->name; ngp++)
	  ;/* do nothing */
	if(ngp->val == End){
	    /*
	     * Cache is full, throw away top entry, move everything up,
	     * and put new entry on the bottom.
	     */
	    ngp = ng_cache_ptr;
	    if(ngp->name) /* just making sure */
	      fs_give((void **)&ngp->name);

	    for(; (ngp+1)->name; ngp++){
		ngp->name = (ngp+1)->name;
		ngp->val  = (ngp+1)->val;
	    }
	}
	ngp->name = cpystr(group);
	ngp->val  = result;
    }
    else{
	/*
	 * Move this entry from current location to last to preserve
	 * LRU order.
	 */
	for(ngp = ng_cache_ptr; ngp && ngp->name; ngp++){
	    if(strcmp(group, ngp->name) == 0) /* found it */
	      break;
	}
	save_ngp.name = ngp->name;
	save_ngp.val  = ngp->val;
	for(; (ngp+1)->name; ngp++){
	    ngp->name = (ngp+1)->name;
	    ngp->val  = (ngp+1)->val;
	}
	ngp->name = save_ngp.name;
	ngp->val  = save_ngp.val;
    }
}


void
free_newsgrp_cache()
{
    register NgCache *ngp;

    for(ngp = ng_cache_ptr; ngp && ngp->name; ngp++)
      fs_give((void **)&ngp->name);
    if(ng_cache_ptr)
      fs_give((void **)&ng_cache_ptr);
}


/*----------------------------------------------------------------------
    Browse list of newsgroups available for posting

  Called from composer when ^T is typed in newsgroups field

Args:    none

Returns: pointer to selected newsgroup, or NULL.
         Selector call in composer expects this to be alloc'd here.

  ----*/
char *
news_group_selector(error_mess)
    char **error_mess;
{
    FSTATE_S  post_state, *push_state;
    char     *post_folder;
    int       rc;
    char     *em;

    /* Coming back from composer */
    fix_windsize(ps_global);
    init_sigwinch();

    post_folder = fs_get((size_t)500);

    /*--- build the post_cntxt -----*/
    em = get_post_list(ps_global->VAR_NNTP_SERVER);
    if(em != NULL){
        if(error_mess != NULL)
          *error_mess = em;

	cancel_busy_alarm(-1);
        return(NULL);
    }

    /*----- Call the browser -------*/
    push_state = fs;
    rc = folder_lister(ps_global, PostNews, post_cntxt, NULL,
                       post_folder, NULL, post_cntxt, &post_state);
    fs = push_state;

    cancel_busy_alarm(-1);

    if(rc <= 0)
      return(NULL);

    return(post_folder);
}



/*----------------------------------------------------------------------
    Get the list of news groups that are possible for posting

Args: post_host -- host name for posting

Returns NULL if list is retrieved, pointer to error message if failed

This is kept in a standards "CONTEXT" for a acouple of reasons. First
it makes it very easy to use the folder browser to display the
newsgroup for selection on ^T from the composer. Second it will allow
the same mechanism to be used for all folder lists on memory tight
systems like DOS. The list is kept for the life of the session because
fetching it is a expensive. 

 ----*/
char *
get_post_list(post_host)
     char **post_host;
{
    char *post_context_string;

    if(!post_host || !post_host[0]) {
        /* BUG should assume inews and get this from active file */
        return("Can't post messages, NNTP server needs to be configured");
    }

    if(!post_cntxt){
	int we_cancel;

	we_cancel
	    = busy_alarm(1, "Getting full list of groups for posting", NULL, 0);

        post_context_string = fs_get(strlen(post_host[0]) + 20);
        sprintf(post_context_string, "*{%s/nntp}[]", post_host[0]);
        
        post_cntxt          = new_context(post_context_string);

        post_cntxt->use    |= CNTXT_FINDALL | CNTXT_NOFIND; 
        post_cntxt->next    = NULL;

        find_folders_in_context(NULL, post_cntxt, NULL);
	if(we_cancel)
	  cancel_busy_alarm(-1);
    }
    return(NULL);
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
folder_scroll_callback (cmd, scroll_pos)
int	cmd;
long	scroll_pos;
{
    static short bad_timing = 0;
    int    rv;

    if(bad_timing)
      return(FALSE);
    else
      bad_timing = TRUE;

    switch (cmd) {
      case MSWIN_KEY_SCROLLUPLINE:
	rv = folder_scroll_down(1L);
	break;

      case MSWIN_KEY_SCROLLDOWNLINE:
	rv = folder_scroll_up(1L);
	break;

      case MSWIN_KEY_SCROLLUPPAGE:
	rv = folder_scroll_down(fs->display_rows);
	break;

      case MSWIN_KEY_SCROLLDOWNPAGE:
	rv = folder_scroll_up(fs->display_rows);
	break;

      case MSWIN_KEY_SCROLLTO:
	rv =  folder_scroll_to_pos(scroll_pos);
	break;
    }

    if(rv)
      display_folder(fs, fs->context, fs->folder_index, NULL, -1);

    bad_timing = FALSE;
    return(rv);
}
#endif	/* _WINDOWS */
