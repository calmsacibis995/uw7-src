#ident	"@(#)debugger:libmotif/common/FSdialog.C	1.3"

#include "UI.h"
#include "Sender.h"
#include "Buttons.h"
#include "Dialogs.h"
#include "config.h"
#include "gui_label.h"
#include "Label.h"
#include "Path.h"

#include "Button_bar.h"
#include "Component.h"
#include "FSdialog.h"

#include "Vector.h"

#include <Xm/Xm.h>
#include <Xm/MwmUtil.h>
#include <Xm/DialogS.h>
#include <Xm/FileSB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/Protocols.h>

static void
button_CB(Widget, FileSelectionDialog *dlg, XmFileSelectionBoxCallbackStruct *cd)
{
	char		*text = 0;
	Callback_ptr	func;
	Command_sender	*creator;

	func = dlg->get_activation_cb();
	switch(cd->reason)
	{
	case XmCR_HELP:
		if (!dlg || !dlg->get_help_msg())
			return;
		display_help(dlg->get_widget(), HM_section, 
			dlg->get_help_msg());
		return;
	case XmCR_CANCEL:
	case XmCR_NO_MATCH:
		break;
	case XmCR_OK:
	case XmCR_APPLY:
		if (!XmStringGetLtoR(cd->value, XmSTRING_DEFAULT_CHARSET, &text))
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
		creator = dlg->get_creator();
		(creator->*func)(dlg, (void *)text);
		XtFree(text);
		break;
	}
	dlg->popdown();
}

static void
list_select_CB(Widget, FileSelectionDialog *d, XmListCallbackStruct *ptr)
{
	if (!ptr || !d)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	switch(ptr->reason)
	{
	case XmCR_SINGLE_SELECT:
		d->handle_select(ptr);
		break;
	case XmCR_MULTIPLE_SELECT:
		d->handle_mselect(ptr);
		break;
	}
}

static void
dlist_default_CB(Widget, FileSelectionDialog *d, XmListCallbackStruct *)
{
	if (!d)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	d->reset_sensitivity();
}

static void
extra_button_CB(Widget w, Button *btn, XtPointer)
{
	FileSelectionDialog	*ptr;
	Callback_ptr		func;
	Command_sender		*creator;

	XtVaGetValues(w, XmNuserData, (XtPointer)&ptr, 0);
	func = btn->get_callback();
	creator = ptr->get_creator();
	(creator->*func)(ptr, (void *)btn->get_cdata());
}

FileSelectionDialog::FileSelectionDialog(
	Component *p,
	const char *s, const char *pattern,
	Callback_ptr activate_cb, Command_sender *c, Help_id h,
	Select_mode sm, Bar_descriptor *bd)
	: Component(p, s, c, h, DIALOG_SHELL)
{
	Arg		args[2];
	int 		n = 0;
	XmString	str = 0;
	XmString	dir = 0;
	Atom		WM_DELETE_WINDOW;

	acb = activate_cb;
	_is_open = FALSE;
	cmd_sent = FALSE;
	shell = XtVaCreatePopupShell(label, xmDialogShellWidgetClass,
		parent ? parent->get_widget() : base_widget,
		XmNtitle, s, 
		XmNmwmDecorations, MWM_DECOR_ALL|MWM_DECOR_MINIMIZE|MWM_DECOR_MAXIMIZE,
		XmNmwmFunctions, MWM_FUNC_ALL|MWM_FUNC_MINIMIZE|MWM_FUNC_MAXIMIZE,
		XmNdeleteResponse, XmDO_NOTHING,
		0);
	if (pattern)
	{
		str = XmStringCreateSimple((char *)pattern);
		XtSetArg(args[0], XmNpattern, str);
		n = 1;
	}
	if (current_dir || init_cwd())
	{
		dir = XmStringCreateSimple(current_dir);
		XtSetArg(args[n], XmNdirectory, dir);
		n++;
	}
	widget = XmCreateFileSelectionBox(shell, (char *)s, args, n);
	if (str)
		XmStringFree(str);
	if (dir)
		XmStringFree(dir);
	XtAddCallback(widget, XmNokCallback, 
		(XtCallbackProc)button_CB, (XtPointer)this);
	XtAddCallback(widget, XmNcancelCallback, 
		(XtCallbackProc)button_CB, (XtPointer)this);
	XtAddCallback(widget, XmNhelpCallback, 
		(XtCallbackProc)button_CB, (XtPointer)this);
	XtAddCallback(widget, XmNnoMatchCallback, 
		(XtCallbackProc)button_CB, (XtPointer)this);

	// map "Close" menu button to "Close" dialog button
	WM_DELETE_WINDOW = XmInternAtom(XtDisplay(shell),
		"WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, WM_DELETE_WINDOW,
		(XtCallbackProc)button_CB, this);

	Widget listw = XmFileSelectionBoxGetChild(widget, XmDIALOG_LIST);
	if (sm == SM_multiple)
	{
		XtVaSetValues(listw, XmNselectionPolicy, XmMULTIPLE_SELECT, 0);
	}
	if (bd)
	{
		Widget dlistw = XmFileSelectionBoxGetChild(widget, XmDIALOG_DIR_LIST);
		XtAddCallback(dlistw, XmNdefaultActionCallback,
			(XtCallbackProc)dlist_default_CB, (XtPointer)this);
		XtAddCallback(listw, (sm == SM_multiple) ? 
				XmNmultipleSelectionCallback : 
				XmNsingleSelectionCallback,
			(XtCallbackProc)list_select_CB, (XtPointer)this);
		Widget rc = XtVaCreateWidget("extra_buttons",
			xmRowColumnWidgetClass, widget,
			XmNorientation, XmHORIZONTAL,
			0);
		extra_buttons = bd;
		buttons = new Widget[bd->get_nbuttons()];
		int i = 0;
		for (Button *btn = extra_buttons->get_buttons();
			btn; ++i, btn = btn->next())
		{
			buttons[i] = XtVaCreateManagedWidget((char *)btn->get_name(), 
				xmPushButtonGadgetClass,
				rc,
				XmNuserData, this,
				XmNsensitive, btn->get_sensitivity()->sel_required() ?
						FALSE : TRUE,
				0);
			if (btn->get_callback())
				XtAddCallback(buttons[i], XmNactivateCallback,
					(XtCallbackProc)extra_button_CB, 
					(XtPointer)btn);
		}
		XtManageChild(rc);
	}
	else
		buttons = 0;
	last_selection = -1;
	selected = 0;
}

FileSelectionDialog::~FileSelectionDialog()
{
	delete extra_buttons;
}

void
FileSelectionDialog::reset_sensitivity()
{
	int i = 0;
	for (Button *btn = extra_buttons->get_buttons(); btn; 
		++i, btn = btn->next())
	{
		XtVaSetValues(buttons[i], XmNsensitive, 
			btn->get_sensitivity()->always() ? TRUE : FALSE,
			0);
	}
	last_selection = -1;
	selected = 0;
}

void
FileSelectionDialog::handle_select(XmListCallbackStruct *ptr)
{
	if (last_selection != ptr->item_position)
	{
		// select
		set_sensitivity(TRUE);
		last_selection = ptr->item_position;
	}
	else
	{
		// deselect
		set_sensitivity(FALSE);
		last_selection = -1;
	}
}

static Boolean
is_selected(int pos, int *sels, int nsels)
{
	for (int i = 0; i < nsels; ++i)
	{
		if (pos < sels[i])
			return FALSE;
		else if (pos == sels[i])
			return TRUE;
	}
	return FALSE;
}

// handle SELECT clicks in XmMULTIPLE_SELECT mode
void
FileSelectionDialog::handle_mselect(XmListCallbackStruct *ptr)
{
	if (is_selected(ptr->item_position, ptr->selected_item_positions,
		 	ptr->selected_item_count))
	{
		if (selected == 0)
			set_sensitivity(TRUE);
		++selected;
	}
	else
	{
		--selected;
		if (!selected)
			set_sensitivity(FALSE);
	}
}

void
FileSelectionDialog::set_sensitivity(Boolean is_selected)
{
	int i = 0;
	for (Button *btn = extra_buttons->get_buttons(); btn; 
		++i, btn = btn->next())
	{
		if (btn->get_sensitivity()->sel_required())
		{
			XtVaSetValues(buttons[i], XmNsensitive, is_selected, 0);
		}
	}
}

int
FileSelectionDialog::get_files(Vector *v, Boolean selected)
{
	Widget		file_list;
	XmStringTable	file_tab;
	int nfiles = 0;
	file_list = XmFileSelectionBoxGetChild(widget, XmDIALOG_LIST);
	if (selected)
	{
		XtVaGetValues(file_list, 
			XmNselectedItems, &file_tab,
			XmNselectedItemCount, &nfiles,
			0);
	}
	else
	{
		XtVaGetValues(file_list, 
			XmNitems, &file_tab,
			XmNitemCount, &nfiles,
			0);
	}
	v->clear();
	for (int i = 0; i < nfiles; ++i)
	{
		char	*file;
		XmStringGetLtoR(file_tab[i], XmFONTLIST_DEFAULT_TAG, &file);
		char 	*full_path_name = makestr(file);
		v->add(&full_path_name, sizeof(char *));
		XtFree(file);
	}
	return nfiles;
}

void
FileSelectionDialog::popup()
{
	if (_is_open)
	{
		XRaiseWindow(XtDisplay(shell), XtWindow(shell));
		return;
	}
	_is_open = TRUE;
	XtManageChild(widget);
}

void
FileSelectionDialog::popdown()
{
	if (cmd_sent)
		return;
	XtUnmanageChild(widget);
	_is_open = FALSE;
}

void
FileSelectionDialog::wait_for_response()
{
	cmd_sent = TRUE;
	// set busy mode
	busy_window(shell, TRUE);
}

void
FileSelectionDialog::cmd_done()
{
	cmd_sent = FALSE;
	// unset busy mode
	busy_window(shell, FALSE);
}
