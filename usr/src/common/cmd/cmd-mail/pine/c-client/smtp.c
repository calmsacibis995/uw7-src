/*
 * Program:	Simple Mail Transfer Protocol (SMTP) routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	27 July 1988
 * Last Edited:	28 June 1996
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1994 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington or The
 * Leland Stanford Junior University not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written prior
 * permission.  This software is made available "as is", and
 * THE UNIVERSITY OF WASHINGTON AND THE LELAND STANFORD JUNIOR UNIVERSITY
 * DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO THIS SOFTWARE,
 * INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL THE UNIVERSITY OF
 * WASHINGTON OR THE LELAND STANFORD JUNIOR UNIVERSITY BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <ctype.h>
#include <stdio.h>
#include "mail.h"
#include "osdep.h"
#include "smtp.h"
#include "rfc822.h"
#include "misc.h"


/* Mailer parameters */

long smtp_port = 0;		/* default port override */

/* Mail Transfer Protocol open connection
 * Accepts: service host list
 *	    SMTP open options
 * Returns: T on success, NIL on failure
 */

SMTPSTREAM *smtp_open (hostlist,options)
	char **hostlist;
	long options;
{
  SMTPSTREAM *stream = NIL;
  char *s,tmp[MAILTMPLEN];
  void *tcpstream;
  if (!(hostlist && *hostlist)) mm_log ("Missing SMTP service host",ERROR);
  else do {			/* try to open connection */
    if (smtp_port) sprintf (s = tmp,"%s:%ld",*hostlist,smtp_port);
    else s = *hostlist;		/* get server name */
    if (tcpstream = tcp_open (s,"smtp",SMTPTCPPORT)) {
      stream = (SMTPSTREAM *) fs_get (sizeof (SMTPSTREAM));
      stream->tcpstream = tcpstream;
      if(smtp_greeting (stream,
		     strcmp ("localhost",(char*)lcase(strcpy(tmp,*hostlist)))
			      ? tcp_localhost (tcpstream) : "localhost",
		     options))
	return stream;
      smtp_close (stream);	/* otherwise punt stream */
    }
  } while (*++hostlist);	/* try next server */
  return NIL;
}


/* Mail Transfer Protocol salutation
 * Accepts: local host name
 *	    SMTP open options
 * Returns: T on success, NIL on failure
 */
long smtp_greeting (stream,lhost,options)
	SMTPSTREAM *stream;
	char *lhost;
	long options;
{
  stream->size = 0;		/* size limit */
  stream->debug = (options & SOP_DEBUG) ? T : NIL;
  stream->esmtp = (options & SOP_ESMTP) ? T : NIL;
  stream->ok_send = stream->ok_soml = stream->ok_saml = stream->ok_expn =
    stream->ok_help = stream->ok_turn = stream->ok_size =
      stream->ok_8bitmime = NIL;
  stream->reply = NIL;
				/* get SMTP greeting */
  if (smtp_reply (stream) == SMTPGREET) {
    if ((stream->ehlo = stream->esmtp) &&
	(smtp_send (stream,"EHLO",lhost) == SMTPOK)) return T;
				/* try ordinary SMTP then */
    stream->ehlo = stream->esmtp = NIL;
    if (smtp_send (stream,"HELO",lhost) == SMTPOK) return T;
  }
  mm_log (stream->reply,ERROR);
  return NIL;
}
/* Mail Transfer Protocol close connection
 * Accepts: stream
 * Returns: NIL always
 */

SMTPSTREAM *smtp_close (stream)
	SMTPSTREAM *stream;
{
  if (stream) {			/* send "QUIT" */
    smtp_send (stream,"QUIT",NIL);
				/* close TCP connection */
    (* (postclose_t) mail_parameters (NIL,GET_POSTCLOSE,NIL)) (stream->tcpstream);
    if (stream->reply) fs_give ((void **) &stream->reply);
    fs_give ((void **) &stream);/* flush the stream */
  }
  return NIL;
}

/* Mail Transfer Protocol deliver mail
 * Accepts: stream
 *	    delivery option (MAIL, SEND, SAML, SOML)
 *	    message envelope
 *	    message body
 * Returns: T on success, NIL on failure
 */

long smtp_mail (stream,type,env,body)
	SMTPSTREAM *stream;
	char *type;
	ENVELOPE *env;
	BODY *body;
{
  char tmp[8*MAILTMPLEN];
  long error = NIL;
  rfc822emit_t f;
  if (!(env->to || env->cc || env->bcc)) {
  				/* no recipients in request */
    smtp_fake (stream,SMTPHARDERROR,"No recipients specified");
    return NIL;
  }
  				/* make sure stream is in good shape */
  smtp_send (stream,"RSET",NIL);
  strcpy (tmp,"FROM:<");	/* compose "MAIL FROM:<return-path>" */
  rfc822_address (tmp,env->return_path);
  strcat (tmp,">");
  if (stream->ok_8bitmime) strcat (tmp," BODY=8BITMIME");
				/* send "MAIL FROM" command */
  if (!(smtp_send (stream,type,tmp) == SMTPOK)) return NIL;
				/* negotiate the recipients */
  if (env->to) smtp_rcpt (stream,env->to,&error);
  if (env->cc) smtp_rcpt (stream,env->cc,&error);
  if (env->bcc) smtp_rcpt (stream,env->bcc,&error);
  if (error) {			/* any recipients failed? */
      				/* reset the stream */
    smtp_send (stream,"RSET",NIL);
    smtp_fake (stream,SMTPHARDERROR,"One or more recipients failed");
    return NIL;
  }
				/* negotiate data command */
  if (!(smtp_send (stream,"DATA",NIL) == SMTPREADY)) return NIL;
				/* set up error in case failure */
  smtp_fake (stream,SMTPSOFTFATAL,"SMTP connection went away!");
				/* encode body as necessary */
  if((f = mail_parameters (NIL,GET_RFC822OUTPUT,NIL)) == (rfc822emit_t) rfc822_output){
				/* encode body as necessary */
      if (stream->ok_8bitmime) rfc822_encode_body_8bit (env,body);
      else rfc822_encode_body_7bit (env,body);
  }
				/* output data, return success status */
  return (*f) (tmp,env,body,smtp_soutr,stream->tcpstream) &&
    (smtp_send (stream,".",NIL) == SMTPOK);
}

/* Mail Transfer Protocol turn on debugging telemetry
 * Accepts: stream
 */

void smtp_debug (stream)
	SMTPSTREAM *stream;
{
  stream->debug = T;		/* turn on protocol telemetry */
}


/* Mail Transfer Protocol turn off debugging telemetry
 * Accepts: stream
 */

void smtp_nodebug (stream)
	SMTPSTREAM *stream;
{
  stream->debug = NIL;		/* turn off protocol telemetry */
}

/* Internal routines */


/* Simple Mail Transfer Protocol send recipient
 * Accepts: SMTP stream
 *	    address list
 *	    pointer to error flag
 */

void smtp_rcpt (stream,adr,error)
	SMTPSTREAM *stream;
	ADDRESS *adr;
	long *error;
{
  char tmp[MAILTMPLEN];
  while (adr) {
				/* clear any former error */
    if (adr->error) fs_give ((void **) &adr->error);
    if (adr->host) {		/* ignore group syntax */
      strcpy (tmp,"TO:<");	/* compose "RCPT TO:<return-path>" */
      rfc822_address (tmp,adr);
      strcat (tmp,">");
				/* send "RCPT TO" command */
      if (!(smtp_send (stream,"RCPT",tmp) == SMTPOK)) {
	*error = T;		/* note that an error occurred */
	adr->error = cpystr (stream->reply);
      }
    }
    adr = adr->next;		/* do any subsequent recipients */
  }
}


/* Simple Mail Transfer Protocol send command
 * Accepts: SMTP stream
 *	    text
 * Returns: reply code
 */

long smtp_send (stream,command,args)
	SMTPSTREAM *stream;
	char *command;
	char *args;
{
  char tmp[MAILTMPLEN];
				/* build the complete command */
  if (args) sprintf (tmp,"%s %s",command,args);
  else strcpy (tmp,command);
  if (stream->debug) mm_dlog (tmp);
  strcat (tmp,"\015\012");
				/* send the command */
  return (*(postsoutr_t) mail_parameters (NIL,GET_POSTSOUTR,NIL))
							(stream->tcpstream,tmp)
	   ? smtp_reply (stream)
	   : smtp_fake (stream,SMTPSOFTFATAL,"SMTP connection went away!");
}

/* Simple Mail Transfer Protocol get reply
 * Accepts: SMTP stream
 * Returns: reply code
 */

long smtp_reply (stream)
	SMTPSTREAM *stream;
{
  unsigned long i,j;
  postgetline_t getline =
    (postgetline_t) mail_parameters (NIL,GET_POSTGETLINE,NIL);
  postverbose_t pv;
				/* flush old reply */
  if (stream->reply) fs_give ((void **) &stream->reply);
  				/* get reply */
  if (!(stream->reply = (*getline) (stream->tcpstream)))
    return smtp_fake (stream,SMTPSOFTFATAL,"SMTP connection went away!");
  if (stream->debug) mm_dlog (stream->reply);
				/* got an OK reply? */
  if (((i = atoi (stream->reply)) == SMTPOK) && stream->ehlo) {
    char tmp[MAILTMPLEN];	/* yes, make uppercase copy of response text */
    ucase (strcpy (tmp,stream->reply+4));
				/* command name */
    j = (((long) tmp[0]) << 24) + (((long) tmp[1]) << 16) +
      (((long) tmp[2]) << 8) + tmp[3];
				/* defined by SMTP 8bit-MIMEtransport */
    if (j == (((long) '8' << 24) + ((long) 'B' << 16) + ('I' << 8) + 'T') &&
	tmp[4] == 'M' && tmp[5] == 'I' && tmp[6] == 'M' && tmp[7] == 'E' &&
	!tmp[8]) stream->ok_8bitmime = T;
				/* defined by SMTP Size Declaration */
    else if (j == (((long) 'S' << 24) + ((long) 'I' << 16) + ('Z' << 8) + 'E')
	     && (!tmp[4] || tmp[4] == ' ')) {
      if (tmp[4]) stream->size = atoi (tmp+5);
      stream->ok_size = T;
    }
				/* defined by SMTP Service Extensions */
    else if (j == (((long) 'S' << 24) + ((long) 'E' << 16) + ('N' << 8) + 'D')
	     && !tmp[4]) stream->ok_send = T;
    else if (j == (((long) 'S' << 24) + ((long) 'O' << 16) + ('M' << 8) + 'L')
	     && !tmp[4]) stream->ok_soml = T;
    else if (j == (((long) 'S' << 24) + ((long) 'A' << 16) + ('M' << 8) + 'L')
	     && !tmp[4]) stream->ok_saml = T;
    else if (j == (((long) 'E' << 24) + ((long) 'X' << 16) + ('P' << 8) + 'N')
	     && !tmp[4]) stream->ok_expn = T;
    else if (j == (((long) 'H' << 24) + ((long) 'E' << 16) + ('L' << 8) + 'P')
	     && !tmp[4]) stream->ok_help = T;
    else if (j == (((long) 'T' << 24) + ((long) 'U' << 16) + ('R' << 8) + 'N')
	     && !tmp[4]) stream->ok_turn = T;
  }
  else if (i < 100 && (pv = mail_parameters (NIL,GET_POSTVERBOSE,NIL))){
    (*pv) (stream->reply);
    return smtp_reply(stream);
  }
				/* handle continuation by recursion */
  if (stream->reply[3]=='-') return smtp_reply (stream);
  stream->ehlo = NIL;		/* not doing EHLO any more */
  return i;			/* return the response code */
}

/* Simple Mail Transfer Protocol set fake error
 * Accepts: SMTP stream
 *	    SMTP error code
 *	    error text
 * Returns: error code
 */

long smtp_fake (stream,code,text)
	SMTPSTREAM *stream;
	long code;
	char *text;
{
				/* flush any old reply */
  if (stream->reply ) fs_give ((void **) &stream->reply);
  				/* set up pseudo-reply string */
  stream->reply = (char *) fs_get (20+strlen (text));
  sprintf (stream->reply,"%ld %s",code,text);
  return code;			/* return error code */
}


/* Simple Mail Transfer Protocol filter mail
 * Accepts: stream
 *	    string
 * Returns: T on success, NIL on failure
 */

long smtp_soutr (stream,s)
	void *stream;
	char *s;
{
  char c,*t;
  postsoutr_t soutr =
    (postsoutr_t) mail_parameters (NIL, GET_POSTSOUTR, 0);
				/* "." on first line */
  if (s[0] == '.') (*soutr) (stream,".");
				/* find lines beginning with a "." */
  while (t = strstr (s,"\015\012.")) {
    c = *(t += 3);		/* remember next character after "." */
    *t = '\0';			/* tie off string */
				/* output prefix */
    if (!(*soutr) (stream,s)) return NIL;
    *t = c;			/* restore delimiter */
    s = t - 1;			/* push pointer up to the "." */
  }
				/* output remainder of text */
  return *s ? (*soutr) (stream,s) : T;
}
