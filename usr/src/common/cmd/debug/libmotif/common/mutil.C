#ident	"@(#)debugger:libmotif/common/mutil.C	1.15"

// GUI headers
#include "Alert_sh.h"
#include "UI.h"
#include "Dispatcher.h"
#include "Message.h"
#include "Msgtab.h"
#include "Transport.h"
#include "Resources.h"
#include "gui_label.h"
#include "Label.h"

// Debug headers
#include "str.h"
#include "Vector.h"
#include "Machine.h"

#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <Xm/Xm.h>
#include <Xm/AtomMgr.h>
#include <Xm/Display.h>
#include <X11/cursorfont.h>
#include <sys/types.h>
#include <sys/stat.h>

#if OLD_HELP
#include <Dt/Desktop.h>
char			*Help_path;
#else
extern "C" {
#include <X11/scohelp/api.h>
}
static	XtPointer	help_handle;
static	Widget		current_window;
#endif

Widget                  base_widget;
XtAppContext		base_context;
extern Transport	transport;
static const char	*table_atom_name = "DEBUG_TABLE_ATOM";
Atom			table_atom;
static const char	*slist_atom_name = "DEBUG_SLIST_ATOM";
Atom			slist_atom;
Boolean			btn1_transfer;
#ifdef DEBUG
int			debug_flags_t = DBG_MASK;
#endif

static Vector		argv_new;
static void		parse_args(int *, char **&);
static char 		*parse_one(char *&);

static void
get_dmsg(XtPointer, int *, XtInputId)
{
	do {
		dispatcher.process_msg();
	} while(!transport.inqempty());
}

static Boolean	fatal_pending = FALSE;

static void
check_alert()
{
	if (old_alert_shell)
	{
		delete old_alert_shell;
		old_alert_shell = 0;
	}
	if (fatal_pending && fatal_list.isempty())
	{
		dispatcher.send_msg(0, 0, "quit\n");
		exit(1);
	}
}

void
toolkit_main_loop()
{

        for (;;)
	{
		XEvent  event;

		// let X events through first so debugger doesn't
		// block them out
        	while(XtAppPending(base_context) & XtIMXEvent) 
		{
			if (!fatal_list.isempty())
				fatal_pending = TRUE;
               		XtAppProcessEvent(base_context, XtIMXEvent);
			check_alert();
		}

		if (!fatal_list.isempty())
			fatal_pending = TRUE;

		XtAppNextEvent(base_context, &event);
		XtDispatchEvent(&event);

		// clear queues
		while (!transport.inqempty())
			dispatcher.process_msg();
		check_alert();
	}
}

#if OLD_HELP
static Boolean
help_path_check(String dir)
{
	struct stat sbuf;

	// make sure it's a readable directory
	if (access(dir, R_OK|X_OK) == 0 && 
	    stat(dir, &sbuf) == 0 && 
	    (sbuf.st_mode & S_IFDIR))
		return True;
	return False;
}

static void
initialize_help()
{
	DtInitialize(base_widget);
	Help_path = XtResolvePathname(XtDisplay(base_widget), 
		"help",		// %T 
		"debug",	// %N 
		"",		// %S
		NULL,		// path 
		NULL, 		// subst
		0, 		// nsubst
		(XtFilePredicate)help_path_check);
	if (!Help_path)
	{
		char *locale = setlocale(LC_MESSAGES, NULL);
		Help_path = makesf(strlen("/usr/X/lib/locale//help/debug") + strlen(locale),
				"/usr/X/lib/locale/%s/help/debug", locale);
	}
}
#else

static void
busy_cursor(Boolean busy)
{
	busy_window(current_window, busy);
}

static void
display_str(const char *msg)
{
	Alert_shell *shell = new Alert_shell(msg, AT_error, LAB_dok);
	shell->popup();
}

static void
help_close()
{
	if (help_handle)
		HelpClose(help_handle);
}

static void
initialize_help()
{
	HelpOpen(&help_handle, base_widget, Help_path, busy_cursor, 
		(void (*)(char *))display_str);
	atexit(help_close);
}
#endif

void
init_gui(const char *name, const char *wclass, int *argc, char **argv)
{
	XrmOptionDescRec	*options;
	int			noptions;

	parse_args(argc, argv);

	options = resources.get_options(noptions);
	argv[0] = (char *)wclass;

	base_widget = XtVaAppInitialize(&base_context, wclass, 
		options, noptions,
		argc, argv, NULL, 
		0);
	XtVaGetValues((Widget)XmGetXmDisplay(XtDisplay(base_widget)),
		"enableBtn1Transfer", &btn1_transfer,
		0);
	resources.initialize();
#if OLD_HELP
	initialize_help();
#endif

	XtAppAddInput(base_context, fileno(stdin), 
		(XtPointer) XtInputReadMask,
		(XtInputCallbackProc) get_dmsg, NULL);
	table_atom = XmInternAtom(XtDisplay(base_widget), 
			(String)table_atom_name, FALSE);
	slist_atom = XmInternAtom(XtDisplay(base_widget), 
			(String)slist_atom_name, FALSE);
}

// the -X options from the command line are passed in verbatim,
// e.g. if -X "-xrm 'debug*background: blue'" was specified, we would get a
// single argument "-xrm 'debug*background: blue'" here. in order for 
// XtInitialize to understand it, we must break it up into separate args.
// in this particular case, the result should be 2 args: "-xrm" and
// "debug*background: blue".
static void
parse_args(int *argc, char **&argv)
{
	int argc_old, argc_new;
	char *input;
	char *arg1;

	argc_old = *argc;
	if(argc_old <= 1)
		return;
	// skip argv[0], it's never interesting
	argv_new.add(&argv[0], sizeof(char *));
	argc_new = 1;
	while(--argc_old > 0)
	{
		input = *++argv;
		while(arg1 = parse_one(input))
		{
			argv_new.add(&arg1, sizeof(char *));
			++argc_new;
		}
	}

	argv = (char **)argv_new.ptr();
	*argc = argc_new;
}

// scan & return the next token. input starts from 'input'.
// the character following the last character in the token gets
// overwritten with '\0'. 'input' is updated to point to the
// beginning of next token.
// the following (shell-like) quoting mechanisms are supported 
// currently:
// 1) \c => c, except inside '...' and sometimes inside "..."
// 2) inside "...", all whitespaces are preserved and \ 
//    quotes only " and \, that is \" => ", and \\ => \;
//    and ' loses its special meaning.
// 3) inside '...', everything is preserved verbatim, including
//    \ and ".
// note that the resulting string may be shorter than the original
// since all quoting characters are thrown out. to handle this,
// we make use of a trailing pointer 'ss' as we scan and overwrite
// the existing data space.
static char *
parse_one(char *&input)
{
	char	*tok;		// beginning of token
	char	*s;		// scanning pointer
	char	*ss;		// trailing pointer
	Boolean in_sq;		// True if inside '...'
	Boolean in_dq;		// True if inside "..."

	if(!input)
		return NULL;
	// assume that on entry, input points to 
	// the first char of a token
	s = ss = tok = input;
	// find end of token
	in_sq = in_dq = False;
	for(; *s && !(isspace(*s) && !in_sq && !in_dq); ++s)
	{
		switch(*s)
		{
		case '"':
			if (!in_sq)
			{
				in_dq = in_dq ? False : True;
				continue;
			}
			break;
		case '\'':
			if (!in_dq)
			{
				in_sq = in_sq ? False : True;
				continue;
			}
			break;
		case '\\':
			if (in_sq)
				break;
			if (in_dq)
				switch(s[1])
				{
				default:
					if (ss < s)
						*ss = '\\';
					break;
				case '"':
				case '\\':
					break;
				}
			++s;
			break;
		}
		if (ss < s)
			*ss = *s;
		++ss;
	}
	if (*s)
	{
		// turn first char past the end of token to '\0'
		*ss = '\0';
		// s is pointing at a whitespace following the token
		// by now, so skip to first nonwhite space (i.e. beginning
		// of next token)
		for(++s; isspace(*s); ++s)
			;
	}
	// else we've reached end of this arg, so make sure
	// the token's terminated properly
	else if (ss < s)
		*ss = '\0';
	// advance input to next token
	input = *s ? s : NULL;
	return tok;
			
}

static Alert_type
get_alert_type(Message *m)
{
	if (Mtable.msg_class(m->get_msg_id()) == MSGCL_error)
	{
		switch(m->get_severity())
		{
		default:
		case E_NONE:	break;
		case E_ERROR:	return AT_error;
		case E_WARNING:	return AT_warning;
		case E_FATAL:	return AT_fatal;
		}
	}
	return AT_message;
}

void
display_msg(Message *m)
{
	Alert_shell *shell = new Alert_shell(m->format(), get_alert_type(m), 
		LAB_dok);
	shell->popup();
}

void
display_msg(Severity sev, Gui_msg_id id ...)
{
	va_list		ap;
	char		*msg;
	Alert_type	atype;

	va_start(ap, id);
	msg = do_vsprintf(gm_format(id), ap);
	va_end(ap);

	switch(sev)
	{
	default:
	case E_NONE:
		atype = AT_message;
		break;
	case E_ERROR:
		atype = AT_error;
		break;
	case E_WARNING:
		atype = AT_warning;
		break;
	case E_FATAL:
		atype = AT_fatal;
		break;
	}
	Alert_shell *shell = new Alert_shell(msg, atype, LAB_dok);
	shell->popup();
}

void
display_msg(Callback_ptr handler, void *object, LabelId action,
	LabelId no_action, Alert_type atype, Gui_msg_id id ...)
{
	va_list	ap;
	char	*msg;

	va_start(ap, id);
	msg = do_vsprintf(gm_format(id), ap);
	va_end(ap);

	Alert_shell *shell = new Alert_shell(msg, atype,
		action, no_action, handler, object);
	shell->popup();
}

void
display_msg(Callback_ptr handler, Command_sender *object, LabelId action,
	LabelId no_action, Message *m)
{
	Alert_shell *shell = new Alert_shell(m->format(), get_alert_type(m),
		action, no_action, handler, object);
	shell->popup();
}

void
beep()
{
	XBell(XtDisplay(base_widget), -1);
}

void
busy_window(Widget w, Boolean busy)
{
	Display		*dpy = XtDisplay(w);
	Window		win = XtWindow(w);
	static Cursor	busy_cur;

	if (busy)
	{
		if(!busy_cur)
			busy_cur = XCreateFontCursor(dpy, XC_watch);
		XDefineCursor(dpy, win, busy_cur);
		XSync(dpy, FALSE);
	}
	else
	{
		XUndefineCursor(dpy, win);
	}
}

static void
help_CB(Widget w, Help_id help, XtPointer)
{
	display_help(w, HM_section, help);
}

void
register_help(Widget widget, const char *title, Help_id help)
{
        if(!help)
	{
                return;
	}

	XtAddCallback(widget, XmNhelpCallback,
		(XtCallbackProc)help_CB, (XtPointer)help);
}

void
display_help(Widget w, Help_mode mode, Help_id help)
{
#if OLD_HELP
	DtRequest request;
	Display *dpy = XtDisplayOfObject(w);

	if(!help)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	// extract help entry data
	Help_info *help_entry = &Help_files[help];

	if (DtGetAppId(dpy, _HELP_QUEUE(dpy)) == None)
	{
		// dtm not running?
		display_msg(E_NONE, GM_no_help);
		return;
	}

	// issue request to DTM 
	memset(&request, 0, sizeof(request));
	request.display_help.rqtype = DT_DISPLAY_HELP;
	request.display_help.source_type = 
		mode == HM_section ? DT_SECTION_HELP : DT_TOC_HELP;
	request.display_help.app_name = (String)"debug";
	request.display_help.help_dir = (String)Help_path;
	request.display_help.file_name = (String)help_entry->filename;
	request.display_help.sect_tag = (String)help_entry->section;
	DtEnqueueRequest(XtScreenOfObject(w), _HELP_QUEUE(dpy),
		_DT_QUEUE(dpy), XtWindowOfObject(w), &request);
#else
	if (!help_handle)
		initialize_help();

	// extract help entry data
	Help_info *help_entry = &Help_files[help];

	Widget parent = XtParent(w);
	while (parent != base_widget)
	{
		w = parent;
		parent = XtParent(w);
	}
	current_window = w;
	HelpDisplay(help_handle, NULL, helpTopic, (char *)help_entry->section);
#endif
}

#if OLD_HELP
// open Help Desk
void
helpdesk_help(Widget w)
{
	DtRequest request;
	Display	*dpy = XtDisplay(w);

	if (DtGetAppId(dpy, _HELP_QUEUE(dpy)) == None)
	{
		// dtm not running?
		display_msg(E_NONE, GM_no_help);
		return;
	}

	// issue request to DTM 
	memset(&request, 0, sizeof(request));
	request.display_help.rqtype = DT_DISPLAY_HELP;
	request.display_help.source_type =  DT_OPEN_HELPDESK;
	request.display_help.app_name = (String)labeltab.get_label(LAB_debug);
	DtEnqueueRequest(XtScreen(w), _HELP_QUEUE(dpy),
		_DT_QUEUE(dpy), XtWindow(w), &request);
}
#endif

// Calculate padding as a function of the shadow thicknesses of
// adjacent widgets. This also makes the padding independent of
// monitor resolution, and avoids the overlapping problem when
// adjacent widgets' thicknesses add up to > PADDING
int
get_widget_pad(Widget w1, Widget w2)
{
	// no need for this in motif
	return 0;
}

XFontStruct *
get_default_font(Widget w)
{
	XmFontList fl = 0;
	XtVaGetValues(w, XmNfontList, &fl, 0);
	return get_default_font(fl);
}

XFontStruct *
get_default_font(XmFontList fl)
{
	XmFontContext f_context;
	XmFontListEntry f_entry;
	XmFontType f_type;

	XmFontListInitFontContext(&f_context, fl);
	while((f_entry = XmFontListNextEntry(f_context)) != NULL)
	{
		XtPointer fp;

		fp = XmFontListEntryGetFont(f_entry, &f_type);
		if (f_type == XmFONT_IS_FONT)
		{
			return (XFontStruct *)fp;
		}
		else if (f_type == XmFONT_IS_FONTSET)
		{
			XFontSet fset = (XFontSet )fp;
			XFontStruct **fonts;
			char **fontnames;
			int n = XFontsOfFontSet(fset, &fonts, &fontnames);
			if (n > 0)
			{
				return fonts[0];
			}
		}
	}
	return 0;
}

void
background_task(Background_proc bf, void *cdata)
{
	XtAppAddWorkProc(base_context, bf, (XtPointer)cdata);
}

Input_id
register_input_proc(int fd, Input_proc ip, void *cdata)
{
	return XtAppAddInput(base_context,
		fd, (XtPointer)XtInputReadMask, ip, (XtPointer)cdata);
}

void
unregister_input_proc(Input_id id)
{
	XtRemoveInput(id);
}

static	int		has_color = -1;
static	Colormap	default_cmap;

int
is_color()
{
	if (has_color >= 0)
		return has_color;

	int		default_depth;
	Display		*dpy = XtDisplay(base_widget);
	Screen		*screen = XtScreen(base_widget);
	int		i = 5;
	XVisualInfo	visual_info;

	default_depth = DefaultDepthOfScreen(screen);
	if (default_depth == 1)
	{
		// monochrome
		has_color = 0;
		return 0;
	}
	while(!XMatchVisualInfo(dpy, 0, default_depth, i--, 
		&visual_info))
		;
	if (i < StaticColor) 
	{
		// gray scale
		has_color = 0;
		return 0;
	}
	has_color = 1;
	default_cmap = DefaultColormapOfScreen(screen);
	return 1;
}

Pixel
get_color(char *cname)
{
	if (!has_color)
		return 0;

	XColor		sdef_return;
	XColor		edef_return;

	if (XAllocNamedColor(XtDisplay(base_widget), 
		default_cmap, cname,
		&sdef_return, &edef_return) == 0)
		return 0;
	return(sdef_return.pixel);
}

KeySym
get_mnemonic(LabelId label)
{
	char	buf[MB_LEN_MAX*2];
	wchar_t	wbuf[2];

	mbtowc(&wbuf[0], labeltab.get_label(label), MB_CUR_MAX);
	wbuf[1] = 0;
	wcstombs(buf, wbuf, MB_LEN_MAX * 2);
	return(XStringToKeysym(buf));
}

KeySym
get_mnemonic(wchar_t mne)
{
	char	buf[MB_LEN_MAX*2];
	wchar_t	wbuf[2];

	wbuf[0] = mne;
	wbuf[1] = 0;
	wcstombs(buf, wbuf, MB_LEN_MAX * 2);
	return(XStringToKeysym(buf));
}
