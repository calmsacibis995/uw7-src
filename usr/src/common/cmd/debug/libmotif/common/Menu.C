#ident	"@(#)debugger:libmotif/common/Menu.C	1.11"

// GUI headers
#include "UI.h"
#include "Component.h"
#include "Menu.h"
#include "List.h"
#include "Windows.h"
#include "Machine.h"
#include "Base_win.h"
#include "Panes.h"
#include "config.h"
#include "str.h"

#include <stdio.h>
#include <string.h>

#include <Xm/RowColumn.h>
#include <Xm/LabelG.h>
#include <Xm/SeparatoG.h>
#include <Xm/CascadeBG.h>
#include <Xm/PushBG.h>

// called when button is selected - the widget's user data is the
// framework object, and func is its callback function
static void
menu_buttonCB(Widget w, Button *btn, XtPointer)
{
	Menu		*ptr;
	Callback_ptr	func;
	Base_window	*win;
	Window_set	*ws;
	Pane		*pane;
	void		*udata;

	XtVaGetValues(w, XmNuserData, (XtPointer)&ptr, 0);
	if (!ptr || !btn)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	func = btn->get_callback();
	udata = btn->get_udata();
	if (!udata)
		udata = (void *)btn->get_cdata();
	ws = ptr->get_window_set();

	switch(btn->get_flags())
	{
	case Set_cb:
		(ws->*func)(ptr, ptr->get_window());
		break;
	case Window_cb:
		win = ptr->get_window();
		(win->*func)(ptr, udata);
		break;
	case Pane_cb:
		win = ptr->get_window();
		pane = win->get_pane(btn->get_pane_type());
		if (!pane)
		{
			display_msg(E_ERROR, GE_internal, __FILE__,
				__LINE__);
			break;
		}
		(pane->*func)(ptr, udata);
		break;
	case Set_data_cb:
		(ws->*func)(ptr, udata);
		break;
	}
}

// callback when a menu is popped up - make sure
// menu buttons' sensitivity is set

static void
popup_CB(Widget, Menu *menu, XtPointer)
{
	Base_window	*win = menu->get_window();
	menu->set_popped_up();
	win->set_sensitivity(menu);
}

// callback when a menu is popped down - clear
// popped_up flag so we don't unnecesarily set sensitivity
static void
popdown_CB(Widget, Menu *menu, XtPointer)
{
	menu->clear_popped_up();
}

Menu::Menu(Component *p, Boolean has_title, Widget pwidget,
	Base_window *w, Window_set *ws, Menu_descriptor *mdesc,
	Boolean is_pull_down, Help_id h)
{
	parent = p;
	help_msg = h;
	window = w;
	trigger = 0;
	window_set = ws;
	popped_up = FALSE;
	name = makestr(mdesc->get_name());
	children = 0;
	nbuttons = (unsigned short)mdesc->get_nbuttons();

	Arg		args[1];
	int		i, n = 0;
	Widget		*xbutton;
	Button		*btn, *last;

	btn = mdesc->get_buttons();
	buttons = last = 0;
	for(; btn; btn = btn->next())
	{
		Button	*nbtn = new Button(btn);
		if (!buttons)
			buttons = nbtn;
		else
			nbtn->append(last);
		last = nbtn;
	}

#if 0
	// MOTIF problem: tear off menus are causing core dumps
	// when you destroy a second window set
	if (has_title)
	{
		XtSetArg(args[n], XmNtearOffModel, XmTEAR_OFF_ENABLED); n++;
	}
#endif
	if (is_pull_down)
		widget = XmCreatePulldownMenu(pwidget, (char *)name,
			args, n);
	else
		widget = XmCreatePopupMenu(pwidget, (char *)name,
			args, n);
	if (has_title)
	{
		Widget		wid;
		char		*title_str;

		if (ws->get_id() > 1)
		{
			title_str = new char [strlen(name)+1+MAX_INT_DIGITS+1];
			sprintf(title_str, "%s %d", name, ws->get_id());
			delete((char *)name);
			name = (const char *)title_str;
		}
		else
			title_str = (char *)name;
		label = title_str;
		// create title and separator
		XmString xstr = XmStringCreateLocalized(title_str);
		XtSetArg(args[0], XmNlabelString, xstr);
		wid = XmCreateLabelGadget(widget, "Title", args, 1);
		XtManageChild(wid);
		wid = XmCreateSeparatorGadget(widget, "title_separator",
			NULL, 0);
		XtManageChild(wid);
		XmStringFree(xstr);
	}

	XtAddCallback(XtParent(widget), XmNpopupCallback, 
		(XtCallbackProc)popup_CB, (XtPointer)this);
	XtAddCallback(XtParent(widget), XmNpopdownCallback, 
		(XtCallbackProc)popdown_CB, (XtPointer)this);

	if (nbuttons > 0)
		list = xbutton = new Widget[nbuttons];
	else
		list = 0;

	btn = buttons;
	for (i = 0; i < nbuttons; ++xbutton, ++i, btn = btn->next())
	{
		if (btn->get_flags() == Menu_separator)
		{
			(void)XtVaCreateManagedWidget("menu_separator",
				xmSeparatorGadgetClass,
				widget,
				XmNseparatorType, XmSINGLE_LINE,
				0);
			*xbutton = NULL;
		}
		else
		{
			*xbutton = create_button(btn);
		}
	}
}

// create a new button for the menu and return its
// widget; if button is a sub-menu, create child
Widget
Menu::create_button(Button *btn)
{
	KeySym	mne;
	int	n = 0;
	Arg	args[4];
	Widget	w;

	mne = get_mnemonic(btn->get_mne());
	XmString button_str = 
		XmStringCreateLocalized((String)btn->get_name());
	XtSetArg(args[n], XmNlabelString, button_str); n++;
	XtSetArg(args[n], XmNmnemonic, mne); n++; 
	XtSetArg(args[n], XmNuserData, this); n++;

	if (btn->get_flags() == Menu_button)
	{
		// cascading menu - should not have title
		Menu *child = new Menu(this, FALSE, widget,
			window, window_set, btn->get_sub_table(), 
			TRUE, btn->get_help_msg());
		child->trigger = btn;
		if (!children)
			children = new List();
		children->add(child);
		XtSetArg(args[n], XmNsubMenuId, child->get_widget()); n++;
		w = XmCreateCascadeButtonGadget(widget, 
			(char *)btn->get_name(), args, n);
	}
	else
	{
		XtSetArg(args[n], XmNsensitive, 
			(btn->get_callback()) ? TRUE : FALSE); n++;
		w = XmCreatePushButtonGadget(widget, 
			(char *)btn->get_name(), args, n);
		XtAddCallback(w, XmNactivateCallback, 
			(XtCallbackProc)menu_buttonCB, (XtPointer)btn);
	}
	XtManageChild(w);
	XmStringFree(button_str);
	if (btn->get_help_msg())
		register_help(w, 0, btn->get_help_msg());
	return w;
}

// Find a button with label 'label'
Button *
Menu::find_item(const char *label)
{
	Button	*btn;
	for(btn = buttons; btn; btn = btn->next()) 
	{
		if(strcmp(btn->get_name(), label) == 0)
			return btn;
	}
	return NULL;
}

// add an item to end of list
void
Menu::add_item(Button *item)
{
	Button		*btn = buttons, *prev = 0;

	for(; btn; prev = btn, btn = btn->next())
		;
	if (prev)
		item->append(prev);
	else
		buttons = item;

	Widget *new_list = new Widget[nbuttons+1];
	if (list)
		memcpy(new_list, list, sizeof(Widget)*nbuttons);
	new_list[nbuttons] = create_button(item);
	delete list;
	++nbuttons;
	list = new_list;
}

// delete a menu item from menu_table
void
Menu::delete_item(const	char * label)
{
	Button		*btn;
	int		i;

	btn = buttons;
	for(i = 0; btn; i++, btn = btn->next())
	{
		if (strcmp(btn->get_name(), label) == 0)
			break;
	}
	if (!btn)
		return;
	delete_item(btn, i);
}

void
Menu::delete_item(Button *btn, int i)
{
	Widget		*new_list;
	new_list = new Widget[nbuttons-1];
	if (i != 0)
		memcpy(new_list, list, sizeof(Widget)*i);
	if (i < nbuttons-1)
	{
		memcpy(new_list+i, list+i+1, sizeof(Widget)*(nbuttons-i-1));
	}
	XtDestroyWidget(list[i]);
	delete list;
	list = new_list;
	--nbuttons;
	if (i == 0)
		buttons = btn->next();
	btn->unlink();
	if (btn->get_flags() == Menu_button)
	{
		Menu	*mp;
		for (mp = (Menu *)children->first(); mp;
			mp = (Menu *)children->next())
		{
			if (mp->get_trigger() == btn)
			{
				children->remove(mp);
				XtDestroyWidget(mp->get_widget());
				delete(mp);
				break;
			}
		}
	}
	delete(btn);
}

void
Menu::set_menu_label(int b, const char *new_lab)
{
	if (b < 0 || b >= nbuttons)
	{
		// internal error
		return;
	}
	Button	*btn = buttons;
	for(int i = 0; i < b; i++)
		btn = btn->next();
	btn->set_label(new_lab);
	XmString new_str = XmStringCreateLocalized((String)new_lab);
	XtVaSetValues(list[b], XmNlabelString, new_str, 0);
	XmStringFree(new_str);
}

Menu::~Menu()
{
	Button	*btn;
	if (children)
	{
		for (Menu *ptr = (Menu *)children->first(); ptr;
			ptr = (Menu *)children->next())
			delete ptr;
		delete children;
	}
	delete list;
	delete (char *)name;
	btn = buttons;
	while(btn)
	{
		buttons = btn->next();
		delete btn;
		btn = buttons;
	}
}

void
Menu::set_sensitive(int button, Boolean value)
{
	if (list[button] == NULL)
		// internal error?
		return;
	XtVaSetValues(list[button], XmNsensitive, value, 0);
}

Menu_bar::Menu_bar(Component *p, Base_window *w, Window_set *ws,
	Menu_descriptor *menus, Help_id h) : Component(p, 0, 0, h)
{

	Menu_descriptor	*mptr;
	int	i;

	nbuttons = 0;
	for(mptr = menus; mptr; mptr = mptr->next())
		nbuttons++;

	children = new Menu *[nbuttons];
	widget = XmCreateMenuBar( parent->get_widget(),
			"menubar", NULL, 0);
	
	for(i = 0, mptr = menus; mptr; i++, mptr = mptr->next())
	{
		KeySym	mne;
		mne = get_mnemonic(mptr->get_mne());
		children[i] = new Menu(this, TRUE, widget, w, ws, mptr,
			TRUE, HELP_none);
		XmString lab_str = XmStringCreateLocalized((String)mptr->get_name());
		Widget button = XtVaCreateManagedWidget((char *)mptr->get_name(),
			xmCascadeButtonGadgetClass,
			widget,
			XmNlabelString, lab_str,
			XmNmnemonic, mne,
			XmNsubMenuId, children[i]->get_widget(),
			0);
		XmStringFree(lab_str);
#if 0
		if (children[i]->get_help_msg())
			register_help(button, 0, children[i]->get_help_msg());
#endif
	}
	XtManageChild(widget);
}

Menu_bar::~Menu_bar()
{
	Menu	**mp = children;
	int	i;
	for(i = 0; i < nbuttons; i++, mp++)
	{
		delete *mp;
	}
	delete children;
}

static Menu *
search_menu(Menu *mp, CButtons btype)
{
	Menu	*child;
	Button	*btn = mp->get_buttons();
	for(; btn; btn = btn->next())
	{
		if (btn->get_type() == btype)
		{
			if (btn->get_flags() == Menu_button)
			{
				// find corresponding sub-menu
				for (child = mp->first_child(); child;
					child = mp->next_child())
				{
					if (child->get_trigger() == btn)
						return child;
				}
				display_msg(E_ERROR, GE_internal,
					__FILE__, __LINE__);

			}
			return mp;
		}
	}
	// if not found in top-level menu, search sub-menus
	for (child =  mp->first_child(); child;
		child = mp->next_child())
	{
		Menu *mp2;
		if ((mp2 = search_menu(child, btype)) != 0)
			return mp2;
	}
	return 0;
}

// return the sub-menu with the given button type as its trigger
Menu *
Menu_bar::find_item(CButtons btype)
{
	Menu	**mlist = children;
	int	i;

	for(i = 0; i < nbuttons; i++, mlist++)
	{
		Menu	*mp;
		if ((mp = search_menu(*mlist, btype)) != 0)
			return mp;
	}
	return 0;
}

// 3rd mouse button pushed - post popup menu
static void
post_menu_handler(Widget w, Widget menu, XButtonPressedEvent *event)
{
	if (event->button != 3)
		return;
	XmMenuPosition(menu, event);
	XtManageChild(menu);
}

// utility routine used by panes to create popup menus
Menu *
create_popup_menu(Component *cp, Base_window *bw, Menu_descriptor *md)
{
	Menu	*popup;
	popup = new Menu(cp, FALSE, cp->get_widget(), bw,
		bw->get_window_set(), md, FALSE, HELP_none);
	XtAddEventHandler(cp->get_widget(), ButtonPressMask, False,
		(XtEventHandler)post_menu_handler, 
		(XtPointer)popup->get_widget());
	return popup;
}
