#ifndef SHOW_VALUE_H
#define SHOW_VALUE_H
#ident	"@(#)debugger:gui.d/common/Show_value.h	1.4"

// GUI headers
#include "Dialogs.h"
#include "Component.h"
#include "Reason.h"

// DEBUG headers
#include "Vector.h"
#include "Link.h"

class Buffer;
class Base_window;
class Message;
class ProcObj;
class Radio_list;
class Toggle_button;
class Text_line;

// The Show_value_dialog displays the values of valid debugger 
// expressions.
// It also allows the user to expand the value of a structure or array
// pointer.
// The user can further expand displayed pointers by selecting the line
// containing the pointer and pushing the Expand button.  The Show_value dialog
// uses the expansion_vector to track which lines contain pointers that can
// be expanded.  Since the information displayed is not necessarily flat,
// but may contain nested structures and arrays, the expansion_vector is a
// tree which may contain pointers to other vectors.  The stack is used to
// build up the tree as the messages are read from the debugger.
// As pointers are expanded, the expressions are stored in the expression_vector,
// so the previous expression can be retrieved easily, instead of having to
// be built up again from scratch when Collapse is pushed.
// As the messages making up the output of the print statement are read
// from the debugger, the text is stored in resbuf, and only written to
// result at the end, to avoid excessive traffic to the screen

struct Expansion_stack : public Stack
{
	Vector		*v;
};

class Show_value_dialog : public Process_dialog
{
	Base_window	*window;
	Text_line	*expression;
	Text_line	*format_line;
	Text_area	*result;
	char		*expr;
	int		elevel;
	Vector		expression_vector;
	Vector		*expansion_vector;
	Expansion_stack	stack;
	Buffer		*resbuf;
	Radio_list	*formats;
	int		line_cnt;
	char		*type_name;
	Vector		type_name_vector;
	Toggle_button	*verbose;
	Show_value_dialog *free_list;
	Show_value_dialog *next_free;
	Show_value_dialog *parent;
	Boolean		verbose_state;
	Boolean		selection;

	char		*CC_expression(const char *expression);
	int		CC_member_name(int, Buffer *, Vector *, int indirection);

	void		clear_expansion(Vector *);
	void		add_text(const char *);
	int		pointer_ok(const char *expr);
	const char	*get_format();
	void		add_to_free_list(Show_value_dialog *);

public:
			Show_value_dialog(Base_window *, Show_value_dialog *parent);
			~Show_value_dialog();

	char		*make_expression(const char *expression);
	void		set_expression(const char *name);

			// callbacks
	void		show_value(Component *, void *);
	void		expand(Component *, void *);
	void		backtrack(Component *, void *);
	void		dismiss(Component *, void *);
	void		toggle_verbose(Component *, void *);
	void		selection_cb(Text_area *, void *);
	void		set_format_type(Component *, void *);
	void		peel(Component *, void *);

			// functions overriding those from Dialog_box
	void		de_message(Message *);
	void		cmd_complete();
	void		update_obj(ProcObj *, Reason_code);
	void		set_obj(Boolean);
};

extern char	*protect(char *string, Buffer *buf);

#endif
