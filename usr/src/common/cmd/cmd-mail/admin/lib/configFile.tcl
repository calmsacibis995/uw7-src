#===============================================================================
#
#	ident @(#) configFile.tcl 11.1 97/10/30 
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
# SCO Mail System Administration:
#       This (along with table.tcl) is the basis of the back ends for
#       all mail system administration clients.
#
#
# Modification History
#
# M000, 20-Feb-97, andrean
#	- Created.
#
#===============================================================================

#
# configFile.tcl: routines for manipulating lines
# in a configuration file.
#

proc ConfigFile:Open {} {
    global Config_next

    if {![info exists Config_next]} {
	set Config_next 0
    }

    set file "config$Config_next"
    incr Config_next

    upvar #0 Config_${file}_next file_next
    set file_next "1.0"
    return $file
}

proc ConfigFile:Close {file} {
    upvar #0 Config_$file fileList
    upvar #0 Config_${file}_next file_next

    unset file_next
    foreach line [ConfigFile:GetList $file] {
	unset fileList($line)
    }
}

#
# Compare two line "numbers" (indices to the data array)
# and return -1, 0, or 1 depending upon whether line1 is
# less than, equal to, or greater than line2.
#
proc ConfigFile:Compare {line1 line2} {
    set subLine1 [ctoken line1 "."]
    set subLine2 [ctoken line2 "."]

    if {$subLine1 == "" && $subLine2 == ""} {
	return 0
    } elseif {$subLine1 == ""} {
	return -1
    }  elseif {$subLine2 == ""} {
	return 1
    } elseif {$subLine1 < $subLine2} {
	return -1
    } elseif {$subLine1 > $subLine2} {
	return 1
    } else {
	set line1 [string trimleft $line1 "."]
	set line2 [string trimleft $line2 "."]
	return [ConfigFile:Compare $line1 $line2]
    }
}

#
# Add an entry to the file at a particular line.
#
proc ConfigFile:ReplaceLine {file line type id dataVar} {
    upvar #0 Config_$file fileList
    upvar #0 Config_${file}_next file_next
    upvar $dataVar data

    keylset fileList($line) type $type
    keylset fileList($line) data $data
    keylset fileList($line) id $id

    if {[ConfigFile:Compare $line $file_next] == 1} {
	set temp $line
	set file_next [ctoken temp "."]
	incr file_next
	set file_next "${file_next}.0"
    }
}

#
# Append an entry after a particular one.
# The array indices are not ordered real numbers, but
# rather a notation that is similar to header numbering in documents.
# For example, the following are in increasing line order in a file:
#	1.0 
#	1.0.0.1
#	1.0.0.2
#	1.0.1 
#	1.1
#	2.0 
#	2.1 
#	3.0 
#	3.0.1 
#	3.0.2 
#	3.1
#	4.0
#	5.0
# You can actually think of it in this way: remove all dots but the
# first one, then they are actually in numerical real order.
# The problem with doing it that way in the first place is that tcl
# is limited in precision -- doing it with the extra dots, and
# providing a sorting algorighm means we insert a much larger number of
# lines after a particular one.
#
proc ConfigFile:AppendLine {file line type id dataVar} {
    upvar #0 Config_$file fileList
    upvar #0 Config_${file}_next file_next
    upvar $dataVar data

    if {$line == -1} {
	set line $file_next
	set temp [ctoken file_next "."]
	incr temp
	set file_next "${temp}.0"
    } else {
	set found 0
	set left [ctoken line "."]
	set right [ctoken line "."]
	if {$right == "0"} {
	    set tryNext $left
	    incr tryNext
	    set tryNext "${tryNext}.0"
	    if {![info exists fileList($tryNext)]} { 
		set found 1
	    }
	}
	if {!$found} {
		set lastDigit [cindex $right end]
		if {$lastDigit < 9} {
		    incr lastDigit
		    set tryNext "${left}.[csubstr $right 0 end]${lastDigit}"
		    if {![info exists fileList($tryNext)]} {
			set found 1
		    }	
		}
	}
	if {!$found} {
	    set leadingZeros ""
	    set tryNext "${left}.${right}.1"
	    while {[info exists fileList($tryNext)]} {
		set leadingZeros "${leadingZeros}.0"
		set tryNext "${left}.${right}${leadingZeros}.1"
	    }
	}
	set line $tryNext
    }

    keylset fileList($line) type $type
    keylset fileList($line) id $id
    keylset fileList($line) data $data

    if {[string compare $line $file_next] == 1} {
	set temp $line
	set file_next [ctoken temp "."]
	incr file_next
	set file_next "${file_next}.0"
    }

    return $line
}

#
# Return the entry at a particular line.
#
proc ConfigFile:GetLine {file line typeVar idVar dataVar} {
    upvar #0 Config_$file fileList
    upvar $typeVar type
    upvar $idVar id
    upvar $dataVar data

    if {![info exists fileList($line)]} {
	set type {}
	set id {}
	set data {}
	return -1
    }

    keylget fileList($line) type type
    keylget fileList($line) id id
    keylget fileList($line) data data

    return 0
}

#
# Delete an entry.
#
proc ConfigFile:RemoveLine {file line} {
    upvar #0 Config_$file fileList

    if {![info exists fileList($line)]} {
	# already gone
	return
    }

    unset fileList($line)
}

#
# Return the sorted file of entry indices.
#
proc ConfigFile:GetList {file} {
    upvar #0 Config_$file fileList

    if {![info exists fileList]} {
	return {}
    }

    return [lsort -command ConfigFile:Compare [array names fileList]]
}
