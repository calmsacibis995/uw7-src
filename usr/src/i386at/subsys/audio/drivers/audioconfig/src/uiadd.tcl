#==============================================================================
# Audio Configuration Manager
#------------------------------------------------------------------------------
# uidd.tcl
#------------------------------------------------------------------------------
# @(#)uiadd.tcl	5.2	97/07/14
#------------------------------------------------------------------------------
# User interface, Add Dialog
#------------------------------------------------------------------------------
# Revision History:
# 1996-Dec-06, shawnm, created
#==============================================================================

proc UiAddCB {cbs} {
	global appvals

	set appvals(addInProgress) 1

	SaStatusBarSet [VxGetVar $appvals(vtMain) statusBar] [IntlMsg BUILDING_ADD_DIALOG] 

	set parent $appvals(vtMain) 

	set add [VtFormDialog $parent.addDialog \
		-title [IntlMsg ADD_DIALOG_TITLE] \
		]
	set appvals(addDialog) $add
	VxSetVar $appvals(addDialog) addDialog $add
	set manufacturerLabel [VtLabel $add.manufacturerLabel \
		-label [IntlMsg ADD_DIALOG_MANUFACTURER_LABEL]]
	set manufacturerList [VtList $add.manufacturerList \
		-topSide $manufacturerLabel \
		-leftSide FORM \
		-callback ManufacturerListCB \
		-columns 20 -rows 10 \
		-itemList [GetManufacturerList]]
	VtListSelectItem $manufacturerList -position 1
	VxSetVar $appvals(addDialog) manufacturerList $manufacturerList
	set modelLabel [VtLabel $add.modelLabel \
		-topSide FORM \
		-leftSide $manufacturerList \
		-label [IntlMsg ADD_DIALOG_MODEL_LABEL]]
	set manufacturer [lindex [VtListGetSelectedItem $manufacturerList \
		-byItemList] 0]
	set modelList [VtList $add.modelList \
		-topSide $modelLabel \
		-leftSide $manufacturerList \
		-rightSide FORM \
		-callback ModelListCB \
		-alignBottom manufacturerList \
		-columns 30 -rows 10 \
		-itemList [GetModelList $manufacturer]]
	VtListSelectItem $modelList -position 1
	VxSetVar $appvals(addDialog) modelList $modelList

	set rc [VtRowColumn $add.rc\
        	-leftSide FORM  \
        	-rightSide FORM \
        	-topSide $manufacturerList \
        	-bottomSide FORM\
		-vertical \
      		-numColumns 3 \
      		-CHARM_spacing 5 \
        	-MOTIF_spacing 60]
	set addOk [VtPushButton $rc.addOk \
		-takeFocus \
		-label [IntlMsg ADD_DIALOG_ADDOK_LABEL] \
		-labelCenter \
		-callback UiAddOkCB]
	VxSetVar $appvals(addDialog)  addOk $addOk

	set autoDetect [VtPushButton $rc.autoDetect \
		-callback UiAutoDetectCB \
		-label [IntlMsg ADD_DIALOG_AUTODETECT_LABEL]]
	VxSetVar $appvals(addDialog) autoDetect $autoDetect

	set addCancel [VtPushButton $rc.addCancel \
		-callback UiAddCancelCB \
		-labelCenter \
		-label [IntlMsg ADD_DIALOG_CANCEL_LABEL]]
	VxSetVar $appvals(addDialog) addCancel $addCancel

	SaStatusBarClear [VxGetVar $appvals(vtMain) statusBar]
	VtShow $add
	VtUnLock
}


proc ManufacturerListCB {cbs} {
	UpdateModelList [lindex [keylget cbs selectedItemList] 0]
}


proc UpdateModelList {manufacturer} {
	global appvals

	set items [GetModelList $manufacturer]
	set modelList [VxGetVar $appvals(addDialog) modelList]
	VtListSetItem $modelList -itemList $items
	VtListSelectItem $modelList -position 1
}


proc ModelListCB {cbs} {
}

proc UiAutoDetectCB {cbs} {
	global appvals

	VtLock
	set newCardList [GetNewCardList]

	set parent $appvals(addDialog)

	if {[lempty $newCardList]} {
		set newPnP [VtFormDialog $parent.newPnPDialog \
			-title [IntlMsg AD_DIALOG_TITLE] \
			-ok -okCallback UiNewPnPCancelCB \
			]
		set appvals(newPnPDialog) $newPnP
		VxSetVar $appvals(newPnPDialog) newPnPDialog $newPnP
		set newPnPmsg [VtLabel $newPnP.msg \
			-label [IntlMsg AD_DIALOG_NOCARDDETECTED_LABEL]]
	} else {
		set newPnP [VtFormDialog $parent.newPnPDialog \
			-title [IntlMsg AD_DIALOG_TITLE] \
			-ok -okCallback UiNewPnPOkCB \
			-cancel -cancelCallback UiNewPnPCancelCB \
			-cancelButton CANCEL \
			]

		set appvals(newPnPDialog) $newPnP
		VxSetVar $appvals(newPnPDialog) newPnPDialog $newPnP

		set newPnPmsg [VtLabel $newPnP.msg \
			-labelCenter \
			-label [IntlMsg AD_DIALOG_CARDDETECTED_LABEL]]

		set manufacturerLabel [VtLabel $newPnP.manufacturerLabel \
			-label [IntlMsg ADD_DIALOG_MANUFACTURER_LABEL]]
		set manufacturers [keylget newCardList]
		set manufacturerList [VtList $newPnP.manufacturerList \
			-topSide $manufacturerLabel \
			-leftSide FORM \
			-callback ManufacturerListCB \
			-columns 20 -rows 3 \
			-itemList $manufacturers]
		VtListSelectItem $manufacturerList -position 1
		VxSetVar $appvals(newPnPDialog) manufacturerList $manufacturerList
		set modelLabel [VtLabel $newPnP.modelLabel \
			-alignTop $manufacturerLabel \
			-leftSide $manufacturerList \
			-label [IntlMsg ADD_DIALOG_MODEL_LABEL]]
		set models {}
 		for {set index 0} {$index < [llength $newCardList]} {incr index} {
                        lappend models [lindex [lindex $newCardList $index] 1]
                }
		set manufacturer [lindex [VtListGetSelectedItem $manufacturerList \
			-byItemList] 0]		
		set modelList [VtList $newPnP.modelList \
			-topSide $modelLabel \
			-leftSide $manufacturerList \
			-rightSide FORM \
			-callback ModelListCB \
			-alignBottom manufacturerList \
			-columns 30 -rows 3 \
			-itemList $models]
		VtListSelectItem $modelList -position 1
		VxSetVar $appvals(newPnPDialog) modelList $modelList
	}

	VtHideDialog $appvals(addDialog)

	VtShow $newPnP
	VtUnLock
}

proc UiNewPnPOkCB {cbs} {	# Modified from UiAddOkCB
	global appvals

	set appvals(addInProgress) 1

	VtLock
	set manufacturerList [VxGetVar $appvals(newPnPDialog) manufacturerList]
	set manufacturer [lindex [VtListGetSelectedItem $manufacturerList \
		-byItemList] 0]
	set modelList [VxGetVar $appvals(newPnPDialog) modelList]
	set model [lindex [VtListGetSelectedItem $modelList \
		-byItemList] 0]
	# Right now we only allow one soundcard
	# In future this should get a correct unit number
	set appvals(pendingUnit) 0
	set appvals(pendingManufacturer) $manufacturer
	set appvals(pendingModel) $model

	set w [keylget cbs widget]
	VtHideDialog $w

	UiExamineCB $cbs
}

proc UiNewPnPCancelCB {cbs} {
	global appvals

	set appvals(addInProgress) 0

	set w [keylget cbs widget]
	VtDestroyDialog $w
	VtShowDialog $appvals(addDialog)
}

proc UiAddOkCB {cbs} {
	global appvals

	set appvals(addInProgress) 1

	VtLock
	set manufacturerList [VxGetVar $appvals(addDialog) manufacturerList]
	set manufacturer [lindex [VtListGetSelectedItem $manufacturerList \
		-byItemList] 0]
	set modelList [VxGetVar $appvals(addDialog) modelList]
	set model [lindex [VtListGetSelectedItem $modelList \
		-byItemList] 0]
	# Right now we only allow one soundcard
	# In future this should get a correct unit number
	set appvals(pendingUnit) 0
	set appvals(pendingManufacturer) $manufacturer
	set appvals(pendingModel) $model

	set w [keylget cbs widget]
	VtHideDialog $w

	UiExamineCB $cbs
}


proc UiAddCancelCB {cbs} {
	global appvals

	set w [keylget cbs widget]
	VtDestroyDialog $w
	set appvals(addInProgress) 0
}

