#ident "@(#)cmd.tcl	11.1"
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
# the command line interface goes here.
#

proc \
mhm_cmd_line_main {} \
{
	global argv argv0 mhm_host_name DIRTY

	if {"$argv" == ""} {
		return
	}
	# check if remote host is there
	if {"[mhm_host_check $mhm_host_name]" != "ok"} {
		echo "multihome: Unable to connect to host $mhm_host_name"
		exit 0
	}
	# check if authorized
	if {"[mhm_authorized $mhm_host_name]" == "fail"} {
		set mhm_short_name [lindex [split $mhm_host_name "."] 0]
		echo "multihome: You are not authorized to run the Virtual Domain User Manager\n           on $mhm_short_name"
		exit 0
	}

	# get our remote files from the appropriate host
	mhm_cmd_file_get

	set DIRTY 0

	# now process rest of command line
	while {"$argv" != ""} {
		# do one command
		mhm_cmd_one
	}

	# put our remote files
	mhm_cmd_file_put
	mhm_cmd_quit 0
}

# do one command from the argv list
proc \
mhm_cmd_one {} \
{
	global CMD argv DIRTY
	global ALIAS ALIASDB ALIASTMP ALIASTMPDB MHOMEPATH MAKEMAP
@if test
	global TEST
@endif

	set cmd [mhm_cmd_popargv]
	set CMD $cmd

	switch -- "$cmd" {
	list {
		set type [mhm_cmd_popargv]
		switch "$type" {
		domains {
			set list [mhm_union_domains]
		}
		users {
			set domain [mhm_cmd_popargv]
			mhm_cmd_valid_domain $domain
			set list [mh_vd_users_get $domain]
		}
		default {
			echo "multihome: Invalid list type: $type"
			mhm_cmd_quit 1
		}
		}
		foreach item $list {
			echo $item
		}
	}
	del {
		set user [mhm_cmd_popargv]
		set useri [string tolower $user]
		set domain [mhm_cmd_popargv]
		mhm_cmd_valid_domain $domain
		set users [mh_vd_users_get $domain]
		set index -1
		set found -1
		foreach i $users {
			set index [expr $index + 1]
			set t1 [lindex $i 0]
			set t1 [string tolower $t1]
			if {$t1 == $useri} {
				set found $index
				break
			}
		}
		if {$found == -1} {
			echo "multihome: User not found for delete: $user"
			mhm_cmd_quit 1
		}
		set users [lreplace $users $index $index]
		mh_vd_users_set $domain $users
		set DIRTY 1
	}
	add {
		set user [mhm_cmd_popargv]
		set realuser [mhm_cmd_popargv]
		set useri [string tolower $user]
		set domain [mhm_cmd_popargv]
		mhm_cmd_valid_domain $domain
		set users [mh_vd_users_get $domain]
		set index -1
		set found -1
		foreach i $users {
			set index [expr $index + 1]
			set t1 [lindex $i 0]
			set t1 [string tolower $t1]
			if {$t1 == $useri} {
				set found $index
				break
			}
		}
		if {$found != -1} {
			echo "multihome: User already exists in domain: $user@$domain"
			mhm_cmd_quit 1
		}
		set newuser [list $user $realuser]
		lappend users $newuser
		mh_vd_users_set $domain $users
		set DIRTY 1
	}
	aliasget {
		set domain [mhm_cmd_popargv]
		mhm_cmd_valid_domain $domain
		set file [mhm_cmd_popargv]
		set addr [mhm_remote_alias_to_ip $domain]
		if {"$addr" == "fail"} {
			echo "multihome: Unable to resolv name $domain"
			mhm_cmd_quit 1
		}
		set rfile $MHOMEPATH/$addr/mail/$ALIAS
		set ret [mhm_remote_copyin $rfile $file]
		if {"$ret" != "ok"} {
			echo "multihome: Copy error on $rfile"
			mhm_cmd_quit 1
		}
	}
	aliasput {
		set domain [mhm_cmd_popargv]
		mhm_cmd_valid_domain $domain
		set file [mhm_cmd_popargv]
		set addr [mhm_remote_alias_to_ip $domain]
		if {"$addr" == "fail"} {
			echo "multihome: Unable to resolv name $domain"
			mhm_cmd_quit 1
		}
		set rfile $MHOMEPATH/$addr/mail/$ALIAS

		system "rm -fr $ALIASTMP $ALIASTMPDB"
		# first make db file
		mhm_local_copy $file $ALIASTMP
		catch {system "$MAKEMAP hash $file < $file > /dev/null 2>&1"} ret
		if {$ret != 0} {
			echo "multihome: Unable to make db file: $file"
			mhm_cmd_quit 1
		}

		# make the directory if it doesn't exist
		if {[mhm_remote_file_exists $MHOMEPATH/$addr/mail] == 0} {
			mhm_remote_mkdir $MHOMEPATH/$addr/mail
		}
		# copy out alias file
		set ret [mhm_remote_copyout $file $rfile]
@if test
		if {"$TEST" == "cmd_aput2"} {
			set ret fail
		}
@endif
		if {"$ret" != "ok"} {
			echo "multihome: Copy error on $domain:$rfile"
			mhm_cmd_quit 1
		}
		# copy out alias db file
		set ret [mhm_remote_copyout $file.db $rfile.db]
@if test
		if {"$TEST" == "cmd_aput3"} {
			set ret fail
		}
@endif
		if {"$ret" != "ok"} {
			echo "multihome: Copy error on $domain:$rfile.db"
			mhm_cmd_quit 1
		}
	}
	retire {
		set user [mhm_cmd_popargv]
		set ret [mhm_retire $user]
		if {"$ret" != "ok"} {
			echo "multihome: Retire user not found in any domain: $user"
			mhm_cmd_quit 1
		} else {
			set DIRTY 1
		}
	}
	domain {
		mhm_rebuild_domain_map no
	}
	default {
		echo "multihome: Unknown command: $cmd"
		mhm_cmd_usage
		mhm_cmd_quit 1
	}
	}
}

proc \
mhm_cmd_usage {} \
{
# no indents to make this easier
echo "Usage:"
echo "multihome"
echo "    invoke GUI"
echo "multihome \[-h host\] command \[arguments\]"
echo "    Command line mode, following are the commands and their arguments."
echo "    Multiple command may be strung together on the command line."
echo "    If any of them fail no changes to the config files are made."
echo
echo "commands:"
echo "list domains - output list of virtual domains."
echo "list users domain - output list of users contained in that virtual domain."
echo "    The virtual usernames are listed in the first column,"
echo "    followed by their physical user name in the second."
echo "del user domain - delete a virtual user from a domain name."
echo "add user realuser domain - add a virtual user to a domain."
echo "retire user - retire a system user from all virtual domains."
echo "aliasget domain file - get the remote alias file for a domain."
echo "aliasput domain file - put the remote alias file for a domain."
echo "domain - rebuild domain database file from TCP configuration"
echo "    if virtual domains have been added or deleted then this must be done."
echo "    It is done automatically in GUI mode."
}

# pop an arg from argv and verify that it actually exists.
proc \
mhm_cmd_popargv {} \
{
	global argv CMD

	if {"$argv" == ""} {
		echo "multihome: Not enough arguments for command: $CMD"
		mhm_cmd_quit 1
	}
	return [lvarpop argv]
}

# get our remote file, similar to mh_all_get, automatically creates virtusers
# file if it is not there.
proc \
mhm_cmd_file_get {} \
{
	global MHOME MHOMEDB MHOMETMP MHOMETMPDB MHOMETMPODB MHOMEPATH
	global mhm_host_name MHOMELPATH
@if test
	global TEST
@endif

	system "rm -fr $MHOMETMP $MHOMETMPDB $MHOMETMPODB"
	set ret [mhm_remote_copyin $MHOME $MHOMETMP]
	if {"$ret" != "ok"} {
		# assume file was not there, create the directory, make file.
		set ret [mhm_remote_mkdir $MHOMELPATH]
@if test
		if {"$TEST" == "cmd_md1"} {
			set ret 0
		}
@endif
		if {$ret != 1} {
			echo "multihome: Unable to mkdir $MHOMELPATH"
			mhm_cmd_quit 1
		}
		system "rm -fr $MHOMETMP; touch $MHOMETMP"
	}
	mhm_remote_copyin $MHOMEDB $MHOMETMPODB

	# now open it
	set ret [mh_open $MHOMETMP]
@if test
	if {"$TEST" == "cmd_parse"} {
		set ret fail
	}
@endif
	if {"$ret" == "fail"} {
		echo "multihome: Unable to parse $mhm_host_name:$MHOME"
		mhm_cmd_quit 1
	}	
}

# put our remote files back
proc \
mhm_cmd_file_put {} \
{
	global MHOME MHOMEDB MHOMETMP MHOMETMPDB MHOMETMPODB MAKEMAP
	global mhm_host_name DIRTY
@if test
	global TEST
@endif

	if {$DIRTY == 1} {
		set ret [mh_write]
@if test
		if {"$TEST" == "cmd_put1"} {
			set ret fail
		}
@endif
		if {"$ret" != "ok"} {
			echo "multihome: Unable to save: $MHOME"
			mhm_cmd_quit 1
		}
		set ret [mhm_remote_copyout $MHOMETMP $MHOME]
@if test
		if {"$TEST" == "cmd_put2"} {
			set ret fail
		}
@endif
		if {"$ret" != "ok"} {
			echo "multihome: Unable to save: $mhm_host_name:$MHOME"
			mhm_cmd_quit 1
		}
		catch {system "$MAKEMAP hash $MHOMETMP < $MHOMETMP > /dev/null 2>&1"} ret
		if {$ret != 0} {
			echo "multihome: Unable to make map for $MHOME"
			mhm_cmd_quit 1
		}
		set ret [mhm_remote_copyout $MHOMETMPDB $MHOMEDB]
@if test
		if {"$TEST" == "cmd_put4"} {
			set ret fail
		}
@endif
		if {"$ret" != "ok"} {
			echo "multihome: Unable to save: $mhm_host_name:$MHOMEDB"
			mhm_cmd_quit 1
		}
	}
}

proc \
mhm_cmd_quit { code } \
{
	global MHOMETMP MHOMETMPDB MHOMETMPODB ALIASTMP ALIASTMPDB ALIASTMPODB

	system "rm -fr $MHOMETMP $MHOMETMPDB $MHOMETMPODB $ALIASTMP $ALIASTMPDB $ALIASTMPODB"
	exit $code
}

proc \
mhm_cmd_valid_domain { domain } \
{
	set list [mhm_union_domains]
	set list [string tolower $list]
	set idomain [string tolower $domain]
	set index [lsearch $list $idomain]
	if {$index == -1} {
		echo "multihome: Invalid domain: $domain"
		mhm_cmd_quit 1
	}
}

@if test
# special test commands

proc \
mhm_cmd_shortcut {} \
{
	global TEST
	global mhm_host_name
	global argv

	# shortcut to execute routines directly by name
	set list [lvarpop argv]
	while {"$argv" != ""} {
		if {"[lindex $argv 0]" == "-test"} {
			break
		}
		set list "$list [lvarpop argv]"
	}
	catch {eval $list} ret
	echo $ret
}
@endif
