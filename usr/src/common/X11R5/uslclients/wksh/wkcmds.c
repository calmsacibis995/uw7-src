/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:wkcmds.c	1.9.1.3"

/* X includes */

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <setjmp.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include "xpm.h"
#include "wksh.h"


#define MAXARGS 128
#define SLISTITEMSIZE	16

int ConvertStringToType(), ConvertTypeToString();
Widget Toplevel;
void Translation_ksh_eval(), stdCB(), stdInputCB(), stdTimerCB();
int stdWorkProcCB();

static const XtActionsRec Ksh_actions[] = {
	{ "ksh_eval",	Translation_ksh_eval }
};

extern void (*toolkit_addcallback)(), (*toolkit_callcallbacks)();

extern char *strdup();
extern char e_colon[];

#ifdef MOOLIT
#define str_XtRString XtRString
#else
static const char *str_XtRString = XtRString;
#endif

const char str_wksh[] = "wksh";
const char str_s_eq_s[] = "%s=%s";
const char str_s_eq[] = "%s=";
const char str_nill[] = "";
const char str_bad_argument_s[] = "Bad argument: ";
const char str_unknown_pointer_type[] = "Unknown pointer type";
const char str_Pointer[] = "Pointer";
const char str_cannot_parse_s[] = "Cannot parse value, unmatched parentheses, skipped";
const char str_0123456789[] = "0123456789";
const char str_left_over_points_ignored[] = "Draw: left over points ignored";
const char str_polygon_not_supported[] = "Draw: POLYGON not yet supported";
const char str_opt_rect[] = "-rect";
const char str_opt_fillrect[] = "-fillrect";
const char str_opt_fillpoly[] = "-fillpoly";
const char str_opt_line[] = "-line";
const char str_opt_string[] = "-string";
const char str_opt_arc[] = "-arc";
const char str_opt_fillarc[] = "-fillarc";
const char str_opt_point[] = "-point";
const char usage_draw[] = "usage: Draw -<object> handle [args ...]";

/*
 * simply do a sprintf and return a static buffer, a convenient way
 * to format usage messages.
 */

char *
usagemsg(fmt, cmd)
char *fmt, *cmd;
{
	static char out[256];

	sprintf(out, fmt, cmd);
	return(out);
}

static void
wtab_destroy(w, wtab, callData)
Widget w;
wtab_t *wtab;
caddr_t callData; /* not used */
{
	extern int Wtab_free;
	char *env_get();

	/* blank out original variable if it
	 * is still set to the id of the widget
	 */
	if (wtab->envar) {
		char *val = env_get(wtab->envar);
		if (val && wtab->widid && strcmp(val, wtab->widid) == 0) {
			char buf[256];

			sprintf(buf, "unset %s", wtab->envar);
			ksh_eval(buf);
		}
		XtFree(wtab->envar);
	}

	if (wtab->wname)
		XtFree(wtab->wname);
	if (wtab->widid)
		XtFree(wtab->widid);
	if (wtab->info) {
		XtFree(wtab->info);
		wtab->info = NULL;
	}
	wtab->type = TAB_EMPTY;
	Wtab_free++;
}

wtab_t *
set_up_w(wid, parent, var, name, class)
Widget wid;
wtab_t *parent;
char *var, *name;
classtab_t *class;
{
	char widid[8];
	static wtab_t *w;

	get_new_wtab(&w, widid);
	if (var) {
		env_set_var(var, widid);
		w->envar = strdup(var);
	} else {
		w->envar = strdup("none");
	}
	w->type = TAB_WIDGET;
	w->wname = name ? strdup(name) : strdup(XtName(wid));
	w->wclass = class;
	w->parent = parent;
	w->widid = strdup(widid);
	w->w = wid;
	w->info = NULL;
	(*toolkit_addcallback)(wid, XtNdestroyCallback, wtab_destroy, (caddr_t)w);
	return(w);
}

static short Needfree[MAXARGS];

void
parse_args(arg0, argc, argv, w, parent, class, n, args)
char *arg0;
int argc;
char **argv;
wtab_t *w;
wtab_t *parent;
classtab_t *class;
int  *n;
Arg *args;
{
	register int i;
	register char *colon, *resource, *val, *p;
	XtArgVal argval, flatargval;
	int freeflag, len;
	char *flatres;

	*n = 0;
	for (i = 0; i < argc; i++) {
		if (i == MAXARGS) {
			printerr(arg0, CONSTCHAR "Too many arguments, skipping:", argv[*n]);
			continue;
		}
		if ((colon = strchr(argv[i], ':')) == NULL && (colon = strchr(argv[i], '=')) == NULL) {
			printerr(arg0, CONSTCHAR "Bad resource, should be of form 'name:value' : ", argv[i]);
			continue;
		}
		if (*colon == '=') {
			printerr(arg0, CONSTCHAR "WARNING: resource separator is ':' but you used '=' : ", argv[i]);
		}
		val = &colon[1];
		len = colon - argv[i];
		resource = XtMalloc(len + 1);
		strncpy(resource, argv[i], len);
		resource[len] = '\0';
		flatres = NULL;
		if (ConvertStringToType(arg0, w, parent, class, resource, val, &argval, &flatargval, &flatres, &freeflag, !i) != FAIL) {
			XtSetArg(args[*n], resource, argval);
			Needfree[*n] = freeflag;
			(*n)++;
			/* Necessary hack for flats */
			if (flatres != NULL) {
				XtSetArg(args[*n], flatres, flatargval);
				Needfree[*n] = FALSE;
				(*n)++;
			}
		}
	}
}

void
free_args(n, args)
int n;
Arg *args;
{
	register int i;

	/*
	 * Free up argument pointers
	 */
	for (i = 0; i < n; i++) {
		XtFree(args[i].name);
		if (Needfree[i]) {
			XtFree((String)args[i].value);
		}
	}
}

#ifdef DEMO_EXPIRATION_DATE
static void
check_demo_expiration()
{
	/*
	 * This code can be used to create evaluation binary copies
	 * of WKSH that expire on a certain date.
	 *
	 * The DEMO_EXPIRATION_DATE is an integer of the format:
	 * YYMMDD, i.e. if the variables, month, day, and year
	 * hold the desired expiration date, then you derive
	 * the expiration date via the formula:
	 *	 year * 10000 + month * 100 + day
	 */
	 {
#include <time.h>

		time_t now = time(NULL);
		struct tm *today = localtime(&now);
		long date = today->tm_year*10000 + (today->tm_mon+1) * 100 + today->tm_mday;

		if (date > DEMO_EXPIRATION_DATE) {
			fprintf(stderr, CONSTCHAR "\
	=======================================================\n\
	|      YOUR EVALUATION COPY OF WKSH HAS EXPIRED!      |\n\
	|                                                     |\n\
	| Thank you for evaluating UNIX System Laboratories'  |\n\
	| Windowing Korn Shell(tm) product.  We hope you were |\n\
	| impressed by its power and ease of use.             |\n\
	|                                                     |\n\
	| PLEASE REMOVE ALL EVALUATION COPIES OF WKSH FROM    |\n\
	|                  YOUR MACHINES!                     |\n\
	|                                                     |\n\
	| Please return the evaluation software and documents |\n\
	| to UNIX System Laboratories.                        |\n\
	|                                                     |\n\
	|                                                     |\n\
	| If you would like information on how to license     |\n\
	| WKSH, please contact your UNIX System Laboratories  |\n\
	| representative:                                     |\n\
	|                                                     |\n\
	|  In the USA and Canada: 1-800-828-UNIX              |\n\
	|  In Europe:             +44-(0)81-567-7711 (London) |\n\
	|  In Asia/Pacific Rim:   +83-3-5484-8601    (Tokyo)  |\n\
	=======================================================\n");
			fflush(stderr);
			exit(1);
		}
	 }
}
#endif

do_XtAppInitialize(argc, argv)
int argc;
char *argv[];
{
	int ret;
	char *wkdb_hook, *env_get();

#ifdef DEMO_EXPIRATION_DATE
	check_demo_expiration();
#endif
	if (Toplevel != NULL) {
		printerrf(argv[0], "Toolkit already initialized");
		return(1);
	}

	if (argc < 4) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s var app-name app-title [args ...]\n", argv[0]), NULL);
		return(1);
	}
	ret = toolkit_initialize(argc, argv);
	XtAddActions((XtActionList)Ksh_actions, XtNumber(Ksh_actions));
	if ((wkdb_hook = env_get(CONSTCHAR "WKDB_HOOK")) != NULL) {
		ksh_eval(wkdb_hook);
	}
	return(ret);
}

static Widget
WkshCreateShell(name, class, parent, args, nargs)
String name;
WidgetClass class;
Widget parent;
ArgList args;
Cardinal nargs;
{
	return(XtCreateApplicationShell(name, class, args, nargs));
}

do_XtAppCreateShell(argc, argv)
int argc;
char *argv[];
{
	return(_CreateWidget(WkshCreateShell, argc, argv));
}

do_XtCreatePopupShell(argc, argv)
int argc;
char *argv[];
{
	return(_CreateWidget(XtCreatePopupShell, argc, argv));
}

do_XtCreateManagedWidget(argc, argv)
int argc;
char *argv[];
{
	return(_CreateWidget(XtCreateManagedWidget, argc, argv));
}

do_XtCreateWidget(argc, argv)
int argc;
char *argv[];
{
	return(_CreateWidget(XtCreateWidget, argc, argv));
}

static int
_CreateWidget(func, argc, argv)
Widget (*func)();
int argc;
char *argv[];
{
	Widget widget;
	classtab_t *class;
	char *arg0 = argv[0];
	wtab_t *w, *pw, *wtab;
	char *wname, *wclass, *parentid, *var;
	Arg	args[MAXARGS];
	register int	i;
	int n;

	if (argc < 5) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s var name class parent [arg:val ...]\n", argv[0]), NULL);
		return(1);
	}
	var = argv[1];
	wname = argv[2];
	wclass = argv[3];
	parentid = argv[4];
	pw = str_to_wtab(argv[0], parentid);
	if (pw == NULL) {
		printerr(argv[0], CONSTCHAR "Could not find parent\n", NULL);
		return(1);
	}
	argv += 5;
	argc -= 5;
	if ((class = str_to_class(arg0, wclass)) == NULL) {
		return(1);
	}
	n = 0;
	parse_args(arg0, argc, argv, NULL, pw, class, &n, args);
	widget = func(wname, class->class, pw->w, args, n);
	if (widget != NULL) {
		wtab = set_up_w(widget, pw, var, wname, class);

		if (class->initfn)
			class->initfn(arg0, wtab, var);
	} else {
		printerr(argv[0], usagemsg(CONSTCHAR "Creation of widget '%s' failed\n", wname), NULL);
		env_blank(argv[1]);
	}
	free_args(n, args);

	return(0);
}

do_XtPopup(argc, argv)
int argc;
char *argv[];
{
	wtab_t *w;
	XtGrabKind grab;

	if (argc < 2 || argc > 3) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s widgetid [GrabNone|GrabNonexclusive|GrabExclusive]\n", argv[0]), NULL);
		return(1);
	}
	w = str_to_wtab(argv[0], argv[1]);
	if (w == NULL) {
		return(1);
	} else {
		grab = XtGrabNone;
		if (argc < 3 || strcmp(argv[2], CONSTCHAR "GrabNone") == 0) {
			grab = XtGrabNone;
		} else if (strcmp(argv[2], CONSTCHAR "GrabNonexclusive") == 0) {
			grab = XtGrabNonexclusive;
		} else if (strcmp(argv[2], CONSTCHAR "GrabExclusive") == 0) {
			grab = XtGrabExclusive;
		} else {
			printerr(argv[0], CONSTCHAR "Warning: Grab type unknown, using GrabNone", NULL);
			grab = XtGrabNone;
		}
		XtPopup(w->w, grab);
	}
	return(0);
}

static int
_WKSH_XtDestroyWidget(w)
Widget w;
{
	XtDestroyWidget(w);
	return(1);
}

int
do_XtDestroyWidget(argc, argv)
int argc;
char *argv[];
{
	return(do_single_widget_arg_func(_WKSH_XtDestroyWidget, argc, argv));
}

static int
do_single_widget_arg_func(func, argc, argv)
int (*func)();
int argc;
char **argv;
{
	wtab_t *w;
	register int i;

	if (argc < 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s widgetid ...\n", argv[0]), NULL);
		return(1);
	}
	for (i = 1; i < argc; i++) {
		w = str_to_wtab(argv[0], argv[i]);
		if (w != NULL) {
			func(w->w);
		}
	}
	return(0);
}

static int
do_single_widget_test_func(func, argc, argv)
int (*func)();
int argc;
char **argv;
{
	wtab_t *w;
	register int i;

	if (argc != 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s widgetid\n", argv[0]), NULL);
		return(1);
	}
	w = str_to_wtab(argv[0], argv[1]);
	if (w != NULL) {
		return(!func(w->w));
	}
	return(255);
}

int
do_XtIsSensitive(argc, argv)
int argc;
char *argv[];
{
	return(do_single_widget_test_func(XtIsSensitive, argc, argv));
}

int
do_XtIsManaged(argc, argv)
int argc;
char *argv[];
{
	return(do_single_widget_test_func(XtIsManaged, argc, argv));
}

int
do_XtIsRealized(argc, argv)
int argc;
char *argv[];
{
	return(do_single_widget_test_func(XtIsRealized, argc, argv));
}

int
do_XtRealizeWidget(argc, argv)
int argc;
char *argv[];
{
	return(do_single_widget_arg_func(XtRealizeWidget, argc, argv));
}

int
do_XtUnrealizeWidget(argc, argv)
int argc;
char *argv[];
{
	return(do_single_widget_arg_func(XtUnrealizeWidget, argc, argv));
}

/*
 * XtMapWidget() is a macro, so can't use do_single_widget_arg_func()
 */

int
do_XtMapWidget(argc, argv)
int argc;
char *argv[];
{
	wtab_t *w;
	register int i;

	if (argc < 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s widgetid ...\n", argv[0]));
		return(1);
	}
	for (i = 1; i < argc; i++) {
		w = str_to_wtab(argv[0], argv[i]);
		if (w != NULL) {
			XtMapWidget(w->w);
		}
	}
	return(0);
}

int
do_XtUnmapWidget(argc, argv)
int argc;
char **argv;
{
	wtab_t *w;
	register int i;

	if (argc < 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s widgetid ...\n", argv[0]), NULL);
		return(1);
	}
	for (i = 1; i < argc; i++) {
		w = str_to_wtab(argv[0], argv[i]);
		if (w != NULL) {
			XtUnmapWidget(w->w);
		}
	}
	return(0);
}

do_XtPopdown(argc, argv)
int argc;
char **argv;
{
	return(do_single_widget_arg_func(XtPopdown, argc, argv));
}

static jmp_buf MainLoopJump;

static void
mainloopsighandler(sig)
int sig;
{
	longjmp(MainLoopJump, 1);
}

do_XtMainLoop(argc, argv)
int argc;
char **argv;
{
	extern void sh_fault();
	void (*old)();
	/*
	 * This will continue until the user hits the <DEL> key
	 */

	if (setjmp(MainLoopJump) == 1) {
		signal(SIGINT, sh_fault);
		sh_fault(SIGINT);
		return(1);
	}
	old = signal(SIGINT, mainloopsighandler);
	XtMainLoop();
	signal(SIGINT, old);
	printerr(argv[0], CONSTCHAR "MainLoop failed\n", NULL);
	return(1);
}

static int
XtAddCallback_usage(arg0)
char *arg0;
{
	printerr(arg0, usagemsg(CONSTCHAR "usage: %s [-n] widget callbackname ksh-command", arg0), NULL);
	return(1);
}

static int
XtCallCallbacks_usage(arg0)
char *arg0;
{
	printerr(arg0, usagemsg(CONSTCHAR "usage: %s Widgetid Callbackname", arg0), NULL);
	return(1);
}


int
do_XtCallCallbacks(argc, argv)
int argc;
char **argv;
{
	wtab_t *w;
	char *arg0 = argv[0];

	if (argc != 3) {
		return(XtCallCallbacks_usage(arg0));
	}
	w = str_to_wtab(argv[0], argv[1]);
	if (w == NULL) {
		return(1);
	} else {
		(*toolkit_callcallbacks)(w->w, argv[2], NULL);
	}
	return(0);
}

int
do_XtAddCallback(argc, argv)
int argc;
char **argv;
{
	wtab_t *w;
	int nocalldata = 0;
	char *arg0 = argv[0];

	if (argc != 4 && argc != 5) {
		return(XtAddCallback_usage(arg0));
	}
	if (argc == 5) {
		if (strcmp(argv[1], CONSTCHAR "-n") == 0) {
			nocalldata++;
			argv++;
			argc--;
		} else
			return(XtAddCallback_usage(arg0));
	}
	w = str_to_wtab(argv[0], argv[1]);
	if (w == NULL) {
		return(1);
	} else {
		wksh_client_data_t *cdata;
		const callback_tab_t *cbtab;

		cdata = (wksh_client_data_t *)XtMalloc(sizeof(wksh_client_data_t));
		cdata->ksh_cmd = strdup(argv[3]);
		cdata->w = w;

		if (nocalldata) {
			cbtab = NULL;
		} else {
			for (cbtab = w->wclass->cbtab; cbtab != NULL && cbtab->resname != NULL; cbtab++) {
				if (strcmp(cbtab->resname, argv[2]) == 0)
					break;
			}
		}
		if (cbtab != NULL && cbtab->resname != NULL)
			cdata->cbtab = cbtab;
		else
			cdata->cbtab = NULL;
		(*toolkit_addcallback)(w->w, argv[2], stdCB, (XtPointer)cdata);
	}
	return(0);
}

int
do_XtGetValues(argc, argv)
int argc;
char **argv;
{
	register int i, j;
	int n;
	char *arg0 = argv[0];
	char *val, *p, *str;
	Arg args[MAXARGS];
	char *envar[MAXARGS];
	wtab_t *w;

	if (argc < 3) {
		printerr(arg0, usagemsg(CONSTCHAR "usage: %s Widgetid resource:envar ...", argv[0]), NULL);
		return(1);
	}
	w = str_to_wtab(argv[0], argv[1]);
	argv += 2;
	argc -= 2;
	if (w == NULL) {
		return(1);
	}
	/*
	 * Arguments are of the form:
	 *
	 *     resource:envar
	 */

	for (i = 0, n = 0; i < argc; i++) {
		if ((p = strchr(argv[i], ':')) == NULL) {
			printerr(arg0, CONSTCHAR "Bad argument: ", argv[i]);
			continue;
		}
		*p = '\0';
		args[n].name = strdup(argv[n]);
		envar[n] = &p[1];
		*p = ':';
		args[n].value = (XtArgVal)stakalloc(1024);
		n++;
	}
	XtGetValues(w->w, args, n);
	for (i = 0; i < n; i++) {
		if (ConvertTypeToString(arg0, w->wclass, w, w->parent, args[i].name, args[i].value, &str) != FAIL) {
			env_set_var(envar[i], str);
		}
		XtFree(args[i].name);
	}
	return(0);
}

int
do_XtSetValues(argc, argv)
int argc;
char **argv;
{
	int n;
	char *arg0 = argv[0];
	Arg args[MAXARGS];
	wtab_t *w;

	if (argc < 3) {
		printerr(arg0, usagemsg(CONSTCHAR "usage: %s Widgetid arg:val ...", arg0), NULL);
		return(1);
	}
	w = str_to_wtab(argv[0], argv[1]);
	argv += 2;
	argc -= 2;
	if (w == NULL) {
		return(1);
	} else {
		n = 0;
		parse_args(arg0, argc, argv, w, w->parent, w->wclass, &n, args);
		if (n > 0) {
			XtSetValues(w->w, args, n);
			free_args(n, args);
		} else {
			printerr(arg0, CONSTCHAR "Nothing to set", NULL);
			return(1);
		}
	}
	return(0);
}

static void
parse_indexlist(argc, argv, max, indices, numindices)
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
			continue;
		}
		indices[*numindices] = atoi(argv[i]);
		if (indices[*numindices] < 0 || indices[*numindices] > max) {
			continue;
		}
		(*numindices)++;
	}
}

int
do_XtAddWorkProc(argc, argv)
int argc;
char *argv[];
{
	char *variable;
	char *cmd;
	char buf[1024];

	if (argc != 3) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s variable command", argv[0]), NULL);
		return(1);
	}

	variable = argv[1];
	cmd = strdup((char *)argv[2]);
	sprintf(buf, "%s=0x%x", variable, XtAddWorkProc(stdWorkProcCB, (XtPointer)cmd));
	env_set(buf);
	return(0);
}

int
do_XtRemoveWorkProc(argc, argv)
int argc;
char *argv[];
{
	XtWorkProcId id;
	char *p;

	if (argc != 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s workprocid", argv[0]), NULL);
		return(1);
	}
	id = strtoul(argv[1], &p, 16);
	if (p == argv[1]) {
		printerrf(argv[0], "Argument must be a hex number");
		return(1);
	}
	XtRemoveWorkProc(id);
	return(0);
}

int
do_XtAddTimeOut(argc, argv)
int argc;
char *argv[];
{
	unsigned long milliseconds = 0;
	wtab_t *w;
	char *variable;
	char *cmd;
	char buf[256];

	if (argc != 4) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s variable milliseconds command", argv[0]), NULL);
		return(1);
	}

	variable = argv[1];
	if ((milliseconds = atoi(argv[2])) <= 0) {
		printerr(argv[0], CONSTCHAR "Milliseconds must be greater than zero", NULL);
		return(1);
	}
	cmd = strdup((char *)argv[3]);
	sprintf(buf, "%s=0x%x", variable, XtAddTimeOut(milliseconds, stdTimerCB, (XtPointer)cmd));
	env_set(buf);
	return(0);
}

int
do_XtRemoveTimeOut(argc, argv)
int argc;
char *argv[];
{
	XtIntervalId id;
	char *p;

	if (argc != 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s intervalid", argv[0]), NULL);
		return(1);
	}
	id = strtoul(argv[1], &p, 16);
	if (p == argv[1]) {
		printerrf(argv[0], "Argument must be a hex number");
		return(1);
	}
	XtRemoveTimeOut(id);
	return(0);
}

do_XtUnmanageChildren(argc, argv)
int argc;
char *argv[];
{
	return(do_managelist_func(argc, argv, XtUnmanageChildren));
}

do_XtManageChildren(argc, argv)
int argc;
char *argv[];
{
	return(do_managelist_func(argc, argv, XtManageChildren));
}

int
do_managelist_func(argc, argv, func)
int argc;
char *argv[];
int (*func)();
{
	wtab_t *w;
	register int i;
	Widget widgets[MAXARGS];
	Cardinal nwidgets;

	if (argc < 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s widgetid ...\n", argv[0]), NULL);
		return(1);
	}
	for (nwidgets = 0, i = 1; i < argc && nwidgets < MAXARGS; i++) {
		w = str_to_wtab(argv[0], argv[i]);
		if (w != NULL) {
			widgets[nwidgets++] = w->w;
		}
	}
	func(widgets, nwidgets);
	return(0);
}

#define PARSE_POINTLIST (-1)
#define PARSE_IMAGE (-2)

GC Standard_GC;

int
create_standard_gc(toplevel)
Widget toplevel;
{
	Standard_GC = XCreateGC(XtDisplay(toplevel), XtWindow(toplevel), 0, NULL);
	return(0);
}

do_XBell(argc, argv)
int argc;
char *argv[];
{
	int volume;

	if (Toplevel == NULL) {
		printerr(argv[0], CONSTCHAR "can't ring bell: toolkit not initialized.", NULL);
		return(1);
	}
	if (argc == 1) {
		volume = 0;
	} else if (argc > 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "Usage: %s volume (volume must be between -100 and 100", argv[0]), NULL);
	} else
		volume = atoi(argv[2]);
	if (volume < -100)
		volume = -100;
	else if (volume > 100)
		volume = 100;

	XBell(XtDisplay(Toplevel), volume);
	XFlush(XtDisplay(Toplevel));
	return(0);
}

int
do_XtRemoveAllCallbacks(argc, argv)
int argc;
char *argv[];
{
	wtab_t *w;
	register int i;

	if (argc < 3) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s widgetid callbackname\n", argv[0]), NULL);
		return(1);
	}
	w = str_to_wtab(argv[0], argv[1]);
	if (w != NULL) {
		XtRemoveAllCallbacks(w->w, argv[2]);
		return(0);
	} else
		return(1);
}

static int
cvtfontstruct(name, fn)
char *name;
XFontStruct **fn;
{
        XrmValue fval, tval;

        fval.addr = name;
        fval.size = strlen(name);
        XtConvert(Toplevel, XtRString, &fval, XtRFontStruct, &tval);

        if (tval.size != 0) {
                *fn = ((XFontStruct **)(tval.addr))[0];
                return(SUCCESS);
        } else
                return(FAIL);
}

static int
cvtfont(name, fn)
char *name;
Font *fn;
{
        XrmValue fval, tval;

        fval.addr = name;
        fval.size = strlen(name);
        XtConvert(Toplevel, XtRString, &fval, XtRFont, &tval);

        if (tval.size != 0) {
                *fn = ((Font *)(tval.addr))[0];
                return(SUCCESS);
        } else
                return(FAIL);
}

static int
cvtcolor(name, pix)
char *name;
Pixel *pix;
{
        XrmValue fval, tval;

        fval.addr = name;
        fval.size = strlen(name);
        XtConvert(Toplevel, XtRString, &fval, XtRPixel, &tval);

        if (tval.size != 0) {
                *pix = ((Pixel *)(tval.addr))[0];
                return(SUCCESS);
        } else
                return(FAIL);
}

int
do_XTextWidth(argc, argv)
int argc;
char *argv[];
{
	XFontStruct *fn;
	char *s;

	if (argc != 3) {
		printerrf(argv[0], "Usage: XTextWidth font string");
		return(1);
	}
	if (cvtfontstruct(argv[1], &fn) != SUCCESS)
		printerrf(argv[0], "XTextWidth: bad font: %s", argv[1]);
	s = argv[2];
	altprintf("%d\n", XTextWidth(fn, s, strlen(s)));
	return(0);
}

Pixel PSfgcolor = 0;

#define YORIGIN (72*11)
#define YMARGIN (36)
#define XMARGIN (36)

static int
PSSetupColor()
{
#define RGBSCALE	((float)65535.0)

	if (PSfgcolor != 0) {	/* don't bother for black */
		Colormap cm = XDefaultColormapOfScreen(XtScreen(Toplevel));
		XColor col;

		col.pixel = PSfgcolor;
		XQueryColor(XtDisplay(Toplevel), cm, &col);
		altprintf("%.2f %.2f %.2f setrgbcolor\n",
			(float)col.red/RGBSCALE,
			(float)col.green/RGBSCALE,
			(float)col.blue/RGBSCALE);
	}

#undef RGBSCALE
}

PSDrawString(dpy, win, gc, x, y, s, len)
Display *dpy;	/* unused */
Drawable win;	/* unused */
GC gc;		/* unused */
int x, y;
char *s;
int len;
{
	altputs("gsave");
	altputs("newpath");
	altprintf("%d %d moveto\n", x+XMARGIN, YORIGIN-y-YMARGIN);
	altprintf("/Times-Bold findfont\n11 scalefont\nsetfont\n");
	PSSetupColor();
	altprintf("(%s) show\n", s);
	altputs("grestore");
}

PSDrawLine(dpy, win, gc, x1, y1, x2, y2)
Display *dpy;	/* unused */
Drawable win;	/* unused */
GC gc;		/* unused */
int x1, y1, x2, y2;
{
	altputs("gsave");
	altputs("newpath");
	altprintf("%d %d moveto\n", x1+XMARGIN, YORIGIN-y1-YMARGIN);
	altprintf("%d %d lineto\n", x2+XMARGIN, YORIGIN-y2-YMARGIN);
	PSSetupColor();
	altputs("stroke");
	altputs("grestore");
}

PSDrawRectangle(dpy, win, gc, x, y, w, h)
Display *dpy;	/* unused */
Drawable win;	/* unused */
GC gc;		/* unused */
int x, y, w, h;
{
	altputs("gsave");
	altputs("newpath");
	altprintf("%d %d moveto\n", x+XMARGIN, YORIGIN-y-YMARGIN);
	altprintf("%d %d rlineto\n", w, 0);
	altprintf("%d %d rlineto\n", 0, 0-h);
	altprintf("%d %d rlineto\n", 0-w, 0);
	altputs("closepath");
	PSSetupColor();
	altputs("stroke");
	altputs("grestore");
}

PSFillArc(dpy, win, gc, x, y, w, h, r1, r2)
Display *dpy;	/* unused */
Drawable win;	/* unused */
GC gc;		/* unused */
int x, y, w, h, r1, r2;
{
#define MIN(x,y) ((x) > (y) ? (y) : (x))

	altputs("gsave");
	altputs("newpath");
	altprintf("%d %d moveto\n", x+XMARGIN + w/2, YORIGIN-y-YMARGIN - h/2);
	altprintf("%d %d %d %.2f %.2f arc\n", 
		x+XMARGIN + w/2, YORIGIN-y-YMARGIN - h/2,
		MIN(h,w)/2, (float)r1/64.0, (float)(r1+r2)/64.0);
	altputs("closepath");
	PSSetupColor();
	altputs("fill");
	altputs("grestore");

#undef MIN
}

PSDrawArc(dpy, win, gc, x, y, w, h, r1, r2)
Display *dpy;	/* unused */
Drawable win;	/* unused */
GC gc;		/* unused */
int x, y, w, h, r1, r2;
{
#define MIN(x,y) ((x) > (y) ? (y) : (x))
	altputs("gsave");
	altputs("newpath");
	altprintf("%d %d moveto\n", x+XMARGIN + w/2, YORIGIN-y-YMARGIN - h/2);
	altprintf("%d %d %d %.2f %.2f arc\n", 
		x+XMARGIN + w/2, YORIGIN-y-YMARGIN - h/2,
		MIN(h,w)/2, (float)r1/64.0, (float)(r1+r2)/64.0);
	altputs("closepath");
	PSSetupColor();
	altputs("stroke");
	altputs("grestore");
#undef MIN
}

PSFillRectangle(dpy, win, gc, x, y, w, h)
Display *dpy;
Drawable win;
GC gc;
int x, y, w, h;
{
	altputs("gsave");
	altputs("newpath");
	altprintf("%d %d moveto\n", x+XMARGIN, YORIGIN-y-YMARGIN);
	altprintf("%d %d rlineto\n", w, 0);
	altprintf("%d %d rlineto\n", 0, 0-h);
	altprintf("%d %d rlineto\n", 0-w, 0);
	altputs("closepath");
	PSSetupColor();
	altputs("fill");
	altputs("grestore");
}

#define MaxPixmapCache 15

struct {
	char *name;
	XImage *image;
	int hits, width, height;
} PixmapCache[MaxPixmapCache];

static int PixmapHit = 0;

int
do_XDraw(argc, argv)
int argc;
char *argv[];
{
	wtab_t *w;
	char *arg0 = argv[0];
	char *funcname = argv[1];
	char *s;
	char *env_get();
	Region reg;
	register int i;
	int mode, parse;
	int text = FALSE;
	int (*func)();
	int (*PSfunc)();
	int PSmode = 0;
	int argtype = 0;
	int polymode;
	int coordmode;
	GC  gc = NULL;
#define MAXDRAWARGS 6
#define LINE_ARGS 1
#define POLYGON_ARGS 2
	int p[MAXDRAWARGS];
	int XDrawRectangle(), XFillRectangle(), XFillPolygon(),
		XDrawLine(), XDrawText(), XDrawSegment(), XDrawArc(),
		XFillArc(), XDrawPoint(), XPutImage();

	PSfgcolor = 0;

	if (argc < 3) {
		printerr(argv[0], usage_draw, NULL);
		return(1);
	}
	PSfunc = NULL;
	if (strcmp(argv[1], str_opt_rect) == 0) {
		parse = 4;
		func = XDrawRectangle;
		PSfunc = PSDrawRectangle;
	} else if (strcmp(argv[1], str_opt_fillrect) == 0) {
		parse = 4;
		func = XFillRectangle;
		PSfunc = PSFillRectangle;
	} else if (strcmp(argv[1], str_opt_fillpoly) == 0) {
		parse = PARSE_POINTLIST;
		func = XFillPolygon;
		argtype = POLYGON_ARGS;
		polymode = Complex;
		coordmode = CoordModeOrigin;
		while (argv[2][0] == '-') {
			if (strcmp(argv[2], "-complex") == 0) {
				continue;
			} else if (strcmp(argv[2], "-convex") == 0) {
				polymode = Convex;
			} else if (strcmp(argv[2], "-nonconvex") == 0) {
				polymode = Nonconvex;
			} else if (strcmp(argv[2], "-coordmodeorigin") == 0) {
				continue;
			} else if (strcmp(argv[2], "-coordmodeprevious") == 0) {
				coordmode = CoordModePrevious;
			}
			argc--;
			argv++;
		}
	} else if (strcmp(argv[1], str_opt_line) == 0) {
		parse = 4;
		func = XDrawLine;
		PSfunc = PSDrawLine;
	} else if (strcmp(argv[1], "-segments") == 0) {
		parse = PARSE_POINTLIST;
		func = XDrawSegments;
		argtype = LINE_ARGS;
	} else if (strcmp(argv[1], "-lines") == 0) {
		parse = PARSE_POINTLIST;
		func = XDrawLines;
		argtype = LINE_ARGS;
		coordmode = CoordModeOrigin;
		while (argv[2][0] == '-') {
			if (strcmp(argv[2], "-coordmodeorigin") == 0) {
				continue;
			} else if (strcmp(argv[2], "-coordmodeprevious") == 0) {
				coordmode = CoordModePrevious;
			}
			argc--;
			argv++;
		}
	} else if (strcmp(argv[1], str_opt_string) == 0) {
		parse = 2;
		text = TRUE;
		func = XDrawString;
		PSfunc = PSDrawString;
	} else if (strcmp(argv[1], str_opt_arc) == 0) {
		parse = 6;
		func = XDrawArc;
		PSfunc = PSDrawArc;
	} else if (strcmp(argv[1], str_opt_fillarc) == 0) {
		parse = 6;
		func = XFillArc;
		PSfunc = PSFillArc;
	} else if (strcmp(argv[1], str_opt_point) == 0) {
		parse = 2;
		func = XDrawPoint;
	} else if (strcmp(argv[1], "-clearall") == 0) {
		parse = 0;
		func = XClearWindow;
	} else if (strcmp(argv[1], "-clear") == 0) {
		parse = 5;
		func = XClearArea;
	} else if (strcmp(argv[1], "-putimage") == 0) {
		parse = PARSE_IMAGE;
		func = XPutImage;
	} else {
		printerrf(argv[0], "%s: no drawing mode specified", argv[0]);
		return(1);
	}
	if (Standard_GC == NULL)
		create_standard_gc(Toplevel);
	/*
	 * if the "EXPOSE_REGION" variable is set, we assume the
	 * user wishes to clip to a region (i.e. we are being called
	 * from an expose event handler of some kind).  The user can always
	 * do an "unset" of this variable if they wish to remove
	 * this feature.
	 */

	if ((s = env_get(CONSTCHAR "EXPOSE_REGION")) != NULL) {
		char *p;

		reg = (Region) strtoul(s, &p, 0);

		if (reg != NULL) {
			gc = XCreateGC(XtDisplay(Toplevel), XtWindow(Toplevel), 0, NULL);
			XSetRegion(XtDisplay(Toplevel), gc, reg);
		}
	}

	while (argc > 3 && argv[2][0] == '-') {
		if (gc == NULL)
			gc = XCreateGC(XtDisplay(Toplevel), XtWindow(Toplevel), 0, NULL);

		if (strcmp(argv[2], "-gc") == 0) {
			XFreeGC(XtDisplay(Toplevel), gc);
			gc = (GC) atoi(argv[3]);
		} else if (strcmp(argv[2], "-fg") == 0) {
			Pixel pix;
			if (cvtcolor(argv[3], &pix) == SUCCESS) {
				XSetForeground(XtDisplay(Toplevel), gc, pix);
				PSfgcolor = pix;
			}
		} else if (strcmp(argv[2], "-bg") == 0) {
			Pixel pix;
			if (cvtcolor(argv[3], &pix) == SUCCESS)
				XSetBackground(XtDisplay(Toplevel), gc, pix);
		} else if (strcmp(argv[2], "-fn") == 0) {
			Font fn;

			if (cvtfont(argv[3], &fn) == SUCCESS)
				XSetFont(XtDisplay(Toplevel), gc, fn);
			else
				printerrf(arg0, "XDraw: bad font: %s", argv[3]);
		} else if (strcmp(argv[2], "-lw") == 0) {
			XGCValues v;

			v.line_width = atoi(argv[3]);
			XChangeGC(XtDisplay(Toplevel), gc, GCLineWidth, &v);
		} else if (strcmp(argv[2], "-func") == 0) {
			XGCValues v;
			long f;

			if (strcmp(argv[3], "xor") == 0) {
				f = GXxor;
			} else if (strcmp(argv[3], "or") == 0) {
				f = GXor;
			} else if (strcmp(argv[3], "clear") == 0) {
				f = GXclear;
			} else if (strcmp(argv[3], "and") == 0) {
				f = GXand;
			} else if (strcmp(argv[3], "copy") == 0) {
				f = GXcopy;
			} else if (strcmp(argv[3], "noop") == 0) {
				f = GXnoop;
			} else if (strcmp(argv[3], "nor") == 0) {
				f = GXnor;
			} else if (strcmp(argv[3], "nand") == 0) {
				f = GXnand;
			} else if (strcmp(argv[3], "set") == 0) {
				f = GXset;
			} else if (strcmp(argv[3], "invert") == 0) {
				f = GXinvert;
			} else if (strcmp(argv[3], "equiv") == 0) {
				f = GXequiv;
			} else if (strcmp(argv[3], "andReverse") == 0) {
				f = GXandReverse;
			} else if (strcmp(argv[3], "orReverse") == 0) {
				f = GXorReverse;
			} else if (strcmp(argv[3], "copyInverted") == 0) {
				f = GXcopyInverted;
			} else {
				printerrf(arg0, "%s: bad graphics function name: %s", arg0, argv[3]);
				return(1);
			}
			v.function = f;
			XChangeGC(XtDisplay(Toplevel), gc, GCFunction, &v);
		} else if (strcmp(argv[2], "-linestyle") == 0) {
			XGCValues v;
			long f;

			if (strcmp(argv[3], "solid") == 0) {
				f = LineSolid;
			} else if (strcmp(argv[3], "doubledash") == 0) {
				f = LineDoubleDash;
			} else if (strcmp(argv[3], "onoffdash") == 0) {
				f = LineOnOffDash;
			} else {
				printerrf(arg0, "%s: bad line style: %s, should be one of: solid, doubledash, onoffdash", arg0, argv[3]);
				return(1);
			}
			v.line_style = f;
			XChangeGC(XtDisplay(Toplevel), gc, GCLineStyle, &v);
		} else {
			printerrf(arg0, "unknown option: '%s'", argv[2]);
		}
		argv += 2;
		argc -= 2;
	}
	if (gc == NULL)
		gc = Standard_GC;
	if (strcmp(argv[2], "POSTSCRIPT") == 0) {
		PSmode++;
		if (PSfunc == NULL) {
			printerrf(arg0, "%s: POSTSCRIPT not available for %s function", arg0, funcname);
			return(1);
		}
	} else if ((w = str_to_wtab(argv[0], argv[2])) == NULL) {
		if (gc != Standard_GC)
			XFreeGC(XtDisplay(Toplevel), gc);
		return(1);
	}
	argc -= 3;
	argv += 3;
	if (parse == PARSE_POINTLIST) {
		XPoint *points = (XPoint *)XtMalloc(sizeof(XPoint )*(argc/2+1));
		int npoints = 0;

		for (i = 0; i < argc-1; i += 2, npoints++) {
			points[npoints].x = atoi(argv[i]);
			points[npoints].y = atoi(argv[i+1]);
		}
		switch (argtype) {
		case POLYGON_ARGS:
			(PSmode ? PSfunc : func)( PSmode ? NULL : XtDisplay(w->w), PSmode ? NULL : XtWindow(w->w), gc, points, argc/2, polymode, coordmode);
			break;
		case LINE_ARGS:
			(PSmode ? PSfunc : func)( PSmode ? NULL : XtDisplay(w->w), PSmode ? NULL : XtWindow(w->w), gc, points, argc/2, coordmode);
			break;
		default:
			printerrf(arg0, "XDraw: internal error\n");
			return(1);
		}
		argc -= 2*npoints;
		argv += 2*npoints;
	} else if (parse == PARSE_IMAGE) {
		XImage *image;
		int format;
		int code;
		Pixmap pixmap;
		unsigned int width, height, x, y;
		int depth, hotx, hoty;
		Display *display;
		Colormap colormap;
		Screen *screen;
		Window root;
		int found;

		if (argc != 3) {
			printerrf("XDraw -putimage", "usage: XDraw -putimage path x y");
			return(1);
		}
		
		screen = XtScreen(w->w);
		colormap = DefaultColormapOfScreen(screen);
		depth = DefaultDepthOfScreen(screen);
		display = DisplayOfScreen(screen);
		root = RootWindowOfScreen(screen);
		/*
		 * see if it's in the (small) cache
		 */
		for (found=FALSE, i=0; !found && i < MaxPixmapCache && PixmapCache[i].name; i++) {
			if (strcmp(argv[0], PixmapCache[i].name) == 0) {
				width = PixmapCache[i].width;
				height = PixmapCache[i].height;
				image = PixmapCache[i].image;
				PixmapCache[i].hits = PixmapHit++;
				found = TRUE;
				printerrf(arg0, "Found image %s in cache\n", argv[0]);
			}
		}

		if (!found) {
			printerrf(arg0, "image %s not in cache, reading file\n", argv[0]);
			if (XReadPixmapFile(display, root, colormap, argv[0], &width, &height, depth, &pixmap) != PixmapSuccess)  {
				printerrf("XDraw -putimage", "Could not read pixmap file %s\n", argv[0]);
				return(1);
			}
			if ((image = XGetImage(display, (Drawable)pixmap, 0, 0, width, height, AllPlanes, XYPixmap)) == NULL) {
				printerrf("XDraw -putimage", "Could not get image\n", argv[0]);
				return(1);
			}
			/* Room in the cache?  Add this one */
			if (i < MaxPixmapCache) {
				PixmapCache[i].name = strdup(argv[0]);
				PixmapCache[i].image = image;
				PixmapCache[i].hits = PixmapHit++;
				PixmapCache[i].width = width;
				PixmapCache[i].height = height;
				printerrf("imagecache", "putting image in cache slot %d\n", i);
			} else {
				/* No room? blow away the LRU */

				int oldest = 1000000;
				int loser = -1;
				for (i=0; i < MaxPixmapCache; i++) {
					if (PixmapCache[i].hits <= oldest) {
						loser = i;
						oldest = PixmapCache[i].hits;
					}
				}
				i = loser;
				XtFree(PixmapCache[i].name);
				XDestroyImage(PixmapCache[i].image);
				PixmapCache[i].name = strdup(argv[0]);
				PixmapCache[i].image = image;
				PixmapCache[i].hits = PixmapHit++;
				PixmapCache[i].width = width;
				PixmapCache[i].height = height;
				printerrf("imagecache", "putting image in cache slot %d\n", i);
			}
		}

		x = atoi(argv[1]);
		y = atoi(argv[2]);
		argv += 3;
		argc -= 3;
		XPutImage(display, XtWindow(w->w), gc, image,
			0, 0, x, y, width, height);
	} else {
		while (argc >= parse) {
			for (i = 0; i < parse && i < argc; i++) {
				p[i] = atoi(argv[i]);
			}
			if (text) {
				(PSmode ? PSfunc : func)(PSmode ? NULL : XtDisplay(w->w), PSmode ? NULL : XtWindow(w->w), gc, p[0], p[1], argv[i], strlen(argv[i]));
				argc--;
				argv++;
			} else
				(PSmode ? PSfunc : func)(PSmode ? NULL : XtDisplay(w->w), PSmode ? NULL : XtWindow(w->w), gc, p[0], p[1], p[2], p[3], p[4], p[5]);
			argc -= parse;
			argv += parse;
			if (parse == 0)
				break;
		}
	}
	if (argc != 0) {
		printerr(arg0, str_left_over_points_ignored, NULL);
		return(1);
	}
	if (gc != Standard_GC)
		XFreeGC(XtDisplay(Toplevel), gc);
	return(0);

#undef LINE_ARGS 
#undef POLYGON_ARGS
}

int
ConvertTypeToString(arg0, class, w, parent, resource, val, ret)
char *arg0;
classtab_t *class;
wtab_t *w;
wtab_t *parent;
char *resource;
XtArgVal val;
char **ret;
{
	char *from_type;
	XtResourceList res;
	XrmValue    fr_val, to_val;
	struct namnod *nam;

	if ((nam = nam_search(resource, class->res, 0)) == NULL) {
		/* If we didn't find it in this widget's class record,
		 * see if the parent is a constraint widget class, and
		 * if so then see if we can find the class there.
		 */
		if (parent == NULL || parent->wclass == NULL ||
			parent->wclass->con == NULL ||
			(nam = nam_search(resource, parent->wclass->con, 0)) == NULL) {
			printerr(arg0, usagemsg(
				CONSTCHAR "No such resource for '%s' widget: ", 
				class->cname), resource);
			return(FAIL);
		}
	}
	res = (XtResourceList)(nam->value.namval.cp);

	/*
	 * unfortunately, we have to have a special case for String
	 * type resources, since their size may vary.
	 */
	if (strcmp(res->resource_type, str_XtRString) == 0) {
		char *strccpy(), *strdup();

		*ret = ((String *)val)[0];
		return(0);
	}
	fr_val.size = res->resource_size;
	fr_val.addr = (caddr_t)val;
	to_val.size = 0;
	to_val.addr = NULL;

	XtConvert(
	    w ? w->w : Toplevel,
	    res->resource_type,	/* from type */
	    &fr_val,	/* from value */
	    str_XtRString,	/* to type */
	    &to_val	/* the converted value */
	);
	if (to_val.addr) {
		*ret = to_val.addr;
	} else {
	    printerr(arg0, usagemsg(CONSTCHAR "Failed conversion from '%s' to 'String'", res->resource_type), NULL);
		return(FAIL);
	}
	return(SUCCESS);
}

wtab_t *WKSHConversionWidget;
classtab_t *WKSHConversionClass;
char *WKSHConversionResource;

int
ConvertStringToType(arg0, w, parent, class, resource, val, ret, flatret, flatres, freeit, firstcall)
char *arg0;
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
	char *to_type;
	XtResourceList res;
	XrmValue    fr_val, to_val;
	struct namnod *nam;

	WKSHConversionClass = class;	/* needed by callback converter */
	WKSHConversionResource = resource;	/* needed by callback converter */
	WKSHConversionWidget = w;	/* needed by callback converter */

	if ((nam = nam_search(resource, class->res, 0)) == NULL) {
		/* If we didn't find it in this widget's class record,
		 * see if the parent is a constraint widget class, and
		 * if so then see if we can find the class there.
		 */
		if (parent == NULL || parent->wclass == NULL ||
			parent->wclass->con == NULL ||
			(nam = nam_search(resource, parent->wclass->con, 0)) == NULL) {
			printerr(arg0, usagemsg(
				CONSTCHAR "No such resource for '%s' widget: ", 
				class->cname), resource);
			return(FAIL);
		}
	}
	res = (XtResourceList)nam->value.namval.cp;

	/*
	 * Unfortunately, because String types can be variable in size,
	 * we have to handle this as a special case.
	 */
	if (strcmp(res->resource_type, str_XtRString) == 0) {
		*ret = (XtArgVal)strdup(val);
		*freeit = TRUE;

                /*
                 * Because the toolkit parses geometry directly into
                 * the string you pass in, you can't free it.
                 */
		if (strcmp(resource, "geometry") == 0)
			*freeit = FALSE;
		return(SUCCESS);
	}

	fr_val.size = strlen(val) + 1;
	fr_val.addr = (caddr_t)val;
	to_val.size = 0;
	to_val.addr = NULL;

	/*
	 * Hook to allow the toolkit to do something special
	 * with a resource.  For example, OPEN LOOK has to
	 * do some special things to convert flats.
	 */
	if (toolkit_special_resource(arg0, res, w, parent, 
		class, resource, val, ret, flatret, 
		flatres, freeit, firstcall)) {
		return(SUCCESS);
	}
	XtConvert(
	    w ? w->w : Toplevel,
	    str_XtRString,	/* from type */
	    &fr_val,	/* from value */
	    res->resource_type,	/* to type */
	    &to_val	/* the converted value */
	);
	/*
	 * PORTABILITY NOTE: the following code assumes that
	 * sizeof(int) is equal to either sizeof(short) or
	 * sizeof(long).
	 */
	if (to_val.size && to_val.addr) {
		switch(to_val.size) {
		case sizeof(char):
		    *ret = ((char *)to_val.addr)[0];
		    *freeit = FALSE;
		    break;
		case sizeof(short):
		    *ret = (XtArgVal)((short *)to_val.addr)[0];
		    *freeit = FALSE;
		    break;
		case sizeof(long):
		    *ret = (XtArgVal)((long *)to_val.addr)[0];
		    *freeit = FALSE;
		    break;
		default:
		    /*
		     * There is a possibility that some
		     * coverters will return malloc'ed space and this
		     * is really unnecessary and will leak memory.  About
		     * the only way to handle this is to handle such types as
		     * special cases.  Maybe we need a hash table that
		     * contains the names of types that need the malloc?
		     * The X specs should really have some mechanism for
		     * knowing when to free the results of a conversion.
		     */
		    *ret = (XtArgVal)XtMalloc(to_val.size);
		    memcpy((char *)ret, to_val.addr, to_val.size);
		    *freeit = TRUE;
		}
	} else {
	    printerr(arg0, usagemsg(CONSTCHAR "Failed string conversion to %s", res->resource_type), NULL);
		return(FAIL);
	}
	return(SUCCESS);
}

static int
XtAddInputUsage(arg0)
char *arg0;
{
	printerr(arg0, CONSTCHAR "usage: XtAddInput [-i variable] [ filename | -d descriptor ] [cmd ...]\n", NULL);
	return(1);
}

int
do_XtAddInput(argc, argv)
int argc;
char *argv[];
{
	register int i;
	int fd;
	char *arg0 = argv[0];
	char *cmd;
	char *variable = NULL;
	inputrec_t *inp;
	XtInputId id;

	if (argc < 2) {
		return(XtAddInputUsage());
	}
	if (strncmp(argv[1], "-i", 2) == 0) {
		if (argv[1][2] != '\0') {
			variable = &argv[1][2];
			argv++;
			argc--;
		} else if (argc > 3) {
			variable = argv[3];
			argv += 2;
			argc -= 2;
		} else
			return(XtAddInputUsage(arg0));
	}
	if (strncmp(argv[1], "-d", 2) == 0) {
		if (isdigit(argv[1][2])) {
			fd = atoi(&argv[1][2]);
			argv++;
			argc--;
		} else if (argv[1][2] != '\0' || argc < 3 || 
			!isdigit(argv[2][0])) {
			printerr(arg0, CONSTCHAR "usage: XtAddInput [ filename | -d descriptor ] [cmd ...]\n", NULL);
			return(1);
		} else {
			fd = atoi(argv[2]);
			argv += 2;
			argc -= 2;
		}
	} else {
		if ((fd = open(argv[1], O_RDONLY)) < 0) {
			printerr(arg0, CONSTCHAR "Could not open input source\n", argv[1]);
			return(1);
		}
		argv += 1;
		argc -= 1;
	}
	inp = (inputrec_t *)XtMalloc(sizeof(inputrec_t));
	memset(inp, '\0', sizeof(inputrec_t));
	inp->fd = fd;
	if (argc > 1) {
		inp->cmds = (char **)XtMalloc(sizeof(char *) * argc);
		for (i = 1; i < argc; i++) {
			inp->cmds[i-1] = strdup(argv[i]);
		}
		inp->cmds[i-1] = NULL;
		inp->ncmds = argc - 1;
	} else {
		/*
		 * With no arguments, the default is to just call
		 * "eval" on its args, which only requires a nil command
		 */
		inp->cmds = (char **)XtMalloc(sizeof(char *));
		inp->cmds[0] = (char *)(CONSTCHAR "");
		inp->ncmds = 1;
	}
	id = XtAddInput(fd, (XtPointer)XtInputReadMask, stdInputCB, (caddr_t)inp);
	if (variable != NULL) {
		char buf[256];

		sprintf(buf, "%s=0x%x", variable, id);
		env_set(buf);
	}
	return(0);
}

int
do_XtRemoveInput(argc, argv)
int argc;
char *argv[];
{
	XtInputId id;
	char *p;

	if (argc != 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s inputid", argv[0]), NULL);
		return(1);
	}
	id = strtoul(argv[1], &p, 16);
	if (p == argv[1]) {
		printerrf(argv[0], "Argument must be a hex number");
		return(1);
	}
	XtRemoveInput(id);
	return(0);
}

#ifdef MALDEBUG
int
do_malctl(argc, argv)
int argc;
char *argv[];
{
	if (argc == 2) {
		extern int Mt_trace, Mt_certify;

		Mt_trace = atoi(argv[1]);
		if (Mt_trace >= 0) {
			Mt_certify = 1;
		} else
			Mt_certify = 0;
		return(0);
	} else {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s fd\n", argv[0]), NULL);
		return(1);
	}
}
#endif

void
Translation_ksh_eval(w, event, params, num_params)
Widget w;
XEvent *event;
String *params;
Cardinal *num_params;
{
	register int i;
	char buf[128];
	const char *ename;
	char ex[16], ey[16], erootx[16], erooty[16];

	sprintf(buf, "TRANSLATION_XEVENT=%ld", (long)event);
	env_set(buf);

	*ex = '\0';
	*ey = '\0';
	switch (event->xany.type) {
	case KeyPress:
		ename = CONSTCHAR "KeyPress";
		break;
	case KeyRelease:
		ename = CONSTCHAR "KeyRelease";
		break;
	case ButtonPress:
		ename = CONSTCHAR "ButtonPress";
		sprintf(ex, "%ld", (long)event->xbutton.x);
		sprintf(ey, "%ld", (long)event->xbutton.y);
		sprintf(erootx, "%ld", (long)event->xbutton.x_root);
		sprintf(erooty, "%ld", (long)event->xbutton.y_root);
		break;
	case ButtonRelease:
		ename = CONSTCHAR "ButtonRelease";
		sprintf(ex, "%ld", (long)event->xbutton.x);
		sprintf(ey, "%ld", (long)event->xbutton.y);
		sprintf(erootx, "%ld", (long)event->xbutton.x_root);
		sprintf(erooty, "%ld", (long)event->xbutton.y_root);
		break;
	case MotionNotify:
		ename = CONSTCHAR "MotionNotify";
		sprintf(ex, "%ld", (long)event->xmotion.x);
		sprintf(ey, "%ld", (long)event->xmotion.y);
		sprintf(erootx, "%ld", (long)event->xmotion.x_root);
		sprintf(erooty, "%ld", (long)event->xmotion.y_root);
		break;
	case EnterNotify:
		ename = CONSTCHAR "EnterNotify";
		break;
	case LeaveNotify:
		ename = CONSTCHAR "LeaveNotify";
		break;
	case FocusIn:
		ename = CONSTCHAR "FocusIn";
		break;
	case FocusOut:
		ename = CONSTCHAR "FocusOut";
		break;
	case KeymapNotify:
		ename = CONSTCHAR "KeymapNotify";
		break;
	case Expose:
		ename = CONSTCHAR "Expose";
		break;
	case GraphicsExpose:
		ename = CONSTCHAR "GraphicsExpose";
		break;
	case NoExpose:
		ename = CONSTCHAR "NoExpose";
		break;
	case VisibilityNotify:
		ename = CONSTCHAR "VisibilityNotify";
		break;
	case CreateNotify:
		ename = CONSTCHAR "CreateNotify";
		break;
	case DestroyNotify:
		ename = CONSTCHAR "DestroyNotify";
		break;
	case UnmapNotify:
		ename = CONSTCHAR "UnmapNotify";
		break;
	case MapNotify:
		ename = CONSTCHAR "MapNotify";
		break;
	case MapRequest:
		ename = CONSTCHAR "MapRequest";
		break;
	case ReparentNotify:
		ename = CONSTCHAR "ReparentNotify";
		break;
	case ConfigureNotify:
		ename = CONSTCHAR "ConfigureNotify";
		break;
	case ConfigureRequest:
		ename = CONSTCHAR "ConfigureRequest";
		break;
	case GravityNotify:
		ename = CONSTCHAR "GravityNotify";
		break;
	case ResizeRequest:
		ename = CONSTCHAR "ResizeRequest";
		break;
	case CirculateNotify:
		ename = CONSTCHAR "CirculateNotify";
		break;
	case CirculateRequest:
		ename = CONSTCHAR "CirculateRequest";
		break;
	case PropertyNotify:
		ename = CONSTCHAR "PropertyNotify";
		break;
	case SelectionClear:
		ename = CONSTCHAR "SelectionClear";
		break;
	case SelectionRequest:
		ename = CONSTCHAR "SelectionRequest";
		break;
	case SelectionNotify:
		ename = CONSTCHAR "SelectionNotify";
		break;
	case ColormapNotify:
		ename = CONSTCHAR "ColormapNotify";
		break;
	case ClientMessage:
		ename = CONSTCHAR "ClientMessage";
		break;
	case MappingNotify:
		ename = CONSTCHAR "MappingNotify";
		break;
	default:
		ename = CONSTCHAR "UnknownEvent";
		break;
	}
	env_set_var("TRANSLATION_XEVENT_NAME", ename);
	if (*ex != '\0') {
		env_set_var("TRANSLATION_XEVENT_X", ex);
		env_set_var("TRANSLATION_XEVENT_Y", ey);
		env_set_var("TRANSLATION_XEVENT_X_ROOT", erootx);
		env_set_var("TRANSLATION_XEVENT_Y_ROOT", erooty);
	}

	if (w != NULL) {
		wtab_t *widget_to_wtab();
		wtab_t *wtab = widget_to_wtab(w, NULL);
		env_set_var("TRANSLATION_XEVENT_WIDGET", wtab ? wtab->widid : "Unknown");
	}
	for (i = 0; i < *num_params; i++) {
		ksh_eval(params[i]);
	}
}

/*
 * stdCB() is the central routine from which all callback
 * functions are dispatched (specified by clientData).  The
 * variable "CB_WIDGET" will be placed in the environment to represent 
 * the CallBackWidget handle.  Certain widgets may have also
 * registered functions to be called before and/or after the callback
 * in order to set up environment variables with callData parameters.
 */

void
stdCB(widget, clientData, callData)
void  *widget;
caddr_t clientData, callData;
{
	wtab_t *widget_to_wtab();
	char buf[128];

	wksh_client_data_t *cdata = (wksh_client_data_t *)clientData;

	if (cdata->cbtab != NULL && cdata->cbtab->precb_proc != NULL)
		(cdata->cbtab->precb_proc)(cdata->w, callData);
	/*
	 * The wtab_t entry of the cdata need not be filled in since
	 * it could have been set via direct resource setting at widget
	 * creation time, and the converter for string to callback would
	 * not have had access to this information (since the widget
	 * was not created yet.
	 * Thus, we set it here.  Note that this will happen at most
	 * one time, since we are modifying the cdata structure.
	 */
	if (cdata->w == NULL) {
		cdata->w = widget_to_wtab(widget, NULL);
	}
	env_set_var(CONSTCHAR "CB_WIDGET", cdata->w->widid);
	sprintf(buf, "CB_CALL_DATA=0x%x", callData);
	env_set(buf);

	ksh_eval((char *)cdata->ksh_cmd);
	if (cdata->cbtab != NULL && cdata->cbtab->postcb_proc != NULL)
		(cdata->cbtab->postcb_proc)(cdata->w, callData);
	return;
}

void
stdInputCB(inp, source, id)
inputrec_t *inp;
int *source;
int *id;
{
	char buf[4096];
	char cmdbuf[5120];
	int cmd;
	char *p;
	register int i, n;
	static const char str_stdInputCB[] = "stdInputCB";

	/* try to read some input from the fd */

	if ((n = read(inp->fd, buf, sizeof(buf)-1)) <= 0) {
		/*
		 * no input was available, this is not a problem.
		 */
		return;
	}
	/* go through appending to current line, execute line if you 
	 * get an unquoted newline.
	 */

	for (i = 0; i < n; i++) {
		if (inp->lnend >= sizeof(inp->lnbuf)-5) {
			printerr(str_stdInputCB, CONSTCHAR "Line Overflow! Line flushed\n", NULL);
			inp->lnend = 0;
		}
		if (buf[i] == '\'') {
			inp->lnbuf[inp->lnend++] = '\'';
			inp->lnbuf[inp->lnend++] = '"';
		}
		inp->lnbuf[inp->lnend++] = buf[i];
		if (buf[i] == '\'') {
			inp->lnbuf[inp->lnend++] = '"';
			inp->lnbuf[inp->lnend++] = '\'';
		}
		if (buf[i] == '\n' && (i == 0 || buf[i-1] != '\\')) {
			inp->lnbuf[inp->lnend] = '\0';
			for (cmd = 0; cmd < inp->ncmds; cmd++) {
				sprintf(cmdbuf, "%s '%s'", inp->cmds[cmd], inp->lnbuf);
				ksh_eval(cmdbuf);
			}
			inp->lnend = 0;
		}
	}
}

int
stdWorkProcCB(clientData, id)
char *clientData;
long *id;
{
	int retcode;

	retcode = ksh_eval((char *)clientData);
	if (retcode != 0)
		XtFree(clientData);
	return(retcode);
}

void
stdTimerCB(clientData, id)
char *clientData;
long *id;
{
	ksh_eval((char *)clientData);
	XtFree(clientData);
	return;
}

static void
force()
{
	wk_libinit();
}

static int
VerifyString_usage(arg0)
char *arg0;
{
	printerr(arg0, usagemsg(CONSTCHAR "Usage: %s [-i|-f|-d|-t] [-l low] [-h high] string", arg0), NULL);
	return(255);
}

do_VerifyString(argc, argv)
int argc;
char *argv[];
{
	char *lowrange = NULL, *highrange = NULL;
	char mode;
	register int i;
	int resultint;
	double resultfloat;
	char *arg0 = argv[0];

	if (argc < 3 || argv[1][0] != '-') {
		return(VerifyString_usage(arg0));
	}
	mode = argv[1][1];
	for (i = 2; i < argc && argv[i][0] == '-'; i++) {
		switch (argv[i][1]) {
		case 'l':
			if (argv[i][2])
				lowrange = &argv[i][2];
			else
				lowrange = argv[++i];
			break;
		case 'h':
			if (argv[i][2])
				highrange = &argv[i][2];
			else
				highrange = argv[++i];
		default:
			return(VerifyString_usage(arg0));
		}
	}
	if (argc - i != 1)
		return(VerifyString_usage(arg0));

	switch (mode) {
	case 'i':
		if (strspn(argv[i], CONSTCHAR "-0123456789") != strlen(argv[i])) {
			return(1);
		}
		resultint = atoi(argv[i]);
		if (lowrange) {
			int low = atoi(lowrange);

			if (resultint < low)
				return(2);
		}
		if (highrange) {
			int high = atoi(highrange);

			if (resultint > high)
				return(3);
		}
		return(0);
	case 'f':
	case 'd':
	case 't':
	default:
		return(VerifyString_usage(arg0));
	}
}

do_XFlush(argc, argv)
int argc;
char *argv[];
{
	if (Toplevel == NULL) {
		printerr(argv[0], CONSTCHAR "can't flush: toolkit not initialized.", NULL);
		return(1);
	}
	XSync(XtDisplay(Toplevel), FALSE);
	return(0);
}

static int
XtSetSensitive_usage(arg0)
char *arg0;
{
	printerr(arg0, usagemsg(CONSTCHAR "usage: %s widgetid [true|false]\n", arg0), NULL);
	return(1);
}

int
do_XtSetSensitive(argc, argv)
int argc;
char *argv[];
{
	wtab_t *w;
	Boolean bool;

	if (argc != 3) {
		return(XtSetSensitive_usage(argv[0]));
	}
	bool = (strcmp(argv[2], CONSTCHAR "true") == 0);
	if (!bool && strcmp(argv[2], CONSTCHAR "false") != 0) {
		return(XtSetSensitive_usage(argv[0]));
	}
	w = str_to_wtab(argv[0], argv[1]);
	if (w != NULL) {
		XtSetSensitive(w->w, bool);
	} else 
		return(XtSetSensitive_usage(argv[0]));
	return(0);
}

int
do_crypteval(argc, argv)
int argc;
char *argv[];
{
	register int i;
	FILE *fp;
	unsigned char *evalbuf;
	struct stat sbuf;

	if (argc != 2) {
		printerrf(argv[0], "usage: %s file", argv[0]);
		return(1);
	}
	if ((fp = fopen(argv[1], "r")) == NULL) {
		printerrf(argv[0], "cannot open %s for reading", argv[1]);
		return(1);
	}
	if (fstat(fileno(fp), &sbuf) == -1) {
		printerrf(argv[0], "cannot get file status for %s", argv[1]);
		return(1);
	}
	if (sbuf.st_size == 0) {
		fclose(fp);
		return(0);
	}
	if (!S_ISREG(sbuf.st_mode)) {
		printerrf(argv[0], "%s is not a plain file", argv[1]);
		fclose(fp);
		return(1);
	}
	if ((evalbuf = (unsigned char *)XtMalloc((int)sbuf.st_size)) == NULL) {
		fclose(fp);
		return(1);
	}
	if ((long)fread(evalbuf, 1, sbuf.st_size, fp) < (long)sbuf.st_size) {
		printerrf(argv[0], "read of %d bytes failed", sbuf.st_size);
		fclose(fp);
		free(evalbuf);
		return(1);
	}
	fclose(fp);
	for (i = 0; i < sbuf.st_size; i++) {
		evalbuf[i] ^= 0xaa;
		evalbuf[i] = ((evalbuf[i]>>5)&0x7)|(evalbuf[i]<<3);
	}
	ksh_eval(evalbuf);
	free(evalbuf);
	return(0);
}

int
do_wkcrypt(argc, argv)
int argc;
char *argv[];
{
	register int i, nbytes;
	FILE *fp, *outfp;
	unsigned char buf[BUFSIZ];
	struct stat sbuf;

	if (argc != 3) {
		printerrf(argv[0], "usage: %s infile outfile", argv[0]);
		return(1);
	}
	if ((fp = fopen(argv[1], "r")) == NULL) {
		printerrf(argv[0], "cannot open %s for reading", argv[1]);
		return(1);
	}
	if ((outfp = fopen(argv[2], "w")) == NULL) {
		printerrf(argv[0], "cannot open %s for writing", argv[2]);
		fclose(fp);
		return(1);
	}
	while ((nbytes = fread(buf, 1, BUFSIZ, fp)) > 0) {
		for (i = 0; i < nbytes; i++) {
			buf[i] = ((buf[i])<<5)|((buf[i]>>3)&0x1F);
			buf[i] ^= 0xaa;
		}
		fwrite(buf, 1, nbytes, outfp);
	}
	fclose(fp);
	fclose(outfp);
	return(0);
}

int
do_XtParent(argc, argv)
int argc;
char **argv;
{
   char *arg0 = argv[0];
   char * var;
   char * wname;
   wtab_t *wtab;
   classtab_t *ctab;
   wtab_t *widget_to_wtab();
   
   if (argc < 3 ) {
	printerrf(arg0, "usage: %s variable widget", arg0);
	return(1);
   }
   var = argv[1];
   wname = argv[2];
   wtab = str_to_wtab(arg0, wname);
   if (wtab == NULL) {
	return(1);
   }

   if (wtab->parent == NULL) {
	   wtab = (wtab_t *)widget_to_wtab(XtParent(wtab->w), NULL);
	   /*
	    * If the widget class has no resources registered, then this is
	    * the first known instance of this widget class, so we need to
	    * force the resource list to be loaded.  This can frequently
	    * occur if a Motif convenience function is used, which creates
	    * a 'hidden' parent.
	    */
	   ctab = wtab->wclass;
	   if (ctab->res == NULL)
	      (void)str_to_class(arg0, ctab->cname);
   } else
	wtab = wtab->parent;

   env_set_var(var,  wtab->widid);
   return(0);
}

#include <X11/cursorfont.h>

static int
XDefineCursor_usage(arg0)
char *arg0;
{
	printerrf(arg0, "Usage: XDefineCursor [ -s | -t | -b | glyph-number ] $widget");
	printerrf(arg0, "       -s: standard cursor, -t: target cursor, -b: busy cursor\n");
	return(1);
}

int
do_XDefineCursor(argc, argv)
int argc;
char *argv[];
{
	Widget handle_to_widget();
	Widget w;
	Display *dpy;
	char *arg0 = argv[0];
	Cursor cursor;
	static Cursor busycursor, standcursor, targetcursor;

	if (argc != 3) {
		return(XDefineCursor_usage(arg0));
	}
	if ((w = handle_to_widget(arg0, argv[2])) == NULL)
		return(1);

	if (! XtIsRealized(w)) {
		return(0);
	}

	dpy = XtDisplay(w);
	if (argv[1][0] == '-') {
		/*
		 * We will cache the most common cursor types: busy, stand,
		 * and target, in local static variables.
		 */
		switch (argv[1][1]) {
		case 's':
		    if ((cursor = standcursor) == NULL)
			cursor = standcursor = XCreateFontCursor(dpy, XC_left_ptr);
		    break;
		case 'b':
		    if ((cursor = busycursor) == NULL)
			cursor = busycursor = XCreateFontCursor(dpy, XC_watch);
		    break;
		case 't':
		    if ((cursor = targetcursor) == NULL)
			cursor = targetcursor = XCreateFontCursor(dpy, XC_target);
		    break;
		default:
			return(XDefineCursor_usage(arg0));
		}
	} else {
		int curnum = atoi(argv[1]);

		if (curnum <= XC_num_glyphs)
			cursor = XCreateFontCursor(dpy, curnum);
		else {
			printerrf(arg0, "Cursor out of range: %d\n", curnum);
			return(XDefineCursor_usage(arg0));
		}
	}
	XDefineCursor(dpy, XtWindow(w), cursor);
	XSync(dpy, FALSE);
	return(0);
}
