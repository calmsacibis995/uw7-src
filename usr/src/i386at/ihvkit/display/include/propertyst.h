#ident	"@(#)ihvkit:display/include/propertyst.h	1.1"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/* $XConsortium: propertyst.h,v 1.2 88/09/06 15:49:12 jim Exp $ */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
#ifndef PROPERTYSTRUCT_H
#define PROPERTYSTRUCT_H 
#include "misc.h"
#include "property.h"
/* 
 *   PROPERTY -- property element
 */

typedef struct _Property {
        struct _Property       *next;
	ATOM 		propertyName;
	ATOM		type;       /* ignored by server */
	short		format;     /* format of data for swapping - 8,16,32 */
	long		size;       /* size of data in (format/8) bytes */
	pointer         data;       /* private to client */
} PropertyRec;

extern int ProcGetProperty();
extern int ProcListProperties();
extern int ProcChangeProperty();

#endif /* PROPERTYSTRUCT_H */

