#ident	"@(#)xdm:dtlogin.c	1.42"

#include "dtlogin.h"
#include "X11/Shell.h"
#include "Xm/Form.h"
#include "Xm/LabelG.h"
#include "Xm/TextF.h"
#include "Xm/PushBG.h"
#include "Xm/RowColumn.h"

#include "DesktopP.h"

#include <pwd.h>
#include <grp.h>
#include <shadow.h>
#include <ia.h>
#include <sys/utsname.h>

#if defined(__STDC__)
#include <stdlib.h>
#else
extern	char	*getenv();
#endif

#include "dm.h"

#define	AGE_UID		"/etc/security/ia/ageduid"
#define DPY		XtDisplay(TOPLEVEL)
#define SCREEN		XtScreen(TOPLEVEL)
typedef	struct	passwd	UserRec, *UserPtr;

	/* External functions not defined in any header */
extern int		GetPasswdMinLen(void);
extern void		NoHome(void);
extern void		ShellFail(void);
extern int		StorePasswd(char *, char *);

	/* External variables not defined in any header */
extern int		loginProblem;

	/* Private functions */
static int		cmplogin(UserPtr, UserPtr);
static int		cmpuid(int *, int *);
static void		ExitCB(Widget, XtPointer, XtPointer);
static void		FreeUserList(void);
static void		HelpCB(Widget, XtPointer, XtPointer);
static void		MakeUserList(void);
static void		LoginCB(Widget, XtPointer, XtPointer);
static void		LoginVerifyCB(Widget, XtPointer, XtPointer);
static void		NewPasswdOkCB(Widget, XtPointer, XtPointer);
static void		NewPasswdVerifyCB(Widget, XtPointer, XtPointer);
static void		PasswdFocusCB(Widget, XtPointer, XtPointer);
static void		PasswdBSAction(Widget, XEvent *, String *, Cardinal *);
static void		PasswdInsertAction(Widget, XEvent*, String*, Cardinal*);
static void		ResetCB(Widget, XtPointer, XtPointer);

	/* Static variables */
static	UserPtr		u_list = (UserPtr)0;
static	int		u_cnt = 0;
static	int *		uid_list;

static void
FreeUserList(void)
{
	UserPtr	up;
	char	*p;

	free (uid_list);
	while (u_cnt--) {
		up = &u_list[u_cnt];
		if (p = up->pw_name)	free(p);
		if (p = up->pw_comment)	free(p);
		if (p = up->pw_dir)	free(p);
		if (p = up->pw_shell)	free(p);
	}
	free (u_list);
	u_list = (UserPtr)0;
	u_cnt = 0;
}

static int
cmpuid(int *m, int *n)
{
	return *m - *n;
}

static int
cmplogin(UserPtr x, UserPtr y)
{
	return strcoll(x->pw_name, y->pw_name);
}

#define	BUNCH	16
#define	U_QUANT	(BUNCH*sizeof(UserRec))

static void
MakeUserList(void)
{
struct	passwd	*pwd;
	FILE	*fp;
	UserPtr	up;
	char	buf[40];
	int	n;

	if (u_list)
		FreeUserList();
	while (pwd = getpwent()) {
		if (u_cnt == 0) {
			u_list = (UserPtr)malloc(U_QUANT);
		}
		else if (u_cnt % BUNCH == 0) {
			u_list = (UserPtr)realloc((void *)u_list,
						(1+(u_cnt/BUNCH))*U_QUANT);
		}
		up = &u_list[u_cnt++];
		up->pw_name = strdup(pwd->pw_name);
		up->pw_uid = pwd->pw_uid;
		up->pw_gid = pwd->pw_gid;
		up->pw_comment = pwd->pw_comment? strdup(pwd->pw_comment): NULL;
		up->pw_dir = pwd->pw_dir? strdup(pwd->pw_dir): NULL;
		up->pw_shell = pwd->pw_shell? strdup(pwd->pw_shell): NULL;
	}
	endpwent();
	if (uid_list = (int *)malloc(u_cnt*sizeof(int))) {
		for (n = 0; n < u_cnt; n++)
			uid_list[n] = u_list[n].pw_uid;
		/*
		 *	attach ageing uids to the list, so they won't
		 *	be chosen by default (this still requires a
		 *	a test in Validate(), as the user my try override
		 */
		if (fp=fopen(AGE_UID,"r")) {
			while (fgets(buf, 40, fp)) {
				uid_list = (int *)realloc(uid_list,
							sizeof(int)*(n+1));
				uid_list[n++] = atoi(buf);
			}
			fclose(fp);
		}
		qsort((void *)uid_list, n, sizeof(int), (int (*) ())cmpuid);
	}
	qsort((void *)u_list, u_cnt, sizeof(UserRec), (int (*) ())cmplogin);
}

extern void
ShellFail(void)
{
	Debug("ShellFail\n");
	loginProblem = BADSHELL;
}

extern void
NoHome(void)
{
	Debug("NoHome\n");
	loginProblem = NOHOME;
}

extern void
ClearTextFields(LoginInfo login_info)
{
	Widget		focus_widget, button;
	unsigned long	mask;

	if (DOING_GREET())
	{
			/* LOGIN_ID will be reset in LoginVerifyCB because of
			 * XmTextFieldSetString(LOGIN_TF, NULL) call below. */
		PASSWORD[0] = 0;
		XmTextFieldSetString(LOGIN_TF, NULL);

		button		= LOGIN_BTN;
		mask		= LOGIN_BTN_BIT;
		focus_widget	= LOGIN_TF;
	}
	else
	{
		NEW_PASSWORD[0]		=
		VERIFY_PASSWORD[0]	= 0;

		button		= OK_BTN;
		mask		= OK_BTN_BIT;
		focus_widget	= NEW_PASSWD_TF;
	}

	XmProcessTraversal(focus_widget, XmTRAVERSE_CURRENT);
}

static void
ResetCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	SET_STATUS(RESET_CB_BIT);
	ClearTextFields((LoginInfo)client_data);
	RESET_STATUS(RESET_CB_BIT);
}

static void
LoginCB(Widget w, XtPointer client_data, XtPointer call_data)
{
		/* Name client_data as login_info for marco convenience */
	LoginInfo	login_info = (LoginInfo)client_data;
	Boolean		doit = True;

	if (!LOGIN_ID)
	{
		String	ptr;

		if ((ptr = XmTextFieldGetString(LOGIN_TF)) && *ptr)
		{
			LOGIN_ID = ptr;
		}
		else
		{
			doit = False;
				/* ptr is not NULL even when !strlen(ptr) */ 
			XtFree(ptr);
			ClearTextFields(login_info);
			PopupDialog(LOGIN_FAILED, LOGIN_FAILED_M, RING_BELL);
		}
	}
#ifdef TESTGUI
 printf("%s (%d), ?LoginCB, ID: %s PASSWD: %s\n", __FILE__, __LINE__,
	LOGIN_ID ? LOGIN_ID : "NULL", PASSWORD ? PASSWORD : "NULL");
#endif

	if (doit)
	{
		SET_STATUS(GREET_DONE_BIT);
	}
}

static void
ExitCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	if (DOING_GREET())
	{
		exit(EXIT_XDM);
	}
	else
	{
		/* Name client_data as login_info for marco convenience */
		LoginInfo	login_info = (LoginInfo)client_data;

		SET_STATUS(NEW_PASSWD_DONE_BIT);
#if 0
		XLowerWindow(DPY, XtWindow(POPUP));
#else
		XtUnmapWidget(POPUP);
#endif
		RESET_STATUS(NEW_PASSWD_REASON_BITS);
		XmProcessTraversal(NEW_PASSWD_TF, XmTRAVERSE_CURRENT);
		XSetInputFocus(
			DPY, XtWindow(TOPLEVEL), RevertToNone, CurrentTime);
	}
}

static void
HelpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
		/* Name client_data as login_info for marco convenience */
	LoginInfo	login_info = (LoginInfo)client_data;
	int		reason;
	unsigned long	flag;
	XmString	msg_str;
	Widget		focus_widget;

		/* Set focus to one of the textfield before popup the dialog */
		/* Or should the focus stay where it was? */
	if (DOING_GREET())
	{
		reason	= HELP_MAIN;
		msg_str	= HELP_MAIN_M;
		flag	= DO_NOTHING;
		focus_widget = CHK_STATUS(PASSWD_BIT) ? PASSWD_TF : LOGIN_TF;
	}
	else
	{
		reason	= HELP_POPUP;
		msg_str	= GGT_LR(GT(MSG_HELP_POPUP));
		flag	= FREE_XMSTR;
		focus_widget = CHK_STATUS(VERIFY_PASSWD_BIT) ?
					VERIFY_PASSWD_TF : NEW_PASSWD_TF;
	}

	XmProcessTraversal(focus_widget, XmTRAVERSE_CURRENT);
	PopupDialog(reason, msg_str, flag);
}

static void
PasswdFocusCB(Widget w, XtPointer client_data, XtPointer call_data)
{
		/* name client_data as login_info for marco convenience */
	LoginInfo		login_info = (LoginInfo)client_data;
	XmAnyCallbackStruct *	cd = (XmAnyCallbackStruct *)call_data;
	Arg			args[1];

	if (cd->reason == XmCR_FOCUS)
	{
		if (DOING_GREET())
		{
			DPY_PASSWD_M;
		}
		else
		{
			SET_STATUS(VERIFY_PASSWD_BIT);
		}
	}
	else if (cd->reason == XmCR_LOSING_FOCUS)
	{
		if (DOING_GREET())
		{
			DPY_LOGIN_ID_M;
		}
		else
		{
			RESET_STATUS(VERIFY_PASSWD_BIT);
		}
	}
}

static void
LoginVerifyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
		/* name client_data as login_info for marco convenience */
	LoginInfo		login_info = (LoginInfo)client_data;
	XmTextVerifyPtr		cd = (XmTextVerifyPtr)call_data;

	if (cd->reason == XmCR_MODIFYING_TEXT_VALUE)
	{
#ifdef TESTGUI
 printf("%s (%d), ?LoginVerifyCB because MODIFY\n", __FILE__, __LINE__);
#endif
			/* Free LOGIN_ID if it's not NULL... */
		if (LOGIN_ID)
		{
#ifdef TESTGUI
 printf("\t%s (%d): Freeing LOGIN_ID (%s)\n", __FILE__, __LINE__, LOGIN_ID);
 printf("\t%s (%d): Rm ModifyCB\n", __FILE__, __LINE__);
#endif
			XtFree(LOGIN_ID);
			LOGIN_ID = NULL;
			XtRemoveCallback(w, XmNmodifyVerifyCallback,
						LoginVerifyCB, client_data);
		}
	}
	else if (cd->reason == XmCR_LOSING_FOCUS)
	{
		String		ptr;

#ifdef TESTGUI
 printf("%s (%d), ?LoginVerifyCB because LOSE FOCUS\n", __FILE__, __LINE__);
#endif
			/* If LOGIN_ID is set, it's because the user
			 * clicks the mouse button, so just return. */
		if (LOGIN_ID)
			return;

		if ((ptr = XmTextFieldGetString(w)) && *ptr)
		{
#ifdef TESTGUI
 printf("\t\t%s (%d)Adding modifyCB\n", __FILE__, __LINE__);
#endif
			LOGIN_ID = ptr;
			XtAddCallback(w, XmNmodifyVerifyCallback,
						LoginVerifyCB, client_data);
		}
		else
		{
				/* ptr is not NULL even when !strlen(ptr) */ 
			XtFree(ptr);
		}
	}
}

/*
 * PasswdBSAction - replacement of TextF.c:backspace().
 *	Delete the last char of the password string for each `bs' key press
 *	and also don't move the cursor position in the textfield widget.
 */
static void
PasswdBSAction(Widget w, XEvent *xe, String *params, Cardinal *num_params)
{
	/* See CreateLoginArea:passwd_trans[], #defines are
	 * close related to this translation table. No checking
	 * is done here assuming that we know what we are doing!!! */
#define DELETE_LINE	'1'
#define DELETE_CHAR	'2'

	/* GET_BUF uses by both PasswdBSAction and PasswdInsertAction */
#define GET_BUF(B)	if (DOING_GREET()) B = PASSWORD;\
			else if (CHK_STATUS(VERIFY_PASSWD_BIT))\
				B = VERIFY_PASSWORD; else B = NEW_PASSWORD

	int	n;
	String	buf;

	GET_BUF(buf);
	if (n = strlen(buf))
	{
		switch(*params[0])
		{
			case DELETE_LINE:
				buf[0] = 0;
				break;
			case DELETE_CHAR:
				buf[n - 1] = 0;
				break;
			default:
				break;
		}
	}

#undef DELETE_LINE
#undef DELETE_CHAR
}

/*
 * PasswdInsertAction - replacement of TextF.c:self-insert(). The default
 *	action routine discards control characters, which are allowed in
 *	the password field.
 */
static void
PasswdInsertAction(Widget w, XEvent *xe, String *params, Cardinal *num_params)
{
	char		value[32];
	XComposeStatus	status;
	int		n;

		/* We should use XmImMbLookupString but I can't locate it
		 * from the man pages, is this function a public one? */
	if ((n = XLookupString(
			(XKeyEvent *)xe,
			value, sizeof(value), (KeySym *)NULL, &status)) > 0) {

		int	len;
		String	buf;

		value[n] = 0;
#ifdef TESTGUI
		printf("%s (%d), type %s\n", __FILE__, __LINE__, value);
#endif
		GET_BUF(buf);
		len = strlen(buf) + strlen(value) + 1;
		if (len < MAXFLD_LEN) {
			strcat(buf, value);
		}
#ifdef TESTGUI
		else
			printf("%s (%d), buffer overflow\n", __FILE__,__LINE__);
#endif
	}

#undef GET_BUF
}

	/* Enable USE_FORM shall be more efficient, but we need to
	 * handle odd situation in other locales, e.g., get really
	 * long translated string which may clip button lables, so
	 * use XmRowColumn instead. This is because XmForm is not
	 * smart enough for relaying out by changing
	 * ref_widget(s) in set_values(). */
#undef	USE_FORM
static Widget
CreateShell(Display * dpy, XtCallbackProc help_cb, Pixmap logo,
	Widget * title,  XmString title_msg,
	WidgetList wids,
	String cap1_s, String cap1_mne, XtCallbackProc tf1_cb,
	String cap2_s, String cap2_mne, XtCallbackProc tf2_cb,
	String name1, String mne1, XtCallbackProc btn1_cb,
	String name3, String mne3,
	Widget * status_label, XmString status_msg,
	Boolean first_toplevel, Boolean tf1_want_passwd_trans)
{
#define SCR		DefaultScreenOfDisplay(dpy)
#define ROOT_WIN	RootWindowOfScreen(SCR)
#define SCR_WD		WidthOfScreen(SCR)
#define SCR_HI		HeightOfScreen(SCR)
#define MARGIN_WD	10
#define VERT_SP		10

#define TF1		wids[0]
#define TF2		wids[1]
#define BTN1		wids[2]
#define BTN2		wids[3]
#define BTN3		wids[4]
#define BTN4		wids[5]

#define I_TF1		mne_info[0]
#define I_TF2		mne_info[1]
#define I_BTN1		mne_info[2]
#define I_BTN2		mne_info[3]
#define I_BTN3		mne_info[4]
#define I_BTN4		mne_info[5]

#define ADD_EH(W)	if (show_mne) XtInsertRawEventHandler(W, KeyPressMask,\
				False, MneEventHandler, (XtPointer)NULL,\
				XtListHead)

	extern int		show_mne;

	DmMnemonicInfoRec	mne_info[NUM_WIDS];

	int		i, sz, cap_wd, center_x, center_y;
	int		num_btns;
	unsigned char *	mne;
	Arg		args[10];	/* used 9 */
	Dimension	max_wd, max_hi, shell_wd, shell_hi;
	KeySym		ks;
	Widget		parent, main_form, logo_w,
			form1, cap1, form2, cap2,
			bar_form, copyright;
	XmString	xm_str;

	XtSetArg(args[0], XmNscreen, (XtArgVal)SCR);
	XtSetArg(args[1], XmNallowShellResize, (XtArgVal)True);
	parent = XtAppCreateShell(
			"dtlogin", "Dtlogin",
			applicationShellWidgetClass, dpy,
			args, 2
	);

	XtSetArg(args[0], XmNmarginWidth, (XtArgVal)MARGIN_WD);
	XtSetArg(args[1], XmNverticalSpacing, (XtArgVal)VERT_SP);
	main_form = XtCreateManagedWidget(
			"main_form", xmFormWidgetClass, parent,
			args, 2
	);

	XtAddCallback(
		main_form, XmNhelpCallback, help_cb, (XtPointer)login_info);

	if (logo != (Pixmap)None)
	{
		XtSetArg(args[0], XmNleftAttachment, (XtArgVal)XmATTACH_FORM);
		XtSetArg(args[1], XmNrightAttachment, (XtArgVal)XmATTACH_FORM);
		XtSetArg(args[2], XmNtraversalOn, (XtArgVal)False);
		XtSetArg(args[3], XmNalignment, (XtArgVal)XmALIGNMENT_CENTER);
		XtSetArg(args[4], XmNtopAttachment, (XtArgVal)XmATTACH_FORM);
		XtSetArg(args[5], XmNlabelType, (XtArgVal)XmPIXMAP);
		XtSetArg(args[6], XmNlabelPixmap, (XtArgVal)logo);
		logo_w = XtCreateManagedWidget(
				"", xmLabelGadgetClass, main_form,
				args, 7
		);
		XtSetArg(args[4], XmNtopAttachment, (XtArgVal)XmATTACH_WIDGET);
		XtSetArg(args[5], XmNtopWidget, (XtArgVal)logo_w);
		i = 6;
	}
	else
	{
		XtSetArg(args[0], XmNleftAttachment, (XtArgVal)XmATTACH_FORM);
		XtSetArg(args[1], XmNrightAttachment, (XtArgVal)XmATTACH_FORM);
		XtSetArg(args[2], XmNtopAttachment, (XtArgVal)XmATTACH_FORM);
		XtSetArg(args[3], XmNtraversalOn, (XtArgVal)False);
		XtSetArg(args[4], XmNalignment, (XtArgVal)XmALIGNMENT_CENTER);
		i = 5;
	}

	XtSetArg(args[i], XmNlabelString, (XtArgVal)title_msg); i++;
	*title = XtCreateManagedWidget(
			"title", xmLabelGadgetClass, main_form,
			args, i
	);

		/* Resue 0-1 above (see *title) */
	XtSetArg(args[2], XmNtopAttachment, (XtArgVal)XmATTACH_WIDGET);
	XtSetArg(args[3], XmNtopWidget, (XtArgVal)*title);
	XtSetArg(args[4], XmNresizable, (XtArgVal)False);
	form1 = XtCreateManagedWidget(
			"form1", xmFormWidgetClass, main_form,
			args, 5
	);

		/* mne/ks code is from MGizmo, except that
		 * I don't do checking... */
#define WHEN_MNE_WORKS_IN_LIBXM
#ifdef WHEN_MNE_WORKS_IN_LIBXM
#define XmSTR_AND_MNE(S,M,II,OP)\
	xm_str = GGT(S);\
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
#define XmSTR_AND_MNE(S,M,II,OP)\
	xm_str = GGT(S);\
	ks = NoSymbol
#endif

	XmSTR_AND_MNE(cap1_s, cap1_mne, I_TF1, DM_B_MNE_GET_FOCUS);
	XtSetArg(args[0], XmNtopAttachment, (XtArgVal)XmATTACH_FORM);
	XtSetArg(args[1], XmNbottomAttachment, (XtArgVal)XmATTACH_FORM);
	XtSetArg(args[2], XmNtraversalOn, (XtArgVal)False);
	XtSetArg(args[3], XmNalignment, (XtArgVal)XmALIGNMENT_END);
	XtSetArg(args[4], XmNrecomputeSize, (XtArgVal)False);
	XtSetArg(args[5], XmNlabelString, (XtArgVal)xm_str);
	XtSetArg(args[6], XmNmnemonic, (XtArgVal)ks);
	cap1 = XtCreateManagedWidget(
			"cap1", xmLabelGadgetClass, form1,
			args, 7
	);
	XmStringFree(xm_str);

/* Augment translation to recognize RETURN, treat RETURN as TAB */

		/* Reuse 0-1 above (see cap1) */
	XtSetArg(args[2], XmNleftAttachment, (XtArgVal)XmATTACH_WIDGET);
	XtSetArg(args[3], XmNleftWidget, (XtArgVal)cap1);
	XtSetArg(args[4], XmNselectionArrayCount, (XtArgVal)1);
	XtSetArg(args[5], XmNmaxLength, (XtArgVal)80);
	XtSetArg(args[6], XmNverifyBell, (XtArgVal)False);
	TF1 = I_TF1.w = XtCreateManagedWidget(
			"tf1", xmTextFieldWidgetClass, form1,
			args, 7
	);
	XtAddCallback(
		TF1, XmNlosingFocusCallback, tf1_cb, (XtPointer)login_info);

	if (tf1_want_passwd_trans)
	{
		XtSetArg(args[0], XmNtranslations, (XtArgVal)PASSWD_TRANS);
		XtSetValues(TF1, args, 1);
	}
	XtOverrideTranslations(TF1, TF1_TRANS);

	XtSetArg(args[0], XmNleftAttachment, (XtArgVal)XmATTACH_FORM);
	XtSetArg(args[1], XmNrightAttachment, (XtArgVal)XmATTACH_FORM);
	XtSetArg(args[2], XmNtopAttachment, (XtArgVal)XmATTACH_WIDGET);
	XtSetArg(args[3], XmNtopWidget, (XtArgVal)form1);
	XtSetArg(args[4], XmNresizable, (XtArgVal)False);
	form2 = XtCreateManagedWidget(
			"form2", xmFormWidgetClass, main_form,
			args, 5
	);

	XmSTR_AND_MNE(cap2_s, cap2_mne, I_TF2, DM_B_MNE_GET_FOCUS);
	XtSetArg(args[0], XmNtopAttachment, (XtArgVal)XmATTACH_FORM);
	XtSetArg(args[1], XmNbottomAttachment, (XtArgVal)XmATTACH_FORM);
	XtSetArg(args[2], XmNalignment, (XtArgVal)XmALIGNMENT_END);
	XtSetArg(args[3], XmNtraversalOn, (XtArgVal)False);
	XtSetArg(args[4], XmNrecomputeSize, (XtArgVal)False);
	XtSetArg(args[5], XmNlabelString, (XtArgVal)xm_str);
	XtSetArg(args[6], XmNmnemonic, (XtArgVal)ks);
	cap2 = XtCreateManagedWidget(
			"cap2", xmLabelGadgetClass, form2,
			args, 7
	);
	XmStringFree(xm_str);

		/* Reuse 0-1 above (see cap2) */
	XtSetArg(args[2], XmNleftAttachment, (XtArgVal)XmATTACH_WIDGET);
	XtSetArg(args[3], XmNleftWidget, (XtArgVal)cap2);
	XtSetArg(args[4], XmNselectionArrayCount, (XtArgVal)1);
	XtSetArg(args[5], XmNmaxLength, (XtArgVal)80);
	XtSetArg(args[6], XmNtranslations, (XtArgVal)PASSWD_TRANS);
	TF2 = I_TF2.w = XtCreateManagedWidget(
			"passwd_tf", xmTextFieldWidgetClass, form2,
			args, 7
	);

	XtAddCallback(TF2, XmNlosingFocusCallback, tf2_cb,
						(XtPointer)login_info);
	XtAddCallback(TF2, XmNfocusCallback, tf2_cb,
						(XtPointer)login_info);
#define HORIZ_SP	30

#ifdef USE_FORM

	XtSetArg(args[0], XmNleftAttachment, (XtArgVal)XmATTACH_FORM);
	XtSetArg(args[1], XmNrightAttachment, (XtArgVal)XmATTACH_FORM);
	XtSetArg(args[2], XmNtopAttachment, (XtArgVal)XmATTACH_WIDGET);
	XtSetArg(args[3], XmNtopWidget, (XtArgVal)form2);
	XtSetArg(args[4], XmNhorizontalSpacing, (XtArgVal)HORIZ_SP);
	XtSetArg(args[5], XmNresizable, (XtArgVal)False);
	bar_form = XtCreateManagedWidget(
			"bar_form", xmFormWidgetClass, main_form,
			args, 6
	);
#else		/* USE_FORM */

	XtSetArg(args[0], XmNleftAttachment, (XtArgVal)XmATTACH_FORM);
	XtSetArg(args[1], XmNrightAttachment, (XtArgVal)XmATTACH_FORM);
	XtSetArg(args[2], XmNtopAttachment, (XtArgVal)XmATTACH_WIDGET);
	XtSetArg(args[3], XmNtopWidget, (XtArgVal)form2);
	XtSetArg(args[4], XmNspacing, (XtArgVal)HORIZ_SP);
	XtSetArg(args[5], XmNorientation, (XtArgVal)XmHORIZONTAL);
	XtSetArg(args[6], XmNpacking, (XtArgVal)XmPACK_COLUMN);
	XtSetArg(args[7], XmNentryAlignment, (XtArgVal)XmALIGNMENT_CENTER);
	XtSetArg(args[8], XmNmarginWidth, (XtArgVal)0);
	bar_form = XtCreateManagedWidget(
			"bar_form", xmRowColumnWidgetClass, main_form,
			args, 9
	);
#endif		/* USE_FORM */

#define CREATE_BTN(B,n,L,M,CB,LW,II)\
	XmSTR_AND_MNE(L, M, II, DM_B_MNE_ACTIVATE_BTN);\
	XtSetArg(args[4], XmNleftWidget, (XtArgVal)LW);\
	XtSetArg(args[5], XmNlabelString, (XtArgVal)xm_str);\
	XtSetArg(args[6], XmNmnemonic, (XtArgVal)ks);\
	B = II.w = XtCreateManagedWidget(\
		n, xmPushButtonGadgetClass, bar_form, args, 7);\
	XmStringFree(xm_str);\
	XtAddCallback(B, XmNactivateCallback, CB, (XtPointer)login_info)

		/* Define the common arguments first, 0-3 */
	XtSetArg(args[0], XmNtopAttachment, (XtArgVal)XmATTACH_FORM);
	XtSetArg(args[1], XmNbottomAttachment, (XtArgVal)XmATTACH_FORM);
	XtSetArg(args[2], XmNleftAttachment, (XtArgVal)XmATTACH_WIDGET);
	XtSetArg(args[3], XmNrecomputeSize, (XtArgVal)False);

	CREATE_BTN(BTN1, "b1", name1, mne1, btn1_cb, bar_form, I_BTN1);
	CREATE_BTN(BTN2, "b2", MSG_RESET_BTN, MSG_RESET_BTN_MNE,
							ResetCB, BTN1, I_BTN2);
	CREATE_BTN(BTN3, "b3", name3, mne3, ExitCB, BTN2, I_BTN3);
	CREATE_BTN(BTN4, "b4", MSG_HELP_BTN, MSG_HELP_BTN_MNE,
							HelpCB, BTN3, I_BTN4);

	XtSetArg(args[0], XmNdefaultButton, (XtArgVal)BTN1);
	XtSetValues(main_form, args, 1);

	if (!first_toplevel)
	{
		copyright = bar_form;
	}
	else
	{
		xm_str = GGT_LR(GT(MSG_COPYRIGHT));
		XtSetArg(args[0], XmNleftAttachment, (XtArgVal)XmATTACH_FORM);
		XtSetArg(args[1], XmNrightAttachment, (XtArgVal)XmATTACH_FORM);
		XtSetArg(args[2], XmNtopAttachment, (XtArgVal)XmATTACH_WIDGET);
		XtSetArg(args[3], XmNtopWidget, (XtArgVal)bar_form);
		XtSetArg(args[4], XmNtraversalOn, (XtArgVal)False);
		XtSetArg(args[5], XmNalignment, (XtArgVal)XmALIGNMENT_CENTER);
		XtSetArg(args[6], XmNlabelString, (XtArgVal)xm_str);
		copyright = XtCreateManagedWidget(
			"cpy", xmLabelGadgetClass, main_form,
			args, 7
		);
		XmStringFree(xm_str);
		SET_LOGIN_ID_M;
	}

	XtSetArg(args[0], XmNleftAttachment, (XtArgVal)XmATTACH_FORM);
	XtSetArg(args[1], XmNrightAttachment, (XtArgVal)XmATTACH_FORM);
	XtSetArg(args[2], XmNtopAttachment, (XtArgVal)XmATTACH_WIDGET);
	XtSetArg(args[3], XmNtopWidget, (XtArgVal)copyright);
	XtSetArg(args[4], XmNbottomAttachment, (XtArgVal)XmATTACH_FORM);
	XtSetArg(args[5], XmNbottomOffset, (XtArgVal)VERT_SP);
	XtSetArg(args[6], XmNalignment, (XtArgVal)XmALIGNMENT_BEGINNING);
	XtSetArg(args[7], XmNtraversalOn, (XtArgVal)False);
	XtSetArg(args[8], XmNlabelString, (XtArgVal)status_msg);
	*status_label = XtCreateManagedWidget(
			"", xmLabelGadgetClass, main_form,
			args, 9
	);

	XtSetMappedWhenManaged(parent, False);
	XtRealizeWidget(parent);

	DmRegisterMnemonic(parent, mne_info, NUM_WIDS);

	XtSetArg(args[0], XmNwidth, &shell_wd);
	XtSetArg(args[1], XmNheight, &shell_hi);
	XtGetValues(parent, args, 2);

#ifndef USE_FORM

	if ((int)shell_wd > WidthOfScreen(XtScreen(parent))) {

		num_btns = 2;
		XtSetArg(args[0], XmNnumColumns, num_btns);
		XtSetArg(args[1], XmNspacing, VERT_SP);
		XtSetValues(bar_form, args, 2);

		XtSetArg(args[0], XmNwidth, &shell_wd);
		XtSetArg(args[1], XmNheight, &shell_hi);
		XtGetValues(parent, args, 2);

	} else {

		num_btns = 4;
	}
#endif		/* USE_FORM */

	XtSetArg(args[0], XmNrecomputeSize, False);

	max_wd = max_hi = 0;
	GetMax(cap1, &max_wd, &max_hi);
	GetMax(cap2, &max_wd, &max_hi);
	cap_wd = max_wd;

	XtSetArg(args[1], XmNwidth, max_wd);
	XtSetArg(args[2], XmNheight, max_hi);

	XtSetValues(cap1, args, 3);
	XtSetValues(cap2, args, 3);

#ifdef USE_FORM
	max_wd = max_hi = 0;
	GetMax(BTN1, &max_wd, &max_hi);
	GetMax(BTN2, &max_wd, &max_hi);
	GetMax(BTN3, &max_wd, &max_hi);
	GetMax(BTN4, &max_wd, &max_hi);

	XtSetArg(args[1], XmNwidth, max_wd);
	XtSetArg(args[2], XmNheight, max_hi);

	XtSetValues(BTN1, args, 3);
	XtSetValues(BTN2, args, 3);
	XtSetValues(BTN3, args, 3);
	XtSetValues(BTN4, args, 3);

	sz = ((int)shell_wd - (int)max_wd * 4 - HORIZ_SP * 3) / 2 - MARGIN_WD;
	XtSetArg(args[1], XmNleftOffset, sz);
	XtSetValues(BTN1, args, 2);
#else		/* USE_FORM */

	max_wd = max_hi = 0;
	GetMax(BTN1, &max_wd, &max_hi);
	sz = ((int)shell_wd - (int)max_wd * num_btns -
		   HORIZ_SP * (num_btns - 1)) / 2 - MARGIN_WD;
	if (sz > 0) {

		XtSetArg(args[0], XmNmarginWidth, sz);
		XtSetValues(bar_form, args, 1);
	}

	XtSetArg(args[0], XmNrecomputeSize, False);
#endif		/* USE_FORM */

	sz = cap_wd;
	max_wd = max_hi = 0;
	GetMax(TF1, &max_wd, &max_hi);
	sz = ((int)shell_wd - (int)sz - (int)max_wd) / 2 - MARGIN_WD;

	XtSetArg(args[1], XmNleftOffset, sz);
	XtSetValues(cap1, args, 2);
	XtSetValues(cap2, args, 2);

	center_x = (SCR_WD - (int)shell_wd) / 2;
	center_y = (SCR_HI - (int)shell_hi) / 2;

	XtSetArg(args[0], XmNx, center_x);
	XtSetArg(args[1], XmNy, center_y);
	XtSetValues(parent, args, 2);
	XtSetMappedWhenManaged(parent, True);
	XtMapWidget(parent);
	XWarpPointer(dpy, None, ROOT_WIN, 0, 0, 0, 0, SCR_WD / 2, SCR_HI / 2);
	return(parent);

#undef SCR
#undef ROOT_WIN
#undef SCR_WD
#undef SCR_HI
#undef MARGIN_WD
#undef VERT_SP
#undef HORIZ_SP
#undef XmSTR_AND_MNE
#undef CREATE_BTN

#undef TF1
#undef TF2
#undef BTN1
#undef BTN2
#undef BTN3
#undef BTN4
#undef I_TF1
#undef I_TF2
#undef I_BTN1
#undef I_BTN2
#undef I_BTN3
#undef I_BTN4
}

/*
 *	Create the login window area 
 */
extern Widget
CreateLoginArea(Display * dpy, XtAppContext context)
{
#define BUF_LEN		   128

extern char *companyLogoPixmap;

static char enable_return_key[] = "\
<Key>osfActivate:next-tab-group()\n\
~m ~a ~s<Key>Return:next-tab-group()\
";

static char passwd_trans[] = "\
s ~m ~a<Key>Tab:prev-tab-group()\n\
~m ~a<Key>Tab:next-tab-group()\n\
c ~s ~m ~a<Key>osfDelete:new-bs(1)\n\
~s ~m ~a<Key>osfDelete:new-bs(2)\n\
~m ~a<Key>osfBackSpace:new-bs(2)\n\
<Key>osfHelp:Help()\n\
<Key>osfActivate:activate()\n\
~s ~m ~a<Key>Return:activate()\n\
<Key>:new-insert()\n\
~c ~s ~m ~a<Btn1Down>:grab-focus()\n\
<Enter>:enter()\n\
<Leave>:leave()\n\
<FocusIn>:focusIn()\n\
<FocusOut>:focusOut()\n\
<Unmap>:unmap()";

	struct utsname	name;
	char		local_buf[BUF_LEN];
	int		sz;
	Pixmap		logo_pix;
	String		welcome_msg, company_logo;
	Widget		shell, title, exit_btn;
	XmString	title_msg;
	XtActionsRec	passwd_actions[2];

	if (!companyLogoPixmap)
	{
		logo_pix = None;
	}
	else
	{
#define SCR		DefaultScreenOfDisplay(dpy)
#define DEPTH		DefaultDepthOfScreen(SCR)
#define ROOT_WIN	RootWindowOfScreen(SCR)
#define COLORMAP	DefaultColormapOfScreen(SCR)

		unsigned int ww, hh;

		if (XReadPixmapFile(dpy, ROOT_WIN, COLORMAP,
				companyLogoPixmap, &ww, &hh, DEPTH, &logo_pix))
		{
			logo_pix = None;
		}

#undef SCR
#undef DEPTH
#undef ROOT_WIN
#undef COLORMAP
	}

	uname(&name); /* already I18N the nodename? */

#define PERCENT_S	2	/* %s in string_greet1, see dtlogin.h */

	company_logo = GT(MSG_GREET);
	if ((sz = strlen(company_logo) + strlen(name.nodename) - PERCENT_S + 1)
								< BUF_LEN)
		welcome_msg = local_buf;
	else
		welcome_msg = (String)XtMalloc(sz);

	sprintf(welcome_msg, company_logo, name.nodename);

	MakeUserList();

	title_msg = XmStringCreateLtoR(welcome_msg, XmFONTLIST_DEFAULT_TAG);
	TF1_TRANS = XtParseTranslationTable(enable_return_key);
	passwd_actions[0].string = "new-bs";
	passwd_actions[0].proc = PasswdBSAction;
	passwd_actions[1].string = "new-insert";
	passwd_actions[1].proc = PasswdInsertAction;
	XtAppAddActions(context, passwd_actions, 2);
	PASSWD_TRANS = XtParseTranslationTable(passwd_trans);

	shell = CreateShell(dpy, HelpCB, logo_pix, &title, title_msg,
			login_info->wids,
			MSG_LOGIN_ID, MSG_LOGIN_ID_MNE, LoginVerifyCB,
			MSG_PASSWD,   MSG_PASSWD_MNE,   PasswdFocusCB,
			MSG_LOGIN_BTN,MSG_LOGIN_BTN_MNE,LoginCB,
			MSG_EXIT_BTN, MSG_EXIT_BTN_MNE,
			&STATUS_LABEL, LOGIN_ID_M,
			True,	/* first_toplevel */
			False	/* tf1_want_passwd_trans */
	);

	XmStringFree(title_msg);

	if (welcome_msg != local_buf)
	{
		XtFree(welcome_msg);
	}

	return(shell);

#undef BUF_LEN
#undef PERCENT_S
}

static void
NewPasswdVerifyCB(Widget w, XtPointer client_data, XtPointer call_data)
{
		/* name client_data as login_info for marco convenience */
	LoginInfo		login_info = (LoginInfo)client_data;
	XmTextVerifyPtr		cd = (XmTextVerifyPtr)call_data;

	if (cd ->reason == XmCR_LOSING_FOCUS)
	{
		if (CHK_STATUS(RESET_CB_BIT))
			return;
	}
}

static void
NewPasswdOkCB(Widget w, XtPointer client_data, XtPointer call_data)
{
		/* name client_data as login_info for marco convenience */
	LoginInfo		login_info = (LoginInfo)client_data;

	int			reason, value = 0;
	Boolean			unknown_error = False;
	String			err1;
	XmString		msg_str;


	NUM_TRIES++;

#ifdef TESTGUI
 printf("%s (%d), ?NewPasswdOkCB, NEW: %s VERIFY: %s\n", __FILE__, __LINE__,
	NEW_PASSWORD ? NEW_PASSWORD : "NULL",
	VERIFY_PASSWORD ? VERIFY_PASSWORD : "NULL");
#endif

	if (strcmp(NEW_PASSWORD, VERIFY_PASSWORD)) /* passwd not match */
	{
		reason = -1;
		err1 = MSG_PASSWD_NOTMATCH;
	}
	else
	{
		switch(reason = StorePasswd(LOGIN_ID, NEW_PASSWORD))
		{
			case LOGIN_SUCCESS:
				err1 = NULL;
				SET_STATUS(NEW_PASSWD_DONE_BIT);
				SET_STATUS(NEW_PASSWD_STORED_BIT);
				break;
			case TOO_SHORT:
				value = GetPasswdMinLen();
				err1 = MSG_TOO_SHORT;
				break;
			case CIRC:
				err1 = MSG_CANT_CIRC;
				break;
			case SPECIAL_CHARS:
				err1 = MSG_SP_CHARS;
				break;
			case DF3CHARS:
				err1 = MSG_DIFF3_POS;
				break;
			default:
				value = reason;
				err1 = MSG_UNKNOWN_ERR;
				unknown_error = True;
				break;
		}
	}

	if (!err1)	/* error free... */
	{
		msg_str = NULL;
	}
	else		/* check if too many attemps */
	{
#define BUF_LEN			128

		char		local_buf[BUF_LEN];
		int		sz;
		String		str, str1, str2, fmt;

		str1 = GT(err1);
		sz = strlen(str1) + 1;
		if (NUM_TRIES < PW_MAXTRYS)
		{
			fmt = "%s";
			str2 = NULL;
		}
		else
		{ 
			SET_STATUS(NEW_PASSWD_DONE_BIT);
			fmt = "%s\n\n%s";
			str2 = GT(MSG_TOO_MANY_TRIES);
			sz += strlen(str2);
		}

		if (sz < BUF_LEN)
			str = local_buf;
		else
			str = (String)XtMalloc(sz);

		if (reason == TOO_SHORT || unknown_error)
		{
			sprintf(str, str1, value);
			str1 = str;
		}
		sprintf(str, fmt, str1, str2);

		msg_str = GGT_LR(str);

		if (str != local_buf)
		{
			XtFree(str);
		}
	}

	if (CHK_STATUS(NEW_PASSWD_DONE_BIT))
	{
			/* I don't have to return input focus to TOPLEVEL,
			 * because if STORED_BIT is set then I'll be able
			 * to login, otherwise, it will be handled by
			 * PopupDialog() */
#if 0
		XLowerWindow(DPY, XtWindow(POPUP));
#else
		XtUnmapWidget(POPUP);
#endif
		RESET_STATUS(NEW_PASSWD_REASON_BITS);
	}

	if (msg_str)
	{
			/* Always do this when DONE_BIT is not set,
			 * otherwise, it will be handled when NewPasswd()
			 * is invoked next time */
		if (!CHK_STATUS(NEW_PASSWD_DONE_BIT))
			ClearTextFields(login_info);

		PopupDialog(NEW_PASSWD_ERR, msg_str, RING_BELL | FREE_XMSTR);
	}

#undef BUF_LEN
}

extern void
NewPasswd(char * user, int reason)
{
#define WINDOW		XtWindow(POPUP)

	static int	last_reason = NOT_DEFINED;
	Boolean		do_focus_switch = False;
	String		title;
	XmString	title_msg;

	XBell(DPY, 0);
	NUM_TRIES		= 0;
	NEW_PASSWORD[0]		=
	VERIFY_PASSWORD[0]	= 0;

	switch(reason)
	{
		case MANDATORY:
			SET_MANDATORY();
			title = MSG_MAND_TITLE;
			break;
		case AGED:
			SET_AGED();
			title = MSG_AGED_TITLE;
			break;
		case PFLAG:
			SET_PFLAG();
			title = MSG_PFLA_TITLE;
			break;
		default:
			printf("%s (%d) - unknown reason: %d\n",
					__FILE__, __LINE__, reason);
			return;
	}

	title_msg = GGT_LR(GT(title));

	if (last_reason != NOT_DEFINED) /* not the first time */
	{
		if (CHK_STATUS(VERIFY_PASSWD_BIT))
		{
			RESET_STATUS(VERIFY_PASSWD_BIT);
			do_focus_switch = True;
		}
		if (reason != last_reason)
		{
			Arg	arg[1];

			XtSetArg(arg[0], XmNlabelString, (XtArgVal)title_msg);
			XtSetValues(POPUP_TITLE, arg, 1);
		}
#if 0
		XRaiseWindow(DPY, WINDOW);
#else
		XtMapWidget(POPUP);
		XFlush(DPY);
#endif
	}
	else
	{
#define MARGIN_WD	10
#define VERT_SP		10

		Widget		status_label, dismiss_btn;

		POPUP = CreateShell(DPY, HelpCB, (Pixmap)None,
				&POPUP_TITLE, title_msg,
				login_info->pwids,
				MSG_NEW_PASSWD,  NULL, NewPasswdVerifyCB,
				MSG_ENTER_AGAIN, NULL, PasswdFocusCB,
				MSG_OK_BTN, MSG_OK_BTN_MNE, NewPasswdOkCB,
				MSG_DISMISS_BTN, MSG_DISMISS_BTN_MNE,
				&status_label, PASSWD_M,
				False,	/* first_toplevel */
				True	/* tf1_want_passwd_trans */
		);
		XtMapWidget(POPUP);
		XDefineCursor(DPY, WINDOW, LEFT_PTR);
	}

	XmStringFree(title_msg);

	XSetInputFocus(DPY, WINDOW, RevertToNone, CurrentTime);

	if (do_focus_switch)
		XmProcessTraversal(NEW_PASSWD_TF, XmTRAVERSE_CURRENT);

	last_reason = reason;

#undef WINDOW
}
