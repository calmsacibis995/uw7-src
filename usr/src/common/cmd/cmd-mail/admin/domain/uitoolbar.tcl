#===============================================================================
#
#	ident @(#) uitoolbar.tcl 11.1 97/10/30 
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
#	Toolbar portion of the User Interface Module(s): UI. vtcl binding.
#
#
# Modification History
#
# M000, 20-Feb-97, andrean
#	- Created.
#
#===============================================================================

#-------------------------------------------------------------------------------
# Toolbar Module 
# 
# This module implements the toolbar.  Currently, toolbar is manually
# created -- we do *not* use SaToolBar functions, due to the fact that
# some of our toolbar icons are not pixmaps, but simply strings.
#
#-------------------------------------------------------------------------------

@if notused
#====================================================================== XXX ===
# UiToolBarResensitizeCB
#   
# The toolbar has changed and new buttons may be present without proper
# sensitization.
#------------------------------------------------------------------------------
proc UiToolBarResensitizeCB {keys cbs} {
	# Toolbar has possibly been modified.
	# traverse key list and recompute and sensitize each button
	UiSensitizeMainFunctions
}
@endif


#====================================================================== XXX ===
# UiToolBarSetVisibilityCB
#   
# The toobar visibility option has been selected (Options menu) --
# show or hide the toolbar depending upon the selection, and
# save the toolbarVisibility preference for the app.
#------------------------------------------------------------------------------
proc UiToolBarSetVisibilityCB {{cbs {}}} {
	global appvals

	set menuBar [VxGetVar $appvals(vtMain) menuBar]
	set optionMenu [VxGetVar $menuBar optionMenu]
	set toolBarToggle [VxGetVar $optionMenu toolBarToggle]
	set visible [VtGetValues $toolBarToggle -value]

	set toolBar [VxGetVar $appvals(vtMain) toolBar]
	set toolBarFrame [VxGetVar $appvals(vtMain) toolBarFrame]

	if {$visible} {
		# We *must* show the frame first, then the form
		VtShow $toolBarFrame
		VtShow $toolBar
	} else {
		# We *must* hide the form first, then the frame, in order
		# to obtain same re-drawing behaviour we get with Sa functions
		VtHide $toolBar
		VtHide $toolBarFrame
	}
	SaScreenPolicySet $appvals(client) toolbarVisibility $visible
}

#====================================================================== XXX ===
# UiToolBarBuildMenuOptions
#   
# Build the menu options that exist for the toolbar.
# Parameters:
#	menu - The widget name of the menu this option will belong under.
#------------------------------------------------------------------------------
proc UiToolBarBuildMenuOptions {menu} {
	global appvals

	set toolBarToggle [VtToggleButton $menu.toolBarToggle \
		-label [IntlMsg OPTIONS_TOOLBAR] \
		-callback UiToolBarSetVisibilityCB \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString [IntlMsg OPTIONS_TOOLBAR_SH] \
		-value 1]

	VxSetVar $menu toolBarToggle $toolBarToggle
}

#====================================================================== XXX ===
# UiBuildToolBar
#   
# Builds the toolbar on the main form along with supporting data structures.
#------------------------------------------------------------------------------
proc UiBuildToolBar {form top} {
	global appvals

	set toolBarFrame [VtFrame $form.toolBarFrame \
			-topSide $top \
			-topOffset 2 \
			-leftSide FORM \
			-rightSide FORM \
			-marginHeight 0 \
			-marginWidth 0 \
			-horizontalSpacing 0 \
			-verticalSpacing 0 \
			-shadowType IN]
	set toolBarForm [VtForm $toolBarFrame.toolBarForm \
			-leftSide FORM \
			-rightSide FORM \
			-borderWidth 0 \
			-marginHeight 0 \
			-marginWidth 0 \
			-horizontalSpacing 0 \
			-verticalSpacing 0]
	set addTB [VtPushButton $toolBarForm.addTB \
			-label [IntlMsg TB_ADD] \
			-topSide FORM \
			-topOffset 0 \
			-leftSide FORM \
			-leftOffset 0 \
			-rightOffset 0 \
			-bottomSide FORM \
			-bottomOffset 0 \
			-shortHelpCallback SaShortHelpCB \
			-shortHelpString [IntlMsg ENTRY_MENU_ADD_SH] \
			-callback UiAddEntryCB \
			-autoLock UiAddEntryCB]
	VxSetVar $toolBarForm addTB $addTB
	set deleteTB [VtPushButton $toolBarForm.deleteTB \
			-label [IntlMsg TB_DELETE] \
			-topSide FORM \
			-leftSide $addTB \
			-leftOffset 0 \
			-bottomSide FORM \
			-bottomOffset 0 \
			-shortHelpCallback SaShortHelpCB \
			-shortHelpString [IntlMsg ENTRY_MENU_DELETE_SH] \
			-callback UiDeleteEntryCB \
			-autoLock UiDeleteEntryCB]
	VxSetVar $toolBarForm deleteTB $deleteTB
	set modifyTB [VtPushButton $toolBarForm.modifyTB \
			-label [IntlMsg TB_MODIFY] \
			-topSide FORM \
			-leftSide $deleteTB \
			-leftOffset 10 \
			-bottomSide FORM \
			-bottomOffset 0 \
			-shortHelpCallback SaShortHelpCB \
			-shortHelpString [IntlMsg ENTRY_MENU_MODIFY_SH] \
			-callback UiModifyEntryCB \
			-autoLock UiModifyEntryCB]
	VxSetVar $toolBarForm modifyTB $modifyTB
	set upTB [VtPushButton $toolBarForm.upTB \
			-pixmap [IntlMsg BTN_UP_PX] \
			-topSide FORM \
			-leftSide $modifyTB \
			-leftOffset 10 \
			-bottomSide FORM \
			-bottomOffset 0 \
			-shortHelpCallback SaShortHelpCB \
			-shortHelpString [IntlMsg ENTRY_MENU_UP_SH] \
			-callback UiMoveUpEntryCB \
			-autoLock UiMoveUpEntryCB]
	VxSetVar $toolBarForm upTB $upTB
	set downTB [VtPushButton $toolBarForm.downTB \
			-pixmap [IntlMsg BTN_DOWN_PX] \
			-topSide FORM \
			-leftSide $upTB \
			-leftOffset 0 \
			-rightSide NONE \
			-bottomSide FORM \
			-bottomOffset 0 \
			-shortHelpCallback SaShortHelpCB \
			-shortHelpString [IntlMsg ENTRY_MENU_DOWN_SH] \
			-callback UiMoveDownEntryCB \
			-autoLock UiMoveDownEntryCB]
	VxSetVar $toolBarForm downTB $downTB
	VxSetVar $appvals(vtMain) toolBar $toolBarForm
	VxSetVar $appvals(vtMain) toolBarFrame $toolBarFrame
	return $toolBarFrame
}
