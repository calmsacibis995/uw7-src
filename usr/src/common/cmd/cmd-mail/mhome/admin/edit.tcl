#ident "@(#)edit.tcl	11.2"

#
# all of our edit actions (callbacks) from the main screen go here.
# There are only three and the aliases one just calls the alias editor:
#	mhm_edit_users
#	mhm_edit_aliases
#	mhm_retire_user

#
# dialog box to allow adding and deleting users from this domain.
#
proc \
mhm_edit_users { domain } \
{
	global outerMainForm mhm_user_form mhm_edit_users_domain mhm_user_list

	set mhm_edit_users_domain $domain
	# build the form.
	set mhm_user_form [VtFormDialog $outerMainForm.users \
		-title [mhm_msg1 USERS_FOR $domain] \
		-okCallback mhi_edit_users_ok \
		-cancelCallback mhi_edit_users_cancel \
		-wmCloseCallback mhi_edit_users_cancel \
	]
	set lbl1 [VtLabel $mhm_user_form.label1 -label [mhm_msg VIRTUAL]\
		-columns 19 -labelLeft]
	set lbl2 [VtLabel $mhm_user_form.label2 -label [mhm_msg SYSTEM] \
		-topSide FORM -leftSide $lbl1]
	set formatlist [list "STRING 20" "STRING 20"]
	set list [VtDrawnList $mhm_user_form.list \
		-autoSelect TRUE \
		-selection MULTIPLE \
		-leftSide FORM \
		-formatList $formatlist \
		-bottomSide FORM \
	]
	set btn_add [VtPushButton $mhm_user_form.add -callback mhi_edit_add \
		-leftSide $list -topSide $lbl2 -label [mhm_msg STR_ADD_USER]]
	set btn_del [VtPushButton $mhm_user_form.del -callback mhi_edit_del \
		-leftSide $list -topSide $btn_add -label [mhm_msg STR_DEL_USER]]
	# load up user list.
	set users [mh_vd_users_get $domain]
	set position 0
	foreach user $users {
		set position [expr $position + 1]
		VtDrawnListAddItem $list \
			-position $position \
			-fieldList $user
	}
	set mhm_user_list $list
	VtShow $mhm_user_form
	VtSetFocus $btn_add
}

# callback for ok button.
proc \
mhi_edit_users_ok { cbs } \
{
	global mhm_user_form mhm_user_list mhm_edit_users_domain DIRTY

	VtLock
	set domain $mhm_edit_users_domain
	set users [VtDrawnListGetItem $mhm_user_list -all]
	set oldusers [mh_vd_users_get $domain]
	if {"$users" != "$oldusers"} {
		mh_vd_users_set $domain $users
		set DIRTY 1
	}
	VtUnLock
	VtDestroy $mhm_user_form
}

# callback for cancel button
proc \
mhi_edit_users_cancel { cbs } \
{
	global mhm_user_form

	VtDestroy $mhm_user_form
}

#
# delete currently selected user
#
proc \
mhi_edit_del { cbs } \
{
	global mhm_user_list

	VtLock
	set poslist [VtDrawnListGetSelectedItem $mhm_user_list -byPositionList]
	if {"$poslist" != "0"} {
		VtDrawnListDeleteItem $mhm_user_list -positionList $poslist
	}
	VtUnLock
}

#
# Add a new user
#
proc \
mhi_edit_add { cbs } \
{
	global mhm_user_form
	global mhm_edit_add_form mhm_edit_add_system mhm_edit_add_virtual
	VtLock
	set form [VtFormDialog $mhm_user_form.addForm \
		-title [mhm_msg USER_ADD] \
		-okCallback mhi_edit_add_ok \
		-cancelCallback mhi_edit_add_cancel \
		-wmCloseCallback mhi_edit_add_cancel]
	set lbl1 [VtLabel $form.label1 -label [mhm_msg SYSTEM]]
	set combo [VtComboBox $form.combo1 \
		-leftSide FORM -rightSide FORM -columns 20 \
		-itemList [mhm_list_users] \
		-valueChangedCallback mhi_edit_add_combo \
		]
	set lbl2 [VtLabel $form.label2 -label [mhm_msg VIRTUAL]]
	set value [VtGetValues $combo -value]
	set text [VtText $form.text -leftSide FORM -rightSide FORM -columns 20 \
		-value $value]
	VtSetValues $combo -callback "mhi_jump $text"
	set ok [VtGetValues $form -ok]
	VtSetValues $text -callback "mhi_jump $ok"

	set mhm_edit_add_form $form
	set mhm_edit_add_system $combo
	set mhm_edit_add_virtual $text
	VtShow $form
	VtSetFocus $combo
	VtUnLock
}

# allow return to act as tab
proc \
mhi_jump { widget cbs } \
{
	VtSetFocus $widget
}

# our cancel callback
proc \
mhi_edit_add_cancel { cbs } \
{
	global mhm_edit_add_form

	VtDestroy $mhm_edit_add_form
}

# combo box data changed, update text field.
proc \
mhi_edit_add_combo { cbs } \
{
	global mhm_edit_add_system mhm_edit_add_virtual

	set value [VtGetValues $mhm_edit_add_system -value]
	VtSetValues $mhm_edit_add_virtual -value $value
}

# all done with add.
proc \
mhi_edit_add_ok { cbs } \
{
	global mhm_edit_add_form
	global mhm_edit_add_system mhm_edit_add_virtual
	global mhm_user_list

	VtLock
	set system [VtGetValues $mhm_edit_add_system -value]
	set virtual [VtGetValues $mhm_edit_add_virtual -value]
	set system [string trim $system]
	set virtual [string trim $virtual]
	set system [string tolower $system]
	set virtual [string tolower $virtual]
	set users [mhm_list_users]
	if {"$system" == ""} {
		VtUnLock
		mhm_error BAD_SUSER
		return
	}
	# system user must exist
	if {[lsearch $users $system] == -1} {
		VtUnLock
		mhm_error BAD_SUSER
		return
	}
	if {"$virtual" == ""} {
		VtUnLock
		mhm_error BAD_VUSER
		return
	}
	# virtual user must be unique
	set users [VtDrawnListGetItem $mhm_user_list -all]
	if {[string first "\{$virtual " $users] >= 0} {
		VtUnLock
		mhm_error DUP_VUSER
		return
	}
	# find place in mhm_user_list, insert it, select it, and scroll to it.
	set index 0
	set newuser [list $virtual $system]
	foreach i $users {
		set index [expr $index + 1]
		if {"$i" > "$newuser"} {
			break
		}
	}
	VtDrawnListAddItem $mhm_user_list \
		-position $index \
		-fieldList $newuser
	VtDrawnListSelectItem $mhm_user_list -position $index

	VtUnLock
	VtDestroy $mhm_edit_add_form
}

#
# edit aliases just calls the alias editor.
#
proc \
mhm_edit_aliases { domain } \
{
	global MHOMEPATH ALIAS ALIAS_EDITOR

	VtLock
	set addr [mhm_remote_alias_to_ip $domain]
	if {"$addr" == "fail"} {
		VtUnLock
		mhm_error1 RESOLV $domain
		return
	}
	set fdir "$MHOMEPATH/$addr/mail"
	if {[mhm_remote_file_exists $fdir] == 0} {
		mhm_remote_mkdir $fdir
	}
	set fname "$MHOMEPATH/$addr/mail/$ALIAS"
	mhm_stand_call_editor $ALIAS_EDITOR $fname
	VtUnLock
}

# general purpose procedure for calling external map editors.
proc \
mhm_stand_call_editor { editor file } \
{
	global PID mhm_host_name mhm_local_host

	set class hash
	set fname $file

	# tmp file names
	set fnamedb "$fname.db"
	set base [exec basename $editor]
	set tmp1 "/tmp/$base.$PID"
	set tmp1db "/tmp/$base.$PID.db"
	set tmp2 "/tmp/$base.1.$PID"
	set tmp2db "/tmp/$base.1.$PID.db"

	# copy in files
	system "rm -fr $tmp1 $tmp1db $tmp2 $tmp2db"
	set ret [mhm_remote_copyin $fname $tmp1]
	set ret [mhm_remote_copyin $fname $tmp2]
	set ret [mhm_remote_copyin $fnamedb $tmp1db]
	set host ""
	if {"$mhm_host_name" != "$mhm_local_host"} {
		set host "-h $mhm_host_name"
	}
	if {"$class" != ""} {
		set cmd "$editor $host -f $class:$tmp2"
	} else {
		set cmd "$editor $host -f $tmp2"
	}
	VtControl -suspend
	catch {system "$cmd"} ret
	VtControl -resume
	if {$ret != 0} {
		VtUnLock
		mhm_error1 EXEC $editor
		VtLock
		system "rm -f $tmp1 $tmp1db $tmp2 $tmp2db"
		return
	}

	# copy back main file
	set ret ok
	if {[file exists $tmp2] == 1} {
		if {[file exists $tmp1] == 1} {
			catch {system "cmp $tmp2 $tmp1 > /dev/null 2>&1"} tmp
			# files are different
			if {$tmp == 1} {
				set ret [mhm_remote_copyout $tmp2 $fname]
			}
		} else {
			set ret [mhm_remote_copyout $tmp2 $fname]
		}
	}
	if {"$ret" != "ok"} {
		VtUnLock
		mhm_error1 COPY_BACK $fname
		VtLock
		system "rm -f $tmp1 $tmp1db $tmp2 $tmp2db"
		return
	}

	# copy back db file
	set ret ok
	if {[file exists $tmp2db] == 1} {
		if {[file exists $tmp1db] == 1} {
			catch {system "cmp $tmp2db $tmp1db > /dev/null 2>&1"} tmp
			# files are different
			if {$tmp == 1} {
				set ret [mhm_remote_copyout $tmp2db $fnamedb]
			}
		} else {
			set ret [mhm_remote_copyout $tmp2db $fnamedb]
		}
	}
	if {"$ret" != "ok"} {
		VtUnLock
		mhm_error1 COPY_BACK $fnamedb
		VtLock
		system "rm -f $tmp1 $tmp1db $tmp2 $tmp2db"
		return
	}
	system "rm -f $tmp1 $tmp1db $tmp2 $tmp2db"
}

#
# retire a user from all domains
#
proc \
mhm_retire_user {} \
{
	global outerMainForm mhm_retire_form mhm_retire_combo

	# build the form.
	set form [VtFormDialog $outerMainForm.retire \
		-title [mhm_msg MENU_RETIRE] \
		-okCallback mhi_retire_ok \
		-cancelCallback mhi_retire_cancel \
		-wmCloseCallback mhi_retire_cancel \
	]
	set ok [VtGetValues $form -ok]
	set combo [VtComboBox $form.combo1 \
		-leftSide FORM -rightSide FORM -columns 20 \
		-itemList [mhm_list_users] \
		-callback "mhi_jump $ok" \
	]

	set mhm_retire_form $form
	set mhm_retire_combo $combo
	VtShow $form
	VtSetFocus $combo
}

# cancel retire operation
proc \
mhi_retire_cancel { cbs } \
{
	global mhm_retire_form

	VtDestroy $mhm_retire_form
}

# do retire operation
proc \
mhi_retire_ok { cbs } \
{
	global mhm_retire_form mhm_retire_combo DIRTY

	VtLock

	# first verify user is valid
	set user [VtGetValues $mhm_retire_combo -value]
	set user [string tolower $user]
	set users [mhm_list_users]
	if {[lsearch $users $user] == -1} {
		VtUnLock
		mhm_error BAD_SUSER
		return
	}
	set ret [mhm_retire $user]
	if {"$ret" == "ok"} {
		set DIRTY 1
	}

	VtUnLock
	VtDestroy $mhm_retire_form
}
