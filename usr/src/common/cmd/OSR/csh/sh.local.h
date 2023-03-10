#ident	"@(#)OSRcmds:csh/sh.local.h	1.1"
#pragma comment(exestr, "@(#) sh.local.h 25.1 93/07/20 ")
/*
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1993 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/* #ident	"@(#)csh:sh.local.h	1.2" */

/*
 *	L000	scol!hughd	19nov90
 *	- dispose of compiler warnings: remove incorrect NOFILE defn
 *
 *	L001	scol!gregw	01jul93
 *	- added new define that defines the maximum length of a variable
 *	  name. From greim@sbsvax.UUCP (Michael Greim)
 */

/*
 *	@(#) sh.local.h 1.1 88/03/29 csh:sh.local.h
 */

/* Copyright (c) 1980 Regents of the University of California */
/*
 * This file defines certain local parameters
 * A symbol should be defined in Makefile for conditional
 * compilation, e.g. CORY for U.C.B. Cory Hall 11/70 and
 * tested here and elsewhere.
 */

/*
 * Fundamental definitions which may vary from system to system.
 *
 *	BUFSIZ		The i/o buffering size; also limits word size
 *	SHELLPATH	Where the shell will live; initalizes $shell
 *	SRCHPATH	The directories in the default search path
 *	MAILINTVL	How often to mailcheck; more often is more expensive
 */

#ifdef VMUNIX
#include <pagsiz.h>
#define BUFSIZ		BSIZE
#else
# ifndef BUFSIZ
#  define BUFSIZ		1024
# endif
#endif

#define	SHELLPATH	"/bin/csh"
#define	OTHERSH		"/bin/sh"
/*
 * Note that the first component of SRCHPATH is set to /etc for root
 * in the file sh.c.
 *
 * Note also that the SRCHPATH is meaningless unless you are on a v6
 * system since the default path will be imported from the environment.
 */
#define	SRCHPATH	".", "/usr/ucb", "/bin", "/usr/bin"
#define	MAILINTVL	300				/* 10 minutes */

/*
 * NCARGS is from <sys/param.h> which we choose not to wholly include
 */
#define	NCARGS	5120		/* Max. chars in an argument list */

/*
 *								L001
 * MAX_NAME_LENGTH is maximum number of chars in a variable name.
 */
#define MAX_NAME_LEN	20

/*
 * The shell moves std in/out/diag and the old std input away from units
 * 0, 1, and 2 so that it is easy to set up these standards for invoked
 * commands.  If possible they should go into descriptors closed by exec.
 */
#define	FSHIN	16		/* Preferred desc for shell input */
#define	FSHOUT	17		/* ... shell output */
#define	FSHDIAG	18		/* ... shell diagnostics */
#define	FOLDSTD	19		/* ... old std input */

#define	V7

#ifdef	V69
#undef	V7
#define V6
#include <retrofit.h>
#define	NCARGS	3100
#define	NOFILE	15
#define	FSHIN	3
#define	FSHOUT	4
#define	FSHDIAG	5
#define	FOLDSTD	6
#endif

#ifdef	NORMAL6
#undef	V7
#define V6
#include <retrofit.h>
#define	NCARGS	510
#define	NOFILE	15
#define	FSHIN	3
#define	FSHOUT	4
#define	FSHDIAG	5
#define	FOLDSTD	6
#endif

#ifdef	CC
#define	NCARGS	5120
#endif
