#ifndef NOIDENT
#pragma ident	"@(#)libMDtI:FIconbox.c	1.2"
#endif

/******************************file*header********************************
 *
 * Description:
 *	This file contains the source code for the flat iconBox
 *	widget.
 */



						/* #includes go here	*/
#include <string.h>			/* for memcpy */
#include "FIconboxP.h"
#include "Xm/RepTypeI.h"		/* for XmRepTypeId etc. */

/*****************************file*variables******************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 */

#define DEBUG(w)		IBPART(w).ib_debug
#define SHOW_BUSY_LABEL(w)	IBPART(w).show_busy_label

#define OBJECT_DATA(I)		((ExmFIconboxGlyphData *)IITEM(I).object_data)
#define VALID_PIXMAP(W,I)	(IBPART(W).use_obj_data ?\
				 (OBJECT_DATA(I) != NULL		&&\
				  OBJECT_DATA(I)->pixmap != None	&&\
				  OBJECT_DATA(I)->depth  != 0		&&\
				  OBJECT_DATA(I)->width  != 0		&&\
				  OBJECT_DATA(I)->height != 0) :\
				 (IBITEM(I).pixmap != None		&&\
				  IBITEM(I).depth  != 0			&&\
				  IBITEM(I).width  != 0			&&\
				  IBITEM(I).height != 0))
	/* Use the marco below to access pixmap, width, height, and depth */
#define GLYPH_DATA(W,I,f)	(IBPART(W).use_obj_data ?\
					OBJECT_DATA(I)->f : IBITEM(I).f)

#define IS_LONG_LABEL(W)	(IBPART(W).label_pos == XmRIGHT_LABELS ||\
				 IBPART(W).label_pos == XmLEFT_LABELS)

#define LABEL_ON_RIGHT(W)	(IBPART(W).label_pos == XmRIGHT_LABEL ||\
				 IBPART(W).label_pos == XmRIGHT_LABELS)

#undef MAX
#undef MIN

#define MAX(x, y)	(((x) > (y)) ? (x) : (y))
#define MIN(x, y)	(((x) < (y)) ? (x) : (y))

static XmRepTypeId	XmRID_LABEL_POS;
static char * label_pos_names[] = {
	"bottom_label",
	"top_label",
	"left_label",
	"right_label",
	"left_labels",
	"right_labels",
};

static XmRepTypeId	XmRID_DIM_POLICY;
static char * dim_policy_names[] = {
	"automatic",
	"application_defined",
};

/**************************forward*declarations***************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 */
					/* private procedures		*/
static Boolean	AddFocusSpace(Widget, Dimension *, Dimension *,
						Dimension *, Dimension *);
static void	CalcItemDimension(Widget, ExmFlatItem);
static void	CalcGlyphDimension(Widget, ExmFlatItem, Dimension*, Dimension*);
static void	CalcLabelDimension(Widget, ExmFlatItem, Dimension*, Dimension*);
static void	CalcShortLabelDimension(Widget, ExmFlatItem, _XmString,
						Dimension *, Dimension *);
static void	CalcLongLabelDimension(Widget, ExmFlatItem,
						Dimension *, Dimension *);
static Boolean	CheckThisItemRsc(Widget, Boolean, String, ArgList, Cardinal *);
static Boolean	CheckThisRsc(Widget, String, ArgList, Cardinal *,
					Dimension, Dimension, Boolean *);
static void	MallocMaxColWidth(Widget, Widget);

					/* class procedures		*/
static void	ClassInitialize(void);
static void	Destroy(Widget);
static void	Initialize(Widget, Widget, ArgList, Cardinal *);
static Boolean	SetValues(Widget, Widget, Widget, ArgList, Cardinal *);

static void	AnalyzeItems(Widget, ArgList, Cardinal *);
static void	DefaultItemLabelsCopy(Widget, ExmFlatItem);
static void	DefaultItemLabelsDestroy(Widget, ExmFlatItem);
static void	DefaultItemObjectDataCopy(Widget, ExmFlatItem);
static void	DefaultItemObjectDataDestroy(Widget, ExmFlatItem);
static void	DefaultItemInitialize(Widget, ExmFlatItem, ExmFlatItem,
							ArgList, Cardinal *);
static Boolean	DefaultItemSetValues(Widget, ExmFlatItem, ExmFlatItem,
					ExmFlatItem, ArgList, Cardinal *);
static void	DrawItemProc(Widget, ExmFlatItem, ExmFlatDrawInfo *);
static void	ItemInitialize(Widget, ExmFlatItem, ExmFlatItem,
							ArgList, Cardinal *);
static void	ItemDimensions(Widget, ExmFlatItem, Dimension *, Dimension *);
static Boolean  ItemSetValues(Widget, ExmFlatItem, ExmFlatItem,
					ExmFlatItem, ArgList, Cardinal *);

					/* action procedures		*/

/***********************widget*translations*actions***********************
 *
 * Define Translations and Actions
 */

/****************************widget*resources*****************************
 *
 * Define Resource list associated with the Widget Instance
 */

#undef  OFFSET
#define OFFSET(f)	XtOffsetOf(ExmFlatIconboxRec, icon_box.f)

static XtResource
resources[] = {
		/* Override superclass resources */

			/* Set XmNgridHeight and XmNgridWidth to 0 initially,
			 * so that I can set calc_[xy]_uom values properly */
	{ XmNgridHeight, XmCGridHeight, XmRVerticalDimension,
	  sizeof(Dimension),
	  XtOffset(ExmFlatIconboxWidget, flat.row_height),
	  XmRImmediate, (XtPointer)0
	},
	{ XmNgridWidth, XmCGridWidth, XmRHorizontalDimension,
	  sizeof(Dimension),
	  XtOffset(ExmFlatIconboxWidget, flat.col_width),
	  XmRImmediate, (XtPointer)0
	},
	{ XmNdrawProc, XmCFunction, XtRFunction,
	  sizeof(ExmFlatDrawItemProc),
	  XtOffset(ExmFlatIconboxWidget, iconBox.draw_proc),
	  XmRImmediate, (XtPointer)NULL
	},
		/* My resources */
	{ XmNcolumnSpacing, XmCSpacing, XmRHorizontalDimension,
	  sizeof(Dimension), OFFSET(col_spacing),
	  XmRImmediate, (XtPointer)5
	},
	{ XmNdimensionPolicy, XmCDimensionPolicy, XmRDimensionPolicy,
	  sizeof(unsigned char), OFFSET(dim_policy),
	  XmRImmediate, (XtPointer)XmAUTOMATIC
	},
			/* Set XmNmaxItemHeight and XmNmaxItemWidth to 0
			 * initially, so that I can set calc_max_[hi|wd]
			 * values properly */
	{ XmNmaxItemHeight, XmCMaxItemHeight, XmRVerticalDimension,
	  sizeof(Dimension), OFFSET(max_item_hi),
	  XmRImmediate, (XtPointer)0
	},
	{ XmNmaxItemWidth, XmCMaxItemWidth, XmRHorizontalDimension,
	  sizeof(Dimension), OFFSET(max_item_wd),
	  XmRImmediate, (XtPointer)0
	},
	{ XmNlabelCount, XmCLabelCount, XmRDimension,
	  sizeof(Dimension), OFFSET(num_labels),
	  XmRImmediate, (XtPointer)0
	},
	{ XmNlabelPosition, XmCLabelPosition, XmRLabelPosition,
	  sizeof(unsigned char), OFFSET(label_pos),
	  XmRImmediate, (XtPointer)XmBOTTOM_LABEL
	},
	{ XmNorientation, XmCOrientation, XmROrientation,
	  sizeof(unsigned char), OFFSET(orientation),
	  XmRImmediate, (XtPointer)XmHORIZONTAL
	},
	{ XmNrowSpacing, XmCSpacing, XmRVerticalDimension,
	  sizeof(Dimension), OFFSET(row_spacing),
	  XmRImmediate, (XtPointer)5
	},
	{ XmNuseObjectData, XmCUseObjectData, XmRBoolean,
	  sizeof(Boolean), OFFSET(use_obj_data),
	  XmRImmediate, (XtPointer)True
	},
	{ XmNibDebug, XmCDebug, XmRBoolean,
	  sizeof(Boolean), OFFSET(ib_debug),
	  XmRImmediate, (XtPointer)False
	},
	{ XmNshowBusyLabel, XmCShowBusyLabel, XmRBoolean,
	  sizeof(Boolean), OFFSET(show_busy_label),
	  XmRImmediate, (XtPointer)False
	},
};

#if 0
/*  Definition for resources that need special processing in get values  */

static XmSyntheticResource syn_resources[] = {
};
#endif

				/* Define Resources for sub-objects	*/

#undef  OFFSET
#define OFFSET(f)	XtOffsetOf(ExmFlatIconboxItemRec, icon_box.f)

static XtResource
item_resources[] = {
	{ XmNglyphDepth, XmCGlyphDepth, XtRInt, sizeof(int),
	  OFFSET(depth), XtRImmediate, (XtPointer)1
	},
	{ XmNglyphHeight, XmCGlyphHeight, XtRDimension, sizeof(Dimension),
	  OFFSET(height), XtRImmediate, (XtPointer)0
	},
	{ XmNglyphWidth, XmCGlyphWidth, XtRDimension, sizeof(Dimension),
	  OFFSET(width), XtRImmediate, (XtPointer)0
	},
	{ XmNmask, XmCPixmap, XtRBitmap, sizeof(Pixmap),
	  OFFSET(mask), XtRImmediate, (XtPointer)None
	},
	{ XmNpixmap, XmCPixmap, XtRBitmap, sizeof(Pixmap),
	  OFFSET(pixmap), XtRImmediate, (XtPointer)None
	},
		/* Should be *_XmString* if there is one */
	{ XmNlabels, XmCLabels, XmRXmStringTable, sizeof(XmStringTable),
	  OFFSET(labels), XtRImmediate, (XtPointer)NULL
	},
};

/***************************widget*class*record***************************
 *
 * Define Class Record structure to be initialized at Compile time
 */

ExmFlatIconboxClassRec
exmFlatIconboxClassRec = {
    {
	(WidgetClass)&exmFlatIconBoxClassRec,	/* superclass		*/
	"ExmFlatIconbox",			/* class_name		*/
	sizeof(ExmFlatIconboxRec),		/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	NULL,					/* initialize		*/
	NULL,					/* initialize_hook	*/
	XtInheritRealize,			/* realize		*/
	NULL,					/* actions		*/
	(Cardinal)0,				/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	TRUE,					/* compress_motion	*/
  XtExposeCompressMultiple | XtExposeGraphicsExposeMerged, /*compress_exposure*/
	TRUE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	Destroy,				/* destroy		*/
	NULL,					/* resize		*/
	XtInheritExpose,			/* expose		*/
	NULL,					/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	NULL,					/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_offsets	*/
	XtInheritTranslations,			/* tm_table		*/
	XtInheritQueryGeometry,			/* query_geometry	*/
	NULL,					/* display_accelerator	*/
	NULL					/* extension		*/
    }, /* End of Core Class Part Initialization */
    {
	XmInheritBorderHighlight,		/* border_highlight   */
	XmInheritBorderUnhighlight,		/* border_unhighlight */
	XtInheritTranslations, 			/* translations       */
	NULL,					/* arm_and_activate   */
	NULL,					/* syn resources      */
	0,					/* num syn_resources  */
	NULL,					/* extension          */
    },	/* End of XmPrimitive Class Part Initialization */
    {
	(XtPointer)NULL,			/* extension		*/
	XtOffsetOf(ExmFlatIconboxRec, default_item),/* default_offset	*/
	sizeof(ExmFlatIconboxItemRec),		/* rec_size		*/
	item_resources,				/* item_resources	*/
	XtNumber(item_resources),		/* num_item_resources	*/
	ExmFLAT_HANDLE_RAISE,			/* mask			*/
	NULL,					/* quarked_items	*/

		/*
		 * See ClassInitialize for procedures
		 */

    }, /* End of ExmFlat Class Part Initialization */
    {
	True					/* check_uoms		*/
    }, /* End of ExmFlatGraph Class Part Initialization */
    {
	NULL					/* no_class_fields	*/
    } /* End of ExmFlatIconBox Class Part Initialization */
};

/*************************public*class*definition*************************
 *
 * Public Widget Class Definition of the Widget Class Record
 */

WidgetClass exmFlatIconboxWidgetClass = (WidgetClass) &exmFlatIconboxClassRec;

/***************************private*procedures****************************
 *
 * Private Procedures
 */

/****************************class*procedures*****************************
 *
 * Class Procedures
 */

/*****************************procedure*header*****************************
 * ClassInitialize -
 */
static void
ClassInitialize(void)
{
	XmRID_LABEL_POS = XmRepTypeRegister(
				XmRLabelPosition, label_pos_names,
				NULL, XtNumber(label_pos_names));

	XmRID_DIM_POLICY = XmRepTypeRegister(
				XmRDimensionPolicy, dim_policy_names,
				NULL, XtNumber(dim_policy_names));

	ExmFlatInheritAll(exmFlatIconboxWidgetClass);


#undef F
#define F	exmFlatIconboxClassRec.flat_class

	F.initialize		= Initialize;
	F.set_values		= SetValues;

	F.analyze_items		= AnalyzeItems;
	F.default_initialize	= DefaultItemInitialize;
	F.default_set_values	= DefaultItemSetValues;

	F.item_dimensions	= ItemDimensions;
	F.item_initialize	= ItemInitialize;
	F.item_set_values	= ItemSetValues;
#undef F
} /* end of ClassInitialize() */

/*****************************procedure*header*****************************
 * Destroy -
 */
static void
Destroy(Widget w)
{
	if (SHOW_BUSY_LABEL(w))
		XmDestroyPixmap(XtScreen(w), IBPART(w).label_insens);

	if (IBPART(w).max_col_wd)
		XtFree((XtPointer)IBPART(w).max_col_wd);

	if (IBITEM(ExmFlatDefaultItem(w)).labels) {

		DefaultItemLabelsDestroy(w, ExmFlatDefaultItem(w));
	}

	if (IITEM(ExmFlatDefaultItem(w)).object_data) {

		DefaultItemObjectDataDestroy(w, ExmFlatDefaultItem(w));
	}
} /* end of Destroy */

/****************************procedure*header*****************************
 * Initialize -
 */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal * num_args)
{
	if (IPART(new).draw_proc != DrawItemProc)
		IPART(new).draw_proc = DrawItemProc;

	if (IBPART(new).dim_policy != XmAUTOMATIC &&
	    !XmRepTypeValidValue(
			XmRID_DIM_POLICY, IBPART(new).dim_policy, new))
		IBPART(new).dim_policy = XmAUTOMATIC;

	if (IBPART(new).orientation != XmHORIZONTAL &&
	    !XmRepTypeValidValue(
			XmRID_ORIENTATION, IBPART(new).orientation, new))
		IBPART(new).orientation = XmHORIZONTAL;

	if (IBPART(new).label_pos != XmBOTTOM_LABEL &&
	    !XmRepTypeValidValue(XmRID_LABEL_POS, IBPART(new).label_pos, new))
		IBPART(new).label_pos = XmBOTTOM_LABEL;

	IBPART(new).max_col_wd = NULL;
	if (IBPART(new).dim_policy == XmAPPLICATION_DEFINED) {

		IBPART(new).calc_x_uom =
		IBPART(new).calc_y_uom = False;

		if (!ExmFLAT_X_UOM(new))
			ExmFLAT_X_UOM(new) = 1;

		if (!ExmFLAT_Y_UOM(new))
			ExmFLAT_Y_UOM(new) = 1;
	} else {				/* XmAUTOMATIC */

		IBPART(new).calc_x_uom = (ExmFLAT_X_UOM(new) == 0);
		IBPART(new).calc_y_uom = (ExmFLAT_Y_UOM(new) == 0);

		if (IBPART(new).num_labels)
			MallocMaxColWidth(new, (Widget)NULL);
		else if (IS_LONG_LABEL(new)) {
			IBPART(new).label_pos = XmBOTTOM_LABEL;
			XtWarning("XmNlabelCount can't be zero,\
 when XmNlabelPosition is *_LABELS, fallback to XmNBOTTOM_LABEL");
		}
	}

	IBPART(new).calc_max_hi = (IBPART(new).max_item_hi == 0);
	IBPART(new).calc_max_wd = (IBPART(new).max_item_wd == 0);

	if (SHOW_BUSY_LABEL(new)) {

			/* Initialize insensitive pixmaps for label (as tile) */
		IBPART(new).label_insens = XmGetPixmapByDepth(
					XtScreen(new), "50_foreground",
		/* foreground */	PPART(new).foreground,
		/* background */	CPART(new).background_pixel,
		/* depth */		CPART(new).depth);
	}
} /* end of Initialize */

/****************************procedure*header*****************************
 * MallocMaxColWidth -
 */
static void
MallocMaxColWidth(Widget new, Widget current)
{
	register int	i;

	IBPART(new).max_col_wd = (Dimension *)XtMalloc(sizeof(Dimension) *
						(IBPART(new).num_labels));

	for (i = 0; i < (int)IBPART(new).num_labels; i++)
		IBPART(new).max_col_wd[i] = 0;

	if (current && IBPART(current).max_col_wd)
		XtFree((XtPointer)IBPART(current).max_col_wd);
} /* end of MallocMaxColWidth */

/****************************procedure*header*****************************
 * SetValues -
 */
static Boolean
SetValues(Widget current, Widget request, Widget new,
	  ArgList args, Cardinal * num_args)
{
#define CDIFF(f)	(CPART(new).f != CPART(current).f)
#define PDIFF(f)	(PPART(new).f != PPART(current).f)
#define FDIFF(f)	(FPART(new).f != FPART(current).f)
#define IDIFF(f)	(IPART(new).f != IPART(current).f)
#define IBDIFF(f)	(IBPART(new).f != IBPART(current).f)

	Boolean		redisplay = False;

	/* Check superclass resources that are creation-only... */

	if (IDIFF(draw_proc))
		IPART(new).draw_proc = DrawItemProc;

	/* Check the resources that are creatation-only... */

	if (IBDIFF(dim_policy))
		IBPART(new).dim_policy = IBPART(current).dim_policy;

	if (IBDIFF(show_busy_label))
		IBPART(new).show_busy_label = IBPART(current).show_busy_label;

	/* Check superclass resources that are settable... */

	if (IBPART(new).dim_policy == XmAPPLICATION_DEFINED) {

		IBPART(new).calc_x_uom =
		IBPART(new).calc_y_uom = False;

		if (!ExmFLAT_X_UOM(new))
			ExmFLAT_X_UOM(new) = 1;

		if (!ExmFLAT_Y_UOM(new))
			ExmFLAT_Y_UOM(new) = 1;
	} else {				/* XmAUTOMATIC */

		(void)CheckThisRsc(
			new, XmNgridWidth, args, num_args,
			ExmFLAT_X_UOM(new), 0, &IBPART(new).calc_x_uom);

		(void)CheckThisRsc(
			new, XmNgridHeight, args, num_args,
			ExmFLAT_Y_UOM(new), 0, &IBPART(new).calc_y_uom);
	}

	/* Check the resources that are settable... */

			/* Should we check depth? */
	if (SHOW_BUSY_LABEL(new) &&
	    (CDIFF(background_pixel) || PDIFF(foreground))) {

			/* redisplay should already set in Primitive or Core */
		XmDestroyPixmap(XtScreen(new), IBPART(new).label_insens);
		IBPART(new).label_insens = XmGetPixmapByDepth(
						XtScreen(new), "50_foreground",
		/* foreground */		PPART(new).foreground,
		/* background */		CPART(new).background_pixel,
		/* depth */			CPART(new).depth);
	}

	if (FDIFF(cols) || FDIFF(rows)) {

		FPART(new).relayout_hint = True;
	}

		/* Set relayout_hint to True, so that ItemSetValues()
		 * will re-calculate Dimension if XmNdimensionPolicy is
		 * XmAUTOMATIC...
		 *
		 * Set relayout_hint to True will force item_touched to
		 * be True, so that, AnalyzeItems() will re-calculate
		 * Position if XmNdimensionPolicy is XmAUTOMATIC...
		 */

	if (IDIFF(vpad) && (IBPART(new).label_pos == XmBOTTOM_LABEL ||
			    IBPART(new).label_pos == XmTOP_LABEL)) {

		FPART(new).relayout_hint = True;
	}

	if (IDIFF(hpad) && IBPART(new).label_pos != XmBOTTOM_LABEL &&
			   IBPART(new).label_pos != XmTOP_LABEL) {

		FPART(new).relayout_hint = True;
	}

	if (IBDIFF(orientation)) {

		FPART(new).relayout_hint = True;
	}

	/* ASSUME users know what they are doing if (IBDIFF(use_obj_data))
	 * usually it means that they supply a new item list and a new field
	 * list OR items_touched is True... */


		/* must check num_labels before label_pos */
	if (IBDIFF(num_labels)) {

		MallocMaxColWidth(new, current);

		if (IS_LONG_LABEL(new))
			FPART(new).relayout_hint = True;
	}

	if (IBDIFF(label_pos)) {
		Boolean		changed = True;

		if (IS_LONG_LABEL(new) && !IBPART(new).num_labels) {

			if (IS_LONG_LABEL(current)) {

				IBPART(new).label_pos = XmBOTTOM_LABEL;
				XtWarning("XmNlabelCount can't be\
 zero, when XmNlabelPosition is *_LABELS, fallback to XmBOTTOM_LABEL");
			} else {

				changed = False;
				IBPART(new).label_pos =
						IBPART(current).label_pos;
				XtWarning("XmNlabelCount can't be\
 zero, when XmNlabelPosition is *_LABELS, XmNlabelPosition is not changed");
				}
		}

		if (changed)
			FPART(new).relayout_hint = True;
	}

	if (IBDIFF(col_spacing) || IBDIFF(row_spacing)) {

		FPART(new).relayout_hint = True;
	}

	if (CheckThisRsc(
		new, XmNmaxItemHeight, args, num_args,
		IBPART(new).max_item_hi, 0, &IBPART(new).calc_max_hi) ||
	    CheckThisRsc(
		new, XmNmaxItemWidth, args, num_args,
		IBPART(new).max_item_wd, 0, &IBPART(new).calc_max_wd)) {

		if (IBPART(new).dim_policy == XmAUTOMATIC)
			FPART(new).relayout_hint = True;
		/* otherwise, just record keeping... */
	}

	/* Check whether there is a need for resetting... */

	if (FPART(new).relayout_hint)
		FPART(new).items_touched = True;

	if (IBPART(new).dim_policy == XmAUTOMATIC && FPART(new).items_touched) {

		if (IBPART(new).calc_max_wd)
			IBPART(new).max_item_wd = 0;

		if (IBPART(new).calc_max_hi)
			IBPART(new).max_item_hi = 0;
	}

	return(redisplay);

#undef CDIFF
#undef PDIFF
#undef FDIFF
#undef IDIFF
#undef IBDIFF
} /* end of SetValues */

/****************************procedure*header*****************************
 * AnalyzeItems -
 */
static void
AnalyzeItems(Widget w, ArgList args, Cardinal * num_args)
{
		/* Determine (x, y) position for each item only when
		 * XmNdimensionPolicy is XmAUTOMATIC, note that
		 * this method will be called only when `items_touched'.
		 *
		 * Can't use ExmFlatGraphInfoList in FGraph, because
		 * if I'm here, either the info list is not created
		 * yet (initialize) or the info list is going to be
		 * re-malloc'd (set_values). */
	if (IBPART(w).dim_policy == XmAUTOMATIC && FPART(w).num_items) {

		register int	I, i;
		Arg		args[2];
		Cardinal	row, col;
		ExmFLAT_ALLOC_ITEM(w, ExmFlatItem, item);

		if (IBPART(w).calc_max_wd)
			IBPART(w).max_item_wd += 2 * IBPART(w).col_spacing;

		if (IBPART(w).calc_max_hi)
			IBPART(w).max_item_hi += 2 * IBPART(w).row_spacing;

		if (IBPART(w).calc_x_uom)
			ExmFLAT_X_UOM(w) = IBPART(w).max_item_wd;

		if (IBPART(w).calc_y_uom)
			ExmFLAT_Y_UOM(w) = IBPART(w).max_item_hi;

		for (I = 0, i = 0; I < FPART(w).num_items; I++) {

			ExmFlatExpandItem(w, I, item);

				/* Skip the unmanaged ones */
			if (!FITEM(item).managed ||
			    !FITEM(item).mapped_when_managed)
				continue;

			row = i / (Cardinal)FPART(w).cols;
			col = i % (Cardinal)FPART(w).cols;
			i++;

			if (IBPART(w).orientation == XmVERTICAL) {

				Cardinal	tmp = row;

				row = col;
				col = tmp;
			}

			FITEM(item).x = IBPART(w).max_item_wd * col +
							IBPART(w).col_spacing;
			FITEM(item).y = IBPART(w).max_item_hi * row +
							IBPART(w).row_spacing;

			ExmFlatSyncItem(w, item);
		}

		ExmFLAT_FREE_ITEM(item);
	}
} /* end of AnalyzeItems */

/****************************procedure*header*****************************
 * DefaultItemLabelsCopy -
 */
static void
DefaultItemLabelsCopy(Widget w, ExmFlatItem item)
{
	if (IBITEM(item).labels && IBPART(w).num_labels) {

		register int	i;
		_XmString *	stuff;

		stuff = IBITEM(item).labels;
		IBITEM(item).labels = (_XmString *)XtMalloc(sizeof(_XmString) *
						IBPART(w).num_labels);
		for (i = 0; i < (int)IBPART(w).num_labels; i++)
			IBITEM(item).labels[i] = _XmStringCopy(stuff[i]);
	}
	else {

		/* IBPART(w).num_labels = 0; */
		IBITEM(item).labels = NULL;
	}
} /* end of DefaultItemLabelsCopy */

/****************************procedure*header*****************************
 * DefaultItemLabelsDestroy -
 */
static void
DefaultItemLabelsDestroy(Widget w, ExmFlatItem item)
{
	register int	i;

	for (i = 0; i < (int)IBPART(w).num_labels; i++)
		_XmStringFree(IBITEM(item).labels[i]);

	XtFree((XtPointer)IBITEM(item).labels);
	IBITEM(item).labels = NULL;
} /* end of DefaultItemLabelsDestroy */

/****************************procedure*header*****************************
 * DefaultItemObjectDataCopy -
 */
static void
DefaultItemObjectDataCopy(Widget w, ExmFlatItem item)
{
	if (IITEM(item).object_data) {

		XtPointer	stuff = IITEM(item).object_data;
		int		the_size = sizeof(ExmFIconboxGlyphData);

		IITEM(item).object_data = (XtPointer)XtMalloc(the_size);
		memcpy(stuff, IITEM(item).object_data, the_size);
	}
} /* end of DefaultItemObjectDataCopy */

/****************************procedure*header*****************************
 * DefaultItemObjectDataDestroy -
 */
static void
DefaultItemObjectDataDestroy(Widget w, ExmFlatItem item)
{
	if (IITEM(item).object_data) {

		XtFree((XtPointer)IITEM(item).object_data);
		IITEM(item).object_data = NULL;
	}
} /* end of DefaultItemObjectDataDestroy */

/****************************procedure*header*****************************
 * DefaultItemInitialize
 */
static void
DefaultItemInitialize(Widget w, ExmFlatItem request, ExmFlatItem new,
			ArgList args, Cardinal * num_args)
{
	DefaultItemLabelsCopy(w, new);
	DefaultItemObjectDataCopy(w, new);
} /* end of DefaultItemInitialize */

/****************************procedure*header*****************************
 * DefaultItemSetValues
 */
static Boolean
DefaultItemSetValues(Widget w, ExmFlatItem current, ExmFlatItem request,
			ExmFlatItem new, ArgList args, Cardinal * num_args)
{
#define IDIFF(field)	(IITEM(new).field != IITEM(current).field)
#define IBDIFF(field)	(IBITEM(new).field != IBITEM(current).field)

	Boolean		redisplay = False;

	if (CheckThisItemRsc(w, IBDIFF(labels), XmNlabels, args, num_args)) {

		DefaultItemLabelsCopy(w, new);

		if (IBITEM(current).labels) {

			DefaultItemLabelsDestroy(w, current);
		}

		if (IS_LONG_LABEL(new)) {

			FPART(w).relayout_hint = True;
			redisplay = True;
		}
	}

	if (CheckThisItemRsc(w, IDIFF(object_data), XmNobjectData,
							args, num_args)) {

		DefaultItemObjectDataCopy(w, new);

		if (IITEM(current).object_data) {

			DefaultItemObjectDataDestroy(w, current);
		}

		if (IBPART(w).use_obj_data) {

			FPART(w).relayout_hint = True;
			redisplay = True;
		}
	}

	return(redisplay);

#undef IDIFF
#undef IBDIFF
} /* end of DefaultItemSetValues */

/****************************procedure*header*****************************
 * AddFocusSpace -
 */
static Boolean
AddFocusSpace(Widget w, Dimension * glyph_wd, Dimension * glyph_hi,
				Dimension * label_wd, Dimension * label_hi)
{
	Boolean		did_it;

	if (IBPART(w).label_pos != XmBOTTOM_LABEL &&
	    IBPART(w).label_pos != XmTOP_LABEL) {

		did_it = False;
	} else {

		did_it = True;
		if (*glyph_wd == 0 && *glyph_hi == 0) {	/* no glyph */

			*label_wd += 2 * PPART(w).highlight_thickness;
			*label_hi += 2 * PPART(w).highlight_thickness;
		} else {				/* had glyph */

			*glyph_wd += 2 * PPART(w).highlight_thickness;
			*glyph_hi += 2 * PPART(w).highlight_thickness;
		}
	}

	return did_it;

} /* end of AddFocusSpace */

/****************************procedure*header*****************************
 * CalcItemDimension -
 */
static void
CalcItemDimension(Widget w, ExmFlatItem item)
{
	if (IBPART(w).dim_policy != XmAUTOMATIC || !FCLASS(w).item_dimensions)
		return;

	if (FITEM(item).managed && FITEM(item).mapped_when_managed) {

		ExmFlatItemDimensions(w, item,
				&FITEM(item).width, &FITEM(item).height);

		if (IBPART(w).calc_max_wd &&
		    IBPART(w).max_item_wd < FITEM(item).width)
			IBPART(w).max_item_wd = FITEM(item).width;

		if (IBPART(w).calc_max_hi &&
		    IBPART(w).max_item_hi < FITEM(item).height)
			IBPART(w).max_item_hi = FITEM(item).height;
	} else {

		FITEM(item).width  = (Dimension)1;
		FITEM(item).height = (Dimension)1;
	}
} /* end of CalcItemDimension */

/****************************procedure*header*****************************
 * CalcGlyphDimension -
 */
static void
CalcGlyphDimension(Widget w, ExmFlatItem item, Dimension * wd, Dimension * hi)
{
	if (!VALID_PIXMAP(w, item)) {

		*wd = *hi = 0;
	} else {
		*wd = GLYPH_DATA(w, item, width);
		*hi = GLYPH_DATA(w, item, height);

		/* add spacing for `selected' visual (shadow_thickness) */
		*wd += 2 * PPART(w).shadow_thickness;
		*hi += 2 * PPART(w).shadow_thickness;
	}
} /* end of CalcGlyphDimension */

/****************************procedure*header*****************************
 * CalcShortLabelDimension -
 */
static void
CalcLongLabelDimension(Widget w, ExmFlatItem item,
						Dimension * wd, Dimension * hi)
{
	Dimension	WD, HI;
	register int	i;

	*wd = *hi = 0;

	for (i = 0; i < (int)IBPART(w).num_labels; i++) {
		CalcShortLabelDimension(
				w, item, IBITEM(item).labels[i], &WD, &HI);

		IBPART(w).max_col_wd[i] = MAX(WD,IBPART(w).max_col_wd[i]);
		*wd += WD;
		*hi = MAX(HI, *hi);
	}

	if (*wd)
		*wd += (IBPART(w).num_labels - 1) * IPART(w).hpad;
} /* end of CalcLongLabelDimension */

/****************************procedure*header*****************************
 * CalcShortLabelDimension -
 */
static void
CalcShortLabelDimension(Widget w, ExmFlatItem item, _XmString this_label,
						Dimension * wd, Dimension * hi)
{
	if (!this_label) {

		*wd = *hi = 0;
	} else {

		_XmStringExtent(FPART(w).font, this_label, wd, hi);

		/* add spacing for `selected' visual (shadow_thickness) */
		*wd += 2 * PPART(w).shadow_thickness;
		*hi += 2 * PPART(w).shadow_thickness;
	}
} /* end of CalcShortLabelDimension */

/****************************procedure*header*****************************
 * CalcLabelDimension -
 */
static void
CalcLabelDimension(Widget w, ExmFlatItem item, Dimension * wd, Dimension * hi)
{
	if (IS_LONG_LABEL(w))
		CalcLongLabelDimension(w, item, wd, hi);
	else
		CalcShortLabelDimension(w, item, FITEM(item).label, wd, hi);
} /* end of CalcLabelDimension */

/****************************procedure*header*****************************
 * CheckThisRsc -
 */
static Boolean
CheckThisRsc(Widget w, String rsc_name, ArgList args, Cardinal * num_args,
		Dimension field_value, Dimension this_value, Boolean * flag)
{
	register int	i;

	for (i = 0; i < *num_args; i++) {

		if (!strcmp(args[i].name, rsc_name)) {
			*flag = (field_value == this_value);
			break;
		}
	}

	return i != *num_args;
} /* end of CheckThisRsc */

/****************************procedure*header*****************************
 * CheckThisItemRsc -
 */
static Boolean
CheckThisItemRsc(Widget w, Boolean rsc_changed, String rsc_name,
					ArgList args, Cardinal * num_args)
{
	if (!rsc_changed) {

		register int	i;

			/* `labels' and `object_data' contains *labels*
			 * data and glyph data.
			 *
			 * Data are shared between FLAT and
			 * application (i.e., data are not cached).
			 * Thus FLAT can't detect the changes inside.
			 *
			 * So just force a redisplay if XmNlabels or
			 * XmNobjectData is part of the `arg' list.
			 */
		for (i = 0; i < *num_args; i++) {

			if (!strcmp(args[i].name, rsc_name)) {

				rsc_changed = True;
				break;
			}
		}
	}

	return(rsc_changed);
} /* end of CheckThisItemRsc */

/****************************procedure*header*****************************
 * DrawItemProc -
 */
static void
DrawItemProc(Widget w, ExmFlatItem item, ExmFlatDrawInfo * di)
{
#define DPY		XtDisplay(w)

	Boolean		added_focus_space;
	Boolean		is_busy;
	Boolean		valid_pixmap;
	Pixmap		pixmap;
	Position	focus_x, focus_y;
	Position	glyph_x, glyph_y;
	Position	label_x, label_y;
	Dimension	focus_wd, focus_hi;
	Dimension	label_wd, label_hi;
	Dimension	glyph_wd, glyph_hi;
	XGCValues	gc_val;
	int		delta;

	if (DEBUG(w))
		XDrawRectangle(
			DPY, di->drawable, di->gc,
			di->x, di->y, di->width, di->height);

	is_busy = !(FITEM(item).sensitive && XtIsSensitive(w));
	CalcLabelDimension(w, item, &label_wd, &label_hi);
	CalcGlyphDimension(w, item, &glyph_wd, &glyph_hi);

#if 0
	if (DEBUG(w))
		printf("%s (%d), (x, y, wd, hi) = (%d, %d, %d, %d)\n", __FILE__,
			__LINE__, di->x, di->y, di->width, di->height);
#endif

	focus_x  = di->x;
	focus_y  = di->y;
	focus_wd = di->width;
	focus_hi = di->height;

	if (label_wd == 0 && label_hi == 0 &&	/* no label */
	    glyph_wd == 0 && glyph_hi == 0) {	/* no glyph */

		goto draw_focus_visual;
	}

	added_focus_space = AddFocusSpace(
				w, &glyph_wd, &glyph_hi, &label_wd, &label_hi);

	if (IBPART(w).label_pos == XmBOTTOM_LABEL ||
	    IBPART(w).label_pos == XmTOP_LABEL) {

		glyph_x = ((delta = di->width - glyph_wd) > 1) ?
					di->x + (delta / 2) : di->x; 
		glyph_y = (IBPART(w).label_pos == XmBOTTOM_LABEL) ?
					di->y : di->y +label_hi + IPART(w).vpad;

		if (FITEM(item).label) {

			label_x = ((delta = di->width - label_wd) > 1) ?
					di->x + (delta / 2) : di->x; 
			label_y = (IBPART(w).label_pos == XmBOTTOM_LABEL) ?
					di->y +glyph_hi + IPART(w).vpad : di->y;
		}
	} else {	/* RIGHT/LEFT_LABEL(S) */

		glyph_x = (LABEL_ON_RIGHT(w)) ?
					di->x : di->x +label_wd + IPART(w).hpad;
		glyph_y = ((delta = di->height - glyph_hi) > 1) ? 
					di->y + (delta / 2) : di->y;

		if (!(label_wd == 0 && label_hi == 0)) {

			label_x = (LABEL_ON_RIGHT(w)) ?
					di->x +glyph_wd + IPART(w).hpad : di->x;
			label_y = ((delta = di->height - label_hi) > 1) ? 
					di->y + (delta / 2) : di->y;
		}
	}
	/************	Draw the glyph		************/

	if (glyph_wd == 0 && glyph_hi == 0)
		goto draw_label;

		/* focus visual for BOTTOM_LABEL and TOP_LABEL is
		 * on glyph..., same as in dtm */
	if (added_focus_space && PPART(w).highlight_thickness) {

		if (di->item_has_focus) {

			focus_x		= glyph_x;
			focus_y		= glyph_y;
			focus_wd	= glyph_wd;
			focus_hi	= glyph_hi;
		}
	
		glyph_x  += PPART(w).highlight_thickness;
		glyph_y  += PPART(w).highlight_thickness;
		glyph_wd -= PPART(w).highlight_thickness * 2;
		glyph_hi -= PPART(w).highlight_thickness * 2;
	}

	if (PPART(w).shadow_thickness) {

		if (FITEM(item).selected)
			_XmDrawShadows (DPY, di->drawable,
			    PPART(w).bottom_shadow_GC, PPART(w).top_shadow_GC,
			    glyph_x, glyph_y, glyph_wd, glyph_hi, 
			    PPART(w).shadow_thickness, XmSHADOW_OUT);
	
		glyph_x  += PPART(w).shadow_thickness;
		glyph_y  += PPART(w).shadow_thickness;
		glyph_wd -= PPART(w).shadow_thickness * 2;
		glyph_hi -= PPART(w).shadow_thickness * 2;
	}

	pixmap = GLYPH_DATA(w, item, pixmap);

	XSetClipOrigin(DPY, di->gc, glyph_x, glyph_y);
	XSetClipMask(DPY, di->gc, GLYPH_DATA(w, item, mask));

	if (GLYPH_DATA(w, item, depth) == 1)
		XCopyPlane(DPY, pixmap, di->drawable, di->gc,
			0, 0, glyph_wd, glyph_hi, glyph_x, glyph_y, 1);
	else
		XCopyArea(DPY, pixmap, di->drawable, di->gc,
			0, 0, glyph_wd, glyph_hi, glyph_x, glyph_y);

	if (is_busy) {

		XSetForeground(DPY, di->gc, CPART(w).background_pixel);
		XSetStipple(DPY, di->gc, FPART(w).insens_pixmap);
		XSetFillStyle(DPY, di->gc, FillStippled);

		XFillRectangle(
			DPY, di->drawable, di->gc,
			glyph_x, glyph_y, glyph_wd, glyph_hi);

		XSetForeground(DPY, di->gc, PPART(w).foreground);
		XSetFillStyle(DPY, di->gc, FillSolid);
	}

	XSetClipMask(DPY, di->gc, None);

draw_label:
		/************	Draw the label		************/
	if (label_wd == 0 && label_hi == 0)
		goto draw_focus_visual;

	if (PPART(w).highlight_thickness && added_focus_space &&
	    glyph_wd == 0 && glyph_hi == 0) {	/* no glyph */

		label_x  += PPART(w).highlight_thickness;
		label_y  += PPART(w).highlight_thickness;
		label_wd -= PPART(w).highlight_thickness * 2;
		label_hi -= PPART(w).highlight_thickness * 2;
	}

	if (PPART(w).shadow_thickness) {

		if (FITEM(item).selected)
			_XmDrawShadows(DPY, di->drawable,
			    PPART(w).bottom_shadow_GC, PPART(w).top_shadow_GC,
			    label_x, label_y, label_wd, label_hi, 
			    PPART(w).shadow_thickness, XmSHADOW_OUT);
	
		label_x  += PPART(w).shadow_thickness;
		label_y  += PPART(w).shadow_thickness;
		label_wd -= PPART(w).shadow_thickness * 2;
		label_hi -= PPART(w).shadow_thickness * 2;
	}

	if (is_busy && SHOW_BUSY_LABEL(w)) {

		XSetTile(DPY, di->gc, IBPART(w).label_insens);
		XSetFillStyle(DPY, di->gc, FillTiled);
	} else {

		XGetGCValues(DPY, di->gc, GCForeground | GCBackground, &gc_val);
		XSetForeground(DPY, di->gc, gc_val.background);
		XFillRectangle(DPY, di->drawable, di->gc,
					label_x, label_y, label_wd, label_hi);
		XSetForeground(DPY, di->gc, gc_val.foreground);
	}

	if (!IS_LONG_LABEL(w)) {

		_XmStringDraw(
			DPY, di->drawable, FPART(w).font, FITEM(item).label,
			di->gc, label_x, label_y, label_wd,
			XmALIGNMENT_CENTER, XmSTRING_DIRECTION_L_TO_R, NULL);
	} else {
		register int	i;

		for (i = 0; i < (int)IBPART(w).num_labels; i++) {

			_XmStringDraw(
				DPY, di->drawable, FPART(w).font,
				IBITEM(item).labels[i],
				di->gc, label_x, label_y,
				IBPART(w).max_col_wd[i],
				XmALIGNMENT_BEGINNING,
				XmSTRING_DIRECTION_L_TO_R, NULL);

			label_x += IBPART(w).max_col_wd[i] + IPART(w).hpad;
		}
	}

	if (is_busy && SHOW_BUSY_LABEL(w))
		XSetFillStyle(DPY, di->gc, FillSolid);

draw_focus_visual:
	if (di->item_has_focus && PPART(w).highlight_thickness) {

		_XmDrawSimpleHighlight(
			DPY, di->drawable,
			PPART(w).highlight_GC, 
			focus_x, focus_y, focus_wd, focus_hi,
			PPART(w).highlight_thickness
		);
	}
} /* end of DrawItemProc */

/****************************procedure*header*****************************
 * ItemDimensions - this routine determines the size of a single sub-object
 */
static void
ItemDimensions(Widget w, ExmFlatItem item,
		register Dimension * ret_wd,
		register Dimension * ret_hi)
{
	Boolean		added_focus_space;
	Dimension	label_wd, label_hi;
	Dimension	glyph_wd, glyph_hi;

	if (IBPART(w).dim_policy == XmAPPLICATION_DEFINED) {

		Arg	args[2];

		XtSetArg(args[0], XmNwidth, ret_wd);
		XtSetArg(args[1], XmNheight, ret_hi);
		ExmFlatGetValues(w, FITEM(item).item_index, args, 2);

		goto final_check;	/* sorry about goto */
	}

	CalcLabelDimension(w, item, &label_wd, &label_hi);
	CalcGlyphDimension(w, item, &glyph_wd, &glyph_hi);

	if (label_wd == 0 && label_hi == 0 &&	/* no label */
	    glyph_wd == 0 && glyph_hi == 0)	/* no glyph */
		goto final_check;

	added_focus_space = AddFocusSpace(
				w, &glyph_wd, &glyph_hi, &label_wd, &label_hi);

	switch(IBPART(w).label_pos) {

		case XmBOTTOM_LABEL:	/* FALL THROUGH */
		case XmTOP_LABEL:
			*ret_wd = MAX(label_wd, glyph_wd);
			*ret_hi = label_hi + glyph_hi + IPART(w).vpad;
			break;
		case XmLEFT_LABEL:	/* FALL THROUGH */
		case XmRIGHT_LABEL:	/* FALL THROUGH */
		case XmLEFT_LABELS:	/* FALL THROUGH */
		case XmRIGHT_LABELS:
			*ret_wd = label_wd + glyph_wd + IPART(w).hpad;
			*ret_hi = MAX(label_hi, glyph_hi);
			break;
	}

	if (!added_focus_space) {
		*ret_wd += 2 * PPART(w).highlight_thickness;
		*ret_hi += 2 * PPART(w).highlight_thickness;
	}

final_check:
		/* return spacing for `focus' visual (highlight_thick + 1)
		 * if the value is zero */
	if (*ret_wd == 0)
		*ret_wd = 2 * (PPART(w).highlight_thickness + 1);

	if (*ret_hi == 0)
		*ret_hi = 2 * (PPART(w).highlight_thickness + 1);
} /* end of ItemDimensions() */

/****************************procedure*header*****************************
 * ItemInitialize -
 */
static void
ItemInitialize(Widget w, ExmFlatItem request, ExmFlatItem new,
		ArgList args, Cardinal * num_args)
{
	CalcItemDimension(w, new);
} /* end of ItemInitialize */

/****************************procedure*header*****************************
 * ItemSetValues -
 */
static Boolean
ItemSetValues(	Widget	      w, 	/* ExmFlat widget container id	*/
		ExmFlatItem   current, 	/* expanded current item	*/
		ExmFlatItem   request, 	/* expanded requested item	*/
		ExmFlatItem   new, 	/* expanded new item		*/
		ArgList	      args, 
		Cardinal *    num_args)
{
#define FDIFF(field)	(FITEM(new).field != FITEM(current).field)
#define IDIFF(field)	(IITEM(new).field != IITEM(current).field)
#define IBDIFF(field)	(IBITEM(new).field != IBITEM(current).field)

	Boolean		redisplay = False;
	Boolean		call_calc_dim = False;


	if (IBPART(w).use_obj_data) {

		if (CheckThisItemRsc(w, IDIFF(object_data), XmNobjectData,
								args, num_args))
			redisplay = call_calc_dim = True;
	} else {

			/* I don't monitor the width/height changes */
		if (IBDIFF(pixmap) || IBDIFF(mask))
			redisplay = call_calc_dim = True;
	}

	if (IS_LONG_LABEL(w) &&
	    CheckThisItemRsc(w, IBDIFF(labels), XmNlabels, args, num_args)) {

		redisplay = call_calc_dim = True;
	}

		/* I need to recalculate this item dimension if
		 * call_calc_dim is True (because glyph_data changes)
		 * or items_touched (maybe label_pos is changed) */
	if (call_calc_dim == True || FPART(w).items_touched) {

		CalcItemDimension(w, new);
	}

	return(redisplay);

#undef FDIFF
#undef IDIFF
} /* end of ItemSetValues() */

/****************************action*procedures****************************
 *
 * Action Procedures
 */

/****************************public*procedures****************************
 *
 * Public Procedures
 */

/****************************procedure*header*****************************
 * ExmXmStringCreate - create a _XmString, the purpose of this routine
 *	is to protect an application from calling _Xm function.
 *
 * Note that should move to Flat.c/Flat.h if libMDtI includes FIconbox.c
 *
 * Note that `_XmString' is an opaque type and is defined in Xm.h
 *		(isn't it great!).
 */
extern _XmString
ExmXmStringCreate(String string)
{
	XmString	xm_string;
	_XmString	_xm_string;

	xm_string = XmStringCreateLtoR(string, XmFONTLIST_DEFAULT_TAG);
	_xm_string = _XmStringCreate(xm_string);
	XmStringFree(xm_string);
	return((XtPointer)_xm_string);
} /* end of ExmXmStringCreate */

/****************************procedure*header*****************************
 * ExmXmStringFree - free a _XmString, the purpose of this routine
 *	is to protect an application from calling _Xm function.
 *
 * Note that should move to Flat.c/Flat.h if libMDtI includes FIconbox.c
 *
 * Note that `_XmString' is an opaque type and is defined in Xm.h
 *		(isn't it great!).
 */
extern void
ExmXmStringFree(_XmString string)
{
	_XmStringFree(string);
} /* end of ExmXmStringFree */
