#ifndef _RPCSVC_YPCLNT_H
#define _RPCSVC_YPCLNT_H

#ident	"@(#)cplusinc:rpcsvc/ypclnt.h	1.1"
#ident  "$Header:  "

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 
/*	@(#)ypclnt.h 1.13 88/02/08 Copyr 1985 Sun Microsystems, Inc	*/

/*
 * ypclnt.h
 * This defines the symbols used in the c language
 * interface to the yp client functions.  A description of this interface
 * can be read in ypclnt(3N).
 */

/*
 * Failure reason codes.  The success condition is indicated by a functional
 * value of "0".
 */
#define YPERR_BADARGS 1			/* Args to function are bad */
#define YPERR_RPC 2			/* RPC failure */
#define YPERR_DOMAIN 3			/* Can't bind to a server which serves
					 *   this domain. */
#define YPERR_MAP 4			/* No such map in server's domain */
#define YPERR_KEY 5			/* No such key in map */
#define YPERR_YPERR 6			/* Internal yp server or client
					 *   interface error */
#define YPERR_RESRC 7			/* Local resource allocation failure */
#define YPERR_NOMORE 8			/* No more records in map database */
#define YPERR_PMAP 9			/* Can't communicate with portmapper */
#define YPERR_YPBIND 10			/* Can't communicate with ypbind */
#define YPERR_YPSERV 11			/* Can't communicate with ypserv */
#define YPERR_NODOM 12			/* Local domain name not set */
#define YPERR_BADDB 13			/*  yp data base is bad */
#define YPERR_VERS 14			/* YP version mismatch */
#define YPERR_ACCESS 15			/* Access violation */
#define YPERR_BUSY 16			/* Database is busy */

/*
 * Types of update operations
 */
#define YPOP_CHANGE 1			/* change, do not add */
#define YPOP_INSERT 2			/* add, do not change */
#define YPOP_DELETE 3			/* delete this entry */
#define YPOP_STORE  4			/* add, or change */
 
 

/*           
 * Data definitions
 */

/*
 * struct ypall_callback * is the arg which must be passed to yp_all
 */

struct ypall_callback {
	int (*foreach)(int, char *, int, char *, int, char *);
					/* Return non-0 to stop getting
					 *  called */
	char *data;			/* Opaque pointer for use of callback
					 *   function */
};

extern int _yp_dobind();

/*
 * External yp client function references. 
 */

#if defined(__STDC__)
extern const char *yperr_string(const int);
extern int yp_bind(char *);
extern void yp_unbind(char *);
extern int yp_get_default_domain(char **);
extern int yp_match(char *, char *, char *, int, char **, int *);
extern int yp_first(char *, char *, char **, int *, char **, int *);
extern int yp_next(char *, char *, char *, int, char **, int *, char **, int *);
extern int yp_master(char *, char *, char **);
extern int yp_order(char *, char *, int *);
extern int yp_all(char *, char *, struct ypall_callback *);
extern int ypprot_err(unsigned int);
extern int yp_update(char *, char *, unsigned int, char *, int, char *, int);
#else
extern char *yperr_string();
extern int yp_bind();
extern void yp_unbind();
extern int yp_get_default_domain ();
extern int yp_match ();
extern int yp_first ();
extern int yp_next();
extern int yp_master();
extern int yp_order();
extern int yp_all();
extern int ypprot_err();
extern int yp_update();
#endif

/*
 * Global yp data structures
 */

#if defined(__cplusplus)
	}
#endif

#endif /* _RPCSVC_YPCLNT_H */
