#===============================================================================
#
#	ident @(#) uimenu.tcl 11.1 97/10/30 
#
#******************************************************************************
#
#	Copyright (C) 1993-1997 The Santa Cruz Operation, Inc.
#		All Rights Reserved.
#
#	The information in this file is provided for the exclusive use of
#	the licensees of The Santa Cruz Operation, Inc.  Such users have the
#	right to use, modify, and incorporate this code into other products
#	for purposes authorized by the license agreement provided they include
#	this notice and the associated copyright notice with any such product.
#	The information in this file is provided "AS IS" without warranty.
#
#******************************************************************************
#
# SCO Mail Administration Domain client:
#	Menu portion of the User Interface Module(s): UI. vtcl binding.
#
#
# Modification History
#
# M000, 20-Feb-97, andrean
#	- Created.
#
#===============================================================================

#====================================================================== XXX ===
# UiBuildMenus
#   
# Driver for building all the pulldown menus.
#------------------------------------------------------------------------------
proc UiBuildMenus {form} {
	global appvals

	set helpMenuItemList [list \
		ON_CONTEXT \
		ON_WINDOW \
		ON_KEYS \
		INDEX \
		ON_HELP]
		
	set menuBar [VtMenuBar $form.menuBar \
			-helpMenuItemList $helpMenuItemList]
	VxSetVar $form menuBar $menuBar

	# Main object menu
	UiBuildObjectMenu $menuBar

	# Options menu
	UiBuildOptionsMenu $menuBar

	return $menuBar
}

#====================================================================== XXX ===
# UiBuildObjectMenu
#   
# Build the object(Who) menu including:
# o Examine...
#------------------------------------------------------------------------------
proc UiBuildObjectMenu {parent} {
	global appvals

	set label $appvals(object) 
	set mnemonic $appvals(objectmn) 
	set fileMenu [VtPulldown $parent.fileMenu \
			-label $label \
			-mnemonic $mnemonic \
			]
	VxSetVar $parent fileMenu $fileMenu

	set add [VtPushButton $fileMenu.add \
		-label [IntlMsg ENTRY_MENU_ADD_LBL] \
		-mnemonic [IntlMsg ENTRY_MENU_ADD_MN]  \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString [IntlMsg ENTRY_MENU_ADD_SH] \
		-callback UiAddEntryCB  \
		-autoLock UiAddEntryCB]
	VxSetVar $parent add $add
	set delete [VtPushButton $fileMenu.delete \
		-label [IntlMsg ENTRY_MENU_DELETE_LBL] \
		-mnemonic [IntlMsg ENTRY_MENU_DELETE_MN]  \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString [IntlMsg ENTRY_MENU_DELETE_SH] \
		-callback UiDeleteEntryCB  \
		-autoLock UiDeleteEntryCB]
	VxSetVar $parent delete $delete
	set modify [VtPushButton $fileMenu.modify \
		-label [IntlMsg ENTRY_MENU_MODIFY_LBL] \
		-mnemonic [IntlMsg ENTRY_MENU_MODIFY_MN]  \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString [IntlMsg ENTRY_MENU_MODIFY_SH] \
		-callback UiModifyEntryCB  \
		-autoLock UiModifyEntryCB]
	VxSetVar $parent modify $modify

	VtSeparator $fileMenu.s1
	set moveUp [VtPushButton $fileMenu.moveUp \
		-label [IntlMsg ENTRY_MENU_UP_LBL] \
		-mnemonic [IntlMsg ENTRY_MENU_UP_MN]  \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString [IntlMsg ENTRY_MENU_UP_SH] \
		-callback UiMoveUpEntryCB  \
		-autoLock UiMoveUpEntryCB]
	VxSetVar $parent moveUp $moveUp

	set moveDown [VtPushButton $fileMenu.moveDown \
		-label [IntlMsg ENTRY_MENU_DOWN_LBL] \
		-mnemonic [IntlMsg ENTRY_MENU_DOWN_MN]  \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString [IntlMsg ENTRY_MENU_DOWN_SH] \
		-callback UiMoveDownEntryCB  \
		-autoLock UiMoveDownEntryCB]
	VxSetVar $parent moveDown $moveDown

	VtSeparator $fileMenu.s2
	set save [VtPushButton $fileMenu.save \
		-label [IntlMsg ENTRY_MENU_SAVE_LBL] \
		-mnemonic [IntlMsg ENTRY_MENU_SAVE_MN]  \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString [IntlMsg ENTRY_MENU_SAVE_SH] \
		-callback UiSaveCB  \
		-autoLock UiSaveCB]
	VxSetVar $parent save $save

	VtSeparator $fileMenu.s3
	set exitMenu $fileMenu
	# exit button
	VtPushButton $exitMenu.exit \
		-label [IntlMsg ENTRY_MENU_EXIT_LBL] \
		-mnemonic [IntlMsg ENTRY_MENU_EXIT_MN]  \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString \ [IntlMsg ENTRY_MENU_EXIT_SH \
					[list $appvals(title)]] \
		-callback UiCloseCB  \
		-autoLock UiCloseCB
}

#====================================================================== XXX ===
# UiBuildOptionsMenu
#   
# Build the option menu including:
# o short help toggle 
#------------------------------------------------------------------------------
proc UiBuildOptionsMenu {form} {
	global appvals 

	# Define the Pulldown itself
	set optionMenu [VtPulldown $form.optionMenu \
		-label  [IntlMsg OPTIONS_MENU] \
		-mnemonic [IntlMsg OPTIONS_MN] \
		] 

	# menu items

	# short help
	SaShortHelpLoad $appvals(client)
	SaShortHelpMenuOptions $optionMenu

	# toolbar
	if {$appvals(toolbar) && ![VtInfo -charm]} {
		UiToolBarBuildMenuOptions $optionMenu
	}

	VxSetVar $form optionMenu $optionMenu
	return $optionMenu
}

