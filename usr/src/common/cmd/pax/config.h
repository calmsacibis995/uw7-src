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

#ident	"@(#)pax:config.h	1.1"

/*
 * OSF/1 1.2
 */
/* @(#)$RCSfile$ $Revision$ (OSF) $Date$ */
/* 
 * config.h - configuration options for PAX
 *
 * DESCRIPTION
 *
 *	This file contains a number of configurable parameters for the
 *	PAX software.  This files should be edited prior to makeing the
 *	package.
 *
 * AUTHOR
 *
 *	Mark H. Colburn, NAPS International (mark@jhereg.mn.org)
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

#ifndef _PAX_CONFIG_H
#define _PAX_CONFIG_H

/* Defines */

/*
 *
 * Define USG if you want the default tar blocking factor to be 1
 */
/* #define USG 	/* Running on a USG System */

/*
 * DEF_AR_FILE - tar only (required)
 *
 * DEF_AR_FILE should contain the full pathname of your favorite archive
 * device.  Normally this would be a tape drive, but it may be a disk drive
 * on those systems that don't have tape drives.
 */
#define DEF_AR_FILE	"-"	/* The default archive on your system */

/*
 * TTY - device which interactive queries should be directed to (required)
 *
 * This is the device to which interactive queries will be sent to and
 * received from.  On most unix systems, this should be /dev/tty, however, on
 * some systems, such as MS-DOS, it my need to be different (e.g. "con:").
 */
#define	TTY	"/dev/tty"	/* for most versions of UNIX */

/*
 * OFFSET - compiler dependent offset type
 * 
 * OFFSET is the type which is returned by lseek().  It is different on
 * some systems.  Most define it to be off_t, but some define it to be long.
 */
#define OFFSET	off_t	/* for most BSD, USG and other systems */
/* #define OFFSET	long	/* for most of the rest of them... */

/*
 * VOID - compiler support for VOID types
 *
 * If your system does not support void, then this should be defined to
 * int, otherwise, it should be left undefined.
 *
 * For ANSI Systems this should always be blank.
 */

/*
 * SIG_T - return type for the signal routine
 *
 * Some systems have signal defines to return an int *, other return a
 * void *.  Please choose the correct value for your system.
 */
#define SIG_T	void	/* signal defined as "void (*signal)()" */
/* #define SIG_T	int	/* signal defined as "int (*signal)()" */

#endif /* _PAX_CONFIG_H */
