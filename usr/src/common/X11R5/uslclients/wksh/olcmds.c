/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:olcmds.c	1.10"

/* X/OL includes */

#include <stdio.h>
#include <signal.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/OpenLookP.h>

#include <Xol/AbbrevMenu.h>
#include <Xol/BulletinBo.h>
#include <Xol/Caption.h>
#include <Xol/CheckBox.h>
#include <Xol/ControlAre.h>
#include <Xol/Exclusives.h>
#include <Xol/FCheckBox.h>
#include <Xol/FCheckBoxP.h>
#include <Xol/FExclusivP.h>
#include <Xol/FNonexcluP.h>
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
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/Flat.h>
#ifdef MOOLIT
#include <Xol/IntegerFie.h>
#include <Xol/StepField.h>
#endif
#include <setjmp.h>
#include <ctype.h>
#include "wksh.h"
#include "olksh.h"

#define MAXARGS 1024
#define SLISTITEMSIZE	32	/* amount to increment internal list by on realloc */

extern Widget Toplevel;	/* set by do_OlInitialize */

#ifndef MEMUTIL
extern char *strdup();
#endif /* MEMUTIL */
extern char *strchr();

static const char str_APPNAME[] = "APPNAME";
static const char str_TOPLEVEL[] = "TOPLEVEL";
char *stakalloc();

void (*toolkit_addcallback)() = OlAddCallback;
void (*toolkit_callcallbacks)() = OlCallCallbacks;

Boolean
IsFlatClass(c)
WidgetClass c;
{
	for ( ; c != (WidgetClass)NULL; c = c->core_class.superclass) {
		if (c == flatWidgetClass)
			return(TRUE);
	}
	return(FALSE);
}

int
toolkit_initialize(argc, argv)
int argc;
char *argv[];
{
	int i;
	char name[8], *var, *env_get();
	wtab_t *w, *set_up_w();
	int newargc;
	char *newargv[256];
	char envbuf[2048];

	extern void WkshCvtStringToPixmap();
	extern void WkshCvtStringToPointer();
	extern void XmuCvtStringToBitmap();
	extern void WkshCvtStringToOlDefineInt();
	extern void WkshCvtStringToXImage();
	extern void WkshCvtStringToOlVirtualName();
	extern void WkshCvtStringToOlChangeBarDefine();
	extern void WkshCvtOlChangeBarDefineToString();
	extern void WkshCvtOlDefineIntToString();
	extern void WkshCvtWidgetToString();
	extern void WkshCvtStringToWidget();
	extern void WkshCvtIntToString();
	extern void WkshCvtBooleanToString();
	extern void WkshCvtStringToCallback();
	extern void WkshCvtStringToCallbackProc();
	extern void WkshCvtCallbackToString();
	extern void WkshCvtHexIntToString();
	extern void WkshCvtOlDefineToString();


	init_widgets();
	if (argc < 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s variable name class [args...]\n", argv[0]), NULL);
		return(1);
	}
	newargv[0] = NULL;
	newargc = 0;

	if (argc < 4) {
		Toplevel = OlInitialize((char *)str_wksh, (char *)str_wksh, NULL, 0, 0, NULL);
	} else {
		int oiargc = argc - 3;

		Toplevel = OlInitialize(argv[2], argv[3], NULL, 0, &oiargc, &argv[3]);
		for (i = 0 ; i < oiargc && newargc < sizeof(newargv)/sizeof(char *) - 1; i++, newargc++) {
			newargv[newargc] = argv[i+3];
		}
		newargv[newargc] = NULL;
	}
	if (Toplevel == NULL) {
		printerr(argv[0], CONSTCHAR "Could not initialize OPEN LOOK Toolkit", NULL);
		env_blank(argv[1]);
		return(1);
	}
	XtAddConverter(str_XtRString, str_XtROlChangeBarDefine, 
		WkshCvtStringToOlChangeBarDefine, NULL, 0);
	XtAddConverter(str_XtRString, XtRPointer, 
		WkshCvtStringToPointer, NULL, 0);
	XtAddConverter(str_XtRString, XtRCallbackProc, 
		WkshCvtStringToCallbackProc, NULL, 0);
	XtAddConverter(str_XtROlChangeBarDefine, str_XtRString,
		WkshCvtOlChangeBarDefineToString, NULL, 0);
	XtAddConverter(str_XtRString, str_XtROlDefineInt, 
		WkshCvtStringToOlDefineInt, NULL, 0);
	XtAddConverter(str_XtROlDefineInt, str_XtRString, 
		WkshCvtOlDefineIntToString, NULL, 0);
	XtAddConverter(XtRShort, str_XtRString, 
		WkshCvtIntToString, NULL, 0);
	XtAddConverter(str_XtRString, str_XtRBitmap, 
		XmuCvtStringToBitmap, NULL, 0);
	XtAddConverter(str_XtRString, CONSTCHAR "XImage", 
		WkshCvtStringToXImage, (XtConvertArgList) screenConvertArg, 1);
	XtAddConverter(str_XtRString, str_XtROlVirtualName, 
		WkshCvtStringToOlVirtualName, NULL, 0);
	XtAddConverter(str_XtRString, str_XtRPixmap, 
		WkshCvtStringToPixmap, (XtConvertArgList) screenConvertArg, 1);
	XtSetTypeConverter(str_XtRString, XtRWidget,
		WkshCvtStringToWidget, NULL, 0, XtCacheNone, NULL);
	XtAddConverter(str_XtRWidget, str_XtRString, 
		WkshCvtWidgetToString, NULL, 0);
	XtAddConverter(str_XtRInt, str_XtRString, 
		WkshCvtIntToString, NULL, 0);
	XtAddConverter(str_XtRCardinal, str_XtRString, 
		WkshCvtIntToString, NULL, 0);
	XtAddConverter(str_XtRDimension, str_XtRString, 
		WkshCvtIntToString, NULL, 0);
	XtAddConverter(str_XtRPosition, str_XtRString, 
		WkshCvtIntToString, NULL, 0);
	XtAddConverter(str_XtRBoolean, str_XtRString, 
		WkshCvtBooleanToString, NULL, 0);
	XtAddConverter(str_XtRBool, str_XtRString, 
		WkshCvtBooleanToString, NULL, 0);
	XtSetTypeConverter(str_XtRString, str_XtRCallback,
		WkshCvtStringToCallback, NULL, 0, XtCacheNone, NULL);
	XtAddConverter(str_XtRCallback, str_XtRString, 
		WkshCvtCallbackToString, NULL, 0);
	XtAddConverter(str_XtROlDefine, str_XtRString, 
		WkshCvtOlDefineToString, NULL, 0);
	XtAddConverter(str_XtRPointer, str_XtRString, 
		WkshCvtHexIntToString, NULL, 0);
	XtAddConverter(str_XtRPixel, str_XtRString, 
		WkshCvtHexIntToString, NULL, 0);
	XtAddConverter(str_XtRPixmap, str_XtRString, 
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
#ifdef MOOLIT
	env_set((char *)"WKSH_API=MOOLIT");
#else
	env_set((char *)"WKSH_API=OPEN_LOOK");
#endif

	ksh_eval(CONSTCHAR "unset ARGV ARGC");
	for (i = 0; i < newargc; i++) {
		sprintf(envbuf, CONSTCHAR "ARGV[%d]=%s", i, newargv[i]);
		env_set(envbuf);
	}
	sprintf(envbuf, CONSTCHAR "ARGC=%d", newargc);
	env_set(envbuf);
#if 0
	{
		Arg args[1];

		XtSetArg(args[0], XtNwmProtocolInterested, (XtArgVal)OL_WM_DELETE_WINDOW|OL_WM_SAVE_YOURSELF|OL_WM_TAKE_FOCUS);
		XtSetValues(Toplevel, args, 1);
	}
#endif

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
int firstcall;	/* nonzero means this is the first call within a widget */
{
	int flatitems;

	/*
	 * Unfortunately, we have to fudge this to work with
	 * flats.  You see, flat converters require the widget id as an arg.
	 * However, this might be called from XtCreateWidget() in which
	 * case there is no widget id existing yet.  However, the flat
	 * stuff cannot be used after the widget is created.  Chicken
	 * and egg problem!
	 *
	 * So, we must treat this as a special case.
	 */
	if (w == NULL && IsFlatClass(class->class) && (
		(flatitems = (strcmp(res->resource_name, XtNitems) == 0)) ||
		strcmp(res->resource_name, XtNitemFields) == 0)) {

		static String *itemfields;
		static int numitemfields;
		static XtArgVal *items;
		static int numitems;

		if (flatitems) {
			register int i;

			WkshCvtStringToFlatItems(val, &items, &numitems, &itemfields, &numitemfields, class);
			*flatres = strdup(XtNnumItems);
			*flatret = numitems;
			*ret = (XtArgVal)items;
			*freeit = FALSE;
		} else {

			/*
			 * This is a fudged version of the flat converter
			 * that does not require the widget to exist yet.
			 */

			WkshCvtStringToFlatItemFields(val, &itemfields, &numitemfields, class);
			*flatres = strdup(XtNnumItemFields);
			*flatret = numitemfields;
			*ret = (XtArgVal)itemfields;
			*freeit = FALSE;
		}
		return(TRUE);
	}
	return(FALSE);
}


int
do_TextEditOp(argc, argv)
int argc;
char **argv;
{
	wtab_t *w;
	char *arg0 = argv[0];
	Arg arg[1];
	char *mode;
	char *buf;
	register int i, j;

	if (argc < 3 || argv[1][0] != '-') {
		printerr(arg0, usagemsg(CONSTCHAR "usage: %s [-c|-r|-e] Widgetid arg ...", arg0), NULL);
		return(1);
	}
	mode = argv[1];
	w = str_to_wtab(argv[0], argv[2]);
	if (w == NULL) {
		return(1);
	}
	if (w->wclass->class != textEditWidgetClass) {
		printerr(arg0, "handle must be textEditWidgetClass:", argv[1]);
		return(1);
	}
	argv += 3;
	argc -= 3;

	switch (mode[1]) {
	case 'c':
		if (OlTextEditSetCursorPosition(w->w, 0, 0, 0))
			return(!OlTextEditClearBuffer(w->w));
		else
			return(1);
	case 'r':
		return(!OlTextEditRedraw(w->w));
	case 'e':
		if (OlTextEditCopyBuffer(w->w, &buf)) {
			if (buf != NULL) {
				altputs(buf);
				if (buf[strlen(buf)-1] != '\n')
					altprintf("\n");
				XtFree(buf);
			}
			return(0);
		} else {
			printerr(arg0, usagemsg(CONSTCHAR "%s: not a text edit widget", arg0), w->wname);
			return(1);
		}
	default:
		printerr(arg0, usagemsg(CONSTCHAR "usage: %s [-c | -e | -r | -s | -S | -C ] Widgetid arg ...", arg0), NULL);
		return(1);
	}
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
				/*
				 * if there are no items in the list, then
				 * "last" makes no sense, so just skip it
				 * in that case.
				 */
				if (max > 0)
					indices[*numindices] = max-1;
				else
					continue;
			} else {
				printerr(arg0, "index not an integer:", argv[i]);
				continue;
			}
		} else {
			indices[*numindices] = atoi(argv[i]);
			if (indices[*numindices] < 0 || indices[*numindices] >= max) {
				printerr(arg0, "index out of range:", argv[i]);
				continue;
			}
		}
		(*numindices)++;
	}
}

static void
sl_add_item(w, str, item, ref, list, ind, addfn, usrdata)
wtab_t *w;
char *str;
OlListItem item;
OlListToken ref;
SListInfo_t *list;
int ind;
OlListToken (*addfn)();
char *usrdata;
{
	OlListToken tok;
	register int i, j;

	item.label_type = (OlDefine)OL_STRING;
	item.label = (String)strdup(str);
	item.attr = NULL;
	item.mnemonic = '\0';
	if (usrdata)
		usrdata = strdup(usrdata);
	item.user_data = usrdata;
	tok = (*addfn)(w->w, NULL, ref, item);
	/*
	 * Associate the token with the list
	 */
	/* grow the array if needed */
	if (list->lastitem >= list->maxitems-1) {
		list->maxitems += SLISTITEMSIZE;
		if (list->items == NULL) {
			list->items = (OlListToken *)XtMalloc(list->maxitems*sizeof(OlListToken));
		} else {
			list->items = (OlListToken *)XtRealloc((char *)list->items, list->maxitems*sizeof(OlListToken));
		}
	}
	if (ref) {
		for (j = list->lastitem; j >= ind; j--)
			list->items[j+1] = list->items[j];
		list->items[ind] = tok;
		ref = tok;
	} else {
		list->items[list->lastitem] = tok;
	}
	list->lastitem++;
}

int
do_ScrollingListOp(argc, argv)
int argc;
char **argv;
{
	wtab_t *w;
	char *arg0 = argv[0];
	int ind, next, indices[MAXARGS], numindices;
	OlListItem item, *ptr;
	OlListToken tok, ref;
	OlListToken (*addfn)(), (*deletefn)(), (*viewfn)(), (*touchfn)(),
			(*editfn)(), (*updatefn)();
	OlBitMask attrib;
	Arg arg[1];
	char *mode;
	SListInfo_t *list;
	register int i, j;
	int usrdatamode = 0;
	char buf[4096];

	if (argc < 3 || argv[1][0] != '-') {
		printerr(arg0, usagemsg(CONSTCHAR "usage: %s [-a | -g | -d | -v | -t | -e | -i | -c | -n | -u ] Widgetid arg ...", arg0), NULL);
		return(1);
	}
	mode = argv[1];
	w = str_to_wtab(argv[0], argv[2]);
	if (w == NULL) {
		return(1);
	}
	if (w->wclass->class != scrollingListWidgetClass) {
		printerr(arg0, "handle must be scrollingListWidgetClass:", argv[1]);
		return(1);
	}
	argv += 3;
	argc -= 3;
	list = (SListInfo_t *)(w->info);
	switch (mode[1]) {
	case 'a':
		ref = NULL;
		XtSetArg(arg[0], XtNapplAddItem, &addfn);
		XtGetValues(w->w, arg, 1);
		for (i = 0; i < argc; i++) {
			if (strncmp(argv[i], "-I", 2) == 0) {
				if (argv[i][2] != 0) {
					ind = atoi(&mode[2]);
				} else {
					i++;
					if (argv[i])
						ind = atoi(argv[i]);
				}
				if (ind >= 0 && ind < list->lastitem) {
					ref = list->items[ind];
				}
				continue;
			}
			if (strcmp(argv[i], "-U") == 0) {
				usrdatamode++;
				continue;
			}
			if (strncmp(argv[i], "-f", 2) == 0) {
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
				if ((fp = fopen(file, "r")) == NULL) {
					printerr(arg0, CONSTCHAR "Cannot open file for reading:", file);
					return(1);
				}
				while (fgets(fbuf, sizeof(fbuf), fp) != NULL) {
					fbuf[strlen(fbuf)-1] = '\0';
					if (usrdatamode) {
						fgets(usrdata, sizeof(usrdata), fp);
					}
					sl_add_item(w, fbuf, item, ref, list, ind, addfn, usrdatamode ? usrdata : NULL);
				}
				fclose(fp);
			} else {
				char *udata = usrdatamode ? argv[i+1] : NULL;
				sl_add_item(w, argv[i], item, ref, list, ind, addfn, udata);
				if (udata && argv[i+1])
					i++;
			}
		}
		sprintf(buf, "%s_NUMITEMS=%d", w->envar, list->lastitem);
		env_set(buf);
		break;
	case 'd':
		XtSetArg(arg[0], XtNapplDeleteItem, &deletefn);
		XtGetValues(w->w, arg, 1);
		if (argc >= 1 && strcmp(argv[0], "all") == 0) {
			/*
			 * Delete them all
			 */
			for (j = 0; j < list->lastitem; j++) {
				ptr = OlListItemPointer(list->items[j]);
				(*deletefn)(w->w, list->items[j]);
				if (ptr->user_data != NULL)
					XtFree(ptr->user_data);
				if (ptr->label != NULL)
					XtFree(ptr->label);
				ptr->label = ptr->user_data = NULL;
			}
			list->lastitem = 0;
			sprintf(buf, "%s_NUMITEMS=0", w->envar);
			env_set(buf);
			sprintf(buf, "%s_CURITEM=", w->envar);
			env_set(buf);
			sprintf(buf, "%s_CURINDEX=", w->envar);
			env_set(buf);
			break;
		}
		/*
		 * Have to do this in two passes, in the first pass, the
		 * items are deleted from the scrollinglist widget, in
		 * the second pass we delete them from our internal list.
		 * We have to do it this way because we want to maintain
		 * consistant indices.
		 */
		parse_indexlist(arg0, argc, argv, list->lastitem, indices, &numindices);
		for (i = 0; i < numindices; i++) {
			ptr = OlListItemPointer(list->items[indices[i]]);
			(*deletefn)(w->w, list->items[indices[i]]);
			if (ptr->user_data)
				XtFree(ptr->user_data);
			if (ptr->label)
				XtFree(ptr->label);
			ptr->label = ptr->user_data = NULL;
		}
		/*
		 * compress the list, looking for items with NULL labels
		 * and deleting them.
		 */
		for (i = 0, next = 0; i < list->lastitem; i++) {
			ptr = OlListItemPointer(list->items[i]);
			if (ptr->label != NULL) {
				if (next != i)
					list->items[next] = list->items[i];
				next++;
			}
		}
		list->lastitem = next;
		sprintf(buf, "%s_NUMITEMS=%d", w->envar, list->lastitem);
		env_set(buf);
		break;
	case 'v':
		XtSetArg(arg[0], XtNapplViewItem, &viewfn);
		XtGetValues(w->w, arg, 1);
		parse_indexlist(arg0, argc, argv, list->lastitem, indices, &numindices);
		for (i = 0; i < numindices; i++) {
			(*viewfn)(w->w, list->items[indices[i]]);
		}
		break;
	case 'S':	/* Make named items selected */
	case 'U':	/* Make named items unselected */
		XtSetArg(arg[0], XtNapplTouchItem, &touchfn);
		XtGetValues(w->w, arg, 1);
		if (argc >= 1 && strcmp(argv[0], CONSTCHAR "all") == 0) {
			for (i = 0; i < list->lastitem; i++) {
				ptr = OlListItemPointer(list->items[i]);
				ptr->attr = (ptr->attr & OL_LIST_ATTR_APPL) | attrib;
				(*touchfn)(w->w, list->items[i]);
			}
			return(0);
		}
		parse_indexlist(arg0, argc, argv, list->lastitem, indices, &numindices);
		attrib = (mode[1] == 'S') ? OL_LIST_ATTR_CURRENT : 0;
		for (i = 0; i < numindices; i++) {
			ptr = OlListItemPointer(list->items[indices[i]]);
			ptr->attr = (ptr->attr & OL_LIST_ATTR_APPL) | attrib;
			(*touchfn)(w->w, list->items[indices[i]]);
		}
		break;
	case 'L':	/* List selected item indexes */
		for (i = 0; i < list->lastitem; i++) {
			ptr = OlListItemPointer(list->items[i]);
			if (ptr->attr & OL_LIST_ATTR_CURRENT) {
				altprintf("%d\n", i);
			}
		}
		break;
	case 'g':
		if (argc >= 1 && strcmp(argv[0], "all") == 0) {
			for (i = 0; i < list->lastitem; i++) {
				ptr = OlListItemPointer(list->items[i]);
				altprintf("%s\n", ptr->label);
			}
		} else {
			parse_indexlist(arg0, argc, argv, list->lastitem, indices, &numindices);
			for (i = 0; i < numindices; i++) {
				ptr = OlListItemPointer(list->items[indices[i]]);
				altprintf("%s\n", ptr->label);
			}
		}
		break;
	case 'p':	/* put an item at an index, takes pairs */
		for (i = 0; i+1 < argc; i += 2) {
			parse_indexlist(arg0, 1, &argv[i], list->lastitem, indices, &numindices);
			if (numindices != 1)
				continue;
			ptr = OlListItemPointer(list->items[indices[0]]);
			if (ptr->label != NULL)
				XtFree(ptr->label);
			ptr->label = strdup(argv[i+1]);
			XtSetArg(arg[0], XtNapplTouchItem, &touchfn);
			XtGetValues(w->w, arg, 1);
			(*touchfn)(w->w, list->items[indices[0]]);
		}
		break;
	case 'P':	/* put user data at an index, takes pairs */
		for (i = 0; i+1 < argc; i += 2) {
			parse_indexlist(arg0, 1, &argv[i], list->lastitem, indices, &numindices);
			if (numindices != 1)
				continue;
			ptr = OlListItemPointer(list->items[indices[0]]);
			if (ptr->user_data != NULL)
				XtFree(ptr->user_data);
			ptr->user_data = strdup(argv[i+1]);
			XtSetArg(arg[0], XtNapplTouchItem, &touchfn);
			XtGetValues(w->w, arg, 1);
			(*touchfn)(w->w, list->items[indices[0]]);
		}
		break;
	case 'G':
		if (argc >= 1 && strcmp(argv[0], "all") == 0) {
			for (i = 0; i < list->lastitem; i++) {
				ptr = OlListItemPointer(list->items[i]);
				if (ptr->user_data != NULL)
					altprintf("%s\n", ptr->user_data);
			}
		} else {
			parse_indexlist(arg0, argc, argv, list->lastitem, indices, &numindices);
			for (i = 0; i < numindices; i++) {
				ptr = OlListItemPointer(list->items[indices[i]]);
				if (ptr->user_data != NULL)
					altprintf("%s\n", ptr->user_data);
			}
		}
		break;
	case 't':
		XtSetArg(arg[0], XtNapplTouchItem, &touchfn);
		XtGetValues(w->w, arg, 1);
		parse_indexlist(arg0, argc, argv, list->lastitem, indices, &numindices);
		for (i = 0; i < numindices; i++) {
			(*touchfn)(w->w, list->items[indices[i]]);
		}
		break;
	case 'e':
	case 'i':
		XtSetArg(arg[0], XtNapplEditOpen, &editfn);
		XtGetValues(w->w, arg, 1);
		parse_indexlist(arg0, argc, argv, list->lastitem, indices, &numindices);
		for (i = 0; i < numindices; i++) {
			(*editfn)(w->w, (mode[1]=='i'), list->items[indices[i]]);
		}
		break;
	case 'c':
		XtSetArg(arg[0], XtNapplEditClose, &editfn);
		XtGetValues(w->w, arg, 1);
		(*editfn)(w->w);
		break;
	case 'n':
	case 'u':
		XtSetArg(arg[0], XtNapplUpdateView, &updatefn);
		XtGetValues(w->w, arg, 1);
		(*updatefn)(w->w, (mode[1]=='u'));
		break;
	default:
		printerr(arg0, usagemsg(CONSTCHAR "usage: %s [-a | -g | -G | -r | -v | -t | -e | -i | -c | -n | -u ] Widgetid arg ...", arg0), NULL);
		return(1);
	}
	return(0);
}

do_OlRegisterHelp(argc, argv)
int argc;
char *argv[];
{
	wtab_t *w;

	if (argc < 4 || argc > 5) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s widgethandle helptag [string | -f filename]", argv[0]), NULL);
		return(1);
	}
	w = str_to_wtab(argv[0], argv[1]);
	if (w == NULL) {
		return(1);
	}
	if (strcmp(argv[3], "-f") == 0 && argc == 5) {
		OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)w->w, (String)strdup(argv[2]), OL_DISK_SOURCE, (XtPointer)strdup(argv[4]));
	} else if (argc == 4) {
		OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)w->w, (String)strdup(argv[2]), OL_STRING_SOURCE, (String)strdup(argv[3]));
	} else {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s widgethandle helptag [string | -f filename]\n", argv[0]), NULL);
		return(1);
	}
	return(0);
}

flatCheck(widget, callData)
Widget widget;
caddr_t callData;
{
	if (XtIsSubclass(widget, flatWidgetClass) == True)
	{ 
		OlFlatCallData *fcd;
		char tmpbuf[20];

		fcd = (OlFlatCallData *)callData;
		return fcd->item_index;
	}
	else
		return -1;
}

do_FocusOp(argc, argv)
int argc;
char *argv[];
{
	Widget wid;
	wtab_t *w1, *wtab;
	char *var;
	char *mode;
	wtab_t *w, *widget_to_wtab();
	char *arg0 = argv[0];

	if (argc < 3 || argv[1][0] != '-') {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s [-a | -c | -s | -g | -h | -m] handle arg ...", arg0), NULL);
		return(1);
	}
	mode = argv[1];
	w = str_to_wtab(argv[0], argv[2]);
	argv += 3;
	argc -= 3;
	if (w == NULL) {
		return(1);
	}
	switch (mode[1]) {
	case 'a':
		return(!OlCallAcceptFocus(w->w, 0));
	case 'c':
		return(!OlCanAcceptFocus(w->w, 0));
	case 's':
		OlSetInputFocus(w->w, 0, 0);
		return(0);
	case 'g':
		if (argc != 1) {
			printerr(arg0, usagemsg(CONSTCHAR "Usage: %s -g handle variable", arg0), NULL);
			return(1);
		}
		wid = OlGetCurrentFocusWidget(w->w);
		if ((w1 = widget_to_wtab(wid, NULL)) == NULL) {
			env_blank(argv[0]);
		} else {
			env_set_var(argv[0], w1->widid);
		}
		return(0);
	case 'h':
		return(!OlHasFocus(w->w));
	case 'm':
		{
			XrmValue f, t;

			var = NULL;
			switch (argc) {
			case 2:
				var = argv[1];
				break;
			case 1:
				break;
			default:
				printerr(arg0, usagemsg(CONSTCHAR "Usage: %s -m handle direction [variable]"), NULL);
				return(1);
			}
			f.addr = argv[0];
			f.size = strlen(argv[0]) + 1;
			t.addr = NULL;
			t.size = 0;
			XtConvert(w->w, str_XtRString, &f, str_XtROlVirtualName, &t);
			if (t.size && t.addr) {
				OlVirtualName direction = ((OlVirtualName *)(t.addr))[0];
				wid = OlMoveFocus(w->w, direction, 0);
				if (var) {
					if ((wtab = widget_to_wtab(wid, NULL)) != NULL) {
						env_set_var(var, wtab->widid);
					}
				}
				return(0);
			} else {
				printerr(arg0, CONSTCHAR "Unknown focus direction: ", argv[1]);
				return(1);
			}
		}
	default:
		printerr(arg0, usagemsg(CONSTCHAR "usage: %s [-a | -c | -s | -g | -h | -m] handle arg ...", arg0), NULL);
		return(1);
	}
}

static int
CursorOp_usage(arg0)
char *arg0;
{
	printerr(arg0, usagemsg(CONSTCHAR "usage: %s [] handle [arg ...]", arg0), NULL);
	return(1);
}

int
do_CursorOp(argc, argv)
int argc;
char *argv[];
{
	Widget wid;
	wtab_t *w1, *wtab;
	char *var;
	char *mode;
	wtab_t *w;
	char *arg0 = argv[0];
	Screen *screen;
	Display *display;
	Window window;
	Cursor cursor;

	if (argc < 2 || argc > 3 || argv[1][0] != '-') {
		printerr(arg0, usagemsg(CONSTCHAR "usage: %s [-a | -b | -c | -s | -g | -h | -m] handle arg ...", arg0), NULL);
		return(1);
	}
	mode = argv[1];
	if (argc == 3) {
		w = str_to_wtab(argv[0], argv[2]);
		argv += 3;
		argc -= 3;
		if (w == NULL) {
			return(1);
		}
		wid = w->w;
	} else {
		wid = Toplevel;
	}
	if (!XtIsRealized(wid)) {
		printerr(arg0, CONSTCHAR "Cannot do CursorOp on unrealized widget:", w->wname);
		return(1);
	}
	screen = XtScreen(wid);
	display = XtDisplay(wid);
	window = XtWindow(wid);

	switch (mode[1]) {
	case 'b':
		cursor = GetOlBusyCursor(screen);
		break;
	case 'd':
		cursor = GetOlDuplicateCursor(screen);
		break;
	case 'm':
		cursor = GetOlMoveCursor(screen);
		break;
	case 'p':
		cursor = GetOlPanCursor(screen);
		break;
	case 'q':
		cursor = GetOlQuestionCursor(screen);
		break;
	case 's':
		cursor = GetOlStandardCursor(screen);
		break;
	case 't':
		cursor = GetOlTargetCursor(screen);
		break;
	default:
		return(CursorOp_usage(arg0));
	}
	XDefineCursor(display, window, cursor);
	XFlush(display);
	return(0);
}

#define RECURSE_NO_CONTINUE	(-1)

static int
recurse_widgets(wid, func, arg1, arg2)
Widget wid;
int (*func)();
XtArgVal arg1, arg2;
{
	register int i;
	Arg args[2];
	WidgetList children;
	Cardinal numchildren = 0;
	String widname;

	widname = XtName(wid);
	if (strcmp(widname, CONSTCHAR "nodata") == 0)
		return(0);
	if (func(wid, arg1, arg2) == RECURSE_NO_CONTINUE) {
		return(0);
	}
	if (XtIsSubclass(wid, managerWidgetClass) || XtIsShell(wid)) {
		/* These manager class widgets are
		 * traversed recursively.
		 */
		XtSetArg(args[0], XtNchildren, &children);
		XtSetArg(args[1], XtNnumChildren, &numchildren);
		XtGetValues(wid, args, 2);
		for (i = 0; i < numchildren; i++) {
			recurse_widgets(children[i], func, arg1, arg2);
		}
	}
	return(0);
}

/*
 * return 0 if you find a changebar on, else 1
 */

static int
test_changebars(wid)
Widget wid;
{
	register int i, n;
	Arg args[3], changeargs[1];
	WidgetList children;
	Cardinal numchildren = 0;
	String widname, str;
	OlDefine changebar;
	Boolean allow;
	WidgetClass class;
	OlDefine barvalue;

	widname = XtName(wid);
	if (strcmp(widname, CONSTCHAR "nodata") == 0)
		return(1);
	class = XtClass(wid);

	if (XtIsSubclass(wid, managerWidgetClass) || XtIsShell(wid)) {
		/* These manager class widgets are
		 * traversed recursively.
		 */
		XtSetArg(args[0], XtNchildren, &children);
		XtSetArg(args[1], XtNnumChildren, &numchildren);
		n = 2;
		if (class == controlAreaWidgetClass) {
			XtSetArg(args[2], XtNallowChangeBars, &allow);
			n = 3;
		} else
			allow = FALSE;
		XtGetValues(wid, args, n);
		XtSetArg(changeargs[0], XtNchangeBar, (XtArgVal) &barvalue);
		for (i = 0; i < numchildren; i++) {
			if (allow) {
				XtGetValues(children[i], changeargs, 1);
				if (barvalue == OL_NORMAL)
					return(0);
			}
			if (test_changebars(children[i]) == 0)
				return(0);
		}
	}
	return(1);
}

static int
remove_changebars(wid)
Widget wid;
{
	register int i, n;
	Arg args[3], changeargs[1];
	WidgetList children;
	Cardinal numchildren = 0;
	String widname, str;
	OlDefine changebar;
	Boolean allow;
	WidgetClass class;

	widname = XtName(wid);
	/*
	 * Due to a bug in GS4 open look, a core dump occurs if
	 * you try to clear a changebar on a widget that isn't realized.
	 * So, we just return in that case.
	 */
	if (strcmp(widname, CONSTCHAR "nodata") == 0 || !XtIsRealized(wid))
		return(0);
	class = XtClass(wid);

	if (XtIsSubclass(wid, managerWidgetClass) || XtIsShell(wid)) {
		/* These manager class widgets are
		 * traversed recursively.
		 */
		XtSetArg(args[0], XtNchildren, &children);
		XtSetArg(args[1], XtNnumChildren, &numchildren);
		n = 2;
		if (class == controlAreaWidgetClass) {
			XtSetArg(args[2], XtNallowChangeBars, &allow);
			n = 3;
		} else
			allow = FALSE;
		XtGetValues(wid, args, n);
		for (i = 0; i < numchildren; i++) {
			if (allow) {
				XtSetArg(changeargs[0], XtNchangeBar, (XtArgVal) OL_ALWAYS);
				XtSetValues(children[i], changeargs, 1);
			}
			remove_changebars(children[i]);
		}
	}
	return(0);
}

static void
ChangeBarCB(w, clientData, callData)
Widget w;
Widget clientData;
caddr_t callData;	/* not used */
{
	Arg args[1];

	XtSetArg(args[0], XtNchangeBar, (XtArgVal) OL_NORMAL);
	XtSetValues(clientData, args, 1);
}

static int
setup_mnemonics(wid, tab, flag)
Widget wid;
char *tab;
Boolean flag;	/* if TRUE, then set mnemonics, else just find existing */
{
	register int i, n;
	char *stackalloc();
	String label;
	Arg args[2];
	WidgetList children;
	Cardinal numchildren = 0;
	WidgetClass class;
	String resource;
	char *scoretab, *s;
	int bestscore;
	char bestchar;
	Boolean isflat = FALSE;
	OlDefine button;
	int numitems;
	int item;

	class = XtClass(wid);

	if (XtIsSubclass(wid, managerWidgetClass) || XtIsShell(wid)) {
		/* These manager class widgets are
		 * traversed recursively.
		 */
		XtSetArg(args[0], XtNchildren, &children);
		XtSetArg(args[1], XtNnumChildren, &numchildren);
		XtGetValues(wid, args, 2);
		for (i = 0; i < numchildren; i++) {
			setup_mnemonics(children[i], tab, flag);
		}
	}
	resource = NULL;
	numitems = 1;
	isflat = FALSE;
	if (class == oblongButtonWidgetClass || 
	    class == oblongButtonGadgetClass || 
	    class == menuButtonWidgetClass || 
	    class == menuButtonGadgetClass || 
	    class == checkBoxWidgetClass || 
	    class == captionWidgetClass ||
	    class == rectButtonWidgetClass) {
		resource = XtNlabel;
	} else if (
		class == flatExclusivesWidgetClass
		|| class == flatNonexclusivesWidgetClass
#ifdef MOOLIT
		|| class == flatButtonsWidgetClass
#endif
		) {
		resource = XtNlabel;
		isflat = TRUE;
		XtSetArg(args[0], XtNnumItems, (XtArgVal)&numitems);
		XtGetValues(wid, args, 1);
	}
	for (item = 0; item < numitems; item++) {
		if (flag && resource != NULL) {
			Arg args[2];
			char m;
			char *p;

			label = NULL;
			XtSetArg(args[0], resource, (XtArgVal)&label);
			XtSetArg(args[1], XtNmnemonic, (XtArgVal)&m);
			if (isflat)
				OlFlatGetValues(wid, item, args, 2);
			else
				XtGetValues(wid, args, 2);
			if (m != '\0' || label == NULL)	/* already has one */
				continue;
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
			scoretab = stakalloc(n = strlen(label));
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

			XtSetArg(args[0], XtNmnemonic, (XtArgVal)bestchar);
			if (isflat)
				OlFlatSetValues(wid, item, args, 1);
			else
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
			char m;

			XtSetArg(args[0], XtNmnemonic, (XtArgVal)&m);
			if (isflat)
				OlFlatGetValues(wid, item, args, 1);
			else
				XtGetValues(wid, args, 1);
			if (m != '\0')
				tab[toupper(m)] = tab[tolower(m)] = 1;
		}
	}
	return(0);
}

static int
setup_changebars(wid, cbparent)
Widget wid, cbparent;
{
	register int i;
	Boolean allow;
	WidgetClass class;
	Widget cbchild, parent;
	Arg args[1];

	class = XtClass(wid);
	parent = XtParent(wid);

	if (cbparent != NULL)
		cbchild = cbparent;
	else
		cbchild = wid;

	allow = FALSE;
	if (parent && XtClass(parent) == controlAreaWidgetClass) {
		XtSetArg(args[0], XtNallowChangeBars, &allow);
		XtGetValues(parent, args, 1);
	}

	if (allow || cbparent != NULL) {
		/*
		 * If the class is caption, set things up
		 * so the child can modify the caption's
		 * change bar.
		 */
		if (class == captionWidgetClass) {
			Arg capargs[2];
			WidgetList capchildren;
			Cardinal capnumchildren = 0;

			XtSetArg(capargs[0], XtNchildren, &capchildren);
			XtSetArg(capargs[1], XtNnumChildren, &capnumchildren);
			XtGetValues(wid, capargs, 2);
			if (capnumchildren == 1) {
				recurse_widgets(capchildren[0], setup_changebars, wid, NULL);
				return(RECURSE_NO_CONTINUE);
			}
		} else if (class == textFieldWidgetClass) {
			Widget te;
			Arg teargs[1];

			XtSetArg(teargs[0], XtNtextEditWidget, &te);
			XtGetValues(wid, teargs, 1);
			XtAddCallback(te, XtNpostModifyNotification, (XtCallbackProc)ChangeBarCB, cbchild);
		} else if (class == textEditWidgetClass) {
			XtAddCallback(wid, XtNpostModifyNotification, (XtCallbackProc)ChangeBarCB, cbchild);
		} else if (class == checkBoxWidgetClass) {
			XtAddCallback(wid, XtNselect, (XtCallbackProc)ChangeBarCB, cbchild);
			XtAddCallback(wid, XtNunselect, (XtCallbackProc)ChangeBarCB, cbchild);
		} else if (class == sliderWidgetClass) {
			XtAddCallback(wid, XtNsliderMoved, (XtCallbackProc)ChangeBarCB, cbchild);
		}
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

	if (class != textFieldWidgetClass && (XtIsSubclass(wid, managerWidgetClass) || XtIsShell(wid))) {
		/* These manager class widgets are
		 * traversed recursively.
		 */
		XtSetArg(args[0], XtNchildren, &children);
		XtSetArg(args[1], XtNnumChildren, &numchildren);
		XtGetValues(wid, args, 2);
		for (i = 0; i < numchildren; i++) {
			print_widgetdata(children[i], shortflag);
		}
	} 
	if (class == textFieldWidgetClass) {
		XtSetArg(args[0], XtNstring, &str);
		XtGetValues(wid, args, 1);
		if (shortflag)
			altprintf("%s|", str);
		else
			altprintf("%s=%s\n", widname, str);
	} else if (class == textEditWidgetClass) {
		char *buf = NULL, *cvt;
		char *p, *q;

		OlTextEditCopyBuffer(wid, &buf);
		if (buf != NULL) {
			cvt = stakalloc(strlen(buf)*2 + 1);
			p = cvt;
			q = buf;
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
			XFree(buf);
		} else
			cvt = "";
		if (shortflag)
			altprintf("%s|", cvt);
		else
			altprintf("%s=%s\n", widname, cvt);
	} else if (class == checkBoxWidgetClass) {
		XtSetArg(args[0], XtNset, &bool);
		XtGetValues(wid, args, 1);
		if (shortflag)
			altprintf("%s|", bool ? CONSTCHAR "s" : CONSTCHAR "u");
		else
			altprintf("%s=%s\n", widname, bool ? CONSTCHAR "set" : CONSTCHAR "unset");
	} else if (class == sliderWidgetClass) {
		XtSetArg(args[0], XtNsliderValue, &val);
		XtGetValues(wid, args, 1);
		if (shortflag)
			altprintf("%d|", val);
		else
			altprintf("%s=%d\n", widname, val);
#ifdef MOOLIT
	} else if (class == integerFieldWidgetClass) {
		XtSetArg(args[0], XtNvalue, &val);
		XtGetValues(wid, args, 1);
		if (shortflag)
			altprintf("%d|", val);
		else
			altprintf("%s=%d\n", widname, val);
	} else if (class == stepFieldWidgetClass) {
		XtSetArg(args[0], XtNstring, &str);
		XtGetValues(wid, args, 1);
		if (shortflag)
			altprintf("%s|", str);
		else
			altprintf("%s=%s\n", widname, str);
#endif
	} else if (class == flatExclusivesWidgetClass ||
			class == flatNonexclusivesWidgetClass ||
			class == flatCheckBoxWidgetClass) {
		char *p;
		int n;
		Boolean set;

		XtSetArg(args[0], XtNnumItems, &n);
		XtGetValues(wid, args, 1);
		if (!shortflag)
			altprintf("%s=", widname, val);
		else	
			altprintf("|");

		for (i = 0; i < n; i++) {
			XtSetArg(args[0], XtNset, &set);
			OlFlatGetValues(wid, i, args, 1);
			altprintf("%s%s", i ? "," : "", set ? "s" : "u");
		}
		if (!shortflag)
			altprintf("\n");
#ifdef MOOLIT
	} else if (class == flatButtonsWidgetClass) {
		OlDefine button;
		char *p;
		int n;
		Boolean set;
		/*
		 * There is "data" here only if the buttons are in
		 * rectbutton mode.  We output:
		 * name=s,s,u,...etc. s for "set", u for "unset"
		 */
		XtSetArg(args[0], XtNbuttonType, &button);
		XtSetArg(args[1], XtNnumItems, &n);
		XtGetValues(wid, args, 2);
		if (button != OL_RECT_BTN)
			return(0);
		if (!shortflag)
			altprintf("%s=", widname, val);
		else	
			altprintf("|");

		for (i = 0; i < n; i++) {
			XtSetArg(args[0], XtNset, &set);
			OlFlatGetValues(wid, i, args, 1);
			altprintf("%s%s", i ? "," : "", set ? "s" : "u");
		}
		if (!shortflag)
			altprintf("\n");
#endif
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

	widname = XtName(wid);
	if (strcmp(widname, CONSTCHAR "nodata") == 0)
		return(0);

	class = XtClass(wid);

	if (class != textFieldWidgetClass && (XtIsSubclass(wid, managerWidgetClass) || XtIsShell(wid))) {
		/* These manager class widgets are
		 * traversed recursively.
		 */
		XtSetArg(args[0], XtNchildren, &children);
		XtSetArg(args[1], XtNnumChildren, &numchildren);
		XtGetValues(wid, args, 2);
		for (i = 0; i < numchildren; i++) {
			if (parse_widgetdata(children[i], data, shortflag) != 0) {
				return(1);
			}
		}
	} 
	if (class == textEditWidgetClass) {
		int len;
		char *cvt;

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
		/*
		 * There is a bug in some versions of OPEN LOOK
		 * that cause a dump if you don't have a newline
		 * on the end of the string.  Add one if one is
		 * not already there.
		 */
		len = strlen(cvt)-1;
		if (cvt[len] != '\n') {
			cvt[len+1] = '\n';
			cvt[len+2] = '\0';
		}
		XtSetArg(args[0], XtNsource, cvt);
		XtSetValues(wid, args, 1);
	} else if (class == textFieldWidgetClass) {
		if (next_line(data, &name, &str, shortflag) != 0) {
			return(1);
		}
		if (!shortflag && strcmp(name, widname) != 0) {
			return(1);
		}
		XtSetArg(args[0], XtNstring, str);
		XtSetValues(wid, args, 1);
	} else if (class == checkBoxWidgetClass) {
		if (next_line(data, &name, &str, shortflag) != 0) {
			return(1);
		}
		if (!shortflag && strcmp(name, widname) != 0) {
			return(1);
		}
		if (strncmp(str, CONSTCHAR "s", 1) == 0)
			bool = TRUE;
		else if (strncmp(str, CONSTCHAR "u", 1) == 0)
			bool = FALSE;
		else {
			printerr(CONSTCHAR "parsedata", CONSTCHAR "bad checkbox value:", str);
			bool = FALSE;
		}
		XtSetArg(args[0], XtNset, (XtArgVal)bool);
		XtSetValues(wid, args, 1);
	} else if (class == sliderWidgetClass) {
		if (next_line(data, &name, &str, shortflag) != 0) {
			return(1);
		}
		if (!shortflag && strcmp(name, widname) != 0) {
			return(1);
		}
		val = atoi(str);
		XtSetArg(args[0], XtNsliderValue, (XtArgVal) val);
		XtSetValues(wid, args, 1);
#ifdef MOOLIT
	} else if (class == integerFieldWidgetClass) {
		if (next_line(data, &name, &str, shortflag) != 0) {
			return(1);
		}
		if (!shortflag && strcmp(name, widname) != 0) {
			return(1);
		}
		val = atoi(str);
		XtSetArg(args[0], XtNvalue, (XtArgVal) val);
		XtSetValues(wid, args, 1);
	} else if (class == stepFieldWidgetClass) {
		if (next_line(data, &name, &str, shortflag) != 0) {
			return(1);
		}
		if (!shortflag && strcmp(name, widname) != 0) {
			return(1);
		}
		XtSetArg(args[0], XtNstring, (XtArgVal) str);
		XtSetValues(wid, args, 1);
#endif
	} else if (class == flatExclusivesWidgetClass || class == flatNonexclusivesWidgetClass || class == flatCheckBoxWidgetClass) {
		char *p;
		if (next_line(data, &name, &str, shortflag) != 0) {
			return(1);
		}
		if (!shortflag && strcmp(name, widname) != 0) {
			return(1);
		}
		for (i = 0, p = strtok(str, ","); p && *p; i++,p = strtok(NULL, ",")) {
			XtSetArg(args[0], XtNset, (Boolean)(*p == 's'));
			OlFlatSetValues(wid, i, args, 1);
		}
#ifdef MOOLIT
	} else if (class == flatButtonsWidgetClass) {
		OlDefine button;
		char *p;
		/*
		 * There is "data" here only if the buttons are in
		 * rectbutton mode.  We output:
		 * name=set,set,unset,...etc.
		 */
		XtSetArg(args[0], XtNbuttonType, &button);
		XtGetValues(wid, args, 1);
		if (button != OL_RECT_BTN)
			return(0);
		if (next_line(data, &name, &str, shortflag) != 0) {
			return(1);
		}
		if (!shortflag && strcmp(name, widname) != 0) {
			return(1);
		}
		for (i = 0,p = strtok(str, ","); p && *p; i++,p = strtok(NULL, ",")) {
			XtSetArg(args[0], XtNset, (Boolean)(*p == 's'));
			OlFlatSetValues(wid, i, args, 1);
		}
#endif
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

static int
ChangeBarOp_usage(arg0)
char *arg0;
{
	printerr(arg0, usagemsg(CONSTCHAR "usage: %s [ -s | -c ] widget ...", arg0), NULL);
	return(1);
}

int
do_ChangeBarOp(argc, argv)
int argc;
char *argv[];
{
	register int i;
	wtab_t *w;
	char *datum;
	char *arg0 = argv[0];
	char mode;

	if (argc < 3 || argv[1][0] != '-')
		return(ChangeBarOp_usage(arg0));

	mode = argv[1][1];
	argv += 2;
	argc -= 2;

	switch (mode) {
	case 's':		/* set up callbacks for changebars */
		for (i = 0; i < argc; i++) {
			w = str_to_wtab(arg0, argv[i]);
			if (w == NULL)
				continue;
			recurse_widgets(w->w, setup_changebars, NULL, NULL);
		}
		break;
	case 'c':		/* turn off all change bars */
		for (i = 0; i < argc; i++) {
			w = str_to_wtab(arg0, argv[i]);
			if (w == NULL || ! XtIsManaged(w->w))
				continue;
			remove_changebars(w->w);
		}
		break;
	case 't':
		for (i = 0; i < argc; i++) {
			w = str_to_wtab(arg0, argv[i]);
			if (w == NULL || ! XtIsManaged(w->w))
				continue;
			if (test_changebars(w->w) == 0)
				return(0);
		}
		return(1);
	default:
		return(ChangeBarOp_usage(arg0));
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

static void
texteditops_reset(wid, ops)
Widget wid;
textfield_op_t *ops;
{
	Arg args[4];
	Pixel background, inputfoc;
	Widget textedit;
	TextPosition size;

	XtSetArg(args[0], XtNbackground, &background);
	XtSetArg(args[1], XtNinputFocusColor, &inputfoc);
	XtSetArg(args[2], XtNtextEditWidget, &textedit);
	XtSetArg(args[3], XtNmaximumSize, &size);
	XtGetValues(wid, args, 4);
	memset(ops, '\0', sizeof(textfield_op_t));
	ops->autoblank = ops->autotraversal = ops->autoreturn = FALSE;
	ops->maxcursor = size;
	ops->focuscolor = ops->nofocuscolor = background;
	ops->cursorcolor = ops->nocursorcolor = inputfoc;
	ops->textfield = wid;
	ops->textedit = textedit;
	ops->charset = NULL;
	ops->cvtcase = '\0';
	ops->active = FALSE;
}

static void
putstring(wid, s)
TextFieldWidget wid;
String s;
{
	Arg args[1];

	XtSetArg(args[0], XtNstring, (XtArgVal)s);
	XtSetValues((Widget)wid, args, 1);
}

static String
getstring(wid)
TextFieldWidget wid;
{
	Arg args[1];
	static String ret;

	XtSetArg(args[0], XtNstring, &ret);
	XtGetValues((Widget)wid, args, 1);
	return(ret);
}

static void
textfieldcaseconvert(wid, ops)
Widget wid;
textfield_op_t *ops;
{
	String p, s, orig;
	Boolean lastalpha;

	if (ops->cvtcase == '\0' && ops->clipwhite == FALSE)
		return;
	orig = s = strdup(getstring(wid));
	if (ops->clipwhite) {
		while (isspace(*s))
			s++;
		for (p = &s[strlen(s)-1]; p >= s && isspace(*p); p--)
			*p = '\0';
	}
	switch (ops->cvtcase) {
	case 'u':
		for (p = s; *p; p++) {
			if (islower(*p))
				*p = toupper(*p);
		}
		break;
	case 'l':
		for (p = s; *p; p++) {
			if (isupper(*p))
				*p = tolower(*p);
		}
		break;
	case 'i':
		lastalpha = FALSE;
		for (p = s; *p; p++) {
			if (!lastalpha && islower(*p))
				*p = toupper(*p);
			lastalpha = isalpha(*p);
		}
		break;
	}
	putstring(wid, s);
	XtFree(orig);
}

static void
textfieldverificationproc(wid, ops, ver)
TextFieldWidget wid;
textfield_op_t *ops;
OlTextFieldVerifyPointer ver;
{
	if (ops == NULL || ops->active == FALSE)
		return;
	textfieldcaseconvert(wid, ops);
	if (ops->autoreturn && ver && ver->reason == OlTextFieldReturn) {
		OlMoveFocus(ops->textfield, OL_NEXTFIELD, 0);
		OlTextEditSetCursorPosition(ops->textedit, 0, 0, 0);
	}
}

static void
textfieldmotionproc(wid, ops, mods)
Widget wid;
textfield_op_t *ops;
OlTextMotionCallDataPointer mods;
{
	Arg args[3];

	if (ops == NULL || ops->active == FALSE)
		return;
	if (ops->autotraversal && ops->maxcursor > 0 && mods != NULL && mods->new_cursor >= ops->maxcursor) {
		OlCallCallbacks(ops->textfield, XtNverification, (XtPointer)ops);
		OlMoveFocus(ops->textfield, OL_NEXTFIELD, 0);
		OlTextEditSetCursorPosition(ops->textedit, 0, 0, 0);
		mods->ok = FALSE;
	}
}

static void
textfieldmodifyproc(wid, ops, mods)
Widget wid;
textfield_op_t *ops;
OlTextModifyCallDataPointer mods;
{
	if (ops->active == FALSE)
		return;
	if (ops->autoblank == TRUE) {
		if (mods->current_cursor == 0 && mods->text_length == 1 &&
		    mods->new_cursor == 1) {
			if (OlTextEditSetCursorPosition(wid, 0, 0, 0))
				OlTextEditClearBuffer(wid);
		}
	}
	if (ops->charset != NULL && mods->text_length == 1) {
		char txt[2];

		txt[0] = mods->text[0];
		txt[1] = '\0';
		if (strmatch(txt, ops->charset) == NULL) {
			mods->ok = FALSE;
			XBell(XtDisplay(Toplevel), 0);
			return;
		}
	}
}

static void
settextfieldfocus(wid, ops, type)
TextFieldWidget wid;
textfield_op_t *ops;
int type;
{
	Arg args[1];

	if (type == FocusIn) {
		XtSetArg(args[0], XtNbackground, ops->focuscolor);
		XtSetValues(ops->textfield, args, 1);
		XtSetArg(args[0], XtNinputFocusColor, ops->cursorcolor);
		XtSetValues(ops->textedit, args, 1);
	} else if (type == FocusOut) {
		XtSetArg(args[0], XtNbackground, ops->nofocuscolor);
		XtSetValues(ops->textfield, args, 1);
		XtSetArg(args[0], XtNinputFocusColor, ops->nocursorcolor);
		XtSetValues(ops->textedit, args, 1);
		textfieldcaseconvert(ops->textfield, ops);
	}
	return;
}

static void
textfieldfocusproc(wid, ops, event, dispatch)
Widget wid;
textfield_op_t *ops;
XEvent *event;
Boolean *dispatch;
{
	if (ops->active == FALSE)
		return;
	settextfieldfocus(ops->textfield, ops, event->type);
	return;
}

static int
cvtcolor(name, pix)
char *name;
Pixel *pix;
{
	XrmValue fval, tval;

	fval.addr = name;
	fval.size = strlen(name);
	XtConvert(Toplevel, str_XtRString, &fval, str_XtRPixel, &tval);

	if (tval.size != 0) {
		*pix = ((Pixel *)(tval.addr))[0];
		return(SUCCESS);
	} else
		return(FAIL);
}

int
TextFieldOp_usage(arg0)
char *arg0;
{
	printerr(arg0, usagemsg(CONSTCHAR "usage: %s [ -r | -f color | -n color | -c | -b | -t | -v pattern | -w | -u | -i | -l ] textfieldwidget", arg0), NULL);
	return(1);
}

/*
 * Options:
 *
 * -r		Automatically go to next field on RETURN
 * -f color	Set a focus background color
 * -n color	Set a nofocus background color
 * -c 		Remove the cursor when no focus by setting it to the -n color
 * -t		Automatically traverse to the next field when the 
 *		limit is reached
 * -b		Automatically blank out the field when user types in field 1.
 * -v pattern	Validate individual characters according to pattern as they
 *		are typed in.  For example, "[0-9]" as a pattern would allow
 *		only numeric entries.
 * -w		Clip leading and trailing whitespace on verify.
 * -u		Convert alpha characters to upper case on verify.
 * -l		Convert alpha characters to lower case on verify.
 * -i		Convert alpha characters to initial caps on verify.
 *
 *  Other options are widget ids.
 */

int
do_TextFieldOp(argc, argv)
int argc;
char *argv[];
{
	register int i = 0;
	textfield_op_t ops, *wops;
	char *arg0 = argv[0];
	wtab_t *w;
	Pixel pixel;
	Boolean showsettings = FALSE;
	int changes;
#define CHG_FOC		1
#define CHG_NOFOC	2
#define CHG_CUR		4
#define CHG_TRAV	8
#define CHG_BLANK	16
#define CHG_RET		32
#define CHG_CHARSET	64
#define CHG_CASE	128
#define CHG_CLIP	256

	if (argc < 3)
		return(TextFieldOp_usage(arg0));
	changes = 0;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			w = str_to_wtab(arg0, argv[i]);
			if (w == NULL || XtClass(w->w) != textFieldWidgetClass)
				return(TextFieldOp_usage(arg0));
			if (w->info == NULL) {
				w->info = (caddr_t) XtMalloc(sizeof(textfield_op_t));
				wops = (textfield_op_t *) w->info;
				texteditops_reset(w->w, w->info);
				XtAddEventHandler(wops->textedit, FocusChangeMask, FALSE, (XtEventHandler)textfieldfocusproc, (caddr_t)w->info);
				XtAddCallback(wops->textedit, XtNmodifyVerification, (XtCallbackProc)textfieldmodifyproc, w->info);
				XtAddCallback(wops->textedit, XtNmotionVerification, (XtCallbackProc)textfieldmotionproc, w->info);
				XtAddCallback(wops->textfield, XtNverification, (XtCallbackProc)textfieldverificationproc, w->info);
			}
			wops = (textfield_op_t *) w->info;
			if (changes & CHG_FOC) {
				wops->focuscolor = ops.focuscolor;
				wops->active = TRUE;
			}
			if (changes & CHG_NOFOC) {
				wops->nofocuscolor = ops.nofocuscolor;
				wops->active = TRUE;
			}
			if (changes & CHG_CUR) {
				wops->nocursorcolor = wops->nofocuscolor;
				wops->active = TRUE;
			}
			if (changes & CHG_BLANK) {
				wops->autoblank = ops.autoblank;
				wops->active = TRUE;
			}
			if (changes & CHG_BLANK) {
				wops->clipwhite = ops.clipwhite;
				wops->active = TRUE;
			}
			if (changes & CHG_TRAV) {
				Arg args[1];

				XtSetArg(args[0], XtNmaximumSize, &wops->maxcursor);
				XtGetValues(wops->textfield, args, 1);

				wops->autotraversal = ops.autotraversal;
				wops->active = TRUE;
			}
			if (changes & CHG_RET) {
				wops->autoreturn = ops.autoreturn;
				wops->active = TRUE;
			}
			if (changes & CHG_CHARSET) {
				wops->charset = ops.charset;
				wops->active = TRUE;
			}
			if (changes & CHG_CASE) {
				wops->cvtcase = ops.cvtcase;
				wops->active = TRUE;
			}
			settextfieldfocus(w->w, wops, OlHasFocus(w->w) ? FocusIn : FocusOut);
			if (showsettings) {
				altprintf("TextField Settings for %s:\n", XtName(w->w));
				altprintf("traversal: %d; blank: %d; active: %d; return: %d\n", wops->autotraversal, wops->autoblank, wops->active, wops->autoreturn);
				altprintf("maxcursor: %d; cvtcase: '%c'; charset: 0x%x\n", wops->maxcursor, wops->cvtcase, wops->charset);
			}
			continue;
		}
		switch (argv[i][1]) {
		case 'f':
			if (++i >= argc)
				return(TextFieldOp_usage(arg0));
			if (cvtcolor(argv[i], &pixel) == FAIL) 
				return(TextFieldOp_usage(arg0));
			ops.focuscolor = pixel;
			changes |= CHG_FOC;
			break;
		case 'n':
			if (++i >= argc)
				return(TextFieldOp_usage(arg0));
			if (cvtcolor(argv[i], &pixel) == FAIL) 
				return(TextFieldOp_usage(arg0));
			ops.nofocuscolor = pixel;
			changes |= CHG_NOFOC;
			break;
		case 'c':
			changes |= CHG_CUR;
			break;
		case 'w':
			changes |= CHG_CLIP;
			ops.clipwhite = TRUE;
			break;
		case 'r':
			changes |= CHG_RET;
			ops.autoreturn = TRUE;
			break;
		case 'v':
			if (++i >= argc)
				return(TextFieldOp_usage(arg0));
			ops.charset = strdup(argv[i]);
			changes |= CHG_CHARSET;
			break;
		case 't':
			ops.autotraversal = TRUE;
			changes |= CHG_TRAV;
			break;
		case 'b':
			ops.autoblank = TRUE;
			changes |= CHG_BLANK;
			break;
		case 'u':
		case 'l':
		case 'i':
			ops.cvtcase = argv[i][1];
			changes |= CHG_CASE;
			break;
		case 's':
			showsettings++;
			break;
		default:
			return(TextFieldOp_usage(arg0));
		}
	}
	return(0);
}

static void
stdConsumeCB(wid, event, ve)
Widget wid;
int event;
OlVirtualEvent ve;
{
	if (event != ve->virtual_name)
		return;
	switch (event) {
	case OL_DEFAULTACTION:
		OlMoveFocus(wid, OL_NEXTFIELD, 0);
		break;
	}
}

#ifdef MOOLIT
static int
OlPostPopupMenu_usage(arg0)
char *arg0;
{
	printerrf(arg0, "usage: OlPostPopupMenu [-k] menu_owner popup_menu rootx rooty initx inity");
	return(1);
}

int
do_OlPostPopupMenu(argc, argv)
int argc;
char *argv[];
{
	wtab_t *owner, *popup;
	register int i;
	OlDefine activation = OL_MENU;
	char *arg0 = argv[0];
	Position rootx, rooty, initx, inity;

	if (argc < 7) {
		return(OlPostPopupMenu_usage(arg0));
	}
	if (strcmp(argv[1], "-k") == 0) {
		activation = OL_MENUKEY;
		argv++;
		argc--;
		if (argc < 7) {
			return(OlPostPopupMenu_usage(arg0));
		}
	}
	owner = str_to_wtab(arg0, argv[1]);
	if (owner == NULL) {
		return(OlPostPopupMenu_usage(arg0));
	}
	popup = str_to_wtab(arg0, argv[2]);
	if (popup == NULL) {
		return(OlPostPopupMenu_usage(arg0));
	}
	for (i = 3; i < 7; i++) {
		if (strspn(argv[i], "01234567989") != strlen(argv[i])) {
			return(OlPostPopupMenu_usage(arg0));
		}
	}
	rootx = atoi(argv[3]);
	rooty = atoi(argv[4]);
	initx = atoi(argv[5]);
	inity = atoi(argv[6]);

	OlPostPopupMenu(owner->w, popup->w, activation, NULL, rootx, rooty, initx, inity);
	return(0);
}
#endif

int
do_EventOp(argc, argv)
int argc;
char *argv[];
{
	register int i;
	char *arg0 = argv[0];
	int event = 0;
	wtab_t *w;

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'd':
				event = OL_DEFAULTACTION;
				break;
			case 'g':
				event = -1;
				break;
			}
		} else {
			w = str_to_wtab(arg0, argv[i]);
			if (w == NULL) {
				printerr(arg0, usagemsg(CONSTCHAR "usage: %s [ -r handle", arg0), NULL);
				return(1);
			}
			if (event == -1) {
				Arg args[1];

				XtSetArg(args[0], XtNgeometry, (XtArgVal)"+0-0");
				XtSetValues(w->w, args, 1);
				return(0);
			}
			XtAddCallback(w->w, CONSTCHAR XtNconsumeEvent, (XtCallbackProc)stdConsumeCB, (XtPointer)event);
		}
	}
	return(0);
}

/*
 * stdFlatCB() is the central routine from which all flat callback
 * functions are dispatched.  The variable "CB_WIDGET" will be placed 
 * in the environment to represent the CallBackWidget handle.  Because
 * callbacks from flats don't work the same as usual callbacks (because
 * they are type XtCallbackProc instead of XtCallbackList) we had to
 * fudge this to work by storing the command string inside the widget's
 * wtab_t->info field.  This is pretty nasty and non-general, but there
 * does not appear to be a good solution.
 */

void
stdFlatCB(proctype, widget, clientData, callData)
int proctype;	/* proc type, FLAT_SELECT, FLAT_UNSELECT, FLAT_DBLSELECT, FLAT_DROP */
void  *widget;
caddr_t clientData;
OlFlatCallData *callData;
{
	wtab_t *widget_to_wtab();
	wtab_t *w;
	char envbuf[64];
	FlatInfo_t *finfo;

	w = widget_to_wtab(widget, NULL);
	finfo = (FlatInfo_t *) w->info;
	if (finfo == NULL)
		return;

	env_set_var(CONSTCHAR "CB_WIDGET", w->widid);
	sprintf(envbuf, "CB_INDEX=%d", callData->item_index);
	env_set(envbuf);

	switch (proctype) {
	case FLAT_SELECT:
		ksh_eval(finfo->selectProcCommand);
		break;
	case FLAT_UNSELECT:
		ksh_eval(finfo->unselectProcCommand);
		break;
#ifdef MOOLIT
	case FLAT_DBLSELECT:
		ksh_eval(finfo->dblSelectProcCommand);
		break;
	case FLAT_DROP:
		ksh_eval(finfo->dropProcCommand);
		break;
#endif
	}
	return;
}

void
stdFlatSelectProc(widget, clientData, callData)
void  *widget;
caddr_t clientData;
OlFlatCallData *callData;
{
	stdFlatCB(FLAT_SELECT, widget, clientData, callData);
	return;
}

void
stdFlatUnselectProc(widget, clientData, callData)
void  *widget;
caddr_t clientData;
OlFlatCallData *callData;
{
	stdFlatCB(FLAT_UNSELECT, widget, clientData, callData);
	return;
}

void
stdFlatDblSelectProc(widget, clientData, callData)
void  *widget;
caddr_t clientData;
OlFlatCallData *callData;
{
	stdFlatCB(FLAT_DBLSELECT, widget, clientData, callData);
	return;
}

void
stdFlatDropProc(widget, clientData, callData)
void  *widget;
caddr_t clientData;
OlFlatCallData *callData;
{
	stdFlatCB(FLAT_DROP, widget, clientData, callData);
	return;
}

int
do_OlFlatSetValues(argc, argv)
int argc;
char *argv[];
{
	int n, i, ind;
	char *arg0 = argv[0];
	Arg args[MAXARGS];
	wtab_t *w;

	if (argc < 4) {
		printerr(arg0, usagemsg(CONSTCHAR "usage: %s Widgetid index arg:val ...\n", arg0), NULL);
		return(1);
	}
	w = str_to_wtab(argv[0], argv[1]);
	ind = atoi(argv[2]);
	if (ind < 0) {
		printerr(arg0, CONSTCHAR "Negative flat widget index not allowed", NULL);
		return(1);
	}
	argv += 3;
	argc -= 3;
	if (w == NULL) {
		return(1);
	} else {
		n = 0;
		parse_args(arg0, argc, argv, w, w->parent, w->wclass, &n, args);
		if (n > 0) {
			OlFlatSetValues(w->w, ind, args, n);
			for (i = 0; i < n; i++)
				XtFree(args[i].name);
		} else {
			printerr(arg0, CONSTCHAR "Nothing to set", NULL);
			return(1);
		}
	}
	return(0);
}

int
do_OlFlatGetValues(argc, argv)
int argc;
char *argv[];
{
	register int i, j;
	int n, ind;
	char *arg0 = argv[0];
	char *val;
	char *p, *str;
	Arg args[MAXARGS];
	char *envar[MAXARGS];
	wtab_t *w;

	if (argc < 3) {
		printerr(arg0, usagemsg(CONSTCHAR "usage: %s Widgetid index resource:envar ...", argv[0]), NULL);
		return(1);
	}
	w = str_to_wtab(argv[0], argv[1]);
	ind = atoi(argv[2]);
	argv += 3;
	argc -= 3;
	if (w == NULL) {
		return(1);
	}
	/*
	 * Arguments are of the form:
	 *
	 *     resource:envar
	 */

	for (i = 0, n = 0; i < argc; i++) {
		p = strchr(argv[i], ':');
		if (p == NULL) {
			printerr(arg0, CONSTCHAR "Bad argument: ", argv[i]);
			continue;
		}
		*p = '\0';
		args[n].name = strdup(argv[n]);
		envar[n] = &p[1];
		*p = ':';
		args[n].value = (XtArgVal)stakalloc(256);
		n++;
	}
	OlFlatGetValues(w->w, ind, args, n);
	for (i = 0; i < n; i++) {
		if (ConvertTypeToString(arg0, w->wclass, w, w->parent, args[i].name, args[i].value, &str) != FAIL) {
			env_set_var(envar[i], str);
		}
		XtFree(args[i].name);
	}
	return(0);
}

int
do_OlWMProtocolAction(argc, argv)
int argc;
char *argv[];
{
	wtab_t *w;

	if (argc != 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s Widget", argv[0]), NULL);
		return(1);
	}
	w = str_to_wtab(argv[0], argv[1]);
	if (w == NULL) {
		return(1);
	}
	if (! XtIsSubclass(w->w, shellWidgetClass)) {
		printerr(argv[0], CONSTCHAR "Widget must be in the Shell class", NULL);
		return(1);
	}
	if (w->info == NULL) {
		printerr(argv[0], CONSTCHAR "no call_data is available on the named widget", NULL);
		return(1);
	}
	OlWMProtocolAction(w->w, (OlWMProtocolVerify *)(w->info), OL_DEFAULTACTION);
	return(0);
}
