#ident "@(#)objedit.tcl	11.3"
#
# called by ma_activate:
# mai_modify(index) - call edit box for the given property.
# we have numerous different types of edit boxes.
#
# mai_modify calls the following routines in order,
# they are defined in this file:
# mai_pre(object,index) - does pre-dialog box processing, returns object.
# mai_edit(object) - puts up dialog box, gets new value, does not validate it.
#	Returns new object or "" on cancel/error.
# mai_post(object,newobject,index) - does post processing on the new object,
#	validation of the new data.
#	returns new object or "" on error.
# mai_state(object,newobject,index) - 
#	Changing some objects affects other properties, here the
#	other properties are changed.  Could have done it in post,
#	but did it here to make things obvious.
#	this routine is also called by mao_default.
#
# stand(alone) routines are those called from within a child process
# for dialog boxes.  The child sends the new object to stdout or no output
# if error or cancel.
#
# mag_stand_edit(object,outfile) - the main child process entrypoint, it crafts
#	and displays the appropriate dialog box for the object (property).
# mag_stand_ok(cbs) - the main exit routine for child dialog boxes.
#	copies the new data out and makes the new object
#

# only called for properties
proc \
mai_modify { index } \
{
	global mainscreen_db DIRTY MS

	set object [lindex $mainscreen_db $index]
	set newobject [mai_pre $object $index]
	if {"$newobject" == ""} {
		return no
	}
	set newobject [mai_edit $newobject]
	if {"$newobject" == ""} {
		return no
	}
	set newobject [mai_post $newobject $index]
	if {"$newobject" == ""} {
		return no
	}

	if {"$object" != "$newobject"} {
		set DIRTY 1
		set value [lindex $newobject $MS(value)]
		set newobject [mai_prop_set $object $newobject]
		switch "$newobject" {
		"conflict" {
			VtUnLock
			mag_error CONFLICT
			VtLock
			return no
		}
		"badname" {
			VtUnLock
			mag_error1 INVALID $value
			VtLock
			return no
		}
		}

		set mainscreen_db [lreplace $mainscreen_db $index $index $newobject]
		# redraw main screen
		set stop [expr $index + 1]
		mag_display_mainlist $index $stop $index
		mai_state $object $newobject $index
	}
	return yes
}

proc \
mai_pre { object index } \
{
	global mainscreen_db MS

	set type [lindex $object $MS(type)]
	set value [lindex $object $MS(value)]
	set name [lindex $object $MS(name)]

	switch "$type" {
	"chtable" {
		# cannot edit if channel not of type table.
		set chindex [mai_ch_find $index]
		# find channel table type
		set ctindex [mai_prop_find $chindex ctable]
		set tmpobject [lindex $mainscreen_db $ctindex]
		set ctable [lindex $tmpobject $MS(value)]
		if {"$ctable" != "[mag_msg TTFILE]"} {
			VtUnLock
			mag_error FILE_ONLY
			VtLock
			return ""
		}
	}
	"group" {
		# user must be set before group can be set
		set chindex [mai_ch_find $index]
		# find channel user setting
		set ctindex [mai_prop_find $chindex cuser]
		set tmpobject [lindex $mainscreen_db $ctindex]
		set cuser [lindex $tmpobject $MS(value)]
		if {"$cuser" == "[mag_msg DEFAULT]"} {
			VtUnLock
			mag_error USER_FIRST
			VtLock
			return ""
		}
	}
	"fromdom" {
		# build enumerated list from host name
		# find host name
		set length [llength $mainscreen_db]
		loop i 0 $length {
			set tmplist [lindex $mainscreen_db $i]
			set tmpclass [lindex $tmplist $MS(class)]
			if {"$tmpclass" == "container"} {
				continue
			}
			set tmpname  [lindex $tmplist $MS(name)]
			if {"$tmpname" == "host"} {
				set hindex $i
				break
			}
		}
		# build enumerated list
		set host [lindex $tmplist $MS(value)]
		set tokens [split $host "."]
		set base [lindex $tokens 0]
		set enum ""
		set length [llength $tokens]
		set length [expr $length - 1]
		loop i 0 $length {
			set itemlist "[lrange $tokens $i end]"
			set item [join $itemlist "."]
			lappend enum $item
		}
		lappend enum $base
		set object [lreplace $object $MS(criteria) $MS(criteria) $enum]
	}
	}
	return $object
}

proc \
mai_edit { object } \
{
	global ME PID mag_host_name
@if test
	global TEST
@endif

	# file is used to pass results to preserve stdout for child process.
	set file "/tmp/mag.$PID"
	system "rm -f $file"
	VtControl -suspend
	set escape [mag_escape $object]
@if test
	if {"$TEST" != ""} {
		system "$ME -test $TEST -h $mag_host_name -mag_stand_edit \"$escape\" $file"
	} else {
@endif
	system "$ME -h $mag_host_name -mag_stand_edit \"$escape\" $file"
@if test
	}
@endif
	VtControl -resume
	if {[file exists $file] == 1} {
		set newobject [exec cat $file]
		system "rm -f $file"
	} else {
		set newobject ""
	}
	return $newobject
}

proc \
mai_post { object index } \
{
	global mainscreen_db MS

	set oldobject [lindex $mainscreen_db $index]
	set type [lindex $object $MS(type)]
	set name [lindex $object $MS(name)]
	set value [lindex $object $MS(value)]
	set criteria [lindex $object $MS(criteria)]

	set error ""
	set arg $value
	# type checking
	switch "$type" {
	"fqdn" {
		if {"$value" == ""} {
			set error FQDN
		}
		# no blanks allowed
		if {[string first " " $value] >= 0} {
			set error FQDN
		}
	}
	"ofqdn" {
		if {"$value" == ""} {
			set value [mag_msg NONE]
			set object [lreplace $object \
				$MS(value) $MS(value) $value]
		}
		# no blanks allowed
		if {[string first " " $value] >= 0} {
			set error FQDN
		}
	}
	"fromdom" {
		# no blanks allowed
		if {[string first " " $value] >= 0} {
			set error FQDN
		}
	}
	"intrange" {
		if {[catch {expr $value}] != 0} {
			set value "1"
			set error INTEGER
		}
		if {"$value" == ""} {
			set error NULL
			set value 1
		}
		set test [mag_trim_zero $value]
		if {"$test" != "$value"} {
			set value $test
			set arg $test
			set object [lreplace $object $MS(value) $MS(value) $value]
		}
		if {"[csubstr $value 0 1]" == "-"} {
			set test [csubstr $value 1 end]
		}
		if {[ctype digit $test] == 0} {
			set error INTEGER
		} else {
			set t1 [lindex $criteria 0]
			set t2 [lindex $criteria 1]
			if {$value < $t1 || $value > $t2} {
				set error INTEGER
			}
		}
	}
	"ointrange" {
		if {"$value" == ""} {
			set value [mag_msg NONE]
			set object [lreplace $object $MS(value) $MS(value) $value]
		}
		if {"$value" != "[mag_msg NONE]"} {
			if {[catch {expr $value}] != 0} {
				set value [lindex $criteria 0]
				set error INTEGER
			}
			set test [mag_trim_zero $value]
			if {"$test" != "$value"} {
				set value $test
				set arg $test
				set object [lreplace $object $MS(value) $MS(value) $value]
			}
			if {"[csubstr $value 0 1]" == "-"} {
				set test [csubstr $value 1 end]
			}
			if {[ctype digit $test] == 0} {
				set error INTEGER
			} else {
				set t1 [lindex $criteria 0]
				set t2 [lindex $criteria 1]
				if {$value < $t1 || $value > $t2} {
					set error INTEGER
				}
			}
		}
	}
	"enum" {
		# check is one of the list
		if {"$value" == ""} {
			set error NULL
		} else {
			set error INVALID
			foreach i $criteria {
				if {"$i" == "$value"} {
					set error ""
					break
				}
			}
		}
	}
	"umask" {
		if {"$value" == ""} {
			set error NULL
			set value "01"
		}
		if {[ctype digit $value] == 0} {
			set error INVALID
		}
                if {[string first 8 $value] >= 0} {
			set error INVALID
		}
		if {[string first 9 $value] >= 0} {
			set error INVALID
		}
		if {"[cindex $value 0]" != "0"} {
			set value "0$value"
			set object [lreplace $object \
				$MS(value) $MS(value) $value]
		}
		if {"$error" == ""} {
			if {$value > 0777} {
				set error INVALID
			}
		}
	}
	"alias" {
		if {"$value" == ""} {
			set error NULL
		} else {
			# have to modify label
			set test [csubstr $value 0 4]
			if {"$test" == "nis:"} {
				set label [mag_msg MAP]
			} else {
				set label [mag_msg FILE]
			}
			set object [lreplace $object \
				$MS(label) $MS(label) $label]
		}
	}
	"rstring" {
		if {"$value" == ""} {
			set error NULL
		}
		# no blanks allowed
		if {[string first " " $value] >=0} {
			set error INVALID
		}
	}
	"string" {
	}
	"path" {
		if {"$value" == ""} {
			set error NULL
		}
		# no blanks allowed
		if {[string first " " $value] >=0} {
			set error INVALID
		}
		# warning if path does not exist
		if {"$error" == ""} {
			if {[mag_remote_file_exists $value] == 0} {
				VtUnLock
				mag_warn1 NOEXIST $value
				VtLock
			}
		}
	}
	"chpath" {
		# warn if path not exist, same as path
		if {"$value" == ""} {
			set error NULL
		}
		# no blanks allowed
		if {[string first " " $value] >=0} {
			set error INVALID
		}
		# warning if path does not exist
		if {"$error" == "" && "$value" != "SMTP"} {
			if {[mag_remote_file_exists $value] == 0} {
				VtUnLock
				mag_warn1 NOEXIST $value
				VtLock
			}
		}
	}
	"chtable" {
		if {"$value" == ""} {
			set error NULL
		}
		# no blanks allowed
		if {[string first " " $value] >=0} {
			set error INVALID
		}
		# warning if path does not exist
		if {"$error" == ""} {
			if {[mag_remote_file_exists $value] == 0} {
				VtUnLock
				mag_warn1 NOEXIST $value
				VtLock
			}
		}
	}
	"flags" {
		# no blanks allowed
		if {[string first " " $value] >=0} {
			set error INVALID
		}
	}
	"user" {
		# must be in list if not a number
		if {[ctype digit $value] == 0} {
			set users "[mag_msg DEFAULT] [mag_list_users]"
			if {[lsearch $users $value] == -1} {
				set error INVALID
			}
		}
	}
	"group" {
		# must be in list if not a number
		if {[ctype digit $value] == 0} {
			set groups "[mag_msg DEFAULT] [mag_list_groups]"
			if {[lsearch $groups $value] == -1} {
				set error INVALID
			}
		}
	}
	}

	switch $name {
	"ctable" {
		# validate channel table type
		set list [mag_table_enum]
		if {[lsearch $list $value] == -1} {
			VtUnLock
			mag_error1 INVALID $value
			VtLock
			return ""
		}
		# cannot switch to baduser if not last channel.
		set old [mag_table_to_internal [lindex $oldobject $MS(value)]]
		set new [mag_table_to_internal $value]
		if {"$old" != "baduser" && "$new" == "baduser"} {
			set chname [lindex $object $MS(chname)]
			set chlist [mai_ch_names_get]
			set chindex [lsearch $chlist $chname]
			set chlen [llength $chlist]
			set expect [expr $chlen - 1]

			# channel not last
			if {$chindex != $expect} {
				VtUnLock
				mag_error CH_NOT_LAST
				VtLock
				return ""
			}

			# now remove "mov" action from our channel object
			set chindex [mai_ch_find $index]
			set channel [lindex $mainscreen_db $chindex]
			set action [lindex $channel $MS(action)]
			set mov [lsearch $action mov]
			if {$mov >= 0} {
				set action [lreplace $action $mov $mov]
			}
			set channel [lreplace $channel \
				$MS(action) $MS(action) $action]
			set mainscreen_db [lreplace $mainscreen_db \
				$chindex $chindex $channel]
		}
		if {"$old" == "baduser" && "$new" != "baduser"} {
			# restore "mov" action to our channel object
			set chindex [mai_ch_find $index]
			set channel [lindex $mainscreen_db $chindex]
			set action [lindex $channel $MS(action)]
			set mov [lsearch $action mov]
			if {$mov == -1} {
				lappend action mov
			}
			set channel [lreplace $channel \
				$MS(action) $MS(action) $action]
			set mainscreen_db [lreplace $mainscreen_db \
				$chindex $chindex $channel]
		}
	}
	"cargs" {
		# channel program arguments cannot be null
		if {"$value" == ""} {
			set error NULL
		}
	}
	}

	if {"$error" != ""} {
		VtUnLock
		mag_error1 $error "$arg"
		VtLock
		return ""
	}

	return $object
}

proc \
mai_state { oldobject object index } \
{
	global mainscreen_db MS

	set type [lindex $object $MS(type)]
	set name [lindex $object $MS(name)]
	set value [lindex $object $MS(value)]
	set criteria [lindex $object $MS(criteria)]

	# Now do state changes where changing one property affects another.
	# This switch is on name, not type like the others in this file
	switch $name {
	"ctable" {
		set old [mag_table_to_internal [lindex $oldobject $MS(value)]]
		set new [mag_table_to_internal $value]
		# if switching to file type and filename is empty, default it.
		if {"$old" != "file" && "$new" == "file"} {
			set chindex [mai_ch_find $index]
			# find channel table file
			set ctindex [mai_prop_find $chindex cfile]
			set tmpobject [lindex $mainscreen_db $ctindex]
			set cfile [lindex $tmpobject $MS(value)]
			if {"$cfile" == ""} {
				set tmpnewobject [mai_prop_def $tmpobject]
				set tmpnewobject [mai_prop_set $tmpobject $tmpnewobject]
				set mainscreen_db [lreplace $mainscreen_db \
					$ctindex $ctindex $tmpnewobject]
				# redraw line item
				set stop [expr $ctindex + 1]
				mag_display_mainlist $ctindex $stop -1
			}
		}
		# if switching to non-file type, blank out the filename.
		if {"$old" == "file" && "$new" != "file"} {
			set chindex [mai_ch_find $index]
			# find channel table file
			set ctindex [mai_prop_find $chindex cfile]
			set tmpobject [lindex $mainscreen_db $ctindex]
			set cfile [lindex $tmpobject $MS(value)]
			if {"$cfile" != ""} {
				set tmpnewobject [lreplace $tmpobject $MS(value) $MS(value) ""]
				set tmpnewobject [mai_prop_set $tmpobject $tmpnewobject]
				set mainscreen_db [lreplace $mainscreen_db \
					$ctindex $ctindex $tmpnewobject]
				# redraw line item
				set stop [expr $ctindex + 1]
				mag_display_mainlist $ctindex $stop -1
			}
		}
	}
	"cuser" {
		# if user is changed to default, we must default the group too
		set old [lindex $oldobject $MS(value)]
		set new $value
		set default [mag_msg DEFAULT]
		if {"$old" != "$default" && "$new" == "$default"} {
			set chindex [mai_ch_find $index]
			# find channel table group
			set ctindex [mai_prop_find $chindex cgroup]
			set tmpobject [lindex $mainscreen_db $ctindex]
			set cgroup [lindex $tmpobject $MS(value)]
			if {"$cgroup" != "$default"} {
				set tmpnewobject [mai_prop_def $tmpobject]
				set tmpnewobject [mai_prop_set $tmpobject $tmpnewobject]
				set mainscreen_db [lreplace $mainscreen_db \
					$ctindex $ctindex $tmpnewobject]
				# redraw line item
				set stop [expr $ctindex + 1]
				mag_display_mainlist $ctindex $stop -1
			}
		}
	}
	"cname" {
		# channel name must change all back links and folder name
		set calias $value
		set chname [mag_channel_to_internal $calias]
		# update channel container
		set chindex [mai_ch_find $index]
		set tmpobject [lindex $mainscreen_db $chindex]
		set tmpobject [lreplace $tmpobject \
			$MS(name) $MS(name) $chname]
		set tmpobject [lreplace $tmpobject \
			$MS(label) $MS(label) $calias]
		set mainscreen_db [lreplace $mainscreen_db \
			$chindex $chindex $tmpobject]
		# update all children
		set start [expr $chindex + 1]
		set length [llength $mainscreen_db]
		loop i $start $length {
			set tmpobject [lindex $mainscreen_db $i]
			set indent [lindex $tmpobject $MS(level)]
			if {$indent != 2} {
				break
			}
			set tmpobject [lreplace $tmpobject \
				$MS(chname) $MS(chname) $chname]
			set mainscreen_db [lreplace $mainscreen_db \
				$i $i $tmpobject]
		}
		# redraw channel container item
		set chindex [mai_ch_find $index]
		set stop [expr $chindex + 1]
		mag_display_mainlist $chindex $stop -1
	}
	"host" {
		# setting host to a non-default name and v.v. affects altnames
		# the back end takes care of this, all we do is close and
		# reopen altnames if it is open.
		set length [llength $mainscreen_db]
		loop i 0 $length {
			set item [lindex $mainscreen_db $i]
			if {"[lindex $item $MS(type)]" == "altname"} {
				break
			}
		}
		# have altname in item and it's index in i.
		set state [lindex $item $MS(state)]
		if {"$state" == "open"} {
			mai_collapse $i
			mai_expand $i
			# reselect hostname
			mag_display_mainlist $index $index $index
		}
	}
	}
}

# This is our stand alone dialog box procedure
proc \
mag_stand_edit { object file } \
{
	global app MS
	global UUX SLOCAL MULTIHOME
	global mag_stand_object mag_stand_form
	global mag_stand_toggle mag_stand_text mag_stand_combo
	global mag_stand_fdir mag_stand_fname mag_stand_outfile

	set type [lindex $object $MS(type)]
	set name [lindex $object $MS(name)]
	set label [lindex $object $MS(label)]
	set value [lindex $object $MS(value)]
	set criteria [lindex $object $MS(criteria)]
	set custom [lindex $object $MS(custom)]

	# globals for callbacks
	set mag_stand_object $object
	set mag_stand_outfile $file

	# init widget server
	set app [VtOpen mailchild "mailadmin"]
	mag_setcat SCO_MAIL_ADMIN

	# std dialog box
	set mag_stand_form [ \
		VtFormDialog $app.mailchild \
		-title [mag_msg TITLE_MOD] \
		-resizable FALSE \
		-ok -okCallback mag_stand_ok \
		-cancel -cancelCallback mag_stand_quit \
		-wmCloseCallback mag_stand_quit \
	]
	set ok [VtGetValues $mag_stand_form -ok]

	# setup dialog box
	switch "$type" {
	"boolean" {
		set tmplabel [VtLabel $mag_stand_form.label -label $label]
		set mag_stand_toggle [ \
			VtToggleButton $mag_stand_form.toggle \
			-label $label \
			-value [mag_bool_to_num $value] \
		]
		set focus $mag_stand_toggle
	}
	"fqdn" {
		set parms ""
		set tmplabel [VtLabel $mag_stand_form.label -label $label]
		set mag_stand_text [ \
			VtText $mag_stand_form.text \
			-topSide $tmplabel \
			-columns 40 \
			-callback "mag_stand_jump $ok" \
		]
		VtSetValues $mag_stand_text -value $value
		VtPushButton $mag_stand_form.browse \
			-leftSide $mag_stand_text \
			-topSide $tmplabel \
			-callback mag_stand_fqdn_browse \
			-label [mag_msg STR_SELECT]
		set focus $mag_stand_text
	}
	"ofqdn" {
		set parms ""
		set tmplabel [VtLabel $mag_stand_form.label -label $label]
		set mag_stand_text [ \
			VtText $mag_stand_form.text \
			-topSide $tmplabel \
			-columns 40 \
			-callback "mag_stand_jump $ok" \
		]
		VtSetValues $mag_stand_text -value $value
		VtPushButton $mag_stand_form.browse \
			-leftSide $mag_stand_text \
			-topSide $tmplabel \
			-callback mag_stand_fqdn_browse \
			-label [mag_msg STR_SELECT]
		if {"$name" == "chost" && \
		    "[lindex $object $MS(chname)]" == "badhost" } {
			set nonelabel [mag_msg STR_NONE_DELAY]
		} else {
			set nonelabel [mag_msg STR_NONE]
		}
		VtPushButton $mag_stand_form.btn \
			-topSide $mag_stand_text \
			-leftSide FORM -rightSide FORM \
			-label "$nonelabel" \
			-callback mag_stand_none
		set focus $mag_stand_text
	}
	"fromdom" {
		VtLabel $mag_stand_form.label -label $label
		set mag_stand_combo [ \
			VtComboBox $mag_stand_form.combo \
			-itemList $criteria \
			-leftSide FORM -rightSide FORM \
			-value $value \
			-callback "mag_stand_jump $ok" \
		]
		set focus $mag_stand_combo
	}
	"intrange" {
		set tmplabel [VtLabel $mag_stand_form.label -label $label]
		set mag_stand_text [ \
			VtText $mag_stand_form.text \
			-columns 10 \
			-callback "mag_stand_jump $ok" \
		]
		set t1 [lindex $criteria 0]
		set t2 [lindex $criteria 1]
		VtLabel $mag_stand_form.range \
			-label "($t1-$t2)" \
			-leftSide $mag_stand_text \
			-topSide $tmplabel
		VtSetValues $mag_stand_text -value $value
		set focus $mag_stand_text
	}
	"ointrange" {
		set tmplabel [VtLabel $mag_stand_form.label -label $label]
		set mag_stand_text [ \
			VtText $mag_stand_form.text \
			-columns 10 \
			-callback "mag_stand_jump $ok" \
		]
		set t1 [lindex $criteria 0]
		set t2 [lindex $criteria 1]
		VtLabel $mag_stand_form.range \
			-label "($t1-$t2)" \
			-leftSide $mag_stand_text \
			-topSide $tmplabel
		VtSetValues $mag_stand_text -value $value
		VtPushButton $mag_stand_form.btn \
			-topSide $mag_stand_text \
			-leftSide FORM -rightSide FORM \
			-label [mag_msg STR_NONE] \
			-callback mag_stand_none
		set focus $mag_stand_text
	}
	"enum" {
		VtLabel $mag_stand_form.label -label $label
		set mag_stand_combo [ \
			VtComboBox $mag_stand_form.combo \
			-itemList $criteria \
			-leftSide FORM -rightSide FORM \
			-value $value \
			-callback "mag_stand_jump $ok" \
		]
		if {"$name" == "flocation"} {
			# add custom folder location callback
			VtSetValues $mag_stand_combo \
				-valueChangedCallback mag_stand_custom
			# add custom folder location text widgets
			VtLabel $mag_stand_form.label1 \
				-label [mag_msg FOLDER_DIR]
			VtLabel $mag_stand_form.label2 \
				-label [mag_msg SHORT_FOLDER_DIR]
			set mag_stand_fdir [ \
				VtText $mag_stand_form.fdir \
				-columns 50 \
			]
			VtLabel $mag_stand_form.label3 \
				-label [mag_msg FOLDER_NAME]
			VtLabel $mag_stand_form.label4 \
				-label [mag_msg SHORT_FOLDER_NAME]
			set mag_stand_fname [ \
				VtText $mag_stand_form.fname \
				-columns 50 \
			]
			VtSetValues $mag_stand_fdir \
				-value [lindex $custom 0] \
				-callback "mag_stand_jump $mag_stand_fname"
			VtSetValues $mag_stand_fname \
				-value [lindex $custom 1] \
				-callback "mag_stand_jump $ok"
			mag_stand_custom cbs
		}
		set focus $mag_stand_combo
	}
	"umask" {
		set tmplabel [VtLabel $mag_stand_form.label -label $label]
		set mag_stand_text [ \
			VtText $mag_stand_form.text \
			-columns 6 \
			-callback "mag_stand_jump $ok" \
		]
		VtLabel $mag_stand_form.range \
			-label "(0-0777)" \
			-leftSide $mag_stand_text \
			-topSide $tmplabel
		VtSetValues $mag_stand_text -value $value
		set focus $mag_stand_text
	}
	"alias" {
		VtLabel $mag_stand_form.label -label [mag_msg ALIAS1]
		set mag_stand_text [ \
			VtText $mag_stand_form.text \
			-columns 50 \
			-callback "mag_stand_jump $ok" \
		]
		set tmp [VtLabel $mag_stand_form.label2 -label [mag_msg ALIAS2]]
		VtSetValues $mag_stand_text -value $value
		VtPushButton $mag_stand_form.btn \
			-topSide $tmp \
			-leftSide FORM -rightSide FORM \
			-label [mag_msg STR_EDIT_ALIAS] \
			-callback mag_stand_alias_edit
		set focus $mag_stand_text
	}
	"rstring" {
		set tmplabel [VtLabel $mag_stand_form.label -label $label]
		set mag_stand_text [ \
			VtText $mag_stand_form.text \
			-columns 50 \
			-callback "mag_stand_jump $ok" \
		]
		VtSetValues $mag_stand_text -value $value
		set focus $mag_stand_text
	}
	"string" {
		set tmplabel [VtLabel $mag_stand_form.label -label $label]
		set mag_stand_text [ \
			VtText $mag_stand_form.text \
			-columns 50 \
			-callback "mag_stand_jump $ok" \
		]
		VtSetValues $mag_stand_text -value $value
		set focus $mag_stand_text
	}
	"path" {
		set tmplabel [VtLabel $mag_stand_form.label -label $label]
		set mag_stand_text [ \
			VtText $mag_stand_form.text \
			-columns 50 \
			-callback "mag_stand_jump $ok" \
		]
		VtSetValues $mag_stand_text -value $value
		set focus $mag_stand_text
	}
	"chpath" {
		set tmplabel [VtLabel $mag_stand_form.label -label $label]
		set programs [list $SLOCAL SMTP $UUX $MULTIHOME]
		set mag_stand_combo [ \
			VtComboBox $mag_stand_form.combo \
			-itemList $programs \
			-leftSide FORM -rightSide FORM \
			-value $value \
			-callback "mag_stand_jump $ok" \
		]
		set focus $mag_stand_combo
	}
	"chtable" {
		set tmplabel [VtLabel $mag_stand_form.label -label $label]
		set mag_stand_text [ \
			VtText $mag_stand_form.text \
			-columns 50 \
			-callback "mag_stand_jump $ok" \
		]
		VtSetValues $mag_stand_text -value $value
		VtPushButton $mag_stand_form.btn \
			-topSide $mag_stand_text \
			-leftSide FORM -rightSide FORM \
			-label [mag_msg STR_EDIT_CTABLE] \
			-callback mag_stand_ctable_edit
		set focus $mag_stand_text
	}
	"domtable" {
		set ret [mag_query_qyn EDIT_DTABLE ""]
		if {"$ret" == "yes"} {
			mag_stand_dtable_edit
		}
		mag_stand_quit cbs
	}
	"flags" {
		set tmplabel [VtLabel $mag_stand_form.label -label $label]
		set mag_stand_text [ \
			VtText $mag_stand_form.text \
			-columns 40 \
			-callback "mag_stand_jump $ok" \
		]
		VtPushButton $mag_stand_form.btn \
			-topSide $tmplabel \
			-leftSide $mag_stand_text -rightSide FORM \
			-label [mag_msg STR_SELECT] \
			-callback mag_stand_flags
		VtSetValues $mag_stand_text -value $value
		set focus $mag_stand_text
	}
	"user" {
		VtLabel $mag_stand_form.label -label $label
		set users "[mag_list_users]"
		set users [lsort $users]
		set users "[mag_msg DEFAULT] $users"
		set mag_stand_combo [ \
			VtComboBox $mag_stand_form.combo \
			-itemList $users \
			-leftSide FORM -rightSide FORM \
			-value $value \
			-callback "mag_stand_jump $ok" \
		]
		set focus $mag_stand_combo
	}
	"group" {
		VtLabel $mag_stand_form.label -label $label
		set groups "[mag_list_groups]"
		set groups [lsort $groups]
		set groups "[mag_msg DEFAULT] $groups"
		set mag_stand_combo [ \
			VtComboBox $mag_stand_form.combo \
			-itemList $groups \
			-leftSide FORM -rightSide FORM \
			-value $value \
			-callback "mag_stand_jump $ok" \
		]
		set focus $mag_stand_combo
	}
	"ruleset" {
		VtLabel $mag_stand_form.label -label $label
		set none [mag_msg NONE]
		switch $name {
		"crecip" {
			set suffix re
		}
		"chrecip" {
			set suffix rh
			set none [mag_msg RULEDEF]
		}
		"csender" {
			set suffix se
		}
		"chsender" {
			set suffix sh
			set none [mag_msg RULEDEF]
		}
		}
		set items [list \
			$none \
			ap822_$suffix \
			aplocal_$suffix \
			ap976_$suffix \
		]
		set mag_stand_combo [ \
			VtComboBox $mag_stand_form.combo \
			-itemList $items \
			-leftSide FORM -rightSide FORM \
			-value $value \
			-callback "mag_stand_jump $ok" \
		]
		set focus $mag_stand_combo
	}
	}

	VtShow $mag_stand_form
	VtSetFocus $focus
	VtMainLoop
}

# ok callback for mag_stand edit
proc \
mag_stand_ok { cbs } \
{
	global mag_stand_object MS
	global mag_stand_toggle mag_stand_text mag_stand_combo
	global mag_stand_fdir mag_stand_fname mag_stand_outfile

	set object $mag_stand_object
	set type [lindex $object $MS(type)]
	set name [lindex $object $MS(name)]

	# extract value and type check it if needed.
	switch "$type" {
	"boolean" {
		set value [VtGetValues $mag_stand_toggle -value]
		set value [mag_num_to_bool $value]
	}
	"fqdn" {
		set value [VtGetValues $mag_stand_text -value]
	}
	"ofqdn" {
		set value [VtGetValues $mag_stand_text -value]
	}
	"fromdom" {
		set value [VtGetValues $mag_stand_combo -value]
	}
	"intrange" {
		set value [VtGetValues $mag_stand_text -value]
	}
	"ointrange" {
		set value [VtGetValues $mag_stand_text -value]
	}
	"enum" {
		set value [VtGetValues $mag_stand_combo -value]
		if {"$name" == "flocation"} {
			set fdir [VtGetValues $mag_stand_fdir -value]
			set fname [VtGetValues $mag_stand_fname -value]
			set custom [list $fdir $fname]
			set object [lreplace $object $MS(custom) $MS(custom) $custom]
		}
	}
	"umask" {
		set value [VtGetValues $mag_stand_text -value]
	}
	"alias" {
		set value [VtGetValues $mag_stand_text -value]
	}
	"rstring" {
		set value [VtGetValues $mag_stand_text -value]
	}
	"string" {
		set value [VtGetValues $mag_stand_text -value]
	}
	"path" {
		set value [VtGetValues $mag_stand_text -value]
	}
	"chpath" {
		set value [VtGetValues $mag_stand_combo -value]
	}
	"chtable" {
		set value [VtGetValues $mag_stand_text -value]
	}
	"flags" {
		set value [VtGetValues $mag_stand_text -value]
	}
	"user" {
		set value [VtGetValues $mag_stand_combo -value]
	}
	"group" {
		set value [VtGetValues $mag_stand_combo -value]
	}
	"ruleset" {
		set value [VtGetValues $mag_stand_combo -value]
	}
	}
	set object [lreplace $object $MS(value) $MS(value) $value]

	# our return value is sent to outfile
	if {[catch {open $mag_stand_outfile w} fd] == 0} {
		puts $fd $object
		close $fd
	}

	VtClose
	exit 0
}

# done with stand-alone dialog box.
proc \
mag_stand_quit { cbs } \
{
	VtClose
	exit 0
}

# callback to set focus to another widget (usually the ok button).
proc \
mag_stand_jump { widget cbs } \
{
	VtSetFocus $widget
}

# callback for "browse" button on fqdn types.
proc \
mag_stand_fqdn_browse { cbs } \
{
	global mag_stand_object MS
	global mag_stand_form

	keylset parms parent $mag_stand_form
	keylset parms instance abc
	keylset parms labeltext [lindex $mag_stand_object $MS(label)]
	keylset parms buttontext [mag_msg STR_SELECT]
	keylset parms userproc mag_stand_fqdn_browse_done
	SaSelectHostDialog $parms $cbs
}

proc \
mag_stand_fqdn_browse_done { cbs } \
{
	global mag_stand_text mag_stand_form

	set value [SaSelectHostGetSelection abc]
	VtSetValues $mag_stand_text -value $value
	set ok [VtGetValues $mag_stand_form -ok]
	mag_stand_jump $ok $cbs
}

# callback for "none" button.
proc \
mag_stand_none { cbs } \
{
	global mag_stand_text

	VtSetValues $mag_stand_text -value [mag_msg NONE]
}

# callback for "edit" button on alias types.
proc \
mag_stand_alias_edit { cbs } \
{
	global mag_stand_text
	global MS app
	global ALIAS_EDITOR

	set value [VtGetValues $mag_stand_text -value]
	set test [csubstr $value 0 4]
	if {"$test" == "nis:"} {
		VtErrorDialog $app.err -block -ok -message [mag_err AFILE_ONLY]
		return
	}
	mag_stand_call_editor $ALIAS_EDITOR $value
}

# callback for "edit" button on channel tables.
proc \
mag_stand_ctable_edit { cbs } \
{
	global mag_stand_object MS app
	global CTABLE_EDITOR

	set object $mag_stand_object
	set value [lindex $object $MS(value)]

	# call channel table editor
	mag_stand_call_editor $CTABLE_EDITOR $value
}

# callback for "edit" button on domain table.
proc \
mag_stand_dtable_edit {} \
{
	global mag_stand_object MS app
	global DTABLE_EDITOR

	set object $mag_stand_object
	set value [lindex $object $MS(value)]

	mag_stand_call_editor $DTABLE_EDITOR $value
}

# callback for combobox on folder location, sets up the two text widgets.
proc \
mag_stand_custom { cbs } \
{
	global mag_stand_object
	global mag_stand_combo
	global mag_stand_fdir mag_stand_fname mag_stand_form
	global MAILSPOOL INBOXNAME

	set value [VtGetValues $mag_stand_combo -value]
	# update sensitivity and fields
	if {"$value" != "[mag_msg FOLDER_CUSTOM]"} {
		VtSetValues $mag_stand_fdir -sensitive 0
		VtSetValues $mag_stand_fname -sensitive 0
		if {"$value" == "[mag_msg FOLDER_SYSTEM]"} {
			VtSetValues $mag_stand_fdir -value $MAILSPOOL
			VtSetValues $mag_stand_fname -value ""
		} else {
			VtSetValues $mag_stand_fdir -value ""
			VtSetValues $mag_stand_fname -value "$INBOXNAME"
		}
		set ok [VtGetValues $mag_stand_form -ok]
		VtSetValues $mag_stand_combo \
			-callback "mag_stand_jump $ok"
	} else {
		VtSetValues $mag_stand_fdir -sensitive 1
		VtSetValues $mag_stand_fname -sensitive 1
		VtSetValues $mag_stand_combo \
			-callback "mag_stand_jump $mag_stand_fdir"
	}
}

# general purpose procedure for calling external map editors.
proc \
mag_stand_call_editor { editor file } \
{
	global PID mag_host_name mag_local_host
@if test
	global TEST
@endif

	VtLock
	# first strip off class if there
	set class ""
	set fname $file
	set pos [string first ":" $file]
	if {$pos >= 0} {
		set class [csubstr $file 0 $pos]
		set start [expr $pos + 1]
		set fname [csubstr $file $start end]
		set class [string trim $class]
		set fname [string trim $fname]
		if {"$class" != "dbm" && "$class" != "hash" &&
			"$class" != "btree" && "$class" != "implicit" } {
			VtUnLock
			mag_error1 INVALID_CLASS $class
			return
		}
	}
	# tmp file names
	set fnamedb "$fname.db"
	set base [exec basename $editor]
	set tmp1 "/tmp/$base.$PID"
	set tmp1db "/tmp/$base.$PID.db"
	set tmp2 "/tmp/$base.1.$PID"
	set tmp2db "/tmp/$base.1.$PID.db"

	# copy in files
	system "rm -fr $tmp1 $tmp1db $tmp2 $tmp2db"
	set ret [mag_remote_copyin $fname $tmp1]
	set ret [mag_remote_copyin $fname $tmp2]
	set ret [mag_remote_copyin $fnamedb $tmp1db]
	set host ""
	if {"$mag_host_name" != "$mag_local_host"} {
		set host "-h $mag_host_name"
	}
	if {"$class" != ""} {
		set cmd "$editor $host -f $class:$tmp2"
	} else {
		set cmd "$editor $host -f $tmp2"
	}
	VtControl -suspend
	catch {system "$cmd"} ret
	VtControl -resume
@if test
	if {"$TEST" == "objedit_test3"} {
		set ret 1
	}
@endif
	if {$ret != 0} {
		VtUnLock
		mag_error1 EXEC $editor
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
				set ret [mag_remote_copyout $tmp2 $fname]
			}
		} else {
			set ret [mag_remote_copyout $tmp2 $fname]
		}
	}
@if test
	if {"$TEST" == "objedit_test4"} {
		set ret fail
	}
@endif
	if {"$ret" != "ok"} {
		VtUnLock
		mag_error1 COPY_BACK $fname
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
				set ret [mag_remote_copyout $tmp2db $fnamedb]
			}
		} else {
			set ret [mag_remote_copyout $tmp2db $fnamedb]
		}
	}
@if test
	if {"$TEST" == "objedit_test5"} {
		set ret fail
	}
@endif
	if {"$ret" != "ok"} {
		VtUnLock
		mag_error1 COPY_BACK $fnamedb
		system "rm -f $tmp1 $tmp1db $tmp2 $tmp2db"
		return
	}
	system "rm -f $tmp1 $tmp1db $tmp2 $tmp2db"
	VtUnLock
}
