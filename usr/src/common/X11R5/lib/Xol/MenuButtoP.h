#ifndef	NOIDENT
#ident	"@(#)buttonstack:MenuButtoP.h	1.3"
#endif

#ifndef _Ol_MenuButtoP_h_
#define _Ol_MenuButtoP_h_

/*************************************************************************
 *
 * Description:
 *		Private ".h" file for the MenuButton Widget and Gadget
 *
 ***************************file*header***********************************/

#include <Xol/ButtonP.h>
#include <Xol/MenuButton.h>

/***********************************************************************
 *
 * MenuButton Widget Private Data
 *
 ***********************************************************************/


		/* New fields for the MenuButton widget class record	*/

typedef struct _MenuButtonClass {
    int makes_compiler_happy;  /* not used */
} MenuButtonClassPart;

				/* Full class record declaration 	*/

typedef struct _MenuButtonClassRec {
    CoreClassPart		core_class;
    PrimitiveClassPart		primitive_class;
    ButtonClassPart		button_class;
    MenuButtonClassPart		menubutton_class;
} MenuButtonClassRec;

typedef struct _MenuButtonGadgetClassRec {
    RectObjClassPart		rect_class;
    EventObjClassPart		event_class;
    ButtonClassPart		button_class;
    MenuButtonClassPart		menubutton_class;
} MenuButtonGadgetClassRec;

extern MenuButtonClassRec	menuButtonClassRec;
extern MenuButtonGadgetClassRec	menuButtonGadgetClassRec;

/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/

			/* New fields for the MenuButton widget record */
typedef struct {
					/* Private data			*/

    Boolean             previewing_default; /* Is this widget previewing
					   its menu's default		*/
    Widget              preview_widget;	/* Widget previewing this one	*/
    Widget     		submenu;	/* Submenu for MenuButton	*/
    Position		root_x;		/* Temporary cache		*/
    Position		root_y;		/* Temporary cache		*/
} MenuButtonPart;


					/* Full widget declaration	*/
typedef struct _MenuButtonRec {
	CorePart	core;
	PrimitivePart	primitive;
	ButtonPart	button;
	MenuButtonPart	menubutton;
} MenuButtonRec;

					/* Full gadget declaration	*/
typedef struct _MenuButtonGadgetRec {
    ObjectPart		object;
    RectObjPart		rect;
    EventObjPart	event;
    ButtonPart		button;
    MenuButtonPart	menubutton;
} MenuButtonGadgetRec;

#endif /* _Ol_MenuButtoP_h_ */
