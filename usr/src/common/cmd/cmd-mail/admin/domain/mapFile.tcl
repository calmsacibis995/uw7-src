#===============================================================================
#
#	ident @(#) mapFile.tcl 11.1 97/10/30 
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
# SCO Mail Administration Domain & Channel clients:
#       Back end routines for parsing the domain and channel table files
#	(map files).
#
#
# Modification History
#
# M000, 20-Feb-97, andrean
#	- Created.
#
#===============================================================================

#
# Map:ReadCB
#
# Purpose:
#	Callback that is evaluated during the read of a map file.
# Accepts:
#	fileID		- file id that is returned from a call to Map:Open
#	fp		- file pointer to the file that is being read
# Returns:
#	none
# Called by:
#	Function Table:Read in module table.tcl.
#
proc Map:ReadCB {fileID fp} {

    set context [scancontext create]

    scanmatch $context {^([^#][^ 	]*)([ 	].*)} {
	Table:ProcessLine $fileID entry $matchInfo(submatch0) \
	    $matchInfo(submatch1)
    }

    scanmatch $context {
	Table:ProcessLine $fileID COMMENT {} $matchInfo(line)
    }

    scanfile $context $fp

    scancontext delete $context
}

#
# Map:WriteCB
#
# Purpose:
#	Callback that is evaluated during the write of a map file.
# Accepts:
#	fileID		- file id that is returned from a call to Map:Open
#	fp		- file pointer to the file that is being read
#	type		- type of data
#	id		- identification of data
#	data		- value associated with id
# Returns:
#	none
# Called by:
#	Function Table:Write in module table.tcl.
#
proc Map:WriteCB {fileID fp type id data} {
    case $type {
	entry {
	    puts $fp "${id}${data}"
	}

	COMMENT {
	    puts $fp $data
	}
    }
}

#
# Map:Open
#
# Purpose:
#	Open the map file $fileName, and load in data to back end
#	data structures.
#
proc Map:Open {fileName {mode 0644}} {
    return [Table:Open $fileName Map:ReadCB Map:WriteCB $mode]
}

#
# Map:Write
#
# Purpose:
#	Write current data for file with handle $fileID to file.
#	If the fileName parameter is specified, then data is written
#	to $fileName, regardless of original filename associated with
#	handle $fileID.  Map:Write does not do a write if the data
#	has not been modified since the read. 
#
proc Map:Write {fileID {fileName {}}} {
    return [Table:Write $fileID $fileName]
}

@if notused
#
# Map:Close
#
# Purpose:
#	Clear out all data structures associated with file $fileID.
#	Map:Close automatically attempts a write (as described above
#	in Map:Write).
#
proc Map:Close {fileID} {
    return [Table:Close $fileID]
}

#
# Map:CloseNoWrite
#
# Purpose:
#	Clear out all data structures associated with file $fileID.
#	Map:CloseNoWrite does not attempt a write, whether data has
#	been modified or not.  This changes the state of the file
#	to not modified.
#
proc Map:CloseNoWrite {fileID} {
    return [Table:CloseNoWrite $fileID]
}
@endif

#
# Map:ForceWrites
#
# Purpose:
#	This will force a Map:Write to write the data to file, whether 
#	it has been modified or not.  It basically marks the file as
#	having been modified.
#
proc Map:ForceWrites {fileID} {
    return [Table:ForceWrites $fileID]
}

#
# Map:Get
#
# Purpose:
#	Returns the value (right hand side of entry ) associated 
#	with key (left hand side of entry ) $id.
#	Returns empty string if no entry for key $id exists.
#
proc Map:Get {fileID id} {
    return [string trimleft [Table:Get $fileID entry $id]]
}

#
# Map:Set
#
# Purpose:
#	Set the value of key $id to $data.  If an entry for $id
#	does not exist, then the entry is created as the last
#	entry in the file.
#
proc Map:Set {fileID id data} {
    if {![string match {[ 	]*} $data]} {
	set data "	$data"
    }

    return [Table:Set $fileID entry $id $data]
}

#
# Map:Unset
#
# Purpose:
#	Removes the entry associated with key $id.
#
proc Map:Unset {fileID id} {
    return [Table:Unset $fileID entry $id]
}

#
# Map:List
#
# Purpose:
#	Return the list of map keys in file $fileID, in the order they appear
#	in the file. 
#
proc Map:List {fileID} {
    set lineOrdered 1
    return [Table:List $fileID entry $lineOrdered]
}

#
# Map:Insert
#
# Purpose:
#	Inserts an entry with key $id and value $data after the entry
#	with key $idBefore.
#
proc Map:Insert {fileID id data idBefore} {
    if {![string match {[ 	]*} $data]} {
	set data "	$data"
    }
    # there should really not be duplicate entry ids (keys),
    # but in case a duplicate was hand edited in, we pick the first one
    # as it is the only one that counts in the map file
    set lineBefore [lindex [Table:GetLineList $fileID entry $idBefore] 0]
    # put the new entry after $idBefore
    Table:AppendLine $fileID entry $id $data $lineBefore 
}

#
# Map:Modified
#
# Purpose:
#	Returns 1 if map file $fileID has been modified, 0 otherwise.
#
proc Map:Modified {fileID} {
    return [Table:Modified $fileID]
}
