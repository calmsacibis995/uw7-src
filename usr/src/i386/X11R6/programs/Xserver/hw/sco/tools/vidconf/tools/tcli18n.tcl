#!/bin/tcl

#
#	@(#)tcli18n.tcl	3.1	8/29/96	21:25:09
#	@(#) tcli18n.tcl 12.1 95/05/09 
#
#	Copyright (C) The Santa Cruz Operation, 1994.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#
#
#	SCO MODIFICATION HISTORY
#
#
#

# For Tcl files, the default strings are provided by textual inclusion into
# the source file.  Therefore, only need to ensure that the message catalogue
# label references are all correct.
#
# Assumptions:
#	- there is only one call to IntlLocalizeMsg per line in the Tcl file.
#
proc CheckTclFile {fileContents msgfile} {
	set lineNum 0
	set continuation 0
	foreach l $fileContents {
		incr lineNum

		if {$continuation} then {
			# The message label will be the first item on the line.
			# (i.e. there was a backslash immediately after the
			# IntlLocalizeMsg call).
			#
			set s [string trim [string trimright $l {\\}]]
			set t [split $s { 	[]}]
			set label [lindex $t 0]
			CheckLabel $label $lineNum

			set continuation 0
		}
		if {[string match {*IntlLocalizeMsg*} $l] == 0} then {
			continue
		}
		set i [string first IntlLocalizeMsg $l]
		set s [string trim [string trimright \
					[string range $l $i end] {\\}]]
#echo "line: '$s'"
		set t [split $s { 	[]}]
		if {[llength $t] > 1} then {
			set label [lindex $t 1]
			CheckLabel $label $lineNum
		} else {
			set continuation 1
		}
	}
	CheckLabelReferences
}

# Ensure that the given label is in the message label list (built from the
# corresponding .msg file).
#
proc CheckLabel {label lineNum} {
	global msgLabelList msgLabelRefs

#echo "Checking label: '$label'"
	if {[lsearch $msgLabelList $label] == -1} then {
		echo "Error at line $lineNum: Label '$label' undefined."
	} else {
		incr msgLabelRefs($label)
	}
}

# Print a warning for all labels which weren't referenced in the source file.
#
proc CheckLabelReferences {} {
	global msgLabelList msgLabelRefs

	foreach l $msgLabelList {
		if {$msgLabelRefs($l) == 0} then {
			echo "Warning: message label '$l' is unused."
		}
	}
}

# For C files, the symbolic message names are #included from a .h file.
# If a message doesn't exist it will give a compile time error.  Therefore,
# it is only necessary to ensure that the default strings are consistent
# between the string used in the catgets call and that defined in the message
# catalogue input file.
#
# Assumptions:
#	- assume that all calls to catgets should be checked, including those
#	  which might be commented out.
# 
proc CheckCFile {filename fileContents} {
	global msgLabelList msgLabelRefs msgLabelStrs

	foreach l $msgLabelList {
		if {[set i [lsearch $fileContents "*${l}[ 	],*"]] == -1} then {
			echo "Error: Message label '$l' not found in $filename."
			continue
		}
		CheckString $l \
			$msgLabelStrs($l) \
			[lrange $fileContents $i end] \
			$i
	}
}

# Verify that the default string in the C source file for the given message 
# catalogue matches the given string (from the message message catalogue
# input file).
#
proc CheckString {label str fileC lineNum} {
#echo "label: '$label', string '$str', lineNum '$lineNum'"
	# The default string is the next argument after the symbolic
	# label; isolate everything on the line after the label.
	#
	set index 0
	set iline [lindex $fileC $index]
	set j [string first $label $iline]
	set s [string trim \
		[string range $iline [expr {$j + [string length $label]}] end]]
#echo "line: '$iline', isolated string: '$s'"

	if {[regsub {.*(".*").*} $s {\1} quotedStr]} then {
#echo "found string: '$quotedStr'"
		if {[string compare $quotedStr $str] != 0} then {
			echo "Error at line [expr {$lineNum + 1}]: Default string for label '$label' does not match."
		}
	} else {
		# The default string was not on the same line.  Find
		# the next line in the source file with a string on it.
		# (there may be intervening comments).
		#
		incr index
		while {$index < [llength $fileC]} {
			set s [string trim [lindex $fileC $index]]

			if {[regsub {.*(".*").*} $s {\1} quotedStr]} then {
#echo "(continuation) found string: '$quotedStr'"
				if {[string compare $quotedStr $str] != 0} \
				then {
					echo "Error at line [expr {$lineNum + 1}]: Default string for label '$label' does not match."
				}
				break
			} else {
				# No string on this line, check the
				# next line.
				#
				incr index
			}
		}
	}
	incr index
	set remainingFile [lrange $fileC $index end]
	if {[set i [lsearch $remainingFile "*${label}[ 	],*"]] != -1} then {
		# Another occurrence of the label has been found.  Recursively
		# call this function to ensure that this (and any other 
		# subsequent references) to the message label are checked.
		#
		CheckString $label \
			$str \
			[lrange $remainingFile $i end] \
			[expr {$lineNum + $index + $i}]
	}
}

# Build a list and two arrays (indexed by the message catalogue symbolic name)
# for the messages defined in the message catalogue input file:
#
#	msgLabelList : a list of symbolic message catalogue names.
#	msgLabelRefs : an array of reference counts.
#	msgLabelStrs : an array of message catalogue strings.
#
# All of these lists have one-to-one correspondance.
#
#
proc BuildMsgLabelList {msgfile} {
	global msgLabelList msgLabelRefs msgLabelStrs

	set msgs [ReadFile $msgfile]

	set msgLabelList {}
	set continuation 0
	foreach l $msgs {
		set s [string trim $l]
		if {$continuation == 1} then {
			append str [string trimright $s {\\}]
			if {![string match {*\\} $s]} then {
#echo "multi-line string: '$str'"
				set msgLabelStrs($label) $str
				set continuation 0
			}
			continue
		}
		if {$s != "" && [string match {\$*} $s] == 0} then {
			set t [split $s]
			set label [lindex $t 0]
			lappend msgLabelList $label
			set msgLabelRefs($label) 0
			set str [join [lrange $t 1 end]]
#echo "string: '$str'"
			if {[string match {*\\} $str]} then {
				# The string is continued on the next line.
				# Remove the backslash and continue.
				#
				set str [string trimright $str {\\}]
				set continuation 1
			} else {
				set msgLabelStrs($label) $str
			}
		}
	}
#echo "msgLabelList '$msgLabelList'"
}

proc ReadFile {file} {
	if {[set fd [open $file r]] == -1} then {
		puts stderr \
			"Unable to open input file '$file'"
		exit 1
	}
	set fileC [split [read $fd] "\n"]
	close $fd

	return $fileC
}


# MAIN Main main

set scriptname [file tail [file root [info script]]]
if {[llength $argv] != 1} then {
	puts stderr "usage: $scriptname source_file"
	exit 1
}
set filename [lindex $argv 0]

# Determine the type of file and record the base filename.
#
set i [string last . $filename]
incr i
set suffix [string range $filename $i end]
set basename [string range $filename 0 [expr {$i - 2}]]

# Ensure the message catalogue input file exists.
#
#set msgfile "$basename.msg"
set msgfile vidconfGUI.msg
if {![file exists $msgfile]} then {
	echo "Error: Message catalogue input file '$msgfile' does not exist."
	exit 1
}

BuildMsgLabelList $msgfile
set fileContents [ReadFile $filename]

case $suffix in {
tcl {
	echo "Checking message catalogue consistency for Tcl file: '$filename'"
	CheckTclFile $fileContents $msgfile
}
c {
	echo "Checking message catalogue consistency for C file: '$filename'"
	CheckCFile $filename $fileContents
}
default {
	echo "Unrecognized file type: '$suffix'"
	exit 1
}
}
