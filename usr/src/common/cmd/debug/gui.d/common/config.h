#ifndef _config_h_
#define _config_h_
#ident	"@(#)debugger:gui.d/common/config.h	1.9"

#include "Buttons.h"
#include "Link.h"
#include "UI.h"

#ifndef SAVED_CONFIG_FILE
#define SAVED_CONFIG_FILE ".debug_layout"
#endif

class Menu_descriptor : public Link {
	const char	*mname;	// just to find it
	const char	*name;
	mnemonic_t	mnemonic;
	Button		*buttons;
	int		nbuttons;
public:
			Menu_descriptor(const char *m,
				const char *n, mnemonic_t mne)
				{ mname = m; name = n; mnemonic = mne;
					nbuttons = 0; buttons = 0; }
			~Menu_descriptor() { delete (char *)mname; 
						delete (char *)name; }
	void		add_button(Button *);
	const char	*get_name() { return name; }
	const char	*get_mname() { return mname; }
	mnemonic_t	get_mne() { return mnemonic; }
	Button		*get_buttons() { return buttons; }
	int		get_nbuttons() { return nbuttons; }
	Menu_descriptor	*next() { return (Menu_descriptor *)Link::next(); }
};

class Pane_descriptor : public Link
{
	int		type;
	int		nlines;
	int		ncolumns;
	Menu_descriptor	*menu_descriptor;  // for popup menu
public:
			Pane_descriptor(int t, int l, int c, 
				Menu_descriptor *md)
				{ type = t; nlines = l; ncolumns = c; 
					menu_descriptor = md; }
			~Pane_descriptor() 
				{ delete menu_descriptor; }
	Pane_descriptor	*next() { return 
				(Pane_descriptor *)Link::next(); }
	int		get_type() { return type; }
	int		get_rows() { return nlines; }
	int		get_cols() { return ncolumns; }
	void		set_rows(int r) { nlines = r; }
	void		set_cols(int c) { ncolumns = c; }
	Menu_descriptor	*get_menu_descriptor() { return menu_descriptor; }
};

class Bar_descriptor : public Link {
	Button		*buttons;
	unsigned char	nbuttons;
	Boolean		bottom;
public:
			Bar_descriptor(Boolean b)
				{ nbuttons = 0; buttons = 0; bottom = b; }
			Bar_descriptor(Bar_descriptor *);
			~Bar_descriptor() {}
	void		add_button(Button *);
	void		remove_all();
	void		remove_button(CButtons, const char *);
	void		replace_button(Button *oldb, Button *newb);
	Button		*get_buttons() { return buttons; }
	int		is_bottom() { return (bottom == TRUE); }
	int		get_nbuttons() { return nbuttons; }
	Bar_descriptor	*next() { return (Bar_descriptor *)Link::next(); }
	Button		*get_button(CButtons, const char *);
};

#define	W_DESC_CHANGED		0x40000000	// descriptor modified
#define	W_AUTO_POPUP		0x80000000	// start whenever
						// creating a new
						// window set
struct Bar_info;

// other flags are pane types

class Window_descriptor : public Link
{
	const char		*name;
	Pane_descriptor		*panes;
	Menu_descriptor		*menus;
	Bar_descriptor		*top_button_bars;
	Bar_descriptor		*bottom_button_bars;
	unsigned long		flags;		
	int			npanes;
public:
				Window_descriptor(const char *n)
				{ name = n; panes = 0; menus = 0;
					top_button_bars = 0; npanes = 0; 
					bottom_button_bars = 0;
					flags = 0; }
				~Window_descriptor() {}
	int			add_pane(Pane_descriptor *);
	void			add_menu(Menu_descriptor *);
	void			add_button_bar(Bar_descriptor *);
	void			replace_button_bar(Bar_info *bi);
	void			remove_button_bar(Bar_info *bi);
				// access functions
	void			set_flag(int f) { flags |= f; }
	void			clear_flag(int f) { flags &= ~f; }
	unsigned long		get_flags() { return flags; }
	Bar_descriptor		*get_top_button_bars() { return top_button_bars; }
	Bar_descriptor		*get_bottom_button_bars() { return bottom_button_bars; }
	Menu_descriptor		*get_menus() { return menus; }
	Pane_descriptor		*get_panes() { return panes; }
	int			get_npanes() { return npanes; }
	const char		*get_name() { return name; }
	Window_descriptor	*next() { return (Window_descriptor *)Link::next(); }
};

// struct passed around to routines dynamically adding or
// deleting button bars
struct Bar_info {
	Bar_descriptor	*old_bdesc;
	Bar_descriptor	*new_bdesc;
	Bar_descriptor	*new_wbar;
	Window_descriptor	*window_desc;
	char		old_index;
	Boolean		bottom;
	Boolean		remove;
};

// startup configuration
class Window_configuration {
	int			ws_id;
	Window_descriptor	*wdesc;
	Window_configuration	*_next;
	Boolean			iconic;
	unsigned short		x_pos;
	unsigned short		y_pos;
public:
			Window_configuration(int wid,
				Window_descriptor *wd)
				{ ws_id = wid; wdesc = wd;
					_next = 0; iconic = FALSE; }
			~Window_configuration() {}
	Window_configuration	*next() { return _next; }
	void		append(Window_configuration *n) { _next = n; }
	Window_descriptor *get_wdesc() { return wdesc; }
	int		get_wid() { return ws_id; }
	Boolean		is_iconic() { return iconic; }
	unsigned short	get_x_pos() { return x_pos; }
	unsigned short	get_y_pos() { return y_pos; }
	void		set_iconic() { iconic = TRUE; }
	void		set_position(unsigned short x, unsigned short y)
				{ x_pos = x; y_pos = y; }
};

struct Button_descriptor {
	CButtons	button_id;
	LabelId		button_name;
	LabelId		button_mne;
};

extern char			*config_directory;
extern Window_configuration	*window_configuration;
extern Window_descriptor	*window_descriptor;
extern int			windows_per_set;
extern int			max_rows;
extern const char		*debug_config_path;

extern int			build_configuration();
extern int			read_saved_layout();
extern void			save_current_layout();
extern int			save_current_configuration(const char *);
extern Bar_descriptor		*make_bar_descriptor(Boolean bottom,
					const Button_descriptor *btn, 
					int nbuttons);

#endif
