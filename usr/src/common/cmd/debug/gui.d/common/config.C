#ident	"@(#)debugger:gui.d/common/config.C	1.26"

#include "Resources.h"
#include "Severity.h"
#include "str.h"
#include "config.h"
#include "Base_win.h"
#include "Windows.h"
#include "Buttons.h"
#include "Button_bar.h"
#include "Label.h"
#include "Menu.h"
#include "Window_sh.h"
#include "Path.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#ifdef MULTIBYTE
#include <wctype.h>
#endif

Window_descriptor	*window_descriptor;
Window_configuration	*window_configuration;
char			*config_directory;
int			windows_per_set;
int			max_rows = 10;

static Severity			config_sev;
static Window_descriptor	*last_window;
static Menu_descriptor		*last_mref;

static char *
do_toupper(char *s)
{
	char 	*ptr = s;
	for(; *ptr; ptr++)
		*ptr = toupper(*ptr);
	return s;
}

int
Window_descriptor::add_pane(Pane_descriptor *pane)
{
	if (flags & pane->get_type())
		// already a pane of this type
		return 0;

	flags |= pane->get_type();
	if (pane->get_type() == PT_command)
	{
		if (pane->get_rows() > max_rows)
			max_rows = pane->get_rows();
		cmd_panes_per_set++;
	}

	Pane_descriptor	*p = panes;
	for(; p; p = p->next())
	{
		if (!p->next())
			break;
	}
	if (!p)
		panes = pane;
	else
		pane->append(p);
	npanes++;
	return 1;
}

void
Window_descriptor::add_menu(Menu_descriptor *menu)
{
	Menu_descriptor	*m = menus;
	for(; m; m = m->next())
	{
		if (!m->next())
			break;
	}
	if (!m)
		menus = menu;
	else 
		menu->append(m);
}

void
Window_descriptor::add_button_bar(Bar_descriptor *bar)
{
	Bar_descriptor	*b;
	if (bar->is_bottom())
		b = bottom_button_bars;
	else
		b = top_button_bars;
	for(; b; b = b->next())
	{
		if (!b->next())
			break;
	}
	if (!b)
	{
		if (bar->is_bottom())
			bottom_button_bars = bar;
		else
			top_button_bars = bar;
	}
	else
		bar->append(b);
}

void
Window_descriptor::remove_button_bar(Bar_info *bi)
{
	int	index = 0;
	Bar_descriptor *old_desc = bi->old_bdesc;
	Bar_descriptor *first = bi->bottom ?
		bottom_button_bars : top_button_bars;
	Bar_descriptor	*p = (Bar_descriptor *)old_desc->prev();
	Bar_descriptor	*n = old_desc->next();
	for(; first != old_desc; first = first->next())
		index++;
	old_desc->unlink();
	if (!p)
	{
		if (bi->bottom)
			bottom_button_bars = n;
		else
			top_button_bars = n;
	}
	bi->old_index = index;
	bi->new_wbar = bi->bottom ? bottom_button_bars : top_button_bars;
}

void
Window_descriptor::replace_button_bar(Bar_info *bi)
{
	int	index = 0;
	Bar_descriptor *old_desc = bi->old_bdesc;
	Bar_descriptor *new_desc = bi->new_bdesc;
	Bar_descriptor *first = bi->bottom ?
		bottom_button_bars : top_button_bars;
	Bar_descriptor	*n = old_desc->next();
	Bar_descriptor	*p = (Bar_descriptor *)old_desc->prev();
	for(; first != old_desc; first = first->next())
		index++;
	old_desc->unlink();
	if (n)
		new_desc->prepend(n);
	else if (p)
		new_desc->append(p);
	if (!p)
	{
		if (new_desc->is_bottom())
			bottom_button_bars = new_desc;
		else
			top_button_bars = new_desc;
	}
	bi->old_index = index;
	bi->new_wbar = bi->bottom ? bottom_button_bars : top_button_bars;
}

Bar_descriptor::Bar_descriptor(Bar_descriptor *bar)
{
	Button	*btn;
	nbuttons = 0;
	buttons = 0;
	bottom = bar->bottom;
	for(btn = bar->buttons; btn; btn = btn->next())
	{
		Button	*nbtn = new Button(btn);
		add_button(nbtn);
	}
}

void
Bar_descriptor::add_button(Button *btn)
{
	Button	*b = buttons;
	for(; b; b = b->next())
	{
		if (!b->next())
			break;
	}
	if (!b)
		buttons = btn;
	else
		btn->append(b);
	nbuttons++;
}

void
Bar_descriptor::remove_all()
{
	while(buttons)
	{
		Button *btn = buttons->next();
		delete buttons;
		buttons = btn;
	}
	nbuttons = 0;
}

void
Bar_descriptor::remove_button(CButtons btype, const char *name)
{
	Button	*btn;
	if ((btn = get_button(btype, name)) == 0)
		return;
	if (btn == buttons)
		buttons = btn->next();
	btn->unlink();
	delete(btn);
	nbuttons--;
}

void
Bar_descriptor::replace_button(Button *oldb, Button *newb)
{
	Button	*p = (Button *)oldb->prev();
	Button	*n = (Button *)oldb->next();
	oldb->unlink();
	delete oldb;
	if (p)
		newb->append(p);
	else
	{
		buttons = newb;
		if (n)
			newb->prepend(n);
	}
}

void
Menu_descriptor::add_button(Button *button)
{
	Button	*b = buttons, *prev = 0;
	for(; b; prev = b, b = b->next())
		;
	if (prev)
		button->append(prev);
	else 
		buttons = button;
	nbuttons++;
}

static Button *
make_button(const Button_descriptor *btn)
{
	Button		*button;
	Button_core	*bcore;
	const char	*bname = 0;
	mnemonic_t	mne = 0;

	bcore = find_button(btn->button_id);
	if (btn->button_id != B_separator)
	{
		bname = makestr(labeltab.get_label(btn->button_name));
		if (btn->button_mne != LAB_none)
			mbtowc(&mne, labeltab.get_label(btn->button_mne), MB_CUR_MAX);
	}
	button = new Button(bname, mne, bcore);
	return button;
}

Bar_descriptor *
make_bar_descriptor(Boolean bottom, const Button_descriptor *btn, 
	int nbuttons)
{
	Bar_descriptor	*bd = new Bar_descriptor(bottom);
	for(int i = 0; i < nbuttons; i++, btn++)
	{
		Button		*button = make_button(btn);
		bd->add_button(button);
	}
	return bd;
}

Button *
Bar_descriptor::get_button(CButtons btype, const char *wname)
{
	Button	*b = buttons;
	for(; b; b = b->next())
	{
		if (b->get_type() == btype)
		{
			if ((btype != B_popup) ||
				strcmp(wname, b->get_name()) == 0)
				return b;
		}
	}
	return 0;
}

static Window_descriptor *
find_wdesc(const char *wname)
{
	Window_descriptor	*wptr = window_descriptor;

	for (; wptr; wptr = (Window_descriptor *)wptr->next())
	{
		if (strcmp(wname, wptr->get_name()) == 0)
			return wptr;
	}
	return 0;
}

// recursively search menus and sub-menus
static Menu_descriptor *
find_menu(Menu_descriptor *md, const char *name)
{
	Button	*btn;
	if (strcmp(md->get_mname(), name) == 0)
		return md;
	btn = md->get_buttons();
	for(; btn; btn = btn->next())
	{
		Menu_descriptor *mret;
		if (btn->get_sub_table())
		{
			if ((mret = find_menu(btn->get_sub_table(), 
				name)) != 0)
			{
				last_mref = mret;
				return mret;
			}
		}
			
	}
	return 0;
}

static Menu_descriptor *
find_menu(Window_descriptor *wdesc, const char *name)
{
	Menu_descriptor	*menu, *md;
	Pane_descriptor	*pdesc;

	if (!last_mref)
		return 0;
	if (strcmp(last_mref->get_mname(), name) == 0)
		return last_mref;
	menu = wdesc->get_menus();
	for(; menu; menu = menu->next())
	{
		if ((md = find_menu(menu, name)) != 0)
		{
			last_mref = md;
			return md;
		}
	}
	pdesc = wdesc->get_panes();
	for(; pdesc; pdesc = pdesc->next())
	{
		if ((menu = pdesc->get_menu_descriptor()) != 0)
		{
			if ((md = find_menu(menu, name)) != 0)
			{
				last_mref = md;
				return md;
			}
		}
	}
	return 0;
}

static void
build_windows_menu(Button *btn)
{
	Menu_descriptor		*md;
	Window_descriptor	*wd = window_descriptor;
	const char		*name = makestr(btn->get_name());
	Button_core		*bcore = find_button(B_popup);
	int			i;
	mnemonic_t		*mnemonics = new mnemonic_t[windows_per_set];
	char			*mname = makestr("windows_menu");

	md = new Menu_descriptor(mname, name, btn->get_mne());
	for(i = 0; wd; i++, wd = wd->next())
	{
		// secondary src windows are used for new_source
		// only, they don't appear in the Windows menu
		if (wd->get_flags() & PT_second_src)
			continue;
		Button	*nbtn;
		name = makestr(wd->get_name());
		// assign mnemonic, checking for duplicates
		const char	*current = name;
#ifdef MULTIBYTE
		wchar_t		c, cupper;
		int		len;
		while((len = mbtowc(&c, current, MB_CUR_MAX)) > 0)
		{
			current += len;
			cupper = towupper(c);
#else
		char	c, cupper;
		while((c = *current++) != 0)
		{
			cupper = toupper(c);
#endif
			int dup = 0;
			for(int j = 0; j < i; j++)
			{
				if (mnemonics[j] == cupper)
				{
					dup = 1;
					break;
				}
			}
			if (!dup)
				break;
		}
		// mnemonic could be 0 if no conflict-free choice found
		mnemonics[i] = cupper;
		nbtn = new Button(name, c, bcore);
		nbtn->set_udata((void *)i);
		md->add_button(nbtn);
	}
	btn->set_sub_table(md);
	delete mnemonics;
}

// Check the following:
// 1. At least 1 pane of every type, except status and second_source
// 2. Each button that requires a given pane is in a window with
//      that pane.
// 3. If a button is of type POPUP, verify that there is a
//    window with the same name as the button.
// Also, if Windows button present, build Windows menu.

static int
verify_buttons(Button *btn, Window_descriptor *wdesc)
{
	for(; btn; btn = btn->next())
	{
		if (!(btn->get_panes_req() & wdesc->get_flags()))
		{
			display_msg(config_sev, GE_config_button,
				btn->get_name(), wdesc->get_name());
			return 0;
		}
		if (btn->get_type() == B_popup)
		{
			Window_descriptor	*wd = window_descriptor;
			for(int j = 0; wd; j++, wd = wd->next())
			{
				if (strcmp(wd->get_name(), btn->get_name()) == 0)
				{
					btn->set_udata((void *)j);
					break;
				}
			}
				
			if (!wd)
			{
				display_msg(config_sev, GE_config_popup_match2,
					wdesc->get_name(), btn->get_name());
				return 0;
			}
		}
		else if (btn->get_type() == B_windows)
		{
			if (!btn->get_sub_table())
				build_windows_menu(btn);
		}
		else if (btn->get_sub_table())
		{
			if (!verify_buttons(btn->get_sub_table()->get_buttons(),
				wdesc))
				return 0;
		}
	}
	return 1;
}

// set default rows, columns if not given in pane_descriptor
// columns has no affect on panes containing tables
static void
check_pane_size(Pane_descriptor *pdesc)
{
	int	rows = pdesc->get_rows();
	int	cols = pdesc->get_cols();

	if (rows != 0 && cols != 0)
		return;

	switch(pdesc->get_type())
	{
		case PT_process:
			if (rows == 0)
				rows = 5;
			break;
		case PT_stack:
			if (rows == 0)
				rows = 5;
			break;

		case PT_symbols:
			if (rows == 0)
				rows = 8;
			break;

		case PT_registers:
			if (rows == 0)
				rows = 6;
			if (cols == 0)
				cols = 70;
			break;

		case PT_status:
			rows = 1;
			break;

		case PT_event:
			if (rows == 0)
				rows = 8;
			break;

		case PT_command:
			if (rows == 0)
				rows = 10;
			if (cols == 0)
				cols = 60;
			break;

		case PT_disassembler:
			if (rows == 0)
				rows = 10;
			if (cols == 0)
				cols = 70;
			break;

		case PT_source:
		case PT_second_src:
			if (rows == 0)
				rows = 10;
			if (cols == 0)
				cols = 60;
			break;
	}
	pdesc->set_rows(rows);
	pdesc->set_cols(cols);
}

#define PANE_TYPE(I)	(1 << (I))

static int 
verify_config()
{
	int			i;
	int			ret = 1;
	Window_descriptor	*wd = window_descriptor;
	unsigned long		flags = 0;

	for(; wd; wd = wd->next())
	{
		Menu_descriptor	*md;
		Bar_descriptor	*bd;
		Pane_descriptor	*pd;

		flags |= wd->get_flags();

		for(md = wd->get_menus(); md; md = md->next())
		{
			if (!verify_buttons(md->get_buttons(), wd))
				return 0;
		}
		for(bd = wd->get_top_button_bars(); bd; bd = bd->next())
		{
			if (!verify_buttons(bd->get_buttons(), wd))
				return 0;
		}
		for(bd = wd->get_bottom_button_bars(); bd; bd = bd->next())
		{
			if (!verify_buttons(bd->get_buttons(), wd))
				return 0;
		}
		for(pd = wd->get_panes(); pd; pd = pd->next())
		{
			check_pane_size(pd);
		}
	}
	for (i = 0; i < N_PANES; i++, flags >>= 1)
	{
		int	f = flags & 1;
		if (!f)
		{
			int	t = PANE_TYPE(i);
			if (t != PT_status)
			{
				display_msg(config_sev, GE_config_no_pane,
					pane_name(t));
				ret = 0;
			}
		}
	}
	return ret;
}

static void 
cleanup_menu(Menu_descriptor *md)
{
	Button	*btn, *nbtn;

	btn = md->get_buttons();
	while(btn)
	{
		nbtn = btn->next();
		if (btn->get_sub_table())
		{
			cleanup_menu(btn->get_sub_table());
			delete btn->get_sub_table();
		}
		delete btn;
		btn = nbtn;
	}
}

static void
cleanup_button_bar(Bar_descriptor *bd)
{
	while(bd)
	{
		Button	*btn, *nbtn;
		Bar_descriptor *nbd = bd->next();
		btn = bd->get_buttons();
		while(btn)
		{
			nbtn = btn->next();
			delete btn;
			btn = nbtn;
		}
		delete bd;
		bd = nbd;
	}
}


static void
cleanup_config()
{
	Window_descriptor	*wd = window_descriptor;
	while(wd)
	{
		Window_descriptor	*nwd = wd->next();;
		Pane_descriptor	*pd = wd->get_panes();
		while(pd)
		{
			Pane_descriptor	*npd = pd->next();
			delete pd;
			pd = npd;
		}
		cleanup_button_bar(wd->get_top_button_bars());
		cleanup_button_bar(wd->get_bottom_button_bars());
		Menu_descriptor *md = wd->get_menus();
		while(md)
		{
			Menu_descriptor *nmd = md->next();
			cleanup_menu(md);
			delete md;
			md = nmd;
		}
		delete wd;
		wd = nwd;
	}
	window_descriptor = 0;
	windows_per_set = 0;
	last_window = 0;
	last_mref = 0;
}

// Config parse and lexing routines.

enum Ctoken {
	T_invalid,
	T_eof,
	T_autoload,
	T_bottom,
	T_buttons,
	T_flags,
	T_lcurly,
	T_menu,
	T_menu_bar,
	T_number,
	T_name,
	T_panes,
	T_rcurly,
	T_top,
};

struct Token {
	Ctoken	type;
	long	line;
	char	*name;
	int	val;
};

static Token	curtok;

struct Keyword {
	const char	*kname;
	Ctoken		ktoken;
};

// names must be sorted alphabetically
static const Keyword	keytable[] = {
	{  "Buttons",  	T_buttons },
	{  "Flags",  	T_flags },
	{  "Menu",  	T_menu },
	{  "MenuBar", 	T_menu_bar },
	{  "Panes",  	T_panes },
	{  "autoload",  T_autoload },
	{  "bottom",  	T_bottom },
	{  "top",  	T_top },
	{  0,		T_invalid }
};

class Lexer {
	const char	*fname;
	unsigned char	*buf;
	size_t		bsize;
	char		*namebuf;
	size_t		namesize;
	long		curline;
	unsigned char	*cur;
	int	nextc() { 	int	ctmp;
				if ((cur - buf) >= bsize)
					ctmp = -1;
				else
					ctmp = *cur++;
				return ctmp;  }
	void	pushback() { if ((cur - buf) < bsize) cur--; }
public:
		Lexer(const char *config, const char *path);
		~Lexer();
	void	nexttok();	// assigns to curtok;
	void	syntax_error();
	const char *get_fname() { return fname; }
};

Lexer::Lexer(const char *config, const char *path)
{
	fname = path;
	bsize = strlen(config);
	buf = (unsigned char *)config;
	cur = buf;
	curline = 1;
	namesize = 100;
	namebuf = new char[namesize];
}

Lexer::~Lexer()
{
	delete namebuf;
}

static const Keyword *
getkey(unsigned char *name)
{
	const Keyword	*k = &keytable[0];
	for(; k->kname != 0; k++)
	{
		int	i;
		i = strcmp((char *)name, k->kname);
		if (i == 0)
			return k;
		else if (i < 0)
			break;
	}
	return 0;
}

void
Lexer::syntax_error()
{
	display_msg(config_sev, GE_config_file_syntax, 
		fname, curtok.line);
}

void
Lexer::nexttok()
{
	int		c;
	unsigned char	*first;
	int		count;

	c = nextc();

	while(isspace(c))
	{
		if (c == '\n')
			curline++;
		c = nextc();
	}
	
	while(c == '#' || c == '!')
	{
		// comment
		do {
			c = nextc();
		} while(c != '\n' && c != -1);
		while(isspace(c))
		{
			if (c == '\n')
				curline++;
			c = nextc();
		}
	}

	if (c == -1)
	{
		curtok.type = T_eof;
		return;
	}

	if (c == '{')
	{
		curtok.type = T_lcurly;
		curtok.line = curline;
		return;
	}
	else if (c == '}')
	{
		curtok.type = T_rcurly;
		curtok.line = curline;
		return;
	}
	else if (isdigit(c))
	{
		int	val;

		val = c - '0';
		c = nextc();
		while(isdigit(c))
		{
			val *= 10;
			val += c - '0';
			c = nextc();
		}
		pushback();
		curtok.type = T_number;
		curtok.val = val;
		curtok.line = curline;
		return;
	}
	else if (c == '\'' || c == '\"')
	{
		// name
		int	quote;

		quote = c;
		first = cur;
		count = 0;
		c = nextc();
		while(c != quote)
		{
			if (c == -1 || c == '\n')	
			{
				curtok.type = T_invalid;
				curtok.line = curline;
				return;
			}
			count++;
			c = nextc();
		}
		if (count >= namesize)
		{
			delete namebuf;
			namesize = count + 100;
			namebuf = new char[namesize];
		}
		strncpy((char *)namebuf, (const char *)first, count);
		namebuf[count] = 0;
		curtok.type = T_name;
		curtok.name = namebuf;
		curtok.line = curline;
		return;
	}
	// keyword or name
	first = cur-1;
	count = 1;
	c = nextc();
	while(c != -1 && !isspace(c) && c != '\n')
	{
		count++;
		c = nextc();
	}
	if (count >= namesize)
	{
		delete namebuf;
		namesize = count + 100;
		namebuf = new char[namesize];
	}
	strncpy((char *)namebuf, (const char *)first, count);
	namebuf[count] = 0;
	pushback();

	const Keyword	*k;
	if ((k = getkey((unsigned char *)namebuf)) != 0)
	{
		curtok.type = k->ktoken;
		curtok.name = (char *)k->kname;
	}
	else
	{
		curtok.type = T_name;
		curtok.name = namebuf;
	}
	curtok.line = curline;
}

static int
parse_menu(Lexer *lex, Window_descriptor *wdesc)
{
	Menu_descriptor		*menu;

	lex->nexttok();
	if (curtok.type != T_name)
	{
		lex->syntax_error();
		return 0;
	}
	if ((menu = find_menu(wdesc, curtok.name)) == 0)
	{
		display_msg(config_sev, GE_config_menu_name,
			wdesc->get_name(), curtok.name);
		return 0;
	}
	lex->nexttok();
	if (curtok.type != T_lcurly)
	{
		lex->syntax_error();
		return 0;
	}
	lex->nexttok();
	while(curtok.type == T_name)
	{
		Button_core	*bcore;
		const char	*bname;
		mnemonic_t	mne = 0;
		Button		*button;
		char		*cmd;

		bname = makestr(curtok.name);
		lex->nexttok();
		if ((curtok.type == T_name) && (*curtok.name == '_'))
		{
#ifdef MULTIBYTE
			mbtowc(&mne, &curtok.name[1], MB_CUR_MAX);
#else
			mne = (mnemonic_t)curtok.name[1];
#endif
			lex->nexttok();
		}
		if (curtok.type != T_name)
		{
			delete bname;
			lex->syntax_error();
			return 0;
		}
		if ((bcore = find_button(curtok.name)) == 0)
		{
			delete bname;
			display_msg(config_sev, GE_config_button_name,
				wdesc->get_name(), curtok.name);
				return 0; 
		} 
		CButtons bt = bcore->button_type;
		lex->nexttok();
		if (bt == B_separator)
		{
			delete bname;
			button = new Button(0, 0, bcore); 
		}
		else
			button = new Button(bname, mne, bcore); 
		if (bt == B_exec_cmd || bt == B_debug_cmd)
		{
			if (curtok.type != T_name)
			{
				delete button;
				display_msg(config_sev, GE_config_need_cmd, 
					wdesc->get_name(), bname);
				return 0;
			}
			cmd = makestr(curtok.name);
			button->set_udata((void *)cmd);
			lex->nexttok();
		}
		else if (button->get_type() == B_menu)
		{
			if (curtok.type != T_name)
			{
				delete button;
				display_msg(config_sev, GE_config_sub_menu, 
					wdesc->get_name(), bname);
				return 0;
			}
			char	*mname = makestr(curtok.name);
			Menu_descriptor	*sub_menu =
				new Menu_descriptor(mname, bname, mne);
			button->set_sub_table(sub_menu);
			lex->nexttok();
		}
		menu->add_button(button);
	}
	if (curtok.type != T_rcurly)
	{
		lex->syntax_error();
		return 0;
	}
	return 1;
}

static int
parse_buttons(Lexer *lex, Window_descriptor *wdesc)
{
	Boolean		bottom = FALSE;
	Bar_descriptor	*bar;

	lex->nexttok();
	if (curtok.type == T_top)
	{
		lex->nexttok();
	}
	else if (curtok.type == T_bottom)
	{
		bottom = TRUE;
		lex->nexttok();
	}
	if (curtok.type != T_lcurly)
	{
		lex->syntax_error();
		return 0;
	}

	bar = new Bar_descriptor(bottom);
	wdesc->add_button_bar(bar);

	lex->nexttok();
	while(curtok.type == T_name)
	{
		
		Button_core	*bcore;
		const char	*bname;
		mnemonic_t	mne = 0;
		Button		*button;
		char		*cmd;

		bname = makestr(curtok.name);
		lex->nexttok();
		if ((curtok.type == T_name) && (*curtok.name == '_'))
		{
#ifdef MULTIBYTE
			mbtowc(&mne, &curtok.name[1], MB_CUR_MAX);
#else
			mne = (mnemonic_t)curtok.name[1];
#endif
			lex->nexttok();
		}
		if (curtok.type != T_name)
		{
			delete bname;
			lex->syntax_error();
			return 0;
		}
		if ((bcore = find_button(curtok.name)) == 0)
		{
			delete bname;
			display_msg(config_sev, GE_config_button_name,
				wdesc->get_name(), curtok.name);
			return 0;
		}
		if ((bcore->button_type == B_menu) ||
			(bcore->button_type == B_separator))
		{
			delete bname;
			display_msg(config_sev, GE_config_bar_btn_type, 
				wdesc->get_name(), curtok.name);
			return 0;
		}
		button = new Button(bname, mne, bcore);
		lex->nexttok();
		CButtons	bt = bcore->button_type;
		if (bt == B_exec_cmd || bt == B_debug_cmd)
		{
			if (curtok.type != T_name)
			{
				delete button;
				display_msg(config_sev, GE_config_need_cmd, 
					wdesc->get_name(), bname);
				return 0;
			}
			cmd = makestr(curtok.name);
			button->set_udata((void *)cmd);
			lex->nexttok();
		}
		bar->add_button(button);
	}
	if (curtok.type != T_rcurly)
	{
		lex->syntax_error();
		return 0;
	}
	return 1;
}

static int
parse_menu_bar(Lexer *lex, Window_descriptor *wdesc)
{
	lex->nexttok();
	if (curtok.type != T_lcurly)
	{
		lex->syntax_error();
		return 0;
	}
	lex->nexttok();
	while(curtok.type == T_name)
	{
		Menu_descriptor	*menu;
		const char	*bname, *mname;
		mnemonic_t	mne = 0;

		bname = makestr(curtok.name);
		lex->nexttok();
		if ((curtok.type == T_name) && (*curtok.name == '_'))
		{
#ifdef MULTIBYTE
			mbtowc(&mne, &curtok.name[1], MB_CUR_MAX);
#else
			mne = (mnemonic_t)curtok.name[1];
#endif
			lex->nexttok();
		}
		if (curtok.type != T_name)
		{
			delete bname;
			lex->syntax_error();
			return 0;
		}
		mname = makestr(curtok.name);
		menu = new Menu_descriptor(mname, bname, mne);
		wdesc->add_menu(menu);
		last_mref = menu;
		lex->nexttok();
	}
	if (curtok.type != T_rcurly)
	{
		lex->syntax_error();
		return 0;
	}
	return 1;
}

static int
parse_panes(Lexer *lex, Window_descriptor *wdesc)
{
	int	curline;

	lex->nexttok();
	if (curtok.type != T_lcurly)
	{
		lex->syntax_error();
		return 0;
	}
	lex->nexttok();
	while(curtok.type == T_name)
	{
		Pane_descriptor	*pdesc;
		int		rows = 0, columns = 0;
		char		*pane_name = makestr(curtok.name);
		int		ptype;
		Menu_descriptor	*md;

		curline = curtok.line;
		if ((ptype = find_pane_type(do_toupper(pane_name)))
			== 0)
		{
			display_msg(config_sev, GE_config_pane_type, 
				wdesc->get_name(), pane_name);
			delete pane_name;
			return 0;
		}
		lex->nexttok();
		if (curtok.type == T_number)
		{
			rows = curtok.val;
			lex->nexttok();
			if (curtok.type == T_number)
			{
				columns = curtok.val;
				lex->nexttok();
			}
		}
		if ((curtok.type == T_name) && (curtok.line == curline))
		{
			// menu name
			const char *mname = makestr(curtok.name);
			md = new Menu_descriptor(mname, makestr(""), 0);
			last_mref = md;
			lex->nexttok();
		}
		else 
			md = 0;
		pdesc = new Pane_descriptor(ptype, rows, columns, md);
		if (!wdesc->add_pane(pdesc))
		{
			display_msg(config_sev, 
				GE_config_duplicate_panes2,
				wdesc->get_name(), pane_name);
			delete pane_name;
			delete pdesc;
			return 0;
		}
		delete pane_name;
	}
	if (curtok.type != T_rcurly)
	{
		lex->syntax_error();
		return 0;
	}
	return 1;
}

static int
parse_flags(Lexer *lex, Window_descriptor *wdesc)
{
	lex->nexttok();
	if (curtok.type != T_lcurly)
	{
		lex->syntax_error();
		return 0;
	}
	lex->nexttok();
	if (curtok.type == T_autoload)
	{
		wdesc->set_flag(W_AUTO_POPUP);
		lex->nexttok();
	}
	if (curtok.type != T_rcurly)
	{
		lex->syntax_error();
		return 0;
	}
	return 1;
}

static int
parse_window(Lexer *lex, const char *name)
{
	const char		*wname;
	Window_descriptor	*wdesc;

	wname = makestr(name);
	wdesc = new Window_descriptor(wname);

	if (last_window)
		wdesc->append(last_window);
	else
		window_descriptor = wdesc;
	last_window = wdesc;
	windows_per_set++;

	for(;;)
	{
		lex->nexttok();
		switch(curtok.type)
		{
		case T_eof:
			return 1;
		case T_flags:
			if (!parse_flags(lex, wdesc))
				return 0;
			break;
		case T_buttons:
			if (!parse_buttons(lex, wdesc))
				return 0;
			break;
		case T_panes:
			if (!parse_panes(lex, wdesc))
				return 0;
			break;
		case T_menu_bar:
			if (!parse_menu_bar(lex, wdesc))
				return 0;
			break;
		case T_menu:
			if (!parse_menu(lex, wdesc))
				return 0;
			break;
		case T_invalid:
		case T_autoload:
		case T_bottom:
		case T_lcurly:
		case T_name:
		case T_number:
		case T_rcurly:
		case T_top:
		default:
			lex->syntax_error();
			return 0;
		}
	}
	// NOTREACHED
}

static char *
read_config_file(const char *path)
{
	char		*config;
	struct stat	sbuf;
	int		fd;

	if ((fd = open(path, O_RDONLY)) == -1)
	{
		display_msg(config_sev, GE_config_open, path,
			strerror(errno));
		return 0;
	}
	if ((fstat(fd, &sbuf) == -1) || (sbuf.st_size == 0))
	{
		display_msg(config_sev, GE_config_stat, path);
		close(fd);
		return 0;
	}
	config = new char[sbuf.st_size+1];
	if (read(fd, config, sbuf.st_size) != sbuf.st_size)
	{
		display_msg(config_sev, GE_config_read, path);
		delete config;
		close(fd);
		return 0;
	}
	config[sbuf.st_size] = 0;
	close(fd);
	return config;
}

static int
eval_and_verify(const char *dirname)
{
	DIR	*dirp;
	dirent	*direntp;
	char	buf[PATH_MAX+1];
	char	*ptr;
	int	len;

	last_mref = 0;
	if ((dirp = opendir(dirname)) == 0)
	{
		display_msg(config_sev, GE_config_dir_open,
			dirname, strerror(errno));
		return 0;
	}
	len = strlen(dirname);
	strncpy(buf, dirname, len);
	buf[len] = '/';
	ptr = &buf[len+1];
	while((direntp = readdir(dirp)) != 0)
	{
		char	*config;
		if (*direntp->d_name == '.')
		{
			// files beginning with '.' are ignored
			continue;
		}
		strcpy(ptr, direntp->d_name);
		if ((config = read_config_file(buf)) == 0)
		{
			return 0;
		}
		Lexer	lex(config, buf);
		int	ret = parse_window(&lex, direntp->d_name);
		delete config;
		if (!ret)
		{
			cleanup_config();
			return 0;
		}
	}
	if (!verify_config())
	{
		cleanup_config();
		return 0;
	}
	return 1;
}


int
build_configuration()
{
	const char	*dname;

	config_sev = E_ERROR;
	if (((dname = resources.get_config_dir()) != 0) &&
		eval_and_verify(dname))
	{
		config_directory = pathcanon(dname);
		return 1;
	}
	config_sev = E_FATAL;
	return(eval_and_verify(debug_config_path));
}

// save and read layout files

// format of saved layout is:
// Window_set\n
// win1^_x,y\n
// win2^_x,y\n
// Window_set\n
// win1^_x,y\n
// win2^_x,y\n
// ... 

// return number of window sets that need to be opened
int
read_saved_layout()
{
	char		config_file[PATH_MAX+1];
	char 		*home_dir;
	FILE		*fptr;
	char		line[BUFSIZ];
	int		ws_id = 0;
	int		found = 1;
	Window_configuration	*last = 0;


	if ((home_dir = getenv("HOME")) == 0)
		return 0;
	sprintf(config_file, "%s/%s", home_dir, SAVED_CONFIG_FILE);
	if ((fptr = fopen(config_file, "r")) == 0)
		return 0;

	while(fgets(line, BUFSIZ, fptr) != 0)
	{
		char			*ptr, *num;
		Window_descriptor	*wdesc;
		Window_configuration	*wconf;
		unsigned short		x, y;

		if (strncmp(line, "Window_set", 10) == 0)
		{
			if (found)
			{
				// found a valid window in last id
				ws_id++;
			}
			found = 0;
			continue;
		}
		if ((ptr = strstr(line, "^_")) == 0)
			// just ignore it
			continue;
		*ptr = 0;
		ptr += 2;
		if ((wdesc = find_wdesc(line)) == 0)
			// no such window
			continue;
		if (strncmp(ptr, "iconic", 6) == 0)
		{
			found++;
			wconf = new Window_configuration(ws_id, wdesc);
			wconf->set_iconic();
		}
		else
		{
			if ((num = strchr(ptr, ',')) == 0)
				continue;
			*num++ = 0;
			x = (unsigned short)atoi(ptr);
			if ((ptr = strchr(num, '\n')) == 0)
				continue;
			*ptr = 0;
			y = (unsigned short)atoi(num);
			found++;
			wconf = new Window_configuration(ws_id, wdesc);
			wconf->set_position(x, y);
		}
		if (last)
			last->append(wconf);
		else
			window_configuration = wconf;
		last = wconf;
	}
	fclose(fptr);
	if (!found)
		ws_id--;
	return ws_id;
}

void
save_current_layout()
{
	char		config_file[PATH_MAX+1];
	char 		*home_dir;
	FILE		*fptr;
	Window_set	*ws;
	int		found;

	if ((home_dir = getenv("HOME")) == 0)
		return;

	sprintf(config_file, "%s/%s", home_dir, SAVED_CONFIG_FILE);

	if ((fptr = fopen(config_file, "w")) == 0)
	{
		display_msg(E_ERROR, GE_config_open, config_file,
			strerror(errno));
		return;
	}
	
	ws = (Window_set *)windows.first();
	for(; ws; ws = (Window_set *)windows.next())
	{
		found = 0;
		Base_window	**bw = ws->get_base_windows();
		for(int i = 0; i < windows_per_set; i++, bw++)
		{
			Window_shell	*wsh = (*bw)->get_window_shell();
			if (*bw && (*bw)->is_open())
			{
				unsigned short	x, y;
				if (!found)
				{
					found = 1;
					fprintf(fptr, "Window_set\n");
				}
				if (wsh->is_iconic())
				{
					fprintf(fptr, "%s^_iconic\n",
						(*bw)->get_wdesc()->get_name());
				}
				else
				{
					(*bw)->get_window_shell()->get_current_pos(x, y);
					fprintf(fptr, "%s^_%d,%d\n",
						(*bw)->get_wdesc()->get_name(),
						x, y);
				}
			}
		}
	}
	fclose(fptr);
}

static char *
mne_string(mnemonic_t mne)
{
	static char	mbuf[MB_LEN_MAX + 2];
	int	len;
	if (!mne)
		return "";
	mbuf[0] = '_';
	len = wctomb(&mbuf[1], mne);
	mbuf[len+1] = 0;
	return mbuf;
}

// return string surrounded by quotes
static char *
name_string(const char *name)
{
	static char	*name_ptr;
	static int	name_len;
	int		len = strlen(name);
	if ((len + 3) > name_len)
	{
		// 3 for 2 quotes and null
		delete name_ptr;
		name_len = len + 50;
		name_ptr = new char[name_len];
	}
	*name_ptr = '"';
	strcpy(name_ptr+1, name);
	name_ptr[len+1] = '"';
	name_ptr[len+2] = 0;
	return name_ptr;
}

static void
write_bar_config(FILE *fp, Bar_descriptor *bbar)
{
	Button	*btn = bbar->get_buttons();
	fprintf(fp, "Buttons %s {", bbar->is_bottom() ? "bottom" :
		"top");
	for(; btn; btn = btn->next())
	{
		CButtons	bt = btn->get_type();
		char		*cmd = 0;
		if (bt == B_exec_cmd || bt == B_debug_cmd)
		{
			cmd = makestr(name_string((char *)btn->get_udata()));
		}
		fprintf(fp, "\n\t%s\t%s\t%s",
			name_string(btn->get_name()),
			mne_string(btn->get_mne()),
			button_action(bt));
		if (cmd)
		{
			fprintf(fp, " %s", cmd);
			delete cmd;
		}
	}
	fprintf(fp, "\n}\n\n");
}

static void
write_menu_config(FILE *fp, Menu_descriptor *menu)
{
	Button	*sub_menu = 0;
	Button	*btn = menu->get_buttons();

	fprintf(fp, "Menu %s {", menu->get_mname());
	for(; btn; btn = btn->next())
	{
		CButtons	bt = btn->get_type();
		char		*cmd = 0;
		const char	*mname = 0;
		if (bt == B_exec_cmd || bt == B_debug_cmd)
		{
			cmd = makestr(name_string((char *)btn->get_udata()));
		}
		else if (bt == B_menu)
		{
			if (!sub_menu)
				sub_menu = btn;
			mname = btn->get_sub_table()->get_mname();
		}
		fprintf(fp, "\n\t%s\t%s\t%s",
			name_string(btn->get_name()),
			mne_string(btn->get_mne()),
			button_action(bt));
		if (cmd)
		{
			fprintf(fp, " %s", cmd);
			delete cmd;
		}
		else if (mname)
		{
			fprintf(fp, " %s", mname);
		}
	}
	fprintf(fp, "\n}\n\n");
	for(; sub_menu; sub_menu = sub_menu->next())
	{
		if (sub_menu->get_type() == B_menu)
			write_menu_config(fp, sub_menu->get_sub_table());
	}
}

// save configuration for a single window
static int
save_window_configuration(Window_descriptor *wd, const char *cpath)
{
	FILE	*fp;
	char	cfile[PATH_MAX];
	char	*tfile = tempnam(cpath, 0);
	char	tbuf[100];
	time_t	clock = time(0);
	struct	tm	*ltime = localtime(&clock);
	Pane_descriptor		*panes;
	Menu_descriptor		*menus;
	Bar_descriptor		*bbar;

	strftime(tbuf, 100, "%x", ltime);

	if ((fp = fopen(tfile, "w")) == 0)
	{
		display_msg(E_ERROR, GE_config_tmp_open, tfile, 
			strerror(errno));
		return 0;
	}
	if (fprintf(fp, "! Configuration for window: %s\n", 
		wd->get_name()) <= 0)
		goto print_error;
	if (fprintf(fp, "! Generated on %s\n\n", tbuf) <= 0)
		goto print_error;
	if (wd->get_flags() & W_AUTO_POPUP)
	{
		if (fprintf(fp, "Flags {\n\tautoload\n}\n\n" ) <= 0)
			goto print_error;
	}
	panes = wd->get_panes();
	fprintf(fp, "Panes {\n");
	for(; panes; panes = panes->next())
	{
		int		r, c;
		const char	*mname;
		menus = panes->get_menu_descriptor();

		fprintf(fp, "\t%s", pane_name(panes->get_type()));
		r = panes->get_rows();
		c = panes->get_cols();
		if (menus)
			mname = menus->get_mname();
		else
			mname = "";
		if (r && c)
			fprintf(fp, " %d %d %s\n", r, c, mname);
		else if (r)
			fprintf(fp, " %d %s\n", r, mname);
		else
			fprintf(fp, " %s\n", mname);
	}
	if (fprintf(fp, "}\n\n") <= 0)
		goto print_error;
	menus = wd->get_menus();
	fprintf(fp, "MenuBar {\n");
	for(; menus; menus = menus->next())
	{
		fprintf(fp, "\t%s\t%s\t%s\n", menus->get_name(),
			mne_string(menus->get_mne()),
			menus->get_mname());
	}
	if (fprintf(fp, "}\n\n") <= 0)
		goto print_error;
	bbar = wd->get_top_button_bars();
	for(; bbar; bbar = bbar->next())
		write_bar_config(fp, bbar);
	if (ferror(fp))
		goto print_error;
	bbar = wd->get_bottom_button_bars();
	for(; bbar; bbar = bbar->next())
		write_bar_config(fp, bbar);
	if (ferror(fp))
		goto print_error;
	menus = wd->get_menus();
	for(; menus; menus = menus->next())
		write_menu_config(fp, menus);
	if (ferror(fp))
		goto print_error;
	panes = wd->get_panes();
	for(; panes; panes = panes->next())
	{
		menus = panes->get_menu_descriptor();
		if (menus)
			write_menu_config(fp, menus);
	}
	if (ferror(fp))
		goto print_error;

	fclose(fp);
	sprintf(cfile, "%s/%s", cpath, wd->get_name());
	if (rename(tfile, cfile) == -1)
	{
		display_msg(E_ERROR, GE_config_create, cfile,
			strerror(errno));
		remove(tfile);
		delete tfile;
		return 0;
	}
	delete tfile;
	wd->clear_flag(W_DESC_CHANGED);
	return 1;
print_error:
	display_msg(E_ERROR, GE_config_write, tfile, strerror(errno));
	fclose(fp);
	remove(tfile);
	delete tfile;
	return 0;
}

int
save_current_configuration(const char *config_dir)
{
	const char		*cpath;
	Window_descriptor	*wd = window_descriptor;
	Boolean			new_dir = FALSE;
	if (!config_directory || 
		(strcmp(config_dir, config_directory) != 0))
	{
		cpath = pathcanon(config_dir);
		if ((mkdir(config_dir, 
			S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) == -1) 
				&& (errno != EEXIST))
		{
			display_msg(E_ERROR, GE_config_dir_create,
				config_dir, strerror(errno));
			return 0;
		}
		new_dir = TRUE;
	}
	else
		cpath = config_dir;

	for(; wd; wd = wd->next())
	{
		if (!new_dir && !(wd->get_flags() & W_DESC_CHANGED))
			continue;
		if (!save_window_configuration(wd, cpath))
			return 0;
	}
	if (new_dir)
	{
		delete config_directory;
		config_directory = (char *)cpath;
	}
	return 1;
}
