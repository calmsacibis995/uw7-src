#ifndef	_TOOLKIT_H
#define	_TOOLKIT_H
#ident	"@(#)debugger:libmotif/common/Toolkit.h	1.3"

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <stdlib.h>

#include <Xm/Xm.h>
#include "List.h"
#include "Help.h"
#include "gui_label.h"

#ifdef DEBUG
#define DBG_A	0x1	// Alert_shell
#define DBG_B	0x2	// Boxes
#define DBG_C	0x4	// Caption
#define DBG_D	0x8	// Dialog_shell
#define DBG_M	0x10	// Menu
#define DBG_R	0x20	// Radio
#define DBG_SE	0x40	// Selection_list
#define DBG_SL	0x80	// Slider
#define DBG_ST	0x100	// Simple_text
#define DBG_T	0x200	// Table
#define DBG_TA	0x400	// Text_area
#define DBG_TD	0x800	// Text_display
#define DBG_TL	0x1000	// Text_line
#define DBG_TO	0x2000	// Toggle
#define DBG_W	0x4000	// Window_shell
#define DBG_MASK 0xffff
#endif

#define	PADDING	4
#define DISMISS		"Close Window"
#define DISMISS_MNE	'C'

#define FontHeight(f)	((f)->max_bounds.ascent + (f)->max_bounds.descent)
#define FontWidth(f)	((f)->max_bounds.width)

extern	Widget		base_widget;
extern	XtAppContext	base_context;
extern	Atom		table_atom;
extern	Atom		slist_atom;

typedef XtWorkProc	Background_proc;
typedef XtInputCallbackProc	Input_proc;
typedef XtInputId		Input_id;

XFontStruct	*get_default_font(Widget);
XFontStruct	*get_default_font(XmFontList);
void	busy_window(Widget, Boolean);
void	register_help(Widget, const char *title, Help_id);
void	display_help(Widget, Help_mode, Help_id);
#if OLD_HELP
void	helpdesk_help(Widget);
#endif
int	get_widget_pad(Widget, Widget);
void	recover_notice();
void	background_task(Background_proc, void *);
Input_id register_input_proc(int, Input_proc, void *);
void	unregister_input_proc(Input_id);

extern List	fatal_list;
extern int	is_color();
extern Pixel	get_color(char *);
extern KeySym	get_mnemonic(LabelId label);
extern KeySym	get_mnemonic(wchar_t);

#ifdef __cplusplus
extern "C" {
#endif
	extern Pixmap XCreatePixmapFromData(	// defined in xpm.c
		Display *, 	/* display */
		Drawable,	/* drawable */ 
		Colormap,	/* colormap */
		unsigned int,	/* width */
		unsigned int,	/* height */
		unsigned int,	/* depth */
		unsigned int,	/* ncolors */
		unsigned int,	/* chars_per_pixel */
		char **,	/* colors */
		char **		/* pixels */
	);
#ifdef __cplusplus
}
#endif

#endif // _TOOLKIT_H
