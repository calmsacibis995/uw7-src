#ifndef	_TEXT_DISPP_H
#define	_TEXT_DISPP_H
#ident	"@(#)debugger:libmotif/common/Text_dispP.h	1.7"

class RegExp;
class DND;

// toolkit specific members of the Text_display class
// included by ../../gui.d/common/Text_disp.h

struct Text_save;

#define	TEXT_DISPLAY_TOOLKIT_SPECIFICS	\
private:				\
	Widget		textwin;	\
	int		*line_map;	\
	RegExp		*regexp;	\
	int		top_pos;	\
	Text_save	*text_save;	\
	XmTextPosition	highlight_start;\
	XmTextPosition	highlight_end;\
	DND		*dnd;		\
	Boolean		is_source;	\
					\
	Boolean		read_file(const char *path);	\
	void		read_buf(const char *buf);	\
	void		finish_setup();			\
	Boolean		update_line_map(XmTextPosition); \
	void		clear_line_map();		\
	int		line_to_row(int line);		\
	int		last_visible();			\
	Boolean		search_forward(XmTextPosition *, XmTextPosition *); \
	Boolean		search_backward(XmTextPosition *, XmTextPosition *); \
	void		clear_highlight();		\
	void		set_highlight(XmTextPosition start, XmTextPosition end); \
public:							\
	void		set_selection(XmTextPosition start, XmTextPosition end); \
	void		set_selection_size(int n)	{ selection_size = n; } \
	void		adjust_word(XmTextPosition &, XmTextPosition &);	\
	void		text_about_to_change(XmTextVerifyCallbackStruct *ptr);	\
	void		text_changed();	\
	void		cursor_motion();	\
	void		resize();	\
	void		redraw();	\
	void		scroll();	\
	void		toggle_bkpt(int x, int y); \

#endif	// _TEXT_DISPP_H
