#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:internet/util.c	1.4"
#endif

#include <stdio.h>
#include <sys/types.h>

#include "lookup.h"
#include "lookupG.h"
#include "util.h"
#include "inet.h"
#include "inetMsg.h"

#include <Dt/Desktop.h>

void
DisplayHelp(Widget widget, HelpText * help)
{
	DtRequest	request;

	memset(&request, 0, sizeof(request));
	request.display_help.rqtype = DT_DISPLAY_HELP;
	if (help) {
		request.display_help.source_type =
			help->section ? DT_SECTION_HELP : DT_TOC_HELP;
		request.display_help.app_name = mygettxt(TXT_appName);
		request.display_help.app_title = mygettxt(TXT_appName);
		request.display_help.title = mygettxt(TXT_appName);
		request.display_help.help_dir = NULL;
		request.display_help.file_name = "dtadmin/inet.hlp";
		request.display_help.sect_tag = help->section;
	} else {
		request.display_help.source_type = DT_OPEN_HELPDESK;
	}
	(void)DtEnqueueRequest(
		XtScreen(hi.net.common.toplevel),
		_HELP_QUEUE(XtDisplay(hi.net.common.toplevel)),
		XInternAtom(
			XtDisplay(hi.net.common.toplevel),
			"_DT_QUEUE",
			False
		),
		XtWindow(hi.net.common.toplevel),
		&request
	);
}

void
helpkeyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	DisplayHelp(w, (HelpText *)client_data);
}

void
displayFooterMsgEH(Widget w, MiniHelp_t * mh, XFocusChangeEvent * event)
{
	XmString	xms;

	if (event->type == FocusIn) {
		xms = XmStringCreateLocalized((String)mygettxt(mh->msg));
	} else {
		xms = XmStringCreateLocalized(" ");
	}
	XtVaSetValues(mh->widget, XmNlabelString, xms, NULL);
	XmStringFree(xms);
}

void
noSpacesCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmTextVerifyPtr	cbs = (XmTextVerifyPtr)call_data;

	if (isspace(*cbs->text->ptr)) {
		cbs->doit = False;
	}

	return;
}
