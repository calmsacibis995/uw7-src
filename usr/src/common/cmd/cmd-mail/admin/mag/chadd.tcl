#ident "@(#)chadd.tcl	11.1"
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
# mag_channel_create(index) - called by mao_add,
#	index of channels container passed in.
#	this is our create channel dialog.
#
# mai_channel_dialog() - exec channel dialog stand-alone routine.
#	returns "" if cancel.  Returns list if success, list entries are:
#	o - new channel name
#	o - new channel program name.
#	o - new channel table type.
#
# mag_stand_channel(file) - our stand-alone routine for presenting the dialog
#	puts its results into a file.  No output file is for cancel or error.
#
# mai_channel_order_set(name) - set new sequence order for a channel.
#

proc \
mag_channel_create { index } \
{
	global mainscreen_db MS MAILTABLE

	# initial values
	set name [mag_msg NEW]
	set program /etc/mail/$name
	set table [mag_table_to_external file]
	set done no

	while {"$done" == "no"} {
		# call channel dialog first to get new channel data
		set list [list $name $program $table]
		set list [mai_channel_dialog $list]

		# user hit cancel
		if {"$list" == ""} {
			return
		}

		# validate new values
		set name [lindex $list 0]
		set program [lindex $list 1]
		set table [lindex $list 2]

		# channel name cannot conflict
		set list [ma_ch_names_get]
		if {[lsearch $list $name] >= 0} {
			VtUnLock
			mag_error CONFLICT
			VtLock
			continue
		}

		# program name must be entered
		if {"$program" == ""} {
			VtUnLock
			mag_error NULL
			VtLock
			continue
		}
		# no blanks allowed
		if {[string first " " $program] >=0} {
			VtUnLock
			mag_error1 INVALID $program
			VtLock
			continue
		}
		# program name should exist
		if {"$program" != "SMTP"} {
			if {[mag_remote_file_exists $program] == 0} {
				VtUnLock
				set ret [mag_query_qyn NOEXIST_IGNORE $program]
				VtLock
				if {"$ret" == "no"} {
					continue
				}
			}
		}

		# table type must be one of several enumerated values.
		set list [mag_table_enum]
		if {[lsearch $list $table] == -1} {
			VtUnLock
			mag_error1 INVALID_TABLE $table
			VtLock
			continue
		}
		# have picked a set of valid values, now create the channel
		set ret [ma_ch_create $name]
		switch $ret {
		"conflict" {
			VtUnLock
			mag_error CONFLICT
			VtLock
			continue
		}
		"badname" {
			VtUnLock
			mag_error1 INVALID_NAME $name
			VtLock
			continue
		}
		}
		# everything appears ok
		break
	}
	# finish off channel parameters
	ma_ch_equate_set $name P $program
	ma_ch_table_type_set $name [mag_table_to_internal $table]

	# set ordering, add to end, or next to end if baduser is last, we could
	# be baduser as well.  Only one baduser is allowed.
	set ret [mai_channel_order_set $name]
	if {"$ret" != "ok"} {
		ma_ch_delete $name
		# error
		VtUnLock
		mag_error BADUSER_ONLY
		VtLock
		return
	}

	# channel now exists in the back-end, we have to draw it onscreen.

	# collapse onscreen channels container
	mai_collapse $index

	# expand back again, this adds the new channel onscreen.
	mai_expand $index

	# now find the new channel.
	set start [expr $index + 1]
	set stop [llength $mainscreen_db]
	loop i $start $stop {
		set channel [lindex $mainscreen_db $i]
		set tmpname [lindex $channel $MS(name)]
		if {"$name" == "$tmpname"} {
			set found $i
			break
		}
	}

	# set all values in the channel to default values
	mao_default $found no

	# select the new channel
	mag_display_mainlist $found $found $found

	# expand the new channel
	mai_expand $found
}

proc \
mai_channel_dialog { list } \
{
	global ME PID

	# file is used to pass results to preserve stdout for child process.
	set file "/tmp/mag.PID"
	system "rm -f $file"
	VtControl -suspend
	set escape [mag_escape $list]
	system "$ME -mag_stand_channel \"$escape\" $file"
	VtControl -resume
	if {[file exists $file] == 1} {
		set result [exec cat $file]
		system "rm -f $file"
	} else {
		set result ""
	}
	return $result
}

proc \
mag_stand_channel { list file } \
{
	global app MS SLOCAL UUX MULTIHOME
	global mag_stand_name mag_stand_program mag_stand_table
	global mag_stand_outfile

	set app [VtOpen mailchild "mailadmin"]
	mag_setcat SCO_MAIL_ADMIN

	set name [lindex $list 0]
	set program [lindex $list 1]
	set table [lindex $list 2]

	set mag_stand_outfile $file

	# std dialog box.
	set mag_stand_form [ \
		VtFormDialog $app.mailchild \
		-title [mag_msg NEW_CHANNEL] \
		-resizable FALSE \
		-ok -okCallback mag_stand_channel_ok \
		-cancel -cancelCallback mag_stand_channel_quit \
		-wmCloseCallback mag_stand_channel_quit
	]

	VtLabel $mag_stand_form.label1 -label [mag_msg CH_NAME]
	set mag_stand_name [ \
		VtText $mag_stand_form.name \
		-columns 50 \
		-value $name \
	]

	VtLabel $mag_stand_form.label2 -label [mag_msg CH_PROGRAM]
	set programs [list $SLOCAL SMTP $UUX $MULTIHOME]
	set mag_stand_program [ \
		VtComboBox $mag_stand_form.program \
		-itemList $programs \
		-leftSide FORM -rightSide FORM \
		-value $program \
	]

	VtLabel $mag_stand_form.label3 -label [mag_msg CH_TABLE]
	set enum [mag_table_enum]
	set mag_stand_table [ \
		VtComboBox $mag_stand_form.table \
		-itemList $enum \
		-leftSide FORM -rightSide FORM \
		-value $table \
	]
	VtSetValues $mag_stand_name \
		-callback "mag_stand_jump $mag_stand_program"
	VtSetValues $mag_stand_program \
		-callback "mag_stand_jump $mag_stand_table"
	VtSetValues $mag_stand_table \
		-callback "mag_stand_jump [VtGetValues $mag_stand_form -ok]"

	VtShow $mag_stand_form
	VtSetFocus $mag_stand_name
	VtMainLoop
}

proc \
mag_stand_channel_ok { cbs } \
{
	global mag_stand_name mag_stand_program mag_stand_table
	global mag_stand_outfile

	set name [VtGetValues $mag_stand_name -value]
	set program [VtGetValues $mag_stand_program -value]
	set table [VtGetValues $mag_stand_table -value]

	set list [list $name $program $table]

	# our return value is sent to outfile
	if {[catch {open $mag_stand_outfile w} fd] == 0} {
		puts $fd $list
		close $fd
	}

	VtClose
	exit 0
}

proc \
mag_stand_channel_quit { cbs } \
{
	VtClose
	exit 0
}

# attempt to sequence to end of channel list,
# expected to be at end of list.
# we have to put it into the correct order.
# returns ok, fail.
proc \
mai_channel_order_set { name } \
{
	# find out if any other bad user channels
	set names [mai_ch_names_get]
	set found ""
	foreach channel $names {
		if {"$channel" == "$name"} {
			continue
		}
		set type [ma_ch_table_type_get $channel]
		if {"$type" == "baduser"} {
			set found $channel
			break
		}
	}

	# find out if we are baduser
	set type [ma_ch_table_type_get $name]
	if {"$type" == "baduser" && "$found" != ""} {
		# there can only be one baduser channel
		if {"$found" != ""} {
			return fail
		}
	}
	set len [llength $names]
	# subtract ourself off
	set len [expr $len - 1]
	set prev [expr $len - 1]
	if {"$found" == ""} {
		# add to end of list
		ma_ch_sequence_set $name $len
	} else {
		# add in front of bad user channel
		ma_ch_sequence_set $name $prev
		ma_ch_sequence_set $found $len
	}	

	return ok
}
