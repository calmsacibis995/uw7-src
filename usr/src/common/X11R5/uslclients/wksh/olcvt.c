
/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:olcvt.c	1.6"

/* X/OL includes */

#include <stdio.h>
#include <signal.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrollingL.h>
#include <Xol/Flat.h>
#include <Xol/ChangeBar.h>
#include <Xol/TextEdit.h>
#include <setjmp.h>
#include <string.h>
#include <ctype.h>
#include <search.h>
#include "wksh.h"
#include "olksh.h"
#include "xpm.h"

extern Widget Toplevel;

/*
 * Converters for OPEN LOOK Wksh
 */

void
WkshCvtStringToOlChangeBarDefine(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	static OlDefine cvt;
	char *str = (String)(fval->addr);

	if (isdigit(str[0])) {
		XtConvert(Toplevel, 
			str_XtRString, fval, CONSTCHAR "Short", toval);
		return;
	} else if (strcmp(str, "dim") == 0) {
		cvt = OL_DIM;
	} else if (strcmp(str, "normal") == 0) {
		cvt = OL_NORMAL;
	} else {
		XtConvert(Toplevel, 
			str_XtRString, fval, str_XtROlDefine, toval);
		if (toval->addr != NULL) {
			cvt = (((OlDefine *)toval->addr)[0]);
		} else
			return;
	}
	toval->addr = (caddr_t)&cvt;
	toval->size = sizeof(OlDefine);
}

void
WkshCvtOlChangeBarDefineToString(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	static OlDefine cvt;
	
	cvt = (OlDefine)(((int *)fval)[0]);

	fval->addr = (caddr_t)&cvt;
	fval->size = sizeof(OlDefine);
	XtConvert(Toplevel, 
		str_XtROlDefine, fval, str_XtRString, toval);
}

/*
 * Some resources in OPEN LOOK are define as "int" even though they use
 * OlDefine values.  So, we define a "string to oldefine int" converter
 * for those resources to call either the usual string to int converter
 * if the thing starts with a digit, or the String to OlDefine converter
 * followed by a "cast" to int.
 */

void
WkshCvtStringToOlDefineInt(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	if (isdigit(((String)fval->addr)[0])) {
		XtConvert(Toplevel, 
			str_XtRString, fval, str_XtRInt, toval);
	} else {
		XtConvert(Toplevel, 
			str_XtRString, fval, str_XtROlDefine, toval);
		if (toval->addr != NULL) {
			static int cvt;
			
			cvt = (int)(((OlDefine *)toval->addr)[0]);
			toval->addr = (caddr_t)&cvt;
			toval->size = sizeof(int);
		}
	}
}

void
WkshCvtOlDefineIntToString(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	static OlDefine cvt;
	
	cvt = (OlDefine)(((int *)fval)[0]);

	fval->addr = (caddr_t)&cvt;
	fval->size = sizeof(OlDefine);
	XtConvert(Toplevel, 
		str_XtROlDefine, fval, str_XtRString, toval);
}

struct oldefinetab {
	OlDefine val;
	const char *name;
};

static const struct oldefinetab VirtualNameTab[] = {
	OL_IMMEDIATE,	"immediate",
	OL_MOVERIGHT,	"moveright",
	OL_MOVELEFT,	"moveleft",
	OL_MOVEUP,	"moveup",
	OL_MOVEDOWN,	"movedown",
	OL_MULTIRIGHT,	"multiright",
	OL_MULTILEFT,	"multileft",
	OL_MULTIUP,	"multiup",
	OL_MULTIDOWN,	"multidown",
	OL_NEXTFIELD,	"nextfield",
	OL_PREVFIELD,	"prevfield",
	0,		NULL
};

static struct Amemory *VirtHash = NULL; 

void
WkshCvtStringToOlVirtualName(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	struct oldefinetab key;
	struct oldefinetab *found;
	char *string;
	struct namnod *nam;
	static OlVirtualName addr;

	if (fval->size <= 0) {
		XtWarningMsg(CONSTCHAR "cvtStringToOlVirtualName", CONSTCHAR "badsize", CONSTCHAR "XtToolkitError",
			CONSTCHAR "From size is zero or negative", NULL, 0);
		toval->addr = NULL;
		toval->size = 0;
		return;
	}
	string = (char *)fval->addr;
	if (VirtHash == NULL) {
		struct Amemory *gettree();
		register int i;

		VirtHash = gettree(8);
		for (i = 0; VirtualNameTab[i].name != NULL; i++) {
			if ((nam = nam_search(VirtualNameTab[i].name, VirtHash, 1)) == NULL) {
				XtWarningMsg(CONSTCHAR "cvtStringToOlVirtualName", CONSTCHAR "badhash", CONSTCHAR "XtToolkitError",
					CONSTCHAR "unable to set up hash table", NULL, 0);
			} else {
				nam->value.namval.i = VirtualNameTab[i].val;
			}
		}
	}
	if ((nam = nam_search(string, VirtHash, 0)) == NULL) {
		XtWarningMsg(CONSTCHAR "cvtStringToOlVirtualName", CONSTCHAR "nosuch", CONSTCHAR "XtToolkitError",
			CONSTCHAR "No such direction", NULL, 0);
	}
	toval->size = sizeof(OlVirtualName);
	addr = (OlDefine)nam->value.namval.i;
	toval->addr = (caddr_t)&addr;
	return;
}

static const struct oldefinetab OlDefineTab[] = {
	OL_ABSENT_PAIR,	"absentpair",
	OL_ALL,	"all",
	OL_ALWAYS, "always",
	OL_ATOM_HELP, "atomhelp",
	OL_BOTH,	"both",
	OL_BOTTOM,	"bottom",
	OL_BUTTONSTACK,	"buttonstack",
	OL_CENTER,	"center",
	OL_CLASS_HELP,	"classhelp",
	OL_COLUMNS,	"columns",
	OL_CURRENT,	"current",
	OL_DISK_SOURCE,	"disksource",
	OL_DISPLAY_FORM,	"displayform",
	OL_DOWN,	"down",
	OL_EXISTING_SOURCE, "existingsource",
	OL_FIXEDCOLS, "fixedcols",
	OL_FIXEDHEIGHT, "fixedheight",
	OL_FIXEDROWS, "fixedrows",
	OL_FIXEDWIDTH, "fixedwidth",
	OL_FLAT_BUTTON, "flatbutton",
	OL_FLAT_CHECKBOX, "flatcheckbox",
	OL_FLAT_CONTAINER, "flatcontainer",
	OL_FLAT_EXCLUSIVES, "flatexclusives",
	OL_FLAT_HELP, "flathelp",
	OL_FLAT_NONEXCLUSIVES, "flatnonexclusives",
	OL_HALFSTACK, "halfstack",
	OL_HORIZONTAL, "horizontal",
	OL_IMAGE, "image",
	OL_IN, "in",
	OL_INDIRECT_SOURCE, "indirectsource",
	OL_LABEL, "label",
	OL_LEFT, "left",
	OL_MASK_PAIR, "mask_pair",
	OL_MAXIMIZE, "maximize",
	OL_MILLIMETERS, "millimeters",
	OL_MINIMIZE, "minimize",
	OL_NEVER, "never",
	OL_NEXT, "next",
	OL_NONE,	"none",
	OL_NONEBOTTOM, "nonebottom",
	OL_NONELEFT, "noneleft",
	OL_NONERIGHT, "noneright",
	OL_NONETOP, "nontop",
	OL_NOTICES, "notices",
	OL_NO_VIRTUAL_MAPPING, "novirtualmapping",
	OL_OBLONG, "oblong",
	OL_OUT, "out",
	OL_OVERRIDE_PAIR, "overridepair",
	OL_PIXELS, "pixels",
	OL_POINTS, "points",
	OL_POPUP, "popup",
	OL_PREVIOUS, "previous",
	OL_PROG_DEFINED_SOURCE, "progdefinedsource",
	OL_RECTBUTTON, "rectbutton",
	OL_RIGHT, "right",
	OL_ROWS, "rows",
	OL_SOURCE_FORM, "sourceform",
	OL_SOURCE_PAIR, "sourcepair",
	OL_STAYUP, "stayup",
	OL_STRING, "string",
	OL_STRING_SOURCE, "stringsource",
	OL_TEXT_APPEND, "textappend",
	OL_TEXT_EDIT, "textedit",
	OL_TEXT_READ, "textread",
	OL_TOP, "top",
	OL_TRANSPARENT_SOURCE, "transparentsource",
	OL_VERTICAL, "vertical",
	OL_VIRTUAL_BUTTON, "verticalbutton",
	OL_VIRTUAL_KEY, "virtualkey",
	OL_WIDGET_HELP, "widgethelp",
	OL_WINDOW_HELP, "windowhelp",
	OL_WRAP_ANY, "wrapany",
	OL_WRAP_WHITE_SPACE, "wrapwhitepspace",
	OL_CONTINUOUS, "continuous",
	OL_GRANULARITY, "granularity",
	OL_RELEASE, "release",
	OL_TICKMARK, "tickmark",
	OL_PERCENT, "percent",
	OL_SLIDERVALUE, "slidervalue",
	OL_WT_BASE, "wtbase",
	OL_WT_CMD, "wtcmd",
	OL_WT_NOTICE, "wtnotice",
	OL_WT_HELP, "wthelp",
	OL_WT_OTHER, "wtother",
	OL_SUCCESS, "success",
	OL_DUPLICATE_KEY, "duplicatekey",
	OL_DUPLICATEKEY, "duplicatekey",
	OL_BAD_KEY, "badkey",
	OL_MENU_FULL, "menufull",
	OL_MENU_LIMITED, "menulimited",
	OL_MENU_CANCEL, "menucancel",
	OL_SELECTKEY, "selectkey",
	OL_MENUKEY, "menukey",
	OL_MENUDEFAULT, "menudefault",
	OL_MENUDEFAULTKEY, "menudefaultkey",
	OL_HSBMENU, "hsbmenu",
	OL_VSBMENU, "vsbmenu",
	OL_ADJUSTKEY, "adjustkey",
	OL_NEXTAPP, "nextapp",
	OL_NEXTWINDOW, "nextwindow",
	OL_PREVAPP, "prevapp",
	OL_PREVWINDOW, "prevwindow",
	OL_WINDOWMENU, "windowmenu",
	OL_WORKSPACEMENU, "workspacemenu",
	OL_DEFAULTACTION, "defaultaction",
	OL_DRAG, "drag",
	OL_DROP, "drop",
	OL_TOGGLEPUSHPIN, "togglepushpin",
	OL_PAGELEFT, "pageleft",
	OL_PAGERIGHT, "pageright",
	OL_SCROLLBOTTOM, "scrollbottom",
	OL_SCROLLTOP, "scrolltop",
	OL_MULTIRIGHT,	"multiright",
	OL_MULTILEFT, "multileft",
	OL_MULTIDOWN, "multidown",
	OL_MULTIUP, "multiup",
	OL_IMMEDIATE, "multiimmediate",
	OL_MOVEUP, "moveup",
	OL_MOVEDOWN, "movedown",
	OL_MOVERIGHT, "moveright",
	OL_MOVELEFT, "moveleft",
	OL_CLICK_TO_TYPE, "clicktotype",
	OL_REALESTATE, "realestate",
	OL_UNDERLINE, "underline",
	OL_HIGHLIGHT, "highlight",
	OL_INACTIVE, "inactive",
	OL_DISPLAY, "display",
	OL_PROC, "proc",
	OL_SIZE_PROC, "sizeproc",
	OL_DRAW_PROC, "drawproc",
	OL_PINNED_MENU, "pinnedmenu",
	OL_PRESS_DRAG_MENU, "pressdragmenu",
	OL_STAYUP_MENU, "stayupmenu",
	OL_POINTER, "pointer",
	OL_INPUTFOCUS, "inputfocus",
	OL_QUIT, "quit",
	OL_DESTROY, "destroy",
	OL_DISMISS, "dismiss",
	OL_PRE, "pre",
	OL_POST, "post",
};

static int
oldefinecompare(d1, d2)
const void *d1;
const void *d2;
{
	return(((struct oldefinetab *)d1)->val - ((struct oldefinetab *)d2)->val);
}

void
WkshCvtOlDefineToString(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	struct oldefinetab key;
	struct oldefinetab *found;

	if (fval->size != sizeof(OlDefine) || fval->addr == NULL) {
		XtWarningMsg(CONSTCHAR "cvtOlDefineToString", CONSTCHAR "badsize", CONSTCHAR "XtToolkitError",
			CONSTCHAR "From size != sizeof(OlDefine)", NULL, 0);
		toval->addr = NULL;
		toval->size = 0;
		return;
	}
	key.val = ((OlDefine *)fval->addr)[0];
	found = (struct oldefinetab *)bsearch(&key, OlDefineTab, 
		sizeof(OlDefineTab)/sizeof(struct oldefinetab),
		sizeof(struct oldefinetab), 
		oldefinecompare);
	if (found == NULL) {
		static char result[16];

		toval->addr = result;
		toval->size = strlen(result)+1;
		sprintf(result, "%d", key.val);

		XtWarningMsg(CONSTCHAR "cvtOlDefineToString", CONSTCHAR "no such define", CONSTCHAR "XtToolkitError", "Cannot find OlDefine, using integer representation", NULL, 0);
			
		return;
	}
	toval->addr = (XtPointer)found->name;
	toval->size = strlen(found->name) + 1;
}

void
WkshCvtStringToCallbackProc(args, nargs, fval, toval)
XrmValuePtr args;
Cardinal *nargs;
XrmValuePtr fval, toval;
{
	static void (*proc)();
	void stdFlatSelectProc(), stdFlatUnselectProc(), stdFlatDblSelectProc(),
		stdFlatDropProc();
	extern wtab_t *WKSHConversionWidget;
	wtab_t *w = WKSHConversionWidget;
	extern char *WKSHConversionResource;
	FlatInfo_t *finfo;

	if (w != NULL) {
		if (w->info == NULL) {
			w->info = (caddr_t)XtMalloc(sizeof(FlatInfo_t));
			memset(w->info, '\0', sizeof(FlatInfo_t));
		}
		finfo = (FlatInfo_t *)w->info;
		if (strcmp(WKSHConversionResource, CONSTCHAR "unselectProc") == 0) {
			if (finfo->unselectProcCommand != NULL)
				XtFree(finfo->unselectProcCommand);
			finfo->unselectProcCommand = strdup((String)fval->addr);
			proc = stdFlatUnselectProc;
		} else if (strcmp(WKSHConversionResource, CONSTCHAR "selectProc") == 0) {
			if (finfo->selectProcCommand != NULL)
				XtFree(finfo->selectProcCommand);
			finfo->selectProcCommand = strdup((String)fval->addr);
			proc = stdFlatSelectProc;
		}
#ifdef MOOLIT
		else if (strcmp(WKSHConversionResource, CONSTCHAR "dblSelectProc") == 0) {
			if (finfo->dblSelectProcCommand != NULL)
				XtFree(finfo->dblSelectProcCommand);
			finfo->dblSelectProcCommand = strdup((String)fval->addr);
			proc = stdFlatDblSelectProc;
		}
		else if (strcmp(WKSHConversionResource, CONSTCHAR "dropProc") == 0) {
			if (finfo->dropProcCommand != NULL)
				XtFree(finfo->dropProcCommand);
			finfo->dropProcCommand = strdup((String)fval->addr);
			proc = stdFlatDropProc;
		}
#endif
		else {
			XtWarningMsg(CONSTCHAR "cvtStringToCallbackProc", CONSTCHAR "unsupported flat callback proc", CONSTCHAR "XtToolkitError", "This resource is not currently supported by WKSH", NULL, 0);
		}
	} else {
		XtWarningMsg(CONSTCHAR "cvtStringToCallbackProc", CONSTCHAR "widget must exist", CONSTCHAR "XtToolkitError", "This resource cannot be set at creation time by WKSH, use XtSetValues after creation instead", NULL, 0);
		toval->size = 0;
		toval->addr = NULL;
		return;
	}

	toval->size = sizeof(XtCallbackProc);
	toval->addr = (caddr_t)&proc;
	return;
}
