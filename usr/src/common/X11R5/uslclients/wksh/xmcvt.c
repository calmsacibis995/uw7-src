
/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:xmcvt.c	1.7"

/* X/OL includes */

#include <stdio.h>
#include <signal.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <setjmp.h>
#include <string.h>
#include <ctype.h>
#include "wksh.h"
#include "xmksh.h"

extern Widget Toplevel;

/*
 * Converters for Motif WKSH
 */

char *
XmStringToString(string)
XmString string;
{
	static char *buf;
	XmStringContext context;
	XmStringCharSet charset;
	XmStringDirection dir;
	char *text;
	Boolean separator = FALSE;

	XmStringInitContext(&context, string);
	buf = NULL;
	while (!separator) {
		if (XmStringGetNextSegment(context, &text, &charset, &dir,
			&separator)) {
			if (buf) {
				buf = XtRealloc(buf, strlen(buf) + strlen(text) + 2);
				strcat(buf, text);
			} else
				buf = strdup(text);
			XtFree(text);
		} else
			break;
	}
	XmStringFreeContext(context);
	return(buf);
}

void
WkshCvtXmStringToString(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	XmString string = ((XmString *)fval->addr)[0];
	char *buf;

	buf = XmStringToString(string);
	toval->addr = (caddr_t)buf;
	toval->size = buf ? strlen(buf) : 0;
}

void
WkshCvtKeySymToString(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	static char buf[2];

	buf[0] = (char) (((KeySym *)(fval->addr))[0]);
	buf[1] = '\0';
	toval->addr = (caddr_t)buf;
	toval->size = 1;
}

#if 0
/*
 * Convert an XmStringTable to a String.
 * In keeping with the standard CvtStringToStringTable function provided
 * by Motif, we will separate each item by a comma.  This of course does not
 * work properly if there is a comma in the data, but what can we do?
 * If the user wants full control, they can use ListOp to build an exact
 * copy of what they want.
 */

void
WkshCvtXmStringTableToString(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	/*
	 * same buffer will get used each time
	 */
	static char *buf = NULL;

	XmStringContext context;
	XmStringCharSet charset;
	XmStringDirection dir;
	char *text;
	Boolean separator = FALSE;
	XmStringTable stringtable = ((XmString *)fval->addr)[0];
	XmString string

	if (buf)
		buf[0] = '\0';

	XmStringInitContext(&context, string);
	while (!separator) {
		if (XmStringGetNextSegment(context, &text, &charset, &dir,
			&separator)) {
			if (buf) {
				buf = XtRealloc(buf, strlen(buf) + strlen(text) + 2);
				strcat(buf, text);
			} else
				buf = strdup(text);
			XtFree(text);
		} else
			break;
	}
	XmStringFreeContext(context);

	toval->addr = (caddr_t)buf;
	toval->size = buf ? strlen(buf) : 0;
}
#endif

/*
 * There are a number of resources in motif that consist of a few
 * named integer values.  Most such resources only have 2 to 4 values,
 * none have more than 7.  Because there are so few values, it's not
 * really worth the memory overhead to hash them.  Also, these kinds
 * of resources are rarely read by programmers (most are written but
 * not read).  So, we decided to go with a simple linear search converter
 * that takes as its first argument a table of the values allowed, and
 * as its second argument the number of items in the table.
 *
 * Note that we could not go with a simple indexing scheme because:
 * (1) the values are not guaranteed to be contiguous and (2) some
 * of the tables start with -1 instead of 0.
 *
 * If there are in the future many more items added to these lists, we
 * might want to convert to a hashing scheme or a binary search.
 */

void
WkshCvtNamedIntegerToString(args, nargs, fval, toval)
XrmValue *args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	/*
	 * same buffer will get used each time
	 */
	static char *ret = NULL;
	struct named_integer *table;
	int numtable;
	int value;
	register int i;

	value = ((int *)fval->addr)[0];

	if (*nargs != 1) {
		XtWarningMsg(CONSTCHAR "cvtNamedIntegerToString",
				CONSTCHAR "missingArgs",
				CONSTCHAR "XtToolkitError",
				CONSTCHAR "Converter requires an argument",
				NULL, 0);
		toval->addr = NULL;
		toval->size = 0;
		return;
	}
	table = (struct named_integer *)args[0].addr;
	numtable = args[0].size/sizeof(struct named_integer);

	for (i = 0; i < numtable; i++) {
		if (value = table[i].value) {
			toval->addr = (caddr_t)table[i].name;
			toval->size = strlen(table[i].name);
			return;
		}
	}
	XtWarningMsg(CONSTCHAR "cvtNamedIntegerToString",
			CONSTCHAR "nosuchvalue",
			CONSTCHAR "XtToolkitError",
			CONSTCHAR "could not find a name for that value",
			NULL, 0);
	toval->addr = NULL;
	toval->size = 0;
	return;
}

void
WkshCvtStringToNamedInteger(args, nargs, fval, toval)
XrmValue *args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	/*
	 * same buffer will get used each time
	 */
	static int ret;
	struct named_integer *table;
	int numtable;
	char *value;
	register int i;

	value = (String)fval->addr;

	if (*nargs != 1) {
		XtWarningMsg(CONSTCHAR "cvtStringToNamedInteger",
				CONSTCHAR "missingArgs",
				CONSTCHAR "XtToolkitError",
				CONSTCHAR "Converter requires an argument",
				NULL, 0);
		toval->addr = NULL;
		toval->size = 0;
		return;
	}
	table = (struct named_integer *)args[0].addr;
	numtable = args[0].size/sizeof(struct named_integer);

	for (i = 0; i < numtable; i++) {
		if (strcmp(value, table[i].name) == 0) {
			toval->addr = (caddr_t)&table[i].value;
			toval->size = sizeof(table[i].value);
			return;
		}
	}
	XtWarningMsg(CONSTCHAR "cvtStringToNamedInteger",
			CONSTCHAR "nosuchvalue",
			CONSTCHAR "XtToolkitError",
			CONSTCHAR "could not find a value for that name",
			NULL, 0);
	toval->addr = NULL;
	toval->size = 0;
	return;
}

struct named_integer NI_TraversalDirection[] = {
	{ "TRAVERSE_CURRENT",	XmTRAVERSE_CURRENT },
	{ "TRAVERSE_DOWN",	XmTRAVERSE_DOWN },
	{ "TRAVERSE_HOME",	XmTRAVERSE_HOME },
	{ "TRAVERSE_LEFT",	XmTRAVERSE_LEFT },
	{ "TRAVERSE_NEXT",	XmTRAVERSE_NEXT },
	{ "TRAVERSE_NEXT_TAB_GROUP",	XmTRAVERSE_NEXT_TAB_GROUP },
	{ "TRAVERSE_PREV",	XmTRAVERSE_PREV },
	{ "TRAVERSE_PREV_TAB_GROUP",	XmTRAVERSE_PREV_TAB_GROUP },
	{ "TRAVERSE_PREV_TAB_GROUP",	XmTRAVERSE_PREV_TAB_GROUP },
	{ "TRAVERSE_RIGHT",	XmTRAVERSE_RIGHT },
	{ "TRAVERSE_UP",	XmTRAVERSE_UP },
};

struct named_integer NI_ArrowDirection[] = {
	{ "ARROW_UP",	XmARROW_UP },
	{ "ARROW_DOWN",	XmARROW_DOWN },
	{ "ARROW_LEFT",	XmARROW_LEFT },
	{ "ARROW_RIGHT",XmARROW_RIGHT },
};

struct named_integer NI_DialogStyle[] = {
	{ "DIALOG_SYSTEM_MODAL",	XmDIALOG_SYSTEM_MODAL },
	{ "DIALOG_APPLICATION_MODAL",	XmDIALOG_APPLICATION_MODAL },
	{ "DIALOG_MODELESS",		XmDIALOG_MODELESS },
	{ "DIALOG_WORK_AREA",		XmDIALOG_WORK_AREA },
};

struct named_integer NI_ResizePolicy[] = {
	{ "RESIZE_NONE",	XmRESIZE_NONE },
	{ "RESIZE_ANY",		XmRESIZE_ANY },
	{ "RESIZE_GROW",	XmRESIZE_GROW },
};

struct named_integer NI_ShadowType[] = {
	{ "SHADOW_IN",		XmSHADOW_IN },
	{ "SHADOW_OUT",		XmSHADOW_OUT },
	{ "SHADOW_ETCHED_IN",	XmSHADOW_ETCHED_IN },
	{ "SHADOW_ETCHED_OUT",	XmSHADOW_ETCHED_OUT },
};

struct named_integer NI_Attachment[] = {
	{ "ATTACH_NONE",	XmATTACH_NONE },
	{ "ATTACH_FORM",	XmATTACH_FORM },
	{ "ATTACH_OPPOSITE_FORM",	XmATTACH_OPPOSITE_FORM },
	{ "ATTACH_WIDGET",	XmATTACH_WIDGET },
	{ "ATTACH_OPPOSITE_WIDGET",	XmATTACH_OPPOSITE_WIDGET },
	{ "ATTACH_POSITION",	XmATTACH_POSITION },
	{ "ATTACH_SELF",	XmATTACH_SELF },
};

struct named_integer NI_UnitType[] = {
	{ "PIXELS",	XmPIXELS },
	{ "100TH_MILLIMETERS",	Xm100TH_MILLIMETERS },
	{ "1000TH_INCHES",	Xm1000TH_INCHES },
	{ "100TH_POINTS",	Xm100TH_POINTS },
	{ "100TH_FONT_UNITS",	Xm100TH_FONT_UNITS },
};

struct named_integer NI_Alignment[] = {
	{ "ALIGNMENT_CENTER",	XmALIGNMENT_CENTER },
	{ "ALIGNMENT_END",	XmALIGNMENT_END },
	{ "ALIGNMENT_BEGINNING",	XmALIGNMENT_BEGINNING },
};

struct named_integer NI_LabelType[] = {
	{ "STRING",	XmSTRING },
	{ "PIXMAP",	XmPIXMAP }
};

struct named_integer NI_StringDirection[] = {
	{ "STRING_DIRECTION_L_TO_R",	XmSTRING_DIRECTION_L_TO_R },
	{ "STRING_DIRECTION_R_TO_L",	XmSTRING_DIRECTION_R_TO_L },
};

struct named_integer NI_SelectionPolicy[] = {
	{ "SINGLE_SELECT",	XmSINGLE_SELECT },
	{ "MULTIPLE_SELECT",	XmMULTIPLE_SELECT },
	{ "EXTENDED_SELECT",	XmEXTENDED_SELECT },
	{ "BROWSE_SELECT",	XmBROWSE_SELECT },
};

struct named_integer NI_ScrollBarDisplayPolicy[] = {
	{ "TOP_LEFT",	XmTOP_LEFT },
	{ "BOTTOM_LEFT",	XmBOTTOM_LEFT },
	{ "TOP_RIGHT",	XmTOP_RIGHT },
	{ "BOTTOM_RIGHT",	XmBOTTOM_RIGHT },
};

struct named_integer NI_ListSizePolicy[] = {
	{ "CONSTANT",	XmCONSTANT },
	{ "VARIABLE",	XmVARIABLE },
	{ "RESIZE_IF_POSSIBLE",	XmRESIZE_IF_POSSIBLE },
};

struct named_integer NI_EditMode[] = {
	{ "SINGLE_LINE_EDIT",	XmSINGLE_LINE_EDIT },
	{ "MULTI_LINE_EDIT",	XmMULTI_LINE_EDIT },
};

void
WkshRegisterNamedIntConverters()
{
	XtConvertArgRec args[1];

#define SETARGS(X) args[0].address_mode = XtAddress; args[0].address_id = (caddr_t)&X[0]; args[0].size = sizeof(X);

	SETARGS(NI_ArrowDirection);
	XtAddConverter(XmRArrowDirection, XtRString, 
		WkshCvtNamedIntegerToString, 
		args, 1);
	SETARGS(NI_ResizePolicy);
	XtAddConverter(XmRResizePolicy, XtRString, 
		WkshCvtNamedIntegerToString, 
		args, 1);
	SETARGS(NI_DialogStyle);
	XtAddConverter(XmRDialogStyle, XtRString, 
		WkshCvtNamedIntegerToString, 
		args, 1);
	SETARGS(NI_ShadowType);
	XtAddConverter(XmRShadowType, XtRString, 
		WkshCvtNamedIntegerToString, 
		args, 1);
	SETARGS(NI_TraversalDirection);
	XtAddConverter(XmRString, CONSTCHAR "TraversalDirection", 
		WkshCvtStringToNamedInteger, 
		args, 1);
	SETARGS(NI_Attachment);
	XtAddConverter(XmRAttachment, XtRString, 
		WkshCvtNamedIntegerToString, 
		args, 1);
	SETARGS(NI_UnitType);
	XtAddConverter(XmRUnitType, XtRString, 
		WkshCvtNamedIntegerToString, 
		args, 1);
	SETARGS(NI_Alignment);
	XtAddConverter(XmRAlignment, XtRString, 
		WkshCvtNamedIntegerToString, 
		args, 1);
	SETARGS(NI_LabelType);
	XtAddConverter(XmRLabelType, XtRString, 
		WkshCvtNamedIntegerToString, 
		args, 1);
	SETARGS(NI_StringDirection);
	XtAddConverter(XmRStringDirection, XtRString, 
		WkshCvtNamedIntegerToString, 
		args, 1);
	SETARGS(NI_SelectionPolicy);
	XtAddConverter(XmRSelectionPolicy, XtRString, 
		WkshCvtNamedIntegerToString, 
		args, 1);
	SETARGS(NI_ScrollBarDisplayPolicy);
	XtAddConverter(XmRScrollBarDisplayPolicy, XtRString, 
		WkshCvtNamedIntegerToString, 
		args, 1);
	SETARGS(NI_ListSizePolicy);
	XtAddConverter(XmRListSizePolicy, XtRString, 
		WkshCvtNamedIntegerToString, 
		args, 1);
	SETARGS(NI_EditMode);
	XtAddConverter(XmREditMode, XtRString, 
		WkshCvtNamedIntegerToString, 
		args, 1);
}
