#ident "@(#)main.tcl	11.2"
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
# user multihome administration main program.
#

# app globals go here

# version number
set versionNumber [SaGetSSOVersion]

# our resource path
set RESPATH /etc/mail/admin/px
# our pid
set PID [pid]
# our path since we call ourself via exec
set ME "/etc/mail/admin/multihome"

# the files we are editting
set MHOMEPATH /var/internet/ip
set MHOMELPATH /var/internet/ip/127.0.0.1
set MHOME /var/internet/ip/127.0.0.1/virtusers
set MHOMEDB /var/internet/ip/127.0.0.1/virtusers.db
set MHOMETMP /tmp/virtusers.$PID
set MHOMETMPDB /tmp/virtusers.$PID.db
set MHOMETMPODB /tmp/virtusers.$PID.odb

set MDOMAINPATH /var/internet/ip/127.0.0.1/mail
set MDOMAIN /var/internet/ip/127.0.0.1/mail/virtdomains
set MDOMAINDB /var/internet/ip/127.0.0.1/mail/virtdomains.db

# alias files we pass on to alias file editor
set ALIAS mail.aliases
set ALIASDB mail.aliases.db
set ALIASTMP /tmp/alias.$PID
set ALIASTMPDB /tmp/alias.$PID.db
set ALIASTMPODB /tmp/alias.$PID.db

# the alias editor
set ALIAS_EDITOR /etc/mail/admin/aliases

# make our new db file.
set MAKEMAP /etc/mail/makemap

set RESOLVER /usr/sbin/host
set VDOMAINS /etc/mail/vdomains
# our managed host starts as local
set mhm_host_name [mhm_host_local]
set mhm_local_host $mhm_host_name

#
# function that redraws the main list based upon the file contents.
# Only two icons are used:
# domain icon (looks like a sheet of paper with lines on it).
# domain icon with international no (red circle), means domain was found
#	that does not exist in the TCP virtual domain configuration.
#
proc \
mhm_display_mainlist {} \
{
	global main_list

	set domains [mhm_union_domains]
	set realdomains [mhm_remote_aliases]
	set length [llength $domains]

	VtDrawnListDeleteItem $main_list -all
	loop i 0 $length {
		set domain [lindex $domains $i]
		set icon 0
		if {[lsearch $realdomains $domain] == -1} {
			set icon 1
		}
		set fieldlist [list $icon $domain]
		set formatlist [list "ICON 1" "STRING 75"]

		set position [expr $i + 1]
		VtDrawnListAddItem $main_list \
			-position $position \
			-fieldList $fieldlist \
			-formatList $formatlist
	}
	VtDrawnListSelectItem $main_list -position 1
}

# add our widgets
proc \
mhm_build_widgets { parent } \
{
	global RESPATH main_list status_line
	global btn_user btn_alias
	global menu_user menu_alias
	global mhm_host_name mhm_host_label

	# menubar
	set menuBar [VtMenuBar $parent.menubar \
		-helpMenuItemList [SaHelpGetOptionsList]]

	set hostMenu [VtPulldown $menuBar.hostMenu -label [mhm_msg MENU_HOST] \
		-mnemonic [mhm_msg MENU_HOST_MN]]
	VtPushButton $hostMenu.open -label [mhm_msg MENU_OPEN] \
		-mnemonic [mhm_msg MENU_OPEN_MN] \
		-callback mhm_open_host_cb \
		-shortHelpString [mhm_msg SHORT_OPEN] \
		-shortHelpCallback mhm_short_help
	VtPushButton $hostMenu.exit -label [mhm_msg MENU_EXIT] \
		-mnemonic [mhm_msg MENU_EXIT_MN] \
		-callback "quit_cb 1" \
		-shortHelpString [mhm_msg SHORT_EXIT] \
		-shortHelpCallback mhm_short_help

	set editMenu [VtPulldown $menuBar.editMenu -label [mhm_msg MENU_EDIT] \
		-mnemonic [mhm_msg MENU_EDIT_MN]]
	set menu_user [VtPushButton $editMenu.User -label [mhm_msg MENU_USER] \
		-mnemonic [mhm_msg MENU_USER_MN] \
		-callback mhm_user_cb \
		-shortHelpString [mhm_msg SHORT_USER] \
		-shortHelpCallback mhm_short_help]
	set menu_alias [VtPushButton $editMenu.Alias -label [mhm_msg MENU_ALIAS] \
		-mnemonic [mhm_msg MENU_ALIAS_MN] \
		-callback mhm_alias_cb \
		-shortHelpString [mhm_msg SHORT_ALIAS] \
		-shortHelpCallback mhm_short_help]
	set menu_retire [VtPushButton $editMenu.Retire -label [mhm_msg MENU_RETIRE] \
		-mnemonic [mhm_msg MENU_RETIRE_MN] \
		-callback mhm_retire_cb \
		-shortHelpString [mhm_msg SHORT_RETIRE] \
		-shortHelpCallback mhm_short_help]

	# toolbar widget
	if {[VtInfo -charm] == 0} {
		set frameBar [VtFrame $parent.framebar \
			-topSide $menuBar \
			-topOffset 1 \
			-leftSide FORM \
			-leftOffset 2 \
			-rightSide FORM \
			-rightOffset 2 \
			-marginWidth 0 \
			-marginHeight 0 \
			-horizontalSpacing 0 \
			-verticalSpacing 0 \
			-shadowType IN \
		]
		set toolBar [VtForm $frameBar.toolbar \
			-leftSide FORM \
			-rightSide FORM \
			-borderWidth 0 \
			-marginHeight 0 \
			-marginWidth 0 \
			-horizontalSpacing 0 \
			-verticalSpacing 0 \
		]
		set btn_user [VtPushButton $toolBar.user \
			-label [mhm_msg MENU_USER] \
			-leftSide FORM \
			-topSide FORM \
			-bottomSide FORM \
			-callback mhm_user_cb \
			-shortHelpString [mhm_msg SHORT_USER] \
			-shortHelpCallback mhm_short_help \
		]
		set btn_alias [VtPushButton $toolBar.alias \
			-label [mhm_msg MENU_ALIAS] \
			-leftSide $btn_user \
			-topSide FORM \
			-bottomSide FORM \
			-callback mhm_alias_cb \
			-shortHelpString [mhm_msg SHORT_ALIAS] \
			-shortHelpCallback mhm_short_help \
		]
	}

	# Top status line
	set mhm_host_label [VtLabel $parent.toplab \
		-leftSide FORM -rightSide FORM \
		-label [mhm_msg1 STATUS $mhm_host_name] \
	]

	# list widget
	set main_list [VtDrawnList $parent.list -columns 78 -rows 8 \
		-autoSelect TRUE -selection BROWSE \
		-leftSide FORM -rightSide FORM \
		-horizontalScrollBar 1 \
		-defaultCallback mhm_user_cb \
		-iconList [list \
			$RESPATH/domain.px \
			$RESPATH/baddomain.px \
		]]

	# status line
	set status_line [SaStatusBar $parent.status 1]
	SaStatusBarSet $status_line [mhm_msg LOADING]

	VtSetValues $main_list -bottomSide $status_line
}

proc \
mhm_open_host_cb { cbs } \
{
	VtLock
	mhm_open_host
	VtUnLock
}

proc \
mhm_user_cb { cbs } \
{
	global main_list

	VtLock
        set index [VtDrawnListGetSelectedItem $main_list]
        set index [expr $index - 1]
        set domains [mhm_union_domains]
        set domain [lindex $domains $index]

	mhm_edit_users $domain
	VtUnLock
}

proc \
mhm_alias_cb { cbs } \
{
	global main_list

	VtLock
        set index [VtDrawnListGetSelectedItem $main_list]
        set index [expr $index - 1]
        set domains [mhm_union_domains]
        set domain [lindex $domains $index]

	mhm_edit_aliases $domain
	VtUnLock
}

proc \
mhm_retire_cb { cbs } \
{
	VtLock
	mhm_retire_user
	VtUnLock
}

proc \
mhm_short_help { cbs } \
{
	global status_line

	set helpstring [keylget cbs helpString]
	SaStatusBarSet $status_line $helpstring
}

# get /etc/sendmail.cf or a factory default version into /tmp
# set dirty if factory default file used.
# returns ok, fail, or no (user said no).
proc \
mhm_all_get {} \
{
	global DIRTY MHOME MHOMEDB MHOMETMP MHOMETMPDB MHOMETMPODB MDOMAINPATH
@if test
	global TEST
@endif

	system "rm -fr $MHOMETMP $MHOMETMPDB $MHOMETMPODB"
	set ret [mhm_remote_copyin $MHOME $MHOMETMP]
	if {$ret != "ok"} {
		VtUnLock
		set ret [mhm_query_eyn COPY_OUT $MHOME]
		VtLock
		if {$ret == "no"} {
			return no
		}
		# contine, ignoring errors.
		system "rm -fr $MHOMETMP; touch $MHOMETMP"
		# we do make the directory on the other end just in case.
		mhm_remote_mkdir $MDOMAINPATH
		set DIRTY 1
	}
	# if db file is missing it is not an error.
	mhm_remote_copyin $MHOMEDB $MHOMETMPODB
	# ready to attempt open.
	set ret start
	while {"$ret" != "ok"} {
		set ret [mh_open $MHOMETMP]
@if test
		if {"$TEST" == "main_4"} {
			set ret fail
		}
@endif
		switch $ret {
		"ok"	{
		}
		"fail"	{
			VtUnLock
			mhm_error1 OPEN_FAIL $MHOMETMP
			VtLock
			return fail
		}
		"parserr" {
			VtUnLock
			set ret [mhm_query_eyn PARSE $MHOME]
			VtLock
			if {$ret == "no"} {
				return no
			}
			# continue, strip bad lines in file.
			set ret ok
			set DIRTY 1
		}
		}
	}
	set domains [mhm_union_domains]
@if test
	if {"$TEST" == "main_6"} {
		set domains ""
	}
@endif
	if {[llength $domains] == 0} {
		VtUnLock
		mhm_error NO_VDOMAINS
		VtLock
		return fail
	}
	return ok
}

#
# write out current stuff and close
# return ok, fail, or cancel.
#
proc \
mhm_all_put {} \
{
	global app DIRTY
	global MHOME MHOMEDB MHOMETMP MHOMETMPDB MHOMETMPODB MAKEMAP
@if test
	global TEST
@endif

	if {$DIRTY == 1} {
		VtUnLock
		set ret [mhm_query_qync DIRTY notused]
		VtLock
		if {"$ret" == "cancel"} {
			return cancel
		}
		if {"$ret" == "yes"} {
			set ret [mh_write]
@if test
			if {"$TEST" == "main_9"} {
				set ret fail
			}
@endif
			if {"$ret" != "ok"} {
				VtUnLock
				mhm_query_eok WRITE $MHOMETMP
				VtLock
				return fail
			}
			set ret [mhm_remote_copyout $MHOMETMP $MHOME]
@if test
			if {"$TEST" == "main_10"} {
				set ret fail
			}
@endif
			if {"$ret" != "ok"} {
				VtUnLock
				mhm_query_eok COPY_BACK $MHOME
				VtLock
				return fail
			}
			catch {system "$MAKEMAP hash $MHOMETMP < $MHOMETMP >/dev/null 2>&1"} ret
@if test
			if {"$TEST" == "main_11"} {
				set ret fail
			}
@endif
			if {$ret != 0} {
				VtUnLock
				mhm_error1 NO_MAP $MHOME
				VtLock
				return fail
			}
			# copy back db file
			if {[file exists $MHOMETMPODB]} {
				catch {system "cmp $MHOMETMPDB $MHOMETMPODB > /dev/null 2>&1"} ret
			} else {
				set ret 1
			}
			if {$ret == 1} {
				set ret [mhm_remote_copyout $MHOMETMPDB $MHOMEDB]
@if test
				if {"$TEST" == "main_12"} {
					set ret fail
				}
@endif
				if {"$ret" != "ok"} {
					VtUnLock
					mhm_query_eok COPY_BACK $MHOMEDB
					VtLock
					return fail
				}
			}
		}
		set DIRTY 0
	}
	mhm_rebuild_domain_map yes
	return ok
}

proc \
mhm_remoteCommand {host cmd args errMsgId} \
{
	set class    [list sco remoteCommand]
	set instance [GetInstanceList NULL $host]
	set command [list ObjectAction $class $instance $cmd $args]
	set result [SaMakeObjectCall $command]
	set result [lindex $result 0]
	if { [BmipResponseErrorIsPresent result] } {
		set errorStack [BmipResponseErrorStack result]
		set topErrorCode [lindex [lindex $errorStack 0] 0]
		if { $topErrorCode != "SCO_SA_ERRHANDLER_ERR_CONNECT_FAIL" } {
			ErrorPush errorStack 1 $errMsgId
			return {}
		} else {
			ErrorThrow errorStack
			return
		}
	}
	set retVal [BmipResponseActionInfo result]
	return $retVal
}

proc \
mhm_authorized {host} \
{
	set uid [id effective userid]

	if {$uid != 0} {
		set cmd "/sbin/tfadmin"
		set args1 "-t multihome"
		set args2 "-t cpfile"
		if {[ErrorCatch errStack 0 {mhm_remoteCommand $host \
                        $cmd $args1 SCO_USER_MHOME_ERR_TFADMIN} ret1] || \
                    [ErrorCatch errStack 0 {mhm_remoteCommand $host \
                        $cmd $args2 SCO_USER_MHOME_ERR_TFADMIN} ret2]} {
                        return fail
		}
	}
	return ok
}

proc \
quit_cb { {save {}} cbs  } \
{
	global status_line

	set ret "ok"

	# save everything
	if {"$save" == "" || $save == 1} {
		VtLock
		SaStatusBarSet $status_line [mhm_msg SAVING]
		set ret [mhm_all_put]
		VtUnLock
	}

	switch $ret {
	"ok" {
		quit
	}
	"fail" {
		return
	}
	"cancel" {
		return
	}
	}
	error "unknown quit"
}

proc \
quit {} \
{
	VtClose
	cleanup
	exit
}

proc \
cleanup {} \
{
	global MHOMETMP MHOMETMPDB MHOMETMPODB

	system "rm -fr $MHOMETMP $MHOMETMPDB $MHOMETMPODB"
}

proc \
main {} \
{
	global app DIRTY status_line main_list outerMainForm mainscreen_db
	global mhm_host_name versionNumber

	# if command line mode, this routine does not return
	mhm_cmd_line_main

	mhm_setcat SCO_USER_MHOME
#		 VtOpen appname helpname
	set app [VtOpen multihome [mhm_msg HELPBOOK]]
	set versionString [mhm_msg APP_TITLE]
	set versionString "$versionString $versionNumber"
	VtSetAppValues $app -versionString $versionString

	set mhm_short_name [lindex [split $mhm_host_name "."] 0]
	set outerMainForm [ \
		VtFormDialog $app.main \
		-resizable FALSE \
		-wmCloseCallback "quit_cb 1" \
		-title [mhm_msg1 TITLE $mhm_short_name] \
		]
	mhm_build_widgets $outerMainForm

	VtShow $outerMainForm
	VtLock

	# form is visible and we are locked, now init stuff
	set DIRTY 0
	# check if host if ok
	if {"[mhm_host_check $mhm_host_name]" != "ok"} {
		VtUnLock
		mhm_error1 BADHOST $mhm_host_name
		quit
	}	
	# check if authorized
	if {"[mhm_authorized $mhm_host_name]" == "fail"} {
		SaDisplayNoAuths $outerMainForm.noAuths [mhm_msg APP_TITLE] \
		"quit_cb 0" $mhm_short_name
		VtUnLock
		VtMainLoop
		return
	}
	# get virtual domain list and users file.
	if {"[mhm_all_get]" != "ok"} {
		quit
	}
	# display mainlist
	mhm_display_mainlist
	VtUnLock
	SaStatusBarClear $status_line
	VtMainLoop
}

@if test
set TEST ""
set shortcut no
while {[lindex $argv 0] == "-test"} {
	lvarpop argv
	set tmp [lvarpop argv]
	if {"$tmp" == "shortcut"} {
		set shortcut yes
		mhm_cmd_shortcut
	} else {
		set TEST $tmp
	}
}
if {"$shortcut" == "yes"} {
	mhm_cmd_quit 0
}
@endif

ErrorTopLevelCatch {
# hostname flag.
if {"[lindex $argv 0]" == "-h"} {
	lvarpop argv
	set mhm_host_name [lvarpop argv]
	if {"$mhm_host_name" == ""} {
		echo "multihome: No host name"
		exit 1
	}
}

# edit dialog boxes
if {[lindex $argv 0] == "-mhm_stand_edit"} {
	mhm_stand_edit [lindex $argv 1] [lindex $argv 2]
	exit
}

# open host dialog box
if {[lindex $argv 0] == "-mhm_stand_host"} {
	mhm_stand_host [lindex $argv 1]
	exit
}

main} "multihome"
