
/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:xmextra.h	1.7.1.1"


#define WK_TK_EXTRA_FUNCS \
	int do_DmCreateIconContainer(); \
	int do_ListOp(); \
	int do_TextOp(); \
	int do_MnemonicOp(); \
	int do_DataOp(); \
	int do_RegisterHelp(); \
	int do_XmAddProtocolCallback(); \
	int do_XmMainWindowSetAreas(); \
	int do_XmCreateArrowButton(); \
	int do_XmCreateArrowButtonGadget(); \
	int do_XmCreateBulletinBoard(); \
	int do_XmCreateBulletinBoardDialog(); \
	int do_XmCreateCascadeButton(); \
	int do_XmCreateCascadeButtonGadget(); \
	int do_XmCreateCommand(); \
	int do_XmCreateDialogShell(); \
	int do_XmCreateDrawingArea(); \
	int do_XmCreateDrawnButton(); \
	int do_XmCreateErrorDialog(); \
	int do_XmCreateFileSelectionBox(); \
	int do_XmCreateFileSelectionDialog(); \
	int do_XmCreateForm(); \
	int do_XmCreateFormDialog(); \
	int do_XmCreateFrame(); \
	int do_XmCreateInformationDialog(); \
	int do_XmCreateLabel(); \
	int do_XmCreateLabelGadget(); \
	int do_XmCreateList(); \
	int do_XmCreateMainWindow(); \
	int do_XmCreateMenuBar(); \
	int do_XmCreateMenuShell(); \
	int do_XmCreateMessageBox(); \
	int do_XmCreateMessageDialog(); \
	int do_XmCreateOptionMenu(); \
	int do_XmCreatePanedWindow(); \
	int do_XmCreatePopupMenu(); \
	int do_XmCreatePromptDialog(); \
	int do_XmProcessTraversal(); \
	int do_XmCreatePulldownMenu(); \
	int do_XmCreatePushButton(); \
	int do_XmCreatePushButtonGadget(); \
	int do_XmCreateQuestionDialog(); \
	int do_XmCreateRadioBox(); \
	int do_XmCreateRowColumn(); \
	int do_XmCreateScale(); \
	int do_XmCreateScrollBar(); \
	int do_XmCreateScrolledList(); \
	int do_XmCreateScrolledText(); \
	int do_XmCreateScrolledWindow(); \
	int do_XmCreateSelectionBox(); \
	int do_XmCreateSelectionDialog(); \
	int do_XmCreateSeparator(); \
	int do_XmCreateSeparatorGadget(); \
	int do_XmCreateText(); \
	int do_XmCreateTextField(); \
	int do_XmCreateField(); \
	int do_XmCreateToggleButton(); \
	int do_XmCreateToggleButtonGadget(); \
	int do_XmCreateWarningDialog(); \
	int do_XmCreateWorkingDialog();

#define WK_TK_EXTRA_ALIAS \
	"listadd",	"ListOp -a",			N_FREE, \
	"listdel",	"ListOp -d",			N_FREE, \
	"listsel",	"ListOp -S",			N_FREE, \
	"listdesel",	"ListOp -D",			N_FREE, \
	"listbot",	"ListOp -b",			N_FREE, \
	"listtop",	"ListOp -t",			N_FREE, \
	"listget",	"ListOp -g",			N_FREE, \
	"listput",	"ListOp -p",			N_FREE, \
	"listgetsel",	"ListOp -s",			N_FREE, \
	"listgetselpos",	"ListOp -i",		N_FREE, \
	"XmListAddItems",	"ListOp -a",		N_FREE, \
	"XmListDeleteAllItems",	"ListOp -dall",		N_FREE, \
	"XmListDeleteItemsPos",	"ListOp -d",		N_FREE, \
	"XmListDeleteItems",	"ListOp -dtext",	N_FREE, \
	"XmListSelectPos",	"ListOp -S",		N_FREE, \
	"XmListDeselectPos",	"ListOp -D",		N_FREE, \
	"XmListSetBottomPos",	"ListOp -b",		N_FREE, \
	"XmListSetPos",	"ListOp -t",			N_FREE, \
	"XmListGetItemsPos",	"ListOp -g",		N_FREE, \
	"XmListReplaceItemsPos","ListOp -p",		N_FREE, \
	"XmListGetSelectedItems",	"ListOp -s",	N_FREE, \
	"XmListGetSelectedPos",	"ListOp -i",		N_FREE, \
	"XmTextGetString","TextOp -g",			N_FREE, \
	"XmTextSetString",	"TextOp -s",		N_FREE, \
	"XmTextReplace",	"TextOp -r",		N_FREE, \
	"XmTextGetSelection",	"TextOp -G",		N_FREE, \
	"XmTextSetSelection",	"TextOp -S",		N_FREE, \
	"XmTextClearSelection",	"TextOp -C",		N_FREE, \
	"textget",		"TextOp -g",		N_FREE, \
	"textset",		"TextOp -s",		N_FREE, \
	"textreplace",		"TextOp -r",		N_FREE, \
	"textgetsel",		"TextOp -G",		N_FREE, \
	"textsetsel",		"TextOp -S",		N_FREE, \
	"textclearsel",		"TextOp -C",		N_FREE, \
	"textappend",		"TextOp -a",		N_FREE, \
	"textinsert",		"TextOp -i",		N_FREE, \
	"textshow",		"TextOp -d",		N_FREE, \
	"textlastpos",		"TextOp -l",		N_FREE, \
	"mnsetup",	"MnemonicOp -s",		N_FREE, \
	"mnclear",	"MnemonicOp -c",		N_FREE, \
	"mainwsetareas","XmMainWindowSetAreas",		N_FREE, \
	"addprotocolcb","XmAddProtocolCallback",	N_FREE, \
	"dataprint",	"DataOp -p",			N_FREE, \
	"datareset",	"DataOp -r",			N_FREE, \
	"rh",		"RegisterHelp",			N_FREE, \
	"curbusy",	"XDefineCursor -b",	N_FREE, \
	"curstand",	"XDefineCursor -s",	N_FREE, \
	"crtarrowb",	"XmCreateArrowButton -m",	N_FREE, \
	"crtarrowbg",	"XmCreateArrowButtonGadget -m",	N_FREE, \
	"crtbulletinb",	"XmCreateBulletinBoard -m",	N_FREE, \
	"crtbulletinbd","XmCreateBulletinBoardDialog -m",	N_FREE, \
	"crtcascadeb",	"XmCreateCascadeButton -m",	N_FREE, \
	"crtcascadebg",	"XmCreateCascadeButtonGadget -m",	N_FREE, \
	"crtcommand",	"XmCreateCommand -m",		N_FREE, \
	"crtdialogs",	"XmCreateDialogShell -m",	N_FREE, \
	"crtdrawinga",	"XmCreateDrawingArea -m",	N_FREE, \
	"crtdrawnb",	"XmCreateDrawnButton -m",	N_FREE, \
	"crterrord",	"XmCreateErrorDialog -m",	N_FREE, \
	"crtfilesb",	"XmCreateFileSelectionBox -m",	N_FREE, \
	"crtfilesd",	"XmCreateFileSelectionDialog -m",N_FREE, \
	"crtform",	"XmCreateForm -m",		N_FREE, \
	"crtformd",	"XmCreateFormDialog -m",	N_FREE, \
	"crtframe",	"XmCreateFrame -m",		N_FREE, \
	"crtinformationd","XmCreateInformationDialog -m",N_FREE, \
	"crtlabel",	"XmCreateLabel -m",		N_FREE, \
	"crtlabelg",	"XmCreateLabelGadget -m",	N_FREE, \
	"crtlist",	"XmCreateList -m",		N_FREE, \
	"crtmainw",	"XmCreateMainWindow -m",	N_FREE, \
	"crtmenub",	"XmCreateMenuBar -m",		N_FREE, \
	"crtmenus",	"XmCreateMenuShell -m",		N_FREE, \
	"crtmessageb",	"XmCreateMessageBox -m",	N_FREE, \
	"crtmessaged",	"XmCreateMessageDialog -m",	N_FREE, \
	"crtoptionm",	"XmCreateOptionMenu -m",	N_FREE, \
	"crtpanedw",	"XmCreatePanedWindow -m",	N_FREE, \
	"crtpopupm",	"XmCreatePopupMenu -m",		N_FREE, \
	"crtpromptd",	"XmCreatePromptDialog -m",	N_FREE, \
	"crtpulldownm",	"XmCreatePulldownMenu -m",	N_FREE, \
	"crtpushb",	"XmCreatePushButton -m",	N_FREE, \
	"crtpushbg",	"XmCreatePushButtonGadget -m",	N_FREE, \
	"crtquestiond",	"XmCreateQuestionDialog -m",	N_FREE, \
	"crtradiob",	"XmCreateRadioBox -m",		N_FREE, \
	"crtrowc",	"XmCreateRowColumn -m",		N_FREE, \
	"crtscale",	"XmCreateScale -m",		N_FREE, \
	"crtscrollb",	"XmCreateScrollBar -m",		N_FREE, \
	"crtscrolledl",	"XmCreateScrolledList -m",	N_FREE, \
	"crtscrolledt",	"XmCreateScrolledText -m",	N_FREE, \
	"crtscrolledw",	"XmCreateScrolledWindow -m",	N_FREE, \
	"crtselectionb","XmCreateSelectionBox -m",	N_FREE, \
	"crtselectiond","XmCreateSelectionDialog -m",	N_FREE, \
	"crtseparator",	"XmCreateSeparator -m",		N_FREE, \
	"crtseparatorg","XmCreateSeparatorGadget -m",	N_FREE, \
	"crttext",	"XmCreateText -m",		N_FREE, \
	"crttextf",	"XmCreateTextField -m",		N_FREE, \
	"crttoggleb",	"XmCreateToggleButton -m",	N_FREE, \
	"crttogglebg",	"XmCreateToggleButtonGadget -m",N_FREE, \
	"crtwarningd",	"XmCreateWarningDialog -m",	N_FREE, \
	"crtworkingd",	"XmCreateWorkingDialog -m",	N_FREE,

#define WK_TK_EXTRA_TABLE \
	{ "DmCreateIconContainer", VALPTR(do_DmCreateIconContainer), N_BLTIN|BLT_FSUB}, \
	{ "ListOp", VALPTR(do_ListOp), N_BLTIN|BLT_FSUB},  \
	{ "TextOp", VALPTR(do_TextOp), N_BLTIN|BLT_FSUB},  \
	{ "MnemonicOp", VALPTR(do_MnemonicOp), N_BLTIN|BLT_FSUB},  \
	{ "DataOp", VALPTR(do_DataOp), N_BLTIN|BLT_FSUB},  \
	{ "RegisterHelp", VALPTR(do_RegisterHelp), N_BLTIN|BLT_FSUB},  \
	{ "XmAddProtocolCallback", VALPTR(do_XmAddProtocolCallback), N_BLTIN|BLT_FSUB},  \
	{ "XmMainWindowSetAreas", VALPTR(do_XmMainWindowSetAreas), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateArrowButton", VALPTR(do_XmCreateArrowButton), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateArrowButtonGadget", VALPTR(do_XmCreateArrowButtonGadget), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateBulletinBoard", VALPTR(do_XmCreateBulletinBoard), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateBulletinBoardDialog", VALPTR(do_XmCreateBulletinBoardDialog), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateCascadeButton", VALPTR(do_XmCreateCascadeButton), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateCascadeButtonGadget", VALPTR(do_XmCreateCascadeButtonGadget), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateCommand", VALPTR(do_XmCreateCommand), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateDialogShell", VALPTR(do_XmCreateDialogShell), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateDrawingArea", VALPTR(do_XmCreateDrawingArea), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateDrawnButton", VALPTR(do_XmCreateDrawnButton), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateErrorDialog", VALPTR(do_XmCreateErrorDialog), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateFileSelectionBox", VALPTR(do_XmCreateFileSelectionBox), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateFileSelectionDialog", VALPTR(do_XmCreateFileSelectionDialog), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateForm", VALPTR(do_XmCreateForm), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateFormDialog", VALPTR(do_XmCreateFormDialog), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateFrame", VALPTR(do_XmCreateFrame), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateInformationDialog", VALPTR(do_XmCreateInformationDialog), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateLabel", VALPTR(do_XmCreateLabel), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateLabelGadget", VALPTR(do_XmCreateLabelGadget), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateList", VALPTR(do_XmCreateList), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateMainWindow", VALPTR(do_XmCreateMainWindow), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateMenuBar", VALPTR(do_XmCreateMenuBar), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateMenuShell", VALPTR(do_XmCreateMenuShell), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateMessageBox", VALPTR(do_XmCreateMessageBox), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateMessageDialog", VALPTR(do_XmCreateMessageDialog), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateOptionMenu", VALPTR(do_XmCreateOptionMenu), N_BLTIN|BLT_FSUB},  \
	{ "XmCreatePanedWindow", VALPTR(do_XmCreatePanedWindow), N_BLTIN|BLT_FSUB},  \
	{ "XmCreatePopupMenu", VALPTR(do_XmCreatePopupMenu), N_BLTIN|BLT_FSUB},  \
	{ "XmCreatePromptDialog", VALPTR(do_XmCreatePromptDialog), N_BLTIN|BLT_FSUB},  \
	{ "XmProcessTraversal", VALPTR(do_XmProcessTraversal), N_BLTIN|BLT_FSUB},  \
	{ "XmCreatePulldownMenu", VALPTR(do_XmCreatePulldownMenu), N_BLTIN|BLT_FSUB},  \
	{ "XmCreatePushButton", VALPTR(do_XmCreatePushButton), N_BLTIN|BLT_FSUB},  \
	{ "XmCreatePushButtonGadget", VALPTR(do_XmCreatePushButtonGadget), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateQuestionDialog", VALPTR(do_XmCreateQuestionDialog), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateRadioBox", VALPTR(do_XmCreateRadioBox), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateRowColumn", VALPTR(do_XmCreateRowColumn), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateScale", VALPTR(do_XmCreateScale), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateScrollBar", VALPTR(do_XmCreateScrollBar), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateScrolledList", VALPTR(do_XmCreateScrolledList), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateScrolledText", VALPTR(do_XmCreateScrolledText), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateScrolledWindow", VALPTR(do_XmCreateScrolledWindow), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateSelectionBox", VALPTR(do_XmCreateSelectionBox), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateSelectionDialog", VALPTR(do_XmCreateSelectionDialog), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateSeparator", VALPTR(do_XmCreateSeparator), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateSeparatorGadget", VALPTR(do_XmCreateSeparatorGadget), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateText", VALPTR(do_XmCreateText), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateTextField", VALPTR(do_XmCreateTextField), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateToggleButton", VALPTR(do_XmCreateToggleButton), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateToggleButtonGadget", VALPTR(do_XmCreateToggleButtonGadget), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateWarningDialog", VALPTR(do_XmCreateWarningDialog), N_BLTIN|BLT_FSUB},  \
	{ "XmCreateWorkingDialog", VALPTR(do_XmCreateWorkingDialog), N_BLTIN|BLT_FSUB},

#define WK_TK_EXTRA_VAR
