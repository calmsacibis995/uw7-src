#===============================================================================
#
#	ident @(#) ui.tcl 11.1 97/10/30 
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
#        User Interface Module(s): UI. Binding to vtcl, the widget server
#
#
# Modification History
#
# M000, 20-Feb-97, andrean
#	- Created.
#
#===============================================================================

#-------------------------------------------------------------------------------
# User Interface Modules: UI
#
# This file contains all the Ui components for modularity.
# This binds the generic Ui API to vtcl.  While it may
# seem academic to abstract the vtcl binding using generic Ui* calls, 
# history and practice have shown that this is useful and necessary even for
# API changes within vtcl itself. It is highly recommended that this
# modular abstraction be diligently maintained for the convenience and
# sanity of future engineers. Negligible efficiency tradeoffs are not a 
# concern. 
#-------------------------------------------------------------------------------
# Fundamental Globals: appvals array
#
# To reduce global name space pollution, a handful of variables that are
# global for convenience are collected under one name, appvals, which is
# a tcl array.
#-------------------------------------------------------------------------------
set appvals(vtApp)	{}		;# VtOpen return; generally not used 
set appvals(vtMain)	{}		;# widget string of main dialog

# Graphical frill: toolbar for quick mapping of menu functions
set appvals(toolbar) 		1		;# toolbar
set appvals(toolbarcust) 	0		;# customizable toolbar
set appvals(fieldAdjustment)	0.90		;# adjustment for string lengths
						 # for drawn list in graphical

#-------------------------------------------------------------------------------
# Generic UI Functions
#-------------------------------------------------------------------------------

@if notused
#====================================================================== XXX ===
# UiWidgetGet
#   
# Mechanism for retrieving widget strings with a symbolic name under an 
# optional parent. Defaults to vtMain
#------------------------------------------------------------------------------
proc UiWidgetGet {widget {parent {}}} {
	global appvals
	if {[lempty $parent]} {
		set parent $appvals(vtMain)
	}
	return [VxGetVar $parent $widget]
}
@endif

#====================================================================== XXX ===
# UiDefaultFormSpacingSet
#   
# Every form gets the same spacing and margins: all 0
#------------------------------------------------------------------------------
proc UiDefaultFormSpacingSet {form} {
	VtSetValues $form \
		-horizontalSpacing 0 \
		-verticalSpacing 0 \
		-marginWidth 0 \
		-marginHeight 0 
}

#====================================================================== XXX ===
# UiDisplayErrorStackCB
#   
# Call back for OK button on error stack dialog. 
# Return control to original caller if arranged.
#------------------------------------------------------------------------------
proc UiDisplayErrorStackCB {callback cbs} {
	global appvals

	set dialog [keylget cbs dialog]
	VtDestroyDialog $dialog
	SaStatusBarClear [VxGetVar $appvals(vtMain) statusBar]

	if {![lempty $callback]} {
		eval $callback [list $cbs]
	}
}

#====================================================================== XXX ===
# UiDisplayErrorStacks
#   
# Error stack dialog
# o Deal with autorefresh if necessary
# o return control to caller if desired
#------------------------------------------------------------------------------
proc UiDisplayErrorStacks {caller stack {callback ""}} {
	global appvals

	SaDisplayErrorStacks $appvals(vtMain).$caller [list $stack] HELP \
		[list UiDisplayErrorStackCB $callback]
}

#====================================================================== XXX ===
# UiSetAppFocus
#   
# Sets proper widget focus depending on contents of main list 
#------------------------------------------------------------------------------
proc UiSetAppFocus {} {
	global appvals

	VtSetFocus [VxGetVar $appvals(vtMain) menuBar] 
	if {![lempty [UiMainListGetItems]]} {
		VtSetFocus [VxGetVar $appvals(vtMain) mainList]
	}
}

#------------------------------------------------------------------------------
# MainList Functions:
#------------------------------------------------------------------------------

proc AdjustFieldLength {charLength} {
	global appvals
	set fieldLength [expr $charLength * $appvals(fieldAdjustment)]
	return $fieldLength
}

proc UiMainListSetItems {items} {
	global appvals

	set mainList [VxGetVar $appvals(vtMain) mainList]
	VtDrawnListDeleteItem $mainList -all

	set maxLength [VxGetVar $mainList maxLength]
	DomainSetMainListLabel $maxLength

	set isCharm [VtInfo -charm]

	set CHARM_formatList [list  \
		[list STRING 1 1 1] \
		[list STRING $maxLength 1 1]]
	set MOTIF_formatList [list  \
		[list STRING 1 10 10] \
		[list STRING [AdjustFieldLength $maxLength] 0 10]]

	foreach entry $items {
		keylget entry partial partial
		keylget entry hostName hostName
		keylget entry realName realName
		keylget entry route route

		# set up the first two columns in this list item--
		# these are always there: 
		#	indicator of partial domain matching
		#	hostname (key of domain table entry)
		if {$partial == "TRUE"} {
			if {$isCharm} {
				set partial [IntlMsg COLUMN_PARTIAL_LBL_CHARM]
			} else {
				set partial [IntlMsg COLUMN_PARTIAL_LBL]
			}
		} else {
			set partial [IntlMsg COLUMN_PARTIAL_NONE]
		}
		set fieldList [list $partial $hostName]

		if {$isCharm} {
			set formatList $CHARM_formatList
		} else {
			set formatList $MOTIF_formatList
		}

		# for each hop in the route (except final),
		# add the string, then the arrow icon
		set routeString ""
		foreach hop $route {
			append routeString "$hop > "
		}

		# add on the final hop in route
		append routeString "$realName"
		lappend fieldList $routeString
		lappend formatList [list \
				STRING \
				[AdjustFieldLength [clength $routeString]]]
		lappend formatList [list DATA]
		lappend fieldList $entry

		VtDrawnListAddItem $mainList \
			-formatList $formatList \
			-fieldList $fieldList
	}
}

proc UiMainListGetItems {} {
	global appvals

	return [VtDrawnListGetItem [VxGetVar $appvals(vtMain) mainList] -all]
}

#====================================================================== XXX ===
# UiMainListCB
#   
#------------------------------------------------------------------------------
proc UiMainListCB {cbs} {
	global appvals

	# update selected label and widget sensitivity
	UiUpdateCountLabel
	UiSensitizeMainFunctions
}

#====================================================================== XXX ===
# UiMainListDoubleCB
#   
# double click functionality for main list 
#------------------------------------------------------------------------------
proc UiMainListDoubleCB {cbs} {
	UiModifyEntryCB $cbs
	VtUnLock
}

#====================================================================== XXX ===
# UiBuildMainList
#   
# Build the main form data list
#------------------------------------------------------------------------------
proc UiBuildMainList {form top} {
	global appvals 

	# Build basic main form drawnlist widget including:
	# o Manually built title label for CHARM
	# o Drawnlist record format
	# o traditional Count label beneath the drawn list

	set labelFont medNormalFont

	set mainList [DomainBuildMainList $form $top]
	VxSetVar $form mainList $mainList

	set labelForm [VtForm $form.labelForm \
		-topSide NONE \
		-leftSide FORM \
		-rightSide FORM \
		-bottomSide FORM]

        # Define the partial lookup info label

	if {[VtInfo -charm]} {
		set label [IntlMsg PARTIAL_LABEL_STR_CHARM] 
	} else {
		set label [IntlMsg PARTIAL_LABEL_STR] 
	}
	set partialLabel [VtLabel $labelForm.partialLabel \
		-label $label \
		-font smallNormalFont \
		-topSide FORM \
		-topOffset 0 \
		-leftSide FORM \
		-MOTIF_leftOffset 5 -CHARM_leftOffset 0 \
		-rightSide NONE \
		-bottomSide FORM \
		-labelLeft]
	VxSetVar $form partialLabel $partialLabel

	# Define the accompanying count label
	set countLabel [VtLabel $labelForm.countLabel -label " " \
				-topSide FORM \
				-leftSide $partialLabel \
				-rightSide FORM \
				-bottomSide FORM \
				-labelRight]
	VxSetVar $form countLabel $countLabel

	VtSetValues $mainList -bottomSide $labelForm

	# Stash the localized string "Selected" here so we only grab it once
	# from the message catalog
	VxSetVar $countLabel selected [IntlMsg SELECTED]
	
	# Return the bottom-most widget for further attachments
	return $labelForm
}

#====================================================================== XXX ===
# UiUpdateCountLabel
#   
# Update main list count, selected label
#------------------------------------------------------------------------------
proc UiUpdateCountLabel {} {
	global appvals

	set mainList [VxGetVar $appvals(vtMain) mainList]
	set countLabel [VxGetVar $appvals(vtMain) countLabel]
	set numSelected [llength \
		[VtDrawnListGetSelectedItem $mainList -byRecordList]]
        lappend labelArgs [llength [VtDrawnListGetItem $mainList -all]]
        lappend labelArgs $appvals(itemname)
        lappend labelArgs $numSelected
        lappend labelArgs [VxGetVar $countLabel selected]
        set labelValue [IntlMsg COUNT_LABEL_STR $labelArgs]
	VtSetValues $countLabel -label $labelValue
}

#------------------------------------------------------------------------------
# Add/Modify Functions:
#------------------------------------------------------------------------------

proc UiBuildAddModifyScreen {} {
	global appvals
	
	set isCharm [VtInfo -charm]
	set parent $appvals(vtMain)
	set addModifyScreen [VtFormDialog $parent.addModifyScreen \
		-ok -okLabel [IntlMsg OK] \
		-cancel -cancelLabel [IntlMsg CANCEL] \
		-help -helpLabel [IntlMsg HELP] \
		-cancelButton CANCEL \
		-okCallback AddModifyOkCB \
		-cancelCallback AddModifyCancelCB \
		-wmCloseCallback AddModifyCancelCB \
		-autoLock [list AddModifyOkCB AddModifyCancelCB] \
		-resizable FALSE]
	VxSetVar $parent addModifyScreen $addModifyScreen
	UiDefaultFormSpacingSet $addModifyScreen
	SaCharmSetMaxFormDimensions $addModifyScreen 1

	set nameForm [VtForm $addModifyScreen.nameForm \
		-leftSide FORM -MOTIF_leftOffset 0 -CHARM_leftOffset 1 \
		-rightSide FORM -MOTIF_rightOffset 0 -CHARM_rightOffset 1\
		-topSide FORM -MOTIF_topOffset 10 -CHARM_topOffset 1]
	set nameLabel [VtLabel $nameForm.nameLabel \
		-label [IntlMsg NAME_LBL] \
		-labelLeft ]
	set nameText [VtText $nameForm.nameText \
		-leftSide FORM -rightSide FORM \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString [IntlMsg NAME_SH] \
		-callback {SaSetFocus next}]
	VxSetVar $addModifyScreen nameText $nameText

	set realNameForm [VtForm $addModifyScreen.realNameForm \
		-leftSide FORM -MOTIF_leftOffset 0 -CHARM_leftOffset 1 \
		-rightSide FORM -MOTIF_rightOffset 0 -CHARM_rightOffset 1\
		-topSide $nameForm -MOTIF_topOffset 0 -CHARM_topOffset 1]
	set realNameLabel [VtLabel $realNameForm.realNameLabel \
		-label [IntlMsg REAL_NAME_LBL] \
		-labelLeft ]
	set realNameText [VtText $realNameForm.realNameText \
		-leftSide FORM -rightSide FORM \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString [IntlMsg REAL_NAME_SH] \
		-callback {SaSetFocus next}]
	VxSetVar $addModifyScreen realNameText $realNameText

	set routeForm [VtForm $addModifyScreen.routeForm \
		-leftSide FORM -alignLeft $realNameForm\
		-rightSide FORM \
		-topSide $realNameForm \
		-bottomSide NONE \
		-MOTIF_topOffset 10 -CHARM_topOffset 1]
	set labelList [list [IntlMsg ROUTE_LIST_LBL]]
	if {$isCharm} {
		set labelFormat "%s"
		set label [format $labelFormat [lindex $labelList 0]]
		set routeLabel [VtLabel $routeForm.routeLabel \
			-labelLeft \
			-leftSide FORM \
			-leftOffset 0 \
			-rightSide FORM \
			-topSide FORM \
			-bottomSide NONE \
			-label $label]
	}
	set routeList [VtDrawnList $routeForm.routeList \
		-labelFormatList {{STRING 20}} \
		-labelList $labelList \
		-formatList {{STRING 20}} \
		-columns 25 -CHARM_columns 35 \
		-rows 6 \
		-topSide FORM -bottomSide NONE \
		-leftSide FORM -rightSide FORM\
		-horizontalScrollBar TRUE \
		-selection MULTIPLE \
		-callback RouteListCB]
	VxSetVar $addModifyScreen routeList $routeList
	if {$isCharm} {
		VtSetValues $routeList -topSide $routeLabel
	}

	set buttonForm [VtForm $routeForm.buttonForm \
		-leftSide NONE -rightSide FORM \
		-MOTIF_rightOffset 0 -CHARM_rightOffset 1 \
		-topSide FORM -bottomSide NONE \
		-MOTIF_topOffset 15 -CHARM_topOffset 1]
	VtSetValues $routeList -rightSide $buttonForm
	set pbRowCol1 [VtRowColumn $buttonForm.pbForm1 \
		-topSide FORM -bottomSide NONE \
		-leftSide FORM -rightSide FORM \
		-vertical]
	set routeAddButton [VtPushButton $pbRowCol1.routeAddButton \
		-label [IntlMsg ROUTE_ADD_BTN_LBL] \
		-labelCenter \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString \
			[IntlMsg ROUTE_ADD_BTN_SH] \
		-callback AddToRouteCB \
		-autoLock AddToRouteCB ]
	VxSetVar $addModifyScreen routeAddButton $routeAddButton
	set routeDeleteButton [VtPushButton $pbRowCol1.routeDeleteButton \
		-label [IntlMsg ROUTE_DELETE_BTN_LBL] \
		-labelCenter \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString \
			[IntlMsg ROUTE_DELETE_BTN_SH] \
		-callback DeleteFromRouteCB \
		-autoLock DeleteFromRouteCB ]
	VxSetVar $addModifyScreen routeDeleteButton $routeDeleteButton
		
	set pbRowCol2 [VtRowColumn $buttonForm.pbRowCol2 \
		-topSide $pbRowCol1 -topOffset 0 -bottomSide NONE \
		-leftSide FORM -rightSide FORM \
		-horizontal \
		-MOTIF_spacing 5 -CHARM_spacing 1 \
		-packing COLUMN]
	set routeMoveUpButton [VtPushButton $pbRowCol2.routeMoveUpButton \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString \
			[IntlMsg ROUTE_UP_BTN_SH] \
		-callback MoveUpRouteCB \
		-autoLock MoveUpRouteCB ]
	if {$isCharm} {
		VtSetValues $routeMoveUpButton \
			-label " [IntlMsg ENTRY_MENU_UP_LBL] " \
			-labelCenter
	} else {
		VtSetValues $routeMoveUpButton \
			-pixmap [IntlMsg BTN_UP_PX]
	}
	VxSetVar $addModifyScreen routeMoveUpButton $routeMoveUpButton
	set routeMoveDownButton [VtPushButton $pbRowCol2.routeMoveDownButton \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString \
			[IntlMsg ROUTE_DOWN_BTN_SH] \
		-callback MoveDownRouteCB \
		-autoLock MoveDownRouteCB ]
	if {$isCharm} {
		VtSetValues $routeMoveDownButton \
			-label " [IntlMsg ENTRY_MENU_DOWN_LBL] " \
			-labelCenter
	} else {
		VtSetValues $routeMoveDownButton \
			-pixmap [IntlMsg BTN_DOWN_PX]
	}
	VxSetVar $addModifyScreen routeMoveDownButton $routeMoveDownButton

	set optionsCheckBox [VtCheckBox $routeForm.optionsCheckBox \
		-topSide NONE -bottomSide FORM \
		-leftSide FORM -MOTIF_leftOffset 5 -CHARM_leftOffset 0]
	set doPartial [VtToggleButton $optionsCheckBox.doPartial \
		-label [IntlMsg DO_PARTIAL_LBL] \
		-set FALSE \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString [IntlMsg DO_PARTIAL_SH] \
		-callback {SaSetFocus next}]
	VxSetVar $addModifyScreen doPartial $doPartial

	set separator [VtSeparator $addModifyScreen.separator \
		-leftSide FORM -leftOffset 0 \
		-rightSide FORM -rightOffset 0 \
		-topSide NONE]

	set statusBar [SaStatusBar $addModifyScreen.statusbar 0]
	VtSetValues $separator \
		-bottomSide $statusBar -bottomOffset 0
	VtSetValues $routeList \
		-bottomSide $optionsCheckBox
	VtSetValues $routeForm \
		-bottomSide $separator

	SaSetTabGroups $buttonForm [list $pbRowCol1 $pbRowCol2]
	SaSetTabGroups $addModifyScreen [list \
		$nameText \
		$realNameText \
		$routeList \
		$buttonForm \
		$doPartial]

	set okButton [VtGetValues $addModifyScreen -ok]
	SaSetFocusList $addModifyScreen [list \
		$nameText \
		$realNameText \
		$buttonForm \
		$doPartial \
		$okButton]

	return $addModifyScreen
}

proc UiBuildAddToRouteScreen {parent} {

	set addToRouteScreen [VtFormDialog $parent.addToRouteScreen \
		-title [IntlMsg ROUTE_ADD_DLG_TITLE] \
		-ok -okLabel [IntlMsg OK] \
		-okCallback AddToRouteOkCB \
		-cancel -cancelLabel [IntlMsg CANCEL] \
		-cancelCallback AddToRouteCancelCB \
		-cancelButton CANCEL \
		-wmCloseCallback AddToRouteCancelCB \
		-autoLock [list AddToRouteOkCB AddToRouteCancelCB] ]
        VxSetVar $parent addToRouteScreen $addToRouteScreen

	set hostNameForm [VtForm $addToRouteScreen.hostNameForm \
		-marginHeight 0 -marginWidth 0 \
		-rightSide FORM -leftSide FORM \
		-topSide FORM -topOffset 10 -CHARM_topOffset 1]
	set hostNameLabel [VtLabel $hostNameForm.hostNameLabel \
		-label [IntlMsg ROUTE_ADD_DLG_TEXT_LBL] \
		-labelLeft ]
	set selectHostButton [VtPushButton $hostNameForm.selectHostButton \
		-label [IntlMsg ROUTE_ADD_DLG_BTN_LBL] \
		-leftSide NONE -rightSide FORM \
		-bottomSide FORM -MOTIF_bottomOffset 20 -CHARM_bottomOffset 0 \
		-callback SelectHostCB \
		-autoLock SelectHostCB]
        VxSetVar $addToRouteScreen selectHostButton $selectHostButton
	set hostNameText [VtText $hostNameForm.hostNameText \
		-columns 30  -CHARM_columns 35 \
		-leftSide FORM -rightSide $selectHostButton \
		-alignTop $selectHostButton \
		-horizontalScrollBar TRUE \
		-callback EnterHostNameCB \
		-autoLock EnterHostNameCB ]
        VxSetVar $addToRouteScreen hostNameText $hostNameText

	VtAddTabGroup $hostNameText
	VtAddTabGroup $selectHostButton

	return $addToRouteScreen
}

		
proc UiRouteListSetItems {items} {
	global appvals

	set addModifyScreen [VxGetVar $appvals(vtMain) addModifyScreen] 
	set routeList [VxGetVar $addModifyScreen routeList]
	VtDrawnListDeleteItem $routeList -all

	foreach entry $items {
		set fieldList [list $entry]
		set formatList [list [list STRING [clength $entry]]]
		VtDrawnListAddItem $routeList \
			-formatList $formatList \
			-fieldList $fieldList
	}
}
		
proc UiBuildSaveConfirmDialog {} {
	global appvals

	set parent $appvals(vtMain)
	# we don't do autoDestroy on this dialog, because we
	# want wmCloseCallback to do the cancel, and the dialog
	# cannot auto destroy in that case
	set saveConfirmDlg [VtQuestionDialog $parent.saveConfirmDlg \
		-title [IntlMsg SAVE_CONFIRM_TITLE] \
		-message [IntlMsg SAVE_CONFIRM_QUERY] \
		-autoDestroy FALSE \
		-okLabel [IntlMsg SAVE_CONFIRM_BTN_YES] \
		-ok -okCallback SaveConfirmOkCB \
		-applyLabel [IntlMsg SAVE_CONFIRM_BTN_NO] \
		-apply -applyCallback SaveConfirmNoCB \
		-cancelLabel [IntlMsg SAVE_CONFIRM_BTN_CANCEL] \
		-cancel -cancelCallback SaveConfirmCancelCB \
		-wmCloseCallback SaveConfirmCancelCB \
		-autoLock [list \
			SaveConfirmOkCB SaveConfirmNoCB SaveConfirmCancelCB]]
	VxSetVar $parent saveConfirm $saveConfirmDlg
	return $saveConfirmDlg
}
				

#-------------------------------------------------------------------------------
# UI CallBacks
#-------------------------------------------------------------------------------

#====================================================================== XXX ===
# UiCloseCB
#   
# Generally, the exit callback including Ui shutdown. 
# Two callbacks into the underlying application module, 1 before shutting
# the Ui (e.g. so that message/error/question dialogs are still possible,
# and 1 after Ui has been shutdown.
#------------------------------------------------------------------------------
proc UiCloseCB {cbs} {
	# Call underlying non-ui module: close that may need additional ui
	if {[DomainIsModified]} {
		CloseWithUi
		VtUnLock
		return
	}
	UiStop	UiCloseCB2
}

#====================================================================== XXX ===
# UiCloseCB2
#   
# Part 2 (see CloseCB) 
# Regained control from UiStop which may have required a detour to
# the event-driven loop to present an error dialog. We now resume
# procedural control and call back out of the Ui module.
#------------------------------------------------------------------------------
proc UiCloseCB2 {{cbs {}}} {
	# Call underlying non-ui module: close without Ui
	CloseAfterUi
}

#====================================================================== XXX ===
# UiRefreshCB
#   
# Dynamic refresh of data
# Call to non-ui refresh function surrounded by code to:
# 1) indicate action on statusbar
# 2) do the work to save/restore selected items that persist across refresh
#------------------------------------------------------------------------------
proc UiRefreshCB {{cbs {}}} {
	global appvals

	set statusBar [VxGetVar $appvals(vtMain) statusBar]
	set mainList [VxGetVar $appvals(vtMain) mainList]

	SaStatusBarSet $statusBar [IntlMsg REFRESH] 

	Refresh

	# Update count label and widget sensitivity
	UiUpdateCountLabel
	UiSensitizeMainFunctions

	SaStatusBarClear $statusBar

	VtUnLock
}

proc EnterHostNameCB {cbs} {
	set dialog [keylget cbs dialog]
	set widget [keylget cbs widget]
	set mode [keylget cbs mode]
	set name [keylget cbs value]

	if {$mode == "done"} {
		if {[lempty $name]} {
			VtShow [VtErrorDialog $dialog.usererr1 \
				-message [IntlErr ENTER_NAME] \
				-ok]
			set hostNameText [VxGetVar $dialog hostNameText]
			VtSetFocus $hostNameText
			VtUnLock
			return
		} else {
			set okButton [VtGetValues $dialog -ok]
			VtSetFocus $okButton
		}
	}
	VtUnLock
}

proc SelectHostOkCB {addToRouteScreen cbs} {
	global appvals
	set dialog [keylget cbs dialog]
	set instance $appvals(selecthost)
	set selection [SaSelectHostGetSelection $instance]
	set hostNameText [VxGetVar $addToRouteScreen hostNameText]
	set currentVal [VtGetValues $hostNameText -value]
	lappend currentVal $selection
	VtSetValues $hostNameText -value $currentVal
	VtSetValues $hostNameText \
		-xmArgs [list XmNcursorPosition [string length $currentVal]]
	VtSetFocus $hostNameText
	VtUnLock
}

proc SelectHostCB {cbs} {
	global appvals
	set parent [keylget cbs dialog]

	keylset parameters parent $parent
	keylset parameters instance selectHostScreen
	set appvals(selecthost) selectHostScreen
	
	keylset parameters userproc "SelectHostOkCB $parent" 
	SaSelectHostDialog $parameters $cbs
}

proc AddToRouteOkCB {cbs} {
	set dialog [keylget cbs dialog]
	set hostNameText [VxGetVar $dialog hostNameText]
	set route [VtGetValues $hostNameText -value]

	if {[lempty $route]} {
		VtShow [VtErrorDialog $dialog.usererr1 \
			-message [IntlErr ENTER_NAME] \
			-ok]
		VtSetFocus $hostNameText
		VtUnLock
		return
	}

	set addModifyScreen [VxGetVar $dialog parent_dlg]
	set routeList [VxGetVar $addModifyScreen routeList]
	set currentItems [VtDrawnListGetItem $routeList -all]
	set currentSelectedItems [VtDrawnListGetSelectedItem $routeList \
					-byPositionList]
	VtDestroyDialog $dialog

	if {[lempty $currentItems]} {
		UiRouteListSetItems $route
		VtDrawnListSelectItem $routeList -all
	} else {
		if {$currentSelectedItems == 0} {
			UiRouteListSetItems [concat $currentItems $route]
			set pos [llength $currentItems]
			set positions {}
			foreach hop $route {
				incr pos
				lappend positions $pos
			}
			VtDrawnListSelectItem \
				$routeList -positionList $positions
		} else {
			set pos [lindex $currentSelectedItems end]
			set firstPositions {}
			for {set index 1} {$index <= $pos} {incr index} {
				lappend firstPositions $index
			}
			set lastPositions {}
			for {set index [expr $pos + 1]} \
			    {$index <= [llength $currentItems]} {incr index} {
				lappend lastPositions $index
			}
			set lastNewPosition [expr $pos + [llength $route]]
			set newPositions {}
			for {set index [expr $pos + 1]} \
			    {$index <= $lastNewPosition} {incr index} {
				lappend newPositions $index
			}

			set begItems [VtDrawnListGetItem $routeList \
					-positionList $firstPositions]
			set lastItems [VtDrawnListGetItem $routeList \
					-positionList $lastPositions]
			UiRouteListSetItems [concat $begItems $route $lastItems]
			VtDrawnListSelectItem \
				$routeList -positionList $newPositions
		}
	}
	SaDrawnListShowSelectedItem $routeList
	UiSensitizeAddModifyFunctions $addModifyScreen

	VtUnLock
}

proc AddToRouteCancelCB {cbs} {
	set dialog [keylget cbs dialog]
	VtDestroyDialog $dialog
	VtUnLock
	return
}

proc AddToRouteCB {cbs} {
	set parent [keylget cbs dialog]
	set mode [keylget cbs mode]
	set name [keylget cbs value]

	set addToRouteScreen [UiBuildAddToRouteScreen $parent]
	VxSetVar $addToRouteScreen parent_dlg $parent
	set hostNameText [VxGetVar $addToRouteScreen hostNameText]
	
	VtShow $addToRouteScreen
	VtSetFocus $hostNameText
	VtUnLock
}

proc DeleteFromRouteCB {cbs} {
	set dialog [keylget cbs dialog]
	set routeList [VxGetVar $dialog routeList]
	set selectedPositions [VtDrawnListGetSelectedItem \
				$routeList -byPositionList]
	set prevTotal [llength [VtDrawnListGetItem $routeList -all]]

	VtDrawnListDeleteItem $routeList -positionList $selectedPositions

	set numItemsLeft [llength [VtDrawnListGetItem $routeList -all]]
	if {$numItemsLeft > 0} {
		set first [lindex $selectedPositions 0]
		set last [lindex $selectedPositions end]
		set numDeleted [llength $selectedPositions]
		if {[expr $last - $first + 1] == $numDeleted} {
			set contiguous 1
		} else {
			set contiguous 0
		}

		if {$last == $prevTotal && $contiguous} {
			# select the one above
			 VtDrawnListSelectItem $routeList \
				-position [expr $first - 1]
		} else {
			# select the item that was after the first deleted
			# (same position number as first deleted was)
			VtDrawnListSelectItem $routeList -position $first
		}
		SaDrawnListShowSelectedItem $routeList
	}
	UiSensitizeAddModifyFunctions $dialog
	VtUnLock
}

proc RouteListCB {cbs} {
	set dialog [keylget cbs dialog]
	UiSensitizeAddModifyFunctions $dialog
}

proc MoveUpRouteCB {cbs} {
	set dialog [keylget cbs dialog]
	set routeList [VxGetVar $dialog routeList]
	set selectedNum [lindex \
		[VtDrawnListGetSelectedItem $routeList -byPositionList] 0]
	if {$selectedNum == 1} {
		VtBeep
		VtUnLock
		return
	}
	set selectedItem [VtDrawnListGetItem $routeList -position $selectedNum]
	set newPosition [expr $selectedNum - 1]
	VtDrawnListDeleteItem $routeList -position $selectedNum
	VtDrawnListAddItem $routeList -position $newPosition \
				      -formatList [list [list STRING 25]] \
				      -fieldList $selectedItem
	VtDrawnListSelectItem $routeList -position $newPosition
	SaDrawnListShowSelectedItem $routeList
	VtUnLock
	return
}
	
proc MoveDownRouteCB {cbs} {
	set dialog [keylget cbs dialog]
	set routeList [VxGetVar $dialog routeList]
	set selectedNum [lindex \
		[VtDrawnListGetSelectedItem $routeList -byPositionList] 0]
	set totalNum [llength [VtDrawnListGetItem $routeList -all]]
	if {$selectedNum == $totalNum} {
		VtBeep
		VtUnLock
		return
	}
	set selectedItem [VtDrawnListGetItem $routeList -position $selectedNum]
	set newPosition [expr $selectedNum + 1]
	VtDrawnListDeleteItem $routeList -position $selectedNum
	VtDrawnListAddItem $routeList -position $newPosition \
				      -formatList [list [list STRING 25]] \
				      -fieldList [list $selectedItem]
	VtDrawnListSelectItem $routeList -position $newPosition
	SaDrawnListShowSelectedItem $routeList
	VtUnLock
	return
}

proc AddModifyCancelCB {cbs} {
	set dialog [keylget cbs dialog]
	VtDestroyDialog $dialog
	VtUnLock
	return
}

proc AddModifyOkCB {cbs} {
	global appvals

	set dialog [keylget cbs dialog]
	set function [VxGetVar $dialog function]
	set nameText [VxGetVar $dialog nameText]
	set realNameText [VxGetVar $dialog realNameText]
	set routeList [VxGetVar $dialog routeList]
	set doPartial [VxGetVar $dialog doPartial]
	
	# get the values entered into the widgets
	set newHostName [string trim [VtGetValues $nameText -value]]
	set newRealName [string trim [VtGetValues $realNameText -value]]
	set newRoute [VtDrawnListGetItem $routeList -all]
	set newPartial [VtGetValues $doPartial -value]
	
	if {[clength $newHostName] == 0 || [ctype space $newHostName]} {
		VtShow [VtErrorDialog $dialog.usererr1 \
			-message [IntlErr NAME_MISSING] \
			-ok]
		VtUnLock
		VtSetFocus $nameText
		return
	}

	if {[regexp {[ 	]} $newHostName]} { 
		VtShow [VtErrorDialog $dialog.usererr2 \
			-message [IntlErr NAME_SPACE] \
			-ok]
		VtUnLock
		VtSetFocus $nameText
		return
	}

	if {[clength $newRealName] == 0 || [ctype space $newRealName]} {
		VtShow [VtErrorDialog $dialog.usererr3 \
			-message [IntlErr REAL_NAME_MISSING] \
			-ok]
		VtUnLock
		VtSetFocus $realNameText
		return
	}
	
	if {[regexp {[ 	]} $newRealName]} { 
		VtShow [VtErrorDialog $dialog.usererr4 \
			-message [IntlErr NAME_SPACE] \
			-ok]
		VtUnLock
		VtSetFocus $realNameText
		return
	}

	# Don't allow partial lookups on numeric addresses
	if {[regexp {\[[0-9\.]+\]} $newHostName] && $newPartial} {
		VtShow [VtErrorDialog $dialog.usererr5 \
			-message [IntlErr NUMERIC] \
			-ok]
		VtUnLock
		VtSetFocus $doPartial
		return
	}
		
	# set up the new entry
	if {$newPartial} {
		set newPartial TRUE
		keylset newEntry partial $newPartial
	} else {
		set newPartial FALSE
		keylset newEntry partial $newPartial
	}
	keylset newEntry hostName $newHostName
	keylset newEntry realName $newRealName
	keylset newEntry route $newRoute

	set mainList [VxGetVar $appvals(vtMain) mainList]
	set selected [VtDrawnListGetSelectedItem $mainList -byRecordList]

	if {$function == "add"} {
		if {[DomainEntryExists $newHostName]} {
			VtShow [VtErrorDialog $dialog.usererr5 \
				-message [IntlErr NAME_DUPLICATE $newHostName] \
				-ok]
			VtUnLock
			VtSetFocus $nameText
			return
		}
			
		if {[lempty $selected]} {
			# just put the new entry last
			set position 0
			set before [lindex [VtDrawnListGetItem $mainList \
						-position 0] 0]
		} else {
			# put the new entry after the last selected entry
			set selectedNum [lindex [VtDrawnListGetSelectedItem \
						$mainList -byPositionList] end]
			set position [expr $selectedNum + 1] 
			set before [lindex $selected end]
		}
		
		if {[cequal $before ""]} {
			set beforeEntry ""
		} else {
			set beforeEntry [lindex $before end]
		}
			
		# update the data structures
		DomainAdd $beforeEntry $newEntry
		
	} else {
		set position [lindex [VtDrawnListGetSelectedItem $mainList \
					-byPositionList] 0]
		set oldEntry [lindex [lindex $selected 0] end]

		set oldPartial ""
		keylget oldEntry partial oldPartial 
		set oldHostName ""
		keylget oldEntry hostName oldHostName 
		set oldRealName ""
		keylget oldEntry realName oldRealName 
		set oldRoute ""
		keylget oldEntry route oldRoute 

		if {![cequal [string tolower $newHostName] \
			     [string tolower $oldHostName]] && \
		    [DomainEntryExists $newHostName]} {
			VtShow [VtErrorDialog $dialog.usererr6 \
				-message [IntlErr NAME_DUPLICATE $newHostName] \
				-ok]
			VtUnLock
			VtSetFocus $nameText
			return
		}

		set modified 0
		if {$newPartial != $oldPartial || \
		    $newHostName != $oldHostName || \
		    $newRealName != $oldRealName || \
		    $newRoute != $oldRoute} {
			DomainModify $oldEntry $newEntry
			set modified 1
		}
		
	}

	VtDestroyDialog $dialog

	# update the main drawn list
	if {$function == "add" || ($function == "modify" && $modified)} {
		set maxLength [VxGetVar $mainList maxLength]
		set newLength [clength $newHostName]
		if {$newLength < $maxLength} {
			set newLength $maxLength
		}
		# For now, we punt on re-drawing the list if the new
		# host length is larger than the known max length --
		# this is an expensive operation.
		set isCharm [VtInfo -charm]
		if {$isCharm} {
			set formatList [list \
				[list STRING 1 1 1] \
				[list STRING $newLength 1 1] ]
		} else {
			set formatList [list \
				[list STRING 1 10 10] \
				[list STRING [AdjustFieldLength $newLength] 0 10]]
		}
		if {$newPartial == "TRUE"} {
			if {[VtInfo -charm]} {
				set fieldList [list \
					[IntlMsg COLUMN_PARTIAL_LBL_CHARM]]
			} else {
				set fieldList [list \
					[IntlMsg COLUMN_PARTIAL_LBL]]
			}
		} else {
			set fieldList \
				[list [IntlMsg COLUMN_PARTIAL_NONE]]
		}

		lappend fieldList $newHostName

		set routeString ""
		foreach hop $newRoute {
			append routeString "$hop > "
		}
		append routeString $newRealName
		lappend fieldList $routeString
		lappend formatList [list \
				STRING \
				[AdjustFieldLength [clength $routeString]]]

		lappend fieldList $newEntry
		lappend formatList [list DATA]

		if {$function == "modify"} {
			VtDrawnListDeleteItem $mainList -position $position
		}
		VtDrawnListAddItem $mainList -formatList $formatList \
					     -fieldList $fieldList \
					     -position $position
		VtDrawnListSelectItem $mainList -position $position
		SaDrawnListShowSelectedItem $mainList
		UiSensitizeMainFunctions
		if {"$function" == "add"} {
			UiUpdateCountLabel
		}
	}

	VtUnLock
	return
}

#====================================================================== XXX ===
# UiAddEntryCB
#   
#------------------------------------------------------------------------------
proc UiAddEntryCB {cbs} {

	set addScreen [UiBuildAddModifyScreen]
	VxSetVar $addScreen function add

	set nameText [VxGetVar $addScreen nameText]
	set addButton [VxGetVar $addScreen routeAddButton]
	set deleteButton [VxGetVar $addScreen routeDeleteButton]
	set upButton [VxGetVar $addScreen routeMoveUpButton]
	set downButton [VxGetVar $addScreen routeMoveDownButton]

	VtSetValues $addScreen -title [IntlMsg ADD_TITLE]
	VtSetSensitive $addButton 1
	VtSetSensitive $deleteButton 0
	VtSetSensitive $upButton 0
	VtSetSensitive $downButton 0

	VtShow $addScreen
	VtSetFocus $nameText
	VtUnLock
}

#====================================================================== XXX ===
# UiDeleteEntryCB
#   
#------------------------------------------------------------------------------
proc UiDeleteEntryCB {cbs} {
	global appvals

	set list [VxGetVar $appvals(vtMain) mainList]
	set selectedPositions [VtDrawnListGetSelectedItem $list -byPositionList]
	set prevTotal [llength [VtDrawnListGetItem $list -all]]

	set deleteEntries {}
	foreach item [VtDrawnListGetSelectedItem $list -byRecordList] {
		lappend deleteEntries [lindex $item end]
	}
	DomainDelete $deleteEntries
	VtDrawnListDeleteItem $list -positionList $selectedPositions

	set numItemsLeft [llength [VtDrawnListGetItem $list -all]]
	if {$numItemsLeft > 0} {
		set first [lindex $selectedPositions 0]
		set last [lindex $selectedPositions end]
		set numDeleted [llength $selectedPositions]
		if {[expr $last - $first + 1] == $numDeleted} {
			set contiguous 1
		} else {
			set contiguous 0
		}
			
		if {$last == $prevTotal && $contiguous} {
			# select the one above
			VtDrawnListSelectItem $list -position [expr $first - 1]
		} else {
			# select the item that was after the first deleted
			# (same position number as first deleted was)
			VtDrawnListSelectItem $list -position $first
		}
		SaDrawnListShowSelectedItem $list
	}
	UiSensitizeMainFunctions
	UiUpdateCountLabel
	VtUnLock
}

#====================================================================== XXX ===
# UiModifyEntryCB
#   
#------------------------------------------------------------------------------
proc UiModifyEntryCB {cbs} {
	set dialog [keylget cbs dialog] 
	set mainList [VxGetVar $dialog mainList]
	set selected [lindex \
		[VtDrawnListGetSelectedItem $mainList -byItemList] 0]

	set modifyScreen [UiBuildAddModifyScreen]
	VxSetVar $modifyScreen function modify

	set nameText [VxGetVar $modifyScreen nameText]
	set realNameText [VxGetVar $modifyScreen realNameText]
	set routeList [VxGetVar $modifyScreen routeList]
	set doPartial [VxGetVar $modifyScreen doPartial]
	set addButton [VxGetVar $modifyScreen routeAddButton]
	set deleteButton [VxGetVar $modifyScreen routeDeleteButton]
	set upButton [VxGetVar $modifyScreen routeMoveUpButton]
	set downButton [VxGetVar $modifyScreen routeMoveDownButton]

	set modifyEntry [lindex $selected end]
	set partial ""
	keylget modifyEntry partial partial
	set hostName ""
	keylget modifyEntry hostName hostName
	set realName ""
	keylget modifyEntry realName realName
	set route ""
	keylget modifyEntry route route

	VtSetValues $modifyScreen -title [IntlMsg MODIFY_TITLE]
	VtSetValues $doPartial -set $partial
	VtSetValues $nameText -value $hostName
	VtSetValues $realNameText -value $realName
	foreach hop $route {
		VtDrawnListAddItem $routeList  \
			-fieldList [list $hop] \
			-formatList [list [list STRING 25]]
	}
	
	if {[lempty $route]} {
		VtSetSensitive $addButton 1
		VtSetSensitive $deleteButton 0
		VtSetSensitive $upButton 0
		VtSetSensitive $downButton 0
	} else {
		VtSetSensitive $addButton 1
		VtSetSensitive $deleteButton 1
		VtSetSensitive $upButton 1
		VtSetSensitive $downButton 1
		VtDrawnListSelectItem $routeList -position 1
	}

	VtShow $modifyScreen
	VtSetFocus $nameText
	VtUnLock
}

#====================================================================== XXX ===
# UiMoveUpEntryCB
#   
#------------------------------------------------------------------------------
proc UiMoveUpEntryCB {cbs} {
	global appvals

	set list [VxGetVar $appvals(vtMain) mainList]
	set selectedNum [lindex \
		[VtDrawnListGetSelectedItem $list -byPositionList] 0]
	if {$selectedNum == 1} {
		VtBeep
		VtUnLock
		return
	}
	set selectedItem [lindex \
		[VtDrawnListGetItem $list -position $selectedNum] 0]
	set newPosition [expr $selectedNum - 1]
	set beforeItem [lindex \
		[VtDrawnListGetItem $list -position $newPosition] 0]

	#
	# Update table data structures
	#
	set entry1 [lindex $beforeItem end]
	set entry2 [lindex $selectedItem end]

	DomainSwapPosition $entry1 $entry2

	#
	# Modify main list widget
	#

	# Delete the item selected in the list
	VtDrawnListDeleteItem $list -position $selectedNum

	# Add the deleted item back in the new position
	set maxLength [VxGetVar $list maxLength]	
	if {[VtInfo -charm]} {
		set formatList [list \
			[list STRING 1 1 1] \
			[list STRING $maxLength 1 1]]
		set icon [list ICON 1 1 1]
	} else {
		set formatList [list \
			[list STRING 1 10 10] \
			[list STRING [AdjustFieldLength $maxLength] 0 10]]
		set icon [list ICON 1 0 5]
	}

	lappend formatList [list STRING [clength [lindex $selectedItem 2]]]

	VtDrawnListAddItem $list -position $newPosition \
		-fieldList $selectedItem \
		-formatList $formatList

	VtDrawnListSelectItem $list -position $newPosition
	SaDrawnListShowSelectedItem $list
	VtUnLock
}

#====================================================================== XXX ===
# UiMoveDownEntryCB
#   
#------------------------------------------------------------------------------
proc UiMoveDownEntryCB {cbs} {
	global appvals

	set list [VxGetVar $appvals(vtMain) mainList]
	set selectedNum [lindex \
		[VtDrawnListGetSelectedItem $list -byPositionList] 0]
	set numItems [llength [VtDrawnListGetItem $list -all]]
	if {$selectedNum == $numItems} {
		VtBeep
		VtUnLock
		return
	}
	set selectedItem [lindex \
		[VtDrawnListGetItem $list -position $selectedNum] 0]
	set newPosition [expr $selectedNum + 1]
	set afterItem [lindex \
		[VtDrawnListGetItem $list -position $newPosition] 0]

	#
	# Update table data structures
	#
	set entry1 [lindex $selectedItem end]
	set entry2 [lindex $afterItem end]

	DomainSwapPosition $entry1 $entry2

	#
	# Modify main list widget
	#

	# Delete the item selected in the list
	VtDrawnListDeleteItem $list -position $selectedNum

	# Add the deleted item back in the new position
	set maxLength [VxGetVar $list maxLength]
	if {[VtInfo -charm]} {
		set formatList [list \
			[list STRING 1 1 1] \
			[list STRING $maxLength 1 1]]
		set icon [list ICON 1 1 1]
	} else {
		set formatList [list \
			[list STRING 1 10 10] \
			[list STRING [AdjustFieldLength $maxLength] 0 10]]
		set icon [list ICON 1 0 5]
	}

	lappend formatList [list STRING [clength [lindex $selectedItem 2]]]

	VtDrawnListAddItem $list -position $newPosition \
		-fieldList $selectedItem \
		-formatList $formatList

	VtDrawnListSelectItem $list -position $newPosition
	SaDrawnListShowSelectedItem $list
	VtUnLock
}

proc SaveConfirmOkCB {cbs} {
	set dialog [keylget cbs dialog]
	VtDestroyDialog $dialog
	if {[UiSaveCB {}] != 1} {
		VtUnLock
		return
	} else {
		VtUnLock
		UiStop  UiCloseCB2
	}
}

proc SaveConfirmNoCB {cbs} {
	set dialog [keylget cbs dialog]
	VtDestroyDialog $dialog
	VtUnLock
	UiStop  UiCloseCB2
}
	
proc SaveConfirmCancelCB {cbs} {
	set dialog [keylget cbs dialog]
	VtDestroyDialog $dialog
	VtUnLock
	return
}

#====================================================================== XXX ===
# UiSaveCB
#   
#------------------------------------------------------------------------------
proc UiSaveCB {cbs} {
	global appvals

	set statusBar [VxGetVar $appvals(vtMain) statusBar]
	SaStatusBarSet $statusBar [IntlMsg SAVE]
	if {[ErrorCatch errStack 0 DomainWrite data]} {
		VtUnLock
		UiDisplayErrorStacks UiSaveCB $errStack
		return 0
	}
	SaStatusBarClear $statusBar
	VtUnLock
	return 1
}

#====================================================================== XXX ===
# UiSensitizeMainFunctions
#   
# Set the sensitivity of all pulldown menu items 
# Called before initial main screen is unlocked, and any time an action
# changes state such that item sensitivity must be recomputed globally
#------------------------------------------------------------------------------
proc UiSensitizeMainFunctions {} {
	# visit all appropriate menu items:
	# 	VxGetVar
	#	VtSetsensitive
	global appvals

	set mainList [VxGetVar $appvals(vtMain) mainList]
	set item [VtDrawnListGetSelectedItem $mainList]
	set selected [llength \
		[VtDrawnListGetSelectedItem $mainList -byRecordList]]

	set menuBar [VxGetVar $appvals(vtMain) menuBar]
	set add [VxGetVar $menuBar add]
	set delete [VxGetVar $menuBar delete]
	set modify [VxGetVar $menuBar modify]
	set moveUp [VxGetVar $menuBar moveUp]
	set moveDown [VxGetVar $menuBar moveDown]

	set isCharm [VtInfo -charm]

	if {!$isCharm} {
		set toolBar [VxGetVar $appvals(vtMain) toolBar]
		set addTB [VxGetVar $toolBar addTB]
		set deleteTB [VxGetVar $toolBar deleteTB]
		set modifyTB [VxGetVar $toolBar modifyTB]
		set upTB [VxGetVar $toolBar upTB]
		set downTB [VxGetVar $toolBar downTB]
	}

	if {$selected == 0} {
		VtSetSensitive $delete 0
		VtSetSensitive $modify 0
		VtSetSensitive $moveUp 0
		VtSetSensitive $moveDown 0
		if {!$isCharm} {
			VtSetSensitive $deleteTB 0
			VtSetSensitive $modifyTB 0
			VtSetSensitive $upTB 0
			VtSetSensitive $downTB 0
		}
	} elseif {$selected == 1} {
		VtSetSensitive $delete 1
		VtSetSensitive $modify 1
		VtSetSensitive $moveUp 1
		VtSetSensitive $moveDown 1
		if {!$isCharm} {
			VtSetSensitive $deleteTB 1
			VtSetSensitive $modifyTB 1
			VtSetSensitive $upTB 1
			VtSetSensitive $downTB 1
		}
	} else {
		VtSetSensitive $delete 1
		VtSetSensitive $modify 0
		VtSetSensitive $moveUp 0
		VtSetSensitive $moveDown 0
		if {!$isCharm} {
			VtSetSensitive $deleteTB 1
			VtSetSensitive $modifyTB 0
			VtSetSensitive $upTB 0
			VtSetSensitive $downTB 0
		}
	}
}

proc UiSensitizeAddModifyFunctions {addModifyScreen} {
	set deleteButton [VxGetVar $addModifyScreen routeDeleteButton]
	set upButton [VxGetVar $addModifyScreen routeMoveUpButton]
	set downButton [VxGetVar $addModifyScreen routeMoveDownButton]
	set routeList [VxGetVar $addModifyScreen routeList]
	set selected [llength \
		[VtDrawnListGetSelectedItem $routeList -byRecordList]]
	if {$selected == 0} {
		VtSetSensitive $deleteButton 0
		VtSetSensitive $upButton 0
		VtSetSensitive $downButton 0
	} elseif {$selected == 1} {
		VtSetSensitive $deleteButton 1
		VtSetSensitive $upButton 1
		VtSetSensitive $downButton 1
	} else {
		VtSetSensitive $deleteButton 1
		VtSetSensitive $upButton 0
		VtSetSensitive $downButton 0
	}
}
	

#-------------------------------------------------------------------------------
# Basic UI Entry Points: start, stop, loop
#-------------------------------------------------------------------------------

#====================================================================== XXX ===
# UiStart
#   
# Build/Startup the Ui:
# o build main form
# o pulldown menus
# o main form widgets
# o statusBar
# Main form is presented in a locked state and control returns to the 
# underlying application for additional startup tasks. Control eventually
# returns to Ui loop for user events via VtMainLoop
#------------------------------------------------------------------------------
proc UiStart {} {
	global appvals


	# Open/connect to widget server
	set vtApp [VtOpen $appvals(client) [IntlMsg HELPBOOK]]
	set appvals(vtApp) $vtApp
	VtSetAppValues $vtApp \
                -errorCallback {SaUnexpectedErrorCB {}}

	# Create main dialog
	set vtMain [VtFormDialog $vtApp.main \
			-title $appvals(title) \
			-resizable FALSE \
			-wmShadowThickness 0 \
			-wmCloseCallback UiCloseCB] 
	set appvals(vtMain) $vtMain
	VxSetVar $appvals(vtMain) vtMain $vtMain
	UiDefaultFormSpacingSet $vtMain
	SaCharmSetMaxFormDimensions $vtMain

	# Build pulldown menus
	set menuBar [UiBuildMenus $vtMain]
	VxSetVar $appvals(vtMain) menuBar $menuBar

	# Build the toolbar
	if {! [VtInfo -charm]} {
		set toolBar [UiBuildToolBar $vtMain $menuBar]
	}

	# Basic drawnlist for domain table entries
	# Pass main form and widget just above for attachments
	if {[VtInfo -charm]} {
		set mainList [UiBuildMainList $vtMain $menuBar]
	} else {
		set mainList [UiBuildMainList $vtMain $toolBar]
	}

	# Build the status bar with standard SCOadmin status bar pixmap
	set statusBar [SaStatusBar $vtMain.statusBar 1]
	VxSetVar $appvals(vtMain) statusBar $statusBar

	# Connect main screen widget bottom to statusbar
	VtSetValues $mainList -bottomSide $statusBar

	# Display the main form and lock it
	VtShow $vtMain
	VtLock
}

#====================================================================== XXX ===
# UiStop
#   
# Shut down the Ui. We may have to interrupt the procedural flow
# from the caller to present error dialogs to the user. This requires
# returning to the event-driven Ui loop. To return control to the caller,
# UiStop utilizes UiStop2 (e.g. part 2) and a callback to return control
# to the caller.
#------------------------------------------------------------------------------
proc UiStop {cb} {
	global appvals

	set vtMain $appvals(vtMain)
	set client $appvals(client)

	# Store modified preferences

	# short help
	set cmd [list SaShortHelpStore $client]
	if {[ErrorCatch errorStack 0 $cmd dummy] != 0 } {
		lappend errorStacks $errorStack
	}

	# any problems?
	if [info exists errorStacks] {
		# present error dialog and enter event-driven Ui loop.
		# Re-gain procedural control via UiStop2 (part 2) and
		# eventuall, the caller's cb
		SaDisplayErrorStacks $vtMain.UIStop $errorStacks {} \
			[list UiStop2 $cb]
		VtUnLock
		VtMainLoop
	} else {
		# stop vtcl
		VtUnLock
		VtClose
		# return control to the caller
		eval $cb
	}
}

#====================================================================== XXX ===
# UiStop2
#   
# Part 2 (see UiStop).
# Shutdown the UI for sure: no more possible dialogs, no more event-driven
# control flow. Shutdown and return control to caller of UiStop.
#------------------------------------------------------------------------------
proc UiStop2 {cb {cbs {}}} {
	VtUnLock
	VtClose
	# return control to the caller
	eval $cb
}
