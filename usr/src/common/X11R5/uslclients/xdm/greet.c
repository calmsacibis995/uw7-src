#ident	"@(#)xdm:greet.c	1.25"
/*
 * xdm - display manager daemon
 *
 * $XConsortium: greet.c,v 1.29 91/04/02 11:58:51 rws Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

/*
 * Get username/password
 *
 */

#include "dtlogin.h"
#include "Xm/DragC.h"
#include "Xm/MessageB.h"
#include <X11/cursorfont.h>
#include <X11/Shell.h>

#include "DesktopP.h"

#include "dm.h"

		/* Assume `dpy' */
#define SCREEN		DefaultScreenOfDisplay(dpy)
#define ROOT_WIN	RootWindowOfScreen(SCREEN)
#define COLORMAP	DefaultColormapOfScreen(SCREEN)


		/* External functions not defined in any header */
extern void		CloseGreet(struct display *);
extern Display *	InitGreet(struct display *);
extern void		SetBackgroundSolid(Display *);
extern void		XdmMainLoop(struct display *, unsigned long);

		/* External variables not defined in any header */
extern int		loginProblem;

		/* Global variable declarions */
LoginInfo		login_info;

		/* Private functions */
static void		GreetPingServer(XtPointer, XtIntervalId *);
static void		UnmapCB(Widget, XtPointer, XtPointer);

static void
UnmapCB(Widget w, XtPointer client_data, XtPointer call_data)
{
		/* name client_data as login_info for marco convenience */
	LoginInfo	login_info = (LoginInfo)client_data;
	Window		window;

	if (DOING_GREET())
		window = XtWindow(TOPLEVEL);
	else
		window = XtWindow(POPUP);
	XSetInputFocus(DPY, window, RevertToNone, CurrentTime);
}

extern void
PopupDialog(int reason, XmString msg_str, unsigned long flag)
{
#define DISPLAY		XtDisplay(dialog)
#define GET_CHILD(T)	XmMessageBoxGetChild(dialog, T)
#define WINDOW		XtWindow(dialog)
#define DIALOG_TYPE	info[reason].dialog_type
#define CREATE_DIALOG	info[reason].create_dialog

	typedef Widget (*CreateDialogFunc)(Widget, String, ArgList, Cardinal);
	typedef struct {
		unsigned char		dialog_type;
		CreateDialogFunc	create_dialog;
	} Token;

		/* See dtlogin.h for #define(s) */
	static CONST Token info[NUM_REASONS] = {
/* NOT_DEFINED */	0,			NULL,
/* ACCOUNT_AGED */	XmDIALOG_WARNING,	XmCreateWarningDialog,
/* HELP_MAIN */		XmDIALOG_INFORMATION,	XmCreateInformationDialog,
/* LOGIN_FAILED */	XmDIALOG_ERROR,		XmCreateErrorDialog,
/* PASSWORD_AGED */	XmDIALOG_WARNING,	XmCreateWarningDialog,
/* HELP_POPUP */	XmDIALOG_INFORMATION,	XmCreateInformationDialog,
/* NEW_PASSWD_ERR */	XmDIALOG_ERROR,		XmCreateErrorDialog,
/* BADSHELL */		XmDIALOG_ERROR,		XmCreateErrorDialog,
/* NOHOME */		XmDIALOG_ERROR,		XmCreateErrorDialog,
	};

	Arg			args[3];
	static Widget		dialog = NULL;
	static int		last_reason = NOT_DEFINED;

	if (reason >= NUM_REASONS)
		return;		/* Can't handle this */

	XtSetArg(args[0], XmNmessageString, msg_str);

	if (dialog)	/* Already created */
	{
			/* Instead of defining 10 different values, simply
			 * do it without opimization for NEW_PASSWD_ERR */
		if (reason != last_reason || reason == NEW_PASSWD_ERR)
		{
			XtSetArg(args[1], XmNdialogType, DIALOG_TYPE);
			XtSetValues(dialog, args, 2);
		}
	}
	else		/* First time */
	{

#define WHEN_MNE_WORKS_IN_LIBXM
#ifdef WHEN_MNE_WORKS_IN_LIBXM
#define Xm_MNE(M,II,OP)\
	if (M) {\
		mne = (unsigned char *)GT(M);\
		II.mne = (unsigned char *)mne;\
		II.mne_len = strlen((char *)mne);\
		II.op = OP;\
		if (show_mne) ks = XStringToKeysym((char *)mne);\
			else ks = NoSymbol;\
	} else {\
		II.mne=NULL;\
		II.mne_len=0;\
		II.op=DM_B_MNE_NOOP;\
		ks = NoSymbol;\
	}
#else
#define Xm_MNE(M,II,OP)\
	xm_str = GGT(S);\
	ks = NoSymbol
#endif

		extern int		show_mne;
		unsigned char *		mne;
		KeySym			ks;
		DmMnemonicInfoRec	mne_info[1];

		XmString		ok_label;

		ok_label = GGT(MSG_OK_BTN);
		XtSetArg(args[1], XmNokLabelString, ok_label);
		XtSetArg(args[2], XmNdialogStyle,
					XmDIALOG_PRIMARY_APPLICATION_MODAL);

		dialog = (*CREATE_DIALOG)(TOPLEVEL, "", args, 3);

		XmStringFree(ok_label);

		XtUnmanageChild(GET_CHILD(XmDIALOG_CANCEL_BUTTON));
		XtUnmanageChild(GET_CHILD(XmDIALOG_HELP_BUTTON));

		XtAddCallback(dialog, XmNunmapCallback, UnmapCB,
							(XtPointer)login_info);

		Xm_MNE(MSG_OK_BTN_MNE, mne_info[0], DM_B_MNE_ACTIVATE_BTN);
		mne_info[0].w = GET_CHILD(XmDIALOG_OK_BUTTON);

		XtSetArg(args[0], XmNmnemonic, ks);
		XtSetValues(mne_info[0].w, args, 1);
		DmRegisterMnemonic(dialog, mne_info, 1);
	}

		/* This should only happen for BADSHELL and NOHOME and
		 * it will happen when dialog is NULL, put it here for
		 * easier expansion later on when the above assumption
		 * is no longer True */
	if (flag & FREE_XMSTR)
		XmStringFree(msg_str);

	if (flag & RING_BELL)
		XBell(DISPLAY, 0);

	last_reason = reason;

	XtManageChild(dialog);

	XDefineCursor(DISPLAY, WINDOW, LEFT_PTR);
	XSetInputFocus(DISPLAY, WINDOW, RevertToNone, CurrentTime);

#undef GET_CHILD
#undef DISPLAY
#undef WINDOW
#undef DIALOG_TYPE
#undef CREATE_DIALOG
}

/*ARGSUSED*/
static void
GreetPingServer(XtPointer closure, XtIntervalId *intervalId)
{
	struct display *	d = (struct display *)closure;

	if (!PingServer(d, DPY))
		SessionPingFailed(d);

	PING_TIMEOUT = XtAppAddTimeOut(
				APP_CONTEXT, d->pingInterval * 60 * 1000,
				GreetPingServer, closure);
}

extern Display *
InitGreet(struct display * d)
{
	int	argc = 3;	/* one less than in argv */
	char *	argv[4];

	Arg		args[1];
	Display *	dpy;
	XtAppContext	context;

	Debug("InitGreet: greet %s\n", d->name? d->name : "<NULL>");

	if (!(login_info = (LoginInfo)malloc(sizeof(LoginInfoRec))))
	{
		Debug("Could not allocate memory for widget structure\n");
		exit(EXIT_XDM);
	}

	SetMsgFileName((CONST char *)MSG_FILENAME);

		/* Assume we can rely on the LANG environment, which is
		 * TRUE in our product */
	argv[0] = "dtlogin";
	argv[1] = "-xnllanguage";	/* same effect as -xrm *xnlLanguage:?*/
	if ((argv[2] = getenv("LANG")) == NULL)
	{
		argv[2] = "C";
	}
	argv[3] = 0;

	STATUS		= 0;

	LOGIN_ID	= NULL;
	PASSWORD[0]	= 0;

		/* register default language proc, so that setlocale()
		 * will be called, and will hook up the right input
		 * method, and other I18N related stuff... */
	XtSetLanguageProc(NULL, NULL, NULL);

	XtToolkitInitialize();
	context = XtCreateApplicationContext();
	if (!(dpy = XtOpenDisplay(context,
			d->name, "dtlogin", "Dtlogin", 0, 0, &argc, argv))) {
		return(0);
	}

		/* Have to initialize I18N messages from this point on because
		 * setlocale() is called from either XtOpenDisplay() (i.e.,
		 * XtInitialize/XtAppInitialize or XtDisplayInitialize */
	ACCOUNT_AGED_M	= GGT(MSG_ACCOUNT_AGED);
	LOGIN_FAILED_M	= GGT(MSG_LOGIN_FAILED);
	PASSWORD_AGED_M = GGT(MSG_PASSWD_AGED);
	PASSWD_M	= GGT(MSG_PASSWORD);
	HELP_MAIN_M	= GGT_LR(GT(MSG_HELP_MAIN));

	RegisterCloseOnFork(ConnectionNumber(dpy));

	SecureDisplay(d, dpy);

	if (backgroundPixmap)
	{
#define DEPTH		DefaultDepthOfScreen(SCREEN)

		unsigned int	ww, hh;
		Pixmap		pixmap;

		if (XReadPixmapFile(dpy, ROOT_WIN, COLORMAP,
				backgroundPixmap, &ww, &hh, DEPTH, &pixmap))
		{
			LogError("unable to read pixmap %s\n",
							backgroundPixmap);
		}
		else
		{
			XSetWindowBackgroundPixmap(dpy, ROOT_WIN, pixmap);
			XFreePixmap(dpy, pixmap);
			XClearWindow(dpy, ROOT_WIN);
		}
#undef DEPTH
	}

	TOPLEVEL = CreateLoginArea(dpy, context);

		/* Disable drag-and-drop */
	XtSetArg(args[0], XmNdragInitiatorProtocolStyle, XmDRAG_NONE);
	XtSetValues(XmGetXmDisplay(DPY), args, 1);

	LEFT_PTR = XCreateFontCursor(dpy, XC_left_ptr);
	XDefineCursor(dpy, XtWindow(TOPLEVEL), LEFT_PTR);

	XSetInputFocus(DPY, XtWindow(TOPLEVEL), RevertToNone, CurrentTime);

	if (!d->pingInterval)
	{
		PING_TIMEOUT = 0;
	}
	else
	{
		PING_TIMEOUT = XtAppAddTimeOut(
				context, d->pingInterval * 60 * 1000,
				GreetPingServer, (XtPointer)d);
	}

	if (loginProblem)
	{
		String		str;

		str = (loginProblem == BADSHELL) ? MSG_BADSHELL : MSG_NOHOME;

			/* Force all pending exposure events to be processed
			 * before popping up the dialog. Otherwise, the
			 * contents of the login window may not be showed
			 * completely (e.g., Login ID and Password strings).
			 *
			 * Motif toolkit problem? or
			 * X server problem (save under)?
			 * Work around this problem for now!! */
		XmUpdateDisplay(TOPLEVEL);
		PopupDialog(loginProblem, GGT(str), RING_BELL | FREE_XMSTR);
	}

	return dpy;
}

extern void
CloseGreet(struct display * d)
{
	Display *	dpy = DPY;

	if (PING_TIMEOUT)
	{
		XtRemoveTimeOut(PING_TIMEOUT);
		PING_TIMEOUT = 0;
	}

	UnsecureDisplay(d, dpy);
/* The call is not necessary because this child will go away after
 * returning it to the caller...
 *
 *	XtDestroyWidget(TOPLEVEL);
 */
	ClearCloseOnFork(ConnectionNumber(dpy));
	ClearCloseOnFork(ConnectionNumber(dpy));
	XCloseDisplay(dpy);
}

extern void
XdmMainLoop(struct display * d, unsigned long bit)
{
	XEvent		event;

	RESET_STATUS(bit);
	while (!CHK_STATUS(bit))
	{
		XtAppNextEvent(APP_CONTEXT, &event);
		XtDispatchEvent(&event);
	}

	XFlush(DPY);
}

extern void
SetBackgroundSolid(Display * dpy)
{
	char *		color_name;
	XColor		ecolor;

	color_name = "White";
	if (!XParseColor(dpy, COLORMAP, color_name, &ecolor))
	{
		LogError("Unknown color, %s\n", color_name);
		return;
	}

	if (!XAllocColor(dpy, COLORMAP, &ecolor))
	{
		LogError("Unable to allocate color for %s\n", color_name);
		return;
	}

	XSetWindowBackground(dpy, ROOT_WIN, ecolor.pixel);
	XClearWindow(dpy, ROOT_WIN);

}
