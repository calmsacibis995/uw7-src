#===============================================================================
#
#	ident "%W"
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
# SCO Mail Sysadm
#	cf module : back end routines to manipulate the sendmail.cf file.
#
# Modification History
#
# M000, 20-Feb-97, andrean
#	- Created.
#
#===============================================================================

#
# Manipulate the sendmail.cf file.
#

#
# The line length at which wrappable entries (such as classes)
# are wrapped.
#
set CF_MaxLine	80

#
# Table mapping old single character option names to new full names.
#
array set optionNames {\
			7	SevenBitInput \
			8	EightBitMode \
			A	AliasFile \
			a	AliasWait \
			B	BlankSub \
			b	MinFreeBlocks \
			C	CheckpointInterval \
			c	HoldExpensive \
			D	AutoRebuildAliases \
			d	DeliveryMode \
			E	ErrorHeader \
			e	ErrorMode \
			f	SaveFromLine \
			F	TempFileMode \
			G	MatchGECOS \
			H	HelpFile \
			h	MaxHopCount \
			i	IgnoreDots \
			I	ResolverOptions \
			J	ForwardPath \
			j	SendMimeErrors \
			k	ConnectionCacheSize \
			K	ConnectionCacheTimeout \
			L	LogLevel \
			l	UseErrorsTo \
			m	MeToo \
			n	CheckAliases \
			O	DaemonPortOptions \
			o	OldStyleHeaders \
			P	PostmasterCopy \
			p	PrivacyOptions \
			Q	QueueDirectory \
			q	QueueFactor \
			R	DontPruneRoutes \
			S	StatusFile \
			s	SuperSafe \
			t	TimeZoneSpec \
			u	DefaultUser \
			U	UserDatabaseSpec \
			V	FallbackMXhost \
			v	Verbose \
			w	TryNullMXList \
			x	QueueLA \
			X	RefuseLA \
			Y	ForkEachJob \
			y	RecipientFactor \
			z	ClassFactor \
			Z	RetryFactor}

# Don't know what to do with this one yet
# r	Timeout \
# T	T has been split into Timeout.queuereturn  and Timeout.queuewarn ?\

array set listOptions {AliasFile "," \
		       DaemonPortOptions "," \
		       PrivacyOptions "," \
		       ResolverOptions " "}

###
# Routines for parsing the cf file.
###

#
# Convert a single character option name to its full name.
#
proc CF:OptionFullname {id} {
    global optionNames
    if {[clength $id] == 1 && [info exists optionNames($id)]} {
	return $optionNames($id)
    } else {
	return $id
    }
}

proc CF:CommaListOption {id} {
    global listOptions
    set id [CF:OptionFullname $id]
    if {[info exists listOptions($id)] && $listOptions($id) == ","} {
	return 1
    } else {
	return 0
    }
}


#
# Read the cf file and parse the lines therein.
#
proc CF:ReadCB {fileID fp} {
    Table:SetOption $fileID newFirst 1

    set context [scancontext create]

    scanmatch $context {^D\{(.*)\}(.*)} {
	Table:ProcessLine $fileID macro $matchInfo(submatch0) \
	    $matchInfo(submatch1)
    }

    scanmatch $context {^D([^\{])(.*)} {
	Table:ProcessLine $fileID macro $matchInfo(submatch0) \
	    $matchInfo(submatch1)
    }

    scanmatch $context {^O ([^ 	=]+)([ 	=]*)?(.*)} {
	Table:ProcessLine $fileID option $matchInfo(submatch0) \
	    $matchInfo(submatch2)
    }

    scanmatch $context {^O([^ ])(.*)} {
	Table:ProcessLine $fileID option \
	    [CF:OptionFullname $matchInfo(submatch0)] $matchInfo(submatch1)
    }

    scanmatch $context {^K([^ 	]*)(.*)} {
	Table:ProcessLine $fileID map $matchInfo(submatch0) \
	    $matchInfo(submatch1)
    }

    scanmatch $context {^C\{(.*)\}(.*)} {
	Table:ProcessLine $fileID class $matchInfo(submatch0) \
	    $matchInfo(submatch1)
    }

    scanmatch $context {^C([^\{])(.*)} {
	Table:ProcessLine $fileID class $matchInfo(submatch0) \
	    $matchInfo(submatch1)
    }

    scanmatch $context {^S(.*)} {
	Table:ProcessLine $fileID ruleset $matchInfo(submatch0) {}
    }

    scanmatch $context {^R.*} {
	Table:ProcessLine $fileID CONTINUATION {} $matchInfo(line)
    }

    scanmatch $context {^M([^,]*),(.*)} {
	Table:ProcessLine $fileID mailer $matchInfo(submatch0) \
	    $matchInfo(submatch1)
    }

#
# The following are commented out because they're not currently
# used anywhere, and they slow things down.  Uncomment them as
# you need them.
#

#    scanmatch $context {^F\{(.*)\}(.*)} {
#	Table:ProcessLine $fileID classFile $matchInfo(submatch0) \
#	    $matchInfo(submatch1)
#    }

#    scanmatch $context {^F(.)(.*)} {
#	Table:ProcessLine $fileID classFile $matchInfo(submatch0) \
#	    $matchInfo(submatch1)
#    }

#    scanmatch $context {^P([^=]*)=(.*)} {
#	Table:ProcessLine $fileID precedence $matchInfo(submatch0) \
#	    $matchInfo(submatch1)
#    }

#    scanmatch $context {^V(.*)} {
#	Table:ProcessLine $fileID version $matchInfo(submatch0) {}
#    }

#    scanmatch $context {^H([^:]*):(.*)} {
#	Table:ProcessLine $fileID header $matchInfo(submatch0) \
#	    $matchInfo(submatch1)
#    }


#
# If you uncomment either mailers or headers above, make sure the
# next one is uncommented, too
#

    scanmatch $context {^[ 	]+[^ 	]+} {
	global Table_lastType

	#if {$Table_lastType == "header" || $Table_lastType == "mailer"} {
	    Table:ProcessLine $fileID CONTINUATION {} $matchInfo(line)
	#} else {
	#    Table:ProcessLine $fileID COMMENT {} $matchInfo(line)
	#}
    }

#    scanmatch $context {^T(.*)} {
#	Table:ProcessLine $fileID trusted $matchInfo(submatch0) {}
#    }

#
# Catch-all for unmatched lines; just treat them as comments
# and remember them.
#

    scanmatch $context {
	Table:ProcessLine $fileID COMMENT {} $matchInfo(line)
    }

    scanfile $context $fp

    scancontext delete $context
}

#
# Write the cf file.
#
proc CF:WriteCB {fileID fp type id data} {
    case $type {
	macro {
	    if {[string length $id] > 1} {
		puts $fp "D\{${id}\}${data}"
	    } else {
		puts $fp "D$id$data"
	    }
	}

	option {
	    if {[string length $id] > 1} {
		puts $fp "O $id=$data"
	    } else {
		puts $fp "O$id$data"
	    }
	}

	map {
	    puts $fp "K$id$data"
	}

	class {
	    if {[clength $id] > 1} {
		puts $fp "C\{${id}\}${data}"
	    } else {
		puts $fp "C$id$data"
	    }
	}

	ruleset {
	    if {[clength $data] > 0} {
		puts $fp "S${id}\n${data}"
	    } else {
		puts $fp "S$id"
	    }
	}

	mailer {
	    if {$id != {}} {
		puts $fp "M$id," nonewline
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

proc CF:Open {fileName {mode 0644}} {
    return [Table:Open $fileName CF:ReadCB CF:WriteCB $mode]
}

proc CF:Write {fileID {fileName {}}} {
    return [Table:Write $fileID $fileName]
}

proc CF:Close {fileID} {
    return [Table:Close $fileID]
}

@if notused
proc CF:CloseNoWrite {fileID} {
    return [Table:CloseNoWrite $fileID]
}

proc CF:DeferWrites {fileID} {
    return [Table:DeferWrites $fileID]
}
@endif

proc CF:ForceWrites {fileID} {
    return [Table:ForceWrites $fileID]
}

proc CF:Get {fileID type id} {
    if {$type == "option"} {
	set id [CF:OptionFullname $id]
	if {[CF:CommaListOption $id]} {
	    set result [Table:Get $fileID $type $id ","]
	} else {
	    set result [Table:Get $fileID $type $id " "]
	}
    } elseif {$type == "ruleset"} {
	set result [Table:Get $fileID $type $id "\n"]
	set result [string trim $result " \n"]
    } else {
	set result [Table:Get $fileID $type $id " "]
    }
    return $result
}

proc CF:Set {fileID type id data} {
    case $type {
	{class mailer option} {
	    return [CF:SetList $fileID $type $id $data]
	}

	default {
	    return [Table:Set $fileID $type $id $data]
	}
    }
}

proc CF:Unset {fileID type id} {
@if notused
    if {$type == "option"} {
	set id [CF:OptionFullname $id]
    }
@endif
    return [Table:Unset $fileID $type $id]
}

proc CF:SetList {fileID type id value} {
    global CF_MaxLine
    set separator " "
    set continuation 0

    # some option values are separated by commas, not whitespace
    case $type {
	option {
	    set continuation 1
	    set id [CF:OptionFullname $id]
	    if {[CF:CommaListOption $id]} {
		set valueList {}
		foreach item [split $value {,}] {
		    lappend valueList [string trim $item]
		}
		set value $valueList
		set separator ","
	    }
	    set data "[lvarpop value]$separator"
	    if {[clength $id] > 1} {
		set length [expr \
		    [string length $data] + [string length $id] + 3]
	    } else {
		set length [expr [string length $data] + 2]
   	    }
	}

	mailer {
	    set continuation 1
	    set valueList {}
	    foreach item [split $value {,}] {
		lappend valueList [string trim $item]
	    }
	    set value $valueList
	    set separator ", "
    	    set data " [lvarpop value]$separator"
	    set length [expr [string length $data] + [string length $id] + 2]
	} 

	class {
	    set continuation 1
	    set data "[lvarpop value]$separator"
	    if {[clength $id] > 1} {
		set length [expr \
		    [string length $data] + [string length $id] + 3]
	    } else {
		set length [expr [string length $data] + 2]
	    }
	}
@if notused
	default {
	    set data "[lvarpop value]$separator"
	    set length [expr [string length $data] + [string length $id]]
	}
@endif
    }
    
    set separatorLength [string length $separator]
    set first 1

    foreach word $value {
	set wordLen [string length $word]

	if {[expr "$length + $wordLen + $separatorLength"] <= $CF_MaxLine} {
	    append data "${word}${separator}"
	    set length [expr "$length + $wordLen + $separatorLength"]
	} else {
	    if {$first} {
		Table:Set $fileID $type $id $data
	    } else {
		if {$continuation} {
		    Table:AppendLine $fileID CONTINUATION $id $data
@if notused
		} else {
		    Table:AppendLine $fileID $type $id $data
@endif
		}
	    }

	    set first 0
	    if {$continuation} {
		set data "\t${word}${separator}"
		set length [expr 8 + $wordLen + $separatorLength]
@if notused
	    } else {
		set data "${word}${separator}"
		set length [expr $wordLen + [string length $separator]]
@endif
	    }
	}
    }

    set data [string trimright $data $separator]
    if {$first} {
	return [Table:Set $fileID $type $id $data]
    } else {
	if {$continuation} {
	    return [Table:AppendLine $fileID CONTINUATION $id $data]
@if notused
	} else {
	    return [Table:AppendLine $fileID $type $id $data]
@endif
	}
    }
}

#
# This function cannot be currently used for adding equates to mailers,
# as the additional equates must be put before the last A equate--
# you must use the CF:Set funtion for mailers.
#
proc CF:AddValue {fileID type id addValue} {
    set currentValue [CF:Get $fileID $type $id]

@if notused
    # Some option values separated by commas, not whitespace
    if {$type == "option"} {
	set id [CF:OptionFullname $id]
	if {[CF:CommaListOption $id]} {
	    # NOTE:  This does not remove redundancies, nor does it
	    #	     sort the values -- for AliasFile, we DON'T want
	    #	     to sort, as order is significant.  For other options 
	    # 	     with comma separated values, we should later modify 
	    #	     this to both remove duplicates, and sort.
	    set newValue [string trim "${currentValue},${addValue}" ","]
	    return [CF:SetList $fileID $type $id $newValue]
	}
    }
@endif

    set newValue [union $currentValue $addValue]
    return [CF:SetList $fileID $type $id $newValue]
}

proc CF:RemoveValue {fileID type id removeValue} {
    set currentValue [CF:Get $fileID $type $id]

@if notused
    if {$type == "option"} {
	set id [CF:OptionFullname $id]

	# Some option values can be comma separated lists, not space separated.
	if {[CF:CommaListOption $id]} {
	    regsub -all {[ 	]*,[ 	]*} $currentValue  "," formattedValue
	    set currentList [split [string trim $formattedValue] {,}]

	    regsub -all {[ 	]*,[ 	]*} $removeValue  "," formattedValue
	    set removeList [split [string trim $formattedValue] {,}]
	    
	    # For AliasFile option, order is significant.
	    # NOTE: Later, modify this so other comma separated option
	    #	    lists have redundancies removed, and sorted.
	    foreach item $removeList {
		set removeIndex [lsearch -exact $currentList $item]
		if {$removeIndex != -1} {
		    lvarpop currentList $removeIndex
		}
	    }
	    set newValue [join $currentList {,}]
	    return [CF:SetList $fileID $type $id $newValue]
	}
    }
@endif

    set newValue [lindex [intersect3 $currentValue $removeValue] 0]
    return [CF:SetList $fileID $type $id $newValue]
}

proc CF:GetMapInfo {fileID mapName} {
    global optarg optind

    if {[set data [CF:Get $fileID map $mapName]] == {}} {
	return {}
    }

    keylset info name $mapName
    keylset info class [ctoken data " \t"]

    while {[set item [ctoken data " \t"]] != {}} {
	lappend argv $item
    }

#    parse argv and put into keyed list if needed.
#    while {"$argv" != "" && "[csubstr $argv 0 1]" == "-"} {
#    }

    set count [expr [llength $argv] - 1]
    keylset info pathName [lindex $argv $count]
    return $info
}
