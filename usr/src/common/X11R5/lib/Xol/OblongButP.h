#ifndef	NOIDENT
#ident	"@(#)button:OblongButP.h	1.13"
#endif

/*
 ************************************************************
 *
 *  Description:
 *	This file contains the private definitions for the
 *	OPEN LOOK(tm) OblongButton widget and gadget.
 *
 ************************************************************
 */


#ifndef _Ol_OblongButP_h_
#define _Ol_OblongButP_h_

#include <Xol/ButtonP.h>
#include <Xol/OblongButt.h>

/*
 *  There are no new fields in the OblongButton class record 
 */
typedef struct _OblongButtonClass {
    char no_class_fields;		/* Makes compiler happy */
} OblongButtonClassPart;

typedef struct _OblongButtonClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	ButtonClassPart		button_class;
	OblongButtonClassPart	oblongButton_class;
} OblongButtonClassRec;

typedef struct _OblongButtonGadgetClassRec {
        RectObjClassPart        rect_class;
        EventObjClassPart       event_class;
        ButtonClassPart         button_class;
        OblongButtonClassPart   oblongButton_class;
} OblongButtonGadgetClassRec;


/*
 *  oblongButtonClassRec is defined in OblongButton.c
 */
extern OblongButtonClassRec oblongButtonClassRec;
extern OblongButtonGadgetClassRec oblongButtonGadgetClassRec;

/*
 *  declaration of the Button widget fields
 */
typedef struct {
	/*
	 *  Public resources
	 */
	int	unused;
} OblongButtonPart;


typedef struct _OblongButtonRec {
	CorePart		core;
	PrimitivePart		primitive;
	ButtonPart		button;
	OblongButtonPart	oblong;
} OblongButtonRec;

typedef struct _OblongButtonGadgetRec {
        ObjectPart              object;
        RectObjPart             rect;
        EventObjPart            event;
        ButtonPart              button;
        OblongButtonPart        oblong;
} OblongButtonGadgetRec;

#endif /* _Ol_OblongButP_h_ */
