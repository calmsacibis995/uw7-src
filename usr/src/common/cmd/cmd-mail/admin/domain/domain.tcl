#===============================================================================
#
#	ident @(#) domain.tcl 11.2 97/11/05 
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
# 	Domain module for handling domain table data
#
#
# Modification History
#
# M000, 20-Feb-97, andrean
#	- Created.
#
#===============================================================================

#====================================================================== XXX ===
# DomainGetData
#   
# Get all domain table file data
#------------------------------------------------------------------------------
proc DomainGetData {} {
	global appvals

	set data {}
	set maxLength 0
	foreach key [Map:List $appvals(fileid)] {
		if {[cindex $key 0] == "."} {
			set partial TRUE
			set hostName [crange $key 1 len]
		} else {
			set partial FALSE
			set hostName $key
		}
		set hostNameLength [clength $hostName]
		if {$hostNameLength > $maxLength} {
			set maxLength $hostNameLength
		}
		set route [Map:Get $appvals(fileid) $key]
		set realName [lvarpop route]
		keylset entry partial $partial
		keylset entry hostName $hostName
		keylset entry realName $realName
		keylset entry route $route
		lappend data $entry
	}
	set mainList [VxGetVar $appvals(vtMain) mainList]
	if {$maxLength < 30} {
		set maxLength 30
	}
	VxSetVar $mainList maxLength $maxLength
	return $data
}

proc DomainDelete {entries} {
	global appvals

	foreach entry $entries {
		set partial ""
		set hostName ""
		keylget entry partial partial
		keylget entry hostName hostName

		if {$partial == "TRUE"} {
			set hostName ".${hostName}"
		} 
		Map:Unset $appvals(fileid) $hostName
	}
}

#
# Insert newEntry after the entry beforeEntry
#
proc DomainAdd {beforeEntry newEntry} {
	global appvals
	
	keylget newEntry partial partial
	keylget newEntry hostName hostName
	keylget newEntry realName realName
	keylget newEntry route route

	if {$partial == "TRUE"} {
		set newKey ".$hostName"
	} else {
		set newKey $hostName
	}
	set newData [concat [string trim $realName] $route]

	if {$beforeEntry == ""} {
		# the file was empty !
		Map:Set $appvals(fileid) $newKey $newData
	} else {
		set beforePartial ""
		set beforeHostName ""
		keylget beforeEntry partial beforePartial
		keylget beforeEntry hostName beforeHostName
		if {$beforePartial == "TRUE"} {
			set beforeKey ".${beforeHostName}"
		} else {
			set beforeKey $beforeHostName
		}

		Map:Insert $appvals(fileid) $newKey $newData $beforeKey
	}
}

proc DomainModify {oldEntry newEntry} {
	global appvals
	
	keylget newEntry partial partial
	keylget newEntry hostName hostName
	keylget newEntry realName realName
	keylget newEntry route route

	if {$partial == "TRUE"} {
		set newKey ".${hostName}"
	} else {
		set newKey $hostName
	}
	set newData [concat [string trim $realName] $route]

	keylget oldEntry partial oldPartial
	keylget oldEntry hostName oldHostName
	if {$oldPartial == "TRUE"} {
		set oldKey ".${oldHostName}"
	} else {
		set oldKey $oldHostName
	}

	if {[cequal $newKey $oldKey]} {
		Map:Set $appvals(fileid) $newKey $newData
	} else {
		Map:Insert $appvals(fileid) $newKey $newData $oldKey
		Map:Unset $appvals(fileid) $oldKey
	}
}

#
# swaps two adjacent entries -- entry1 must be before entry2 before the swap
#
proc DomainSwapPosition {entry1 entry2} {
	global appvals

	set partial1 ""
	set hostName1 ""
	keylget entry1 partial partial1
	keylget entry1 hostName hostName1

	if {$partial1 == "TRUE"} {
		set key1 ".${hostName1}"
	} else {
		set key1 $hostName1
	}
	set data1 [Map:Get $appvals(fileid) $key1]

	set partial2 ""
	set hostName2 ""
	keylget entry2 partial partial2
	keylget entry2 hostName hostName2
	if {$partial2 == "TRUE"} {
		set key2 ".${hostName2}"
	} else {
		set key2 $hostName2
	}

	Map:Unset $appvals(fileid) $key1
	Map:Insert $appvals(fileid) $key1 $data1 $key2
}


proc DomainIsModified {} {
	global appvals

	if {$appvals(fileid) != -1} {
		return [Map:Modified $appvals(fileid)]
	} else {
		return 0
	}
}
	
proc DomainEntryExists {hostname} {
	global appvals

	set allKeys [string tolower [Map:List $appvals(fileid)]]
	if {[lsearch -exact $allKeys [string tolower $hostname]] == -1 && \
	    [lsearch -exact $allKeys [string tolower ".${hostname}"]] == -1} {
		return 0
	} else {
		return 1
	}
}
	
proc DomainSetMainListLabel {maxHostNameLength} {
	global appvals

	set mainList [VxGetVar $appvals(vtMain) mainList]
	
	if {[VtInfo -charm]} {
		set labelList [list \
			[IntlMsg COLUMN_PARTIAL_LBL_CHARM] \
			[IntlMsg COLUMN_HOST_LBL] \
			[IntlMsg COLUMN_ROUTE_LBL]]
	} else {
		set labelList [list \
			[IntlMsg COLUMN_PARTIAL_LBL] \
			[IntlMsg COLUMN_HOST_LBL] \
			[IntlMsg COLUMN_ROUTE_LBL]]
	}
	
	if {[VtInfo -charm]} {
		# add 1 for the right margin width
		set fieldLength [expr $maxHostNameLength + 1]
		# For charm, we can't really go beyond 60 chars for the
		# hostname portion of the label
		if {$fieldLength > 60} {
			set fieldLength 60
		}
		set labelFormat "%-3s%-${fieldLength}s%-20s"
		set label [format $labelFormat \
			[lindex $labelList 0] \
			[lindex $labelList 1] \
			[lindex $labelList 2]]
		set header [VxGetVar $appvals(vtMain) header]
		VtSetValues $header -label $label
	}

	# Try to adjust the length to compensate for variable char widths,
	# and then add 1 for right margin 
	set fieldLength [expr [AdjustFieldLength $maxHostNameLength] + 1]
	VtSetValues $mainList -labelFormatList [list \
		[list STRING 1 14 10] \
		[list STRING $fieldLength] \
		[list STRING 20]]
	VtSetValues $mainList -labelList $labelList
}

#====================================================================== XXX ===
# DomainBuildMainList
#   
# specific main list format 
#------------------------------------------------------------------------------
proc DomainBuildMainList {form top} {
	global appvals

                        
	if {[VtInfo -charm]} {
		set labelList [list \
			[IntlMsg COLUMN_PARTIAL_LBL_CHARM] \
			[IntlMsg COLUMN_HOST_LBL] \
			[IntlMsg COLUMN_ROUTE_LBL]]
		set labelFormat "%-3s%-20s%-20s"
		set label [format $labelFormat \
			[lindex $labelList 0] \
			[lindex $labelList 1] \
			[lindex $labelList 2]]
		set header [VtLabel $form.header \
				-labelLeft \
				-leftSide FORM \
				-leftOffset 3 \
				-rightSide FORM \
				-topSide $top \
				-topOffset 1\
				-bottomSide NONE \
				-label $label]
		VxSetVar $form header $header
		set top $header
	}

	set mainList [VtDrawnList $form.mainList \
			-MOTIF_rows 10 -CHARM_rows 25 \
			-MOTIF_columns 80 -CHARM_columns 20 \
			-horizontalScrollBar TRUE \
			-leftSide FORM \
			-MOTIF_leftOffset 5 -CHARM_leftOffset 0 \
			-rightSide FORM \
			-MOTIF_rightOffset 5 -CHARM_rightOffset 0 \
			-topSide $top \
			-MOTIF_topOffset 10 -CHARM_topOffset 0 \
			-bottomSide NONE \
			-selection MULTIPLE \
			-callback DomainMainListCB \
			-defaultCallback DomainMainListDoubleCB \
			-autoLock {DomainMainListDoubleCB}]
	VxSetVar $form mainList $mainList
	DomainSetMainListLabel 30
	return $mainList
}

#====================================================================== XXX ===
# DomainMainListCB
#   
# main list callback 
#------------------------------------------------------------------------------
proc DomainMainListCB {cbs} {
	UiMainListCB $cbs
}

#====================================================================== XXX ===
# DomainMainListDoubleCB
#   
# who main list double click callback 
#------------------------------------------------------------------------------
proc DomainMainListDoubleCB {cbs} {
	UiMainListDoubleCB $cbs
}

@if notused
#====================================================================== XXX ===
# DomainAuthorized
#   
# Determine if the current user on the current host has authorization s
#------------------------------------------------------------------------------
proc DomainAuthorized {host} {
	# Only root can run the domain manager!
	if {[id effective userid] != 0} {
		return 0
	}
	return 1
}
@endif

#====================================================================== XXX ===
# DomainInit
#   
# Initialize Domain module 
#------------------------------------------------------------------------------
proc DomainInit {} {
	global appvals

	set appvals(title) 	[IntlMsg TITLE $appvals(filename)]
	set appvals(object) 	[IntlMsg OBJECT]	
	set appvals(objectmn)	[IntlMsg OBJECT_MN]	
	set appvals(itemname)	[IntlMsg ITEMS]	
}

proc DomainWrite {} {
	global appvals
	global errorInfo
@if test
	global TEST
@endif

	#
	# commit the changes to file
	#

	# force a write since Map:Write will usually only write if
	# modifcations are made
	Map:ForceWrites $appvals(fileid)

	if {[ErrorCatch errStack 0 "Map:Write $appvals(fileid)" result]} {
		ErrorPush errStack 0 [IntlErrId NONSTDCMD] [list $errorInfo]
		ErrorPush errStack 1 [IntlErrId WRITE] $appvals(filename)
	}

@if test
	if {"$TEST" == "domain_gui_error_database"} {
		 system "rm -f $appvals(filename)"
	}
@endif
	#
	# compile the database
	#
	set cmd [list exec /etc/mail/makemap hash $appvals(filename) < $appvals(filename) >& /dev/null]
	if {[ErrorCatch errStack 0 $cmd result]} {
		ErrorPush errStack 0 [IntlErrId NONSTDCMD] [list $errorInfo]
		ErrorPush errStack 1 [IntlErrId DATABASE] $appvals(filename).db
	}
}

@if notused
proc DomainClose {} {
	global appvals

	Map:CloseNoWrite $appvals(fileid)
}
@endif
