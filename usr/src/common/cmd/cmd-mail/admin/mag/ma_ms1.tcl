#ident "@(#)ma_ms1.tcl	11.2"
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
#===============================================================================
#

#
# Message Store Version 1 config file routines
#
# ma prefix is for externally visible routines.
# mai prefix is for internal "static" routines.
#

#
# read file into memory
#
proc \
ma_ms1_open { path } \
{
	global ma_ms1_db ma_ms1_path ma_ms1_index
@if test
	global TEST
@endif

@if test
	if {"$TEST" == "grp6_open"} {
		system "rm -f $path"
	}
@endif
	set ma_ms1_path $path
	if {[catch {open $path r} fd] != 0} {
		return fail
	}

	# read in file
	set ma_ms1_db ""
	set ma_ms1_index ""
	set linenum -1
	while {[gets $fd line] != -1} {
		set linenum [expr $linenum + 1]
		lappend ma_ms1_db $line
		# check that line is valid syntax
		if {[mai_ms1_skip_line $line] == "ok"} {
			continue
		}
		set list [mai_ms1_parse_line $line]
		if {$list == "parserr"} {
			close $fd
			return parserr
		}
		# check if valid keyword
		set keyword [lindex $list 0]
		if {[catch {ma_ms1_default $keyword}] != 0} {
			return parserr
		}
		set value [lindex $list 1]
		if {"[mai_ms1_valid $keyword $value]" == "fail"} {
			return parserr
		}
		set list [list $keyword $linenum]
		lappend ma_ms1_index $list
	}
	close $fd
	return ok
}

proc \
ma_ms1_get { keyword } \
{
	global ma_ms1_db

	set index [mai_ms1_find $keyword]
	if {$index == -1} {
		return [ma_ms1_default $keyword]
	}
	set line [lindex $ma_ms1_db $index]
	set list [mai_ms1_parse_line $line]
	set value [lindex $list 1]
	return $value
}

proc \
ma_ms1_set { keyword value } \
{
	global ma_ms1_db

	set index [mai_ms1_find $keyword]
	if {"[mai_ms1_valid $keyword $value]" == "fail"} {
		error "Invalid value for $keyword: $value"
	}
	if {$index == -1} {
		# not exist
		lappend ma_ms1_db "${keyword}=${value}"
	} else {
		# exists
		set ma_ms1_db [lreplace $ma_ms1_db $index $index "${keyword}=${value}"]
	}
}

proc \
ma_ms1_default { keyword } \
{
	switch $keyword {
	"MS1_FOLDER_FORMAT" {
		return "MMDF"
	}
	"MS1_INBOX_DIR" {
		return "/var/mail"
	}
	"MS1_INBOX_NAME" {
		return ""
	}
	"MS1_FSYNC" {
		return "FALSE"
	}
	"MS1_EXTENDED_CHECKS" {
		return "FALSE"
	}
	"MS1_EXPUNGE_THRESHOLD" {
		return "50"
	}
	"MS1_FOLDERS_INCORE" {
		return "FALSE"
	}
	"MS1_FILE_LOCKING" {
		return "FALSE"
	}
	"MS1_LOCK_TIMEOUT" {
		return "10"
	}
	"MS1_UMASK" {
		return "077"
	}
	}
	error "Unknown keyword: $keyword"
}

proc \
ma_ms1_write {} \
{
	global ma_ms1_db ma_ms1_path
@if test
	global TEST

	if {"$TEST" == "grp6_write"} {
		system "rm -f $ma_ms1_path"
		# this makes the open fail
		system "mkdir $ma_ms1_path"
	}
@endif
	if {[catch {open $ma_ms1_path w} fd] != 0} {
		return fail
	}

	# write out file
	foreach item $ma_ms1_db {
		puts $fd $item
	}
	close $fd
	return ok
}

#
# validate keyword and value, returns ok and fail
#
proc \
mai_ms1_valid { keyword value } \
{
	switch $keyword {
	"MS1_FOLDER_FORMAT" { 
		set val [string toupper $value]
		if {"$val" == "SENDMAIL"} {
			return ok
		}
		if {"$val" == "MMDF"} {
			return ok
		}
		return fail
	}
	"MS1_INBOX_DIR" { return ok }
	"MS1_INBOX_NAME" { return ok }
	"MS1_FSYNC" {
		set val [string toupper $value]
		if {"$val" == "TRUE"} {
			return ok
		}
		if {"$val" == "FALSE"} {
			return ok
		}
		return fail
	}
	"MS1_EXTENDED_CHECKS" {
		set val [string toupper $value]
		if {"$val" == "TRUE"} {
			return ok
		}
		if {"$val" == "FALSE"} {
			return ok
		}
		return fail
	}
	"MS1_EXPUNGE_THRESHOLD" {
		if {[ctype digit $value] == 0} {
			return fail
		}
		while {"[csubstr $value 0 1]" == "0" && $value != 0} {
			return fail
		}
		if {$value > 100} {
			return fail
		}
		return ok
	}
	"MS1_FOLDERS_INCORE" {
		set val [string toupper $value]
		if {"$val" == "TRUE"} {
			return ok
		}
		if {"$val" == "FALSE"} {
			return ok
		}
		return fail
	}
	"MS1_FILE_LOCKING" {
		set val [string toupper $value]
		if {"$val" == "TRUE"} {
			return ok
		}
		if {"$val" == "FALSE"} {
			return ok
		}
		return fail
	}
	"MS1_UMASK" {
		if {[ctype digit $value] == 0} {
			return fail
		}
		if {[string first 8 $value] >= 0} {
			return fail
		}
		if {[string first 9 $value] >= 0} {
			return fail
		}
		# first digit must be zero
		if {"[cindex $value 0]" != "0"} {
			return fail
		}
		# valid octal number, now do range check,
		# leading zero makes base 8 in tcl just like in C.
		if {$value > 0777} {
			return fail
		}
		return ok
	}
	"MS1_LOCK_TIMEOUT" {
		if {[ctype digit $value] == 0} {
			return fail
		}
		if {$value > 0} {
			return ok
		}
		return fail
	}
	}
	error "Unknown keyword: $keyword"
}

proc \
mai_ms1_find { keyword } \
{
	global ma_ms1_index

	set count [llength $ma_ms1_index]
	set value ""
	loop i 0 $count {
		set list [lindex $ma_ms1_index $i]
		set lkeyword [lindex $list 0]
		if {$lkeyword == $keyword} {
			return [lindex $list 1]
		}
	}
	return -1
}

proc \
mai_ms1_skip_line { line } \
{
	if {[string length $line] == 0} {
		return ok
	}
	if {[cindex $line 0] == "#"} {
		return ok
	}
	return fail
}

proc \
mai_ms1_parse_line { line } \
{

# find '=' and split
# strip white space
# then strip quotes
	set index [string first "=" $line]
	if {$index == "-1"} {
		return parserr
	}
	set left [csubstr $line 0 $index]
	set left [string trim $left]
	set index [expr $index + 1]
	set right [csubstr $line $index end]
	set right [string trim $right]
	set right [string trim $right "\""]
	if {$left == ""} {
		return parserr
	}
	set list [list "$left" "$right"]
	lappend list $left $right
	return $list
}
