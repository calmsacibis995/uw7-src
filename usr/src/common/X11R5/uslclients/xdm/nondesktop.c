#ident	"@(#)xdm:nondesktop.c	1.17"

/*
 * nondesktop - handle people without graphical preferences from xdm
 */

#include <stdlib.h>		/* for getenv() */
#include <locale.h>		/* for LC_ALL */

#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>

#include <Xm/Xm.h>
#include <Xm/DragC.h>
#include <Xm/Form.h>
#include <Xm/LabelG.h>
#include <Xm/MessageB.h>
#include <Xm/PushBG.h>

#include "DesktopP.h"
#include "nondesktop.h"

typedef struct {
	Boolean		show_mne;
} AppRscs;

#if defined(__STDC__)
#define CONST	const
#else
#define CONST
#endif

	/* The following defines are copied from dm.h */
#define OBEYSESS_DISPLAY	0	/* obey multipleSession resource */
#define EXIT_XDM		5	/* kill xdm */
#define DTUSER			6	/* make user a desktop user */
#define USER_XTERM		7	/* give the user an xterm, not used */

#define DPY		XtDisplay(toplevel)
#define ROOT_WIN	RootWindowOfScreen(SCREEN)
#define SCREEN		XtScreen(toplevel)
#define SCR_WD		WidthOfScreen(SCREEN)
#define SCR_HI		HeightOfScreen(SCREEN)

#define GGT(S)		XmStringCreate(GT(S), XmFONTLIST_DEFAULT_TAG)
#define GGT_LR(S)	XmStringCreateLtoR(GT(S), XmFONTLIST_DEFAULT_TAG)
#define	GT(S)		CallGetText((CONST char *)S)

extern char *		CallGetText(CONST char *);
extern void		GetMax(Widget, Dimension *, Dimension *);
extern void		SetMsgFileName(CONST char *);

static Widget		CreateHelpDialog(Widget, Boolean);
static void		ExitCB(Widget, XtPointer, XtPointer);
static void		HelpCB(Widget, XtPointer, XtPointer);
static void		SetItUp(Widget, Boolean);
static void		UnmapCB(Widget, XtPointer, XtPointer);

static Cursor		left_ptr;
static Widget		toplevel;

#define NUM_WIDS	4

static AppRscs		rsc_table;
static XtResource	app_rscs[] = {
	{ "showMnemonic",   "ShowMnemonic",   XmRBoolean, sizeof(Boolean),
		XtOffsetOf(AppRscs, show_mne), XmRImmediate, (XtPointer)True },
};

static Widget
CreateHelpDialog(Widget toplevel, Boolean show_desktop)
{
#define GET_CHILD(T)	XmMessageBoxGetChild(dialog, T)

#define WHEN_MNE_WORKS_IN_LIBXM
#ifdef WHEN_MNE_WORKS_IN_LIBXM
#define Xm_MNE(M,II,OP)\
	if (M) {\
		mne = (unsigned char *)GT(M);\
		II.mne = (unsigned char *)mne;\
		II.mne_len = strlen((char *)mne);\
		II.op = OP;\
		if (rsc_table.show_mne) ks = XStringToKeysym((char *)mne);\
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

	Arg			args[3];
	String			string;
	Widget			dialog;
	XmString		help_str, ok_str;
	DmMnemonicInfoRec	mne_info[1];
	unsigned char *		mne;
	KeySym			ks;

	string	= show_desktop ? MSG_HELP_DTUSER : MSG_HELP_NON_DTUSER;
	help_str= GGT_LR(string);
	ok_str	= GGT(MSG_OK_BTN);

	XtSetArg(args[0], XmNmessageString, help_str);
	XtSetArg(args[1], XmNokLabelString, ok_str);
	XtSetArg(args[2], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL);

	dialog = XmCreateInformationDialog(toplevel, "dialog", args, 3);

	XtUnmanageChild(GET_CHILD(XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(GET_CHILD(XmDIALOG_HELP_BUTTON));

	XtAddCallback(dialog, XmNunmapCallback, UnmapCB, (XtPointer)toplevel);

	Xm_MNE(MSG_OK_MNE, mne_info[0], DM_B_MNE_ACTIVATE_BTN);
	mne_info[0].w = GET_CHILD(XmDIALOG_OK_BUTTON);

	XtSetArg(args[0], XmNmnemonic, ks);
	XtSetValues(mne_info[0].w, args, 1);
	DmRegisterMnemonic(dialog, mne_info, 1);

	XmStringFree(help_str);
	XmStringFree(ok_str);

	return(dialog);

#undef GET_CHILD
#undef Xm_MNE
}

static void
HelpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	static Widget	help_dialog = (Widget)NULL;

	if (!help_dialog)
		help_dialog = CreateHelpDialog(toplevel, (Boolean)client_data);

	XtManageChild(help_dialog);
	XDefineCursor(DPY, XtWindow(help_dialog), left_ptr);
	XSetInputFocus(DPY, XtWindow(help_dialog), RevertToNone, CurrentTime);
}

static void
ExitCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int	exit_code = (int)client_data;

#ifdef TESTGUI
 printf("exit_code: %d, dtuser: %d, exit_btn: %d, cancel_btn: %d\n",
			exit_code, DTUSER, EXIT_XDM, OBEYSESS_DISPLAY);
#else
	exit(exit_code);
#endif
}

static void
UnmapCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget		toplevel = (Widget)client_data;

	XSetInputFocus(DPY, XtWindow(toplevel), RevertToNone, CurrentTime);
}

static void
SetItUp(Widget toplevel, Boolean show_desktop)
{
#define HORIZ_SP	10
#define MARGIN_WD	10
#define VERT_SP		10

#define DTUSER_BTN	mne_info[0].w
#define EXIT_BTN	mne_info[1].w
#define CANCEL_BTN	mne_info[2].w
#define HELP_BTN	mne_info[3].w

#define I_DTUSER_BTN	mne_info[0]
#define I_EXIT_BTN	mne_info[1]
#define I_CANCEL_BTN	mne_info[2]
#define I_HELP_BTN	mne_info[3]

	DmMnemonicInfoRec	mne_info[NUM_WIDS];

	Arg		args[8];
	Dimension	shell_wd, shell_hi, max_wd, max_hi;
	KeySym		ks;
	String		info_string;
	Widget		main_form, info_label, bar_form, lefter, dft_btn;
	XmString	xm_str;

	int		sz, num_btns,
			center_x, center_y;
	unsigned char *	mne;

	XtSetArg(args[0], XmNmarginWidth,	MARGIN_WD);
	XtSetArg(args[1], XmNverticalSpacing,	VERT_SP);
	main_form = XtCreateManagedWidget(
			"form", xmFormWidgetClass, toplevel, args, 2);

	XtAddCallback(main_form, XmNhelpCallback, HelpCB, (XtPointer)NULL);

	info_string = show_desktop ? MSG_DTUSER : MSG_NON_DTUSER;
	xm_str = GGT_LR(info_string);

	XtSetArg(args[0], XmNleftAttachment,	XmATTACH_FORM);
	XtSetArg(args[1], XmNrightAttachment,	XmATTACH_FORM);
	XtSetArg(args[2], XmNtopAttachment,	XmATTACH_FORM);
	XtSetArg(args[3], XmNtraversalOn,	False);
	XtSetArg(args[4], XmNalignment,		XmALIGNMENT_BEGINNING);
	XtSetArg(args[5], XmNlabelString,	xm_str);

	info_label = XtCreateManagedWidget(
			"info", xmLabelGadgetClass, main_form, args, 6);

	XmStringFree(xm_str);

		/* Reuse 0-1 above */
	XtSetArg(args[2], XmNtopAttachment,	XmATTACH_WIDGET);
	XtSetArg(args[3], XmNbottomAttachment,	XmATTACH_FORM);
	XtSetArg(args[4], XmNbottomOffset,	VERT_SP);
	XtSetArg(args[5], XmNtopWidget,		info_label);
	XtSetArg(args[6], XmNhorizontalSpacing,	HORIZ_SP);
	XtSetArg(args[7], XmNresizable,		False);
	
	bar_form = XtCreateManagedWidget(
			"bar", xmFormWidgetClass, main_form, args, 8);

		/* Define the common part of button's argument list */
	XtSetArg(args[0], XmNrecomputeSize,	False);
	XtSetArg(args[1], XmNtopAttachment,	XmATTACH_FORM);
	XtSetArg(args[2], XmNbottomAttachment,	XmATTACH_FORM);
	XtSetArg(args[3], XmNleftAttachment,	XmATTACH_WIDGET);

#define WHEN_MNE_WORKS_IN_LIBXM
#ifdef WHEN_MNE_WORKS_IN_LIBXM
#define XmSTR_AND_MNE(L,M,II)\
		xm_str = GGT(L);\
		mne = (unsigned char *)GT(M);\
		II.op = DM_B_MNE_GET_FOCUS | DM_B_MNE_ACTIVATE_BTN;\
		II.mne = (unsigned char *)mne;\
		II.mne_len = strlen((char *)mne);\
		if (rsc_table.show_mne) ks = XStringToKeysym((char *)mne);\
			else ks = NoSymbol
#else
#define XmSTR_AND_MNE(L,M,II)\
		xm_str = GGT(L);\
		ks = NoSymbol
#endif

#define CREATE_BTN(W,NAME,S,LW,CB,D,II)\
		XmSTR_AND_MNE(MSG##_##S##_BTN, MSG##_##S##_MNE,II);\
		XtSetArg(args[4], XmNleftWidget,	LW);\
		XtSetArg(args[5], XmNlabelString,	xm_str);\
		XtSetArg(args[6], XmNmnemonic,		ks);\
		W = XtCreateManagedWidget(\
			NAME, xmPushButtonGadgetClass, bar_form, args, 7);\
		XtAddCallback(W, XmNactivateCallback, CB, (XtPointer)D);\
		XmStringFree(xm_str)

	if (!show_desktop)
	{
		lefter = bar_form;
		DTUSER_BTN = NULL;
		I_DTUSER_BTN.mne = NULL;
		I_DTUSER_BTN.mne_len = 0;
	}
	else
	{
		CREATE_BTN(DTUSER_BTN, "b1", DTUSER, bar_form, ExitCB, DTUSER,
							I_DTUSER_BTN);
		lefter = DTUSER_BTN;
	}
	CREATE_BTN(EXIT_BTN,   "b2", EXIT, lefter, ExitCB, EXIT_XDM,
							I_EXIT_BTN);
	CREATE_BTN(CANCEL_BTN, "b3", CANCEL, EXIT_BTN, ExitCB,OBEYSESS_DISPLAY,
							I_CANCEL_BTN);
	CREATE_BTN(HELP_BTN,   "b4", HELP, CANCEL_BTN, HelpCB, show_desktop,
							I_HELP_BTN);

	dft_btn = show_desktop ? DTUSER_BTN : EXIT_BTN;
	XtSetArg(args[0], XmNdefaultButton, dft_btn);
	XtSetValues(main_form, args, 1);

	DmRegisterMnemonic(toplevel, mne_info, NUM_WIDS);

	XtSetMappedWhenManaged(toplevel, False);
	XtRealizeWidget(toplevel);

		/* Find out the size of toplevel */
	XtSetArg(args[0], XmNwidth,  &shell_wd);
	XtSetArg(args[1], XmNheight, &shell_hi);
	XtGetValues(toplevel, args, 2);

		/* Make all buttons the same size */
	XtSetArg(args[0], XmNrecomputeSize, False);
	max_wd = max_hi = 0;
	if (!show_desktop)
	{
		num_btns = 3;
	}
	else
	{
		num_btns = 4;
		GetMax(DTUSER_BTN, &max_wd, &max_hi);
	}
	GetMax(EXIT_BTN, &max_wd, &max_hi);
	GetMax(CANCEL_BTN, &max_wd, &max_hi);
	GetMax(HELP_BTN, &max_wd, &max_hi);

	XtSetArg(args[1], XmNwidth, max_wd);
	XtSetArg(args[2], XmNheight, max_hi);

	if (show_desktop)
		XtSetValues(DTUSER_BTN, args, 3);
	XtSetValues(EXIT_BTN, args, 3);
	XtSetValues(CANCEL_BTN, args, 3);
	XtSetValues(HELP_BTN, args, 3);

		/* Now center the `bar' */
	sz = ((int)shell_wd - (int)max_wd * num_btns -
				HORIZ_SP * (num_btns - 1)) / 2 - MARGIN_WD;
	XtSetArg(args[1], XmNleftOffset, sz);
	XtSetValues(dft_btn, args, 2);

		/*
		 *  Place the warning window in the center of the screen
		 *  based on the width and height
		 */
	center_x = (SCR_WD - (int)shell_wd) / 2;
	center_y = (SCR_HI - (int)shell_hi) / 2;

	XtSetArg(args[0], XmNx, center_x);
	XtSetArg(args[1], XmNy, center_y);

	XtSetValues(toplevel, args, 2);
	XtSetMappedWhenManaged(toplevel, True);
	XtMapWidget(toplevel);

#undef HORIZ_SP
#undef MARGIN_WD
#undef VERT_SP
#undef XmSTR_AND_MNE
#undef CREATE_BTN

#undef DTUSER_BTN
#undef EXIT_BTN
#undef CANCEL_BTN
#undef HELP_BTN
#undef I_DTUSER_BTN
#undef I_EXIT_BTN
#undef I_CANCEL_BTN
#undef I_HELP_BTN
#undef ADD_EH
}

int
main (int argc, char **argv)
{
	int		ARGC = 3;	/* one less than in ARGV */
	char *		ARGV[4];
	Arg		arg[1];
	Boolean		show_desktop;
	XtAppContext	app_context;

	show_desktop = (argc == 1) ? True : False;

	SetMsgFileName((CONST char *)MSG_FILENAME);

		/* Assume we can rely on the LANG environment, which is
		 * TRUE in our product */
	ARGV[0] = argv[0];
	ARGV[1] = "-xnllanguage";	/* same effect as -xrm *xnlLanguage:?*/
	if ((ARGV[2] = getenv("LANG")) == NULL)
	{
		ARGV[2] = "C";
	}
	ARGV[3] = 0;

		/* register default language proc, so that setlocale()
		 * will be called, and will hook up the right input
		 * method, and other I18N related stuff... */
	XtSetLanguageProc(NULL, NULL, NULL);


	XtSetArg(arg[0], XmNallowShellResize, False);
	toplevel = XtAppInitialize(
			&app_context, "Nondesktop", NULL, 0,
			&ARGC, ARGV, NULL, arg, 1);

	XtGetApplicationResources(toplevel, (XtPointer)&rsc_table,
		app_rscs, XtNumber(app_rscs), NULL, 0);
#ifdef TESTGUI
 printf("%s (%d): rsc_table.show_mne: %d\n",
		__FILE__, __LINE__, rsc_table.show_mne);
#endif

	SetItUp(toplevel, show_desktop);

		/* Disable drag-and-drop */
	XtSetArg(arg[0], XmNdragInitiatorProtocolStyle, XmDRAG_NONE);
	XtSetValues(XmGetXmDisplay(DPY), arg, 1);

	left_ptr = XCreateFontCursor(DPY, XC_left_ptr);
	XDefineCursor(DPY, XtWindow(toplevel), left_ptr);
	XSetInputFocus(DPY, XtWindow(toplevel), RevertToNone, CurrentTime);
	XWarpPointer(DPY, None, ROOT_WIN, 0, 0, 0, 0, SCR_WD / 2, SCR_HI / 2);

	XtAppMainLoop(app_context);
}
