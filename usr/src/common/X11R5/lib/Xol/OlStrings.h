/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olmisc:OlStrings.h	1.45.1.17"
#endif

#if	!defined(_OlStrings_h)
#define _OlStrings_h

/*
 *************************************************************************
 *
 * Description:
 *	This file contains external declarations for OPEN LOOK (Tm)
 *	resource names, resource classes and resource types.
 *
 ******************************file*header********************************
 */

		/* Use `const' to define the `STRING' marco if ANSI C	*/
#if defined(__STDC__)

# define STRING(prefix,string)    extern char prefix ## string [];
# define VSTRING(prefix,string,v) extern char prefix ## string [];

#else

# define STRING(prefix,string)    extern char prefix/**/string [];
# define VSTRING(prefix,string,v) extern char prefix/**/string [];

#endif

/*
 * Stylized names like XtNfoo, XtCfoo, XtRfoo should be defined
 * in the following common file (included by both OlStrings.c and
 * OlStrings.h), as follows:
 *
 *	STRING (XtN,foo) 	no spaces inside the () !!
 *
 * This produces
 *
 *	 extern char XtNfoo[];
 *
 * Other names that don't fit this style should be declared here
 * and defined in OlStrings.c.
 */

#include "Xol/StringList"

#undef	STRING
#undef	VSTRING

#endif
