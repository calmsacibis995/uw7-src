#ident	"@(#)debugger:libmotif/common/Text_line.C	1.6"

#include "UI.h"
#include "Component.h"
#include "Text_line.h"
#include "str.h"

#include <Xm/TextF.h>

#include <wchar.h>
#include <wctype.h>

// callback called when user hits return
static void
activateCB(Widget w, Text_line *ptr, XmAnyCallbackStruct *)
{
	Callback_ptr	cb;
	Command_sender	*creator;
	char 		*string = 0;

	if (!ptr)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	XtVaGetValues(w, XmNvalue, &string, 0);
	creator = ptr->get_creator();
	cb = ptr->get_return_cb();
	if (cb && creator)
		(creator->*cb)(ptr, string);
}

// A Text_line is simply Motif TextField widget

Text_line::Text_line(Component *p, const char *s, const char *text, int width,
	Boolean edit, Callback_ptr ret, Command_sender *c, Help_id h,
	Boolean)
	: Component(p, s, c, h)
{
	return_cb = ret;
	editable = edit;
	string = 0;
	if (!width)
		width = 10;

	widget = XtVaCreateManagedWidget(label, xmTextFieldWidgetClass,
		parent->get_widget(), 
		XmNcolumns, width,
		XmNeditable, edit,
		XmNsensitive, edit,
		XmNvalue, text,
		XmNuserData, this,
		0);

	if (return_cb)
		XtAddCallback(widget, XmNactivateCallback, 
			(XtCallbackProc)activateCB, (XtPointer)this);

	if (help_msg)
		register_help(widget, label, help_msg);
}

Text_line::~Text_line()
{
	delete string;
}

// blank out the display
void
Text_line::clear()
{
	XtVaSetValues(widget, XmNvalue, "", 0);
}

// returns the current contents.  The toolkit makes a copy of the string.
// Text_line frees the space when through with it
char *
Text_line::get_text()
{
	delete string;
	XtVaGetValues(widget, XmNvalue, &string, 0);
	return string;
}

// strip leading non-printables; break at first trailing non-printable
static char *
strip_text(const char *str)
{
	wchar_t	*w1, *w2;
	wchar_t	*wptr;
	int len = mbstowcs(0, str, len);
	len++;
	wptr = new wchar_t[len];

	mbstowcs(wptr, str, len);
	for(w1 = wptr; *w1; w1++)
	{
		if (iswprint(*w1))
		{
			break;
		}
	}
	if (!*w1)
	{
		delete wptr;
		return makestr("");
	}
	for(w2 = w1 + 1; *w2; w2++)
	{
		if (!iswprint(*w2))
		{
			*w2 = 0;
			break;
		}
	}
	len = wcstombs(0, w1, len);
	len++;
	char	*newstr = new char[len];
	wcstombs(newstr, w1, len);
	delete wptr;
	return newstr;
}

void
Text_line::set_text(const char *s)
{
	if (!s || !*s)
		XtVaSetValues(widget, XmNvalue, "", 0);
	else
	{
		// text field does not like non-printable chars
		char *p = strip_text(s);
		XtVaSetValues(widget, XmNvalue, p, 0);
		delete p;
	}
}

void
Text_line::set_cursor(int pos)
{
	XtVaSetValues(widget, XmNcursorPosition, (XmTextPosition)pos, 0);
	XmProcessTraversal(widget, XmTRAVERSE_CURRENT);
}

void
Text_line::set_editable(Boolean e)
{
	if (e == editable)
		return;

	editable = e;
	XtVaSetValues(widget, 
		XmNeditable, e, 
		XmNsensitive, e, 
		0);
}

void
Text_line::set_sensitive(Boolean s)
{
	XtVaSetValues(widget, XmNsensitive, s, 0);
}
