#ifndef	_STEXT_H
#define	_STEXT_H
#ident	"@(#)debugger:gui.d/common/Stext.h	1.2"

#include "Component.h"
#include "StextP.h"

class Simple_text : public Component
{
	SIMPLE_TEXT_TOOLKIT_SPECIFICS

public:
			Simple_text(Component *parent, const char *text,
				Boolean resize, Help_id help_msg = HELP_none);
			~Simple_text();

	void		set_text(const char *text);	// changes the display
};

#endif	// _STEXT_H
