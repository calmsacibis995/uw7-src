#ident	"@(#)debugger:gui.d/common/Show_value.C	1.12"

// GUI headers
#include "Base_win.h"
#include "Boxes.h"
#include "Caption.h"
#include "Dialogs.h"
#include "Dialog_sh.h"
#include "Dispatcher.h"
#include "Show_value.h"
#include "Proclist.h"
#include "Stext.h"
#include "Text_area.h"
#include "Text_line.h"
#include "Radio.h"
#include "Toggle.h"
#include "UI.h"
#include "Windows.h"
#include "Window_sh.h"
#include "gui_label.h"

// DEBUG headers
#include "Buffer.h"
#include "Machine.h"
#include "Msgtab.h"
#include "Vector.h"
#include "str.h"

#include <stdio.h>
#include <ctype.h>

// The contents of the expansion_vector is an array of Expansion_data structures,
// each of which represents one member of the class or array displayed in the
// result area.  The type - ET_class_member or ET_array_member - determines
// what is stored in the union.  If the member is itself a class or array,
// sub_type will point to another array of Expansion_data

enum Expansion_type { ET_none, ET_class_member, ET_array_member };

struct Expansion_data
{
	Expansion_type	type;
	long		line;
	Vector		*sub_type;
	Boolean		is_null;

	union
	{
		char	*name;
		long	index;
	};

			Expansion_data() { type = ET_none; line = 0; name = 0;
						sub_type = 0; is_null = FALSE; }
};

// choices in list of formats
#define DEFAULT_FMT	0
#define DECIMAL_FMT	1
#define UNSIGNED_FMT	2
#define OCTAL_FMT	3
#define HEX_FMT		4
#define FLOAT_FMT	5
#define STRING_FMT	6
#define CHAR_FMT	7
#define USER_FMT	8

Show_value_dialog::Show_value_dialog(Base_window *bw, 
	Show_value_dialog *par) : PROCESS_DIALOG(bw)
{
	static const DButton	buttons[] =
	{
		{ B_non_exec, LAB_show_value, LAB_show_value_mne,
			(Callback_ptr)(&Show_value_dialog::show_value) },
		{ B_non_exec, LAB_expand, LAB_expand_mne,
			(Callback_ptr)(&Show_value_dialog::expand) },
		{ B_non_exec, LAB_button_backtrack, LAB_button_backtrack_mne,
			(Callback_ptr)(&Show_value_dialog::backtrack) },
		{ B_non_exec, LAB_peel, LAB_peel_mne,
			(Callback_ptr)(&Show_value_dialog::peel) },
		{ B_close, LAB_none, LAB_none, 
			(Callback_ptr)(&Show_value_dialog::dismiss) },
		{ B_help, LAB_none, LAB_none,0 },
	};

	static const Toggle_data show_toggles[] = {
		{ LAB_verbose, FALSE,
			(Callback_ptr)(&Show_value_dialog::toggle_verbose) },
	};

	// *_FMT values must be kept in sync with this array
	static const LabelId format_choices[] =
	{
		LAB_default,
		LAB_decimal,
		LAB_unsigned,
		LAB_octal,
		LAB_hex,
		LAB_float,
		LAB_string,
		LAB_char,
		LAB_other,
	};


	Expansion_box	*box;
	Expansion_box	*box2;
	Caption		*caption;

	dialog = new Dialog_shell(bw->get_window_shell(),
		LAB_show_value, (Callback_ptr)(&Show_value_dialog::dismiss), this,
		buttons, sizeof(buttons)/sizeof(DButton), HELP_show_value_dialog);
	box = new Expansion_box(dialog, "show_value", OR_vertical);
	component_init(box);

	caption = new Caption(box, LAB_expression_line, CAP_LEFT);
	expression = new Text_line(caption, "expression", "", 25, 1);
	caption->add_component(expression);
	box->add_component(caption);

	verbose = new Toggle_button(box, "toggle", show_toggles,
		sizeof(show_toggles)/sizeof(Toggle_data), 
		OR_horizontal, this);
	box->add_component(verbose);

	box2 = new Expansion_box(box, "show value", OR_horizontal);
	box->add_component(box2, TRUE);

	caption = new Caption(box2, LAB_format, CAP_TOP_LEFT);
	formats = new Radio_list(caption, "formats", OR_vertical, format_choices,
		(sizeof(format_choices)/sizeof(LabelId)), 0,
		(Callback_ptr)(&Show_value_dialog::set_format_type), this);
	caption->add_component(formats, FALSE);
	box2->add_component(caption);

	caption = new Caption(box2, LAB_result_of_expr_line, CAP_TOP_LEFT);
	result = new Text_area(caption, "result", 8, 30, FALSE,
		(Callback_ptr)(&Show_value_dialog::selection_cb), this);
	caption->add_component(result);
	box2->add_component(caption, TRUE);

	caption = new Caption(box, LAB_format_line, CAP_LEFT);
	format_line = new Text_line(caption, "format", "", 20, FALSE);
	caption->add_component(format_line);
	box->add_component(caption);

	box2->update_complete();
	box->update_complete();
	dialog->add_component(box);
	dialog->set_popup_focus(expression);

	window = bw;
	parent = par;
	elevel = 0;
	resbuf = 0;
	line_cnt = 0;
	expr = 0;
	expansion_vector = 0;
	type_name = 0;
	free_list = next_free = 0;
	verbose_state = FALSE;
	selection = FALSE;
	dialog->set_sensitive(LAB_button_backtrack, FALSE);
}

Show_value_dialog::~Show_value_dialog()
{
	while(free_list)
	{
		Show_value_dialog *sd = free_list;
		free_list = free_list->next_free;
		delete(sd);
	}
}

static void
match_string(char *&string, Buffer *buf)
{
	char	end = *string;
	char	c;

	buf->add(end);
	for (string++; (c = *string) != '\0'; string++)
	{
		buf->add(c);
		if (c == '\\')
			buf->add(*++string);
		else if (c == end)
			break;	
	}
}

// protection is needed so that expressions like "-p" won't
// be confused with options to "print".
char *
protect(char *string, Buffer *buf)
{
	if (!string || !*string)
		return 0;

	buf->clear();

	// prescan for simple names
	char	*p = string;
	// skip whitespace
	for (; isspace(*p); ++p)
		;
	// scan til non-alpha
	for (; *p == '_' || isalnum(*p); ++p)
		;
	// skip whitespace
	for (; isspace(*p); ++p)
		;
	if (*p == '\0')
	{
		// if a simple name then no need for protection
		// in fact protection can cause syntax error if name
		// is also a type, since "(name)" will be interpreted
		// as a cast.
		buf->add(string);
		return (char *)*buf;
	}

	buf->add('(');
	for (; string < p; ++string)
		buf->add(*string);

	int	depth = 0;

	for ( ; *string; string++)
	{
		char	c = *string;
		switch (c)
		{
		case '\'':
		case '"':
			match_string(string, buf);
			break;

		case ',':
			if (!depth)
			{
				buf->add(')');
				buf->add(c);
				buf->add('(');
			}
			else
			{
				buf->add(c);
			}
			break;

		case ')':
			if (depth)
				depth--;
			buf->add(c);
			break;

		case '(':
			depth++;
			// fall-through

		default:
			buf->add(c);
			break;
		}
	}

	buf->add(')');
	return (char *)*buf;
}

// Unlike most other Process_dialogs, the Show Value dialog does not
// require a current process or process selection (so the use may view
// debugger or user variables in an empty window set)
void
Show_value_dialog::show_value(Component *, void *)
{
	const char	*fmt;
	char		*exp;
	char		*dash_v;

	clear_expansion(expansion_vector);
	expansion_vector = 0;

	resbuf = buf_pool.get();
	exp = protect(expression->get_text(), resbuf);

	dialog->clear_msg();
	if (!exp || !*exp)
	{
		dialog->error(E_ERROR, GE_no_expression);
		buf_pool.put(resbuf);
		resbuf = 0;
		return;
	}
	if ((fmt = get_format()) == (char *)-1)
	{
		buf_pool.put(resbuf);
		resbuf = 0;
		return;
	}

	// push previous
	expression_vector.add(&expr, sizeof(char *));
	type_name_vector.add(&type_name, sizeof(char *));
	elevel++;
	expr = makestr(exp);
	type_name = 0;

	dash_v = (verbose_state == TRUE) ? "-v" : "";

	result->clear();
	if (fmt && pobjs)
	{
		dispatcher.send_msg(this, pobjs[0]->get_id(),
			"print -f\"%s\\n\" -p %s %s %s\n", fmt,
			make_plist(total, pobjs, 0, level), 
			dash_v, exp);
	}
	else if (fmt)
	{
		dispatcher.send_msg(this, 0, "print -f\"%s\\n\" %s %s\n"
			, fmt, dash_v, exp);
	}
	else if (pobjs)
	{
		dispatcher.send_msg(this, pobjs[0]->get_id(),
			"print -p %s %s %s\n", 
				make_plist(total, pobjs, 0, level),
				dash_v, exp);
	}
	else
	{
		dispatcher.send_msg(this, 0, "print %s %s\n", dash_v,
			exp);
	}
	dialog->wait_for_response();
	resbuf->clear();
}

const char *
Show_value_dialog::get_format()
{
	static char	*buf;
	static size_t	maxlen;
	char		*fmt;
	char		*p1;
	char		*p2;
	size_t		len;

	switch (formats->which_button())
	{
	case DECIMAL_FMT:		return "%d";
	case UNSIGNED_FMT:		return "%u";
	case OCTAL_FMT:			return "%#o";
	case HEX_FMT:			return "%#x";
	case FLOAT_FMT:			return "%g";
	case STRING_FMT:		return "%s";
	case CHAR_FMT:			return "%c";

	case USER_FMT:
		fmt = format_line->get_text();
		if (!fmt || !*fmt)
		{
			dialog->error(E_ERROR, GE_no_format);
			return (char *)-1;
		}

		len = strlen(fmt)+1;
		if (len*2 > maxlen)
		{
			delete buf;
			maxlen = len*2 + 40;	// give some room to grow
			buf = new char[maxlen];
		}

		for (p1 = fmt, p2 = buf; *p1; p1++)
		{
			if (*p1 == '"')
				*p2++ = '\\';
			*p2++ = *p1;
		}
		*p2 = '\0';
		return buf;

	case DEFAULT_FMT:
	default:	return 0;
	}
}

void
Show_value_dialog::dismiss(Component *, void *)
{
	if (parent)
	{
		parent->add_to_free_list(this);
	}
	clear_expansion(expansion_vector);
	expansion_vector = 0;
	dismiss_cb(0, 0);
}

// set_expression is called when the dialog is first popped up,
// to initialize the expression and result
void
Show_value_dialog::set_expression(const char *name)
{
	expression->set_text(name);
}

void
Show_value_dialog::expand(Component *, void *)
{
	char		*dash_v;
	const char	*fmt;
	char		*tmp_expr;
	char		*tmp_type;

	dialog->clear_msg();
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	tmp_type = type_name;
	tmp_expr = expr;

	if (selection)
	{
		expr = make_expression(expr);
	}
	else
	{
		clear_expansion(expansion_vector);
		expansion_vector = 0;
		type_name = 0;
		expr = make_expression(expression->get_text());
	}

	if (!expr ||((fmt = get_format()) == (char *)-1))
	{
		type_name = tmp_type;
		expr = tmp_expr;
		return;
	}

	// push previous
	expression_vector.add(&tmp_expr, sizeof(char *));
	type_name_vector.add(&tmp_type, sizeof(char *));
	elevel++;

	dash_v = (verbose_state == TRUE) ? "-v" : "";

	expression->set_text(expr);

	if (fmt)
		dispatcher.send_msg(this, pobjs[0]->get_id(),
			"print -f\"%s\\n\" -p %s %s %s\n", fmt,
			make_plist(total, pobjs, 0, level), 
			dash_v, expr);
	else
		dispatcher.send_msg(this, pobjs[0]->get_id(),
			"print -p %s %s %s\n", 
				make_plist(total, pobjs, 0, level),
				dash_v, expr);
	dialog->wait_for_response();
	clear_expansion(expansion_vector);
	expansion_vector = 0;
}

static void
do_clear(Vector *vec)
{
	// assert( vec != 0 );
	Expansion_data	*ptr = (Expansion_data *)vec->ptr();
	int		nentries = vec->size()/sizeof(Expansion_data);

	for (int i = 0; i < nentries; i++, ptr++)
	{
		if (ptr->type == ET_class_member)
			delete ptr->name;
		if (ptr->sub_type)
			do_clear(ptr->sub_type);
	}
	delete vec;
}

// clear_expansion frees the space used by the expansion_vector and
// its sub-vectors
void
Show_value_dialog::clear_expansion(Vector *vec)
{
	if (vec)
		do_clear(vec);

	result->clear();
	if (resbuf)
	{
		buf_pool.put(resbuf);
		resbuf = 0;
	}
	// first line contains a '{'
	line_cnt = 1;
}

// make_expression builds up the expression in the current language
// needed to dereference the selected member
// if the selected text can't be expanded, make_expression
// will display an error message and return 0
char *
Show_value_dialog::make_expression(const char *exp_str)
{
	Language	lang = cur_language;

	if (cur_language == UnSpec)
	{
		Message	*msg;

		dispatcher.query(this, pobjs[0]->get_id(),
			"print %%db_lang\n");
		while ((msg = dispatcher.get_response()) != 0)
		{
			if (msg->get_msg_id() == MSG_print_val)
			{
				char *l;
				msg->unbundle(l);
				if (strcmp(l, "\"C\"\n") == 0)
					lang = C;
				else if (strcmp(l, "\"C++\"\n") == 0)
					lang = CPLUS;
			}
		}
	}

	switch(lang)
	{
	case C:
	case CPLUS:
		return CC_expression(exp_str);

	case UnSpec:
	default:
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return NULL;
	}
}

// CC_expression builds up an expression from the previous expression
// and the current selection, and checks that the resulting expression
// is valid and can be dereferenced
char *
Show_value_dialog::CC_expression(const char *exp_str)
{
	Buffer	*buf = buf_pool.get();
	char	*selection = result->get_selection();
	char	*string;
	int	indirection = (*exp_str == '*');

	if (indirection && expansion_vector)
		exp_str++;
	buf->clear();
	buf->add('*');
	if (type_name)
	{
		// type_name is set only when printing a derived class 
		// when given the base class ptr
		buf->add("((");
		buf->add(type_name);
		buf->add("*)");
		buf->add(exp_str);
		buf->add(')');
	}
	else if (*exp_str == '*')
	{
		// parens needed for pointer-to-pointer
		buf->add('(');
		buf->add(exp_str);
		buf->add(')');
	}
	else
		buf->add(exp_str);

	if (expansion_vector)
	{
		// a class can't be expanded further without selecting
		// a member
		if (selection && *selection)
		{
			int	line = result->get_current_position();

			if (!CC_member_name(line, buf, expansion_vector, indirection))
			{
				buf_pool.put(buf);
				dialog->error(E_ERROR, GE_cant_expand);
				return 0;
			}
		}
		else
		{
			dialog->error(E_ERROR, GE_cant_expand);
			buf_pool.put(buf);
			return 0;
		}
	}
	else if (selection && *selection)
	{
		if (formats->which_button() != DEFAULT_FMT)
		{
			dialog->error(E_ERROR, GE_cant_expand);
			buf_pool.put(buf);
			return 0;
		}
	}

	string = makestrn((char *)*buf, buf->size());
	buf_pool.put(buf);
	return string;
}

static void
add_member(Buffer *buf, Expansion_data *ptr, int indirection)
{
	char		number[MAX_LONG_DIGITS+3];

	if (ptr->type == ET_class_member)
	{
		if (indirection)
			buf->add("->");
		else
			buf->add('.');
		buf->add(ptr->name);
	}
	else
	{
		sprintf(number, "[%d]", ptr->index);
		buf->add(number);
	}
}

// CC_member_name builds up the string of ->, ., and [] operators needed
// to reference the selected member
int
Show_value_dialog::CC_member_name(int line, Buffer *buf, Vector *vector, int indirection)
{
	if (!vector)
		return 0;

	Expansion_data	*ptr = (Expansion_data *)vector->ptr();
	int		nentries = vector->size()/sizeof(Expansion_data);

	for (int i = 0; i < nentries; i++, ptr++)
	{
		if (ptr->sub_type)
		{
			if (i < nentries-1 && line >= (ptr+1)->line)
				continue;

			if (line > ptr->line)
			{
				// line is in this subtree
				if (ptr->name)
				{
					// ptr->name will be 0 for members
					// of anonymous unions
					add_member(buf, ptr, indirection);
					indirection = 0;
				}
				return CC_member_name(line, buf, ptr->sub_type,
							indirection);
			}
			else
				return 0;
		}

		if (line == ptr->line)
		{
			if (ptr->is_null)
			{
				dialog->error(E_ERROR, GE_deref_null);
				return 0;
			}
			if (ptr->name)
				add_member(buf, ptr, indirection);
			return 1;
		}
		if (line < ptr->line)
			// line not found?
			return 0;
	}

	return 0;
}

void
Show_value_dialog::de_message(Message *m)
{
	Expansion_data	data;
	Expansion_stack	*es;
	Msg_id		mtype = m->get_msg_id();
	char		*name;
	char		*spaces;
	char		*val;
	char		*prefix;
	Word		index;

	if (!resbuf)
	{
		resbuf = buf_pool.get();
		resbuf->clear();
	}

	switch (mtype)
	{
	case ERR_assume_size:
		dialog->error(m);
		return;

	case MSG_start_class:
	case MSG_start_array:
		// If expansion_vector is already set, this indicates that there is
		// a nested structure or array.  The current expansion_vector is
		// pushed onto the stack, and a new expansion_vector is built for
		// the sub-aggregate, until the matching MSG_val_close_brack
		// is received
		// Note: the only purpose of the stack is to keep track of the
		//  nesting levels
		if (expansion_vector)
		{
			Expansion_data	*ptr = (Expansion_data *)expansion_vector->ptr();
			int		index = expansion_vector->size()/sizeof(data) - 1;

			Vector	*vector = new Vector();

			ptr[index].sub_type = vector;

			es = new Expansion_stack;
			es->v = expansion_vector;
			stack.push((Link *)es);
			expansion_vector = vector;
		}
		else	
			expansion_vector = new Vector;
		break;

	case MSG_val_close_brack:
		if (!expansion_vector)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
		if (!stack.is_empty())
		{
			es = (Expansion_stack *)stack.pop();
			expansion_vector = es->v;
		}
		break;

	case MSG_print_member1:
	case MSG_print_member3:
		if (!expansion_vector)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
		m->unbundle(spaces, prefix, name, val);
		data.name = makesf(strlen(prefix) + strlen(name),
				"%s%s", prefix, name);
		data.line = line_cnt;
		data.type = ET_class_member;
		if (strcmp(val, "0x0") == 0 || strcmp(val, "NULL") == 0)
			data.is_null = TRUE;
		expansion_vector->add(&data, sizeof(data));
		break;
	case MSG_print_member2:
		if (!expansion_vector)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
		m->unbundle(spaces, name, val);
		data.name = makestr(name);
		data.line = line_cnt;
		data.type = ET_class_member;
		if (strcmp(val, "0x0") == 0 || strcmp(val, "NULL") == 0)
			data.is_null = TRUE;
		expansion_vector->add(&data, sizeof(data));
		break;

	case MSG_print_member4:
		// anonymous unions
		if (!expansion_vector)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
		data.name = 0;
		data.line = line_cnt;
		data.type = ET_class_member;
		data.is_null = FALSE;
		expansion_vector->add(&data, sizeof(data));
		break;

	case MSG_array_member:
		if (!expansion_vector)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
		m->unbundle(spaces, index, val);
		data.type = ET_array_member;
		data.index = (long)index;
		data.line = line_cnt;
		if (strcmp(val, "0x0") == 0 || strcmp(val, "NULL") == 0)
			data.is_null = TRUE;
		expansion_vector->add(&data, sizeof(data));
		break;

	case MSG_type:
		m->unbundle(name);
		type_name = makestr(name);
		break;
		
	default:
		if (Mtable.msg_class(mtype) == MSGCL_error)
		{
			show_error(m, TRUE);
			return;
		}
		else if (gui_only_message(m) || 
			(mtype == MSG_proc_output))
		{
			return;
		}
		break;
	}
	add_text(m->format());
}

void
Show_value_dialog::add_text(const char *string)
{
	resbuf->add(string);

	char	c;
	while ((c = *string++) != '\0')
	{
		if (c == '\n')
			line_cnt++;
	}
}

void
Show_value_dialog::cmd_complete()
{
	result->add_text((char *)*resbuf);
	result->position(1);	// scroll back to top of text
	buf_pool.put(resbuf);
	resbuf = 0;

	if (!stack.is_empty())
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		while (!stack.is_empty())
			(void) stack.pop();
	}
	if (elevel >= 2)
		dialog->set_sensitive(LAB_button_backtrack, TRUE);
	else
		dialog->set_sensitive(LAB_button_backtrack, FALSE);
	dialog->cmd_complete();
}

// backtrack pops the stack of expressions, and redisplays the previous
// expression
void
Show_value_dialog::backtrack(Component *, void *)
{
	char		*dash_v;
	const char	*fmt;

	if (elevel < 2)
		return;

	dialog->clear_msg();
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	if ((fmt = get_format()) == (char *)-1)
	{
		return;
	}
	dash_v = (verbose_state == TRUE) ? "-v" : "";

	delete expr;
	delete type_name;
	elevel--;
	expr = ((char **)expression_vector.ptr())[elevel];
	type_name = ((char **)type_name_vector.ptr())[elevel];
	expression_vector.drop(sizeof(char *));
	type_name_vector.drop(sizeof(char *));
	expression->set_text(expr);

	if (fmt)
		dispatcher.send_msg(this, pobjs[0]->get_id(),
			"print -f\"%s\\n\" -p %s %s %s\n", fmt,
			make_plist(total, pobjs, 0, level), 
			dash_v, expr);
	else
		dispatcher.send_msg(this, pobjs[0]->get_id(),
			"print -p %s %s %s\n", 
				make_plist(total, pobjs, 0, level),
				dash_v, expr);
	dialog->wait_for_response();
	clear_expansion(expansion_vector);
	expansion_vector = 0;
}

// Don't let the user try to dereference a null pointer
int
Show_value_dialog::pointer_ok(const char *str)
{
	dispatcher.query(this, pobjs[0]->get_id(),
		"print %s\n", str);

	Message	*msg;
	int	ret = 1;
	while ((msg = dispatcher.get_response()) != 0)
	{
		if (msg->get_msg_id() == MSG_print_val)
		{
			char	*ptr_value;
			msg->unbundle(ptr_value);
			if (strcmp(ptr_value, "0x0\n") == 0
				|| strcmp(ptr_value, "NULL\n") == 0)
			{
				dialog->error(E_ERROR, GE_deref_null);
				ret = 0;
			}
		}
		else if (Mtable.msg_class(msg->get_msg_id()) == MSGCL_error)
		{
			dialog->error(msg);
			ret = 0;
		}
	}
	return ret;
}

void
Show_value_dialog::set_obj(Boolean reset)
{
	// make sure this dialog is ALWAYS sensitive!
	if (reset)
		dialog->set_sensitive(TRUE);
	if (elevel < 2)
		dialog->set_sensitive(LAB_button_backtrack, FALSE);
}

// Whenever the current object stops, update the result pane to
// show the new value of the expression
void
Show_value_dialog::update_obj(ProcObj *p, Reason_code rc)
{
	char		*dash_v;
	const char	*fmt;

	if (p->is_running() || p->is_animated() || resbuf
		|| rc == RC_fcall_in_expr)
		return;
	dialog->clear_msg();
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}
	if (!expr ||((fmt = get_format()) == (char *)-1))
	{
		return;
	}

	dash_v = (verbose_state == TRUE) ? "-v" : "";
	if (fmt)
		dispatcher.send_msg(this, pobjs[0]->get_id(),
			"print -f\"%s\\n\" -p %s %s %s\n", fmt,
			make_plist(total, pobjs, 0, level), 
			dash_v, expr);
	else
		dispatcher.send_msg(this, pobjs[0]->get_id(),
			"print -p %s %s %s\n", 
				make_plist(total, pobjs, 0, level),
				dash_v, expr);
	dialog->wait_for_response();
	clear_expansion(expansion_vector);
	expansion_vector = 0;
}

void
Show_value_dialog::selection_cb(Text_area *, void *cdata)
{
	if ((int)cdata)
		selection = TRUE;
	else
		selection = FALSE;

}

// called when verbosity toggle changes
void
Show_value_dialog::toggle_verbose(Component *, void *state)
{
	verbose_state = ((int)state == 0) ? FALSE : TRUE;
	if (pobjs)
		update_obj(pobjs[0], RC_change_state);
}

void
Show_value_dialog::set_format_type(Component *, void *)
{
	int which = formats->which_button();
	if (which == USER_FMT)
	{
		format_line->set_editable(TRUE);
		dialog->set_focus(format_line);
	}
	else
	{
		format_line->set_editable(FALSE);
		dialog->set_focus(expression);
	}
	dialog->set_sensitive(LAB_expand, (which == DEFAULT_FMT));
}

void
Show_value_dialog::peel(Component *, void *)
{
	Show_value_dialog	*sd;
	if (!expr)
		return;
	if (free_list)
	{
		sd = free_list;
		free_list = sd->next_free;
		sd->next_free = 0;
	}
	else
		sd = new Show_value_dialog(window, this);
	sd->set_plist(window, window_set->get_command_level());
	sd->set_expression(expr);
	sd->show_value(0, 0);
	sd->display();
}

void
Show_value_dialog::add_to_free_list(Show_value_dialog *child)
{
	child->next_free = free_list;
	free_list = child;
}
