#ident "@(#)host.tcl	11.1"
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
# remote host services
#

#
# open host.
#
proc \
mhm_open_host {} \
{
	global mhm_host_name mhm_host_label mhm_host_resolv mhm_vdomains_data
	global outerMainForm

	# first save any previous changes, but keep open for cancel.
	set ret [mhm_all_put]

	switch $ret {
	"ok" {
	}
	"fail" {
		return
	}
	"cancel" {
		return
	}
	}

	# put up open host dialog and get result
	set newhost [mhm_open_host_dialog]
	if {"$newhost" == ""} {
		return
	}

	# get full name of remote machine
	set newhost [mhm_host_resolv $newhost]
	if {"$newhost" == ""} {
		VtUnLock
		mhm_error1 NOHOST $newhost
		VtLock
		return
	}

	# save old host name for backout
	set oldhost $mhm_host_name

	# check if new host there
	set ret [mhm_host_check $newhost]
	if {"$ret" != "ok"} {
		VtUnLock
		mhm_error1 BADHOST $newhost
		VtLock
		return
	}

	# set new host name
	set mhm_host_name $newhost
	# unset cached list of vdomains
	unset mhm_vdomains_data

	set ret [mhm_all_get]
	if {"$ret" == "no"} {
		mhm_host_backout $oldhost
		return
	}
	if {"$ret" == "fail"} {
		mhm_host_backout $oldhost
		return
	}
	set mhm_short_name [lindex [split $mhm_host_name "."] 0]
        VtSetValues $outerMainForm -title [mhm_msg1 TITLE $mhm_short_name]
	VtSetValues $mhm_host_label -label [mhm_msg1 STATUS $mhm_host_name]
	mhm_display_mainlist
}

# attempt to restore old host configuration files
proc \
mhm_host_backout { oldhost } \
{
	global mhm_host_name mhm_host_label mhm_vdomains_data DIRTY

	VtUnLock
	set ret [mhm_query_qyn GO_BACK $oldhost]
	VtLock
	if {"$ret" != "yes"} {
		quit
	}
	set mhm_host_name $oldhost
	unset mhm_vdomains_data
	set DIRTY 0
	set ret [mhm_all_get]
	if {"$ret" == "no"} {
		quit
	}
	if {"$ret" == "fail"} {
		quit
	}
}

# open host dialog, returns new host or "" if none selected.
proc \
mhm_open_host_dialog {} \
{
	global ME PID

	# file is used to pass results to preserve stdout for child process.
	set file "/tmp/mhm.$PID"
	system "rm -f $file"
	VtControl -suspend
	system "$ME -mhm_stand_host $file"
	VtControl -resume
	if {[file exists $file] == 1} {
		set result [exec cat $file]
		system "rm -f $file"
	} else {
		set result ""
	}
	return $result
}

# stand-alone routine to put up open host dialog box
proc \
mhm_stand_host { file } \
{
	global app
	global mhm_stand_text mhm_stand_outfile mhm_stand_form

	# globals for callbacks
	set mhm_stand_outfile $file

	# init widget server
	set app [VtOpen mailchild "mailadmin"]
	mhm_setcat SCO_USER_MHOME

	# std dialog box.
	set mhm_stand_form [ \
		VtFormDialog $app.mailchild \
		-title [mhm_msg TITLE_OPEN] \
		-resizable FALSE \
		-ok -okCallback mhm_stand_host_ok \
		-cancel -cancelCallback mhm_stand_open_quit \
		-wmCloseCallback mhm_stand_open_quit \
	]
	set mhm_stand_text [ \
		VtText $mhm_stand_form.text \
		-columns 40 \
		-callback mhm_stand_host_jump \
	]
	VtPushButton $mhm_stand_form.browse \
		-leftSide $mhm_stand_text \
		-topSide FORM \
		-callback mhm_stand_host_browse \
		-label [mhm_msg STR_BROWSE]
	VtShow $mhm_stand_form
	VtSetFocus $mhm_stand_text
	VtMainLoop
}

# enter calllback for text widget
proc \
mhm_stand_host_jump { cbs } \
{
	global mhm_stand_form

	set ok [VtGetValues $mhm_stand_form -ok]
	VtSetFocus $ok
}

# browse button callback
proc \
mhm_stand_host_browse { cbs } \
{
	global mhm_stand_form

	keylset parms parent $mhm_stand_form
	keylset parms instance abc
	keylset parms labeltext [mhm_msg MENU_HOST]
	keylset parms buttontext [mhm_msg STR_BROWSE]
	keylset parms userproc mhm_stand_host_browse_done
	SaSelectHostDialog $parms $cbs
}

# browse done callback
proc \
mhm_stand_host_browse_done { cbs } \
{
	global mhm_stand_text

	set value[SaSelectHostGetSelection abc]
	VtSetValues $mhm_stand_text -value $value
}

# ok callback
proc \
mhm_stand_host_ok { cbs } \
{
	global mhm_stand_text mhm_stand_outfile

	set value [VtGetValues $mhm_stand_text -value]

	if {[catch {open $mhm_stand_outfile w} fd] == 0} {
		puts $fd $value
		close $fd
	}

	VtClose
	exit 0
}

# quit callback
proc \
mhm_stand_open_quit { cbs } \
{
	VtClose
	exit 0
}

# check if host exists, run uname on it via remote OSA
proc \
mhm_host_check { host } \
{
	global mhm_host_name mhm_local_host

	if {"$host" == "$mhm_local_host"} {
		return ok
	}

	set class [ list sco remoteCommand ]
	set instance [GetInstanceList NULL $host]
	set command [list ObjectAction $class $instance uname "-n"]
	set result [SaMakeObjectCall $command]
	set result [lindex $result 0]
	if { [BmipResponseErrorIsPresent result] } {
		return fail
	}
	set retStr [BmipResponseActionInfo result]
	return ok
}

# copy a local file to a remote one,
# do local copy if mhm_host_name matches default
# returns ok, fail
# Note: assume we have permission on $src, but not necessarily on $dst
proc \
mhm_remote_copyout { src dst } \
{
	global mhm_host_name mhm_local_host

	if {"$mhm_host_name" == "$mhm_local_host"} {
		return [mhm_local_copy $src $dst]
	}

	set host $mhm_host_name
	set instance [GetInstanceList NULL $host]
	set uid [id effective userid]

	if {$uid == 0} {
		if {[catch {system "/bin/rcp $src $host:$dst"} ret] == 0} {
			return ok
		} else {
			return fail
		}
	} else {
		# first copy to a temp file on remote machine
		set tmp "/tmp/[file tail $src].[pid]"
		if {[catch {system "/bin/rcp $src $host:$tmp"} ret] != 0} {
			return fail
		}
		# then on remote machine, copy the temp file to destination,
		# using tfadmin cpfile
		set class [list sco remoteCommand]
		set command [list ObjectAction $class $instance /sbin/tfadmin "cpfile $tmp $dst"]
		set result [SaMakeObjectCall $command]
		set result [lindex $result 0]
		if {[BmipResponseErrorIsPresent result]} {
			return fail
		}
		# now remove the temp file on remote machine
		set class [list sco remoteCommand]
		set command [list ObjectAction $class $instance /bin/rm "-f $tmp"]
		set result [SaMakeObjectCall $command]
		return ok
	}
}

# copy a remote file to a local one,
# do local copy if mhm_host_name matches default
# returns ok, fail
# Note: assumes you have permissions on $dst, but not necessarily on $src
proc \
mhm_remote_copyin { src dst } \
{
	global mhm_host_name mhm_local_host

	if {"$mhm_host_name" == "$mhm_local_host"} {
		return [mhm_local_copy $src $dst]
	}

	set host $mhm_host_name
	set instance [GetInstanceList NULL $host]
	set uid [id effective userid]

	if {$uid == 0} {
		if {[catch {system "/bin/rcp $host:$src $dst"} ret] == 0} {
			return ok
		} else {
			return fail
		}
	} else {
		# copy to a temp file on remote machine, using tfadmin
		set tmp "/tmp/[file tail $src].[pid]"
		set class [list sco remoteCommand]
		set command [list ObjectAction $class $instance /sbin/tfadmin "cpfile $src $tmp"]
		set result [SaMakeObjectCall $command]
		set result [lindex $result 0]
		if {[BmipResponseErrorIsPresent result]} {
			return fail
		}
		# copy the remote temp file in to local destination using rcp
		if {[catch {system "/bin/rcp $host:$tmp $dst"} ret] != 0} {
			return fail
		}
		# remove the temp file on remote machine
		set class [list sco remoteCommand]
		set command [list ObjectAction $class $instance /bin/rm "-f $tmp"]
		set result [SaMakeObjectCall $command]
                return ok
	}
}

# do copy, return ok, fail
proc \
mhm_local_copy { src dst } \
{
	set uid [id effective userid]

	if {$uid == 0} {
		catch {system "copy $src $dst > /dev/null 2>&1"} ret
	} else {
		catch {system "/sbin/tfadmin cpfile $src $dst > /dev/null 2>&1"} ret
	}

	if {"$ret" == 0} {
		return ok
	} else {
		return fail
	}
}

# copied from scoadmin defaults manager
proc \
GetInstanceList { names host } \
{
	set instanceList {}
	if { $host == "localhost" } {
		foreach name $names {
			set instance [list $name]
			lappend instanceList $instance
		}
	} else {
		foreach name $names {
			set instance [list [list systemId $host] [list $name]]
			lappend instanceList $instance
		}
	}
	return $instanceList
}

# get the local host name and try to fully qualify it.
proc \
mhm_host_local {} \
{
	set localhost [exec uname -n]
	set fqdn [mhm_host_resolv $localhost]
	return $fqdn
}

# resolv a host, if unable to resolv it returns original string unchanged.
proc \
mhm_host_resolv { hostname } \
{
	global RESOLVER

	set ret [catch {exec $RESOLVER $hostname} hostlist]
	if {"$ret" != "0"} {
		return $hostname
	}
	set newhost [lindex $hostlist 0]
	return $newhost
}

# return list of valid users for our managed host
proc \
mhm_list_users {} \
{
	global mhm_host_name mhm_local_host

	if {"$mhm_host_name" == "$mhm_local_host"} {
		set users [SaUsersGet]
	} else {
		set users [SaUsersGet $mhm_host_name]
	}
	set users [string tolower $users]
	return $users
}

# check if remote file is there
proc \
mhm_remote_file_exists { path } \
{
	global mhm_host_name mhm_local_host

	if {"$mhm_host_name" == "$mhm_local_host"} {
		return [file exists $path]
	}

	set host $mhm_host_name
	set class [list sco remoteCommand]
	set instance [GetInstanceList NULL $host]
	set command [list ObjectAction $class $instance file $path]

	set result [SaMakeObjectCall $command]
	set result [lindex $result 0]

	if { [BmipResponseErrorIsPresent result] } {
		return 0
	}
	return 1
}

# if we are root, run mkdir -p on a path
# otherwise, we must use tfadmin
# return 1 or 0
proc \
mhm_remote_mkdir { path } \
{
	global mhm_host_name mhm_local_host

	set uid [id effective userid]

	if {[mhm_remote_file_exists $path]} {
		return 1
	}

	if {"$mhm_host_name" == "$mhm_local_host"} {
		if {$uid == 0} {
			catch {system "mkdir -p $path > /dev/null 2>&1"} ret
			if {$ret == 0} {
				return 1
			} else {
				return 0
			}
		} else {
			# create the path in /tmp
			set tmpdir "/tmp/tmpdir.[pid]"
			set tmpvar $path
			set basedir [ctoken tmpvar "/"]
			catch {system "mkdir -p $tmpdir/$path > /dev/null 2>&1"} ret
			if {$ret != 0} {
				return 0
			}
			# now use tfadmin cpfile to create the path
			if {[catch {exec /sbin/tfadmin cpfile [list -r $tmpdir/$basedir] /} ret] != 0} {
				return 0
			}
			# remove the tmp dir
			catch {system "rm -rf $tmpdir"} ret
			return 1
		}
	}

	# guess this is remote
	set host $mhm_host_name
	set instance [GetInstanceList NULL $host]
	if {$uid == 0} {
		set class [list sco remoteCommand]
		set command [list ObjectAction $class $instance mkdir "-p $path"]
		set result [SaMakeObjectCall $command]
		set result [lindex $result 0]
		if { [BmipResponseErrorIsPresent result] } {
			return 0
		}
		return 1
	} else {
		# create the path in /tmp on remote host
		set tmpdir "/tmp/tmpdir.[pid]"
		set class [list sco remoteCommand]
		set command [list ObjectAction $class $instance mkdir "-p $tmpdir/$path"]
		set result [SaMakeObjectCall $command]
		set result [lindex $result 0]
		if { [BmipResponseErrorIsPresent result] } {
			return 0
		}

		# now use tfadmin cpfile to create the path on remote host
		set tmpvar $path
		set basedir [ctoken tmpvar "/"]
		set arglist [list cpfile [list -r $tmpdir/$basedir] /]
		set command [list ObjectAction $class $instance /sbin/tfadmin $arglist]
		set result [SaMakeObjectCall $command]
		set result [lindex $result 0]
		if { [BmipResponseErrorIsPresent result] } {
			return 0
		}

		# remove the tmp dir on remote host
		set command [list ObjectAction $class $instance /bin/rm "-rf $tmpdir"]
		set result [SaMakeObjectCall $command]
		set result [lindex $result 0]
		return 1
	}
}

#
# routine to get the virtual domain list from our managed host
# this routine caches the data
#
proc \
mhm_vdomains {} \
{
	global mhm_host_name mhm_local_host VDOMAINS mhm_vdomains_data

	if {[info exists mhm_vdomains_data]} {
		return $mhm_vdomains_data
	}

	if {"$mhm_host_name" == "$mhm_local_host"} {
		set ret [catch {exec "$VDOMAINS"} output]
		if {$ret != 0} {
			set mhm_vdomains_data ""
			return $mhm_vdomains_data
		}
		set newlist [split $output "\n"]
		set mhm_vdomains_data $newlist
		return $mhm_vdomains_data
	}

	set host $mhm_host_name
	set class [list sco remoteCommand]
	set instance [GetInstanceList NULL $host]
	set command [list ObjectAction $class $instance $VDOMAINS ""]

	set result [SaMakeObjectCall $command]
	set result [lindex $result 0]

	if { [BmipResponseErrorIsPresent result] } {
		return ""
	}
	set output [BmipResponseActionInfo result]
	set newlist [split $output "\n"]
	set mhm_vdomains_data $newlist
	return $mhm_vdomains_data
}

#
# routine to get remote host aliases or virtual domains.
# returns a list of strings which are the virtual domain names.
# the physical domain is not returned in the list.
# returns the string "fail" if an error occurs.
#
proc \
mhm_remote_aliases {} \
{
	global mhm_host_name mhm_local_host

	set list [mhm_vdomains]
	set newlist ""
	foreach item $list {
		set value [lindex $item 0]
		lappend newlist $value
	}
	return $newlist
}

# convert a remote alias to an ip address, try our local cache.
# if it is not there return an error.
proc \
mhm_remote_alias_to_ip { alias } \
{
	set list [mhm_vdomains]

	foreach item $list {
		set value [lindex $item 0]
		if {"$value" == "$alias"} {
			set ip [lindex $item 1]
			return $ip
		}
	}
	return fail
}
