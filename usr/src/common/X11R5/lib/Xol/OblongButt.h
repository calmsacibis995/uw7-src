/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)button:OblongButt.h	1.10"
#endif

#ifndef _Ol_OblongButt_h_
#define _Ol_OblongButt_h_

/*
 ************************************************************
 *
 *  Description:
 *	This file contains the definitions for the
 *	OPEN LOOK(tm) OblongButton widget and gadget.
 *
 ************************************************************
 */

#include <Xol/Button.h>

/*
 *  oblongButtonWidgetClass is defined in  OblongButton.c
 */
extern WidgetClass				oblongButtonWidgetClass;
typedef struct _OblongButtonClassRec *		OblongButtonWidgetClass;
typedef struct _OblongButtonRec *		OblongButtonWidget;

/*
 *  oblongButtonGadgetClass is defined in  OblongButton.c
 */
extern WidgetClass				oblongButtonGadgetClass;
typedef struct _OblongButtonGadgetClassRec *	OblongButtonGadgetClass;
typedef struct _OblongButtonGadgetRec *		OblongButtonGadget;

#endif /* _Ol_OblongButt_h */
