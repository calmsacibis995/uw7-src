#ident	"@(#)debugger:libmotif/common/Window_sh.C	1.9"

#include "UI.h"
#include "Component.h"
#include "Window_sh.h"
#include "Windows.h"
#include "Message.h"
#include "Msgtab.h"
#include "Machine.h"
#include "gui_label.h"
#include "Label.h"
#include <stdio.h>
#include <stdlib.h>

#include <Xm/Xm.h>
#include <X11/Shell.h>
#include <X11/cursorfont.h>
#include <X11/Xmd.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/AtomMgr.h>
#include <Xm/Protocols.h>

#if DEBUG == 2
#include <X11/Xmu/Editres.h>
#endif

// icon pixmap data
#include "debug.pmp"

// catch the message from the window manager when the user selects
// Close in the window menu, and close the window
static void
wm_delete_cb(Widget, Base_window *bw, XtPointer)
{
	bw->get_window_set()->dismiss(0, bw);
}

Window_shell	*focus_window;

// notice focus change
static void
wm_take_focus_cb(Widget, Window_shell *wsh, XtPointer)
{
	focus_window = wsh;
}

// notify debugger we are ready - first window has been mapped
static Boolean	debugger_notified;

static void
window_mapped_CB(Widget w, Window_shell *ptr, XtPointer)
{
	notify_debugger();
	XtRemoveEventHandler(w, StructureNotifyMask,
		FALSE, (XtEventHandler)window_mapped_CB, 
		(XtPointer)ptr);
}

Window_shell::Window_shell(const char *s, Callback_ptr d, Base_window *c,
	Help_id h, Boolean iconic) : Component(0, s, c, h, WINDOW_SHELL)
{
	const char	*lab = labeltab.get_label(LAB_debug);
	char	*title = new char[strlen(lab)+3+MAX_INT_DIGITS+strlen(label)];

	int	id = c->get_window_set()->get_id();
	if (id > 1)
		sprintf(title, "%s %d: %s", lab, id, label);
	else
		sprintf(title, "%s: %s", lab, label);

	child = focus = 0;
	dismiss = d;
	busy_count = 0;
	msg_widget = 0;
	focus_widget = 0;
	string = 0;
	errors = 0;
	int	state = iconic ? IconicState : NormalState;
	blank_str = XmStringCreateLocalized(" ");

	if (dismiss && !creator)
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);

	window_widget = XtVaCreatePopupShell(label, topLevelShellWidgetClass,
		base_widget, 
		XmNtitle, title, 
		XmNiconName, title,
		XmNdeleteResponse, XmDO_NOTHING,
		XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
		0);
	Display	*dpy = XtDisplay(window_widget);
	static Pixmap icon_pixmap;
	if (!icon_pixmap)
	{
		Screen *screen = XtScreen(window_widget);
		icon_pixmap = XCreatePixmapFromData(dpy,
			RootWindowOfScreen(screen),
			DefaultColormapOfScreen(screen),
			debug_width, debug_height, DefaultDepthOfScreen(screen),
			debug_ncolors, debug_chars_per_pixel, 
			debug_colors, debug_pixels);
	}
	XtVaSetValues(window_widget, 
		XmNiconPixmap, icon_pixmap, 
		XmNiconic, iconic,
		XmNinitialState, state,
		0);

#if DEBUG == 2
	XtAddEventHandler(window_widget, 0, True, 
		(XtEventHandler)_XEditResCheckMessages, 0);
#endif

	// works with MWM only?
	Atom wm_delete_atom = XmInternAtom(dpy, "WM_DELETE_WINDOW", 
		FALSE);
	XmAddWMProtocolCallback(window_widget, wm_delete_atom,
		(XtCallbackProc)wm_delete_cb, (XtPointer)creator);

	Atom	wm_take_focus_atom = XmInternAtom(dpy, "WM_TAKE_FOCUS",
		FALSE);
	XmAddWMProtocolCallback(window_widget, wm_take_focus_atom,
		(XtCallbackProc)wm_take_focus_cb, (XtPointer)this);

	widget = XtVaCreateWidget(label, xmFormWidgetClass, window_widget,
		0);

	// notify debugger when first window comes up
	if (debugger_notified == FALSE)
	{
		debugger_notified = TRUE;
		XtAddEventHandler(window_widget, StructureNotifyMask,
			FALSE, (XtEventHandler)window_mapped_CB, 
			(XtPointer)this);
	}

	// no helpCallback for toplevel shells (only managers & primitives)
	if (help_msg)
		register_help(widget, label, help_msg);
	delete title;
}

Window_shell::~Window_shell()
{
	XtDestroyWidget(window_widget);	// other components don't need to call
					// XtDestroyWidget since it works recursively
	delete child;
	delete string;
	XmStringFree(blank_str);
}

void
Window_shell::popup()
{
	focus_window = this;
	XtPopup(window_widget, XtGrabNone);
	if (focus_widget)
		XmProcessTraversal(focus_widget, XmTRAVERSE_CURRENT);
}

void
Window_shell::popdown()
{
	XtPopdown(window_widget);
}

// raise and set input focus
void
Window_shell::raise(Boolean)	
{
	Display *dpy = XtDisplay(window_widget);
	Window win = XtWindow(window_widget);

	XMapRaised(dpy, win);
	focus_window = this;
}

void
Window_shell::add_component(Component *p)
{
	if (child)
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	child = p;

	msg_widget = XtVaCreateManagedWidget("footer", xmLabelWidgetClass,
		widget,
		XmNlabelString, blank_str,
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		0);
	XtVaSetValues(p->get_widget(),
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, msg_widget,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		0);
	// assumes all children have been created and managed
	XtManageChild(widget);
}

void
Window_shell::set_focus(Component *p)
{
	focus_widget = p->get_widget();
}

void
Window_shell::set_busy(Boolean busy)
{
	Display	*dpy = XtDisplay(window_widget);

	if (busy)
	{
		if (!busy_count)
		{
			static Cursor busy_cur;
			if(!busy_cur)
				busy_cur = XCreateFontCursor(dpy, XC_watch);
			XDefineCursor(dpy, XtWindow(window_widget), busy_cur);
		}
		busy_count++;
	}
	else
	{
		if (!busy_count)
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		busy_count--;
		if (!busy_count)
		{
			XUndefineCursor(dpy, XtWindow(window_widget));
		}
	}	
}

void
Window_shell::display_msg(Severity sev, const char *message)
{
	size_t	len = strlen(message);

	if (!string)
		string = new char[BUFSIZ]; // big enough for any readable message

	if (!errors || sev == E_NONE)
	{
		if (len >= BUFSIZ)
			len = BUFSIZ - 1;
		strncpy(string, message, len);
		// chop off ending new line
		if (string[len-1] == '\n')
			len--;
		string[len] = '\0';
	}
	else
	{
		// already displaying a message, append
		size_t	curlen = strlen(string);
		if (curlen + 1 >= BUFSIZ)
			return;

		if (len + curlen + 1 >= BUFSIZ)
			len = BUFSIZ - (curlen + 2);
		strcat(string, "\n");
		strncat(string, message, len);
		curlen = curlen+len;
		if (string[curlen-1] == '\n')
			curlen--;
		string[curlen] = '\0';
	}
	if (sev > E_NONE)
		errors++;

	// reset the string
	XmString	msg_str = XmStringCreateLocalized(string);
	XtVaSetValues(msg_widget, 
		XmNlabelString, msg_str,
		0);
	XmStringFree(msg_str);
}

void
Window_shell::display_msg(Severity severity, Gui_msg_id mid, ...)
{
	va_list		ap;

	va_start(ap, mid);
	const char *message = do_vsprintf(gm_format(mid), ap);
	va_end(ap);

	display_msg(severity, message);
}

void
Window_shell::display_msg(Message *msg)
{
	if (Mtable.msg_class(msg->get_msg_id()) == MSGCL_error)
		display_msg(msg->get_severity(), msg->format());
	else
		display_msg(E_NONE, msg->format());
}

// get rid of old messages
void
Window_shell::clear_msg()
{
	XtVaSetValues(msg_widget, 
		XmNlabelString, blank_str,
		0);
	errors = 0;
}

// set initial x,y coordinates for Window_shell based on saved
// configuration
void
Window_shell::set_initial_pos(unsigned short x, unsigned short y)
{
	XtVaSetValues(window_widget,
		XmNx, x,
		XmNy, y,
		0);
}

// get current x,y coordinates for Window_shell to save
void
Window_shell::get_current_pos(unsigned short &x, unsigned short &y)
{
	XtVaGetValues(window_widget,
		XmNx, &x,
		XmNy, &y,
		0);
}

#define NumPropWMStateElements 2
typedef struct {
	CARD32		state;
	Window		icon;
} xPropWMState;

// code borrowed from Desktop Manager
Boolean
Window_shell::is_iconic()
{
	int		state;
	Atom		atr, wm_state;
	int		afr;
	unsigned long	nir, bar;
	xPropWMState	*prop;
	Display		*dpy = XtDisplay(window_widget);

	wm_state = XInternAtom(dpy, "WM_STATE", False);
	if (XGetWindowProperty(dpy, XtWindow(window_widget), 
				wm_state, 0L,
				NumPropWMStateElements, False,
				wm_state, &atr, &afr, &nir, &bar,
				(unsigned char **) (&prop)) != Success)
	{
		return False;		// punt
	}

        if (atr != wm_state || nir < NumPropWMStateElements || 
		afr != 32)
	{
		state = NormalState;	// punt
	}
	else
	{
		state = prop->state;
	}
	if (prop != (xPropWMState *) 0)
		free((char *)prop);
	return(state == IconicState);
}
