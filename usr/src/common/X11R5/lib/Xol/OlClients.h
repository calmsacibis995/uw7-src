#ifndef	NOIDENT
#ident	"@(#)olmisc:OlClients.h	1.20"
#endif

#ifndef __OlClients_h__
#define __OlClients_h__


typedef struct {
	long	flags;
	enum {
	MENU_FULL = 0,
	MENU_LIMITED = 1,
	MENU_NONE = 2
	} menu_type;
	long	pushpin_initial_state;	/* as for WM_PUSHPIN_STATE */
} WMDecorationHints;

#define DELIMITER	'\n'

#define WMDecorationHeader	(1L<<0)		/* has title bar */
#define WMDecorationPushpin	(1L<<2)		/* has push pin */
#define WMDecorationCloseButton	(1L<<3)		/* has shine mark */
#define WMDecorationResizeable	(1L<<5)		/* has grow corners */


#define WMWindowNotBusy		0
#define WMWindowIsBusy		1


#define WMPushpinIsOut		0
#define WMPushpinIsIn		1


typedef struct {
	int		state;
	Window		icon;
} WMState;

#define OLWSM_STATE	(1L<<0)	/* olwsm is running */
#define OLWM_STATE	(1L<<1)	/* olwm is running */
#define OLFM_STATE	(1L<<2)	/* olfm is running */

typedef struct {
	unsigned long	state;
} OLManagerState;

typedef XIconSize	WMIconSize;

#define MessageHint	(1L<<7)

#define ConfigureDenied	(1L<<0)		/* WM_CONFIGURE_DENIED */
#define WindowMoved	(1L<<1)		/* WM_WINDOW_MOVED */
#define BangMessage	(1L<<2)		/* BANG! */
#define FocusMessage	(1L<<3)		/* WM_TAKE_FOCUS */

/*
 * Following for WMState structure
 */

#define WithdrawnState	0
#define NormalState	1
#define IconicState	3


typedef struct OlWinAttr {
	unsigned long	flags;
	Atom		win_type;
	long		menu_type;
	long		pin_state;
	long		cancel;
} OLWinAttr;


typedef struct OlWinColors {
	unsigned long	flags;
	unsigned long	fg_red;
	unsigned long	fg_green;
	unsigned long	fg_blue;
	unsigned long	bg_red;
	unsigned long	bg_green;
	unsigned long	bg_blue;
	unsigned long	bd_red;
	unsigned long	bd_green;
	unsigned long	bd_blue;
} OLWinColors;

/*
 * values for _OL_WIN_ATTR flags
 */

#define	_OL_WA_WIN_TYPE		(1<<0)
#define	_OL_WA_MENU_TYPE	(1<<1)
#define	_OL_WA_PIN_STATE	(1<<2)
#define	_OL_WA_CANCEL		(1<<3)

/*
 * values for _OL_WIN_COLORS flags
 */

#define	_OL_WC_FOREGROUND	(1<<0)
#define	_OL_WC_BACKGROUND	(1<<1)
#define	_OL_WC_BORDER		(1<<2)

/*
 * for compatiblity with earlier software
 */
#define MENU_DISMISS_ONLY	MENU_NONE


#define XA_WM_DISMISS(d)	XInternAtom((d), "WM_DISMISS", False)

#define XA_WM_DECORATION_HINTS(d) \
				XInternAtom((d), "WM_DECORATION_HINTS", False)

#define XA_WM_WINDOW_MOVED(d)	XInternAtom((d), "WM_WINDOW_MOVED", False)

#define XA_WM_TAKE_FOCUS(d)	XInternAtom((d), "WM_TAKE_FOCUS", False)

#define XA_WM_DELETE_WINDOW(d)	XInternAtom((d), "WM_DELETE_WINDOW", False)

#define XA_BANG(d)		XInternAtom((d), "BANG", False)

#define XA_WM_SAVE_YOURSELF(d)	XInternAtom((d), "WM_SAVE_YOURSELF", False)

#define XA_WM_STATE(d)		XInternAtom((d), "WM_STATE", False)

#define XA_WM_CHANGE_STATE(d)	XInternAtom((d), "WM_CHANGE_STATE", False)

#define XA_WM_PROTOCOLS(d)	XInternAtom((d), "WM_PROTOCOLS", False)

#define XA_OL_COPY(d)		XInternAtom((d), "_OL_COPY", False)

#define XA_OL_CUT(d)		XInternAtom((d), "_OL_CUT", False)

#define XA_OL_HELP_KEY(d)	XInternAtom((d), "_OL_HELP_KEY", False)

#define XA_OL_WIN_ATTR(d)	XInternAtom((d), "_OL_WIN_ATTR", False)

#define XA_OL_WT_BASE(d)	XInternAtom((d), "_OL_WT_BASE", False)

#define XA_OL_WT_CMD(d)		XInternAtom((d), "_OL_WT_CMD", False)

#define XA_OL_WT_PROP(d)	XInternAtom((d), "_OL_WT_PROP", False)

#define XA_OL_WT_HELP(d)	XInternAtom((d), "_OL_WT_HELP", False)

#define XA_OL_WT_NOTICE(d)	XInternAtom((d), "_OL_WT_NOTICE", False)

#define XA_OL_WT_OTHER(d)	XInternAtom((d), "_OL_WT_OTHER", False)

#define XA_OL_DECOR_ADD(d)	XInternAtom((d), "_OL_DECOR_ADD", False)

#define XA_OL_DECOR_DEL(d)	XInternAtom((d), "_OL_DECOR_DEL", False)

#define XA_OL_DECOR_CLOSE(d)	XInternAtom((d), "_OL_DECOR_CLOSE", False)

#define XA_OL_DECOR_RESIZE(d)	XInternAtom((d), "_OL_DECOR_RESIZE", False)

#define XA_OL_DECOR_HEADER(d)	XInternAtom((d), "_OL_DECOR_HEADER", False)

#define XA_OL_DECOR_PIN(d)	XInternAtom((d), "_OL_DECOR_PIN", False)

#define XA_OL_WIN_COLORS(d)	XInternAtom((d), "_OL_WIN_COLORS", False)

#define XA_OL_PIN_STATE(d)	XInternAtom((d), "_OL_PIN_STATE", False)

#define XA_OL_WIN_BUSY(d)	XInternAtom((d), "_OL_WIN_BUSY", False)

#define XA_OL_MENU_FULL(d)	XInternAtom((d), "_OL_MENU_FULL", False)

#define XA_OL_MENU_LIMITED(d)	XInternAtom((d), "_OL_MENU_LIMITED", False)

#define XA_OL_NONE(d)		XInternAtom((d), "_OL_NONE", False)

extern void	InitializeOpenLook();
extern void	SetWMDecorationHints();
extern void	SetWMState();
extern void	SetWMWindowBusy();
extern Status	GetHelpKeyMessage();
extern void	SendProtocolMessage();
extern void	SetWMPushpinState();
extern int	GetOLWinAttr();
extern int	GetOLWinColors();

extern void     EnqueueCharProperty();
extern char *   GetCharProperty();
extern Atom *   GetAtomList();

#endif /* __OlClients_h__ */
