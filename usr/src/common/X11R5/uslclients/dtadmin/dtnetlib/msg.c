#ifndef NOIDENT
#pragma ident	"@(#)dtnetlib:msg.c	1.8"
#endif

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <Xm/DialogS.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/CascadeBG.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/PanedW.h>
#include <Xm/ScrolledW.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/TextF.h>
#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
#include <Xm/MessageB.h>

#include "lookup.h"
#include "lookupG.h"
#include "lookupMsg.h"
#include "util.h"
#include "inet.h"


extern XmString	createString(char *);
extern XmString i18nString(char *);
extern void	freeString(XmString);
extern char *	mygettxt(char *);

void	infoOkCB();
void	infoCancelCB();
void	infoMsg(Widget, char *);
void	createMsg(Widget, msgType, char *, char *);

void
exitCB(Widget w, XtPointer client_data, XmAnyCallbackStruct *cbs)
{
	exit(0);
}

void
msgOkCB(Widget w, XtPointer client_data, XmAnyCallbackStruct *cbs)
{
	XmString	string;
	char		*tmpname;

/*
	switch(hi.net.common.msgType) {
		default:
			printf("unkonwn type\n");
			break;
	}
*/
}

void
msgCancelCB(Widget w, XtPointer client_data, XmAnyCallbackStruct *cbs)
{
	XmString	string;
	XtUnmanageChild(w);
}

void
msgHelpCB(Widget w, XtPointer client_data, XmAnyCallbackStruct *cbs)
{
	createMsg(w, INFO, INFO_noHelp, NULL);
}

void
createMsg(Widget parent, msgType type, char *msg, char *title)
{
	static Widget	msgDialog;
	char		buf[MEDIUMBUFSIZE];
	XmString	text;
	Arg		args[10];
	int		i;

	if (!msgDialog) {
		msgDialog = XmCreateMessageDialog(parent,
			"infoMsg", NULL, 0);
		if (title == NULL) {
		XtVaSetValues(XtParent(msgDialog),
			XmNtitle, mygettxt(TXT_INTERNET), NULL);
		} else { 
		XtVaSetValues(XtParent(msgDialog),
			XmNtitle, title , NULL);
		}
		XtAddCallback(msgDialog, XmNokCallback,
			(void(*)())msgOkCB, NULL);
		/*
		XtAddCallback(msgDialog, XmNcancelCallback,
			(void(*)())msgCancelCB, NULL);
		XtAddCallback(msgDialog, XmNhelpCallback,
			(void(*)())msgHelpCB, NULL);
		*/
		XtUnmanageChild(
			XmMessageBoxGetChild(
				msgDialog,
				XmDIALOG_CANCEL_BUTTON
			)
		);
		XtUnmanageChild(
			XmMessageBoxGetChild(
				msgDialog,
				XmDIALOG_HELP_BUTTON
			)
		);
	}
	switch (type) {
		case INFO:
			XtVaSetValues(msgDialog, XmNdialogType, XmDIALOG_INFORMATION, NULL);
			break;
		case ERROR:
			XtVaSetValues(msgDialog, XmNdialogType, XmDIALOG_ERROR, NULL);
			break;
		case WARN:
			XtVaSetValues(msgDialog, XmNdialogType, XmDIALOG_WARNING, NULL);
			break;
	}
	/* strcpy(buf, mygettxt(msg)); */
	text = XmStringCreateLtoR(msg, "charset");
	XtVaSetValues(msgDialog,
		XmNmessageString, text,
		NULL);
	XmStringFree(text);
	text = XmStringCreateLtoR(title, "charset");
	XtVaSetValues(msgDialog, XmNdialogTitle, text, NULL);
	XmStringFree(text);
	/*XtVaSetValues(parent, XtNtitle, title, NULL);*/
	XtManageChild(msgDialog);
}

void
createExitMsg(Widget parent, msgType type, char *msg, char *title)
{
	static Widget	msgDialog;
	char		buf[MEDIUMBUFSIZE];
	XmString	text;
	Arg		args[10];
	int		i;

	if (!msgDialog) {
		msgDialog = XmCreateMessageDialog(parent,
			"infoMsg", NULL, 0);
		if (title == NULL) {
			XtVaSetValues(XtParent(msgDialog),
				XmNtitle, mygettxt(TXT_INTERNET), NULL);
		} else { 
			XtVaSetValues(XtParent(msgDialog),
				XmNtitle, title , NULL);
		}
		XtAddCallback(msgDialog, XmNokCallback,
			(void(*)())exitCB, NULL);
		XtAddCallback(msgDialog, XmNcancelCallback,
			(void(*)())msgCancelCB, NULL);
		XtAddCallback(msgDialog, XmNhelpCallback,
			(void(*)())msgHelpCB, NULL);
	}
	switch (type) {
		case INFO:
			XtVaSetValues(msgDialog, XmNdialogType, XmDIALOG_INFORMATION, NULL);
			break;
		case ERROR:
			XtVaSetValues(msgDialog, XmNdialogType, XmDIALOG_ERROR, NULL);
			break;
		case WARN:
			XtVaSetValues(msgDialog, XmNdialogType, XmDIALOG_WARNING, NULL);
			break;
	}
	strcpy(buf, mygettxt(msg));
	text = XmStringCreateLtoR(buf, "charset");
	XtVaSetValues(msgDialog,
		XmNmessageString, text,
		NULL);
	XmStringFree(text);
	XtManageChild(msgDialog);
}

void
createCommonMsg(Widget parent, Widget *mb, msgType type, char *msg, char *title)
{
	char		buf[MEDIUMBUFSIZE];
	XmString	text;
	Arg		args[10];
	int		i;

	if (!*mb) {
		*mb = XmCreateMessageDialog(parent,
			"infoMsg", NULL, 0);
		if (title == NULL) {
			XtVaSetValues(XtParent(*mb),
				XmNtitle, mygettxt(TXT_INTERNET), NULL);
		} else { 
			XtVaSetValues(XtParent(*mb),
				XmNtitle, title , NULL);
		}
		/*
		XtVaSetValues(XtParent(*mb),
			XmNtitle, mygettxt(TXT_error), NULL);
		XtAddCallback(*mb, XmNcancelCallback,
			(void(*)())msgCancelCB, NULL);
		XtAddCallback(*mb, XmNhelpCallback,
			(void(*)())msgHelpCB, NULL);
		*/
		XtAddCallback(*mb, XmNokCallback,
			(void(*)())msgOkCB, NULL);
		XtUnmanageChild(
			XmMessageBoxGetChild(
				*mb,
				XmDIALOG_CANCEL_BUTTON
			)
		);
		XtUnmanageChild(
			XmMessageBoxGetChild(
				*mb,
				XmDIALOG_HELP_BUTTON
			)
		);
	}
	switch (type) {
		case INFO:
			XtVaSetValues(*mb, XmNdialogType, XmDIALOG_INFORMATION, NULL);
			break;
		case ERROR:
			XtVaSetValues(*mb, XmNdialogType, XmDIALOG_ERROR, NULL);
			break;
		case WARN:
			XtVaSetValues(*mb, XmNdialogType, XmDIALOG_WARNING, NULL);
			break;
	}
	strcpy(buf, mygettxt(msg));
	text = XmStringCreateLtoR(buf, "charset");
	XtVaSetValues(*mb,
		XmNmessageString, text,
		NULL);
	XmStringFree(text);
	XtManageChild(*mb);
}
