#ident "@(#)host.tcl	11.3"
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
mag_open_host {} \
{
	global mag_host_name mag_host_label mag_host_resolv outerMainForm
	global mainscreen_db

	# first save any previous changes, but keep open for cancel.
	set ret [mag_all_put]

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
	set newhost [mag_open_host_dialog]
	if {"$newhost" == ""} {
		return
	}

	# get full name of remote machine
	set newhost [mag_host_resolv $newhost]

	# save old host name for backout
	set oldhost $mag_host_name

	# check if new host there
	set ret [mag_host_check $newhost]
	if {"$ret" != "ok"} {
		VtUnLock
		mag_error1 BADHOST $newhost
		VtLock
		return
	}

	# set new host name
	set mag_host_name $newhost

	set ret [mag_cf_get]
	if {"$ret" == "no"} {
		mag_host_backout $oldhost
		return
	}
	if {"$ret" == "fail"} {
		mag_host_backout $oldhost
		return
	}
	set ret [mag_ms1_get]
	if {"$ret" == "no"} {
		mag_host_backout $oldhost
		return
	}
	if {"$ret" == "fail"} {
		mag_host_backout $oldhost
		return
	}
	VtSetValues $outerMainForm -title [mag_msg1 WIN_TITLE \
		[mag_short_name_default]]
	VtSetValues $mag_host_label -label [mag_msg1 TITLE $mag_host_name]
	# clear and redraw main list
	mag_delete_mainlist -1 [llength $mainscreen_db]
        mao_init
        mag_insert_mainlist -1 [llength $mainscreen_db]
        mag_display_mainlist 0 1 0
}

# attempt to restore old host configuration files
proc \
mag_host_backout { oldhost } \
{
	global mag_host_name mag_host_label DIRTY

	VtUnLock
	set ret [mag_query_qyn GO_BACK $oldhost]
	VtLock
	if {"$ret" != "yes"} {
		quit
	}
	set mag_host_name $oldhost
	set DIRTY 0
	set ret [mag_cf_get]
	if {"$ret" == "no"} {
		quit
	}
	if {"$ret" == "fail"} {
		quit
	}
	set ret [mag_ms1_get]
	if {"$ret" == "no"} {
		quit
	}
	if {"$ret" == "fail"} {
		quit
	}
}

# open host dialog, returns new host or "" if none selected.
proc \
mag_open_host_dialog {} \
{
	global ME PID

	# file is used to pass results to preserve stdout for child process.
	set file "/tmp/mag.$PID"
	system "rm -f $file"
	VtControl -suspend
	system "$ME -mag_stand_host $file"
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
mag_stand_host { file } \
{
	global app
	global mag_stand_text mag_stand_outfile mag_stand_form

	# globals for callbacks
	set mag_stand_outfile $file

	# init widget server
	set app [VtOpen mailchild "mailadmin"]
	mag_setcat SCO_MAIL_ADMIN

	# std dialog box.
	set mag_stand_form [ \
		VtFormDialog $app.mailchild \
		-title [mag_msg TITLE_OPEN] \
		-resizable FALSE \
		-ok -okCallback mag_stand_host_ok \
		-cancel -cancelCallback mag_stand_open_quit \
		-wmCloseCallback mag_stand_open_quit \
	]
	set mag_stand_text [ \
		VtText $mag_stand_form.text \
		-columns 40 \
		-callback mag_stand_host_jump \
	]
	VtPushButton $mag_stand_form.browse \
		-leftSide $mag_stand_text \
		-topSide FORM \
		-callback mag_stand_host_browse \
		-label [mag_msg STR_SELECT]
	VtShow $mag_stand_form
	VtSetFocus $mag_stand_text
	VtMainLoop
}

# enter calllback for text widget
proc \
mag_stand_host_jump { cbs } \
{
	global mag_stand_form

	set ok [VtGetValues $mag_stand_form -ok]
	VtSetFocus $ok
}

# browse button callback
proc \
mag_stand_host_browse { cbs } \
{
	global mag_stand_form

	keylset parms parent $mag_stand_form
	keylset parms instance abc
	keylset parms labeltext [mag_msg MENU_HOST]
	keylset parms buttontext [mag_msg STR_SELECT]
	keylset parms userproc mag_stand_host_browse_done
	SaSelectHostDialog $parms $cbs
}

# browse done callback
proc \
mag_stand_host_browse_done { cbs } \
{
	global mag_stand_text

	set value [SaSelectHostGetSelection abc]
	VtSetValues $mag_stand_text -value $value
}

# ok callback
proc \
mag_stand_host_ok { cbs } \
{
	global mag_stand_text mag_stand_outfile

	set value [VtGetValues $mag_stand_text -value]

	if {[catch {open $mag_stand_outfile w} fd] == 0} {
		puts $fd $value
		close $fd
	}

	VtClose
	exit 0
}

# quit callback
proc \
mag_stand_open_quit { cbs } \
{
	VtClose
	exit 0
}

# check if host exists, run uname on it via remote OSA
proc \
mag_host_check { host } \
{
	global mag_host_name mag_local_host

	if {"$host" == "$mag_local_host"} {
		return "ok"
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
# do local copy if mag_host_name matches default
# returns ok, fail
# Note: assume we have permission on $src, but not necessarily on $dst
proc \
mag_remote_copyout { src dst } \
{
	global mag_host_name mag_local_host

	if {"$mag_host_name" == "$mag_local_host"} {
		return [mag_local_copy $src $dst]
	}

	set host $mag_host_name
	set instance [GetInstanceList NULL $host]
	set uid [id effective userid]

	if {$uid == 0} {
		if {[catch {system "/bin/rcp $src $host:$dst >/dev/null 2>&1"} ret] == 0} {
			return ok
		} else {
			return fail
		}
	} else {
		# first copy to a temp file on remote machine
		set tmp "/tmp/[file tail $src].[pid]"
		if {[catch {system "/bin/rcp $src $host:$tmp >/dev/null 2>&1"} ret] != 0} {
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
# do local copy if mag_host_name matches default
# returns ok, fail
# Note: assumes you have permissions on $dst, but not necessarily on $src
proc \
mag_remote_copyin { src dst } \
{
	global mag_host_name mag_local_host

	if {"$mag_host_name" == "$mag_local_host"} {
		return [mag_local_copy $src $dst]
	}

	set host $mag_host_name
	set instance [GetInstanceList NULL $host]
	set uid [id effective userid]

	if {$uid == 0} {
		if {[catch {system "/bin/rcp $mag_host_name:$src $dst >/dev/null 2>&1"} ret] == 0} {
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
		if {[catch {system "/bin/rcp $mag_host_name:$tmp $dst >/dev/null 2>&1"} ret] != 0} {
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
mag_local_copy { src dst } \
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

# using the remote command osa if needed, restart sendmail
# prompt is whether the GUI or cmd line is calling it.
proc \
mag_restart_sendmail { prompt } \
{
	global mag_host_name mag_local_host SENDMAILPID

	set uid [id effective userid]

	if {"$mag_host_name" == "$mag_local_host"} {
		if {$uid == 0} {
			system "kill -HUP `head -1 $SENDMAILPID`"
		} else {
			system "/sbin/tfadmin kill -HUP `head -1 $SENDMAILPID`"
		}
		return ok
	}

	set class [ list sco remoteCommand ]
	set instance [GetInstanceList NULL $mag_host_name]
	if {$uid == 0} {
		set command [list ObjectAction $class $instance \
			"/usr/bin/kill" "-HUP `head -1 $SENDMAILPID`"]
	} else {
		set command [list ObjectAction $class $instance \
			"/sbin/tfadmin" "kill -HUP `head -1 $SENDMAILPID`"]
	}
	set result [SaMakeObjectCall $command]
	set result [lindex $result 0]
	if { [BmipResponseErrorIsPresent result] } {
		if {"$prompt" == "yes"} {
			VtUnLock
			mag_error SENDMAIL_RESTART
			VtLock
		}
		return fail
	}

	return ok
}

# get the local host name and try to fully qualify it.
proc \
mag_host_local {} \
{
	set localhost [exec uname -n]
	set fqdn [mag_host_resolv $localhost]
	return $fqdn
}

# resolv a host, if unable to resolv it returns original string unchanged.
proc \
mag_host_resolv { hostname } \
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
mag_list_users {} \
{
	global mag_host_name mag_local_host

	if {"$mag_host_name" == "$mag_local_host"} {
		return [SaUsersGet]
	} else {
		return [SaUsersGet $mag_host_name]
	}
}

# return list of valid groupnames for our managed host.
proc \
mag_list_groups {} \
{
	global mag_host_name mag_local_host

	if {"$mag_host_name" == "$mag_local_host"} {
		return [SaGroupsGet]
	} else {
		return [SaGroupsGet $mag_host_name]
	}
}

# check if remote file is there
proc \
mag_remote_file_exists { path } \
{
	global mag_host_name mag_local_host

	if {"$mag_host_name" == "$mag_local_host"} {
		return [file exists $path]
	}

	set host $mag_host_name
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
