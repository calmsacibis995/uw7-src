#ident	"@(#)debugger:libmotif/common/Button_bar.C	1.7"

#include "UI.h"
#include "Component.h"
#include "Windows.h"
#include "Base_win.h"
#include "Panes.h"
#include "Button_bar.h"
#include "Buttons.h"
#include "Label.h"
#include "gui_label.h"
#include "config.h"
#include "Mnemonic.h"

#include <Xm/RowColumn.h>
#include <Xm/PushBG.h>

// callback when button is selected - the widget's user data is the
// framework object, and func is its callback function

static void
button_CB(Widget w, Button_bar *bar, XtPointer)
{
	Button		*btn = 0;
	Callback_ptr	func;
	Base_window	*win;
	Window_set	*ws;
	Pane		*pane;
	Command_sender	*creator;
	void		*udata;

	XtVaGetValues(w, XmNuserData, (XtPointer)&btn, 0);
	if (!btn || !bar)
		return;

	udata = btn->get_udata();
	if (!udata)
		udata = (void*)btn->get_cdata();

	func = btn->get_callback();

	ws = bar->get_window_set();
	switch (btn->get_flags())
	{
	case Set_cb:
		(ws->*func)(bar, bar->get_window());
		break;
	case Window_cb:
		win = bar->get_window();
		(win->*func)(bar, udata);
		break;
	case Pane_cb:
		win = bar->get_window();
		pane = win->get_pane(btn->get_pane_type());
		if (!pane)
		{
			display_msg(E_ERROR, GE_internal, __FILE__,
				__LINE__);
			break;
		}
		(pane->*func)(bar, udata);
		break;
	case Set_data_cb:
		(ws->*func)(bar, udata);
		break;
	case Creator_cb:
		creator = bar->get_creator();
		(creator->*func)(bar, (void *)btn->get_cdata());
		break;
	}
}

static void
next_panel_CB(Widget w, Button_bar *bar, XtPointer)
{
	bar->display_next_panel();
}

// create button widget or just change its values
void
Button_bar::set_button(Widget *w, Button *btn, int change)
{
	XmString	button_str;
	KeySym		mne;
	XtCallbackProc	callback;
	const char	*name;
	Arg		args[3];

	if (btn)
	{
		name = btn->get_name();
		mne = get_mnemonic(btn->get_mne());
		callback = btn->get_callback() ? 
			(XtCallbackProc)button_CB : 0;
		XtSetArg(args[0], XmNuserData, btn);
			
	}
	else
	{
		// Next Panel button
		name = labeltab.get_label(LAB_next_panel);
		mne = get_mnemonic(LAB_next_panel_mne);
		callback = (XtCallbackProc)next_panel_CB;
		XtSetArg(args[0], XmNsensitive, True);
	}
	button_str = XmStringCreateLocalized((String)name);
	XtSetArg(args[1], XmNlabelString, button_str);
	XtSetArg(args[2], XmNmnemonic, mne);

	if (!change)
	{
		// creating new button
		*w = XtCreateManagedWidget(name,
			xmPushButtonGadgetClass, widget, args, 3);
		mne_info->add_mne_info(*w, mne,
			MNE_ACTIVATE_BTN|MNE_GET_FOCUS);
	}
	else
	{
		// just changing values
		XtSetValues(*w, args, 3);
		mne_info->change_mne_info(*w, mne,
			MNE_ACTIVATE_BTN|MNE_GET_FOCUS);
		if (!btn)
			// changing regular button to Change Panel
			// button
			XtRemoveCallback(*w, XmNactivateCallback,
				(XtCallbackProc)button_CB, 
				(XtPointer)this);
		else
			callback = 0;
	}
	if (callback)
		XtAddCallback(*w, XmNactivateCallback,
			callback, (XtPointer)this);
	XmStringFree(button_str);
}

Button_bar::Button_bar(Component *p, Base_window *w, Window_set *ws,
	Bar_descriptor *bdesc, Orientation o, Command_sender *c) 
	: Component(p, 0, c, HELP_none)
{
	int	i;
	Button	*btn;

	window = w;
	window_set = ws;
	nbuttons = bdesc->get_nbuttons();
	mne_info = new MneInfo(this);

	first_panel = current_panel = 0;
	if (bdesc->next())
	{
		// need to add "Next Panel" button
		multiple = TRUE;
		nbuttons++;
	}
	else
		multiple = FALSE;

	for(; bdesc; bdesc = bdesc->next())
	{
		Bar_descriptor *nbar = new Bar_descriptor(bdesc);
		if (!first_panel)
			first_panel = nbar;
		else
			nbar->append(current_panel);
		current_panel = nbar;
	}
	current_panel = first_panel;
	current_index = 0;
	widget = XtVaCreateWidget("button_bar", 
		xmRowColumnWidgetClass,
		parent->get_widget(),
		XmNpacking, XmPACK_TIGHT,
		XmNisHomogeneous, TRUE,
		XmNentryClass, xmPushButtonGadgetClass,
		XmNorientation, o == OR_horizontal ? XmHORIZONTAL : XmVERTICAL,
		0);
	
	xbuttons = new Widget[nbuttons];
	btn = first_panel->get_buttons();
	i = 0;
	if (multiple)
	{
		set_button(xbuttons, 0, 0);
		i++;
	}
	for (; i < nbuttons; ++i, btn = btn->next())
	{
		set_button(&xbuttons[i], btn, 0);
	}
	if (XtIsRealized(parent->get_widget()))
		XtRealizeWidget(widget);
	XtManageChild(widget);
	mne_info->register_mnemonics(TRUE);
}

Button_bar::~Button_bar()
{
	Bar_descriptor	*bd;
	Button		*btn;

	delete xbuttons;
	delete mne_info;
	bd = first_panel;
	while(bd)
	{
		first_panel = bd->next();
		btn = bd->get_buttons();
		while(btn)
		{
			Button	*buttons;
			buttons = btn->next();
			delete btn;
			btn = buttons;
		}
		delete bd;
		bd = first_panel;
	}
}

void
Button_bar::destroy()
{
	XtDestroyWidget(widget);
}

void
Button_bar::set_sensitivity(int button, Boolean sense)
{
	if (multiple)
		button++;
	XtVaSetValues(xbuttons[button], XmNsensitive, sense, 0);
}

void
Button_bar::display_next_panel()
{
	Bar_descriptor	*nbar;

	if (!multiple)
		return;

	if ((nbar = current_panel->next()) == 0)
	{
		// wrap around
		nbar = first_panel;
		current_index = 0;
	}
	else
		current_index++;
	change_display(nbar, FALSE);
	current_panel = nbar;
	// set_sensitivity must be called after current_panel
	// is reset
	window->set_sensitivity();
	window->update_button_config_cb();
}

static Bar_descriptor *
replace_descriptor(int old_index, Bar_descriptor *update, 
	Bar_descriptor *first_panel)
{
	Bar_descriptor	*n, *p;
	Bar_descriptor	*old = first_panel;
	for(; old_index > 0; old_index--)
	{
		old = old->next();
	}
	n = old->next();
	p = (Bar_descriptor *)old->prev();
	old->unlink();
	if (n)
		update->prepend(n);
	else if (p)
		update->append(p);
	if (!p)
		first_panel = update;
	Button *btn = old->get_buttons();
	while(btn)
	{
		Button	*buttons;
		buttons = btn->next();
		delete btn;
		btn = buttons;
	}
	delete old;
	return first_panel;
}

// add to existing descriptor;
// entire new set of button descriptors is passed in; we are
// adding last descriptor in set
static void
add_descriptor(Bar_descriptor *update, Bar_descriptor *old)
{
	Bar_descriptor *bd;
	while(update->next())
		update = update->next();
	bd = new Bar_descriptor(update);
	while(old->next())
		old = old->next();
	bd->append(old);
}

static Bar_descriptor *
remove_descriptor(int old_index, Bar_descriptor *first_panel)
{
	Bar_descriptor *bd = first_panel;
	Bar_descriptor *p, *n;
	int		i;

	for(i = 0; i < old_index; i++)
		bd = bd->next();
	n = bd->next();
	p = (Bar_descriptor *)bd->prev();
	bd->unlink();
	if (!p)
		first_panel = n;
	Button *btn = bd->get_buttons();
	while(btn)
	{
		Button	*buttons;
		buttons = btn->next();
		delete btn;
		btn = buttons;
	}
	delete bd;
	return first_panel;
}

void
Button_bar::update_bar(Bar_info *bi)
{
	int	old_index = bi->old_index;

	if (bi->remove)
	{
		// deleting 
		// must already have 2 or more panels
		int	old_current = current_index;
		first_panel = remove_descriptor(old_index, first_panel);
		current_index = 0;
		if (old_index == old_current)
		{
			current_panel = first_panel;
		}
		else
		{
			Bar_descriptor	*bd = first_panel;
			for(; bd != current_panel; bd = bd->next())
			{
				current_index++;
			}
		}
		if (!first_panel->next())
			multiple = FALSE;
		if ((old_index == old_current) || !multiple)
			change_display(current_panel, (multiple == FALSE));
		return;
	}
	if (old_index == current_index)
	{
		// just updating current bar
		update_current(bi->new_wbar);
	}
	else if (old_index >= 0)
	{
		// replacing existing bar - not currently displayed
		Bar_descriptor *new_desc = 
			new Bar_descriptor(bi->new_bdesc);
		first_panel = replace_descriptor(old_index, 
			new_desc, first_panel);
	}
	else
	{
		// add to existing bars
		add_descriptor(bi->new_wbar, first_panel);
		if (!multiple)
		{
			multiple = TRUE;
			change_display(current_panel, TRUE);
		}
	}
}

void
Button_bar::update_current(Bar_descriptor *update)
{
	int	i;
	Bar_descriptor	*udesc = update;
	Bar_descriptor	*new_desc;
	for(i = 0; i < current_index; i++)
		udesc = udesc->next();

	new_desc = new Bar_descriptor(udesc);
	change_display(new_desc, FALSE);
	first_panel = replace_descriptor(current_index, 
		new_desc, first_panel);
	current_panel = first_panel;
	for(i = 0; i < current_index; i++)
		current_panel = current_panel->next();
	// set_sensitivity must be called after current_panel
	// is reset
	window->set_sensitivity();
}

void
Button_bar::change_display(Bar_descriptor *nbar, 
	Boolean change_multiple)
{
	int		nbuts, i;
	Widget		*nxbuttons;
	int		cp;
	Button		*nbtn;
	Position	x, last_x = 0;
	Boolean		nowarn = FALSE;

	nbuts = nbar->get_nbuttons();
	if (multiple)
		nbuts++;
	nxbuttons = new Widget[nbuts];
	if (nbuts < nbuttons)
	{
		for(i = nbuts; i < nbuttons; i++)
		{
			mne_info->remove_mne_info(xbuttons[i]);
			XtDestroyWidget(xbuttons[i]);
		}
		cp = nbuts;
	}
	else 
		cp = nbuttons;
	// copy old and reset labels and callbacks
	for(i = 0; i < cp; i++)
	{
		nxbuttons[i] = xbuttons[i];
	}
	for(; i < nbuts; i++)
		nxbuttons[i] = 0;
	if (change_multiple)
	{
		if (multiple)
		{
			// adding next panel button
			set_button(nxbuttons, 0, 1);
			XtVaGetValues(nxbuttons[0], XmNx, &last_x, 0);
		}
		else
		{
			// removing next panel button
			XtRemoveCallback(nxbuttons[0], XmNactivateCallback,
				(XtCallbackProc)next_panel_CB, (XtPointer)this);
			XtAddCallback(nxbuttons[0], XmNactivateCallback,
				(XtCallbackProc)button_CB, (XtPointer)this);
		}
	}
	nbtn = nbar->get_buttons();
	i = multiple ? 1 : 0;
	Widget	*wptr = &nxbuttons[i];
	for(; i < nbuts; i++, wptr++, nbtn = nbtn->next())
	{
		set_button(wptr, nbtn, (*wptr != 0));
		if (!nowarn)
		{
			XtVaGetValues(*wptr, XmNx, &x, 0);
			if (x < last_x)
			{
				// warn only once
				nowarn = TRUE;
				display_msg(E_WARNING, 
					GE_buttons_too_wide);
			}
			last_x = x;
		}
	}
	delete xbuttons;
	xbuttons = nxbuttons;
	nbuttons = nbuts;
}
