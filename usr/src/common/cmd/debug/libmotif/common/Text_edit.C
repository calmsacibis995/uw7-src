#ident	"@(#)debugger:libmotif/common/Text_edit.C	1.6"

// GUI headers
#include "UI.h"
#include "Toolkit.h"
#include "Component.h"
#include "Text_edit.h"
#include "NewHandle.h"

#include <Xm/Form.h>
#include <Xm/DrawingA.h>

#include "str.h"
#include "Machine.h"
#include "Link.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

// Functions shared by Text_editor and Text_terminal
// A Text_editor consists of a form widget with
// a drawing area at the left to draw arrows for current
// line, stop signs for breakpoints and, optionally, line numbers

// arrow_bits and stop_bits are compact, machine-generated descriptions
// of the bits that describes the bitmap image of the arrow and stop signs.
// x_height and x_width give the dimensions, in pixels, of the bitmap

// arrow definition
#define arrow_width 18
#define arrow_height 9
static unsigned char arrow_bits[] = {
   0x00, 0x10, 0x00, 0x00, 0x30, 0x00, 0x00, 0x70, 0x00, 0xfe, 0xff, 0x00,
   0xfe, 0xff, 0x01, 0xfe, 0xff, 0x00, 0x00, 0x70, 0x00, 0x00, 0x30, 0x00,
   0x00, 0x10, 0x00};

// stop sign definition
#define stop_width 11
#define stop_height 11
static unsigned char stop_bits[] = {
   0xf8, 0x00, 0xfc, 0x01, 0x06, 0x03, 0xf7, 0x07, 0xf7, 0x07, 0x07, 0x07,
   0x7f, 0x07, 0x7f, 0x07, 0x06, 0x03, 0xfc, 0x01, 0xf8, 0x00};

static Pixmap stop_sign;
static Pixmap arrow;

static void
exposeCB(Widget, Text_editor *pane, XmDrawingAreaCallbackStruct *)
{
	if (!pane)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	pane->redraw();
}

static void
Toggle_break(Widget w, XButtonEvent *event, String */*params*/, Cardinal */*nparams*/)
{
	Text_editor	*text = 0;

	XtVaGetValues(w, XmNuserData, &text, 0);
	if (!text || !event)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	text->toggle_bkpt(event->x, event->y);
}

#define PAD	2

Text_editor::Text_editor(Component *p, const char *name,
	Command_sender *c, Base_window *b, int nrows, int ncolumns,
	Callback_ptr tbcb, Help_id h)
{
	// fill in Component values - Component is a virtual base class
	parent = p;
	label = name;
	creator = c;
	help_msg = h;

	bw = b;
	rows = nrows;
	columns = ncolumns;
	breaks = 0;
	search_expr = 0;
	last_line = 0;
	empty = TRUE;
	curr_file = 0;
	curr_temp = 0;
	stored_files = 0;
	sfile_changed = FALSE;
	current_line = 0;
	top = 1;
	gc_base = gc_stop = 0;
	toggle_break_cb = tbcb;

	widget = XtVaCreateManagedWidget(name, xmFormWidgetClass, 
		parent->get_widget(),
		0);
	// initialize x positions for stop and arrow signs
	x_stop = PAD;
	x_arrow = x_stop + stop_width + PAD;
	// width of drawing area is initially width of stop sign + 
	// width of arrow + padding. it is dynamically resized for
	// source windows depending on the number of lines of text
	canvas = XtVaCreateManagedWidget("text_canvas", xmDrawingAreaWidgetClass,
		widget,
		XmNwidth, x_arrow + arrow_width + PAD,
		XmNborderWidth, 1,
		XmNrightAttachment, XmATTACH_NONE,
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNuserData, this,
		0);

	XtAddCallback(canvas, XmNexposeCallback, 
		(XtCallbackProc)exposeCB, (XtPointer)this);

	if (toggle_break_cb)
	{
		String			trans = "<Btn1Down>:	Toggle_break()";
		static XtActionsRec	acts[] = {
			{ "Toggle_break",	(XtActionProc)Toggle_break },
		};

		XtAppAddActions(base_context, acts, XtNumber(acts));
		XtVaSetValues(canvas,
			XmNtranslations, XtParseTranslationTable(trans), 0);
	}
	if (help_msg)
		register_help(canvas, 0, help_msg);
}

Text_editor::~Text_editor()
{
	clear_breaks();
	delete (char *)search_expr;
	delete (char *)curr_file;
	if (gc_base)
		XFreeGC(XtDisplay(canvas), gc_base);
	if (gc_stop)
		XFreeGC(XtDisplay(canvas), gc_stop);
}

void 
Text_editor::clear_breaks()
{
	Breaklist	*bptr = breaks;
	while(bptr)
	{
		breaks = bptr->next();
		delete bptr;
		bptr = breaks;
	}
	breaks = 0;
}

void
Text_editor::set_breaklist(int *breaklist)
{
	clear_breaks();
	if (breaklist)
	{
		for (int i = 0; breaklist[i]; i++)
			set_stop(breaklist[i]);
	}
}

Boolean
Text_editor::has_breakpoint(int line)
{
	Breaklist	*bptr = breaks;
	for(; bptr; bptr = bptr->next())
	{
		if (bptr->get_line() == line)
			return 1;
	}
	return 0;
}

// compute the width of canvas according to number of lines of text
void
Text_editor::size_canvas(Boolean def)
{
	// assert(!empty && is_source)
	int ndigits;

	if (!def)
	{
		ndigits = 0;
		for (int i = get_last_line(); i > 0; i /= 10)
			++ndigits;
	}
	else
		ndigits = 5;
	x_stop = PAD + ndigits*XTextWidth(xfont, "0", 1) + PAD;
	x_arrow = x_stop + stop_width + 2;
	XtVaSetValues(canvas, 
		XmNwidth, x_arrow + arrow_width + PAD, 
		0);
}

// 'offset' is line offset from top; starts at 0
// x, y coordinates of characters are at baseline
// 'baseline' member is baseline of first row
void
Text_editor::draw_number(int line, int offset)
{
	char	buf[MAX_INT_DIGITS+1];

	sprintf(buf, "%d", line);
	int width = XTextWidth(xfont, buf, strlen(buf));
	// numbers are right justified
	int x = x_stop - PAD - width;
	int y = baseline + font_height * offset;

	if (!gc_base)
		set_gc();
	XDrawString(XtDisplay(canvas), XtWindow(canvas), gc_base,
		x, y, buf, strlen(buf));
}

// calculate y coordinate of arrow or stop sign - argument
// is height in pixels of arrow or stop sign
// x,y coordinates for arrow and stop sign are upper left corner
// whereas x,y coordinates for characters are at baseline
// 'offset' is line offset from top; starts at 0
inline int
Text_editor::calculate_y(int element_ht, int offset)
{
	return (baseline - xfont->ascent + (font_height * offset)
		+ ((font_height - element_ht)/2));
}

// 'offset' is line offset from top; starts at 0
void
Text_editor::draw_arrow(int offset)
{
	int	y = calculate_y(arrow_height, offset);

	if (!gc_base)
		set_gc();
	if (!arrow)
	{
		if ((arrow = XCreateBitmapFromData(XtDisplay(canvas), 
			XtWindow(canvas), 
			(char *)arrow_bits, arrow_width, arrow_height)) == 0)
			display_msg(E_ERROR, GE_internal, __FILE__, 
				__LINE__);
	}

	XCopyPlane(XtDisplay(canvas), arrow, XtWindow(canvas), gc_base, 0, 0,
			arrow_width, arrow_height, x_arrow, y, 1);
}

// erase_arrow assumes calling function has checked that the arrow 
// is visible,
void
Text_editor::erase_arrow(int offset)
{
	int	y = calculate_y(arrow_height, offset);

	XClearArea(XtDisplay(canvas), XtWindow(canvas),
		x_arrow, y, arrow_width, arrow_height, FALSE);
}

// 'offset' is line offset from top; starts at 0
void
Text_editor::draw_stop(int offset)
{
	int	y = calculate_y(stop_height, offset);

	if (!gc_base)
		set_gc();
	if (!stop_sign)
	{
		if ((stop_sign = XCreateBitmapFromData(XtDisplay(canvas),
			XtWindow(canvas), (char *)stop_bits, 
			stop_width, stop_height)) == 0)
			display_msg(E_ERROR, GE_internal, __FILE__, 
				__LINE__);
	}

	XCopyPlane(XtDisplay(canvas), stop_sign, XtWindow(canvas), 
		gc_stop, 0, 0, stop_width, stop_height, x_stop, y, 1);
}

void
Text_editor::erase_stop(int offset)
{
	int	y = calculate_y(stop_height, offset);
	XClearArea(XtDisplay(canvas), XtWindow(canvas),
		x_stop, y, stop_width, stop_height, FALSE);
}

void
Text_editor::set_gc()
{
	XGCValues	gc_values;
	unsigned long	gc_mask;
	Pixel		foreground;
	Pixel		background;
	Pixel		red;
	Display		*dpy = XtDisplay(canvas);
	Window		win = XtWindow(canvas);

	XtVaGetValues(canvas,
		XmNforeground, &foreground, 
		XmNbackground, &background,
		0);

	gc_values.foreground = foreground;
	gc_values.background = background;
	gc_values.font = xfont->fid;
	gc_mask = GCFont | GCForeground | GCBackground;
	gc_base = XCreateGC(dpy, win, gc_mask, &gc_values);

	if (is_color() && ((red = get_color("red")) != 0))
		gc_values.foreground = red;
	gc_stop = XCreateGC(dpy, win, gc_mask, &gc_values);
}

void
Text_editor::clear_canvas()
{
	int canvas_width = 0, canvas_height = 0;

	XtVaGetValues(canvas, 
		XmNwidth, &canvas_width,
		XmNheight, &canvas_height,
		0);
	XClearArea(XtDisplay(canvas), XtWindow(canvas), 0, 0,
		canvas_width, canvas_height, FALSE);
}

static char	*tmp_prefix;

int
Text_editor::save_current_to_temp()
{
	char		*tmpfile;
	StoredFiles	*sf;

	if (!curr_file || !sfile_changed)
	{
		return 1;
	}

	// create temporary file name
	if (!tmp_prefix)
	{
		pid_t	pid = getpid();
		tmp_prefix = new char[6]; // max size of prefix is 5
		if (pid >= 100000)
		{
			pid = pid % 100000;
		}
		sprintf(tmp_prefix, "%d", pid);
	}
	if ((tmpfile = tempnam(0, tmp_prefix)) == 0)
	{
		newhandler.invoke_handler();
		return 0;
	}

	if (!save_current(tmpfile))
	{
		delete tmpfile;
		return 0;
	}

	sf = new StoredFiles(curr_file, tmpfile, stored_files);
	stored_files = sf;
	sfile_changed = FALSE;
	return 1;
}

// if current file has unsaved changes, save to a temp file;
// if we have saved version of new file, use that, else
// use new file
int
Text_editor::set_new_file(const char *path)
{
	StoredFiles	*sf, *prev = 0;
	int		ret;

	if (curr_file)
	{
		if (strcmp(curr_file, path) == 0)
			return 1;
		if (sfile_changed && !save_current_to_temp())
		{
			return 0;
		}
	}

	sfile_changed = FALSE;

	for (sf = stored_files; sf; prev = sf, sf = sf->get_next())
	{
		if (sf->is_file(path))
			break;
	}
	if (sf)
	{
		// read from saved temporary
		ret = set_file(sf->get_saved());
		delete curr_temp;
		curr_temp = makestr(sf->get_saved());

		sfile_changed = TRUE;

		if (!prev)
			stored_files = sf->get_next();
		else
			prev->set_next(sf->get_next());
		delete sf;
	}
	else
	{
		ret = set_file(path);
		delete curr_temp;
		curr_temp = 0;
		sfile_changed = FALSE;
	}
	if (ret)
	{
		delete (char *)curr_file;
		curr_file = makestr(path);
	}
	return ret;
}

int
Text_editor::save_current_as(const char *path)
{
	if (!save_current(path))
	{
		return 0;
	}
	sfile_changed = FALSE;
	return 1;
}

// discard changes to the current file; re-start with
// original version
int
Text_editor::discard_changes()
{
	if (!curr_file || !sfile_changed)
		return 1;
	if (set_file(curr_file))
	{
		delete curr_temp;
		curr_temp = 0;
		sfile_changed = FALSE;
		return 1;
	}
	else
		return 0;
}

// discard changes to saved temporary version of file
void
Text_editor::discard_changes(StoredFiles *savef)
{
	StoredFiles	*sf, *prev = 0;
	for (sf = stored_files; sf; prev = sf, sf = sf->get_next())
	{
		if (sf == savef)
			break;
	}
	if (!sf)
		return;
	unlink(sf->get_saved());
	if (!prev)
		stored_files = sf->get_next();
	else
		prev->set_next(sf->get_next());
	delete sf;
}

int
Text_editor::save_stored_file(StoredFiles *savef, char *fname)
{
	StoredFiles	*sf, *prev = 0;
	struct stat	new_stat, saved_stat;
	char		*endp;

	for (sf = stored_files; sf; prev = sf, sf = sf->get_next())
	{
		if (sf == savef)
			break;
	}
	if (!sf)
	{
		display_msg(E_ERROR, GE_internal, __FILE__,
			__LINE__);
		return 0;
	}

	if ((endp = strrchr(fname, '/')) != 0)
	{
		*endp = 0;
		if (stat(fname, &new_stat) == -1)
		{
			display_msg(E_ERROR, GE_cant_open,
				fname, strerror(errno));
			return 0;
		}
		*endp = '/';
	}
	else
	{
		if (stat(".", &new_stat) == -1)
		{
			display_msg(E_ERROR, GE_cant_open,
				".", strerror(errno));
			return 0;
		}
	}
	if (stat(sf->get_saved(), &saved_stat) == -1)
	{
		display_msg(E_ERROR, GE_open_source,
			sf->get_saved(), strerror(errno));
		return 0;
	}
	if (saved_stat.st_dev == new_stat.st_dev)
	{
		// same filesystem; just rename
		if (rename(sf->get_saved(), fname) == 0)
		{
			return 1;
		}
	}
	// different filesystems, or rename failed,
	// try it by hand
	FILE	*sptr, *nptr;
	char	buf[BUFSIZ];
	int	last = 0;

	if ((nptr = fopen(fname, "w")) == 0)
	{
		display_msg(E_ERROR, GE_create_source,
			fname, strerror(errno));
		return 0;
	}
	if ((sptr = fopen(sf->get_saved(), "r")) == 0)
	{
		fclose(nptr);
		unlink(fname);
		display_msg(E_ERROR, GE_open_source,
			sf->get_saved(), strerror(errno));
		return 0;
	}
	while(!last)
	{
		int len;
		if ((len = fread(buf, 1, BUFSIZ, sptr)) < BUFSIZ)
			last = 1;
		if (fwrite(buf, 1, len, nptr) != len)
		{
			display_msg(E_ERROR, GE_source_file_write,
				fname, strerror(errno));
			fclose(sptr);
			fclose(nptr);
			unlink(fname);
			return 0;
		}
	}
	fclose(nptr);
	fclose(sptr);
	unlink(sf->get_saved());
	if (!prev)
		stored_files = sf->get_next();
	else
		prev->set_next(sf->get_next());
	delete sf;
	return 1;
}
