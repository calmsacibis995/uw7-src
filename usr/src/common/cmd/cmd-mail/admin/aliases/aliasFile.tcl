#==============================================================================
#
#	ident @(#) aliasFile.tcl 11.1 97/10/30 
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
# 	Back end routines for parsing the aliases file.
#
#
# Modification History
#
# M000, 20-Feb-97, andrean
#	- Created.
# 
#==============================================================================


###
# Routines for parsing the aliases file.
###

# maximum line length
set Alias_MaxLine	80

#
# Read the alias file and parse the lines therein.
#
proc Alias:ReadCB {fileID fp} {

    set context [scancontext create]

    scanmatch $context {^(#.*|[ 	]*)$} {
	Table:ProcessLine $fileID COMMENT {} $matchInfo(line)
	continue
    }

    scanmatch $context {^([^: 	]+):(.*)} {
	Table:ProcessLine $fileID alias $matchInfo(submatch0) \
	    $matchInfo(submatch1)
    }

    scanmatch $context {^[ 	]+[^ 	]+} {
	Table:ProcessLine $fileID CONTINUATION {} $matchInfo(line)
    }

    scanmatch $context {
	Table:ProcessLine $fileID COMMENT {} $matchInfo(line)
    }

    scanfile $context $fp

    scancontext delete $context
}

#
# Write the alias file.
#
proc Alias:WriteCB {fileID fp type id data} {
    case $type {
	alias {
	    if {$id != {}} {
		puts $fp "$id:" nonewline
	    }

	    puts $fp $data
	}

	default {
	    puts $fp $data
	}
    }
}

###
# Begin public entry points.
###

#
# Open the alias file to initialize all data structures.
# Returns a string which is a unique handle to the alias file--
# all other functions require this handle as a parameter.
#
proc Alias:Open {fileName {mode 0644}} {
    return [Table:Open $fileName Alias:ReadCB Alias:WriteCB $mode]
}

#
# Write the current data out to file.
#
# fileID is the handle for the file returned from a previous
# call to Alias:Open.
#
# fileName is the name of the file to which data will be written to,
# but should only be used if this is different from the original filename
# associated with fileID.
#
proc Alias:Write {fileID {fileName {}}} {
    return [Table:Write $fileID $fileName]
}

#
# Close the file associated with fileID.
# This will automatically write the current data out to file
# if it has been modified from the file's original contents.
#
proc Alias:Close {fileID} {
    return [Table:Close $fileID]
}

#
# Prevents write to file from occuring until an Alias:Close (basically
# blocks Alias:Write).
#
proc Alias:DeferWrites {fileID} {
    return [Table:DeferWrites $fileID]
}

#
# Forces a write to file to occur on the next call to Alias:Write or
# Alias:Close, even if file was not modified.
#
proc Alias:ForceWrites {fileID} {
    return [Table:ForceWrites $fileID]
}

#
# Returns a tcl list (space separated) of recipients associated with
# alias name $id, for file fileID.
#
proc Alias:Get {fileID id} {
    set value [Table:Get $fileID alias $id]
    set result {}

    # alias recipients are separated by commas--
    # but a recipient may be a quoted string which also has a comma
    while {[clength $value] > 0} {
	set thisToken [ctoken value ","]
	set numQuotes [expr [llength [split $thisToken "\""]] - 1]
	if {[fmod $numQuotes 2] == 1.0} {
	    set restOfQuotedPart [ctoken value "\""]
		append thisToken $restOfQuotedPart
		append thisToken [ctoken value ","]
	}
	set recipient [string trim $thisToken]
	if {[clength $recipient] > 0} {
	    lappend result $recipient
	}
    }

    return $result
}

#
# Set alias name $id with recipient list $value, in file $fileID.
# Parameter "value" must be a tcl list of recipients (space separated).
#
proc Alias:Set {fileID id value} {
    global Alias_MaxLine

    set data " [lvarpop value],"

    # for first line, add length of alias name, plus 1 for the colon,
    # plus length of $data
    set length [expr [string length $data] + [string length $id] + 1]
    set first 1

    foreach word $value {
	set wordLen [string length $word]

	if {[expr "$length + $wordLen + 2"] <= $Alias_MaxLine} {
	    append data " $word,"
	    set length [expr "$length + $wordLen + 2"]
	} else {
	    if {$first} {
		Table:Set $fileID alias $id $data
	    } else {
		# if this isn't the first line, append as a continued line
		Table:AppendLine $fileID CONTINUATION $id $data
	    }

	    set first 0
	    set data "\t${word},"
	    set length [expr "$wordLen + 9"]
	}
    }

    set data [string trimright $data ","]
    if {$first} {
	return [Table:Set $fileID alias $id $data]
    } else {
	return [Table:AppendLine $fileID CONTINUATION $id $data]
    }
}

#
# Remove the alias $id from the table.
#
proc Alias:Unset {fileID id} {
    return [Table:Unset $fileID alias $id]
}

#
# Return a list of all alias names in $fileID.
#
proc Alias:List {fileID} {
    return [Table:List $fileID alias]
}

#
# Add recipients in $addValue to alias $id.
# Parameter addValue must be a tcl list.
# Duplicates are removed, and the recipients are sorted.
#
proc Alias:AddValue {fileID id addValue} {
    set currentValue [Alias:Get $fileID $id]
    if {[lempty $currentValue]} {
	set newValue $addValue
    } else {
	set newValue [union $currentValue $addValue]
    }
    return [Alias:Set $fileID $id $newValue]
}

#
# Remove recipients in $removeValue from alias $id.
# Parameter removeValue must be a tcl list.
# The recipients are sorted.
#
proc Alias:RemoveValue {fileID id removeValue} {
    set currentValue [Alias:Get $fileID $id]
    set newValue [lindex [intersect3 $currentValue $removeValue] 0]
    if {[lempty $newValue]} {
	return [Alias:Unset $fileID $id]
    } else {
	return [Alias:Set $fileID $id $newValue]
    }
}

#
# Returns a boolean indicating whether the table has been
# modified (since the last write or open).
#
proc Alias:Modified {fileID} {
    return [Table:Modified $fileID]
}

#
# Returns 1 if $recipient is a member of alias $id.
# Returns 0 otherwise.
#
proc Alias:IsMember {fileID id recipient} {
    set members [Alias:Get $fileID $id]
    if {[lempty [lmatch -exact $members $recipient]]} {
	return 0
    } else {
	return 1
    }
}

#
# Returns a tcl list of all aliases that $recipient is a member of.
#
proc Alias:Memberships {fileID recipient} {
    set result {}
    foreach alias [Alias:List $fileID] {
	if {[Alias:IsMember $fileID $alias $recipient]} {
	    lappend result $alias
	}
    }
    return $result
}
