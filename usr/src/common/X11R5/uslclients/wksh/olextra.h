
/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:olextra.h	1.3"

#ifdef MOOLIT
#define WK_MOOLIT_EXTRA_FUNCS \
	int do_OlPostPopupMenu();
#define WK_MOOLIT_EXTRA_TABLE \
	{ "OlPostPopupMenu", VALPTR(do_OlPostPopupMenu), N_BLTIN|BLT_FSUB },
#else
#define WK_MOOLIT_EXTRA_FUNCS
#define WK_MOOLIT_EXTRA_TABLE
#endif

#define WK_TK_EXTRA_FUNCS \
	WK_MOOLIT_EXTRA_FUNCS \
	int do_ChangeBarOp(); \
	int do_OlWMProtocolAction(); \
	int do_EventOp(); \
	int do_MnemonicOp(); \
	int do_DataOp(); \
	int do_TextFieldOp(); \
	int do_FocusOp(); \
	int do_CursorOp(); \
	int do_OlRegisterHelp(); \
	int do_TextEditOp(); \
	int do_ScrollingListOp(); \
	int do_OlFlatSetValues(); \
	int do_OlFlatGetValues();


#define WK_TK_EXTRA_MSG 


#define WK_TK_EXTRA_TABLE \
	WK_MOOLIT_EXTRA_TABLE \
	{ "ChangeBarOp", VALPTR(do_ChangeBarOp), N_BLTIN|BLT_FSUB }, \
	{ "OlWMProtocolAction", VALPTR(do_OlWMProtocolAction), N_BLTIN|BLT_FSUB }, \
	{ "EventOp", VALPTR(do_EventOp), N_BLTIN|BLT_FSUB }, \
	{ "MnemonicOp", VALPTR(do_MnemonicOp), N_BLTIN|BLT_FSUB }, \
	{ "DataOp", VALPTR(do_DataOp), N_BLTIN|BLT_FSUB }, \
	{ "TextFieldOp", VALPTR(do_TextFieldOp), N_BLTIN|BLT_FSUB }, \
	{ "FocusOp", VALPTR(do_FocusOp), N_BLTIN|BLT_FSUB }, \
	{ "CursorOp", VALPTR(do_CursorOp), N_BLTIN|BLT_FSUB }, \
	{ "ScrollingListOp", VALPTR(do_ScrollingListOp), N_BLTIN|BLT_FSUB }, \
	{ "TextEditOp", VALPTR(do_TextEditOp), N_BLTIN|BLT_FSUB }, \
	{ "OlRegisterHelp", VALPTR(do_OlRegisterHelp), N_BLTIN|BLT_FSUB }, \
	{ "OlFlatSetValues", VALPTR(do_OlFlatSetValues), N_BLTIN|BLT_FSUB }, \
	{ "OlFlatGetValues", VALPTR(do_OlFlatGetValues), N_BLTIN|BLT_FSUB },

#define WK_TK_EXTRA_VAR

#define WK_TK_EXTRA_ALIAS \
	"dataprint",	"DataOp -p",		N_FREE, \
	"datareset",	"DataOp -r",		N_FREE, \
	"defaultaction", "OlWMProtocolAction",	N_FREE, \
	"cbclear",	"ChangeBarOp -c",	N_FREE, \
	"cbsetup",	"ChangeBarOp -s",	N_FREE, \
	"cbtest",	"ChangeBarOp -t",	N_FREE, \
	"mnsetup",	"MnemonicOp -s",	N_FREE, \
	"mnclear",	"MnemonicOp -c",	N_FREE, \
	"focacc",	"FocusOp -a",		N_FREE, \
	"foccanacc",	"FocusOp -c",		N_FREE, \
	"focset",	"FocusOp -s",		N_FREE, \
	"focget",	"FocusOp -g",		N_FREE, \
	"fochas",	"FocusOp -h",		N_FREE, \
	"focmv",	"FocusOp -m",		N_FREE, \
	"curbusy",	"CursorOp -b",		N_FREE, \
	"curdup",	"CursorOp -d",		N_FREE, \
	"curmove",	"CursorOp -m",		N_FREE, \
	"curpan",	"CursorOp -p",		N_FREE, \
	"curquest",	"CursorOp -q",		N_FREE, \
	"curstand",	"CursorOp -s",		N_FREE, \
	"curtarget",	"CursorOp -t",		N_FREE, \
	"curname",	"CursorOp -n",		N_FREE, \
	"curfont",	"CursorOp -f",		N_FREE, \
	"curundef",	"CursorOp -u",		N_FREE, \
	"ofsv",		"OlFlatSetValues",	N_FREE, \
	"ofgv",		"OlFlatGetValues",	N_FREE, \
	"OlInitialize",	"XtAppInitialize",	N_FREE, \
	"oi",		"XtAppInitialize",	N_FREE, \
	"orh",		"OlRegisterHelp",	N_FREE, \
	"oppm",		"OlPostPopupMenu",	N_FREE, \
	"teclear",	"TextEditOp -c",	N_FREE, \
	"teecho",	"TextEditOp -e",	N_FREE, \
	"tegetsel",	"TextEditOp -s",	N_FREE, \
	"tecutsel",	"TextEditOp -C",	N_FREE, \
	"tesubstr",	"TextEditOp -S",	N_FREE, \
	"teredraw",	"TextEditOp -r",	N_FREE, \
	"sladd",	"ScrollingListOp -a",	N_FREE,	\
	"sldel",	"ScrollingListOp -d",	N_FREE, \
	"slview",	"ScrollingListOp -v",	N_FREE,	\
	"sledit",	"ScrollingListOp -e",	N_FREE,	\
	"slget",	"ScrollingListOp -g",	N_FREE,	\
	"slgetud",	"ScrollingListOp -G",	N_FREE,	\
	"slgetsel",	"ScrollingListOp -L",	N_FREE,	\
	"slsel",	"ScrollingListOp -S",	N_FREE,	\
	"slunsel",	"ScrollingListOp -U",	N_FREE,	\
	"slput",	"ScrollingListOp -p",	N_FREE,	\
	"slputud",	"ScrollingListOp -P",	N_FREE,	\
	"slinsert",	"ScrollingListOp -i",	N_FREE,	\
	"slclose",	"ScrollingListOp -c",	N_FREE,	\
	"sltouch",	"ScrollingListOp -t",	N_FREE,	\
	"slnoupdate",	"ScrollingListOp -n",	N_FREE,	\
	"slupdate",	"ScrollingListOp -u",	N_FREE,
