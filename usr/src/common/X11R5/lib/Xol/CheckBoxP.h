#ifndef	NOIDENT
#ident	"@(#)checkbox:CheckBoxP.h	1.23"
#endif

/*
 ************************************************************
 *
 * File: CheckBoxP.h - Private definitions for CheckBox widget
 * 
 ************************************************************
 */

#ifndef _OlCheckBoxP_h
#define _OlCheckBoxP_h

#include <Xol/ManagerP.h>
#include <Xol/ButtonP.h>
#include <Xol/CheckBox.h>
#include <Xol/Olg.h>

/* New fields for the CheckBox widget class record */

typedef struct _CheckBoxClass {
    char no_class_fields;		/* Makes compiler happy */
} CheckBoxClassPart;

   /* Full class record declaration */
typedef struct _CheckBoxClassRec {
    CoreClassPart	core_class;
    CompositeClassPart	composite_class;
    ConstraintClassPart	constraint_class;
    ManagerClassPart	manager_class;
    CheckBoxClassPart	checkBox_class;
} CheckBoxClassRec;

extern CheckBoxClassRec checkBoxClassRec;

		    /* New fields for the CheckBox widget record: */
typedef struct {

		   				/* fields for resources */

						/* public */
    XtCallbackList	select;
    XtCallbackList	unselect;
    Boolean		set;
    Boolean		dim;
    Boolean		sensitive;
    Boolean		setvalue;
    char		*label;
    Pixel		foreground;
    Pixel		fontcolor;
    OlDefine		labeljustify;
    OlDefine		position;
    OlDefine		labeltype;
    Boolean		labeltile;
    XImage	       *labelimage;
    Boolean		recompute_size;
    XFontStruct	       *font;
    OlMnemonic		mnemonic;
    String		accelerator;
    String		accelerator_text;


					/* private fields */ 
    Widget		label_child;
    Dimension		normal_height;
    Dimension		normal_width;
    Position		x1,x2;		/* position of checkbox box */
    Position		y1,y2;		
    unsigned char	dyn_flags;
    OlgAttrs		*pAttrs;

} CheckBoxPart;


/*    XtEventsPtr eventTable;*/


   /* Full widget declaration */
typedef struct _CheckBoxRec {
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
    ManagerPart		manager;
    CheckBoxPart	checkBox;
} CheckBoxRec;

/* dynamics resource bit masks */
#define OL_B_CHECKBOX_FG		(1 << 0)
#define OL_B_CHECKBOX_FONTCOLOR		(1 << 1)
#endif /* _OlCheckBoxP_h */
