
/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:olwidgets.c	1.10"

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/AbbrevMenu.h>
#include <Xol/BulletinBo.h>
#include <Xol/Caption.h>
#include <Xol/CheckBox.h>
#include <Xol/ControlAre.h>
#include <Xol/Exclusives.h>
#include <Xol/FCheckBox.h>
#include <Xol/FExclusive.h>
#include <Xol/FNonexclus.h>
#include <Xol/FooterPane.h>
#include <Xol/Form.h>
#include <Xol/Gauge.h>
#include <Xol/MenuButton.h>
#include <Xol/Menu.h>
#include <Xol/Nonexclusi.h>
#include <Xol/Notice.h>
#include <Xol/OblongButt.h>
#include <Xol/PopupWindo.h>
#include <Xol/RectButton.h>
#include <Xol/Scrollbar.h>
#include <Xol/ScrolledWi.h>
#include <Xol/ScrollingL.h>
#include <Xol/Slider.h>
#include <Xol/StaticText.h>
#include <Xol/Stub.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>

#ifdef MOOLIT
#include <Xol/AbbrevButt.h>
#include <Xol/FButtons.h>
#include <Xol/FList.h>
#include <Xol/Footer.h>
#include <Xol/Handles.h>
#include <Xol/IntegerFie.h>
#include <Xol/MenuShell.h>
#include <Xol/Panes.h>
#include <Xol/PopupMenu.h>
#include <Xol/RubberTile.h>
#include <Xol/StepField.h>

/* libDtI widgets */

#include <HyperText.h>
#include <FIconBox.h>
#endif

#include "wksh.h"
#include "olksh.h"

wtab_t *set_up_w();

void olinit_menu(), olinit_scrollinglist(), olinit_noticeshell(),
	olinit_popupwindowshell(), olinit_textfield(), olinit_stub();

/*
 * Form widgets require fixups on some of the constraints
 * because the resource types are listed simply as
 * "Pointer" rather than "Widget" making conversion
 * difficult.
 */
const resfixup_t form_fixups[] = {
	{ CONSTCHAR "xRefWidget", str_XtRWidget, sizeof(Widget) },
	{ CONSTCHAR "yRefWidget", str_XtRWidget, sizeof(Widget) },
	{ NULL }
}; 

const resfixup_t flat_fixups[] = {
	{ CONSTCHAR "selectProc", XtRCallbackProc, sizeof(XtCallbackProc) },
	{ CONSTCHAR "unselectProc", XtRCallbackProc, sizeof(XtCallbackProc) },
	{ CONSTCHAR "label", XtRString, sizeof(String) },
	{ CONSTCHAR "labelImage", CONSTCHAR "XImage", sizeof(XImage) },
	{ CONSTCHAR "set", XtRBoolean, sizeof(Boolean) },
	{ NULL }
}; 

#ifdef MOOLIT
const resfixup_t flatlist_fixups[] = {
	{ CONSTCHAR "selectProc", XtRCallbackProc, sizeof(XtCallbackProc) },
	{ CONSTCHAR "unselectProc", XtRCallbackProc, sizeof(XtCallbackProc) },
	{ CONSTCHAR "dblSelectProc", XtRCallbackProc, sizeof(XtCallbackProc) },
	{ CONSTCHAR "dropProc", XtRCallbackProc, sizeof(XtCallbackProc) },
	{ CONSTCHAR "label", str_XtRString, sizeof(String) },
	{ CONSTCHAR "labelImage", CONSTCHAR "XImage", sizeof(XImage) },
	{ CONSTCHAR "set", XtRBoolean, sizeof(Boolean) },
	{ NULL }
}; 
const resfixup_t flatbuttons_fixups[] = {
	{ CONSTCHAR "selectProc", XtRCallbackProc, sizeof(XtCallbackProc) },
	{ CONSTCHAR "unselectProc", XtRCallbackProc, sizeof(XtCallbackProc) },
	{ CONSTCHAR "dropProc", XtRCallbackProc, sizeof(XtCallbackProc) },
	{ CONSTCHAR "label", str_XtRString, sizeof(String) },
	{ CONSTCHAR "labelImage", CONSTCHAR "XImage", sizeof(XImage) },
	{ CONSTCHAR "labelJustify", CONSTCHAR "OlDefine", sizeof(OlDefine) },
	{ CONSTCHAR "recomputeSize", XtRBoolean, sizeof(Boolean) },
	{ CONSTCHAR "set", XtRBoolean, sizeof(Boolean) },
	{ CONSTCHAR "popupMenu", str_XtRWidget, sizeof(Widget) },
	{ NULL }
}; 
#endif

const resfixup_t controlarea_fixups[] = {
	{ CONSTCHAR "changeBar", str_XtROlChangeBarDefine, sizeof(OlDefine) },
	{ NULL }
};

const resfixup_t button_fixups[] = {
	{ CONSTCHAR "labelImage", CONSTCHAR "XImage", sizeof(XImage) },
	{ NULL }
};

const resfixup_t scrolledwindow_fixups[] = {
	{ CONSTCHAR XtNalignHorizontal, str_XtROlDefineInt, sizeof(int) },
	{ CONSTCHAR XtNalignVertical, str_XtROlDefineInt, sizeof(int) },
	{ NULL }
};

/*
 * Scrolling Lists use several resources from within the ListPane
 * widget class that never appear in its resource list.
 */
const resfixup_t scrollinglist_fixups[] = {
	{ CONSTCHAR XtNapplAddItem,	str_Function,	sizeof(char *) },
	{ CONSTCHAR XtNapplDeleteItem,	str_Function,	sizeof(char *) },
	{ CONSTCHAR XtNapplEditClose,	str_Function,	sizeof(char *) },
	{ CONSTCHAR XtNapplEditOpen,	str_Function,	sizeof(char *) },
	{ CONSTCHAR XtNapplTouchItem,	str_Function,	sizeof(char *) },
	{ CONSTCHAR XtNapplUpdateView,	str_Function,	sizeof(char *) },
	{ CONSTCHAR XtNapplViewItem,	str_Function,	sizeof(char *) },
	{ CONSTCHAR XtNbackground,	str_XtRPixel,	sizeof(Pixel) },
	{ CONSTCHAR XtNbackgroundPixmap,str_XtRPixmap,	sizeof(Pixmap) },
	{ CONSTCHAR XtNborderColor,	str_XtRPixel,	sizeof(Pixel) },
	{ CONSTCHAR XtNborderPixmap,	str_XtRPixmap,	sizeof(Pixmap) },
	{ CONSTCHAR XtNborderWidth,	str_XtRDimension,	sizeof(Dimension) },
	{ CONSTCHAR XtNfont,		str_XtRFontStruct,	sizeof(XFontStruct *) },
	{ CONSTCHAR XtNfontColor,	str_XtRPixel,	sizeof(Pixel) },
	{ CONSTCHAR XtNforeground,	str_XtRPixel,	sizeof(Pixel) },
	{ CONSTCHAR XtNmaximumSize,	str_XtRInt,	sizeof(int) },
	{ CONSTCHAR XtNrecomputeWidth,	str_XtRBoolean,	sizeof(Boolean) },
	{ CONSTCHAR XtNselectable,	str_XtRBoolean,	sizeof(Boolean) },
	{ CONSTCHAR XtNstring,		str_XtRString,	sizeof(String) },
	{ CONSTCHAR XtNtextField,	str_XtRWidget,	sizeof(Widget) },
	{ CONSTCHAR XtNtraversalOn,	str_XtRBoolean,	sizeof(Boolean) },
	{ CONSTCHAR XtNviewHeight,	str_XtRCardinal,	sizeof(Cardinal) },

	{ NULL }
}; 

/*
 * All widgets that are subclasses of vendorShell have these problems
 * due to the mechanism employed by open look to define them.
 */

const resfixup_t vendorshell_fixups[] = {
#ifdef SVR4_0
	{ CONSTCHAR XtNiconPixmap, str_XtRPixmap, sizeof(Pixmap) },
#endif
	{ CONSTCHAR XtNpushpin, str_XtROlDefine, sizeof(OlDefine) },
	{ CONSTCHAR XtNresizeCorners, str_XtRBoolean, sizeof(Boolean) },
	{ CONSTCHAR XtNwindowHeader, str_XtRBoolean, sizeof(Boolean) },
	{ CONSTCHAR XtNmenuButton, str_XtRBoolean, sizeof(Boolean) },
	{ CONSTCHAR XtNmenuType, str_XtROlDefine, sizeof(OlDefine) },
	{ CONSTCHAR XtNwmProtocol, XtRCallback, sizeof(XtCallbackList) },
	{ NULL }
};

/*
 * A usual, resources of type Widget are incorrectly listed as
 * type XtPointer.
 */
const resfixup_t popupwindowshell_fixups[] = {
	{ CONSTCHAR XtNlowerControlArea, str_XtRWidget,	sizeof(Widget) },
	{ CONSTCHAR XtNupperControlArea, str_XtRWidget,	sizeof(Widget) },
	{ CONSTCHAR XtNfooterPanel,	 str_XtRWidget,	sizeof(Widget) },

	/* NOTE: THE FOLLOWING WERE COPIED FROM vendorshell_fixups ABOVE */

	{ CONSTCHAR XtNiconPixmap, str_XtRPixmap, sizeof(Pixmap) },
	{ CONSTCHAR XtNpushpin, str_XtROlDefine, sizeof(OlDefine) },
	{ CONSTCHAR XtNresizeCorners, str_XtRBoolean, sizeof(Boolean) },
	{ CONSTCHAR XtNwindowHeader, str_XtRBoolean, sizeof(Boolean) },
	{ CONSTCHAR XtNmenuButton, str_XtRBoolean, sizeof(Boolean) },
	{ CONSTCHAR XtNmenuType, str_XtROlDefine, sizeof(OlDefine) },
	{ CONSTCHAR XtNwmProtocol, XtRCallback, sizeof(XtCallbackList) },
	{ NULL }
};


/*
 * Callback tables allow one to provide the user with call_data.  There
 * should be an table for each class that has at least one callback
 * resource that uses call_data.  For each of these, a precb_proc()
 * would typically set up a variable of the form $CALL_DATA_<fieldname>
 * for each field of the call_data.  The procedure is passed the call_data
 * as its only argument.
 */

void textfield_precb_verification(), manager_precb_consumeevent(),
	manager_postcb_consumeevent(), scrollinglist_precb_delete(),
	scrollinglist_precb_makecurrent(), slider_precb_slidermoved(),
	scrollbar_precb_slidermoved(), scrollbar_postcb_slidermoved(),
	vendorshell_precb_wmprotocol();

const callback_tab_t textfield_cbtab[] = {
	{ CONSTCHAR XtNverification, textfield_precb_verification, NULL },
	{ CONSTCHAR XtNconsumeEvent, manager_precb_consumeevent, manager_postcb_consumeevent },
	{ NULL }
};

const callback_tab_t vendorshell_cbtab[] = {
	{ CONSTCHAR XtNconsumeEvent, manager_precb_consumeevent, manager_postcb_consumeevent },
	{ CONSTCHAR XtNwmProtocol, vendorshell_precb_wmprotocol, NULL },
	{ NULL }
};

const callback_tab_t scrollinglist_cbtab[] = {
	{ CONSTCHAR XtNuserMakeCurrent, scrollinglist_precb_makecurrent, NULL },
	{ CONSTCHAR XtNuserDeleteItems, scrollinglist_precb_delete, NULL },
	{ CONSTCHAR XtNconsumeEvent, manager_precb_consumeevent, manager_postcb_consumeevent },
	{ NULL, NULL}
};

const callback_tab_t slider_cbtab[] = {
	{ CONSTCHAR XtNsliderMoved, slider_precb_slidermoved, NULL },
	{ CONSTCHAR XtNconsumeEvent, manager_precb_consumeevent, manager_postcb_consumeevent },
	{ NULL, NULL}
};

const callback_tab_t scrollbar_cbtab[] = {
	{ CONSTCHAR XtNsliderMoved, scrollbar_precb_slidermoved, scrollbar_postcb_slidermoved },
	{ NULL, NULL}
};

const callback_tab_t manager_cbtab[] = {
	{ CONSTCHAR XtNconsumeEvent, manager_precb_consumeevent, manager_postcb_consumeevent },
	{ NULL }
};

#ifdef MOOLIT
void hypertext_precb_select();

const callback_tab_t hypertext_cbtab[] = {
	{ CONSTCHAR XtNselect, hypertext_precb_select, NULL },
	{ NULL, NULL}
};
#endif


classtab_t C[] = {
	/*
	 * NOTE: Keep these in alphabetical order
	 * because the initialize code below depends on
	 * the order.
	 */
	{ CONSTCHAR	"abbrevMenuButton",	olinit_menu },
	{ CONSTCHAR	"applicationShell",	NULL },
	{ CONSTCHAR	"bulletinBoard",	NULL },
	{ CONSTCHAR	"caption",		NULL },
	{ CONSTCHAR	"checkBox",		NULL },
	{ CONSTCHAR	"controlArea",		NULL },
	{ CONSTCHAR	"exclusives",		NULL },
	{ CONSTCHAR	"flatCheckBox",		NULL },
	{ CONSTCHAR	"flatExclusives",	NULL },
	{ CONSTCHAR	"flatNonexclusives",	NULL },
	{ CONSTCHAR	"footerPanel",		NULL },
	{ CONSTCHAR	"form",			NULL },
	{ CONSTCHAR	"gauge",		NULL },
	{ CONSTCHAR	"menuButton",		olinit_menu },
	{ CONSTCHAR	"menuButtonGadget",	olinit_menu },
	{ CONSTCHAR	"menuShell",		olinit_menu },
	{ CONSTCHAR	"nonexclusives",	NULL },
	{ CONSTCHAR	"noticeShell",		olinit_noticeshell },
	{ CONSTCHAR	"oblongButton",		NULL },
	{ CONSTCHAR	"oblongButtonGadget",		NULL },
	{ CONSTCHAR	"popupWindowShell",	olinit_popupwindowshell },
	{ CONSTCHAR	"rectButton",		NULL },
	{ CONSTCHAR	"scrollbar",		NULL },
	{ CONSTCHAR	"scrolledWindow",	NULL },
	{ CONSTCHAR	"scrollingList",	olinit_scrollinglist },
	{ CONSTCHAR	"slider",		NULL },
	{ CONSTCHAR	"staticText",		NULL },
	{ CONSTCHAR	"stub",			olinit_stub },
	{ CONSTCHAR	"textEdit",		NULL },
	{ CONSTCHAR	"textField",		olinit_textfield },
	{ CONSTCHAR	"topLevelShell",	NULL },
	{ CONSTCHAR	"transientShell",	NULL },
#ifdef MOOLIT
        /*
         * For simplicity, the new MOOLIT widgets are all
         * together here at the end in alphabetical order.
         */
        { CONSTCHAR     "abbreviatedButton",         NULL },
        { CONSTCHAR     "flatButtons",          NULL },
        { CONSTCHAR     "flatGraph",            NULL },
        { CONSTCHAR     "flatIconBox",          NULL },
        { CONSTCHAR     "flatList",             NULL },
        { CONSTCHAR     "footer",               NULL },
        { CONSTCHAR     "handles",              NULL },
        { CONSTCHAR     "hyperText",            NULL },
        { CONSTCHAR     "integerField",         NULL },
        { CONSTCHAR     "modalShell",           NULL },
        { CONSTCHAR     "panes",                NULL },
        { CONSTCHAR     "popupMenuShell",       NULL },
        { CONSTCHAR     "rubberTile",           NULL },
        { CONSTCHAR     "stepField",            NULL },
#endif
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
	C[n++].class = abbrevMenuButtonWidgetClass;

	C[n].resfix = vendorshell_fixups;
	C[n].cbtab = &vendorshell_cbtab[0];
	C[n++].class = applicationShellWidgetClass;

	C[n].cbtab = &manager_cbtab[0];
	C[n++].class = bulletinBoardWidgetClass;

	C[n].cbtab = &manager_cbtab[0];
	C[n++].class = captionWidgetClass;

	C[n].cbtab = &manager_cbtab[0];
	C[n++].class = checkBoxWidgetClass;

	C[n].cbtab = &manager_cbtab[0];
	C[n].confix = controlarea_fixups;
	C[n++].class = controlAreaWidgetClass;

	C[n].cbtab = &manager_cbtab[0];
	C[n++].class = exclusivesWidgetClass;

	C[n].resfix = &flat_fixups[0];
	C[n].cbtab = &manager_cbtab[0];
	C[n++].class = flatCheckBoxWidgetClass;

	C[n].resfix = &flat_fixups[0];
	C[n].cbtab = &manager_cbtab[0];
	C[n++].class = flatExclusivesWidgetClass;

	C[n].resfix = &flat_fixups[0];
	C[n].cbtab = &manager_cbtab[0];
	C[n++].class = flatNonexclusivesWidgetClass;

	C[n].cbtab = &manager_cbtab[0];
	C[n++].class = footerPanelWidgetClass;

	C[n].cbtab = &manager_cbtab[0];
	C[n].confix = form_fixups;
	C[n++].class = formWidgetClass;

	C[n++].class = gaugeWidgetClass;

	C[n].cbtab = &manager_cbtab[0];
	C[n++].class = menuButtonWidgetClass;

	C[n].cbtab = &manager_cbtab[0];
	C[n++].class = menuButtonGadgetClass;

	C[n].cbtab = &vendorshell_cbtab[0];
	C[n].resfix = vendorshell_fixups;
	C[n++].class = menuShellWidgetClass;

	C[n].cbtab = &manager_cbtab[0];
	C[n++].class = nonexclusivesWidgetClass;

	C[n].cbtab = &vendorshell_cbtab[0];
	C[n].resfix = vendorshell_fixups;
	C[n++].class = noticeShellWidgetClass;

	C[n].cbtab = &manager_cbtab[0];
	C[n].resfix = button_fixups;
	C[n++].class = oblongButtonWidgetClass;

	C[n].cbtab = &manager_cbtab[0];
	C[n].resfix = button_fixups;
	C[n++].class = oblongButtonGadgetClass;

	C[n].cbtab = &manager_cbtab[0];
	C[n].resfix = popupwindowshell_fixups;
	C[n++].class = popupWindowShellWidgetClass;

	C[n].cbtab = &manager_cbtab[0];
	C[n].resfix = button_fixups;
	C[n++].class = rectButtonWidgetClass;

	C[n].cbtab = &scrollbar_cbtab[0];
	C[n++].class = scrollbarWidgetClass;

	C[n].cbtab = &manager_cbtab[0];
	C[n].resfix = scrolledwindow_fixups;
	C[n++].class = scrolledWindowWidgetClass;

	C[n].cbtab = &scrollinglist_cbtab[0];
	C[n].resfix = scrollinglist_fixups;
	C[n++].class = scrollingListWidgetClass;

	C[n].cbtab = &slider_cbtab[0];
	C[n++].class = sliderWidgetClass;

	C[n].cbtab = &manager_cbtab[0];
	C[n++].class = staticTextWidgetClass;

	C[n++].class = stubWidgetClass;

	C[n].cbtab = &manager_cbtab[0];
	C[n++].class = textEditWidgetClass;

	C[n].cbtab = &textfield_cbtab[0];
	C[n++].class = textFieldWidgetClass;

	C[n].cbtab = &vendorshell_cbtab[0];
	C[n].resfix = vendorshell_fixups;
	C[n++].class = topLevelShellWidgetClass;

	C[n++].class = transientShellWidgetClass;
#ifdef MOOLIT
        C[n++].class = abbreviatedButtonWidgetClass;

        C[n].resfix = &flatbuttons_fixups[0];
        C[n++].class = flatButtonsWidgetClass;

        C[n++].class = flatGraphWidgetClass;
        C[n++].class = flatIconBoxWidgetClass;

        C[n].resfix = &flatlist_fixups[0];
        C[n++].class = flatListWidgetClass;

        C[n++].class = footerWidgetClass;
        C[n++].class = handlesWidgetClass;

	C[n].cbtab = &hypertext_cbtab[0];
        C[n++].class = hyperTextWidgetClass;

        C[n++].class = integerFieldWidgetClass;

        C[n++].class = modalShellWidgetClass;
        C[n++].class = panesWidgetClass;
        C[n++].class = popupMenuShellWidgetClass;
        C[n++].class = rubberTileWidgetClass;
        C[n++].class = stepFieldWidgetClass;

#endif
}

static char *
varmake(var, suffix)
char *var;
const char *suffix;
{
	static char v[128];
	register char *p;
	char *strchr();


	/*
	 * If the variable was an array, make sure to put the new
	 * suffix after the variable name but before the square braces.
	 */
	if ((p = strchr(var, '[')) != NULL) {
		*p = '\0';
		sprintf(v, CONSTCHAR "%s_%s[%s", var, suffix, &p[1]);
		*p = '[';
	} else {
		sprintf(v, CONSTCHAR "%s_%s", var, suffix);
	}
	return(v);
}

static void
stubExposeFunction(w, xevent, region)
Widget w;
XEvent *xevent;
Region region;
{
	Arg arg;
	char *cmd;
	char buf[256];

	sprintf(buf, "EXPOSE_XEVENT=0x%x", (unsigned long)xevent);
	env_set(buf);
	sprintf(buf, "EXPOSE_REGION=0x%x", (unsigned long)region);
	env_set(buf);

	XtSetArg(arg, XtNuserData, &cmd);
	XtGetValues(w, &arg, 1);
	if (cmd != NULL && *cmd != '\0')
		ksh_eval(cmd);

	env_blank("EXPOSE_XEVENT");
	env_blank("EXPOSE_REGION");
#if 0
	ksh_eval("unset EXPOSE_XEVENT EXPOSE_REGION");
#endif
}

/*
 * The stub widget gets a default expose procedure that simply
 * executes the ksh command stored in user data.
 * It's limited, but at least allows some uses of the stub widget
 * without escaping to C language.  You can use the XDraw builtin
 * to draw stuff on the widget.
 */

static void
olinit_stub(arg0, wtab, var)
char *arg0;
wtab_t *wtab;
char *var;
{
	Arg arg;

	XtSetArg(arg, XtNexpose, stubExposeFunction);
	XtSetValues(wtab->w, &arg, 1);
}

static void
olinit_menu(arg0, wtab, var)
char *arg0;
wtab_t *wtab;
char *var;
{
	/*
	 * create a convenience variable named $<VAR>_MP
	 * to represent the menu pane.
	 */
	Arg arg;
	Widget pane;
	char panename[128], *panevar;

	XtSetArg(arg, XtNmenuPane, &pane);
	XtGetValues(wtab->w, &arg, 1);
	if (pane == NULL) {
		printerr(arg0, usagemsg(CONSTCHAR "Unable to set up menu pane for %s\n", wtab->wname), NULL);
	} else {
		panevar = varmake(var, CONSTCHAR "MP");
		sprintf(panename, CONSTCHAR "%s_mp", wtab->wname);
		set_up_w(pane, wtab, panevar, panename, wtab->wclass);
	}
}

static void
scrollinglistDestroyCB(widget, wtab)
void  *widget;
wtab_t *wtab;
{
	SListInfo_t *list = (SListInfo_t *)(wtab->info);
	OlListItem *ptr;
	register int i;

	XtFree(list->envar);
	if (list->items) {
		for (i = 0; i < list->lastitem; i++) {
			ptr = OlListItemPointer(list->items[i]);
			if (ptr->user_data)
				XtFree(ptr->user_data);
			if (ptr->label)
				XtFree(ptr->label);
		}
		XtFree((String)list->items);
	}
}

static void
olinit_scrollinglist(arg0, wtab, var)
char *arg0;
wtab_t *wtab;
char *var;
{
	char buf[128];
	void scrollinglistCB();
	SListInfo_t *list;
	Arg arg[1];
	Widget tf;
	char *newvar;
	wtab_t *newwtab;
	char name[128];
	char *strdup();

	/* create a convenience variable named 
	 * $<VAR>_TF to hold the handle of
	 * the textField subcomponent
	 */
	XtSetArg(arg[0], XtNtextField, &tf);
	XtGetValues(wtab->w, &arg[0], 1);
	if (tf == NULL) {
		printerr(arg0, usagemsg(CONSTCHAR "Unable to set up text field variable for %s\n", wtab->wname), NULL);
	} else {
		newvar = varmake(var, CONSTCHAR  "TF");
		sprintf(name, CONSTCHAR "textfield", wtab->wname);
		newwtab = set_up_w(tf, wtab, newvar, name, str_to_class(arg0, CONSTCHAR "textField"));
		olinit_textfield(arg0, newwtab, newvar);
	}

	/* set up a data structure for tracking
	 * list items.
	 */
	list = (SListInfo_t *)XtMalloc(sizeof(SListInfo_t));
	list->maxitems = 0;
	list->lastitem = 0;
	list->items = (OlListToken *)NULL;
	list->envar = strdup(var);
	wtab->info = (caddr_t)list;

	/* Set up a callback for the scrolling list to keep
	 * the <LIST>_CURITEM and <LIST>_CURINDEX variables up to date.
	 * which will be set to the currently selected item
	 * by an automatically added callback function.
	 */
	XtAddCallback(wtab->w, XtNuserMakeCurrent, (XtCallbackProc)scrollinglistCB, (XtPointer)wtab);
	XtAddCallback(wtab->w, XtNdestroyCallback, (XtCallbackProc)scrollinglistDestroyCB, (XtPointer)wtab);

	/* Set the <LIST>_NUMITEMS to zero */
	sprintf(buf, "%s_NUMITEMS=0", var);
	env_set(buf);
	/* Set the <LIST>_CURITEM to nothing */
	sprintf(buf, "%s_CURITEMS=", var);
	env_set(buf);
	/* Set the <LIST>_CURINDEX to nothing */
	sprintf(buf, "%s_CURINDEX=", var);
	env_set(buf);
}

static void
olinit_textfield(arg0, wtab, var)
char *arg0;
wtab_t *wtab;
char *var;
{
	Arg arg[1];
	Widget te;
	char *newvar;
	char name[128];

	/* create a convenience variable named 
	 * $<VAR>_TE to hold the textEdit widget associated
	 * with the textField
	 */
	XtSetArg(arg[0], XtNtextEditWidget, &te);
	XtGetValues(wtab->w, &arg[0], 1);
	if (te == NULL) {
		printerr(arg0, usagemsg(CONSTCHAR "Unable to set up text edit variable for %s\n", wtab->wname), NULL);
	} else {
		newvar = varmake(var, CONSTCHAR  "TE");
		sprintf(name, CONSTCHAR "%s_te", wtab->wname);
		set_up_w(te, wtab, newvar, name, str_to_class(arg0, CONSTCHAR "textEdit"));
	}
}

static void
olinit_popupwindowshell(arg0, wtab, var)
char *arg0;
wtab_t *wtab;
char *var;
{
	Arg arg[3];
	Widget uca, lca, footer;
	char *newvar;
	char name[128];
	wtab_t *footerwtab;

	/* create convenience variables named 
	 * $<VAR>_UCA, $<VAR>_LCA, $<VAR>_FP,
	 * to hold the widget
	 * id's of the upper controlarea and
	 * lower controlarea components of the popupShell.
	 */
	XtSetArg(arg[0], XtNupperControlArea, &uca);
	XtSetArg(arg[1], XtNlowerControlArea, &lca);
	XtSetArg(arg[2], XtNfooterPanel, &footer);
	XtGetValues(wtab->w, &arg[0], 3);
	if (uca == NULL) {
		printerr(arg0, usagemsg(CONSTCHAR "Unable to set up upper control area variable for %s\n", wtab->wname), NULL);
	} else {
		newvar = varmake(var, CONSTCHAR  "UCA");
		sprintf(name, CONSTCHAR "%s_uca", wtab->wname);
		set_up_w(uca, wtab, newvar, name, str_to_class(arg0, CONSTCHAR "controlArea"));
	}
	if (lca == NULL) {
		printerr(arg0, usagemsg(CONSTCHAR "Unable to set up lower control area variable for %s\n", wtab->wname), NULL);
	} else {
		newvar = varmake(var, CONSTCHAR "LCA");
		sprintf(name, CONSTCHAR "%s_lca", wtab->wname);
		set_up_w(lca, wtab, newvar, name, str_to_class(arg0, CONSTCHAR "controlArea"));
	}
	if (footer == NULL) {
		printerr(arg0, usagemsg(CONSTCHAR "Unable to set up footer panel variable for %s\n", wtab->wname), NULL);
	} else {
		newvar = varmake(var, CONSTCHAR "FP");
		sprintf(name, CONSTCHAR "%s_footer", wtab->wname);
		footerwtab = set_up_w(footer, wtab, newvar, name, str_to_class(arg0, CONSTCHAR "footerPanel"));
	}
}

static void
olinit_noticeshell(arg0, wtab, var)
char *arg0;
wtab_t *wtab;
char *var;
{
	Arg arg[2];
	Widget ca, ta;
	char *newvar;
	char name[128];

	/* create convenience variables named 
	 * $<VAR>_CA and $<VAR>_TA to hold the widget
	 * id's of the controlarea component and the
	 * text area components of the notice shell.
	 */
	XtSetArg(arg[0], XtNcontrolArea, &ca);
	XtSetArg(arg[1], XtNtextArea, &ta);
	XtGetValues(wtab->w, &arg[0], 2);
	if (ca == NULL) {
		printerr(arg0, usagemsg(CONSTCHAR "Unable to set up control area variable for %s\n", wtab->wname), NULL);
	} else {
		newvar = varmake(var, CONSTCHAR "CA");
		sprintf(name, CONSTCHAR "%s_ca", wtab->wname);
		set_up_w(ca, wtab, newvar, name, str_to_class(arg0, CONSTCHAR "controlArea"));
	}
	if (ta == NULL) {
		printerr(arg0, usagemsg(CONSTCHAR "Unable to set up text area variable for %s\n", wtab->wname), NULL);
	} else {
		newvar = varmake(var, CONSTCHAR "TA");
		sprintf(name, CONSTCHAR "%s_ta", wtab->wname);
		set_up_w(ta, wtab, newvar, name, str_to_class(arg0, CONSTCHAR "staticText"));
	}
}

static void
scrollinglistCB(widget, wtab, token)
Widget  widget;
wtab_t *wtab;
OlListToken token;
{
	OlListItem *item;
	register int i;
	SListInfo_t *list = (SListInfo_t *)(wtab->info);
	char *name, num[32];
	extern wtab_t W[];

	item = OlListItemPointer(token);

	name = varmake(list->envar, CONSTCHAR "CURITEM");
	env_set_var(name, item->label);
	name = varmake(list->envar, CONSTCHAR "CURINDEX");
	for (i = 0; i < list->lastitem; i++) {
		if (token == list->items[i]) {
			sprintf(num, CONSTCHAR "%d", i);
			env_set_var(name, num);
			return;
		}
	}
	env_set_var(name, CONSTCHAR "");
	return;
}

const char CallDataPrefix[] = "CALL_DATA_";

void
precb_set_str(field, str)
const char *field, *str;
{
	char buf[1024];

	sprintf(buf, CONSTCHAR "%s%s=%s", CallDataPrefix, field, str ? str : "");
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
vendorshell_precb_wmprotocol(w, ver)
wtab_t *w;
OlWMProtocolVerify *ver;
{
	char buf[1024];
	const char *str;

	if (ver == NULL) {
                precb_set_str(CONSTCHAR "REASON", CONSTCHAR "focusout");
                precb_set_str(CONSTCHAR "STRING", "");
                precb_set_bool(CONSTCHAR "OK", "true");
                return;
        }

	switch (ver->msgtype) {
	case OL_WM_DELETE_WINDOW:
		str = CONSTCHAR "deletewindow";
		break;
	case OL_WM_SAVE_YOURSELF:
		str = CONSTCHAR "saveyourself";
		break;
	case OL_WM_TAKE_FOCUS:
		str = CONSTCHAR "takefocus";
		break;
	default:
		str = CONSTCHAR "unknown";
		break;
	}
	precb_set_str(CONSTCHAR "MSGTYPE", str);
	if (w->info != NULL) {
		XtFree(w->info);
	}
	w->info = (char *)XtMalloc(sizeof(OlWMProtocolVerify));
	memcpy(w->info, ver, sizeof(OlWMProtocolVerify));
}


static void
textfield_precb_verification(w, ver)
wtab_t *w;
OlTextFieldVerifyPointer ver;
{
	char buf[1024];
	const char *str;

	if (ver == NULL)
		return;

	switch (ver->reason) {
	case OlTextFieldReturn:
		str = CONSTCHAR "return";
		break;
	case OlTextFieldPrevious:
		str = CONSTCHAR "previous";
		break;
	case OlTextFieldNext:
		str = CONSTCHAR "next";
		break;
	default:
		str = CONSTCHAR "unknown";
		break;
	}
	precb_set_str(CONSTCHAR "REASON", str);
	precb_set_str(CONSTCHAR "STRING", ver->string);
	precb_set_bool(CONSTCHAR "OK", ver->ok);
}

static void
textfield_postcb_verification(w, ver)
wtab_t *w;
OlTextFieldVerifyPointer ver;
{
	char *env_get();
	char *ok = env_get(CONSTCHAR "CALL_DATA_OK");

	if (ver == NULL)
		return;

	if (ok == NULL || strcmp(ok, CONSTCHAR "false") == 0)
		ver->ok = 0;
	else
		ver->ok = 1;
}

static void
manager_precb_consumeevent()
{
}

static void
manager_postcb_consumeevent()
{
}

static void
scrollbar_precb_slidermoved(w, sb)
wtab_t *w;
OlScrollbarVerify *sb;
{
	precb_set_int(CONSTCHAR "NEW_LOCATION", sb->new_location);
	precb_set_int(CONSTCHAR "NEW_PAGE", sb->new_page);
	precb_set_bool(CONSTCHAR "OK", sb->ok);
	precb_set_int(CONSTCHAR "SLIDERMIN", sb->slidermin);
	precb_set_int(CONSTCHAR "SLIDERMAX", sb->slidermax);
	precb_set_int(CONSTCHAR "DELTA", sb->delta);
	precb_set_bool(CONSTCHAR "MORE_CB_PENDING", sb->more_cb_pending);
}

static void
scrollbar_postcb_slidermoved(w, sb)
wtab_t *w;
OlTextFieldVerifyPointer sb;
{
	char *env_get();
	char *ok = env_get(CONSTCHAR "CALL_DATA_OK");

	if (ok == NULL || strcmp(ok, CONSTCHAR "false") == 0)
		sb->ok = 0;
	else
		sb->ok = 1;
}

static void
slider_precb_slidermoved(w, sl)
wtab_t *w;
OlSliderVerify *sl;
{
	precb_set_int(CONSTCHAR "NEW_LOCATION", sl->new_location);
	precb_set_bool(CONSTCHAR "MORE_CB_PENDING", sl->more_cb_pending);
}

static void
scrollinglist_precb_makecurrent(w, tok)
wtab_t *w;
OlListToken tok;
{
	register int i;
	SListInfo_t *list = (SListInfo_t *)(w->info);

	for (i = 0; i < list->lastitem; i++) {
		if (tok == list->items[i]) {
			precb_set_int(CONSTCHAR "ITEM", i);
			break;
		}
	}
}

static void
scrollinglist_precb_delete(w, ld)
wtab_t *w;
OlListDelete *ld;
{
	register int i, j;
	SListInfo_t *list = (SListInfo_t *)(w->info);
	char buf[128];

	ksh_eval(CONSTCHAR "unset CALL_DATA_ITEMS");
	for (i = 0; i < ld->num_tokens; i++) {
		for (j = 0; j < list->lastitem; j++) {
			if (list->items[j] == ld->tokens[i]) {
				sprintf(buf, CONSTCHAR "%s_ITEMS[%d]=%d", 
					CallDataPrefix, i, j);
				env_set(buf);
				break;
			}
		}
	}
}

#ifdef MOOLIT
static void
hypertext_precb_select(w, hs)
wtab_t *w;
HyperSegment *hs;
{
	Position rootx, rooty;

	if (hs == NULL)
		return;

	precb_set_str(CONSTCHAR "KEY", hs->key ? hs->key : "");
	precb_set_str(CONSTCHAR "SCRIPT", hs->script ? hs->script : "");
	precb_set_str(CONSTCHAR "TEXT", hs->text ? hs->text : "");
	precb_set_int(CONSTCHAR "X", hs->x);
	precb_set_int(CONSTCHAR "Y", hs->y);
	precb_set_int(CONSTCHAR "W", hs->w);
	precb_set_int(CONSTCHAR "H", hs->h);
	precb_set_int(CONSTCHAR "REVERSE", hs->reverse_video);
	XtTranslateCoords(w->w, hs->x, hs->y, &rootx, &rooty);
	precb_set_int(CONSTCHAR "ROOTX", rootx);
	precb_set_int(CONSTCHAR "ROOTY", rooty);
}
#endif
