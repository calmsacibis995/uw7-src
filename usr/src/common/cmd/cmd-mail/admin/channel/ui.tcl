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
# SCO Mail Administration Channel client:
# 	User Interface Module(s): UI. Binding to vtcl, the widget server
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
# Because of variable character widths in graphical mode, we try to
# compensate by multiplying character length by this number
set appvals(fieldAdjustment)	0.90

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
	ChannelSetMainListLabel $maxLength

	set isCharm [VtInfo -charm]
	set CHARM_formatList [list \
		[list STRING $maxLength 0 2]]
	set MOTIF_formatList [list \
		[list STRING [AdjustFieldLength $maxLength] 0 10]]
	foreach entry $items {
		keylget entry hostName hostName
		keylget entry realName realName

		if {$isCharm} {
			set formatList $CHARM_formatList
		} else {
			set formatList $MOTIF_formatList
		}

		lappend formatList [list STRING [clength $realName]]
		set fieldList [list $hostName $realName]

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

	set mainList [ChannelBuildMainList $form $top]

	VxSetVar $form mainList $mainList

	# Define the accompanying count label
	set countLabel [VtLabel $form.countLabel -label " " \
				-topSide NONE \
				-leftSide FORM \
				-rightSide FORM \
				-bottomSide FORM \
				-labelRight]
	VxSetVar $form countLabel $countLabel
	VtSetValues $mainList -bottomSide $countLabel

	# Stash the localized string "Selected" here so we only grab it once
	# from the message catalog
	VxSetVar $countLabel selected [IntlMsg SELECTED]
	
	# Return the bottom-most widget for further attachments
	return $countLabel
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
        set labelValue [IntlMsg COUNT_LABEL_STRING $labelArgs]
	VtSetValues $countLabel -label $labelValue
}

#------------------------------------------------------------------------------
# Add/Modify Functions:
#------------------------------------------------------------------------------

proc UiBuildAddModifyScreen {function} {
	global appvals
	
	set parent $appvals(vtMain)
	set addModifyScreen [VtFormDialog $parent.addModifyScreen \
		-ok -okLabel [IntlMsg OK] \
		-cancel -cancelLabel [IntlMsg CANCEL] \
		-help -helpLabel [IntlMsg HELP] \
		-cancelButton CANCEL \
		-okCallback AddModifyOkCB \
		-cancelCallback AddModifyCancelCB \
		-wmCloseCallback AddModifyCancelCB \
		-autoLock [list AddModifyOkCB AddModifyCancelCB] ]
	VxSetVar $parent addModifyScreen $addModifyScreen

	set nameForm [VtForm $addModifyScreen.nameForm \
		-marginHeight 0 -marginWidth 0 \
		-leftSide FORM -MOTIF_leftOffset 10 -CHARM_leftOffset 1 \
		-rightSide FORM -MOTIF_rightOffset 10 -CHARM_rightOffset 1 \
		-topSide FORM -topOffset 10 -CHARM_topOffset 1]
	set nameLabel [VtLabel $nameForm.nameLabel \
		-label [IntlMsg NAME_LBL] \
		-labelLeft ]
	set selectNameButton [VtPushButton $nameForm.selectNameButton \
		-label [IntlMsg SELECT_BTN_LBL]\
		-leftSide NONE \
		-rightSide FORM \
		-bottomSide FORM \
		-callback SelectHostCB \
		-autoLock SelectHostCB]
	set nameText [VtText $nameForm.nameText \
		-columns 30  -CHARM_columns 35 \
		-leftSide FORM \
		-rightSide $selectNameButton \
		-alignTop $selectNameButton \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString [IntlMsg NAME_SH] \
		-callback {SaSetFocus next}]
	VxSetVar $addModifyScreen nameText $nameText
	VxSetVar $addModifyScreen selectNameButton $selectNameButton


	set realNameForm [VtForm $addModifyScreen.realNameForm \
		-marginHeight 0 -marginWidth 0 \
		-leftSide FORM -MOTIF_leftOffset 10 -CHARM_leftOffset 1 \
		-rightSide FORM -MOTIF_rightOffset 10 -CHARM_rightOffset 1 \
		-topSide $nameForm -MOTIF_topOffset 10 -CHARM_topOffset 1]
	set realNameLabel [VtLabel $realNameForm.realNameLabel \
		-label [IntlMsg REAL_NAME_LBL] \
		-labelLeft ]
	set selectRealNameButton [VtPushButton \
					$realNameForm.selectRealNameButton \
		-label [IntlMsg SELECT_BTN_LBL]\
		-leftSide NONE \
		-rightSide FORM \
		-bottomSide FORM \
		-callback SelectHostCB \
		-autoLock SelectHostCB]
	set realNameText [VtText $realNameForm.realNameText \
		-columns 30 -CHARM_columns 35 \
		-leftSide FORM \
		-rightSide $selectRealNameButton \
		-alignTop $selectRealNameButton \
		-shortHelpCallback SaShortHelpCB \
		-shortHelpString [IntlMsg REAL_NAME_SH] \
		-callback {SaSetFocus next}]
	VxSetVar $addModifyScreen realNameText $realNameText
	VxSetVar $addModifyScreen selectRealNameButton $selectRealNameButton

	set separator [VtSeparator $addModifyScreen.separator \
		-leftSide FORM -leftOffset 0 \
		-rightSide FORM -rightOffset 0 \
		-topSide $realNameForm -MOTIF_topOffset 10 -CHARM_topOffset 1]
	set statusBar [SaStatusBar $addModifyScreen.statusbar 0]
	VtSetValues $separator \
		-bottomSide $statusBar -bottomOffset 0

	if {$function == "add"} {
		VxSetVar $addModifyScreen function add
        	VtSetValues $addModifyScreen -title [IntlMsg ADD_TITLE]
	} else {
		VxSetVar $addModifyScreen function modify
        	VtSetValues $addModifyScreen -title [IntlMsg MODIFY_TITLE]
	}
		
	SaSetTabGroups $addModifyScreen [list \
		$nameText \
		$selectNameButton \
		$realNameText \
		$selectRealNameButton]

	set okButton [VtGetValues $addModifyScreen -ok]
	SaSetFocusList $addModifyScreen [list \
		$nameText \
		$realNameText \
		$okButton]

        VtShow $addModifyScreen
	VtSetFocus $nameText
	return $addModifyScreen
}

proc UiBuildSaveConfirmDialog {} {
	global appvals

	set parent $appvals(vtMain)
	# set autoDestroy FALSE, since we want to do cancel
	# on a window manager close, and autoDestroy is not
	# possible in that case
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
	if {[ChannelIsModified]} {
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

proc SelectHostOkCB {button cbs} {
        global appvals

        set addModifyScreen [VxGetVar $appvals(vtMain) addModifyScreen]

	set selectNameButton \
		[VxGetVar $addModifyScreen selectNameButton]
	set selectRealNameButton \
		[VxGetVar $addModifyScreen selectRealNameButton]

	set nameText [VxGetVar $addModifyScreen nameText]
	set realNameText [VxGetVar $addModifyScreen realNameText]

	set instance $appvals(selecthost)
	set selection [SaSelectHostGetSelection $instance]

	if {[cequal $button $selectNameButton]} {
		VtSetValues $nameText -value $selection
		VtSetFocus $nameText
	} else {
		VtSetValues $realNameText -value $selection
		VtSetFocus $realNameText
	}
	VtUnLock
}

proc SelectHostCB {cbs} {
	global appvals

	set parent [keylget cbs dialog]
	set whichButton [keylget cbs widget]

	keylset parameters parent $parent
	keylset parameters instance selectHostScreen
	set appvals(selecthost) selectHostScreen
	
	keylset parameters userproc "SelectHostOkCB $whichButton"
	SaSelectHostDialog $parameters $cbs
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
	
	# get the values entered into the widgets
	set newHostName [string trim [VtGetValues $nameText -value]]
	set newRealName [string trim [VtGetValues $realNameText -value]]
	
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

	# set up the new entry
	keylset newEntry hostName $newHostName
	keylset newEntry realName $newRealName

	set mainList [VxGetVar $appvals(vtMain) mainList]
	set selected [VtDrawnListGetSelectedItem $mainList -byRecordList]

	if {$function == "add"} {
		if {[ChannelEntryExists $newHostName]} {
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
			keylset beforeEntry hostName [lindex $before 0]
		}
			
		# update the data structures
		ChannelAdd $beforeEntry $newEntry
		
	} else {
		set position [lindex [VtDrawnListGetSelectedItem $mainList \
					-byPositionList] 0]
		set old [lindex $selected 0]
		set oldHostName [lindex $old 0]
		set oldRealName [lindex $old 1]

		if {![cequal [string tolower $newHostName] \
			     [string tolower $oldHostName]] && \
		    [ChannelEntryExists $newHostName]} {
			VtShow [VtErrorDialog $dialog.usererr6 \
				-message [IntlErr NAME_DUPLICATE $newHostName] \
				-ok]
			VtUnLock
			VtSetFocus $nameText
			return
                }

		set modified 0
		if {![cequal $newHostName $oldHostName] || \
		    ![cequal $newRealName $oldRealName]} {
			keylset oldEntry hostName $oldHostName
			keylset oldEntry realName $oldRealName
			ChannelModify $oldEntry $newEntry
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
		if {[VtInfo -charm]} {
			set formatList [list \
				[list STRING $newLength 0 2] \
				[list STRING [clength $newRealName]]]
		} else {
			set formatList [list \
				[list STRING [AdjustFieldLength $newLength] \
				      0 10] \
				[list STRING [clength $newRealName]]]
		}
		set fieldList [list \
			$newHostName \
			$newRealName]

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

	set addScreen [UiBuildAddModifyScreen add]
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
		keylset entry hostName [lvarpop item]
		lappend deleteEntries $entry
	}
	ChannelDelete $deleteEntries
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

	set modifyScreen [UiBuildAddModifyScreen modify]

	set nameText [VxGetVar $modifyScreen nameText]
	set realNameText [VxGetVar $modifyScreen realNameText]

	VtSetValues $nameText -value [lindex $selected 0]
	VtSetValues $realNameText -value [lindex $selected 1]

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

	set beforeHostName [lindex $beforeItem 0]
	keylset entry1 hostName $beforeHostName

	set hostName [lindex $selectedItem 0]
	set realName [lindex $selectedItem 1]
	keylset entry2 hostName $hostName
	keylset entry2 realName $realName

	ChannelSwapPosition $entry1 $entry2

	#
	# Modify main list widget
	#

	# Delete the item selected in the list
	VtDrawnListDeleteItem $list -position $selectedNum

	# Add the deleted item back in the new position
	set maxLength [VxGetVar $list maxLength]	
	if {[VtInfo -charm]} {
		set formatList [list \
			[list STRING $maxLength 0 2] \
			[list STRING [clength $realName]]]
	} else {
		set formatList [list \
			[list STRING [AdjustFieldLength $maxLength] 0 10] \
			[list STRING [clength $realName]]]
	}
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
	set hostName [lindex $selectedItem 0]
	set realName [lindex $selectedItem 1]
	keylset entry1 hostName $hostName
	keylset entry1 realName $realName

	set afterHostName [lindex $afterItem 0]
	keylset entry2 hostName $afterHostName

	ChannelSwapPosition $entry1 $entry2

	#
	# Modify main list widget
	#

	# Delete the item selected in the list
	VtDrawnListDeleteItem $list -position $selectedNum

	# Add the deleted item back in the new position
	set maxLength [VxGetVar $list maxLength]
	if {[VtInfo -charm]} {
		set formatList [list \
			[list STRING $maxLength 0 2] \
			[list STRING [clength $realName]]]
	} else {
		set formatList [list \
			[list STRING [AdjustFieldLength $maxLength] 0 10] \
			[list STRING [clength $realName]]]
	}
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
	if {[ErrorCatch errStack 0 ChannelWrite data]} {
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

	# Basic drawnlist for channel table entries
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
