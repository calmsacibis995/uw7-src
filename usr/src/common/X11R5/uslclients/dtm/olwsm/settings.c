#pragma ident	"@(#)dtm:olwsm/settings.c	1.33"

#include <stdio.h>
#include <errno.h>
#include <signal.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/ModalGizmo.h>

#include <DesktopP.h>		/* for DtGetShellOfWidget */

#include <misc.h>
#include <node.h>
#include <list.h>
#include <slider.h>
#include <resource.h>
#include <xtarg.h>
#include <wsm.h>
#include "error.h"
#include "exclusive.h"
#include <property.h>

	/*
	 * Define the following if you want to support user changing
	 * the mouse acceleration.
	 */

/*
 * Local routines:
 */

static void		Import(XtPointer closure);
static void		Export(XtPointer closure);
static void		Create(Widget work, XtPointer closure);
static ApplyReturn *	ApplyCB(Widget w, XtPointer closure);
static void		ResetCB(Widget w, XtPointer closure);
static void		FactoryCB(Widget w, XtPointer closure);
static void		TouchChangeBarDB(void);

/*
 * Convenient macros:
 */

#define FACTORY(x)		*(ADDR *)&(_factory[x].value)
#define CURRENT(x)		*(ADDR *)&(_current[x].value)
#define ON_EXIT(x)		*(ADDR *)&(_on_exit[x].value)
#ifdef GLOBAL
#undef GLOBAL
#endif
#define GLOBAL(x) \
	resource_value(&global_resources, _factory[x].name)

/*
 * Local data:
 */


static HelpInfo MseSettingsHelp = {
	NULL, NULL, "DesktopMgr/moupref.hlp", NULL
};


Property		settingsProperty = {
	"Mouse Modifiers",
	NULL,
	0,
	&MseSettingsHelp,
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
	562,
	250,
};

#define DEFAULT_NUM_MOUSE_BTNS	2

static char		_two[] = "2";
static char		_three[] = "3";

static enum {
	F_NUM_BTNS, F_CLICK_INDEX, F_MWM_CLICK_INDEX
};
static enum {
	NUM_BTNS
};
static enum {
	CLICK_INDEX, MWM_CLICK_INDEX
};

static Resource		_ol_factory_3btn[] = {
	{ "*selectBtn",		"<Button1>" },
	{ "*adjustBtn",		"Ctrl <Button1>" },
	{ "*menuBtn",		"<Button3>" },
	{ "*pasteBtn",        	"<Button2>" },
	{ "*toggleBtn",        	"<adjustBtn>" },
	{ "*extendBtn",        	"Shift <selectBtn>" },
	{ "*duplicateBtn",	"Ctrl <selectBtn>" },
	{ "*linkBtn",        	"Ctrl Shift <selectBtn>" },
	{ "*constrainBtn",      "Shift Alt <selectBtn>" },
	{ "*cutBtn",        	"Alt <pasteBtn>" },
	{ "*copyBtn",        	"Alt <pasteBtn>" },
	{ "*panBtn",        	"Shift <pasteBtn>" },
	{ "*menuDefaultBtn",    "Alt <menuBtn>"},
};

static Resource		_ol_factory_2btn[] = {
	{ "*selectBtn",		"<Button1>" },
	{ "*adjustBtn",		"Ctrl <Button1>" },
	{ "*menuBtn",		"<Button3>" },
	{ "*pasteBtn",        	"Ctrl Alt <Button1>" },
	{ "*toggleBtn",        	"<adjustBtn>" },
	{ "*extendBtn",        	"Shift <selectBtn>" },
	{ "*duplicateBtn",	"Ctrl <selectBtn>" },
	{ "*linkBtn",        	"Ctrl Shift <selectBtn>" },
	{ "*constrainBtn",      "Shift Alt <selectBtn>" },
	{ "*cutBtn",        	"Alt <pasteBtn>" },
	{ "*copyBtn",        	"Ctrl <pasteBtn>" },
	{ "*panBtn",        	"Shift <pasteBtn>" },
	{ "*menuDefaultBtn",    "Shift Ctrl Alt <menuBtn>"},
};

static List	ol_factory_3btn		= LIST(Resource, _ol_factory_3btn);
static List	ol_factory_2btn		= LIST(Resource, _ol_factory_2btn);

static Resource		_factory[]	= {
#if DEFAULT_NUM_MOUSE_BTNS == 2
	{ "*numMouseBtns",		_two },
#else
	{ "*numMouseBtns",		_three },
#endif
	{ "*multiClickTime",		"500" },	/* milliseconds   */
	{ "Mwm*doubleClickTime",	"500" },	/* milliseconds   */
};
static List		factory		= LIST(Resource, _factory);

static Resource		_current[]	= {
	{ "*multiClickTime",		NULL },	/* milliseconds   */
	{ "Mwm*doubleClickTime",	NULL },	/* milliseconds   */
};
static List		current		= LIST(Resource, _current);

static Resource		_on_exit[]	= {
	{ "*numMouseBtns",		NULL },
};
static List		on_exit		= LIST(Resource, _on_exit);

/* Define "Number of Buttons" exclusive setting (top control) */
static ExclusiveItem _num_btns[] = {
	{ (XtArgVal)NULL,	(XtArgVal)_two		},
	{ (XtArgVal)NULL,	(XtArgVal)_three	},
};
static List num_btns = LIST(ExclusiveItem, _num_btns);
static Exclusive NumBtns = {
	True, "num_buttons", "Number of Mouse Buttons:",
	NULL, NULL, &num_btns, NULL,
	NULL, NULL, True, NULL, TouchChangeBarDB
};

static Slider	MClick	 = SLIDER(
	"multiClick", 100, 100, 1000, "0.1 sec", "1.0 sec", 25, 100
);

/**
 ** Import()
 **/

static void
Import (
	XtPointer		closure)
{
	merge_resources(&global_resources, &factory);
	return;
} /* Import */

/**
 ** Export()
 **/

static void
Export (
	XtPointer		closure)
{
	/* Get init value of resources from resource buffer */
	CURRENT(CLICK_INDEX)		= GLOBAL(F_CLICK_INDEX);
	CURRENT(MWM_CLICK_INDEX)	= GLOBAL(F_MWM_CLICK_INDEX);
	ON_EXIT(NUM_BTNS)		= GLOBAL(F_NUM_BTNS);
	/*
	 *	Enforce our notion of valid resource values ("re-Import").
	 */
	merge_resources(&global_resources, &current);
	merge_resources(&global_resources, &on_exit);
	XtSetMultiClickTime(
		XtDisplayOfObject(handleRoot), atoi(CURRENT(CLICK_INDEX))
	);  /* For dynamic */
	if (strcmp(ON_EXIT(NUM_BTNS), _two) == 0) {
		merge_resources(&global_resources, &ol_factory_2btn);
	}
	else {
		merge_resources(&global_resources, &ol_factory_3btn);
	}

	return;
} /* Export */

/**
 ** Create()
 **/

static void
Create (
	Widget			work,
	XtPointer		closure)
{
	Widget			label[2];

	NumBtns.string	= pGGT(TXT_fixedString_NumBtns);

	/* Insure name is consistent with resource value */
	_num_btns[0].name = (XtArgVal)_two;
	_num_btns[1].name = (XtArgVal)_three;

	/* Create num-btns control before creating controls below */
	NumBtns.current_item = ResourceItem(&NumBtns, ON_EXIT(NUM_BTNS));

	/* Change name to value to be displayed to user */
	_num_btns[0].name = (XtArgVal) pGGT(TXT_fixedString_numbtns_two);
	_num_btns[1].name = (XtArgVal) pGGT(TXT_fixedString_numbtns_three);
	label[0] = CreateExclusive(work, &NumBtns, True);

	MClick.sensitive	= True;
	MClick.slider_value	= atoi(CURRENT(CLICK_INDEX));

	MClick.string		= pGGT(TXT_fixedString_multiClick);
	MClick.min_label	= pGGT(TXT_minLabel_multiClick);
	MClick.max_label	= pGGT(TXT_maxLabel_multiClick);
	
	label[1] = CreateSlider(work, &MClick, True, TouchChangeBarDB);

	XtVaSetValues(
		XtParent(label[1]),
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, XtParent(label[0]),
		NULL
	);

	AlignLabels (label, 2);
	
	return;
} /* Create */

static void
Exit(XtPointer closure)
{
	merge_resources(&global_resources, &on_exit);
	/* Merge the old OpenLook mouse setting resources */
	if (strcmp(ON_EXIT(NUM_BTNS), _two) == 0) {
		merge_resources(&global_resources, &ol_factory_2btn);
	}
	else {
		merge_resources(&global_resources, &ol_factory_3btn);
	}
}

static void
CallbackPopdown(Widget w, XtPointer clientData, XtPointer callData)
{
	XtPopdown(DtGetShellOfWidget(w));
	((void(*)())clientData)();
}

static Notice	notice = {
	TXT_PREFERENCES_FOLDER,
	TXT_fixedString_change_numBtns,
	CallbackPopdown
};

/**
 ** ApplyCB()
 **/

static ApplyReturn *
ApplyCB (Widget	wid, XtPointer closure)
{
	static ApplyReturn	ret;
	Boolean			btnsChanged;
	Widget			w;

	btnsChanged = strcmp(
		ON_EXIT(NUM_BTNS), (ADDR)NumBtns.current_item->addr
	);

	/* Apply */
	CURRENT(CLICK_INDEX) = StringSliderValue(&MClick);
	SetChangeBarState(MClick.changebar, 0, WSM_NONE, False, MClick.change);
	CURRENT(MWM_CLICK_INDEX) = CURRENT(CLICK_INDEX);

	XtSetMultiClickTime(
		XtDisplayOfObject(handleRoot), atoi(CURRENT(CLICK_INDEX))
	);  /* For dynamic */
	
	merge_resources(&global_resources, &current);

	ON_EXIT(NUM_BTNS) = (ADDR)NumBtns.current_item->addr;
	SetChangeBarState (
		NumBtns.ChangeBarDB, 0, WSM_NONE, True, NumBtns.change
	);

	ret.reason = APPLY_OK;

	/* Warn the user of Button change taking effect at login */
	if (btnsChanged) {
		/* The value of numBtns has changed */
		ret.reason = APPLY_NOTICE;
		ret.u.notice = &notice;
		settingsProperty.exit = Exit;
	}

	return (&ret);
} /* ApplyCB */

/**
 ** ResetCB()
 **/

static void
ResetCB (
	Widget			w,
	XtPointer		closure)
{
	SetExclusive(
		&NumBtns, ResourceItem(&NumBtns, ON_EXIT(NUM_BTNS)),
		WSM_NONE
	);
#define _RESET(slider, value)	SetSlider(&slider, value, WSM_NONE)

	_RESET (MClick,    atoi(CURRENT(CLICK_INDEX)));

#undef	_RESET
	return;
} /* ResetCB */

/**
 ** FactoryCB()
 **/

static void
FactoryCB (
	Widget			w,
	XtPointer		closure)
{
	SetExclusive(
		&NumBtns, ResourceItem(&NumBtns, FACTORY(F_NUM_BTNS)),
		WSM_NORMAL
	);
#define _FACTORY(slider, value) \
	SetSlider (&slider, value, WSM_NORMAL)

	_FACTORY (MClick,    atoi(FACTORY(F_CLICK_INDEX)));

#undef	_FACTORY
	return;
} /* FactoryCB */

/****************************procedure*header*****************************
    TouchChangeBarDB-
*/

static void TouchChangeBarDB()
{
	int i;
	int j = 0;
	
	if( (CheckExclusiveChangeBar(&NumBtns)) )
	  {
		++j;
	  }
	if( (MClick.changebar->state) )
	  {
	  	++j;
	  }
	
	if(j > 0)
	  {
	    settingsProperty.changed = True;
	  }
	else
	  {
	    settingsProperty.changed = False;
	  }
	
	TouchPropertySheets();
	
	return;
}				/* TouchChangeBarDB */
