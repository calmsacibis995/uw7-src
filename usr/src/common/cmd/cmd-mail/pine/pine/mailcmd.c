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
     mailcmd.c
     The meat and pototoes of mail processing here:
       - initial command processing and dispatch
       - save message
       - capture address off incoming mail
       - jump to specific numbered message
       - open (broach) a new folder
       - search message headers (where is) command
  ====*/

#include "headers.h"


/*
 * Internal Prototypes
 */
void      cmd_delete PROTO((struct pine *, MSGNO_S *, int));
void      cmd_undelete PROTO((struct pine *, MSGNO_S *, int));
void      cmd_reply PROTO((struct pine *, MSGNO_S *, int));
void      cmd_forward PROTO((struct pine *, MSGNO_S *, int));
void      cmd_bounce PROTO((struct pine *, MSGNO_S *, int));
void      cmd_print PROTO((struct pine *, MSGNO_S *, int, int));
void      cmd_save PROTO((struct pine *, MSGNO_S *, int, int));
void      cmd_export PROTO((struct pine *, MSGNO_S *, int, int));
void      cmd_pipe PROTO((struct pine *, MSGNO_S *, int));
PIPE_S	 *cmd_pipe_open PROTO((char *, char **, int, gf_io_t *));
void	  prime_raw_text_getc PROTO((MAILSTREAM *, long));
void      cmd_flag PROTO((struct pine *, MSGNO_S *, int));
int	  cmd_flag_prompt PROTO((struct pine *, struct flag_screen *));
long	  save PROTO((struct pine *, CONTEXT_S *, char *, MSGNO_S *, int));
void      get_save_fldr_from_env PROTO((char *, int, ENVELOPE *,
					 struct pine *, MSGNO_S *));
void	  saved_date PROTO((char *, char *));
int	  create_for_save PROTO((MAILSTREAM *, CONTEXT_S *, char *));
void	  flag_string PROTO((MESSAGECACHE *, long, char *));
int	  select_sort PROTO((struct pine *, int));
void	  aggregate_select PROTO((struct pine *, MSGNO_S *, int, int));
int	  individual_select PROTO((struct pine *, MSGNO_S *, int, int));
int	  select_number PROTO((MAILSTREAM *, MSGNO_S *, long));
int	  select_date PROTO((MAILSTREAM *, MSGNO_S *, long));
int	  select_text PROTO((MAILSTREAM *, MSGNO_S *, long));
int	  select_flagged PROTO((MAILSTREAM *, MSGNO_S *, long));
int	  apply_command PROTO((struct pine *, MSGNO_S *, int));
long	  zoom_index PROTO((struct pine *, MSGNO_S *, long *));
int	  unzoom_index PROTO((struct pine *, MSGNO_S *));
void	  jump_to PROTO((MSGNO_S *, int, int));
void	  search_headers PROTO((struct pine *, MAILSTREAM *, int, MSGNO_S *));
char	 *currentf_sequence PROTO((MAILSTREAM *, MSGNO_S *, long, long *,int));
char	 *selected_sequence PROTO((MAILSTREAM *, MSGNO_S *, long *));
char	 *build_sequence PROTO((MAILSTREAM *, MSGNO_S *, long *));
void	  cmd_cancelled PROTO((char *));
int	  any_messages PROTO((MSGNO_S *, char *, char *));
int	  can_set_flag PROTO((struct pine *, char *));
int	  bezerk_delimiter PROTO((ENVELOPE *, gf_io_t, int));
char	 *move_read_msgs PROTO((MAILSTREAM *, char *, char *, long));
int	  read_msg_prompt PROTO((long, char *));
char	 *move_read_incoming PROTO((MAILSTREAM *, CONTEXT_S *, char *, char **,
				    char *));


/*
 * List of Select options used by apply_* functions...
 */
static char *sel_pmt1 = "ALTER message selection : ";
static ESCKEY_S sel_opts1[] = {
    {'a', 'a', "A", "unselect All"},
    {'c', 'c', "C", NULL},
    {'b', 'b', "B", "Broaden selctn"},
    {'n', 'n', "N", "Narrow selctn"},
    {'f', 'f', "F", "Flip selected"},
    {-1, 0, NULL, NULL}
};


static char *sel_pmt2 = "SELECT criteria : ";
static ESCKEY_S sel_opts2[] = {
    {'a', 'a', "A", "select All"},
    {'c', 'c', "C", "select Cur"},
    {'n', 'n', "N", "Number"},
    {'d', 'd', "D", "Date"},
    {'t', 't', "T", "Text"},
    {'s', 's', "S", "Status"},
    {-1, 0, NULL, NULL}
};


static char *sel_pmt3 = "APPLY command : ";
static ESCKEY_S sel_opts3[] = {
    {'d', 'd',  "D", "Del"},
    {'u', 'u',  "U", "Undel"},
    {'r', 'r',  "R", "Reply"},
    {'f', 'f',  "F", "Forward"},
    {'y', 'y',  "Y", "prYnt"},
    {'t', 't',  "T", "TakeAddr"},
    {'s', 's',  "S", "Save"},
    {'e', 'e',  "E", "Export"},
    { -1,   0, NULL, NULL},
    { -1,   0, NULL, NULL},
    { -1,   0, NULL, NULL},
    {-1, 0, NULL, NULL}
};


static char *sel_flag = 
    "Select New, Deleted, Answered, or Important messages ? ";
static char *sel_flag_not = 
    "Select NOT New, NOT Deleted, NOT Answered or NOT Tagged msgs ? ";
static ESCKEY_S sel_flag_opt[] = {
    {'n', 'n', "N", "New"},
    {'*', '*', "*", "Important"},
    {'d', 'd', "D", "Deleted"},
    {'a', 'a', "A", "Answered"},
    {'!', '!', "!", "Not"},
    {-1, 0, NULL, NULL}
};


static ESCKEY_S sel_date_opt[] = {
    {0, 0, NULL, NULL},
    {ctrl('P'), 12, "^P", "Prev Day"},
    {ctrl('N'), 13, "^N", "Next Day"},
    {ctrl('X'), 11, "^X", "Cur Msg"},
    {ctrl('W'), 14, "^W", "Toggle When"},
    {KEY_UP,    12, "", ""},
    {KEY_DOWN,  13, "", ""},
    {-1, 0, NULL, NULL}
};


static char *sel_text =
    "Select based on To, From, Cc, or Subject fields or All message text ? ";
static ESCKEY_S sel_text_opt[] = {
    {'f', 'f', "F", "From"},
    {'s', 's', "S", "Subject"},
    {'t', 't', "T", "To"},
    {'a', 'a', "A", "All Text"},
    {'c', 'c', "C", "Cc"},
    {-1, 0, NULL, NULL}
};

static char *select_num =
  "Enter comma-delimited list of numbers (dash between ranges): ";


/*----------------------------------------------------------------------
         The giant switch on the commands for index and viewing

  Input:  command  -- The command char/code
          in_index -- flag indicating command is from index
          orig_command -- The original command typed before pre-processing
  Output: force_mailchk -- Set to tell caller to force call to new_mail().

  Result: Manifold

          Returns 1 if the message number or attachment to show changed 
 ---*/
int
process_cmd(state, msgmap, command, in_index, orig_command, force_mailchk)
    struct pine *state;
    MSGNO_S     *msgmap;
    int		 command, in_index, orig_command;
    int		*force_mailchk;
{
    int           question_line, a_changed, is_unread, we_cancel;
    long          new_msgno, del_count, old_msgno, cur_msgno, i,
		  hide_count, exld_count, select_count, old_max_msgno;
    char         *newfolder, prompt[80+MAXFOLDER];
    CONTEXT_S    *tc;
#if	defined(DOS) && !defined(_WINDOWS)
    extern long coreleft();
#endif

    dprint(4, (debugfile, "\n - process_cmd((%d)%c) -\n",
                                                 command, (char)command));

    question_line         = -FOOTER_ROWS(state);
    state->mangled_screen = 0;
    state->mangled_footer = 0;
    state->mangled_header = 0;
    state->next_screen    = SCREEN_FUN_NULL;
    cur_msgno		  = mn_get_cur(msgmap);
    old_msgno             = cur_msgno;
    a_changed             = 0;
    *force_mailchk        = 0;

    switch (command)
      {
          /*------------- Help --------*/
        case PF1:
        case OPF1:
        case OOPF1:
        case OOOPF1:
        case '?':
        case ctrl('G'):
          if(state->nr_mode) {
              q_status_message(SM_ORDER, 0, 3,
			       "No help text currently available");
              break;
          }
	  /*
	   * We're not using the h_mail_view portion of this right now because
	   * that call is being handled in scrolltool() before it gets
	   * here.  Leave it in case we change how it works.
	   */
          helper(in_index?h_mail_index : h_mail_view,
		 in_index?"HELP FOR FOLDER INDEX VIEW":"HELP FOR MESSAGE VIEW",
		 0);
          dprint(2, (debugfile,"MAIL_CMD: did help command\n"));
          state->mangled_screen = 1;
          break;


          /*--------- Return to main menu ------------*/
        case PF3: 
        case 'm':
          if(state->nr_mode && command == 'm')
            goto bogus;
          if(state->nr_mode && command == PF3)
	    goto do_quit;
          state->next_screen = main_menu_screen;
#if	defined(DOS) && !defined(WIN32)
	  flush_index_cache();		/* save room on PC */
#endif
          dprint(2, (debugfile,"MAIL_CMD: going back to main menu\n"));
          break;


          /*------- View mail or attachment --------*/
        case ctrl('M'):
        case ctrl('J'):
	  if(!in_index){
	      q_status_message(SM_ORDER | SM_DING, 0, 3,
			      "No default action in the Message Text screen.");
	      break;
	  }

        case PF4:
        case 'v':
	  if(in_index) {
	      if(any_messages(msgmap, NULL, "to View")){
		  state->next_screen = mail_view_screen;
#if	defined(DOS) && !defined(WIN32)
		  flush_index_cache();		/* save room on PC */
#endif
	      }
	  }
	  else if(state->nr_mode)
	    goto bogus;
	  else
	    attachment_screen(state, msgmap);

          break;


          /*---------- Previous message ----------*/
        case PF5: 
        case KEY_UP:
        case 'p':
        case ctrl('P'):		    
	  if(any_messages(msgmap, NULL, NULL)){
	      if(cur_msgno > 1L){
		  mn_dec_cur(state->mail_stream, msgmap);
		  if(cur_msgno == mn_get_cur(msgmap))
		    q_status_message(SM_ORDER, 0, 2,
				  "Already on first message in Zoomed Index");
		  else
		    cur_msgno = mn_get_cur(msgmap);
	      }
	      else
		q_status_message(SM_ORDER, 0, 1, "Already on first message");
	  }

          break;


          /*---------- Next Message ----------*/
        case PF6:
        case KEY_DOWN:
        case 'n':
        case ctrl('N'):
	  if(mn_get_total(msgmap) > 0L && cur_msgno < mn_get_total(msgmap)){
	      mn_inc_cur(state->mail_stream, msgmap);
	      if(cur_msgno == mn_get_cur(msgmap))
		any_messages(NULL, "more", "in Zoomed Index");
	      else
		cur_msgno = mn_get_cur(msgmap);
          }
	  else{
	      prompt[0] = '\0';
	      if(!state->nr_mode
		 && (IS_NEWS(state->mail_stream)
		     || (state->context_current->use & CNTXT_INCMNG))){
		  char nextfolder[MAXPATH];

		  strcpy(nextfolder, state->cur_folder);
		  if(next_folder(NULL, nextfolder, nextfolder,
				 state->context_current, FALSE))
		    strcat(prompt, ".  Press TAB for next folder.");
		  else
		    strcat(prompt, ".  No more folders to TAB to.");
	      }

	      any_messages(NULL, (mn_get_total(msgmap) > 0L) ? "more" : NULL,
			   prompt[0] ? prompt : NULL);

	      if(!IS_NEWS(state->mail_stream))
		*force_mailchk = 1;
	  }
          break;


          /*---------- Delete message ----------*/
        case PF9: 
        case 'd':
        case KEY_DEL:
          if(state->nr_mode) {
	    if(command == PF9)
	      goto do_forward;
	    else
              goto bogus;
	  }

	  cmd_delete(state, msgmap, 0);
	  cur_msgno = mn_get_cur(msgmap);
	  break;
          

          /*---------- Undelete message ----------*/
        case PF10:
        case 'u':
          if(state->nr_mode) {
	    if(command == PF10)
	      goto do_jump;
	    else
              goto bogus;
	  }

	  cmd_undelete(state, msgmap, 0);
          break;


          /*---------- Reply to message ----------*/
        case PF11: 
        case 'r':
          if(state->anonymous && command == PF11) {
	    if(in_index)
	      goto do_sortindex;
	    else
	      goto do_index;
	  }

          if(state->nr_mode && command == PF11)
	    goto do_print;
          if(state->nr_mode)
            goto bogus;

	  cmd_reply(state, msgmap, 0);
	  cur_msgno = mn_get_cur(msgmap);
          break;


          /*---------- Forward message ----------*/
        case PF12: 
        case 'f':
do_forward:
          if(command == PF12) {
	    if(state->anonymous)
              goto bogus;
	    if(state->nr_mode)
	      goto do_save;
	  }

	  cmd_forward(state, msgmap, 0);
	  cur_msgno = mn_get_cur(msgmap);
          break;


          /*---------- Quit pine ------------*/
        case 'q':
        case OPF3:
          if(state->nr_mode && command == OPF3)
            goto do_export;
do_quit:
	  state->next_screen = quit_screen;
	  dprint(1, (debugfile,"MAIL_CMD: quit\n"));		    
          break;


          /*---------- Compose message ----------*/
        case OPF4:		    
        case 'c':
          if(state->anonymous)
            goto bogus;
          state->prev_screen = in_index ? mail_index_screen :
                                        mail_view_screen;
#if	defined(DOS) && !defined(WIN32)
	  flush_index_cache();		/* save room on PC */
#endif
          compose_screen(state);
          state->mangled_screen = 1;
          break;


          /*--------- Folders menu ------------*/
	case OPF5: 
        case 'l':
          if(state->anonymous)
	    goto bogus;
          if(state->nr_mode) {
	    if(command == OPF5)
	      goto do_sortindex;
	    goto bogus;
	  }
          state->next_screen = folder_screen;
#if	defined(DOS) && !defined(WIN32)
	  flush_index_cache();		/* save room on PC */
#endif
          dprint(2, (debugfile,"MAIL_CMD: going to folder/collection menu\n"));
          break;


          /*---------- Open specific new folder ----------*/
        case OPF6:
        case 'g':
          if(state->nr_mode)
            goto bogus;

	  tc = (state->context_last
		      && !(state->context_current->type & FTYPE_BBOARD)) 
                       ? state->context_last : state->context_current;

          newfolder = broach_folder(question_line, 1, &tc);
#if	defined(DOS) && !defined(_WINDOWS)
	  if(newfolder && *newfolder == '{' && coreleft() < 20000){
	      q_status_message(SM_ORDER | SM_DING, 3, 3,
			       "Not enough memory to open IMAP folder");
	      newfolder = NULL;
	  }
#endif
          if(newfolder)
	    visit_folder(state, newfolder, tc);

          break;
    	  
    	    
          /*------- Go to index (or tab) ----------*/
        case OPF7:
        case 'i':
do_index:
          if(!in_index) {
#if	defined(DOS) && !defined(WIN32)
	      flush_index_cache();		/* save room on PC */
#endif
              state->next_screen = mail_index_screen;
          }
	  else {
	      if(command == OPF7)
		goto do_tab;
	      else
		q_status_message(SM_ORDER, 0, 3, "Already in Index");
          }
          break;

do_tab:
          /*------- Skip to next interesting message -----------*/
        case TAB :
          if(mn_get_total(msgmap) > 0) {
              new_msgno = next_sorted_flagged((F_UNDEL 
					       | F_UNSEEN
					       | ((IS_NEWS(state->mail_stream)
						   && F_ON(F_FAKE_NEW_IN_NEWS,
							   state))
						       ? F_RECENT : 0)
					       | ((F_ON(F_TAB_TO_NEW,state))
						       ? 0 : F_OR_FLAG)),
					      state->mail_stream,
					      cur_msgno + 1L, &is_unread);
              if(is_unread){
                  if(new_msgno > 0){
		      mn_set_cur(msgmap, new_msgno);
                      cur_msgno = new_msgno;
                  }
	      }
	  }

	  /*
	   * If there weren't any unread messages left, OR there
	   * aren't any messages at all, we may want to offer to
	   * go on to the next folder...
	   */
	  if(!is_unread || mn_get_total(msgmap) <= 0){
	      char ret = 'n';
	      int  in_inbox = !strucmp(state->cur_folder,state->inbox_name);

	      if(!state->nr_mode && state->context_current
		 && (((state->context_current->use & CNTXT_NEWS)
		      && context_isambig(state->cur_folder))
		     || ((state->context_current->use & CNTXT_INCMNG)
			 && (in_inbox
			     || folder_index(state->cur_folder,
				     state->context_current->folders) >=0)))){
		  char		 nextfolder[MAXPATH];
		  MAILSTREAM	*nextstream = NULL;

		  strcpy(nextfolder, state->cur_folder);
		  while(1){
		      if(!(next_folder(&nextstream, nextfolder, nextfolder,
				       state->context_current, TRUE))){
			  if(!in_inbox){
			      sprintf(prompt, "No more %ss.  Return to \"%s\"",
				     (state->context_current->use&CNTXT_INCMNG)
				       ? "incoming folder" : "news group", 
				      state->inbox_name);
			      ret = want_to(prompt, 'y', 'x', NO_HELP, 0, 0);
			      if(ret == 'y')
				visit_folder(state, state->inbox_name,
					     state->context_current);
			  }
			  else
			    q_status_message1(SM_ORDER, 0, 2, "No more %ss",
				     (state->context_current->use&CNTXT_INCMNG)
				        ? "incoming folder" : "news group");

			  break;
		      }

		      sprintf(prompt, "View next %s \"%s\"? ",
			      (state->context_current->use&CNTXT_INCMNG)
				 ? "Incoming folder" : "news group",
			      nextfolder);

		      /*
		       * When help gets added, this'll have to become
		       * a loop like the rest...
		       */
		      if(F_OFF(F_AUTO_OPEN_NEXT_UNREAD, state)){
			  static ESCKEY_S next_opt[] = {
			      {'y', 'y', "Y", "Yes"},
			      {'n', 'n', "N", "No"},
			      {TAB, 'n', "Tab", "NextNew"},
			      {-1, 0, NULL, NULL}
			  };

			  ret = radio_buttons(prompt, -FOOTER_ROWS(state),
					      next_opt, 'y', 'x', NO_HELP,
					      RB_NORM);
			  if(ret == 'x'){
			      cmd_cancelled(NULL);
			      break;
			  }
		      }

		      if(ret == 'y' || F_ON(F_AUTO_OPEN_NEXT_UNREAD, state)){
			  visit_folder(state, nextfolder,
				       state->context_current);
			  break;
		      }
		  }

		  if(nextstream)
		    mail_close(nextstream);
	      }
	      else
		any_messages(NULL,
			     (mn_get_total(msgmap) > 0L)
			       ? IS_NEWS(state->mail_stream)
				 ? "more undeleted"
				 : "more new"
			       : NULL,
			     NULL);
	  }

          break;


          /*------- Zoom -----------*/
	case OOOPF4:
        case 'z':
	  if(F_OFF(F_ENABLE_AGG_OPS,state) || !in_index)
	    goto bogus;

	  /*
	   * Right now the way zoom is implemented is sort of silly.
	   * There are two per-message flags where just one and a 
	   * global "zoom mode" flag to suppress messags from the index
	   * should suffice.
	   */
	  if(any_messages(msgmap, NULL, "to Zoom on")){
	      if(unzoom_index(state, msgmap)){
		  dprint(4, (debugfile, "\n\n ---- Exiting ZOOM mode ----\n"));
		  q_status_message(SM_ORDER,0,2, "Index Zoom Mode is now off");
	      }
	      else if(i = zoom_index(state, msgmap, &cur_msgno)){
		  dprint(4, (debugfile,"\n\n ---- Entering ZOOM mode ----\n"));
		  q_status_message2(SM_ORDER, 0, 2,
	"In Zoomed Index of %s message%s.  Use \"Z\" to restore regular Index",
				    comatose(i), plural(i));
	      }
	      else
		any_messages(NULL, "selected", "to Zoom on");
	  }

          break;


          /*---------- Jump To ----------*/
       case OOPF8:
	  if(!in_index)
	    goto do_tab;
       case '0':
       case '1':
       case '2':
       case '3':
       case '4':
       case '5':
       case '6':
       case '7':
       case '8':
       case '9':
          if(F_OFF(F_ENABLE_JUMP,state))
	    goto bogus;
       case 'j':
do_jump:
	  jump_to(msgmap, question_line,
		  (command >= '0' && command <= '9') ? command : '\0');
	  cur_msgno		= mn_get_cur(msgmap);
	  state->mangled_footer = 1;
	  break;


          /*---------- Search (where is command) ----------*/
       case OPF8:
       case ctrl('W'):
       case 'w':		/* NOTE: whereis preempted outside Index */
	  search_headers(state, state->mail_stream, question_line, msgmap);
	  cur_msgno		= mn_get_cur(msgmap);
	  state->mangled_footer = 1;
          break;


          /*---------- print message on paper ----------*/
        case OPF9:
        case 'y':
do_print:
          if(state->anonymous || (state->nr_mode && command == OPF9))
            goto bogus;

	  if(any_messages(msgmap, NULL, "to prYnt"))
	    cmd_print(state, msgmap, 0, in_index);

          break;


          /*---------- Take Address ----------*/
        case OPF10:
        case 't':
          if(state->nr_mode)
            goto bogus;

	  if(any_messages(msgmap, NULL, "to Take address from"))
	    cmd_take_addr(state, msgmap, 0);

          break;


          /*---------- Save Message ----------*/
        case OPF11: 
        case 's':
          if(state->anonymous || (state->nr_mode && command == OPF11))
            goto bogus;
do_save:
	  if(any_messages(msgmap, NULL, "to Save")){
	      cmd_save(state, msgmap, question_line, 0);
	      cur_msgno = mn_get_cur(msgmap);
	  }

          break;


          /*---------- Export message ----------*/
        case OPF12:
        case 'e':
          if(state->anonymous || (state->nr_mode && command == OPF12))
            goto bogus;
do_export:
	  if(any_messages(msgmap, NULL, "to Export")){
	      cmd_export(state, msgmap, question_line, 0);
	      cur_msgno = mn_get_cur(msgmap);
	      state->mangled_footer = 1;
	  }

          break;


          /*---------- Expunge ----------*/
        case OOPF3:
        case 'x':
          if(state->nr_mode || !in_index)
            goto bogus;

          dprint(2, (debugfile, "\n - expunge -\n"));
	  if(IS_NEWS(state->mail_stream) && state->mail_stream->rdonly){
	      del_count = count_flagged(state->mail_stream, "DELETED")
					       - any_lflagged(msgmap, MN_EXLD);
	      if(del_count > 0L){
		  state->mangled_footer = 1;
		  sprintf(prompt, "Exclude %ld message%s from %s", del_count,
			  plural(del_count), pretty_fn(state->cur_folder));
		  if(F_ON(F_AUTO_EXPUNGE, state)
		     || want_to(prompt, 'y', 0, NO_HELP, 0, 0) == 'y'){
		      msgno_exclude(state->mail_stream, msgmap);
		      clear_index_cache();
		      state->mangled_body = 1;
		      state->mangled_header = 1;
		      q_status_message2(SM_ORDER, 0, 4,
					"%s message%s excluded",
					long2string(del_count),
					plural(del_count));
		  }
		  else
		    any_messages(NULL, NULL, "Excluded");
	      }
	      else
		any_messages(NULL, "deleted", "to Exclude");

              break;
	  } else if(READONLY_FOLDER){
              q_status_message(SM_ORDER, 0, 4,
		  "Can't expunge. Folder is read-only");
              break;
          }

	  if((del_count = count_flagged(state->mail_stream, "DELETED")) == 0){
	     q_status_message(SM_ORDER, 0, 4, 
		"Nothing to Expunge!  No messages marked \"Deleted\".");
	     *force_mailchk = 1;
	     break;
	  }

	  if(F_OFF(F_AUTO_EXPUNGE,state)) {
	      int ret;

	      sprintf(prompt, "Expunge %ld message%s from %s", del_count,
		      plural(del_count), pretty_fn(state->cur_folder));
	      state->mangled_footer = 1;
	      if((ret = want_to(prompt, 'y', 'x', NO_HELP, 0, 0)) == 'n'){
		  break;
	      }else if(ret == 'x') {		/* ^C */
		  cmd_cancelled("Expunge");
		  break;
	      }
	  }

	  /*
	   * count local flags so we can maintain the total count
	   * without having to traipse thru the whole folder every time.
	   * we don't want to actually remove the flags here in case
	   * the expunge should fail for some reason...
	   */
	  hide_count = exld_count = select_count = 0L;
	  if(!any_lflagged(msgmap, MN_NONE)){
	      /*
	       * Make sure c-client elt's current
	       */
	      FETCH_ALL_FLAGS(state->mail_stream);
	      for(i = 1L; i <= mn_get_total(msgmap); i++){
		  MESSAGECACHE *mc;

		  if((mc = mail_elt(state->mail_stream, mn_m2raw(msgmap, i)))
		     && mc->deleted){
		      if(get_lflag(state->mail_stream, msgmap, i, MN_HIDE))
			hide_count++;

		      if(get_lflag(state->mail_stream, msgmap, i, MN_EXLD))
			exld_count++;

		      if(get_lflag(state->mail_stream, msgmap, i, MN_SLCT))
			select_count++;
		  }
	      }
	  }

          dprint(8,(debugfile, "Expunge max:%ld cur:%ld kill:%d\n",
                    mn_get_total(msgmap), cur_msgno, del_count));

	  old_max_msgno = mn_get_total(msgmap);
          StartInverse();
          PutLine0(0, 0, "**");			/* indicate delay */
          EndInverse();
	  MoveCursor(state->ttyo->screen_rows -FOOTER_ROWS(state), 0);
          fflush(stdout);

	  we_cancel = busy_alarm(1, "Busy Expunging", NULL, 0);
	  ps_global->expunge_in_progress = 1;
	  mail_expunge(state->mail_stream);
	  ps_global->expunge_in_progress = 0;
	  if(we_cancel)
	    cancel_busy_alarm(0);

	  dprint(2,(debugfile,"expunge complete cur:%ld max:%ld\n",
		    mn_get_cur(msgmap), mn_get_total(msgmap)));
	  /*
	   * This is only actually necessary if this causes the width of the
	   * message number field to change.  That is, it depends on the
	   * format the user is using as well as on the max_msgno.  Since it
	   * should be rare, we'll just do it whenever it happens.
	   * Also have to check for an increase in max_msgno on new mail.
	   */
	  if(old_max_msgno >= 1000L && mn_get_total(msgmap) < 1000L
	     || old_max_msgno >= 10000L && mn_get_total(msgmap) < 10000L
	     || old_max_msgno >= 100000L && mn_get_total(msgmap) < 100000L){
	      clear_index_cache();
	      state->mangled_body = 1;
	  }

          /*
	   * mm_exists and mm_expunge take care of updating max_msgno
	   * and selecting a new message should the selected get removed
	   */
          reset_check_point();

	  /*
	   * This sort needs to be here in case the expunge discovers
	   * new mail.  However, if we were zoomed and the expunge didn't
	   * expunge all of the non-hidden messages, then we may skip the
	   * sort for now.
	   */
	  if(any_lflagged(msgmap, MN_HIDE))
	    state->unsorted_newmail = 1;
	  else
            sort_current_folder(1, mn_get_sort(msgmap),mn_get_revsort(msgmap));

          cur_msgno = mn_get_cur(msgmap);
          StartInverse();
          PutLine0(0, 0, "  ");			/* indicate delay's over */
          EndInverse();
          fflush(stdout);
          if(state->expunge_count > 0) {
              q_status_message3(SM_ORDER, 3, 3,
				"Expunged %s message%s from folder \"%s\"",
				long2string(state->expunge_count),
				plural(state->expunge_count),
				pretty_fn(state->cur_folder));
              state->expunge_count = 0;

	      /*
	       * This is kind of hokey.  We decrement each type of 
	       * local flag's count according to the number we counted
	       * before the expunge...
	       */
	      if(hide_count)
		dec_lflagged(msgmap, MN_HIDE, hide_count);

	      if(exld_count)
		dec_lflagged(msgmap, MN_EXLD, exld_count);

	      if(select_count)
		dec_lflagged(msgmap, MN_SLCT, select_count);

	      /*
	       * remember: since the ps->mail_box_changed bit got
	       * set, the call to new_mail() in the loops that called
	       * us will take care of fixing up the current msgno
	       * if there are selected msgs and such...
	       */
          }
	  else
	    q_status_message1(SM_ORDER, 0, 3,
			      "No messages expunged from folder \"%s\"",
			      pretty_fn(state->cur_folder));

          break;


          /*------- Unexclude -----------*/
	case OOPF4:
        case '&':
	  if(!in_index)
	    goto bogus;
	  else if(!(IS_NEWS(state->mail_stream)
		    && state->mail_stream->rdonly))
            q_status_message(SM_ORDER, 0, 3,
			     "Unexclude not available for mail folders");
	  else{
	      del_count = any_lflagged(msgmap, MN_EXLD);
	      if(del_count > 0L){
		  state->mangled_footer = 1;
		  sprintf(prompt, "UNexclude %ld message%s in %s", del_count,
			  plural(del_count), pretty_fn(state->cur_folder));
		  if(F_ON(F_AUTO_EXPUNGE, state)
		     || want_to(prompt, 'y', 0, NO_HELP, 0, 0) == 'y'){
		      msgno_include(state->mail_stream, msgmap);
		      clear_index_cache();
		      /*
		       * Have to add the excluded messages into the
		       * sort array.
		       */
		      sort_current_folder(1, mn_get_sort(msgmap),
					  mn_get_revsort(msgmap));
		      state->mangled_header = 1;
		      q_status_message2(SM_ORDER, 0, 4,
					"%s message%s UNexcluded",
					long2string(del_count),
					plural(del_count));
		  }
		  else
		    any_messages(NULL, NULL, "UNexcluded");
	      }
	      else
		any_messages(NULL, "excluded", "to UNexclude");
	  }

          break;


          /*------- Make Selection -----------*/
	case OOPF5:
        case ';':
	  if(!in_index || F_OFF(F_ENABLE_AGG_OPS, state))
	    goto bogus;

	  if(any_messages(msgmap, NULL, "to Select")){
	      aggregate_select(state, msgmap, question_line, in_index);
	      if(in_index && any_lflagged(msgmap, MN_SLCT) > 0L
		 && !any_lflagged(msgmap, MN_HIDE)
		 && F_ON(F_AUTO_ZOOM, state))
		(void) zoom_index(state, msgmap, &cur_msgno);
	      else
		cur_msgno = mn_get_cur(msgmap);
	  }
	    
          break;


          /*------- Toggle Current Message Selection State -----------*/
	case OOOPF3:
        case ':':
	  if(F_OFF(F_ENABLE_AGG_OPS, state))
	    goto bogus;

	  if(any_messages(msgmap, NULL, NULL)){
	      if(individual_select(state, msgmap, question_line, in_index)
		 && cur_msgno < mn_get_total(msgmap)){
		  /* advance current */
		  mn_inc_cur(state->mail_stream, msgmap);
		  if(cur_msgno != mn_get_cur(msgmap))
		    cur_msgno = mn_get_cur(msgmap);
	      }
	      else
		cur_msgno = mn_get_cur(msgmap);
	  }

          break;


          /*------- Apply command -----------*/
	case OOPF6:
        case 'a':
	  if(!in_index || F_OFF(F_ENABLE_AGG_OPS, state))
	    goto bogus;

	  if(any_messages(msgmap, NULL, NULL)){
	      if(any_lflagged(msgmap, MN_SLCT) > 0L){
		  if(apply_command(state, msgmap, question_line)
		     && F_ON(F_AUTO_UNZOOM, state))
		    unzoom_index(state, msgmap);
	      }
	      else
		any_messages(NULL, NULL,
			     "to Apply command to.  Try \"Select\"");

	      cur_msgno = mn_get_cur(msgmap);
	  }

          break;


          /*-------- Sort command -------*/
	case OOPF7:
        case '$':
	  {SortOrder oldorder;
	   int oldrev;
do_sortindex:
	    if(!in_index){
	      if(command == '$')
                goto bogus;
	      else
	        goto do_jump;
	    }

	    dprint(1, (debugfile,"MAIL_CMD: sort\n"));		    
	    oldorder = mn_get_sort(state->msgmap);
	    oldrev = mn_get_revsort(state->msgmap);
	    if(select_sort(state, question_line)){
	        clear_index_cache();
	        sort_current_folder(0, oldorder, oldrev);
	    }

	    state->mangled_footer = 1;
	  }
          break;


          /*------- Toggle Full Headers -----------*/
	case OOPF9: 
        case 'h':
          if(F_OFF(F_ENABLE_FULL_HDR,state))
            goto bogus;
          state->full_header = !state->full_header;
          q_status_message3(SM_ORDER, 0, 3,
		"Display of full headers is now o%s.  Use %s to turn back o%s",
			    state->full_header ? "n" : "ff",
			    F_ON(F_USE_FK, state) ? "F9" : "H",
			    !state->full_header ? "n" : "ff");
          a_changed = 1;
          break;


          /*------- Bounce -----------*/
	case OOPF10:
        case 'b':
          if(F_OFF(F_ENABLE_BOUNCE,state))
            goto bogus;

          if(command == PF12) {
	    if(state->anonymous)
              goto bogus;
	    if(state->nr_mode)
	      goto do_save;
	  }

	  cmd_bounce(state, msgmap, 0);
	  cur_msgno = mn_get_cur(msgmap);
          break;


          /*------- Flag -----------*/
	case OOPF11:
        case '*':
	  if(F_OFF(F_ENABLE_FLAG,state))
	    goto bogus;

          dprint(4, (debugfile, "\n - flag message -\n"));
	  cmd_flag(state, msgmap, 0);
	  cur_msgno = mn_get_cur(msgmap);

          break;


          /*------- Pipe message -----------*/
	case OOPF12:
        case '|':
	  if(F_ON(F_ENABLE_PIPE,state)){
	      cmd_pipe(state, msgmap, 0);
	      break;
	  }


          /*--------- Default, unknown command ----------*/
        default:
        bogus:
	  bogus_command(orig_command, F_ON(F_USE_FK,state) ? "F1" : "?");
          break;
      }

    return(cur_msgno != old_msgno || a_changed);
}



/*----------------------------------------------------------------------
   Complain about bogus input

  Args: ch -- input command to complain about
	help -- string indicating where to get help

 ----*/
void
bogus_command(cmd, help)
    int   cmd;
    char *help;
{
    if(cmd == ctrl('Q') || cmd == ctrl('S'))
      q_status_message1(SM_ASYNC, 0, 2,
 "%s char received.  Set \"preserve-start-stop\" feature in Setup/Config.",
			pretty_command(cmd));
    else if(cmd == KEY_JUNK)
      q_status_message3(SM_ORDER, 0, 2,
		      "Invalid key pressed.%s%s%s",
		      (help) ? " Use " : "",
		      (help) ?  help   : "",
		      (help) ? " for help" : "");
    else
      q_status_message4(SM_ORDER, 0, 2,
		      "Command \"%s\" not defined for this screen.%s%s%s",
		      pretty_command(cmd),
		      (help) ? " Use " : "",
		      (help) ?  help   : "",
		      (help) ? " for help" : "");
}



/*----------------------------------------------------------------------
   Complain about command on empty folder

  Args: map -- msgmap 
	type -- type of message that's missing
	cmd -- string explaining command attempted

 ----*/
int
any_messages(map, type, cmd)
    MSGNO_S *map;
    char *type, *cmd;
{
    if(mn_get_total(map) <= 0L){
	q_status_message4(SM_ORDER, 0, 2, "No %s%smessages%s%s",
			  type ? type : "",
			  type ? " " : "",
			  (!cmd || *cmd != '.') ? " " : "",
			  cmd ? cmd : "in folder");
	return(FALSE);
    }

    return(TRUE);
}


/*----------------------------------------------------------------------
   test whether or not we have a valid stream to set flags on

  Args: state -- pine state containing vital signs
	cmd -- string explaining command attempted

  Result: returns 1 if we can set flags, otw 0 and complains

 ----*/
int
can_set_flag(state, cmd)
    struct pine *state;
    char	*cmd;
{
    if(READONLY_FOLDER || state->dead_stream){
	q_status_message2(SM_ORDER | (state->dead_stream ? SM_DING : 0), 0, 3,
			  "Can't %s message.  Folder is %s.", cmd,
			  (state->dead_stream) ? "closed" : "read-only");
	return(FALSE);
    }

    return(TRUE);
}



/*----------------------------------------------------------------------
   Complain about command on empty folder

  Args: type -- type of message that's missing
	cmd -- string explaining command attempted

 ----*/
void
cmd_cancelled(cmd)
    char *cmd;
{
    q_status_message1(SM_INFO, 0, 2, "%s cancelled",
		      cmd ? cmd : "Command");
}



/*----------------------------------------------------------------------
   Execute DELETE message command

  Args: state --  Various satate info
        msgmap --  map of c-client to local message numbers

 Result: with side effect of "current" message delete flag set

 ----*/
void
cmd_delete(state, msgmap, agg)
     struct pine *state;
     MSGNO_S     *msgmap;
     int	  agg;
{
    int           is_unread;
    long	  cur_msgno, new_msgno, del_count = 0L;
    char	 *sequence = NULL, prompt[128];
    MESSAGECACHE *mc;

    dprint(4, (debugfile, "\n - delete message -\n"));
    if(!(any_messages(msgmap, NULL, "to Delete")
	 && can_set_flag(state, "delete")))
      return;

    cur_msgno = mn_get_cur(msgmap);

    if(state->io_error_on_stream) {
	state->io_error_on_stream = 0;
	mail_check(state->mail_stream); /* forces write */
    }

    if(agg){
	if(!(sequence = selected_sequence(state->mail_stream,
						msgmap, &del_count)))
	  return;				/* silently fail */
	else
	  sprintf(prompt, "%ld message%s ", del_count, plural(del_count));
    }
    else{
	del_count = 1L;				/* deleting single message */

	(void)mail_fetchstructure(state->mail_stream,
				  mn_m2raw(state->msgmap, cur_msgno), NULL);
	if(!(mc = mail_elt(state->mail_stream, mn_m2raw(msgmap, cur_msgno)))){
	    q_status_message1(SM_ORDER, 3, 4,
			     "Can't delete message %s. Error accessing folder",
			      long2string(cur_msgno));
	    return;
	}

	if(cur_msgno >= mn_get_total(msgmap))
	  strcpy(prompt, "Last message ");
	else
	  sprintf(prompt, "Message %ld ", cur_msgno);

	if(!mc->deleted){
	    clear_index_cache_ent(cur_msgno);
	    sequence = cpystr(long2string(mn_m2raw(msgmap, cur_msgno)));
	}
	else
	  strcat(prompt, "already ");
    }

    dprint(3,(debugfile, "Delete: msg %s%s\n",
	      (sequence) ? sequence : long2string(cur_msgno),
	      (sequence) ? "" : " ALREADY DELETED"));

    if(sequence){
	mail_setflag(state->mail_stream, sequence, "\\DELETED");
	fs_give((void **)&sequence);
	check_point_change();
	update_titlebar_status();
    }

    if(agg){
	sprintf(prompt, "%ld selected message%s marked for deletion",
		del_count, plural(del_count));
    }
    else{
	is_unread = 1;
	if((IS_NEWS(state->mail_stream)
	    || (state->context_current->use & CNTXT_INCMNG))
	   || F_ON(F_DEL_SKIPS_DEL, state))
	  new_msgno = next_sorted_flagged(F_UNDEL, state->mail_stream,
					  cur_msgno + 1, &is_unread);
	if(F_OFF(F_DEL_SKIPS_DEL,state) && cur_msgno < mn_get_total(msgmap)){
	    mn_inc_cur(state->mail_stream, msgmap);
	    cur_msgno = mn_get_cur(msgmap);
	}
	else if(F_ON(F_DEL_SKIPS_DEL,state) && is_unread){
	    /* more interesting mail, goto next msg */
	    mn_set_cur(msgmap, new_msgno);
	    cur_msgno = new_msgno;
	}

	if(prompt[0])
	  strcat(prompt, "marked for deletion");

	if((IS_NEWS(state->mail_stream)
	    || (state->context_current->use & CNTXT_INCMNG))
	   && !state->nr_mode && !is_unread){
	    char nextfolder[MAXPATH];

	    if(prompt[0] == '\0')
	      strcat(prompt, "Last undeleted message");

	    strcpy(nextfolder, state->cur_folder);
	    if(next_folder(NULL, nextfolder, nextfolder,
			   state->context_current, FALSE))
	      strcat(prompt, ".  Press TAB for next folder.");
	    else
	      strcat(prompt, ".  No more folders to TAB to.");
	}
    }

    if(prompt[0])
      q_status_message(SM_ORDER, 0, 3, prompt);

    mn_set_cur(msgmap, cur_msgno);
}



/*----------------------------------------------------------------------
   Execute UNDELETE message command

  Args: state --  Various satate info
        msgmap --  map of c-client to local message numbers

 Result: with side effect of "current" message delete flag UNset

 ----*/
void
cmd_undelete(state, msgmap, agg)
     struct pine *state;
     MSGNO_S     *msgmap;
     int	  agg;
{
    long	  cur_msgno, del_count = 0L;
    char	 *sequence = NULL;
    ENVELOPE     *e;

    dprint(4, (debugfile, "\n - undelete -\n"));
    if(!(any_messages(msgmap, NULL, "to Undelete")
	 && can_set_flag(state, "undelete")))
      return;

    cur_msgno = mn_get_cur(msgmap);

    if(agg){
	if(!(sequence = selected_sequence(state->mail_stream,
						msgmap, &del_count)))
	  return;				/* silently fail */
    }
    else{
	MESSAGECACHE *mc;

	del_count = 1L;				/* deleting single message */
	e  = mail_fetchstructure(state->mail_stream,mn_m2raw(msgmap,cur_msgno),
				 NULL); 
	mc = mail_elt(state->mail_stream, mn_m2raw(msgmap, cur_msgno));
	if(!e || !mc) {
	    q_status_message(SM_ORDER, 3, 4,
			 "Can't undelete message. Error accessing folder");
	    return;
	}
	else if(mc->deleted) {
	    clear_index_cache_ent(cur_msgno);
	    sequence = cpystr(long2string(mn_m2raw(msgmap, cur_msgno)));
	}
	else{
	    q_status_message(SM_ORDER, 0, 3, 
			     "Can't undelete a message that isn't deleted");
	    return;
	}
    }

    dprint(3,(debugfile, "Undeleted: msg %s\n",
	      (sequence) ? sequence : "already deleted"));

    if(sequence){
	mail_clearflag(state->mail_stream, sequence, "\\DELETED");
	fs_give((void **)&sequence);

	if(del_count == 1L){
	    update_titlebar_status();
	    q_status_message(SM_ORDER, 0, 3,
			    "Deletion mark removed, message won't be deleted");
	}
	else
	  q_status_message2(SM_ORDER, 0, 3,
			    "Deletion mark removed from %s message%s",
			    comatose(del_count), plural(del_count));

	if(state->io_error_on_stream) {
	    state->io_error_on_stream = 0;
	    mail_check(state->mail_stream); /* forces write */
	}
	else
	  check_point_change();
    }
}



/*----------------------------------------------------------------------
   Execute FLAG message command

  Args: state --  Various satate info
        msgmap --  map of c-client to local message numbers

 Result: with side effect of "current" message FLAG flag set or UNset

 ----*/
void
cmd_flag(state, msgmap, agg)
    struct pine *state;
    MSGNO_S     *msgmap;
    int		 agg;
{
    char	  *flagit, *seq, *screen_text[20], **exp, **p, *answer = NULL;
    long	   unflagged, flagged;
    void	 (*flagger)();
    MESSAGECACHE  *mc = NULL;
    struct	   flag_table  *fp;
    struct flag_screen flag_screen;
    static char   *flag_screen_text1[] = {
	"    Set desired flags for current message below.  An 'X' means set",
	"    it, and a ' ' means to unset it.  Choose \"Exit\" when finished.",
	NULL
    };
    static char   *flag_screen_text2[] = {
	"    Set desired flags below for selected messages.  A '?' means to",
	"    leave the flag unchanged, 'X' means to set it, and a ' ' means",
	"    to unset it.  Use the \"Return\" key to toggle, and choose",
	"    \"Exit\" when finished.",
	NULL
    };
    static char   *flag_screen_boiler_plate[] = {
	"",
	"            Set        Flag Name",
	"            ---   ----------------------",
	NULL
    };
    static struct  flag_table ftbl[] = {
	/*
	 * At some point when user defined flags are possible,
	 * it should just be a simple matter of grabbing this
	 * array from the heap and explicitly filling the
	 * non-system flags in at run time...
	 *  {NULL, h_flag_user, F_USER, 0, 0},
	 */
	{"Important", h_flag_important, F_FLAG, 0, 0},
	{"New",	  h_flag_new, F_SEEN, 0, 0},
	{"Answered",  h_flag_answered, F_ANS, 0, 0},
	{"Deleted",   h_flag_deleted, F_DEL, 0, 0},
	{NULL, NO_HELP, 0, 0, 0}
    };

    if(!(any_messages(msgmap, NULL, "to Flag")
	 && can_set_flag(state, "flag")))
      return;

    if(state->io_error_on_stream) {
	state->io_error_on_stream = 0;
	mail_check(state->mail_stream); /* forces write */
	return;
    }

    flag_screen.flag_table  = ftbl;
    flag_screen.explanation = screen_text;
    if(agg){
	if(!pseudo_selected(msgmap))
	  return;

	exp = flag_screen_text2;
	for(fp = ftbl; fp->name; fp++){
	    fp->set = CMD_FLAG_UNKN;		/* set to unknown */
	    fp->ukn = TRUE;
	}
    }
    else if(mc = mail_elt(state->mail_stream,
			  mn_m2raw(msgmap, mn_get_cur(msgmap)))){
	exp = flag_screen_text1;
	for(fp = &ftbl[0]; fp->name; fp++){
	    fp->ukn = 0;
	    fp->set = ((fp->flag == F_SEEN && !mc->seen)
		       || (fp->flag == F_DEL && mc->deleted)
		       || (fp->flag == F_FLAG && mc->flagged)
		       || (fp->flag == F_ANS && mc->answered))
			? CMD_FLAG_SET : CMD_FLAG_CLEAR;
	}
    }
    else{
	q_status_message(SM_ORDER | SM_DING, 3, 4,
			 "Error accessing message data");
	return;
    }

#ifdef _WINDOWS
    if (mswin_usedialog ()) {
	if (!os_flagmsgdialog (&ftbl[0]))
	  return;
    }
    else
#endif	    
    if(F_ON(F_FLAG_SCREEN_DFLT, ps_global)
       || !cmd_flag_prompt(state, &flag_screen)){
	screen_text[0] = "";
	for(p = &screen_text[1]; *exp; p++, exp++)
	  *p = *exp;

	for(exp = flag_screen_boiler_plate; *exp; p++, exp++)
	  *p = *exp;

	*p = NULL;

	flag_maintenance_screen(state, &flag_screen);
    }

    /* reaquire the elt pointer */
    mc = mail_elt(state->mail_stream,mn_m2raw(msgmap,mn_get_cur(msgmap)));

    for(fp = ftbl; fp->name; fp++){
	flagger = NULL;
	switch(fp->flag){
	  case F_SEEN:
	    if((!agg && fp->set != !mc->seen)
	       || (agg && fp->set != CMD_FLAG_UNKN)){
		flagit = "\\SEEN";
		if(fp->set){
		    flagger = mail_clearflag;
		    unflagged = F_SEEN;
		}
		else{
		    flagger = mail_setflag;
		    unflagged = F_UNSEEN;
		}
	    }

	    break;

	  case F_ANS:
	    if((!agg && fp->set != mc->answered)
	       || (agg && fp->set != CMD_FLAG_UNKN)){
		flagit = "\\ANSWERED";
		if(fp->set){
		    flagger = mail_setflag;
		    unflagged = F_UNANS;
		}
		else{
		    flagger = mail_clearflag;
		    unflagged = F_ANS;
		}
	    }

	    break;

	  case F_DEL:
	    if((!agg && fp->set != mc->deleted)
	       || (agg && fp->set != CMD_FLAG_UNKN)){
		flagit = "\\DELETED";
		if(fp->set){
		    flagger = mail_setflag;
		    unflagged = F_UNDEL;
		}
		else{
		    flagger = mail_clearflag;
		    unflagged = F_DEL;
		}
	    }

	    break;

	  case F_FLAG:
	    if((!agg && fp->set != mc->flagged)
	       || (agg && fp->set != CMD_FLAG_UNKN)){
		flagit = "\\FLAGGED";
		if(fp->set){
		    flagger = mail_setflag;
		    unflagged = F_UNFLAG;
		}
		else{
		    flagger = mail_clearflag;
		    unflagged = F_FLAG;
		}
	    }

	    break;

	  default:
	    break;
	}

	flagged = 0L;
	if(flagger && (seq = currentf_sequence(state->mail_stream, msgmap,
					       unflagged, &flagged, 1))){
	    (*flagger)(state->mail_stream, seq, flagit);
	    fs_give((void **)&seq);
	    if(flagged){
		sprintf(tmp_20k_buf, "%slagged%s%s%s%s%s message%s%s \"%s\"",
			(fp->set) ? "F" : "Unf",
			agg ? " " : "",
			agg ? long2string(flagged) : "",
			(agg && flagged != mn_total_cur(msgmap))
			  ? " (of " : "",
			(agg && flagged != mn_total_cur(msgmap))
			  ? comatose(mn_total_cur(msgmap)) : "",
			(agg && flagged != mn_total_cur(msgmap))
			  ? ")" : "",
			agg ? plural(flagged) : " ",
			agg ? "" : long2string(mn_get_cur(msgmap)),
			fp->name);
		q_status_message(SM_ORDER, 0, 2, answer = tmp_20k_buf);
	    }
	}
    }

    if(!answer)
      q_status_message(SM_ORDER, 0, 2, "No status flags changed.");

  fini:
    if(agg)
      restore_selected(msgmap);
}



/*----------------------------------------------------------------------
   Offer concise status line flag prompt 

  Args: state --  Various satate info
        flags -- flags to offer setting

 Result: TRUE if flag to set specified in flags struct or FALSE otw

 ----*/
int
cmd_flag_prompt(state, flags)
    struct pine	       *state;
    struct flag_screen *flags;
{
    int			r, setflag = 1;
    struct flag_table  *fp;
    static char *flag_text = "Flag New, Deleted, Answered, or Important ? ";
    static char *flag_text2	=
	"Flag NOT New, NOT Deleted, NOT Answered, or NOT Important ? ";
    static ESCKEY_S flag_text_opt[] = {
	{'n', 'n', "N", "New"},
	{'*', '*', "*", "Important"},
	{'d', 'd', "D", "Deleted"},
	{'a', 'a', "A", "Answered"},
	{'!', '!', "!", "Not"},
	{ctrl('T'), 10, "^T", "To Flag Details"},
	{-1, 0, NULL, NULL}
    };

    while(1){
	r = radio_buttons(setflag ? flag_text : flag_text2,
			  -FOOTER_ROWS(state), flag_text_opt, '*', 'x',
			  NO_HELP, RB_NORM);
	if(r == 'x')			/* ol'cancelrooney */
	  return(TRUE);
	else if(r == 10)		/* return and goto flag screen */
	  return(FALSE);
	else if(r == '!')		/* flip intention */
	  setflag = !setflag;
	else
	  break;
    }

    for(fp = flags->flag_table; fp->name; fp++)
      if((r == 'n' && fp->flag == F_SEEN)
	 || (r == '*' && fp->flag == F_FLAG)
	 || (r == 'd' && fp->flag == F_DEL)
	 || (r == 'a' && fp->flag == F_ANS)){
	  fp->set = setflag ? CMD_FLAG_SET : CMD_FLAG_CLEAR;
	  break;
      }

    return(TRUE);
}



/*----------------------------------------------------------------------
   Execute REPLY message command

  Args: state --  Various satate info
        msgmap --  map of c-client to local message numbers

 Result: reply sent or not

 ----*/
void
cmd_reply(state, msgmap, agg)
     struct pine *state;
     MSGNO_S     *msgmap;
     int	  agg;
{
    if(any_messages(msgmap, NULL, "to Reply to")){
#if	defined(DOS) && !defined(WIN32)
	flush_index_cache();		/* save room on PC */
#endif
	if(agg && !pseudo_selected(msgmap))
	  return;

	reply(state);

	if(agg)
	  restore_selected(msgmap);

	state->mangled_screen = 1;
    }
}



/*----------------------------------------------------------------------
   Execute FORWARD message command

  Args: state --  Various satate info
        msgmap --  map of c-client to local message numbers

 Result: selected message[s] forwarded or not

 ----*/
void
cmd_forward(state, msgmap, agg)
     struct pine *state;
     MSGNO_S     *msgmap;
     int	  agg;
{
    if(any_messages(msgmap, NULL, "to Forward")){
#if	defined(DOS) && !defined(WIN32)
	flush_index_cache();		/* save room on PC */
#endif
	if(agg && !pseudo_selected(msgmap))
	  return;

	forward(state);

	if(agg)
	  restore_selected(msgmap);

	if(state->anonymous)
	  state->mangled_footer = 1;
	else
	  state->mangled_screen = 1;
    }
}



/*----------------------------------------------------------------------
   Execute BOUNCE message command

  Args: state --  Various satate info
        msgmap --  map of c-client to local message numbers

 Result: selected message[s] bounced or not

 ----*/
void
cmd_bounce(state, msgmap, agg)
     struct pine *state;
     MSGNO_S     *msgmap;
     int	  agg;
{
    if(any_messages(msgmap, NULL, "to Bounce")){
#if	defined(DOS) && !defined(WIN32)
	flush_index_cache();			/* save room on PC */
#endif
	if(agg && !pseudo_selected(msgmap))
	  return;

	bounce(state);
	if(agg)
	  restore_selected(msgmap);

	state->mangled_footer = 1;
    }
}



/*----------------------------------------------------------------------
   Execute save message command: prompt for folder and call function to save

  Args: screen_line    --  Line on the screen to prompt on
        message        --  The MESSAGECACHE entry of message to save

 Result: The folder lister can be called to make selection; mangled screen set

   This does the prompting for the folder name to save to, possibly calling 
 up the folder display for selection of folder by user.                 
 ----*/
void
cmd_save(state, msgmap, screen_line, agg)
    struct pine *state;
    MSGNO_S     *msgmap;
    int          screen_line;
    int	  agg;
{
    static char	      folder[MAXFOLDER+1] = {'\0'};
    static CONTEXT_S *last_context = NULL;
    char	      newfolder[MAXFOLDER+1], prompt[MAXFOLDER+80],
		      nmsgs[32], *p;
    int		      rc, saveable_count = 0, del = 0, we_cancel = 0;
    long	      i;
    HelpType	      help;
    CONTEXT_S	     *cntxt = NULL, *tc;
    ESCKEY_S	      ekey[7];
    ENVELOPE	     *e = NULL;

    dprint(4, (debugfile, "\n - saving message -\n"));

    if(agg && !pseudo_selected(msgmap))
      return;

    if(mn_total_cur(msgmap) <= 1L){
	nmsgs[0] = '\0';
	e = mail_fetchstructure(state->mail_stream,
				mn_m2raw(msgmap, mn_get_cur(msgmap)), NULL);
	if(!e) {
	    q_status_message(SM_ORDER | SM_DING, 3, 4,
			     "Can't save message.  Error accessing folder");
	    restore_selected(msgmap);
	    return;
	}
    }
    else
      sprintf(nmsgs, "%s msgs ", comatose(mn_total_cur(msgmap)));

    /* start with the default save context */
    if((cntxt = default_save_context(state->context_list)) == NULL)
       cntxt = state->context_list;

    if(!e || ps_global->save_msg_rule == MSG_RULE_LAST
	  || ps_global->save_msg_rule == MSG_RULE_DEFLT){
	if(ps_global->save_msg_rule == MSG_RULE_LAST && last_context)
	  cntxt = last_context;
	else
	  strcpy(folder, ps_global->VAR_DEFAULT_SAVE_FOLDER);
    }
    else
      get_save_fldr_from_env(folder, sizeof(folder), e, state, msgmap);

    /* how many context's can be saved to... */
    for(tc = state->context_list; tc; tc = tc->next)
      if(!(tc->type&FTYPE_BBOARD))
        saveable_count++;

    /* set up extra command option keys */
    rc = 0;
    ekey[rc].ch      = ctrl('T');
    ekey[rc].rval    = 2;
    ekey[rc].name    = "^T";
    ekey[rc++].label = "To Fldrs";

    if(saveable_count > 1){
	ekey[rc].ch      = ctrl('P');
	ekey[rc].rval    = 10;
	ekey[rc].name    = "^P";
	ekey[rc++].label = "Prev Collection";

	ekey[rc].ch      = ctrl('N');
	ekey[rc].rval    = 11;
	ekey[rc].name    = "^N";
	ekey[rc++].label = "Next Collection";
    }

    if(F_ON(F_ENABLE_TAB_COMPLETE, ps_global)){
	ekey[rc].ch      = TAB;
	ekey[rc].rval    = 12;
	ekey[rc].name    = "TAB";
	ekey[rc++].label = "Complete";
    }

    if(saveable_count > 1){
	ekey[rc].ch      = KEY_UP;
	ekey[rc].rval    = 10;
	ekey[rc].name    = "";
	ekey[rc++].label = "";

	ekey[rc].ch      = KEY_DOWN;
	ekey[rc].rval    = 11;
	ekey[rc].name    = "";
	ekey[rc++].label = "";
    }

    ekey[rc].ch = -1;

    *newfolder = '\0';
    help = NO_HELP;
    ps_global->mangled_footer = 1;
    while(1) {
	/* only show collection number if more than one available */
	if(ps_global->context_list->next)
	  sprintf(prompt, "SAVE %sto folder in <%.25s> [%s] : ",
		  nmsgs, cntxt->label[0], folder);
	else
	  sprintf(prompt, "SAVE %sto folder [%s] : ", nmsgs, folder);

        rc = optionally_enter(newfolder, screen_line, 0, MAXFOLDER, 1, 0,
                              prompt, ekey, help, 0);

	if(rc == -1){
	    q_status_message(SM_ORDER | SM_DING, 3, 3,
			     "Error reading folder name");
	    rc = 1;
	    break;
	}
	else if(rc == 1){
	    cmd_cancelled("Save message");
	    break;
	}
	else if(rc == 2) {
	    int  f_rc;				/* folder_lister return val */
	    void (*redraw)() = ps_global->redrawer;

	    push_titlebar_state();
	    f_rc = folder_lister(ps_global,SaveMessage,cntxt,&cntxt,newfolder,
				 NULL, ps_global->context_list, NULL);
            ClearScreen();
	    pop_titlebar_state();
	    redraw_titlebar();
            if(ps_global->redrawer = redraw)	/* reset old value, and test */
              (*ps_global->redrawer)();

	    if(f_rc == 1 && F_ON(F_SELECT_WO_CONFIRM, ps_global))
	      break;
	}
	else if(rc == 3){
            help = (help == NO_HELP) ? h_oe_save : NO_HELP;
	}
	else if(rc == 10){			/* Previous collection */
	    CONTEXT_S *start;
	    start = cntxt;
	    tc    = cntxt;

	    while(1){
		if((tc = tc->next) == NULL)
		  tc = ps_global->context_list;

		if(tc == start)
		  break;

		if((tc->type&FTYPE_BBOARD) == 0)
		  cntxt = tc;
	    }
	}
	else if(rc == 11){			/* Next collection */
	    tc = cntxt;

	    do
	      if((cntxt = cntxt->next) == NULL)
		cntxt = ps_global->context_list;
	    while((cntxt->type&FTYPE_BBOARD) && cntxt != tc);
	}
	else if(rc == 12){			/* file name completion! */
	    if(!folder_complete(cntxt, newfolder))
	      Writechar(BELL, 0);

	}
	else if(rc != 4)
          break;
    }

    dprint(9, (debugfile, "rc = %d, \"%s\"  \"%s\"\n", rc, newfolder,folder));
    if(!(rc == 1 || (!*newfolder && !*folder) || (rc == 2 && !*newfolder))) {
	removing_trailing_white_space(newfolder);
	removing_leading_white_space(newfolder);

	last_context = cntxt;		/* remember for next time */
	if(!*newfolder)
	  strcpy(newfolder, folder);
	else
	  strcpy(folder, newfolder);

	/* is it a nickname?  If so, copy real name to newfolder */
	if(context_isambig(newfolder)
	   && (p = folder_is_nick(newfolder, cntxt->folders)))
	  strcpy(newfolder, p);

	del = !READONLY_FOLDER && F_OFF(F_SAVE_WONT_DELETE, ps_global);
	we_cancel = busy_alarm(1, NULL, NULL, 0);
	i = save(state, cntxt, newfolder, msgmap, del);
	if(we_cancel)
	  cancel_busy_alarm(0);

	if(i == mn_total_cur(msgmap)){
	    if(mn_total_cur(msgmap) <= 1L){
		if(ps_global->context_list->next && context_isambig(newfolder))
		  sprintf(tmp_20k_buf, 
			  "Message %s copied to \"%.15s%s\" in <%.15s%s>",
			  long2string(mn_get_cur(msgmap)), newfolder,
			  (strlen(newfolder) > 15) ? "..." : "",
			  cntxt->label[0],
			  (strlen(cntxt->label[0]) > 15) ? "..." : "");
		else
		  sprintf(tmp_20k_buf,
			  "Message %s copied to folder \"%.27s%s\"",
			  long2string(mn_get_cur(msgmap)), newfolder,
			  (strlen(newfolder) > 27) ? "..." : "");
	    }
	    else
	      sprintf(tmp_20k_buf, "%s messages saved",
		      comatose(mn_total_cur(msgmap)));

	    if(del)
	      strcat(tmp_20k_buf, " and deleted");

	    q_status_message(SM_ORDER, 0, 3, tmp_20k_buf);

	    if(!agg && F_ON(F_SAVE_ADVANCES, state)){
		mn_inc_cur(state->mail_stream, msgmap);
	    }
	}
    }
    else
      cmd_cancelled("No folder named; save message");

    if(agg)					/* straighten out fakes */
      restore_selected(msgmap);

    if(del)
      update_titlebar_status();			/* make sure they see change */
}


/*----------------------------------------------------------------------
   Grope through envelope to find default folder name to save to

  Args: fbuf   --  Buffer to return result in
        nfbuf  --  Size of fbuf
        e      --  The envelope to look in
        state  --  Usual pine state
        msgmap --  Message map of currently selected messages

 Result: The appropriate default folder name is copied into fbuf.
 ----*/
void
get_save_fldr_from_env(fbuf, nfbuf, e, state, msgmap)
    char         fbuf[];
    int          nfbuf;
    ENVELOPE    *e;
    struct pine *state;
    MSGNO_S     *msgmap;
{
    char     fakedomain[2];
    ADDRESS *tmp_adr = NULL;
    char     buf[max(MAXFOLDER,MAX_NICKNAME) + 1];
    char    *bufp;
    char    *folder_name;
    static char botch[] = "programmer botch, unknown message save rule";
    unsigned save_msg_rule;

    if(!e)
      return;

    /* copy this because we might change it below */
    save_msg_rule = state->save_msg_rule;

    /* first get the relevant address to base the folder name on */
    switch(save_msg_rule){
      case MSG_RULE_FROM:
      case MSG_RULE_NICK_FROM:
      case MSG_RULE_FCC_FROM:
      case MSG_RULE_FCC_FROM_DEF:
      case MSG_RULE_NICK_FROM_DEF:
        tmp_adr = e->from ? e->from : e->sender ? e->sender : NULL;
	break;

      case MSG_RULE_SENDER:
      case MSG_RULE_NICK_SENDER:
      case MSG_RULE_FCC_SENDER:
      case MSG_RULE_NICK_SENDER_DEF:
      case MSG_RULE_FCC_SENDER_DEF:
        tmp_adr = e->sender ? e->sender : e->from ? e->from : NULL;
	break;

      case MSG_RULE_RECIP:
      case MSG_RULE_NICK_RECIP:
      case MSG_RULE_FCC_RECIP:
      case MSG_RULE_NICK_RECIP_DEF:
      case MSG_RULE_FCC_RECIP_DEF:
	/* news */
	if(state->mail_stream
		&& state->mail_stream->mailbox[0] == '*'){ /* IS_NEWS ? */
	    char *tmp_a_string, *ng_name;

	    fakedomain[0] = '@';
	    fakedomain[1] = '\0';

	    /* find the news group name */
	    ng_name = (state->mail_stream->mailbox[1] == '{')
			   ? strchr(state->mail_stream->mailbox, '}') + 1
			   : &state->mail_stream->mailbox[1];
	    /* copy this string so rfc822_parse_adrlist can't blast it */
	    tmp_a_string = cpystr(ng_name);
	    /* make an adr */
	    rfc822_parse_adrlist(&tmp_adr, tmp_a_string, fakedomain);
	    fs_give((void **)&tmp_a_string);
	    if(tmp_adr && tmp_adr->host && tmp_adr->host[0] == '@')
	      tmp_adr->host[0] = '\0';
	}
	/* not news */
	else{
	    static char *fields[] = {"Resent-To", NULL};
	    char *extras, *values[sizeof(fields)/sizeof(fields[0])];

	    extras = xmail_fetchheader_lines(state->mail_stream,
					  mn_m2raw(msgmap, mn_get_cur(msgmap)),
					  fields);
	    if(extras){
		long i;

		memset(values, 0, sizeof(fields));
		simple_header_parse(extras, fields, values);
		fs_give((void **)&extras);

		for(i = 0; i < sizeof(fields)/sizeof(fields[0]); i++)
		  if(values[i]){
		      if(tmp_adr)		/* take last matching value */
			mail_free_address(&tmp_adr);

		      /* build a temporary address list */
		      fakedomain[0] = '@';
		      fakedomain[1] = '\0';
		      rfc822_parse_adrlist(&tmp_adr, values[i], fakedomain);
		      fs_give((void **)&values[i]);
		  }
	    }

	    if(!tmp_adr)
	      tmp_adr = e->to ? e->to : NULL;
	}

	break;
      
      default:
	panic(botch);
	break;
    }

    /* For that address, lookup the fcc or nickname from address book */
    switch(save_msg_rule){
      case MSG_RULE_NICK_FROM:
      case MSG_RULE_NICK_SENDER:
      case MSG_RULE_NICK_RECIP:
      case MSG_RULE_FCC_FROM:
      case MSG_RULE_FCC_SENDER:
      case MSG_RULE_FCC_RECIP:
      case MSG_RULE_NICK_FROM_DEF:
      case MSG_RULE_NICK_SENDER_DEF:
      case MSG_RULE_NICK_RECIP_DEF:
      case MSG_RULE_FCC_FROM_DEF:
      case MSG_RULE_FCC_SENDER_DEF:
      case MSG_RULE_FCC_RECIP_DEF:
	switch(save_msg_rule){
	  case MSG_RULE_NICK_FROM:
	  case MSG_RULE_NICK_SENDER:
	  case MSG_RULE_NICK_RECIP:
	  case MSG_RULE_NICK_FROM_DEF:
	  case MSG_RULE_NICK_SENDER_DEF:
	  case MSG_RULE_NICK_RECIP_DEF:
	    bufp = get_nickname_from_addr(tmp_adr, buf);
	    break;

	  case MSG_RULE_FCC_FROM:
	  case MSG_RULE_FCC_SENDER:
	  case MSG_RULE_FCC_RECIP:
	  case MSG_RULE_FCC_FROM_DEF:
	  case MSG_RULE_FCC_SENDER_DEF:
	  case MSG_RULE_FCC_RECIP_DEF:
	    bufp = get_fcc_from_addr(tmp_adr, buf);
	    break;
	}

	if(bufp && *bufp){
	    fbuf[nfbuf - 1] = '\0';
	    strncpy(fbuf, bufp, nfbuf - 1);
	}
	else
	  /* fall back to non-nick/non-fcc version of rule */
	  switch(save_msg_rule){
	    case MSG_RULE_NICK_FROM:
	    case MSG_RULE_FCC_FROM:
	      save_msg_rule = MSG_RULE_FROM;
	      break;

	    case MSG_RULE_NICK_SENDER:
	    case MSG_RULE_FCC_SENDER:
	      save_msg_rule = MSG_RULE_SENDER;
	      break;

	    case MSG_RULE_NICK_RECIP:
	    case MSG_RULE_FCC_RECIP:
	      save_msg_rule = MSG_RULE_RECIP;
	      break;
	    
	    default:
	      strcpy(fbuf, ps_global->VAR_DEFAULT_SAVE_FOLDER);
	      break;
	  }

	break;
    }

    /* get the username out of the mailbox for this address */
    switch(save_msg_rule){
      case MSG_RULE_FROM:
      case MSG_RULE_SENDER:
      case MSG_RULE_RECIP:
	/*
	 * Fish out the user's name from the mailbox portion of
	 * the address and put it in folder.
	 */
	folder_name = (tmp_adr && tmp_adr->mailbox && tmp_adr->mailbox[0])
		      ? tmp_adr->mailbox : NULL;
	if(!get_uname(folder_name, fbuf, nfbuf))
	  strcpy(fbuf, ps_global->VAR_DEFAULT_SAVE_FOLDER);

	break;
    }

    if(tmp_adr && tmp_adr != e->from && tmp_adr != e->sender
       && tmp_adr != e->to)
      mail_free_address(&tmp_adr);
}



/*----------------------------------------------------------------------
        Do the work of actually saving messages to a folder

    Args: state -- pine state struct (for stream pointers)
	  context -- context to interpret name in if not fully qualified
	  folder  -- The folder to save the message in
          msgmap -- message map of currently selected messages
	  delete -- whether or not to delete messages saved

  Result: Returns number of messages saved

  Note: There's a bit going on here; temporary clearing of deleted flags
	since they are *not* preserved, picking or creating the stream for
	copy or append, and dealing with errors...
 ----*/
long
save(state, context, folder, msgmap, delete)
    struct pine  *state;
    CONTEXT_S    *context;
    char         *folder;
    MSGNO_S	 *msgmap;
    int		  delete;
{
    int		  rv, rc, j, our_stream = 0, cancelled = 0;
    char	 *tmp, *save_folder, *seq, flags[64], date[64];
    long	  i, nmsgs, mlen;
    STORE_S	 *so = NULL;
    STRING	  msg;
    MAILSTREAM	 *save_stream = NULL;
    MESSAGECACHE *mc;
#if	defined(DOS) && !defined(WIN32)
    struct {					/* hack! stolen from dawz.c */
	int fd;
	unsigned long pos;
    } d;
    extern STRINGDRIVER dawz_string;
#define	SAVE_TMP_TYPE		FileStar
#else
#define	SAVE_TMP_TYPE		CharStar
#endif

    save_folder = (strucmp(folder, state->inbox_name) == 0)
		    ? state->VAR_INBOX_PATH : folder;

    /*
     * Compare the current stream (the save's source) and the stream
     * the destination folder will need...
     */
    save_stream = context_same_stream(context->context, save_folder,
				      state->mail_stream);

    /*
     * Here we try to use context_copy if at all possible.  This should 
     * preserve flags, and, in the remote case, let the server move
     * the bits without network traffic.  It also turns out to be 
     * necessary talking to certain, known imapd's that don't/can't
     * provide APPEND...
     *
     * So, if we don't yet have a save_stream, then the source folder
     * and the destination aren't remote *and* the same server, so we
     * want to see if they're both local *and* exist (or will exist)
     * in the same local driver...
     *
     * NOTE: If the feature to aggregate the COPY sequence is set
     *	     AND the folder's not in good ol' arrival order, don't
     *	     even bother with local test since we know the current
     *	     c-client will unwrap our ordering...
     */
    if(!save_stream && (F_OFF(F_AGG_SEQ_COPY, ps_global)
			|| (mn_get_sort(msgmap) == SortArrival
			    && !mn_get_revsort(msgmap))))
      save_stream = context_same_driver(context->context, save_folder,
					state->mail_stream);

    /* if needed, this'll get set in mm_notify */
    ps_global->try_to_create = 0;
    rv = rc = 0;
    nmsgs = 0L;

    /*
     * At this point, if we found a save_stream, then the current stream
     * is either remote, or local with both current folder and destination
     * in the same driver...
     */
    if(save_stream){
	char *dseq, *oseq;

	if(dseq = currentf_sequence(state->mail_stream, msgmap, F_DEL, NULL,0))
	  mail_clearflag(state->mail_stream, dseq, "\\DELETED");

	seq = currentf_sequence(state->mail_stream, msgmap, 0L, &nmsgs, 0);
	if(F_ON(F_AGG_SEQ_COPY, ps_global)){
	    /*
	     * currentf_sequence() above lit all the elt "sequence"
	     * bits of the interesting messages.  Now, build a sequence
	     * that preserves sort order...
	     */
	    oseq = build_sequence(state->mail_stream, msgmap, &nmsgs);
	}
	else{
	    oseq  = NULL;			/* no single sequence! */
	    nmsgs = 0L;
	    i = mn_first_cur(msgmap);		/* set first to copy */
	}

	do{
	    while(!(rv = (int) context_copy(context->context, save_stream,
				oseq ? oseq : long2string(mn_m2raw(msgmap, i)),
				save_folder))){
		if(rc++ || !ps_global->try_to_create)   /* abysmal failure! */
		  break;			/* c-client returned error? */

		if((context->use & CNTXT_INCMNG)
		   && context_isambig(save_folder)){
		    q_status_message(SM_ORDER, 3, 5,
		   "Can only save to existing folders in Incoming Collection");
		    break;
		}

		ps_global->try_to_create = 0;	/* reset for next time */
		if((j = create_for_save(save_stream,context,save_folder)) < 1){
		    if(j < 0)
		      cancelled = 1;		/* user cancels */

		    break;
		}
	    }

	    if(rv){				/* failure or finished? */
		if(oseq)			/* all done? */
		  break;
		else
		  nmsgs++;
	    }
	    else{				/* failure! */
		if(oseq)
		  nmsgs = 0L;			/* nothing copy'd */

		break;
	    }
	}
	while((i = mn_next_cur(msgmap)) > 0L);

	if(rv && delete){			/* delete those saved */
	    mail_setflag(state->mail_stream, seq, "\\DELETED");
	    for(i = mn_first_cur(msgmap); i > 0L; i = mn_next_cur(msgmap))
	      clear_index_cache_ent(i);
	}
	else if(dseq)				/* or restore previous state */
	  mail_setflag(state->mail_stream, dseq, "\\DELETED");

	if(dseq)				/* clean up */
	  fs_give((void **)&dseq);

	if(oseq)
	  fs_give((void **)&oseq);

	fs_give((void **)&seq);
    }
    else{
	/*
	 * Looks like the current mail stream and the stream needed by
	 * the destination folder don't match, so we might as well see
	 * if there's another pine stream to piggy back the APPEND of
	 * the destination on.  Create our own if we need to...
	 */
	save_stream = context_same_stream(context->context, save_folder,
					  state->inbox_stream);
	if(!save_stream
	   && ((context_isambig(save_folder) && IS_REMOTE(context->context))
	       || IS_REMOTE(save_folder))
	   && (save_stream=context_open(context->context,NULL,save_folder,
					OP_HALFOPEN)))
	  our_stream = 1;

	/*
	 * Allocate a storage object to temporarily store the message
	 * object in.  Below it'll get mapped into a c-client STRING struct
	 * in preparation for handing off to context_append...
	 */
	if(!(so = so_get(SAVE_TMP_TYPE, NULL, WRITE_ACCESS))){
	    dprint(1, (debugfile, "Can't allocate store for save: %s\n",
		       error_description(errno)));
	    q_status_message(SM_ORDER | SM_DING, 3, 4,
			     "Problem creating space for message text.");
	}

	/* make sure flags for any message we might touch are valid */
	FETCH_ALL_FLAGS(state->mail_stream);

	/*
	 * If we're supposed set the deleted flag, clear the elt bit
	 * we'll use to build the sequence later...
	 */
	if(delete)
	  for(i = 1L; i <= state->mail_stream->nmsgs; i++)
	    mail_elt(state->mail_stream, i)->sequence = 0;

	nmsgs = 0L;
	for(i = mn_first_cur(msgmap); so && i > 0L; i = mn_next_cur(msgmap)){
	    so_truncate(so, 0L);

	    /* store flags before the fetch so UNSEEN bit isn't flipped */
	    mc = mail_elt(state->mail_stream, mn_m2raw(msgmap, i));
	    flag_string(mc, F_ANS|F_FLAG|F_SEEN, flags);

	    if(tmp = mail_fetchheader(state->mail_stream, mn_m2raw(msgmap,i))){
		/*
		 * If the MESSAGECACHE element doesn't already have it, 
		 * parse the "internal date" by hand since fetchstructure
		 * hasn't been called yet for this particular message, and
		 * we don't want to call it now just to get the date since
		 * the full header has what we want.  Likewise, don't even
		 * think about calling mail_fetchfast either since it also
		 * wants to load mc->rfc822_size (which we could actually
		 * use but...), which under some drivers is *very*
		 * expensive to acquire (can you say NNTP?)...
		 */
		mc = mail_elt(state->mail_stream, mn_m2raw(msgmap, i));
		if(mc->day)
		  mail_date(date, mc);
		else
		  saved_date(date, tmp);

		if(!so_puts(so, tmp))
		  break;
	    }
	    else
	      break;				/* fetchtext writes error */

#if	defined(DOS) && !defined(WIN32)
	    /*
	     * Set append file and install dos_gets so message text
	     * is fetched directly to disk.
	     */
	    mail_parameters(state->mail_stream, SET_GETS, (void *)dos_gets);
	    append_file = (FILE *) so_text(so);
	    mail_gc(state->mail_stream, GC_TEXTS);

	    if(!(tmp = mail_fetchtext(state->mail_stream, mn_m2raw(msgmap,i))))
	      break;

	    /*
	     * Clean up after our DOS hacks...
	     */
	    append_file = NULL;
	    mail_parameters(state->mail_stream, SET_GETS, (void *)NULL);
	    mail_gc(state->mail_stream, GC_TEXTS);
#else
	    if(!((tmp = mail_fetchtext(state->mail_stream, mn_m2raw(msgmap,i)))
		 && so_puts(so, tmp)))
	      break;
#endif

	    so_seek(so, 0L, 0);			/* just in case */

	    /*
	     * Set up a c-client string driver so we can hand the
	     * collected text down to mail_append.
	     *
	     * NOTE: We only test the size if and only if we already
	     *	     have it.  See, in some drivers, especially under
	     *	     dos, its too expensive to get the size (full
	     *	     header and body text fetch plus MIME parse), so
	     *	     we only verify the size if we already know it.
	     */
	    mc = mail_elt(state->mail_stream, mn_m2raw(msgmap, i));
#if	defined(DOS) && !defined(WIN32)
	    d.fd  = fileno((FILE *)so_text(so));
	    d.pos = 0L;
	    mlen = filelength(d.fd);
	    if(mc->rfc822_size && mlen < mc->rfc822_size){
		char buf[128];

		sprintf(buf, "Message to save shrank!  (#%ld: %ld --> %ld)",
			mc->msgno, mc->rfc822_size, mlen);
		q_status_message(SM_ORDER | SM_DING, 3, 4, buf);
		dprint(1, (debugfile, "BOTCH: %s\n", buf));
		break;
	    }

	    INIT(&msg, dawz_string, (void *)&d, mlen);
#else
	    mlen = strlen((char *)so_text(so));
	    if(mc->rfc822_size && mlen < mc->rfc822_size){
		char buf[128];

		sprintf(buf, "Message to save shrank!  (#%ld: %ld --> %ld)",
			mc->msgno, mc->rfc822_size, mlen);
		q_status_message(SM_ORDER | SM_DING, 3, 4, buf);
		dprint(1, (debugfile, "BOTCH: %s\n", buf));
		break;
	    }

	    INIT(&msg, mail_string, (void *)so_text(so), mlen);
#endif
	    while(!(rv = (int) context_append_full(context->context,
						   save_stream, save_folder,
						   flags, *date ? date : NULL,
						   &msg))){
		if(rc++ || !ps_global->try_to_create) /* abysmal failure! */
		  break;			/* c-client returned error? */

		if((context->use & CNTXT_INCMNG)
		   && context_isambig(save_folder)){
		    q_status_message(SM_ORDER, 3, 5,
	       "Can only save to existing folders in Incoming Collection");
		    break;
		}

		ps_global->try_to_create = 0;	/* reset for next time */
		if((j = create_for_save(save_stream,context,save_folder)) < 1){
		    if(j < 0)
		      cancelled = 1;		/* user cancelled */

		    break;
		}

		SETPOS((&msg), 0L);		/* reset string driver */
	    }

	    if(!rv)
	      break;				/* horrendous failure */

	    /*
	     * Success!  Count it, and if it's not already deleted and 
	     * it's supposed to be, mark it to get deleted later...
	     */
	    nmsgs++;
	    if(delete){
		mc = mail_elt(state->mail_stream, mn_m2raw(msgmap, i));
		if(!mc->deleted){
		    mc->sequence = 1;		/* mark for later deletion */
		    clear_index_cache_ent(i);
		    check_point_change();
		}
	    }
	}

	if(our_stream)
	  mail_close(save_stream);

	if(so)
	  so_give(&so);

	if(delete && (seq = build_sequence(state->mail_stream, NULL, NULL))){
	    mail_setflag(state->mail_stream, seq, "\\DELETED");
	    fs_give((void **)&seq);
	}
    }

    ps_global->try_to_create = 0;		/* reset for next time */
    if(!cancelled && nmsgs < mn_total_cur(msgmap)){
	dprint(1, (debugfile, "FAILED save of msg %s (c-client sequence #)\n",
		   long2string(mn_m2raw(msgmap, mn_get_cur(msgmap)))));
	if(mn_total_cur(msgmap) > 1L){
	    sprintf(tmp_20k_buf,
		    "%ld of %ld messages saved before error occurred",
		    nmsgs, mn_total_cur(msgmap));
	    dprint(1, (debugfile, "\t%s\n", tmp_20k_buf));
	    q_status_message(SM_ORDER, 3, 5, tmp_20k_buf);
	}
    }

    return(nmsgs);
}



/*----------------------------------------------------------------------
    Offer to create a non-existant folder to copy message[s] into

   Args: stream -- stream to use for creation
	 context -- context to create folder in
	 name -- name of folder to create

 Result: 0 if create failed (c-client writes error)
	 1 if create successful
	-1 if user declines to create folder
 ----*/
int
create_for_save(stream, context, folder)
    MAILSTREAM *stream;
    CONTEXT_S  *context;
    char       *folder;
{
    if(ps_global->context_list->next && context_isambig(folder)){
	sprintf(tmp_20k_buf,
		"Folder \"%.15s%s\" in <%.15s%s> doesn't exist. Create",
		folder, (strlen(folder) > 15) ? "..." : "",
		context->label[0],
		(strlen(context->label[0]) > 15) ? "..." : "");
    }
    else
      sprintf(tmp_20k_buf,"Folder \"%.40s\" doesn't exist.  Create", folder);

    if(want_to(tmp_20k_buf, 'y', 'n', NO_HELP, 0, 0) != 'y'){
	cmd_cancelled("Save message");
	return(-1);
    }

    return(context_create(context->context, NULL, folder));
}



/*----------------------------------------------------------------------
  Set 

   Args: mc -- message cache element to dig the flags out of

 Result: Malloc'd string representing flags set in mc
 ----*/
void
flag_string(mc, flags, flagbuf)
    MESSAGECACHE *mc;
    long	  flags;
    char	 *flagbuf;
{
    char *p;

    *(p = flagbuf) = '\0';

    if((flags & F_DEL) && mc->deleted)
      sstrcpy(&p, "\\DELETED ");

    if((flags & F_ANS) && mc->answered)
      sstrcpy(&p, "\\ANSWERED ");

    if((flags & F_FLAG) && mc->flagged)
      sstrcpy(&p, "\\FLAGGED ");

    if((flags & F_SEEN) && mc->seen)
      sstrcpy(&p, "\\SEEN ");

    if(p != flagbuf)
      *--p = '\0';			/* tie off tmp_20k_buf   */
}



/*----------------------------------------------------------------------
   Save() helper function to create canonical date string from given header

   Args: date -- buf to recieve canonical date string
	 header -- rfc822 header to fish date string from

 Result: date filled with canonicalized date in header, or null string
 ----*/
void
saved_date(date, header)
    char *date, *header;
{
    char	 *d, *p, c;
    MESSAGECACHE  elt;

    *date = '\0';
    if((toupper((unsigned char)(*(d = header)))
	== 'D' && !strncmp(d, "date:", 5))
       || (d = srchstr(header, "\015\012date:"))){
	for(d += 7; *d == ' '; d++)
	  ;					/* skip white space */

	if(p = strstr(d, "\015\012")){
	    for(; p > d && *p == ' '; p--)
	      ;					/* skip white space */

	    c  = *p;
	    *p = '\0';				/* tie off internal date */
	}

	if(mail_parse_date(&elt, d))		/* let c-client normalize it */
	  mail_date(date, &elt);

	if(p)					/* restore header */
	  *p = c;
    }
}



/*----------------------------------------------------------------------
    Export a message to a plain file in users home directory

   Args:  q_line -- screen line to prompt on
         message -- MESSAGECACHE enrty of message to export

 Result: 
 ----*/
void
cmd_export(state, msgmap, q_line, agg)
    struct pine *state;
    MSGNO_S     *msgmap;
    int          q_line;
    int		 agg;
{
    HelpType  help;
    char      filename[MAXPATH+1], full_filename[MAXPATH+1],*ill;
    int       rc, new_file, failure = 0, orig_errno, over = 0;
    ENVELOPE *env;
    BODY     *b;
    long      i, count = 0L, start_of_append;
    gf_io_t   pc;
    STORE_S  *store;
    struct variable *vars = ps_global->vars;
    static ESCKEY_S export_opts[] = {
	{ctrl('T'), 10, "^T", "To Files"},
	{-1, 0, NULL, NULL},
	{-1, 0, NULL, NULL},
	{-1, 0, NULL, NULL}};

    if(ps_global->restricted){
	q_status_message(SM_ORDER, 0, 3,
	    "Pine demo can't export messages to files");
	return;
    }

    if(agg && !pseudo_selected(msgmap))
      return;

    i = 0;

#if	!defined(DOS) && !defined(MAC) && !defined(OS2)
    if(ps_global->VAR_DOWNLOAD_CMD && ps_global->VAR_DOWNLOAD_CMD[0]){
	export_opts[++i].ch  = ctrl('V');
	export_opts[i].rval  = 12;
	export_opts[i].name  = "^V";
	export_opts[i].label = "Downld Msg";
    }
#endif	/* !(DOS || MAC) */

    if(F_ON(F_ENABLE_TAB_COMPLETE,ps_global)){
	export_opts[++i].ch  =  ctrl('I');
	export_opts[i].rval  = 11;
	export_opts[i].name  = "TAB";
	export_opts[i].label = "Complete";
    }

    export_opts[++i].ch = -1;

    help = NO_HELP;
    filename[0] = '\0';
    while(1) {
        char prompt[100];
#ifdef	DOS
        (void)strcpy(prompt, "File to save message text in: ");
#else
	sprintf(prompt, "EXPORT: (copy message) to file in %s directory: ",
	        F_ON(F_USE_CURRENT_DIR, ps_global) ? "current"
		: VAR_OPER_DIR ? VAR_OPER_DIR : "home");
#endif
        rc = optionally_enter(filename, q_line, 0, MAXPATH, 1, 0,
                 prompt, export_opts, help, 0);

        /*--- Help ----*/
	if(rc == 10){			/* ^T to files */
	    if(filename[0])
	      strcpy(full_filename, filename);
	    else if(F_ON(F_USE_CURRENT_DIR, ps_global))
	      (void) getcwd(full_filename, MAXPATH);
	    else if(VAR_OPER_DIR)
	      build_path(full_filename, VAR_OPER_DIR, filename);
	    else
              build_path(full_filename, ps_global->home_dir, filename);

	    rc = file_lister("EXPORT", full_filename, filename, TRUE, FB_SAVE);

	    if(rc == 1){
		strcat(full_filename, "/");
		strcat(full_filename, filename);
		break;
	    }
	    else
	      continue;
	}
	else if(rc == 11){		/* tab completion */
	    char dir[MAXPATH], *fn;
	    int  l = MAXPATH;

	    dir[0] = '\0';
	    if(*filename && (fn = last_cmpnt(filename))){
		l -= fn - filename;
		if(is_absolute_path(filename)){
		    strncpy(dir, filename, fn - filename);
		    dir[fn - filename] = '\0';
		}
		else{
		    char *p = NULL;
		    sprintf(full_filename, "%.*s", fn - filename, filename);
		    build_path(dir, F_ON(F_USE_CURRENT_DIR, ps_global)
				       ? p = (char *) getcwd(NULL, MAXPATH)
				       : VAR_OPER_DIR ? VAR_OPER_DIR
						      : ps_global->home_dir,
			       full_filename);
		    if(p)
		      free(p);
		}
	    }
	    else{
		fn = filename;
		if(F_ON(F_USE_CURRENT_DIR, ps_global))
		  (void) getcwd(dir, MAXPATH);
		else if(VAR_OPER_DIR)
		  strcpy(dir, VAR_OPER_DIR);
		else
		  strcpy(dir, ps_global->home_dir);
	    }

	    if(!pico_fncomplete(dir, fn, l - 1))
	      Writechar(BELL, 0);

	    continue;
	}
#if	!defined(DOS) && !defined(MAC) && !defined(OS2)
	else if(rc == 12){			/* download current messge */
	    char     cmd[MAXPATH], *tfp, *errstr = NULL;
	    int	     next = 0;
	    PIPE_S  *syspipe;
	    STORE_S *so;
	    gf_io_t  pc;

	    if(ps_global->restricted){
		q_status_message(SM_ORDER | SM_DING, 3, 3,
				 "Download disallowed in restricted mode");
		goto fini;
	    }

	    tfp = temp_nam(NULL, "pd");
	    build_updown_cmd(cmd, ps_global->VAR_DOWNLOAD_CMD_PREFIX,
			     ps_global->VAR_DOWNLOAD_CMD, tfp);
	    dprint(1, (debugfile, "Download cmd called: \"%s\"\n", cmd));
	    if(so = so_get(FileStar, tfp, WRITE_ACCESS)){
		gf_set_so_writec(&pc, so);

		for(i = mn_first_cur(msgmap); i > 0L; i = mn_next_cur(msgmap))
		  if(!(env = mail_fetchstructure(state->mail_stream,
						 mn_m2raw(msgmap, i), &b))
		     || !bezerk_delimiter(env, pc, next++)
		     || !format_message(mn_m2raw(msgmap, mn_get_cur(msgmap)),
					env, b, FM_NEW_MESS|FM_DO_PRINT, pc)){
		      q_status_message(SM_ORDER | SM_DING, 3, 3,
			       errstr = "Error writing tempfile for download");
		      break;
		  }

		so_give(&so);			/* close file */

		if(!errstr){
		    if(syspipe = open_system_pipe(cmd, NULL, NULL,
						  PIPE_USER | PIPE_RESET))
		      (void) close_system_pipe(&syspipe);
		    else
		      q_status_message(SM_ORDER | SM_DING, 3, 3,
				    errstr = "Error running download command");
		}

		unlink(tfp);
	    }
	    else
	      q_status_message(SM_ORDER | SM_DING, 3, 3,
			     errstr = "Error building temp file for download");

	    fs_give((void **)&tfp);
	    if(!errstr)
	      q_status_message(SM_ORDER, 0, 3, "Download Command Completed");

	    goto fini;
	}
#endif	/* !(DOS || MAC) */
	else if(rc == 3) {
            help = (help == NO_HELP) ? h_oe_export : NO_HELP;
            continue;
        }

        removing_trailing_white_space(filename);
        removing_leading_white_space(filename);
        if(rc == 1 || filename[0] == '\0') {
	    cmd_cancelled("Export message");
	    goto fini;
        }

        if(rc == 4)
          continue;


        /*-- check out and expand file name. give possible error messages --*/
        strcpy(full_filename, filename);
        if((ill = filter_filename(filename)) != NULL) {
            q_status_message1(SM_ORDER | SM_DING, 3, 3, "%s", ill);
            continue;
        }

#if defined(DOS) || defined(OS2)
	if(is_absolute_path(full_filename)){
	    fixpath(full_filename, MAXPATH);
	}
#else
        if(full_filename[0] == '~') {
            if(fnexpand(full_filename, sizeof(full_filename)) == NULL) {
		char *p = last_cmpnt(full_filename);
		if(p != NULL)
		  *p = '\0';

		q_status_message1(SM_ORDER | SM_DING, 3, 3,
			      "Error expanding file name: \"%s\" unknown user",
			      full_filename);
		continue;
	    }
	}
#endif

	if(!is_absolute_path(full_filename)){
	    if(F_ON(F_USE_CURRENT_DIR, ps_global))
              (void)strcpy(full_filename, filename);
	    else if(VAR_OPER_DIR)
	      build_path(full_filename, VAR_OPER_DIR, filename);
	    else
              build_path(full_filename, ps_global->home_dir, filename);
        }

        break; /* Must have got an OK file name */

    }


    if(VAR_OPER_DIR && !in_dir(VAR_OPER_DIR, full_filename)){
	q_status_message1(SM_ORDER, 0, 2, "Can't export to file outside of %s",
			  VAR_OPER_DIR);
	goto fini;
    }


    /* ---- full_filename already contains the absolute path ---*/
    if(!can_access(full_filename, ACCESS_EXISTS)) {
        char prompt[100];
	static ESCKEY_S access_opts[] = {
	    {'o', 'o', "O", "Overwrite"},
	    {'a', 'a', "A", "Append"},
	    {-1, 0, NULL, NULL}};

	rc = strlen(filename);
        sprintf(prompt,
		"File \"%s%s\" already exists.  Overwrite or append it ? ",
		(rc > 20) ? "..." : "",
                filename + ((rc > 20) ? rc - 20 : 0));
	switch(radio_buttons(prompt, -FOOTER_ROWS(state), access_opts,
			     'a', 'x', NO_HELP, RB_NORM)){
	  case 'o' :
	    new_file = 1;
	    over = 1;
	    if(unlink(full_filename) < 0){	/* BUG: breaks links */
		q_status_message2(SM_ORDER | SM_DING, 3, 5,
				  "Error deleting old %s: %s",
				  full_filename, error_description(errno));
		goto fini;
	    }

	    break;

	  case 'a' :
	    new_file = 0;
 	    over = -1;
	    break;

	  case 'x' :
	  default :
	    cmd_cancelled("Export message");
	    goto fini;
	}
    }
    else
      new_file = 1;

    dprint(5, (debugfile, "Opening file \"%s\" for export\n", full_filename));

    if(!(store = so_get(FileStar, full_filename, WRITE_ACCESS))){
        q_status_message2(SM_ORDER | SM_DING, 3, 4,
			  "Error opening file \"%s\" to export message: %s",
                          full_filename, error_description(errno));
	goto fini;
    }
    else
      gf_set_so_writec(&pc, store);

    for(i = mn_first_cur(msgmap); i > 0L; i = mn_next_cur(msgmap), count++){
	env = mail_fetchstructure(state->mail_stream, mn_m2raw(msgmap, i), &b);
	if(!env) {
	    q_status_message(SM_ORDER | SM_DING, 3, 4,
			  "Can't export message. Error accessing mail folder");
	    goto fini;
	}

	start_of_append = ftell((FILE *)so_text(store));
	if(!bezerk_delimiter(env, pc, new_file)
	   || !format_message(mn_m2raw(msgmap, i), env, b,
			      (FM_NEW_MESS|FM_DO_PRINT), pc)){
	    orig_errno = errno;		/* save incase things are really bad */
	    failure    = 1;		/* pop out of here */
	    break;
	}
    }

    so_give(&store);				/* release storage */
    if(failure){
#ifdef	DOS
	chsize(fileno((FILE *)so_text(store)), start_of_append);
#else
	truncate(full_filename, start_of_append);
#endif
	dprint(1, (debugfile, "FAILED Export: file \"%s\" : %s\n",
		   full_filename,  error_description(orig_errno)));
	q_status_message2(SM_ORDER | SM_DING, 3, 4,
			  "Error exporting to \"%s\" : %s",
			  filename, error_description(orig_errno));
    }
    else{
	if(mn_total_cur(msgmap) > 1L)
	  q_status_message4(SM_ORDER,0,3,"%s message%s %s to file \"%s\"",
			    long2string(count), plural(count),
			    over==0 ? "exported"
				    : over==1 ? "overwritten" : "appended",
			    filename);
	else
	  q_status_message3(SM_ORDER,0,3,"Message %s %s to file \"%s\"",
			    long2string(mn_get_cur(msgmap)),
			    over==0 ? "exported"
				    : over==1 ? "overwritten" : "appended",
			    filename);
    }

  fini:
    if(agg)
      restore_selected(msgmap);
}



/*----------------------------------------------------------------------
  parse the config'd upload/download command

  Args: cmd -- buffer to return command fit for shellin'
	prefix --
	cfg_str --
	fname -- file name to build into the command

  Returns: pointer to cmd_str buffer or NULL on real bad error

  NOTE: One SIDE EFFECT is that any defined "prefix" string in the
	cfg_str is written to standard out right before a successful
	return of this function.  The call immediately following this
	function darn well better be the shell exec...
 ----*/
char *
build_updown_cmd(cmd, prefix, cfg_str, fname)
    char *cmd;
    char *prefix;
    char *cfg_str;
    char *fname;
{
    char *p;
    int   fname_found = 0;

    if(prefix && *prefix){
	/* loop thru replacing all occurances of _FILE_ */
	for(p = strcpy(cmd, prefix); (p = strstr(p, "_FILE_")); )
	  rplstr(p, 6, fname);

	fputs(cmd, stdout);
    }

    /* loop thru replacing all occurances of _FILE_ */
    for(p = strcpy(cmd, cfg_str); (p = strstr(p, "_FILE_")); ){
	rplstr(p, 6, fname);
	fname_found = 1;
    }

    if(!fname_found)
      sprintf(cmd + strlen(cmd), " %s", fname);

    dprint(4, (debugfile, "\n - build_updown_cmd = \"%s\" -\n", cmd));
    return(cmd);
}






/*----------------------------------------------------------------------
  Write a berzerk format message delimiter using the given putc function

    Args: e -- envelope of message to write
	  pc -- function to use 

    Returns: TRUE if we could write it, FALSE if there was a problem

    NOTE: follows delimiter with OS-dependent newline
 ----*/
int
bezerk_delimiter(env, pc, leading_newline)
    ENVELOPE *env;
    gf_io_t   pc;
    int	      leading_newline;
{
    time_t  now = time(0);
    char   *p = ctime(&now);
    
    /* write "[\n]From mailbox[@host] " */
    if(!((leading_newline ? gf_puts(NEWLINE, pc) : 1)
	 && gf_puts("From ", pc)
	 && gf_puts((env && env->from) ? env->from->mailbox
				       : "the-concourse-on-high", pc)
	 && gf_puts((env && env->from && env->from->host) ? "@" : "", pc)
	 && gf_puts((env && env->from && env->from->host) ? env->from->host
							  : "", pc)
	 && (*pc)(' ')))
      return(0);

    while(p && *p && *p != '\n')	/* write date */
      if(!(*pc)(*p++))
	return(0);

    if(!gf_puts(NEWLINE, pc))		/* write terminating newline */
      return(0);

    return(1);
}



/*----------------------------------------------------------------------
      Execute command to jump to a given message number

    Args: qline -- Line to ask question on

  Result: returns -1 or the message number to jump to
          the mangled_footer flag is set
 ----*/
void
jump_to(msgmap, qline, first_num)
     MSGNO_S *msgmap;
     int      qline, first_num;
{
    char     jump_num_string[80], *j, prompt[70];
    HelpType help;
    int      rc;
    long     jump_num;

    dprint(4, (debugfile, "\n - jump_to -\n"));

    if(!any_messages(msgmap, NULL, "to Jump to"))
      return;

    if(first_num){
	jump_num_string[0] = first_num;
	jump_num_string[1] = '\0';
    }
    else
      jump_num_string[0] = '\0';

    if(mn_total_cur(msgmap) > 1L){
	sprintf(prompt, "Unselect %s msgs in favor of number to be entered", 
		comatose(mn_total_cur(msgmap)));
	if((rc = want_to(prompt, 'n', 0, NO_HELP, 0, 0)) == 'n')
	    return;
    }

    strcpy(prompt, "Message number to jump to : ");

    help = NO_HELP;
    while (1) {
        rc = optionally_enter(jump_num_string, qline, 0,
                              sizeof(jump_num_string) - 1, 1, 0, prompt,
                              NULL, help, 0);
        if(rc == 3) {
            help = help == NO_HELP ? h_oe_jump : NO_HELP;
            continue;
        }

        if(rc == 0 && *jump_num_string != '\0') {
	    removing_trailing_white_space(jump_num_string);
	    removing_leading_white_space(jump_num_string);
            for(j=jump_num_string; isdigit((unsigned char)*j) || *j=='-'; j++);
	    if(*j != '\0') {
	        q_status_message(SM_ORDER | SM_DING, 2, 2,
                           "Invalid number entered. Use only digits 0-9");
            } else {
                jump_num = atol(jump_num_string);
                if(jump_num < 1L) {
	            q_status_message1(SM_ORDER | SM_DING, 2, 2,
			      "Message number (%s) must be greater than 0",
				      long2string(jump_num));
                } else if(jump_num > mn_get_total(msgmap)) {
                    q_status_message1(SM_ORDER | SM_DING, 2, 2,
	      "Message number must be no more than %s, the number of messages",
				      long2string(mn_get_total(msgmap)));
                } else if(get_lflag(ps_global->mail_stream, msgmap,
				    jump_num, MN_HIDE)){
	            q_status_message1(SM_ORDER | SM_DING, 2, 2,
			  "Message number (%s) is not in \"Zoomed Index\"",
				      long2string(jump_num));
		} else {
		    if(mn_total_cur(msgmap) > 1L){
			mn_reset_cur(msgmap, jump_num);
		    }
		    else{
			mn_set_cur(msgmap, jump_num);
		    }

		    break;
                }
            }

            jump_num_string[0] = '\0';
            continue;
	}

        if(rc != 4)
          break;
    }
}



/*----------------------------------------------------------------------
     Prompt for folder name to open, expand the name and return it

   Args: qline      -- Screen line to prompt on
         allow_list -- if 1, allow ^T to bring up collection lister

 Result: returns the folder name or NULL
         pine structure mangled_footer flag is set
         may call the collection lister in which case mangled screen will be set

 This prompts the user for the folder to open, possibly calling up
the collection lister if the user types ^T.
----------------------------------------------------------------------*/
char *
broach_folder(qline, allow_list, context)
    int qline, allow_list;
    CONTEXT_S **context;
{
    HelpType	help;
    static char newfolder[MAXFOLDER+1];
    char        expanded[MAXPATH+1],
                prompt[MAXFOLDER+80],
               *last_folder;
    CONTEXT_S  *tc, *tc2;
    ESCKEY_S    ekey[7];
    int         rc, inbox;

    /*
     * the idea is to provide a clue for the context the file name
     * will be saved in (if a non-imap names is typed), and to
     * only show the previous if it was also in the same context
     */
    help	   = NO_HELP;
    *expanded	   = '\0';
    *newfolder	   = '\0';
    last_folder	   = NULL;
    ps_global->mangled_footer = 1;

    /*
     * There are three possibilities for the prompt's offered default.
     *  1) always the last folder visited
     *  2) if non-inbox current, inbox else last folder visited
     *  3) if non-inbox current, inbox else last folder visited in
     *     the first collection
     */
    if(ps_global->goto_default_rule == GOTO_LAST_FLDR){
	tc = (context && *context) ? *context : ps_global->context_current;
	inbox = 1;		/* fill in last_folder below */
    }
    else{
	inbox = strucmp(ps_global->cur_folder,ps_global->inbox_name) == 0;
	if(!inbox)
	  tc = ps_global->context_list;		/* inbox's context */
	else if(ps_global->goto_default_rule == GOTO_INBOX_FIRST_CLCTN)
	  tc = (ps_global->context_list->use & CNTXT_INCMNG)
		 ? ps_global->context_list->next : ps_global->context_list;
	else
	  tc = (context && *context) ? *context : ps_global->context_current;
    }

    /* set up extra command option keys */
    rc = 0;
    ekey[rc].ch	     = (allow_list) ? ctrl('T') : 0 ;
    ekey[rc].rval    = (allow_list) ? 2 : 0;
    ekey[rc].name    = (allow_list) ? "^T" : "";
    ekey[rc++].label = (allow_list) ? "ToFldrs" : "";

    if(ps_global->context_list->next){
	ekey[rc].ch      = ctrl('P');
	ekey[rc].rval    = 10;
	ekey[rc].name    = "^P";
	ekey[rc++].label = "Prev Collection";

	ekey[rc].ch      = ctrl('N');
	ekey[rc].rval    = 11;
	ekey[rc].name    = "^N";
	ekey[rc++].label = "Next Collection";
    }

    if(F_ON(F_ENABLE_TAB_COMPLETE,ps_global)){
	ekey[rc].ch      = TAB;
	ekey[rc].rval    = 12;
	ekey[rc].name    = "TAB";
	ekey[rc++].label = "Complete";
    }

    if(ps_global->context_list->next){
	ekey[rc].ch      = KEY_UP;
	ekey[rc].rval    = 10;
	ekey[rc].name    = "";
	ekey[rc++].label = "";

	ekey[rc].ch      = KEY_DOWN;
	ekey[rc].rval    = 11;
	ekey[rc].name    = "";
	ekey[rc++].label = "";
    }

    ekey[rc].ch = -1;

    while(1) {
	/*
	 * Figure out next default value for this context.  The idea
	 * is that in each context the last folder opened is cached.
	 * It's up to pick it out and display it.  This is fine
	 * and dandy if we've currently got the inbox open, BUT
	 * if not, make the inbox the default the first time thru.
	 */
	if(!inbox){
	    last_folder = ps_global->inbox_name;
	    inbox = 1;		/* pretend we're in inbox from here on out */
	}
	else
	  last_folder = (tc->last_folder[0]) ? tc->last_folder : NULL;

	if(last_folder)
	  sprintf(expanded, " [%s]", last_folder);
	else
	  *expanded = '\0';

	/* only show collection number if more than one available */
	if(ps_global->context_list->next){
	    sprintf(prompt, "GOTO %s in <%.20s> %s%s: ",
		    (tc->type&FTYPE_BBOARD) ? "news group" : "folder",
		    tc->label[0], expanded, *expanded ? " " : "");
	}
	else
	  sprintf(prompt, "GOTO folder %s: ", expanded, *expanded ? " " : "");

        rc = optionally_enter(newfolder, qline, 0, MAXFOLDER, 1 ,0, prompt,
                              ekey, help, 0);

	if(rc == -1){
	    q_status_message(SM_ORDER | SM_DING, 3, 3,
			     "Error reading folder name");
	    return(NULL);
	}
	else if(rc == 1){
	    cmd_cancelled("Open Folder");
	    return(NULL);
	}
	else if(rc == 2){
	    void (*redraw)() = ps_global->redrawer;

	    push_titlebar_state();
	    rc = folder_lister(ps_global, OpenFolder, tc, &tc, newfolder,
			       NULL, ps_global->context_list, NULL);
            ClearScreen();
	    pop_titlebar_state();
            redraw_titlebar();
            if(ps_global->redrawer = redraw) /* reset old value, and test */
              (*ps_global->redrawer)();

	    if(rc == 1 && F_ON(F_SELECT_WO_CONFIRM, ps_global))
	      break;
	}
	else if(rc == 3){
            help = help == NO_HELP ? h_oe_broach : NO_HELP;
	}
	else if(rc == 10){			/* Previous collection */
	    tc2 = ps_global->context_list;
	    while(tc2->next && tc2->next != tc)
	      tc2 = tc2->next;

	    tc = tc2;
	}
	else if(rc == 11){			/* Next collection */
	    tc = (tc->next) ? tc->next : ps_global->context_list;
	}
	else if(rc == 12){			/* file name completion! */
	    if(!folder_complete(tc, newfolder))
	      Writechar(BELL, 0);
	}
	else if(rc != 4)
          break;
    }

    removing_trailing_white_space(newfolder);
    removing_leading_white_space(newfolder);

    if(*newfolder == '\0'  && last_folder == NULL) {
	cmd_cancelled("Open folder");
        return(NULL);
    }

    if(*newfolder == '\0')
      strcpy(newfolder, last_folder);

    dprint(2, (debugfile, "broach folder, name entered \"%s\"\n",newfolder));

    /*-- Just check that we can expand this. It gets done for real later --*/
    strcpy(expanded, newfolder);
    if (! expand_foldername(expanded)) {
        dprint(1, (debugfile,
                    "Error: Failed on expansion of filename %s (save)\n", 
    	  expanded));
        return(NULL);
    }

    *context = tc;
    return(newfolder);
}




/*----------------------------------------------------------------------
    Actually attempt to open given folder 

  Args: newfolder -- The folder name to open

 Result:  1 if the folder was successfully opened
          0 if the folder open failed and went back to old folder
         -1 if open failed and no folder is left open
      
  Attempt to open the folder name given. If the open of the new folder
  fails then the previously open folder will remain open, unless
  something really bad has happened. The designate inbox will always be
  kept open, and when a request to open it is made the already open
  stream will be used. Making a folder the current folder requires
  setting the following elements of struct pine: mail_stream, cur_folder,
  current_msgno, max_msgno. Attempting to reopen the current folder is a 
  no-op.

  The first time the inbox folder is opened, usually as Pine starts up,
  it will be actually opened.
  ----*/

do_broach_folder(newfolder, new_context) 
     char      *newfolder;
     CONTEXT_S *new_context;
{
    MAILSTREAM *m;
    int         open_inbox, rv, old_tros, we_cancel = 0;
    char        expanded_file[MAXPATH+1], *old_folder, *old_path, *p;
    long        openmode;
    char        status_msg[81];
    SortOrder	old_sort;

#if	defined(DOS) && !defined(WIN32)
    openmode = OP_SHORTCACHE;
#else
    openmode = 0L;
#endif
#ifdef	DEBUG
    if(debug > 8)
      openmode |= OP_DEBUG;
#endif
    dprint(1, (debugfile, "About to open folder \"%s\"    inbox: \"%s\"\n",
	       newfolder, ps_global->inbox_name));

    /*----- Little to do to if reopening same folder -----*/
    if(new_context == ps_global->context_current && ps_global->mail_stream
       && strcmp(newfolder, ps_global->cur_folder) == 0){
	if(ps_global->dead_stream){
	    /* though, if it's not healthy, we reset things and fall thru
	     * to actually reopen it...
	     */
	    q_status_message1(SM_ORDER, 0, 4, 
			      "Attempting to reopen closed folder \"%s\"",
			      newfolder);
	    mail_close(ps_global->mail_stream);
	    if(ps_global->mail_stream == ps_global->inbox_stream)
	      ps_global->inbox_stream = NULL;

	    ps_global->mail_stream = NULL;
	    ps_global->expunge_count       = 0;
	    ps_global->new_mail_count      = 0;
	    ps_global->noticed_dead_stream = 0;
	    ps_global->dead_stream         = 0;
	    ps_global->last_msgno_flagged  = 0L;
	    ps_global->mangled_header	   = 1;
	    mn_give(&ps_global->msgmap);
	    clear_index_cache();
	    reset_check_point();
	}
	else
	  return(1);			/* successful open of same folder! */
    }

    /*--- Set flag that we're opening the inbox, a special case ---*/
    /*
     * We want to know if inbox is being opened either by name OR
     * fully qualified path...
     *
     * So, IF we're asked to open inbox AND it's already open AND
     * the only stream AND it's healthy, just return ELSE fall thru
     * and close mail_stream returning with inbox_stream as new stream...
     */
    if(open_inbox = (strucmp(newfolder, ps_global->inbox_name) == 0
		     || strcmp(newfolder, ps_global->VAR_INBOX_PATH) == 0)){
	new_context = ps_global->context_list; /* restore first context */
	if(ps_global->inbox_stream 
	   && (ps_global->inbox_stream == ps_global->mail_stream))
	  return(1);
    }

    /*
     * If ambiguous foldername (not fully qualified), make sure it's
     * not a nickname for a folder in the given context...
     */
    strcpy(expanded_file, newfolder); 	/* might get reset below */
    if(!open_inbox && new_context && context_isambig(newfolder)){
	if (p = folder_is_nick(newfolder, new_context->folders)){
	    strcpy(expanded_file, p);
	    dprint(2, (debugfile, "broach_folder: nickname for %s is %s\n",
		       expanded_file, newfolder));
	}
	else if ((new_context->use & CNTXT_INCMNG)
		 && (folder_index(newfolder, new_context->folders) < 0)){
	    q_status_message1(SM_ORDER, 3, 4,
			    "Can't find Incoming Folder %s.", newfolder);
	    return(0);
	}
    }

    /*--- Opening inbox, inbox has been already opened, the easy case ---*/
    if(open_inbox && ps_global->inbox_stream != NULL ) {
        expunge_and_close(ps_global->mail_stream, ps_global->context_current,
			  ps_global->cur_folder, NULL);

	ps_global->mail_stream              = ps_global->inbox_stream;
        ps_global->new_mail_count           = 0L;
        ps_global->expunge_count            = 0L;
        ps_global->last_msgno_flagged       = 0L;
        ps_global->mail_box_changed         = 0;
        ps_global->noticed_dead_stream      = 0;
        ps_global->noticed_dead_inbox       = 0;
        ps_global->dead_stream              = 0;
        ps_global->dead_inbox               = 0;
	mn_give(&ps_global->msgmap);
	ps_global->msgmap		    = ps_global->inbox_msgmap;
	ps_global->inbox_msgmap		    = NULL;

	dprint(7, (debugfile, "%ld %ld %x\n",
		   mn_get_cur(ps_global->msgmap),
                   mn_get_total(ps_global->msgmap),
		   ps_global->mail_stream));
	/*
	 * remember last context and folder
	 */
	if(context_isambig(ps_global->cur_folder)){
	    ps_global->context_last = ps_global->context_current;
	    strcpy(ps_global->context_current->last_folder,
		 ps_global->cur_folder);
	}

	strcpy(ps_global->cur_folder, ps_global->inbox_name);
	ps_global->context_current = ps_global->context_list;
        clear_index_cache();
        /* MUST sort before restoring msgno! */
	sort_current_folder(1, mn_get_sort(ps_global->msgmap),
			    mn_get_revsort(ps_global->msgmap));
        q_status_message2(SM_ORDER, 0, 3,
			  "Opened folder \"INBOX\" with %s message%s",
                          long2string(mn_get_total(ps_global->msgmap)),
                          mn_get_total(ps_global->msgmap) != 1 ? "s" : "" );
	return(1);
    }

    if(!new_context && ! expand_foldername(expanded_file))
      return(0);

    old_folder = NULL;
    old_path   = NULL;
    old_sort   = SortArrival;			/* old sort */
    old_tros   = 0;				/* old reverse sort ? */
    /*---- now close the old one we had open if there was one ----*/
    if(ps_global->mail_stream != NULL){
        old_folder   = cpystr(ps_global->cur_folder);
        old_path     = cpystr(ps_global->mail_stream->mailbox);
	old_sort     = mn_get_sort(ps_global->msgmap);
	old_tros     = mn_get_revsort(ps_global->msgmap);
	if(strcmp(ps_global->cur_folder, ps_global->inbox_name) == 0){
	    /*-- don't close the inbox stream, save a bit of state --*/
	    if(ps_global->inbox_msgmap)
	      mn_give(&ps_global->inbox_msgmap);

	    ps_global->inbox_msgmap = ps_global->msgmap;
	    ps_global->msgmap       = NULL;

	    dprint(2, (debugfile,
		       "Close - saved inbox state: max %ld\n",
		       mn_get_total(ps_global->inbox_msgmap)));
	}
	else
          expunge_and_close(ps_global->mail_stream, ps_global->context_current,
			    ps_global->cur_folder, NULL);
    }

    strcat(strncat(strcpy(status_msg, "Opening \""),
	    pretty_fn(newfolder), 70), "\"");
    we_cancel = busy_alarm(1, status_msg, NULL, 1);

    /* 
     * if requested, make access to folder readonly (only once)
     */
    if (ps_global->open_readonly_on_startup) {
	openmode |= OP_READONLY ;
	ps_global->open_readonly_on_startup = 0 ;
    }

    /*
     * The name "inbox" is special, so treat it so 
     * (used to by handled by expand_folder)...
     */
    if(ps_global->nr_mode)
      ps_global->noshow_warn = 1;

    m = context_open((new_context && !open_inbox) ? new_context->context:NULL,
		     NULL, 
		     (open_inbox) ? ps_global->VAR_INBOX_PATH : expanded_file,
		     openmode);

    if(ps_global->nr_mode)
      ps_global->noshow_warn = 0;

    dprint(8, (debugfile, "Opened folder %p \"%s\" (context: \"%s\")\n",
               m, (m) ? m->mailbox : "nil",
	       (new_context) ? new_context->context : "nil"));


    /* Can get m != NULL if correct passwd for remote, but wrong name */
    if(m == NULL || ((p = strindex(m->mailbox, '<')) != NULL &&
                      strcmp(p + 1, "no_mailbox>") == 0)) {
	/*-- non-existent local mailbox, or wrong passwd for remote mailbox--*/
        /* fall back to currently open mailbox */
	if(we_cancel)
	  cancel_busy_alarm(-1);

        rv = 0;
        dprint(8, (debugfile, "Old folder: \"%s\"\n",
               old_folder == NULL ? "" : old_folder));
        if(old_folder != NULL) {
            if(strcmp(old_folder, ps_global->inbox_name) == 0){
                ps_global->mail_stream = ps_global->inbox_stream;
		if(ps_global->msgmap)
		  mn_give(&ps_global->msgmap);

		ps_global->msgmap       = ps_global->inbox_msgmap;
		ps_global->inbox_msgmap = NULL;

                dprint(8, (debugfile, "Reactivate inbox %ld %ld %p\n",
                           mn_get_cur(ps_global->msgmap),
                           mn_get_total(ps_global->msgmap),
                           ps_global->mail_stream));
                strcpy(ps_global->cur_folder, ps_global->inbox_name);
            } else {
                ps_global->mail_stream = mail_open(NULL, old_path, openmode);
                /* mm_log will take care of error message here */
                if(ps_global->mail_stream == NULL) {
                    rv = -1;
                } else {
		    mn_init(&(ps_global->msgmap),
			    ps_global->mail_stream->nmsgs);
		    mn_set_sort(ps_global->msgmap, old_sort);
		    mn_set_revsort(ps_global->msgmap, old_tros);
                    ps_global->expunge_count       = 0;
                    ps_global->new_mail_count      = 0;
                    ps_global->noticed_dead_stream = 0;
                    ps_global->dead_stream         = 0;
		    ps_global->last_msgno_flagged  = 0L;
		    ps_global->mangled_header	   = 1;

		    clear_index_cache();
                    reset_check_point();
		    if(mn_get_total(ps_global->msgmap) > 0)
		      mn_set_cur(ps_global->msgmap, 1L);

		    if(mn_get_sort(ps_global->msgmap) != SortArrival
		       || mn_get_revsort(ps_global->msgmap))
		      sort_current_folder(1, mn_get_sort(ps_global->msgmap),
					  mn_get_revsort(ps_global->msgmap));

                    q_status_message1(SM_ORDER, 0, 3, "Folder \"%s\" reopened",
                                      old_folder);
                }
            }

	    if(rv == 0)
	      mn_set_cur(ps_global->msgmap,
			 min(mn_get_cur(ps_global->msgmap), 
			     mn_get_total(ps_global->msgmap)));

            fs_give((void **)&old_folder);
            fs_give((void **)&old_path);
        } else {
            rv = -1;
        }
        if(rv == -1) {
            q_status_message(SM_ORDER | SM_DING, 0, 4, "No folder opened");
	    mn_set_total(ps_global->msgmap, 0L);
	    mn_set_cur(ps_global->msgmap, -1L);
            strcpy(ps_global->cur_folder, "");
        }
        return(rv);
    } else {
        if(old_folder != NULL) {
            fs_give((void **)&old_folder);
            fs_give((void **)&old_path);
        }
    }

    /*----- success in opening the new folder ----*/
    dprint(2, (debugfile, "Opened folder \"%s\" with %ld messages\n",
	       m->mailbox, m->nmsgs));


    /*--- A Little house keeping ---*/
    ps_global->mail_stream          = m;
    ps_global->expunge_count        = 0L;
    ps_global->new_mail_count       = 0L;
    ps_global->last_msgno_flagged   = 0L;
    ps_global->noticed_dead_stream  = 0;
    ps_global->noticed_dead_inbox   = 0;
    ps_global->dead_stream          = 0;
    ps_global->dead_inbox           = 0;
    mn_init(&(ps_global->msgmap), m->nmsgs);

    /*
     * remember old folder and context...
     */
    if(context_isambig(ps_global->cur_folder)
       || strucmp(ps_global->cur_folder, ps_global->inbox_name) == 0)
      strcpy(ps_global->context_current->last_folder,ps_global->cur_folder);

    strcpy(ps_global->cur_folder, (open_inbox) ? ps_global->inbox_name
					       : newfolder);
    if(new_context){
	ps_global->context_last    = ps_global->context_current;
	ps_global->context_current = new_context;
    }

    reset_check_point();
    clear_index_cache();
    ps_global->mail_box_changed = 0;

    /*
     * Start news reading with messages the user's marked deleted
     * hidden from view...
     */
    if(IS_NEWS(ps_global->mail_stream) && ps_global->mail_stream->rdonly)
      msgno_exclude(ps_global->mail_stream, ps_global->msgmap);

    if(we_cancel)
      cancel_busy_alarm(0);

    /* UWIN doesn't want to see this message */
    if(!ps_global->nr_mode)
      q_status_message7(SM_ORDER, 0, 4, "%s \"%s\" opened with %s message%s%s",
			IS_NEWS(ps_global->mail_stream)
			  ? "News group" : "Folder",
			pretty_fn(newfolder),
			comatose(mn_get_total(ps_global->msgmap)),
			plural(mn_get_total(ps_global->msgmap)),
			READONLY_FOLDER ?" READONLY" : "",
			NULL, NULL);

    sort_current_folder(0, SortArrival, 0);

    if(mn_get_total(ps_global->msgmap) > 0L) {
	if(ps_global->start_entry > 0) {
	    mn_set_cur(ps_global->msgmap,
		       min(ps_global->start_entry,
			   mn_get_total(ps_global->msgmap)));
	    ps_global->start_entry = 0;
        }
	/* if faking new, goto first recent, unseen */
	else if(IS_NEWS(ps_global->mail_stream)
		&& F_ON(F_FAKE_NEW_IN_NEWS, ps_global)){
	    mn_set_cur(ps_global->msgmap,
		       first_sorted_flagged(F_RECENT|F_UNDEL,m));
	}
	/* if it is an incoming mailbox, goto first unseen */
	else if(open_inbox || (ps_global->context_current->use&CNTXT_INCMNG)
		  || IS_NEWS(ps_global->mail_stream)){
	    mn_set_cur(ps_global->msgmap,
		       first_sorted_flagged(F_UNSEEN|F_UNDEL,m));
	}
        else{
	    mn_set_cur(ps_global->msgmap,
		       mn_get_revsort(ps_global->msgmap)
		         ? 1L
			 : mn_get_total(ps_global->msgmap));
	}
    }
    else{
	mn_set_cur(ps_global->msgmap, -1L);
    }

    /*--- If we just opened the inbox remember it's special stream ---*/
    if(open_inbox && ps_global->inbox_stream == NULL)
      ps_global->inbox_stream = ps_global->mail_stream;
	
    return(1);
}



/*----------------------------------------------------------------------
    Open the requested folder in the requested context

    Args: state -- usual pine state struct
	  newfolder -- folder to open
	  new_context -- folder context might live in

   Result: New folder open or not (if error), and we're set to
	   enter the index screen.
 ----*/
void
visit_folder(state, newfolder, new_context)
    struct pine *state;
    char	*newfolder;
    CONTEXT_S	*new_context;
{
    dprint(9, (debugfile, "do_broach_folder (%s, %s)\n",
	       newfolder, new_context->context));
    if(do_broach_folder(newfolder, new_context) > 0
       || state->mail_stream != state->inbox_stream)
      state->next_screen = mail_index_screen;
}



/*----------------------------------------------------------------------
      Expunge (if confirmed) and close a mail stream

    Args: stream   -- The MAILSTREAM * to close
	  context  -- The contest to interpret the following name in
          folder   -- The name the folder is know by 
        final_msg  -- If non-null, this should be set to point to a
		      message to print out in the caller, it is allocated
		      here and freed by the caller.

   Result:  Mail box is expunged and closed. A message is displayed to
             say what happened
 ----*/
void
expunge_and_close(stream, context, folder, final_msg)
    MAILSTREAM *stream;
    CONTEXT_S  *context;
    char       *folder;
    char      **final_msg;
{
    long  delete_count, max_folder, seen_not_del;
    char  prompt_b[MAX_SCREEN_COLS+1], short_folder_name[MAXFOLDER*2+1],
          temp[MAXFOLDER*2+1], buff1[MAX_SCREEN_COLS+1], *moved_msg = NULL,
	  buff2[MAX_SCREEN_COLS+1];
    struct variable *vars = ps_global->vars;
    int ret;
    char ing[4];

    if(final_msg)
      strcpy(ing, "ed");
    else
      strcpy(ing, "ing");

    buff1[0] = '\0';
    buff2[0] = '\0';
    if(stream != NULL){
        dprint(2, (debugfile, "expunge and close mail stream \"%s\"\n",
                   stream->mailbox));
        if(!stream->rdonly){

            q_status_message1(SM_INFO, 0, 1, "Closing \"%s\"...", folder);
	    flush_status_messages(1);

	    /* Save read messages? */
	    if(VAR_READ_MESSAGE_FOLDER && VAR_READ_MESSAGE_FOLDER[0]
	       && stream == ps_global->inbox_stream
	       && (seen_not_del = count_flagged(stream, "SEEN UNDELETED"))){

		if(F_ON(F_AUTO_READ_MSGS,ps_global)
		   || read_msg_prompt(seen_not_del, VAR_READ_MESSAGE_FOLDER))
		  /* move inbox's read messages */
		  moved_msg = move_read_msgs(stream, VAR_READ_MESSAGE_FOLDER,
					     buff1, -1L);
	    }
	    else if(VAR_ARCHIVED_FOLDERS)
	      moved_msg = move_read_incoming(stream, context, folder,
					     VAR_ARCHIVED_FOLDERS,
					     buff1);

            delete_count = count_flagged(stream, "DELETED");
	    ret = 'n';
	    if(delete_count){
		max_folder = ps_global->ttyo->screen_cols - 50;
		strcpy(temp, pretty_fn(folder));
		if(strlen(temp) > max_folder){
		    strcpy(short_folder_name, "...");
		    strcat(short_folder_name,temp + strlen(temp) -max_folder);
		}
		else
		  strcpy(short_folder_name, temp);
                
		if(F_OFF(F_AUTO_EXPUNGE,ps_global)){
		    sprintf(prompt_b,
			    "Expunge the %ld deleted message%s from \"%s\"",
			    delete_count,
			    delete_count == 1 ? "" : "s",
			    short_folder_name);
		    ret = want_to(prompt_b, 'y', 0, NO_HELP, 0, 0);
		}
		else
		  ret = 'y';

		/* get this message back in queue */
		if(moved_msg)
		  q_status_message(SM_ORDER,
		      F_ON(F_AUTO_READ_MSGS,ps_global) ? 0 : 3, 5, moved_msg);

		if(ret == 'y'){
		    sprintf(buff2,
		      "Clos%s \"%.30s\". %s %s message%s and delet%s %s.",
			ing,
	 		pretty_fn(folder),
			final_msg ? "Kept" : "Keeping",
			comatose((stream->nmsgs - delete_count)),
			plural(stream->nmsgs - delete_count),
			ing,
			long2string(delete_count));
		    if(final_msg)
		      *final_msg = cpystr(buff2);
		    else
		      q_status_message(SM_ORDER,
				       F_ON(F_AUTO_EXPUNGE,ps_global) ? 0 :3,
				       5, buff2);
		      
		    flush_status_messages(1);
		    ps_global->mm_log_error = 0;
		    ps_global->expunge_in_progress = 1;
		    mail_expunge(stream);
		    ps_global->expunge_in_progress = 0;
		    if(ps_global->mm_log_error && final_msg && *final_msg){
			fs_give((void **)final_msg);
			*final_msg = NULL;
		    }
		}
	    }

	    if(ret != 'y'){
		if(stream->nmsgs){
		    sprintf(buff2,
		        "Clos%s folder \"%s\". %s%s%s message%s.",
			ing,
			pretty_fn(folder), 
			final_msg ? "Kept" : "Keeping",
			(stream->nmsgs == 1L) ? " single" : " all ",
			(stream->nmsgs > 1L)
			  ? comatose(stream->nmsgs) : "",
			plural(stream->nmsgs));
		}
		else{
		    sprintf(buff2, "Clos%s empty folder \"%s\"",
			ing, pretty_fn(folder));
		}

		if(final_msg)
		  *final_msg = cpystr(buff2);
		else
		  q_status_message(SM_ORDER, 0, 3, buff2);
	    }
        }
	else{
            if(IS_NEWS(stream)){
		/* first, look to archive read messages */
		if(moved_msg = move_read_incoming(stream, context, folder,
						  VAR_ARCHIVED_FOLDERS,
						  buff1))
		  q_status_message(SM_ORDER,
		      F_ON(F_AUTO_READ_MSGS,ps_global) ? 0 : 3, 5, moved_msg);

		sprintf(buff2, "Clos%s news group \"%s\"",
			ing, pretty_fn(folder));
	    }
            else
	      sprintf(buff2,
			"Clos%s read-only folder \"%s\". No changes to save",
			ing, pretty_fn(folder));

	    if(final_msg)
	      *final_msg = cpystr(buff2);
	    else
	      q_status_message(SM_ORDER, 0, 2, buff2);
        }

	/*
	 * Make darn sure any mm_log fallout caused above get's seen...
	 */
	flush_status_messages(1);
    }

    mail_close(stream);
}



/*----------------------------------------------------------------------
  Move all read messages from srcfldr to dstfldr
 
  Args: stream -- stream to usr
	dstfldr -- folder to receive moved messages
	buf -- place to write success message

  Returns: success message or NULL for failure
  ----*/
char *
move_read_msgs(stream, dstfldr, buf, searched)
    MAILSTREAM *stream;
    char       *dstfldr, *buf;
    long	searched;
{
    long	  i;
    int           we_cancel = 0;
    MSGNO_S	 *msgmap = NULL;
    CONTEXT_S	 *save_context;
    char	  *bufp = NULL;

    save_context = default_save_context(ps_global->context_list);
    if(!save_context)
      save_context = ps_global->context_list;

    /*
     * Use the "sequence" bit to select the set of messages
     * we want to save.  If searched is non-neg, the message
     * cache already has the necessary "sequence" bits set.
     */
    if(searched < 0L)
      searched = count_flagged(stream, "SEEN UNDELETED");

    if(searched){
	mn_init(&msgmap, stream->nmsgs);
	for(i = 1L; i <= mn_get_total(msgmap); i++)
	  set_lflag(stream, msgmap, i, MN_SLCT, 0);

	/*
	 * re-init msgmap to fix the MN_SLCT count, "flagged_tmp", in
	 * case there were any flagged such before we got here.
	 *
	 * BUG: this means the count of MN_SLCT'd msgs in the
	 * folder's real msgmap is instantly bogus.  Until Cancel
	 * after "Really quit?" is allowed, this isn't a problem since
	 * that mapping table is either gone or about to get nuked...
	 */
	mn_init(&msgmap, stream->nmsgs);

	/* select search results */
	for(i = 1L; i <= mn_get_total(msgmap); i++)
	  if(mail_elt(stream, mn_m2raw(msgmap, i))->searched)
	    set_lflag(stream, msgmap, i, MN_SLCT, 1);

	pseudo_selected(msgmap);
	sprintf(buf, "Moving %s read message%s to \"%.45s\"",
		comatose(searched), plural(searched), dstfldr);
	we_cancel = busy_alarm(1, buf, NULL, 1);
	if(save(ps_global, save_context, dstfldr, msgmap, 1) != searched)
	  q_status_message1(SM_ORDER | SM_DING, 4, 6,
			    "Error saving to %.35s.  Not all messages moved.",
			    dstfldr);
	else
	  strncpy(bufp = buf + 1, "Moved", 5); /* change Moving to Moved */

	mn_give(&msgmap);
	if(we_cancel)
	  cancel_busy_alarm(bufp ? 0 : -1);
    }

    return(bufp);
}



/*----------------------------------------------------------------------
  Move read messages from folder if listed in archive
 
  Args: 

  ----*/
int
read_msg_prompt(n, f)
    long  n;
    char *f;
{
    char buf[MAX_SCREEN_COLS+1];

    sprintf(buf, "Save the %ld read message%s in \"%s\"", n, plural(n), f);
    return(want_to(buf, 'y', 0, NO_HELP, 0, 0) == 'y');
}



/*----------------------------------------------------------------------
  Move read messages from folder if listed in archive
 
  Args: 

  ----*/
char *
move_read_incoming(stream, context, folder, archive, buf)
    MAILSTREAM *stream;
    CONTEXT_S  *context;
    char       *folder;
    char      **archive;
    char       *buf;
{
    char *s, *d, *f = folder;
    long  seen_undel;

    buf[0] = '\0';

    if(archive && stream != ps_global->inbox_stream
       && (context && CNTXT_INCMNG)
       && ((context_isambig(folder)
	    && folder_is_nick(folder, context->folders))
	   || folder_index(folder, context->folders) > 0)
       && (seen_undel = count_flagged(stream, "SEEN UNDELETED"))){

	for(; f && *archive; archive++){
	    get_pair(*archive, &s, &d, 1);
	    if(s && d && !strcmp(s, folder)){
		if(F_ON(F_AUTO_READ_MSGS,ps_global)
		   || read_msg_prompt(seen_undel, d))
		  buf = move_read_msgs(stream, d, buf, seen_undel);

		f = NULL;		/* bust out after cleaning up */
	    }

	    fs_give((void **)&s);
	    fs_give((void **)&d);
	}
    }

    return((buf && *buf) ? buf : NULL);
}



/*----------------------------------------------------------------------
      Search the message headers as display in index
 
  Args: command_line -- The screen line to prompt on
        msg          -- The current message number to start searching at
        max_msg      -- The largest message number in the current folder

  The headers are searched exactly as they are displayed on the screen. The
search will wrap around to the beginning if not string is not found right 
away.
  ----*/
void
search_headers(state, stream, command_line, msgmap)
    struct pine *state;
    MAILSTREAM  *stream;
    int          command_line;
    MSGNO_S     *msgmap;
{
    int         rc, select_all = 0;
    long        i, sorted_msg, selected = 0L;
    char        prompt[MAX_SEARCH+50], new_string[MAX_SEARCH+1];
    HelpType	help;
    static char search_string[MAX_SEARCH+1] = { '\0' };
    static ESCKEY_S header_search_key[] = { {0, 0, NULL, NULL },
					    {ctrl('Y'), 10, "^Y", "First Msg"},
					    {ctrl('V'), 11, "^V", "Last Msg"},
					    {-1, 0, NULL, NULL} };

    dprint(4, (debugfile, "\n - search headers - \n"));

    if(!any_messages(msgmap, NULL, "to search")){
	return;
    }
    else if(mn_total_cur(msgmap) > 1L){
	q_status_message1(SM_ORDER, 0, 2, "%s msgs selected; Can't search",
			  comatose(mn_total_cur(msgmap)));
	return;
    }
    else
      sorted_msg = mn_get_cur(msgmap);

    help = NO_HELP;
    new_string[0] = '\0';

    while(1) {
	sprintf(prompt, "Word to search for [%s] : ", search_string);

	if(F_ON(F_ENABLE_AGG_OPS, ps_global)){
	    header_search_key[0].ch    = ctrl('X');
	    header_search_key[0].rval  = 12;
	    header_search_key[0].name  = "^X";
	    header_search_key[0].label = "Select Matches";
	}
	else{
	    header_search_key[0].ch   = header_search_key[0].rval  = 0;
	    header_search_key[0].name = header_search_key[0].label = NULL;
	}
	
        rc = optionally_enter(new_string, command_line, 0, MAX_SEARCH, 1,
			      0, prompt, header_search_key, help, 0);

        if(rc == 3) {
	    help = (help != NO_HELP) ? NO_HELP :
		     F_ON(F_ENABLE_AGG_OPS, ps_global) ? h_os_index_whereis_agg
		       : h_os_index_whereis;
            continue;
        }
	else if(rc == 10){
	    q_status_message(SM_ORDER, 0, 3, "Searched to First Message.");
	    if(any_lflagged(msgmap, MN_HIDE)){
		do{
		    selected = sorted_msg;
		    mn_dec_cur(stream, msgmap);
		    sorted_msg = mn_get_cur(msgmap);
		}
		while(selected != sorted_msg);
	    }
	    else
	      sorted_msg = (mn_get_total(msgmap) > 0L) ? 1L : 0L;

	    mn_set_cur(msgmap, sorted_msg);
	    return;
	}
	else if(rc == 11){
	    q_status_message(SM_ORDER, 0, 3, "Searched to Last Message.");
	    if(any_lflagged(msgmap, MN_HIDE)){
		do{
		    selected = sorted_msg;
		    mn_inc_cur(stream, msgmap);
		    sorted_msg = mn_get_cur(msgmap);
		}
		while(selected != sorted_msg);
	    }
	    else
	      sorted_msg = mn_get_total(msgmap);

	    mn_set_cur(msgmap, sorted_msg);
	    return;
	}
	else if(rc == 12){
	    select_all = 1;
	    break;
	}

        if(rc != 4)			/* redraw */
          break; /* redraw */
    }

    if(rc == 1 || (new_string[0] == '\0' && search_string[0] == '\0')) {
	cmd_cancelled("Search");
        return;
    }

    if(new_string[0] == '\0')
      strcpy(new_string, search_string);

    strcpy(search_string, new_string);

    for(i = sorted_msg + ((select_all)?0:1); i <= mn_get_total(msgmap); i++) {
        if(!get_lflag(stream, msgmap, i, MN_HIDE) &&
	   srchstr(build_header_line(state, stream,
			   msgmap, i)->line, search_string)){
	    if(select_all){
		set_lflag(stream, msgmap, i, MN_SLCT, 1);
		selected++;
	    }
	    else
	      goto found;
	}
    }

    for(i = 1; i < sorted_msg; i++) {
        if(!get_lflag(stream, msgmap, i, MN_HIDE) &&
	   srchstr(build_header_line(state, stream,
			   msgmap, i)->line, search_string)){
	    if(select_all){
		set_lflag(stream, msgmap, i, MN_SLCT, 1);
		selected++;
	    }
	    else
	      goto found;
	}
    }

    if(!select_all){
	q_status_message(SM_ORDER, 0, 3, "Word not found");
	return;
  found:
	q_status_message1(SM_ORDER, 0, 3, "Word found%s", i > sorted_msg ? "" :
			  ". Search wrapped to beginning"); 
	mn_set_cur(msgmap, i);
    }
    else{
	q_status_message1(SM_ORDER, 0, 3, "%s messages found matching word",
			  long2string(selected));
    }
}



/*----------------------------------------------------------------------
    Print current message[s] or folder index

 Filters the original header and sends stuff to printer
  ---*/
void
cmd_print(state, msgmap, agg, in_index)
     struct pine *state;
     MSGNO_S     *msgmap;
     int	  agg, in_index;
{
    char      prompt[250];
    long      i, msgs;
    int	      next = 0, print_index = 0;
    ENVELOPE *e;
    BODY     *b;

    if(agg && !pseudo_selected(msgmap))
      return;

    msgs = mn_total_cur(msgmap);

    if(in_index && F_ON(F_PRINT_INDEX, state)){
	char m[10];
	static ESCKEY_S prt_opts[] = {
	    {'i', 'i', "I", "Index"},
	    {'m', 'm', "M", NULL},
	    {-1, 0, NULL, NULL}};

	sprintf(m, "Message%s", (msgs>1L) ? "s" : "");
	prt_opts[1].label = m;
	sprintf(prompt, "Print %sFolder Index or %s %s? ",
	    agg ? "selected " : "", agg ? "selected" : "current", m);
	switch(radio_buttons(prompt, -FOOTER_ROWS(state), prt_opts, 'm', 'x',
			     NO_HELP, RB_NORM)){
	  case 'x' :
	    cmd_cancelled("Print");
	    if(agg)
	      restore_selected(msgmap);

	    return;

	  case 'i':
	    print_index = 1;
	    break;

	  default :
	  case 'm':
	    break;
	}
    }

    if(print_index)
      sprintf(prompt, "%sFolder Index ", agg ? "Selected " : "");
    else if(msgs > 1L)
      sprintf(prompt, "%s messages ", long2string(msgs));
    else
      sprintf(prompt, "Message %s ", long2string(mn_get_cur(msgmap)));

    if(open_printer(prompt) < 0){
	if(agg)
	  restore_selected(msgmap);

	return;
    }
    
    if(print_index){
	/*
	 * Print titlebar, then all the index members...
	 */
	print_text1("%s\n\n", format_titlebar(NULL));

	for(i = 1L; i <= mn_get_total(msgmap); i++){
	    if(agg && !get_lflag(state->mail_stream, msgmap, i, MN_SLCT))
	      continue;

	    if(!print_char((mn_is_cur(msgmap, i)) ? '>' : ' ')
	       || !gf_puts(build_header_line(state, state->mail_stream,
					     msgmap, i)->line + 1, print_char)
	       || !gf_puts(NEWLINE, print_char)){
		q_status_message(SM_ORDER | SM_DING, 3, 3,
				 "Error printing folder index");
		break;
	    }
	}
    }
    else{
        for(i = mn_first_cur(msgmap); i > 0L; i = mn_next_cur(msgmap), next++){
	    if(next && F_ON(F_AGG_PRINT_FF, state))
	      if(!print_char(FORMFEED))
	        break;

	    if(!(e=mail_fetchstructure(state->mail_stream, mn_m2raw(msgmap,i),
				       &b))
	       || (F_ON(F_FROM_DELIM_IN_PRINT, ps_global)
		   && !bezerk_delimiter(e, print_char, next))
	       || !format_message(mn_m2raw(msgmap, mn_get_cur(msgmap)), e, b,
				(FM_NEW_MESS|FM_DO_PRINT), print_char)){
	        q_status_message(SM_ORDER | SM_DING, 3, 3,
			       "Error printing message");
	        break;
	    }
        }
    }

    close_printer();

    if(agg)
      restore_selected(msgmap);
}



/*
 * Support structure and functions to support piping raw message texts...
 */
static struct raw_pipe_data {
    MAILSTREAM *stream;
    long	msgno;
    char       *cur, *body;
} raw_pipe;


int
raw_pipe_getc(c)
     unsigned char *c;
{
    if((!raw_pipe.cur
	&& !(raw_pipe.cur = mail_fetchheader(raw_pipe.stream, raw_pipe.msgno)))
       || (!*raw_pipe.cur && !raw_pipe.body
	   && !(raw_pipe.cur = raw_pipe.body = mail_fetchtext(raw_pipe.stream,
							      raw_pipe.msgno)))
       || (!*raw_pipe.cur && raw_pipe.body))
      return(0);

    *c = (unsigned char) *raw_pipe.cur++;
    return(1);
}


void
prime_raw_text_getc(stream, msgno)
    MAILSTREAM *stream;
    long	msgno;
{
    raw_pipe.stream = stream;
    raw_pipe.msgno  = msgno;
    raw_pipe.cur    = raw_pipe.body = NULL;
}



/*----------------------------------------------------------------------
    Pipe message text

   Args: state -- various pine state bits
	 msgmap -- Message number mapping table
	 agg -- whether or not to aggregate the command on selected msgs

   Filters the original header and sends stuff to specified command
  ---*/
void
cmd_pipe(state, msgmap, agg)
     struct pine *state;
     MSGNO_S *msgmap;
     int	  agg;
{
    ENVELOPE      *e;
    BODY	  *b;
    PIPE_S	  *syspipe;
    char          *resultfilename = NULL, prompt[80];
    int            done = 0, flags;
    gf_io_t	   pc;
    int		   next = 0;
    long           i;
    HelpType       help;
    static	   capture = 1, raw = 0, delimit = 0, newpipe = 0;
    static char    pipe_command[MAXPATH+1];
    static ESCKEY_S pipe_opt[] = {
	{0, 0, "", ""},
	{ctrl('W'), 10, "^W", NULL},
	{ctrl('Y'), 11, "^Y", NULL},
	{ctrl('R'), 12, "^R", NULL},
	{0, 13, "^T", NULL},
	{-1, 0, NULL, NULL}
    };

    if(ps_global->restricted){
	q_status_message(SM_ORDER | SM_DING, 0, 4,
			 "Pine demo can't pipe messages");
	return;
    }
    else if(!any_messages(msgmap, NULL, "to Pipe"))
      return;

    if(agg){
	if(!pseudo_selected(msgmap))
	  return;
	else
	  pipe_opt[4].ch = ctrl('T');
    }
    else
      pipe_opt[4].ch = -1;

    help = NO_HELP;
    while (!done) {
	sprintf(prompt, "Pipe %smessage%s%s to %s%s%s%s%s%s%s: ",
		raw ? "RAW " : "",
		agg ? "s" : " ",
		agg ? "" : comatose(mn_get_cur(msgmap)),
		(!capture || delimit || (newpipe && agg)) ? "(" : "",
		capture ? "" : "uncaptured",
		(!capture && delimit) ? "," : "",
		delimit ? "delimited" : "",
		((!capture || delimit) && newpipe && agg) ? "," : "",
		(newpipe && agg) ? "new pipe" : "",
		(!capture || delimit || (newpipe && agg)) ? ") " : "");
	pipe_opt[1].label = raw ? "Shown Text" : "Raw Text";
	pipe_opt[2].label = capture ? "Free Output" : "Capture Output";
	pipe_opt[3].label = delimit ? "No Delimiter" : "With Delimiter";
	pipe_opt[4].label = newpipe ? "To Same Pipe" : "To Individual Pipes";
	switch(optionally_enter(pipe_command, -FOOTER_ROWS(state), 0,
				MAXPATH, 1, 0, prompt, pipe_opt, help, 0)){
	  case -1 :
	    q_status_message(SM_ORDER | SM_DING, 3, 4,
			     "Internal problem encountered");
	    done++;
	    break;
      
	  case 10 :			/* flip raw bit */
	    raw = !raw;
	    break;

	  case 11 :			/* flip capture bit */
	    capture = !capture;
	    break;

	  case 12 :			/* flip delimit bit */
	    delimit = !delimit;
	    break;

	  case 13 :			/* flip newpipe bit */
	    newpipe = !newpipe;
	    break;

	  case 0 :
	    if(pipe_command[0]){
		flags = PIPE_USER | PIPE_WRITE | PIPE_STDERR;
		if(!capture){
#ifndef	_WINDOWS
		    ClearScreen();
		    fflush(stdout);
		    clear_cursor_pos();
		    ps_global->mangled_screen = 1;
#endif
		    flags |= PIPE_RESET;
		}

		if(!newpipe && !(syspipe = cmd_pipe_open(pipe_command,
							 (flags & PIPE_RESET)
							   ? NULL
							   : &resultfilename,
							 flags, &pc)))
		  done++;

		for(i = mn_first_cur(msgmap);
		    i > 0L && !done;
		    i = mn_next_cur(msgmap)){
		    e = mail_fetchstructure(ps_global->mail_stream,
					    mn_m2raw(msgmap, i), &b);

		    if((newpipe
			&& !(syspipe = cmd_pipe_open(pipe_command,
						     (flags & PIPE_RESET)
						       ? NULL
						       : &resultfilename,
						     flags, &pc)))
		       || (delimit && !bezerk_delimiter(e, pc, next++)))
		      done++;

		    if(!done){
			if(raw){
			    char    *pipe_err;

			    prime_raw_text_getc(ps_global->mail_stream,
						mn_m2raw(msgmap, i));
			    gf_filter_init();
			    gf_link_filter(gf_nvtnl_local);
			    if(pipe_err = gf_pipe(raw_pipe_getc, pc)){
				q_status_message1(SM_ORDER|SM_DING,
						  3, 3,
						  "Internal Error: %s",
						  pipe_err);
				done++;
			    }
			}
			else if(!format_message(mn_m2raw(msgmap, i), e, b,
						(FM_NEW_MESS), pc))
			  done++;
		    }

		    if(newpipe)
		      (void) close_system_pipe(&syspipe);
		}

		if(!newpipe)
		  (void) close_system_pipe(&syspipe);

		if(done)		/* say we had a problem */
		  q_status_message(SM_ORDER | SM_DING, 3, 3,
				   "Error piping message");
		else if(resultfilename){
		    /* only display if no error */
		    display_output_file(resultfilename, "PIPE MESSAGE", NULL);
		    fs_give((void **)&resultfilename);
		}
		else
		  q_status_message(SM_ORDER, 0, 2, "Pipe command completed");

		done++;
		break;
	    }
	    /* else fall thru as if cancelled */

	  case 1 :
	    cmd_cancelled("Pipe command");
	    done++;
	    break;

	  case 3 :
            help = (help == NO_HELP) ? h_pipe_msg : NO_HELP;
	    break;

	  case 2 :                              /* no place to escape to */
	  case 4 :                              /* can't suspend */
	  default :
	    break;   
	}
    }

    ps_global->mangled_footer = 1;
    if(agg)
      restore_selected(msgmap);
}



/*----------------------------------------------------------------------
  Actually open the pipe used to write piped data down

   Args: 
   Returns: TRUE if success, otherwise FALSE

  ----*/
PIPE_S *
cmd_pipe_open(cmd, result, flags, pc)
    char     *cmd;
    char    **result;
    int       flags;
    gf_io_t  *pc;
{
    PIPE_S *pipe;

    if(pipe = open_system_pipe(cmd, result, NULL, flags))
      gf_set_writec(pc, pipe->out.f, 0L, FileStar);
    else
      q_status_message(SM_ORDER | SM_DING, 3, 3, "Error opening pipe") ;

    return(pipe);
}



/*----------------------------------------------------------------------
 Prompt the user for the type of sort desired

   Args: none
   Returns: 0 if search OK (matching numbers selected by side effect)
            1 if there's a problem

   NOTE: any and all functions that successfully exit the second
	 switch() statement below (currently "select_*() functions"),
	 *MUST* update the folder's MESSAGECACHE element's "searched"
	 bits to reflect the search result.  Functions using
	 mail_search() get this for free, the others must update 'em
	 by hand.

  ----*/
void
aggregate_select(state, msgmap, q_line, in_index)
    struct pine *state;
    MSGNO_S     *msgmap;
    int	  q_line, in_index;
{
    long       i, diff, old_tot, msgno;
    int        q = 0, rv = 0, narrow = 0, hidden;
    HelpType   help = NO_HELP;
    ESCKEY_S  *sel_opts;
    extern     MAILSTREAM *mm_search_stream;
    extern     long	   mm_search_count;

    hidden           = any_lflagged(msgmap, MN_HIDE) > 0L;
    mm_search_stream = state->mail_stream;
    mm_search_count  = 0L;

    sel_opts = sel_opts2;
    if(old_tot = any_lflagged(msgmap, MN_SLCT)){
	i = get_lflag(state->mail_stream, msgmap, mn_get_cur(msgmap), MN_SLCT);
	sel_opts1[1].label = "unselect Cur" + (i ? 0 : 2);
	sel_opts += 2;			/* disable extra options */
	switch(q = radio_buttons(sel_pmt1, q_line, sel_opts1, 'c', 'x', help,
				 RB_NORM)){
	  case 'f' :			/* flip selection */
	    msgno = 0L;
	    for(i = 1L; i <= mn_get_total(msgmap); i++){
		q = !get_lflag(state->mail_stream, msgmap, i, MN_SLCT);
		set_lflag(state->mail_stream, msgmap, i, MN_SLCT, q);
		if(hidden){
		    set_lflag(state->mail_stream, msgmap, i, MN_HIDE, !q);
		    if(!msgno && q)
		      mn_reset_cur(msgmap, msgno = i);
		}
	    }

	    return;

	  case 'n' :			/* narrow selection */
	    narrow++;
	  case 'b' :			/* broaden selection */
	    q = 0;			/* but don't offer criteria prompt */
	    break;

	  case 'c' :			/* Un/Select Current */
	  case 'a' :			/* Unselect All */
	  case 'x' :			/* cancel */
	    break;

	  default :
	    q_status_message(SM_ORDER | SM_DING, 3, 3,
			     "Unsupported Select option");
	    return;
	}
    }

    if(!q)
      q = radio_buttons(sel_pmt2, q_line, sel_opts, 'c', 'x', help, RB_NORM);

    /*
     * NOTE: See note about MESSAGECACHE "searched" bits above!
     */
    switch(q){
      case 'x':				/* cancel */
	cmd_cancelled("Select command");
	return;

      case 'c' :			/* select/unselect current */
	(void) individual_select(state, msgmap, q_line, in_index);
	return;

      case 'a' :			/* select/unselect all */
	msgno = any_lflagged(msgmap, MN_SLCT);
	diff    = (!msgno) ? mn_get_total(msgmap) : 0L;

	for(i = 1L; i <= mn_get_total(msgmap); i++){
	    if(msgno){		/* unmark 'em all */
		if(get_lflag(state->mail_stream, msgmap, i, MN_SLCT)){
		    diff++;
		    set_lflag(state->mail_stream, msgmap, i, MN_SLCT, 0);
		}
		else if(hidden)
		  set_lflag(state->mail_stream, msgmap, i, MN_HIDE, 0);
	    }
	    else			/* mark 'em all */
	      set_lflag(state->mail_stream, msgmap, i, MN_SLCT, 1);
	}

	q_status_message4(SM_ORDER,0,2,"%s%s message%s %sselected",
			  msgno ? "" : "All ", comatose(diff), 
			  plural(diff), msgno ? "UN" : "");
	return;

      case 'n' :			/* Select by Number */
	rv = select_number(state->mail_stream, msgmap, mn_get_cur(msgmap));
	break;

      case 'd' :			/* Select by Date */
	rv = select_date(state->mail_stream, msgmap, mn_get_cur(msgmap));
	break;

      case 't' :			/* Text */
	rv = select_text(state->mail_stream, msgmap, mn_get_cur(msgmap));
	break;

      case 's' :			/* Status */
	rv = select_flagged(state->mail_stream, msgmap, mn_get_cur(msgmap));
	break;

      default :
	q_status_message(SM_ORDER | SM_DING, 3, 3,
			 "Unsupported Select option");
	return;
    }

    if(rv)				/* bad return value.. */
      return;				/* error already displayed */

    if(narrow)				/* make sure something was selected */
      for(i = 1L; i <= mn_get_total(msgmap); i++)
	if(mail_elt(state->mail_stream, mn_m2raw(msgmap, i))->searched){
	    if(get_lflag(state->mail_stream, msgmap, i, MN_SLCT))
	      break;
	    else
	      mm_search_count--;
	}

    diff = 0L;
    if(mm_search_count){
	/*
	 * loop thru all the messages, adjusting local flag bits
	 * based on their "searched" bit...
	 */
	for(i = 1L, msgno = 0L; i <= mn_get_total(msgmap); i++)
	  if(narrow){
	      /* turning OFF selectedness if the "searched" bit isn't lit. */
	      if(get_lflag(state->mail_stream, msgmap, i, MN_SLCT)){
		  if(!mail_elt(state->mail_stream,
			       mn_m2raw(msgmap, i))->searched){
		      diff--;
		      set_lflag(state->mail_stream, msgmap, i, MN_SLCT, 0);
		      if(hidden)
			set_lflag(state->mail_stream, msgmap, i, MN_HIDE, 1);
		  }
		  else if(msgno < mn_get_cur(msgmap))
		    msgno = i;
	      }
	  }
	  else if(mail_elt(state->mail_stream,mn_m2raw(msgmap,i))->searched){
	      /* turn ON selectedness if "searched" bit is lit. */
	      if(!get_lflag(state->mail_stream, msgmap, i, MN_SLCT)){
		  diff++;
		  set_lflag(state->mail_stream, msgmap, i, MN_SLCT, 1);
		  if(hidden)
		    set_lflag(state->mail_stream, msgmap, i, MN_HIDE, 0);
	      }
	  }

	/* if we're zoomed and the current message was unselected */
	if(narrow && msgno
	   && get_lflag(state->mail_stream,msgmap,mn_get_cur(msgmap),MN_HIDE))
	  mn_reset_cur(msgmap, msgno);
    }

    if(!diff){
	if(narrow)
	  q_status_message4(SM_ORDER, 0, 2,
			"%s.  %s message%s remain%s selected.",
			mm_search_count ? "No change resulted"
					: "No messages in intersection",
			comatose(old_tot), plural(old_tot),
			(old_tot == 1L) ? "s" : "");
	else if(old_tot && mm_search_count)
	  q_status_message(SM_ORDER, 0, 2,
		   "No change resulted.  Matching messages already selected.");
	else
	  q_status_message1(SM_ORDER | SM_DING, 0, 2,
			    "Select failed!  No %smessages selected.",
			    old_tot ? "additional " : "");
    }
    else if(old_tot){
	sprintf(tmp_20k_buf,
		"Select matched %ld message%s!  %s %smessage%s %sselected.",
		(diff > 0) ? diff : old_tot + diff,
		plural((diff > 0) ? diff : old_tot + diff),
		comatose((diff > 0) ? any_lflagged(msgmap, MN_SLCT) : -diff),
		(diff > 0) ? "total " : "",
		plural((diff > 0) ? any_lflagged(msgmap, MN_SLCT) : -diff),
		(diff > 0) ? "" : "UN");
	q_status_message(SM_ORDER, 0, 2, tmp_20k_buf);
    }
    else
      q_status_message2(SM_ORDER, 0, 2, "Select matched %s message%s!",
			comatose(diff), plural(diff));
}



/*----------------------------------------------------------------------
 Toggle the state of the current message

   Args: state -- pointer pine's state variables
	 msgmap -- message collection to operate on
	 q_line -- line on display to write prompts
	 in_index -- in the message index view
   Returns: TRUE if current marked selected, FALSE otw
  ----*/
int
individual_select(state, msgmap, q_line, in_index)
     struct pine *state;
     MSGNO_S     *msgmap;
     int	  q_line, in_index;
{
    long i;
    int  rv;

    i = mn_get_cur(msgmap);
    if(rv = get_lflag(state->mail_stream, msgmap, i, MN_SLCT)){ /* set? */
	set_lflag(state->mail_stream, msgmap, i, MN_SLCT, 0);
	if(any_lflagged(msgmap, MN_HIDE) > 0L){
	    set_lflag(state->mail_stream, msgmap, i, MN_HIDE, 1);
	    /*
	     * See if there's anything left to zoom on.  If so, 
	     * pick an adjacent one for highlighting, else make
	     * sure nothing is left hidden...
	     */
	    if(any_lflagged(msgmap, MN_SLCT)){
		mn_inc_cur(state->mail_stream, msgmap);
		if(mn_get_cur(msgmap) == i)
		  mn_dec_cur(state->mail_stream, msgmap);
	    }
	    else{			/* clear all hidden flags */
		for(i = 1L; i <= mn_get_total(msgmap); i++)
		  set_lflag(state->mail_stream, msgmap, i, MN_HIDE, 0);
	    }
	}
    }
    else
      set_lflag(state->mail_stream, msgmap, i, MN_SLCT, 1);

    if(!in_index)
      q_status_message2(SM_ORDER, 0, 2, "Message %s %sselected",
			long2string(i), rv ? "UN" : "");

    return(!rv);
}



/*----------------------------------------------------------------------
 Prompt the user for the command to perform on selected messages

   Args: state -- pointer pine's state variables
	 msgmap -- message collection to operate on
	 q_line -- line on display to write prompts
   Returns: 1 if the selected messages are suitably commanded,
	    0 if the choice to pick the command was declined

  ----*/
int
apply_command(state, msgmap, q_line)
     struct pine *state;
     MSGNO_S     *msgmap;
     int	  q_line;
{
    int i = 8, rv = 1;
    int we_cancel = 0;

    if(F_ON(F_ENABLE_FLAG,state)){ /* flag? */
	sel_opts3[i].ch      = '*';
	sel_opts3[i].rval    = '*';
	sel_opts3[i].name    = "*";
	sel_opts3[i++].label = "Flag";
    }

    if(F_ON(F_ENABLE_PIPE,state)){ /* pipe? */
	sel_opts3[i].ch      = '|';
	sel_opts3[i].rval    = '|';
	sel_opts3[i].name    = "|";
	sel_opts3[i++].label = "Pipe";
    }

    /*
     * This doesn't fit on the normal keymenu, so it will go in the help
     * slot instead (see "hacking" in status.c).  If either of above two
     * commands are disabled then it does fit.
     */
    if(F_ON(F_ENABLE_BOUNCE,state)){ /* bounce? */
	sel_opts3[i].ch      = 'b';
	sel_opts3[i].rval    = 'b';
	sel_opts3[i].name    = "B";
	sel_opts3[i++].label = "Bounce";
    }

    sel_opts3[i].ch = -1;
    /*
     * Can't put an actual help in here instead of NO_HELP.  See comment
     * above.
     */
    switch(radio_buttons(sel_pmt3,q_line,sel_opts3,0,'x',NO_HELP,RB_NORM)){
      case 'd' :			/* delete */
	we_cancel = busy_alarm(1, NULL, NULL, 0);
	cmd_delete(state, msgmap, 1);
	if(we_cancel)
	  cancel_busy_alarm(0);
	break;

      case 'u' :			/* undelete */
	we_cancel = busy_alarm(1, NULL, NULL, 0);
	cmd_undelete(state, msgmap, 1);
	if(we_cancel)
	  cancel_busy_alarm(0);
	break;

      case 'r' :			/* reply */
	cmd_reply(state, msgmap, 1);
	break;

      case 'f' :			/* Forward */
	cmd_forward(state, msgmap, 1);
	break;

      case 'y' :			/* prYnt */
	cmd_print(state, msgmap, 1, 1);
	break;

      case 't' :			/* take address */
	cmd_take_addr(state, msgmap, 1);
	break;

      case 's' :			/* save */
	cmd_save(state, msgmap, q_line, 1);
	break;

      case 'e' :			/* export */
	cmd_export(state, msgmap, q_line, 1);
	break;

      case '|' :			/* pipe */
	cmd_pipe(state, msgmap, 1);
	break;

      case '*' :			/* flag */
	we_cancel = busy_alarm(1, NULL, NULL, 0);
	cmd_flag(state, msgmap, 1);
	if(we_cancel)
	  cancel_busy_alarm(0);
	break;

      case 'b' :			/* bounce */
	cmd_bounce(state, msgmap, 1);
	break;

      case 'x' :			/* cancel */
	cmd_cancelled("Apply command");
	rv = 0;
	break;

	default:
	break;
    }

    return(rv);
}



/*----------------------------------------------------------------------
  ZOOM the message index (set any and all necessary hidden flag bits)

   Args: state -- usual pine state
	 msgmap -- usual message mapping
   Returns: number of messages zoomed in on

  ----*/
long
zoom_index(state, msgmap, cur_msgno)
    struct pine *state;
    MSGNO_S	*msgmap;
    long	*cur_msgno;
{
    long i, count = 0L, first = 0L;

    if(any_lflagged(msgmap, MN_SLCT)){
	for(i = 1L; i <= mn_get_total(msgmap); i++){
	    if(!get_lflag(state->mail_stream, msgmap, i, MN_SLCT)){
		set_lflag(state->mail_stream, msgmap, i, MN_HIDE, 1);
	    }
	    else{
		count++;
		if(!first)
		  first = i;
	    }
	}

	if(!get_lflag(state->mail_stream, msgmap, *cur_msgno, MN_SLCT))
	  mn_set_cur(msgmap, *cur_msgno = first);
    }

    return(count);
}



/*----------------------------------------------------------------------
  UnZOOM the message index (clear any and all hidden flag bits)

   Args: state -- usual pine state
	 msgmap -- usual message mapping
   Returns: 1 if hidden bits to clear and they were, 0 if none to clear

  ----*/
int
unzoom_index(state, msgmap)
    struct pine *state;
    MSGNO_S	*msgmap;
{
    register long i;

    if(!any_lflagged(msgmap, MN_HIDE))
      return(0);

    for(i = 1L; i <= mn_get_total(msgmap); i++)
      if(get_lflag(state->mail_stream, msgmap, i, MN_HIDE))
	set_lflag(state->mail_stream, msgmap, i, MN_HIDE, 0);

    return(1);
}



/*----------------------------------------------------------------------
 Prompt the user for the type of sort he desires

   Args: none
   Returns: 0 if search OK (matching numbers selected by side effect)
            1 if there's a problem

  ----*/
int
select_number(stream, msgmap, msgno)
     MAILSTREAM *stream;
     MSGNO_S    *msgmap;
     long	 msgno;
{
    int r;
    long n1, n2;
    char number1[16], number2[16], numbers[80], *p, *t;
    HelpType help;

    numbers[0] = '\0';
    ps_global->mangled_footer = 1;
    help = NO_HELP;
    while(1){
        r = optionally_enter(numbers, -FOOTER_ROWS(ps_global), 0, 79, 1, 0,
                             select_num, NULL, help, 0);
        if(r == 4)
	  continue;

        if(r == 3){
            help = (help == NO_HELP) ? h_select_by_num : NO_HELP;
	    continue;
	}

	for(t = p = numbers; *p ; p++)	/* strip whitespace */
	  if(!isspace((unsigned char)*p))
	    *t++ = *p;

	*t = '\0';

        if(r == 1 || numbers[0] == '\0'){
	    cmd_cancelled("Selection by number");
	    return(1);
        }
	else
	  break;
    }

    for(n1 = 1; n1 <= stream->nmsgs; n1++)
      mail_elt(stream, n1)->searched = 0;	/* clear searched bits */

    for(p = numbers; *p ; p++){
	t = number1;
	while(*p && isdigit((unsigned char)*p))
	  *t++ = *p++;

	*t = '\0';
	if((n1 = atol(number1)) < 1L || n1 > mn_get_total(msgmap)){
	    q_status_message1(SM_ORDER | SM_DING, 0, 2,
			      "%s out of message number range",
			      long2string(n1));
	    return(1);
	}

	t = number2;
	if(*p && *p == '-'){
	    while(*++p && isdigit((unsigned char)*p))
	      *t++ = *p;

	    *t = '\0';
	    if((n2 = atol(number2)) < 1L 
	       || n2 > mn_get_total(msgmap)){
		q_status_message1(SM_ORDER | SM_DING, 0, 2,
				  "%s out of message number range",
				  long2string(n2));
		return(1);
	    }

	    if(n2 <= n1){
		q_status_message(SM_ORDER | SM_DING, 0, 2,
				 "Invalid message number range");
		break;
	    }

	    for(;n1 <= n2; n1++)
	      mail_searched(stream, mn_m2raw(msgmap, n1));
	}
	else
	  mail_searched(stream, mn_m2raw(msgmap, n1));

	if(*p == '\0')
	  break;
    }
    
    return(0);
}



/*----------------------------------------------------------------------
 Prompt the user for the type of sort he desires

   Args: none
   Returns: 0 if search OK (matching numbers selected by side effect)
            1 if there's a problem

  ----*/
int
select_date(stream, msgmap, msgno)
     MAILSTREAM *stream;
     MSGNO_S    *msgmap;
     long	 msgno;
{
    int  r, we_cancel = 0, when = 0;
    char date[32], defdate[32], prompt[128];
    time_t	   seldate = time(0);
    struct tm *seldate_tm;
    static char *when_range[] = {"ON", "SINCE", "BEFORE"},
		*when_mod[] = {"", " (inclusive)", " (exclusive)"};

    date[0] = '\0';
    ps_global->mangled_footer = 1;

    while(1){
	seldate_tm = localtime(&seldate);
	sprintf(defdate, "%.2d-%.4s-%.4d", seldate_tm->tm_mday,
		month_abbrev(seldate_tm->tm_mon + 1),
		seldate_tm->tm_year + 1900);
	sprintf(prompt,"Select messages arriving %s%s [%s]: ",
		when_range[when], when_mod[when], defdate);
	r = optionally_enter(date,-FOOTER_ROWS(ps_global), 0, 31, 1, 0,
			     prompt, sel_date_opt, NO_HELP, 0);
	switch (r){
	  case 1 :
	    cmd_cancelled("Selection by date");
	    return(1);

	  case 3 :
	  case 4 :
	    continue;

	  case 11 :
	    {
		MESSAGECACHE *mc;

		if(mc = mail_elt(stream, mn_m2raw(msgmap, msgno))){
		    /* mail_date returns fixed field width date */
		    *(mail_date(tmp_20k_buf, mc) + 11) = '\0';
		    strcpy(date, tmp_20k_buf);
		}
	    }

	    continue;

	  case 12 :			/* set default to PREVIOUS day */
	    seldate -= 86400;
	    continue;

	  case 13 :			/* set default to NEXT day */
	    seldate += 86400;
	    continue;

	  case 14 :
	    when = ++when % (sizeof(when_range) / sizeof(char *));
	    continue;

	  default:
	    break;
	}

	removing_leading_white_space(date);
	removing_trailing_white_space(date);
	if(!*date)
	  strcpy(date, defdate);

	break;
    }

    we_cancel = busy_alarm(1, "Busy Selecting", NULL, 0);
    sprintf(prompt, "%s %s", when_range[when], date);
    mail_search(stream, prompt);
    if(we_cancel)
      cancel_busy_alarm(0);

    return(0);
}



/*----------------------------------------------------------------------
 Prompt the user for the type of sort he desires

   Args: none
   Returns: 0 if search OK (matching numbers selected by side effect)
            1 if there's a problem

  ----*/
int
select_text(stream, msgmap, msgno)
     MAILSTREAM *stream;
     MSGNO_S    *msgmap;
     long	 msgno;
{
    int       r, type, we_cancel = 0;
    char     *sval = NULL, sstring[80], tmp[128];
    ESCKEY_S  ekey[3];
    ENVELOPE *env = NULL;
    HelpType  help;

    ps_global->mangled_footer = 1;

    /*
     * prepare some friendly defaults...
     */
    ekey[1].ch   = -1;
    ekey[2].ch   = -1;
    switch(type = radio_buttons(sel_text, -FOOTER_ROWS(ps_global),
				sel_text_opt, 's', 'x', NO_HELP, RB_NORM)){
      case 't' :			/* address fields, offer To or From */
      case 'f' :
      case 'c' :
	sval          = (type == 't') ? "TO" : (type == 'f') ? "FROM" : "CC";
	ekey[0].ch    = ctrl('T');
	ekey[0].name  = "^T";
	ekey[0].rval  = 10;
	ekey[0].label = "Cur To";
	ekey[1].ch    = ctrl('R');
	ekey[1].name  = "^R";
	ekey[1].rval  = 11;
	ekey[1].label = "Cur From";
	break;

      case 's' :
	sval          = "SUBJECT";
	ekey[0].ch    = ctrl('X');
	ekey[0].name  = "^X";
	ekey[0].rval  = 13;
	ekey[0].label = "Cur Subject";
	break;

      case 'a' :
	sval = "TEXT";			/* fall thru */
	ekey[0].ch = -1;
	break;

      case 'x':
	break;

      default:
	dprint(1, (debugfile,"\n - BOTCH: select_text unrecognized option\n"));
	return(1);
    }

    if(type != 'x'){
	if(ekey[0].ch > -1 && msgno > 0L
	   && !(env=mail_fetchstructure(stream,mn_m2raw(msgmap,msgno),NULL)))
	  ekey[0].ch = -1;

	sstring[0] = '\0';
	help = NO_HELP;
	r = type;
	while(r != 'x'){
	    sprintf(tmp, "String in message %s to match : ", sval);
	    switch(r = optionally_enter(sstring, -FOOTER_ROWS(ps_global), 0,
					79, 1, 0, tmp, ekey, help, 0)){
	      case 3 :
		help = (help == NO_HELP)
			    ? ((type == 'f') ? h_select_txt_from
			      : (type == 't') ? h_select_txt_to
			       : (type == 'c') ? h_select_txt_cc
				: (type == 's') ? h_select_txt_subj
				 : (type == 'a') ? h_select_txt_all
				  :              NO_HELP)
			    : NO_HELP;
	      case 4 :
		continue;

	      case 10 :			/* To: default */
		if(env && env->to && env->to->mailbox)
		  sprintf(sstring, "%.30s%s%.40s", env->to->mailbox,
			  env->to->host ? "@" : "",
			  env->to->host ? env->to->host : "");
		continue;

	      case 11 :			/* From: default */
		if(env && env->from && env->from->mailbox)
		  sprintf(sstring, "%.30s%s%.40s", env->from->mailbox,
			  env->from->host ? "@" : "",
			  env->from->host ? env->from->host : "");
		continue;

	      case 12 :			/* Cc: default */
		if(env && env->cc && env->cc->mailbox)
		  sprintf(sstring, "%.30s%s%.40s", env->cc->mailbox,
			  env->cc->host ? "@" : "",
			  env->cc->host ? env->cc->host : "");
		continue;

	      case 13 :			/* Subject: default */
		if(env && env->subject && env->subject[0]){
		    char *p = NULL;
		    sprintf(sstring, "%.70s",
			    rfc1522_decode((unsigned char *)tmp_20k_buf,
					   env->subject, &p));
		    if(p)
		      fs_give((void **) &p);
		}

		continue;

	      default :
		break;
	    }

	    if(r == 1 || sstring[0] == '\0')
	      r = 'x';

	    break;
	}
    }

    if(type == 'x' || r == 'x'){
	cmd_cancelled("Selection by text");
	return(1);
    }

    sprintf(tmp, "%s ", sval);
    sval = tmp + strlen(tmp);		/* sval overloaded! */
    if(strpbrk(sstring, "\012\015\"%{\\() "))
      sprintf(sval, "{%d}\015\012%s", strlen(sstring), sstring);
    else
      strcpy(sval, sstring);

    we_cancel = busy_alarm(1, "Busy Selecting", NULL, 0);
    mail_search(stream, tmp);
    if(we_cancel)
      cancel_busy_alarm(0);

    return(0);
}



/*----------------------------------------------------------------------
 Prompt the user for the type of sort he desires

   Args: none
   Returns: 0 if search OK (matching numbers selected by side effect)
            1 if there's a problem

  ----*/
int
select_flagged(stream, msgmap, msgno)
     MAILSTREAM *stream;
     MSGNO_S    *msgmap;
     long	 msgno;
{
    int   s, not = 0, we_cancel = 0;
    char *flagp;

    while(1){
	s = radio_buttons((not) ? sel_flag_not : sel_flag,
			  -FOOTER_ROWS(ps_global), sel_flag_opt, '*', 'x',
			  NO_HELP, RB_NORM);
			  
	if(s == 'x'){
	    cmd_cancelled("Selection by status");
	    return(1);
	}
	else if(s == '!')
	  not = !not;
	else
	  break;
    }

    flagp = cpystr((s == 'n') ? (not) ? "SEEN"
				      : "UNSEEN UNDELETED UNANSWERED" :
		      (s == 'd') ? (not) ? "UNDELETED"
					 : "DELETED" :
			 (s == 'a') ? (not) ? "UNANSWERED"
					    : "ANSWERED UNDELETED" :
			    (not) ? "UNFLAGGED" : "FLAGGED");
    we_cancel = busy_alarm(1, "Busy Selecting", NULL, 0);
    mail_search(stream, flagp);
    if(we_cancel)
      cancel_busy_alarm(0);

    fs_give((void **)&flagp);
    return(0);
}



/*----------------------------------------------------------------------
   Prompt the user for the type of sort he desires

Args: state -- pine state pointer
      q1 -- Line to prompt on

      Returns 0 if it was cancelled, 1 otherwise.
  ----*/
int
select_sort(state, ql)
     struct pine *state;
     int ql;
{
    char      prompt[200], tmp[3], *p;
    int       s, i;
    int       deefault = 'a', retval = 1;
    HelpType  help;
    ESCKEY_S  sorts[10];

#ifdef _WINDOWS
    DLG_SORTPARAM	sortsel;

    if (mswin_usedialog ()) {

	sortsel.reverse = mn_get_revsort (state->msgmap);
	sortsel.cursort = mn_get_sort (state->msgmap);
	sortsel.helptext = get_help_text (h_select_sort, NULL);
	sortsel.rval = 0;

	if ((retval = os_sortdialog (&sortsel))) {
	    mn_set_revsort (state->msgmap, sortsel.reverse);
	    mn_set_sort (state->msgmap, sortsel.cursort);
        }

	if (sortsel.helptext != NULL) 
	    free_help_text (sortsel.helptext);
	return (retval);
    }
#endif

    /*----- String together the prompt ------*/
    tmp[1] = '\0';
    strcpy(prompt, "Choose type of sort, or Reverse current sort : ");
    for(i = 0; state->sort_types[i] != EndofList && i < 8; i++) {
	sorts[i].rval	   = i;
	p = sorts[i].label = sort_name(state->sort_types[i]);
	while(*(p+1) && islower((unsigned char)*p))
	  p++;

	sorts[i].ch   = tolower((unsigned char)(tmp[0] = *p));
	sorts[i].name = cpystr(tmp);

        if(mn_get_sort(state->msgmap) == state->sort_types[i])
	  deefault = sorts[i].rval;
    }

    sorts[i].ch     = 'r';
    sorts[i].rval   = 'r';
    sorts[i].name   = cpystr("R");
    sorts[i].label  = "Reverse";
    sorts[++i].ch   = -1;
    help = h_select_sort;

    if((s = radio_buttons(prompt,ql,sorts,deefault,'x',help,RB_NORM)) != 'x'){
	state->mangled_body = 1;		/* signal screen's changed */
	if(s == 'r'){
	    mn_set_revsort(state->msgmap, !mn_get_revsort(state->msgmap));
	}
	else
	  mn_set_sort(state->msgmap, state->sort_types[s]);
    }
    else{
	retval = 0;
	cmd_cancelled("Sort");
    }

    while(--i >= 0)
      fs_give((void **)&sorts[i].name);

    blank_keymenu(ps_global->ttyo->screen_rows - 2, 0);
    return(retval);
}




/*----------------------------------------------------------------------
  Build comma delimited list of selected messages

  Args: stream -- mail stream to use for flag testing
	msgmap -- message number struct of to build selected messages in
	count -- pointer to place to write number of comma delimited

  Returns: malloc'd string containing sequence, else NULL if
	   no messages in msgmap with local "selected" flag.
  ----*/
char *
selected_sequence(stream, msgmap, count)
    MAILSTREAM *stream;
    MSGNO_S    *msgmap;
    long       *count;
{
    long  i;

    if(!any_lflagged(msgmap, MN_SLCT) || !stream)
      return(NULL);

    /*
     * The plan here is to use the c-client elt's "sequence" bit
     * to work around any orderings or exclusions in pine's internal
     * mapping that might cause the sequence to be artificially
     * lengthy.  It's probably cheaper to run down the elt list
     * twice rather than call nm_raw2m() for each message as
     * we run down the elt list once...
     */
    for(i = 1L; i <= stream->nmsgs; i++)
      mail_elt(stream, i)->sequence = 0;

    for(i = 1L; i <= mn_get_total(msgmap); i++)
      if(get_lflag(stream, msgmap, i, MN_SLCT)){
	  /*
	   * Forget we knew about it, and set "add to sequence"
	   * bit...
	   */
	  clear_index_cache_ent(i);
	  mail_elt(stream, mn_m2raw(msgmap, i))->sequence = 1;
      }

    return(build_sequence(stream, NULL, count));
}


/*----------------------------------------------------------------------
  Build comma delimited list of current, flagged messages

  Args: stream -- mail stream to use for flag testing
	msgmap -- message number struct of to build selected messages in
	flag -- system flag to 
	count -- pointer to place to write number of comma delimited
	mark -- mark index cache entry changed, and count state change

  Returns: malloc'd string containing sequence, else NULL if
	   no messages in msgmap with local "selected" flag (a flag
	   of zero means all current msgs).
  ----*/
char *
currentf_sequence(stream, msgmap, flag, count, mark)
    MAILSTREAM *stream;
    MSGNO_S    *msgmap;
    long	flag;
    long       *count;
    int		mark;
{
    long	  i;
    MESSAGECACHE *mc;

    FETCH_ALL_FLAGS(stream);			/* make sure all flags valid */
    for(i = 1L; i <= stream->nmsgs; i++)
      mail_elt(stream, i)->sequence = 0;	/* clear "sequence" bits */

    for(i = mn_first_cur(msgmap); i > 0L; i = mn_next_cur(msgmap)){
	/* if not already set, go on... */
	mc = mail_elt(stream, mn_m2raw(msgmap, i));
	if((flag == F_DEL && !mc->deleted)
	   || (flag == F_UNDEL && mc->deleted)
	   || (flag == F_SEEN && !mc->seen)
	   || (flag == F_UNSEEN && mc->seen)
	   || (flag == F_ANS && !mc->answered)
	   || (flag == F_UNANS && mc->answered)
	   || (flag == F_FLAG && !mc->flagged)
	   || (flag == F_UNFLAG && mc->flagged))
	  continue;

	mc->sequence = 1;			/* set "sequence" flag */
	if(mark){
	    clear_index_cache_ent(i);		/* force new index line */
	    check_point_change();		/* count state change */
	}
    }

    return(build_sequence(stream, NULL, count));
}


/*----------------------------------------------------------------------
  Build comma delimited list of messages with elt "sequence" bit set

  Args: stream -- mail stream to use for flag testing
	msgmap -- struct containing sort to build sequence in
	count -- pointer to place to write number of comma delimited
		 NOTE: if non-zero, it's a clue as to how many messages
		       have the sequence bit lit.

  Returns: malloc'd string containing sequence, else NULL if
	   no messages in msgmap with elt's "sequence" bit set
  ----*/
char *
build_sequence(stream, msgmap, count)
    MAILSTREAM *stream;
    MSGNO_S    *msgmap;
    long       *count;
{
#define	SEQ_INCREMENT	128
    long    n = 0L, i, x, lastn = 0L, runstart = 0L;
    size_t  size = SEQ_INCREMENT;
    char   *seq = NULL, *p;

    if(count){
	if(*count > 0L)
	  size = min((*count) * 4, 16384);

	*count = 0L;
    }

    for(x = 1L; x <= stream->nmsgs; x++){
	if(msgmap){
	    if((i = mn_m2raw(msgmap, x)) == 0L)
	      break;
	}
	else
	  i = x;

	if(mail_elt(stream, i)->sequence){
	    n++;
	    if(!seq)				/* initialize if needed */
	      seq = p = fs_get(size);

	    /*
	     * This code will coalesce the ascending runs of
	     * sequence numbers, but fails to break sequences
	     * into a reasonably sensible length for imapd's to
	     * swallow (reasonable addtition to c-client?)...
	     */
	    if(lastn){				/* if may be in a run */
		if(lastn + 1L == i){		/* and its the next raw num */
		    lastn = i;			/* skip writing anything... */
		    continue;
		}
		else if(runstart != lastn){
		    *p++ = (runstart + 1L == lastn) ? ',' : ':';
		    sstrcpy(&p, long2string(lastn));
		}				/* wrote end of run */
	    }

	    runstart = lastn = i;		/* remember last raw num */

	    if(n > 1L)				/* !first num, write delim */
	      *p++ = ',';

	    if(size - (p - seq) < 16){	/* room for two more nums? */
		size_t offset = p - seq;	/* grow the sequence array */
		size += SEQ_INCREMENT;
		fs_resize((void **)&seq, size);
		p = seq + offset;
	    }

	    sstrcpy(&p, long2string(i));	/* write raw number */
	}
    }

    if(lastn && runstart != lastn){		/* were in a run? */
	*p++ = (runstart + 1L == lastn) ? ',' : ':';
	sstrcpy(&p, long2string(lastn));	/* write the trailing num */
    }

    if(seq)					/* if sequence, tie it off */
      *p  = '\0';

    if(count)
      *count = n;

    return(seq);
}



/*----------------------------------------------------------------------
  If any messages flagged "selected", fake the "currently selected" array

  Args: map -- message number struct of to build selected messages in

  OK folks, here's the tradeoff: either all the functions have to
  know if the user want's to deal with the "current" hilited message
  or the list of currently "selected" messages, *or* we just
  wrap the call to these functions with some glue that tweeks
  what these functions see as the "current" message list, and let them
  do their thing.
  ----*/
int
pseudo_selected(map)
    MSGNO_S *map;
{
    long i, later = 0L;

    if(any_lflagged(map, MN_SLCT)){
	map->hilited = mn_m2raw(map, mn_get_cur(map));

	for(i = 1L; i <= mn_get_total(map); i++)
	  /* BUG: using the global mail_stream is kind of bogus since
	   * everybody that calls us get's a pine stuct passed it.
	   * perhaps a stream pointer in the message struct makes 
	   * sense?
	   */
	  if(get_lflag(ps_global->mail_stream, map, i, MN_SLCT)){
	      if(!later++){
		  mn_set_cur(map, i);
	      }
	      else{
		  mn_add_cur(map, i);
	      }
	  }

	return(1);
    }

    return(0);
}


/*----------------------------------------------------------------------
  Antidote for the monkey business committed above

  Args: map -- message number struct of to build selected messages in

  ----*/
void
restore_selected(map)
    MSGNO_S *map;
{
    if(map->hilited){
	mn_reset_cur(map, mn_raw2m(map, map->hilited));
	map->hilited = 0L;
    }
}


/*
 * Get the user name from the mailbox portion of an address.
 *
 * Args: mailbox -- the mailbox portion of an address (lhs of address)
 *       target  -- a buffer to put the result in
 *       len     -- length of the target buffer
 *
 * Returns the left most portion up to the first '%', ':' or '@',
 * and to the right of any '!' (as if c-client would give us such a mailbox).
 * Returns NULL if it can't find a username to point to.
 */
char *
get_uname(mailbox, target, len)
    char  *mailbox,
	  *target;
    int    len;
{
    int i, start, end;

    if(!mailbox || !*mailbox)
      return(NULL);

    end = strlen(mailbox) - 1;
    for(start = end; start > -1 && mailbox[start] != '!'; start--)
        if(strindex("%:@", mailbox[start]))
	    end = start - 1;

    start++;			/* compensate for either case above */

    for(i = start; i <= end && (i-start) < (len-1); i++) /* copy name */
      target[i-start] = isupper((unsigned char)mailbox[i])
					  ? tolower((unsigned char)mailbox[i])
					  : mailbox[i];

    target[i-start] = '\0';	/* tie it off */

    return(*target ? target : NULL);
}


/*
 * file_lister - call pico library's file lister
 */
int
file_lister(title, path, file, newmail, flags)
    char *title, *path, *file;
    int   newmail, flags;
{
    PICO pbuf;
    int  rv;

    memset(&pbuf, 0, sizeof(PICO));
/* BUG: what about help command and text? */
    pbuf.raw_io        = Raw;
    pbuf.showmsg       = display_message_for_pico;
    pbuf.keybinit      = init_keyboard;
    pbuf.helper        = helper;
    pbuf.resize	       = resize_for_pico;
    pbuf.browse_help   = h_composer_browse;
    pbuf.menu_rows     = FOOTER_ROWS(ps_global) - 1;
    pbuf.pine_anchor   = title;
    pbuf.pine_version  = pine_version;
    pbuf.pine_flags    = flags_for_pico(ps_global);
    if(ps_global->VAR_OPER_DIR){
	pbuf.oper_dir    = ps_global->VAR_OPER_DIR;
	pbuf.pine_flags |= P_TREE;
    }

    if(newmail)
      pbuf.newmail = new_mail_for_pico;

    rv = pico_file_browse(&pbuf, path, file, NULL, flags);
    fix_windsize(ps_global);
    init_signals();		/* has it's own signal stuff */
    redraw_titlebar();
    if(ps_global->redrawer != (void (*)())NULL)
      (*ps_global->redrawer)();

    redraw_keymenu();
    return(rv);
}
