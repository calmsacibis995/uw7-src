#ident "@(#)bad.tcl	11.5"
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
# our bad host, bad user, UUCP, and multihome editting callbacks are here:
#
# mao_badhost - short cut for creating and editting the badhost channel
# mao_baduser - short cut for creating and editting the baduser channel
# mai_badchan - generic thing to do the work for the above two routines
# mao_mhome - short cut for enabling and disabling the multihome channel
# mao_uucp - short cut for enabling and disabling the UUCP channel
#
# our overal strategy is to find the channel named badhost or baduser
# and edit the "forward to host" field.
#
# the only additional processing is that the badhost and baduser channels
# are created and deleted automatically by this short-cut.
# To make this work we find or create the badhost/baduser channel,
# select the host field, and call mai_modify on it.  Then we do
# the appropriate thing with the result.

proc \
mao_badhost {} \
{
	mai_badchan badhost
}

proc \
mao_baduser {} \
{
	mai_badchan baduser
}

# generic thing to edit badhost/baduser channels
proc \
mai_badchan { badchan } \
{
	global mainscreen_db MS main_list DIRTY

	# find channels container
	set length [llength $mainscreen_db]
	loop i 0 $length {
		set channels [lindex $mainscreen_db $i]
		set type [lindex $channels $MS(type)]
		if {"$type" == "channels"} {
			break
		}
	}

	# have channels container
	set chs_index $i
	set chs_state [lindex $channels $MS(state)]
	if {"$chs_state" == "open"} {
		mai_collapse $chs_index
	}

	# setup for search
	set chans [mai_ch_names_get]
	set found [lsearch $chans $badchan]

	# check that baduser channel does not already exist under another name.
	# We always need to know this for channel ordering later as
	# the baduser channel must always go last.
	set baduserfound -1
	set last [llength $chans]
	set badalias $badchan
	if {$last > 0} {
		set last [expr $last - 1]
		set lastname [lindex $chans $last]
		set lasttype [ma_ch_table_type_get $lastname]
		# found it
		if {"$lasttype" == "baduser"} {
			set baduserfound $last
			if {"$badchan" == "baduser"} {
				set found $last
				set badalias $lastname
			}
		}
	}

	# create channel if needed
	if {$found == -1} {
		set ret [ma_ch_create $badchan]
		ma_ch_equate_set $badchan P "\[IPC\]"
		if {"$badchan" == "badhost"} {
			ma_ch_table_type_set $badchan remote
		}
		if {"$badchan" == "baduser"} {
			ma_ch_table_type_set $badchan baduser
		}
		# have channel name, need to get channel object
		set children [mai_get_children $channels]
		foreach child $children {
			# default for our new channel
			set tmpname [lindex $child $MS(name)]
			if {"$tmpname" != "$badchan"} {
				continue
			}
			set tmpchildren [mai_get_children $child]
			foreach tmpchild $tmpchildren {
				set tmplist [mai_prop_def $tmpchild]
				mai_prop_set $tmpchild $tmplist
			}
			break
		}
		# now set ordering
		mai_channel_order_set $badchan
	}

	# now expand the channels container and $badchan channel.
	# and select the host field and call mai_modify on it.
	mai_expand $chs_index

	# looking for badalias now
	set length [llength $mainscreen_db]
	loop i 0 $length {
		set channel [lindex $mainscreen_db $i]
		set type [lindex $channel $MS(type)]
		if {"$type" != "channel"} {
			continue
		}
		set name [lindex $channel $MS(name)]
		if {"$name" == "$badalias"} {
			set badchanindex $i
			break
		}
	}

	# have our $badchan index, find host field
	mai_expand $badchanindex
	set length [llength $mainscreen_db]
	loop i $badchanindex $length {
		set prop [lindex $mainscreen_db $i]
		set class [lindex $prop $MS(class)]
		if {"$class" != "property"} {
			continue
		}
		set name [lindex $prop $MS(name)]
		if {"$name" == "chost"} {
			set hostindex $i
			break
		}
	}

	# have our host index, select in onscreen
	mag_display_mainlist $hostindex $hostindex $hostindex

	# now scroll if necessary to get selected item on screen
	set rows [VtGetValues $main_list -rows]
	set pos [VtGetValues $main_list -topItemPosition]
	set spos [expr $hostindex - $pos + 2]
	# if offscreen then fix up so host is last line on screen
	if {$spos >= $rows || $spos < 0} {
		set mpos [expr $hostindex - $rows + 2]
		VtSetValues $main_list -topItemPosition $mpos
	}

	# call mai_modify
	set modret [mai_modify $hostindex]

	# get host property's value:
	#       - if NONE, and this is baduser channel then we must always
	#         delete this channel
	#       - if this is badhost channel, host can be either NONE or
	#         a hostname-- so we delete only if the user had pressed
	#         Cancel, and the channel did not already exist!
	set object [lindex $mainscreen_db $hostindex]
	set value [lindex $object $MS(value)]
	if {"$badchan" == "baduser" && "$value" == "[mag_msg NONE]" || \
	    "$badchan" == "badhost" && "$modret" == "no" && $found == -1} {
		# select our $badchan container
		mao_delete $badchanindex
	}
	if {"$badchan" == "badhost" && "$modret" == "yes"} {
		set DIRTY 1
	}
}

# generic thing to enable and disable the multihome channel
proc \
mao_mhome {} \
{
	global mainscreen_db MS main_list MULTIHOME MDOMAINDB DIRTY

	# find channels container
	set length [llength $mainscreen_db]
	loop i 0 $length {
		set channels [lindex $mainscreen_db $i]
		set type [lindex $channels $MS(type)]
		if {"$type" == "channels"} {
			break
		}
	}

	# have channels container
	set chs_index $i
	set chs_state [lindex $channels $MS(state)]
	if {"$chs_state" == "open"} {
		mai_collapse $chs_index
	}

	# setup for search
	set chans [mai_ch_names_get]

	# check if a multihome channel exists, we depend on the name
	set mhomefound 0
	set mhomeindex [lsearch -exact $chans multihome]
	if {$mhomeindex >= 0} {
		set mhomefound 1
	}

	if {$mhomefound == 0} {
		# create multihome channel
		set ret [ma_ch_create multihome]
		ma_ch_equate_set multihome P $MULTIHOME
		ma_ch_table_type_set multihome file
		set children [mai_get_children $channels]
		foreach child $children {
			# default for our new channel
			set tmpname [lindex $child $MS(name)]
			if {"$tmpname" != "multihome"} {
				continue
			}
			set tmpchildren [mai_get_children $child]
			foreach tmpchild $tmpchildren {
				set tmplist [mai_prop_def $tmpchild]
				mai_prop_set $tmpchild $tmplist
			}
			break
		}
		ma_ch_table_file_set multihome $MDOMAINDB
		# set the V macro-- check_rcpt depends upon this to
		# know whether virtual domains map exists, and it should
		# check hosts in the map
		ma_multihome_macro_set 1
		# now set ordering
		ma_ch_sequence_set multihome 0
		set order 1
		foreach i $chans {
			ma_ch_sequence_set $i $order
			set order [expr $order + 1]
		}
	} else {
		# delete multihome channel
		set order 0
		foreach i $chans {
			if {"$i" == "multihome"} {
				ma_ch_delete multihome
				continue
			}
			ma_ch_sequence_set $i $order
			set order [expr $order + 1]
		}
		# unset the V macro-- check_rcpt depends upon this to
		# know whether virtual domains map exists and it should
		# check hosts in the map
		ma_multihome_macro_set 0
	}

	# now expand the channels container
	mai_expand $chs_index

	# now scroll if necessary to make all channels visible
	set rows [VtGetValues $main_list -rows]
	set pos [VtGetValues $main_list -topItemPosition]
	set last [llength $mainscreen_db]
	set spos [expr $last - $pos + 2]
	# if offscreen then fix up so last channel is last line on screen
	if {$spos >= $rows || $spos < 0} {
		set mpos [expr $last - $rows + 2]
		VtSetValues $main_list -topItemPosition $mpos
	}
	set DIRTY 1
}

# generic thing to enable and disable the uucp channel
proc \
mao_uucp {} \
{
	global mainscreen_db MS main_list UUX UUCPDB DIRTY

	# find channels container
	set length [llength $mainscreen_db]
	loop i 0 $length {
		set channels [lindex $mainscreen_db $i]
		set type [lindex $channels $MS(type)]
		if {"$type" == "channels"} {
			break
		}
	}

	# have channels container
	set chs_index $i
	set chs_state [lindex $channels $MS(state)]
	if {"$chs_state" == "open"} {
		mai_collapse $chs_index
	}

	# setup for search
	set chans [mai_ch_names_get]

	# check if a UUCP channel exists, we depend on the name
	set uucpfound 0
	set uucpindex [lsearch -exact $chans UUCP]
	if {$uucpindex >= 0} {
		set uucpfound 1
	}

	if {$uucpfound == 0} {
		# create UUCP channel
		set ret [ma_ch_create UUCP]
		ma_ch_equate_set UUCP P $UUX
		ma_ch_table_type_set UUCP UUCP
		set children [mai_get_children $channels]
		foreach child $children {
			# default for our new channel
			set tmpname [lindex $child $MS(name)]
			if {"$tmpname" != "UUCP"} {
				continue
			}
			set tmpchildren [mai_get_children $child]
			foreach tmpchild $tmpchildren {
				set tmplist [mai_prop_def $tmpchild]
				mai_prop_set $tmpchild $tmplist
			}
			break
		}
		# now set ordering
		ma_ch_sequence_set UUCP 0
		set order 1
		foreach i $chans {
			ma_ch_sequence_set $i $order
			set order [expr $order + 1]
		}
	} else {
		# delete UUCP channel
		set order 0
		foreach i $chans {
			if {"$i" == "UUCP"} {
				ma_ch_delete UUCP
				continue
			}
			ma_ch_sequence_set $i $order
			set order [expr $order + 1]
		}
	}

	# now expand the channels container
	mai_expand $chs_index

	# now scroll if necessary to make all channels visible
	set rows [VtGetValues $main_list -rows]
	set pos [VtGetValues $main_list -topItemPosition]
	set last [llength $mainscreen_db]
	set spos [expr $last - $pos + 2]
	# if offscreen then fix up so last channel is last line on screen
	if {$spos >= $rows || $spos < 0} {
		set mpos [expr $last - $rows + 2]
		VtSetValues $main_list -topItemPosition $mpos
	}
	set DIRTY 1
}
