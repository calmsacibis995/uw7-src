/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <libXoli.h>
#include <OlMinStr.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)checkbox:CheckBox.c	1.29.2.41"
#endif

/*
 *************************************************************************
 *
 * Description: CheckBox.c - CheckBox widget
 *
 ******************************file*header********************************
 */

						/* #includes go here	*/
#include <stdio.h>
#include <ctype.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <OpenLookP.h>
#include <NonexclusP.h>
#include <CheckBoxP.h>

#define ClassName CheckBox
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
static void DrawBox();
static Boolean PointInBox();
static void ResizeSelf();

					/* class procedures		*/

static Boolean	AcceptFocus OL_ARGS((Widget, Time *));
static Boolean	ActivateWidget OL_ARGS((Widget, OlVirtualName, XtPointer));
static void ClassInitialize();
static XtGeometryResult GeometryManager();
static void GetValuesHook();
static void HighlightHandler OL_ARGS((Widget, OlDefine));
static void Initialize();
static void InsertChild();
static XtGeometryResult QueryGeometry();
static void Realize();
static void Redisplay();
static void Resize();
static Boolean SetValues();

					/* action procedures		*/

static void CheckBoxHandler();
static void MotionHandler();
static Boolean PreviewState();
static Boolean SetState();

					/* public procedures		*/

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define offset(field) XtOffset(CheckBoxWidget, field)

static Boolean defTRUE = (Boolean) TRUE;
static Boolean defFALSE = (Boolean) FALSE;

#define BYTE_OFFSET	XtOffsetOf(CheckBoxRec, checkBox.dyn_flags)
static _OlDynResource dyn_res[] = {
{ { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultForeground }, BYTE_OFFSET, OL_B_CHECKBOX_FG, NULL },
{ { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultForeground }, BYTE_OFFSET, OL_B_CHECKBOX_FONTCOLOR, NULL },
};
#undef BYTE_OFFSET

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

#if 1
static char defaultTranslations[] = "\
	<FocusIn>:	OlAction() \n\
	<FocusOut>:	OlAction() \n\
	<Key>:		OlAction() \n\
	<BtnDown>:	OlAction() \n\
	<BtnUp>:	OlAction() \n\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction() \n\
	<BtnMotion>:	OlAction()";
#else
static char defaultTranslations[] = "#augment\n\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction() \n\
	<BtnMotion>:	OlAction()";
#endif

static OlEventHandlerRec
CBevents[] = {
	{ ButtonPress,	CheckBoxHandler},
	{ ButtonRelease,CheckBoxHandler},
	{ EnterNotify,	CheckBoxHandler},
	{ LeaveNotify,	CheckBoxHandler},
	{ MotionNotify, MotionHandler},
};

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource resources[] = { 

	{XtNselect, XtCCallback, XtRCallback, sizeof(XtPointer), 
	 offset(checkBox.select), XtRCallback, (XtPointer) NULL},

	{XtNunselect, XtCCallback, XtRCallback, sizeof(XtPointer), 
	 offset(checkBox.unselect), XtRCallback, (XtPointer) NULL},

	{XtNset, XtCSet, XtRBoolean, sizeof(Boolean),
	 offset(checkBox.set), XtRBoolean, (XtPointer) &defFALSE},

	{XtNdim, XtCDim, XtRBoolean, sizeof(Boolean),
	 offset(checkBox.dim), XtRBoolean, (XtPointer) &defFALSE},

	{XtNsensitive, XtCSensitive, XtRBoolean, sizeof(Boolean),
	 offset(checkBox.sensitive), XtRBoolean, (XtPointer) &defTRUE},

	{XtNlabel, XtCLabel, XtRString, sizeof(String),
	 offset(checkBox.label), XtRString, (XtPointer) NULL},

	{XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
	 offset(checkBox.foreground), XtRString, (XtPointer) XtDefaultForeground},

	{XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
	 offset(checkBox.font), XtRString, (XtPointer)OlDefaultFont},

	{XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel),
	 offset(checkBox.fontcolor), XtRString, (XtPointer) XtDefaultForeground},

 	{XtNlabelType, XtCLabelType, XtROlDefine, sizeof(OlDefine),
 	 offset(checkBox.labeltype), XtRImmediate,
 		(XtPointer) ((OlDefine) OL_STRING ) },

 	{XtNlabelJustify, XtCLabelJustify, XtROlDefine, sizeof(OlDefine),
 	 offset(checkBox.labeljustify), XtRImmediate,
 		(XtPointer) ((OlDefine) OL_LEFT ) },

 	{XtNposition, XtCPosition, XtROlDefine, sizeof(OlDefine),
 	  offset(checkBox.position),XtRImmediate,
		(XtPointer) ((OlDefine) OL_LEFT ) },

	{XtNlabelTile, XtCLabelTile, XtRBoolean, sizeof(Boolean),
	 offset(checkBox.labeltile), XtRBoolean, (XtPointer) &defFALSE},

	{XtNlabelImage, XtCLabelImage, XtRPointer, sizeof(XImage *),
	 offset(checkBox.labelimage), XtRPointer, (XtPointer)NULL},

	{XtNrecomputeSize, XtCRecomputeSize, XtRBoolean, sizeof(Boolean),
	 offset(checkBox.recompute_size), XtRBoolean, (XtPointer) &defTRUE },

	{ XtNaccelerator, XtCAccelerator, XtRString, sizeof(String),
	  offset(checkBox.accelerator), XtRString, (XtPointer) NULL },

	{ XtNacceleratorText, XtCAcceleratorText, XtRString, sizeof(String),
	  offset(checkBox.accelerator_text), XtRString, (XtPointer) NULL },

	{ XtNmnemonic, XtCMnemonic, OlRChar, sizeof(char),
	  offset(checkBox.mnemonic), XtRImmediate, (XtPointer) '\0' }
};

#undef offset

/* 
 *************************************************************************
 *
 *  Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

CheckBoxClassRec checkBoxClassRec = {
  {
    (WidgetClass) &(managerClassRec),	/* superclass		  */	
    "CheckBox",				/* class_name		  */
    sizeof(CheckBoxRec),		/* widget_size		  */
    ClassInitialize,			/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    (XtInitProc) Initialize,		/* initialize		  */
    NULL,				/* initialize_hook	  */
    (XtRealizeProc) Realize,		/* realize		  */
    NULL,				/* actions		  */
    0,					/* num_actions		  */
    resources,				/* resources		  */
    XtNumber(resources),		/* num_resources	  */
    NULLQUARK,				/* xrm_class		  */
    FALSE,				/* compress_motion	  */
    TRUE,				/* compress_exposure	  */
    TRUE,				/* compress_enterleave    */
    FALSE,				/* visible_interest	  */
    NULL,				/* destroy		  */
    (XtWidgetProc) Resize,		/* resize		  */
    (XtExposeProc) Redisplay,		/* expose		  */
    (XtSetValuesFunc) SetValues,	/* set_values		  */
    NULL,				/* set_values_hook	  */
    XtInheritSetValuesAlmost,		/* set_values_almost	  */
    (XtArgsProc) GetValuesHook,		/* get_values_hook	  */
    AcceptFocus,			/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    defaultTranslations,		/* tm_table		  */
    (XtGeometryHandler) QueryGeometry,	/* query_geometry	  */
  },  /* CoreClass fields initialization */
  {
    (XtGeometryHandler) GeometryManager,/* geometry_manager	*/
    NULL,				/* changed_managed	*/
    (XtWidgetProc) InsertChild,		/* insert_child		*/
    XtInheritDeleteChild,		/* delete_child		*/
    NULL,				/* extension    	*/
  },  /* CompositeClass fields initialization */
  {
    /* resources	  */	NULL,
    /* num_resources	  */	0,
    /* constraint_size	  */	0,
    /* initialize	  */	NULL,
    /* destroy		  */	NULL,
    /* set_values	  */	NULL
  },	/* constraint_class fields */
  {
    /* highlight_handler  */	HighlightHandler,
    /* focus_on_select	   */	True,
    /* traversal_handler  */    NULL,
    /* activate		  */    ActivateWidget, 
    /* event_procs	  */    CBevents,
    /* num_event_procs	  */	XtNumber(CBevents),
    /* register_focus	  */	NULL,
    /* version		  */	OlVersion,
    /* extension	  */	NULL,
    /* dyn_data		  */	{ dyn_res, XtNumber(dyn_res) },
    /* transparent_proc	  */	XtInheritTransparentProc,
  },	/* manager_class fields   */
  {
    0					/* not used now */
  }  /* CheckBoxClass fields initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

   WidgetClass checkBoxWidgetClass = (WidgetClass) &checkBoxClassRec;  
/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 ************************************************************
 *
 *  GetAttrs - This function get the attributes for the checkbox box 
 *
 *********************function*header************************
 */

static void
GetAttrs (cb)
    CheckBoxWidget	cb;
{
    Pixel	fg;
    OlgBG	bg;
    Boolean	pixmapBG;
    Pixel	focus_color;
    Boolean	has_focus;

    if (cb->checkBox.pAttrs)
	OlgDestroyAttrs (cb->checkBox.pAttrs);

    /* Worry about input focus color conflicts */
    focus_color = cb->manager.input_focus_color;
    has_focus = cb->manager.has_focus;

    if (has_focus)
    {
	if (cb->checkBox.foreground == focus_color ||
	    cb->core.background_pixel == focus_color)
	{
	    /* input focus color conflicts with either the foreground
	     * or background color.  Reverse fg and bg.
	     */
	    fg = cb->core.background_pixel;
	    bg.pixel = cb->checkBox.foreground;
	    pixmapBG = False;
	}
	else
	{
	    /* no color conflict */
	    fg = cb->checkBox.foreground;
	    bg.pixel = focus_color;
	    pixmapBG = False;
	}
    }
    else
    {
	/* normal coloration */
	fg = cb->checkBox.foreground;

	if (cb->core.background_pixmap != None &&
	    cb->core.background_pixmap != XtUnspecifiedPixmap)
	{
	    bg.pixmap = cb->core.background_pixmap;
	    pixmapBG = True;
	}
	else
	{
	    bg.pixel = cb->core.background_pixel;
	    pixmapBG = False;
	}
    }

    cb->checkBox.pAttrs = OlgCreateAttrs (XtScreen (cb), fg, &(bg), pixmapBG,
					  OL_DEFAULT_POINT_SIZE);
}

/*
 ************************************************************
 *
 *  DrawBox - This function displays/redisplays the checkbox box 
 *		and redisplays/erases the check.
 *
 *********************function*header************************
 */

static void 
DrawBox(w) 
	Widget w;
{
	CheckBoxWidget cb = (CheckBoxWidget) w;
	unsigned flags = 0;

	if(XtIsRealized(w)==FALSE) 
		return;

	if (cb->checkBox.set)
	    flags |= CB_CHECKED;

	if(cb->checkBox.dim || !XtIsSensitive(w))
	    flags |= CB_DIM;

	OlgDrawCheckBox (XtScreen (w), XtWindow (w), cb->checkBox.pAttrs,
			 cb->checkBox.x1, cb->checkBox.y1, flags);
} /* DrawBox */

/*
 ************************************************************
 *
 *  PointInBox - This function checks to see if pointer 
 *  is within the boundaries of the checkbox box.
 *
 *********************function*header************************
 */

static Boolean
PointInBox(w,x,y,activate)
	Widget w;
	Position x,y;
	Boolean activate;
{
	CheckBoxWidget cb = (CheckBoxWidget) w;

	if(activate) 
		return TRUE;

	if(x>=cb->checkBox.x1
		&& x<=cb->checkBox.x2
		&& y>=cb->checkBox.y1
		&& y<=cb->checkBox.y2)

		return TRUE;

	return FALSE;

}	/* PointInBox */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * AcceptFocus - this routine is to set focus to the checkbox itself
 * instead of looking for a child to set focus to (as XtInheritAcceptFocus
 * for the manager class does - code is same as for primitive).
 ****************************procedure*header*****************************
 */
static Boolean
AcceptFocus OLARGLIST((w, time))
	OLARG( Widget,	w)
	OLGRA( Time *,	time)
{
	if (OlCanAcceptFocus(w, *time))
	{
		return(OlSetInputFocus(w, RevertToNone, *time));
	}

	return (False);

} /* AcceptFocus */

/*
 *************************************************************************
 * ActivateWidget - this routine is used to activate the callbacks of
 * this widget.
 ****************************procedure*header*****************************
 */

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
	Boolean ret=FALSE;
	Boolean activate=TRUE;
	XEvent dummy_event;

	ret=PreviewState(w,&dummy_event,type,activate);
	if(ret==FALSE)
		return FALSE;
	ret=SetState(w,&dummy_event,type,activate);
		return ret;

} /* END OF ActivateWidget() */

/*
 ************************************************************
 *
 *  ClassInitialize - Register OlDefine string values.
 *
 *********************function*header************************
 */

static void 
ClassInitialize()
{
	_OlAddOlDefineType ("left",   OL_LEFT);
	_OlAddOlDefineType ("right",  OL_RIGHT);
	_OlAddOlDefineType ("string", OL_STRING);
	_OlAddOlDefineType ("image",  OL_IMAGE);

}	/*  ClassInitialize  */

/*
 ************************************************************
 *
 *  Destroy
 *
 *********************function*header************************
 */

static void 
Destroy(widget)
    Widget widget;
{
	CheckBoxWidget cb = (CheckBoxWidget) widget;

	if (cb->checkBox.pAttrs != (OlgAttrs *) 0)
	    OlgDestroyAttrs (cb->checkBox.pAttrs);

}	/* Destroy */

/*
 ************************************************************
 *
 *  GeometryManager - This function is called when a button child
 *	wants to resize itself; the current policy is to allow
 *	the requested resizing.
 *
 *********************function*header************************
 */

static XtGeometryResult GeometryManager(widget,request,reply)
	Widget widget;			
	XtWidgetGeometry *request;
	XtWidgetGeometry *reply;
{
	CheckBoxWidget cb;
	ButtonWidget lw;
	XtGeometryResult xtgeometryresult=XtGeometryYes;

	cb = (CheckBoxWidget) XtParent(widget);

	if ((request->height==(Dimension)0 &&
	     (request->request_mode & CWHeight)) ||
	    (request->width==(Dimension)0 &&
	     (request->request_mode & CWWidth))) {

	OlVaDisplayWarningMsg(	XtDisplay(widget),
				OleNinvalidDimension,
				OleTbadGeometry,
				OleCOlToolkitWarning,
				OleMinvalidDimension_badGeometry,
				XtName(widget),
				OlWidgetToClassName(widget));
 
		return XtGeometryNo;
	}

	ResizeSelf(cb,TRUE);		/* does geometry request too */


					/* this is the string or image */
	if(widget == cb->checkBox.label_child) {
		lw= (ButtonWidget) cb->checkBox.label_child;
		reply->height= lw->core.height;
		reply->width= lw->core.width;
		if(reply->height==request->height 
			&& reply->width==request->width) {

			xtgeometryresult = XtGeometryYes;
		}
		else 	xtgeometryresult = XtGeometryAlmost;
	}			

	reply->request_mode=CWWidth | CWHeight;
	return xtgeometryresult;

} /* GeometryManager */

/*
 ************************************************************
 *
 *  GetValuesHook - This function gets values from the button
 *	widget label/image as needed.
 *
 *********************function*header************************
 */

static void 
GetValuesHook(w,args,num_args)
Widget w;
ArgList args;
Cardinal *num_args;
{
	Cardinal          count;
	CheckBoxWidget	  cb = (CheckBoxWidget) w;
	ArgList	          new_list = (ArgList) NULL;
	static MaskArg    mask_list[] = {
		{ XtNlabel,	NULL,	OL_SOURCE_PAIR	},
		{ XtNfont,	NULL,	OL_SOURCE_PAIR	},
		{ XtNlabelImage,NULL,	OL_SOURCE_PAIR	}
	};

	_OlComposeArgList(args, *num_args, mask_list, XtNumber(mask_list),
			 &new_list, &count);

	if (cb->checkBox.label_child != (Widget) NULL && count > (Cardinal)0) {
		XtGetValues(cb->checkBox.label_child, new_list, count);
		XtFree((char *)new_list);
	}

} /* GetValuesHook */


/*
 *************************************************************************
 * HighlightHandler - changes the colors when this widget gains or loses
 * focus.
 ****************************procedure*header*****************************
 */
static void
HighlightHandler OLARGLIST((w, type))
	OLARG(Widget,	w)
	OLGRA(OlDefine,	type)
{
	GetAttrs ((CheckBoxWidget) w);
	Redisplay (w, NULL, NULL);
} /* END OF HighlightHandler() */


/*
 ************************************************************
 *
 *  Initialize - This function checks that checkBox variable
 *	values are within range and initializes private fields
 *
 *********************function*header************************
 */

/* ARGSUSED */
static void 
Initialize(request, new, args, num_args)
	CheckBoxWidget	request;
	CheckBoxWidget	new;
	ArgList		args;
	Cardinal *	num_args;
{
	Widget ewidget,parent;
	CorePart *cp;
	CheckBoxPart *ep;
	ManagerPart	*mp;
	Arg arg[10];
	int n,n1;
	char *label;

	ewidget= (Widget) new;
	cp = &(new->core);
	ep = &(new->checkBox);
	mp = &(new->manager);
	parent= XtParent(new);

	/* ******************************************* */
	/* check that core values correct or not reset */
	/* ******************************************* */

	if (cp->height==(Dimension)0) {
		cp->height=(Dimension)1;
	}

	if (cp->width==(Dimension)0) {
		cp->width=(Dimension)1;
	}

	if (cp->border_width!=(Dimension)0) {
		cp->border_width=(Dimension)0;
	}

	if (cp->background_pixel!=parent->core.background_pixel) {
		cp->background_pixel=parent->core.background_pixel;
	}

	if (cp->background_pixmap!=parent->core.background_pixmap) {
		cp->background_pixmap=parent->core.background_pixmap;
	}

	ep->pAttrs = NULL;
	GetAttrs (new);

	if (ep->labeljustify!=(OlDefine)OL_LEFT 
		&& ep->labeljustify!=(OlDefine) OL_RIGHT ) {

		ep->labeljustify= (OlDefine) OL_LEFT;

		OlVaDisplayWarningMsg(	XtDisplay((Widget)new),
					OleNinvalidResource,
					OleTinitialize,
					OleCOlToolkitWarning,
					OleMinvalidResource_initialize,
					XtName((Widget)new),
					OlWidgetToClassName((Widget)new),
					XtNlabelJustify,
					"OL_LEFT");
	}

	if (ep->position!=(OlDefine)OL_LEFT 
		&& ep->position!=(OlDefine)OL_RIGHT) {

		ep->position=(OlDefine)OL_LEFT;

		OlVaDisplayWarningMsg(	XtDisplay((Widget)new),
					OleNinvalidResource,
					OleTinitialize,
					OleCOlToolkitWarning,
					OleMinvalidResource_initialize,
					XtName((Widget)new),
					OlWidgetToClassName((Widget)new),
					XtNposition,
					"OL_LEFT");
	}

					/* make the label child */
	ep->label_child=(Widget) NULL;

	n=0;

	if (ep->labeltype!=(OlDefine)OL_STRING 
		&& ep->labeltype!=(OlDefine) OL_IMAGE ) {

		ep->labeltype=(OlDefine) OL_STRING;

		OlVaDisplayWarningMsg(	XtDisplay((Widget)new),
					OleNinvalidResource,
					OleTinitialize,
					OleCOlToolkitWarning,
					OleMinvalidResource_initialize,
					XtName((Widget)new),
					OlWidgetToClassName((Widget)new),
					XtNlabelType,
					"OL_STRING");
	}

	XtSetArg(arg[n],XtNlabelType,(XtArgVal)ep->labeltype);
	n++;

	if (ep->labelimage!=(XImage *)NULL) {
		XtSetArg(arg[n],XtNlabelImage,(XtArgVal)ep->labelimage);
		n++;
	}
	XtSetArg(arg[n],XtNlabelTile,(XtArgVal)ep->labeltile);
	n++;

	XtSetArg(arg[n],XtNbackground,(XtArgVal) cp->background_pixel);
	n++;

	if(cp->background_pixmap!=XtUnspecifiedPixmap) {
	XtSetArg(arg[n],XtNbackgroundPixmap,(XtArgVal)cp->background_pixmap);
	n++;
	}

	if(ep->font!=(XFontStruct *)NULL) {
	XtSetArg(arg[n],XtNfont,(XtArgVal)ep->font);
	n++;
	}

	XtSetArg(arg[n],XtNforeground,ep->fontcolor);
	n++;

	n1=n-1;
    	ep->label_child = 
	XtCreateManagedWidget(cp->name,buttonWidgetClass,ewidget,arg,n);

			/* Delete this button Widget from the traversal
			 * list
			 */

	_OlDeleteDescendant(ep->label_child);

	XtSetSensitive(ep->label_child,ep->sensitive);

	ep->setvalue = ep->set;	/* to track press-drag-release behavior */

		/* add me to the traversal list */
	_OlUpdateTraversalWidget(ewidget, mp->reference_name,
				 mp->reference_widget, True);

/*
 * For mnemonics and accelerators, do SetValues() on button child 
 * to check validity of values and modify label display but then 
 * move  to checkbox since this is the widget that should be fired.
 */

	if(ep->label!=(char *)NULL) {
		label=ep->label;
	}
	else {
		label=cp->name;
	}

	XtVaSetValues( ep->label_child,
		XtNmnemonic, (XtArgVal) ep->mnemonic,
		XtNaccelerator, (XtArgVal) ep->accelerator,
		XtNacceleratorText, (XtArgVal) ep->accelerator_text,
		XtNlabel, (XtArgVal) label, 
		NULL);

	XtVaGetValues( ep->label_child,
		XtNmnemonic, (XtArgVal) &ep->mnemonic,
		XtNaccelerator, (XtArgVal) &ep->accelerator,
		XtNacceleratorText, (XtArgVal) &ep->accelerator_text,
		NULL);

	if(ep->mnemonic!=(OlMnemonic) 0) {
		_OlRemoveMnemonic(ep->label_child,
				(XtPointer)0, FALSE, ep->mnemonic);
		_OlAddMnemonic((Widget) new, 
				(XtPointer)NULL, ep->mnemonic);

	}
		
	if(ep->accelerator!=(String) NULL) {
		ep->accelerator = XtNewString(ep->accelerator);
		_OlRemoveAccelerator(ep->label_child,
				(XtPointer)0, FALSE, ep->accelerator);
		_OlAddAccelerator((Widget) new, 
			(XtPointer)NULL, ep->accelerator);
	}
		
	if(ep->accelerator_text!=(String) NULL)
		ep->accelerator_text = XtNewString(ep->accelerator_text);

	/* reset fields necessary: note that these lines must be last */

	if(ep->label!=(char *)NULL) ep->label=(char *)NULL;
	if(ep->labelimage!=(XImage *)NULL) ep->labelimage=(XImage *)NULL;
	if(ep->font!=(XFontStruct *)NULL) ep->font=(XFontStruct *)NULL;

	ResizeSelf(new,TRUE);

} 	/* Initialize */

/*
 ************************************************************
 *
 *  InsertChild - This function prevents other children
 *	from being added --- needed since checkBox a composite widget.
 *
 *********************function*header************************
 */

static void
InsertChild(widget)
	Widget widget;
{
    CheckBoxWidget	pw = (CheckBoxWidget) XtParent(widget);
    XtWidgetProc	insert_child = ((CompositeWidgetClass)
	(checkBoxClassRec.core_class.superclass))->composite_class.insert_child;

    if(pw->checkBox.label_child!=(Widget)NULL) {

	OlVaDisplayWarningMsg(	XtDisplay(widget),
				OleNgoodParent,
				OleTnoChild,
				OleCOlToolkitWarning,
				OleMgoodParent_noChild,
				XtName(widget),
				OlWidgetToClassName(widget));

	XtDestroyWidget(widget);
	return;
    }

    (*insert_child)(widget);

    XtSetMappedWhenManaged(widget,TRUE); /* default but do it anyhow */
    XtSetSensitive(widget,pw->core.sensitive);

	_OlDeleteDescendant(widget);	/* remove button from traversal list */

}	/* InsertChild */

/*
 ************************************************************
 *
 *  QueryGeometry - This function is a rather rigid prototype
 *	since the widget insists upon its core height and width.
 *
 ************************************************************
*/

static XtGeometryResult QueryGeometry(widget,request,preferred) 
	Widget widget;
	XtWidgetGeometry *request;
	XtWidgetGeometry *preferred;
{
	CheckBoxWidget cb;

	cb= (CheckBoxWidget) widget;

		/* if not height or width it is okay with widget */

	if(!(request->request_mode & CWHeight 
			|| request->request_mode & CWWidth)) {

		return XtGeometryYes;
	}

		/* if still here look at requested height and/or width */

	if((request->request_mode & CWHeight) 
			&& (request->height!=cb->checkBox.normal_height)) {

			preferred->request_mode |=CWHeight;
			preferred->height=cb->checkBox.normal_height;
	};
		
	if((request->request_mode & CWWidth)
		&& (request->width!=cb->checkBox.normal_width)) {

		preferred->request_mode |=CWWidth;
		preferred->width=cb->checkBox.normal_width;
	};

	if((preferred->request_mode & CWHeight)
		|| (preferred->request_mode & CWWidth)) {

		return XtGeometryAlmost;
	}

	else return XtGeometryNo;

} /* QueryGeometry */

/*
 ************************************************************
 *
 *  Realize - This function realizes the CheckBox widget in
 *	order to be able to set window properties
 *
 *********************function*header************************
 */

static void 
Realize(widget,ValueMask,attributes) 
	Widget widget;
	Mask *ValueMask;
	XSetWindowAttributes *attributes;
{
	CheckBoxWidget cb = (CheckBoxWidget) widget;
	Widget label_child = cb->checkBox.label_child;

	(* (checkBoxClassRec.core_class.superclass->core_class.realize))
		(widget, ValueMask, attributes);

	XtRealizeWidget(label_child);
					/* checkbox to get select events */
	XtUninstallTranslations(label_child); 

} /* Realize */

/*
 ************************************************************
 *
 *  Resize - This function warns when checkbox truncated.
 *
 *********************function*header************************
 */

static void 
Resize(w)
	Widget w;
{
	CheckBoxWidget cb = (CheckBoxWidget) w;

	ResizeSelf(cb,FALSE);		/* do not make geometry request */

} 	/* Resize */

/*
 ************************************************************
 *
 *  Redisplay - This function is needed to redraw a 
 *		check once the widget is being mapped.
 *
 *********************function*header************************
 */

static void 
Redisplay(widget, event, region)
Widget widget;
XEvent *event;	
Region region;
{
	DrawBox(widget);

}	/* Redisplay */

/*
 ************************************************************
 *
 *  ResizeSelf - This function calculates the size needed for the
 *	CheckBox widget, as a function of children's sizes,
 *	and repositions the latter accordingly.
 *
 *********************function*header************************
 */

static void 
ResizeSelf(ew,greq) 
	CheckBoxWidget ew;
	Boolean greq;
{
	XtWidgetGeometry request,reply;
	ButtonWidget label;
	Widget widget,parent;
	Position xlabel,ylabel,xbox,ybox;
	Dimension hbox, wbox;
	int wlabel,hlabel,left_space,center_space,right_space,
		top_space,bottom_space,wcheckbox,maxheight;

	widget= (Widget) ew;
	parent=XtParent(widget);

	label = (ButtonWidget) ew->checkBox.label_child;

	wlabel= (int) label->button.normal_width;
	hlabel= (int) label->button.normal_height;
	left_space= 1;
	right_space= 3;
	top_space= 3;
	bottom_space= 1;
 	center_space= label->button.scale; 

	OlgSizeCheckBox (XtScreen (ew), ew->checkBox.pAttrs, &wbox, &hbox);

	if((Dimension)hlabel >= hbox)
		maxheight=hlabel;
	else
		maxheight=hbox;

	ew->checkBox.normal_height = 
			(Dimension) (top_space + maxheight + bottom_space) ;

	ew->checkBox.normal_width =
			(Dimension) (left_space + wlabel 
			    + center_space + wbox + right_space);

	if(greq) {			/* geometry request */

	request.height = ew->checkBox.normal_height;
	request.width =  ew->checkBox.normal_width;

	request.request_mode = CWHeight | CWWidth ;

	switch(XtMakeGeometryRequest((Widget)ew,&request,&reply)) {
		case XtGeometryDone:
			break;
		case XtGeometryYes:
			break;
		case XtGeometryAlmost:
			XtMakeGeometryRequest((Widget)ew,&reply,&reply);
			break;
		case XtGeometryNo:
			break;
	}

	} /* if geometry request */

				/* nonexclusives sizes its own children */
	if(XtIsSubclass(parent,nonexclusivesWidgetClass)) 
		wcheckbox=(int)ew->core.width;
	else 			/* assume normal width and clip */
		wcheckbox=(int)ew->checkBox.normal_width;

				/* compute label and box position */

	if(ew->checkBox.labeljustify==(OlDefine)OL_LEFT
			&&	ew->checkBox.position==OL_LEFT) {

		xbox = (Position) ( wcheckbox - (wbox + right_space));
		xlabel= (Position) left_space;
	}
	else if(ew->checkBox.labeljustify==(OlDefine)OL_RIGHT
			&&	ew->checkBox.position==(OlDefine)OL_LEFT) {

		xbox = (Position) ( wcheckbox - (wbox + right_space));
		xlabel= (Position) (xbox - (center_space + wlabel));
	}
	else if(ew->checkBox.labeljustify==(OlDefine)OL_LEFT 
			&& ew->checkBox.position==(OlDefine)OL_RIGHT) {
		xbox = (Position) left_space;
		xlabel= (Position) (left_space + wbox + center_space);
	}
	else if(ew->checkBox.labeljustify==(OlDefine)OL_RIGHT 
			&& ew->checkBox.position==(OlDefine)OL_RIGHT) {

		xbox = (Position) left_space;
		xlabel= (Position) (wcheckbox -( right_space + wlabel)) ;
	}

	ylabel = (Position)
		((int)(ew->checkBox.normal_height 
				- label->button.normal_height) / (int)2);
	ybox = (Position) 
		(((int)((int)(ew->checkBox.normal_height) - hbox)) / (int)2);

			/* Note: for font, y is at baseline */

	ew->checkBox.x1 = xbox;
	ew->checkBox.x2 = xbox + (Position) wbox;
	ew->checkBox.y1 = ybox -(Position) 2;
	ew->checkBox.y2 = ybox + (Position) hbox -(Position)2;

	XtConfigureWidget(ew->checkBox.label_child,
		xlabel, ylabel,
		label->button.normal_width,
		label->button.normal_height, (Dimension)0);

} /* ResizeSelf */

/*
 ************************************************************
 *
 *  SetValues - This function checks and allows setting and
 *	resetting of CheckBox resources.
 *
 *********************function*header************************
 */

/* ARGSUSED */
static Boolean
SetValues(current, request, new, args, num_args)
	Widget		current;
	Widget		request;
	Widget		new;
	ArgList		args;
	Cardinal *	num_args;
{
	int oldvalue,newvalue;
	OlDefine olddefine,newdefine;
	Boolean needs_redisplay=FALSE,sensitive;
	Widget label,parent;
	ButtonWidget lw,oldlw;
	Arg arg[2];
	XSetWindowAttributes attributes;
	Boolean Grequest=FALSE, reposition_children=FALSE;

	CheckBoxWidget currew =	(CheckBoxWidget) current;
	CheckBoxWidget newew =	(CheckBoxWidget) new;

	parent=XtParent(current);
	label=newew->checkBox.label_child;
	lw= (ButtonWidget) label;

	/* ******************************************* */
	/* check that core values correct or not reset */
	/* ******************************************* */

	if (newew->core.height==(Dimension)0) {
		newew->core.height=(Dimension)1;
		needs_redisplay=TRUE;
	}

	if (newew->core.width==(Dimension)0) {
		newew->core.width=(Dimension)1;
		needs_redisplay=TRUE;
	}

	if (newew->core.border_width!=(Dimension)0) {
		newew->core.border_width=(Dimension)0;
		needs_redisplay=TRUE;
	}

	if (newew->core.background_pixel!=parent->core.background_pixel) {
		newew->core.background_pixel=parent->core.background_pixel;
		if (XtIsRealized((Widget)newew)) {
		attributes.background_pixel=parent->core.background_pixel;
		XChangeWindowAttributes(XtDisplay(newew),XtWindow(newew),
			CWBackPixel,&attributes);
		needs_redisplay=TRUE;
		}
	}
	
		/* 	this is a kludgy check: in case parent's and
			widget's background pixel have both been changed,
			need to change label background */
		
	if(lw->core.background_pixel!=newew->core.background_pixel) {
		XtSetArg(arg[0],XtNbackground,newew->core.background_pixel);
		XtSetValues(label,arg,1);
		needs_redisplay=TRUE;
	}

	if (newew->core.background_pixmap!=parent->core.background_pixmap) {
		newew->core.background_pixmap=parent->core.background_pixmap;
		if ((newew->core.background_pixmap!=XtUnspecifiedPixmap) &&
		    XtIsRealized((Widget)newew)) {

		attributes.background_pixmap=newew->core.background_pixmap;
		XChangeWindowAttributes(XtDisplay(newew),XtWindow(newew),
			CWBackPixmap,&attributes);
		}
		needs_redisplay=TRUE;
	}

		/* 	this is also a kludgy check: in case parent's and
			widget's background pixmap have both been changed,
			need to change label background pixmap */
		
	if(lw->core.background_pixmap!=newew->core.background_pixmap) {
		if(newew->core.background_pixmap!=XtUnspecifiedPixmap) {

		XtSetArg(arg[0],XtNbackgroundPixmap,newew->core.background_pixmap);
		XtSetValues(label,arg,1);
		needs_redisplay=TRUE;
		}
	}

	/* XtNforeground */

	if((newew->checkBox.foreground!=currew->checkBox.foreground) ||
	  (newew->core.background_pixel!=currew->core.background_pixel)) {
	        OlgBG	bg;
		Boolean	bgPixmapType;
		CorePart *cp = &newew->core;

		GetAttrs (newew);
		needs_redisplay=TRUE;
	}

	/* XtNfont */

	if(newew->checkBox.font!=(XFontStruct *)NULL) {
		XtSetArg(arg[0],XtNfont,newew->checkBox.font);
		XtSetValues(label,arg,1);
		newew->checkBox.font=(XFontStruct *)NULL;
		Grequest=TRUE;
	}

	/* XtNfontColor */

	if(newew->checkBox.fontcolor!=currew->checkBox.fontcolor) {
		XtSetArg(arg[0],XtNforeground,newew->checkBox.fontcolor);
		XtSetValues(label,arg,1);
	}

	/* XtNlabelTile */

	if(newew->checkBox.labeltile!=currew->checkBox.labeltile) {
		XtSetArg(arg[0],XtNlabelTile,newew->checkBox.labeltile);
		XtSetValues(label,arg,1);
	}
	
	/* ******************************** */
	/* check checkbox private resources */
	/* ******************************** */

	/* XtNselect resource */
	/* XtNunselect resource */

		/* intrinsics does work here */

	/* XtNset resource */

	oldvalue=currew->checkBox.set;
	newvalue=newew->checkBox.set;

	if((newvalue==FALSE && oldvalue!=FALSE)
		|| (newvalue!=FALSE && oldvalue==FALSE)) {

		needs_redisplay=TRUE;
	}

	/* XtNdim resource */

	oldvalue=currew->checkBox.dim;
	newvalue=newew->checkBox.dim;

	if((newvalue==FALSE && oldvalue!=FALSE)
		|| (newvalue!=FALSE && oldvalue==FALSE)) {

		needs_redisplay=TRUE;
	}

	/* XtNsensitive resource */

	if((sensitive=XtIsSensitive(new))!=XtIsSensitive(current)) {
		XtSetSensitive(label,sensitive);
		needs_redisplay=TRUE;
	}

	/* XtNlabel resource */

	if (newew->checkBox.label!= (char *) NULL) { 
		XtSetArg(arg[0],XtNlabelType,(OlDefine) OL_LABEL);
		XtSetArg(arg[1],XtNlabel,newew->checkBox.label);
		XtSetValues(newew->checkBox.label_child,arg,2);
		newew->checkBox.label=(char *)NULL;
		Grequest=TRUE;
		reposition_children=TRUE;
	}

	/* XtNlabelJustify resource */

	olddefine=currew->checkBox.labeljustify;
	newdefine=newew->checkBox.labeljustify;
	if(newdefine!=olddefine) {
		if(newdefine==(OlDefine)OL_LEFT 
			|| newdefine==(OlDefine)OL_RIGHT) {
			reposition_children=TRUE;
		}

		else {
			newew->checkBox.labeljustify=olddefine;

			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNinvalidResource,
						OleTsetValuesNC,
						OleCOlToolkitWarning,
						OleMinvalidResource_setValuesNC,
						XtName(new),
						OlWidgetToClassName(new),
						XtNlabelJustify);
		}
	}

	/* XtNposition resource */

	olddefine=currew->checkBox.position;
	newdefine=newew->checkBox.position;
	if(newdefine!=olddefine) {
		if(newdefine==(OlDefine)OL_LEFT 
			|| newdefine==(OlDefine)OL_RIGHT) {

			reposition_children=TRUE;
		}
		else {
			newew->checkBox.position=olddefine;

			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNinvalidResource,
						OleTsetValuesNC,
						OleCOlToolkitWarning,
						OleMinvalidResource_setValuesNC,
						XtName(new),
						OlWidgetToClassName(new),
						XtNposition);
		}
	}

	/* XtNlabelType resource */

	olddefine=currew->checkBox.labeltype;
	newdefine=newew->checkBox.labeltype;

	if(newdefine!=olddefine) {
		if(newdefine==(OlDefine)OL_STRING  
			|| newdefine==(OlDefine)OL_IMAGE) {
			XtSetArg(arg[0],XtNlabelType,newew->checkBox.labeltype);
			XtSetValues(newew->checkBox.label_child,arg,1);
		}
		else {
			newew->checkBox.labeltype=olddefine;

			OlVaDisplayWarningMsg(	XtDisplay(new),
						OleNinvalidResource,
						OleTsetValuesNC,
						OleCOlToolkitWarning,
						OleMinvalidResource_setValuesNC,
						XtName(new),
						OlWidgetToClassName(new),
						XtNlabelType);
		}
	}

	/* XtNlabelImage resource */

	if(newew->checkBox.labelimage!=(XImage *)NULL) {
		XtSetArg(arg[0],XtNlabelType,(OlDefine) OL_IMAGE);
		XtSetArg(arg[1],XtNlabelImage,newew->checkBox.labelimage);
		XtSetValues(label,arg,2);
		newew->checkBox.labelimage=(XImage *)NULL;
	}

	/* XtNrecomputeSize resource */

	oldvalue=currew->checkBox.recompute_size;
	newvalue=newew->checkBox.recompute_size;

	if(newvalue!=oldvalue) {
		if((newvalue==FALSE && oldvalue!=FALSE)
			|| (newvalue!=FALSE && oldvalue==FALSE)) {
			Grequest=TRUE;
		}
	}

	/* XtNdefaultData resource */

		/* let widget writer set and get any pointer needed 
		   for updating "cloned" menus - intrinsics does work here */

	/* XtNmnemonic resource */

	if(newew->checkBox.mnemonic!=currew->checkBox.mnemonic) {

	OlMnemonic mnemonic = (OlMnemonic) 0;

	XtSetArg(arg[0],XtNmnemonic, (XtArgVal) newew->checkBox.mnemonic);
	XtSetValues(label,arg,1); 
	XtSetArg(arg[0],XtNmnemonic, &mnemonic);
	XtGetValues(label,arg,1);
		
	if(newew->checkBox.mnemonic==mnemonic)
	{
		_OlRemoveMnemonic(label,
			(XtPointer)0, FALSE, newew->checkBox.mnemonic);
		_OlRemoveMnemonic(new,
			(XtPointer)0, FALSE, currew->checkBox.mnemonic);
		_OlAddMnemonic(new,
				(XtPointer)NULL, newew->checkBox.mnemonic);
		Grequest = TRUE;
		needs_redisplay = TRUE;
	}
	else
		newew->checkBox.mnemonic = (OlMnemonic)0;
	}
	
{
#define CHANGED(field)	(newew->checkBox.field != currew->checkBox.field)

	Boolean changed_accelerator = CHANGED(accelerator);
	Boolean changed_accelerator_text = CHANGED(accelerator_text);

	/* XtNaccelerator resource */

	if(changed_accelerator) {

	String accelerator = (String) NULL;

	XtSetArg(arg[0],XtNaccelerator, (XtArgVal) newew->checkBox.accelerator);
	XtSetValues(label,arg,1);
	XtSetArg(arg[0],XtNaccelerator, &accelerator);
	XtGetValues(label,arg,1);
		
	if(accelerator!=(String)NULL) {

		_OlRemoveAccelerator(label,
			(XtPointer)0, FALSE, newew->checkBox.accelerator);
		_OlRemoveAccelerator(new,
			(XtPointer)0, FALSE, currew->checkBox.accelerator);
		_OlAddAccelerator(new,
				(XtPointer)NULL, newew->checkBox.accelerator);
		XtFree(currew->checkBox.accelerator);
		newew->checkBox.accelerator = 
			XtNewString(newew->checkBox.accelerator);
		Grequest = TRUE;
		needs_redisplay = TRUE;
	}
	else
		newew->checkBox.accelerator = (String) NULL;
	}

	/* XtNacceleratorText resource */

	if(changed_accelerator || changed_accelerator_text) {

	String accelerator_text = (String) NULL;

	XtFree(currew->checkBox.accelerator_text);
	currew->checkBox.accelerator_text = (String)NULL;

	if(changed_accelerator && !changed_accelerator_text)
			newew->checkBox.accelerator_text = (String)NULL;

	XtSetArg(arg[0],XtNacceleratorText, 
		(XtArgVal) newew->checkBox.accelerator_text);
	XtSetValues(label,arg,1);
	XtSetArg(arg[0],XtNacceleratorText, &accelerator_text);
	XtGetValues(label,arg,1);
		
	if(accelerator_text!=(String)NULL) {
		newew->checkBox.accelerator_text = 
			XtNewString(newew->checkBox.accelerator_text);
	}
	else 
		newew->checkBox.accelerator_text = (String) NULL;

	Grequest = TRUE;
	needs_redisplay = TRUE;
	}
#undef CHANGED
}
	/* do not make geometry request - intrinsics will request */

	if(reposition_children || Grequest) {
		ResizeSelf(newew,FALSE);  

		if(reposition_children) {
			needs_redisplay=TRUE;
		}

			/* nonexclusives sizes its own children */
		if(Grequest && !XtIsSubclass(parent,nonexclusivesWidgetClass)){ 
			newew->core.width=newew->checkBox.normal_width;
			newew->core.height=newew->checkBox.normal_height;
		}
	}

	return needs_redisplay;

}	/* SetValues */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 ************************************************************
 *
 *  CheckBoxHandler - this function is called by OPEN LOOK
 *	for requested events.
 *
 *********************function*header************************
 */

static void
CheckBoxHandler(w,ve)
	Widget		w;
	OlVirtualEvent	ve;
{

	Boolean activate=FALSE;

	switch(ve->xevent->type) {

		case EnterNotify:
		case LeaveNotify:
			if(ve->virtual_name==OL_SELECT
				|| ve->virtual_name==OL_MENU) {
			ve->consumed =
			PreviewState(w,ve->xevent,ve->virtual_name,activate);
			}
			break;

		case ButtonPress:

		(void) _OlUngrabPointer(w);

		if(ve->virtual_name==OL_SELECT) {
			ve->consumed =
			PreviewState(w,ve->xevent,ve->virtual_name,activate);
		}
			break;

		case ButtonRelease:

			ve->consumed =
			SetState(w,ve->xevent,ve->virtual_name,activate);

			break;
	}
} /* CheckBoxHandler */

/*
 ************************************************************
 *
 * MotionHandler: temporary code
 *
 *********************function*header************************
 */

static void MotionHandler (w, ve)
	Widget		w;
	OlVirtualEvent	ve;
{
	static Boolean activate = FALSE;

	switch (ve->virtual_name)
	{
		case OL_SELECT:
			ve->consumed = True;
			PreviewState(w,ve->xevent,ve->virtual_name,activate);
			break;
	}
} /* MotionHandler */

/*
 ************************************************************
 *
 *  PreviewState - this function is called on a select button 
 *	down event.  It previews the appearance of the button if it
 *	were to be selected or unselected.
 *
 *********************function*header************************
 */

static Boolean
PreviewState(w,event,vname,activate)
Widget w;
XEvent *event;
OlVirtualName vname;
Boolean activate;
{
	CheckBoxWidget cb = (CheckBoxWidget) w;
	Arg arg;
	XCrossingEvent *xce;
	XButtonEvent *xbe;
	XMotionEvent *xme;
	Position x,y;
	Boolean point_in_box=FALSE;
	Boolean toggle=FALSE;

	if(!XtIsSensitive(w) )
		return TRUE;

	if(!activate) {			/* filter out pointer crossings */

	if(event->type==EnterNotify || event->type==LeaveNotify) {
		xce = (XCrossingEvent *) &(event->xcrossing); 
		if(xce->mode!=NotifyNormal) return FALSE;
	}
	}

	(void) _OlUngrabPointer(w);

	if(!activate && event->type==ButtonPress) {
		xbe = (XButtonEvent *) &(event->xbutton);
		x= xbe->x;
		y= xbe->y;
		if(PointInBox(w,x,y,activate))
			toggle=TRUE;
	}
	else if(activate)
		toggle=TRUE;

	
	else if(event->type == MotionNotify) {
		xme = (XMotionEvent *) &(event->xmotion);
		x= xme->x;
		y= xme->y;
		point_in_box = PointInBox(w,x,y,activate); 

		if((point_in_box && (cb->checkBox.setvalue==cb->checkBox.set)) 
					||
		(!point_in_box && (cb->checkBox.setvalue!=cb->checkBox.set))) 
			toggle = TRUE;
	}

	else if(event->type == EnterNotify) {
		if((point_in_box && cb->checkBox.setvalue==cb->checkBox.set) ||
		(!point_in_box && cb->checkBox.setvalue!=cb->checkBox.set)) 
			toggle = TRUE;
	}
	else if(event->type == LeaveNotify) {
		if(cb->checkBox.setvalue!=cb->checkBox.set)
			toggle = TRUE;
	}

	if(toggle) {
		if(cb->checkBox.set==FALSE) {
			XtSetArg(arg, XtNset, TRUE);
			XtSetValues(w, &arg, 1);
			return TRUE;
		}
		XtSetArg(arg, XtNset, FALSE);
		XtSetValues(w, &arg, 1);
		return TRUE;
	}

	return FALSE;

}	/* PreviewState */

 /*
  ************************************************************
  *
  *  SetState - this function is called on a select button up
  *	event.  It invokes the users select and unselect callback(s) 
  *	and changes the state of the checkbox button from 
  *	set to unset or unset to set.
  *
  *********************function*header************************
  */
 
static Boolean 
SetState(w,event,vname,activate)
Widget w;
XEvent *event;
OlVirtualName vname;
Boolean activate;
{
 	CheckBoxWidget cb = (CheckBoxWidget) w;
 	Arg arg;
 	XButtonEvent *xbe;
 
 	if(!XtIsSensitive(w)) 
 		return;
 
	if(!activate && event->type!=ButtonRelease)
		return;

 	if(event->type==ButtonRelease) {
 		xbe = (XButtonEvent *) &(event->xbutton);
 		if(!PointInBox(w,(Position) xbe->x,(Position) xbe->y,activate))
 			return FALSE;
	}
 
	cb->checkBox.setvalue = cb->checkBox.set;

 		if(!cb->checkBox.set) {
 			if(cb->checkBox.unselect!=(XtCallbackList)NULL)
 				XtCallCallbacks(w,XtNunselect,(XtPointer)NULL);
 		} 
 						/* here if unset */
		else {

 			if(cb->checkBox.dim) {
 				XtSetArg(arg,XtNdim,FALSE);
 				XtSetValues(w,&arg,1);
 			}
 			if(cb->checkBox.select!=(XtCallbackList)NULL)
 				XtCallCallbacks(w,XtNselect,(XtPointer)NULL); 
 		} 

	return TRUE;

 }	/* SetState */ 

 /*
  *************************************************************************
  *
  * Public Procedures
  *
  ****************************public*procedures****************************
  */
