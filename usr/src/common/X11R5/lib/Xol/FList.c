#ifndef NOIDENT
#ident	"@(#)flat:FList.c	1.81"
#endif

/******************************file*header********************************

    Description:
	This file contains the source code for the flat list container.
*/
						/* #includes go here	*/
#if defined(__STDC__) || defined(sun)
#include <stdlib.h>
#include <limits.h>
#else
extern long strtol();
#define USHRT_MAX (unsigned short)(~0)
#endif

#ifdef I18N
#include <ctype.h>

#if defined(SVR4_0) || defined(SVR4)	/* do we care SVR3?? */

#include <widec.h>
#include <wctype.h>	/* For post-6.0 ccs, include <wchar.h> instead */

#else /* defined(SVR4_0) || defined(SVR4) */

	/* Let Xlib.h take care this if XlibSpecificationRelease
	 * is defined (note that, this symbol is introduced in
	 * X11R5), otherwise do similar check as in X11R5:Xlib.h
	 */
#if !defined(XlibSpecificationRelease)

#if defined(X_WCHAR) || defined(sun)
#include <stddef.h>

#ifndef iswspace
#define iswspace	isspace
#endif

#endif

#endif /* !defined(XlibSpecificationRelease) */
#endif /* defined(SVR4_0) || defined(SVR4) */
#endif /* I18N */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/OlCursors.h>
#include <Xol/Dynamic.h>
#include <Xol/Error.h>
#include <Xol/FListP.h>
#include <Xol/Scrollbar.h>
#include <Xol/ScrolledWi.h>

#define ClassName FlatList
#include <Xol/NameDefs.h>


/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Class   Procedures
		3. Action  Procedures
		4. Public  Procedures 
*/

typedef void	(*OlgFlatListLabelProc) OL_ARGS((
			Screen *, Drawable, OlgAttrs *,
			Position, Position, Dimension, Dimension,
			XtPointer, OlBitMask));

					/* private procedures		*/
static void	AdjustTop OL_ARGS((FlatListWidget));
static void	CalibrateVSBar OL_ARGS((FlatListWidget));
static void	CallCallProc OL_ARGS((FlatListWidget, Cardinal,
				      XtCallbackProc, XtPointer, XtPointer,
				      OlFlatDropCallData *));
static Boolean	CallLimitExceeded OL_ARGS((FlatListWidget, Cardinal,
						Dimension, Dimension));
static void	CallSelectProc OL_ARGS((FlatListWidget, Cardinal, Boolean));
static Cardinal	CharCnt OL_ARGS((String, char));
static void	ComputeFieldSize OL_ARGS((FlatListWidget,
					  FlatListItem, int,
					  Dimension *, Dimension *));
static void	ComputePadding OL_ARGS((FlatListWidget));
static void	ComputeTextSize OL_ARGS((FlatListWidget,
					 FlatListItem, int,
					 Dimension *, Dimension *));
static OlBitMask
		CreateDragCursor OL_ARGS((Widget, OlVirtualEvent,
					  XtPointer, XtPointer,
					  OlFlatDragCursorCallData *));
static void	DisplayList OL_ARGS((FlatListWidget,
				     Position, Dimension, Boolean));
static void	DrawFields OL_ARGS((Screen *, Drawable, OlgAttrs *,
				    Position, Position, Dimension,
				    Dimension, void *, OlBitMask));
static unsigned short
		GetMaxCharLen OL_ARGS((OlFontList *, String, unsigned short));
static OlFlatCallData *
		GetItemData OL_ARGS((FlatListWidget, Cardinal, XtPointer));
static Cardinal	GetSelectedItems OL_ARGS((FlatListWidget, Cardinal, 
					  Cardinal, Cardinal, Cardinal,
					  Cardinal **));
static String	GetTextSegment OL_ARGS((FlatListWidget, FlatListItem,
					String, unsigned short, Boolean,
					unsigned short *));
static Boolean	_IsSelected OL_ARGS((FlatListWidget, Cardinal));
static void	ItemsTouched OL_ARGS((Widget, Widget));
static Dimension ItemWidth OL_ARGS((FlatListWidget));
static void	NonexcSelection OL_ARGS((FlatListWidget, Cardinal));
static void	OlgDrawFlatListItem OL_ARGS((
			Screen *, Drawable, OlgAttrs *, Position, Position,
			Dimension, Dimension, XtPointer,
			OlgFlatListLabelProc, OlBitMask,
			Dimension, Dimension));
static void	ParseFormat OL_ARGS((FlatListWidget));
static void	PostSWinInterface OL_ARGS((Widget, OlSWGeometries *));
static void	ScrollList OL_ARGS((FlatListWidget, int));
static void	SelectItem OL_ARGS((FlatListWidget, Cardinal, Boolean));
static void	SelectItems OL_ARGS((FlatListWidget,
				     Cardinal, int, Boolean)); 
static void	SetCurrentItem OL_ARGS((FlatListWidget, Cardinal));
static void	SetViewSize OL_ARGS((FlatListWidget));
static void	SWinInterface OL_ARGS((Widget, OlSWGeometries *));
static Boolean	UpdateItemMetrics OL_ARGS((FlatListWidget,
					   FlatListItem,
					   FlatListItem));
static void	ViewItem OL_ARGS((FlatListWidget, Cardinal));
static void	VSliderMovedCB OL_ARGS((Widget, XtPointer, XtPointer));
static void	WarnOnLimitExceeded OL_ARGS((Widget, Cardinal, Boolean));

					/* class procedures		*/

static void	AnalyzeItems OL_ARGS((Widget, ArgList, Cardinal *));
static void	ChangeManaged OL_ARGS((Widget, FlatItem *, Cardinal));
static void	ClassInitialize OL_NO_ARGS();
static void	Destroy OL_ARGS((Widget));
static void	DrawItem OL_ARGS((Widget, FlatItem, OlFlatDrawInfo *));
static Cardinal	GetItemIndex OL_ARGS((Widget, Position, Position, Boolean));
static void	Initialize OL_ARGS((Widget, Widget, ArgList, Cardinal *));
static Boolean	ItemActivate OL_ARGS((Widget, FlatItem, OlVirtualName,
				      XtPointer));
static void	ItemDimensions OL_ARGS((Widget, FlatItem, Dimension *,
					Dimension *));
static void	ItemInitialize OL_ARGS((Widget, FlatItem, FlatItem,
					ArgList, Cardinal *));
static Boolean	ItemSetValues OL_ARGS((Widget, FlatItem, FlatItem,
				       FlatItem, ArgList, Cardinal *));
static void	Redisplay OL_ARGS((Widget, XEvent *, Region));
static void	Resize OL_ARGS((Widget));
static Boolean	SetValues OL_ARGS((Widget, Widget, Widget,
				   ArgList, Cardinal *));
static Cardinal	TraverseItems OL_ARGS((Widget, Cardinal, OlVirtualName,
				       Time));

					/* action procedures		*/

static void	ButtonHandler OL_ARGS((Widget, OlVirtualEvent));
static void	KeyHandler OL_ARGS((Widget, OlVirtualEvent));
static void	ProcessClick OL_ARGS((FlatListWidget, Cardinal, Boolean));
static void	WipeThru OL_ARGS((XtPointer, XtIntervalId *));

					/* public procedures		*/


/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

#ifdef DEBUG
# include <stdio.h>
  Boolean FLdebug = False;
# define DPRINT(x)	if (FLdebug) (void)fprintf x
#else
# define DPRINT(x)
#endif

    /* This struct contains field specific information.  Members with '+' are
       derived from the format string.
    */
typedef struct _Field {
    unsigned char	type;		/* + string, image, etc */
#define STRING	PL_BITMAP + 1		/*  PL_ type ordering dependent! */

    unsigned short	width;		/* + spec'd field width if any */
    unsigned short	max;		/* + max string length */
    Dimension		max_width;	/*   computed maximum field width */
    Dimension		pre_padding;	/*   pre-field padding (in pixels) */
    unsigned char	pre_pad_cnt;	/* + number of pads before field */
    Boolean		right_justify;	/* + left/right justification */
    Boolean		wrap;		/* + wrap vs clip */
} Field;

/* Drawing-specific info is passed in this struct to the DrawFields proc */
typedef struct {
    Widget		widget;
    FlatItem		item;
    OlFlatDrawInfo *	draw_info;
} LabelInfo;

#define FLW(w)		( (FlatListWidget)(w) )
#define FLWC(w)		( (FlatListWidgetClass)XtClass(w) )
#define FLI(i)		( (FlatListItem)(i) )
#define FLPART(w)	( &FLW(w)->list )
#define FLCPART(w)	( &FLWC(w)->list_class )
#define FLIPART(i)	( &FLI(i)->list )

#define FPART(w)	( &((FlatWidget)(w))->flat )
#define FRCPART(w)	( &((FlatRowColumnWidget)(w))->row_column )
#define FIPART(i)	( &((FlatItem)(i))->flat )

#define FixNullFormat(w) \
		if (Format(w) == NULL) \
		{ \
		    OlVaDisplayWarningMsg(XtDisplay(w), \
					  OleNnullFormat, \
					  OleTinvalidFormat, \
					  OleCOlToolkitWarning, \
					  OleMnullFormat_invalidFormat, \
					  XtName(w), OlWidgetToClassName(w), \
					  OL_LIST_DEFAULT_FORMAT); \
							\
		    Format(w) = OL_LIST_DEFAULT_FORMAT; \
		}

#define InitState(w)	FLPART(w)->begin_scroll	= False;	\
			FLPART(w)->prev_motion	= NO_MOTION;	\
			FLPART(w)->start_indx	= OL_NO_ITEM;	\
			FLPART(w)->prev_indx	= OL_NO_ITEM

#define AboveView(w, i)		( (ItemOffset(w, i) + \
				   ItemHeight(w, i) - VERT_MARGIN) < YTop(w) )
#define BottomIsPartial(w)	( FLPART(w)->bottom_is_partial )
#define CurrentItem(w)		( FLPART(w)->prev_indx )
#define ExcItemIsSelected(w)	( SelectedItem(w) != OL_NO_ITEM )
#define Exclusives(w)		( FLPART(w)->exclusive_settings )
#define EmptyList(w)		( NumItems(w) == 0 )
#define EmptyHeight(w)		( (OlFontHeight(w->primitive.font, \
					       w->primitive.font_list) + \
				   (2 * VERT_MARGIN)) * ViewHeight(w) )
#define FocusItem(w)		( FPART(w)->focus_item )
#define Font(w, item)		( FIPART(item)->font )
#define FontHeight(w, item)	( OlFontHeight(Font(w, item), \
					       FontList(w, item)) )
#define FontList(w, item)	( ((PrimitiveWidget)(w))->primitive.font_list )
#define FontWidth(w, item)	( OlFontWidth(Font(w, item), \
					      FontList(w, item)) )
#define Format(w)		( FLPART(w)->format )
		/* name change because of NameDefs.h:GetIndex() */
#define FGetIndex(w, y)		OlFlatGetIndex((Widget)w, 0, y, True)
#define HighlightThickness(w)	( (w)->primitive.highlight_thickness )
#define InSWin(w)		( FLPART(w)->vSBar != NULL )
#define InView(w, i)		( YInView(w, ItemOffset(w, i) + VERT_MARGIN) \
				 || YInView(w, ItemOffset(w, i) + \
					    ItemHeight(w, i) - VERT_MARGIN) )
#define IsSelected(w, i)	( Exclusives(w) ? (i) == SelectedItem(w) : \
				 _IsSelected(FLW(w), i) )
#define ItemHeight(w, i)	( (((i) < NumItems(w) - 1) ? \
				   ItemOffset(w, (i) + 1) : TotalHeight(w)) \
				 - ItemOffset(w, i) )
#define ItemIndex(item)		( (item)->flat.item_index )
#define ItemOffset(w, i)	( FLPART(w)->y_offsets[i] )
#define LastFocusItem(w)	( FPART(w)->last_focus_item )
#define MaxFieldWidth(w, i)	( FLPART(w)->fields[i].max_width )
#define MinFieldWidth(w)	( FLPART(w)->min_field_width )
#define MinHeight(w)		( FLPART(w)->min_height )
#define NoneSet(w)		( FLPART(w)->none_set )
#define NumFields(w)		( FLPART(w)->num_fields )
#define NumItems(w)		( FPART(w)->num_items )
#define NumSlots(w)		( FLPART(w)->num_slots )
#define Preferred(w)		( FLPART(w)->preferred )
#define SelectedItem(w)		( FLPART(w)->selected_item )
#define SWin(w)			( XtParent(FLPART(w)->vSBar) )
#define Top(w)			( FLPART(w)->top_slot )
#define TopOffset(w)		( FRCPART(w)->y_offset )
#define TotalHeight(w)		( FLPART(w)->total_height )
#define TotalPadding(w)		( FLPART(w)->total_padding )
#define UseLabel(w, item)	( (NumFields(w) == 1) && \
				  (FLIPART(item)->field_data == NULL) )
#define UsePreferred(w)		( FLPART(w)->use_preferred )
#define ViewHeight(w)		( FLPART(w)->view_height )
#define ViewPixelHeight(w)	( ViewHeight(w) * MinHeight(w) )
#define VScrollable(w)		( InSWin(w) && (ViewHeight(w) < NumSlots(w)) )
#define YBottom(w)		( YTop(w) + ViewPixelHeight(w) )
#define YInView(w, y)		( ((y) > YTop(w)) && ((y) < YBottom(w)) )
#define YTop(w)			( Top(w) * MinHeight(w) )

#undef DELIMITER		/* undef definition in OlClients.h !! */
#define DELIMITER	'%'	/* conversion delimiter */
#define RIGHT_JUSTIFY	'-'	/* specifies right-justified field */
#define UNSPECIFIED	USHRT_MAX/* to flag unspecified field value */
#define WHITE_SPACE	" \t"	/* for wrapping */

#define CountFields(str)	CharCnt(str, DELIMITER)
#define CountLines(str)		( CharCnt(str, '\n') + 1 )
#define CountPads(str)		CharCnt(str, ' ')

/* vert margin must be at least 3 (see _OlgDrawRectButton and DrawItem) */
#define VERT_MARGIN	3
#define HORIZ_MARGIN	3

#define UP		'u'
#define DOWN		'd'
#define NO_MOTION	'\0'

/***********************widget*translations*actions***********************

    Define Translations and Actions
*/

#define OL_N_EVENT_PROCS	XtNumber(event_procs)-1 /* See ClassInit */
static OlEventHandlerRec event_procs[] = {
    { ButtonPress,	ButtonHandler },
    { ButtonRelease,	ButtonHandler },
    { KeyPress,		KeyHandler },	/* Have to be the last ...	*/
					/* because it's only applied to	*/
					/* Motif. See ClassInit...	*/
    };

/****************************widget*resources*****************************

    Define Resource list associated with the Widget Instance
*/

#define OFFSET(field) XtOffsetOf(FlatListRec, field)

static XtResource resources[] =
{
    /* Override some superclass' resource values */

  { XtNhSpace, XtCHSpace, XtRDimension, sizeof(Dimension),
    OFFSET(row_column.h_space), XtRImmediate, 0 },

  { XtNvSpace, XtCVSpace, XtRDimension, sizeof(Dimension),
    OFFSET(row_column.v_space), XtRImmediate, 0 },

  { XtNhPad, XtCHPad, XtRDimension, sizeof(Dimension),
    OFFSET(row_column.h_pad), XtRImmediate, 0 },

  { XtNvPad, XtCVPad, XtRDimension, sizeof(Dimension),
    OFFSET(row_column.v_pad), XtRImmediate, 0 },

  { XtNlayoutType, XtCLayoutType, XtROlDefine, sizeof(OlDefine),
    OFFSET(row_column.layout_type), XtRImmediate, (XtPointer)OL_FIXEDCOLS },

  { XtNmeasure, XtCMeasure, XtRInt, sizeof(int),
    OFFSET(row_column.measure), XtRImmediate, (XtPointer)((int) 1) },

  { XtNsameHeight, XtCSameHeight, XtROlDefine, sizeof(OlDefine),
    OFFSET(row_column.same_height), XtRImmediate, (XtPointer) OL_ROWS },

  { XtNsameWidth, XtCSameWidth, XtROlDefine, sizeof(OlDefine),
    OFFSET(row_column.same_width), XtRImmediate, (XtPointer) OL_COLUMNS },

    /* Specify default values for this class */

  { XtNexclusives, XtCExclusives, XtRBoolean, sizeof(Boolean),
    OFFSET(list.exclusive_settings), XtRImmediate, (XtPointer) True },

  { XtNformat, XtCFormat, XtRString, sizeof(String),
    OFFSET(list.format), XtRString, OL_LIST_DEFAULT_FORMAT },

  { XtNitemVisibility, XtCCallback, XtRCallback, sizeof(XtPointer),
    OFFSET(list.visibility), XtRCallback, NULL },

  { XtNitemsLimitExceeded, XtCCallback, XtRCallback, sizeof(XtPointer),
    OFFSET(list.limit_exceeded), XtRCallback, NULL },

  { XtNnoneSet, XtCNoneSet, XtRBoolean, sizeof(Boolean),
    OFFSET(list.none_set), XtRImmediate, (XtPointer) False },

  { XtNviewHeight, XtCViewHeight, XtRCardinal, sizeof(Cardinal),
    OFFSET(list.view_height), XtRImmediate, (XtPointer)4 },

  { XtNmaintainView, XtCMaintainView, XtRBoolean, sizeof(Boolean),
    OFFSET(list.maintain_view), XtRImmediate, (XtPointer)False },
};

				/* Define Resources for sub-objects	*/
#undef OFFSET
#define OFFSET(field) XtOffsetOf(FlatListItemRec, list.field)

static OLconst Boolean def_false = False;	/* needed for req resc */

static XtResource item_resources[] =
{
  { XtNclientData, XtCClientData, XtRPointer, sizeof(XtPointer),
    OFFSET(client_data), XtRPointer, NULL },

  { XtNdblSelectProc, XtCCallbackProc, XtRCallbackProc, sizeof(XtCallbackProc),
    OFFSET(dbl_select_proc), XtRCallbackProc, NULL },

  { XtNdragCursorProc, XtCCallbackProc, XtRCallbackProc, sizeof(XtCallbackProc),
    OFFSET(cursor_proc), XtRCallbackProc, NULL },

  { XtNdropProc, XtCCallbackProc, XtRCallbackProc, sizeof(XtCallbackProc),
    OFFSET(drop_proc), XtRCallbackProc, NULL },

  { XtNformatData, XtCFormatData, XtRPointer, sizeof(XtPointer),
    OFFSET(field_data), XtRPointer, NULL },

  { XtNselectProc, XtCCallbackProc, XtRCallbackProc, sizeof(XtCallbackProc),
    OFFSET(select_proc), XtRCallbackProc, NULL },

  { XtNset, XtCSet, XtRBoolean, sizeof(Boolean),
    OFFSET(selected), XtRBoolean, (XtPointer) &def_false },

  { XtNunselectProc, XtCCallbackProc, XtRCallbackProc,
    sizeof(XtCallbackProc), OFFSET(unselect_proc), XtRCallbackProc, NULL },
};

#undef OFFSET

    /* Specify resources that we want the flat class to manage internally if
       the application doesn't put them in their item-fields list.
    */
static OlFlatReqRsc required_resources[] = {
  { XtNset }
};

/***************************widget*class*record***************************

    Define Class Record structure to be initialized at Compile time
*/

FlatListClassRec flatListClassRec = {
    {
	(WidgetClass)&flatRowColumnClassRec,	/* superclass		*/
	"FlatList",				/* class_name		*/
	sizeof(FlatListRec),			/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	NULL, /* See ClassInitialize */		/* initialize		*/
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
	Resize,					/* resize		*/
	Redisplay,				/* expose		*/
	NULL, /* See ClassInitialize */		/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	XtInheritAcceptFocus,			/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_offsets	*/
	XtInheritTranslations,			/* tm_table		*/
	XtInheritQueryGeometry,			/* query_geometry	*/
	NULL,					/* display_accelerator	*/
	NULL					/* extension		*/
    }, /* End of Core Class Part Initialization */
    {
        True,					/* focus_on_select	*/
	XtInheritHighlightHandler,		/* highlight_handler	*/
	XtInheritTraversalHandler,		/* traversl_handler	*/
	NULL,					/* register_focus	*/
	XtInheritActivateFunc,			/* activate		*/
	event_procs,				/* event_procs		*/
	XtNumber(event_procs),		/* num_event_procs, See ClassInit */
	OlVersion,				/* version		*/
	(XtPointer)NULL,			/* extension		*/
	{
	    NULL,				/* dynamic resources	*/
	    0					/* num_resources	*/
	},					/* dynamic_data		*/
	XtInheritTransparentProc		/* transparent_proc	*/
    },	/* End of Primitive Class Part Initialization */
    {
	(XtPointer)NULL,			/* extension		*/
	True,					/* transparent_bg	*/
	XtOffsetOf(FlatListRec, default_item),	/* default_offset	*/
	sizeof(FlatListItemRec),		/* rec_size		*/
	item_resources,				/* item_resources	*/
	XtNumber(item_resources),		/* num_item_resources	*/
	required_resources,			/* required_resources	*/
	XtNumber(required_resources),		/* num_required_resources*/
          
		/*
		 * See ClassInitialize for procedures
		 */

    }, /* End of Flat Class Part Initialization */
    {
	NULL,					/* no fields		*/
    }, /* End of FlatRowColumn Class Part Initialization */
    {
	NULL,					/* no_class_fields	*/
    }, /* End of FlatList Class Part Initialization */
};

/*************************public*class*definition*************************

    Public Widget Class Definition of the Widget Class Record
*/

WidgetClass flatListWidgetClass = (WidgetClass) &flatListClassRec;

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    AdjustTop- if either ViewHeight or NumSlots changes, Top may need
    adjusting
*/
static void
AdjustTop OLARGLIST((w))
    OLGRA( FlatListWidget,	w )
{
    int delta = Top(w) + ViewHeight(w) - NumSlots(w);

    if (delta > 0)
    {
	if (delta >= Top(w))
	    Top(w) = 0;

	else
	    Top(w) -= delta;
    }
}

/****************************procedure*header*****************************
    CallCallProc- convenience function to generate OlFlatCallData structure
	and call a callproc.  'callproc' must be non-null.

	"call_data" is NULL for select & unselect callprocs (which use
	OlFlatCallData alone for their call data).

	When "call_data" is non-NULL (drop callproc), make the 1st member in
	the calldata struct contain the OlFlatCallData.
*/
static void
CallCallProc OLARGLIST((w, indx, callproc, client_data, user_data, call_data))
    OLARG( FlatListWidget,	w )
    OLARG( Cardinal,		indx )
    OLARG( XtCallbackProc,	callproc )
    OLARG( XtPointer,		client_data )
    OLARG( XtPointer,		user_data )
    OLGRA( OlFlatDropCallData *,	call_data )
{
    OlFlatCallData *	item_data = GetItemData(w, indx, user_data);
    XtPointer		cd;

    if (call_data == NULL)
    {
	cd = (XtPointer)item_data;

    } else
    {
	call_data->item_data = *item_data;
	cd = (XtPointer)call_data;
    }

    (*callproc)((Widget)w, client_data, cd);
}

/****************************procedure*header*****************************
    CallSelectProc - call the items select call proc (when 'select' is True)
	or the unselect call proc (when 'select' is False).
*/
static void
CallSelectProc OLARGLIST((w, indx, select))
    OLARG( FlatListWidget,	w )
    OLARG( Cardinal,		indx )
    OLGRA( Boolean,		select )
{
    Arg			args[3];
    XtCallbackProc	callproc = NULL;
    XtPointer		client_data = NULL;
    XtPointer		user_data = NULL;

    /* Get the Item's appropriate callback */

    XtSetArg(args[0], (select) ? XtNselectProc : XtNunselectProc, &callproc);
    XtSetArg(args[1], XtNclientData, &client_data);
    XtSetArg(args[2], XtNuserData, &user_data);
    OlFlatGetValues((Widget)w, indx, args, 3);

    /* If callproc defined, call it */
    if (callproc != NULL)
	CallCallProc(w, indx, callproc, client_data, user_data, NULL);
}

/****************************procedure*header*****************************
    ComputeFieldSize - compute the size of a field.

	If the field is a string, call ComputTextSize.
	Otherwise, if the image or pixmap is NULL, return 0 height and 0 or
	the specified width.
	Otherwise, if not NULL, call the Olg Pixmap sizing routine.
*/
static void
ComputeFieldSize OLARGLIST((w, item, indx, return_width, return_height))
    OLARG( FlatListWidget,	w )
    OLARG( FlatListItem,	item )
    OLARG( int,			indx )
    OLARG( Dimension *,		return_width )
    OLGRA( Dimension *,		return_height )
{
    FlatListPart *	flp = FLPART(w);
    _OlgDevice *	pDev;

    if (flp->fields[indx].type == STRING)
    {
	ComputeTextSize(w, item, indx, return_width, return_height);

    } else
    {
	XtPointer data = UseLabel(w, item) ?
	    (XtPointer)item->flat.label_image : item->list.field_data[indx];

	if (data == NULL)
	{
	    *return_height = 0;
	    *return_width = (flp->fields[indx].width == UNSPECIFIED) ?
		0 : flp->fields[indx].width;
	} else
	{
	    OlgPixmapLbl lbl;	/* { NULL } -> no need to do this because */
				/* this may upset some compilers. 	  */

	    /* Call the Olg routine to size the image, bitmap or pixmap.
	       Stuff 'data' and 'type' in lbl.  Other fields are not needed.
	    */

	    if (flp->fields[indx].type == PL_IMAGE)
		lbl.label.image = (XImage *)data;
	    else
		lbl.label.pixmap = (Pixmap)data;

	    lbl.type = flp->fields[indx].type;

	    OlgSizePixmapLabel(XtScreen((Widget)w), NULL,
				      &lbl, return_width, return_height);
			     
	    /* If the field width is specified, use it instead. */
	    if (flp->fields[indx].width != UNSPECIFIED)
		*return_width = flp->fields[indx].width;
	}
    }

    pDev = (FPART(w)->pAttrs)->pDev;

	/* Pad field height BUT DON'T ASSUME verticalStroke is always one */
    *return_height += 2 * VERT_MARGIN * pDev->verticalStroke;
}

/****************************procedure*header*****************************
    ComputePadding- for each field, compute the padding in pixels which will
	preceed the field.

	Inter-field padding is specified by including spaces between
	conversion specifications in the format string: "  %s   %i %w"
	The padding in pixels depends on the font used for the item.  The
	maximum total padding for all items is stored in TotalPadding(w) and
	the number of pad char's preceeding each field is stored in the Field
	record.  So to compute the number of pixels that should preceed each
	field, "pixels per pad-char" should be multiplied by the number of
	pad char's associated with this field.  This is done as:
	    ( pre_pad_cnt (for this field) * TotalPadding (in pixels) ) /
		total pad char's in the format string

       to minimize the effect of integer division.

*/
static void
ComputePadding OLARGLIST((w))
    OLGRA( FlatListWidget,	w )
{
    Cardinal	pads = CountPads(Format(w));
    Field *	field;
    int		i;

    for (i = 0, field = w->list.fields; i < NumFields(w); i++, field++)
	field->pre_padding = (pads == 0) ? 0 :
	    (field->pre_pad_cnt * TotalPadding(w)) / pads;
}

/****************************procedure*header*****************************
    ComputeTextSize - compute the size of a field with text.  This is done by
	extracting text segments from the field string.  Segments are
	determined by newline characters and any field width or max string
	length specifications (see GetTextSegment).

	height = number of segments * height of font.
	width = MAX(segment lengths)
*/
static void
ComputeTextSize OLARGLIST((w, item, f_indx, return_width, return_height))
    OLARG( FlatListWidget,	w )
    OLARG( FlatListItem,	item )
    OLARG( int,			f_indx )	/* Field index */
    OLARG( Dimension *,		return_width )
    OLGRA( Dimension *,		return_height )
{
    FlatListPart *	flp = FLPART(w);
    Field *		field = &(flp->fields[f_indx]);
    Boolean		compute_width = (field->width == UNSPECIFIED);
    String		str;
    Dimension		height;
    Dimension		width;
    Dimension		field_pixel_width;
    unsigned short	len;
    unsigned short	prev_len;
    String		seg;

    width = (compute_width) ? 0 : field->width * FontWidth(w, item);

    /* Get string from field_data or label */
    str = UseLabel(w, item) ?
	item->flat.label : (String)item->list.field_data[f_indx];

    if ((field->max != 0) && (str != NULL) && (*str != '\0'))
    {
	/* Get char len <= max */
	len = GetMaxCharLen(FontList(w, item), str, field->max);

	/* Compute pixel width of field from specified field->width. */
	field_pixel_width = (compute_width) ?
	    UNSPECIFIED : field->width * FontWidth(w, item);

	height = 0;

	while ((*str != '\0') && (len != 0))
	{
	    prev_len = len;

	    seg = GetTextSegment(w, item, str, field_pixel_width,
				 field->wrap, &len);

	    height += FontHeight(w, item);

	    if ((compute_width) && (seg > str))
	    {
		/* The font_list should be gotten from the item !! */
		Dimension	seg_width;

		seg_width = (FontList(w, item) == NULL) ?
		    XTextWidth(Font(w, item), str, seg - str)
			: OlTextWidth(FontList(w, item),
				      (unsigned char *)str, seg-str);

		if (seg_width > width)
		    width = seg_width;
	    }

	    str += (prev_len - len);
	}

    } else {
	height = FontHeight(w, item);
    }

    *return_width = width;
    *return_height = height;
}

/****************************procedure*header*****************************
    CalibrateVSBar- calibrate the vertical scrollbar by setting the proportion
	length indicator, sliderMax and sliderMin.  The scrollbar is made
	insensitive if the number of slots is less then the view height.
*/
static void
CalibrateVSBar OLARGLIST((w))
    OLGRA( FlatListWidget,	w)
{
    FlatListPart *	flp = FLPART(w);
    Arg			arg[4];
    Cardinal		cnt = 0;

    if (VScrollable(w))		/* view_height < num_slots */
    {
	XtSetArg(arg[cnt], XtNsensitive, True); cnt++;
	XtSetArg(arg[cnt], XtNproportionLength, ViewHeight(w)); cnt++;
	XtSetArg(arg[cnt], XtNsliderMax, NumSlots(w)); cnt++;
	XtSetArg(arg[cnt], XtNsliderValue, Top(w)); cnt++;

    } else			/* view_height >= num_slots */
    {
	XtSetArg(arg[cnt], XtNsensitive, False); cnt++;

	if (EmptyList(w))
	{
	    XtSetArg(arg[cnt], XtNproportionLength, 1); cnt++;
	    XtSetArg(arg[cnt], XtNsliderMax, 1); cnt++;
	    XtSetArg(arg[cnt], XtNsliderValue, 0); cnt++;

	} else
	{
	    XtSetArg(arg[cnt], XtNproportionLength, NumSlots(w)); cnt++;
	    XtSetArg(arg[cnt], XtNsliderMax, NumSlots(w)); cnt++;
	    XtSetArg(arg[cnt], XtNsliderValue, Top(w)); cnt++;
	}
    }

    XtSetValues(flp->vSBar, arg, cnt);
}

/****************************procedure*header*****************************
    CharCnt- count the number of characters 'ch' in string 'str'.
*/
static Cardinal
CharCnt OLARGLIST((str, ch))
    OLARG( String,	str )
    OLGRA( char,	ch )
{
    register Cardinal cnt = 0;

    while (*str != '\0')
	if (*str++ == ch)
	    cnt++;

    return (cnt);
}

/****************************procedure*header*****************************
    DisplayList- draw all items between 'y' and 'y' + 'height' in the view.

	'consider_clear' is True when scrolling and False when expose
	(Redisplay).  When scrolling the list, any managed but *not* mapped
	(when_managed) items must be cleared.  Clearing an item which becomes
	"unmapped" is done once by Flat but must be re-done each time the
	view scroll's.  The protocol with Flat is that it does not call the
	DrawItem class proc for unmapped items.

	If 'consider_clear' is True, get the mapped state of the item and
	clear it's area if the item is not mapped.  Note that this would be
	unnecessary if scrolling employed "CopyArea".
*/
static void
DisplayList OLARGLIST((w, y, height, consider_clear))
    OLARG( FlatListWidget,	w )
    OLARG( Position,		y )	/* relative to top of window */
    OLARG( Dimension,		height )
    OLGRA( Boolean,		consider_clear )
{
    Cardinal	end_indx,
		start_indx;
    Cardinal	indx;
    Boolean	clear;
    Widget	widget = (Widget)w;

    /* Get index of last item to draw.  If it's "out of bounds", make
       it the last item.
    */
    end_indx = FGetIndex(w, y + height);	/* get last index to draw: */
    if (end_indx == OL_NO_ITEM)
	end_indx = NumItems(w) - 1;

    start_indx = FGetIndex(w, y+1);

		/* Adjust "focus_item" here because we don't
		 * want to be in a state, flat list has focus
		 * but it looks like no "focus" (e.g.,
		 * scroll or resize to smaller size)
		 */
    if ( FocusItem(widget) != OL_NO_ITEM &&
	(consider_clear == False &&		/* expose... */
		!InView(widget, FocusItem(widget)) ||
	(consider_clear == True  &&		/* scroll... */
		(FocusItem(widget)<start_indx || FocusItem(widget)>end_indx))) )
    {
	FocusItem(widget) = LastFocusItem(widget) = start_indx;
    }

    for (indx = start_indx; indx <= end_indx; indx++)
    {
	if (consider_clear)
	{
	    Arg		args[2];
	    Boolean	managed = True;
	    Boolean	mapped_when_managed = True;

	    XtSetArg(args[0], XtNmanaged, &managed);
	    XtSetArg(args[1], XtNmappedWhenManaged, &mapped_when_managed);
	    OlFlatGetValues((Widget)w, indx, args, XtNumber(args));

	    clear = managed && !mapped_when_managed;

	} else
	{
	    clear = False;
	}

	OlFlatRefreshItem((Widget)w, indx, clear);
    }
}

/****************************procedure*header*****************************
    DrawFields- this func is passed to OlDrawRectButton as the drawing routine
	to draw the button label.  The 'lbl' parameter to this routine is a
	pointer.  In addition to the label pointer, we also need the widget ID
	and the Flat item to draw the button label.  So DrawItem sets up 'lbl'
	to point to a 'LabelInfo' struct which has this additional info.

	For fields with strings, get text segments and draw each one.  For
	image fields, use the drawing proc returned from calling
	OlFlatSetupAttributes.
*/
/* ARGSUSED */
static void
DrawFields OLARGLIST((scr, win, rect_btn_attrs, x, y, width, height, fake_lbl, flags))
    OLARG( Screen *,	scr )			/* unused */
    OLARG( Drawable,	win )			/* unused */
    OLARG( OlgAttrs *,	rect_btn_attrs )	/* unused */
    OLARG( Position,	x )
    OLARG( Position,	y )
    OLARG( Dimension,	width )
    OLARG( Dimension,	height )
    OLARG( void *,	fake_lbl )
    OLGRA( OlBitMask,	flags )
{
    OlFlatDrawInfo *	di = ((LabelInfo *)fake_lbl)->draw_info;
    FlatListWidget	w = FLW(((LabelInfo *)fake_lbl)->widget);
    FlatListItem	item = FLI(((LabelInfo *)fake_lbl)->item);
    FlatItemPart *	fip = FIPART(item);
    int			cnt;		/* field cnt */
    Field *		field;		/* item field */
    Position		field_x;	/* horiz pos of next field */
    Dimension		field_width = 0;/* for lint */
    Position		line_y;		/* vert pos of next text line */
    String		str;
    unsigned short	len;		/* string length */
    unsigned short	prev_len;	/* previous string length */
    String		seg;		/* text segment */
    Boolean		use_label;	/* Use label data (or not) */
    OlgLabelProc	draw_proc;	/* to draw XImage fields */
    union {
	OlgTextLbl	text;
	OlgPixmapLbl	pixmap;
    } *lbl;
    OlgAttrs *		lbl_attrs;	/* to draw XImage fields */
    GC			gc;		/* to draw String fields */

    /* See if we should use label data.  (Check outside loop) */
    use_label = UseLabel(w, item);

    for (cnt = 0, field = w->list.fields, field_x = x;
	 cnt < NumFields(w);
	 cnt++, field++, field_x += field_width)
    {
	field_x += field->pre_padding;

	if (field_x >= (Position)(x + width))
	    break;

	field_width = _OlMin(field->max_width,
			     (Dimension)(x + width - field_x));

	/* Put label or image pointer in flat item part so that
	   SetupAttributes can be used to get label GC's.  If "using label
	   data", it's already there.
	*/
	if (!use_label)
	{
	    if (field->type == STRING)
	    {
		fip->label = (String)item->list.field_data[cnt];
		fip->label_image = NULL;

	    } else {		/* image, pixmap, bitmap */
		fip->label = NULL;
		fip->label_image = (XImage *)item->list.field_data[cnt];
	    }
	}

	OlFlatSetupAttributes ((Widget)w, (FlatItem)item, di, &lbl_attrs,
				(XtPointer *)&lbl, &draw_proc);

	if (field->type == STRING)
	{
	    str = fip->label;
	
	    if ((field->max != 0) && (str != NULL) && (*str != '\0'))
	    {
		/* Get char len <= max */
		len = GetMaxCharLen(FontList(w, item), str, field->max);

		/* borrowed from OlgDrawTextLabel  :-( */
		if (OlGetGui() == OL_MOTIF_GUI)  {
			if (flags & RB_SELECTED)
				gc = lbl->text.inverseGC;
			else
				gc = lbl->text.normalGC;
		}
		else if (((lbl->text.flags & TL_SELECTED) && !OlgIs3d()))  { 
			gc = lbl->text.inverseGC;
		}
		else  {
			gc = lbl->text.normalGC;
		}

		/* Get each text segment (line) and draw it */

		for (line_y = OlFontAscent(Font(w, item), FontList(w, item));
		     (line_y < (Position)height) &&
		     (*str != '\0') && (len != 0);
		     line_y += FontHeight(w, item))
		{
		    prev_len = len;

		    seg = GetTextSegment(w, item, str, field_width,
					 field->wrap, &len);
		
		    if (seg > str)
			if (lbl->text.font_list == NULL)
			    XDrawString(XtDisplay(w), XtWindow(w), gc,
					field->right_justify ?
					field_x + field_width -
					XTextWidth(lbl->text.font, str,
						   seg - str) : field_x,
					y + line_y, str, seg - str);
			else
			    OlDrawString(XtDisplay(w), XtWindow(w),
					 lbl->text.font_list, gc,
					 field->right_justify ?
					 field_x + field_width -
					 OlTextWidth(lbl->text.font_list,
						     (unsigned char *)str,
						     seg - str)
					 : field_x,
					 y + line_y, (unsigned char *)str,
					 seg - str);

		    str += (prev_len - len);
		}
	    }
	
	} else
	{
	    /* If this is a Pixmap, set the correct type in the label struct
	       and put the Pixmap where is belongs (SetupAttributes doesn't
	       handle Pixmaps for our case).
	    */
	    if (field->type != PL_IMAGE)
	    {
		lbl->pixmap.type = field->type;
		lbl->pixmap.label.pixmap = (Pixmap)fip->label_image;
	    }

	    (*draw_proc)(di->screen, di->drawable, lbl_attrs,
			 field_x, y, field_width, height, lbl);
	}
    }
}
/****************************procedure*header*****************************
    GetMaxCharLen- return number of char's found in string.
	Compute string length: MAX(strlen(str), field->max)
*/
static unsigned short
GetMaxCharLen OLARGLIST((font_list, str, max))
    OLARG( OlFontList *,	font_list )
    OLARG( String,		str )
    OLGRA( unsigned short,	max )
{
    unsigned short	len;

    len = _OlStrlen(str);

#ifdef I18N
    if ((len != 0) && (font_list != NULL))	/* handle multi-byte chars */
    {
	int	char_len;
	size_t	byte_len = len;

	/* iterate on each char: len counts char's */

	for (len = 0; (byte_len != 0) && (*str != '\0'); len++)
	    if ((char_len = mblen(str, byte_len)) == -1)
	    {
		OlWarning("invalid multi-byte character");
		break;
	    } else
	    {
		byte_len	-= char_len;	/* dec byte count */
		str		+= char_len;	/* point to next char */
	    }
    }
#endif

    if ((max != UNSPECIFIED) && (len > max))
	len = max;

    return (len);
}

/****************************procedure*header*****************************
    GetItemData- return a pointer to OlFlatCallData.  Convenience function
	for calling callprocs and callbacks.
*/
static OlFlatCallData *
GetItemData OLARGLIST((w, indx, data))
    OLARG( FlatListWidget,	w )
    OLARG( Cardinal,		indx )
    OLGRA( XtPointer,		data )
{
    static OlFlatCallData	item_data;
    FlatPart *			fp = FPART(w);

    item_data.item_index	= indx;
    item_data.items		= fp->items;
    item_data.num_items		= fp->num_items;
    item_data.item_fields	= fp->item_fields;
    item_data.num_item_fields	= fp->num_item_fields;
    item_data.user_data		= w->primitive.user_data;
    item_data.item_user_data	= data;

    return (&item_data);
}

/****************************procedure*header*****************************
    GetSelectedItems- support routine for generating itemVisibility
	call back.

	From 'start' to 'end', not within the range from 'constrain_start'
	to 'constrain_end', look for selected items.

	The scroll func sets up start/end and constrain_start/end to define
	the previous and current views (any intersection represents overlap
	between the two views).  Items in the current view that were not in
	the previous view have "entered".  Items in the previous view that
	were not in the current view have "left".

	If any candidate items are found, alloc 'items' here.  Alloc space
	for ViewHeight item indexes since no more than that many items can
	enter or leave the view.
*/
static Cardinal
GetSelectedItems OLARGLIST((w, start, end, constrain_start,
			    constrain_end, items))
    OLARG( FlatListWidget,	w )
    OLARG( Cardinal,		start)
    OLARG( Cardinal,		end )
    OLARG( Cardinal,		constrain_start)
    OLARG( Cardinal,		constrain_end )
    OLGRA( Cardinal **,		items )
{
    Cardinal	num_items;
    Cardinal	indx;

    num_items = 0;
    for (indx = start; indx <= end; indx++)
    {
	if (((indx < constrain_start) || (indx > constrain_end)) &&
	    IsSelected(w, indx))
	{
	    if (*items == NULL)
		*items = (Cardinal *)
		    XtMalloc(ViewHeight(w) * sizeof(Cardinal));
	
	    (*items)[num_items] = indx;
	    num_items++;
	}
    }

    return(num_items);
}

/****************************procedure*header*****************************
    GetTextSegment- return the end of a text segment.

	A segment is delimited:
	    - by a newline char or
	    - when the field width is exceeded by the text or
	    - if the end of the input string (str) is reached

	The length of str is decremented appropriately thru &len
*/
static String
GetTextSegment OLARGLIST((w, item, str, width, wrap, len))
    OLARG( FlatListWidget,	w )
    OLARG( FlatListItem,	item )
    OLARG( String,		str )
    OLARG( Dimension,		width )
    OLARG( Boolean,		wrap )
    OLGRA( unsigned short *,	len )
{
    String	end;
    short	field_width = width;		/* make it signed */
    Boolean	width_specified = (width != UNSPECIFIED);
    Boolean	is_multi_byte = (FontList(w, item) != NULL);
    int		ch_len;		/* char length for multi-byte chars */
    Boolean	prev_ws;	/* prev char was white space */
    String	word_end;	/* points beyond last word */

#define _CharWidth(font, ch) \
	( (((font)->per_char != NULL) && \
	 ((ch) >= (font)->min_char_or_byte2) && \
	 ((ch) <= (font)->max_char_or_byte2)) \
	 ? (font)->per_char[(ch) - (font)->min_char_or_byte2].width \
	 : (font)->max_bounds.width )

#ifdef I18N
#define CharWidth(font, font_list, ch, ch_len) \
    ( ((font_list) == NULL) ? _CharWidth(font, *(ch)) : \
     OlTextWidth(font_list, (unsigned char *)(ch), ch_len) )
#else
#define CharWidth(font, font_list, ch, ch_len) _CharWidth(font, *(ch))
#endif

    if (!is_multi_byte)
	ch_len = 1;

    word_end = NULL;
    prev_ws = True;

    for (end = str; (*len != 0) && (*end != '\0');
	 end += ch_len, (*len)--)
    {
	wchar_t	ch;
	
	if (!is_multi_byte)
	    ch = (wchar_t)*end;

	else if ((ch_len = mbtowc(&ch, end, MB_LEN_MAX)) == -1)
	{
	    OlWarning("invalid multi-byte character");
	    break;
	}

	if (iswspace(ch))
	{
	    if (!prev_ws)
	    {
		word_end = end;
		prev_ws = True;
	    }

	    if (ch == (wchar_t)'\n')
	    {
		/* Special case when '\n' terminates the string: if this is
		   the case and it's not the start of the string, don't dec
		   length. The 'if' below is a negation of this.
		*/
		if ( ! ((end[ch_len] == '\0') && (end != str)) )
		    (*len)--;

		break;

	    }
	} else
	    prev_ws = False;

	/* If width is specified and char doesn't fit, break: */
	if (width_specified)
	{
	    field_width -= CharWidth(Font(w, item), FontList(w, item),
				     end, ch_len);
	    if (field_width < 0)
		break;
	}
    }

    /* Now handle wrapping or clipping.  If end of string or length
       exhausted, return now.
    */
    if ((*end == '\0') || (*len == 0))
	return(end);

    /* If field_width is exhausted, consider wrapping vs clipping. */
    if (width_specified && (field_width < 0))
    {
	String s;

	if (wrap)
	{
	    /* If 'end' is in whitespace, remove whitespace from end of
	       previous word to beginning of next word.
	       Otherwise, move back to end of last word.
	    */
	    if (isspace(*end))
	    {
		/* Subtract from remaining length till beginning of
		   next word.
		*/
		for (s = end; (*len != 0) && (*s != '\0') && isspace(*s); s++)
		    (*len)--;

	    } else		/* segment doesn't end in whitespace */
	    {
		for (s = end - 1; s > str; s--)
		    if (isspace(*s))
		    {
			/* ws found: give back char's to remaining length */
			(*len) += end - (s + 1);

			break;
		    }
				/* couldn't find ws, so adjst word_end */
		if (s == str)
			word_end = end;
	    }

	    /* Point to end of previous word */
	    end = (word_end == NULL) ? str : word_end;
	}
	else			/* no wrapping */
	{
	    /* Clip text by subtracting from remaining length */
	    for (s = end; (*len != 0) && (*s != '\0'); s++)
	    {
		(*len)--;
	    
		if (*s == '\n')
		    break;
	    }
	}
    }

    return (end);

#undef CHAR_WIDTH
}

/****************************procedure*header*****************************
    _IsSelected- called by macro 'IsSelected' to return selected state of
	non-exclusives item.
*/
static Boolean
_IsSelected OLARGLIST((w, indx))
    OLARG( FlatListWidget,	w )
    OLGRA( Cardinal,		indx )
{
    Boolean	is_selected = False;
    Arg		args[1];

    XtSetArg(args[0], XtNset, &is_selected);
    OlFlatGetValues((Widget)w, indx, args, 1);

    return (is_selected);
}

/****************************procedure*header*****************************
    ItemsTouched- prepare for all items to be initialized.  Conditions:
			Number of items has changed.
			Number of fields has changed.
			Format string has changed.

	This occurs during Initialization (current == NULL) and can
	occur during SetValues (current == current item).

	This will not be called initially if the list is empty.

	The name of this function derives from the name of the old Flat
	class procedure.
*/
/* ARGSUSED */
static void
ItemsTouched OLARGLIST((current, new))
    OLARG( Widget,	current)
    OLGRA( Widget,	new)
{
    FlatListWidget	flw = FLW(new);
    FlatListPart *	flp = FLPART(new);

    /* Items about to be initialized... give metrics initial values */
    TotalHeight(new)	= 0;
    TotalPadding(new)	= 0;
    MinHeight(new)	= USHRT_MAX;
    MinFieldWidth(new)	= USHRT_MAX;

    /* Initialize list-wide members */
    SelectedItem(new)	= OL_NO_ITEM;
    Top(new)		= 0;	/* "initialize" top item in view */
    TopOffset(new)	= 0;	/* Offset of top slot */

    if (!EmptyList(new) &&
	(current == NULL || NumItems(current) != NumItems(new)))
	flp->y_offsets = (Position *)
	    XtRealloc((char *)flp->y_offsets, NumItems(new)*sizeof(Position));

    if (current == NULL)		/* Initialization */
    {
	ParseFormat(flw);
	if (!EmptyList(new))
		ItemOffset(new, 0) = 0;	/* Initialize offset of 1st item */

    } else				/* SetValues */
    {
	if (Format(current) != Format(new) || EmptyList(current))
	    ParseFormat(flw);

	/* Must notify ScrolledWindow if list has become NULL.  (Otherwise,
	   in the usual case, resizes and scrollbar calibration will be done
	   during item initialization.)
	*/
	if (EmptyList(new))
	{
	    if (InSWin(new))
		OlLayoutScrolledWindow((ScrolledWindowWidget)XtParent(XtParent(new)), False);

	} else if (EmptyList(current))
	{
	    ItemOffset(new, 0) = 0;	/* Initialize offset of 1st item */
	}
    }
}

/****************************procedure*header*****************************
    ItemWidth- return pixel width of item.
	item width = sum(max field widths) + total padding + horiz padding
*/
static Dimension
ItemWidth OLARGLIST((w))
    OLGRA( FlatListWidget,	w )
{
    register Dimension	width = 0;
    register int	i;
    unsigned		h_stroke;
    Dimension		h_thickness = (OlGetGui() == OL_OPENLOOK_GUI) ?
					0 : 2 * HighlightThickness(w);

    for (i = 0; i < NumFields(w); i++)
    {
	width += MaxFieldWidth(w, i);
    }

		/* NULL if it's an empty list initially */
    if (FPART(w)->pAttrs)
    {
	_OlgDevice *	pDev;

	pDev = (FPART(w)->pAttrs)->pDev;
	h_stroke = pDev->horizontalStroke;
    }
    else
    {
	h_stroke = OlScreenPointToPixel(OL_HORIZONTAL, 1, XtScreen((Widget)w));
    }
	
    
	/* BUT DON'T ASSUME horizontalStroke is always one */
    width += TotalPadding(w) + 2 * HORIZ_MARGIN * h_stroke +
		+ h_thickness;

    return (width);
}

/****************************procedure*header*****************************
    NonexcSelection- performs wipe-thru selection for nonexclusive settings.

	Direction and magnitude of motion is determined by the index over
	which the pointer is currently (indx) and the previous index it was
	over (flp->prev_indx).  Motion is also always considered relative to
	the starting index (flp->start_indx); that is, where the wipe-thru
	selection began.  The task is to determine which items should be
	selected and/or unselected.

	If the pointer is above start_indx and moving up or below start_indx
	and moving down, items are selected.

	If the pointer is above start_indx and moving down or below
	start_indx and moving up, items are unselected.

	If the pointer has crossed start_indx with this motion, items must be
	selected and unselected.
*/
static void
NonexcSelection OLARGLIST((w, indx))
    OLARG( FlatListWidget,	w )
    OLGRA( Cardinal,		indx )
{
    FlatListPart *	flp = FLPART(w);
    Cardinal		prev;
    Boolean		moving_up;
    Cardinal		select_start;
    int			select_cnt;
    Cardinal		unselect_start;
    int			unselect_cnt;

    /* Macros: is item On, Above or Below starting item */
#define AboveStart(flp, indx)		( (indx) < (flp)->start_indx )
#define OnOrAboveStart(flp, indx)	( (indx) <= (flp)->start_indx )
#define BelowStart(flp, indx)		( (indx) > (flp)->start_indx )
#define OnOrBelowStart(flp, indx)	( (indx) >= (flp)->start_indx )

    /* Macro: has there been a change in direction ? */
#define ChgDirect(flp, moving_up) \
		( ((moving_up) && ((flp)->prev_motion == DOWN)) || \
		  (!(moving_up) && ((flp)->prev_motion == UP)) )

    prev	= flp->prev_indx;
    moving_up	= ( indx < prev );
    select_start= unselect_start = prev;
    select_cnt	= unselect_cnt = 0;

    if (AboveStart(flp, indx) && moving_up)
    {
	if (OnOrAboveStart(flp, prev))
	{
	    /* If prev is on starting item, there might not be prev_motion yet
	       so check for this.
	    */
	    if ((flp->prev_motion != NO_MOTION) && ChgDirect(flp, moving_up))
		select_start--;

	    select_cnt = indx - select_start;
	}
	else		/* prev index is below start */
	{
	    if (ChgDirect(flp, moving_up))
		unselect_start--;

	    unselect_cnt = flp->start_indx - unselect_start;
	    select_start = flp->start_indx - 1;
	    select_cnt = indx - select_start;
	}

    }
    else if (OnOrAboveStart(flp, indx) && !moving_up)
    {
	/* Since we're on or above the starting item and we're going down, the
	   prev index must also be above the starting item.
	*/
	if (ChgDirect(flp, moving_up))
	    unselect_start++;

	unselect_cnt = indx - unselect_start;
    }
    else if (OnOrBelowStart(flp, indx) && moving_up)
    {
	/* Since we're on or below the starting item and we're going up, the
	   prev index must also be below the starting item.
	*/
	if (ChgDirect(flp, moving_up))
	    unselect_start--;

	unselect_cnt = indx - unselect_start;
    }
    else	/* Below & moving down */
    {
	if (OnOrBelowStart(flp, prev))
	{
	    /* If prev is on starting item, there might not be prev_motion yet
	       so check for this.
	    */
	    if ((flp->prev_motion != NO_MOTION) && ChgDirect(flp, moving_up))
		select_start++;

	    select_cnt = indx - select_start;
	}
	else		/* prev index is above start */
	{
	    if (ChgDirect(flp, moving_up))
		unselect_start++;

	    unselect_cnt = flp->start_indx - unselect_start;
	    select_start = flp->start_indx + 1;
	    select_cnt = indx - select_start;
	}
    }

    /* If the prev item is below the start and we're going up or it's above
	and we're going down (that is, if we're moving so that items would
	be unselected), clear the current item.
    */
    if ((BelowStart(flp, prev) && moving_up) ||
	 (AboveStart(flp, prev) && !moving_up))
    {
	/* If there's a change in direction, prev will not be unselected
	   (and redisplayed) so clear it.  Otherwise, prev will be
	   unselected (and redisplayed) so make sure it's not seen as
	   'current'.
	*/
	CurrentItem(w) = OL_NO_ITEM;

	/* Now when redisplayed (via ItemSetValues), prev will be correctly
	   drawn as unselected.  If direction has changed though, no
	   ItemSetValues will be made to unselect the item so clear the item
	   here.
	*/
	if (ChgDirect(flp, moving_up))
	    OlFlatRefreshItem((Widget)w, prev, True);
    }

    /* Update prev_motion */
    flp->prev_motion = moving_up ? UP : DOWN;

    /* Select and/or Unselect items.  Items are always unselected first.  A
       count of zero means no items to be [un]selected.
    */
    if (unselect_cnt != 0)
	SelectItems(w, unselect_start, unselect_cnt, False);

    if (select_cnt != 0)
	SelectItems(w, select_start, select_cnt, True);

    /* Item under pointer is set to current by calling proc */

#undef AboveStart
#undef OnOrAboveStart
#undef BelowStart
#undef OnOrBelowStart
#undef ChgDirect
}

/****************************procedure*header*****************************
    ParseFormat- parse printf-style format string and populate vector of
    'Field' structs.
*/
static void
ParseFormat OLARGLIST((w))
    OLGRA( FlatListWidget, w )
{
    FlatListPart *	flp = FLPART(w);
    Field *		field;
    Cardinal		num_fields;
    String		fmt;
    long		num;
    String		tmp;
    int			i;

    /* Allocate Field vector, 1 element per field conversion (%) */

    num_fields = CountFields(Format(w));

    if (num_fields == 0)
	OlVaDisplayErrorMsg(XtDisplay((Widget)w),	/* doesn't return */
			    OleNbadFormat,
			    OleTinvalidFormat,
			    OleCOlToolkitError,
			    OleMbadFormat_invalidFormat, /* message */
			    XtName((Widget)w),			/* arg 1 */
			    OlWidgetToClassName((Widget)w));	/* arg 2 */
			    
    if (num_fields != NumFields(w))
    {
	NumFields(w) = num_fields;

	/* re-alloc space for field info */
	flp->fields = (Field *)XtRealloc((char *)flp->fields,
					 num_fields * sizeof(Field));
    }

    /* Now parse the format string and fill in Field rec's from each
       conversion specification.
    */
    fmt = Format(w);

    for (i = 0, field = flp->fields; i < num_fields; i++, field++)
    {
	/* init values not derived from format string */
	field->max_width = 0;
	field->pre_padding = 0;

	/* count spaces that precede this field */
	for (field->pre_pad_cnt = 0; *fmt == ' '; field->pre_pad_cnt++)
	    fmt++;

	if (*fmt != DELIMITER)
	{
	    OlVaDisplayWarningMsg(XtDisplay((Widget)w),
				  OleNnoDelimiter,
				  OleTinvalidFormat,
				  OleCOlToolkitWarning,
				  OleMnoDelimiter_invalidFormat,
				  XtName((Widget)w),
				  OlWidgetToClassName((Widget)w));

	    while ((*fmt != '\0') && (*fmt != DELIMITER))
		fmt++;

	    if (*fmt == '\0')
		continue;
	}

	fmt++;				/* Jump over delimiter */

	/* Determine justification */
	if (*fmt == RIGHT_JUSTIFY)
	{
	    field->right_justify = True;
	    fmt++;

	} else {
	    field->right_justify = False;
	}

	/* Get width */
					/* attempt conversion to long */
	num = strtol(fmt, &tmp, 10);
	
	if (tmp != fmt)			/* tmp updated, conversion succeeded */
	{
	    /* Check for out-of-range value */
	    if ((num < 0) || (num >= (unsigned long)USHRT_MAX))
	    {
		OlVaDisplayWarningMsg(XtDisplay((Widget)w),
				      OleNbadWidth,
				      OleTinvalidFormat,
				      OleCOlToolkitWarning,
				      OleMbadWidth_invalidFormat,
				      XtName((Widget)w),
				      OlWidgetToClassName((Widget)w),
				      num, USHRT_MAX);
		num = UNSPECIFIED;
	    }

	    field->width = num;	/* store converted width */
	    fmt = tmp;		/* update pos in format string */

	} else {
	    field->width = UNSPECIFIED;
	}

	/* Get max */

	if (*fmt == '.')
	{
	    fmt++;				/* jump over '.' */
	    num = strtol(fmt, &tmp, 10); /* attempt conversion to long */
	
	    /* Here, checking for a successful conversion is unnecessary.  An
	       unsuccessful conversion returns 0 which is what a
	       "non-existent" max spec should yield.  Range checking is still
	       needed, however.
	    */
	    if ((num < 0) || (num >= (unsigned long)USHRT_MAX))
	    {
		OlVaDisplayWarningMsg(XtDisplay((Widget)w),
				      OleNbadMax,
				      OleTinvalidFormat,
				      OleCOlToolkitWarning,
				      OleMbadMax_invalidFormat,
				      XtName((Widget)w),
				      OlWidgetToClassName((Widget)w),
				      num, USHRT_MAX);
		num = UNSPECIFIED;
	    }

	    field->max = num;
	    fmt = tmp;

	} else {
	    field->max = UNSPECIFIED;
	}

	switch (*fmt)
	{
	case 'b' :
	    field->type = PL_BITMAP;
	    break;

	case 'i' :
	    field->type = PL_IMAGE;
	    break;

	case 'p' :
	    field->type = PL_PIXMAP;
	    break;

	case 's' :
	    field->wrap = False;
	    field->type = STRING;
	    break;

	case 'w' :
	    if (field->width == UNSPECIFIED)
	    {
		OlVaDisplayWarningMsg(XtDisplay((Widget)w),
				      OleNcantWrap,
				      OleTinvalidFormat,
				      OleCOlToolkitWarning,
				      OleMcantWrap_invalidFormat,
				      XtName((Widget)w),
				      OlWidgetToClassName((Widget)w));
		field->wrap = False;

	    } else
	    {
		field->wrap = True;
	    }

	    field->type = STRING;
	    break;

	default:
	    OlVaDisplayWarningMsg(XtDisplay((Widget)w),
				  OleNnoConvChar,
				  OleTinvalidFormat,
				  OleCOlToolkitWarning,
				  OleMnoConvChar_invalidFormat,
				  XtName((Widget)w),
				  OlWidgetToClassName((Widget)w));
	    break;
	}

	fmt++;		/* inc past conversion char */

	if ((field->type == STRING) && (field->max != UNSPECIFIED))
	{
	    OlVaDisplayWarningMsg(XtDisplay((Widget)w),
				  OleNcantUseMax,
				  OleTinvalidFormat,
				  OleCOlToolkitWarning,
				  OleMcantUseMax_invalidFormat,
				  XtName((Widget)w),
				  OlWidgetToClassName((Widget)w));
	    field->max = UNSPECIFIED;
	}
    }
}

/****************************procedure*header*****************************
    ScrollList- scroll the list by updating the top item in view and
    redisplaying the list.

    When scrolling in bottom of list, bottom of view must be cleared.

    If interested in itemVisibility callback, compute items that have entered
    and left the view and call callproc.
*/
static void
ScrollList OLARGLIST((w, diff))
    OLARG( FlatListWidget,	w )
    OLGRA( int,			diff )
{
    Top(w) += diff;				/* Adjust Top */
    TopOffset(w) -= diff * MinHeight(w);	/* Update offset of Top */

    /* Everything else requires the widget is realized */
    if (!XtIsRealized((Widget)w))
	return;

	/* round up */
    DisplayList(w, 0,
		(ViewHeight(w) + BottomIsPartial(w)) * MinHeight(w) - 1,
		True);

    /* If the n'th or n-1'st slot has just scrolled in, consider clearing
       the area between the bottom of the last item and the bottom of the
       window.  (This should only need to be done for the n'th slot but doing
       this also leaves garbage at bottom of list (don't know why).
    */
    if (Top(w) >= NumSlots(w) - ViewHeight(w) - 1)
    {
	Position y = TotalHeight(w) - YTop(w);

	if (y < (Position)w->core.height)
	    XClearArea(XtDisplay((Widget)w), XtWindow((Widget)w),
		       0, y, w->core.width, w->core.height - y, False);
    }

    /* If interested in visibility changes, compute top and bottom items
       in prev and current views and call GetSelectedItems to find any
       selected items that have entered or left the view.

       We know now not to do this if exclusives and no item set.
    */
    if ((XtHasCallbacks((Widget)w, XtNitemVisibility) == XtCallbackHasSome) &&
	!(Exclusives(w) && !ExcItemIsSelected(w)))
    {
	OlFListItemVisibilityCD	cd;
	Cardinal		prev_top;
	Cardinal		prev_bottom;
	Cardinal		current_top;
	Cardinal		current_bottom;
	Position		y;

#define YToTop(w, y)	FGetIndex(w, (y) + VERT_MARGIN)
#define YToBottom(w, y)	FGetIndex(w, (y) - VERT_MARGIN + ViewPixelHeight(w))

	/* Compute 'y' (relative to the view) of current Top and top &
	   bottom items in current view.
	*/
	y = 0;
	current_top = YToTop(w, y);
	current_bottom = YToBottom(w, y);

	/* Compute 'y' (relative to the view) of previous Top and top &
	   bottom items in previous view.
	*/
	y =  -diff * MinHeight(w);
	prev_top = YToTop(w, y);
	prev_bottom = YToBottom(w, y);

	cd.leaves = NULL;	/* storage alloc'ed by GetSelectedItems */
	cd.num_leaves = GetSelectedItems(w, prev_top, prev_bottom,
					 current_top, current_bottom,
					 &cd.leaves);

	cd.enters = NULL;
	cd.num_enters = (Exclusives(w) && (cd.num_leaves == 1)) ? 0 :
	    GetSelectedItems(w, current_top, current_bottom,
			     prev_top, prev_bottom, &cd.enters);

	if ((cd.num_enters != 0) || (cd.num_leaves != 0))
	{
	    cd.item_data = GetItemData(w, OL_NO_ITEM, NULL);

	    XtCallCallbacks((Widget)w, XtNitemVisibility, &cd);

	    if (cd.num_enters != 0)
		XtFree((char *)cd.enters);

	    if (cd.num_leaves != 0)
		XtFree((char *)cd.leaves);
	}
#undef YToTop
#undef YToBottom
    }
}

/****************************procedure*header*****************************
    SelectItem- [un]select the item whose index is 'indx'.  Item is selected
    if 'select' is True or unselected if 'select' is False.
*/
static void
SelectItem OLARGLIST((w, indx, select))
    OLARG( FlatListWidget,	w )
    OLARG( Cardinal,		indx )
    OLGRA( Boolean,		select )
{
    Arg			args[1];
    Cardinal		other;		/* other exc item set (if any) */

    /* An easy optimization for exclusives: if 'select' is not different
       from current state of item, return now.
    */
    if (Exclusives(w))
    {
	Boolean set_state = (indx == SelectedItem(w));

	if ( !(set_state ^ select) )
	    return;
    }

    /* For exclusives, any other selected item will be unseleted by
       FlatSetValues.  Save that item now (if any) so that unselect callbacks
       can be called.
    */
    other = (Exclusives(w) && select) ? SelectedItem(w) : OL_NO_ITEM;

    XtSetArg(args[0], XtNset, select);
    OlFlatSetValues((Widget)w, indx, args, 1);

    /* If 'previous' is defined, call unselect proc for it. */
    if (other != OL_NO_ITEM)
	CallSelectProc(w, other, False);

    /* Now call [un]select proc for this item that was just [un]selected */
    CallSelectProc(w, indx, select);
}

/****************************procedure*header*****************************
    SelectItems- [un]select 'cnt' item(s) starting at 'start'.
	'cnt' is assumed to be non-zero.  A negative 'cnt' [un]selects items
	in descending order, a positive 'cnt' [un]selects items in ascending
	order.

	'select' determines whether item is selected (True) or unselected.

	SelectItems must be used for nonexclusives (since multiple items may
	be [un]selected).  (SelectItems may also be used for exclusives with a
	count of 1).

	When selecting ('select' == True), an optimization can be made to see
	if the item is already selected. 
*/
static void
SelectItems OLARGLIST((w, start, cnt, select))
    OLARG( FlatListWidget,	w )
    OLARG( Cardinal,		start )
    OLARG( int,			cnt )
    OLGRA( Boolean,		select )
{
    Cardinal		indx;
    int			inc;
    Arg			args[7];
    Cardinal		arg_cnt;
    Boolean		selected;
    XtCallbackProc	callproc = NULL;
    XtPointer		client_data = NULL;
    XtPointer		user_data = NULL;
    Boolean		sensitive;
    Boolean		managed;
    Boolean		mapped;

    arg_cnt = 0;
    XtSetArg(args[arg_cnt], XtNsensitive, &sensitive); arg_cnt++;
    XtSetArg(args[arg_cnt], XtNmanaged, &managed); arg_cnt++;
    XtSetArg(args[arg_cnt], XtNmappedWhenManaged, &mapped); arg_cnt++;

    /* As part of the optimization, if the request is to select, get the
       current state of the item.
    */
    if (select)
    {
	/* Get the Item's current state and select callback */
	XtSetArg(args[arg_cnt], XtNset, &selected); arg_cnt++;
	XtSetArg(args[arg_cnt], XtNselectProc, &callproc); arg_cnt++;
	XtSetArg(args[arg_cnt], XtNclientData, &client_data); arg_cnt++;
	XtSetArg(args[arg_cnt], XtNuserData, &user_data); arg_cnt++;
    }

    for (indx = start, inc = (cnt < 0) ? -1 : 1; cnt != 0;
	 cnt -= inc, indx += inc)
    {
	sensitive = True;
	managed = True;
	mapped = True;

	OlFlatGetValues((Widget)w, indx, args, arg_cnt);

	/* Perform selection only if sensitive, etc */

	if (sensitive && managed && mapped)
	{
	    /* Here's the optimization for selecting items.
	       
	       If the request is to select the item and the item is already
	       selected, do nothing.  If the item is not selected, do the
	       usual stuff (taken from SelectItem and CallSelectProc :-( )
	       
	       If the request is to unselect the item, call SelectItem to
	       unselect it.  The optimization doesn't make sense when the
	       request is to unselect the item since, for nonexclusives, the
	       item is known to be selected.
	       
	    */
	    if (select)
	    {	
		if (!selected)
		{
		    Arg set_args[1];

		    XtSetArg(set_args[0], XtNset, True);
		    OlFlatSetValues((Widget)w, indx, set_args, 1);

		    if (callproc != NULL)
			CallCallProc(w, indx, callproc,
				     client_data, user_data, NULL);
		}
	    } else
	    {
		SelectItem(w, indx, False);
	    }
	}
    }
}

/****************************procedure*header*****************************
    SetCurrentItem- set the index of the current item and draw it.  For
    exclusives, "undraw" the "currently current" item first.
*/
static void
SetCurrentItem OLARGLIST((w, indx))
    OLARG( FlatListWidget,	w )
    OLGRA( Cardinal,		indx )
{
    Cardinal		other;

    if (CurrentItem(w) == indx)
	return;

    other = (CurrentItem(w) == OL_NO_ITEM) ? SelectedItem(w) : CurrentItem(w);
    CurrentItem(w) = indx;

    if (Exclusives(w) && (other != OL_NO_ITEM))
	OlFlatRefreshItem((Widget)w, other, True);

    OlFlatRefreshItem((Widget)w, indx, False);
}

/****************************procedure*header*****************************
    SetViewSize- set the size of the SWin view.

    This routine is called when the view_height changes or after a new list
    of items has been set (as during Initiazation & SetVaules).  It must be
    called *after* all items have been initialized so that min_height is
    valid (view_height is specified in 'min_height' units).

    It is assumed that the FList widget is in a scrolled window (ie., the
    called has done InSWin(w)).

    Setting the view size via the SWin resources, XtNviewWidth &
    XtNviewHeight, should work but doesn't.  Instead, like the TextEdit
    widget, we are forced to use XtNwidth & XtNheight :-(

    The step size for the horiz scrollbar is also set here for convenience.
    Since it depends on the item field sizes, it too must be set when the
    list is changed.
*/
static void
SetViewSize OLARGLIST((w))
    OLGRA( FlatListWidget,	w )
{
    Widget		sw = SWin(w);
    OlSWGeometries	swg;
    Arg			args[3];
    int			step_size;
    Dimension		view_height;
    Dimension		view_width;

    swg = GetOlSWGeometries((ScrolledWindowWidget)sw);

	/* For an empty list, use the font height as the
	 * view height unit (a reasonable fallback?).
	 * One additional check might be to see of there is
	 * a "string" fields specified in the format.  If no
	 * strings are used, maybe default height to the existing
	 * widget height.
	 */
    if (EmptyList(w))
    {
	step_size = 1;
	view_height = EmptyHeight(w);
	view_width = ItemWidth(w);
    } else
    {
	Dimension	extra_width;

	step_size = MinFieldWidth(w);

	view_height = ViewPixelHeight(w);

		/* note that, it seems to me there is a
		 * bug in Scrolled window.
		 * Scrolled Window can't grand new XtNwidth
		 * if this new value is less than old value.
		 */
	if (ViewHeight(w) < NumSlots(w))
		extra_width = swg.vsb_width;
	else
		extra_width = 0;
	view_width = ItemWidth(w) + extra_width;
    }

    if (view_height < swg.vsb_min_height)
	view_height = swg.vsb_min_height;

    if (view_width < swg.hsb_min_width)
	view_width = swg.hsb_min_width;

    XtSetArg(args[0], XtNhStepSize, step_size);
    XtSetArg(args[1], XtNheight, view_height + 2 * swg.bb_border_width);
    XtSetArg(args[2], XtNwidth, view_width + 2 * swg.bb_border_width);
    XtSetValues(sw, args, 3);
}

/****************************procedure*header*****************************
    PostSWinInterface- called by ScrolledWin to notify the geometry
	changes after compute_geometries. This can happen when "one"
	of the scrollbars turned into MINIMUM/ABBREV scrollbar. In this
	case, scrolled window will give up spaces and need to notify
	its child so that "child" can do proper changes afterward.
*/
static void
PostSWinInterface OLARGLIST((widget, swg))
    OLARG( Widget,		widget )
    OLGRA( OlSWGeometries *,	swg )
{
	FlatListWidget	w = FLW(widget);

	if (!EmptyList(w))
	{
			/* Recompute view height, rounding down. */
		ViewHeight(w) = swg->sw_view_height / MinHeight(w);
		BottomIsPartial(w) = ((swg->sw_view_height % MinHeight(w))!=0);

		if (w->list.need_vsb)
			CalibrateVSBar(w);
	}
} /* end of PostSWinInterface */

/****************************procedure*header*****************************
    SWinInterface- called by ScrolledWin to compute geometry.  It's not
	apparent when and how often this is called.  From the source it's
	called whenever OlLayoutScrolledWindow() is called which is at
	Realize, InsertChild, ChildResize, GeometryManager, and others :-(

      Must set:
	bbc_width/_height	view size <= to ScrolledWin view size minus
				any scrollbars.
	bbc_real_width/_height	'virtual' size of list.
	force_hsb/_vsb		to force the display of scrollbar(s).
*/
static void
SWinInterface OLARGLIST((widget, swg))
    OLARG( Widget,		widget )
    OLGRA( OlSWGeometries *,	swg )
{
    FlatListWidget	w = FLW(widget);

#define SUB(A,B) ((A) > (B)? (A)-(B) : 0)

    /* If list is empty, simply use the widget's size */
    if (EmptyList(w))
    {
	swg->bbc_width = swg->bbc_real_width = swg->force_vsb ?
	    SUB(swg->sw_view_width, swg->vsb_width) : swg->sw_view_width;
	swg->bbc_height = swg->bbc_real_height =
	    SUB(EmptyHeight(w), (swg->force_hsb ? swg->hsb_height : 0));

    } else
    {
	Dimension	view_width, view_height;
	Dimension	list_width, list_height;

	view_width	= swg->sw_view_width;
	view_height	= swg->sw_view_height;
	list_width	= ItemWidth(w);
	list_height	= NumSlots(w) * MinHeight(w);

#define TooNarrow()	( view_width < list_width )
#define TooShort()	( view_height < list_height )

	if (swg->force_vsb && swg->force_hsb)
	{
	    view_width = SUB(view_width, swg->vsb_width);
	    view_height = SUB(view_height, swg->hsb_height);
					/* too short to view items? */
	} else if (swg->force_vsb || TooShort())
	{
	    swg->force_vsb = True;	/* need vert scrolling */
	    view_width = SUB(view_width, swg->vsb_width);
					/* too narrow to view an item? */
	    if (TooNarrow())
	    {
		swg->force_hsb = True;	/* need horiz scrolling */
		view_height = SUB(view_height, swg->hsb_height);
	    }
					/* too narrow to view an item? */
	} else if (swg->force_hsb || TooNarrow())
	{
	    swg->force_hsb = True;	/* need horiz scrolling */
	    view_height = SUB(view_height, swg->hsb_height);
					/* too short to view items? */
	    if (TooShort())
	    {
		swg->force_vsb = True;	/* need vert scrolling */
		view_width = SUB(view_width, swg->vsb_width);
	    }
	}

	/* BulletinBoard Child, bbc (that's me), must be as wide as an item
	   and as tall as the shortest item.  Make sure the height is an
	   integral number of slots.
	*/
	swg->bbc_width = list_width;
	swg->bbc_height = _OlMax(MinHeight(w), view_height);

	/* The 'real' or virtual width is the same but the height is the
	   height of the entire list.
	*/
	swg->bbc_real_width = swg->bbc_width;
	swg->bbc_real_height= list_height;

	/* Recompute view height, rounding down. */
	ViewHeight(w) = swg->bbc_height / MinHeight(w);
	BottomIsPartial(w) = ((swg->bbc_height % MinHeight(w)) != 0);

	if (w->list.need_vsb)
	    AdjustTop(w); /* Top may need adjusting if ViewHeight changed. */

#undef TooNarrow
#undef TooShort

    }

    w->list.need_hsb = swg->force_hsb;
    w->list.need_vsb = swg->force_vsb;
    if (w->list.need_vsb)
	CalibrateVSBar(w);

#undef	SUB
}

/*
 * WarnOnLimitExceeded -
 */
static void
WarnOnLimitExceeded OLARGLIST((w, preferred, new_list))
	OLARG( Widget,		w)
	OLARG( Cardinal,	preferred)
	OLGRA( Boolean,		new_list)  /* T: new list, F: flat_set_values */
{
	OlVaDisplayWarningMsg(
		XtDisplay(w),
		OleNbadNumItems, OleToverflow, OleCOlToolkitWarning,
		new_list ? OleMfileFList_msg1 : OleMfileFList_msg2,
		XtName(w), OlWidgetToClassName(w), preferred, NumItems(w)
	);
} /* end of of WarnOnLimitExceeded */

/*
 * CallLimitExceeded - invoked from UpdateItemMetrics when
 *	an overflow situation is detected at the first time.
 *
 * True  - can replace NumItems with Preferred,
 * False - otherwise.
 */
static Boolean
CallLimitExceeded OLARGLIST((w, preferred, sv_min_h, sv_total_padding))
	OLARG( FlatListWidget,	w)
	OLARG( Cardinal,	preferred)
	OLARG( Dimension,	sv_min_h)
	OLGRA( Dimension,	sv_total_padding)
{
	Preferred(w)		= preferred;

	if ( XtHasCallbacks(
		(Widget)w, XtNitemsLimitExceeded) == XtCallbackHasSome )
	{
		OlFListItemsLimitExceededCD	cd;

		cd.item_data	= GetItemData(w, OL_NO_ITEM, NULL);
		cd.preferred	= Preferred(w);
		cd.ok		= False;

		XtCallCallbacks((Widget)w, XtNitemsLimitExceeded, &cd);

		if ( UsePreferred(w) = cd.ok )
		{
			MinHeight(w)	= sv_min_h;
			TotalPadding(w) = sv_total_padding;
		}
	}

	if ( !UsePreferred(w) )
		WarnOnLimitExceeded((Widget)w, preferred, True);

	return(UsePreferred(w));

} /* end of CallLimitExceeded */

/****************************procedure*header*****************************
    UpdateItemMetrics- maintain item metrics when item is initialized
	(ItemInitialize) or changed (ItemSetValues).

      Metrics are kept for
	max_field_width		per-field maximum widths
	min_field_width		narrowest field (for horiz scrolling)
	padding			total padding and field pre_padding
	min_height		shortest item (for vert scrolling)
	item_offsets		vert offset for each item
	total_height		sum of item heights
	num_slots		total_height/min_height (for vert slider max)
*/
static Boolean
UpdateItemMetrics OLARGLIST((w, old_item, new_item))
    OLARG( FlatListWidget,	w )
    OLARG( FlatListItem,	old_item )
    OLGRA( FlatListItem,	new_item )
{
    Cardinal		indx;
    Dimension		new_height;
    Dimension		field_height;
    Dimension		field_width;
    Dimension		padding;
    Boolean		padding_changed = False;
    Boolean		ret_val = True;
    int			i;

    Dimension		h_thickness = (OlGetGui() == OL_OPENLOOK_GUI) ?
					0 : 2 * HighlightThickness(w);

    Dimension		sv_min_height = MinHeight(w);
    Dimension		sv_total_padding = TotalPadding(w);


    indx	= ItemIndex(new_item);

#define LastItem	( indx == NumItems(w) - 1 )

    if ( UsePreferred(w) )
    {
	if ( LastItem )
	{
		NumItems(w) = Preferred(w);
		Preferred(w) = OL_NO_ITEM;
		UsePreferred(w) = False;
	}
	return(False);
    }

    new_height	= 0;

    for (i = 0; i < NumFields(w); i++)
    {
	ComputeFieldSize(w, new_item, i, &field_width, &field_height);

	if (field_width > MaxFieldWidth(w, i))
	    MaxFieldWidth(w, i) = field_width;

	/* For purposes of min field width (for horizontal scrolling),
	   if the calculated width of a string field is less than the 'width'
	   of the font, set it equal to the font width.
	   string.
	*/
	if ((w->list.fields[i].type == STRING) &&
	    (field_width < (Dimension)FontWidth(w, new_item)))
	    field_width = FontWidth(w, new_item);

	if ((field_width != 0) && (field_width < MinFieldWidth(w)))
	    MinFieldWidth(w) = field_width;

	if (field_height > new_height)
	    new_height = field_height;	/* height is max of all fields */
    }

    new_height += h_thickness;

    /* Update minimum item height (for vertical scrolling) */
    if (new_height < MinHeight(w))
	MinHeight(w) = new_height;

    /* Update maximum padding */
    padding = CountPads(Format(w)) * FontWidth(w, new_item);

    if (padding > TotalPadding(w))
    {
	TotalPadding(w) = padding;
	padding_changed = True;
    }

    /* If processing ItemInitialize (old_item == NULL):
	1. If this is the last item:
	    a. Compute total field padding
	    b. Update total height and num-slots.
	2. Else, update offset of next item (below this item).

       Else processing ItemSetValues (old_item != NULL):
	1. If height of item has changed:
	    a. Update y_offsets of items below new item.
	    b. Update total height and num-slots.
	    c. Re-calibrate VSBar.
	2. If padding changed, re-compute field padding.
	3. Set the item size.
    */

#define UpdateNumSlots(w) \
    NumSlots(w) = ((Dimension) \
		   (TotalHeight(w) + MinHeight(w) - 1)) / MinHeight(w)

#define IdeaSize	indx
#define OverFlow(ii,h)	( (Position)(ItemOffset(w, ii) + (h)) < \
				ItemOffset(w, ii) )

    if (old_item == NULL)
    {
	Boolean		overflowed = False;

		    /* detect overflow the very first time */
	if ( OverFlow(indx, new_height) && Preferred(w) == OL_NO_ITEM &&
	     CallLimitExceeded(w, IdeaSize, sv_min_height, sv_total_padding) )
	{
		overflowed = True;
		ret_val = False;
	}

	if ( !LastItem )
		ItemOffset(w, indx + 1) = ItemOffset(w, indx) + new_height;

	if ( overflowed || LastItem )
	{
	    ComputePadding(w);

	    if ( overflowed )
	    {
		TotalHeight(w) = ItemOffset(w, IdeaSize);

		if ( LastItem )
		{
			if ( UsePreferred(w) )
			{
				NumItems(w) = Preferred(w);
				UsePreferred(w) = False;
			}
			Preferred(w) = OL_NO_ITEM;
		}
	    }
	    else
	    {
		TotalHeight(w) = ItemOffset(w, indx) + new_height;
	        Preferred(w) = OL_NO_ITEM;
	    }

	    UpdateNumSlots(w);
	}

    } else {
	Dimension old_height = ItemHeight(w, indx);

	if (old_height != new_height)		/* height has changed */
	{
	    Position delta = new_height - old_height;

	    for (i = indx + 1; i < NumItems(w); i++)
	    {
		if ( OverFlow(i, delta) )
			WarnOnLimitExceeded((Widget)w, indx, False);

		ItemOffset(w, i) += delta;
	    }

	    TotalHeight(w) += delta;
	    UpdateNumSlots(w);

	    if (w->list.need_vsb)
	    {
		AdjustTop(w); /* Top may need adjusting if NumSlots changed */
		CalibrateVSBar(w);
	    }
	}

	if (padding_changed)
	    ComputePadding(w);

	/* Set size of item.  If size has changed, ItemSetValues will
	   generate a geometry request.  Do this last after all other
	   metrics have been computed.
	*/
	new_item->flat.width	= ItemWidth(w);
	new_item->flat.height	= new_height;
    }

    return(ret_val);

#undef UpdateNumSlots
#undef LastItem
#undef IdeaSize
#undef OverFlow
}

/****************************procedure*header*****************************
    ViewItem- scroll view to make item visible.

    It's assumed the item is not in view (caller has done !InView()).
*/
static void
ViewItem OLARGLIST((w, indx))
    OLARG( FlatListWidget,	w )
    OLGRA( Cardinal,		indx )
{
    int	diff;
    Arg	args[1];

    /* Compute (signed) number of pixels to move item into view */

    diff =
	AboveView(w, indx) ?
	    ItemOffset(w, indx) - YTop(w) - (MinHeight(w) - 1)

	:	/* Below View */

	    ItemOffset(w, indx) + ItemHeight(w, indx) -	YBottom(w) +
		(MinHeight(w) - 1);

    /* Now convert pixel value into # of slots */
    diff /= (int)MinHeight(w);

    ScrollList(w, diff);

    /* Manually position slider */
    XtSetArg(args[0], XtNsliderValue, Top(w));
    XtSetValues(w->list.vSBar, args, 1);
}

/****************************procedure*header*****************************
    VSliderMovedCB- XtNvSliderMoved callback: scroll view.
*/
/* ARGSUSED */
static void
VSliderMovedCB OLARGLIST((widget, client_data, call_data))
    OLARG( Widget,	widget )	/* unused */
    OLARG( XtPointer,	client_data )
    OLGRA( XtPointer,	call_data )
{
    FlatListWidget	flw = (FlatListWidget)client_data;
    OlScrollbarVerify *	sbv = (OlScrollbarVerify *)call_data;

    if (sbv->delta != 0)
	ScrollList(flw, sbv->delta);
}

/****************************class*procedures*****************************

    Class Procedures
*/

/****************************procedure*header*****************************
    AnalyzeItems - this routine analyzes new items for this class.

    This routine is called after all of the items have been initialized, so we
    can assume that all items have valid values.  Also, the ItemInitialize
    routine has already found the set item (if applicable), provided the
    application set one.  If not, this routine will choose one.
*/
/* ARGSUSED */
static void
AnalyzeItems OLARGLIST((widget, args, num_args))
    OLARG( Widget,	widget )
    OLARG( ArgList,	args )
    OLGRA( Cardinal *,	num_args )
{
    FlatListWidget	w = FLW(widget);
	
    /* For an exclusives, if no item is set and none-set is not allowed,
       select one.
    */
    if (Exclusives(w) && !ExcItemIsSelected(w) && !NoneSet(w))
    {
	Cardinal	indx;
	Arg		set_args[2];
	Boolean		managed;
	Boolean		mapped;

	XtSetArg(set_args[0], XtNmanaged, &managed);
	XtSetArg(set_args[1], XtNmappedWhenManaged, &mapped);

	for (indx = 0; indx < NumItems(w); indx++)
	{
	    managed	= False;
	    mapped	= False;

	    OlFlatGetValues(widget, indx, set_args, XtNumber(set_args));

	    if (managed && mapped)
	    {
		XtSetArg(set_args[0], XtNset, True);
		OlFlatSetValues(widget, indx, set_args, 1);
		break;
	    }
	}
    }
}

/****************************procedure*header*****************************

    ChangeManaged- perform layout as the result of a changed number of
	managed items.  'num_changed' == 0 means we're working with a new
	list.

	Let the superclass do the real work.  If we're in a ScrolledWin,
	override the resulting bounding height and offset the 1st item..
	While in a SWin, we're only as high as the view height.
*/
static void
ChangeManaged OLARGLIST((widget, items, num_changed))
    OLARG( Widget,	widget)
    OLARG( FlatItem *,	items)
    OLGRA( Cardinal,	num_changed)
{
    FlatListWidget		w;
    OlFlatChangeManagedProc	change_managed =
	((FlatWidgetClass)(flatListWidgetClass->core_class.superclass))->
	    flat_class.change_managed;

    if (change_managed == NULL)
	return;

    w = FLW(widget);

    /* Let superclass do the real work */
    (*change_managed)(widget, items, num_changed);

    /* Now override some results from superclass layout if we're in a SWin */
    if (InSWin(w) && !EmptyList(w))
    {
	TopOffset(w)			= -YTop(w);
	FRCPART(w)->bounding_height	= ViewPixelHeight(w);

	/* If this is the result of 'items_touched' or "relayout_hint",
	 * set SWin view size. Note that, changing "font" is one
	 * instance that will cause "relayout_hint" to be True.
	 */
	if (FPART(w)->items_touched || FPART(w)->relayout_hint)
	    SetViewSize(w);
    }

} /* END OF ChangeManaged() */

/****************************procedure*header*****************************

    ClassInitialize - this procedure inherits all superclass procedures.
*/
static void
ClassInitialize OL_NO_ARGS()
{
    /* Inherit all superclass procedures.  This scheme saves us from
       worrying about putting function pointers in the wrong class slot if
       they were statically declared.  It also allows us to inherit new
       functions simply be recompiling, i.e., we don't have to stick
       XtInheritBlah into the class slot.
    */
	OlFlatInheritAll(flatListWidgetClass);

    /* Now override procedures we don't want to inherit. */
#undef F
#define F	flatListClassRec.flat_class
	F.analyze_items		= AnalyzeItems;
	F.change_managed	= ChangeManaged;
	F.draw_item		= DrawItem;
	F.get_index		= GetItemIndex;
	F.initialize		= Initialize;
	F.item_activate		= ItemActivate;
	F.item_dimensions	= ItemDimensions;
	F.item_initialize	= ItemInitialize;
	F.item_set_values	= ItemSetValues;
	F.set_values		= SetValues;
	F.traverse_items	= TraverseItems;
#undef F

	if (OlGetGui() == OL_OPENLOOK_GUI)
	{
		flatListClassRec.primitive_class.num_event_procs =
						OL_N_EVENT_PROCS;
	}

} /* END OF ClassInitialize() */

/****************************procedure*header*****************************
    Destroy-
*/
static void
Destroy OLARGLIST((widget))
    OLGRA( Widget,	widget)
{
    XtFree((char *)FLPART(widget)->y_offsets);	/* free offsets vector */
    XtFree((char *)FLPART(widget)->fields);	/* free field records */
    XtFree(FLPART(widget)->format);	/* free 'format' string */
}

/****************************procedure*header*****************************
    DrawItem - this routine draws a single instance of a list sub-object.
*/
static void
DrawItem OLARGLIST((widget, item, di))
    OLARG( Widget,		widget )/* container widget id	*/
    OLARG( FlatItem,		item )	/* expanded item	*/
    OLGRA( OlFlatDrawInfo *,	di )	/* Drawing information	*/
{
    FlatListWidget	w;
    String		save_label;
    XImage *		save_label_image;
    OlgAttrs *		attrs;
    unsigned int	flags;
    LabelInfo		lbl_info;
    XtPointer		fake_lbl;
    OlgLabelProc	fake_draw_proc;

    Dimension		h_thickness = 0;

    w = FLW(widget);

    if (OlGetGui() == OL_MOTIF_GUI)
    {
	h_thickness = HighlightThickness(w);

		/* Use superclass draw_item to draw Motif Location Cursor */
#define SUPERCLASS ((FlatListClassRec *)flatListClassRec.core_class.superclass)

		/* We need to draw Location cursor either ways
		 * because this may be in a scrolled window...
		 */
	(*SUPERCLASS->flat_class.draw_item)(widget, item, di);

#undef SUPERCLASS
    }

    /* Get GCs and label information.  Make flat.label and flat.label_image
       NULL since we're not interested in drawing the label at this time.
       This saves time in SetupAttributes.  If we're going to use the label
       data to draw the item (1 field and field_data is NULL), save it away.
    */
    if (UseLabel(w, item))
    {
	save_label = item->flat.label;
	save_label_image = item->flat.label_image;

    } else
    {
	save_label = NULL;
	save_label_image = NULL;
    }

    item->flat.label = NULL;
    item->flat.label_image = NULL;
    OlFlatSetupAttributes (widget, item, di, &attrs,
			    &fake_lbl, &fake_draw_proc);

    if (save_label != NULL)
	item->flat.label = save_label;

    if (save_label_image != NULL)
	item->flat.label_image = save_label_image;

    /*** Determine draw/visual flags ***/

    /* Draw 'set' visual or no frame */
    flags = ((CurrentItem(w) == ItemIndex(item)) ||
	     (Exclusives(w) &&
	      ((CurrentItem(w) == OL_NO_ITEM) &&
	       (SelectedItem(w) == ItemIndex(item)))) ||
	     (!Exclusives(w) && FLIPART(item)->selected))
	? RB_SELECTED : RB_NOFRAME;

    if (flags == RB_SELECTED && OlGetGui() == OL_MOTIF_GUI)
	flags |= RB_NOFRAME;

    if (!item->flat.sensitive || !XtIsSensitive((Widget)w))
	flags |= RB_INSENSITIVE;		/* Draw insensitive */

    /* If 2D, the background must be cleared since DrawRectButton only fills
       the area within the default ring.
    */
    if (!OlgIs3d())
	XClearArea(XtDisplay(w), XtWindow(w),
		   di->x, di->y, di->width, di->height, False);

    /* Draw the button.  Pass necessary things to DrawFields via lbl_info.
       '3' must be subtracted off of any desired margin since '3' is added by
       DrawRectButton to draw the border, set border, and default ring :-(
    */
    lbl_info.widget	= widget;
    lbl_info.item	= item;
    lbl_info.draw_info	= di;

    {
	Position	xx, yy;
	Dimension	ww, hh;
	Dimension	h2_thickness = 2 * h_thickness;

	xx = di->x + h_thickness;
	yy = di->y + h_thickness;
	ww = di->width - h2_thickness;
	hh = di->height - h2_thickness;

	OlgDrawFlatListItem(
		di->screen, di->drawable, attrs,
		xx, yy, ww, hh,
		(caddr_t)&lbl_info,
		DrawFields, flags, HORIZ_MARGIN-3, VERT_MARGIN-3); 
    }
}				/* END OF DrawItem() */

/****************************procedure*header*****************************
    GetItemIndex- The FlatList is currently restricted to a single column of
	items.  This makes computing an item index from its 'y' value
	trivial using the offsets vector.

	For small lists (< 100 items), a linear search is done.  For large
	lists, a binary search is done.

	Note that when the NFList is in a SWin and is vertically scrollable,
	'y' values are "normalized":
		y < 0 ==> indx=0 and y > TotalHeight ==> indx = NumItems - 1
	This is only necessary for WipeThru selection but, alas, it it best
	done here.  This is probably different from other class get_index
	functions; they probably return OL_NO_ITEM when y is outside of
	the widget.
*/
/* ARGSUSED */
static Cardinal
GetItemIndex OLARGLIST((widget, x, y, ignore_sensitivity))
    OLARG( Widget,	widget )
    OLARG( Position,	x )		/* unused */
    OLARG( Position,	y )
    OLGRA( Boolean,	ignore_sensitivity )
{
    FlatListWidget	w = FLW(widget);
    Cardinal		indx = OL_NO_ITEM;

    if (EmptyList(widget))
	return(OL_NO_ITEM);

    /* y is relative to the view.  Make it absolute */
    y += YTop(w);

    /* If we are vertically scrollable and the pointer is above or below (the
       virtual extent of) the list, normalize the indx by making it the minimum
       or maximum index.
    */
    if (y < 0)
	indx = VScrollable(w) ? 0 : OL_NO_ITEM;

    else if ((Dimension)y > TotalHeight(w))
	indx = VScrollable(w) ? NumItems(w) - 1 : OL_NO_ITEM;

    /* y is within the (virtual) range of the list */
    else if (NumItems(w) < 100)
    {					/* linear search for small lists */
	register Position *	offset;

	/* divide the list in half and do a simple linear search */
	if (y < (Position)(TotalHeight(w) / 2))
	{
	    for (indx = 0, offset = &ItemOffset(w, indx);
		 indx < NumItems(w) && *offset <= y;
		 indx++, offset++)
		;

	    indx--;
	    
	} else
	{
	    for (indx = NumItems(w) - 1, offset = &ItemOffset(w, indx);
		 indx != 0 && *offset >= y; indx--, offset--)
		;
	}

    } else
    {					/* binary search for large lists */
	register int		first, last, i;
	register Position	top, bottom;
	
	first	= 0;
	last	= NumItems(w) - 1;
	indx	= OL_NO_ITEM;

	while ((first <= last) && (indx == OL_NO_ITEM))
	{
	    i		= (first + last) / 2;	/* middle index */
	    top		= ItemOffset(w, i);	/* top of middle item */
						/* bottom of middle item: */
	    bottom	= (i == NumItems(w) - 1) ?
		TotalHeight(w) : ItemOffset(w, i + 1);

	    if (y < top)
		last = i - 1;

	    else if (y > bottom)
		first = i + 1;

	    else
		indx = i;			/* match */
	}
    }

    if ((indx != OL_NO_ITEM) && !ignore_sensitivity)
    {
	Arg	args[1];
	Boolean	sensitive;

	XtSetArg(args[0], XtNsensitive, &sensitive);
	OlFlatGetValues(widget, indx, args, XtNumber(args));

	if (!sensitive)
	    indx = OL_NO_ITEM;
    }

    return (indx);
}

/****************************procedure*header*****************************
    Initialize - this procedure initializes the instance part of the widget

	Note that our geometry is determined by the superclass'
	ChangeManaged procedure called at the end of this initialization
	phase.
*/
/*ARGSUSED*/
static void
Initialize OLARGLIST((request, new, args, num_args))
    OLARG( Widget,	request)/* unused */
    OLARG( Widget,	new)	/* What the application gets, so far...	*/
    OLARG( ArgList,	args)	/* unused */
    OLGRA( Cardinal *,	num_args)	/* unused */
{
    FlatListPart *	flp = FLPART(new);
    OlSWGeometries	swg;

    /* Are we in a ScrolledWindow?  (During Initialization, check for
       XtParent rather than XtParent(XtParent()) since the FList widget
       hasn't been re-parented to the SWin's internal BulletineBoard yet).
       If we are in a SWin:
	 1. Set vSBar and need_vsb.  vSBar is used everywhere else to
	    indicate that we're in a ScrolledWindow.
	 2. Override vert auto-scroll and set computeGeometries proc.
	 3. Add Callback for vertical slider.

       If not in a SWin, clear vSBar and need_vsb.
    */
    if (XtIsSubclass(XtParent(new), scrolledWindowWidgetClass))
    {
	Widget		sw = XtParent(new);
	Arg		sw_args[4];

	swg		= GetOlSWGeometries((ScrolledWindowWidget)sw);

	flp->vSBar	= swg.vsb;
	flp->need_vsb	= swg.force_vsb;
	flp->need_hsb	= swg.force_hsb;

	XtSetArg(sw_args[0], XtNvAutoScroll, False);
	XtSetArg(sw_args[1], XtNhAutoScroll, True);
	XtSetArg(sw_args[2], XtNcomputeGeometries, SWinInterface);
	XtSetArg(sw_args[3], XtNpostModifyGeometryNotification,
							PostSWinInterface);
	XtSetValues(sw, sw_args, XtNumber(sw_args));

	XtAddCallback(sw, XtNvSliderMoved, VSliderMovedCB, new);
    }
    else
    {
	flp->vSBar	= NULL;
	flp->need_vsb	= False;
	flp->need_hsb	= False;
    }

    flp->bottom_is_partial = False;
    flp->use_preferred	= False;
    flp->preferred	= OL_NO_ITEM;
    flp->fields		= NULL;
    NumFields(new)	= 0;
    flp->y_offsets	= NULL;
    TotalPadding(new)	= 0;

    /* Establish geometry and SetViewSize if list is empty */
    if (EmptyList(new))
    {
	FlatListWidget	flw = FLW(new);

	if (new->core.height == 0)
	    new->core.height = (ViewHeight(flw) == 0) ? 10 : EmptyHeight(flw);

		/* Don't be smart, let Scrolled Window to figure out...	*/
	if (new->core.width == 0)
	    new->core.width = 10;

	/* If InSWin be as wide as view */
	if (InSWin(flw))
	{
	    SetViewSize(flw);
	}
    }

    InitState(new);		/* init state for wipe-thru and auto-scroll */
    flp->repeat_rate = 0;

    /* Modify 'none_set' for Nonexclusives */
    if (!Exclusives(new) && !NoneSet(new))
	NoneSet(new) = True;

    /* Check for NULL format string and make a widget copy of it */
    FixNullFormat(new);
    flp->format = XtNewString(flp->format);

    /* If processing new list, do new-list specific stuff */
    if (FPART(new)->items_touched)
	ItemsTouched(NULL, new);

}				/* END OF Initialize() */

/****************************procedure*header*****************************
    ItemActivate - this routine is used to activate the item with focus.
*/
/* ARGSUSED */
static Boolean
ItemActivate OLARGLIST((widget, item, type, data))
    OLARG( Widget,		widget )
    OLARG( FlatItem,		item )
    OLARG( OlVirtualName,	type )
    OLGRA( XtPointer,		data )
{
    FlatListWidget	w;
    Boolean		activate;

    switch (type)
    {
    case OL_SELECTKEY :
	w		= FLW(widget);
	activate	= True;

	ProcessClick(w, ItemIndex(item), IsSelected(w, ItemIndex(item)));

	break;

    default:
	activate = False;
	break;
    }

    return (activate);
}

/****************************procedure*header*****************************
    ItemDimensions - this routine determines the size of a single sub-object
*/
static void
ItemDimensions OLARGLIST((widget, item, return_width, return_height))
    OLARG( Widget,		widget )	/* Widget making request */
    OLARG( FlatItem,		item )		/* expanded item */
    OLARG( register Dimension *,return_width )	/* returned width */
    OLGRA( register Dimension *,return_height )	/* returned height */
{
    *return_width = ItemWidth(FLW(widget));
    *return_height = ItemHeight(widget, ItemIndex(item));
}

/****************************procedure*header*****************************
    ItemInitialize-
*/
/* ARGSUSED */
static void
ItemInitialize OLARGLIST((w, request, new, args, num_args))
    OLARG( Widget,	w )
    OLARG( FlatItem,	request )	/* unused */
    OLARG( FlatItem,	new )
    OLARG( ArgList,	args )		/* unused */
    OLGRA( Cardinal *,	num_args )	/* unused */
{
    FlatListItem	fli = FLI(new);

    if ( !UpdateItemMetrics(FLW(w), NULL, fli) )	/* overflowed */
	return;

    /* For exclusives, maintain at most 1 set item */
    if (Exclusives(w) && fli->list.selected)
    {
	if (!ExcItemIsSelected(w))
	{
	    SelectedItem(w) = ItemIndex(new);

	} else {
	    fli->list.selected = False;

	    OlVaDisplayWarningMsg(XtDisplay(w),
				  OleNtooManySet, OleTflatState,
				  OleCOlToolkitWarning,
				  OleMtooManySet_flatState,
				  XtName(w), OlWidgetToClassName(w),
				  ItemIndex(new), SelectedItem(w));
	}
    }
}

/****************************procedure*header*****************************
    ItemSetValues - this routine is called whenever the application does
    an XtSetValues on the container, requesting that an item be updated.
    If the item is to be refreshed, the routine returns True.
*/
/* ARGSUSED */
static Boolean
ItemSetValues OLARGLIST((widget, current, request, new, args, num_args))
    OLARG( Widget,	widget)		/* Flat widget container id	*/
    OLARG( FlatItem,	current)	/* expanded current item	*/
    OLARG( FlatItem,	request)	/* expanded requested item	*/
    OLARG( FlatItem,	new)		/* expanded new item		*/
    OLARG( ArgList,	args)
    OLGRA( Cardinal *,	num_args)
{
    FlatListWidget	w = FLW(widget);
    FlatListItem	cfli = (FlatListItem)current;
    FlatListItem	fli = (FlatListItem)new;
    Boolean		redisplay = False;
    Cardinal		indx = ItemIndex(new);

#define DIFFER(field)	(cfli->list.field != fli->list.field)

	/* can't recover from an overflow	*/
    (void)UpdateItemMetrics(w, cfli, fli);

	/*
	 * "field_data" (XtNformatData) contains a list of pointers
	 * (i.e., XtPointer) and Data are shared between FLAT and
	 * applications (i.e., Data are not cached). So FLAT won't
	 * be able to detect changes if "field_data" pointer address
	 * didn't change but one/more of the contents is/are changed
	 * (because of data sharing...).
	 *
	 * So force a redisplay if address of "field_data" doesn't
	 * change and if XtNformatData is part of the "arg" list.
	 */
    if (DIFFER(field_data))
	redisplay = True;
    else
    {
	Cardinal	i;

	for (i = 0; i < *num_args; i++)
		if (strcmp(args[i].name, XtNformatData) == 0)
		{
			redisplay = True;
			break;
		}
    }

    if (DIFFER(selected))		/* Item being [un]selected */
    {
	/* For Exclusives, if the item is being selected, make it the selected
	   item.  If there is already an item selected, unselect it first.  If
	   the item is being unselected, allow it if noneSet allowed.
	*/
	if (Exclusives(w))
	{
	    if (fli->list.selected)		/* Item is being selected */
	    {
		if (!ExcItemIsSelected(w))
		{
		    SelectedItem(w) = indx;

		} else if (SelectedItem(w) != indx) {
		    Arg		args[1];
		    Cardinal	old;

		    old = SelectedItem(w);	/* save currently-selected */

		    SelectedItem(w) = indx;

		    XtSetArg(args[0], XtNset, False);
		    OlFlatSetValues(widget, old, args, 1); /* Unselect prev */
		}

		redisplay = True;

	    } else {			/* Exclusives: item being unselected */

		if (SelectedItem(w) == indx)
		{
		    if (NoneSet(w))	/* Unselect item if allowed */
		    {
			SelectedItem(w) = OL_NO_ITEM;
			redisplay = True;
		    }

		} else {
		    redisplay = True;
		}
	    }

	} else {			/* Nonexclusives */
		redisplay = True;
	}
    }

#undef DIFFER

    /* redisplay must always be tempered with whether the item is in view.
       This is only meaningful if vert scrolling is possible.
    */
    if (redisplay && VScrollable(w))
    {
		/* Check if the bottom item is partially displayed... */
	if ( !(redisplay = InView(w, indx)) && BottomIsPartial(w) )
	{
		ViewHeight(w) += 1;

		if (InView(w, indx))
			redisplay = True;

		ViewHeight(w) -= 1;
	}
    }

    return (redisplay);
}				/* END OF ItemSetValues() */

/****************************procedure*header*****************************
    Redisplay- redisplay only those items within expose region.
*/
static void
Redisplay OLARGLIST((widget, event, region))
    OLARG( Widget,	widget)
    OLARG( XEvent *,	event)
    OLGRA( Region,	region)
{
    if (EmptyList(widget))
	XClearWindow(XtDisplay(widget), XtWindow(widget));

    else
	DisplayList(FLW(widget),
		    event->xexpose.y, event->xexpose.height-1, False);
}

/****************************procedure*header*****************************
    Resize- override some results of calling superclasses' resize proc
*/
static void
Resize OLARGLIST((widget))
    OLGRA( Widget,	widget)
{
    FlatListWidget	w;
    XtWidgetProc	proc =
	flatListWidgetClass->core_class.superclass->core_class.resize;

    if (proc == NULL)
	return;

    w = FLW(widget);

    (*proc)(widget);

    if (InSWin(w))
	TopOffset(w) = -YTop(w);
}

/****************************procedure*header*****************************
    SetValues - this procedure monitors the changing of instance data
*/
/* ARGSUSED */
static Boolean
SetValues OLARGLIST((current, request, new, args, num_args))
    OLARG( Widget,	current)	/* What we had			*/
    OLARG( Widget,	request)	/* What we want			*/
    OLARG( Widget,	new)		/* What we get, so far		*/
    OLARG( ArgList,	args)
    OLGRA( Cardinal *,	num_args)
{
    FlatPart *		fp = FPART(new);
    FlatListPart *	flp = FLPART(new);
    FlatListPart *	cflp = FLPART(current);
    Boolean		redisplay = False;

#define DIFFER(field)	(flp->field != cflp->field)
#define PW(w)		(((FlatWidget)(w))->primitive)
#define PDIFF(field)	(PW(new).field != PW(current).field)

    if (DIFFER(exclusive_settings) && !fp->items_touched)
    {
	flp->exclusive_settings = cflp->exclusive_settings;

	OlVaDisplayWarningMsg(XtDisplay(new),
			      OleNfileFExclusive, OleTsetValues,
			      OleCOlToolkitWarning,
			      OleMfileFExclusive_msg1,
			      XtName(new), OlWidgetToClassName(new));
    }

    /* Modify 'none_set' for Nonexclusives */
    if (DIFFER(none_set) && !Exclusives(new) && !NoneSet(new))
	NoneSet(new) = True;

    if (DIFFER(format))
    {
	FixNullFormat(new);

	if (strcmp(flp->format, cflp->format) == 0)
	{
	    /* Pointers are different but contents are the same */
	    flp->format = cflp->format;

	} else
	{
	    /* 'format' has changed:
		    Make copy of new format
		    Free old copy of format
		    Indicate that items have been 'touched'
	    */
	    flp->format = XtNewString(flp->format);
	    XtFree(cflp->format);
	    fp->items_touched = True;
	}
    }

	/* If processing new list or because of "relayout" or
	 * because of "font" changes, do new-list specific stuff.
	 *
	 * Note that, PDIFF(font) eventually will cause a relayout
	 * change but it won't be seen in set_values (will be seen
	 * in chanage_managed) because this flag will be turned on
	 * in DefaultItemSetValues().
	 */
    if (fp->items_touched || fp->relayout_hint || PDIFF(font) 
	|| PDIFF(font_list))
    {
	Cardinal	old_top = Top(new);
	Cardinal	old_selected_item = SelectedItem(new);

	ItemsTouched(current, new);

		/* If "font" is changed and item list is not touched,
		 * then we will need to calculate "item_metrics"
		 * here because ItemSetValues won't be invoked
		 * in this case...
		 */
	if ((PDIFF(font) || PDIFF(font_list)) && !fp->items_touched)
	{
		Cardinal	i;
		OL_FLAT_ALLOC_ITEM(new, FlatItem, item);

			/* keep this because ItemsTouched resets it */
		SelectedItem(new) = old_selected_item;
		for (i = 0; i < FPART(new)->num_items; ++i)
		{
			OlFlatExpandItem(new, i, item);
			(void)UpdateItemMetrics(FLW(new), NULL, FLI(item));
		}
		OL_FLAT_FREE_ITEM(item);
	}

		/* Reset it to the older value so it won't start
		 * with the first item. Don't worry about the new
		 * item list is shorter than older one because
		 * SetViewSize will be invoked later and Top(w)
		 * will be adjusted properly in that routine...
		 */
	if (!EmptyList(new) && InSWin(new) &&
	    flp->maintain_view && ViewHeight(new) < NumItems(new))
		Top(new) = old_top;
	redisplay = True;

    } else
    {
	/* Consider these only when there isn't a new list: */

	/* If we're in a SWin and the view_height has changed, set the size
	   of the SWin view.  Don't do this if 'items_touched' since new
	   item metrics will have to be computed first; view size is set in
	   ChangeManaged in this case.
	   */
	if (InSWin(new) && (fp->items != NULL) && DIFFER(view_height))
	    SetViewSize(FLW(new));

	/* If we're vertically scrollable, consider the XtNviewItemIndex
	   resource which forces an item into view.
	*/
	if (VScrollable(new))
	{
	    int i;
	
	    for (i = 0; i < *num_args; i++)
		if (strcmp(args[i].name, XtNviewItemIndex) == 0)
		{
		    Cardinal indx = (Cardinal)args[i].value;

		    if (indx < NumItems(new))
		    {
			if (!InView(new, indx))
			{
			    if ( LastFocusItem(new) == OL_NO_ITEM )
				LastFocusItem(new) = indx;

			    ViewItem((FlatListWidget)new, indx);
			}

		    } else
		    {
			/* FIX THIS after OlFlatListViewItem is removed */
			OlVaDisplayWarningMsg(XtDisplay(new),
					      OleNbadItemIndex,
					      OleTviewItem,
					      OleCOlToolkitWarning,
					      OleMbadItemIndex_flatState,
					      XtName(new),
					      OlWidgetToClassName(new),
					      "OlFlatListViewItem", indx);
		    }
		    break;
		}
	}

    }

    return(redisplay);

#undef DIFFER
#undef PW
#undef PDIFF
}				/* END OF SetValues() */

/****************************procedure*header*****************************
    TraverseItems- adjust view if necessary to keep focus item in view.
*/
static Cardinal
TraverseItems OLARGLIST((widget, start_fi, dir, time))
    OLARG( Widget,		widget)		/* FlatWidget id */
    OLARG( Cardinal,		start_fi)	/* start focus item */
    OLARG( OlVirtualName,	dir)		/* Direction to move */
    OLGRA( Time,		time)		/* Time of move (ignored) */
{
    OlFlatTraverseItemsFunc	func =
	((FlatWidgetClass)(flatListWidgetClass->core_class.superclass))->
	    flat_class.traverse_items;
    Cardinal	new_fi;			/* index of new focus item */

    new_fi = (func == NULL) ? start_fi : (*func)(widget, start_fi, dir, time);

    if (VScrollable(widget) && !InView(widget, new_fi))
    {
	ViewItem(FLW(widget), new_fi);
    }

    return (new_fi);
}

/****************************action*procedures****************************

   Action Procedures
*/

#define B_FREE_YES_CURSOR	(1 << 0)
#define B_FREE_NO_CURSOR	(1 << 1)
/****************************procedure*header*****************************
 * CreateDragCursor - creates cursor suitable for the drag operation.
 *	note that this routine should initialize *call_data* properly
 *		before invoking XtNdragCursorProc...
 *
 *	also note that the *caller* should use *returned* value from
 *		this routine to determine whether *cursor* resource
 *		should be freed afterward (not the *static_cursor*
 *		value)...
 */
static OlBitMask
CreateDragCursor OLARGLIST((w, ve, user_data, client_data, call_data))
	OLARG( Widget,				w)
	OLARG( OlVirtualEvent,			ve)
	OLARG( XtPointer,			user_data)
	OLARG( XtPointer,			client_data)
	OLGRA( OlFlatDragCursorCallData *,	call_data)
{
	Arg			args[1];
	OlBitMask		ret_val = 0;
	OlFlatCallData *	item_data;
	XtCallbackProc		cursor_proc = NULL;

	call_data->ve		= ve;
	call_data->yes_cursor	= None;
	call_data->no_cursor	= None;
	call_data->x_hot	= 0;
	call_data->y_hot	= 0;
	call_data->static_cursor= True;

	XtSetArg(args[0], XtNdragCursorProc, (XtArgVal)&cursor_proc);
	OlFlatGetValues(w, ve->item_index, args, 1);

	if (cursor_proc)
	{
		item_data = GetItemData(
				(FlatListWidget)w, ve->item_index, user_data);

		call_data->item_data	= *item_data;

		(*cursor_proc)(w, client_data, (XtPointer)call_data);
	}

	if (call_data->no_cursor == None)
		call_data->no_cursor = OlGetNoCursor(XtScreenOfObject(w));
	else
	{
		if (call_data->static_cursor == False)
			ret_val |= B_FREE_NO_CURSOR;
	}

	if (call_data->yes_cursor == None)
		call_data->yes_cursor =
			(ve->virtual_name == OL_SELECT ||
			 ve->virtual_name == OLM_BDrag ||
			 ve->virtual_name == OL_DRAG) ?
				OlGetMoveCursor(w) : OlGetDuplicateCursor(w);
	else
	{
		if (call_data->static_cursor == False)
			ret_val |= B_FREE_YES_CURSOR;
	}

	return(ret_val);

} /* end of CreateDragCursor */

/****************************procedure*header*****************************
    ButtonHandler- handle OL_SELECT ButtonPress and ButtonRelease.

	For ButtonPress, OlDetermineMouseAction is used to determine if a
	Release (click), double-click or Motion is to follow.
*/
static void
ButtonHandler OLARGLIST((widget, ve))
    OLARG( Widget,		widget )
    OLGRA( OlVirtualEvent,	ve )
{
    FlatListWidget	w;
    FlatListPart *	flp;
    Cardinal		indx;
    Boolean		selected;
    Arg			args[3];
    XtCallbackProc	callproc;
    XtPointer		client_data;
    XtPointer		user_data;

    if (!XtIsSensitive(widget))
	return;

    w	= FLW(widget);
    flp	= FLPART(widget);

    switch (ve->xevent->type)
    {
    case ButtonPress :
	/* For ButtonPress, only interested in SELECT over "valid" item
	   (SELECT in margin ignored, for instance)
	*/
	if ((ve->item_index == OL_NO_ITEM) ||
	    (ve->virtual_name != OL_SELECT && ve->virtual_name != OLM_BDrag))
	    break;

	ve->consumed = True;

	flp->start_indx = indx = ve->item_index;

	selected = IsSelected(w, indx);
	if (!selected)
	{
	    if (ve->virtual_name == OLM_BDrag)
	    {
		flp->start_indx = OL_NO_ITEM;
		break;
	    }
	    SetCurrentItem(w, indx);
	}

	switch (OlDetermineMouseAction(widget, ve->xevent))
	{
	case MOUSE_CLICK :
	    if (ve->virtual_name != OLM_BDrag)
		    ProcessClick(w, indx, selected);
	    break;

	case MOUSE_MULTI_CLICK :
	    if (ve->virtual_name == OLM_BDrag)
		break;

	    /* If 'dblSelect' callback is not registered, consider this a
	       (second) single click by processing a button release.
	       Otherwise, call the dblSelect proc.
	    */
	    callproc = NULL;
	    client_data = NULL;
	    user_data = NULL;
	    XtSetArg(args[0], XtNdblSelectProc, &callproc);
	    XtSetArg(args[1], XtNclientData, &client_data);
	    XtSetArg(args[2], XtNuserData, &user_data);
	    OlFlatGetValues(widget, indx, args, 3);

	    if (callproc == NULL)
	    {
		ProcessClick(w, indx, selected);

	    } else
	    {
		InitState(w);

		if (!selected)
		    OlFlatRefreshItem(widget, indx, False);

		CallCallProc(w, indx, callproc,
			     client_data, user_data, NULL);
	    }
	    break;

	case MOUSE_MOVE :
	    /* If motion started over selected item, we're in drag 'n drop.
	       If motion started over unselected item, wipe-thru selection.
	    */
	    if (selected)
	    {
	        Display *			dpy;

		dpy = XtDisplayOfObject(widget);

		/* If 'drop' callback is registered, perform drag 'N drop */
		callproc = NULL;
		client_data = NULL;
		user_data = NULL;
		XtSetArg(args[0], XtNdropProc, &callproc);
		XtSetArg(args[1], XtNclientData, &client_data);
		XtSetArg(args[2], XtNuserData, &user_data);
		OlFlatGetValues(widget, flp->start_indx, args, 3);

		/* Ignore this gesture if motion started over selected
		   item but no callproc
		*/
		if (callproc == NULL)
		{
		    XUngrabPointer(dpy, CurrentTime);

		} else			/* Do drag 'N drop */
		{
		    OlBitMask			free_cursor_mask;
		    OlFlatDragCursorCallData 	drag_cursor_call_data_rec;

		    OlDnDAnimateCursors		cursors;
		    OlDnDDestinationInfo	dst_info_rec;
		    OlDnDDragDropInfo		root_info_rec;
		    OlDnDDropStatus		drop_status;

		    free_cursor_mask = CreateDragCursor(
				widget, ve, user_data, client_data,
				&drag_cursor_call_data_rec);

		    cursors.yes_cursor = drag_cursor_call_data_rec.yes_cursor;
		    cursors.no_cursor  = drag_cursor_call_data_rec.no_cursor;

		    XUngrabPointer(dpy, CurrentTime);

		    drop_status = OlDnDTrackDragCursor(
					widget, &cursors,
					&dst_info_rec, &root_info_rec
		    );

		    /* Drop only handled outside of this window */
		    if (dst_info_rec.window != XtWindow(widget))
		    {
			OlFlatDropCallData	call_data;

			/* Set up the call data structure	*/
			call_data.ve		= ve;
			call_data.dst_info	= &dst_info_rec;
			call_data.root_info	= &root_info_rec;
			call_data.drop_status	= drop_status;

			CallCallProc(w, indx, callproc,
				     client_data, user_data, &call_data);
		    }

		    if (free_cursor_mask & B_FREE_YES_CURSOR)
			XFreeCursor(dpy, cursors.yes_cursor);

		    if (free_cursor_mask & B_FREE_NO_CURSOR)
			XFreeCursor(dpy, cursors.no_cursor);
		}
	        InitState(w);

	    }
	    else /* Motion started over unselected item: wipethru selection */
	    {
	        if (ve->virtual_name != OLM_BDrag)
			WipeThru((XtPointer)w, (XtIntervalId *)NULL);
	    }		

	    break;

	default:
	    break;
	}		/* switch(OlDetermineMouseAction()) */
	break;		/* ButtonPress */

    case ButtonRelease :
	/* SELECT: We cannot restrict the release to SELECT.  Press may have
	   been SELECT but release may have modifier which makes it look not
	   like a SELECT.

	   False release: press didn't occur over the list (button down moved
	   into the list and then was released).  This is detected by looking
	   at the start_indx.  If the press didn't occur over an item,
	   start_indx is undefined.

	   Valid release: can only occur at the end of wipe-thru selection.
	   This button release strictly does not follow a drag N drop, click
	   or multi-click since OlDragAndDrop "consumes" the button release
	   for these actions.  If valid, select the current item to terminate
	   wipe-thru.
	*/

	ve->consumed = True;

	if (flp->start_indx != OL_NO_ITEM)
	{
	    if (Exclusives(w))
		SelectItem(w, CurrentItem(w), True);

	    else
		SelectItems(w, CurrentItem(w), 1, True);

	    XUngrabPointer(XtDisplay(widget), CurrentTime);
	    InitState(w);
	}
	break;
    }
}

/****************************procedure*header*****************************
    ProcessClick- initialize the wipe-thru and auto-scroll state and select or
	unselect the item 'indx'.  ProcessClick strictly handles clicks (and
	item activation) where there has been no intervening motion.
*/
static void
ProcessClick OLARGLIST((w, indx, selected))
    OLARG( FlatListWidget,	w )
    OLARG( Cardinal,		indx )
    OLGRA( Boolean,		selected )
{
    InitState(w);

    if (selected)
    {
	if (!Exclusives(w) || NoneSet(w))
	    SelectItem(w, indx, False);
    } else
    {
	SelectItem(w, indx, True);
    }
}

/****************************procedure*header*****************************
    WipeThru- called by way of timer to handle wipe-thru selection and
	auto-scrolling.  This is called when selection motion begins over an
	unselected item and is called subsequently as long as the button is
	down.

	Wipe-thru selection determines which item(s) if any are to be
	selected.  Auto-scrolling determines how many slots if any should be
	scrolled into view.
*/
/* ARGSUSED */
static void
WipeThru OLARGLIST((client_data, id))
    OLARG( XtPointer,		client_data )
    OLGRA( XtIntervalId *,	id )		/* unused */
{
    FlatListWidget	w = (FlatListWidget)client_data;
    FlatListPart *	flp = FLPART(w);
    Window		root, child;
    int			root_x, root_y, x, y;
    unsigned int	buttons;
    Cardinal		indx;
    Boolean		in_pane;
    unsigned long	interval = 0;	/* timer interval for auto-scroll */
    int			scroll_delta;	/* number of 'slots' to scroll in */
    Arg			args[1];

#define IsAbove(w, y)	( (y) < (Position)w->row_column.v_pad )
#define IsBelow(w, y)	( (y) > (Position)(w->core.height - \
						w->row_column.v_pad) )

    /* If there is no starting index (button release must have occurred) or
       widget is being destroyed, return immediately without resetting timer.
    */
    if ((flp->start_indx == OL_NO_ITEM) || w->core.being_destroyed)
	return;

    /* Get position of pointer */
    (void)XQueryPointer(XtDisplay((Widget)w), XtWindow((Widget)w), &root,
			&child, &root_x, &root_y, &x, &y, &buttons);

    /* Pointer is within pane if it's not Above or Below */
    in_pane = ! ( IsAbove(w, y) || IsBelow(w, y) );

    /* 1)  Determine timer interval for auto-scrolling.
       2)  Compute number of 'slots' to scroll in (scroll_delta).
       3)  If we haven't started scrolling, adjust 'y' if necessary so that
	   items above or below the pane are not selected and scrolled in yet.
	   This way we emulate the initial delay of the scrollbar.
    */
    if (!in_pane && VScrollable(w))
    {
	if (!flp->begin_scroll)
	{
	    XtSetArg(args[0], XtNinitialDelay, &interval);
	    XtGetValues(flp->vSBar, args, 1);

	    y = IsAbove(w, y) ?
		w->row_column.v_pad : (w->core.height - w->row_column.v_pad);

	    in_pane		= True;	/* 'y' is now in the pane */
	    flp->begin_scroll	= True;	/* for next time thru */

	} else		/* initial delay is over */
	{
	    if (flp->repeat_rate == 0)
	    {
		XtSetArg(args[0], XtNrepeatRate, &interval);
		XtGetValues(flp->vSBar, args, 1);

		flp->repeat_rate = interval;
	    }

	    interval = flp->repeat_rate;
	}

	/* Compute number of slots to scroll in from below (if pointer is
	   below pane) or from above (if pointer is above pane).

	   'y' may have been adjusted to within pane so re-check 'in_pane'
	*/
	if (in_pane)
	{
	    scroll_delta = 0;

	} else if (IsAbove(w, y))
	{
	    scroll_delta = (y - (int)(MinHeight(w) - 1)) / (int)MinHeight(w);

	    if (scroll_delta < -(int)Top(w))
		scroll_delta = -(int)Top(w);

	} else
	{
	    scroll_delta = (y + (int)(MinHeight(w) - 1)) / (int)MinHeight(w) -
		ViewHeight(w);

	    if (scroll_delta > (NumSlots(w) - (Top(w) + ViewHeight(w))))
		scroll_delta = NumSlots(w) - (Top(w) + ViewHeight(w));
	}

    } else
    {
	interval		= 0;
	flp->begin_scroll	= False;
	scroll_delta		= 0;
    }

    /* Get item "under" pointer.  Note: item may not (yet) be in view.  Use
       OlFlatGetIndex so that we can check for sensitivity.
    */
    indx = OlFlatGetIndex((Widget)w, 0, y, False);

    /* Perform selection only if item is different from current item */

    if ((indx != OL_NO_ITEM) && (indx != CurrentItem(w)))
    {
	/* Do wipe-thru for non-exclusives */
	if (!Exclusives(w))
	    NonexcSelection(w, indx);

	/* Always make item under pointer the current item */
	SetCurrentItem(w, indx);
    }

    if (scroll_delta != 0)
    {
	ScrollList(w, scroll_delta);

	/* Manually position slider */
	XtSetArg(args[0], XtNsliderValue, Top(w));
	XtSetValues(flp->vSBar, args, 1);
    }

    (void)OlAddTimeOut((Widget)w, interval, WipeThru, (XtPointer)w);

#undef IsAbove
#undef IsBelow
}

/* KeyHandler -
 *	Handle KBeginData, KEndData, KBeginLine, KEndLine in Motif mode.
 *
 *	Motif Certification Checklist Item #8-11, us92-13518.
 */
static void
KeyHandler OLARGLIST((w,	ve))
	OLARG( Widget,		w)
	OLGRA( OlVirtualEvent,	ve)
{
#define HScrollable		( FLW(w)->list.need_hsb )
#define KBeginData		ve->virtual_name == OLM_KBeginData
#define KBeginLine		ve->virtual_name == OLM_KBeginLine
#define KEndData		ve->virtual_name == OLM_KEndData
#define KEndLine		ve->virtual_name == OLM_KEndLine

	Cardinal		new_index = (Cardinal)OL_NO_ITEM;

	if ( (new_index = 0,             (KBeginData || KBeginLine))	||
	     (new_index = NumItems(w)-1, (KEndData   || KEndLine  )) )
	{
		OlDefine	op;

		ve->consumed = True;

		if (EmptyList(FLW(w)) || (!VScrollable(w) && !HScrollable))
			return;

		if ( HScrollable &&
		     ((op = OL_SCROLLLEFTEDGE, KBeginLine) ||
		      (op = OL_SCROLLRIGHTEDGE,KEndLine)) )
		{
			Arg	arg[1];
			Widget	hsb,
				sw = SWin(FLW(w));

			XtSetArg(arg[0], XtNhScrollbar, (XtArgVal)&hsb);
			XtGetValues(sw, arg, 1);
			OlActivateWidget(hsb, op, (XtPointer)NULL);
		}
		else if (!InView(w, new_index))
		{
			ViewItem(FLW(w), new_index);
		}
	}

#undef HScrollable
#undef KBeginData
#undef KBeginLine
#undef KEndData
#undef KEndLine
} /* end of KeyHandler */

/* Draw a flat list item  - mostly the same as OlgDrawRectButton, but
 * draws motif mode with a inverse background.
 *
 * Draw a flat list item to fit in the box given by x, y, width, height.
 * The label for the button is drawn by the function labelProc using label
 * as data.  Item appearance is controlled by flags.  Valid flags:
 *
 *	RB_SELECTED		button is selected
 *	RB_NOFRAME		don't draw box
 *	RB_DEFAULT		draw default ring
 *	RB_DIM			stipple borders with 50% gray
 *	RB_INSENSITIVE		stipple everything with 50% gray (this
 *					assumes that the label drawing
 *					functions will correctly stipple
 *					themselves.  With the current
 *					implementation, this is the same
 *					as RB_DIM)
 *
 */

static void
OlgDrawFlatListItem OLARGLIST((scr, win, pInfo, x, y, width, height, label, labelProc, flags, hMargin, vMargin))
    OLARG( Screen *,	scr)
    OLARG( Drawable,	win)
    OLARG( OlgAttrs *,	pInfo)
    OLARG( Position,	x)
    OLARG( Position,	y)
    OLARG( Dimension,	width)
    OLARG( Dimension,	height)
    OLARG( XtPointer,	label)
    OLARG( OlgFlatListLabelProc,labelProc)
    OLARG( OlBitMask,	flags)
    OLARG( Dimension,	hMargin)	/* number of strokes around label */
    OLGRA( Dimension,	vMargin)	/* number of strokes around label */
{
    register _OlgDevice	*pDev;
    Position	xLbl, yLbl;		/* position of label box */
    Position	xLblCorner, yLblCorner;	/* position of lower right of label */
    unsigned	clip;			/* must clip flag */
    XRectangle	clipRect;		/* clip rectangle */
    XRectangle	rects [4];		/* border rectangles */

    pDev = pInfo->pDev;

    /* If the button is too small, then clip */
    if (width < pDev->horizontalStroke*8 || height < pDev->verticalStroke*8)
    {
	clip = True;
	clipRect.x = x;
	clipRect.y = y;
	clipRect.width = width;
	clipRect.height = height;

	width = pDev->horizontalStroke * 8;
	height = pDev->verticalStroke * 8;
    }
    else
	clip = False;

    /* Just about all aspects of draw 3-D buttons is different than 2-D
     * buttons.  Sigh.
     */
    if (OlgIs3d())
    {
	/* Add clip rectangle to GCs */
	if (clip)
	{
	    XSetClipRectangles (DisplayOfScreen (scr), OlgGetBrightGC (pInfo),
				0, 0, &clipRect, 1, YXBanded);
	    XSetClipRectangles (DisplayOfScreen (scr), OlgGetBg1GC (pInfo),
				0, 0, &clipRect, 1, YXBanded);
	    XSetClipRectangles (DisplayOfScreen (scr), OlgGetBg2GC (pInfo),
				0, 0, &clipRect, 1, YXBanded);
	    XSetClipRectangles (DisplayOfScreen (scr), OlgGetBg3GC (pInfo),
				0, 0, &clipRect, 1, YXBanded);
	}

	/* Draw the button background. */
	if (OlGetGui() == OL_OPENLOOK_GUI)  {
		XFillRectangle (DisplayOfScreen (scr), win, flags & RB_SELECTED ?
			OlgGetBg2GC (pInfo) : OlgGetBg1GC (pInfo),
			x, y, width, height);
	}
	else  {
		XFillRectangle (DisplayOfScreen (scr), win, flags & RB_SELECTED ?
			OlgGetBg3GC (pInfo) : OlgGetBg1GC (pInfo),
			x, y, width, height);
	}

	/* If the button is dim, add a stipple to the GCs for border drawing */
	if (flags & RB_DIM || flags & RB_INSENSITIVE)
	{
	    XSetStipple (DisplayOfScreen (scr), OlgGetBrightGC (pInfo),
			 pDev->inactiveStipple);
	    XSetFillStyle (DisplayOfScreen (scr), OlgGetBrightGC (pInfo),
			   FillStippled);
	    XSetStipple (DisplayOfScreen (scr), OlgGetBg3GC (pInfo),
			 pDev->inactiveStipple);
	    XSetFillStyle (DisplayOfScreen (scr), OlgGetBg3GC (pInfo),
			   FillStippled);
	}

	/* Draw border and default ring */
	if (!(flags & RB_NOFRAME))
	    OlgDrawBox (scr, win, pInfo, x, y, width, height,
			flags & RB_SELECTED);
	if (flags & RB_DEFAULT)
	{
	    int	hOffset = pDev->horizontalStroke * 2;
	    int vOffset = pDev->verticalStroke * 2;

	    rects [0].x = x + hOffset;
	    rects [0].y = y + vOffset;
	    rects [0].width = width - hOffset - hOffset;
	    rects [0].height = pDev->verticalStroke;

	    rects [1].x = rects [0].x;
	    rects [1].y = rects [0].y + pDev->verticalStroke;
	    rects [1].width = pDev->horizontalStroke;
	    rects [1].height = height - vOffset * 3;

	    rects [2].x = rects [1].x;
	    rects [2].y = rects [1].y + rects [1].height;
	    rects [2].width = rects [0].width;
	    rects [2].height = pDev->verticalStroke;

	    rects [3].x = rects [0].x + rects [0].width -
		pDev->horizontalStroke;
	    rects [3].y = rects [1].y;
	    rects [3].width = pDev->horizontalStroke;
	    rects [3].height = rects [1].height;

	    XFillRectangles (DisplayOfScreen (scr), win, OlgGetBg3GC (pInfo),
			     rects, 4);
	}

	/* If dim, restore the GCs */
	if (flags & RB_DIM || flags & RB_INSENSITIVE)
	{
	    XSetFillStyle (DisplayOfScreen (scr), OlgGetBrightGC (pInfo),
			   FillSolid);
	    XSetFillStyle (DisplayOfScreen (scr), OlgGetBg3GC (pInfo),
			   FillSolid);
	}

	/* if clipping, restore the GCs */
	if (clip)
	{
	    XSetClipMask (DisplayOfScreen (scr), OlgGetBrightGC (pInfo), None);
	    XSetClipMask (DisplayOfScreen (scr), OlgGetBg1GC (pInfo), None);
	    XSetClipMask (DisplayOfScreen (scr), OlgGetBg2GC (pInfo), None);
	    XSetClipMask (DisplayOfScreen (scr), OlgGetBg3GC (pInfo), None);
	}
    }
    else
    {
	int	borderWidth, borderHeight;

	/* Add clip rectangle to GCs */
	if (clip)
	{
	    XSetClipRectangles (DisplayOfScreen (scr), OlgGetBg1GC (pInfo),
				0, 0, &clipRect, 1, YXBanded);
	    XSetClipRectangles (DisplayOfScreen (scr), OlgGetFgGC (pInfo),
				0, 0, &clipRect, 1, YXBanded);
	}

	/* Draw the button background.  For 2-D buttons the background is drawn
	 * within the default ring only (the window background is assumed to
	 * be drawn around the outside).
	 */
	/* Draw the button background. */
	if (OlGetGui() == OL_OPENLOOK_GUI)  {
		XFillRectangle (DisplayOfScreen (scr), win, OlgGetBg1GC (pInfo),
			x + pDev->horizontalStroke * 3,
			y + pDev->verticalStroke * 3,
			width - pDev->horizontalStroke * 6,
			height - pDev->verticalStroke * 6);
	}
	else  {
		XFillRectangle (DisplayOfScreen (scr), win,
			flags & RB_SELECTED ?
				OlgGetFgGC (pInfo) : OlgGetBg1GC (pInfo),
			x, y, width, height);
	}


	/* If the button is dim, add a stipple to the GCs for border drawing */
	if (flags & RB_DIM || flags & RB_INSENSITIVE)
	{
	    XSetStipple (DisplayOfScreen (scr), OlgGetFgGC (pInfo),
			 pDev->inactiveStipple);
	    XSetFillStyle (DisplayOfScreen (scr), OlgGetFgGC (pInfo),
			   FillStippled);
	}

	/* Draw border, set ring, and default ring */
	if (flags & RB_NOFRAME)
	{
	    x += pDev->horizontalStroke;
	    y += pDev->verticalStroke;
	    width -= 2*pDev->horizontalStroke;
	    height -= 2*pDev->verticalStroke;
	    borderWidth = borderHeight = 0;
	}
	else
	{
	    borderWidth = pDev->horizontalStroke;
	    borderHeight = pDev->verticalStroke;
	}

	if (flags & RB_SELECTED)
	{
	    borderWidth += pDev->horizontalStroke;
	    borderHeight += pDev->verticalStroke;
	    if (flags & RB_DEFAULT)
	    {
		borderWidth += pDev->horizontalStroke;
		borderHeight += pDev->verticalStroke;
	    }
	}

	rects [0].x = x;
	rects [0].y = y;
	rects [0].width = width;
	rects [0].height = borderHeight;

	rects [1].x = x;
	rects [1].y = y + borderHeight;
	rects [1].width = borderWidth;
	rects [1].height = height - borderHeight - borderHeight;

	rects [2].x = x;
	rects [2].y = rects [1].y + rects [1].height;
	rects [2].width = width;
	rects [2].height = borderHeight;

	rects [3].x = rects [0].x + rects [0].width - borderWidth;
	rects [3].y = rects [1].y;
	rects [3].width = borderWidth;
	rects [3].height = rects [1].height;

	if (!(flags & RB_NOFRAME) || flags & RB_SELECTED)
	XFillRectangles (DisplayOfScreen (scr), win, OlgGetFgGC (pInfo),
			 rects, 4);

	if (flags & RB_NOFRAME)
	{
	    x -= pDev->horizontalStroke;
	    y -= pDev->verticalStroke;
	    width += 2*pDev->horizontalStroke;
	    height += 2*pDev->verticalStroke;
	}

	if ((flags & (RB_SELECTED | RB_DEFAULT)) == RB_DEFAULT)
	{
	    int	hOffset = pDev->horizontalStroke * 2;
	    int vOffset = pDev->verticalStroke * 2;

	    rects [0].x = x + hOffset;
	    rects [0].y = y + vOffset;
	    rects [0].width = width - hOffset - hOffset;
	    rects [0].height = pDev->verticalStroke;

	    rects [1].x = rects [0].x;
	    rects [1].y = rects [0].y + pDev->verticalStroke;
	    rects [1].width = pDev->horizontalStroke;
	    rects [1].height = height - vOffset * 3;

	    rects [2].x = rects [1].x;
	    rects [2].y = rects [1].y + rects [1].height;
	    rects [2].width = rects [0].width;
	    rects [2].height = pDev->verticalStroke;

	    rects [3].x = rects [0].x + rects [0].width -
		pDev->horizontalStroke;
	    rects [3].y = rects [1].y;
	    rects [3].width = pDev->horizontalStroke;
	    rects [3].height = rects [1].height;

	    XFillRectangles (DisplayOfScreen (scr), win, OlgGetFgGC (pInfo),
			     rects, 4);
	}

	/* If dim, restore the GCs */
	if (flags & RB_DIM || flags & RB_INSENSITIVE)
	{
	    XSetFillStyle (DisplayOfScreen (scr), OlgGetFgGC (pInfo),
			   FillSolid);
	}

	/* if clipping, restore the GCs */
	if (clip)
	{
	    XSetClipMask (DisplayOfScreen (scr), OlgGetBg1GC (pInfo), None);
	    XSetClipMask (DisplayOfScreen (scr), OlgGetFgGC (pInfo), None);
	}
    }

    /* Calculate the bounding box for the label area. */
    xLbl = x + pDev->horizontalStroke * (3 + hMargin);
    yLbl = y + pDev->verticalStroke * (3 + vMargin);
    xLblCorner = x + width - pDev->horizontalStroke * (3 + hMargin);
    yLblCorner = y + height - pDev->verticalStroke * (3 + vMargin);

    /* Draw the label if any the label area is not completely clipped out */
    if (xLbl < xLblCorner && yLbl < yLblCorner)
    {
	(*labelProc) (scr, win, pInfo, xLbl, yLbl,
	      xLblCorner - xLbl, yLblCorner - yLbl, label, flags);

    }
}  /* end of OlgDrawFlatListItem() */
