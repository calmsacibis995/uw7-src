#ident	"@(#)R5Xlib:XConnDis.c	1.18"
/*
 * $Header$
 * $XConsortium: XConnDis.c,v 11.88 91/12/17 17:55:57 rws Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * 
 * This file contains operating system dependencies.
 */

/*
 *
 * The connection code/ideas in lib/X and server/os for SVR4/Intel 
 * environments was contributed by the following companies/groups:
 *
 *	MetroLink Inc
 *	NCR
 *	Pittsburgh Powercomputing Corporation (PPc)/Quarterdeck Office Systems
 *	SGCS
 *	Unix System Laboratories (USL) / Novell
 *	XFree86
 *
 * The goal is to have common connection code among all SVR4/Intel vendors.
 *
 * ALL THE ABOVE COMPANIES DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, 
 * IN NO EVENT SHALL THESE COMPANIES * BE LIABLE FOR ANY SPECIAL, INDIRECT 
 * OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS 
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE 
 * OR PERFORMANCE OF THIS SOFTWARE.
 */


/*
 * Copyright (c) 1993 Unix System Laboratories (USL)
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of USL not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  USL makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * USL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL USL
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*=========================================================================*\
|| Copyright (c) 1993 Pittsburgh Powercomputing Corporation (PPc).
|| Copyright (c) 1993 Quarterdeck Office Systems, Inc. (QOS).
||
|| Permission to use, copy, modify, and distribute this software and its
|| documentation for any purpose and without fee is hereby granted, provided
|| that the above copyright notice appear in all copies and that both that
|| copyright notice and this permission notice appear in supporting
|| documentation, and that the names of PPc and QOS not be used in
|| advertising or publicity pertaining to distribution of the software
|| without specific, written prior permission.  Neither PPc nor QOS
|| make any representations about the suitability of this software for any
|| purpose.  It is provided "as is" without express or implied warranty.
||
|| PPc AND QOS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
|| INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
|| EVENT SHALL PPc OR QOS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
|| CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
|| USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
|| OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
|| PERFORMANCE OF THIS SOFTWARE.
\*=========================================================================*/

#define NEED_EVENTS

#include <X11/Xlibint.h>
#include <X11/Xos.h>
#include "Xlibnet.h"
#include <X11/Xauth.h>
#include <stdio.h>
#include <ctype.h>
#ifdef DNETCONN
#include <netdnet/dn.h>
#include <netdnet/dnetdb.h>
#endif

#ifdef STREAMSCONN
#define select _XSelect
#endif

#ifndef X_CONNECTION_RETRIES		/* number retries on ECONNREFUSED */
#define X_CONNECTION_RETRIES 5
#endif

#ifdef DNETCONN
static int MakeDECnetConnection();
#endif
#if defined(UNIXCONN) && !defined(CLIENTS_LOCALCONN)
static int MakeUNIXSocketConnection();
#endif
#ifdef TCPCONN
static int MakeTCPConnection();
#endif
#ifdef STREAMSCONN
extern int _XMakeStreamsConnection();
#endif
#ifdef CLIENTS_LOCALCONN
#include <sys/utsname.h>
#endif

#if defined (SVR4) && !defined (ARCHIVE) /* Note: this #define has been added */
#define select _abi_select               /* for ABI compliance. This #define  */
#endif /* SVR4 */                        /* appears in XlibInt.c, XConnDis.c  */
                                         /* NextEvent.c(libXt), Xmos.c(libXm) */
                                         /* fsio.c (libfont), and Berk.c      */					 /* (libX11.so.1)                     */
#ifdef SVR4
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/select.h>

#ifdef TCPCONN
extern int select();

#include <sys/socket.h>
#include <arpa/inet.h>

#ifdef NOVELL
#include <stdio.h>
#include <fcntl.h>
#include <netconfig.h>
#include <netdir.h>
#include <tiuser.h>
static int
MakeSPX_TLIConnection();
#endif /* NOVELL */
#endif /* SVR4 */
static int MakeLOCALConnection();
#endif

static void GetAuthorization();

#if !defined (SVR4) || defined(ARCHIVE)
extern int connect(), select(), socket(), setsockopt();
#else
#include <dlfcn.h>
#define connect (*(SockFuncPtrs.connect_p))
#define gethostbyname (*(SockFuncPtrs.gethostbyname_p))
#define socket (*(SockFuncPtrs.socket_p))
#define setsockopt (*(SockFuncPtrs.setsockopt_p))
#define DL_ERROR() { fprintf(stderr,"%s\n",dlerror()); return -1; }
static struct _SockFuncPtrs {
int (*connect_p)(), 
    (*setsockopt_p)(), 
    (*socket_p)();
struct hostent * (*gethostbyname_p)();
} SockFuncPtrs;

#endif /* SVR4 */

static char *copystring (src, len)
    char *src;
    int len;
{
    char *dst = Xmalloc (len + 1);

    if (dst) {
	(void) strncpy (dst, src, len);
	dst[len] = '\0';
    }

    return dst;
}


/* 
 * Attempts to connect to server, given display name. Returns file descriptor
 * (network socket) or -1 if connection fails.  Display names may be of the
 * following format:
 *
 *     [hostname] : [:] displaynumber [.screennumber]
 *
 * The second colon indicates a DECnet style name.  No hostname is interpretted
 * as the most efficient local connection to a server on the same machine.  
 * This is usually:
 *
 *     o  shared memory
 *     o  local stream
 *     o  UNIX domain socket
 *     o  TCP to local host
 */
int _XConnectDisplay (display_name, fullnamep, dpynump, screenp,
		      auth_namep, auth_namelenp, auth_datap, auth_datalenp)
    char *display_name;
    char **fullnamep;			/* RETURN */
    int *dpynump;			/* RETURN */
    int *screenp;			/* RETURN */
    char **auth_namep;			/* RETURN */
    int *auth_namelenp;			/* RETURN */
    char **auth_datap;			/* RETURN */
    int *auth_datalenp;			/* RETURN */
{
    int family;
    int saddrlen;
    char *saddr;
    char *lastp, *p;			/* char pointers */
    char *phostname = NULL;		/* start of host of display */
    char *pdpynum = NULL;		/* start of dpynum of display */
    char *pscrnum = NULL;		/* start of screen of display */
    Bool dnet = False;			/* if true, then DECnet format */
    int idisplay;			/* required display number */
    int iscreen = 0;			/* optional screen number */
#ifdef NOVELL
    int (*connfunc[2])();		/* method to create connections */
                                        /* the array allows for one tcp */
					/* listener and one spx listener */
#else
    int (*connfunc)();			/* method to create connection */
#endif /* NOVELL */
    int fd = -1;			/* file descriptor to return */
    int len;				/* length tmp variable */

#ifdef CLIENTS_LOCALCONN
    struct utsname sys;
#endif
    p = display_name;

    saddrlen = 0;			/* set so that we can clear later */
    saddr = NULL;

    /*
     * Step 1, find the hostname.  This is delimited by the required 
     * first colon.
     */
    for (lastp = p; *p && *p != ':'; p++) ;
    if (!*p) return -1;		/* must have a colon */

    if (p != lastp) {		/* no hostname given */
	phostname = copystring (lastp, p - lastp);
	if (!phostname) goto bad;	/* no memory */
    }

#ifdef CLIENTS_LOCALCONN
	/* check if phostname == localnodename */
    if (uname(&sys) != -1) {
	if(!strncmp(phostname, sys.nodename, strlen(sys.nodename)))
	  phostname = "unix";
    }
#endif /* CLIENTS_LOCALCONN */

    /*
     * Step 2, see if this is a DECnet address by looking for the optional
     * second colon.
     */
    if (p[1] == ':') {			/* then DECnet format */
	dnet = True;
	p++;
    }

    /*
     * see if we're allowed to have a DECnet address
     */
#ifndef DNETCONN
    if (dnet) goto bad;
#endif

    
    /*
     * Step 3, find the display number.  This field is required and is 
     * delimited either by a nul or a period, depending on whether or not
     * a screen number is present.
     */

    for (lastp = ++p; *p && isascii(*p) && isdigit(*p); p++) ;
    if ((p == lastp) ||			/* required field */
	(*p != '\0' && *p != '.') ||	/* invalid non-digit terminator */
	!(pdpynum = copystring (lastp, p - lastp)))  /* no memory */
      goto bad;
    idisplay = atoi (pdpynum);


    /*
     * Step 4, find the screen number.  This field is optional.  It is 
     * present only if the display number was followed by a period (which
     * we've already verified is the only non-nul character).
     */

    if (*p) {
	for (lastp = ++p; *p && isascii(*p) && isdigit (*p); p++) ;
	if (p != lastp) {
	    if (*p ||			/* non-digits */
		!(pscrnum = copystring (lastp, p - lastp))) /* no memory */
		goto bad;
	    iscreen = atoi (lastp);
	}
    }



    /*
     * At this point, we know the following information:
     *
     *     phostname                hostname string or NULL
     *     idisplay                 display number
     *     iscreen                  screen number
     *     dnet                     DECnet boolean
     * 
     * We can now decide which transport to use based on the ConnectionFlags
     * build parameter the hostname string.  If phostname is NULL or equals
     * the string "local", then choose the best transport.  If phostname
     * is "unix", then choose BSD UNIX domain sockets (if configured).
     *
     * First, choose default transports:  DECnet else (TCP or STREAMS)
     */
     /* it appears that we need to support at least two network providers, */
     /* we have to be able to connect over ipx/spx as well as tcp... */ 

#ifdef DNETCONN
    if (dnet)
      connfunc = MakeDECnetConnection;
    else
#endif
#ifdef TCPCONN
#ifdef NOVELL
      connfunc[0] = MakeTCPConnection;
#else
      connfunc = MakeTCPConnection;
#endif /* NOVELL */
#else
#ifdef STREAMSCONN
      connfunc = _XMakeStreamsConnection;
#else
#ifdef NOVELL
      connfunc[0] = NULL;
#else
      connfunc = NULL;
#endif /* NOVELL */
#endif
#endif
#ifdef NOVELL
      connfunc[1] = MakeSPX_TLIConnection;
#endif /* NOVELL */

#if defined(UNIXCONN) && !defined(CLIENTS_LOCALCONN)
    /*
     * Now that the defaults have been established, see if we have any 
     * special names that we have to override:
     *
     *     :N         =>     if UNIXCONN then unix-domain-socket
     *     ::N        =>     if UNIXCONN then unix-domain-socket
     *     unix:N     =>     if UNIXCONN then unix-domain-socket
     *
     * Note that if UNIXCONN isn't defined, then we can use the default
     * transport connection function set above.
     */
    if (!phostname) {
#ifdef apollo
	;   /* Unix domain sockets are *really* bad on apollos */
#else
#ifdef NOVELL
	connfunc[0] = MakeUNIXSocketConnection;
#else
	connfunc = MakeUNIXSocketConnection;
#endif
#endif /* NOVELL */
    }
    else if (strcmp (phostname, "unix") == 0) {
#ifdef NOVELL
	connfunc[0] = MakeUNIXSocketConnection;
#else
	connfunc = MakeUNIXSocketConnection;
#endif /* NOVELL */
    }
#endif /* UNIXCONN && !CLIENTS_LOCALCONN */

#ifdef CLIENTS_LOCALCONN
#define LOCALCONNECTION (!phostname || !strcmp(phostname, "unix"))
    /*
     *     :N         =>
     *     unix:N     =>   use a local connection method (see below)
     */
    if (LOCALCONNECTION) {
#ifdef NOVELL
	connfunc[0] = MakeLOCALConnection;
#else
	connfunc = MakeLOCALConnection;
#endif /* NOVELL */
    }
#endif /* CLIENTS_LOCALCONN */

#ifdef NOVELL
    if (!connfunc[0])
#else 
    if (!connfunc)
#endif NOVELL
	goto bad;

#if defined(UNIXCONN) && !defined(CLIENTS_LOCALCONN)
#ifdef NOVELL
#define LOCALCONNECTION (!phostname || connfunc[0] == MakeUNIXSocketConnection)
#else
#define LOCALCONNECTION (!phostname || connfunc == MakeUNIXSocketConnection)
#endif /* NOVELL */
#endif

#ifndef LOCALCONNECTION
#define LOCALCONNECTION (!phostname)
#endif

    if (LOCALCONNECTION) {
	/*
	 * Get the auth info for local hosts so that it doesn't have to be
	 * repeated everywhere; the particular values in these fields are
	 * not part of the protocol.
	 */
	char hostnamebuf[256];
	int len = _XGetHostname (hostnamebuf, sizeof hostnamebuf);

	family = FamilyLocal;
	if (len > 0) {
	    saddr = Xmalloc (len + 1);
	    if (saddr) {
		(void) strcpy (saddr, hostnamebuf);
		saddrlen = len;
	    } else {
		saddrlen = 0;
	    }
	}
    }
#undef LOCALCONNECTION


    /*
     * Make the connection, also need to get the auth address info for
     * non-local connections.  Do retries in case server host has hit its
     * backlog (which, unfortunately, isn't distinguishable from there not
     * being a server listening at all, which is why we have to not retry
     * too many times).
     */
#ifdef NOVELL
    if ((fd = (*connfunc[0]) (phostname, idisplay, X_CONNECTION_RETRIES,
#else
    if ((fd = (*connfunc) (phostname, idisplay, X_CONNECTION_RETRIES,
#endif /* NOVELL */
			   &family, &saddrlen, &saddr)) >= 0)
       ;	 /* connecting via local or TCP */
#ifdef NOVELL
    else if ( connfunc[1] != NULL &&
    (fd = (*connfunc[1]) (phostname, idisplay)) >= 0)
       ;         /* try spx if set up fails for tcp */
#endif /* NOVELL */
    else
      goto bad;
    if (fd >= OPEN_MAX)
	goto bad;

    /*
     * Set close-on-exec so that programs that fork() doesn't get confused.
     */

#ifdef FIOSNBIO
    {
	int arg = 1;
	ioctl (fd, FIOSNBIO, &arg);
    }
#else
# ifdef FNDELAY
    (void) fcntl (fd, F_SETFL, FNDELAY);
# else
    (void) fcntl (fd, F_SETFL, O_NDELAY);
# endif /* FNDELAY */
#endif /* FIOSNBIO */

#ifdef FD_CLOEXEC
    (void) fcntl (fd, F_SETFD, FD_CLOEXEC);
#else
    (void) fcntl (fd, F_SETFD, 1);
#endif

    /*
     * Build the expanded display name:
     *
     *     [host] : [:] dpy . scr \0
     */
    len = ((phostname ? strlen(phostname) : 0) + 1 + (dnet ? 1 : 0) +
	   strlen(pdpynum) + 1 + (pscrnum ? strlen(pscrnum) : 1) + 1);
    *fullnamep = (char *) Xmalloc (len);
    if (!*fullnamep) goto bad;

    (void) sprintf (*fullnamep, "%s%s%d.%d",
		    (phostname ? phostname : ""), (dnet ? "::" : ":"),
		    idisplay, iscreen);

    *dpynump = idisplay;
    *screenp = iscreen;
    if (phostname) Xfree (phostname);
    if (pdpynum) Xfree (pdpynum);
    if (pscrnum) Xfree (pscrnum);

    GetAuthorization(fd, family, saddr, saddrlen, idisplay,
		     auth_namep, auth_namelenp, auth_datap, auth_datalenp);
    return fd;


    /*
     * error return; make sure everything is cleaned up.
     */
  bad:
    if (fd >= 0) (void) close (fd);
    if (saddr) Xfree (saddr);
    if (phostname) Xfree (phostname);
    if (pdpynum) Xfree (pdpynum);
    if (pscrnum) Xfree (pscrnum);
    return -1;

}


/*****************************************************************************
 *                                                                           *
 *			   Make Connection Routines                          *
 *                                                                           *
 *****************************************************************************/

#ifdef DNETCONN				/* stupid makedepend */
#define NEED_BSDISH
#endif
#ifdef UNIXCONN
#define NEED_BSDISH
#endif
#ifdef TCPCONN
#define NEED_BSDISH
#endif

#ifdef NEED_BSDISH			/* makedepend can't handle #if */
/*
 * 4.2bsd-based systems
 */
/* #include <sys/socket.h> */

#ifdef hpux
#define NO_TCP_H
#endif
#ifdef MOTOROLA
#ifdef SYSV
#define NO_TCP_H
#endif
#endif
#ifndef NO_TCP_H
#ifdef __OSF1__
#include <sys/param.h>
#endif
#include <netinet/tcp.h>
#endif
#endif /* NEED_BSDISH */


#ifdef DNETCONN
static int MakeDECnetConnection (phostname, idisplay, retries,
				 familyp, saddrlenp, saddrp)
    char *phostname;
    int idisplay;
    int retries;
    int *familyp;			/* RETURN */
    int *saddrlenp;			/* RETURN */
    char **saddrp;			/* RETURN */
{
    int fd;
    char objname[20];
    extern int dnet_conn();
    struct dn_naddr *dnaddrp, dnaddr;
    struct nodeent *np;

    if (!phostname) phostname = "0";

    /*
     * build the target object name.
     */
    (void) sprintf (objname, "X$X%d", idisplay);

    /*
     * Attempt to open the DECnet connection, return -1 if fails; ought to
     * do some retries here....
     */
    if ((fd = dnet_conn (phostname, objname, SOCK_STREAM, 0, 0, 0, 0)) < 0) {
	return -1;
    }

    *familyp = FamilyDECnet;
    if (dnaddrp = dnet_addr (phostname)) {  /* stolen from xhost */
	dnaddr = *dnaddrp;
    } else {
	if ((np = getnodebyname (phostname)) == NULL) {
	    (void) close (fd);
	    return -1;
	}
	dnaddr.a_len = np->n_length;
	bcopy (np->n_addr, dnaddr.a_addr, np->n_length);
    }

    *saddrlenp = sizeof (struct dn_naddr);
    *saddrp = Xmalloc (*saddrlenp);
    if (!*saddrp) {
	(void) close (fd);
	return -1;
    }
    bcopy ((char *)&dnaddr, *saddrp, *saddrlenp);
    return fd;
}
#endif /* DNETCONN */


#if defined(UNIXCONN) && !defined(CLIENTS_LOCALCONN)
#include <sys/un.h>

/*ARGSUSED*/
static int MakeUNIXSocketConnection (phostname, idisplay, retries,
				     familyp, saddrlenp, saddrp)
    char *phostname;
    int idisplay;
    int retries;
    int *familyp;			/* RETURN */
    int *saddrlenp;			/* RETURN */
    char **saddrp;			/* RETURN */
{
    struct sockaddr_un unaddr;		/* UNIX socket data block */
    struct sockaddr *addr;		/* generic socket pointer */
    int addrlen;			/* length of addr */
    int fd;				/* socket file descriptor */
#ifdef hpux /* this is disgusting */
    struct sockaddr_un ounaddr;		/* UNIX socket data block */
    struct sockaddr *oaddr;		/* generic socket pointer */
    int oaddrlen;			/* length of addr */
#endif

    unaddr.sun_family = AF_UNIX;
    (void) sprintf (unaddr.sun_path, "%s%d", X_UNIX_PATH, idisplay);

    addr = (struct sockaddr *) &unaddr;
#ifdef SUN_LEN
    addrlen = SUN_LEN (&unaddr);
#else
    addrlen = strlen(unaddr.sun_path) + sizeof(unaddr.sun_family);
#endif

#ifdef hpux /* this is disgusting */
    ounaddr.sun_family = AF_UNIX;
    (void) sprintf (ounaddr.sun_path, "%s%d", OLD_UNIX_PATH, idisplay);
    oaddr = (struct sockaddr *) &ounaddr;
    oaddrlen = strlen(ounaddr.sun_path) + sizeof(ounaddr.sun_family);
#endif

    /*
     * Open the network connection.
     */
    do {
	if ((fd = socket ((int) addr->sa_family, SOCK_STREAM, 0)) < 0) {
	    return -1;
	}

	if (connect (fd, addr, addrlen) < 0) {
	    int olderrno = errno;
	    (void) close (fd);
#ifdef hpux /* this is disgusting */
	    if (olderrno == ENOENT) {
		fd = socket ((int) oaddr->sa_family, SOCK_STREAM, 0);
		if (fd >= 0) {
		    if (connect (fd, oaddr, oaddrlen) >= 0)
			break;
		    olderrno = errno;
		    (void) close (fd);
		}
	    }
#endif
	    if (olderrno != ENOENT || retries <= 0) {
		errno = olderrno;
		return -1;
	    }
	    (void) sleep (1);
	} else {
	    break;
	}
    } while (retries-- > 0);

    /*
     * Don't need to get auth info since we're local
     */
    return fd;
}
#endif /* UNIXCONN */


#ifdef TCPCONN
static int MakeTCPConnection (phostname, idisplay, retries,
			      familyp, saddrlenp, saddrp)
    char *phostname;
    int idisplay;
    int retries;
    int *familyp;			/* RETURN */
    int *saddrlenp;			/* RETURN */
    char **saddrp;			/* RETURN */
{
    char hostnamebuf[256];		/* tmp space */
    unsigned long hostinetaddr;		/* result of inet_addr of arpa addr */
    struct sockaddr_in inaddr;		/* IP socket */
    struct sockaddr *addr;		/* generic socket pointer */
    int addrlen;			/* length of addr */
    struct hostent *hp;			/* entry in hosts table */
    char *cp;				/* character pointer iterator */
    int fd;				/* file descriptor to return */
    int len;				/* length tmp variable */

#define INVALID_INETADDR ((unsigned long) -1)
#if defined (SVR4) && !defined(ARCHIVE)

void *libsocket_handle;

    if( ! SockFuncPtrs.socket_p ) {
        if((libsocket_handle = dlopen("libsocket.so", RTLD_LAZY)) != NULL)
        {
            if((SockFuncPtrs.socket_p = (int (*)()) dlsym(libsocket_handle, 
                    "socket")) == NULL) DL_ERROR()
            if((SockFuncPtrs.connect_p = (int (*)()) dlsym(libsocket_handle, 
                    "connect")) == NULL) DL_ERROR()
            if((SockFuncPtrs.setsockopt_p = (int (*)()) dlsym(libsocket_handle, 
                    "setsockopt")) == NULL) DL_ERROR()
            if((SockFuncPtrs.gethostbyname_p = (struct hostent* (*)()) 
                     dlsym(libsocket_handle, "gethostbyname")) == NULL)
                        DL_ERROR()
         }
         else 
            DL_ERROR()
         }
#endif /* SVR4 */
    if (!phostname) {
	hostnamebuf[0] = '\0';
	(void) _XGetHostname (hostnamebuf, sizeof hostnamebuf);
	phostname = hostnamebuf;
    }
    /*
     * if numeric host name then try to parse it as such; do the number
     * first because some systems return garbage instead of INVALID_INETADDR
     */
    if (isascii(phostname[0]) && isdigit(phostname[0])) {
	hostinetaddr = inet_addr (phostname);
    } else {
	hostinetaddr = INVALID_INETADDR;
    }

    /*
     * try numeric
     */
    if (hostinetaddr == INVALID_INETADDR) {
	if ((hp = gethostbyname(phostname)) == NULL) {
	    /* No such host! */
	    return -1;
	}
	if (hp->h_addrtype != AF_INET) {  /* is IP host? */
	    /* Not an Internet host! */
	    return -1;
	}
	/* Set up the socket data. */
	inaddr.sin_family = hp->h_addrtype;
#if defined(CRAY) && defined(OLDTCP)
	/* Only Cray UNICOS3 and UNICOS4 will define this */
	{
	    long t;
	    bcopy ((char *)hp->h_addr, (char *)&t, sizeof(t));
	    inaddr.sin_addr = t;
	}
#else
	bcopy ((char *)hp->h_addr, (char *)&inaddr.sin_addr, 
	       sizeof(inaddr.sin_addr));
#endif /* CRAY and OLDTCP */
    } else {
#if defined(CRAY) && defined(OLDTCP)
	/* Only Cray UNICOS3 and UNICOS4 will define this */
	inaddr.sin_addr = hostinetaddr;
#else
	inaddr.sin_addr.s_addr = hostinetaddr;
#endif /* CRAY and OLDTCP */
	inaddr.sin_family = AF_INET;
    }

    addr = (struct sockaddr *) &inaddr;
    addrlen = sizeof (struct sockaddr_in);
    inaddr.sin_port = X_TCP_PORT + idisplay;
    inaddr.sin_port = htons (inaddr.sin_port);	/* may be funky macro */


    /*
     * Open the network connection.
     */
    do {
	if ((fd = socket ((int) addr->sa_family, SOCK_STREAM, 0)) < 0) {
	    return -1;
	}

	/*
	 * turn off TCP coalescence
	 */
#ifdef TCP_NODELAY
	{
	    int tmp = 1;
	    (void) setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, (char*)&tmp, sizeof (int));
	}
#endif

	/*
	 * connect to the socket; if there is no X server or if the backlog has
	 * been reached, then ECONNREFUSED will be returned.
	 */
	if (connect (fd, (caddr_t)addr, addrlen) < 0) {
	    int olderrno = errno;
	    (void) close (fd);
	    if (olderrno != ECONNREFUSED || retries <= 0) {
		errno = olderrno;
		return -1;
	    }
	    (void) sleep (1);
	} else {
	    break;
	}
    } while (retries-- > 0);


    /*
     * Success!  So, save the auth information
     */
#ifdef CRAY
#ifdef OLDTCP
    len = sizeof(inaddr.sin_addr);
#else
    len = SIZEOF_in_addr;
#endif /* OLDTCP */
    cp = (char *) &inaddr.sin_addr;
#else /* else not CRAY */
    len = sizeof(inaddr.sin_addr.s_addr);
    cp = (char *) &inaddr.sin_addr.s_addr;
#endif /* CRAY */

    /*
     * We are special casing the BSD hack localhost address
     * 127.0.0.1, since this address shouldn't be copied to
     * other machines.  So, we simply omit generating the auth info
     * since we set it to the local machine before calling this routine!
     */
    if (!((len == 4) && (cp[0] == 127) && (cp[1] == 0) &&
	  (cp[2] == 0) && (cp[3] == 1))) {
	*saddrp = Xmalloc (len);
	if (*saddrp) {
	    *saddrlenp = len;
	    bcopy (cp, *saddrp, len);
	    *familyp = FamilyInternet;
	} else {
	    *saddrlenp = 0;
	}
    }

    return fd;
}
#undef INVALID_INETADDR
#endif /* TCPCONN */

#ifdef CLIENTS_LOCALCONN
/*=========================================================================*\
||
||      The following is an amalgamation of various local connection
||      methods as used on PC-UNIX boxes.  It uses the environment
||      variable XLOCAL to define a preferred sequence of connection
||      methods to use.  Failing that, it tries its own sequence.
||      Failing all local connection methods, it will use TCP/IP.
||
||      Pay special attention to the primary and alternate
||      nodes used by the ISC and UNIX connection methods.
||
||      The environment variable XLOCAL_VERBOSE may be set to enable
||      connection-related information messages printed to stderr.
||
|| EXAMPLE:
||	setenv XLOCAL ISC:NAMED:SCO:PTS:UNIX
||	setenv XLOCAL_VERBOSE 1
||
|| VALID OPTIONS: (the first option for each is the recommended usage)
||
||	ISC_STREAMS	= { ISC | SP }
||	SCO_STREAMS	= { SCO | XSIGHT }
||	NAMED_STREAMS	= { NAMED | USL }
||	PTS_STREAMS	= { PTS | ATT }
||	UNIX_SOCKETS	= { UNIX | UNIXSOCKET | UNIXDOMAIN }
||	TCP/IP		= { TCP }
||	(if TCP is specified, no further local methods will be tried)
||
|| COMPILATION SWITCHES:
||	-DTCPCONN	= enables TCP connections.
||	-DUNIXCONN	= enables UNIX domain sockets.
||	-DCLIENTS_LOCALCONN = enables ATT pts connections,
||			      uses slightly different UNIX domain code,
||			      also is a prerequisite for the following two...
||	-DSVR4		= enables NAMED pipes.
||	-DSVR4_ACP	= enables ISC and SCO streams for SVR4 systems.
||
||
|| This should probably all be moved to a separate file (Xlocal.c?)
\*=========================================================================*/

#include <signal.h>

#ifdef SVR4
#define XLOCAL_NAMED
#define X_NAMED_PATH	"/dev/X/Nserver."
#endif

#if !defined(SVR4) || defined(SVR4_ACP)
#define XLOCAL_ISC
#define XLOCAL_SCO
#include <sys/stream.h>
#include <sys/stropts.h>
#define X_ISC_DEVPATH	"/dev/X/ISCCONN/X"
#define X_ISC_PATH	"/tmp/.X11-unix/X"
#define X_SCO_PATH	"/dev/X"
#define DEV_SPX		"/dev/spx"
#endif

#ifdef unix
#define XLOCAL_PTS
#define X_PTS_PATH "/dev/X/server."
extern char *ptsname();
extern int grantpt(), unlockpt();
#endif

#ifdef UNIXCONN
#define XLOCAL_UNIX
#include <sys/un.h>
#define X_UNIX_DEVPATH "/dev/X/UNIXCON/X"
#endif

/* define NO_XLOCAL_VERBOSE to remove the XLOCAL_VERBOSE functionality */
#ifdef NO_XLOCAL_VERBOSE
#define XLOCAL_MSG(x)   /* as nothing */
#else
static void xlocalMsg();
#define XLOCAL_MSG(x)   xlocalMsg x;
#endif

/* dummy signal handler */
/*ARGSUSED*/
static void
_dummy(temp)
  int temp;
{
    return;
}

#ifndef NO_XLOCAL_VERBOSE
/*PRINTFLIKE1*/
static void
xlocalMsg(lvl,str,a,b,c,d,e,f,g,h)
  int lvl;
  char *str;
  int a,b,c,d,e,f,g,h;
{
    static int xlocalMsgLvl = -2;

    if (xlocalMsgLvl == -2) {
	char *tmp, *getenv();
        if ((tmp = getenv("XLOCAL_VERBOSE")) != NULL) {
	    xlocalMsgLvl = atoi(tmp);
	} else {
	    xlocalMsgLvl = -1;
	}
    }

    if (xlocalMsgLvl >= lvl) {
	(void) fprintf(stderr,"Xlib: XLOCAL - ");
	(void) fprintf(stderr,str,a,b,c,d,e,f,g,h);
    }
}
#endif /* NO_XLOCAL_MSG */

#include <sys/stat.h>
#include <sys/inode.h>

/*=========================================================================*/
#ifdef XLOCAL_NAMED
/*
 * AT&T's named pipe connection (SVR4)
 * XLOCAL = NAMED or USL
 */
/*ARGSUSED*/
static int
MakeLocalNAMEDConnection(idisplay,retries)
  int idisplay;
  int retries;
{
    int fd;
    char server_path[64];
    struct stat filestat;
    extern int isastream();
    struct strrecvfd str;

    (void) sprintf(server_path,"%s%d",X_NAMED_PATH, idisplay);
    XLOCAL_MSG((1,"XLOCAL_NAMED trying [%s]\n",server_path));

    if (stat(server_path, &filestat) != -1) {
	if ((filestat.st_mode & IFMT) == IFIFO) {
	    if ((fd = open(server_path, O_RDWR)) >= 0) {
#ifdef I_BIGPIPE
		if( ioctl( fd, I_BIGPIPE, &str ) != -1 )
		    XLOCAL_MSG((1,"I_BIGPIPE option succeeded for XLOCAL_NAMED\n"));
#endif /* I_BIGPIPE */
		if (isastream(fd) > 0) {
		    XLOCAL_MSG((1,"XLOCAL_NAMED [%s] succeeded\n",
			      server_path));
		    return(fd);
		}
		(void) close(fd);
	    }
	} else {
	    XLOCAL_MSG((1,"XLOCAL_NAMED non-pipe node.\n"));
	}
    }
    XLOCAL_MSG((1,"XLOCAL_NAMED failed\n"));
    return(-1);
}
#endif /* XLOCAL_NAMED */

/*=========================================================================*/
#ifdef XLOCAL_ISC
/*
 * ISC local connection (/dev/spx)
 * XLOCAL = ISC or SP
 */
/*ARGSUSED*/
static int
MakeLocalISCConnection(idisplay,retries)
  int idisplay;
  int retries;
{
    int fd,fds,server;
    char server_path[64];
    struct strfdinsert buf;
    long temp;
    o_mode_t spmode;
    struct stat filestat;

    fd = fds = server = -1;

    if (stat(DEV_SPX, &filestat) == -1) {
	XLOCAL_MSG((1,"XLOCAL_ISC no sp driver or spx node.\n"));
	return(-1);
    }
    spmode = (filestat.st_mode & IFMT);

    (void) sprintf(server_path,"%s%d", X_ISC_DEVPATH, idisplay);
    XLOCAL_MSG((1,"XLOCAL_ISC trying [%s]\n",server_path));
    if (stat(server_path, &filestat) != -1) {
	if ((filestat.st_mode & IFMT) == spmode) {
	    if ((server = open(server_path, O_RDWR)) != -1) {
		XLOCAL_MSG((1,"SP_STREAMS opened [%s]\n",server_path));
	    }
	}
    }

    if (server == -1) {
	(void) sprintf(server_path,"%s%d", X_ISC_PATH, idisplay);
	XLOCAL_MSG((1,"XLOCAL_ISC trying [%s]\n",server_path));
	if (stat(server_path, &filestat) != -1) {
	    if ((filestat.st_mode & IFMT) == spmode) {
		if ((server = open(server_path, O_RDWR)) != -1) {
		    XLOCAL_MSG((1,"XLOCAL_ISC opened [%s]\n",server_path));
		}
	    }
	}
    }

    if (server >= 0) {
	if ((fds = open(DEV_SPX, O_RDWR)) >= 0 &&
	    (fd  = open(DEV_SPX, O_RDWR)) >= 0) {
	
	    /* make a STREAMS-pipe */
	    buf.databuf.maxlen = -1;
	    buf.databuf.len = -1;
	    buf.databuf.buf = NULL;
	    buf.ctlbuf.maxlen = sizeof(long);
	    buf.ctlbuf.len = sizeof(long);
	    buf.ctlbuf.buf = (caddr_t)&temp;
	    buf.offset = 0;
	    buf.fildes = fd;
	    buf.flags = 0;
	
	    if (ioctl(fds, I_FDINSERT, &buf) != -1 &&
		ioctl(server, I_SENDFD, fds) != -1) {
		(void) close(fds); 
		(void) close(server);
		XLOCAL_MSG((1,"XLOCAL_ISC [%s] succeeded\n",server_path));
		return (fd);
	    }
	}
	(void) close(server);
	if (fds != -1)
	  (void) close(fds);
	if (fd != -1)
	  (void) close(fd);
    }
    XLOCAL_MSG((1,"XLOCAL_ISC failed\n"));
    return(-1);
}
#endif /* XLOCAL_ISC */

/*=========================================================================*/
#ifdef XLOCAL_SCO
/*
 * SCO's local connection (XSIGHT version)
 * XLOCAL = SCO or XSIGHT
 */
/*ARGSUSED*/
static int
MakeLocalSCOConnection(idisplay,retries)
  int idisplay;
  int retries;
{
    int fd,server,fl,ret;
    char server_path[64];
    struct strbuf ctlbuf;
    unsigned alarm_time;
    void (*savef)();
    long temp;
    extern int getmsg(), putmsg();

    (void) sprintf(server_path,"%s%1dR", X_SCO_PATH, idisplay);
    XLOCAL_MSG((1,"SCO_STREAMS trying [%s]\n",server_path));

    if ((server = open(server_path, O_RDWR)) >= 0) {
	if ((fd = open(DEV_SPX, O_RDWR)) >= 0) {
	
	    (void) write(server, &server, 1);
	    ctlbuf.len = 0;
	    ctlbuf.maxlen = sizeof(long);
	    ctlbuf.buf = (caddr_t)&temp;
	    fl = 0;
	
	    savef = signal(SIGALRM, _dummy);
	    alarm_time = alarm(10);
	
	    ret = getmsg(server, &ctlbuf, 0, &fl);

	    (void) alarm(alarm_time);
	    (void) signal(SIGALRM, savef);
	    
	    if (ret >= 0) {
		/* The msg we got via getmsg is the result of an 
		 * I_FDINSERT, so if we do a putmsg with whatever
		 * we recieved, we're doing another I_FDINSERT ...
		 */
		(void) putmsg(fd, &ctlbuf, 0, 0);
		(void) fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NDELAY);

		(void) close(server);
		XLOCAL_MSG((1,"SCO_STREAMS [%s] succeeded\n",server_path));
		return(fd);
	    }
	    (void) close(fd);
	}
	(void) close(server);
    }
    XLOCAL_MSG((1,"SCO_STREAMS failed\n"));
    return(-1);
}
#endif /* XLOCAL_SCO */

/*=========================================================================*/
#ifdef XLOCAL_PTS
/*
 * AT&T's local connection (SVR3&SVR4)
 * XLOCAL = PTS or ATT
 */
/*ARGSUSED*/
static int
MakeLocalPTSConnection(idisplay,retries)
  int idisplay;
  int retries;
{
    int fd,server,ret;
    char server_path[64];
    char rbuf[64], *slave;
    void (*savef)();
    long temp;
    unsigned alarm_time;

    (void) sprintf(server_path,"%s%d", X_PTS_PATH, idisplay);
    XLOCAL_MSG((1,"PTS_STREAMS trying [%s]\n",server_path));

    if ((server = open (server_path, O_RDWR)) >= 0) {
	if ((fd = open("/dev/ptmx", O_RDWR)) >= 0) {

	    (void) grantpt(fd);
	    (void) unlockpt(fd);
	    slave = ptsname(fd); /* get name */

	    /*
	     * write slave name to server
	     */
	    temp = strlen(slave);
	    rbuf[0] = temp;
	    (void) sprintf(&rbuf[1], slave);
	    (void) write(server, rbuf, temp+1);

	    /*
	     * wait for server to respond
	     */
	    savef = signal(SIGALRM, _dummy);
	    alarm_time = alarm (30);

	    ret = read(fd, rbuf, 1);

	    (void) alarm(alarm_time);
	    (void) signal(SIGALRM, savef);
	
	    if (ret == 1) {
		(void) close(server);
		XLOCAL_MSG((1,"PTS_STREAMS [%s] succeeded\n",server_path));
		return(fd);
	    }
	    (void) close(fd);
	}
	(void) close(server);
    }
    XLOCAL_MSG((1,"PTS_STREAMS [%s] failed\n",server_path));
    return(-1);
}
#endif /* XLOCAL_PTS */

/*=========================================================================*/
#ifdef XLOCAL_UNIX
/*
 * UNIX or UNIXDOMAIN connections
 */
static int
MakeLocalUNIXDOMAINConnection(idisplay, retries)
  int idisplay;
  int retries;
{
    struct sockaddr_un unaddr;	/* UNIX socket data block */
    struct sockaddr *addr;	/* generic socket pointer */
    int addrlen;		/* length of addr */
    int fd;			/* socket file descriptor */
    struct stat filestat;
    int olderrno;

    unaddr.sun_family = AF_UNIX;
    (void) sprintf (unaddr.sun_path, "%s%d", X_UNIX_DEVPATH, idisplay);
    XLOCAL_MSG((1,"UNIX_SOCKET trying [%s]\n",unaddr.sun_path));
    if (stat(unaddr.sun_path, &filestat) == -1) {
	(void) sprintf (unaddr.sun_path, "%s%d", X_UNIX_PATH, idisplay);
	XLOCAL_MSG((1,"UNIX_SOCKET trying [%s]\n",unaddr.sun_path));
	if (stat(unaddr.sun_path, &filestat) == -1) {
	    XLOCAL_MSG((1,"UNIX_SOCKET failed\n"));
	    return(-1);
	}
    }

    addr = (struct sockaddr *) &unaddr;
#ifdef SUN_LEN
    addrlen = SUN_LEN (&unaddr);
#else
    addrlen = strlen(unaddr.sun_path) + sizeof(unaddr.sun_family);
#endif

    /*
     * Open the network connection.
     */
    do {
	if ((fd = socket ((int) addr->sa_family, SOCK_STREAM, 0)) < 0) {
	    olderrno = errno;
	    break;
	}

	if (connect (fd, (caddr_t)addr, addrlen) >= 0) {
	    /* Don't need to get auth info since we're local */
	    XLOCAL_MSG((1,"UNIX_SOCKET [%s] succeeded\n",unaddr.sun_path));
	    return(fd);
	}

	olderrno = errno;
	(void) close(fd);
	if (olderrno != ENOENT || retries <= 0) {
	    break;
	}
	(void) sleep (1);
    } while (retries-- > 0);

    errno = olderrno;
    XLOCAL_MSG((1,"UNIX_SOCKET failed\n"));
    return(-1);
}
#endif /* XLOCAL_UNIX */

/*=========================================================================*/
typedef struct {
    char *name;
    int (*connFn)();
} xlocalListRec, *xlocalListPtr;

#define PRIMARY_END	"PRIMARY_END"

static xlocalListRec xlocalList[] = {
    /**** PRIMARY METHODS ****/
#ifdef XLOCAL_NAMED
    { "NAMED",	MakeLocalNAMEDConnection, },
#endif
#ifdef XLOCAL_ISC
    { "ISC",	MakeLocalISCConnection, },
#endif
#ifdef XLOCAL_SCO
    { "SCO",	MakeLocalSCOConnection, },
#endif
#ifdef XLOCAL_PTS
    { "PTS",	MakeLocalPTSConnection, },
#endif
#ifdef XLOCAL_UNIX
    { "UNIX",	MakeLocalUNIXDOMAINConnection, },
#endif
    { PRIMARY_END, NULL, },
    /**** ALTERNATE NAMES FOR ABOVE METHOD ****/
#ifdef XLOCAL_NAMED
    { "USL",	MakeLocalNAMEDConnection, },
#endif
#ifdef XLOCAL_ISC
    { "SP",	MakeLocalISCConnection, },
#endif
#ifdef XLOCAL_SCO
    { "XSIGHT",	MakeLocalSCOConnection, },
#endif
#ifdef XLOCAL_PTS
    { "ATT",	MakeLocalPTSConnection, },
#endif
#ifdef XLOCAL_UNIX
    { "UNIXSOCKET", MakeLocalUNIXDOMAINConnection, },
    { "UNIXDOMAIN", MakeLocalUNIXDOMAINConnection, },
#endif
    { NULL, NULL, },
};

#define WHITE	" :\t\n\r"

static int
MakeLOCALConnection (phostname, idisplay, retries,
		     familyp, saddrlenp, saddrp)
  char *phostname;
  int idisplay;
  int retries;
  int *familyp;			/* RETURN */
  int *saddrlenp;		/* RETURN */
  char **saddrp;		/* RETURN */
{
    int ret;
    char *name,*nameList,*envNameList;
    xlocalListRec *xlocal;

    if ((envNameList = getenv("XLOCAL")) != NULL) {
	XLOCAL_MSG((0,"Method list=[%s]\n",envNameList));
	nameList = copystring(envNameList, strlen(envNameList));
	name = strtok(nameList,WHITE);
	while (name) {
	    XLOCAL_MSG((1,"[%s] lookup...\n",name));
	    if (!strncmp(name,"TCP",3))
	      goto do_tcpip;
	    for (xlocal = xlocalList; xlocal->name; ++xlocal) {
		if (!strcmp(PRIMARY_END,xlocal->name))
		  continue;
		if (!strcmp(name,xlocal->name)) {
		    XLOCAL_MSG((1,"[%s] found it, try to connect...\n",name));
		    if ((ret = (*xlocal->connFn)(idisplay,retries)) != -1) {
			XLOCAL_MSG((0,"[%s] success (%d)! \n",name,ret));
			Xfree(nameList);
			return(ret);
		    }
		    XLOCAL_MSG((0,"[%s] connect failure\n",name));
		}
	    }
	    name = strtok(NULL,WHITE);
	}
	XLOCAL_MSG((0,"Method list exhausted.\n"));
	Xfree(nameList);
    } else {
	XLOCAL_MSG((0,"No XLOCAL in environment.\n"));
    }

    XLOCAL_MSG((0,"Starting automatic list...\n"));
    for (xlocal = xlocalList; xlocal->name; ++xlocal) {
	if (!strcmp(PRIMARY_END,xlocal->name))
	  break;
	XLOCAL_MSG((1,"[%s] try to connect...\n",xlocal->name));
	if ((ret = (*xlocal->connFn)(idisplay,retries)) != -1) {
	    XLOCAL_MSG((0,"[%s] success (%d)! \n",xlocal->name,ret));
	    return(ret);
	}
	XLOCAL_MSG((0,"[%s] connect failure\n",xlocal->name));
    }
  do_tcpip:
    XLOCAL_MSG((1,"Unable to connect locally.\n"));
#ifdef TCPCONN
    XLOCAL_MSG((0,"Fallback to TCP/IP...\n"));
    return(MakeTCPConnection(phostname, idisplay, retries,
			     familyp, saddrlenp, saddrp));
#else
    return (-1);
#endif
}
#endif /* CLIENTS_LOCALCONN */
/*=========================================================================*/

#ifdef NOVELL
extern int errno;
#define MAX_AUTO_BUF_LEN 50
static int MakeSPX_TLIConnection (phostname, idisplay)
    char *phostname;
    int idisplay;
{
       int     i;
       char    service[MAX_AUTO_BUF_LEN];
       int     fd, type;
       struct nd_hostserv  nd_hostserv;
       struct netconfig   *netconfigp = NULL;
       struct nd_addrlist *nd_addrlistp = NULL;
       struct netbuf      *netbufp = NULL;
       struct t_call callbuf;
       void *handlep;

       nd_hostserv.h_host = phostname;
       sprintf(service,"%s%d", "xserver", idisplay);
       nd_hostserv.h_serv = service;
       if((handlep = setnetpath()) == NULL )
       {
          t_error("Error in network initialization for spx");
          return -1;
       }
/*
 * Look for an spx entry in /etc/netconfig
 *
*/

       while((netconfigp = getnetpath(handlep)) != NULL)
          if(!strcmp(netconfigp->nc_proto, "spx"))
             break;

       if(netconfigp == ( struct netconfig* )NULL)
       {
           nc_perror("getnetpath failed:"); 
           return -1;
       }
       if(netdir_getbyname(netconfigp, &nd_hostserv, 
                           &nd_addrlistp) == 0)
       {
           netbufp = nd_addrlistp->n_addrs;
           callbuf.addr.buf = netbufp->buf;
           callbuf.addr.len = netbufp->len;
           callbuf.addr.maxlen = netbufp->maxlen;
           callbuf.opt.buf = NULL;
           callbuf.opt.len = 0;
           callbuf.opt.maxlen = 0;
           callbuf.udata.buf = NULL;
           callbuf.udata.len = 0;
           callbuf.udata.maxlen = 0;
       }
       else
       {
           netdir_perror("netdir_getbyname failed:"); 
           return -1;
       }

       if(( fd = t_open(netconfigp->nc_device, O_RDWR, NULL)) < 0)
       {
           t_error("t_open failed for /dev/nspx:");
           return -1;
       }

       if( t_bind(fd, NULL, NULL) < 0 )
       {
           t_error("t_bind failed for /dev/nspx:");
           return -1;
       }
       endnetpath(handlep);
       if(t_connect(fd, &callbuf, NULL) < 0)
       {
           t_error("t_connect failed");
           t_close(fd);
           return -1;
        }
        if( ioctl( fd, I_PUSH, "tirdwr" ) < 0 ) {
    	   fprintf(stderr, "I_PUSH of tirdwr failed\n");
	   return -1;
        }
        return(fd);

} /* END of MakeSPX_TLIConnection() */

#endif /* NOVELL */




/*****************************************************************************
 *                                                                           *
 *			  Connection Utility Routines                        *
 *                                                                           *
 *****************************************************************************/

/* 
 * Disconnect from server.
 */

int _XDisconnectDisplay (server)

    int server;

{
    (void) close(server);
    return 0;
}

/*
 * This is an OS dependent routine which:
 * 1) returns as soon as the connection can be written on....
 * 2) if the connection can be read, must enqueue events and handle errors,
 * until the connection is writable.
 */
void
_XWaitForWritable(dpy)
    Display *dpy;
{
#ifdef SVR4
    fd_set r_mask;
    fd_set w_mask;
#else
    unsigned long r_mask[MSKCNT];
    unsigned long w_mask[MSKCNT];
#endif
    int nfound;

#ifdef SVR4
    FD_ZERO(&r_mask);
    FD_ZERO(&w_mask);
#else
    CLEARBITS(r_mask);
    CLEARBITS(w_mask);
#endif

    while (1) {

#ifdef SVR4
        FD_SET(dpy->fd, &r_mask);
        FD_SET(dpy->fd, &w_mask);
#else
	BITSET(r_mask, dpy->fd);
        BITSET(w_mask, dpy->fd);
#endif
	do {
#ifdef SVR4
	    nfound = select (dpy->fd + 1, &r_mask, &w_mask,
			     (fd_set *)NULL, (struct timeval *)NULL);
#else
	    nfound = select (dpy->fd + 1, r_mask, w_mask,
			     (unsigned long *)NULL, (struct timeval *)NULL);
#endif
	    if (nfound < 0 && errno != EINTR)
		_XIOError(dpy);
	} while (nfound <= 0);
#ifdef SVR4
	if (_XANYSET((unsigned long *)r_mask.fds_bits)) {
#else
	if (_XANYSET(r_mask)) {
#endif
	    char buf[BUFSIZE];
	    long pend_not_register;
	    register long pend;
	    register xEvent *ev;

	    /* find out how much data can be read */
	    if (BytesReadable(dpy->fd, (char *) &pend_not_register) < 0)
		_XIOError(dpy);
	    pend = pend_not_register;

	    /* must read at least one xEvent; if none is pending, then
	       we'll just block waiting for it */
	    if (pend < SIZEOF(xEvent)) pend = SIZEOF(xEvent);
		
	    /* but we won't read more than the max buffer size */
	    if (pend > BUFSIZE) pend = BUFSIZE;

	    /* round down to an integral number of XReps */
	    pend = (pend / SIZEOF(xEvent)) * SIZEOF(xEvent);

	    _XRead (dpy, buf, pend);

	    /* no space between comma and type or else macro will die */
	    STARTITERATE (ev,xEvent, buf, (pend > 0),
			  (pend -= SIZEOF(xEvent))) {
		if (ev->u.u.type == X_Error)
		    _XError (dpy, (xError *) ev);
		else		/* it's an event packet; enqueue it */
		    _XEnq (dpy, ev);
	    }
	    ENDITERATE
	}
#ifdef SVR4
	if (_XANYSET((unsigned long *)w_mask.fds_bits))
#else
	if (_XANYSET(w_mask))
#endif
	    return;
    }
}

void
_XWaitForReadable(dpy)
  Display *dpy;
{
#ifdef SVR4
    fd_set r_mask;
#else
    unsigned long r_mask[MSKCNT];
#endif
    int result;
	
#ifdef SVR4
    FD_ZERO(&r_mask);
#else
    CLEARBITS(r_mask);
#endif
    do {
#ifdef SVR4
        FD_SET(dpy->fd, &r_mask);
#else 
	BITSET(r_mask, dpy->fd);
#endif
#ifdef SVR4
	result = select(dpy->fd + 1, &r_mask,
			(fd_set *)NULL, (fd_set *)NULL,
			(struct timeval *)NULL);
#else
	result = select(dpy->fd + 1, r_mask,
			(unsigned long *)NULL, (unsigned long *)NULL,
			(struct timeval *)NULL);
#endif
	if (result == -1 && errno != EINTR) _XIOError(dpy);
    } while (result <= 0);
}


static int padlength[4] = {0, 3, 2, 1};	 /* make sure auth is multiple of 4 */

Bool
_XSendClientPrefix (dpy, client, auth_proto, auth_string)
     Display *dpy;
     xConnClientPrefix *client;		/* contains count for auth_* */
     char *auth_proto, *auth_string;	/* NOT null-terminated */
{
    int auth_length = client->nbytesAuthProto;
    int auth_strlen = client->nbytesAuthString;
    char padbuf[3];			/* for padding to 4x bytes */
    int pad;
    struct iovec iovarray[5], *iov = iovarray;
    int niov = 0;
    int len = 0;

#define add_to_iov(b,l) \
  { iov->iov_base = (b); iov->iov_len = (l); iov++, niov++; len += (l); }

    add_to_iov ((caddr_t) client, SIZEOF(xConnClientPrefix));

    /*
     * write authorization protocol name and data
     */
    if (auth_length > 0) {
	add_to_iov (auth_proto, auth_length);
	pad = padlength [auth_length & 3];
	if (pad) add_to_iov (padbuf, pad);
    }
    if (auth_strlen > 0) {
	add_to_iov (auth_string, auth_strlen);
	pad = padlength [auth_strlen & 3];
	if (pad) add_to_iov (padbuf, pad);
    }

#undef add_to_iov

    len -= WritevToServer (dpy->fd, iovarray, niov);

    /*
     * Set the connection non-blocking since we use select() to block.
     */
    /* ultrix reads hang on Unix sockets, hpux reads fail */
#if defined(O_NONBLOCK) && (!defined(ultrix) && !defined(hpux) && !defined(AIXV3) && !defined(uniosu))
    (void) fcntl (dpy->fd, F_SETFL, O_NONBLOCK);
#else
#ifdef FIOSNBIO
    {
	int arg = 1;
	ioctl (dpy->fd, FIOSNBIO, &arg);
    }
#else
#if (defined(AIXV3) || defined(uniosu)) && defined(FIONBIO)
    {
	int arg;
	arg = 1;
	ioctl(dpy->fd, FIONBIO, &arg);
    }
#else
    (void) fcntl (dpy->fd, F_SETFL, FNDELAY);
#endif
#endif
#endif
    return len == 0;
}


#ifdef STREAMSCONN
#ifdef SVR4
#include <tiuser.h>
#else
#undef HASXDMAUTH
#endif
#endif

#ifdef SECURE_RPC
#include <rpc/rpc.h>
#ifdef ultrix
#include <time.h>
#include <rpc/auth_des.h>
#endif
#endif

/*
 * First, a routine for setting authorization data
 */
static int xauth_namelen = 0;
static char *xauth_name = NULL;	 /* NULL means use default mechanism */
static int xauth_datalen = 0;
static char *xauth_data = NULL;	 /* NULL means get default data */

/*
 * This is a list of the authorization names which Xlib currently supports.
 * Xau will choose the file entry which matches the earliest entry in this
 * array, allowing us to prioritize these in terms of the most secure first
 */

static char *default_xauth_names[] = {
#ifdef SECURE_RPC
    "SUN-DES-1",
#endif
#ifdef HASXDMAUTH
    "XDM-AUTHORIZATION-1",
#endif
    "MIT-MAGIC-COOKIE-1"
};

static int default_xauth_lengths[] = {
#ifdef SECURE_RPC
    9,	    /* strlen ("SUN-DES-1") */
#endif
#ifdef HASXDMAUTH
    19,	    /* strlen ("XDM-AUTHORIZATION-1") */
#endif
    18	    /* strlen ("MIT-MAGIC-COOKIE-1") */
};

#define NUM_DEFAULT_AUTH    (sizeof (default_xauth_names) / sizeof (default_xauth_names[0]))
    
static char **xauth_names = default_xauth_names;
static int  *xauth_lengths = default_xauth_lengths;

static int  xauth_names_length = NUM_DEFAULT_AUTH;

void XSetAuthorization (name, namelen, data, datalen)
    int namelen, datalen;		/* lengths of name and data */
    char *name, *data;			/* NULL or arbitrary array of bytes */
{
    char *tmpname, *tmpdata;

    if (xauth_name) Xfree (xauth_name);	 /* free any existing data */
    if (xauth_data) Xfree (xauth_data);

    xauth_name = xauth_data = NULL;	/* mark it no longer valid */
    xauth_namelen = xauth_datalen = 0;

    if (namelen < 0) namelen = 0;	/* check for bogus inputs */
    if (datalen < 0) datalen = 0;	/* maybe should return? */

    if (namelen > 0)  {			/* try to allocate space */
	tmpname = Xmalloc ((unsigned) namelen);
	if (!tmpname) return;
	bcopy (name, tmpname, namelen);
    } else {
	tmpname = NULL;
    }

    if (datalen > 0)  {
	tmpdata = Xmalloc ((unsigned) datalen);
	if (!tmpdata) {
	    if (tmpname) (void) Xfree (tmpname);
	    return;
	}
	bcopy (data, tmpdata, datalen);
    } else {
	tmpdata = NULL;
    }

    xauth_name = tmpname;		/* and store the suckers */
    xauth_namelen = namelen;
    if (tmpname)
    {
	xauth_names = &xauth_name;
	xauth_lengths = &xauth_namelen;
	xauth_names_length = 1;
    }
    else
    {
	xauth_names = default_xauth_names;
	xauth_lengths = default_xauth_lengths;
	xauth_names_length = NUM_DEFAULT_AUTH;
    }
    xauth_data = tmpdata;
    xauth_datalen = datalen;
    return;
}

#ifdef SECURE_RPC
/*
 * Create a credential that we can send to the X server.
 */
static int
auth_ezencode(servername, window, cred_out, len)
        char           *servername;
        int             window;
	char	       *cred_out;
        int            *len;
{
        AUTH           *a;
        XDR             xdr;

        a = authdes_create(servername, window, NULL, NULL);
        if (a == (AUTH *)NULL) {
                perror("auth_create");
                return 0;
        }
        xdrmem_create(&xdr, cred_out, *len, XDR_ENCODE);
        if (AUTH_MARSHALL(a, &xdr) == FALSE) {
                perror("auth_marshall");
                AUTH_DESTROY(a);
                return 0;
        }
        *len = xdr_getpos(&xdr);
        AUTH_DESTROY(a);
	return 1;
}
#endif

/*ARGSUSED*/
static void
GetAuthorization(fd, family, saddr, saddrlen, idisplay,
		 auth_namep, auth_namelenp, auth_datap, auth_datalenp)
    int fd;
    int family;
    int saddrlen;
    int idisplay;
    char *saddr;
    char **auth_namep;			/* RETURN */
    int *auth_namelenp;			/* RETURN */
    char **auth_datap;			/* RETURN */
    int *auth_datalenp;			/* RETURN */
{
#ifdef SECURE_RPC
    char rpc_cred[MAX_AUTH_BYTES];
#endif
#ifdef HASXDMAUTH
    char xdmcp_data[192/8];
#endif
    char *auth_name;
    int auth_namelen;
    char *auth_data;
    int auth_datalen;
    Xauth *authptr = NULL;

/*
 * Look up the authorization protocol name and data if necessary.
 */
    if (xauth_name && xauth_data) {
	auth_namelen = xauth_namelen;
	auth_name = xauth_name;
	auth_datalen = xauth_datalen;
	auth_data = xauth_data;
    } else {
	char dpynumbuf[40];		/* big enough to hold 2^64 and more */
	(void) sprintf (dpynumbuf, "%d", idisplay);

	authptr = XauGetBestAuthByAddr ((unsigned short) family,
				    (unsigned short) saddrlen,
				    saddr,
				    (unsigned short) strlen (dpynumbuf),
				    dpynumbuf,
				    xauth_names_length,
				    xauth_names,
				    xauth_lengths);
	if (authptr) {
	    auth_namelen = authptr->name_length;
	    auth_name = (char *)authptr->name;
	    auth_datalen = authptr->data_length;
	    auth_data = (char *)authptr->data;
	} else {
	    auth_namelen = 0;
	    auth_name = NULL;
	    auth_datalen = 0;
	    auth_data = NULL;
	}
    }
#ifdef HASXDMAUTH
    /*
     * build XDM-AUTHORIZATION-1 data
     */
    if (auth_namelen == 19 && !strncmp (auth_name, "XDM-AUTHORIZATION-1", 19))
    {
	int     j;
	long    now;
	for (j = 0; j < 8; j++)
	    xdmcp_data[j] = auth_data[j];
#ifdef STREAMSCONN /* && SVR4 */
	{
	    int			i;
	    struct netbuf	netb;
	    char		addrret[1024];

	    netb.maxlen = sizeof addrret;
	    netb.buf = addrret;
	    if (t_getname (fd, &netb, LOCALNAME) == -1)
		t_error ("t_getname");
	    /*
	     * XXX - assumes that the return data
	     * are in a struct sockaddr_in, and that
	     * the data structure is layed out in
	     * the normal fashion.  This WILL NOT WORK
	     * on a non 32-bit machine (same in Xstreams.c)
	     */
	    for (i = 4; i < 8; i++)
		xdmcp_data[j++] = netb.buf[i];
	    for (i = 2; i < 4; i++)
		xdmcp_data[j++] = netb.buf[i];
	}
#else
	{
	    unsigned long	addr;
	    unsigned short	port;
#ifdef TCPCONN
	    int	    addrlen;
	    struct sockaddr_in	in_addr;

	    addrlen = sizeof (in_addr);
	    if (getsockname (fd,
			     (struct sockaddr *) &in_addr,
			     &addrlen) != -1 &&
		addrlen >= sizeof in_addr &&
		in_addr.sin_family == AF_INET)
	    {
		addr = ntohl (in_addr.sin_addr.s_addr);
		port = ntohs (in_addr.sin_port);
	    }
	    else
#endif
	    {
		static unsigned long	unix_addr = 0xFFFFFFFF;
		LockMutex();
		addr = unix_addr--;
		UnlockMutex();
		port = getpid ();
	    }
	    xdmcp_data[j++] = (addr >> 24) & 0xFF;
	    xdmcp_data[j++] = (addr >> 16) & 0xFF;
	    xdmcp_data[j++] = (addr >>  8) & 0xFF;
	    xdmcp_data[j++] = (addr >>  0) & 0xFF;
	    xdmcp_data[j++] = (port >>  8) & 0xFF;
	    xdmcp_data[j++] = (port >>  0) & 0xFF;
	}
#endif
	time (&now);
	xdmcp_data[j++] = (now >> 24) & 0xFF;
	xdmcp_data[j++] = (now >> 16) & 0xFF;
	xdmcp_data[j++] = (now >>  8) & 0xFF;
	xdmcp_data[j++] = (now >>  0) & 0xFF;
	while (j < 192 / 8)
	    xdmcp_data[j++] = 0;
	XdmcpWrap (xdmcp_data, auth_data + 8,
		      xdmcp_data, j);
	auth_data = xdmcp_data;
	auth_datalen = j;
    }
#endif /* HASXDMAUTH */
#ifdef SECURE_RPC
    /*
     * The SUN-DES-1 authorization protocol uses the
     * "secure RPC" mechanism in SunOS 4.0+.
     */
    if (auth_namelen == 9 && !strncmp(auth_name, "SUN-DES-1", 9)) {
	char servernetname[MAXNETNAMELEN + 1];

	/*
	 * Copy over the server's netname from the authorization
	 * data field filled in by XauGetAuthByAddr().
	 */
	if (auth_datalen > MAXNETNAMELEN) {
	    auth_datalen = 0;
	    auth_data = NULL;
	} else {
	    bcopy(auth_data, servernetname, auth_datalen);
	    servernetname[auth_datalen] = '\0';

	    auth_datalen = sizeof (rpc_cred);
	    if (auth_ezencode(servernetname, 100, rpc_cred,
			      &auth_datalen))
		auth_data = rpc_cred;
	    else
		auth_data = NULL;
	}
    }
#endif
    if (saddr) Xfree (saddr);
    if (*auth_namelenp = auth_namelen)
    {
	if (*auth_namep = Xmalloc(auth_namelen))
	    bcopy(auth_name, *auth_namep, auth_namelen);
	else
	    *auth_namelenp = 0;
    }
    else
	*auth_namep = NULL;
    if (*auth_datalenp = auth_datalen)
    {
	if (*auth_datap = Xmalloc(auth_datalen))
	    bcopy(auth_data, *auth_datap, auth_datalen);
	else
	    *auth_datalenp = 0;
    }
    else
	*auth_datap = NULL;
    if (authptr) XauDisposeAuth (authptr);
}
