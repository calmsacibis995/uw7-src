#ifndef	NOIDENT
#pragma ident	"@(#)libMDtI:Flat.c	1.17"
#endif

/******************************file*header********************************
 *
 * Description:
 *	This file contains the source code for the flat widget class.
 *	The flat widget class is not a class that's intended for
 *	instantiation; rather, it serves as a managing class for its
 *	instantiated subclasses.
 */

						/* #includes go here	*/
#include <stdio.h>
#include <memory.h>

#include "FlatP.h"
#include "ScrollUtil.h"
#ifdef USE_FONT_OBJECT
#include <dlfcn.h>	/* for dlopen */
#include <Xm/FontObj.h>
#endif /* USE_FONT_OBJECT */

/*************************************************************************
 
   Forward Procedure definitions listed by category:
 		1. Private Procedures
 		2. Class   Procedures
 		3. Action  Procedures
*/

/**************************forward*declarations***************************/

					/* private procedures		*/

static void	ItemsTouched(Widget);
static void	SetDefaultGCs(Widget);
static Boolean	SetFocusToAnyItem(Widget);
static void	UpdateLastSelectedItem(Widget, Cardinal);

					/* class procedures		*/

static void	AnalyzeItems(Widget, ArgList, Cardinal *);
static void	ClassInitialize(void);
static void	ClassPartInitialize(WidgetClass);
static void	DefaultItemInitialize(Widget, ExmFlatItem,
				      ExmFlatItem, ArgList, Cardinal *);
static Boolean	DefaultItemSetValues(Widget, ExmFlatItem, ExmFlatItem,
				     ExmFlatItem, ArgList, Cardinal *);
static void	Destroy(Widget);
static void	Initialize(Widget, Widget, ArgList, Cardinal *);
static void	HighlightBorder(Widget);
static void	UnhighlightBorder(Widget);
static Boolean	ItemAcceptFocus(Widget, ExmFlatItem);
static void	ItemHighlight(Widget, ExmFlatItem, int);
static void	ItemInitialize(Widget, ExmFlatItem, ExmFlatItem,
			       ArgList, Cardinal *);
static Boolean	ItemSetValues(Widget, ExmFlatItem, ExmFlatItem,
			      ExmFlatItem, ArgList, Cardinal *);
static XtGeometryResult
		QueryGeom(Widget, XtWidgetGeometry *, XtWidgetGeometry *);
static Boolean	SetValues(Widget, Widget, Widget, ArgList, Cardinal*);

					/* action procedures		*/

static void FlatTraverseAction(Widget, XEvent *, String *, Cardinal *);

/*****************************file*variables******************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 */

#ifdef __STDC__
#define SIZE_T	size_t
#else
#define SIZE_T	int
#endif

/* Declare a structre that describes characteristics of inheritable class
   fields.  Each function denoted in this structure should have a
   corresponding XtInherit... token.
*/

typedef struct {
    Cardinal	offset;		/* field's offset			*/
    String	required;	/* If this pointer is non-NULL, then
				 * the class field must be non-NULL.  This
				 * check is made for some important class
				 * procedures since macros are provided
				 * (and the macros don't check to see if
				 * the procedures exist.		*/
} ClassFields;

#undef OFFSET
#define OFFSET(f)	XtOffsetOf(ExmFlatClassRec, flat_class.f)

static _XmConst ClassFields
class_fields[] = {
    { OFFSET(draw_item),		NULL },
    { OFFSET(change_managed),		"ExmFlatChangeManagedProc" },
    { OFFSET(geometry_handler),		NULL },
    { OFFSET(get_item_geometry),	"ExmFlatGetItemGeometryProc" },
    { OFFSET(get_index),		"ExmFlatGetIndexFunc" },
    { OFFSET(item_accept_focus),	NULL },
    { OFFSET(item_dimensions),		"ExmFlatItemDimensionsProc" },
    { OFFSET(item_highlight),		"ExmFlatItemHighlightProc" },
    { OFFSET(item_set_values_almost),	NULL },
    { OFFSET(refresh_item),		NULL },
    { OFFSET(traverse_items),		NULL }
};

#define MAINTAIN_SAME_VALUES(i)					\
	FITEM(i).x			= (WidePosition)ExmIGNORE;	\
	FITEM(i).y			= (WidePosition)ExmIGNORE;	\
	FITEM(i).width			= (Dimension)ExmIGNORE;\
	FITEM(i).height			= (Dimension)ExmIGNORE;\
	FITEM(i).sensitive		= (Boolean)True;	\
	FITEM(i).managed		= (Boolean)True;	\
	FITEM(i).mapped_when_managed	= (Boolean)True

					/* Define some handy macros	*/

#define PCLASS(wc, f)	( ((XmPrimitiveWidgetClass)wc)->primitive_class.f )
#define CALL_ITEM_HIGHLIGHT(w, i, t)		\
	if (FCLASS(w).item_highlight)	\
		(*FCLASS(w).item_highlight)(w, i, t)

#define	LOCAL_SIZE	50

#ifdef DONT_RM_HACK
#define INDEX_IS_CACHED(d)	((d) != (XtPointer)NULL)
#endif
#define INDEX_TO_DATA(i)	((XtPointer)((i)+1))
#define DATA_TO_INDEX(i)	((Cardinal)((Cardinal)(i)-1))

static _XmConst Dimension	def_dimension = (Dimension)ExmIGNORE;
static _XmConst WidePosition	def_position = (WidePosition)ExmIGNORE;

#define IS_TRUE(B)		((B) ? 1 : 0)
#define ACTION_IDX(B2, B1, B0)  ((IS_TRUE(B2) << 2) + \
				 (IS_TRUE(B1) << 1) + \
				  IS_TRUE(B0))
#define CHK_CNT			(1 << 0)
#define CHK_OK			(1 << 1)
#define RESET			(1 << 2)
#define INC			(1 << 3)
#define DEC			(1 << 4)
#define ONE			(1 << 5)
#define RAISE			(1 << 6)

static _XmConst char set_actions[] = {
    /* new
      value   none_set	exclusives */
    /* N	N	N */	CHK_OK | CHK_CNT| DEC,
    /* N	N	Y */	0,
    /* N	Y	N */	CHK_OK | DEC,
    /* N	Y	Y */	CHK_OK | RESET	| DEC,
    /* Y	N	N */	CHK_OK | INC | RAISE,
    /* Y	N	Y */	CHK_OK | RESET	| ONE | RAISE,
    /* Y	Y	N */	CHK_OK | INC | RAISE,
    /* Y	Y	Y */	CHK_OK | RESET	| ONE | RAISE,
};

/***********************widget*translations*actions***********************
 *
 * Define Translations and Actions
 */

/* override some primitive traversal translations so that flat can handle
 * traversing between items.
 * 
 * NOTE: the direction of traversal is passed as a parameter to the action
 * routine. The directions are also defined for subclasses in FlatP.h
 */
static _XmConst char traversal_translations[] = "\
~s ~m ~a <Key>Return:PrimitiveParentActivate()\n\
<Key>osfActivate:PrimitiveParentActivate()\n\
<Key>osfCancel:PrimitiveParentCancel()\n\
<Key>osfHelp:PrimitiveHelp()\n\
c <Key>osfUp:flat-traverse-items(U)\n\
<Key>osfUp:flat-traverse-items(u)\n\
c <Key>osfDown:flat-traverse-items(D)\n\
<Key>osfDown:flat-traverse-items(d)\n\
c <Key>osfLeft:flat-traverse-items(L)\n\
<Key>osfLeft:flat-traverse-items(l)\n\
c <Key>osfRight:flat-traverse-items(R)\n\
<Key>osfRight:flat-traverse-items(r)\n\
s ~m ~a <Key>Tab:PrimitivePrevTabGroup()\n\
~m ~a <Key>Tab:PrimitiveNextTabGroup()\n\
<FocusIn>:PrimitiveFocusIn()\n\
<FocusOut>:PrimitiveFocusOut()\n\
<Unmap>:PrimitiveUnmap()";

static XtActionsRec flat_actions[] = {
  { "flat-traverse-items", FlatTraverseAction },
};

/*****************************widget*resources*****************************
 *
 * Define Resource list associated with the Widget Instance
 *
 */
			/* Define the resources for the container.	*/
#undef OFFSET
#define OFFSET(f)	XtOffsetOf(ExmFlatRec,flat.f)

static XtResource
resources[] = {
	/* Define flat resources. Note, the order of XmNitems, XmNnumItems
	 * XmNitemFields and XmNnumItemFields is significant.
	 */
	{ XmNnumItems, XmCNumItems, XmRCardinal, sizeof(Cardinal),
	  OFFSET(num_items), XmRImmediate, (XtPointer)ExmIGNORE },

	{ XmNnumItemFields, XmCNumItemFields, XmRCardinal, sizeof(Cardinal),
	  OFFSET(num_item_fields), XmRImmediate, (XtPointer)ExmIGNORE },

	{ XmNitemFields, XmCItemFields, XmRFlatItemFields, sizeof(String *),
	  OFFSET(item_fields), XmRFlatItemFields, (XtPointer)NULL },

	{ XmNitems, XmCItems, XmRFlatItems, sizeof(XtPointer),
	  OFFSET(items), XmRFlatItems, (XtPointer)NULL },

	    /* Specify navigationTyoe in XmPrimitive */
	{ XmNnavigationType, XmCNavigationType, XmRNavigationType,
	  sizeof (unsigned char),
	  XtOffsetOf( struct _XmPrimitiveRec, primitive.navigation_type),
	  XmRImmediate, (XtPointer)XmTAB_GROUP },

	    /* DnD related (dragging part) */
	{ XmNtargets, XmCTargets, XmRAtomList, sizeof(Atom *),
	  OFFSET(targets), XmRImmediate, (XtPointer)NULL },

	{ XmNnumTargets, XmCNumTargets, XmRCardinal, sizeof(Cardinal),
	  OFFSET(num_targets), XmRImmediate, (XtPointer)0 },

	{ XmNconvertProc, XmCConvertProc, XmRFunction,
	  sizeof(XmConvertSelectionRec), OFFSET(convert_proc),
	  XmRImmediate, (XtPointer)NULL },

	{ XmNdragCursorProc, XmCCallbackProc, XmRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(cursor_proc),
	  XmRCallbackProc, (XtPointer)NULL },

	{ XmNdragDropFinishProc, XmCCallbackProc, XmRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(dnd_done_proc),
	  XmRCallbackProc, (XtPointer)NULL },

	{ XmNdropProc, XmCCallbackProc, XmRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(drop_proc),
	  XmRCallbackProc, (XtPointer)NULL },

	{ XmNdragOperations, XmCDragOperations, XmRUnsignedChar,
	  sizeof(unsigned char), OFFSET(drag_ops),
	  XmRImmediate, (XtPointer)(XmDROP_COPY | XmDROP_MOVE | XmDROP_LINK) },

	    /* CallProcs for select, unselect, and dbl-click */
	{ XmNselectProc, XmCCallbackProc, XmRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(select_proc),
	  XmRCallbackProc, (XtPointer) NULL},

	{ XmNdblSelectProc, XmCCallbackProc, XmRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(dbl_select_proc),
	  XmRCallbackProc, (XtPointer) NULL },

	{ XmNunselectProc, XmCCallbackProc, XmRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(unselect_proc),
	  XmRCallbackProc, (XtPointer)NULL },

	    /* Remaining Flat resources alphabetically */
	{ XmNexclusives, XmCExclusives, XmRBoolean, sizeof(Boolean),
	  OFFSET(exclusives), XmRImmediate, (XtPointer)False },

	{ XmNitemsTouched, XmCItemsTouched, XmRBoolean, sizeof(Boolean),
	  OFFSET(items_touched), XmRImmediate, (XtPointer)False },

	{ XmNfontList, XmCFontList, XmRFontList, sizeof(XmFontList),
	  OFFSET(font), XmRImmediate, (XtPointer) NULL },

	{ XmNlastSelectItem, XmCLastSelectItem, XmRCardinal, sizeof(Cardinal),
	  OFFSET(last_select), XmRImmediate, (XtPointer)ExmNO_ITEM },

	{ XmNnoneSet, XmCNoneSet, XmRBoolean, sizeof(Boolean),
	  OFFSET(none_set), XmRImmediate, (XtPointer)True },

	{ XmNselectCount, XmCSelectCount, XmRCardinal, sizeof(Cardinal),
	  OFFSET(select_count), XmRImmediate, (XtPointer)0 },

	{ XmNclientData, XmCClientData, XmRPointer, sizeof(XtPointer),
	  OFFSET(client_data), XmRPointer, (XtPointer)NULL },

		/* Create Only resource */
	{ XmNwantGraphicsExpose, XmCWantGraphicsExpose, XmRBoolean,
	  sizeof(Boolean),
	  OFFSET(want_graphics_expose), XmRImmediate, (XtPointer)True },
};

				/* Define Resources for sub-objects	*/
#undef OFFSET
#define OFFSET(field)	XtOffsetOf(ExmFlatItemRec, flat.field)

static XtResource
item_resources[] = {
	{ XmNx, XmCX, XmRWidePosition, sizeof(WidePosition),
	  OFFSET(x), XmRWidePosition, (XtPointer)&def_position },

	{ XmNy, XmCY, XmRWidePosition, sizeof(WidePosition),
	  OFFSET(y), XmRWidePosition, (XtPointer)&def_position },

	{ XmNwidth, XmCWidth, XmRDimension, sizeof(Dimension),
	  OFFSET(width), XmRDimension, (XtPointer)&def_dimension },

	{ XmNheight, XmCHeight, XmRDimension, sizeof(Dimension),
	  OFFSET(height), XmRDimension, (XtPointer)&def_dimension },

	    /* NOTE: This should be XmR_XmString !! */
	{ XmNlabelString, XmCXmString, XmRXmString, sizeof(_XmString),
	  OFFSET(label), XmRImmediate, (XtPointer)NULL },

	{ XmNmanaged, XmCManaged, XmRBoolean, sizeof(Boolean),
	  OFFSET(managed), XmRImmediate, (XtPointer)True },

	{ XmNmappedWhenManaged, XmCMappedWhenManaged, XmRBoolean,
	  sizeof(Boolean), OFFSET(mapped_when_managed),
	  XmRImmediate, (XtPointer)True },

	{ XmNsensitive, XmCSensitive, XmRBoolean, sizeof(Boolean),
	  OFFSET(sensitive), XmRImmediate, (XtPointer)True },

	{ XmNset, XmCSet, XmRBoolean, sizeof(Boolean),
	  OFFSET(selected), XmRImmediate, (XtPointer)False },

	{ XmNtraversalOn, XmCTraversalOn, XmRBoolean, sizeof(Boolean),
	  OFFSET(traversal_on), XmRImmediate, (XtPointer)True },

	{ XmNuserData, XmCUserData, XmRPointer, sizeof(XtPointer),
	  OFFSET(user_data), XmRImmediate, (XtPointer)NULL },
};

/***************************widget*class*record***************************
 
  Define Class Record structure to be initialized at Compile time
*/

ExmFlatClassRec
exmFlatClassRec = {
    {
	(WidgetClass)&xmPrimitiveClassRec,	/* superclass		*/
	"ExmFlat",				/* class_name		*/
	sizeof(ExmFlatRec),			/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	ClassPartInitialize,			/* class_part_initialize*/
	False,					/* class_inited		*/
	ExmFlatStateInitialize,			/* initialize		*/
	NULL,					/* initialize_hook	*/
	XtInheritRealize,			/* realize		*/
	flat_actions,				/* actions		*/
	XtNumber(flat_actions),			/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	TRUE,					/* compress_motion	*/
	XtExposeCompressMultiple,		/* compress_exposure	*/
	TRUE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	Destroy,				/* destroy		*/
	NULL,					/* resize		*/
	XtInheritExpose,			/* expose		*/
	ExmFlatStateSetValues,			/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	NULL,					/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_offsets	*/
	NULL,					/* tm_table		*/
	QueryGeom,				/* query_geometry	*/
	NULL,					/* display_accelerator	*/
	NULL,					/* extension		*/
    }, /* End of Core Class Part Initialization */
    {
	HighlightBorder,			/* border_highlight   */
	UnhighlightBorder,			/* border_unhighlight */
	traversal_translations,		 	/* translations       */
	NULL,					/* arm_and_activate   */
	NULL,					/* syn resources      */
	0,					/* num syn_resources  */
	NULL,					/* extension          */
    },	/* End of XmPrimitive Class Part Initialization */
    {
	(XtPointer)NULL,			/* extension		*/
	XtOffsetOf(ExmFlatRec, default_item),	/* default_offset	*/
	sizeof(ExmFlatItemRec),			/* rec_size		*/
	item_resources,				/* item_resources	*/
	XtNumber(item_resources),		/* num_item_resources	*/
	(Cardinal)0,				/* mask			*/
	NULL,					/* quarked_items	*/

		/* Container procedures	*/

	Initialize,				/* initialize		*/
	SetValues,				/* set_values		*/
	ExmInheritFlatGeometryHandler,		/* geometry_handler	*/
	ExmInheritFlatChangeManaged,		/* change_managed	*/
	ExmInheritFlatGetItemGeometry,		/* get_item_geometry	*/
	ExmInheritFlatGetIndex,			/* get_index		*/
	ExmInheritFlatTraverseItems,		/* traverse_items	*/
	NULL,					/* refresh_item		*/

		/* Item procedures	*/

        DefaultItemInitialize,			/* default_initialize	*/
        DefaultItemSetValues,			/* default_set_values	*/
	AnalyzeItems,				/* analyze_items	*/
	ExmInheritFlatDrawItem,			/* draw_item		*/
	ItemAcceptFocus,			/* item_accept_focus	*/
	ExmInheritFlatItemDimensions,		/* item_dimensions	*/
	NULL,					/* item_get_values	*/
	ItemHighlight,				/* item_highlight	*/
	ItemInitialize,				/* item_initialize	*/
	ItemSetValues,				/* item_set_values	*/
	NULL,					/* item_set_values_almost*/
    } /* End of ExmFlat Class Part Initialization */
};

/*************************public*class*definition*************************
 *
 * Public Widget Class Definition of the Widget Class Record
 */

WidgetClass exmFlatWidgetClass = (WidgetClass)&exmFlatClassRec;

/***************************private*procedures****************************
 *
 * Private Procedures
 */

/****************************procedure*header*****************************
 * SetDefaultGCs - this routine sets the default GCs used to draw normal and
 * selected items
 */ 
static void
SetDefaultGCs(Widget w)
{
    unsigned long	mask;
    XGCValues		values;
    XFontStruct *	fs = NULL;
    XrmValue		res_value;

    /* Set the GC for scrollbar related... */
    mask = GCGraphicsExposures;
    values.graphics_exposures = FPART(w).want_graphics_expose;
    if (FPART(w).scroll_gc)
	XtReleaseGC(w, FPART(w).scroll_gc);
    FPART(w).scroll_gc = XtGetGC(w, mask, &values);

    mask = GCForeground | GCBackground | GCFillStyle | GCGraphicsExposures;
    values.foreground		= PPART(w).foreground;
    values.fill_style		= FillSolid;
    values.graphics_exposures	= False;

    /* From (Xm)Label.c */
    /* This is sloppy - get the default font and use it for the GC. The
     * StringDraw routines will change it if needed.
     */
    _XmFontListGetDefaultFont(FPART(w).font, &fs);
    if (fs) {

	mask |= GCFont;
	values.font = fs->fid;
    }

    /* Set the GC for drawing "normal" (unselected) items */
    values.background = CPART(w).background_pixel;
    if (FPART(w).normal_gc)
	XtReleaseGC(w, FPART(w).normal_gc);
    FPART(w).normal_gc = XtGetGC(w, mask, &values);

    /* Set the GC for drawing selected items. */
    /* Use _XmSelectColorDefault to get selected color. */
    _XmSelectColorDefault(w, 0, &res_value);
    values.background = *(Pixel *)(res_value.addr);
    if (FPART(w).select_gc)
	XtReleaseGC(w, FPART(w).select_gc);
    FPART(w).select_gc = XtGetGC(w, mask, &values);

    /* As an added convenience: get the pixmap used for insensitive items.
     * NOTE: from Label.c:
     * "50_foreground" is in the installed set and should always be
     * found so omit check for XmUNSPECIFIED_PIXMAP.
     */
    if (FPART(w).insens_pixmap)
	XmDestroyPixmap(XtScreen(w), FPART(w).insens_pixmap);
    FPART(w).insens_pixmap =
	XmGetPixmapByDepth(
		XtScreen(w), "50_foreground",
		WhitePixelOfScreen(XtScreen(w)),	/* foreground */
		BlackPixelOfScreen(XtScreen(w)),	/* background */
		1);					/* depth */

} /* end of SetDefaultGCs()  */

/****************************procedure*header*****************************
 * SetFocusToAnyItem - sets focus to the first item willing to take it.
 * TRUE is returned if an item took focus, FALSE is returned otherwise.
 */
static Boolean
SetFocusToAnyItem(Widget w)
{
    Boolean		ret_val = False;

    if (FCLASS(w).item_accept_focus)
    {
	Cardinal	i;
	ExmFLAT_ALLOC_ITEM(w, ExmFlatItem, item);

	for (i = 0; i < FPART(w).num_items; ++i)
	{
	    ExmFlatExpandItem(w, i, item);

	    if ((*FCLASS(w).item_accept_focus)(w, item))
	    {
		ret_val = True;
		break;
	    }
	}
	ExmFLAT_FREE_ITEM(item);
    }
    return(ret_val);
} /* end of SetFocusToAnyItem() */

/****************************procedure*header*****************************
 * UpdateLastSelectedItem- The next to the last item was turned off, so find
 * the only remaining item that is selected.
 */
static void
UpdateLastSelectedItem(Widget w, Cardinal except_idx)
{
    Arg		args[2];
    Boolean	selected;
    Boolean	managed;
    int		i;

    FPART(w).last_select = ExmNO_ITEM;

    XtSetArg(args[0], XmNset, &selected);
    XtSetArg(args[1], XmNmanaged, &managed);
    for (i=0; i < FPART(w).num_items; i++)
	if (i != except_idx)
	{
	    managed = selected = False;
	    ExmFlatGetValues(w, i, args, XtNumber(args));
	    if (managed && selected)
	    {
		FPART(w).last_select = i;
		break;
	    }
	}
}					/* end of UpdateLastSelectedItem */

/****************************class*procedures*****************************
 *
 * Class Procedures
 */

/****************************procedure*header*****************************
 * AnalyzeItems - this routine analyzes new items for this class.
 * This routine is called after all of the items have been initialized,
 * so we can assume that all items have valid values.  If this widget
 * is the focus widget, then the application probably touched the item
 * list, so let's pick an item to take focus.
 */
/* ARGSUSED */
static void
AnalyzeItems(Widget w, ArgList args, Cardinal * num_args)
{
    if (PPART(w).have_traversal)
	(void)SetFocusToAnyItem(w);

    /* If one and only one item must be selected (exclusives && !none_set)
     * and none is, set the first item.
     */
    if (FPART(w).exclusives && !FPART(w).none_set &&
	(FPART(w).select_count == 0))
    {
	Cardinal	indx;
	Arg		set_args[2];
	Boolean		managed;
	Boolean		mapped;

	XtSetArg(set_args[0], XmNmanaged, &managed);
	XtSetArg(set_args[1], XmNmappedWhenManaged, &mapped);

	for (indx = 0; indx < FPART(w).num_items; indx++)
	{
	    managed	= False;
	    mapped	= False;

	    ExmFlatGetValues(w, indx, set_args, XtNumber(set_args));

	    if (managed && mapped)
	    {
		XtSetArg(set_args[0], XmNset, True);
		ExmFlatSetValues(w, indx, set_args, 1);
		break;
	    }
	}
    }
} /* end of AnalyzeItems() */

/****************************procedure*header*****************************
 * ClassInitialize - 
 */
static void
ClassInitialize(void)
{
    void *handle;
    void *font_proc;

    ExmFlatAddConverters();

#ifdef USE_FONT_OBJECT
    handle = dlopen(NULL, RTLD_LAZY);
    font_proc = dlsym(handle, "XmAddDynamicFontListClassExtension");
    if (font_proc)
	XmAddDynamicFontListClassExtension(exmFlatWidgetClass);
    dlclose(handle);

#endif /* USE_FONT_OBJECT */
} /* end of ClassInitialize() */

/****************************procedure*header*****************************
 * ClassPartInitialize - this routine initializes the widget's class
 * part field.  It Quarkifies the classes item's resource names and puts
 * them into a quark list.
 */
static void
ClassPartInitialize(WidgetClass wc)	/* Flat Widget subclass		*/
{
    ExmFlatClassPart * fcp;		/* this class's Flat Class Part	*/
    ExmFlatClassPart * sfcp;		/* superclass's Flat Class Part	*/
    Cardinal	i;

    fcp = &(((ExmFlatWidgetClass) wc)->flat_class);

    /* If this is the flatWidgetClass, quark its item resources and return. */
    if (wc == exmFlatWidgetClass)
    {
	Cardinal	i;
	XrmQuarkList	qlist;
	XtResourceList	rsc = fcp->item_resources;

	qlist = (XrmQuarkList)
	    XtMalloc((Cardinal)(sizeof(XrmQuark) * fcp->num_item_resources));
	fcp->quarked_items = qlist;

	for (i = 0; i < fcp->num_item_resources; ++i, ++qlist, ++rsc)
	{
	    *qlist  = XrmStringToQuark(rsc->resource_name);
	}

	return;
    }

    /* Get the superclasses flat class part	*/
    sfcp = &(((ExmFlatWidgetClass) wc->core_class.superclass)->flat_class);

    /* Now, set up the item resources for this class.  The item resources for
     * this class will be merged with those of the superclass.  If the
     * subclass and superclass item resource have the same offset, the
     * subclasses item resource will be used.
     */
    if (fcp->num_item_resources == (Cardinal)0 ||
	fcp->item_resources != (XtResourceList)NULL)
    {
	/* If this subclass doesn't add any of it's own resources, simply
	 * copy the superclass's pointers into this class's fields.
	 */
	if (fcp->num_item_resources == (Cardinal)0)
	{
	    fcp->quarked_items	= sfcp->quarked_items;
	    fcp->item_resources	= sfcp->item_resources;
	    fcp->num_item_resources	= sfcp->num_item_resources;
	}
	else
	{
	    XrmQuarkList	qlist;
	    XtResourceList	rsc;
	    XtResourceList	fcp_list;
	    XtResourceList	sfcp_list;
	    XrmQuark		quark;
	    Cardinal		i;
	    Cardinal		j;
	    Cardinal		num =
		fcp->num_item_resources + sfcp->num_item_resources;

	    /* Allocate arrays large enough to fit both the superclass and
	     * the subclass item resources.
	     */
	    qlist = (XrmQuarkList) XtMalloc((Cardinal)
					    (sizeof(XrmQuark) * num));
	    fcp->quarked_items = qlist;

	    rsc = (XtResourceList)
		XtMalloc((Cardinal)(sizeof(XtResource) * num));

	    /* Copy the superclass item resources into the new list. */
	    for (i = 0; i < sfcp->num_item_resources; ++i)
	    {
		rsc[i] = sfcp->item_resources[i];
		qlist[i] = sfcp->quarked_items[i];
	    }

	    /* Now merge this class's item resources into the new list. */
	    for (i = 0, num = sfcp->num_item_resources,
		 fcp_list = fcp->item_resources;
		 i < fcp->num_item_resources;
		 ++i, ++fcp_list)
	    {
		quark = XrmStringToQuark(fcp_list->resource_name);

		/* Compare each new item resource against the superclass's */

		for (j=0, sfcp_list = sfcp->item_resources;
		     j < sfcp->num_item_resources;
		     ++j, ++sfcp_list)
		{
		    /* If match, override the super's
		     * with the subclass's		*/
					 
		    if (fcp_list->resource_offset ==
			sfcp_list->resource_offset &&
			quark == sfcp->quarked_items[j])
		    {
			rsc[j] = *fcp_list;
			break;
		    }
		}

		/* If these equal, the item resource
		 * is not in the superclass list	*/

		if (j == sfcp->num_item_resources)
		{
		    qlist[num] = quark;
		    rsc[num] = *fcp_list;
		    ++num;
		}
	    }		/* end of merging in this subclass's resources */

	    /* At this point the two lists are merged.
	     * Now deallocate extra memory.
	     */
	    if (num < (fcp->num_item_resources + sfcp->num_item_resources))
	    {
		rsc = (XtResourceList)XtRealloc((char *) rsc,
						sizeof(XtResource) * num);
		qlist = (XrmQuarkList)XtRealloc((char *) qlist,
						sizeof(XrmQuark) * num);
	    }

	    /* Cache the results in this class.	*/
	    fcp->quarked_items	= qlist;
	    fcp->item_resources	= rsc;
	    fcp->num_item_resources	= num;
	}
    }
    else
    {
	OlVaDisplayErrorMsg(((Display *)NULL,OleNinvalidResource,
			    OleTnullList, OleCOlToolkitError,
			    OleMinvalidResource_nullList, "",
			    OlWidgetClassToClassName(wc),
			    "item_resources", "num_item_resources"));
    }


    /* Inherit procedures that need to be inherited.  If a required class
     * procedure is NULL, an error is generated.
     */
    for (i = XtNumber(class_fields); i--; )
    {
	XtPointer * dest = (XtPointer *)((char *)wc + class_fields[i].offset);

	if (*dest == (XtPointer)_XtInherit)
	{
	    *dest = *(XtPointer*)
		((char *)wc->core_class.superclass + class_fields[i].offset);
	}
	else if (*dest == (XtPointer)NULL && class_fields[i].required)
	{
	    OlVaDisplayErrorMsg(((Display *)NULL,
				OleNinvalidProcedure,
				OleTinheritanceProc, OleCOlToolkitError,
				OleMinvalidProcedure_inheritanceProc,
				OlWidgetClassToClassName(wc),
				class_fields[i].required));
	}
    }

    /* Now check the record size	*/

    if (fcp->rec_size == (Cardinal)0 || fcp->rec_size < sfcp->rec_size)
    {
	OlVaDisplayErrorMsg(((Display *)NULL, OleNinvalidItemRecord,
			    OleTflatState, OleCOlToolkitError,
			    OleMinvalidItemRecord_flatState,
			    OlWidgetClassToClassName(wc),
			    (unsigned)fcp->rec_size, (unsigned)sfcp->rec_size));
    }

    /* Now check to see if the subclass is incorrectly using the CORE
     * Initialize and SetValues procedures.  */

#define OleNimproperSubclassing		"improperSubclassing"
#define OleMimproperSubclassing_flatState "Flat Subclass \"%s\" should not have\
 non-NULL CORE %s procedure, put the procedure pointer in the flat class part"
#define ECHO_ERROR(proc)	\
     OlVaDisplayErrorMsg(((Display *)NULL, OleNimproperSubclassing,\
			 OleTflatState, OleCOlToolkitError,\
			 OleMimproperSubclassing_flatState, \
			 OlWidgetClassToClassName(wc), proc))

    if (wc->core_class.initialize != (XtInitProc)NULL)
    {
	ECHO_ERROR("Initialize");
    }
    if (wc->core_class.set_values != (XtSetValuesFunc)NULL)
    {
	ECHO_ERROR("SetValues");
    }
} /* end of ClassPartInitialize() */

/****************************procedure*header*****************************
 * DefaultItemInitialize - checks the initial values of the default item.
 */
/* ARGSUSED */
static void
DefaultItemInitialize(Widget w,		/* New widget			*/
		      ExmFlatItem request,/* expanded requested item	*/
		      ExmFlatItem new,	/* expanded new item		*/
		      ArgList args,
		      Cardinal * num_args)
{
    if (FITEM(new).label)
	FITEM(new).label = _XmStringCopy(FITEM(new).label);

    /* Insure these fields are not inherited from the container. */

    MAINTAIN_SAME_VALUES(new);
}					/* end of DefaultItemInitialize() */

/****************************procedure*header*****************************
 * DefaultItemSetValues - this routine is called whenever the application
 * does an XtSetValues on the container, possibly requesting that 
 * attributes of the default item be updated.
 * If the "widget" is to be refreshed, the routine returns True.
 */
/* ARGSUSED */
static Boolean
DefaultItemSetValues(Widget w,		/* New widget			*/
		     ExmFlatItem current,/* expanded current item	*/
		     ExmFlatItem request,/* expanded requested item	*/
		     ExmFlatItem new,	/* expanded new item		*/
		     ArgList args,
		     Cardinal * num_args)
{
    Boolean	redisplay = False;

#define DIFFERENT(field)	( FITEM(new).field != FITEM(current).field )

    /* Insure these fields are changed via a set
     * values on the container.			*/

    MAINTAIN_SAME_VALUES(new);

    if (DIFFERENT(label))
    {
	if (FITEM(new).label)
	{
	    FITEM(new).label = _XmStringCopy(FITEM(new).label);
	}

	if (FITEM(current).label)
	{
	    _XmStringFree(FITEM(current).label);
	    FITEM(current).label = NULL;
	}

	FPART(w).relayout_hint = True;
	redisplay = True;
    }

    return(redisplay);

#undef DIFFERENT
} /* end of DefaultItemSetValues() */

/****************************procedure*header*****************************
 * Destroy - this procedure frees memory allocated by the instance part
 */
static void
Destroy(Widget w)
{
    ExmFlatStateDestroy(w);

    /* Destroy the default item's label	*/
    if (FITEM(ExmFlatDefaultItem(w)).label)
	_XmStringFree(FITEM(ExmFlatDefaultItem(w)).label);

    /* Free copy of font list */
    if (FPART(w).font != NULL)
	XmFontListFree(FPART(w).font);

    XmDestroyPixmap(XtScreen(w), FPART(w).insens_pixmap);
    XtReleaseGC(w, FPART(w).normal_gc);
    XtReleaseGC(w, FPART(w).select_gc);
    XtReleaseGC(w, FPART(w).scroll_gc);

} /* end of Destroy() */

/****************************procedure*header*****************************
 * Initialize - this procedure initializes the instance part of the widget
 */
/* ARGSUSED */
static void
Initialize(Widget request,	/* What we want		*/
	   Widget new,		/* What we get, so far..*/
	   ArgList args,
	   Cardinal * num_args)
{
    /* If the items and itemFields converters have not modified the num_items
     * and num_item_fields values, set them now.
     */
    if (FPART(new).num_items == (Cardinal)ExmIGNORE)
    {
	FPART(new).num_items = (Cardinal)0;
    }

    if (FPART(new).num_item_fields == (Cardinal)ExmIGNORE)
    {
	FPART(new).num_item_fields = (Cardinal)0;
    }

    ExmSWinCreateScrollbars(new, &FPART(new).hsb, &FPART(new).vsb,

			    /* initial offset values */
			    &FPART(new).x_offset, &FPART(new).y_offset,

			    (XtCallbackProc)NULL); /* cb set by subclass */

    FPART(new).in_geom_hdr	= False;
    FPART(new).focus_item	= ExmNO_ITEM;
    FPART(new).last_focus_item	= ExmNO_ITEM;

    FPART(new).min_x		= 0;
    FPART(new).min_y		= 0;

    /* Modify 'none_set' for Nonexclusives */
    if (!FPART(new).exclusives && !FPART(new).none_set)
	FPART(new).none_set = True;

    /* Deal with font */
    if (FPART(new).font == NULL)
	FPART(new).font = _XmGetDefaultFontList(new, XmLABEL_FONTLIST);

    /* make a local copy of the font list.
     * See Label.c:Initialize, can use some improvements there
     */
    FPART(new).font = XmFontListCopy(FPART(new).font);

    /* Set the GCs */
    FPART(new).normal_gc	= NULL;
    FPART(new).select_gc	= NULL;
    FPART(new).scroll_gc	= NULL;
    FPART(new).insens_pixmap	= NULL;
    SetDefaultGCs(new);

    if (FPART(new).items_touched)
    {
	ItemsTouched(new);
    }
} /* end of Initialize() */

/****************************procedure*header*****************************
 * HighlightBorder - this routine is called whenever the flattened widget
 * container gains focus.
 */
static void 
HighlightBorder(Widget wid)
{
    /* If we get a focus in, try to set focus to the last item that had it
     * (since the focus out may have been caused by a pointer grab, e.g., the
     * window manager grabbed the pointer when dragging the window).  But if
     * the focus out wasn't due to a grab, then we have to check to see if
     * the last focus item still can take focus.  So in either case, we have
     * to play it safe and formally request the item to take focus.
     */
    if ((FPART(wid).last_focus_item == ExmNO_ITEM) ||
	!ExmFlatCallAcceptFocus(wid, FPART(wid).last_focus_item))
    {
	FPART(wid).last_focus_item = ExmNO_ITEM;
	(void)SetFocusToAnyItem(wid);
    }
} /* end of BorderHighlight() */

/****************************procedure*header*****************************
 * UnhighlightBorder - this routine is called whenever the flattened widget
 * container looses focus.
 */
static void 
UnhighlightBorder(Widget wid)
{
    if (FPART(wid).focus_item != ExmNO_ITEM)
    {
	ExmFLAT_ALLOC_ITEM(wid, ExmFlatItem, item);

	ExmFlatExpandItem(wid, FPART(wid).focus_item, item);

	FPART(wid).focus_item = ExmNO_ITEM;

	CALL_ITEM_HIGHLIGHT(wid, item, FocusOut);

	ExmFLAT_FREE_ITEM(item);
    }
} /* end of BorderUnhighlight() */

/****************************procedure*header*****************************
 * ItemAcceptFocus - this routine is called to set focus to a particular
 * sub-object.  If this object can take focus, it set focus to itself.
 */
static Boolean
ItemAcceptFocus(Widget w, ExmFlatItem item)
{
    Boolean	can_take_focus;
    Cardinal	old_focus_item;

    if (!FITEM(item).sensitive	|| !FITEM(item).traversal_on		||
	!FITEM(item).managed	|| !FITEM(item).mapped_when_managed	||
	/* NOTE: Should we use _XmIsTraversable(w, True)? */
	(!PPART(w).have_traversal && !XmIsTraversable(w)) )
    {
	return(False);
    }

    /* If the requested item can take focus and the flat container
     * already has focus, call the unhighlight handler for the item that
     * has focus and the highlight handler the new focus item.  If the
     * container does not have focus yet, set it to the container.
     */
    if (PPART(w).have_traversal)
    {
	can_take_focus = True;

	old_focus_item			= FPART(w).focus_item;
	FPART(w).last_focus_item	= FITEM(item).item_index;
	FPART(w).focus_item		= FITEM(item).item_index;
	if (old_focus_item != ExmNO_ITEM)
	{
	    ExmFLAT_ALLOC_ITEM(w, ExmFlatItem, old_item);

	    ExmFlatExpandItem(w, old_focus_item, old_item);
	    CALL_ITEM_HIGHLIGHT(w, old_item, FocusOut);

	    ExmFLAT_FREE_ITEM(old_item);
	}

	CALL_ITEM_HIGHLIGHT(w, item, FocusIn);

    } else
    {
	/* Set focus to the container.  This will cause the container's
	 * HighlightBorder method to be called which will call ItemHighlight
	 * which will use last_focus_item.
	 *
	 * NOTE: must set last_focus_item *before* calling XmProcessTraversal
	 * because Focus change will happen *in line* in Motif.  Therefore,
	 * it is set before we can see if XmProcessTraversal succeeds; we
	 * assume it will succeed since most of the important checks have
	 * been made above.  Alas, some checks will be done again by
	 * XmProcessTraversal (in particular, XmIsTraversable()).
	 */
	old_focus_item			= FPART(w).last_focus_item;
	FPART(w).last_focus_item	= FITEM(item).item_index;
	if ( !(can_take_focus = XmProcessTraversal(w, XmTRAVERSE_CURRENT)) )
	    FPART(w).last_focus_item = old_focus_item;
    }

    return(can_take_focus);

} /* end of ItemAcceptFocus() */

/****************************procedure*header*****************************
 * ItemHighlight - this routine is called to highlight or unhighlight a
 * particular sub-object as it gains or loses focus.
 */
/* ARGSUSED2 */
static void
ItemHighlight(Widget w, ExmFlatItem item, int type)
{
    WidePosition	x, y;
    Dimension		wd, hi;

    /* Maintain primitive state.  This is normally done by primitive
     * border highlight methods (HighlightBorder and UnhighlightBorder).
     */
    PPART(w).highlighted = PPART(w).highlight_drawn = (type == FocusIn);

    /* Only draw the highlighting if the item is in view */

    ExmFlatGetItemGeometry(w, FITEM(item).item_index, &x, &y, &wd, &hi);

    /* Adjust (x, y) because we are dealing with virtual coords, */
    x -= FPART(w).x_offset;
    y -= FPART(w).y_offset;

#define ULx	0
#define ULy	0
#define LRx	(WidePosition)CPART(w).width
#define LRy	(WidePosition)CPART(w).height

    /* Call draw_proc only when it's visible */
    if (((x >= ULx &&  x < LRx) || (x <= ULx &&
				    ((x + (WidePosition)wd) > ULx))) &&
        ((y >= ULy &&  y < LRy) || (y <= ULy &&
				    ((y + (WidePosition)hi) > ULy))))
    {
	ExmFlatRefreshExpandedItem(w, item, True);
    }
#undef ULx
#undef ULy
#undef LRx
#undef LRy
}					/* end of ItemHighlight() */

/****************************procedure*header*****************************
 * ItemsTouched - this procedure is called whenever a new list is given to
 * the flat widget or whenever the existing list is touched.  These
 * conditions are checked for the Initialize and SetValues routines.
 * In this routine, we de-allocate any memory associated with the last
 * list and create any memory needed for the new list.
 */
static void
ItemsTouched(Widget new)
{ 
    if (FPART(new).focus_item != ExmNO_ITEM)
    {
	FPART(new).last_focus_item	= ExmNO_ITEM;
	FPART(new).focus_item		= ExmNO_ITEM;
    }

    FPART(new).last_select	= ExmNO_ITEM;
    FPART(new).select_count	= 0;

    /* Once we've set the focus item, reset the last focus item. */

    FPART(new).last_focus_item = FPART(new).focus_item;

} /* end of ItemsTouched() */

/****************************procedure*header*****************************
 * ItemInitialize - this procedure checks a single item to see if
 * it has valid values.
 */
static void
ItemInitialize(
	Widget w, ExmFlatItem request, ExmFlatItem new,
	ArgList args, Cardinal * num_args)
{
	if (FPART(w).exclusives && FITEM(new).selected)
	{
		if (FPART(w).last_select != (Cardinal)ExmNO_ITEM)
		{
			printf("Warning: unselecting item %u because item %u\
 is already set\n", FITEM(new).item_index, FPART(w).last_select);
			FITEM(new).selected = False;
		}
		else
		{
			FPART(w).last_select = FITEM(new).item_index;
			FPART(w).select_count = 1;
		}
	}
	else if (FITEM(new).selected)
	{
		if (FPART(w).last_select == (Cardinal)ExmNO_ITEM)
			FPART(w).last_select = FITEM(new).item_index;
		FPART(w).select_count++;
	}
} /* end of ItemInitialize */

/****************************procedure*header*****************************
 * ItemSetValues - this routine is called whenever the application does
 * an XtSetValues on the container, requesting that an item be updated.
 * If the item is to be refreshed, the routine returns True.
 */
/* ARGSUSED */
static Boolean
ItemSetValues(Widget w,			/* Flat widget container id	*/
	      ExmFlatItem current,	/* expanded current item	*/
	      ExmFlatItem request,	/* expanded requested item	*/
	      ExmFlatItem new,		/* expanded new item		*/
	      ArgList args,
	      Cardinal * num_args)
{
    Boolean	reconfigure = False;
    Boolean	redisplay = False;
    Boolean	call_refresh = False;

#define DIFFERENT(field)	(FITEM(new).field != FITEM(current).field)

    if (DIFFERENT(mapped_when_managed) ||
	DIFFERENT(managed) ||
	DIFFERENT(sensitive)	||
	DIFFERENT(label))
    {
#ifdef MOOLIT
	if ( (DIFFERENT(sensitive) && !FITEM(new).sensitive ||
	      DIFFERENT(managed) && !FITEM(new).managed ||
	      DIFFERENT(mapped_when_managed) &&
	      !FITEM(new).mapped_when_managed) &&
	    FITEM(new).item_index == FPART(w).focus_item)
	{
	    _OlSetCurrentFocusWidget(w, FoucsOut);
	    if (w == OlMoveFocus(w, OL_IMMEDIATE, CurrentTime))
		_OlSetCurrentFocusWidget(w, FocusIn);
	}
#endif
	if (DIFFERENT(managed) && !FITEM(new).managed &&
	    (FPART(w).last_select == FITEM(new).item_index) &&
	    (FPART(w).select_count <= 2))
	{
	    UpdateLastSelectedItem(w, FITEM(new).item_index);
	}

	if (DIFFERENT(label))
	{
	    reconfigure = True;
	}
	redisplay = True;
    }

    if (DIFFERENT(selected))
    {
	int actions;

	redisplay = True;

	/* The actions to be performed here depends on 3 parameters, set,
	 * none_set, and the new exclusives setting. Because similar
	 * actions may be taken in various combination of the 3 parameters,
	 * the actions will be table driven.
	 */
	actions = set_actions[ACTION_IDX(FITEM(new).selected,
					 FPART(w).none_set,
					 FPART(w).exclusives)];

	if (actions & CHK_CNT)
	    if (FPART(w).select_count <= 1)
		goto skip;

	if (actions & RESET)
	    if ((FPART(w).last_select != ExmNO_ITEM) &&
		(FPART(w).last_select != FITEM(new).item_index))
	    {
		Arg largs[1];

		XtSetArg(largs[0], XmNset, False);
		ExmFlatSetValues(w, FPART(w).last_select, largs, 1);
	    }

	if (actions & INC)
	    FPART(w).select_count++;
	else if (actions & DEC)
	    FPART(w).select_count--;
	else if (actions & ONE)
	    FPART(w).select_count = 1;

	if (actions & CHK_OK)
	    FPART(w).last_select =
		(FITEM(new).selected) ? FITEM(new).item_index : ExmNO_ITEM;

	if ((ExmFLATCLASS(w).mask | ExmFLAT_HANDLE_RAISE) &&
	    (actions & RAISE))
	{
		call_refresh = True;
	}
    }

 skip:
    if (reconfigure)
	ExmFlatItemDimensions(w, new, &FITEM(new).width, &FITEM(new).height);

	/* Subclass is responsible for either returning
	 * True from ItemSetValues() or calling
	 * ExmFlatRaiseExpandedItem(). */
    return(call_refresh ? False : redisplay);

#undef DIFFERENT
} /* end of ItemSetValues() */

/****************************procedure*header*****************************
 * QueryGeom - we will handle `query_geom' only if the parent is
 *	XmScrolledWindow.
 */
static XtGeometryResult
QueryGeom(Widget w, XtWidgetGeometry * req, XtWidgetGeometry * rep)
{
#define CALL_RESIZE (FPART(w).vsb && (req->request_mode & (CWWidth | CWHeight)))

    if (FPART(w).in_geom_hdr || !CALL_RESIZE || !FPART(w).num_items)
    {
		/* Assume the first two are False when 3rd is True */
	if (!FPART(w).num_items)
	{
		FPART(w).actual_width = req->width;
		FPART(w).actual_height = req->height;
	}
	return(XtGeometryYes);
    }
    else
    {
	   return(ExmSWinHandleResize(w, FPART(w).hsb, FPART(w).vsb,

			       FPART(w).min_x, FPART(w).min_y,

			       FPART(w).actual_width, FPART(w).actual_height,

			       /* slider units in x/y directions */
			       ExmFLAT_X_UOM(w), ExmFLAT_Y_UOM(w),

			       /* current offset values */
			       &FPART(w).x_offset, &FPART(w).y_offset,
			       req, rep));
    }
#undef CALL_RESIZE
}					/* end of QueryGeom */

/****************************procedure*header*****************************
 * SetValues - this procedure monitors the changing of instance data.
 * Since the subclasses inherit the layout from this class, whenever this
 * class detects a change in any of its layout parameters, it sets a flag
 * in the instance part so that the subclasses merely have to check the
 * flag instead of checking all of their layout parameters.
 */
/* ARGSUSED */
static Boolean
SetValues(Widget current,	/* What we had		*/
	  Widget request,	/* What we want		*/
	  Widget new,		/* What we get, so far	*/
	  ArgList args,
	  Cardinal * num_args)
{
    Arg		arg[1];
    Boolean	redisplay = False;

#define CDIFF(field)	(CPART(new).field != CPART(current).field)
#define PDIFF(field)	(PPART(new).field != PPART(current).field)
#define FDIFF(field)	(FPART(new).field != FPART(current).field)


	/* Notes for the next two if blocks:
	 * The current fix is a quicky because we always revert the
	 * offset to `0'. A more correct fix shall be -
	 *   a. revert to `0' only when items_touched and relayout_hint
	 *	are False. In this case, change_managed() won't be called
	 *      so we have to reset it to a reasonable value.
	 *      Note that this is what dtm is doing today.
	 *   b. if items_touched and/or relayout_hint are/or True, then
	 *      we should let change_managed() (or more correctly
	 *      ExmSWinCalcViewSize) readjust the values based on the
	 *      new uom(s) and physical sizes.
	 */
    if (FDIFF(col_width) && FPART(new).x_offset) {

	if (FPART(new).hsb) {
		XtSetArg(arg[0], XmNvalue, 0);
		XtSetValues(FPART(new).hsb, arg, 1);
	}
	FPART(new).x_offset = 0;
    }

    if (FDIFF(row_height) && FPART(new).y_offset) {

	if (FPART(new).vsb) {
		XtSetArg(arg[0], XmNvalue, 0);
		XtSetValues(FPART(new).vsb, arg, 1);
	}
	FPART(new).y_offset = 0;
    }

    /* Check resources that are creation-only... */
    if (FDIFF(want_graphics_expose))
	FPART(new).want_graphics_expose = FPART(current).want_graphics_expose;

    /* Must 'touch items' if changing 'exclusives' */
    if (FDIFF(exclusives) && !FPART(new).items_touched)
    {
	FPART(new).exclusives = FPART(current).exclusives;

	OlVaDisplayWarningMsg((XtDisplay(new),
			      OleNfileFExclusive, OleTsetValues,
			      OleCOlToolkitWarning,
			      OleMfileFExclusive_msg1,
			      XtName(new), OlWidgetToClassName(new)));
    }

    /* Modify 'none_set' for Nonexclusives */
    if (FDIFF(none_set) && !FPART(new).exclusives && !FPART(new).none_set)
	FPART(new).none_set = True;

    if (FDIFF(items_touched))
    {
	redisplay = True;
	ItemsTouched(new);
    }

    /* If the sensitivity is changed, then we should force a redisplay. Note:
     * we don't need to track the ancestor_sensitive because the Xt manual
     * said that: you should use XtSetSensitive() so tracking "sensitive" is
     * enough.
     */
    if (XtIsSensitive(new) != XtIsSensitive(current))
    {
	redisplay = True;
    }

    /* If anything related to drawing has changed, set new GCs */
    if 	(CDIFF(background_pixel) || PDIFF(foreground) || FDIFF(font))
    {
	redisplay = True;
	if (FDIFF(font))  {
	    if (FPART(new).font == NULL)
		FPART(new).font = _XmGetDefaultFontList(new, XmLABEL_FONTLIST);
	    FPART(new).font = XmFontListCopy(FPART(new).font);
	    if (FPART(current).font)
		XmFontListFree(FPART(current).font);
	    FPART(new).relayout_hint = True;
	}
	SetDefaultGCs(new);
    }

    if (PDIFF(shadow_thickness) || PDIFF(highlight_thickness))
    {
	FPART(new).relayout_hint = True;

	redisplay = True;
    }

    return(redisplay);

#undef CDIFF
#undef PDIFF
#undef FDIFF
} /* end of SetValues() */

/****************************action*procedures****************************
 *
 * Action Procedures
 */

/****************************procedure*header*****************************
 * FlatTraverseAction- action procedure for traversal.  Dispatch user's
 * traverse request to sub-class' traverse_items method.
 *
 * NOTE: direction of traversal is passed as param.  These are defined for
 * subclasses in FlatP.h.
 */
static void
FlatTraverseAction(Widget w, XEvent * event,
		   String * params, Cardinal * num_params)
{
    if (FPART(w).focus_item != ExmNO_ITEM &&
	(*num_params == 1) && FCLASS(w).traverse_items)
    {
	(void)(*FCLASS(w).traverse_items)(w, FPART(w).focus_item,
						(*params)[0]);
    }
} /* end of FlatTraverseAction */

/****************************public*procedures****************************
 *
 * Public Procedures
 */

/****************************procedure*header*****************************
 * ExmFlatInheritAll - procedure used by subclass's to inherit all 
 * 'inheritable' attributes from its superclass.  This procedure is
 * called from a subclass's ClassInitialize procedure.  After calling
 * this procedure, a subclass usually then overrides a few of the
 * inherited routines to give the subclass some unique characteristics
 * of it's own.
 */
void
ExmFlatInheritAll(WidgetClass wc)
{
    Cardinal	i = XtNumber(class_fields);
    XtPointer	dest;

    while(i--)
    {
	*(XtPointer*)((char*)wc + class_fields[i].offset) = 
	    *(XtPointer*)((char*)wc->core_class.superclass +
			  class_fields[i].offset);
	dest = *(XtPointer*)((char*)wc + class_fields[i].offset);
	dest = dest;
    }
} /* end of ExmFlatInheritAll */
