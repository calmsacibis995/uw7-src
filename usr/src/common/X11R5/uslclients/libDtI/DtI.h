/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef __DtI_h_
#define __DtI_h_

#pragma ident	"@(#)libDtI:DtI.h	1.73.1.1"

#include <stdio.h>
#include <limits.h>
#include <sys/types.h>		/* for time_t in DmContainerRec */
#include <Xol/OpenLook.h>
#include <Xol/OlgP.h>
#include <DesktopP.h>	/* contains OLIT-independent Dm declarations */
#include "FIconBox.h"

/* MOVE Dm__arg int MDtI to accommodate libMundo */
#define ARGLIST_SIZE 16
extern Arg  Dm__arg[ARGLIST_SIZE];


/*
 * Information about the items in an iconbox is stored in an array of
 * DmItemRec structure.
 */
typedef struct dmItemRec {
    XtArgVal	/* String	*/	label;	
    XtArgVal	/* Position	*/	x;	
    XtArgVal	/* Position	*/	y;
    XtArgVal	/* Dimension	*/	icon_width;
    XtArgVal	/* Dimension	*/	icon_height;
    XtArgVal	/* Boolean	*/	managed;
    XtArgVal	/* Boolean	*/	select;
    XtArgVal	/* Boolean	*/	busy;
    XtArgVal	/* XtPointer	*/	client_data;
    XtArgVal	/* DmObjectPtr	*/	object_ptr;
} DmItemRec, *DmItemPtr;

/* macros to access members of DmItemRec */
#define ITEM_LABEL(item)	( (char *)	(item)->label )
#define ITEM_X(item)		( (Position)	(item)->x )
#define ITEM_Y(item)		( (Position)	(item)->y )
#define ITEM_WIDTH(item)	( (Dimension)	(item)->icon_width )
#define ITEM_HEIGHT(item)	( (Dimension)	(item)->icon_height )
#define ITEM_MANAGED(item)	( (Boolean)	(item)->managed )
#define ITEM_SELECT(item)	( (Boolean)	(item)->select )
#define ITEM_BUSY(item)		( (Boolean)	(item)->busy )
#define ITEM_CLIENT(item)	(		(item)->client_data )
#define ITEM_OBJ(item)		( (DmObjectPtr)	(item)->object_ptr )

/* DO NOT REMOVE - used by dtadmin clients */
#define OBJECT_PTR(item) ITEM_OBJ(item)

/* Macros to access fields given item_data from FIconBox call data */
#define ITEM_CD(ID)		( (DmItemPtr)((ID).items) + (ID).item_index )
#define OBJECT_CD(ID)		( (DmObjectPtr)ITEM_OBJ(ITEM_CD(ID)) )

/* Macros to access DmObjectPtr object_ptr fields given an item pointer */
#define FILEINFO_PTR(item)	( (DmFileInfoPtr)ITEM_OBJ(item)->objectdata )
#define FCLASS_PTR(item)	( (DmFclassPtr)ITEM_OBJ(item)->fcp )
#define OBJ_IS_DIR(op)		( (op)->ftype == DM_FTYPE_DIR )
#define ITEM_IS_DIR(item)	( ITEM_OBJ(item)->ftype == DM_FTYPE_DIR )
#define ITEM_OBJ_NAME(item)	( ITEM_OBJ(item)->name )

/* Macros to access GlyphPtr fields given an item pointer */
#define GLYPH_PTR(item)		( (DmGlyphPtr)FCLASS_PTR(item)->glyph )
#define SMALLICON_PTR(item)	( (DmGlyphPtr)FCLASS_PTR(item)->small_icon )


/* options for DmCreatePromptBox() */
#define DM_B_SHELL_CREATED	(1 << 0)

#define VertJustifyInGrid(y, item_height, grid_height) \
    ( ((grid_height - item_height) > 0) ? y + (grid_height - item_height) : y )

/* Convenience macro for getting text width (should be in OpenLook.h) */
#define DM_TextWidth(font, fontlist, str, cnt) \
    ( ((fontlist) == NULL) ? \
     XTextWidth(font, (char *)(str), cnt) : \
     OlTextWidth(fontlist, (unsigned char *)(str), cnt) ) \

/* function prototypes */
extern int	Dm__GetFreeItems(DmItemPtr * items, Cardinal * num_items,
				 Cardinal need_cnt, DmItemPtr * ret_item);

extern void	Dm__vaprtwarning(const char *format,...);
extern void	Dm__vaprterror(const char *format,...);
extern void	DmRefreshIcon(Widget flat, DmItemPtr ip);
extern int	Dm__ObjectToIndex(Widget flat, DmObjectPtr op);
extern int	Dm__ItemToIndex(Widget flat, DmItemPtr ip);
extern void	DmSizeIcon(DmItemPtr, OlFontList *, XFontStruct *);
extern int	Dm__ItemNameToIndex(DmItemPtr ip, int nitems, char *name);
extern void DmDelFileClass(Screen *scrn, DmFclassPtr *list, DmFclassPtr fcp);
extern void DmDrawIcon( Widget w, XtPointer client_data, XtPointer call_data);
extern void DmDrawIconGlyph(Widget w,
			    GC gc,
			    DmGlyphPtr gp,
			    OlgAttrs *attrs,
			    int x,
			    int y,
			    unsigned attr_flags);

extern Widget DmResetIconContainer(Widget iconbox,
				    Widget parent,
				    DtAttrs attrs,
				    ArgList args,
				    Cardinal num_args,
				    DmObjectPtr objp,
				    Cardinal num_objs,
				    DmItemPtr *itemp,
				    Cardinal num_items,
				    Widget *swin,
				    OlFontList *fontlist,
				    XFontStruct *font,
				    int charheight);

extern Widget DmCreateIconContainer(Widget parent,
				    DtAttrs attrs,
				    ArgList args,
				    Cardinal num_args,
				    DmObjectPtr objp,
				    Cardinal num_objs,
				    DmItemPtr *itemp,
				    Cardinal num_items,
				    Widget *swin,
				    OlFontList *fontlist,
				    XFontStruct *font,
				    int charheight);



#define DmAddObjectToIconContainer(w, items, num_items, cp, obj, x, y, attrs, fontlist, font, charheight) \
    Dm__AddObjToIcontainer(w, items, num_items, cp, op, x, y, \
			      attrs, fontlist, font, 0, 0, 0)

extern Cardinal Dm__AddObjToIcontainer(
		Widget,			/* FIconBox widget */
		DmItemPtr *,		/* ptr to items array */
		Cardinal *,		/* ptr to num_items */
		DmContainerPtr,		/* ptr to container */
		DmObjectPtr,		/* ptr to obj to be added */
		Position, Position,	/* x,y (or DM_UNSPECIFIED_POS) */
		DtAttrs,		/* options */
		OlFontList *,		/* font list (for sizing) */
		XFontStruct *,		/* font (for sizing) */
		Dimension,		/* wrap width (for positioning) */
		Dimension,		/* grid width (for positioning) */
		Dimension);		/* grid height (for positioning) */
			       
extern void DmCreateIconCursor(Widget, XtPointer, XtPointer);
extern void DmIconAdjustProc(Widget, XtPointer, XtPointer);
extern void DmIconSelect1Proc(Widget, XtPointer, XtPointer);
extern void DmDestroyIconContainer(Widget shell, Widget w,
				   DmItemPtr ilist, int nitems);
extern void DmUpdateDnDRect();
extern Boolean DmIntersectItems(DmItemPtr item, Cardinal num_items,
				int x, int y, int w, int h);
extern void DmGetAvailIconPos(DmItemPtr,	/* items */
			      Cardinal,		/* num_items */
			      Dimension,	/* item width */
			      Dimension,	/* item height */
			      Dimension,	/* wrap width */
			      Dimension,	/* grid width */
			      Dimension,	/* grid height */
			      Position *,	/* x (returned) */
			      Position *);	/* y (returned) */

extern String DmXYToIconLabel(Widget w, Position x, Position y);
extern int DmIconLabelToXY(Widget w, String icon_name, Position *x, Position *y);

#endif /* __DtI_h_ */
