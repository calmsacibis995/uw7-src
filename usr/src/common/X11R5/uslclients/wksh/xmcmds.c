/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:xmcmds.c	1.12"


/* X/MO includes */

#include <stdio.h>
#include <signal.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/Scale.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>

#include <setjmp.h>
#include <string.h>
#include <ctype.h>

#include <DesktopP.h>

#include "wksh.h"
#include "xmksh.h"


#define MAXARGS 1024
#define SLISTITEMSIZE	16

extern Widget Toplevel;	/* set by do_OlInitialize */

extern char *strdup(), *stakalloc();

char *XmStringToString();

static const char str_APPNAME[] = "APPNAME";
static const char str_TOPLEVEL[] = "TOPLEVEL";
extern const char str_wksh[];
wtab_t *set_up_w();

void (*toolkit_addcallback)() = XtAddCallback;
void (*toolkit_callcallbacks)() = XtCallCallbacks;

static XrmOptionDescRec *ExtraOptions;
static Cardinal ExtraOptionsNumber;

void 
set_parse_options(options, numoptions)
XrmOptionDescRec options[];
Cardinal numoptions;
{
	ExtraOptions = options;
	ExtraOptionsNumber = numoptions;
}

int
toolkit_initialize(argc, argv)
int argc;
char *argv[];
{
	int i;
	char name[8], *var, *env_get();
	wtab_t *w;
	int newargc;
	char *newargv[256];
	char envbuf[2048];

	extern void WkshCvtStringToPixmap();
	extern void WkshCvtKeySymToString();
	extern void XmCvtStringToUnitType();
	extern void XmuCvtStringToBitmap();
	extern void WkshCvtStringToXImage();
	extern void WkshCvtWidgetToString();
	extern void WkshCvtStringToWidget();
	extern void WkshCvtIntToString();
	extern void WkshCvtBooleanToString();
	extern void WkshCvtStringToCallback();
	extern void WkshCvtCallbackToString();
	extern void WkshCvtHexIntToString();

	extern void WkshCvtXmStringToString();

	init_widgets();
	if (argc < 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s variable name class [args...]\n", argv[0]), NULL);
		return(1);
	}
	newargv[0] = NULL;
	newargc = 0;

	if (argc < 4) {
		Toplevel = XtInitialize((char *)str_wksh, (char *)str_wksh, ExtraOptions, ExtraOptionsNumber, 0, NULL);
	} else {
		int moargc = argc - 3;

		Toplevel = XtInitialize(argv[2], argv[3], ExtraOptions, ExtraOptionsNumber, &moargc, &argv[3]);
		for (i = 0 ; i < moargc && newargc < sizeof(newargv)/sizeof(char *) - 1; i++, newargc++) {
			newargv[newargc] = argv[i+3];
		}
		newargv[newargc] = NULL;
	}
	if (Toplevel == NULL) {
		printerr(argv[0], CONSTCHAR "Could not initialize OPEN LOOK Toolkit", NULL);
		env_blank(argv[1]);
		return(1);
	}
	DtiInitialize(Toplevel);
	WkshRegisterNamedIntConverters();
	XtAddConverter(XmRKeySym, XtRString, 
		WkshCvtKeySymToString, NULL, 0);
	XtAddConverter(XtRString, XmRUnitType, 
		XmCvtStringToUnitType, NULL, 0);
	XtAddConverter(XtRString, XtRBitmap, 
		XmuCvtStringToBitmap, NULL, 0);
	XtAddConverter(XmRXmString, XtRString, 
		WkshCvtXmStringToString, NULL, 0);
	XtAddConverter(XtRString, CONSTCHAR "XImage", 
		WkshCvtStringToXImage, screenConvertArg, 1);

	XtSetTypeConverter(XtRString, XtRWidget,
		WkshCvtStringToWidget, NULL, 0, XtCacheNone, NULL);
	XtSetTypeConverter(XtRString, CONSTCHAR "MenuWidget",
		WkshCvtStringToWidget, NULL, 0, XtCacheNone, NULL);
	XtAddConverter(XtRWidget, XtRString, 
		WkshCvtWidgetToString, NULL, 0);

	XtAddConverter(XtRInt, XtRString, 
		WkshCvtIntToString, NULL, 0);
	XtAddConverter("TextPosition", XtRString, 
		WkshCvtIntToString, NULL, 0);
	XtAddConverter(XtRShort, XtRString, 
		WkshCvtIntToString, NULL, 0);
	XtAddConverter(XtRCardinal, XtRString, 
		WkshCvtIntToString, NULL, 0);
	XtAddConverter(XtRDimension, XtRString, 
		WkshCvtIntToString, NULL, 0);
	XtAddConverter(XtRPosition, XtRString, 
		WkshCvtIntToString, NULL, 0);
	XtAddConverter(XmRHorizontalDimension, XtRString, 
		WkshCvtIntToString, NULL, 0);
	XtAddConverter(XmRVerticalDimension, XtRString, 
		WkshCvtIntToString, NULL, 0);
	XtAddConverter(XmRHorizontalPosition, XtRString, 
		WkshCvtIntToString, NULL, 0);
	XtAddConverter(XmRVerticalPosition, XtRString, 
		WkshCvtIntToString, NULL, 0);
	XtAddConverter(XtRBoolean, XtRString, 
		WkshCvtBooleanToString, NULL, 0);
	XtAddConverter(XtRBool, XtRString, 
		WkshCvtBooleanToString, NULL, 0);
	XtSetTypeConverter(XtRString, XtRCallback,
		WkshCvtStringToCallback, NULL, 0, XtCacheNone, NULL);
	XtAddConverter(XtRCallback, XtRString, 
		WkshCvtCallbackToString, NULL, 0);
	XtAddConverter(XtRPointer, XtRString, 
		WkshCvtHexIntToString, NULL, 0);
	XtAddConverter(XtRPixel, XtRString, 
		WkshCvtHexIntToString, NULL, 0);
	XtAddConverter(XtRPixmap, XtRString, 
		WkshCvtHexIntToString, NULL, 0);

	w = set_up_w(Toplevel, NULL, argv[1], argv[2], str_to_class(argv[0], CONSTCHAR "topLevelShell"));

	var = env_get(str_TOPLEVEL);
	if (var == NULL || *var == '\0') {
		env_set_var(str_TOPLEVEL, w->widid);
	}

	var = env_get(str_APPNAME);
	if (var == NULL || *var == '\0') {
		env_set_var(str_APPNAME, argv[2]);
	}

	ksh_eval(CONSTCHAR "unset ARGV");
	for (i = 0; i < newargc; i++) {
		sprintf(envbuf, CONSTCHAR "ARGV[%d]=%s", i, newargv[i]);
		env_set(envbuf);
	}
	env_set("WKSH_API=MOTIF");
	return(0);
}

int
toolkit_special_resource(arg0, res, w, parent, class, resource, val, ret, flatret, flatres, freeit, firstcall)
char *arg0;
XtResourceList res;
wtab_t *w;
wtab_t *parent;
classtab_t *class;
char *resource;
char *val;
XtArgVal *ret;
XtArgVal *flatret;
char **flatres;
int *freeit;
int firstcall;  /* nonzero means this is the first call within a widget */
{
	if (w == NULL && 
		(strcmp(res->resource_type, XmRHorizontalDimension) == 0 ||
		strcmp(res->resource_type, XmRVerticalDimension) == 0)) {

			printerrf("Conversion", CONSTCHAR "Warning: the %s resource cannot be set at widget\ncreation time use XtSetValues after creation instead.", res->resource_name);
		return(TRUE);
	}
	return(FALSE);
}

#ifdef NORECOMP
/*
 * The beta Dell version of MOTIF uses these, but they don't seem
 * to be defined anywhere.  These stubs are a temporary workaround
 * so we can compile.  Everything seems to work without them(!)
 */

re_comp() {
	return(NULL);
}

re_exec() {
	return(NULL);
}
#endif

static int
_xmcreatefunc(func, wclass, argc, argv)
Widget (*func)();
char *wclass;
int argc;
char *argv[];
{
	Widget widget, realparent;
	classtab_t *class;
	char *arg0 = argv[0];
	wtab_t *w, *pw, *wtab, *parenttab;
	char *wname, *parentid, *var;
	Arg	args[MAXARGS];
	register int	i;
	int n;
	int manageflag = FALSE;

	if (argc < 4) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s [-m] var parent name [arg:val ...]\n", argv[0]), NULL);
		return(1);
	}
	while (argv[1][0] == '-') {
		if (strcmp(argv[1], CONSTCHAR "-m") == 0) {
			manageflag = TRUE;
			argv++;
			argc--;
		} else if (strcmp(argv[1], CONSTCHAR "+m") == 0) {
			manageflag = FALSE;
			argv++;
			argc--;
		} else {
			printerrf(arg0, "bad option: %s", argv[1]);
			return(1);
		}
	}
	var = argv[1];
	parentid = argv[2];
	wname = argv[3];
	pw = str_to_wtab(argv[0], parentid);
	if (pw == NULL) {
		printerr(argv[0], CONSTCHAR "Could not find parent\n", NULL);
		return(1);
	}
	argv += 4;
	argc -= 4;
	if ((class = str_to_class(arg0, wclass)) == NULL) {
		return(1);
	}
	parse_args(arg0, argc, argv, NULL, pw, class, &n, args);
	widget = func(pw->w, wname, args, n);
	if (widget != NULL) {
		/* Some of the XmCreate* functions return a widget
		 * id whose parent is not necessarily the parent
		 * passed in.  For example, DialogShell returns the
		 * widget of the dialog, not the Shell which is the
		 * real parent.
		 *
		 * So, we check to see if the parent is the same as
		 * the passed-in parent, and if not then we create
		 * a new entry for the real parent.
		 */
		realparent = XtParent(widget);
		if (realparent != pw->w) {
			parenttab = (wtab_t *)widget_to_wtab(realparent, NULL);
		} else
			parenttab = pw;
		wtab = set_up_w(widget, parenttab, var, wname, class);

		if (class->initfn)
			class->initfn(arg0, wtab, var);
		if (manageflag)
			XtManageChild(widget);
	} else {
		printerr(argv[0], usagemsg(CONSTCHAR "Creation of widget '%s' failed\n", wname), NULL);
		env_blank(argv[1]);
	}
	free_args(n, args);

	return(0);
}

do_XmCreateArrowButton(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateArrowButton();

	return(_xmcreatefunc(XmCreateArrowButton, CONSTCHAR "arrowButton", argc, argv));
}


do_XmCreateArrowButtonGadget(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateArrowButtonGadget();

	return(_xmcreatefunc(XmCreateArrowButtonGadget, CONSTCHAR "arrowButtonGadget", argc, argv));
}


do_XmCreateBulletinBoard(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateBulletinBoard();

	return(_xmcreatefunc(XmCreateBulletinBoard, CONSTCHAR "bulletinBoard", argc, argv));
}


do_XmCreateBulletinBoardDialog(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateBulletinBoardDialog();

	return(_xmcreatefunc(XmCreateBulletinBoardDialog, CONSTCHAR "bulletinBoard", argc, argv));
}


do_XmCreateCascadeButton(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateCascadeButton();

	return(_xmcreatefunc(XmCreateCascadeButton, CONSTCHAR "cascadeButton", argc, argv));
}


do_XmCreateCascadeButtonGadget(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateCascadeButtonGadget();

	return(_xmcreatefunc(XmCreateCascadeButtonGadget, CONSTCHAR "cascadeButtonGadget", argc, argv));
}


do_XmCreateCommand(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateCommand();

	return(_xmcreatefunc(XmCreateCommand, CONSTCHAR "command", argc, argv));
}


do_XmCreateDialogShell(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateDialogShell();

	return(_xmcreatefunc(XmCreateDialogShell, CONSTCHAR "dialogShell", argc, argv));
}


do_XmCreateDrawingArea(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateDrawingArea();

	return(_xmcreatefunc(XmCreateDrawingArea, CONSTCHAR "drawingArea", argc, argv));
}


do_XmCreateDrawnButton(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateDrawnButton();

	return(_xmcreatefunc(XmCreateDrawnButton, CONSTCHAR "drawnButton", argc, argv));
}


do_XmCreateErrorDialog(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateErrorDialog();

	return(_xmcreatefunc(XmCreateErrorDialog, CONSTCHAR "messageBox", argc, argv));
}


do_XmCreateFileSelectionBox(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateFileSelectionBox();

	return(_xmcreatefunc(XmCreateFileSelectionBox, CONSTCHAR "fileSelectionBox", argc, argv));
}


do_XmCreateFileSelectionDialog(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateFileSelectionDialog();

	return(_xmcreatefunc(XmCreateFileSelectionDialog, CONSTCHAR "fileSelectionBox", argc, argv));
}


do_XmCreateForm(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateForm();

	return(_xmcreatefunc(XmCreateForm, CONSTCHAR "form", argc, argv));
}


do_XmCreateFormDialog(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateFormDialog();

	return(_xmcreatefunc(XmCreateFormDialog, CONSTCHAR "form", argc, argv));
}


do_XmCreateFrame(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateFrame();

	return(_xmcreatefunc(XmCreateFrame, CONSTCHAR "frame", argc, argv));
}


do_XmCreateInformationDialog(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateInformationDialog();

	return(_xmcreatefunc(XmCreateInformationDialog, CONSTCHAR "messageBox", argc, argv));
}


do_XmCreateLabel(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateLabel();

	return(_xmcreatefunc(XmCreateLabel, CONSTCHAR "label", argc, argv));
}


do_XmCreateLabelGadget(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateLabelGadget();

	return(_xmcreatefunc(XmCreateLabelGadget, CONSTCHAR "labelGadget", argc, argv));
}


do_XmCreateList(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateList();

	return(_xmcreatefunc(XmCreateList, CONSTCHAR "list", argc, argv));
}


do_XmCreateMainWindow(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateMainWindow();

	return(_xmcreatefunc(XmCreateMainWindow, CONSTCHAR "mainWindow", argc, argv));
}


do_XmCreateMenuBar(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateMenuBar();

	return(_xmcreatefunc(XmCreateMenuBar, CONSTCHAR "rowColumn", argc, argv));
}


do_XmCreateMenuShell(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateMenuShell();

	return(_xmcreatefunc(XmCreateMenuShell, CONSTCHAR "menuShell", argc, argv));
}


do_XmCreateMessageBox(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateMessageBox();

	return(_xmcreatefunc(XmCreateMessageBox, CONSTCHAR "messageBox", argc, argv));
}


do_XmCreateMessageDialog(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateMessageDialog();

	return(_xmcreatefunc(XmCreateMessageDialog, CONSTCHAR "messageBox", argc, argv));
}


do_XmCreateOptionMenu(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateOptionMenu();

	return(_xmcreatefunc(XmCreateOptionMenu, CONSTCHAR "rowColumn", argc, argv));
}


do_XmCreatePanedWindow(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreatePanedWindow();

	return(_xmcreatefunc(XmCreatePanedWindow, CONSTCHAR "panedWindow", argc, argv));
}


do_XmCreatePopupMenu(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreatePopupMenu();

	return(_xmcreatefunc(XmCreatePopupMenu, CONSTCHAR "rowColumn", argc, argv));
}


do_XmCreatePromptDialog(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreatePromptDialog();

	return(_xmcreatefunc(XmCreatePromptDialog, CONSTCHAR "selectionBox", argc, argv));
}


do_XmCreatePulldownMenu(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreatePulldownMenu();

	return(_xmcreatefunc(XmCreatePulldownMenu, CONSTCHAR "rowColumn", argc, argv));
}


do_XmCreatePushButton(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreatePushButton();

	return(_xmcreatefunc(XmCreatePushButton, CONSTCHAR "pushButton", argc, argv));
}


do_XmCreatePushButtonGadget(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreatePushButtonGadget();

	return(_xmcreatefunc(XmCreatePushButtonGadget, CONSTCHAR "pushButtonGadget", argc, argv));
}


do_XmCreateQuestionDialog(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateQuestionDialog();

	return(_xmcreatefunc(XmCreateQuestionDialog, CONSTCHAR "messageBox", argc, argv));
}


do_XmCreateRadioBox(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateRadioBox();

	return(_xmcreatefunc(XmCreateRadioBox, CONSTCHAR "rowColumn", argc, argv));
}


do_XmCreateRowColumn(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateRowColumn();

	return(_xmcreatefunc(XmCreateRowColumn, CONSTCHAR "rowColumn", argc, argv));
}


do_XmCreateScale(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateScale();

	return(_xmcreatefunc(XmCreateScale, CONSTCHAR "scale", argc, argv));
}


do_XmCreateScrollBar(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateScrollBar();

	return(_xmcreatefunc(XmCreateScrollBar, CONSTCHAR "scrollBar", argc, argv));
}


do_XmCreateScrolledList(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateScrolledList();

	return(_xmcreatefunc(XmCreateScrolledList, CONSTCHAR "list", argc, argv));
}


do_XmCreateScrolledText(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateScrolledText();

	return(_xmcreatefunc(XmCreateScrolledText, CONSTCHAR "text", argc, argv));
}


do_XmCreateScrolledWindow(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateScrolledWindow();

	return(_xmcreatefunc(XmCreateScrolledWindow, CONSTCHAR "scrolledWindow", argc, argv));
}


do_XmCreateSelectionBox(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateSelectionBox();

	return(_xmcreatefunc(XmCreateSelectionBox, CONSTCHAR "selectionBox", argc, argv));
}


do_XmCreateSelectionDialog(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateSelectionDialog();

	return(_xmcreatefunc(XmCreateSelectionDialog, CONSTCHAR "selectionBox", argc, argv));
}


do_XmCreateSeparator(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateSeparator();

	return(_xmcreatefunc(XmCreateSeparator, CONSTCHAR "separator", argc, argv));
}


do_XmCreateSeparatorGadget(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateSeparatorGadget();

	return(_xmcreatefunc(XmCreateSeparatorGadget, CONSTCHAR "separatorGadget", argc, argv));
}

do_XmCreateTextField(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateText();

	return(_xmcreatefunc(XmCreateTextField, CONSTCHAR "textField", argc, argv));
}


do_XmCreateText(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateText();

	return(_xmcreatefunc(XmCreateText, CONSTCHAR "text", argc, argv));
}


do_XmCreateToggleButton(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateToggleButton();

	return(_xmcreatefunc(XmCreateToggleButton, CONSTCHAR "toggleButton", argc, argv));
}


do_XmCreateToggleButtonGadget(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateToggleButtonGadget();

	return(_xmcreatefunc(XmCreateToggleButtonGadget, CONSTCHAR "toggleButtonGadget", argc, argv));
}


do_XmCreateWarningDialog(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateWarningDialog();

	return(_xmcreatefunc(XmCreateWarningDialog, CONSTCHAR "messageBox", argc, argv));
}


do_XmCreateWorkingDialog(argc, argv)
int argc;
char *argv[];
{
	Widget XmCreateWorkingDialog();

	return(_xmcreatefunc(XmCreateWorkingDialog, CONSTCHAR "messageBox", argc, argv));
}

static void
parse_indexlist(arg0, argc, argv, max, indices, numindices)
char *arg0;
int argc;
char *argv[];
int max;
int *indices;
int *numindices;
{
	register int i;

	*numindices = 0;
	for (i = 0; i < argc; i++) {
		if (!isdigit(argv[i][0])) {
			if (strcmp(argv[i], CONSTCHAR "last") == 0) {
				indices[*numindices] = max;
			} else {
				printerr(arg0, CONSTCHAR "index not an integer:", argv[i]);
				continue;
			}
		} else {
			indices[*numindices] = atoi(argv[i]);
			if (indices[*numindices] < 0 || indices[*numindices] > max) {
				printerr(arg0, CONSTCHAR "index out of range:", argv[i]);
				continue;
			}
		}
		(*numindices)++;
	}
}

static void
list_add_item(w, str, position, usrdata)
wtab_t *w;
char *str;
int position;
char *usrdata;
{
	XmString string = XmStringCreateLtoR(str, XmSTRING_DEFAULT_CHARSET);

	XmListAddItem(w->w, string, position);
}

static int
numitems(w)
Widget w;
{
	Arg arg[1];
	int num = 0;

	XtSetArg(arg[0], XmNitemCount, &num);
	XtGetValues(w, arg, 1);
	return(num);
}

static int
TextOp_usage(arg0)
char *arg0;
{
	printerr(arg0, usagemsg(CONSTCHAR "usage: %s [-gcsraidlGCS] Widgetid arg ...", arg0), NULL);
	return(1);
}

int
do_TextOp(argc, argv)
int argc;
char **argv;
{
	wtab_t *w;
	char *arg0 = argv[0];
	char *mode;
	int frompos, topos, len;
	char *str;

	if (argc < 3 || argv[1][0] != '-')
		return(TextOp_usage(arg0));

	mode = argv[1];
	w = str_to_wtab(argv[0], argv[2]);
	if (w == NULL) {
		return(1);
	}
	if (w->wclass->class != xmTextWidgetClass) {
		printerr(arg0, CONSTCHAR "handle must be a 'text' widget:", argv[1]);
		return(1);
	}
	argv += 3;
	argc -= 3;
	switch (mode[1]) {
	case 'g':	/* Get the text, print to stdout */
		str = XmTextGetString(w->w);
		if (str != NULL) {
			altputs(str);
			XtFree(str);
		}
		break;
	case 's':	/* Set the text */
		if (argc < 1) {
			XmTextSetString(w->w, "");
		} else {
			XmTextSetString(w->w, argv[0]);
		}
		break;
	case 'r':	/* Replace text at a position */
		if (argc != 3 || !isdigit(argv[0][0]) || !isdigit(argv[1][0])) {
			printerr(arg0, CONSTCHAR "textreplace: usage: textreplace widget from-pos to-pos string\n", NULL);
			return(1);
		}
		frompos = atoi(argv[0]);
		topos = atoi(argv[1]);
		str = argv[2];
		len = strlen(str);
                { int   total_len;
                total_len = strlen(XmTextGetString(w->w));
                if (frompos <= total_len) {
                        if (total_len < topos)
                                topos = total_len;
                        XmTextReplace(w->w, frompos, topos, str);
                }
                }
		break;
	case 'G':	/* Get the current selection */
		str = XmTextGetSelection(w->w);
		if (str != NULL)
			altputs(str);
		break;
	case 'S':	/* Set the current selection */
		if (argc != 2 || !isdigit(argv[0][0]) || !isdigit(argv[1][0])) {
			printerr(arg0, CONSTCHAR "textsetsel: usage: textsetsel widget from-pos to-pos\n", NULL);
			return(1);
		}
		frompos = atoi(argv[0]);
		topos = atoi(argv[1]);
		XmTextSetSelection(w->w, frompos, topos, CurrentTime);
		break;
	case 'C':	/* Clear the current selection */
		XmTextClearSelection(w->w, CurrentTime);
		break;
	case 'a':	/* append text to end of buffer */
		frompos = XmTextGetLastPosition(w->w);
		XmTextInsert(w->w, frompos, argv[0] ? argv[0] : "");
		break;
	case 'i':	/* insert text */
		if (argc != 2 || !isdigit(argv[0][0])) {
			printerr(arg0, CONSTCHAR "textinsert: usage: textinsert widget position string\n", NULL);
			return(1);
		}
		frompos = atoi(argv[0]);
		XmTextInsert(w->w, frompos, argv[1] ? argv[1] : "");
		break;
	case 'd':	/* display text at position */
		if (argc != 1 || !isdigit(argv[0][0])) {
			printerr(arg0, CONSTCHAR "textposition: usage: textposition widget position\n", NULL);
			return(1);
		}
		frompos = atoi(argv[0]);
		XmTextShowPosition(w->w, frompos);
		break;
	case 'l':	/* get last position, print to stdout */
		altprintf("%d\n", XmTextGetLastPosition(w->w));
		break;
	default:
		return(TextOp_usage(arg0));
	}
	return(0);
}

static int
ListOp_usage(arg0)
char *arg0;
{
	printerr(arg0, usagemsg(CONSTCHAR "usage: %s [-adgpsSDtb ] Widgetid arg ...", arg0), NULL);
	return(1);
}


int
do_ListOp(argc, argv)
int argc;
char **argv;
{
	wtab_t *w;
	char *arg0 = argv[0];
	int ind, next, indices[MAXARGS], numindices, usrdatamode = 0;
	Arg args[2];
	char *mode;
	register int i, j;
	int pos, num;
	char buf[4096];

	if (argc < 3 || argv[1][0] != '-')
		return(ListOp_usage(arg0));

	mode = argv[1];
	w = str_to_wtab(argv[0], argv[2]);
	if (w == NULL) {
		return(1);
	}
	if (w->wclass->class != xmListWidgetClass) {
		printerr(arg0, CONSTCHAR "handle must be a 'list' widget:", argv[1]);
		return(1);
	}
	argv += 3;
	argc -= 3;
	switch (mode[1]) {
	case 'a':
		pos = 0;
		for (i = 0; i < argc; i++) {
			if (strncmp(argv[i], CONSTCHAR "-P", 2) == 0) {
				if (argv[i][2] != 0) {
					pos = atoi(&argv[i][2]);
				} else {
					i++;
					if (argv[i])
						pos = atoi(argv[i]);
				}
				continue;
			}
			if (strncmp(argv[i], CONSTCHAR "-f", 2) == 0) {
				char fbuf[4096], usrdata[4096];
				FILE *fp;
				char *file;

				if (argv[i][2] != '\0')
					file = &argv[i][2];
				else if (argc > i+1) {
					i++;
					file = argv[i];
				} else {
					printerr(arg0, CONSTCHAR "Cannot find filename argument for add", NULL);
					continue;
				}
				if ((fp = fopen(file, CONSTCHAR "r")) == NULL) {
					printerr(arg0, CONSTCHAR "Cannot open file for reading:", file);
					return(1);
				}
				while (fgets(fbuf, sizeof(fbuf), fp) != NULL) {
					fbuf[strlen(fbuf)-1] = '\0';
					if (usrdatamode) {
						fgets(usrdata, sizeof(usrdata), fp);
					}
					list_add_item(w, fbuf, pos, usrdatamode ? usrdata : NULL);
					if (pos != 0)
						pos++;
				}
				fclose(fp);
			} else {
				char *udata = usrdatamode ? argv[i+1] : NULL;

				list_add_item(w, argv[i], pos, udata);
				if (pos != 0)
					pos++;
				if (udata && argv[i+1])
					i++;
			}
		}
		num = numitems(w->w);

		sprintf(buf, CONSTCHAR "%s_NUMITEMS=%d", w->envar, num);
		env_set(buf);
		break;
	case 'C':
		/* Change:  Remove all then add args fast */
		XmListDeleteAllItems(w->w);
		/*
		 * put data at a position args: position string string2 ...
		 */
		{
		XmString strings[MAXARGS];

                for (i = 0; i < argc; i++) {

			strings[i] = XmStringCreateLtoR(argv[i], XmSTRING_DEFAULT_CHARSET);
                }
		XmListAddItems(w->w, &strings[0], i, 0);
		for (j = 0; j < i; j++)
			XmStringFree(strings[j]);
		}
		break;
	case 'd':
		if (strcmp(mode, "-dall") == 0 || (argc >= 1 && strcmp(argv[0], CONSTCHAR "all")) == 0) {
			/*
			 * Delete them all
			 */
			XmListDeleteAllItems(w->w);
			sprintf(buf, CONSTCHAR "%s_NUMITEMS=0", w->envar);
			env_set(buf);
			sprintf(buf, CONSTCHAR "%s_CURITEM=", w->envar);
			env_set(buf);
			sprintf(buf, CONSTCHAR "%s_CURINDEX=", w->envar);
			env_set(buf);
			break;
		}
		if (strcmp(mode, "-dtext") == 0) {
			for (i = 0; i < argc; i++) {
				XmString string = XmStringCreateLtoR(argv[i], XmSTRING_DEFAULT_CHARSET);
				XmListDeleteItem(w->w, string);
			}
		} else {
			parse_indexlist(arg0, argc, argv, numitems(w->w), indices, &numindices);
			for (i = 0; i < numindices; i++) {
				XmListDeletePos(w->w, indices[i]);
			}
		}
		num = numitems(w->w);
		sprintf(buf, CONSTCHAR "%s_NUMITEMS=%d", w->envar, num);
		env_set(buf);
		break;
	case 'i':
		/*
		 * get selected positions.
		 *
		 * With no additional args, prints to stdout, otherwise
		 * the argument is assumed to be an environment variable
		 * name which is an array that will be set to hold all
		 * the selected items.
		 */
		{
			XmString *items;
			int numitems;
			char *buf = NULL;
			int buflen = 0, max;
			char *array = argv[0];
			int *pos_list;

			items = NULL;
			XtSetArg(args[0], XmNselectedItems, &items);
			XtSetArg(args[1], XmNselectedItemCount, &numitems);
			XtGetValues(w->w, args, 2);
			if (items == NULL) {
				printerr(arg0, CONSTCHAR "listgetsel: selectedItems is NULL in list:", w->envar);
				break;
			}

			pos_list = (int *)stakalloc((numitems+1)*sizeof(int));
			XmListGetSelectedPos(w->w, &pos_list, &numitems);
			if (array != NULL) {
				buflen = 24 + strlen(array);
				buf = stakalloc(buflen);
				sprintf(buf, CONSTCHAR "unset %s", array);
				sh_eval(buf);
			}
			for (i = 0; i < numitems; i++) {
				if (array != NULL) {
					max = strlen(array) + 64;
					if (buflen < max) {
						buf = stakalloc(buf, max);
						buflen = max;
					}
					sprintf(buf, CONSTCHAR "%s[%d]=%d", array, i, pos_list[i]);
					env_set(buf);
				} else {
					altprintf("%d\n", pos_list[i]);
				}
			}
		}
		break;
	case 'p':
		/*
		 * put data at a position args: position string pos2 string2 ...
		 */
                for (i = 0; i+1 < argc; i += 2) {
			XmString string;

			parse_indexlist(arg0, 1, &argv[i], numitems(w->w), indices, &numindices);
                        if (numindices != 1)
                                continue;
			string = XmStringCreateLtoR(argv[i+1], XmSTRING_DEFAULT_CHARSET);
			XmListReplaceItemsPos(w->w, &string, 1, indices[0]);
                }
                break;
	case 'R':
		/*
		 * put data at a position args: position string string2 ...
		 */
		{
			XmString strings[MAXARGS];

		parse_indexlist(arg0, 1, &argv[0], numitems(w->w), indices, &numindices);
		if (numindices != 1)
			break;
		argv++;
		argc--;
                for (i = 0; i < argc; i++) {

			strings[i] = XmStringCreateLtoR(argv[i], XmSTRING_DEFAULT_CHARSET);
                }
		XmListReplaceItemsPos(w->w, &strings[0], i, indices[0]);
		for (j = 0; j < i; j++)
			XmStringFree(strings[j]);
		}
                break;
	case 's':
		/*
		 * get selected items.
		 *
		 * With no additional args, prints to stdout, otherwise
		 * the argument is assumed to be an environment variable
		 * name which is an array that will be set to hold all
		 * the selected items.
		 */
		{
			int num;
			XmString *items;
			int numitems;
			char *buf = NULL;
			int buflen = 0, max;
			char *array = argv[0];

			items = NULL;
			XtSetArg(args[0], XmNselectedItems, &items);
			XtSetArg(args[1], XmNselectedItemCount, &numitems);
			XtGetValues(w->w, args, 2);
			if (items == NULL) {
				printerr(arg0, CONSTCHAR "listgetsel: selectedItems is NULL in list:", w->envar);
				break;
			}
			if (array != NULL) {
				/*
				 * allocate more than strictly necessary
				 * because same buffer used below in
				 * for loop
				 */
				buflen = 256 + strlen(array);
				buf = stakalloc(buflen);
				sprintf(buf, CONSTCHAR "unset %s", array);
				sh_eval(buf);
			}
			for (i = 0; i < numitems; i++) {
				char *str;

				str = XmStringToString(items[i]);
				if (items[i] != NULL && str != NULL) {
					if (array != NULL) {
						max = strlen(str) + strlen(array) + 16;
						if (buflen < max) {
							buf = stakalloc(buf, max);
							buflen = max;
						}
						sprintf(buf, CONSTCHAR "%s[%d]=%s", array, i, str);
						env_set(buf);
					} else {
						altprintf("%s\n", str);
					}
				} else {
					printerr(arg0, CONSTCHAR "listget: an item is NULL in list:", w->envar);
				}
				XtFree(str);
			}
		}
		break;
	case 'g':
		/*
		 * get data at a position args: position ...
		 */
		{
			int num;
			XmString *items;
			char *XmStringToString();

			items = NULL;
			XtSetArg(args[0], XmNitems, &items);
			XtSetArg(args[1], XmNitemCount, &num);
			XtGetValues(w->w, args, 2);
			if (items == NULL) {
				printerr(arg0, CONSTCHAR "listget: items is NULL in list:", w->envar);
				break;
			}
			if (argc >= 1 && strcmp(argv[0], "all") == 0) {
				for (i = 0; i < num; i++) {
					char *str;

					str = XmStringToString(items[i]);
					if (items[i] != NULL && str != NULL) {
						altputs(str);
					} else {
						printerr(arg0, CONSTCHAR "listget: an item is NULL in list:", w->envar);
					}
					XtFree(str);
				}
			} else {
				parse_indexlist(arg0, argc, argv, (num = numitems(w->w)), indices, &numindices);
				for (i = 0; i < numindices; i++) {
					char *str;

					if (indices[i] < 0 || indices[i] > num) {
						continue;
					} else if (indices[i] == 0) {
						ind = num-1;
					} else
						ind = indices[i]-1;
					if (ind < 0) {
						printerr(arg0, CONSTCHAR "There is no last item in list:", w->envar);
						continue;
					}
					if (items[ind] != NULL && 
					    (str = XmStringToString(items[ind]))
					    != NULL) {
						altputs(str);
						XtFree(str);
					} else {
						printerr(arg0, CONSTCHAR "listget: an item is NULL in list:", w->envar);
					}
				}
			}
		}
                break;
	case 'S':	/* select the named positions */
		{
			Boolean notify = FALSE;

			if (strcmp(argv[0], CONSTCHAR "-n") == 0) {
				notify = TRUE;
				argv++;
				argc--;
			}
			parse_indexlist(arg0, argc, argv, numitems(w->w), indices, &numindices);
			for (i = 0; i < numindices; i++) {
				XmListSelectPos(w->w, indices[i], notify);
			}
		}
		break;
	case 'D':	/* deselect the named positions */
                if (argc >= 1 && strcmp(argv[0], CONSTCHAR "all") == 0) {
                        /*
                         * Deselect them all
                         */
                        XmListDeselectAllItems(w->w);
                        break;
                }
		parse_indexlist(arg0, argc, argv, numitems(w->w), indices, &numindices);
		for (i = 0; i < numindices; i++) {
			XmListDeselectPos(w->w, indices[i]);
		}
		break;
	case 'b':	/* move the selected item to the bottom */
		parse_indexlist(arg0, argc, argv, numitems(w->w), indices, &numindices);
		for (i = 0; i < numindices; i++) {
			XmListSetBottomPos(w->w, indices[i]);
		}
		break;
	case 't':	/* move the selected item to the top */
		parse_indexlist(arg0, argc, argv, numitems(w->w), indices, &numindices);
		for (i = 0; i < numindices; i++) {
			XmListSetPos(w->w, indices[i]);
		}
		break;
	default:
		return(ListOp_usage(arg0));
	}
	return(0);
}

static int
setup_mnemonics(wid, tab, flag)
Widget wid;
char *tab;
Boolean flag;	/* if TRUE, then set mnemonics, else just find existing */
{
	register int i, n;
	XmString xmlabel;
	String label;
	Arg args[2];
	WidgetList children;
	Cardinal numchildren = 0;
	WidgetClass class;
	String resource;
	char *scoretab, *s;
	int bestscore;
	char bestchar;

	class = XtClass(wid);

	if (XtIsSubclass(wid, xmManagerWidgetClass) || XtIsShell(wid)) {
		/* These manager class widgets are
		 * traversed recursively.
		 */
		XtSetArg(args[0], XmNchildren, &children);
		XtSetArg(args[1], XmNnumChildren, &numchildren);
		XtGetValues(wid, args, 2);
		for (i = 0; i < numchildren; i++) {
			setup_mnemonics(children[i], tab, flag);
		}
	} else {
		resource = NULL;
		if (XtIsSubclass(wid, xmLabelWidgetClass) ||
		    XtIsSubclass(wid, xmLabelGadgetClass)) {
			resource = XmNlabelString;
		}
		if (flag && resource != NULL) {
			Arg kidargs[2];
			KeySym m;
			char *p;

			label = NULL;
			XtSetArg(kidargs[0], XmNlabelString, (XtArgVal)&xmlabel);
			XtSetArg(kidargs[1], XmNmnemonic, (XtArgVal)&m);
			XtGetValues(wid, kidargs, 2);
			if (m != '\0' || xmlabel == NULL)	/* already has one */
				return(0);
			label = XmStringToString(xmlabel);
			/*
			 * Go through each character, assigning a "score"
			 * The scores are as follows:
			 *
			 *    Already used or space:   0, no bonuses apply
			 *
			 *    Not used:               10
			 *    Alpha character:        +5
			 *    Upper Case:             +2
			 *    Punctuation char:	      -6
			 *    First char in a "word": +2
			 */
			scoretab = stakalloc(n = (strlen(label)));
			for (p = label, s = scoretab; *p; p++, s++) {
				if (!isspace(*p) && tab[*p] == '\0') {
					*s = 10;
					if (p == label || isspace(p[-1]))
						*s += 2;
					if (isalpha(*p))
						*s += 5;
					if (ispunct(*p))
						*s -= 6;
					if (isupper(*p))
						*s += 2;
				} else {
					*s = 0;
				}
			}
			for (bestchar = '\0', bestscore = 0, i = 0; i < n; i++) {
				if ((int)(scoretab[i]) > bestscore) {
					bestscore = (int)(scoretab[i]);
					bestchar = label[i];
				}
			}

			XtFree(label);
			XtSetArg(args[0], XmNmnemonic, (XtArgVal)bestchar);
			XtSetValues(wid, args, 1);
			tab[bestchar] = 1;
			if (isupper(bestchar))
				tab[tolower(bestchar)] = 1;
			else if (islower(bestchar))
				tab[toupper(bestchar)] = 1;
			if (bestchar == '\0') {
				printerr(CONSTCHAR "MnemonicOp", CONSTCHAR "unable to find unused character for ", label);
			}
		} else if (resource != NULL) {
			KeySym m;

			XtSetArg(args[0], XmNmnemonic, (XtArgVal)&m);
			XtGetValues(wid, args, 1);
			if (m != '\0')
				tab[toupper(m)] = tab[tolower(m)] = 1;
		}
	}
	return(0);
}

static int
MnemonicOp_usage(arg0)
char *arg0;
{
	printerr(arg0, usagemsg(CONSTCHAR "usage: %s [ -s | -c ] widget ...", arg0), NULL);
	return(1);
}


int
do_MnemonicOp(argc, argv)
int argc;
char *argv[];
{
	register int i;
	wtab_t *w;
	char *arg0 = argv[0];
	char mode;
	char tab[256]; /* table of used chars */

	if (argc < 3 || argv[1][0] != '-')
		return(MnemonicOp_usage(arg0));

	mode = argv[1][1];
	argv += 2;
	argc -= 2;

	switch (mode) {
	case 's':		/* set up menmonics */
		memset(tab, '\0', sizeof(tab));
		/*
		 * First, make a pass over the widgets and track any
		 * ones that have existing mnemonics so we don't collide.
		 */
		for (i = 0; i < argc; i++) {
			w = str_to_wtab(arg0, argv[i]);
			if (w == NULL)
				continue;
			setup_mnemonics(w->w, &tab[0], FALSE);
		}
		/*
		 * Now, make a second pass over the widgets and assign
		 * mnemonics to any that don't have them.
		 */
		for (i = 0; i < argc; i++) {
			w = str_to_wtab(arg0, argv[i]);
			if (w == NULL)
				continue;
			setup_mnemonics(w->w, &tab[0], TRUE);
		}
		break;
	case 'c':
	default:
		return(MnemonicOp_usage(arg0));
	}
	return(0);
}

static int
print_widgetdata(wid, shortflag)
Widget wid;
Boolean shortflag;
{
	register int i;
	Arg args[2];
	WidgetList children;
	Cardinal numchildren = 0;
	String widname, str;
	Boolean bool;
	int val;
	WidgetClass class;

	widname = XtName(wid);
	if (strcmp(widname, CONSTCHAR "nodata") == 0)
		return(0);
	class = XtClass(wid);

	if (XtIsSubclass(wid, xmManagerWidgetClass) || XtIsShell(wid)) {
		/* These manager class widgets are
		 * traversed recursively.
		 */
		XtSetArg(args[0], XmNchildren, &children);
		XtSetArg(args[1], XmNnumChildren, &numchildren);
		XtGetValues(wid, args, 2);
		for (i = 0; i < numchildren; i++) {
			print_widgetdata(children[i], shortflag);
		}
	} 
	if (class == xmTextWidgetClass || class == xmTextFieldWidgetClass) {
		char *str = NULL, *cvt;

		XtSetArg(args[0], XmNvalue, &str);
		XtGetValues(wid, args, 1);
		if (str != NULL) {
			char *p, *q;

			cvt = stakalloc(strlen(str)*4 + 1);
			p = cvt;
                        q = str;
                        while (*q) {
                                if (*q == '\n') {
                                        *p++ = '\\';
                                        *p++ = 'n';
                                        q++;
                                } else if (*q == '\\') {
                                        *p++ = '\\';
                                        *p++ = '\\';
                                        q++;
                                } else {
                                        *p++ = *q++;
                                }
                        }
                        *p++ = '\0';
			XFree(str);
		} else
			cvt = "";
		if (shortflag)
			altprintf("%s|", cvt);
		else
			altprintf("%s=%s\n", widname, cvt);
	} else if (class == xmScaleWidgetClass) {
		XtSetArg(args[0], XmNvalue, &val);
		XtGetValues(wid, args, 1);
		if (shortflag)
			altprintf("%d|", val);
		else
			altprintf("%s=%d\n", widname, val);
	} else if (class == xmToggleButtonWidgetClass || class == xmToggleButtonGadgetClass) {
		XtSetArg(args[0], XmNset, &val);
		XtGetValues(wid, args, 1);
		if (shortflag)
			altprintf("%s|", val ? "s" : "u");
		else
			altprintf("%s=%s\n", widname, val ? "set" : "unset");
	}
	return(0);
}

static int
next_line(data, name, val, shortflag)
char **data, **name, **val;
Boolean shortflag;
{
	register char *p;
	
	if (*data == NULL)
		return(1);

	if (shortflag)
		p = strchr(*data, '|');
	else
		p = strchr(*data, '=');

	if (p == NULL) {
		*name = *val = "";
		return(1);
	}
	if (shortflag) {
		*p = '\0';
		*name = "";
		*val = *data;
		*data = &p[1];
	} else {
		*p = '\0';
		*name = *data;
		*val = &p[1];
		p = strchr(&p[1], '\n');
		if (p != NULL) {
			*data = &p[1];
			p[0] = '\0';
		} else
			*data = NULL;
	}
	return(0);
}

static int
parse_widgetdata(wid, data, shortflag)
Widget wid;
char **data;
Boolean shortflag;
{
	register int i;
	register char *p;
	Arg args[2];
	WidgetList children;
	Cardinal numchildren = 0;
	String widname, name, str;
	Boolean bool;
	int val;
	WidgetClass class;
	char *cvt;

	widname = XtName(wid);
	if (strcmp(widname, CONSTCHAR "nodata") == 0)
		return(0);

	class = XtClass(wid);

	if (XtIsSubclass(wid, xmManagerWidgetClass) || XtIsShell(wid)) {
		/* These manager class widgets are
		 * traversed recursively.
		 */
		XtSetArg(args[0], XmNchildren, &children);
		XtSetArg(args[1], XmNnumChildren, &numchildren);
		XtGetValues(wid, args, 2);
		for (i = 0; i < numchildren; i++) {
			if (parse_widgetdata(children[i], data, shortflag) != 0) {
				return(1);
			}
		}
	} 
	if (class == xmTextWidgetClass || class == xmTextFieldWidgetClass) {
		if (next_line(data, &name, &str, shortflag) != 0) {
			return(1);
		}
		if (!shortflag && strcmp(name, widname) != 0) {
			return(1);
		}
                cvt = stakalloc(strlen(str)+3);

                /* un-escape newlines and backslashes */
                p = cvt;
                while (*str) {
                        if (*str == '\\' && str[1] == 'n') {
                                *p++ = '\n';
                                str += 2;
                        } else if (*str == '\\' && str[1] == '\\') {
                                *p++ = '\\';
                                str += 2;
                        } else
                                *p++ = *str++;
                }
                *p = '\0';

		XtSetArg(args[0], XmNvalue, cvt);
		XtSetValues(wid, args, 1);
	} else if (class == xmScaleWidgetClass) {
		if (next_line(data, &name, &str, shortflag) != 0) {
			return(1);
		}
		if (!shortflag && strcmp(name, widname) != 0) {
			return(1);
		}
		val = atoi(str);
		XtSetArg(args[0], XmNvalue, (XtArgVal) val);
		XtSetValues(wid, args, 1);
	} else if (class == xmToggleButtonWidgetClass || class == xmToggleButtonGadgetClass) {
		if (next_line(data, &name, &str, shortflag) != 0) {
			return(1);
		}
		if (!shortflag && strcmp(name, widname) != 0) {
			return(1);
		}
		if (str[0] == 's') {
			val = TRUE;
		} else {
			val = FALSE;
		}
		XtSetArg(args[0], XmNset, (XtArgVal) val);
		XtSetValues(wid, args, 1);
	}
	return(0);
}

static int
DataOp_usage(arg0)
char *arg0;
{
	printerr(arg0, usagemsg(CONSTCHAR "usage: %s [ -p | -s | -c | -v ] args ...", arg0), NULL);
	return(1);
}

/*
 * For parsing, the first arg is the widget tree, the next, which
 * should be a single string, is the datum to be set with newline
 * separated lines.  There can be multiple such pairs of args.
 */

int
do_DataOp(argc, argv)
int argc;
char *argv[];
{
	register int i;
	wtab_t *w;
	char *datum, *freedatum;
	char *arg0 = argv[0];
	char *mode;
	Boolean shortflag = 0;

	if (argc < 3 || argv[1][0] != '-')
		return(DataOp_usage(arg0));

	mode = &argv[1][1];
	argv += 2;
	argc -= 2;

	switch (*mode) {
	case 'r':
		if (mode[1] == 's') {
			shortflag++;
		}
		for (i = 0; i < argc; i += 2) {
			w = str_to_wtab(argv[0], argv[i]);
			if (w == NULL)
				continue;
			datum = argv[i+1];
			if (datum == NULL) {
				printerr(arg0, usagemsg(CONSTCHAR "usage: %s -r widget \"data\" ...", arg0), NULL);
			}
			freedatum = datum = strdup(datum);
			if (parse_widgetdata(w->w, &datum, shortflag) != 0) {
				XtFree(freedatum);
				return(1);
			}
			XtFree(freedatum);
		}
		break;
	case 'p':
		if (mode[1] == 's') {
			shortflag++;
		}
		for (i = 0; i < argc; i++) {
			w = str_to_wtab(arg0, argv[i]);
			if (w == NULL)
				continue;
			print_widgetdata(w->w, shortflag);
		}
		break;
	default:
		return(DataOp_usage(arg0));
	}
	return(0);
}

int
do_XmMainWindowSetAreas(argc, argv)
int argc;
char *argv[];
{
	char *arg0 = argv[0];
	wtab_t *w[6];
	register int i;

	if (argc != 7) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s mainwindow menu command hscroll vscroll work", argv[0]), NULL);
		return(1);
	}
	for (i = 1; i < 7; i++) {
		if (argv[i][0] == '\0' || strcmp(argv[i], (char *)(CONSTCHAR "NULL")) == 0) {
			w[i-1] = NULL;
			continue;
		}
		w[i-1] = str_to_wtab(arg0, argv[i]);
		if (w[i-1] == NULL) {
			continue;
		}
	}
	if (w[0] == NULL) {
		printerr(argv[0], CONSTCHAR "XmMainWindow is NULL", NULL);
		return(1);
	}
	XmMainWindowSetAreas(w[0]->w, w[1] ? w[1]->w : NULL, 
		w[2] ? w[2]->w : NULL, w[3] ? w[3]->w : NULL,
		w[4] ? w[4]->w : NULL, w[5] ? w[5]->w : NULL);
	return(0);
}

int
do_XmProcessTraversal(argc, argv)
int argc;
char *argv[];
{
	XrmValue f, t;
	wtab_t *w;
	int direction;

	if (argc != 3 && argc != 2) {
		printerrf(argv[0], "usage: %s widget [direction]", argv[0]);
		return(1);
	}

	w = str_to_wtab(argv[0], argv[1]);
	if (w == NULL)
		return(1);
	if (argc == 3) {
		f.addr = argv[2];
		f.size = strlen(argv[2]) + 1;
		t.addr = NULL;
		t.size = 0;
		XtConvert(w->w, XtRString, &f, CONSTCHAR "TraversalDirection", &t);
		if (t.size && t.addr) {
			direction = ((int *)(t.addr))[0];
		} else {
			printerr(argv[0], CONSTCHAR "Unknown traversal direction: ", argv[2]);
			return(1);
		}
	} else {
		direction = XmTRAVERSE_CURRENT;
	}
	return(!XmProcessTraversal(w->w, direction));
}

static XtCallbackProc
stdProtocolCB(w, clientData, callData)
Widget w;
caddr_t clientData, callData;
{
	ksh_eval((char *)clientData);
	free(clientData);
	return;
}

int
do_XmAddProtocolCallback(argc, argv)
int argc;
char *argv[];
{
	Widget handle_to_widget();
	Widget wid;
	Atom protocols, action;
	Display *dpy;
	char *arg0 = argv[0];

	if (argc != 5) {
		printerrf(argv[0], "Usage: XmAddProtocolCallback $shell protocol action callback");
		printerrf(argv[0], "Example: XmAddProtocolCallback $TOPLEVEL WM_PROTOCOLS WM_DELETE_WINDOW 'echo I'm dying!'");
		return(1);
	}
	if ((wid = handle_to_widget(argv[0], argv[1])) == NULL)
		return(1);
	if ( !XtIsRealized(wid) ) {
		printerrf(arg0, "Cannot add protocols to unrealized widget");
		return(1);
	}
	dpy = XtDisplay(wid);
	protocols = XmInternAtom(dpy, argv[2], FALSE);
	action = XmInternAtom(dpy, argv[3], FALSE);
	XmAddProtocolCallback(wid, protocols, action,
			   stdProtocolCB, strdup(argv[4]));
	return(0);
}
