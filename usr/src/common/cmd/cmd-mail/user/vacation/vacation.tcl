#===============================================================================
#
#	ident @(#) vacation.tcl 11.1 97/10/30 
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
# User vacation admin gui.
#
# Modification History
#
# M000, 14-Apr-97, andrean
#	- Ported OSR vacation client.
#
#===============================================================================

########
# Globals
set HOME $env(HOME)
set rcvtrip_is_on 0
set new_alteregos 0
set asked 0
set want_cleared 0
set rcvtripPattern {^\*[ 	]+-[ 	]+(pipe|\|)[ 	]+R[ 	]+(/usr/bin/|)vacation[ 	]*$}
@if test
set TEST ""
@endif


########
# Main

# DEBUGging
#cmdtrace on [open "%cmdtrace" "w"]
umask 077

proc _quit { cbs } {
	VtClose
	exit
}

proc quit { cbs } {
	global outerMainForm

	VtDestroyDialog $outerMainForm
	_quit $cbs
}

# Now, figure out if rcvtrip is setup (On) or not...
if {[catch {open $HOME/.maildelivery "a+"} fh] == 0} {
	seek $fh 0 start
	set scxt [scancontext create]
		#{^\*[ 	]+-[ 	]+(pipe|\|)[ 	]+R[ 	]+vacation[ 	]*$} {}
	scanmatch $scxt\
		"$rcvtripPattern" {
			global rcvtrip_is_on
			set rcvtrip_is_on 1
		}
	scanfile $scxt $fh
	#close $fh			;# We want to use this to lock the file...
	scancontext delete $scxt
}
if {![flock -write -nowait $fh]} {
	set app [VtOpen vacation [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_HELPBOOK]]
	VtShow\
		[VtErrorDialog $app.eb -ok -help\
			-okCallback _quit\
			-autoLock _quit\
			-message [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_ERR_CANT_LOCK]]
	VtMainLoop
	exit 1
}

# Check if we have any command-line arguments...
set argc [llength $argv]
while {$argc > 0} {
	set arg [lvarpop argv]
	incr argc -1
	if {$arg == "-query"} {
		echo $rcvtrip_is_on
		if {$rcvtrip_is_on} {
			exit 0
		} else {
			exit 1
		}
@if test
	} elseif {$arg == "-t"} {
		set TEST [lvarpop argv]
		incr argc -1
@endif
	} else {
		puts stderr [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_ERR_UNK_ARG $arg]
		exit 1
	}
}


########
# Support routines (non-GUI)

proc read_file_as_list { fileName } {
	set rlist {}

	if {[catch {open $fileName "r"} fh] == 0} {
		while {[gets $fh line] != -1} {
			lappend rlist $line
		}

		close $fh
	}

	return $rlist
}

########


########
# GUI support routines and callbacks

proc texterr { errmsg } {
	global mainForm

	VtUnLock
	set errd\
		[VtErrorDialog $mainForm.err\
			-ok\
			-message $errmsg]
	VtShow $errd
}

proc cantread { name quit } {
	global mainForm

	VtUnLock
	set errd\
		[VtErrorDialog $mainForm.err\
			-ok -help\
			-message\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_ERR_CANT_READ $name]]
	if {$quit} {
		VtSetValues $errd -okCallback quit -autoLock quit
	}
	VtShow $errd
}

proc cantwrite { name } {
	global mainForm

	VtUnLock
	VtShow\
		[VtErrorDialog $mainForm.err\
			-ok -help\
			-message\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_ERR_CANT_WRITE $name]]
}

proc cantremove { name } {
	global mainForm

	VtUnLock
	VtShow\
		[VtErrorDialog $mainForm.err\
			-ok -help\
			-message\
				[IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_ERR_CANT_REMOVE $name]]
}

proc ask_want_cleared_CB { what cbs } {
	global	HOME asked
@if test
	global TEST
@endif

	set parent [keylget cbs "dialog"]
	VtDestroyDialog $parent

	if {$what == "yes"} {
@if test
		if {"$TEST" == "vacation_gui_error_ask"} {
			system "rm -f $HOME/triplog"
		}
@endif
		if {[catch {unlink $HOME/triplog} err] != 0} {
			cantremove $HOME/triplog
			VtUnLock
			return
		}
	}

	if {$what != "cancel"} {
		set asked 1
		vacDoneCB "OK" {} 0
	}

	VtUnLock
}

proc ask_want_cleared {} {
	global mainForm

	set form\
		[VtFormDialog $mainForm.wantCleared\
			-title [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_WANT_CLR_TITLE]\
			-help\
			-okLabel [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_YES]\
			-okCallback {ask_want_cleared_CB yes}\
			-autoLock ask_want_cleared_CB\
			-applyLabel [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_NO]\
			-applyCallback {ask_want_cleared_CB no}\
			-cancelCallback {ask_want_cleared_CB cancel}]
	VtLabel $form.message\
			-rightSide FORM\
			-bottomSide FORM\
			-label [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_WANT_CLEARED]
	VtShow $form
	VtUnLock
}

proc clearCB { cbs } {
	global noticesList clearPB want_cleared

	set want_cleared 1
	VtListDeleteItem $noticesList -all
	VtSetSensitive $clearPB 0
	VtUnLock
}

proc subjectDefaultCB { cbs } {
	global noticeSubject

	VtSetValues $noticeSubject -value [IntlLocalizeMsg \
		SCO_MAIL_ADMIN_VACATION_MSG_TRIPSUBJECT]
	VtUnLock
}

proc messageDefaultCB { cbs } {
	global noticeMessage

	VtSetValues $noticeMessage -value [IntlLocalizeMsg \
		SCO_MAIL_ADMIN_VACATION_MSG_TRIPNOTE [id user]]
	VtUnLock
}

proc vacDoneCB { what cbs {doUnlock 1} } {
	global HOME orig_msgtext orig_subjtext rcvtripPattern
	global mainForm noticeSubject noticeMessage noticesOn clearPB akaList 
	global rcvtrip_is_on new_alteregos want_cleared asked
@if test
	global TEST
@endif

	if {$what == "OK"} {
		set onOff [VtGetValues $noticesOn -set]

	  ## First, check that tripsubject and tripnote aren't empty
		set subjtext [VtGetValues $noticeSubject -value]
		if {$subjtext == "" || [ctype space $subjtext]} {
			texterr [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_ERR_SUBJECT]
			VtSetFocus $noticeSubject
			if {$doUnlock} {
				VtUnLock
			}
			return
		}
			
		set msgtext [VtGetValues $noticeMessage -value]
		if {$msgtext == "" || [ctype space $msgtext]} {
			texterr [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_ERR_MESSAGE]
			VtSetFocus $noticeMessage
			if {$doUnlock} {
				VtUnLock
			}
			return
		}

	  ## Next, if requested, nuke the triplog file
		if {$want_cleared && [file exists $HOME/triplog]} {
@if test
			if {"$TEST" == "vacation_gui_error_triplog"} {
				system "rm -f $HOME/triplog"
			}
@endif
			if {[catch {unlink $HOME/triplog} err] != 0} {
				cantremove $HOME/triplog
				if {$doUnlock} {
					VtUnLock
				}
				return
			}
		}

		if {!$asked && \
		    $onOff != $rcvtrip_is_on && \
		    [file exists $HOME/triplog] && \
		    [file size $HOME/triplog] > 0} {
			ask_want_cleared
			if {$doUnlock} {
				VtUnLock
			}
			return
		}

	  ## Next, save the tripsubject text (if it's changed)
		if {$subjtext != $orig_subjtext} {
@if test
			if {"$TEST" == "vacation_gui_error_tripsubject"} {
				chmod 0444 $HOME/tripsubject
			}
@endif
			if {[catch {write_file $HOME/tripsubject $subjtext} err] != 0} {
				cantwrite $HOME/tripsubject
				if {$doUnlock} {
					VtUnLock
				}
				return
			}
		}

	  ## Next, save the tripnote text (if it's changed)
		if {$msgtext != $orig_msgtext} {
@if test
			if {"$TEST" == "vacation_gui_error_tripnote"} {
				chmod 0444 $HOME/tripnote
			}
@endif
			if {[catch {write_file $HOME/tripnote $msgtext} err] != 0} {
				cantwrite $HOME/tripnote
				if {$doUnlock} {
					VtUnLock
				}
				return
			}
		}

	  ## Next, save the new alter-egos list (if there is one)
		if {$new_alteregos} {
@if test
			if {"$TEST" == "vacation_gui_error_alteregos"} {
				chmod 0444 $HOME/.alter_egos
			}
@endif
			if {[catch {open $HOME/.alter_egos "w"} fh] != 0} {
				cantwrite $HOME/.alter_egos
				if {$doUnlock} {
					VtUnLock
				}
				return
			}

			set user [id user]
			set egoList [VtListGetItem $akaList -all]
			if {[lsearch $egoList $user] == -1} {
				lappend egoList $user
			}
			foreach ego [lsort $egoList] {
				puts $fh $ego
			}
			close $fh
		}


	  ## Finally, turn rcvtrip ON or OFF by editing $HOME/.maildelivery
		if {$onOff && !$rcvtrip_is_on} {
		  ## Create an empty triplog file if needed
			if {$onOff && ![file exists $HOME/triplog]} {
				# If we're turning notices ON,
				# the triplog file must exist
@if test
				if {"$TEST" == "vacation_gui_error_triplogon"} {
					chmod 0444 $HOME
				}
@endif
				if {[catch {open $HOME/triplog "w"} fh] != 0} {
					cantwrite $HOME/triplog
					if {$doUnlock} {
						VtUnLock
					}
					return
				}
				close $fh
			}
@if test
			if {"$TEST" == "vacation_gui_error_deliveryon"} {
				chmod 0444 $HOME/.maildelivery
			}
@endif
			if {[catch {open $HOME/.maildelivery "a"} fh] != 0} {
				cantwrite $HOME/.maildelivery
				if {$doUnlock} {
					VtUnLock
				}
				return
			}

			puts $fh {*	-	pipe	R	vacation}
			close $fh
		} elseif {!$onOff && $rcvtrip_is_on} {
@if test
			if {"$TEST" == "vacation_gui_error_offread"} {
				chmod 0000 $HOME/.maildelivery
			}
@endif
			if {[catch {open $HOME/.maildelivery "r"} fh] != 0} {
				cantread $HOME/.maildelivery 0
				if {$doUnlock} {
					VtUnLock
				}
				return
			}
			close $fh
			set mailDelLines [read_file_as_list $HOME/.maildelivery]
@if test
			if {"$TEST" == "vacation_gui_error_offwrite"} {
				chmod 0444 $HOME/.maildelivery
			}
@endif
			if {[catch {open $HOME/.maildelivery "w"} fh] != 0} {
				cantwrite $HOME/.maildelivery
				if {$doUnlock} {
					VtUnLock
				}
				return
			}

			foreach line $mailDelLines {
				if {![regexp "$rcvtripPattern" $line]} {
					puts $fh $line
				}
			}
			close $fh
		}
	}

	quit {}
}

proc akaAddDone { isOK doClose cbs } {
	global	akaList akaRemove new_alteregos

	set parent [keylget cbs "dialog"]
	set address [VxGetVar $parent "address"]

	set mailAddress [string trim [VtGetValues $address -value]]
	if {$isOK && $mailAddress != ""} {
		if {[regexp {[ 	]} $mailAddress]} {
			texterr [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_ERR_SPACE]
			VtSetFocus $address
			VtUnLock
			return
		}
			
		set currentList [VtListGetItem $akaList -all]
		if {[lsearch -exact $currentList $mailAddress] == -1} {
			VtListAddItem $akaList -item $mailAddress
			if {[lempty $currentList]} {
				VtSetSensitive $akaRemove 1
			}
		}
		VtListSelectItem $akaList -item $mailAddress
		SaListShowSelectedItem $akaList
		set new_alteregos 1
	}

	if {$doClose} {
		VtDestroyDialog $parent
	} else {
		VtSetValues $address -value ""
	}

	VtUnLock
}

proc akaAddCB { cbs } {
	set parent [keylget cbs "dialog"]
	set akaAddForm\
		[VtFormDialog $parent.AddForm\
			-help\
			-autoLock akaAddDone\
			-okCallback {akaAddDone 1 1}\
			-cancelCallback {akaAddDone 0 1}\
			-wmCloseCallback {akaAddDone 0 1}\
			-title [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_AKA_ADD_TITLE]]

	set address\
		[VxText $akaAddForm.address\
			-rightSide FORM\
			-bottomSide FORM\
			-title [IntlLocalizeMsg\
						SCO_MAIL_ADMIN_VACATION_MSG_AKA_ADD_ADDRESS [id user]]\
			-callback {SaSetFocus next}]
	VxSetVar $akaAddForm "address" $address

	set okButton [VtGetValues $akaAddForm -ok]
	SaSetFocusList $akaAddForm [list\
		$address\
		$okButton]

	VtShow $akaAddForm
	VtSetFocus $address
	VtUnLock
}

proc akaRemoveCB { cbs } {
	global	akaList akaRemove new_alteregos

	set parent [keylget cbs "dialog"]
	set posList [VtListGetSelectedItem $akaList -byPositionList]
	if {$posList != 0} {
		set prevTotal [llength [VtListGetItem $akaList -all]]
		VtListDeleteItem $akaList -positionList $posList
		set new_alteregos 1
		set numLeft [llength [VtListGetItem $akaList -all]]
		if {$numLeft > 0} {
			# the list has selection BROWSE, so only one was deleted
			set deleted [lindex $posList end]
			if {$deleted == $prevTotal} {
				VtListSelectItem $akaList -position 0
			} else {
				VtListSelectItem $akaList -position $deleted
			}
		} else {
			VtSetSensitive $akaRemove 0
		}
	}

	VtUnLock
}

proc mainErrorCB { cbs } {
	VtClose
	exit 255
}


########
# Main form

ErrorTopLevelCatch {
set app [VtOpen vacation [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_HELPBOOK]]

VtSetAppValues $app -errorCallback {SaUnexpectedErrorCB mainErrorCB}

set outerMainForm\
	[VtFormDialog $app.main\
		-marginWidth 0\
		-marginHeight 0\
		-help\
		-okCallback { vacDoneCB OK }\
		-autoLock vacDoneCB\
		-cancelCallback { vacDoneCB Cancel }\
		-wmCloseCallback { vacDoneCB Cancel }\
		-title [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_TITLE]]
set mainForm [VtForm $outerMainForm.mainForm -rightSide FORM]

if {$rcvtrip_is_on} {
	set on 1
	set off 0
} else {
	set on 0
	set off 1
}
set onOffForm\
	[VtForm $mainForm.onOffForm\
		-topSide FORM\
		-MOTIF_topOffset 10\
		-leftSide FORM\
		-borderWidth 1]
set onOffLabel\
	[VtLabel $onOffForm.onOffLabel\
		-topSide FORM\
		-rightSide FORM\
		-MOTIF_rightOffset 10\
		-label [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_SET]\
		-labelLeft]
set onOff\
	[VtRadioBox $onOffForm.onOff\
		-topSide $onOffLabel\
		-leftSide FORM\
		-rightSide FORM\
		-bottomSide FORM\
		-MOTIF_bottomOffset 7\
		-horizontal]

set noticesOn\
	[VtToggleButton $onOff.On\
		-set $on\
		-label [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_NOTICE_ON]]
set noticesOff\
	[VtToggleButton $onOff.Off\
		-set $off\
		-label [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_NOTICE_OFF]]

set akaLabel\
	[VtLabel $mainForm.akaLabel\
		-leftSide $onOffForm\
		-MOTIF_leftOffset 20 -CHARM_leftOffset 1\
		-topSide FORM\
		-label [IntlLocalizeMsg\
					SCO_MAIL_ADMIN_VACATION_MSG_AKA_TITLE [id user]]]
set akaForm [VtForm $mainForm.aka\
		-alignLeft $akaLabel\
		-topSide $akaLabel\
		-topOffset 0\
		-rightSide FORM\
		-marginWidth 0 -marginHeight 0]
set akaRC\
	[VtRowColumn $akaForm.rc\
		-leftSide NONE\
		-rightSide FORM\
		-topSide FORM\
		-CHARM_topOffset 0\
		-bottomSide FORM\
		-vertical]
set akaAdd\
	[VtPushButton $akaRC.Add\
		-label [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_AKA_ADD]\
		-shortHelpCallback SaShortHelpCB\
		-shortHelpString\
			[IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_HELP_AKA_ADD]\
		-callback akaAddCB\
		-autoLock akaAddCB]
set akaRemove\
	[VtPushButton $akaRC.Remove\
		-label [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_AKA_REMOVE]\
		-shortHelpCallback SaShortHelpCB\
		-shortHelpString\
			[IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_HELP_AKA_REMOVE]\
		-callback akaRemoveCB\
		-autoLock akaRemoveCB]
set list [read_file_as_list $HOME/.alter_egos]
set akaList\
	[VtList $akaForm.List\
		-leftSide FORM\
		-rightSide $akaRC\
		-topSide FORM -MOTIF_topOffset 0 -CHARM_topOffset 0\
		-bottomSide FORM\
		-shortHelpCallback SaShortHelpCB\
		-shortHelpString\
			[IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_HELP_AKA_LIST]\
		-MOTIF_rows 3\
		-CHARM_rows 1\
		-itemList $list]
if {[lempty $list]} {
	VtSetSensitive $akaRemove 0
}

VtSeparator $mainForm._ -MOTIF_topOffset 7 -leftSide FORM -rightSide FORM

set noticesLabel\
	[VtLabel $mainForm.noticesL\
		-MOTIF_topOffset 7 -CHARM_topOffset 0\
		-label [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_NOTICE_TITLE]]
set noticesForm\
	[VtForm $mainForm.noticesF\
		-topOffset 0\
		-rightSide FORM\
		-marginWidth 0 -marginHeight 0]
set clearPB\
	[VtPushButton $noticesForm.clearPB\
		-leftSide NONE\
		-rightSide FORM\
		-CHARM_topOffset 2\
		-MOTIF_bottomSide FORM\
		-shortHelpCallback SaShortHelpCB\
		-shortHelpString\
			[IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_HELP_NOTICE_CLEAR]\
		-label [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_NOTICE_CLEAR]\
		-callback clearCB\
		-autoLock clearCB]
set list {}
foreach line [read_file_as_list $HOME/triplog] {
	lappend list [format "%.65s" $line]
}
set noticesList\
	[VtList $noticesForm.List\
		-leftSide FORM\
		-rightSide $clearPB\
		-topSide FORM\
		-bottomSide FORM\
		-shortHelpCallback SaShortHelpCB\
		-shortHelpString\
			[IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_HELP_NOTICE_LIST]\
		-MOTIF_rows 3\
		-CHARM_rows 2\
		-itemList $list]
VtAddTabGroup $clearPB
if {[lempty $list]} {
	VtSetSensitive $clearPB 0
}

set subjtext [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_TRIPSUBJECT]
if {[file exists $HOME/tripsubject]} {
@if test
	if {"$TEST" == "vacation_gui_error_readsubject"} {
		chmod 0000 $HOME/tripsubject
	}
@endif
	if {[catch {read_file -nonewline $HOME/tripsubject} subjtext] != 0} {
		cantread $HOME/tripsubject 1
		VtMainLoop
		exit 1
	}
	set orig_subjtext "$subjtext"
} else {
	set orig_subjtext ""
}

set noticeSubjectLabel\
	[VtLabel $mainForm.subjectLabel\
		-MOTIF_topOffset 7\
		-label [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_SUBJECT]]

set noticeSubjectForm\
	[VtForm $mainForm.subjectForm\
		-topSide $noticeSubjectLabel\
		-topOffset 0\
		-marginHeight 0 -marginWidth 0\
		-leftSide FORM\
		-rightSide FORM]

set noticeSubjectDefault\
	[VtPushButton $noticeSubjectForm.subjectDefault\
		-label [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_DEFAULT]\
		-topSide NONE\
		-bottomSide FORM\
		-leftSide NONE\
		-rightSide FORM\
		-shortHelpCallback SaShortHelpCB\
		-shortHelpString [IntlLocalizeMsg \
			SCO_MAIL_ADMIN_VACATION_MSG_HELP_SUBJECT_DEFAULT]\
		-callback subjectDefaultCB\
		-autoLock subjectDefaultCB]
VtAddTabGroup $noticeSubjectDefault

set noticeSubject\
	[VxText $noticeSubjectForm.subjectText\
		-topSide FORM\
		-topOffset 0\
		-bottomSide NONE\
		-rightSide $noticeSubjectDefault\
		-shortHelpCallback SaShortHelpCB\
		-shortHelpString [IntlLocalizeMsg\
			SCO_MAIL_ADMIN_VACATION_MSG_HELP_SUBJECT]\
		-rows 1\
		-MOTIF_columns 70\
		-CHARM_columns 61\
		-value $subjtext]

set msgtext [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_TRIPNOTE [id user]]
if {[file exists $HOME/tripnote]} {
@if test
	if {"$TEST" == "vacation_gui_error_readmessage"} {
		chmod 0000 $HOME/tripnote
	}
@endif
	if {[catch {read_file -nonewline $HOME/tripnote} msgtext] != 0} {
		cantread $HOME/tripnote 1
		VtMainLoop
		exit 1
	}
	set orig_msgtext "$msgtext"
} else {
	set orig_msgtext ""
}

set noticeMessageLabel\
	[VtLabel $mainForm.messageLabel\
		-topSide $noticeSubjectForm\
		-MOTIF_topOffset 7 -CHARM_topOffset 1\
		-label [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_MESSAGE]]

set noticeMessageForm\
	[VtForm $mainForm.messageForm\
		-topSide $noticeMessageLabel\
		-topOffset 0\
		-marginHeight 0 -marginWidth 0\
		-leftSide FORM\
		-rightSide FORM\
		-bottomSide FORM]

set noticeMessageDefault\
	[VtPushButton $noticeMessageForm.messageDefault\
		-label [IntlLocalizeMsg SCO_MAIL_ADMIN_VACATION_MSG_DEFAULT]\
		-topSide NONE\
		-bottomSide FORM\
		-leftSide NONE\
		-rightSide FORM\
		-shortHelpCallback SaShortHelpCB\
		-shortHelpString [IntlLocalizeMsg \
			SCO_MAIL_ADMIN_VACATION_MSG_HELP_MESSAGE_DEFAULT]\
		-callback messageDefaultCB\
		-autoLock messageDefaultCB]
VtAddTabGroup $noticeMessageDefault

set noticeMessage\
	[VxText $noticeMessageForm.message\
		-topSide FORM\
		-bottomSide FORM\
		-rightSide $noticeMessageDefault\
		-shortHelpCallback SaShortHelpCB\
		-shortHelpString [IntlLocalizeMsg\
			 SCO_MAIL_ADMIN_VACATION_MSG_HELP_MESSAGE]\
		-MOTIF_rows 3\
		-CHARM_rows 2\
		-verticalScrollBar 1\
		-MOTIF_columns 70\
		-CHARM_columns 61\
		-value $msgtext]

set statusBar [SaStatusBar $outerMainForm.StatusBar 0]
set sep [VtSeparator $outerMainForm._\
			-leftSide FORM -rightSide FORM\
			-topSide NONE -bottomSide $statusBar]
VtSetValues $mainForm -bottomSide $sep

VtShow $outerMainForm
VtSetFocus $onOff

VtMainLoop
} "Extended Absense Client"
