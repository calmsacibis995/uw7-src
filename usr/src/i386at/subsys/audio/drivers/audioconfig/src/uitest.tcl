#==============================================================================
# Audio Configuration Manager
#------------------------------------------------------------------------------
# uitest.tcl
#------------------------------------------------------------------------------
# @(#)uitest.tcl	7.1	97/10/22
#------------------------------------------------------------------------------
# User interface, Test Dialog
#------------------------------------------------------------------------------
# Revision History:
# 1996-Dec-06, shawnm, created
#==============================================================================

#====================================================================== XXX ===
# UiTestCB
#   
#------------------------------------------------------------------------------
proc UiTestCB {cbs} {
	global appvals

	set parent $appvals(vtMain) 

	if {!$appvals(addInProgress) && !$appvals(examineInProgress)} {
		set selected [UiMainListGetSelected]
		set appvals(pendingManufacturer) \
			[MainListEntryGetManufacturer $selected]
		set appvals(pendingModel) [MainListEntryGetModel $selected]
	}

	set test [VtFormDialog $parent.testDialog \
		-ok -okCallback UiTestOkCB \
		-title [IntlMsg TEST_DIALOG_TITLE] \
		]

	set label [VtLabel $test.label \
		-label [IntlMsg TEST_DIALOG_LABEL \
		[list "$appvals(pendingManufacturer) $appvals(pendingModel)"]]]

	set sep [VtSeparator $test.sep -horizontal -rightSide FORM]

	set start [VtPushButton $test.startTestButton \
		-label [IntlMsg TEST_DIALOG_STARTTEST] \
		-topSide $sep \
		-leftSide FORM \
		-rightSide 50 \
		-callback TestStartCB]

	set stop [VtPushButton $test.stopTestButton \
		-label [IntlMsg TEST_DIALOG_STOPTEST] \
		-topSide $sep \
		-leftSide 50 \
		-rightSide FORM \
		-callback TestStopCB]

	VtShow $test
	VtUnLock
}


proc TestStartCB {cbs} {
	TestAudioStart
}


proc TestStopCB {cbs} {
	TestAudioStop
}


proc UiTestOkCB {cbs} {
	global appvals

	VtLock
	# destroy the test dialog
	set w [keylget cbs widget]
	VtDestroyDialog $w
	VtUnLock
}

