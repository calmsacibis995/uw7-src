#ident	"@(#)debugger:libmotif/common/Dialog_sh.C	1.14"

// GUI headers
#include "Help.h"
#include "UI.h"
#include "Component.h"
#include "Dialog_sh.h"
#include "Dialogs.h"
#include "Windows.h"
#include "gui_label.h"
#include "Label.h"
#include "Mnemonic.h"

// Debug headers
#include "Machine.h"
#include "Message.h"
#include "Msgtab.h"
#include "UIutil.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <Xm/MwmUtil.h>
#include <Xm/PushB.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/SeparatoG.h>
#include <Xm/TextF.h>
#include <Xm/Protocols.h>

#define TITLE_SIZE	80
#define BUTTON_SPACING	4

// handle XmNpopdownCallback - received whenever XtPopdown
// is called - either by window manager or explicitly as result
// of Cancel button
static void
dialog_popdownCB(Widget w, Dialog_shell *ptr, XtPointer)
{
	if (!ptr)
		return;
	if (ptr->get_state() == Dialog_popped_up)
		ptr->dismiss();
	ptr->set_state(Dialog_popped_down);
}

// catch unmap events - if parent window is closed, dialog is
// unmapped, but doesn't get the popdownCallback
static void
unmap_handler(Widget w, Dialog_shell *ptr, XEvent *event, 
	Boolean *cont_to_dispatch)
{
	*cont_to_dispatch = True;
	if ((event->type != UnmapNotify) || !ptr ||
		(ptr->get_state() != Dialog_popped_up))
		return;
	ptr->dismiss();
	ptr->set_state(Dialog_unmapped);
}

// close the dialog (close button in window-manager menu or on
// dialog) - the popdownCB will get invoked as a result of the
// call to XtPodown
static void
dialog_closeCB(Widget w, Dialog_shell *dptr, 
	XmPushButtonCallbackStruct *)
{
	if (!dptr || (dptr->get_state() != Dialog_popped_up))
		return;

	XtPopdown(dptr->get_popup_window());
	dptr->set_state(Dialog_popped_down);
}

// invoke the associated framework callback
// if the callback doesn't call wait_for_response() 
// (incrementing cmds_sent) the window
// will be popped down when the callback returns
static void
dialog_execCB(Widget w, Dialog_shell *dptr, XmPushButtonCallbackStruct *)
{
	Command_sender	*creator;
	Callback_ptr	func;

	if (!dptr || (dptr->get_state() != Dialog_popped_up))
		return;

	DButton	*button = 0;

	XtVaGetValues(w, 
		XmNuserData, &button,
		0);
	if (!button || !button->func)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	func = button->func;

	dptr->set_ok_to_popdown(TRUE);
	dptr->set_busy(TRUE);
	dptr->clear_msg();
	creator = dptr->get_creator();
	(creator->*func)(dptr, (void *)0);
	if (!dptr->get_cmds_sent())
	{
		dptr->set_busy(FALSE);
		dptr->popdown();
	}
}
	
// invoke the associated framework callback, without closing the window
static void
dialog_non_execCB(Widget w, Dialog_shell *dptr, XmPushButtonCallbackStruct *)
{
	Command_sender	*creator;
	Callback_ptr	func;

	if (!dptr || (dptr->get_state() != Dialog_popped_up))
		return;

	DButton	*button = 0;

	XtVaGetValues(w,
		XmNuserData, &button,
		0);
	if (!button || !button->func)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	func = button->func;

	dptr->set_busy(TRUE);
	dptr->clear_msg();
	dptr->set_ok_to_popdown(FALSE);
	creator = dptr->get_creator();
	(creator->*func)(dptr, (void *)0);
	if (!dptr->get_cmds_sent())
		dptr->set_busy(FALSE);
}

// invoke the help window
static void
dialog_help(Widget w, Dialog_shell *dptr, XmPushButtonCallbackStruct *)
{
	if (!dptr || !dptr->get_help_msg())
		return;

	display_help(dptr->get_widget(), HM_section, dptr->get_help_msg());
}

// get rid of old error messages
void
Dialog_shell::clear_msg()
{
	XmTextFieldSetString(msg_widget, "");
	delete error_string;
	error_string = 0;
}

// create the row of buttons at the bottom of the window
void
Dialog_shell::make_buttons(const DButton *b, int nb)
{
	Widget	*btn, *prev_btn = 0;

	if (!b)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	control_area = XtVaCreateWidget("lowerControl",
		xmFormWidgetClass,
		widget,
		0);
	nbuttons = nb;
	btn = buttons = new Widget[nb];
	for (int i = 0; i < nb; b++, i++)
	{
		XtCallbackProc	cb = 0;
		XtArgVal	is_default = (prev_btn == 0);
				// i could be > 0 even though
				// we are not working on first button;
				// see case B_help
		LabelId 	name;
		XmString	label_string;
		KeySym		mne;
		LabelId		mne_label = LAB_none;
		Boolean		map_escape = FALSE;

		switch (b->type)
		{
		case B_ok:
			if (b->label)
				name = b->label;
			else
			{
				name = LAB_dok;
				mne_label = LAB_dok_mne;
			}
			if (b->func)
				cb = (XtCallbackProc)dialog_execCB;
			break;
		case B_apply:
			if (b->label)
				name = b->label;
			else
			{
				name = LAB_dapply;
				mne_label = LAB_dapply_mne;
			}
			if (b->func)
				cb = (XtCallbackProc)dialog_non_execCB;
			if (is_default)
				default_is_exec = FALSE;
			break;

		// Close dismisses the popup, without changing any controls
		case B_close:
			if (b->label)
				name = b->label;
			else
			{
				name = LAB_dclose;
				mne_label = LAB_dclose_mne;
			}
			cb = (XtCallbackProc)dialog_closeCB;
			map_escape = TRUE;
			break;

		// Cancel is equivalent to Reset followed by Close
		case B_cancel:
			if (b->label)
				name = b->label;
			else
			{
				name = LAB_dcancel;
				mne_label = LAB_dcancel_mne;
			}
			cb = (XtCallbackProc)dialog_execCB;
			map_escape = TRUE;
			break;

		// Reset the dialog to its state when last popped up
		case B_reset:
			if (b->label)
				name = b->label;
			else
			{
				name = LAB_dreset;
				mne_label = LAB_dreset_mne;
			}
			if (b->func)
				cb = (XtCallbackProc)dialog_non_execCB;
			break;

		// execute the associated action then dismiss the window
		case B_exec:
			if (b->label)
				name = b->label;
			else
			{
				name = LAB_dok;
				mne_label = LAB_dok_mne;
			}
			if (b->func)
				cb = (XtCallbackProc)dialog_execCB;
			break;

		// execute the associated action without dismissing the window
		case B_non_exec:
			if (b->label)
				name = b->label;
			else
			{
				name = LAB_dapply;
				mne_label = LAB_dapply_mne;
			}
			if (b->func)
				cb = (XtCallbackProc)dialog_non_execCB;
			if (is_default)
				default_is_exec = FALSE;
			break;

		case B_help:
			if (!help_msg)
				continue;
			name = LAB_dhelp;
			mne_label = LAB_dhelp_mne;
			cb = (XtCallbackProc)dialog_help;
			break;

		default:
			break;
		}
		const char	*label = labeltab.get_label(name);
		if (mne_label != LAB_none)
			mne = get_mnemonic(mne_label);
		else if (b->mnemonic)
			mne = get_mnemonic(b->mnemonic);
		else
		{
			wchar_t	wc;
			mbtowc(&wc, label, MB_CUR_MAX);
			mne = get_mnemonic(wc);
		}

		label_string = XmStringCreateLocalized((String)label);
		*btn = XtVaCreateManagedWidget(label,
			xmPushButtonWidgetClass,
			control_area,
			XmNlabelString, label_string,
			XmNmnemonic, mne,
			XmNdefaultButtonShadowThickness, 2,
			XmNshowAsDefault, is_default,
			XmNsensitive, cb ? TRUE : FALSE,
			XmNuserData, b,
			0);
		mne_info->add_mne_info(*btn, mne,
			MNE_ACTIVATE_BTN|MNE_GET_FOCUS);
		if (is_default)
			XtVaSetValues(widget, XmNdefaultButton, *btn, 0);
		if (map_escape)
			escape_button = *btn;
		XmStringFree(label_string);

		// do the initial attachments
		// NOTE: we need to do this even though we're going to
		// reassign positions to each button later because
		// otherwise there might not be enough space to fit all
		// buttons
		if (prev_btn == 0)
			XtVaSetValues(*btn,
				XmNleftAttachment, XmATTACH_FORM,
				0);
		else
			XtVaSetValues(*btn,
				XmNleftAttachment, XmATTACH_WIDGET,
				XmNleftWidget, *prev_btn,
				0);

		if (cb)
			XtAddCallback(*btn, XmNactivateCallback,
				(XtCallbackProc)cb, (XtPointer)this);
		prev_btn = btn;
		btn++;
	}
	nbuttons = btn-buttons; // may be less than nb
	XtManageChild(control_area);
}

// Dialog_shell is implemented as a dialog shell with box, control area
// (for buttons) and footer children.

Dialog_shell::Dialog_shell(Component *p, LabelId tid, Callback_ptr d, Dialog_box *c,
	const DButton *bptr, int num_buttons, Help_id h, Callback_ptr, 
	Drop_cb_action) : COMPONENT(p, 0, c, h, DIALOG_SHELL)
{
	if (!creator)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	Atom	WM_DELETE_WINDOW;

	const char	*dlabel = labeltab.get_label(LAB_debug);
	label = labeltab.get_label(tid);
	char *title = new char[strlen(dlabel)+3+MAX_INT_DIGITS+strlen(label)+1];
	int	id = ((Dialog_box *)creator)->get_window_set()->get_id();
	if (id > 1)
		sprintf(title, "%s %d: %s", dlabel, id, label);
	else
		sprintf(title, "%s: %s", dlabel, label);

	dismiss_cb = d;
	ok_to_popdown = FALSE;
	cmds_sent = 0;
	child = 0;
	error_string = 0;
	state = Dialog_popped_down;
	focus_widget = 0;
	escape_button = 0;
	default_is_exec = TRUE;
	is_busy = FALSE;
	busy_buttons = 0;
	buttons_centered = FALSE;
	mne_info = new MneInfo(this);

	popup_window = XtVaCreatePopupShell(label, 
		xmDialogShellWidgetClass,
		parent ? parent->get_widget() : base_widget,
		XmNtitle, title,
		XmNmwmDecorations, MWM_DECOR_ALL|MWM_DECOR_MINIMIZE|MWM_DECOR_MAXIMIZE,
		XmNmwmFunctions, MWM_FUNC_ALL|MWM_FUNC_MINIMIZE|MWM_FUNC_MAXIMIZE,
		XmNdeleteResponse, XmDO_NOTHING,
		XmNuserData, this,
		0);
	widget = XtVaCreateWidget(label, xmFormWidgetClass,
		popup_window,
		XmNuserData, this,
		XmNhorizontalSpacing, BUTTON_SPACING,
		0);
	XtAddCallback(popup_window, XmNpopdownCallback,
		(XtCallbackProc)dialog_popdownCB, (XtPointer)this);
	// catch unmap events - if parent window is closed, dialog is
	// unmapped, but doesn't get the XmNpopdownCallback
	XtAddEventHandler(popup_window, StructureNotifyMask,
		False, (XtEventHandler)unmap_handler, (XtPointer)this);

	// map "Close" menu button to "Close" dialog button
	WM_DELETE_WINDOW = XmInternAtom(XtDisplay(popup_window),
		"WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(popup_window, WM_DELETE_WINDOW,
		(XtCallbackProc)dialog_closeCB, this);
	
	mseparator = XtVaCreateManagedWidget("msg_separator",
		xmSeparatorGadgetClass,
		widget,
		0);

	msg_widget = XtVaCreateManagedWidget("error_msg", 
		xmTextFieldWidgetClass,
		widget,
		XmNeditable, FALSE,
		XmNcursorPositionVisible, FALSE,
		XmNvalue, "",
		XmNuserData, this,
		0);

	Widget	bseparator;
	bseparator = XtVaCreateManagedWidget("button_separator",
		xmSeparatorGadgetClass,
		widget,
		0);

	make_buttons(bptr, num_buttons);

	// do layout
	XtVaSetValues(mseparator,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, msg_widget,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		0);
	XtVaSetValues(msg_widget,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, bseparator,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		0);
	XtVaSetValues(bseparator,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, control_area,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		0);
	XtVaSetValues(control_area,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		0);

	if (help_msg)
		register_help(widget, label, help_msg);

	delete title;
}

Dialog_shell::~Dialog_shell()
{
	XtDestroyWidget(popup_window);
	delete child;
	delete error_string;
	delete buttons;
	delete busy_buttons;
	delete mne_info;
}

void
Dialog_shell::set_popup_focus(Component *cp)
{
	focus_widget = cp->get_widget();
}

void
Dialog_shell::add_component(Component *box)
{
	if (!box || !box->get_widget() || child)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	child = box;

	XtVaSetValues(box->get_widget(),
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, mseparator,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		0);
	// assumes all children have been created and managed
	XtManageChild(widget);
	mne_info->register_mnemonics(TRUE);
}

// return the item dropped from the desktop
char *
Dialog_shell::get_drop_item()
{
	return NULL;
}

void
Dialog_shell::popup()
{
	Display *dpy = XtDisplay(popup_window);

	clear_msg();
	switch(state)
	{
	case Dialog_popped_up:
		XRaiseWindow(dpy, XtWindow(popup_window));
		break;
	case Dialog_unmapped:
		XMapRaised(dpy, XtWindow(popup_window));
		break;
	case Dialog_popped_down:
		XtPopup(popup_window, XtGrabNone);
		// center buttons
		if (!buttons_centered)
		{
			center_buttons();
			buttons_centered = TRUE;
		}
		break;
	}
	state = Dialog_popped_up;
	ok_to_popdown = FALSE;
	if (focus_widget)
	{
		XmProcessTraversal(focus_widget, XmTRAVERSE_CURRENT);
	}
	else
	{
		XmProcessTraversal(buttons[0], XmTRAVERSE_CURRENT);
	}
	recover_notice();
}

// center and position the buttons so that resizing the dialog
// will cause the buttons to realign properly
void
Dialog_shell::center_buttons()
{
	if (nbuttons <= 0)
		// internal error
		return;
	int shell_w = 0;
	XtVaGetValues (popup_window, XmNwidth, &shell_w, 0);
	int fract_base = 0;
	XtVaGetValues(widget, XmNfractionBase, &fract_base, 0);
	int	total_bw = 0;
	int	*bws = new int[nbuttons];
	int	i;
	for (i = 0; i < nbuttons; ++i)
	{	
		bws[i] = 0;
		XtVaGetValues(buttons[i], XmNwidth, &bws[i], 0);
		total_bw += bws[i];
	}
	total_bw += BUTTON_SPACING*(nbuttons-1);
	// assert (shell_w >= total_bw)
	int x = (shell_w-total_bw)/2;
	for (i = 0; i < nbuttons; ++i)
	{
		XtVaSetValues(buttons[i],
			XmNleftAttachment, XmATTACH_POSITION,
			XmNleftPosition, x*fract_base/shell_w,
			0);
		x += bws[i] + BUTTON_SPACING;
	}
	delete bws;
}

// If there were no errors, dismiss the window.  If there were
// errors, the window is left up so the user has time to read the error
// message (of course, that means that the user has to do something to
// get rid of the popup)
void
Dialog_shell::popdown()
{
	if (cmds_sent)
		return;

	if (ok_to_popdown && !error_string)
	{
		XtPopdown(popup_window);
		state = Dialog_popped_down;
	}
}

Boolean
Dialog_shell::is_pinned()
{
	return FALSE;
}
	
void
Dialog_shell::cmd_complete()
{
	if (cmds_sent)
	{
		cmds_sent = FALSE;
		set_busy(FALSE);
		if (ok_to_popdown)
		{
			popdown();
		}
	}
}

// This function isn't inlined, even though it is an obvious candidate, because
// there doesn't seem to be any way to do it, since the interface is in the common
// header but the implementation is toolkit specific
void
Dialog_shell::wait_for_response()
{
	cmds_sent = TRUE;
}

// display an error or warning message in the dialog's footer;
// don't write over existing error messages
void
Dialog_shell::error(Severity sev, const char *message)
{
	if (error_string)
		return;

	const char	*wmsg;
	size_t		wlen;
	size_t		len = strlen(message);
	if (sev == E_WARNING)
	{
		wmsg = ::get_label(sev);
		wlen = strlen(wmsg);
		len += wlen;
	}
	error_string = new char[len + 1];
	if (sev == E_WARNING)
	{
		char	*p;
		strcpy(error_string, wmsg);
		p = error_string + wlen;
		strcpy(p, message);
	}
	else
		strcpy(error_string, message);
	// chop off ending new line(s)
	while (error_string[len-1] == '\n')
		len--;
	error_string[len] = '\0';

	// reset the string
	XmTextFieldSetString(msg_widget, error_string);
}

void
Dialog_shell::error(Severity severity, Gui_msg_id mid, ...)
{
	va_list		ap;

	if (error_string)
		return;
	va_start(ap, mid);
	const char *message = do_vsprintf(gm_format(mid), ap);
	va_end(ap);

	error(severity, message);
}

void
Dialog_shell::error(Message *msg)
{
	if (error_string)
		return;
	if (Mtable.msg_class(msg->get_msg_id()) == MSGCL_error)
		error(msg->get_severity(), msg->format());
	else
		error(E_NONE, msg->format());
}

// set_busy sets the busy indicator until the command is completed
// set_busy(FALSE) unsets the busy indicator
void
Dialog_shell::set_busy(Boolean busy)
{
	busy_window(popup_window, busy);
	if (busy)
	{
		if (is_busy)
			// already busied
			return;
		is_busy = TRUE;
		if (!busy_buttons)
			busy_buttons = new Boolean[nbuttons];
		for (int i = 0; i < nbuttons; ++i)
		{
			// save old sensitive state
			XtVaGetValues(buttons[i], XmNsensitive, &busy_buttons[i], 0);
			// set button insensitive
			XtVaSetValues(buttons[i], XmNsensitive, FALSE, 0);
		}
	}
	else
	{
		if (!is_busy)
			// already unbusied
			return;
		is_busy = FALSE;
		for (int i = 0; i < nbuttons; ++i)
			// if button was sensitive and still is, restore its
			// sensitivity
			if (busy_buttons[i])
				XtVaSetValues(buttons[i], XmNsensitive, TRUE, 0);
	}
}

void
Dialog_shell::set_label(LabelId new_label, LabelId mnemonic)
{
	const char	*dlabel = labeltab.get_label(LAB_debug);
	const char	*nlabel = labeltab.get_label(new_label);
	KeySym		mne = get_mnemonic(mnemonic);

	char	*title = new char[strlen(dlabel)+3+MAX_INT_DIGITS+strlen(nlabel)+1];
	int	id = ((Dialog_box *)creator)->get_window_set()->get_id();

	if (id > 1)
		sprintf(title, "%s %d: %s", dlabel, id, nlabel);
	else
		sprintf(title, "%s: %s", dlabel, nlabel);
	XtVaSetValues(popup_window, XmNtitle, title, 0);

	// reset the label on the default button (the zero-th entry)
	XmString	new_string = XmStringCreateLocalized((String)nlabel);
	XtVaSetValues(buttons[0],
		XmNlabelString, new_string,
		XmNmnemonic, mne,
		0);
	mne_info->change_mne_info(buttons[0], mne,
		MNE_ACTIVATE_BTN|MNE_GET_FOCUS);
	if (buttons_centered == TRUE)
		// set once, must reset
		center_buttons();
	XmStringFree(new_string);
	delete title;
}

void
Dialog_shell::set_focus(Component *cp)
{
	XmProcessTraversal(cp->get_widget(), XmTRAVERSE_CURRENT);
}

void
Dialog_shell::set_sensitive(LabelId lab, Boolean s)
{
	int		i;

	for (i = 0; i < nbuttons; i++)
	{
		DButton *b = 0;

		XtVaGetValues(buttons[i], XmNuserData, &b, 0);
		if (b->label == lab)
		{
			if (is_busy)
				busy_buttons[i] = s;
			else
				XtVaSetValues(buttons[i], XmNsensitive, s, 0);
			break;
		}
	}
}

void
Dialog_shell::set_sensitive(Boolean s)
{
	int	i;

	for (i = 0; i < nbuttons; i++)
	{
		DButton	*b = 0;
		XtVaGetValues(buttons[i], XmNuserData, &b, 0);
		switch(b->type)
		{
		case B_close:
		case B_help:
		case B_cancel:
			continue;
		}
		XtVaSetValues(buttons[i], XmNsensitive, s, 0);
	}
}

void
Dialog_shell::set_help(Help_id help)
{
	if (!help)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	help_msg = help;
	//register_help(popup_window, label, help_msg);
}

Boolean
Dialog_shell::is_open()
{
	return (state == Dialog_popped_up);
}

// called by framework at the start of default activation cb
void
Dialog_shell::default_start()
{
	set_busy(TRUE);
	clear_msg();
	ok_to_popdown = default_is_exec ? TRUE : FALSE;
}

// called by framework at the end of default activation cb
void
Dialog_shell::default_done()
{
	if (!cmds_sent)
	{
		set_busy(FALSE);
		if (default_is_exec)
			popdown();
	}
}

// called when escape is hit - maps to cancel or close
void
Dialog_shell::handle_escape(XEvent *xe)
{
	if (escape_button)
		XtCallActionProc(escape_button, "ArmAndActivate", 
			xe, 0, 0);
}

// this dialog should not have a default when return is hit
void
Dialog_shell::remove_default_button()
{
	XtVaSetValues(widget, 
		XmNdefaultButton, 0,
		0);
}

void
Dialog_shell::dismiss()
{
	if (dismiss_cb && creator)
		(creator->*dismiss_cb)(this, 0);
}
