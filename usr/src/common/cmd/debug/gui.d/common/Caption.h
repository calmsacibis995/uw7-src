#ifndef	_CAPTION_H
#define	_CAPTION_H
#ident	"@(#)debugger:gui.d/common/Caption.h	1.5"

#include "Component.h"
#include "CaptionP.h"
#include "gui_label.h"

// The string passed to the Caption constructor is used to label the caption's child
enum Caption_position
{
	CAP_LEFT,
	CAP_TOP_LEFT,
	CAP_TOP_CENTER
};

class Caption : public Component
{
	CAPTION_TOOLKIT_SPECIFICS

private:
	Component       *child;

public:
			Caption(Component *parent, LabelId caption,
				Caption_position, Help_id help_msg = HELP_none);
			~Caption();

			// accepts one child only
			// all sub-children must already
			// be created
	void		add_component(Component *, Boolean resizable = TRUE);
	void		set_label(const char *label);	// changes the label
	void		remove_child();
	void		set_sensitive(Boolean);
};

#endif	// _CAPTION_H
