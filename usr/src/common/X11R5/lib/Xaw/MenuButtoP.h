#ident	"@(#)r5Xaw:MenuButtoP.h	1.4"

/* $XConsortium: MenuButtoP.h,v 1.7 91/09/23 11:25:56 converse Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
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


/***********************************************************************
 *
 * MenuButton Widget
 *
 ***********************************************************************/

/*
 * MenuButtonP.h - Private Header file for MenuButton widget.
 *
 * This is the private header file for the Athena MenuButton widget.
 * It is intended to provide an easy method of activating pulldown menus.
 *
 * Date:    May 2, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium 
 *          kit@expo.lcs.mit.edu
 */

#ifndef _XawMenuButtonP_h
#define _XawMenuButtonP_h

#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/CommandP.h>

/************************************
 *
 *  Class structure
 *
 ***********************************/


   /* New fields for the MenuButton widget class record */
typedef struct _MenuButtonClass 
{
  int makes_compiler_happy;  /* not used */
} MenuButtonClassPart;

   /* Full class record declaration */
typedef struct _MenuButtonClassRec {
  CoreClassPart	    core_class;
  SimpleClassPart	    simple_class;
  LabelClassPart	    label_class;
  CommandClassPart	    command_class;
  MenuButtonClassPart     menuButton_class;
} MenuButtonClassRec;

extern MenuButtonClassRec menuButtonClassRec;

/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/

    /* New fields for the MenuButton widget record */
typedef struct {
  /* resources */
  String menu_name;

} MenuButtonPart;

   /* Full widget declaration */
typedef struct _MenuButtonRec {
    CorePart         core;
    SimplePart	     simple;
    LabelPart	     label;
    CommandPart	     command;
    MenuButtonPart   menu_button;
} MenuButtonRec;

#endif /* _XawMenuButtonP_h */


