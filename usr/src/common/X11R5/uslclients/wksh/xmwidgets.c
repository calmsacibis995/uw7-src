
/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:xmwidgets.c	1.7"

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>

#include <Xm/ArrowB.h>
#include <Xm/ArrowBG.h>
#include <Xm/BulletinB.h>
#include <Xm/CascadeB.h>
#include <Xm/CascadeBG.h>
#include <Xm/Command.h>
#include <Xm/DialogS.h>
#include <Xm/DrawingA.h>
#include <Xm/DrawnB.h>
#include <Xm/FileSB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/MainW.h>
#include <Xm/MenuShell.h>
#include <Xm/MessageB.h>
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/SashP.h>
#include <Xm/Scale.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <Xm/SelectioB.h>
#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>

#include <FIconBox.h>

#include "wksh.h"
#include "xmksh.h"

wtab_t *set_up_w();

/*
 * Some widgets require fixups on some of the resources
 * because the resources don't get reported by XGetResourceList().
 */

const resfixup_t text_fixups[] = {
        { CONSTCHAR "pendingDelete", CONSTCHAR XtRBoolean, sizeof(Boolean) },
        { CONSTCHAR "selectThreshold", CONSTCHAR XtRInt, sizeof(int) },
        { CONSTCHAR "blinkRate", CONSTCHAR XtRInt, sizeof(int) },
        { CONSTCHAR "columns", CONSTCHAR XtRShort, sizeof(short) },
        { CONSTCHAR "cursorPositionVisible", CONSTCHAR XtRBoolean, sizeof(Boolean) },
        { CONSTCHAR "resizeHeight", CONSTCHAR XtRBoolean, sizeof(Boolean) },
        { CONSTCHAR "resizeWidth", CONSTCHAR XtRBoolean, sizeof(Boolean) },
        { CONSTCHAR "rows", CONSTCHAR XtRShort, sizeof(short) },
        { CONSTCHAR "wordWrap", CONSTCHAR XtRBoolean, sizeof(Boolean) },
        { CONSTCHAR "scrollHorizontal", CONSTCHAR XtRBoolean, sizeof(Boolean) },
        { CONSTCHAR "scrollLeftSide", CONSTCHAR XtRBoolean, sizeof(Boolean) },
        { CONSTCHAR "scrollTopSide", CONSTCHAR XtRBoolean, sizeof(Boolean) },
        { CONSTCHAR "scrollVertical", CONSTCHAR XtRBoolean, sizeof(Boolean) },
        { NULL }
};

const resfixup_t togglebuttongadget_fixups[] = {
        { CONSTCHAR "selectColor", CONSTCHAR XtRPixel, sizeof(Pixel) },
        { CONSTCHAR "selectInsensitivePixmap", CONSTCHAR XtRPixmap, sizeof(Pixmap) },
        { CONSTCHAR "selectPixmap", CONSTCHAR XtRPixmap, sizeof(Pixmap) },
        { CONSTCHAR "fillOnSelect", CONSTCHAR XtRBoolean, sizeof(Boolean) },
        { CONSTCHAR "visibleWhenOff", CONSTCHAR XtRBoolean, sizeof(Boolean) },
	{ NULL }
};

const resfixup_t form_con_fixups[] = {
        { CONSTCHAR "bottomWidget", CONSTCHAR XtRWidget, sizeof(Widget) },
        { CONSTCHAR "topWidget", CONSTCHAR XtRWidget, sizeof(Widget) },
        { CONSTCHAR "leftWidget", CONSTCHAR XtRWidget, sizeof(Widget) },
        { CONSTCHAR "rightWidget", CONSTCHAR XtRWidget, sizeof(Widget) },
	{ NULL }
};

const resfixup_t messagebox_fixups[] = {
        { CONSTCHAR "cancelButton", CONSTCHAR XtRWidget, sizeof(Widget) },
        { CONSTCHAR "defaultButton", CONSTCHAR XtRWidget, sizeof(Widget) },
        { NULL }
};

const resfixup_t fileselectionbox_fixups[] = {
        { CONSTCHAR "cancelButton", CONSTCHAR XtRWidget, sizeof(Widget) },
        { CONSTCHAR "defaultButton", CONSTCHAR XtRWidget, sizeof(Widget) },
        { NULL }
};

const resfixup_t mainwindow_fixups[] = {
        { CONSTCHAR "menuBar", CONSTCHAR XtRWidget, sizeof(Widget) },
        { CONSTCHAR "commandWindow", CONSTCHAR XtRWidget, sizeof(Widget) },
        { CONSTCHAR "clipWindow", CONSTCHAR XtRWidget, sizeof(Widget) },
        { CONSTCHAR "horizontalScrollBar", CONSTCHAR XtRWidget, sizeof(Widget) },
        { CONSTCHAR "verticalScrollBar", CONSTCHAR XtRWidget, sizeof(Widget) },
        { CONSTCHAR "workWindow", CONSTCHAR XtRWidget, sizeof(Widget) },
        { CONSTCHAR "messageWindow", CONSTCHAR XtRWidget, sizeof(Widget) },
        { NULL }
};

const resfixup_t scrolledwindow_fixups[] = {
        { CONSTCHAR "clipWindow", CONSTCHAR XtRWidget, sizeof(Widget) },
        { CONSTCHAR "horizontalScrollbar", CONSTCHAR XtRWidget, sizeof(Widget) },
        { CONSTCHAR "verticalScrollbar", CONSTCHAR XtRWidget, sizeof(Widget) },
        { NULL }
};

const resfixup_t bulletinboard_fixups[] = {
        { CONSTCHAR "defaultButton", CONSTCHAR XtRWidget, sizeof(Widget) },
        { CONSTCHAR "cancelButton", CONSTCHAR XtRWidget, sizeof(Widget) },
        { NULL }
};

void xminit_messagebox(), xminit_fileselectionbox();

classtab_t C[] = {
	/*
	 * NOTE: Keep these in alphabetical order
	 * because the initialize code below depends on
	 * the order.
	 */
	{ CONSTCHAR	"arrowButton",		NULL },
	{ CONSTCHAR	"arrowButtonGadget",	NULL },
	{ CONSTCHAR	"bulletinBoard",	NULL },
	{ CONSTCHAR	"cascadeButton",	NULL },
	{ CONSTCHAR	"cascadeButtonGadget",	NULL },
	{ CONSTCHAR	"command",	NULL },
	{ CONSTCHAR	"dialogShell",	NULL },
	{ CONSTCHAR	"drawingArea",	NULL },
	{ CONSTCHAR	"drawnButton",	NULL },
	{ CONSTCHAR	"fileSelectionBox",	xminit_fileselectionbox },
	{ CONSTCHAR	"form",	NULL },
	{ CONSTCHAR	"frame",	NULL },
	{ CONSTCHAR	"label",	NULL },
	{ CONSTCHAR	"labelGadget",	NULL },
	{ CONSTCHAR	"list",	NULL },
	{ CONSTCHAR	"mainWindow",	NULL },
	{ CONSTCHAR	"menuShell",	NULL },
	{ CONSTCHAR	"messageBox",	xminit_messagebox },
	{ CONSTCHAR	"panedWindow",	NULL},
	{ CONSTCHAR	"pushButton",	NULL },
	{ CONSTCHAR	"pushButtonGadget",	NULL },
	{ CONSTCHAR	"rowColumn",	NULL },
	{ CONSTCHAR	"scale",	NULL },
	{ CONSTCHAR	"scrollBar",	NULL },
	{ CONSTCHAR	"scrolledWindow",	NULL },
	{ CONSTCHAR	"selectionBox",	NULL },
	{ CONSTCHAR	"separator",	NULL },
	{ CONSTCHAR	"separatorGadget",	NULL },
	{ CONSTCHAR	"text",	NULL },
	{ CONSTCHAR	"textField",	NULL },
	{ CONSTCHAR	"toggleButton",	NULL },
	{ CONSTCHAR	"toggleButtonGadget",	NULL },
	{ CONSTCHAR	"topLevelShell",	NULL },
	{ CONSTCHAR	"flatIconBox",	NULL },
	{ NULL }
};

void
toolkit_init_widgets()
{
	register int i, n = 0;
	struct namnod *nam;

	if (C[0].class != NULL)
		return;
	/*
	 * NOTE: keep these in alphabetical order because
	 * the widget table above is in the same order.
	 */
	C[n++].class = xmArrowButtonWidgetClass;;
	C[n++].class = xmArrowButtonGadgetClass;;

	C[n].resfix = &bulletinboard_fixups[0];
	C[n++].class = xmBulletinBoardWidgetClass;

	C[n++].class = xmCascadeButtonWidgetClass;
	C[n++].class = xmCascadeButtonGadgetClass;
	C[n++].class = xmCommandWidgetClass;
	C[n++].class = xmDialogShellWidgetClass;
	C[n++].class = xmDrawingAreaWidgetClass;
	C[n++].class = xmDrawnButtonWidgetClass;

	C[n].resfix = &fileselectionbox_fixups[0];
	C[n++].class = xmFileSelectionBoxWidgetClass;

	C[n].confix = &form_con_fixups[0];
	C[n].resfix = &bulletinboard_fixups[0]; /* form's subclass of b'board */
	C[n++].class = xmFormWidgetClass;

	C[n++].class = xmFrameWidgetClass;
	C[n++].class = xmLabelWidgetClass;
	C[n++].class = xmLabelGadgetClass;
	C[n++].class = xmListWidgetClass;

	C[n].resfix = &mainwindow_fixups[0];
	C[n++].class = xmMainWindowWidgetClass;

	C[n++].class = xmMenuShellWidgetClass;

	C[n].resfix = &messagebox_fixups[0];
	C[n++].class = xmMessageBoxWidgetClass;

	C[n++].class = xmPanedWindowWidgetClass;
	C[n++].class = xmPushButtonWidgetClass;
	C[n++].class = xmPushButtonGadgetClass;
	C[n++].class = xmRowColumnWidgetClass;
	C[n++].class = xmScaleWidgetClass;
	C[n++].class = xmScrollBarWidgetClass;

	C[n].resfix = &scrolledwindow_fixups[0];
	C[n++].class = xmScrolledWindowWidgetClass;

	C[n++].class = xmSelectionBoxWidgetClass;
	C[n++].class = xmSeparatorWidgetClass;
	C[n++].class = xmSeparatorGadgetClass;

	C[n].resfix = &text_fixups[0];
	C[n++].class = xmTextWidgetClass;

	C[n++].class = xmTextFieldWidgetClass;
	C[n++].class = xmToggleButtonWidgetClass;

	C[n].resfix = &togglebuttongadget_fixups[0];
	C[n++].class = xmToggleButtonGadgetClass;

	C[n++].class = topLevelShellWidgetClass;

	C[n++].class = exmFlatIconBoxWidgetClass;
}

static char *
varmake(var, suffix)
char *var;
const char *suffix;
{
	static char v[128];
	register char *p;

	/*
	 * If the variable was an array, make sure to put the new
	 * suffix after the variable name but before the square braces.
	 */
	if ((p = (char *)strchr(var, '[')) != NULL) {
		*p = '\0';
		sprintf(v, CONSTCHAR "%s_%s[%s", var, suffix, &p[1]);
		*p = '[';
	} else {
		sprintf(v, CONSTCHAR "%s_%s", var, suffix);
	}
	return(v);
}

const char CallDataPrefix[] = "CALL_DATA_";

void
precb_set_str(field, str)
const char *field, *str;
{
	char buf[1024];

	sprintf(buf, CONSTCHAR "%s%s=%s", CallDataPrefix, field, str);
	env_set(buf);
}

void
precb_set_bool(field, bool)
const char *field;
Boolean bool;
{
	char buf[1024];

	sprintf(buf, CONSTCHAR "%s%s=%s", CallDataPrefix, field, bool ? CONSTCHAR "true" : CONSTCHAR "false");
	env_set(buf);
}

void
precb_set_int(field, i)
const char *field;
int i;
{
	char buf[1024];

	sprintf(buf, CONSTCHAR "%s%s=%d", CallDataPrefix, field, i);
	env_set(buf);
}

void
precb_set_type(field, type, arg)
const char *field, *type;
XtArgVal arg;
{
	char buf[1024];

	env_set(buf);
}

static void
childvarsetup(arg0, getfunc, wtab, var, suffix, classname, child)
char *arg0;
Widget (*getfunc)();
wtab_t *wtab;
char *var;
const char *suffix;
const char *classname;
unsigned char child;
{
	Widget wid;
	char newvar[256];
	char name[256];

	wid = (*getfunc)(wtab->w, child);
	sprintf(name, "%s_%s", wtab->wname, suffix);
	sprintf(newvar, "%s_%s", var, suffix);
	set_up_w(wid, wtab, newvar, name, str_to_class(arg0, classname));
}

void
xminit_messagebox(arg0, wtab, var)
char *arg0;
wtab_t *wtab;
char *var;
{

	/* Create convenience variables:
	 * 
	 * $<VAR>_CAN	cancel button widget id
	 * $<VAR>_DEF	default button widget id
	 * $<VAR>_HELP	help button widget id
	 * $<VAR>_LAB	message label widget id
	 * $<VAR>_OK	ok button widget id
	 * $<VAR>_SEP	separator widget id
	 * $<VAR>_SYM	sumbol label widget id
	 */
	childvarsetup(arg0, XmMessageBoxGetChild, wtab, var, CONSTCHAR "CAN", CONSTCHAR "pushButtonGadget", XmDIALOG_CANCEL_BUTTON);
	childvarsetup(arg0, XmMessageBoxGetChild, wtab, var, CONSTCHAR "DEF", CONSTCHAR "pushButtonGadget", XmDIALOG_DEFAULT_BUTTON);
	childvarsetup(arg0, XmMessageBoxGetChild, wtab, var, CONSTCHAR "DEF", CONSTCHAR "pushButtonGadget", XmDIALOG_DEFAULT_BUTTON);
	childvarsetup(arg0, XmMessageBoxGetChild, wtab, var, CONSTCHAR "HELP", CONSTCHAR "pushButtonGadget", XmDIALOG_HELP_BUTTON);
	childvarsetup(arg0, XmMessageBoxGetChild, wtab, var, CONSTCHAR "LAB", CONSTCHAR "label", XmDIALOG_MESSAGE_LABEL);
	childvarsetup(arg0, XmMessageBoxGetChild, wtab, var, CONSTCHAR "OK", CONSTCHAR "pushButtonGadget", XmDIALOG_OK_BUTTON);
	childvarsetup(arg0, XmMessageBoxGetChild, wtab, var, CONSTCHAR "SEP", CONSTCHAR "separatorGadget", XmDIALOG_SEPARATOR);
	childvarsetup(arg0, XmMessageBoxGetChild, wtab, var, CONSTCHAR "SYM", CONSTCHAR "label", XmDIALOG_SYMBOL_LABEL);
}

void
xminit_fileselectionbox(arg0, wtab, var)
char *arg0;
wtab_t *wtab;
char *var;
{
	/* Create convenience variables:
	 * 
	 * $<VAR>_APPLY	apply button widget id
	 * $<VAR>_CAN	cancel button widget id
	 * $<VAR>_DEF	default button widget id
	 * $<VAR>_FILTLAB	filter label widget id
	 * $<VAR>_FILTTXT	filter text widget id
	 * $<VAR>_HELP	help button widget
	 * $<VAR>_LIST	list widget
	 * $<VAR>_LISTLAB	list widget label
	 * $<VAR>_OK	ok button widget
	 * $<VAR>_SELLAB	selection label widget
	 * $<VAR>_TEXT	text widget
	 */
	childvarsetup(arg0, XmFileSelectionBoxGetChild, wtab, var, CONSTCHAR "APPLY", CONSTCHAR "pushButtonGadget", XmDIALOG_APPLY_BUTTON);
	childvarsetup(arg0, XmFileSelectionBoxGetChild, wtab, var, CONSTCHAR "CAN", CONSTCHAR "pushButtonGadget", XmDIALOG_CANCEL_BUTTON);
	childvarsetup(arg0, XmFileSelectionBoxGetChild, wtab, var, CONSTCHAR "DEF", CONSTCHAR "pushButtonGadget", XmDIALOG_DEFAULT_BUTTON);
	childvarsetup(arg0, XmFileSelectionBoxGetChild, wtab, var, CONSTCHAR "FILTLAB", CONSTCHAR "label", XmDIALOG_FILTER_LABEL);
	childvarsetup(arg0, XmFileSelectionBoxGetChild, wtab, var, CONSTCHAR "FILTTXT", CONSTCHAR "text", XmDIALOG_FILTER_TEXT);
	childvarsetup(arg0, XmFileSelectionBoxGetChild, wtab, var, CONSTCHAR "HELP", CONSTCHAR "pushButtonGadget", XmDIALOG_HELP_BUTTON);
	childvarsetup(arg0, XmFileSelectionBoxGetChild, wtab, var, CONSTCHAR "LIST", CONSTCHAR "list", XmDIALOG_LIST);
	childvarsetup(arg0, XmFileSelectionBoxGetChild, wtab, var, CONSTCHAR "LISTLAB", CONSTCHAR "label", XmDIALOG_LIST_LABEL);
	childvarsetup(arg0, XmFileSelectionBoxGetChild, wtab, var, CONSTCHAR "OK", CONSTCHAR "pushButtonGadget", XmDIALOG_OK_BUTTON);
	childvarsetup(arg0, XmFileSelectionBoxGetChild, wtab, var, CONSTCHAR "SELLAB", CONSTCHAR "label", XmDIALOG_SELECTION_LABEL);
	childvarsetup(arg0, XmFileSelectionBoxGetChild, wtab, var, CONSTCHAR "TEXT", CONSTCHAR "text", XmDIALOG_TEXT);
}

void
xminit_mainwindow(arg0, wtab, var)
char *arg0;
wtab_t *wtab;
char *var;
{
	/* Create convenience variables:
	 * 
	 * $<VAR>_SEP1	separator 1 widget
	 * $<VAR>_SEP2	separator 2 widget
	 */
	childvarsetup(arg0, XmMainWindowSep1, wtab, var, CONSTCHAR "SEP1", CONSTCHAR "separatorGadget", NULL);
	childvarsetup(arg0, XmMainWindowSep2, wtab, var, CONSTCHAR "SEP2", CONSTCHAR "separatorGadget", NULL);
}
