#===============================================================================
#
#	ident @(#) channel.tcl 11.2 97/11/05 
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
#	Channel module for handling channel table data
#
#
# Modification History
#
# M000, 20-Feb-97, andrean
#	- Created.
#
#===============================================================================

#====================================================================== XXX ===
# ChannelGetData
#   
# Get all channel table file data
#------------------------------------------------------------------------------
proc ChannelGetData {} {
	global appvals

	set data {}
	set maxLength 0
	foreach key [Map:List $appvals(fileid)] {
		set hostName $key
		set hostNameLength [clength $hostName]
		if {$hostNameLength > $maxLength} {
			set maxLength $hostNameLength
		}
		set value [Map:Get $appvals(fileid) $key]
		set realName [lvarpop value]
		keylset entry hostName $hostName
		keylset entry realName $realName
		lappend data $entry
	}
	set mainList [VxGetVar $appvals(vtMain) mainList]
	if {$maxLength < 30} {
		set maxLength 30
	}
	VxSetVar $mainList maxLength $maxLength
	return $data
}

proc ChannelDelete {entries} {
	global appvals

	foreach entry $entries {
		set hostName ""
		keylget entry hostName hostName

		Map:Unset $appvals(fileid) $hostName
	}
}

#
# Insert newEntry after the entry beforeEntry
#
proc ChannelAdd {beforeEntry newEntry} {
	global appvals
	
	set newKey ""
	set newData ""
	keylget newEntry hostName newKey
	keylget newEntry realName newData

	if {$beforeEntry == ""} {
		# the file was empty !
		Map:Set $appvals(fileid) $newKey $newData
	} else {
		set beforeKey ""
		keylget beforeEntry hostName beforeKey

		Map:Insert $appvals(fileid) $newKey $newData $beforeKey
	}
}

proc ChannelModify {oldEntry newEntry} {
	global appvals
	
	keylget newEntry hostName newKey
	keylget newEntry realName newData
	keylget oldEntry hostName oldKey

	if {[cequal $newKey $oldKey]} {
		Map:Set $appvals(fileid) $newKey $newData
	} else {
		Map:Insert $appvals(fileid) $newKey $newData $oldKey
		Map:Unset $appvals(fileid) $oldKey
	}
}

#
# swaps two adjacent entries -- entry1 must be directly before entry2 in order
# prior to the swap
#
proc ChannelSwapPosition {entry1 entry2} {
	global appvals

	set key1 ""
	keylget entry1 hostName key1
	set data1 [Map:Get $appvals(fileid) $key1]

	set key2 ""
	keylget entry2 hostName key2

	Map:Unset $appvals(fileid) $key1
	Map:Insert $appvals(fileid) $key1 $data1 $key2
}

proc ChannelIsModified {} {
	global appvals

	if {$appvals(fileid) != -1} {
		return [Map:Modified $appvals(fileid)]
	} else {
		return 0
	}
}

proc ChannelEntryExists {hostname} {
	global appvals
	
	set allKeys [string tolower [Map:List $appvals(fileid)]]
	if {[lsearch -exact $allKeys [string tolower $hostname]] == -1} {
		return 0
	} else {
		return 1
	}
}
	
proc ChannelSetMainListLabel {maxHostNameLength} {
	global appvals

	set mainList [VxGetVar $appvals(vtMain) mainList]
	
	set labelList [list \
		[IntlMsg COLUMN_HOST_LBL] \
		[IntlMsg COLUMN_FORWARD_LBL]]
	
	if {[VtInfo -charm]} {
		# add 2 for the right margin width
		set fieldLength [expr $maxHostNameLength + 2]
		# For charm, we can't really go beyond 60 chars for the
                # hostname portion of the label
		if {$fieldLength > 60} {
			set fieldLength 60
		}
		set labelFormat "%-${fieldLength}s%-20s"
		set label [format $labelFormat \
                        [lindex $labelList 0] \
                        [lindex $labelList 1]]
		set header [VxGetVar $appvals(vtMain) header]
		VtSetValues $header -label $label
        }

	# adjust host name length, then add 1 for right margin width
	set fieldLength [expr [AdjustFieldLength $maxHostNameLength] + 1]
	set formatList [list \
		[list STRING $fieldLength 5] \
		[list STRING 20]]

	VtSetValues $mainList -labelFormatList $formatList
	VtSetValues $mainList -labelList $labelList
}

#====================================================================== XXX ===
# ChannelBuildMainList
#   
# specific main list format 
#------------------------------------------------------------------------------
proc ChannelBuildMainList {form top} {
	global appvals


	if {[VtInfo -charm]} {
		set labelList [list \
			[IntlMsg COLUMN_HOST_LBL] \
			[IntlMsg COLUMN_FORWARD_LBL]]
		set label [format "%-20s%-20s" \
			[lindex $labelList 0] \
			[lindex $labelList 1]]
		set header [VtLabel $form.header \
				-topOffset 1 \
				-labelLeft \
				-leftSide FORM -leftOffset 1 \
				-rightSide FORM \
				-topSide $top \
				-bottomSide NONE \
				-label $label]
		VxSetVar $form header $header
		set top $header
	}

	set mainList [VtDrawnList $form.mainList \
			-MOTIF_rows 10 -CHARM_rows 25 \
			-MOTIF_columns 60 -CHARM_columns 20 \
			-horizontalScrollBar TRUE \
			-leftSide FORM \
			-MOTIF_leftOffset 5 -CHARM_leftOffset 0 \
			-rightSide FORM \
			-MOTIF_rightOffset 5 -CHARM_rightOffset 0 \
			-topSide $top \
			-MOTIF_topOffset 10 -CHARM_topOffset 0 \
			-bottomSide NONE \
			-selection MULTIPLE \
			-callback ChannelMainListCB \
			-defaultCallback ChannelMainListDoubleCB \
			-autoLock {ChannelMainListDoubleCB}]
	VxSetVar $form mainList $mainList
	ChannelSetMainListLabel 30
	return $mainList
}

#====================================================================== XXX ===
# ChannelMainListCB
#   
# main list callback 
#------------------------------------------------------------------------------
proc ChannelMainListCB {cbs} {
	UiMainListCB $cbs
}

#====================================================================== XXX ===
# ChannelMainListDoubleCB
#   
# who main list double click callback 
#------------------------------------------------------------------------------
proc ChannelMainListDoubleCB {cbs} {
	UiMainListDoubleCB $cbs
}

#====================================================================== XXX ===
# ChannelInit
#   
# Initialize Channel module 
#------------------------------------------------------------------------------
proc ChannelInit {} {
	global appvals

	set appvals(title) 	[IntlMsg TITLE $appvals(filename)]
	set appvals(object) 	[IntlMsg OBJECT]	
	set appvals(objectmn)	[IntlMsg OBJECT_MN]	
	set appvals(itemname)	[IntlMsg ITEMS]	
}

proc ChannelWrite {} {
	global appvals
	global errorInfo
@if test
	global TEST
@endif

	#
	# commit the changes to file
	#

	# force a write since Map:Write will only write if modifications
	# were made
	Map:ForceWrites $appvals(fileid)

	if {[ErrorCatch errStack 0 "Map:Write $appvals(fileid)" result]} {
		ErrorPush errStack 0 [IntlErrId NONSTDCMD] [list $errorInfo]
		ErrorPush errStack 1 [IntlErrId WRITE] $appvals(filename)
	}

@if test
	if {"$TEST" == "channel_gui_error_database"} {
		system "rm -f $appvals(filename)"
	}
@endif
	# compile the database
	set cmd [list exec /etc/mail/makemap hash $appvals(filename) \
			< $appvals(filename) >& /dev/null]

	if {[ErrorCatch errStack 0 $cmd result]} {
		ErrorPush errStack 0 [IntlErrId NONSTDCMD] [list $errorInfo]
		ErrorPush errStack 1 [IntlErrId DATABASE] $appvals(filename).db
	}
}

@if notused
proc ChannelClose {} {
	global appvals

	Map:CloseNoWrite $appvals(fileid)
}
@endif
