#ifndef NOIDENT
#ident	"@(#)popupwindo:PopupWindo.c	1.62"
#endif

/*
 *************************************************************************
 *
 * Description:	Open Look Popup Window widget.
 *		
 *******************************file*header*******************************
 */

#include <stdio.h>
#include <ctype.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/VendorI.h>
#include <Xol/PopupWindP.h>

#include <Xol/ControlArP.h>
#include <Xol/PopupMenu.h>
#include <Xol/FButtons.h>
#include <Xol/FooterPane.h>

#define ClassName PopupWindowShell
#include <Xol/NameDefs.h>

#ifdef DEBUG
#define FPRINTF(x) fprintf x
#else
#define FPRINTF(x)
#endif

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

               /* dynamically linked private procedures */
static void    (*_olmPWAddButtons) OL_ARGS((PopupWindowShellWidget));



					/* class procedures		*/
static Boolean	ActivateWidget OL_ARGS((Widget,OlVirtualName,XtPointer));
static void	WMMsgHandler OL_ARGS((Widget, OlDefine, OlWMProtocolVerify *));
static void	ClassInitialize OL_NO_ARGS();
static void	Initialize OL_ARGS((Widget, Widget, ArgList, Cardinal *));
static void 	Realize OL_ARGS((Widget, XtValueMask *,XSetWindowAttributes *));
static void 	Destroy OL_ARGS((Widget));

					/* action procedures		*/

					/* public procedures		*/


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************
 */


/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */


static XtResource resources[] =
{
	{ XtNupperControlArea, XtCUpperControlArea, XtRPointer, sizeof(Widget), 
		XtOffset(PopupWindowShellWidget, popupwindow.upperControlArea), 
		XtRPointer, NULL },

	{ XtNlowerControlArea, XtCLowerControlArea, XtRPointer, sizeof(Widget),
		XtOffset(PopupWindowShellWidget, popupwindow.lowerControlArea), 
		XtRPointer, NULL },

	{ XtNfooterPanel, XtCFooterPanel, XtRPointer, sizeof(Widget),
		XtOffset(PopupWindowShellWidget, popupwindow.footerPanel), 
		XtRPointer, NULL },

	{ XtNapply, XtCCallback, XtRCallback, sizeof(XtPointer),
		XtOffset(PopupWindowShellWidget, popupwindow.apply),
		XtRCallback, (XtPointer)NULL},

	{ XtNsetDefaults, XtCCallback, XtRCallback, sizeof(XtPointer),
		XtOffset(PopupWindowShellWidget, popupwindow.setDefaults),
		XtRCallback, (XtPointer)NULL},

	{ XtNreset, XtCCallback, XtRCallback, sizeof(XtPointer),
		XtOffset(PopupWindowShellWidget, popupwindow.reset),
		XtRCallback, (XtPointer)NULL},

	{ XtNresetFactory, XtCCallback, XtRCallback, sizeof(XtPointer),
		XtOffset(PopupWindowShellWidget, popupwindow.resetFactory),
		XtRCallback, (XtPointer)NULL},

	{ XtNverify, XtCCallback, XtRCallback, sizeof(XtPointer),
		XtOffset(PopupWindowShellWidget, popupwindow.verify), 
		XtRCallback, NULL },

	{ XtNcancel, XtCCallback, XtRCallback, sizeof(XtPointer),
		XtOffset(PopupWindowShellWidget, popupwindow.cancel), 
		XtRCallback, NULL },

	{ XtNpropertyChange, XtCPropertyChange, XtRBoolean, sizeof(Boolean),
		XtOffset(PopupWindowShellWidget, popupwindow.propchange), 
		XtRBoolean, False },
};
#undef OFFSET

/*
 *************************************************************************
 *
 * Define Class Extension Resource List
 *
 *************************************************************************
 */
#define OFFSET(field)	XtOffsetOf(OlVendorPartExtensionRec, field)

static XtResource
ext_resources[] = {
        { XtNmenuButton, XtCMenuButton, XtRBoolean, sizeof(Boolean),
          OFFSET(menu_button), XtRImmediate, (XtPointer)False },

        { XtNpushpin, XtCPushpin, XtROlDefine, sizeof(OlDefine),
          OFFSET(pushpin), XtRImmediate, (XtPointer)OL_OUT },

        { XtNresizeCorners, XtCResizeCorners, XtRBoolean, sizeof(Boolean),
          OFFSET(resize_corners), XtRImmediate, (XtPointer)False },

        { XtNmenuType, XtCMenuType, XtROlDefine, sizeof(OlDefine),
          OFFSET(menu_type), XtRImmediate, (XtPointer)OL_MENU_LIMITED },

        { XtNwinType, XtCWinType, XtROlDefine, sizeof(OlDefine),
          OFFSET(win_type), XtRImmediate, (XtPointer)OL_WT_CMD },
};

/*
 *************************************************************************
 *
 * Define Class Extension Records
 *
 *************************************************************************
 */
static OlVendorClassExtensionRec
vendor_extension_rec = {
    {
        NULL,                                   /* next_extension       */
        NULLQUARK,                              /* record_type          */
        OlVendorClassExtensionVersion,          /* version              */
        sizeof(OlVendorClassExtensionRec)       /* record_size          */
    },  /* End of OlClassExtension header       */
        ext_resources,                          /* resources            */
        XtNumber(ext_resources),                /* num_resources        */
        NULL,                                   /* private              */
        NULL,                                   /* set_default          */
	NULL,					/* get_default 		*/
	NULL,					/* destroy		*/
        NULL,                                   /* initialize           */
        NULL,                                   /* set_values           */
        NULL,                                   /* get_values           */
        XtInheritTraversalHandler,              /* traversal_handler    */
        XtInheritHighlightHandler,  		/* highlight_handler    */
        ActivateWidget, 			/* activate function    */
        NULL,              			/* event_procs          */
        0,              			/* num_event_procs      */
        NULL,					/* part_list            */
	{ NULL, 0 },				/* dyn_data		*/
	NULL,					/* transparent_proc	*/
	WMMsgHandler,				/* wm_proc		*/
	FALSE,					/* override_callback	*/
}, *vendor_extension = &vendor_extension_rec;

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

PopupWindowShellClassRec popupWindowShellClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass) &transientShellClassRec,
    /* class_name         */    "PopupWindowShell",
    /* size               */    sizeof(PopupWindowShellRec),
    /* Class Initializer  */    ClassInitialize,
    /* class_part_initialize*/	NULL,
    /* Class init'ed ?    */	FALSE,
    /* initialize         */    (XtInitProc) Initialize,
    /* initialize_hook    */	NULL,
    /* realize            */    Realize,
    /* actions            */    NULL,
    /* num_actions        */    0,
    /* resources          */    resources,
    /* resource_count     */	XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion    */    FALSE,
    /* compress_exposure  */    TRUE,
    /* compress_enterleave*/	FALSE,
    /* visible_interest   */    FALSE,
    /* destroy            */    NULL,
    /* resize             */    XtInheritResize,
    /* expose             */    NULL,
    /* set_values         */    NULL,
    /* set_values_hook    */	NULL,			
    /* set_values_almost  */	XtInheritSetValuesAlmost,  
    /* get_values_hook    */	NULL,			
    /* accept_focus       */    XtInheritAcceptFocus,
    /* intrinsics version */	XtVersion,
    /* callback offsets   */    NULL,
    /* tm_table           */    XtInheritTranslations,
    /* query_geometry     */    NULL,
  },{
/* composite_class fields */
    /* geometry_manager   */    XtInheritGeometryManager,
    /* change_managed     */    XtInheritChangeManaged,
    /* insert_child	  */	XtInheritInsertChild,
    /* delete_child	  */	XtInheritDeleteChild,
    /* extension	  */    NULL
  },{
/* shell_class fields 	*/
    /* grumble		*/	0
  },{
/* wm_shell_class fields*/
    /* mumble		*/	0
  },{
/* vendor_shell_class fields*/
    /* tumble           */	(XtPointer)&vendor_extension_rec
  },{
/* tranisent_shell_class fields*/
    /* stumble		*/	0
  },{
/* popup_shell_class fields*/
    /* fumble		*/	0
  }
};


/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass popupWindowShellWidgetClass = (WidgetClass)&popupWindowShellClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 *
 * _OlPWBringDownPopup(w, blastpushpin)
 *
 * Given a widget, find its shell and try the shell's verify callbacks.
 * If the verify succeeds, call the dismiss callbacks.  Then check the 
 * shell's pushpin state, and pop down the shell if pin is out.  
 *
 * If blastpushpin is true, pull the pin out, call dismiss callbacks,
 * and pop down the shell without verifying.
 *
 ****************************procedure*header*****************************
 */

extern void
_OlPWBringDownPopup OLARGLIST((w,blastpushpin))
	OLARG(Widget,	w)
	OLGRA(Boolean,	blastpushpin)
{
	Widget 			shellw;
	Boolean			verifyok = True;
	OlVendorPartExtension	part;

	shellw = _OlGetShellOfWidget(w);

	if (!shellw) {
		OlVaDisplayWarningMsg(	XtDisplay(w),
					OleNfilePopupWindo,
					OleTmsg1,
					OleCOlToolkitWarning,
					(String) OleMfilePopupWindo_msg1,
					XtName(w),
					OlWidgetToClassName(w));
		return;
	}

	if (blastpushpin == True) {
		Arg args[1];
		XtSetArg(args[0], XtNpushpin, OL_OUT);
		XtSetValues(shellw, args, 1);
		XtPopdown(shellw);
		return;
	}

	if (((PopupWindowShellWidget) shellw)->popupwindow.verify) {
		XtCallCallbacks(shellw, XtNverify, &verifyok);
		if (verifyok == False)
			return;
	}

 	part = _OlGetVendorPartExtension(shellw);

	switch (part->pushpin) {
		case OL_IN:
		break;
		case OL_OUT:
		default:
			XtPopdown(shellw);
			break;
	}
}

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 *
 *  Initialize
 *
 *	Add an event handler to intercept client messages.
 * 
 ****************************procedure*header*****************************
 */

/* ARGSUSED */
static void 
Initialize OLARGLIST((request_w, new_w, args, pnum_args))
	OLARG( Widget,		request_w)
	OLARG( Widget,		new_w)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	pnum_args)
{

	PopupWindowShellWidget	new = (PopupWindowShellWidget)new_w;
	Widget		mastercontrol;
	MaskArg		maskargs[20];
	Arg		localargs[20];
	ArgList		mergedargs;
	Cardinal	i,j,mergedcnt;
	static XtCallbackRec cb[] = {
		{ (XtCallbackProc) _OlPWBringDownPopup, (XtPointer) False },
		{ NULL, NULL}
	};

	new->popupwindow.menu = NULL;

/*
 **  Make footer panel
 */

	new->popupwindow.footerPanel = 
	    XtCreateManagedWidget("panel",
				  footerPanelWidgetClass, 
				  new_w, NULL, 0);
	if (new_w->core.background_pixmap != XtUnspecifiedPixmap)
		XtVaSetValues(new->popupwindow.footerPanel,
			XtNbackgroundPixmap, ParentRelative, 0);
/*
**  Make main control area
*/

	i = 0;
	XtSetArg(localargs[i], XtNlayoutType, OL_FIXEDCOLS);	i++;
	XtSetArg(localargs[i], XtNcenter, True);		i++;
	XtSetArg(localargs[i], XtNsameSize, OL_NONE);		i++;
	XtSetArg(localargs[i], XtNmeasure, 1);			i++;
	XtSetArg(localargs[i], XtNhPad, 0);			i++;
	XtSetArg(localargs[i], XtNvPad, 0);			i++;
	XtSetArg(localargs[i], XtNhSpace, 0);			i++;
	XtSetArg(localargs[i], XtNvSpace, 0);			i++;
	XtSetArg(localargs[i], XtNborderWidth, 0);		i++;
	XtSetArg(localargs[i], XtNshadowThickness, 0);		i++;
	if (new_w->core.background_pixmap != XtUnspecifiedPixmap)  {
		XtSetArg(localargs[i], XtNbackgroundPixmap, ParentRelative);	i++;
	}

	mastercontrol = XtCreateManagedWidget("control",
					      controlAreaWidgetClass, 
					      new->popupwindow.footerPanel,
					      localargs,
					      i);
/*
**  Make child control areas.  Inherit application resources from parent's
**  arg list, if specified.  Note that some default values are different
**  for the two control areas.
*/
	i = 0;
	_OlSetMaskArg(	maskargs[i], XtNalignCaptions, 
			True, OL_DEFAULT_PAIR);				i++;
	_OlSetMaskArg(maskargs[i], XtNcenter, FALSE, OL_DEFAULT_PAIR);	i++;
	_OlSetMaskArg(maskargs[i], XtNhPad, 0, OL_SOURCE_PAIR);		i++;
	_OlSetMaskArg(maskargs[i], XtNhSpace, 0, OL_SOURCE_PAIR);	i++;
	_OlSetMaskArg(	maskargs[i], XtNlayoutType, 
			OL_FIXEDCOLS, OL_DEFAULT_PAIR);			i++;
	_OlSetMaskArg(maskargs[i], XtNmeasure, 1, OL_DEFAULT_PAIR);	i++;
	_OlSetMaskArg(maskargs[i], XtNsameSize, 0, OL_SOURCE_PAIR);	i++;
	_OlSetMaskArg(maskargs[i], XtNvPad, 0, OL_SOURCE_PAIR);		i++;
	_OlSetMaskArg(maskargs[i], XtNvSpace, 0, OL_SOURCE_PAIR);	i++;
	_OlSetMaskArg(	maskargs[i], XtNborderWidth, 
			0, OL_OVERRIDE_PAIR);				i++;
	_OlSetMaskArg(	maskargs[i], XtNshadowThickness, 
			0, OL_OVERRIDE_PAIR);				i++;
	if (new_w->core.background_pixmap != XtUnspecifiedPixmap)  {
		_OlSetMaskArg(	maskargs[i], XtNbackgroundPixmap, 
			ParentRelative, OL_DEFAULT_PAIR);		i++;
	}

	_OlComposeArgList(	args, *pnum_args, maskargs, 
				i, &mergedargs, &mergedcnt);

	new->popupwindow.upperControlArea = 
	    XtCreateManagedWidget("upper",
				  controlAreaWidgetClass, 
				  mastercontrol,
				  mergedargs,
				  mergedcnt);

	XtFree((char *)mergedargs);

	i = 0;
	_OlSetMaskArg(	maskargs[i], XtNalignCaptions, 
			0, OL_SOURCE_PAIR);				i++;
	_OlSetMaskArg(maskargs[i], XtNcenter, 0, OL_SOURCE_PAIR);	i++;
	_OlSetMaskArg(maskargs[i], XtNhPad, 0, OL_SOURCE_PAIR);		i++;
	_OlSetMaskArg(maskargs[i], XtNhSpace, 0, OL_SOURCE_PAIR);	i++;
	_OlSetMaskArg(	maskargs[i], XtNlayoutType, 
			OL_FIXEDROWS, OL_DEFAULT_PAIR);			i++;
	_OlSetMaskArg(maskargs[i], XtNmeasure, 1, OL_DEFAULT_PAIR);	i++;
	_OlSetMaskArg(maskargs[i], XtNsameSize, 0, OL_SOURCE_PAIR);	i++;
	_OlSetMaskArg(maskargs[i], XtNvPad, 0, OL_SOURCE_PAIR);		i++;
	_OlSetMaskArg(maskargs[i], XtNvSpace, 0, OL_SOURCE_PAIR);	i++;
	_OlSetMaskArg(maskargs[i], XtNborderWidth, 0, OL_OVERRIDE_PAIR);i++;
	_OlSetMaskArg(maskargs[i], XtNshadowThickness, 0, OL_OVERRIDE_PAIR);i++;
	if (new_w->core.background_pixmap != XtUnspecifiedPixmap)  {
		_OlSetMaskArg(	maskargs[i], XtNbackgroundPixmap, 
			ParentRelative, OL_DEFAULT_PAIR);		i++;
	}
	_OlSetMaskArg(maskargs[i], XtNpostSelect, cb, OL_OVERRIDE_PAIR);i++;

	_OlComposeArgList(args, *pnum_args, maskargs, 
				i, &mergedargs, &mergedcnt);

	new->popupwindow.lowerControlArea = 
	    XtCreateManagedWidget("lower",
				  controlAreaWidgetClass, 
				  mastercontrol,
				  mergedargs,
				  mergedcnt);

	XtFree((char *)mergedargs);

	(*_olmPWAddButtons)(new);
} /* Initialize */

/*
 *************************************************************************
 * ActivateWidget - this procedure allows external forces to activate this
 * widget indirectly.
 ****************************procedure*header*****************************
*/
/* ARGSUSED */
static Boolean
ActivateWidget OLARGLIST((w, type, call_data))
	OLARG( Widget,		w)
	OLARG( OlVirtualName,	type)
	OLGRA( XtPointer,	call_data)
{
	Boolean			consumed = False;
	OlVendorPartExtension	part;

	part = _OlGetVendorPartExtension(w);
	switch (type)
	{
		case OL_DEFAULTACTION:
			consumed = TRUE;
			if ((w = _OlGetDefault(w)) != (Widget)NULL)
				(void) OlActivateWidget(w, OL_SELECTKEY,
						(XtPointer)OL_NO_ITEM);
			break;
		case OL_CANCEL:
			consumed = TRUE;
			if (part->pushpin == OL_IN)
				_OlSetPinState(w, OL_OUT);
			XtPopdown(w);
			break;
		case OL_TOGGLEPUSHPIN:
			consumed = TRUE;
			if (part->pushpin == OL_OUT)
				_OlSetPinState(w, OL_IN);
			else
			{
				_OlSetPinState(w, OL_OUT);
				XtPopdown(w);
			}
			break;
		default:
			break;
	}
	return(consumed);
} /* END OF ActivateWidget() */

static void
WMMsgHandler OLARGLIST((w, action, wmpv))
	OLARG(Widget,			w)
	OLARG(OlDefine,			action)
	OLGRA(OlWMProtocolVerify *,	wmpv)
{
	if (wmpv->msgtype == OL_WM_DELETE_WINDOW) {
		switch(action) {
		case OL_DEFAULTACTION:
		case OL_DISMISS:
			_OlPWBringDownPopup(w,True);
			break;
		case OL_QUIT:
			XtUnmapWidget(w);
			exit(0);
			break;
		case OL_DESTROY:
			XtDestroyWidget(w);
			break;
		}
	}
}

static void
ClassInitialize OL_NO_ARGS()
{
	vendor_extension->header.record_type = OlXrmVendorClassExtension;
	OLRESOLVESTART
	OLRESOLVEEND(PWAddButtons,	_olmPWAddButtons)
}

static void 
Realize OLARGLIST((ww, valueMask, attributes))
	OLARG( Widget,			ww)
	OLARG( XtValueMask *,		valueMask)
	OLGRA( XSetWindowAttributes *,	attributes)
{
	PopupWindowShellWidget	w = (PopupWindowShellWidget)ww;
	Window		win;
	Display *	dpy;
	XWMHints *	hintp = &w->wm.wm_hints;

	w->core.border_width = 0;

	(*popupWindowShellClassRec.core_class.superclass->core_class.realize)
		(ww, valueMask, attributes);

	if(w->core.parent) {
		Widget	ptr;

		win = XtWindow(w);
		dpy = XtDisplay(w);
		for (	ptr = w->core.parent; 
			ptr->core.parent;
			ptr = ptr->core.parent) {
			}
		hintp->window_group = XtWindow(ptr);
		hintp->flags |=  WindowGroupHint;
		XSetWMHints(dpy, win, hintp);
	}
}
