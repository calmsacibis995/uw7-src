#==============================================================================
#
#	ident @(#) aliases.tcl 11.1 97/10/30 
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
# SCO Mail Administration Aliases client:
# GUI front end for editing an aliases file
#
#
# Modification History
#
# M000, 20-Feb-97, andrean
#	- Created -- based upon OSR 5.0.2 MMDF Aliases client.
# 
#==============================================================================

#
# Aliases file editor
#

# Globals

# Handle for alias file table
set MA_ALIAS_FILEID -1
# Alias filename
set MA_ALIAS_FILENAME ""
# Alias database class
set MA_ALIAS_CLASS NONE
# All supported database classes
set MA_ALIAS_DATABASE_CLASSES [list dbm btree hash implicit NONE]
# Name of host we are managing
set MA_ALIAS_MANAGED_HOST "localhost"
# The factory defaults sendmail.cf file
set MA_ALIAS_SENDMAILCF /etc/mail/sendmailcf.factory
# The newaliases binary (to compile alias file databases)
set MA_ALIAS_NEWALIASES /etc/mail/newaliases

proc SaSetFocusUnlock { what cbs } {
	SaSetFocus $what $cbs
	VtUnLock
}

proc GetUsers {} {
	global MA_ALIAS_MANAGED_HOST
	# SaUsersGet should already sort the users
	return [SaUsersGet $MA_ALIAS_MANAGED_HOST]
}

proc NameErr { parent } {
	VtShow\
		[VtErrorDialog $parent.eb -ok\
			-message [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_NAME]]
}

set MyStatusCurID "*not*a*form*id*"
set MyStatusCurForm ""
proc MyStatusBox { id what } {
	global MyStatusCurID MyStatusCurForm

	if {$what == "-close"} {
		if {$id == $MyStatusCurID} {
			VtDestroyDialog $MyStatusCurForm
			VtUnLock -once
			set MyStatusCurID "*not*a*form*id*"
		} else {
			error "MyStatusBox: $id not the current form"
		}
		return
	}

	if {$id == $MyStatusCurID} {
		set label [VxGetVar $MyStatusCurForm "label"]
		VtSetValues $label -label "$what"
		return
	}

	if {$MyStatusCurID != "*not*a*form*id*"} {
		error "MyStatusBox: another status box already being displayed"
		return
	}

	set MyStatusCurID $id
	#VtLock
	set MyStatusCurForm\
		[VtFormDialog $id\
			-title [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_STATUS_BOX_TITLE]]
	set label\
		[VtLabel $MyStatusCurForm.Label\
			-label $what\
			-rightSide FORM\
			-bottomSide FORM]
	VxSetVar $MyStatusCurForm "label" $label
	VtShow $MyStatusCurForm
	VtLock
}

proc SaveCB { cbs {doUnlock 1}} {
	global MA_ALIAS_FILEID MA_ALIAS_FILENAME MA_ALIAS_CLASS
	global MA_ALIAS_SENDMAILCF MA_ALIAS_NEWALIASES
	global mainForm errorInfo
@if test
	global TEST
@endif

	MyStatusBox $mainForm.savebox [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_SAVING]
	# We want to save whether there have been modifications or not
	Alias:ForceWrites $MA_ALIAS_FILEID

	if {[ErrorCatch errStack 0 { Alias:Write $MA_ALIAS_FILEID } errmsg] != 0} {
		MyStatusBox $mainForm.savebox -close
		SaDisplayErrorInfo $mainForm.ErrorWrite [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_WRITE $MA_ALIAS_FILENAME] noHelp
		if {$doUnlock} {
			VtUnLock
		}
		return 0
	}

	# the newaliases command doesn't work if the alias file specified in
	# the command line is not a full pathname
	if {[cindex $MA_ALIAS_FILENAME 0] != "/"} {
		set MA_ALIAS_FILENAME "[pwd]/${MA_ALIAS_FILENAME}"
	}

@if test
	if {"$TEST" == "aliases_cmd_error_newaliases" || \
	    "$TEST" == "aliases_gui_error_newaliases"} {
		set MA_ALIAS_NEWALIASES "/tmp/newaliases"
	}
@endif
	if {$MA_ALIAS_CLASS == "NONE"} {
		set cmd \
		"exec $MA_ALIAS_NEWALIASES -oA${MA_ALIAS_FILENAME} -C${MA_ALIAS_SENDMAILCF} >& /dev/null"
	} else {
		set cmd \
		"exec $MA_ALIAS_NEWALIASES -oA${MA_ALIAS_CLASS}:${MA_ALIAS_FILENAME} -C${MA_ALIAS_SENDMAILCF} >& /dev/null"
	}

	if {[ErrorCatch errStack 0 $cmd errmsg] != 0} {
		MyStatusBox $mainForm.savebox -close
		SaDisplayErrorInfo $mainForm.ErrorDB [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_DB ${MA_ALIAS_FILENAME}.db] noHelp
		if {$doUnlock} {
			VtUnLock
		}
		return 0
	}
	MyStatusBox $mainForm.savebox -close
	if {$doUnlock} {
		VtUnLock
	}
	return 1
}

proc quitCB { cbs } {
	quit
}

proc quit {} {
	VtUnLock
	VtClose
	exit
}

proc _saveConCB { which cbs } {
	set parent [keylget cbs "dialog"]
	set callback [VxGetVar $parent "callback"]
	VtDestroyDialog $parent
	$callback $which
}

proc saveConfirm { wname callback } {
	set isCharm [VtInfo -charm]
	set dlog\
		[VtFormDialog $wname\
			-okLabel [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_SAVE_CONFIRM_PB_YES]\
			-okCallback {_saveConCB "save"}\
			-autoLock _saveConCB\
			-applyLabel [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_SAVE_CONFIRM_PB_NO]\
			-applyCallback {_saveConCB "discard"}\
			-cancelLabel\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_SAVE_CONFIRM_PB_CANCEL]\
			-cancelCallback {_saveConCB "cancel"}\
			-wmCloseCallback {_saveConCB "cancel"}\
			-title [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_SAVE_CONFIRM_TITLE]]

	VxSetVar $dlog "callback" $callback

	VtLabel $dlog.label\
		-rightSide FORM\
		-font medBoldFont\
		-label "\n[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_SAVE_CONFIRM_QUERY]\n"

	VtShow $dlog
}

proc saveConfirmCB { action } {
	if { $action == "cancel"} {
		VtUnLock
		return
	}
	if { $action == "save"} {
		if {![SaveCB {} 0]} {
			VtUnLock
			return
		}
	}
	quit
}

proc CloseCB { cbs } {
	global MA_ALIAS_FILEID
	if {[Alias:Modified $MA_ALIAS_FILEID]} {
		set parent [keylget cbs "dialog"]
		saveConfirm $parent.savebox saveConfirmCB
		VtUnLock
		return
	}
	quit
}

proc ConfirmRemoval { what names okCB remove } {
	global	mainForm

	if {[llength $names] > 1} {
		set msg [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_REMOVE_MEMBERS]
	} else {
		set msg\
		  [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_REMOVE [list $what $names]]
	}
	if {"$remove" == "alias"} {
		set formname $mainForm.iboxAlias
	} else {
		set formname $mainForm.iboxMember
	}
	VtShow [VtQuestionDialog $formname\
				-okCallback $okCB -autoLock $okCB -message $msg]
}

set _UserSelPBCallback "<null>"
proc _UserSelPBDone { what cbs } {
	global _UserSelPBCallback

	set parent [keylget cbs "dialog"]
	if {$what == "OK"} {
		set userSelectList [VxGetVar $parent "userSelectList"]
		set users [VtListGetSelectedItem $userSelectList -byItemList]
	} else {
		set users {}
	}
	VtDestroyDialog $parent

	if {$_UserSelPBCallback != "<null>" && ![lempty $users]} {
		set routine [lvarpop _UserSelPBCallback]
		eval $routine [list $users] $_UserSelPBCallback [list $cbs]
	}

	VtUnLock
}

proc UserSelectPBCB { callbackName cbs } {
	global _UserSelPBCallback

	set parent [keylget cbs "dialog"]

	set _UserSelPBCallback $callbackName

	set userSelForm\
		[VtFormDialog $parent.userSelForm\
			-help\
			-okCallback { _UserSelPBDone OK }\
			-autoLock _UserSelPBDone\
			-cancelCallback { _UserSelPBDone Cancel }\
			-wmCloseCallback { _UserSelPBDone Cancel }\
			-title [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USER_SELECT_TITLE]]
	
	set userSelectList\
		[VxList $userSelForm.userSelectList\
			-rightSide FORM\
			-bottomSide FORM\
			-title [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USER_LIST_TITLE]\
			-itemList [GetUsers]\
			-MOTIF_selection EXTENDED\
			-CHARM_selection MULTIPLE\
			-rows 10\
			-columns 20]
	VxSetVar $userSelForm "userSelectList" $userSelectList
	
	VtSetFocus $userSelectList
	VtShowDialog $userSelForm
	VtUnLock
}

set _UserSelCallback "<null>"
proc _UserSelDone { what cbs } {
	global _UserSelCallback

	set parent [keylget cbs "dialog"]
	if {$what == "OK"} {
		set userSelectText [VxGetVar $parent "userSelectText"]
		set users [alias_recipients_parse \
			[VtGetValues $userSelectText -value] " "]
	} else {
		set users {}
	}
	VtDestroyDialog $parent

	if {$_UserSelCallback != "<null>"} {
		set routine [lvarpop _UserSelCallback]
		eval $routine [list $users] $_UserSelCallback [list $cbs]
	}

	VtUnLock
}

proc _UserSelectInsCB { userList parent cbs } {
	set userSelectText [VxGetVar $parent "userSelectText"]
        set currentList [alias_recipients_parse \
		[VtGetValues $userSelectText -value] " "]
	if {$currentList == ""} {
		set newValue $userList
	} else {
		set newList [lrmdups [concat $currentList $userList]]
		set newValue [alias_list_to_string $newList]
	}
	VtSetValues $userSelectText \
		-value $newValue
	VtSetValues $userSelectText \
		-xmArgs [list XmNcursorPosition [clength $newValue]]
	VtSetFocus $userSelectText
	VtUnLock
}

proc UserSelectCB { callbackName cbs } {
	global _UserSelCallback

	set parent [keylget cbs "dialog"]

	set _UserSelCallback $callbackName

	set userSelDlg\
		[VtFormDialog $parent.userSelDlg\
			-help\
			-okCallback {_UserSelDone OK}\
			-autoLock _UserSelDone\
			-cancelCallback {_UserSelDone Cancel}\
			-wmCloseCallback {_UserSelDone Cancel}\
			-title [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USER_SELECT_TITLE]]

	set selForm\
		[VtForm $userSelDlg.selForm\
			-topSide FORM\
			-bottomSide NONE\
			-leftSide FORM\
			-rightSide FORM\
			-MOTIF_marginHeight 5 -marginWidth 0]
	set label\
		[VtLabel $selForm.label\
			-topSide FORM\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USER_LIST_TITLE]\
			-labelLeft]
	set userSelectButton\
		[VtPushButton $selForm.userSelectPB\
			-topSide $label\
			-bottomSide FORM\
			-leftSide NONE\
			-rightSide FORM\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USER_SELECT_PB]\
			-callback\
				[list UserSelectPBCB [list _UserSelectInsCB $userSelDlg]]\
			-autoLock UserSelectPBCB]
	VxSetVar $userSelDlg "userSelectButton" $userSelectButton
	
	set userSelectText\
		[VxText $selForm.userSelectText\
			-topSide $label\
			-bottomSide FORM\
			-leftSide FORM\
			-rightSide $userSelectButton\
			-columns 30 \
			-callback {SaSetFocusUnlock next} \
			-autoLock SaSetFocusUnlock]
	VxSetVar $userSelDlg "userSelectText" $userSelectText

	set okButton [VtGetValues $userSelDlg -ok]
	SaSetFocusList $userSelDlg [list $userSelectText $okButton]
	VtShowDialog $userSelDlg
	VtSetFocus $userSelectText
	VtUnLock
}


#-----
# Alias:Add support routines

proc aliasAddOK { isOK cbs } {
	global MA_ALIAS_FILEID
	global	curAlias

	set parent [keylget cbs "dialog"]
	set func [VxGetVar $parent "func"]
	if { $isOK } {
		aliasAddTypeCB $cbs 0		;# In case the user forgot to press [Add]

		set newName [VxGetVar $parent "newName"]
		set newMembers [VxGetVar $parent "newMembers"]

		set name [VtGetValues $newName -value]
		set new [VtListGetItem $newMembers -all]
		if {$name == "" || $new == ""} {
			VtShow\
			  [VtErrorDialog $parent.eb -ok -help\
				-message [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_NONAMES]]
			VtUnLock
			return
		}
		if {[regexp {[:, 	]}  $name]} {
			NameErr $parent
			VtUnLock
			return
		}

		set aliasList [string tolower [Alias:List $MA_ALIAS_FILEID]]
		if { ( $func == "add" || \
		       [string tolower $curAlias] != [string tolower $name] ) \
		   && [lsearch $aliasList [string tolower $name]] != -1} {
			VtShow\
			  [VtErrorDialog $parent.eb -ok -help\
				-message [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_DUPLICATE]]
			VtUnLock
			return
		}

		if {$func == "add"} {
			Alias:Set $MA_ALIAS_FILEID $name $new
		} else {
			Alias:Unset $MA_ALIAS_FILEID $curAlias
			Alias:Set $MA_ALIAS_FILEID $name $new
		}
	}

	VtDestroyDialog $parent

	if { $isOK } {
		# Display new list
		aliasRedrawList $name
	}

	VtUnLock
}

proc aliasAddSelectCB { userList parent cbs } {
	set listWidget [VxGetVar $parent "newMembers"]
	set newList [lrmdups [concat [VtListGetItem $listWidget -all] $userList]]
	VtListSetItem $listWidget -itemList $newList
	VtListSelectItem $listWidget -itemList $userList
	SaListShowSelectedItem $listWidget
	aliasAddListCB $parent $cbs
}

proc aliasAddSelect { cbs } {
	set parent [keylget cbs "dialog"]
	UserSelectPBCB [list aliasAddSelectCB $parent] $cbs
	VtUnLock
}

proc aliasAddRem { cbs } {
	set parent [keylget cbs "dialog"]
	set newMembers [VxGetVar $parent "newMembers"]
	set posnList [VtListGetSelectedItem $newMembers -byPositionList]
	if {$posnList != 0} {
		set numAll [llength [VtListGetItem $newMembers -all]]
		VtListDeleteItem $newMembers -positionList $posnList
		set numLeft [llength [VtListGetItem $newMembers -all]]
		if {$numLeft > 0} {
			set first [lindex $posnList 0]
			set last [lindex $posnList end]
			set removed [llength $posnList]
			if {[expr $last - $first + 1] == $removed} {
				set contiguous 1
			} else {
				set contiguous 0
			}

			if {$last == $numAll && $contiguous} {
				# select the one above
				VtListSelectItem $newMembers -position [expr $first - 1]
			} else {
				# select the item that was after the first
				# deleted (same position number as first
				# deleted was)
				VtListSelectItem $newMembers -position $first
			}
			SaListShowSelectedItem $newMembers
		}
	}
	VtUnLock
}

proc aliasAddTypeCB { cbs {doUnlock 1} } {
	set parent [keylget cbs "dialog"]
	set textWidget [VxGetVar $parent "typeMembers"]
	set listWidget [VxGetVar $parent "newMembers"]
	set userList [alias_recipients_parse \
		[VtGetValues $textWidget -value] " "]
	if {![lempty $userList]} {
		set newList [lrmdups [concat [VtListGetItem $listWidget -all] $userList]]
		VtListSetItem $listWidget -itemList $newList
		positionList $listWidget [lindex [lsort $userList] 0]
		VtListSelectItem $listWidget -itemList $userList
		VtSetValues $textWidget -value {}
		aliasAddListCB $parent $cbs
	}
	if {$doUnlock} {
		VtUnLock
	}
}

proc aliasAddListCB { parent cbs } {
	VtUnLock
}

proc aliasAddEdit { cbs } {
	set parent [keylget cbs "dialog"]
	set typeMembers [VxGetVar $parent "typeMembers"]
	set listWidget [VxGetVar $parent "newMembers"]

	# Get the selected names
	set selList [VtListGetSelectedItem $listWidget -byItemList]

	if {![lempty $selList]} {
		set selString [alias_list_to_string $selList]

		# Put them into the text widget
		VtSetValues $typeMembers\
			-value $selString
		VtSetValues $typeMembers\
			-xmArgs [list \
				XmNcursorPosition \
				[string length $selString]]

		# Then remove them from the list
		aliasAddRem $cbs
	}

	VtUnLock
}

proc positionList { listWidget prefix {deSelect 0} } {
	set itemList [VtListGetItem $listWidget -all]
	set listCB [VxGetVar $listWidget "callback"]
	if {$prefix != ""} {
		# recipient names can have backslash, so need to
		# double them before the search
		regsub -all {\\} $prefix {\\\\} prefix
		set pos [lsearch -glob $itemList "${prefix}*"]
		if {$pos != -1} {
			set rows [VtGetValues $listWidget -rows]
			set mpos [expr $pos - ($rows / 2)]
			if {$mpos < 1} {
				set mpos 1
			}
			VtSetValues $listWidget -topItemPosition $mpos
			if {$deSelect} {
				VtListDeselectItem $listWidget -all
			}
			VtListSelectItem $listWidget -position [expr $pos + 1]
			if {$listCB != ""} {
				$listCB 0 {}
			}
		}
	}
}

proc aliasJumpCB { listWidget cbs } {
	set prefix [keylget cbs "value"]
	positionList $listWidget $prefix 1
	VtUnLock
}

proc aliasAdd { func cbs } {
	global MA_ALIAS_FILEID
	global	isCharm curAlias

	set parent [keylget cbs "dialog"]

	if {$func == "add"} {
		set title  [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_ADD_TITLE]
	} else {
		set title [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MFY_TITLE $curAlias]
	}

	set aliasAddForm\
		[VtFormDialog $parent.aliasAddForm\
			-title "$title"\
			-help\
			-okCallback {aliasAddOK 1}\
			-autoLock aliasAddOK\
			-cancelCallback {aliasAddOK 0}\
			-wmCloseCallback {aliasAddOK 0}]
	SaCharmSetMaxFormDimensions $aliasAddForm 1
	VxSetVar $aliasAddForm "func" $func

	if {$func == "add"} {
		set value ""
	} else {
		set value $curAlias
	}
	set newName\
		[VxText $aliasAddForm.newName\
			-topSide FORM\
			-leftSide FORM\
			-rightSide FORM\
			-title [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_ADD_NAME]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_ALIAS_ADD_NAME]\
			-activateCallback {SaSetFocusUnlock next}\
			-autoLock SaSetFocusUnlock\
			-value $value\
			-columns 30]
	VxSetVar $aliasAddForm "newName" $newName
	set newNameForm [VxGetVar $newName "form"]

	if {$isCharm} {
		VtLabel $aliasAddForm.blank1 -label " "
	}

	VtLabel $aliasAddForm.newLabel\
		-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_ADD_MEMBERS]
	set newForm [VtForm $aliasAddForm.newForm\
					-leftSide FORM\
					-rightSide FORM\
					-borderWidth 1]

	set typeMembers\
		[VxText $newForm.typeMembers\
			-topSide FORM\
			-leftSide FORM\
			-rightSide FORM\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_ADD_TYPE]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_ALIAS_ADD_TYPE]\
			-activateCallback aliasAddTypeCB\
			-autoLock aliasAddTypeCB]
	set typeMembersForm [VxGetVar $typeMembers "form"]
	VxSetVar $aliasAddForm "typeMembers" $typeMembers
	VtSetValues $typeMembers -rightSide NONE
	set addPB\
		[VtPushButton $typeMembersForm.add\
			-topSide FORM\
			-bottomSide FORM\
			-rightSide FORM\
			-leftSide NONE\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_ADD_PB]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_ALIAS_ADD_PB]\
			-callback aliasAddTypeCB\
			-autoLock aliasAddTypeCB]
	VxSetVar $aliasAddForm "addPB" $addPB
	VtSetValues $typeMembers -rightSide $addPB

	if {$func == "add"} {
		set items {}
	} else {
		set items [lsort [Alias:Get $MA_ALIAS_FILEID $curAlias]]
	}

	VtLabel $newForm.newMembersLabel\
		-topSide $typeMembersForm\
		-leftSide FORM\
		-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_ADD_MEMBERS_TITLE]
	set listForm\
		[VtForm $newForm.newMembers\
			-leftSide FORM\
			-rightSide FORM\
			-bottomSide FORM\
			-marginWidth 0\
			-marginHeight 0]
	set listJumpForm\
		[VtForm $listForm.newMembers\
			-leftSide FORM\
			-rightSide FORM\
			-bottomSide FORM\
			-marginWidth 0\
			-marginHeight 0]
	set newMembers\
		[VtList $listJumpForm.newMembers\
			-topSide FORM\
			-bottomSide NONE\
			-leftSide FORM\
			-rightSide FORM\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
			  [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_ALIAS_ADD_MEMBERS]\
			-callback [list aliasAddListCB $aliasAddForm]\
			-autoLock aliasAddListCB\
			-MOTIF_rows 4 -CHARM_rows 10\
			-columns 25\
			-scrollBar 1\
			-itemList $items\
			-MOTIF_selection EXTENDED\
			-CHARM_selection MULTIPLE]
	VxSetVar $aliasAddForm "newMembers" $newMembers
	VxSetVar $newMembers "callback" {}
	set jumpbox\
		[VxText $listJumpForm.jumpbox\
			-topSide NONE\
			-bottomSide FORM\
			-leftSide FORM\
			-rightSide FORM\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_SEARCH]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
			  [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_SEARCH]\
			-valueChangedCallback [list aliasJumpCB $newMembers]\
			-autoLock aliasJumpCB]
	set jumpboxForm [VxGetVar $jumpbox "form"]
	VtSetValues $newMembers -bottomSide $jumpboxForm
	set pbForm [VtRowColumn $listForm.pbForm\
				-topSide FORM\
				-bottomSide FORM\
				-rightSide FORM\
				-leftSide NONE\
				-vertical]
	VtSetValues $listJumpForm -rightSide $pbForm

	set selectPB\
		[VtPushButton $pbForm.select\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_SELECT_PB]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_ALIAS_SEL_PB]\
			-callback aliasAddSelect\
			-autoLock aliasAddSelect]
	VxSetVar $aliasAddForm "selectPB" $selectPB
	set removePB\
		[VtPushButton $pbForm.remove\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_REMOVE_PB]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_ALIAS_REM_PB]\
			-callback aliasAddRem\
			-autoLock aliasAddRem]
	VxSetVar $aliasAddForm "removePB" $removePB
	VtSetValues $removePB -leftOffset -[expr [VtGetValues $removePB -width] / 2]
	set editPB\
		[VtPushButton $pbForm.edit\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_EDIT_PB]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_ALIAS_EDIT_PB]\
			-callback aliasAddEdit\
			-autoLock aliasAddEdit]
	VxSetVar $aliasAddForm "editPB" $editPB
	VtSetValues $editPB -leftOffset -[expr [VtGetValues $editPB -width] / 2]

  # Now, pile things on the bottom of the form, in reverse order:
	set statusbar [SaStatusBar $aliasAddForm.StatusBar]
	set sep [VtSeparator $aliasAddForm._\
				-rightSide FORM -rightOffset 0\
				-leftSide FORM -leftOffset 0\
				-bottomSide $statusbar -bottomOffset 0\
				-topSide NONE]

	VtSetValues $newForm -bottomSide $sep

	VtShow $aliasAddForm

	SaSetFocusList $aliasAddForm [list $newName $typeMembers]
	SaSetTabGroups $aliasAddForm\
		[list $newName $typeMembers $addPB $newMembers $pbForm $jumpbox]

	VtSetFocus $newName
	VtUnLock
}

#-----

proc aliasRemoveCB { cbs } {
	global MA_ALIAS_FILEID
	global curAlias members aliasList

	Alias:Unset $MA_ALIAS_FILEID $curAlias

	# Update lists
	set selected [VtListGetSelectedItem $aliasList]
	VtListDeleteItem $aliasList -position $selected
	set numItems [llength [VtListGetItem $aliasList -all]]
	if {$numItems > 0} {
		if {$selected <= $numItems} {
			VtListSelectItem $aliasList \
				-position $selected
			SaListShowSelectedItem $aliasList
		} else {
			VtListSelectItem $aliasList \
				-position [expr $selected - 1]
		}
	}
        AliasCB 1 {} 0
	VtUnLock
}

proc aliasRemove { cbs } {
	global curAlias

	ConfirmRemoval [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_ALIAS]\
					$curAlias aliasRemoveCB alias
	VtUnLock
}

#-----

proc memberAddOK { isOK cbs } {
	global MA_ALIAS_FILEID
	global curAlias members
	global memberList

	set parent [keylget cbs "dialog"]
	if { $isOK } {
		set newMembers [VxGetVar $parent "newMembers"]

		set new [alias_recipients_parse \
			[VtGetValues $newMembers -value] " "]
		if {$new == ""} {
			VtShow\
			  [VtErrorDialog $parent.eb -ok -help\
				-message [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_NOMEMBERS]]
			VtSetFocus $newMembers
			VtUnLock
			return
		}
		set alias $curAlias
		Alias:AddValue $MA_ALIAS_FILEID $alias $new
	}

	VtDestroyDialog $parent

	if { $isOK } {
		# Display new list
		set alias $curAlias
		set members [lsort [Alias:Get $MA_ALIAS_FILEID $alias]]
		VtListSetItem $memberList -itemList $members
		VtListSelectItem $memberList -itemList $new
		SaListShowSelectedItem $memberList
		MemberCB 1 {}
	}

	VtUnLock
}

proc memberAddSelectCB { userList parent cbs } {
	set textWidget [VxGetVar $parent "newMembers"]
	set oldList [alias_recipients_parse \
		[VtGetValues $textWidget -value] " "]
	set newList [lrmdups [concat $oldList $userList]]
	set newString [alias_list_to_string $newList]
	VtSetValues $textWidget \
		-value $newString
	VtSetValues $textWidget \
		-xmArgs [list XmNcursorPosition [clength $newString]]
	VtSetFocus $textWidget
}

proc memberAddSelect { cbs } {
	set parent [keylget cbs "dialog"]
	UserSelectPBCB [list memberAddSelectCB $parent] $cbs
	VtUnLock
}

proc memberAdd { cbs } {
	global curAlias

	set parent [keylget cbs "dialog"]
	set memberAddForm\
		[VtFormDialog $parent.memberAddForm\
			-title [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MEMBER_TITLE]\
			-help\
			-okCallback {memberAddOK 1}\
			-autoLock memberAddOK\
			-cancelCallback {memberAddOK 0}]
	set textLabel\
		[VtLabel $memberAddForm.textLabel\
			-label [IntlLocalizeMsg\
				SCO_MAIL_ADMIN_ALIAS_MSG_MEMBER_ADD $curAlias]\
			-topSide FORM\
			-bottomSide NONE\
			-labelLeft]
	set selectForm\
		[VtForm $memberAddForm.selectForm\
			-topSide $textLabel\
			-leftSide FORM\
			-rightSide FORM\
			-bottomSide NONE\
			-marginHeight 0 -marginWidth 0]
	set selectPB\
		[VtPushButton $selectForm.selectPB\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_SELECT_PB]\
			-topSide FORM -MOTIF_topOffset 0\
			-bottomSide FORM -MOTIF_bottomOffset 18\
			-leftSide NONE\
			-rightSide FORM\
			-callback memberAddSelect\
			-autoLock memberAddSelect]
	set newMembers\
		[VxText $selectForm.newMembers\
			-topSide FORM\
			-bottomSide FORM\
			-leftSide FORM\
			-rightSide $selectPB\
			-columns 50\
			-activateCallback {SaSetFocusUnlock next}\
			-autoLock SaSetFocusUnlock\
			-horizontalScrollBar 1]
	VxSetVar $memberAddForm "newMembers" $newMembers
	set label\
		[VtLabel $memberAddForm.label\
			-topSide $selectForm\
			-bottomSide NONE\
			-leftSide FORM\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MEMBER_ADD_INST]]

	VtShow $memberAddForm
	SaSetFocusList $memberAddForm [list \
		$newMembers \
		[VtGetValues $memberAddForm -ok]]
	VtSetFocus $newMembers
	VtUnLock
}

#-----

proc memberRemoveCB { cbs } {
	global MA_ALIAS_FILEID
	global curAlias
	global memberList

	set alias $curAlias
	set selMembers [VtListGetSelectedItem $memberList -byItemList]
	set numAll [llength [VtListGetItem $memberList -all]]
	Alias:RemoveValue $MA_ALIAS_FILEID $alias $selMembers

	# Display new list
	set selPositions [VtListGetSelectedItem $memberList -byPositionList]
	VtListDeleteItem $memberList -positionList $selPositions
	set numLeft [llength [VtListGetItem $memberList -all]]
	if {$numLeft > 0} {
		set first [lindex $selPositions 0]
		set last [lindex $selPositions end]
		set deleted [llength $selPositions]
		if {[expr $last - $first + 1] == $deleted} {
			set contiguous 1
		} else {
			set contiguous 0
		}

		if {$last == $numAll && $contiguous} {
			# select the one above
			VtListSelectItem $memberList -position [expr $first - 1]
		} else {
			# select the item that was after the first deleted
			# (same position number as first deleted was)
			VtListSelectItem $memberList -position $first
		}
		SaListShowSelectedItem $memberList
	}

	MemberCB 1 {} 0
	VtUnLock
}

proc memberRemove { cbs } {
	global MA_ALIAS_FILEID
	global mainForm memberList
	global curAlias

	set alias $curAlias
	set size [llength [Alias:Get $MA_ALIAS_FILEID $alias]]
	if {$size <= 1} {
	    # Only allow member deletion if more than one member left
		VtShow\
		  [VtErrorDialog $mainForm.eb -ok -help\
			-message [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_LAST_MEMBER]]
		VtUnLock
		return
	}

	set selMembers [VtListGetSelectedItem $memberList -byItemList]
	ConfirmRemoval [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MEMBER]\
					$selMembers memberRemoveCB member

	VtUnLock
}

#-----

#
# Add $userName to each alias in tcl list $aliasList.
# If $aliasList is NULL, then add $userName to all existing aliases.
#
proc userSubscribe { userName {aliasList {}} } {
	global MA_ALIAS_FILEID

	# Since userName could be a quoted string (as in pipe to a program)
	set addList [list $userName]

	if {[lempty $aliasList]} {
		set allAliases [Alias:List $MA_ALIAS_FILEID]
		foreach alias $allAliases {
			Alias:AddValue $MA_ALIAS_FILEID $alias $addList
		}
	} else {
		foreach alias $aliasList {
			Alias:AddValue $MA_ALIAS_FILEID $alias $addList
		}
	}
}

proc usersSubscribeCB { userList cbs } {
	global list_o_users

	set list_o_users $userList
	usersSubscribeScreen
}

proc usersSubscribeDone { isOK cbs } {
	global MA_ALIAS_FILEID
	global	curAlias mainForm list_o_users user

	set parent [keylget cbs "dialog"]

	if {$isOK} {
		set sysListBox [VxGetVar $parent "sysList"]
		set userListBox [VxGetVar $parent "userList"]

		# First, go through all aliases *not* on the user's list 
		# and remove her
		set sysList [VtListGetItem $sysListBox -all]
		if {![lempty $sysList]} {
			userRemove $user $sysList
		}

		# Then, loop through the selected list and put her on
		set userList [VtListGetItem $userListBox -all]
		if {![lempty $userList]} {
			userSubscribe $user $userList
		}

		# Cause the current member list to be redrawn, just in case
		AliasCB 0 {} 0
	}

	VtDestroyDialog $parent

	if {![lempty $list_o_users]} {
		usersSubscribeScreen
	}

	VtUnLock
}

proc usersSubAdd { cbs } {
	set parent [keylget cbs "dialog"]
	set sysListBox [VxGetVar $parent "sysList"]
	set userListBox [VxGetVar $parent "userList"]

	set addList [VtListGetSelectedItem $sysListBox -byItemList]
	set userList [VtListGetItem $userListBox -all]
	VtSetValues $userListBox -itemList [lsort [concat $addList $userList]]
	VtListDeleteItem $sysListBox -itemList $addList
	VtUnLock
}

proc usersSubRem { cbs } {
	set parent [keylget cbs "dialog"]
	set sysListBox [VxGetVar $parent "sysList"]
	set userListBox [VxGetVar $parent "userList"]

	set remList [VtListGetSelectedItem $userListBox -byItemList]
	set sysList [VtListGetItem $sysListBox -all]
	VtSetValues $sysListBox -itemList [lsort [concat $remList $sysList]]
	VtListDeleteItem $userListBox -itemList $remList
	VtUnLock
}

proc usersSubscribeScreen {} {
	global MA_ALIAS_FILEID MA_ALIAS_FILENAME
	global mainForm list_o_users user

	if {[lempty $list_o_users]} {
		return
	}

	VtLock
	set parent $mainForm

	set user [lvarpop list_o_users]
	set userString [list $user]
	set title [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USER_SUB_TITLE $userString]
	set subForm\
		[VtFormDialog $parent.subform\
			-title $title\
			-help\
			-okCallback {usersSubscribeDone 1}\
			-autoLock usersSubscribeDone\
			-cancelCallback {usersSubscribeDone 0}]
	set label\
	  [VtLabel $subForm.inst\
		-font largeBoldFont\
		-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USER_SUB_INST $userString]]

	set label [VtSeparator -horizontal -rightSide FORM]
	
	set bForm\
		[VtRowColumn $subForm.bForm\
			-MOTIF_topOffset 50\
			-CHARM_topOffset 5\
			-leftSide 50\
			-vertical]
	set add\
		[VtPushButton $bForm.add\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USER_SUB_ADD_PB]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
			  [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_USER_SUB_ADD_PB]\
			-callback usersSubAdd\
			-autoLock usersSubAdd]
	set remove\
		[VtPushButton $bForm.remove\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USER_SUB_RMV_PB]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
			  [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_USER_SUB_RMV_PB]\
			-callback usersSubRem\
			-autoLock usersSubRem]
	set offset [expr [VtGetValues $add -width] / 2 + 10]	;# 10 for border
	VtSetValues $bForm -leftOffset -$offset

	set sysList\
		[VxList $subForm.sysList\
			-topSide $label\
			-leftSide FORM\
			-rightSide $bForm\
			-title [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USER_SUB_SYSLIST]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
			  [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_USER_SUB_SYSLIST $MA_ALIAS_FILENAME]\
			-itemList [lsort [Alias:List $MA_ALIAS_FILEID]]\
			-MOTIF_selection EXTENDED\
			-CHARM_selection MULTIPLE\
			-rows 10]
	set sysListForm [VxGetVar $sysList "form"]
	VxSetVar $subForm "sysList" $sysList

	set title [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USER_SUB_ULIST $userString]
	set userList\
		[VxList $subForm.userList\
			-topSide $label\
			-leftSide $bForm\
			-rightSide FORM\
			-title $title\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
			  [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_USER_SUB_ULIST]\
			-MOTIF_selection EXTENDED\
			-CHARM_selection MULTIPLE\
			-rows 10]
	set userListForm [VxGetVar $userList "form"]
	VxSetVar $subForm "userList" $userList

	set statusBar [SaStatusBar $subForm.StatusBar 0]
	VtSetValues $sysListForm -bottomSide $statusBar
	VtSetValues $userListForm -bottomSide $statusBar

	VtShow $subForm

	# Now find all aliases this user is a member of, and "ADD >>" them...
	set subList {}
	foreach alias [Alias:Memberships $MA_ALIAS_FILEID $user] {
		lappend subList $alias
	}
	if {![lempty $subList]} {
		# Select all items we found...
		VtListSelectItem $sysList -itemList $subList
		# NOW push the button, Max...
		keylset cbs "dialog" $subForm
		usersSubAdd $cbs
	}

	VtUnLock
}

#-----

#
# Remove the $user from all aliases in $aliasList.
# If $aliasList is empty, then remove the user from all aliases.
#
proc userRemove { userName {aliasList {}} } {
	global MA_ALIAS_FILEID

	# Since userName could be a quoted string (as in pipe to a program)
	set removeList [list $userName]
	if {[llength $aliasList] == 0} {
		foreach alias [Alias:Memberships $MA_ALIAS_FILEID $userName] {
			Alias:RemoveValue $MA_ALIAS_FILEID $alias $removeList
		}
	} else {
		foreach alias $aliasList {
			Alias:RemoveValue $MA_ALIAS_FILEID $alias $removeList
		}
	}
}

proc usersRetireDone { isOK cbs } {
	global MA_ALIAS_FILEID
	global mainForm list_o_users user

	set parent [keylget cbs "dialog"]

	if {$isOK} {
		set redir [VxGetVar $parent "redir"]
		set doRedirection [VtGetValues $redir -value]
		set redirAlias [VxGetVar $parent "redirAlias"]
		set addrList [alias_recipients_parse \
			[VtGetValues $redirAlias -value] " "]

		# Check the redirection name(s)
		if {$doRedirection && [lempty $addrList]} {
			VtShow\
			  [VtErrorDialog $parent.eb -ok\
				-message [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_NO_REDIR]]
			VtUnLock
			VtSetFocus $redirAlias
			return
		}

		# Remove the user from all aliases and lists...
		userRemove $user

		# Redirect the user's mail, as if requested...
		if {$doRedirection} {
			Alias:Set $MA_ALIAS_FILEID $user $addrList
		}

		# Cause all lists to be redrawn, just in case they changed
		aliasRedrawList $user
	}

	VtDestroyDialog $parent

	if {![lempty $list_o_users]} {
		usersRetireScreen
	}

	VtUnLock
}

proc usersRetireCB { userList cbs } {
	global list_o_users

	set list_o_users $userList
	usersRetireScreen
}

proc usersRetCheckCB { cbs } {
	set parent [keylget cbs "dialog"]
	set value [keylget cbs "value"]

	set redir [VxGetVar $parent "redir"]
	set redirAlias [VxGetVar $parent "redirAlias"]

	if {$value == $redir } {
		VtSet $redirAlias -sensitive 1
		VtSetFocus $redirAlias
	} else {
		VtSet $redirAlias -sensitive 0
	}

	VtUnLock
}

proc usersRetireScreen {} {
	global mainForm list_o_users user tailorLoaded MA_ALIAS_FILENAME

	if {[lempty $list_o_users]} {
		return
	}

	VtLock
	set parent $mainForm
	set user [lvarpop list_o_users]

	set focusList {}

	# $user may be a quoted string (as in a pipe to program)
	# so want to make a list of $user before IntlLocalizeMsg
	set title [IntlLocalizeMsg \
		SCO_MAIL_ADMIN_ALIAS_MSG_USER_RET_TITLE [list $user]]
	set retireForm\
		[VtFormDialog $parent.retireForm\
			-title $title\
			-help\
			-okCallback {usersRetireDone 1}\
			-autoLock usersRetireDone\
			-cancelCallback {usersRetireDone 0}]

	set label\
	  [VtLabel $retireForm.label\
		-rightSide FORM\
		-font medBoldFont\
		-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USER_RET_INST\
				[list $user $MA_ALIAS_FILENAME]]]

	set sep [VtSeparator -horizontal -topSide $label -rightSide FORM]

	set cbox\
		[VtCheckBox $retireForm.cbox\
			-topSide $sep\
			-leftSide FORM\
			-callback usersRetCheckCB\
			-autoLock usersRetCheckCB]
	lappend focusList $cbox

	set redir\
                [VtToggleButton $cbox.redir\
                        -label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USER_RET_REDIR [list $user]]]
	VxSetVar $retireForm "redir" $redir

	set redirAlias\
		[VtText $retireForm.redirAlias\
			-rightSide FORM\
			-bottomSide FORM\
			-callback {SaSetFocus next}]

	VxSetVar $retireForm "redirAlias" $redirAlias
	VtSetSensitive $redirAlias 0
	lappend focusList $redirAlias
	set okButton [VtGetValues $retireForm -ok]
	lappend focusList $okButton

	if {[regexp {[:, 	]} $user] || [cequal [cindex $user 0] "\\"]} {
		VtSetSensitive $redir 0
	}

	VtShow $retireForm
	SaSetFocusList $retireForm $focusList
	SaSetTabGroups $retireForm $focusList
	VtSetFocus $redir
	VtUnLock
}

#-----

proc MemberCB { clearSearch cbs {doUnlock 1}} {
	global memberList meRmv membSearch

	set curMember [lindex [VtListGetSelectedItem $memberList -byItemList] 0]
	if { $curMember == "" } {
		VtSetSensitive $meRmv 0
	} else {
		VtSetSensitive $meRmv 1
	}

	if {$clearSearch} {
		VtSetValues $membSearch -value ""
	}

	if {$doUnlock} {
		VtUnLock
	}
}

#-----

proc AliasCB { clearSearch cbs {doUnlock 1}} {
	global MA_ALIAS_FILEID
	global curAlias members
	global aliasList memberList alMfy alRmv meAdd meRmv usSub usRet aliasSearch

	if {[lempty [VtListGetItem $aliasList -all]]} {
		set curAlias ""
	} else {
		set selItem [lindex [VtListGetSelectedItem $aliasList -byItemList] 0]
		set curAlias [string trim [lindex $selItem 0]]
	}

	if {$curAlias == ""} {
		set members {}
		VtSetSensitive $alRmv 0
		VtSetSensitive $alMfy 0
		VtSetSensitive $meAdd 0
		VtSetSensitive $usSub 0
		VtSetSensitive $usRet 0
	} else {
		set members [lsort [Alias:Get $MA_ALIAS_FILEID $curAlias]]
		VtSetSensitive $alMfy 1
		VtSetSensitive $alRmv 1
		VtSetSensitive $meAdd 1
		VtSetSensitive $usSub 1
		VtSetSensitive $usRet 1
	}
	VtSetSensitive $meRmv 0
	VtListSetItem $memberList -itemList $members
	if {![lempty $members]} {
		VtListSelectItem $memberList -position 1
		MemberCB 1 {} 0
	}

	if {$clearSearch} {
		VtSetValues $aliasSearch -value ""
	}

	if {$doUnlock} {
		VtUnLock
	}
}

#-----

proc aliasRedrawList { itemToSelect } {
	global MA_ALIAS_FILEID
	global aliasList
	global Bypass Public ALIAS LIST

	set aliasLines {}
	set aliases [lsort [Alias:List $MA_ALIAS_FILEID]]

	set aliasLines $aliases
	VtSetValues $aliasList -itemList $aliasLines

	if {![lempty $aliases]} {
		if {$itemToSelect == ""} {
			VtListSelectItem $aliasList -position 1
		} else {
			set pos [expr {[lsearch $aliases $itemToSelect] + 1}]
			if {$pos == 0} {
				set pos 1
			}
			VtListSelectItem $aliasList -position $pos

			# Now position the item to the middle (or as close as we can)
			set offset [expr [VtGetValues $aliasList -rows] / 2]
			incr pos -$offset
			if {$pos < 1} {
				set pos 1
			}
			VtSetValues $aliasList -topItemPosition $pos
		}
	}
	AliasCB 1 {} 0
}

proc mainErrorCB { cbs } {
	VtUnLock
	VtClose
	exit 1
}

proc alias_load {} {
	global MA_ALIAS_FILENAME MA_ALIAS_FILEID 
	global mainForm MyStatusCurID errorInfo

	set cmd "Alias:Open $MA_ALIAS_FILENAME"
	if {[ErrorCatch errStack 0 $cmd result] != 0} {
		if {"$mainForm" == ""} {
			puts stderr [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_CANT_LOAD $MA_ALIAS_FILENAME]
			puts stderr $result
			exit 1
		} else {
			SaDisplayErrorInfo $mainForm.ErrorLoad [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_CANT_LOAD $MA_ALIAS_FILENAME] noHelp quitCB
                }
	} else {
		set MA_ALIAS_FILEID $result
	}
}

proc alias_list_to_string {memberList} {
	set memberString ""

	if {![lempty $memberList]} {
		set memberString [lvarpop memberList]
		foreach member $memberList {
			append memberString " $member"
		}
	}

	return $memberString
}

proc alias_recipients_parse {recipients {separator {}}} {
	set result {}
	if {$separator == ""} {
		set separator ","
	}

	# need to trim separators on the ends, otherwise
	# null list items will be created
	set recipients [string trim $recipients $separator]

	while {[clength $recipients] > 0} {

		set thisToken [ctoken recipients $separator]

		# Spaces are allowed between the ":include:" and filename,
		# which we must anticipate if the separator is also a space.
		if {[regexp {^:include:$} $thisToken] && \
		    $separator == " "} {
			set filepath [ctoken recipients $separator]
			append thisToken $filepath
		}

		# If there is an un-matched quote, then we need to grab
		# text through the next quote.
		set numQuotes [expr [llength [split $thisToken "\""]] - 1]
		if {[fmod $numQuotes 2] == 1.0} {
			set restOfQuotedPart [ctoken recipients "\""]
			append thisToken $restOfQuotedPart
			append thisToken [ctoken recipients $separator]
		}
		lappend result [string trim $thisToken]
	}
	return $result
}

proc alias_sub_ret_parse {argument} {
	set user {}
	set aliases {}
	if {[regexp {^(:include:.*):(.*)$} $argument match \
		recipientName aliasNames]} {
		set user $recipientName 
		set aliases [translit "," " " $aliasNames]
	} else {
		set thisToken [ctoken argument ":"]
		if {[regexp {^[ 	]+$} $thisToken]} {
			set thisToken ""
		}
		set numQuotes [expr [llength [split $thisToken "\""]] - 1]
		if {[fmod $numQuotes 2] == 1.0} {
			set restOfQuotedPart [ctoken argument "\""]
			append thisToken "${restOfQuotedPart}"
			append thisToken [ctoken argument ":"]
		}
		set user $thisToken
		set aliases [translit "," " " [crange $argument 1 len]]
	}
	return [list $user $aliases]
}


#--------
# main()

ErrorTopLevelCatch {
	global MA_ALIAS_FILENAME MA_ALIAS_CLASS MA_ALIAS_FILEID
	global MA_ALIAS_SENDMAILCF MA_ALIAS_NEWALIASES MA_ALIAS_MANAGED_HOST
	global argc optarg optind

	set ADD 0
	set DELETE 0
	set RETIRE 0
	set SUBSCRIBE 0
	set add_aliases {}
	set delete_aliases {}
	set retire_users {}
	set subscribe_users {}
	set mainForm ""
@if test
	global TEST
	set TEST ""
	set shortcut {}
	set DO_SHORTCUT 0
@endif

	# scan for commandline flags
	set argc [llength $argv]
	if {$argc == 0} {
		puts stderr [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_USAGE]
		exit 1
	}
		
@if test
	while {[set opt [getopt $argc $argv "a:d:f:h:R:r:S:s:t:"]] != -1} {
@else
	while {[set opt [getopt $argc $argv "a:d:f:h:R:r:S:s:"]] != -1} {
@endif
		switch $opt {

		f {
			set beforeColon [ctoken optarg ":"]
			if {$optarg == ""} {
				set MA_ALIAS_FILENAME [string trim $beforeColon]
				set MA_ALIAS_CLASS NONE
			} else {
				set MA_ALIAS_FILENAME \
					[string trim [ctoken optarg ":"]]
				set MA_ALIAS_CLASS [string trim $beforeColon]
			}

		}

		h {
			set MA_ALIAS_MANAGED_HOST $optarg
			
		}

		a {
			set aliasName [ctoken optarg ":"]
			set aliasRecipients \
				[alias_recipients_parse [crange $optarg 1 len]]
			if {[lempty $aliasRecipients]} {
				puts stderr [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_MISSING_ADD_ARG]
				puts stderr [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_USAGE]
				exit 1
			}
			lappend add_aliases [list $aliasName $aliasRecipients]
			set ADD 1
		}

		d {
			set delete_aliases [union $delete_aliases \
						[translit "," " " $optarg]]
			set DELETE 1
		}

		s {
			set retVal [alias_sub_ret_parse $optarg]
			if {[clength [lindex $retVal 0]] == 0 || \
			    [lempty [lindex $retVal 1]]} {
				puts stderr [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_USAGE]
				exit 1
			}

			lappend subscribe_users $retVal
			set SUBSCRIBE 1
		}

		S {
			set userList [alias_recipients_parse $optarg]
			foreach user $userList {
				lappend subscribe_users [list $user {}]
			}
			set SUBSCRIBE 1
		}

		r {
			set retVal [alias_sub_ret_parse $optarg]
			if {[clength [lindex $retVal 0]] == 0 || \
			    [lempty [lindex $retVal 1]]} {
				puts stderr [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_USAGE]
				exit 1
			}

			lappend retire_users $retVal
			set RETIRE 1
		}

		R {
			set userList [alias_recipients_parse $optarg]
			foreach user $userList {
				lappend retire_users [list $user {}]
			}
			set RETIRE 1
		}

@if test
# special test commands
		t {
			set first [ctoken optarg " "]
			if {"$first" == "shortcut"} {
				lappend shortcut [string trim $optarg]
				set DO_SHORTCUT 1
			} else {
				set TEST $first
			}
		}
@endif
		? {
			puts stderr [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_USAGE]
			exit 1
		}
		}
	}

	# process commandline arguments
	set optCount [expr "$argc - $optind"]

	if {$optCount > 0} {
		puts stderr [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_USAGE]
		exit 1
	}

	if {"$MA_ALIAS_FILENAME" == ""} {
		puts stderr [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_USAGE]
		exit 1
	}

@if test
	if {$ADD || $DELETE || $RETIRE || $SUBSCRIBE || $DO_SHORTCUT} {
@else
	if {$ADD || $DELETE || $RETIRE || $SUBSCRIBE} {
@endif

		if {[file isdirectory $MA_ALIAS_FILENAME]} {
			puts stderr [IntlLocalizeMsg \
				SCO_MAIL_ADMIN_ALIAS_ERR_DIRECTORY $MA_ALIAS_FILENAME]
			puts stderr [IntlLocalizeMsg \
				SCO_MAIL_ADMIN_ALIAS_ERR_USAGE]
			exit 1
		}

		if {[lsearch -exact $MA_ALIAS_DATABASE_CLASSES \
			$MA_ALIAS_CLASS] == -1} {
			puts stderr [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_UNKNOWN_CLASS $MA_ALIAS_CLASS]
			puts stderr [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_USAGE]
			exit 1
		}
			
		alias_load

		foreach newAlias $add_aliases {
			set aliasName [lindex $newAlias 0]
			set aliasRecipients [lindex $newAlias 1]
			Alias:Set $MA_ALIAS_FILEID $aliasName $aliasRecipients
		}

		foreach delAlias $delete_aliases {
			Alias:Unset $MA_ALIAS_FILEID $delAlias
		}

		foreach subscriber $subscribe_users {
			set userName [lindex $subscriber 0]
			set aliasNames [lindex $subscriber 1]
			userSubscribe $userName $aliasNames
		}
			
		foreach retiree $retire_users {
			set userName [lindex $retiree 0]
			set aliasNames [lindex $retiree 1]
			userRemove $userName $aliasNames
		}

@if test
		foreach command $shortcut {
			if {[catch {eval $command} ret] != 0} {
				echo ERROR: $ret
			} else {
				echo $ret
			}
		}
@endif
		if {[Alias:Modified $MA_ALIAS_FILEID]} {
			
			if {[ErrorCatch errStack 0 \
				{Alias:Close $MA_ALIAS_FILEID} errmsg] != 0} {
				puts stderr [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_CANT_SAVE $MA_ALIAS_FILENAME]
				puts stderr $errmsg
				exit 1
        		}

			if {[cindex $MA_ALIAS_FILENAME 0] != "/"} {
				set MA_ALIAS_FILENAME "[pwd]/${MA_ALIAS_FILENAME}"
			}

@if test
			if {"$TEST" == "aliases_cmd_error_newaliases" || \
			    "$TEST" == "aliases_gui_error_newaliases"} {
				set MA_ALIAS_SENDMAILCF "/tmp/sendmail.cf"
			}
@endif
			if {$MA_ALIAS_CLASS == "NONE"} {
				set cmd  "exec $MA_ALIAS_NEWALIASES -oA${MA_ALIAS_FILENAME} -C${MA_ALIAS_SENDMAILCF} >& /dev/null"
			} else {
				set cmd "exec $MA_ALIAS_NEWALIASES -oA${MA_ALIAS_CLASS}:${MA_ALIAS_FILENAME} -C${MA_ALIAS_SENDMAILCF} >& /dev/null"
			}
				
			if {[catch $cmd errmsg] != 0} {
				puts stderr [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_NEWALIASES "${MA_ALIAS_FILENAME}.db"]
				puts stderr $errmsg
				exit 1
			}
		}

		exit 0
	}

	set app [VtOpen aliases [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELPBOOK]]
	VtSetAppValues $app\
		-errorCallback {SaUnexpectedErrorCB mainErrorCB}
	set isCharm [VtInfo -charm]

	set mainForm\
		[VtFormDialog $app.form1\
			-marginWidth 0\
			-title [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_TITLE $MA_ALIAS_FILENAME]\
			-wmCloseCallback CloseCB\
			-wmShadowThickness 0\
			-autoLock {CloseCB aliasError}]
	SaCharmSetMaxFormDimensions $mainForm

	#-----

	set menubar\
		[VtMenuBar $mainForm.menubar\
			-helpMenuItemList [list \
			ON_CONTEXT \
			ON_WINDOW \
			ON_KEYS \
			INDEX \
			ON_HELP]]

	set aliasesMenu\
		[VtPulldown $menubar.aliases\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MENU_LABEL]\
			-mnemonic [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MENU_MNEM]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_ALIASES]]
	  set alAdd\
		 [VtPushButton $aliasesMenu.alAdd\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MENU_ADD_LABEL]\
			-mnemonic [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MENU_ADD_MNEM]\
			-callback {aliasAdd add}\
			-autoLock aliasAdd\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
			   [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_ALIASES_ADD]]
	  set alRmv\
		[VtPushButton $aliasesMenu.alRmv\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MENU_REMOVE_LABEL]\
			-mnemonic [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MENU_REMOVE_MNEM]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
			   [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_ALIASES_RMV]\
			-callback aliasRemove\
			-autoLock aliasRemove]
	   VtSetSensitive alRmv 0
	  set alMfy\
		[VtPushButton $aliasesMenu.alMfy\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MENU_MODIFY_LABEL]\
			-mnemonic [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MENU_MODIFY_MNEM]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
			  [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_ALIASES_MFY]\
			-callback {aliasAdd modify}\
			-autoLock aliasAdd]
	   VtSetSensitive alMfy 0
	  VtSeparator $aliasesMenu._1
	  set alSave\
		[VtPushButton $aliasesMenu.save\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MENU_SAVE_LABEL]\
			-mnemonic [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MENU_SAVE_MNEM]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
			  [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_ALIASES_SAVE]\
			-callback SaveCB\
			-autoLock SaveCB]
	  VtSeparator $aliasesMenu._2
	 set alQuit\
		[VtPushButton $aliasesMenu.quit\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MENU_EXIT_LABEL]\
			-mnemonic [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MENU_EXIT_MNEM]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
			  [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_ALIASES_QUIT]\
			-callback CloseCB\
			-autoLock CloseCB]

	set membersMenu\
		[VtPulldown $menubar.members\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MEMB_MENU_LABEL]\
			-mnemonic [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MEMB_MENU_MNEM]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_MEMBERS]]
	  set meAdd\
		[VtPushButton $membersMenu.meAdd\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MEMB_MENU_ADD]\
			-mnemonic [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MEMB_MENU_ADD_MNEM]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
			  [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_MEMBERS_ADD]\
			-callback memberAdd\
			-autoLock memberAdd]
	  VtSetSensitive $meAdd 0
	  set meRmv\
		[VtPushButton $membersMenu.meRmv\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MEMB_MENU_REMOVE]\
			-mnemonic [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MEMB_MENU_REMOVE_MNEM]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
			  [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_MEMBERS_RMV]\
			-callback memberRemove\
			-autoLock memberRemove]
	  VtSetSensitive $meRmv 0

	set usersMenu\
		[VtPulldown $menubar.users\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USERS_MENU_LABEL]\
			-mnemonic [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USERS_MENU_MNEM]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_USERS]]
	  set usSub\
		[VtPushButton $usersMenu.usSub\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USERS_MENU_SUB]\
			-mnemonic [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USERS_MENU_SUB_MNEM]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_USERS_SUB]\
			-callback {UserSelectCB usersSubscribeCB}\
			-autoLock UserSelectCB]
	  VtSetSensitive $usSub 0
	  set usRet\
		[VtPushButton $usersMenu.usRet\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USERS_MENU_RET]\
			-mnemonic [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_USERS_MENU_RET_MNEM]\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MENU_USERS_RET]\
			-callback {UserSelectCB usersRetireCB}\
			-autoLock UserSelectCB]
	  VtSetSensitive $usRet 0

	#-----

	VtLabel $mainForm.aliasLabel\
		-MOTIF_leftOffset 10\
		-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_LIST_TITLE]
	set aliasListForm\
		[VtForm $mainForm.aliasForm\
			-topOffset 0\
			-MOTIF_leftOffset 10\
			-leftSide FORM\
			-borderWidth 1]
	set aliasList\
		[VtList $aliasListForm.aliasList\
			-rightSide FORM\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString [IntlLocalizeMsg \
				SCO_MAIL_ADMIN_ALIAS_MSG_HELP_ALIASES_LIST \
				$MA_ALIAS_FILENAME]\
			-rows 10\
			-scrollBar 1\
			-callback {AliasCB 1}\
			-autoLock AliasCB\
			-defaultCallback {aliasAdd modify}\
			-autoLock aliasAdd]
	VxSetVar $aliasList "callback" "AliasCB"
	set aliasSearch\
		[VxText $aliasListForm.aliasSearch\
			-topSide NONE\
			-leftSide FORM\
			-rightSide FORM\
			-bottomSide FORM\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_SEARCH]\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_SEARCH]\
			-valueChangedCallback [list aliasJumpCB $aliasList]\
			-autoLock aliasJumpCB]
	set aliasSearchForm [VxGetVar $aliasSearch "form"]
	VtSetValues $aliasList -bottomSide $aliasSearchForm

	set members {}
	set memberListForm\
		[VtForm $mainForm.memberListForm\
			-topSide $menubar\
			-leftSide $aliasListForm\
			-MOTIF_leftOffset 15\
			-rightSide FORM\
			-MOTIF_rightOffset 10\
			-marginWidth 0\
			-marginHeight 0]
	VtLabel $memberListForm.memberLabel\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_MEMBERS_TITLE]
	set memberListInnerForm\
		[VtForm $memberListForm.memberInnerForm\
			-topOffset 0\
			-borderWidth 1\
			-rightSide FORM\
			-bottomSide FORM]
	set memberList\
		[VtList $memberListInnerForm.memberList\
			-rightSide FORM\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_MEMBERS_LIST]\
			-itemList $members\
			-MOTIF_selection EXTENDED\
			-CHARM_selection MULTIPLE\
			-rows 10\
			-scrollBar 1\
			-MOTIF_columns 25\
			-CHARM_columns 20\
			-callback {MemberCB 1}\
			-autoLock MemberCB\
			-defaultCallback {aliasAdd modify}]
	VxSetVar $memberList "callback" "MemberCB"
	set membSearch\
		 [VxText $memberListInnerForm.membSearch\
			-topSide NONE\
			-leftSide FORM\
			-rightSide FORM\
			-bottomSide FORM\
			-shortHelpCallback SaShortHelpCB\
			-shortHelpString\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_HELP_SEARCH]\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_SEARCH]\
			-columns 5\
			-valueChangedCallback [list aliasJumpCB $memberList]\
			-autoLock aliasJumpCB]
	set membSearchForm [VxGetVar $membSearch "form"]
	VtSetValues $memberList -bottomSide $membSearchForm

	set statusBar [SaStatusBar $mainForm.StatusBar 1]
	VtSetValues $aliasListForm \
		-bottomSide $statusBar \
		-MOTIF_bottomOffset 10
	VtSetValues $memberListForm \
		-bottomSide $statusBar \
		-MOTIF_bottomOffset 10

	VtShow $mainForm

	if {[lsearch -exact $MA_ALIAS_DATABASE_CLASSES $MA_ALIAS_CLASS] == -1} {
		set errorDlg [VtErrorDialog $mainForm.error1 -ok \
			-message "[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_UNKNOWN_CLASS $MA_ALIAS_CLASS]\n[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_ERR_USAGE]"\
			-okCallback quitCB \
			-autoLock quitCB]
		VtShow $errorDlg
	} elseif {[file exists $MA_ALIAS_FILENAME] && \
		  [file isdirectory $MA_ALIAS_FILENAME]} {
		set errorDlg [VtErrorDialog $mainForm.error2 -ok \
			-message [IntlLocalizeMsg \
				SCO_MAIL_ADMIN_ALIAS_ERR_DIRECTORY \
				$MA_ALIAS_FILENAME] \
			-okCallback quitCB \
			-autoLock quitCB]
		VtShow $errorDlg
	} elseif {! [file exists $MA_ALIAS_FILENAME]} {
		set dialog [VtInformationDialog $mainForm.exist \
			-message [IntlLocalizeMsg \
				SCO_MAIL_ADMIN_ALIAS_MSG_FILE_DOESNT_EXIST] \
			-ok]
		VtShow $dialog
		alias_load
	} else {
		MyStatusBox $mainForm.loadbox\
			[IntlLocalizeMsg SCO_MAIL_ADMIN_ALIAS_MSG_READING]
		set loadTime [time alias_load]
		set redrawTime [time [list aliasRedrawList ""]]
		set totalTime \
			[expr [ctoken loadTime " "] + [ctoken redrawTime " "]]
		if {$totalTime < 2000000} {
			sleep 2
		}
		MyStatusBox $mainForm.loadbox -close
	}


	#-----
	VtMainLoop

} "Alias Client"
