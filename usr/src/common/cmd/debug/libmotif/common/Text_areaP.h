#ifndef	_TEXT_AREAP_H
#define	_TEXT_AREAP_H
#ident	"@(#)debugger:libmotif/common/Text_areaP.h	1.8"

// toolkit specific members of the Text_area class
// included by ../../gui.d/common/Text_area.h

class Text_data
{
private:
	int	next_line;
	int	nchars;
	int	*lpos;
	int	maxlines;
public:
		Text_data();
		~Text_data() { delete lpos; }
	int	get_nlines() { return next_line ? next_line-1 : 0; }
	int	get_nchars() { return nchars; }
	void	reset();
	void	add_string(const char *);
	int	get_line(int, int);
	int	get_lpos(int);
	int	get_epos(int);
	void	delete_first_n_lines(int n);
	void	replace_text(int spos, int epos, int len, char *text);
};

struct Selection_data
{
	unsigned char	nclicks;
	XmTextPosition	anchor;
	Time		last_time;

			Selection_data() 
				{ anchor = 0; nclicks = 0; 
				last_time = CurrentTime;  }
			~Selection_data()  {}
};

// The constructor declared here is called from the Text_display constructor.
// It is declared here instead of in ../../gui.d/common/Text_area.h because
// the way the two classes relate is really toolkit-specific

#define TEXT_AREA_TOOLKIT_SPECIFICS		\
protected:					\
	Widget		text_area;		\
	Text_data	text_data;		\
	char		*sel_string;		\
	char		*text_content;		\
	Selection_data	selection_data;		\
	XmTextPosition	selection_start;	\
	int		selection_size;		\
	Boolean		update_content;		\
						\
						\
public:						\
			Text_area(Component *, const char *name,\
				Command_sender *creator, 	\
				Callback_ptr select_cb, Help_id help_msg);\
						\
	Widget		get_text_area() { return text_area; }	\
	void		set_selection_notifier();		\
	void		selection_begin(XEvent *);		\
	void		selection_end(XEvent *);		\
	void		resize();				\
	virtual void	set_selection(XmTextPosition start, XmTextPosition end);

#endif	// _TEXT_AREAP_H
