#ifndef _Mnemonic_h_
#define _Mnemonic_h_

#ident	"@(#)debugger:libmotif/common/Mnemonic.h	1.5"

#include "UI.h"
#include "Vector.h"

// Class to handle the management of mnemonics from all
// push buttons, toggle buttons and radio buttons defined
// for a given dialog or button bar. We maintain a list of all button
// widgets/gadgets and their mnemonics. 
// Two methods of implementation are provided: with and without
// grab.  (We currently use the with grab method).
// In the first method, we setup a passive keyboard 
// grab for each mnemonic in the nearest enclosing shell (for a
// dialog) or the highest level parent manager widget.
// We establish an event handler for keypress events in the
// shell or manager.
// In the second method, we register a keypress event handler
// for each child widget of the dialog or closest manager
// to the buttons.
// In either case, the event handler checks for the
// ALT key and then for each mnemonic in the list.  If
// the mnemonic matches, the associated widget is armed
// and activated.
//
// We also catch the escape key within a dialog and call
// the dialog's handle_escape routine.

// Constraints:
// a. For the no-grab case, you must call register_mnemonics 
// after all children are created, otherwise it will not be able to
// catch all children within the shell; also see b.
// b. For the no-grab case, ther is a problem if a dialog or 
// one of its components needs to destroy/create object(s)
// on demand.
// c. This class does not handle the mnemonic visuals, they come
// from Motif/libXm (usually, XmLabel subclasses).

// Note: - it's difficult to say that whether its better to use
// the grab or no grab version of register_mnemonics from 
// the performance perspective..
// With grab version puts passive grabs on the dialog shell, so
// the startup time may be slower if num_mne_recs is large (each
// causes 3 server round trips, I think). However, the advantage
// is that the caller will not need to worry about the constraints
// a and/or b.

// Bit values for the op field of MneInfoRec.
#define MNE_NOOP		0
#define MNE_ACTIVATE_BTN	1	// arm and activate button
#define MNE_GET_FOCUS		2	// move focus to button
#define MNE_KEEP_LOOKING	4	// if button insensitive, look
					// for another candidate
#define MNE_PASS_KEY		8	// pass keystroke on to application

//  The MneInfoRec is used to store mnemonic information for an
// object (widget/gadget)
struct MneInfoRec {
	Widget		w;
	KeySym		mne;
	unsigned char	op;
	KeyCode		keycode;
};

class Component;

// keep track of and register mnemonic information
class MneInfo {
	Component		*component;
	Widget			widget; // shell or top manager
	Vector			*mne_recs;
	short 			num_mne_recs;
	Boolean			with_grab;
public:
				MneInfo(Component *c) {
					component = c;
					mne_recs = new Vector(); 
					widget = 0;
					num_mne_recs = 0;
					with_grab = 0; }
				~MneInfo();
	void			add_mne_info(Widget, KeySym mne,
					unsigned short op);
	void			remove_mne_info(Widget);
	void			change_mne_info(Widget btn, 
					KeySym new_mne,
					unsigned short op);
	void			register_mnemonics(Boolean grab);
	int			num_recs() { return num_mne_recs; }
	Component		*get_component() { return component; }
	Widget			get_widget() { return widget; }
	MneInfoRec		**mne_info() { return (MneInfoRec **)mne_recs->ptr(); }
};

#endif
