#ifndef	NOIDENT
#ident	"@(#)flat:Flat.c	1.75"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This file contains the source code for the flat widget class.
 *	The flat widget class is not a class that's intended for
 *	instantiation; rather, it serves as a managing class for its
 *	instantiated subclasses.
 *
 ******************************file*header********************************
 */

						/* #includes go here	*/

#include <stdio.h>
#include <memory.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/FlatP.h>

#define ClassName Flat
#include <Xol/NameDefs.h>

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures 
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static void	CheckFont OL_ARGS((Widget, FlatItem));
static void	GetGCs OL_ARGS((Widget, FlatItem));
static void	ItemsTouched OL_ARGS((Widget));
static Boolean	SetAccAndMnem OL_ARGS((Widget, FlatItem, FlatItem));
static Boolean	SetFocusToAnyItem OL_ARGS((Widget, Time *));
static void	SetupRequiredResources OL_ARGS((WidgetClass, FlatClassPart *,
						FlatClassPart *));

					/* class procedures		*/

static void	AcceleratorTextDestroy OL_ARGS((Widget, Cardinal, String,
						XtPointer));
static Boolean	AcceleratorTextPredicate OL_ARGS((Widget, Cardinal, String));
static Boolean	AcceptFocus OL_ARGS((Widget, Time *));
static Boolean	ActivateWidget OL_ARGS((Widget, OlVirtualName, XtPointer));
static void	AnalyzeItems OL_ARGS((Widget, ArgList, Cardinal *));
static void	ClassInitialize OL_NO_ARGS();
static void	ClassPartInitialize OL_ARGS((WidgetClass));
static void	DefaultItemInitialize OL_ARGS((Widget, FlatItem,
					FlatItem, ArgList, Cardinal *));
static Boolean	DefaultItemSetValues OL_ARGS((Widget, FlatItem, FlatItem,
				FlatItem, ArgList, Cardinal *));
static void	Destroy OL_ARGS((Widget));
static void	HighlightHandler OL_ARGS((Widget, OlDefine));
static void	Initialize OL_ARGS((Widget, Widget, ArgList, Cardinal *));
static Boolean	ItemAcceptFocus OL_ARGS((Widget, FlatItem, Time *));
static void	ItemHighlight OL_ARGS((Widget, FlatItem, OlDefine));
static void	ItemInitialize OL_ARGS((Widget, FlatItem, FlatItem,
						ArgList, Cardinal *));
static Boolean	ItemSetValues OL_ARGS((Widget, FlatItem, FlatItem, FlatItem,
					ArgList, Cardinal *));
static Boolean	SetValues OL_ARGS((Widget, Widget, Widget,ArgList,Cardinal*));
static void	TransparentProc OL_ARGS((Widget, Pixel, Pixmap));
static Widget	TraversalHandler OL_ARGS((Widget,Widget,OlVirtualName,Time));

					/* action procedures		*/

/* There are no action procedures */

					/* public procedures		*/

void	OlFlatInheritAll OL_ARGS((WidgetClass));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#ifdef __STDC__
#define SIZE_T	size_t
#else
#define SIZE_T	int
#endif

			/* Declare a structre that describes 
			 * characteristics of inheritable class fields.
			 * Each function denoted in this structure should
			 * have a corresponding XtInherit... token.	*/

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
#define OFFSET(f)	XtOffsetOf(FlatClassRec, flat_class.f)

static OLconst ClassFields
class_fields[] = {
    { OFFSET(draw_item),		NULL },
    { OFFSET(change_managed),		"OlFlatChangeManagedProc" },
    { OFFSET(geometry_handler),		NULL },
    { OFFSET(get_item_geometry),	"OlFlatGetItemGeometryProc" },
    { OFFSET(get_index),		"OlFlatGetIndexFunc" },
    { OFFSET(item_accept_focus),	NULL },
    { OFFSET(item_activate),		NULL },
    { OFFSET(item_dimensions),		"OlFlatItemDimensionsProc" },
    { OFFSET(item_highlight),		"OlFlatItemHighlightProc" },
    { OFFSET(item_location_cursor_dimensions), NULL },
    { OFFSET(item_set_values_almost),	NULL },
    { OFFSET(item_resize),		NULL },
    { OFFSET(refresh_item),		NULL },
    { OFFSET(traverse_items),		NULL }
};

#define MAINTAIN_SAME_VALUES(i)					\
	i->flat.x			= (Position) OL_IGNORE;	\
	i->flat.y			= (Position) OL_IGNORE;	\
	i->flat.width			= (Dimension) OL_IGNORE;\
	i->flat.height			= (Dimension) OL_IGNORE;\
	i->flat.mnemonic		= (OlMnemonic)0;	\
	i->flat.sensitive		= (Boolean)True;	\
	i->flat.accelerator		= (String)NULL;		\
	i->flat.accelerator_text	= (String)NULL;		\
	i->flat.managed			= (Boolean)True;	\
	i->flat.mapped_when_managed	= (Boolean)True

					/* Define some handy macros	*/

#define FPART(w)	(((FlatWidget)(w))->flat)
#define PWIDGET(w)	(((PrimitiveWidget)w)->primitive)
#define PCLASS(wc, f)	(((PrimitiveWidgetClass)wc)->primitive_class.f)
#define CALL_ITEM_HIGHLIGHT(w, i, t)		\
	if (OL_FLATCLASS(w).item_highlight)	\
		(*OL_FLATCLASS(w).item_highlight)(w, i, t)

#define CALL_TRANSPARENT_PROC(w)\
	{\
		WidgetClass wc = XtClass(w);\
		if (PCLASS(wc, transparent_proc) != (OlTransparentProc)NULL)\
		{\
			(*PCLASS(wc, transparent_proc))(w,\
				XtParent(w)->core.background_pixel,\
				XtParent(w)->core.background_pixmap);\
		}\
	}

#define	LOCAL_SIZE	50

#ifdef DONT_RM_HACK
#define INDEX_IS_CACHED(d)	((d) != (XtPointer)NULL)
#endif
#define INDEX_TO_DATA(i)	((XtPointer)((i)+1))
#define DATA_TO_INDEX(i)	((Cardinal)((Cardinal)(i)-1))

static OLconst Dimension	def_dimension = (Dimension)OL_IGNORE;
static OLconst Position		def_position = (Position)OL_IGNORE;

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

/* There are no translations or actions */

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

#undef OFFSET
#define OFFSET(field)	XtOffsetOf(FlatItemRec, flat.field)

static char	empty_string[] = "";	/* for required resources */

static XtResource
item_resources[] = {

	{ XtNx, XtCX, XtRPosition, sizeof(Position),
	  OFFSET(x), XtRPosition, (XtPointer) &def_position },

	{ XtNy, XtCY, XtRPosition, sizeof(Position),
	  OFFSET(y), XtRPosition, (XtPointer) &def_position },

	{ XtNwidth, XtCWidth, XtRDimension, sizeof(Dimension),
	  OFFSET(width), XtRDimension, (XtPointer)&def_dimension },

	{ XtNheight, XtCHeight, XtRDimension, sizeof(Dimension),
	  OFFSET(height), XtRDimension, (XtPointer)&def_dimension },

	{ XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
	  OFFSET(font), XtRString, OlDefaultFont },

	{ XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
	  OFFSET(foreground), XtRString, (XtPointer) XtDefaultForeground },

	{ XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel),
	  OFFSET(font_color), XtRString, (XtPointer) XtDefaultForeground },

	{ XtNlabel, XtCLabel, XtRString, sizeof(String),
	  OFFSET(label), XtRString, (XtPointer) NULL },

	{ XtNlabelImage, XtCLabelImage, XtRPointer, sizeof(XImage *),
	  OFFSET(label_image), XtRPointer, (XtPointer) NULL },

	{ XtNlabelJustify, XtCLabelJustify, XtROlDefine, sizeof(OlDefine),
	  OFFSET(label_justify), XtRImmediate, (XtPointer) OL_LEFT },

	{ XtNlabelTile, XtCLabelTile, XtRBoolean, sizeof(Boolean),
	  OFFSET(label_tile), XtRImmediate, (XtPointer) False },

	{ XtNinputFocusColor, XtCInputFocusColor, XtRPixel,
	  sizeof(Pixel), OFFSET(input_focus_color), XtRString,
	  (XtPointer) "Red" },

	{ XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
	  OFFSET(background_pixel), XtRString,
	  (XtPointer) XtDefaultBackground },

		/*
		 * The default value for backgroundPixmap should have
		 * been XtUnspecifiedPixmap, but since that doesn't work
		 * well in static shared library, the constant 2 is used.
		 * This assumes that XtUnspecifiedPixmap is 2, which is
		 * true for Intrinsic Rel 3.0.
		 */
	{ XtNbackgroundPixmap, XtCPixmap, XtRPixmap, sizeof(Pixmap),
	  OFFSET(background_pixmap), XtRImmediate, (XtPointer) 2},

	{ XtNtraversalOn, XtCTraversalOn, XtRBoolean, sizeof(Boolean),
	  OFFSET(traversal_on), XtRImmediate, (XtPointer)True },

	{ XtNuserData, XtCUserData, XtRPointer, sizeof(XtPointer),
	  OFFSET(user_data), XtRPointer, (XtPointer) NULL },

	{ XtNmanaged, XtCManaged, XtRBoolean, sizeof(Boolean),
	  OFFSET(managed), XtRImmediate, (XtPointer) True },

	{ XtNmappedWhenManaged, XtCMappedWhenManaged, XtRBoolean,
	  sizeof(Boolean), OFFSET(mapped_when_managed),
	  XtRImmediate, (XtPointer) True },

	{ XtNsensitive, XtCSensitive, XtRBoolean, sizeof(Boolean),
	  OFFSET(sensitive), XtRImmediate, (XtPointer) True },

	{ XtNaccelerator, XtCAccelerator, XtRString, sizeof(String),
	  OFFSET(accelerator), XtRString, (XtPointer) NULL },

	{ XtNacceleratorText, XtCAcceleratorText, XtRString, sizeof(String),
	  OFFSET(accelerator_text), XtRString, (XtPointer) empty_string },

	{ XtNmnemonic, XtCMnemonic, OlRChar, sizeof(char),
	  OFFSET(mnemonic), XtRImmediate, (XtPointer) '\0' }
};

				/* Define the required resources	*/

static OlFlatReqRsc
required_resources[] = {
	{ XtNacceleratorText, AcceleratorTextPredicate,
	  AcceleratorTextDestroy,
	}
};

			/* Define the resources for the container.	*/

#undef OFFSET
#define OFFSET(f)	XtOffsetOf(FlatRec,flat.f)

static XtResource
resources[] = {
				/* Define flat resources.
				 * Note, the order of XtNitems, XtNnumItems
				 * XtNitemFields and XtNnumItemFields
				 * is significant.			*/

	{ XtNnumItems, XtCNumItems, XtRCardinal, sizeof(Cardinal),
	  OFFSET(num_items), XtRImmediate, (XtPointer) OL_IGNORE },

	{ XtNnumItemFields, XtCNumItemFields, XtRCardinal, sizeof(Cardinal),
	  OFFSET(num_item_fields), XtRImmediate, (XtPointer) OL_IGNORE },

	{ XtNitemFields, XtCItemFields, OlRFlatItemFields, sizeof(String *),
	  OFFSET(item_fields), OlRFlatItemFields, (XtPointer) NULL },

	{ XtNitems, XtCItems, OlRFlatItems, sizeof(XtPointer),
	  OFFSET(items), OlRFlatItems, (XtPointer) NULL },

	{ XtNitemsTouched, XtCItemsTouched, XtRBoolean, sizeof(Boolean),
	  OFFSET(items_touched), XtRImmediate, (XtPointer) False }
};

				/* Define Resources for sub-objects	*/

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

FlatClassRec
flatClassRec = {
    {
	(WidgetClass)&primitiveClassRec,	/* superclass		*/
	"Flat",					/* class_name		*/
	sizeof(FlatRec),			/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	ClassPartInitialize,			/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	_OlFlatStateInitialize,			/* initialize		*/
	NULL,					/* initialize_hook	*/
	XtInheritRealize,			/* realize		*/
	NULL,					/* actions		*/
	0,					/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	TRUE,					/* compress_motion	*/
	TRUE,					/* compress_exposure	*/
	TRUE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	Destroy,				/* destroy		*/
	NULL,					/* resize		*/
	XtInheritExpose,			/* expose		*/
	_OlFlatStateSetValues,			/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	AcceptFocus,				/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_offsets	*/
	XtInheritTranslations,			/* tm_table		*/
	NULL,					/* query_geometry	*/
	NULL,					/* display_accelerator	*/
	NULL					/* extension		*/
    }, /* End of Core Class Part Initialization */
    {
        True,					/* focus_on_select	*/
	HighlightHandler,			/* highlight_handler	*/
	TraversalHandler,			/* traversal_handler	*/
	NULL,					/* register_focus	*/
	ActivateWidget,				/* activate		*/
	NULL,					/* event_procs		*/
	0,					/* num_event_procs	*/
	OlVersion,				/* version		*/
	NULL,					/* extension		*/
	{
		(_OlDynResourceList)NULL,	/* resources		*/
		(Cardinal)0			/* num_resources	*/
	},					/* dyn_data		*/
	TransparentProc				/* transparent_proc	*/
    },	/* End of Primitive Class Part Initialization */
    {
	(XtPointer)NULL,			/* extension		*/
	False,					/* transparent_bg	*/
	XtOffsetOf(FlatRec, default_item),	/* default_offset	*/
	sizeof(FlatItemRec),			/* rec_size		*/
	item_resources,				/* item_resources	*/
	XtNumber(item_resources),		/* num_item_resources	*/
	required_resources,			/* required_resources	*/
	XtNumber(required_resources),		/* num_required_resources*/
	NULL,					/* quarked_items	*/

		/* Container procedures	*/

	Initialize,				/* initialize		*/
	SetValues,				/* set_values		*/
	XtInheritFlatGeometryHandler,		/* geometry_handler	*/
	XtInheritFlatChangeManaged,		/* change_managed	*/
	XtInheritFlatGetItemGeometry,		/* get_item_geometry	*/
	XtInheritFlatGetIndex,			/* get_index		*/
	XtInheritFlatTraverseItems,		/* traverse_items	*/
	NULL,					/* refresh_item		*/

		/* Item procedures	*/

        DefaultItemInitialize,			/* default_initialize	*/
        DefaultItemSetValues,			/* default_set_values	*/
	AnalyzeItems,				/* analyze_items	*/
	XtInheritFlatDrawItem,			/* draw_item		*/
	ItemAcceptFocus,			/* item_accept_focus	*/
	(OlFlatItemActivateFunc)NULL,		/* item_activate	*/
	XtInheritFlatItemDimensions,		/* item_dimensions	*/
	(OlFlatItemGetValuesProc)NULL,		/* item_get_values	*/
	ItemHighlight,				/* item_highlight	*/
	ItemInitialize,				/* item_initialize	*/
	ItemSetValues,				/* item_set_values	*/
	NULL					/* item_resize		*/
    } /* End of Flat Class Part Initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass flatWidgetClass = (WidgetClass) &flatClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * CheckFont -
 ****************************procedure*header*****************************
 */
static void
CheckFont OLARGLIST((w, item))
	OLARG( Widget,		w)
	OLGRA( FlatItem,	item)
{
	if (item->flat.font == (XFontStruct *)NULL)
	{
		item->flat.font = OlFlatDefaultItem(w)->flat.font;

		OlVaDisplayWarningMsg(XtDisplay(w),
			OleNinvalidResource, OleTflatState,
			OleCOlToolkitWarning,
			OleMinvalidResource_flatState,
			XtName(w), OlWidgetToClassName(w), XtNfont,
			(unsigned)item->flat.item_index);
	}
} /* END OF CheckFont() */

/*
 *************************************************************************
 * AcceleratorTextDestroy - destroys the string used for the required
 * resource XtNacceleratorText.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
AcceleratorTextDestroy OLARGLIST((w, offset, name, addr))
	OLARG( Widget,		w)
	OLARG( Cardinal,	offset)
	OLARG( String,		name)
	OLGRA( XtPointer,	addr)
{
	if (*(String *)addr != (String)NULL)
	{
		XtFree((XtPointer) *(String*)addr);
	}
} /* END OF AcceleratorTextDestroy() */

/*
 *************************************************************************
 * AcceleratorTextPredicate - determines whether the widget should have
 * the flat's required resource facility manage the XtNacceleratorText
 * resource for each item.  This routine is only called when
 * XtNacceleratorText is not one of the application specified item fields.
 * So, if the application specified the XtNaccelerator resource as one of
 * item fields, we'll request that the XtNacceleratorText be a required
 * resource; otherwise, we don't want the XtNacceleratorText being managed
 * internally.
 ****************************procedure*header*****************************
 */
/* ARGSUSED1 */
static Boolean
AcceleratorTextPredicate OLARGLIST((w, offset, name))
	OLARG( Widget,		w)
	OLARG( Cardinal,	offset)		/* ignored	*/
	OLGRA( String,		name)		/* ignored	*/
{
	Cardinal	i;

			/* Scan the new item fields looking for
			 * XtNaccelerator.  If it's not found, then
			 * we'll need to manage the acceleratorText
			 * list internally via the required resource
			 * facility.					*/

	for (i=0; i < FPART(w).num_item_fields; ++i)
	{
		if (!strcmp(XtNaccelerator, FPART(w).item_fields[i]))
		{
			return(True);
		}
	}

	return(False);
} /* END OF AcceleratorTextPredicate() */

/*
 *************************************************************************
 * GetGCs - this routine gets the GCs for the flat exclusives
 * container.  If any of the GCs existed, they are deleted first.
 ****************************procedure*header*****************************
 */
static void
GetGCs OLARGLIST((w, item))
	OLARG( Widget,		w)
	OLGRA( FlatItem,	item)
{
	XtGCMask	mask;
	XGCValues	values;

			/* Get the normal drawing GCs	*/

	if (FPART(w).pAttrs != (OlgAttrs *) NULL)
	{
		OlgDestroyAttrs (FPART(w).pAttrs);
	}
	FPART(w).pAttrs = OlgCreateAttrs(
				XtScreen(w), PWIDGET(w).foreground,
				(OlgBG *)&(item->flat.background_pixel),
				False, OL_DEFAULT_POINT_SIZE);

				/* Get the GCs for the labels/images	*/

	if (FPART(w).label_gc != (GC) NULL)
	{
		XtReleaseGC(w, FPART(w).label_gc);
	}
	mask				= (GCForeground|GCBackground|GCFont|
						GCFillStyle|
						GCGraphicsExposures);
	values.foreground		= item->flat.font_color;
	values.background		= item->flat.background_pixel;
	values.font			= item->flat.font->fid;
	values.fill_style		= FillSolid;
	values.graphics_exposures	= (Bool)False;
	FPART(w).label_gc		= XtGetGC(w, mask, &values);

	if (FPART(w).inverse_gc != (GC) NULL)
	{
		XtReleaseGC(w, FPART(w).inverse_gc);
	}
	values.foreground		= item->flat.background_pixel;
	values.background		= item->flat.font_color;
	FPART(w).inverse_gc		= XtGetGC(w, mask, &values);

} /* END OF GetGCs()  */

/*
 *************************************************************************
 * SetFocusToAnyItem - sets focus to the first item willing to take it.
 * TRUE is returned if an item took focus, FALSE is returned otherwise.
 ****************************procedure*header*****************************
 */
static Boolean
SetFocusToAnyItem OLARGLIST((w, time))
	OLARG( Widget,	w)
	OLGRA( Time *,	time)
{
	Boolean		ret_val = False;

	if (OL_FLATCLASS(w).item_accept_focus)
	{
		Cardinal	i;
		OL_FLAT_ALLOC_ITEM(w, FlatItem, item);

		for (i = 0; i < FPART(w).num_items; ++i)
		{
			OlFlatExpandItem(w, i, item);

			if ((*OL_FLATCLASS(w).item_accept_focus)(w, item,
						time) == True)
			{
				ret_val = True;
				break;
			}
		}
		OL_FLAT_FREE_ITEM(item);
	}
	return(ret_val);
} /* END OF SetFocusToAnyItem() */

/*
 *************************************************************************
 * SetAccAndMnem - This routine registers a new mnemonic and/or
 * accelerator for a sub-object.  If one, previously exists, it's removed
 * first.  The routine returns a flag indicating that a refresh is
 * necessary.
 ****************************procedure*header*****************************
 */
static Boolean
SetAccAndMnem OLARGLIST((w, current, new))
	OLARG( Widget,		w)		/* flat widget id	*/
	OLARG( FlatItem,	current)	/* current item or NULL	*/
	OLGRA( FlatItem,	new)
{
	Boolean		redisplay	= False;
	Cardinal	i		= new->flat.item_index;
	XtPointer	data		= INDEX_TO_DATA(i);

	if (current != (FlatItem)NULL &&
	    current->flat.mnemonic != new->flat.mnemonic &&
	    current->flat.mnemonic != '\0')
	{
		redisplay = True;
		_OlRemoveMnemonic(w, data, False, current->flat.mnemonic);
	}

	if (new->flat.mnemonic != '\0'
			&&
	    (current == (FlatItem)NULL ||
		current->flat.mnemonic != new->flat.mnemonic))
	{
		redisplay = True;

		switch (_OlAddMnemonic(w, data, new->flat.mnemonic)) {
		case OL_SUCCESS:
			break;
		case OL_DUPLICATE_KEY:
			OlVaDisplayWarningMsg (
				XtDisplay(w),
				OleNflatIllegalMnemonic, OleTduplicateKey,
				OleCOlToolkitWarning,
				OleMflatIllegalMnemonic_duplicateKey,
				XtName(w),
				OlWidgetToClassName(w),
			        new->flat.mnemonic,
				(int)i
			);
			goto IllegalMnemonic;
		case OL_BAD_KEY:
			OlVaDisplayWarningMsg (
				XtDisplay(w),
				OleNflatIllegalMnemonic, OleTbadKey,
				OleCOlToolkitWarning,
				OleMflatIllegalMnemonic_badKey,
				XtName(w),
				OlWidgetToClassName(w),
			        new->flat.mnemonic,
				(int)i
			);
IllegalMnemonic:	new->flat.mnemonic = '\0';
			break;
		}
	}

			/* Unregister the old accelerator and free it's
			 * string (provided we're internally managing it).
			 */

	if (current != (FlatItem)NULL &&
	    current->flat.accelerator != new->flat.accelerator)
	{
			/* If we're managing the accelerator text
			 * internally, unregister it.
			 * NOTE: we don't have to free the old string
			 * here since the required resource destroy
			 * procedure will get called when this item
			 * is synched-up with the application data.	*/

		_OlRemoveAccelerator(w, data, False, current->flat.accelerator);
		redisplay = True;
	}

	if (new->flat.accelerator != (String)NULL
			&&
	    (current == (FlatItem)NULL ||
		(current->flat.accelerator != new->flat.accelerator)))
	{
		Boolean internal_list = AcceleratorTextPredicate(w,0,NULL);

		redisplay = True;

		switch (_OlAddAccelerator(w, data, new->flat.accelerator)) {

		case OL_SUCCESS:
		    if (internal_list)
		    {
			new->flat.accelerator_text =
			    _OlMakeAcceleratorText(w, new->flat.accelerator);
		    }
		    break;

		case OL_DUPLICATE_KEY:
			OlVaDisplayWarningMsg (
				XtDisplay(w),
				OleNflatIllegalAccelerator, OleTduplicateKey,
				OleCOlToolkitWarning,
				OleMflatIllegalAccelerator_duplicateKey,
				XtName(w),
				OlWidgetToClassName(w),
			        new->flat.accelerator,
				(int)i
			);
			goto IllegalAccelerator;
		case OL_BAD_KEY:
			OlVaDisplayWarningMsg (
				XtDisplay(w),
				OleNflatIllegalAccelerator, OleTbadKey,
				OleCOlToolkitWarning,
				OleMflatIllegalAccelerator_badKey,
				XtName(w),
				OlWidgetToClassName(w),
			        new->flat.accelerator,
				(int)i
			);
IllegalAccelerator:	new->flat.accelerator_text = (String)NULL;
			break;
		}
	}

	return(redisplay);
} /* END OF SetAccAndMnem() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * AcceptFocus - this routine checks to see if focus can be set to this
 * flat Widget.  This routine does not actually set the focus.  It instead
 * redirects the request to the sub-object procedure.
 ****************************procedure*header*****************************
 */
static Boolean
AcceptFocus OLARGLIST((w, time))
	OLARG( Widget,	w)
	OLGRA( Time *,	time)
{
	Boolean	ret_val = False;

	if (OlCanAcceptFocus(w, *time) == True)
	{
		Cardinal		i;

		if ((i = FPART(w).last_focus_item) !=
					(Cardinal)OL_NO_ITEM)
		{
			ret_val = OlFlatCallAcceptFocus(w, i, *time);
		}

		if (ret_val == False)
		{
			ret_val = SetFocusToAnyItem(w, time);
		}
	}
	return(ret_val);
} /* END OF AcceptFocus() */

/*
 *************************************************************************
 * ActivateWidget - this generic routine is used to activate sub-objects.
 * If the data field is non-NULL, then cast the data to a Cardinal since
 * it is the item index.  (This occurs when a sub-object is activated
 * via a mnemonic or an accelerator.)  If the data field is NULL, then
 * activate the focus item.
 * Once the object to be activated has been found, the subclass's
 * item activation procedure is called.
 ****************************procedure*header*****************************
 */
static Boolean
ActivateWidget OLARGLIST((w, type, data))
	OLARG( Widget,		w)
	OLARG( OlVirtualName,	type)
	OLGRA( XtPointer,	data)
{
	Boolean		ret_val = False;
	Cardinal	item_index;

	if (data == (XtPointer)NULL)
	{
		item_index = FPART(w).focus_item;
	} else
	{
		item_index = DATA_TO_INDEX(data);

		if (item_index >= FPART(w).num_items)
		{
			item_index = (Cardinal)OL_NO_ITEM;
		}
	}

	if (item_index != (Cardinal)OL_NO_ITEM)
	{
		OL_FLAT_ALLOC_ITEM(w, FlatItem, item);

		OlFlatExpandItem(w, item_index, item);
		ret_val = OlFlatItemActivate(w, item, type, data);

		OL_FLAT_FREE_ITEM(item);
	}
	else
	{
		if (type == OL_SELECTKEY)
		{
			Time	time = CurrentTime;

			ret_val = TRUE;
			(void)XtCallAcceptFocus(w, &time);
		}
	}
	return(ret_val);
} /* END OF ActivateWidget() */

/*
 *************************************************************************
 * AnalyzeItems - this routine analyzes new items for this class.
 * This routine is called after all of the items have been initialized,
 * so we can assume that all items have valid values.  If this widget
 * is the focus widget, then the application probably touched the item
 * list, so let's pick an item to take focus.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
AnalyzeItems OLARGLIST((w, args, num_args))
	OLARG( Widget,		w)		/* flat widget id	*/
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	if (PWIDGET(w).has_focus == True)
	{
		Time		timestamp = CurrentTime;

		(void) SetFocusToAnyItem(w, &timestamp);
	}
} /* END OF AnalyzeItems() */

/*
 *************************************************************************
 * ClassInitialize - 
 ****************************procedure*header*****************************
 */
static void
ClassInitialize OL_NO_ARGS()
{
	_OlFlatAddConverters();
} /* END OF ClassInitialize() */

/*
 *************************************************************************
 * SetupRequiredResources - this routine sets up the list used to specify
 * resources that should be set on each sub-object.  If the application
 * supplies an item fields list that does not contain any of these
 * resources, the flat superclass maintains them internally.  This
 * features permits flattened widget writers from worrying about tracking
 * resources that the application did not set in its list.  A typical
 * example is the XtNset or XtNcurrent.
 ****************************procedure*header*****************************
 */
static void
SetupRequiredResources OLARGLIST((wc, fcp, sfcp))
	OLARG( WidgetClass,		wc)
	OLARG( FlatClassPart *,		fcp)
	OLGRA( FlatClassPart *,		sfcp)		/* may be NULL	*/
{
		/* Append this subclass's required resources to those
		 * required by the superclasses.  If the
		 * subclass does not have a list, the superclass's list
		 * information is copied into the subclass.		*/

	if (fcp->num_required_resources != (Cardinal)0 &&
	    fcp->required_resources == (OlFlatReqRscList)NULL)
	{
	OlVaDisplayErrorMsg((Display *)NULL, 
	                OleNinvalidRequiredResource,
			OleTnullList, OleCOlToolkitError,
			OleMinvalidRequiredResource_nullList,
			OlWidgetClassToClassName(wc),
			"required_resources", "num_required_resources");
	}
	else if (fcp->num_required_resources == (Cardinal)0)
	{
		if (sfcp != (FlatClassPart *)NULL)
		{
			fcp->required_resources     = sfcp->required_resources;
			fcp->num_required_resources =
						sfcp->num_required_resources;
		}
	}
	else
	{
		Cardinal	super_num = (sfcp ?
					sfcp->num_required_resources : 0);
		Cardinal	num = fcp->num_required_resources + super_num;
		OlFlatReqRsc *	req_rsc;
		XrmQuarkList	qlist;
		Cardinal	i;
		Cardinal	j;
		XrmQuark	quark;
		OlFlatReqRsc *	new_list;
		Boolean		copied_into_super_list = False;

		new_list = (OlFlatReqRsc *)XtMalloc(num * sizeof(OlFlatReqRsc));

				/* copy superclass's list into new list	*/

		if (super_num != (Cardinal)0)
		{
			(void)memcpy((char *)new_list,
				(OLconst char *) sfcp->required_resources,
				(int) (super_num * sizeof(OlFlatReqRsc)));
		}

			/* Loop over subclass's req. resources and if a
			 * resource is not in the superclass's req. list,
			 * include it the new list.			*/

		for (i=0, num = super_num,
		     req_rsc = fcp->required_resources,
		     qlist = fcp->quarked_items;
		     i < fcp->num_required_resources;
		     ++i, ++req_rsc)
		{
			quark = XrmStringToQuark(req_rsc->name);

			for (j=0; j < super_num; ++j)
			{
				/* If the quark signatures equal, then
				 * a superclass has already specified this
				 * resource as a required one, so copy the
				 * subclass' required_resource structure
				 * into the array since the subclass may
				 * have changed one of the fields.	*/

				if (quark == qlist[new_list[j].rsc_index])
				{
					if (memcmp((OLconst char *)(new_list+j),
						(OLconst char *)req_rsc,
						(int)sizeof(OlFlatReqRsc)))
					{
						req_rsc->rsc_index =
							new_list[j].rsc_index;

						copied_into_super_list = True;

						(void)memcpy(
						   (char *)(new_list+j),
						   (OLconst char *)req_rsc,
						   (int)sizeof(OlFlatReqRsc));
					}
					continue;
				}
			}

				/* Check to see if this resource is
				 * a valid one.				*/

			for (j=0; j < fcp->num_item_resources; ++j)
			{
			    if (quark == qlist[j])
			    {
				char *	default_type = (char *)
					  fcp->item_resources[j].default_type;

				if (default_type != (char *)NULL &&
				    strcmp(XtRImmediate, default_type)!=0)
				{
					new_list[num] = *req_rsc;
					new_list[num].rsc_index = j;
					++num;
				}
				else
				{
	OlVaDisplayErrorMsg((Display *)NULL, 
	                OleNinvalidRequiredResource,
			OleTbadDefaultType, OleCOlToolkitError,
			OleMinvalidRequiredResource_badDefaultType,
			OlWidgetClassToClassName(wc), req_rsc->name);
				}
				break;
			    }
			}

			if (j == fcp->num_item_resources)
			{
	OlVaDisplayErrorMsg((Display *)NULL, 
	                OleNinvalidRequiredResource,
			OleTbadValue, OleCOlToolkitError,
			OleMinvalidRequiredResource_badValue,
			OlWidgetClassToClassName(wc), req_rsc->name);
			}
		}

			/* Now that the two lists have been merged,
			 * free any extra memory.			*/

		if (num < (fcp->num_required_resources + super_num))
		{
			/* If the number of required_resources in the
			 * subclass equal the number of resources in the
			 * superclass, then if the subclass has not
			 * modified any of the superclass' resources,
			 * free the new_list and let the subclass use the
			 * same list as the superclass.			*/

			if (num == super_num &&
			    copied_into_super_list == (Boolean)False)
			{
				XtFree((char *)new_list);
				new_list = (OlFlatReqRsc *)(sfcp ?
					sfcp->required_resources : NULL);
			}
			else
			{
				new_list = (OlFlatReqRsc *)XtRealloc(
						(char *)new_list,
						num * sizeof(OlFlatReqRsc));
			}
		}

				/* Cache information in the subclass.	*/

		fcp->num_required_resources	= num;
		fcp->required_resources		= new_list;
	}
} /* END OF SetupRequiredResources() */

/*
 *************************************************************************
 * ClassPartInitialize - this routine initializes the widget's class
 * part field.  It Quarkifies the classes item's resource names and puts
 * them into a quark list.
 ****************************procedure*header*****************************
 */
static void
ClassPartInitialize OLARGLIST((wc))
	OLGRA( WidgetClass,  wc)	/* Flat Widget subclass		*/
{
	FlatClassPart * fcp;		/* this class's Flat Class Part	*/
	FlatClassPart * sfcp;		/* superclass's Flat Class Part	*/
	Cardinal	i;
#ifdef SHARELIB
	void **__libXol__XtInherit = _libXol__XtInherit;
#undef _XtInherit
#define _XtInherit		(*__libXol__XtInherit)
#endif

	fcp = &(((FlatWidgetClass) wc)->flat_class);

			/* If this is the flatWidgetClass, quark its
			 * item resources and return.			*/

	if (wc == flatWidgetClass)
	{
		Cardinal	i;
		XrmQuarkList	qlist;
		XtResourceList	rsc = fcp->item_resources;

		qlist = (XrmQuarkList) XtMalloc((Cardinal)
			(sizeof(XrmQuark) * fcp->num_item_resources));
		fcp->quarked_items = qlist;

		for (i = 0; i < fcp->num_item_resources; ++i, ++qlist, ++rsc)
		{
		    *qlist  = XrmStringToQuark(rsc->resource_name);
		}

		SetupRequiredResources(wc, fcp, (FlatClassPart *)NULL);
		return;
	}

				/* Get the superclasses flat class part	*/

	sfcp = &(((FlatWidgetClass) wc->core_class.superclass)->flat_class);

			/* Now, set up the item resources for this class.
			 * The item resources for this class will be merged
			 * with those of the superclass.  If the subclass
			 * and superclass item resource have the same offset,
			 * the subclasses item resource will be used.	*/

	if (fcp->num_item_resources == (Cardinal)0 ||
	    fcp->item_resources != (XtResourceList)NULL)
	{
			/* If this subclass doesn't add any of it's
			 * own resources, simply copy the superclass's
			 * pointers into this class's fields.		*/

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
		XrmQuark	quark;
		Cardinal	i;
		Cardinal	j;
		Cardinal	num = fcp->num_item_resources +
					sfcp->num_item_resources;

				/* Allocate arrays large enough to fit
				 * both the superclass and the subclass
				 * item resources.			*/

		qlist = (XrmQuarkList) XtMalloc((Cardinal)
					(sizeof(XrmQuark) * num));
		fcp->quarked_items = qlist;

		rsc = (XtResourceList) XtMalloc((Cardinal)
					(sizeof(XtResource) * num));

				/* Copy the superclass item resources
				 * into the new list.			*/

		for (i = 0; i < sfcp->num_item_resources; ++i)
		{
			rsc[i] = sfcp->item_resources[i];
			qlist[i] = sfcp->quarked_items[i];
		}

				/* Now merge this class's item resources
				 * into the new list.			*/

		for (i = 0, num = sfcp->num_item_resources,
		     fcp_list = fcp->item_resources;
		     i < fcp->num_item_resources;
		     ++i, ++fcp_list)
		{
			quark = XrmStringToQuark(fcp_list->resource_name);

					/* Compare each new item resource
					 * against the superclass's	*/

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
		} /* end of merging in this subclass's resources	*/

				/* At this point the two lists are merged.
				 * Now deallocate extra memory.		*/

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
		OlVaDisplayErrorMsg((Display *)NULL,OleNinvalidResource,
			OleTnullList, OleCOlToolkitError,
			OleMinvalidResource_nullList, "",
			OlWidgetClassToClassName(wc),
			"item_resources", "num_item_resources");
	}

	SetupRequiredResources(wc, fcp, sfcp);

		/* Inherit procedures that need to be inherited.
		 * If a required class procedure is NULL, an error
		 * is generated.					*/

	for (i = XtNumber(class_fields); i--; )
	{
		XtPointer *	dest = (XtPointer *)
				((char *)wc + class_fields[i].offset);

		if (*dest == (XtPointer)_XtInherit)
		{
			*dest = *(XtPointer*)
					((char *)wc->core_class.superclass +
						class_fields[i].offset);
		}
		else if (*dest == (XtPointer)NULL && class_fields[i].required)
		{
			OlVaDisplayErrorMsg((Display *)NULL,
				OleNinvalidProcedure,
				OleTinheritanceProc, OleCOlToolkitError,
				OleMinvalidProcedure_inheritanceProc,
				OlWidgetClassToClassName(wc),
				class_fields[i].required);
		}
	}

				/* Now check the record size	*/

	if (fcp->rec_size == (Cardinal)0 || fcp->rec_size < sfcp->rec_size)
	{
		OlVaDisplayErrorMsg((Display *)NULL, OleNinvalidItemRecord,
			OleTflatState, OleCOlToolkitError,
			OleMinvalidItemRecord_flatState,
			OlWidgetClassToClassName(wc),
			(unsigned)fcp->rec_size, (unsigned)sfcp->rec_size);
	}

			/* Now check to see if the subclass is incorrectly
			 * using the CORE Initialize and SetValues
			 * procedures.
			 */

#define OleNimproperSubclassing		"improperSubclassing"
#define OleMimproperSubclassing_flatState "Flat Subclass \"%s\" should not have\
 non-NULL CORE %s procedure, put the procedure pointer in the flat class part"
#define ECHO_ERROR(proc)	\
	OlVaDisplayErrorMsg((Display *)NULL, OleNimproperSubclassing,\
		OleTflatState, OleCOlToolkitError,\
		OleMimproperSubclassing_flatState, \
		OlWidgetClassToClassName(wc), proc)

	if (wc->core_class.initialize != (XtInitProc)NULL)
	{
		ECHO_ERROR("Initialize");
	}
	if (wc->core_class.set_values != (XtSetValuesFunc)NULL)
	{
		ECHO_ERROR("SetValues");
	}
} /* END OF ClassPartInitialize() */

/*
 *************************************************************************
 * DefaultItemInitialize - checks the initial values of the default item.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
DefaultItemInitialize OLARGLIST((w, request, new, args, num_args))
	OLARG( Widget,	   w)		/* New widget			*/
	OLARG( FlatItem,   request)	/* expanded requested item	*/
	OLARG( FlatItem,   new)		/* expanded new item		*/
	OLARG( ArgList,	   args)
	OLGRA( Cardinal *, num_args)
{
	if (new->flat.label != (String)NULL)
	{
		new->flat.label = XtNewString(new->flat.label);
	}

			/* Insure these fields are not inherited from
			 * the container.				*/

	MAINTAIN_SAME_VALUES(new);

			/* Always make the font mirror the font value in
			 * the Primitive widget part since we know we've
			 * already checked the validity of the font in
			 * our Initialize procedure (which is called
			 * sometime before this one).
			 */

	new->flat.font = PWIDGET(w).font;

							/* Get the GCs	*/
	GetGCs(w, new);
} /* END OF DefaultItemInitialize() */

/*
 *************************************************************************
 * DefaultItemSetValues - this routine is called whenever the application
 * does an XtSetValues on the container, possibly requesting that 
 * attributes of the default item be updated.
 * If the "widget" is to be refreshed, the routine returns True.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
DefaultItemSetValues OLARGLIST((w, current, request, new,
			args, num_args))
	OLARG( Widget,	   w)		/* New widget			*/
	OLARG( FlatItem,   current)	/* expanded current item	*/
	OLARG( FlatItem,   request)	/* expanded requested item	*/
	OLARG( FlatItem,   new)		/* expanded new item		*/
	OLARG( ArgList,	   args)
	OLGRA( Cardinal *, num_args)
{
	Boolean	redisplay = False;
	Boolean	get_new_gcs = False;

#define DIFFERENT(field)	(new->flat.field != current->flat.field)

			/* Insure these fields are changed via a set
			 * values on the container.			*/

	MAINTAIN_SAME_VALUES(new);

	if (DIFFERENT(background_pixel)		||
	    DIFFERENT(background_pixmap)	||
	    DIFFERENT(font_color)		||
	    DIFFERENT(foreground))
	{
		get_new_gcs = True;
		redisplay = True;
	}

	if (DIFFERENT(input_focus_color)	||
	    DIFFERENT(label_tile)		||
	    DIFFERENT(label_image)		||
	    DIFFERENT(label_justify))
	{
	       redisplay = True;
	}

		/* If the font changed, suggest a re-layout since the
		 * new font might be a different size,
		 * Remember, if the new font is NULL, use the one
		 * in the screen manager.				*/

	if (DIFFERENT(font))
	{

			/* Always make the font mirror the font value in
			 * the Primitive widget part since we know we've
			 * already checked the validity of the font in
			 * our SetValues procedure (which is called
			 * sometime before this one).
			 */

		new->flat.font = PWIDGET(w).font;

		FPART(w).relayout_hint = True;
		redisplay = True;
		get_new_gcs = True;
	}

	if (DIFFERENT(label))
	{
		if (new->flat.label != (String)NULL)
		{
			new->flat.label = XtNewString(new->flat.label);
		}

		if (current->flat.label != (String)NULL)
		{
			XtFree((char *)current->flat.label);
			current->flat.label = (String)NULL;
		}

		FPART(w).relayout_hint = True;
		redisplay = True;
	}

	if (get_new_gcs == True)
	{
		GetGCs(w, new);
	}

	return(redisplay);

#undef DIFFERENT
#undef KEEP_NEW_SAME_AS_CURRENT
} /* END OF DefaultItemSetValues() */

/*
 *************************************************************************
 * Destroy - this procedure frees memory allocated by the instance part
 ****************************procedure*header*****************************
 */
static void
Destroy OLARGLIST((w))
	OLGRA( Widget,	w)
{
	_OlFlatStateDestroy(w);

				/* Destroy the default item's label	*/

	XtFree((XtPointer) OlFlatDefaultItem(w)->flat.label);

				/* Free reference from screen cache	*/

	(void) OlFlatScreenManager(w, OL_DEFAULT_POINT_SIZE, OL_DELETE_REF);

						/* Destroy the GCs	*/

	OlgDestroyAttrs(FPART(w).pAttrs);
	XtReleaseGC(w, FPART(w).label_gc);
	XtReleaseGC(w, FPART(w).inverse_gc);

} /* END OF Destroy() */

/*
 *************************************************************************
 * HighlightHandler - this routine is called whenever the flattened widget
 * container gains or looses focus.
 ****************************procedure*header*****************************
 */
static void
HighlightHandler OLARGLIST((w, type))
	OLARG( Widget,		w)
	OLGRA( OlDefine,	type)
{
	if (type == OL_IN)
	{
		Boolean	took_focus = False;

		/* If we get a focus in, use try to set focus to the
		 * last item that had it (since the focus out may have
		 * been caused by a pointer grab, e.g., the window
		 * manager grabbed the pointer when dragging the window).
		 * But if the focus out wasn't due to a grab, then we
		 * have to check to see if the last focus item still
		 * can take focus.  So in either case, we have to
		 * play it safe and formally request the item to 
		 * take focus.
		 */
		if (FPART(w).last_focus_item != (Cardinal) OL_NO_ITEM)
		{
			took_focus = OlFlatCallAcceptFocus(w,
					FPART(w).last_focus_item, CurrentTime);
		}

		if (took_focus == False)
		{
			Time	timestamp = CurrentTime;

			FPART(w).last_focus_item = (Cardinal)OL_NO_ITEM;
			(void) SetFocusToAnyItem(w, &timestamp);
		}
	}
	else if (type == OL_OUT && FPART(w).focus_item != (Cardinal)OL_NO_ITEM)
	{
		OL_FLAT_ALLOC_ITEM(w, FlatItem, item);

		OlFlatExpandItem(w, FPART(w).focus_item, item);

		FPART(w).focus_item = (Cardinal)OL_NO_ITEM;

		CALL_ITEM_HIGHLIGHT(w, item, OL_OUT);

		OL_FLAT_FREE_ITEM(item);
	}

} /* END OF HighlightHandler() */

/*
 *************************************************************************
 * Initialize - this procedure initializes the instance part of the widget
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize OLARGLIST((request, new, args, num_args))
	OLARG( Widget,		request)	/* What we want		*/
	OLARG( Widget,		new)		/* What we get, so far..*/
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	OlFlatScreenCache *	sc;

			/* If the items and itemFields converters have not
			 * modified the num_items and num_item_fields
			 * values, set them now.			*/

	if (FPART(new).num_items == (Cardinal) OL_IGNORE)
	{
		FPART(new).num_items = (Cardinal)0;
	}

	if (FPART(new).num_item_fields == (Cardinal) OL_IGNORE)
	{
		FPART(new).num_item_fields = (Cardinal)0;
	}

	FPART(new).focus_item		= (Cardinal)OL_NO_ITEM;
	FPART(new).last_focus_item	= (Cardinal)OL_NO_ITEM;
	FPART(new).pAttrs		= (OlgAttrs *) NULL;
	FPART(new).label_gc		= (GC) NULL;
	FPART(new).inverse_gc		= (GC) NULL;

						/* Cache screen data	*/

	sc = OlFlatScreenManager(new, OL_DEFAULT_POINT_SIZE, OL_ADD_REF);

			/* Set the container's background properly	*/

	CALL_TRANSPARENT_PROC(new);


	if (FPART(new).items_touched == True)
	{
		ItemsTouched(new);
	}

	/*  Set the border_width back to the original request to 
	    override the Primitive's XtNhighlightHandler.  For Flats,
	    the individual items draw the highlight border, rather than
	    using the window's border.  */
	if (OlGetGui() == OL_MOTIF_GUI)  {
		new->core.border_width = request->core.border_width;
	}

} /* END OF Initialize() */

/*
 *************************************************************************
 * ItemAcceptFocus - this routine is called to set focus to a particular
 * sub-object.  If this object can take focus, it set focus to itself.
 ****************************procedure*header*****************************
 */
static Boolean
ItemAcceptFocus OLARGLIST((w, item, time))
	OLARG( Widget,		w)
	OLARG( FlatItem,	item)
	OLGRA( Time *,		time)
{
	Boolean		can_take_focus;

	if (item->flat.sensitive == (Boolean)True		&&
	    item->flat.managed == (Boolean)True			&&
	    item->flat.traversal_on == (Boolean)True		&&
	    item->flat.mapped_when_managed == (Boolean)True	&&
	    (PWIDGET(w).has_focus == True ||
	     OlCanAcceptFocus(w, *time) == True))
	{
				/* If the requested item can take focus
				 * and the flat container already has focus,
				 * call the unhighlight handler for the
				 * item that has focus and the highlight
				 * handler the new focus item.
				 * If the container does not have focus
				 * yet, set it to the container.
				 */

		can_take_focus			= True;

		if (PWIDGET(w).has_focus == True)
		{
			Cardinal	old_focus_item;

			old_focus_item	= FPART(w).focus_item;
			FPART(w).last_focus_item= item->flat.item_index;
			FPART(w).focus_item	= item->flat.item_index;
			if (old_focus_item != (Cardinal)OL_NO_ITEM)
			{
				OL_FLAT_ALLOC_ITEM(w, FlatItem, old_item);

				OlFlatExpandItem(w, old_focus_item, old_item);
				CALL_ITEM_HIGHLIGHT(w, old_item, OL_OUT);

				OL_FLAT_FREE_ITEM(old_item);
			}

			CALL_ITEM_HIGHLIGHT(w, item, OL_IN);
		}
		else
		{
				/* Set focus to the container.  When the
				 * container gains focus, it's highlight
				 * handler will be called and the appropriate
				 * sub-object will be highlighted since we've
				 * set the container's focus_item field.
				 */
			can_take_focus = OlSetInputFocus(w, RevertToNone,*time);

			if (can_take_focus)
				FPART(w).last_focus_item =item->flat.item_index;
		}
	}
	else
	{
		can_take_focus = False;
	}

	return(can_take_focus);
} /* END OF ItemAcceptFocus() */

/*
 *************************************************************************
 * ItemHighlight - this routine is called to highlight or unhighlight a
 * particular sub-object as it gains or loses focus.
 ****************************procedure*header*****************************
 */
/* ARGSUSED2 */
static void
ItemHighlight OLARGLIST((w, item, type))
	OLARG( Widget,		w)
	OLARG( FlatItem,	item)
	OLGRA( OlDefine,	type)
{
	OlFlatRefreshExpandedItem(w, item, True);
} /* END OF ItemHighlight() */

/*
 *************************************************************************
 * ItemInitialize -
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
ItemInitialize OLARGLIST((w, request, new, args, num_args))
	OLARG( Widget,	   w)		/* Flat widget container id	*/
	OLARG( FlatItem,   request)	/* expanded requested item	*/
	OLARG( FlatItem,   new)		/* expanded new item		*/
	OLARG( ArgList,	   args)
	OLGRA( Cardinal *, num_args)
{
	switch(new->flat.label_justify)
	{
	case OL_LEFT:		/* FALLTHROUGH	*/
	case OL_RIGHT:		/* FALLTHROUGH	*/
	case OL_CENTER:
		break;		/* good justification, do nothing*/
	default:
		OlVaDisplayWarningMsg((Display *)NULL,
			OleNinvalidResource,
			OleTflatState, OleCOlToolkitWarning,
			OleMinvalidResource_flatState,
			XtName(w), OlWidgetToClassName(w), XtNlabelJustify,
			(unsigned)new->flat.item_index, "OL_CENTER");

		new->flat.label_justify = (OlDefine)OL_CENTER;
		break;
	}

	CheckFont(w, new);

	(void)SetAccAndMnem(w, (FlatItem)NULL, new);

} /* END OF ItemInitialize() */

/*
 *************************************************************************
 * ItemsTouched - this procedure is called whenever a new list is given to
 * the flat widget or whenever the existing list is touched.  These
 * conditions are checked for the Initialize and SetValues routines.
 * In this routine, we de-allocate any memory associated with the last
 * list and create any memory needed for the new list.
 ****************************procedure*header*****************************
 */
static void
ItemsTouched OLARGLIST((new))
	OLGRA( Widget,	new)
{ 
	if (FPART(new).focus_item != (Cardinal) OL_NO_ITEM)
	{
		FPART(new).last_focus_item	= (Cardinal)OL_NO_ITEM;
		FPART(new).focus_item		= (Cardinal)OL_NO_ITEM;
	}

			/* Once we've set the focus item, reset the
			 * last focus item.				*/

	FPART(new).last_focus_item = FPART(new).focus_item;

			/* Since the user has added a new list or
			 * changed the old list, we must remove all
			 * mnemonics and accelerators associated
			 * with this flat widget.
			 */
	_OlRemoveAccelerator(new, (XtPointer)NULL, True, (String)NULL);
	_OlRemoveMnemonic(new, (XtPointer)NULL, True, (OlMnemonic)'\0');

} /* END OF ItemsTouched() */

/*
 *************************************************************************
 * ItemSetValues - this routine is called whenever the application does
 * an XtSetValues on the container, requesting that an item be updated.
 * If the item is to be refreshed, the routine returns True.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
ItemSetValues OLARGLIST((w, current, request, new, args, num_args))
	OLARG( Widget,	   w)		/* Flat widget container id	*/
	OLARG( FlatItem,   current)	/* expanded current item	*/
	OLARG( FlatItem,   request)	/* expanded requested item	*/
	OLARG( FlatItem,   new)		/* expanded new item		*/
	OLARG( ArgList,	   args)
	OLGRA( Cardinal *, num_args)
{
	Boolean	reconfigure = False;
	Boolean	redisplay = False;

				/* Set the mnemonic and accelerators	*/

	if (SetAccAndMnem(w, current, new) == True)
	{
		reconfigure	= True;
		redisplay	= True;
	}

#define DIFFERENT(field)	(new->flat.field != current->flat.field)

	if (DIFFERENT(mapped_when_managed) ||
	    DIFFERENT(managed) ||
	    DIFFERENT(background_pixel) ||
	    DIFFERENT(background_pixmap) ||
	    DIFFERENT(sensitive)	||
	    DIFFERENT(font_color)	||
	    DIFFERENT(font)		||
	    DIFFERENT(label)		||
	    DIFFERENT(label_justify)	||
	    DIFFERENT(label_image)	||
	    DIFFERENT(label_tile))
	{
		if ( (DIFFERENT(sensitive) && new->flat.sensitive == False ||
		      DIFFERENT(managed) && new->flat.managed == False ||
		      DIFFERENT(mapped_when_managed) &&
				new->flat.mapped_when_managed == False) &&
		     new->flat.item_index == FPART(w).focus_item)
		{
			_OlSetCurrentFocusWidget(w, OL_OUT);
			if (w == OlMoveFocus(w, OL_IMMEDIATE, CurrentTime))
				_OlSetCurrentFocusWidget(w, OL_IN);
		}

		if (DIFFERENT(font))
		{
			reconfigure = True;
			CheckFont(w, new);
		}
		else if (DIFFERENT(label))
		{
			reconfigure = True;
		}
		redisplay = True;
	}

	if (reconfigure == True)
	{
		OlFlatItemDimensions(w, new, &new->flat.width,
						&new->flat.height);
	}

	return(redisplay);

#undef DIFFERENT
} /* END OF ItemSetValues() */

/*
 *************************************************************************
 * SetValues - this procedure monitors the changing of instance data.
 * Since the subclasses inherit the layout from this class, whenever this
 * class detects a change in any of its layout parameters, it sets a flag
 * in the instance part so that the subclasses merely have to check the
 * flag instead of checking all of their layout parameters.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues OLARGLIST((current, request, new, args, num_args))
	OLARG( Widget,		current)	/* What we had		*/
	OLARG( Widget,		request)	/* What we want		*/
	OLARG( Widget,		new)		/* What we get, so far	*/
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	Boolean	redisplay = False;

			/* Adjust the container in accordance with
			 * it's transparency.				*/

#define DIFFERENT(field)	(new->core.field != current->core.field)
#define PW(w)			(((FlatWidget)(w))->primitive)
#define PDIFF(field)		(PW(new).field != PW(current).field)

	if (DIFFERENT(background_pixel) ||
	    DIFFERENT(background_pixmap))
	{
		CALL_TRANSPARENT_PROC(new);
	}

	if (FPART(new).items_touched != FPART(current).items_touched)
	{
		redisplay = True;
		ItemsTouched(new);
	}

		/* If the sensitivity is changed, then we should force
		 * a redisplay. Note: we don't need to track the
		 * ancestor_sensitive because the Xt manual said
		 * that: you should use XtSetSensitive() so tracking
		 * "sensitive" is enough.
		 */
	if (XtIsSensitive(new) != XtIsSensitive(current))
	{
		redisplay = True;
	}

	if (OlGetGui() == OL_MOTIF_GUI && PDIFF(highlight_thickness))
	{
		/*  Undo the Primitive widget setting the border_width
		    to match the highlight_thickness.  */
		if (new->core.border_width != request->core.border_width)  {
			new->core.border_width = request->core.border_width;
		}
	}

		/* We don't need to check PDIFF(font) because this
		 * change will be picked up in DefaultItemSetValue()
		 */
	if (PDIFF(shadow_thickness) || PDIFF(font_list))
	{
		FPART(new).relayout_hint = True;

		redisplay = True;
	}


	return(redisplay);

#undef DIFFERENT
#undef PW
#undef PDIFF
} /* END OF SetValues() */

/*
 *************************************************************************
 * TransparentProc - this routine maintains the "transparency" of the
 * flat widget container.   It is called whenever the parent of the flat
 * widget has it's background changed via a SetValues.  It is also
 * called if the flat widget's background is updated.
 ****************************procedure*header*****************************
 */
static void
TransparentProc OLARGLIST((w, pixel, pixmap))
	OLARG( Widget,	w)
	OLARG( Pixel,	pixel)			/* parent's bg pixel	*/
	OLGRA( Pixmap,	pixmap)			/* parent's bg pixmap	*/
{
			/* If the container's background should be
			 * transparent, get it from it's parent		*/

	if (OL_FLATCLASS(w).transparent_bg == True)
	{
		unsigned long	mask = (unsigned long)0;

		if (w->core.background_pixel != pixel)
		{
			mask |= CWBackPixel;
			w->core.background_pixel = pixel;
		}

		if (w->core.background_pixmap != pixmap)
		{
			if (pixmap == XtUnspecifiedPixmap)
			{
				/* If the parent now has an unspecified
				 * pixmap, use the parents background
				 * pixel color and reset our pixmap	*/

				mask |= CWBackPixel;
				w->core.background_pixmap = pixmap;
			}
			else
			{
				mask |= CWBackPixmap;
				w->core.background_pixmap = ParentRelative;
			}
		}

		if (mask && XtIsRealized(w) == (Boolean)True)
		{
			XSetWindowAttributes	values;

			values.background_pixmap = w->core.background_pixmap;
			values.background_pixel = w->core.background_pixel;

			XChangeWindowAttributes(XtDisplay(w),
					XtWindow(w), mask, &values);
			_OlClearWidget(w, True);
		}
	}
} /* END OF TransparentProc() */

/*
 *************************************************************************
 * TraversalHandler - this routine is the external interface for moving
 * focus among flattened widget sub-objects.  This routine calls the
 * a flat class procedure to do the actual focus movement.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Widget
TraversalHandler OLARGLIST((w, ignore, dir, time))
	OLARG( Widget,		w)	/* FlatWidget id		*/
	OLARG( Widget,		ignore)	/* same FlatWidget id		*/
	OLARG( OlVirtualName,	dir)	/* Direction to move		*/
	OLGRA( Time,		time)	/* Time of move (ignored)	*/
{
	if (FPART(w).focus_item != (Cardinal)OL_NO_ITEM &&
	    OL_FLATCLASS(w).traverse_items)
	{
		(void)(*OL_FLATCLASS(w).traverse_items)(w, FPART(w).focus_item,
					dir, time);
	}

	return((FPART(w).focus_item != (Cardinal)OL_NO_ITEM ? w : NULL));
} /* END OF TraversalHandler() */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/* There are no action procedures */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * OlFlatInheritAll - procedure used by subclass's to inherit all 
 * 'inheritable' attributes from its superclass.  This procedure is
 * called from a subclass's ClassInitialize procedure.  After calling
 * this procedure, a subclass usually then overrides a few of the
 * inherited routines to give the subclass some unique characteristics
 * of it's own.
 ****************************procedure*header*****************************
 */
void
OlFlatInheritAll OLARGLIST((wc))
	OLGRA( WidgetClass,	wc)
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
} /* END OF OlFlatInheritAll */

