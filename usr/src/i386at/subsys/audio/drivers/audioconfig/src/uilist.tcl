#==============================================================================
# Audio Configuration Manager
#------------------------------------------------------------------------------
# uilist.tcl
#------------------------------------------------------------------------------
# @(#)uilist.tcl	5.1	97/06/25
#------------------------------------------------------------------------------
# User Interface: List, binding to SCO Visual Tcl
#------------------------------------------------------------------------------
# Revision History:
# 1996-Dec-18, shawnm, add main list functions
# 1996-Oct-08, shawnm, main screen prototype
# 1996-Sep-27, shawnm, created from template
#==============================================================================

#====================================================================== XXX ===
# UiListSetItems
#   
# Set items in a list 
#------------------------------------------------------------------------------
proc UiListSetItems {list items} {

	VtDrawnListDeleteItem $list -all
	VtDrawnListAddItem $list -recordList $items

}

#====================================================================== XXX ===
# UiListSaveSelectedItems
#   
# Stash selected items in the list widget in preparation for a refresh
#------------------------------------------------------------------------------
proc UiListSaveSelectedItems {list} {

	set selected [VtDrawnListGetSelectedItem $list -byRecordList]
	VxSetVar $list selected $selected
}

#====================================================================== XXX ===
# UiListRestoreSelectedItems
#   
# Retrieve previously selected items and if they still exist in the
# new set of list data, select them again
#------------------------------------------------------------------------------
proc UiListRestoreSelectedItems {list} {

	set selected [VxGetVar $list selected ]
	if {[lempty $selected]} {
		# default selection policy??
		return
	}
	set items [VtDrawnListGetItem $list -all]
	# tough job using vtcl
	set positions {}
	foreach item $selected {
		if {[set position [lsearch $items $item]] != -1} {
			incr position
			lappend positions $position
		}
	}
	if {![lempty $positions]} {
		VtDrawnListSelectItem $list -positionList $positions
	}
}


#------------------------------------------------------------------------------
# MainList Functions
#------------------------------------------------------------------------------

proc UiMainListSetItems {items} {
	global appvals

	set mainList [VxGetVar $appvals(vtMain) mainList]
	if {$appvals(deviceCount) > 0} {
		UiListSetItems $mainList $items
		UiMainListSetRows $appvals(deviceCount)
		VtDrawnListSelectItem $mainList -position 1
	} else {
		UiListSetItems $mainList [MainListEntryNone]
		UiMainListSetRows 1
	}
	UiSensitizeFunctions
}


proc UiMainListGetItems {} {
	global appvals

	return [VtDrawnListGetItem [VxGetVar $appvals(vtMain) mainList] -all]
}


proc UiMainListAddItem {item} {
	global appvals

	set mainList [VxGetVar $appvals(vtMain) mainList]
	if {$appvals(deviceCount) == 0} {
		VtDrawnListDeleteItem $mainList -all
	}
	VtDrawnListAddItem $mainList -fieldList $item
	incr appvals(deviceCount)
	UiMainListSetRows $appvals(deviceCount)
	VtDrawnListSelectItem $mainList -position 0
	UiSensitizeFunctions
}


proc UiMainListReplaceItem {item} {
	global appvals

	set mainList [VxGetVar $appvals(vtMain) mainList]
	set unit [MainListEntryGetUnit $item]
	VtDrawnListDeleteItem $mainList -field 0 $unit
	VtDrawnListAddItem $mainList -fieldList $item
	VtDrawnListSelectItem $mainList -position 0
	UiSensitizeFunctions
}


proc UiMainListDeleteItem {item} {
	global appvals

	set mainList [VxGetVar $appvals(vtMain) mainList]
	VtDrawnListDeleteItem $mainList -positionList $item
	incr appvals(deviceCount) -1
	if {$appvals(deviceCount) > 0} {
		UiMainListSetRows $appvals(deviceCount)
		VtDrawnListSelectItem $mainList -position 1
	} else {
		UiListSetItems $mainList [MainListEntryNone]
		UiMainListSetRows 1
	}
	UiSensitizeFunctions
}


proc UiMainListGetSelected {} {
	global appvals

	set mainList [VxGetVar $appvals(vtMain) mainList]
	return [lindex [VtDrawnListGetSelectedItem $mainList \
			-byRecordList] 0]
}


proc UiMainListGetSelectedName {} {
	set selected [UiMainListGetSelected]
	return "[lindex $selected 1] [lindex $selected 2]"
}


proc UiMainListSetRows {rows} {
	global appvals

	set mainList [VxGetVar $appvals(vtMain) mainList]
	VtSetValues $mainList -rows $rows
}


#====================================================================== XXX ===
# UiMainListCB
#   
#------------------------------------------------------------------------------
proc UiMainListCB {cbs} {
	global appvals

	# update widget sensitivity
	UiSensitizeFunctions
}

#====================================================================== XXX ===
# UiMainListDoubleCB
#   
# double click functionality for main list 
#------------------------------------------------------------------------------
proc UiMainListDoubleCB {cbs} {
	UiExamineCB $cbs
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

	set labelFont medNormalFont
	set listColumns 55

	set formatList { \
		{STRING 2}
		{STRING 20}
		{STRING 30}
		{DATA}
		{DATA}
		{DATA}
		{DATA}
		{DATA}
		{DATA}
		{DATA}
		{DATA}
		}

	set labelList [list \
		[IntlMsg MAINLIST_HEADER_UNIT] \
		[IntlMsg MAINLIST_HEADER_MANUFACTURER] \
		[IntlMsg MAINLIST_HEADER_MODEL] \
		]

        # In current vtcl, need to manually build drawnlist label for CHARM
        if {[VtInfo -charm]} {
                set label [format "  %-2s%-20s%-30s" \
                        [lindex $labelList 0] \
                        [lindex $labelList 1] \
                        [lindex $labelList 2] \
			]
                set header [VtLabel $form.header \
                                -topOffset 1 \
                                -labelLeft \
                                -leftSide FORM \
                                -rightSide FORM \
                                -topSide $top \
                                -bottomSide NONE \
                                -label $label]
                set top $header
        }

	set cmd {VtDrawnList $form.mainList \
                        -labelFormatList $formatList \
                        -labelList $labelList \
                        -formatList $formatList \
			-rows 1 \
			-columns $listColumns \
			-leftSide FORM \
			-rightSide FORM \
			-topSide $top \
			-bottomSide NONE \
			-callback UiMainListCB \
			-defaultCallback UiMainListDoubleCB \
			-autoLock {UiMainListDoubleCB}
			}

	set mainList [eval $cmd]

	VxSetVar $form mainList $mainList

	# Return the bottom-most widget for further attachments
	return $mainList
}

# MainListEntry data access routines

proc MainListEntryNone {} {
	return [list [list 0 [IntlMsg NONE] {} {} {} {} {} {} {} {}]]
}


proc MainListEntryGetUnit {item} {
	return [lindex $item 0]
}


proc MainListEntryGetManufacturer {item} {
	return [lindex $item 1]
}


proc MainListEntryGetModel {item} {
	return [lindex $item 2]
}


proc MainListEntryGetAudioIO {item} {
	return [lindex $item 3]
}


proc MainListEntryGetMidiIO {item} {
	return [lindex $item 4]
}


proc MainListEntryGetSynthIO {item} {
	return [lindex $item 5]
}


proc MainListEntryGetPrimaryIRQ {item} {
	return [lindex $item 6]
}


proc MainListEntryGetSecondaryIRQ {item} {
	return [lindex $item 7]
}


proc MainListEntryGetPrimaryDMA {item} {
	return [lindex $item 8]
}


proc MainListEntryGetSecondaryDMA {item} {
	return [lindex $item 9]
}


proc MainListEntryGetEnabledDrivers {item} {
	return [lindex $item 10]
}


proc ConvertConfigDataToMainListData {configdata} {
	set errorstack {}
	set mainlistdata {}
	foreach card $configdata {
		if {[catch "keylget card unit" unit]} {
			ErrorPush errorstack 1 \
				SCO_AUDIOCONFIG_ERR_MISSING_ATTR \
				{unit ConvertConfigDataToMainListData}
		}
		if {[catch "keylget card manufacturer" mf]} {
			ErrorPush errorstack 1 \
				SCO_AUDIOCONFIG_ERR_MISSING_ATTR \
				{manufacturer ConvertConfigDataToMainListData}
		}
		if {[catch "keylget card model" mo]} {
			ErrorPush errorstack 1 \
				SCO_AUDIOCONFIG_ERR_MISSING_ATTR \
				{model ConvertConfigDataToMainListData}
		}
		if {[catch "keylget card audio_io" aio]} {
			set aio -1
		}
		if {[catch "keylget card midi_io" mio]} {
			set mio -1
		}
		if {[catch "keylget card synth_io" sio]} {
			set sio -1
		}
		if {[catch "keylget card primary_irq" pirq]} {
			set pirq -1
		}
		if {[catch "keylget card secondary_irq" sirq]} {
			set sirq -1
		}
		if {[catch "keylget card primary_dma" pdma]} {
			set pdma -1
		}
		if {[catch "keylget card secondary_dma" sdma]} {
			set sdma -1
		}
		if {[catch "keylget card enabled_drivers" drivers]} {
			set drivers {}
		}
		lappend mainlistdata [list $unit $mf [join $mo " "]\
			$aio $mio $sio $pirq $sirq $pdma $sdma $drivers]
	}
	return $mainlistdata
}

