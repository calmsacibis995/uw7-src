#ifndef	NOIDENT
#ident	"@(#)button:RectButton.h	1.6"
#endif

/*
 ************************************************************
 *
 *  Date:	August 12, 1988
 *
 *  Description:
 *	This file contains the definitions for the
 *	OPEN LOOK(tm) RectButton widget.
 *
 ************************************************************
 */

#include <Xol/Button.h>

#ifndef _OlRectButton_h
#define _OlRectButton_h

/*
 *  rectButtonWidgetClass is defined in RectButton.c
 */
extern WidgetClass rectButtonWidgetClass;

typedef struct _RectButtonClassRec   *RectButtonWidgetClass;
typedef struct _RectButtonRec        *RectButtonWidget;

#endif /*  _OlRectButton_h  */
/* DON'T ADD STUFF AFTER THIS */
