#ifndef _TEXT_EDITP_H
#define _TEXT_EDITP_H
#ident	"@(#)debugger:libmotif/common/Text_editP.h	1.4"


//  gc_base and gc_stop are Graphics Contexts - information about foreground,
//	background, etc. needed to draw the arrow and stop sign

// baseline, xfont and font_height must be initialized
// by the derived classes

#define TEXT_EDIT_TOOLKIT_SPECIFICS	\
protected:				\
	Widget		canvas;		\
	int		current_line;	\
	int		top;		\
	int		x_arrow;	\
	int		x_stop;		\
	GC		gc_base;	\
	GC		gc_stop;	\
	int		baseline;	\
	XFontStruct	*xfont;		\
	int		font_height;	\
					\
	void		draw_number(int line, int offset);	\
	void		draw_arrow(int offset);		\
	void		erase_arrow(int offset);	\
	void		draw_stop(int offset);		\
	void		erase_stop(int offset);		\
	void		set_gc();			\
	void		clear_canvas();			\
	void		size_canvas(Boolean def = FALSE);\
	int		calculate_y(int element_ht, int offset);\
public:							\
	virtual void	redraw() = 0;			\
	virtual void	toggle_bkpt(int x, int y) = 0;

#endif
