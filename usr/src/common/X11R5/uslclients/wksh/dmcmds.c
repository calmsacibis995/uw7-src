/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:dmcmds.c	1.2.1.1"

#include <signal.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>

#include <Xm/Xm.h>
#include <Xm/FontObj.h>

#include <FIconBox.h>

#include "Dt/Desktop.h"
#include "Dtm.h"
#include "DtI.h"
#include "dm_strings.h"
#include "extern.h"

#include "wksh.h"

/*
 * Usage: DmCreateIconContainer [options] VAR $PARENT \
 * 		label pixmap x y ...
 *
 * OPTIONS:
 * 	-p defaultpixmap	Set the default pixmap for those specified
 *				by empty strings.
 */

extern Widget Toplevel;
extern char *env_get();

DmCreateIconContainerUsage(arg0)
char *arg0;
{
	fprintf(stderr, "Usage: %s [options] VAR $PARENT label icon x y ...\n", arg0);
	return(1);
}

typedef struct {
	char *selectcmd;
	char *dblselectcmd;
} DmInfo;

/*
 * This is the central routine from which all callback
 * functions are dispatched (specified by clientData).
 */

void
DmDblSelectCB(widget, clientData, cd)
void  *widget;
caddr_t clientData;
ExmFIconBoxButtonCD *cd;
{
	wtab_t *widget_to_wtab();
	wtab_t *w;
        char envbuf[1024];
        DmInfo *info;

        w = widget_to_wtab(widget, NULL);
        info = (DmInfo *) w->info;
        if (info == NULL)
                return;

        env_set_var(CONSTCHAR "CB_WIDGET", w->widid);
        sprintf(envbuf, "CALL_DATA_INDEX=%d", cd->item_data.item_index);
        env_set(envbuf);

	ksh_eval(info->dblselectcmd);
        return;
}


void
DmSelectCB(widget, clientData, cd)
void  *widget;
caddr_t clientData;
ExmFIconBoxButtonCD *cd;
{
	wtab_t *widget_to_wtab();
	wtab_t *w;
        char envbuf[1024];
        DmInfo *info;

        w = widget_to_wtab(widget, NULL);
        info = (DmInfo *) w->info;
        if (info == NULL)
                return;

        env_set_var(CONSTCHAR "CB_WIDGET", w->widid);
        sprintf(envbuf, "CALL_DATA_INDEX=%d", cd->item_data.item_index);
        env_set(envbuf);

	ksh_eval(info->selectcmd);
        return;
}

do_DmCreateIconContainer(argc, argv)
int argc;
char *argv[];
{
	Widget ret, handle_to_widget();
	char *selectcmd = NULL, *dblselectcmd = NULL;
	wtab_t *parent, *wtab;
	DmInfo *info;
	char *var;
	DtAttrs attrs;
	Arg args[10];
	int nargs = 0, numargs = 0;
	Cardinal num_args;
	DmObjectPtr objp, optr, nextptr;
	Cardinal num_objs;
	DmItemPtr retitems;
	Cardinal num_items;
	Widget swin = NULL;
	DmGlyphPtr defaultglyph = NULL;
	DmContainerRec container;
	char *arg0 = argv[0];

	while (argv[1][0] == '-') {
		switch (argv[1][1]) {
		case 'p':
			/*defaultglyph = DmGetPixmap(XtScreen(Toplevel), argv[2]); */
			defaultglyph = DmGetPixmap(XtScreen(Toplevel), argv[2]);
			argv += 2;
			argc -= 2;
			break;
		case 's':
			selectcmd = argv[2];
			argv += 2;
			argc -= 2;
			break;
		case 'd':
			dblselectcmd = argv[2];
			argv += 2;
			argc -= 2;
			break;
		default:
			return(DmCreateIconContainerUsage(arg0));
			
		}
	}

	if (argc < 3) {
		return(DmCreateIconContainerUsage(arg0));
	}

	var = argv[1];
	parent = str_to_wtab(arg0, argv[2]);
	if (parent == NULL) {
		return(1);
	}
	argv += 3;
	argc -= 3;

	container.op = optr;
	container.num_objs = 0;

	while (argc >= 4) {
		optr = (DmObjectPtr)calloc(1, sizeof(DmObjectRec));
		optr->container = &container;

		optr->name = argv[0];
		optr->fcp = (DmFclassPtr)calloc(1, sizeof(DmFclassRec));
		if (argv[1][0] == '\0' || argv[1][0] == '-') {
			optr->fcp->glyph = defaultglyph;
		} else {
			optr->fcp->glyph = DmGetPixmap(XtScreen(Toplevel), argv[1]);
		}
		optr->x = atoi(argv[2]);
		optr->y = atoi(argv[3]);

		if (container.num_objs++ == 0) {
			container.op = optr;
			nextptr = optr;
		} else {
			nextptr->next = optr;
			nextptr = optr;
		}

		argv += 4;
		argc -= 4;
	}
	numargs=0;
	XtSetArg(args[numargs], XmNexclusives,   (XtArgVal)TRUE); numargs++;
	XtSetArg(args[numargs], XmNmovableIcons, (XtArgVal)FALSE); numargs++;
	XtSetArg(args[numargs], XmNminWidth,     (XtArgVal)1); numargs++;
	XtSetArg(args[numargs], XmNminHeight,    (XtArgVal)1); numargs++;
	XtSetArg(args[numargs], XmNdrawProc,     (XtArgVal)DmDrawIcon); numargs++;
	XtSetArg(args[numargs], XmNdblSelectProc, (XtArgVal)DmDblSelectCB); numargs++;
	XtSetArg(args[numargs], XmNselectProc, (XtArgVal)DmSelectCB); numargs++;

	ret = DmCreateIconContainer(parent->w, DM_B_CALC_SIZE, args, numargs,
		container.op, container.num_objs,
		&retitems, container.num_objs,
		NULL);

	wtab = (wtab_t *)set_up_w(ret, parent, var, XtName(ret), str_to_class(arg0, "flatIconBox"));
	info = (DmInfo *)malloc((sizeof(DmInfo)));
	wtab->info = (XtPointer)info;
	info->selectcmd = selectcmd ? strdup(selectcmd) : NULL;
	info->dblselectcmd = dblselectcmd ? strdup(dblselectcmd) : NULL;
	return(0);
}

static void
displayhelp(Widget w, XtPointer client_data, XtPointer call_data)
{
	DtDisplayHelpRequest *req = (DtDisplayHelpRequest *)client_data;
	Display	*display;
	Window	win;

	/*
	 * Find out display and window from widget
	 */
	display = XtDisplay(w);
	win = XtWindow(w); 

	req->rqtype = DT_DISPLAY_HELP;
	req->serial = 0;
	req->client = win;
	req->nodename = NULL;

	/*
	 * Display help
	 */
	(void)DtEnqueueRequest(XtScreen(w), _HELP_QUEUE(display),
 	    _HELP_QUEUE(display), win, (DtRequest *)req); 
}

static void
rh_freereq(DtDisplayHelpRequest *req)
{
	if (req->app_name != NULL)
		free(req->app_name);
	if (req->title != NULL)
		free(req->title);
	if (req->help_dir != NULL)
		free(req->help_dir);
	if (req->string != NULL)
		free(req->string);
	if (req->file_name != NULL)
		free(req->file_name);
	if (req->sect_tag != NULL)
		free(req->sect_tag);
	XtFree((char *)req);
}

static void
rh_destroy(Widget w, XtPointer client_data, XtPointer call_data)
{
	rh_freereq((DtDisplayHelpRequest *)client_data);
}

do_RegisterHelp (int argc, char **argv)
{

	DtDisplayHelpRequest *req = (DtDisplayHelpRequest *)XtMalloc(sizeof(DtDisplayHelpRequest));
	Widget	widget = NULL, handle_to_widget();
	int	badargs = 0;
	char	*p = NULL;

	if (req == NULL)
		return(1);

	req->source_type = DT_OPEN_HELPDESK;	/* default request */
	req->app_title = NULL;
	req->title = NULL;
	req->icon_file = NULL;
	req->string = NULL;
	req->file_name = NULL;
	req->sect_tag = NULL;

	/*
	 * hardly a "standard" way to process arguments, but the expected
	 * arguments are also a little off the wall (for handling w/ getopt(),
	 * anyway)...
	 */

	switch (argc) {
	case 5: if (strcmp(argv[2], "-s") != 0) {
			badargs++;
			break;
		}
		req->title = strdup(argv[4]);
		/* fall through */
	case 4:
	case 3: if (strcmp(argv[2], "-s") == 0) {
			if (argc == 3) {	/* -s but no string? */
				badargs++;
				break;
			}
			req->source_type = DT_STRING_HELP;
			req->string = strdup(argv[3]);
		} else {
			req->source_type = DT_SECTION_HELP;
			req->file_name = strdup(argv[2]);
			req->sect_tag = strdup(argv[3]);
		}
		/* fall through */
	case 2: if ((widget = handle_to_widget(argv[0], argv[1])) != NULL) {
			req->app_name = (p = env_get("APPNAME")) ? strdup(p) : NULL;
			req->help_dir = (p = env_get("HELPDIR")) ? strdup(p) : NULL;
			XtAddCallback(widget, XmNhelpCallback, displayhelp, (XtPointer)req);
			XtAddCallback(widget, XtNdestroyCallback, rh_destroy, (XtPointer)req);
			return(0);
		}
		break;
	default:
		badargs++;
		break;
	}


	/*
	 * if we get here, something went wrong, so let's clean it all up
	 */
	if (badargs)
		fprintf(stderr, "Usage: %s Handle [ filename [section] | -s string [title] ]\n", argv[0]);

	rh_freereq(req);
	return(1);
}
