#ifndef NOIDENT
#ident	"@(#)olmisc:OlStrings.c	1.21"
#endif

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifdef R5
#include <X11/Intrinsic.h>
#endif

#include <stdio.h>
/*
 *************************************************************************
 *
 * Description:
 *	This file contains external definitions for OPEN LOOK (Tm)
 *	resource names, resource classes and resource types.
 *
 ******************************file*header********************************
 */

/*
 * Stylized names like XtNfoo, XtCfoo, XtRfoo should be defined
 * in the following common file (included by both OlStrings.c and
 * OlStrings.h), as follows:
 *
 *	STRING (XtN,foo);	no spaces inside the () !!
 *
 * This produces
 *
 *	 char XtNfoo[] = "foo";
 *
 * Other names that don't fit this style should be defined here
 * and declared in OlStrings.h.
 */

		/* Use `const' to define the `STRING' marco if ANSI C	*/
#if defined(__STDC__)

# define STRING(prefix,string)    const char prefix ## string [] = #string;
# define VSTRING(prefix,string,v) const char prefix ## string [] = #v;

#else

# define lq(x) "x
# define rq(x) x"
# define quote(x) rq(lq(x))
# define STRING(prefix,string)    char prefix/**/string [] = quote(string);
# define VSTRING(prefix,string,v) char prefix/**/string [] = quote(v);

#endif

#if defined(XtSpecificationRelease) && XtSpecificationRelease != 5
#include "Xol/XtStrings"
#endif
#undef XtCResize
#include "Xol/StringList"

#if defined(__STDC__)

#undef STRING
#undef VSTRING
# define STRING(prefix,string)    prefix ## string ,
# define VSTRING(prefix,string,v) prefix ## string ,

static const char * OlStringsArray[] = {

#else

#undef STRING
#undef VSTRING
# define STRING(prefix,string)    prefix/**/string ,
# define VSTRING(prefix,string,v) prefix/**/string ,

static char * OlStringsArray[] = {

#endif

#if defined(XtSpecificationRelease) && XtSpecificationRelease != 5
#include <Xol/XtStrings>
#endif
#include <Xol/StringList>

(char *)0
};

/* This is not a good place to keep this file because this file	*/
/* only contains "variable" definitions. One other reason is	*/
/* that we can't use prototype checking...			*/
extern void
_OlLoadQuarkTable()
{
	/* XlibSpecificationRelease and XrmPermStringToQuark()
	 * are introduced in X11R5.
	 */
#if defined(XlibSpecificationRelease)
	char ** p = (char **)OlStringsArray;
	char *  env_switch = (char *)getenv("NOPERMQUARK");

	if (!env_switch)
		while (*p)
			(void)XrmPermStringToQuark(*p++);
#endif
} /* end of _OlLoadQuarkTable */
