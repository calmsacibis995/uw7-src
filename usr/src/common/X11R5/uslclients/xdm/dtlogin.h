#ident	"@(#)xdm:dtlogin.h	1.38.2.1"

#ifndef	_DTLOGIN_H_
#define	_DTLOGIN_H_

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

#if defined(__STDC__)
#define CONST	const
#else
#define CONST
#endif

#define GGT(S)		XmStringCreate(GT(S), XmFONTLIST_DEFAULT_TAG)
#define GGT_LR(S)	XmStringCreateLtoR(S, XmFONTLIST_DEFAULT_TAG)
#define GT(S)		CallGetText((CONST char *)S)

#define	MAX_LOGNAME	1024
#define MAXFLD_LEN	128

/*
 * This file contains all the message strings for the Desktop UNIX
 * Graphical Login client dtlogin, used by xdm.
 * 
 */

#define MSG_FILENAME		"xdm:"
#define FS			"\000"

#define	MSG_LOGIN_ID		"1" FS "Login ID:"
#define MSG_PASSWD		"2" FS "Password:"

#define	MSG_LOGIN_BTN		"3" FS "Login"
#define	MSG_RESET_BTN		"4" FS "Reset"
#define	MSG_EXIT_BTN		"5" FS "Exit"
#define	MSG_HELP_BTN		"6" FS "Help"

#define	MSG_OK_BTN		"7" FS "OK"
#define MSG_DISMISS_BTN		"8" FS "Dismiss"

#define MSG_GREET		"9" FS "Welcome to %s.\
 Please enter your Login ID and Password."

#define	MSG_ACCOUNT_AGED	"10" FS "Your Account has expired.\
 Please contact your systems administrator."

#define	MSG_LOGIN_FAILED	"11" FS "Login attempt failed. Try again."

#define	MSG_PASSWD_AGED		"12" FS "Your Password has expired.\
 Please contact your systems administrator."

#define	MSG_PASSWORD		"13" FS "Password entries will not\
 be displayed as you type them."

#define	MSG_COPYRIGHT		"41" FS "Copyright 1996 The Santa Cruz Operation, Inc.  All Rights Reserved.\nCopyright 1984-1995 Novell, Inc. All Rights Reserved."

#define	MSG_BADSHELL		"15" FS "Your SHELL is not valid"

#define	MSG_NOHOME		"16" FS "Unable to change to home directory"

#define MSG_MAND_TITLE		"17" FS "Password Needed\n\n\
Please enter a Password for your Account."

#define MSG_AGED_TITLE		"18" FS "Password Expired\n\n\
Your Password has expired.  Please choose a new one."

#define MSG_PFLA_TITLE		"19" FS "Requested Password Change\n\n\
You have requested a Password change.\nPlease Enter a New Password."

#define	MSG_NEW_PASSWD		"20" FS "Enter New Password:"
#define	MSG_ENTER_AGAIN		"21" FS "Enter It Again:"

#define MSG_PASSWD_NOTMATCH	"22" FS "The Passwords supplied do\
 not match. Try again."

#define MSG_TOO_MANY_TRIES	"23" FS "Too many attempts.\
 Could not change Password."

#define MSG_TOO_SHORT		"24" FS "Password is too short -\
 must be at least %d characters."

#define	MSG_CANT_CIRC		"25" FS "Password cannot be circular\
 shift of login id."

#define	MSG_SP_CHARS		"26" FS "Password must contain at least\
 two alphabetic characters\nand at least one numeric or special character."

#define	MSG_DIFF3_POS		"27" FS "Passwords must differ by at\
 least 3 positions."

#define MSG_UNKNOWN_ERR		"28" FS "Unknown problem with reason %d,\
 Please contact your systems administrator."

#define	MSG_HELP_MAIN		"29" FS "Enter your Login id in the Login\
 id field.\nEnter your Password in the Password field.\nThe Password will\
 not be echoed.\n\nThe Login button will log you into the system if you\
 have typed\nyour Login id and Password.\n\nThe Reset button will remove\
 any text that you have typed from\nthe Login id and Password fields.\n\nYou\
 can switch to another virtual terminal to perform a task and\nreturn to\
 graphical login by pressing and holding down the\nfollowing keys: Alt,\
 SysRq and P.\n\nThe Exit button will exit graphical login and will return\
 you to the\nConsole Login prompt.\n\nTo return to graphical login after\
 switching to another virtual\nterminal press and hold down: Alt, SysRq and N."

#define	MSG_HELP_POPUP		"30" FS "Your current Password has\
 expired or you specifically\nrequested to change it.\n\nPlease enter a\
 new Password in the first Field.\nPlease retype the same Password in\
 the second Field.\n\nThe text you type will not be displayed back to you."

#define	MSG_LOGIN_ID_MNE	"31" FS "I"
#define	MSG_PASSWD_MNE		"32" FS "P"
#define	MSG_LOGIN_BTN_MNE	"33" FS "L"
#define	MSG_RESET_BTN_MNE	"34" FS "R"
#define	MSG_EXIT_BTN_MNE	"35" FS "E"
#define	MSG_HELP_BTN_MNE	"36" FS "H"
#define	MSG_OK_BTN_MNE		"37" FS "O"
#define MSG_DISMISS_BTN_MNE	"38" FS "D"

/*
 *	Structure to hold widgets that must be passed and used
 *	in multiple routines/files.
 */
#define APP_CONTEXT	XtWidgetToApplicationContext(TOPLEVEL)
#define DPY		XtDisplay(TOPLEVEL)

					/* Assumed args[] in the code */
#define DPY_STATUS(M)		XtSetArg(args[0], XmNlabelString, (XtArgVal)M);\
				XtSetValues(STATUS_LABEL, args, 1)

	/* The following definitions are used for dealing with STATUS */
#define CHK_STATUS(B)		((STATUS & (B)) ? True : False)
#define SET_STATUS(B)		STATUS |= (B)
#define RESET_STATUS(B)		STATUS &= ~(B)

#define BTN_STATE(M)		(CHK_STATUS(M) ? True : False)

#define SET_LOGIN_ID_M		RESET_STATUS(PASSWD_BIT)
#define SET_PASSWD_M		SET_STATUS(PASSWD_BIT)
#define DPY_LOGIN_ID_M		SET_LOGIN_ID_M; DPY_STATUS(LOGIN_ID_M)
#define DPY_PASSWD_M		SET_PASSWD_M; DPY_STATUS(PASSWD_M)

#define DOING_GREET()		!CHK_STATUS(NEW_PASSWD_REASON_BITS)
#define DOING_MANDATORY()	CHK_STATUS(MANDATORY_BIT)
#define DOING_AGED()		CHK_STATUS(AGED_BIT)
#define DOING_PFLAG()		CHK_STATUS(PFLAG_BITS)
#define SET_MANDATORY()		RESET_STATUS(NEW_PASSWD_REASON_BITS);\
				SET_STATUS(MANDATORY_BIT)
#define SET_AGED()		RESET_STATUS(NEW_PASSWD_REASON_BITS);\
				SET_STATUS(AGED_BIT)
#define SET_PFLAG()		SET_STATUS(PFLAG_BITS)

#define GREET_DONE_BIT		(1L << 0)
#define NEW_PASSWD_DONE_BIT	(1L << 1)
#define PASSWD_BIT		(1L << 2)	/* 0: LOGIN_ID_M,
						 * 1: PASSWD_M */
#define VERIFY_PASSWD_BIT	(1L << 3)	/* 0: Enter New Passwd,
						 * 1: Enter It Again */
#define RESET_CB_BIT		(1L << 4)	/* Activate Reset Button  */
#define LOGIN_BTN_BIT		(1L << 5)
#define OK_BTN_BIT		(1L << 6)
#define NEW_PASSWD_STORED_BIT	(1L << 7)
#define MANDATORY_BIT		(1L << 8)
#define AGED_BIT		(1L << 9)
#define PFLAG_BITS		NEW_PASSWD_REASON_BITS
#define NEW_PASSWD_REASON_BITS	(MANDATORY_BIT | AGED_BIT)

	/* The following definitions are reasons for invoking PopupDialog().
	 * Note that, BADSHELL == 7 and NOHOME == 8, see dm.h,
	 * DON'T CHANGE THE #, see greet.c:PopupDialog() */
#define NOT_DEFINED		0
#define ACCOUNT_AGED		1	/* EXPIRED */
#define HELP_MAIN		2	/* Help button */
#define LOGIN_FAILED		3
#define PASSWORD_AGED		4	/* INACTIVE or IDLEWEEKS */
#define HELP_POPUP		5
#define NEW_PASSWD_ERR		6
 /*
  * BADSHELL			7
  * NOHOME			8
  */
#define NUM_REASONS		9

	/* The following definitions are `flag's for invoking PopupDialog(). */
#define DO_NOTHING		0
#define FREE_XMSTR		(1L << 0)
#define RING_BELL		(1L << 1)

	/* Convenience marcos for accessing various fields in `login_info'.
	 * Assumed `login_info' is the name! */
#define STATUS			login_info->flag

	/* TOPLEVEL related children */
#define TOPLEVEL		login_info->shell
#define LOGIN_TF		login_info->wids[0] /* login_tf */
#define PASSWD_TF		login_info->wids[1] /* passwd_tf */
#define LOGIN_BTN		login_info->wids[2] /* login_btn */
#define RESET_BTN		login_info->wids[3] /* reset_btn */
#define EXIT_BTN		login_info->wids[4] /* exit_btn */
#define HELP_BTN		login_info->wids[5] /* help_btn */
#define STATUS_LABEL		login_info->status_label

#define PING_TIMEOUT		login_info->ping_timeout
#define LEFT_PTR		login_info->left_ptr

	/* Compiled translations */
#define TF1_TRANS		login_info->tf1_trans
#define PASSWD_TRANS		login_info->passwd_trans

	/* Login id and Passwd strings */
#define LOGIN_ID		login_info->login_id
#define PASSWORD		login_info->password

	/* Assume that TOPLEVEL related messages will be used more often
	 * than POPUP's, so we cache all TOPLEVEL related messages, but
	 * POPUP's. */
#define LOGIN_ID_M		NULL
#define LOGIN_FAILED_M		login_info->login_failed_m
#define PASSWD_M		login_info->passwd_m
#define ACCOUNT_AGED_M		login_info->account_aged_m
#define HELP_MAIN_M		login_info->help_main_m
#define PASSWORD_AGED_M		login_info->passwd_aged_m

	/* POPUP related children */
#define POPUP			login_info->popup
#define POPUP_TITLE		login_info->popup_title
#define NEW_PASSWD_TF		login_info->pwids[0] /* new_passwd_tf*/
#define VERIFY_PASSWD_TF	login_info->pwids[1] /* verify_passwd_tf*/
#define OK_BTN			login_info->pwids[2] /* ok_btn */
#define P_RESET_BTN		login_info->pwids[3] /* p_reset_btn */
#define DISMISS_BTN		login_info->pwids[4] /* dismiss_btn */
#define P_HELP_BTN		login_info->pwids[5] /* p_help_btn */

	/* New passwd and Enter it again strings */
#define NEW_PASSWORD		login_info->new_password
#define VERIFY_PASSWORD		login_info->verify_password

	/* Number of tries so far... */
#define NUM_TRIES		login_info->num_tries

#define NUM_WIDS		6
typedef struct {
	Widget		shell;
	Widget		wids[NUM_WIDS];
	Widget		status_label;
	Widget		popup;
	Widget		popup_title;
	Widget		pwids[NUM_WIDS];
	Cursor		left_ptr;
	XtIntervalId	ping_timeout;
	XmString	passwd_m;
	XmString	account_aged_m;
	XmString	help_main_m;
	XmString	login_failed_m;
	XmString	passwd_aged_m;
	XtTranslations	tf1_trans;
	XtTranslations	passwd_trans;
	unsigned long	flag;
	String		login_id;
	char		password[MAXFLD_LEN];
	char		new_password[MAXFLD_LEN];
	char		verify_password[MAXFLD_LEN];
	unsigned short	num_tries;
} *LoginInfo, LoginInfoRec;

extern char *		CallGetText(CONST char *);
extern void		ClearTextFields(LoginInfo);
extern Widget		CreateLoginArea(Display *, XtAppContext);
extern void		GetMax(Widget, Dimension *, Dimension *);
extern void		NewPasswd(char *, int);
extern void		PopupDialog(int, XmString, unsigned long);
extern void		PopupHelp(Widget);
extern void		PopupPasshelp(Widget);
extern void		SetBackgroundSolid(Display *);
extern void		SetMsgFileName(CONST char *);

extern char *		backgroundPixmap;
extern LoginInfo	login_info;

#ifdef NOT_IN_USE
#define	string_malloc	"dtlogin:1" FS "Operation failed from lack of memory"
#define	string_login_fail	"dtlogin:4" FS "Login attempt failed."
#define	string_nopass		"dtlogin:9" FS "You must supply a password.  Try again."
#define mnemonic_apply		"dtlogin:24" FS "A"
#define	error_passwd		"dtlogin:26" FS "ERROR: Could not store PASSWORD.  Login through Console"
#define	string_phelp1		"dtlogin:32" FS "You do not have a Password on this System and you need to have one.\nPlease enter your Password in the first Field.\nPlease retype the same Password in the second Field.\n\nThe text you type will not be displayed back to you."
#define	label_ok1		"dtlogin:34" FS "OK"
#define	mnemonic_ok1		"dtlogin:35" FS "OK"
#endif /* NOT_IN_USE */

#endif	/* Don't add anything after this endif */
