#ifndef	NOIDENT
#ident	"@(#)buttonstack:MenuButton.c	1.47"
#endif

/*
 *************************************************************************
 *
 * Description:
 *		This file contains the source code for the MenuButton
 *	widget.  Since some of the MenuButton code deals directly with
 *	the Menu widget, some MenuButton routines actually exist in
 *	the Menu source code file.
 *
 ******************************file*header********************************
 */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookI.h>
#define _OL_GADGETS_ARE_NOT_WIDGETS
#include <Xol/MenuButtoP.h>
#include <Xol/MenuP.h>
#include <Xol/ControlAre.h>
#include <Xol/Pushpin.h>
#include <Xol/Font.h>

#define ClassName MenuButton
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

static void	ActivateDefault OL_ARGS((Widget));
static void	MenuButtonCB();		/* gadget's consumed callback	*/
static void	MenuButtonEH();		/* event handler for gadgets	*/
static void	PopupSubmenu();		/* pops up submenu		*/
static void	PositionSubmenu OL_ARGS((Widget, Widget, Cardinal, OlDefine,
			Position *, Position *, Position *, Position *));
	/* Returns an inverted button to normal state			*/
static void	RevertButton OL_ARGS((MenuShellWidget, Boolean));

					/* class procedures		*/

static Boolean	ActivateWidget OL_ARGS((Widget, OlVirtualName, XtPointer));
static void	ClassInitialize();	/* initialize Class Fields for
					 * each subclass		*/
static void	GetValuesHook();	/* Return Menu data		*/
static void	Initialize();		/* Class initialization		*/
static void	InitializeHook();	/* Submenu creation		*/
static void	Redisplay();		/* Handles instance refreshing	*/

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
static void	ParseBtnUp OL_ARGS((Widget, XEvent *, int));
static void	ParseMenuBtnMotion OL_ARGS((Widget, XEvent *));
static void	ParseMenuBtnDown OL_ARGS((Widget, XEvent *));
static void	Preview OL_ARGS((Widget, XEvent *));
static void	SetDefault OL_ARGS((Widget, OlVirtualName));
static Boolean	SetValues();		/* Monitor instance state chgs	*/

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
	{ KeyPress,		HandleKey	},
	{ LeaveNotify,		HandleCrossing	},
	{ MotionNotify,		HandleMotion	}
};

#define FIND_MENUBUTTON_PART(w) (_OlIsGadget((Widget)(w)) ?	\
		&((MenuButtonGadget)(w))->menubutton :\
		&((MenuButtonWidget)(w))->menubutton)

#define INSTANCE_RESOURCES(base) \
    { XtNbuttonType, XtCButtonType, XtROlDefine, sizeof(OlDefine),\
	XtOffset(base, button.button_type), XtRImmediate,\
	(XtPointer) ((OlDefine) OL_BUTTONSTACK) },\
\
    { XtNdefault, XtCDefault, XtRBoolean, sizeof(Boolean),\
	XtOffset(base, button.is_default), XtRImmediate, (XtPointer) False },\
\
    { XtNpreview, XtCPreview, XtRWidget, sizeof(Widget),\
	XtOffset(base, button.preview), XtRWidget, (XtPointer) NULL },\
\
    { XtNshellBehavior, XtCShellBehavior, XtRInt, sizeof(int),\
	XtOffset(base, button.shell_behavior), XtRImmediate,\
	(XtPointer) BaseWindow}

/* End of INSTANCE_RESOURCE macro definition */
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
	INSTANCE_RESOURCES(MenuButtonWidget)
};

static XtResource
gadget_resources[] = {
	INSTANCE_RESOURCES(MenuButtonGadget)
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

MenuButtonClassRec
menuButtonClassRec = {
  {
	(WidgetClass) &buttonClassRec,		/* superclass		*/
	"MenuButton",				/* class_name		*/
	sizeof(MenuButtonRec),			/* widget_size		*/
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
	NULL,					/* destroy		*/
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
	XtInheritHighlightHandler,		/* highlight_handler	*/
	NULL,					/* traversal_handler	*/
	NULL,					/* register_func	*/
	ActivateWidget,				/* activate		*/
	handlers,				/* event_procs		*/
	XtNumber(handlers),			/* num_event_procs	*/
	OlVersion,				/* version		*/
	NULL,					/* extension		*/
	{ NULL, 0 },				/* dyn_data		*/
	XtInheritTransparentProc,		/* transparent_proc	*/
  },	/* End of Primitive field initializations */
  {
	NULL,					/* field not used	*/
  },	/* End of ButtonClass field initializations */
  {
	NULL,					/* field not used	*/
  }	/* End of MenuButtonClass field initializations */
}; 

MenuButtonGadgetClassRec
menuButtonGadgetClassRec = {
  {
        (WidgetClass) &(buttonGadgetClassRec),  /* superclass           */
        "MenuButton",                          /* class_name           */
        sizeof(MenuButtonGadgetRec),           /* widget_size          */
        NULL,                                   /* class_initialize     */
        NULL,                                   /* class_part_initialize*/
        FALSE,                                  /* class_inited         */
        Initialize,                             /* initialize           */
        InitializeHook,                         /* initialize_hook      */
        (XtProc)XtInheritRealize,               /* realize              */
        NULL,	                                /* actions              */
        0,					/* num_actions          */
        gadget_resources,                       /* resources            */
        XtNumber(gadget_resources),             /* num_resources        */
        NULLQUARK,                              /* xrm_class            */
        TRUE,                                   /* compress_motion      */
        TRUE,                                   /* compress_exposure    */
        TRUE,                                   /* compress_enterleave  */
        FALSE,                                  /* visible_interest     */
        NULL,                                   /* destroy              */
        XtInheritResize,                        /* resize               */
        Redisplay,	                        /* expose               */
        NULL,                                   /* set_values           */
        NULL,                                   /* set_values_hook      */
        XtInheritSetValuesAlmost,               /* set_values_almost    */
        GetValuesHook,                          /* get_values_hook      */
	(XtProc)XtInheritAcceptFocus,		/* accept_focus		*/
        XtVersion,                              /* version              */
        NULL,                                   /* callback_private     */
        NULL,                                   /* tm_table             */
        XtInheritQueryGeometry                  /* query_geometry       */
  },    /* End of RectObjClass field initializations */
  {
        False,					/* focus_on_select	*/
	XtInheritHighlightHandler,		/* highlight_handler	*/
	NULL,					/* traversal_handler	*/
	NULL,					/* register_focus	*/
	ActivateWidget,				/* activate		*/
	NULL,					/* event_procs		*/
	NULL,					/* num_event_procs	*/
	OlVersion,				/* version		*/
	NULL					/* extension		*/
  },    /* End of EventObjClass field initializations */
  {
        NULL,                                   /* field not used       */
  },    /* End of ButtonClass field initializations */
  {
        NULL,                                   /* field not used       */
  }     /* End of MenuButtonClass field initializations */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass menuButtonWidgetClass = (WidgetClass) &menuButtonClassRec;
WidgetClass menuButtonGadgetClass = (WidgetClass) &menuButtonGadgetClassRec;
WidgetClass buttonStackWidgetClass = (WidgetClass) &menuButtonClassRec;
WidgetClass buttonStackGadgetClass = (WidgetClass) &menuButtonGadgetClassRec;


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
	Widget			w;
{
	MenuButtonWidget	mbw = (MenuButtonWidget)w;
	Widget			the_default;
	Boolean			made_it_busy = False;	
        MenuButtonPart *	mbp = FIND_MENUBUTTON_PART(mbw);
        ButtonPart *		bp = find_button_part(mbw);
	Widget			submenu = mbp->submenu;

	if (submenu == (Widget) NULL)
	{
		return;
	}
	else
	{
		ShellBehavior sb =
			((MenuShellWidget) submenu)->menu.shell_behavior;

		if ((bp->busy == True && mbp->previewing_default == True) ||
		    sb == PinnedMenu || sb == UnpinnedMenu)
		{
			_OlBeepDisplay((Widget) mbw, 1);
			return;
		}
	}
				/* Make the Button Busy to show the
				 * user that something is indeed going
				 * on.					*/

	if (bp->busy == False)
	{
		made_it_busy = True;
		bp->busy = True;
		if (XtIsRealized((Widget)mbw) == True)
		{
			(* (XtClass((Widget) mbw)->core_class.expose))
				((Widget) mbw, (XEvent *) NULL, (Region) NULL);
		}
	}

	the_default = _OlGetDefault(submenu);

	if (the_default != (Widget) NULL)
	{
		if (XtIsSubclass(the_default, menuButtonWidgetClass) == True ||
		    XtIsSubclass(the_default, menuButtonGadgetClass) == True)
		{
			ActivateDefault(the_default);
		}
		else if (OlActivateWidget(the_default, OL_SELECTKEY,
					  (XtPointer)OL_NO_ITEM) == False)
		{
			_OlBeepDisplay((Widget) mbw, 1);
		}
	}
	else
	{
		_OlBeepDisplay((Widget) mbw, 1);
	}

				/* If we made this button busy,
				 * return it to its normal state	*/

	if (made_it_busy == True)
	{
		bp->busy = False;
		if (XtIsRealized((Widget)mbw) == True)
		{
			(* (XtClass((Widget) mbw)->core_class.expose))
				((Widget) mbw, (XEvent *) NULL, (Region) NULL);
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
#if OlNeedFunctionPrototypes
ActivateWidget(
	Widget		w,
	OlVirtualName	type,
	XtPointer	data)
#else /* OlNeedFunctionPrototypes */
ActivateWidget(w, type, data)
	Widget		w;
	OlVirtualName	type;
	XtPointer	data;
#endif /* OlNeedFunctionPrototypes */
{
	Boolean 	ret_val = FALSE;

	switch (type)
	{
		case OL_SELECTKEY:
			ret_val = TRUE;
			if (_OlSelectDoesPreview(w) == True)
			{
				HandleSelect (w, IS_SELECTBTN);
				break;
			}
/* FALLTHROUGH */
/* fall from OL_SELECTKEY to OL_MENUKEY */
		case OL_MENUKEY:
			{
				MenuButtonWidget mbw = (MenuButtonWidget) w;
        			MenuButtonPart * mbp = FIND_MENUBUTTON_PART
							(mbw);
        			ButtonPart *	 bp = find_button_part(mbw);
				MenuShellWidget  menu = (MenuShellWidget)
							 mbp->submenu;

				ret_val = TRUE;

				PopupSubmenu(mbw, _OlIsGadget((Widget)mbw), mbp,
					bp, menu,
					(int) 0, (int) 0,
					(int) 0, (int) 0,
					IS_KEY);
				break;
			}
		case OL_MENUDEFAULTKEY:
			ret_val = TRUE;
			SetDefault(w, type);
			break;
	}
	return (ret_val);
}

/*
 *************************************************************************
 *
 *  MenuButtonCB - the XtNconsumeEvent for gadget
 *
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
MenuButtonCB(w, client_data, call_data)
	Widget		w;
	XtPointer	client_data;
	XtPointer	call_data;
{
	HandleKey(w, (OlVirtualEvent) call_data);
} /* END OF MenuButtonCB() */

/*
 *************************************************************************
 *
 *  MenuButtonEH - This event handler is used to interpret events for the
 *	MenuButton gadget.
 *
 ****************************procedure*header*****************************
 */
static void
MenuButtonEH(w, client_data, event)
	Widget		w;
	XtPointer	client_data;
	XEvent		*event;
{
	OlVirtualEventRec	ve;

	OlLookupInputEvent(w, event, &ve, OL_DEFAULT_IE);

	switch (event->type)
	{
		case ButtonPress:
		case ButtonRelease:
			HandleButton(w, &ve);
			break;
		case MotionNotify:
			HandleMotion(w, &ve);
			break;
		case LeaveNotify:
			LeaveParse(w, event);
			break;
		case Expose:
		case GraphicsExpose:
			if (XtIsRealized(w) == True)
			{
				(*(XtClass(w)->core_class.expose))
					(w, event, client_data);
			}
			break;
		default:

			OlVaDisplayWarningMsg(	XtDisplay(w),
						OleNfileMenuButton,
						OleTmsg1,
						OleCOlToolkitWarning,
						OleMfileMenuButton_msg1);
			break;
	}
} /* END OF MenuButtonEH() */

/*
 *************************************************************************
 * PopupSubmenu - this routine does the basic steps to popup a submenu
 ****************************procedure*header*****************************
 */
static void
PopupSubmenu(mbw, is_gadget, mbp, bp, menu, root_x, root_y, window_x, window_y,
		flag)
	MenuButtonWidget	mbw;
	Boolean			is_gadget;
	MenuButtonPart *	mbp;
	ButtonPart *		bp;
	MenuShellWidget		menu;
	int			root_x;
	int			root_y;
	int			window_x;
	int			window_y;
	OlDefine		flag;		/* IS_KEY or IS_BUTTON */
{
	OlDefine		state;
						/* Highlight the button	*/

	bp->set = True;
	if (XtIsRealized((Widget)mbw) == True)
	{
		(* (XtClass((Widget)mbw)->core_class.expose))
			((Widget) mbw, (XEvent *) NULL, (Region) NULL);
	}

				/* Cache the menubutton's position
				 * relative to the RootWindow.  If
				 * the menubutton is a gadget, then
				 * the position is relative to the
				 * parent, so add the position of the
				 * gadget.			*/

	if (flag == IS_BUTTON)
	{
		mbp->root_x = (Position) (root_x - window_x);
		mbp->root_y = (Position) (root_y - window_y);

		if (is_gadget == True)
		{
			mbp->root_x += mbw->core.x;
			mbp->root_y += mbw->core.y;
		}
		state = OL_PRESS_DRAG_MENU;
	}
	else	/* IS_KEY */
	{
		Position	rx, ry;

		XtTranslateCoords ((Widget) mbw,
				(Position) 0, (Position) 0,
				&rx, &ry);

		mbp->root_x = root_x = (Position) rx;
		mbp->root_y = root_y = (Position) ry;

		if (bp->menumark == OL_RIGHT)
		{
			Position ten_pts = (Position) OlScreenPointToPixel(
						OL_HORIZONTAL, 10,
						XtScreenOfObject ((Widget)mbw));
			if ((Position) mbw->core.width > (Position) ten_pts)
			{
				root_x += (Position) (mbw->core.width-ten_pts);
			}
			root_y += (Position) (mbw->core.height/2);
		}

		state = OL_STAYUP_MENU;
	}

	OlMenuPopup((Widget) menu, (Widget)mbw, OL_NO_ITEM, state, TRUE,
			root_x, root_y, PositionSubmenu);

	_OlAddGrab((Widget)mbw, False, False);
} /* END OF PopupSubmenu() */

/*
 *************************************************************************
 * PositionSubmenu - This routine has the task of positioning the
 * MenuButton's submenu alongside the parent MenuButton.  The call
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
	MenuButtonWidget mbw;		/* MenuButton Widget id		*/
	Boolean		warp_pointer = False;
	CompositeWidget	cw = (CompositeWidget)menu->menu.pane;
	Position	x;	/* menu pos. relative to RootWindow	*/
	Position	y;	/* menu position relative to RootWindow	*/
	Position	abs_x;	/* Absolute x coor. wrt to root		*/
	Position 	abs_y;	/* Absolute y coor. wrt to root		*/
	Position	horiz_buf;	/* distance from right menu	*/
					/* edge to right screen edge	*/
	Position	vert_buf;	/* distance from bottom menu	*/
					/* edge to bottom screen edge	*/
	Widget		first_widget;	/* 1st widget on the submenu's pane */
	MenuButtonPart *mbp;
	ButtonPart *	bp;
	Screen *	screen;


				/* Check for Parent's existence		*/

	if ((mbw = (MenuButtonWidget) XtParent(w)) == NULL)

		OlVaDisplayErrorMsg(	XtDisplay(w),
					OleNfileMenuButton,
					OleTmsg2,
					OleCOlToolkitError,
					OleMfileMenuButton_msg2);

	if (XtIsSubclass((Widget)mbw, menuButtonWidgetClass) == False)
	{
		/* since parent is not a menuButtonWidget as expected, must */
		/* be a menuButtonGadget and parent is the *button's* parent. */
		/* Menu must pop up from menuButtonGadget stored in */
		/* menu.emanate */

		mbw = (MenuButtonWidget) menu->menu.emanate;

		if (XtIsSubclass((Widget)mbw, menuButtonGadgetClass) == False)

			OlVaDisplayErrorMsg(	XtDisplay(w),
						OleNfileMenuButton,
						OleTmsg3,
						OleCOlToolkitError,
						OleMfileMenuButton_msg3);
	}

	mbp	= FIND_MENUBUTTON_PART(mbw);
	bp	= find_button_part(mbw);
	screen	= XtScreen(w);		/* get the menu's screen	*/

				/* Determine the first widget on the
				 * submenu's pane.  This does not
				 * include the pushpin.			*/

	if ((first_widget = _OlGetDefault((Widget)menu)) == (Widget)NULL ||
	     XtIsSubclass(first_widget, pushpinWidgetClass) == True)
	{
		Cardinal	i;
		WidgetList	kids;

		for (first_widget = (Widget)NULL,
		     kids = cw->composite.children,
		     i = cw->composite.num_children; i; ++kids, --i)
		{
			if ((*kids)->core.managed == True &&
			    (_OlIsGadget(*kids) == True ||
			      (*kids)->core.mapped_when_managed == True))
			{
				first_widget = *kids;
				break;
			}
		}
	}

			    /* Get the coordinates of the menubutton
			     * which are relative to root.  These were
			     * cached on the button press or motion that
			     * caused this routine to be called.	*/

	abs_x = mbp->root_x;
	abs_y = mbp->root_y;

			    /* Determine the x and y coordinates of the
			     * submenu.  These coordinates are relative to
			     * the root window.				*/

	if (bp->menumark == OL_RIGHT)
	{
			/* Line the top of the menu with the bottom
			 * line of the MenuButton and move it
			 * 10 points to the right.  But, we must prevent
			 * the x coordinate from going beyond the
			 * width of the button.				*/
			
		x = *px + (Position)OlScreenPointToPixel(OL_HORIZONTAL, 10,
								screen);
		
		if (x > (abs_x + (Position)mbw->core.width))
		{
		    x = abs_x + (Position)mbw->core.width;
		}

			/* locate the position oval's top line for
			 * this menu button.				*/

		y = abs_y;

			/* Position the y coordinate so that the top
			 * of the first item in the submenu lines up
			 * (horizontally with the top of this button	*/

		if (first_widget != (Widget)NULL)
		{
			register Widget self;

			for (self = first_widget;
			     self != (Widget)NULL && self != (Widget) menu;
			     self = self->core.parent)
			{
				y -= self->core.y + (Position)
						self->core.border_width;
			}
		}
	}
	else
	{
				/* Menu is a pull-down, so center the
				 * menu under the MenuButton		*/

		x  = abs_x + (Position) (mbw->core.width / (Dimension)2);
		x -= (Position)
		    ((int)(menu->core.width - menu->menu.shadow_right)/(int)2);
		y = abs_y + (Position)mbw->core.height;
	}

				/* Now, make sure that the menu does not
				 * go off of the screen.  I can do a
				 * logical shift instead of a multiple
				 * here since the value is non-zero	*/

	horiz_buf = (Position)WidthOfScreen(screen) -
				(x + (Position)_OlWidgetWidth(w));
	vert_buf = (Position)HeightOfScreen(screen) -
				(y + (Position)_OlWidgetHeight(w));

			/* Check to see if we have gone off the bottom	*/

	if (vert_buf < (Position)0)
	{
		y += vert_buf;

				/* if the menu moved under the pointer,
				 * move the pointer			*/

		if (y < *py)
			warp_pointer = True;
	}

			/* See if the menu is off the top of the
			 * screen.  This only applies for menubuttons
			 * that are in menus				*/

	if (y < (Position)0)
		y = (Position)0;

			/* If the horizontal buffer is less than zero,
			 * we've gone off the right edge; so, move the
			 * root position to the left and move the pointer
			 * to the left by necessary amount.		*/

	if (horiz_buf < (Position)0)
	{
		x += horiz_buf;

		if (y < *py)
		    warp_pointer = True;
	}

			/* Check to see if the submenu is off the left
			 * edge of the screen.  This applies only to
			 * MenuButtons in control areas			*/

	x = (x < (Position)0 ? (Position)0 : x);
		
			/* See if the pointer needs to be warped.  If
			 * does, calculate the new position.
			 * When warping the pointer, the vertical
			 * position never changes.  The horizontal
			 * horizontal position moves to be four
			 * points to the left of the pane items.	*/

	if (warp_pointer == True)
	{
		register Widget self = first_widget;
		Position	four_points;

		four_points = (Position)OlScreenPointToPixel(OL_HORIZONTAL, 4,
								  screen);

			/* Calculate the position of the widget w.r.t
			 * the menu.					*/

		for (*px = x; self != (Widget)NULL && self != (Widget) menu;
			  self = self->core.parent)
		{
			 *px += self->core.x + (Position)self->core.border_width;
		}

			/* Move the pointer 4 points to the left	*/

		*px -= four_points;
	}

	*mx = x;
	*my = y;

} /* END OF PositionSubmenu() */

/*
 *************************************************************************
 * RevertButton - This routine returns an inverted MenuButton to its
 * normal state.  This procedure is called when a menu is posted in
 * the "stay-up" mode.  It's also called when the menu is pinned or
 * when the menu unposts.
 ****************************procedure*header*****************************
 */
static void
RevertButton OLARGLIST((menu, for_stay_up_mode))
    OLARG( MenuShellWidget,	menu)	/* Menu that is child of button	*/
    OLGRA( Boolean,		for_stay_up_mode) /* Are we going from */
						  /* PDR to StayUp mode? */
{
    Widget		parent = menu->core.parent;
    MenuButtonWidget	mbw;
    ButtonPart *	bp;


    if (parent == (Widget) NULL)
	parent = menu->menu.parent_cache;

    if (parent == (Widget) NULL || menu->menu.application_menu == True)
	return;

    if ( !XtIsSubclass(parent, menuButtonWidgetClass) ) {
	/* since parent is not a menuButtonWidget as expected, must */
	/* be a menuButtonGadget and parent is the *button's* parent. */
	/* Must get menuButtonGadget stored in menu.emanate */
	parent = menu->menu.emanate;
	if ( !XtIsSubclass(parent, menuButtonGadgetClass) )

		OlVaDisplayErrorMsg(	XtDisplay(menu),
					OleNfileMenuButton,
					OleTmsg3,
					OleCOlToolkitError,
					OleMfileMenuButton_msg3);
    }

    mbw = (MenuButtonWidget) parent;
    bp = find_button_part(mbw);

    if (for_stay_up_mode == True || bp->busy == True || bp->set == True)
    {
	bp->busy= for_stay_up_mode;
	bp->set	= False;
	if (XtIsRealized(parent) == True)
	{
		(*(XtClass(parent)->core_class.expose))
		    (parent, (XEvent *)NULL, (Region)NULL);
	}
    }
}				/* END OF RevertButton() */

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
	MenuButtonWidget mbw = (MenuButtonWidget) w;
	ArgList	          new_list = (ArgList)NULL;
	static MaskArg    mask_list[] = {
		{ XtNmenu, NULL /* see below */, OL_COPY_MASK_VALUE },
		{ NULL,	(XtArgVal)sizeof(Widget),OL_COPY_SIZE	},
		{ XtNmenuPane,		NULL,	OL_SOURCE_PAIR	},
		{ XtNpushpin,		NULL,	OL_SOURCE_PAIR	},
		{ XtNpushpinDefault,	NULL,	OL_SOURCE_PAIR	},
		{ XtNtitle,		NULL,	OL_SOURCE_PAIR	}
	};
        MenuButtonPart *mbp;


	if (*num_args == (Cardinal)0)
		return;

        mbp = FIND_MENUBUTTON_PART(mbw);

				/* before parsing the mask arg list,
				 * put in the id of the submenu.	*/

	_OlSetMaskArg(mask_list[0], XtNmenu, mbp->submenu,
			OL_COPY_MASK_VALUE);

	_OlComposeArgList(args, *num_args, mask_list, XtNumber(mask_list),
			 &new_list, &count);

			/* Call the menu with the get values if
			 * necessary					*/

	if (count > (Cardinal)0) {
		if (mbp->submenu)
			XtGetValues((Widget) mbp->submenu,
					new_list, count);

		XtFree((char *)new_list);
	}

} /* END OF GetValuesHook() */

/*
 *************************************************************************
 * Initialize - Initializes the MenuButton Instance.  Any conflicts 
 * between the "request" and "new" widgets should be resolved here.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize(request, new, args, num_args)
	Widget request;			/* What user wants		*/
	Widget new;			/* What user gets, so far....	*/
	ArgList		args;
	Cardinal *	num_args;
{
	MenuButtonWidget nmbw = (MenuButtonWidget) new;
        MenuButtonPart *nmbp;

        nmbp = FIND_MENUBUTTON_PART(nmbw);

	if (_OlIsGadget((Widget)nmbw)) {
	    _OlAddEventHandler(new,
			       (ExposureMask |
				ButtonPressMask | ButtonReleaseMask |
				ButtonMotionMask |
				LeaveWindowMask),
			       FALSE, MenuButtonEH, NULL);
	    XtAddCallback(new, XtNconsumeEvent, MenuButtonCB, NULL);
	}

	nmbp->previewing_default = False;

	nmbp->preview_widget = (Widget) NULL;

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
	MenuButtonWidget mbw = (MenuButtonWidget) w;
	char *		  menu_name = "menu";		/* default name	*/
	ArgList           comp_args = (ArgList) NULL;	/* composed Args*/
	Cardinal          count;
	static MaskArg    mask_args[] = {
		{ XtNtitle,	NULL /* below */, OL_DEFAULT_PAIR },
		{ XtNsensitive, NULL /* below */, OL_OVERRIDE_PAIR },
		{ XtNmenuName,	NULL /* below */, OL_COPY_SOURCE_VALUE },
		{ NULL,	(XtArgVal)sizeof(String),OL_COPY_SIZE	},
		{ XtNpushpin,		NULL,	OL_SOURCE_PAIR },
		{ XtNpushpinDefault,	NULL,	OL_SOURCE_PAIR },
		{ XtNpaneName,		NULL,	OL_SOURCE_PAIR },
		{ XtNmenuAugment, (XtArgVal) False, OL_OVERRIDE_PAIR },
		{ XtNrevertButton, (XtArgVal)RevertButton,
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
        MenuButtonPart *mbp;

        mbp = FIND_MENUBUTTON_PART(mbw);

			/* Set the default submenu title		*/

	_OlSetMaskArg(mask_args[0], XtNtitle,
		XrmNameToString(mbw->core.xrm_name), OL_DEFAULT_PAIR);

			/* The menubutton is insensitive, create
			 * an insensitive menu.  We don't look at
			 * ancestor_sensitive for this.			*/

	_OlSetMaskArg(mask_args[1], XtNsensitive, mbw->core.sensitive,
			OL_OVERRIDE_PAIR);

			/* Extract the menu's name from the args	*/

	_OlSetMaskArg(mask_args[2], XtNmenuName, &menu_name,
			OL_COPY_SOURCE_VALUE);

	

			/* Parse the arglist for the menu		*/

	_OlComposeArgList(args, *num_args, mask_args, XtNumber(mask_args),
			 &comp_args, &count);


			/* Now Create the Submenu			*/

	if (_OlIsGadget(w)) {
	    MenuShellWidget submenu;

	    submenu = (MenuShellWidget)XtCreatePopupShell(menu_name,
					      menuShellWidgetClass,
					      XtParent(w),
					      comp_args, count);
	    mbp->submenu = (Widget)submenu;

	    /* for gadgets, menu must be told where to emanate from */
	    submenu->menu.emanate = w;

	    /* The initialize of the menu assumes that if the parent
	     * is not a menuButton that the menu is an application
	     * menu.  Since we play the reparent trick, we have to
	     * set this field to False here.
	     */
	    submenu->menu.application_menu = False;
	} else {
	    mbp->submenu = XtCreatePopupShell(menu_name,
					      menuShellWidgetClass,
					      w, comp_args, count);
	}

	XtFree((char *)comp_args);

} /* END OF InitializeHook() */

/*
 *************************************************************************
 * Redisplay - this procedure redisplay's the instance object.  If the
 * menubutton is currently previewing the default, then signal it to 
 * do the redisplay (needed since an Expose event could arrive after the
 * initial preview visual was displayed), else call superclass to
 * redisplay.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Redisplay(w, xevent, region)
	Widget		w;
	XEvent *	xevent;
	Region		region;
{
	MenuButtonPart *	mbp =
			FIND_MENUBUTTON_PART(((MenuButtonWidget)w));
	ButtonPart *		bp = find_button_part(w);


	if (mbp->previewing_default == True && bp->busy == False)
	{
		Widget		the_default = _OlGetDefault(mbp->submenu);
		Arg		notify[1];

		XtSetArg(notify[0], XtNpreview, w);
		XtSetValues(the_default, notify, 1);
	}
	else
	{
		(* (menuButtonWidgetClass->core_class.superclass->
			core_class.expose)) (w, xevent, region);
	}
} /* END OF Redisplay() */

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
 * leaves the MenuButton's window.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
LeaveParse(w, xevent)
	register Widget	w;		/* parent widget of Menu	*/
	XEvent *	xevent;		/* Menu widget's XEvent	*/
{
	MenuButtonWidget	mbw = (MenuButtonWidget) w;
        MenuButtonPart *	mbp = FIND_MENUBUTTON_PART(mbw);
	MenuShellWidget		menu = (MenuShellWidget)mbp->submenu;

			/* Return immediately if there's no menu or if
			 * no mouse buttons are pressed.
			 */
	if (menu == (MenuShellWidget)NULL ||
	    !(xevent->xcrossing.state &
		(Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask)))
	{
		return;
	}

				/* If we were previewing, undo that and
				 * return immediately.  (Do this before
				 * looking at the Leave Event's detail	*/

	if (mbp->previewing_default == True)
	{
		ButtonPart * bp = find_button_part(mbw);

		bp->busy = (menu->shell.popped_up == True &&
			menu->menu.shell_behavior == StayUpMenu ? True : False);
		mbp->previewing_default = False;

		_OlClearWidget(w, True);

		return;
	}
				/* Ignore LeaveWindow events
				 * generated by pointer grabs		*/

	if (xevent->xcrossing.mode != NotifyNormal)
		return;

	if (menu->shell.popped_up == True &&
	    menu->menu.shell_behavior == PressDragReleaseMenu )
	{
		Boolean	in_menu;	 

		in_menu = (Boolean)
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
	else
	{
    		ButtonPart * bp = find_button_part(mbw);

		if (bp->set == True)
		{
					/* Unhighlight the button	*/
			bp->set = False;
			if (XtIsRealized((Widget)mbw) == True)
			{
				(* (XtClass((Widget)mbw)->core_class.expose))
				((Widget) mbw, (XEvent *) NULL, (Region) NULL);
			}
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
	register Widget	w;		/* menu button Widget		*/
	XEvent *	xevent;		/* Menu widget's XEvent		*/
	int		flag;		/* IS_SELECT/MENU/OTHERBTN	*/
{
	MenuButtonWidget	mbw = (MenuButtonWidget) w;
        MenuButtonPart *	mbp = FIND_MENUBUTTON_PART(mbw);
        ButtonPart *		bp = find_button_part(mbw);

	if (_OlSelectDoesPreview(w) == True)
	{
				/* If we're in this part of the code,
				 * make sure that a SELECT button got us
				 * here.				*/

		if (flag == IS_MENUBTN || /* because Menu will handle this */
		    mbp->previewing_default == False)
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
			MenuShellWidget menu = (MenuShellWidget) mbp->submenu;

			if (menu->shell.popped_up == False)
			{
						/* Popup the submenu	*/

				PopupSubmenu(mbw, _OlIsGadget((Widget)mbw), mbp,
					bp, menu, xevent->xbutton.x_root,
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
 * HandleButton - handles all button presses/releases for this widget
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
		case OL_MENUDEFAULT:
			ve->consumed = True;
			if (ve->xevent->type == ButtonPress)
				SetDefault (w, ve->virtual_name);
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
 * HandleCrossing - handles all `leaves' for this widget
 ****************************procedure*header*****************************
 */
static void
HandleCrossing(w, ve)
	Widget	w;
	OlVirtualEvent	ve;
{
	ve->consumed = True;
	LeaveParse (w, ve->xevent);
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

	if (ve->virtual_name == OL_MOVERIGHT ||
	    ve->virtual_name == OL_MOVEDOWN)
	{
		int m = (int) (find_button_part(w)->menumark);

		if ((ve->virtual_name == OL_MOVERIGHT && m == OL_RIGHT) ||
		    (ve->virtual_name == OL_MOVEDOWN && m == OL_DOWN))
		{
			ve->consumed = True;
			OlActivateWidget(w, OL_MENUKEY, (XtPointer)NULL);
		}
	}
} /* END OF HandleKey() */

/*
 *************************************************************************
 * HandleMotion - handles all `motions' for this widget
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
		case OL_MENUDEFAULT:
			ve->consumed = True;
			SetDefault (w, OL_UNKNOWN_BTN_INPUT);
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
	register Widget		w;
	int			flag;	/* IS_SELECTBTN or IS_OTHERBTN */
{
	MenuButtonWidget	mbw = (MenuButtonWidget) w;
        MenuButtonPart *	mbp = FIND_MENUBUTTON_PART(mbw);
        ButtonPart *		bp = find_button_part(mbw);

			/* We should be in here only if we've
			 * been previewing.			*/

	mbp->previewing_default = False;

			/* If the number of parameters is
			 * non-zero, it is OK to activate
			 * the default.				*/

	if (flag == IS_SELECTBTN)
	{
		ActivateDefault((Widget)mbw);
		XtCallCallbacks((Widget) mbw, XtNpostSelect,
					(XtPointer) NULL);
	}
	
		/* If the MenuButton is busy, "unbusy" it	*/

	if (bp->busy == True)
	{
		bp->busy = False;
	}

		/* Now clear the menubutton's window to get 
		 * rid of the preview or busy visual		*/

	_OlClearWidget(w, True);

	return;
}

/*
 *************************************************************************
 * ParseMenuBtnDown - this procedure handles what happens when a MENU
 * button is pressed on a MenuButton.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
ParseMenuBtnDown(w, xevent)
	register Widget  w;		/* MenuButton Widget		*/
	XEvent   	*xevent;	/* Menu widget's XEvent		*/
{
	MenuButtonWidget	mbw = (MenuButtonWidget) w;
	MenuShellWidget		menu;
	MenuShellWidget		shell;	/* MenuButton's shell widget	*/
	Boolean			is_descendent = False;
        MenuButtonPart *	mbp = FIND_MENUBUTTON_PART(mbw);
        ButtonPart *		bp = find_button_part(mbw);
	Boolean			is_gadget = _OlIsGadget((Widget)mbw);

	menu = (MenuShellWidget) mbp->submenu;

				/* Remove the active pointer grab so
				 * that other widgets can get events.	*/

	_OlUngrabPointer((is_gadget == False ? w : XtParent(w)));

	if (menu == (MenuShellWidget) NULL)
		return;

	shell = (MenuShellWidget) _OlGetShellOfWidget(w);

	if (shell != (MenuShellWidget)NULL)
		is_descendent = XtIsSubclass((Widget) shell,
						menuShellWidgetClass);

			/* If the 
			 * menubutton is a descendent of a menu and
			 * that menu is in "stay-up" mode, set the
			 * existing cascade to "press-drag-release"	*/

	if (is_descendent == (Boolean)True &&
	    shell->shell.popped_up == True &&
	    shell->menu.shell_behavior == StayUpMenu)
	{
		MenuShellWidget   next = shell->menu.next;

			/* we must insure that the submenu cascade is
			 * not more than one menu deep beyond this
			 * shell.					*/

		if (next != (MenuShellWidget)NULL)
		{

				/* Remove any subcascade off of "next"	*/

			if (next->menu.next != (MenuShellWidget)NULL)
			{
				OlMenuUnpost((Widget)next->menu.next);
			}

				/* Remove Grab created By "stay-up"
				 * mode					*/

			_OlRemoveGrab((Widget) next);

			next->shell.spring_loaded = False;
			next->shell.grab_kind     = XtGrabNone;
		}
					/* Inform pane items of new
					 * state			*/

		for(next = menu->menu.root; next != (MenuShellWidget)NULL;
		    next = next->menu.next)
		{
			_OlPropagateMenuState(next, PressDragReleaseMenu);
		}
	}

	if (menu->shell.popped_up == False)
	{

				/* Protect against degenerate state	*/

		if (is_descendent == True && shell->shell.popped_up == False)
			return;

						/* Popup the submenu	*/

		PopupSubmenu(mbw, is_gadget, mbp, bp, menu,
				xevent->xbutton.x_root, xevent->xbutton.y_root,
				xevent->xbutton.x, xevent->xbutton.y,
				IS_BUTTON);
	}
	else if (menu->menu.shell_behavior == PinnedMenu ||
		 menu->menu.shell_behavior == UnpinnedMenu) 
	{
		XRaiseWindow(XtDisplay((Widget)menu), XtWindow((Widget)menu));
	}
} /* END OF ParseMenuBtnDown() */

/*
 *************************************************************************
 * ParseMenuBtnMotion - figures out what to do when the MenuButton
 * receives a MENU button motion XEvent.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
ParseMenuBtnMotion(w, xevent)
	register Widget  w;		/* parent widget of Menu	*/
	XEvent   	*xevent;	/* Menu widget's XEvent	*/
{
	MenuButtonWidget	mbw = (MenuButtonWidget) w;
	ShellWidget		shell;
	MenuShellWidget		menu;
	XEvent			leave_event;
	Boolean			pending_leave;
	register XEvent	*	xev = &leave_event;
	register Window		win = XtWindowOfObject(w);
	register Display *	dpy = XtDisplayOfObject(w);
        MenuButtonPart *	mbp = FIND_MENUBUTTON_PART(mbw);
        ButtonPart *		bp = find_button_part(mbw);

	if ((menu = (MenuShellWidget) mbp->submenu) == (MenuShellWidget)NULL)
		return;

				/* If there is a Leave XEvent already
				 * on the queue for this MenuButton,
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
				(int) ButtonMotionMask, xev) == True) {

			if (leave_time < xev->xmotion.time) {
				XPutBackEvent(dpy, xev);
				break;
			}
		}

				/* Only put the LeaveEvent back on the
				 * queue if the submenu is up already	*/

		if (menu->shell.popped_up == True) {
			XPutBackEvent(dpy, &leave_event);
		}
		return;
	}

				/* Look at the state of the submenu to
				 * determine what to do			*/

	if (menu->shell.popped_up == False)
	{
		int	check_drag_right = 0;

			/* Protect against degenerate state	*/

		shell = (ShellWidget) _OlGetShellOfWidget(w);

		if (shell != (ShellWidget)NULL &&
		    XtIsSubclass((Widget) shell, menuShellWidgetClass) &&
		    shell->shell.popped_up == False)
		{
			return;
		}

						/* Highlight the button	*/
		if (bp->set == False)
		{
				/* Cache the pointer's x coordinate so
				 * that we can use it for 'drag-right'
				 * comparison later on.			*/

			mbp->root_x = xevent->xmotion.x;

			bp->set = True;
			_OlPopdownTrailingCascade((Widget)mbw, False);

			if (XtIsRealized((Widget)mbw) == True)
			{
				(*(XtClass((Widget)mbw)->core_class.expose))
				((Widget) mbw, (XEvent *) NULL, (Region) NULL);
			}
		}
		else
		{
			check_drag_right = 1;
		}

				/* If we're supposed to be over the menu
				 * mark and we're not, return.		*/

		switch(bp->shell_behavior)
		{
		case PressDragReleaseMenu:		/* Fall Through	*/
		case StayUpMenu:
			{
				_OlAppAttributes *	app_attrs =
						_OlGetAppAttributesRef(w);

				if (!(check_drag_right &&
				    xevent->xmotion.x >= (int)(mbp->root_x +
				    (Position)app_attrs->drag_right_distance))
					&&
				    xevent->xmotion.x <
				     (int)(mbw->core.width - 
					app_attrs->menu_mark_region))
				{
					return;
				}
			}
			break;
		default:
			break;
		}

		PopupSubmenu(mbw, _OlIsGadget((Widget)mbw), mbp, bp, menu,
				xevent->xmotion.x_root, xevent->xmotion.y_root,
				xevent->xmotion.x, xevent->xmotion.y,
				IS_BUTTON);

				/* Prevent Stay-up mode from happening	*/

		menu->menu.prevent_stayup = True;
	}
	else {			/* Menu already popped up		*/

		if (menu->menu.shell_behavior == PressDragReleaseMenu &&
		    menu->menu.prevent_stayup == False) {

			if (!_OlIsMenuMouseClick(menu, xevent->xmotion.x_root,
						xevent->xmotion.y_root)) {
				menu->menu.prevent_stayup = True;
			}
		}
	}

} /* END OF ParseMenuBtnMotion() */

/*
 *************************************************************************
 * Preview - this routine is called whenever the SELECT button is pressed
 * or if the SELECT button motion is detected.
 ****************************procedure*header*****************************
 */
static void
Preview(w, xevent)
	Widget    w;		/* MenuButton widget			*/
	XEvent   *xevent;	/* Action-causing XEvent		*/
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
		MenuButtonWidget	mbw = (MenuButtonWidget) w;
		MenuButtonPart *	mbp = FIND_MENUBUTTON_PART(mbw);
		ButtonPart *		bp = find_button_part(mbw);
		Widget			submenu = mbp->submenu;
		Boolean			is_gadget = _OlIsGadget((Widget)mbw);

		if (xevent->type == ButtonPress)
		{
			_OlUngrabPointer((is_gadget == True ? XtParent(w) : w));
		}

		if (submenu == (Widget) NULL)
		{
			return;
		}

		if (mbp->previewing_default == False)
		{
			Widget		the_default = _OlGetDefault(submenu);
			ShellBehavior	sb = ((MenuShellWidget) submenu)->
							menu.shell_behavior;
			Boolean	is_busy = False;
			Boolean is_sensitive = True;
			Boolean ancestors_sensitive = True;

			mbp->previewing_default = True;

			if (the_default != (Widget) NULL)
			{
				Arg args[3];

				XtSetArg(args[0], XtNsensitive, &is_sensitive);
				XtSetArg(args[1], XtNancestorSensitive,
					&ancestors_sensitive);
				XtSetArg(args[2], XtNbusy, &is_busy);

				XtGetValues(the_default, args, 3);

				if (XtIsSubclass(the_default,
					menuButtonWidgetClass) == True ||
				    XtIsSubclass(the_default,
					menuButtonGadgetClass) == True)
				{
					is_busy = False;
				}
			}

			if (the_default == (Widget) NULL ||
			    sb == PinnedMenu ||
			    sb == UnpinnedMenu ||
			    is_busy == True ||
			    is_sensitive == False ||
			    ancestors_sensitive == False)
			{
				bp->busy = True;
				_OlClearWidget(w, True);
			}
			else {
				Arg notify[1];

				XtSetArg(notify[0], XtNpreview, w);
				XtSetValues(the_default, notify, 1);
			}
		}
	}
} /* END OF Preview() */

/*
 *************************************************************************
 * SetDefault - this routine sets the MenuButton as the Menu's default.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
#if OlNeedFunctionPrototypes
SetDefault(
	Widget		w,
	OlVirtualName	type)
#else
SetDefault(w, type)
	Widget		w;		/* MenuButton widget	*/
	OlVirtualName	type;
#endif
{
	MenuButtonWidget	mbw = (MenuButtonWidget) w;
        ButtonPart *		bp = find_button_part(mbw);
	ShellBehavior		sb;
	static Arg		notify[] = {
		{ XtNdefault,	(XtArgVal) True }
	};

	sb = bp->shell_behavior;

	if (type == OL_MENUDEFAULT || type == OL_MENUDEFAULTKEY)
	{
		if (!_OlIsGadget((Widget)mbw))  {
			_OlUngrabPointer(w);
		}
		else  {
			_OlUngrabPointer(XtParent((Widget)mbw));
		}
	}

	if (sb != PressDragReleaseMenu &&
	    sb != StayUpMenu &&
	    sb != PinnedMenu &&
	    sb != UnpinnedMenu)
		return;

	if (bp->is_default == False) {
		XtSetValues(w, notify, 1);
		_OlSetDefault(w, True);
	}

} /* END OF SetDefault() */

/*
 *************************************************************************
 * SetValues - used to set resources associated with the instance
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues(current, request, new, args, num_args)
	Widget current;		/* The original Widget			*/
	Widget request;		/* Widget user wants			*/
	Widget new;		/* Widget user gets, so far ....	*/
	ArgList		args;
	Cardinal *	num_args;
{
	MenuButtonPart *	nmbp = FIND_MENUBUTTON_PART(new);

				/* If the instance changes sensitivity,
				 * update the submenu's sensitivity.
				 * This does not apply to
				 * ancestor_sensitive.			*/

	if (new->core.sensitive != current->core.sensitive)
	{
		if (nmbp->submenu != (Widget)NULL)
		{
			XtSetSensitive(nmbp->submenu, new->core.sensitive);
		}
	}

	return((Boolean)False);
} /* END OF SetValues() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*  There are no public procedures  */

#undef IS_KEY
#undef IS_BUTTON
#undef IS_SELECTBTN
#undef IS_MENUBTN
#undef IS_OTHERBTN
