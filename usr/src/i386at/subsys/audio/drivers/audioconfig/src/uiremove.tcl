#==============================================================================
# Audio Configuration Manager
#------------------------------------------------------------------------------
# uiremove.tcl
#------------------------------------------------------------------------------
# @(#)uiremove.tcl	5.2	97/07/14
#------------------------------------------------------------------------------
# User interface, Remove Dialog
#------------------------------------------------------------------------------
# Revision History:
# 1997-Oct-15, georgec, clean up system files after card removal
# 1997-Sep-16, stevegi, call to RemoveSoundcard now passes Make/Model info
# 1996-Dec-06, shawnm, created
#==============================================================================

proc UiRemoveCB {cbs} {
	global appvals

	set list [VxGetVar $appvals(vtMain) mainList]
	set parent $appvals(vtMain) 
	set selpos [VtDrawnListGetSelectedItem $list -byPositionList]
	set selname [UiMainListGetSelectedName]

	set remove [VtFormDialog $parent.UiRemoveCB \
		-ok -okCallback "UiRemoveOkCB $selpos" \
		-cancel -cancelCallback UiRemoveCancelCB \
		-cancelButton CANCEL \
		-title [IntlMsg REMOVE_DIALOG_TITLE] \
		]
	VtLabel $remove.label \
		-label [IntlMsg REMOVE_DIALOG_LABEL [list "$selname"]]
	VtShow $remove
	VtUnLock
}


proc UiRemoveOkCB {selpos cbs} {
	global appvals

	VtLock
	set w [keylget cbs widget]
	VtHideDialog $w
	SaStatusBarSet [VxGetVar $appvals(vtMain) statusBar] [IntlMsg UPDATING_DRIVERS]
	set selected [UiMainListGetSelected]
	set appvals(pendingUnit) [MainListEntryGetUnit $selected]
	set appvals(pendingManufacturer) \
		[MainListEntryGetManufacturer $selected]
	set appvals(pendingModel) \
		[MainListEntryGetModel $selected]
	RemoveSoundcard $appvals(pendingUnit) \
			$appvals(pendingManufacturer) \
			$appvals(pendingModel)
	UiMainListDeleteItem $selpos
	VtDestroyDialog $w
	SaStatusBarClear [VxGetVar $appvals(vtMain) statusBar]
	VtUnLock
}


proc UiRemoveCancelCB {cbs} {
	set w [keylget cbs widget]
	VtDestroyDialog $w
}

