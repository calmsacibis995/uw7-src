#==============================================================================
# Audio Configuration Manager
#------------------------------------------------------------------------------
# uimenu.tcl
#------------------------------------------------------------------------------
# @(#)uimenu.tcl	7.1	97/10/22
#------------------------------------------------------------------------------
# User Interface: MenuBar, binding to SCO Visual Tcl
#------------------------------------------------------------------------------
# Revision History:
# 1996-Nov-01, shawnm, remove option menu
# 1996-Oct-28, shawnm, add test button
# 1996-Oct-08, shawnm, main screen prototype
# 1996-Sep-27, shawnm, created from template
#==============================================================================

#====================================================================== XXX ===
# UiBuildMenus
#   
# Driver for building all the pulldown menus.
#------------------------------------------------------------------------------
proc UiBuildMenus {form} {
	global appvals

	set menuBar [VtMenuBar $form.menuBar \
			-helpMenuItemList [SaHelpGetOptionsList]]
	VxSetVar $form menuBar $menuBar

	UiBuildHostMenu $menuBar

	UiBuildObjectMenu $menuBar

	return $menuBar
}

#====================================================================== XXX ===
# UiBuildHostMenu
#   
# Build the host menu including:
# o Open Host...
# o Exit 
#------------------------------------------------------------------------------
proc UiBuildHostMenu {parent} {
	global appvals

	set hostMenu [VtPulldown $parent.hostMenu \
			-label [IntlMsg HOST] \
			-mnemonic [IntlMsg HOST_MN] \
			]
	VxSetVar $parent hostMenu $hostMenu

	VtPushButton $hostMenu.openhost \
		-label [IntlMsg OPENHOST] \
		-mnemonic [IntlMsg OPENHOST_MN]  \
		-acceleratorString [IntlMsg OPENHOST_ACCSTR] \
		-accelerator [IntlMsg OPENHOST_ACC] \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString  [IntlMsg OPENHOST_SH] \
		-callback SaOpenHostCB  \
		-autoLock SaOpenHostCB
	VtSeparator $hostMenu.s1
	set exitMenu $hostMenu

	# exit button
	VtPushButton $exitMenu.exit \
		-label [IntlMsg EXIT] \
		-mnemonic [IntlMsg EXIT_MN]  \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString  [IntlMsg EXIT_SH [list $appvals(title)]] \
		-callback UiCloseCB  \
		-autoLock UiCloseCB

		# Correct/consistent UI does not use Exit accelerators
		#-acceleratorString "Ctrl+X" 
		#-accelerator "Ctrl<Key>X" 
}

#====================================================================== XXX ===
# UiBuildObjectMenu
#   
# Build the Soundcard menu
# o Add...
# o Examine...
# o Remove...
# o Test...
#------------------------------------------------------------------------------
proc UiBuildObjectMenu {parent} {
	global appvals

	set label [IntlMsg SOUNDCARD]
	set mnemonic [IntlMsg SOUNDCARD_MN]
	set fileMenu [VtPulldown $parent.fileMenu \
			-label $label \
			-mnemonic $mnemonic \
			]
	VxSetVar $parent fileMenu $fileMenu

	# add button
	set add [VtPushButton $fileMenu.add \
		-label [IntlMsg ADD] \
		-mnemonic [IntlMsg ADD_MN]  \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString  [IntlMsg ADD_SH] \
		-callback UiAddCB  \
		-autoLock UiAddCB]
	VxSetVar $parent add $add

	# examine button
	set examine [VtPushButton $fileMenu.examine \
		-label [IntlMsg EXAMINE] \
		-mnemonic [IntlMsg EXAMINE_MN]  \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString  [IntlMsg EXAMINE_SH] \
		-callback UiExamineCB  \
		-autoLock UiExamineCB]
	VxSetVar $parent examine $examine

	# test button
	set test [VtPushButton $fileMenu.test \
		-label [IntlMsg TEST] \
		-mnemonic [IntlMsg TEST_MN]  \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString  [IntlMsg TEST_SH] \
		-callback UiTestCB  \
		-autoLock UiTestCB]
	VxSetVar $parent test $test

	# remove button
	set remove [VtPushButton $fileMenu.remove \
		-label [IntlMsg REMOVE] \
		-mnemonic [IntlMsg REMOVE_MN]  \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString  [IntlMsg REMOVE_SH] \
		-callback UiRemoveCB  \
		-autoLock UiRemoveCB]
	VxSetVar $parent remove $remove
}

