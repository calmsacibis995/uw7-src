
/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:widget.c	1.7"


#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include "wksh.h"

/* values for the flags field of the W array */

#define RESOURCE_HASHSIZE	64
#define CLASS_HASHSIZE		32
#define WIDGETALLOC		64	/* Will realloc the widget array in this increment */

static wtab_t **W;
static int NumW;
static int MaxW;
int Wtab_free = 0;	/* A count of how many table entries have been freed */

extern struct Amemory *gettree();
extern struct namnod  *nam_search();
static void fixupresources();

extern Widget Toplevel;

/* CONSTANTS */
const char str_no_widget[] =	"Could not find a widget class named:";
extern const char str_0123456789[];
const char str_bad_handle_format[] = 	"Bad handle format: ";
const char str_handle_out_of_range[] = "Handle out of range: ";
const char str_widget_no_exist[] = "Widget does not exist: ";
const char str_widget_id_missing_in_callback[] = "Widget ID not found in callback\n";
const char str_widget_overflow[] = "WIDGET OVERFLOW\n";

extern classtab_t C[];

struct Amemory *Wclasses;

void
init_widgets()
{
	register int i, n = 0;
	struct namnod *nam;
	wtab_t *wentries;

	if (C[0].class != NULL)
		return;

	toolkit_init_widgets();

	Wclasses = gettree(CLASS_HASHSIZE);

	for (i = 0; C[i].cname != NULL; i++) {
		if ((nam = nam_search(C[i].cname, Wclasses, 1)) == NULL) {
			printerr(CONSTCHAR "INTERNAL", CONSTCHAR "Internal hash failure", NULL);
			exit(1);
		}
		nam->value.namval.cp = (char *)(&C[i]);
	}

	/*
	 * The array of widget records starts out big enough to
	 * hold WIDGETALLOC widgets, and will grow in increments
	 * of WIDGETALLOC as it overflows.
	 */
	W = (wtab_t **)XtMalloc(WIDGETALLOC*sizeof(wtab_t *));
	wentries = (wtab_t *)XtCalloc(WIDGETALLOC, sizeof(wtab_t));
	for (i = 0; i < WIDGETALLOC; i++)
		W[i] = &wentries[i];
	MaxW = WIDGETALLOC;
}

/*
 * string to widgetclass
 */

classtab_t *
str_to_class(arg0, s)
char *arg0;
char *s;
{
	register int i, n;
	Widget w;
	struct namnod *nam;
	classtab_t *ret;

	/*
	 * If it looks like a handle, look it up and return the
	 * class of the widget associated with the handle.
	 */
	if (s[0] == 'W' && strspn(&s[1], str_0123456789) == strlen(&s[1])) {
		wtab_t *w = str_to_wtab(arg0, s);
		if (w != NULL)
			return(w->wclass);
	}
	if ((nam = nam_search(s, Wclasses, 0)) != NULL) {
		ret = (classtab_t *)nam->value.namval.cp;
		if (ret->res == NULL) {
			XtResourceList resources;
			Cardinal numresources;
			/* First reference of a given widget class
			 * Automatically causes that widget class to
			 * be initialized, it's resources read and
			 * hashed.
			 */
			ret->res = gettree(RESOURCE_HASHSIZE);
			/*
			 * Have to force the class init
			 * of this widget to execute, else we won't
			 * get a complete list of resources, and any
			 * converters added by this widget won't be
			 * available.
			 */
			XtInitializeWidgetClass(ret->class);

			XtGetResourceList(ret->class, &resources, &numresources);
			for (i = 0; i < numresources; i++) {
				if ((nam = nam_search(resources[i].resource_name, ret->res, 1)) == NULL) {
					printerr("INTERNAL", "Resource hash failed for widget class", ret->cname);
				} else {
					nam->value.namval.cp = (char *)&resources[i];
				}
			}
			fixupresources(s, ret->res, &ret->resfix[0]);
			/*
			 * Get constraint resources, if there are any
			 */
			XtGetConstraintResourceList(ret->class, &resources, &numresources);
			if (resources != NULL) {
				ret->con = gettree(RESOURCE_HASHSIZE);
				for (i = 0; i < numresources; i++) {
					if ((nam = nam_search(resources[i].resource_name, ret->con, 1)) == NULL) {
						printerr("INTERNAL", "Constraint resource hash failed for widget class", ret->cname);
					} else {
						nam->value.namval.cp = (char *)&resources[i];

					}
				}
				fixupresources(s, ret->con, &ret->confix[0]);
			} else {
				ret->con = NULL;
			}
		}
		return(ret);
	}
	printerr(arg0, str_no_widget, s);
	return(NULL);
}

Widget
WkshNameToWidget(s)
String s;
{
	Widget w;
	char *p;
	int len;
	Widget XtNameToWidget();

	if (s == NULL || *s == '\0')
		return(NULL);

	if (W == NULL || W[0] == NULL) {
		printerrf("", "Command cannot be executed: Toolkit is not initialized");
		return(NULL);
	}
	len = strlen(W[0]->wname);
	if (strncmp(s, W[0]->wname, len) == 0) {
		if (s[len] == '\0')
			return(Toplevel);
		if (s[len] == '.')
			return(XtNameToWidget(Toplevel, &s[len+1]));
	}
	return(NULL);
}

/*
 * Take a character string and translate it into a wtab_t.
 * The string should be of the form: W<num>.  The <num> must
 * point to a valid index in the W array.
 *
 * If the name is not of the correct form, we use XtNameToWidget
 * to try to convert the name to a widget id.
 */

wtab_t *
str_to_wtab(arg0, v)
char *arg0;
char *v;
{
	int index, len;

	if (v == NULL || strcmp(v, "NULL") == 0)
		return(NULL);

	if (v[0] != 'W' || (len = strlen(v)) < 2 ||
		strspn(&v[1], str_0123456789) != len-1) {
		Widget wid;
		wtab_t *widget_to_wtab();

		if ((wid = WkshNameToWidget(v)) == NULL) {
			if (arg0)
				printerr(arg0, str_bad_handle_format, v);
			return(NULL);
		}

		return(widget_to_wtab(wid, NULL));
	}
	index = atoi(&v[1]);
	if (index < 0 || index >= NumW) {
		if (arg0)
			printerr(arg0, str_handle_out_of_range, v);
		return(NULL);
	}
	if (W[index]->type == TAB_EMPTY || W[index]->w == NULL) {
		if (arg0)
			printerr(arg0, str_widget_no_exist, v);
		return(NULL);
	}
	return(W[index]);
}

Widget
handle_to_widget(arg0, handle)
char *handle;
{
	wtab_t *w = str_to_wtab(arg0, handle);

	if (w)
		return(w->w);
	else
		return(NULL);
}

/*
 * This function takes a widget and finds the wtab associated with it.
 * This operation is performed infrequently, for example if the user
 * gets a resource that is a widget.  So, we're just using a linear
 * search right now.  If profiling reveals this to be too slow we'll
 * have to introduce another hash table or something.
 */

wtab_t *
widget_to_wtab(w, v)
Widget w;
char *v;
{
	register int i;
	extern wtab_t *set_up_w();

	if (w == NULL)
		return(NULL);
	for (i = 0; i < NumW; i++) {
		if (W[i]->w == w)
			return(W[i]);
	}
	/*
	 * If we failed to find the widget id in the
	 * internal table then this was probably a widget that
	 * was created as a side effect of another widget's creation,
	 * or perhaps was created by a user-defined builtin or something.
	 * So, we'll create a new table entry for it using set_up_w().
	 * Of course, set_up_w() needs to know the widget's parent's
	 * wtab_t, which we get by recursively calling ourself.  Also,
	 * we need the widget's class.
	 */
	{
		wtab_t *pwtab;		/* parent wtab */
		WidgetClass wclass;	/* class record */
		classtab_t *class;	/* widget's class */

		if ((pwtab = widget_to_wtab(XtParent(w), NULL)) == NULL) {
			printerrf(CONSTCHAR "widget_to_wtab", "unable to find parent");
			return(NULL);
		}
		wclass = XtClass(w);
		/*
		 * Again, we have to go linear searching for this
		 * right now.
		 */
		class = NULL;
		for (i = 0; C[i].cname != NULL; i++) {
			if (C[i].class == wclass) {
				class = &C[i];
				break;
			}
		}
		if (class == NULL) {
			printerrf(CONSTCHAR "widget_to_wtab", "unable to find class");
			return(NULL);
		}
		/*
		 * If this class has not been initialized, we
		 * better force it to be set up by calling
		 * str_to_class();
		 */
		if (class->res == NULL)
			(void) str_to_class(CONSTCHAR "widget_to_wtab", class->cname);
		return(set_up_w(w, pwtab, v, NULL, class));
	}
}

void
get_new_wtab(w, name)
wtab_t **w;
char *name;
{
	register int i;

	/*
	 * If there has been a destroywidget call, then one or more
	 * table entries may have been freed.  We might want to make
	 * a free list for this stuff someday, but for now we do a
	 * linear search for the free slot.  Most applications don't
	 * do much widget destroying anyway, so this should rarely
	 * execute and thus I'm not too dismayed by the linear search.
	 */
	i = NumW;
	if (Wtab_free > 0) {
		for (i = 0; i < NumW; i++) {
			if (W[i]->type == TAB_EMPTY) {
				Wtab_free--;
				break;
			}
		}
	}
	if (i == NumW) {
		if (NumW < MaxW) {
			i = NumW++;
		} else {
			register int j;
			int oldmax = MaxW;
			wtab_t *wentries;

			MaxW += WIDGETALLOC;
			W = (wtab_t **)XtRealloc((char *)W, sizeof(wtab_t *)*MaxW);
			wentries = (wtab_t *)XtCalloc(WIDGETALLOC, sizeof(wtab_t));
			for (j = 0; j < WIDGETALLOC; j++)
				W[oldmax+j] = &wentries[j];
			i = NumW++;
		}
	}
	sprintf(name, "W%d", i);
	*w = W[i];
	return;
}

int
set_up_evar(var, widget)
char *var;
Widget widget;
{
	register int i;
	char vareqval[128];

	for (i = 0; i < NumW; i++) {
		if (widget == W[i]->w) {
			sprintf(vareqval, "%s=W%d", var, i);
			env_set(vareqval);
			return(0);
		}
	}
	printerr(CONSTCHAR "CallBackHandler", str_widget_id_missing_in_callback, NULL);
	return(1);
}

static void
fixupresources(name, res, fixups)
char *name;		/* the class name, used only for error printing */
struct Amemory *res;	/* the hash table of resources for this class */
resfixup_t *fixups;	/* the list of fixups to be made to the resources */
{
	XtResource *resource;
	register int i;
	struct namnod *nam;

	if (fixups == NULL)
		return;

	for (i = 0; fixups[i].name != NULL; i++) {
		if ((nam = nam_search(fixups[i].name, res, 1)) == NULL) {
			printerr("INTERNAL", "Resource fixup hash failed for widget class", name);
		} else {
			resource = (XtResource *)nam->value.namval.cp;

			/*
			 * We could be either adding a new resource or
			 * modifying an old one.
			 */
			if (resource == NULL)
				resource = (XtResource *)XtMalloc(sizeof(XtResource));
			/*
			 * The only fields wksh uses are the name, type and
			 * size, so that's all we attempt to fix up.
			 */
			resource->resource_name = (String)fixups[i].name;
			resource->resource_type = (String)fixups[i].type;
			resource->resource_size = fixups[i].size;
			nam->value.namval.cp = (char *)resource;
		}
	}
}

int
do_widload(argc, argv)
int argc;
char *argv[];
{
	register int i, len;
	char classrec[BUFSIZ];
	classtab_t *classtab;
	void *address;
	void *fsym();
	struct namnod *nam;
	int gadgetflag = 0;

	init_widgets();

	if (argc < 2) {
		printerr(argv[0], usagemsg(CONSTCHAR "usage: %s widgetname ...\n", argv[0]), NULL);
		return(1);
	}

	for (i = 1; i < argc; i++) {
		if ((len = strlen(argv[i])) > 6 &&
			strcmp(argv[i]+len-6, CONSTCHAR "Gadget") == 0)
			gadgetflag = 1;
		else
			gadgetflag = 0;
		if (gadgetflag)
			sprintf(classrec, CONSTCHAR "%sClass", argv[i]);
		else
			sprintf(classrec, CONSTCHAR "%sWidgetClass", argv[i]);
		if ((address = fsym(classrec, -1)) == NULL) {
			printerr(argv[0], usagemsg(CONSTCHAR "could not find class record called: %s ", classrec), NULL);
		} else {
			classtab = (classtab_t *)XtMalloc(sizeof(classtab_t));
			memset(classtab, '\0', sizeof(classtab_t));
			classtab->cname = (char *)strdup(argv[i]);
			classtab->class = ((WidgetClass *)address)[0];
			if ((nam = nam_search(classtab->cname, Wclasses, 1)) == NULL) {
				printerr(CONSTCHAR "INTERNAL", CONSTCHAR "Internal hash failure", NULL);
				return(1);
			}
			nam->value.namval.cp = (char *)(classtab);
		}
	}
	return(0);
}

static XtResource *Res[128];
static int Nres;

static int
rescompare(r1, r2)
XtResource **r1, **r2; 
{
	return(strcmp(r1[0]->resource_name, r2[0]->resource_name));
}

static void
_pr_class(c)
classtab_t *c;
{
	altprintf("%s\n", c->cname);
}

static void
_pr_resource_list(res)
XtResource *res;
{
	altprintf("\t%-24.24s %-24.24s %s\n", res->resource_name, res->resource_class, res->resource_type);
}

static void
sort_and_print_res()
{
	register int i;
	qsort(Res, Nres, sizeof(XtResource *), rescompare);
	for (i = 0; i < Nres; i++) {
		_pr_resource_list(Res[i]);
	}
}

static void
gather_resource_list(r)
struct namnod *r;
{
        XtResource *res = (XtResource *)r->value.namval.cp;

	Res[Nres++] = res;
}

static int Show_constraint;

static void
_pr_resource(c, w)
classtab_t *c;
wtab_t *w;
{
	if (c->res == NULL)
		(void)str_to_class(CONSTCHAR "widlist", c->cname);	/* force initialization */

	if (Show_constraint && c->con == NULL)	/* No constraint resources */
		return;

	altprintf(CONSTCHAR "\n%sRESOURCES FOR %s%s%s:\n", 
		Show_constraint ? CONSTCHAR "CONSTRAINT " : "", c->cname,
		w ? " " : "", w ? w->widid : "");

	Nres = 0;

	scan_all(gather_resource_list, Show_constraint ? c->con : c->res);
	if (!Show_constraint && w && w->parent != NULL && 
		XtIsConstraint(w->parent->w)) {
		scan_all(gather_resource_list, w->parent->wclass->con);
	}

	sort_and_print_res();
}

static void
pr_resource(r)
struct namnod *r;
{
	classtab_t *c = (classtab_t *)r->value.namval.cp;

	_pr_resource(c, NULL);
}

static void
pr_class(c)
struct namnod *c;
{
	classtab_t *class = (classtab_t *)c->value.namval.cp;

	_pr_class(class);
}

static char *
getname(w, buf, max)
wtab_t *w;
char *buf;
int max;
{
	char *p;
	int len;

	/* calculate a widget's name.  Goes backwards through the
	 * list of parents, filling in the names backwards in the
	 * buffer, then returns a pointer to the start of the name
	 */
	p = &buf[max];	/* that's right, buf[max] not buf[max-1] */
	for ( ; w; w = w->parent) {
		if (p - (len = strlen(w->wname)) < buf+3) {	/* overflow! */
			p--;
			*p = '*';
			return(p);
		}
		p -= len+1;
		strcpy(p, w->wname);
		if (p + len != buf + max - 1)
			p[len] = '.';
	}
	return(p);
}

static void
pr_widinfo(w)
wtab_t *w;
{
	char namebuf[256];
	char *name;
	char statbuf[8];

	name = getname(w, namebuf, sizeof(namebuf));
	sprintf(statbuf, "%s%s%s",
		XtIsRealized(w->w) ? "R" : "",
		XtIsManaged(w->w) ? "M" : "",
		XtIsSensitive(w->w) ? "S" : "");

	altprintf("%-15s %-6s %-6s %-18s %-6s %s\n", 
		w->envar,
		w->widid, 
		w->parent ? w->parent->widid : "none", 
		w->wclass->cname,
		statbuf,
		name);
}

static void
pr_widheader()
{
	altprintf("ENV VARIABLE    HANDLE PARENT CLASS              STATUS NAME\n");
}

/*
 * widlist -r [widget|class]	print resources and their types for widgets
 * widlist -R [widget|class]	print constraint resources for widgets
 * widlist -c [class]		print info about a class
 * widlist -h			print widget handles
 * widlist			print summary info about all widgets that live
 * widlist -C [widget]		print all the children handles of widget
 */

static const char str_widlist_usage[] = "usage: widlist [-rch] [handle|class]...";

int
do_widlist(argc, argv)
int argc;
char *argv[];
{
	register int i, j;
	char buf[BUFSIZ];
	wtab_t *w;
	classtab_t *c;
	int errs = 0;

	if (C[0].class == NULL) {
		printerr(argv[0], CONSTCHAR "can't list widgets: toolkit not initialized", NULL);
		return(1);
	}
	if (argc == 1 || argv[1][0] != '-') {
		/* Print long listing of each widget */
		pr_widheader();
		if (argc == 1) {
			for (i = 0; i < NumW; i++) {
				if (W[i]->type == TAB_EMPTY)
					continue;
				pr_widinfo(W[i]);
			}
		} else {
			for (i = 1; i < argc; i++) {
				if ((w = str_to_wtab(argv[0], argv[i])) != NULL)
					pr_widinfo(w);
			}
		}
	} else if (argv[1][0] == '-') {
		if ((Show_constraint = strcmp(argv[1], CONSTCHAR "-r")) == 0 || 
			strcmp(argv[1], CONSTCHAR "-R") == 0) {
			/* print all the resources in each widget or class */
			if (argc == 2) {
				return(0);
/*
				gscan_all(pr_resource, Wclasses);
*/
			} else {
				for (i = 2; i < argc; i++) {
					if ((c = str_to_class(argv[0], argv[i])) != NULL) {
						if (Show_constraint && c->con == NULL)
							return(0);

						if (!Show_constraint && c-> res == NULL)
							return(0);

						w = str_to_wtab(NULL, argv[i]);
						_pr_resource(c, w);
					}
				}
			}
			return(0);
		} else if (strcmp(argv[1], CONSTCHAR "-c") == 0) {
			/*
			 * print all the available classes, or check if a
			 * class is available
			 */
			if (argc == 2) {
				gscan_all(pr_class, Wclasses);
			} else {
				for (i = 2; i < argc; i++) {
					if ((c = str_to_class(argv[0], argv[i])) != NULL)
						_pr_class(c);
				}
			}
			return(0);
		} else if (strcmp(argv[1], CONSTCHAR "-h") == 0) {
			/* print active widget handles */
			if (argc == 2) {
				for (i = 0; i < NumW; i++) {
					if (W[i]->type == TAB_EMPTY)
						continue;
					altprintf("%s\n", W[i]->widid);
				}
			} else {
				for (i = 2; i < argc; i++) {
					if ((w = str_to_wtab(argv[0], argv[i])) == NULL) {
						errs++;
						continue;
					}
					altprintf(CONSTCHAR "%s\n", w->wname);
				}
			}
		} else {
			printerr(argv[0], CONSTCHAR "Bad option: Usage: ", str_widlist_usage);
			return(255);
		}
	} else {
		printerr(argv[0], str_widlist_usage, NULL);
		return(255);
	}
	return(errs);
}

