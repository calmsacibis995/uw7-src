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
     status.c
     Functions that manage the status line (third from the bottom)
       - put messages on the queue to be displayed
       - display messages on the queue with timers 
       - check queue to figure out next timeout
       - prompt for yes/no type of questions
  ====*/

#include "headers.h"

/*
 * Internal queue of messages.  The circular, double-linked list's 
 * allocated on demand, and cleared as each message is displayed.
 */
typedef struct message {
    char	    text[MAX_SCREEN_COLS+1];
    unsigned	    flags:8;
    unsigned	    shown:1;
    int		    min_display_time, max_display_time;
    struct message *next, *prev;
} SMQ_T;

#define	LAST_MESSAGE(X)	((X) == (X)->next)
#define	RAD_BUT_COL	0


/*
 * Internal prototypes
 */
int  output_message PROTO((SMQ_T *));
void radio_help PROTO((int, int, HelpType));
void draw_radio_prompt PROTO((int, int, char *));
void pause_for_current_message PROTO(());
int  messages_in_queue PROTO(());
void delay_cmd_cue PROTO((int));



/*----------------------------------------------------------------------
     Manage the second line from the bottom where status and error messages
are displayed. A small queue is set up and messages are put on the queue
by calling one of the q_status_message routines. Even though this is a queue
most of the time message will go right on through. The messages are 
displayed just before the read for the next command, or when a read times
out. Read timeouts occur every minute or so for new mail checking and every
few seconds when there are still messages on the queue. Hopefully, this scheme 
will not let messages fly past that the user can't see.
  ----------------------------------------------------------------------*/


static SMQ_T *message_queue = NULL;
static short  needs_clearing = 0, /* Flag set by want_to()
                                              and optionally_enter() */
	      prevstartcol;
static char   prevstatusbuff[MAX_SCREEN_COLS+1];
static time_t displayed_time;


/*----------------------------------------------------------------------
        Put a message for the status line on the queue

  Args: time    -- the min time in seconds to display the message
        message -- message string

  Result: queues message on queue represented by static variables

    This puts a single message on the queue to be shown.
  ----------*/
void
q_status_message(flags, min_time, max_time, message)
    int   flags;
    int   min_time,max_time;
    char *message;
{
    SMQ_T *new;

    if((flags & SM_INFO) && message_queue)
      return;

    new = (SMQ_T *)fs_get(sizeof(SMQ_T));
    memset(new, 0, sizeof(SMQ_T));
    strncpy(new->text, message, MAX_SCREEN_COLS);
    new->min_display_time = min_time;
    new->max_display_time = max_time;
    new->flags            = flags;
    if(message_queue){
	new->next = message_queue;
	new->prev = message_queue->prev;
	new->prev->next = message_queue->prev = new;
    }
    else
      message_queue = new->next = new->prev = new;

    dprint(9, (debugfile, "q_status_message(%.40s)\n", message));
}


/*----------------------------------------------------------------------
        Put a message with 1 printf argument on queue for status line
 
    Args: min_t -- minimum time to display message for
          max_t -- minimum time to display message for
          s -- printf style control string
          a -- argument for printf
 
   Result: message queued
  ----*/

/*VARARGS1*/
void
q_status_message1(flags, min_t, max_t, s, a)
    int	  flags;
    int   min_t, max_t;
    char *s;
    void *a;
{
    char buf[1000];

    sprintf(buf, s, a);
    q_status_message(flags, min_t, max_t, buf);
}



/*----------------------------------------------------------------------
        Put a message with 2 printf argument on queue for status line

    Args: min_t  -- minimum time to display message for
          max_t  -- maximum time to display message for
          s  -- printf style control string
          a1 -- argument for printf
          a2 -- argument for printf

  Result: message queued
  ---*/

/*VARARGS1*/
void
q_status_message2(flags, min_t, max_t, s, a1, a2)
    int   flags;
    int   min_t, max_t;
    char *s;
    void *a1, *a2;
{
    char buf[1000];

    sprintf(buf, s, a1, a2);
    q_status_message(flags, min_t, max_t, buf);
}



/*----------------------------------------------------------------------
        Put a message with 3 printf argument on queue for status line

    Args: min_t  -- minimum time to display message for
          max_t  -- maximum time to display message for
          s  -- printf style control string
          a1 -- argument for printf
          a2 -- argument for printf
          a3 -- argument for printf

  Result: message queued
  ---*/

/*VARARGS1*/
void
q_status_message3(flags, min_t, max_t, s, a1, a2, a3)
    int   flags;
    int   min_t, max_t;
    char *s;
    void *a1, *a2, *a3;
{
    char buf[1000];

    sprintf(buf, s, a1, a2, a3);
    q_status_message(flags, min_t, max_t, buf);
}



/*----------------------------------------------------------------------
        Put a message with 4 printf argument on queue for status line


    Args: min_t  -- minimum time to display message for
          max_t  -- maximum time to display message for
          s  -- printf style control string
          a1 -- argument for printf
          a2 -- argument for printf
          a3 -- argument for printf
          a4 -- argument for printf

  Result: message queued
  ----------------------------------------------------------------------*/
/*VARARGS1*/
void
q_status_message4(flags, min_t, max_t, s, a1, a2, a3, a4)
    int   flags;
    int   min_t, max_t;
    char *s;
    void *a1, *a2, *a3, *a4;
{
    char buf[1000];

    sprintf(buf, s, a1, a2, a3, a4);
    q_status_message(flags, min_t, max_t, buf);
}


/*----------------------------------------------------------------------
        Put a message with 7 printf argument on queue for status line


    Args: min_t  -- minimum time to display message for
          max_t  -- maximum time to display message for
          s  -- printf style control string
          a1 -- argument for printf
          a2 -- argument for printf
          a3 -- argument for printf
          a4 -- argument for printf
          a5 -- argument for printf
          a6 -- argument for printf
          a7 -- argument for printf


  Result: message queued
  ----------------------------------------------------------------------*/
/*VARARGS1*/
void
q_status_message7(flags, min_t, max_t, s, a1, a2, a3, a4, a5, a6, a7)
    int   flags;
    int   min_t, max_t;
    char *s;
    void *a1, *a2, *a3, *a4, *a5, *a6, *a7;
{
    char buf[1000];

    sprintf(buf, s, a1, a2, a3, a4, a5, a6, a7);
    q_status_message(flags, min_t, max_t, buf);
}


/*VARARGS1*/
void
q_status_message8(flags, min_t, max_t, s, a1, a2, a3, a4, a5, a6, a7, a8)
    int   flags;
    int   min_t, max_t;
    char *s;
    void *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8;
{
    char buf[1000];

    sprintf(buf, s, a1, a2, a3, a4, a5, a6, a7, a8);
    q_status_message(flags, min_t, max_t, buf);
}


/*----------------------------------------------------------------------
     Mark the status line as dirty so it gets cleared next chance
 ----*/
void
mark_status_dirty()
{
    mark_status_unknown();
    needs_clearing++;
}


/*----------------------------------------------------------------------
    Cause status line drawing optimization to be turned off, because we
    don't know what the status line looks like.
 ----*/
void
mark_status_unknown()
{
    prevstartcol = -1;
    prevstatusbuff[0]  = '\0';
}



/*----------------------------------------------------------------------
     Wait a suitable amount of time for the currently displayed message
 ----*/
void
pause_for_current_message()
{
    if(message_queue){
	int w;

	if(w = status_message_remaining()){
	    delay_cmd_cue(1);
	    sleep(w);
	    delay_cmd_cue(0);
	}

	d_q_status_message();
    }
}


/*----------------------------------------------------------------------
    Time remaining for current message's minimum display
 ----*/
int
status_message_remaining()
{
    if(message_queue){
	int d = (int)(displayed_time - time(0))
					  + message_queue->min_display_time;
	return((d > 0) ? d : 0);
    }

    return(0);
}


/*----------------------------------------------------------------------
        Find out how many messages are queued for display

  Args:   dtime -- will get set to minimum display time for current message

  Result: number of messages in the queue.

  ---------*/
int
messages_queued(dtime)
    long *dtime;
{
    if(message_queue && dtime)
      *dtime = (long)max(message_queue->min_display_time, 1L);

    return((ps_global->in_init_seq) ? 0 : messages_in_queue());
}



/*----------------------------------------------------------------------
       Return number of messages in queue
  ---------*/
int
messages_in_queue()
{
    int	   n = message_queue ? 1 : 0;
    SMQ_T *p = message_queue;

    while(n && (p = p->next) != message_queue)
      n++;

    return(n);
}



/*----------------------------------------------------------------------
     Return last message queued
  ---------*/
char *
last_message_queued()
{
    SMQ_T *p, *r = NULL;

    if(p = message_queue){
	do
	  if(p->flags & SM_ORDER)
	    r = p;
	while((p = p->next) != message_queue);
    }

    return(r ? r->text : NULL);
}



/*----------------------------------------------------------------------
       Update status line, clearing or displaying a message

   Arg: command -- The command that is about to be executed

  Result: status line cleared or
             next message queued is displayed or
             current message is redisplayed.
	     if next message displayed, it's min display time
	     is returned else if message already displayed, it's
	     time remaining on the display is returned, else 0.

   This is called when ready to display the next message, usually just
before reading the next command from the user. We pass in the nature
of the command because it affects what we do here. If the command just
executed by the user is a redraw screen, we don't want to reset or go to 
next message because it might not have been seen.  Also if the command
is just a noop, which are usually executed when checking for new mail 
and happen every few minutes, we don't clear the message.

If it was really a command and there's nothing more to show, then we
clear, because we know the user has seen the message. In this case the
user might be typing commands very quickly and miss a message, so
there is a time stamp and time check that each message has been on the
screen for a few seconds.  If it hasn't we just return and let it be
taken care of next time.

At slow terminal output speeds all of this can be for naught, the amount
of time it takes to paint the screen when the whole screen is being painted
is greater than the second or two delay so the time stamps set here have
nothing to do with when the user actually sees the message.
----------------------------------------------------------------------*/
int
display_message(command)
    int command;
{
    if(ps_global == NULL || ps_global->ttyo == NULL
       || ps_global->ttyo->screen_rows <= 1 || ps_global->in_init_seq)
      return(0);

    /*---- Deal with any previously displayed messages ----*/
    if(message_queue && message_queue->shown) {
	int rv = -1;

	if(command == ctrl('L')) {	/* just repaint it, and go on */
	    mark_status_unknown();
	    mark_keymenu_dirty();
	    mark_titlebar_dirty();
	    rv = 0;
	}
	else {				/* ensure sufficient time's passed */
	    time_t now;
	    int    diff;

	    now  = time(0);
	    diff = (int)(displayed_time - now)
			+ ((command == NO_OP_COMMAND || command == NO_OP_IDLE)
			    ? message_queue->max_display_time
			    : message_queue->min_display_time);
            dprint(9, (debugfile,
		       "STATUS: diff:%d, displayed: %ld, now: %ld\n",
		       diff, displayed_time, now));
            if(diff > 0)
	      rv = diff;			/* check again next time  */
	    else if(LAST_MESSAGE(message_queue)
		    && (command == NO_OP_COMMAND || command == NO_OP_IDLE)
		    && message_queue->max_display_time)
	      rv = 0;				/* last msg, no cmd, has max */
	}

	if(rv >= 0){				/* leave message displayed? */
	    if(prevstartcol < 0)		/* need to redisplay it? */
	      output_message(message_queue);

	    return(rv);
	}
	  
	d_q_status_message();			/* remove it from queue and */
	needs_clearing++;			/* clear the line if needed */
    }

    if(!message_queue && (command == ctrl('L') || needs_clearing)) {
	dprint(9, (debugfile, "Clearing status line\n"));
	ClearLine(ps_global->ttyo->screen_rows - FOOTER_ROWS(ps_global));
	mark_status_unknown();
	if(command == ctrl('L')){
	    mark_keymenu_dirty();
	    mark_titlebar_dirty();
	}
    }

    /*---- Display any queued messages, weeding 0 disp times ----*/
    while(message_queue && !message_queue->shown)
      if(message_queue->min_display_time || LAST_MESSAGE(message_queue)){
	  displayed_time = time(0);
	  output_message(message_queue);
      }
      else
	d_q_status_message();

    needs_clearing = 0;				/* always cleared or written */
    dprint(9, (debugfile,
               "STATUS cmd:%d, max:%d, min%d\n", command, 
	       (message_queue) ? message_queue->max_display_time : -1,
	       (message_queue) ? message_queue->min_display_time : -1));
    fflush(stdout);
    return(0);
}



/*----------------------------------------------------------------------
     Display all the messages on the queue as quickly as possible
  ----*/
void
flush_status_messages(skip_last_pause)
    int skip_last_pause;
{
    while(message_queue){
	if(LAST_MESSAGE(message_queue)
	   && skip_last_pause
	   && message_queue->shown)
	  break;

	if(message_queue->shown)
	  pause_for_current_message();

	while(message_queue && !message_queue->shown)
	  if(message_queue->min_display_time
	     || LAST_MESSAGE(message_queue)){
	      displayed_time = time(0);
	      output_message(message_queue);
	  }
	  else
	    d_q_status_message();
    }
}



/*----------------------------------------------------------------------
     Make sure any and all SM_ORDER messages get displayed.

     Note: This flags the message line as having nothing displayed.
           The idea is that it's a function called by routines that want
	   the message line for a prompt or something, and that they're
	   going to obliterate the message anyway.
 ----*/
void
flush_ordered_messages()
{
    SMQ_T *start = NULL;

    while(message_queue && message_queue != start){
	if(message_queue->shown)
	  pause_for_current_message(); /* changes "message_queue" */

	while(message_queue && !message_queue->shown
	      && message_queue != start)
	  if(message_queue->flags & SM_ORDER){
	      if(message_queue->min_display_time){
		  displayed_time = time(0);
		  output_message(message_queue);
	      }
	      else
		d_q_status_message();
	  }
	  else if(!start)
	    start = message_queue;
    }
}


     
/*----------------------------------------------------------------------
      Remove a message from the message queue.
  ----*/
void
d_q_status_message()
{
    if(message_queue){
	dprint(9, (debugfile, "d_q_status_message(%.40s)\n",
		   message_queue->text));
	if(!LAST_MESSAGE(message_queue)){
	    SMQ_T *p = message_queue;
	    p->next->prev = p->prev;
	    message_queue = p->prev->next = p->next;
	    fs_give((void **)&p);
	}
	else
	  fs_give((void **)&message_queue);
    }
}



/*----------------------------------------------------------------------
    Actually output the message to the screen

  Args: message            -- The message to output
	from_alarm_handler -- Called from alarm signal handler.
			      We don't want to add this message to the review
			      message list since we may mess with the malloc
			      arena here, and the interrupt may be from
			      the middle of something malloc'ing.
 ----*/
int
status_message_write(message, from_alarm_handler)
    char *message;
    int   from_alarm_handler;
{
    int  col, row, max_length, invert;
    char obuff[MAX_SCREEN_COLS + 1];

    if(!from_alarm_handler)
      add_review_message(message);

    invert = !InverseState();	/* already in inverse? */
    row = max(0, ps_global->ttyo->screen_rows - FOOTER_ROWS(ps_global));

    /* Put [] around message and truncate to screen width */
    max_length = ps_global->ttyo != NULL ? ps_global->ttyo->screen_cols : 80;
    max_length = min(max_length, MAX_SCREEN_COLS);
    obuff[0] = '[';
    obuff[1] = '\0';
    strncat(obuff, message, max_length - 2);
    obuff[max_length - 1] = '\0';
    strcat(obuff, "]");

    if(prevstartcol == -1 || strcmp(obuff, prevstatusbuff)){
	/*
	 * Try to optimize drawing in this case.  If the length of the string
	 * changed, it is very likely a lot different, so probably not
	 * worth looking at.  Just go down the two strings drawing the
	 * characters that have changed.
	 */
	if(prevstartcol != -1 && strlen(obuff) == strlen(prevstatusbuff)){
	    char *p, *q, *uneq_str;
	    int   column, start_col;

	    if(invert)
	      StartInverse();

	    q = prevstatusbuff;
	    p = obuff;
	    col = column = prevstartcol;

	    while(*q){
		/* skip over string of equal characters */
		while(*q && *p == *q){
		    q++;
		    p++;
		    column++;
		}

		if(!*q)
		  break;

		uneq_str  = p;
		start_col = column;

		/* find end of string of unequal characters */
		while(*q && *p != *q){
		    *q++ = *p++;  /* update prevstatusbuff */
		    column++;
		}

		/* tie off and draw the changed chars */
		*p = '\0';
		PutLine0(row, start_col, uneq_str);

		if(*q){
		    p++;
		    q++;
		    column++;
		}
	    }

	    if(invert)
	      EndInverse();

	    /* move cursor to a consistent position */
	    MoveCursor(row, 0);
	    fflush(stdout);
	}
	else{
	    ClearLine(row);
	    if(invert)
	      StartInverse();

	    col = Centerline(row, obuff);
	    if(invert)
	      EndInverse();

	    MoveCursor(row, 0);
	    fflush(stdout);
	    strcpy(prevstatusbuff, obuff);
	    prevstartcol = col;
	}
    }
    else
      col = prevstartcol;

    return(col);
}



/*----------------------------------------------------------------------
    Write the given status message to the display.

  Args: mq_entry -- pointer to message queue entry to write.
 
 ----*/
int 
output_message(mq_entry)
    SMQ_T *mq_entry;
{
    int rv;

    dprint(9, (debugfile, "output_message(%s)\n", mq_entry->text));

    if((mq_entry->flags & SM_DING) && F_OFF(F_QUELL_BEEPS, ps_global))
      Writechar(BELL, 0);			/* ring bell */
      /* flush() handled below */

    rv = status_message_write(mq_entry->text, 0);

    /*
     * If we're a modal message, loop waiting for acknowlegment
     * *and* keep the stream alive.
     */
    if((mq_entry->flags & SM_MODAL) && !mq_entry->shown){
	static char *modal_msg =
	     "* * * Read message above then hit Return to continue Pine * * *";
	int ch;

	ClearLine(ps_global->ttyo->screen_rows-2);
	MoveCursor(ps_global->ttyo->screen_rows-2,
		   max((ps_global->ttyo->screen_rows-sizeof(modal_msg))/2, 0));
	Write_to_screen(modal_msg);
	ClearLine(ps_global->ttyo->screen_rows-1);
	mark_status_dirty();
	mark_keymenu_dirty();

	while(1){
	    flush_input();
	    MoveCursor(ps_global->ttyo->screen_rows-FOOTER_ROWS(ps_global), 0);
	    if((ch = read_char(600)) == NO_OP_COMMAND || ch == NO_OP_IDLE)
	      new_mail(0, 2, 0);		/* wake up and prod server */
	    else if(ch != ctrl('M'))
	      Writechar(BELL, 0);		/* complain */
	    else
	      break;				/* done. */
	}

	if(FOOTER_ROWS(ps_global) != 1){
	    ClearLine(ps_global->ttyo->screen_rows-FOOTER_ROWS(ps_global));
	    if(!ps_global->mangled_footer)
	      redraw_keymenu();			/* else repaint next pass... */
	}
	else if(ps_global->redrawer)
	  (*ps_global->redrawer)();
    }
    else if(!(mq_entry->flags & SM_MODAL) && ps_global->status_msg_delay){
	MoveCursor(ps_global->ttyo->screen_rows-FOOTER_ROWS(ps_global), 0);
	fflush(stdout);
	sleep(ps_global->status_msg_delay);
    }

    mq_entry->shown = 1;
    return(rv);
}



/*----------------------------------------------------------------------
    Write or clear delay cue

  Args: on -- whether to turn it on or not
 
 ----*/
void
delay_cmd_cue(on)
    int on;
{
    int l;

    if(prevstartcol >= 0 && (l = strlen(prevstatusbuff))){
	MoveCursor(ps_global->ttyo->screen_rows - FOOTER_ROWS(ps_global),
		   prevstartcol ? max(prevstartcol - 1, 0) : 0);
	StartInverse();
	Write_to_screen(on ? "[>" : " [");
	EndInverse();
	MoveCursor(ps_global->ttyo->screen_rows - FOOTER_ROWS(ps_global),
		   min(prevstartcol + l, ps_global->ttyo->screen_cols) - 1);
	StartInverse();
	Write_to_screen(on ? "<]" : "] ");
	EndInverse();
	MoveCursor(ps_global->ttyo->screen_rows - FOOTER_ROWS(ps_global), 0);
    }

    fflush(stdout);
#ifdef	_WINDOWS
    mswin_setcursor ((on) ? MSWIN_CURSOR_BUSY : MSWIN_CURSOR_ARROW);
#endif
}



/*
 * want_to's array passed to radio_buttions...
 */
static ESCKEY_S yorn[] = {
    {'y', 'y', "Y", "Yes"},
    {'n', 'n', "N", "No"},
    {-1, 0, NULL, NULL}
};


/*----------------------------------------------------------------------
     Ask a yes/no question in the status line

   Args: question     -- string to prompt user with
         dflt         -- The default answer to the question (should probably
			 be y or n)
         on_ctrl_C    -- Answer returned on ^C
	 help         -- Two line help text
	 display_help -- If true, display help without being asked

 Result: Messes up the status line,
         returns y, n, dflt, or dflt_C
  ---*/
int
want_to(question, dflt, on_ctrl_C, help, display_help, flush)
    char	*question;
    HelpType	help;
    int		dflt, on_ctrl_C, display_help, flush;
{
    char *q2;
    int	  rv;

#ifdef _WINDOWS
    if (mswin_usedialog ()) {
	mswin_flush ();
	switch (mswin_yesno (question)) {
	default:
	case 0:		return (on_ctrl_C);
	case 1:		return ('y');
	case 2:		return ('n');
        }
    }
#endif

    /*----
       One problem with adding the (y/n) here is that shrinking the 
       screen while in radio_buttons() will cause it to get chopped
       off. It would be better to truncate the question passed in
       hear and leave the full "(y/n) [x] : " on.
      ----*/
    q2 = fs_get(strlen(question) + 6);
    sprintf(q2, "%.*s? ", ps_global->ttyo->screen_cols - 6, question);
    if(on_ctrl_C == 'n')	/* don't ever let cancel == 'n' */
      on_ctrl_C = 0;

    rv = radio_buttons(q2,
	(ps_global->ttyo->screen_rows > 4) ? - FOOTER_ROWS(ps_global) : -1,
	yorn, dflt, on_ctrl_C, help, flush ? RB_FLUSH_IN : RB_NORM);
    fs_give((void **)&q2);

    return(rv);
}


int
one_try_want_to(question, dflt, on_ctrl_C, help, display_help, flush)
    char      *question;
    HelpType   help;
    int    dflt, on_ctrl_C, display_help, flush;
{
    char     *q2;
    int	      rv;

    q2 = fs_get(strlen(question) + 6);
    sprintf(q2, "%.*s? ", ps_global->ttyo->screen_cols - 6, question);
    rv = radio_buttons(q2,
	(ps_global->ttyo->screen_rows > 4) ? - FOOTER_ROWS(ps_global) : -1,
	yorn, dflt, on_ctrl_C, help,
        (flush ? RB_FLUSH_IN : RB_NORM) | RB_ONE_TRY);
    fs_give((void **)&q2);

    return(rv);
}



/*----------------------------------------------------------------------
    Prompt user for a choice among alternatives

Args --  prompt:    The prompt for the question/selection
         line:      The line to prompt on, if negative then relative to bottom
         esc_list:  ESC_KEY_S list of keys
         dflt:	    The selection when the <CR> is pressed (should probably
		      be one of the chars in esc_list)
         on_ctrl_C: The selection when ^C is pressed
         help_text: Text to be displayed on bottom two lines
	 flags:     Logically OR'd flags modifying our behavior to:
		RB_FLUSH_IN    - Discard any pending input chars.
		RB_ONE_TRY     - Only give one chance to answer.  Returns
				 on_ctrl_C value if not answered acceptably
				 on first try.
		RB_NO_NEWMAIL  - Quell the usual newmail check.
	
	 Note: If there are enough keys in the esc_list to need a second
	       screen, and there is no help, then the 13th key will be
	       put in the help position.

Result -- Returns the letter pressed. Will be one of the characters in the
          esc_list argument or one of the two deefaults.

This will pause for any new status message to be seen and then prompt the user.
The prompt will be truncated to fit on the screen. Redraw and resize are
handled along with ^Z suspension. Typing ^G will toggle the help text on and
off. Character types that are not buttons will result in a beep (unless one_try
is set).
  ----*/
int
radio_buttons(prompt, line, esc_list, dflt, on_ctrl_C, help_text, flags)
    char     *prompt;
    int	      line;
    ESCKEY_S *esc_list;
    int       dflt;
    int       on_ctrl_C;
    HelpType  help_text;
    int	      flags;
{
    register int     ch, real_line;
    char            *q, *ds = NULL;
    int              cursor_moved, max_label, i, start, fkey_table[12];
    int		     km_popped = 0;
    struct key	     rb_keys[12];
    struct key_menu  rb_keymenu;
    bitmap_t	     bitmap;

#ifdef _WINDOWS
    if (mswin_usedialog ()) {
	MDlgButton		button_list[12];
	int			b;
	int			i;
	int			ret;
	char			**help;

	memset (&button_list, 0, sizeof (MDlgButton) * 12);
	b = 0;
	for (i = 0; esc_list && esc_list[i].ch != -1 && i < 11; ++i) {
	    button_list[b].ch = esc_list[i].ch;
	    button_list[b].rval = esc_list[i].rval;
	    button_list[b].name = esc_list[i].name;
	    button_list[b].label = esc_list[i].label;
	    ++b;
	}
	button_list[b].ch = -1;
	
	help = get_help_text (help_text, NULL);

	ret = mswin_select (prompt, button_list, dflt, on_ctrl_C, 
			help, flags);
	if (help != NULL) 
	    free_help_text (help);
	return (ret);
    }
#endif

    suspend_busy_alarm();
    flush_ordered_messages();		/* show user previous status msgs */
    mark_status_dirty();		/* clear message next display call */
    real_line = line > 0 ? line : ps_global->ttyo->screen_rows + line;
    MoveCursor(real_line, RAD_BUT_COL);
    CleartoEOLN();

    /*---- Find longest label ----*/
    max_label = 0;
    for(i = 0; esc_list && esc_list[i].ch != -1 && i < 12; i++){
      if(esc_list[i].ch == -2) /* -2 means to skip this key and leave blank */
	continue;
      max_label = max(max_label, strlen(esc_list[i].name));
    }

    q = cpystr(prompt); /* So we can truncate string below if need be */
    if(strlen(q) + max_label > ps_global->ttyo->screen_cols) 
        q[ps_global->ttyo->screen_cols - max_label] = '\0';

    /*---- Init structs for keymenu ----*/
    for(i = 0; i < 12; i++)
      memset((void *)&rb_keys[i], 0, sizeof(struct key));

    memset((void *)&rb_keymenu, 0, sizeof(struct key_menu));
    rb_keymenu.how_many = 1;
    rb_keymenu.keys     = rb_keys;

    /*---- Setup key menu ----*/
    start = 0;
    clrbitmap(bitmap);
    memset(fkey_table, NO_OP_COMMAND, 12 * sizeof(int));
    if(help_text != NO_HELP){		/* if shown, always at position 0 */
	rb_keymenu.keys[0].name  = "?";
	rb_keymenu.keys[0].label = "Help";
	setbitn(0, bitmap);
	fkey_table[0] = ctrl('G');
	start++;
    }

    if(on_ctrl_C){			/* if shown, always at position 1 */
	rb_keymenu.keys[1].name  = "^C";
	rb_keymenu.keys[1].label = "Cancel";
	setbitn(1, bitmap);
	fkey_table[1] = ctrl('C');
	start++;
    }

    start = (start) ? 2 : 0;
    /*---- Show the usual possible keys ----*/
    for(i=start; esc_list && esc_list[i-start].ch != -1 && i <= 12; i++){
	/*
	 * If we have an esc_list item we'd like to put in the non-existent
	 * 13th slot, and there is no help, we put it in the help slot
	 * instead.  We're hacking now...!
	 */
	if(i == 12){
	    if(help_text != NO_HELP)
	      panic("Programming botch in radio_buttons(): too many keys");

	    if(esc_list[i-start].ch != -2)
	      setbitn(0, bitmap); /* the help slot */

	    fkey_table[0] = esc_list[i-start].ch;
	    rb_keymenu.keys[0].name  = esc_list[i-start].name;
	    if(esc_list[i-start].rval == dflt){
		ds = (char *)fs_get((strlen(esc_list[i-start].label) + 3)
				    * sizeof(char));
		sprintf(ds, "[%s]", esc_list[i-start].label);
		rb_keymenu.keys[0].label = ds;
	    }
	    else
	      rb_keymenu.keys[0].label = esc_list[i-start].label;
	}
	else{
	    if(esc_list[i-start].ch != -2)
	      setbitn(i, bitmap);

	    fkey_table[i] = esc_list[i-start].ch;
	    rb_keymenu.keys[i].name  = esc_list[i-start].name;
	    if(esc_list[i-start].rval == dflt){
		ds = (char *)fs_get((strlen(esc_list[i-start].label) + 3)
				    * sizeof(char));
		sprintf(ds, "[%s]", esc_list[i-start].label);
		rb_keymenu.keys[i].label = ds;
	    }
	    else
	      rb_keymenu.keys[i].label = esc_list[i-start].label;
	}
    }

    for(; i < 12; i++)
      rb_keymenu.keys[i].name = NULL;

    ps_global->mangled_footer = 1;

    draw_radio_prompt(real_line, RAD_BUT_COL, q);

    while(1){
        fflush(stdout);

	/*---- Paint the keymenu ----*/
	EndInverse();
	draw_keymenu(&rb_keymenu, bitmap, ps_global->ttyo->screen_cols,
		     1 - FOOTER_ROWS(ps_global), 0, FirstMenu, 0);
	StartInverse();
	MoveCursor(real_line, RAD_BUT_COL + strlen(q));

	if(flags & RB_FLUSH_IN)
	  flush_input();

  newcmd:
	/* Timeout 5 min to keep imap mail stream alive */
        ch = read_char(600);
        dprint(2, (debugfile,
                   "Want_to read: %s (%d)\n", pretty_command(ch), ch));
	if(isascii(ch) && isupper((unsigned char)ch))
	  ch = tolower((unsigned char)ch);

	if(F_ON(F_USE_FK,ps_global)
	   && ((isascii(ch) && isalpha((unsigned char)ch) && !strchr("YyNn",ch))
	       || ((ch >= PF1 && ch <= PF12)
		   && (ch = fkey_table[ch - PF1]) == NO_OP_COMMAND))){
	    /*
	     * The funky test above does two things.  It maps
	     * esc_list character commands to function keys, *and* prevents
	     * character commands from input while in function key mode.
	     * NOTE: this breaks if we ever need more than the first
	     * twelve function keys...
	     */
	    if(flags & RB_ONE_TRY){
		ch = on_ctrl_C;
	        goto out_of_loop;
	    }
	    Writechar(BELL, 0);
	    continue;
	}

        switch(ch) {

          default:
	    for(i = 0; esc_list && esc_list[i].ch != -1; i++)
	      if(ch == esc_list[i].ch){
		  int len, n;

		  MoveCursor(real_line, len = RAD_BUT_COL + strlen(q));
		  for(n = 0, len = ps_global->ttyo->screen_cols - len;
		      esc_list[i].label[n] && len > 0;
		      n++, len--)
		    Writechar(esc_list[i].label[n], 0);

		  ch = esc_list[i].rval;
		  goto out_of_loop;
	      }

	    if(flags & RB_ONE_TRY){
		ch = on_ctrl_C;
	        goto out_of_loop;
	    }
	    Writechar(BELL, 0);
	    break;

          case ctrl('M'):
          case ctrl('J'):
            ch = dflt;
            goto out_of_loop;

          case ctrl('C'):
	    if(on_ctrl_C || (flags & RB_ONE_TRY)){
		ch = on_ctrl_C;
		goto out_of_loop;
	    }

	    Writechar(BELL, 0);
	    break;


          case '?':
          case ctrl('G'):
	    if(FOOTER_ROWS(ps_global) == 1 && km_popped == 0){
		km_popped++;
		FOOTER_ROWS(ps_global) = 3;
		line = -3;
		real_line = ps_global->ttyo->screen_rows + line;
		clearfooter(ps_global);
		draw_radio_prompt(real_line, RAD_BUT_COL, q);
		break;
	    }

	    if(help_text != NO_HELP && FOOTER_ROWS(ps_global) > 1){
		mark_keymenu_dirty();
		MoveCursor(real_line + 1, RAD_BUT_COL);
		CleartoEOLN();
		MoveCursor(real_line + 2, RAD_BUT_COL);
		CleartoEOLN();
		radio_help(real_line, RAD_BUT_COL, help_text);
		sleep(5);
		MoveCursor(real_line, RAD_BUT_COL + strlen(q));
	    }
	    else
	      Writechar(BELL, 0);

            break;
            

          case NO_OP_COMMAND:
	    goto newcmd;		/* misunderstood escape? */
	    break;			/* never reached */

          case NO_OP_IDLE:		/* UNODIR, keep the stream alive */
	    if(flags & RB_NO_NEWMAIL)
	      goto newcmd;
	    else if(new_mail(0, 2, 0) < 0)
              break;			/* no changes, get on with life */
            /* Else fall into redraw to adjust displayed numbers and such */


          case KEY_RESIZE:
          case ctrl('L'):
            real_line = line > 0 ? line : ps_global->ttyo->screen_rows + line;
            EndInverse();
            ClearScreen();
            redraw_titlebar();
            if(ps_global->redrawer != NULL)
              (*ps_global->redrawer)();
            redraw_keymenu();
            draw_radio_prompt(real_line, RAD_BUT_COL, q);
            break;

        } /* switch */
    }

  out_of_loop:
    fs_give((void **)&q);
    if(ds)
      fs_give((void **)&ds);

    EndInverse();
    fflush(stdout);
    resume_busy_alarm();
    if(km_popped){
	FOOTER_ROWS(ps_global) = 1;
	clearfooter(ps_global);
	ps_global->mangled_body = 1;
    }

    return(ch);
}


/*----------------------------------------------------------------------

  ----*/
void
radio_help(line, column, help)
     int line, column;
     HelpType help;
{
    char **text;

#if defined(HELPFILE)
    text = get_help_text(help, NULL);
#else
    text = help;
#endif
    if(text == NULL)
      return;
    
    EndInverse();
    MoveCursor(line + 1, column);
    CleartoEOLN();
    if(text[0])
      PutLine0(line + 1, column, text[0]);

    MoveCursor(line + 2, column);
    CleartoEOLN();
    if(text[1])
      PutLine0(line + 2, column, text[1]);

    StartInverse();
#if defined(HELPFILE)
    free_help_text(text);
#endif
    fflush(stdout);
}


/*----------------------------------------------------------------------
   Paint the screen with the radio buttons prompt
  ----*/
void
draw_radio_prompt(l, c, q)
    int       l, c;
    char     *q;
{
    int x;

    StartInverse();
    PutLine0(l, c, q);
    x = c + strlen(q);
    MoveCursor(l, x);
    while(x++ < ps_global->ttyo->screen_cols)
      Writechar(' ', 0);
    MoveCursor(l, c + strlen(q));
    fflush(stdout);
}
