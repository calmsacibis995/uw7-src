/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#include <Xol/OlMinStr.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)manager:Manager.c	1.53"
#endif

/*************************************************************************
 *
 * Description:	This file along with Manager.h and ManagerP.h implements
 *		the OPEN LOOK ManagerWidget.  This widget interacts with
 *		subclasses of the PrimitiveWidgetClass to provide
 *		keyboard traversal/acceleration.
 *
 *******************************file*header******************************/

						/* #includes go here	*/

#include <malloc.h>
#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <Xol/OpenLookP.h>
#include <Xol/ManagerP.h>
#include <Xol/LayoutExtP.h>
#include <Xol/HandlesExP.h>

#define ClassName Manager
#include <Xol/NameDefs.h>

/*************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables*****************************/

#define BYTE_OFFSET	XtOffsetOf(ManagerRec, manager.dyn_flags)
static _OlDynResource dyn_res[] = {
 { { XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultBackground }, BYTE_OFFSET, OL_B_MANAGER_BG, NULL },
 { { XtNinputFocusColor, XtCInputFocusColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, "Red" }, BYTE_OFFSET, OL_B_MANAGER_FOCUSCOLOR, NULL },
 { { XtNborderColor, XtCBorderColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, "Black" }, BYTE_OFFSET, OL_B_MANAGER_BORDERCOLOR, NULL },
};
#undef BYTE_OFFSET

/*************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 **************************forward*declarations**************************/

					/* private procedures		*/

					/* class procedures		*/
static Boolean	AcceptFocus OL_ARGS((Widget, Time *));
static void	ClassInitialize OL_NO_ARGS();
static void	ClassPartInitialize OL_ARGS((WidgetClass));
static void	Destroy OL_ARGS((Widget));
static void	GetValuesHook OL_ARGS((Widget, ArgList, Cardinal *));
static void	Initialize OL_ARGS((Widget, Widget, ArgList, Cardinal *));
static void	Redisplay OL_ARGS((Widget, XEvent *, Region));
static Boolean	SetValues OL_ARGS((Widget, Widget, Widget,
				   ArgList, Cardinal *));
static void	TransparentProc OL_ARGS((Widget, Pixel, Pixmap));

					/* action procedures		*/

					/* public procedures		*/

/*************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************/

/* See generic translations */
static XtActionsRec
ActionTable[] = {
	{ "OlAction",	OlAction }
};

/*************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources****************************/


#define OFFSET(field)	XtOffsetOf(ManagerRec, manager.field)
static XtResource
resources[] = {
  { XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
    XtOffsetOf(ManagerRec, core.border_width), XtRImmediate, (XtPointer)0
  },
  { XtNconsumeEvent, XtCCallback, XtRCallback, sizeof(XtCallbackList),
    OFFSET(consume_event), XtRCallback, (XtPointer)NULL
  },
  { XtNinputFocusColor, XtCInputFocusColor, XtRPixel, sizeof(Pixel),
    OFFSET(input_focus_color), XtRString, "Red"
  },
  { XtNreferenceName, XtCReferenceName, XtRString, sizeof(String),
    OFFSET(reference_name), XtRString, (XtPointer)NULL
  },
  { XtNreferenceWidget, XtCReferenceWidget, XtRWidget, sizeof(Widget),
    OFFSET(reference_widget), XtRWidget, (XtPointer)NULL
  },
  { XtNtraversalOn, XtCTraversalOn, XtRBoolean, sizeof(Boolean),
    OFFSET(traversal_on), XtRImmediate, (XtPointer)True
  },
  { XtNuserData, XtCUserData, XtRPointer, sizeof(XtPointer),
    OFFSET(user_data), XtRPointer, (XtPointer)NULL
  },
  { XtNshadowType, XtCShadowType, XtROlDefine, sizeof(OlDefine),
    OFFSET(shadow_type), XtRImmediate, (XtPointer)OL_SHADOW_OUT,
  },
  { XtNshadowThickness, XtCShadowThickness, XtRDimension, sizeof(Dimension),
    OFFSET(shadow_thickness), XtRImmediate, (XtPointer)0
  },
};
#undef OFFSET


/*************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record**************************/

/*
 * Class record structure:
 *
 *	(I)	XtInherit'd field
 *	(D)	Chained downward (super-class to sub-class)
 *	(U)	Chained upward (sub-class to super-class)
 */

static
CompositeClassExtensionRec	compositeClassExtension = {
/* next_extension       */ (XtPointer)0,
/* record_type          */            NULLQUARK,
/* version              */            XtCompositeExtensionVersion,
/* record_size          */            sizeof(CompositeClassExtensionRec),
/* accepts_objects      */            True
};

static
ConstraintClassExtensionRec	constraintClassExtension = {
/* next_extension       */ (XtPointer)0,
/* record_type          */            NULLQUARK,
/* version              */            XtConstraintExtensionVersion,
/* record_size          */            sizeof(ConstraintClassExtensionRec),
/* get_values_hook   (D)*/            _OlDefaultConstraintGetValuesHook
};

ManagerClassRec		managerClassRec = {
	/*
	 * Core class:
	 */
	{
/* superclass           */ (WidgetClass)         &constraintClassRec,
/* class_name           */                       "Manager",
/* widget_size          */                       sizeof(ManagerRec),
/* class_initialize     */                       ClassInitialize,
/* class_part_init   (D)*/                       ClassPartInitialize,
/* class_inited         */                       False,
/* initialize        (D)*/                       Initialize,
/* initialize_hook   (D)*/ (XtArgsProc)          0, /* Obsolete */
/* realize           (I)*/                       _OlDefaultRealize,
/* actions           (U)*/                       ActionTable,
/* num_actions          */                       XtNumber(ActionTable),
/* resources         (D)*/                       resources,
/* num_resources        */                       XtNumber(resources),
/* xrm_class            */                       NULLQUARK,
/* compress_motion      */                       True,
/* compress_exposure    */                       XtExposeCompressSeries,
/* compress_enterleave  */                       True,
/* visible_interest     */                       False,
/* destroy           (U)*/                       Destroy,
/* resize            (I)*/                       _OlDefaultResize,
/* expose            (I)*/                       Redisplay,
/* set_values        (D)*/                       SetValues,
/* set_values_hook   (D)*/ (XtArgsFunc)          0, /* Obsolete */
/* set_values_almost (I)*/                       XtInheritSetValuesAlmost,
/* get_values_hook   (D)*/ (XtArgsProc)          GetValuesHook,
/* accept_focus      (I)*/                       AcceptFocus,
/* version              */                       XtVersion,
/* callback_private     */ (XtPointer)           0,
/* tm_table          (I)*/ (String)              0, /* ClassInitialize */
/* query_geometry    (I)*/                       _OlDefaultQueryGeometry,
/* display_acceler   (I)*/                       XtInheritDisplayAccelerator,
/* extension            */ (XtPointer)           0
	},
	/*
	 * Composite class
	 */
	{
/* geometry_manager  (I)*/                       _OlDefaultGeometryManager,
/* change_managed    (I)*/                       _OlDefaultChangeManaged,
/* insert_child      (I)*/                       _OlDefaultInsertChild,
/* delete_child      (I)*/                       _OlDefaultDeleteChild,
/* extension            */ (XtPointer)           &compositeClassExtension
	},
	/*
	 * Constraint class:
	 */
	{
/* resources            */ (XtResourceList)      0,
/* num_resources        */ (Cardinal)            0,
/* constraint_size      */ (Cardinal)            0,
/* initialize        (D)*/                       _OlDefaultConstraintInitialize,
/* destroy           (U)*/ (XtWidgetProc)        0,
/* set_values        (D)*/                       _OlDefaultConstraintSetValues,
/* extension            */ (XtPointer)           &constraintClassExtension
	},
	/*
	 * Manager class:
	 */
	{
/* highlight_handler (I)*/ (OlHighlightProc)     0,
/* focus_on_select      */			 True,
#if	defined(OL_VERSION) && OL_VERSION < 5   
/* reserved             */ (XtPointer)           0,
#endif                                             
/* traversal_handler (I)*/ (OlTraversalFunc)     0,
/* activate          (I)*/ (OlActivateFunc)      0,
/* event_procs          */ (OlEventHandlerList)  0, /* ClassInitialize */
/* num_event_procs      */ (Cardinal)            0, /* ClassInitialize */
/* register_focus    (I)*/                       0,
#if	defined(OL_VERSION) && OL_VERSION < 5
/* reserved             */ (XtPointer)           0,
#endif
/* version              */                       OlVersion,
/* extension            */ (XtPointer)           0,
/* dyn_data             */ { dyn_res, XtNumber(dyn_res) },
/* transparent_proc  (I)*/                       TransparentProc,
	}
};


/*************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition************************/

WidgetClass managerWidgetClass = (WidgetClass)&managerClassRec;


/*************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures***************************/

/*************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures****************************/

/***************************function*header*******************************
 * AcceptFocus - If I can accept focus, look thru list of children for
 *		one that can accept focus.
 */
static Boolean
AcceptFocus OLARGLIST((w, time))
    OLARG( Widget,	w)		/* me */
    OLGRA( Time *,	time)
{
    /* If I can accept focus, find a willing child to take it */
    if (OlCanAcceptFocus(w, *time))
    {
	WidgetList	child = ((CompositeWidget)w)->composite.children;
	Cardinal	i = ((CompositeWidget)w)->composite.num_children;

    	for ( ; i > 0; i--, child++)
    	{
	    if (XtIsManaged(*child) &&
	      (XtCallAcceptFocus(*child, time) == True))
		return (True);
    	}
    }

   return (False);

} /* END OF AcceptFocus() */


/*************************************************************************
 * ClassInitialize - Sets the generic action table, translation table and
 * event handlers.
 ***************************function*header*******************************/
static void
ClassInitialize OL_NO_ARGS()
{
	 ManagerWidgetClass wc = (ManagerWidgetClass)managerWidgetClass;

	 wc->core_class.tm_table	= (String)_OlGenericTranslationTable;
	 wc->manager_class.event_procs = (OlEventHandlerList)
					_OlGenericEventHandlerList;
	 wc->manager_class.num_event_procs = (Cardinal)
					_OlGenericEventHandlerListSize;

	 _OlAddOlDefineType("shadowOut",	OL_SHADOW_OUT);
	 _OlAddOlDefineType("shadowIn",		OL_SHADOW_IN);
	 _OlAddOlDefineType("shadowEtchedOut",	OL_SHADOW_ETCHED_OUT);
	 _OlAddOlDefineType("shadowEtchedIn",	OL_SHADOW_ETCHED_IN);
} /* END OF ClassInitialize() */

/*************************************************************************
 * ClassPartInitialize - Provides for inheritance of the class procedures
 ***************************function*header*******************************/

static void
ClassPartInitialize OLARGLIST((wc))
	OLGRA( WidgetClass,		wc)
{
	OlClassExtension	ext;


	/*
	 * Warn if the subclass' version isn't up to date with this code.
	 */
	if (MANAGER_C(wc).version != OlVersion && MANAGER_C(wc).version < 5000) {
		OlVaDisplayWarningMsg((Display *)NULL,
			OleNinternal, OleTbadVersion,
			OleCOlToolkitWarning, OleMinternal_badVersion,
			CORE_C(wc).class_name,
			MANAGER_C(wc).version, OlVersion);
	}

	/*
	 * By default all subclasses accept objects. If a subclass
	 * chooses not to, it should define the CompositeExtension
	 * with accepts_objects False (or change the accepts_objects
	 * field or delete the extension in class_part_initialize).
	 */
	ext = _OlGetClassExtension(
		(OlClassExtension)&COMPOSITE_C(wc).extension,
		NULLQUARK, XtCompositeExtensionVersion
	);
	if (!ext) {
		ext = (OlClassExtension)
			XtMalloc(compositeClassExtension.record_size);
		memcpy (
			(XtPointer)ext, &compositeClassExtension,
			compositeClassExtension.record_size
		);
		ext->next_extension = COMPOSITE_C(wc).extension;
		COMPOSITE_C(wc).extension = (XtPointer)ext;
	}

#define INHERIT(WC,F,INH) \
	if (MANAGER_C(WC).F == (INH))					\
		MANAGER_C(WC).F = MANAGER_C(SUPER_C(WC)).F

	INHERIT (wc, highlight_handler, XtInheritHighlightHandler);
	INHERIT (wc, traversal_handler, XtInheritTraversalHandler);
	INHERIT (wc, activate,          XtInheritActivateFunc);
	INHERIT (wc, register_focus,    XtInheritRegisterFocus);
	INHERIT (wc, transparent_proc,  XtInheritTransparentProc);
#undef	INHERIT

	if (wc == managerWidgetClass)
		return;

	if (MANAGER_C(wc).dyn_data.num_resources == 0) {
		/* give the superclass's resource list to this class */
		MANAGER_C(wc).dyn_data = MANAGER_C(SUPER_C(wc)).dyn_data;
	}
	else {
		/* merge the two lists */
		_OlMergeDynResources(&(MANAGER_C(wc).dyn_data),
				     &(MANAGER_C(SUPER_C(wc)).dyn_data));
	}
} /* END OF ClassPartInitialize() */

/*************************************************************************
 * Destroy - Remove `w' from the traversal list
 ***************************function*header*******************************/

static void
Destroy OLARGLIST((w))
	OLGRA( Widget,	w)
{
	ManagerWidget	mw = (ManagerWidget)w;

	_OlDestroyKeyboardHooks(w);
	if (mw->manager.reference_name)
		XtFree(mw->manager.reference_name);

	if (mw->manager.attrs != NULL)
	{
		OlgDestroyAttrs(mw->manager.attrs);
	}
} /* END OF Destroy */

/*************************************************************************
 * GetValuesHook - check for XtNreferenceWidget and XtNreferenceName
 *		and return info according to the traversal list
 */
static void
GetValuesHook OLARGLIST((w, args, num_args))
	OLARG( Widget,		w)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	_OlGetRefNameOrWidget(w, args, num_args);
} /* END OF GetValuesHook */

/*************************************************************************
 * Initialize - Initialize non-resource class part members, etc.
 ***************************function*header*******************************/

static void
Initialize OLARGLIST((request, new, args, num_args))
	OLARG( Widget,		request)
	OLARG( Widget,		new)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	ManagerWidget mw = (ManagerWidget)new;

	mw->manager.has_focus = False;	/* doesn't have focus */

	if (mw->manager.reference_name)
		mw->manager.reference_name = XtNewString(
						mw->manager.reference_name);

	_OlInitDynResources((Widget)mw, &(((ManagerWidgetClass)
			(mw->core.widget_class))-> manager_class.dyn_data));
	_OlCheckDynResources((Widget)mw, &(((ManagerWidgetClass)
			(mw->core.widget_class))-> manager_class.dyn_data),
			args, *num_args);

	if (mw->manager.shadow_thickness == 0)
	{
		mw->manager.attrs = NULL;
	}
	else
	{
		if ((mw->manager.shadow_type == OL_SHADOW_ETCHED_IN ||
		     mw->manager.shadow_type == OL_SHADOW_ETCHED_OUT) &&
		    (mw->manager.shadow_thickness % 2 != 0))
		{
				/* round it to nearest even nos */
			mw->manager.shadow_thickness++;
		}
		mw->manager.attrs = OlgCreateAttrs(
			XtScreen((Widget)mw),
			(Pixel)0,		/* foreground, ignored	*/
			(OlgBG *)&(mw->core.background_pixel),
			False,			/* bg is a pixel	*/
			OL_DEFAULT_POINT_SIZE
		);

		/* no need to guard "border_width"...			*/
	}
} /* END OF Initialize */

/*************************************************************************
 * Redisplay -
 ***************************function*header*******************************/
static void
Redisplay OLARGLIST((w, xevent, region))
	OLARG( Widget,		w)
	OLARG( XEvent *,	xevent)
	OLGRA( Region,		region)
{
	ManagerWidget		mw = (ManagerWidget)w;

	if (mw->manager.shadow_thickness != 0)
	{
		OlgDrawBorderShadow(
			XtScreen(w), XtWindow(w),
			mw->manager.attrs,
			mw->manager.shadow_type,
			mw->manager.shadow_thickness,
			(Position)0, (Position)0,
			mw->core.width, mw->core.height
		);
	}
} /* END OF Redisplay */

static Boolean
SetValues OLARGLIST((current, request, new, args, num_args))
	OLARG( Widget, 		current)
	OLARG( Widget, 		request)
	OLARG( Widget, 		new)
	OLARG( ArgList, 	args)
	OLGRA( Cardinal *,	num_args)
{
	ManagerWidget	current_mw = (ManagerWidget) current;
	ManagerPart *	curPart = &current_mw->manager;
	ManagerWidget	new_mw = (ManagerWidget) new;
	ManagerPart *	newPart = &new_mw->manager;

	Boolean		redisplay = False;
	Boolean		reset_attrs = False;

#define CHANGED(field)	(newPart->field != curPart->field)
#define CORE_CHANGED(field)	(new_mw->core.field != current_mw->core.field)
#define RESET_ATTRS		if (newPart->attrs != NULL)		\
					OlgDestroyAttrs(newPart->attrs);\
				newPart->attrs = OlgCreateAttrs(	\
					XtScreen(new), (Pixel)0,	\
					(OlgBG *)&(new_mw->core.background_pixel),\
					False, OL_DEFAULT_POINT_SIZE);	\
				reset_attrs = True;			\
				redisplay = True

	if (CHANGED(shadow_thickness))
	{
		if (newPart->shadow_thickness != 0)
		{
			if ((newPart->shadow_type == OL_SHADOW_ETCHED_IN ||
			     newPart->shadow_type == OL_SHADOW_ETCHED_OUT) &&
			    (newPart->shadow_thickness % 2 != 0))
			{
					/* round it to nearest even nos */
				newPart->shadow_thickness++;
			}
			RESET_ATTRS;
		}
		else
		{
			if (newPart->attrs) {
				OlgDestroyAttrs(newPart->attrs);
				newPart->attrs = NULL;
			}
			redisplay = True;
		}
	}
	if (CORE_CHANGED(background_pixel) && reset_attrs == False)
	{
		RESET_ATTRS;
	}
	if (CHANGED(shadow_type) && redisplay == False)
	{
		redisplay = True;
	}

	/* no need to guard "border_width"				*/
#undef CORE_CHANGED
#undef RESET_ATTRS

		/* since not every manager is in the traversal list. 	 */
		/* setting up referenceName or referenceWidget for those */
		/* managers will cause warning(s). There will be no	 */
		/* checking in toolkit.					 */
	if (CHANGED(reference_name))	/* this has higher preference	 */
	{
		if (newPart->reference_name)
		{
			newPart->reference_name = XtNewString(
						newPart->reference_name);
		}
		if (curPart->reference_name != NULL)
		{
			XtFree(curPart->reference_name);
			curPart->reference_name = NULL;
		}
	}
	else if (CHANGED(reference_widget))
	{
			/* no need to keep this info */
		if (curPart->reference_name != NULL)
		{
			XtFree(curPart->reference_name);
			curPart->reference_name = NULL;
		}
	}

	if (!XtIsSensitive(new) && (OlGetCurrentFocusWidget(new) == new)) {
		/*
		 * When it becomes insensitive, need to move focus elsewhere,
		 * if this widget currently has focus.
		 */
		OlMoveFocus(new, OL_IMMEDIATE, CurrentTime);
	}

	_OlCheckDynResources(new,
		&(((ManagerWidgetClass)(new->core.widget_class))->
			manager_class.dyn_data), args, *num_args);

		/* handle background transparency */
	if ((_OlDynResProcessing == FALSE) &&
	    (new->core.background_pixel != current->core.background_pixel) ||
	    (new->core.background_pixmap != current->core.background_pixmap))
	{
		_OlDefaultTransparentProc(new, new->core.background_pixel,
					new->core.background_pixmap);
	}

	return (redisplay);
} /* END OF SetValues */

static void
TransparentProc OLARGLIST((w, pixel, pixmap))
	OLARG(Widget,	w)
	OLARG(Pixel,	pixel)
	OLGRA(Pixmap,	pixmap)
{
	ManagerWidget mw = (ManagerWidget)w;

	if (mw->manager.attrs) {
		OlgDestroyAttrs(mw->manager.attrs);
	}
	mw->manager.attrs = OlgCreateAttrs(
			XtScreen(w),
			(Pixel)0,		/* foreground, ignored	*/
			(OlgBG *)&pixel,	/* new bg pixel		*/
			False,			/* bg is a pixel	*/
			OL_DEFAULT_POINT_SIZE
	);

	/* do the default also */
	_OlDefaultTransparentProc(w, pixel, pixmap);
} /* end of TransparentProc */

/*************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures***************************/

/*************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures***************************/
