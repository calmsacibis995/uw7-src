#ifndef NOIDENT
#pragma ident	"@(#)dtnetlib:rac.c	1.9"
#endif


#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <Xm/RowColumn.h>
#include <Xm/MwmUtil.h>
#include "lookup.h"
#include "lookupG.h"
#include "util.h"
#include "inet.h"

#include <Dt/Desktop.h>

static	Widget statusWid;

void
racDisplayHelp(Widget widget, HelpText * help)
{

	DtRequest	request;

	memset(&request, 0, sizeof(request));
	request.display_help.rqtype = DT_DISPLAY_HELP;
	if (help) {
		request.display_help.source_type =
			help->section ? DT_SECTION_HELP : DT_TOC_HELP;
		request.display_help.app_name = "Remote Login";
		request.display_help.app_title = "Remote Login";
		request.display_help.title = "Remote Login";
		request.display_help.help_dir = NULL;
		request.display_help.file_name = "Remote_Access/Remote_Access.hlp";
		request.display_help.sect_tag = help->section;
	} else {
		request.display_help.source_type = DT_OPEN_HELPDESK;
	}
	(void)DtEnqueueRequest(
		XtScreen(widget),
		_HELP_QUEUE(XtDisplay(widget)),
		XInternAtom(
			XtDisplay(widget),
			"_DT_QUEUE",
			False
		),
		XtWindow(widget),
		&request
	);
}

checkHostName(char *name)
{
	struct	hostent	*hostptr;
	struct	in_addr	*ptr;
		if ((hostptr = gethostbyname(name)) != NULL) {
			return(1);
		}
		else {
			return(0);
		}
}

r_decor(Widget	top)
{
	int	decor;
	
	decor = MWM_FUNC_MOVE | MWM_FUNC_CLOSE;
	XtVaSetValues(top, XmNmwmFunctions, decor, NULL);
	return(0);
}

int
prop_help(Widget wid)
{
static HelpText	phelp;

	phelp.section = strdup("60");
	
	racDisplayHelp(wid, &phelp);
	return(0);
}

int
access_help(Widget wid)
{
static HelpText ahelp;

	ahelp.section = strdup("30");

	racDisplayHelp(wid, &ahelp);
	return(0);	
}

int
copy_help(Widget wid)
{
static HelpText chelp;

	chelp.section = strdup("70");

	racDisplayHelp(wid, &chelp);
	return(0);	
}

void
handleFocus(Widget w, XtPointer msg, XFocusChangeEvent *event)
{
	XmString tmp;

	if (event->type == FocusIn) 
		tmp = XmStringCreateLocalized(msg);
	else 
		tmp = XmStringCreateLocalized("  ");

	XtVaSetValues(statusWid, XmNlabelString, tmp, NULL);
	XmStringFree(tmp);
}

void
eventCB(Widget wid, Widget status, char *msg)
{
	statusWid = status;
	XtAddEventHandler(wid, FocusChangeMask, False,
		(void(*)())handleFocus, strdup(msg));
}

int
getlen(Widget wid)
{
	XtWidgetGeometry	size;
	size.request_mode = CWWidth;
	XtQueryGeometry(wid, NULL, &size);
	return(size.width);	
}

setFocus(Widget wid, int iterations)
{
	int	i;
	/*altprintf("iterations is %d\n", iterations); */
	for (i=0; i < iterations; i++) {
		XmProcessTraversal(wid, XmTRAVERSE_NEXT_TAB_GROUP);
	} 
	return(0);
}
