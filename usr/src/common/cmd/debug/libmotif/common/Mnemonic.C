#ident	"@(#)debugger:libmotif/common/Mnemonic.C	1.6"

#include "UI.h"
#include "Toolkit.h"
#include "Mnemonic.h"
#include "Vector.h"
#include "Dialog_sh.h"
#include "Component.h"

#include <string.h>

static unsigned int	mne_prefix;	// alt key modifier
static KeyCode		esc_kc[2];	// escape key codes

// Event Handler for keypress events - expect either
// escape or ALT...
static void
key_press_handler(Widget w, XtPointer client_data, XEvent *xe, 
	Boolean *cont_to_dispatch)
{
	MneInfo		*minfo = (MneInfo *)client_data;
	XKeyEvent	*key = (XKeyEvent *)xe;

	if (!minfo || !key || !cont_to_dispatch)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	if (((key->keycode == esc_kc[0]) || ((esc_kc[1] != NoSymbol) &&
		(key->keycode == esc_kc[1]))) &&
		(w == minfo->get_widget()))
	{
		Component	*c = minfo->get_component();
		if (c->get_type() != DIALOG_SHELL)
			return;
		*cont_to_dispatch = FALSE;
		((Dialog_shell *)c)->handle_escape(xe);
		return;
	}
	if (key->state != mne_prefix)
		// only interested in ALT-...
		return;

	key->state = 0;

	int		num_recs = minfo->num_recs();
	MneInfoRec	**mptr = minfo->mne_info();
	int		i;

	for (i = 0; i < num_recs; i++, mptr++) 
	{
		MneInfoRec	*mrec = *mptr;
		if (mrec->keycode == 1)
			mrec->keycode = XKeysymToKeycode(XtDisplay(w),
				mrec->mne);
		if (mrec->keycode != key->keycode)
			continue;
		
		unsigned short op = mrec->op;
		if (XtIsManaged(mrec->w) &&
		    XtIsSensitive(mrec->w))
		{
			if (op & MNE_PASS_KEY)
				*cont_to_dispatch = True;
			else
				*cont_to_dispatch = False;

			if (op & MNE_GET_FOCUS)
				XmProcessTraversal(mrec->w,
					XmTRAVERSE_CURRENT);

			if (op & MNE_ACTIVATE_BTN)
			{
				if (XmIsGadget(mrec->w))
					XtCallActionProc(
						minfo->get_component()->get_widget(),
						"ManagerGadgetSelect",
						xe, 0, 0);
				else
					XtCallActionProc(mrec->w,
						"ArmAndActivate", 
						xe, 0, 0);
			}
		} 
		else if (op & MNE_KEEP_LOOKING) 
		{
			continue;
		}
		break;
	}
	key->state = mne_prefix;
}

// recursively register event handler for all primitive and manager
// widgets that are children of the dialog shell or manager;
// used for non-grab case
static void
RegisterAllWidgets(Widget w, MneInfo *minfo)
{
	Arg		args[2];
	Cardinal	num_kids;
	WidgetList	kids;

	XtSetArg(args[0], XmNchildren, &kids);
	XtSetArg(args[1], XmNnumChildren, &num_kids);
	XtGetValues(w, args, 2);

	for (int i = 0; i < num_kids; i++) 
	{
		Boolean	is_manager;
		if (XmIsPrimitive(kids[i]) ||
		    (is_manager = XmIsManager(kids[i]))) 
		{
			XtInsertRawEventHandler(
				kids[i], KeyPressMask, False,
				key_press_handler, (XtPointer)minfo, 
				XtListHead);
			if (is_manager)
				RegisterAllWidgets(kids[i], minfo);
		}
	}
}

// find alt key prefix and escape key code(s)
static void
setup_key_info(Widget w)
{
	int		i;
	Display		*dpy = XtDisplay(w);

	KeySym	osf_ks, reg_ks;

	// determine Escape keycode(s)
	if ((reg_ks = XStringToKeysym("Escape")) != NoSymbol &&
		(esc_kc[0] = XKeysymToKeycode(dpy, reg_ks))
		!= NoSymbol) 
	{
		// Determine osfCancel keycode if it's different
		// from Escape
		if ((osf_ks = XStringToKeysym("osfCancel")) != NoSymbol
			&& osf_ks != reg_ks)
			esc_kc[1] = XKeysymToKeycode(dpy, osf_ks);
	}

#define KS	XKeycodeToKeysym(dpy, map->modifiermap[k], 0)

	// Each server has its own map of keysyms used as modifier
	// keys.  We need to find the one for the ALT key.
	// The modifier map is an 8*max_keypermod array.
	XModifierKeymap *map;
	char		*nm;
	register int	j, k;

	map = XGetModifierMapping(dpy);

	mne_prefix = Mod1Mask;	// fall back...
	k = 0;
	for (i = 0; i < 8; i++) 
	{
		for (j = 0; j < map->max_keypermod; j++, k++) 
		{
			if (map->modifiermap[k]) 
			{
				// Search for XK_ALT_L 
				if ((nm = XKeysymToString(KS)) &&
					(strcmp("Alt_L", nm) == 0))
				{
					// the prefix is a bit
					// corresponding to the
					// position in the modifier
					// array - this is used
					// in the KeyEvent's state
					// field.
					mne_prefix = (1 << i);
					break;
				}
			}
		}
		if (j != map->max_keypermod)	// got it 
			break;
	}
#undef KS
}

MneInfo::~MneInfo()
{
	if (num_mne_recs)
	{
		MneInfoRec	**mptr;
		mptr = (MneInfoRec **)mne_recs->ptr();
		for(int i = 0; i < num_mne_recs; i++, mptr++)
		{
			delete *mptr;
		}
	}
	delete mne_recs;
}

void
MneInfo::add_mne_info(Widget w, KeySym m,
	unsigned short operation)
{
	MneInfoRec	*mrec = new MneInfoRec;
	mrec->w = w;
	mrec->op = operation;
	mrec->mne = m;
	mrec->keycode = 1;  // invalid code
	mne_recs->add(&mrec, sizeof(MneInfoRec *));
	num_mne_recs++;
	if (!widget)
		// register_mnemonics not yet called; let it do
		// the work
		return;
	if (with_grab)
	{
		KeyCode		kc;
		if ((kc = XKeysymToKeycode(XtDisplay(widget), m))
			!= NoSymbol)
		{
			XtGrabKey(widget, kc, mne_prefix,
				True, GrabModeAsync, GrabModeAsync);
			mrec->keycode = kc;
		}
	}
	else if (XmIsPrimitive(w))
	{
		XtInsertRawEventHandler(w, KeyPressMask, False,
			key_press_handler, (XtPointer)this, XtListHead);
	}
}

// Assume this is called after mnemonics have been registered.
// For no grab case, we simply change the contents of the
// MneInfoRec.  For the grab case, we ungrab the old mne
// and grab the new.
void
MneInfo::change_mne_info(Widget w, KeySym m,
	unsigned short operation)
{
	MneInfoRec	**mptr = (MneInfoRec **)mne_recs->ptr();
	MneInfoRec	*mrec;
	int		i;

	for(i = 0; i < num_mne_recs; i++, mptr++)
	{
		if (w == (*mptr)->w)
			break;
	}
	if (i >= num_mne_recs)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	mrec = *mptr;
	mrec->op = operation;
	mrec->mne = m;
	if (!with_grab)
	{
		mrec->keycode = 1; // invalid code
		return;
	}

	KeyCode	kc;

	XtUngrabKey(widget, mrec->keycode, mne_prefix);
	if ((kc = XKeysymToKeycode(XtDisplay(widget), m)) != NoSymbol)
	{
		XtGrabKey(widget, kc, mne_prefix,
			True, GrabModeAsync, GrabModeAsync);
		mrec->keycode = kc;
	}
}

void
MneInfo::remove_mne_info(Widget w)
{
	MneInfoRec	**mptr = (MneInfoRec **)mne_recs->ptr();
	MneInfoRec	*mrec;
	Vector		*nrecs;
	int		i, ndx;

	for(ndx = 0; ndx < num_mne_recs; ndx++, mptr++)
	{
		if (w == (*mptr)->w)
			break;
	}
	if (ndx >= num_mne_recs)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	mrec = *mptr;
	if (with_grab)
		XtUngrabKey(widget, mrec->keycode, mne_prefix);

	nrecs = new Vector;
	mptr = (MneInfoRec **)mne_recs->ptr();
	for(i = 0; i < ndx; i++, mptr++)
		nrecs->add(mptr, sizeof(MneInfoRec *));
	mptr++; i++;
	for(; i < num_mne_recs; i++, mptr++)
		nrecs->add(mptr, sizeof(MneInfoRec *));
	delete mrec;
	delete mne_recs;
	mne_recs = nrecs;
	num_mne_recs--;
}

void 
MneInfo::register_mnemonics(Boolean grab)
{
	if (component->get_type() == DIALOG_SHELL)
		widget = ((Dialog_shell *)component)->get_popup_window();
	else
	{
		if (num_mne_recs == 0)
			return;
		widget = component->get_widget();
		while(XmIsManager(XtParent(widget)))
			widget = XtParent(widget);
	}

	if (!mne_prefix)
		setup_key_info(widget);

	// Establish passive grab for escape key(s)
	if (component->get_type() == DIALOG_SHELL)
	{
		XtGrabKey(widget, esc_kc[0], AnyModifier,
			True, GrabModeAsync, GrabModeAsync);
		if (esc_kc[1] != NoSymbol)
			XtGrabKey(widget, esc_kc[1], AnyModifier,
				True, GrabModeAsync, GrabModeAsync);
	}

	if (grab || component->get_type() == DIALOG_SHELL)
		// Event handler for shell or manager widget itself
		XtInsertRawEventHandler(widget, KeyPressMask, False,
			key_press_handler, (XtPointer)this, 
			XtListHead);

	if (!grab)
	{
		RegisterAllWidgets(widget, this);
		return;
	}

	// Establish passive grab for all mnemonic keys with AltMask 
	Display		*dpy = XtDisplay(widget);
	KeyCode		kc;
	MneInfoRec	**mptr = (MneInfoRec **)mne_recs->ptr();

	with_grab = TRUE;
	for (int i = 0; i < num_mne_recs; i++, mptr++) 
	{
		if ((kc = XKeysymToKeycode(dpy, (*mptr)->mne))
			!= NoSymbol)
		{
			XtGrabKey(widget, kc, mne_prefix,
				True, GrabModeAsync, GrabModeAsync);
			(*mptr)->keycode = kc;
		}
	}
}
