#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/window.c	1.8"
#endif

/******************************file*header********************************

    Description:
	This file contains the source code for desktop property sheet.
*/
						/* #includes go here	*/
#include <signal.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>
#include <Xm/Label.h>

#include <MGizmo/Gizmo.h>
#include <MGizmo/NumericGiz.h>
#include <DtWidget/SpinBox.h>

#include "error.h"
#include <exclusive.h>
#include <dtprop.h>
#include <list.h>
#include <misc.h>
#include <node.h>
#include <property.h>
#include <resource.h>
#include <wsm.h>
#include <xtarg.h>
#include "changebar.h"

#include <Dtm.h>		/* for Desktop */

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/

static void		Import (XtPointer closure);
static void		Export (XtPointer closure);
static void		Create (Widget work, XtPointer closure);
static ApplyReturn *	ApplyCB (Widget w, XtPointer closure);
static void		ResetCB (Widget w, XtPointer closure);
static void		FactoryCB (Widget w, XtPointer closure);
static void		Report(Exclusive *);
static void		TouchChangeBarDB ();


/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

#define OLFACTORY(x)		*(ADDR *)&(ol_factory[x].value)
#define SETVAR			GetXWINHome("adm/olsetvar")
#define FACTORY(x)		*(ADDR *)&(_factory[x].value)
#define CURRENT(x)		*(ADDR *)&(_current[x].value)
#ifdef GLOBAL
#undef GLOBAL
#endif
#define GLOBAL(x) \
	resource_value(&global_resources, _factory[x].name)

#define SETVAR			GetXWINHome("adm/olsetvar")
#define DT_AT_LOGIN		"DT"

static Arg window_args[] = {
/* Note: if you change the location of this element you must also */
/* change the index value in CreatePropertyPopup(). */
        {XmNorientation,	XmVERTICAL},
	{XmNnumColumns,		1},
};

static HelpInfo WindowHelp = {
	NULL, NULL, "DesktopMgr/winpref.hlp", NULL
};

Property windowProperty = {
	"Window",
	window_args,
	XtNumber (window_args),
	&WindowHelp,
	'\0',
	Import,
	Export,
	Create,
	ApplyCB,
	ResetCB,
	FactoryCB,
	0,
	0,
	0,
	0,
	0,
	NULL,
	NULL,
	False,
	NULL,
	420,
	265,
};

/* These define the offsets for the various control on this property sheet.
   That is, the controls appear in this order.
*/
static enum { 
	SHOW_ICONS, PARKING, SET_INPUT, WINDOW, ON_TOP,
	OL_PARKING, OL_SET_INPUT
};

/* Define strings used as resource values - eliminate these */

static char _true[]	= "True";
static char _false[]	= "False";

static char		_realEstate[] = "true";
static char		_clickToType[] = "false";

static char		_north   [] = "left top";
static char		_south   [] = "left bottom";
static char		_west    [] = "top left";
static char		_east    [] = "top right";

static char		_explicit[] = "explicit";
static char		_pointer [] = "pointer";

/*
   Note that _factory entries with NULL values are those with integer
   defaults and are initialized during "Import".
*/
static Resource _factory[] = {
	{ "Mwm*useIconBox",		_false },	/* SHOW_ICONS */
	{ "Mwm*iconPlacement",		_south },	/* PARKING */
	{ "Mwm*keyboardFocusPolicy",	_explicit },	/* SET_INPUT */
	{ "Mwm*focusAutoRaise",		_true },	/* WINDOW */
	{ "Mwm*secondariesOnTop",	_false },	/* ON_TOP */
/* Openlook resources */
	{ "*iconGravity",		_south },	/* OL_PARKING */
	{ "*pointerFocus",		_clickToType },	/* OL_SET_INPUT */
};
static List factory = LIST(Resource, _factory);

static Resource _current[] = {
	{ "Mwm*useIconBox",		NULL },
	{ "Mwm*iconPlacement", 		NULL },
	{ "Mwm*keyboardFocusPolicy",	NULL },
	{ "Mwm*focusAutoRaise",		NULL },
	{ "Mwm*secondariesOnTop",	NULL },
/* Openlook resources */
	{ "*iconGravity",		NULL },
	{ "*pointerFocus",		NULL },
};
static List current = LIST(Resource, _current);

/* Need to change the names under ExclusiveItems to come from resource file */

/* Define the exclusive setting for Always Keep Popups in Front area */
static ExclusiveItem _ontop[] = {
    { (XtArgVal)"",	(XtArgVal)_true,	False, False },
    { (XtArgVal)"",	(XtArgVal)_false,	False, False },
};
static List		ontop = LIST(ExclusiveItem, _ontop);
static Exclusive	Ontop =
    EXCLUSIVE("on_top", " ", &ontop, TouchChangeBarDB);

/* Define the exclusive setting for set input area */
static ExclusiveItem _input[] = {
    { (XtArgVal)"",	(XtArgVal)_explicit,	False, False },
    { (XtArgVal)"",	(XtArgVal)_pointer,	False, False },
};
static List		input = LIST(ExclusiveItem, _input);
static Exclusive	Input =
    EXCLUSIVE("set_input", "To Set Input Area", &input, TouchChangeBarDB);

/* Define the exclusive setting for Input Window area */
static ExclusiveItem _inputWin[] = {
    { (XtArgVal)"",	(XtArgVal)_false,	False, False },
    { (XtArgVal)"",	(XtArgVal)_true,	False, False },
};
static List		inputWin = LIST(ExclusiveItem, _inputWin);
static Exclusive	InputWin =
    EXCLUSIVE("input_window", "Bring Input", &inputWin, TouchChangeBarDB);

/* Define the exclusive setting for Show Icons area */
static ExclusiveItem _show[] = {
    { (XtArgVal)"",	(XtArgVal)_true,	False, False },
    { (XtArgVal)"",	(XtArgVal)_false,	False, False },
};
static List		show = LIST(ExclusiveItem, _show);
static Exclusive	Show =
    EXCLUSIVE("show_icons", "Show Icons:", &show, TouchChangeBarDB);

static ExclusiveItem	_parking[] = {
	{ (XtArgVal)"Top Left",    (XtArgVal)_north },
	{ (XtArgVal)"Bottom Left", (XtArgVal)_south },
	{ (XtArgVal)"Top Right",   (XtArgVal)_west  },
	{ (XtArgVal)"Bottom Right",  (XtArgVal)_east  },
};
static List		parking    = LIST(ExclusiveItem, _parking);
static Exclusive	Parking    = EXCLUSIVE("location", "Location:", &parking, TouchChangeBarDB);

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    Import- 
*/
static void
Import (XtPointer closure)
{
#define _SET_FACTORY(buf, default, index) \
    sprintf(buf, "%d", default); FACTORY(index) = buf;

    merge_resources(&global_resources, &factory);
    return;

#undef	_SET_FACTORY

}				/* Import */

/****************************procedure*header*****************************
    Export-
*/
static void
Export (XtPointer closure)
{
    ExclusiveItem *	p;
    char *		value;

#define _EXPORT(exclusive, index) \
    p = ResourceItem(exclusive, GLOBAL(index)); \
	CURRENT(index) = (p ? (ADDR)p->addr : FACTORY(index));

    _EXPORT ( &Input,		SET_INPUT );
    _EXPORT ( &Parking,		PARKING );
    _EXPORT ( &InputWin,	WINDOW );
    _EXPORT ( &Show,		SHOW_ICONS );
    _EXPORT ( &Ontop,		ON_TOP );

#undef	_EXPORT
    /* Enforce our notion of valid resource values ("re-Import"). */

    merge_resources(&global_resources, &current);

}				/* Export */

Widget
CreateToggle(Widget parent, Exclusive *e, Exclusive *top)
{
    Widget	label;

    label = CreateExclusive (parent, e, True);
    XtVaSetValues(
	XtParent(XtParent(e->w[0])),
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, XtParent(XtParent(top->w[0])),
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	NULL
    );
    return label;
}

/****************************procedure*header*****************************
    Create-
*/
static void
Create (Widget parent, XtPointer closure)
{
    Widget	topWidget;
    Widget	labels[5];

    _ontop[0].name	= (XtArgVal) pGGT(TXT_fixedString_yes);
    _ontop[1].name	= (XtArgVal) pGGT(TXT_fixedString_no);
    Ontop.current_item	= ResourceItem(&Ontop, CURRENT(ON_TOP));
    Ontop.string	= pGGT(TXT_fixedString_ONTOP);

    _input[0].name	= (XtArgVal) pGGT(TXT_fixedString_CLICK);
    _input[1].name	= (XtArgVal) pGGT(TXT_fixedString_MOVE);
    Input.current_item	= ResourceItem(&Input, CURRENT(SET_INPUT));
    Input.string	= pGGT(TXT_fixedString_INPUT);

    _inputWin[0].name	= (XtArgVal) pGGT(TXT_fixedString_AUTO1);
    _inputWin[1].name	= (XtArgVal) pGGT(TXT_fixedString_WHEN_SET1);
    InputWin.current_item= ResourceItem(&InputWin, CURRENT(WINDOW));
    InputWin.string	= pGGT(TXT_fixedString_WINDOW1);

    _parking[0].name	= (XtArgVal) pGGT(TXT_fixedString_top);
    _parking[1].name	= (XtArgVal) pGGT(TXT_fixedString_bottom);
    _parking[2].name	= (XtArgVal) pGGT(TXT_fixedString_left);
    _parking[3].name	= (XtArgVal) pGGT(TXT_fixedString_right);
    Parking.current_item= ResourceItem(&Parking,CURRENT(PARKING));
    Parking.string	= pGGT(TXT_fixedString_LOCATION);
    

    _show[0].name	= (XtArgVal) pGGT(TXT_fixedString_IN_ICON_BOX);
    _show[1].name	= (XtArgVal) pGGT(TXT_fixedString_ON_BG1);
    Show.current_item	= ResourceItem(&Show, CURRENT(SHOW_ICONS));
    Show.string		= pGGT(TXT_fixedString_SHOW_ICONS);

    XtVaSetValues(
	parent,
	XmNfractionBase, 12,
	NULL
    );

    labels[0] = CreateExclusive(parent, &Input, True);
    XtVaSetValues(
	XtParent(XtParent(Input.w[0])),
	XmNtopAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	NULL
    );
    
    labels[1] = CreateToggle(parent, &InputWin, &Input);
    labels[2] = CreateToggle(parent, &Ontop, &InputWin);
    labels[3] = CreateToggle(parent, &Parking, &Ontop);
    XtVaSetValues(
	XtParent(Parking.w[0]),
	XmNnumColumns,  2,
	XmNpacking,	XmPACK_COLUMN,
	NULL
    );
    labels[4] = CreateToggle(parent, &Show, &Parking);

    AlignLabels(labels, 5);
    
#undef SET_INIT
}				/* Create */

/****************************procedure*header*****************************
    Report-
*/
static void
Report(Exclusive * exc)
{
    if (exc->current_item)		/* ie, SELECT */
      {
	FooterMessage(exc->w[list_index(exc->items, exc->current_item)], 
	    pGGT(TXT_footerMsg_changeGUI), True);
      }
}

/****************************procedure*header*****************************
    SetVar-
*/
static int
SetVar(String var, String value)
{
    char	buf[80];
    int		status;
    void	(*f)( int );

    sprintf(buf, "%s %s %s", SETVAR, var, value);

    /* Normally we are ignoring/catching child-process death,
       but here we want system() to catch the clean-up.
    */
    f = signal(SIGCLD, SIG_DFL);
    status = system(buf);
    signal (SIGCLD, f);
    debug((stderr, "system(%s) returned %d\n", buf, status));

    return(status);
}

/****************************procedure*header*****************************
    ApplyCB()
*/
static ApplyReturn *
ApplyCB (Widget w, XtPointer closure)
{
    static ApplyReturn	ret = { APPLY_OK };
    extern void		DmApplyShowFullPath(Boolean);

#define _APPLY(exclusive, index) \
	CURRENT(index) = (ADDR)(exclusive).current_item->addr; \
	SetChangeBarState( \
	    exclusive.ChangeBarDB, 0, WSM_NONE, True,  exclusive.change \
	)

    _APPLY ( Input,	SET_INPUT );
    _APPLY ( Parking,	PARKING );
    _APPLY ( InputWin,	WINDOW );
    _APPLY ( Show,	SHOW_ICONS );
    _APPLY ( Ontop,	ON_TOP );
    
    /* Apply OpenLook resources */
    CURRENT(OL_PARKING) = CURRENT(PARKING);
    CURRENT(OL_SET_INPUT) = _clickToType;
    if (strcmp (CURRENT(SET_INPUT), _explicit) == 0) {
	CURRENT(OL_SET_INPUT) = _realEstate;
    }

    merge_resources(&global_resources, &current);

    /* Now apply settings to Desktop fields */

    return (&ret);
}				/* ApplyCB */

/****************************procedure*header*****************************
    ResetCB()
*/
static void
ResetCB (Widget w, XtPointer closure)
{
#define _RESET(exclusive, index) \
    SetExclusive(exclusive, ResourceItem(exclusive, CURRENT(index)), WSM_NONE) 

    _RESET ( &Parking,	PARKING );
    _RESET ( &Input,	SET_INPUT );
    _RESET ( &InputWin,	WINDOW );
    _RESET ( &Show,	SHOW_ICONS );
    _RESET ( &Ontop,	ON_TOP );

#undef	_RESET
}				/* ResetCB */

/****************************procedure*header*****************************
    FactoryCB-
*/
static void
FactoryCB (Widget w, XtPointer closure)
{
    int		value;

#define _FACTORY(exclusive, index) \
	SetExclusive( \
	    exclusive, \
	    ResourceItem(exclusive, FACTORY(index)), WSM_NORMAL \
	)

    _FACTORY ( &Parking,	PARKING );
    _FACTORY ( &Input,		SET_INPUT );
    _FACTORY ( &InputWin,	WINDOW );
    _FACTORY ( &Show,		SHOW_ICONS );
    _FACTORY ( &Ontop,		ON_TOP );

#undef	_FACTORY
}				/* FactoryCB */

/****************************procedure*header*****************************
    TouchChangeBarDB-
*/

static void TouchChangeBarDB()
{
	/* Check exclusives */
	if( (CheckExclusiveChangeBar(&Input)) ||
	    (CheckExclusiveChangeBar(&InputWin)) ||
	    (CheckExclusiveChangeBar(&Show)) ||
	    (CheckExclusiveChangeBar(&Ontop)) ||
	    (CheckExclusiveChangeBar(&Parking)))
	  {
	    windowProperty.changed = True;
	  }
	else
	  {
	    windowProperty.changed = False;
	  }
	
	TouchPropertySheets();
	
	return;
}				/* TouchChangeBarDB */
