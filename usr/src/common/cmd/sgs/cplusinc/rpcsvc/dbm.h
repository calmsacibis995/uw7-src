/*		copyright	"%c%" 	*/

#ident	"@(#)cplusinc:rpcsvc/dbm.h	1.1"
#ident  "$Header:  "

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


#ifndef _RPCSVC_DBM_H	/* wrapper symbol */
#define _RPCSVC_DBM_H	/* subject to change without notice */

#if defined(__cplusplus)
extern "C" {
#endif

#define	PBLKSIZ	1024
#define	DBLKSIZ	4096
#define	BYTESIZ	8
#ifndef NULL
#define	NULL	((char *) 0)
#endif

long	bitno;
long	maxbno;
long	blkno;
long	hmask;

char	pagbuf[PBLKSIZ];
char	dirbuf[DBLKSIZ];

int	dirf;
int	pagf;
int	dbrdonly;

typedef	struct
{
	char	*dptr;
	int	dsize;
} datum;

#ifdef __STDC__

extern int	dbminit(char *);
extern int	dbmclose(void);
extern datum	fetch(datum);
extern int	store(datum, datum);
#ifndef __cplusplus
extern int	delete(datum);
#endif
extern datum	firstkey(void);
extern datum	nextkey(datum);

#else

extern int	dbminit();
extern int	dbmclose();
extern datum	fetch();
extern int	store();
extern int	delete();
extern datum	firstkey();
extern datum	nextkey();

#endif

/* The following  function declarations appear to be needed as forward
 * function declarations when compiling dbm.c for libnsl.so.  These
 * routines probably should be static functions of dbm.c and not part
 * of the libnsl name space.
 */
datum	makdatum();
datum	firsthash();
long	calchash();
long	hashinc();

#if defined(__cplusplus)
	}
#endif

#endif /* _RPCSVC_DBM_H */
