/*
 * COPYRIGHT NOTICE
 * 
 * This source code is designated as Restricted Confidential Information
 * and is subject to special restrictions in a confidential disclosure
 * agreement between HP, IBM, SUN, NOVELL and OSF.  Do not distribute
 * this source code outside your company without OSF's specific written
 * approval.  This source code, and all copies and derivative works
 * thereof, must be returned or destroyed at request. You must retain
 * this notice on any copies which you make.
 * 
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED 
 */

#ident	"@(#)pax:func.h	1.1"

/*
 * OSF/1 1.2
 */
/* @(#)$RCSfile$ $Revision$ (OSF) $Date$ */
/*
 * func.h - function type and argument declarations
 *
 * DESCRIPTION
 *
 *	This file contains function delcarations in both ANSI style
 *	(function prototypes) and traditional style. 
 *
 * AUTHOR
 *
 *     Mark H. Colburn, NAPS International (mark@jhereg.mn.org)
 *
 * Sponsored by The USENIX Association for public distribution. 
 *
 * Copyright (c) 1989 Mark H. Colburn.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Mark H. Colburn and sponsored by The USENIX Association. 
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef _PAX_FUNC_H
#define _PAX_FUNC_H

/* Function Prototypes */


extern Link    	       *linkfrom(char *, Stat *);
extern Link    	       *linkto(char *, Stat *);
extern char    	       *mem_get(uint);
extern char    	       *mem_str(char *);
extern char            *mem_rpl_name(char *);
extern int      	ar_read(void);
extern int      	buf_read(char *, uint);
extern int      	buf_skip(OFFSET);
extern int      	create_archive(void);
extern int      	dirneed(char *);
extern int      	read_archive(void);
extern int      	inentry(char *, Stat *);
extern int      	lineget(FILE *, char *);
extern int      	name_match(char *, int);
extern int      	name_next(char *, Stat *);
extern int      	nameopt(char *);
extern int      	open_archive(int);
extern int      	open_tty(void);
extern int      	openin(char *, Stat *);
extern int      	openout(char *, Stat *, Link *, int);
extern int      	pass(char *);
extern int      	passitem(char *, Stat *, int, char *);
extern int      	read_header(char *, Stat *);
extern void     	buf_allocate(OFFSET);
extern void		backup(void);
extern void     	close_archive(void);
extern void     	fatal(char *);
extern void     	name_gather(void);
extern void     	name_init(int, char **);
extern void     	names_notfound(void);
extern void     	next(int);
extern int      	nextask(char *, char *, int);
extern void     	outdata(int, char *, Stat *);
extern void     	outwrite(char *, uint);
extern void     	passdata(char *, int, char *, int);
extern void     	print_entry(char *, Stat *);
extern void		reset_directories(void);
extern void     	warn();
extern void		warnarch(char *, OFFSET);
extern void     	write_eot(void);
extern void		get_archive_type(void);
extern struct group    *getgrgid();
extern struct group    *getgrnam();
extern struct passwd   *getpwuid();
extern SIG_T   	      (*signal())();
extern Link            *islink(char *, Stat *);
extern char            *finduname(int);
extern char            *findgname(int);
extern int		findgid(char *);
extern time_t 	       	hash_lookup(char *);
extern void 		hash_name(char *, Stat *);
extern int		charmap_convert(char *);
extern void		regsub(regmatch_t *, char *, char *, char *);

#endif /* _PAX_FUNC_H */
