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
    mailindx.c
    Implements the mail index screen
     - most code here builds the header list and displays it

 ====*/
 
#include "headers.h"


static struct key index_keys[] = 
       {{"?","Help",KS_SCREENHELP},	{"O","OTHER CMDS",KS_NONE},
	{"M","Main Menu",KS_MAINMENU},	{"V","[ViewMsg]",KS_VIEW},
	{"P","PrevMsg",KS_PREVMSG},	{"N","NextMsg",KS_NEXTMSG},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{"D","Delete",KS_DELETE},	{"U","Undelete",KS_UNDELETE},
	{"R","Reply",KS_REPLY},		{"F","Forward",KS_FORWARD},

	{"?","Help",KS_SCREENHELP},	{"O","OTHER CMDS",KS_NONE},
	{"Q","Quit",KS_EXIT},		{"C","Compose",KS_COMPOSER},
	{"L","ListFldrs",KS_FLDRLIST},	{"G","GotoFldr",KS_GOTOFLDR},
	{"Tab","NextNew",KS_NONE},	{"W","WhereIs",KS_WHEREIS},
	{"Y","prYnt",KS_PRINT},		{"T","TakeAddr",KS_TAKEADDR},
	{"S","Save",KS_SAVE},		{"E","Export",KS_EXPORT},

	{"?","Help",KS_SCREENHELP},	{"O","OTHER CMDS",KS_NONE},
	{"X",NULL,KS_NONE},		{"&","unXclude",KS_NONE},
	{";","Select",KS_SELECT},	{"A","Apply",KS_APPLY},
	{"$","SortIndex",KS_SORT},	{"J","Jump",KS_JUMPTOMSG},
	{"H","HdrMode",KS_HDRMODE},	{"B","Bounce",KS_BOUNCE},
	{"*","Flag",KS_FLAG},		{"|","Pipe",KS_NONE},

	{"?","Help",KS_SCREENHELP},	{"O","OTHER CMDS",KS_NONE},
	{":","SelectCur",KS_SELECTCUR},	{"Z","ZoomMode",KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE}};
INST_KEY_MENU(index_keymenu, index_keys);
#define PREVM_KEY 4
#define NEXTM_KEY 5
#define EXCLUDE_KEY 26
#define UNEXCLUDE_KEY 27
#define SELECT_KEY 28
#define APPLY_KEY 29
#define VIEW_FULL_HEADERS_KEY 32
#define BOUNCE_KEY 33
#define FLAG_KEY 34
#define VIEW_PIPE_KEY 35
#define ZOOM_KEY 39

static struct key nr_anon_index_keys[] = 
       {{"?","Help",KS_SCREENHELP},	{"W","WhereIs",KS_WHEREIS},
	{"Q","Quit",KS_EXIT},		{"V","[ViewMsg]",KS_VIEW},
	{"P","PrevMsg",KS_PREVMSG},	{"N","NextMsg",KS_NEXTMSG},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{"F","Fwd Email",KS_FORWARD},	{"J","Jump",KS_JUMPTOMSG},
	{"$","SortIndex",KS_SORT},	{NULL,NULL,KS_NONE}};
INST_KEY_MENU(nr_anon_index_keymenu, nr_anon_index_keys);

static struct key nr_index_keys[] = 
       {{"?","Help",KS_SCREENHELP},	{"O","OTHER CMDS",KS_NONE},
	{"Q","Quit",KS_EXIT},		{"V","[ViewMsg]",KS_VIEW},
	{"P","PrevMsg",KS_PREVMSG},	{"N","NextMsg",KS_NEXTMSG},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{"F","Fwd Email",KS_FORWARD},	{"J","Jump",KS_JUMPTOMSG},
	{"Y","prYnt",KS_PRINT},		{"S","Save",KS_SAVE},

	{"?","Help",KS_SCREENHELP},	{"O","OTHER CMDS",KS_NONE},
	{"E","Export",KS_EXPORT},	{"C","Compose",KS_COMPOSER},
	{"$","SortIndex",KS_SORT},	{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},		{"W","WhereIs",KS_WHEREIS},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE}};
INST_KEY_MENU(nr_index_keymenu, nr_index_keys);
  
static struct key simple_index_keys[] = 
       {{"?","Help",KS_SCREENHELP},	{NULL,NULL,KS_NONE},
	{"E","ExitSelect",KS_EXITMODE},	{"S","[Select]",KS_SELECT},
	{"P","PrevMsg",KS_PREVMSG},	{"N","NextMsg",KS_NEXTMSG},
	{"-","PrevPage",KS_PREVPAGE},	{"Spc","NextPage",KS_NEXTPAGE},
	{"D","Delete",KS_DELETE},	{"U","Undelete",KS_UNDELETE},
	{NULL,NULL,KS_NONE},		{NULL,NULL,KS_NONE}};
INST_KEY_MENU(simple_index_keymenu, simple_index_keys);


static OtherMenu what_keymenu = FirstMenu;
jmp_buf jump_past_qsort;


/*-----------
  Saved state to redraw message index body 
  ----*/
struct entry_state {
    unsigned hilite:1;
    unsigned bolded:1;
    long     id;
};


#define MAXIFLDS 20  /* max number of fields in index format */
static struct index_state {
    long        msg_at_top,
	        lines_per_page;
    struct      entry_state *entry_state;
    MSGNO_S    *msgmap;
    MAILSTREAM *stream;
    int         status_col;		/* column for select X's */
} *current_index_state = NULL;

static int get_body_for_index;


/*
 * cache used by get_sub() so it doesn't have to do a fetchstructure
 * and malloc/free for every qsort comparison...
 */
#if	defined(DOS) && !defined(_WINDOWS)
#define	SUB_CACHE_LEN	128
typedef struct {
    short used;				/* whether entry's been used or not */
    char  buf[SUB_CACHE_LEN];		/* actual subject string */
} scache_ent;


static struct scache {
    short	last;			/* last one referenced */
    long	size,			/* number of elements */
		msgno[2];		/* which message in which entry */
    scache_ent  ent[2];			/* subject and cache info for msg */
    char       *cname;			/* file containing cache */
    FILE       *cfile;
} *scache = NULL;
#else
static struct scache {
    long   size;
    char **ent;
} *scache = NULL;
#endif


/*
 * Internal prototypes
 */
int	 update_index PROTO((struct pine *, struct index_state *));
int	 index_scroll_up PROTO((long));
int	 index_scroll_down PROTO((long));
int	 index_scroll_to_pos PROTO((long));
long     top_ent_calc PROTO((MAILSTREAM *, MSGNO_S *, long, long));
char    *get_sub PROTO((long));
int      compare_subjects PROTO((const QSType *, const QSType *));
int      compare_subject_2 PROTO((const QSType *, const QSType *));
int      compare_from PROTO((const QSType *, const QSType *));
int      compare_to PROTO((const QSType *, const QSType *));
int      compare_cc PROTO((const QSType *, const QSType *));
int      compare_message_dates PROTO((const QSType *, const QSType *));
int      compare_size PROTO((const QSType *, const QSType *));
int      compare_arrival PROTO((const QSType *, const QSType *));
HLINE_S *get_index_cache PROTO((long));
int	 set_index_addr PROTO((MAILSTREAM *, long, char *, ADDRESS *, char *,
			       int, char *));
int      i_cache_size PROTO((long));
int      i_cache_width PROTO(());
void	 setup_header_widths PROTO((INDEX_COL_S *, int, long));
int	 parse_index_format PROTO((char *, INDEX_COL_S **));
void	 set_need_format_setup();
void	 clear_need_format_setup();
int	 check_need_format_setup();
void	 msgno_flush_selected PROTO((MSGNO_S *, long));
void	 second_subject_sort PROTO((long *, long, long));
void	 init_subject_cache PROTO((long));
int	 subject_cache_slot PROTO((long));
char	*subject_cache_ent PROTO((long));
char	*subject_cache_add PROTO((long, char *));
void	 clear_subject_cache PROTO(());
#ifdef	DOS
void     i_cache_hit PROTO((long));
void     icread PROTO((void));
void	 icwrite PROTO((void));
#ifdef	_WINDOWS
int	 index_scroll_callback PROTO((int,long));
int	 index_gettext_callback PROTO((char *, void **, long *, int *));
#endif
#endif
long	 seconds_since_epoch PROTO((long));




/*----------------------------------------------------------------------


  ----*/
void
do_index_border(cntxt, folder, stream, msgmap, style, which_keys, flags)
     CONTEXT_S   *cntxt;
     char        *folder;
     MAILSTREAM  *stream;
     MSGNO_S     *msgmap;
     IndexType    style;
     int         *which_keys, flags;
{
    if(flags & INDX_CLEAR)
      ClearScreen();

    if(flags & INDX_HEADER)
      set_titlebar((stream == ps_global->mail_stream)
		     ? (style == MsgIndex || style == MultiMsgIndex)
		         ? "FOLDER INDEX"
			 : "ZOOMED FOLDER INDEX"
		     : (!strcmp(folder, INTERRUPTED_MAIL))
			 ? "COMPOSE: SELECT INTERRUPTED"
			 : "COMPOSE: SELECT POSTPONED",
		   stream, cntxt, folder, msgmap, 1, MessageNumber, 0, 0);

    if(flags & INDX_FOOTER) {
	struct key_menu *km;
	bitmap_t	 bitmap;

	setbitmap(bitmap);
	if(ps_global->anonymous)
	  km = &nr_anon_index_keymenu;
	else if(ps_global->nr_mode)
          km = &nr_index_keymenu;
	else if(ps_global->mail_stream != stream)
	  km = &simple_index_keymenu;
	else{
	    km = &index_keymenu;

#ifndef DOS
	    if(F_OFF(F_ENABLE_PIPE,ps_global))
#endif
	      clrbitn(VIEW_PIPE_KEY, bitmap);  /* always clear for DOS */
	    if(F_OFF(F_ENABLE_FULL_HDR,ps_global))
	      clrbitn(VIEW_FULL_HEADERS_KEY, bitmap);
	    if(F_OFF(F_ENABLE_BOUNCE,ps_global))
	      clrbitn(BOUNCE_KEY, bitmap);
	    if(F_OFF(F_ENABLE_FLAG,ps_global))
	      clrbitn(FLAG_KEY, bitmap);
	    if(F_OFF(F_ENABLE_AGG_OPS,ps_global)){
		clrbitn(SELECT_KEY, bitmap);
		clrbitn(APPLY_KEY, bitmap);

		/*
		 * Since "Zoom" is alone on the last keymenu page
		 * if it's not enabled, hide that menu altogether
		 */
		if(style != ZoomIndex){
		    clrbitn(ZOOM_KEY, bitmap);
		    km->how_many = 3;
		}
		else
		  km->how_many = 4;
	    }
	    else
	      km->how_many = 4;

	    if(IS_NEWS(stream)){
		index_keys[EXCLUDE_KEY].label = "eXclude";
		KS_OSDATASET(&index_keys[EXCLUDE_KEY], KS_NONE);
	    }
	    else {
		clrbitn(UNEXCLUDE_KEY, bitmap);
		index_keys[EXCLUDE_KEY].label = "eXpunge";
		KS_OSDATASET(&index_keys[EXCLUDE_KEY], KS_EXPUNGE);
	    }

	    if(style == MultiMsgIndex){
		clrbitn(PREVM_KEY, bitmap);
		clrbitn(NEXTM_KEY, bitmap);
	    }
	}
        draw_keymenu(km, bitmap, ps_global->ttyo->screen_cols,
			   1-FOOTER_ROWS(ps_global), 0, what_keymenu, 0);
	what_keymenu = SameTwelve;
	if(which_keys)
	  *which_keys = km->which;  /* pass back to caller */
    }
}

      
    
/*----------------------------------------------------------------------
        Main loop executing commands for the mail index screen

   Args: state -- the pine_state structure for next/prev screen pointers
                  and to pass to the index manager...
 ----*/

void
mail_index_screen(state)
     struct pine *state;
{
    dprint(1, (debugfile, "\n\n ---- MAIL INDEX ----\n"));
    if(!state->mail_stream) {
	q_status_message(SM_ORDER, 0, 3, "No folder is currently open");
        state->prev_screen = mail_index_screen;
	state->next_screen = main_menu_screen;
	return;
    }

    index_lister(state, state->context_current, state->cur_folder,
		 state->mail_stream, state->msgmap);
    state->prev_screen = mail_index_screen;
}



/*----------------------------------------------------------------------
        Main loop executing commands for the mail index screen

   Args: state -- pine_state structure for display flags and such
         msgmap -- c-client/pine message number mapping struct
 ----*/

int
index_lister(state, cntxt, folder, stream, msgmap)
     struct pine *state;
     CONTEXT_S   *cntxt;
     char        *folder;
     MAILSTREAM  *stream;
     MSGNO_S     *msgmap;
{
    int		 ch, orig_ch, which_keys, force, cur_row, cur_col, km_popped;
    long	 i, j, k, old_max_msgno;
    IndexType    style, old_style = MsgIndex;
    struct index_state id;
#if defined(DOS) || defined(OS2)
    extern void (*while_waiting)();
#endif

    dprint(1, (debugfile, "\n\n ---- INDEX MANAGER ----\n"));
    
    ch                    = 'x';	/* For displaying msg 1st time thru */
    force                 = 0;
    km_popped             = 0;
    state->mangled_screen = 1;
    what_keymenu          = FirstMenu;
    memset((void *)&id, 0, sizeof(struct index_state));
    current_index_state   = &id;
    id.msgmap    = msgmap;

    if((id.stream = stream) != state->mail_stream)
      clear_index_cache();	/* BUG: should better tie stream to cache */

    set_need_format_setup();

    while (1) {
	if(km_popped){
	    km_popped--;
	    if(km_popped == 0){
		clearfooter(state);
		if(!state->mangled_body
		   && id.entry_state
		   && id.lines_per_page > 1){
		    id.entry_state[id.lines_per_page-2].id = -1;
		    id.entry_state[id.lines_per_page-1].id = -1;
		}
		else
		  state->mangled_body = 1;
	    }
	}

	old_max_msgno = mn_get_total(msgmap);

	/*------- Check for new mail -------*/
        new_mail(force, NM_TIMING(ch), 1);
	force = 0;			/* may not need to next time around */

	/*
	 * This is only actually necessary if this causes the width of the
	 * message number field to change.  That is, it depends on the
	 * format the user is using as well as on the max_msgno.  Since it
	 * should be rare, we'll just do it whenever it happens.
	 * Also have to check for a reduction in max_msgno when we expunge.
	 */
	if(old_max_msgno < 1000L && mn_get_total(msgmap) >= 1000L
	   || old_max_msgno < 10000L && mn_get_total(msgmap) >= 10000L
	   || old_max_msgno < 100000L && mn_get_total(msgmap) >= 100000L){
	    clear_index_cache();
	    state->mangled_body = 1;
        }

        if(streams_died())
          state->mangled_header = 1;

        if(state->mangled_screen){
            state->mangled_header = 1;
            state->mangled_body   = 1;
            state->mangled_footer = 1;
            state->mangled_screen = 0;
        }

	/*
	 * events may have occured that require us to shift from
	 * mode to another...
	 */
	style = (any_lflagged(msgmap, MN_HIDE))
		  ? ZoomIndex
		  : (mn_total_cur(msgmap) > 1L) ? MultiMsgIndex : MsgIndex;
	if(style != old_style){
            state->mangled_header = 1;
            state->mangled_footer = 1;
	    old_style = style;
	    id.msg_at_top = 0L;
	}

        /*------------ Update the title bar -----------*/
	if(state->mangled_header) {
            do_index_border(cntxt, folder, stream, msgmap, style, NULL,
			    INDX_HEADER);
	    state->mangled_header = 0;
	} 
	else if(mn_get_total(msgmap) > 0) {
	    /*
	     * No fetchstructure necessary before mail_elt as the elt's
	     * should have been loaded with the correct flags when we did
	     * a fetchflags as part of get_lflags the first time we painted
	     * the index...
	     */
	    update_titlebar_message();
            update_titlebar_status();
	}

	current_index_state = &id;

        /*------------ draw the index body ---------------*/
	cur_row = update_index(state, &id);
	if(F_OFF(F_SHOW_CURSOR, state)){
	    cur_row = state->ttyo->screen_rows - FOOTER_ROWS(state);
	    cur_col = 0;
	}
	else if(id.status_col >= 0)
	  cur_col = min(id.status_col, state->ttyo->screen_cols-1);

        ps_global->redrawer = redraw_index_body;

        /*------------ draw the footer/key menus ---------------*/
	if(state->mangled_footer) {
            if(!state->painted_footer_on_startup){
		if(km_popped){
		    FOOTER_ROWS(state) = 3;
		    clearfooter(state);
		}

		do_index_border(cntxt, folder, stream, msgmap, style,
			      &which_keys, INDX_FOOTER);
		if(km_popped){
		    FOOTER_ROWS(state) = 1;
		    mark_keymenu_dirty();
		}
	    }

	    state->mangled_footer = 0;
	}

        state->painted_body_on_startup   = 0;
        state->painted_footer_on_startup = 0;

	/*-- Display any queued message (eg, new mail, command result --*/
	if(km_popped){
	    FOOTER_ROWS(state) = 3;
	    mark_status_unknown();
	}

        display_message(ch);
	if(km_popped){
	    FOOTER_ROWS(state) = 1;
	    mark_status_unknown();
	}

	if(F_ON(F_SHOW_CURSOR, state) && cur_row < 0){
	    q_status_message(SM_ORDER,
		(ch==NO_OP_IDLE || ch==NO_OP_COMMAND) ? 0 : 3, 5,
		"No messages in folder");
	    cur_row = state->ttyo->screen_rows - FOOTER_ROWS(state);
	    display_message(ch);
	}

	MoveCursor(cur_row, cur_col);

        /* Let read_command do the fflush(stdout) */

        /*---------- Read command and validate it ----------------*/
#ifdef	MOUSE
	if(stream == ps_global->mail_stream){	/* prime the handler */
	    mouse_in_content(KEY_MOUSE, -1, -1, 0x5, 0);
	    register_mfunc(mouse_in_content, HEADER_ROWS(ps_global), 0,
			   state->ttyo->screen_rows-(FOOTER_ROWS(ps_global)+1),
			   state->ttyo->screen_cols);
			   
	}
#endif
#if defined(DOS) || defined(OS2)
	/*
	 * AND pre-build header lines.  This works just fine under
	 * DOS since we wait for characters in a loop. Something will
         * will have to change under UNIX if we want to do the same.
	 */
	while_waiting = build_header_cache;
#ifdef	_WINDOWS
	while_waiting = NULL;
	mswin_setscrollcallback (index_scroll_callback);
#endif
#endif
	ch = read_command();
#ifdef	MOUSE
	if(stream == ps_global->mail_stream)
	  clear_mfunc(mouse_in_content);
#endif
#if defined(DOS) || defined(OS2)
	while_waiting = NULL;
#ifdef	_WINDOWS
	mswin_setscrollcallback(NULL);
#endif
#endif

        orig_ch = ch;

	if(ch < 0x0100 && isupper((unsigned char)ch))	/* force lower case */
	  ch = tolower((unsigned char)ch);
	else if(ch >= PF1 && ch <= PF12 && which_keys > 0 && which_keys < 4)
	  ch = (which_keys == 1) ? PF2OPF(ch) :	/* map f-key to menu page */
		(which_keys == 2) ? PF2OOPF(ch) : PF2OOOPF(ch);

	ch = validatekeys(ch);

	if(km_popped)
	  switch(ch){
	    case NO_OP_IDLE:
	    case NO_OP_COMMAND: 
	    case PF2:
	    case OPF2:
            case OOPF2:
            case OOOPF2:
	    case 'o' :
	    case KEY_RESIZE:
	    case ctrl('L'):
	      km_popped++;
	      break;
	    
	    default:
	      clearfooter(state);
	      break;
	  }

	/*----------- Execute the command ------------------*/
	switch(ch) {

            /*---------- Roll keymenu ----------*/
          case PF2:
          case OPF2:
          case OOPF2:
          case OOOPF2:
	  case 'o':
            if(ps_global->anonymous) {
	      if(ch == PF2)
		ch = 'w';
              goto df;
	    }
            if(ch == 'o')
	      warn_other_cmds();
	    what_keymenu = NextTwelve;
	    state->mangled_footer = 1;
	    break;


            /*---------- Scroll back up ----------*/
	  case PF7:
	  case '-' :
          case ctrl('Y'): 
	  case KEY_PGUP:
	  pageup:
	    j = -1L;
	    for(k = i = id.msg_at_top; ; i--){
		if(!get_lflag(stream, msgmap, i, MN_HIDE)){
		    k = i;
		    if(++j >= id.lines_per_page){
			if((id.msg_at_top = i) == 1L)
			  q_status_message(SM_ORDER, 0, 1, "First Index page");

			break;
		    }
	       }

		if(i <= 1L){
		    if(mn_get_cur(msgmap) == 1L)
		      q_status_message(SM_ORDER, 0, 1,
			  "Already at start of Index");

		    break;
		}
	    }

	    if(mn_total_cur(msgmap) == 1L)
	      mn_set_cur(msgmap, k);

	    break;


            /*---------- Scroll forward, next page ----------*/
	  case PF8:
	  case '+':
          case ctrl('V'): 
	  case KEY_PGDN:
	  case ' ':
	  pagedown:
	    j = -1L;
	    for(k = i = id.msg_at_top; ; i++){
		if(!get_lflag(stream, msgmap, i, MN_HIDE)){
		    k = i;
		    if(++j >= id.lines_per_page){
			if(i+id.lines_per_page >= mn_get_total(msgmap))
			  q_status_message(SM_ORDER, 0, 1, "Last Index page");

			id.msg_at_top = i;
			break;
		    }
		}

		if(i >= mn_get_total(msgmap)){
		    if(mn_get_cur(msgmap) == k)
		      q_status_message(SM_ORDER,0,1,"Already at end of Index");

		    break;
		}
	    }

	    if(mn_total_cur(msgmap) == 1L)
	      mn_set_cur(msgmap, k);

	    break;

#ifdef MOUSE	    
	  case KEY_MOUSE:
	    {
	      static int lastWind;
	      MOUSEPRESS mp;
	      void	*text;
	      long	len;
	      int	format;
	      

	      mouse_get_last (NULL, &mp);
	      mp.row -= 2;
	      if (mp.doubleclick && mp.button != M_BUTTON_RIGHT) {
		  orig_ch = ch = F_ON(F_USE_FK, ps_global) ? PF4 : 'v';
		  goto df;
	      }
	      else{
		for(i = id.msg_at_top;
		    mp.row >= 0 && i <= mn_get_total(msgmap);
		    i++)
		  if(!get_lflag(stream, msgmap, i, MN_HIDE)){
		      mp.row--;
		      if(mn_total_cur(msgmap) == 1L)
			mn_set_cur(msgmap, i);
		  }
#ifdef _WINDOWS		     
		if (mp.button == M_BUTTON_RIGHT) {
		  char title[GETTEXT_TITLELEN+1];

		    /* Launch text in alt window. */
		  if (mp.flags & M_KEY_CONTROL)
		    lastWind = 0;
		  if (index_gettext_callback (title, &text, &len, &format)) {
		    if (format == GETTEXT_TEXT) 
		      lastWind = mswin_displaytext (title, text, (size_t)len, 
					      NULL, lastWind, 0);
		    else if (format == GETTEXT_LINES) 
		      lastWind = mswin_displaytext (title, NULL, 0, 
					      text, lastWind, 0);
		  }
		}
#endif /* _WINDOWS */
	      }
	    }

	    break;
#endif	/* MOUSE */

            /*---------- Redraw/resize ----------*/
          case KEY_RESIZE:
	    clear_index_cache();
	  case ctrl('L'):
	    mark_status_dirty();
	    mark_keymenu_dirty();
	    mark_titlebar_dirty();
            state->mangled_screen = 1;		/* force repaint and... */
	    if(ch == ctrl('L'))
	      force = 1;			/* check for new mail on ^L */

            break;


            /*---------- No op command ----------*/
          case NO_OP_IDLE:
	  case NO_OP_COMMAND:
            break;	/* no op check for new mail */


            /*---------- Default -- all other command ----------*/
          default:
          df:
	    if((ch == '?' || ch == ctrl('G') || ch == PF1
		    || ch == OPF1 || ch == OOPF1 || ch == OOOPF1)
		&& FOOTER_ROWS(state) == 1
		&& km_popped == 0){
		km_popped = 2;
		mark_status_unknown();
		mark_keymenu_dirty();
		state->mangled_footer = 1;
	    }
	    else if(stream == state->mail_stream){
		process_cmd(state, msgmap, ch,
			    (style == MsgIndex || style == MultiMsgIndex)?1:2,
			    orig_ch, &force);
		if(state->next_screen != SCREEN_FUN_NULL){
		    ps_global->redrawer = NULL;
		    current_index_state = NULL;
		    if(id.entry_state)
		      fs_give((void **)&(id.entry_state));

		    return(0);
		}
		else{
		    if(stream != state->mail_stream){
			/*
			 * Must have had an failed open.  repair our
			 * pointers...
			 */
			id.stream = stream = state->mail_stream;
			id.msgmap = msgmap = state->msgmap;
		    }

		    current_index_state = &id;
		}

		/*
		 * Page framing exception handling here.  If we
		 * did something that should scroll-by-a-line, frame
		 * the page by hand here rather than leave it to the
		 * page-by-page framing in update_index()...
		 */
		switch(ch){
		  case KEY_DOWN:
		  case ctrl('N'):
		    for(j = 0L, k = i = id.msg_at_top; ; i++){
			if(!get_lflag(stream, msgmap, i, MN_HIDE)){
			    k = i;
			    if(j++ >= id.lines_per_page)
			      break;
			}

			if(i >= mn_get_total(msgmap)){
			    k = 0L;		/* don't scroll */
			    break;
			}
		    }

		    if(k && (mn_get_cur(msgmap) + HS_MARGIN(state)) >= k)
		      index_scroll_down(1L);

		    break;

		  case ctrl('P'):
		  case KEY_UP:
		    if(mn_get_cur(msgmap) < (id.msg_at_top + HS_MARGIN(state)))
		      index_scroll_up(1L);

		    break;

		  default:
		    break;
		}
	    }
	    else{			/* special processing */
		switch(ch){
		  case '?':		/* help! */
		  case PF1:
		  case ctrl('G'):
		    helper(h_simple_index,
			   (!strcmp(folder, INTERRUPTED_MAIL))
			     ? "HELP FOR SELECTING INTERRUPTED MSG"
			     : "HELP FOR SELECTING POSTPONED MSG",
			   1);
		    state->mangled_screen = 1;
		    break;

		  case 'd':		/* delete */
		  case 'D':
		  case PF9:
		    dprint(3,(debugfile, "Special delete: msg %s\n",
			      long2string(mn_get_cur(msgmap))));
		    {
			MESSAGECACHE *mc;
			long	      raw;
			int	      del = 0;

			raw = mn_m2raw(msgmap, mn_get_cur(msgmap));
			if(!mail_elt(stream, raw)->deleted){
			    clear_index_cache_ent(mn_get_cur(msgmap));
			    mail_setflag(stream,long2string(raw),"\\DELETED");
			    update_titlebar_status();
			    del++;
			}

			q_status_message2(SM_ORDER, 0, 1,
					  "Message %s %sdeleted",
					  long2string(mn_get_cur(msgmap)),
					  (del) ? "" : "already ");
		    }

		    break;

		  case 'u':		/* UNdelete */
		  case 'U':
		  case PF10:
		    dprint(3,(debugfile, "Special UNdelete: msg %s\n",
			      long2string(mn_get_cur(msgmap))));
		    {
			MESSAGECACHE *mc;
			long	      raw;
			int	      del = 0;

			raw = mn_m2raw(msgmap, mn_get_cur(msgmap));
			if(mail_elt(stream, raw)->deleted){
			    clear_index_cache_ent(mn_get_cur(msgmap));
			    mail_clearflag(stream, long2string(raw),
					   "\\DELETED");
			    update_titlebar_status();
			    del++;
			}

			q_status_message2(SM_ORDER, 0, 1,
					  "Message %s %sdeleted",
					  long2string(mn_get_cur(msgmap)),
					  (del) ? "UN" : "NOT ");
		    }

		    break;

		  case 'e':		/* exit */
		  case 'E':
		  case PF3:
		    ps_global->redrawer = NULL;
		    current_index_state = NULL;
		    if(id.entry_state)
		      fs_give((void **)&(id.entry_state));

		    return(1);
		    break;

		  case 's':		/* select */
		  case ctrl('M'):
		  case ctrl('J'):
		  case PF4:
		    ps_global->redrawer = NULL;
		    current_index_state = NULL;
		    if(id.entry_state)
		      fs_give((void **)&(id.entry_state));

		    return(0);

		  case 'p':		/* previous */
		  case ctrl('P'):
		  case KEY_UP:
		  case PF5:
		    mn_dec_cur(stream, msgmap);
		    break;

		  case 'n':		/* next */
		  case ctrl('N'):
		  case KEY_DOWN:
		  case PF6:
		    mn_inc_cur(stream, msgmap);
		    break;

		  default :
		    bogus_command(ch, NULL);
		    break;
		}
	    }
	}				/* The big switch */
    }					/* the BIG while loop! */
}



/*----------------------------------------------------------------------
  Manage index body painting

  Args: state - pine struct containing selected message data
	index_state - struct describing what's currently displayed

  Returns: screen row number of first highlighted message

  The idea is pretty simple.  Maintain an array of index line id's that
  are displayed and their hilited state.  Decide what's to be displayed
  and update the screen appropriately.  All index screen painting
  is done here.  Pretty simple, huh?
 ----*/
int
update_index(state, screen)
    struct pine         *state;
    struct index_state  *screen;
{
    int  i, agg, retval = -1;
    long n;

    if(!screen)
      return(-1);

#ifdef _WINDOWS
    mswin_beginupdate();
#endif

    /*---- reset the works if necessary ----*/
    if(state->mangled_body && screen->entry_state){
	fs_give((void **)&(screen->entry_state));
	screen->lines_per_page = 0;
	ClearBody();
    }

    state->mangled_body = 0;

    /*---- make sure we have a place to write state ----*/
    if(screen->lines_per_page
	!= max(0, state->ttyo->screen_rows - FOOTER_ROWS(state)
					   - HEADER_ROWS(state))){
	i = screen->lines_per_page;
	screen->lines_per_page
	    = max(0, state->ttyo->screen_rows - FOOTER_ROWS(state)
					      - HEADER_ROWS(state));
	if(!i){
	    size_t len = screen->lines_per_page * sizeof(struct entry_state);
	    screen->entry_state = (struct entry_state *) fs_get(len);
	}
	else
	  fs_resize((void **)&(screen->entry_state),
		    (size_t)screen->lines_per_page);

	for(; i < screen->lines_per_page; i++)	/* init new entries */
	  screen->entry_state[i].id = -1;
    }

    /*---- figure out the first message on the display ----*/
    if(screen->msg_at_top < 1L
       || (any_lflagged(screen->msgmap, MN_HIDE) > 0L
	   && get_lflag(screen->stream, screen->msgmap,
			screen->msg_at_top, MN_HIDE))){
	screen->msg_at_top = top_ent_calc(screen->stream, screen->msgmap,
					  screen->msg_at_top,
					  screen->lines_per_page);
    }
    else if(mn_get_cur(screen->msgmap) < screen->msg_at_top){
	long i, j, k;

	/* scroll back a page at a time until current is displayed */
	while(mn_get_cur(screen->msgmap) < screen->msg_at_top){
	    for(i = screen->lines_per_page, j = screen->msg_at_top-1L, k = 0L;
		i > 0L && j > 0L;
		j--)
	      if(!get_lflag(screen->stream, screen->msgmap, j, MN_HIDE)){
		  k = j;
		  i--;
	      }

	    if(i == screen->lines_per_page)
	      break;				/* can't scroll back ? */
	    else
	      screen->msg_at_top = k;
	}
    }
    else if(mn_get_cur(screen->msgmap) >= screen->msg_at_top
						     + screen->lines_per_page){
	long i, j, k;

	while(1){
	    for(i = screen->lines_per_page, j = k = screen->msg_at_top;
		j <= mn_get_total(screen->msgmap) && i > 0L;
		j++)
	      if(!get_lflag(screen->stream, screen->msgmap, j, MN_HIDE)){
		  k = j;
		  i--;
	      }

	    if(mn_get_cur(screen->msgmap) <= k)
	      break;
	    else{
		/* set msg_at_top to next displayed message */
		for(i = k + 1L; i <= mn_get_total(screen->msgmap); i++)
		  if(!get_lflag(screen->stream, screen->msgmap, i, MN_HIDE)){
		      k = i;
		      break;
		  }

		screen->msg_at_top = k;
	    }
	}
    }

#ifdef	_WINDOWS
    /* Set scroll range and position.  Note that message numbers start at 1
     * while scroll position starts at 0. */
    scroll_setrange(mn_get_total(screen->msgmap) - 1L);
    scroll_setpos(screen->msg_at_top - 1L);
#endif

    /*---- march thru display lines, painting whatever is needed ----*/
    for(i = 0, n = screen->msg_at_top; i < (int) screen->lines_per_page; i++){
	if(n < 1 || n > mn_get_total(screen->msgmap)){
	    if(screen->entry_state[i].id){
		screen->entry_state[i].hilite = 0;
		screen->entry_state[i].bolded = 0;
		screen->entry_state[i].id     = 0L;
		ClearLine(HEADER_ROWS(state) + i);
	    }
	}
	else{
	    int      cur = mn_is_cur(screen->msgmap, n),
		     sel = get_lflag(screen->stream,screen->msgmap,n,MN_SLCT),
		     status_col;
	    HLINE_S *h   = build_header_line(state,
				screen->stream,screen->msgmap,n);

	    status_col = screen->status_col;

	    if(h->id != screen->entry_state[i].id
	       || (cur != screen->entry_state[i].hilite)
	       || (sel != screen->entry_state[i].bolded)){
		MoveCursor(HEADER_ROWS(state) + i, 0);
		if(F_ON(F_FORCE_LOW_SPEED,ps_global) || ps_global->low_speed){
		    MoveCursor(HEADER_ROWS(state) + i, status_col);
		    Writechar((sel) ? 'X' :
			        (cur && h->line[status_col] == ' ') ?
				    '-' : h->line[status_col], 0);
		    Writechar((cur) ? '>' : h->line[status_col+1], 0);

		    if(h->id != screen->entry_state[i].id){
			if(status_col == 0)
			  PutLine0(HEADER_ROWS(state) + i, 2, &h->line[2]);
			else{ /* this will rarely be set up this way */
			    char save_char1, save_char2;

			    save_char1 = h->line[status_col];
			    save_char2 = h->line[status_col+1];
			    h->line[status_col] = (sel) ? 'X' :
				(cur && save_char1 == ' ') ?
				 '-' : save_char1;
			    h->line[status_col+1] = (cur) ? '>' :
							     save_char2;
			    PutLine0(HEADER_ROWS(state) + i, 0, &h->line[0]);
			    h->line[status_col]   = save_char1;
			    h->line[status_col+1] = save_char2;
			}
		    }
		}
		else{
		    char *draw = h->line;
		    char  save_char;
		    int   drew_X = 0;

		    if(cur)
		      StartInverse();
		    
		    save_char = draw[status_col];

		    if(sel && (F_OFF(F_SELECTED_SHOWN_BOLD, state)
			       || !StartBold())){
			draw[status_col] = 'X';
			drew_X++;
		    }

		    Write_to_screen(draw);
		    if(drew_X)
		      draw[status_col] = save_char;

		    if(sel && !drew_X)
		      EndBold();

		    if(cur)
		      EndInverse();
		}
	    }

	    screen->entry_state[i].hilite = cur;
	    screen->entry_state[i].bolded = sel;
	    screen->entry_state[i].id     = h->id;

	    if(cur && retval < 0L)
	      retval = i + HEADER_ROWS(state);
	}

	/*--- increment n ---*/
	while(++n <= mn_get_total(screen->msgmap)
	      && get_lflag(screen->stream, screen->msgmap, n, MN_HIDE))
	  ;

    }

#ifdef _WINDOWS
    mswin_endupdate();
#endif
    fflush(stdout);
    return(retval);
}



/*----------------------------------------------------------------------
     Scroll to specified postion.


  Args: paint - TRUE when this function shoult repaint the screen.
	pos - position to scroll to.

  Returns: TRUE - did the scroll operation.
	   FALSE - was not able to do the scroll operation.
 ----*/
int
index_scroll_to_pos (pos)
long	pos;
{
    static short bad_timing = 0;
    long	i, j, k;
    
    if(bad_timing)
      return (FALSE);

    /*
     * Put the requested line at the top of the screen...
     */
#if 1
    /*
     * Starting at msg 'pos' find next visable message.
     */
    for(i=pos; i <= mn_get_total(current_index_state->msgmap); i++) {
      if(!get_lflag(current_index_state->stream, 
	            current_index_state->msgmap, i, MN_HIDE)){
	  current_index_state->msg_at_top = i;
	  break;
      }
    }
#else
    for(i=1L, j=pos; i <= mn_get_total(current_index_state->msgmap); i++) {
      if(!get_lflag(current_index_state->stream, 
	            current_index_state->msgmap, i, MN_HIDE)){
	  if((current_index_state->msg_at_top = i) > 
		  mn_get_cur(current_index_state->msgmap))
	    mn_set_cur(current_index_state->msgmap, i);

	  if(--j <= 0L)
	    break;
      }
    }
#endif

    /*
     * If single selection, move selected message to be on the sceen.
     */
    if (mn_total_cur(current_index_state->msgmap) == 1L) {
      if (current_index_state->msg_at_top > 
			      mn_get_cur (current_index_state->msgmap)) {
	/* Selection was above screen, move to top of screen. */
	mn_set_cur (current_index_state->msgmap, 
					current_index_state->msg_at_top);
      }
      else {
	/* Scan through the screen.  If selection found, leave where is.
	 * Otherwise, move to end of screen */
        for(  i = current_index_state->msg_at_top, 
	        j = current_index_state->lines_per_page;
	      i != mn_get_cur(current_index_state->msgmap) && 
		i <= mn_get_total(current_index_state->msgmap) && 
		j > 0L;
	      i++) {
	    if(!get_lflag(current_index_state->stream, 
	            current_index_state->msgmap, i, MN_HIDE)){
	        j--;
	        k = i;
            }
        }
	if(j <= 0L)
	    /* Move to end of screen. */
	    mn_set_cur(current_index_state->msgmap, k);
      }
    }

    bad_timing = 0;
    return (TRUE);
}



/*----------------------------------------------------------------------
     Adjust the index display state down a line, and repaint.


  Returns: TRUE - did the scroll operation.
	   FALSE - was not able to do the scroll operation.
 ----*/
int
index_scroll_down(scroll_count)
    long scroll_count;
{
    static short bad_timing = 0;
    long i, j, k;
    long cur, total;

    if(bad_timing)
      return (FALSE);

    bad_timing = 1;
    
    
    j = -1L;
    total = mn_get_total (current_index_state->msgmap);
    for(k = i = current_index_state->msg_at_top; ; i++){

	/* Only examine non-hidden messages. */
        if(!get_lflag(current_index_state->stream, 
		      current_index_state->msgmap, i, MN_HIDE)){
	    /* Remember this message */
	    k = i;
	    /* Increment count of lines.  */
	    if (++j >= scroll_count) {
		/* Counted enough lines, stop. */
		current_index_state->msg_at_top = k;
		break;
	    }
	}
	    
	/* If at last message, stop. */
	if (i >= total){
	    current_index_state->msg_at_top = k;
	    break;
	}
    }

    /*
     * If not multiple selection, see if selected message visable.  if not
     * set it to last visable message. 
     */
    if(mn_total_cur(current_index_state->msgmap) == 1L) {
	j = 0L;
	cur = mn_get_cur (current_index_state->msgmap);
	for (i = current_index_state->msg_at_top; i <= total; ++i) {
	    if(!get_lflag(current_index_state->stream, 
		          current_index_state->msgmap, i, MN_HIDE)) {
	        if (++j >= current_index_state->lines_per_page) {
		    break;
	        }
		if (i == cur) 
		    break;
	    }
        }
	if (i != cur) 
	    mn_set_cur(current_index_state->msgmap, 
					    current_index_state->msg_at_top);
    }

    bad_timing = 0;
    return (TRUE);
}



/*----------------------------------------------------------------------
     Adjust the index display state up a line

  Args: paint - TRUE when this function shoult repaint the screen.

  Returns: TRUE - did the scroll operation.
	   FALSE - was not able to do the scroll operation.

 ----*/
int
index_scroll_up(scroll_count)
    long scroll_count;
{
    static short bad_timing = 0;
    long i, j, k;
    long cur;

    if(bad_timing)
      return(FALSE);

    bad_timing = 1;
    
    j = -1L;
    for(k = i = current_index_state->msg_at_top; ; i--){

	/* Only examine non-hidden messages. */
        if(!get_lflag(current_index_state->stream, 
		      current_index_state->msgmap, i, MN_HIDE)){
	    /* Remember this message */
	    k = i;
	    /* Increment count of lines.  */
	    if (++j >= scroll_count) {
		/* Counted enough lines, stop. */
		current_index_state->msg_at_top = k;
		break;
	    }
	}
	    
	/* If at first message, stop */
	if (i <= 1L){
	    current_index_state->msg_at_top = k;
	    break;
	}
    }

    
    /*
     * If not multiple selection, see if selected message visable.  if not
     * set it to last visable message. 
     */
    if(mn_total_cur(current_index_state->msgmap) == 1L) {
	j = 0L;
	cur = mn_get_cur (current_index_state->msgmap);
	for (	i = current_index_state->msg_at_top; 
		i <= mn_get_total(current_index_state->msgmap);
		++i) {
	    if(!get_lflag(current_index_state->stream, 
		          current_index_state->msgmap, i, MN_HIDE)) {
	        if (++j >= current_index_state->lines_per_page) {
		    k = i;
		    break;
	        }
		if (i == cur) 
		    break;
	    }
        }
	if (i != cur) 
	    mn_set_cur(current_index_state->msgmap, k);
    }


    bad_timing = 0;
    return (TRUE);
}



/*----------------------------------------------------------------------
     Calculate the message number that should be at the top of the display

  Args: current - the current message number
        lines_per_page - the number of lines for the body of the index only

  Returns: -1 if the current message is -1 
           the message entry for the first message at the top of the screen.

When paging in the index it is always on even page boundies, and the
current message is always on the page thus the top of the page is
completely determined by the current message and the number of lines
on the page. 
 ----*/
long
top_ent_calc(stream, msgs, at_top, lines_per_page)
     MAILSTREAM *stream;
     MSGNO_S *msgs;
     long     at_top, lines_per_page;
{
    long current;

    current = (mn_total_cur(msgs) <= 1L) ? mn_get_cur(msgs) : at_top;

    if(current < 0L)
      return(-1);

    if(lines_per_page == 0L)
      return(current);

    if(any_lflagged(msgs, (MN_HIDE|MN_EXLD))){
	long n, m = 0L, t = 1L;

	for(n = 1L; n <= mn_get_total(msgs); n++)
	  if(!get_lflag(stream, msgs, n, MN_HIDE)
	     && (++m % lines_per_page) == 1L){
	      if(n > current)
		break;

	      t = n;
	  }

	return(t);
    }
    else
      return(lines_per_page * ((current - 1L)/ lines_per_page) + 1L);
}


/*----------------------------------------------------------------------
      Initialize the index_disp_format array in ps_global from this
      format string.

   Args: format -- the string containing the format tokens
	 answer -- put the answer here, free first if there was a previous
		    value here
 ----*/
void
init_index_format(format, answer)
char         *format;
INDEX_COL_S **answer;
{
    int column = 0;

    set_need_format_setup();
    /* if custom format is specified, try it, else go with default */
    if(!(format && *format && parse_index_format(format, answer))){
	if(*answer)
	  fs_give((void **)answer);

	*answer = (INDEX_COL_S *)fs_get(7*sizeof(INDEX_COL_S));
	memset((void *)(*answer), 0, 7*sizeof(INDEX_COL_S));

	(*answer)[column].ctype		= iStatus;
	(*answer)[column].wtype		= Fixed;
	(*answer)[column++].req_width	= 3;

	/*
	 * WeCalculate with a non-zero req_width means that this space
	 * will be reserved before calculating the percentages.  It may
	 * get increased or decreased later when we see what we actually need.
	 */
	(*answer)[column].ctype		= iMessNo;
	(*answer)[column++].wtype	= WeCalculate;

	(*answer)[column].ctype		= iDate;
	(*answer)[column].wtype		= Fixed;
	(*answer)[column++].req_width	= 6;

	(*answer)[column].ctype		= iFromTo;
	(*answer)[column].wtype		= Percent;
	(*answer)[column++].req_width	= 33; /* percent of rest */

	(*answer)[column].ctype		= iSize;
	(*answer)[column++].wtype	= WeCalculate;

	(*answer)[column].ctype		= iSubject;
	(*answer)[column].wtype		= Percent;
	(*answer)[column++].req_width	= 67;

	(*answer)[column].ctype		= iNothing;
    }

    /*
     * Fill in req_width's for WeCalculate items.
     */
    for(column = 0; (*answer)[column].ctype != iNothing; column++){
	if((*answer)[column].wtype == WeCalculate){
	    switch((*answer)[column].ctype){
	      case iStatus:
		(*answer)[column].req_width = 3;
		break;
	      case iFStatus:
		(*answer)[column].req_width = 6;
		break;
	      case iMessNo:
		(*answer)[column].req_width = 3;
		break;
	      case iDate:
		(*answer)[column].req_width = 6;
		break;
	      case iSize:
		(*answer)[column].req_width = 8;
		break;
	      case iDescripSize:
		(*answer)[column].req_width = 9;
		break;
	    }
	}
    }
}


struct index_parse_tokens {
    char        *name;
    IndexColType ctype;
};

struct index_parse_tokens itokens[] = {
    {"STATUS",      iStatus},
    {"FULLSTATUS",  iFStatus},
    {"MSGNO",       iMessNo},
    {"DATE",        iDate},
    {"FROMORTO",    iFromTo},
    {"FROM",        iFrom},
    {"TO",          iTo},
    {"SENDER",      iSender},
    {"SIZE",        iSize},
    {"DESCRIPSIZE", iDescripSize},
    {"SUBJECT",     iSubject},
    {NULL,          iNothing}
};

int
parse_index_format(format_str, answer)
char         *format_str;
INDEX_COL_S **answer;
{
    int i, column = 0;
    char *p, *q;
    struct index_parse_tokens *pt;
    INDEX_COL_S cdesc[MAXIFLDS]; /* temp storage for answer */

    get_body_for_index = 0;
    memset((void *)cdesc, 0, sizeof(cdesc));

    p = format_str;
    while(p && *p && column < MAXIFLDS-1){
	/* skip leading white space for next word */
	while(p && *p && isspace((unsigned char)*p))
	  p++;
    
	/* look for the token this word matches */
	for(pt = itokens; pt->name; pt++)
	    if(!struncmp(pt->name, p, strlen(pt->name)))
	      break;
	
	/* ignore unrecognized word */
	if(!pt->name){
	    for(q = p; *p && !isspace((unsigned char)*p); p++)
	      ;

	    if(*p)
	      *p++ = '\0';

	    dprint(1, (debugfile,
		       "parse_index_format: unrecognized token: %s\n", q));
	    q_status_message1(SM_ORDER | SM_DING, 0, 3,
			      "Unrecognized string in index-format: %s", q);
	    continue;
	}

	cdesc[column].ctype = pt->ctype;
	if(pt->ctype == iDescripSize)
	  get_body_for_index = 1;

	/* skip over name and look for parens */
	p += strlen(pt->name);
	if(*p == '('){
	    p++;
	    q = p;
	    while(p && *p && isdigit((unsigned char)*p))
	      p++;
	    
	    if(p && *p && *p == ')' && p > q){
		cdesc[column].wtype = Fixed;
		cdesc[column].req_width = atoi(q);
	    }
	    else if(p && *p && *p == '%' && p > q){
		cdesc[column].wtype = Percent;
		cdesc[column].req_width = atoi(q);
	    }
	    else{
		cdesc[column].wtype = WeCalculate;
		cdesc[column].req_width = 0;
	    }
	}
	else{
	    cdesc[column].wtype     = WeCalculate;
	    cdesc[column].req_width = 0;
	}

	column++;
	/* skip text at end of word */
	while(p && *p && !isspace((unsigned char)*p))
	  p++;
    }

    /* if, after all that, we didn't find anything recoznizable, bitch */
    if(!column){
	dprint(1, (debugfile, "Completely unrecognizable index-format\n"));
	q_status_message(SM_ORDER | SM_DING, 0, 3,
		 "Configured \"index-format\" unrecognizable. Using default.");
	return(0);
    }

    /* Finish with Nothing column */
    cdesc[column].ctype = iNothing;

    /* free up old answer */
    if(*answer)
      fs_give((void **)answer);

    /* allocate space for new answer */
    *answer = (INDEX_COL_S *)fs_get((column+1)*sizeof(INDEX_COL_S));
    memset((void *)(*answer), 0, (column+1)*sizeof(INDEX_COL_S));
    /* copy answer to real place */
    for(i = 0; i <= column; i++)
      (*answer)[i] = cdesc[i];

    return(1);
}


/*----------------------------------------------------------------------
    This redraws the body of the index screen, taking into
account any change in the size of the screen. All the state needed to
repaint is in the static variables so this can be called from
anywhere.
 ----*/
void
redraw_index_body()
{
    int agg;

    if(agg = (mn_total_cur(current_index_state->msgmap) > 1L))
      restore_selected(current_index_state->msgmap);

    ps_global->mangled_body = 1;

    (void) update_index(ps_global, current_index_state);
    if(agg)
      pseudo_selected(current_index_state->msgmap);
}



/*----------------------------------------------------------------------
      Setup the widths of the various columns in the index display

   Args: news      -- mail stream is news
	 max_msgno -- max message number in mail stream
 ----*/
void
setup_header_widths(cdesc, news, max_msgno)
    INDEX_COL_S *cdesc;
    int          news;
    long         max_msgno;
{
    int       i, j, columns, some_to_calculate;
    int	      space_left, screen_width, width, fix, col, scol, altcol;
    int	      keep_going, tot_pct, was_sl;
    WidthType wtype;


    dprint(8, (debugfile, "=== setup_header_widths(%d,%ld) ===\n",
	news, max_msgno));

    clear_need_format_setup();
    screen_width = ps_global->ttyo->screen_cols;
    columns = 0;
    some_to_calculate = 0;
    space_left = screen_width;

    /*
     * Calculate how many fields there are so we know how many spaces
     * between columns to reserve.  Fill in Fixed widths now.  Reserve
     * special case WeCalculate with non-zero req_widths before doing
     * Percent cases below.
     */
    for(i = 0; cdesc[i].ctype != iNothing; i++){

	/* These aren't included in nr mode */
	if(ps_global->nr_mode && (cdesc[i].ctype == iFromTo ||
				  cdesc[i].ctype == iFrom ||
				  cdesc[i].ctype == iSender ||
				  cdesc[i].ctype == iTo)){
	    cdesc[i].req_width = 0;
	    cdesc[i].width = 0;
	    cdesc[i].wtype = Fixed;
	}
	else if(cdesc[i].wtype == Fixed){
	  cdesc[i].width = cdesc[i].req_width;
	  if(cdesc[i].width > 0)
	    columns++;
	}
	else if(cdesc[i].wtype == Percent){
	    cdesc[i].width = 0; /* calculated later */
	    columns++;
	}
	else{ /* WeCalculate */
	    cdesc[i].width = cdesc[i].req_width; /* reserve this for now */
	    some_to_calculate++;
	    columns++;
	}

	space_left -= cdesc[i].width;
    }

    space_left -= (columns - 1); /* space between columns */

    for(i = 0; cdesc[i].ctype != iNothing; i++){
	wtype = cdesc[i].wtype;
	if(wtype != WeCalculate && wtype != Percent && cdesc[i].width == 0)
	  continue;

	switch(cdesc[i].ctype){
	  case iStatus:
	    cdesc[i].actual_length = 3;
	    cdesc[i].adjustment = Left;
	    break;

	  case iFStatus:
	    cdesc[i].actual_length = 6;
	    cdesc[i].adjustment = Left;
	    break;

	  case iDate:
	    cdesc[i].actual_length = 6;
	    cdesc[i].adjustment = Left;
	    break;

	  case iFromTo:
	  case iFrom:
	  case iSender:
	  case iTo:
	  case iSubject:
	    cdesc[i].adjustment = Left;
	    break;

	  case iMessNo:
	    if(max_msgno < 1000)
	      cdesc[i].actual_length = 3;
	    else if(max_msgno < 10000)
	      cdesc[i].actual_length = 4;
	    else if(max_msgno < 100000)
	      cdesc[i].actual_length = 5;
	    else
	      cdesc[i].actual_length = 6;

	    cdesc[i].adjustment = Right;
	    break;

	  case iSize:
	    if(news)
	      cdesc[i].actual_length = 0;
	    else
	      cdesc[i].actual_length = 8;

	    cdesc[i].adjustment = Right;
	    break;

	  case iDescripSize:
	    if(news)
	      cdesc[i].actual_length = 0;
	    else
	      cdesc[i].actual_length = 9;

	    cdesc[i].adjustment = Right;
	    break;
	}
    }

    /* if have reserved unneeded space for size, give it back */
    for(i = 0; cdesc[i].ctype != iNothing; i++){
      if(cdesc[i].ctype == iSize || cdesc[i].ctype == iDescripSize){
	if(cdesc[i].actual_length == 0){
	  if((fix=cdesc[i].width) > 0){ /* had this reserved */
	    cdesc[i].width = 0;
	    space_left += fix;
	  }

	  space_left++;  /* +1 for space between columns */
	}
      }
    }

    /*
     * Calculate the field widths that are basically fixed in width.
     * Do them in this order in case we don't have enough space to go around.
     */
    for(j = 0; j < 6 && space_left > 0 && some_to_calculate; j++){
      IndexColType targetctype;

      switch(j){
	case 0:
	  targetctype = iMessNo;
	  break;

	case 1:
	  targetctype = iStatus;
	  break;

	case 2:
	  targetctype = iFStatus;
	  break;

	case 3:
	  targetctype = iDate;
	  break;

	case 4:
	  targetctype = iSize;
	  break;

	case 5:
	  targetctype = iDescripSize;
	  break;
      }

      for(i = 0;
	  cdesc[i].ctype != iNothing && space_left >0 && some_to_calculate;
	  i++){
	if(cdesc[i].ctype == targetctype && cdesc[i].wtype == WeCalculate){
	  some_to_calculate--;
	  fix = min(cdesc[i].actual_length - cdesc[i].width, space_left);
	  cdesc[i].width += fix;
	  space_left -= fix;
	}
      }
    }

    /*
     * Fill in widths for Percent cases.  If there are no more to calculate,
     * use the percentages as relative numbers and use the rest of the space,
     * else treat them as absolute percentages of the original avail screen.
     */
    if(space_left > 0){
      if(some_to_calculate){
        for(i = 0; cdesc[i].ctype != iNothing && space_left > 0; i++){
	    if(cdesc[i].wtype == Percent){
		/* The 2, 200, and +100 are because we're rounding */
		fix = ((2*cdesc[i].req_width *
		    (screen_width-(columns-1)))+100) / 200;
	        fix = min(fix, space_left);
	        cdesc[i].width += fix;
	        space_left -= fix;
	    }
	}
      }
      else{
	tot_pct = 0;
	was_sl = space_left;
	/* add up total percentages requested */
        for(i = 0; cdesc[i].ctype != iNothing; i++)
	    if(cdesc[i].wtype == Percent)
	      tot_pct += cdesc[i].req_width;

	/* give relative weight to requests */
        for(i = 0;
	    cdesc[i].ctype != iNothing && space_left > 0 && tot_pct > 0;
	    i++){
	    if(cdesc[i].wtype == Percent){
		fix = ((2*cdesc[i].req_width*was_sl)+tot_pct) / (2*tot_pct);
	        fix = min(fix, space_left);
	        cdesc[i].width += fix;
	        space_left -= fix;
	    }
	}
      }
    }

    /* split up rest, give twice as much to Subject */
    keep_going = 1;
    while(space_left > 0 && keep_going){
      keep_going = 0;
      for(i = 0; cdesc[i].ctype != iNothing && space_left > 0; i++){
	if(cdesc[i].wtype == WeCalculate &&
	  (cdesc[i].ctype == iFromTo ||
	   cdesc[i].ctype == iFrom ||
	   cdesc[i].ctype == iSender ||
	   cdesc[i].ctype == iTo ||
	   cdesc[i].ctype == iSubject)){
	  keep_going++;
	  cdesc[i].width++;
	  space_left--;
	  if(space_left > 0 && cdesc[i].ctype == iSubject){
	      cdesc[i].width++;
	      space_left--;
	  }
	}
      }
    }

    /* if still more, pad out percent's */
    keep_going = 1;
    while(space_left > 0 && keep_going){
      keep_going = 0;
      for(i = 0; cdesc[i].ctype != iNothing && space_left > 0; i++){
	if(cdesc[i].wtype == Percent &&
	  (cdesc[i].ctype == iFromTo ||
	   cdesc[i].ctype == iFrom ||
	   cdesc[i].ctype == iSender ||
	   cdesc[i].ctype == iTo ||
	   cdesc[i].ctype == iSubject)){
	  keep_going++;
	  cdesc[i].width++;
	  space_left--;
	}
      }
    }

    col = 0;
    scol = -1;
    altcol = -1;
    /* figure out what column is start of status field */
    for(i = 0; cdesc[i].ctype != iNothing; i++){
	width = cdesc[i].width;
	if(width == 0)
	  continue;

	/* space between columns */
	if(col > 0)
	  col++;

	if(cdesc[i].ctype == iStatus){
	    scol = col;
	    break;
	}

	if(cdesc[i].ctype == iFStatus){
	    scol = col;
	    break;
	}

	if(cdesc[i].ctype == iMessNo)
	  altcol = col;

	col += width;
    }

    if(scol == -1){
	if(altcol == -1)
	  scol = 0;
	else
	  scol = altcol;
    }

    if(current_index_state)
      current_index_state->status_col = scol;
}



/*----------------------------------------------------------------------
      Create a string summarizing the message header for index on screen

   Args: stream -- mail stream to fetch envelope info from
	 msgmap -- message number to pine sort mapping
	 msgno  -- Message number to create line for

  Result: returns a malloced string
          saves string in a cache for next call for same header
 ----*/
HLINE_S *
build_header_line(state, stream, msgmap, msgno)
    struct pine *state;
    MAILSTREAM  *stream;
    MSGNO_S     *msgmap;
    long         msgno;
{
    ENVELOPE     *envelope;
    MESSAGECACHE *cache;
    ADDRESS      *addr;
    char          str_buf[MAXIFLDS][MAX_SCREEN_COLS+1], to_us, *field;
    char         *s_tmp, *buffer, *p, *str;
    HLINE_S      *hline;
    struct date   d;
    INDEX_COL_S  *cdesc;
    int           i, width, which_array, no_data = 0;
    BODY *body = NULL, **bodyp;


    dprint(8, (debugfile, "=== build_header_line(%ld) ===\n", msgno));

    if(check_need_format_setup())
      setup_header_widths(state->index_disp_format, IS_NEWS(stream),
			  mn_get_total(msgmap));

    /* cache hit */
    if(*(buffer = (hline = get_index_cache(msgno))->line) != '\0') {
        dprint(9, (debugfile, "Hit: Returning %p -> <%s (%d), %ld>\n",
		   hline, buffer, strlen(buffer), hline->id));
	return(hline);
    }

    bodyp = (get_body_for_index && !IS_NEWS(stream)) ? &body : NULL;

    envelope = mail_fetchstructure(stream, mn_m2raw(msgmap, msgno), bodyp);
    cache    = mail_elt(stream, mn_m2raw(msgmap, msgno));

    if(!envelope || !cache)
      no_data = 2;
    /*
     * Check that the envelope returned has something to display.
     * If empty, indicate that no message info found.
     */
    else if(!envelope->remail && !envelope->return_path && !envelope->date &&
       !envelope->from && !envelope->sender && !envelope->reply_to &&
       !envelope->subject && !envelope->to && !envelope->cc &&
       !envelope->bcc && !envelope->in_reply_to && !envelope->message_id &&
       !envelope->newsgroups)
      no_data = 1;

    cdesc = state->index_disp_format;
    which_array = 0;

    /* calculate contents of the required fields */
    for(i = 0; cdesc[i].ctype != iNothing; i++){
	width  = cdesc[i].width;
	if(width == 0)
	  continue;

	str             = str_buf[which_array++];
	str[0]          = '\0';
	cdesc[i].string = str;
	if(no_data){
	    if(cdesc[i].ctype == iMessNo)
	      sprintf(str, "%ld", msgno);
	    else if(no_data < 2 && cdesc[i].ctype == iSubject)
	      sprintf(str, "%-*.*s", width, width,
			  "[ No Message Text Available ]");
	}
	else{

	    switch(cdesc[i].ctype){
	      case iStatus:
		to_us = (cache->flagged) ? '*' : ' ';
		for(addr = envelope->to; addr && to_us == ' '; addr=addr->next)
		  if(address_is_us(addr, ps_global))
		    to_us = '+';

#ifdef	LATER
    if(to_us == ' '){				/* look for reset-to */
	char    *fields[2], *resent, *p;
	ADDRESS *resent_addr = NULL;

	fields[0] = "Resent-To";
	fields[1] = NULL;
	resent = xmail_fetchheader_lines(stream,mn_m2raw(msgmap,msgno),fields);
	if(resent){
	    if(p = strchr(resent, ':')){
		for(++p; *p && isspace((unsigned char)*p); p++)
		  ;

		removing_trailing_white_space(p);
		rfc822_parse_adrlist(&resent_addr, p, ps_global->maildomain);
		for(addr = resent_addr; addr && to_us == ' '; addr=addr->next)
		  if(address_is_us(addr, ps_global))
		    to_us = '+';

		mail_free_address(&resent_addr);
	    }

	    fs_give((void **)&resent);
	}
    }
#endif

		sprintf(str, "%c %s", to_us, status_string(stream, cache));
		break;

	      case iFStatus:
	       {char new, answered, deleted, flagged;

		to_us = ' ';
		for(addr = envelope->to; addr && to_us == ' '; addr=addr->next)
		  if(address_is_us(addr, ps_global))
		    to_us = '+';
		new = answered = deleted = flagged = ' ';
		if(cache && !ps_global->nr_mode){
		    if(!cache->seen &&
		      (!stream
		       || !IS_NEWS(stream)
		       || (cache->recent&&F_ON(F_FAKE_NEW_IN_NEWS,ps_global))))
		      new = 'N';

		    if(cache->answered)
		      answered = 'A';

		    if(cache->deleted)
		      deleted = 'D';

		    if(cache->flagged)
		      flagged = '*';
		}

		sprintf(str, "%c %c%c%c%c", to_us,
			flagged, new, answered, deleted);
	       }
		break;

	      case iMessNo:
		sprintf(str, "%ld", msgno);
		break;

	      case iDate:
		parse_date(envelope->date, &d);
		sprintf(str, "%s %2d", month_abbrev(d.month), d.day);
		break;

	      case iFromTo:
		addr = envelope->to ? envelope->to
					 : envelope->cc ? envelope->cc : NULL;
		field = envelope->to ? "To" : envelope->cc ? "Cc" : NULL;
		if(addr && (!envelope->from ||
			   address_is_us(envelope->from, ps_global))){

		    if(width > 4)
		      set_index_addr(stream, mn_m2raw(msgmap, msgno),
				     field, addr, "To: ", width, str);
		    else{
			strcpy(str, "To: ");
			str[width] = '\0';
		    }
		    
		    break;
		}

		/*
		 * Note: Newsgroups won't work over imap so we won't get
		 * it in that case, and we'll use from.  We don't want to
		 * waste an rtt getting it.
		 */
		if(!addr && envelope->newsgroups
		       && (!envelope->from
			   || address_is_us(envelope->from, ps_global))){
		    if(width > 4)
		      sprintf(str, "To: %-*.*s", width-4, width-4,
			      envelope->newsgroups);
		    else{
			strcpy(str, "To: ");
			str[width] = '\0';
		    }

		    break;
		}
		/* ELSE fall thru and act like a "From" */

	      case iFrom:
		set_index_addr(stream, mn_m2raw(msgmap, msgno),
			       "From", envelope->from, NULL, width, str);
		break;

	      case iTo:
		addr = envelope->to ? envelope->to
					 : envelope->cc ? envelope->cc : NULL;
		field = envelope->to ? "To" : envelope->cc ? "Cc" : NULL;
		if(!set_index_addr(stream, mn_m2raw(msgmap, msgno),
				   field, addr, NULL, width, str)
		   && envelope->newsgroups)
		  sprintf(str, "%-*.*s", width, width, envelope->newsgroups);

		break;

	      case iSender:
		set_index_addr(stream, mn_m2raw(msgmap, msgno),
			       "Sender", envelope->sender, NULL, width, str);
		break;

	      case iSize:
		if(!IS_NEWS(stream)){
		    if(cache->rfc822_size < 100000){
			s_tmp = comatose(cache->rfc822_size);
			sprintf(str, "(%s)", s_tmp);
		    }
		    else if(cache->rfc822_size < 10000000){
			s_tmp = comatose(cache->rfc822_size/1000);
			sprintf(str, "(%sK)", s_tmp);
		    }
		    else
		      strcpy(str, "(BIG!)");
		}

		break;

	      case iDescripSize:
		if(!IS_NEWS(stream) && body){
		  switch(body->type){
		    case TYPETEXT:
		      if(cache->rfc822_size < 6000)
			strcpy(str, "(short  )");
		      else if(cache->rfc822_size < 25000)
			strcpy(str, "(medium )");
		      else if(cache->rfc822_size < 100000)
			strcpy(str, "(long   )");
		      else
			strcpy(str, "(huge   )");

		      break;

		    case TYPEMULTIPART:
		      if(strucmp(body->subtype, "MIXED") == 0){
			switch(body->contents.part->body.type){
			  case TYPETEXT:
			    if(body->contents.part->body.size.bytes < 6000)
			      strcpy(str, "(short+ )");
			    else if(body->contents.part->body.size.bytes < 25000)
			      strcpy(str, "(medium+)");
			    else if(body->contents.part->body.size.bytes < 100000)
			      strcpy(str, "(long+  )");
			    else
			      strcpy(str, "(huge+  )");
			    break;

			  default:
			    strcpy(str, "(multi  )");
			    break;
			}
		      }
		      else if(strucmp(body->subtype, "DIGEST") == 0)
			strcpy(str, "(digest )");
		      else if(strucmp(body->subtype, "ALTERNATIVE") == 0)
			strcpy(str, "(mul/alt)");
		      else if(strucmp(body->subtype, "PARALLEL") == 0)
			strcpy(str, "(mul/par)");
		      else
		        strcpy(str, "(multi  )");

		      break;

		    case TYPEMESSAGE:
		      strcpy(str, "(message)");
		      break;

		    case TYPEAPPLICATION:
		      strcpy(str, "(applica)");
		      break;

		    case TYPEAUDIO:
		      strcpy(str, "(audio  )");
		      break;

		    case TYPEIMAGE:
		      strcpy(str, "(image  )");
		      break;

		    case TYPEVIDEO:
		      strcpy(str, "(video  )");
		      break;

		    default:
		      strcpy(str, "(other  )");
		      break;
		  }
		}
		break;

	      case iSubject:
		p = str;
		if(ps_global->nr_mode){
		    str[0] = ' ';
		    str[1] = '\0';
		    p++;
		    width--;
		}

		if(envelope->subject)
		  istrncpy(p,
			   (char*)rfc1522_decode((unsigned char *)tmp_20k_buf,
						 envelope->subject, NULL),
			   width);

		break;
	    }
	}
    }

    *buffer = '\0';
    p = buffer;
    /*--- Put them all together ---*/
    for(i = 0; cdesc[i].ctype != iNothing; i++){
	width = cdesc[i].width;
	if(width == 0)
	  continue;

	/* space between columns */
	if(p > buffer){
	    *p++ = ' ';
	    *p = '\0';
	}

	if(cdesc[i].adjustment == Left)
	  sprintf(p, "%-*.*s", width, width, cdesc[i].string);
	else
	  sprintf(p, "%*.*s", width, width, cdesc[i].string);
	
	p += width;
    }

    /* Truncate it to be sure not too wide */
    buffer[min(ps_global->ttyo->screen_cols, i_cache_width())] = '\0';
    hline->id = line_hash(buffer);
    dprint(9, (debugfile, "Returning %p -> <%s (%d), %ld>\n",
	       hline, buffer, strlen(buffer), hline->id));
    return(hline);
}


int
set_index_addr(stream, msgno, field, addr, prefix, width, s)
    MAILSTREAM *stream;
    long	msgno;
    char       *field;
    ADDRESS    *addr;
    char       *prefix;
    int		width;
    char       *s;
{
    ADDRESS *atmp;

    for(atmp = addr; stream && atmp; atmp = atmp->next)
      if(atmp->host && atmp->host[0] == '.'){
	  char *p, *h, *fields[2];

	  fields[0] = field;
	  fields[1] = NULL;
	  if(h = xmail_fetchheader_lines(stream, msgno, fields)){
	      /* skip "field:" */
	      for(p = h + strlen(field) + 1;
		  *p && isspace((unsigned char)*p); p++)
		;

	      while(width--)
		if(*p == '\015' || *p == '\012')
		  p++;				/* skip CR LF */
		else if(!*p)
		  *s++ = ' ';
		else
		  *s++ = *p++;

	      *s = '\0';			/* tie off return string */
	      fs_give((void **) &h);
	      return(TRUE);
	  }
	  /* else fall thru and display what c-client gave us */
      }

    if(addr && !addr->next		/* only one address */
       && addr->host			/* not group syntax */
       && addr->personal){		/* there is a personal name */
	char *dummy = NULL;
	int   l;

	if(l = prefix ? strlen(prefix) : 0)
	  strcpy(s, prefix);

	istrncpy(s + l,
		 (char *) rfc1522_decode((unsigned char *)tmp_20k_buf,
					 addr->personal, &dummy),
		 width - l);
	if(dummy)
	  fs_give((void **)&dummy);

	return(TRUE);
    }
    else if(addr){
	char *a_string;
	int   l;

	a_string = addr_list_string(addr, NULL, 0);
	if(l = prefix ? strlen(prefix) : 0)
	  strcpy(s, prefix);

	istrncpy(s + l, a_string, width - l);

	fs_give((void **)&a_string);
	return(TRUE);
    }

    return(FALSE);
}



long
line_hash(s)
     char *s;
{
    register long xsum = 0L;

    while(*s)
      xsum = ((((xsum << 4) & 0xffffffff) + (xsum >> 24)) & 0x0fffffff) + *s++;

    return(xsum ? xsum : 1L);
}


/*-----
  The following are used to report on sorting progress to the user

This code is not ideal as it varies depending on how the mail driver
performs with respect to fetching envelopes.  For some c-client drivers
the initial fetch of the envelope is expensive and it is cached after
that. For others every fetch is expensive.  The code here deals with
with the case where only the first fetch is expensive. It reports
progress based on how many messages have been fetched, assuming the sort
will complete very rapidly when all the messages have been fetched. 

There is a bitmap that keeps track of which messages have been
fetched that is used to keep an accurate count of the number of messages
that have been fetched. The sort algorithm fetches envelopes in a random
order and possibly many times. 

For c-client drivers for which every fetch envelope is expensive, this
algorithm will report that the sort is 100% complete once all the envelopes
have been fetched at least once.
  ----*/

#ifndef DOS
/*
 * Bitmap for keeping track of messages fetched so progress of
 * sort can be reported 
 */
static unsigned short *fetched_map;
static long	       fetched_count;

#define  GET_FETCHED_MAP(x)  (fetched_map[(x-1)/16] & (0x1 << ((x-1) & 0xf)))
#define  SET_FETCHED_MAP(x)  (fetched_map[(x-1)/16] |= (0x1 << ((x-1) & 0xf)))


/*
 * Return value for use by progress bar.
 */
int
percent_sorted()
{
    return((fetched_count*100) / mn_get_total(ps_global->msgmap));
}
#endif	/* !DOS */


/*----------------------------------------------------------------------
  Compare function for sorting on subjects. Ignores case, space and "re:"
  ----*/
int
compare_subjects(a, b)
    const QSType *a, *b;
{
    char *suba, *subb;
    long *mess_a = (long *)a, *mess_b = (long *)b;
    int   diff, res;
    long  mdiff;

    if(ps_global->intr_pending)
      longjmp(jump_past_qsort, 1);

    suba = get_sub(*mess_a);
    subb = get_sub(*mess_b);

    diff = strucmp(suba, subb);

    if(diff == 0)
      mdiff = *mess_a - *mess_b;

    /* convert to int */
    res = diff != 0 ? diff :
           mdiff != 0L ? (mdiff > 0L ? 1 : -1) : 0;
    return(mn_get_revsort(ps_global->msgmap) ? -res : res);
}


/*----------------------------------------------------------------------
   Get the subject of a message suitable for sorting.  This removes
 all re[*]: or [ from the beginning of a message.
 ----*/
char *
get_sub(mess)
    long mess;
{
    ENVELOPE *e;
    char *subj, *s, *s3, *ss, *dummy = NULL;
    int   l;

    if(!(ss = subject_cache_ent(mess))){
	e = mail_fetchstructure(ps_global->mail_stream, mess, NULL);
	if(e && e->subject) {
	    /* ---- Deal with any necessary RFC 1522 decoding ----*/
	    subj = (char *)rfc1522_decode((unsigned char *)tmp_20k_buf,
					  e->subject, &dummy);
	    if(dummy)
	      fs_give((void **)&dummy);
 
	    /* ---- Clean junk off the front of the subject ----*/
	    for(s = subj; *s && (isspace((unsigned char)*s) || *s == '['); s++)
	      /* do nothing */;

	    if((*s == 'R' || *s == 'r') && (*(s+1) == 'E' || *(s+1) == 'e')
	       && (*(s+2) == ':' || *(s+2) == '[')){
		s += 3;
		if(*(s+2) == '['){
		    while(*s && *s != ':')
		      s++;

		    s++;
		}
	    }

	    while(*s && (isspace((unsigned char)*s) || *s == '['))
	      s++;

	    if(*(ss = subject_cache_add(mess, s))){
		/*----- Now, truncate junk off the back end of the subject---*/
		for(s3 = NULL, s = ss; *s; s++)	/* blast ws and ']' */
		  s3 = (!isspace((unsigned char)*s) && *s != ']')
					      ? NULL : (!s3) ? s : s3;

		if(s3)
		  *s3 = '\0';

		if((l=(s3 ? s3 : s)-ss) > 5 && !strucmp(ss+l-5,"(fwd)"))
		  ss[l-5] = '\0';

		for(s3 = NULL, s = ss; *s; s++)	/* blast ws and ']' */
		  s3 = (!isspace((unsigned char)*s) && *s != ']')
					      ? NULL : (!s3) ? s : s3;

		if(s3)
		  *s3 = '\0';
	    }
	}
	else
	  ss = subject_cache_add(mess, "");

	dprint(9, (debugfile, "SUB-GET-%s-GET-SUB\n", ss));
    }
    else{
	dprint(9, (debugfile, "SUB-HIT-%s-HIT-SUB\n", ss));
    }

#ifndef DOS
    if(fetched_map != NULL && !GET_FETCHED_MAP(mess)) {
        SET_FETCHED_MAP(mess);
        fetched_count++;
    }
#endif

    ALARM_BLIP();

    return(ss);
}


/*----------------------------------------------------------------------
   Compare the From: fields for sorting. Ignore case.  Only consider the
   mailbox portion of the address (part left of @).
   ----*/
int 
compare_from(a, b)
    const QSType *a, *b;
{
    long     *mess_a = (long *)a, *mess_b = (long *)b;
    ENVELOPE *e;
    char      froma[200], fromb[200];
    int       diff, res;

    if(ps_global->intr_pending)
      longjmp(jump_past_qsort, 1);

    e = mail_fetchstructure(ps_global->mail_stream, *mess_a, NULL);
    if(e == NULL || e->from == NULL || e->from->mailbox == NULL)  
      froma[0] = '\0';
    else
      strncpy(froma, e->from->mailbox, sizeof(froma) - 1);

    froma[sizeof(froma) - 1] = '\0';

    e = mail_fetchstructure(ps_global->mail_stream, *mess_b, NULL);
    if(e == NULL || e->from == NULL || e->from->mailbox == NULL)  
      fromb[0] = '\0';
    else
      strncpy(fromb, e->from->mailbox, sizeof(fromb) - 1);

    fromb[sizeof(fromb) - 1] = '\0';

#ifndef DOS
    if(!GET_FETCHED_MAP(*mess_a)){
        SET_FETCHED_MAP(*mess_a);
        fetched_count++;
    }

    if(!GET_FETCHED_MAP(*mess_b)){
        SET_FETCHED_MAP(*mess_b);
        fetched_count++;
    }
#endif

    ALARM_BLIP();

    diff = strucmp(froma, fromb);
    if(diff == 0){
	long mdiff;

        mdiff = *mess_a - *mess_b;  /* arrival order */
	/* convert to int */
	res = mdiff != 0L ? (mdiff > 0L ? 1 : -1) : 0;
    }
    else
      res = diff;
    
    return(mn_get_revsort(ps_global->msgmap) ? -res : res);
} 


/*----------------------------------------------------------------------
   Compare the To: fields for sorting. Ignore case. Use 1st to.
   ----*/
int 
compare_to(a, b)
    const QSType *a, *b;
{
    long     *mess_a = (long *)a, *mess_b = (long *)b;
    ENVELOPE *e;
    char      toa[200], tob[200];
    int       diff, res;

    if(ps_global->intr_pending)
      longjmp(jump_past_qsort, 1);

    e = mail_fetchstructure(ps_global->mail_stream, *mess_a, NULL);
    if(e == NULL || e->to == NULL || e->to->mailbox == NULL)  
      toa[0] = '\0';
    else
      strncpy(toa, e->to->mailbox, sizeof(toa) - 1);

    toa[sizeof(toa) - 1] = '\0';

    e = mail_fetchstructure(ps_global->mail_stream, *mess_b, NULL);
    if(e == NULL || e->to == NULL || e->to->mailbox == NULL)  
      tob[0] = '\0';
    else
      strncpy(tob, e->to->mailbox, sizeof(tob) - 1);

    tob[sizeof(tob) - 1] = '\0';

#ifndef DOS
    if(!GET_FETCHED_MAP(*mess_a)){
        SET_FETCHED_MAP(*mess_a);
        fetched_count++;
    }

    if(!GET_FETCHED_MAP(*mess_b)){
        SET_FETCHED_MAP(*mess_b);
        fetched_count++;
    }
#endif

    ALARM_BLIP();

    diff = strucmp(toa, tob);
    if(diff == 0){
	long mdiff;

        mdiff = *mess_a - *mess_b;  /* arrival order */
	/* convert to int */
	res = mdiff != 0L ? (mdiff > 0L ? 1 : -1) : 0;
    }
    else
      res = diff;
    
    return(mn_get_revsort(ps_global->msgmap) ? -res : res);
} 


/*----------------------------------------------------------------------
   Compare the Cc: fields for sorting. Ignore case. Use 1st cc.
   ----*/
int 
compare_cc(a, b)
    const QSType *a, *b;
{
    long     *mess_a = (long *)a, *mess_b = (long *)b;
    ENVELOPE *e;
    char      cca[200], ccb[200];
    int       diff, res;

    if(ps_global->intr_pending)
      longjmp(jump_past_qsort, 1);

    e = mail_fetchstructure(ps_global->mail_stream, *mess_a, NULL);
    if(e == NULL || e->cc == NULL || e->cc->mailbox == NULL)  
      cca[0] = '\0';
    else
      strncpy(cca, e->cc->mailbox, sizeof(cca) - 1);

    cca[sizeof(cca) - 1] = '\0';

    e = mail_fetchstructure(ps_global->mail_stream, *mess_b, NULL);
    if(e == NULL || e->cc == NULL || e->cc->mailbox == NULL)  
      ccb[0] = '\0';
    else
      strncpy(ccb, e->cc->mailbox, sizeof(ccb) - 1);

    ccb[sizeof(ccb) - 1] = '\0';

#ifndef DOS
    if(!GET_FETCHED_MAP(*mess_a)){
        SET_FETCHED_MAP(*mess_a);
        fetched_count++;
    }

    if(!GET_FETCHED_MAP(*mess_b)){
        SET_FETCHED_MAP(*mess_b);
        fetched_count++;
    }
#endif

    ALARM_BLIP();

    diff = strucmp(cca, ccb);
    if(diff == 0){
	long mdiff;

        mdiff = *mess_a - *mess_b;  /* arrival order */
	/* convert to int */
	res = mdiff != 0L ? (mdiff > 0L ? 1 : -1) : 0;
    }
    else
      res = diff;
    
    return(mn_get_revsort(ps_global->msgmap) ? -res : res);
} 


/*----------------------------------------------------------------------
   Compare dates (not arrival times, but time from Date header)
  ----*/
int
compare_message_dates(a, b)
    const QSType *a, *b;
{
    long        *mess_a = (long *)a, *mess_b = (long *)b;
    int          diff, res;
    long         mdiff;
    ENVELOPE    *e;
    MESSAGECACHE mc_a, mc_b;

    if(ps_global->intr_pending)
      longjmp(jump_past_qsort, 1);

    mdiff = *mess_a - *mess_b;
    res = mdiff != 0L ? (mdiff > 0L ? 1 : -1) : 0;

    mc_a.minutes   = mc_b.minutes = 0;		/* init interesting bits */
    mc_a.hours	   = mc_b.hours = 0;
    mc_a.day	   = mc_b.day = 0;
    mc_a.month	   = mc_b.month = 0;
    mc_a.year	   = mc_b.year = 0;
    mc_a.zhours	   = mc_b.zhours = 0;
    mc_a.zminutes  = mc_b.zminutes = 0;
    mc_a.zoccident = mc_b.zoccident = 0;

    e = mail_fetchstructure(ps_global->mail_stream, *mess_a, NULL);
    mc_a.valid = (e && e->date && mail_parse_date(&mc_a, e->date));

    e = mail_fetchstructure(ps_global->mail_stream, *mess_b, NULL);
    mc_b.valid = (e && e->date && mail_parse_date(&mc_b, e->date));

    if(!mc_a.valid)
      return((mn_get_revsort(ps_global->msgmap) ? -1 : 1)
	      * (mc_b.valid ? -1 : res));
    else if(!mc_b.valid)
      return(mn_get_revsort(ps_global->msgmap) ? -1 : 1);

    diff = compare_dates(&mc_a, &mc_b);

#ifndef DOS
    if(!GET_FETCHED_MAP(*mess_a)){
        SET_FETCHED_MAP(*mess_a);
        fetched_count++;
    }

    if(!GET_FETCHED_MAP(*mess_b)){
        SET_FETCHED_MAP(*mess_b);
        fetched_count++;
    }
#endif

    ALARM_BLIP();
    
    res = diff != 0 ? diff : res;
    return(mn_get_revsort(ps_global->msgmap) ? -res : res);
}

    

/*----------------------------------------------------------------------
  Compare size of messages for sorting
 ----*/
int
compare_size(a, b)
    const QSType *a, *b;
{
    long 	 *mess_a = (long *)a, *mess_b = (long *)b;
    long	  size_a, size_b, sdiff, mdiff;
    MESSAGECACHE *mc;
    int		  res;

    if(ps_global->intr_pending)
      longjmp(jump_past_qsort, 1);

    mail_fetchstructure(ps_global->mail_stream, *mess_a, NULL);
    mc = mail_elt(ps_global->mail_stream, *mess_a);
    size_a = mc != NULL ? mc->rfc822_size : -1L;

    mail_fetchstructure(ps_global->mail_stream, *mess_b, NULL);
    mc = mail_elt(ps_global->mail_stream, *mess_b);
    size_b = mc != NULL ? mc->rfc822_size : -1L;

#ifndef DOS
    if(!GET_FETCHED_MAP(*mess_a)){
        SET_FETCHED_MAP(*mess_a);
        fetched_count++;
    }

    if(!GET_FETCHED_MAP(*mess_b)){
        SET_FETCHED_MAP(*mess_b);
        fetched_count++;
    }
#endif    

    ALARM_BLIP();

    sdiff = size_a - size_b;
    if(sdiff == 0L)
      mdiff = *mess_a - *mess_b;

    /* convert to int */
    res = sdiff != 0L ? (sdiff > 0L ? 1 : -1) :
           mdiff != 0L ? (mdiff > 0L ? 1 : -1) : 0;
    return(mn_get_revsort(ps_global->msgmap) ? -res : res);
}


/*----------------------------------------------------------------------
  Compare raw message numbers 
 ----*/
int
compare_arrival(a, b)
    const QSType *a, *b;
{
    long *mess_a = (long *)a, *mess_b = (long *)b, mdiff;
    int   res;

    res = (mdiff = *mess_a - *mess_b) ? (mdiff > 0L ? 1 : -1) : 0;
    return(mn_get_revsort(ps_global->msgmap) ? -res : res);
}


#if	defined(DOS) && !defined(_WINDOWS)
static FILE *second_sort_file;	/* Can't afford an array with DOS */
static char *second_sort_file_name = NULL;
#else
static long *second_sort;	/* Array of message numbers */
#endif

/*----------------------------------------------------------------------
   Each message is in a subject group.  That group is indexed by the
   Date of its oldest member.  Sort first by that date, and within a
   single subject group, sort by Date of each message.
  ----*/
int
compare_subject_2(a, b)
    const QSType *a, *b;
{
    long       *mess_a = (long *)a, *mess_b = (long *)b;
    long        a1, b1;
    long        diff;
    int         res;

    if(ps_global->intr_pending)
      longjmp(jump_past_qsort, 1);

#if	defined(DOS) && !defined(_WINDOWS)
    /* There ought to be a better way than seeking all over 
       the disk, but too lazy at the moment */
    fseek(second_sort_file, *mess_a * sizeof(long), 0);
    fread((void *)&a1, sizeof(a1), (size_t)1, second_sort_file);
    fseek(second_sort_file, *mess_b * sizeof(long), 0);
    fread((void *)&b1, sizeof(b1), (size_t)1, second_sort_file);
#else
    a1 = second_sort[*mess_a];
    b1 = second_sort[*mess_b];
#endif
    
    diff = a1 - b1;

    /*
     * Note, secondary sort is by Date instead of by arrival time as
     * with most of the other compare functions.
     */
    res = diff != 0L ? (diff > 0L ? 1 : -1) : compare_message_dates(a, b);

    return((mn_get_revsort(ps_global->msgmap) && diff != 0L) ? -res : res);
}


    
/*----------------------------------------------------------------------
     Sort messages on subject, grouping subjects by date of oldest member

Args: sort -- The resulting sorted array of message numbers
      max_msgno -- The number of messages in this view
      total_max -- The largest possible message number

This is the OrderedSubjectSort style of subject sort akin to message threading
based on subject.  All the subjects are grouped together and then the 
groups are ordered based on the oldest message in each group. 

Here the groups of subjects are sorted.  Before calling this function,
the messages have been sorted by subject.

The second array is indexed by raw message numbers and contains the
the date (in seconds) of the oldest message in the same subject group.

This code could be used to calculate information for true
message threading.  For example, a data structure that had one entry
per unique subject.  The data structure would probably only need to contain
the *sorted* message number of the first message in the thread and the 
number of messages in the thread. That would be assuming that the sort
array is sorted in the appropriate order. 

On DOS where memory is expensive the secondary array is a disk file instead
of being in memory. (Though for many formats sorting on DOS will be 
extremely painful until some local caching of sort terms is invented.)
 ----*/
void
second_subject_sort(sort, max_msgno, total_max)
    long *sort, max_msgno, total_max;
{
    char  *sub_group, *sub;
    long   start_sub_group, m, s, t, min_sub_group_date;
    long   for_zero_dates = 0L;
    int    check_again;

    if(max_msgno < 1 || total_max < 1)
      return; 

#if	defined(DOS) && !defined(_WINDOWS)
    second_sort_file_name = temp_nam(NULL, "sf");
    second_sort_file      = fopen(second_sort_file_name, "wb+");
#else
    second_sort      = (long *)fs_get(sizeof(long) * (size_t)(total_max + 1));
#endif

    /* init for loop */
    sub_group          = get_sub(sort[1]);
    start_sub_group    = 1;
    min_sub_group_date = seconds_since_epoch(sort[1]);
    /*
     * This is so that unparseable dates won't all be the same.  That would
     * make it look like they're all in the same subject group.
     */
    if(min_sub_group_date == 0){
	for_zero_dates += 10;
	min_sub_group_date = for_zero_dates;
    }

    /*
     * Create the second array of subject groups.  The sort array had to be
     * sorted by subject before this function was called, so that each
     * subject group would be contiguous in the array.  (Didn't actually have
     * to be sorted, just contiguous subject groups in any order.)
     */
    for(s = 2; s <= max_msgno; s++){
	sub = get_sub(sort[s]);
        if(strucmp(sub_group, sub)){ /* new subject */
	    /* record the previous group */
#if	!defined(DOS) || defined(_WINDOWS)
	    /*
	     * If two different groups both have the same min_sub_group_date,
	     * then they'll get mushed together when we sort.  We want to make
	     * sure that doesn't happen, so we need to check if this date
	     * is already recorded somewhere in second_sort and change it
	     * if it is.
	     *
	     * (Too expensive for DOS, count on it being very rare.)
	     */
	    check_again = 1;
	    while(check_again){
		check_again = 0;
		for(m = 1; m < start_sub_group; m++){
		    if(min_sub_group_date == second_sort[sort[m]]){
			min_sub_group_date++;
			check_again = 1;
			break;
		    }
		}
	    }
#endif
            for(t = start_sub_group; t < s; t++){
#if	defined(DOS) && !defined(_WINDOWS)
                fseek(second_sort_file, sort[t] * sizeof(long), 0);
                fwrite((void *)&min_sub_group_date,
                       sizeof(min_sub_group_date),
                       (size_t)1,  second_sort_file);
#else
		second_sort[sort[t]] = min_sub_group_date;
#endif                
	    }

	    /* reset for next subject */
            start_sub_group     = s;
            sub_group           = sub;
        }

	if(s > start_sub_group)
	  min_sub_group_date =
		min(min_sub_group_date, seconds_since_epoch(sort[s]));
	else /* new subject */
	  min_sub_group_date = seconds_since_epoch(sort[s]);

	if(min_sub_group_date == 0){
	    for_zero_dates += 10;
	    min_sub_group_date = for_zero_dates;
	}
    }

    /* record final group */
#if	!defined(DOS) || defined(_WINDOWS)
	check_again = 1;
	while(check_again){
	    check_again = 0;
	    for(m = 1; m < start_sub_group; m++){
		if(min_sub_group_date == second_sort[sort[m]]){
		    min_sub_group_date++;
		    check_again = 1;
		    break;
		}
	    }
	}
#endif

    for(t = start_sub_group; t < s; t++){
#if	defined(DOS) && !defined(_WINDOWS)
        fseek(second_sort_file, sort[t] * sizeof(long), 0);
        fwrite((void *)&min_sub_group_date, sizeof(min_sub_group_date),
                       (size_t)1,  second_sort_file);
#else
	second_sort[sort[t]] = min_sub_group_date;
#endif
    }

#ifdef DEBUG
    for(t = 1; t <= max_msgno; t++) 
      dprint(9, (debugfile, "Second_sort[%3ld] is %3ld\n", t, second_sort[t]));
#endif

    /* Actually perform the sort */
    qsort(sort+1, (size_t)max_msgno, sizeof(long), compare_subject_2);

    /* Clean up */
#if	defined(DOS) && !defined(_WINDOWS)
    fclose(second_sort_file);
    unlink(second_sort_file_name);
    fs_give((void **)&second_sort_file_name);
#else    
    fs_give((void **)&second_sort);
#endif
}


/*
 * This is just used to help with the OrderedSubjSort.
 * The argument is a raw message number and the return value is the number
 * of seconds between the start of the BASEYEAR (1969) and the date in the
 * Date header.  Returns 0 if date can't be parsed or no date.
 */
long
seconds_since_epoch(msgno)
long msgno;
{
    ENVELOPE    *e;
    MESSAGECACHE mc;
    long seconds = 0L;
    int y, m;

#define MINUTE       (60L)
#define HOUR         (60L * MINUTE)
#define DAY          (24L * HOUR)
#define MONTH(mo,yr) ((long)days_in_month(mo,yr) * DAY)
#define YEAR(yr)     (((!((yr)%4L) && ((yr)%100L)) ? 366L : 365L) * DAY)

    e = mail_fetchstructure(ps_global->mail_stream, msgno, NULL);
    if(e == NULL || e->date == NULL)
      return(0L);

    if(!mail_parse_date(&mc, e->date))
      return(0L);

    convert_to_gmt(&mc);

    for(y = 0; (unsigned)y < mc.year; y++)
      seconds += YEAR(y+BASEYEAR);

    for(m = 1; (unsigned)m < mc.month; m++)
      seconds += MONTH(m, mc.year + BASEYEAR);
    
    seconds +=
	((mc.day-1) * DAY + mc.hours * HOUR + mc.minutes * MINUTE + mc.seconds);

    return(seconds);
}


/*----------------------------------------------------------------------
    Sort the current folder into the order set in the msgmap

Args: defer_on_intr   -- If we get interrupted before finishing, set the
			  deferred variable.
      default_so      -- If we get interrupted before finishing, set the
			  sort order to default_so.
      default_reverse -- If we get interrupted before finishing, set the
			  revsort to default_reverse.
    
    The idea of the deferred sort is to let the user interrupt a long sort
    and have a chance to do a different command, such as a sort by arrival
    or a Goto.  The next newmail call will increment the deferred variable,
    then the user may do a command, then the newmail call after that
    causes the sort to happen if it is still needed.
  ----*/
void
sort_current_folder(defer_on_intr, default_so, default_reverse)
    int defer_on_intr;
    SortOrder default_so;
    int default_reverse;
{
    long        i, *sort, *saved_sort, total = mn_get_total(ps_global->msgmap);
    MSGNO_S    *sortmap = NULL;
    SortOrder   so;
    char        sort_msg[101];
    int         skip_qsort = 0, we_cancel = 0, rev, is_default_order;

    so = mn_get_sort(ps_global->msgmap);	/* current sort order */
    rev = mn_get_revsort(ps_global->msgmap);	/* currently reverse? */
    /*
     * If is_default_order, that means we aren't changing the sort order,
     * we're just sorting because we may have gotten new mail or unexcluded
     * some mail that was hidden.
     */
    is_default_order = (so == default_so && rev == default_reverse);
    if(!ps_global->msgmap || !(sort = ps_global->msgmap->sort))
      return;

    dprint(2, (debugfile, "Sorting by %s%s\n", sort_name(so),
	       mn_get_revsort(ps_global->msgmap) ? "/reverse" : ""));

    /*
     * translate the selected numbers into an array of raw numbers
     * temporarily, then translate it back after the sort so the
     * same physical messages are selected...
     */
    mn_init(&sortmap, 0L);
    mn_set_cur(sortmap, mn_m2raw(ps_global->msgmap,
				 mn_first_cur(ps_global->msgmap)));
    while((i = mn_next_cur(ps_global->msgmap)) > 0L)
      mn_add_cur(sortmap, mn_m2raw(ps_global->msgmap, i));

    if(so == SortArrival){
	/*
	 * BEWARE: "exclusion" may leave holes in the unsorted sort order
	 * so we have to do a real sort if that is the case.
	 */
	if(any_lflagged(ps_global->msgmap, MN_EXLD))
	  qsort(sort+1, (size_t) total, sizeof(long), compare_arrival);
	else
	  for(i = 1L; i <= total; i++)
	    sort[i] = mn_get_revsort(ps_global->msgmap) ? (total + 1 - i) : i;
    }
    else{
        if(so == SortSubject || so == SortSubject2)	/* nmsgs cuz MN_EXLD */
	  init_subject_cache(ps_global->mail_stream->nmsgs);

        /*========= Sorting ================================================*/
#ifndef DOS
        /*--- fetched map for keep track of progress of sort ----*/
        fetched_count = 0;
        fetched_map   = (unsigned short *)fs_get(
                         (ps_global->mail_stream->nmsgs/16 + 1)
                                              * sizeof(unsigned short));
	memset((void *)fetched_map, 0,
	    (ps_global->mail_stream->nmsgs/16 + 1) * sizeof(unsigned short));
#endif


#ifdef DOS
	sprintf(sort_msg, "Sorting \"%.90s\"", ps_global->cur_folder);
	we_cancel = busy_alarm(1, sort_msg, NULL, 1);
#else
	sprintf(sort_msg, "Sorting \"%.90s\"", ps_global->cur_folder);
	we_cancel = busy_alarm(1, sort_msg, percent_sorted, 1);

	/*
	 * Interruptible sorting is only available under unix where we
	 * can rely on the tty driver to deliver the SIGINT.  If we
	 * can safely work out a similar scheme under windows/max, this
	 * #ifdef will have to get rearranged...
	 */
	/* save array in case of interrupt */
	saved_sort
	    = (long *)fs_get(ps_global->msgmap->sort_size * sizeof(long));
	for(i = 1L; i <= total; i++)
	  saved_sort[i] = sort[i];

	if(setjmp(jump_past_qsort)){
	    intr_handling_off();
	    if(we_cancel)
	      cancel_busy_alarm(-1);

	    we_cancel = 0;
	    skip_qsort = 1;
	    /* restore default SortOrder */
	    mn_set_sort(ps_global->msgmap, default_so);
	    mn_set_revsort(ps_global->msgmap, default_reverse);
	    /* restore sort array */
	    for(i = 1L; i <= total; i++)
	      sort[i] = saved_sort[i];

	    fs_give((void **)&saved_sort); 
	}
#endif

	if(!skip_qsort){
#ifndef	DOS
	    intr_handling_on();
#endif

	    qsort(sort + 1, (size_t) total, sizeof(long),
		  so == SortSubject  ? compare_subjects :
		   so == SortFrom     ? compare_from :
		    so == SortTo       ? compare_to :
		     so == SortCc       ? compare_cc :
		      so == SortDate     ? compare_message_dates :
		       so == SortSubject2 ? compare_subjects:
                                         compare_size);

	    /*---- special case -- reorder the groups of subjects ------*/
	    if(so == SortSubject2) 
	      second_subject_sort(sort, total, ps_global->mail_stream->nmsgs);
#ifndef	DOS
	    intr_handling_off();
	    fs_give((void **)&saved_sort); 
#endif
	}

	if(we_cancel)
	  cancel_busy_alarm(1);

	if(skip_qsort)
	  q_status_message3(SM_ORDER, 3, 3, "Sort cancelled%s%s%s",
	      is_default_order ? "" : "; now sorting by ",
	      is_default_order ? "" : sort_name(default_so),
	      is_default_order ? "" : default_reverse ? "/reverse" : "");

#ifndef DOS
        fs_give((void **)&fetched_map); 
#endif
	clear_subject_cache();
    }

    if(skip_qsort)
      ps_global->unsorted_newmail = 1;
    else{
	ps_global->sort_is_deferred = 0;
	ps_global->unsorted_newmail = 0;
    }

    /*
     * restore the selected array of message numbers.  No expunge could
     * have happened yet, so we don't worry about raw2m returning 0...
     */
    mn_reset_cur(ps_global->msgmap, mn_raw2m(ps_global->msgmap,
					     mn_first_cur(sortmap)));
    while((i = mn_next_cur(sortmap)) > 0L)
      mn_add_cur(ps_global->msgmap, mn_raw2m(ps_global->msgmap, i));

    mn_give(&sortmap);
}


/*----------------------------------------------------------------------
    Map sort types to names
  ----*/
char *    
sort_name(so)
  SortOrder so;
{
    /*
     * Make sure the first upper case letter of any new sort name is
     * unique.  The command char and label for sort selection is 
     * derived from this name and its first upper case character.
     * See mailcmd.c:select_sort().
     */
    return((so == SortArrival)  ? "Arrival" :
	    (so == SortDate)	 ? "Date" :
	     (so == SortSubject)  ? "Subject" :
	      (so == SortCc)	   ? "Cc" :
	       (so == SortFrom)	    ? "From" :
		(so == SortTo)	     ? "To" :
		 (so == SortSize)     ? "siZe" :
		  (so == SortSubject2) ? "OrderedSubj" :
					  "BOTCH");
}



/*----------------------------------------------------------------------
    initialize the subject cache
  ----*/
void
init_subject_cache(n)
    long n;
{
    scache = (struct scache *) fs_get(sizeof(struct scache));
    memset((void *)scache, 0, sizeof(struct scache));
    scache->size = n;
#if	defined(DOS) && !defined(_WINDOWS)
    scache->cname = temp_nam(NULL, "sc");
    scache->cfile = fopen(scache->cname, "wb+");
    /* BUG? go into non-caching mode if failure below? */
    while(n-- >= 0L){
	if(fwrite(&scache->ent[1], sizeof(scache_ent), (size_t)1,
		  scache->cfile) != 1){
	    sprintf(tmp_20k_buf, "subject cache: %s",
		    error_description(errno));
	    fatal(tmp_20k_buf);
	}
    }
#else
    scache->ent  = (char **)fs_get((size_t)(n+1L) * sizeof(char *));
    memset((void *)scache->ent, 0, (size_t)(n+1L) * sizeof(char *));
#endif
}


/*----------------------------------------------------------------------
    return the subject of the given number if it's in the cache, else NULL
  ----*/
char *
subject_cache_ent(n)
    long n;
{
#if	defined(DOS) && !defined(_WINDOWS)
    char *buf;
    int   i;

    /* if the one we want's in core, return it */
    buf = (scache->msgno[i=0] == n || scache->msgno[i=1] == n)
	    ? scache->ent[i].buf : NULL;

    /* else pick one of the slots, and fetch n's subject off disk */
    if(!buf){
	i = subject_cache_slot(n);
	buf = scache->ent[i].used ? scache->ent[i].buf : NULL;
    }

    scache->last = i;
    return(buf);
#else
    return(scache->ent[n]);
#endif
}


/*----------------------------------------------------------------------
    add the given subject to the cache
  ----*/
char *
subject_cache_add(n, s)
    long n;
    char *s;
{
#if	defined(DOS) && !defined(_WINDOWS)
    int i;
    
    if(scache->msgno[i=0] != n && scache->msgno[i=1] != n)
      i = subject_cache_slot(n);

    scache->last = i;
    scache->ent[i].used = 1;
    scache->msgno[i] = n;
    strncpy(scache->ent[i].buf, s, SUB_CACHE_LEN-1);
    scache->ent[i].buf[SUB_CACHE_LEN-1] = '\0';
    return(scache->ent[i].buf);
#else
    return(scache->ent[n] = cpystr(s));
#endif
}


/*----------------------------------------------------------------------
    pick the next slot, and fetch n's subject off disk
  ----*/
int
subject_cache_slot(n)
    long n;
{
#if	defined(DOS) && !defined(_WINDOWS)
    int i;

    i = scache->last ? 0 : 1;
    if(scache->msgno[i]){
	fseek(scache->cfile, scache->msgno[i] * sizeof(scache_ent), 0);
	fwrite(&scache->ent[i], sizeof(scache_ent), (size_t)1,
	       scache->cfile);
    }

    scache->msgno[i] = n;
    if(fseek(scache->cfile, scache->msgno[i] * sizeof(scache_ent), 0)
       || fread(&scache->ent[i], sizeof(scache_ent), (size_t)1,
		scache->cfile) != 1){
	sprintf(tmp_20k_buf, "Can't access subject cache: ",
		error_description(errno));
	fatal(tmp_20k_buf);
    }

    return(i);
#endif
}


/*----------------------------------------------------------------------
    flush the subject cache
  ----*/
void
clear_subject_cache()
{
    if(scache){
#if	defined(DOS) && !defined(_WINDOWS)
	fclose(scache->cfile);
	unlink(scache->cname);
	fs_give((void **)&scache->cname);
#else
	for(; scache->size; scache->size--)
	  if(scache->ent[scache->size])
	    fs_give((void **)&scache->ent[scache->size]);

	if(scache->ent)
	  fs_give((void **)&scache->ent);

#endif
	fs_give((void **)&scache);
    }
}



/*
 *           * * *  Message number management functions  * * *
 */


/*----------------------------------------------------------------------
  Initialize a message manipulation structure for the given total

   Accepts: msgs - pointer to message manipulation struct
	    n - number to test
   Returns: true if n is in selected array, false otherwise

  ----*/
void
msgno_init(msgs, tot)
     MSGNO_S **msgs;
     long      tot;
{
    long   slop = (tot + 1L) % 64;
    size_t len;

    if(!msgs)
      return;

    if(!(*msgs)){
	(*msgs) = (MSGNO_S *)fs_get(sizeof(MSGNO_S));
	memset((void *)(*msgs), 0, sizeof(MSGNO_S));
    }

    (*msgs)->sel_cur  = 0L;
    (*msgs)->sel_cnt  = 1L;
    (*msgs)->sel_size = 8L;
    len		      = (size_t)(*msgs)->sel_size * sizeof(long);
    if((*msgs)->select)
      fs_resize((void **)&((*msgs)->select), len);
    else
      (*msgs)->select = (long *)fs_get(len);

    (*msgs)->select[0] = (tot) ? 1L : 0L;

    (*msgs)->sort_size = (tot + 1L) + (64 - slop);
    len		       = (size_t)(*msgs)->sort_size * sizeof(long);
    if((*msgs)->sort)
      fs_resize((void **)&((*msgs)->sort), len);
    else
      (*msgs)->sort = (long *)fs_get(len);

    memset((void *)(*msgs)->sort, 0, len);
    for(slop = 1L ; slop <= tot; slop++)	/* reusing "slop" */
      (*msgs)->sort[slop] = slop;

    (*msgs)->max_msgno    = tot;
    (*msgs)->sort_order   = ps_global->def_sort;
    (*msgs)->reverse_sort = ps_global->def_sort_rev;
    (*msgs)->flagged_hid  = 0L;
    (*msgs)->flagged_exld = 0L;
    (*msgs)->flagged_tmp  = 0L;
}



/*----------------------------------------------------------------------
 Increment the current message number

   Accepts: msgs - pointer to message manipulation struct
  ----*/
void
msgno_inc(stream, msgs)
     MAILSTREAM *stream;
     MSGNO_S    *msgs;
{
    long i;

    if(!msgs || mn_get_total(msgs) < 1L)
      return;

    for(i = msgs->select[msgs->sel_cur] + 1; i <= mn_get_total(msgs); i++){
	if(!get_lflag(stream, msgs, i, MN_HIDE)){
	    (msgs)->select[((msgs)->sel_cur)] = i;
	    break;
	}
    }
}
 


/*----------------------------------------------------------------------
  Decrement the current message number

   Accepts: msgs - pointer to message manipulation struct
  ----*/
void
msgno_dec(stream, msgs)
     MAILSTREAM *stream;
     MSGNO_S     *msgs;
{
    long i;

    if(!msgs || mn_get_total(msgs) < 1L)
      return;

    for(i = (msgs)->select[((msgs)->sel_cur)] - 1L; i >= 1L; i--){
	if(!get_lflag(stream, msgs, i, MN_HIDE)){
	    (msgs)->select[((msgs)->sel_cur)] = i;
	    break;
	}
    }
}



/*----------------------------------------------------------------------
  Got thru the message mapping table, and remove messages with DELETED flag

   Accepts: stream -- mail stream to removed message references from
	    msgs -- pointer to message manipulation struct
	    f -- flags to use a purge criteria
  ----*/
void
msgno_exclude(stream, msgs)
     MAILSTREAM *stream;
     MSGNO_S     *msgs;
{
    long	  i, j;

    if(!msgs || msgs->max_msgno < 1L)
      return;

    /*
     * With 3.91 we're using a new strategy for finding and operating
     * on all the messages with deleted status.  The idea is to do a
     * mail_search for deleted messages so the elt's "searched" bit gets
     * set, and then to scan the elt's for them and set our local bit
     * to indicate they're excluded...
     */
    (void)count_flagged(stream, "DELETED");

    for(i = 1L; i <= msgs->max_msgno; ){
	if(mail_elt(stream, mn_m2raw(msgs, i))->searched){
	    /*--- clear all flags to keep our counts consistent  ---*/
	    set_lflag(stream, msgs, i, (MN_HIDE|MN_SLCT), 0);
	    set_lflag(stream, msgs, i, MN_EXLD, 1); /* mark excluded */

	    /* --- erase knowledge in sort array (shift array down) --- */
	    for(j = i + 1; j <= msgs->max_msgno; j++)
	      msgs->sort[j-1] = msgs->sort[j];
/* BUG: should compress strings of deleted message numbers */

	    msgs->max_msgno = max(0L, msgs->max_msgno - 1L);
	    msgno_flush_selected(msgs, i);
	}
	else
	  i++;
    }

    /*
     * If we excluded away a zoomed display, unhide everything...
     */
    if(msgs->max_msgno > 0L && any_lflagged(msgs, MN_HIDE) >= msgs->max_msgno)
      for(i = 1L; i <= msgs->max_msgno; i++)
	set_lflag(stream, msgs, i, MN_HIDE, 0);
}



/*----------------------------------------------------------------------
  Got thru the message mapping table, and remove messages with given flag

   Accepts: stream -- mail stream to removed message references from
	    msgs -- pointer to message manipulation struct
	    f -- flags to use a purge criteria
  ----*/
void
msgno_include(stream, msgs)
     MAILSTREAM *stream;
     MSGNO_S     *msgs;
{
    long   i, slop, old_total, old_size;
    size_t len;

    for(i = 1L; i <= stream->nmsgs; i++)
      if(get_lflag(stream, NULL, i, MN_EXLD)){
	  old_total        = msgs->max_msgno;
	  old_size         = msgs->sort_size;
	  slop             = (msgs->max_msgno + 1L) % 64;
	  msgs->sort_size  = (msgs->max_msgno + 1L) + (64 - slop);
	  len		   = (size_t) msgs->sort_size * sizeof(long);
	  if(msgs->sort){
	      if(old_size != msgs->sort_size)
		fs_resize((void **)&(msgs->sort), len);
	  }
	  else
	    msgs->sort = (long *)fs_get(len);

	  msgs->sort[++msgs->max_msgno] = i;
	  set_lflag(stream, msgs, msgs->max_msgno, MN_EXLD, 0);

	  if(old_total <= 0L){			/* if no previous messages, */
	      if(!msgs->select){		/* select the new message   */
		  msgs->sel_size = 8L;
		  len		 = (size_t)msgs->sel_size * sizeof(long);
		  msgs->select   = (long *)fs_get(len);
	      }

	      msgs->sel_cnt   = 1L;
	      msgs->sel_cur   = 0L;
	      msgs->select[0] = 1L;
	  }
      }
}



/*----------------------------------------------------------------------
 Add the given number of raw message numbers to the end of the
 current list...

   Accepts: msgs - pointer to message manipulation struct
	    n - number to add
   Returns: with fixed up msgno struct

   Only have to adjust the sort array, as since new mail can't cause
   selection!
  ----*/
void
msgno_add_raw(msgs, n)
     MSGNO_S *msgs;
     long     n;
{
    long   slop, old_total, old_size;
    size_t len;

    if(!msgs || n <= 0L)
      return;

    old_total        = msgs->max_msgno;
    old_size         = msgs->sort_size;
    msgs->max_msgno += n;
    slop             = (msgs->max_msgno + 1L) % 64;
    msgs->sort_size  = (msgs->max_msgno + 1L) + (64 - slop);
    len		     = (size_t) msgs->sort_size * sizeof(long);
    if(msgs->sort){
	if(old_size != msgs->sort_size)
	  fs_resize((void **)&(msgs->sort), len);
    }
    else
      msgs->sort = (long *)fs_get(len);

    for(slop = old_total + 1L; slop <= msgs->max_msgno; slop++)
      msgs->sort[slop] = slop;

    if(old_total <= 0L){			/* if no previous messages, */
	if(!msgs->select){			/* select the new message   */
	    msgs->sel_size = 8L;
	    len		   = (size_t)msgs->sel_size * sizeof(long);
	    msgs->select   = (long *)fs_get(len);
	}

	msgs->sel_cnt   = 1L;
	msgs->sel_cur   = 0L;
	msgs->select[0] = 1L;
    }
}



/*----------------------------------------------------------------------
  Remove all knowledge of the given raw message number

   Accepts: msgs - pointer to message manipulation struct
	    n - number to remove
   Returns: with fixed up msgno struct

   After removing *all* references, adjust the sort array and
   various pointers accordingly...
  ----*/
void
msgno_flush_raw(msgs, n)
     MSGNO_S *msgs;
     long     n;
{
    long i, old_sorted = 0L;
    int  shift = 0;

    if(!msgs)
      return;

    /*---- blast n from sort array ----*/
    for(i = 1L; i <= msgs->max_msgno; i++){
	if(msgs->sort[i] == n){
	    old_sorted = i;
	    shift++;
	}

	if(shift)
	  msgs->sort[i] = msgs->sort[i + 1L];

	if(msgs->sort[i] > n)
	  msgs->sort[i] -= 1L;
    }

    /*---- now, fixup select array ----*/
    msgs->max_msgno = max(0L, msgs->max_msgno - 1L);
    msgno_flush_selected(msgs, old_sorted);
}



/*----------------------------------------------------------------------
  Remove all knowledge of the given selected message number

   Accepts: msgs - pointer to message manipulation struct
	    n - number to remove
   Returns: with fixed up selec members in msgno struct

   Remove reference and fix up selected message numbers beyond
   the specified number
  ----*/
void
msgno_flush_selected(msgs, n)
     MSGNO_S *msgs;
     long     n;
{
    long i;
    int  shift = 0;

    for(i = 0L; i < msgs->sel_cnt; i++){
	if(!shift && (msgs->select[i] == n))
	  shift++;

	if(shift && i + 1L < msgs->sel_cnt)
	  msgs->select[i] = msgs->select[i + 1L];

	if(n < msgs->select[i] || msgs->select[i] > msgs->max_msgno)
	  msgs->select[i] -= 1L;
    }

    if(shift && msgs->sel_cnt > 1L)
      msgs->sel_cnt -= 1L;
}



/*----------------------------------------------------------------------
  Test to see if the given message number is in the selected message
  list...

   Accepts: msgs - pointer to message manipulation struct
	    n - number to test
   Returns: true if n is in selected array, false otherwise

  ----*/
int
msgno_in_select(msgs, n)
     MSGNO_S *msgs;
     long     n;
{
    long i;

    if(msgs)
      for(i = 0L; i < msgs->sel_cnt; i++)
	if(msgs->select[i] == n)
	  return(1);

    return(0);
}



/*----------------------------------------------------------------------
  return our index number for the given raw message number

   Accepts: msgs - pointer to message manipulation struct
	    n - number to locate
   Returns: our index number of given raw message

  ----*/
long
msgno_in_sort(msgs, n)
     MSGNO_S *msgs;
     long     n;
{
    static long start = 1L;
    long        i;

    if(mn_get_total(msgs) < 1L)
      return(-1L);
    else if(mn_get_sort(msgs) == SortArrival && !any_lflagged(msgs, MN_EXLD))
      return((mn_get_revsort(msgs)) ? 1 + mn_get_total(msgs) - n  : n);

    if(start > mn_get_total(msgs))		/* reset start? */
      start = 1L;

    i = start;
    do {
	if(mn_m2raw(msgs, i) == n)
	  return(start = i);

	if(++i > mn_get_total(msgs))
	  i = 1L;
    }
    while(i != start);

    return(0L);
}



/*
 *           * * *  Index entry cache manager  * * *
 */

/*
 * at some point, this could be made part of the pine_state struct.
 * the only changes here would be to pass the ps pointer around
 */
static struct index_cache {
   void	  *cache;				/* pointer to cache         */
   char	  *name;				/* pointer to cache name    */
   long    num;					/* # of last index in cache */
   size_t  size;				/* size of each index line  */
   int     need_format_setup;
} icache = { (void *) NULL, (char *) NULL, (long) 0, (size_t) 0, (int) 0 };
  
/*
 * cache size growth increment
 */

#ifdef	DOS
/*
 * the idea is to have the cache increment be a multiple of the block
 * size (4K), for efficient swapping of blocks.  we can pretty much
 * assume 81 character lines.
 *
 * REMEMBER: number of lines in the incore cache has to be a multiple 
 *           of the cache growth increment!
 */
#define	IC_SIZE		(50L)			/* cache growth increment  */
#define	ICC_SIZE	(50L)			/* enties in incore cache  */
#define FUDGE           (46L)			/* extra chars to make 4096*/

static char	*incore_cache = NULL;		/* pointer to incore cache */
static long      cache_block_s = 0L;		/* save recomputing time   */
static long      cache_base = 0L;		/* index of line 0 in block*/
#else
#define	IC_SIZE		100
#endif

/*
 * important values for cache building
 */
static MAILSTREAM *bc_this_stream = NULL;
static long  bc_start, bc_current;
static short bc_done = 0;


/*
 * way to return the current cache entry size
 */
int
i_cache_width()
{
    return(icache.size - sizeof(HLINE_S));
}


/* 
 * i_cache_size - make sure the cache is big enough to contain
 * requested entry
 */
int
i_cache_size(indx)
    long         indx;
{
    long j;
    size_t  newsize = sizeof(HLINE_S)
		     + (max(ps_global->ttyo->screen_cols, 80) * sizeof(char));

    if(j = (newsize % sizeof(long)))		/* alignment hack */
      newsize += (sizeof(long) - (size_t)j);

    if(icache.size != newsize){
	clear_index_cache();			/* clear cache, start over! */
	icache.size = newsize;
    }

    if(indx > (j = icache.num - 1L)){		/* make room for entry! */
	size_t  tmplen = icache.size;
	char   *tmpline;

	while(indx >= icache.num)
	  icache.num += IC_SIZE;

#ifdef	DOS
	tmpline = fs_get(tmplen);
	memset(tmpline, 0, tmplen);
	if(icache.cache == NULL){
	    if(!icache.name)
	      icache.name = temp_nam(NULL, "pi");

	    if((icache.cache = (void *)fopen(icache.name,"w+b")) == NULL){
		sprintf(tmp_20k_buf, "Can't open index cache: %s",icache.name);
		fatal(tmp_20k_buf);
	    }

	    for(j = 0; j < icache.num; j++){
	        if(fwrite(tmpline,tmplen,(size_t)1,(FILE *)icache.cache) != 1)
		  fatal("Can't write index cache in resize");

		if(j%ICC_SIZE == 0){
		  if(fwrite(tmpline,(size_t)FUDGE,
				(size_t)1,(FILE *)icache.cache) != 1)
		    fatal("Can't write FUDGE factor in resize");
	        }
	    }
	}
	else{
	    /* init new entries */
	    fseek((FILE *)icache.cache, 0L, 2);		/* seek to end */

	    for(;j < icache.num; j++){
	        if(fwrite(tmpline,tmplen,(size_t)1,(FILE *)icache.cache) != 1)
		  fatal("Can't write index cache in resize");

		if(j%ICC_SIZE == 0){
		  if(fwrite(tmpline,(size_t)FUDGE,
				(size_t)1,(FILE *)icache.cache) != 1)
		    fatal("Can't write FUDGE factor in resize");
	        }
	    }
	}

	fs_give((void **)&tmpline);
#else
	if(icache.cache == NULL){
	    icache.cache = (void *)fs_get((icache.num+1)*tmplen);
	    memset(icache.cache, 0, (icache.num+1)*tmplen);
	}
	else{
            fs_resize((void **)&(icache.cache), (size_t)(icache.num+1)*tmplen);
	    tmpline = (char *)icache.cache + ((j+1) * tmplen);
	    memset(tmpline, 0, (icache.num - j) * tmplen);
	}
#endif
    }

    return(1);
}

#ifdef	DOS
/*
 * read a block into the incore cache
 */
void
icread()
{
    size_t n;

    if(fseek((FILE *)icache.cache, (cache_base/ICC_SIZE) * cache_block_s, 0))
      fatal("ran off end of index cache file in icread");

    n = fread((void *)incore_cache, (size_t)cache_block_s, 
		(size_t)1, (FILE *)icache.cache);

    if(n != 1L)
      fatal("Can't read index cache block in from disk");
}


/*
 * write the incore cache out to disk
 */
void
icwrite()
{
    size_t n;

    if(fseek((FILE *)icache.cache, (cache_base/ICC_SIZE) * cache_block_s, 0))
      fatal("ran off end of index cache file in icwrite");

    n = fwrite((void *)incore_cache, (size_t)cache_block_s,
		(size_t)1, (FILE *)icache.cache);

    if(n != 1L)
      fatal("Can't write index cache block in from disk");
}


/*
 * make sure the necessary block of index lines is in core
 */
void
i_cache_hit(indx)
    long         indx;
{
    dprint(9, (debugfile, "i_cache_hit: %ld\n", indx));
    /* no incore cache, create it */
    if(!incore_cache){
	cache_block_s = (((long)icache.size * ICC_SIZE) + FUDGE)*sizeof(char);
	incore_cache  = (char *)fs_get((size_t)cache_block_s);
	cache_base = (indx/ICC_SIZE) * ICC_SIZE;
	icread();
	return;
    }

    if(indx >= cache_base && indx < (cache_base + ICC_SIZE))
	return;

    icwrite();

    cache_base = (indx/ICC_SIZE) * ICC_SIZE;
    icread();
}
#endif


/*
 * return the index line associated with the given message number
 */
HLINE_S *
get_index_cache(msgno)
    long         msgno;
{
    if(!i_cache_size(--msgno)){
	q_status_message(SM_ORDER, 0, 3, "get_index_cache failed!");
	return(NULL);
    }

#ifdef	DOS
    i_cache_hit(msgno);			/* get entry into core */
    return((HLINE_S *)(incore_cache 
	      + ((msgno%ICC_SIZE) * (long)max(icache.size,FUDGE))));
#else
    return((HLINE_S *) ((char *)(icache.cache) 
	   + (msgno * (long)icache.size * sizeof(char))));
#endif
}


/*
 * the idea is to pre-build and cache index lines while waiting
 * for command input.
 */
void
build_header_cache()
{
    long lines_per_page = max(0,ps_global->ttyo->screen_rows - 5);

    if(mn_get_total(ps_global->msgmap) == 0 || ps_global->mail_stream == NULL
       || (bc_this_stream == ps_global->mail_stream && bc_done >= 2)
       || any_lflagged(ps_global->msgmap, (MN_HIDE|MN_EXLD|MN_SLCT)))
      return;

    if(bc_this_stream != ps_global->mail_stream){ /* reset? */
	bc_this_stream = ps_global->mail_stream;
	bc_current = bc_start = top_ent_calc(ps_global->mail_stream,
					     ps_global->msgmap,
					     mn_get_cur(ps_global->msgmap),
					     lines_per_page);
	bc_done  = 0;
    }

    if(!bc_done && bc_current > mn_get_total(ps_global->msgmap)){ /* wrap? */
	bc_current = bc_start - lines_per_page;
	bc_done++;
    }
    else if(bc_done == 1 && (bc_current % lines_per_page) == 1)
      bc_current -= (2L * lines_per_page);

    if(bc_current < 1)
      bc_done = 2;			/* really done! */
    else
      (void)build_header_line(ps_global, ps_global->mail_stream,
			      ps_global->msgmap, bc_current++);
}


/*
 * erase a particular entry in the cache
 */
void
clear_index_cache_ent(indx)
    long indx;
{
    HLINE_S *tmp = get_index_cache(indx);

    tmp->line[0] = '\0';
    tmp->id      = 0L;
}


/*
 * clear the index cache associated with the current mailbox
 */
void
clear_index_cache()
{
#ifdef	DOS
    cache_base = 0L;
    if(incore_cache)
      fs_give((void **)&incore_cache);

    if(icache.cache){
	fclose((FILE *)icache.cache);
	icache.cache = NULL;
    }

    if(icache.name){
	unlink(icache.name);
	fs_give((void **)&icache.name);
    }
#else
    if(icache.cache)
      fs_give((void **)&(icache.cache));
#endif
    icache.num  = 0L;
    icache.size = 0;
    bc_this_stream = NULL;
    set_need_format_setup();
}


void
set_need_format_setup()
{
    icache.need_format_setup = 1;
}


void
clear_need_format_setup()
{
    icache.need_format_setup = 0;
}


int
check_need_format_setup()
{
    return(icache.need_format_setup);
}


#ifdef	DOS
/*
 * flush the incore_cache, but not the whole enchilada
 */
void
flush_index_cache()
{
    if(incore_cache){
	if(mn_get_total(ps_global->msgmap) > 0L)
	  icwrite();			/* write this block out to disk */

	fs_give((void **)&incore_cache);
	cache_base = 0L;
    }
}
#endif


#ifdef _WINDOWS
/*----------------------------------------------------------------------
  Callback to get the text of the current message.  Used to display
  a message in an alternate window.	  

  Args: cmd - what type of scroll operation.
	text - filled with pointer to text.
	l - length of text.
	style - Returns style of text.  Can be:
		GETTEXT_TEXT - Is a pointer to text with CRLF deliminated
				lines
		GETTEXT_LINES - Is a pointer to NULL terminated array of
				char *.  Each entry points to a line of
				text.
					
		this implementation always returns GETTEXT_TEXT.

  Returns: TRUE - did the scroll operation.
	   FALSE - was not able to do the scroll operation.
 ----*/
int
index_scroll_callback (cmd, scroll_pos)
int	cmd;
long	scroll_pos;
{
    int paint = TRUE;
    
    switch (cmd) {
      case MSWIN_KEY_SCROLLUPLINE:
	paint = index_scroll_up (1);
	break;

      case MSWIN_KEY_SCROLLDOWNLINE:
	paint = index_scroll_down (1);
	break;

      case MSWIN_KEY_SCROLLUPPAGE:
	paint = index_scroll_up (current_index_state->lines_per_page);
	break;

      case MSWIN_KEY_SCROLLDOWNPAGE:
	paint = index_scroll_down (current_index_state->lines_per_page);
	break;

      case MSWIN_KEY_SCROLLTO:
	paint = index_scroll_to_pos (scroll_pos + 1);
	break;
    }

    if(paint){
	mswin_beginupdate();
	update_titlebar_message();
	update_titlebar_status();
	redraw_index_body();
	mswin_endupdate();
    }

    return(paint);
}


/*----------------------------------------------------------------------
     MSWin scroll callback to get the text of the current message

  Args: title - title for new window
	text - 
	l - 
	style - 

  Returns: TRUE - got the requested text
	   FALSE - was not able to get the requested text
 ----*/
int
index_gettext_callback(title, text, l, style)
    char  *title;
    void **text;
    long  *l;
    int   *style;
{
    ENVELOPE *env;
    BODY     *body;
    STORE_S  *so;
    gf_io_t   pc;

    if(mn_get_total(ps_global->msgmap) > 0L
       && (so = so_get(CharStar, NULL, WRITE_ACCESS))){
	gf_set_so_writec(&pc, so);

	if((env = mail_fetchstructure(ps_global->mail_stream,
				      mn_m2raw(ps_global->msgmap,
					       mn_get_cur(ps_global->msgmap)),
				      &body))
	   && format_message(mn_m2raw(ps_global->msgmap,
				      mn_get_cur(ps_global->msgmap)),
			     env, body, FM_NEW_MESS, pc)){
	    sprintf(title, "Folder %.50s  --  Message %ld of %ld",
		    ps_global->cur_folder,
		    mn_get_cur(ps_global->msgmap),
		    mn_get_total(ps_global->msgmap));
	    *text  = so_text(so);
	    *l     = strlen((char *)so_text(so));
	    *style = GETTEXT_TEXT;

	    /* free alloc'd so, but preserve the text passed back to caller */
	    so->txt = (void *)NULL;
	    so_give(&so);
	    return(1);
	}
    }

    return(0);
}
#endif	/* _WINDOWS */
