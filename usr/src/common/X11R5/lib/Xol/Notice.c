/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)notice:Notice.c	1.80"
#endif

/*******************************file*header*******************************
    Description: Notice.c - OPEN LOOK(TM) Notice Widget
*/

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/NoticeP.h>
#include <Xol/ModalP.h>
#include <Xol/StaticText.h>
#include <Xol/ControlAre.h>

#define ClassName NoticeShell
#include <Xol/NameDefs.h>

/*
 * Convenient macros:
 */
#define NPART(w)		( &((NoticeShellWidget)(w))->notice_shell )

/**************************forward*declarations***************************

    Forward function definitions listed by category:
		1. Private functions
		2. Class   functions
		3. Action  functions
		4. Public  functions
 */
						/* private procedures */

static void PostSelectCB OL_ARGS((
    Widget nw,
    XtPointer closure,
    XtPointer call_data
));

						/* class procedures */
static void		Initialize OL_ARGS((
	Widget			request,
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));
static Boolean		SetValues OL_ARGS((
	Widget			current,
	Widget			request,
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));

/****************************widget*resources*****************************
 *
 * Notice Resources
 */

#define OFFSET(field)  XtOffsetOf(NoticeShellRec, notice_shell.field)

static XtResource resources[] =
  {
    { XtNcontrolArea, XtCControlArea, XtRWidget, sizeof(Widget),
	  OFFSET(control), XtRWidget, (XtPointer)NULL },

    { XtNtextArea, XtCTextArea, XtRWidget, sizeof(Widget),
	  OFFSET(text), XtRWidget, (XtPointer)NULL },
  };

#undef OFFSET

/***************************widget*class*record***************************
 *
 * Full class record constant
 */

NoticeShellClassRec noticeShellClassRec = {
  {
/* core_class fields		*/
    /* superclass		*/	(WidgetClass) &modalShellClassRec,
    /* class_name		*/	"NoticeShell",
    /* widget_size		*/	sizeof(NoticeShellRec),
    /* class_initialize		*/	NULL,
    /* class_part_init		*/	NULL,
    /* class_inited		*/	False,
    /* initialize		*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	XtInheritRealize,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	resources,
    /* num_resources		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	True,
    /* compress_exposure	*/	True,
    /* compress_enterleave	*/	True,
    /* visible_interest		*/	False,
    /* destroy			*/	NULL,
    /* resize			*/	NULL,
    /* expose			*/	XtInheritExpose,
    /* set_values		*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus		*/	XtInheritAcceptFocus,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	XtInheritTranslations,
    /* query geometry		*/	NULL
  },{
/* composite_class fields	*/
    /* geometry_manager		*/	XtInheritGeometryManager,
    /* change_managed		*/	XtInheritChangeManaged,
    /* insert_child		*/	XtInheritInsertChild,
    /* delete_child		*/	XtInheritDeleteChild,
    /* extension		*/	NULL
  },{
/* shell_class fields		*/
					NULL
  },{
/* WHShell_class fields		*/
					NULL
  },{
/* VendorShell_class fields	*/
					NULL
  },{
/* TransientShell_class fields	*/
					NULL
  },{
/* ModalShell_class fields	*/
					NULL
  },{
/* NoticeShell_class fields	*/
					NULL
  }
};

WidgetClass noticeShellWidgetClass = (WidgetClass)&noticeShellClassRec;


/***************************private*procedures****************************
 *
 * Private Functions
 *
 */

/******************************function*header****************************
 * PostSelectCB():  called when button pressed in Notice
 */
/* ARGSUSED */
static void
PostSelectCB OLARGLIST((nw, closure, call_data))
    OLARG(Widget, nw)
    OLARG(XtPointer, closure)
    OLGRA(XtPointer, call_data)
{
    XtPopdown((Widget)closure);
}  /* end of PostSelectCB() */

/*************************************************************************
 *
 * Class Procedures
 *
 */

/******************************function*header****************************
 * Initialize():  override any superclass resources of interest and
 *		establish own resource defaults.
 */

/* ARGSUSED */
static void
Initialize OLARGLIST((request, new, args, num_args))
    OLARG(Widget,		request)
    OLARG(Widget,		new)
    OLARG(ArgList,		args)
    OLGRA(Cardinal *,		num_args)
{
    NoticeShellPart *	nPart = NPART(new);
    Cardinal		m;
    MaskArg		mArgs[20];
    ArgList		mergedArgs;
    Cardinal		mergedCnt;

    /* postSelect callbacks must be added at Create time. */
    static XtCallbackRec popdown_cb[] = {
	{ NULL, NULL },		/* filled in below */
        { NULL, NULL }		/* delimiter */
    };

    /****************************************************************
     * CREATE FIXED CHILDREN
     */

    /* create the component parts: text area & control area.
     * some resources can be specified/overidden by the appl:
     * OL_OVERRIDE_PAIR: full control of these resources
     * OL_DEFAULT_PAIR: the appl has some say about these resources
     * OL_SOURCE_PAIR: don't care, include them on appl's behalf
     */

	m = 0;
	_OlSetMaskArg(mArgs[m], XtNalignment, OL_LEFT, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNlineSpace, 0, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNstring, NULL, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNstrip, True, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNwrap, True, OL_SOURCE_PAIR); m++;
	_OlSetMaskArg(mArgs[m], XtNwidth, OlPointToPixel(OL_HORIZONTAL, 350),
					OL_SOURCE_PAIR); m++;

	_OlComposeArgList(args, *num_args, mArgs, m, &mergedArgs, &mergedCnt);
	
	nPart->text = XtCreateManagedWidget("textarea",
					staticTextWidgetClass, new,
					mergedArgs, mergedCnt);
	XtFree((char *)mergedArgs);

    /*
     * create control area: will contain application's controls
     */

    popdown_cb[0].callback	= PostSelectCB;
    popdown_cb[0].closure	= (XtPointer)new;

    m = 0;
    _OlSetMaskArg(mArgs[m], XtNhPad, 0, OL_DEFAULT_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNhSpace, 0, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNsameSize, OL_NONE, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNvPad, 0, OL_DEFAULT_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNvSpace, 0, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNpostSelect, popdown_cb, OL_OVERRIDE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNlayoutType, OL_FIXEDROWS,
						 OL_DEFAULT_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNcenter, True, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNshadowThickness, 0, OL_DEFAULT_PAIR); m++;

    _OlComposeArgList(args, *num_args, mArgs, m, &mergedArgs, &mergedCnt);
	
    nPart->control = XtCreateManagedWidget("controlarea",
					   controlAreaWidgetClass,
					   new, mergedArgs, mergedCnt);
    XtFree((char *)mergedArgs);

}  /* end of Initialize() */

/******************************function*header****************************
    SetValues- check for attempts to set read-only resources.  Pass on
    certain resource changes to Text.
*/

/* ARGSUSED */
static Boolean
SetValues OLARGLIST((current, request, new, args, num_args))
    OLARG(Widget,	current)
    OLARG(Widget,	request)
    OLARG(Widget,	new)
    OLARG(ArgList,	args)
    OLGRA(Cardinal *,	num_args)
{
    Cardinal	m;
    MaskArg	mArgs[2];
    ArgList	mergedArgs;
    Cardinal	mergedCnt;
    Boolean	redisplay = False;

    if ((NPART(new)->text != NPART(current)->text) ||
      (NPART(new)->control != NPART(current)->control))
		OlVaDisplayWarningMsg(	XtDisplay(new),
					OleNinvalidResource,
					OleTsetValuesRO,
					OleCOlToolkitWarning,
					OleMinvalidResource_setValuesRO,
					XtName(new),
					OlWidgetToClassName(new));

    /* "forward" font & fontColor changes to the Static Text or caption */
    m = 0;
    _OlSetMaskArg(mArgs[m], XtNfont, NULL, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNfontColor, 0, OL_SOURCE_PAIR); m++;
    _OlComposeArgList(args, *num_args, mArgs, m, &mergedArgs, &mergedCnt);
	
    if (mergedCnt != 0)
    {
	XtSetValues(NPART(new)->text, mergedArgs, mergedCnt);
	XtFree((char *)mergedArgs);
    }
    return (redisplay);
}  /* end of SetValues() */
