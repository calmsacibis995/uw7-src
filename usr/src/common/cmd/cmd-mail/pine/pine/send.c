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
       send.c
       Functions for composing and sending mail

 ====*/

#include "headers.h"
#include "../c-client/smtp.h"
#include "../c-client/nntp.h"


#ifndef TCPSTREAM
#define TCPSTREAM void
#endif

#define	MIME_VER	"MIME-Version: 1.0\015\012"


/*
 * Internal Prototypes
 */
int	   redraft PROTO((CONTEXT_S *, char *, ENVELOPE **, BODY **, char **,
		   char **, char **, PINEFIELD **));
void	   redraft_cleanup PROTO((char *, char *, MAILSTREAM *));
void       outgoing2strings PROTO((METAENV *, BODY *, void **, PATMT **));
void       strings2outgoing PROTO((METAENV *, BODY **, PATMT *));
void	   resolve_encoded_entries PROTO((ADDRESS *, ADDRESS *));
int        write_fcc PROTO((char *, CONTEXT_S *, STORE_S *, char *));
void       create_message_body PROTO((BODY **, PATMT *));
int	   check_addresses PROTO((METAENV *));
ADDRESS   *generate_from PROTO(());
void       free_attachment_list PROTO((PATMT **));
PINEFIELD *get_dflt_custom_hdrs PROTO(());
void       free_customs PROTO((PINEFIELD *));
int        view_as_rich PROTO((char *, int));
int        count_custom_hdrs PROTO((void));
int        is_a_forbidden_hdr PROTO((char *));
int        is_a_std_hdr PROTO((char *));
int        hdr_has_custom_value PROTO((char *));
void       set_default_hdrval PROTO((PINEFIELD *));
void       customized_hdr_setup PROTO((PINEFIELD *));
void       update_message_id PROTO((ENVELOPE *, char *));
int	   filter_message_text PROTO((char *, ENVELOPE *, BODY *, STORE_S **));
long	   message_format_for_pico PROTO((long, int (*)(int)));
char	  *send_exit_for_pico PROTO(());
int	   mime_type_for_pico PROTO((char *));
void       pine_write_body_header PROTO((char **, BODY  *));
void       pine_encode_body PROTO((BODY *));
int        pine_rfc822_header PROTO((METAENV *, BODY *, soutr_t, TCPSTREAM *));
int        pine_header_line PROTO((char *, char *, METAENV *, char *, soutr_t,
			    TCPSTREAM *, int, int));
int	   pine_address_line PROTO((char *, char *, METAENV *, ADDRESS *,
			     soutr_t, TCPSTREAM *, int, int));
char	  *prune_personal PROTO((ADDRESS *, char *));
int        post_rfc822_output PROTO ((char *, ENVELOPE *, BODY *, soutr_t,
				      TCPSTREAM *));
int        pine_rfc822_output PROTO ((METAENV *, BODY *, soutr_t,TCPSTREAM *));
long       pine_rfc822_output_body PROTO((BODY *,soutr_t,TCPSTREAM *));
void       pine_free_body PROTO((BODY **));
void       pine_free_body_data PROTO((BODY *));
void       pine_free_body_part PROTO((PART **));
long	   pine_smtp_verbose PROTO((SMTPSTREAM *));
void	   pine_smtp_verbose_out PROTO((char *));
int        call_mailer PROTO((METAENV *, BODY *));
int        news_poster PROTO((METAENV *, BODY *));
char      *tidy_smtp_mess PROTO((char *, char *, char *));
char      *mime_stats PROTO((BODY *));
void       mime_recur PROTO((BODY *));
int        open_fcc PROTO((char *, CONTEXT_S **, int));
long	   pine_pipe_soutr PROTO((void *, char *));
char	  *pine_pipe_getline PROTO((void *));
void	   pine_pipe_close PROTO((void *));
void	   pine_pipe_abort PROTO((void *));
int	   sent_percent PROTO(());
long	   send_body_size PROTO((BODY *));
void	   set_body_size PROTO((BODY *));
BODY	  *first_text_8bit PROTO((BODY *));
char	  *set_mime_charset PROTO((int, char *));


/*
 * Buffer to hold pointers into pine data that's needed by pico. 
 * Defined here so as not to take up room on the stack.  Better malloc'd?
 */
static	PICO	pbuf;


/* 
 * Storage object where the FCC (or postponed msg) is to be written.
 * This is amazingly bogus.  Much work was done to put messages 
 * together and encode them as they went to the tmp file for sendmail
 * or into the SMTP slot (especially for for DOS, to prevent a temporary
 * file (and needlessly copying the message).
 * 
 * HOWEVER, since there's no piping into c-client routines
 * (particularly mail_append() which copies the fcc), the fcc will have
 * to be copied to disk.  This global tells pine's copy of the rfc822
 * output functions where to also write the message bytes for the fcc.
 * With piping in the c-client we could just have two pipes to shove
 * down rather than messing with damn copies.  FIX THIS!
 *
 * The function open_fcc, locates the actual folder and creates it if
 * requested before any mailing or posting is done.
 */
static STORE_S *local_so      = NULL;
static int	local_written = 0;


/*
 * Locally global pointer to stream used for sending/posting.
 * It's also used to indicate when/if we write the Bcc: field in
 * the header.
 */
static SMTPSTREAM *sending_stream = NULL;
static struct hooks {
    void	*soutr;				/* Line output routine */
    void	*getline;			/* Line input routine */
    void	*close;				/* Stream close routine */
    void	*rfc822_out;			/* Message outputter */
} sending_hooks;
static FILE	  *verbose_send_output = NULL;
static long	   send_bytes_sent, send_bytes_to_send;
static char	  *sending_filter_requested;
static char	   verbose_requested, background_requested;
static METAENV	  *send_header = NULL;


#ifdef	DOS
#define	FCC_SOURCE	FileStar
#define	FCC_STREAM_MODE	OP_SHORTCACHE
#else
#define	FCC_SOURCE	CharStar
#define	FCC_STREAM_MODE	0L
#endif


#define	INTRPT_PMT \
	    "Continue INTERRUPTED composition (answering \"n\" won't erase it)"
#define	PSTPND_PMT \
	    "Continue postponed composition (answering \"No\" won't erase it)"
#define	POST_PMT   \
	    "Posted message may go to thousands of readers. Really post"
#define	INTR_DEL_FAIL	\
	   "Undelete messages to remain postponed, and then continue message"
#define	INTR_DEL_PMT	"Deleted messages will be removed from folder.  Delete"

/*
 * Since c-client preallocates, it's necessary here to define a limit
 * such that we don't blow up in c-client (see rfc822_address_line()).
 */
#define	MAX_SINGLE_ADDR	MAILTMPLEN


/*
 * Macros to help sort out posting results
 */
#define	P_MAIL_WIN	0x01
#define	P_MAIL_LOSE	0x02
#define	P_MAIL_BITS	0x03
#define	P_NEWS_WIN	0x04
#define	P_NEWS_LOSE	0x08
#define	P_NEWS_BITS	0x0C
#define	P_FCC_WIN	0x10
#define	P_FCC_LOSE	0x20
#define	P_FCC_BITS	0x30


#ifdef	DEBUG_PLUS
#define	TIME_STAMP(str, l)	{ \
				  struct timeval  tp; \
				  struct timezone tzp; \
				  if(gettimeofday(&tp, &tzp) == 0) \
				    dprint(l, (debugfile, \
					   "\nKACHUNK (%s) : %.8s.%3.3ld\n", \
					   str, ctime(&tp.tv_sec) + 11, \
					   tp.tv_usec));\
				}
#else
#define	TIME_STAMP(str, l)
#endif



/*----------------------------------------------------------------------
    Compose screen (not forward or reply). Set up envelope, call composer
  
   Args: pine_state -- The usual pine structure
 
  Little front end for the compose screen
  ---*/
void
compose_screen(pine_state)
    struct pine *pine_state;
{
    ps_global = pine_state;
    mailcap_free(); /* free resources we won't be using for a while */
    pine_state->next_screen = pine_state->prev_screen;
    compose_mail(NULL, NULL, NULL);
}



/*----------------------------------------------------------------------
     Format envelope for outgoing message and call editor

 Args:  given_to -- An address to send mail to (usually from command line 
                       invocation)
        fcc_arg  -- The fcc that goes with this address.
 
 If a "To" line is given format that into the envelope and get ready to call
           the composer
 If there's a message postponed, offer to continue it, and set it up,
 otherwise just fill in the outgoing envelope as blank.

 NOTE: we ignore postponed and interrupted messages in nr mode
 ----*/
void 
compose_mail(given_to, fcc_arg, inc_text_getc)
    char    *given_to,
	    *fcc_arg;
    gf_io_t  inc_text_getc;
{
    BODY      *body;
    ENVELOPE  *outgoing = NULL;
    PINEFIELD *custom   = NULL;
    char      *p,
	      *fcc_to_free,
	      *fcc      = NULL,
	      *refs     = NULL,
	      *lcc      = NULL,
	      *sig      = NULL;
    int        fcc_is_sticky = 0;

    dprint(1, (debugfile,
                 "\n\n    ---- COMPOSE SCREEN (not in pico yet) ----\n"));

    /*-- Check for INTERRUPTED mail  --*/
    if(!ps_global->anonymous && !given_to){
	char	     file_path[MAXPATH+1];
	int	     ret = 'n';

	/* build filename and see if it exists.  build_path creates
	 * an explicit local path name, so all c-client access is thru
	 * local drivers.
	 */
	file_path[0] = '\0';
	build_path(file_path,
		   ps_global->VAR_OPER_DIR ? ps_global->VAR_OPER_DIR
					   : ps_global->home_dir,
		   INTERRUPTED_MAIL);

	/* check to see if the folder exists, the user wants to continue
	 * and that we can actually read something in...
	 */
	if(folder_exists("[]", file_path) > 0
	   && (ret = want_to(INTRPT_PMT, 'y', 'x', NO_HELP, 0, 0)) == 'y'
	   && !redraft(NULL, file_path, &outgoing, &body, &fcc,
		       &refs, &lcc, &custom)){
	    return;
	}
	else if(ret == 'x'){
	    q_status_message(SM_ORDER, 0, 3, "Composition cancelled");
	    return;
	}
    }

    /*-- Check for postponed mail --*/
    if(!outgoing				/* not replying/forwarding */
       && !ps_global->anonymous			/* not special anon mode */
       && !given_to				/* not command line send */
       && ps_global->VAR_POSTPONED_FOLDER	/* folder to look in */
       && ps_global->VAR_POSTPONED_FOLDER[0]){
	CONTEXT_S   *p_cntxt;
	int	     ret = 'n';

	/* find default context to look for folder */
	if(!(p_cntxt = default_save_context(ps_global->context_list)))
	  p_cntxt = ps_global->context_list;

	/* check to see if the folder exists, the user wants to continue
	 * and that we can actually read something in...
	 */
	if(folder_exists(p_cntxt ? p_cntxt->context : "[]", 
			 ps_global->VAR_POSTPONED_FOLDER) > 0
	   && (ret = want_to(PSTPND_PMT, 'y', 'x', NO_HELP, 0, 0)) == 'y'
	   && !redraft(p_cntxt, ps_global->VAR_POSTPONED_FOLDER,
		       &outgoing, &body, &fcc, &refs, &lcc, &custom)){
	    return;
	}
	else if(ret == 'x'){
	    q_status_message(SM_ORDER, 0, 3, "Composition cancelled");
	    return;
	}
    }

    /*-- normal composition --*/
    if(!outgoing){
        /*=================  Compose new message ===============*/
        body         = mail_newbody();
        outgoing     = mail_newenvelope();

        if(given_to)
	  rfc822_parse_adrlist(&outgoing->to, given_to, ps_global->maildomain);

        outgoing->message_id = generate_message_id(ps_global);

	/*
	 * The type of storage object allocated below is vitally
	 * important.  See SIMPLIFYING ASSUMPTION #37
	 */
	if(body->contents.binary = (void *)so_get(PicoText,NULL,EDIT_ACCESS)){
	    char ch;

	    if(inc_text_getc){
		while((*inc_text_getc)(&ch))
		  if(!so_writec(ch, (STORE_S *)body->contents.binary)){
		      break;
		  }
	    }
	}
	else{
	    q_status_message(SM_ORDER | SM_DING, 3, 4,
			     "Problem creating space for message text.");
	    return;
	}

	if(sig = get_signature()){
	    if(*sig)
	      so_puts((STORE_S *)body->contents.binary, sig);

	    fs_give((void **)&sig);
	}

	body->type = TYPETEXT;
    }

    ps_global->prev_screen = compose_screen;
    if(!(fcc_to_free = fcc))		/* Didn't pick up fcc, use given  */
      fcc = fcc_arg;

    /*
     * check whether a build_address-produced fcc is different from
     * fcc.  If same, do nothing, if different, set sticky bit in pine_send.
     */
    if(fcc && outgoing->to){
	char *tmp_fcc;

	tmp_fcc = get_fcc_based_on_to(outgoing->to);
	if(strcmp(fcc, tmp_fcc ? tmp_fcc : ""))
	  fcc_is_sticky++;  /* cause sticky bit to get set */

	if(tmp_fcc)
	  fs_give((void **)&tmp_fcc);
    }

    pine_send(outgoing, &body, "COMPOSE MESSAGE", fcc, NULL, refs, NULL, lcc,
	      custom, fcc_is_sticky);

    if(fcc_to_free)
      fs_give((void **)&fcc_to_free);

    if(lcc)
      fs_give((void **)&lcc);

    mail_free_envelope(&outgoing);
    pine_free_body(&body);
}



/*----------------------------------------------------------------------
    Given context and folder, offer the context for redraft...

 Args:	cntxt -- 
	mbox -- 
	outgoing -- 
	body -- 
	fcc -- 
	custom -- 
 
 ----*/
int
redraft(cntxt, mbox, outgoing, body, fcc, ref_list, lcc, custom)
    CONTEXT_S  *cntxt;
    char       *mbox;
    ENVELOPE  **outgoing;
    BODY      **body;
    char      **fcc;
    char      **ref_list;
    char      **lcc;
    PINEFIELD **custom;
{
    MAILSTREAM	*stream;
    ENVELOPE	*e = NULL;
    BODY	*b;
    PART        *part;
    STORE_S	*so = NULL;
    PINEFIELD   *pf;
    gf_io_t	 pc;
    char	*extras, **fields, **values, *c_string, *p;
    long	 cont_msg = 1L;
    int		 i, j, rv = 0, pine_generated = 0;
#define	TMP_BUF1	(tmp_20k_buf)
#define	TMP_BUF2	(tmp_20k_buf + MAXPATH)

    /*
     * if we have a context AND we're looking at an ambiguous name, 
     * set the context string.  Otherwise, our context "relativeness"
     * code in context.c may cause the fully qualified name to still
     * get interpreted relative to the provided context...
     */
    c_string = (cntxt && context_isambig(mbox)) ? cntxt->context : "[]";

    /*
     * Need to check here to see if the folder being opened for redraft
     * is the currently opened folder.  If so, just use the current
     * mail_stream...
     */
    context_apply(TMP_BUF1, c_string, mbox);
    if(context_isambig(ps_global->cur_folder))
      context_apply(TMP_BUF2, ps_global->context_current->context,
		    ps_global->cur_folder);
    else
      strcpy(TMP_BUF2, ps_global->cur_folder);

    if(!strcmp(TMP_BUF1, TMP_BUF2)){
	stream = ps_global->mail_stream;
    }
    else{
	if((stream = context_open(c_string, NULL, mbox, FCC_STREAM_MODE))
	   && (extras = strindex(stream->mailbox, '<'))
	   && !strcmp(extras + 1, "no_mailbox>"))
	  stream = NULL;
    }

    if(stream){
	/*
	 * If the we're manipulating the current folder, don't bother
	 * with index
	 */
	if(!stream->nmsgs){
	    q_status_message(SM_ORDER | SM_DING, 3, 5,
			     "Empty folder.  No messages really postponed!");
	    redraft_cleanup(c_string, mbox, stream);
	    return(0);
	}
	else if(stream == ps_global->mail_stream){
	    /*
	     * Since the user's got this folder already opened and they're
	     * on a selected message, pick that one rather than rebuild
	     * another index screen...
	     */
	    cont_msg = mn_m2raw(ps_global->msgmap,
				mn_get_cur(ps_global->msgmap));
	}
	else if(stream->nmsgs > 1L){		/* offer browser ? */
	    MSGNO_S *msgmap = NULL;

	    mn_init(&msgmap, stream->nmsgs);
	    mn_set_cur(msgmap, 1L);

	    clear_index_cache();
	    while(1){
		rv = index_lister(ps_global, cntxt, mbox, stream, msgmap);
		cont_msg = mn_get_cur(msgmap);
		if(count_flagged(stream, "DELETED")
		   && want_to(INTR_DEL_PMT, 'n', 0, NO_HELP, 0, 0) == 'n'){
		    q_status_message(SM_ORDER, 3, 3, INTR_DEL_FAIL);
		    continue;
		}

		break;
	    }

	    clear_index_cache();
	    mn_give(&msgmap);

	    if(rv){
		redraft_cleanup(c_string, mbox, stream);
		q_status_message(SM_ORDER, 0, 3, "Composition cancelled");
		return(0);
	    }
	}

	if((e = mail_fetchstructure(stream, cont_msg, &b))
	   && (so = (void *)so_get(PicoText, NULL, EDIT_ACCESS))){
	    gf_set_so_writec(&pc, so);

	    *custom = get_dflt_custom_hdrs();
	    i       = (count_custom_hdrs() + 5) * sizeof(char *);

	    /*
	     * Having these two fields separated isn't the slickest, but
	     * converting the pointer array for fetchheader_lines() to
	     * a list of structures or some such for simple_header_parse()
	     * is too goonie.  We could do something like reuse c-client's
	     * PARAMETER struct which is a simple char * pairing, but that
	     * doesn't make sense to pass to fetchheader_lines()...
	     */
	    fields = (char **) fs_get((size_t) i * sizeof(char *));
	    values = (char **) fs_get((size_t) i * sizeof(char *));
	    memset(fields, 0, (size_t) i * sizeof(char *));
	    memset(values, 0, (size_t) i * sizeof(char *));

	    fields[i = 0] = "Fcc";		/* Fcc: special case */
	    fields[++i]   = "References";	/* References: too... */
	    fields[++i]   = "X-Post-Error";	/* posting errors too */
	    fields[++i]   = "Lcc";		/* Lcc: too... */

	    /*
	     * BOGUS ALERT!  A c-client DEFICIENCY doesn't give us the
	     * newsgroups if the folder's accessed via IMAP, so if
	     * fetchstructure says there aren't any, we need to go to 
	     * the trouble of checking ourselves.  Sheesh.
	     */
	    if(!e->newsgroups)
	      fields[++i] = "Newsgroups";

	    for(pf = *custom, ++i; pf && pf->name; pf = pf->next, i++)
	      fields[i] = pf->name;		/* assign custom fields */

	    if(extras = xmail_fetchheader_lines(stream, cont_msg, fields)){
		simple_header_parse(extras, fields, values);
		fs_give((void **)&extras);

		/* translate RFC 1522 strings */
		for(i = 3; fields[i]; i++)	/* starting with "Lcc" field */
		  if(values[i]){
		      char *charset;

		      p = (char *)rfc1522_decode((unsigned char*)tmp_20k_buf,
						 values[i], &charset);
		      if(charset)
			fs_give((void **)&charset);

		      if(p == tmp_20k_buf){
			  fs_give((void **)&values[i]);
			  values[i] = cpystr(p);
		      }
		  }

		for(pf = *custom, i = (e->newsgroups ? 4 : 5);
		    pf && pf->name;
		    pf = pf->next, i++){
		    if(values[i]){	/* use this instead of default */
			if(pf->textbuf)
			  fs_give((void **)&pf->textbuf);

			pf->textbuf = values[i]; /* freed in pine_send! */
		    }
		    else if(pf->textbuf)  /* was erased before postpone */
		      fs_give((void **)&pf->textbuf);
		}

		if(values[0])			/* If "Fcc:" was there...  */
		  pine_generated = 1;		/* we put it there? */

		if(fcc)				/* fcc: special case... */
		  *fcc = values[0] ? values[0] : cpystr("");

		if(ref_list)
		  *ref_list = values[1];

		if(values[2]){		/* x-post-error?!?1 */
		    q_status_message(SM_ORDER|SM_DING, 4, 4, values[2]);
		}

		if(lcc)
		  *lcc = values[3];

		if(!e->newsgroups && values[4])
		  e->newsgroups = values[4];
	    }

	    fs_give((void **)&fields);
	    fs_give((void **)&values);

	    *outgoing = copy_envelope(e);

	    /*
	     * Look at each empty address and see if the user has specified
	     * a default for that field or not.  If they have, that means
	     * they have erased it before postponing, so they won't want
	     * the default to come back.  If they haven't specified a default,
	     * then the default should be generated in pine_send.  We prevent
	     * the default from being assigned by assigning an empty address
	     * to the variable here.
	     *
	     * BUG: We should do this for custom Address headers, too, but
	     * there isn't such a thing yet.
	     *
	     * BUG: This doesn't work right for reply-to because c-client
	     * fills in reply-to even if there isn't one.
	     */
	    if(!(*outgoing)->to && hdr_has_custom_value("to"))
	      (*outgoing)->to = mail_newaddr();
	    if(!(*outgoing)->reply_to && hdr_has_custom_value("reply-to"))
	      (*outgoing)->reply_to = mail_newaddr();
	    if(!(*outgoing)->cc && hdr_has_custom_value("cc"))
	      (*outgoing)->cc = mail_newaddr();
	    if(!(*outgoing)->bcc && hdr_has_custom_value("bcc"))
	      (*outgoing)->bcc = mail_newaddr();

	    if(!(*outgoing)->subject && hdr_has_custom_value("subject"))
	      (*outgoing)->subject = cpystr("");

	    if(!pine_generated){
		/*
		 * Now, this is interesting.  We should have found 
		 * the "fcc:" field if pine wrote the message being
		 * redrafted.  Hence, we probably can't trust the 
		 * "originator" type fields, so we'll blast them and let
		 * them get set later in pine_send.  This should allow
		 * folks with custom or edited From's and such to still
		 * use redraft reasonably, without inadvertently sending
		 * messages that appear to be "From" others...
		 */
		if((*outgoing)->from)
		  mail_free_address(&(*outgoing)->from);

		/*
		 * Ditto for Reply-To and Sender...
		 */
		if((*outgoing)->reply_to)
		  mail_free_address(&(*outgoing)->reply_to);

		if((*outgoing)->sender)
		  mail_free_address(&(*outgoing)->sender);

		/*
		 * Generate a fresh message id for pretty much the same
		 * reason From and such got wacked...
		 */
		if((*outgoing)->message_id)
		  fs_give((void **)&(*outgoing)->message_id);

		(*outgoing)->message_id = generate_message_id(ps_global);
	    }

	    if(b && b->type != TYPETEXT){	/* already TYPEMULTIPART */
		*body			   = copy_body(NULL, b);
		part			   = (*body)->contents.part;
		part->body.type		   = TYPETEXT;
		part->body.contents.binary = (void *)so;
		get_body_part_text(stream, &b->contents.part->body,
				   cont_msg, "1", pc, "", NULL);

		if(!fetch_contents(stream, cont_msg, *body, *body))
		  q_status_message(SM_ORDER | SM_DING, 3, 4,
				   "Error including all message parts");
	    }
	    else{
		*body			 = mail_newbody();
		(*body)->type		 = TYPETEXT;
		(*body)->contents.binary = (void *)so;
		get_body_part_text(stream, b, cont_msg, "1", pc, "", NULL);
	    }

	    /* We have what we want, blast this message... */
	    if(!mail_elt(stream, cont_msg)->deleted)
	      mail_setflag(stream, long2string(cont_msg), "\\DELETED");

	    redraft_cleanup(c_string, mbox, stream);
	    rv = 1;
	}
	else
	  rv = 0;
    }

    return(rv);
}



/*----------------------------------------------------------------------
    Clear deleted messages from given stream and expunge if necessary

Args:	context -- 
	folder --
	stream -- 

 ----*/
void
redraft_cleanup(context, folder, stream)
    char       *context;
    char       *folder;
    MAILSTREAM *stream;
{
    if(stream->nmsgs){
	ps_global->expunge_in_progress = 1;
	mail_expunge(stream);			/* clean out deleted */
	ps_global->expunge_in_progress = 0;
    }

    if(stream->nmsgs){				/* close and return */
	if(stream != ps_global->mail_stream)
	  mail_close(stream);
    }
    else{					/* close and delete folder */
	if(stream == ps_global->mail_stream){
	    q_status_message1(SM_ORDER, 3, 7,
			     "No more postponed messages, returning to \"%s\"",
			     ps_global->inbox_name);
	    do_broach_folder(ps_global->inbox_name, ps_global->context_list);
	    ps_global->next_screen = mail_index_screen;
	}
	else
	  mail_close(stream);

	stream = context_same_stream(context, folder, ps_global->mail_stream);
	if(!stream && ps_global->mail_stream != ps_global->inbox_stream)
	  stream = context_same_stream(context, folder,
				       ps_global->inbox_stream);

	context_delete(context, stream, folder);
    }
}



/*----------------------------------------------------------------------
    Parse the given header text for any given fields

Args:  text -- Text to parse for fcc and attachments refs
       fields -- array of field names to look for
       values -- array of pointer to save values to, returned NULL if
                 fields isn't in text.

This function simply looks for the given fields in the given header
text string.
NOTE: newlines are expected CRLF, and we'll ignore continuations
 ----*/
void
simple_header_parse(text, fields, values)
    char   *text, **fields, **values;
{
    int   i, n;
    char *p, *t;

    for(i = 0; fields[i]; i++)
      values[i] = NULL;				/* clear values array */

    /*---- Loop until the end of the header ----*/
    for(p = text; *p; ){
	for(i = 0; fields[i]; i++)		/* find matching field? */
	  if(!struncmp(p, fields[i], (n=strlen(fields[i]))) && p[n] == ':'){
	      for(p += n + 1; *p; p++){		/* find start of value */
		  if(*p == '\015' && *(p+1) == '\012'
		     && !isspace((unsigned char) *(p+2)))
		    break;

		  if(!isspace((unsigned char) *p))
		    break;			/* order here is key... */
	      }

	      if(!values[i]){			/* if we haven't already */
		  values[i] = fs_get(strlen(text) + 1);
		  values[i][0] = '\0';		/* alloc space for it */
	      }

	      if(*p && *p != '\015'){		/* non-blank value. */
		  t = values[i] + (values[i][0] ? strlen(values[i]) : 0);
		  while(*p){			/* check for cont'n lines */
		      if(*p == '\015' && *(p+1) == '\012'){
			  if(isspace((unsigned char) *(p+2))){
			      p += 3;
			      continue;
			  }
			  else
			    break;
		      }

		      *t++ = *p++;
		  }

		  *t = '\0';
	      }

	      break;
	  }

        /* Skip to end of line, what ever it was */
        for(; *p ; p++)
	  if(*p == '\015' && *(p+1) == '\012'){
	      p += 2;
	      break;
	  }
    }
}

 

#if defined(DOS) || defined(OS2)
/*----------------------------------------------------------------------
    Verify that the necessary pieces are around to allow for
    message sending under DOS

Args:  strict -- tells us if a remote stream is required before
		 sending is permitted.

The idea is to make sure pine knows enough to put together a valid 
from line.  The things we MUST know are a user-id, user-domain and
smtp server to dump the message off on.  Typically these are 
provided in pine's configuration file, but if not, the user is
queried here.
 ----*/
int
dos_valid_from(strict)
    int strict;
{
    char        prompt[80], answer[60], *p;
    int         rc, i;
    HelpType    help;

    /*
     * If we're told to, require that a remote stream exist before
     * permitting mail to get sent.  Someday this will be removed and
     * sent mail sans a stream will get stuffed into an "outbox"...
     */
    if(strict && (!(ps_global->mail_stream
		    && !ps_global->mail_stream->anonymous
		    && mail_valid_net(ps_global->mail_stream->mailbox,
				      ps_global->mail_stream->dtb,NULL,NULL))
		  && !(ps_global->inbox_stream
		       && ps_global->inbox_stream != ps_global->mail_stream
		       && !ps_global->inbox_stream->anonymous
		       && mail_valid_net(ps_global->inbox_stream->mailbox,
				    ps_global->inbox_stream->dtb,NULL,NULL)))){
	q_status_message(SM_ORDER, 3, 7,
			 "Can't send message without an open remote folder");
	return(0);
    }

    /*
     * query for user name portion of address, use IMAP login
     * name as default
     */
    if(!ps_global->VAR_USER_ID || ps_global->VAR_USER_ID[0] == '\0'){
	if(ps_global->mail_stream
	   && ps_global->mail_stream->dtb
	   && ps_global->mail_stream->dtb->name
	   && !strncmp(ps_global->mail_stream->dtb->name, "imap", 4)
	   && (p = imap_user(ps_global->mail_stream)))
	  strcpy(answer, p);
	else
	  answer[0] = '\0';

	sprintf(prompt,"User-id for From address : "); 

	help = NO_HELP;
	while(1) {
	    rc = optionally_enter(answer,-FOOTER_ROWS(ps_global),0,79,
				    1,0,prompt,NULL,help,0);
	    if(rc == 2)
	      continue;

	    if(rc == 3){
		help = (help == NO_HELP) ? h_sticky_user_id : NO_HELP;
		continue;
	    }

	    if(rc != 4)
	      break;
	}

	if(rc == 1 || (rc == 0 && !answer)) {
	    q_status_message(SM_ORDER, 3, 4,
		   "Send cancelled (User-id must be provided before sending)");
	    return(0);
	}

	/* save the name */
	sprintf(prompt, "Preserve %s as \"user-id\" in PINERC", answer);
	if(ps_global->blank_user_id
	   && want_to(prompt, 'y', 'n', NO_HELP, 0, 0) == 'y'){
	    set_variable(V_USER_ID, answer, 1);
	}
	else{
            fs_give((void **)&(ps_global->VAR_USER_ID));
	    ps_global->VAR_USER_ID = cpystr(answer);
	}
    }

    /* query for personal name */
    if(!ps_global->VAR_PERSONAL_NAME || ps_global->VAR_PERSONAL_NAME[0]=='\0'){
	answer[0] = '\0';
	sprintf(prompt,"Personal name for From address : "); 

	help = NO_HELP;
	while(1) {
	    rc = optionally_enter(answer,-FOOTER_ROWS(ps_global),0,79,
				    1,0,prompt,NULL,help,0);
	    if(rc == 2)
	      continue;

	    if(rc == 3){
		help = (help == NO_HELP) ? h_sticky_personal_name : NO_HELP;
		continue;
	    }

	    if(rc != 4)
	      break;
    	}

	if(rc == 0 && answer){		/* save the name */
	    sprintf(prompt, "Preserve %s as \"personal-name\" in PINERC",
		    answer);
	    if(ps_global->blank_personal_name
	       && want_to(prompt, 'y', 'n', NO_HELP, 0, 0) == 'y'){
		set_variable(V_PERSONAL_NAME, answer, 1);
	    }
	    else{
        	fs_give((void **)&(ps_global->VAR_PERSONAL_NAME));
		ps_global->VAR_PERSONAL_NAME = cpystr(answer);
	    }
	}
    }

    /* 
     * query for host/domain portion of address, using IMAP
     * host as default
     */
    if(ps_global->blank_user_domain 
       || ps_global->maildomain == ps_global->localdomain
       || ps_global->maildomain == ps_global->hostname){
	if(ps_global->inbox_name[0] == '{'){
	    for(i=0;ps_global->inbox_name[i+1] != '}'; i++)
		answer[i] = ps_global->inbox_name[i+1];

	    answer[i] = '\0';
	}
	else
	  answer[0] = '\0';

	sprintf(prompt,"Host/domain for From address : "); 

	help = NO_HELP;
	while(1) {
	    rc = optionally_enter(answer,-FOOTER_ROWS(ps_global),0,79,
				    1,0,prompt,NULL,help,0);
	    if(rc == 2)
	      continue;

	    if(rc == 3){
		help = (help == NO_HELP) ? h_sticky_domain : NO_HELP;
		continue;
	    }

	    if(rc != 4)
	      break;
	}

	if(rc == 1 || (rc == 0 && !answer)) {
	    q_status_message(SM_ORDER, 3, 4,
	  "Send cancelled (Host/domain name must be provided before sending)");
	    return(0);
	}

	/* save the name */
	sprintf(prompt, "Preserve %s as \"user-domain\" in PINERC",
		answer);
	if(!ps_global->userdomain && !ps_global->blank_user_domain
	   && want_to(prompt, 'y', 'n', NO_HELP, 0, 0) == 'y'){
	    set_variable(V_USER_DOMAIN, answer, 1);
	    fs_give((void **)&(ps_global->maildomain));	/* blast old val */
	    ps_global->userdomain      = cpystr(answer);
	    ps_global->maildomain      = ps_global->userdomain;
	}
	else{
            fs_give((void **)&(ps_global->maildomain));
            ps_global->userdomain = cpystr(answer);
	    ps_global->maildomain = ps_global->userdomain;
	}
    }

    /* check for smtp server */
    if(!ps_global->VAR_SMTP_SERVER ||
       !ps_global->VAR_SMTP_SERVER[0] ||
       !ps_global->VAR_SMTP_SERVER[0][0]){
	char **list;

	if(ps_global->inbox_name[0] == '{'){
	    for(i=0;ps_global->inbox_name[i+1] != '}'; i++)
	      answer[i] = ps_global->inbox_name[i+1];
	    answer[i] = '\0';
	}
	else
          answer[0] = '\0';

        sprintf(prompt,"SMTP server to forward message : "); 

	help = NO_HELP;
        while(1) {
            rc = optionally_enter(answer,-FOOTER_ROWS(ps_global),0,79,
				    1,0,prompt,NULL,help,0);
            if(rc == 2)
                  continue;

	    if(rc == 3){
		help = (help == NO_HELP) ? h_sticky_smtp : NO_HELP;
		continue;
	    }

            if(rc != 4)
                  break;
        }

        if(rc == 1 || (rc == 0 && answer[0] == '\0')) {
            q_status_message(SM_ORDER, 3, 4,
	       "Send cancelled (SMTP server must be provided before sending)");
            return(0);
        }

	/* save the name */
        list    = (char **) fs_get(2 * sizeof(char *));
	list[0] = cpystr(answer);
	list[1] = NULL;
	set_variable_list(V_SMTP_SERVER, list);
	fs_give((void *)&list[0]);
	fs_give((void *)list);
    }

    return(1);
}
#endif


/* this is for initializing the fixed header elements in pine_send() */
/*
prompt::name::help::prlen::maxlen::realaddr::
builder::affected_entry::next_affected::selector::key_label::
display_it::break_on_comma::is_attach::rich_header::only_file_chars::
single_space::sticky::dirty::start_here::KS_ODATAVAR
*/
static struct headerentry he_template[]={
  {"From    : ",  "From",        h_composer_from,       10, 0, NULL,
   build_address, NULL, NULL, addr_book_compose,    "To AddrBk",
   0, 1, 0, 1, 0, 1, 0, 0, 0, KS_TOADDRBOOK},
  {"Reply-To: ",  "Reply To",    h_composer_reply_to,   10, 0, NULL,
   build_address, NULL, NULL, addr_book_compose,    "To AddrBk",
   0, 1, 0, 1, 0, 1, 0, 0, 0, KS_TOADDRBOOK},
  {"To      : ",  "To",          h_composer_to,         10, 0, NULL,
   build_address, NULL, NULL, addr_book_compose,    "To AddrBk",
   0, 1, 0, 0, 0, 1, 0, 0, 0, KS_TOADDRBOOK},
  {"Cc      : ",  "Cc",          h_composer_cc,         10, 0, NULL,
   build_address, NULL, NULL, addr_book_compose,    "To AddrBk",
   0, 1, 0, 0, 0, 1, 0, 0, 0, KS_TOADDRBOOK},
  {"Bcc     : ",  "Bcc",         h_composer_bcc,        10, 0, NULL,
   build_address, NULL, NULL, addr_book_compose,    "To AddrBk",
   0, 1, 0, 1, 0, 1, 0, 0, 0, KS_TOADDRBOOK},
  {"Newsgrps: ",  "Newsgroups",  h_composer_news,        10, 0, NULL,
   news_build,    NULL, NULL, news_group_selector,  "To NwsGrps",
   0, 1, 0, 1, 0, 1, 0, 0, 0, KS_NONE},
  {"Fcc     : ",  "Fcc",         h_composer_fcc,        10, 0, NULL,
   NULL,          NULL, NULL, folders_for_fcc,      "To Fldrs",
   0, 0, 0, 1, 1, 1, 0, 0, 0, KS_NONE},
  {"Lcc     : ",  "Lcc",         h_composer_lcc,        10, 0, NULL,
   build_addr_lcc, NULL, NULL, addr_book_compose_lcc,"To AddrBk",
   0, 1, 0, 1, 0, 1, 0, 0, 0, KS_NONE},
  {"Attchmnt: ",  "Attchmnt",    h_composer_attachment, 10, 0, NULL,
   NULL,          NULL, NULL, NULL,                 "To Files",
   0, 1, 1, 0, 0, 1, 0, 0, 0, KS_NONE},
  {"Subject : ",  "Subject",     h_composer_subject,    10, 0, NULL,
   valid_subject, NULL, NULL, NULL,                 NULL,
   0, 0, 0, 0, 0, 0, 0, 0, 0, KS_NONE},
  {"",            "References",  NO_HELP,               10, 0, NULL,
   NULL,          NULL, NULL, NULL,                 NULL,
   0, 0, 0, 0, 0, 0, 0, 0, 0, KS_NONE},
  {"",            "Date",        NO_HELP,               10, 0, NULL,
   NULL,          NULL, NULL, NULL,                 NULL,
   0, 0, 0, 0, 0, 0, 0, 0, 0, KS_NONE},
  {"",            "In-Reply-To", NO_HELP,               10, 0, NULL,
   NULL,          NULL, NULL, NULL,                 NULL,
   0, 0, 0, 0, 0, 0, 0, 0, 0, KS_NONE},
  {"",            "Message-ID",  NO_HELP,               10, 0, NULL,
   NULL,          NULL, NULL, NULL,                 NULL,
   0, 0, 0, 0, 0, 0, 0, 0, 0, KS_NONE},
  {"",            "To",          NO_HELP,               10, 0, NULL,
   NULL,          NULL, NULL, NULL,                 NULL,
   0, 0, 0, 0, 0, 0, 0, 0, 0, KS_NONE},
  {"",            "X-Post-Error",NO_HELP,               10, 0, NULL,
   NULL,          NULL, NULL, NULL,                 NULL,
   0, 0, 0, 0, 0, 0, 0, 0, 0, KS_NONE},
#if	!(defined(DOS) || defined(OS2)) || defined(NOAUTH)
  {"",            "Sender",      NO_HELP,               10, 0, NULL,
   NULL,          NULL, NULL, NULL,                 NULL,
   0, 0, 0, 0, 0, 0, 0, 0, 0, KS_NONE}
#endif
};

static struct headerentry he_custom_addr_templ={
   NULL,          NULL,          h_composer_custom_addr,10, 0, NULL,
   build_address, NULL, NULL, addr_book_compose,    "To AddrBk",
   0, 1, 0, 1, 0, 1, 0, 0, 0, KS_TOADDRBOOK};
static struct headerentry he_custom_free_templ={
   NULL,          NULL,          h_composer_custom_free,10, 0, NULL,
   NULL,          NULL, NULL, NULL,                 NULL,
   0, 0, 0, 1, 0, 0, 0, 0, 0, KS_NONE};

/*
 * Note, these are in the same order in the he_template and pf_template arrays.
 */
#define N_FROM    0
#define N_REPLYTO 1
#define N_TO      2
#define N_CC      3
#define N_BCC     4
#define N_NEWS    5
#define N_FCC     6
#define N_LCC     7
#define N_ATTCH   8
#define N_SUBJ    9
#define N_REF     10
#define N_DATE    11
#define N_INREPLY 12
#define N_MSGID   13
#define N_NOBODY  14
#define	N_POSTERR 15
#define N_SENDER  16

/* this is used in pine_send and pine_simple_send */
/* name::type::canedit::writehdr::localcopy::rcptto */
static PINEFIELD pf_template[] = {
  {"From",        Address,	0, 1, 1, 0},
  {"Reply-To",    Address,	0, 1, 1, 0},
  {"To",          Address,	1, 1, 1, 1},
  {"cc",          Address,	1, 1, 1, 1},
  {"bcc",         Address,	1, 0, 1, 1},
  {"Newsgroups",  FreeText,	1, 1, 1, 0},
  {"Fcc",         Fcc,		1, 0, 0, 0},
  {"Lcc",         Address,	1, 0, 1, 1},
  {"Attchmnt",    Attachment,	1, 1, 1, 0},
  {"Subject",     Subject,	1, 1, 1, 0},
  {"References",  FreeText,	0, 0, 0, 0},
  {"Date",        FreeText,	0, 1, 1, 0},
  {"In-Reply-To", FreeText,	0, 1, 1, 0},
  {"Message-ID",  FreeText,	0, 1, 1, 0},
  {"To",          Address,	0, 0, 0, 0},	/* N_NOBODY */
  {"X-Post-Error",FreeText,	0, 0, 0, 0},	/* N_POSTERR */
#if	!(defined(DOS) || defined(OS2)) || defined(NOAUTH)
  {"X-Sender",    Address,	0, 1, 1, 0},
#endif
  {NULL,         FreeText}
};


/*----------------------------------------------------------------------
     Get addressee for message, then post message

  Args:  outgoing -- Partially formatted outgoing ENVELOPE
         body     -- Body of outgoing message
        prmpt_who -- Optional prompt for optionally_enter call
        prmpt_cnf -- Optional prompt for confirmation call
    used_tobufval -- The string that the to was eventually set equal to.
		      This gets passed back if non-NULL on entry.
    prompt_for_to -- Allow user to change recipient

  Result: message "To: " field is provided and message is sent or cancelled.

  Fields:
     remail                -
     return_path           -
     date                 added here
     from                 added here
     sender                -
     reply_to              -
     subject              passed in, NOT edited but maybe canonized here
     to                   possibly passed in, edited and canonized here
     cc                    -
     bcc                   -
     in_reply_to           -
     message_id            -
  
Can only deal with message bodies that are either TYPETEXT or TYPEMULTIPART
with the first part TYPETEXT! All newlines in the text here also end with
CRLF.

Returns 0 on success, -1 on failure.
  ----*/
int
pine_simple_send(outgoing, body, prmpt_who, prmpt_cnf, used_tobufval,
		 prompt_for_to)
    ENVELOPE  *outgoing;  /* envelope for outgoing message */
    BODY     **body;   
    char      *prmpt_who, *prmpt_cnf;
    char     **used_tobufval;
    int        prompt_for_to;
{
    char     **tobufp, *p;
    void      *messagebuf;
    int        done = 0, retval = 0;
    int        cnt, i, resize_len, result;
    PINEFIELD *pfields, *pf, **sending_order;
    METAENV    header;
    HelpType   help;
    ESCKEY_S   ekey[2];
    BUILDER_ARG ba_fcc;

    dprint(1, (debugfile,"\n === simple send called === \n"));

    ba_fcc.tptr = NULL; ba_fcc.next = NULL; ba_fcc.xtra = NULL;

    /* how many fields are there? */
    cnt = (sizeof(pf_template)/sizeof(pf_template[0])) - 1;

    /* temporary PINEFIELD array */
    i = (cnt + 1) * sizeof(PINEFIELD);
    pfields = (PINEFIELD *)fs_get((size_t) i);
    memset(pfields, 0, (size_t) i);

    i             = (cnt + 1) * sizeof(PINEFIELD *);
    sending_order = (PINEFIELD **)fs_get((size_t) i);
    memset(sending_order, 0, (size_t) i);

    header.env           = outgoing;
    header.local         = pfields;
    header.custom        = NULL;
    header.sending_order = sending_order;

    /*----- Fill in a few general parts of the envelope ----*/
    if(!outgoing->date){
	rfc822_date(tmp_20k_buf);		/* format and copy new date */
	outgoing->date = cpystr(tmp_20k_buf);
    }

    outgoing->from	  = generate_from();
    outgoing->return_path = rfc822_cpy_adr(outgoing->from);

#if	!(defined(DOS) || defined(OS2)) || defined(NOAUTH)
#define NN 3
#else
#define NN 2
#endif
    /* initialize pfield */
    pf = pfields;
    for(i=0; i < cnt; i++, pf++){

        pf->name        = cpystr(pf_template[i].name);
	if(i == N_SENDER && F_ON(F_USE_SENDER_NOT_X, ps_global))
	  /* slide string over so it is Sender instead of X-Sender */
	  for(p=pf->name; *(p+1); p++)
	    *p = *(p+2);

        pf->type        = pf_template[i].type;
	pf->canedit     = pf_template[i].canedit;
	pf->rcptto      = pf_template[i].rcptto;
	pf->writehdr    = pf_template[i].writehdr;
	pf->localcopy   = pf_template[i].localcopy;
        pf->he          = NULL; /* unused */
        pf->next        = pf + 1;

        switch(pf->type){
          case FreeText:
            switch(i){
              case N_NEWS:
		pf->text		= &outgoing->newsgroups;
		sending_order[0]	= pf;
                break;

              case N_DATE:
		pf->text		= &outgoing->date;
		sending_order[1]	= pf;
                break;

              case N_INREPLY:
		pf->text		= &outgoing->in_reply_to;
		sending_order[NN+9]	= pf;
                break;

              case N_MSGID:
		pf->text		= &outgoing->message_id;
		sending_order[NN+10]	= pf;
                break;

              case N_REF: /* won't be used here */
		sending_order[NN+11]	= pf;
                break;

              case N_POSTERR: /* won't be used here */
		sending_order[NN+12]	= pf;
                break;

              default:
                q_status_message1(SM_ORDER,3,3,
		    "Internal error: 1)FreeText header %d", (void *)i);
                break;
            }

            break;

          case Attachment:
            break;

          case Address:
            switch(i){
	      case N_FROM:
		sending_order[2]	= pf;
		pf->addr		= &outgoing->from;
		break;

	      case N_TO:
		sending_order[NN+2]	= pf;
		pf->addr		= &outgoing->to;
	        tobufp			= &pf->scratch;
		break;

	      case N_CC:
		sending_order[NN+3]	= pf;
		pf->addr		= &outgoing->cc;
		break;

	      case N_BCC:
		sending_order[NN+4]	= pf;
		pf->addr		= &outgoing->bcc;
		break;

	      case N_REPLYTO:
		sending_order[NN+1]	= pf;
		pf->addr		= &outgoing->reply_to;
		break;

	      case N_LCC:		/* won't be used here */
		sending_order[NN+7]	= pf;
		break;

#if	!(defined(DOS) || defined(OS2)) || defined(NOAUTH)
              case N_SENDER:
		sending_order[3]	= pf;
		pf->addr		= &outgoing->sender;
                break;
#endif

              case N_NOBODY: /* won't be used here */
		sending_order[NN+5]	= pf;
                break;

              default:
                q_status_message1(SM_ORDER,3,3,
		    "Internal error: Address header %d", (void *) i);
                break;
            }
            break;

          case Fcc:
	    sending_order[NN+8] = pf;
	    ba_fcc.tptr            = NULL;
	    ba_fcc.next            = NULL;
	    pf->text		= &ba_fcc.tptr;
            break;

	  case Subject:
	    sending_order[NN+6]	= pf;
	    pf->text		= &outgoing->subject;
	    break;

          default:
            q_status_message1(SM_ORDER,3,3,
		"Unknown header type %d in pine_simple_send", (void *)pf->type);
            break;
        }
    }

    pf->next = NULL;

    ekey[0].ch    = ctrl('T');
    ekey[0].rval  = 2;
    ekey[0].name  = "^T";
    ekey[0].label = "To AddrBk";
    ekey[1].ch    = -1;

    /*----------------------------------------------------------------------
       Loop editing the "To: " field until everything goes well
     ----*/
    help = NO_HELP;

    while(!done){
	outgoing2strings(&header, *body, &messagebuf, NULL);

	resize_len = max(MAXPATH, strlen(*tobufp));
	fs_resize((void **)tobufp, resize_len+1);

	if(prompt_for_to)
	  i = optionally_enter(*tobufp, -FOOTER_ROWS(ps_global),
				0, resize_len, 1, 0,
				prmpt_who
				  ? prmpt_who
				  : outgoing->remail == NULL 
				    ? "FORWARD (as e-mail) to : "
				    : "BOUNCE (redirect) message to : ",
				ekey, help, 0);
	else
	  i = 0;

	switch(i){
	  case -1:
	    q_status_message(SM_ORDER | SM_DING, 3, 4,
			     "Internal problem encountered");
	    retval = -1;
	    done++;
	    break;

	  case 0:
	    if(**tobufp != '\0'){
		char *errbuf, *addr, prompt[200];
		int   tolen, ch;

		errbuf = NULL;
		ba_fcc.tptr = NULL;
		ba_fcc.next = NULL;
		if(build_address(*tobufp, &addr, &errbuf, &ba_fcc) >= 0){
		    if(errbuf)
		      fs_give((void **)&errbuf);

		    if(strlen(*tobufp) < (tolen = strlen(addr) + 1))
		      fs_resize((void **)tobufp, (size_t) tolen);

		    strcpy(*tobufp, addr);
		    if(used_tobufval)
		      *used_tobufval = cpystr(addr);

		    sprintf(prompt, "%s\"%.50s\"",
			prmpt_cnf ? prmpt_cnf : "Send message to ",
			(addr && *addr)
			    ? addr
			    : (F_ON(F_FCC_ON_BOUNCE, ps_global)
			       && ba_fcc.tptr && ba_fcc.tptr[0])
				? ba_fcc.tptr
				: "");
		    strings2outgoing(&header, body, NULL);
		    if(addr)
		      fs_give((void **)&addr);

		    if(prompt_for_to && check_addresses(&header))
		      /*--- Addresses didn't check out---*/
		      continue;

		    /* confirm address */
		    if(!prompt_for_to
		     || (ch = want_to(prompt, 'y', 'n', NO_HELP, 0, 0)) == 'y'){
			char *fcc = NULL;
			CONTEXT_S *fcc_cntxt = NULL;

			if(F_ON(F_FCC_ON_BOUNCE, ps_global)){
			    fcc = ba_fcc.tptr;
			    set_last_fcc(fcc);
			}

			/*---- Check out fcc -----*/
			if(fcc && *fcc){
			    local_written = 0;
			    if(!open_fcc(fcc, &fcc_cntxt, 0)
		   || !(local_so = so_get(FCC_SOURCE, NULL, WRITE_ACCESS))){
				/* Open or allocation of fcc failed */
				q_status_message(SM_ORDER, 3, 5,
					 "Message NOT sent or written to fcc.");
				dprint(4, (debugfile,"can't open fcc, cont\n"));
				if(!prompt_for_to){
				    retval = -1;
				    goto finish;
				}
				else
				  continue;
			    }
			}
			else
			  local_so = NULL;

			if(!(outgoing->to || outgoing->cc || outgoing->bcc
			     || local_so)){
			    q_status_message(SM_ORDER, 3, 5,
				"No recipients specified!");
			    continue;
			}

			if(outgoing->to || outgoing->cc || outgoing->bcc)
			  result = call_mailer(&header, *body);
			else
			  result = 0;

			/*----- Was there an fcc involved? -----*/
			if(local_so){
			    if(result == 1
			       || (result == 0
			   && pine_rfc822_output(&header, *body, NULL, NULL))){
				char label[50];

				strcpy(label, "Fcc");
				if(strcmp(fcc, ps_global->VAR_DEFAULT_FCC))
				  sprintf(label + 3, " to %.40s", fcc);

				/* Now actually copy to fcc folder and close */
				write_fcc(fcc, fcc_cntxt, local_so, label);
			    }
			    else if(result == 0){
				q_status_message(SM_ORDER,3,5,
				    "Fcc Failed!.  No message saved.");
				retval = -1;
				dprint(1,
				  (debugfile, "explicit fcc write failed!\n"));
			    }

			    so_give(&local_so);
			}

			if(result < 0){
			    dprint(1, (debugfile, "Bounce failed\n"));
			    if(!prompt_for_to)
			      retval = -1;
			    else
			      continue;
			}
		    }
		    else{
			q_status_message(SM_ORDER, 0, 3, "Send cancelled");
			retval = -1;
		    }
		}
		else{
		    q_status_message1(SM_ORDER | SM_DING, 3, 5,
				      "Error in address: %s", errbuf);
		    if(errbuf)
		      fs_give((void **)&errbuf);

		    if(!prompt_for_to)
		      retval = -1;
		    else
		      continue;
		}

	    }
	    else{
		q_status_message(SM_ORDER | SM_DING, 3, 5,
				 "No addressee!  No e-mail sent.");
		retval = -1;
	    }

	    done++;
	    break;

	  case 1:
	    q_status_message(SM_ORDER, 0, 3, "Send cancelled");
	    done++;
	    retval = -1;
	    break;

	  case 2: /* ^T */
	    {
	    void (*redraw) () = ps_global->redrawer;
	    char  *returned_addr = NULL;
	    int    len;

	    push_titlebar_state();
	    returned_addr = addr_book_bounce();
	    if(returned_addr){
		if(resize_len < (len = strlen(returned_addr) + 1))
		  fs_resize((void **)tobufp, (size_t)len);

		strcpy(*tobufp, returned_addr);
		fs_give((void **)&returned_addr);
		strings2outgoing(&header, body, NULL);
	    }

	    ClearScreen();
	    pop_titlebar_state();
	    redraw_titlebar();
	    if(ps_global->redrawer = redraw) /* reset old value, and test */
	      (*ps_global->redrawer)();
	    }

	    break;

	  case 3:
	    help = (help == NO_HELP)
			    ? (outgoing->remail == NULL
				? h_anon_forward
				: h_bounce)
			    : NO_HELP;
	    break;

	  case 4:				/* can't suspend */
	  default:
	    break;
	}
    }

finish:
    if(ba_fcc.tptr)
      fs_give((void **)&ba_fcc.tptr);

    for(i=0; i < cnt; i++)
      fs_give((void **)&pfields[i].name);

    fs_give((void **)&pfields);
    fs_give((void **)&sending_order);

    return(retval);
}


/*----------------------------------------------------------------------
     Prepare data structures for pico, call pico, then post message

  Args:  outgoing     -- Partially formatted outgoing ENVELOPE
         body         -- Body of outgoing message
         editor_title -- Title for anchor line in composer
         fcc          -- The file carbon copy field
	 reply_list   -- List of raw c-client message numbers of
			 we're replying to.  This should be GIDs
			 after IMAP4.
	 ref_list     -- News references list of message id's
	 custom -- custom header list. Passed void * so routines outside
		   send.c don't need to know what PINEFIELD is...

  Result: message is edited, then postponed, cancelled or sent.

  Fields:
     remail                -
     return_path           -
     date                 added here
     from                 added here
     sender                -
     reply_to              -
     subject              passed in, edited and cannonized here
     to                   possibly passed in, edited and cannonized here
     cc                   possibly passed in, edited and cannonized here
     bcc                  edited and cannonized here
     in_reply_to          generated in reply() and passed in
     message_id            -
  
 Storage for these fields comes from anywhere outside. It is remalloced
 here so the composer can realloc them if needed. The copies here are also 
 freed here.

Can only deal with message bodies that are either TYPETEXT or TYPEMULTIPART
with the first part TYPETEXT! All newlines in the text here also end with
CRLF.

There's a further assumption that the text in the TYPETEXT part is 
stored in a storage object (see filter.c).
  ----*/
void
pine_send(outgoing, body, editor_title, fcc_arg, reply_list, ref_list,
	  reply_prefix, lcc_arg, custom, sticky_fcc)
    ENVELOPE  *outgoing;  /* c-client envelope for outgoing message */
    BODY     **body;   
    char      *editor_title;
    char      *fcc_arg;
    long      *reply_list;
    char      *ref_list;
    char      *reply_prefix;
    char      *lcc_arg;
    void      *custom;
    int        sticky_fcc;
{
    int			i, fixed_cnt, total_cnt, index,
			editor_result = 0, body_start = 0;
    char	       *p, *addr, *fcc, *fcc_to_free = NULL;
    struct headerentry *he, *headents, *he_to, *he_fcc, *he_news, *he_lcc;
    PINEFIELD          *pfields, *pf, *pf_nobody, *pf_ref, *pf_fcc, *pf_err,
		      **sending_order;
    METAENV             header;
    ADDRESS            *lcc_addr = NULL;
    ADDRESS            *nobody_addr = NULL;
    STORE_S	       *orig_so = NULL;
#ifdef	DOS
    char               *reserve;
#endif

    dprint(1, (debugfile,"\n=== send called ===\n"));

    /*
     * Cancel any pending initial commands since pico uses a different
     * input routine.  If we didn't cancel them, they would happen after
     * we returned from the editor, which would be confusing.
     */
    if(ps_global->in_init_seq){
	ps_global->in_init_seq = 0;
	ps_global->save_in_init_seq = 0;
	clear_cursor_pos();
	if(ps_global->initial_cmds){
	    if(ps_global->free_initial_cmds)
	      fs_give((void **)&(ps_global->free_initial_cmds));

	    ps_global->initial_cmds = 0;
	}

	F_SET(F_USE_FK,ps_global,ps_global->orig_use_fkeys);
    }

    if(ps_global->post){
	q_status_message(SM_ORDER|SM_DING, 3, 3, "Can't post while posting!");
	return;
    }

#if defined(DOS) || defined(OS2)
    if(!dos_valid_from(1))
      return;

    pbuf.upload	       = NULL;
#else
    pbuf.upload	       = (ps_global->VAR_UPLOAD_CMD
			  && ps_global->VAR_UPLOAD_CMD[0])
			  ? upload_msg_to_pico : NULL;
#endif

    pbuf.raw_io        = Raw;
    pbuf.showmsg       = display_message_for_pico;
    pbuf.newmail       = new_mail_for_pico;
    pbuf.msgntext      = message_format_for_pico;
    pbuf.ckptdir       = checkpoint_dir_for_pico;
    pbuf.mimetype      = mime_type_for_pico;
    pbuf.exittest      = send_exit_for_pico;
    pbuf.resize	       = resize_for_pico;
    pbuf.keybinit      = init_keyboard;
    pbuf.helper        = helper;
    pbuf.alt_ed        = (ps_global->VAR_EDITOR && ps_global->VAR_EDITOR[0])
				? ps_global->VAR_EDITOR : NULL;
    pbuf.alt_spell     = ps_global->VAR_SPELLER;
    pbuf.quote_str     = reply_prefix;
    pbuf.fillcolumn    = ps_global->composer_fillcol;
    pbuf.menu_rows     = FOOTER_ROWS(ps_global) - 1;
    pbuf.ins_help      = h_composer_ins;
    pbuf.search_help   = h_composer_search;
    pbuf.browse_help   = h_composer_browse;
    pbuf.composer_help = h_composer;
    pbuf.pine_anchor   = set_titlebar(editor_title, ps_global->mail_stream,
				      ps_global->context_current,
				      ps_global->cur_folder,ps_global->msgmap, 
				      0, FolderName, 0, 0);
    pbuf.pine_version  = pine_version;
    pbuf.pine_flags    = flags_for_pico(ps_global);
    if(ps_global->VAR_OPER_DIR){
	pbuf.oper_dir    = ps_global->VAR_OPER_DIR;
	pbuf.pine_flags |= P_TREE;
    }

    /* NOTE: initial cursor position set below */

    dprint(9, (debugfile, "flags: %x\n", pbuf.pine_flags));

    /*
     * When user runs compose and the current folder is a newsgroup,
     * offer to post to the current newsgroup.
     */
    if(!outgoing->to && !(outgoing->newsgroups && *outgoing->newsgroups)
       && ps_global->mail_stream && ps_global->mail_stream->mailbox
       && ps_global->mail_stream->mailbox[0] == '*'){
	char  prompt[200];
	char  newsgroup_name[MAILTMPLEN];

	newsgroup_name[0] = '\0';

	/* if remote, c-client has a function to parse out mailbox */
	if(ps_global->mail_stream->mailbox[1] == '{')
	  mail_valid_net(ps_global->mail_stream->mailbox,
		ps_global->mail_stream->dtb,
		NULL,
		newsgroup_name);
	else
	  (void)strcpy(newsgroup_name, &(ps_global->mail_stream->mailbox[1]));

	/*
	 * Replies don't get this far because To or Newsgroups will already
	 * be filled in.  So must be either ordinary compose or forward.
	 * Forward sets subject, so use that to tell the difference.
	 */
	if(newsgroup_name[0] && !outgoing->subject){
	    int ch = 'y';
	    int ret_val;
	    char *errmsg = NULL;
	    BUILDER_ARG	 *fcc_build = NULL;

	    if(F_OFF(F_COMPOSE_TO_NEWSGRP,ps_global)){
		sprintf(prompt,
		    "Post to current newsgroup (%s)", newsgroup_name);
		ch = want_to(prompt, 'y', 'x', NO_HELP, 0, 0);
	    }

	    switch(ch){
	      case 'y':
		if(outgoing->newsgroups)
		  fs_give((void **)&outgoing->newsgroups); 

		if(!fcc_arg){
		    fcc_build = (BUILDER_ARG *)fs_get(sizeof(BUILDER_ARG));
		    fcc_build->tptr = fcc_to_free;
		    fcc_build->next = NULL;
		    fcc_build->xtra = NULL;
		}

		ret_val = news_build(newsgroup_name, &outgoing->newsgroups,
				     &errmsg, fcc_build);

		if(ret_val == -1){
		    if(outgoing->newsgroups)
		      fs_give((void **)&outgoing->newsgroups); 

		    outgoing->newsgroups = cpystr(newsgroup_name);
		}

		if(!fcc_arg){
		    fcc_arg = fcc_to_free = fcc_build->tptr;
		    fs_give((void **)&fcc_build);
		}

		if(errmsg){
		    if(*errmsg){
			q_status_message(SM_ORDER, 3, 3, errmsg);
			display_message(NO_OP_COMMAND);
		    }

		    fs_give((void **)&errmsg);
		}

		break;

	      case 'x': /* ^C */
		q_status_message(SM_ORDER, 0, 3, "Message cancelled");
		dprint(4, (debugfile, "=== send: cancelled\n"));
		return;

	      case 'n':
		break;

	      default:
		break;
	    }
	}
    }

    /* how many fixed fields are there? */
    fixed_cnt = (sizeof(pf_template)/sizeof(pf_template[0])) - 1;

    total_cnt = fixed_cnt + count_custom_hdrs();

    /* the fixed part of the PINEFIELDs */
    i       = fixed_cnt * sizeof(PINEFIELD);
    pfields = (PINEFIELD *)fs_get((size_t) i);
    memset(pfields, 0, (size_t) i);

    /* temporary headerentry array for pico */
    i        = (total_cnt + 1) * sizeof(struct headerentry);
    headents = (struct headerentry *)fs_get((size_t) i);
    memset(headents, 0, (size_t) i);

    i             = total_cnt * sizeof(PINEFIELD *);
    sending_order = (PINEFIELD **)fs_get((size_t) i);
    memset(sending_order, 0, (size_t) i);

    pbuf.headents        = headents;
    header.env           = outgoing;
    header.local         = pfields;
    header.sending_order = sending_order;

    /* custom part of PINEFIELDs */
    header.custom = (custom) ? (PINEFIELD *)custom : get_dflt_custom_hdrs();

    he = headents;
    pf = pfields;

    /*
     * For Address types, pf->addr points to an ADDRESS *.
     * If that address is in the "outgoing" envelope, it will
     * be freed by the caller, otherwise, it should be freed here.
     * Pf->textbuf for an Address is used a little to set up a default,
     * but then is freed right away below.  Pf->scratch is used for a
     * pointer to some alloced space for pico to edit in.  Addresses in
     * the custom area are freed by free_customs().
     *
     * For FreeText types, pf->addr is not used.  Pf->text points to a
     * pointer that points to the text.  Pf->textbuf points to a copy of
     * the text that must be freed before we leave, otherwise, it is
     * probably a pointer into the envelope and that gets freed by the
     * caller.
     *
     * He->realaddr is the pointer to the text that pico actually edits.
     */

#if	!(defined(DOS) || defined(OS2)) || defined(NOAUTH)
#define NN 3
#else
#define NN 2
#endif

    /* initialize the fixed header elements of the two temp arrays */
    for(i=0; i < fixed_cnt; i++, pf++){

	/* slightly different editing order if sending to news */
	if(outgoing->newsgroups && *outgoing->newsgroups){
	    index = (i == 0) ? N_FROM :
		     (i == 1) ? N_REPLYTO :
		      (i == 2) ? N_NEWS :
		       (i == 3) ? N_TO :
			(i == 4) ? N_CC :
			 (i == 5) ? N_BCC :
			  (i == 6) ? N_FCC :
			   (i == 7) ? N_LCC :
			    (i == 8) ? N_ATTCH :
			     (i == 9) ? N_SUBJ :
			      (i == 10) ? N_REF :
			       (i == 11) ? N_DATE :
				(i == 12) ? N_INREPLY :
				 (i == 13) ? N_MSGID :
				  (i == 14) ? N_NOBODY :
				   (i == 15) ? N_POSTERR :
#if	!(defined(DOS) || defined(OS2)) || defined(NOAUTH)
				    (i == 16) ? N_SENDER :
#endif
							  i;
	}
	else
	  index = i;

	if(i > 16)
	  q_status_message1(SM_ORDER,3,7,
	      "Internal error: i=%d in pine_send", (void *)i);

	/* copy the templates */
	*he             = he_template[index];

	pf->name        = cpystr(pf_template[index].name);
	if(index == N_SENDER && F_ON(F_USE_SENDER_NOT_X, ps_global))
	  /* slide string over so it is Sender instead of X-Sender */
	  for(p=pf->name; *(p+1); p++)
	    *p = *(p+2);

	pf->type        = pf_template[index].type;
	pf->canedit     = pf_template[index].canedit;
	pf->rcptto      = pf_template[index].rcptto;
	pf->writehdr    = pf_template[index].writehdr;
	pf->localcopy   = pf_template[index].localcopy;
	pf->he          = he;
	pf->next        = pf + 1;

	he->rich_header = view_as_rich(pf->name, he->rich_header);

	switch(pf->type){
	  case FreeText:   		/* realaddr points to c-client env */
	    if(index == N_NEWS){
		sending_order[0]	= pf;
		he->realaddr		= &outgoing->newsgroups;
		/* If there is a newsgrp already, we'd better show them */
		if(outgoing->newsgroups && *outgoing->newsgroups)
		  he->rich_header = 0; /* force on by default */

	        he_news			= he;

		/*
		 * If this field doesn't already have a value, then we want
		 * to check for a default value assigned by the user.  If no
		 * default, give it an alloced empty string.
		 */
		if(!*he->realaddr){		/* no value */
		    set_default_hdrval(pf);	/* default in pf->textbuf */
		    *he->realaddr = pf->textbuf;
		    pf->textbuf   = NULL;
		}

		pf->text = he->realaddr;
	    }
	    else if(index == N_DATE){
		sending_order[1]	= pf;
		pf->text		= &outgoing->date;
		pf->he			= NULL;
	    }
	    else if(index == N_INREPLY){
		sending_order[NN+9]	= pf;
		pf->text		= &outgoing->in_reply_to;
		pf->he			= NULL;
	    }
	    else if(index == N_MSGID){
		sending_order[NN+10]	= pf;
		pf->text		= &outgoing->message_id;
		pf->he			= NULL;
	    }
	    else if(index == N_REF){
		sending_order[NN+11]	= pf;
		pf_ref			= pf;
		if(ref_list)
		  pf->textbuf = cpystr(ref_list);

		pf->text		= &pf->textbuf;
		pf->he			= NULL;
	    }
	    else if(index == N_POSTERR){
		sending_order[NN+12]	= pf;
		pf_err			= pf;
		pf->text		= &pf->textbuf;
		pf->he			= NULL;
	    }
	    else{
		q_status_message(SM_ORDER | SM_DING, 3, 7,
			    "Botched: Unmatched FreeText header in pine_send");
	    }

	    break;

	  /* can't do a default for this one */
	  case Attachment:
	    /* If there is an attachment already, we'd better show them */
            if(body && *body && (*body)->type != TYPETEXT)
	      he->rich_header = 0; /* force on by default */

	    break;

	  case Address:
	    switch(index){
	      case N_FROM:
		sending_order[2]	= pf;
		pf->addr		= &outgoing->from;
		break;

	      case N_TO:
		sending_order[NN+2]	= pf;
		pf->addr		= &outgoing->to;
		/* If already set, make it act like we typed it in */
		if(outgoing->to
		   && outgoing->to->mailbox
		   && outgoing->to->mailbox[0])
		  he->sticky = 1;

		he_to			= he;
		break;

	      case N_NOBODY:
		sending_order[NN+5]	= pf;
		pf_nobody		= pf;
		if(ps_global->VAR_EMPTY_HDR_MSG
		   && !ps_global->VAR_EMPTY_HDR_MSG[0]){
		    pf->addr		= NULL;
		}
		else{
		    nobody_addr          = mail_newaddr();
		    nobody_addr->next    = mail_newaddr();
		    nobody_addr->mailbox = cpystr(rfc1522_encode(tmp_20k_buf,
			    (unsigned char *)(ps_global->VAR_EMPTY_HDR_MSG
						? ps_global->VAR_EMPTY_HDR_MSG
						: "Undisclosed recipients"),
			    ps_global->VAR_CHAR_SET));
		    pf->addr		= &nobody_addr;
		}

		break;

	      case N_CC:
		sending_order[NN+3]	= pf;
		pf->addr		= &outgoing->cc;
		break;

	      case N_BCC:
		sending_order[NN+4]	= pf;
		pf->addr		= &outgoing->bcc;
		/* if bcc exists, make sure it's exposed so nothing's
		 * sent by mistake...
		 */
		if(outgoing->bcc)
		  he->display_it = 1;

		break;

	      case N_REPLYTO:
		sending_order[NN+1]	= pf;
		pf->addr		= &outgoing->reply_to;
		break;

	      case N_LCC:
		sending_order[NN+7]	= pf;
		pf->addr		= &lcc_addr;
		he_lcc			= he;
		if(lcc_arg){
		    build_address(lcc_arg, &addr, NULL, NULL);
		    rfc822_parse_adrlist(&lcc_addr, addr,
			ps_global->maildomain);
		    fs_give((void **)&addr);
		    he->display_it = 1;
		}

		break;

#if	!(defined(DOS) || defined(OS2)) || defined(NOAUTH)
              case N_SENDER:
		sending_order[3]	= pf;
		pf->addr		= &outgoing->sender;
                break;
#endif

	      default:
		q_status_message1(SM_ORDER,3,7,
		    "Internal error: Address header %d", (void *)index);
		break;
	    }

	    /*
	     * If this is a reply to news, don't show the regular email
	     * recipient headers (unless they are non-empty).
	     */
	    if((outgoing->newsgroups && *outgoing->newsgroups)
	       && (index == N_TO || index == N_CC
		   || index == N_BCC || index == N_LCC)
	       && (!*pf->addr)){
		he->rich_header = 1; /* hide */
	    }

	    /*
	     * If this address doesn't already have a value, then we check
	     * for a default value assigned by the user.
	     */
	    else if(pf->addr && !*pf->addr){
		set_default_hdrval(pf);

#ifndef ALLOW_CHANGING_FROM
		if(index != N_FROM){  /* don't set default for From */
#endif
		    if(pf->textbuf && *pf->textbuf){
		        /* strip quotes around whole default */
		        if(*pf->textbuf == '"'){
		          char *p;

		          /* strip trailing whitespace */
		          p = pf->textbuf + strlen(pf->textbuf) - 1;
		          while(isspace((unsigned char)*p) && p > pf->textbuf)
			    p--;

		          /* a set of surrounding quotes we want to strip */
		          if(*p == '"'){
			    *p = '\0';
			    for(p=pf->textbuf; *(p+1); p++)
			      *p = *(p+1);

			    *p = '\0';
		          }
		        }

		        build_address(pf->textbuf, &addr, NULL, NULL);
		        rfc822_parse_adrlist(pf->addr,
			    addr, ps_global->maildomain);
		        fs_give((void **)&addr);
		    }
#ifndef ALLOW_CHANGING_FROM
		}
#endif

		/* if we still don't have a from */
		if(index == N_FROM && !*pf->addr)
		  *pf->addr = generate_from();

	    }else if(index == N_FROM || index == N_REPLYTO){
		/* side effect of this may set canedit */
		set_default_hdrval(pf);
	    }else if(pf->addr && *pf->addr && !(*pf->addr)->mailbox){
		fs_give((void **)pf->addr);
		he->display_it = 1;  /* start this off showing */
	    }
	    else
	      he->display_it = 1;  /* start this off showing */

	    if(!outgoing->return_path)
	      outgoing->return_path = rfc822_cpy_adr(outgoing->from);

	    if(pf->textbuf)		/* free default value in any case */
	      fs_give((void **)&pf->textbuf);

	    /* outgoing2strings will alloc the string pf->scratch below */
	    he->realaddr = &pf->scratch;
	    break;

	  case Fcc:
	    sending_order[NN+8] = pf;
	    pf_fcc              = pf;
	    fcc                 = get_fcc(fcc_arg);
	    if(fcc_to_free){
		fs_give((void **)&fcc_to_free);
		fcc_arg = NULL;
	    }

	    if(sticky_fcc && fcc[0])
	      he->sticky = 1;

	    if(strcmp(fcc, ps_global->VAR_DEFAULT_FCC) != 0)
	      he->display_it = 1;  /* start this off showing */

	    he->realaddr  = &fcc;
	    pf->text      = &fcc;
	    he_fcc        = he;
	    break;

	  case Subject :
	    sending_order[NN+6]	= pf;
	    if(outgoing->subject){
		pf->scratch = cpystr(outgoing->subject);
	    }
	    else{
		set_default_hdrval(pf);	/* default in pf->textbuf */
		pf->scratch = pf->textbuf;
		pf->textbuf   = NULL;
	    }

	    he->realaddr = &pf->scratch;
	    pf->text	 = &outgoing->subject;
	    break;

	  default:
	    q_status_message1(SM_ORDER,3,7,
			      "Unknown header type %d in pine_send",
			      (void *)pf->type);
	    break;
	}

	/*
	 * We may or may not want to give the user the chance to edit
	 * the From and Reply-To lines.  If they are listed in either
	 * Default-composer-hdrs or Customized-hdrs, then they can edit
	 * them, else no.
	 * If canedit is not set, that means that this header is not in
	 * the user's customized-hdrs.  If rich_header is set, that
	 * means that this header is not in the user's
	 * default-composer-hdrs (since From and Reply-To are rich
	 * by default).  So, don't give it an he to edit with in that case.
	 *
	 * For other types, just not setting canedit will cause it to be
	 * uneditable, regardless of what the user does.
	 */
	switch(index){
	  case N_FROM:
/* to allow it, we let this fall through to the reply-to case below */
#ifndef ALLOW_CHANGING_FROM
	    if(pf->canedit || !he->rich_header)
	      q_status_message(SM_ORDER, 3, 3,
		    "Not allowed to change header \"From\"");

	    memset(he, 0, (size_t)sizeof(*he));
	    pf->he = NULL;
	    break;
#endif

	  case N_REPLYTO:
	    if(!pf->canedit && he->rich_header){
	        memset(he, 0, (size_t)sizeof(*he));
		pf->he = NULL;
	    }
	    else{
		pf->canedit = 1;
		he++;
	    }

	    break;

	  default:
	    if(!pf->canedit){
	        memset(he, 0, (size_t)sizeof(*he));
		pf->he = NULL;
	    }
	    else
	      he++;

	    break;
	}
    }

    /*
     * This is so the builder can tell the composer to fill the affected
     * field based on the value in the field on the left.
     *
     * Note that this mechanism isn't completely general.  Each entry has
     * only a single next_affected, so if some other entry points an
     * affected entry at an entry with a next_affected, they all inherit
     * that next_affected.  Since this isn't used much a careful ordering
     * of the affected fields should make it a sufficient mechanism.
     */
    he_to->affected_entry   = he_fcc;
    he_news->affected_entry = he_fcc;
    he_lcc->affected_entry  = he_to;
    he_to->next_affected    = he_fcc;

    (--pf)->next = (total_cnt != fixed_cnt) ? header.custom : NULL;

    i--;  /* subtract one because N_ATTCH doesn't get a sending_order slot */
    /*
     * Set up headerentries for custom fields.
     * NOTE: "i" is assumed to now index first custom field in sending
     *       order.
     */
    for(pf = pf->next; pf && pf->name; pf = pf->next, he++){
	char *addr;

	pf->he          = he;
	pf->canedit     = 1;
	pf->rcptto      = 0;
	pf->writehdr    = 1;
	pf->localcopy   = 1;
	
	switch(pf->type){
	  case Address:
	    sending_order[i++] = pf;
	    *he = he_custom_addr_templ;
	    /* change default text into an ADDRESS */
	    /* strip quotes around whole default */
	    if(*pf->textbuf == '"'){
	      char *p;

	      /* strip trailing whitespace */
	      p = pf->textbuf + strlen(pf->textbuf) - 1;
	      while(isspace((unsigned char)*p) && p > pf->textbuf)
		p--;

	      /* this is a set of surrounding quotes we want to strip */
	      if(*p == '"'){
		*p = '\0';
		for(p=pf->textbuf; *(p+1); p++)
		  *p = *(p+1);

		*p = '\0';
	      }
	    }

	    build_address(pf->textbuf, &addr, NULL, NULL);
	    rfc822_parse_adrlist(pf->addr, addr, ps_global->maildomain);
	    fs_give((void **)&addr);
	    if(pf->textbuf)
	      fs_give((void **)&pf->textbuf);

	    he->realaddr = &pf->scratch;
	    break;

	  case FreeText:
	    sending_order[i++] = pf;
	    *he                = he_custom_free_templ;
	    he->realaddr       = &pf->textbuf;
	    pf->text           = &pf->textbuf;
	    break;

	  default:
	    q_status_message1(SM_ORDER,0,7,"Unknown custom header type %d",
							(void *)pf->type);
	    break;
	}

	he->name = pf->name;

	/* use first 8 characters for prompt */
	he->prompt = cpystr("        : ");
	strncpy(he->prompt, he->name, min(strlen(he->name), he->prlen - 2));

	he->rich_header = view_as_rich(he->name, he->rich_header);
    }

    /*
     * Make sure at least *one* field is displayable...
     */
    for(index = -1, i=0, pf=header.local; pf && pf->name; pf=pf->next, i++)
      if(pf->he && !pf->he->rich_header){
	  index = i;
	  break;
      }

    /*
     * None displayable!!!  Warn and display defaults.
     */
    if(index == -1){
	q_status_message(SM_ORDER,0,5,
		     "No default-composer-hdrs matched, displaying defaults");
	for(i = 0, pf = header.local; pf; pf = pf->next, i++)
	  if(i == N_TO || i == N_CC || i == N_SUBJ || i == N_ATTCH)
	    pf->he->rich_header = 0;
    }

    /*----------------------------------------------------------------------
       Loop calling the editor until everything goes well
     ----*/
    while(1){
	/*
	 * set initial cursor position based on how many times we've been
	 * thru the loop...
	 */
	if((reply_list && reply_list[0] != -1) || body_start){
	    pbuf.pine_flags |= P_BODY;
	    body_start = 0;		/* maybe not next time */
	}
	else if(reply_list)
	  pbuf.pine_flags |= P_HEADEND;

	/* in case these were turned on in previous pass through loop */
	if(pf_ref){
	    pf_ref->writehdr     = 0;
	    pf_ref->localcopy    = 0;
	}

	if(pf_nobody){
	    pf_nobody->writehdr  = 0;
	    pf_nobody->localcopy = 0;
	}

	if(pf_fcc)
	  pf_fcc->localcopy = 0;

	/*
	 * If a sending attempt failed after we passed the message text
	 * thru a user-defined filter, "orig_so" points to the original
	 * text.  Replace the body's encoded data with the original...
	 */
	if(orig_so){
	    STORE_S **so = (STORE_S **)(((*body)->type == TYPEMULTIPART)
				? &(*body)->contents.part->body.contents.binary
				: &(*body)->contents.binary);
	    so_give(so);
	    *so     = orig_so;
	    orig_so = NULL;
	}

        /*
         * Convert the envelope and body to the string format that
         * pico can edit
         */
        outgoing2strings(&header, *body, &pbuf.msgtext, &pbuf.attachments);

	for(pf = header.local; pf && pf->name; pf = pf->next){
	    /*
	     * If this isn't the first time through this loop, we may have
	     * freed some of the FreeText headers below so that they wouldn't
	     * show up as empty headers in the finished message.  Need to
	     * alloc them again here so they can be edited.
	     */
	    if(pf->type == FreeText && pf->he && !*pf->he->realaddr)
	      *pf->he->realaddr = cpystr("");

	    if(pf->type != Attachment && pf->he && *pf->he->realaddr)
	      pf->he->maxlen = strlen(*pf->he->realaddr);
	}

#ifdef _WINDOWS
	mswin_setwindowmenu (MENU_COMPOSER);
#endif
#if	defined(DOS) && !defined(_WINDOWS)
/*
 * dumb hack to help ensure we've got something left
 * to send with if the user runs out of precious memory
 * in the composer...	(FIX THIS!!!)
 */
	if((reserve = (char *)malloc(16384)) == NULL)
	  q_status_message(SM_ORDER | SM_DING, 0, 5,
			   "LOW MEMORY!  May be unable to complete send!");
#endif

	cancel_busy_alarm(-1);
        flush_status_messages(1);

	dprint(1, (debugfile, "\n  ---- COMPOSER ----\n"));
	editor_result = pico(&pbuf);
	dprint(4, (debugfile, "... composer returns (0x%x)\n", editor_result));

#if	defined(DOS) && !defined(_WINDOWS)
	free((char *)reserve);
#endif
#ifdef _WINDOWS
	mswin_setwindowmenu (MENU_DEFAULT);
#endif
	fix_windsize(ps_global);
	/*
	 * Only reinitialize signals if we didn't receive an interesting
	 * one while in pico, since pico's return is part of processing that
	 * signal and it should continue to be ignored.
	 */
	if(!(editor_result & COMP_GOTHUP))
	  init_signals();        /* Pico has it's own signal stuff */

	/*
	 * We're going to save in DEADLETTER.  Dump attachments first.
	 */
	if(editor_result & COMP_CANCEL)
	  free_attachment_list(&pbuf.attachments);

        /* Turn strings back into structures */
        strings2outgoing(&header, body, pbuf.attachments);
  
        /* Make newsgroups NULL if it is "" (so won't show up in headers) */
	if(outgoing->newsgroups){
	    sqzspaces(outgoing->newsgroups);
	    if(!outgoing->newsgroups[0])
	      fs_give((void **)&(outgoing->newsgroups));
	}

        /* Make subject NULL if it is "" (so won't show up in headers) */
        if(outgoing->subject && !outgoing->subject[0])
          fs_give((void **)&(outgoing->subject)); 
	
	/* remove custom fields that are empty */
	for(pf = header.local; pf && pf->name; pf = pf->next){
	  if(pf->type == FreeText && pf->textbuf){
	    if(pf->textbuf[0] == '\0'){
		fs_give((void **)&pf->textbuf); 
		pf->text = NULL;
	    }
	  }
	}

        removing_trailing_white_space(fcc);

	/*-------- Stamp it with a current date -------*/
	if(outgoing->date)			/* update old date */
	  fs_give((void **)&(outgoing->date));

	rfc822_date(tmp_20k_buf);		/* format and copy new date */
	outgoing->date = cpystr(tmp_20k_buf);

#ifdef OLDWAY
/* some people objected to our doing this. */
	/*------- If Reply-To same as From, get rid of it -------*/
	if(outgoing->reply_to
	   && address_is_same(outgoing->from, outgoing->reply_to))
	  mail_free_address(&outgoing->reply_to);
#endif /* OLDWAY */

	/*
	 * Don't ever believe the sender that is there.
	 * If From doesn't look quite right, generate our own sender.
	 */
	if(outgoing->sender)
	  mail_free_address(&outgoing->sender);

	/*
	 * If the LHS of the address doesn't match, or the RHS
	 * doesn't match one of localdomain or hostname,
	 * then add a sender line (really X-Sender).
	 *
	 * Don't add a personal_name since the user can change that.
	 */
	if(!outgoing->from
	   || !outgoing->from->mailbox
	   || strucmp(outgoing->from->mailbox, ps_global->VAR_USER_ID) != 0
	   || !outgoing->from->host
	   || !(strucmp(outgoing->from->host, ps_global->localdomain) == 0
	   || strucmp(outgoing->from->host, ps_global->hostname) == 0)){

	    outgoing->sender	      = mail_newaddr();
	    outgoing->sender->mailbox = cpystr(ps_global->VAR_USER_ID);
	    outgoing->sender->host    = cpystr(ps_global->hostname);
	}

        /*----- Message is edited, now decide what to do with it ----*/
	if(editor_result & (COMP_SUSPEND | COMP_GOTHUP | COMP_CANCEL)){
            /*=========== Postpone or Interrupted message ============*/
	    CONTEXT_S *fcc_cntxt = NULL;
	    char       folder[MAXPATH+1];
	    int	       fcc_result = 0;
	    char       label[50];

	    dprint(4, (debugfile, "pine_send:%s handling\n",
		       (editor_result & COMP_SUSPEND)
			   ? "SUSPEND"
			   : (editor_result & COMP_GOTHUP)
			       ? "HUP"
			       : (editor_result & COMP_CANCEL)
				   ? "CANCEL" : "HUH?"));
	    if((editor_result & COMP_CANCEL)
	       && F_ON(F_QUELL_DEAD_LETTER, ps_global)){
		q_status_message(SM_ORDER, 0, 3, "Message cancelled");
		break;
	    }

	    /*
	     * The idea here is to use the Fcc: writing facility
	     * to append to the special postponed message folder...
	     *
	     * NOTE: the strategy now is to write the message and
	     * all attachments as they exist at composition time.
	     * In other words, attachments are postponed by value
	     * and not reference.  This may change later, but we'll
	     * need a local "message/external-body" type that
	     * outgoing2strings knows how to properly set up for
	     * the composer.  Maybe later...
	     */

	    label[0] = '\0';

	    if(F_ON(F_COMPOSE_REJECTS_UNQUAL, ps_global)
	       && (editor_result & COMP_SUSPEND)
	       && check_addresses(&header)){
		/*--- Addresses didn't check out---*/
		q_status_message(SM_ORDER, 7, 7,
	      "Not allowed to postpone message until addresses are qualified");
		continue;
            }

	    /*
	     * Build the local_so.  In the HUP case, we'll write the
	     * bezerk delimiter by hand and output the message directly
	     * into the folder.  It's not only faster, we don't have to
	     * worry about c-client reentrance and less hands paw over
	     * over the data so there's less chance of a problem.
	     *
	     * In the Postpone case, just create it if the user wants to
	     * and create a temporary storage object to write into.
	     */
  fake_hup:
	    local_written = 0;
	    if(editor_result & COMP_GOTHUP){
		int    newfile;
		time_t now = time((time_t *)0);

		build_path(folder,
			  ps_global->VAR_OPER_DIR ? ps_global->VAR_OPER_DIR
						  : ps_global->home_dir,
			  INTERRUPTED_MAIL);
		newfile = can_access(folder, ACCESS_EXISTS);
		if(local_so = so_get(FCC_SOURCE, NULL, WRITE_ACCESS)){
		    sprintf(tmp_20k_buf, "%sFrom %s@%s %.24s\015\012",
			    newfile ? "" : "\015\012",
			    outgoing->from->mailbox,
			    outgoing->from->host, ctime(&now));
		    if(!so_puts(local_so, tmp_20k_buf))
		      dprint(1, (debugfile,"***CAN'T WRITE %s: %s\n",
				 folder, error_description(errno)));
		}
	    }
	    else if(editor_result & COMP_SUSPEND){
		if(!ps_global->VAR_POSTPONED_FOLDER
		   || !ps_global->VAR_POSTPONED_FOLDER[0]){
		    q_status_message(SM_ORDER, 3, 3,
				     "No postponed file defined");
		    continue;
		}

		strcpy(folder, ps_global->VAR_POSTPONED_FOLDER);
		strcpy(label, "postponed message");
		local_so = open_fcc(folder,&fcc_cntxt, 1)
			  ? so_get(FCC_SOURCE, NULL, WRITE_ACCESS) : NULL;
	    }
	    else{				/* canceled message */
#if defined(DOS) || defined(OS2)
		/*
		 * we can't assume anything about root or home dirs, so
		 * just plunk it down in the same place as the pinerc
		 */
		if(!getenv("HOME")){
		    char *lc = last_cmpnt(ps_global->pinerc);
		    folder[0] = '\0';
		    if(lc != NULL){
			strncpy(folder,ps_global->pinerc,lc-ps_global->pinerc);
			folder[lc - ps_global->pinerc] = '\0';
		    }

		    strcat(folder, DEADLETTER);
		}
		else
#endif
		build_path(folder,
		    ps_global->VAR_OPER_DIR ? ps_global->VAR_OPER_DIR
					: ps_global->home_dir, DEADLETTER);

		strcpy(label, DEADLETTER);
		unlink(folder);
		local_so = open_fcc(folder,&fcc_cntxt, 1)
			  ? so_get(FCC_SOURCE, NULL, WRITE_ACCESS) : NULL;
	    }

	    if(local_so){

		/* Turn on references header */
		if(ref_list){
		    pf_ref->writehdr  = 1;
		    pf_ref->localcopy = 1;
		}

		/* copy fcc line to postponed or interrupted folder */
	        if(pf_fcc)
		  pf_fcc->localcopy = 1;

		if((editor_result & ~0xff) && last_message_queued()){
		    pf_err->writehdr  = 1;
		    pf_err->localcopy = 1;
		    pf_err->textbuf   = cpystr(last_message_queued());
		}

		/*
		 * We need to make sure any header values that got cleared
		 * get written to the postponed message (they won't if
		 * pf->text is NULL).  Otherwise, we can't tell previously
		 * non-existent custom headers or default values from 
		 * custom (or other) headers that got blanked in the
		 * composer...
		 */
		for(pf = header.local; pf && pf->name; pf = pf->next)
		  if(pf->type == FreeText && pf->he && !*(pf->he->realaddr))
		    *(pf->he->realaddr) = cpystr("");

		if(pine_rfc822_output(&header,*body,NULL,NULL) >= 0){
		    if(editor_result & COMP_GOTHUP){
			char	*err;
			STORE_S *hup_so;
			gf_io_t	 gc, pc;

			if(hup_so = so_get(FileStar, folder, WRITE_ACCESS)){
			    gf_set_so_readc(&gc, local_so);
			    gf_set_so_writec(&pc, hup_so);
			    so_seek(local_so, 0L, 0); 	/* read msg copy and */
			    so_seek(hup_so, 0L, 2);	/* append to folder  */
			    gf_filter_init();
			    gf_link_filter(gf_nvtnl_local);
			    if(!(fcc_result = !(err = gf_pipe(gc, pc))))
			      dprint(1, (debugfile, "*** PIPE FAILED: %s\n",
					 err));

			    so_give(&hup_so);
			}
			else
			  dprint(1, (debugfile, "*** CAN'T CREATE %s: %s\n",
				     folder, error_description(errno)));
		    }
		    else
		      fcc_result = write_fcc(folder,fcc_cntxt,local_so,label);
		}

		so_give(&local_so);
	    }
	    else
	      dprint(1, (debugfile, "***CAN'T ALLOCATE temp store: %s ",
			 error_description(errno)));

	    if(editor_result & COMP_GOTHUP){
		/*
		 * Special Hack #291: if any hi-byte bits are set in
		 *		      editor's result, we put them there.
		 */
		if(editor_result & 0xff00)
		  exit(editor_result >> 8);

		dprint(1, (debugfile, "Save composition on HUP %sED\n",
			   fcc_result ? "SUCCEED" : "FAIL"));
		hup_signal();		/* Do what we normally do on SIGHUP */
	    }
	    else if(editor_result & COMP_SUSPEND && fcc_result){
		q_status_message(SM_ORDER, 0, 3,
			 "Composition postponed. Select Compose to resume.");
                break; /* postpone went OK, get out of here */
	    }
	    else if(editor_result & COMP_CANCEL){
		q_status_message(SM_ORDER, 0, 3, "Message cancelled");
		break;
            }
	    else{
		q_status_message(SM_ORDER, 0, 4,
		    "Continuing composition.  Message not postponed or sent");
		body_start = 1;
		continue; /* postpone failed, jump back in to composer */
            }
	}
	else{
	    /*------ Must be sending mail or posting ! -----*/
	    int	       result;
	    CONTEXT_S *fcc_cntxt = NULL;

	    result = 0;
	    dprint(4, (debugfile, "=== sending: "));

            /* --- If posting, confirm with user ----*/
	    if(outgoing->newsgroups && *outgoing->newsgroups
	       && want_to(POST_PMT, 'n', 'n', NO_HELP, 0, 0) == 'n'){
		q_status_message(SM_ORDER, 0, 3, "Message not posted");
		dprint(4, (debugfile, "no post, continuing\n"));
		continue;
	    }

	    if(!(outgoing->to || outgoing->cc || outgoing->bcc || lcc_addr
		 || outgoing->newsgroups || (fcc && fcc[0]))){
		q_status_message(SM_ORDER, 3, 4, "No recipients specified!");
		dprint(4, (debugfile, "no recip, continuing\n"));
		continue;
	    }

	    if(check_addresses(&header)){
		/*--- Addresses didn't check out---*/
		dprint(4, (debugfile, "addrs failed, continuing\n"));
		continue;
	    }

	    set_last_fcc(fcc);

            /*---- Check out fcc -----*/
            if(fcc && *fcc){
		local_written = 0;
	        if(!open_fcc(fcc, &fcc_cntxt, 0)
		   || !(local_so = so_get(FCC_SOURCE, NULL, WRITE_ACCESS))){
		    /* ---- Open or allocation of fcc failed ----- */
                    q_status_message(SM_ORDER, 3, 5,
				     "Message NOT sent or written to fcc.");
		    dprint(4, (debugfile,"can't open/allocate fcc, cont'g\n"));

		    /*
		     * Find field entry associated with fcc, and start
		     * composer on it...
		     */
		    for(pf = header.local; pf && pf->name; pf = pf->next)
		      if(pf->type == Fcc && pf->he)
			pf->he->start_here = 1;

		    continue;
		}
            }
	    else
	      local_so = NULL;

            /*---- recompute message-id to encode body info stats ----*/
            update_message_id(outgoing, mime_stats(*body));

            /*---- Take care of any requested prefiltering ----*/
	    if(sending_filter_requested
	       && !filter_message_text(sending_filter_requested, outgoing,
				       *body, &orig_so)){
		q_status_message1(SM_ORDER, 3, 3,
				 "Problem filtering!  Nothing sent%s.",
				 fcc ? " or saved to fcc" : "");
		continue;
	    }

            /*------ Actually post  -------*/
            if(outgoing->newsgroups){
		/* Turn on references header */
		if(ref_list){
		    pf_ref->writehdr  = 1;
		    pf_ref->localcopy = 1;
		}

		if(news_poster(&header, *body) < 0){
		    dprint(1, (debugfile, "Post failed, continuing\n"));
		    continue;
		}
		else
		  result |= P_NEWS_WIN;
	    }

	    /*
	     * BUG: IF we've posted the message *and* an fcc was specified
	     * then we've already got a neatly formatted message in the
	     * local_so.  It'd be nice not to have to re-encode everything
	     * to insert it into the smtp slot...
	     */

	    /*
	     * Turn on "undisclosed recipients" header if no To or cc.
	     */
            if(!(outgoing->to || outgoing->cc || outgoing->newsgroups)
	      && (outgoing->bcc || lcc_addr) && pf_nobody && pf_nobody->addr){
		pf_nobody->writehdr  = 1;
		pf_nobody->localcopy = 1;
	    }

#if	defined(BACKGROUND_POST) && defined(SIGCHLD)
	    /*
	     * If requested, launch backgroud posting...
	     */
	    if(background_requested && !verbose_requested){
		ps_global->post = (POST_S *)fs_get(sizeof(POST_S));
		memset(ps_global->post, 0, sizeof(POST_S));
		if(fcc)
		  ps_global->post->fcc = cpystr(fcc);

		if((ps_global->post->pid = fork()) == 0){
		    int rv;

		    /*
		     * Put us in new process group...
		     */
		    setpgrp(0, ps_global->post->pid);

		    /* BUG: should fix argv[0] to indicate what we're up to */

		    /*
		     * If there are any live stream, pretend we never
		     * knew them.  Problem is two processes writing
		     * same server process.
		     */
		    ps_global->mail_stream = ps_global->inbox_stream = NULL;

		    /* quell any display output */
		    ps_global->in_init_seq = 1;

		    /*------- Actually mail the message ------*/
		    if(outgoing->to || outgoing->cc
		       || outgoing->bcc || lcc_addr)
		      result |= (call_mailer(&header, *body) > 0)
				  ? P_MAIL_WIN : P_MAIL_LOSE;

		    /*----- Was there an fcc involved? -----*/
		    if(local_so){
			/*------ Write it if at least something worked ------*/
			if((result & (P_MAIL_WIN | P_NEWS_WIN))
			   || (!(result & (P_MAIL_BITS | P_NEWS_BITS))
			       && pine_rfc822_output(&header, *body,
						     NULL, NULL))){
			    char label[50];

			    strcpy(label, "Fcc");
			    if(strcmp(fcc, ps_global->VAR_DEFAULT_FCC))
			      sprintf(label + 3, " to %.40s", fcc);

			    /*-- Now actually copy to fcc folder and close --*/
			    result |= (write_fcc(fcc, fcc_cntxt, local_so,
						 label))
					? P_FCC_WIN : P_FCC_LOSE;
			}
			else if(!(result & (P_MAIL_BITS | P_NEWS_BITS))){
			    q_status_message(SM_ORDER, 3, 5,
					    "Fcc Failed!.  No message saved.");
			    dprint(1, (debugfile,
				       "explicit fcc write failed!\n"));
			    result |= P_FCC_LOSE;
			}

			so_give(&local_so);
		    }

		    /* BUG: do something about "Answered" flag */
		    if(result & (P_MAIL_LOSE | P_NEWS_LOSE | P_FCC_LOSE)){
			/*
			 * Encode child's result in hi-byte of
			 * editor's result
			 */
			editor_result = ((result << 8) | COMP_GOTHUP);
			goto fake_hup;
		    }

		    exit(result);
		}

		if(ps_global->post->pid > 0){
		    q_status_message(SM_ORDER, 3, 3,
				     "Message handed off for posting");
		    break;		/* up to our child now */
		}
		else{
		    q_status_message1(SM_ORDER | SM_DING, 3, 3,
				      "Can't fork for send: %s",
				      error_description(errno));
		    if(ps_global->post->fcc)
		      fs_give((void **) &ps_global->post->fcc);

		    fs_give((void **) &ps_global->post);
		}

		if(local_so)	/* throw away unused store obj */
		  so_give(&local_so);

		continue;		/* if we got here, there was a prob */
	    }
#endif /* BACKGROUND_POST */

            /*------- Actually mail the message ------*/
            if(outgoing->to || outgoing->cc || outgoing->bcc || lcc_addr)
	      result |= (call_mailer(&header, *body) > 0)
			  ? P_MAIL_WIN : P_MAIL_LOSE;

	    /*----- Was there an fcc involved? -----*/
            if(local_so){
		/*------ Write it if at least something worked ------*/
		if((result & (P_MAIL_WIN | P_NEWS_WIN))
		   || (!(result & (P_MAIL_BITS | P_NEWS_BITS))
		       && pine_rfc822_output(&header, *body, NULL, NULL))){
		    char label[50];

		    strcpy(label, "Fcc");
		    if(strcmp(fcc, ps_global->VAR_DEFAULT_FCC))
		      sprintf(label + 3, " to %.40s", fcc);

		    /*-- Now actually copy to fcc folder and close --*/
		    result |= (write_fcc(fcc, fcc_cntxt, local_so, label))
				? P_FCC_WIN : P_FCC_LOSE;
		}
		else if(!(result & (P_MAIL_BITS | P_NEWS_BITS))){
		    q_status_message(SM_ORDER,3,5,
			"Fcc Failed!.  No message saved.");
		    dprint(1, (debugfile, "explicit fcc write failed!\n"));
		    result |= P_FCC_LOSE;
		}

		so_give(&local_so);
	    }

            /*----- Mail Post FAILED, back to composer -----*/
            if(result & (P_MAIL_LOSE | P_FCC_LOSE)){
		dprint(1, (debugfile, "Send failed, continuing\n"));
		continue;
	    }

	    /*
	     * If message sent *completely* successfully, there's a
	     * reply_list AND we're allowed to write back state, do it.
	     * But also protect against shifted message numbers due 
	     * to new mail arrival.  Since the number passed is based
	     * on the real imap msg no, AND we're sure no expunge has 
	     * been done, just fix up the sorted number...
	     */
	    if(reply_list && reply_list[0] > -1 && !READONLY_FOLDER){
		char *seq, *p;
		long  i, j;

		for(i = 0L, p = tmp_20k_buf; reply_list[i] != -1L; i++){
		    if(i)
		      sstrcpy(&p, ",");

		    sstrcpy(&p, long2string(reply_list[i]));
		    if(j = mn_raw2m(ps_global->msgmap, reply_list[i]))
		      clear_index_cache_ent(j);
		}

		seq = cpystr(tmp_20k_buf);
		mail_setflag(ps_global->mail_stream, seq, "\\ANSWERED");
		fs_give((void **)&seq);
		check_point_change();
	    }

            /*----- Signed, sealed, delivered! ------*/
	    q_status_message(SM_ORDER, 0, 3,
			     pine_send_status(result, fcc, tmp_20k_buf, NULL));

            break; /* All's well, pop out of here */
        }
    }

    if(orig_so)
      so_give(&orig_so);

    if(fcc)
      fs_give((void **)&fcc);

    free_attachment_list(&pbuf.attachments);
    for(i=0; i < fixed_cnt; i++){
	if(pfields[i].textbuf)
	  fs_give((void **)&pfields[i].textbuf);

	fs_give((void **)&pfields[i].name);
    }

    if(lcc_addr)
      mail_free_address(&lcc_addr);
    
    if(nobody_addr)
      mail_free_address(&nobody_addr);
    
    free_customs(header.custom);
    fs_give((void **)&pfields);
    for(he = headents; he->name; he++)
      if(he->bldr_private)
	fs_give((void **)&he->bldr_private);

    fs_give((void **)&headents);
    fs_give((void **)&sending_order);
    dprint(4, (debugfile, "=== send returning ===\n"));
}



/*----------------------------------------------------------------------
   Build a status message suitable for framing

Returns: pointer to resulting buffer
  ---*/
char *
pine_send_status(result, fcc_name, buf, goodorbad)
    int   result;
    char *fcc_name;
    char *buf;
    int  *goodorbad;
{
    sprintf(buf, "Message %s%s%s%s%s%s%s.",
	    (result & P_NEWS_WIN)
	       ? "posted" 
	       : (result & P_NEWS_LOSE)
	            ? "NOT posted" : "",
	    ((result & P_NEWS_BITS) && (result & P_MAIL_BITS)
	     && (result & P_FCC_BITS))
	      ? ", "
	      : ((result & P_NEWS_BITS) && (result & P_MAIL_BITS))
	          ? " and " : "",
	    (result & P_MAIL_WIN)
	      ? "sent"
	      : (result & P_MAIL_LOSE)
	          ? "NOT SENT" : "",
	    ((result & (P_MAIL_BITS | P_NEWS_BITS)) && (result & P_FCC_BITS))
	      ? " and copied to " 
	      : (result & P_FCC_WIN) ? "ONLY copied to " : "",
	    (result & P_FCC_WIN) ? "\"" : "",
	    (result & P_FCC_WIN) ? fcc_name  : "",
	    (result & P_FCC_WIN) ? "\"" : "");

    if(goodorbad)
      *goodorbad = (result & (P_MAIL_LOSE | P_NEWS_LOSE | P_FCC_LOSE)) == 0;

    return(buf);
}



/*----------------------------------------------------------------------
   Check for addresses the user is not permitted to send to, or probably
   doesn't want to send to
   
Returns: 0 if OK, and 1 if the message shouldn't be sent

Queues a message indicating what happened
  ---*/
int
check_addresses(header)
    METAENV *header;
{
    PINEFIELD *pf;
    ADDRESS *a;
    int	     send_daemon = 0;

    /*---- Is he/she trying to send mail to the mailer-daemon ----*/
    for(pf = header->local; pf && pf->name; pf = pf->next)
      if(pf->type == Address && pf->rcptto && pf->addr && *pf->addr)
	for(a = *pf->addr; a != NULL; a = a->next){
	    if(a->host && (a->host[0] == '.'
			   || (F_ON(F_COMPOSE_REJECTS_UNQUAL, ps_global)
			       && a->host[0] == '@'))){
		q_status_message2(SM_ORDER, 4, 7,
				  "Can't send to address %s: %s",
				  a->mailbox,
				  (a->host[0] == '.')
				    ? a->host
				    : "not in addressbook");
		return(1);
	    }
	    else if(ps_global->restricted
		    && !address_is_us(*pf->addr, ps_global)){
		q_status_message(SM_ORDER, 3, 3,
	"Restricted demo version of Pine. You may only send mail to yourself");
		return(1);
	    }
	    else if(a->mailbox && strucmp(a->mailbox, "mailer-daemon") == 0
		    && !send_daemon){
		send_daemon = 1;
		if(want_to("Really send this message to the MAILER-DAEMON",
			   'n', 'n', NO_HELP, 0, 0) == 'n')
		  return(1);
	    }
	}

    return(0);
}


/*----------------------------------------------------------------------
    Validate the given subject relative to any news groups.
     
Args: none

Returns: always returns 1, but also returns error if
----*/      
int
valid_subject(given, expanded, error, fcc)
    char	 *given,
		**expanded,
		**error;
    BUILDER_ARG  *fcc;
{
    struct headerentry *hp;

    if(expanded)
      *expanded = cpystr(given);

    if(error){
	/*
	 * Now look for any header entry we passed to pico that has to do
	 * with news.  If there's no subject, gripe.
	 */
	for(hp = pbuf.headents; hp->prompt; hp++)
	  if(hp->help == h_composer_news){
	      if(hp->hd_text->text[0] && !*given)
		*error = cpystr(
			"News postings MUST have a subject!  Please add one!");

	      break;
	  }
    }

    return(0);
}



/*----------------------------------------------------------------------
    Call to map pine's flags into those pico makes available

Args: ps -- usual pine structure

Returns: long contining bitmap of pico flags
----*/      
long
flags_for_pico(ps)
    struct pine *ps;
{
    return((F_ON(F_CAN_SUSPEND, ps)		? P_SUSPEND : 0L)
	   | (F_ON(F_USE_FK,ps)			? P_FKEYS : 0L)
	   | (ps->restricted			? P_SECURE : 0L)
	   | (F_ON(F_ALT_ED_NOW,ps)		? P_ALTNOW : 0L)
	   | (F_ON(F_USE_CURRENT_DIR,ps)	? P_CURDIR : 0L)
	   | (F_ON(F_SUSPEND_SPAWNS,ps)		? P_SUBSHELL : 0L)
	   | (F_ON(F_COMPOSE_MAPS_DEL,ps)	? P_DELRUBS : 0L)
	   | (F_ON(F_ENABLE_TAB_COMPLETE,ps)	? P_COMPLETE : 0L)
	   | (F_ON(F_SHOW_CURSOR, ps)		? P_SHOCUR : 0L)
	   | (F_ON(F_DEL_FROM_DOT, ps)		? P_DOTKILL : 0L)
	   | (F_ON(F_ENABLE_DOT_FILES, ps)	? P_DOTFILES : 0L)
	   | (F_ON(F_ALLOW_GOTO, ps)		? P_ALLOW_GOTO : 0L)
	   | ((F_ON(F_ENABLE_ALT_ED,ps) || F_ON(F_ALT_ED_NOW,ps)
				||(ps->VAR_EDITOR && ps->VAR_EDITOR[0]))
	        ? P_ADVANCED : 0L)
	   | ((!ps->VAR_CHAR_SET || !strucmp(ps->VAR_CHAR_SET, "US-ASCII"))
	        ? P_HIBITIGN: 0L));
}



/*----------------------------------------------------------------------
    Call back for pico to use to check for new mail.
     
Args: cursor -- pointer to in to tell caller if cursor location changed
                if NULL, turn off cursor positioning.
      timing -- whether or not it's a good time to check 
 

Returns: returns 1 on success, zero on error.
----*/      
long
new_mail_for_pico(timing, status)
    int  timing;
    int  status;
{
    int old_cue, rv;
    int save_defer;

    /*
     * If we're not interested in the status, don't display the busy
     * cue either...
     */
    if(!status){
	old_cue = F_ON(F_SHOW_DELAY_CUE, ps_global);
	F_SET(F_SHOW_DELAY_CUE, ps_global, 0);
    }

    /* don't know where the cursor's been, reset it */
    clear_cursor_pos();
    save_defer = ps_global->sort_is_deferred;
    /* don't sort while composing, just announce new mail */
    ps_global->sort_is_deferred = 1;
    rv = new_mail(0, timing, status);
    ps_global->sort_is_deferred = save_defer;

    if(!status)
      F_SET(F_SHOW_DELAY_CUE, ps_global, old_cue);

    return(rv);
}




/*----------------------------------------------------------------------
    Call back for pico to insert the specified message's text
     
Args: n -- message number to format
      f -- function to use to output the formatted message
 

Returns: returns msg number formatted on success, zero on error.
----*/      
long
message_format_for_pico(n, f)
    long n;
    int  (*f) PROTO((int));
{
    ENVELOPE *e;
    BODY     *b;
    char     *old_quote = NULL;
    long      rv = n;

    if(!(n > 0L && n <= mn_get_total(ps_global->msgmap)
       && (e = mail_fetchstructure(ps_global->mail_stream,
				   mn_m2raw(ps_global->msgmap, n), &b)))){
	q_status_message(SM_ORDER|SM_DING,3,3,"Error inserting Message");
	flush_status_messages(0);
	return(0L);
    }

    /* temporarily assign a new quote string */
    old_quote = pbuf.quote_str;
    pbuf.quote_str = reply_quote_str(e, 1);

    /* build separator line */
    reply_delimiter(e, f);

    /* actually write message text */
    if(!format_message(mn_m2raw(ps_global->msgmap, n), e, b,
		       FM_NEW_MESS, f)){
	q_status_message(SM_ORDER|SM_DING,3,3,"Error inserting Message");
	flush_status_messages(0);
	rv = 0L;
    }

    fs_give((void **)&pbuf.quote_str);
    pbuf.quote_str = old_quote;
    return(rv);
}



/*----------------------------------------------------------------------
    Call back for pico to prompt the user for exit confirmation

Args: dflt -- default answer for confirmation prompt

Returns: either NULL if the user accepts exit, or string containing
	 reason why the user declined.
----*/      
char *
send_exit_for_pico()
{
    int	       i, rv, c, verbose_label = 0, bg_label = 0, old_suspend;
    char      *rstr = NULL, *p, *lc;
    void     (*redraw)() = ps_global->redrawer;
    ESCKEY_S   opts[9];
    struct filters {
	char  *filter;
	int    index;
	struct filters *prev, *next;
    } *filters = NULL, *fp;

    sending_filter_requested = NULL;
    if(old_suspend = F_ON(F_CAN_SUSPEND, ps_global))
      F_SET(F_CAN_SUSPEND, ps_global, 0);

    /*
     * Build list of available filters...
     */
    for(i=0; ps_global->VAR_SEND_FILTER && ps_global->VAR_SEND_FILTER[i]; i++){
	for(p = ps_global->VAR_SEND_FILTER[i];
	    *p && !isspace((unsigned char)*p); p++)
	  ;

	c  = *p;
	*p = '\0';
	if(!(is_absolute_path(ps_global->VAR_SEND_FILTER[i])
	     && can_access(ps_global->VAR_SEND_FILTER[i],EXECUTE_ACCESS) ==0)){
	    *p = c;
	    continue;
	}

	fp	   = (struct filters *)fs_get(sizeof(struct filters));
	fp->index  = i;
	if(lc = last_cmpnt(ps_global->VAR_SEND_FILTER[i])){
	    fp->filter = cpystr(lc);
	}
	else if((p - ps_global->VAR_SEND_FILTER[i]) > 20){
	    sprintf(tmp_20k_buf, "...%s", p - 17);
	    fp->filter = cpystr(tmp_20k_buf);
	}
	else
	  fp->filter = cpystr(ps_global->VAR_SEND_FILTER[i]);

	*p = c;

	if(filters){
	    fp->next	   = filters;
	    fp->prev	   = filters->prev;
	    fp->prev->next = filters->prev = fp;
	}
	else{
	    filters = (struct filters *)fs_get(sizeof(struct filters));
	    filters->index  = -1;
	    filters->filter = NULL;
	    filters->next = filters->prev = fp;
	    fp->next = fp->prev = filters;
	}
    }

    i = 0;
    opts[i].ch      = 'y';
    opts[i].rval    = 'y';
    opts[i].name    = "Y";
    opts[i++].label = "Yes";

    opts[i].ch      = 'n';
    opts[i].rval    = 'n';
    opts[i].name    = "N";
    opts[i++].label = "No";

    if(filters){
	/* set global_filter_pointer to desired filter or NULL if none */
	/* prepare two keymenu slots for selecting filter */
	opts[i].ch      = ctrl('P');
	opts[i].rval    = 10;
	opts[i].name    = "^P";
	opts[i++].label = "Prev Filter";

	opts[i].ch      = ctrl('N');
	opts[i].rval    = 11;
	opts[i].name    = "^N";
	opts[i++].label = "Next Filter";

	if(F_ON(F_FIRST_SEND_FILTER_DFLT, ps_global))
	  filters = filters->next;
    }

    verbose_requested = 0;
    if(F_ON(F_VERBOSE_POST, ps_global)){
	/* setup keymenu slot to toggle verbose mode */
	opts[i].ch    = ctrl('W');
	opts[i].rval  = 12;
	opts[i].name  = "^W";
	verbose_label = i++;
    }

#if	defined(BACKGROUND_POST) && defined(SIGCHLD)
    background_requested = 0;
    if(F_ON(F_BACKGROUND_POST, ps_global)){
	opts[i].ch    = ctrl('R');
	opts[i].rval  = 15;
	opts[i].name  = "^R";
	bg_label = i++;
    }
#endif

    if(filters){
	opts[i].ch      = KEY_UP;
	opts[i].rval    = 10;
	opts[i].name    = "";
	opts[i++].label = "";

	opts[i].ch      = KEY_DOWN;
	opts[i].rval    = 11;
	opts[i].name    = "";
	opts[i++].label = "";
    }

    opts[i].ch = -1;

    ps_global->redrawer = NULL;
    fix_windsize(ps_global);

    while(1){
	if(filters && filters->filter && (p = strindex(filters->filter, ' ')))
	  *p = '\0';
	else
	  p = NULL;

	sprintf(tmp_20k_buf, "Send message%s%s%s%s%s%s%s%s%s%s? ",
		(filters || verbose_requested || background_requested)
		  ? " (" : "",
		(filters && filters->filter) ? "filtered thru \"" : "",
		(filters)
		  ? (filters->filter
		      ? filters->filter
		      : "unfiltered")
		  : "",
		(filters && filters->filter) ? "\"" : "",
		(filters && (verbose_requested || background_requested))
		  ? " " : "",
		(verbose_requested || background_requested)
		  ? "in " : "",
		(verbose_requested) ? "verbose " : "",
		(background_requested) ? "background " : "",
		(verbose_requested || background_requested)
		  ? "mode" : "",
		(filters || verbose_requested || background_requested)
		  ? ")" : "");

	if(p)
	  *p = ' ';

	if(verbose_label)
	  opts[verbose_label].label = verbose_requested ? "Normal" : "Verbose";

	if(bg_label)
	  opts[bg_label].label = background_requested
				   ? "Foreground" : "Background";

/* BUG: fix resize during prompt!! */
/* BUG: test kmpopped stuff? */
	rv = radio_buttons(tmp_20k_buf, -FOOTER_ROWS(ps_global), opts,
			   'y', 'x', NO_HELP, RB_NORM);
	if(rv == 'y'){				/* user ACCEPTS! */
	    break;
	}
	else if(rv == 'n'){			/* Declined! */
	    rstr = "No Message Sent";
	    break;
	}
	else if(rv == 'x'){			/* Cancelled! */
	    rstr = "Send Cancelled";
	    break;
	}
	else if(rv == 10)			/* PREVIOUS filter */
	  filters = filters->prev;
	else if(rv == 11)			/* NEXT filter */
	  filters = filters->next;
	else if(rv == 12){			/* flip verbose bit */
	    if((verbose_requested = !verbose_requested)
	       && background_requested)
	      background_requested = 0;
	}
	else if(rv == 15){
	    if((background_requested = !background_requested)
	       && verbose_requested)
	      verbose_requested = 0;
	}
    }

    /* remember selection */
    if(filters && filters->index > -1)
      sending_filter_requested = ps_global->VAR_SEND_FILTER[filters->index];

    if(filters){
	filters->prev->next = NULL;			/* tie off list */
	while(filters){				/* then free it */
	    fp = filters->next;
	    if(filters->filter)
	      fs_give((void **)&filters->filter);

	    fs_give((void **)&filters);
	    filters = fp;
	}
    }

    if(old_suspend)
      F_SET(F_CAN_SUSPEND, ps_global, 1);

    ps_global->redrawer = redraw;
    return(rstr);
}



/*----------------------------------------------------------------------
    Call back for pico to get newmail status messages displayed

Args: x -- char processed

Returns: 
----*/      
int
display_message_for_pico(x)
    int x;
{
    clear_cursor_pos();			/* can't know where cursor is */
    mark_status_dirty();		/* don't count on cached text */
    return(display_message(x));
}



/*----------------------------------------------------------------------
    Call back for pico to get desired directory for its check point file
     
  Args: s -- buffer to write directory name
	n -- length of that buffer

  Returns: pointer to static buffer
----*/      
char *
checkpoint_dir_for_pico(s, n)
    char *s;
    int   n;
{
#if defined(DOS) || defined(OS2)
    /*
     * we can't assume anything about root or home dirs, so
     * just plunk it down in the same place as the pinerc
     */
    if(!getenv("HOME")){
	char *lc = last_cmpnt(ps_global->pinerc);

	if(lc != NULL){
	    strncpy(s, ps_global->pinerc, min(n-1,lc-ps_global->pinerc));
	    s[min(n-1,lc-ps_global->pinerc)] = '\0';
	}
	else{
	    strncpy(s, ".\\", n-1);
	    s[n-1] = '\0';
	}
    }
    else
#endif
    strcpy(s, ps_global->home_dir);

    return(s);
}


/*----------------------------------------------------------------------
    Call back for pico to display mime type of attachment
     
Args: file -- filename being attached

Returns: returns 1 on success (message queued), zero otherwise (don't know
	  type so nothing queued).
----*/      
int
mime_type_for_pico(file)
    char *file;
{
    BODY *body;
    int   rv;
    void *file_contents;

    body           = mail_newbody();
    body->type     = TYPEOTHER;
    body->encoding = ENCOTHER;

    /* don't know where the cursor's been, reset it */
    clear_cursor_pos();
    if(!set_mime_type_by_extension(body, file)){
	if((file_contents=(void *)so_get(FileStar,file,READ_ACCESS)) != NULL){
	    body->contents.binary = file_contents;
	    set_mime_type_by_grope(body);
	}
    }

    if(body->type != TYPEOTHER){
	rv = 1;
	q_status_message3(SM_ORDER, 0, 3,
	    "File %s attached as type %s/%s", file,
	    body_types[body->type],
	    body->subtype ? body->subtype : rfc822_default_subtype(body->type));
    }
    else
      rv = 0;

    pine_free_body(&body);
    return(rv);
}



/*----------------------------------------------------------------------
  Call back for pico to receive an uploaded message

  Args: fname -- name for uploaded file (empty if they want us to assign it)
	size -- pointer to long to hold the attachment's size

  Notes: the attachment is uploaded to a temp file, and 

  Returns: TRUE on success, FALSE otherwise
----*/
int
upload_msg_to_pico(fname, size)
    char *fname;
    long *size;
{
    char     cmd[MAXPATH+1], prefix[1024], *fnp;
    long     l;
    PIPE_S  *syspipe;

    dprint(1, (debugfile, "Upload cmd called to xfer \"%s\"\n",
	       fname ? fname : "<NO FILE>"));

    if(!fname)				/* no place for file name */
      return(0);

    if(!*fname){			/* caller wants temp file */
	strcpy(fname, fnp = temp_nam(NULL, "pu"));
	fs_give((void **)&fnp);
    }

    build_updown_cmd(cmd, ps_global->VAR_UPLOAD_CMD_PREFIX,
		     ps_global->VAR_UPLOAD_CMD, fname);
    if(syspipe = open_system_pipe(cmd, NULL, NULL, PIPE_USER | PIPE_RESET)){
	(void) close_system_pipe(&syspipe);
	if((l = name_file_size(fname)) < 0L){
	    q_status_message2(SM_ORDER | SM_DING, 3, 4,
			      "Error determining size of %s: %s", fname,
			      fnp = error_description(errno));
	    dprint(1, (debugfile,
		       "!!! Upload cmd \"%s\" failed for \"%s\": %s\n",
		       cmd, fname, fnp));
	}
	else if(size)
	  *size = l;

	return(l >= 0);
    }
    else
      q_status_message(SM_ORDER | SM_DING, 3, 4, "Error opening pipe");

    return(0);
}



/*----------------------------------------------------------------------
  Call back for pico to tell us the window size's changed

  Args: none

  Returns: none (but pine's ttyo structure may have been updated)
----*/
void
resize_for_pico()
{
    fix_windsize(ps_global);
}




/*----------------------------------------------------------------------
    Pass the first text segment of the message thru the "send filter"
     
Args: body pointer and address for storage object of old data

Returns: returns 1 on success, zero on error.
----*/      
int
filter_message_text(fcmd, outgoing, body, old)
    char      *fcmd;
    ENVELOPE  *outgoing;
    BODY      *body;
    STORE_S  **old;
{
    char     *cmd, *tmpf = NULL, *resultf = NULL, *errstr = NULL;
    int	      key = 0, rv;
    gf_io_t   gc, pc;
    STORE_S **so = (STORE_S **)((body->type == TYPEMULTIPART)
				? &body->contents.part->body.contents.binary
				: &body->contents.binary),
	     *tmp_so = NULL, *tmpf_so;

    if(fcmd && (cmd=expand_filter_tokens(fcmd,outgoing,&tmpf,&resultf,&key))){
	if(tmpf){
	    if(tmpf_so = so_get(FileStar, tmpf, EDIT_ACCESS)){
#ifndef	DOS
		chmod(tmpf, 0600);
#endif
		so_seek(*so, 0L, 0);
		gf_set_so_readc(&gc, *so);
		gf_set_so_writec(&pc, tmpf_so);
		gf_filter_init();
		if(key){
		    so_puts(tmpf_so, filter_session_key());
		    so_puts(tmpf_so, NEWLINE);
		}

		errstr = gf_pipe(gc, pc);
		so_give(&tmpf_so);
	    }
	    else
	      errstr = "Can't create space for filter temporary file.";
	}

	if(!errstr){
	    if(tmp_so = so_get(PicoText, NULL, EDIT_ACCESS)){
		gf_set_so_writec(&pc, tmp_so);
		ps_global->mangled_screen = 1;
		suspend_busy_alarm();
		ClearScreen();
		fflush(stdout);
		if(tmpf){
		    PIPE_S *fpipe;

		    if(fpipe = open_system_pipe(cmd, NULL, NULL,
						PIPE_NOSHELL | PIPE_RESET)){
			if(close_system_pipe(&fpipe) == 0){
			    if(tmpf_so = so_get(FileStar, tmpf, READ_ACCESS)){
				gf_set_so_readc(&gc, tmpf_so);
				gf_filter_init();
				errstr = gf_pipe(gc, pc);
				so_give(&tmpf_so);
			    }
			    else
			      errstr = "Can't open temp file filter wrote.";
			}
			else
			  errstr = "Filter command returned error.";
		    }
		    else
		      errstr = "Can't exec filter text.";
		}
		else
		  errstr = gf_filter(cmd, key ? filter_session_key() : NULL,
				     *so, pc, NULL);

		if(errstr){
		    int ch;

		    fprintf(stdout, "\r\n%s  Hit return to continue.", errstr);
		    fflush(stdout);
		    while((ch = read_char(300)) != ctrl('M')
			  && ch != NO_OP_IDLE)
		      putchar(BELL);
		}
		else{
		    BODY *b = (body->type == TYPEMULTIPART)
					   ? &body->contents.part->body : body;

		    *old = *so;			/* save old so */
		    *so = tmp_so;		/* return new one */

		    /*
		     * Reevaluate the encoding in case the data form's
		     * changed...
		     */
		    b->encoding = ENCOTHER;
		    set_mime_type_by_grope(b);
		}

		ClearScreen();
		resume_busy_alarm();
	    }
	    else
	      errstr = "Can't create space for filtered text.";
	}

	fs_give((void **)&cmd);
    }
    else
      return(rv == 0);

    if(tmpf){
	unlink(tmpf);
	fs_give((void **)&tmpf);
    }

    if(resultf){
	if(name_file_size(resultf) > 0L)
	  display_output_file(resultf, "Filter", NULL);

	fs_give((void **)&resultf);
    }
    else if(errstr){
	if(tmp_so)
	  so_give(&tmp_so);

	q_status_message1(SM_ORDER | SM_DING, 3, 6, "Problem filtering: %s",
			  errstr);
	dprint(1, (debugfile, "Filter FAILED: %s\n", errstr));
    }

    return(errstr == NULL);
}



/*----------------------------------------------------------------------
     Generate and send a message back to the pine development team
     
Args: none

Returns: none
----*/      
void
phone_home()
{
    int	      loser;
    char      tmp[MAX_ADDRESS], *from_addr;
    ENVELOPE *outgoing;
    BODY     *body;

#if defined(DOS) || defined(OS2)
    if(!dos_valid_from(0))
      return;
#endif

    outgoing = mail_newenvelope();
    sprintf(tmp, "pine%s@%s", PHONE_HOME_VERSION, PHONE_HOME_HOST);
    rfc822_parse_adrlist(&outgoing->to, tmp, ps_global->maildomain);
    outgoing->message_id  = generate_message_id(ps_global);
    outgoing->subject	  = cpystr("Document Request");

    body       = mail_newbody();
    body->type = TYPETEXT;

    if(body->contents.binary = (void *)so_get(PicoText,NULL,EDIT_ACCESS)){
	so_puts((STORE_S *)body->contents.binary, "Document request: Pine");
	so_puts((STORE_S *)body->contents.binary, PHONE_HOME_VERSION);
	if(ps_global->first_time_user)
	  so_puts((STORE_S *)body->contents.binary, " for New Users");

	if(ps_global->VAR_INBOX_PATH && ps_global->VAR_INBOX_PATH[0] == '{')
	  so_puts((STORE_S *)body->contents.binary, " and IMAP");

	if(ps_global->VAR_NNTP_SERVER && ps_global->VAR_NNTP_SERVER[0]
	      && ps_global->VAR_NNTP_SERVER[0][0])
	  so_puts((STORE_S *)body->contents.binary, " and NNTP");

	loser = pine_simple_send(outgoing, &body, NULL, NULL, NULL, 0);

	q_status_message2(SM_ORDER, 0, 3,"%sequest sent from \"%s\"",
			  (loser) ? "No r" : "R",
			  from_addr = addr_list_string(outgoing->from,NULL,1));
	fs_give((void **)&from_addr);
    }
    else
      q_status_message(SM_ORDER | SM_DING, 3, 4,
		       "Problem creating space for message text.");

    mail_free_envelope(&outgoing);
    pine_free_body(&body);

}


/*----------------------------------------------------------------------
     Call the mailer, SMTP, sendmail or whatever
     
Args: header -- full header (envelope and local parts) of message to send
      body -- The full body of the message including text
      verbose -- flag to indicate verbose transaction mode requested

Returns: -1 if failed, 1 if succeeded
----*/      
int
call_mailer(header, body)
    METAENV *header;
    BODY    *body;
{
    char         error_buf[100], *error_mess = NULL, *postcmd = NULL;
    ADDRESS     *a;
    ENVELOPE	*fake_env = NULL;
    int          addr_error_count, we_cancel = 0;
    long	 smtp_opts = 0L;
    char	*verbose_file = NULL;
    PIPE_S	*postpipe;
    BODY	*bp = NULL;
    PINEFIELD	*pf;

#define MAX_ADDR_ERROR 2  /* Only display 2 address errors */

    dprint(4, (debugfile, "Sending mail...\n"));

    /* Check for any recipients */
    for(pf = header->local; pf && pf->name; pf = pf->next)
      if(pf->type == Address && pf->rcptto && pf->addr && *pf->addr)
	break;

    if(!pf){
	q_status_message(SM_ORDER,3,3,
	    "Can't send message. No recipients specified!");
	return(0);
    }

    /* set up counts and such to keep track sent percentage */
    send_bytes_sent = 0;
    gf_filter_init();				/* zero piped byte count, 'n */
    send_bytes_to_send = send_body_size(body);	/* count body bytes	     */
    ps_global->c_client_error[0] = error_buf[0] = '\0';
    we_cancel = busy_alarm(1, "Sending mail",
			   send_bytes_to_send ? sent_percent : NULL, 1);

    /* try posting via local "<mta> <-t>" if specified */
    if(mta_handoff(header, body, error_buf)){
	if(error_buf[0])
	  error_mess = error_buf;

	goto done;
    }

    /*
     * If the user's asked for it, and we find that the first text
     * part (attachments all get b64'd) is non-7bit, ask for ESMTP.
     */
    if(F_ON(F_ENABLE_8BIT, ps_global) && (bp = first_text_8bit(body)))
       smtp_opts |= SOP_ESMTP;

#ifdef	DEBUG
    if(debug > 5 || verbose_requested)
      smtp_opts |= SOP_DEBUG;
#endif


    /*
     * Set global header pointer so post_rfc822_output can get at it when
     * it's called back from c-client's sending routine...
     */
    send_header = header;

    /*
     * Fabricate a fake ENVELOPE to hand c-client's SMTP engine.
     * The purpose is to give smtp_mail the list for SMTP RCPT when
     * there are recipients in pine's METAENV that are outside c-client's
     * envelope.
     *  
     * NOTE: If there aren't any, don't bother.  Dealt with it below.
     */
    for(pf = header->local; pf && pf->name; pf = pf->next)
      if(pf->type == Address && pf->rcptto && pf->addr && *pf->addr
	 && !(*pf->addr == header->env->to || *pf->addr == header->env->cc
	      || *pf->addr == header->env->bcc))
	break;

    if(pf && pf->name){
	ADDRESS **tail;

	fake_env = (ENVELOPE *)fs_get(sizeof(ENVELOPE));
	memset(fake_env, 0, sizeof(ENVELOPE));
	fake_env->return_path = rfc822_cpy_adr(header->env->return_path);
	tail = &(fake_env->to);
	for(pf = header->local; pf && pf->name; pf = pf->next)
	  if(pf->type == Address && pf->rcptto && pf->addr && *pf->addr){
	      *tail = rfc822_cpy_adr(*pf->addr);
	      while(*tail)
		tail = &((*tail)->next);
	  }
    }

    /*
     * Install our rfc822 output routine 
     */
    sending_hooks.rfc822_out = mail_parameters(NULL, GET_RFC822OUTPUT, NULL);
    (void)mail_parameters(NULL, SET_RFC822OUTPUT, (void *)post_rfc822_output);

    /*
     * Allow for verbose posting
     */
    (void)mail_parameters(NULL, SET_POSTVERBOSE,(void *)pine_smtp_verbose_out);

    /*
     * OK, who posts what?  We tried an mta_handoff above, but there
     * was either none specified or we decided not to use it.  So,
     * if there's an smtp-server defined anywhere, 
     */
    if(ps_global->VAR_SMTP_SERVER && ps_global->VAR_SMTP_SERVER[0]
       && ps_global->VAR_SMTP_SERVER[0][0]){
	/*---------- SMTP ----------*/
	dprint(4, (debugfile, "call_mailer: via TCP\n"));
	ps_global->noshow_error = 1;
	TIME_STAMP("smtp-open start (tcp)", 1);
	sending_stream = smtp_open(ps_global->VAR_SMTP_SERVER, smtp_opts);
	ps_global->noshow_error = 0;
    }
    else if(postcmd = smtp_command(ps_global->c_client_error)){
	/*----- Send via LOCAL SMTP agent ------*/
	dprint(4, (debugfile, "call_mailer: via pipe\n"));
	sending_hooks.soutr = mail_parameters(NULL, GET_POSTSOUTR, NULL);
	sending_hooks.getline = mail_parameters(NULL, GET_POSTGETLINE, NULL);
	sending_hooks.close = mail_parameters(NULL, GET_POSTCLOSE, NULL);
	(void)mail_parameters(NULL, SET_POSTSOUTR, (void *)pine_pipe_soutr);
	(void)mail_parameters(NULL, SET_POSTGETLINE,(void *)pine_pipe_getline);
	(void)mail_parameters(NULL, SET_POSTCLOSE, (void *)pine_pipe_close);

/* BUG: should provide separate stderr output! */
	TIME_STAMP("smtp-open start (pipe)", 1);
	if(postpipe = open_system_pipe(postcmd, NULL, NULL,
	   PIPE_READ|PIPE_STDERR|PIPE_WRITE|PIPE_PROT|PIPE_NOSHELL|PIPE_DESC)){
	    sending_stream = (SMTPSTREAM *) fs_get (sizeof (SMTPSTREAM));
	    memset(sending_stream, 0, sizeof(SMTPSTREAM));
	    sending_stream->tcpstream = (void *) postpipe;
	    TIME_STAMP("smtp greeting (pipe)", 1);
	    if(!smtp_greeting(sending_stream, "localhost", smtp_opts)){
		smtp_close(sending_stream);
		sending_stream = NULL;
	    }
	}
	else{
	    dprint(1, (debugfile, "Send via pipe failed!\n"));
	    q_status_message(SM_ORDER | SM_DING, 4, 7, "Send Failed!");
	}
    }

    TIME_STAMP("smtp open", 1);
    if(sending_stream){
	dprint(1, (debugfile, "Opened SMTP server \"%s\"\n",
		   postcmd ? postcmd : tcp_host(sending_stream->tcpstream)));

	if(verbose_requested){
	    TIME_STAMP("verbose start", 1);
	    if(verbose_file = temp_nam(NULL, "sd")){
		if(verbose_send_output = fopen(verbose_file, "w")){
		    if(!pine_smtp_verbose(sending_stream))
		      sprintf(error_mess = error_buf,
			      "Mail not sent.  VERBOSE mode error%s%.50s.",
			      (sending_stream && sending_stream->reply)
			        ? ": ": "",
			      (sending_stream && sending_stream->reply)
			        ? sending_stream->reply : "");
		}
		else
		  strcpy(error_mess = error_buf,
			 "Can't open tmp file for VERBOSE mode.");
	    }
	    else
	      strcpy(error_mess = error_buf,
		     "Can't create tmp file name for VERBOSE mode.");

	    TIME_STAMP("verbose end", 1);
	}

	/*
	 * Before we actually send data, see if we have to protect
	 * the first text body part from getting encoded.  We protect
	 * it from getting encoded in "pine_rfc822_output_body" by
	 * temporarily inventing a synonym for ENC8BIT...
	 */
	if(bp && sending_stream->ok_8bitmime){
	    int i;

	    for(i = 0; (i <= ENCMAX) && body_encodings[i]; i++)
	      ;

	    if(i > ENCMAX){		/* no empty encoding slots! */
		bp = NULL;
	    }
	    else {
		body_encodings[i] = body_encodings[ENC8BIT];
		bp->encoding = (unsigned short) i;
	    }
	}

	TIME_STAMP("smtp start", 1);
	if(!error_mess && !smtp_mail(sending_stream, "MAIL",
				     fake_env ? fake_env : header->env, body)){
	    struct headerentry *last_he = NULL;

	    sprintf(error_buf,
		    "Mail not sent. Sending error%s%.40s",
		    (sending_stream && sending_stream->reply) ? ": ": ".",
		    (sending_stream && sending_stream->reply)
		      ? sending_stream->reply : "");
	    dprint(1, (debugfile, error_buf));
	    addr_error_count = 0;
	    for(pf = header->local; pf && pf->name; pf = pf->next)
	      if(pf->type == Address && pf->rcptto && pf->addr && *pf->addr)
		for(a = *pf->addr; a != NULL; a = a->next)
		  if(a->error != NULL){
		      if(addr_error_count++ < MAX_ADDR_ERROR){
			  if(pf->he){
			      if(last_he)	/* start last reported err */
				last_he->start_here = 0;

			      (last_he = pf->he)->start_here = 1;
			  }

			  if(error_mess)	/* previous error? */
			    q_status_message(SM_ORDER, 4, 7, error_mess);

			  error_mess = tidy_smtp_mess(a->error,
						      "Mail not sent: %.60s",
						      error_buf);
		      }

		      dprint(1, (debugfile, "Send Error: \"%s\"\n",
				 a->error));
		  }

	    if(!error_mess)
	      error_mess = error_buf;
	}

	/* repair modified "body_encodings" array? */
	if(bp && sending_stream->ok_8bitmime)
	  body_encodings[bp->encoding] = NULL;

	TIME_STAMP("smtp closing", 1);
	smtp_close(sending_stream);
	sending_stream = NULL;
	TIME_STAMP("smtp done", 1);
    }
    else if(!error_mess)
      sprintf(error_mess = error_buf, "Error sending: %.60s",
	      ps_global->c_client_error);

    if(verbose_file){
	if(verbose_send_output){
	    TIME_STAMP("verbose start", 1);
	    fclose(verbose_send_output);
	    verbose_send_output = NULL;
	    q_status_message(SM_ORDER, 0, 3, "Verbose SMTP output received");
	    display_output_file(verbose_file, "Verbose SMTP Interaction",NULL);
	    TIME_STAMP("verbose end", 1);
	}

	fs_give((void **)&verbose_file);
    }

    /*
     * Restore original 822 emitter...
     */
    (void) mail_parameters(NULL, SET_RFC822OUTPUT, sending_hooks.rfc822_out);
    if(postcmd)
      fs_give((void **)&postcmd);

    if(fake_env)
      mail_free_envelope(&fake_env);

  done:
    if(we_cancel)
      cancel_busy_alarm(0);

    TIME_STAMP("call_mailer done", 1);
    /*-------- Did message make it ? ----------*/
    if(error_mess){
        /*---- Error sending mail -----*/
	if(local_so && !local_written)
	  so_give(&local_so);

        q_status_message(SM_ORDER | SM_DING, 4, 7, error_mess);
	dprint(1, (debugfile, "call_mailer ERROR: %s\n", error_mess));
	return(-1);
    }
    else{
	local_written = 1;
	return(1);
    }
}


/*----------------------------------------------------------------------
    Checks to make sure the fcc is available and can be opened

Args: fcc -- the name of the fcc to create.  It can't be NULL.
      fcc_cntxt -- Returns the context the fcc is in.
      force -- supress user option prompt

Returns 0 on failure, 1 on success
  ----*/
open_fcc(fcc, fcc_cntxt, force)
    char       *fcc;
    CONTEXT_S **fcc_cntxt;
    int 	 force;
{
    MAILSTREAM *create_stream;
    int		ok = 1;

    *fcc_cntxt = NULL;

    /* 
     * check for fcc's existance...
     */
    TIME_STAMP("open_fcc start", 1);
    if(context_isambig(fcc)){
	int flip_dot = 0;

	/*
	 * Don't want to preclude a user from Fcc'ing a .name'd folder
	 */
	if(F_OFF(F_ENABLE_DOT_FOLDERS, ps_global)){
	    flip_dot = 1;
	    F_TURN_ON(F_ENABLE_DOT_FOLDERS, ps_global);
	}

	/*
	 * We only want to set the "context" if fcc is an ambiguous
	 * name.  Otherwise, our "relativeness" rules for contexts 
	 * (implemented in context.c) might cause the name to be
	 * interpreted in the wrong context...
	 */
	if(!(*fcc_cntxt = default_save_context(ps_global->context_list)))
	  *fcc_cntxt = ps_global->context_list;

        find_folders_in_context(NULL, *fcc_cntxt, fcc);
        if(folder_index(fcc, (*fcc_cntxt)->folders) < 0){
	    if(ps_global->context_list->next){
		sprintf(tmp_20k_buf,
			"Folder \"%.20s\" in <%.30s> doesn't exist. Create",
			fcc, (*fcc_cntxt)->label[0]);
	    }
	    else
	      sprintf(tmp_20k_buf,"Folder \"%.40s\" doesn't exist.  Create",
		      fcc);

	    if(!force && want_to(tmp_20k_buf, 'y', 'n', NO_HELP, 0, 0) != 'y'){
		q_status_message(SM_ORDER | SM_DING, 3, 4,
				 "Fcc of message rejected");
		ok = 0;
	    }
	    else{
		/*
		 * See if an already open stream will service the create
		 */
		create_stream = context_same_stream((*fcc_cntxt)->context,
						    fcc,
						    ps_global->mail_stream);
		if(!create_stream
		   && ps_global->mail_stream != ps_global->inbox_stream)
		  create_stream = context_same_stream((*fcc_cntxt)->context,
						      fcc,
						      ps_global->inbox_stream);

		if(!context_create((*fcc_cntxt)->context, create_stream, fcc))
		  ok = 0;
	    }
        }

	if(flip_dot)
	  F_TURN_OFF(F_ENABLE_DOT_FOLDERS, ps_global);

        free_folders_in_context(*fcc_cntxt);
        if(!ok){
	    TIME_STAMP("open_fcc done.", 1);
	    return(0);
	}
    }
    else if((ok = folder_exists(NULL, fcc)) <= 0){
        sprintf(tmp_20k_buf,"Folder \"%.40s\" doesn't exist.  Create",fcc);

        if(ok < 0
	   || (!force && want_to(tmp_20k_buf, 'y', 'n', NO_HELP,0,0) != 'y')){
	    q_status_message(SM_ORDER | SM_DING, 3, 3,
			     "Fcc of message rejected");
	    TIME_STAMP("open_fcc done.", 1);
	    return(0);
	}

	/*
	 * See if an already open stream will service the create
	 */
	create_stream = same_stream(fcc, ps_global->mail_stream);
	if(!create_stream && ps_global->mail_stream != ps_global->inbox_stream)
	  create_stream = same_stream(fcc, ps_global->inbox_stream);

        if(!mail_create(create_stream, fcc)){
	    TIME_STAMP("open_fcc done.", 1);
	    return(0);
	}
    }

    TIME_STAMP("open_fcc done.", 1);
    return(1);
}


/*----------------------------------------------------------------------
   mail_append() the fcc accumulated in temp_storage to proper destination

Args:  fcc -- name of folder
       fcc_cntxt -- context for folder
       temp_storage -- String of file where Fcc has been accumulated

This copies the string of file to the actual folder, which might be IMAP
or a disk folder.  The temp_storage is freed after it is written.
An error message is produced if this fails.
  ----*/
int
write_fcc(fcc, fcc_cntxt, tmp_storage, label)
     char      *fcc;
     CONTEXT_S *fcc_cntxt;
     STORE_S   *tmp_storage;
     char      *label;
{
    STRING      msg;
    MAILSTREAM *fcc_stream;
    char       *cntxt_string;
    int         we_cancel = 0;
#ifdef	DOS
    struct {			/* hack! stolen from dawz.c */
	int fd;
	unsigned long pos;
    } d;
    extern STRINGDRIVER dawz_string;
#endif

    if(!tmp_storage)
      return(0);

    TIME_STAMP("write_fcc start.", 1);
    dprint(4, (debugfile, "Writing %s\n", (label && *label) ? label : ""));
    if(label && *label){
	char msg_buf[80];

	strncat(strcpy(msg_buf, "Writing "), label, 70);
	we_cancel = busy_alarm(1, msg_buf, NULL, 1);
    }
    else
      we_cancel = busy_alarm(1, NULL, NULL, 0);

    so_seek(tmp_storage, 0L, 0);
#ifdef	DOS
    d.fd  = fileno((FILE *)so_text(tmp_storage));
    d.pos = 0L;
    INIT(&msg, dawz_string, (void *)&d, filelength(d.fd));
#else
    INIT(&msg, mail_string, (void *)so_text(tmp_storage), 
	     strlen((char *)so_text(tmp_storage)));
#endif

    cntxt_string = fcc_cntxt ? fcc_cntxt->context : "[]";
    fcc_stream   = context_same_stream(cntxt_string, fcc,
				       ps_global->mail_stream);

    if(!fcc_stream && ps_global->mail_stream != ps_global->inbox_stream)
      fcc_stream = context_same_stream(cntxt_string, fcc,
				       ps_global->inbox_stream);

    if(!context_append(cntxt_string, fcc_stream, fcc, &msg)){
	if(we_cancel)
	  cancel_busy_alarm(-1);

	q_status_message1(SM_ORDER | SM_DING, 3, 5,
			  "Write to \"%s\" FAILED!!!", fcc);
	dprint(1, (debugfile, "ERROR appending %s in \"%s\"",
		   fcc, cntxt_string));
	return(0);
    }

    if(we_cancel)
      cancel_busy_alarm(label ? 0 : -1);

    dprint(4, (debugfile, "done.\n"));
    TIME_STAMP("write_fcc done.", 1);
    return(1);
}

  

/*
 * first_text_8bit - return TRUE if somewhere in the body 8BIT data's
 *		     contained.
 */
BODY *
first_text_8bit(body)
    BODY *body;
{
    if(body->type == TYPEMULTIPART)	/* advance to first contained part */
      body = &body->contents.part->body;

    return((body->type == TYPETEXT && body->encoding != ENC7BIT)
	     ? body : NULL);
}



/*----------------------------------------------------------------------
    Remove the leading digits from SMTP error messages
 -----*/
char *
tidy_smtp_mess(error, printstring, outbuf)
    char *error, *printstring, *outbuf;
{
    while(isdigit((unsigned char)*error) || isspace((unsigned char)*error))
      error++;

    sprintf(outbuf, printstring, error);
    return(outbuf);
}

        
    
/*----------------------------------------------------------------------
    Set up fields for passing to pico.  Assumes first text part is
    intended to be passed along for editing, and is in the form of
    of a storage object brought into existence sometime before pico_send().
 -----*/
void
outgoing2strings(header, bod, text, pico_a)
    METAENV   *header;
    BODY      *bod;
    void     **text;
    PATMT    **pico_a;
{
    PART      *part;
    PATMT     *pa;
    char      *type;
    PINEFIELD *pf;
    PARAMETER *parms;

    /*
     * SIMPLIFYING ASSUMPTION #37: the first TEXT part's storage object
     * is guaranteed to be of type PicoText!
     */
    if(bod->type == TYPETEXT){
	*text = so_text(bod->contents.binary);
    } else if(bod->type == TYPEMULTIPART){
	/*
	 * We used to jump out the window if the first part wasn't text,
	 * but that may not be the case when bouncing a message with
	 * a leading non-text segment.  So, IT'S UNDERSTOOD that the 
	 * contents of the first part to send is still ALWAYS in a 
	 * PicoText storage object, *AND* if that object doesn't contain
	 * data of type text, then it must contain THE ENCODED NON-TEXT
	 * DATA of the piece being sent.
	 *
	 * It's up to the programmer to make sure that such a message is
	 * sent via pine_simple_send and never get to the composer via
	 * pine_send.
	 *
	 * Make sense?
	 */
	*text = so_text(bod->contents.part->body.contents.binary);

	/*
	 * If we already had a list, blast it now, so we can build a new
	 * attachment list that reflects what's really there...
	 */
	if(pico_a)
	  free_attachment_list(pico_a);


        /* Simplifyihg assumption #28e. (see cross reference) 
           All parts in the body passed in here that are not already
           in the attachments list are added to the end of the attachments
           list. Attachment items not in the body list will be taken care
           of in strings2outgoing, but they are unlikey to occur
         */

        for(part = bod->contents.part->next; part != NULL; part = part->next) {
            for(pa = *pico_a; pa != NULL; pa = pa->next) {
                if(strcmp(pa->id, part->body.id) == 0)
                  break;  /* Already in list */
            }
            if(pa != NULL) /* Was in the list, don't worry 'bout this part */
              continue;

            /* to end of list */
            for(pa = *pico_a;  pa!= NULL && pa->next != NULL;  pa = pa->next);
            /* empty list or no? */
            if(pa == NULL) {
                /* empty list */
                *pico_a = (PATMT *)fs_get(sizeof(PATMT));
                pa = *pico_a;
            } else {
                pa->next = (PATMT *)fs_get(sizeof(PATMT));
                pa = pa->next;
            }
            pa->description = part->body.description == NULL ? cpystr("") : 
                                              cpystr(part->body.description);
            
            type = type_desc(part->body.type,
			     part->body.subtype,part->body.parameter,0);

	    /*
	     * If we can find a "name" parm, display that too...
	     */
	    for(parms = part->body.parameter; parms; parms = parms->next)
	      if(!strucmp(parms->attribute, "name") && parms->value)
		break;

            pa->filename = fs_get(strlen(type)
				  + (parms ? strlen(parms->value) : 0) + 5);

            sprintf(pa->filename, "[%s%s%s]", type,
		    parms ? ": " : "", 
		    parms ? parms->value : "");
            pa->flags    = A_FLIT;
            pa->size     = cpystr(byte_string(part->body.size.bytes));
            if(part->body.id == NULL)
              part->body.id = generate_message_id(ps_global);
            pa->id       = cpystr(part->body.id);
            pa->next     = NULL;
        }
    }
        

    /*------------------------------------------------------------------
       Malloc strings to pass to composer editor because it expects
       such strings so it can realloc them
      -----------------------------------------------------------------*/
    /*
     * turn any address fields into text strings
     */
    /*
     * SIMPLIFYING ASSUMPTION #116: all header strings are understood
     * NOT to be RFC1522 decoded.  Said differently, they're understood
     * to be RFC1522 ENCODED as necessary.  The intent is to preserve
     * original charset tagging as far into the compose/send pipe as
     * we can.
     */
    for(pf = header->local; pf && pf->name; pf = pf->next)
      if(pf->canedit)
	switch(pf->type){
	  case Address :
	    if(pf->addr){
		char *p, *t, *u;
		long  l;

		pf->scratch = addr_list_string(*pf->addr, NULL, 0);

		/*
		 * Scan for and fix-up patently bogus fields.
		 *
		 * NOTE: collaboration with this code and what's done in
		 * reply.c:reply_cp_addr to package up the bogus stuff
		 * is required.
		 */
		for(p = pf->scratch; p = strstr(p, "@.RAW-FIELD."); )
		  for(t = p; ; t--)
		    if(*t == '&'){		/* find "leading" token */
			*t++ = ' ';		/* replace token */
			*p = '\0';		/* tie off string */
			u = rfc822_base64((unsigned char *) t,
					  (unsigned long) strlen(t),
					  (unsigned long *) &l);
			*p = '@';		/* restore 'p' */
			rplstr(p, 12, "");	/* clear special token */
			rplstr(t, strlen(t), u);
			fs_give((void **) &u);
			if(pf->he)
			  pf->he->start_here = 1;

			break;
		    }
		    else if(t == pf->scratch)
		      break;
	    }

	    break;

	  case Subject :
	    if(pf->text){
		if(pf->scratch){
		    char *p, *charset = NULL;

		    p = (char *)fs_get((strlen(pf->scratch)+1)*sizeof(char));
		    if(rfc1522_decode((unsigned char *)p, pf->scratch,&charset)
						       == (unsigned char *) p){
			fs_give((void **)&pf->scratch);
			pf->scratch = p;
		    }
		    else
		      fs_give((void **)&p);

		    if(charset)
		      fs_give((void **)&charset);
		}
		else
		  pf->scratch = cpystr((*pf->text) ? *pf->text : "");
	    }

	    break;
	}
}


/*----------------------------------------------------------------------
    Restore fields returned from pico to form useful to sending
    routines.
 -----*/
void
strings2outgoing(header, bod, attach)
    METAENV  *header;
    BODY    **bod;
    PATMT    *attach;
{
    PINEFIELD *pf;
    int we_cancel = 0;

    we_cancel = busy_alarm(1, NULL, NULL, 0);

    /*
     * turn any local address strings into address lists
     */
    for(pf = header->local; pf && pf->name; pf = pf->next)
      if(pf->scratch){
	  if(pf->canedit && (!pf->he || pf->he->dirty)){
	      switch(pf->type){
		case Address :
		  removing_trailing_white_space(pf->scratch);
		  if(*pf->scratch){
		      ADDRESS     *new_addr = NULL;
		      static char *fakedomain = "@";

		      rfc822_parse_adrlist(&new_addr, pf->scratch,
				   (F_ON(F_COMPOSE_REJECTS_UNQUAL, ps_global))
				     ? fakedomain : ps_global->maildomain);
		      resolve_encoded_entries(new_addr, *pf->addr);
		      mail_free_address(pf->addr);	/* free old addrs */
		      *pf->addr = new_addr;		/* assign new addr */
		  }
		  else
		    mail_free_address(pf->addr);	/* free old addrs */

		  break;

		case Subject :
		  if(*pf->text)
		    fs_give((void **)pf->text);

		  if(*pf->scratch)
		    *pf->text = cpystr(pf->scratch);

		  break;
	      }
	  }

	  fs_give((void **)&pf->scratch);	/* free now useless text */
      }

    create_message_body(bod, attach);
    pine_encode_body(*bod);

    if(we_cancel)
      cancel_busy_alarm(-1);
}


/*----------------------------------------------------------------------
 -----*/
void
resolve_encoded_entries(new, old)
    ADDRESS *new, *old;
{
    ADDRESS *a;

    /* BUG: deal properly with group syntax? */
    for(; old; old = old->next)
      if(old->personal && old->mailbox && old->host)
	for(a = new; a; a = a->next)
	  if(a->personal && a->mailbox && !strcmp(old->mailbox, a->mailbox)
	     && a->host && !strcmp(old->host, a->host)){
	      char *charset = NULL, *p;

	      /*
	       * if we actually found 1522 in the personal name, then
	       * make sure the decoded personal name matches the old 
	       * un-encoded name.  If not, replace the new string with
	       * with the old one.  If 1522 isn't involved, just trust
	       * the change.
	       */
	      p = (char *) rfc1522_decode((unsigned char *)tmp_20k_buf,
					  old->personal, &charset);
	      if(p == tmp_20k_buf		/* personal was decoded */
		 && !strcmp(a->personal, p)){
		  fs_give((void **)&a->personal);
		  a->personal = cpystr(old->personal);
	      }

	      if(charset)
		fs_give((void **)&charset);

	      break;
	  }
}


/*----------------------------------------------------------------------

 The head of the body list here is always either TEXT or MULTIPART. It may be
changed from TEXT to MULTIPART if there are attachments to be added
and it is not already multipart. 
  ----*/
void
create_message_body(b, attach)
    BODY  **b;
    PATMT  *attach;
{
    PART         *p, *p_trail;
    PATMT        *pa;
    BODY         *b1;
    void         *file_contents;
    PARAMETER    *pm;
    char         *lc;

    TIME_STAMP("create_body start.", 1);
    if((*b)->type != TYPEMULTIPART && !attach){
	/* only override assigned encoding if it might need upgrading */
	if((*b)->type == TYPETEXT && (*b)->encoding == ENC7BIT)
	  (*b)->encoding = ENCOTHER;

	set_mime_type_by_grope(*b);
	set_body_size(*b);
	TIME_STAMP("create_body end.", 1);
        return;
    }

    if((*b)->type == TYPETEXT) {
        /*-- Current type is text, but there are attachments to add --*/
        /*-- Upgrade to a TYPEMULTIPART --*/
        b1                                  = (BODY *)mail_newbody();
        b1->type                            = TYPEMULTIPART;
        b1->contents.part                   = mail_newbody_part();
        b1->contents.part->body             = **b;

        (*b)->subtype = (*b)->id = (*b)->description = NULL;
	(*b)->parameter = NULL;
	(*b)->contents.binary               = NULL;
	pine_free_body(b);
        *b = b1;
    }

    /*-- Now type must be MULTIPART with first part text --*/
    (*b)->contents.part->body.encoding = ENCOTHER;
    set_mime_type_by_grope(&((*b)->contents.part->body));
    set_body_size(&((*b)->contents.part->body));

    /*------ Go through the parts list remove those to be deleted -----*/
    for(p = p_trail = (*b)->contents.part->next; p != NULL;) {
	for(pa = attach; pa && p->body.id; pa = pa->next)
	  if(pa->id && strcmp(pa->id, p->body.id) == 0){ /* already existed */
	      if(!p->body.description		/* update description? */
		 || strcmp(pa->description, p->body.description)){
		  if(p->body.description)
		    fs_give((void **)&p->body.description);

		  p->body.description = bitstrip(cpystr(pa->description));
	      }

	      break;
	  }

        if(pa == NULL) {
            /* attachment wasn't in the list; zap it */
            if(p == (*b)->contents.part->next) {
                /* Beginning of list */
                (*b)->contents.part->next = p->next;
                p->next = NULL;  /* Don't free the whole chain */
                pine_free_body_part(&p);
                p = p_trail = (*b)->contents.part->next;
            } else {
                p_trail->next = p->next;
                p->next = NULL;  /* Don't free the whole chain */
                pine_free_body_part(&p);
                p = p_trail->next;
            }
        } else {
            p_trail = p;
            p       = p->next;
        }
    }

    /*---------- Now add any new attachments ---------*/
    for(p = (*b)->contents.part ; p->next != NULL; p = p->next);
    for(pa = attach; pa != NULL; pa = pa->next) {
        if(pa->id != NULL)
	  continue;			/* Has an ID, it's old */

	/*
	 * the idea is handle ALL attachments as open FILE *'s.  Actual
         * encoding and such is handled at the time the message
         * is shoved into the mail slot or written to disk...
	 *
         * Also, we never unlink a file, so it's up to whoever opens
         * it to deal with tmpfile issues.
	 */
	if((file_contents = (void *)so_get(FileStar, pa->filename,
					   READ_ACCESS)) == NULL){
            q_status_message2(SM_ORDER | SM_DING, 3, 4,
                              "Error \"%s\", couldn't attach file \"%s\"",
                              error_description(errno), pa->filename);
            display_message('x');
            continue;
        }
        
        p->next                      = mail_newbody_part();
        p                            = p->next;
        p->body.id                   = generate_message_id(ps_global);
        p->body.contents.binary      = file_contents;

	/*
	 * Set type to unknown and let set_mime_type_by_* figure it out.
	 * Always encode attachments we add as BINARY.
	 */
	p->body.type		     = TYPEOTHER;
	p->body.encoding	     = ENCBINARY;
	p->body.size.bytes           = name_file_size(pa->filename);
	if(!set_mime_type_by_extension(&p->body, pa->filename))
	  set_mime_type_by_grope(&p->body);

	so_release((STORE_S *)p->body.contents.binary);
        p->body.description          = bitstrip(cpystr(pa->description));
	/* add name attribute */
	if (p->body.parameter == NULL) {
	    pm = p->body.parameter = mail_newbody_parameter();
	    pm->attribute = cpystr("name");
	}else {
            for (pm = p->body.parameter;
			strucmp(pm->attribute, "name") && pm->next != NULL;
								pm = pm->next);
	    if (strucmp(pm->attribute, "name") != 0) {
		pm->next = mail_newbody_parameter();
		pm = pm->next;
		pm->attribute = cpystr("name");
	    }
	}

	pm->value = bitstrip(cpystr((lc = last_cmpnt(pa->filename))
				      ? lc : pa->filename));

        p->next = NULL;
        pa->id = cpystr(p->body.id);
    }

    TIME_STAMP("create_body end.", 1);
}


/*
 * Build and return the "From:" address for outbound messages from
 * global data...
 */
ADDRESS *
generate_from()
{
    ADDRESS *addr = mail_newaddr();
    if(ps_global->VAR_PERSONAL_NAME)
      addr->personal = cpystr(ps_global->VAR_PERSONAL_NAME);

    addr->mailbox = cpystr(ps_global->VAR_USER_ID);
    addr->host    = cpystr(ps_global->maildomain);
    return(addr);
}


/*
 * free_attachment_list - free attachments in given list
 */
void
free_attachment_list(alist)
    PATMT  **alist;
{
    PATMT  *leading;

    while(alist && *alist){		/* pointer pointing to something */
	leading = (*alist)->next;
	if((*alist)->description)
          fs_give((void **)&(*alist)->description);

	if((*alist)->filename){
	    if((*alist)->flags & A_TMP)
	      unlink((*alist)->filename);

	    fs_give((void **)&(*alist)->filename);
	}

	if((*alist)->size)
          fs_give((void **)&(*alist)->size);

	if((*alist)->id)
          fs_give((void **)&(*alist)->id);

	fs_give((void **)alist);

	*alist = leading;
    }
}



/*----------------------------------------------------------------------
  Insert the addition into the message id before first "@"
 
 This may be called twice on the same message-ID, but it won't do anything
 the second time. This will cause the status in the message-ID to be wrong
 if an attempt to send the message is made, an error occurs, and then the
 size or MIME parts changed and it is sent again.
  ----*/
void 
update_message_id(e, addition)
    ENVELOPE *e;
    char *addition;
{
    char *p, *q, *r, *new;

    new = fs_get(strlen(e->message_id) + strlen(addition) + 5);
    for(p = new, q = e->message_id; *q && *q != '@'; *p++ = *q++);
    if(p > new && isdigit((unsigned char)*(p-1))) {
	/* Already been updated if it's a digit, not a letter */
	fs_give((void **)&new);
	return;
    }

    *p++ = '-';
    for(r = addition; *r ; *p++ = *r++);
    for(; *q; *p++ = *q++);
    *p = *q;
    fs_give((void **)&(e->message_id));
    e->message_id = new;
}




static struct mime_count {
    int text_parts;
    int image_parts;
    int message_parts;
    int application_parts;
    int audio_parts;
    int  video_parts;
} mc;

char *
mime_stats(body)
    BODY *body;
{
    static char id[10];
    mc.text_parts = 0;
    mc.image_parts = 0;
    mc.message_parts = 0;
    mc.application_parts = 0;
    mc.audio_parts = 0;
    mc.video_parts = 0;

    mime_recur(body);

    mc.text_parts        = min(8, mc.text_parts );
    mc.image_parts       = min(8, mc.image_parts );
    mc.message_parts     = min(8, mc.message_parts );
    mc.application_parts = min(8, mc.application_parts );
    mc.audio_parts       = min(8, mc.audio_parts );
    mc.video_parts       = min(8, mc.video_parts );


    id[0] = encode_bits(mc.text_parts);
    id[1] = encode_bits(mc.message_parts);
    id[2] = encode_bits(mc.application_parts);
    id[3] = encode_bits(mc.video_parts);
    id[4] = encode_bits(mc.audio_parts);
    id[5] = encode_bits(mc.image_parts);
    id[6] = '\0';
    return(id);
}
    


/*----------------------------------------------------------------------
   ----*/
void
mime_recur(body)
    BODY *body;
{
    PART *part;
    switch (body->type) {
      case TYPETEXT:
        mc.text_parts++;
        break;
      case TYPEIMAGE:
        mc.image_parts++;
        break;
      case TYPEMESSAGE:
        mc.message_parts++;
        break;
      case TYPEAUDIO:
        mc.audio_parts++;
        break;
      case TYPEAPPLICATION:
        mc.application_parts++;
        break;
      case TYPEVIDEO:
        mc.video_parts++;
        break;
      case TYPEMULTIPART:
        for(part = body->contents.part; part != NULL; part = part->next) 
          mime_recur(&(part->body));
        break;
    }
}
        
int        
encode_bits(bits)
    int bits;
{
    if(bits < 10)
      return(bits + '0');
    else if(bits < 36)
      return(bits - 10 + 'a');
    else if (bits < 62)
      return(bits - 36 + 'A');
    else
      return('.');
}


/*
 * set_mime_type_by_grope - sniff the given storage object to determine its 
 *                  type, subtype and encoding
 *
 *		"Type" and "encoding" must be set before calling this routine.
 *		If "type" is set to something other than TYPEOTHER on entry,
 *		then that is the "type" we wish to use.  Same for "encoding"
 *		using ENCOTHER instead of TYPEOTHER.  Otherwise, we
 *		figure them out here.  If "type" is already set, we also
 *		leave subtype alone.  If not, we figure out subtype here.
 *		There is a chance that we will upgrade "encoding" to a "higher"
 *		level.  For example, if it comes in as 7BIT we may change
 *		that to 8BIT if we find a From_ we want to escape.
 *
 * NOTE: this is rather inefficient if the store object is a CharStar
 *       but the win is all types are handled the same
 */
void
set_mime_type_by_grope(body)
    BODY *body;
{
#define RBUFSZ	(8193)
    unsigned char   *buf, *p, *bol;
    register size_t  n;
    long             max_line = 0L,
                     eight_bit_chars = 0L,
                     line_so_far = 0L,
                     len = 0L,
                     can_be_ascii = 1L;
    STORE_S         *so = (STORE_S *)body->contents.binary;
    unsigned short   new_encoding = ENCOTHER;
    int              we_cancel = 0;
#ifdef ENCODE_FROMS
    short            froms = 0, dots = 0,
                     bmap  = 0x1, dmap = 0x1;
#endif

    we_cancel = busy_alarm(1, NULL, NULL, 0);

#ifndef DOS
    buf = (unsigned char *)fs_get(RBUFSZ);
#else
    buf = (unsigned char *)tmp_20k_buf;
#endif
    so_seek(so, 0L, 0);

    for(n = 0; n < RBUFSZ-1 && so_readc(&buf[n], so) != 0; n++)
      ;

    buf[n] = '\0';

    if(n){    /* check first few bytes to look for magic numbers */
	if(body->type == TYPEOTHER){
	    if(buf[0] == 'G' && buf[1] == 'I' && buf[2] == 'F'){
		body->type    = TYPEIMAGE;
		body->subtype = cpystr("GIF");
	    }
	    else if((n > 9) && buf[0] == 0xFF && buf[1] == 0xD8
		    && buf[2] == 0xFF && buf[3] == 0xE0
		    && !strncmp((char *)&buf[6], "JFIF", 4)){
	        body->type    = TYPEIMAGE;
	        body->subtype = cpystr("JPEG");
	    }
	    else if((buf[0] == 'M' && buf[1] == 'M')
		    || (buf[0] == 'I' && buf[1] == 'I')){
		body->type    = TYPEIMAGE;
		body->subtype = cpystr("TIFF");
	    }
	    else if((buf[0] == '%' && buf[1] == '!')
		     || (buf[0] == '\004' && buf[1] == '%' && buf[2] == '!')){
		body->type    = TYPEAPPLICATION;
		body->subtype = cpystr("PostScript");
	    }
	    else if(buf[0] == '%' && !strncmp((char *)buf+1, "PDF-", 4)){
		body->type = TYPEAPPLICATION;
		body->subtype = cpystr("PDF");
	    }
	    else if(buf[0] == '.' && !strncmp((char *)buf+1, "snd", 3)){
		body->type    = TYPEAUDIO;
		body->subtype = cpystr("Basic");
	    }
	    else if((n > 3) && buf[0] == 0x00 && buf[1] == 0x05
		            && buf[2] == 0x16 && buf[3] == 0x00){
	        body->type    = TYPEAPPLICATION;
		body->subtype = cpystr("APPLEFILE");
	    }
	    else if((n > 3) && buf[0] == 0x50 && buf[1] == 0x4b
		            && buf[2] == 0x03 && buf[3] == 0x04){
	        body->type    = TYPEAPPLICATION;
		body->subtype = cpystr("ZIP");
	    }

	    /*
	     * if type was set above, but no encoding specified, go
	     * ahead and make it BASE64...
	     */
	    if(body->type != TYPEOTHER && body->encoding == ENCOTHER)
	      body->encoding = ENCBINARY;
	}
    }
    else{
	/* PROBLEM !!! */
	if(body->type == TYPEOTHER){
	    body->type = TYPEAPPLICATION;
	    body->subtype = cpystr("octet-stream");
	    if(body->encoding == ENCOTHER)
		body->encoding = ENCBINARY;
	}
    }

    if (body->encoding == ENCOTHER || body->type == TYPEOTHER){
#if defined(DOS) || defined(OS2) /* for binary file detection */
	int lastchar = '\0';
#define BREAKOUT 300   /* a value that a character can't be */
#endif

	p   = bol = buf;
	len = n;
	while (n--){
/* Some people don't like quoted-printable caused by leading Froms */
#ifdef ENCODE_FROMS
	    Find_Froms(froms, dots, bmap, dmap, *p);
#endif
	    if(*p == '\n'){
		max_line    = max(max_line, line_so_far + p - bol);
		bol	    = NULL;		/* clear beginning of line */
		line_so_far = 0L;		/* clear line count */
#if	defined(DOS) || defined(OS2)
		/* LF with no CR!! */
		if(lastchar != '\r')		/* must be non-text data! */
		  lastchar = BREAKOUT;
#endif
	    }
	    else if(*p == ctrl('O') || *p == ctrl('N') || *p == ESCAPE){
		can_be_ascii--;
	    }
	    else if(*p & 0x80){
		eight_bit_chars++;
	    }
	    else if(!*p){
		/* NULL found. Unless we're told otherwise, must be binary */
		if(body->type == TYPEOTHER){
		    body->type    = TYPEAPPLICATION;
		    body->subtype = cpystr("octet-stream");
		}

		/*
		 * The "TYPETEXT" here handles the case that the NULL
		 * comes from imported text generated by some external
		 * editor that permits or inserts NULLS.  Otherwise,
		 * assume it's a binary segment...
		 */
		new_encoding = (body->type==TYPETEXT) ? ENC8BIT : ENCBINARY;

		/*
		 * Since we've already set encoding, count this as a 
		 * hi bit char and continue.  The reason is that if this
		 * is text, there may be a high percentage of encoded 
		 * characters, so base64 may get set below...
		 */
		if(body->type == TYPETEXT)
		  eight_bit_chars++;
		else
		  break;
	    }

#if defined(DOS) || defined(OS2) /* for binary file detection */
	    if(lastchar != BREAKOUT)
	      lastchar = *p;
#endif

	    /* read another buffer in */
	    if(n == 0){
		if(bol)
		  line_so_far += p - bol;

		for (n = 0; n < RBUFSZ-1 && so_readc(&buf[n], so) != 0; n++)
		  ;

		len += n;
		p = buf;
	    }
	    else
	      p++;

	    /*
	     * If there's no beginning-of-line pointer, then we must
	     * have seen and end-of-line.  Set bol to the start of the
	     * new line...
	     */
	    if(!bol)
	      bol = p;

#if defined(DOS) || defined(OS2) /* for binary file detection */
	    /* either a lone \r or lone \n indicate binary file */
	    if(lastchar == '\r' || lastchar == BREAKOUT){
		if(lastchar == BREAKOUT || n == 0 || *p != '\n'){
		    if(body->type == TYPEOTHER){
			body->type    = TYPEAPPLICATION;
			body->subtype = cpystr("octet-stream");
		    }

		    new_encoding = ENCBINARY;
		    break;
		}
	    }
#endif
	}
    }

    if(body->encoding == ENCOTHER || body->type == TYPEOTHER){
	/*
	 * Since the type or encoding aren't set yet, fall thru a 
	 * series of tests to make sure an adequate type and 
	 * encoding are set...
	 */

	if(max_line >= 1000L){ 		/* 1000 comes from rfc821 */
	    if(body->type == TYPEOTHER){
		/*
		 * Since the types not set, then we didn't find a NULL.
		 * If there's no NULL, then this is likely text.  However,
		 * since we can't be *completely* sure, we set it to
		 * the generic type.
		 */
		body->type    = TYPEAPPLICATION;
		body->subtype = cpystr("octet-stream");
	    }

	    if(new_encoding != ENCBINARY)
	      /*
	       * As with NULL handling, if we're told it's text, 
	       * qp-encode it, else it gets base 64...
	       */
	      new_encoding = (body->type == TYPETEXT) ? ENC8BIT : ENCBINARY;
	}

	if(eight_bit_chars == 0L){
	    if(body->type == TYPEOTHER)
	      body->type = TYPETEXT;

	    if(new_encoding == ENCOTHER)
	      new_encoding = ENC7BIT;  /* short lines, no 8 bit */
	}
	else if(len <= 3000L || (eight_bit_chars * 100L)/len < 30L){
	    /*
	     * The 30% threshold is based on qp encoded readability
	     * on non-MIME UA's.
	     */
	    can_be_ascii--;
	    if(body->type == TYPEOTHER)
	      body->type = TYPETEXT;

	    if(new_encoding != ENCBINARY)
	      new_encoding = ENC8BIT;  /* short lines, < 30% 8 bit chars */
	}
	else{
	    can_be_ascii--;
	    if(body->type == TYPEOTHER){
		body->type    = TYPEAPPLICATION;
		body->subtype = cpystr("octet-stream");
	    }

	    /*
	     * Apply maximal encoding regardless of previous
	     * setting.  This segment's either not text, or is 
	     * unlikely to be readable with > 30% of the
	     * text encoded anyway, so we might as well save space...
	     */
	    new_encoding = ENCBINARY;   /*  > 30% 8 bit chars */
	}
    }

#ifdef ENCODE_FROMS
    /* If there were From_'s at the beginning of a line or standalone dots */
    if((froms || dots) && new_encoding != ENCBINARY)
      new_encoding = ENC8BIT;
#endif

    /* need to set the subtype, and possibly the charset */
    if(body->type == TYPETEXT && body->subtype == NULL){
        PARAMETER *pm;

	body->subtype = cpystr("PLAIN");

	/* need to add charset */
	if(can_be_ascii > 0 || ps_global->VAR_CHAR_SET){
	    if(body->parameter == NULL){
	        pm = body->parameter = mail_newbody_parameter();
	        pm->attribute = cpystr("charset");
	    }
	    else{
                for(pm = body->parameter;
		    strucmp(pm->attribute, "charset") && pm->next != NULL;
		    pm = pm->next)
		  ;/* find charset parameter */

	        if(strucmp(pm->attribute, "charset") != 0){ /* add one */
		    pm->next = mail_newbody_parameter();
		    pm = pm->next;
		    pm->attribute = cpystr("charset");
	        }
		else if(pm->value)
		  fs_give((void **)&pm->value);
	    }

	    pm->value = set_mime_charset(can_be_ascii > 0,
					 ps_global->VAR_CHAR_SET);
	}
    }

    /* need to set the charset */
    if(body->type == TYPEAPPLICATION
       && body->subtype
       && strucmp(body->subtype, "DIRECTORY") == 0
       && body->parameter
       && strucmp(body->parameter->attribute, "PROFILE") == 0
       && strucmp(body->parameter->value, "X-Email-Abook-Entry") == 0
       && (can_be_ascii > 0 || ps_global->VAR_CHAR_SET)){
        PARAMETER *pm;

	for(pm = body->parameter;
	    strucmp(pm->attribute, "charset") && pm->next != NULL;
	    pm = pm->next)
	  ;/* find charset parameter */

	if(strucmp(pm->attribute, "charset") != 0){  /* wasn't one, add it */
	    pm->next = mail_newbody_parameter();
	    pm = pm->next;
	    pm->attribute = cpystr("charset");
	    pm->value = set_mime_charset(can_be_ascii > 0,
					 ps_global->VAR_CHAR_SET);
	}
	/* else, leave what was there */
    }

    if(body->encoding == ENCOTHER)
      body->encoding = new_encoding;

#ifndef	DOS
    fs_give((void **)&buf);
#endif

    if(we_cancel)
      cancel_busy_alarm(-1);
}


/*
 * set_mime_charset - assign character set for MIME body parts based
 *		      on content and "downgradeability" of the provided
 *		      charset.
 */
char *
set_mime_charset(ascii_ok, cs)
    int   ascii_ok;
    char *cs;
{
    char	**excl;
    static char  *non_ascii[] = {"UNICODE-1-1-UTF-7", NULL};

    for(excl = non_ascii; cs && *excl && strucmp(*excl, cs); excl++)
      ;

    return(cpystr((!cs || (ascii_ok && !*excl)) ? "US-ASCII" : cs));
}


/*
 *
 */
void
set_body_size(b)
    BODY *b;
{
    unsigned char c;
    int we_cancel = 0;

    we_cancel = busy_alarm(1, NULL, NULL, 0);
    so_seek((STORE_S *)b->contents.binary, 0L, 0);
    b->size.bytes = 0L;
    while(so_readc(&c, (STORE_S *)b->contents.binary))
      b->size.bytes++;

    if(we_cancel)
      cancel_busy_alarm(-1);
}



/*
 * pine_header_line - simple wrapper around c-client call to contain
 *                    repeated code, and to write fcc if required.
 */
int
pine_header_line(tmp, field, header, text, f, s, writehdr, localcopy)
    char      *tmp;
    char      *field;
    METAENV   *header;
    char      *text;
    soutr_t    f;
    TCPSTREAM *s;
    int        writehdr, localcopy;
{
    char *p;

    *(p = tmp) = '\0';
    rfc822_header_line(&p, field, header ? header->env : NULL,
		       rfc1522_encode(tmp_20k_buf, (unsigned char *) text,
				      ps_global->VAR_CHAR_SET));
    return(((writehdr && f) ? (*f)(s, tmp) : 1)
         && ((localcopy && local_so
	     && !local_written) ? so_puts(local_so,tmp) : 1));
}


/*
 * pine_address_line - write a header field containing addresses,
 *                     one by one (so there's no buffer limit), and
 *                     wrapping where necessary.
 * Note: we use c-client functions to properly build the text string,
 *       but have to screw around with pointers to fool c-client functions
 *       into not blatting all the text into a single buffer.  Yeah, I know.
 */
int
pine_address_line(tmp, field, header, alist, f, s, writehdr, localcopy)
    char      *tmp;
    char      *field;
    METAENV   *header;
    ADDRESS   *alist;
    soutr_t    f;
    TCPSTREAM *s;
    int        writehdr, localcopy;
{
    char    *p, *delim, *ptmp, *pruned, ch;
    ADDRESS *atmp;
    int      i, count;
    int      in_group = 0, was_start_of_group = 0, fix_lcc = 0;
    static   char comma[]    = ", ";
#define	no_comma	(&comma[1])

    if(!alist)				/* nothing in field! */
      return(1);

    *(p = tmp)  = '\0';
    if(!alist->host && alist->mailbox){ /* c-client group convention */
        in_group = 1;
	was_start_of_group = 1;
    }

    ptmp	    = alist->personal;	/* remember personal name */
    alist->personal = rfc1522_encode(tmp_20k_buf,
				     (unsigned char *) alist->personal,
				     ps_global->VAR_CHAR_SET);
    pruned	    = prune_personal(alist, &ch);
    atmp            = alist->next;
    alist->next	    = NULL;		/* digest only first address! */
    rfc822_address_line(&p, field, header ? header->env : NULL, alist);
    alist->next	    = atmp;		/* restore pointer to next addr */
    alist->personal = ptmp;		/* in case it changed, restore name */
    if(pruned)				/* restore truncated string */
      *pruned = ch;

    if((count = strlen(tmp)) > 2){	/* back over CRLF */
	count -= 2;
	tmp[count] = '\0';
    }

    /*
     * If there is no sending_stream and we are writing the Lcc header,
     * then we are piping it to sendmail -t which expects it to be a bcc,
     * not lcc.
     *
     * When we write it to the fcc or postponed (the local_so),
     * we want it to be lcc, not bcc, so we put it back.
     */
    if(!sending_stream && writehdr && struncmp("lcc:", tmp, 4) == 0)
      fix_lcc = 1;

    if(writehdr && f){
	int failed;

	if(fix_lcc)
	  tmp[0] = 'b';
	
	failed = !(*f)(s, tmp);
	if(fix_lcc)
	  tmp[0] = 'L';

	if(failed)
	  return(0);
    }

    if(localcopy && local_so && !local_written && !so_puts(local_so, tmp))
      return(0);

    for(alist = atmp; alist; alist = alist->next){
	delim = comma;
	/* account for c-client's representation of group names */
	if(in_group){
	    if(!alist->host){  /* end of group */
		in_group = 0;
		was_start_of_group = 0;
		delim = no_comma;
	    }
	    else if(!localcopy || !local_so || local_written)
	      continue;
	}
	/* start of new group, print phrase below */
        else if(!alist->host && alist->mailbox){
	    in_group = 1;
	    was_start_of_group = 1;
	}

	/* no comma before first address in group syntax */
	if(was_start_of_group && alist->host){
	    delim = no_comma;
	    was_start_of_group = 0;
	}

	/* first, write delimiter */
	if(((!in_group||was_start_of_group) && writehdr && f && !(*f)(s,delim))
	   || (localcopy && local_so && !local_written
	       && !so_puts(local_so, delim)))
	  return(0);

	tmp[0]		= '\0';
	ptmp		= alist->personal; /* remember personal name */
	alist->personal = rfc1522_encode(tmp_20k_buf,
					 (unsigned char *) alist->personal,
					 ps_global->VAR_CHAR_SET);
	pruned		= prune_personal(alist, &ch);
	atmp		= alist->next;
	alist->next	= NULL;		/* tie off linked list */
	rfc822_write_address(tmp, alist);
	alist->next	= atmp;		/* restore next pointer */
	alist->personal = ptmp;		/* in case it changed, restore name */

	/*
	 * BUG
	 * With group syntax addresses we no longer have two identical
	 * streams of output.  Instead, for the fcc/postpone copy we include
	 * all of the addresses inside the :; of the group, and for the
	 * mail we're sending we don't include them.  That means we aren't
	 * correctly keeping track of the column to wrap in, below.  That is,
	 * we are keeping track of the fcc copy but we aren't keeping track
	 * of the regular copy.  It could result in too long or too short
	 * lines.  Should almost never come up since group addresses are almost
	 * never followed by other addresses in the same header, and even
	 * when they are, you have to go out of your way to get the headers
	 * messed up.
	 */
	if(count + 2 + (i = strlen(tmp)) > 78){ /* wrap long lines... */
	    count = i + 4;
	    if((!in_group && writehdr && f && !(*f)(s, "\015\012    "))
	       || (localcopy && local_so && !local_written &&
				       !so_puts(local_so, "\015\012    ")))
	      return(0);
	}
	else
	  count += i + 2;

	if(((!in_group || was_start_of_group) && writehdr && f && !(*f)(s, tmp))
	   || (localcopy && local_so && !local_written
	       && !so_puts(local_so, tmp)))
	  return(0);
    }

    return((writehdr && f ? (*f)(s, "\015\012") : 1)
	   && ((localcopy && local_so
	       && !local_written) ? so_puts(local_so, "\015\012") : 1));
}


/*
 * prune_personal - function to help make sure we don't pop any c-client
 *		    fixed length buffers.  Yeah, I know.
 *
 * Returns: NULL if things look ok, else pointer into some ADDRESS
 *	    structure string that got truncated, and the character the
 *	    new terminating NULL replaced so the string can be restored.
 */
char *
prune_personal(addr, c)
    ADDRESS *addr;
    char    *c;
{
    char *p = NULL;

    if(addr && addr->personal && strlen(addr->personal) > MAX_SINGLE_ADDR/2){
	p = &addr->personal[MAX_SINGLE_ADDR/2];
	*c = *p;
	*p = '\0';
    }

    return(p);
}


/*
 * mutated pine version of c-client's rfc822_header() function. 
 * changed to call pine-wrapped header and address functions
 * so we don't have to limit the header size to a fixed buffer.
 * This function also calls pine's body_header write function
 * because encoding is delayed until output_body() is called.
 */
int
pine_rfc822_header(header, body, f, s)
    METAENV   *header;
    BODY      *body;
    soutr_t    f;
    TCPSTREAM *s;
{
    char        tmp[MAX_SINGLE_ADDR], *p;
    PINEFIELD  *pf;
    int         j, private_header;

    if(header->env->remail){			/* if remailing */
	long i = strlen (header->env->remail);
	if(i > 4 && header->env->remail[i-4] == '\015')
	  header->env->remail[i-2] = '\0'; /* flush extra blank line */

	if((f && !(*f)(s, header->env->remail)) 
	  || (local_so && !local_written
	      && !so_puts(local_so, header->env->remail)))
	  return(0);				/* start with remail header */
    }

    j = 0;
    for(pf = header->sending_order[j]; pf; pf = header->sending_order[++j]){
	switch(pf->type){
	  /*
	   * Warning:  This is confusing.  The 2nd to last argument used to
	   * be just pf->writehdr.  We want Bcc lines to be written out
	   * if we are handing off to a sendmail temp file but not if we
	   * are talking smtp, so bcc's writehdr is set to 0 and
	   * pine_address_line was sending if writehdr OR !sending_stream.
	   * That works as long as we want to write everything when
	   * !sending_stream (an mta handoff to sendmail).  But then we
	   * added the undisclosed recipients line which should only get
	   * written if writehdr is set, and not when we pass to a
	   * sendmail temp file.  So pine_address_line has been changed
	   * so it bases its decision solely on the writehdr passed to it,
	   * and the logic that worries about Bcc and sending_stream
	   * was moved up to the caller (here) to decide when to set it.
	   *
	   * So we have:
	   *   undisclosed recipients:;  This will just be written
	   *                             if writehdr was set and not
	   *                             otherwise, nothing magical.
	   *** We may want to change this, because sendmail -t doesn't handle
	   *** the empty group syntax well unless it has been configured to
	   *** do so.  It isn't configured by default, or in any of the
	   *** sendmail v8 configs.  So we may want to not write this line
	   *** if we're doing an mta_handoff (!sending_stream).
	   *
	   *   !sending_stream (which means a handoff to a sendmail -t)
	   *           bcc or lcc both set the arg so they'll get written
	   *             (There is also Lcc hocus pocus in pine_address_line
	   *              which converts the Lcc: to Bcc: for sendmail
	   *              processing.)
	   *   sending_stream (which means an smtp handoff)
	   *           bcc and lcc will never have writehdr set, so
	   *             will never be written (They both do have rcptto set,
	   *             so they both do cause RCPT TO commands.)
	   *
	   *   The localcopy is independent of sending_stream and is just
	   *   written if it is set for all of these.
	   */
	  case Address:
	    if(!pine_address_line(tmp,
				  pf->name,
				  header,
				  pf->addr ? *pf->addr : NULL,
				  f,
				  s,
				  (!strucmp("bcc",pf->name ? pf->name : "")
				    || !strucmp("Lcc",pf->name ? pf->name : ""))
					   ? !sending_stream
					   : pf->writehdr,
				  pf->localcopy))
	      return(0);

	    break;

	  case Fcc:
	  case FreeText:
	  case Subject:
	    if(!pine_header_line(tmp, pf->name, header,
		pf->text ? *pf->text : NULL,
		f, s, pf->writehdr, pf->localcopy))
	      return(0);

	    break;

	  default:
	    q_status_message1(SM_ORDER,3,7,"Unknown header type: %s",pf->name);
	    break;
	}
    }


#if	(defined(DOS) || defined(OS2)) && !defined(NOAUTH)
    /*
     * Add comforting "X-" header line indicating what sort of 
     * authenticity the receiver can expect...
     */
    {
	 NETMBX	     netmbox;
	 char	     sstring[MAILTMPLEN], *label;	/* place to write  */
	 MAILSTREAM *stream;

	 if(((stream = ps_global->inbox_stream)
	     && mail_valid_net_parse(stream->mailbox, &netmbox)
	     && !netmbox.anoflag)
	    || ((stream = ps_global->mail_stream) != ps_global->inbox_stream
		&& mail_valid_net_parse(stream->mailbox, &netmbox)
		&& !netmbox.anoflag)){
	     char  last_char = netmbox.host[strlen(netmbox.host) - 1],
		  *user = !strncmp(stream->dtb->name, "imap", 4)
			    ? imap_user(stream)
			    : cached_user_name(netmbox.mailbox);
	     sprintf(sstring, "%s@%s%s%s", user ? user : "NULL", 
		     isdigit((unsigned char)last_char) ? "[" : "",
		     netmbox.host,
		     isdigit((unsigned char) last_char) ? "]" : "");
	     label = "X-X-Sender";		/* Jeez. */
	     if(F_ON(F_USE_SENDER_NOT_X,ps_global))
	       label += 4;
	 }
	 else{
	     strcpy(sstring,"UNAuthenticated Sender");
	     label = "X-Warning";
	 }

	 if(!pine_header_line(tmp, label, header, sstring, f, s, 1, 1))
	   return(0);
     }
#endif

    if(body && !header->env->remail){	/* not if remail or no body */
	if((f && !(*f)(s, MIME_VER))
	   || (local_so && !local_written && !so_puts(local_so, MIME_VER)))
	  return(0);

	*(p = tmp) = '\0';
	pine_write_body_header(&p, body);

	if((f && !(*f)(s, tmp)) 
	   || (local_so && !local_written && !so_puts(local_so, tmp)))
	  return(0);
    }
    else{					/* write terminating newline */
	if((f && !(*f)(s, "\015\012"))
	   || (local_so && !local_written && !so_puts(local_so, "\015\012")))
	  return(0);
    }

    return(1);
}


/*
 * since encoding happens on the way out the door, this is basically
 * just needed to handle TYPEMULTIPART
 */
void
pine_encode_body (body)
    BODY *body;
{
  void *f;
  PART *part;
  dprint(4, (debugfile, "-- pine_encode_body: %d\n", body ? body->type : 0));
  if (body) switch (body->type) {
  case TYPEMULTIPART:		/* multi-part */
    if (!body->parameter) {	/* cookie not set up yet? */
      char tmp[MAILTMPLEN];	/* make cookie not in BASE64 or QUOTEPRINT*/
      sprintf (tmp,"%ld-%ld-%ld=:%ld",gethostid (),random (),time (0),
	       getpid ());
      body->parameter = mail_newbody_parameter ();
      body->parameter->attribute = cpystr ("BOUNDARY");
      body->parameter->value = cpystr (tmp);
    }
    part = body->contents.part;	/* encode body parts */
    do pine_encode_body (&part->body);
    while (part = part->next);	/* until done */
    break;
/* case MESSAGE:	*/	/* here for documentation */
    /* Encapsulated messages are always treated as text objects at this point.
       This means that you must replace body->contents.msg with
       body->contents.text, which probably involves copying
       body->contents.msg.text to body->contents.text */
  default:			/* all else has some encoding */
    /*
     * but we'll delay encoding it until the message is on the way
     * into the mail slot...
     */
    break;
  }
}


/*
 * post_rfc822_output - cloak for pine's 822 output routine.  Since
 *			we can't pass opaque envelope thru c-client posting
 *			logic, we need to wrap the real output inside
 *			something that c-client knows how to call.
 */
int
post_rfc822_output (tmp, env, body, f, s)
    char      *tmp;
    ENVELOPE  *env;
    BODY      *body;
    soutr_t    f;
    TCPSTREAM *s;
{
    return(pine_rfc822_output(send_header, body, f, s));
}


/*
 * pine_rfc822_output - pine's version of c-client call.  Necessary here
 *			since we're not using its structures as intended!
 */
int
pine_rfc822_output(header, body, f, s)
    METAENV   *header;
    BODY      *body;
    soutr_t    f;
    TCPSTREAM *s;
{
    int we_cancel = 0;
    int retval;

    dprint(4, (debugfile, "-- pine_rfc822_output\n"));

    we_cancel = busy_alarm(1, NULL, NULL, 0);
    pine_encode_body(body);		/* encode body as necessary */
    /* build and output RFC822 header, output body */
    retval = pine_rfc822_header(header, body, f, s)
	   && (body ? pine_rfc822_output_body(body, f, s) : 1);

    if(we_cancel)
      cancel_busy_alarm(-1);

    return(retval);
}


/*
 * Local globals pine's body output routine needs
 */
static soutr_t    l_f;
static TCPSTREAM *l_stream;
static unsigned   c_in_buf = 0;

/*
 * def to make our pipe write's more friendly
 */
#ifdef	PIPE_MAX
#if	PIPE_MAX > 20000
#undef	PIPE_MAX
#endif
#endif

#ifndef	PIPE_MAX
#define	PIPE_MAX	1024
#endif


/*
 * l_flust_net - empties gf_io terminal function's buffer
 */
int
l_flush_net(force)
    int force;
{
    if(c_in_buf){
	char *p = &tmp_20k_buf[0], *lp = NULL, c = '\0';

	tmp_20k_buf[c_in_buf] = '\0';
	if(!force){
	    /*
	     * The start of each write is expected to be the start of a
	     * "record" (i.e., a CRLF terminated line).  Make sure that is true
	     * else we might screw up SMTP dot quoting...
	     */
	    for(p = tmp_20k_buf, lp = NULL;
		p = strstr(p, "\015\012");
		lp = (p += 2))
	      ;


	    if(!lp && c_in_buf > 2)			/* no CRLF! */
	      for(p = &tmp_20k_buf[c_in_buf] - 2;
		  p > &tmp_20k_buf[0] && *p == '.';
		  p--)					/* find last non-dot */
		;

	    if(lp && *lp){				/* snippet remains */
		c = *lp;
		*lp = '\0';
	    }
	}

	if((l_f && !(*l_f)(l_stream, tmp_20k_buf))
	   || (local_so && !local_written && !so_puts(local_so, tmp_20k_buf)))
	  return(0);

	c_in_buf = 0;
	if(lp && (*lp = c))				/* Shift text left? */
	  while(tmp_20k_buf[c_in_buf] = *lp)
	    c_in_buf++, lp++;
    }

    return(1);
}


/*
 * l_putc - gf_io terminal function that calls smtp's soutr_t function.
 *
 */
int
l_putc(c)
    int c;
{
    tmp_20k_buf[c_in_buf++] = (char) c;
    return((c_in_buf >= PIPE_MAX) ? l_flush_net(FALSE) : TRUE);
}



/*
 * pine_rfc822_output_body - pine's version of c-client call.  Again, 
 *                necessary since c-client doesn't know about how
 *                we're treating attachments
 */
long
pine_rfc822_output_body(body, f, s)
    BODY *body;
    soutr_t f;
    TCPSTREAM *s;
{
    PART *part;
    PARAMETER *param;
    char *cookie = NIL, *t, *encode_error;
    char tmp[MAILTMPLEN];
    gf_io_t            gc;
#if defined(DOS) || defined(OS2)
    extern unsigned char  *xlate_from_codepage;
#endif

    dprint(4, (debugfile, "-- pine_rfc822_output_body: %d\n",
	       body ? body->type : 0));
    if(body->type == TYPEMULTIPART) {   /* multipart gets special handling */
	part = body->contents.part;	/* first body part */
					/* find cookie */
	for (param = body->parameter; param && !cookie; param = param->next)
	  if (!strcmp (param->attribute,"BOUNDARY")) cookie = param->value;
	if (!cookie) cookie = "-";	/* yucky default */

	/*
	 * Output a bit of text before the first multipart delimiter
	 * to warn unsuspecting users of non-mime-aware ua's that
	 * they should expect weirdness...
	 */
	if(f && !(*f)(s, "  This message is in MIME format.  The first part should be readable text,\015\012  while the remaining parts are likely unreadable without MIME-aware tools.\015\012  Send mail to mime@docserver.cac.washington.edu for more info.\015\012\015\012"))
	  return(0);

	do {				/* for each part */
					/* build cookie */
	    sprintf (t = tmp,"--%s\015\012",cookie);
					/* append mini-header */
	    pine_write_body_header (&t,&part->body);
				/* output cookie, mini-header, and contents */
	    if(local_so && !local_written)
	      so_puts(local_so, tmp);

	    if (!((f ? (*f) (s,tmp) : 1)
		  && pine_rfc822_output_body (&part->body,f,s)))
	      return(0);
	} while (part = part->next);	/* until done */
					/* output trailing cookie */
	sprintf (t = tmp,"--%s--",cookie);
	if(local_so && !local_written){
	    so_puts(local_so, t);
	    so_puts(local_so, "\015\012");
	}

	return(f ? ((*f) (s,t) && (*f) (s,"\015\012")) : 1);
    }

    l_f      = f;			/* set up for writing chars...  */
    l_stream = s;			/* out other end of pipe...     */
    gf_filter_init();
    dprint(4, (debugfile, "-- pine_rfc822_output_body: segment %ld bytes\n",
	       body->size.bytes));

    if(body->contents.binary)
      gf_set_so_readc(&gc, (STORE_S *)body->contents.binary);
    else
      return(1);

    so_seek((STORE_S *)body->contents.binary, 0L, 0);

    if(body->type != TYPEMESSAGE){ 	/* NOT encapsulated message */
	/*
	 * Convert text pieces to canonical form
	 * BEFORE applying any encoding (rfc1341: appendix G)...
	 */
	if(body->type == TYPETEXT){
	    gf_link_filter(gf_local_nvtnl);

#if defined(DOS) || defined(OS2)
	    if(xlate_from_codepage){
		gf_translate_opt(xlate_from_codepage, 256);
		gf_link_filter(gf_translate);
	    }
#endif
	}

	switch (body->encoding) {	/* all else needs filtering */
	  case ENC8BIT:			/* encode 8BIT into QUOTED-PRINTABLE */
	    gf_link_filter(gf_8bit_qp);
	    break;

	  case ENCBINARY:		/* encode binary into BASE64 */
	    gf_link_filter(gf_binary_b64);
	    break;

	  default:			/* otherwise text */
	    break;
	}
    }

    if(encode_error = gf_pipe(gc, l_putc)){ /* shove body part down pipe */
	q_status_message1(SM_ORDER | SM_DING, 3, 4,
			  "Encoding Error \"%s\"", encode_error);
	display_message('x');
	return(0);
    }
    else if(!l_flush_net(TRUE)){
	return(0);
    }

    send_bytes_sent += gf_bytes_piped();
    so_release((STORE_S *)body->contents.binary);

    if(local_so && !local_written)
      so_puts(local_so, "\015\012");

    return(f ? (*f)(s, "\015\012") : 1);	/* output final stuff */
}


/*
 * pine_write_body_header - another c-client clone.  This time only
 *                          so the final encoding labels get set 
 *                          correctly since it hasn't happened yet.
 */
void
pine_write_body_header(dst, body)
    char **dst;
    BODY  *body;
{
    char *s;
    PARAMETER *param = body->parameter;
    extern const char *tspecials;

    sprintf (*dst += strlen (*dst),"Content-Type: %s",body_types[body->type]);
    s = body->subtype ? body->subtype : rfc822_default_subtype (body->type);
    sprintf (*dst += strlen (*dst),"/%s",s);
    if (param) do {
	sprintf (*dst += strlen (*dst),"; %s=",param->attribute);
	rfc822_cat (*dst,param->value,tspecials);
    } while (param = param->next);
    else if (body->type == TYPETEXT) strcat (*dst,"; charset=US-ASCII");
    strcpy (*dst += strlen (*dst),"\015\012");
    if (body->encoding)		/* note: encoding 7BIT never output! */
      sprintf (*dst += strlen (*dst), "Content-Transfer-Encoding: %s\015\012",
	       body_encodings[(body->encoding == ENCBINARY)
				? ENCBASE64
				: (body->encoding == ENC8BIT)
				    ? ENCQUOTEDPRINTABLE
				    : (body->encoding <= ENCMAX)
					? body->encoding : ENCOTHER]);
    if (body->id) sprintf (*dst += strlen (*dst),"Content-ID: %s\015\012",
			   body->id);
    if (body->description)
      sprintf (*dst += strlen (*dst),"Content-Description: %s\015\012",
	       body->description);
    strcat (*dst,"\015\012");	/* write terminating blank line */
}


/*
 * pine_free_body - yet another c-client clone just so the body gets
 *                  free'd appropriately
 */
void
pine_free_body(body)
    BODY **body;
{
    if (*body) {			/* only free if exists */
	pine_free_body_data (*body);	/* free its data */
	fs_give ((void **) body);	/* return body to free storage */
    }
}


/* 
 * pine_free_body_data - not just releasing strings anymore!
 */
void
pine_free_body_data(body)
    BODY *body;
{
    if (body->subtype) fs_give ((void **) &body->subtype);
    mail_free_body_parameter (&body->parameter);
    if (body->id) fs_give ((void **) &body->id);
    if (body->description) fs_give ((void **) &body->description);
    if(body->type == TYPEMULTIPART){
	pine_free_body_part (&body->contents.part);
    }
    else if(body->contents.binary){
	so_give((STORE_S **)&body->contents.binary);
	body->contents.binary = NULL;
    }
}


/*
 * pine_free_body_part - c-client clone to call the right routines
 *             for cleaning up.
 */
void
pine_free_body_part(part)
    PART **part;
{
    if (*part) {		/* only free if exists */
	pine_free_body_data (&(*part)->body);
				/* run down the list as necessary */
	pine_free_body_part (&(*part)->next);
	fs_give ((void **) part); /* return body part to free storage */
    }
}


/*
 *
 */
int
sent_percent()
{
    int i = (int) (((send_bytes_sent + gf_bytes_piped()) * 100)
							/ send_bytes_to_send);
    return(min(i, 100));
}



/*
 *
 */
long
send_body_size(body)
    BODY *body;
{
    long  l = 0L;
    PART *part;

    if(body->type == TYPEMULTIPART) {   /* multipart gets special handling */
	part = body->contents.part;	/* first body part */
	do				/* for each part */
	  l += send_body_size(&part->body);
	while (part = part->next);	/* until done */
	return(l);
    }

    return(l + body->size.bytes);
}



/*
 * pine_smtp_verbose - pine's contribution to the smtp stream.  Return
 *		       TRUE for any positive reply code to our "VERBose"
 *		       request.
 *
 *	NOTE: at worst, this command may cause the SMTP connection to get
 *	      nuked.  Modern sendmail's recognize it, and possibly other
 *	      SMTP implementations (the "ON" arg is for PMDF).  What's
 *	      more, if it works, what's returned (sendmail uses reply code
 *	      050, but we'll accept anything less than 100) may not get 
 *	      recognized, and if it does the accompanying text will likely
 *	      vary from server to server.
 */
long
pine_smtp_verbose(stream)
    SMTPSTREAM *stream;
{
    /* any 2xx reply to this is acceptable */
    return(smtp_send(stream,"VERB","ON")/100L == 2L);
}



/*
 * pine_smtp_verbose_out - write 
 */
void
pine_smtp_verbose_out(s)
    char *s;
{
    if(verbose_send_output && s){
	char *p, last = '\0';

	for(p = s; *p; p++)
	  if(*p == '\015')
	    *p = ' ';
	  else
	    last = *p;

	fputs(s, verbose_send_output);
	if(last != '\012')
	  fputc('\n', verbose_send_output);
    }

}



/*----------------------------------------------------------------------
      Post news via NNTP or inews

Args: env -- envelope of message to post
       body -- body of message to post

Returns: -1 if failed or cancelled, 1 if succeeded

WARNING: This call function has the side effect of writing the message
    to the local_so object.   
  ----*/
news_poster(header, body)
     METAENV *header;
     BODY    *body;
{
    char *error_mess, error_buf[100];
    int	  rv, we_cancel = 0;
    void *orig_822_output;
    BODY *bp = NULL;

    we_cancel = busy_alarm(1, "Posting news", NULL, 1);

    dprint(4, (debugfile, "Posting: [%s]\n", header->env->newsgroups));

    if(ps_global->VAR_NNTP_SERVER && ps_global->VAR_NNTP_SERVER[0]
       && ps_global->VAR_NNTP_SERVER[0][0]){
       /*---------- NNTP server defined ----------*/
        error_mess              = NULL;

	/*
	 * Install our rfc822 output routine 
	 */
	orig_822_output = mail_parameters(NULL, GET_RFC822OUTPUT, NULL);
	(void)mail_parameters(NULL,SET_RFC822OUTPUT,(void *)post_rfc822_output);

        ps_global->noshow_error = 1;
#ifdef DEBUG
        sending_stream = nntp_open(ps_global->VAR_NNTP_SERVER, debug);
#else
        sending_stream = nntp_open(ps_global->VAR_NNTP_SERVER, 0L);
#endif
        ps_global->noshow_error = 0;

        if(sending_stream != NULL) {
	    /*
	     * Fake that we've got clearance from the transport agent
	     * for 8bit transport for the benefit of our output routines...
	     */
	    if(F_ON(F_ENABLE_8BIT_NNTP, ps_global)
	       && (bp = first_text_8bit(body))){
		int i;

		for(i = 0; (i <= ENCMAX) && body_encodings[i]; i++)
		  ;

		if(i > ENCMAX){		/* no empty encoding slots! */
		    bp = NULL;
		}
		else {
		    body_encodings[i] = body_encodings[ENC8BIT];
		    bp->encoding = (unsigned short) i;
		}
	    }

	    /*
	     * Set global header pointer so we can get at it later...
	     */
	    send_header = header;

            if(nntp_mail(sending_stream, header->env, body) == 0)
	      sprintf(error_mess = error_buf, "Error posting message: %.60s",
		      sending_stream->reply);

            smtp_close(sending_stream);
	    sending_stream = NULL;
	    if(F_ON(F_ENABLE_8BIT_NNTP, ps_global) && bp)
	      body_encodings[bp->encoding] = NULL;

        } else {
            /*---- Open of NNTP connection failed ------ */
            sprintf(error_mess = error_buf,
		    "Error connecting to news server: %.60s",
                    ps_global->c_client_error);
            dprint(1, (debugfile, error_buf));
        }

	(void) mail_parameters (NULL, SET_RFC822OUTPUT, orig_822_output);
    } else {
        /*----- Post via local mechanism -------*/
        error_mess = post_handoff(header, body, error_buf);
    }

    if(we_cancel)
      cancel_busy_alarm(0);

    if(error_mess){
	if(local_so && !local_written)
	  so_give(&local_so);	/* clean up any fcc data */

        q_status_message(SM_ORDER | SM_DING, 4, 5, error_mess);
        return(-1);
    }

    local_written = 1;
    return(1);
}



/*
 * view_as_rich - set the rich_header flag
 *
 *         name  - name of the header field
 *         deflt - default value to return if user didn't set it
 *
 *         Note: if the user tries to turn them all off with "", then
 *		 we take that to mean default, since otherwise there is no
 *		 way to get to the headers.
 */
int
view_as_rich(name, deflt)
char *name;
int deflt;
{
    char **p;
    char *q;

    p = ps_global->VAR_COMP_HDRS;

    if(p && *p && **p){
        for(; (q = *p) != NULL; p++){
	    if(!struncmp(q, name, strlen(name)))
	      return 0; /* 0 means we *do* view it by default */
	}

        return 1; /* 1 means it starts out hidden */
    }
    return(deflt);
}


/*
 * is_a_forbidden_hdr - is this name a "forbidden" header?
 *
 *          name - the header name to check
 *  We don't allow user to change these.
 */
int
is_a_forbidden_hdr(name)
char *name;
{
    char **p;
    static char *forbidden_headers[] = {
	"sender",
	"x-sender",
	"date",
	"received",
	"message-id",
	"in-reply-to",
	"path",
	"resent-message-id",
	"resent-message-date",
	"resent-message-from",
	"resent-message-sender",
	"resent-message-to",
	"resent-message-cc",
	"resent-message-reply-to",
	NULL
    };

    for(p = forbidden_headers; *p; p++)
      if(!strucmp(name, *p))
	break;

    return((*p) ? 1 : 0);
}


/*
 * hdr_has_custom_value - is there a custom value for this header?
 *
 *          hdr - the header name to check
 *  Returns 1 if there is a custom value, 0 otherwise.
 */
int
hdr_has_custom_value(hdr)
char *hdr;
{
    char **p;
    char  *q, *t, *name;
    char   save;

    if(ps_global->VAR_CUSTOM_HDRS){
        for(p = ps_global->VAR_CUSTOM_HDRS; (q = *p) != NULL; p++){

	    if(q[0]){

		/* remove leading whitespace */
	        for(name = q; *name && isspace((unsigned char)*name); name++)
	    	  ;/* do nothing */
		
		if(!*name)
		  continue;

		/* look for colon or space or end */
	        for(t = name;
		    *t && !isspace((unsigned char)*t) && *t != ':'; t++)
	    	  ;/* do nothing */

		save = *t;
		*t = '\0';

		if(strucmp(name, hdr) == 0){
		    *t = save;
		    return 1;
		}

		*t = save;
	    }
	}
    }

    return 0;
}
/*
 * count_custom_hdrs - returns number of custom headers defined by user
 */
int
count_custom_hdrs()
{
    char **p;
    char  *q     = NULL;
    char  *name;
    char  *t;
    char   save;
    int    ret   = 0;

    if(ps_global->VAR_CUSTOM_HDRS){
        for(p = ps_global->VAR_CUSTOM_HDRS; (q = *p) != NULL; p++){
	    if(q[0]){
		/* remove leading whitespace */
	        for(name = q; *name && isspace((unsigned char)*name); name++)
	    	  ;/* do nothing */
		
		/* look for colon or space or end */
	        for(t = name;
		    *t && !isspace((unsigned char)*t) && *t != ':'; t++)
	    	  ;/* do nothing */

		save = *t;
		*t = '\0';
		if(!is_a_std_hdr(name) && !is_a_forbidden_hdr(name))
		  ret++;

		*t = save;
	    }
	}
    }

    return(ret);
}


/*
 * set_default_hdrval - put the user's default value for this header
 *                      into pf->textbuf.
 */
void
set_default_hdrval(pf)
PINEFIELD *pf;
{
    char       **p = 0;
    char        *q = 0;
    char        *t;
    char        *name;
    char        *value;
    char         save;

    if(!pf || !pf->name){
	q_status_message(SM_ORDER,3,7,"Internal error setting default header");
	return;
    }

    pf->textbuf = NULL;

    if(ps_global->VAR_CUSTOM_HDRS){
        for(p = ps_global->VAR_CUSTOM_HDRS; (q = *p) != NULL; p++){

	    if(q[0]){

		/* remove leading whitespace */
	        for(name = q; *name && isspace((unsigned char)*name); name++)
	    	  ;/* do nothing */
		
		if(!*name)
		  continue;

		/* look for colon or space or end */
	        for(t = name;
		    *t && !isspace((unsigned char)*t) && *t != ':'; t++)
	    	  ;/* do nothing */

		save = *t;
		*t = '\0';

		if(strucmp(name, pf->name) != 0){
		    *t = save;
		    continue;
		}

		/* turn on editing */
		if(strucmp(name, "From") == 0
		  || strucmp(name, "Reply-To") == 0)
		  pf->canedit = 1;

		*t = save;

		/* remove space between name and colon */
	        for(value = t;
		    *value && isspace((unsigned char)*value); value++)
	    	  ;/* do nothing */

		if(*value && *value == ':'){
		    /* remove leading whitespace from default value */
		    for(++value;
			*value && isspace((unsigned char)*value); value++)
		      ;/* do nothing */

		    pf->textbuf = cpystr(value);
		}

		break;
	    }
	}
    }

    if(!pf->textbuf)
      pf->textbuf = cpystr("");
}


/*
 * is_a_std_hdr - is this name a "standard" header?
 *
 *          name - the header name to check
 */
int
is_a_std_hdr(name)
char *name;
{
    int    i;
    /* how many standard headers are there? */
    int fixed_cnt = (sizeof(pf_template)/sizeof(pf_template[0])) - 1;

    /* check to see if this is a standard header */
    for(i = 0; i < fixed_cnt; i++)
      if(!strucmp(name, pf_template[i].name))
	break;

    return((i < fixed_cnt) ? 1 : 0);
}


/*
 * customized_hdr_setup - setup the PINEFIELDS for all the customized headers
 *                    Allocates space for each name and addr ptr.
 *                    Allocates space for default in textbuf, even if empty.
 *
 *              head - the first PINEFIELD to fill in
 */
void
customized_hdr_setup(head)
PINEFIELD *head;
{
    char **p, *q, *t, *name, *value, save;
    int forbid;
    PINEFIELD *pf;

    pf = head;

    if(ps_global->VAR_CUSTOM_HDRS){
        for(p = ps_global->VAR_CUSTOM_HDRS; (q = *p) != NULL; p++){

	    if(q[0]){

		/* remove leading whitespace */
	        for(name = q; *name && isspace((unsigned char)*name); name++)
	    	  ;/* do nothing */
		
		if(!*name)
		  continue;

		/* look for colon or space or end */
	        for(t = name;
		    *t && !isspace((unsigned char)*t) && *t != ':'; t++)
	    	  ;/* do nothing */

		save = *t;
		*t = '\0';

		/*
		 * The same pinerc variable is used to customize standard
		 * headers, so skip them here.  Also, don't allow
		 * any of the forbidden headers.
		 */
		forbid = 0;
		if(is_a_std_hdr(name) || (forbid=is_a_forbidden_hdr(name))){
		    if(forbid)
		      q_status_message1(SM_ORDER, 3, 3,
			    "Not allowed to change header \"%s\"", name);

		    *t = save;
		    continue;
		}

		pf->name = cpystr(name);
		pf->type = FreeText;
		pf->next = pf+1;

#ifdef OLDWAY
		/*
		 * Some mailers apparently break if we change
		 * user@domain into Fred <user@domain> for return-receipt-to,
		 * so we'll just call this a FreeText field, too.
		 */
		/*
		 * For now, all custom headers are FreeText except for
		 * this one that we happen to know about.  We might
		 * have to add some syntax to the config option so that
		 * people can tell us their custom header takes addresses.
		 */
		if(!strucmp(pf->name, "Return-Receipt-to")){
		    pf->type = Address;
 		    pf->addr = (ADDRESS **)fs_get(sizeof(ADDRESS *));
		    *pf->addr = (ADDRESS *)NULL;
		}
#endif /* OLDWAY */

		*t = save;

		/* remove space between name and colon */
	        for(value = t;
		    *value && isspace((unsigned char)*value); value++)
	    	  ;/* do nothing */

		/* give them an alloc'd default, even if empty */
		if(!*value || (*value && *value != ':'))
		  pf->textbuf = cpystr("");
		else{
		    /* remove leading whitespace from default value */
		    for(++value;
			*value && isspace((unsigned char)*value); value++)
		      ;/* do nothing */

		    pf->textbuf = cpystr(value);
		}

		pf++;
	    }
	}
    }

    /* fix last next pointer */
    if(pf != head)
      (pf-1)->next = NULL;
}


/*
 * get_dflt_custom_hdrs - allocate PINEFIELDS for custom headers
 *                        fill in the defaults
 */
PINEFIELD *
get_dflt_custom_hdrs()
{
    PINEFIELD          *pfields, *pf;
    int			i;

    /* add one for possible use by fcc */
    i       = (count_custom_hdrs() + 2) * sizeof(PINEFIELD);
    pfields = (PINEFIELD *)fs_get((size_t) i);
    memset(pfields, 0, (size_t) i);

    /* set up the custom header pfields */
    customized_hdr_setup(pfields);

    return(pfields);
}


/*
 * free_customs - free misc. resources associated with custom header fields
 *
 *           pf - pointer to first custom field
 */
void
free_customs(head)
PINEFIELD *head;
{
    PINEFIELD *pf;

    for(pf = head; pf && pf->name; pf = pf->next){

	fs_give((void **)&pf->name);

	/* only true for FreeText */
	if(pf->textbuf)
	  fs_give((void **)&pf->textbuf);

	/* only true for Address */
	if(pf->addr){
	    if(*pf->addr)
	      mail_free_address(pf->addr);

	    fs_give((void **)&pf->addr);
	}

	if(pf->he && pf->he->prompt)
	  fs_give((void **)&pf->he->prompt);
    }

    fs_give((void **)&head);
}



/**************** "PIPE" READING POSTING I/O ROUTINES ****************/


/*
 * helpful def's
 */
#define	S(X)		((PIPE_S *)(X))
#define	GETBUFLEN	(4 * MAILTMPLEN)


/* 
 * pine_pipe_soutr - Replacement for tcp_soutr that writes one of our
 *		     pipes rather than a tcp stream
 */
long
pine_pipe_soutr (stream,s)
     void *stream;
     char *s;
{
    int i, o;

    if(S(stream)->out.d < 0)
      return(0L);

    if(i = strlen(s)){
	while((o = write(S(stream)->out.d, s, i)) != i)
	  if(o < 0){
	      if(errno != EINTR){
		  pine_pipe_abort(stream);
		  return(0L);
	      }
	  }
	  else{
	      s += o;			/* try again, fix up counts */
	      i -= o;
	  }
    }

    return(1L);
}


/* 
 * pine_pipe_getline - Replacement for tcp_getline that reads one
 *		       of our pipes rather than a tcp pipe
 *
 *                     C-client expects that the \r\n will be stripped off.
 */
char *
pine_pipe_getline (stream)
    void *stream;
{
    static int   cnt;
    static char *ptr;
    int		 n, m;
    char	*ret, *s, *sp, c = '\0', d;

    if(S(stream)->in.d < 0)
      return(NULL);

    if(!S(stream)->tmp){		/* initialize! */
	/* alloc space to collect input (freed in close_system_pipe) */
	S(stream)->tmp = (char *) fs_get(GETBUFLEN);
	memset(S(stream)->tmp, 0, GETBUFLEN);
	cnt = -1;
    }

    while(cnt < 0){
	while((cnt = read(S(stream)->in.d, S(stream)->tmp, GETBUFLEN)) < 0)
	  if(errno != EINTR){
	      pine_pipe_abort(stream);
	      return(NULL);
	  }

	ptr = S(stream)->tmp;
    }

    s = ptr;
    n = 0;
    while(cnt--){
	d = *ptr++;
	if((c == '\015') && (d == '\012')){
	    ret = (char *)fs_get (n--);
	    memcpy(ret, s, n);
	    ret[n] = '\0';
	    return(ret);
	}

	n++;
	c = d;
    }
					/* copy partial string from buffer */
    memcpy((ret = sp = (char *) fs_get (n)), s, n);
					/* get more data */
    while(cnt < 0){
	while((cnt = read(S(stream)->in.d, S(stream)->tmp, GETBUFLEN)) < 0)
	  if(errno != EINTR){
	      fs_give((void **) &ret);
	      pine_pipe_abort(stream);
	      return(NULL);
	  }

	ptr = S(stream)->tmp;
    }

    if(c == '\015' && *ptr == '\012'){
	ptr++;
	cnt--;
	ret[n - 1] = '\0';		/* tie off string with null */
    }
    else if (s = pine_pipe_getline(stream)) {
	ret = (char *) fs_get(n + 1 + (m = strlen (s)));
	memcpy(ret, sp, n);		/* copy first part */
	memcpy(ret + n, s, m);		/* and second part */
	fs_give((void **) &sp);		/* flush first part */
	fs_give((void **) &s);		/* flush second part */
	ret[n + m] = '\0';		/* tie off string with null */
    }

    return(ret);
}


/* 
 * pine_pipe_close - Replacement for tcp_close that closes pipes to our
 *		     child rather than a tcp connection
 */
void
pine_pipe_close(stream)
    void *stream;
{
    /*
     * Uninstall our hooks into smtp_send since it's being used by
     * the nntp driver as well...
     */
    (void) mail_parameters(NULL, SET_POSTSOUTR, sending_hooks.soutr);
    (void) mail_parameters(NULL, SET_POSTGETLINE, sending_hooks.getline);
    (void) mail_parameters(NULL, SET_POSTCLOSE, sending_hooks.close);
    (void) close_system_pipe((PIPE_S **) &stream);
}


/*
 * pine_pipe_abort - close down the pipe we were using to post
 */
void
pine_pipe_abort(stream)
    void *stream;
{
    if(S(stream)->in.d >= 0){
	close(S(stream)->in.d);
	S(stream)->in.d = -1;
    }

    if(S(stream)->out.d){
	close(S(stream)->out.d);
	S(stream)->out.d = -1;
    }
}
