# Syntax:
# 	mkmdmdbase InfDbase_Dir Support_Dir Dest_DBaseDir Dest_DialerDir Vendor
# 
# Make modem detection files ... based on modem decription files
#

proc ReadConfig { cfglist cfgfile } {
    upvar $cfglist cfg

    for_file line $cfgfile {

	if { [cindex $line 0] == "\#" || \
		 [cindex $line 0] == "\n" || \
		 [cindex $line 0] == ""} {
	    continue
	}

	set fieldname [lindex $line 0]
	set value [lrange $line 1 99]
	keylset cfg $fieldname $value
    }
}

proc GenDialer {cfgList vendor dir filename} {
    upvar $cfgList cfg

    set ofd [open $dir/$filename w]

    set desc [keylget cfg DESC]
    puts $ofd "\#\n\# Vendor: $vendor\n\# Modem : $desc\n\#"

    foreach k [keylkeys cfg] {

	case $k in {
	    DESC {
	    }
	    DETECT {
	    }
	    TYPE {
	    }
	    AUTODETECT {
	    }
	    DIALER {
	    }
	    default {
		puts $ofd "$k=[keylget cfg $k]"
	    }
	}
    }
    close $ofd
}

proc GenDbaseEntry {cfgList} {
    upvar $cfgList cfg

    set dialer [keylget cfg DIALER]
    set desc [keylget cfg DESC]
    set type [keylget cfg TYPE]
    set detect [keylget cfg DETECT]

    return [list $dialer $desc $type $detect]
}

#
#
set InfDbase [lindex $argv 0]
set SupportDir [lindex $argv 1]
set DestDbase [lindex $argv 2]
set DialerDir [lindex $argv 3]
set Vendor [lindex $argv 4]

set db_fd [open $DestDbase/$Vendor w]

foreach dialer [readdir $InfDbase/$Vendor] {

    if { $dialer == "vendor" } {
	continue
    }

    set mdmcfg ""

    echo "Dialer : $dialer"

    # Get the config for the dialer


    ReadConfig mdmcfg $InfDbase/$Vendor/$dialer 

    # Add anything from the support database

    if { [file exists $SupportDir/$dialer] } {
	ReadConfig mdmcfg $SupportDir/$dialer
    }

    # Generate the dialer

    GenDialer mdmcfg $Vendor $DialerDir $dialer 

    # Output the modem database entry

    puts $db_fd [GenDbaseEntry mdmcfg]

}

close $db_fd

exit 0

