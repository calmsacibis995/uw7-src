#!/bin/osavtcl
#
#	@(#)ndprompter.BE	3.1	8/29/96	21:15:44
#       @(#) ndprompter.BE 12.7 96/01/31 
#
#	Copyright (C) 1993-1995 The Santa Cruz Operation, Inc.  
#	    All Rights Reserved.
#
#       The information in this file is provided for the exclusive use of
#       the licensees of The Santa Cruz Operation, Inc.  Such users have the
#       right to use, modify, and incorporate this code into other products
#       for purposes authorized by the license agreement provided they include
#       this notice and the associated copyright notice with any such product.
#       The information in this file is provided "AS IS" without warranty.
#
#cmdtrace on stderr

set lclPatchArea {__SSO_ID_ST@SCO-aga:Vidconf:5.2.2a                @SSO_ID_END__}
set SSOPATH "/usr/X11R6.1/lib/vidconf"

loadlibindex /usr/lib/sysadm.tlib

proc InitializeGlobals {} \
{
	global BUS_TYPE GFX_ROOT TMP_DIR SSO_ROOT SSOPATH

	set BUS_TYPE [exec uname -X | awk "/BusType/ \{ print \$3 \}"]

	set SSO_ROOT $SSOPATH/AOF
	source $SSO_ROOT/lib/libSCO.tcl

	set GFX_ROOT /usr/X11R6.1/lib/vidconf/AOF
	set TMP_DIR $GFX_ROOT/tmp
}

proc GetAdapterAttributes {acfg_output reconf sect ATTR_LIST} \
{
	global TMP_DIR

	set basic [ lindex $ATTR_LIST 0 ]
	set advanced [ lindex $ATTR_LIST 1 ]
	set hidden [ lindex $ATTR_LIST 2 ]

	foreach i {KEY REQUIRED ADVANCED} {
		set attrs [GetValueFromDB $acfg_output $sect $i]
		foreach j $attrs {
			set p [ lindex [ GetValueFromDB $acfg_output $j PROMPT no_errors] 0 ]
			if { $p == "" } {
				set new_p [ ParamPrompts $j ]
				if { $new_p != "" } {
					set p $new_p
				} else {
					set p $j
				}
			}

			if { $j == "MAC_ADDRESS" } {
				set w LABEL
			} else {
				set w ROLIST
			}
			
			set l [ list $j $p $w ]
			case $i {
			ADVANCED {
				lappend advanced $l
			}
			REQUIRED {
				lappend basic $l
			}
			KEY {
				# Only include the KEY attr if this is a
				# netconfig Reconf or the KEY is swconfigurable
				if { $reconf == "-r" } {
					lappend basic $l
				} else {
					set t [GetValueFromDB $acfg_output $j TYPE]
					set is_swconfig 0
					foreach u $t {
						if { $u == "swconfig" } {
							set is_swconfig 1
							break
						}
					}
					if { $is_swconfig } {
						lappend basic $l
					} else {
						lappend hidden $l
					}
				}
			}
			}
		}
	}
	return [ list $basic $advanced $hidden ]
}

proc CheckForConflicts {acfg_output attr value driver unit} \
{
#puts stderr "CheckForConflicts: $acfg_output $attr $value $driver $unit"
	set in_use [ AdapterSettingInUse $acfg_output $attr $value ]
	set prefix "*"
	if { $in_use == "" } {
		set prefix " "
	} else {
		set in_use_drv [ keylget in_use DRIVER ]
		set in_use_unit [ keylget in_use UNIT ]
#puts stderr "Conflict: $attr=$value with $in_use_drv/$in_use_unit"
		if { $in_use_drv == $driver && $in_use_unit == $unit } {
			set prefix " "
		}
	}
	return "$prefix[ FormatValue $value ]"
}

proc GetAdapterCurrent {acfg_output attr driver unit} \
{
	global CHANGE_LIST

	set val [ CheckForConflicts \
		$acfg_output $attr [ GetAdapterSetting $acfg_output $attr ] \
		$driver $unit \
	]
	foreach j $CHANGE_LIST {
		if { [ lindex $j 0 ] == $attr } {
			set val [ lindex $j 1 ]
			break
		}
	}

	return $val
}

proc GetAdapterCurrents {acfg_output attr_list driver unit} \
{
	set list ""
	foreach j $attr_list {
		set attr [ lindex $j 0 ]
		set val [ GetAdapterCurrent $acfg_output $attr $driver $unit ]

		lappend list [ list $attr 1 $val ]
	}
	return $list
}

proc GetAttributeValues {acfg_output attr driver unit} \
{
	global TMP_DIR

#puts stderr "GetAttrValues: $acfg_output $attr $driver $unit"
	set v [ ExpandRanges [ GetValueFromDB $acfg_output $attr VALUES] ]
	set cur_val [GetAdapterCurrent $acfg_output $attr $driver $unit ]

	set m ""
	foreach i $v {
		lappend m [ CheckForConflicts \
			$acfg_output $attr $i \
			$driver $unit \
		]
	}
	return $m
}

proc WriteOutAdapterSettings {acfg_output attr_list driver unit} \
{
	global TMP_DIR
	global GFX_ROOT

	set key_name [ GetValueFromDB $acfg_output ADAPTER KEY ]

	set run_acfg "N"

	set sd [ StanzaOpen $acfg_output STZ_RDWR ]
	foreach attrs $attr_list {
		foreach i $attrs {
			set attr [ lindex $i 0 ]
			set v [ GetAdapterCurrent $acfg_output \
				$attr $driver $unit \
			]
			set val [ UnFormatValue [ string trim $v " *" ] ]

			if { $attr == $key_name } {
				set old_key [ GetAdapterSetting $acfg_output $attr ]
			}
			set type [ GetValueFromDB $acfg_output $attr TYPE ]
			foreach j $type {
				if { $j == "swconfig" } {
					set run_acfg "Y"
				}
			}
			catch {StanzaAddSect $sd $attr}
			if [catch {StanzaAddAttr $sd $attr SELECT}] {
				StanzaDelAttr $sd $attr SELECT
				StanzaAddAttr $sd $attr SELECT
			}
			StanzaAddValue $sd $attr SELECT $val
		}
	}
	StanzaClose $sd

	set ret 0
	if { $run_acfg == "Y" } {
		set ACFG "$GFX_ROOT/ID/$driver/acfg"
		if [ file executable $ACFG ] {
			set ret [ system "$ACFG -k $old_key -f $acfg_output" ]
		}
	}

	return $ret
}

proc CheckMACaddress {arg1} \
{
	upvar $arg1 addr

	set l [ translit : " " $addr ]
	case [ llength $l ] {
	1 {
		if { $addr == " default" || $addr == "default" } {
			return ""
		}
		if { [ string length $addr ] != 12 } {
			return "Incorrect length. MAC Address must be of the form: xx:xx:xx:xx:xx:xx"
		}
		set newaddr ""
		for {set i 0} { $i < 12 } {incr i 2} {
			set s [ string range $addr $i [ expr $i+1 ] ]
			if { ! [ string match "\[0-9a-fA-F\]\[0-9a-fA-F\]" $s ] } {
				return "MAC Address must be of the form: xx:xx:xx:xx:xx:xx"
			}
			if { $newaddr == "" } {
				set newaddr $s
			} else {
				append newaddr ":$s"
			}
		}
		set addr $newaddr
		return ""
	}
	6 {
		foreach i $l {
			if { ! [ string match "\[0-9a-fA-F\]\[0-9a-fA-F\]" $i ] } {
				return "MAC Address must be of the form: xx:xx:xx:xx:xx:xx"
			}
		}
		return ""
	}
	default {
		return "MAC Address must be of the form: xx:xx:xx:xx:xx:xx"
	}
	}
}

# MAIN Main main

ErrorTopLevelCatch {

InitializeGlobals

set init_reconf [lindex $argv 0]
set acfg_output [lindex $argv 1]
set driver [ lindex $argv 2 ]
set unit [ lindex $argv 3 ]

fcntl stdout NOBUF 1
fcntl stderr NOBUF 1

#set acfg_output $TMP_DIR/acfg_output
set ATTR_LIST ""
set ATTR_LIST [ GetAdapterAttributes $acfg_output $init_reconf \
		ADAPTER $ATTR_LIST ]

# more adapter info in the DOF file
if {[GetValueFromDB $acfg_output ADAPTER MEDIA_TYPE] == "token-ring"} {
	set ATTR_LIST [ GetAdapterAttributes $acfg_output $init_reconf \
			SOURCE_ROUTING $ATTR_LIST]
}
set CHANGE_LIST ""

while { [ gets stdin line ] != -1 } {
	case [ lindex $line 0 ] {
	LIST {
		case [ lindex $line 1 ] {
		BASIC {
			echo [ lindex $ATTR_LIST 0 ]
		}
		ADVANCED {
			echo [ lindex $ATTR_LIST 1 ]
		}
		default {
			puts stderr "netBE: Unknown argument to LIST"
			exit 1
		} }
	}
	CURRENT {
		case [ lindex $line 1 ] {
		BASIC {
			set list [ lindex $ATTR_LIST 0 ]
		}
		ADVANCED {
			set list [ lindex $ATTR_LIST 1 ]
		}
		default {
			puts stderr "BE: Unknown argument to LIST"
			exit 1
		} }
		echo [ GetAdapterCurrents $acfg_output $list $driver $unit ]
	}
	VALUES {
		echo [ GetAttributeValues $acfg_output \
			[ lindex $line 1 ] $driver $unit \
		]
	}
	SET {
		set attr [ lindex $line 1 ]
		set value [ lindex $line 2 ]

		if { $attr == "MAC_ADDRESS" } {
			set result [ CheckMACaddress value ]
			if { $result != "" } {
				echo "ERROR \"$result\""
				continue
			}
		}
		set x ""
		set found 0
		foreach i $CHANGE_LIST {
			
			if { [ lindex $i 0 ] == $attr } {
				lappend x [ list $attr $value ]
				set found 1
			} else {
				lappend x $i
			}
		}
		if {!$found} {
			lappend x [ list $attr $value ]
		}
		set CHANGE_LIST $x
		echo "OK"
	}
	USER_DONE {
		set ret [ WriteOutAdapterSettings \
			$acfg_output $ATTR_LIST $driver $unit \
		]
		set ndinfo ""
		foreach attr [ lindex $ATTR_LIST 0 ] {
			set types [GetValueFromDB $acfg_output [lindex $attr 0] TYPE]
			set swconfig 0
			set lk 0
			foreach j $types {
				case $j {
				swconfig {
					set swconfig 1
				}
				link-kit {
					set lk 1
				} }
			}
			if { $lk && ! $swconfig } {
				set ndinfo NDTYPE_READ_ONLY
			}
		}
		if { $ret } {
			echo "ERROR"
		} else {
			echo "OK $ndinfo"
		}
	}
	default {
		puts stderr "BE: Unknown request <$line>"
		exit 1
	} }
	flush stdout
}
} ndprompter.BE
