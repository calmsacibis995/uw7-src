#ident	"%W"

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL.                                           	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef __UTIL_H

#define __UTIL_H
#include <sys/types.h>
#define _POSIX_SOURCE
#include <sys/signal.h>
#undef _POSIX_SOURCE
#include <sys/socket.h>

#include <stdio.h>
#include <stdarg.h>
#include <sys/fcntl.h>				/* for O_RDWR */
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/stream.h>
#include <sys/termios.h>
#include <sys/tiuser.h>
#include <sys/tp.h>
#include <sys/sockmod.h>
#include <sys/osocket.h>
#include <netdir.h>
#include <sys/stropts.h>

#include <sys/ioctl.h>
#include <sys/file.h>
#include <syslog.h>

extern int      t_errno;
extern int      errno;

#define 	TRUE	1
#define		FALSE	0
#ifdef TRANS_DEBUG
#define		PROVIDER_NAME	"tcp"
#else
#define		PROVIDER_NAME	"spx"
#endif

/*
 * Where the error messages should go
 */
#define TO_SYSLOG  1
#define TO_STDERR  2
#define TO_DEVNULL 3

/*
 * Transport release type
 */
#define DIS_ORDERLY  T_COTS
#define ORDERLY  T_COTS_ORD

/*
 *  Function prototypes all in util.c
 */
int	BindReserved(char *, char *, int, struct t_call **);
int	ListenAndAccept(int, struct t_call *);
int	ReceiveData(int, char *, int);
int	BindReservedAndConnect(char *, char *, int);
int	SendData(int, char *, int);
struct netbuf *get_addrs(char *, char *, int);
extern void tli_error(char *msg , int dir);
extern void tet_msg(char *, ...);

#ifdef COMMENT
extern int BindAddressAndConnect(struct netconfig *,struct netbuf *);
extern int BindPortAndConnect(struct netconfig *,int *,char *);
extern int BindReservedAndConnect( struct netconfig *, struct netbuf *);
extern int BindAndAccept(int , struct t_call **);
extern int BindAndListen(struct netconfig *, int *port);
extern int BindAndListenOnWellKnown(struct netconfig *,struct netbuf *, \
                                                      struct t_call **);
extern int AcceptConnection(int,struct t_call **);
extern int BindRSAndListen(struct netconfig *,int *,char *);
extern int create_netbuf_arg(char *[],int ,struct netbuf *,struct netconfig *);
extern int decode_netbuf_arg(char *[], int ,struct netbuf *,char *);
extern void tstNsetsig(int signo,void (*sigstate)(), void (*)());
extern void tli_error(char *msg , int dir);
extern int releaseNcloseTransport(int fd,int dir,int flags);
extern void tet_msg(char *, ...);
#endif
/*
 *  BSD equates
 */
#ifndef sigmask
#define sigmask(m)	(1 << (((m) -1)))
#endif
#define rindex		strrchr
#define index		strchr
/*
 *  Some Debug macros
 */
#if !defined(DEBUG) && !defined(SYSLOG)
#define PRMSG(x,a,b)
#endif

#ifdef DEBUG
#define PRMSG(x,a,b)	fprintf(stderr, x,a,b); fflush(stderr)
#endif
#ifdef SYSLOG
#define PRMSG(x,a,b)	syslog(LOG_DEBUG,x,a,b)
#endif

#define IPXPORT_RESERVED 0x4CFF
#define	STARTPORT 0x4C00
#define	ENDPORT (IPXPORT_RESERVED - 1)
#define	NPORTS	(ENDPORT - STARTPORT + 1)

#define	END_DYNM_RESERVED 	IPXPORT_RESERVED
#define	START_DYNM_RESERVED 		STARTPORT
#define	START_STATIC_RESERVED 	36917
#define	END_STATIC_RESERVED 	36985

#endif
