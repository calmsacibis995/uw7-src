#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:userad/LoginMgr.h	1.1.1.1"
#endif

#include "findlocales.h"

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Xlib.h>
#include <X11/Vendor.h>

#include <Xm/Xm.h>
#include <Xm/TextF.h>

#include <Dt/Desktop.h>
#include <libMDtI/DesktopP.h>
#include <libMDtI/DtI.h>

#include <MGizmo/Gizmo.h>
#include <MGizmo/MsgGizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/BaseWGizmo.h>
#include <MGizmo/PopupGizmo.h>
#include <MGizmo/ModalGizmo.h>
#include <MGizmo/SpaceGizmo.h>
#include <MGizmo/ContGizmo.h>
#include <MGizmo/InputGizmo.h>
#include <MGizmo/LabelGizmo.h>
#include <MGizmo/ChoiceGizm.h>
#include <MGizmo/TextGizmo.h>
#include <MGizmo/IconBGizmo.h>
#include <nwusers.h>

#include "login_msgs.h"

#define MENU_ITEM(a, name, b, c, d) \
	{a, label_##name, mnemonic_##name, I_PUSH_BUTTON, b, c, d}

#define	GGT	GetGizmoText
#define	SET_BTN(id,n,name)	id[n].label = GGT(label##_##name);\
				id[n].mnem = (XtArgVal)*GGT(mnemonic##_##name);\
				id[n].sensitive = (XtArgVal)TRUE;\
				id[n].selCB = (XtArgVal)name##CB

#define GGG		GetGizmoGizmo
#define GGW		GetGizmoWidget
#define	HELP_PATH	"dtadmin/user.hlp"
#define GROUPHEIGHT	(XtArgVal)3

#define	P_ADD	1
#define	P_CHG	2
#define	P_DEL	3
#define	P_OWN	4
#define	P_DEL2	5

#define USERS	0
#define GROUPS	1
#define RESERVED	2

#define BASICLOCALE  "*basicLocale:"
#define DISPLAYLANG  "*displayLang:"
#define INPUTLANG    "*inputLang:"
#define NUMERIC      "*numeric:"
#define TIMEFORMAT   "*timeFormat:"
#define XNLLANGUAGE  "*xnlLanguage:"
#define FONTGROUP    "*fontGroup:"
#define FONTGROUPDEF "*fontGroupDef:"
#define INPUTMETHOD  "*inputMethod:"
#define IMSTATUS     "*imStatus:"

#define LANG        "LANG="
#define LANG2       "setenv LANG"

#define MOVE_XDEFAULTS  "/sbin/mv /tmp/.Xdefaults"
#define TMP_XDEFAULTS   "/tmp/.Xdefaults"
#define DONT_EDIT 	"#!@ Do not edit this line !@"

#define MOVE_PROFILE  "/sbin/mv /tmp/.profile"
#define MOVE_LOGIN    "/sbin/mv /tmp/.login"
#define TMP_PROFILE   "/tmp/.profile"
#define TMP_LOGIN     "/tmp/.login"

#define	FooterMsg(txt)	SetBaseWindowLeftMsg(g_base,txt)

 /* Locale related variables and structures */

struct	_LocaleItems {
	XtArgVal   label;
	XtArgVal   user_data;
	XtArgVal   set;
       };

typedef struct _LocaleItems FListItem2, *FListPtr2;
typedef void (*PFV)();

typedef struct Options {
	Dimension           gridWidth;
	Dimension           gridHeight;
	u_char              folderCols;
	u_char              folderRows;
} Options;
