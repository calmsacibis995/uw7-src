/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#include <Xol/OlMinStr.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)flat:FButtons.c	1.101"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This file contains the source code for the flat menu buttons
 *	containers.
 *
 ******************************file*header********************************
 */

						/* #includes go here	*/

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookI.h>
#include <Xol/FButtonsP.h>
#include <Xol/PopupMenuP.h>
#include <Xol/VendorI.h>

#define ClassName FlatButtons
#include <Xol/NameDefs.h>


/*****************************file*variables******************************

    Define global/static variables and #defines, and declare externally
    referenced variables.  This must be done before forward declarations
    since functions may return types defined here.

*/

#ifdef DEBUG
#include <stdio.h>
#define FPRINTF(x) fprintf x
#else
#define FPRINTF(x)
#endif

#define ITEMS_TOUCHED(n)					\
    FEPART(n)->preview_item	= (Cardinal) OL_NO_ITEM;	\
    FEPART(n)->current_item	= (Cardinal) OL_NO_ITEM;	\
    FEPART(n)->set_item		= (Cardinal) OL_NO_ITEM;	\
    FEPART(n)->default_item	= (Cardinal) OL_NO_ITEM

#define SHOULD_POST_MENU (drag_right_mode == IN_SUBMENU_REGION)

#define SET_XtNset_TO(flag,w,i)					\
	{							\
		Arg	arg[1];					\
		XtSetArg(arg[0], XtNset, (XtArgVal)flag);	\
		OlFlatSetValues(w, i, arg, 1);			\
	}

typedef enum 
   {
   NOTHING_HAPPENED, 
   SAME_THING_HAPPENED, 
   SELECT_HAPPENED, 
   UNSELECT_HAPPENED,
   SWITCH_HAPPENED
   } FBEvent;

typedef enum 
   {
   NOT_IN_SUBMENU_REGION,
   IN_SUBMENU_REGION,
   RESET_DRAG_RIGHT
   } DragRightMode;

static Position      current_mouse_position = 0;
static DragRightMode drag_right_mode  = RESET_DRAG_RIGHT;

static void     (*_olmFBButtonHandler) OL_ARGS((Widget, OlVirtualEvent));

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

static OlDefine def_pos = OL_LEFT;
static Boolean def_focus_on_sel = False;	/* for O/L */

					/* private procedures */

static void	CheckDragRight OL_ARGS((Widget, FlatButtonsPart *,
					Cardinal, Position, Boolean, Boolean));

static XtResourceDefaultProc
  DefaultSelectColor OL_ARGS((Widget, int, XrmValue *));

static void	ConsumeTakeFocusEH OL_ARGS((Widget, XtPointer,
					   XEvent *, Boolean *));

					/* Calls callbacks for item: */
static Boolean  FBCheckLayoutParameters OL_ARGS((Widget, Widget));
  
static FBEvent  FBDetermineWhatHappened OL_ARGS((Cardinal, Cardinal));
  
static void     FBMenuSetToCurrent OL_ARGS((Widget, FlatButtonsPart *,
					    Cardinal, Cardinal,
					    Position, Position,
					    Boolean, Boolean));
  
#ifdef CLONE
static void	FBPopdownMenu OL_ARGS((Widget));
#endif /*CLONE*/
  
static void     FBSelectKey OL_ARGS((Widget, FlatButtonsPart *,
				     Cardinal, Cardinal));

static void     FBSetItemField OL_ARGS((Widget, String));

#ifdef CLONE
static Bool     WaitForMap OL_ARGS((Display *, XEvent*, char *));
#endif /*CLONE*/

					/* class procedures */

static Boolean	AcceptFocus OL_ARGS((Widget, Time *));

static Boolean	ActivateWidget OL_ARGS((Widget, OlVirtualName, XtPointer));
  
static void     AnalyzeItems OL_ARGS((Widget, ArgList, Cardinal *));
  
static void     ClassInitialize OL_NO_ARGS();

static void	DefaultItemInitialize OL_ARGS((Widget, FlatItem, FlatItem,
						 ArgList, Cardinal *));
  
static Boolean	DefaultItemSetValues OL_ARGS((Widget, FlatItem, FlatItem, 
						FlatItem, ArgList,
						Cardinal *));
  
static void	Destroy OL_ARGS((Widget));
  
static void     HighlightHandler OL_ARGS((Widget, OlDefine));
  
static void	Initialize OL_ARGS((Widget, Widget, ArgList, Cardinal *));

static Boolean  ItemActivate OL_ARGS((Widget, FlatItem, OlVirtualName,
					XtPointer));
  
static void     ItemDimensions OL_ARGS((Widget, FlatItem, Dimension *,
					  Dimension *));
  
static void     ItemInitialize OL_ARGS((Widget, FlatItem, FlatItem,
                                        ArgList, Cardinal *));
  
static Boolean  ItemSetValues	OL_ARGS((Widget, FlatItem, FlatItem,
					 FlatItem, ArgList, Cardinal *));
  
static Boolean	SetValues OL_ARGS((Widget, Widget, Widget,
				     ArgList, Cardinal *));

static Cardinal	TraverseItems OL_ARGS((Widget, Cardinal,
					 OlVirtualName, Time));

					/* action procedures */

static void     ButtonHandler OL_ARGS((Widget, OlVirtualEvent));
  
static void     KeyHandler OL_ARGS((Widget, OlVirtualEvent));
  
					/* public procedures */

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

/* Note: since the 'augment' and 'override' directives don't work for
 * class translations, we have to copy the generic translations and then
 * append what we need.  See the FlatButtonsClassInitialize Procedure.
 */
#if Xt_augment_works_right
Olconst static char
translations[] = "#augment\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction() \n\
	<BtnMotion>:	OlAction()";
#endif

static OlEventHandlerRec
event_procs[] =
{
	{ ButtonPress,	ButtonHandler	},
	{ ButtonRelease,ButtonHandler	},
	{ EnterNotify,	ButtonHandler	},
	{ LeaveNotify,	ButtonHandler	},
	{ MotionNotify, ButtonHandler	},
	{ KeyPress,	KeyHandler	},
};

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

        /* Specify resources that we want the flat class to manage
         * internally if the application doesn't put them in their
         * item fields list.                                            */

static OlFlatReqRsc required_resources[] = {
    { XtNset },
    { XtNdefault },
};


#define OFFSET(f)	XtOffsetOf(FlatButtonsRec, f)

static XtResource resources[] = {

		/* Override some superclass resource values */

  { XtNhSpace, XtCHSpace, XtRDimension, sizeof(Dimension),
    OFFSET(row_column.h_space), XtRImmediate, (XtPointer)OL_IGNORE },

  { XtNvSpace, XtCVSpace, XtRDimension, sizeof(Dimension),
    OFFSET(row_column.v_space), XtRImmediate, (XtPointer)OL_IGNORE },

		/* Declare Container resources	*/

  { XtNdefault, XtCDefault, XtRBoolean, sizeof(Boolean),
    OFFSET(buttons.has_default), XtRImmediate, (XtPointer)FALSE },

  { XtNdim, XtCDim, XtRBoolean, sizeof(Boolean),
    OFFSET(buttons.dim), XtRImmediate, (XtPointer)False },

  { XtNexclusives, XtCExclusives, XtRBoolean, sizeof(Boolean),
    OFFSET(buttons.exclusive_settings), XtRImmediate,(XtPointer)False},

  { XtNnoneSet, XtCNoneSet, XtRBoolean, sizeof(Boolean),
    OFFSET(buttons.none_set), XtRImmediate, (XtPointer)False },

  { XtNbuttonType, XtCButtonType, XtROlDefine, sizeof(OlDefine),
    OFFSET(buttons.button_type), XtRImmediate, (XtPointer)OL_OBLONG_BTN },

  { XtNposition, XtCPosition, XtROlDefine, sizeof(OlDefine),
    OFFSET(buttons.position), XtROlDefine, (XtPointer) &def_pos },

  { XtNinMenu, XtCInMenu, XtRBoolean, sizeof(Boolean),
    OFFSET(buttons.menu_descendant), XtRImmediate, (XtPointer)False },

  { XtNpreview, XtCPreview, XtRWidget, sizeof(Widget),
    OFFSET(buttons.preview), XtRWidget, (XtPointer)NULL },

  { XtNpreviewItem, XtCPreviewItem, XtRCardinal, sizeof(Cardinal),
    OFFSET(buttons.preview_item), XtRImmediate, (XtPointer)OL_NO_ITEM },

  { XtNpostSelect, XtCCallback, XtRCallback, sizeof(XtCallbackList),
    OFFSET(buttons.post_select), XtRCallback, (XtPointer)NULL },

  { XtNfillOnSelect, XtCFillOnSelect, XtRBoolean, sizeof(Boolean),
    OFFSET(buttons.fill_on_select), XtRImmediate, (XtPointer)False },
  
  { XtNmenubarBehavior, XtCMenubarBehavior, XtRBoolean, sizeof(Boolean),
    OFFSET(buttons.menubar_behavior), XtRImmediate, (XtPointer)False },

  { XtNselectColor, XtCSelectColor, XtRPixel, sizeof(Pixel),
    OFFSET(buttons.select_color), XtRCallProc, (XtPointer)DefaultSelectColor },

  { XtNfocusOnSelect, XtCFocusOnSelect, XtRBoolean, sizeof(Boolean),
    OFFSET(buttons.focus_on_select), XtRBoolean, (XtPointer)&def_focus_on_sel },

  { XtNshadowType, XtCShadowType, XtROlDefine, sizeof(OlDefine),
    OFFSET(primitive.shadow_type), XtRImmediate, (XtPointer)OL_SHADOW_OUT
  },

	/* private resource, valid values are: OL_IGNORE, OL_PREV_FIELD,
	   OL_NEXT_FIELD...	*/
  { XtNtraversalType, XtCTraversalType, XtROlDefine, sizeof(OlDefine),
    OFFSET(buttons.traversal_type), XtRImmediate, (XtPointer) OL_IGNORE },

};


				/* Define Resources for sub-objects	*/

#undef OFFSET
#define OFFSET(field) XtOffsetOf(FlatButtonsItemRec, buttons.field)

static OLconst Boolean def_false = False;	/* needed for req resc */

static XtResource item_resources[] = {
  { XtNset, XtCSet, XtRBoolean, sizeof(Boolean),
    OFFSET(set), XtRBoolean, (XtPointer) &def_false },

  { XtNdefault, XtCDefault, XtRBoolean, sizeof(Boolean),
    OFFSET(is_default), XtRBoolean, (XtPointer) &def_false },

  { XtNpopupMenu, XtCPopupMenu, XtRWidget, sizeof(Widget),
    OFFSET(popupMenu), XtRImmediate, (XtPointer)NULL },

  { XtNclientData, XtCClientData, XtRPointer, sizeof(XtPointer),
    OFFSET(client_data), XtRPointer, (XtPointer)NULL },

  { XtNselectProc, XtCCallbackProc, XtRCallbackProc, sizeof(XtCallbackProc),
    OFFSET(select_proc), XtRCallbackProc, (XtPointer)NULL },

  { XtNunselectProc, XtCCallbackProc, XtRCallbackProc, sizeof(XtCallbackProc),
    OFFSET(unselect_proc), XtRCallbackProc, (XtPointer)NULL },

  { XtNbusy, XtCBusy, XtRBoolean, sizeof(Boolean),
    OFFSET(is_busy), XtRBoolean, (XtPointer) &def_false },

  { XtNshadowThickness, XtCShadowThickness, XtRDimension, sizeof(Dimension),
    OFFSET(shadow_thickness), XtRString, (XtPointer)"2 points" },
};

#undef OFFSET

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

FlatButtonsClassRec
flatButtonsClassRec = {
    {
	(WidgetClass)&flatRowColumnClassRec,	/* superclass		*/
	"FlatButtons",				/* class_name		*/
	sizeof(FlatButtonsRec),			/* widget_size		*/
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
	XtInheritResize,			/* resize		*/
	XtInheritExpose,			/* expose		*/
	NULL, /* See ClassInitialize */		/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	AcceptFocus,				/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_offsets	*/
	NULL, /* See ClassInitialize */		/* tm_table		*/
	XtInheritQueryGeometry,			/* query_geometry	*/
	NULL,					/* display_accelerator	*/
	NULL					/* extension		*/
    }, /* End of Core Class Part Initialization */
    {
        False,					/* focus_on_select	*/
	HighlightHandler,			/* highlight_handler	*/
	XtInheritTraversalHandler,		/* traversal_handler	*/
	NULL,					/* register_focus	*/
	ActivateWidget,				/* activate		*/
	event_procs,				/* event_procs		*/
	XtNumber(event_procs),			/* num_event_procs	*/
	OlVersion,				/* version		*/
	(XtPointer)NULL,			/* extension		*/
        {
                (_OlDynResourceList)NULL,       /* resources            */
                (Cardinal)0                     /* num_resources        */
        },                                      /* dyn_data             */
        XtInheritTransparentProc                /* transparent_proc     */
    },	/* End of Primitive Class Part Initialization */
    {
	(XtPointer)NULL,			/* extension		*/
	False,					/* transparent_bg	*/
	XtOffsetOf(FlatButtonsRec, default_item),/* default_offset	*/
	sizeof(FlatButtonsItemRec),		/* rec_size		*/
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
	True					/* allow_class_default	*/
    } /* End of FlatButtons Class Part Initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass flatButtonsWidgetClass = (WidgetClass)&flatButtonsClassRec;

/*
 *************************************************************************
 *
 * Default Procedures
 *
 ***************************default*procedures****************************
 */
/* ARGSUSED */
static XtResourceDefaultProc
DefaultSelectColor OLARGLIST((widget, offset, value_p))
  OLARG( Widget,	widget)
  OLARG( int,		offset)
  OLGRA( XrmValue *,	value_p)
{
  value_p->addr = (XtPointer)(&(((PrimitiveWidget)widget)->
						primitive.input_focus_color));
  return;
}

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * _OlFBCallCallbacks - this routine calls the callbacks for the given
 * item.  The 'set' indicates whether the set or unset callbacks should be
 * called.
 ****************************procedure*header*****************************
 */
extern void
_OlFBCallCallbacks OLARGLIST((w, item_index, use_only_select))
        OLARG( Widget,		w)
        OLARG( Cardinal,	item_index)
        OLGRA( Boolean,		use_only_select)
{
	Arg			args[5];
	XtCallbackProc		select_proc = (XtCallbackProc)NULL;
	XtCallbackProc		unselect_proc = (XtCallbackProc)NULL;
	XtCallbackProc		callback = (XtCallbackProc)NULL;
	XtPointer		client_data = (XtPointer)NULL;
	Boolean			set = False;
	XtPointer		user_data = (XtPointer)NULL;

					/* Get the Item's callbacks	*/

	XtSetArg(args[0], XtNselectProc, &select_proc);
	XtSetArg(args[1], XtNunselectProc, &unselect_proc);
	XtSetArg(args[2], XtNclientData, &client_data);
	XtSetArg(args[3], XtNset, &set);
	XtSetArg(args[4], XtNuserData, &user_data);
	OlFlatGetValues(w, item_index, args, XtNumber(args));

	callback = (use_only_select == True || set == True) ?
	  select_proc : unselect_proc;

						/* Call the callbacks	*/

	if (callback != (XtCallbackProc) NULL)
	{
		OlFlatCallData	call_data;

				/* Set up the call data structure	*/

		call_data.item_index		= item_index;
		call_data.items			= FPART(w)->items;
		call_data.num_items		= FPART(w)->num_items;
		call_data.item_fields		= FPART(w)->item_fields;
		call_data.num_item_fields	= FPART(w)->num_item_fields;
		call_data.user_data		= ((PrimitiveWidget)w)->
		    primitive.user_data;
		call_data.item_user_data	= user_data;

		(* callback)(w->core.self, client_data, &call_data);
	}

        if (XtHasCallbacks(w, XtNpostSelect) == XtCallbackHasSome)
        {
                XtCallCallbacks(w, XtNpostSelect, NULL);
        }
} /* END OF _OlFBCallCallbacks() */

/*
 *************************************************************************
 * FBCheckLayoutParameters - this routine checks the validity of various
 * layout parameters.  If any of the parameters are invalid, a warning
 * is generated and the values are set to a valid value.
 ****************************procedure*header*****************************
 */
static Boolean
FBCheckLayoutParameters OLARGLIST((current, new))
	OLARG( Widget,	current)	/* Current widget or NULL	*/
	OLGRA( Widget,	new)		/* New widget id		*/
{
	String		error_type;
	String		error_msg;
	Boolean		relayout = False;
	FlatButtonsWidget w = (FlatButtonsWidget)new;

	if (current == (Widget) NULL)
	{
		error_type = (String)OleTinitialize;
		error_msg  = (String)OleMinvalidResource_initialize;
	}
	else
	{
		error_type = (String)OleTsetValues;
		error_msg  = (String)OleMinvalidResource_setValues;
	}

			/* Define a macro to speed things up (typing that is)
			 * and make sure that there are no spaces after
			 * the commas when this is used.		*/

#define CLEANUP(field, rsc, string, type, value) \
	OlVaDisplayWarningMsg(XtDisplay(new),OleNinvalidResource,\
		error_type,OleCOlToolkitWarning,error_msg,\
		XtName(new),OlWidgetToClassName(new),rsc,string);\
	RCPART(new)->field = (type)value;\
	if (current == (Widget)NULL ||\
	    RCPART(new)->field != RCPART(current)->field){\
		relayout = True;\
	}

	if (w-> buttons.button_type == OL_RECT_BTN &&
	    w-> buttons.exclusive_settings == True)
	{
	  if (OlGetGui() == OL_OPENLOOK_GUI) {

		if (RCPART(new)->h_space != (Dimension)0 &&
		    RCPART(new)->h_space != (Dimension)OL_IGNORE)
		{
			CLEANUP(h_space, XtNhSpace, "0", Dimension, 0)
		}

		if (RCPART(new)->v_space != (Dimension)0 &&
		    RCPART(new)->v_space != (Dimension)OL_IGNORE)
		{
			CLEANUP(v_space, XtNvSpace, "0", Dimension, 0)
		}
	      }

		if (RCPART(new)->same_width != (OlDefine) OL_COLUMNS &&
		    RCPART(new)->same_width != (OlDefine) OL_ALL)
		{
			CLEANUP(same_width, XtNsameWidth, "OL_COLUMNS",
				OlDefine, OL_COLUMNS);
		}

		if (RCPART(new)->same_height != (OlDefine) OL_ROWS &&
		    RCPART(new)->same_height != (OlDefine) OL_ALL)
		{
			CLEANUP(same_height, XtNsameHeight, "OL_ALL",
				OlDefine, OL_ALL);
		}
	}
#undef CLEANUP

	return(relayout);

} /* END OF FBCheckLayoutParameters() */

/*
 *************************************************************************
 * FBSetItemField - this routine is used to make an item a 'set' item or
 * make an item be the 'default' item.
 ****************************procedure*header*****************************
 */
static void
FBSetItemField OLARGLIST((w, name))
	OLARG( Widget,	w)
	OLGRA( String,	name)
{
	FlatPart *	fp = FPART(w);
	Cardinal	i;
	Arg		args[2];
	Boolean		managed;
	Boolean		mapped_when_managed;
	
	XtSetArg(args[0], XtNmanaged, &managed);
	XtSetArg(args[1], XtNmappedWhenManaged, &mapped_when_managed);
	
	for (i=0; i < fp->num_items; ++i)
	{
		managed = False;
		mapped_when_managed = False;
		OlFlatGetValues(w, i, args, 2);

		if (managed == True &&
		    mapped_when_managed == True)
		{
			XtSetArg(args[0], name, True);
			OlFlatSetValues(w, i, args, 1);
			break;
		}
	}
} /* END OF FBSetItemField() */

/*
 *************************************************************************
 * _OlFBIsSet - this routine returns a zero value if the item is set and a
 * non-zero value if the item is set.
 ****************************procedure*header*****************************
 */
extern int
_OlFBIsSet OLARGLIST((w, item_index))
        OLARG( Widget,          w)
        OLGRA( Cardinal,        item_index)
{
        FlatButtonsPart *	fexcp = FEPART(w);
        int                     is_set;

        if (fexcp->exclusive_settings == True)
        {
                is_set = (fexcp->set_item == item_index);
        }
        else
        {
                Arg     args[1];
                Boolean value;

                XtSetArg(args[0], XtNset, (XtArgVal)&value);
                OlFlatGetValues(w, item_index, args, 1);

                is_set = (value == True);
        }

        return(is_set);

} /* END OF _OlFBIsSet() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * ItemInitialize - this procedure checks a single item to see if
 * it have valid values.
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
	FlatButtonsPart *	fexcp = FEPART(w);
	FlatButtonsItemPart *	item_part = FEIPART(new);
	Cardinal		item_index = new->flat.item_index;

	if (fexcp->button_type != OL_OBLONG_BTN && item_part->is_busy)
		item_part->is_busy = False;

				/* Determine if this item can be the
				 * default item for the setting.	*/

	if (item_part->is_default == True)
	{
		if (fexcp->default_item != (Cardinal) OL_NO_ITEM)
		{
			item_part->is_default = False;

			OlVaDisplayWarningMsg(XtDisplay(w),
				OleNtooManyDefaults, OleTflatState,
				OleCOlToolkitWarning,
				OleMtooManyDefaults_flatState,
				XtName(w), OlWidgetToClassName(w),
				(unsigned)item_index,
				(unsigned)fexcp->default_item);
		}
		else if (fexcp->allow_instance_default == True)
		{
			fexcp->default_item = item_index;
			fexcp->has_default = True;
			_OlSetDefault(w, True);
		}
	}

					/* Determine if this item
					 * can be in the set state.	*/

	if (fexcp->exclusive_settings == True &&
	    item_part->set == True)
	{
		if (fexcp->set_item == (Cardinal) OL_NO_ITEM)
		{
			fexcp->set_item = item_index;
		}
		else if (fexcp->set_item != item_index)
		{
			item_part->set = False;

			OlVaDisplayWarningMsg(XtDisplay(w),
				OleNtooManySet, OleTflatState,
				OleCOlToolkitWarning,
				OleMtooManySet_flatState,
				XtName(w), OlWidgetToClassName(w),
				(unsigned)item_index,
				(unsigned)fexcp->set_item);
		}
	}
} /* END OF ItemInitialize() */

/*
 *************************************************************************
 * ClassInitialize - this procedure initializes the virtual translations
 ****************************procedure*header*****************************
 */
static void
ClassInitialize OL_NO_ARGS()
{
	OLconst char * new_productions = "\
		<Enter>:	OlAction() \n\
		<Leave>:	OlAction() \n\
		<BtnMotion>:	OlAction() \n";

	char *	translations = (char *)XtMalloc(
			strlen(_OlGenericTranslationTable) +
			strlen(new_productions) + 1);

	(void) sprintf(translations, "%s%s", new_productions,
				_OlGenericTranslationTable);
	flatButtonsWidgetClass->core_class.tm_table = translations;

	_OlAddOlDefineType("rectbtn",	OL_RECT_BTN);
	_OlAddOlDefineType("oblongbtn",	OL_OBLONG_BTN);
	_OlAddOlDefineType("checkbox",	OL_CHECKBOX);

			/* Inherit all superclass procedures, but
			 * override a few as well as provide some chained
			 * procedures.  This scheme saves us from
			 * worrying about putting function pointers
			 * in the wrong class slot if they were statically
			 * declared.  It also allows us to inherit
			 * new functions simply be recompiling, i.e.,
			 * we don't have to stick XtInheritBlah into the
			 * class slot.					*/

	OlFlatInheritAll(flatButtonsWidgetClass);

#undef F
#define F	flatButtonsClassRec.flat_class
	
	OLRESOLVESTART
	OLRESOLVE(FBDrawItem, (F.draw_item))
	OLRESOLVE(FBItemLocCursorDims, (F.item_location_cursor_dimensions))
	OLRESOLVEEND(FBButtonHandler, _olmFBButtonHandler)

	F.analyze_items		= AnalyzeItems;
	F.default_initialize	= DefaultItemInitialize;
	F.default_set_values	= DefaultItemSetValues;
	F.initialize		= Initialize;
	F.item_activate		= ItemActivate;
	F.item_dimensions	= ItemDimensions;
	F.item_initialize	= ItemInitialize;
	F.item_set_values	= ItemSetValues;
	F.set_values		= SetValues;
	F.traverse_items	= TraverseItems;
#undef F


	if ( OlGetGui() == OL_MOTIF_GUI )
	{
		def_pos = OL_RIGHT;
		def_focus_on_sel = True;
	}

} /* END OF ClassInitialize() */

/****************************procedure*header*****************************
 * DefaultItemInitialize - checks the initial values of the default item.
 */
/* ARGSUSED */
static void
DefaultItemInitialize OLARGLIST((w, request, new, args, num_args))
	OLARG( Widget,	   w)		/* New flat widget		*/
	OLARG( FlatItem,   request)	/* expanded requested item	*/
	OLARG( FlatItem,   new)		/* expanded new item		*/
	OLARG( ArgList,	   args)
	OLGRA( Cardinal *, num_args)
{
	FEIPART(new)->is_default	= (Boolean)False;

} /* END OF DefaultItemInitialize() */

/****************************procedure*header*****************************
 * DefaultItemSetValues - this routine is called whenever the application
 * does an XtSetValues on the container, possibly requesting that 
 * attributes of the default item be updated.
 * If the "widget" is to be refreshed, the routine returns True.
 */
/* ARGSUSED */
static Boolean
DefaultItemSetValues OLARGLIST((w, current, request, new, args, num_args))
	OLARG( Widget,	   w)		/* New flat widget		*/
	OLARG( FlatItem,   current)	/* expanded current item	*/
	OLARG( FlatItem,   request)	/* expanded requested item	*/
	OLARG( FlatItem,   new)		/* expanded new item		*/
	OLARG( ArgList,	   args)
	OLGRA( Cardinal *, num_args)
{
	/* Always make sure the field 'is_default' in the is False. */

	if (FEIPART(new)->is_default != (Boolean)False)
	{
		FEIPART(new)->is_default = False;
	}

	return((Boolean)FALSE);

} /* END OF DefaultItemSetValues() */

/*
 * Destroy
 * 	simply unregister the menubar if it was registered.
 */
/* ARGSUSED */
static void
Destroy OLARGLIST((w))
  OLGRA( Widget,	w )
{
	FlatButtonsPart *	fexcp = FEPART(w);
	Widget			s;

	if ( fexcp->button_type == OL_OBLONG_BTN &&
	     (s = _OlGetShellOfWidget(w),
	      XtIsSubclass(s, popupMenuShellWidgetClass)) )
	{
		XtRemoveRawEventHandler(
			s, (EventMask)NoEventMask, True,
			ConsumeTakeFocusEH, (XtPointer)NULL);
	}

	if (fexcp->menubar_behavior == True)
		_OlSetMenubarWidget(w, False);
} /* END OF Destroy */

#ifdef CLONE
/*
 * UnbusyPin
 *
 * This callback procedure is used to reset the pushpin resource
 * of a shell which had been pinned then unpinned.
 *
 */
/* ARGSUSED */
static void
UnbusyPin OLARGLIST((shell, client_data, call_data))
    OLARG( Widget,	shell)		/* unused */
    OLARG( XtPointer,	client_data)
    OLGRA( XtPointer,	call_data)	/* unused */
{
   Arg arg[1];

   XtSetArg(arg[0], XtNpushpin, OL_OUT);
   XtSetValues((Widget)client_data, arg, 1);

} /* end of UnbusyPin */

/*
 * MakeClone
 *
 * This procedure creates a clone shell and item list for
 * a newly created flat button widget.  Later the callback
 * PostClone will be used to map this clone when the pushpin
 * of the PopupMenuShell widget is pinned.
 *
 */
static void
MakeClone OLARGLIST((shell, new, fexcp, args, num_args))
  OLARG( WMShellWidget,	  shell)
  OLARG( Widget,            new)
  OLARG( FlatButtonsPart *, fexcp)
  OLARG( Arg,               args[])
  OLGRA( Cardinal,          num_args)
{
   Arg                   arg[30];
   Arg *                 copy;
   Cardinal              i;
   Widget                new_shell;
   Widget                new_items;

   if (fexcp-> clone == (Widget)NULL)
      {
      /*
       * Create a suitable shell
       */
      Widget parent = _OlGetShellOfWidget(new /* _OlRootOfMenuStack() */);

      if (parent == NULL)
         parent = XtWindowToWidget(XtDisplay(shell), 
                                   RootWindowOfScreen(XtScreen(shell)));

      XtSetArg(arg[0], XtNpushpin,       OL_IN);
      XtSetArg(arg[1], XtNtitle,         shell-> wm.title);
      XtSetArg(arg[2], XtNresizeCorners, False);
      XtSetArg(arg[3], XtNwinType,       OL_WT_CMD);
      new_shell = XtCreatePopupShell("title", 
         transientShellWidgetClass, parent, arg, 4);

      /*
       * make sure the flat thinks it's a menu descendant
       */

      if (num_args > XtNumber(arg) + 1)
         copy = (Arg *)XtMalloc(sizeof(Arg) * (num_args + 1));
      else
         copy = arg;
      for (i = 0; i < num_args; i++)
         copy[i] = args[i];
      XtSetArg(copy[i], XtNinMenu, True);
      new_items = XtCreateManagedWidget("fm", flatButtonsWidgetClass, 
                                        new_shell, copy, i + 1);
      if (copy != arg)
         XtFree(copy);

      ((FlatButtonsWidget)new_items)-> buttons.clone = new;

      XtAddCallback(new_shell, XtNpopdownCallback, UnbusyPin, shell);
      fexcp-> clone_shell = NULL;
      fexcp-> clone       = new_items;
      }

} /* end of MakeClone */

/*
 * PostClone
 *
 * This callback procedure is invoked when the pushpin of the
 * parent shell is toggled.  It posts a cloned popup (created 
 * in MakeClone) if the call_data argument is set.  It always
 * resets the stay_up_mode so that the spring_loaded release
 * handler will popdown the cascade.
 *
 */
static void
PostClone OLARGLIST((shell, client_data, call_data))
  OLARG( WMShellWidget,	shell)
  OLARG( XtPointer,	client_data)
  OLGRA( XtPointer,	call_data)
{
   FlatButtonsWidget     nw    = (FlatButtonsWidget)client_data;
   int                   post  = (int)call_data;
   FlatButtonsPart *     fexcp = FEPART(nw);
   XEvent                event;
   Arg                   arg[5];

   if (post)
      {
      if (!fexcp-> clone_shell)
         {
         fexcp-> clone_shell = XtParent(fexcp-> clone);
         XtSetArg(arg[0], XtNx,             shell-> core.x);
         XtSetArg(arg[1], XtNy,             shell-> core.y);
         XtSetValues(fexcp-> clone_shell, arg, 2);
         ((WMShellWidget)fexcp-> clone_shell)-> 
            wm.size_hints.flags |= USPosition;
         }

      XtPopup(fexcp-> clone_shell, XtGrabNone);
      XPeekIfEvent(XtDisplay(fexcp-> clone_shell), &event, 
         WaitForMap, (char *)XtWindow(fexcp-> clone_shell));
      }

   _OlResetStayupMode((Widget)shell);

} /* end of PostClone */
#endif /*CLONE*/

/*
 * KeyHandler
 *
 * This procedure is used to consume any KeyPress events which
 * are directed to the root of a menu because of the spring loaded
 * feature of the Intrinsics.  Basically any event which is a key
 * press whose window does not match the window of the widget to
 * which it is being delivered is consumed.  This prevents the
 * root of a menu from processing events which would not (in the
 * absence of the spring loaded attribute) have been delivered to it.
 *
 */
/* ARGSUSED */
static void
KeyHandler OLARGLIST((w, ve))
  OLARG( Widget,    w)
  OLGRA( OlVirtualEvent,	ve)
{
	Boolean in_menu = (FEPART(w)->button_type == OL_OBLONG_BTN &&
				! FEPART(w)->menubar_behavior &&
				FEPART(w)->menu_descendant);

	if (!_OlIsEmptyMenuStack(w) && (w == (Widget)_OlRootOfMenuStack(w)))
	{
		ve->consumed = True;
		return;
	}

	/*  When flat oblong buttons are in a menu, the behavior of the
	    up and down arrows should be identical to the tab key.  */
	if (ve->virtual_name == OL_MOVEUP && in_menu)
		ve->virtual_name = OL_PREV_FIELD;

	if (ve->virtual_name == OL_MOVEDOWN && in_menu)
		ve->virtual_name = OL_NEXT_FIELD;

	if ((ve->virtual_name == OL_NEXT_FIELD ||
	     ve->virtual_name == OL_PREV_FIELD) &&
	    FEPART(w)->button_type == OL_OBLONG_BTN &&
	    FPART(w)->focus_item != (Cardinal)OL_NO_ITEM)
	{
		Time		timestamp = ve->xevent->xkey.time;


			/* this is the easy case...	*/
	if ((FPART(w)->focus_item == FPART(w)->num_items - 1 &&
	     ve->virtual_name == OL_NEXT_FIELD) ||
	    (FPART(w)->focus_item == 0 &&
	     ve->virtual_name == OL_PREV_FIELD))
	{
			/* We know focus will be in next/prev widget,
			 * so reset last_focus_item now.
			 */
		FPART(w)->last_focus_item = (Cardinal)OL_NO_ITEM;
	}
	else /* now we handle the harder case...		*/
	{
		OL_FLAT_ALLOC_ITEM(w, FlatItem, item);
		int	i;

		if (ve->virtual_name == OL_NEXT_FIELD)
		{
			for (i = FPART(w)->focus_item + 1;
				i < FPART(w)->num_items; i++)
			{
				OlFlatExpandItem(w, (Cardinal)i, item);

				if ((*OL_FLATCLASS(w).item_accept_focus)
					(w, item, &timestamp) == True)
				{
					ve->consumed = True;
					break;
				}
			}
		}
		else
		{
			for (i = FPART(w)->focus_item - 1; i >= 0; i--)
			{
				OlFlatExpandItem(w, (Cardinal)i, item);

				if ((*OL_FLATCLASS(w).item_accept_focus)
					(w, item, &timestamp) == True)
				{
					ve->consumed = True;
					break;
				}
			}
		}

			/* No one can take focus, same as "if" */
		if (ve->consumed == False)
			FPART(w)->last_focus_item =(Cardinal)OL_NO_ITEM;

		OL_FLAT_FREE_ITEM(item);
	}

	}
} /* end of KeyHandler */

/*
 *************************************************************************
 * Initialize - this procedure initializes the instance part of the widget
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize OLARGLIST((request, new, args, num_args))
	OLARG( Widget,		request)
	OLARG( Widget,		new)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	FlatButtonsPart *	fexcp  = FEPART(new);
	Widget			shell  = _OlGetShellOfWidget(new);
	Boolean			in_menu,
				inMenu = XtIsSubclass(shell,
						popupMenuShellWidgetClass);
	OlDefine		current_gui = OlGetGui();

	in_menu = inMenu;

	if (fexcp-> button_type != OL_OBLONG_BTN &&
	    fexcp-> button_type != OL_RECT_BTN &&
	    fexcp-> button_type != OL_CHECKBOX)
	{
			/* maybe a warning but why the overhead... */
		fexcp-> button_type  = OL_OBLONG_BTN;
	}

	fexcp->set_item             = (Cardinal)OL_NO_ITEM;
#ifdef CLONE
	fexcp->clone_shell          =
	fexcp->clone                = NULL;
#endif /*CLONE*/
	fexcp->being_reset          = False;
	fexcp->drag_right_x	    = 0;

					/* Initialize the instance data	*/
#ifdef CLONE
	if (inMenu)
	{
	        Arg		arg[1];
		Boolean 	pin = False;
		OlDefine	pushpin_state;

 		XtSetArg(arg[0], XtNpushpin, (XtArgVal)&pushpin_state);
 		XtGetValues(shell, arg, 1);
		switch(pushpin_state)
		{
			case OL_IN:
				pin = True;
				/*FALL THROUGH*/
			case OL_BUSY:
			case OL_OUT:
				MakeClone((WMShellWidget)shell, new, fexcp, 
					args, *num_args);
				XtAddCallback(shell, XtNpin, PostClone, new);
				if (pin)
					XtCallCallbacks(shell, XtNpin, NULL);
				break;
			case OL_NONE:
				break;
		}
	}
#endif /*CLONE*/


	fexcp->menu_descendant = fexcp->menu_descendant || inMenu;

			/* Set up the horizontal and vertical spacing	*/

	if (fexcp-> button_type == OL_OBLONG_BTN)
	{
		if (!in_menu)
		{
				/* Xt only increases "count" when
				 * inserting a same EH more than once.
				 * So we don't need to worry about the
				 * overhead in this case...
				 */
			XtInsertRawEventHandler(
				shell, (EventMask)NoEventMask, True,
				ConsumeTakeFocusEH, (XtPointer)NULL,
				XtListHead);
		}

		if (fexcp->menubar_behavior == True)
		{
			_OlSetMenubarWidget((Widget)new, True);

				/* See Comments in SetValues... */
			if (current_gui == OL_MOTIF_GUI)
				_OlDeleteDescendant(new);
		}

		fexcp->none_set = True;
		fexcp->exclusive_settings = False;
		if (current_gui == OL_OPENLOOK_GUI)
		{
		   RCPART(new)->h_space = 
			OlScreenPointToPixel(OL_HORIZONTAL, 3, XtScreen(new));
		   RCPART(new)->v_space = 
			OlScreenPointToPixel(OL_VERTICAL, 3, XtScreen(new));
		}
		else
		{
			fexcp->menu_descendant = fexcp->menu_descendant ||
				fexcp->menubar_behavior;

		}
	}
	else
	{
		fexcp->menubar_behavior = False;

		if (RCPART(new)->h_space == (Dimension) OL_IGNORE)
		{
			if (current_gui == OL_MOTIF_GUI)
				RCPART(new)->h_space = (Dimension)0;
			else
			{
				RCPART(new)->h_space = (Dimension)
				  ((fexcp->exclusive_settings == True) ? 0 :
					   OlScreenPointToPixel(OL_HORIZONTAL,
						(OL_DEFAULT_POINT_SIZE / 2),
						XtScreen(new)));
			}
		}
	  
		if (RCPART(new)->v_space == (Dimension) OL_IGNORE)
		{
			if (current_gui == OL_MOTIF_GUI)
				RCPART(new)->v_space = (Dimension) 0;
			else
			{
				RCPART(new)->v_space = (Dimension)
				  ((fexcp->exclusive_settings == True) ? 0 :
					OlScreenPointToPixel(OL_VERTICAL,
						(OL_DEFAULT_POINT_SIZE / 2),
						XtScreen(new)));
			}
		}
	}

						/* Set the item overlap	*/

	RCPART(new)->overlap = OlgIs3d() ? 0 :
				OlgGetHorizontalStroke (FPART(new)->pAttrs);

			/* Modify the XtNnoneSet resource if this is not
			 * an exclusive settings			*/

	if (fexcp->exclusive_settings == False && fexcp->none_set == False)
	{
		fexcp->none_set = True;
	}
	
				/* See if this instance is permitted to
				 * have a default sub-object		*/

	fexcp->allow_instance_default =
	  (Boolean)(FECPART(new)->allow_class_default == False ||
		    (fexcp->menu_descendant == False &&
		     fexcp->button_type != OL_OBLONG_BTN) ? False : True);

	(void) FBCheckLayoutParameters((Widget)NULL, new);

		/* flat will handle Motif Location Cursor and
		 * can't take advantage of "border" as other
		 * primitives do, so back the changes in
		 * superclass...
		 */
	if (current_gui == OL_MOTIF_GUI)
		((PrimitiveWidget)new)->core.border_width = 0;

	if (FPART(new)->items_touched == True)
	{
		ITEMS_TOUCHED(new);
	}
} /* END OF Initialize() */

/****************************procedure*header*****************************
 * AcceptFocus...
 */
static Boolean
AcceptFocus OLARGLIST((w, time))
	OLARG( Widget,		w)
	OLGRA( Time *,		time)
{
	Boolean		ret_val = False;

#define SUPERCLASS \
	((FlatButtonsClassRec *)flatButtonsClassRec.core_class.superclass)
#define TraversalType	FEPART(w)->traversal_type
#define LastItem	FPART(w)->num_items - 1
#define ITEM_AF		OL_FLATCLASS(w).item_accept_focus

	if (FEPART(w)->button_type != OL_OBLONG_BTN ||
	    TraversalType == OL_IGNORE)
	{
		TraversalType = OL_IGNORE;
		return (*SUPERCLASS->core_class.accept_focus)(w, time);
	}

	if (OlCanAcceptFocus(w, *time) == True)
	{
		Cardinal	t;

		t = TraversalType == OL_NEXT_FIELD ? 0 : LastItem;

		ret_val = OlFlatCallAcceptFocus(w, t, *time);

		if (ret_val == False && ITEM_AF)
		{
			Cardinal	i;
			OL_FLAT_ALLOC_ITEM(w, FlatItem, item);

			if (TraversalType == OL_NEXT_FIELD)
			{
				for (i = 1; i <= LastItem; i++)
				{
				   OlFlatExpandItem(w, i, item);

				   if ((*ITEM_AF)(w, item, time) == True)
				   {
					ret_val = True;
					break;
				   }
				}
			}
			else
			{
				Cardinal	j;

					/* The logic should be:
					 *   for (i = LastItem-1; i >= 0; i--)
					 * but "i" is a Cardinal (unsigned int)
					 * so can't check "< 0" situation.
					 */
				for (i = 0; i < LastItem; i++)
				{
					/* Have to do it in reverse order */
				   j = LastItem - 1 - i;

				   OlFlatExpandItem(w, j, item);

				   if ((*ITEM_AF)(w, item, time) == True)
				   {
					ret_val = True;
					break;
				   }
				}
			}

			OL_FLAT_FREE_ITEM(item);
		}
	}

	TraversalType = OL_IGNORE;
	return(ret_val);

#undef SUPERCLASS
#undef TraversalType
#undef LastItem
#undef ITEM_AF
} /* end of AcceptFocus */

/****************************procedure*header*****************************
 * ActivateWidget- this is for compatibility with the old MenuButton. To
 * fire the default, the old MenuButton activates the flat with 'data'
 * set to OL_NO_ITEM.  Here we adjust 'data' to be the default item in
 * this case and let the superclass do the real work.
 *
 * This is not needed when the old MenuButton is no longer supported.
 */
static Boolean
ActivateWidget OLARGLIST((w, type, data))
    OLARG( Widget,		w)
    OLARG( OlVirtualName,	type)
    OLGRA( XtPointer,		data)
{
	/* this first part is new for MooLIT and must stay	*/
	/* regardless of the old  MenuButton.			*/
    if (type == OL_MENUBARKEY && FEPART(w)->menubar_behavior == True) {
	Time	time = CurrentTime;

	FPART(w)->last_focus_item = OL_NO_ITEM;
	if (!_OlIsEmptyMenuStack(w))
	{
			/* unpost the menu and let VendorShell does
			 * proper focus assignment...
			 */
		OlActivateWidget(
			_OlTopOfMenuStack(w), OL_CANCEL, (XtPointer)NULL);
	}
	else
	{
			/* Basically, just does a toggle here, i.e.,
			 * if menubar has focus then revert it to the
			 * previous one otherwise let menubar has
			 * the focus...
			 */
		if (w != OlGetCurrentFocusWidget(w))
			XtCallAcceptFocus(w, &time);
		else
			XtCallAcceptFocus(_OlGetShellOfWidget(w), &time);
	}
	return (True);
    }
    
    else {
	Boolean		ret_val = False;
	OlActivateFunc	activate = ((PrimitiveWidgetClass)
			    flatButtonsWidgetClass->core_class.superclass)->
				primitive_class.activate;


	if (activate != NULL)
	{
			/* *data* is passed in as OL_NO_ITEM when a
			 * *shell* handles OL_DEFAULTACTION (usually call
			 * OlActivateWidget with type == OL_SELECTKEY...
			 */
		if ((Cardinal)data == OL_NO_ITEM)
		{
				/* Motif check list #3-6, ul92-13355,
				 * default ring should follow focus item,
				 * so the functionality...
				 */
			if ( FEPART(w)->button_type == OL_OBLONG_BTN &&
			     FPART(w)->focus_item != OL_NO_ITEM &&
			     OlGetGui() == OL_MOTIF_GUI )
				data = (XtPointer)(FPART(w)->focus_item + 1);
			else
				data = (XtPointer)(FEPART(w)->default_item + 1);
		}
	  
		ret_val = (*activate)(w, type, data);
	  
	}
      
      return (ret_val);
    }
} /* end of ActivateWidget */

/*
 *************************************************************************
 * AnalyzeItems - this routine analyzes new items for this class.
 * This routine is called after all of the items have been initialized,
 * so we can assume that all items have valid values.
 * Also, the ItemInitialize routine has already found the set item and
 * the default item (if applicable), provided the application set them.
 * If not, this routine will choose them.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
AnalyzeItems OLARGLIST((w, args, num_args))
	OLARG( Widget,		w)		/* flat widget id	*/
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	FlatButtonsPart *	fexcp = FEPART(w);
	
				/* Now, if none set is False, we have to
				 * set an item to be true if no items
				 * are set				*/

	if (fexcp->set_item == (Cardinal) OL_NO_ITEM &&
	    fexcp->exclusive_settings == True &&
	    fexcp->none_set == False)
	{
		FBSetItemField(w, XtNset);
	}


                        /* If a default is allowed, attempt to set
                         * one.  Note, we turn off the 'has_default'
                         * flag since the ItemSetValues will set it
                         * to TRUE if an item becomes the default.      */

        if (fexcp->has_default == True &&
            fexcp->default_item == (Cardinal) OL_NO_ITEM)
        {
                fexcp->has_default = False;

                if (fexcp->allow_instance_default == True)
                {
                        FBSetItemField(w, XtNdefault);
                }
        }

} /* END OF AnalyzeItems() */

/*
 *************************************************************************
 * HighlightHandler - this routine is called whenever the flattened widget
 * container gains or looses focus.
 * We need this routine because the flattened exclusives widgets do
 * want want to remember the last_focus_item.  Therefore, we'll let
 * our superclass do the really work, and then we'll unset this field.
 ****************************procedure*header*****************************
 */
static void
HighlightHandler OLARGLIST((w, type))
        OLARG( Widget,          w)
        OLGRA( OlDefine,        type)
{
        FlatWidgetClass fwc = (FlatWidgetClass)
                          flatButtonsWidgetClass->core_class.superclass;

	Cardinal	focus_item = FPART(w)->focus_item;
	Cardinal	default_item = FEPART(w)->default_item;
	Cardinal	last_focus_item = FPART(w)->last_focus_item;

        (*fwc->primitive_class.highlight_handler)(w, type);

		/* Motif check list #3-6, ul92-13355,
		 * default ring should follow focus item...
		 */
	if ( FEPART(w)->button_type == OL_OBLONG_BTN &&
	     default_item != OL_NO_ITEM &&
	     ((type == OL_IN  && default_item != focus_item) ||
	      (type == OL_OUT && default_item != last_focus_item)) &&
	     OlGetGui() == OL_MOTIF_GUI )
	{
		OlFlatRefreshItem(w, default_item, True);
	}

	if (type == OL_IN)	/* do nothing */
		return;

	/* else type is OL_OUT...		*/
	
		/* if in menu (but NOT menubar), focus goes to default item */
	if (FEPART(w)->menubar_behavior == False &&
	    FEPART(w)->menu_descendant == True)
	{
		Boolean		is_option_menu = False;

		if (OlGetGui() == OL_MOTIF_GUI)
		{
			Arg		arg[1];

			XtSetArg(arg[0], XtNoptionMenu, &is_option_menu);
			XtGetValues(_OlGetShellOfWidget(w), arg, 1);
		}

		if (!is_option_menu)
			FPART(w)->last_focus_item = FEPART(w)->default_item;
	}
	else
	{
		if (FEPART(w)->button_type != OL_OBLONG_BTN)
			FPART(w)->last_focus_item = (Cardinal)OL_NO_ITEM;
	}

} /* END OF HighlightHandler() */

/*
 *************************************************************************
 * ItemActivate - this routine is used to activate this widget.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
ItemActivate OLARGLIST((w, item, type, data))
	OLARG( Widget,		w)
	OLARG( FlatItem,	item)
	OLARG( OlVirtualName,	type)
	OLGRA( XtPointer,	data)
{
	FlatButtonsPart *		fexcp	= FEPART(w);
	Boolean				ret_val = TRUE;
	Cardinal			current	= fexcp->current_item;
	FlatButtonsItemPart * 		item_part;
	Cardinal			focus_item = item->flat.item_index;

	FPRINTF((stderr,
		 "item activate for item %x (%d) in widget %x: ",
		 item, focus_item, w));

	item_part = FEIPART(item);
	
	if (fexcp->button_type == OL_OBLONG_BTN && item_part->is_busy)
	{
		return(ret_val);
	}
	switch(type) {
	case OL_MENUKEY:
		if (item_part-> popupMenu)
		  {
		    _OlFBCallCallbacks(w, focus_item, True);
		    FBSelectKey(w, fexcp, focus_item, current);
		  }
		else ret_val = FALSE;
		break;
	case OL_DEFAULTACTION:
		FPRINTF((stderr, "defaultaction\n"));
		_OlResetStayupMode(w);
		_OlSetPreviewMode(w);
		break;
	case OL_SELECTKEY:
                FPRINTF((stderr,"selectkey\n"));
		if (OlGetGui() == OL_OPENLOOK_GUI) {
		  if (item_part->popupMenu) {
		    _OlResetStayupMode(w);
		    _OlSetPreviewMode(w);
		  }
		}
		if (item_part-> popupMenu)
		  {
		    _OlFBCallCallbacks(w, focus_item, True);
		  }
		FBSelectKey(w, fexcp, focus_item, current);
		break;
	case OL_MENUDEFAULTKEY:
                FPRINTF((stderr,"menudefaultkey\n"));
		_OlFBSetDefault(w, True, 0, 0, OL_NO_ITEM);
		break;
	default:
                FPRINTF((stderr,"default %d key\n", type));
		ret_val = FALSE;
	}

	return(ret_val);
 
} /* END OF ItemActivate() */

/*
 *************************************************************************
 * ItemDimensions - this routine determines the size of a single sub-object
 ****************************procedure*header*****************************
 */
static void
ItemDimensions OLARGLIST((w, item_rec, width, height))
	OLARG( Widget,			w)
	OLARG( FlatItem,		item_rec)
	OLARG( register Dimension *,	width)
	OLGRA( register Dimension *,	height)
{
	FlatPart *		fp        = FPART(w);
	FlatButtonsItemPart * 	item_part = FEIPART(item_rec);
	FlatButtonsPart *	fexcp     = FEPART(w);
	int			flags     = 0;
	void			(*sizeProc)();
	XtPointer		lbl;
	OlFlatScreenCache *	sc;
	OlDefine		current_gui = OlGetGui();
	Dimension		padding;

	switch (fexcp-> button_type)
	{
		default:
			FPRINTF((stderr,"in widget '%s'\n ", w-> core.name));
			FPRINTF((stderr,"error error error!!!\n"));
			break;
		case OL_OBLONG_BTN:

			/*
 	 		 * Add (potential) menu button component to visual size.
 	 		 */

			if (fexcp->menu_descendant)
				flags |= OB_MENUITEM;

			if (item_part->popupMenu)  {
       				if (RCPART(w)->layout_type == OL_FIXEDCOLS)
					flags |= OB_MENU_R;
				else
					flags |= OB_MENU_D;
			}

			/* Get label information */

			OlFlatSetupLabelSize (w, item_rec, &lbl, &sizeProc);

			OlgSizeOblongButton (XtScreen (w), fp->pAttrs, 
				lbl, sizeProc, flags, width, height);
			break;
		case OL_RECT_BTN:
			if (current_gui == OL_OPENLOOK_GUI) {
			  OlFlatSetupLabelSize (w, item_rec, &lbl,
						&sizeProc); 

			  OlgSizeRectButton (XtScreen (w), 
					     fp->pAttrs, lbl, sizeProc, 0,
					     width, height);
			  break;
			}
			/*FALLTHROUGH*/
		case OL_CHECKBOX:
			sc = OlFlatScreenManager(w,
				OL_DEFAULT_POINT_SIZE, OL_JUST_LOOKING);

			OlFlatSetupLabelSize (w, item_rec, &lbl, &sizeProc);

			(*sizeProc) (XtScreen (w), fp->pAttrs, lbl, 
					width, height);

			/* Add the horizontal padding.	*/

			padding = (current_gui == OL_OPENLOOK_GUI) ?
					PADDING : MPADDING;
			*width	+= OlgGetHorizontalStroke(fp->pAttrs) 
						* padding + sc->check_width;
		
			/* If the checkbox is taller, use its height */

			if (*height < sc->check_height)
			{
				*height = sc->check_height;
			}

			break;
	}

	/* Add location cursor to height and width in Motif Mode */

	/* See FBIninitialize and _OlmFBDrawItem for details...	*/
#define SHADOWTHICKNESS	item_part->shadow_thickness
#define HILITETHICKNESS ((PrimitiveWidget)w)->primitive.highlight_thickness
#define PSIZE(x)	(int)_OlMax(OlScreenPointToPixel(		\
					OL_HORIZONTAL, x, XtScreen(w)), \
				 OlScreenPointToPixel(			\
					OL_VERTICAL, x, XtScreen(w)))

	if (current_gui == OL_MOTIF_GUI) {

		  /* account for shadow thickness */
	  if (fexcp->button_type == OL_OBLONG_BTN)
	  {
		Dimension	val = 2 * SHADOWTHICKNESS;

		*width  += val;
		*height += val;
	  }

	  /* add menu mark width, if necessary */

	  if ( (flags & OB_MENUMARK) &&
	       (!(flags & OB_MENUITEM) || (flags & OB_MENU_D) != OB_MENU_D) )
	  {
	    *width += OlgDrawMenuMark(NULL, NULL, fp->pAttrs, 0, 0, 0, 0,
				      MM_JUST_SIZE);
	  }
	    
	  /* default ring and location cursor only when not in a menu */

	  if (fexcp->button_type != OL_OBLONG_BTN || !(flags & OB_MENUITEM))
	  {

		/* default ring is 4 pts, double it for both sides.  */
	    if (fexcp->button_type == OL_OBLONG_BTN) {
		Dimension	val = 2 * PSIZE(4);

		*width  += val;
		*height += val;
	    }
	    
	    /* account for location cursor */
	    
	    *width  += 2 * HILITETHICKNESS;
	    *height += 2 * HILITETHICKNESS;
	  }
	}

#undef PSIZE
#undef SHADOWTHICKNESS
#undef HILITETHICKNESS

} /* END OF ItemDimensions() */

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
	Cardinal		item_index 	= new->flat.item_index;
	Boolean			redisplay 	= False;
	FlatButtonsPart *	fexcp 		= FEPART(w);
	FlatButtonsItemPart *	npart 		= FEIPART(new);
	FlatButtonsItemPart *	cpart 		= FEIPART(current);
#ifdef CLONE
	static int              recur_lock      = 0;

	if (fexcp-> clone && !recur_lock)
	{
		recur_lock = 1;
		OlFlatSetValues(fexcp-> clone, item_index, args, *num_args);
		recur_lock = 0;
	}
#endif /*CLONE*/

#define DIFFERENT(field)	(npart->field != cpart->field)

	if (DIFFERENT(is_busy))
	{
		if (fexcp->button_type == OL_OBLONG_BTN)
			redisplay = True;
		else
			npart->is_busy = False;
	}

                /* If the 'set' state changed, update the internal
                 * fields.  Note, some times its ok to redraw
                 * the item without indirectly causing an XClearArea
                 * request.  When this optimization can occur, a call
                 * to OlFlatRefreshExpandedItem is done instead of
                 * setting the 'redisplay' flag to True.
		 */

	if (DIFFERENT(set))
	{
		if (fexcp->exclusive_settings == (Boolean)False)
		{
		    if (npart->set == True)
		    {
			OlFlatRefreshExpandedItem(w, new, False);
		    }
		    else
		    {
			redisplay = True;
		    }
		}
		else				/* Exclusives setting	*/
		{
		    if (npart->set == True)
		    {
				/* If there's no currently set item,
				 * make this one be the set one		*/

			if (fexcp->set_item == (Cardinal)OL_NO_ITEM)
			{
				fexcp->set_item = item_index;
				OlFlatRefreshExpandedItem(w, new, False);
			}
			else if (fexcp->set_item != item_index)
			{
				/* There is an item currently set, so
				 * make 'item_index' be the new set
				 * item and then unset the old one	*/

				Cardinal	old = fexcp->set_item;

						/* set the new item	*/

				fexcp->set_item = item_index;

					/* Unset the previous item	*/

				SET_XtNset_TO(False, w, old)

				  
					/* Now refresh this item without
					 * generating an exposure.	*/

				OlFlatRefreshExpandedItem(w, new, False);
			}
		    }
		    else		/* npart->set == False */
		    {
			if (fexcp->set_item == item_index)
			{
				/* If none_set is true, we can unset this
				 * item; else, ignore the request.	*/

				if (fexcp->none_set == True)
				{
					fexcp->set_item = (Cardinal)OL_NO_ITEM;
					redisplay = True;
				}
			}
			else
			{
				redisplay = True;
			}
		    }
		}
	} /* end of DIFFERENT(set) */

	if (DIFFERENT(is_default))
	{
		if (fexcp->allow_instance_default == (Boolean)True)
		{
			redisplay = True;

			if (fexcp->default_item == (Cardinal)OL_NO_ITEM)
			{
				fexcp->default_item	= item_index;
				fexcp->has_default	= True;
				_OlSetDefault(w, True);
		    	}
			else if (fexcp->default_item == item_index)
			{
				fexcp->has_default	= False;
				fexcp->default_item	= (Cardinal)OL_NO_ITEM;
				_OlSetDefault(w, False);
			}
			else if (npart->is_default == True)
			{
				/* Case where this item wants to
				 * be the default and some other item
				 * is the current (but not for long)
				 * default item.  Set the index of the
				 * new default item and then turn off
				 * the old default item.                */

				Cardinal        old = fexcp->default_item;
				Arg             args[1];

				fexcp->default_item = item_index;

				XtSetArg(args[0], XtNdefault, False);
				OlFlatSetValues(w, old, args, 1);
			}
		}
		else
		{
			npart->is_default = False;
		}
	}

#undef DIFFERENT
	return(redisplay);

} /* END OF ItemSetValues() */

/*
 *************************************************************************
 * SetValues - this procedure monitors the changing of instance data
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
	Boolean			redisplay 	= False;
	FlatPart *		fp		= FPART(new);
	FlatButtonsPart *	fexcp		= FEPART(new);
	FlatButtonsPart *	cfexcp		= FEPART(current);

#ifdef CLONE
	static int              recur_lock      = 0;

	if (fexcp-> clone && !recur_lock)
	{
		recur_lock = 1;
		XtSetValues(fexcp-> clone, args, *num_args);
		recur_lock = 0;
	}
#endif /*CLONE*/

#define DIFFERENT(field)	(fexcp->field != cfexcp->field)

	if (DIFFERENT(button_type))	/* GI resource...	*/
	{
			/* maybe a warning but why the overhead... */
		fexcp->button_type = cfexcp->button_type;
	}

				/* Make sure the layout parameters
				 * are valid.				*/

	if (FBCheckLayoutParameters(current, new) == True)
	{
		redisplay = True;
	}

	if (DIFFERENT(menubar_behavior))	/* GI resource...	*/
	{
			/* maybe a warning but why the overhead... */
		fexcp->menubar_behavior = cfexcp->menubar_behavior;
	}

	if (DIFFERENT(exclusive_settings) && fp->items_touched == False)
	{
		fexcp->exclusive_settings = cfexcp->exclusive_settings;

		OlVaDisplayWarningMsg(XtDisplay(new),
				      OleNfileFExclusive,
				      OleTmsg1,
				      OleCOlToolkitWarning,
				      OleMfileFExclusive_msg1,
				      XtName(new),
				      OlWidgetToClassName(new));
	}

			/* Modify the XtNnoneSet resource if this is not
			 * an exclusive settings			*/

	if (fexcp->exclusive_settings == False &&
	    DIFFERENT(none_set) &&
	    fexcp->none_set == False)
	{
		fexcp->none_set = True;
	}

        if (DIFFERENT(has_default))
        {
                if (fexcp->allow_instance_default == False)
                {
                                /* Don't issue a warning here since
                                 * there are many existing applications
                                 * that misuse this resource.           */

                        fexcp->has_default = False;
                }
                else if (fp->items_touched == False)
                {
                        if (fexcp->default_item == (Cardinal) OL_NO_ITEM)
                        {
                                /* Unset the 'has_default' flag since
                                 * the item's SetValue's proc will set
                                 * it back to TRUE if an item becomes the
                                 * default.                             */

                                fexcp->has_default = False;
                                FBSetItemField(new, XtNdefault);
                        }
                        else
                        {
                                Arg args[1];
                                XtSetArg(args[0], XtNdefault, False);
                                OlFlatSetValues(new, fexcp->default_item,
                                                args, 1);
                        }
                }
                else
                {
                        /* EMPTY */

                        /* Do nothing here since the 'AnalyzeItems'
                         * procedure will pick a default.               */
                }
        }

	if (DIFFERENT(preview) && fexcp->preview != (Widget)NULL)
	{
		OlFlatPreviewItem(new, fexcp->default_item,
					fexcp->preview, fexcp->preview_item);
		fexcp->preview		= (Widget)NULL;
		fexcp->preview_item	= (Cardinal)OL_NO_ITEM;
	}

        if (DIFFERENT(dim))
        {
                redisplay = True;
        }

#undef DIFFERENT

	if (FPART(new)->items_touched == True)
	{
		ITEMS_TOUCHED(new);
	}

	return(redisplay);

} /* END OF SetValues() */

#undef CHK_TRAVERSEITEMS

/*
 * _OlFBFindMenuItem -
 *	return item_index of the given popup_menu
 */
extern Cardinal
_OlFBFindMenuItem OLARGLIST((w, key))
	OLARG( Widget,	w)
	OLGRA( Widget,	key)	/* popup_menu of current item */
{
	OL_FLAT_ALLOC_ITEM(w, FlatItem, item);
	Cardinal	i,
			start_pos = OL_NO_ITEM;

#define NUM_ITEMS	FPART(w)->num_items

	for (i = 0; i < NUM_ITEMS; i++)
	{
		OlFlatExpandItem(w, (Cardinal)i, item);
		if (FEIPART(item)->popupMenu == key)
		{
			start_pos = item->flat.item_index;
#ifdef CHK_TRAVERSEITEMS
 printf("\tgot start_pos: %d (%s)\n", start_pos, XtName(w));
#endif
			break;
		}
	}

	OL_FLAT_FREE_ITEM(item);
	return(start_pos);
} /* end of _OlFBFindMenuItem */

static Boolean
IsOverrideRedirect OLARGLIST((w))
	OLGRA( Widget,	w)	/* popup menu shell */
{
	Arg	args[1];
	Boolean	flag = True;

	XtSetArg(args[0], XtNoverrideRedirect, &flag);
	XtGetValues(w, args, 1);

	return(flag);
} /* end of IsOverrideRedirect */

/*
 * FindNewMenuBtn -
 *	return prev/next menu item from the given flat buttons.
 */
static Cardinal
FindNewMenuBtn OLARGLIST((w, start_pos, is_prev, any_type, time))
	OLARG( Widget,		w)
	OLARG( Cardinal,	start_pos) /* from... */
	OLARG( Boolean,		is_prev)   /* T: current - 1, F: current + 1 */
	OLARG( Boolean,		any_type)  /* F: menu item, T: any kind... */
	OLGRA( Time *,		time)
{
	OL_FLAT_ALLOC_ITEM(w, FlatItem, item);
	Cardinal		i,
				new_menu_item = OL_NO_ITEM;

#define HAS_FOCUS(t)	(FPART(t)->last_focus_item != OL_NO_ITEM)

		/* setup the starting point to search...	*/
	if (is_prev)
	{
		if (start_pos == 0)
			i = NUM_ITEMS - 1;
		else
			i = start_pos - 1;

	}
	else	/* is_next... */
	{
		if (start_pos == NUM_ITEMS - 1)
			i = 0;
		else
			i = start_pos + 1;
	}

#define BASIC_CHECK	(item->flat.sensitive == (Boolean)True	&& \
			 item->flat.managed == (Boolean)True	&& \
			 item->flat.mapped_when_managed == (Boolean)True)

	do {
		OlFlatExpandItem(w, (Cardinal)i, item);

		if (BASIC_CHECK)
		{
			Boolean		override_redirect = True;

			if ( FEIPART(item)->popupMenu &&
			     (override_redirect = IsOverrideRedirect(
						FEIPART(item)->popupMenu)) )
			{
				new_menu_item = item->flat.item_index;
			}

			if (HAS_FOCUS(w))
			{
				OlFlatRefreshItem(w, start_pos, True);
				FPART(w)->last_focus_item =
					item->flat.item_index;
			}

#ifdef CHK_TRAVERSEITEMS
 printf("\tnew_menu_item: %d (%s)\n", new_menu_item, XtName(w));
#endif

			if (any_type ||
			    (FEIPART(item)->popupMenu && override_redirect) )
				break;
		}

		if (is_prev)
		{
			if (i == 0)
				i = NUM_ITEMS - 1;
			else
				--i;
		}
		else
		{
			if (i == NUM_ITEMS - 1)
				i = 0;
			else
				++i;
		}
	} while (i != start_pos);

#undef NUM_ITEMS	/* is defined in _OlFBFindMenuItem */
#undef HAS_FOCUS
#undef BASIC_CHECK

	OL_FLAT_FREE_ITEM(item);
	return(new_menu_item);
} /* end of FindNewMenuBtn */

/*
 *************************************************************************
 * TraverseItems - this routine deals with the menu traveral
 * (i.e., pull right and pull down) when button_type is OL_OBLONG_BTN.
 * It invokes superclass method if button_type is not OL_OBLONG_BTN or
 * it can't satisfy the wanted cases...
 *************************************************************************
 */
static Cardinal
TraverseItems OLARGLIST((w, sfi, dir, time))
	OLARG( Widget,		w)	/* FlatWidget id		*/
	OLARG( Cardinal,	sfi)	/* start focus item		*/
	OLARG( OlVirtualName,	dir)	/* Direction to move		*/
	OLGRA( Time,		time)	/* Time of move (ignored)	*/
{
#define SUPERCLASS	\
  ((FlatButtonsClassRec *)flatButtonsClassRec.core_class.superclass)

#define SUPERCLASS_METHOD	SUPERCLASS->flat_class.traverse_items

	OL_FLAT_ALLOC_ITEM(w, FlatItem, item);
	Boolean			was_not_multi	= True;
	Cardinal		next_item	= (Cardinal)OL_NO_ITEM;
	FlatButtonsPart *	fbp		= FEPART(w);
	OlDefine		layout		= RCPART(w)->layout_type;
	Time			timestamp	= time;
	Widget			shell		= _OlGetShellOfWidget(w);
	Boolean			override_redirect;

#define IS_WANTED	(dir == OL_MOVERIGHT  || dir == OL_MOVELEFT  ||	\
			 dir == OL_MOVEDOWN   || dir == OL_MOVEUP    || \
			 dir == OL_MULTIRIGHT || dir == OL_MULTILEFT || \
			 dir == OL_MULTIDOWN  || dir == OL_MULTIUP)

	if (!(fbp->button_type == OL_OBLONG_BTN && IS_WANTED))
	{
		return((*SUPERCLASS_METHOD)(w, sfi, dir, time));
	}

#undef IS_WANTED

	OlFlatExpandItem(w, sfi, item);

	if ( (FEIPART(item))->popupMenu )
		override_redirect = IsOverrideRedirect(
					(FEIPART(item))->popupMenu );
	else
		override_redirect = False;

#define MENU_BTN	override_redirect

	if (!_OlIsEmptyMenuStack(w))
	{
		if (layout == OL_FIXEDCOLS)
		{
			switch(dir)
			{
				case OL_MULTILEFT:
				case OL_MULTIUP:
#ifdef CHK_TRAVERSEITEMS
 printf("\nMap MULTILEFT/MULTIUP to LEFT\n");
#endif
					was_not_multi = False;
					dir = OL_MOVELEFT;
					break;
				case OL_MULTIRIGHT:
				case OL_MULTIDOWN:
#ifdef CHK_TRAVERSEITEMS
 printf("\nMap MULTIRIGHT/MULTIDOWN to RIGHT\n");
#endif
					was_not_multi = False;
					dir = OL_MOVERIGHT;
					break;
			}
		}
		else	/* OL_FIXEDROWS */
		{
			switch(dir)
			{
				case OL_MULTILEFT:
				case OL_MULTIUP:
#ifdef CHK_TRAVERSEITEMS
 printf("\nMap MULTILEFT/MULTIUP to UP\n");
#endif
					was_not_multi = False;
					dir = OL_MOVEUP;
					break;
				case OL_MULTIRIGHT:
				case OL_MULTIDOWN:
#ifdef CHK_TRAVERSEITEMS
 printf("\nMap MULTIRIGHT/MULTIDOWN to DOWN\n");
#endif
					was_not_multi = False;
					dir = OL_MOVEDOWN;
					break;
			}
		}
	}

#define CASE1 ((layout == OL_FIXEDCOLS && dir == OL_MOVERIGHT) || \
	       (layout == OL_FIXEDROWS && dir == OL_MOVEDOWN))

	if ( CASE1 && MENU_BTN &&
	     ItemActivate(w, item, OL_MENUKEY, (XtPointer)NULL) )
	{
#ifdef CHK_TRAVERSEITEMS
 printf("\nCASE1 - Popup pull-right or pull-down menu\n");
#endif
		OL_FLAT_FREE_ITEM(item);
		return(OL_NO_ITEM);
#undef CASE1
	}

#define CASE2 ((layout == OL_FIXEDCOLS && dir == OL_MOVELEFT) || \
	       (layout == OL_FIXEDROWS && dir == OL_MOVEUP))

#define CASE3 ( (layout == OL_FIXEDCOLS && (dir == OL_MOVELEFT || \
					    dir == OL_MOVERIGHT)) 	|| \
	        (layout == OL_FIXEDROWS && (dir == OL_MOVEUP   || \
					    dir == OL_MOVEDOWN)) )

	if ( (MENU_BTN && CASE2 && !_OlIsEmptyMenuStack(w)) ||
	     (!MENU_BTN && CASE3) )
	{
		Widget		root = (Widget)NULL,
				mu_sh = _OlRoot1OfMenuStack(w),
				top_sh = _OlTopOfMenuStack(w);
		Cardinal	start_pos;

#ifdef CHK_TRAVERSEITEMS
 printf("\n%s\n", (!MENU_BTN && CASE3) ?
	"CASE3 - Popup/goto next/prev menu/oblong OR Popdownn top menu" :
	"CASE2 - Popup/goto prev menu/oblong");
#endif

#define CASE3a	(dir == OL_MOVELEFT || dir == OL_MOVEUP)

		if ( (CASE3a && top_sh == mu_sh) ||
		     (!CASE3a && top_sh == shell) )
		{
#undef CASE3a
			root = _OlRootOfMenuStack(w);

				/* don't care if... */
			if ( !XtIsSubclass(root, flatButtonsWidgetClass) ||
			     FEPART(root)->button_type != OL_OBLONG_BTN )
				root = (Widget)NULL;
#ifdef CHK_TRAVERSEITEMS
 printf("\troot: %s, mu_sh: %s\n", XtName(root), XtName(mu_sh));
#endif
		}

			/* pop one down if "root" is (Widget)NULL
			 * otherwise, pop down the current cascade
			 * and then post prev/next menu cascade...
			 */
		OlActivateWidget(shell, dir, (XtPointer)time);

#define CASE3b	(dir == OL_MOVELEFT  || dir == OL_MOVEUP)	/* prev */
	      /* dir == OL_MOVERIGHT || dir == OL_MOVEDOWN	 * next */

		if ( root &&		/* should post prev/next menu?	*/
		     (start_pos = _OlFBFindMenuItem(root, mu_sh))
					!= (Cardinal)OL_NO_ITEM &&

			/* if was_not_multi == False then this is a
			 * mapped "dir", in this case, we want to post
			 * next/prev menu, otherwise, anything item
			 * will do...
			 */
		     (next_item = FindNewMenuBtn(root, start_pos,
					CASE3b, was_not_multi,
					&timestamp)) != (Cardinal)OL_NO_ITEM )
		{
			OlFlatExpandItem(root, next_item, item);
			ItemActivate(root, item, OL_MENUKEY, (XtPointer)NULL);
#undef CASE3b
		}

		if ( root &&
		     start_pos != (Cardinal)OL_NO_ITEM &&
		     next_item == (Cardinal)OL_NO_ITEM )
		{
			w = root;
			sfi = start_pos;

#ifdef CHK_TRAVERSEITEMS
 printf("\t\tGoto CASE5 with Widget: %s, sfi = %d\n", XtName(w), sfi);
#endif
		}
		else
		{
			OL_FLAT_FREE_ITEM(item);
				/* This is crazy, when poping down the
				 * top menu thru activation of shell,
				 * it invokes FBPopdownMenu() ->
				 * _OlFBUnsetCurrent() -> FBMenuSetToCurrent()
				 * -> UNSELECT_HAPPENED simply reset stayup
				 * mode because this can come from a mouse
				 * or a keyboard. The former case should
				 * reset, but later one is questionable.
				 */
			if (!_OlIsEmptyMenuStack(w) && _OlIsNotInStayupMode(w))
				_OlSetStayupMode(w);
			return(OL_NO_ITEM);
		}
#undef CASE3
#undef CASE2
	}

#define CASE4	(dir == OL_MULTIRIGHT || dir == OL_MULTILEFT || \
		 dir == OL_MULTIDOWN  || dir == OL_MULTIUP)

#define CASE4a	(dir == OL_MULTILEFT  || dir == OL_MULTIUP)	/* prev */
	      /* dir == OL_MULTIRIGHT || dir == OL_MULTIDOWN)	 * next */

	if ( CASE4 &&
	     (next_item = FindNewMenuBtn(w, sfi, CASE4a, False, &timestamp))
				!= (Cardinal)OL_NO_ITEM )
	{
#ifdef CHK_TRAVERSEITEMS
 printf("\nCASE4 - Popup prev/next menu from a flat buttons (menubar)\n");
#endif
		OlFlatExpandItem(w, next_item, item);
		ItemActivate(w, item, OL_MENUKEY, (XtPointer)NULL);

		OL_FLAT_FREE_ITEM(item);
		return(OL_NO_ITEM);

#undef CASE4
#undef CASE4a
	}

#ifdef CHK_TRAVERSEITEMS
 printf("\nCASE5 - normal traversal, sfi: %d, dir: %d\n", sfi, dir);
#endif

	OL_FLAT_FREE_ITEM(item);
	return((*SUPERCLASS_METHOD)(w, sfi, dir, time));

#undef SUPERCLASS
#undef SUPERCLASS_METHOD
#undef MENU_BTN
} /* END OF TraverseItems */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 * ButtonHandler - this routine handles all button press and release
 * events.
 ****************************procedure*header*****************************
 */
static void
ButtonHandler OLARGLIST((w, ve))
	OLARG( Widget,		w)
	OLGRA( OlVirtualEvent,	ve)
{
  (*_olmFBButtonHandler)(w, ve);
}

/*
 *************************************************************************
 * _OlFBSelectItem - this routine selects the current item and activates
 * its callbacks.
 * Note: SetValues calls this routine with NULL values for xevent and
 * params.
 ****************************procedure*header*****************************
 */
extern void
_OlFBSelectItem OLARGLIST((w))
	OLGRA( Widget,	w)
{
	Arg			args[1];
	Widget 			popupMenu 	= NULL;
	FlatButtonsPart *	fexcp 		= FEPART(w);
	Cardinal		current 	= fexcp->current_item;
	Cardinal		previous;


			/* If there's no current item, exit immediately	*/

	if (current == (Cardinal) OL_NO_ITEM)
	{
		return;
	}

	switch(fexcp-> button_type)
	{
	case OL_OBLONG_BTN:
		/*
 	 	 * for flat menu buttons, change the visual, call the 
	 	 * callbacks then revert the visual
 	 	 */

		SET_XtNset_TO(True, w, current)

		/*
		 * set it to make it busy ...
		 */

		if (OlGetGui() == OL_OPENLOOK_GUI)
		{
			fexcp->set_item = current;
			OlFlatRefreshItem(w, current, True);
		}
		
			/* Find out if it has a menu */
		XtSetArg(args[0], XtNpopupMenu, &popupMenu);
		OlFlatGetValues(w, current, args, 1);

		if (popupMenu)
		{
			if (_OlInPreviewMode(popupMenu))
			{
					/* Visual is already there */
				Widget child;
				Cardinal default_index;
				
				_OlGetMenuDefault(popupMenu, &child, 
					&default_index, True);
				OlActivateWidget(child, OL_SELECTKEY, 
					 (XtPointer)(default_index + 1));

				/* Now get rid of child menu */
				OlActivateWidget(popupMenu, OL_CANCEL,
						 (XtPointer)NULL);
				
				_OlFBUnsetCurrent(w);
			}
			else
			{
				if (_OlIsPendingStayupMode(w))
					_OlSetStayupMode(w);
			}
		}
                else
		{
			if (!_OlIsEmptyMenuStack(w) && _OlIsNotInStayupMode(w))
			{
				_OlFBResetParentSetItem(w, True);
				_OlPopdownCascade(_OlRootOfMenuStack(w), False);
			}

			_OlFBCallCallbacks(w, current, False);
				/* unset it to have it revert	*/
			SET_XtNset_TO(False, w, current)
			fexcp->set_item = OL_NO_ITEM;
			_OlFBUnsetCurrent(w);
		}
		break;
	case OL_RECT_BTN:
	case OL_CHECKBOX:

		previous		= fexcp->set_item;
		fexcp->current_item	= (Cardinal) OL_NO_ITEM;

		if (fexcp->none_set == False)
		{
			/* This can be true only when we have an
			 * exclusives type setting.			*/

			if (current != previous)
			{
				/* Set the new Item -- this will unset
				 * the old item for us.  After we've
				 * set the items, we can call the
				 * callbacks.				*/

				SET_XtNset_TO(True, w, current)

				if (previous != (Cardinal) OL_NO_ITEM)
				{
					_OlFBCallCallbacks(w, previous, False);
				}

				_OlFBCallCallbacks(w, current, False);
			}
		}
		else if (fexcp->exclusive_settings == True)
		{
			if (current == previous)
			{
				/* Unset the current item and call
				 * its callback procedure.		*/

				SET_XtNset_TO(False, w, current)

				_OlFBCallCallbacks(w, current, False);
			}
			else /* current != previous */
			{
				if (previous == (Cardinal) OL_NO_ITEM)
				{
				/* Set the new item and call its
				 * callbacks.				*/

					SET_XtNset_TO(True, w, current)

					_OlFBCallCallbacks(w, current, False);
				}
				else /* current & previous are both valid */
				{
				/* Set the new item -- this will unset
				 * the old item for us.  After we've
				 * set the items, we can call the
				 * callbacks.				*/

					SET_XtNset_TO(True, w, current)

					_OlFBCallCallbacks(w, previous, False);
					_OlFBCallCallbacks(w, current, False);
				}
			}
		}
		else				/* Non-exclusives type	*/
		{
			Boolean		flag;

			flag = _OlFBIsSet(w, current) ? False : True;
			SET_XtNset_TO(flag, w, current)

			_OlFBCallCallbacks(w, current, False);
		}
		break;
	}

} /* END OF _OlFBSelectItem() */

/*
 *************************************************************************
 * _OlFBSetDefault - this routine sets a setting to be the default item
 * whenever the user uses the "SET MENU DEFAULT" button
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
extern void
_OlFBSetDefault OLARGLIST((w, use_focus_item, x, y, new_current))
	OLARG( Widget,		w)
	OLARG( Boolean,		use_focus_item)
	OLARG( Position,	x)
	OLARG( Position, 	y)
        OLGRA( Cardinal,	new_current)
{
	FlatButtonsPart *		fexcp 	= FEPART(w);

	if (fexcp->allow_instance_default == True)
	{
		Cardinal	item_index;

		if (use_focus_item == True)
		{
			item_index = FPART(w)->focus_item;
		}
		else
		{
			item_index = new_current;
		}

		if (item_index != (Cardinal)OL_NO_ITEM &&
		    item_index != fexcp->default_item)
		{
			Arg	args[1];

			XtSetArg(args[0], XtNdefault, True);
			OlFlatSetValues(w, item_index, args, 1);
		}
	}

} /* END OF _OlFBSetDefault() */

/*
 *************************************************************************
 * _OlFBSetToCurrent - this routine mkaes the item under the pointer equal
 * to the current item.
 ****************************procedure*header*****************************
 */
extern void
_OlFBSetToCurrent OLARGLIST((w, is_menu_cmd, x, y, new_current, motion_or_enter))
  OLARG( Widget,	w)
  OLARG( Boolean, 	is_menu_cmd)
  OLARG( Position,	x)
  OLARG( Position, 	y)
  OLARG( Cardinal,	new_current)
  OLGRA( Boolean,	motion_or_enter)
{
  FlatButtonsPart *	fexcp = FEPART(w);
  Cardinal		old_current;
  
  switch (fexcp-> button_type)
    {
    case OL_OBLONG_BTN:
      FBMenuSetToCurrent(w, fexcp, fexcp->current_item, new_current,
			 x, y, motion_or_enter, is_menu_cmd);
      break;
    case OL_RECT_BTN:
    case OL_CHECKBOX:
      
      /* If the command invoking this routine is a
       * menu-related command and we're not on a menu,
       * return.
       */
      if (is_menu_cmd == True && fexcp->menu_descendant == False)
	{
	  return;
	}
      
      /* Get the index of the new current item	*/
      
      old_current = fexcp->current_item;
      fexcp->current_item = new_current;
      
      if (old_current == fexcp->current_item)
	{
	  return;
	}
      /* Update visuals of the items	*/
      
      if (fexcp->none_set == False)	/* Can only happen with
					 * with exclusives	*/
	{
	  if (!(fexcp->current_item == fexcp->set_item &&
		(old_current == (Cardinal) OL_NO_ITEM ||
		 old_current == fexcp->set_item)))
	    {
	      /* Turn off the set visual for
	       * the old current item		*/
	      
	      if (old_current != (Cardinal) OL_NO_ITEM)
		{
		  OlFlatRefreshItem(w, old_current, True);
		}
	      else
		if ((fexcp->set_item !=
		     (Cardinal)OL_NO_ITEM) &&
		    (fexcp->set_item !=
		     fexcp->current_item))
		  {
		    /* If the old current item is 
		     * OL_NO_ITEM,
		     * turn off the set item	*/
		    
		    OlFlatRefreshItem(w, fexcp->set_item, True);
		  }
	      
		      /* Turn on the visual for the
		       * new current item.		*/
	      if (fexcp->current_item != (Cardinal) OL_NO_ITEM)
		{
		  OlFlatRefreshItem(w, fexcp->current_item, False);
		}
	      else if (fexcp->set_item != (Cardinal) OL_NO_ITEM)
		  {
			    /* If the new current item is
			     * OL_NO_ITEM, turn on the set item. */
		    OlFlatRefreshItem(w, fexcp->set_item, False);
		  }
	    }
	}
      else 
	if (fexcp->exclusive_settings == True)
	  {
	    Cardinal	other_item;
	    
	    other_item =
	      (old_current != (Cardinal) OL_NO_ITEM ?
	       old_current :
	       fexcp->set_item != (Cardinal) OL_NO_ITEM ?
	       fexcp->set_item : fexcp->current_item);
	    
	    if (other_item != fexcp->current_item)
	      {
		OlFlatRefreshItem(w, other_item, True);
	      }
	    
	    if (fexcp->current_item != (Cardinal)OL_NO_ITEM)
	      {
		OlFlatRefreshItem(w,
				  fexcp->current_item,
				  (Boolean) (fexcp->current_item ==
					     fexcp->set_item ? True : False));
	      }
	    else
	      if (other_item != fexcp->set_item &&
		  fexcp->set_item != (Cardinal) OL_NO_ITEM)
		{
		  OlFlatRefreshItem(w, fexcp->set_item, False);
		}
	  }
	else /* Non-exclusives type */
	  {
	    if (old_current != (Cardinal) OL_NO_ITEM)
	      {
		OlFlatRefreshItem(w, old_current, 
				  (Boolean) (_OlFBIsSet(w, old_current) ? 
					     False : True));
	      }
	    
	    if (fexcp->current_item != (Cardinal) OL_NO_ITEM)
	      {
		OlFlatRefreshItem(w,
				  fexcp->current_item, (Boolean)
				  (_OlFBIsSet(w, fexcp->current_item) ? 
				   True : False));
	      }
	  }
      break;
    }
} /* END OF _OlFBSetToCurrent() */

/*
 *************************************************************************
 * _OlFBUnsetCurrent - if there is a current item, this routine unsets it.
 ****************************procedure*header*****************************
 */
extern void
_OlFBUnsetCurrent OLARGLIST((w))
	OLGRA( Widget,	w)
{
	FlatButtonsPart *	fexcp = FEPART(w);
	Cardinal		old_current = fexcp->current_item;
	Cardinal		current_item = (Cardinal)OL_NO_ITEM;
	FBEvent		        what_happened;

	switch (fexcp-> button_type)
	{
	case OL_OBLONG_BTN:
		if (fexcp->current_item != (Cardinal)OL_NO_ITEM)
		{
				/*
				 * Last two Booleans should be ignored
				 * by FBMenuSetToCurrent in this case.
				 * If this is not true, then we need to
				 * change the functional interface.
				 * No big deal...
				 */
			FBMenuSetToCurrent(
				w, fexcp, old_current, (Cardinal)OL_NO_ITEM, 
				-1, -1, False, False);
		}

		break;
	case OL_RECT_BTN:
	case OL_CHECKBOX:
		if (old_current == (Cardinal) OL_NO_ITEM)
		{
			return;
		}

                what_happened = FBDetermineWhatHappened(old_current,
							current_item); 

		fexcp->current_item = (Cardinal) OL_NO_ITEM;

		if (fexcp->exclusive_settings == True)
		{
			if (old_current != fexcp->set_item)
			{
				OlFlatRefreshItem(w, old_current, True);
		
				if (fexcp->set_item != (Cardinal) OL_NO_ITEM)
				{
					OlFlatRefreshItem(w, fexcp->set_item,
					   False);
				}
			}
			else
			{
				OlFlatRefreshItem(w, old_current,
					(Boolean) (fexcp->none_set == False ?
							False : True));
			}
		}
		else
		{
			OlFlatRefreshItem(w, old_current, (Boolean)
				(_OlFBIsSet(w, old_current) ? False : True));
		}

		/*
 		 * It the default_item needs refreshing (and won't be refreshed
 		 * since it'll be handled as a result of redrawing old or new)
 		 * refresh it here.
 		 */

		if (fexcp->default_item != (Cardinal)OL_NO_ITEM &&
    		    fexcp->default_item != old_current &&
    		    (what_happened == UNSELECT_HAPPENED))

   		{
   			OlFlatRefreshItem(w, fexcp->default_item, False);
   			FPRINTF((stderr,"redisplay default\n"));
   		}
#ifdef DEBUG
		else
   			FPRINTF((stderr,"dont redisplay default\n"));
#endif
		break;
	}

} /* END OF _OlFBUnsetCurrent() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/* There are no public procedures */

/*
 *************************************************************************
 *
 * Totally New Procedures
 *
 *******************************new*procedures****************************
 */

/*
 * FBDetermineWhatHappened
 *
 */
static FBEvent
FBDetermineWhatHappened OLARGLIST((old_current, current_item))
  OLARG( Cardinal, old_current)
  OLGRA( Cardinal, current_item)
{
   FBEvent what_happened;

   if (old_current == current_item)
      if (old_current == (Cardinal)OL_NO_ITEM)
         what_happened = NOTHING_HAPPENED;
      else
         what_happened = SAME_THING_HAPPENED;
   else
      if (old_current == (Cardinal)OL_NO_ITEM)
         what_happened = SELECT_HAPPENED;
      else
         if (current_item == (Cardinal)OL_NO_ITEM)
            what_happened = UNSELECT_HAPPENED;
         else
            what_happened = SWITCH_HAPPENED;

   return (what_happened);

} /* end of FBDetermineWhatHappened */

/*
 * FBMenuSetToCurrent
 *
 * This procedure is called from the _OlFBSetToCurrent and _OlFBUnsetCurrent
 * routines when the class of the widget is flatButtonsWidgetClass.
 * Special processing needed for menu button select/unselect occurs here.
 *
 */
static void
FBMenuSetToCurrent OLARGLIST((w, fexcp, old_current, current_item, x, y, motion_or_enter, is_menu_cmd))
  OLARG( Widget,		w)
  OLARG( FlatButtonsPart *,	fexcp)
  OLARG( Cardinal,		old_current)
  OLARG( Cardinal,		current_item)
  OLARG( Position, 		x)
  OLARG( Position, 		y)
  OLARG( Boolean,		motion_or_enter)
  OLGRA( Boolean,		is_menu_cmd)	/* True if OL_MENU	*/
{
   FBEvent		what_happened;
   Boolean		was_in_stayup;
   Position		root_x, root_y;

   /*
    * This prevents unwanted effects of recursive calls to this routine
    * for the same widget.  These calls can occur when a selected menu
    * button with associated popup is unselected for example.
    */

   if (fexcp-> being_reset)
      return;
   else
      fexcp-> being_reset = True;

   fexcp->current_item = current_item;

   /*
    * Determine what has occurred.  Possibilities are:
    * 
    * NOTHING_HAPPED    Both old and new current items are set to OL_NO_ITEM
    * SAME_THING_HAPPED Both old and new current items are set to the same thing
    * SELECT_HAPPEND    Old was set to OL_NO_ITEM and new is not
    * UNSELECT_HAPPENED New was set to OL_NO_ITEM and old was not
    * SWITCH_HAPPENED   Old and new are different, yet neither is OL_NO_ITEM
    */

   what_happened = FBDetermineWhatHappened(old_current, current_item);

   /*
    * It the default_item needs refreshing (and won't be refreshed
    * since it'll be handled as a result of redrawing old or new)
    * refresh it here.
    */

   if (fexcp->default_item != (Cardinal)OL_NO_ITEM &&
       fexcp->default_item != old_current &&
       fexcp->default_item != current_item &&
       what_happened == SWITCH_HAPPENED)

      OlFlatRefreshItem(w, fexcp->default_item, False);

   /*
    * Do what's neccessary given what has happened
    */

#undef TELL_WHAT_HAPPENED
   switch(what_happened)
      {
      case NOTHING_HAPPENED:    /* new == old == no_item */
#ifdef TELL_WHAT_HAPPENED
 printf("NOTHING HAPPENED: %s\n", XtName(w));
#endif
		/* Reminder:, I may need to pop down the top menu...	*/
	 drag_right_mode = RESET_DRAG_RIGHT;
		 /* nothing to do */
         if (fexcp-> set_item != (Cardinal)OL_NO_ITEM)
	 {
		SET_XtNset_TO(False, w, fexcp-> set_item)
		fexcp->set_item = OL_NO_ITEM;
	 }
         break;
      case SAME_THING_HAPPENED: /* new == old != no_item */
#ifdef TELL_WHAT_HAPPENED
 printf("SAME_THING_HAPPENED: %s\n", XtName(w));
#endif
         _OlQueryResetStayupMode(w, XtWindowOfObject(w), x, y);

		/* Drag was started with an EnterNotify	*/
	if (_OlIsNotInStayupMode(w))
	{
			/*
			 * If drag_right_mode is RESET_DRAG_RIGHT,
			 * then Drag was started with a ButtonRelease
			 *  (i.e., if part of SELECT_HAPPENED),
			 * else Drag was started with an EnterNotify
			 *  (i.e., else part of SELECT_HAPPENED).
			 *
			 * We also need to check whether power user
			 * switch is on to determine "do_popdown".
			 * (Q: should we do same checking in SWITHCH_HAPPENED?
			 *     The answer is NO for now. If the answer is YES
			 *     then we should just change the interface of
			 *     CheckDragRight().)
			 *
			 */
		CheckDragRight(
			w, fexcp, current_item, x,
			(drag_right_mode == RESET_DRAG_RIGHT ||
			 (!is_menu_cmd && _OlSelectDoesPreview(w))) ?
			False : True,
			True
		);
	}
	else
	{
			/* This must be a mouse click if this does happen. */
		if (drag_right_mode == RESET_DRAG_RIGHT &&
		    motion_or_enter == False)
		{
			drag_right_mode = IN_SUBMENU_REGION;
			OlFlatRefreshItem(w, current_item, False);
		}
	}
         break;
      case SELECT_HAPPENED:     /* new != old == no_item */
#ifdef TELL_WHAT_HAPPENED
 printf("SELECT_HAPPENED: %s\n", XtName(w));
#endif
	XtTranslateCoords(w, x, y, &root_x, &root_y);
        _OlInitStayupMode(w,XtWindowOfObject(w), root_x, root_y);

		/* refresh default item now...	*/
	if ( fexcp->default_item != (Cardinal)OL_NO_ITEM &&
	     fexcp->default_item != old_current &&
	     fexcp->default_item != current_item )
		OlFlatRefreshItem(w, fexcp->default_item, True);

	drag_right_mode = RESET_DRAG_RIGHT;
	fexcp->drag_right_x = x;
	if (motion_or_enter == True)
	{
		CheckDragRight(w, fexcp, current_item, x, False, False);
	}
	else /* must be a Button Press	*/
	{
			/* refresh new but don't pop up the menu now */
		if (fexcp-> set_item != (Cardinal)OL_NO_ITEM)
		{
			SET_XtNset_TO(False, w, fexcp-> set_item)
			fexcp->set_item = OL_NO_ITEM;
		}

			/*
			 * Menu shouldn't popup unless the mouse pointer
			 * is on the menu mark region. I'll decide whether
			 * the operation is a mouse click and then
			 * popping up the menu later on. Note that
			 * this will be handled by the "else" part
			 * of SAMETHING_HAPPENED.
			 */
		CheckDragRight(w, fexcp, current_item, x, False, False);

			/*
			 * Reset the mode to RESET_DRAG_RIGHT so we
			 * can handle "mouse click" correctly...
			 */
		if (drag_right_mode == NOT_IN_SUBMENU_REGION)
			drag_right_mode = RESET_DRAG_RIGHT;
	}
         break;
      case UNSELECT_HAPPENED:   /* new != old != no_item && new == no_item */
#ifdef TELL_WHAT_HAPPENED
 printf("UNSELECT_HAPPENED: %s\n", XtName(w));
#endif

	 _OlResetStayupMode(w);
         /* clear the drag_right bits */
         drag_right_mode = RESET_DRAG_RIGHT;
         if (fexcp-> set_item != (Cardinal)OL_NO_ITEM)
	 {
		SET_XtNset_TO(False, w, fexcp-> set_item)
		fexcp->set_item = OL_NO_ITEM;
	 }

		 /* refresh old */
         OlFlatRefreshItem(w, old_current, True);
         break;
      case SWITCH_HAPPENED:     /* new != old != no_item && new != no_item */
#ifdef TELL_WHAT_HAPPENED
 printf("SWITCH_HAPPENED: %s\n", XtName(w));
#endif
         was_in_stayup = _OlIsInStayupMode(w);
         _OlResetStayupMode(w);
         /* refresh old and new */
         if (fexcp-> set_item != (Cardinal)OL_NO_ITEM)
	 {
		SET_XtNset_TO(False, w, fexcp-> set_item)
		fexcp->set_item = OL_NO_ITEM;
	 }

	drag_right_mode = RESET_DRAG_RIGHT;
         OlFlatRefreshItem(w, old_current, True);
	if (motion_or_enter)
	{
		fexcp->drag_right_x = x;
		CheckDragRight(
			w, fexcp, current_item, x,
			_OlTopOfMenuStack(w) == _OlGetShellOfWidget(w) ?
							False : True, False);
	}
	else
	{
		drag_right_mode = IN_SUBMENU_REGION;
		OlFlatRefreshItem(w, current_item, False);
	}

        if (was_in_stayup)
	{
		XtTranslateCoords(w, x, y, &root_x, &root_y);
		_OlInitStayupMode(w, XtWindowOfObject(w), root_x, root_y);
	}

         break;
      }

   /*
    * reset the recursion lock for this widget
    */

   fexcp-> being_reset = False;

} /* end of FBMenuSetToCurrent */

/*
 * CheckDragRight -
 */
static void
CheckDragRight OLARGLIST((w, fexcp, item_index, x, do_popdown, check_drag_right))
	OLARG( Widget,			w)
	OLARG( FlatButtonsPart *,	fexcp)
	OLARG( Cardinal,		item_index)
	OLARG( Position,		x)
	OLARG( Boolean,			do_popdown)
	OLGRA( Boolean,			check_drag_right)
{
	_OlAppAttributes *	app_attrs;
	Arg			arg[1];
	DragRightMode		old_mode = drag_right_mode;
	OlFlatDrawInfo		di;
	Widget			popup_menu = (Widget)NULL;

	XtSetArg(arg[0], XtNpopupMenu, (XtArgVal)&popup_menu);
	OlFlatGetValues(w, item_index, arg, 1);

		/* The 3rd check is because we don't want
		 * dragRightDistance if it's a pull-down menu.
		 * Note that, in Motif mode, menu_descendant is
		 *	set to True when menubar_behavior is set
		 *	to True so this check also fixes a known
		 *	problem.
		 */
	if (popup_menu != (Widget)NULL && fexcp->menu_descendant &&
	    RCPART(w)->layout_type == OL_FIXEDCOLS)
	{
		current_mouse_position = x;

		app_attrs = _OlGetAppAttributesRef(w);
		OlFlatGetItemGeometry(
			w, item_index,
			&di.x, &di.y, &di.width, &di.height
		);

#define SUBMENU_REGION   (int)(app_attrs-> menu_mark_region)
#define DRAG_RIGHT_AMT   (int)(app_attrs-> drag_right_distance)

		if (!(check_drag_right &&
		      x >= (Position)(fexcp->drag_right_x + DRAG_RIGHT_AMT)) &&
	            x < (Position)(di.x + di.width - SUBMENU_REGION))
			drag_right_mode = NOT_IN_SUBMENU_REGION;
		else
			drag_right_mode = IN_SUBMENU_REGION;

#undef SUBMENU_REGION
#undef DRAG_RIGHT_AMT
	}
	else
	    drag_right_mode = IN_SUBMENU_REGION;

	if (drag_right_mode != old_mode)
	{
		if (drag_right_mode != IN_SUBMENU_REGION)
		{
			if (do_popdown)
				_OlPopdownCascade(_OlTopOfMenuStack(w), False);
		}
		OlFlatRefreshItem(w, item_index, False);
	}
} /* end of CheckDragRight */

#ifdef CLONE
/*
 * WaitForMap
 *
 * This predicate function returns True when a map event for the 
 * window specified in the parameter arg is found.  It can be used
 * with the XPeekIfEvent procedure to cause the application to
 * block until a window is mapped.
 *
 */
/* ARGSUSED */
static Bool
WaitForMap OLARGLIST((display, event, arg))
  OLARG( Display *,	display)	/* unused */
  OLARG( XEvent *,	event)
  OLGRA( char *,		arg)
{

   return (event-> type == MapNotify && event-> xmap.window == (Window) arg);

} /* end of WaitForMap */
#endif /*CLONE*/

/*
 * FBPopdownMenu
 *
 */
static void
FBPopdownMenu OLARGLIST((w))
	OLGRA( Widget,	w)
{
#ifdef CLONE
	FlatButtonsPart *	fexcp = FEPART(w);
	static char		recur_lock = 0;
#endif /*CLONE*/

   _OlFBUnsetCurrent(w);

#ifdef CLONE
   if (fexcp-> clone && !recur_lock)
      {
      FlatButtonsPart * clone_fexcp = FEPART(fexcp-> clone);

      recur_lock = 1;

      _OlFBUnsetCurrent(fexcp-> clone);
      if (clone_fexcp-> set_item != (Cardinal)OL_NO_ITEM)
         SET_XtNset_TO(False, w, fexcp-> set_item)
	   
      recur_lock = 0;
      }
#endif /*CLONE*/

} /* end of FBPopdownMenu */

/*
 * _OlFBPostMenu
 *
 * This procedure is called from the FBDrawItem routine.  It is responsible
 * for processing potential menu posting for flat menu buttons.  It 
 * will also unpost any extraneous menus and perform previewing if
 * the preview mode flag is set and the item (new) has is a menu node.
 *
 */
void
_OlFBPostMenu OLARGLIST((w, new, di, item_part))
  OLARG( Widget,			w)
  OLARG( Cardinal,		new)
  OLARG( OlFlatDrawInfo *,	di)
  OLGRA( FlatButtonsItemPart *,	item_part)
{
   FlatButtonsPart * fexcp     = FEPART(w);
   Widget            shell     = _OlGetShellOfWidget(w);
   Widget            popupMenu = item_part-> popupMenu;
   OlMenuAlignment	align;

   if (popupMenu == NULL)
      {
      if (w == (Widget) _OlRootOfMenuStack(w))
         _OlPopdownCascade((Widget)_OlRootOfMenuStack(w), True);
      else
         if ((Widget)_OlTopOfMenuStack(w) != shell)
            _OlPopdownCascade(shell, True);
      _OlResetStayupMode(w);
      }
   else
      if (!_OlIsInMenuStack(popupMenu))
         {
         if (_OlInPreviewMode(popupMenu))
            {
            _OlPreviewMenuDefault(popupMenu, w, new);
            }
         else
            {
            if (RCPART(w)->layout_type == OL_FIXEDCOLS)
               {
               align  = PullAsideAlignment;
               }
            else
               {
               align  = DropDownAlignment;
               }
   
            if ((Widget)_OlTopOfMenuStack(shell) != shell)
               if (_OlIsInMenuStack(shell))
                  _OlPopdownCascade(shell, True);
               else
                  _OlPopdownCascade((Widget)_OlRootOfMenuStack(shell),True);

            if (!fexcp-> menu_descendant ||
                 (fexcp->menu_descendant && SHOULD_POST_MENU))
	    {
		int		shellx;
		int		shelly;
		Window		child;
		XRectangle	rect;

		XTranslateCoordinates(
			XtDisplay(w), XtWindow(w), 
			RootWindowOfScreen(XtScreen(w)),
			0, 0, &shellx, &shelly, &child);

		if (!fexcp-> menu_descendant && OlGetGui() == OL_MOTIF_GUI)
		{
#define HILITETHICKNESS ((PrimitiveWidget)w)->primitive.highlight_thickness
#define PSIZE(x)        (int)_OlMax(OlScreenPointToPixel(               \
                                        OL_HORIZONTAL, x, XtScreen(w)), \
                                 OlScreenPointToPixel(                  \
                                        OL_VERTICAL, x, XtScreen(w)))

			int	tmp =  PSIZE(4) + PSIZE(2) + HILITETHICKNESS;

				/* adjust with default ring and location
				 * cursor removal. See FBItemDimension...
				 */
			if (align == DropDownAlignment)
			{
				rect.x	= shellx + di-> x + tmp;
				rect.y	= shelly + di-> y - tmp;
			}
			else
			{
					/* See MenuShell.c:_OlPopupMenu
					 * for why "- PSIZE(1)".
					 */
				rect.x	= shellx + di-> x - tmp - PSIZE(1);
				rect.y	= shelly + di-> y;
			}

#undef HILITETHICKNESS
#undef PSIZE
		}
		else
		{
			rect.x	= shellx + di-> x;
			rect.y	= shelly + di-> y;
		}
		rect.width	= di-> width;
		rect.height	= di-> height;

			/* readjust "width" because of drag_right_distance */
		if (align == PullAsideAlignment &&
		    current_mouse_position - di->x > 0 &&
		    OlGetGui() == OL_OPENLOOK_GUI)
		{
			Dimension ten_pts = OlScreenPointToPixel(
					OL_HORIZONTAL, 10, XtScreen(w)),
				  possible_width;

			possible_width = current_mouse_position - di->x +
					 ten_pts;
			if (di->width > possible_width)
				rect.width = possible_width;
		}

		_OlPopupMenu(
			popupMenu, w, FBPopdownMenu, &rect, align, 
			False, 0, (Position)0, (Position)0);
	    }

	    SET_XtNset_TO(True, w, new)
            }
         }

} /* end of _OlFBPostMenu */

/*
 * FBSelectKey
 *
 */
static void
FBSelectKey OLARGLIST((w, fexcp, focus_item, current))
  OLARG( Widget,            w)
  OLARG( FlatButtonsPart *, fexcp)
  OLARG( Cardinal,          focus_item)
  OLGRA( Cardinal,          current)
{
  Widget shell = _OlGetShellOfWidget(w);
  
   if (_OlInPreviewMode(shell))
      {
	fexcp->current_item = focus_item;
	_OlFBSelectItem(w);

#ifdef CLONE
	/* Now get rid of parent menu, if there is one */
	FBPopdownMenu(w);
	_OlResetPreviewMode(shell);
#endif /*CLONE*/
      }
   else
      {

      _OlMenuLock(shell, w, NULL);

      _OlResetPreviewMode(shell);

      drag_right_mode     = IN_SUBMENU_REGION;
	current_mouse_position = 0;
	fexcp->drag_right_x = 0;
      if (fexcp-> set_item != (Cardinal)OL_NO_ITEM)
      {
	 Cardinal	set_item = fexcp->set_item;
         SET_XtNset_TO(False, w, fexcp-> set_item)
	 fexcp->set_item = OL_NO_ITEM;
	 OlFlatRefreshItem(w, set_item, True);
      }

      fexcp->current_item = focus_item;

      if (current != (Cardinal)OL_NO_ITEM)
         {
         OlFlatRefreshItem(w, current, True);
         }

      OlFlatRefreshItem(w, fexcp-> current_item, True);

      if (!_OlIsEmptyMenuStack(shell))
         _OlInitStayupMode(shell, NULL, 0, 0);

      _OlFBSelectItem(w);

      if (!_OlIsInStayupMode(shell))
         _OlPopdownCascade((Widget)_OlRootOfMenuStack(shell), False);
      }

} /* end of FBSelectKey */

/*
 * _OlFBResetParentSetItem - reset "set_item" to OL_NO_ITEM so "BUSY"
 *	can be cleared.
 *
 * 	If "reset_all" is "False" then only reset "top parent"
 *		of menu stack, otherwise reset all parents in the
 *		cascade.
 *	
 */
extern void
_OlFBResetParentSetItem OLARGLIST((w, reset_all))
	OLARG( Widget,		w)
	OLGRA( Boolean,		reset_all) /* False: top_parent	*/
{
	FlatButtonsPart *	fbp;

	if (reset_all == False)
	{
		Widget		p;

		if ((p = _OlTopParentOfMenuStack(w)) != (Widget)NULL &&
		    XtIsSubclass(p, flatButtonsWidgetClass))
		{
			fbp = FEPART(p);
			fbp->set_item = OL_NO_ITEM;
		}
	}
	else
	{
		Cardinal	how_many, i;
		WidgetList	list;

		if (how_many = _OlParentsInMenuStack(w, &list))
		{
			for (i = 0; i < how_many; i++)
			{
				if (XtIsSubclass(
					list[i], flatButtonsWidgetClass))
				{
					fbp = FEPART(list[i]);
					fbp->set_item = OL_NO_ITEM;
				}
			}
			XtFree((XtPointer)list);
		}
	}
} /* end of _OlFBResetParentSetItem */

/*
 * ConsumeTakeFocusEH - consume WM_TAKE_FOCUS if this is in
 *	the middle of "menu" posting...
 */
static void
ConsumeTakeFocusEH OLARGLIST((w, client_data, xevent, cont_to_dispatch))
	OLARG( Widget,		w)
	OLARG( XtPointer,	client_data)	/* not used */
	OLARG( XEvent *,	xevent)
	OLGRA( Boolean *,	cont_to_dispatch)
{
	if ( xevent->type == ClientMessage &&
	     xevent->xclient.message_type ==
			XA_WM_PROTOCOLS(xevent->xany.display) &&
	     (Atom)xevent->xclient.data.l[0] ==
			XA_WM_TAKE_FOCUS(xevent->xany.display) &&
	     !_OlIsEmptyMenuStack(w) &&
	     !_OlIsNotInStayupMode(w) )
	{
		*cont_to_dispatch = False;
	}
} /* end of ConsumeTakeFocusEH */
