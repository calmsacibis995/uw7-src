#ifndef	NOIDENT
#ident	"@(#)buttonstack:MenuButton.h	1.4"
#endif

#ifndef _Ol_MenuButton_h_
#define _Ol_MenuButton_h_

/*
 ************************************************************************
 *
 * Description:
 *		This is the "public" include file for the MenuButton
 *	Widget and Gadget.
 *
 *****************************file*header********************************
 */

#include <Xol/Button.h>
#include <X11/Shell.h>			/* need this for XtNtitle	*/

extern WidgetClass				menuButtonWidgetClass;
typedef struct _MenuButtonClassRec *		MenuButtonWidgetClass;
typedef struct _MenuButtonRec *			MenuButtonWidget;

extern WidgetClass				menuButtonGadgetClass;
typedef struct _MenuButtonGadgetClassRec *	MenuButtonGadgetClass;
typedef struct _MenuButtonGadgetRec *		MenuButtonGadget;

#endif /* _Ol_MenuButton_h_ */
