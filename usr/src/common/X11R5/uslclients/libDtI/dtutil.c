/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef IDENT
#ident	"@(#)libDtI:dtutil.c	1.22.1.1"
#endif

/*
 * dtutil.c
 *
 */


#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <RectButton.h>
#include <Caption.h>
#include <PopupWindo.h>
#include <ControlAre.h>
#include <TextField.h>
#include <StaticText.h>
#include <FButtons.h>
#include <MenuShell.h>
#include <Notice.h>

#include "DtI.h"
#include "dtutil.h"

Arg  Dm__arg[ARGLIST_SIZE];

#define INPUT_LABEL	"File Name:"
#define TF_LENGTH	256
#define TF_WIDTH	120

static void
Dm__freeflatlist(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	free(client_data);
}

void
Dm__VerifyPromptCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	DmPromptPtr prompt = (DmPromptPtr)client_data;

	if (prompt->flag != False) {
		prompt->flag = False;
		*((Boolean *)call_data) = False;
	}
}

Widget
Dm__CreatePopupButtons(parent, list, count)
Widget parent;
XtArgVal *list;
int count;
{
	static char *fields[] = { XtNlabel, XtNmnemonic,
				  XtNselectProc, XtNclientData, XtNdefault};

	(void)Dm__CreateButtons(parent, list, count,
				 fields, XtNumber(fields), NULL, False, False);
}


/*
 * DmCreatePromptBox
 * ---------------
 * The DmCreatePromptBox function is a utility used to create a simple dialog
 * box.
 *
 */
int 
DmCreatePromptBox(Widget parent, DmPromptPtr prompt, DmPromptInfoPtr info,
		  XtPointer a_cd, XtPointer c_cd, DtAttrs options)
{
	Widget buttonarea;
	Widget promptarea;
	XtArgVal *p;

	if (!(options & DM_B_SHELL_CREATED)) {
		if (prompt->shell) {
			XRaiseWindow(XtDisplay(prompt->shell),
				     XtWindow(prompt->shell));
			return(0);
		}
	
		prompt->shell = XtCreatePopupShell(info->title, 
				   	popupWindowShellWidgetClass,
				   	parent, Dm__arg, 0);
	}

	if (prompt->button_items)
		free(prompt->button_items);

	XtSetArg(Dm__arg[0], XtNupperControlArea, &promptarea);
	XtSetArg(Dm__arg[1], XtNlowerControlArea, &buttonarea);
	XtGetValues(prompt->shell, Dm__arg, 2);

	prompt->input = DmCreateInputPrompt(promptarea, info->caption_label,
						prompt->current);

	if (!(options & DM_B_DONT_CREATE_BUTTONS)) {
		prompt->button_items = p = malloc(sizeof(XtArgVal) * 5 * 2);
		*p++ = (XtArgVal)(info->action_label);
		*p++ = (XtArgVal)(info->action_mnemonic);
		*p++ = (XtArgVal)(info->action_proc);
		*p++ = (XtArgVal)a_cd;
		*p++ = (XtArgVal)True;
		*p++ = (XtArgVal)(info->cancel_label);
		*p++ = (XtArgVal)(info->cancel_mnemonic);
		*p++ = (XtArgVal)(info->cancel_proc);
		*p++ = (XtArgVal)c_cd;
		*p++ = (XtArgVal)False;
		(void)Dm__CreatePopupButtons(buttonarea,prompt->button_items,2);
	}
	XtAddCallback(prompt->shell, XtNverify, Dm__VerifyPromptCB,
			(XtPointer)prompt);
	XtAddCallback(prompt->shell, XtNpopdownCallback, info->cancel_proc,
			(XtPointer)c_cd);

	if (!(options & DM_B_DONT_POPUP_SHELL))
		XtPopup(prompt->shell, XtGrabNone);
	return (0);
} /* end of DmCreatePromptBox */

void
DmDestroyPromptBox(prompt)
DmPromptPtr prompt;
{
	/* we are not freeing prompt->current and prompt->previous strings */
	if(prompt->shell) {
		XtDestroyWidget(prompt->shell);
		prompt->shell = NULL;

		XtFree(prompt->userdata);
		prompt->userdata = NULL;
	}
}

/*
 * DmCreateNotice
 * ------------
 * The DmCreateNotice function is a utility used to create a simple 
 * one or two-choice notice box of the form:
 *
*/
int
DmCreateNotice(name, parent, emanate, text, notice, f, fp, v)
char * name;
Widget  parent;
Widget  emanate;
char	*text;
DmNoticeRec  *notice;
PFV 	f;
XtPointer fp;
PFV 	v;
{
	Widget	controlarea, textfield, flat;
	XtArgVal *items, *p;
	int num_buttons;

	XtSetArg(Dm__arg[0], XtNemanateWidget, emanate);
	notice->shell = XtCreatePopupShell( name,
					    noticeShellWidgetClass,
					    parent,
					    Dm__arg,
					    1);
			
/*	XtAddCallback(notice->shell, XtNpopdownCallback, f, fp); */

	XtSetArg(Dm__arg[0], XtNtextArea, &notice->textarea);
	XtSetArg(Dm__arg[1], XtNcontrolArea, &controlarea);
	XtGetValues(notice->shell, Dm__arg, 2);

	XtSetArg(Dm__arg[0], XtNstring, text);
	XtSetValues(notice->textarea, Dm__arg, 1);

	if (notice->cancel_label != NULL)
		num_buttons = 2;
	else
		num_buttons = 1;
	p = items = malloc(sizeof(XtArgVal) * 4 * num_buttons);
	*p++ = (XtArgVal)(notice->action_label);
	*p++ = (XtArgVal)(*(notice->action_label));
	*p++ = (XtArgVal)f;
	if (num_buttons == 2) {
		*p++ = (XtArgVal)(notice->cancel_label);
		*p++ = (XtArgVal)(*(notice->cancel_label));
		*p++ = (XtArgVal)v;
	}
	flat = DmCreateButtons(controlarea, items, num_buttons, fp);
/*
	XtAddCallback(notice->shell,XtNpopdownCallback,Dm__freeflatlist,items);
*/
	return (0);
} /* end of DmCreateNotice */

/*
 * DmCreateInputPrompt
 * -----------------
 * The DmCreateInputPrompt function is a utility used to create a 
 * collection of caption and text field widgets suitable for use as a 
 * prompted input field.
 */

Widget
DmCreateInputPrompt(parent, label, string)
Widget parent;
char   *label;
char   *string;
{

	Widget	caption;
	Widget	textfield;
	int i;

	XtSetArg(Dm__arg[0], XtNlabel, label);
	XtSetArg(Dm__arg[1], XtNborderWidth, 0);
	XtSetArg(Dm__arg[2], XtNposition, OL_LEFT);
	XtSetArg(Dm__arg[3], XtNalignment, OL_CENTER);
	caption  = XtCreateManagedWidget(
			 	 	 "caption",
    			 	 	 captionWidgetClass, 
				 	 parent,
				 	 Dm__arg,
				 	 4);

	XtSetArg(Dm__arg[0], XtNborderWidth, 0);
	XtSetArg(Dm__arg[1], XtNmaximumSize, TF_LENGTH);
	XtSetArg(Dm__arg[2], XtNwidth,  TF_WIDTH);
	i = 3;
	if (string) {
		XtSetArg(Dm__arg[3], XtNstring, string);
		i++;
	}
	return(XtCreateManagedWidget("textfield",
    				     textFieldWidgetClass, 
				     caption,
				     Dm__arg,
				     i));

} /* end of DmCreateInputPrompt */

/*
 * DmCreateStaticText
 * -----------------
 * The DmCreateStaticText function is a utility used to create a 
 * collection of caption and static text widgets.
 */

Widget
DmCreateStaticText(parent, label, string)
Widget parent;
char   *label;
char   *string;
{
	Widget	caption;
	int i;

	XtSetArg(Dm__arg[0], XtNlabel, label);
	XtSetArg(Dm__arg[1], XtNborderWidth, 0);
	XtSetArg(Dm__arg[2], XtNposition, OL_LEFT);
	XtSetArg(Dm__arg[3], XtNalignment, OL_CENTER);
	caption  = XtCreateManagedWidget("caption",
    			 	 	 captionWidgetClass, 
				 	 parent,
				 	 Dm__arg,
				 	 4);

	if (string) {
		XtSetArg(Dm__arg[0], XtNstring, string);
		i = 1;
	}
	else
		i = 0;
	return(XtCreateManagedWidget("statictext",
    				     staticTextWidgetClass, 
				     caption,
				     Dm__arg,
				     i));
} /* end of DmCreateStaticText */

extern Widget
Dm__CreateButtons(Widget parent,
		  XtArgVal *items,
		  int count,
		  char **fields,
		  int nfields,
		  XtPointer client_data,
		  Boolean exclusive,
		  Boolean noneset)
{
	Widget flat;
	int i;

	XtSetArg(Dm__arg[0], XtNitemFields,    fields);
	XtSetArg(Dm__arg[1], XtNnumItemFields, nfields);
	XtSetArg(Dm__arg[2], XtNitems,         items);
	XtSetArg(Dm__arg[3], XtNnumItems,      count);
	XtSetArg(Dm__arg[4], XtNlayoutType,    OL_FIXEDROWS);
	XtSetArg(Dm__arg[5], XtNmeasure,       1);
	XtSetArg(Dm__arg[6], XtNclientData,    client_data);
	XtSetArg(Dm__arg[7], XtNexclusives,    exclusive);
	XtSetArg(Dm__arg[8], XtNnoneSet,       noneset);
	i = 9;
	if (exclusive != FALSE) {
		XtSetArg(Dm__arg[9], XtNbuttonType, OL_RECT_BTN);
		i++;
	}
	flat = XtCreateManagedWidget("buttons",
				flatButtonsWidgetClass,
				parent, Dm__arg, i);

	XtSetArg(Dm__arg[0], XtNdefault, True);
	OlFlatSetValues(flat, 0, Dm__arg, 1);
	return(flat);
}

Widget
DmCreateButtons(parent, items, count, client_data)
Widget parent;
XtArgVal *items;
int count;
XtPointer client_data;
{
	static char *fields[] = { XtNlabel, XtNmnemonic, XtNselectProc };

	return(Dm__CreateButtons(parent, items, count, fields, XtNumber(fields),
				client_data, False, False));
}

Widget
DmCreateExclusives(Widget parent,
		   char *label,
		   XtArgVal *items,
		   int count,
		   XtPointer client_data,
		   Boolean noneset)
{
	Widget caption;
	static char *fields[] = { XtNlabel, XtNset };

	XtSetArg(Dm__arg[0], XtNlabel, label);
	XtSetArg(Dm__arg[1], XtNborderWidth, 0);
	XtSetArg(Dm__arg[2], XtNposition, OL_LEFT);
	XtSetArg(Dm__arg[3], XtNalignment, OL_CENTER);
	caption  = XtCreateManagedWidget("caption", captionWidgetClass, 
				 	 parent, Dm__arg, 4);

	return(Dm__CreateButtons(caption, items, count, fields,
				 XtNumber(fields), client_data, True, noneset));
}

/* 
 * DmXYToIconLabel - Returns the label of an item in a flat icon box widget
 * given its x and y positions.  Value returned must not be freed by caller.
 * Returns NULL on failure.
 */
extern String
DmXYToIconLabel(Widget w, Position x, Position y)
{
	DmItemPtr itp;
	DmItemPtr items = NULL;
	Cardinal item_index = OlFlatGetItemIndex(w, x, y);
#if 0
	/* ignore icon sensitivity */
	Cardinal item_index = OlFlatGetIndex(w, x, y, True);
#endif

	if (item_index == OL_NO_ITEM) {
		fprintf(stderr,
		"DmXYToIconLabel: Couldn't get item index for %d %d\n", x, y);
		return(NULL);
	}
	XtSetArg(Dm__arg[0], XtNitems, &items);
	XtGetValues(w, Dm__arg, 1);

	if (items == NULL) {
		fprintf(stderr,
		"DmXYToIconLabel: Couldn't get item index for %d %d\n", x, y);
		return(NULL);
	}
	itp = items + item_index;
	if (ITEM_LABEL(itp))
		return((String)ITEM_LABEL(itp));
	else {
		String label;
		XtSetArg(Dm__arg[0], XtNlabel, &label);
		OlFlatGetValues(w, item_index, Dm__arg, 1);
		return(label);
	}

} /* end of DmXYToIconLabel */

/*
 * DmIconLabelToXY - Returns the x and y positions of an item in a flat icon 
 * box widget given its label.  Returns -1 on failure.
 */
extern int
DmIconLabelToXY(Widget w, String icon_name, Position *x, Position *y)
{
	DmItemPtr itp;
	DmItemPtr items = NULL;
	Cardinal nitems;
	Cardinal item_index;
	int i;

	if (w == NULL || icon_name == NULL) {
		fprintf(stderr, "DmIconLabelToXY: Couldn't get x and y\n");
		return(-1);
	}
	XtSetArg(Dm__arg[0], XtNitems, &items);
	XtSetArg(Dm__arg[1], XtNnumItems, &nitems);
	XtGetValues(w, Dm__arg, 2);

	if (items == NULL || nitems == 0) {
		fprintf(stderr, "DmconNameToXY: Couldn't get x and y\n");
		return(-1);
	}
	for (i=0, itp = items; i < nitems; i++, itp++) {
		if (strcmp(icon_name, (String)ITEM_LABEL(itp)) == 0)
		{
			Dimension width, height;
			OlFlatGetItemGeometry(w, i, x, y, &width, &height);
			return(0);
		}
	}
	return(-1);

} /* end of DmIconLabelToXY */

