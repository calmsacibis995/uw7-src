#===============================================================================
#
#	ident @(#) table.tcl 11.1 97/10/30 
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
#	This (along with configFile.tcl) is the basis of the back ends for 
#	all system administration clients.
#
# Modification History
#
# M001, 01-Apr-97, andrean
#	- In Table:List(), added a parameter "ordered".  If $ordered is true,
#	  then Table:List() now returns the list of ids in the line order they
#	  appear in the file, regardless of whether the makelower option is set
#	  or not. 
#
# M000, 20-Feb-97, andrean
#	- Created.
#
#===============================================================================

#
# Process each line in the table.
#
proc Table:ProcessLine {fileID type id data} {
    global Table_lastType Table_lastID

    set lineNum [ConfigFile:AppendLine $fileID -1 $type $id data]

    if {[Table:GetOption $fileID makelower] != {}} {
	set id [string tolower $id]
    }

    case $type {
	CONTINUATION {
	    set type $Table_lastType
	    set id $Table_lastID
	}

	COMMENT {
	}

	default {
	    set Table_lastType $type
	    set Table_lastID $id
	}
    }

    if {$id != {}} {
	upvar #0 Table_${fileID}_$type typeList
	lappend typeList($id) $lineNum
    }
}

#
# Read the table file and parse the lines therein.
#
proc Table:Read {fileName callBack mode} {
    global Table_info
@if test
    global TEST
@endif

    if {[info exists Table_info($fileName.ID)]} {
	return $Table_info($fileName.ID)
    }

    set fileID [ConfigFile:Open]
    set Table_info($fileID.mode) $mode

    if {![file exists $fileName]} {
	return $fileID
    }

@if test
    if {"$TEST" == "aliases_cmd_error_alias_load" || \
	"$TEST" == "aliases_gui_error_alias_load" || \
	"$TEST" == "domain_gui_error_load" || \
	"$TEST" == "channel_gui_error_load"} {
	system "rm -f $fileName"
    }
@endif
    set fp [open $fileName r]

    set Table_info($fileName.ID) $fileID
    set Table_info($fileID.fileName) $fileName

    eval $callBack $fileID $fp

    close $fp
    return $fileID
}

#
# Write the table file.
#
proc Table:Write {fileID {fileName {}} {callBack {}}} {
@if test
    global TEST
@endif
    global Table_info

    if {[info exists Table_info($fileID.deferred)]} {
	return
    }

    if {![info exists Table_info($fileID.modified)] && $fileName == {}} {
	return
    }

    if {$callBack == {}} {
	set callBack $Table_info($fileID.writeCB)
    }

    if {$fileName == {}} {
	set fileName $Table_info($fileID.fileName)
	set clearModified 1
    } else {
	set clearModified 0
    }

    #
    # If the file exists, make sure the new file has the
    # same owner and perms.  Otherwise, make sure the
    # parent directory exists.
    #
    if {[file exists $fileName]} {
	file stat $fileName info

	set Table_info($fileID.uid) $info(uid)
	set Table_info($fileID.gid) $info(gid)
	set Table_info($fileID.mode) $info(mode)
    } else {
	set directory [file dirname $fileName]
	set oldUmask [umask]
	umask 0222

	if {![file isdirectory $directory]} {
	    mkdir -path $directory
	}

	umask $oldUmask
    }

    set fp [open ${fileName}.new w]

    chmod $Table_info($fileID.mode) ${fileName}.new

    if {[info exists Table_info($fileID.uid)]} {
	chown [list $Table_info($fileID.uid) $Table_info($fileID.gid)] \
	    ${fileName}.new
    }

@if test
    if {"$TEST" == "aliases_cmd_error_save" || \
	"$TEST" == "aliases_gui_error_save" || \
	"$TEST" == "domain_gui_error_write" || \
	"$TEST" == "channel_gui_error_write"} {
	system "rm -f ${fileName}.new"
    }
@endif
    foreach line [ConfigFile:GetList $fileID] {
	if {[ConfigFile:GetLine $fileID $line type id data] != 0} {
	    continue
	}

	eval "$callBack $fileID $fp {$type} {$id} {$data}"
    }

    close $fp

    if {[file exists $fileName]} {
	unlink $fileName
    }

    frename ${fileName}.new $fileName

    if {$clearModified} {
	unset Table_info($fileID.modified)
    }
}

proc Table:Open {fileName readCB writeCB {mode 0666}} {
    global Table_info

    set fileID [Table:Read $fileName $readCB $mode]

    set Table_info($fileID.fileName) $fileName
    set Table_info($fileID.readCB) $readCB
    set Table_info($fileID.writeCB) $writeCB

    set Table_info($fileName.ID) $fileID

    return $fileID
}

proc Table:Close {fileID {fileName {}}} {
    global Table_info

    if {![info exists Table_info($fileID.fileName)]} {
	return
    }

    if {[info exists Table_info($fileID.deferred)]} {
	unset Table_info($fileID.deferred)
    }

    Table:Write $fileID $fileName $Table_info($fileID.writeCB)

    if {$fileName == {}} {
	set fileName $Table_info($fileID.fileName)
    }

    unset Table_info($fileID.fileName)
    unset Table_info($fileName.ID)

    ConfigFile:Close $fileID
}

proc Table:CloseNoWrite {fileID {fileName {}}} {
    global Table_info

    if {![info exists Table_info($fileID.fileName)]} {
	return
    }

    if {[info exists Table_info($fileID.deferred)]} {
	unset Table_info($fileID.deferred)
    }

    if {$fileName == {}} {
	set fileName $Table_info($fileID.fileName)
    }

    unset Table_info($fileID.fileName)
    unset Table_info($fileName.ID)

    ConfigFile:Close $fileID
}

proc Table:DeferWrites {fileID} {
    global Table_info

    set Table_info($fileID.deferred) 1
}

proc Table:ForceWrites {fileID} {
    global Table_info

    if {[info exists Table_info($fileID.deferred)]} {
	unset Table_info($fileID.deferred)
    }
    set Table_info($fileID.modified) 1
} 

proc Table:SetOption {fileID option {value 1}} {
    global Table_options
    set Table_options($fileID.$option) $value
}

proc Table:HasOption {fileID option} {
    return [info exists Table_options($fileID.$option)]
}

proc Table:GetOption {fileID option} {
    global Table_options

    if {![info exists Table_options($fileID.$option)]} {
	return {}
    }

    return $Table_options($fileID.$option)
}


proc Table:Modified {fileID} {
    global Table_info

    if {[info exists Table_info($fileID.modified)]} {
	return $Table_info($fileID.modified)
    } else {
	return 0
    }
}

proc Table:GetLineList {fileID type {idList {}}} {
    upvar #0 Table_${fileID}_$type typeList

    if {![info exists typeList]} {
	return {}
    }

    if {$idList == {}} {
	set idList [array names typeList]
    }

    set lineList {}

    foreach id $idList {
	if {[info exists typeList($id)]} {
	    append lineList " $typeList($id)"
	}
    }

    return [lsort -command ConfigFile:Compare $lineList]
}

proc Table:Get {fileID type id {separator {}}} {
    upvar #0 Table_${fileID}_$type typeList

    if {[Table:GetOption $fileID makelower] != {}} {
	set id [string tolower $id]
    }

    set value {}

    if {![info exists typeList($id)]} {
	return $value
    }

    set numLines [llength [Table:GetLineList $fileID $type $id]]
    set thisLine 1
    foreach line $typeList($id) {
	ConfigFile:GetLine $fileID $line dummy dummy data
	if {$thisLine == $numLines} {
	    append value "$data"
	} else {
	    set data [string trim $data "$separator"]
	    append value "$data$separator"
	}
	incr thisLine
    }

    set value [string trimleft $value $separator]
    return $value
}

#
# Set the specified type and id to the given data.
# If that id isn't already in the file, append it after
# the last line of the same type.  If there are none,
# put it at the beginning or end of the file, depending
# on the value of the newFirst option.
#
proc Table:Set {fileID type id data} {
    upvar #0 Table_${fileID}_${type} typeList
    global Table_info
    global Table_lastType Table_lastID

    set name $id

    if {[Table:GetOption $fileID makelower] != {}} {
	set id [string tolower $id]
    }

    if {[info exists typeList($id)]} {
	foreach line $typeList($id) {
	    ConfigFile:RemoveLine $fileID $line
	}

	set line [lindex $typeList($id) 0]
	unset typeList($id)
    } else {
	if {[set lineList [Table:GetLineList $fileID $type]] != {}} {
	    set line [lindex $lineList end]
	} else {
	    if {[Table:GetOption $fileID newFirst] == 1} {
		set line 0
	    } else {
		set line -1
	    }
	}
    }

    lappend typeList($id) [ConfigFile:AppendLine $fileID $line \
	$type $name data]


    set Table_lastType $type
    set Table_lastID $id
    set Table_info($fileID.modified) 1
}

proc Table:Unset {fileID type id} {
    upvar #0 Table_${fileID}_$type typeList
    global Table_info

    if {[Table:GetOption $fileID makelower] != {}} {
	set id [string tolower $id]
    }

    if {![info exists typeList($id)]} {
	return
    }

    foreach line $typeList($id) {
	ConfigFile:RemoveLine $fileID $line
    }

    unset typeList($id)
    set Table_info($fileID.modified) 1
}

proc Table:AppendLine {fileID type id value {line {}}} {
    global Table_lastType Table_lastID
    global Table_info

    if {$type == "CONTINUATION"} {
	upvar #0 Table_${fileID}_$Table_lastType typeList
    } else {
	upvar #0 Table_${fileID}_$type typeList
    }

    if {[Table:GetOption $fileID makelower] != {}} {
	set id [string tolower $id]
    }

    if {$line == ""} {
	if {[info exists typeList($id)]} {
	    set line [lindex $typeList($id) end]
	} else {
	    if {[set lineList [Table:GetLineList $fileID $type]] == {}} {
		set line [lindex $lineList end]
	    } else {
		if {[Table:GetOption $fileID newFirst] == 1} {
		    set line 0
		} else {
		    set line -1
		}
	    }
	}
    }

    if {$type == "CONTINUATION"} {
	lappend typeList($id) \
		[ConfigFile:AppendLine $fileID $line $type {} value]
    } else {
	lappend typeList($id) \
		[ConfigFile:AppendLine $fileID $line $type $id value]
    }
    set Table_info($fileID.modified) 1
}

proc Table:RemoveLine {fileID type id deleteLine} {
    upvar #0 Table_${fileID}_$type typeList
    global Table_info

    if {[Table:GetOption $fileID makelower] != {}} {
	set id [string tolower $id]
    }

    if {![info exists typeList($id)]} {
	return
    }

    set index 0

    foreach line $typeList($id) {
	if {$line == $deleteLine} {
	    lvarpop typeList($id) $index
	    ConfigFile:RemoveLine $fileID $line
	    break
	}

	incr index
    }

    set Table_info($fileID.modified) 1
}

proc Table:ReplaceLine {fileID type replaceLine id data} {
    global Table_info
    upvar #0 Table_${fileID}_$type typeList
    
    if {[ConfigFile:GetLine $fileID $replaceLine oldType oldId oldData] == 0} {
	if {$oldId != ""} {
    	    upvar #0 Table_${fileID}_$oldType oldTypeList
	    set index [lsearch -exact $oldTypeList($oldId) $replaceLine]
	    if {$index != -1} {
		lvarpop oldTypeList($oldId) $index
		if {[lempty $oldTypeList($oldId)]} {
			unset oldTypeList($oldId)
		}
	    }
	}
    }

    ConfigFile:ReplaceLine $fileID $replaceLine $type $id data
    if {$id != ""} {
	if {![info exists typeList($id)]} {
	    set typeList($id) {}
	}
	lappend typeList($id) $replaceLine
    }
    set Table_info($fileID.modified) 1
}


    
proc Table:List {fileID type {ordered 0}} {
    upvar #0 Table_${fileID}_$type typeList

    if {![info exists typeList]} {
	return {}
    }

    if {[Table:GetOption $fileID makelower] == {} && $ordered == 0} {
	return [array names typeList]
    }

    #
    # We have to go get the original ids from before they were converted 
    # to lower case, or they want the ids in the order they appear anyway.
    #
    set returnList {}
    foreach line [Table:GetLineList $fileID $type] {
	ConfigFile:GetLine $fileID $line dummy id dummy
	append returnList " $id"
    }

    return [string trimleft $returnList]
}
