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
    imap.c
    The call back routines for the c-client/imap
       - handles error messages and other notification
       - handles prelimirary notification of new mail and expunged mail
       - prompting for imap server login and password 

 ====*/

#include "headers.h"


/*
 * struct used to keep track of password/host/user triples.
 * The problem is we only want to try user names and passwords if
 * we've already tried talking to this host before.
 * 
 */
typedef struct mmlogin_s {
    char	   *user,
		   *host,
		   *passwd;
    struct mmlogin_s *next;
} MMLOGIN_S;


/*
 * Internal prototypes
 */
int   imap_get_passwd PROTO((MMLOGIN_S *, char *, char *, char *));
void  imap_set_passwd PROTO((MMLOGIN_S **, char *, char *, char *));
#if defined(DOS) || defined(OS2)
char  xlate_in PROTO((int));
char  xlate_out PROTO((char));
char *passfile_name PROTO((char *, char *));
int   read_passfile PROTO((char *, MMLOGIN_S **));
void  write_passfile PROTO((char *, MMLOGIN_S *));
int   get_passfile_passwd PROTO((char *, char *, char *, char *));
void  set_passfile_passwd PROTO((char *, char *, char *, char *));
#endif


/*
 * Exported globals setup by searching functions to tell mm_searched
 * where to put message numbers that matched the search criteria,
 * and to allow mm_searched to return number of matches.
 */
MAILSTREAM *mm_search_stream;
long	    mm_search_count	= 0L;


/*
 * Exported globals used to report folder FIND/LIST responses.
 */
void       *find_folder_list   = NULL;
MAILSTREAM *find_folder_stream = NULL;
long	    find_folder_count  = 0L;
int	    find_folder_inbox    = 0;
#ifdef NEWBB
void	   *newbb_folder_list;
#endif


/*
 * Local global to hook cached list of host/user/passwd triples to.
 */
static	MMLOGIN_S	*mm_login_list = NULL;



/*----------------------------------------------------------------------
      Write imap debugging information into log file

   Args: strings -- the string for the debug file

 Result: message written to the debug log file
  ----*/
void
mm_dlog(string)
    char *string;
{
#ifdef	DEBUG
    time_t      now;
    struct tm  *tm_now;

    now = time((time_t *)0);
    tm_now = localtime(&now);
    dprint(0, (debugfile, "IMAP DEBUG %2.2d:%2.2d:%2.2d %d/%d: %s\n",
	       tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec,
	       tm_now->tm_mon+1, tm_now->tm_mday, string));
#endif
}



/*----------------------------------------------------------------------
      Queue imap log message for display in the message line

   Args: string -- The message 
         errflg -- flag set to 1 if pertains to an error

 Result: Message queued for display

 The c-client/imap reports most of it's status and errors here
  ---*/
void
mm_log(string, errflg)
    char *string;
    long  errflg;
{
    char        message[300];
    char       *occurance;
    int         was_capitalized;
    time_t      now;
    struct tm  *tm_now;

    now = time((time_t *)0);
    tm_now = localtime(&now);

    dprint((errflg == ERROR ? 1 : 2),
	   (debugfile,
	    "IMAP %2.2d:%2.2d:%2.2d %d/%d mm_log %s: %s\n",
	    tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec, tm_now->tm_mon+1,
	    tm_now->tm_mday,
	    (errflg == ERROR)
	      ? "ERROR"
	      : (errflg == WARN)
		  ? "warn"
		  : (errflg == PARSE)
		      ? "parse"
		      : "babble",
	    string));

    if(errflg == ERROR && !strncmp(string, "[TRYCREATE]", 11)){
	ps_global->try_to_create = 1;
	return;
    }
    else if(ps_global->try_to_create
       || (ps_global->dead_stream
	   && (!strncmp(string, "[CLOSED]", 8) || strstr(string, "No-op"))))
      /*
       * Don't display if creating new folder OR
       * warning about a dead stream ...
       */
      return;

    /*---- replace all "mailbox" with "folder" ------*/
    strncpy(message, string, sizeof(message));
    message[sizeof(message) - 1] = '\0';
    occurance = srchstr(message, "mailbox");
    while(occurance) {
	if(!*(occurance+7) || isspace((unsigned char)*(occurance+8))){
	    was_capitalized = isupper((unsigned char)*occurance);
	    rplstr(occurance, 7, (errflg == PARSE ? "address" : "folder"));
	    if(was_capitalized)
	      *occurance = (errflg == PARSE ? 'A' : 'F');
	}
	else
	  occurance += 7;

        occurance = srchstr(occurance, "mailbox");
    }

    if(errflg == PARSE || ps_global->noshow_error) 
      strcpy(ps_global->c_client_error, message);

    if(ps_global->noshow_error ||
		       (ps_global->noshow_warn && errflg == WARN) ||
                       (errflg != ERROR && errflg != WARN))
      return; /* Only care about errors; don't print when asked not to */

    if(errflg == ERROR)
      ps_global->mm_log_error = 1;

    /*---- Display the message ------*/
    q_status_message(SM_ORDER | SM_DING, 3, 5, message);
    strcpy(ps_global->last_error, message);
}



/*----------------------------------------------------------------------
         recieve notification from IMAP

  Args: stream  --  Mail stream message is relavant to 
        string  --  The message text
        errflag --  Set if it is a serious error

 Result: message displayed in status line

 The facility is for general notices, such as connection to server;
 server shutting down etc... It is used infrequently.
  ----------------------------------------------------------------------*/
void
mm_notify(stream, string, errflag)
    MAILSTREAM *stream;
    char       *string;
    long        errflag;
{
    /* be sure to log the message... */
    dprint(1, (debugfile, "IMAP mm_notify %s : %s : %s\n",
               (!errflag) ? "NIL" : 
		 (errflag == ERROR) ? "error" :
		   (errflag == WARN) ? "warning" :
		     (errflag == BYE) ? "bye" : "unknown",
	       (stream && stream->mailbox) ? stream->mailbox : "-no folder-",
	       string));

    sprintf(ps_global->last_error, "%.50s : %.*s",
	    (stream && stream->mailbox) ? stream->mailbox : "-no folder-",
	    min(MAX_SCREEN_COLS, 500-70),
	    string);
    ps_global->last_error[ps_global->ttyo->screen_cols] = '\0';

    /*
     * Then either set special bits in the pine struct or
     * display the message if it's tagged as an "ALERT" or
     * its errflag > NIL (i.e., WARN, or ERROR)
     */
    if(errflag == BYE){
	if(stream == ps_global->mail_stream){
	    if(ps_global->dead_stream)
	      return;
	    else
	      ps_global->dead_stream = 1;
	}
	else if(stream && stream == ps_global->inbox_stream){
	    if(ps_global->dead_inbox)
	      return;
	    else
	      ps_global->dead_inbox = 1;
	}
    }
    else if(!strncmp(string, "[TRYCREATE]", 11))
      ps_global->try_to_create = 1;
    else if(!strncmp(string, "[ALERT]", 7))
      q_status_message2(SM_MODAL, 3, 3, "%s : %s",
			(stream && stream->mailbox)
			  ? stream->mailbox : "-no folder-",
			string + 7);
    else if(!strncmp(string, "[READ-ONLY]", 11))
      q_status_message2(SM_ORDER | SM_DING, 3, 3, "%s : %s",
			(stream && stream->mailbox)
			  ? stream->mailbox : "-no folder-",
			string + 11);
    else if(errflag && errflag != BYE)
      q_status_message(SM_ORDER | ((errflag == ERROR) ? SM_DING : 0),
		       3, 6, ps_global->last_error);
}



/*----------------------------------------------------------------------
       receive notification of new mail from imap daemon

   Args: stream -- The stream the message count report is for.
         number -- The number of messages now in folder.
 
  Result: Sets value in pine state indicating new mailbox size

     Called when the number of messages in the mailbox goes up.  This
 may also be called as a result of an expunge. It increments the
 new_mail_count based on a the difference between the current idea of
 the maximum number of messages and what mm_exists claims. The new mail
 notification is done in newmail.c

 Only worry about the cases when the number grows, as mm_expunged
 handles shrinkage...

 ----*/
void
mm_exists(stream, number)
    MAILSTREAM *stream;
    long number;
{
    long new_this_call;

    dprint(3, (debugfile, "=== mm_exists(%ld,%s) called ===\n", number,
     !stream ? "(no stream)" : !stream->mailbox ? "(null)" : stream->mailbox));

    if(stream == ps_global->mail_stream){
	if(mn_get_total(ps_global->msgmap) != number){
	    ps_global->mail_box_changed = 1;
	    ps_global->mangled_header   = 1;
	}

        if(mn_get_total(ps_global->msgmap) < number){
	    new_this_call = number - mn_get_total(ps_global->msgmap);
	    ps_global->new_mail_count += new_this_call;
	    if(mn_get_sort(ps_global->msgmap) != SortArrival 
	       || mn_get_revsort(ps_global->msgmap))
	      clear_index_cache();

	    mn_add_raw(ps_global->msgmap, new_this_call);
	}
    } else if(stream == ps_global->inbox_stream) {
	if(mn_get_total(ps_global->inbox_msgmap) != number)
	  ps_global->inbox_changed = 1;

        if(mn_get_total(ps_global->inbox_msgmap) < number){
	    new_this_call = number - mn_get_total(ps_global->inbox_msgmap);
	    ps_global->inbox_new_mail_count += new_this_call;
	    mn_add_raw(ps_global->inbox_msgmap, new_this_call);
	}
    } else {
        /*--- ignore mm_exist for other. These are quick opens --*/
    }
}



/*----------------------------------------------------------------------
    Receive notification from IMAP that a message has been expunged

   Args: stream -- The stream/folder the message is expunged from
         number -- The message number that was expunged

mm_expunged is always called on an expunge.  Simply remove all 
reference to the expunged message, shifting internal mappings as
necessary.
  ----*/
void
mm_expunged(stream, number)
    MAILSTREAM *stream;
    long        number;
{
    MSGNO_S *msgs;

    dprint(3, (debugfile, "mm_expunged called %s %ld\n",
     !stream  ? "(no stream)" : !stream->mailbox ? "(null)" : stream->mailbox, 
	       number));

    /*
     * If we ever deal with more than two streams, this'll break
     */
    if(stream == ps_global->mail_stream){
	long i = mn_raw2m(ps_global->msgmap, number);
	if(i){
	    while(i <= mn_get_total(ps_global->msgmap))	/* flush invalid */
	      clear_index_cache_ent(i++);		/* cache entries! */

	    mn_flush_raw(ps_global->msgmap, number);
	    ps_global->mail_box_changed = 1;
	    ps_global->mangled_header   = 1;
	    ps_global->expunge_count++;
	}
    }
    else if(stream == ps_global->inbox_stream){
	long i = mn_raw2m(ps_global->msgmap, number);
	if(i){
	    mn_flush_raw(ps_global->inbox_msgmap, number);
	    ps_global->inbox_changed = 1;
	    ps_global->inbox_expunge_count++;
	}
    }
}



/*---------------------------------------------------------------------- 
        receive notification that search found something           

 Input:  mail stream and message number of located item

 Result: nothing, not used by pine
  ----*/
void
mm_searched(stream, number)
    MAILSTREAM *stream;
    long        number;
{
    if(stream == mm_search_stream)
      mm_search_count++;
}



/*----------------------------------------------------------------------
      Get login and password from user for IMAP login
  
  Args:  host   -- The host name the user is trying to log in to 
         user   -- Buffer to return the user name in 
         passwd -- Buffer to return the passwd in
         trial  -- The trial number or number of attempts to login

 Result: username and password passed back to imap
  ----*/
void
mm_login(host, user, passwd, trial)
    char *host;
    char *user;
    char *passwd;
    long  trial;
{
    static char junk[] = {'?', '\0'};
    char      prompt[80], *p, *named_user = NULL;
    HelpType  help ;
    int       rc, q_line;

    q_line = -FOOTER_ROWS(ps_global);			/* 3 from bottom */

    if(ps_global->anonymous) {
        /*------ Anonymous login mode --------*/
        if(trial >= 1) {
            user[0]   = '\0';
            passwd[0] = '\0';
        } else {
            strcpy(user, "anonymous");
            sprintf(passwd, "%s@%s", ps_global->VAR_USER_ID,
		    ps_global->hostname);
        }
        return;
    }

    /*
     * Initialize user name with either /user= value in the stream
     * being logged into (accessed via the GET_USERNAMEBUF c-client
     * parameter) or the user name we're running under...
     */
    named_user = (char *) mail_parameters(NULL, GET_USERNAMEBUF, NULL);
    if(trial == 0L)
      strcpy(user, (named_user && *named_user)
		     ? named_user : ps_global->VAR_USER_ID);

    /*
     * try last working password associated with this host.
     */
    if(trial == 0L && imap_get_passwd(mm_login_list, passwd, user, host))
      return;

#if defined(DOS) || defined(OS2)
    /* check to see if there's a password left over from last session */
    if(trial == 0L && get_passfile_passwd(ps_global->pinerc,passwd,user,host)){
	imap_set_passwd(&mm_login_list, passwd, user, host);
	return;
    }
#endif

    ps_global->mangled_footer = 1;
    if(!(named_user && *named_user)){
	help = NO_HELP;
	sprintf(prompt, "HOST: %s  ENTER LOGIN NAME: ", host);
	while(1) {
	    rc = optionally_enter(user, q_line, 0, MAILTMPLEN - 1, 1, 0,
				  prompt, NULL, help, 0);
	    if(rc == 3) {
		help = help == NO_HELP ? h_oe_login : NO_HELP;
		continue;
	    }
	    if(rc != 4)
	      break;
	}

	if(rc == 1 || !user[0]) {
	    user[0]   = '\0';
	    passwd[0] = '\0';
	    return;
	}
    }

    help = NO_HELP;
    sprintf(prompt, "HOST: %s  USER: %s  ENTER PASSWORD: ", host, user);
    *passwd = '\0';
    while(1) {
        rc = optionally_enter(passwd, q_line, 0, MAILTMPLEN - 1, 0,
			      1, prompt, NULL, help, 0);
        if(rc == 3) {
            help = help == NO_HELP ? h_oe_passwd : NO_HELP;
            continue;
        }
        if(rc != 4)
          break;
    }

    if(rc == 1 || !passwd[0]) {
	if(!named_user){
	    strcpy(user,junk);
	    strcpy(passwd, junk);
	}
	else{
	    user[0]   = '\0';
	    passwd[0] = '\0';
	}

        return;
    }

    /* remember the password for next time */
    imap_set_passwd(&mm_login_list, passwd, user, host);
#if defined(DOS) || defined(OS2)
    /* if requested, remember it on disk for next session */
    set_passfile_passwd(ps_global->pinerc, passwd, user, host);
#endif
}



/*----------------------------------------------------------------------
       Receive notification of an error writing to disk
      
  Args: stream  -- The stream the error occured on
        errcode -- The system error code (errno)
        serious -- Flag indicating error is serious (mail may be lost)

Result: If error is non serious, the stream is marked as having an error
        and deletes are disallowed until error clears
        If error is serious this goes modal, allowing the user to retry
        or get a shell escape to fix the condition. When the condition is
        serious it means that mail existing in the mailbox will be lost
        if Pine exits without writing, so we try to induce the user to 
        fix the error, go get someone that can fix the error, or whatever
        and don't provide an easy way out.
  ----*/
long
mm_diskerror (stream, errcode, serious)
    MAILSTREAM *stream;
    long        errcode;
    long        serious;
{
    int  i, j;
    char *p, *q, *s;
    static ESCKEY_S de_opts[] = {
	{'r', 'r', "R", "Retry"},
	{'f', 'f', "F", "FileBrowser"},
	{'s', 's', "S", "ShellPrompt"},
	{-1, 0, NULL, NULL}
    };
#define	DE_COLS		(ps_global->ttyo->screen_cols)
#define	DE_LINE		(ps_global->ttyo->screen_rows - 3)

#define	DE_FOLDER(X)	((X) ? (X)->mailbox : "<no folder>")
#define	DE_PMT	\
   "Disk error!  Choose Retry, or the File browser or Shell to clean up: "
#define	DE_STR1		"SERIOUS DISK ERROR WRITING: \"%s\""
#define	DE_STR2	\
   "The reported error number is %s.  The last reported mail error was:"
    static char *de_msg[] = {
	"Please try to correct the error preventing Pine from saving your",
	"mail folder.  For example if the disk is out of space try removing",
	"unneeded files.  You might also contact your system administrator.",
	"",
	"Both Pine's File Browser and an option to enter the system's",
	"command prompt are offered to aid in fixing the problem.  When",
	"you believe the problem is resolved, choose the \"Retry\" option.",
	"Be aware that messages may be lost or this folder left in an",
	"inaccessible condition if you exit or kill Pine before the problem",
	"is resolved.",
	NULL};
    static char *de_shell_msg[] = {
	"\n\nPlease attempt to correct the error preventing saving of the",
	"mail folder.  If you do not know how to correct the problem, contact",
	"your system administrator.  To return to Pine, type \"exit\".",
	NULL};

    dprint(0, (debugfile,
       "\n***** DISK ERROR on stream %s. Error code %ld. Error is %sserious\n",
	       DE_FOLDER(stream), errcode, serious ? "" : "not "));
    dprint(0, (debugfile, "***** message: \"%s\"\n\n", ps_global->last_error));

    if(!serious) {
        if(stream == ps_global->mail_stream) {
            ps_global->io_error_on_stream = 1;
        }

        return (1) ;
    }

    while(1){
	/* replace pine's body display with screen full of explanatory text */
	ClearLine(2);
	PutLine1(2, max((DE_COLS - sizeof(DE_STR1)
					    - strlen(DE_FOLDER(stream)))/2, 0),
		 DE_STR1, DE_FOLDER(stream));
	ClearLine(3);
	PutLine1(3, 4, DE_STR2, long2string(errcode));
	     
	PutLine0(4, 0, "       \"");
	removing_leading_white_space(ps_global->last_error);
	for(i = 4, p = ps_global->last_error; *p && i < DE_LINE; ){
	    for(s = NULL, q = p; *q && q - p < DE_COLS - 16; q++)
	      if(isspace((unsigned char)*q))
		s = q;

	    if(*q && s)
	      q = s;

	    while(p < q)
	      Writechar(*p++, 0);

	    if(*(p = q)){
		ClearLine(++i);
		PutLine0(i, 0, "        ");
		while(*p && isspace((unsigned char)*p))
		  p++;
	    }
	    else{
		Writechar('\"', 0);
		CleartoEOLN();
		break;
	    }
	}

	ClearLine(++i);
	for(j = ++i; i < DE_LINE && de_msg[i-j]; i++){
	    ClearLine(i);
	    PutLine0(i, 0, "  ");
	    Write_to_screen(de_msg[i-j]);
	}

	while(i < DE_LINE)
	  ClearLine(i++);

	switch(radio_buttons(DE_PMT, -FOOTER_ROWS(ps_global), de_opts,
			     'r', 0, NO_HELP, RB_FLUSH_IN | RB_NO_NEWMAIL)){
	  case 'r' :				/* Retry! */
	    ps_global->mangled_screen = 1;
	    return(0L);

	  case 'f' :				/* File Browser */
	    {
		char full_filename[MAXPATH+1], filename[MAXPATH+1];

		filename[0] = '\0';
		build_path(full_filename, ps_global->home_dir, filename);
		file_lister("DISK ERROR", full_filename, filename, FALSE,
			    FB_SAVE);
	    }

	    break;

	  case 's' :
	    EndInverse();
	    end_keyboard(ps_global ? F_ON(F_USE_FK,ps_global) : 0);
	    end_tty_driver(ps_global);
	    for(i = 0; de_shell_msg[i]; i++)
	      puts(de_shell_msg[i]);

	    /*
	     * Don't use our piping mechanism to spawn a subshell here
	     * since it will the server (thus reentering c-client).
	     * Bad thing to do.
	     */
#ifdef	_WINDOWS
#else
	    system("csh");
#endif
	    init_tty_driver(ps_global);
	    init_keyboard(F_ON(F_USE_FK,ps_global));
	    break;
	}

	if(ps_global->redrawer)
	  (*ps_global->redrawer)();
    }
}



/*----------------------------------------------------------------------
      Receive list of folders from c-client/imap.
 
 Puts an entry into the specified list of mail folders associated with
 the global context (aka collection). 
 ----*/
   
void
context_mailbox(name)
    char *name;
{
    dprint(4, (debugfile, "====== context_mailbox: (%s)\n", name));

    /*
     * "Inbox" is filtered out here, since pine only supports one true
     * inbox...
     */
    if((!ps_global->show_dot_names && *name == '.')
       || (!find_folder_inbox && !strucmp(name,"inbox")))
       return;

    find_folder_count++;
    if(find_folder_list){
	FOLDER_S *new_f     = new_folder(name);
	new_f->prefix[0]    = '\0';
	new_f->msg_count    = 0;
	new_f->unread_count = 0;
	folder_insert(-1, new_f, find_folder_list);
    }
}



/*----------------------------------------------------------------------
     Receive list of bulliten boards from c-client/imap

 Puts an entry into the specified list of bulletin boards associated with
 the global context (aka collection). 
 ----*/
void
context_bboard(name)
    char *name;
{
    dprint(4, (debugfile, "====== context_bboard: (%s)\n", name));

#ifdef NEWBB
    /*----

       Hack to handle mm_bboard calls that result from the 
       nntp_find_new_bboards call to find the newly created news groups
     ----*/
    if(find_folder_list == newbb_folder_list) {
        mark_folder_as_new(name);
        return;
    }
#endif

    if(!ps_global->show_dot_names && *name == '.')
       return;

    find_folder_count++;
    if(find_folder_list){
	FOLDER_S *new_f = new_folder(name);
	folder_insert(folder_total(find_folder_list), new_f, find_folder_list);
    }
}


void
mm_fatal(message)
    char *message;
{
    panic(message);
}


void
mm_flags(stream,number)
    MAILSTREAM *stream;
    long number;
{
    long i;

    /*
     * The idea here is to clean up any data pine might have cached
     * that has anything to do with the indicated message number.
     * At the momment, this amounts only to cached index lines, but
     * watch out for future changes...
     */
    if(stream != ps_global->mail_stream)
      return;			/* only worry about displayed msgs */

    /* in case number is current, fix titlebar */
    ps_global->mangled_header = 1;

    /* then clear index entry */
    if(i = mn_raw2m(ps_global->msgmap, number))
      clear_index_cache_ent(i);
}


/*
 * Seconds afterwhich it's ok to prompt for connention severage...
 */
#define	TO_BAIL_THRESHOLD	60L


/*
 * pine_tcptimeout - C-client callback to handle tcp-related timeouts.
 */
long
pine_tcptimeout(t)
    long t;
{
    long rv = 1L;			/* keep trying by default */
    int  ch;

    if(ps_global->noshow_timeout)
      return(rv);

    /*
     * Prompt after a minute (since by then things are probably really bad)
     * A prompt timeout means "keep trying"...
     */
    if(t >= TO_BAIL_THRESHOLD){
	int clear_inverse;

	ClearLine(ps_global->ttyo->screen_rows - FOOTER_ROWS(ps_global));
	if(clear_inverse = !InverseState())
	  StartInverse();

	PutLine1(ps_global->ttyo->screen_rows - FOOTER_ROWS(ps_global), 0,
       "\007Waited %s seconds for server reply.  Break connection to server? ",
	   long2string(t));		/* if not DOS, add fflush() */
	CleartoEOLN();
	fflush(stdout);
	flush_input();
	ch = read_char(7);
	if(ch == 'y' || ch == 'Y')
	  rv = 0L;

	if(clear_inverse)
	  EndInverse();

	ClearLine(ps_global->ttyo->screen_rows - FOOTER_ROWS(ps_global));
    }

    if(rv == 1L){			/* just warn 'em something's up */
	q_status_message1(SM_ORDER, 0, 0,
		  "Waited %s seconds for server reply.  Still Waiting...",
		  long2string(t));
	flush_status_messages(0);	/* make sure it's seen */
    }

    mark_status_dirty();		/* make sure it get's cleared */
    return(rv);
}



/*----------------------------------------------------------------------
   Exported method to retrieve logged in user name associated with stream

   Args: host -- host to find associated login name with.

 Result: 
  ----*/
char *
cached_user_name(host)
    char *host;
{
    MMLOGIN_S *l;

    if((l = mm_login_list) && host)
      do
	if(!strucmp(host, l->host))
	  return(l->user);
      while(l = l->next);

    return(NULL);
}


int
imap_get_passwd(l, passwd, user, host)
    MMLOGIN_S *l;
    char *passwd, *user, *host;
{
    while(l){
	/* host name and user name must match */
	if(!strucmp(host, l->host) && (!*user || !strcmp(user, l->user))){
	    strcpy(user, l->user);
	    strcpy(passwd, l->passwd);
	    break;
	}
	else
	  l = l->next;
    }

    return(l != NULL);
}



void
imap_set_passwd(l, passwd, user, host)
    MMLOGIN_S **l;
    char *passwd, *user, *host;
{
    while(*l){
	if(!strucmp(host, (*l)->host) && !strcmp(user, (*l)->user)){
	    if(strcmp(passwd, (*l)->passwd)){
		fs_give((void **)&(*l)->passwd);
		break;
	    }
	    else
	      return;
	}
	else
	  l = &(*l)->next;
    }

    if(!*l){
	*l = (MMLOGIN_S *)fs_get(sizeof(MMLOGIN_S));
	memset(*l, 0, sizeof(MMLOGIN_S));
    }

    (*l)->passwd = cpystr(passwd);

    if(!(*l)->user)
      (*l)->user = cpystr(user);

    if(!(*l)->host)
      (*l)->host = cpystr(host);
}



void
imap_flush_passwd_cache()
{
    MMLOGIN_S *l;
    while(l = mm_login_list){
	mm_login_list = mm_login_list->next;
	if(l->user)
	  fs_give((void **)&l->user);

	if(l->host)
	  fs_give((void **)&l->host);

	if(l->passwd)
	  fs_give((void **)&l->passwd);

	fs_give((void **)&l);
    }
}


/* 
 * DOS-specific functions to support caching username/passwd/host
 * triples on disk for use from one session to the next...
 */
#if defined(DOS) || defined(OS2)

#define	FIRSTCH		0x20
#define	LASTCH		0x7e
#define	TABSZ		(LASTCH - FIRSTCH + 1)

static	int		xlate_key;
static	MMLOGIN_S	*passfile_cache = NULL;



/*
 * xlate_in() - xlate_in the given character
 */
char
xlate_in(c)
    int	c;
{
    register int  eti;

    eti = xlate_key;
    if((c >= FIRSTCH) && (c <= LASTCH)){
        eti += (c - FIRSTCH);
	eti -= (eti >= 2*TABSZ) ? 2*TABSZ : (eti >= TABSZ) ? TABSZ : 0;
        return((xlate_key = eti) + FIRSTCH);
    }
    else
      return(c);
}



/*
 * xlate_out() - xlate_out the given character
 */
char
xlate_out(c)
    char c;
{
    register int  dti;
    register int  xch;

    if((c >= FIRSTCH) && (c <= LASTCH)){
        xch  = c - (dti = xlate_key);
	xch += (xch < FIRSTCH-TABSZ) ? 2*TABSZ : (xch < FIRSTCH) ? TABSZ : 0;
        dti  = (xch - FIRSTCH) + dti;
	dti -= (dti >= 2*TABSZ) ? 2*TABSZ : (dti >= TABSZ) ? TABSZ : 0;
        xlate_key = dti;
        return(xch);
    }
    else
      return(c);
}



char *
passfile_name(pinerc, path)
    char *pinerc, *path;
{
    char *p = NULL;
    int   i;
    FILE *fp;
#define	PASSFILE	"pine.pwd"

    if(!path || !pinerc || pinerc[0] == '\0')
      return(NULL);

    if((p = strrchr(pinerc, '\\')) == NULL)
      p = strchr(pinerc, ':');

    if(p){
	strncpy(path, pinerc, i = (p - pinerc) + 1);
	path[i] = '\0';
    }

    strcat(path, PASSFILE);
    return(path);
}



int
read_passfile(pinerc, l)
    char       *pinerc;
    MMLOGIN_S **l;
{
    char  tmp[MAILTMPLEN], *ui[3];
    int   i, j, n;
    FILE *fp;

    /* if there's no password to read, bag it!! */
    if(!passfile_name(pinerc, tmp) || !(fp = fopen(tmp, "r")))
      return(0);

    for(n = 0; fgets(tmp, MAILTMPLEN, fp); n++){
	/*** do any necessary DEcryption here ***/
	xlate_key = n;
	for(i = 0; tmp[i]; i++)
	  tmp[i] = xlate_out(tmp[i]);

	if(i && tmp[i-1] == '\n')
	  tmp[i-1] = '\0';			/* blast '\n' */

	ui[0] = ui[1] = ui[2] = NULL;
	for(i = 0, j = 0; tmp[i] && j < 3; j++){
	    for(ui[j] = &tmp[i]; tmp[i] && tmp[i] != '\t'; i++)
	      ;					/* find end of data */

	    if(tmp[i])
	      tmp[i++] = '\0';			/* tie off data */
	}

	if(ui[0] && ui[1] && ui[2])		/* valid field? */
	  imap_set_passwd(l, ui[0], ui[1], ui[2]);
    }

    fclose(fp);
    return(1);
}



void
write_passfile(pinerc, l)
    char      *pinerc;
    MMLOGIN_S *l;
{
    char  tmp[MAILTMPLEN];
    int   i, n;
    FILE *fp;

    /* if there's no password to read, bag it!! */
    if(!passfile_name(pinerc, tmp) || !(fp = fopen(tmp, "w")))
      return;

    for(n = 0; l; l = l->next, n++){
	/*** do any necessary ENcryption here ***/
	sprintf(tmp, "%s\t%s\t%s\n", l->passwd, l->user, l->host);
	xlate_key = n;
	for(i = 0; tmp[i]; i++)
	  tmp[i] = xlate_in(tmp[i]);

	fputs(tmp, fp);
    }

    fclose(fp);
}



/*
 * get_passfile_passwd - return the password contained in the special passord
 *            cache.  The file is assumed to be in the same directory
 *            as the pinerc with the name defined above.
 */
int
get_passfile_passwd(pinerc, passwd, user, host)
    char *pinerc, *passwd, *user, *host;
{
    return((passfile_cache || read_passfile(pinerc, &passfile_cache))
	     ? imap_get_passwd(passfile_cache, passwd, user, host) : 0);
}



/*
 * set_passfile_passwd - set the password file entry associated with
 *            cache.  The file is assumed to be in the same directory
 *            as the pinerc with the name defined above.
 */
void
set_passfile_passwd(pinerc, passwd, user, host)
    char *pinerc, *passwd, *user, *host;
{
    if((passfile_cache || read_passfile(pinerc, &passfile_cache))
       && want_to("Preserve password on DISK for next login", 'y', 'x',
		  NO_HELP, 0, 0) == 'y'){
	imap_set_passwd(&passfile_cache, passwd, user, host);
	write_passfile(pinerc, passfile_cache);
    }
}
#endif	/* DOS */
