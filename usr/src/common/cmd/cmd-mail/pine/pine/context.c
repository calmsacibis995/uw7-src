#if !defined(lint) && !defined(DOS)
static char rcsid[] = "$Id$";
#endif
/*
 * Program:	Mailbox Context Management
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 *
 * Pine and Pico are registered trademarks of the University of Washington.
 * No commercial use of these trademarks may be made without prior written
 * permission of the University of Washington.
 *
 * Pine, Pico, and Pilot software and its included text are Copyright
 * 1989-1996 by the University of Washington.
 *
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this distribution.
 *
 *
 * USENET News reading additions in part by L Lundblade / NorthWestNet, 1993
 * lgl@nwnet.net
 */

#include "headers.h"

char *current_context = NIL;	/* current context for FIND */

/* Mailbox found
 * Accepts: Mailbox name as FQN
 */

void
mm_mailbox (string)
    char *string;
{
    context_fqn_to_an (context_mailbox,current_context,string);
}


/* BBoard found
 * Accepts: BBoard name as FQN
 */

void
mm_bboard (string)
    char *string;
{
    context_fqn_to_an (context_bboard,current_context,string);
}

/* Convert fully-qualified name to ambiguous name and send to function
 * Accepts: find return function
 *	    current context
 *	    fully-qualified name
 */

void
context_fqn_to_an (f, context_string, fqn)
    find_return f;
    char *context_string,*fqn;
{
  char tmp[MAILTMPLEN];
  char *ret = fqn;
  char *delimiter,*prefix,*suffix;
  int pfxlen = 0;
  int sfxlen = 0;
  int fqnlen = strlen (fqn);
  int anlen;
  char context[MAILTMPLEN];

  if (context_string && *context_string && /* have a valid context? */
      context_digest(context_string, context, NULL, NULL, NULL) == NULL){
    delimiter = strstr(context, "%s");
				/* determine prefix */
    pfxlen = delimiter - context;
    if (pfxlen) {
      strncpy (tmp,context,pfxlen);
      prefix = tmp;
      prefix[pfxlen] = '\0';
    } else
      prefix = NIL;
				/* determine suffix */
    if (suffix = delimiter[2] ? delimiter+2 : NIL) sfxlen = strlen (suffix);

    /*
     * The fqn prefix may not match the context prefix.
     * Two cases so far:
     *   1) mail_find on open stream returns {host}context
     *   2) mail_find_all returns full paths though find string
     *      relative
     */

    if((ret=((!prefix) ? fqn : strstr(fqn, prefix))) != NULL){
	if(prefix && !strcmp(fqn = ret, prefix))
	  return;			/* no folder name! */

	fqnlen = strlen(fqn);
    }

    /* Determine if FQN is proper match for given prefix and suffix.  This
     * should normally always happen, but if it doesn't, it probably means
     * that this test isn't hairy enough and needs more work.
     */
    if (((anlen = fqnlen - (pfxlen + sfxlen)) < 1) ||
	(pfxlen && strncmp (prefix,fqn,pfxlen) != 0) ||
	(sfxlen && strcmp (suffix,fqn + fqnlen - sfxlen) != 0)) {
      sprintf (tmp,"Found mailbox outside context: %s",fqn);
      mm_log (tmp,(long) NIL);
      pfxlen = 0;
      anlen  = fqnlen;
    }
				/* copy the an */
    strcpy (ret = tmp,fqn+pfxlen);
    ret[anlen] = '\0';		/* tie off string */
  }
  (*f) (ret);			/* in case no context? */
}


/* Context Manager context format digester
 * Accepts: context string and buffers for sprintf-suitable context,
 *          remote host (for display), remote context (for display), 
 *          and view string
 * Returns: NULL if successful, else error string
 *
 * Comments: OK, here's what we expect a fully qualified context to 
 *           look like:
 *
 * [*] [{host ["/"<proto>] [:port]}] [<cntxt>] "[" [<view>] "]" [<cntxt>]
 *
 *         2) It's understood that double "[" or "]" are used to 
 *            quote a literal '[' or ']' in a context name.
 *
 *         3) an empty view in context implies a view of '*', so that's 
 *            what get's put in the view string
 *
 */
char *
context_digest (context, scontext, host, rcontext, view)
    char *context, *scontext, *host, *rcontext, *view;
{
    char *p, *viewp = view, tmp[MAILTMPLEN];
    int   i = 0;

    if((p = context) == NULL || *p == '\0'){
	if(scontext)			/* so the caller can apply */
	  strcpy(scontext, "%s");	/* folder names as is.     */

	return(NULL);			/* no error, just empty context */
    }

    if(!rcontext && scontext)		/* place to collect rcontext ? */
      rcontext = tmp;

    /* find hostname if requested and exists */
    if(*p == '{' || (*p == '*' && *++p == '{')){
	for(p++; *p && *p != '/' && *p != ':' && *p != '}' ; p++)
	  if(host)
	    *host++ = *p;		/* copy host if requested */

	while(*p && *p != '}')		/* find end of imap host */
	  p++;

	if(*p == '\0')
	  return("Unbalanced '}'");	/* bogus. */
	else
	  p++;				/* move into next field */
    }

    for(; *p ; p++){			/* get thru context */
	if(rcontext)
	  rcontext[i++] = *p;		/* copy if requested */

	if(*p == '['){			/* done? */
	    if(*++p == '\0')		/* look ahead */
	      return("Unbalanced '['");

	    if(*p != '[')		/* not quoted: "[[" */
	      break;
	}
    }

    if(*p == '\0')
      return("No '[' in context");

    for(; *p ; p++){			/* possibly in view portion ? */
	if(*p == ']'){
	    if(*(p+1) == ']')		/* is quoted */
	      p++;
	    else
	      break;
	}

	if(viewp)
	  *viewp++ = *p;
    }

    if(*p != ']')
      return("No ']' in context");

    for(; *p ; p++){			/* trailing context ? */
	if(rcontext)
	  rcontext[i++] = *p;
    }

    if(host) *host = '\0';
    if(rcontext) rcontext[i] = '\0';
    if(viewp) {
	if(viewp == view)
	  *viewp++ = '*';

	*viewp = '\0';
    }

    if(scontext){			/* sprint'able context request ? */
	if(*context == '*')
	  *scontext++ = *context++;

	if(*context == '{'){
	    while(*context && *context != '}')
	      *scontext++ = *context++;

	    *scontext++ = '}';
	}

	for(p = rcontext; *p ; p++){
	    if(*p == '[' && *(p+1) == ']'){
		*scontext++ = '%';	/* replace "[]" with "%s" */
		*scontext++ = 's';
		p++;			/* skip ']' */
	    }
	    else
	      *scontext++ = *p;
	}

	*scontext = '\0';
    }

    return(NULL);			/* no problems to report... */
}


/* Context Manager apply name to context
 * Accepts: buffer to write, context to apply, ambiguous folder name
 * Returns: buffer filled with fully qualified name in context
 *          No context applied if error
 */
void
context_apply(b, c, n)
    char *b, *c, *n;
{
    char tmp[MAILTMPLEN];

    if(context_digest(c, tmp, NULL, NULL, NULL) == NULL)
      sprintf(b, tmp, n);		/* everythings ok,  */
    else
      strcpy(b, n);			/* don't apply bogus context */
}


/* Context Manager check if name is ambiguous
 * Accepts: candidate string
 * Returns: T if ambiguous, NIL if fully-qualified
 */

int
context_isambig (s)
    char *s;
{
				/* first character after possible bboard * */
    char c = (*s == '*') ? *(s+1) : *s;
    if(c == '\0' && *s == '*')	/* special case! */
      return(1);
#if defined(DOS) || defined(OS2)
    else if (c != '{' && isalpha((unsigned char)*s) && *(s+1) == ':')
      return(0);
#endif
    else
      /*
       * Since we can't know how the server handles foldernames or
       * heirarchy, treat only what we know as being absolute folder
       * specifiers.  We know '{' means a c-client remote folder, and
       * that '/' breaks out of the current context under UNIX, and we're
       * guessing that '[' and ':' are special under VMS (for which
       * a couple of imapd's are known).  If you know this guess to be
       * wrong or know of other servers, please send us diffs for this...
       * 
       * The good news is, THIS MUST AND WILL BE REMOVED
       * WITH THE COMING IMAP4 SUPPORT!!!!
       */
      return(!(c == '{' || c == '/' || c == ':' || c == '[' || c == '#'
	       || strucmp(s, ps_global->inbox_name) == 0));
}

/* Context Manager find list of subscribed mailboxes
 * Accepts: context
 *	    mail stream
 *	    pattern to search
 */

void
context_find (context, stream, pat)
    char       *context;
    MAILSTREAM *stream;char *pat;
{
  char tmp[MAILTMPLEN];
				/* must be within context */
  if (!context_isambig (pat)) {
    sprintf (tmp,"Find of mailbox outside context: %s",pat);
    mm_log (tmp,(long) NIL);
    strcpy(tmp, pat);		/* allow find for now */
  }
  else{
    current_context = context;	/* note current context */
    context_apply (tmp,context,pat); /* build fully-qualified name */
  }

  mail_find (stream,tmp);/* try to do the find */
}


/* Context Manager find list of subscribed bboards
 * Accepts: context
 *	    mail stream
 *	    pattern to search
 */

void
context_find_bboards (context, stream, pat)
    char       *context;
    MAILSTREAM *stream;
    char       *pat;
{
  char tmp[MAILTMPLEN];
				/* must be within context */
  if (!context_isambig (pat)) {
    sprintf (tmp,"Find of bboard outside context: %s",pat);
    mm_log (tmp,(long) NIL);
    strcpy(tmp, pat);		/* allow find for now */
  }
  else{
    current_context = context;	/* note current context */
    context_apply (tmp,context,pat); /* build fully-qualified name */
  }

  mail_find_bboards (stream,tmp);
}

/* Context Manager find list of all mailboxes
 * Accepts: context
 *	    mail stream
 *	    pattern to search
 */

void
context_find_all (context, stream, pat)
    char       *context;
    MAILSTREAM *stream;
    char       *pat;
{
  char tmp[MAILTMPLEN];
				/* must be within context */
  if (!context_isambig (pat)) {
    sprintf (tmp,"Find of mailbox outside context: %s",pat);
    mm_log (tmp,(long) NIL);
    strcpy(tmp, pat);		/* allow find for now */
  }
  else{
    current_context = context;	/* note current context */
    context_apply (tmp,context,pat); /* build fully-qualified name */
  }
				/* try to do the find */
  mail_find_all (stream,tmp);
}


/* Context Manager find list of all bboards
 * Accepts: context
 *	    mail stream
 *	    pattern to search
 */

void
context_find_all_bboard (context, stream, pat)
    char       *context;
    MAILSTREAM *stream;
    char       *pat;
{
  char tmp[MAILTMPLEN];
				/* must be within context */
  if (!context_isambig (pat)) {
    sprintf (tmp,"Find of bboard outside context: %s",pat);
    mm_log (tmp,(long) NIL);
    strcpy(tmp, pat);		/* allow find for now */
  }
  else{
    current_context = context;	/* note current context */
    context_apply (tmp,context,pat); /* build fully-qualified name */
  }
				/* try to do the find */
  mail_find_all_bboard (stream,tmp);
}
#ifdef NEWBB
/* Context Manager find list of new bboards
 * Accepts: context
 *	    mail stream
 *	    pattern to search
 */

void
context_find_new_bboard (context, stream, pat, lasttime)
    char       *context;
    MAILSTREAM *stream;
    char       *pat, *lasttime;
{
  char tmp[MAILTMPLEN];
				/* must be within context */
  if (!context_isambig (pat)) {
    sprintf (tmp,"Find of bboard outside context: %s",pat);
    mm_log (tmp,WARN);
    strcpy(tmp, pat);		/* allow find for now */
  }
  else{
    current_context = context;	/* note current context */
    context_apply (tmp,context,pat); /* build fully-qualified name */
  }
				/* try to do the find */
  nntp_find_new_bboard (stream,tmp, lasttime);
}

#endif

/* Context Manager create mailbox
 * Accepts: context
 *	    mail stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long
context_create (context, stream, mailbox)
    char       *context;
    MAILSTREAM *stream;
    char       *mailbox;
{
  char tmp[MAILTMPLEN];		/* must be within context */

  if (!context_isambig (mailbox)) {
    char *s, errbuf[MAILTMPLEN];
    if(mailbox[0] != '{' && context[0] == '{' && (s = strindex(context, '}'))){
      strncpy(tmp, context, (s - context) + 1);
      strcpy(&tmp[(s - context)+1], mailbox);
    }
    else strcpy(tmp, mailbox);	/* allow create for now */

    sprintf (errbuf,"Create of mailbox outside context: %s", tmp);
    mm_log (errbuf,(long) NIL);
  }
  else
    context_apply (tmp,context,mailbox);/* build fully-qualified name */
				/* try to do the create */
  return(cntxt_allowed(tmp) ? mail_create(stream,tmp) : 0L);
}


/* Context Manager open
 * Accepts: context
 *	    candidate stream for recycling
 *	    mailbox name
 *	    open options
 * Returns: stream to use on success, NIL on failure
 */

MAILSTREAM *
context_open (context, old, name, opt)
    char       *context;
    MAILSTREAM *old;
    char       *name;
    long	opt;
{
  char *s,tmp[MAILTMPLEN];	/* build FQN from ambiguous name */
  char host[MAILTMPLEN];

  if (!context) context = "[]";	/* non spec'd, pass name */
  if(context_isambig (name)) context_apply(s = tmp, context, name);
  else if(name[0] != '{' && context[0] == '{' && (s = strindex(context, '}'))){
      strncpy(tmp, context, (s - context) + 1);
      strcpy(&tmp[(s - context)+1], name);
      s = tmp;
  }
  else s = name;		/* fully-qualified name */
  return(cntxt_allowed(s) ? mail_open(old,s,opt) : (MAILSTREAM *)NULL);
}


/* Context Manager rename
 * Accepts: context
 *	    mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long
context_rename (context, stream, old, new)
    char       *context;
    MAILSTREAM *stream;
    char       *old, *new;
{
  char *s,*t,tmp[MAILTMPLEN],tmp2[MAILTMPLEN];
	/* build FQN from ambiguous name */
  if (context_isambig (old)) context_apply (s = tmp,context,old);
  else if(old[0] != '{' && context[0] == '{' && (s = strindex(context, '}'))){
      strncpy(tmp, context, (s - context) + 1);
      strcpy(&tmp[(s - context)+1], old);
      s = tmp;
  }
  else s = old;		/* fully-qualified name */
  if (context_isambig (new)) context_apply (t = tmp2,context,new);
  else if(new[0] != '{' && context[0] == '{' && (t = strindex(context, '}'))){
      strncpy(tmp, context, (t - context) + 1);
      strcpy(&tmp[(t - context)+1], new);
      t = tmp;
  }
  else t = new;		/* fully-qualified name */
  return((cntxt_allowed(s) && cntxt_allowed(t)) ? mail_rename(stream,s,t)
						 : 0L);
}


/* Context Manager delete mailbox
 * Accepts: context
 *	    mail stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long
context_delete (context, stream, name)
    char       *context;
    MAILSTREAM *stream;
    char       *name;
{
  char *s,tmp[MAILTMPLEN];	/* build FQN from ambiguous name */
  if (context_isambig (name)) context_apply (s = tmp,context,name);
  else if(name[0] != '{' && context[0] == '{' && (s = strindex(context, '}'))){
      strncpy(tmp, context, (s - context) + 1);
      strcpy(&tmp[(s - context)+1], name);
      s = tmp;
  }
  else s = name;		/* fully-qualified name */
  return(cntxt_allowed(s) ? mail_delete(stream,s) : 0L);
}


/* Context Manager append message string
 * Accepts: context
 *	    mail stream
 *	    destination mailbox
 *	    stringstruct of message to append
 * Returns: T on success, NIL on failure
 */

long
context_append (context, stream, name, msg)
    char       *context;
    MAILSTREAM *stream;
    char       *name;
    STRING     *msg;
{
  char *s,tmp[MAILTMPLEN];	/* build FQN from ambiguous name */
  if (context_isambig (name)) context_apply (s = tmp,context,name);
  else if(name[0] != '{' && context[0] == '{' && (s = strindex(context, '}'))){
      strncpy(tmp, context, (s - context) + 1);
      strcpy(&tmp[(s - context)+1], name);
      s = tmp;
  }
  else s = name;		/* fully-qualified name */
  return(cntxt_allowed(s) ? mail_append(stream,s,msg) : 0L);
}


/* Context Manager append message string with flags
 * Accepts: context
 *	    mail stream
 *	    destination mailbox
 *          flags to assign message being appended
 *          date of message being appended
 *	    stringstruct of message to append
 * Returns: T on success, NIL on failure
 */

long context_append_full (context, stream, name, flags, date, msg)
    char       *context;
    MAILSTREAM *stream;
    char       *name, *flags, *date;
    STRING     *msg;
{
  char *s,tmp[MAILTMPLEN];	/* build FQN from ambiguous name */
  if (context_isambig (name)) context_apply (s = tmp,context,name);
  else if(name[0] != '{' && context[0] == '{' && (s = strindex(context, '}'))){
      strncpy(tmp, context, (s - context) + 1);
      strcpy(&tmp[(s - context)+1], name);
      s = tmp;
  }
  else s = name;		/* fully-qualified name */
  return(cntxt_allowed(s) ? mail_append_full(stream,s,flags,date,msg) : 0L);
}


/* Mail copy message(s)
 * Accepts: context
 *	    mail stream
 *	    sequence
 *	    destination mailbox
 */

long context_copy (context, stream, sequence, name)
    char *context;
    MAILSTREAM *stream;
    char *sequence;
    char *name;
{
  char *s,tmp[MAILTMPLEN];		/* build FQN from ambiguous name */
  if (context_isambig (name)) context_apply (tmp,context,name);
  else if(name[0] != '{' && context[0] == '{' && (s = strindex(context, '}'))){
      strncpy(tmp, context, (s - context) + 1);
      strcpy(&tmp[(s - context)+1], name);
  }
  else strcpy(tmp,name);		/* fully-qualified name */
  if((*(s=tmp) == '{' || (*s == '*' && *(s+1) == '{')) && (s=strindex(s,'}'))){
      if(!*++s)				/* strip imap host name ? */
	strcpy(s = tmp, "INBOX");	/* presume "inbox" ala c-client */
  }

  return((s && cntxt_allowed(s)) ? mail_copy(stream,sequence,s) : 0L);
}


/* Context Manager subscribe
 * Accepts: context
 *	    mail stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long
context_subscribe (context, stream, name)
    char       *context;
    MAILSTREAM *stream;
    char       *name;
{
  char *s,tmp[MAILTMPLEN];	/* build FQN from ambiguous name */
  if (context_isambig (name)) context_apply (s = tmp,context,name);
  else s = name;		/* fully-qualified name */
  return mail_subscribe (stream,s);
}


/* Context Manager unsubscribe
 * Accepts: context
 *	    mail stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long
context_unsubscribe (context, stream, name)
    char       *context;
    MAILSTREAM *stream;
    char       *name;
{
  char *s,tmp[MAILTMPLEN];	/* build FQN from ambiguous name */
  if (context_isambig (name)) context_apply (s = tmp,context,name);
  else s = name;		/* fully-qualified name */
  return mail_unsubscribe (stream,s);
}


/*
 * Context manager stream usefulness test
 * Accepts: context
 *	    mail name
 *	    mailbox name
 *	    mail stream to test against mailbox name
 * Returns: stream if useful, else NIL
 */
MAILSTREAM *
context_same_stream(context, name, stream)
    char       *context;
    MAILSTREAM *stream;
    char       *name;
{
  extern MAILSTREAM *same_stream();
  char *s,tmp[MAILTMPLEN];	/* build FQN from ambiguous name */
  if (context_isambig (name)) context_apply (s = tmp,context,name);
  else if(name[0] != '{' && context[0] == '{' && (s = strindex(context, '}'))){
      strncpy(tmp, context, (s - context) + 1);
      strcpy(&tmp[(s - context)+1], name);
      s = tmp;
  }
  else s = name;		/* fully-qualified name */
  return same_stream (s,stream);
}


/*
 * Context manager driver compatibility test
 * Accepts: context
 *	    mail name
 *	    mailbox name
 *	    mail stream to test against mailbox name
 * Returns: stream if drivers are the same, else NIL
 */
MAILSTREAM *
context_same_driver(context, name, stream)
    char       *context;
    MAILSTREAM *stream;
    char       *name;
{
  MAILSTREAM *proto_stream;
  int rv;
  char *s,tmp[MAILTMPLEN];	/* build FQN from ambiguous name */
  if (context_isambig (name)) context_apply (s = tmp,context,name);
  else if(name[0] != '{' && context[0] == '{' && (s = strindex(context, '}'))){
      strncpy(tmp, context, (s - context) + 1);
      strcpy(&tmp[(s - context)+1], name);
      s = tmp;
  }
  else s = name;		/* fully-qualified name */

  if(!cntxt_allowed(s))
    return((MAILSTREAM *)NULL);

  /* IF we're given a stream to compare *and* it's not remote *and*
   * the given mailbox name isn't remote *and* we know the driver
   * the given mailbox will be in *and* the drivers are the same
   * return the given stream.
   *
   * Also, turn off any mm_log errors that might bubble up from 
   * mail_open because anything other than "doesn't exist" will
   * get encountered during whatever action follows this call...
   */
  ps_global->try_to_create = 1;
  rv = (stream
	&& !((*stream->mailbox == '{'
	      || (*stream->mailbox == '*' && *(stream->mailbox + 1) == '{'))
	     || (*s == '{' || (*s == '*' && *(s+1) == '{')))
	&& ((proto_stream = mail_open(NIL,s,OP_PROTOTYPE))
	    || (*s != '#' && (proto_stream = default_proto())))
	&& !strcmp(stream->dtb->name, proto_stream->dtb->name));
  ps_global->try_to_create = 0;

  return(rv ? stream : NULL);
}


#ifdef NEWBB

/*  This is code for inclusion in the c-client to support the command
    to discover all the new newly created news groups, since the last
    time the subscription screen was viewed. 

    Some of this code goes un nntpunx.c and one line in nntpunx.h

    It's not clear whether or not this code will be included as part
    of the official c-client distribution so it's included here.
*/

#ifdef INCLUDE_THIS_CODE_AT_THE_END_OF_NNTPUNX_C

/* NNTP mail find list of all bboards
 * Accepts: mail stream
 *	    pattern to search
 *          last time that new groups were checked for in NNTP format
 */

void nntp_find_new_bboard (stream, pat, lasttime)
    MAILSTREAM *stream;
    char       *pat, *lasttime;
{
  char *s,*t,*bbd,*patx,tmp[MAILTMPLEN], checktime[20];
				/* use .newsrc if a stream given */
  if (stream && LOCAL && LOCAL->nntpstream) {
				/* begin with a host specification? */
    if (((*pat == '{') || ((*pat == '*') && (pat[1] == '{'))) &&
	(t = strchr (pat,'}')) && *(patx = ++t)) {
      if (*pat == '*') pat++;	/* yes, skip leading * (old Pine behavior) */
      strcpy (tmp,pat);		/* copy host name */
      bbd = tmp + (patx - pat);	/* where we write the bboards */
    }
    else {			/* no host specification */
      bbd = tmp;		/* no prefix */
      patx = pat;		/* use entire specification */
    }
				/* ask server for all active newsgroups */
    if (!(smtp_send (LOCAL->nntpstream,"NEWGROUPS",lasttime) == NNTPNEWLIST))
      return;
				/* process data until we see final dot */
    while ((s = tcp_getline (LOCAL->nntpstream->tcpstream)) && *s != '.') {
				/* tie off after newsgroup name */
      if (t = strchr (s,' ')) *t = '\0';
      if (pmatch (s,patx)) {	/* report to main program if have match */
	strcpy (bbd,s);		/* write newsgroup name after prefix */
	mm_bboard(tmp);
      }
      fs_give ((void **) &s);	/* clean up */
    }
  }
}

     
#endif


#ifdef INCLUDE_THIS_CODE_NEAR_THE_TOP_OF_NNTPUNX_H

#define NNTPNEWLIST (long) 231  /* NNTP list of new news groups */

#endif

#endif /* NEWBB */
