#===============================================================================
#
#	ident @(#) ma_sendmail.tcl 11.2 97/11/17 
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
# SCO Mail Sysadm :
#	This is the backend api to the mail system administration GUI
#	to edit the sendmail configuration file sendmail.cf.
#
# Dependencies:
#	This module depends upon
#		cf.tcl
#	which in turn depends upon
#		configFile.tcl
#		table.tcl
#
# Basic Architecture:
#	ma_sendmail.tcl (this file) has the api calls the front end will use
#	to change the sendmail configuration.  This file in turn calls
#	functions in cf.tcl which make the corresponding sendmail.cf
#	file changes.  The functions in cf.tcl use functions in configFile.tcl
#	and table.tcl (as do all the gui's back end pieces), which manage
#	a table of data which is a configuration file's contents.  
#	configFile.tcl and table.tcl are general enough to use with many
#	types of configuration files (we have used them for the sendmail.cf
#	file, database map files, and aliases files).
#
#
# Modification History
#
# M005, 25-Sept-97, andrean
#	- Fixed MR# us97-24542 : In mai_ch_structure_commit(), we no longer
#	  throw an error if a channel uses the "baduser" table (match unknown
#	  users), but no forwarding host is defined.  Now just checks if
#	  a forwarding host is defined for this case, and create the rule
#	  accordingly (i.e. if no forwarding host, then no $@ part of
#	  ruleset 0 triple defined).  Also, made corresponding change in
#	  mai_ch_structure_init(), to *not* throw an error if a channel
#	  with baduser table is found with no forwarding host.
#
# M004, 02-Apr-97, andrean
#	- In ma_machine_name_set(), must remove current machine names from
#	  the class w (alternate hostnames list).
#
# M003, 01-Apr-97, andrean
#	- In mai_ch_structure_init(), throw error if we cannot determine the
#	  delivery agent.
#	- In mai_ch_structure_commit(), if table type was file, was always
#	  using a default file path-- fixed this to always use the table file
#	  attribute value.
#	- Added lots of comments.
#	  
# M002, 24-Mar-97, andrean
#	- In mai_ch_structure_commit(), changed the channel table file
#	  map configuration command from a hash to btree database.
#
# M001, 27-Feb-97, andrean
#	- Added more sendmail.cf compatibility checks to ma_cf_open().
#	- Make sure the A equate is always last when creating mailer in
#	  mai_ch_structure_commit().
#
# M000, 20-Feb-97, andrean
#	- Created.
#
#===============================================================================

#
# Handle for the sendmail.cf file.
#
set MA_SENDMAIL_FILEID		-1

#
# Flag set if channel structure is modified
#
set MA_CH_MODIFIED		0

#
# Directory where channel table files and their databases are stored
#
set MA_CH_TABLE_DIRECTORY "/etc/mail/table"

#
# Arrays mapping ruleset name to table type, and vice versa, for those tables 
# pre-coded  into sendmail.cf --
#	MA_CH_TABLE_TYPE(ruleset_name) = table_type
#	MA_CH_RULESET(table_type) = ruleset_name
#
array set MA_CH_TABLE_TYPE {\
	dns_TBL		DNS \
	uucp_TBL	UUCP \
	local_TBL	local \
	remote_TBL	remote \
	baduser_TBL	baduser}

array set MA_CH_RULESET {\
	DNS		dns_TBL \
	UUCP		uucp_TBL \
	local		local_TBL \
	remote		remote_TBL \
	baduser		baduser_TBL}

#
# All possible channel table types
#
set MA_CH_TABLE_TYPE_LIST [list \
	"DNS" \
	"UUCP" \
	"local" \
	"remote" \
	"baduser" \
	"file"]

#
# Reserved channel names-- these are basically built-in mailers in sendmail.
# Since we are, by convention, naming both the associated mailer and ruleset
# after the channel name, we must check for these internal mailer names for
# conflicts (sendmail defines these even if they are NOT set in sendmail.cf).
#
set MA_CH_RESERVED_NAMES [list \
	"error" \
	"prog" \
	"*file*" \
	"*include*"]

#
# ma_cf_valid
#
# Purpose:
#	Determines the validity of the sendmail.cf file.  By convention,
#	the sendmail.cf file has as its first line a checksum of its contents
#	(without that first checksum line).  If this checksum line is missing,
#	or the checksum is incorrect, then the file is not valid, as we assume
#	then the cf file has not been edited through the GUI.
# Accepts:
#	filename	- pathname of sendmail.cf file
# Returns:
#	"fail"		- returns string "fail" if $filename is invalid
#	"ok"		- return string "ok" otherwise
#
proc ma_cf_valid {filename} {
@if test
    global TEST
@endif

    set retval "ok"
    if {[catch {open $filename r} fd] != 0} {
        return "fail"
    }

    if {[gets $fd line] != -1} {
	if {[regexp {^# checksum:(.*)$} $line match recordedSum] == 1} {
	    set tmp /tmp/ma_cf_valid.[pid]
	    set ret [catch {open $tmp w} tmpfd]
@if test
	    if {"$TEST" == "grp8_v6"} {
	    	set ret 1
	    }
@endif
	    if {$ret != 0} {
		error "Unable to open $tmp"
	    }
	    copyfile $fd $tmpfd
	    set csum [exec sum -r $tmp]
	    set currentSum [ctoken csum " \t"]
	    if {$currentSum != $recordedSum} {
		set retval "fail"
	    }
	    close $tmpfd
	    system "rm -f $tmp"
	} else {
	    set retval "fail"
	}
    } else {
	set retval "fail"
    }

    close $fd
    return $retval
}

#
# ma_cf_open
#
# Purpose:
#	Open and read in the sendmail.cf file, determine if all elements 
#	needed for compatibility with the GUI exist within the file, and 
#	initialize the internal channel data structure.
# Accepts:
#	filename	- pathname of sendmail.cf file
# Returns:
#	"fail"		- returns the string "fail" if $filename does not exist
#	"parserr"	- returns the string "parserr" if some element of the
#			  cf file needed to maintain compatibility with the GUI
#			  is missing
#	"ok"		- returns the string "ok" otherwise
#	
proc ma_cf_open {filename} {
    global MA_SENDMAIL_FILEID MA_SENDMAIL_FILEPATH MA_SENDMAIL_FILESUM

    if {$MA_SENDMAIL_FILEID != -1} {
    	ma_cf_close
    }

    if {![file exists $filename]} {
	return "fail"
    }

    set MA_SENDMAIL_FILEID [CF:Open $filename]

    #
    # For compatibility with the GUI, sendmail.cf file must have...
    #

    # a csumline
    if {[ConfigFile:GetLine \
	$MA_SENDMAIL_FILEID 1.0 dummy dummy csumline] == -1} {
	ma_cf_close
	return "parserr"
    }
    if {[regexp {^# checksum:(.*)$} $csumline match csum] == 1} {
	set MA_SENDMAIL_FILESUM $csum
    } else {
	ma_cf_close
	return "parserr"
    }

    # correctly defined channels
    if {[catch mai_ch_structure_init errorMsg] != 0} {
	ma_cf_close
	return "parserr"
    }

    # the users map (for baduser channel table)
    if {[CF:Get $MA_SENDMAIL_FILEID map users] == ""} {
	ma_cf_close
	return "parserr"
    }

    # the uucp map (for uucp channel table)
    if {[CF:Get $MA_SENDMAIL_FILEID map uucp] == ""} {
	ma_cf_close
	return "parserr"
    }

    # the domain table map 
    if {[CF:Get $MA_SENDMAIL_FILEID map domain] == ""} {
	ma_cf_close
	return "parserr"
    }
    # macros, classes for domain table
    if {[CF:Get $MA_SENDMAIL_FILEID class P] != "." || \
	[CF:Get $MA_SENDMAIL_FILEID class D] != "DMNTAB"} {
	ma_cf_close
	return "parserr"
    }
    # the rulesets for domain table parsing
    if {[CF:Get $MA_SENDMAIL_FILEID ruleset domains] == "" || \
	[CF:Get $MA_SENDMAIL_FILEID ruleset domains_helper] == ""} {
	ma_cf_close
	return "parserr"
    }

    # all the rulesets for built-in channel tables
    if {[CF:Get $MA_SENDMAIL_FILEID ruleset local_TBL] == ""	|| \
	[CF:Get $MA_SENDMAIL_FILEID ruleset dns_TBL] == ""	|| \
	[CF:Get $MA_SENDMAIL_FILEID ruleset uucp_TBL] == ""	|| \
	[CF:Get $MA_SENDMAIL_FILEID ruleset remote_TBL] == ""	|| \
	[CF:Get $MA_SENDMAIL_FILEID ruleset baduser_TBL] == ""} {
	ma_cf_close
	return "parserr"
    }
 
    # all the rulesets for built-in R=, S= equates
    if {[CF:Get $MA_SENDMAIL_FILEID ruleset ap822_se] == ""	|| \
	[CF:Get $MA_SENDMAIL_FILEID ruleset ap822_sh] == ""	|| \
	[CF:Get $MA_SENDMAIL_FILEID ruleset ap822_re] == ""	|| \
	[CF:Get $MA_SENDMAIL_FILEID ruleset ap822_rh] == ""	|| \
	[CF:Get $MA_SENDMAIL_FILEID ruleset ap976_se] == ""	|| \
	[CF:Get $MA_SENDMAIL_FILEID ruleset ap976_sh] == ""	|| \
	[CF:Get $MA_SENDMAIL_FILEID ruleset ap976_re] == ""	|| \
	[CF:Get $MA_SENDMAIL_FILEID ruleset ap976_rh] == ""	|| \
	[CF:Get $MA_SENDMAIL_FILEID ruleset aplocal_se] == ""	|| \
	[CF:Get $MA_SENDMAIL_FILEID ruleset aplocal_sh] == ""	|| \
	[CF:Get $MA_SENDMAIL_FILEID ruleset aplocal_re] == ""	|| \
	[CF:Get $MA_SENDMAIL_FILEID ruleset aplocal_rh] == ""} {
	ma_cf_close
	return "parserr"
    }
    
    set MA_SENDMAIL_FILEPATH $filename
    return ok
}

#
# ma_cf_close
#
# Purpose:
#	Free data structures associated with the last read of the cf file.
# Accepts:
#	none
# Returns:
#	none
#
proc ma_cf_close {} {
    global MA_SENDMAIL_FILEID MA_CH_MODIFIED MA_SENDMAIL_FILESUM channels

    if {$MA_SENDMAIL_FILEID == -1} {
	error "Sendmail configuration file is not open"
    }

    Table:CloseNoWrite $MA_SENDMAIL_FILEID
    set MA_SENDMAIL_FILEID -1
    set MA_CH_MODIFIED 0
    if {[info exists MA_SENDMAIL_FILESUM]} {
	unset MA_SENDMAIL_FILESUM
    }
    if {[info exists channels]} {
	unset channels
    }
}

#
# ma_cf_write
#
# Purpose:
#	Write changes to the cf file.  This file should have been opened
#	previously by a call to ma_cf_open(), otherwise, an error will be
#	thrown.
# Accepts:
#	none
# Returns:
#	"fail"		- returns string "fail" for any failure to write
#			  changes to file
#	"ok"		- returns string "ok" otherwise
#	
proc ma_cf_write {} {
    global MA_SENDMAIL_FILEID MA_SENDMAIL_FILEPATH MA_SENDMAIL_FILESUM
    global MA_CH_MODIFIED 
@if test
    global TEST
@endif

    if {$MA_SENDMAIL_FILEID == -1} {
	error "Sendmail configuration file is not open"
    }

    if {$MA_CH_MODIFIED} {
	if {[catch mai_ch_structure_commit errorMsg] != 0} {
	   return "fail"
	}
    }

    CF:Write $MA_SENDMAIL_FILEID

    # get a checksum of contents, *without* the csum line

    set ret [catch {open $MA_SENDMAIL_FILEPATH r} fd]
@if test
    if {"$TEST" == "grp8_op1"} {
    	set ret 1
    }
@endif
    if {$ret != 0} {
        return "fail"
    }

    set ret [gets $fd line]
@if test
    if {"$TEST" == "grp8_op2"} {
    	set ret -1
    }
@endif
    if {$ret == -1} {
        return "fail"
    }

    set tmp /tmp/ma_cf_write.[pid]
    set ret [catch {open $tmp w} tmpfd]
@if test
    if {"$TEST" == "grp8_op3"} {
    	set ret 1
    }
@endif
    if {$ret != 0} {
	return "fail"
    }
    copyfile $fd $tmpfd
    close $fd
    close $tmpfd
    set csum [exec sum -r $tmp]
    set currentSum [ctoken csum " \t"]
    if {$currentSum != $MA_SENDMAIL_FILESUM} {
	set newCsumline "# checksum:$currentSum"
	set type "COMMENT"
	set id ""
	ConfigFile:ReplaceLine $MA_SENDMAIL_FILEID 1.0 $type $id newCsumline
	CF:ForceWrites $MA_SENDMAIL_FILEID
	CF:Close $MA_SENDMAIL_FILEID
    }
    system "rm -f $tmp"

    return "ok"
}

#
# ma_machine_name_get
#
# Purpose:
#	Returns the contents of the j macro in sendmail.cf.
#	Sendmail itself defines this macro internally as the machine name,
#	whether it is defined in sendmail.cf or not.  So this procedure
#	literally just returns what the sendmail.cf file has this set to, and
#	not necessarily what sendmail has determined to be the machine name.
# Accepts:
#	none
# Returns:
#	A string which is the machine name.  May be an empty string, which 
#	means either that the j macro is not defined in sendmail.cf, or is
#	defined as an empty string.
#
proc ma_machine_name_get {} {
    global MA_SENDMAIL_FILEID

    if {$MA_SENDMAIL_FILEID == -1} {
	error "Sendmail configuration file is not open"
    }

    # NOTE: If the j macro is in turned defined by other macros, these
    # 	    are not expanded, but are passed as is to the front end.
    set machineName [CF:Get $MA_SENDMAIL_FILEID macro j]

    return $machineName
}

#
# ma_machine_name_set
#
# Purpose:
#	Sets the machine name for sendmail.  This sets (or unsets) the following
#	macros in sendmail.cf:
#		j	- the fqdn
#		w	- everything before the first "." in the fqdn
#		m	- everything after the first "." in the fqdn
#		k	- the uucp node name (should be same as w)
#	These must all be changed, since sendmail will define them internally
#	if they are not defined in sendmail.cf.
# Accepts:
#	A string which is the configured machine name for sendmail.  If this
#	string is null, then all above macros should be unset in sendmail.cf.
# Returns:
#	none
#
proc ma_machine_name_set {name} {
    global MA_SENDMAIL_FILEID

    if {$MA_SENDMAIL_FILEID == -1} {
	error "Sendmail configuration file is not open"
    }

    # NOTE: We are assuming that we would never want to intentionally
    #	    set the j macro to a null string, or white space
    #	    Also, if the j macro is in turn defined by other
    #	    macros, then the macros are not expanded.

    if {[ctype space $name]} {
	set name ""
    }

    set currentName [ma_machine_name_get]

    if {[cequal $name $currentName ]} {
	return
    }

    if {[cequal $name ""]} {
	# unset the machine name in sendmail.cf, which allows
	# sendmail itself to internally define these
	CF:RemoveValue $MA_SENDMAIL_FILEID class w \
		[list [CF:Get $MA_SENDMAIL_FILEID macro j] \
		      [CF:Get $MA_SENDMAIL_FILEID macro w]]
	CF:Unset $MA_SENDMAIL_FILEID macro j
	CF:Unset $MA_SENDMAIL_FILEID macro w
	CF:Unset $MA_SENDMAIL_FILEID macro m
	CF:Unset $MA_SENDMAIL_FILEID macro k
    } else {
	set fqdn $name
	set host [ctoken name "."]
	set domain [string trim $name "."]

	if {"$currentName" != ""} {
		if {[regexp {([^\.]+)\..+} $currentName \
			match currentHost] == 1} {
			set removeList [list $currentName $currentHost]
		} else {
			set removeList [list $currentName]
		}
		CF:RemoveValue $MA_SENDMAIL_FILEID class w $removeList
	}
		
	CF:Set $MA_SENDMAIL_FILEID macro w $host
	CF:Set $MA_SENDMAIL_FILEID macro m $domain
	CF:Set $MA_SENDMAIL_FILEID macro j $fqdn
	CF:Set $MA_SENDMAIL_FILEID macro k $host
	CF:AddValue $MA_SENDMAIL_FILEID class w \
		[list $host $fqdn]
    }
}
	
#
# ma_alternate_names_get
#
# Purpose:
#	Return all alternate machine names defined in sendmail.cf-- basically
#	all names in the class w which appear in sendmail.cf.  Note that this
#	may not include $w and $j, in the case that the machine name has
#	not been explicitly set in sendmail.cf with ma_machine_name_set(),
#	since sendmail would automatically, internally define these and append
#	them to class w.
# Accepts:
#	none
# Returns:
#	A string which is the contents of the w class (a space separated list
#	of names).  NOTE: $M (masqeraade name) is NOT returned if it exists
#	as an alternate name.  See ma_alternate_names_set().
#
proc ma_alternate_names_get {} {
    global MA_SENDMAIL_FILEID

    if {$MA_SENDMAIL_FILEID == -1} {
	error "Sendmail configuration file is not open"
    }

    set list [CF:Get $MA_SENDMAIL_FILEID class w]
    if { [lindex $list 0] == "\$M" } {
    	set list [lrange $list 1 end]
    }
    return $list
}

#
# ma_alternate_names_set
#
# Purpose:
#	Set alternate machine names, by setting the w class.
#	NOTE: $M is always silently added to the alternate names-- this
#	      ensures that local mail is properly handled.
# Accepts:
#	A space separated string (or tcl list) of names.  
#	May be an empty string.
# Returns:
#	none
#
proc ma_alternate_names_set {nameList} {
    global MA_SENDMAIL_FILEID

    if {$MA_SENDMAIL_FILEID == -1} {
	error "Sendmail configuration file is not open"
    }

    if {[llength $nameList] == 0} {
	CF:Set $MA_SENDMAIL_FILEID class w "\$M"
    } else {
	CF:Set $MA_SENDMAIL_FILEID class w "\$M $nameList"
    }
}

#
# ma_from_domain_get
#
# Purpose:
#	Returns the currently configured from domain (domain hiding)-- 
#	basically just returns the contents of the M macro.
# Accepts:
#	none
# Returns:
#	A string which is the configured from domain.  May be an empty string,
#	meaning no domain hiding is configured.
#
proc ma_from_domain_get {} {
    global MA_SENDMAIL_FILEID

    if {$MA_SENDMAIL_FILEID == -1} {
	error "Sendmail configuration file is not open"
    }

    return [CF:Get $MA_SENDMAIL_FILEID macro M]
}
    
#
# ma_from_domain_set
#
# Purpose:
#	Sets the from domain.
# Accepts:
#	A string which is the from domain name.  This may be the empty string,
#	which means that domain hiding is not configured.
# Returns:
#	none
#
proc ma_from_domain_set {domain} {
    global MA_SENDMAIL_FILEID

    if {$MA_SENDMAIL_FILEID == -1} {
	error "Sendmail configuration file is not open"
    }

    CF:Set $MA_SENDMAIL_FILEID macro M $domain
}
    
#
# ma_aliases_get
#
# Purpose:
#	Returns all configured alias files in sendmail.cf.
# Accepts:
#	none
# Returns:
#	Returns a tcl list of configured alias files in sendmail.cf, with
#	the alias file specification syntax left as is (so any class and
#	flags specified are left in the string).
#
proc ma_aliases_get {} {
    global MA_SENDMAIL_FILEID
    set result {}

    if {$MA_SENDMAIL_FILEID == -1} {
	error "Sendmail configuration file is not open"
    }

    foreach aliasFile \
	[split [CF:Get $MA_SENDMAIL_FILEID option AliasFile] {,}] {
	lappend result "[string trim $aliasFile]"
    }

    return $result
}

#
# ma_aliases_set
#
# Purpose:
#	Sets the AliasFile option in sendmail.cf, thus configuring all alias
#	files for sendmail.
# Accepts:
#	A tcl list of space separated strings, each defining an alias file.
#	Assumes that the syntax for specifying an alias file is correct, and
#	passes them as is to the cf file.
# Returns:
#	none
#
proc ma_aliases_set {aliasFileList} {
    global MA_SENDMAIL_FILEID

    if {$MA_SENDMAIL_FILEID == -1} {
	error "Sendmail configuration file is not open"
    }

    set cfString ""
    foreach aliasFile $aliasFileList {
	append cfString "$aliasFile,"
    }
    set cfString [string trimright $cfString ","]

    CF:Set $MA_SENDMAIL_FILEID option AliasFile $cfString
}

#
# ma_domain_table_enabled_get
#
# Purpose:
#	Determines if the domain table is enabled or not.  Basically does this
#	by seeing what the D macro is set to-- if the D macro is set to a
#	non-empty string, it is enabled, otherwise it is disabled.
# Accepts:
#	none
# Returns:
#	0	- if the domain table is not enabled
#	1	- if the domain table is enabled
#
proc ma_domain_table_enabled_get {} {
    global MA_SENDMAIL_FILEID

    if {$MA_SENDMAIL_FILEID == -1} {
	error "Sendmail configuration file is not open"
    }

    set cfString [CF:Get $MA_SENDMAIL_FILEID macro D]

    if {[clength $cfString] == 0} {
	return 0
    } else {
	return 1
    }
}

#
# ma_domain_table_enabled_set
#
# Purpose:
#	Enables or disables the domain table lookup.  Basically sets the
#	D macro to do this.  If the D macro is set to "DMNTAB" the domain table
#	is enabled; if the D macro is an empty string, then it is disabled.
# Accepts:
#	true		- to enable the domain table
#	false		- to disable the domain table
# Returns:
#	none
#
proc ma_domain_table_enabled_set {enable} {
    global MA_SENDMAIL_FILEID

    if {$MA_SENDMAIL_FILEID == -1} {
	error "Sendmail configuration file is not open"
    }

    if {$enable} {
	CF:Set $MA_SENDMAIL_FILEID macro D DMNTAB
    } else {
	CF:Set $MA_SENDMAIL_FILEID macro D ""
    }
}

#
# ma_domain_table_file_get
#
# Purpose:
#	Returns the currently configured domain table file location.
#
proc ma_domain_table_file_get {} {
    global MA_SENDMAIL_FILEID

    if {$MA_SENDMAIL_FILEID == -1} {
	error "Sendmail configuration file is not open"
    }

    set mapInfo [CF:GetMapInfo $MA_SENDMAIL_FILEID domain]
    keylget mapInfo pathName mapFile
    return $mapFile
}

############################# Channel Functions #############################

#
# mai_ch_default_equates
#
# Purpose:
#	Internal (non api) function used to initialize the equates in a 
#	channel's mailer.
# Accepts:
#	none
# Returns:
#	A tcl keyed list of equates, where the key is the (upper case)
#	equate letter.
#
proc mai_ch_default_equates {} {
    set equateList {}
    keylset equateList P "\[IPC\]"
    keylset equateList F "mDFMuXa"
    keylset equateList S "ap822_se/ap822_sh"
    keylset equateList R "ap822_re/ap822_rh"
    keylset equateList E "\\r\\n"
    keylset equateList L "990"
    keylset equateList A "IPC \$h"
    return $equateList
}

#
# mai_ch_equates_get
#
# Purpose:
#	Parses the cf mailer definition to determine the equates defined	
#	for the mailer-- since commas may be imbedded within the A=
#	by a \ prefix, and commas are also the separators between the
#	equates.
# Accepts:
#	mailer name
# Returns:
#	A tcl keyed list of equates, where the key is the (upper case)
#	equate letter.
proc mai_ch_equates_get { mailer } {
    global MA_SENDMAIL_FILEID

    set equateStr [CF:Get $MA_SENDMAIL_FILEID mailer $mailer]
    set equateList {}
    set equatesAttr {}
    set listitem ""
    while {[set token [ctoken equateStr ","]] != {}} {
	set i [expr [clength $token] - 1]
	set backslashes 0
	# if there is a \ before the comma, we have to count the
	# number of \'s before the comma, since \\ is acceptable
	# as well as \,
	while {$i >= 0 && [cindex $token $i] == "\\"} {
	    incr backslashes
	    set i [expr $i - 1]
	}
	if {[expr $backslashes % 2] == 1} {
	    set listitem "${listitem}${token},"
	} else {
	    set listitem "${listitem}${token}"
	    lappend equateList $listitem
	    set listitem ""
	}
    }

    foreach equate $equateList {
	if {[regexp {([A-Z]).*=[ 	]*(.*)} $equate \
	    match equateLetter equateValue] == 1} {
		keylset equatesAttr $equateLetter [string trim $equateValue]
	}
    }

    return $equatesAttr
}
		
#
# mai_ch_order
#
# Purpose:
#	Internal (non-api) function used to sort the configured channels by 
#	their sequence order.
# Accepts:
#	2 channel names.
# Returns:
#	-1	- if $chName1 is ordered below $chName2
#	0	- if $chName1 and $chName2 have the same order
#	1	- if $chName1 is ordered above $chName2
#
proc mai_ch_order {chName1 chName2} {
    global channels
    set seq1 [lindex $channels($chName1) 0]
    set seq2 [lindex $channels($chName2) 0]
    if {$seq1 < $seq2} {
	return -1
    } elseif {$seq1 == $seq2} {
	return 0
    } else {
	return 1
    }
}

#
# mai_ch_valid_table_type
#
# Purpose:
#	An internal (non-api) function used to determine whether a channel 
#	table type is valid or not.
# Accepts:
#	A string which is the table type name.
# Returns:
#	1	- if the $type is a valid table type.
#	0	- otherwise
#
proc mai_ch_valid_table_type {type} {
    global MA_CH_TABLE_TYPE_LIST
    if {[lsearch -exact $MA_CH_TABLE_TYPE_LIST $type] == -1} {
	return 0
    } else {
	return 1
    }
}
    
#
# ma_ch_valid_name
#
# Purpose:
#	An internal (non-api) function used to determine if the channel name
#	is valid (i.e. no name conflicts).
# Accepts:
#	A string which is the channel name.
# Returns:
#	1	- if the channel name is valid
#	0	- otherwise
#
proc mai_ch_valid_name {name} {
    global MA_SENDMAIL_FILEID MA_CH_RESERVED_NAMES channels

    # check if this channel already exists
    set currentNames [ma_ch_names_get]
    if {[lsearch -exact $currentNames $name] != -1} {
        return 0
    }

    # check if this channel name is a reserved name
    if {[lsearch -exact $MA_CH_RESERVED_NAMES $name] != -1} {
	return 0
    }

    # check if this channel name already exists as a ruleset or mailer name
    set rulesetAndMailerNames [union [Table:List $MA_SENDMAIL_FILEID ruleset] \
				[Table:List $MA_SENDMAIL_FILEID mailer]]
    if {[lsearch -exact $rulesetAndMailerNames $name] != -1} {
	return 0
    }

    # would this channel name have a conflict with the map name ${name}_MAP ?
    set mapNames [Table:List $MA_SENDMAIL_FILEID map]
    if {[lsearch -exact $mapNames ${name}_MAP] != -1} {
	return 0
    }

    return 1
}

#
# mai_ch_valid_chars
#
# Purpose:
#	An internal (non-api) function used to determine if there are any
#	invalid characters in a channel name.
# Accepts:
#	A string which is the channel name.
# Returns:
#	1	- if all characters in $name are valid
#	0	- otherwise
#
proc mai_ch_valid_chars {name} {
    global MA_SENDMAIL_FILEID

    # get the characters that sendmail uses internally as separation chars
    set oValue [CF:Get $MA_SENDMAIL_FILEID macro o]

    set invalidChars "${oValue}()<>,;\\\"\r\n\t "

    if {[llength [split $name $invalidChars]] > 1} {
	return 0
    }

    return 1
}
     
#
# mai_ch_structure_init
#
# Purpose:
#	This function is used internally to initialize a data structure
#	holding information about all configured channels.  This data structure
#	is the global array "channels".  This is needed since the concept of 
#	channels does not exist in sendmail and is basically implemented by a
# 	a set of conventions we use in our pre-coded sendmail.cf.
#
#	A channel has the following attributes (value type listed in parens):
#		- name
#			A string which is the channel name.  By convention,
#			this name is used to name the ruleset and mailer
#			associated with that channel.
#		- sequence 
#			Integer indicating order of this channel.
#		- table type 
#			A string which is the channel table type-- these are 
#			all listed in global variable MA_CH_TABLE_TYPE_LIST
#		- equate list 
#			Tcl keyed list of the channel's mailer equates.
#		- channel table file
#			A string which is the pathname to the channel table
#			file (ignored if the table type is not "file"). 
#		- forwarding host
#			A string which is the name of a host the channel will 
#			forward mail to (may be the empty string).
#
#	Each entry in the internal array "channels" contains information for
#	one channel-- a list of the above attributes.  The index to the array
#	is the channel name.  So, an entry is contructed as follows:
#
#	channels(channel_name) =
#		{sequence table_type table_file forwarding_host equate_list}
#
#	Each attribute value must be accessed by position in this list.
#
#	All modifications made to channels via the api are first done to
#	this array, and later translated into cf commands by function
#	mai_ch_structure_commit() (see below).
#
# Accepts:
#	none
#
# Returns:
#	none
#
proc mai_ch_structure_init {} {
    global MA_SENDMAIL_FILEID MA_CH_TABLE_TYPE channels MA_CH_TABLE_DIRECTORY
    
    set channelsRuleset [CF:Get $MA_SENDMAIL_FILEID ruleset channels]

    if {$channelsRuleset == ""} {
	error "Ruleset \"channels\" doesn't exist; config file not compatible."
    }

    set channelsList [split $channelsRuleset "\n"]

    set thisSeq 0
    foreach rule $channelsList {
	if {[regexp {^R\$\*[	]+\$:[ ]*\$>[ ]*([^ ]+)[ ]+\$1} \
	    $rule match chName] == 1} {
	    set sequenceAttr $thisSeq
	    set channelRulesList \
		[split [CF:Get $MA_SENDMAIL_FILEID ruleset $chName] "\n"]
	    set rule1 [lindex $channelRulesList 0]
	    set rule2 [lindex $channelRulesList 1]

	    #
	    # Determine table type, table file...
	    #
	    if {[regexp {\$:[ ]*\$>[ ]*([^ ]+)[ ]*\$} \
		$rule1 match table] == 1} {

		# See if a built in table (ruleset)
		if {[info exists MA_CH_TABLE_TYPE($table)]} {
		    # this is a built-in table type
		    set tableTypeAttr "$MA_CH_TABLE_TYPE($table)"
		} else {
		    # we don't know what table type this is
		    error "Unknown table type \"$table\""
		}	
		set tableFileAttr ""

	    } elseif {[regexp {\$:[ ]*<[ ]*\$\(([^ ]+)[ ]+.*\$\)[ ]*>} \
			$rule1 match mapName] == 1} {

		# Otherwise, see if a there's a table map file
		set tableTypeAttr "file"
		set mapConfigStr [CF:Get $MA_SENDMAIL_FILEID map $mapName]
		if {[clength $mapConfigStr] != 0} {
		    set tableFileAttr "[lindex $mapConfigStr end]"
		} else {
		    set tableFileAttr "$MA_CH_TABLE_DIRECTORY/${chName}"
		}

	    } else {
	        # This channel is not defined within expected conventions
		error "Invalid channel \"$chName\": cannot determine table"
	    }

	    #
	    # Determine delivery agent...
	    #
	    set equatesAttr {}
	    if {[regexp {\$#[ ]*([^ ]+)[ ]+} \
		$rule2 match mailer] == 1} {

		# By convention, the mailer name should be the same as the
		# channel name.
		if {![cequal $mailer $chName]} {
		    error "Channel \"$chName\" has different mailer name \"$mailer\""
		}

		set equatesAttr [mai_ch_equates_get $mailer]

	    } else {
	        # This channel is not defined within expected conventions
		error "Invalid channel \"$chName\": cannot determine deliver agent"
	    }

	    if {[lempty $equatesAttr]} {
		error "Invalid channel \"$chName\": no delivery agent equates"
	    }

	    #
	    # Determine forwarding host (if any)...
	    #
	    if {[regexp {\$@[ ]*([^ ]+)[ ]*\$:} \
		$rule2 match host] == 1} {


		if {[regexp {\$[0-9]+} $host] == 1} {
		   # $@ is a positional parameter
		    set forwardHostAttr ""
		} else {
		    # Note: This allows host= to be a defined macro
		    set forwardHostAttr $host	
		}
	    } else {
		# No $@ host specification (probably a channel that
		# does local delivery)
		set forwardHostAttr ""
	    }

	    set attributeList [list $sequenceAttr\
				    $tableTypeAttr \
				    $tableFileAttr \
				    $forwardHostAttr \
				    $equatesAttr]
	    set channels($chName) $attributeList
	    incr thisSeq	
	}
    }

    #
    # Determine if baduser host is configured
    #
    set lastRuleset5 \
	[lindex [split [CF:Get $MA_SENDMAIL_FILEID ruleset 5] "\n" ] end]

    if {[regexp {\$:[ ]*\$>([^ ]+)[ ]*\$1} $lastRuleset5 \
	match subroutineName] == 1} {

	set subroutineRules \
	    [split [CF:Get $MA_SENDMAIL_FILEID ruleset $subroutineName] "\n"]
	
	if {[regexp {\$:[ ]*\$>([^ ]*)[ ]*} [lindex $subroutineRules 0] \
	    match tableRoutine] == 1 && [cequal $tableRoutine "baduser_TBL"]} {

	    set chName $subroutineName
	    set sequenceAttr $thisSeq
	    set tableTypeAttr "baduser"
	    set tableFileAttr ""
	    set forwardHostAttr ""
	    set equatesAttr {}

	    # get mailer's equates
	    if {[regexp {\$#[ ]*([^ ]+)[ ]+} \
		[lindex $subroutineRules 1] match mailer] == 1} {

		if {![cequal $mailer $chName]} {
		    error "Channel \"$chName\" has different mailer name \"$mailer\""
		}

		set equatesAttr [mai_ch_equates_get $mailer]
	    }

	    if {[lempty $equatesAttr]} {
		error "Invalid channel \"$chName\": no delivery agent equates"
	    }

	    # get the forwarding host
	    if {[regexp {\$@[ ]*([^ ]*)[ ]*\$:} \
		[lindex $subroutineRules 1] match host] == 1} {
		    # NOTE: By convention, we always put the
		    # name of the forwarding host in the $@ portion
		    # of the triple.  But the cf file could be hand
		    # edited to put some macro, or operator there-- these
		    # are simply passed up for now, but maybe some error
		    # checks should be put in?
		    set forwardHostAttr $host
	    }
	
	    set attributeList [list \
		$sequenceAttr \
		$tableTypeAttr \
		$tableFileAttr \
		$forwardHostAttr \
		$equatesAttr]
	    set channels($chName) $attributeList
	}
    }
}

#
# mai_ch_structure_commit
#
# Purpose:
#	This function is used internally to commit all modifications made to 
#	the channel configuration, contained within global array "channels".
#	Basically, any api calls to change the channel configuration are first 
#	done to the internal (to this module) array "channels".  Then upon 
#	write of the entire sendmail.cf file, this function is used to first
#	make appropriate changes to the sendmail.cf file backend data. 
#	(See comments for mai_ch_structure_init(), above.)
# Accepts:
#	none
# Returns:
#	none
#
proc mai_ch_structure_commit {} {
    global MA_SENDMAIL_FILEID MA_CH_RULESET MA_CH_TABLE_DIRECTORY channels

    set orderedNames [ma_ch_names_get]
    set numChannels [llength $orderedNames]

    set channelsRuleset ""
    loop index 1 [expr $numChannels + 1] {

	set chName [lvarpop orderedNames]
	set tableType [lindex $channels($chName) 1]
	set tableFile [lindex $channels($chName) 2]
	set forwardingHost [lindex $channels($chName) 3]
	set equateList [lindex $channels($chName) 4]
	

	#
	# Construct the ruleset for this channel, depending upon the table type
	#
	set thisChannelRuleset ""
	case $tableType {
	    file {

		if {[clength $tableFile] == 0 || [ctype space $tableFile]} {
		    error "Channel \"$chName\" has table type \"file\" but no table file defined."
		}
			
		append thisChannelRuleset \
		"R\$*<@\$+.>\$*\t\t\$: <\$(${chName}_MAP \$2 \$)> \$1<@\$2.>\$3\n"
		append thisChannelRuleset \
		"R<\$+.FOUND>\$+\t\t\$# $chName \$@ \$1 \$: \$2\n"
		append thisChannelRuleset \
		"R<\$+>\$+\t\t\t$@ \$2"

		# Add call to this channel's ruleset to Schannels
		append channelsRuleset "R\$*\t\t\t\$: \$>${chName} \$1\n"

		# Add the K map command too
		CF:Set $MA_SENDMAIL_FILEID map ${chName}_MAP \
		    " hash -o -a.FOUND $tableFile"

	    }

	    local {
		append thisChannelRuleset \
		"R\$*\t\t\t\$: \$>local_TBL \$1\n"
		if {[clength $forwardingHost] == 0} {
		    append thisChannelRuleset \
		    "R<FOUND>\$+\t\t\$# $chName \$: \$1"
		} else {
		    append thisChannelRuleset \
		    "R<FOUND>\$+\t\t\$# $chName \$@ $forwardingHost \$: \$1"
		}	

		# Add call to this channel's ruleset to Schannels
		append channelsRuleset "R\$*\t\t\t\$: \$>${chName} \$1\n"
	    }

	    baduser {
		if {$index != $numChannels} {
		    error "Baduser channel \"$chName\" is not last in order"
		}

		append thisChannelRuleset \
		"R\$+\t\t\t\$: \$>baduser_TBL \$1\n"

		if {[clength $forwardingHost] == 0} {
		    append thisChannelRuleset \
		    "R<FOUND>\$+\t\t\$# $chName \$: \$1"
		} else {
		    append thisChannelRuleset \
		    "R<FOUND>\$+\t\t\$# $chName \$@ $forwardingHost \$: \$1"
		}

		# Add call to baduser ruleset to S5, 
		# if not already there
		set baduserRule "\nR\$*\t\t\t\$: \$>${chName} \$1"
		set exp "\\\$>${chName}"
		set ruleset5 [CF:Get $MA_SENDMAIL_FILEID ruleset 5]
		set lastRuleset5 [lindex [split $ruleset5 "\n"] end]
		if {[regexp $exp $lastRuleset5] == 0} {
		    append ruleset5 $baduserRule
		    CF:Set $MA_SENDMAIL_FILEID ruleset 5 $ruleset5
		}
	    }

	    default {
		append thisChannelRuleset \
		"R\$*<@\$+.>\$*\t\t\$: \$>$MA_CH_RULESET($tableType) \$1<@\$2.>\$3\n"
		if {[clength $forwardingHost] == 0} {
		    append thisChannelRuleset \
		    "R<FOUND>\$*<@\$+.>\$*\t\$# $chName \$@ \$2 \$: \$1<@\$2.>\$3"
		} else {
		    append thisChannelRuleset \
		    "R<FOUND>\$*<@\$+.>\$*\t\$# $chName \$@ $forwardingHost \$: \$1<@\$2.>\$3"
		}

		# Add call to this channel's ruleset to Schannels
		append channelsRuleset "R\$*\t\t\t\$: \$>${chName} \$1\n"

	    }
	}
		
	# Set this channel's ruleset in the cf file
	CF:Set $MA_SENDMAIL_FILEID ruleset $chName $thisChannelRuleset

	# Set the mailer for this channel
	set equateString ""
	set fields [keylkeys equateList]
	set indexA [lsearch -exact $fields "A"]
	if {$indexA != -1} {
	    lvarpop fields $indexA
	    set argA [keylget equateList A]
	}
	foreach field $fields {
	    set arg [keylget equateList $field]
	    append equateString "${field}=${arg},"
	}
	if {$indexA != -1} {
	    append equateString "A=${argA}"
	}
	if {[clength $equateString] == 0} {
	    error "Channel \"$chName\" has no delivery agent equates specified"
	}
	CF:Set $MA_SENDMAIL_FILEID mailer $chName \
	    [string trim $equateString " \t,"]
    }

    # Set the Schannels in the cf file
    CF:Set $MA_SENDMAIL_FILEID ruleset channels \
	[string trim $channelsRuleset "\n"]
}
	
#
# ma_ch_names_get
#
# Purpose:
#	Returns a tcl list of all currently configured channel names.
#	This list is ordered by channel sequence.
#
proc ma_ch_names_get {} {
    global MA_SENDMAIL_FILEID channels
    set result {}

    if {$MA_SENDMAIL_FILEID == -1} {
	error "Sendmail configuration file is not open"
    }

    if {[info exists channels]} {
	set allNames [array names channels]
	set result [lsort -command mai_ch_order $allNames]
    }
    return $result
}

#
# ma_ch_sequence_get
#
# Purpose:
#	Returns the sequence of channel $chName.
#
proc ma_ch_sequence_get {chName} {
    global channels
    if {![info exists channels($chName)]} {
	error "Unknown channel \"$chName\""
    }
    return [lindex $channels($chName) 0]
}

#
# ma_ch_sequence_set
#
# Purpose:
#	Sets the sequence attribute of channel $chName to $sequence.
#
proc ma_ch_sequence_set {chName sequence} {
    global channels MA_CH_MODIFIED
    if {![info exists channels($chName)]} {
	error "Unknown channel \"$chName\""
    }

    lvarpop channels($chName)
    lvarpush channels($chName) $sequence
    set MA_CH_MODIFIED 1
}

#
# ma_ch_table_type_get
#
# Purpose:
#	Returns the table type for channel $chName.
#
proc ma_ch_table_type_get {chName} {
    global channels

    if {![info exists channels($chName)]} {
	error "Unknown channel \"$chName\""
    }

    return [lindex $channels($chName) 1]
}

#
# ma_ch_table_type_set
#
# Purpose:
#	Sets the table type for channel $chName to $type.
#
proc ma_ch_table_type_set {chName type} {
    global channels MA_CH_MODIFIED

    if {![info exists channels($chName)]} {
	error "Unknown channel \"$chName\""
    }

    if {[mai_ch_valid_table_type $type]} {
	lvarpop channels($chName) 1
	lvarpush channels($chName) $type 1
	set MA_CH_MODIFIED 1
    } else {
	error "Unknown channel table type \"$type\""
    }
}

#
# ma_machine_name_get
#
# Purpose:
#	Returns the channel table file path for channel $chName.
#	This may be an empty string, meaning none defined.
#
proc ma_ch_table_file_get {chName} {
    global channels

    if {![info exists channels($chName)]} {
	error "Unknown channel \"$chName\"" 
    }

    return [lindex $channels($chName) 2]
}

#
# ma_ch_table_file_set
#
# Purpose:
#	Sets the channel table file path for channel $chName to $file.
#	This may be an empty string.  NOTE: This table file attribute is 
#	ignored unless the table type is "file".
#
proc ma_ch_table_file_set {chName file} {
    global channels MA_CH_MODIFIED

    if {![info exists channels($chName)]} {
	error "Unknown channel \"$chName\""
    }

    lvarpop channels($chName) 2
    lvarpush channels($chName) $file 2
    set MA_CH_MODIFIED 1
}

#
# ma_ch_host_get
#
# Purpose:
#	Returns the forwarding host for channel $chName, if any.  This may
#	be an empty string, meaning none defined.
#
proc ma_ch_host_get {chName} {
    global channels

    if {![info exists channels($chName)]} {
	error "Unknown channel \"$chName\""
    }

    return [lindex $channels($chName) 3]
}

#
# ma_ch_host_set
#
# Purpose:
#	Sets the forwarding host for channel $chName, if any.  This may
#	be an empty string, meaning none defined.
#
proc ma_ch_host_set {chName host} {
    global channels MA_CH_MODIFIED

    if {![info exists channels($chName)]} {
	error "Unknown channel \"$chName\""
    }

    lvarpop channels($chName) 3
    lvarpush channels($chName) $host 3
    set MA_CH_MODIFIED 1
}

#
# ma_ch_equate_get
#
# Purpose:
#	Return the value of the equate $equate, for the mailer associated with
#	channel $chName.
#
proc ma_ch_equate_get {chName equate} {
    global channels
    set equateValue ""

    if {![info exists channels($chName)]} {
	error "Unknown channel \"$chName\""
    }

    set equateList [lindex $channels($chName) 4]
    keylget equateList $equate equateValue

    return $equateValue
}

#
# ma_ch_equate_set
#
# Purpose:
#	Set the equate $equateName to value $equateValue for the mailer
#	associated with channel $chName.
#
proc ma_ch_equate_set {chName equateName equateValue} {
    global channels MA_CH_MODIFIED

    if {![info exists channels($chName)]} {
	error "Unknown channel \"$chName\""
    }

    set equateList [lindex $channels($chName) 4]
    if {[clength $equateValue] == 0} {
	set currentEquates [keylkeys equateList]
	if {[lsearch -exact $currentEquates $equateName] != -1} {
	    keyldel equateList $equateName
	} else {
	    return
	}
    } else {
	keylset equateList $equateName $equateValue
    }

    set oldChannel $channels($chName)
    set newChannel [lreplace $oldChannel 4 4 $equateList]
    set channels($chName) $newChannel
    set MA_CH_MODIFIED 1
}

#
# ma_ch_create
#
# Purpose:
#	Creates a new channel named $chName, and initializes the channel to
#	default values.  It is assumed that all attributes will be defined
#	through the api calls that do this.
# Accepts:
#	chName		- string which is the channel name
# Returns:
#	"badname"	- if the name has invalid characters
#	"conflict"	- if there is a naming conflict
#	"ok"		- otherwise
#
proc ma_ch_create {chName} {
    global channels MA_CH_MODIFIED

    if {![mai_ch_valid_chars $chName]} {
	return "badname"
    }

    if {[cequal "$chName" ""]} {
	return "badname"
    }

    if {![mai_ch_valid_name $chName]} {
	return "conflict"
    }

    set sequence [llength [ma_ch_names_get]]
    set tableType "DNS"
    set tableFile ""
    set host ""
    set equates [mai_ch_default_equates]
    set channels($chName) [list $sequence $tableType $tableFile $host $equates]
    set MA_CH_MODIFIED 1
    return "ok"
}

#
# ma_ch_delete
#
# Purpose:
#	This function deletes the channel $chName from the configuration.
# Accepts:
#	chName	- string which is the channel name
# Returns:
#	none
#
proc ma_ch_delete {chName} {
    global MA_SENDMAIL_FILEID MA_CH_MODIFIED channels

    if {![info exists channels($chName)]} {
	error "Unknown channel \"$chName\""
    }

    CF:Unset $MA_SENDMAIL_FILEID ruleset $chName
    CF:Unset $MA_SENDMAIL_FILEID mailer $chName

    set tableType [lindex $channels($chName) 1]

    # if there is a channel table file, remove associated map config cmd
    if {[cequal $tableType "file"]} {
	CF:Unset $MA_SENDMAIL_FILEID map "${chName}_MAP"
    }

    # if this is the baduser channel, then remove subroutine call in S5
    if {[cequal $tableType "baduser"]} {

	set ruleset5 [split [CF:Get $MA_SENDMAIL_FILEID ruleset 5] "\n" ]
	set lastLine [lindex $ruleset5 end]

	if {[regexp {\$:[ ]*\$>([^ ]+)[ ]*\$1} $lastLine \
	    match subroutineName] == 1 && [cequal $subroutineName $chName]} {
	    lvarpop ruleset5 end
	    set newRuleset5 [join $ruleset5 "\n"]
	    CF:Set $MA_SENDMAIL_FILEID ruleset 5 $newRuleset5
	}
    }

    unset channels($chName)
    set MA_CH_MODIFIED 1
}

#
# ma_ch_rename
#
# Purpose:
#	Renames channel $oldName to new name $newName.
# Accepts:
#	oldName		- string which the channel's old name
#	newName		- string which the channel's new name
# Returns:
#	"badname"	- if the new name has invalid characters
#	"conflict"	- if there is a naming conflict for the new name
#	"ok"		- otherwise
#
proc ma_ch_rename {oldName newName} {
    global MA_SENDMAIL_FILEID MA_CH_MODIFIED channels

    if {![info exists channels($oldName)]} {
	error "Unknown channel \"$oldName\""
    }

    if {![mai_ch_valid_chars $newName]} {
	return "badname"
    }

    if {[cequal "$newName" ""]} {
	return "badname"
    }

    if {![mai_ch_valid_name $newName]} {
	return "conflict"
    }

    # create a new channel in our internal channel data structure,
    # using the old channel's data
    set channels($newName) $channels($oldName)

    # unset the cf lines associated with the old channel
    # (we don't have to set them here-- that will be done anyway in 
    # mai_ch_structure_commit)
    CF:Unset $MA_SENDMAIL_FILEID ruleset $oldName
    CF:Unset $MA_SENDMAIL_FILEID mailer $oldName
    set tableType [lindex $channels($oldName) 1]
    if {[cequal $tableType "file"]} {
	CF:Unset $MA_SENDMAIL_FILEID map "${oldName}_MAP"
    }
    # if this is the baduser channel, want to remove the last rule in
    # ruleset 5
    if {[cequal $tableType "baduser"]} {

	set ruleset5 [split [CF:Get $MA_SENDMAIL_FILEID ruleset 5] "\n" ]
	set lastLine [lindex $ruleset5 end]

	if {[regexp {\$:[ ]*\$>([^ ]+)[ ]*\$1} $lastLine \
	    match subroutineName] == 1 && [cequal $subroutineName $oldName]} {
	    lvarpop ruleset5 end
	    set newRuleset5 [join $ruleset5 "\n"]
	    CF:Set $MA_SENDMAIL_FILEID ruleset 5 $newRuleset5
	}
    }

    # remove the old channel from our internal channel data structure
    unset channels($oldName)
    set MA_CH_MODIFIED 1
    return "ok"
}


#
# ma_multihome_macro_set
#
# Purpose:
#	Sets the V macro to indicate that the multihome channel has been
#	created, and that the virtual domains map exists in the cf file.
#	This is used by the cf check_compat ruleset to know whether it should
#	check the virtual domains map or not.  If the V macro is set to
#	non-null string, the virtual domains map exists, otherwise it does not.
# Accepts:
#	true		- to set the V macro
#	false		- to unset the V macro
# Returns:
#	none
#
proc ma_multihome_macro_set {enable} {
    global MA_SENDMAIL_FILEID

    if {$MA_SENDMAIL_FILEID == -1} {
	error "Sendmail configuration file is not open"
    }

    if {$enable} {
	CF:Set $MA_SENDMAIL_FILEID macro V VIRTDOMAINS
    } else {
	CF:Set $MA_SENDMAIL_FILEID macro V ""
    }
}
