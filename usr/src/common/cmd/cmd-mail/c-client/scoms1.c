#ident	"@(#)scoms1.c	11.1"

/*
 * SCO Message Store Version 1 routines.
 */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/param.h>
extern int errno;		/* just in case */
#include <signal.h>
#include <pwd.h>
#include "mail.h"
#include "osdep.h"
#include "scoms1.h"
#include "scomsc1.h"
#include "rfc822.h"
#include "misc.h"
#include "dummy.h"

#define debug printf

/* Driver dispatch used by MAIL */

DRIVER scoms1driver = {
  "scoms1",			/* driver name */
  DR_LOCAL|DR_MAIL,		/* driver flags */
  (DRIVER *) NIL,		/* next driver */
  scoms1_valid,			/* mailbox is valid for us */
  scoms1_parameters,		/* manipulate parameters */
  scoms1_scan,			/* scan mailboxes */
  scoms1_list,			/* list mailboxes */
  scoms1_lsub,			/* list subscribed mailboxes */
  NIL,				/* subscribe to mailbox */
  NIL,				/* unsubscribe from mailbox */
  scoms1_create,		/* create mailbox */
  scoms1_delete,		/* delete mailbox */
  scoms1_rename,		/* rename mailbox */
  NIL,				/* status of mailbox */
  scoms1_open,			/* open mailbox */
  scoms1_close,			/* close mailbox */
  scoms1_fetchfast,		/* fetch message "fast" attributes */
  scoms1_fetchflags,		/* fetch message flags */
  scoms1_fetchstructure,	/* fetch message envelopes */
  scoms1_fetchheader,		/* fetch message header only */
  scoms1_fetchtext,		/* fetch message body only */
  scoms1_fetchbody,		/* fetch message body section */
  NIL,				/* unique identifier */
  scoms1_setflag,		/* set message flag */
  scoms1_clearflag,		/* clear message flag */
  NIL,				/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  scoms1_ping,			/* ping mailbox to see if still alive */
  scoms1_check,			/* check for new messages */
  scoms1_expunge,		/* expunge deleted messages */
  scoms1_copy,			/* copy messages to another mailbox */
  scoms1_append,		/* append string message to mailbox */
  scoms1_gc,			/* garbage collect stream */
  scoms1_deliver,		/* append message with envelope info */
};

				/* prototype stream */
MAILSTREAM scoms1proto = {&scoms1driver};

				/* driver parameters */
static long scoms1_fromwidget = T;

/* Scoms1 mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *
scoms1_valid(char *name)
{
  int ret;
  struct stat sbuf;

  errno = EINVAL;		/* assume invalid argument */
  ret = 1;
  if (msc1_strccmp(name, "INBOX")) {
    ret = scomsc1_valid(name);
    ret = ret ? 1 : 0;
  }
  return ret ? &scoms1driver : NIL;
}

/* Scoms1 manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *
scoms1_parameters(long function,void *value)
{
  switch ((int) function) {
  case SET_FROMWIDGET:
    scoms1_fromwidget = (long) value;
    break;
  case GET_FROMWIDGET:
    value = (void *) scoms1_fromwidget;
    break;
  default:
    value = NIL;		/* error case */
    break;
  }
  return value;
}

/* Scoms1 mail scan mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void
scoms1_scan(MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
  if (stream) dummy_scan(NIL,ref,pat,contents);
}


/* Scoms1 mail list mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void
scoms1_list(MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_list(NIL,ref,pat);
}


/* Scoms1 mail list subscribed mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void
scoms1_lsub(MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_lsub(NIL,ref,pat);
}

/* Scoms1 mail create mailbox
 * Accepts: MAIL stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long
scoms1_create(MAILSTREAM *stream,char *mailbox)
{
  int ret;

  ret = scomsc1_create(mailbox);
  if (ret == 0)
    mm_log("Unable to create mailbox", ERROR);
  return ret;
}


/* Scoms1 mail delete mailbox
 * Accepts: MAIL stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long
scoms1_delete(MAILSTREAM *stream,char *mailbox)
{
  int ret;

  ret = scomsc1_remove(mailbox);
  if (ret == 0)
    mm_log("Unable to remove mailbox", ERROR);
  return(ret);
}

/* Scoms1 mail rename mailbox
 * Accepts: MAIL stream
 *	    old mailbox name
 *	    new mailbox name (or NIL for delete)
 * Returns: T on success, NIL on failure
 */

long
scoms1_rename(MAILSTREAM *stream,char *old,char *newname)
{
  int ret;

  ret = scomsc1_rename(newname, old);
  if (ret == 0)
    mm_log("Unable to rename mailbox", ERROR);
  return ret;			/* return success */
}

/* Scoms1 mail open
 * Accepts: Stream to open
 * Returns: Stream on success, NIL on failure
 */

MAILSTREAM *
scoms1_open(MAILSTREAM *stream)
{
  long i;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &scoms1proto;
  stream->scache = !conf_long_cache;	/* override caller's preference */
  if (LOCAL) {			/* close old file if stream being recycled */
    scoms1_close(stream,NIL);	/* dump and save the changes */
    stream->dtb = &scoms1driver;/* reattach this driver */
    mail_free_cache(stream);	/* clean up cache */
  }
  stream->local = fs_get(sizeof(SCOMS1LOCAL));

  /* You may wonder why LOCAL->name is needed.  It isn't at all obvious from
   * the code.  The problem is that when a stream is recycled with another
   * mailbox of the same type, the driver's close method isn't called because
   * it could be IMAP and closing then would defeat the entire point of
   * recycling.  Hence there is code in the file drivers to call the close
   * method such as what appears above.  The problem is, by this point,
   * mail_open() has already changed the stream->mailbox name to point to the
   * new name, and scoms1_close() needs the old name.
   */
  /* init values */
  LOCAL->buf = (char *) fs_get((LOCAL->buflen = MSCSBUFSIZ) + 1);
  LOCAL->handle = 0;
  LOCAL->rdonly = stream->rdonly;
  stream->sequence++;		/* bump sequence number */

  if (stream->rdonly == 0)
    LOCAL->handle = scomsc1_open(stream->mailbox, ACCESS_SE, 0);
  if ((LOCAL->handle == (void *)1) || (LOCAL->handle == (void *)2))
    LOCAL->handle = 0;
  if (LOCAL->handle == 0) {
    LOCAL->handle = scomsc1_open(stream->mailbox, ACCESS_RD, 0);
    if (LOCAL->handle == 0) {
      LOCAL->handle = 0;
      mm_log("System error", ERROR);
      return(0);
    }
    if (LOCAL->handle == (void *)1) {
      LOCAL->handle = 0;
      mm_log("Invalid mailbox format", ERROR);
      return(0);
    }
    if (LOCAL->handle == (void *)2) {
      LOCAL->handle = 0;
      mm_log("Mailbox is locked", ERROR);
      return(0);
    }
    if (stream->rdonly == 0) {
      mm_log("Mailbox is open by another process, access is readonly", WARN);
      stream->rdonly = 1;
      LOCAL->rdonly = 1;
    }
  }
  /* parse mailbox */
  stream->nmsgs = stream->recent = 0;

				/* reset UID validity */
  stream->uid_validity = stream->uid_last = 0;
				/* abort if can't get RW silent stream */
  if (stream->silent && !stream->rdonly)
    scoms1_abort(stream);
				/* parse mailbox */
  else {
    scoms1_parse(stream);
    mail_unlock(stream);
  }
  if (!LOCAL)
    return NIL;			/* failure if stream died */

				/* notify about empty mailbox */
  if (!(stream->nmsgs || stream->silent))
    mm_log("Mailbox is empty",NIL);
  if (!stream->rdonly)
    stream->perm_seen = stream->perm_deleted = stream->perm_flagged
      = stream->perm_answered = stream->perm_draft = T;
  return stream;		/* return stream alive to caller */
}

/* Scoms1 mail close
 * Accepts: MAIL stream
 *	    close options
 */

void
scoms1_close(MAILSTREAM *stream,long options)
{
  int silent = stream->silent;
  stream->silent = T;		/* note this stream is dying */
  if (options & CL_EXPUNGE)
    scoms1_expunge(stream);
				/* dump final checkpoint if needed */
  stream->silent = silent;	/* restore previous status */
  scoms1_abort(stream);		/* now punt the file and local data */
}


/* Scoms1 mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void
scoms1_fetchfast(MAILSTREAM *stream,char *sequence,long flags)
{
  return;			/* no-op for local mail */
}


/* Scoms1 mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void
scoms1_fetchflags(MAILSTREAM *stream,char *sequence,long flags)
{
  return;			/* no-op for local mail */
}

/* Scoms1 mail fetch structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 *	    option flags
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *
scoms1_fetchstructure(MAILSTREAM *stream,unsigned long msgno,
				 BODY **body,long flags)
{
  ENVELOPE **env;
  BODY **b;
  STRING bs;
  LONGCACHE *lelt;
  unsigned long i;
  char *bodystr;		/* pointer from our cache */
  int bodylen;
  char *hdrstr;			/* pointer from our cache */
  int hdrlen;

  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid(stream,i) == msgno)
	return scoms1_fetchstructure(stream,i,body,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  hdrlen = scomsc1_fetchsize(LOCAL->handle, msgno, 0);
  bodylen = scomsc1_fetchsize(LOCAL->handle, msgno, 1);
  i = max(hdrlen, bodylen);
  if (stream->scache) {		/* short cache */
    if (msgno != stream->msgno){/* flush old poop if a different message */
      mail_free_envelope(&stream->env);
      mail_free_body(&stream->body);
    }
    stream->msgno = msgno;
    env = &stream->env;		/* get pointers to envelope and body */
    b = &stream->body;
  }
  else {			/* long cache */
    lelt = mail_lelt(stream,msgno);
    env = &lelt->env;		/* get pointers to envelope and body */
    b = &lelt->body;
  }
  if ((body && !*b) || !*env) {	/* have the poop we need? */
    mail_free_envelope(env);	/* flush old envelope and body */
    mail_free_body(b);
    if (i > LOCAL->buflen) {	/* make sure enough buffer space */
      fs_give((void **) &LOCAL->buf);
      LOCAL->buf = (char *) fs_get((LOCAL->buflen = i) + 1);
    }
    hdrstr = scomsc1_fetch(LOCAL->handle, 0, msgno, 0);
    if (hdrstr == 0)
      return(NIL);
    bodystr = scomsc1_fetch(LOCAL->handle, 0, msgno, 1);
    if (bodystr == 0) {
      free(hdrstr);
      return(NIL);
    }
    INIT (&bs,mail_string,(void *) bodystr, bodylen);
		/* parse envelope and body */
    rfc822_parse_msg(env,body ? b : NIL,hdrstr,hdrlen,&bs,
		    mylocalhost(),LOCAL->buf);
    free(hdrstr);
    free(bodystr);
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* Scoms1 mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    list of headers to fetch
 *	    pointer to returned header text length
 *	    option flags
 * Returns: message header in RFC822 format, in a local static buffer
 */

char *
scoms1_fetchheader(MAILSTREAM *stream,unsigned long msgno,
			  STRINGLIST *lines,unsigned long *len,long flags)
{
  char *hdr;
  char *hdrstr;
  int hdrlen;
  unsigned long i;

  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid(stream,i) == msgno)
	return scoms1_fetchheader(stream,i,lines,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  hdrlen = scomsc1_fetchsize(LOCAL->handle, msgno, 0);
  hdrstr = scomsc1_fetch(LOCAL->handle, 0, msgno, 0);
  if (hdrstr == 0)
    return(NIL);
  if (lines || !(flags & FT_INTERNAL)) {
				/* copy the string */
    i = strcrlfcpy(&LOCAL->buf,&LOCAL->buflen,hdrstr,hdrlen);
    if (lines) i = mail_filter(LOCAL->buf,i,lines,flags);
    hdr = LOCAL->buf;		/* return processed copy */
  }
  else {
    if (hdrlen > LOCAL->buflen) {	/* make sure enough buffer space */
      fs_give((void **) &LOCAL->buf);
      LOCAL->buf = (char *) fs_get((LOCAL->buflen = hdrlen) + 1);
    }
    strcpy(LOCAL->buf, hdrstr);
    hdr = LOCAL->buf;
    i = hdrlen;
  }
  free(hdrstr);

  if (len) *len = i;
  return hdr;
}

/* Scoms1 mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned message length
 *	    option flags
 * Returns: message text in RFC822 format
 */

char *
scoms1_fetchtext(MAILSTREAM *stream,unsigned long msgno,
			unsigned long *len,long flags)
{
  char *txt;
  unsigned long i;
  MESSAGECACHE *elt;
  char *bodystr;
  int bodylen;

  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid(stream,i) == msgno)
	return scoms1_fetchtext(stream,i,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  elt = mail_elt(stream,msgno);/* get message status */
				/* if message not seen */
  if (!(flags & FT_PEEK) && !elt->seen) {
    elt->seen = T;		/* mark message as seen */
    scoms1_update_status(stream, elt);
  }
  bodylen = scomsc1_fetchsize(LOCAL->handle, msgno, 1);
  bodystr = scomsc1_fetch(LOCAL->handle, 0, msgno, 1);
  if (bodystr == 0)
    return(NIL);
  if (flags & FT_INTERNAL) {	/* internal data OK? */
    if (bodylen > LOCAL->buflen) {	/* make sure enough buffer space */
      fs_give((void **) &LOCAL->buf);
      LOCAL->buf = (char *) fs_get((LOCAL->buflen = bodylen) + 1);
    }
    strcpy(LOCAL->buf, bodystr);
    txt = LOCAL->buf;
    i = bodylen;
  }
  else {			/* need to process data */
    i = strcrlfcpy(&LOCAL->buf,&LOCAL->buflen,bodystr,bodylen);
    txt = LOCAL->buf;
  }
  free(bodystr);
  if (len) *len = i;		/* return size */
  return txt;
}

/* Scoms1 fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 *	    option flags
 * Returns: pointer to section of message body
 */

char *
scoms1_fetchbody(MAILSTREAM *stream,unsigned long msgno,char *s,
			unsigned long *len,long flags)
{
  BODY *b;
  PART *pt;
  char *base;
  char *ret;
  unsigned long offset = 0;
  unsigned long i,size = 0;
  unsigned short enc = ENC7BIT;
  MESSAGECACHE *elt;

  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid(stream,i) == msgno)
	return scoms1_fetchbody(stream,i,s,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  elt = mail_elt(stream,msgno);/* get elt */
				/* make sure have a body */
  if (!(scoms1_fetchstructure(stream,msgno,&b,flags & ~FT_UID) && b && s &&
	*s && isdigit(*s))) return NIL;
  if (!(i = strtoul(s,&s,10)))	/* section 0 */
    return *s ? NIL : scoms1_fetchheader(stream,msgno,NIL,len,flags);
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
    case TYPEMESSAGE:		/* embedded message */
      if (!((*s++ == '.') && isdigit(*s))) return NIL;
				/* get message's body if non-zero */
      if (i = strtoul(s,&s,10)) {
	offset = b->contents.msg.offset;
	b = b->contents.msg.body;
      }
      else {			/* want header */
	size = b->contents.msg.offset - offset;
	b = NIL;		/* make sure the code below knows */
      }
      break;
    case TYPEMULTIPART:		/* multipart, get next section */
      if ((*s++ == '.') && isdigit(*s) && (i = strtoul(s,&s,10)) > 0) break;
    default:			/* bogus subpart specification */
      return NIL;
    }
  } while (i);
  if (b) {			/* looking at a non-multipart body? */
    if (b->type == TYPEMULTIPART) return NIL;
    size = b->size.ibytes;	/* yes, get its size */
    enc = b->encoding;
  }
  else if (!size) return NIL;	/* lose if not getting a header */
				/* if message not seen */
  if (!(flags & FT_PEEK) && !elt->seen) {
    elt->seen = T;		/* mark message as seen */
				/* recalculate Status/X-Status lines */
    scoms1_update_status(stream, elt);
  }
  base = scomsc1_fetch(LOCAL->handle, 0, msgno, 1);
  ret = rfc822_contents(&LOCAL->buf,&LOCAL->buflen,len,base+offset,size,enc);
  free(base);
  return(ret);
}

/* Scoms1 mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void
scoms1_setflag(MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  unsigned long i,uf;
  long f = mail_parse_flags(stream,flag,&uf);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if ((flags & ST_UID) ? mail_uid_sequence(stream,sequence) :
      mail_sequence(stream,sequence))
    for (i = 1; i <= stream->nmsgs; i++)
      if ((elt = mail_elt(stream,i))->sequence) {
				/* set all requested flags */
	if (f&fSEEN) elt->seen = T;
	if (f&fDELETED) elt->deleted = T;
	if (f&fFLAGGED) elt->flagged = T;
	if (f&fANSWERED) elt->answered = T;
	if (f&fDRAFT) elt->draft = T;
				/* recalculate Status/X-Status lines */
	scoms1_update_status(stream, elt);
      }
}


/* Scoms1 mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void
scoms1_clearflag(MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  unsigned long i,uf;
  long f = mail_parse_flags(stream,flag,&uf);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if ((flags & ST_UID) ? mail_uid_sequence(stream,sequence) :
      mail_sequence(stream,sequence))
    for (i = 1; i <= stream->nmsgs; i++)
      if ((elt = mail_elt(stream,i))->sequence) {
				/* clear all requested flags */
	if (f&fSEEN) elt->seen = NIL;
	if (f&fDELETED) elt->deleted = NIL;
	if (f&fFLAGGED) elt->flagged = NIL;
	if (f&fANSWERED) elt->answered = NIL;
	if (f&fDRAFT) elt->draft = NIL;
				/* recalculate Status/X-Status lines */
	scoms1_update_status(stream, elt);
      }
}

/* Scoms1 mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 * locking protects readers from expunge underneath
 */

long
scoms1_ping(MAILSTREAM *stream)
{
  char lock[MAILTMPLEN];
				/* does he want to give up readwrite? */
  if (LOCAL && stream->rdonly && (LOCAL->rdonly == 0)) {
    scomsc1_chopen(LOCAL->handle, ACCESS_RD);
    LOCAL->rdonly = 1;
  }
				/* make sure it is alright to do this at all */
  if (LOCAL && !stream->lock) {
    scoms1_parse(stream);
    mail_unlock(stream);
  }
  return LOCAL ? T : NIL;	/* return if still alive */
}

/* Scoms1 mail check mailbox
 * Accepts: MAIL stream
 * locking protects readers from expunge underneath
 */

void
scoms1_check(MAILSTREAM *stream)
{
  if (LOCAL && !LOCAL->rdonly) {
    scoms1_parse(stream);
    mail_unlock(stream);	/* flush locks */
  }
  if (LOCAL && !LOCAL->rdonly && !stream->silent)
    mm_log("Check completed",NIL);
}

/* Scoms1 mail expunge mailbox
 * Accepts: MAIL stream
 */

void
scoms1_expunge(MAILSTREAM *stream)
{
  unsigned long i = 1;
  unsigned long j;
  unsigned long n = 0;
  unsigned long recent;
  MESSAGECACHE *elt;
  char *r = "No messages deleted, so no update needed";

  if (LOCAL && !LOCAL->rdonly && !stream->lock) {/* parse and lock mailbox */
    if (scomsc1_expunge(LOCAL->handle) == 0) {
      r = "Failed to get mailbox lock, no data loss, no messages expunged.";
      mm_log(r,ERROR);
    }
    else {
      mail_lock(stream);
      recent = stream->recent;	/* get recent now that new ones parsed */
      while ((j = (i<=stream->nmsgs)) && !(elt = mail_elt(stream,i))->deleted)
	i++;			/* find first deleted message */
      if (j) {			/* found one? */
				/* make sure we can do the worst case thing */
	do {			/* flush deleted messages */
	  if ((elt = mail_elt(stream,i))->deleted) {
				/* if recent, note one less recent message */
	    if (elt->recent) --recent;
				/* flush local cache entry */
				/* notify upper levels */
	    mail_expunged(stream,i);
	    n++;		/* count another expunged message */
	  }
	  else i++;		/* otherwise try next message */
	} while (i <= stream->nmsgs);
				/* dump checkpoint of the results */
	sprintf ((r = LOCAL->buf),"Expunged %ld messages",n);
      }
				/* notify upper level, free locks */
      mail_unlock(stream);
      scoms1_parse(stream);
      mail_unlock(stream);
    }
  }
  else r = "Expunge ignored on readonly mailbox";
  if (LOCAL && !stream->silent) mm_log(r,NIL);
}

/* Scoms1 mail copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 *	    copy options
 * Returns: T if copy successful, else NIL
 */

long
scoms1_copy(MAILSTREAM *stream,char *sequence,char *mailbox,long options)
{
  unsigned long i;
  MESSAGECACHE *elt;
  MESSAGECACHE telt;
  char emsg[256];
  char *str;
  int len;
  int hdrlen;
  int bodylen;
  char date[50];
  char sender[MSCSBUFSIZ];
  int flags;

  if (!((options & CP_UID) ? mail_uid_sequence(stream,sequence) :
	mail_sequence(stream,sequence))) return NIL;
				/* make sure valid mailbox */
  if (!scoms1_valid(mailbox)) switch (errno) {
  case ENOENT:			/* no such file? */
    mm_notify(stream,"[TRYCREATE] Must create mailbox before copy",NIL);
    return NIL;
  case EINVAL:
    sprintf(LOCAL->buf,"Invalid MS1-format mailbox name: %s",mailbox);
    mm_log(LOCAL->buf,ERROR);
    return NIL;
  default:
    sprintf(LOCAL->buf,"Not a MS1-format mailbox: %s",mailbox);
    mm_log(LOCAL->buf,ERROR);
    return NIL;
  }
  emsg[0] = 0;
  mm_critical(stream);		/* go critical */
				/* write all requested messages to mailbox */
  for (i = 1; i <= stream->nmsgs; i++) if (mail_elt(stream,i)->sequence) {
				/* write message */
    flags = scomsc1_getflags(LOCAL->handle, i);
    /* get header and body and sizes */
    hdrlen = scomsc1_fetchsize(LOCAL->handle, i, 0);
    bodylen = scomsc1_fetchsize(LOCAL->handle, i, 1);
    len = hdrlen + bodylen;
    if (len > LOCAL->buflen) {	/* make sure enough buffer space */
      if (LOCAL->buf)
        fs_give((void **) &LOCAL->buf);
      LOCAL->buf = (char *) fs_get((LOCAL->buflen = len) + 1);
    }
    str = scomsc1_fetch(LOCAL->handle, LOCAL->buf, i, 0);
    sprintf(emsg,"Message copy failed: %s", strerror(errno));
    if (str == 0)
      break;
    str = scomsc1_fetch(LOCAL->handle, LOCAL->buf + hdrlen, i, 1);
    if (str == 0)
      break;
    if (scomsc1_fetchdate(LOCAL->handle, i, &telt) == 0)
      break;
    mail_cdate(date, &telt);
    date[strlen(date)-1] = 0;
    scomsc1_fetchsender(LOCAL->handle, i, sender);
    flags |= fOLD;	/* old always on for append */
    if (scomsc1_deliver(mailbox, sender, date, LOCAL->buf, len, flags) != 1)
      break;
    emsg[0] = 0;
  }
  mm_nocritical(stream);	/* release critical */
  if (emsg[0]) {		/* was there an error? */
    mm_log(emsg,ERROR);		/* log the error */
    return NIL;			/* failed */
  }
				/* delete if requested message */
  if (options & CP_MOVE) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt(stream,i))->sequence) {
      elt->deleted = T;		/* mark message deleted */
				/* recalculate Status/X-Status lines */
      scoms1_update_status(stream, elt);
    }
  return T;
}

/* Scoms1 mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    initial flags
 *	    internal date
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

#define BUFLEN MAILTMPLEN

long
scoms1_append(MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		    STRING *message)
{
  long f;
  MESSAGECACHE elt;
  char buf[BUFLEN];
  char sender[BUFLEN];
  char datebuf[BUFLEN];
  int c;
  char *cp;
  char *ocp;
  unsigned long size;
  unsigned long uf;
  long t;
  int ok = 1;
  char *bufp;
  int newsize;
  
  f = mail_parse_flags(stream ? stream : &scoms1proto,flags,&uf);
  size = SIZE(message);
				/* parse date if given */
  if (date && !mail_parse_date(&elt,date)) {
    sprintf(buf,"Bad date in append: %s",date);
    mm_log(buf,ERROR);
    return NIL;
  }
				/* make sure valid mailbox */
  if (!scomsc1_valid(mailbox)) switch (errno) {
  case ENOENT:			/* no such file? */
    if (strcmp(ucase(strcpy(buf,mailbox)),"INBOX")) {
      mm_notify(stream,"[TRYCREATE] Must create mailbox before append",NIL);
      return NIL;
    }
    else break;
  case EINVAL:
    sprintf(buf,"Invalid MS1-format mailbox name: %s",mailbox);
    mm_log(buf,ERROR);
    return NIL;
  default:
    sprintf(buf,"Not a MS1-format mailbox: %s",mailbox);
    mm_log(buf,ERROR);
    return NIL;
  }
  mm_critical(stream);		/* go critical */
				/* write the date given */
  if (date) {
    mail_cdate(datebuf,&elt);
  }
  else {
    t = time(0);
    strcpy(datebuf,ctime(&t));	/* otherwise write the time now */
  }
  datebuf[strlen(datebuf)-1] = 0;

  /* copy message to tmp buffer, skipping cr's */
  bufp = (char *) fs_get(size + 1);
  if (bufp == 0)
    return NIL;
  ocp = bufp;
  newsize = 0;
  while (size--) {
  	c = SNX(message);
  	if (c == '\r')
  		continue;
  	*ocp++ = c;
  	newsize++;
  }
  *ocp = 0;

  /* make an envelope sender from the "From:" line */
  strcpy(sender, "unknown");
  cp = bufp;
  for (cp = bufp; *cp; cp = ocp) {
    ocp = strchr(cp, '\n');
    if (ocp == 0)
      ocp = ocp + strlen(ocp);
    c = *ocp;
    *ocp = 0;
    if (msc1_strhccmp("From", cp, ':') == 0) {
      cp = strchr(cp, ':') + 1;
      while ((*cp == ' ') || (*cp == '\t'))
        cp++;
      if (*cp)
	      msc1_parse_address(sender, cp);
      *ocp = c;
      break;
    }
    *ocp = c;
    if (*ocp)
      ocp++;
  }

  f |= fOLD;	/* old always on for append */
  if (scomsc1_deliver(mailbox, sender, datebuf, bufp, newsize, f) != 1) {
    sprintf(buf,"Message append failed: %s", strerror(errno));
    mm_log(buf,ERROR);
    ok = NIL;
  }
  fs_give((void **)&bufp);
  mm_nocritical(stream);	/* release critical */
  return ok;			/* return success */
}

/* Scoms1 mail deliver message from file pointer
 * Accepts: MAIL stream
 *	    internal date
 *	    envelope sender
 *	    stringstruct of messages to append
 * Returns: 1 if successful, 0 - error
 * almost the same as append but has envelope sender and no flags
 * also does not require message to be in core.
 */

long
scoms1_deliver(MAILSTREAM *stream, char *mailbox,
		    char *realsender, FILE *fp)
{
  MESSAGECACHE elt;
  char buf[BUFLEN];
  char datebuf[BUFLEN];
  char sender[BUFLEN];
  long t;
  int ok = 1;
  
				/* make sure valid mailbox */
  if (!scomsc1_valid(mailbox)) switch (errno) {
  case ENOENT:			/* no such file? */
    break;
  case EINVAL:
    sprintf(buf,"Invalid MS1-format mailbox name: %s",mailbox);
    mm_log(buf,ERROR);
    return NIL;
  default:
    sprintf(buf,"Not a MS1-format mailbox: %s",mailbox);
    mm_log(buf,ERROR);
    return NIL;
  }
  mm_critical(stream);		/* go critical */
  if (realsender)
    strcpy(sender, realsender);
  else
    sprintf(sender,"%s@%s",myusername(),mylocalhost());

  t = time(0);
  strcpy(datebuf,ctime(&t));	/* write the time now */
  datebuf[strlen(datebuf)-1] = 0;

  if (scomsc1_deliverf(mailbox, sender, datebuf, fp, 0) != 1) {
    sprintf(buf,"Message append failed: %s", strerror(errno));
    mm_log(buf,ERROR);
    ok = 0;
  }
  mm_nocritical(stream);	/* release critical */
  return ok;			/* return success */
}

/* Scoms1 garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void
scoms1_gc(MAILSTREAM *stream,long gcflags)
{
  /* nothing here for now */
}

/* Internal routines */


/* Scoms1 mail abort stream
 * Accepts: MAIL stream
 */

void
scoms1_abort(MAILSTREAM *stream)
{
  if (LOCAL) {			/* only if a file is open */
    if (LOCAL->handle)
      scomsc1_close(LOCAL->handle);
				/* free local text buffers */
    if (LOCAL->buf)
      fs_give((void **) &LOCAL->buf);
				/* nuke the local data */
    fs_give((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}

/* Scoms1 mail parse and lock mailbox
 * Accepts: MAIL stream
 * Returns: 1 if OK
 *	    0 if failure, stream aborted
 */

int
scoms1_parse(MAILSTREAM *stream)
{
  mailcache_t mc;
  scomsc1_stat_t *finfo;
  MESSAGECACHE *elt;
  unsigned long newcnt;
  int flags;
  unsigned long nmsgs;
  int i;

  mc = (mailcache_t) mail_parameters(NIL,GET_CACHE,NIL);
  mail_lock(stream);		/* guard against recursion or pingers */

				/* calculate change in size */
  finfo = scomsc1_check(LOCAL->handle);
  if (finfo == 0) {
    scoms1_abort(stream);
    return(0);
  }
  nmsgs = finfo->m_msgs;
  (*mc)(stream,nmsgs,CH_SIZE);	/* expand the primary cache */
  newcnt = 0;
  for (i = stream->nmsgs; i < nmsgs; i++) {
				/* instantiate elt */
    (elt = mail_elt(stream,i+1))->valid = T;
				/* set internal date */
    if (scomsc1_fetchdate(LOCAL->handle, i+1, elt) == 0) {
      scoms1_abort(stream);
      return(0);
    }
    elt->uid = scomsc1_fetchuid(LOCAL->handle, i+1);
    if (elt->uid == 0) {
      scoms1_abort(stream);
      return(0);
    }
    stream->uid_last = elt->uid;
    flags = scomsc1_getflags(LOCAL->handle, i+1);
    if (flags == -1) {
      scoms1_abort(stream);
      return(0);
    }
    elt->seen = (flags&fSEEN) ? 1 : 0;
    elt->deleted = (flags&fDELETED) ? 1 : 0;
    elt->flagged = (flags&fFLAGGED) ? 1 : 0;
    elt->answered = (flags&fANSWERED) ? 1 : 0;
    elt->draft = (flags&fDRAFT) ? 1 : 0;
    elt->recent = (flags&fOLD) ? 0 : 1;
    /* size includes CRLF which are added later if FT_INTERNAL not specified */
    elt->rfc822_size = scomsc1_fetchsize(LOCAL->handle, i+1, 0) +
    			scomsc1_fetchlines(LOCAL->handle, i+1, 0) +
    			scomsc1_fetchsize(LOCAL->handle, i+1, 1) +
    			scomsc1_fetchlines(LOCAL->handle, i+1, 1);
    if (elt->recent)
      newcnt++;
  }
  mail_exists(stream,nmsgs);	/* notify upper level of new mailbox size */
  mail_recent(stream,stream->recent + newcnt);
  stream->uid_validity = finfo->m_validity;
  return(1);			/* return the winnage */
}

/* Scoms1 update local status
 * Accepts: message cache entry
 *	    source elt
 */

void
scoms1_update_status(MAILSTREAM *stream, MESSAGECACHE *elt)
{
  int flags;

  flags = scomsc1_getflags(LOCAL->handle, elt->msgno)&fOLD;	/* keep fOLD */
  flags |= elt->seen ? fSEEN : 0;		/* system Seen flag */
  flags |= elt->deleted ? fDELETED : 0;		/* system Deleted flag */
  flags |= elt->flagged ? fFLAGGED : 0; 	/* system Flagged flag */
  flags |= elt->answered ? fANSWERED : 0;	/* system Answered flag */
  flags |= elt->draft ? fDRAFT : 0;		/* system Draft flag */
  scomsc1_setflags(LOCAL->handle, elt->msgno, flags);
}
