#
#	@(#)libSCO.tcl	3.1	8/29/96	21:15:41
#        @(#) libSCO.tcl 12.3 95/07/20 
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

proc OpenStanza {file} \
{
	global STANZAS_OPEN STZO_LIST

	file stat $file foo
	if { [ info exists STANZAS_OPEN($file) ] == 0 } {
		if { ! [ info exists STZO_LIST ] } {
			set STZO_LIST $file
		} else {
			set STZO_LIST [ linsert $STZO_LIST 0 $file ]
		}
#puts stderr "OS: Open $file ($STZO_LIST)"
		set stz [StanzaOpen "$file" STZ_RDONLY]
		keylset STANZAS_OPEN($file) FD $stz
		keylset STANZAS_OPEN($file) INO $foo(ino)
		keylset STANZAS_OPEN($file) MTIME $foo(mtime)

	} else {
		if { [ keylget STANZAS_OPEN($file) INO ] == $foo(ino) && \
		     [ keylget STANZAS_OPEN($file) MTIME ] == $foo(mtime) } {
			set stz [ keylget STANZAS_OPEN($file) FD ]
		} else {
#puts stderr "OS: Reopen $file"
			StanzaClose [ keylget STANZAS_OPEN($file) FD ]

			set stz [StanzaOpen "$file" STZ_RDONLY]
			keylset STANZAS_OPEN($file) FD $stz
			keylset STANZAS_OPEN($file) INO $foo(ino)
			keylset STANZAS_OPEN($file) MTIME $foo(mtime)
			set inx [ lsearch $STZO_LIST $file ]
			set STZO_LIST [ linsert \
				[ lreplace $STZO_LIST $inx $inx ] \
				0 $file \
			]
		}
	}
	set len [ llength $STZO_LIST ]
#puts stderr "$len: $STZO_LIST"
	if { $len > 5 } {
		set file [ lindex $STZO_LIST [ expr $len-1 ] ]
#puts stderr "OS: Close $file"
		set STZO_LIST [ lrange $STZO_LIST 0 [ expr $len-2 ] ]
		StanzaClose [ keylget STANZAS_OPEN($file) FD ]
		unset STANZAS_OPEN($file)
	}
	return $stz
}

# Get a value from a stanza file.  If the SECTION:ATTRIBUTE does not exist,
# then the function returns "".  Otherwise the value of the SECTION:ATTRIBUTE
# is returned as the only element of a list.
#
# Use GetValueFromDB or GetValueFromStanza (see below) instead of this function.
proc GVFStanza {file sect attr} \
{
	set stz [ OpenStanza $file ]
#puts stderr "STZ:$file == $stz"
	if { [ lsearch [StanzaGetSects $stz] $sect ] == -1 } {
		return ""
	}
	if { [ lsearch [StanzaGetAttrs $stz $sect] $attr ] == -1 } {
		return ""
	}
	return [ list [ StanzaGetVals $stz $sect $attr ] ]
}

# Gets the value of SECTION:ATTRIBUTE from the Stanza file.  If the attr does
# not exist or contains an no value:
# E.g.
#	ADAPTER:
#		REQUIRED=
# then the function returns "".
proc GetValueFromStanza {file sect attr} \
{
	return [ lindex [ GVFStanza $file $sect $attr ] 0 ]
}

proc GetAOFPath {file} \
{
	global GFX_ROOT

	set aof_path [ GVFStanza $file ADAPTER AOF ]

	if { $aof_path == "" } {
		error "GetValueFromDB($file):No ADAPTER:AOF attr"
	}
#puts stderr "GetAOFPath returns $GFX_ROOT/ID/$aof_path"
	return $GFX_ROOT/ID/$aof_path
}

proc StripAOFPath {aof_file} \
{
	# Strip off the <ver>/ID portion of the AOF
	# path. (or //opt/K/SCO in the case of ndipu)
	regsub {..*/ID/} $aof_file {} aof_file
#puts stderr "StripAOFPath return $aof_file from"
	return $aof_file
}

# MDIDriverNameFromAOF depends on a relative AOF value; e.g., wdn/AOF/wdn-isa
proc MDIDriverNameFromAOF {aof_file} \
{
#puts stderr " MDIDriverNameFromAOF($aof_file)"
	set aof [ GVFStanza $aof_file ADAPTER AOF ]
	return [ string range $aof 0 [expr { [ string first "/" $aof ] - 1 }] ]
}

proc GetDOFPath {} \
{
	global SSO_ROOT

	return $SSO_ROOT/dlpi/dlpi/DOF
}

# Gets the value of SECTION:ATTRIBUTE from the Stanza file.  If is it not found
# in the stanza file 'file' then the AOF associated with it is searched for the
# value.  If the attribute still cannot be found:
#	IF err="no_errors" THEN "" is returned OTHERWISE an error is generated
proc GetValueFromDB {file sect attr {err ""}} \
{
	set val [ GVFStanza $file $sect $attr ]
	if { $val != "" } {
		return [ lindex $val 0 ]
	}

	set aof_path [ GetAOFPath $file ]
	set val [ GVFStanza $aof_path $sect $attr ]
	if { $val != "" } {
		return [ lindex $val 0 ]
	}

	set dof_path [ GetDOFPath ]
	set val [ GVFStanza $dof_path $sect $attr ]
	if { $val == "" && $err != "no_errors" } {
		error "GetValueFromDB($file):No $sect:$attr attr"
	}

	return [ lindex $val 0 ]
}

# Used by AdapterSettingInUse.  Reads all the mdevice/sdevice files in the
# link-kit and generates 4 global lists:
#	BASE_IO_USED	{ driver_name start end }...
#	IRQ_USED	{ driver_name irq }...
#	RAM_USED	{ driver_name start end }...
#	DMA_USED	{ driver_name dma }...
proc ReadLinkKitFiles {{master /etc/conf/cf.d/mdevice} {system /etc/conf/sdevice.d}} \
{
	global HW_DEP_USED
	global BASE_IO_USED
	global IRQ_USED
	global RAM_USED
	global DMA_USED

	set HW_DEP_USED ""
	set BASE_IO_USED ""
	set IRQ_USED ""
	set RAM_USED ""
	set DMA_USED ""

	set mfd [ open $master ]
	while { [ gets $mfd line ] != -1 } {
		# Ignore comments
		if { [ string index $line 0 ] == "*" } {
			continue
		}
		scan $line "%s %s %s %s %d %d %d %d %d" \
			driver funcs chars prefix \
			bmajor cmajor minu maxu dma

		set driver_configured 0

		set sfd [ open $system/$driver ]
		set line_no -1
		while { [ gets $sfd line ] != -1 } {
			# Ignore comments
			if { [ string index $line 0 ] == "*" } {
				continue
			}
			incr line_no
			scan $line "%s %s %d %d %d %d %x %x %x %x" \
				driver_name in_use \
				hw_dep ipl_level irq_type irq \
				bio_st bio_end ram_st ram_end
			if { $in_use == "N" || $in_use == "n" } {
				continue
			}
			set driver_configured 1
			if { $hw_dep != 0 } {
				set x ""
				keylset x DRIVER $driver
				keylset x UNIT $line_no
				keylset x VALUE $hw_dep
				lappend HW_DEP_USED $x
			}
			if { $irq != 0 } {
				set x ""
				keylset x DRIVER $driver
				keylset x UNIT $line_no
				keylset x VALUE $irq
				lappend IRQ_USED $x
			}
			if { $bio_st != 0 } {
				set x ""
				keylset x DRIVER $driver
				keylset x UNIT $line_no
				keylset x START $bio_st
				keylset x END $bio_end
				lappend BASE_IO_USED $x
			}
			if { $ram_st != 0 } {
				set x ""
				keylset x DRIVER $driver
				keylset x UNIT $line_no
				keylset x START $ram_st
				keylset x END $ram_end
				lappend RAM_USED $x
			}
		}
		close $sfd
		if { $driver_configured == 1 && $dma != -1 } {
			set x ""
			keylset x DRIVER $driver
			keylset x UNIT $line_no
			keylset x VALUE $dma
			lappend DMA_USED $x
		}
	}
	close $mfd
}

proc NetToMDIdrv {net} \
{
	global GFX_ROOT

	set GFXMDI_PATH $GFX_ROOT/llimdi

	set result ""
	set ifd [ open $GFXMDI_PATH ]
	while { [ gets $ifd line ] != -1 } {
		set e [ split $line : ]
		
		if { [ lindex $e 0 ] == $net } {
			set d [ lindex $e 1 ]
			
			set drv [ string trimright $d 0123456789 ]
			set drv_len [ string length $drv ]
			keylset result DRIVER $drv
			keylset result UNIT [ string range $d $drv_len end ]
			break
		}
	}
	close $ifd
	return $result
}


# Returns conflict list DRIVER UNIT VALUE.  Returns "" if it is free.
proc AdapterSettingInUse {acfg_output attr val} \
{
	global GFX_ROOT

	case $attr {
	BASE_IO {
		global BASE_IO_USED
		if { ! [ info exists BASE_IO_USED ] } {
			ReadLinkKitFiles
		}
		scan $val "%x" start 
		scan [ GetValueFromDB $acfg_output BASE_IO SIZE ] "%x" size
		set end [ expr { $start + $size } ]
		foreach i $BASE_IO_USED {
			if { $start <= [ keylget i START ] && \
			     $end >= [ keylget i END ] } {
				return $i
			}
		} }
	IRQ	{
		global IRQ_USED
		if { ! [ info exists IRQ_USED ] } {
			ReadLinkKitFiles
		}
		if { $val == 2 } {
			set val 9
		}
		foreach i $IRQ_USED {
			if { $val == [ keylget i VALUE ] } {
				return $i
			}
		} }
	{ RAM ROM } {
		global RAM_USED
		if { ! [ info exists RAM_USED ] } {
			ReadLinkKitFiles
		}
		if { [ scan $val "%x(%d)" start size ] != 2 } {
			if { [ scan $val "%x" start ] != 1 } {
				return ""
			} else {
				# DEFAULT values in AOF file is given
				# in the form of "start"
				foreach i $RAM_USED {
					if { $start == [ keylget i START ] } {
						return $i
					}
				}
			}
		} else {
			# DEFAULT values in AOF file is given
			# in the form of "start(size)"
			set end [ expr { $start + $size * 1024 - 1 } ]
#			puts stderr "start $start size $size end $end"
			foreach i $RAM_USED {
				if { $start <= [ keylget i START ] && \
				     $end >= [ keylget i END ] } {
					return $i
				}
			}
		}
	}
	DMA	{
		global DMA_USED
		if { ! [ info exists DMA_USED ] } {
			ReadLinkKitFiles
		}
		foreach i $DMA_USED {
			if { $val == [ keylget i VALUE ] } {
				return $i
			}
		}
	}
	SLOT {
		# Search all the sysdbs for conflicts
		foreach i [ glob -nocomplain $GFX_ROOT/sysdb/net\[0-9\] ] {
			set x [ GetValueFromDB $i SLOT SELECT no_errors ]
			if { $x != "" && $val == $x } {
				return [ NetToMDIdrv [ file tail $i ] ]
			}
		}
	}
	default	{
		foreach i [ GetValueFromDB $acfg_output $attr TYPE ] {
			if { $i == "unique" } {
				set drv [ MDIDriverNameFromAOF $acfg_output ]
				foreach i [glob -nocomplain $GFX_ROOT/sysdb/\[0-9\]] {
					set x [ GetValueFromDB $i $attr SELECT no_errors ]
					set sysdrv [ MDIDriverNameFromAOF $i ]
					if { $x != "" && $val == $x && $drv == $sysdrv } {
						set ret [ NetToMDIdrv [ string range $i [ expr { [ string last "/" $i ] + 1 } ] end ] ]
						return $ret
					}
				}
			}
		}
	} }
	return ""
}

# Converts the internal representation of an area of memory:
#	%x(%d)			E.g.	d0000(16)
# to a more human readable form:
#	%x-%x (%dk)		E.g.	d0000-d3fff (16k)
# This function is harmless to strings not of the form %x(%d) and will return
# them unchanged.
proc FormatValue {i} \
{
	if { [ scan $i "%x(%d)" start size ] == 2 } {
		set end [ expr { $start + $size * 1024 - 1 } ]
		return [ format "%x-%x (%dk)" $start $end $size ]
	} else {
		return $i
	}
	
}

# Does the opposite of FormatValue {i}
proc UnFormatValue {i} \
{
	if { [ scan $i "%x-%x (%dk)" start end size ] == 3 } {
		return [ format "%x(%d)" $start $size ]
	} else {
		return $i
	}
}

# Expand sizelist of the form %d|%d|... to
#	{$start $size1} {$start $size2}...
# Used to expand:
#	d0000(8|16|32)
# to
#	d0000(8) d0000(16) d0000(32)
proc ExpandSizeList {start sizelist} \
{
	set v ""
	foreach size [ translit "|" " " $sizelist ] {
		lappend v [ format "%x(%d)" $start $size ]
	}
	return $v
}

# Expand all ranges in the AOF file ATTRIBUTE:VALUES attribute.  Input is of
# the form:
#	values         ::= value (' ' value)*
#	value          ::= string | range | region | region_range
#	range          ::= start '-' end ':' step
#	region         ::= start '(' sizelist ')'
#	region_list    ::= start '-' end ':' step '(' sizelist ')'
#	sizelist       ::= size ('|' size)*
#	start,end,step ::= hex_number
#	size           ::= decimal_number
# Output is of the form:
#	output         ::= output_value ( ' ' output_value )*
#	output_value   ::= string | simple_region
#	simple_region  ::= start '(' size ')'
proc ExpandRanges {values} \
{
	set v ""
	foreach i $values {
		if { [ scan $i "%x-%x:%x(%\[^)\])" start end step sizelist ] == 4 } {
			for {set j $start} {$j <= $end} {incr j $step} {
				set v [concat $v [ExpandSizeList $j $sizelist ]]
			}
			continue
		}
		if { [ scan $i "%x-%x:%x" start end step ] == 3 } {
			for {set j $start} {$j <= $end} {incr j $step} {
				lappend v [ format "%x" $j ]
			}
			continue
		}
		if { [ scan $i "%x(%\[^)\])" start sizelist ] == 2 } {
			set v [ concat $v [ ExpandSizeList $start $sizelist ] ]
			continue
		}
		lappend v $i
	}
	return $v
}

# Returns the value for ATTR in the INITDB:
#	1.  Look up the value in $acfg_output.  If its there, return it.
#	3.  Look up the value in ATTR:DEFAULTS.  If there is a non-conflicting
#		value there, return it.
#	4.  Look up the value in ATTR:VALUES.  If there is a non-conflicting
#		value there, return it.
#	5.  Return the first element in the ATTR:VALUES list.
proc GetAdapterSetting {acfg_output attr} \
{
	set val [ GetValueFromDB $acfg_output $attr SELECT no_errors ]
	if { $val != "" } {
		return [ FormatValue [ lindex $val 0 ] ]
	}

	# Manufacture a setting from the ATTR:DEFAULTS attribute
	set def [ GetValueFromDB $acfg_output $attr DEFAULTS no_errors ]
	foreach i $def {
		if { [ AdapterSettingInUse $acfg_output $attr $i ] == "" } {
			return [ FormatValue $i ]
		}
	}

	# Manufacture a setting from the ATTR:VALUES attribute
	set def [ ExpandRanges [GetValueFromDB $acfg_output $attr VALUES] ]
	foreach i $def {
		if { [ AdapterSettingInUse $acfg_output $attr $i ] == "" } {
			return [ FormatValue $i ]
		}
	}
	return [ FormatValue [ lindex $def 0 ] ]
}

proc IncrGFXMapCount {dlpi_name incr} \
{
	global GFX_ROOT

	set MAPFILE $GFX_ROOT/llimdi

	set ifd [ open $MAPFILE r ]
	set ofd [ open ${MAPFILE}1 w ]

	set x -1

	while { [ gets $ifd line ] != -1 } {
		set l [ split $line : ]
		if { [ lindex $l 0 ] == $dlpi_name } {
			set x [ expr { [ lindex $l 3 ] + $incr } ]
			puts $ofd [ format "%s:%s:%s:%d" $dlpi_name [ lindex $l 1 ] [ lindex $l 2 ] $x ]
		} else {
			puts $ofd $line
		}
	}
	close $ifd
	close $ofd

	exec mv ${MAPFILE}1 $MAPFILE
	chmod 0644 $MAPFILE
	chgrp bin $MAPFILE
	chown bin $MAPFILE
	return $x
}


#
# Function: ParamPrompts
#
# Purpose:
#   Return the prompt associated with the given parameter.
#   Returns "" if none defined.
#
proc ParamPrompts {media} \
{
    set p ""
    case $media {
        BASE_IO         { set p "I/O Address" }
        DMA	        { set p "DMA Channel" }
        IRQ	        { set p "Interrupt Vector" }
        MEDIA	        { set p "Media/Cable Type" }
        RAM	        { set p "Shared RAM" }
        ROM	        { set p "Boot ROM" }
        SLOT	        { set p "Slot #" }
        TRANSCEIVER	{ set p "Transceiver Type" }
	MAC_ADDR	{ set p "MAC Address" }
    }
    return $p
}

#
# Function: ParamList
#
# Purpose:
#   Return the list of defined parameters, eg. for use with ParamPrompts.
#  
proc ParamList {} \
{
    return {BASE_IO DMA	IRQ MEDIA RAM ROM SLOT TRANSCEIVER MAC_ADDR}
}

proc MakeDescription {acfg_output} \
{
#puts stderr MD($acfg_output)
	set aof_path [ GetAOFPath $acfg_output ]
	set key_attr_list [ GetValueFromDB $acfg_output ADAPTER KEY ]
	if { $key_attr_list == "" } {
		set key_attr_list [ GetValueFromDB $aof_path ADAPTER KEY ]
	}
	set desc [ lindex [ GetValueFromDB $acfg_output ADAPTER DESCRIPTION ] 0]
	set first_attr 1
	foreach key_attr $key_attr_list {
		set prompt [ GetValueFromDB $acfg_output $key_attr PROMPT no_errors ]
		if { $prompt == "" } {
			set prompt [ ParamPrompts $key_attr ]
		} else {
			set prompt [ lindex $prompt 0 ]
		}
		set key_val [ GetValueFromDB $acfg_output $key_attr SELECT no_errors ]
		if { $key_val != "" } {
			if { $first_attr } {
				set joiner " - "
				set first_attr 0
			} else {
				set joiner ","
			}
			set desc "$desc$joiner$prompt $key_val"
		}
	}
	return $desc
}

# Add two hex numbers and print the results
proc hex_add {x1 x2} \
{
	scan $x1 "%x" n1
	scan $x2 "%x" n2

	set result [ expr $n1+$n2 ]

	return [ format "%x" $result ]
}

#
# Parse the system file (/etc/conf/sdevice.d/*) for a driver and change:
# the information for a particular unit
#
# Usage:	set_system_info path_of_sdevice_file unit_number config
#				hwdep irq-type irq bio eio bram eram
#
proc set_system_info {SystemFile LineNo \
	config hwdep irqtype irq bio eio bram eram} \
{
	set irqlevel 5
	if { $irq == 2 } {
		set irq 9
	}

	set ifd [ open $SystemFile ]
	set ofd [ open /tmp/xx$$ w ]
	set line_no 1
	while { [ gets $ifd line ] != -1 } {
		if { [ string index $line 0 ] == "*" } {
			puts $ofd $line
			continue
		}
		if { $line_no == $LineNo } {
			set drv [ lindex [ split $line ] 0 ]
			set line "$drv\t$config\t$hwdep\t$irqlevel\t$irqtype\t$irq\t$bio\t$eio\t$bram\t$eram"
		}
		puts $ofd $line
		incr line_no
	}
	close $ofd
	close $ifd
	exec mv -f /tmp/xx$$ $SystemFile
}

proc system_all_Ns {SystemFile} \
{
	set ifd [ open $SystemFile ]
	while { [ gets $ifd line ] != -1 } {
		if { [ string index $line 0 ] == "*" } {
			continue
		}
		if { [ lindex [ split $line ] 1 ] == "Y" } {
			close $ifd
			return 1
		}
	}
	close $ifd
	return 0
}

#
# Parse the master file (/etc/conf/cf.d/mdevice) for a driver and change
# the dma channel for a particular driver
#
# Usage:	set_master_info path_of_master_file driver dma_channel
#
proc set_master_info {MasterFile Driver DMAChannel} \
{
	set ifd [ open $MasterFile ]
	set ofd [ open /tmp/xx$$ w ]
	while { [ gets $ifd line ] != -1 } {
		if { [ string index $line 0 ] != "*" } {
			set s [ split $line ]
			set drv [ lindex $s 0 ]
			if { $drv == $Driver } {
				set line "$drv\t[lindex $s 1]\t[lindex $s 2]\t[lindex $s 3]\t[lindex $s 4]\t[lindex $s 5]\t[lindex $s 6]\t[lindex $s 7]\t[lindex $s 8]\t$DMAChannel"
			}
		}
		puts $ofd $line
	}
	close $ofd
	close $ifd
	exec mv -f /tmp/xx$$ $MasterFile
}

#
# Parse a value from a stanza file of the form:
#		base(size)
#	E.g.	d0000(16)
# into its constituent parts:
#		base_address (hex)
#		end_address  (hex)
#		size         (decimal)
#
proc parse_region {region} \
{
	scan $region "%x(%d)" base_address size
	keylset result BASE_ADDRESS $base_address
	set hsize [ format "%x" [ expr $size*1024-1 ] ]
	keylset result SIZE_IN_BYTES_HEX $hsize
	keylset result END_ADDRESS [ hex_add $base_address $hsize ]
	return $result
}
#
# Creates/Removes net entries as needed to ensure that there is one,
# and only one net? netconfig chain component which has not been confgured
# yet.
#
proc AddOrDelNETs {{this_net ""}} \
{
	foreach i [ exec ncfginstall -l ] {
		if { [ regexp "net\[0-9\]+" $i match ] == 1 } {
			scan $match "net%d" m
			set nets($m) 0
		}
	}

	foreach i [ exec netconfig -s ] {
		if { [ regexp "#net\[0-9\]+" $i match ] } {
			scan $match "#net%d" m
			incr nets($m)
		}
	}
	if { $this_net != "" } {
		incr nets($this_net)
	}

	set not_in_use ""
	foreach i [ array names nets ] {
		if { $nets($i) == 0 } {
			if { $not_in_use != "" } {
				exec ncfginstall -d net$i
			} else {
				set not_in_use $i
			}
		}
	}
	if { $not_in_use == "" } {
		set i 0
		while { [ info exists nets($i) ] } {
			incr i
		}

		global SSO_ROOT
		cd $SSO_ROOT/ncfg/net0
		exec ncfginstall -a net$i
	}
}

# Updates the netconfig following lines in the netconfig info file:
#	DESCRIPTION
#	DLPI_VERSION
#	NETWORK_MEDIA
#	NETWORK_FRAME_FMTS
#	SEND_SPEED
#	RECEIVE_SPEED
proc UpdateNetconfigInfo {net_board acfg_output} \
{
	global TMP_DIR

	set cwd [pwd]
	exec rm -rf $TMP_DIR/link
	exec mkdir $TMP_DIR/link

	set aof_path [ GetAOFPath $acfg_output ]
	set media [ GetValueFromDB $acfg_output ADAPTER MEDIA_TYPE ]
	case $media {
	ethernet {
		set ffmts "ethernet-II xns llc1 snap"
	}
	default {
		set ffmts "llc1 snap"
	}
	}
	set desc [ MakeDescription $acfg_output ]

	set rate [ GetValueFromDB $acfg_output ADAPTER SPEED no_errors]
	case $rate {
	very_slow {
		set tx_rate 10
		set rx_rate 10
	}
	slow {
		set tx_rate 20
		set rx_rate 20
	}
	fast {
		set tx_rate 40
		set rx_rate 40
	}
	default {
		set tx_rate 30
		set rx_rate 30
	}
	}

	cd $TMP_DIR/link
	exec ncfginstall -e net$net_board

	set ifd [ open Info ]
	set ofd [ open Info+ w ]
	while { [ gets $ifd line ] != -1 } {
		if { [string range $line 0 11] == "DESCRIPTION=" } {
			puts $ofd "DESCRIPTION=\"$desc\""
		} else { if { [string range $line 0 12] == "DLPI_VERSION=" } {
			puts $ofd "DLPI_VERSION=\"2.0\""
		} else { if { [string range $line 0 13] == "NETWORK_MEDIA=" } {
			puts $ofd "NETWORK_MEDIA=\"$media\""
		} else { if { [string range $line 0 18] == "NETWORK_FRAME_FMTS=" } {
			puts $ofd "NETWORK_FRAME_FMTS=\"$ffmts\""
		} else { if { [string range $line 0 10] == "SEND_SPEED=" } {
			puts $ofd "SEND_SPEED=\"$tx_rate\""
		} else { if { [string range $line 0 13] == "RECEIVE_SPEED=" } {
			puts $ofd "RECEIVE_SPEED=\"$rx_rate\""
		} else {
			puts $ofd $line
		} } } } } }
	}
	close $ofd
	close $ifd
	exec mv Info+ Info

	exec ncfginstall -u net$net_board
	cd $cwd
	exec rm -rf $TMP_DIR/link
}

# Check that SSO_ROOT is set correctly
if { ! [ info exists SSO_ROOT ] } {
	error "libSCO.tcl: SSO_ROOT must be set, and isn't"
}
