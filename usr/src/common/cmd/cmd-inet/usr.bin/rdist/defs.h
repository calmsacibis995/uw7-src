#ident	"@(#)defs.h	1.2"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)defs.h	5.8 (Berkeley) 6/1/90
 */

#if !defined(_DEFS_H_)
#define _DEFS_H_

#include <sys/param.h>
#if !defined(SYSV)
#include <sys/dir.h>
#else
#include <sys/types.h>
#include <dirent.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#if defined(SYSV)
struct utimbuf {
	time_t	actime;		/* Access time */
	time_t	modtime;	/* Modification time */
};
#include <sys/fcntl.h>
#include <signal.h>
#include <limits.h>
#ifndef NCARGS
#define NCARGS	ARG_MAX
#endif
#endif
#include "pathnames.h"

#if defined(SYSV)
#define	index	strchr
#define rindex	strrchr
#endif
/*
 * The version number should be changed whenever the protocol changes.
 */
#define VERSION	 3

	/* defines for yacc */
#define EQUAL	1
#define LP	2
#define RP	3
#define SM	4
#define ARROW	5
#define COLON	6
#define DCOLON	7
#define NAME	8
#define STRING	9
#define INSTALL	10
#define NOTIFY	11
#define EXCEPT	12
#define PATTERN	13
#define SPECIAL	14
#define OPTION	15

	/* lexical definitions */
#define	QUOTE 	0200		/* used internally for quoted characters */
#define	TRIM	0177		/* Mask to strip quote bit */

	/* table sizes */
#define HASHSIZE	1021
#define INMAX	3500

	/* option flags */
#define VERIFY	0x1
#define WHOLE	0x2
#define YOUNGER	0x4
#define COMPARE	0x8
#define REMOVE	0x10
#define FOLLOW	0x20
#define IGNLNKS	0x40

	/* expand type definitions */
#define E_VARS	0x1
#define E_SHELL	0x2
#define E_TILDE	0x4
#define E_ALL	0x7

	/* actions for lookup() */
#define LOOKUP	0
#define INSERT	1
#define REPLACE	2

#define ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

#define ALLOC(x) (struct x *) malloc(sizeof(struct x))

struct namelist {	/* for making lists of strings */
	char	*n_name;
	struct	namelist *n_next;
};

struct subcmd {
	short	sc_type;	/* type - INSTALL,NOTIFY,EXCEPT,SPECIAL */
	short	sc_options;
	char	*sc_name;
	struct	namelist *sc_args;
	struct	subcmd *sc_next;
};

struct cmd {
	int	c_type;		/* type - ARROW,DCOLON */
	char	*c_name;	/* hostname or time stamp file name */
	char	*c_label;	/* label for partial update */
	struct	namelist *c_files;
	struct	subcmd *c_cmds;
	struct	cmd *c_next;
};

struct linkbuf {
	ino_t	inum;
	dev_t	devnum;
	int	count;
	char	pathname[BUFSIZ];
	char	target[BUFSIZ];
	struct	linkbuf *nextp;
};

#if defined(SVR4)
#define setreuid(a,b) seteuid(b)
#endif

extern int debug;		/* debugging flag */
extern int nflag;		/* NOP flag, don't execute commands */
extern int qflag;		/* Quiet. don't print messages */
extern int options;		/* global options */

extern int nerrs;		/* number of errors seen */
extern int rem;			/* remote file descriptor */
extern int iamremote;		/* acting as remote server */
extern char tmpfilename[];	/* file name for logging changes */
extern struct linkbuf *ihead;	/* list of files with more than one link */
extern struct passwd *pw;	/* pointer to static area used by getpwent */
extern struct group *gr;	/* pointer to static area used by getgrent */
extern char host[];		/* host name of master copy */
extern char buf[];		/* general purpose buffer */
extern int errno;		/* system error number */

extern struct group *getgrent();
extern struct group *getgrgid();
extern struct group *getgrnam();

char *makestr();
struct namelist *makenl();
struct subcmd *makesubcmd();
struct namelist *lookup();
struct namelist *expand();
char *exptilde();
#if defined(u3b2)
struct passwd *getpwuid();
struct passwd *getpwnam();
#endif
#if !defined(SVR4)
void *malloc();
#endif
char *rindex();
char *index();
#endif
