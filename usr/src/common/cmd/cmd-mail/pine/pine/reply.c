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
    reply.c
   
   Code here for forward and reply to mail
   A few support routines as well

  This code will forward and reply to MIME messages. The Pine composer
at this time will only support non-text segments at the end of a
message so, things don't always come out as one would like. If you
always forward a message in MIME format, all will be correct.  Forwarding
of nested MULTIPART messages will work.  There's still a problem with
MULTIPART/ALTERNATIVE as the "first text part" rule doesn't allow modifying
the equivalent parts.  Ideally, we should probably such segments as a 
single attachment when forwarding/replying.  It would also be real nice to
flatten out the nesting in the composer so pieces inside can get snipped.

The evolution continues...

  =====*/


#include "headers.h"


/*
 * Internal Prototypes
 */
void	 reply_forward_header PROTO((MAILSTREAM *, long, ENVELOPE *, gf_io_t,
				     char *));
char	*generate_in_reply_to PROTO((ENVELOPE *));
int	 fetch_contents PROTO((MAILSTREAM *, long, BODY *, BODY *));
ADDRESS	*reply_cp_addr PROTO((struct pine *, long, char *, ADDRESS *,
			      ADDRESS *, ADDRESS *));
void	 reply_fish_personal PROTO((ENVELOPE *, ENVELOPE *));
char	*reply_build_refs PROTO((char *, ENVELOPE *));
int	 addr_in_env PROTO((ADDRESS *, ENVELOPE *));
int	 addr_lists_same PROTO((ADDRESS *, ADDRESS *));
char	*reply_subject PROTO((char *, char *));
int	 reply_poster_followup PROTO((char *));
void	 forward_delimiter PROTO((gf_io_t));
char	*forward_subject PROTO((struct pine *, long));
void	 bounce_mask_header PROTO((char **, char *));



/*
 * Little defs to keep the code a bit neater...
 */
#define	FRM_PMT	"Use \"Reply-To:\" address instead of \"From:\" address"
#define	ALL_PMT		"Reply to all recipients"
#define	NEWS_PMT	"Follow-up to news group(s), Reply via email to author or Both? "

/*
 * standard type of storage object used for body parts...
 */
#if	defined(DOS) && !defined(WIN32)
#define		  PART_SO_TYPE	TmpFileStar
#else
#define		  PART_SO_TYPE	CharStar
#endif



/*----------------------------------------------------------------------
        Fill in an outgoing message for reply and pass off to send

    Args: pine_state -- The usual pine structure

  Result: Reply is formatted and passed off to composer/mailer

Reply

   - put senders address in To field
   - search to and cc fields to see if we aren't the only recipients
   - if other than us, ask if we should reply to all.
   - if answer yes, fill out the To and Cc fields
   - fill in the fcc argument
   - fill in the subject field
   - fill out the body and the attachments
   - pass off to pine_send()
  ---*/
void
reply(pine_state)
     struct pine *pine_state;
{
    ADDRESS   **to_tail, **cc_tail, *atmp,
	       *saved_from, *saved_to, *saved_cc, *saved_resent,
	      **saved_from_tail, **saved_to_tail, **saved_cc_tail,
	      **saved_resent_tail;
    ENVELOPE   *env, *outgoing;
    BODY       *body, *orig_body;
    PART       *part;
    void       *msgtext;
    char       *sig, *tp, *prefix = NULL, *refs = NULL;
    long        msgno, totalm, *seq = NULL;
    int         i, processed_resent, parsed_resent,
    		asked_replyall = 0, replytoall = 0, include_text = 0,
		reply_ret = 0, cc_ret = 0, times = -1, ret, warned = 0;
    gf_io_t     pc;
    BUILDER_ARG fcc;
#if	defined(DOS) && !defined(_WINDOWS)
    char *reserve;
#endif
#define NEWSGRPS   0
#define PATH       1
#define REFS       2
#define	FOLLOWUP   3
#define RESENTFROM 4
#define RESENTTO   5
#define RESENTCC   6
#define NEXTRA     7
    char       *hdrs, *values[NEXTRA+1];
    static char *fields[] = {"Newsgroups", "Path", "References", "Followup-To",
			     "Resent-From", "Resent-To", "Resent-Cc", NULL};
    static ESCKEY_S news_opt[] = { {'f', 'f', "F", "Follow-up"},
				   {'r', 'r', "R", "Reply"},
				   {'b', 'b', "B", "Both"},
				   {-1, 0, NULL, NULL} };

    outgoing = mail_newenvelope();
    fcc.tptr = NULL; fcc.next = NULL; fcc.xtra = NULL;
    totalm   = mn_total_cur(pine_state->msgmap);
    sig      = get_signature();
    seq	     = (long *)fs_get(((size_t)totalm + 1) * sizeof(long));

    dprint(4, (debugfile,"\n - reply (%s msgs) -\n", comatose(totalm)));

    memset(values, 0, (NEXTRA+1)*sizeof(char *));
    saved_from		  = (ADDRESS *)NULL;
    saved_from_tail       = &saved_from;
    saved_to		  = (ADDRESS *)NULL;
    saved_to_tail	  = &saved_to;
    saved_cc		  = (ADDRESS *)NULL;
    saved_cc_tail	  = &saved_cc;
    saved_resent	  = (ADDRESS *)NULL;
    saved_resent_tail	  = &saved_resent;
    outgoing->subject	  = NULL;
    ret			  = 0;

    /*
     * For consistency, the first question is always "include text?"
     */
    if(F_ON(F_AUTO_INCLUDE_IN_REPLY, pine_state)){
	include_text = 1;
    }
    else{
	sprintf(tmp_20k_buf, "Include %s%soriginal message%s in Reply",
		(totalm > 1L) ? comatose(totalm) : "",
		(totalm > 1L) ? " " : "",
		(totalm > 1L) ? "s" : "");

	if((ret = want_to(tmp_20k_buf, 'n', 'x', NO_HELP, 0, 0)) == 'x'){
	    q_status_message(SM_ORDER, 0, 3,"Reply cancelled");
	    goto done_early;
	}
	else
	  include_text = ret == 'y';
    }

    /*
     * We go to considerable trouble to avoid extra RTT's looking for
     * Resent-* headers if we don't have to.  We piggy-back the request
     * with the Newsgroups request if we're already doing that.  We ask
     * the reply-to-all question as early as possible so that we can avoid
     * looking for Resents if user says no.  However, we're also trying
     * to do this in one pass through the messages, so that adds some
     * complexity.  That's why there are three separate places in the
     * code where we might get the resent headers.
     */
    for(msgno = mn_first_cur(pine_state->msgmap);
	msgno > 0L;
	msgno = mn_next_cur(pine_state->msgmap)){

	memset(values, 0, (NEXTRA+1)*sizeof(char *));
	hdrs             = NULL;
	processed_resent = 0;
	parsed_resent    = 0;
	/*--- Grab current envelope ---*/
	env = mail_fetchstructure(pine_state->mail_stream,
			    seq[++times] = mn_m2raw(pine_state->msgmap, msgno),
			    NULL);
	if(!env) {
	    q_status_message1(SM_ORDER,3,4,
			      "Error fetching message %s. Can't reply to it.",
			      long2string(msgno));
	    goto done_early;
	}

	/*
	 * If we're talking on an IMAP stream, get the dang newsgroups
	 * by hand since this field's not supported by the IMAP driver,
	 * otherwise copy the newsgroups if present and fetch the
	 * references (another unsupported yet necessary header).
	 *
	 * However, even if c-client gave us the newsgroup we need to
	 * fetch the References and Followup-To headers since they're
	 * not yet supported either.
	 *
	 * NOTE: Don't bother looking if we don't want to know.  In other
	 * words, only look if it's our first time thru the loop and we're
	 * either replying to a single message or in a newsgroup.  The single
	 * message case get's us around the aggregate reply to messages
	 * in a mixed mail-news archive where some might have newsgroups
	 * and others not or whatever.
	 */
	if(times == 0 && (totalm == 1L || IS_NEWS(pine_state->mail_stream))
	   && ((env->newsgroups && *env->newsgroups)
	       || (pine_state->mail_stream->mailbox[0] == '{'
		   || (pine_state->mail_stream->mailbox[0] == '*'
		       && pine_state->mail_stream->mailbox[1] == '{')))){

	    /*
	     * Since we're already spending an RTT to get News stuff,
	     * get the Resent's that we may need later.
	     */
	    if(hdrs = xmail_fetchheader_lines(pine_state->mail_stream,
					      seq[times], fields)){
		parsed_resent++;
		simple_header_parse(hdrs, fields, values);
	    }

	    /*
	     * If we were given newsgroups OR we hunted (mimicing c-client's
	     * rules for bogosity [See c-client/rfc822.c around line 500])
	     * and found them AND Followup-To isn't "poster", ask what it
	     * is the user wants to do with it...
	     */
	    if((env->newsgroups
		|| (values[NEWSGRPS] && values[NEWSGRPS][0]
		    && (values[PATH] && values[PATH][0]
			|| (env->message_id
			    && !(strncmp(env->message_id,"<Pine.",6)
				 && strncmp (env->message_id,"<MS-C.",6)
				 && strncmp (env->message_id,"<ML-.",4))))))
	       && !(totalm == 1 && reply_poster_followup(values[FOLLOWUP]))){
		/*
		 * Now that we know a newsgroups field is present, 
		 * ask if the user is posting a follow-up article...
		 */
		ret = radio_buttons(NEWS_PMT, -FOOTER_ROWS(pine_state),
				    news_opt, 'r', 'x', NO_HELP, RB_NORM);

		if(ret == 'b' || ret == 'f'){
		    /* make sure we only send to news */
		    if(ret == 'f'){
			mail_free_address(&saved_from);
			mail_free_address(&saved_to);
			mail_free_address(&saved_cc);
			mail_free_address(&saved_resent);
		    }

		    /*
		     * Followup-To takes precedence over Newsgroups, if
		     * we're not doing aggregate reply...
		     */
		    if(totalm == 1L && values[FOLLOWUP]
		       && values[FOLLOWUP][0]){
			q_status_message(SM_ORDER, 2, 3,
				    "Posting to specified Followup-To groups");
			outgoing->newsgroups = values[FOLLOWUP];
			values[FOLLOWUP] = NULL;
			if(values[NEWSGRPS])
			  fs_give((void **)&values[NEWSGRPS]);
		    }
		    else{
			outgoing->newsgroups = env->newsgroups
						 ? cpystr(env->newsgroups)
						 : values[NEWSGRPS];
			values[NEWSGRPS] = NULL;
			if(values[FOLLOWUP])
			  fs_give((void **)&values[FOLLOWUP]);
		    }

		    /* Likewise no refs if aggregate reply! */
		    if(totalm == 1L && env->message_id && *env->message_id)
		      refs = reply_build_refs(values[REFS], env);

		    /*
		     * General Notes:
		     *
		     * 1) Seems unlikey anyone will want to both post
		     *    news and follow up to the author of the posting
		     * 	  assuming the author reads news.  Well maybe not
		     * 	  so unlikely, but potentially five questions
		     * 	  before posting seems excessive:
		     * 	  1. include orig     2. Follow up      3. Also e-mail
		     * 	  4. use reply-to     5. all recipients
		     *
		     * 2) BUG: we need to decide what to do if reply in
		     *    mixed news/mail folder and some message have
		     *    newsgroups and some don't or the ones that do
		     *    don't all match.
		     */

		}
		else if(ret == 'x'){
		    q_status_message(SM_ORDER, 0, 3,"Reply cancelled");
		    goto done_early;
		}
	    }
	}

	if(!outgoing->newsgroups || ret == 'b'){
	    /*
	     * Always use the reply-to if we're replying to more than one 
	     * msg...
	     */
	    if(env->reply_to && !addr_lists_same(env->reply_to, env->from)
	       && (F_ON(F_AUTO_REPLY_TO, pine_state)
		   || totalm > 1L
		   || (reply_ret = want_to(FRM_PMT,'y','x',NO_HELP,0,0))=='y'))
	      *saved_from_tail
		  = reply_cp_addr(pine_state, seq[times], "reply-to",
				  saved_from, (ADDRESS *)NULL,
				  env->reply_to);
	    else
	      *saved_from_tail
		  = reply_cp_addr(pine_state, seq[times], "From",
				  saved_from, (ADDRESS *)NULL,
				  env->from);

	    if(reply_ret == 'x') {
		q_status_message(SM_ORDER, 0, 3, "Reply cancelled");
		goto done_early;
	    }

	    while(*saved_from_tail)		/* stay on last address */
	      saved_from_tail = &(*saved_from_tail)->next;

	    /*--------- check for other recipients ---------*/
	    if(replytoall || !asked_replyall){
		*saved_to_tail = reply_cp_addr(pine_state, seq[times], "To",
					       saved_to, saved_from, env->to);
					       

		while(*saved_to_tail)		/* stay on last address */
		  saved_to_tail = &(*saved_to_tail)->next;
	    }

	    if(replytoall || !asked_replyall){
		*saved_cc_tail = reply_cp_addr(pine_state, seq[times], "Cc",
					       saved_cc,
					       saved_from, env->cc);

		while(*saved_cc_tail)		/* stay on last address */
		  saved_cc_tail = &(*saved_cc_tail)->next;
	    }

	    /*
	     * In these cases, we either need to look at the resent headers
	     * to include in the reply-to-all, or to decide whether or not
	     * we need to ask the reply-to-all question.
	     */
	    if(replytoall
	       || (!asked_replyall
	          && ((!saved_from && !saved_cc)
		     || (saved_from && !saved_to && !saved_cc)))){
		static char *fakedomain = "@";

		/* fetch resent headers */
		if(!hdrs)
		  hdrs = xmail_fetchheader_lines(pine_state->mail_stream,
						 seq[times], fields);
		
		processed_resent++;
		if(hdrs){
		    if(!parsed_resent)
		      simple_header_parse(hdrs, fields, values);

		    parsed_resent++;

		    for(i = RESENTFROM; i <= RESENTCC; i++){
			atmp = NULL;
			if(values[i]){
			    removing_trailing_white_space(values[i]);
			    rfc822_parse_adrlist(&atmp, values[i],
			      (F_ON(F_COMPOSE_REJECTS_UNQUAL, pine_state))
				? fakedomain : pine_state->maildomain);
			    /*
			     * look for bogus addr entries and replace
			     */


			    *saved_resent_tail = reply_cp_addr(pine_state,
							       0, NULL,
					   saved_resent, saved_from, atmp);
			    mail_free_address(&atmp);
			    while(*saved_resent_tail)
			      saved_resent_tail = &(*saved_resent_tail)->next;
			}
		    }
		}
	    }

	    /*
	     * It makes sense to ask reply-to-all now.
	     */
	    if(!asked_replyall
	          && ((saved_from && (saved_to||saved_cc||saved_resent))
		     || (saved_cc || saved_resent))){
		  cc_ret = want_to(ALL_PMT,'n','x',NO_HELP,0,0);
		  asked_replyall++;
		  replytoall = (cc_ret == 'y');
	    }

	    /*
	     * If we just answered yes to the reply-to-all question and
	     * we still haven't collected the resent headers, do so now.
	     */
	    if(replytoall && !processed_resent){
		static char *fakedomain = "@";

		if(!hdrs)
		  hdrs = xmail_fetchheader_lines(pine_state->mail_stream,
						 seq[times], fields);
		processed_resent++;
		if(hdrs){
		    if(!parsed_resent)
		      simple_header_parse(hdrs, fields, values);
		    
		    parsed_resent++;

		    for(i = RESENTFROM; i <= RESENTCC; i++){
			atmp = NULL;
			if(values[i]){
			    removing_trailing_white_space(values[i]);
			    rfc822_parse_adrlist(&atmp, values[i],
			      (F_ON(F_COMPOSE_REJECTS_UNQUAL, pine_state))
				? fakedomain : pine_state->maildomain);

			    /*
			     * look for bogus addr entries and replace
			     */

			    *saved_resent_tail = reply_cp_addr(pine_state,
							       0, NULL,
					   saved_resent, saved_from, atmp);
			    mail_free_address(&atmp);
			    while(*saved_resent_tail)
			      saved_resent_tail = &(*saved_resent_tail)->next;
			}
		    }
		}
	    }

	    if(cc_ret == 'x') {
		q_status_message(SM_ORDER, 0, 3, "Reply cancelled");
		goto done_early;
	    }
	}

	/*------------ Format the subject line ---------------*/
	if(outgoing->subject){
	    /*
	     * if reply to more than one message, and all subjects
	     * match, so be it.  otherwise set it to something generic...
	     */
	    if(strucmp(outgoing->subject,
		       reply_subject(env->subject, tmp_20k_buf))){
		fs_give((void **)&outgoing->subject);
		outgoing->subject = cpystr("Re: several messages");
	    }
	}
	else
	  outgoing->subject = reply_subject(env->subject, NULL);

	if(hdrs)
	  fs_give((void **)&hdrs);

	for(i=0; i <= NEXTRA; i++)
	  if(values[i])
	    fs_give((void **)&values[i]);
    }

    to_tail	  = &outgoing->to;
    cc_tail	  = &outgoing->cc;
    outgoing->to  = (ADDRESS *)NULL;
    outgoing->cc  = (ADDRESS *)NULL;

    if(saved_from){
	/* Put From in To. */
	*to_tail = reply_cp_addr(pine_state, 0, NULL, outgoing->to, (ADDRESS *)NULL,
				 saved_from);
	/* and the rest in cc */
	if(replytoall){
	    *cc_tail = reply_cp_addr(pine_state, 0, NULL, outgoing->cc, outgoing->to,
				     saved_to);
	    while(*cc_tail)		/* stay on last address */
	      cc_tail = &(*cc_tail)->next;

	    *cc_tail = reply_cp_addr(pine_state, 0, NULL, outgoing->cc, outgoing->to,
				     saved_cc);
	    while(*cc_tail)
	      cc_tail = &(*cc_tail)->next;

	    *cc_tail = reply_cp_addr(pine_state, 0, NULL, outgoing->cc, outgoing->to,
				     saved_resent);
	}
    }
    else if(saved_to){
	/* No From, put To in To. */
	*to_tail = reply_cp_addr(pine_state, 0, NULL, outgoing->to, (ADDRESS *)NULL,
				 saved_to);
	/* and the rest in cc */
	if(replytoall){
	    *cc_tail = reply_cp_addr(pine_state, 0, NULL, outgoing->cc, outgoing->to,
				     saved_cc);
	    while(*cc_tail)
	      cc_tail = &(*cc_tail)->next;

	    *cc_tail = reply_cp_addr(pine_state, 0, NULL, outgoing->cc, outgoing->to,
				     saved_resent);
	}
    }
    else{
	/* No From or To, put everything else in To if replytoall, */
	if(replytoall){
	    *to_tail = reply_cp_addr(pine_state, 0, NULL, outgoing->to, (ADDRESS *)NULL,
				     saved_cc);
	    while(*to_tail)
	      to_tail = &(*to_tail)->next;

	    *to_tail = reply_cp_addr(pine_state, 0, NULL, outgoing->to, (ADDRESS *)NULL,
				     saved_resent);
	}
	/* else, reply to original From which must be us */
	else{
	    /*
	     * Put self in To if in original From.
	     * (notice NULL first arg)
	     */
	    if(!outgoing->newsgroups)
	      *to_tail = reply_cp_addr(NULL, 0, NULL, outgoing->to, (ADDRESS *)NULL,
				       env->from);
	}
    }

    /* add any missing personal data */
    reply_fish_personal(outgoing, env);

    /* get fcc */
    if(outgoing->to && outgoing->to->host[0] != '.')
      fcc.tptr = get_fcc_based_on_to(outgoing->to);
    else if(outgoing->newsgroups){
	char *errmsg = NULL, *newsgroups_returned = NULL;
	int ret_val;

	ret_val = news_build(outgoing->newsgroups, &newsgroups_returned,
			     &errmsg, &fcc);
	if(errmsg){
	    if(*errmsg){
		q_status_message1(SM_ORDER, 3, 3, "%s", errmsg);
		display_message(NO_OP_COMMAND);
	    }
	    fs_give((void **)&errmsg);
	}
	if(ret_val != -1 &&
	    strcmp(outgoing->newsgroups, newsgroups_returned)){
	    fs_give((void **)&outgoing->newsgroups);
	    outgoing->newsgroups = newsgroups_returned;
	}
	else
	  fs_give((void **)&newsgroups_returned);
    }

    seq[++times] = -1L;		/* mark end of sequence list */

    /*==========  Other miscelaneous fields ===================*/   
    outgoing->return_path = (ADDRESS *)NULL;
    outgoing->bcc         = (ADDRESS *)NULL;
    outgoing->sender      = (ADDRESS *)NULL;
    outgoing->return_path = (ADDRESS *)NULL;
    outgoing->in_reply_to = generate_in_reply_to(env);
    outgoing->remail      = NULL;
    outgoing->reply_to    = (ADDRESS *)NULL;

    prefix = reply_quote_str(env, times);
    outgoing->message_id  = generate_message_id(pine_state);

    if(!outgoing->to &&
       !outgoing->cc &&
       !outgoing->bcc &&
       !outgoing->newsgroups)
      q_status_message(SM_ORDER | SM_DING, 3, 6,
		       "Warning: no valid addresses to reply to!");

#if	defined(DOS) && !defined(_WINDOWS)
#if	defined(LWP) || defined(PCTCP) || defined(PCNFS)
#define	IN_RESERVE	8192
#else
#define	IN_RESERVE	16384
#endif
    if((reserve=(char *)malloc(IN_RESERVE)) == NULL){
        q_status_message(SM_ORDER | SM_DING, 3, 4,
			 "Insufficient memory for message text");
	goto done_early;
    }
#endif

   /*==================== Now fix up the message body ====================*/

    /* 
     * create storage object to be used for message text
     */
    if((msgtext = (void *)so_get(PicoText, NULL, EDIT_ACCESS)) == NULL){
        q_status_message(SM_ORDER | SM_DING, 3, 4,
			 "Error allocating message text");
        goto done_early;
    }

    gf_set_so_writec(&pc, (STORE_S *)msgtext);

    /*---- Include the original text if requested ----*/
    if(include_text){
	/* write preliminary envelope info into message text */
	if(F_OFF(F_SIG_AT_BOTTOM, pine_state)){
	    if(sig[0]){
		so_puts((STORE_S *)msgtext, sig);
		fs_give((void **)&sig);
	    }
	    else
	      so_puts((STORE_S *)msgtext, NEWLINE);

	    so_puts((STORE_S *)msgtext, NEWLINE);
	}

	if(totalm > 1L){
	    body                  = mail_newbody();
	    body->type            = TYPETEXT;
	    body->contents.binary = msgtext;
	    env			  = NULL;

	    for(msgno = mn_first_cur(pine_state->msgmap);
		msgno > 0L;
		msgno = mn_next_cur(pine_state->msgmap)){

		if(env){			/* put 2 between messages */
		    gf_puts(NEWLINE, pc);
		    gf_puts(NEWLINE, pc);
		}

		/*--- Grab current envelope ---*/
		env = mail_fetchstructure(pine_state->mail_stream,
					  mn_m2raw(pine_state->msgmap, msgno),
					  &orig_body);
		if(!env || !orig_body){
		    q_status_message1(SM_ORDER,3,4,
			      "Error fetching message %s. Can't reply to it.",
			      long2string(mn_get_cur(pine_state->msgmap)));
		    goto done_early;
		}

		if(orig_body == NULL || orig_body->type == TYPETEXT) {
		    reply_delimiter(env, pc);
		    if(F_ON(F_INCLUDE_HEADER, pine_state))
		      reply_forward_header(pine_state->mail_stream,
					   mn_m2raw(pine_state->msgmap,msgno),
					   env, pc, prefix);

		    get_body_part_text(pine_state->mail_stream, orig_body,
				       mn_m2raw(pine_state->msgmap, msgno),
				       "1", pc, prefix, ". Text not included");
		} else if(orig_body->type == TYPEMULTIPART) {
		    if(!warned++)
		      q_status_message(SM_ORDER,3,7,
		      "WARNING!  Attachments not included in multiple reply.");

		    if(orig_body->contents.part &&
		       orig_body->contents.part->body.type == TYPETEXT) {
			/*---- First part of the message is text -----*/
			reply_delimiter(env, pc);
			if(F_ON(F_INCLUDE_HEADER, pine_state))
			  reply_forward_header(pine_state->mail_stream,
					       mn_m2raw(pine_state->msgmap,
							msgno),
					       env, pc, prefix);

			get_body_part_text(pine_state->mail_stream,
					   &orig_body->contents.part->body,
					   mn_m2raw(pine_state->msgmap, msgno),
					   "1", pc, prefix,
					   ". Text not included");
		    } else {
			q_status_message(SM_ORDER,0,3,
				     "Multipart with no leading text part!");
		    }
		} else {
		    /*---- Single non-text message of some sort ----*/
		    q_status_message(SM_ORDER,3,3,
				     "Non-text message not included!");
		}
	    }
	}
	else{
	    msgno = mn_m2raw(pine_state->msgmap,
			     mn_get_cur(pine_state->msgmap));

	    /*--- Grab current envelope ---*/
	    env = mail_fetchstructure(pine_state->mail_stream, msgno,
				      &orig_body);
	    if(!env || !orig_body) {
		q_status_message1(SM_ORDER,3,4,
			      "Error fetching message %s. Can't reply to it.",
			      long2string(mn_get_cur(pine_state->msgmap)));
		goto done_early;
	    }

	    if(orig_body == NULL || orig_body->type == TYPETEXT
	       || F_OFF(F_ATTACHMENTS_IN_REPLY, pine_state)){

		/*------ Simple text-only message ----*/
		body                  = mail_newbody();
		body->type            = TYPETEXT;
		body->contents.binary = msgtext;
		reply_delimiter(env, pc);
		if(F_ON(F_INCLUDE_HEADER, pine_state))
		  reply_forward_header(pine_state->mail_stream, msgno,
				       env, pc, prefix);

		if(!orig_body || orig_body->type == TYPETEXT){
		    get_body_part_text(pine_state->mail_stream, orig_body,
				       msgno, "1", pc, prefix,
				       ". Text not included");
		}
		else if(orig_body->type == TYPEMULTIPART
			&& orig_body->contents.part->body.type == TYPETEXT){
		    get_body_part_text(pine_state->mail_stream,
				       &orig_body->contents.part->body,
				       msgno, "1", pc, prefix,
				       ". Text not included");
		}
		else{
		    gf_puts(NEWLINE, pc);
		    gf_puts("  [NON-Text Body part not included]", pc);
		    gf_puts(NEWLINE, pc);
		}
	    } else if(orig_body->type == TYPEMULTIPART) {

		/*------ Message is Multipart ------*/
/* BUG: need to do a better job for MULTIPART/alternate --*/
		body = copy_body(NULL, orig_body);

		if(orig_body->contents.part &&
		   orig_body->contents.part->body.type == TYPETEXT) {
		    /*---- First part of the message is text -----*/
		    body->contents.part->body.contents.binary = msgtext;
/* BUG: ? matter that we're not setting body.size.bytes */
		    reply_delimiter(env, pc);
		    if(F_ON(F_INCLUDE_HEADER, pine_state))
		      reply_forward_header(pine_state->mail_stream, msgno,
					   env, pc, prefix);

		    get_body_part_text(pine_state->mail_stream, 
				       &orig_body->contents.part->body,
				       msgno, "1", pc, prefix,
				       ". Text not included");
		    if(!fetch_contents(pine_state->mail_stream,msgno,body,
				       body))
		      q_status_message(SM_ORDER | SM_DING, 3, 4,
				       "Error including all message parts");
		} else {
		    /*--- Fetch the original pieces ---*/
		    if(!fetch_contents(pine_state->mail_stream, msgno, body,
				       body))
		      q_status_message(SM_ORDER | SM_DING, 3, 4,
				       "Error including all message parts");

		    /*--- No text part, create a blank one ---*/
		    part                       = mail_newbody_part();
		    part->next                 = body->contents.part;
		    body->contents.part        = part;
		    part->body.contents.binary = msgtext;
/* BUG: ? matter that we're not setting body.size.bytes */
		}
	    } else {
		/*---- Single non-text message of some sort ----*/
		body                = mail_newbody();
		body->type          = TYPEMULTIPART;
		part                = mail_newbody_part();
		body->contents.part = part;
    
		/*--- The first part, a blank text part to be edited ---*/
		part->body.type            = TYPETEXT;
		part->body.contents.binary = msgtext;
/* BUG: ? matter that we're not setting body.size.bytes */

		/*--- The second part, what ever it is ---*/
		part->next           = mail_newbody_part();
		part                 = part->next;
		part->body.id
				     = generate_message_id(pine_state);
		copy_body(&(part->body), orig_body);
		/*
		 * the idea here is to fetch part into storage object
		 */
		if(part->body.contents.binary = (void *) so_get(PART_SO_TYPE,
							    NULL,EDIT_ACCESS)){
#if	defined(DOS) && !defined(WIN32)
		    mail_parameters(ps_global->mail_stream, SET_GETS,
				    (void *)dos_gets); /* fetched to disk */
		    append_file = (FILE *)so_text(
					(STORE_S *)part->body.contents.binary);

		    if(mail_fetchbody(pine_state->mail_stream, msgno, "1",
				      &part->body.size.bytes) == NULL)
		      goto done;

		    mail_parameters(ps_global->mail_stream, SET_GETS,
				    (void *)NULL);
		    append_file = NULL;
		    mail_gc(pine_state->mail_stream, GC_TEXTS);
#else
		    if((tp=mail_fetchbody(pine_state->mail_stream, msgno, "1",
					  &part->body.size.bytes)) == NULL)
		      goto done;
		    so_puts((STORE_S *)part->body.contents.binary, tp);
#endif
		}
		else
		  goto done;
	    }
	}
    } else {
        /*--------- No text included --------*/
        body                  = mail_newbody();
        body->type            = TYPETEXT;
	body->contents.binary = msgtext;
    }

    if(sig)
      so_puts((STORE_S *)msgtext, sig);

#if	defined(DOS) && !defined(_WINDOWS)
    free((void *)reserve);
#endif

    /* partially formatted outgoing message */
    pine_send(outgoing, &body, "COMPOSE MESSAGE REPLY",
	      fcc.tptr, seq, refs, prefix, NULL, NULL, 0);
  done:
    pine_free_body(&body);
  done_early:
    mail_free_envelope(&outgoing);
    mail_free_address(&saved_from);
    mail_free_address(&saved_to);
    mail_free_address(&saved_cc);
    mail_free_address(&saved_resent);
    fs_give((void **)&seq);

    if(refs)
      fs_give((void **)&refs);

    if(prefix)
      fs_give((void **)&prefix);

    if(fcc.tptr)
      fs_give((void **)&fcc.tptr);

    if(sig)
      fs_give((void **)&sig);

    for(i=0; i <= NEXTRA; i++)
      if(values[i])
	fs_give((void **)&values[i]);
}



/*----------------------------------------------------------------------
    Return a pointer to a copy of the given address list
  filtering out those already in the "mask" lists and ourself.

Args:  mask1  -- Don't copy if in this list
       mask2  --  or if in this list
       source -- List to be copied

  ---*/
ADDRESS *
reply_cp_addr(ps, msgno, field, mask1, mask2, source)
     struct pine *ps;
     long	  msgno;
     char	 *field;
     ADDRESS     *mask1, *mask2, *source;
{
    ADDRESS *tmp1, *tmp2, *ret = NULL, **ret_tail;
    char    *p;

    for(tmp1 = source; msgno && tmp1; tmp1 = tmp1->next)
      if(tmp1->host && tmp1->host[0] == '.'){
	  char *h, *fields[2];

	  fields[0] = field;
	  fields[1] = NULL;
	  if(h = xmail_fetchheader_lines(ps->mail_stream, msgno, fields)){
	      char *p, fname[32];

	      sprintf(fname, "%s:", field);
	      for(p = h; p = strstr(p, fname); )
		rplstr(p, strlen(fname), "");	/* strip field strings */

	      sqznewlines(h);			/* blat out CR's & LF's */
	      for(p = h; p = strchr(p, TAB); )
		*p++ = ' ';			/* turn TABs to whitespace */

	      if(*h){
		  long l;

		  ret = (ADDRESS *) fs_get(sizeof(ADDRESS));
		  memset(ret, 0, sizeof(ADDRESS));

		  /* base64 armor plate the gunk to protect against
		   * c-client quoting in output.
		   */
		  p = (char *) rfc822_binary(h, strlen(h),
					     (unsigned long *) &l);
		  fs_give((void **) &h);
		  ret->mailbox = (char *) fs_get(strlen(p) + 4);
		  sprintf(ret->mailbox, "&%s", p);
		  fs_give((void **) &p);
		  ret->host = cpystr(".RAW-FIELD.");
	      }
	  }

	  return(ret);
      }

    ret_tail = &ret;
    for(source = first_addr(source); source; source = source->next){
	for(tmp1 = first_addr(mask1); tmp1; tmp1 = tmp1->next)
	  if(address_is_same(source, tmp1))
	    break;

	for(tmp2 = first_addr(mask2); tmp2; tmp2 = tmp2->next)
	  if(address_is_same(source, tmp2))
	    break;

	/*
	 * If there's no match in masks *and* this address isn't us, copy...
	 */
	if(!tmp1 && !tmp2 && (!ps || !address_is_us(source, ps))){
	    tmp1         = source->next;
	    source->next = NULL;	/* only copy one addr! */
	    *ret_tail    = rfc822_cpy_adr(source);
	    ret_tail     = &(*ret_tail)->next;
	    source->next = tmp1;	/* restore rest of list */
	}
    }

    return(ret);
}



/*----------------------------------------------------------------------
    Test the given address lists for equivalence

Args:  x -- First address list for comparison
       y -- Second address for comparison

  ---*/
int
addr_lists_same(x, y)
     ADDRESS *x, *y;
{
    for(x = first_addr(x), y = first_addr(y); x && y; x = x->next, y = y->next)
      if(!address_is_same(x, y))
	return(0);

    return(!x && !y);			/* true if ran off both lists */
}



/*----------------------------------------------------------------------
    Test the given address against those in the given envelope's to, cc

Args:  addr -- address for comparison
       env  -- envelope to compare against

  ---*/
int
addr_in_env(addr, env)
    ADDRESS  *addr;
    ENVELOPE *env;
{
    ADDRESS *ap;

    for(ap = env ? env->to : NULL; ap; ap = ap->next)
      if(address_is_same(addr, ap))
	return(1);

    for(ap = env ? env->cc : NULL; ap; ap = ap->next)
      if(address_is_same(addr, ap))
	return(1);

    return(0);				/* not found! */
}



/*----------------------------------------------------------------------
    Add missing personal info dest from src envelope

Args:  dest -- envelope to add personal info to
       src  -- envelope to get personal info from

NOTE: This is just kind of a courtesy function.  It's really not adding
      anything needed to get the mail thru, but it is nice for the user
      under some odd circumstances.
  ---*/
void
reply_fish_personal(dest, src)
     ENVELOPE *dest, *src;
{
    ADDRESS *da, *sa;

    for(da = dest ? dest->to : NULL; da; da = da->next){
	for(sa = src ? src->to : NULL; sa && !da->personal ; sa = sa->next)
	  if(address_is_same(da, sa) && sa->personal)
	    da->personal = cpystr(sa->personal);

	for(sa = src ? src->cc : NULL; sa && !da->personal; sa = sa->next)
	  if(address_is_same(da, sa) && sa->personal)
	    da->personal = cpystr(sa->personal);
    }

    for(da = dest ? dest->cc : NULL; da; da = da->next){
	for(sa = src ? src->to : NULL; sa && !da->personal; sa = sa->next)
	  if(address_is_same(da, sa) && sa->personal)
	    da->personal = cpystr(sa->personal);

	for(sa = src ? src->cc : NULL; sa && !da->personal; sa = sa->next)
	  if(address_is_same(da, sa) && sa->personal)
	    da->personal = cpystr(sa->personal);
    }
}


/*----------------------------------------------------------------------
   Given a header field and envelope, build "References: " header data

Args:  

Returns: 
  ---*/
char *
reply_build_refs(h, env)
    char     *h;
    ENVELOPE *env;
{
    int   l, id_len = strlen(env->message_id);
    char *p, *refs = NULL;

    if(h){
	while((l = strlen(h)) + id_len + 1 > 512 && (p = strstr(h, "> <")))
	  h = p + 2;

	refs = fs_get(id_len + l + 2);
	sprintf(refs, "%s %s", h, env->message_id);
    }

    if(!refs && id_len)
      refs = cpystr(env->message_id);

    return(refs);
}



/*----------------------------------------------------------------------
    Format and return subject suitable for the reply command

Args:  subject -- subject to build reply subject for
       buf -- buffer to use for writing.  If non supplied, alloc one.

Returns: with either "Re:" prepended or not, if already there.
         returned string is allocated.
  ---*/
char *
reply_subject(subject, buf)
     char *subject;
     char *buf;
{
    size_t  l   = (subject && *subject) ? strlen(subject) : 10;
    char   *tmp = fs_get(l + 1), *decoded;

    if(!buf)
      buf = fs_get(l + 5);

    /* decode any 8bit into tmp buffer */
    decoded = (char *) rfc1522_decode((unsigned char *)tmp, subject, NULL);

    if(decoded					/* already "re:" ? */
       && (decoded[0] == 'R' || decoded[0] == 'r')
       && (decoded[1] == 'E' || decoded[1] == 'e')
       &&  decoded[2] == ':')
      strcpy(buf, subject);
    else
      sprintf(buf, "Re: %s", (subject && *subject) ? subject : "your mail");

    fs_give((void **) &tmp);
    return(buf);
}



/*----------------------------------------------------------------------
    return a suitable quoting string "> " by default for replied text

Args:  env -- envelope of message being replied to

Returns: malloc'd array containing quoting string
  ---*/
char *
reply_quote_str(env, times)
    ENVELOPE *env;
    int	      times;
{
    char *prefix, *p;

    /* set up the prefix to quote included text */
    if(p = strstr(ps_global->VAR_REPLY_STRING, "_FROM_")){
	char *from_name = (times > 1) ? "Many"
				      : env->from->mailbox
					 ? env->from->mailbox : "";

	prefix = (char *)fs_get((strlen(ps_global->VAR_REPLY_STRING)
				+ strlen(from_name)) * sizeof(char));
	strcpy(prefix, ps_global->VAR_REPLY_STRING);
	rplstr(prefix + (p - ps_global->VAR_REPLY_STRING), 6, from_name);
	removing_quotes(prefix);
    }
    else
      prefix = removing_quotes(cpystr(ps_global->VAR_REPLY_STRING));

    return(prefix);
}



/*
 * reply_delimeter - output formatted reply delimiter for given envelope
 *		     with supplied character writing function.
 */
void
reply_delimiter(env, pc)
    ENVELOPE *env;
    gf_io_t   pc;
{
    struct date d;

    parse_date(env->date, &d);
    gf_puts("On ", pc);				/* All delims have... */
    if(d.wkday != -1){				/* "On day, date month year" */
	gf_puts(week_abbrev(d.wkday), pc);	/* in common */
	gf_puts(", ", pc);
    }

    gf_puts(int2string(d.day), pc);
    (*pc)(' ');
    gf_puts(month_abbrev(d.month), pc);
    (*pc)(' ');
    gf_puts(int2string(d.year), pc);

    /* but what follows, depends */
    if(!env->from || (env->from->host && env->from->host[0] == '.')){
	gf_puts(", it was written:", pc);
    }
    else if (env->from->personal) {
	gf_puts(", ", pc);
	gf_puts((char *) rfc1522_decode((unsigned char *) tmp_20k_buf,
					env->from->personal, NULL), pc);
	gf_puts(" wrote:", pc);
    }
    else {
	(*pc)(' ');
	gf_puts(env->from->mailbox, pc);
	if(env->from->host){
	    (*pc)('@');
	    gf_puts(env->from->host, pc);
	}

	gf_puts(" wrote:", pc);
    }

    gf_puts(NEWLINE, pc);			/* and end with two newlines */
    gf_puts(NEWLINE, pc);
}


/*
 * reply_poster_followup - return TRUE if "followup-to" set to "poster"
 *
 * NOTE: queues status message indicating such
 */
int
reply_poster_followup(s)
    char *s;
{
    if(s && !strucmp(s, "poster")){
	q_status_message(SM_ORDER, 2, 3,
			 "Replying to Poster as specified in \"Followup-To\"");
	return(1);
    }

    return(0);
}



/*
 * forward_delimiter - return delimiter for forwarded text
 */
void
forward_delimiter(pc)
    gf_io_t pc;
{
    gf_puts(NEWLINE, pc);
    gf_puts("---------- Forwarded message ----------", pc);
    gf_puts(NEWLINE, pc);
}



/*----------------------------------------------------------------------
    Wrapper for header formatting tool

    Args: stream --
	  msgno --
	  env --
	  pc --
	  prefix --

  Result: header suitable for reply/forward text written using "pc"

  ----------------------------------------------------------------------*/
void
reply_forward_header(stream, msgno, env, pc, prefix)
    MAILSTREAM *stream;
    long	msgno;
    ENVELOPE   *env;
    gf_io_t	pc;
    char       *prefix;
{
    int rv;
    HEADER_S h;

    HD_INIT(&h, ps_global->VAR_VIEW_HEADERS, ps_global->view_all_except,
	    FE_DEFAULT & ~FE_BCC);
    if(rv = format_header_text(stream, msgno, env, &h, pc, prefix)){
	if(rv == 1)
	  gf_puts("  [Error fetching message header data]", pc);
    }
    else{
	if(prefix)			/* prefix to write? */
	  gf_puts(prefix, pc);

	gf_puts(NEWLINE, pc);		/* write header delimiter */
    }
}
    


/*----------------------------------------------------------------------
       Partially set up message to forward and pass off to composer/mailer

    Args: pine_state -- The usual pine structure

  Result: outgoing envelope and body created and passed off to composer/mailer

   Create the outgoing envelope for the mail being forwarded, which is 
not much more than filling in the subject, and create the message body
of the outgoing message which requires formatting the header from the
envelope of the original messasge.
  ----------------------------------------------------------------------*/
void
forward(pine_state)
    struct pine   *pine_state;
{
    ENVELOPE      *env, *outgoing;
    BODY          *body, *orig_body, *text_body, *b;
    char          *tmp_text, *sig;
    long           msgno, totalmsgs;
    PART          *part;
    MAILSTREAM    *stream;
    void          *msgtext;
    gf_io_t        pc;
    int            ret;
#if	defined(DOS) && !defined(_WINDOWS)
    char	  *reserve;
#endif

    dprint(4, (debugfile, "\n - forward -\n"));

    stream                = pine_state->mail_stream;
    outgoing              = mail_newenvelope();
    outgoing->message_id  = generate_message_id(pine_state);

    if((totalmsgs = mn_total_cur(pine_state->msgmap)) > 1L){
	sprintf(tmp_20k_buf, "%s forwarded messages...", comatose(totalmsgs));
	outgoing->subject = cpystr(tmp_20k_buf);
    }
    else{
	/*---------- Get the envelope of message we're forwarding ------*/
	msgno = mn_m2raw(pine_state->msgmap, mn_get_cur(pine_state->msgmap));
	if(!(outgoing->subject = forward_subject(pine_state, msgno))){
	    q_status_message1(SM_ORDER,3,4,
			      "Error fetching message %s. Can't forward it.",
			      long2string(msgno));
	    goto clean_early;
	}
    }

    /*
     * as with all text bound for the composer, build it in 
     * a storage object of the type it understands...
     */
    if((msgtext = (void *)so_get(PicoText, NULL, EDIT_ACCESS)) == NULL){
	q_status_message(SM_ORDER | SM_DING, 3, 4,
			 "Error allocating message text");
	goto clean_early;
    }

    
    so_puts((STORE_S *)msgtext, *(sig = get_signature()) ? sig : NEWLINE);
    fs_give((void **)&sig);
    gf_set_so_writec(&pc, (STORE_S *)msgtext);

#if	defined(DOS) && !defined(_WINDOWS)
#if	defined(LWP) || defined(PCTCP) || defined(PCNFS)
#define	IN_RESERVE	8192
#else
#define	IN_RESERVE	16384
#endif
    if((reserve=(char *)malloc(IN_RESERVE)) == NULL){
	so_give(&(STORE_S *)msgtext);
        q_status_message(SM_ORDER | SM_DING, 3, 4,
			 "Insufficient memory for message text");
	goto clean_early;
    }
#endif

    ret = (totalmsgs > 1L)
	   ? want_to("Forward messages as a MIME digest",'y','x',NO_HELP,0,0)
	   : (pine_state->full_header)
	     ? want_to("Forward message as an attachment",'n','x',NO_HELP,0,0)
	     : 0;

    /*
     * If we're forwarding multiple messages *or* the forward-as-mime
     * is turned on and the users wants it done that way, package things
     * up...
     */
    if(ret == 'x'){
	q_status_message(SM_ORDER, 0, 3, "Forward message cancelled");
	goto clean_early;
    }
    else if(ret == 'y'){			/* attach message[s]!!! */
	PART **pp;
	long   totalsize = 0L;

	/*---- New Body to start with ----*/
        body	       = mail_newbody();
        body->type     = TYPEMULTIPART;

        /*---- The TEXT part/body ----*/
        body->contents.part                       = mail_newbody_part();
        body->contents.part->body.type            = TYPETEXT;
        body->contents.part->body.contents.binary = msgtext;

	if(totalmsgs > 1L){
	    /*---- The MULTIPART/DIGEST part ----*/
	    body->contents.part->next			= mail_newbody_part();
	    body->contents.part->next->body.type	= TYPEMULTIPART;
	    body->contents.part->next->body.subtype	= cpystr("Digest");
	    sprintf(tmp_20k_buf, "Digest of %s messages", comatose(totalmsgs));
	    body->contents.part->next->body.description = cpystr(tmp_20k_buf);
	    pp = &(body->contents.part->next->body.contents.part);
	}
	else
	  pp = &(body->contents.part->next);

	/*---- The Message body subparts ----*/
	for(msgno = mn_first_cur(pine_state->msgmap);
	    msgno > 0L;
	    msgno = mn_next_cur(pine_state->msgmap)){
	    msgno		 = mn_m2raw(pine_state->msgmap, msgno);
	    *pp			 = mail_newbody_part();
	    b			 = &((*pp)->body);
	    pp			 = &((*pp)->next);
	    b->type		 = TYPEMESSAGE;
	    b->id		 = generate_message_id(pine_state);
	    b->description       = forward_subject(pine_state, msgno);
	    b->contents.msg.env  = NULL;
	    b->contents.msg.body = NULL;

	    /*---- Package each message in a storage object ----*/
	    if((b->contents.binary = (void *) so_get(PART_SO_TYPE, NULL,
						     EDIT_ACCESS)) == NULL)
	      goto bomb;

	    /* write the header */
	    if((tmp_text = mail_fetchheader(stream, msgno)) && *tmp_text)
	      so_puts((STORE_S *)b->contents.binary, tmp_text);
	    else
	      goto bomb;

#if	defined(DOS) && !defined(WIN32)
	    /* write fetched text to disk */
	    mail_parameters(stream, SET_GETS, (void *)dos_gets);
	    append_file = (FILE *)so_text((STORE_S *)b->contents.binary);

	    /* HACK!  See mailview.c:format_message for details... */
	    stream->text = NULL;
	    /* write the body */
	    if(mail_fetchtext(stream, msgno) == NULL)
	      goto bomb;

	    b->size.bytes = ftell(append_file);
	    /* next time body may stay in core */
	    mail_parameters(stream, SET_GETS, (void *)NULL);
	    append_file   = NULL;
	    mail_gc(stream, GC_TEXTS);
	    so_release((STORE_S *)b->contents.binary);
#else
	    b->size.bytes = strlen(tmp_text);
	    so_puts((STORE_S *)b->contents.binary, "\015\012");
	    if(tmp_text = mail_fetchtext(stream, msgno)){
		if(*tmp_text)
		  so_puts((STORE_S *)b->contents.binary, tmp_text);
	    }
	    else
	      goto bomb;

	    b->size.bytes += strlen(tmp_text);
#endif
	    totalsize += b->size.bytes;
	}

	if(totalmsgs > 1L)
	  body->contents.part->next->body.size.bytes = totalsize;

    }
    else if(totalmsgs > 1L){
	int		        warned = 0;
	body                  = mail_newbody();
	body->type            = TYPETEXT;
	body->contents.binary = msgtext;
	env		      = NULL;

	for(msgno = mn_first_cur(pine_state->msgmap);
	    msgno > 0L;
	    msgno = mn_next_cur(pine_state->msgmap)){

	    if(env){			/* put 2 between messages */
		gf_puts(NEWLINE, pc);
		gf_puts(NEWLINE, pc);
	    }

	    /*--- Grab current envelope ---*/
	    env = mail_fetchstructure(pine_state->mail_stream,
				      mn_m2raw(pine_state->msgmap, msgno),
				      &orig_body);
	    if(!env || !orig_body){
		q_status_message1(SM_ORDER,3,4,
			       "Error fetching message %s. Can't forward it.",
			       long2string(msgno));
		goto bomb;
	    }

	    if(orig_body == NULL || orig_body->type == TYPETEXT) {
		if(!pine_state->anonymous){
		    forward_delimiter(pc);
		    reply_forward_header(pine_state->mail_stream,
					 mn_m2raw(pine_state->msgmap, msgno),
					 env, pc, "");
		}

		if(!get_body_part_text(pine_state->mail_stream, orig_body,
				       mn_m2raw(pine_state->msgmap, msgno),
				       "1", pc, "", ". Text not included"))
		  goto bomb;
	    } else if(orig_body->type == TYPEMULTIPART) {
		if(!warned++)
		  q_status_message(SM_ORDER,3,7,
		    "WARNING!  Attachments not included in multiple forward.");

		if(orig_body->contents.part &&
		   orig_body->contents.part->body.type == TYPETEXT) {
		    /*---- First part of the message is text -----*/
		    forward_delimiter(pc);
		    reply_forward_header(pine_state->mail_stream,
					 mn_m2raw(pine_state->msgmap,msgno),
					 env, pc, "");

		    if(!get_body_part_text(pine_state->mail_stream,
					   &orig_body->contents.part->body,
					   mn_m2raw(pine_state->msgmap, msgno),
					   "1", pc, "", ". Text not included"))
		      goto bomb;
		} else {
		    q_status_message(SM_ORDER,0,3,
				     "Multipart with no leading text part!");
		}
	    } else {
		/*---- Single non-text message of some sort ----*/
		q_status_message(SM_ORDER,0,3,
				 "Non-text message not included!");
	    }
	}
    }
    else {
	env = mail_fetchstructure(pine_state->mail_stream, msgno, &orig_body);
	if(!env || !orig_body){
	    q_status_message1(SM_ORDER,3,4,
			      "Error fetching message %s. Can't forward it.",
			      long2string(msgno));
	    goto clean_early;
	}

        if(orig_body == NULL || orig_body->type == TYPETEXT) {
            /*---- Message has a single text part -----*/
            body                  = mail_newbody();
            body->type            = TYPETEXT;
            body->contents.binary = msgtext;
    	    if(!pine_state->anonymous){
		forward_delimiter(pc);
		reply_forward_header(pine_state->mail_stream, msgno, env,
				     pc, "");
				     
	    }

	    if(!get_body_part_text(pine_state->mail_stream, orig_body,
				   msgno, "1", pc, "", ". Text not included"))
	      goto bomb;
/* BUG: ? matter that we're not setting body.size.bytes */
        } else if(orig_body->type == TYPEMULTIPART) {
            /*---- Message is multipart ----*/
    
            /*--- Copy the body and entire structure  ---*/
            body = copy_body(NULL, orig_body);
    
            /*--- The text part of the message ---*/
	    if(!orig_body->contents.part){
		q_status_message(SM_ORDER | SM_DING, 3, 6,
				 "Error referencing body part 1");
		goto bomb;
	    }
	    else if(orig_body->contents.part->body.type == TYPETEXT) {
                /*--- The first part is text ----*/
		text_body                  = &body->contents.part->body;
		text_body->contents.binary = msgtext;
    		if(!pine_state->anonymous){
		    forward_delimiter(pc);
		    reply_forward_header(pine_state->mail_stream, msgno, env,
					 pc, "");
		}

		if(!get_body_part_text(pine_state->mail_stream, 
				       &orig_body->contents.part->body,
				       msgno, "1", pc, "",
				       ". Text not included"))
		  goto bomb;
/* BUG: ? matter that we're not setting body.size.bytes */
                if(!fetch_contents(stream, msgno, body, body))
                  goto bomb;
	    } else {
		if(!fetch_contents(stream, msgno, body, body))
		  goto bomb;

                /*--- Create a new blank text part ---*/
                part                       = mail_newbody_part();
                part->next                 = body->contents.part;
                body->contents.part        = part;
                part->body.contents.binary = msgtext;
            }
        } else {
            /*---- A single part message, not of type text ----*/
            body                     = mail_newbody();
            body->type               = TYPEMULTIPART;
            part                     = mail_newbody_part();
            body->contents.part      = part;
    
            /*--- The first part, a blank text part to be edited ---*/
            part->body.type            = TYPETEXT;
            part->body.contents.binary = msgtext;
    
            /*--- The second part, what ever it is ---*/
            part->next               = mail_newbody_part();
            part                     = part->next;
            part->body.id            = generate_message_id(pine_state);
            copy_body(&(part->body), orig_body);
	    /*
	     * the idea here is to fetch part into storage object
	     */
	    if(part->body.contents.binary = (void *) so_get(PART_SO_TYPE, NULL,
							    EDIT_ACCESS)){
#if	defined(DOS) && !defined(WIN32)
		/* fetched text to disk */
		mail_parameters(stream, SET_GETS, (void *)dos_gets);
		append_file = (FILE *)so_text(
				       (STORE_S *)part->body.contents.binary);

		if(mail_fetchbody(stream, msgno, "1",
					&part->body.size.bytes) == NULL)
		  goto bomb;

		/* next time body may stay in core */
		mail_parameters(stream, SET_GETS, (void *)NULL);
		append_file = NULL;
		mail_gc(stream, GC_TEXTS);
#else
		if(tmp_text = mail_fetchbody(stream, msgno, "1",
					  &part->body.size.bytes))
		  so_puts((STORE_S *)part->body.contents.binary, tmp_text);
		else
		  goto bomb;
#endif
	    }
	    else
	      goto bomb;
        }
    }

#if	defined(DOS) && !defined(_WINDOWS)
    free((void *)reserve);
#endif
    if(pine_state->anonymous)
      pine_simple_send(outgoing, &body, NULL, NULL, NULL, 1);
    else			/* partially formatted outgoing message */
      pine_send(outgoing, &body,
		pine_state->nr_mode ? "SEND MESSAGE" : "FORWARD MESSAGE",
		NULL, NULL, NULL, NULL, NULL, NULL, 0);

  clean:
    pine_free_body(&body);
  clean_early:
    mail_free_envelope(&outgoing);
    return;

  bomb:
#if	defined(DOS) && !defined(WIN32)
    mail_parameters(stream, SET_GETS, (void *)NULL);
    append_file = NULL;
    mail_gc(pine_state->mail_stream, GC_TEXTS);
#endif
    q_status_message(SM_ORDER | SM_DING, 4, 5,
		   "Error fetching message contents.  Can't forward message.");
    goto clean;
}



/*----------------------------------------------------------------------
  Build the subject for the message number being forwarded

    Args: pine_state -- The usual pine structure
          msgno      -- The message number to build subject for

  Result: malloc'd string containing new subject or NULL on error

  ----------------------------------------------------------------------*/
char *
forward_subject(pine_state, msgno)
     struct pine *pine_state;
     long	  msgno;
{
    ENVELOPE *env;
    size_t    l;

    if(!(env = mail_fetchstructure(pine_state->mail_stream, msgno, NULL)))
      return(NULL);

    dprint(9, (debugfile, "checking subject: \"%s\"\n",
	       env->subject ? env->subject : "NULL"));

    if(env->subject && env->subject[0]){		/* add (fwd)? */
	/* decode any 8bit (copy to the temp buffer if decoding doesn't) */
	if(rfc1522_decode((unsigned char *) tmp_20k_buf,
			 env->subject, NULL) == (unsigned char *) env->subject)
	  strcpy(tmp_20k_buf, env->subject);

	removing_trailing_white_space(tmp_20k_buf);
	if((l=strlen(tmp_20k_buf)) < 5 || strcmp(tmp_20k_buf+l-5,"(fwd)"))
#ifdef	OLDWAY
	  strcat(tmp_20k_buf, " (fwd)");
#else
	  sprintf(tmp_20k_buf, "%s (fwd)", env->subject);
#endif
	return(cpystr(tmp_20k_buf));

    }

    return(cpystr("Forwarded mail...."));
}



/*----------------------------------------------------------------------
       Partially set up message to forward and pass off to composer/mailer

    Args: pine_state -- The usual pine structure
          message    -- The MESSAGECACHE of entry to reply to 

  Result: outgoing envelope and body created and passed off to composer/mailer

   Create the outgoing envelope for the mail being forwarded, which is 
not much more than filling in the subject, and create the message body
of the outgoing message which requires formatting the header from the
envelope of the original messasge.
  ----------------------------------------------------------------------*/
void
forward_text(pine_state, text, source)
     struct pine *pine_state;
     void        *text;
     SourceType   source;
{
    ENVELOPE *env;
    BODY     *body;
    gf_io_t   pc, gc;
    STORE_S  *msgtext;
    char     *enc_error, *sig;

    if(msgtext = so_get(PicoText, NULL, EDIT_ACCESS)){
	env                   = mail_newenvelope();
	env->message_id       = generate_message_id(pine_state);
	body                  = mail_newbody();
	body->type            = TYPETEXT;
	body->contents.binary = (void *) msgtext;

	if(!pine_state->anonymous){
	    so_puts(msgtext, *(sig = get_signature()) ? sig : NEWLINE);
	    so_puts(msgtext, NEWLINE);
	    so_puts(msgtext, "----- Included text -----");
	    so_puts(msgtext, NEWLINE);
	    fs_give((void **)&sig);
	}

	gf_filter_init();
	gf_set_so_writec(&pc, msgtext);
	gf_set_readc(&gc,text,(source == CharStar) ? strlen((char *)text) : 0L,
		     source);

	if((enc_error = gf_pipe(gc, pc)) == NULL){
	    if(pine_state->anonymous){
		pine_simple_send(env, &body, NULL, NULL, NULL, 1);
		pine_state->mangled_footer = 1;
	    }
	    else{
		pine_send(env, &body, "SEND MESSAGE", NULL, NULL, NULL, NULL,
			  NULL, NULL, 0);
		pine_state->mangled_screen = 1;
	    }
	}
	else{
	    q_status_message1(SM_ORDER | SM_DING, 3, 5,
			      "Error reading text \"%s\"",enc_error);
	    display_message('x');
	}

	mail_free_envelope(&env);
	pine_free_body(&body);
    }
    else {
	q_status_message(SM_ORDER | SM_DING, 3, 4,
			 "Error allocating message text");
	display_message('x');
    }
}



/*----------------------------------------------------------------------
       Partially set up message to resend and pass off to mailer

    Args: pine_state -- The usual pine structure

  Result: outgoing envelope and body created and passed off to mailer

   Create the outgoing envelope for the mail being resent, which is 
not much more than filling in the subject, and create the message body
of the outgoing message which requires formatting the header from the
envelope of the original messasge.
  ----------------------------------------------------------------------*/
void
bounce(pine_state)
    struct pine   *pine_state;
{
    ENVELOPE      *env, *outgoing = NULL;
    BODY          *body;
    long           msgno, rawmsgno, firstone;
    MAILSTREAM    *stream;
    void          *msgtext;
    gf_io_t        pc;
    char          *save_to = NULL, *h, *p, **save_toptr,
		  *prmpt_who, bprmpt_who[80],
		  *prmpt_cnf, bprmpt_cnf[80];
    int            i, got_body;

    dprint(4, (debugfile, "\n - bounce -\n"));

    stream = pine_state->mail_stream;
    firstone = mn_first_cur(pine_state->msgmap);
    if(mn_total_cur(pine_state->msgmap) > 1L){
	sprintf(bprmpt_who, "BOUNCE (redirect) %d messages to : ",
			mn_total_cur(pine_state->msgmap));
	prmpt_who = bprmpt_who;
	sprintf(bprmpt_cnf, "Send %d messages to ",
			mn_total_cur(pine_state->msgmap));
	prmpt_cnf = bprmpt_cnf;
    }
    else{
	prmpt_who = NULL;
	prmpt_cnf = NULL;
    }

    for(msgno = firstone; msgno > 0L; msgno = mn_next_cur(pine_state->msgmap)){

	rawmsgno = mn_m2raw(pine_state->msgmap, msgno);
	env = mail_fetchstructure(stream, rawmsgno, &body);
	if(!env || !body){
	    q_status_message(SM_ORDER | SM_DING, 4, 7,
		     "Error fetching message contents. Can't resend message");
	    goto bomb;
	}

	outgoing              = mail_newenvelope();
	outgoing->message_id  = generate_message_id(pine_state);

	/*==================== Subject ====================*/
	if(env->subject == NULL || strlen(env->subject) == 0)
	  /* --- Blank, make up one ---*/
	  outgoing->subject = cpystr("Resent mail....");

	/* build remail'd header */
	if(h = mail_fetchheader(stream, rawmsgno)){
	    for(p = h, i = 0; p = strchr(p, ':'); p++)
	      i++;

	    /* allocate it */
	    outgoing->remail = (char *) fs_get(strlen(h) + (2 * i) + 1);

	    /*
	     * copy it, "X-"ing out transport headers bothersome to
	     * software but potentially useful to the human recipient...
	     */
	    p = outgoing->remail;
	    bounce_mask_header(&p, h);
	    do
	      if(*h == '\015' && *(h+1) == '\012'){
		  *p++ = *h++;		/* copy CR LF */
		  *p++ = *h++;
		  bounce_mask_header(&p, h);
	      }
	    while(*p++ = *h++);
	}
	/* BUG: else complain? */
	     
	/*
	 * as with all text bound for the composer, build it in 
	 * a storage object of the type it understands...
	 */
	if((msgtext = (void *)so_get(PicoText, NULL, EDIT_ACCESS)) == NULL){
	    q_status_message(SM_ORDER | SM_DING, 3, 4,
			     "Error allocating message text");
	    mail_free_envelope(&outgoing);
	    return;
	}

	/*
	 * Build a fake body description.  It's ignored by pine_rfc822_header,
	 * but we need to set it to something that makes set_mime_types
	 * not sniff it and pine_rfc822_output_body not re-encode it.
	 * Setting the encoding to (ENCMAX + 1) will work and shouldn't cause
	 * problems unless something tries to access body_encodings[] using
	 * it without proper precautions.  We don't want to use ENCOTHER
	 * cause that tells set_mime_types to sniff it, and we don't want to
	 * use ENC8BIT since that tells pine_rfc822_output_body to qp-encode
	 * it.  When there's time, it'd be nice to clean this interaction
	 * up...
	 */
	body		  = mail_newbody();
	body->type	  = TYPETEXT;
	body->encoding	  = ENCMAX + 1;
	body->subtype	  = cpystr("Plain");
	body->contents.binary = msgtext;
	gf_set_so_writec(&pc, (STORE_S *)msgtext);

	if(mn_total_cur(pine_state->msgmap) > 1L){
	    if(msgno == firstone)
	      save_toptr = &save_to;
	    else{
		static char *fakedomain = "@";
		char *tmp_a_string;

		save_toptr = NULL;
		/* rfc822_parse_adrlist feels free to destroy input so copy */
		tmp_a_string = cpystr(save_to);
		rfc822_parse_adrlist(&outgoing->to, tmp_a_string,
		    (F_ON(F_COMPOSE_REJECTS_UNQUAL, ps_global))
		      ? fakedomain : ps_global->maildomain);
		fs_give((void **)&tmp_a_string);
	    }
	}
	else
	  save_toptr = NULL;

	/* pass NULL body to force mail_fetchtext */
	if(!(got_body = get_body_part_text(stream, NULL, rawmsgno, "1", pc, "",
					". Text not included"))
	   || pine_simple_send(outgoing, &body, prmpt_who,
			       prmpt_cnf, save_toptr, msgno==firstone) < 0){
	    pine_free_body(&body);
	    mail_free_envelope(&outgoing);
	    if(!got_body)
	      q_status_message(SM_ORDER | SM_DING, 4, 7,
		     "Error fetching message contents. Can't resend message");

	    goto bomb;
	}

	pine_free_body(&body);
	mail_free_envelope(&outgoing);
    }

    if(save_to)
      fs_give((void **)&save_to);

    return;

  bomb:
#if	defined(DOS) && !defined(WIN32)
    mail_parameters(stream, SET_GETS, (void *)NULL);
    append_file = NULL;
    mail_gc(pine_state->mail_stream, GC_TEXTS);
#endif
    if(save_to)
      fs_give((void **)&save_to);

    return;

}



/*----------------------------------------------------------------------
    Mask off any header entries we don't want xport software to see

Args:  d -- destination string pointer pointer
       s -- source string pointer pointer

  ----*/
void
bounce_mask_header(d, s)
    char **d, *s;
{
    if((*s == 'R' || *s == 'r')
       && (!struncmp(s+1, "esent-", 6) || !struncmp(s+1, "eceived:", 8))){
	*(*d)++ = 'X';				/* write mask */
	*(*d)++ = '-';
    }
}


        
/*----------------------------------------------------------------------
    Fetch and format text for forwarding

Args:  stream      -- Mail stream to fetch text for
       message_no  -- Message number of text for foward
       part_number -- Part number of text to forward
       env         -- Envelope of message being forwarded
       body        -- Body structure of message being forwarded

Returns:  true if OK, false if problem occured while filtering

If the text is richtext, it will be converted to plain text, since there's
no rich text editing capabilities in Pine (yet). The character sets aren't
really handled correctly here. Theoretically editing should not be allowed
if text character set doesn't match what term-character-set is set to.

It's up to calling routines to plug in signature appropriately

As with all internal text, NVT end-of-line conventions are observed.
DOESN'T sanity check the prefix given!!!
  ----*/
int
get_body_part_text(stream, body, msg_no, part_no, pc, prefix, hmm)
    MAILSTREAM *stream;
    BODY       *body;
    long        msg_no;
    char       *part_no;
    gf_io_t     pc;
    char       *prefix;
    char       *hmm; 
{
    int	      i, we_cancel = 0;
    filter_t  filters[3];
    long      len;
    char     *err;
#if	defined(DOS) && !defined(WIN32)
    char     *tmpfile_name = NULL;
#endif

    we_cancel = busy_alarm(1, NULL, NULL, 0);

    /* if null body, we must be talking to a non-IMAP2bis server.
     * No MIME parsing provided, so we just grab the message text...
     */
    if(body == NULL){
	char         *text, *decode_error;
	MESSAGECACHE *mc;
	gf_io_t       gc;
	SourceType    src = CharStar;
	int           rv = 0;

	(void)mail_fetchstructure(stream, msg_no, NULL);
	mc = mail_elt(stream,  msg_no);

#if	defined(DOS) && !defined(WIN32)
	if(mc->rfc822_size > MAX_MSG_INCORE
	  || (ps_global->context_current->type & FTYPE_BBOARD)){
	    src = FileStar;		/* write fetched text to disk */
	    if(!(tmpfile_name = temp_nam(NULL, "pt"))
	       || !(append_file = fopen(tmpfile_name, "w+b"))){
		if(tmpfile_name)
		  fs_give((void **)&tmpfile_name);

		q_status_message1(SM_ORDER,3,4,"Can't build tmpfile: %s",
				  error_description(errno));
		if(we_cancel)
		  cancel_busy_alarm(-1);

		return(rv);
	    }

	    mail_parameters(stream, SET_GETS, (void *)dos_gets);
	}
	else				/* message stays in core */
	  mail_parameters(stream, SET_GETS, (void *)NULL);
#endif

	if(text = mail_fetchtext(stream, msg_no)){
#if	defined(DOS) && !defined(WIN32)
	    if(src == FileStar)
	      gf_set_readc(&gc, append_file, 0L, src);
	    else
#endif
	    gf_set_readc(&gc, text, (unsigned long)strlen(text), src);

	    gf_filter_init();		/* no filters needed */
	    if(decode_error = gf_pipe(gc, pc)){
		sprintf(tmp_20k_buf, "%s%s    [Formatting error: %s]%s",
			NEWLINE, NEWLINE,
			decode_error, NEWLINE);
		gf_puts(tmp_20k_buf, pc);
		rv++;
	    }
	}
	else{
	    gf_puts(NEWLINE, pc);
	    gf_puts("    [ERROR fetching text of message]", pc);
	    gf_puts(NEWLINE, pc);
	    gf_puts(NEWLINE, pc);
	    rv++;
	}

#if	defined(DOS) && !defined(WIN32)
	/* clean up tmp file created for dos_gets ?  If so, trash
	 * cached knowledge, and make sure next fetch stays in core
	 */
	if(src == FileStar){
	    fclose(append_file);
	    append_file = NULL;
	    unlink(tmpfile_name);
	    fs_give((void **)&tmpfile_name);
	    mail_parameters(stream, SET_GETS, (void *)NULL);
	    mail_gc(stream, GC_TEXTS);
	}
#endif
	if(we_cancel)
	  cancel_busy_alarm(-1);

	return(rv == 0);
    }

    filters[i = 0] = NULL;

    /*
     * just use detach, but add an auxiliary filter to insert prefix,
     * and, perhaps, digest richtext
     */
    if(body->subtype != NULL && strucmp(body->subtype,"richtext") == 0){
	gf_rich2plain_opt(1);		/* set up to filter richtext */
	filters[i++] = gf_rich2plain;
    }

/* BUG: is msgno stuff working right ??? */
/* BUG: not using "hmm" !!! */
    gf_prefix_opt(prefix);
    filters[i++] = gf_prefix;
    filters[i++] = NULL;
    err = detach(stream, msg_no, body, part_no, &len, pc, filters);
    if (err != (char *)NULL)
       q_status_message2(SM_ORDER, 3, 4,
	   "%s: message number %ld",err,(void *)msg_no);
    if(we_cancel)
      cancel_busy_alarm(-1);

    return((int)len);
}



/*----------------------------------------------------------------------
  return the c-client reference name for the given end_body part
  ----*/
char *
partno(body, end_body)
     BODY *body, *end_body;
{
    PART *part;
    int   num = 0;
    char  tmp[64], *p = NULL;

    if(body && body->type == TYPEMULTIPART) {
	part = body->contents.part;	/* first body part */

	do {				/* for each part */
	    num++;
	    if(&part->body == end_body || (p = partno(&part->body, end_body))){
		sprintf(tmp, "%d%s%s", num, (p) ? "." : "", (p) ? p : "");
		if(p)
		  fs_give((void **)&p);

		return(cpystr(tmp));
	    }
	} while (part = part->next);	/* until done */

	return(NULL);
    }
    else if(body && body->type == TYPEMESSAGE && body->subtype 
	    && !strucmp(body->subtype, "rfc822")){
	return(partno(body->contents.msg.body, end_body));
    }

    return((body == end_body) ? cpystr("1") : NULL);
}



/*----------------------------------------------------------------------
   Fill in the contents of each body part

Args: stream      -- Stream the message is on
      msgno       -- Message number the body structure is for
      root        -- Body pointer to start from
      body        -- Body pointer to fill in

Result: 1 if all went OK, 0 if there was a problem

This function copies the contents from an original message/body to
a new message/body.  It recurses down all multipart levels.

If one or more part (but not all) can't be fetched, a status message
will be queued.
 ----*/
int
fetch_contents(stream, msgno, root, body)
     MAILSTREAM *stream;
     long        msgno;
     BODY       *root, *body;
{
    char *pnum = NULL, *tp;
    int   got_one = 0;

    if(!body->id)
      body->id = generate_message_id(ps_global);
          
    if(body->type == TYPEMULTIPART){
	int   last_one = 10;		/* remember worst case */
	PART *part     = body->contents.part;

	if(!(part = body->contents.part))
	  return(0);

	do {
	    got_one  = fetch_contents(stream, msgno, root, &part->body);
	    last_one = min(last_one, got_one);
	}
	while(part = part->next);

	return(last_one);
    }

    if(body->contents.binary)
      return(1);			/* already taken care of... */

    pnum = partno(root, body);

    if(body->type == TYPEMESSAGE){
	body->contents.msg.env  = NULL;
	body->contents.msg.body = NULL;
	if(body->subtype && strucmp(body->subtype,"external-body")){
	    /*
	     * the idea here is to fetch everything into storage objects
	     */
	    body->contents.binary = (void *) so_get(PART_SO_TYPE, NULL,
						    EDIT_ACCESS);
#if	defined(DOS) && !defined(WIN32)
	    if(body->contents.binary){
		/* fetch text to disk */
		mail_parameters(stream, SET_GETS, (void *)dos_gets);
		append_file =(FILE *)so_text((STORE_S *)body->contents.binary);

		if(mail_fetchbody(stream, msgno, pnum, &body->size.bytes)){
		    so_release((STORE_S *)body->contents.binary);
		    got_one = 1;
		}
		else
		  q_status_message1(SM_ORDER | SM_DING, 3, 3,
				    "Error fetching part %s",pnum);

		/* next time body may stay in core */
		mail_parameters(stream, SET_GETS, (void *)NULL);
		append_file = NULL;
		mail_gc(stream, GC_TEXTS);
	    }
#else
	    if(body->contents.binary
	       && (tp = mail_fetchbody(stream,msgno,pnum,&body->size.bytes))){
		so_puts((STORE_S *)body->contents.binary, tp);
		got_one = 1;
	    }
#endif
	    else
	      q_status_message1(SM_ORDER | SM_DING, 3, 3,
				"Error fetching part %s",pnum);
	} else {
	    got_one = 1;
	}
    } else {
	/*
	 * the idea here is to fetch everything into storage objects
	 * so, grab one, then fetch the body part
	 */
	body->contents.binary = (void *)so_get(PART_SO_TYPE,NULL,EDIT_ACCESS);
#if	defined(DOS) && !defined(WIN32)
	if(body->contents.binary){
	    /* write fetched text to disk */
	    mail_parameters(stream, SET_GETS, (void *)dos_gets);
	    append_file = (FILE *)so_text((STORE_S *)body->contents.binary);
	    if(mail_fetchbody(stream, msgno, pnum, &body->size.bytes)){
		so_release((STORE_S *)body->contents.binary);
		got_one = 1;
	    }
	    else
	      q_status_message1(SM_ORDER | SM_DING, 3, 3,
				"Error fetching part %s",pnum);

	    /* next time body may stay in core */
	    mail_parameters(stream, SET_GETS, (void *)NULL);
	    append_file = NULL;
	    mail_gc(stream, GC_TEXTS);
	}
#else
	if(body->contents.binary
	   && (tp=mail_fetchbody(stream, msgno, pnum, &body->size.bytes))){
	    so_puts((STORE_S *)body->contents.binary, tp);
	    got_one = 1;
	}
#endif
	else
	  q_status_message1(SM_ORDER | SM_DING, 3, 3,
			    "Error fetching part %s",pnum);
    }

    if(pnum)
      fs_give((void **)&pnum);

    return(got_one);

}



/*----------------------------------------------------------------------
    Copy the body structure

Args: new_body -- Pointer to already allocated body, or NULL, if none
      old_body -- The Body to copy


 This is called recursively traverses the body structure copying all the
elements. The new_body parameter can be NULL in which case a new body is
allocated. Alternatively it can point to an already allocated body
structure. This is used when copying body parts since a PART includes a 
BODY. The contents fields are *not* filled in.
  ----*/

BODY *
copy_body(new_body, old_body)
     BODY *old_body, *new_body;
{
    PART *new_part, *old_part;

    if(old_body == NULL)
      return(NULL);

    if(new_body == NULL)
      new_body = mail_newbody();

    *new_body = *old_body;
    if(old_body->id)
      new_body->id = cpystr(old_body->id);

    if(old_body->description)
      new_body->description = cpystr(old_body->description);

    if(old_body->subtype)
      new_body->subtype = cpystr(old_body->subtype);

    new_body->parameter = copy_parameters(old_body->parameter);

    new_part = NULL;
    if(new_body->type == TYPEMULTIPART) {
        for(old_part = new_body->contents.part; old_part != NULL;
            old_part = old_part->next){
            if(new_part == NULL) {
                new_part = mail_newbody_part();
                new_body->contents.part = new_part;
            } else {
                new_part->next = mail_newbody_part();
                new_part = new_part->next;
            }
            copy_body(&(new_part->body), &(old_part->body));
        }
    } else {
        new_body->contents.binary = NULL;
    }
    return(new_body);
}



/*----------------------------------------------------------------------
    Copy the MIME parameter list
 
 Allocates storage for new part, and returns pointer to new paramter
list. If old_p is NULL, NULL is returned.
 ----*/

PARAMETER *
copy_parameters(old_p)
     PARAMETER *old_p;
{
    PARAMETER *new_p, *p1, *p2;

    if(old_p == NULL)
      return((PARAMETER *)NULL);

    new_p = p2 = NULL;
    for(p1 = old_p; p1 != NULL; p1 = p1->next) {
        if(new_p == NULL) {
            p2 = mail_newbody_parameter();
            new_p = p2;
        } else {
            p2->next = mail_newbody_parameter();
            p2 = p2->next;
        }
        p2->attribute = cpystr(p1->attribute);
        p2->value     = cpystr(p1->value);
    }
    return(new_p);
}
    
    

/*----------------------------------------------------------------------
    Make a complete copy of an envelope and all it's fields

Args:    e -- the envelope to copy

Result:  returns the new envelope, or NULL, if the given envelope was NULL

  ----*/

ENVELOPE *    
copy_envelope(e)
     register ENVELOPE *e;
{
    register ENVELOPE *e2;

    if(!e)
      return(NULL);

    e2		    = mail_newenvelope();
    e2->remail      = e->remail	     ? cpystr(e->remail)	      : NULL;
    e2->return_path = e->return_path ? rfc822_cpy_adr(e->return_path) : NULL;
    e2->date        = e->date	     ? cpystr(e->date)		      : NULL;
    e2->from        = e->from	     ? rfc822_cpy_adr(e->from)	      : NULL;
    e2->sender      = e->sender	     ? rfc822_cpy_adr(e->sender)      : NULL;
    e2->reply_to    = e->reply_to    ? rfc822_cpy_adr(e->reply_to)    : NULL;
    e2->subject     = e->subject     ? cpystr(e->subject)	      : NULL;
    e2->to          = e->to          ? rfc822_cpy_adr(e->to)	      : NULL;
    e2->cc          = e->cc          ? rfc822_cpy_adr(e->cc)	      : NULL;
    e2->bcc         = e->bcc         ? rfc822_cpy_adr(e->bcc)	      : NULL;
    e2->in_reply_to = e->in_reply_to ? cpystr(e->in_reply_to)	      : NULL;
    e2->newsgroups  = e->newsgroups  ? cpystr(e->newsgroups)	      : NULL;
    e2->message_id  = e->message_id  ? cpystr(e->message_id)	      : NULL;
    return(e2);
}


/*----------------------------------------------------------------------
     Generate the "In-reply-to" text from message header

  Args: message -- Envelope of original message

  Result: returns an alloc'd string or NULL if there is a problem
 ----*/
char *
generate_in_reply_to(env)
    ENVELOPE *env;
{
    return((env && env->message_id) ? cpystr(env->message_id) : NULL);
}


/*----------------------------------------------------------------------
        Generate a unique message id string.

   Args: ps -- The usual pine structure

  Result: Alloc'd unique string is returned

Uniqueness is gaurenteed by using the host name, process id, date to the
second and a single unique character
*----------------------------------------------------------------------*/
char *
generate_message_id(ps)
     struct pine *ps;
{
    static char a = 'A';
    char       *id;
    time_t      now;
    struct tm  *now_x;

    now   = time((time_t *)0);
    now_x = localtime(&now);
    id    = (char *)fs_get(128 * sizeof(char));

    sprintf(id,"<Pine.%.4s.%.20s.%02d%02d%02d%02d%02d%02d.%d%c@%.50s>",
	    SYSTYPE, pine_version, now_x->tm_year, now_x->tm_mon + 1,
	    now_x->tm_mday, now_x->tm_hour, now_x->tm_min, now_x->tm_sec,
	    getpid(), a, ps->hostname);

    a = (a == 'Z') ? 'a' : (a == 'z') ? 'A' : a + 1;
    return(id);
}



/*----------------------------------------------------------------------
  Return the first true address pointer (modulo group syntax allowance)

  Args: addr  -- Address list

 Result: First real address pointer, or NULL
  ----------------------------------------------------------------------*/
ADDRESS *
first_addr(addr)
    ADDRESS *addr;
{
    while(addr && !addr->host)
      addr = addr->next;

    return(addr);
}



/*----------------------------------------------------------------------
  Acquire the pinerc defined signature file

  ----*/
char *
get_signature()
{
    char *sig = NULL, *tmp_sig, sig_path[MAXPATH+1];

    /*----- Get the signature if there is one to get -----*/
    if(signature_path(ps_global->VAR_SIGNATURE_FILE, sig_path, MAXPATH)
       && can_access(sig_path, ACCESS_EXISTS) == 0){
	if(tmp_sig = read_file(sig_path)){
	    sig = fs_get(strlen(tmp_sig) + 10);
	    strcpy(sig, NEWLINE);
	    strcat(sig, NEWLINE);
	    strcat(sig, tmp_sig);
	    fs_give((void **)&tmp_sig);
	}
	else
	  q_status_message2(SM_ORDER | SM_DING, 3, 4,
			    "Error \"%s\" reading signature file \"%s\"",
			    error_description(errno), sig_path);
    }

    return(sig ? sig : cpystr(""));
}



/*----------------------------------------------------------------------
  Acquire the pinerc defined signature file pathname

  ----*/
char *
signature_path(sname, sbuf, len)
    char   *sname, *sbuf;
    size_t  len;
{
    *sbuf = '\0';
    if(sname && *sname){
	size_t spl = strlen(sname);
	if(is_absolute_path(sname)){
	    if(spl < len - 1)
	      strcpy(sbuf, sname);
	}
#ifndef	DOS
	else if(ps_global->VAR_SIGNATURE_FILE[0] == '~'){
	    strcpy(sbuf, sname);
	    fnexpand(sbuf, len);
	}
#endif
	else{
	    char *lc = last_cmpnt(ps_global->pinerc);

	    sbuf[0] = '\0';
	    if(lc != NULL){
		strncpy(sbuf,ps_global->pinerc,min(len-1,lc-ps_global->pinerc));
		sbuf[min(len-1,lc-ps_global->pinerc)] = '\0';
	    }

	    sbuf[spl+ strlen(sbuf)] = '\0';
	    strncat(sbuf, sname, spl + strlen(sbuf));
	}
    }

    return(*sbuf ? sbuf : NULL);
}
