/*
 * Program:	Mailbox Access routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	22 November 1989
 * Last Edited:	15 October 1996
 *
 * Copyright 1996 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made
 * available "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */


#include <ctype.h>
#include <stdio.h>
#include "mail.h"
#include "osdep.h"
#include <time.h>
#include "misc.h"
#include "rfc822.h"


/* c-client global data */

				/* list of mail drivers */
static DRIVER *maildrivers = NIL;
				/* pointer to alternate gets function */
static mailgets_t mailgets = NIL;
				/* mail cache manipulation function */
static mailcache_t mailcache = mm_cache;
				/* place to stash last user name */
static char *mailusernamebuf = NIL;
				/* pointer to alternate rfc822 generator */
static rfc822emit_t rfc822_emit = rfc822_output;
				/* pointer to SMTP verbose output collector */
static postverbose_t post_verbose = NIL;
				/* POST send string as record */
static postsoutr_t post_soutr = tcp_soutr;
				/* POST receive line */
static postgetline_t post_getline = tcp_getline;
				/* POST close connection */
static postclose_t post_close = tcp_close;

/* Default limited get string
 * Accepts: readin function pointer
 *	    stream to use
 *	    number of bytes
 * Returns: string read in, truncated if necessary
 *
 * This is a sample mailgets routine.  It simply truncates any data larger
 * than MAXMESSAGESIZE.  On most systems, you generally don't use a mailgets
 * routine at all, but on some systems (e.g. DOS) it's required to prevent the
 * application from crashing.  This one is filled in by the os driver for any
 * OS that requires a mailgets routine and the main program has not already
 * supplied one, generally in tcp_open().
 */

char *mm_gets (f,stream,size)
	readfn_t f;
	void *stream;
	unsigned long size;
{
  char *s;
  char tmp[MAILTMPLEN+1];
  unsigned long i,j = 0;
				/* truncate? */
  if (i = (size > MAXMESSAGESIZE)) {
    sprintf (tmp,"%ld character literal truncated to %ld characters",
	     size,MAXMESSAGESIZE);
    mm_log (tmp,WARN);		/* warn user */
    i = size - MAXMESSAGESIZE;	/* number of bytes of slop */
    size = MAXMESSAGESIZE;	/* maximum length string we can read */
  }
  s = (char *) fs_get (size + 1);
  *s = s[size] = '\0';		/* init in case getbuffer fails */
  (*f) (stream,size,s);		/* get the literal */
				/* toss out everything after that */
  while (i -= j) (*f) (stream,j = min ((long) MAILTMPLEN,i),tmp);
  return s;
}

/* Default mail cache handler
 * Accepts: pointer to cache handle
 *	    message number
 *	    caching function
 * Returns: cache data
 */

void *mm_cache (stream,msgno,op)
	MAILSTREAM *stream;
	long msgno;
	long op;
{
  size_t new;
  void *ret = NIL;
  long i = msgno - 1;
  unsigned long j = stream->cachesize;
  switch ((int) op) {		/* what function? */
  case CH_INIT:			/* initialize cache */
    if (stream->cachesize) {	/* flush old cache contents */
      while (stream->cachesize) mm_cache (stream,stream->cachesize--,CH_FREE);
      fs_give ((void **) &stream->cache.c);
      stream->nmsgs = 0;	/* can't have any messages now */
    }
    break;
  case CH_SIZE:			/* (re-)size the cache */
    if (msgno > j) {		/* do nothing if size adequate */
      new = (stream->cachesize = msgno + CACHEINCREMENT) * sizeof (void *);
      if (stream->cache.c) fs_resize ((void **) &stream->cache.c,new);
      else stream->cache.c = (void **) fs_get (new);
				/* init cache */
      while (j < stream->cachesize) stream->cache.c[j++] = NIL;
    }
    break;
  case CH_MAKELELT:		/* return long elt, make if necessary */
    if (!stream->cache.c[i]) {	/* have one already? */
				/* no, instantiate it */
      stream->cache.l[i] = (LONGCACHE *) fs_get (sizeof (LONGCACHE));
      memset (&stream->cache.l[i]->elt,0,sizeof (MESSAGECACHE));
      stream->cache.l[i]->elt.lockcount = 1;
      stream->cache.l[i]->elt.msgno = msgno;
      stream->cache.l[i]->env = NIL;
      stream->cache.l[i]->body = NIL;
    }
				/* drop in to CH_LELT */
  case CH_LELT:			/* return long elt */
    ret = stream->cache.c[i];	/* return void version */
    break;

  case CH_MAKEELT:		/* return short elt, make if necessary */
    if (!stream->cache.c[i]) {	/* have one already? */
      if (stream->scache) {	/* short cache? */
	stream->cache.s[i] = (MESSAGECACHE *) fs_get (sizeof(MESSAGECACHE));
	memset (stream->cache.s[i],0,sizeof (MESSAGECACHE));
	stream->cache.s[i]->lockcount = 1;
	stream->cache.s[i]->msgno = msgno;
      }
      else mm_cache (stream,msgno,CH_MAKELELT);
    }
				/* drop in to CH_ELT */
  case CH_ELT:			/* return short elt */
    ret = stream->cache.c[i] && !stream->scache ?
      (void *) &stream->cache.l[i]->elt : stream->cache.c[i];
    break;
  case CH_FREE:			/* free (l)elt */
    if (stream->scache) mail_free_elt (&stream->cache.s[i]);
    else mail_free_lelt (&stream->cache.l[i]);
    break;
  case CH_EXPUNGE:		/* expunge cache slot */
				/* slide down remainder of cache */
    for (i = msgno; i < stream->nmsgs; ++i)
      if (stream->cache.c[i-1] = stream->cache.c[i])
	((MESSAGECACHE *) mm_cache (stream,i,CH_ELT))->msgno = i;
    stream->cache.c[stream->nmsgs-1] = NIL;
    break;
  default:
    fatal ("Bad mm_cache op");
    break;
  }
  return ret;
}

/* Dummy string driver for complete in-memory strings */

STRINGDRIVER mail_string = {
  mail_string_init,		/* initialize string structure */
  mail_string_next,		/* get next byte in string structure */
  mail_string_setpos		/* set position in string structure */
};


/* Initialize mail string structure for in-memory string
 * Accepts: string structure
 *	    pointer to string
 *	    size of string
 */

void mail_string_init (s,data,size)
	STRING *s;
	void *data;
	unsigned long size;
{
				/* set initial string pointers */
  s->chunk = s->curpos = (char *) (s->data = data);
				/* and sizes */
  s->size = s->chunksize = s->cursize = size;
  s->data1 = s->offset = 0;	/* never any offset */
}

/* Get next character from string
 * Accepts: string structure
 * Returns: character, string structure chunk refreshed
 */

char mail_string_next (s)
	STRING *s;
{
  return *s->curpos++;		/* return the last byte */
}


/* Set string pointer position
 * Accepts: string structure
 *	    new position
 */

void mail_string_setpos (s,i)
	STRING *s;
	unsigned long i;
{
  s->curpos = s->chunk + i;	/* set new position */
  s->cursize = s->chunksize - i;/* and new size */
}

/* Mail routines
 *
 *  mail_xxx routines are the interface between this module and the outside
 * world.  Only these routines should be referenced by external callers.
 *
 *  Note that there is an important difference between a "sequence" and a
 * "message #" (msgno).  A sequence is a string representing a sequence in
 * {"n", "n:m", or combination separated by commas} format, whereas a msgno
 * is a single integer.
 *
 */

/* Mail link driver
 * Accepts: driver to add to list
 */

void mail_link (driver)
	DRIVER *driver;
{
  DRIVER **d = &maildrivers;
  while (*d) d = &(*d)->next;	/* find end of list of drivers */
  *d = driver;			/* put driver at the end */
  driver->next = NIL;		/* this driver is the end of the list */
}

/* Mail manipulate driver parameters
 * Accepts: mail stream
 *	    function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *mail_parameters (stream,function,value)
	MAILSTREAM *stream;
	long function;
	void *value;
{
  void *r,*ret = NIL;
  DRIVER *d;
  switch ((int) function) {
  case SET_DRIVERS:
    fatal ("SET_DRIVERS not permitted");
  case GET_DRIVERS:
    ret = (void *) maildrivers;
    break;
  case SET_GETS:
    mailgets = (mailgets_t) value;
  case GET_GETS:
    ret = (void *) mailgets;
    break;
  case SET_CACHE:
    mailcache = (mailcache_t) value;
  case GET_CACHE:
    ret = (void *) mailcache;
    break;
  case SET_USERNAMEBUF:
    mailusernamebuf = (char *) value;
    /* BUG: missing break?  anyone care? */
  case GET_USERNAMEBUF:
    ret = (void *) mailusernamebuf;
    break;
  case SET_RFC822OUTPUT:
    rfc822_emit = (rfc822emit_t) value;
    break;
  case GET_RFC822OUTPUT:
    ret = (void *) rfc822_emit;
    break;
  case SET_POSTVERBOSE:
    post_verbose = (postverbose_t) value;
    break;
  case GET_POSTVERBOSE:
    ret = (void *) post_verbose;
    break;
  case SET_POSTSOUTR:
    post_soutr = (postsoutr_t) value;
    break;
  case GET_POSTSOUTR:
    ret = (void *) post_soutr;
    break;
  case SET_POSTGETLINE:
    post_getline = (postgetline_t) value;
    break;
  case GET_POSTGETLINE:
    ret = (void *) post_getline;
    break;
  case SET_POSTCLOSE:
    post_close = (postclose_t) value;
    break;
  case GET_POSTCLOSE:
    ret = (void *) post_close;
    break;
  default:
    if (stream && stream->dtb)	/* if have stream, do for that stream only */
      ret = (*stream->dtb->parameters) (function,value);
				/* else do all drivers */
    else for (d = maildrivers; d; d = d->next)
      if (r = (d->parameters) (function,value)) ret = r;
				/* do environment */
    if (r = env_parameters (function,value)) ret = r;
				/* do TCP/IP */
    if (r = tcp_parameters (function,value)) ret = r;
    break;
  }
  return ret;
}

/* Mail find list of subscribed mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void mail_find (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  DRIVER *d = maildrivers;
				/* if have a stream, do it for that stream */
  if (stream && stream->dtb) (*stream->dtb->find) (stream,pat);
  else do (d->find) (NIL,pat);	/* otherwise do for all DTB's */
  while (d = d->next);		/* until at the end */
}


/* Mail find list of subscribed bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void mail_find_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  DRIVER *d = maildrivers;
  if (stream && stream->dtb) (*stream->dtb->find_bboard) (stream,pat);
  else do (d->find_bboard) (NIL,pat);
  while (d = d->next);		/* until at the end */
}

/* Mail find list of all mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void mail_find_all (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  DRIVER *d = maildrivers;
				/* if have a stream, do it for that stream */
  if (stream && stream->dtb) (*stream->dtb->find_all) (stream,pat);
				/* otherwise do for all DTB's */
  else do (d->find_all) (NIL,pat);
  while (d = d->next);		/* until at the end */
}


/* Mail find list of all bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void mail_find_all_bboard (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  DRIVER *d = maildrivers;
				/* if have a stream, do it for that stream */
  if (stream && stream->dtb) (*stream->dtb->find_all_bboard) (stream,pat);
				/* otherwise do for all DTB's */
  else do (d->find_all_bboard) (NIL,pat);
  while (d = d->next);		/* until at the end */
}

/* Mail validate mailbox name
 * Accepts: MAIL stream
 *	    mailbox name
 *	    purpose string for error message
 * Return: driver factory on success, NIL on failure
 */

DRIVER *mail_valid (stream,mailbox,purpose)
	MAILSTREAM *stream;
	char *mailbox;
	char *purpose;
{
  char *s,tmp[MAILTMPLEN];
  DRIVER *factory;
  for (factory = maildrivers; factory && !(*factory->valid) (mailbox);
       factory = factory->next);
				/* must match stream if not dummy */
  if (factory && stream && (stream->dtb != factory))
    factory = strcmp (factory->name,"dummy") ? NIL : stream->dtb;
  if (!factory && purpose) {	/* if want an error message */
    switch (*mailbox) {		/* error depends upon first character */
    case '*':			/* bboard */
      if (mailbox[1] != '{') {	/* but only if local */
	s = "no such bboard";
	break;
      }
				/* otherwise drop into remote */
    case '{':			/* remote specification */
      s = "invalid remote specification";
      break;
    default:			/* others */
      s = "no such mailbox";
      break;
    }
    sprintf (tmp,"Can't %s %s: %s",purpose,mailbox,s);
    mm_log (tmp,ERROR);
  }
  return factory;		/* return driver factory */
}

/* Mail validate network mailbox name
 * Accepts: mailbox name
 *	    mailbox driver to validate against
 *	    pointer to where to return host name if non-NIL
 *	    pointer to where to return mailbox name if non-NIL
 * Returns: driver on success, NIL on failure
 */

DRIVER *mail_valid_net (name,drv,host,mailbox)
	char *name;
	DRIVER *drv;
	char *host;
	char *mailbox;
{
  NETMBX mb;
  if (!mail_valid_net_parse (name,&mb) ||
      (strcmp (mb.service,"imap") ? strcmp (mb.service,drv->name) :
       strncmp (drv->name,"imap",4))) return NIL;
  if (host) strcpy (host,mb.host);
  if (mailbox) strcpy (mailbox,mb.mailbox);
  return drv;
}


/* Mail validate network mailbox name
 * Accepts: mailbox name
 *	    NETMBX structure to return values
 * Returns: T on success, NIL on failure
 */

long mail_valid_net_parse (name,mb)
	char *name;
	NETMBX *mb;
{
  long i;
  char c,*s,*t,*v;
  mb->port = 0;			/* initialize structure */
  *mb->host = *mb->mailbox = *mb->service = '\0';
				/* init flags */
  mb->anoflag = mb->dbgflag = NIL;
  if (mailusernamebuf) *mailusernamebuf = '\0';
				/* check if bboard */
  if (mb->bbdflag = (*name == '*') ? T : NIL) name++;
				/* have host specification? */
  if (!(*name == '{' && (t = strchr (s = name+1,'}')) && (i = t - s)))
    return NIL;			/* not valid host specification */
  strncpy (mb->host,s,i);	/* set host name */
  mb->host[i] = '\0';		/* tie it off */
  strcpy (mb->mailbox,t+1);	/* set mailbox name */
				/* any switches or port specification? */
  if (t = strpbrk (mb->host,"/:")) {
    c = *t;			/* yes, remember delimiter */
    *t++ = '\0';		/* tie off host name */
    lcase (t);			/* coerce remaining stuff to lowercase */
    do switch (c) {		/* act based upon the character */
    case ':':			/* port specification */
      if (mb->port || ((mb->port = strtol (t,&t,10)) <= 0)) return NIL;
      c = t ? *t++ : '\0';	/* get delimiter, advance pointer */
      break;

    case '/':			/* switch */
				/* find delimiter */
      if (t = strpbrk (s = t,"/:=")) {
	c = *t;			/* remember delimiter for later */
	*t++ = '\0';		/* tie off switch name */
      }
      else c = '\0';		/* no delimiter */
      if (c == '=') {		/* parse switches which take arguments */
	if (t = strpbrk (v = t,"/:")) {
	  c = *t;		/* remember delimiter for later */
	  *t++ = '\0';		/* tie off switch name */
	}
	else c = '\0';		/* no delimiter */
	if (!strcmp (s,"service")) {
	  if (*mb->service) return NIL;
	  else strcpy (mb->service,v);
	}
	if (!strcmp (s,"user") && mailusernamebuf) {
	  if (*mailusernamebuf) return NIL;
	  else strcpy (mailusernamebuf,v);
	}
	else return NIL;	/* invalid argument switch */
      }
      else {			/* non-argument switch */
	if (!strcmp (s,"anonymous")) mb->anoflag = T;
	else if (!strcmp (s,"bboard")) mb->bbdflag = T;
	else if (!strcmp (s,"debug")) mb->dbgflag = T;
	else if (!strcmp (s,"imap") || !strcmp (s,"imap2") ||
		 !strcmp (s,"imap4") || !strcmp (s,"pop3") ||
		 !strcmp (s,"nntp")) {
	  if (*mb->service) return NIL;
	  else strcpy (mb->service,s);
	}
	else return NIL;	/* invalid non-argument switch */
      }
      break;
    default:			/* anything else is bogus */
      return NIL;
    } while (c);		/* see if anything more to parse */
  }
				/* default mailbox name */
  if (!*mb->mailbox) strcpy (mb->mailbox,mb->bbdflag ? "general" : "INBOX");
				/* default service name */
  if (!*mb->service) strcpy (mb->service,"imap");
  return T;
}

/* Mail subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long mail_subscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  DRIVER *factory = mail_valid (stream,mailbox,"subscribe to mailbox");
  return factory ? (*factory->subscribe) (stream,mailbox) : NIL;
}


/* Mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long mail_unsubscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
 DRIVER *factory = mail_valid (stream,mailbox,"unsubscribe to mailbox");
  return factory ? (*factory->unsubscribe) (stream,mailbox) : NIL;
}


/* Mail subscribe to bboard
 * Accepts: mail stream
 *	    bboard to add to subscription list
 * Returns: T on success, NIL on failure
 */

long mail_subscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  char tmp[MAILTMPLEN];
  DRIVER *factory;
  sprintf (tmp,"*%s",mailbox);
  return (factory = mail_valid (stream,tmp,"subscribe to bboard")) ?
    (*factory->subscribe_bboard) (stream,mailbox) : NIL;
}


/* Mail unsubscribe to bboard
 * Accepts: mail stream
 *	    bboard to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long mail_unsubscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  char tmp[MAILTMPLEN];
  DRIVER *factory;
  sprintf (tmp,"*%s",mailbox);
  return (factory = mail_valid (stream,tmp,"unsubscribe to bboard")) ?
    (*factory->unsubscribe_bboard) (stream,mailbox) : NIL;
}

/* Mail create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long mail_create (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  /* A local mailbox is one that is not qualified as being a remote or a
   * namespace mailbox.  Any remote or namespace mailbox driver must check
   * for itself whether or not the mailbox already exists. */
  int localp = (*mailbox != '{') && (*mailbox != '#');
  char tmp[MAILTMPLEN];
				/* guess at driver if stream not specified */
  if (!(stream || (stream = localp ? default_proto () :
		   mail_open (NIL,mailbox,OP_PROTOTYPE)))) {
    sprintf (tmp,"Can't create mailbox %s: indeterminate format",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* must not already exist if local */
  if (localp && mail_valid (stream,mailbox,NIL)) {
    sprintf (tmp,"Can't create mailbox %s: mailbox already exists",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
  return stream->dtb ? (*stream->dtb->create) (stream,mailbox) : NIL;
}

/* Mail delete mailbox
 * Accepts: mail stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long mail_delete (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  DRIVER *factory = mail_valid (stream,mailbox,"delete mailbox");
  return factory ? (*factory->mbxdel) (stream,mailbox) : NIL;
}


/* Mail rename mailbox
 * Accepts: mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long mail_rename (stream,old,new)
	MAILSTREAM *stream;
	char *old;
	char *new;
{
  char tmp[MAILTMPLEN];
  DRIVER *factory = mail_valid (stream,old,"rename mailbox");
  if ((*old != '{') && (*old != '#') && mail_valid (NIL,new,NIL)) {
    sprintf (tmp,"Can't rename to mailbox %s: mailbox already exists",new);
    mm_log (tmp,ERROR);
    return NIL;
  }
  return factory ? (*factory->mbxren) (stream,old,new) : NIL;
}

/* Mail open
 * Accepts: candidate stream for recycling
 *	    mailbox name
 *	    open options
 * Returns: stream to use on success, NIL on failure
 */

MAILSTREAM *mail_open (stream,name,options)
	MAILSTREAM *stream;
	char *name;
	long options;
{
  DRIVER *factory = mail_valid (NIL,name,(options & OP_SILENT) ?
				NIL : "open mailbox");
  if (factory) {		/* must have a factory */
    if (!stream) {		/* instantiate stream if none to recycle */
      if (options & OP_PROTOTYPE) return (*factory->open) (NIL);
      stream = (MAILSTREAM *) fs_get (sizeof (MAILSTREAM));
				/* initialize stream */
      memset ((void *) stream,0,sizeof (MAILSTREAM));
      stream->dtb = factory;	/* set dispatch */
				/* set mailbox name */
      stream->mailbox = cpystr (name);
				/* initialize cache */
      (*mailcache) (stream,(long) 0,CH_INIT);
    }
    else {			/* close driver if different from factory */
      if (stream->dtb != factory) {
	if (stream->dtb) (*stream->dtb->close) (stream);
	stream->dtb = factory;	/* establish factory as our driver */
	stream->local = NIL;	/* flush old driver's local data */
	mail_free_cache (stream);
      }
				/* clean up old mailbox name for recycling */
      if (stream->mailbox) fs_give ((void **) &stream->mailbox);
      stream->mailbox = cpystr (name);
    }
    stream->lock = NIL;		/* initialize lock and options */
    stream->debug = (options & OP_DEBUG) ? T : NIL;
    stream->rdonly = (options & OP_READONLY) ? T : NIL;
    stream->anonymous = (options & OP_ANONYMOUS) ? T : NIL;
    stream->scache = (options & OP_SHORTCACHE) ? T : NIL;
    stream->silent = (options & OP_SILENT) ? T : NIL;
    stream->halfopen = (options & OP_HALFOPEN) ? T : NIL;
				/* have driver open, flush if failed */
    if (!(*factory->open) (stream)) stream = mail_close (stream);
  }
  return stream;		/* return the stream */
}

/* Mail close
 * Accepts: mail stream
 * Returns: NIL
 */

MAILSTREAM *mail_close (stream)
	MAILSTREAM *stream;
{
  if (stream) {			/* make sure argument given */
				/* do the driver's close action */
    if (stream->dtb) (*stream->dtb->close) (stream);
    if (stream->mailbox) fs_give ((void **) &stream->mailbox);
    stream->sequence++;		/* invalidate sequence */
    if (stream->flagstring) fs_give ((void **) &stream->flagstring);
    mail_free_cache (stream);	/* finally free the stream's storage */
    if (!stream->use) fs_give ((void **) &stream);
  }
  return NIL;
}

/* Mail make handle
 * Accepts: mail stream
 * Returns: handle
 *
 *  Handles provide a way to have multiple pointers to a stream yet allow the
 * stream's owner to nuke it or recycle it.
 */

MAILHANDLE *mail_makehandle (stream)
	MAILSTREAM *stream;
{
  MAILHANDLE *handle = (MAILHANDLE *) fs_get (sizeof (MAILHANDLE));
  handle->stream = stream;	/* copy stream */
				/* and its sequence */
  handle->sequence = stream->sequence;
  stream->use++;		/* let stream know another handle exists */
  return handle;
}


/* Mail release handle
 * Accepts: Mail handle
 */

void mail_free_handle (handle)
	MAILHANDLE **handle;
{
  MAILSTREAM *s;
  if (*handle) {		/* only free if exists */
				/* resign stream, flush unreferenced zombies */
    if ((!--(s = (*handle)->stream)->use) && !s->dtb) fs_give ((void **) &s);
    fs_give ((void **) handle);	/* now flush the handle */
  }
}


/* Mail get stream handle
 * Accepts: Mail handle
 * Returns: mail stream or NIL if stream gone
 */

MAILSTREAM *mail_stream (handle)
	MAILHANDLE *handle;
{
  MAILSTREAM *s = handle->stream;
  return (s->dtb && (handle->sequence == s->sequence)) ? s : NIL;
}

/* Mail fetch long cache element
 * Accepts: mail stream
 *	    message # to fetch
 * Returns: long cache element of this message
 * Can also be used to create cache elements for new messages.
 */

LONGCACHE *mail_lelt (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  if (stream->scache) fatal ("Short cache in mail_lelt");
				/* be sure it the cache is large enough */
  (*mailcache) (stream,msgno,CH_SIZE);
  return (LONGCACHE *) (*mailcache) (stream,msgno,CH_MAKELELT);
}


/* Mail fetch cache element
 * Accepts: mail stream
 *	    message # to fetch
 * Returns: cache element of this message
 * Can also be used to create cache elements for new messages.
 */

MESSAGECACHE *mail_elt (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  if (msgno < 1) fatal ("Bad msgno in mail_elt");
				/* be sure it the cache is large enough */
  (*mailcache) (stream,msgno,CH_SIZE);
  return (MESSAGECACHE *) (*mailcache) (stream,msgno,CH_MAKEELT);
}

/* Mail fetch fast information
 * Accepts: mail stream
 *	    sequence
 *
 * Generally, mail_fetchstructure is preferred
 */

void mail_fetchfast (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->fetchfast) (stream,sequence);
}


/* Mail fetch flags
 * Accepts: mail stream
 *	    sequence
 */

void mail_fetchflags (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->fetchflags) (stream,sequence);
}


/* Mail fetch message structure
 * Accepts: mail stream
 *	    message # to fetch
 *	    pointer to return body
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *mail_fetchstructure (stream,msgno,body)
	MAILSTREAM *stream;
	long msgno;
	BODY **body;
{
  if (msgno < 1 || msgno > stream->nmsgs)
    fatal ("Bad msgno in mail_fetchstructure");
  				/* do the driver's action */
  return stream->dtb ? (*stream->dtb->fetchstructure) (stream,msgno,body) :NIL;
}

/* Mail fetch message header
 * Accepts: mail stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *mail_fetchheader (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  if (msgno < 1 || msgno > stream->nmsgs)
    fatal ("Bad msgno in mail_fetchheader");
  				/* do the driver's action */
  return stream->dtb ? (*stream->dtb->fetchheader) (stream,msgno) : "";
}


/* Mail fetch message text (only)
	body only;
 * Accepts: mail stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *mail_fetchtext (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  if (msgno < 1 || msgno > stream->nmsgs)
    fatal ("Bad msgno in mail_fetchtext");
  				/* do the driver's action */
  return stream->dtb ? (*stream->dtb->fetchtext) (stream,msgno) : "";
}


/* Mail fetch message body part text
 * Accepts: mail stream
 *	    message # to fetch
 *	    section specifier (#.#.#...#)
 *	    pointer to returned length
 * Returns: pointer to section of message body
 */

char *mail_fetchbody (stream,m,sec,len)
	MAILSTREAM *stream;
	long m;
	char *sec;
	unsigned long *len;
{
  if (m < 1 || m > stream->nmsgs) fatal ("Bad msgno in mail_fetchbody");
  				/* do the driver's action */
  return stream->dtb ? (*stream->dtb->fetchbody) (stream,m,sec,len) : "";
}

/* Mail fetch From string for menu
 * Accepts: destination string
 *	    mail stream
 *	    message # to fetch
 *	    desired string length
 * Returns: string of requested length
 */

void mail_fetchfrom (s,stream,msgno,length)
	char *s;
	MAILSTREAM *stream;
	long msgno;
	long length;
{
  char *t;
  char tmp[MAILTMPLEN];
  ENVELOPE *env = mail_fetchstructure (stream,msgno,NIL);
  ADDRESS *adr = env ? env->from : NIL;
  memset (s,' ',length);	/* fill it with spaces */
  s[length] = '\0';		/* tie off with null */
				/* get first from address from envelope */
  while (adr && !adr->host) adr = adr->next;
  if (adr) {			/* if a personal name exists use it */
    if (!(t = adr->personal)) sprintf (t = tmp,"%s@%s",adr->mailbox,adr->host);
    memcpy (s,t,min (length,(long) strlen (t)));
  }
}


/* Mail fetch Subject string for menu
 * Accepts: destination string
 *	    mail stream
 *	    message # to fetch
 *	    desired string length
 * Returns: string of no more than requested length
 */

void mail_fetchsubject (s,stream,msgno,length)
	char *s;
	MAILSTREAM *stream;
	long msgno;
	long length;
{
  ENVELOPE *env = mail_fetchstructure (stream,msgno,NIL);
  memset (s,'\0',length+1);
				/* copy subject from envelope */
  if (env && env->subject) strncpy (s,env->subject,length);
  else *s = ' ';		/* if no subject then just a space */
}

/* Mail set flag
 * Accepts: mail stream
 *	    sequence
 *	    flag(s)
 */

void mail_setflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->setflag) (stream,sequence,flag);
}


/* Mail clear flag
 * Accepts: mail stream
 *	    sequence
 *	    flag(s)
 */

void mail_clearflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->clearflag) (stream,sequence,flag);
}


/* Mail search for messages
 * Accepts: mail stream
 *	    search criteria
 */

void mail_search (stream,criteria)
	MAILSTREAM *stream;
	char *criteria;
{
  long i = 1;
  while (i <= stream->nmsgs) mail_elt (stream,i++)->searched = NIL;
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->search) (stream,criteria);
}


/* Mail ping mailbox
 * Accepts: mail stream
 * Returns: stream if still open else NIL
 */

long mail_ping (stream)
	MAILSTREAM *stream;
{
  				/* do the driver's action */
  return stream->dtb ? (*stream->dtb->ping) (stream) : NIL;
}

/* Mail check mailbox
 * Accepts: mail stream
 */

void mail_check (stream)
	MAILSTREAM *stream;
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->check) (stream);
}


/* Mail expunge mailbox
 * Accepts: mail stream
 */

void mail_expunge (stream)
	MAILSTREAM *stream;
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->expunge) (stream);
}

/* Mail copy message(s)
	s;
 * Accepts: mail stream
 *	    sequence
 *	    destination mailbox
 */

long mail_copy (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  				/* do the driver's action */
  return stream->dtb ? (*stream->dtb->copy) (stream,sequence,mailbox) : NIL;
}


/* Mail move message(s)
	s;
 * Accepts: mail stream
 *	    sequence
 *	    destination mailbox
 */

long mail_move (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  				/* do the driver's action */
  return stream->dtb ? (*stream->dtb->move) (stream,sequence,mailbox) : NIL;
}


/* Mail append message string
 * Accepts: mail stream
 *	    destination mailbox
 *	    initial flags
 *	    message internal date
 *	    stringstruct of message to append
 * Returns: T on success, NIL on failure
 */

long mail_append (stream,mailbox,message)
	MAILSTREAM *stream;
	char *mailbox;
	STRING *message;
{
				/* compatibility jacket */
  return mail_append_full (stream,mailbox,NIL,NIL,message);
}


long mail_append_full (stream,mailbox,flags,date,message)
	MAILSTREAM *stream;
	char *mailbox;
	char *flags;
	char *date;
	 		       STRING *message;
{
  DRIVER *factory = mail_valid (stream,mailbox,NIL);
  if (!factory) {		/* got a driver to use? */
    if (!stream &&		/* ask default for TRYCREATE if no stream */
	(*default_proto ()->dtb->append) (stream,mailbox,flags,date,message)) {
				/* timing race?? */
      mm_notify (stream,"Append validity confusion",WARN);
      return T;
    }
				/* now generate error message */
    mail_valid (stream,mailbox,"append to mailbox");
    return NIL;			/* return failure */
  }
				/* do the driver's action */
  return (factory->append) (stream,mailbox,flags,date,message);
}

/* Mail garbage collect stream
 * Accepts: mail stream
 *	    garbage collection flags
 */

void mail_gc (stream,gcflags)
	MAILSTREAM *stream;
	long gcflags;
{
  unsigned long i = 1;
  LONGCACHE *lelt;
  				/* do the driver's action first */
  if (stream->dtb) (*stream->dtb->gc) (stream,gcflags);
  if (gcflags & GC_ENV) {	/* garbage collect envelopes? */
				/* yes, free long cache if in use */
    if (!stream->scache) while (i <= stream->nmsgs)
      if (lelt = (LONGCACHE *) (*mailcache) (stream,i++,CH_LELT)) {
	mail_free_envelope (&lelt->env);
	mail_free_body (&lelt->body);
      }
    stream->msgno = 0;		/* free this cruft too */
    mail_free_envelope (&stream->env);
    mail_free_body (&stream->body);
  }
				/* free text if any */
  if ((gcflags & GC_TEXTS) && (stream->text)) fs_give ((void **)&stream->text);
}

/* Mail output date from elt fields
 * Accepts: character string to write into
 *	    elt to get data data from
 * Returns: the character string
 */

const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

char *mail_date (string,elt)
	char *string;
	MESSAGECACHE *elt;
{
  const char *s = (elt->month && elt->month < 13) ?
    months[elt->month - 1] : (const char *) "???";
  sprintf (string,"%2d-%s-%d %02d:%02d:%02d %c%02d%02d",
	   elt->day,s,elt->year + BASEYEAR,
	   elt->hours,elt->minutes,elt->seconds,
	   elt->zoccident ? '-' : '+',elt->zhours,elt->zminutes);
  return string;
}


/* Mail output cdate format date from elt fields
 * Accepts: character string to write into
 *	    elt to get data data from
 * Returns: the character string
 */

char *mail_cdate (string,elt)
	char *string;
	MESSAGECACHE *elt;
{
  const char *s = (elt->month && elt->month < 13) ?
    months[elt->month - 1] : (const char *) "???";
  int m = elt->month;
  int y = elt->year + BASEYEAR;
  if (elt->month <= 2) {	/* if before March, */
    m = elt->month + 9;		/* January = month 10 of previous year */
    y--;
  }
  else m = elt->month - 3;	/* March is month 0 */
  sprintf (string,"%s %s %2d %02d:%02d:%02d %4d\n",
	   days[(int)(elt->day+2+((7+31*m)/12)+y+(y/4)+(y/400)-(y/100)) % 7],s,
	   elt->day,elt->hours,elt->minutes,elt->seconds,elt->year + BASEYEAR);
  return string;
}

/* Mail parse date into elt fields
 * Accepts: elt to write into
 *	    date string to parse
 * Returns: T if parse successful, else NIL
 * This routine parses dates as follows:
 * . leading three alphas followed by comma and space are ignored
 * . date accepted in format: mm/dd/yy, mm/dd/yyyy, dd-mmm-yy, dd-mmm-yyyy,
 *    dd mmm yy, dd mmm yyyy
 * . space or end of string required
 * . time accepted in format hh:mm:ss or hh:mm
 * . end of string accepted
 * . timezone accepted: hyphen followed by symbolic timezone, or space
 *    followed by signed numeric timezone or symbolic timezone
 * Examples of normal input:
 * . IMAP date-only (SEARCH): dd-mmm-yy, dd-mmm-yyyy, mm/dd/yy, mm/dd/yyyy
 * . IMAP date-time (INTERNALDATE):
 *    dd-mmm-yy hh:mm:ss-zzz
 *    dd-mmm-yyyy hh:mm:ss +zzzz
 * . RFC-822:
 *    www, dd mmm yy hh:mm:ss zzz
 *    www, dd mmm yyyy hh:mm:ss +zzzz
 */

long mail_parse_date (elt,s)
	MESSAGECACHE *elt;
	char *s;
{
  long d,m,y;
  int ms;
  struct tm *t;
  time_t tn;
  char tmp[MAILTMPLEN];
				/* make a writeable uppercase copy */
  if (s && *s && (strlen (s) < MAILTMPLEN)) s = ucase (strcpy (tmp,s));
  else return NIL;
				/* skip over possible day of week */
  if (isalpha (*s) && isalpha (s[1]) && isalpha (s[2]) && (s[3] == ',') &&
      (s[4] == ' ')) s += 5;
  while (*s == ' ') s++;	/* parse first number (probable month) */
  if (!(m = strtol ((const char *) s,&s,10))) return NIL;

  switch (*s) {			/* different parse based on delimiter */
  case '/':			/* mm/dd/yy format */
    if (!((d = strtol ((const char *) ++s,&s,10)) && *s == '/' &&
	  (y = strtol ((const char *) ++s,&s,10)) && *s == '\0')) return NIL;
    break;
  case ' ':			/* dd mmm yy format */
    while (s[1] == ' ') s++;	/* slurp extra whitespace */
  case '-':			/* dd-mmm-yy format */
    d = m;			/* so the number we got is a day */
				/* make sure string long enough! */
    if (strlen (s) < 5) return NIL;
    /* Some compilers don't allow `<<' and/or longs in case statements. */
				/* slurp up the month string */
    ms = ((s[1] - 'A') * 1024) + ((s[2] - 'A') * 32) + (s[3] - 'A');
    switch (ms) {		/* determine the month */
    case (('J'-'A') * 1024) + (('A'-'A') * 32) + ('N'-'A'): m = 1; break;
    case (('F'-'A') * 1024) + (('E'-'A') * 32) + ('B'-'A'): m = 2; break;
    case (('M'-'A') * 1024) + (('A'-'A') * 32) + ('R'-'A'): m = 3; break;
    case (('A'-'A') * 1024) + (('P'-'A') * 32) + ('R'-'A'): m = 4; break;
    case (('M'-'A') * 1024) + (('A'-'A') * 32) + ('Y'-'A'): m = 5; break;
    case (('J'-'A') * 1024) + (('U'-'A') * 32) + ('N'-'A'): m = 6; break;
    case (('J'-'A') * 1024) + (('U'-'A') * 32) + ('L'-'A'): m = 7; break;
    case (('A'-'A') * 1024) + (('U'-'A') * 32) + ('G'-'A'): m = 8; break;
    case (('S'-'A') * 1024) + (('E'-'A') * 32) + ('P'-'A'): m = 9; break;
    case (('O'-'A') * 1024) + (('C'-'A') * 32) + ('T'-'A'): m = 10; break;
    case (('N'-'A') * 1024) + (('O'-'A') * 32) + ('V'-'A'): m = 11; break;
    case (('D'-'A') * 1024) + (('E'-'A') * 32) + ('C'-'A'): m = 12; break;
    default: return NIL;	/* unknown month */
    }
    if ((s[4] == *s) &&	(y = strtol ((const char *) s+5,&s,10)) &&
	(*s == '\0' || *s == ' ')) break;
  default: return NIL;		/* unknown date format */
  }
				/* minimal validity check of date */
  if (d < 1 || d > 31 || m < 1 || m > 12 || y < 0) return NIL;
				/* Tenex/ARPAnet began in 1969 */
  if (y < 100) y += (y >= (BASEYEAR - 1900)) ? 1900 : 2000;
				/* set values in elt */
  elt->day = d; elt->month = m; elt->year = y - BASEYEAR;

  if (*s) {			/* time specification present? */
				/* parse time */
    d = strtol ((const char *) s,&s,10);
    if (*s != ':') return NIL;
    m = strtol ((const char *) ++s,&s,10);
    y = (*s == ':') ? strtol ((const char *) ++s,&s,10) : 0;
				/* validity check time */
    if (d < 0 || d > 23 || m < 0 || m > 59 || y < 0 || y > 59) return NIL;
				/* set values in elt */
    elt->hours = d; elt->minutes = m; elt->seconds = y;
    switch (*s) {		/* time zone specifier? */
    case ' ':			/* numeric time zone */
      while (s[1] == ' ') s++;	/* slurp extra whitespace */
      if (!isalpha (s[1])) {	/* treat as '-' case if alphabetic */
				/* test for sign character */
	if ((elt->zoccident = (*++s == '-')) || (*s == '+')) s++;
				/* validate proper timezone */
	if (isdigit(*s) && isdigit(s[1]) && isdigit(s[2]) && isdigit(s[3])) {
	  elt->zhours = (*s - '0') * 10 + (s[1] - '0');
	  elt->zminutes = (s[2] - '0') * 10 + (s[3] - '0');
	}
	return T;		/* all done! */
      }
				/* falls through */

    case '-':			/* symbolic time zone */
      if (!(ms = *++s)) ms = 'Z';
      else if (*++s) {		/* multi-character? */
	ms -= 'A'; ms *= 1024;	/* yes, make compressed three-byte form */
	ms += ((*s++ - 'A') * 32);
	if (*s) ms += *s++ - 'A';
	if (*s) ms = '\0';	/* more than three characters */
      }
      /* This is not intended to be a comprehensive list of all possible
       * timezone strings.  Such a list would be impractical.  Rather, this
       * listing is intended to incorporate all military, north American, and
       * a few special cases such as Japan and the major European zone names,
       * such as what might be expected to be found in a Tenex format mailbox
       * and spewed from an IMAP server.  The trend is to migrate to numeric
       * timezones which lack the flavor but also the ambiguity of the names.
       */
      switch (ms) {		/* determine the timezone */
	/* oriental (from Greenwich) timezones */
				/* Middle Europe */
      case (('M'-'A')*1024)+(('E'-'A')*32)+'T'-'A':
      case 'A': elt->zhours = 1; break;
				/* Eastern Europe */
      case (('E'-'A')*1024)+(('E'-'A')*32)+'T'-'A':
      case 'B': elt->zhours = 2; break;
      case 'C': elt->zhours = 3; break;
      case 'D': elt->zhours = 4; break;
      case 'E': elt->zhours = 5; break;
      case 'F': elt->zhours = 6; break;
      case 'G': elt->zhours = 7; break;
      case 'H': elt->zhours = 8; break;
				/* Japan */
      case (('J'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
      case 'I': elt->zhours = 9; break;
      case 'K': elt->zhours = 10; break;
      case 'L': elt->zhours = 11; break;
      case 'M': elt->zhours = 12; break;

	/* occidental (from Greenwich) timezones */
      case 'N': elt->zoccident = 1; elt->zhours = 1; break;
      case 'O': elt->zoccident = 1; elt->zhours = 2; break;
      case (('A'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
      case 'P': elt->zoccident = 1; elt->zhours = 3; break;
				/* Atlantic */
      case (('A'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
      case (('E'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
      case 'Q': elt->zoccident = 1; elt->zhours = 4; break;
				/* Eastern */
      case (('E'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
      case (('C'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
      case 'R': elt->zoccident = 1; elt->zhours = 5; break;
				/* Central */
      case (('C'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
      case (('M'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
      case 'S': elt->zoccident = 1; elt->zhours = 6; break;
				/* Mountain */
      case (('M'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
      case (('P'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
      case 'T': elt->zoccident = 1; elt->zhours = 7; break;
				/* Pacific */
      case (('P'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
      case (('Y'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
      case 'U': elt->zoccident = 1; elt->zhours = 8; break;
				/* Yukon */
      case (('Y'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
      case (('H'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
      case 'V': elt->zoccident = 1; elt->zhours = 9; break;
				/* Hawaii */
      case (('H'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
      case (('B'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
      case 'W': elt->zoccident = 1; elt->zhours = 10; break;
				/* Bering */
      case (('B'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
      case 'X': elt->zoccident = 1; elt->zhours = 11; break;
      case 'Y': elt->zoccident = 1; elt->zhours = 12; break;
				/* Universal */
      case (('U'-'A')*1024)+(('T'-'A')*32):
      case (('G'-'A')*1024)+(('M'-'A')*32)+'T'-'A':
      case 'Z': elt->zhours = 0; break;

      default:			/* assume local otherwise */
	tn = time (0);		/* time now... */
	t = localtime (&tn);	/* get local minutes since midnight */
	m = t->tm_hour * 60 + t->tm_min;
	ms = t->tm_yday;	/* note Julian day */
	t = gmtime (&tn);	/* minus UTC minutes since midnight */
	m -= t->tm_hour * 60 + t->tm_min;
	/* ms can be one of:
	 *  36x  local time is December 31, UTC is January 1, offset -24 hours
	 *    1  local time is 1 day ahead of UTC, offset +24 hours
	 *    0  local time is same day as UTC, no offset
	 *   -1  local time is 1 day behind UTC, offset -24 hours
	 * -36x  local time is January 1, UTC is December 31, offset +24 hours
	 */
	if (ms -= t->tm_yday)	/* correct offset if different Julian day */
	  m += ((ms < 0) == (abs (ms) == 1)) ? -24*60 : 24*60;
	if (m < 0) {		/* occidental? */
	  m = abs (m);		/* yup, make positive number */
	  elt->zoccident = 1;	/* and note west of UTC */
	}
	elt->zhours = m / 60;	/* now break into hours and minutes */
	elt->zminutes = m % 60;
	break;
      }
      elt->zminutes = 0;	/* never a fractional hour */
      break;
    case '\0':			/* no time zone */
    default:			/* bogus time zone */
      break;			/* ignore both cases */
    }
  }
  else {			/* make sure all time fields zero */
    elt->hours = elt->minutes = elt->seconds = elt->zhours = elt->zminutes =
      elt->zoccident = 0;
  }
  return T;
}

/* Mail messages have been searched out
 * Accepts: mail stream
 *	    message number
 *
 * Calls external "mm_searched" function to notify main program
 */

void mail_searched (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
				/* mark as searched */
  mail_elt (stream,msgno)->searched = T;
  mm_searched (stream,msgno);	/* notify main program */
}


/* Mail n messages exist
 * Accepts: mail stream
 *	    number of messages
 *
 * Calls external "mm_exists" function that notifies main program prior
 * to updating the stream
 */

void mail_exists (stream,nmsgs)
	MAILSTREAM *stream;
	long nmsgs;
{
				/* make sure cache is large enough */
  (*mailcache) (stream,nmsgs,CH_SIZE);
				/* notify main program of change */
  if (!stream->silent) mm_exists (stream,nmsgs);
  stream->nmsgs = nmsgs;	/* update stream status */
}

/* Mail n messages are recent
 * Accepts: mail stream
 *	    number of recent messages
 */

void mail_recent (stream,recent)
	MAILSTREAM *stream;
	long recent;
{
  stream->recent = recent;	/* update stream status */
}


/* Mail message n is expunged
 * Accepts: mail stream
 *	    message #
 *
 * Calls external "mm_expunged" function that notifies main program prior
 * to updating the stream
 */

void mail_expunged (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  long i = msgno - 1;
  MESSAGECACHE *elt = (MESSAGECACHE *) (*mailcache) (stream,msgno,CH_ELT);
  if (elt) {			/* if an element is there */
    elt->msgno = 0;		/* invalidate its message number and free */
    (*mailcache) (stream,msgno,CH_FREE);
  }
				/* expunge the slot */
  (*mailcache) (stream,msgno,CH_EXPUNGE);
  --stream->nmsgs;		/* update stream status */
  stream->msgno = 0;		/* nuke the short cache too */
  mail_free_envelope (&stream->env);
  mail_free_body (&stream->body);
				/* notify main program of change */
  if (!stream->silent) mm_expunged (stream,msgno);
}

/* mail stream status routines */


/* Mail lock stream
 * Accepts: mail stream
 */

void mail_lock (stream)
	MAILSTREAM *stream;
{
  if (stream->lock) fatal ("Lock when already locked");
  else stream->lock = T;	/* lock stream */
}


/* Mail unlock stream
 * Accepts: mail stream
 */

void mail_unlock (stream)
	MAILSTREAM *stream;
{
  if (!stream->lock) fatal ("Unlock when not locked");
  else stream->lock = NIL;	/* unlock stream */
}


/* Mail turn on debugging telemetry
 * Accepts: mail stream
 */

void mail_debug (stream)
	MAILSTREAM *stream;
{
  stream->debug = T;		/* turn on debugging telemetry */
}


/* Mail turn off debugging telemetry
 * Accepts: mail stream
 */

void mail_nodebug (stream)
	MAILSTREAM *stream;
{
  stream->debug = NIL;		/* turn off debugging telemetry */
}

/* Mail parse sequence
 * Accepts: mail stream
 *	    sequence to parse
 * Returns: T if parse successful, else NIL
 */

long mail_sequence (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  long i,j,x;
  for (i = 1; i <= stream->nmsgs; i++) mail_elt (stream,i)->sequence = NIL;
  while (*sequence) {		/* while there is something to parse */
				/* parse and validate message number */
    if (((i = (int) strtol ((const char *) sequence,&sequence,10)) < 1) ||
	(i > stream->nmsgs)) {
      mm_log ("Sequence invalid",ERROR);
      return NIL;
    }
    switch (*sequence) {	/* see what the delimiter is */
    case ':':			/* sequence range */
				/* parse end of range */
      if (((j = (int) strtol ((const char *) ++sequence,&sequence,10)) < 1) ||
	  (j > stream->nmsgs) || (*sequence && *sequence++ != ',')) {
	mm_log ("Sequence range invalid",ERROR);
	return NIL;
      }
      if (i > j) {		/* swap the range if backwards */
	x = i; i = j; j = x;
      }
				/* mark each item in the sequence */
      while (i <= j) mail_elt (stream,j--)->sequence = T;
      break;
    case ',':			/* single message */
      ++sequence;		/* skip the delimiter, fall into end case */
    case '\0':			/* end of sequence, mark this message */
      mail_elt (stream,i)->sequence = T;
      break;
    default:			/* anything else is a syntax error! */
      mm_log ("Syntax error in sequence",ERROR);
      return NIL;
    }
  }
  return T;			/* successfully parsed sequence */
}

/* Mail data structure instantiation routines */


/* Mail instantiate envelope
 * Returns: new envelope
 */

ENVELOPE *mail_newenvelope ()
{
  ENVELOPE *env = (ENVELOPE *) fs_get (sizeof (ENVELOPE));
  env->remail = NIL;		/* initialize all fields */
  env->return_path = NIL;
  env->date = NIL;
  env->subject = NIL;
  env->from = env->sender = env->reply_to = env->to = env->cc = env->bcc = NIL;
  env->in_reply_to = env->message_id = env->newsgroups = env->followup_to =
    env->references = NIL;
  return env;
}


/* Mail instantiate address
 * Returns: new address
 */

ADDRESS *mail_newaddr ()
{
  ADDRESS *adr = (ADDRESS *) fs_get (sizeof (ADDRESS));
				/* initialize all fields */
  adr->personal = adr->adl = adr->mailbox = adr->host = adr->error = NIL;
  adr->next = NIL;
  return adr;
}


/* Mail instantiate body
 * Returns: new body
 */

BODY *mail_newbody ()
{
  return mail_initbody ((BODY *) fs_get (sizeof (BODY)));
}

/* Mail initialize body
 * Accepts: body
 * Returns: body
 */

BODY *mail_initbody (body)
	BODY *body;
{
  body->type = TYPETEXT;	/* content type */
  body->encoding = ENC7BIT;	/* content encoding */
  body->subtype = body->id = body->description = NIL;
  body->parameter = NIL;
  body->contents.text = NIL;	/* no contents yet */
  body->contents.binary = NIL;
  body->contents.part = NIL;
  body->contents.msg.env = NIL;
  body->contents.msg.body = NIL;
  body->contents.msg.text = NIL;
  body->size.lines = body->size.bytes = body->size.ibytes = 0;
  body->md5 = NIL;
  return body;
}


/* Mail instantiate body parameter
 * Returns: new body part
 */

PARAMETER *mail_newbody_parameter ()
{
  PARAMETER *parameter = (PARAMETER *) fs_get (sizeof (PARAMETER));
  parameter->attribute = parameter->value = NIL;
  parameter->next = NIL;	/* no next yet */
  return parameter;
}


/* Mail instantiate body part
 * Returns: new body part
 */

PART *mail_newbody_part ()
{
  PART *part = (PART *) fs_get (sizeof (PART));
  mail_initbody (&part->body);	/* initialize the body */
  part->offset = 0;		/* no offset yet */
  part->next = NIL;		/* no next yet */
  return part;
}

/* Mail garbage collection routines */


/* Mail garbage collect body
 * Accepts: pointer to body pointer
 */

void mail_free_body (body)
	BODY **body;
{
  if (*body) {			/* only free if exists */
    mail_free_body_data (*body);/* free its data */
    fs_give ((void **) body);	/* return body to free storage */
  }
}


/* Mail garbage collect body data
 * Accepts: body pointer
 */

void mail_free_body_data (body)
	BODY *body;
{
  if (body->subtype) fs_give ((void **) &body->subtype);
  mail_free_body_parameter (&body->parameter);
  if (body->id) fs_give ((void **) &body->id);
  if (body->description) fs_give ((void **) &body->description);
  if (body->md5) fs_give ((void **) &body->md5);
  switch (body->type) {		/* free contents */
  case TYPETEXT:		/* unformatted text */
    if (body->contents.text) fs_give ((void **) &body->contents.text);
    break;
  case TYPEMULTIPART:		/* multiple part */
    mail_free_body_part (&body->contents.part);
    break;
  case TYPEMESSAGE:		/* encapsulated message */
    mail_free_envelope (&body->contents.msg.env);
    mail_free_body (&body->contents.msg.body);
    if (body->contents.msg.text)
      fs_give ((void **) &body->contents.msg.text);
    break;
  case TYPEAPPLICATION:		/* application data */
  case TYPEAUDIO:		/* audio */
  case TYPEIMAGE:		/* static image */
  case TYPEVIDEO:		/* video */
    if (body->contents.binary) fs_give (&body->contents.binary);
    break;
  default:
    break;
  }
}

/* Mail garbage collect body parameter
 * Accepts: pointer to body parameter pointer
 */

void mail_free_body_parameter (parameter)
	PARAMETER **parameter;
{
  if (*parameter) {		/* only free if exists */
    if ((*parameter)->attribute) fs_give ((void **) &(*parameter)->attribute);
    if ((*parameter)->value) fs_give ((void **) &(*parameter)->value);
				/* run down the list as necessary */
    mail_free_body_parameter (&(*parameter)->next);
				/* return body part to free storage */
    fs_give ((void **) parameter);
  }
}


/* Mail garbage collect body part
 * Accepts: pointer to body part pointer
 */

void mail_free_body_part (part)
	PART **part;
{
  if (*part) {			/* only free if exists */
    mail_free_body_data (&(*part)->body);
				/* run down the list as necessary */
    mail_free_body_part (&(*part)->next);
    fs_give ((void **) part);	/* return body part to free storage */
  }
}

/* Mail garbage collect message cache
 * Accepts: mail stream
 *
 * The message cache is set to NIL when this function finishes.
 */

void mail_free_cache (stream)
	MAILSTREAM *stream;
{
				/* flush the cache */
  (*mailcache) (stream,(long) 0,CH_INIT);
  stream->msgno = 0;		/* free this cruft too */
  mail_free_envelope (&stream->env);
  mail_free_body (&stream->body);
  if (stream->text) fs_give ((void **) &stream->text);
}


/* Mail garbage collect cache element
 * Accepts: pointer to cache element pointer
 */

void mail_free_elt (elt)
	MESSAGECACHE **elt;
{
				/* only free if exists and no sharers */
  if (*elt && !--(*elt)->lockcount) fs_give ((void **) elt);
  else *elt = NIL;		/* else simply drop pointer */
}


/* Mail garbage collect long cache element
 * Accepts: pointer to long cache element pointer
 */

void mail_free_lelt (lelt)
	LONGCACHE **lelt;
{
				/* only free if exists and no sharers */
  if (*lelt && !--(*lelt)->elt.lockcount) {
    mail_free_envelope (&(*lelt)->env);
    mail_free_body (&(*lelt)->body);
    fs_give ((void **) lelt);	/* return cache element to free storage */
  }
  else *lelt = NIL;		/* else simply drop pointer */
}

/* Mail garbage collect envelope
 * Accepts: pointer to envelope pointer
 */

void mail_free_envelope (env)
	ENVELOPE **env;
{
  if (*env) {			/* only free if exists */
    if ((*env)->remail) fs_give ((void **) &(*env)->remail);
    mail_free_address (&(*env)->return_path);
    if ((*env)->date) fs_give ((void **) &(*env)->date);
    mail_free_address (&(*env)->from);
    mail_free_address (&(*env)->sender);
    mail_free_address (&(*env)->reply_to);
    if ((*env)->subject) fs_give ((void **) &(*env)->subject);
    mail_free_address (&(*env)->to);
    mail_free_address (&(*env)->cc);
    mail_free_address (&(*env)->bcc);
    if ((*env)->in_reply_to) fs_give ((void **) &(*env)->in_reply_to);
    if ((*env)->message_id) fs_give ((void **) &(*env)->message_id);
    if ((*env)->newsgroups) fs_give ((void **) &(*env)->newsgroups);
    if ((*env)->followup_to) fs_give ((void **) &(*env)->followup_to);
    if ((*env)->references) fs_give ((void **) &(*env)->references);
    fs_give ((void **) env);	/* return envelope to free storage */
  }
}


/* Mail garbage collect address
 * Accepts: pointer to address pointer
 */

void mail_free_address (address)
	ADDRESS **address;
{
  if (*address) {		/* only free if exists */
    if ((*address)->personal) fs_give ((void **) &(*address)->personal);
    if ((*address)->adl) fs_give ((void **) &(*address)->adl);
    if ((*address)->mailbox) fs_give ((void **) &(*address)->mailbox);
    if ((*address)->host) fs_give ((void **) &(*address)->host);
    if ((*address)->error) fs_give ((void **) &(*address)->error);
    mail_free_address (&(*address)->next);
    fs_give ((void **) address);/* return address to free storage */
  }
}
