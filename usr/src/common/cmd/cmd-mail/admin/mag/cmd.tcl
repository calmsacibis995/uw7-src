#ident "@(#)cmd.tcl	11.2"
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
# command line interface goes here
#
# it is modelled after a more or less direct translation of the ma_* calls.
# it is also mostly independent from the GUI portion of the program
# as only a few utility routines are shared.
#
# the command line interface is not internationalized.
#

proc \
mag_cmd_line_main {} \
{
	global argv argv0 mag_host_name DIRTY

	if {"$argv" == ""} {
		return
	}
        # check if remote host is there
        if {"[mag_host_check $mag_host_name]" != "ok"} {
                echo "mailadmin: Unable to connect to host $mag_host_name"
                exit 0
        }
	# check authorizations
	if {"[mag_authorized $mag_host_name]" == "fail"} {
		echo "mailadmin: You are not authorized to run the Mail Manager on [mag_short_name_default]"
		exit 0
	}

	# get our remote files from the appropriate host
	mag_cmd_file_get

	set DIRTY 0

	# now process rest of command line
	while {"$argv" != ""} {
		# do one command
		mag_cmd_one
	}

	# put our remote files
	mag_cmd_file_put
	mag_cmd_quit 0
}

# do one command from the argv list
proc \
mag_cmd_one {} \
{
	global CMD argv DIRTY

	set cmd [mag_cmd_popargv]
	set CMD $cmd

	switch -- "$cmd" {
	"sendmail" {
		set action [mag_cmd_popargv]
		if {"$action" != "restart"} {
			echo "mailadmin: Unknown sendmail action: $action"
			mag_cmd_quit 1
		}
		mag_restart_sendmail no
	}
	"uucp" {
		set action [mag_cmd_popargv]
		if {"$action" != "rebuild"} {
			echo "mailadmin: Unknown uucp action: $action"
			mag_cmd_quit 1
		}
		mag_rebuild_uucp_map no
	}
	"add" {
		set DIRTY 1
		set type [mag_cmd_popargv]
		set name [mag_cmd_popargv]
		switch "$type" {
		"map" {
			set list [ma_aliases_get]
			if {[lsearch $list $name] >= 0} {
				echo "mailadmin: Duplicate map name for add: $name"
				mag_cmd_quit 1
			}
			lappend list $name
			ma_aliases_set $list
		}
		"altname" {
			set list [ma_alternate_names_get]
			if {[lsearch $list $name] >= 0} {
				echo "mailadmin: Duplicate altname for add: $name"
				mag_cmd_quit 1
			}
			lappend list $name
			ma_alternate_names_set $list
		}
		"channel" {
			set list [mai_ch_names_get]
			set program [mag_cmd_popargv]
			set table [mag_cmd_popargv]
			if {[lsearch $list $name] >= 0} {
				echo "mailadmin: Duplicate channel name for add: $name"
				mag_cmd_quit 1
			}
			set ret [ma_ch_create $name]
			switch "$ret" {
			"conflict" {
				echo "mailadmin: Name conflict or reserved name: $name"
				mag_cmd_quit 1
			}
			"badname" {
				echo "mailadmin: Invalid channel name: $name"
				mag_cmd_quit 1
			}
			}
			ma_ch_equate_set $name P $program
			mag_cmd_table_type_check $table
			ma_ch_table_type_set $name $table
			set ret [mai_channel_order_set $name]
			if {"$ret" != "ok"} {
				echo "mailadmin: Only one baduser channel may exist."
				mag_cmd_quit 1
			}
			# now set all channel properties to default values.
			set list [mag_cmd_channel_prop_list]
			foreach item $list {
				set propname [list $name $item]
				mag_cmd_default $propname
			}
		}
		default {
			echo "mailadmin: Unknown add type: $type"
			mag_cmd_quit 1
		}
		}
	}
	"del" {
		set DIRTY 1
		set type [mag_cmd_popargv]
		set name [mag_cmd_popargv]
		switch "$type" {
		"map" {
			set list [ma_aliases_get]
			set index [lsearch $list $name]
			if {$index == -1} {
				echo "mailadmin: Map name not found: $name"
				mag_cmd_quit 1
			}
			set list [lreplace $list $index $index]
			ma_aliases_set $list
		}
		"altname" {
			set list [ma_alternate_names_get]
			set index [lsearch $list $name]
			if {$index == -1} {
				echo "mailadmin: Altname not found: $name"
				mag_cmd_quit 1
			}
			set list [lreplace $list $index $index]
			ma_alternate_names_set $list
		}
		"channel" {
			set list [mai_ch_names_get]
			set index [lsearch $list $name]
			if {$index == -1} {
				echo "mailadmin: Channel name not found: $name"
				mag_cmd_quit 1
			}
			ma_ch_delete $name
		}
		default {
			echo "mailadmin: Invalid del type: $type"
			mag_cmd_quit 1
		}
		}
	}
	"order" {
		set DIRTY 1
		set type [mag_cmd_popargv]
		set neworder ""
		set item [mag_cmd_popargv]
		while {"$item" != "" && "$item" != "--"} {
			lappend neworder $item
			if {"$argv" != ""} {
				set item [mag_cmd_popargv]
			} else {
				set item ""
			}
		}
		switch "$type" {
		"maps" {
			set oldorder [ma_aliases_get]
			mag_cmd_vfy_lists $neworder $oldorder
			ma_aliases_set $neworder
		}
		"altnames" {
			set oldorder [ma_alternate_names_get]
			mag_cmd_vfy_lists $neworder $oldorder
			ma_alternate_names_set $neworder
		}
		"channels" {
			set oldorder [mai_ch_names_get]
			mag_cmd_vfy_lists $neworder $oldorder
			# now verify that new order doesn't have a
			# baduser channel in the wrong place.
			set length [llength $neworder]
			set stop [expr $length - 1]
			loop index 0 $stop {
				set chname [lindex $neworder $index]
				set table [ma_ch_table_type_get $chname]
				if {"$table" == "baduser"} {
					echo "mailadmin: New order has a baduser channel that is not the last channel."
					mag_cmd_quit 1
				}
			}
			# now set the new order
			loop index 0 $length {
				set chname [lindex $neworder $index]
				ma_ch_sequence_set $chname $index
			}
		}
		default {
			echo "mailadmin: Invalid order type: $type"
			mag_cmd_quit 1
		}
		}
	}
	"list" {
		set type [mag_cmd_popargv]
		switch "$type" {
		"maps" {
			set list [ma_aliases_get]
		}
		"altnames" {
			set list [ma_alternate_names_get]
		}
		"channels" {
			set list [mai_ch_names_get]
		}
		default {
			echo "mailadmin: Invalid list type: $type"
			mag_cmd_quit 1
		}
		}
		foreach item $list {
			echo $item
		}
	}
	"get" {
		set propname [mag_cmd_getprop]
		mag_cmd_get $propname
	}
	"set" {
		set propname [mag_cmd_getprop]
		set value [mag_cmd_popargv]
		mag_cmd_set $propname $value
	}
	"def" {
		set propname [mag_cmd_getprop]
		mag_cmd_default $propname
	}
	default {
		echo "mailadmin: Unknown command: $cmd"
		mag_cmd_usage
		mag_cmd_quit 1
	}
	}
}

# get value for a given property
proc \
mag_cmd_get { propname } \
{
	global mag_host_name MQUEUE

	set chname ""
	if {[llength $propname] == 2} {
		set chname [lindex $propname 0]
		set propname [lindex $propname 1]
	}	
	# regular properties
	if {"$chname" == ""} {
		switch $propname {
		host {
			set value [ma_machine_name_get]
			if {"$value" == ""} {
				set value $mag_host_name
			}
			echo $value
		}
		from {
			set value [ma_from_domain_get]
			if {"$value" == ""} {
				set value $mag_host_name
			}
			echo $value
		}
		domain {
			set ret [ma_domain_table_enabled_get]
			if {"$ret" == "1"} {
				echo TRUE
			} else {
				echo FALSE
			}
		}
		fdir {
			echo [ma_ms1_get MS1_INBOX_DIR]
		}
		fname {
			echo [ma_ms1_get MS1_INBOX_NAME]
		}
		fformat {
			set ret [ma_ms1_get MS1_FOLDER_FORMAT]
			set ret [string tolower $ret]
			switch "$ret" {
			"mmdf" {
				echo MMDF
			}
			"sendmail" {
				echo Sendmail
			}
			}
		}
		fsync {
			echo [string toupper [ma_ms1_get MS1_FSYNC]]
		}
		fccheck {
			echo [string toupper [ma_ms1_get MS1_EXTENDED_CHECKS]]
		}
		fincore {
			echo [string toupper [ma_ms1_get MS1_FOLDERS_INCORE]]
		}
		fthreshold {
			echo [ma_ms1_get MS1_EXPUNGE_THRESHOLD]
		}
		ftimeout {
			echo [ma_ms1_get MS1_LOCK_TIMEOUT]
		}
		ffilelock {
			echo [string toupper [ma_ms1_get MS1_FILE_LOCKING]]
		}
		fumask {
			echo [ma_ms1_get MS1_UMASK]
		}
		}
	} else {
		# channel properties
		switch "$propname" {
		name {
			echo $chname
		}
		program {
			echo [ma_ch_equate_get $chname P]
		}
		args {
			echo [ma_ch_equate_get $chname A]
		}
		dir {
			set ret [ma_ch_equate_get $chname D]
			if {"$ret" == ""} {
				set ret $MQUEUE
			}
			echo $ret
		}
		table {
			echo [ma_ch_table_type_get $chname]
		}
		file {
			echo [ma_ch_table_file_get $chname]
		}
		host {
			echo [ma_ch_host_get $chname]
		}
		flags {
			echo [ma_ch_equate_get $chname F]
		}
		eol {
			set value [ma_ch_equate_get $chname E]
			if {"$value" == ""} {
				set value "\\n"
			}
			echo $value
		}
		maxmsg {
			set value [ma_ch_equate_get $chname M]
			if {"$value" == ""} {
				set value 0
			}
			echo $value
		}
		maxline {
			set value [ma_ch_equate_get $chname L]
			if {"$value" == ""} {
				set value 0
			}
			echo $value
		}
		nice {
			set value [ma_ch_equate_get $chname N]
			if {"$value" == ""} {
				set value 0
			}
			echo $value
		}
		user {
			echo [ma_ch_equate_get $chname U]
		}
		rruleset {
			echo [ma_ch_equate_get $chname R]
		}
		sruleset {
			echo [ma_ch_equate_get $chname S]
		}
		}
	}
}

# set value for a given property.
# only minimal value checking is performed, user must be careful.
proc \
mag_cmd_set { propname value } \
{
	global MQUEUE DIRTY mag_host_name

	set DIRTY 1
	set chname ""
	if {[llength $propname] == 2} {
		set chname [lindex $propname 0]
		set propname [lindex $propname 1]
	}	
	# regular properties
	if {"$chname" == ""} {
		switch $propname {
		host {
			if {"$value" == "$mag_host_name"} {
				set value ""
			}
			ma_machine_name_set $value
		}
		from {
			if {"$value" == "$mag_host_name"} {
				set value ""
			}
			ma_from_domain_set $value
		}
		domain {
			set bool [mag_cmd_check_tf $value]
			ma_domain_table_enabled_set $bool
		}
		fdir {
			ma_ms1_set MS1_INBOX_DIR $value
		}
		fname {
			ma_ms1_set MS1_INBOX_NAME $value
		}
		fformat {
			set test [string tolower $value]
			switch "$test" {
			"mmdf" {
				set value MMDF
			}
			"sendmail" {
				set value Sendmail
			}
			default {
				echo "mailadmin: Folder format must be MMDF or Sendmail: $value"
				mag_cmd_quit 1
			}
			}
			ma_ms1_set MS1_FOLDER_FORMAT $value
		}
		fsync {
			mag_cmd_check_tf $value
			ma_ms1_set MS1_FSYNC [string toupper $value]
		}
		fccheck {
			mag_cmd_check_tf $value
			ma_ms1_set MS1_EXTENDED_CHECKS [string toupper $value]
		}
		fincore {
			mag_cmd_check_tf $value
			ma_ms1_set MS1_FOLDERS_INCORE [string toupper $value]
		}
		fthreshold {
			set value [mag_cmd_number $value 0 100]
			ma_ms1_set MS1_EXPUNGE_THRESHOLD $value
		}
		ftimeout {
			set value [mag_cmd_number $value 1 999]
			ma_ms1_set MS1_LOCK_TIMEOUT $value
		}
		ffilelock {
			mag_cmd_check_tf $value
			ma_ms1_set MS1_FILE_LOCKING [string toupper $value]
		}
		fumask {
			mag_cmd_check_umask $value
			ma_ms1_set MS1_UMASK $value
		}
		}
	} else {
		# channel properties
		switch "$propname" {
		name {
			# wants to rename channel
			set newname $value
			set oldname $chname
			set ret [ma_ch_rename $oldname $newname]
			switch "$ret" {
			"conflict" {
				echo "mailadmin: Channel name conflict: $newname"
				mag_cmd_quit 1
			}
			"badname" {
				echo "mailadmin: Channel name invalid: $newname"
				mag_cmd_quit 1
			}
			}
		}
		program {
			ma_ch_equate_set $chname P $value
		}
		args {
			ma_ch_equate_set $chname A $value
		}
		dir {
			if {"$value" == "$MQUEUE"} {
				set value ""
			}
			ma_ch_equate_set $chname D $value
		}
		table {
			mag_cmd_table_type_check $value
			ma_ch_table_type_set $chname $value
		}
		file {
			ma_ch_table_file_set $chname $value
		}
		host {
			ma_ch_host_set $chname $value
		}
		flags {
			ma_ch_equate_set $chname F $value
		}
		eol {
			if {"$value" == "\\n"} {
				set value ""
			}
			ma_ch_equate_set $chname E $value
		}
		maxmsg {
			set value [mag_cmd_number $value 0 2000000000]
			if {$value == 0} {
				set value ""
			}
			ma_ch_equate_set $chname M $value
		}
		maxline {
			set value [mag_cmd_number $value 0 10000]
			if {$value == 0} {
				set value ""
			}
			ma_ch_equate_set $chname L $value
		}
		nice {
			set value [mag_cmd_number $value -20 20]
			if {$value == 0} {
				set value ""
			}
			ma_ch_equate_set $chname N $value
		}
		user {
			ma_ch_equate_set $chname U $value
		}
		rruleset {
			ma_ch_equate_set $chname R $value
		}
		sruleset {
			ma_ch_equate_set $chname S $value
		}
		}
	}
}

# set default value for a given property
proc \
mag_cmd_default { propname } \
{
	global MQUEUE MAILSPOOL MAILTABLE DIRTY mag_host_name

	set DIRTY 1
	set chname ""
	if {[llength $propname] == 2} {
		set chname [lindex $propname 0]
		set propname [lindex $propname 1]
	}	
	# regular properties
	if {"$chname" == ""} {
		switch $propname {
		host {
			ma_machine_name_set ""
		}
		from {
			ma_from_domain_set ""
		}
		domain {
			ma_domain_table_enabled_set 0
		}
		fdir {
			ma_ms1_set MS1_INBOX_DIR $MAILSPOOL
		}
		fname {
			ma_ms1_set MS1_INBOX_NAME ""
		}
		fformat {
			ma_ms1_set MS1_FOLDER_FORMAT Sendmail
		}
		fsync {
			ma_ms1_set MS1_FSYNC FALSE
		}
		fccheck {
			ma_ms1_set MS1_EXTENDED_CHECKS FALSE
		}
		fincore {
			ma_ms1_set MS1_FOLDERS_INCORE FALSE
		}
		fthreshold {
			ma_ms1_set MS1_EXPUNGE_THRESHOLD 50
		}
		ftimeout {
			ma_ms1_set MS1_LOCK_TIMEOUT 10
		}
		ffilelock {
			ma_ms1_set MS1_FILE_LOCKING FALSE
		}
		fumask {
			ma_ms1_set MS1_UMASK 077
		}
		}
	} else {
		# channel properties
		switch "$propname" {
		name {
			# does nothing
		}
		program {
			# does nothing
		}
		table {
			# does nothing
		}
		args {
			# default based on channel program
			set chprogram [ma_ch_equate_get $chname P]
			set base [exec basename $chprogram]
			set value "$base \$u"
			switch $base {
			"\[IPC\]" {
				set value "IPC \$h"
			}
			"uux" {
				set value "uux - -r -a\$f -gmedium \$h!rmail (\$u)"
			}
			"slocal" {
				set value "slocal \$u"
			}
			}
			ma_ch_equate_set $chname A $value
		}
		dir {
			ma_ch_equate_set $chname D ""
		}
		file {
			# based on table type
			set table [ma_ch_table_type_get $chname]
			if {"$table" == "file"} {
				set value "$MAILTABLE/$chname"
			} else {
				set value ""
			}
			ma_ch_table_file_set $chname $value
		}
		host {
			ma_ch_host_set $chname ""
		}
		flags {
			# default based on channel program
			set chprogram [ma_ch_equate_get $chname P]
			# custom same as SMTP
			set value "mlsDFMPeu8"
			switch [exec basename $chprogram] {
			"\[IPC\]" {
				set value "mlsDFMPeu8"
			}
			"uux" {
				set value "mDFMhuU8"
			}
			"slocal" {
				set value "lsDFMPhoAw5:|/@8"
			}
			}
			ma_ch_equate_set $chname F $value
		}
		eol {
			set chprogram [ma_ch_equate_get $chname P]
			if {"$chprogram" == "\[IPC\]"} {
				set value "\\r\\n"
			} else {
				set value ""
			}
			ma_ch_equate_set $chname E $value
		}
		maxmsg {
			set chprogram [ma_ch_equate_get $chname P]
			if {"[exec basename $chprogram]" == "uux"} {
				set value 100000
			} else {
				#set value ""
				set value 20000000
			}
			ma_ch_equate_set $chname M $value
		}
		maxline {
			set chprogram [ma_ch_equate_get $chname P]
			if {"$chprogram" == "\[IPC\]"} {
				set value 990
			} else {
				set value ""
			}
			ma_ch_equate_set $chname L $value
		}
		nice {
			ma_ch_equate_set $chname N ""
		}
		user {
			ma_ch_equate_set $chname U ""
		}
		rruleset {
			# default based on channel program
			set chprogram [ma_ch_equate_get $chname P]
			# custom same as SMTP
			set value "ap822_re/ap822_rh"
			switch [exec basename $chprogram] {
			"\[IPC\]" {
				set value "ap822_re/ap822_rh"
			}
			"uux" {
				set value "ap976_re/ap976_rh"
			}
			"slocal" {
				set value "aplocal_re/aplocal_rh"
			}
			}
			ma_ch_equate_set $chname R $value
		}
		sruleset {
			# default based on channel program
			set chprogram [ma_ch_equate_get $chname P]
			# custom same as SMTP
			set value "ap822_se/ap822_sh"
			switch [exec basename $chprogram] {
			"\[IPC\]" {
				set value "ap822_se/ap822_sh"
			}
			"uux" {
				set value "ap976_se/ap976_sh"
			}
			"slocal" {
				set value "aplocal_se/aplocal_sh"
			}
			}
			ma_ch_equate_set $chname S $value
		}
		}
	}
}

proc \
mag_cmd_usage {} \
{
	global MQUEUE
# no indents to make this easier
echo "Usage:"
echo "mailadmin"
echo "    invoke GUI"
echo "mailadmin \[-h host\] command \[arguments\]"
echo "    Command line mode, following are the commands and their arguments."
echo "    Multiple command may be strung together on the command line."
echo "    If any of them fail no changes to the config files are made."
echo "    Commands with a variable number of arguments may be terminated with --."
echo
echo "sendmail restart - stop and restart the sendmail daemon"
echo "    Sendmail should be restarted after all changes are completed."
echo "    The GUI does this automatically."
echo
echo "uucp rebuild - rebuild /etc/mail/table/uucp.db from /usr/lib/uucp/Systems."
echo "    This only needs to be done if a UUCP channel exists and the Systems file"
echo "    has been changed.  The GUI does this automatically."
echo
echo "add map name - add an nis alias map or alias file entry.
echo "    formats are nis:mapname or a full pathname for an alias file."
echo "add altname name - add an alternate name for this host."
echo "add channel name program type - add a new channel entry."
echo "    table type is one of: DNS UUCP local remote baduser file."
echo
echo "del map name - delete an alias map or alias file by name."
echo "del altname name - delete an alternate name entry by name."
echo "del channel name - delete a channel entry by name."
echo
echo "order maps list - give new alias map order."
echo "order altnames list - give new alternate names order."
echo "order channels list - give new channel order."
echo "    channels using the baduser table type must go last,"
echo "    and there can be at most one such channel."
echo
echo "list maps - output alias map names in order."
echo "list altnames - output alternate names in order."
echo "list channels - output channel names in order."
echo
echo "The following commands deal with individual properties."
echo
echo "def propname - set a given property to it's default value."
echo "get propname - display current value of the given property."
echo "set propname value - set a given property to the given value."
echo
echo "Propnames can be a single token or they can be a channel name,"
echo "followed by a colon and then the property name."
echo
echo "Valid property names and their values are as follows:"
echo
echo "General Mail Properties"
echo "host - Host Name, default is to use the system name."
echo "from - From Domain Name, default (systemid) is used."
echo "domain - TRUE/FALSE, Domain Table Enabled (/etc/mail/table/domain)."
echo
echo "Message Store (Folder) properties"
echo "fdir - inbox directory, zero length string means use user's home directories."
echo "fname - inbox name, zero length string means use user's login name."
echo "fformat - MMDF/Sendmail, default folder format."
echo "fsync - TRUE/FALSE, enable O_FSYNC on folders."
echo "fccheck - TRUE/FALSE, enable extended consistency checks."
echo "fincore - TRUE/FALSE, enable incore caching of entire mail folders."
echo "fthreshold - 0-100, percent of file that must be maintained as valid."
echo "    set 100 for maximum compatibility, 50 for higher performance."
echo "ftimeout - 1-999, file lock timeout for mail folders."
echo "ffilelock - TRUE/FALSE, enable file based locking on mail folders."
echo "    folder.lock files will be used.  Kernel locking is always enabled."
echo "    This option adds file based locking."
echo "fumask - 0-0777, set mask for folder and directory creation for mail system."
echo
echo "Channel properties (must be preceeded with channel name and a colon)."
echo "name - channel name, no white space in name, domain is a reserved name,"
echo "    names must be unique.  baduser, badhost, multihome, and local are treated"
echo "    specially, they are translated into local names in the GUI based on LOCALE."
echo "program - channel program (P=), either a path name or \[IPC\] for SMTP."
echo "args - channel program arguments (A=), \$h and \$u are special to sendmail."
echo "    See sendmail documentation on mailer arguments."
echo "dir - channel program execution directory, default is $MQUEUE."
echo "table - DNS, UUCP, remote, local, file, baduser, channel table type."
echo "   baduser channel is special, it must be last and there can be only one."
echo "file - channel table file, only used for channel tables of type file."
echo "host - route all mail for this channel through this host."
echo "   set this option to the zero length string to remove this routing."
echo "   This value is ignored for file table channels."
echo "flags - channel program flags (F=), see sendmail doc on mailer flags."
echo "eol - channel program end of line (usually \\n although SMTP wants \\r\\n)."
echo "maxmsg - maximum message size, zero means no maximum."
echo "maxline - maximum line size, zero means no maximum, 990 is expected to be"
echo "    set for all channels that use SMTP, most others use 0."
echo "nice - -20-20 (N=), channel program nice increment, 0 is use default value."
echo "user - user:group (U=), user and group to run channel program as. If set to"
echo "    zero length string, then bin:bin (1:1) is used."
echo "rruleset - envelope\[/header\], envelope and header recipient rulesets."
echo "    We define ap822_re/ap822_rh for SMTP, ap975_re/ap976_rh for UUCP,"
echo "    and aplocal_re/aplocal_rh for local delivery."
echo "    Custom channels will generally want the rfc822 rewriting rules."
echo "    See the sendmail doc if you to add your own rulesets to sendmail.cf."
echo "sruleset - envelope\[/header\], envelope and header sender rulesets."
echo "    We define ap822_se/ap822_sh for SMTP, ap975_se/ap976_sh for UUCP,"
echo "    and aplocal_se/aplocal_sh for local delivery."
}

# verify table type is one of the valid ones
proc \
mag_cmd_table_type_check { table } \
{
	set list [list baduser DNS UUCP remote local file]
	if {[lsearch $list $table] == -1} {
		echo "mailadmin: Unknown channel table type: $table"
		mag_cmd_quit 1
	}
}

# verify lists have the same values in an order independent way
proc \
mag_cmd_vfy_lists { newlist oldlist } \
{
	set slist1 [lsort $newlist]
	set slist2 [lsort $oldlist]
	if {"$slist1" != "$slist2"} {
		echo "mailadmin: Ordering lists don't have the same items:"
		echo "    new list: $newlist"
		echo "    old list: $oldlist"
		mag_cmd_quit 1
	}
}

# pop an arg from argv and verify that it actually exists.
proc \
mag_cmd_popargv {} \
{
	global argv CMD

	if {"$argv" == ""} {
		echo "mailadmin: Not enough arguments for command: $CMD"
		mag_cmd_quit 1
	}
	return [lvarpop argv]
}

# get a property name from argv and verify that it is valid
# channel names are parsed and returned as a two item list,
# first item is channel name, second is property name.
proc \
mag_cmd_getprop {} \
{
	set propname [mag_cmd_popargv]

	# have a channel property
	set colon [string first ":" $propname]
	if {$colon >= 0} {
		set end [expr $colon - 1]
		set chname [csubstr $propname 0 $colon]
		if {"$chname" == ""} {
			echo "mailadmin: Null channel name"
			mag_cmd_quit 1
		}
		set channels [mai_ch_names_get]
		if {[lsearch $channels $chname] == -1} {
			echo "mailadmin: Unknown channel name: $chname"
			mag_cmd_quit 1
		}
		set start [expr $colon + 1]
		set propname [csubstr $propname $start end]
		mag_cmd_channel_prop_vfy $propname
		return [list $chname $propname]
	}
	# regular property
	mag_cmd_regular_prop_vfy $propname
	return $propname
}

# validate a channel property name
proc \
mag_cmd_channel_prop_vfy { propname } \
{
	if {"$propname" == ""} {
		echo "mailadmin: Null channel property name"
		mag_cmd_quit 1
	}
	set list [mag_cmd_channel_prop_list]
	if {[lsearch $list $propname] == -1} {
		echo "mailadmin: Invalid channel property name: $propname"
		mag_cmd_quit 1
	}
}

# get a list of valid channel properties
proc \
mag_cmd_channel_prop_list {} \
{
	set list [list name program args dir table file host flags eol maxmsg maxline nice user rruleset sruleset]
	return $list
}

# validate a regular property name
proc \
mag_cmd_regular_prop_vfy { propname } \
{
	if {"$propname" == ""} {
		echo "mailadmin: Null property name"
		mag_cmd_quit 1
	}
	set list [list host from domain fdir fname fformat fsync fccheck fincore fthreshold ftimeout ffilelock fumask]
	if {[lsearch $list $propname] == -1} {
		echo "mailadmin: Invalid property name: $propname"
		mag_cmd_quit 1
	}
}

# get TRUE/FALSE and validate it, allow case insensitivity
# return 0 or 1 based on TRUE/FALSE value
proc \
mag_cmd_check_tf { value } \
{
	set test [string toupper $value]
	if {"$test" == "TRUE"} {
		return 1
	}
	if {"$test" == "FALSE"} {
		return 0
	}
	echo "mailadmin: Invalid TRUE/FALSE value: $value"
	mag_cmd_quit 1
}

# check if umask is valid
proc \
mag_cmd_check_umask { value } \
{
	if {[ctype digit $value] == 0} {
		echo "mailadmin: Invalid umask value: $value"
		mag_cmd_quit 1
	}
	if {[string first 8 $value] >= 0} {
		echo "mailadmin: Invalid umask, not an octal number: $value"
		mag_cmd_quit 1
	}
	if {[string first 9 $value] >= 0} {
		echo "mailadmin: Invalid umask, not an octal number: $value"
		mag_cmd_quit 1
	}
	if {"[cindex $value 0]" != "0"} {
		echo "mailadmin: First digit of a umask must be 0"
		mag_cmd_quit 1
	}
	if {$value > 0777} {
		echo "mailadmin: Invalid umask value, greater than 0777: $value"
		mag_cmd_quit 1
	}
}

# verify that we have digits
proc \
mag_cmd_number { value min max } \
{
	set test $value
	if {"[csubstr $value 0 1]" == "-"} {
		set test [csubstr $value 1 end]
	}
	if {[ctype digit $test] == 0} {
		echo "mailadmin: Invalid numeric value: $value"
		mag_cmd_quit 1
	}
	set value [mag_trim_zero $value]
	if {$value < $min} {
		echo "mailadmin: Number less than minimum value of $min: $value"
		mag_cmd_quit 1
	}
	if {$value > $max} {
		echo "mailadmin: Number greater than maximum value of $max: $value"
		mag_cmd_quit 1
	}
	return $value
}

# get our remote file, similar to mag_cf_get and mag_ms1_get, but
# no factory defaults or editting recovery capability here.
# just get the files.
proc \
mag_cmd_file_get {} \
{
	global MAILCF MAILCFTMP MAILDEF MAILDEFTMP mag_host_name

	system "rm -fr $MAILCFTMP $MAILDEFTMP"
	set ret [mag_remote_copyin $MAILCF $MAILCFTMP]
	if {"$ret" != "ok"} {
		echo "mailadmin: Unable to access $mag_host_name:$MAILCF"
		mag_cmd_quit 1
	}
	set ret [mag_remote_copyin $MAILDEF $MAILDEFTMP]
	if {"$ret" != "ok"} {
		echo "mailadmin: Unable to access $mag_host_name:$MAILDEF"
		mag_cmd_quit 1
	}

	# now open them
	set ret [ma_cf_open $MAILCFTMP]
	if {"$ret" != "ok"} {
		echo "mailadmin: Unable to parse $mag_host_name:$MAILCF"
		mag_cmd_quit 1
	}	

	set ret [ma_ms1_open $MAILDEFTMP]
	if {"$ret" != "ok"} {
		echo "mailadmin: Unable to parse $mag_host_name:$MAILDEF"
		mag_cmd_quit 1
	}	
}

# put our remote files back
proc \
mag_cmd_file_put {} \
{
	global MAILCF MAILCFTMP MAILDEF MAILDEFTMP mag_host_name DIRTY
@if test
	global TEST
@endif

	if {$DIRTY == 1} {
		set ret [ma_cf_write]
@if test
		if {"$TEST" == "grp5_write1"} {
			set ret fail
		}
@endif
		if {"$ret" != "ok"} {
			echo "mailadmin: Unable to save: $MAILCF"
			mag_cmd_quit 1
		}
		set ret [ma_ms1_write]
@if test
		if {"$TEST" == "grp5_write2"} {
			set ret fail
		}
@endif
		if {"$ret" != "ok"} {
			echo "mailadmin: Unable to save: $MAILDEF"
			mag_cmd_quit 1
		}
		set ret [mag_remote_copyout $MAILCFTMP $MAILCF]
@if test
		if {"$TEST" == "grp5_copy1"} {
			set ret fail
		}
@endif
		if {"$ret" != "ok"} {
			echo "mailadmin: Unable to save $mag_host_name:$MAILCF"
			mag_cmd_quit 1
		}
		set ret [mag_remote_copyout $MAILDEFTMP $MAILDEF]
@if test
		if {"$TEST" == "grp5_copy2"} {
			set ret fail
		}
@endif
		if {"$ret" != "ok"} {
			echo "mailadmin: Unable to save $mag_host_name:$MAILDEF"
			mag_cmd_quit 1
		}
	}
}

proc \
mag_cmd_quit { code } \
{
	cleanup
	exit $code
}
@if test
# special test commands

proc \
mag_cmd_shortcut {} \
{
	global TEST
	global MA_SENDMAIL_FILEID
	global mag_host_name
	global argv

	# shortcut to execute routines directly by name
	set list [lvarpop argv]
	while {"$argv" != ""} {
		if {"[lindex $argv 0]" == "-test"} {
			break
		}
		set list "$list [lvarpop argv]"
	}
	if {[catch {eval $list} ret] != 0} {
		echo ERROR: $ret
	} else {
		echo $ret
	}
}
@endif
