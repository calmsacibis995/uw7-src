#ifndef	_TEXT_AREA_H
#define	_TEXT_AREA_H
#ident	"@(#)debugger:gui.d/common/Text_area.h	1.7"

#include "Component.h"
#include "Text_areaP.h"

// A Text_area is a scrollable window of multi-line text
// editable specifies whether or not the user may edit the text

// Framework callbacks:
// there is one callback function for getting or losing the selection
// the callback is invoked as
//	selection:	creator->function((Text_area *)this, 1)
//	deselection:	creator->function((Text_area *)this, 0)

// Text_area derived virtually from component becasuse
// Text_display is derived from both Text_area and Text_editor,
// both of which are components
class Text_area : public virtual Component
{
	TEXT_AREA_TOOLKIT_SPECIFICS

protected:
	Callback_ptr	select_cb;
	short		nrows;
	short		npages;

public:
			Text_area(Component *, const char *name,
				int rows, int columns, Boolean editable = FALSE,
				Callback_ptr select_cb = 0,
				Command_sender *creator = 0, Help_id help_msg = HELP_none, int pages = 0);
			~Text_area();

			// editing functions
	void		copy_selection();	// copy selection to the clipboard
	char		*get_selection();	// return selection string
	char		*get_text();		// returns entire contents
	void		add_text(const char *);	// append to the end
	void		position(int line);	// scroll the given line into view
	void		clear();		// blanks out display

			// access functions
	int		get_current_position();
	void		set_highlight(int, Boolean);
#ifdef SDE_SUPPORT
	void		set_editable(Boolean);
	char		*get_current_line();
#endif
};

#endif	// _TEXT_AREA_H
