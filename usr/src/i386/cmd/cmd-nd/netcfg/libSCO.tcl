#ident "@(#)libSCO.tcl	27.1"
#ident "$Header$"
#
#  Based on OpenServer libSCO.tcl version 12.1
#
#	Copyright (C) 1993-1996 The Santa Cruz Operation, Inc.  
#	    All Rights Reserved.
#
#       The information in this file is provided for the exclusive use of
#       the licensees of The Santa Cruz Operation, Inc.  Such users have the
#       right to use, modify, and incorporate this code into other products
#       for purposes authorized by the license agreement provided they include
#       this notice and the associated copyright notice with any such product.
#       The information in this file is provided "AS IS" without warranty.
#

set NCFG_DIR /usr/lib/netcfg
source $NCFG_DIR/lib/libSCO.msg.tcl
source $NCFG_DIR/bin/ncfgBE.msg.tcl

proc PresentString {s {len 75} {repstr \n}} \
{
	set desc ""
	while { [ clength $s ] > $len && [string match "* *" [ crange $s 0 $len ]] } {
		set i [ string last " " [ crange $s 0 $len ] ]
		set desc "$desc[ crange $s 0 $i ]$repstr"
		set s "[ crange $s [ incr i ] end ]"
	}
	if { ! [ string compare $desc "" ] } {
		return $s
	} else {
		return "$desc$s"
	}
}

# returns 1 if service is registered with the port monitor, else 0
proc InPortMonitor {netx} \
{
	set results [CallNonStdCmd /usr/sbin/pmadm "-L -p isdnmon -s $netx" \
		SCO_NETCONFIG_BE_MSG_SERIAL_OSA errStack]
	
	if {[string length $errStack] == 0} {
		return 1
	} else {
		return 0
	}
}

# returns 1 if there are no services registered with the port monitor
proc LastService { portmonitor } \
{
	set results [CallNonStdCmd /usr/sbin/pmadm "-L -p $portmonitor" \
		SCO_NETCONFIG_BE_MSG_SERIAL_OSA errStack]

	if {[string length $errStack] == 0} {
		return 0
	} else {
		return 1
	}
}

# deconfigures port monitor with sac
proc RemovePortMonitor { portmonitor } \
{
	set results [CallNonStdCmd /usr/sbin/sacadm "-r -p $portmonitor" \
		SCO_NETCONFIG_BE_MSG_SERIAL_OSA errStack]
	return $results
}

# removes a service from the port monitor, stops port monitor if it
# is the last service registered
proc RemoveIncoming {netx} {
	set results [CallNonStdCmd /usr/sbin/pmadm "-r -p isdnmon -s $netx" \
		SCO_NETCONFIG_BE_MSG_SERIAL_OSA errStack]
	if { [LastService isdnmon] } {
		RemovePortMonitor isdnmon
	}
}

# InDevices returns 1 if netx present in uucp Devices file, 0 if not present
proc InDevices {netx} \
{
	set DevicesConfig {}
	set objcall [list ObjectGet -filter {{type eq ISDN_SYNC} \
		or {type eq ISDN_ASYNC}} {sco UUCPdevices} NULL {}]
	set bmipResponse [SaMakeObjectCall $objcall]
	set firstBmip [lindex $bmipResponse 0]
	set errorStack [BmipResponseErrorStack firstBmip]
	if { ! [lempty $errorStack] } {
		# Call to the OSA failed - netx MAY be in Devices
	   	return 0
	} else {
		foreach item $bmipResponse {
			set object [BmipResponseObjectInstance item]
			set attrs  [BmipResponseAttrValList item]
			keylset DevicesConfig $object $attrs 
		}
	}
	if { [keylget DevicesConfig ISDN_SYNC_/dev/$netx {} ] || \
		[ keylget DevicesConfig ISDN_ASYNC_/dev/$netx {} ] } {
		return 1
	} else {
		return 0
	}
}

# uses uucpOSA to remove netx devices from the uucp Devices file
# currently, bug in uucpOSA causes this to fail
# so it hasn't been tested
proc RemoveOutgoing {netx} \
{
	keylset attrs port /dev/$netx
	set instance ISDN_SYNC_/dev/$netx
	set objcall [list ObjectDelete \
		{sco UUCPdevices} $instance $attrs]
	set bmipResponse [SaMakeObjectCall $objcall]
	set firstBmip [lindex $bmipResponse 0]
	set errStack [BmipResponseErrorStack firstBmip]
	if { ![lempty $errStack] } {
#puts stderr "RemoveOutgoing: $errStack"
	}
	keylset attrs port /dev/$netx
	set instance ISDN_ASYNC_/dev/$netx
	set objcall [list ObjectDelete \
		{sco UUCPdevices} $instance $attrs]
	set bmipResponse [SaMakeObjectCall $objcall]
	set firstBmip [lindex $bmipResponse 0]
	set errStack [BmipResponseErrorStack firstBmip]
	if { ![lempty $errStack] } {
#puts stderr "RemoveOutgoing: $errStack
	}
}

# remove ISDN device from isdnmon and Devices

proc RemoveIncomingOutgoing { netx } \
{
 	if { [InPortMonitor $netx] } {
		RemoveIncoming $netx
	}

	if { [InDevices $netx] } {
		RemoveOutgoing $netx
	}
}


# tmpfile { basename { perms 0666 }} 
# creates a new temporary file with the basename and a unique
# extention.  Returns the fileid and filename so its best to call
# the proc as follows:
# lassign [ tmpfile /tmp/test 0600 ] tmpfd tmpname
proc tmpfile { basename { perms 0666 }} \
{
	set ext [ pid ]
	while { [ file exists $basename.$ext ] } {
		incr ext
	}
	set fileid [ open $basename.$ext { WRONLY CREAT EXCL } $perms ] 

	return  [ list $fileid $basename.$ext ]

}


proc TfadminUnlink { file } \
{
# puts stderr "TfadminUnlink $file"
	if {[id userid] == "0"} {
		exec /bin/rm -f $file
	} else {
		set RETCODE [ catch {  exec /sbin/tfadmin -t NETCFG: /bin/rm ] } ]

		if { $RETCODE == "0" } {
			exec /sbin/tfadmin /bin/rm -f $file
		} else {
			exec /bin/rm -f $file
		}
	}
}

proc TfadminMv { file dest } \
{
# puts stderr "TfadminMv $file $dest"

	if {[id userid] == "0"} {
		exec /bin/mv -f $file $dest
	} else {
		set RETCODE [ catch {  exec /sbin/tfadmin -t NETCFG: /bin/mv ] } ]

		if { $RETCODE == "0" } {
			exec /sbin/tfadmin /bin/mv -f $file $dest
		} else {
			exec /bin/mv -f $file $dest
		}
	}
}
