#ident	"@(#)debugger:libmotif/common/Alert_sh.C	1.5"

// GUI headers
#include "Alert_sh.h"
#include "UI.h"
#include "gui_label.h"
#include "Label.h"

// Debug headers
#include "List.h"

#include <Xm/MessageB.h>
#include <Xm/Text.h>

#define DEBUG_A(x) 	Debug_t(DBG_A,x)

static List	notice_list;
List		fatal_list;

Alert_shell	*old_alert_shell;
Alert_shell	*current_notice;

// 
static void
alert_CB(Widget, Alert_shell *ptr, XmAnyCallbackStruct *cbs)
{
	if (!ptr)
		return;

	Callback_ptr	hdlr = ptr->get_handler();
	Command_sender	*obj = ptr->get_object();

	if (!hdlr || !obj || !cbs)
		return;
	switch(cbs->reason)
	{
	case XmCR_CANCEL:
		(obj->*hdlr)(ptr, (void *)0);
		break;
	case XmCR_OK:
		(obj->*hdlr)(ptr, (void *)1);
		break;
	default:
		return;
	}
}

static void
alert_popdown_CB(Widget, Alert_shell *ptr, XtPointer)
{
	if (!ptr)
		return;
	old_alert_shell = ptr;
	current_notice = (Alert_shell *)notice_list.first();
	if (current_notice)
	{
		notice_list.remove(current_notice);
		XtManageChild(current_notice->get_widget());
	}
	if (ptr->is_fatal())
		fatal_list.remove(ptr);
}

Alert_shell::Alert_shell(const char *string,  Alert_type atype,
	LabelId action, LabelId no_action,
	Callback_ptr h, void *obj) : Component(0, "notice", 0, HELP_none)
{
	Arg	args[3];
	int	n;
	
	fatal_error = FALSE;
	urgent_msg = FALSE;

	XmString msg = XmStringCreateLtoR((String)string,XmSTRING_DEFAULT_CHARSET);
	XmString ok_label = XmStringCreateLocalized((String)labeltab.get_label(action));
	XmString cancel_label = 0;
	XtSetArg(args[0], XmNmessageString, msg);
	XtSetArg(args[1], XmNokLabelString, ok_label);
	n = 2;
	if (no_action != LAB_none)
	{
		cancel_label = XmStringCreateLocalized((String)labeltab.get_label(no_action));
		XtSetArg(args[2], XmNcancelLabelString, cancel_label);
		n = 3;
	}

	handler = h;
	object = (Command_sender *)obj;
	
	switch(atype)
	{
	default:
	case AT_message:
		widget = XmCreateInformationDialog(base_widget,
			"notice", args, n);
		break;
	case AT_question:
		widget = XmCreateQuestionDialog(base_widget,
			"notice", args, n);
		break;
	case AT_fatal:
		fatal_error = TRUE;
		// FALLTHROUGH
	case AT_error:
		widget = XmCreateErrorDialog(base_widget,
			"notice", args, n);
		break;
	case AT_urgent:
		urgent_msg = TRUE;
		// FALLTHROUGH
	case AT_warning:
		widget = XmCreateWarningDialog(base_widget,
			"notice", args, n);
		break;
	}
	XmStringFree(msg);
	XmStringFree(ok_label);
	XtAddCallback(widget, XmNokCallback, 
		(XtCallbackProc)alert_CB, this);
	if (no_action != LAB_none)
	{
		XmStringFree(cancel_label);
		XtAddCallback(widget, XmNcancelCallback, 
			(XtCallbackProc)alert_CB, this);
	}
	else
		XtUnmanageChild(XmMessageBoxGetChild(widget, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(widget, XmDIALOG_HELP_BUTTON));
	XtAddCallback(XtParent(widget), XmNpopdownCallback, 
		(XtCallbackProc)alert_popdown_CB,
		(XtPointer)this);
}

Alert_shell::~Alert_shell()
{
	XtDestroyWidget(widget);
}

void
Alert_shell::popup()
{
	if (current_notice)
	{
		// Already have at least one pending notice.
		// If this message is urgent, it becomes
		// the current message; otherwise, just
		// add to list.
		if (urgent_msg == TRUE)
		{
			// make existing current message first
			// in pending list and unmap it
			notice_list.first();
			notice_list.insert(current_notice);
			XtUnmanageChild(current_notice->widget);

			current_notice = this;
			XtManageChild(widget);
		}
		else
			// not urgent; just append to list
			notice_list.add(this);
	}
	else
	{
		current_notice = this;
		XtManageChild(widget);
	}
	if (fatal_error)
		fatal_list.add(this);
}

void
Alert_shell::popdown()
{
	XtUnmanageChild(widget);
}

void
Alert_shell::raise()
{
	XRaiseWindow(XtDisplay(widget), XtWindow(widget));
}

void
recover_notice()
{
	if (current_notice)
	{
		current_notice->raise();
	}
}
