/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)abbrevstack:AbbrevMenu.c	1.33"
#endif

/*
 *************************************************************************
 *
 * Description:
 *		This file contains the source code for the AbbrevMenuButton
 *	widget.  Since some of the AbbrevMenuButton code deals directly with
 *	the menu widget, some AbbrevMenuButton routines actually exist in
 *	the menu source code file.
 *
 ******************************file*header********************************
 */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/AbbrevMenP.h>
#include <Xol/MenuP.h>
#include <Xol/Pushpin.h>

#define ClassName AbbrevMenu
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

static void	ActivateDefault();	/* activates submenu default	*/
static void	GetNormalGC OL_ARGS((AbbrevMenuButtonWidget));
static void	GetWidgetDimensions();	/* Gets width & height of visual*/	
static void	PopupSubmenu();		/* pops up submenu		*/
static void	PositionSubmenu OL_ARGS((Widget, Widget, Cardinal, OlDefine,
			Position *, Position *, Position *, Position *));
static void	RevertButton();		/* Returns an inverted button
					   to normal state		*/

					/* class procedures		*/

static Boolean	ActivateWidget OL_ARGS((Widget, OlVirtualName, XtPointer));
static void	ClassInitialize();	/* initialize Class Fields for
					 * each subclass		*/
static void	Destroy();		/* Destroys allocated data	*/
static void	GetValuesHook();	/* Return Menu data		*/
static void	HighlightHandler OL_ARGS((Widget, OlDefine));
static void	Initialize();		/* Class initialization		*/
static void	InitializeHook();	/* Submenu creation		*/
static void	Redisplay();		/* Refreshes the widget		*/
static Boolean	SetValues();		/* Set Values for class		*/

					/* action procedures		*/

#define IS_KEY		0
#define IS_BUTTON	1
#define IS_SELECTBTN	2
#define IS_MENUBTN	3
#define IS_OTHERBTN	4

static void	HandleButton OL_ARGS((Widget, OlVirtualEvent));
static void	HandleCrossing OL_ARGS((Widget, OlVirtualEvent));
static void	HandleKey OL_ARGS((Widget, OlVirtualEvent));
static void	HandleMotion OL_ARGS((Widget, OlVirtualEvent));
static void	HandleSelect OL_ARGS((Widget, int));
static void	LeaveParse OL_ARGS((Widget, XEvent *));
static void	ParseMenuBtnDown OL_ARGS((Widget, XEvent *));
static void	ParseMenuBtnMotion OL_ARGS((Widget, XEvent *));
static void	ParseBtnUp OL_ARGS((Widget, XEvent *, int));
static void	Preview OL_ARGS((Widget, XEvent *));

					/* public procedures		*/
/* There are no public routines */

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

static OlEventHandlerRec
handlers[] = {
	{ ButtonPress,		HandleButton	},
	{ ButtonRelease,	HandleButton	},
	{ LeaveNotify,		HandleCrossing	},
	{ KeyPress,		HandleKey	},
	{ MotionNotify,		HandleMotion	},
};

#define OFFSET(field)	XtOffset(AbbrevMenuButtonWidget, field)

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

static char
translations[] = "\
	<FocusIn>:	OlAction() \n\
	<FocusOut>:	OlAction() \n\
	<Key>:		OlAction() \n\
	<BtnDown>:	OlAction() \n\
	<BtnUp>:	OlAction() \n\
\
	<Leave>:	OlAction() \n\
	<BtnMotion>:	OlAction() \n\
";

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource
resources[] = {

	{ XtNshellBehavior, XtCShellBehavior, XtRInt, sizeof(int),
	  OFFSET(menubutton.shell_behavior), XtRImmediate,
	  (XtPointer) BaseWindow},

	{ XtNset, XtCSet, XtRBoolean, sizeof(Boolean),
	  OFFSET(menubutton.set), XtRImmediate, (XtPointer) False },

	{ XtNbusy, XtCBusy, XtRBoolean, sizeof(Boolean),
	  OFFSET(menubutton.busy), XtRImmediate, (XtPointer) False },

	{ XtNscale, XtCScale, XtRInt, sizeof(int),
	  OFFSET(menubutton.scale), XtRImmediate,
	  (XtPointer) OL_DEFAULT_POINT_SIZE },

	{ XtNpreviewWidget, XtCPreviewWidget, XtRWidget, sizeof(Widget),
	  OFFSET(menubutton.preview_widget), XtRWidget, (XtPointer) NULL },

	{ XtNpostSelect, XtCCallback, XtRCallback, sizeof(XtCallbackList),
	  OFFSET(menubutton.post_select), XtRCallback, (XtPointer) NULL },
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

AbbrevMenuButtonClassRec
abbrevMenuButtonClassRec = {
  {
	(WidgetClass) &primitiveClassRec,	/* superclass		*/
	"AbbrevMenuButton",			/* class_name		*/
	sizeof(AbbrevMenuButtonRec),		/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	Initialize,				/* initialize		*/
	InitializeHook,				/* initialize_hook	*/
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
	Redisplay,				/* expose		*/
	SetValues,				/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	GetValuesHook,				/* get_values_hook	*/
	XtInheritAcceptFocus,			/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_private	*/
	translations,				/* tm_table		*/
	XtInheritQueryGeometry			/* query_geometry	*/
  },	/* End of CoreClass field initializations */
  {
        False,					/* focus_on_select	*/
	HighlightHandler,			/* highlight_handler	*/
	NULL,					/* traversal_handler	*/
	NULL,					/* register_focus	*/
	ActivateWidget,				/* activate		*/
	handlers,				/* event_procs		*/
	XtNumber(handlers),			/* num_event_procs	*/
	OlVersion,				/* version		*/
	NULL					/* extension		*/
  },	/* End of Primitive field initializations */
  {
	NULL,					/* field not used	*/
  }	/* End of AbbrevMenuButtonClass field initializations */
}; 

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass abbrevMenuButtonWidgetClass = (WidgetClass) &abbrevMenuButtonClassRec;
WidgetClass abbrevStackWidgetClass = (WidgetClass) &abbrevMenuButtonClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * ActivateDefault - this routine is called to activate the menubutton's
 * menu default selection. 
 ****************************procedure*header*****************************
 */
static void
ActivateDefault(w)
	Widget	w;
{
	AbbrevMenuButtonWidget ambw = (AbbrevMenuButtonWidget)w;
	Widget	submenu = ambw->menubutton.submenu;

	if (submenu != (Widget) NULL)
	{
		Widget		the_default = _OlGetDefault(submenu);
		ShellBehavior	sb =
			((MenuShellWidget) submenu)->menu.shell_behavior;

		if (the_default == (Widget) NULL ||
		    sb == PinnedMenu ||
		    sb == UnpinnedMenu ||
		    OlActivateWidget(the_default, OL_SELECTKEY,
					(XtPointer)OL_NO_ITEM) == False)
		{
			_OlBeepDisplay((Widget) ambw, 1);
		}
	}
} /* END OF ActivateDefault() */

/*
 *************************************************************************
 * ActivateWidget - this routine provides the external interface for
 * others to activate this widget indirectly.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
ActivateWidget OLARGLIST((w, type, data))
	OLARG( Widget,		w)
	OLARG( OlVirtualName,	type)
	OLGRA( XtPointer,	data)
{
	Boolean	ret_val = FALSE;

	switch(type)
	{
		case OL_SELECTKEY:
			ret_val = TRUE;
			if (_OlSelectDoesPreview(w) == True)
			{
				HandleSelect (w, IS_SELECTBTN);
				break;
			}
/* FALLTHROUGH from OL_SELECTKEY to OL_MENUKEY */
		case OL_MENUKEY:
			{
				AbbrevMenuButtonWidget ambw =
					(AbbrevMenuButtonWidget) w;
				MenuShellWidget menu = (MenuShellWidget)
					ambw->menubutton.submenu;

				ret_val = TRUE;

				if (menu->shell.popped_up == False)
				{
					PopupSubmenu (ambw, menu,
						(int) 0, (int) 0,
						(int) 0, (int) 0,
						IS_KEY);
				}
				else if (menu->menu.shell_behavior
						== PinnedMenu)
				{
					XRaiseWindow(XtDisplay((Widget)menu),
							XtWindow((Widget)menu));
				}
			}
			break;
	}
	return(ret_val);
} /* END OF ActivateWidget() */

/*
 *************************************************************************
 * GetNormalGC - this routine gets the normal GC for the Abbreviated
 * Button MenuButton Widget.
 ****************************procedure*header*****************************
 */
static void
GetNormalGC(ambw)
	AbbrevMenuButtonWidget ambw;
{
	Pixel	focus_color;
	Boolean	has_focus;
						/* Destroy existing GC	*/

	if (ambw->menubutton.pAttrs != (OlgAttrs *) NULL) {
		OlgDestroyAttrs (ambw->menubutton.pAttrs);
	}

	focus_color = ambw->primitive.input_focus_color;
	has_focus = ambw->primitive.has_focus;

	if (has_focus)
	{
	    if (ambw->primitive.foreground == focus_color ||
		ambw->core.background_pixel == focus_color)
	    {
		/* reverse fg and bg. */
		if (OlgIs3d ())
		{
		    ambw->menubutton.pAttrs =
			OlgCreateAttrs(
				XtScreenOfObject((Widget)ambw),
				ambw->core.background_pixel,
				(OlgBG *)&(ambw->primitive.foreground),
				False, ambw->menubutton.scale);
		}
		else
		{
		    ambw->menubutton.pAttrs =
			OlgCreateAttrs(
				XtScreenOfObject((Widget)ambw),
				ambw->primitive.foreground,
				(OlgBG *)&(ambw->core.background_pixel),
				False, ambw->menubutton.scale);
		}
	    }
	    else
		ambw->menubutton.pAttrs =
		    OlgCreateAttrs(
				XtScreenOfObject ((Widget)ambw),
				ambw->primitive.foreground,
				(OlgBG *)&(focus_color),
				False, ambw->menubutton.scale);
	}
	else
	    ambw->menubutton.pAttrs = OlgCreateAttrs(
					XtScreenOfObject ((Widget)ambw),
					ambw->primitive.foreground,
					(OlgBG *)&(ambw->core.background_pixel),
					False, ambw->menubutton.scale);

} /* END OF GetNormalGC() */

/*
 ************************************************************
 *
 *  GetWidgetDimensions - this function returns the width and height
 *	of the widget visual as function of point size and screen resolution.
 *
 *********************function*header************************
 */
static void
GetWidgetDimensions(w, change_width, change_height)
	Widget	w;
	Boolean	change_width;
	Boolean	change_height;
{
	if (change_width == True || change_height == True)
	{
		AbbrevMenuButtonWidget	ambw = (AbbrevMenuButtonWidget) w;
		Dimension	width, height;

		OlgSizeAbbrevMenuB (XtScreen (ambw), ambw->menubutton.pAttrs,
				    &width, &height);

		if (change_width == True)
		{
			ambw->core.width = width;
		}

		if (change_height == True)
		{
			ambw->core.height = height;
		}
	}
} /* GetWidgetDimensions */

/*
 *************************************************************************
 * PopupSubmenu - this routine does the basic steps to popup a submenu
 ****************************procedure*header*****************************
 */
static void
PopupSubmenu(ambw, menu, root_x, root_y, window_x, window_y, flag)
	AbbrevMenuButtonWidget	ambw;
	MenuShellWidget		menu;
	int			root_x;
	int			root_y;
	int			window_x;
	int			window_y;
	OlDefine		flag;		/* IS_KEY or IS_BUTTON */
{

	OlDefine		state;
						/* Highlight the button	*/

	ambw->menubutton.set = True;
	if (XtIsRealized((Widget)ambw) == True)
	{
		(*(XtClass((Widget)ambw)->core_class.expose))
			((Widget) ambw, (XEvent *) NULL, (Region) NULL);
	}

				/* Cache the button's position
				 * relative to the RootWindow.		*/

	if (flag == IS_BUTTON)
	{
		ambw->menubutton.root_x = (Position) (root_x - window_x);
		ambw->menubutton.root_y = (Position) (root_y - window_y);

		state = OL_PRESS_DRAG_MENU;
	}
	else	/* IS_KEY */
	{
		Position	rx, ry;

		XtTranslateCoords ((Widget) ambw,
				(Position) 0, (Position) 0,
				&rx, &ry);

		ambw->menubutton.root_x = root_x = (Position) rx;
		ambw->menubutton.root_y = root_y = (Position) ry;

		state = OL_STAYUP_MENU;
	}


	OlMenuPopup((Widget) menu, (Widget)ambw, OL_NO_ITEM, state, TRUE,
			root_x, root_y, PositionSubmenu);

	_OlAddGrab((Widget)ambw, False, False);
} /* END OF PopupSubmenu() */

/*
 *************************************************************************
 * PositionSubmenu - This routine has the task of positioning the
 * AbbrevMenuButton's submenu under the parent AbbrevMenuButton.  The call
 * to this routine is handled by the menu posting routine.
 *	This routine should never be called if the menu is pinned since
 * pinned menus post themselves in the last position.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
PositionSubmenu OLARGLIST((w, emanate, emanate_index, state, mx, my, px, py))
	OLARG( register Widget, w)		/* MenuShellWidget id	*/
	OLARG( Widget,		emanate)	/* ignored		*/
	OLARG( Cardinal,	emanate_index)	/* ignored		*/
	OLARG( OlDefine,	state)		/* menu's current state	*/
	OLARG( Position *,	mx)
	OLARG( Position *,	my)
	OLARG( Position *,	px)
	OLGRA( Position *,	py)
{
	register MenuShellWidget menu = (MenuShellWidget) w;
	AbbrevMenuButtonWidget ambw;	/* AbbrevMenuButton Widget id	*/
	Boolean		warp_pointer = False;
	Position	x;	/* menu position relative to RootWindow	*/
	Position	y;	/* menu position relative to RootWindow	*/
	Position	abs_x;	/* Absolute x coor. wrt to root		*/
	Position 	abs_y;	/* Absolute y coor. wrt to root		*/
	Position	horiz_buf;/* distance from right menu edge to
				   right screen edge			*/
	Position	vert_buf;/* distance from bottom menu edge to
				   bottom screen edge			*/

				/* Check for Parent's existence		*/

	if ( !(ambw = (AbbrevMenuButtonWidget) w->core.parent) )

 		OlVaDisplayErrorMsg(	XtDisplay(w),
 					OleNbadParent,
 					OleTnoMenuParent,
 					OleCOlToolkitError,
 					OleMbadParent_noMenuParent,
 					XtName(w),
 					OlWidgetToClassName(w));

			/* Get the coordinates of the menubutton
			 * which are relative to root.	These were
			 * cached on the button press or motion that
			 * caused this routine to be called.		*/

	abs_x = ambw->menubutton.root_x;
	abs_y = ambw->menubutton.root_y;

			/* Position the submenu so that the upper left
			 * hand corner is right under the lower left
			 * hand corner of the Abbreviated Button MenuButton.	*/
			
	x = abs_x;
	y = abs_y + ambw->core.height;

				/* Now, make sure that the menu does not
				 * go off of the screen			*/

	horiz_buf = (Position)WidthOfScreen(XtScreen(w)) -
			(x + (Position)w->core.width + (Position)
			(w->core.border_width * (Dimension)2));
	vert_buf = (Position)HeightOfScreen(XtScreen(w)) -
			(y + (Position) w->core.height + (Position)
			((w->core.border_width) * (Dimension)2));

			/* If the horizontal buffer is less than zero,
			 * we've gone off the right edge; so, move the
			 * menu to the left. (we don't have to warp the
			 * pointer in this case.			*/

	if (horiz_buf < (Position)0)
	{
		x += horiz_buf;
	}

	if (vert_buf < (Position)0)
	{
		y += vert_buf;
		warp_pointer = True;
	}

			/* Check to see if the submenu is off the left
			 * edge of the screen.				*/

	x = (x < (Position)0 ? (Position)0 : x);
		
			/* See if the pointer needs to be warped.  If
			 * does, calculate the new position.
			 * When warping the pointer, the vertical
			 * position never changes.  The horizontal
			 * horizontal position moves to be four
			 * points to the left of the pane items.	*/

	if (warp_pointer == True)
	{
		register Widget self;
		Position	four_points;

		self = _OlGetDefault((Widget)menu);

			/* If the default is a Pushpin, get the next
			 * item						*/

		if (self != (Widget) NULL &&
		    XtIsSubclass(self, pushpinWidgetClass) == True)
		{
			CompositeWidget cw = (CompositeWidget)menu->menu.pane;

			if (cw->composite.num_children > 0)
				self = cw->composite.children[0];
			else
				self = (Widget) NULL;
		}

			/* Calculate the position of the widget w.r.t
			 * the menu.					*/

		*px = x;
		if (self == (Widget) NULL)
		{
			*px += (Position)OlScreenPointToPixel(OL_HORIZONTAL,
							8, XtScreen(w));
		}

		for (; self != (Widget)NULL && self != (Widget) menu;
		     self = self->core.parent)
		{
			*px += self->core.x + (Position)self->core.border_width;
		}

			/* Move the pointer 4 points to the left	*/

		four_points = (Position)OlScreenPointToPixel(OL_HORIZONTAL, 4,
					XtScreen(w));

		if (*px < x ) {
			*px = x;
		}
		else if (*px > (x + four_points)) {
			*px -= four_points;
		}
	}

	*mx = x;
	*my = y;

			/* If we have to Warp the pointer, update the
			 * posting location.				*/

	if (warp_pointer == True)
	{
			/* Prevent the pointer from being warped off both
			 * the menubutton and the submenu.
			 * (I know this is not spec compliant !!!)	*/

		if (*py < menu->core.y && 
		    (*px < abs_x ||
		    (*px > (abs_x + (Position)ambw->core.width))))
		{
			*py = menu->core.y + (Position)
					OlScreenPointToPixel(OL_VERTICAL,
						1, XtScreen(w));
		}
	}

} /* END OF PositionSubmenu() */

/*
 *************************************************************************
 * RevertButton - This routine returns an inverted AbbrevMenuButton to its
 * normal state.  This procedure is called when a menu is posted in
 * the "stay-up" mode.
 ****************************procedure*header*****************************
 */
static void
RevertButton(menu, for_stay_up_mode)
	MenuShellWidget menu;		/* Menu that is child of button	*/
	Boolean		for_stay_up_mode;	/* is this call because
						 * we want stayUp mode	*/
{
	Widget parent = menu->core.parent;

	if (parent == (Widget) NULL)
		parent = menu->menu.parent_cache;

	if (parent == (Widget) NULL || menu->menu.application_menu == True)
		return;

	if (XtIsSubclass(parent, abbrevMenuButtonWidgetClass) == True)
	{
		AbbrevMenuButtonWidget ambw = (AbbrevMenuButtonWidget) parent;

		if (for_stay_up_mode == True ||
		    ambw->menubutton.busy == True ||
		    ambw->menubutton.set == True)
		{
			ambw->menubutton.busy = (for_stay_up_mode == True ?
						True : False);
			ambw->menubutton.set = False;

			XClearArea(XtDisplay((Widget) ambw),
				XtWindow((Widget) ambw), (int)0,(int)0,
					(unsigned int)0, (unsigned int)0,
					(Bool)True);
		}
	}

} /* END OF RevertButton() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * ClassInitialize - Register OlDefine string values.
 ****************************procedure*header*****************************
 */
static void
ClassInitialize()
{
	_OlAddOlDefineType ("none", OL_NONE);
	_OlAddOlDefineType ("out",  OL_OUT);
	_OlAddOlDefineType ("in",   OL_IN);
} /* END OF ClassInitialize() */

/*
 *************************************************************************
 * Destroy - free the GCs stored in Abbreviated Button MenuButton widget.
 ****************************procedure*header*****************************
 */
static void
Destroy(w)
    Widget w;
{
	AbbrevMenuButtonWidget ambw = (AbbrevMenuButtonWidget) w;

	OlgDestroyAttrs (ambw->menubutton.pAttrs);
	XtRemoveAllCallbacks(w, XtNpostSelect);

} /* END OF Destroy() */

/*
 *************************************************************************
 * GetValuesHook - This procedure allows the user to obtain menu
 * information directly from the menubutton widget.  This procedure
 * uses the routine _OlComposeArgList() to determine which arguments are
 * to be sent on to the submenu.
 ****************************procedure*header*****************************
 */
static void
GetValuesHook(w, args, num_args)
	Widget    w;
	ArgList   args;
	Cardinal *num_args;
{
	Cardinal          count;
	AbbrevMenuButtonWidget ambw = (AbbrevMenuButtonWidget) w;
	ArgList	          new_list;
	static MaskArg    mask_args[] = {
		{ XtNmenu, NULL /* See below */,OL_COPY_MASK_VALUE },
		{ NULL,	(XtArgVal) sizeof(Widget),OL_COPY_SIZE	},
		{ XtNmenuPane,		NULL,	OL_SOURCE_PAIR	},
		{ XtNpushpin,		NULL,	OL_SOURCE_PAIR	},
		{ XtNpushpinDefault,	NULL,	OL_SOURCE_PAIR	},
		{ XtNtitle,		NULL,	OL_SOURCE_PAIR	}
	};

				/* before parsing the mask arg list,
				 * put in the id of the submenu.	*/

	mask_args[0].value = (XtArgVal)ambw->menubutton.submenu;

	_OlComposeArgList(args, *num_args, mask_args, XtNumber(mask_args),
			 &new_list, &count);

			/* Call the menu with the get values if
			 * necessary					*/

	if (count != (Cardinal)0)
	{
		if (ambw->menubutton.submenu)
			XtGetValues((Widget) ambw->menubutton.submenu,
					new_list, count);

		XtFree((char *)new_list);
	}

} /* END OF GetValuesHook() */

/*
 *************************************************************************
 * HighlightHandler - changes the colors when this widget gains or loses
 * focus.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
HighlightHandler OLARGLIST((w, type))
	OLARG(Widget,	w)
	OLGRA(OlDefine,	type)
{
	GetNormalGC ((AbbrevMenuButtonWidget) w);
	Redisplay (w, NULL, NULL);
} /* END OF HighlightHandler() */

/*
 *************************************************************************
 * Initialize - Initializes the AbbrevMenuButton Instance.  Any conflicts 
 * between the "request" and "new" widgets should be resolved here.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize(request, new, args, num_args)
	Widget		request;	/* What user wants		*/
	Widget		new;		/* What user gets, so far....	*/
	ArgList		args;
	Cardinal *	num_args;
{
	AbbrevMenuButtonWidget	nambw = (AbbrevMenuButtonWidget) new;
	Widget			shell;

	shell = _OlGetShellOfWidget(new);

	if (shell != (Widget) NULL &&
	    XtIsSubclass(shell, menuShellWidgetClass) == True)

 		OlVaDisplayErrorMsg(	XtDisplay(new),
 					OleNgoodParent,
 					OleTbadChild,
 					OleCOlToolkitError,
 					OleMgoodParent_badChild,
 					XtName(new),
 					OlWidgetToClassName(new),
 					OlWidgetToClassName(shell));

	nambw->menubutton.previewing_default	= False;

		/* Get the GCs	*/

	nambw->menubutton.pAttrs= (OlgAttrs *) NULL;

	GetNormalGC(nambw);

	GetWidgetDimensions(new,
		(new->core.width == (Dimension)0 ? True : False),
		(new->core.height == (Dimension)0 ? True : False));

	new->core.border_width = 0;

} /* END OF Initialize() */

/*
 *************************************************************************
 * InitializeHook - this procedure creates the submenu for the
 * menubutton.
 ****************************procedure*header*****************************
 */
static void
InitializeHook(w, args, num_args)
	Widget    w;
	ArgList   args;
	Cardinal *num_args;
{
	Cardinal          count;
	AbbrevMenuButtonWidget ambw = (AbbrevMenuButtonWidget) w;
	char *		  menu_name = "menu";		/* default name	*/
	ArgList           comp_args;
	static MaskArg    mask_args[] = {
		{ XtNtitle,		NULL /* below */,
						OL_DEFAULT_PAIR	},
		{ XtNmenuName,		NULL /* below */,
						OL_COPY_SOURCE_VALUE },
		{ NULL,	(XtArgVal)sizeof(String),OL_COPY_SIZE	},
		{ XtNpushpin,		OL_NONE,OL_DEFAULT_PAIR },
		{ XtNpushpinDefault,	NULL,	OL_SOURCE_PAIR },
		{ XtNpaneName,		NULL,	OL_SOURCE_PAIR },
		{ XtNmenuAugment, (XtArgVal) False, OL_OVERRIDE_PAIR },
		{ XtNrevertButton, (XtArgVal) ((void (*)()) RevertButton),
						OL_OVERRIDE_PAIR },
		{ XtNcenter,		NULL,	OL_SOURCE_PAIR },
		{ XtNhPad,		NULL,	OL_SOURCE_PAIR },
		{ XtNhSpace,		NULL,	OL_SOURCE_PAIR },
		{ XtNlayoutType,	NULL,	OL_SOURCE_PAIR },
		{ XtNmeasure,		NULL,	OL_SOURCE_PAIR },
		{ XtNsameSize,		NULL,	OL_SOURCE_PAIR },
		{ XtNvPad,		NULL,	OL_SOURCE_PAIR },
		{ XtNvSpace,		NULL,	OL_SOURCE_PAIR }
	};

			/* Set the default submenu title		*/

	_OlSetMaskArg(mask_args[0], XtNtitle, ambw->core.name, OL_DEFAULT_PAIR);

			/* Extract the menu's name from the Arg list	*/

	_OlSetMaskArg(mask_args[1], XtNmenuName, &menu_name,
			OL_COPY_SOURCE_VALUE);

			/* Parse the arglist for the menu		*/

	_OlComposeArgList(args, *num_args, mask_args, XtNumber(mask_args),
			 &comp_args, &count);

			/* Now Create the Submenu			*/

	ambw->menubutton.submenu = XtCreatePopupShell(menu_name,
				menuShellWidgetClass, w, comp_args, count);

	XtFree((char *)comp_args);

} /* END OF InitializeHook() */

/*
 *************************************************************************
 * Redisplay - this routine draws the AbbrevMenuButton in its
 * window.  The image of the AbbrevMenuButton is drawn for each
 * refresh.  The image will be as big as the AbbrevMenuButton's
 * window.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Redisplay(w, xevent, region)
	Widget		w;
	XEvent *	xevent;
	Region		region;
{

	if (XtIsRealized(w) == True)
	{
		AbbrevMenuButtonWidget	ambw = (AbbrevMenuButtonWidget) w;
		unsigned	flags;

		/* the button is draw as set if it really is set or busy,
		 * or if it is 2-D and has input focus and the input
		 * focus color conflicts with either the foreground or
		 * background color.
		 */
		if (ambw->menubutton.set || ambw->menubutton.busy ||
		    (!OlgIs3d () && ambw->primitive.has_focus &&
		     (ambw->primitive.input_focus_color ==
		      ambw->core.background_pixel ||
		      (ambw->primitive.input_focus_color ==
		       ambw->primitive.foreground))))
		    flags = AM_SELECTED;
		else
		    flags = AM_NORMAL;

		OlgDrawAbbrevMenuB (XtScreen (ambw), XtWindow (ambw),
				    ambw->menubutton.pAttrs, 0, 0, flags);
	}

} /* END OF Redisplay() */

/*
 *************************************************************************
 * SetValues - used to set resources associated with the AbbrevMenuButtonPart.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues(current, request, new, args, num_args)
	Widget		current;	/* The original Widget		*/
	Widget		request;	/* Widget user wants		*/
	Widget		new;		/* Widget user gets, so far ...	*/
	ArgList		args;
	Cardinal *	num_args;
{
	Boolean           redisplay = False;
	AbbrevMenuButtonWidget cambw  = (AbbrevMenuButtonWidget) current;
	AbbrevMenuButtonWidget nambw = (AbbrevMenuButtonWidget) new;

	if (nambw->primitive.foreground != cambw->primitive.foreground ||
	    nambw->core.background_pixel != cambw->core.background_pixel ||
	    nambw->primitive.input_focus_color !=
				cambw->primitive.input_focus_color)
	{
		GetNormalGC(nambw);
		redisplay = True;
	}

	return(redisplay);

} /* END OF SetValues() */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 * LeaveParse - this function is called when the mouse pointer
 * leaves the AbbrevMenuButton's window.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
LeaveParse(w, xevent)
	register Widget  w;		/* parent widget of Menu	*/
	XEvent          *xevent;	/* Menu widget's XEvent	*/
{
	AbbrevMenuButtonWidget	ambw = (AbbrevMenuButtonWidget) w;
	MenuShellWidget		menu = (MenuShellWidget)
					ambw->menubutton.submenu;
	Boolean			in_menu;	 

				/* Ignore LeaveWindow events
				 * generated by pointer grabs		*/

	if (menu == (MenuShellWidget) NULL ||
	    xevent->xcrossing.mode != NotifyNormal)
	{
		return;
	}
				/* Turn Off the previewing, if it's on	*/

	if (ambw->menubutton.previewing_default == True)
	{
		ambw->menubutton.busy = (menu->shell.popped_up == True &&
			menu->menu.shell_behavior == StayUpMenu ? True : False);
		ambw->menubutton.previewing_default = False;

					/* clear the window to insure
					 * removal of visual		*/

		XClearArea(XtDisplay(w), XtWindow(w), 0,0,0,0, (Bool)True);

					/* Now clear the preview widget	*/

		if (ambw->menubutton.preview_widget != (Widget) NULL)
		{
			_OlClearWidget(ambw->menubutton.preview_widget,
					(Bool)True);
		}
		return;
	}

	if (menu->shell.popped_up == True &&
	    menu->menu.shell_behavior == PressDragReleaseMenu )
	{
		in_menu =
			(xevent->xcrossing.x_root >= (int)menu->core.x &&
			 xevent->xcrossing.x_root <= (int)menu->core.x +
						      (int)menu->core.width &&
			 xevent->xcrossing.y_root >= (int)menu->core.y &&
			 xevent->xcrossing.y_root <= (int)menu->core.y +
						      (int)menu->core.height
				? True : False);

			/* If the Crossing XEvent coordinates are within
			 * the MENU Button Damping Factor and we have
			 * not yet prevented StayUp mode, then
			 * this Leave event probably came from a 
			 * a PointerWarp from the submenu positioning
			 * routine.					*/

		menu->menu.prevent_stayup = (in_menu == True &&
			 menu->menu.prevent_stayup == False &&
			 xevent->xcrossing.x_root == (int)menu->menu.post_x &&
			 xevent->xcrossing.y_root == (int)menu->menu.post_y
				? False : True);

		if (in_menu == False)
		{
			OlMenuUnpost((Widget)menu);
		} 
	}
} /* END OF LeaveParse() */

/*
 *************************************************************************
 * ParseBtnUp - this procedure handles what happens when any button is
 * released over a menu button.
 * This routine first checks to see if the user has chosen to use the
 * "power-user" feature for the SELECT Button.  If this feature is on,
 * the routine does the logic associated with previewing and activating
 * a submenu default.  If the feature is off, the routine does the logic
 * associated with a menu button.
 * Since this routine handles button releases for SELECT, MENU and any
 * other button, the flag is utilized.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
ParseBtnUp(w, xevent, flag)
	Widget		w;		/* menu button Widget		*/
	XEvent *	xevent;		/* Menu widget's XEvent		*/
	int		flag;		/* IS_SELECT/MENU/OTHERBTN	*/
{
	AbbrevMenuButtonWidget ambw = (AbbrevMenuButtonWidget) w;

	if (_OlSelectDoesPreview(w) == True)
	{
				/* If we're in this part of the code,
				 * make sure that a SELECT button got us
				 * here.				*/

		if (flag == IS_MENUBTN || /* because Menu will handle this */
		    ambw->menubutton.previewing_default == False)
		{
			return;
		}

		HandleSelect(w, flag);
	}
	else			/* SELECT power-user feature is off	*/
	{
				/* If we're in this part of the code,
				 * make sure that a MENU button got us
				 * here.				*/

		if (flag == IS_MENUBTN)
		{
			MenuShellWidget menu =
				(MenuShellWidget) ambw->menubutton.submenu;

			if (menu->shell.popped_up == False)
			{
				PopupSubmenu(ambw, menu,
					xevent->xbutton.x_root,
					xevent->xbutton.y_root,
					xevent->xbutton.x,
					xevent->xbutton.y,
					IS_BUTTON);
			}
		}
	}
} /* END OF ParseBtnUp() */

/*
 *************************************************************************
 * HandleButton - handles all button press/release for this widget
 ****************************procedure*header*****************************
 */
static void
HandleButton(w, ve)
	Widget	w;
	OlVirtualEvent	ve;
{
	switch (ve->virtual_name)
	{
		case OL_SELECT:
			ve->consumed = True;
			if (ve->xevent->type == ButtonPress)
				Preview (w, ve->xevent);
			else			/* ButtonRelease */
				ParseBtnUp (w, ve->xevent, IS_SELECTBTN);
			break;
		case OL_MENU:
			ve->consumed = True;
			if (ve->xevent->type == ButtonPress)
				ParseMenuBtnDown (w, ve->xevent);
			else			/* ButtonRelease */
				ParseBtnUp (w, ve->xevent, IS_MENUBTN);
			break;
		default:
			if (ve->xevent->type == ButtonRelease)
			{
				ve->consumed = True;
				ParseBtnUp (w, ve->xevent, IS_OTHERBTN);
			}
			break;
	}
} /* END OF HandleButton() */

/*
 *************************************************************************
 * HandleCrossing - handles all enter/leave for this widget
 ****************************procedure*header*****************************
 */
static void
HandleCrossing(w, ve)
	Widget	w;
	OlVirtualEvent	ve;
{
	if (ve->xevent->type == LeaveNotify)
	{
		ve->consumed = True;
		LeaveParse (w, ve->xevent);
	}
} /* END OF HandleCrossing() */

/*
 *************************************************************************
 * HandleKey - handles all keypresses for this widget
 ****************************procedure*header*****************************
 */
static void
HandleKey(w, ve)
	Widget	w;
	OlVirtualEvent	ve;
{
			/* Trap the two generic move-related keypresses
			 * here before they fall through to the
			 * superclass.					*/

	if (ve->virtual_name == OL_MOVEDOWN)
	{
		ve->consumed = True;
		OlActivateWidget(w, OL_MENUKEY, (XtPointer)NULL);
	}
} /* END OF HandleKey() */

/*
 *************************************************************************
 * HandleMotion - handles all motion for this widget
 ****************************procedure*header*****************************
 */
static void
HandleMotion(w, ve)
	Widget	w;
	OlVirtualEvent	ve;
{
	switch (ve->virtual_name)
	{
		case OL_SELECT:
			ve->consumed = True;
			Preview (w, ve->xevent);
			break;
		case OL_MENU:
			ve->consumed = True;
			ParseMenuBtnMotion (w, ve->xevent);
			break;
	}
} /* END OF HandleMotion() */

/*
 *************************************************************************
 * HandleSelect - this routine is extracted from ParseBtnUp() so that
 *		both Button and Key can use it.
 ****************************procedure*header*****************************
 */
static void
HandleSelect(w, flag)
	Widget		w;	/* menu button widget */
	int		flag;	/* IS_SELECT/OTHERBTN */
{
	AbbrevMenuButtonWidget ambw = (AbbrevMenuButtonWidget) w;

			/* We should be in here only if we've
			 * been previewing.			*/

	ambw->menubutton.previewing_default = False;

			/* If the number of parameters is
			 * non-zero, it is OK to activate
			 * the default.				*/

	if (flag == IS_SELECTBTN)
	{
		ActivateDefault((Widget)ambw);
		XtCallCallbacks((Widget) ambw, XtNpostSelect,
					(XtPointer) NULL);
	}
	
		/* If the AbbrevMenuButton is busy, "unbusy" it	*/

	if (ambw->menubutton.busy == True)
	{
		ambw->menubutton.busy = False;
		_OlClearWidget(w, (Bool)True);
	}
				/* Now clear the preview widget	*/

	if (ambw->menubutton.preview_widget != (Widget) NULL)
	{
		_OlClearWidget(ambw->menubutton.preview_widget,
					(Bool)True);
	}

	return;
} /* END OF HandleSelect() */

/*
 *************************************************************************
 * ParseMenuBtnDown - this procedure handles what happens when a MENU
 * button is pressed on a AbbrevMenuButton.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
ParseMenuBtnDown(w, xevent)
	register Widget  w;		/* AbbrevMenuButton Widget	*/
	XEvent   	*xevent;	/* Menu widget's XEvent		*/
{
	AbbrevMenuButtonWidget ambw = (AbbrevMenuButtonWidget) w;
	MenuShellWidget   menu = (MenuShellWidget) ambw->menubutton.submenu;

				/* Remove the active pointer grab so
				 * that other widgets can get events.   */

	_OlUngrabPointer(w);

	if (menu != (MenuShellWidget) NULL)
	{
		if (menu->shell.popped_up == False)
		{
			PopupSubmenu(ambw, menu,
				xevent->xbutton.x_root, xevent->xbutton.y_root,
				xevent->xbutton.x, xevent->xbutton.y,
				IS_BUTTON);
		}
		else if (menu->menu.shell_behavior == PinnedMenu ||
			 menu->menu.shell_behavior == UnpinnedMenu) 
		{
			XRaiseWindow(XtDisplay((Widget)menu),
					XtWindow((Widget)menu));
		}
	}
} /* END OF ParseMenuBtnDown() */

/*
 *************************************************************************
 * ParseMenuBtnMotion - figures out what to do when the menubutton
 * receives a MENU button motion XEvent.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
ParseMenuBtnMotion(w, xevent)
	register Widget  w;		/* parent widget of Menu	*/
	XEvent   	*xevent;	/* Menu widget's XEvent	*/
{
	AbbrevMenuButtonWidget	ambw = (AbbrevMenuButtonWidget) w;
	MenuShellWidget		menu = (MenuShellWidget)
						ambw->menubutton.submenu;
	XEvent			leave_event;
	Boolean			pending_leave;
	register XEvent	*	xev = &leave_event;
	register Window		win = XtWindow(w);
	register Display *	dpy = XtDisplay(w);


	if (menu == (MenuShellWidget) NULL)
	{
		return;
	}

				/* If there is a Leave XEvent already
				 * on the queue for this AbbrevMenuButton,
				 * ignore this event.  If we find an
				 * event, remove the all motion
				 * events up to the leave event.	*/

	do {
		pending_leave = XCheckWindowEvent(dpy, win,
						(int) LeaveWindowMask, xev);

				/* Make Sure the leave event is for a
				 * Normal Leave event			*/

	} while (pending_leave == True && xev->xcrossing.mode != NotifyNormal);

	if (pending_leave == True && xev->xcrossing.mode == NotifyNormal)
	{
		XEvent	motion_event;
		Time	leave_time = xev->xcrossing.time;

				/* remove all motion events up to this
				 * leave				*/

		xev = &motion_event;
		while (QLength(dpy) > 0 && XCheckWindowEvent(dpy, win,
				(int) ButtonMotionMask, xev) == True)
		{
			if (leave_time < xev->xmotion.time)
			{
				XPutBackEvent(dpy, xev);
				break;
			}
		}

				/* Only put the LeaveEvent back on the
				 * queue if the submenu is up already	*/

		if (menu->shell.popped_up == True)
		{
			XPutBackEvent(dpy, &leave_event);
		}
		return;
	}

	if (menu->shell.popped_up == False)
	{
		PopupSubmenu(ambw, menu,
				xevent->xmotion.x_root, xevent->xmotion.y_root,
				xevent->xmotion.x, xevent->xmotion.y,
				IS_BUTTON);

				/* Prevent Stay-up mode from happening	*/

		menu->menu.prevent_stayup = True;
	}
	else				/* Menu already popped up	*/
	{
		if (menu->menu.shell_behavior == PressDragReleaseMenu &&
		    menu->menu.prevent_stayup == False)
		{

			if (!_OlIsMenuMouseClick(menu, xevent->xmotion.x_root,
						xevent->xmotion.y_root))
			{
				menu->menu.prevent_stayup = True;
			}
		}
	}

} /* END OF ParseMenuBtnMotion() */

/*
 *************************************************************************
 * Preview - this routine is called to preview the menubutton submenu's
 * default selection.  If there is no preview widget, the routine returns.
 ****************************procedure*header*****************************
 */
static void
Preview(w, xevent)
	Widget		w;		/* AbbrevMenuButton widget	*/
	XEvent *	xevent;		/* Action-causing XEvent	*/
{
	if (_OlSelectDoesPreview(w) == False)
	{
		if (xevent->type == ButtonPress)
		{
			ParseMenuBtnDown(w, xevent);
		}
		else
		{
			ParseMenuBtnMotion(w, xevent);
		}
	}
	else
	{
		AbbrevMenuButtonWidget ambw = (AbbrevMenuButtonWidget) w;
		Widget            submenu = ambw->menubutton.submenu;

		if (xevent->xany.type == ButtonPress)
		{
			_OlUngrabPointer(w);
		}

		if (submenu == (Widget) NULL)
		{
			return;
		}

		if (ambw->menubutton.previewing_default == False)
		{
			ShellBehavior	sb = ((MenuShellWidget) submenu)->
						menu.shell_behavior;
			Widget		the_default = _OlGetDefault(submenu);

			ambw->menubutton.previewing_default	= True;
			ambw->menubutton.busy			= True;

			if (the_default != (Widget) NULL &&
			    ambw->menubutton.preview_widget != (Widget) NULL &&
			    sb != PinnedMenu && sb != UnpinnedMenu)
			{
				Arg notify[1];

				XtSetArg(notify[0], XtNpreview,
					ambw->menubutton.preview_widget);
				XtSetValues(the_default, notify, 1);
			}
					/* Clear window to get visual	*/

			_OlClearWidget(w, (Bool)True);
		}
	}
} /* END OF Preview() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/* There are no public routines */

#undef IS_KEY
#undef IS_BUTTON
#undef IS_SELECTBTN
#undef IS_MENUBTN
#undef IS_OTHERBTN
