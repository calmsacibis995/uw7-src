#ident "@(#)flags.tcl	11.1"
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
# our channel flag editting stuff goes in this file.
#
# only mag_stand_flags is called externally.

# The Drawn list has a bug in scroll bar sizing:
# the scroll bars are not sized properly when you add an item.
# This makes it impossible to scroll far enough to see the longer lines.
# The scroll bars are sized corectly when you remove an item.
# So I add and remove a dummy item each time I add to a list.
# The routine is called mag_stand_two_drawnlist_bug.
# If this bug gets fixed then you can remove this routine and it's references.

# callback for "browse" button on flags type.
# this routine makes our vertical two list in a dialog box.
proc \
mag_stand_flags { cbs } \
{
	global app RESPATH MS
	global mag_stand_text
	global mag_stand_text mag_two_form mag_two_list1 mag_two_list2

	# get flags from text widget
	set value [VtGetValues $mag_stand_text -value]
	set isCharm [VtInfo -charm]

	set mag_two_form [ \
		VtFormDialog $app.mailchild.flags \
		-title [mag_msg CH_FLAGS] \
		-ok -okCallback mag_stand_flags_ok \
		-wmCloseCallback mag_stand_flags_quit \
	]
	VtLabel $mag_two_form.label1 -label [mag_msg FLAGS_LIST1]
	set lists [mag_stand_flags_to_lists $value]
	set fieldlist1 [lindex $lists 0]
	set fieldlist2 [lindex $lists 1]
	# get longest entry
	set max [mag_stand_two_max $fieldlist1 $fieldlist2]
	set formatlist [list [list STRING 3] [list STRING $max]]
	set mag_two_list1 [ \
		VtDrawnList $mag_two_form.list1 \
		-leftSide FORM -rightSide FORM \
		-columns 75 \
		-rows 8 \
		-selection MULTIPLE \
		-formatList $formatlist \
		-recordList $fieldlist1 \
		-horizontalScrollBar 1 \
	]
	set tmp [VtLabel $mag_two_form.label2 -label [mag_msg FLAGS_LIST2]]
	set tmp1 [VtPushButton $mag_two_form.btndn \
		-leftSide 44 \
		-topSide $mag_two_list1 \
		-callback mag_stand_two_dn \
	]
	set tmp2 [VtPushButton $mag_two_form.btnup \
		-leftSide $tmp1 \
		-topSide $mag_two_list1 \
		-callback mag_stand_two_up]
	if {$isCharm} {
		VtSetValues $tmp1 \
			-label [mag_msg MENU_DN] \
			-labelCenter
		VtSetValues $tmp2 \
			-label [mag_msg MENU_UP] \
			-labelCenter
	} else {
		VtSetValues $tmp1 \
			-pixmap $RESPATH/next.px
		VtSetValues $tmp2 \
			-pixmap $RESPATH/prev.px
	}
	set mag_two_list2 [ \
		VtDrawnList $mag_two_form.list2 \
		-leftSide FORM -rightSide FORM \
		-bottomSide FORM \
		-columns 75 \
		-rows 7 \
		-selection MULTIPLE \
		-formatList $formatlist \
		-recordList $fieldlist2 \
		-horizontalScrollBar 1 \
	]
	mag_stand_two_drawnlist_bug $mag_two_list1
	mag_stand_two_drawnlist_bug $mag_two_list2
	VtShow $mag_two_form
}

# callback for ok button of flags dialog
proc \
mag_stand_flags_ok { cbs } \
{
	global mag_stand_text
	global mag_two_list1 mag_two_list2 mag_two_form

	# get flags from drawn list
	set items [VtDrawnListGetItem $mag_two_list2 -all]
	set flags ""
	foreach item $items {
		set flag [lindex $item 0]
		set flags "$flags$flag"
	}
	# put value back into text widget
	VtSetValues $mag_stand_text -value $flags

	VtDestroy $mag_two_form
}

# our quit callback
proc \
mag_stand_flags_quit { cbs } \
{
	global mag_two_form

	VtDestroy $mag_two_form
}

# convert our list of flags to the onscreen lists ready to be passed in
# returns a list of lists, top and bottom of two lists respectively
proc \
mag_stand_flags_to_lists { flags } \
{
	# build master list of characters and description strings
	set allflags "aAbcCdDeEfFghIjkKlLmMnopPqrsSuUwxX035789:|/@"
	set regularflags aAbcCdDeEfFghIjkKlLmMnopPqrsSuUwxX035789
	set specialflags [list ": COLON" "| PIPE" "/ SLASH" "@ AT"]
	set length [clength $regularflags]
	set list1 ""
	set list2 ""
	# regular flags
	loop i 0 $length {
		set flag [cindex $regularflags $i]
		set which [string first $flag $flags]
		eval "set mname F_$flag"
		set msg [mag_msg $mname]
		set item [list $flag $msg]
		if {$which < 0} {
			lappend list1 $item
		} else {
			lappend list2 $item
		}
	}
	# special flags, characters that aren't valid in a msg cat name
	foreach i $specialflags {
		set flag [lindex $i 0]
		set name [lindex $i 1]
		set which [string first $flag $flags]
		eval "set mname F_$name"
		set msg [mag_msg $mname]
		set item [list $flag $msg]
		if {$which < 0} {
			lappend list1 $item
		} else {
			lappend list2 $item
		}
	}
	# now check for unknown flags
	set length [clength $flags]
	loop i 0 $length {
		set flag [cindex $flags $i]
		set found [string first $flag $allflags]
		if {$found >= 0} {
			continue
		}
		set item [list $flag [mag_msg F_UNKNOWN]]
		lappend list2 $item
	}
	set list1 [lsort $list1]
	set list2 [lsort $list2]
	return [list $list1 $list2]
}

# two list up button callback
proc \
mag_stand_two_up { cbs } \
{
	global mag_two_list1 mag_two_list2

	set newitems [VtDrawnListGetSelectedItem $mag_two_list2 -byRecordList]
	if {"$newitems" == ""} {
		return
	}
	set positions [VtDrawnListGetSelectedItem $mag_two_list2 -byPositionList]
	VtDrawnListDeleteItem $mag_two_list2 -positionList $positions
	set items [VtDrawnListGetItem $mag_two_list1 -all]

	# merge two lists
	set list "$items $newitems"
	set list [lsort $list]

	VtDrawnListDeleteItem $mag_two_list1 -all
	VtDrawnListAddItem $mag_two_list1 -recordList $list
	# now select the items that were moved so that undo is possible
	VtDrawnListDeselectItem $mag_two_list1 -all
	set positionlist ""
	foreach item $newitems {
		set pos [lsearch $list $item]
		set pos [expr $pos + 1]
		lappend positionlist $pos
	}
	VtDrawnListSelectItem $mag_two_list1 -positionList $positionlist
	mag_stand_two_drawnlist_bug $mag_two_list1
}

# two list down button callback
proc \
mag_stand_two_dn { cbs } \
{
	global mag_two_list1 mag_two_list2

	set newitems [VtDrawnListGetSelectedItem $mag_two_list1 -byRecordList]
	if {"$newitems" == ""} {
		return
	}
	set positions [VtDrawnListGetSelectedItem $mag_two_list1 -byPositionList]
	VtDrawnListDeleteItem $mag_two_list1 -positionList $positions
	set items [VtDrawnListGetItem $mag_two_list2 -all]

	# merge two lists
	set list "$items $newitems"
	set list [lsort $list]

	VtDrawnListDeleteItem $mag_two_list2 -all
	VtDrawnListAddItem $mag_two_list2 -recordList $list
	# now select the items that were moved so that undo is possible
	VtDrawnListDeselectItem $mag_two_list2 -all
	set positionlist ""
	foreach item $newitems {
		set pos [lsearch $list $item]
		set pos [expr $pos + 1]
		lappend positionlist $pos
	}
	VtDrawnListSelectItem $mag_two_list2 -positionList $positionlist
	mag_stand_two_drawnlist_bug $mag_two_list2
}

# get length of largest string in our field list
proc \
mag_stand_two_max { l1 l2 } \
{
	set max 0

	foreach item $l1 {
		set str [lindex $item 1]
		set len [clength $item]
		if {$max < $len} {
			set max $len
		}
	}
	foreach item $l2 {
		set str [lindex $item 1]
		set len [clength $item]
		if {$max < $len} {
			set max $len
		}
	}
	return $max
}

proc \
mag_stand_two_drawnlist_bug { widget } \
{
	# add and delete a dummy (blank) item
	set list [list "dum" "dum"]
	VtDrawnListAddItem $widget -fieldList $list
	VtDrawnListDeleteItem $widget -field 0 "dum"
}
