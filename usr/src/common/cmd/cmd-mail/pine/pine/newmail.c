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
     newmail.c

   Check for new mail and queue up notification

 ====*/

#include "headers.h"


/*
 * Internal prototypes
 */
void new_mail_mess PROTO((MAILSTREAM *, char *, long, long, int));
void fixup_flags PROTO((MAILSTREAM *, MSGNO_S *, long));


static long mailbox_mail_since_command = 0L,
	    inbox_mail_since_command   = 0L;

/*----------------------------------------------------------------------
     new_mail() - check for new mail in the inbox
 
  Input:  force       -- flag indicating we should check for new mail no matter
          time_for_check_point -- 0: GoodTime, 1: BadTime, 2: VeryBadTime
	  do_status_message -- flag telling us to q a new mail status message

  Result: returns -1 if there was no new mail. Otherwise returns the
          sorted message number of the smallest message number that
          was changed. That is the screen needs to be repainted from
          that message down.

  Limit frequency of checks because checks use some resources. That is
  they generate an IMAP packet or require locking the local mailbox.
  (Acutally the lock isn't required, a stat will do, but the current
   c-client mail code locks before it stats.)

  Returns >= 0 only if there is a change in the given mail stream. Otherwise
  this returns -1.  On return the message counts in the pine
  structure are updated to reflect the current number of messages including
  any new mail and any expunging.
 
 --- */
long
new_mail(force, time_for_check_point, do_status_msg)
    int force, time_for_check_point, do_status_msg;
{
    static time_t last_check = 0;
    static time_t last_check_point_call = 0;
    time_t        now;
    long          rv;
    MAILSTREAM   *stream;
    register struct pine *pine_state;
    int           checknow = 0;
    int           act_on_deferred = 0;
    int           deferring_sort;

    dprint(9, (debugfile, "new mail called (%d %d %d)\n",
               force, time_for_check_point, do_status_msg));
    pine_state = ps_global;  /*  this gets called out of the composer which
                              *  doesn't know about pine_state
                              */
    now = time(0);

    deferring_sort = pine_state->sort_is_deferred;	/* for this time */
    pine_state->sort_is_deferred = 0;			/* for next time */
    if(!deferring_sort && pine_state->unsorted_newmail)
      act_on_deferred++;

    /*
     * only check every 15 seconds, unless we're compelled to
     */
    if(!(stream = pine_state->mail_stream)
       || (timeout == 0 && !force)
       || (now-last_check_point_call <= 15 && time_for_check_point != 0
	   && !pine_state->mail_box_changed && !force
	   && !act_on_deferred))
      return(-1);
    else if(force || now-last_check >= timeout-2){ /* 2: check each timeout */
	checknow++;
        last_check = now;
    }

    last_check_point_call = now;

    if(!check_point(time_for_check_point == 0 ? GoodTime:
	      time_for_check_point == 1 ? BadTime : VeryBadTime) && checknow){
	if(F_ON(F_SHOW_DELAY_CUE, ps_global) && !ps_global->in_init_seq){
	    StartInverse();
	    PutLine0(0, 1, "*");	/* Show something to indicate delay */
	    MoveCursor(ps_global->ttyo->screen_rows -FOOTER_ROWS(ps_global),0);
	    fflush(stdout);
	}

#ifdef	DEBUG
	now = time(0);
	dprint(7, (debugfile, "Mail_Ping(mail_stream): %s\n", ctime(&now)));
#endif
        /*-- Ping the mailstream to check for new mail --*/
        dprint(6, (debugfile, "New mail checked \n"));
	if((char *)mail_ping(stream) == NULL)
	  pine_state->dead_stream = 1;

#ifdef	DEBUG
	now = time(0);
	dprint(7, (debugfile, "Ping complete: %s\n", ctime(&now)));
#endif
	if(F_ON(F_SHOW_DELAY_CUE, ps_global) && !ps_global->in_init_seq){
	    PutLine0(0, 1, " ");
	    EndInverse();
	    fflush(stdout);
	}
    }

    if(checknow && pine_state->inbox_stream 
       && stream != pine_state->inbox_stream){
	if(F_ON(F_SHOW_DELAY_CUE, ps_global)){
	    StartInverse();
	    PutLine0(0, 1, "*"); 	/* Show something to indicate delay */
	    MoveCursor(ps_global->ttyo->screen_rows -FOOTER_ROWS(ps_global),0);
	    fflush(stdout);
	}

#ifdef	DEBUG
	now = time(0);
	dprint(7, (debugfile, "Mail_Ping(inbox_stream): %s\n", ctime(&now)));
#endif
	if((char *)mail_ping(pine_state->inbox_stream) == NULL)
	  pine_state->dead_inbox = 1;

#ifdef	DEBUG
	now = time(0);
	dprint(7, (debugfile, "Ping complete: %s\n", ctime(&now)));
#endif
	if(F_ON(F_SHOW_DELAY_CUE, ps_global)){
	    PutLine0(0, 1, " ");
	    EndInverse();
	    fflush(stdout);
	}
    }


    /*-------------------------------------------------------------
       Mail box state changed, could be additions or deletions.
      -------------------------------------------------------------*/
    if(pine_state->mail_box_changed || act_on_deferred) {
        dprint(7, (debugfile,
        "New mail, %s,  new_mail_count:%d  expunge count:%d,  max_msgno:%d\n",
                   pine_state->mail_stream == pine_state->inbox_stream ?
                      "inbox" : "other",
                   pine_state->new_mail_count,
                   pine_state->expunge_count,
                   mn_get_total(pine_state->msgmap)));

	if(pine_state->mail_box_changed)
	  fixup_flags(pine_state->mail_stream, pine_state->msgmap,
		    pine_state->new_mail_count);

	/*
	 * Lastly, worry about sorting if we got something new, otherwise
	 * it was taken care of inside mm_expunge...
	 */
	if((pine_state->new_mail_count > 0L || pine_state->unsorted_newmail)
	   && !deferring_sort
	   && !any_lflagged(pine_state->msgmap, MN_HIDE)){
	    /*
	     * When new mail arrived, mm_exists cleared the cache.
	     * However, if we deferred the sort at that time and are
	     * sorting now, we need to clear the cache again.
	     */
	    if(pine_state->unsorted_newmail)
	      clear_index_cache();

	    sort_current_folder(1, mn_get_sort(pine_state->msgmap),
				mn_get_revsort(pine_state->msgmap));
	}
	else if(pine_state->new_mail_count > 0L)
	  pine_state->unsorted_newmail = 1;

        if(ps_global->new_mail_count > 0 || mailbox_mail_since_command) {
            mailbox_mail_since_command += ps_global->new_mail_count;

            new_mail_mess(pine_state->mail_stream,
                      pine_state->mail_stream == pine_state->inbox_stream ?
                      NULL :  pine_state->cur_folder,
                      mailbox_mail_since_command,
		      mn_get_total(pine_state->msgmap),
		      do_status_msg);
        }
    }

    if(pine_state->inbox_changed
       && pine_state->inbox_stream != pine_state->mail_stream) {
        /*--  New mail for the inbox, queue up the notification           --*/
        /*-- If this happens then inbox is not current stream that's open --*/
        dprint(7, (debugfile,
         "New mail, inbox, new_mail_count:%ld expunge: %ld,  max_msgno %ld\n",
                   pine_state->inbox_new_mail_count,
                   pine_state->inbox_expunge_count,
                   mn_get_total(pine_state->inbox_msgmap)));

	fixup_flags(pine_state->inbox_stream, pine_state->inbox_msgmap,
		    pine_state->inbox_new_mail_count);

        if(pine_state->inbox_new_mail_count > 0 || inbox_mail_since_command) {
            inbox_mail_since_command       += ps_global->inbox_new_mail_count;
            ps_global->inbox_new_mail_count = 0;
            new_mail_mess(pine_state->inbox_stream,NULL,
                          inbox_mail_since_command,
			  mn_get_total(pine_state->inbox_msgmap),
			  do_status_msg);
        }
    }

    rv = mailbox_mail_since_command
	 + inbox_mail_since_command + pine_state->expunge_count;
    if(do_status_msg){
	pine_state->inbox_changed    = 0;
	pine_state->mail_box_changed = 0;
    }

    ps_global->new_mail_count    = 0L;

    if(pine_state->shown_newmail
       && !(mailbox_mail_since_command || inbox_mail_since_command)){
	icon_text(NULL);
	pine_state->shown_newmail = 0;
    }

    dprint(6, (debugfile, "******** new mail returning %ld  ********\n", 
	       rv ? rv : -1));
    return(rv ? rv : -1);
}


/*----------------------------------------------------------------------
     Format and queue a "new mail" message

  Args: stream     -- mailstream on which a mail has arrived
        folder     -- Name of folder, NULL if inbox
        number     -- number of new messages since last command
        max_num    -- The number of messages now on stream

 Not too much worry here about the length of the message because the
status_message code will fit what it can on the screen and truncation on
the right is about what we want which is what will happen.
  ----*/
void
new_mail_mess(stream, folder, number, max_num, do_status_msg)
     MAILSTREAM *stream;
     long        number, max_num;
     char       *folder;
     int         do_status_msg;

{
    ENVELOPE	*e;
    char	 subject[200], from[2*MAX_SCREEN_COLS], intro[50 + MAXFOLDER];
    static char *carray[] = { "regarding",
				"concerning",
				"about",
				"as to",
				"as regards",
				"as respects",
				"in re",
				"re",
				"respecting",
				"in point of",
				"with regard to",
				"subject:"
    };

#ifdef _WINDOWS
    mswin_newmailicon ();
#endif

    if(!do_status_msg)
      return;

    if(!folder) {
        if(number > 1)
          sprintf(intro, "%ld new messages!", number);
        else
          strcpy(intro, "New mail!");
    }
    else {
        if(number > 1)
          sprintf(intro,"%ld messages saved to folder \"%s\"", number, folder);
        else
          sprintf(intro, "Mail saved to folder \"%s\"", folder);
    }

    if((e = mail_fetchstructure(stream, max_num, NULL)) && e->from){
	sprintf(from, " %srom ", (number > 1L) ? "Most recent f" : "F");
	if(e->from->personal)
	  istrncpy(from + ((number > 1L) ? 18 : 6),
		   (char *) rfc1522_decode((unsigned char *) tmp_20k_buf,
					   e->from->personal, NULL),
		   ps_global->ttyo->screen_cols);
	else
	  sprintf(from + ((number > 1L) ? 18 : 6), "%s@%s", 
		  e->from->mailbox, e->from->host);
    }
    else
      from[0] = '\0';

    if(number <= 1L) {
        if(e && e->subject){
	    sprintf(subject, " %s ", carray[(unsigned)random()%12]);
	    istrncpy(subject + strlen(subject),
		     (char *) rfc1522_decode((unsigned char *) tmp_20k_buf,
					     e->subject, NULL), 100);
	}
	else
	  strcpy(subject, " with no subject");

        if(!from[0])
          subject[1] = toupper((unsigned char)subject[1]);

        q_status_message3(SM_ASYNC | SM_DING, 0, 60,
			  "%s%s%s", intro, from, subject);
    }
    else
      q_status_message2(SM_ASYNC | SM_DING, 0, 60, "%s%s", intro, from);

    sprintf(tmp_20k_buf, "%s%s", intro, from);
    icon_text(tmp_20k_buf);
    ps_global->shown_newmail = 1;
}



/*----------------------------------------------------------------------
  Straighten out any local flag problems here.  We can't take care of
  them in the mm_exists or mm_expunged callbacks since the flags
  themselves are in an MESSAGECACHE and we're not allowed to reenter
  c-client from a callback...

 Args: stream -- mail stream to operate on
       msgmap -- messages in that stream to fix up
       new_msgs -- number of new messages

 Result: returns with local flags as they should be

  ----*/
void
fixup_flags(stream, msgmap, new_msgs)
    MAILSTREAM *stream;
    MSGNO_S    *msgmap;
    long	new_msgs;
{
    long i;

    if(new_msgs > 0L && any_lflagged(msgmap, MN_HIDE)){
	i = mn_get_total(msgmap) - new_msgs + 1L;
	while(i > 0L && i <= mn_get_total(msgmap))
	  set_lflag(stream, msgmap, i++, MN_HIDE, 1);
    }

    /*
     * Also, worry about the case where expunged away all of the 
     * zoomed messages.  Unhide everything in that case...
     */
    if(mn_get_total(msgmap) > 0L){
	if(any_lflagged(msgmap, MN_HIDE) >= mn_get_total(msgmap)){
	    for(i = 1L; i <= mn_get_total(msgmap); i++)
	      set_lflag(stream, msgmap, i, MN_HIDE, 0);

	    mn_set_cur(msgmap, mn_get_total(msgmap));
	}
	else if(any_lflagged(msgmap, MN_HIDE)){
	    /*
	     * if we got here, there are some hidden messages and
	     * some not.  Make sure the current message is one
	     * that's not...
	     */
	    for(i = mn_get_cur(msgmap); i <= mn_get_total(msgmap); i++)
	      if(!get_lflag(stream, msgmap, i, MN_HIDE)){
		  mn_set_cur(msgmap, i);
		  break;
	      }

	    for(i = mn_get_cur(msgmap); i > 0L; i--)
	      if(!get_lflag(stream, msgmap, i, MN_HIDE)){
		  mn_set_cur(msgmap, i);
		  break;
	      }
	}
    }
}



/*----------------------------------------------------------------------
    Force write of the main file so user doesn't lose too much when
 something bad happens. The only thing that can get lost is flags, such 
 as when new mail arrives, is read, deleted or answered.

 Args: timing      -- indicates if it's a good time for to do a checkpoint

  Result: returns 1 if checkpoint was written, 
                  0 if not.

NOTE: mail_check will also notice new mail arrival, so it's imperative that
code exist after this function is called that can deal with updating the 
various pieces of pine's state associated with the message count and such.

Only need to checkpoint current stream because there are no changes going
on with other streams when we're not manipulating them.
  ----*/
static int check_count		= 0;  /* number of changes since last chk_pt */
static long first_status_change = 0;  /* time of 1st change since last chk_pt*/
static long last_status_change	= 0;  /* time of last change                 */
static long check_count_ave	= 10 * 10;

check_point(timing)
     CheckPointTime timing;
{
    int     freq, tm;
    long    adj_cca;
    long    tmp;
#ifdef	DEBUG
    time_t  now;
#endif

    dprint(9, (debugfile, "check_point(%s)\n", 
               timing == GoodTime ? "GoodTime" :
               timing == BadTime  ? "BadTime" :
               timing == VeryBadTime  ? "VeryBadTime" : "DoItNow"));

    if(!ps_global->mail_stream || ps_global->mail_stream->rdonly ||
							     check_count == 0)
	 return(0);

    freq = CHECK_POINT_FREQ * (timing==GoodTime ? 1 : timing==BadTime ? 3 : 4);
    tm   = CHECK_POINT_TIME * (timing==GoodTime ? 1 : timing==BadTime ? 2 : 3);

    dprint(9, (debugfile, "freq: %d  count: %d\n", freq, check_count));
    dprint(9, (debugfile, "tm: %d  elapsed: %d\n", tm,
               time(0) - first_status_change));

    if(!last_status_change)
        last_status_change = time(0);

    tmp = 10*(time(0)-last_status_change);
    adj_cca = (tmp > check_count_ave) ?
	  min((check_count_ave + tmp)/2, 2*check_count_ave) : check_count_ave;

    dprint(9, (debugfile, "freq %d tm %d changes %d since_1st_change %d\n",
	       freq, tm, check_count, time(0)-first_status_change));
    dprint(9, (debugfile, "since_status_chg %d chk_cnt_ave %d (tenths)\n",
	       time(0)-last_status_change, check_count_ave));
    dprint(9, (debugfile, "adj_chk_cnt_ave %d (tenths)\n", adj_cca));
    dprint(9, (debugfile, "Check:if changes(%d)xadj_cca(%d) >= freq(%d)x200\n",
	       check_count, adj_cca, freq));
    dprint(9, (debugfile, "      is %d >= %d ?\n",
	       check_count*adj_cca, 200*freq));

    /* the 200 comes from 20 seconds for an average status change time
       multiplied by 10 tenths per second */
    if((timing == DoItNow || (check_count * adj_cca >= freq * 200) ||
       (time(0) - first_status_change >= tm))){
#ifdef	DEBUG
	now = time(0);
	dprint(7, (debugfile,
                     "Doing checkpoint: %s  Since 1st status change: %d\n",
                     ctime(&now), now - first_status_change));
#endif
	if(F_ON(F_SHOW_DELAY_CUE, ps_global)){
	    StartInverse();
	    PutLine0(0, 0, "**");	/* Show something indicate delay*/
	    MoveCursor(ps_global->ttyo->screen_rows -FOOTER_ROWS(ps_global),0);
	    fflush(stdout);
	}

        mail_check(ps_global->mail_stream);
					/* Causes mail file to be written */
#ifdef	DEBUG
	now = time(0);
	dprint(7, (debugfile, "Checkpoint complete: %s\n", ctime(&now)));
#endif
        check_count = 0;
        first_status_change = time(0);
	if(F_ON(F_SHOW_DELAY_CUE, ps_global)){
	    PutLine0(0, 0, "  ");
	    EndInverse();
	    fflush(stdout);
	}

        return(1);
    } else {
        return(0);
    }
}



/*----------------------------------------------------------------------
    Call this when we need to tell the check pointing mechanism about
  mailbox state changes.
  ----------------------------------------------------------------------*/
void
check_point_change()
{
    if(!last_status_change)
        last_status_change = time(0) - 10;  /* first change 10 seconds */

    if(!check_count++)
      first_status_change = time(0);
    /*
     * check_count_ave is the exponentially decaying average time between
     * status changes, in tenths of seconds, except never grow by more
     * than double, but always at least one (unless there's a fulll moon).
     */
    check_count_ave = min((check_count_ave +
                max(10*(time(0)-last_status_change),2))/2, 2*check_count_ave);

    last_status_change = time(0);
}



/*----------------------------------------------------------------------
    Call this when a mail file is written to reset timer and counter
  for next check_point.
  ----------------------------------------------------------------------*/
void
reset_check_point()
{
    check_count = 0;
    first_status_change = time(0);
}



/*----------------------------------------------------------------------
    Zero the counters that keep track of mail accumulated between
   commands.
 ----*/
void
zero_new_mail_count()
{
    dprint(9, (debugfile, "New_mail_count zeroed\n"));
    mailbox_mail_since_command = 0L;
    inbox_mail_since_command   = 0L;
}


/*----------------------------------------------------------------------
     Check and see if all the stream are alive

Returns:  0 if there was no change
          1 if streams have died since last call

Also outputs a message that the streams have died
 ----*/
streams_died()
{
    int rv = 0, inbox = 0;

    if(ps_global->dead_stream && !ps_global->noticed_dead_stream) {
        rv = 1;
        ps_global->noticed_dead_stream = 1;
        if(ps_global->mail_stream == ps_global->inbox_stream)
          ps_global->noticed_dead_inbox = 1;
    }

    if(ps_global->dead_inbox && !ps_global->noticed_dead_inbox) {
        rv = 1;
        ps_global->noticed_dead_inbox = 1;
        inbox = 1;
    }
    if(rv == 1) 
      q_status_message1(SM_ORDER | SM_DING, 3, 6,
                        "MAIL FOLDER \"%s\" CLOSED DUE TO ACCESS ERROR",
                        pretty_fn(inbox ? ps_global->inbox_name
				  	: ps_global->cur_folder));
    return(rv);
}
        
