/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <libXoli.h>
#include <OlMinStr.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)primitive:Primitive.c	1.78"
#endif

/*************************************************************************
 *
 * Description:	This file along with Primitive.h and PrimitiveP.h implements
 *		the OPEN LOOK Primitive Widget.  This widget's class
 *		includes fields/procedures common to all primitive widgets.
 *
 *******************************file*header******************************/

						/* #includes go here	*/

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <Xol/OpenLookP.h>
#include <Xol/PrimitiveP.h>
#include <Xol/Error.h>
#include <Xol/Accelerate.h>
#include <Xol/LayoutExtP.h>
#include <Xol/HandlesExP.h>

#define ClassName Primative
#include <Xol/NameDefs.h>

/*************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables*****************************/

#define NULL_CHAR	((char)NULL)
#define NULL_WIDGET	((Widget)NULL)
#define PCLASS(w, r) (((PrimitiveWidgetClass)XtClass((w)))->primitive_class.r)

#define BYTE_OFFSET	XtOffsetOf(PrimitiveRec, primitive.dyn_flags)
static _OlDynResource dyn_res[] = {
  { { XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel), 0,
      XtRString, XtDefaultBackground }, BYTE_OFFSET, OL_B_PRIMITIVE_BG, NULL },

  { { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel), 0,
      XtRString, XtDefaultForeground }, BYTE_OFFSET, OL_B_PRIMITIVE_FG, NULL },

  { { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel), 0,
      XtRString, XtDefaultForeground }, BYTE_OFFSET, OL_B_PRIMITIVE_FONTCOLOR, NULL },
  { { XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *), 0,
      XtRString, OlDefaultFont }, BYTE_OFFSET, OL_B_PRIMITIVE_FONT, NULL },

  { { XtNinputFocusColor, XtCInputFocusColor, XtRPixel, sizeof(Pixel), 0,
      XtRString, "Red" }, BYTE_OFFSET, OL_B_PRIMITIVE_FOCUSCOLOR, NULL },

  { { XtNfontGroup, XtCFontGroup, XtROlFontList, sizeof(OlFontList *), 0,
      XtRString, (XtPointer)NULL }, BYTE_OFFSET, OL_B_PRIMITIVE_FONTGROUP , NULL },
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

					/* class procedures		*/
static Boolean  AcceptFocus OL_ARGS((Widget, Time *));
static void     ClassInitialize OL_NO_ARGS();
static void     ClassPartInitialize OL_ARGS((WidgetClass));
static void     Destroy OL_ARGS((Widget));
static void     GetValuesHook OL_ARGS((Widget, ArgList, Cardinal *));
static void     Initialize OL_ARGS((Widget, Widget, ArgList, Cardinal *));
static void     Redisplay OL_ARGS((Widget, XEvent *, Region));
static Boolean  SetValues OL_ARGS((Widget, Widget, Widget,
                                   ArgList, Cardinal *));
static void	HighlightHandler OL_ARGS((Widget, OlDefine));

					/* action procedures		*/

					/* public procedures		*/


/*************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************/

/* See generic translations and actions */
static XtActionsRec ActionTable[] = {
	{ "OlAction",	OlAction }
};

/*************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources****************************/


#define OFFSET(field)	XtOffsetOf(PrimitiveRec, primitive.field)
static XtResource resources[] = {
  { XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
    XtOffset(PrimitiveWidget, core.border_width),
    XtRImmediate, (XtPointer)0
  },
  { XtNaccelerator, XtCAccelerator, XtRString, sizeof(String),
    OFFSET(accelerator), XtRString, (XtPointer) NULL
  },
  { XtNacceleratorText, XtCAcceleratorText, XtRString, sizeof(String),
    OFFSET(accelerator_text), XtRString, (XtPointer) NULL
  },
  { XtNconsumeEvent, XtCCallback, XtRCallback, sizeof(XtCallbackList),
    OFFSET(consume_event), XtRCallback, (XtPointer)NULL
  },
  { XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
    OFFSET(font), XtRString, OlDefaultFont
  },
  { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel),
    OFFSET(font_color), XtRString, XtDefaultForeground
  },
  { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
    OFFSET(foreground), XtRString, XtDefaultForeground
  },
  { XtNinputFocusColor, XtCInputFocusColor, XtRPixel, sizeof(Pixel),
    OFFSET(input_focus_color), XtRString, "Red"
  },
  { XtNmnemonic, XtCMnemonic, OlRChar, sizeof(char),
    OFFSET(mnemonic), XtRImmediate, (XtPointer) '\0'
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
  { XtNfontGroup, XtCFontGroup, XtROlFontList, sizeof(OlFontList *),
    OFFSET(font_list), XtRString, (XtPointer)NULL
  },
  { XtNshadowType, XtCShadowType, XtROlDefine, sizeof(OlDefine),
    OFFSET(shadow_type), XtRImmediate, (XtPointer)OL_SHADOW_IN,
  },
  { XtNshadowThickness, XtCShadowThickness, XtRDimension, sizeof(Dimension),
    OFFSET(shadow_thickness), XtRImmediate, (XtPointer)0
  },
  { XtNhighlightThickness,XtCHighlightThickness,XtRDimension,sizeof(Dimension),
    OFFSET(highlight_thickness), XtRString, (XtPointer)"2 points"
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

PrimitiveClassRec	primitiveClassRec = {
	/*
	 * Core class:
	 */
	{
/* superclass           */ (WidgetClass)         &widgetClassRec,
/* class_name           */                       "Primitive",
/* widget_size          */                       sizeof(PrimitiveRec),
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
	 * Primitive class:
	 */
	{
/* focus_on_select      */ 			 True,
/* highlight_handler (I)*/ (OlHighlightProc)     0, /* ClassInitialize */
/* traversal_handler (I)*/ (OlTraversalFunc)     0,
/* register_focus    (I)*/ (OlRegisterFocusFunc) 0,
/* activate          (I)*/ (OlActivateFunc)      0,
/* event_procs          */ (OlEventHandlerList)  0, /* ClassInitialize */
/* num_event_procs      */ (Cardinal)            0, /* ClassInitialize */
/* version              */                       OlVersion,
/* extension            */ (XtPointer)           0,
/* dyn_data             */ { dyn_res, XtNumber(dyn_res) },
/* transparent_proc  (I)*/ (OlTransparentProc)   0,
	}
};

/*************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition************************/

WidgetClass primitiveWidgetClass = (WidgetClass)&primitiveClassRec;


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

/*************************************************************************
 * AcceptFocus - If this widget can accept focus then it is set here
 *		 FALSE is returned if focus could not be set
 ***************************function*header*******************************/

static Boolean
AcceptFocus OLARGLIST((w, time))
    OLARG( Widget,      w)
    OLGRA( Time *,      time)
{

    if (OlCanAcceptFocus(w, *time))
    {
	return(OlSetInputFocus(w, RevertToNone, *time));
    }

    return (False);

} /* AcceptFocus() */

/*************************************************************************
 * ClassInitialize - Sets up virtual translations
 ***************************function*header*******************************/

static void
ClassInitialize OL_NO_ARGS()
{
	PrimitiveWidgetClass pwc =
				(PrimitiveWidgetClass)primitiveWidgetClass;

	pwc->core_class.tm_table	= (String)_OlGenericTranslationTable;
	pwc->primitive_class.event_procs = (OlEventHandlerList)
					_OlGenericEventHandlerList;
	pwc->primitive_class.num_event_procs = (Cardinal)
					_OlGenericEventHandlerListSize;


		/* probably should keep the following in Applic.c	*/
		/* because Manager.c also have these four lines...	*/
        _OlAddOlDefineType("shadowOut",        OL_SHADOW_OUT);
        _OlAddOlDefineType("shadowIn",         OL_SHADOW_IN);
        _OlAddOlDefineType("shadowEtchedOut",  OL_SHADOW_ETCHED_OUT);
        _OlAddOlDefineType("shadowEtchedIn",   OL_SHADOW_ETCHED_IN);

	(void) _OlResolveOlgGUISymbol();

	if (OlGetGui() == OL_MOTIF_GUI)  {
		pwc->primitive_class.highlight_handler = HighlightHandler;
	}

} /* END OF ClassInitialize() */

/*************************************************************************
 * ClassPartInitialize - Provides for inheritance of the class procedures
 ***************************function*header*******************************/

static void
ClassPartInitialize OLARGLIST((wc))
        OLGRA( WidgetClass,             wc)
{
	/*
	 * Warn if the subclass' version isn't up to date with this code.
	 */
	if (PRIMITIVE_C(wc).version != OlVersion && PRIMITIVE_C(wc).version < 5000) {
	    	OlVaDisplayWarningMsg((Display *)NULL,
    			OleNinternal, OleTbadVersion,
    			OleCOlToolkitWarning, OleMinternal_badVersion,
			CORE_C(wc).class_name,
    			PRIMITIVE_C(wc).version, OlVersion);
	}

#define INHERIT(WC,F,INH) \
	if (PRIMITIVE_C(WC).F == (INH))					\
		PRIMITIVE_C(WC).F = PRIMITIVE_C(SUPER_C(WC)).F

	INHERIT (wc, highlight_handler, XtInheritHighlightHandler);
	INHERIT (wc, register_focus,    XtInheritRegisterFocus);
	INHERIT (wc, traversal_handler, XtInheritTraversalHandler);
	INHERIT (wc, activate,          XtInheritActivateFunc);
	INHERIT (wc, transparent_proc,  XtInheritTransparentProc);

	if (wc == primitiveWidgetClass)
		return;

	if (PRIMITIVE_C(wc).dyn_data.num_resources == 0) {
		/* give the superclass's resource list to this class */
		PRIMITIVE_C(wc).dyn_data = PRIMITIVE_C(SUPER_C(wc)).dyn_data;
	}
	else {
		/* merge the two lists */
		_OlMergeDynResources(&(PRIMITIVE_C(wc).dyn_data),
				     &(PRIMITIVE_C(SUPER_C(wc)).dyn_data));
	} /* else */

	return;
} /* ClassPartInitialize() */


/*************************************************************************
 * Destroy - Remove `w' from its managing ancestor's traversal list
 *************************************************************************/

static void
Destroy OLARGLIST((w))
        OLGRA( Widget,  w)
{
	PrimitiveWidget	pw = (PrimitiveWidget)w;

	_OlDestroyKeyboardHooks(w);

	if (pw->primitive.accelerator)
		XtFree(pw->primitive.accelerator);
	if (pw->primitive.accelerator_text)
		XtFree(pw->primitive.accelerator_text);
	if (pw->primitive.reference_name)
		XtFree(pw->primitive.reference_name);
        if (pw->primitive.attrs != NULL)
                OlgDestroyAttrs(pw->primitive.attrs);

}	/* Destroy() */

/*************************************************************************
 * GetValuesHook - check for XtNreferenceWidget and XtNreferenceName
 *		and return info according to the traversal list
 ***************************function*header*******************************/
static void
GetValuesHook OLARGLIST((w, args, num_args))
        OLARG( Widget,          w)
        OLARG( ArgList,         args)
        OLGRA( Cardinal *,      num_args)
{
	_OlGetRefNameOrWidget(w, args, num_args);
}

/*************************************************************************
 * Initialize - Initializes primitive part
 ***************************function*header*******************************/

/* ARGSUSED */
static void
Initialize OLARGLIST((request, new, args, num_args))
        OLARG( Widget,          request)
        OLARG( Widget,          new)
        OLARG( ArgList,         args)
        OLGRA( Cardinal *,      num_args)
{
	PrimitiveWidget	pw	= (PrimitiveWidget)new;
	PrimitivePart *	pp	= &(pw->primitive);

	pp->has_focus = FALSE;		/* Can't have focus yet */

		/* This will only happen when applications set XtNfont	*/
		/* to NULL. Is it worthwhile the checking?		*/

		/* Anyway, put it here so other subclasses don't need	*/
		/* to do this step. There are a few exceptions, i.e.,	*/
		/* Caption, PopupMenuShell, TextField, Category, 	*/
		/* and EventObj have their own XtNfont and they are not	*/
		/* subclass of Primitive...				*/
		/* Note that macro is in OpenLookP.h			*/
	_OlLoadDefaultFont(new, pp->font);

	if (pp->mnemonic) {
		if (_OlAddMnemonic(new, (XtPointer)0, pp->mnemonic) !=
				OL_SUCCESS)
			pp->mnemonic = 0;
	}

	if (pp->accelerator) {
		if (_OlAddAccelerator(new, (XtPointer)0, pp->accelerator) !=
				OL_SUCCESS)
			pp->accelerator = (String) 0;
	}

	if (pp->accelerator) {
		pp->accelerator = XtNewString(pp->accelerator);
		if (!pp->accelerator_text)
			pp->accelerator_text=_OlMakeAcceleratorText(
							new, pp->accelerator);
		else
			pp->accelerator_text=XtNewString(pp->accelerator_text);
	} else if(pp->accelerator_text) {
		pp->accelerator_text = XtNewString(pp->accelerator_text);
	}

		/* add widget to traversal list */
	if (pp->reference_name)
		pp->reference_name = XtNewString(pp->reference_name);

	_OlUpdateTraversalWidget(new, pp->reference_name,
				 pp->reference_widget, True);

	_OlInitDynResources(new, &(((PrimitiveWidgetClass)
			(new->core.widget_class))->primitive_class.dyn_data));
	_OlCheckDynResources(new, 
		&(((PrimitiveWidgetClass)(new->core.widget_class))->
		primitive_class.dyn_data), args, *num_args);
#ifdef I18N
	/*
	 *	The Ic field must be initialized to NULL because it
	 *	is not associated with a resource
	 */
	 pp->ic = (OlIc *) NULL;
#endif

	/*  The Motif border_width is set through the highlightThickness
	    resource.  */
	if (OlGetGui() == OL_MOTIF_GUI)  {
		pw->core.border_width = pw->primitive.highlight_thickness;
		pw->core.border_pixel = pw->core.background_pixel;
	}

	pp->attrs = NULL;
	if (OlGetGui() == OL_OPENLOOK_GUI)
		pp->shadow_thickness = 0;
	else if (pp->shadow_thickness != 0 )
        {
		if ((pp->shadow_type == OL_SHADOW_ETCHED_IN ||
		     pp->shadow_type == OL_SHADOW_ETCHED_OUT)  &&
				/*  Etched borders must be even.  */
		    (pp->shadow_thickness % 2 != 0))
			pp->shadow_thickness++;

		pp->attrs = OlgCreateAttrs(
				XtScreen((Widget)pw),
				(Pixel)0,	/* foreground, ignored  */
				(OlgBG *)&(pw->core.background_pixel),
				False,		/* bg is a pixel        */
				OL_DEFAULT_POINT_SIZE);
        }
} /* Initialize() */

/*************************************************************************
 * Redisplay -  Draws a shadow border inside the widget.
 ***************************function*header*******************************/
/* ARGSUSED */
static void
Redisplay OLARGLIST((w, xevent, region))
        OLARG( Widget,          w)
        OLARG( XEvent *,        xevent)
        OLGRA( Region,          region)
{
        PrimitiveWidget           pw = (PrimitiveWidget)w;

        if (pw->primitive.shadow_thickness != 0)
        {
                OlgDrawBorderShadow(
                        XtScreen(w), XtWindow(w),
                        pw->primitive.attrs,
                        pw->primitive.shadow_type,
                        pw->primitive.shadow_thickness,
                        (Position)0, (Position)0,
                        pw->core.width, pw->core.height
                );
        }
} /* END OF Redisplay */


/*************************************************************************
 * SetValues - Sets up internal values based on changes made to external
 *	       ones
 ***************************function*header*******************************/

/* ARGSUSED */
static Boolean
SetValues OLARGLIST((current, request, new, args, num_args))
        OLARG( Widget,          current)
        OLARG( Widget,          request)
        OLARG( Widget,          new)
        OLARG( ArgList,         args)
        OLGRA( Cardinal *,      num_args)
{
	PrimitiveWidget	current_pw = (PrimitiveWidget)current;
	PrimitivePart	*curPart   = &current_pw->primitive;
	PrimitiveWidget	new_pw     = (PrimitiveWidget)new;
	PrimitivePart	*newPart   = &new_pw->primitive;

	Boolean		redisplay  = False;
	Boolean		reset_attrs  = False;


#define CHANGED(field)	(newPart->field != curPart->field)
#define CORE_CHANGED(field)     (new_pw->core.field != current_pw->core.field)
#define RESET_ATTRS             if (newPart->attrs != NULL)             \
                                        OlgDestroyAttrs(newPart->attrs);\
                                newPart->attrs = OlgCreateAttrs(        \
                                        XtScreen(new), (Pixel)0,        \
                                        (OlgBG *)&(new_pw->core.background_pixel),\
                                        False, OL_DEFAULT_POINT_SIZE);  \
                                reset_attrs = True;                     \
                                redisplay = True

		/* See Initialize() for explanation...			*/
	if (CHANGED(font) && newPart->font == NULL)
	{
		_OlLoadDefaultFont(new, newPart->font);
	}

	if (CHANGED(font) || CHANGED(font_list))
		redisplay = True;

	if (CHANGED(reference_name))	/* this has higher preference	*/
	{
		if (newPart->reference_name)
		{
			newPart->reference_name = XtNewString(
					newPart->reference_name);

			_OlUpdateTraversalWidget(new, newPart->reference_name,
					 NULL, False);
		}
		if (curPart->reference_name != NULL)
		{
			XtFree(curPart->reference_name);
			curPart->reference_name = NULL;
		}
	}
	else if (CHANGED(reference_widget))
	{
			/* no need to keep the info? */
		if (curPart->reference_name != NULL)
		{
			XtFree(curPart->reference_name);
			curPart->reference_name = NULL;
		}

		_OlUpdateTraversalWidget(new, NULL,
					 newPart->reference_widget, False);
	}

	if (CHANGED(mnemonic)) {
		if (curPart->mnemonic)
			_OlRemoveMnemonic(new, (XtPointer)0, False,
					curPart->mnemonic);
		if (newPart->mnemonic)
			if (_OlAddMnemonic(new, (XtPointer)0,
			    newPart->mnemonic) != OL_SUCCESS)
				newPart->mnemonic = 0;
		redisplay = True;
	}

	if (!XtIsSensitive(new) &&
	    (OlGetCurrentFocusWidget(new) == new)) {
		/*
		 * When it becomes insensitive, need to move focus elsewhere,
		 * if this widget currently has focus.
		 *
		 * R5Xt will not (?optimization?) dispatch an event
		 * thru "tm" action routines if a widget is
		 * insensitive. This is good, except in this case,
		 * it breaks assumptions we had in R4Xt or in prior
		 * releases... So we handle this situation here
		 * because we know we won't see FocusOut later...
		 */
		_OlSetCurrentFocusWidget(new, OL_OUT);
		OlMoveFocus(new, OL_IMMEDIATE, CurrentTime);
	}

	{
	Boolean changed_accelerator=CHANGED(accelerator);
	Boolean changed_accelerator_text=CHANGED(accelerator_text);

	if (changed_accelerator) {
		if (curPart->accelerator) {
			_OlRemoveAccelerator(new, (XtPointer)0,
				False, curPart->accelerator);
			XtFree(curPart->accelerator);
			curPart->accelerator = (String) 0;
		}

		if (newPart->accelerator) {
			if(_OlAddAccelerator(new, (XtPointer)0, 
					newPart->accelerator)==OL_SUCCESS)
				newPart->accelerator = XtNewString(
							newPart->accelerator);
		else
			newPart->accelerator = (String) 0;
		}
		redisplay = True;
	}

	if (changed_accelerator || changed_accelerator_text) {
		if (curPart->accelerator_text) {
			XtFree(curPart->accelerator_text);
			curPart->accelerator_text = (String) 0;
		}

		newPart->accelerator_text = (changed_accelerator_text ?
			XtNewString(newPart->accelerator_text) :
			_OlMakeAcceleratorText(new, newPart->accelerator));
		redisplay = True;
	}
	}

	if (OlGetGui() == OL_MOTIF_GUI)  {
		if (CHANGED(shadow_thickness))  {
			if (newPart->shadow_thickness != 0)  {
				if ((newPart->shadow_type ==
						OL_SHADOW_ETCHED_IN ||
				     newPart->shadow_type ==
						OL_SHADOW_ETCHED_OUT)    &&
				    (newPart->shadow_thickness % 2 != 0))  {
					/*  Etched borders must be even.  */
					newPart->shadow_thickness++;
				}
				RESET_ATTRS;
			}
			else  {
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

		/*  Always reject changes to the borderWidth;
			it is GUI dependent. */
		if (CORE_CHANGED(border_width))
			new->core.border_width = current->core.border_width;

		if (CHANGED(highlight_thickness))  {
			new->core.border_width = newPart->highlight_thickness;
			redisplay = True;

	/* Just in case, you want to know why I (SAMC) removed	*/
	/* XSetWindowBorderWidth...				*/

	/* Well, Core SetValues doesn't check geometry. The	*/
	/* actual checking is done after gettting back from	*/
	/* set_values of all classes...				*/

	/* Other problem is that you'll get into trouble if the	*/
	/* widget is not realized yet, i.e., X protocol error.	*/

		}

		/*  For Motif mode Primitives, force the border color to
		    follow the background color.  */
		if (newPart->has_focus)  {
			if (CHANGED(input_focus_color))
				new->core.border_pixel =
					newPart->input_focus_color;
		}
		else  {
			if (CORE_CHANGED(background_pixel))
				new->core.border_pixel =
					new->core.background_pixel;
		}
	}
	else{
			/* always force shadow_thickness to 0 in OL mode */
			new_pw->primitive.shadow_thickness = 0;
		}
		
#undef RESET_ATTRS


	/* handle dynamic resources */
	_OlCheckDynResources(new, 
		&(((PrimitiveWidgetClass)(new->core.widget_class))->
		primitive_class.dyn_data), args, *num_args);


	return (redisplay);

#undef CHANGED
#undef CORE_CHANGED
}	/* SetValues() */


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

/*********************function*header************************
 *  HighlightHandler - This function is used only for Motif-mode
 *	primitive widgets to change the color of the border to the
 *	input focus color (when focus is coming in) or back to the
 *	background color (when focus is leaving).
 */
static void
HighlightHandler OLARGLIST((w, highlight_type))
    OLARG(Widget,	w)
    OLGRA(OlDefine,	highlight_type)		/* OL_IN / OL_OUT */
{
    PrimitiveWidget pw = (PrimitiveWidget) w;

    switch (highlight_type)  {
    case OL_IN:
        XtVaSetValues(w, XtNborderColor, pw->primitive.input_focus_color, 0);
        break;
    case OL_OUT:
        XtVaSetValues(w, XtNborderColor, pw->core.background_pixel, 0);
        break;
    }

}  /* end of _OlmHighlightHandler() */
