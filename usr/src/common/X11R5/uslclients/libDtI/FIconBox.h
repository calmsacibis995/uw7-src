#ifndef _OL_FICONBOX_H
#define _OL_FICONBOX_H

#ifndef	NOIDENT
#ident	"@(#)libDtI:FIconBox.h	1.13"
#endif

/*
 ************************************************************************
 * Description:
 *	This is the flattened IconBox widget's public header file.
 ************************************************************************
 */

#include <Xol/FGraph.h>
#include <DnD/OlDnDVCX.h>

/*
 ************************************************************************
 * Define class and instance pointers:
 *	- extern pointer to class data/procedures
 *	- typedef pointer to widget's class structure
 *	- typedef pointer to widget's instance structure
 ************************************************************************
 */

extern WidgetClass				flatIconBoxWidgetClass;
typedef struct _FlatIconBoxClassRec *		FlatIconBoxWidgetClass;
typedef struct _FlatIconBoxRec *		FlatIconBoxWidget;


#define XtNdrawProc	"drawProc"
#define XtNmovableIcons	"movableIcons"
#define XtCMovableIcons	"MovableIcons"
#define XtNtriggerMsgProc	"triggerMsgProc"
#define XtCTriggerMsgProc	"TriggerMsgProc"
#define XtNselectCount	"selectCount"
#define XtCSelectCount	"SelectCount"
#define XtNlastSelectItem	"lastSelectItem"
#define XtCLastSelectItem	"LastSelectItem"
#define XtNobjectData	"objectData"
#define XtCObjectData	"ObjectData"
#define XtNpostSelectProc	"postSelectProc"
#define XtNadjustProc	"adjustProc"
#define XtNpostAdjustProc	"postAdjustProc"
#define XtNmenuProc	"menuProc"
#define XtNdropSiteID	"dropSiteID"
#define XtCDropSiteID	"DropSiteID"

typedef struct {
	OlFlatCallData	item_data;
	Position	x;		/* mouse pointer position	*/
	Position	y;		/* mouse pointer position	*/
	int		count;		/* number of select presses 	*/
	int		reason;		/* button name 			*/
	Boolean		ok;		/* ok to do the default action  */
} OlFIconBoxButtonCD;

typedef struct {
	Position	x;		/* item position x */
	Position	y;		/* item position y */
	Dimension	width;		/* item width */
	Dimension	height;		/* item height */
	int		item_index;	/* item index */
	char		*label;		/* item label to draw */
	XtPointer	op;		/* Object Pointer */
	Boolean		select;		/* selected or not */
	Boolean		busy;		/* busy or not */
	Boolean		focus;		/* item has focus or not */
	XFontStruct	*font;		/* label font */
	OlFontList	*font_list;	/* label font list */
	GC		label_gc;	/* label GC */
	Pixel		fg_color;	/* foreground color */
	Pixel		bg_color;	/* background color */
	Pixel		focus_color;	/* focus color */
} OlFIconDrawRec, *OlFIconDrawPtr;

#endif /* _OL_FICONBOX_H */
