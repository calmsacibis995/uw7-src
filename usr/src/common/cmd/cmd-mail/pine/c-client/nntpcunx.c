/*
 * Program:	Network News Transfer Protocol (NNTP) client routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	5 January 1993
 * Last Edited:	15 May 1996
 *
 * Copyright 1996 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made
 * available "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */


#include "mail.h"
#include "osdep.h"
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
extern int errno;		/* just in case */
#include <sys/stat.h>
#include "smtp.h"
#include "nntp.h"
#include "nntpcunx.h"
#include "rfc822.h"
#include "misc.h"
#include "newsrc.h"

/* NNTP mail routines */


/* Driver dispatch used by MAIL */

DRIVER nntpdriver = {
  "nntp",			/* driver name */
  (DRIVER *) NIL,		/* next driver */
  nntp_valid,			/* mailbox is valid for us */
  nntp_parameters,		/* manipulate parameters */
  nntp_find,			/* find mailboxes */
  nntp_find_bboards,		/* find bboards */
  nntp_find_all,		/* find all mailboxes */
  nntp_find_all_bboards,	/* find all bboards */
  nntp_subscribe,		/* subscribe to mailbox */
  nntp_unsubscribe,		/* unsubscribe from mailbox */
  nntp_subscribe_bboard,	/* subscribe to bboard */
  nntp_unsubscribe_bboard,	/* unsubscribe from bboard */
  nntp_create,			/* create mailbox */
  nntp_delete,			/* delete mailbox */
  nntp_rename,			/* rename mailbox */
  nntp_mopen,			/* open mailbox */
  nntp_close,			/* close mailbox */
  nntp_fetchfast,		/* fetch message "fast" attributes */
  nntp_fetchflags,		/* fetch message flags */
  nntp_fetchstructure,		/* fetch message envelopes */
  nntp_fetchheader,		/* fetch message header only */
  nntp_fetchtext,		/* fetch message body only */
  nntp_fetchbody,		/* fetch message body section */
  nntp_setflag,			/* set message flag */
  nntp_clearflag,		/* clear message flag */
  nntp_search,			/* search for message based on criteria */
  nntp_ping,			/* ping mailbox to see if still alive */
  nntp_check,			/* check for new messages */
  nntp_expunge,			/* expunge deleted messages */
  nntp_copy,			/* copy messages to another mailbox */
  nntp_move,			/* move messages to another mailbox */
  nntp_append,			/* append string message to mailbox */
  nntp_gc			/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM nntpproto = {&nntpdriver};

/* NNTP mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *nntp_valid (name)
	char *name;
{
				/* must be bboard */
  return *name == '*' ? mail_valid_net (name,&nntpdriver,NIL,NIL) : NIL;
}


/* News manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *nntp_parameters (function,value)
	long function;
	void *value;
{
  return NIL;
}

/* NNTP mail find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void nntp_find (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  /* Always a no-op */
}


/* NNTP mail find list of bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void nntp_find_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
				/* use .newsrc if a stream given */
  if (stream && !stream->anonymous) newsrc_find (pat);
}

/* NNTP mail find list of all mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void nntp_find_all (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  /* Always a no-op */
}


/* NNTP mail find list of all bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void nntp_find_all_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  char *s,*t,*bbd,*patx,tmp[MAILTMPLEN];
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
    if (!(nntp_send (LOCAL->nntpstream,"LIST","ACTIVE") == NNTPGLIST)) return;
				/* process data until we see final dot */
    while ((s = tcp_getline (LOCAL->nntpstream->tcpstream)) && *s != '.') {
				/* tie off after newsgroup name */
      if (t = strchr (s,' ')) *t = '\0';
      if (pmatch (s,patx)) {	/* report to main program if have match */
	strcpy (bbd,s);		/* write newsgroup name after prefix */
	mm_bboard (tmp);
      }
      fs_give ((void **) &s);	/* clean up */
    }
  }
}

/* NNTP mail subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long nntp_subscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for NNTP */
}


/* NNTP mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long nntp_unsubscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for NNTP */
}


/* NNTP mail subscribe to bboard
 * Accepts: mail stream
 *	    bboard to add to subscription list
 * Returns: T on success, NIL on failure
 */

long nntp_subscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  char *s = strchr (mailbox,'}');
  return s ? newsrc_update (s+1,':') : NIL;
}


/* NNTP mail unsubscribe to bboard
 * Accepts: mail stream
 *	    bboard to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long nntp_unsubscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  char *s = strchr (mailbox,'}');
  return s ? newsrc_update (s+1,'!') : NIL;
}

/* NNTP mail create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long nntp_create (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for NNTP */
}


/* NNTP mail delete mailbox
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long nntp_delete (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for NNTP */
}


/* NNTP mail rename mailbox
 * Accepts: mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long nntp_rename (stream,old,new)
	MAILSTREAM *stream;
	char *old;
	char *new;
{
  return NIL;			/* never valid for NNTP */
}

/* NNTP mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *nntp_mopen (stream)
	MAILSTREAM *stream;
{
  long i,j,k;
  long nmsgs = 0;
  long unseen = 0;
  char c = NIL,*s,*t,tmp[MAILTMPLEN];
  NETMBX mb;
  void *tcpstream;
  SMTPSTREAM *nstream = NIL;
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &nntpproto;
  mail_valid_net_parse (stream->mailbox,&mb);
  if (!*mb.mailbox) strcpy (mb.mailbox,"general");
  if (LOCAL) {			/* if recycle stream, see if changing hosts */
    if (strcmp (lcase (mb.host),lcase (strcpy (tmp,LOCAL->host)))) {
      sprintf (tmp,"Closing connection to %s",LOCAL->host);
      if (!stream->silent) mm_log (tmp,(long) NIL);
    }
    else {			/* same host, preserve NNTP connection */
      sprintf (tmp,"Reusing connection to %s",LOCAL->host);
      if (!stream->silent) mm_log (tmp,(long) NIL);
      nstream = LOCAL->nntpstream;
      LOCAL->nntpstream = NIL;	/* keep nntp_close() from punting it */
    }
    nntp_close (stream);	/* do close action */
    stream->dtb = &nntpdriver;/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
				/* in case /debug switch given */
  if (mb.dbgflag) stream->debug = T;

  if (!nstream) {		/* open NNTP now if not already open */
    char *hostlist[2];
    hostlist[0] = strcpy (tmp,mb.host);
    if (mb.port) sprintf (tmp + strlen (tmp),":%ld",mb.port);
    hostlist[1] = NIL;
    nstream = nntp_open (hostlist,OP_READONLY+(stream->debug ? OP_DEBUG:NIL));
  }
  if (nstream) {		/* now try to open newsgroup */
    if ((!stream->halfopen) &&	/* open the newsgroup if not halfopen */
	((nntp_send (nstream,"GROUP",mb.mailbox) != NNTPGOK) ||
	 ((k = strtol (nstream->reply + 4,&s,10)) < 0) ||
	 ((i = strtol (s,&s,10)) < 0) || ((j = strtol (s,&s,10)) < 0) ||
	 (nmsgs = i | j ? 1 + j - i : 0) < 0)) {
      mm_log (nstream->reply,ERROR);
      smtp_close (nstream);	/* punt stream */
      nstream = NIL;
      return NIL;
    }
				/* newsgroup open, instantiate local data */
    stream->local = fs_get (sizeof (NNTPLOCAL));
    LOCAL->nntpstream = nstream;
    LOCAL->dirty = NIL;		/* no update to .newsrc needed yet */
				/* copy host and newsgroup name */
    LOCAL->host = cpystr (mb.host);
    LOCAL->name = cpystr (mb.mailbox);
    stream->sequence++;		/* bump sequence number */
    stream->rdonly = T;		/* make sure higher level knows readonly */
    LOCAL->number = NIL;
    LOCAL->header = LOCAL->body = NIL;
    LOCAL->buf = NIL;

    if (!stream->halfopen) {	/* if not half-open */
      if (nmsgs) {		/* find what messages exist */
	LOCAL->number = (unsigned long *) fs_get (nmsgs*sizeof(unsigned long));
	sprintf (tmp,"%ld-%ld",i,j);
	if ((nntp_send (nstream,"LISTGROUP",mb.mailbox) == NNTPGOK) ||
	    (nntp_send (nstream,"XHDR Date",tmp) == NNTPHEAD)) {
	  for (i = 0; (s = tcp_getline (nstream->tcpstream)) && strcmp (s,".");
	       i++) {		/* initialize c-client/NNTP map */
	    if (i < nmsgs) LOCAL->number[i] = atol (s);
	    fs_give ((void **) &s);
	  }
	  if (s) fs_give ((void **) &s);
	  if (i != nmsgs) {	/* found holes? */
	    if (nmsgs = i)
	      fs_resize ((void **) &LOCAL->number,nmsgs*sizeof(unsigned long));
	    else fs_give ((void **) &LOCAL->number);
	  }
	}
				/* assume c-client/NNTP map is entire range */
	else for (k = 0; k < nmsgs; ++k) LOCAL->number[k] = i + k;
				/* create caches */
	LOCAL->header = (char **) fs_get (nmsgs * sizeof (char *));
	LOCAL->body = (char **) fs_get (nmsgs * sizeof (char *));
				/* initialize per-message cache */
	for (i = 0; i < nmsgs; ++i) LOCAL->header[i] = LOCAL->body[i] = NIL;
      }
				/* make temporary buffer */
      LOCAL->buf = (char *) fs_get ((LOCAL->buflen = MAXMESSAGESIZE) + 1);
				/* notify upper level that messages exist */
      mail_exists (stream,nmsgs);
				/* read .newsrc entries */
      mail_recent (stream,newsrc_read (LOCAL->name,stream,LOCAL->number));
				/* notify if empty bboard */
      if (!(stream->nmsgs || stream->silent)) {
	sprintf (tmp,"Newsgroup %s is empty",LOCAL->name);
	mm_log (tmp,WARN);
      }
    }
  }
  return LOCAL ? stream : NIL;	/* if stream is alive, return to caller */
}

/* NNTP mail close
 * Accepts: MAIL stream
 */

void nntp_close (stream)
	MAILSTREAM *stream;
{
  if (LOCAL) {			/* only if a file is open */
    nntp_check (stream);	/* dump final checkpoint */
    if (LOCAL->name) fs_give ((void **) &LOCAL->name);
    if (LOCAL->host) fs_give ((void **) &LOCAL->host);
    nntp_gc (stream,GC_TEXTS);	/* free local cache */
    if (LOCAL->number) fs_give ((void **) &LOCAL->number);
    if (LOCAL->header) fs_give ((void **) &LOCAL->header);
    if (LOCAL->body) fs_give ((void **) &LOCAL->body);
				/* free local scratch buffer */
    if (LOCAL->buf) fs_give ((void **) &LOCAL->buf);
				/* close NNTP connection */
    if (LOCAL->nntpstream) smtp_close (LOCAL->nntpstream);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}

/* NNTP mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 */

void nntp_fetchfast (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  long i;
  BODY *b;
				/* ugly and slow */
  if (stream && LOCAL && mail_sequence (stream,sequence))
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_elt (stream,i)->sequence)
	nntp_fetchstructure (stream,i,&b);
}


/* NNTP mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 */

void nntp_fetchflags (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  return;			/* no-op for local mail */
}

/* NNTP mail fetch envelope
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *nntp_fetchstructure (stream,msgno,body)
	MAILSTREAM *stream;
	long msgno;
	BODY **body;
{
  char *h,*t;
  LONGCACHE *lelt;
  ENVELOPE **env;
  STRING bs;
  BODY **b;
  unsigned long hdrsize;
  unsigned long textsize = 0;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  if (stream->scache) {		/* short cache */
    if (msgno != stream->msgno){/* flush old poop if a different message */
      mail_free_envelope (&stream->env);
      mail_free_body (&stream->body);
    }
    stream->msgno = msgno;
    env = &stream->env;		/* get pointers to envelope and body */
    b = &stream->body;
  }
  else {			/* long cache */
    lelt = mail_lelt (stream,msgno);
    env = &lelt->env;		/* get pointers to envelope and body */
    b = &lelt->body;
  }
  if ((body && !*b) || !*env) {	/* have the poop we need? */
    mail_free_envelope (env);	/* flush old envelope and body */
    mail_free_body (b);
    hdrsize = strlen (h = nntp_fetchheader (stream,msgno));
    if (body) {			/* only if want to parse body */
      textsize = strlen (t = nntp_fetchtext_work (stream,msgno));
				/* calculate message size */
      elt->rfc822_size = hdrsize + textsize;
      INIT (&bs,mail_string,(void *) t,textsize);
    }
				/* parse envelope and body */
    rfc822_parse_msg (env,body ? b : NIL,h,hdrsize,body ? &bs : NIL,BADHOST,
		      LOCAL->buf);
				/* parse date */
    if (*env && (*env)->date) mail_parse_date (elt,(*env)->date);
    if (!elt->month) mail_parse_date (elt,"01-JAN-1969 00:00:00 GMT");
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* NNTP mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *nntp_fetchheader (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  char tmp[MAILTMPLEN];
  long m = msgno - 1;
  if (!LOCAL->header[m]) {	/* fetch header if don't have already */
    sprintf (tmp,"%ld",LOCAL->number[m]);
    if (nntp_send (LOCAL->nntpstream,"HEAD",tmp) == NNTPHEAD)
      LOCAL->header[m] = nntp_slurp (stream);
				/* failed, mark as deleted */
    else mail_elt (stream,msgno)->deleted = T;
  }
  return LOCAL->header[m] ? LOCAL->header[m] : "";
}


/* NNTP mail fetch message text (only)
	body only;
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *nntp_fetchtext (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  long m = msgno - 1;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  elt->seen = T;		/* mark as seen */
  return nntp_fetchtext_work (stream,msgno);
}


/* NNTP mail fetch message text work
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *nntp_fetchtext_work (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  char tmp[MAILTMPLEN];
  long m = msgno - 1;
  if (!LOCAL->body[m]) {	/* fetch body if don't have already */
    sprintf (tmp,"%ld",LOCAL->number[m]);
    if (nntp_send (LOCAL->nntpstream,"BODY",tmp) == NNTPBODY)
      LOCAL->body[m] = nntp_slurp (stream);
				/* failed, mark as deleted */
    else mail_elt (stream,msgno)->deleted = T;
  }
  return LOCAL->body[m] ? LOCAL->body[m] : "";
}

/* NNTP mail slurp NNTP dot-terminated text
 * Accepts: MAIL stream
 * Returns: text
 */

char *nntp_slurp (stream)
	MAILSTREAM *stream;
{
  char *s,*t;
  unsigned long i;
  unsigned long bufpos = 0;
  while (s = tcp_getline (LOCAL->nntpstream->tcpstream)) {
    if (*s == '.') {		/* possible end of text? */
      if (s[1]) t = s + 1;	/* pointer to true start of line */
      else {
	fs_give ((void **) &s);	/* free the line */
	break;			/* end of data */
      }
    }
    else t = s;			/* want the entire line */
				/* ensure have enough room */
    if (LOCAL->buflen < (bufpos + (i = strlen (t)) + 5))
      fs_resize ((void **) &LOCAL->buf,LOCAL->buflen += (MAXMESSAGESIZE + 1));
				/* copy the text */
    strncpy (LOCAL->buf + bufpos,t,i);
    bufpos += i;		/* set new buffer position */
    LOCAL->buf[bufpos++] = '\015';
    LOCAL->buf[bufpos++] = '\012';
    fs_give ((void **) &s);	/* free the line */
  }
  LOCAL->buf[bufpos++] = '\015';/* add final newline */
  LOCAL->buf[bufpos++] = '\012';
  LOCAL->buf[bufpos++] = '\0';	/* tie off string with NUL */
  return cpystr (LOCAL->buf);	/* return copy of collected string */
}

/* NNTP fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 * Returns: pointer to section of message body
 */

char *nntp_fetchbody (stream,m,s,len)
	MAILSTREAM *stream;
	long m;
	char *s;
	unsigned long *len;
{
  BODY *b;
  PART *pt;
  unsigned long i;
  char *base;
  unsigned long offset = 0;
  MESSAGECACHE *elt = mail_elt (stream,m);
				/* make sure have a body */
  if (!(nntp_fetchstructure (stream,m,&b) && b && s && *s &&
	((i = strtol (s,&s,10)) > 0) &&
	(base = nntp_fetchtext_work (stream,m))))
    return NIL;
  do {				/* until find desired body part */
				/* multipart content? */
    if (b->type == TYPEMULTIPART) {
      pt = b->contents.part;	/* yes, find desired part */
      while (--i && (pt = pt->next));
      if (!pt) return NIL;	/* bad specifier */
				/* note new body, check valid nesting */
      if (((b = &pt->body)->type == TYPEMULTIPART) && !*s) return NIL;
      offset = pt->offset;	/* get new offset */
    }
    else if (i != 1) return NIL;/* otherwise must be section 1 */
				/* need to go down further? */
    if (i = *s) switch (b->type) {
    case TYPEMESSAGE:		/* embedded message, calculate new base */
      offset = b->contents.msg.offset;
      b = b->contents.msg.body;	/* get its body, drop into multipart case */
    case TYPEMULTIPART:		/* multipart, get next section */
      if ((*s++ == '.') && (i = strtol (s,&s,10)) > 0) break;
    default:			/* bogus subpart specification */
      return NIL;
    }
  } while (i);
				/* lose if body bogus */
  if ((!b) || b->type == TYPEMULTIPART) return NIL;
  elt->seen = T;		/* mark as seen */
  return rfc822_contents (&LOCAL->buf,&LOCAL->buflen,len,base + offset,
			  b->size.ibytes,b->encoding);
}

/* NNTP mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void nntp_setflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  MESSAGECACHE *elt;
  long i;
  short f = nntp_getflags (stream,flag);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 0; i < stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i + 1))->sequence) {
      if (f&fSEEN) elt->seen=T;	/* set all requested flags */
      if (f&fDELETED) {		/* deletion also purges the cache */
	elt->deleted = T;	/* mark deleted */
	LOCAL->dirty = T;	/* mark dirty */
	if (LOCAL->header[i]) fs_give ((void **) &LOCAL->header[i]);
	if (LOCAL->body[i]) fs_give ((void **) &LOCAL->body[i]);
      }
      if (f&fFLAGGED) elt->flagged = T;
      if (f&fANSWERED) elt->answered = T;
    }
}


/* NNTP mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void nntp_clearflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  MESSAGECACHE *elt;
  long i;
  short f = nntp_getflags (stream,flag);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 0; i < stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i + 1))->sequence) {
				/* clear all requested flags */
      if (f&fSEEN) elt->seen = NIL;
      if (f&fDELETED) {
	elt->deleted = NIL;	/* undelete */
	LOCAL->dirty = T;	/* mark stream as dirty */
      }
      if (f&fFLAGGED) elt->flagged = NIL;
      if (f&fANSWERED) elt->answered = NIL;
    }
}

/* NNTP mail search for messages
 * Accepts: MAIL stream
 *	    search criteria
 */

void nntp_search (stream,criteria)
	MAILSTREAM *stream;
	char *criteria;
{
  long i,n;
  char *d;
  search_t f;
				/* initially all searched */
  for (i = 1; i <= stream->nmsgs; ++i) mail_elt (stream,i)->searched = T;
				/* get first criterion */
  if (criteria && (criteria = strtok (criteria," "))) {
				/* for each criterion */
    for (; criteria; (criteria = strtok (NIL," "))) {
      f = NIL; d = NIL; n = 0;	/* init then scan the criterion */
      switch (*ucase (criteria)) {
      case 'A':			/* possible ALL, ANSWERED */
	if (!strcmp (criteria+1,"LL")) f = nntp_search_all;
	else if (!strcmp (criteria+1,"NSWERED")) f = nntp_search_answered;
	break;
      case 'B':			/* possible BCC, BEFORE, BODY */
	if (!strcmp (criteria+1,"CC"))
	  f = nntp_search_string (nntp_search_bcc,&d,&n);
	else if (!strcmp (criteria+1,"EFORE"))
	  f = nntp_search_date (nntp_search_before,&n);
	else if (!strcmp (criteria+1,"ODY"))
	  f = nntp_search_string (nntp_search_body,&d,&n);
	break;
      case 'C':			/* possible CC */
	if (!strcmp (criteria+1,"C"))
	  f = nntp_search_string (nntp_search_cc,&d,&n);
	break;
      case 'D':			/* possible DELETED */
	if (!strcmp (criteria+1,"ELETED")) f = nntp_search_deleted;
	break;
      case 'F':			/* possible FLAGGED, FROM */
	if (!strcmp (criteria+1,"LAGGED")) f = nntp_search_flagged;
	else if (!strcmp (criteria+1,"ROM"))
	  f = nntp_search_string (nntp_search_from,&d,&n);
	break;
      case 'K':			/* possible KEYWORD */
	if (!strcmp (criteria+1,"EYWORD"))
	  f = nntp_search_flag (nntp_search_keyword,&d);
	break;
      case 'N':			/* possible NEW */
	if (!strcmp (criteria+1,"EW")) f = nntp_search_new;
	break;

      case 'O':			/* possible OLD, ON */
	if (!strcmp (criteria+1,"LD")) f = nntp_search_old;
	else if (!strcmp (criteria+1,"N"))
	  f = nntp_search_date (nntp_search_on,&n);
	break;
      case 'R':			/* possible RECENT */
	if (!strcmp (criteria+1,"ECENT")) f = nntp_search_recent;
	break;
      case 'S':			/* possible SEEN, SINCE, SUBJECT */
	if (!strcmp (criteria+1,"EEN")) f = nntp_search_seen;
	else if (!strcmp (criteria+1,"INCE"))
	  f = nntp_search_date (nntp_search_since,&n);
	else if (!strcmp (criteria+1,"UBJECT"))
	  f = nntp_search_string (nntp_search_subject,&d,&n);
	break;
      case 'T':			/* possible TEXT, TO */
	if (!strcmp (criteria+1,"EXT"))
	  f = nntp_search_string (nntp_search_text,&d,&n);
	else if (!strcmp (criteria+1,"O"))
	  f = nntp_search_string (nntp_search_to,&d,&n);
	break;
      case 'U':			/* possible UN* */
	if (criteria[1] == 'N') {
	  if (!strcmp (criteria+2,"ANSWERED")) f = nntp_search_unanswered;
	  else if (!strcmp (criteria+2,"DELETED")) f = nntp_search_undeleted;
	  else if (!strcmp (criteria+2,"FLAGGED")) f = nntp_search_unflagged;
	  else if (!strcmp (criteria+2,"KEYWORD"))
	    f = nntp_search_flag (nntp_search_unkeyword,&d);
	  else if (!strcmp (criteria+2,"SEEN")) f = nntp_search_unseen;
	}
	break;
      default:			/* we will barf below */
	break;
      }
      if (!f) {			/* if can't determine any criteria */
	sprintf (LOCAL->buf,"Unknown search criterion: %.80s",criteria);
	mm_log (LOCAL->buf,ERROR);
	return;
      }
				/* run the search criterion */
      for (i = 1; i <= stream->nmsgs; ++i)
	if (mail_elt (stream,i)->searched && !(*f) (stream,i,d,n))
	  mail_elt (stream,i)->searched = NIL;
    }
				/* report search results to main program */
    for (i = 1; i <= stream->nmsgs; ++i)
      if (mail_elt (stream,i)->searched) mail_searched (stream,i);
  }
}

/* NNTP mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 */

long nntp_ping (stream)
	MAILSTREAM *stream;
{
  /* Kludge alert: SMTPSOFTFATAL is 421 which is used in NNTP to mean ``No
   * next article in this group''.  Hopefully, no NNTP server will send this
   * in response to a STAT */
  return (nntp_send (LOCAL->nntpstream,"STAT",NIL) != SMTPSOFTFATAL);
}


/* NNTP mail check mailbox
 * Accepts: MAIL stream
 */

void nntp_check (stream)
	MAILSTREAM *stream;
{
				/* never do if no updates */
  if (LOCAL->dirty) newsrc_write (LOCAL->name,stream,LOCAL->number);
  LOCAL->dirty = NIL;
}

/* NNTP mail expunge mailbox
 * Accepts: MAIL stream
 */

void nntp_expunge (stream)
	MAILSTREAM *stream;
{
  if (!stream->silent) mm_log ("Expunge ignored on readonly mailbox",NIL);
}


/* NNTP mail copy message(s)
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if copy successful, else NIL
 */

long nntp_copy (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  mm_log ("Copy not valid for NNTP",ERROR);
  return NIL;
}


/* NNTP mail move message(s)
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if move successful, else NIL
 */

long nntp_move (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  mm_log ("Move not valid for NNTP",ERROR);
  return NIL;
}


/* NNTP mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long nntp_append (stream,mailbox,flags,date,message)
	MAILSTREAM *stream;
	char *mailbox;
	char *flags;
	char *date;
	 		  STRING *message;
{
  mm_log ("Append not valid for NNTP",ERROR);
  return NIL;
}

/* NNTP garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void nntp_gc (stream,gcflags)
	MAILSTREAM *stream;
	long gcflags;
{
  unsigned long i;
  if (!stream->halfopen) 	/* never on half-open stream */
    if (gcflags & GC_TEXTS)	/* garbage collect texts? */
				/* flush texts from cache */
      for (i = 0; i < stream->nmsgs; i++) {
	if (LOCAL->header[i]) fs_give ((void **) &LOCAL->header[i]);
	if (LOCAL->body[i]) fs_give ((void **) &LOCAL->body[i]);
      }
}

/* Internal routines */


/* Parse flag list
 * Accepts: MAIL stream
 *	    flag list as a character string
 * Returns: flag command list
 */

short nntp_getflags (stream,flag)
	MAILSTREAM *stream;
	char *flag;
{
  char *t,tmp[MAILTMPLEN],err[MAILTMPLEN];
  short f = 0;
  short i,j;
  if (flag && *flag) {		/* no-op if no flag string */
				/* check if a list and make sure valid */
    if ((i = (*flag == '(')) ^ (flag[strlen (flag)-1] == ')')) {
      mm_log ("Bad flag list",ERROR);
      return NIL;
    }
				/* copy the flag string w/o list construct */
    strncpy (tmp,flag+i,(j = strlen (flag) - (2*i)));
    tmp[j] = '\0';
    t = ucase (tmp);		/* uppercase only from now on */

    while (t && *t) {		/* parse the flags */
      if (*t == '\\') {		/* system flag? */
	switch (*++t) {		/* dispatch based on first character */
	case 'S':		/* possible \Seen flag */
	  if (t[1] == 'E' && t[2] == 'E' && t[3] == 'N') i = fSEEN;
	  t += 4;		/* skip past flag name */
	  break;
	case 'D':		/* possible \Deleted flag */
	  if (t[1] == 'E' && t[2] == 'L' && t[3] == 'E' && t[4] == 'T' &&
	      t[5] == 'E' && t[6] == 'D') i = fDELETED;
	  t += 7;		/* skip past flag name */
	  break;
	case 'F':		/* possible \Flagged flag */
	  if (t[1] == 'L' && t[2] == 'A' && t[3] == 'G' && t[4] == 'G' &&
	      t[5] == 'E' && t[6] == 'D') i = fFLAGGED;
	  t += 7;		/* skip past flag name */
	  break;
	case 'A':		/* possible \Answered flag */
	  if (t[1] == 'N' && t[2] == 'S' && t[3] == 'W' && t[4] == 'E' &&
	      t[5] == 'R' && t[6] == 'E' && t[7] == 'D') i = fANSWERED;
	  t += 8;		/* skip past flag name */
	  break;
	default:		/* unknown */
	  i = 0;
	  break;
	}
				/* add flag to flags list */
	if (i && ((*t == '\0') || (*t++ == ' '))) f |= i;
      }
      else {			/* no user flags yet */
	t = strtok (t," ");	/* isolate flag name */
	sprintf (err,"Unknown flag: %.80s",t);
	t = strtok (NIL," ");	/* get next flag */
	mm_log (err,ERROR);
      }
    }
  }
  return f;
}

/* Search support routines
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to additional data
 *	    pointer to temporary buffer
 * Returns: T if search matches, else NIL
 */

char nntp_search_all (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return T;			/* ALL always succeeds */
}


char nntp_search_answered (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->answered ? T : NIL;
}


char nntp_search_deleted (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->deleted ? T : NIL;
}


char nntp_search_flagged (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->flagged ? T : NIL;
}


char nntp_search_keyword (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return NIL;			/* keywords not supported yet */
}


char nntp_search_new (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (elt->recent && !elt->seen) ? T : NIL;
}

char nntp_search_old (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->recent ? NIL : T;
}


char nntp_search_recent (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->recent ? T : NIL;
}


char nntp_search_seen (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->seen ? T : NIL;
}


char nntp_search_unanswered (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->answered ? NIL : T;
}


char nntp_search_undeleted (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->deleted ? NIL : T;
}


char nntp_search_unflagged (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->flagged ? NIL : T;
}


char nntp_search_unkeyword (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return T;			/* keywords not supported yet */
}


char nntp_search_unseen (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->seen ? NIL : T;
}

char nntp_search_before (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return (char) (nntp_msgdate (stream,msgno) < n);
}


char nntp_search_on (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return (char) (nntp_msgdate (stream,msgno) == n);
}


char nntp_search_since (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
				/* everybody interprets "since" as .GE. */
  return (char) (nntp_msgdate (stream,msgno) >= n);
}


unsigned long nntp_msgdate (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
				/* get date if don't have it yet */
  if (!elt->day) nntp_fetchstructure (stream,msgno,NIL);
  return (long) (elt->year << 9) + (elt->month << 5) + elt->day;
}


char nntp_search_body (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char *t = nntp_fetchtext_work (stream,msgno);
  return (t && search (t,(unsigned long) strlen (t),d,n));
}


char nntp_search_subject (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char *t = nntp_fetchstructure (stream,msgno,NIL)->subject;
  return t ? search (t,strlen (t),d,n) : NIL;
}


char nntp_search_text (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char *t = nntp_fetchheader (stream,msgno);
  return (t && search (t,strlen (t),d,n)) ||
    nntp_search_body (stream,msgno,d,n);
}

char nntp_search_bcc (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  ADDRESS *a = nntp_fetchstructure (stream,msgno,NIL)->bcc;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char nntp_search_cc (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  ADDRESS *a = nntp_fetchstructure (stream,msgno,NIL)->cc;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char nntp_search_from (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  ADDRESS *a = nntp_fetchstructure (stream,msgno,NIL)->from;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char nntp_search_to (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  ADDRESS *a = nntp_fetchstructure (stream,msgno,NIL)->to;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}

/* Search parsers */


/* Parse a date
 * Accepts: function to return
 *	    pointer to date integer to return
 * Returns: function to return
 */

search_t nntp_search_date (f,n)
	search_t f;
	long *n;
{
  long i;
  char *s;
  MESSAGECACHE elt;
				/* parse the date and return fn if OK */
  return (nntp_search_string (f,&s,&i) && mail_parse_date (&elt,s) &&
	  (*n = (elt.year << 9) + (elt.month << 5) + elt.day)) ? f : NIL;
}

/* Parse a flag
 * Accepts: function to return
 *	    pointer to string to return
 * Returns: function to return
 */

search_t nntp_search_flag (f,d)
	search_t f;
	char **d;
{
				/* get a keyword, return if OK */
  return (*d = strtok (NIL," ")) ? f : NIL;
}


/* Parse a string
 * Accepts: function to return
 *	    pointer to string to return
 *	    pointer to string length to return
 * Returns: function to return
 */

search_t nntp_search_string (f,d,n)
	search_t f;
	char **d;
	long *n;
{
  char *end = " ";
  char *c = strtok (NIL,"");	/* remainder of criteria */
  if (!c) return NIL;		/* missing argument */
  switch (*c) {			/* see what the argument is */
  case '{':			/* literal string */
    *n = strtol (c+1,d,10);	/* get its length */
    if ((*(*d)++ == '}') && (*(*d)++ == '\015') && (*(*d)++ == '\012') &&
	(!(*(c = *d + *n)) || (*c == ' '))) {
      char e = *--c;
      *c = DELIM;		/* make sure not a space */
      strtok (c," ");		/* reset the strtok mechanism */
      *c = e;			/* put character back */
      break;
    }
  case '\0':			/* catch bogons */
  case ' ':
    return NIL;
  case '"':			/* quoted string */
    if (strchr (c+1,'"')) end = "\"";
    else return NIL;
  default:			/* atomic string */
    if (*d = strtok (c,end)) *n = strlen (*d);
    else return NIL;
    break;
  }
  return f;
}
