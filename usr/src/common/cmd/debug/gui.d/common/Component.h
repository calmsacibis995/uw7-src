#ifndef	_COMPONENT_H
#define _COMPONENT_H
#ident	"@(#)debugger:gui.d/common/Component.h	1.4"

#include "Toolkit.h"
#include "Help.h"
#include "Sender.h"

// cfront 2.1 requires class name in constructor, 1.2 doesn't accept it
#ifdef __cplusplus
#define COMPONENT	Component
#else
#define COMPONENT
#endif

class Message;
class Base_window;
class Window_set;

// Components - all are derived from Component base class
class Alert_shell;
class Box;
class Caption;
class Component;
class Dialog_shell;
class Divided_box;
class Expansion_box;
class Menu;
class Menu_bar;
class Packed_box;
class Radio_list;
class Selection_list;
class Simple_text;
class Slider;
class Table;
class Text_area;
class Text_display;
class Text_line;
class Toggle_button;
class Window_shell;

enum Component_type
{
	WINDOW_SHELL,
	DIALOG_SHELL,
	OTHER
};

enum Select_mode
{
	SM_single,
	SM_multiple,
};

// Identifies type of the current selection - only one type
// is allowed at a time per window
enum Selection_type
{
	SEL_none,
	SEL_text,		// text in the Transcript pane
	SEL_process,		// Process pane
	SEL_event,		// Either event pane
	SEL_src_line,		// Source window
	SEL_frame,		// Stack pane
	SEL_symbol,		// Symbol pane
	SEL_regs,		// Register pane
	SEL_instruction,	// Disassembly pane
};

enum Orientation
{
	OR_horizontal,
	OR_vertical
};

class Component
{
protected:
	Widget		widget;
	const char	*label;
	Component	*parent;
	Command_sender	*creator;	// framework object, used by callbacks
	Help_id		help_msg;
	Component_type	type;

public:
		Component(Component *p, const char *l, Command_sender *c, Help_id h,
				Component_type t = OTHER) 
			{ parent = p; label = l; creator = c; help_msg = h; type = t; }
		Component() { type = OTHER; }
	virtual	~Component();

			// access functions
	Widget		get_widget()		{ return widget; }
	Component	*get_parent()		{ return parent; }
	Command_sender	*get_creator()		{ return creator; }
	Help_id		get_help_msg()		{ return help_msg; }
	Component_type	get_type()		{ return type; }
	const char	*get_label()		{ return label; }

			// walk up the tree to find the base window
	Base_window	*get_base();
	virtual void	*get_client_data();
};

#endif // _COMPONENT_H
