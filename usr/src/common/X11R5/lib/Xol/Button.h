/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)button:Button.h	1.15"
#endif

#ifndef _Ol_Button_h_
#define _Ol_Button_h_

/*
 ************************************************************
 *
 *  Description:
 *	This file contains the source for the OPEN LOOK(tm)
 *	Button widget and gadget.
 *
 ************************************************************
 */

#include <Xol/Primitive.h>		/* include widget's superclass header */
#include <Xol/EventObj.h>		/* include gadget's superclass header */

/*
 *  These defines are for the scale resource.
 */
#define SMALL_SCALE	10
#define MEDIUM_SCALE	12
#define LARGE_SCALE	14
#define EXTRA_LARGE_SCALE	19

/*
 *  buttonWidgetClass is defined in Button.c
 */
extern WidgetClass			buttonWidgetClass;
typedef struct _ButtonClassRec *	ButtonWidgetClass;
typedef struct _ButtonRec *		ButtonWidget;

/*
 *  buttonGadgetClass is defined in Button.c
 */
extern WidgetClass			buttonGadgetClass;
typedef struct _ButtonGadgetClassRec *	ButtonGadgetClass;
typedef struct _ButtonGadgetRec *	ButtonGadget;

#endif /* _Ol_Button_h_ */
