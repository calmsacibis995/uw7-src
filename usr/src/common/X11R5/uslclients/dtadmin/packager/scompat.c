#ifndef NOIDENT
#pragma ident	"@(#)scompat.c	15.1"
#endif

#include <stdio.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include "packager.h"
#include <Gizmo/InputGizmo.h>
#include <Gizmo/ChoiceGizm.h>

static void	ScoCB (BLAH_BLAH_BLAH);
static void	ScompatCB (BLAH_BLAH_BLAH);

typedef enum { PropApply, PropReset, PropCancel, PropHelp } MenuIndex;

/*
 * "Uninstalled Application Type" popup
 */

static Setting scompatSettings;
static InputGizmo scompatInput = {
	NULL, "scompatInput", string_scompat, "",
	&scompatSettings, 20
};
static GizmoRec scompatArray[] = {
	{InputGizmoClass,	&scompatInput}
};
static MenuItems scompatItems[] = {
	{True, label_ok2,	mnemonic_ok},
	{True, label_reset,	mnemonic_reset},
	{True, label_cancel,	mnemonic_cancel},
	{True, label_help,	mnemonic_help, 0, helpCB, (char*)&HelpCompat},
	{0}
};
static MenuGizmo scompatMenu = {
	0, "scompatMenu", NULL, scompatItems,
	ScompatCB, NULL, CMD, OL_FIXEDROWS, 1, 0
};
static PopupGizmo scompatPopup = {
	0, "scompat", string_option_type,
	&scompatMenu, scompatArray, XtNumber (scompatArray)
};

void
SetFormatCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget	input;
	Time	t = CurrentTime;
	Boolean exists = TRUE;

	if (pr->scompatPopup == (NULL)) {
		pr->scompatPopup = CopyGizmo (PopupGizmoClass, &scompatPopup);
		CreateGizmo (
			pr->base->shell, PopupGizmoClass,
			pr->scompatPopup, NULL, 0
		);
		exists = FALSE;
	}
	input = (Widget)QueryGizmo (
		PopupGizmoClass, pr->scompatPopup,
		GetGizmoWidget, "scompatInput"
	);
	if (exists == FALSE)
	  XtVaSetValues (input, XtNstring, GGT(string_sco_default), (String)0);
	XtCallAcceptFocus (input, &t);
	XtUnmanageChild (pr->scompatPopup->message);
	MapGizmo (PopupGizmoClass, pr->scompatPopup);
}

static void
ScoCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	InputGizmo *	ip;

	ip = QueryGizmo (
		PopupGizmoClass, pr->scompatPopup, GetGizmoGizmo,
		"scompatInput"
	);
	XtManageChild (ip->captionWidget);
}

static void
ScompatCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget			shell = GetPopupGizmoShell (pr->scompatPopup);
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	MenuIndex		i = p->item_index;

	switch (i) {
		case PropApply: {
			ManipulateGizmo (
				PopupGizmoClass, pr->scompatPopup,
				GetGizmoValue
			);
			BringDownPopup(shell);
			break;
		}
		case PropReset: {
			ManipulateGizmo (
				PopupGizmoClass, pr->scompatPopup,
				ResetGizmoValue
			);
			ScoCB (NULL, NULL, NULL);
			break;
		}
		case PropCancel: {
			BringDownPopup(shell);
			break;
		}
		case PropHelp: {
			char *a;
			char *ScompatVariable();
			a = ScompatVariable ();
			break;
		}
	}
}

char *
ScompatVariable ()
{
	Setting *		type;
	Setting *		env;

	type = (Setting *)QueryGizmo (
		PopupGizmoClass, pr->scompatPopup,
		GetGizmoSetting, "appTypeChoice"
	);
	if ((int)type->current_value == 1) {
		return NULL;
	}
	env = (Setting *)QueryGizmo (
		PopupGizmoClass, pr->scompatPopup,
		GetGizmoSetting, "scompatInput"
	);
	return env->current_value;
}
