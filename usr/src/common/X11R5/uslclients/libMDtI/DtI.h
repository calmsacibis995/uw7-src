#ifndef __DtI_h_
#define __DtI_h_

#ifndef	NOIDENT
#pragma ident	"@(#)libMDtI:DtI.h	1.16.1.1"
#endif


#include <Xm/Xm.h>
#include <stdio.h>
#include <limits.h>
#include <sys/types.h>		/* for time_t in DmContainerRec */
#include <DesktopP.h>		/* contains OLIT-independent Dm declarations */
#include "FIconBoxP.h"
#include "array.h"


/* MOVE Dm__arg int MDtI to accommodate libMundo */
#define ARGLIST_SIZE 16
extern Arg  Dm__arg[ARGLIST_SIZE];

/*
 * Information about the items in an iconbox is stored in an array of
 * DmItemRec structure.
 */
typedef struct dmItemRec {
    XtArgVal	/* _XmString	*/	label;	
    XtArgVal	/* WidePosition	*/	x;	
    XtArgVal	/* WidePosition	*/	y;
    XtArgVal	/* Dimension	*/	icon_width;
    XtArgVal	/* Dimension	*/	icon_height;
    XtArgVal	/* Boolean	*/	managed;
    XtArgVal	/* Boolean	*/	select;
    XtArgVal	/* Boolean	*/	sensitive;
    XtArgVal	/* XtPointer	*/	client_data;
    XtArgVal	/* DmObjectPtr	*/	object_ptr;
} DmItemRec, *DmItemPtr;

/* macros to access members of DmItemRec */
#define ITEM_LABEL(item)	( (_XmString)	(item)->label )
#define ITEM_X(item)		( (WidePosition)	(item)->x )
#define ITEM_Y(item)		( (WidePosition)	(item)->y )
#define ITEM_WIDTH(item)	( (Dimension)	(item)->icon_width )
#define ITEM_HEIGHT(item)	( (Dimension)	(item)->icon_height )
#define ITEM_MANAGED(item)	( (Boolean)	(item)->managed )
#define ITEM_SELECT(item)	( (Boolean)	(item)->select )
#define ITEM_SENSITIVE(item)	( (Boolean)	(item)->sensitive )
#define ITEM_CLIENT(item)	(		(item)->client_data )
#define ITEM_OBJ(item)		( (DmObjectPtr)	(item)->object_ptr )


/* Label Macros: must be used to create, copy, compare, measure and free labels.*/
/* FLH MORE: do we need to specify a tag here? */
/* Macro to make a private copy of an item label, stored
   in an internal format */
#define MAKE_LABEL(label, string) \
    { XmString str = XmStringCreateLtoR((String)(string), \
				XmFONTLIST_DEFAULT_TAG); \
      (label) = (XtArgVal)_XmStringCreate(str); \
      XmStringFree(str);	\
    }
#define MAKE_WRAPPED_LABEL(label, font, string) \
		label = (XtArgVal)DmWrappedLabel(font, string)
#define FREE_LABEL(label) _XmStringFree((_XmString)label)
#define COPY_LABEL(label) _XmStringCopy((_XmString)label)
#define SAME_LABEL(label1,label2) _XmStringByteCompare((_XmString)(label1),(_XmString)(label2))


extern char *DmGetTextFromXmString(XmString str);

#define GET_TEXT(s) DmGetTextFromXmString(s)

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
    ( ((int)(grid_height - item_height) > 0) ? \
     y + (WidePosition)(grid_height - item_height) : y )

/* Use this for justifying items with wrapped label */
#define VertWrapJustifyInGrid(y, item_height, grid_height, font_height, label_height) \
	(int)(y + (WidePosition)grid_height - font_height - \
		(item_height - label_height))

#ifdef FLH_REMOVE /*FLH MORE: add these: they should be moved from DesktopP.h*/
#define ICON_PADDING	1
#define ICON_MARGIN	4	/* Margin (in points) around icons in pane */
#define INTER_ICON_PAD	3	/* Space between icons (in points) */
#define ICON_LABEL_GAP	2	/* Space between icon and label (in points) */
#endif


#define XA_WM_DELETE_WINDOW(d)  XInternAtom((d), "WM_DELETE_WINDOW", False)
#define XA_WM_SAVE_YOURSELF(d)  XInternAtom((d), "WM_SAVE_YOURSELF", False)


/* function prototypes */
extern int	Dm__GetFreeItems(DmItemPtr * items, Cardinal * num_items,
				 Cardinal need_cnt, DmItemPtr * ret_item);

extern void	DmRefreshIcon(Widget flat, DmItemPtr ip);
extern int	Dm__ObjectToIndex(Widget flat, DmObjectPtr op);
extern int	Dm__ItemToIndex(Widget flat, DmItemPtr ip);
extern void	DmSizeIcon(Widget, DmItemPtr);
extern int	Dm__ItemNameToIndex(DmItemPtr ip, int nitems, _XmString name);
extern void DmDelFileClass(Screen *scrn, DmFclassPtr *list, DmFclassPtr fcp);
extern void DmDrawIcon( Widget w, XtPointer client_data, XtPointer call_data);
extern void DmDrawIconGlyph(Widget w, ExmFlatItem item, ExmFlatDrawInfo *di, 
			    DmGlyphPtr glyph, WidePosition x, WidePosition y);
extern void	DmInitObjType(Widget w, DmObjectPtr op);
/* FLH MORE: should width be a Dimension or unsigned int? */
extern void DmDrawIconLabel(Widget w, ExmFlatItem item, ExmFlatDrawInfo *di, 
			    WidePosition x, WidePosition y, Dimension width);


extern void DmResetIconContainer(Widget iconbox,
				 DtAttrs attrs,
				 DmObjectPtr objp,
				 Cardinal num_objs,
				 DmItemPtr *itemp,
				 Cardinal *num_items,
				 Cardinal extra_slots);

extern Widget DmCreateIconContainer(Widget parent,
				    DtAttrs attrs,
				    ArgList args,
				    Cardinal num_args,
				    DmObjectPtr objp,
				    Cardinal num_objs,
				    DmItemPtr *itemp,
				    Cardinal num_items,
				    Widget *swin);

#define DmAddObjectToIconContainer(w, items, num_items, cp, obj, x, y, attrs) \
    Dm__AddObjToIcontainer(w, items, num_items, cp, op, x, y, \
			      attrs, 0, 0, 0)

extern Cardinal Dm__AddObjToIcontainer(
		Widget,			/* FIconBox widget */
		DmItemPtr *,		/* ptr to items array */
		Cardinal *,		/* ptr to num_items */
		DmContainerPtr,		/* ptr to container */
		DmObjectPtr,		/* ptr to obj to be added */
		WidePosition, WidePosition, /* x,y (or DM_UNSPECIFIED_POS) */
		DtAttrs,		/* options */
		Dimension,		/* wrap width (for positioning) */
		Dimension,		/* grid width (for positioning) */
		Dimension);		/* grid height (for positioning) */
			       
extern void DmCreateIconCursor(Widget, XtPointer, XtPointer);
extern void DmDestroyIconContainer(Widget shell, Widget w,
				   DmItemPtr ilist, int nitems);
extern void DmUpdateDnDRect();
extern Boolean DmIntersectItems(DmItemPtr item, Cardinal num_items,
				WidePosition x, WidePosition y,
				Dimension w, Dimension h);
extern void DmGetAvailIconPos(Widget,		/* icon box */
			      DmItemPtr,	/* items */
			      Cardinal,		/* num_items */
			      Dimension,	/* item width */
			      Dimension,	/* item height */
			      Dimension,	/* wrap width */
			      Dimension,	/* grid width */
			      Dimension,	/* grid height */
			      WidePosition *,	/* x (returned) */
			      WidePosition *);	/* y (returned) */


extern Dimension DM_TextWidth(Widget flat, DmItemPtr item);
extern Dimension DM_TextHeight(Widget flat, DmItemPtr item);
extern void DM_TextExtent(Widget flat, DmItemPtr item, Dimension *width, 
			  Dimension *height);
extern Dimension DM_FontHeight(XmFontList font);
extern Dimension DM_FontWidth(XmFontList font);
extern void DM_FontExtent(XmFontList font,Dimension *width, Dimension *height);
extern int DmCompareXmStrings(XmString str1, XmString str2);
extern int DmCompareItemLabels(DmItemPtr item1, DmItemPtr item2);
extern Boolean DmModalCascadeActive(Widget w);
extern _XmString DmWrappedLabel(XmFontList font, char *string);
extern void DmSetWrappedLabelInfo(char *separators, Dimension width);
extern _XmString DmXYToIconLabel(Widget w, WidePosition x, WidePosition y);
extern int DmIconLabelToXY(Widget w, _XmString icon_name, WidePosition *x, WidePosition *y);

#endif /* __DtI_h_ */
