/*
 * Program:	UNIX TCP/IP routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	1 August 1988
 * Last Edited:	21 February 1997
 *
 * Copyright 1997 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made available
 * "as is", and
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

#undef write			/* don't use redefined write() */

				/* TCP timeout handler routine */
static tcptimeout_t tcptimeout = NIL;
				/* TCP timeouts, in seconds */
static long tcptimeout_open = 0;
static long tcptimeout_read = 0;
static long tcptimeout_write = 0;
static long rshtimeout = 15;	/* rsh timeout */

/* TCP/IP manipulate parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *tcp_parameters (function,value)
	long function;
	void *value;
{
  switch ((int) function) {
  case SET_TIMEOUT:
    tcptimeout = (tcptimeout_t) value;
    break;
  case GET_TIMEOUT:
    value = (void *) tcptimeout;
    break;
  case SET_OPENTIMEOUT:
    tcptimeout_open = (long) value;
    break;
  case GET_OPENTIMEOUT:
    value = (void *) tcptimeout_open;
    break;
  case SET_READTIMEOUT:
    tcptimeout_read = (long) value;
    break;
  case GET_READTIMEOUT:
    value = (void *) tcptimeout_read;
    break;
  case SET_WRITETIMEOUT:
    tcptimeout_write = (long) value;
    break;
  case GET_WRITETIMEOUT:
    value = (void *) tcptimeout_write;
    break;
  case SET_RSHTIMEOUT:
    rshtimeout = (long) value;
    break;
  case GET_RSHTIMEOUT:
    value = (void *) rshtimeout;
    break;
  default:
    value = NIL;		/* error case */
    break;
  }
  return value;
}

/* TCP/IP open
 * Accepts: host name
 *	    contact service name
 *	    contact port number
 * Returns: TCP/IP stream if success else NIL
 */

TCPSTREAM *tcp_open (host,service,port)
	char *host;
	char *service;
	long port;
{
  TCPSTREAM *stream = NIL;
  int i,sock,flgs;
  char *s;
  struct sockaddr_in sin;
  struct hostent *host_name;
  char hostname[MAILTMPLEN];
  char tmp[MAILTMPLEN];
  fd_set fds,efds;
  struct protoent *pt = getprotobyname ("ip");
  struct servent *sv = service ? getservbyname (service,"tcp") : NIL;
  struct timeval tmo;
  tmo.tv_sec = tcptimeout_open;
  tmo.tv_usec = 0;
  FD_ZERO (&fds);		/* initialize selection vector */
  FD_ZERO (&efds);		/* handle errors too */
  if (s = strchr (host,':')) {	/* port number specified? */
    *s++ = '\0';		/* yes, tie off port */
    port = strtol (s,&s,10);	/* parse port */
    if (s && *s) {
      sprintf (tmp,"Junk after port number: %.80s",s);
      mm_log (tmp,ERROR);
      return NIL;
    }
    sin.sin_port = htons (port);
  }
				/* copy port number in network format */
  else sin.sin_port = sv ? sv->s_port : htons (port);

  /* The domain literal form is used (rather than simply the dotted decimal
     as with other Unix programs) because it has to be a valid "host name"
     in mailsystem terminology. */
				/* look like domain literal? */
  if (host[0] == '[' && host[(strlen (host))-1] == ']') {
    strcpy (hostname,host+1);	/* yes, copy number part */
    hostname[(strlen (hostname))-1] = '\0';
    if ((sin.sin_addr.s_addr = inet_addr (hostname)) != -1) {
      sin.sin_family = AF_INET;	/* family is always Internet */
      strcpy (hostname,host);	/* hostname is user's argument */
    }
    else {
      sprintf (tmp,"Bad format domain-literal: %.80s",host);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
  else {			/* lookup host name, note that brain-dead Unix
				   requires lowercase! */
    strcpy (hostname,host);	/* in case host is in write-protected memory */
    i = (int) alarm (0);	/* quell alarms */
    if ((host_name = gethostbyname (lcase (hostname)))) {
      alarm (i);		/* restore alarms */
				/* copy address type */
      sin.sin_family = host_name->h_addrtype;
				/* copy host name */
      strcpy (hostname,host_name->h_name);
				/* copy host addresses */
      memcpy (&sin.sin_addr,host_name->h_addr,host_name->h_length);
    }
    else {
      alarm (i);		/* restore alarms */
      sprintf (tmp,"No such host as %.80s",host);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }

				/* get a TCP stream */
  if ((sock = socket (sin.sin_family,SOCK_STREAM,pt ? pt->p_proto : 0)) < 0) {
    sprintf (tmp,"Unable to create TCP socket: %s",strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  flgs = fcntl (sock,F_GETFL,0);/* get current socket flags */
  fcntl (sock,F_SETFL,flgs | FNDELAY);
				/* open connection */
  while ((i = connect (sock,(struct sockaddr *) &sin,sizeof (sin))) < 0 &&
	 errno == EINTR);
  if (i < 0) switch (errno) {	/* failed? */
  case EAGAIN:			/* DG brain damage */
  case EINPROGRESS:
  case EISCONN:
  case EADDRINUSE:
    break;			/* well, not really, it was interrupted */
  default:
    sprintf (tmp,"Can't connect to %.80s,%d: %s",hostname,port,
	     strerror (errno));
    mm_log (tmp,ERROR);
    close (sock);		/* flush socket */
    return NIL;
  }
  FD_SET (sock,&fds);		/* block for error or writeable */
  FD_SET (sock,&efds);
  while (((i = select (sock+1,0,&fds,&efds,tmo.tv_sec ? &tmo : 0)) < 0) &&
	 (errno == EINTR));
  if (i > 0) {			/* success, make sure really connected */
    fcntl (sock,F_SETFL,flgs);	/* restore blocking status */
#ifndef SOLARISKERNELBUG
				/* get socket status */
    while ((i = read (sock,tmp,0)) < 0 && errno == EINTR);
    if (!i) i = 1;		/* make success if the read is OK */
#endif
  }
  if (i <= 0) {			/* timeout or error? */
    sprintf (tmp,"Can't connect to %.80s,%d: %s",hostname,port,
	     strerror (i ? errno : ETIMEDOUT));
    mm_log (tmp,ERROR);
    close (sock);		/* flush socket */
    return NIL;
  }

				/* create TCP/IP stream */
  stream = (TCPSTREAM *) fs_get (sizeof (TCPSTREAM));
				/* copy official host name */
  stream->host = cpystr (hostname);
				/* get local name */
  gethostname (tmp,MAILTMPLEN-1);
  stream->localhost = cpystr ((host_name = gethostbyname (tmp)) ?
			      host_name->h_name : tmp);
  stream->port = port;		/* port number */
				/* init sockets */
  stream->tcpsi = stream->tcpso = sock;
  stream->ictr = 0;		/* init input counter */
  return stream;		/* return success */
}

/* TCP/IP authenticated open
 * Accepts: host name
 *	    service name
 *	    returned user name buffer
 * Returns: TCP/IP stream if success else NIL
 */

TCPSTREAM *tcp_aopen (host,service,usrbuf)
	char *host;
	char *service;
	char *usrbuf;
{
  TCPSTREAM *stream = NIL;
  struct hostent *host_name;
  char *user = (char *) mail_parameters (NIL,GET_USERNAMEBUF,NIL);
  char hostname[MAILTMPLEN];
  int i;
  int pipei[2],pipeo[2];
  struct timeval tmo;
  fd_set fds,efds;
  if (!(tmo.tv_sec = rshtimeout)) return NIL;
  tmo.tv_usec = 0;
  FD_ZERO (&fds);		/* initialize selection vector */
  FD_ZERO (&efds);		/* handle errors too */
  /* The domain literal form is used (rather than simply the dotted decimal
     as with other Unix programs) because it has to be a valid "host name"
     in mailsystem terminology. */
				/* look like domain literal? */
  if (host[0] == '[' && host[i = (strlen (host))-1] == ']') {
    strcpy (hostname,host+1);/* yes, copy without brackets */
    hostname[i-1] = '\0';
  }
				/* note that Unix requires lowercase! */
  else if (host_name = gethostbyname (lcase (strcpy (hostname,host))))
    strcpy (hostname,host_name->h_name);
				/* make command pipes */
  if (pipe (pipei) < 0) return NIL;
  if (pipe (pipeo) < 0) {
    close (pipei[0]); close (pipei[1]);
    return NIL;
  }
  if ((i = fork ()) < 0) {	/* make inferior process */
    close (pipei[0]); close (pipei[1]);
    close (pipeo[0]); close (pipeo[1]);
    return NIL;
  }
  if (!i) {			/* if child */
    if (!fork ()) {		/* make grandchild so it's inherited by init */
      int maxfd = max (20,max (max(pipei[0],pipei[1]),max(pipeo[0],pipeo[1])));
      dup2 (pipei[1],1);	/* parent's input is my output */
      dup2 (pipei[1],2);	/* parent's input is my error output too */
      dup2 (pipeo[0],0);	/* parent's output is my input */
				/* close all unnecessary descriptors */
      for (i = 3; i <= maxfd; i++) close (i);
      setpgrp (0,getpid ());	/* be our own process group */
      if (user && *user)	/* now run it */
	execl (RSHPATH,RSH,hostname,"-l",user,"exec",service,0);
      execl (RSHPATH,RSH,hostname,"exec",service,0);
    }
    _exit (1);			/* child is done */
  }

  grim_pid_reap (i,NIL);	/* reap child; grandchild now owned by init */
  close (pipei[1]);		/* close child's side of the pipes */
  close (pipeo[0]);
				/* create TCP/IP stream */
  stream = (TCPSTREAM *) fs_get (sizeof (TCPSTREAM));
				/* copy official host name */
  stream->host = cpystr (hostname);
				/* get local name */
  gethostname (hostname,MAILTMPLEN-1);
  stream->localhost = cpystr ((host_name = gethostbyname (hostname)) ?
			      host_name->h_name : hostname);
  stream->tcpsi = pipei[0];	/* init sockets */
  stream->tcpso = pipeo[1];
  stream->ictr = 0;		/* init input counter */
  stream->port = 0xffffffff;	/* no port number */
  FD_SET (stream->tcpsi,&fds);	/* set bit in selection vector */
  FD_SET (stream->tcpsi,&efds);	/* set bit in error selection vector */
  while (((i = select (stream->tcpsi+1,&fds,0,&efds,&tmo)) < 0) &&
	 (errno == EINTR));
  if (i <= 0) {			/* timeout or error? */
    mm_log (i ? "error in rsh to IMAP server" : "rsh to IMAP server timed out",
	    WARN);
    tcp_close (stream);		/* punt stream */
    stream = NIL;
  }
				/* return user name */
  strcpy (usrbuf,(user && *user) ? user : myusername ());
  return stream;		/* return success */
}

/* TCP/IP receive line
 * Accepts: TCP/IP stream
 * Returns: text line string or NIL if failure
 */

char *tcp_getline (stream)
	TCPSTREAM *stream;
{
  int n,m;
  char *st,*ret,*stp;
  char c = '\0';
  char d;
				/* make sure have data */
  if (!tcp_getdata (stream)) return NIL;
  st = stream->iptr;		/* save start of string */
  n = 0;			/* init string count */
  while (stream->ictr--) {	/* look for end of line */
    d = *stream->iptr++;	/* slurp another character */
    if ((c == '\015') && (d == '\012')) {
      ret = (char *) fs_get (n--);
      memcpy (ret,st,n);	/* copy into a free storage string */
      ret[n] = '\0';		/* tie off string with null */
      return ret;
    }
    n++;			/* count another character searched */
    c = d;			/* remember previous character */
  }
				/* copy partial string from buffer */
  memcpy ((ret = stp = (char *) fs_get (n)),st,n);
				/* get more data from the net */
  if (!tcp_getdata (stream)) fs_give ((void **) &ret);
				/* special case of newline broken by buffer */
  else if ((c == '\015') && (*stream->iptr == '\012')) {
    stream->iptr++;		/* eat the line feed */
    stream->ictr--;
    ret[n - 1] = '\0';		/* tie off string with null */
  }
				/* else recurse to get remainder */
  else if (st = tcp_getline (stream)) {
    ret = (char *) fs_get (n + 1 + (m = strlen (st)));
    memcpy (ret,stp,n);		/* copy first part */
    memcpy (ret + n,st,m);	/* and second part */
    fs_give ((void **) &stp);	/* flush first part */
    fs_give ((void **) &st);	/* flush second part */
    ret[n + m] = '\0';		/* tie off string with null */
  }
  return ret;
}

/* TCP/IP receive buffer
 * Accepts: TCP/IP stream
 *	    size in bytes
 *	    buffer to read into
 * Returns: T if success, NIL otherwise
 */

long tcp_getbuffer (stream,size,buffer)
	TCPSTREAM *stream;
	unsigned long size;
	char *buffer;
{
  unsigned long n;
  char *bufptr = buffer;
  while (size > 0) {		/* until request satisfied */
    if (!tcp_getdata (stream)) return NIL;
    n = min (size,stream->ictr);/* number of bytes to transfer */
				/* do the copy */
    memcpy (bufptr,stream->iptr,n);
    bufptr += n;		/* update pointer */
    stream->iptr +=n;
    size -= n;			/* update # of bytes to do */
    stream->ictr -=n;
  }
  bufptr[0] = '\0';		/* tie off string */
  return T;
}

/* TCP/IP receive data
 * Accepts: TCP/IP stream
 * Returns: T if success, NIL otherwise
 */

long tcp_getdata (stream)
	TCPSTREAM *stream;
{
  int i;
  fd_set fds,efds;
  struct timeval tmo;
  time_t t = time (0);
  tmo.tv_sec = tcptimeout_read;
  tmo.tv_usec = 0;
  FD_ZERO (&fds);		/* initialize selection vector */
  FD_ZERO (&efds);		/* handle errors too */
  if (stream->tcpsi < 0) return NIL;
  while (stream->ictr < 1) {	/* if nothing in the buffer */
    FD_SET (stream->tcpsi,&fds);/* set bit in selection vector */
    FD_SET(stream->tcpsi,&efds);/* set bit in error selection vector */
    errno = NIL;		/* block and read */
    while (((i = select (stream->tcpsi+1,&fds,0,&efds,tmo.tv_sec ? &tmo:0))<0)
	   && (errno == EINTR));
    if (!i) {			/* timeout? */
      if (tcptimeout && ((*tcptimeout) (time (0) - t))) continue;
      else return tcp_abort (stream);
    }
    if (i < 0) return tcp_abort (stream);
    while (((i = read (stream->tcpsi,stream->ibuf,BUFLEN)) < 0) &&
	   (errno == EINTR));
    if (i < 1) return tcp_abort (stream);
    stream->iptr = stream->ibuf;/* point at TCP buffer */
    stream->ictr = i;		/* set new byte count */
  }
  return T;
}

/* TCP/IP send string as record
 * Accepts: TCP/IP stream
 *	    string pointer
 * Returns: T if success else NIL
 */

long tcp_soutr (stream,string)
	TCPSTREAM *stream;
	char *string;
{
  return tcp_sout (stream,string,(unsigned long) strlen (string));
}


/* TCP/IP send string
 * Accepts: TCP/IP stream
 *	    string pointer
 *	    byte count
 * Returns: T if success else NIL
 */

long tcp_sout (stream,string,size)
	TCPSTREAM *stream;
	char *string;
	unsigned long size;
{
  int i;
  fd_set fds;
  struct timeval tmo;
  time_t t = time (0);
  tmo.tv_sec = tcptimeout_write;
  tmo.tv_usec = 0;
  FD_ZERO (&fds);		/* initialize selection vector */
  if (stream->tcpso < 0) return NIL;
  while (size > 0) {		/* until request satisfied */
    FD_SET (stream->tcpso,&fds);/* set bit in selection vector */
    errno = NIL;		/* block and write */
    while (((i = select (stream->tcpso+1,0,&fds,0,tmo.tv_sec ? &tmo : 0)) < 0)
	   && (errno == EINTR));
    if (!i) {			/* timeout? */
      if (tcptimeout && ((*tcptimeout) (time (0) - t))) continue;
      else return tcp_abort (stream);
    }
    if (i < 0) return tcp_abort (stream);
    while (((i = write (stream->tcpso,string,size)) < 0) &&
	   (errno == EINTR));
    if (i < 0) return tcp_abort (stream);
    size -= i;			/* how much we sent */
    string += i;
  }
  return T;			/* all done */
}

/* TCP/IP close
 * Accepts: TCP/IP stream
 */

void tcp_close (stream)
	TCPSTREAM *stream;
{
  tcp_abort (stream);		/* nuke the stream */
				/* flush host names */
  fs_give ((void **) &stream->host);
  fs_give ((void **) &stream->localhost);
  fs_give ((void **) &stream);	/* flush the stream */
}


/* TCP/IP abort stream
 * Accepts: TCP/IP stream
 * Returns: NIL always
 */

long tcp_abort (stream)
	TCPSTREAM *stream;
{
  int i;
  if (stream->tcpsi >= 0) {	/* no-op if no socket */
    close (stream->tcpsi);	/* nuke the socket */
    if (stream->tcpsi != stream->tcpso) close (stream->tcpso);
    stream->tcpsi = stream->tcpso = -1;
  }
  return NIL;
}

/* TCP/IP get host name
 * Accepts: TCP/IP stream
 * Returns: host name for this stream
 */

char *tcp_host (stream)
	TCPSTREAM *stream;
{
  return stream->host;		/* return host name */
}


/* TCP/IP return port for this stream
 * Accepts: TCP/IP stream
 * Returns: port number for this stream
 */

long tcp_port (stream)
	TCPSTREAM *stream;
{
  return stream->port;		/* return port number */
}


/* TCP/IP get local host name
 * Accepts: TCP/IP stream
 * Returns: local host name
 */

char *tcp_localhost (stream)
	TCPSTREAM *stream;
{
  return stream->localhost;	/* return local host name */
}


/* TCP/IP get server host name
 * Accepts: pointer to destination
 * Returns: string pointer if got results, else NIL
 */

char *tcp_clienthost (dst)
	char *dst;
{
  struct hostent *hn;
  struct sockaddr_in from;
  int fromlen = sizeof (from);
  if (getpeername (0,(struct sockaddr *) &from,&fromlen)) return "UNKNOWN";
  strncpy (dst,(hn = gethostbyaddr ((char *) &from.sin_addr,
				   sizeof (struct in_addr),from.sin_family)) ?
	   hn->h_name : inet_ntoa (from.sin_addr),80);
  return dst;
}
